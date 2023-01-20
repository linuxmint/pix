/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2019 Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include "glib-utils.h"
#include "gtk-utils.h"
#include "gth-main.h"
#include "gth-vfs-tree.h"


enum {
	PROP_0,
	PROP_SHOW_HIDDEN,
	PROP_FOLDER_URI
};


enum {
	CHANGED,
	LAST_SIGNAL
};


struct _GthVfsTreePrivate {
	GFile    *folder;
	char     *folder_uri;
	gulong    monitor_folder_changed_id;
	gulong    monitor_file_renamed_id;
	gboolean  show_hidden;
	gboolean  tree_root_is_vfs_root;
	gboolean  root_loaded;
};


static guint gth_vfs_tree_signals[LAST_SIGNAL] = { 0 };


G_DEFINE_TYPE_WITH_CODE (GthVfsTree,
			 gth_vfs_tree,
			 GTH_TYPE_FOLDER_TREE,
			 G_ADD_PRIVATE (GthVfsTree))


/* -- load_data-- */


typedef enum {
	LOAD_ACTION_LOAD,
	LOAD_ACTION_LIST_CHILDREN
} LoadAction;


typedef struct {
	GthVfsTree    *vfs_tree;
	LoadAction     action;
	GthFileData   *requested_folder;
	GFile         *entry_point;
	GthFileSource *file_source;
	GCancellable  *cancellable;
	GList         *list;
	GList         *current;
	guint          destroy_id;
	GFile         *last_loaded;
	gboolean       scroll_to_cell;
} LoadData;


static void
load_data_vfs_tree_destroy_cb (GtkWidget *widget,
			       gpointer   user_data)
{
	LoadData *load_data = user_data;

	gth_file_source_cancel (load_data->file_source);
	g_cancellable_cancel (load_data->cancellable);

	g_signal_handler_disconnect (load_data->vfs_tree, load_data->destroy_id);
	load_data->destroy_id = 0;
}


static LoadData *
load_data_new (GthVfsTree *vfs_tree,
	       LoadAction  action,
	       GFile      *location)
{
	LoadData *load_data;
	GFile    *root;
	GFile    *file;

	root = gth_folder_tree_get_root (GTH_FOLDER_TREE (vfs_tree));

	load_data = g_new0 (LoadData, 1);
	load_data->vfs_tree = g_object_ref (vfs_tree);
	load_data->action = action;
	load_data->requested_folder = gth_file_data_new (location, NULL);
	if (vfs_tree->priv->tree_root_is_vfs_root && ! g_file_equal (location, root)) {
		GthFileData *entry_point = gth_main_get_nearest_entry_point (location);
		load_data->entry_point = _g_object_ref (entry_point->file);
		g_object_unref (entry_point);
	}
	else
		load_data->entry_point = _g_object_ref (root);
	load_data->file_source = gth_main_get_file_source (load_data->requested_folder->file);
	load_data->cancellable = g_cancellable_new ();
	load_data->list = NULL;
	load_data->current = NULL;
	load_data->destroy_id =
		g_signal_connect (load_data->vfs_tree,
				  "destroy",
				  G_CALLBACK (load_data_vfs_tree_destroy_cb),
				  load_data);
	load_data->last_loaded = NULL;

	if (load_data->entry_point == NULL)
		return load_data;

	file = g_object_ref (load_data->requested_folder->file);
	load_data->list = g_list_prepend (NULL, g_object_ref (file));
	while (! g_file_equal (load_data->entry_point, file)) {
		GFile *parent;

		parent = g_file_get_parent (file);
		g_object_unref (file);
		file = parent;

		load_data->list = g_list_prepend (load_data->list, g_object_ref (file));
	}
	g_object_unref (file);

	if (vfs_tree->priv->tree_root_is_vfs_root)
		load_data->list = g_list_prepend (load_data->list, g_file_new_for_uri ("gthumb-vfs:///"));

	load_data->current = NULL;

	return load_data;
}


static void
load_data_free (LoadData *data)
{
	if (data->destroy_id != 0)
		g_signal_handler_disconnect (data->vfs_tree, data->destroy_id);
	_g_object_unref (data->last_loaded);
	_g_object_list_unref (data->list);
	g_object_unref (data->cancellable);
	_g_object_unref (data->file_source);
	_g_object_unref (data->entry_point);
	g_object_unref (data->requested_folder);
	_g_object_unref (data->vfs_tree);
	g_free (data);
}


static void load_data_load_next_folder (LoadData *load_data);


static GList *
_get_visible_files (GthVfsTree *self,
		    GList      *list)
{
	GList *visible_list = NULL;
	GList *scan;

	for (scan = list; scan; scan = scan->next) {
		GthFileData *file_data = scan->data;

		if (self->priv->show_hidden || ! g_file_info_get_is_hidden (file_data->info))
			visible_list = g_list_prepend (visible_list, g_object_ref (file_data));
	}

	return g_list_reverse (visible_list);
}


static void
select_last_loaded (GthVfsTree *self,
		    LoadData   *load_data)
{
	GtkTreePath *path;

	path = gth_folder_tree_get_path (GTH_FOLDER_TREE (self), load_data->last_loaded);
	if (path != NULL) {
		gth_folder_tree_select_path (GTH_FOLDER_TREE (self), path);
		if (load_data->scroll_to_cell)
			gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (self),
						      path,
						      NULL,
						      TRUE,
						      0.5,
						      0);
		gtk_tree_path_free (path);
	}
	else {
		GtkTreeSelection *selection;

		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (self));
		gtk_tree_selection_unselect_all (selection);
	}
}


static void
load_data_ready_cb (GthFileSource *file_source,
		    GList         *files,
		    GError        *error,
		    gpointer       user_data)
{
	LoadData    *load_data = user_data;
	GthVfsTree  *self = load_data->vfs_tree;
	GFile       *loaded_folder;
	gboolean     loaded_root;
	gboolean     loaded_requested;
	GtkTreePath *path;
	GList       *visible_files;

	if (error != NULL) {
		select_last_loaded (self, load_data);
		load_data_free (load_data);
		return;
	}

	loaded_folder = (GFile *) load_data->current->data;
	loaded_root = gth_folder_tree_is_root (GTH_FOLDER_TREE (self), loaded_folder);
	loaded_requested = g_file_equal (loaded_folder, load_data->requested_folder->file);
	if (loaded_root)
		self->priv->root_loaded = TRUE;

	path = gth_folder_tree_get_path (GTH_FOLDER_TREE (self), loaded_folder);
	if ((path == NULL) && ! loaded_root) {
		select_last_loaded (self, load_data);
		load_data_free (load_data);
		return;
	}

	visible_files = _get_visible_files (self, files);
	gth_folder_tree_set_children (GTH_FOLDER_TREE (self),
				      loaded_folder,
				      visible_files);

	if (path != NULL)
		gth_folder_tree_expand_row (GTH_FOLDER_TREE (self), path, FALSE);

	_g_object_unref (load_data->last_loaded);
	load_data->last_loaded = _g_object_ref (loaded_folder);

	if (! loaded_requested) {
		load_data_load_next_folder (load_data);
	}
	else {
		select_last_loaded (self, load_data);

		if (load_data->action == LOAD_ACTION_LOAD) {
			_g_object_unref (self->priv->folder);
			self->priv->folder = g_object_ref (loaded_folder);

			g_signal_emit (self, gth_vfs_tree_signals[CHANGED], 0);
		}

		load_data_free (load_data);
	}

	_g_object_list_unref (visible_files);
	if (path != NULL)
		gtk_tree_path_free (path);
}


static gboolean
folder_is_vfs_root (GFile *folder)
{
	char     *uri;
	gboolean  result;

	uri = g_file_get_uri (folder);
	result = g_strcmp0 (uri, "gthumb-vfs:///") == 0;

	g_free (uri);

	return result;
}


static void
load_data_load_next_folder (LoadData *load_data)
{
	GthFolderTree *folder_tree = GTH_FOLDER_TREE (load_data->vfs_tree);
	GFile         *folder_to_load = NULL;

	do {
		GtkTreePath *path;
		gboolean     is_root;
		gboolean     is_loaded;

		if (load_data->current == NULL)
			load_data->current = load_data->list;
		else
			load_data->current = load_data->current->next;
		folder_to_load = (GFile *) load_data->current->data;

		if (g_file_equal (folder_to_load, load_data->requested_folder->file))
			break;

		is_root = gth_folder_tree_is_root (GTH_FOLDER_TREE (folder_tree), folder_to_load);
		path = gth_folder_tree_get_path (folder_tree, folder_to_load);
		if ((path == NULL) && ! is_root)
			break;

		is_loaded = is_root ? load_data->vfs_tree->priv->root_loaded : gth_folder_tree_is_loaded (folder_tree, path);
		if (! is_loaded) {
			gtk_tree_path_free (path);
			break;
		}

		if (path != NULL) {
			if (gth_folder_tree_has_no_child (folder_tree, path)) {
				folder_to_load = NULL;
				gtk_tree_path_free (path);
				break;
			}

			gth_folder_tree_expand_row (folder_tree, path, FALSE);

			gtk_tree_path_free (path);
		}
	}
	while (TRUE);

	if (folder_to_load == NULL) {
		load_data_free (load_data);
		return;
	}

	gth_folder_tree_loading_children (folder_tree, folder_to_load);

	if (folder_is_vfs_root (folder_to_load)) {
		GList *entry_points;

		entry_points = gth_main_get_all_entry_points ();
		load_data_ready_cb (NULL, entry_points, NULL, load_data);

		_g_object_list_unref (entry_points);
	}
	else
		gth_file_source_list (load_data->file_source,
				      folder_to_load,
				      GFILE_STANDARD_ATTRIBUTES_WITH_FAST_CONTENT_TYPE,
				      load_data_ready_cb,
				      load_data);
}


static void
_gth_vfs_tree_load_folder (GthVfsTree *self,
			   LoadAction  action,
			   GFile      *folder,
			   gboolean    scroll_to_cell);


static void
mount_volume_ready_cb (GObject      *source_object,
		       GAsyncResult *result,
		       gpointer      user_data)
{
	LoadData *load_data = user_data;
	GError   *error = NULL;

	if (! g_file_mount_enclosing_volume_finish (G_FILE (source_object), result, &error)) {
		GthVfsTree *self = load_data->vfs_tree;
		char       *title;

		title = _g_format_str_for_file (_("Could not load the position “%s”"),
						load_data->requested_folder->file);
		_gtk_error_dialog_from_gerror_show (_gtk_widget_get_toplevel_if_window (GTK_WIDGET (self)),
						    title,
						    error);

		g_free (title);
		load_data_free (load_data);
		return;
	}

	/* update the entry points list */

	gth_monitor_entry_points_changed (gth_main_get_default_monitor());

	/* try to load again */

	_gth_vfs_tree_load_folder (load_data->vfs_tree,
				   load_data->action,
				   load_data->requested_folder->file,
				   load_data->scroll_to_cell);

	load_data_free (load_data);
}


static void
_gth_vfs_tree_load_folder (GthVfsTree *self,
			   LoadAction  action,
			   GFile      *folder,
			   gboolean    scroll_to_cell)
{
	LoadData *load_data;

	load_data = load_data_new (self, action, folder);
	load_data->scroll_to_cell = scroll_to_cell;

	if (load_data->entry_point == NULL) {
		if (self->priv->folder != NULL) {
			GMountOperation *mount_op;

			/* try to mount the enclosing volume */

			mount_op = gtk_mount_operation_new (_gtk_widget_get_toplevel_if_window (GTK_WIDGET (self)));
			g_file_mount_enclosing_volume (folder,
						       0,
						       mount_op,
						       load_data->cancellable,
						       mount_volume_ready_cb,
						       load_data);

			g_object_unref (mount_op);
		}
		else {
			GFile *root = gth_folder_tree_get_root (GTH_FOLDER_TREE (self));
			if (! _g_file_equal (load_data->requested_folder->file, root))
				_gth_vfs_tree_load_folder (self, LOAD_ACTION_LIST_CHILDREN, root, scroll_to_cell);
		}
		return;
	}

	load_data_load_next_folder (load_data);
}


static void
vfs_tree_list_children_cb (GthFolderTree *folder_tree,
			   GFile         *file,
			   gpointer       user_data)
{
	GthVfsTree  *self = user_data;
	GtkTreePath *path;

	path = gth_folder_tree_get_path (GTH_FOLDER_TREE (self), file);
	if (path == NULL)
		return;

	_gth_vfs_tree_load_folder (self, LOAD_ACTION_LIST_CHILDREN, file, FALSE);

	gtk_tree_path_free (path);
}


static void
vfs_tree_open_cb (GthFolderTree *folder_tree,
		  GFile         *file,
		  gpointer       user_data)
{
	GthVfsTree  *self = user_data;
	GtkTreePath *path;

	path = gth_folder_tree_get_path (GTH_FOLDER_TREE (self), file);
	if (path == NULL)
		return;

	_gth_vfs_tree_load_folder (self, LOAD_ACTION_LOAD, file, FALSE);

	gtk_tree_path_free (path);
}


/* -- monitor_event_data -- */


typedef struct {
	int              ref;
	GthVfsTree      *vfs_tree;
	GFile           *parent;
	GthMonitorEvent  event;
	GthFileSource   *file_source;
	guint            destroy_id;
} MonitorEventData;


static void
monitor_data_vfs_tree_destroy_cb (GtkWidget *widget,
				  gpointer   user_data)
{
	MonitorEventData *monitor_data = user_data;

	gth_file_source_cancel (monitor_data->file_source);
	g_signal_handler_disconnect (monitor_data->vfs_tree, monitor_data->destroy_id);
	monitor_data->destroy_id = 0;
}


static MonitorEventData *
monitor_event_data_new (GthVfsTree *vfs_tree)
{
	MonitorEventData *monitor_data;

	monitor_data = g_new0 (MonitorEventData, 1);
	monitor_data->ref = 1;
	monitor_data->vfs_tree = _g_object_ref (vfs_tree);
	monitor_data->parent = NULL;
	monitor_data->file_source = NULL;
	monitor_data->destroy_id =
		g_signal_connect (monitor_data->vfs_tree,
				  "destroy",
				  G_CALLBACK (monitor_data_vfs_tree_destroy_cb),
				  monitor_data);

	return monitor_data;
}


G_GNUC_UNUSED
static MonitorEventData *
monitor_event_data_ref (MonitorEventData *monitor_data)
{
	monitor_data->ref++;
	return monitor_data;
}


static void
monitor_event_data_unref (MonitorEventData *monitor_data)
{
	monitor_data->ref--;

	if (monitor_data->ref > 0)
		return;

	if (monitor_data->destroy_id != 0)
		g_signal_handler_disconnect (monitor_data->vfs_tree, monitor_data->destroy_id);
	_g_object_unref (monitor_data->vfs_tree);
	_g_object_unref (monitor_data->parent);
	_g_object_unref (monitor_data->file_source);
	g_free (monitor_data);
}


static void
file_attributes_ready_cb (GthFileSource *file_source,
			  GList         *files,
			  GError        *error,
			  gpointer       user_data)
{
	MonitorEventData *monitor_data = user_data;
	GthVfsTree       *self = monitor_data->vfs_tree;

	if (error != NULL) {
		g_warning ("%s", error->message);
		g_clear_error (&error);
		monitor_event_data_unref (monitor_data);
		return;
	}

	if (monitor_data->event == GTH_MONITOR_EVENT_CREATED)
		gth_folder_tree_add_children (GTH_FOLDER_TREE (self), monitor_data->parent, files);
	else if (monitor_data->event == GTH_MONITOR_EVENT_CHANGED)
		gth_folder_tree_update_children (GTH_FOLDER_TREE (self), monitor_data->parent, files);

	monitor_event_data_unref (monitor_data);
}


static void
monitor_folder_changed_cb (GthMonitor      *monitor,
			   GFile           *parent,
			   GList           *list,
			   int              position,
			   GthMonitorEvent  event,
			   GthVfsTree      *self)
{
	GtkTreePath *path;

	path = gth_folder_tree_get_path (GTH_FOLDER_TREE (self), parent);
	if (gth_folder_tree_is_root (GTH_FOLDER_TREE (self), parent)
	    || ((path != NULL) && gtk_tree_view_row_expanded (GTK_TREE_VIEW (self), path)))
	{
		MonitorEventData *monitor_data;

		switch (event) {
		case GTH_MONITOR_EVENT_CREATED:
		case GTH_MONITOR_EVENT_CHANGED:
			monitor_data = monitor_event_data_new (self);
			monitor_data->parent = g_file_dup (parent);
			monitor_data->event = event;
			monitor_data->file_source = gth_main_get_file_source (monitor_data->parent);
			gth_file_source_read_attributes (monitor_data->file_source,
						 	 list,
						 	 GFILE_STANDARD_ATTRIBUTES_WITH_FAST_CONTENT_TYPE,
						 	 file_attributes_ready_cb,
						 	 monitor_data);
			break;

		case GTH_MONITOR_EVENT_DELETED:
		case GTH_MONITOR_EVENT_REMOVED:
			gth_folder_tree_delete_children (GTH_FOLDER_TREE (self), parent, list);
			break;
		}
	}

	gtk_tree_path_free (path);
}


static void
monitor_file_renamed_cb (GthMonitor *monitor,
			 GFile      *file,
			 GFile      *new_file,
			 GthVfsTree *self)
{
	GthFileSource *file_source;

	file_source = gth_main_get_file_source (new_file);
	if (file_source != NULL) {
		GFileInfo *info;

		info = gth_file_source_get_file_info (file_source, new_file, GFILE_BASIC_ATTRIBUTES);
		if (info != NULL) {
			GthFileData *file_data;

			file_data = gth_file_data_new (new_file, info);
			gth_folder_tree_update_child (GTH_FOLDER_TREE (self), file, file_data);

			g_object_unref (file_data);
		}

		_g_object_unref (info);
	}

	_g_object_unref (file_source);
}


static void
vfs_tree_realize_cb (GtkWidget *widget,
		     gpointer   user_data)
{
	GthVfsTree *self = GTH_VFS_TREE (widget);
	GFile      *root;

	root = gth_folder_tree_get_root (GTH_FOLDER_TREE (self));
	self->priv->tree_root_is_vfs_root = folder_is_vfs_root (root);

	if ((self->priv->folder_uri != NULL) && (self->priv->folder_uri[0] != 0)) {
		GFile *folder;

		folder = g_file_new_for_uri (self->priv->folder_uri);
		_gth_vfs_tree_load_folder (self, LOAD_ACTION_LOAD, folder, TRUE);

		g_object_unref (folder);
	}
	else
		_gth_vfs_tree_load_folder (self, LOAD_ACTION_LIST_CHILDREN, root, TRUE);
}


static void
gth_vfs_tree_init (GthVfsTree *self)
{
	self->priv = gth_vfs_tree_get_instance_private (self);
	self->priv->folder = NULL;
	self->priv->folder_uri = NULL;
	self->priv->root_loaded = FALSE;

	g_signal_connect (self,
			  "list_children",
			  G_CALLBACK (vfs_tree_list_children_cb),
			  self);
	g_signal_connect (self,
			  "open",
			  G_CALLBACK (vfs_tree_open_cb),
			  self);
	g_signal_connect (self,
			  "realize",
			  G_CALLBACK (vfs_tree_realize_cb),
			  self);

	self->priv->monitor_folder_changed_id =
		g_signal_connect (gth_main_get_default_monitor (),
				  "folder-changed",
				  G_CALLBACK (monitor_folder_changed_cb),
				  self);
	self->priv->monitor_file_renamed_id =
		g_signal_connect (gth_main_get_default_monitor (),
				  "file-renamed",
				  G_CALLBACK (monitor_file_renamed_cb),
				  self);
}


static void
gth_vfs_tree_set_property (GObject      *object,
			   guint         property_id,
			   const GValue *value,
			   GParamSpec   *pspec)
{
	GthVfsTree *self;

	self = GTH_VFS_TREE (object);

	switch (property_id) {
	case PROP_SHOW_HIDDEN:
		self->priv->show_hidden = g_value_get_boolean (value);
		break;

	case PROP_FOLDER_URI:
		g_free (self->priv->folder_uri);
		self->priv->folder_uri = g_value_dup_string (value);
		break;

	default:
		break;
	}
}


static void
gth_vfs_tree_get_property (GObject    *object,
			   guint       property_id,
			   GValue     *value,
			   GParamSpec *pspec)
{
	GthVfsTree *self;

	self = GTH_VFS_TREE (object);

	switch (property_id) {
	case PROP_SHOW_HIDDEN:
		g_value_set_boolean (value, self->priv->show_hidden);
		break;

	case PROP_FOLDER_URI:
		g_value_set_string (value, self->priv->folder_uri);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}


static void
gth_vfs_tree_finalize (GObject *object)
{
	GthVfsTree *self = GTH_VFS_TREE (object);

	g_signal_handler_disconnect (gth_main_get_default_monitor (), self->priv->monitor_folder_changed_id);
	g_signal_handler_disconnect (gth_main_get_default_monitor (), self->priv->monitor_file_renamed_id);
	_g_object_unref (self->priv->folder);
	g_free (self->priv->folder_uri);

	G_OBJECT_CLASS (gth_vfs_tree_parent_class)->finalize (object);
}


static void
gth_vfs_tree_class_init (GthVfsTreeClass *klass)
{
	GObjectClass *gobject_class;

	gobject_class = G_OBJECT_CLASS (klass);
	gobject_class->set_property = gth_vfs_tree_set_property;
	gobject_class->get_property = gth_vfs_tree_get_property;
	gobject_class->finalize = gth_vfs_tree_finalize;

	/* signals */

	gth_vfs_tree_signals[CHANGED] =
		g_signal_new ("changed",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthVfsTreeClass, changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);

	/* properties */

	g_object_class_install_property (gobject_class,
					 PROP_SHOW_HIDDEN,
					 g_param_spec_boolean ("show-hidden",
							       "Show Hidden",
							       "Show hidden folders",
							       FALSE,
							       G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class,
					 PROP_FOLDER_URI,
					 g_param_spec_string ("folder-uri",
							      "Folder",
							      "Folder to load",
							      NULL,
							      G_PARAM_READWRITE));

}


GtkWidget *
gth_vfs_tree_new (const char *root,
		  const char *folder)
{
	return g_object_new (GTH_TYPE_VFS_TREE,
			     "root-uri", root,
			     "folder-uri", folder,
			     NULL);
}


void
gth_vfs_tree_set_folder (GthVfsTree *self,
			 GFile      *folder)
{
	g_return_if_fail (GTH_IS_VFS_TREE (self));
	g_return_if_fail (folder != NULL);

	if (! gtk_widget_get_realized (GTK_WIDGET (self))) {
		g_free (self->priv->folder_uri);
		self->priv->folder_uri = g_file_get_uri (folder);
	}
	else
		_gth_vfs_tree_load_folder (self, LOAD_ACTION_LOAD, folder, TRUE);
}


GFile *
gth_vfs_tree_get_folder (GthVfsTree *self)
{
	g_return_val_if_fail (GTH_IS_VFS_TREE (self), NULL);
	return self->priv->folder;
}


void
gth_vfs_tree_set_show_hidden (GthVfsTree *self,
			      gboolean    show_hidden)
{
	g_return_if_fail (GTH_IS_VFS_TREE (self));

	g_object_set (self, "show-hidden", show_hidden, FALSE, NULL);
	gth_vfs_tree_set_folder (self, self->priv->folder);
}


gboolean
gth_vfs_tree_get_show_hidden (GthVfsTree *self)
{
	g_return_val_if_fail (GTH_IS_VFS_TREE (self), FALSE);

	return self->priv->show_hidden;
}
