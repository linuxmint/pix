/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2008 Free Software Foundation, Inc.
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
#include <gtk/gtk.h>
#include "gth-file-data.h"
#include "glib-utils.h"
#include "gtk-utils.h"
#include "gth-file-source.h"
#include "gth-folder-tree.h"
#include "gth-icon-cache.h"
#include "gth-main.h"
#include "gth-marshal.h"


#define EMPTY_URI   "..."
#define LOADING_URI "."
#define PARENT_URI  ".."
#define UPDATE_MONITORED_LOCATIONS_DELAY 500


typedef enum {
	ENTRY_TYPE_FILE,
	ENTRY_TYPE_PARENT,
	ENTRY_TYPE_LOADING,
	ENTRY_TYPE_EMPTY
} EntryType;


enum {
	COLUMN_STYLE,
	COLUMN_WEIGHT,
	COLUMN_ICON,
	COLUMN_TYPE,
	COLUMN_FILE_DATA,
	COLUMN_SORT_KEY,
	COLUMN_SORT_ORDER,
	COLUMN_NAME,
	COLUMN_NO_CHILD,
	COLUMN_LOADED,
	NUM_COLUMNS
};

enum {
	FOLDER_POPUP,
	LIST_CHILDREN,
	LOAD,
	OPEN,
	OPEN_PARENT,
	RENAME,
	LAST_SIGNAL
};


typedef struct {
	GHashTable *locations;
	GList      *sources;
	guint       update_id;
} MonitorData;


struct _GthFolderTreePrivate
{
	GFile            *root;
	GHashTable       *entry_points;		/* An entry point is a root child */
	gboolean          recalc_entry_points;
	GtkTreeStore     *tree_store;
	GthIconCache     *icon_cache;
	GtkCellRenderer  *text_renderer;
	GtkTreePath      *hover_path;

	/* drag-and-drop */

	gboolean         drag_source_enabled;
	GdkModifierType  drag_start_button_mask;
	GtkTargetList   *drag_target_list;
	GdkDragAction    drag_actions;

	gboolean         dragging : 1;        /* Whether the user is dragging items. */
	gboolean         drag_started : 1;    /* Whether the drag has started. */
	int              drag_start_x;        /* The position where the drag started. */
	int              drag_start_y;

	/* monitored locations  */

	MonitorData      monitor;
};


static guint gth_folder_tree_signals[LAST_SIGNAL] = { 0 };


G_DEFINE_TYPE (GthFolderTree, gth_folder_tree, GTK_TYPE_TREE_VIEW)


static void remove_all_locations_from_the_monitor (GthFolderTree *folder_tree);


static void
gth_folder_tree_finalize (GObject *object)
{
	GthFolderTree *folder_tree;

	folder_tree = GTH_FOLDER_TREE (object);

	if (folder_tree->priv != NULL) {
		if (folder_tree->priv->drag_target_list != NULL) {
			gtk_target_list_unref (folder_tree->priv->drag_target_list);
			folder_tree->priv->drag_target_list = NULL;
		}
		if (folder_tree->priv->monitor.update_id != 0) {
			g_source_remove (folder_tree->priv->monitor.update_id);
			folder_tree->priv->monitor.update_id = 0;
		}
		g_hash_table_unref (folder_tree->priv->entry_points);
		remove_all_locations_from_the_monitor (folder_tree);
		g_hash_table_unref (folder_tree->priv->monitor.locations);
		_g_object_list_unref (folder_tree->priv->monitor.sources);
		if (folder_tree->priv->root != NULL)
			g_object_unref (folder_tree->priv->root);
		gth_icon_cache_free (folder_tree->priv->icon_cache);

		g_free (folder_tree->priv);
		folder_tree->priv = NULL;
	}

	G_OBJECT_CLASS (gth_folder_tree_parent_class)->finalize (object);
}


static void
gth_folder_tree_class_init (GthFolderTreeClass *class)
{
	GObjectClass   *object_class;
	GtkWidgetClass *widget_class;

	object_class = (GObjectClass*) class;
	object_class->finalize = gth_folder_tree_finalize;

	widget_class = (GtkWidgetClass*) class;
	widget_class->drag_begin = NULL;

	gth_folder_tree_signals[FOLDER_POPUP] =
		g_signal_new ("folder_popup",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthFolderTreeClass, folder_popup),
			      NULL, NULL,
			      gth_marshal_VOID__OBJECT_UINT,
			      G_TYPE_NONE,
			      2,
			      G_TYPE_OBJECT,
			      G_TYPE_UINT);
	gth_folder_tree_signals[LIST_CHILDREN] =
		g_signal_new ("list_children",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthFolderTreeClass, list_children),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE,
			      1,
			      G_TYPE_OBJECT);
	gth_folder_tree_signals[LOAD] =
		g_signal_new ("load",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthFolderTreeClass, load),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE,
			      1,
			      G_TYPE_OBJECT);
	gth_folder_tree_signals[OPEN] =
		g_signal_new ("open",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthFolderTreeClass, open),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE,
			      1,
			      G_TYPE_OBJECT);
	gth_folder_tree_signals[OPEN_PARENT] =
		g_signal_new ("open_parent",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthFolderTreeClass, open_parent),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
	gth_folder_tree_signals[RENAME] =
		g_signal_new ("rename",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthFolderTreeClass, rename),
			      NULL, NULL,
			      gth_marshal_VOID__OBJECT_STRING,
			      G_TYPE_NONE,
			      2,
			      G_TYPE_OBJECT,
			      G_TYPE_STRING);
}


static void
text_renderer_edited_cb (GtkCellRendererText *renderer,
			 char                *path,
			 char                *new_text,
			 gpointer             user_data)
{
	GthFolderTree *folder_tree = user_data;
	GtkTreePath   *tree_path;
	GtkTreeIter    iter;
	EntryType      entry_type;
	GthFileData   *file_data;
	char          *name;

	g_object_set (folder_tree->priv->text_renderer,
		      "editable", FALSE,
		      NULL);

	tree_path = gtk_tree_path_new_from_string (path);
	if (! gtk_tree_model_get_iter (GTK_TREE_MODEL (folder_tree->priv->tree_store),
				       &iter,
				       tree_path))
	{
		gtk_tree_path_free (tree_path);
		return;
	}
	gtk_tree_path_free (tree_path);

	gtk_tree_model_get (GTK_TREE_MODEL (folder_tree->priv->tree_store),
			    &iter,
			    COLUMN_TYPE, &entry_type,
			    COLUMN_FILE_DATA, &file_data,
			    COLUMN_NAME, &name,
			    -1);

	if ((entry_type == ENTRY_TYPE_FILE) && (g_utf8_collate (name, new_text) != 0))
		g_signal_emit (folder_tree, gth_folder_tree_signals[RENAME], 0, file_data->file, new_text);

	_g_object_unref (file_data);
	g_free (name);
}


static void
text_renderer_editing_started_cb (GtkCellRenderer *cell,
				  GtkCellEditable *editable,
				  const char      *path,
				  gpointer         user_data)
{
	GthFolderTree *folder_tree = user_data;
	GtkTreePath   *tree_path;
	GtkTreeIter    iter;
	GthFileData   *file_data;

	tree_path = gtk_tree_path_new_from_string (path);
	if (! gtk_tree_model_get_iter (GTK_TREE_MODEL (folder_tree->priv->tree_store),
				       &iter,
				       tree_path))
	{
		gtk_tree_path_free (tree_path);
		return;
	}
	gtk_tree_path_free (tree_path);

	gtk_tree_model_get (GTK_TREE_MODEL (folder_tree->priv->tree_store),
			    &iter,
			    COLUMN_FILE_DATA, &file_data,
			    -1);

	if (GTK_IS_ENTRY (editable))
	      gtk_entry_set_text (GTK_ENTRY (editable), g_file_info_get_edit_name (file_data->info));

	_g_object_unref (file_data);
}


static void
text_renderer_editing_canceled_cb (GtkCellRenderer *renderer,
				    gpointer         user_data)
{
	GthFolderTree *folder_tree = user_data;

	g_object_set (folder_tree->priv->text_renderer,
		      "editable", FALSE,
		      NULL);
}


static void
add_columns (GthFolderTree *folder_tree,
	     GtkTreeView   *treeview)
{
	GtkCellRenderer   *renderer;
	GtkTreeViewColumn *column;

	column = gtk_tree_view_column_new ();

	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_attributes (column, renderer,
					     "pixbuf", COLUMN_ICON,
					     NULL);

	folder_tree->priv->text_renderer = renderer = gtk_cell_renderer_text_new ();
	g_object_set (renderer,
		      "ellipsize", PANGO_ELLIPSIZE_END,
		      NULL);
	g_signal_connect (folder_tree->priv->text_renderer,
			  "edited",
			  G_CALLBACK (text_renderer_edited_cb),
			  folder_tree);
	g_signal_connect (folder_tree->priv->text_renderer,
			  "editing-started",
			  G_CALLBACK (text_renderer_editing_started_cb),
			  folder_tree);
	g_signal_connect (folder_tree->priv->text_renderer,
			  "editing-canceled",
			  G_CALLBACK (text_renderer_editing_canceled_cb),
			  folder_tree);

	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_attributes (column, renderer,
					     "text", COLUMN_NAME,
					     "style", COLUMN_STYLE,
					     "weight", COLUMN_WEIGHT,
					     NULL);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_sort_column_id (column, COLUMN_NAME);

	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
}


static void
open_uri (GthFolderTree *folder_tree,
	  GthFileData   *file_data,
	  EntryType      entry_type)
{
	if (entry_type == ENTRY_TYPE_PARENT)
		g_signal_emit (folder_tree, gth_folder_tree_signals[OPEN_PARENT], 0);
	else if (entry_type == ENTRY_TYPE_FILE)
		g_signal_emit (folder_tree, gth_folder_tree_signals[OPEN], 0, file_data->file);
}


static gboolean
row_activated_cb (GtkTreeView       *tree_view,
		  GtkTreePath       *path,
		  GtkTreeViewColumn *column,
		  gpointer           user_data)
{
	GthFolderTree *folder_tree = user_data;
	GtkTreeIter    iter;
	EntryType      entry_type;
	GthFileData   *file_data;

	if (! gtk_tree_model_get_iter (GTK_TREE_MODEL (folder_tree->priv->tree_store),
				       &iter,
				       path))
	{
		return FALSE;
	}

	gtk_tree_model_get (GTK_TREE_MODEL (folder_tree->priv->tree_store),
			    &iter,
			    COLUMN_TYPE, &entry_type,
			    COLUMN_FILE_DATA, &file_data,
			    -1);
	open_uri (folder_tree, file_data, entry_type);

	_g_object_unref (file_data);

	return TRUE;
}


/* -- update_monitored_locations  -- */


static GthFileSource *
get_monitor_file_source_for_file (GthFolderTree *folder_tree,
				  GFile         *file)
{
	GList *scan;
	char  *uri;

	uri = g_file_get_uri (file);
	for (scan = folder_tree->priv->monitor.sources; scan; scan = scan->next) {
		GthFileSource *file_source = scan->data;

		if (gth_file_source_supports_scheme (file_source, uri)) {
			g_free (uri);
			return file_source;
		}
	}

	g_free (uri);

	return NULL;
}


static void
_gth_folder_tree_remove_from_monitor (GthFolderTree *folder_tree,
				      GFile         *file)
{
	GthFileSource *file_source;

	file_source = get_monitor_file_source_for_file (folder_tree, file);
	if (file_source == NULL)
		return;

	gth_file_source_monitor_directory (file_source, file, FALSE);
}


static void
remove_all_locations_from_the_monitor (GthFolderTree *folder_tree)
{
	GList *locations;
	GList *scan;

	locations = g_hash_table_get_keys (folder_tree->priv->monitor.locations);
	for (scan = locations; scan; scan = scan->next)
		_gth_folder_tree_remove_from_monitor (folder_tree, G_FILE (scan->data));
	g_hash_table_remove_all (folder_tree->priv->monitor.locations);

	g_list_free (locations);
}


static void
_gth_folder_tree_add_to_monitor (GthFolderTree *folder_tree,
				 GFile         *file)
{
	GthFileSource *file_source;

	file_source = get_monitor_file_source_for_file (folder_tree, file);
	if (file_source == NULL) {
		file_source = gth_main_get_file_source (file);
		if (file_source == NULL)
			return;

		folder_tree->priv->monitor.sources = g_list_prepend (folder_tree->priv->monitor.sources, file_source);
	}

	gth_file_source_monitor_directory (file_source, file, TRUE);
	g_hash_table_add (folder_tree->priv->monitor.locations, file);
}


static void
add_to_open_locations (GtkTreeView *tree_view,
		       GtkTreePath *path,
		       gpointer     user_data)
{
	GHashTable  *open_locations = user_data;
	GthFileData *file_data;

	file_data = gth_folder_tree_get_file (GTH_FOLDER_TREE (tree_view), path);
	if (file_data != NULL) {
		g_hash_table_add (open_locations, g_object_ref (file_data->file));
		g_object_unref (file_data);
	}
}


static gboolean
update_monitored_locations (gpointer user_data)
{
	GthFolderTree *folder_tree = user_data;
	GHashTable    *open_locations;
	GList         *locations;
	GList         *locations_to_remove;
	GList         *scan;

	if (folder_tree->priv->monitor.update_id != 0) {
		g_source_remove (folder_tree->priv->monitor.update_id);
		folder_tree->priv->monitor.update_id = 0;
	}

	open_locations = g_hash_table_new_full (g_file_hash, (GEqualFunc) g_file_equal, g_object_unref, NULL);
	gtk_tree_view_map_expanded_rows (GTK_TREE_VIEW (folder_tree),
					 add_to_open_locations,
					 open_locations);

#if 0
	{
		g_print ("** expanded locations **\n");

		locations = g_hash_table_get_keys (open_locations);
		for (scan = locations; scan; scan = scan->next) {
			GFile *file = scan->data;

			g_print ("\t%s\n", g_file_get_uri (file));
		}
		g_list_free (locations);
	}
#endif

	/* remove the old locations */

	locations_to_remove = NULL;
	locations = g_hash_table_get_keys (folder_tree->priv->monitor.locations);
	for (scan = locations; scan; scan = scan->next) {
		GFile *file = scan->data;

		if (! g_hash_table_contains (open_locations, file)) {
			_gth_folder_tree_remove_from_monitor (folder_tree, file);
			locations_to_remove = g_list_prepend (locations_to_remove, g_object_ref (file));
		}
	}

	for (scan = locations_to_remove; scan; scan = scan->next)
		g_hash_table_remove (folder_tree->priv->monitor.locations, G_FILE (scan->data));

	g_list_free (locations);
	g_list_free (locations_to_remove);

	/* add the new locations */

	locations = g_hash_table_get_keys (open_locations);
	for (scan = locations; scan; scan = scan->next) {
		GFile *file = scan->data;

		if (! g_hash_table_contains (folder_tree->priv->monitor.locations, file))
			_gth_folder_tree_add_to_monitor (folder_tree, file);
	}

	g_list_free (locations);
	g_hash_table_unref (open_locations);

	return FALSE;
}


static void
queue_update_monitored_locations (GthFolderTree *folder_tree)
{
	if (folder_tree->priv->monitor.update_id != 0)
		g_source_remove (folder_tree->priv->monitor.update_id);
	folder_tree->priv->monitor.update_id = g_timeout_add (UPDATE_MONITORED_LOCATIONS_DELAY, update_monitored_locations, folder_tree);
}


static gboolean
row_expanded_cb (GtkTreeView  *tree_view,
		 GtkTreeIter  *expanded_iter,
		 GtkTreePath  *expanded_path,
		 gpointer      user_data)
{
	GthFolderTree *folder_tree = user_data;
	EntryType      entry_type;
	GthFileData   *file_data;
	gboolean       loaded;

	gtk_tree_model_get (GTK_TREE_MODEL (folder_tree->priv->tree_store),
			    expanded_iter,
			    COLUMN_TYPE, &entry_type,
			    COLUMN_FILE_DATA, &file_data,
			    COLUMN_LOADED, &loaded,
			    -1);

	if ((entry_type == ENTRY_TYPE_FILE) && ! loaded)
		g_signal_emit (folder_tree, gth_folder_tree_signals[LIST_CHILDREN], 0, file_data->file);

	queue_update_monitored_locations (folder_tree);

	_g_object_unref (file_data);

	return FALSE;
}


static gboolean
row_collapsed_cb (GtkTreeView *tree_view,
		  GtkTreeIter *iter,
		  GtkTreePath *path,
		  gpointer     user_data)
{
	queue_update_monitored_locations (GTH_FOLDER_TREE (user_data));
	return FALSE;
}


static gboolean
popup_menu_cb (GtkWidget *widget,
	       gpointer   user_data)
{
	GthFolderTree *folder_tree = user_data;
	GtkTreeStore  *tree_store = folder_tree->priv->tree_store;
	GthFileData   *file_data = NULL;
	GtkTreeIter    iter;

	if (gtk_tree_selection_get_selected (gtk_tree_view_get_selection (GTK_TREE_VIEW (folder_tree)), NULL, &iter)) {
		EntryType entry_type;

		gtk_tree_model_get (GTK_TREE_MODEL (tree_store),
				    &iter,
				    COLUMN_TYPE, &entry_type,
				    COLUMN_FILE_DATA, &file_data,
				    -1);
		if (entry_type != ENTRY_TYPE_FILE) {
			_g_object_unref (file_data);
			return FALSE;
		}
	}

	g_signal_emit (folder_tree,
		       gth_folder_tree_signals[FOLDER_POPUP],
		       0,
		       file_data,
		       gtk_get_current_event_time ());

	_g_object_unref (file_data);

	return TRUE;
}


static int
button_press_cb (GtkWidget      *widget,
		 GdkEventButton *event,
		 gpointer        user_data)
{
	GthFolderTree     *folder_tree = user_data;
	GtkTreeStore      *tree_store = folder_tree->priv->tree_store;
	GtkTreePath       *path;
	GtkTreeIter        iter;
	gboolean           retval;
	GtkTreeViewColumn *column;
	int                cell_x;
	int                cell_y;

	retval = FALSE;

	gtk_widget_grab_focus (widget);

	if ((event->state & GDK_SHIFT_MASK) || (event->state & GDK_CONTROL_MASK))
		return retval;

	if ((event->button != 1) && (event->button != 3))
		return retval;

	if (! gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (folder_tree),
					     event->x, event->y,
					     &path,
					     &column,
					     &cell_x,
					     &cell_y))
	{
		if (event->button == 3) {
			g_signal_emit (folder_tree,
				       gth_folder_tree_signals[FOLDER_POPUP],
				       0,
				       NULL,
				       event->time);
			retval = TRUE;
		}

		return retval;
	}

	if (! gtk_tree_model_get_iter (GTK_TREE_MODEL (tree_store),
				       &iter,
				       path))
	{
		gtk_tree_path_free (path);
		return retval;
	}

 	if (event->button == 3) {
 		EntryType    entry_type;
		GthFileData *file_data;

		gtk_tree_model_get (GTK_TREE_MODEL (tree_store),
				    &iter,
				    COLUMN_TYPE, &entry_type,
				    COLUMN_FILE_DATA, &file_data,
				    -1);

		if (entry_type == ENTRY_TYPE_FILE) {
			g_signal_emit (folder_tree,
				       gth_folder_tree_signals[FOLDER_POPUP],
				       0,
				       file_data,
				       event->time);
			retval = TRUE;
		}

		_g_object_unref (file_data);
 	}
	else if ((event->button == 1) && (event->type == GDK_BUTTON_PRESS)) {
		/* This can be the start of a dragging action. */

		if (! (event->state & GDK_CONTROL_MASK)
		    && ! (event->state & GDK_SHIFT_MASK)
		    && folder_tree->priv->drag_source_enabled)
		{
			folder_tree->priv->dragging = TRUE;
			folder_tree->priv->drag_start_x = event->x;
			folder_tree->priv->drag_start_y = event->y;
		}
	}
	else if ((event->button == 1) && (event->type == GDK_2BUTTON_PRESS)) {
		if (! gtk_tree_view_row_expanded (GTK_TREE_VIEW (folder_tree), path))
			gtk_tree_view_expand_row (GTK_TREE_VIEW (folder_tree), path, FALSE);
		else
			gtk_tree_view_collapse_row (GTK_TREE_VIEW (folder_tree), path);
		retval = TRUE;
	}

	gtk_tree_path_free (path);

	return retval;
}


static gboolean
motion_notify_event_cb (GtkWidget      *widget,
			GdkEventButton *event,
			gpointer        user_data)
{
	GthFolderTree *folder_tree = user_data;

	if (! folder_tree->priv->drag_source_enabled)
		return FALSE;

	if (folder_tree->priv->dragging) {
		if (! folder_tree->priv->drag_started
		    && gtk_drag_check_threshold (widget,
					         folder_tree->priv->drag_start_x,
					         folder_tree->priv->drag_start_y,
						 event->x,
						 event->y))
		{
			GtkTreePath     *path = NULL;
			GdkDragContext  *context;
			int              cell_x;
			int              cell_y;
			cairo_surface_t *dnd_surface;

			if (! gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (folder_tree),
							     event->x,
							     event->y,
							     &path,
							     NULL,
							     &cell_x,
							     &cell_y))
			{
				return FALSE;
			}

			gtk_tree_view_set_cursor (GTK_TREE_VIEW (folder_tree), path, NULL, FALSE);
			folder_tree->priv->drag_started = TRUE;

			/**/

			context = gtk_drag_begin (widget,
						  folder_tree->priv->drag_target_list,
						  folder_tree->priv->drag_actions,
						  1,
						  (GdkEvent *) event);

			dnd_surface = gtk_tree_view_create_row_drag_icon (GTK_TREE_VIEW (folder_tree), path);
			gtk_drag_set_icon_surface (context, dnd_surface);

			cairo_surface_destroy (dnd_surface);
			gtk_tree_path_free (path);
		}

		return TRUE;
	}

	return FALSE;
}


static gboolean
button_release_event_cb (GtkWidget      *widget,
			 GdkEventButton *event,
			 gpointer        user_data)
{
	GthFolderTree *folder_tree = user_data;

	if (folder_tree->priv->dragging) {
		folder_tree->priv->dragging = FALSE;
		folder_tree->priv->drag_started = FALSE;
	}

	return FALSE;
}


static void
load_uri (GthFolderTree *folder_tree,
	  EntryType      entry_type,
	  GthFileData   *file_data)
{
	if (entry_type == ENTRY_TYPE_FILE)
		g_signal_emit (folder_tree, gth_folder_tree_signals[LOAD], 0, file_data->file);
}


static gboolean
selection_changed_cb (GtkTreeSelection *selection,
		      gpointer          user_data)
{
	GthFolderTree *folder_tree = user_data;
	GtkTreeIter    iter;
	GtkTreePath   *selected_path;
	EntryType      entry_type;
	GthFileData   *file_data;

	if (! gtk_tree_selection_get_selected (selection, NULL, &iter))
		return FALSE;

	selected_path = gtk_tree_model_get_path (GTK_TREE_MODEL (folder_tree->priv->tree_store), &iter);

	gtk_tree_model_get (GTK_TREE_MODEL (folder_tree->priv->tree_store),
			    &iter,
			    COLUMN_TYPE, &entry_type,
			    COLUMN_FILE_DATA, &file_data,
			    -1);

	load_uri (folder_tree, entry_type, file_data);

	_g_object_unref (file_data);
	gtk_tree_path_free (selected_path);

	return FALSE;
}


static gint
column_name_compare_func (GtkTreeModel *model,
			  GtkTreeIter  *a,
			  GtkTreeIter  *b,
			  gpointer      user_data)
{
	char       *key_a;
	char       *key_b;
	int         order_a;
	int         order_b;
	PangoStyle  style_a;
	PangoStyle  style_b;
	gboolean    result;

	gtk_tree_model_get (model, a,
			    COLUMN_SORT_KEY, &key_a,
			    COLUMN_SORT_ORDER, &order_a,
			    COLUMN_STYLE, &style_a,
			    -1);
	gtk_tree_model_get (model, b,
			    COLUMN_SORT_KEY, &key_b,
			    COLUMN_SORT_ORDER, &order_b,
			    COLUMN_STYLE, &style_b,
			    -1);

	if (order_a == order_b) {
		if (style_a == style_b)
			result = strcmp (key_a, key_b);
		else if (style_a == PANGO_STYLE_ITALIC)
			result = -1;
		else
			result = 1;
	}
	else if (order_a < order_b)
		result = -1;
	else
		result = 1;

	g_free (key_a);
	g_free (key_b);

	return result;
}


static gboolean
iter_stores_file (GtkTreeModel *tree_model,
	          GtkTreeIter  *iter,
	          GFile        *file,
	          GtkTreeIter  *file_iter)
{
	GthFileData *iter_file_data;
	EntryType    iter_type;
	gboolean     found;

	gtk_tree_model_get (tree_model, iter,
			    COLUMN_FILE_DATA, &iter_file_data,
			    COLUMN_TYPE, &iter_type,
			    -1);
	found = (iter_type == ENTRY_TYPE_FILE) && (iter_file_data != NULL) && g_file_equal (file, iter_file_data->file);

	_g_object_unref (iter_file_data);

	if (found)
		*file_iter = *iter;

	return found;
}


static gboolean
_gth_folder_tree_find_file_in_children (GtkTreeModel *tree_model,
				        GFile        *file,
				        GtkTreeIter  *file_iter,
				        GtkTreeIter  *root)
{
	GtkTreeIter iter;

	/* check the children... */

	if (! gtk_tree_model_iter_children (tree_model, &iter, root))
		return FALSE;

	do {
		if (iter_stores_file (tree_model, &iter, file, file_iter))
			return TRUE;
	}
	while (gtk_tree_model_iter_next (tree_model, &iter));

	/* ...if no child stores the file, search recursively */

	if (gtk_tree_model_iter_children (tree_model, &iter, root)) {
		do {
			if (_gth_folder_tree_find_file_in_children (tree_model, file, file_iter, &iter))
				return TRUE;
		}
		while (gtk_tree_model_iter_next (tree_model, &iter));
	}

	return FALSE;
}


static gboolean
gth_folder_tree_get_iter (GthFolderTree *folder_tree,
			  GFile         *file,
			  GtkTreeIter   *file_iter,
			  GtkTreeIter   *root)
{
	GtkTreeModel *tree_model = GTK_TREE_MODEL (folder_tree->priv->tree_store);

	if (file == NULL)
		return FALSE;

	if ((root != NULL) && iter_stores_file (tree_model, root, file, file_iter))
		return TRUE;

	/* This type of search is useful to give priority to the first level
	 * of entries which contains all the entry points.
	 * For example if file is "file:///media/usb-disk" this function must
	 * return the entry point corresponding to the device instead of
	 * returing the usb-disk folder located in "file:///media". */

	if (_gth_folder_tree_find_file_in_children (tree_model, file, file_iter, root))
		return TRUE;

	return FALSE;
}


static gboolean
_gth_folder_tree_get_child (GthFolderTree *folder_tree,
			    GFile         *file,
			    GtkTreeIter   *file_iter,
			    GtkTreeIter   *parent)
{
	GtkTreeModel *tree_model = GTK_TREE_MODEL (folder_tree->priv->tree_store);
	GtkTreeIter   iter;

	if (! gtk_tree_model_iter_children (tree_model, &iter, parent))
		return FALSE;

	do {
		GthFileData *test_file_data;
		EntryType    file_entry_type;

		gtk_tree_model_get (tree_model, &iter,
				    COLUMN_FILE_DATA, &test_file_data,
				    COLUMN_TYPE, &file_entry_type,
				    -1);
		if ((file_entry_type == ENTRY_TYPE_FILE) && (test_file_data != NULL) && g_file_equal (file, test_file_data->file)) {
			_g_object_unref (test_file_data);
			*file_iter = iter;
			return TRUE;
		}

		_g_object_unref (test_file_data);
	}
	while (gtk_tree_model_iter_next (tree_model, &iter));

	return FALSE;
}


static gboolean
_gth_folder_tree_child_type_present (GthFolderTree *folder_tree,
				     GtkTreeIter   *parent,
				     EntryType      entry_type)
{
	GtkTreeIter iter;

	if (! gtk_tree_model_iter_children (GTK_TREE_MODEL (folder_tree->priv->tree_store), &iter, parent))
		return FALSE;

	do {
		EntryType file_entry_type;

		gtk_tree_model_get (GTK_TREE_MODEL (folder_tree->priv->tree_store), &iter,
				    COLUMN_TYPE, &file_entry_type,
				    -1);

		if (entry_type == file_entry_type)
			return TRUE;
	}
	while (gtk_tree_model_iter_next (GTK_TREE_MODEL (folder_tree->priv->tree_store), &iter));

	return FALSE;
}


static void
_gth_folder_tree_add_loading_item (GthFolderTree *folder_tree,
				   GtkTreeIter   *parent,
				   gboolean       forced)
{
	char        *sort_key;
	GtkTreeIter  iter;

	if (! forced && _gth_folder_tree_child_type_present (folder_tree, parent, ENTRY_TYPE_LOADING))
		return;

	sort_key = g_utf8_collate_key_for_filename (LOADING_URI, -1);

	gtk_tree_store_append (folder_tree->priv->tree_store, &iter, parent);
	gtk_tree_store_set (folder_tree->priv->tree_store, &iter,
			    COLUMN_STYLE, PANGO_STYLE_ITALIC,
			    COLUMN_TYPE, ENTRY_TYPE_LOADING,
			    COLUMN_NAME, _("Loading..."),
			    COLUMN_SORT_KEY, sort_key,
			    COLUMN_SORT_ORDER, 0,
			    -1);

	g_free (sort_key);
}


static void
_gth_folder_tree_add_empty_item (GthFolderTree *folder_tree,
				 GtkTreeIter   *parent,
				 gboolean       forced)
{
	char        *sort_key;
	GtkTreeIter  iter;

	if (! forced && _gth_folder_tree_child_type_present (folder_tree, parent, ENTRY_TYPE_EMPTY))
		return;

	sort_key = g_utf8_collate_key_for_filename (EMPTY_URI, -1);

	gtk_tree_store_append (folder_tree->priv->tree_store, &iter, parent);
	gtk_tree_store_set (folder_tree->priv->tree_store, &iter,
			    COLUMN_STYLE, PANGO_STYLE_ITALIC,
			    COLUMN_TYPE, ENTRY_TYPE_EMPTY,
			    COLUMN_NAME, _("(Empty)"),
			    COLUMN_SORT_KEY, sort_key,
			    COLUMN_SORT_ORDER, 0,
			    -1);

	g_free (sort_key);
}


static gboolean
_gth_folder_tree_set_file_data (GthFolderTree *folder_tree,
				GtkTreeIter   *iter,
				GthFileData   *file_data)
{
	const char *name;
	char       *sort_key;
	GIcon      *icon;
	GdkPixbuf  *pixbuf;

	name = g_file_info_get_display_name (file_data->info);
	if (name == NULL)
		return FALSE;

	sort_key = g_utf8_collate_key_for_filename (name, -1);
	icon = g_file_info_get_icon (file_data->info);
	pixbuf = gth_icon_cache_get_pixbuf (folder_tree->priv->icon_cache, icon);

	gtk_tree_store_set (folder_tree->priv->tree_store, iter,
			    COLUMN_STYLE, PANGO_STYLE_NORMAL,
			    COLUMN_ICON, pixbuf,
			    COLUMN_TYPE, ENTRY_TYPE_FILE,
			    COLUMN_FILE_DATA, file_data,
			    COLUMN_NAME, name,
			    COLUMN_SORT_KEY, sort_key,
			    COLUMN_SORT_ORDER, g_file_info_get_sort_order (file_data->info),
			    COLUMN_NO_CHILD, g_file_info_get_attribute_boolean (file_data->info, "gthumb::no-child"),
			    COLUMN_LOADED, FALSE,
			    -1);

	g_free (sort_key);
	_g_object_unref (pixbuf);

	return TRUE;
}


static gboolean
_gth_folder_tree_iter_has_no_child (GthFolderTree *folder_tree,
				    GtkTreeIter   *iter)
{
	gboolean no_child;

	if (iter == NULL)
		return FALSE;

	gtk_tree_model_get (GTK_TREE_MODEL (folder_tree->priv->tree_store), iter,
			    COLUMN_NO_CHILD, &no_child,
			    -1);

	return no_child;
}


static void
_gth_folder_tree_update_entry_points (GthFolderTree *folder_tree)
{
	GtkTreeIter iter;

	if (! folder_tree->priv->recalc_entry_points)
		return;

	folder_tree->priv->recalc_entry_points = FALSE;

	g_hash_table_remove_all (folder_tree->priv->entry_points);

	if (! gtk_tree_model_iter_children (GTK_TREE_MODEL (folder_tree->priv->tree_store), &iter, NULL))
		return;

	do {
		GthFileData *file_data;
		EntryType    file_entry_type;

		gtk_tree_model_get (GTK_TREE_MODEL (folder_tree->priv->tree_store), &iter,
				    COLUMN_FILE_DATA, &file_data,
				    COLUMN_TYPE, &file_entry_type,
				    -1);
		if ((file_entry_type == ENTRY_TYPE_FILE) && (file_data != NULL))
			g_hash_table_add (folder_tree->priv->entry_points, g_object_ref (file_data->file));

		_g_object_unref (file_data);
	}
	while (gtk_tree_model_iter_next (GTK_TREE_MODEL (folder_tree->priv->tree_store), &iter));
}


/*
 * Returns TRUE if file_data points to a folder contained in the entry point
 * list but it's not real entry point, for example it returns TRUE for
 * '/home/user/Images' or '/home/user/Documents'.
 * This entries are duplicates of the entry points and are treated in a special
 * way to avoid confusion.
 * */
static gboolean
_gth_folder_tree_is_entry_point_dup (GthFolderTree *folder_tree,
				     GtkTreeIter   *iter,
				     GthFileData   *file_data)
{
	_gth_folder_tree_update_entry_points (folder_tree);

	if (g_hash_table_lookup (folder_tree->priv->entry_points, file_data->file) == NULL)
		return FALSE;

	return ! g_file_info_get_attribute_boolean (file_data->info, "gthumb::entry-point");
}


static gboolean
_gth_folder_tree_add_file (GthFolderTree *folder_tree,
			   GtkTreeIter   *parent,
			   GthFileData   *fd)
{
	GtkTreeIter iter;

	if (g_file_info_get_file_type (fd->info) != G_FILE_TYPE_DIRECTORY)
		return FALSE;

	/* add the folder */

	gtk_tree_store_append (folder_tree->priv->tree_store, &iter, parent);
	if (! _gth_folder_tree_set_file_data (folder_tree, &iter, fd)) {
		gtk_tree_store_remove (folder_tree->priv->tree_store, &iter);
		return FALSE;
	}

	if (g_file_info_get_attribute_boolean (fd->info, "gthumb::entry-point"))
		gtk_tree_store_set (folder_tree->priv->tree_store, &iter,
				    COLUMN_WEIGHT, PANGO_WEIGHT_BOLD,
				    -1);
	else
		gtk_tree_store_set (folder_tree->priv->tree_store, &iter,
				    COLUMN_WEIGHT, PANGO_WEIGHT_NORMAL,
				    -1);

	if (! g_file_info_get_attribute_boolean (fd->info, "gthumb::no-child")
	    && ! _gth_folder_tree_is_entry_point_dup (folder_tree, &iter, fd))
	{
		_gth_folder_tree_add_loading_item (folder_tree, &iter, TRUE);
	}

	return TRUE;
}


static void
gth_folder_tree_init (GthFolderTree *folder_tree)
{
	GtkTreeSelection *selection;

	folder_tree->priv = g_new0 (GthFolderTreePrivate, 1);
	folder_tree->priv->drag_source_enabled = FALSE;
	folder_tree->priv->dragging = FALSE;
	folder_tree->priv->drag_started = FALSE;
	folder_tree->priv->drag_target_list = NULL;
	folder_tree->priv->entry_points = g_hash_table_new_full (g_file_hash, (GEqualFunc) g_file_equal, g_object_unref, NULL);
	folder_tree->priv->recalc_entry_points = FALSE;
	folder_tree->priv->monitor.locations = g_hash_table_new_full (g_file_hash, (GEqualFunc) g_file_equal, g_object_unref, NULL);
	folder_tree->priv->monitor.sources = NULL;
	folder_tree->priv->monitor.update_id = 0;
	folder_tree->priv->icon_cache = gth_icon_cache_new (gtk_icon_theme_get_for_screen (gtk_widget_get_screen (GTK_WIDGET (folder_tree))),
							    _gtk_widget_lookup_for_size (GTK_WIDGET (folder_tree), GTK_ICON_SIZE_MENU));

	folder_tree->priv->tree_store = gtk_tree_store_new (NUM_COLUMNS,
							    PANGO_TYPE_STYLE,
							    PANGO_TYPE_WEIGHT,
							    GDK_TYPE_PIXBUF,
							    G_TYPE_INT,
							    G_TYPE_OBJECT,
							    G_TYPE_STRING,
							    G_TYPE_INT,
							    G_TYPE_STRING,
							    G_TYPE_BOOLEAN,
							    G_TYPE_BOOLEAN);
	gtk_tree_view_set_model (GTK_TREE_VIEW (folder_tree), GTK_TREE_MODEL (folder_tree->priv->tree_store));
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (folder_tree), FALSE);

	add_columns (folder_tree, GTK_TREE_VIEW (folder_tree));

	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (folder_tree), FALSE);
	gtk_tree_view_set_enable_search (GTK_TREE_VIEW (folder_tree), TRUE);
	gtk_tree_view_set_search_column (GTK_TREE_VIEW (folder_tree), COLUMN_NAME);
	gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (folder_tree->priv->tree_store), COLUMN_NAME, column_name_compare_func, folder_tree, NULL);
	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (folder_tree->priv->tree_store), COLUMN_NAME, GTK_SORT_ASCENDING);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (folder_tree));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
	g_signal_connect (selection,
			  "changed",
			  G_CALLBACK (selection_changed_cb),
			  folder_tree);

	/**/

	g_signal_connect (folder_tree,
			  "popup-menu",
			  G_CALLBACK (popup_menu_cb),
			  folder_tree);
	g_signal_connect (folder_tree,
			  "button-press-event",
			  G_CALLBACK (button_press_cb),
			  folder_tree);
	g_signal_connect (folder_tree,
			  "motion-notify-event",
			  G_CALLBACK (motion_notify_event_cb),
			  folder_tree);
	g_signal_connect (folder_tree,
			  "button-release-event",
			  G_CALLBACK (button_release_event_cb),
			  folder_tree);
	g_signal_connect (folder_tree,
			  "row-activated",
			  G_CALLBACK (row_activated_cb),
			  folder_tree);
	g_signal_connect (folder_tree,
			  "row-expanded",
			  G_CALLBACK (row_expanded_cb),
			  folder_tree);
	g_signal_connect (folder_tree,
			  "row-collapsed",
			  G_CALLBACK (row_collapsed_cb),
			  folder_tree);
}


GtkWidget *
gth_folder_tree_new (const char *uri)
{
	GthFolderTree *folder_tree;

	folder_tree = g_object_new (GTH_TYPE_FOLDER_TREE, NULL);
	if (uri != NULL)
		folder_tree->priv->root = g_file_new_for_uri (uri);

	return (GtkWidget *) folder_tree;
}


void
gth_folder_tree_set_list (GthFolderTree *folder_tree,
			  GFile         *root,
			  GList         *files,
			  gboolean       open_parent)
{
	gtk_tree_store_clear (folder_tree->priv->tree_store);

	if (folder_tree->priv->root != NULL) {
		g_object_unref (folder_tree->priv->root);
		folder_tree->priv->root = NULL;
	}
	if (root != NULL)
		folder_tree->priv->root = g_file_dup (root);

	/* add the parent folder item */

	if (open_parent) {
		char        *sort_key;
		GdkPixbuf   *pixbuf;
		GtkTreeIter  iter;

		sort_key = g_utf8_collate_key_for_filename (PARENT_URI, -1);
		pixbuf = gtk_widget_render_icon_pixbuf (GTK_WIDGET (folder_tree), GTK_STOCK_GO_UP, GTK_ICON_SIZE_MENU);

		gtk_tree_store_append (folder_tree->priv->tree_store, &iter, NULL);
		gtk_tree_store_set (folder_tree->priv->tree_store, &iter,
				    COLUMN_STYLE, PANGO_STYLE_ITALIC,
				    COLUMN_ICON, pixbuf,
				    COLUMN_TYPE, ENTRY_TYPE_PARENT,
				    COLUMN_NAME, _("(Open Parent)"),
				    COLUMN_SORT_KEY, sort_key,
				    COLUMN_SORT_ORDER, 0,
				    -1);

		g_object_unref (pixbuf);
		g_free (sort_key);
	}

	/* add the folder list */

	gth_folder_tree_set_children (folder_tree, root, files);
}


/* After changing the children list, the node expander is not highlighted
 * anymore, this prevents the user to close the expander without moving the
 * mouse pointer.  The problem can be fixed emitting a fake motion notify
 * event, this way the expander gets highlighted again and a click on the
 * expander will correctly collapse the node. */
static void
emit_fake_motion_notify_event (GthFolderTree *folder_tree)
{
	GtkWidget      *widget = GTK_WIDGET (folder_tree);
	GdkDevice      *device;
	GdkWindow      *window;
	GdkEventMotion  event;
	int             x, y;

	if (! gtk_widget_get_realized (widget))
		return;

	device = gdk_device_manager_get_client_pointer (
		   gdk_display_get_device_manager (
		     gtk_widget_get_display (GTK_WIDGET (folder_tree))));
	window = gdk_window_get_device_position (gtk_widget_get_window (widget),
						 device,
						 &x,
						 &y,
						 NULL);

	event.type = GDK_MOTION_NOTIFY;
	event.window = (window != NULL) ? window : gtk_tree_view_get_bin_window (GTK_TREE_VIEW (folder_tree));
	event.send_event = TRUE;
	event.time = GDK_CURRENT_TIME;
	event.x = x;
	event.y = y;
	event.axes = NULL;
	event.state = 0;
	event.is_hint = FALSE;
	event.device = device;

	GTK_WIDGET_GET_CLASS (folder_tree)->motion_notify_event (widget, &event);
}


G_GNUC_UNUSED
static GList *
_gth_folder_tree_get_children (GthFolderTree *folder_tree,
			       GtkTreeIter   *parent)
{
	GtkTreeModel *tree_model = GTK_TREE_MODEL (folder_tree->priv->tree_store);
	GtkTreeIter   iter;
	GList        *list;

	if (! gtk_tree_model_iter_children (tree_model, &iter, parent))
		return NULL;

	list = NULL;
	do {
		GthFileData *file_data;
		EntryType    file_type;

		gtk_tree_model_get (tree_model, &iter,
				    COLUMN_FILE_DATA, &file_data,
				    COLUMN_TYPE, &file_type,
				    -1);
		if ((file_type == ENTRY_TYPE_FILE) && (file_data != NULL))
			list = g_list_prepend (list, g_object_ref (file_data));

		_g_object_unref (file_data);
	}
	while (gtk_tree_model_iter_next (tree_model, &iter));

	return g_list_reverse (list);
}


static void
_gth_folder_tree_remove_child_type (GthFolderTree *folder_tree,
				    GtkTreeIter   *parent,
				    EntryType      entry_type)
{
	GtkTreeIter iter;

	if (! gtk_tree_model_iter_children (GTK_TREE_MODEL (folder_tree->priv->tree_store), &iter, parent))
		return;

	do {
		EntryType file_entry_type;

		gtk_tree_model_get (GTK_TREE_MODEL (folder_tree->priv->tree_store), &iter,
				    COLUMN_TYPE, &file_entry_type,
				    -1);

		if (entry_type == file_entry_type) {
			gtk_tree_store_remove (folder_tree->priv->tree_store, &iter);
			break;
		}
	}
	while (gtk_tree_model_iter_next (GTK_TREE_MODEL (folder_tree->priv->tree_store), &iter));
}


void
gth_folder_tree_set_children (GthFolderTree *folder_tree,
			       GFile         *parent,
			       GList         *files)
{
	GtkTreeIter   parent_iter;
	GtkTreeIter  *p_parent_iter;
	gboolean      is_empty;
	GHashTable   *file_hash;
	GList        *scan;
	GList        *old_files;
	GtkTreeModel *tree_model;
	GtkTreeIter   iter;

	if (g_file_equal (parent, folder_tree->priv->root))
		p_parent_iter = NULL;
	else if (gth_folder_tree_get_iter (folder_tree, parent, &parent_iter, NULL))
		p_parent_iter = &parent_iter;
	else
		return;

	if (_gth_folder_tree_iter_has_no_child (folder_tree, p_parent_iter))
		return;

	tree_model = GTK_TREE_MODEL (folder_tree->priv->tree_store);
	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (tree_model), GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID, 0);

	is_empty = TRUE;
	_gth_folder_tree_add_empty_item (folder_tree, p_parent_iter, FALSE);

	/* delete the children not present in the new file list, update the
	 * already existing files */

	file_hash = g_hash_table_new_full (g_file_hash, (GEqualFunc) g_file_equal, g_object_unref, NULL);
	for (scan = files; scan; scan = scan->next) {
		GthFileData *file_data = scan->data;
		g_hash_table_insert (file_hash, g_object_ref (file_data->file), GINT_TO_POINTER (1));
	}

	old_files = NULL;
	if (gtk_tree_model_iter_children (tree_model, &iter, p_parent_iter)) {
		gboolean valid = TRUE;

		do {
			GthFileData *file_data = NULL;
			EntryType    file_type;

			gtk_tree_model_get (tree_model, &iter,
					    COLUMN_FILE_DATA, &file_data,
					    COLUMN_TYPE, &file_type,
					    -1);

			if (file_type == ENTRY_TYPE_LOADING) {
				valid = gtk_tree_store_remove (folder_tree->priv->tree_store, &iter);
			}
			else if (file_type == ENTRY_TYPE_FILE) {
				/* save the old files list to compute the new files list below */
				old_files = g_list_prepend (old_files, g_object_ref (file_data));

				if (g_hash_table_lookup (file_hash, file_data->file)) {
					/* file_data is already present in the list, just update it */
					if (_gth_folder_tree_set_file_data (folder_tree, &iter, file_data))
						is_empty = FALSE;
					valid = gtk_tree_model_iter_next (tree_model, &iter);
				}
				else {
					/* file_data is not present anymore, remove it from the tree */
					valid = gtk_tree_store_remove (folder_tree->priv->tree_store, &iter);
				}
			}
			else
				valid = gtk_tree_model_iter_next (tree_model, &iter);

			_g_object_unref (file_data);
		}
		while (valid);
	}

	g_hash_table_unref (file_hash);

	/* add the new files */

	file_hash = g_hash_table_new_full (g_file_hash, (GEqualFunc) g_file_equal, g_object_unref, NULL);
	for (scan = old_files; scan; scan = scan->next) {
		GthFileData *file_data = scan->data;
		g_hash_table_insert (file_hash, g_object_ref (file_data->file), GINT_TO_POINTER (1));
	}

	for (scan = files; scan; scan = scan->next) {
		GthFileData *file_data = scan->data;

		if (! g_hash_table_lookup (file_hash, file_data->file)) {
			if (_gth_folder_tree_add_file (folder_tree, p_parent_iter, file_data))
				is_empty = FALSE;
		}
	}

	_g_object_list_unref (old_files);
	g_hash_table_unref (file_hash);

	/**/

	if (! is_empty)
		_gth_folder_tree_remove_child_type (folder_tree, p_parent_iter, ENTRY_TYPE_EMPTY);

	if (p_parent_iter != NULL)
		gtk_tree_store_set (folder_tree->priv->tree_store, p_parent_iter,
				    COLUMN_LOADED, TRUE,
				    -1);
	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (folder_tree->priv->tree_store), COLUMN_NAME, GTK_SORT_ASCENDING);
	folder_tree->priv->recalc_entry_points = TRUE;

	emit_fake_motion_notify_event (folder_tree);
}


void
gth_folder_tree_loading_children (GthFolderTree *folder_tree,
				  GFile         *parent)
{
	GtkTreeIter  parent_iter;
	GtkTreeIter *p_parent_iter;
	GtkTreeIter  iter;
	gboolean     valid_iter;

	if (g_file_equal (parent, folder_tree->priv->root))
		p_parent_iter = NULL;
	else if (gth_folder_tree_get_iter (folder_tree, parent, &parent_iter, NULL))
		p_parent_iter = &parent_iter;
	else
		return;

	_gth_folder_tree_add_loading_item (folder_tree, p_parent_iter, FALSE);

	/* remove anything but the loading item */
	gtk_tree_model_iter_children (GTK_TREE_MODEL (folder_tree->priv->tree_store), &iter, p_parent_iter);
	do {
		EntryType file_entry_type;

		gtk_tree_model_get (GTK_TREE_MODEL (folder_tree->priv->tree_store), &iter,
				    COLUMN_TYPE, &file_entry_type,
				    -1);

		if (file_entry_type != ENTRY_TYPE_LOADING)
			valid_iter = gtk_tree_store_remove (folder_tree->priv->tree_store, &iter);
		else
			valid_iter = gtk_tree_model_iter_next (GTK_TREE_MODEL (folder_tree->priv->tree_store), &iter);
	}
	while (valid_iter);

	emit_fake_motion_notify_event (folder_tree);
}


static gboolean
_gth_folder_tree_file_is_in_children (GthFolderTree *folder_tree,
				      GtkTreeIter   *parent_iter,
	 			      GFile         *file)
{
	GtkTreeIter iter;
	gboolean    found = FALSE;

	if (! gtk_tree_model_iter_children (GTK_TREE_MODEL (folder_tree->priv->tree_store), &iter, parent_iter))
		return FALSE;

	do {
		GthFileData *test_file_data;
		EntryType    file_entry_type;

		gtk_tree_model_get (GTK_TREE_MODEL (folder_tree->priv->tree_store), &iter,
				    COLUMN_FILE_DATA, &test_file_data,
				    COLUMN_TYPE, &file_entry_type,
				    -1);
		if ((file_entry_type == ENTRY_TYPE_FILE) && (test_file_data != NULL) && g_file_equal (file, test_file_data->file))
			found = TRUE;

		_g_object_unref (test_file_data);
	}
	while (! found && gtk_tree_model_iter_next (GTK_TREE_MODEL (folder_tree->priv->tree_store), &iter));

	return found;
}


void
gth_folder_tree_add_children (GthFolderTree *folder_tree,
			      GFile         *parent,
			      GList         *files)
{
	GtkTreeIter  parent_iter;
	GtkTreeIter *p_parent_iter;
	gboolean     is_empty;
	GList       *scan;

	if (g_file_equal (parent, folder_tree->priv->root))
		p_parent_iter = NULL;
	else if (gth_folder_tree_get_iter (folder_tree, parent, &parent_iter, NULL))
		p_parent_iter = &parent_iter;
	else
		return;

	is_empty = TRUE;
	for (scan = files; scan; scan = scan->next) {
		GthFileData *file_data = scan->data;

		if (_gth_folder_tree_file_is_in_children (folder_tree, p_parent_iter, file_data->file))
			continue;
		if (_gth_folder_tree_add_file (folder_tree, p_parent_iter, file_data))
			is_empty = FALSE;
	}

	if (! is_empty)
		_gth_folder_tree_remove_child_type (folder_tree, p_parent_iter, ENTRY_TYPE_EMPTY);

	folder_tree->priv->recalc_entry_points = TRUE;
}


void
gth_folder_tree_update_children (GthFolderTree *folder_tree,
				 GFile         *parent,
				 GList         *files)
{
	GtkTreeIter  parent_iter;
	GtkTreeIter *p_parent_iter;
	GList       *scan;
	GtkTreeIter  iter;

	if (g_file_equal (parent, folder_tree->priv->root))
		p_parent_iter = NULL;
	else if (gth_folder_tree_get_iter (folder_tree, parent, &parent_iter, NULL))
		p_parent_iter = &parent_iter;
	else
		return;

	/* update each file if already present */

	for (scan = files; scan; scan = scan->next) {
		GthFileData *file_data = scan->data;

		if (_gth_folder_tree_get_child (folder_tree, file_data->file, &iter, p_parent_iter))
			_gth_folder_tree_set_file_data (folder_tree, &iter, file_data);
	}
}


void
gth_folder_tree_update_child (GthFolderTree *folder_tree,
			      GFile         *old_file,
			      GthFileData   *file_data)
{
	GtkTreeIter old_file_iter;
	GtkTreeIter new_file_iter;

	if (! gth_folder_tree_get_iter (folder_tree, old_file, &old_file_iter, NULL))
		return;

	if (gth_folder_tree_get_iter (folder_tree, file_data->file, &new_file_iter, NULL)) {
		GFile *parent;
		GList *files;

		/* the new file is already present, remove the old file */

		parent = g_file_get_parent (old_file);
		files = g_list_prepend (NULL, g_object_ref (old_file));
		gth_folder_tree_delete_children (folder_tree, parent, files);
		_g_object_list_unref (files);
		g_object_unref (parent);

		/* update the data old of the new file */

		_gth_folder_tree_set_file_data (folder_tree, &new_file_iter, file_data);
	}
	else
		_gth_folder_tree_set_file_data (folder_tree, &old_file_iter, file_data);
}


void
gth_folder_tree_delete_children (GthFolderTree *folder_tree,
				 GFile         *parent,
				 GList         *files)
{
	GtkTreeIter  parent_iter;
	GtkTreeIter *p_parent_iter;
	GList       *scan;

	if (g_file_equal (parent, folder_tree->priv->root))
		p_parent_iter = NULL;
	else if (gth_folder_tree_get_iter (folder_tree, parent, &parent_iter, NULL))
		p_parent_iter = &parent_iter;
	else
		return;

	if (_gth_folder_tree_iter_has_no_child (folder_tree, p_parent_iter))
		return;

	/* add the empty item first to not allow the folder to collapse. */
	_gth_folder_tree_add_empty_item (folder_tree, p_parent_iter, TRUE);

	for (scan = files; scan; scan = scan->next) {
		GFile       *file = scan->data;
		GtkTreeIter  iter;

		if (_gth_folder_tree_get_child (folder_tree, file, &iter, p_parent_iter))
			gtk_tree_store_remove (folder_tree->priv->tree_store, &iter);
	}

	if (gtk_tree_model_iter_n_children (GTK_TREE_MODEL (folder_tree->priv->tree_store), p_parent_iter) > 1)
		_gth_folder_tree_remove_child_type (folder_tree, p_parent_iter, ENTRY_TYPE_EMPTY);

	folder_tree->priv->recalc_entry_points = TRUE;
}


void
gth_folder_tree_start_editing (GthFolderTree *folder_tree,
			       GFile         *file)
{
	GtkTreeIter        iter;
	GtkTreePath       *tree_path;
	GtkTreeViewColumn *tree_column;

	if (! gth_folder_tree_get_iter (folder_tree, file, &iter, NULL))
		return;

	g_object_set (folder_tree->priv->text_renderer,
		      "editable", TRUE,
		      NULL);

	tree_path = gtk_tree_model_get_path (GTK_TREE_MODEL (folder_tree->priv->tree_store), &iter);
	tree_column = gtk_tree_view_get_column (GTK_TREE_VIEW (folder_tree), 0);
	gtk_tree_view_set_cursor (GTK_TREE_VIEW (folder_tree),
				  tree_path,
				  tree_column,
				  TRUE);

	gtk_tree_path_free (tree_path);
}


GtkTreePath *
gth_folder_tree_get_path (GthFolderTree *folder_tree,
			  GFile         *file)
{
	GtkTreeIter iter;

	if (! gth_folder_tree_get_iter (folder_tree, file, &iter, NULL))
		return NULL;
	else
		return gtk_tree_model_get_path (GTK_TREE_MODEL (folder_tree->priv->tree_store), &iter);
}


gboolean
gth_folder_tree_is_loaded (GthFolderTree *folder_tree,
			   GtkTreePath   *path)
{
	GtkTreeIter iter;
	gboolean    loaded;

	if (! gtk_tree_model_get_iter (GTK_TREE_MODEL (folder_tree->priv->tree_store), &iter, path))
		return FALSE;

	gtk_tree_model_get (GTK_TREE_MODEL (folder_tree->priv->tree_store), &iter,
			    COLUMN_LOADED, &loaded,
			    -1);

	return loaded;
}


static void
_gth_folder_tree_reset_loaded (GthFolderTree *folder_tree,
			       GtkTreeIter   *root)
{
	GtkTreeModel *tree_model = GTK_TREE_MODEL (folder_tree->priv->tree_store);
	GtkTreeIter   iter;

	if (root != NULL)
		gtk_tree_store_set (folder_tree->priv->tree_store, root,
				    COLUMN_LOADED, FALSE,
				    -1);

	if (gtk_tree_model_iter_children (tree_model, &iter, root)) {
		do {
			_gth_folder_tree_reset_loaded (folder_tree, &iter);
		}
		while (gtk_tree_model_iter_next (tree_model, &iter));
	}
}


void
gth_folder_tree_reset_loaded (GthFolderTree *folder_tree)
{
	_gth_folder_tree_reset_loaded (folder_tree, NULL);
}


void
gth_folder_tree_expand_row (GthFolderTree *folder_tree,
			    GtkTreePath   *path,
			    gboolean       open_all)
{
	g_signal_handlers_block_by_func (folder_tree, row_expanded_cb, folder_tree);
	gtk_tree_view_expand_row (GTK_TREE_VIEW (folder_tree), path, open_all);
	g_signal_handlers_unblock_by_func (folder_tree, row_expanded_cb, folder_tree);
}


void
gth_folder_tree_collapse_all (GthFolderTree *folder_tree)
{
	GtkTreeSelection *selection;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (folder_tree));
	g_signal_handlers_block_by_func (selection, selection_changed_cb, folder_tree);
	gtk_tree_view_collapse_all (GTK_TREE_VIEW (folder_tree));
	g_signal_handlers_unblock_by_func (selection, selection_changed_cb, folder_tree);
}


GthFileData *
gth_folder_tree_get_file (GthFolderTree *folder_tree,
			  GtkTreePath   *path)
{
	GtkTreeModel *tree_model;
	GtkTreeIter   iter;
	EntryType     entry_type;
	GthFileData  *file_data;

	tree_model = GTK_TREE_MODEL (folder_tree->priv->tree_store);
	if (! gtk_tree_model_get_iter (tree_model, &iter, path))
			return NULL;

	file_data = NULL;
	gtk_tree_model_get (tree_model,
			    &iter,
			    COLUMN_TYPE, &entry_type,
			    COLUMN_FILE_DATA, &file_data,
			    -1);
	if (entry_type != ENTRY_TYPE_FILE) {
		_g_object_unref (file_data);
		file_data = NULL;
	}

	return file_data;
}


void
gth_folder_tree_select_path (GthFolderTree *folder_tree,
			     GtkTreePath   *path)
{
	GtkTreeSelection *selection;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (folder_tree));
	g_signal_handlers_block_by_func (selection, selection_changed_cb, folder_tree);
	gtk_tree_selection_select_path (selection, path);
	g_signal_handlers_unblock_by_func (selection, selection_changed_cb, folder_tree);
}


GFile *
gth_folder_tree_get_root (GthFolderTree *folder_tree)
{
	return folder_tree->priv->root;
}


GthFileData *
gth_folder_tree_get_selected (GthFolderTree *folder_tree)
{
	GtkTreeSelection *selection;
	GtkTreeModel     *tree_model;
	GtkTreeIter       iter;
	EntryType         entry_type;
	GthFileData      *file_data;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (folder_tree));
	if (selection == NULL)
		return NULL;

	tree_model = GTK_TREE_MODEL (folder_tree->priv->tree_store);
	if (! gtk_tree_selection_get_selected (selection, &tree_model, &iter))
		return NULL;

	gtk_tree_model_get (GTK_TREE_MODEL (folder_tree->priv->tree_store),
			    &iter,
			    COLUMN_TYPE, &entry_type,
			    COLUMN_FILE_DATA, &file_data,
			    -1);

	if (entry_type != ENTRY_TYPE_FILE) {
		_g_object_unref (file_data);
		file_data = NULL;
	}

	return file_data;
}


GthFileData *
gth_folder_tree_get_selected_or_parent (GthFolderTree *folder_tree)
{
	GtkTreeSelection *selection;
	GtkTreeModel     *tree_model;
	GtkTreeIter       iter;
	GtkTreeIter       parent;
	EntryType         entry_type;
	GthFileData      *file_data;

	file_data = gth_folder_tree_get_selected (folder_tree);
	if (file_data != NULL)
		return file_data;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (folder_tree));
	if (selection == NULL)
		return NULL;

	tree_model = GTK_TREE_MODEL (folder_tree->priv->tree_store);
	if (! gtk_tree_selection_get_selected (selection, &tree_model, &iter))
		return NULL;

	if (! gtk_tree_model_iter_parent (tree_model, &parent, &iter))
		return NULL;

	gtk_tree_model_get (GTK_TREE_MODEL (folder_tree->priv->tree_store),
			    &parent,
			    COLUMN_TYPE, &entry_type,
			    COLUMN_FILE_DATA, &file_data,
			    -1);

	if (entry_type != ENTRY_TYPE_FILE) {
		_g_object_unref (file_data);
		file_data = NULL;
	}

	return file_data;
}


void
gth_folder_tree_enable_drag_source (GthFolderTree        *self,
				    GdkModifierType       start_button_mask,
				    const GtkTargetEntry *targets,
				    int                   n_targets,
				    GdkDragAction         actions)
{
	if (self->priv->drag_target_list != NULL)
		gtk_target_list_unref (self->priv->drag_target_list);

	self->priv->drag_source_enabled = TRUE;
	self->priv->drag_start_button_mask = start_button_mask;
	self->priv->drag_target_list = gtk_target_list_new (targets, n_targets);
	self->priv->drag_actions = actions;
}


void
gth_folder_tree_unset_drag_source (GthFolderTree *self)
{
	self->priv->drag_source_enabled = FALSE;
}
