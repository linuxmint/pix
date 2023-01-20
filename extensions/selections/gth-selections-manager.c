/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2012 Free Software Foundation, Inc.
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
#include <string.h>
#include <glib/gi18n.h>
#include <glib.h>
#include <gtk/gtk.h>
#include "gth-selections-manager.h"


struct _GthSelectionsManagerPrivate {
	GList      *files[GTH_SELECTIONS_MANAGER_N_SELECTIONS];
	GHashTable *files_hash[GTH_SELECTIONS_MANAGER_N_SELECTIONS];
	char       *order[GTH_SELECTIONS_MANAGER_N_SELECTIONS];
	gboolean    order_inverse[GTH_SELECTIONS_MANAGER_N_SELECTIONS];
	GMutex      mutex;
};


G_DEFINE_TYPE_WITH_CODE (GthSelectionsManager,
			 gth_selections_manager,
			 G_TYPE_OBJECT,
			 G_ADD_PRIVATE (GthSelectionsManager))


static GthSelectionsManager *the_manager = NULL;


static GObject *
gth_selections_manager_constructor (GType                  type,
				    guint                  n_construct_params,
				    GObjectConstructParam *construct_params)
{
	static GObject *object = NULL;

	if (the_manager == NULL) {
		object = G_OBJECT_CLASS (gth_selections_manager_parent_class)->constructor (type, n_construct_params, construct_params);
		the_manager = GTH_SELECTIONS_MANAGER (object);
	}
	else
		object =  G_OBJECT (the_manager);

	return object;
}


static void
gth_selections_manager_finalize (GObject *object)
{
	GthSelectionsManager *self;
	int                   i;

	self = GTH_SELECTIONS_MANAGER (object);

	for (i = 0; i < GTH_SELECTIONS_MANAGER_N_SELECTIONS; i++) {
		_g_object_list_unref (self->priv->files[i]);
		g_hash_table_unref (self->priv->files_hash[i]);
		g_free (self->priv->order[i]);
	}
	g_mutex_clear (&self->priv->mutex);

	G_OBJECT_CLASS (gth_selections_manager_parent_class)->finalize (object);
}


static void
gth_selections_manager_class_init (GthSelectionsManagerClass *klass)
{
	GObjectClass *object_class;

	object_class = (GObjectClass*) klass;
	object_class->constructor = gth_selections_manager_constructor;
	object_class->finalize = gth_selections_manager_finalize;
}


static void
gth_selections_manager_init (GthSelectionsManager *self)
{
	int i;

	self->priv = gth_selections_manager_get_instance_private (self);
	g_mutex_init (&self->priv->mutex);
	for (i = 0; i < GTH_SELECTIONS_MANAGER_N_SELECTIONS; i++) {
		self->priv->files[i] = NULL;
		self->priv->files_hash[i] = g_hash_table_new (g_file_hash, (GEqualFunc) g_file_equal);
		self->priv->order[i] = NULL;
		self->priv->order_inverse[i] = FALSE;
	}
}


static GthSelectionsManager *
gth_selections_manager_get_default (void)
{
	return (GthSelectionsManager*) g_object_new (GTH_TYPE_SELECTIONS_MANAGER, NULL);
}


/* -- gth_selections_manager_for_each_child -- */


typedef struct {
	GthSelectionsManager *selections_manager;
	GList                *files;
	GList                *current_file;
	char                 *attributes;
	GCancellable         *cancellable;
	ForEachChildCallback  for_each_file_func;
	ReadyCallback         ready_callback;
	gpointer              user_data;
} ForEachChildData;


static void
fec_data_free (ForEachChildData *data)
{
	_g_object_list_unref (data->files);
	g_free (data->attributes);
	_g_object_unref (data->cancellable);
	g_free (data);
}


static void
selections_manager_fec_done (ForEachChildData *data,
			     GError           *error)
{
	if (data->ready_callback != NULL)
		data->ready_callback (NULL, error, data->user_data);
	fec_data_free (data);
}


static void
fec__file_info_ready_cb (GObject      *source_object,
			 GAsyncResult *result,
			 gpointer      user_data)
{
	ForEachChildData *data = user_data;
	GFile            *file;
	GFileInfo        *info;

	file = (GFile*) source_object;
	info = g_file_query_info_finish (file, result, NULL);
	if (info != NULL) {
		if (data->for_each_file_func != NULL)
			data->for_each_file_func (file, info, data->user_data);
		g_object_unref (info);
	}

	data->current_file = data->current_file->next;
	if (data->current_file == NULL) {
		selections_manager_fec_done (data, NULL);
		return;
	}

	g_file_query_info_async ((GFile *) data->current_file->data,
				 data->attributes,
				 0,
				 G_PRIORITY_DEFAULT,
				 data->cancellable,
				 fec__file_info_ready_cb,
				 data);

}


static void
selections_manager_fec_done_cb (GObject  *object,
				GError   *error,
				gpointer  user_data)
{
	selections_manager_fec_done (user_data, NULL);
}


void
gth_selections_manager_update_file_info (GFile     *file,
					 GFileInfo *info)
{
	int    n_selection;
	GIcon *icon;
	char  *name;

	n_selection = _g_file_get_n_selection (file);

	g_file_info_set_file_type (info, G_FILE_TYPE_DIRECTORY);
	g_file_info_set_content_type (info, "gthumb/selection");

	g_file_info_set_sort_order (info, n_selection);
	g_file_info_set_attribute_boolean (info, G_FILE_ATTRIBUTE_ACCESS_CAN_READ, TRUE);
	if (n_selection > 0)
		g_file_info_set_attribute_boolean (info, G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE, TRUE);
	g_file_info_set_attribute_boolean (info, G_FILE_ATTRIBUTE_ACCESS_CAN_DELETE, FALSE);
	g_file_info_set_attribute_boolean (info, G_FILE_ATTRIBUTE_ACCESS_CAN_RENAME, FALSE);
	g_file_info_set_attribute_int32 (info, "gthumb::n-selection", n_selection);

	/* icon */

	icon = g_themed_icon_new (gth_selection_get_symbolic_icon_name (n_selection));
	g_file_info_set_symbolic_icon (info, icon);
	g_object_unref (icon);

	/* display name */

	if (n_selection > 0) {
		g_file_info_set_attribute_boolean (info, "gthumb::no-child", TRUE);
		name = g_strdup_printf (_("Selection %d"), n_selection);
	}
	else if (n_selection == 0)
		name = g_strdup (_("Selections"));
	else
		name = g_strdup ("???");
	g_file_info_set_display_name (info, name);
	g_free (name);

	/* name */

	if (n_selection > 0)
		name = g_strdup_printf ("%d", n_selection);
	else
		name = g_strdup ("");
	g_file_info_set_name (info, name);

	g_free (name);

	/* sort order */

	if (n_selection > 0) {
		GthSelectionsManager *self;

		self = gth_selections_manager_get_default ();

		if (self->priv->order[n_selection] != NULL) {
			g_file_info_set_attribute_string (info, "sort::type", self->priv->order[n_selection - 1]);
			g_file_info_set_attribute_boolean (info, "sort::inverse", self->priv->order_inverse[n_selection - 1]);
		}
		else {
			g_file_info_set_attribute_string (info, "sort::type", "general::unsorted");
			g_file_info_set_attribute_boolean (info, "sort::inverse", FALSE);
		}
	}
}


static void
_gth_selections_manager_for_each_selection (gpointer user_data)
{
	ForEachChildData *data = user_data;
	int               i;

	for (i = 0; i < GTH_SELECTIONS_MANAGER_N_SELECTIONS; i++) {
		char      *uri;
		GFile     *file;
		GFileInfo *info;

		uri = g_strdup_printf ("selection:///%d", i + 1);
		file = g_file_new_for_uri (uri);
		info = g_file_info_new ();
		gth_selections_manager_update_file_info (file, info);
		data->for_each_file_func (file, info, data->user_data);

		g_object_unref (info);
		g_object_unref (file);
		g_free (uri);
	}

	object_ready_with_error (data->selections_manager,
				 data->ready_callback,
				 data->user_data,
				 NULL);
	fec_data_free (data);
}


G_GNUC_UNUSED
static void
_gth_selections_manager_load_from_node (GthSelectionsManager *self,
					DomElement           *node)
{
	DomElement *child;
	int         n_selection;
	GList      *file_list;

	n_selection = atoi (dom_element_get_attribute (node, "n"));
	if ((n_selection < 0) || (n_selection > GTH_SELECTIONS_MANAGER_N_SELECTIONS))
		return;

	g_hash_table_remove_all (self->priv->files_hash[n_selection - 1]);
	_g_object_list_unref (self->priv->files[n_selection - 1]);
	self->priv->files[n_selection - 1] = NULL;

	file_list = NULL;
	for (child = node->first_child; child; child = child->next_sibling) {
		if (g_strcmp0 (child->tag_name, "file") == 0) {
			const char *uri;

			uri = dom_element_get_attribute (child, "uri");
			if (uri != NULL) {
				GFile *file = g_file_new_for_uri (uri);
				file_list = g_list_prepend (file_list, file);
				g_hash_table_insert (self->priv->files_hash[n_selection - 1], file, GINT_TO_POINTER (1));
			}
		}
	}
	self->priv->files[n_selection - 1] = g_list_reverse (file_list);
}


void
gth_selections_manager_for_each_child (GFile                *folder,
				       const char           *attributes,
				       GCancellable         *cancellable,
				       ForEachChildCallback  for_each_file_func,
				       ReadyCallback         ready_callback,
				       gpointer              user_data)
{
	GthSelectionsManager *self;
	int                   n_selection;
	ForEachChildData     *data;

	self = gth_selections_manager_get_default ();
	n_selection = _g_file_get_n_selection (folder);

	g_mutex_lock (&self->priv->mutex);
	data = g_new0 (ForEachChildData, 1);
	data->selections_manager = self;
	if (n_selection > 0)
		data->files = _g_object_list_ref (self->priv->files[n_selection - 1]);
	data->current_file = data->files;
	data->attributes = g_strdup (attributes);
	data->cancellable = _g_object_ref(cancellable);
	data->for_each_file_func = for_each_file_func;
	data->ready_callback = ready_callback;
	data->user_data = user_data;
	g_mutex_unlock (&self->priv->mutex);

	if (n_selection == 0) {
		call_when_idle (_gth_selections_manager_for_each_selection, data);
	}
	else if (data->current_file != NULL)
		g_file_query_info_async ((GFile *) data->current_file->data,
					 data->attributes,
					 0,
					 G_PRIORITY_DEFAULT,
					 data->cancellable,
					 fec__file_info_ready_cb,
					 data);
	else
		object_ready_with_error (NULL, selections_manager_fec_done_cb, data, NULL);
}


gboolean
gth_selections_manager_add_files (GFile *folder,
				  GList *file_list, /* GFile list */
				  int    destination_position)
{
	GthSelectionsManager *self;
	int                   n_selection;
	GList                *new_list;
	GList                *scan;
	GList                *link;

	if (! g_file_has_uri_scheme (folder, "selection"))
		return FALSE;

	self = gth_selections_manager_get_default ();
	n_selection = _g_file_get_n_selection (folder);
	if (n_selection <= 0)
		return FALSE;

	g_mutex_lock (&self->priv->mutex);

	new_list = _g_file_list_dup (file_list);

	for (scan = new_list; scan; scan = scan->next)
		g_hash_table_insert (self->priv->files_hash[n_selection - 1], scan->data, GINT_TO_POINTER (1));

	link = g_list_nth (self->priv->files[n_selection - 1], destination_position);
	if (link != NULL) {
		GList *last_new;

		/* insert 'new_list' before 'link' */

		if (link->prev != NULL)
			link->prev->next = new_list;
		new_list->prev = link->prev;

		last_new = g_list_last (new_list);
		last_new->next = link;
		link->prev = last_new;
	}
	else
		self->priv->files[n_selection - 1] = g_list_concat (self->priv->files[n_selection - 1], new_list);

	g_mutex_unlock (&self->priv->mutex);

	gth_monitor_emblems_changed (gth_main_get_default_monitor (), file_list);
	gth_monitor_folder_changed (gth_main_get_default_monitor (),
				    folder,
				    file_list,
				    GTH_MONITOR_EVENT_CREATED);

	return TRUE;
}


void
gth_selections_manager_remove_files (GFile    *folder,
				     GList    *file_list,
				     gboolean  notify)
{
	GthSelectionsManager *self;
	int                   n_selection;
	GHashTable           *files_to_remove;
	GList                *scan;
	GList                *new_list;

	self = gth_selections_manager_get_default ();
	n_selection = _g_file_get_n_selection (folder);
	if (n_selection <= 0)
		return;

	g_mutex_lock (&self->priv->mutex);

	files_to_remove = g_hash_table_new (g_file_hash, (GEqualFunc) g_file_equal);
	for (scan = file_list; scan; scan = scan->next) {
		g_hash_table_insert (files_to_remove, scan->data, GINT_TO_POINTER (1));
		g_hash_table_remove (self->priv->files_hash[n_selection - 1], scan->data);
	}

	new_list = NULL;
	for (scan = self->priv->files[n_selection - 1]; scan; scan = scan->next) {
		GFile *file = scan->data;

		if (g_hash_table_lookup (files_to_remove, file))
			continue;

		new_list = g_list_prepend (new_list, g_object_ref (file));
	}
	new_list = g_list_reverse (new_list);

	g_hash_table_unref (files_to_remove);

	_g_object_list_unref (self->priv->files[n_selection - 1]);
	self->priv->files[n_selection - 1] = new_list;

	g_mutex_unlock (&self->priv->mutex);

	if (notify)
		gth_monitor_folder_changed (gth_main_get_default_monitor (),
					    folder,
					    file_list,
					    GTH_MONITOR_EVENT_REMOVED);
	gth_monitor_emblems_changed (gth_main_get_default_monitor (), file_list);
}


static void
_gth_selections_manager_files_changed_for_selection (GthSelectionsManager *self,
						     int                   n_selection)
{
	GList *scan;

	g_hash_table_remove_all (self->priv->files_hash[n_selection - 1]);
	for (scan = self->priv->files[n_selection - 1]; scan; scan = scan->next) {
		GFile *file = scan->data;
		g_hash_table_insert (self->priv->files_hash[n_selection - 1], file, GINT_TO_POINTER (1));
	}
}


void
gth_selections_manager_reorder (GFile *folder,
				GList *visible_files, /* GFile list */
				GList *files_to_move, /* GFile list */
				int    dest_pos)
{
	GthSelectionsManager *self;
	int                   n_selection;
	int                  *new_order;
	GList                *new_file_list;

	n_selection = _g_file_get_n_selection (folder);
	if (n_selection <= 0)
		return;

	self = gth_selections_manager_get_default ();

	/* reorder the file list */

	g_mutex_lock (&self->priv->mutex);
	_g_list_reorder (self->priv->files[n_selection - 1],
			 visible_files,
			 files_to_move,
			 dest_pos,
			 &new_order,
			 &new_file_list);
	_g_object_list_unref (self->priv->files[n_selection - 1]);
	self->priv->files[n_selection - 1] = new_file_list;
	_gth_selections_manager_files_changed_for_selection (self, n_selection);
	g_mutex_unlock (&self->priv->mutex);

	gth_selections_manager_set_sort_type (folder, "general::unsorted", FALSE);

	gth_monitor_order_changed (gth_main_get_default_monitor (),
				   folder,
				   new_order);

	g_free (new_order);
}


void
gth_selections_manager_set_sort_type (GFile      *folder,
				      const char *sort_type,
				      gboolean    sort_inverse)
{
	GthSelectionsManager *self;
	int                   n_selection;

	n_selection = _g_file_get_n_selection (folder);
	if (n_selection <= 0)
		return;

	self = gth_selections_manager_get_default ();
	g_mutex_lock (&self->priv->mutex);

	g_free (self->priv->order[n_selection - 1]);
	self->priv->order[n_selection - 1] = g_strdup (sort_type);
	self->priv->order_inverse[n_selection - 1] = sort_inverse;

	g_mutex_unlock (&self->priv->mutex);
}


gboolean
gth_selections_manager_file_exists (int    n_selection,
				    GFile *file)
{
	GthSelectionsManager *self;
	gboolean             result;

	if ((n_selection <= 0) || (n_selection > GTH_SELECTIONS_MANAGER_N_SELECTIONS))
		return FALSE;

	self = gth_selections_manager_get_default ();
	g_mutex_lock (&self->priv->mutex);

	result = (g_hash_table_lookup (self->priv->files_hash[n_selection - 1], file) != NULL);

	g_mutex_unlock (&self->priv->mutex);

	return result;
}


gboolean
gth_selections_manager_get_is_empty (int n_selection)
{
	GthSelectionsManager *self;
	guint                 size;

	if ((n_selection <= 0) || (n_selection > GTH_SELECTIONS_MANAGER_N_SELECTIONS))
		return TRUE;

	self = gth_selections_manager_get_default ();
	g_mutex_lock (&self->priv->mutex);

	size = g_hash_table_size (self->priv->files_hash[n_selection - 1]);

	g_mutex_unlock (&self->priv->mutex);

	return size == 0;
}


G_GNUC_UNUSED
static DomElement *
_gth_selections_manager_create_selection_node (GthSelectionsManager *self,
					       int                   n_selection,
					       DomDocument          *doc)
{
	char       *n_selection_txt;
	DomElement *selection_node;
	GList      *scan;

	n_selection_txt = g_strdup_printf ("%d", n_selection);
	selection_node = dom_document_create_element (doc, "selection", "n", n_selection_txt, NULL);
	for (scan = self->priv->files[n_selection - 1]; scan; scan = scan->next) {
		GFile *file = scan->data;
		char  *uri;

		uri = g_file_get_uri (file);
		dom_element_append_child (selection_node, dom_document_create_element (doc, "file", "uri", uri, NULL));

		g_free (uri);
	}

	g_free (n_selection_txt);

	return selection_node;
}


int
_g_file_get_n_selection (GFile *file)
{
	char *uri;
	int   n = -1;

	uri = g_file_get_uri (file);
	if (! g_str_has_prefix (uri, "selection:///"))
		n = -1;
	else if (strcmp (uri, "selection:///") == 0)
		n = 0;
	else
		n = atoi (uri + strlen ("selection:///"));

	g_free (uri);

	if (n > GTH_SELECTIONS_MANAGER_N_SELECTIONS)
		n = -1;

	return n;
}


static const char * selection_icons[] = {
	"emblem-flag-gray",
	"emblem-flag-green",
	"emblem-flag-red",
	"emblem-flag-blue"
};


const char *
gth_selection_get_icon_name (int n_selection)
{
	g_return_val_if_fail (n_selection >= 0 && n_selection <= GTH_SELECTIONS_MANAGER_N_SELECTIONS, NULL);
	return selection_icons[n_selection];
}


static const char * selection_symbolic_icons[] = {
	"emblem-flag-symbolic",
	"emblem-flag-symbolic",
	"emblem-flag-symbolic",
	"emblem-flag-symbolic"
};


const char *
gth_selection_get_symbolic_icon_name (int n_selection)
{
	g_return_val_if_fail (n_selection >= 0 && n_selection <= GTH_SELECTIONS_MANAGER_N_SELECTIONS, NULL);
	return selection_symbolic_icons[n_selection];
}
