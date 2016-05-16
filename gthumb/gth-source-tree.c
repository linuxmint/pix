/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2009 Free Software Foundation, Inc.
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
#include "gth-icon-cache.h"
#include "gth-main.h"
#include "gth-preferences.h"
#include "gth-source-tree.h"


struct _GthSourceTreePrivate {
	GthFileSource *file_source;
	gulong         monitor_folder_changed_id;
	gulong         monitor_file_renamed_id;
};


G_DEFINE_TYPE (GthSourceTree, gth_source_tree, GTH_TYPE_FOLDER_TREE)


/* -- monitor_event_data -- */


typedef struct {
	int              ref;
	GFile           *parent;
	GthMonitorEvent  event;
	GthSourceTree   *source_tree;
} MonitorEventData;


static MonitorEventData *
monitor_event_data_new (void)
{
	MonitorEventData *monitor_data;

	monitor_data = g_new0 (MonitorEventData, 1);
	monitor_data->ref = 1;

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

	g_object_unref (monitor_data->parent);
	g_free (monitor_data);
}


/* -- load_data -- */


typedef struct {
	GthSourceTree *source_tree;
	GFile         *folder;
	GthFileSource *file_source;
} LoadData;


static LoadData *
load_data_new (GthSourceTree *source_tree,
	       GFile         *file)
{
	LoadData *load_data;

	load_data = g_new0 (LoadData, 1);
	load_data->source_tree = source_tree;
	load_data->file_source = gth_main_get_file_source (file);
	load_data->folder = g_file_dup (file);

	return load_data;
}


static void
load_data_free (LoadData *load_data)
{
	g_object_unref (load_data->folder);
	g_object_unref (load_data->file_source);
	g_free (load_data);
}


static void
load_data_run (LoadData  *load_data,
	       ListReady  func)
{
	GSettings *settings;

	settings = g_settings_new (GTHUMB_BROWSER_SCHEMA);
	gth_file_source_list (load_data->file_source,
			      load_data->folder,
			      g_settings_get_boolean (settings, PREF_BROWSER_FAST_FILE_TYPE) ? GFILE_STANDARD_ATTRIBUTES_WITH_FAST_CONTENT_TYPE : GFILE_STANDARD_ATTRIBUTES_WITH_CONTENT_TYPE,
			      func,
			      load_data);

	g_object_unref (settings);
}


/* -- */


static void
source_tree_children_ready (GthFileSource *file_source,
			     GList         *files,
			    GError        *error,
			    gpointer       data)
{
	LoadData      *load_data = data;
	GthSourceTree *source_tree = load_data->source_tree;

	if (error != NULL)
		g_warning ("%s", error->message);
	else
		gth_folder_tree_set_children (GTH_FOLDER_TREE (source_tree), load_data->folder, files);

	load_data_free (load_data);
}


static void
source_tree_list_children_cb (GthFolderTree *folder_tree,
			      GFile         *file,
			      GthSourceTree *source_tree)
{
	LoadData *load_data;

	gth_folder_tree_loading_children (folder_tree, file);

	load_data = load_data_new (source_tree, file);
	load_data_run (load_data, source_tree_children_ready);
}


static void
file_source_rename_ready_cb (GObject  *object,
			     GError   *error,
			     gpointer  user_data)
{
	GthSourceTree *source_tree = user_data;

	if (error != NULL)
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (source_tree))), _("Could not change name"), error);
}


static void
source_tree_rename_cb (GthFolderTree *folder_tree,
		       GFile         *file,
		       const char    *new_name,
		       GthSourceTree *source_tree)
{
	gth_file_source_rename (source_tree->priv->file_source, file, new_name, file_source_rename_ready_cb, source_tree);
}


static void
file_attributes_ready_cb (GthFileSource *file_source,
			  GList         *files,
			  GError        *error,
			  gpointer       user_data)
{
	MonitorEventData *monitor_data = user_data;
	GthSourceTree    *source_tree = monitor_data->source_tree;

	if (error != NULL) {
		g_warning ("%s", error->message);
		g_clear_error (&error);
		monitor_event_data_unref (monitor_data);
		return;
	}

	if (monitor_data->event == GTH_MONITOR_EVENT_CREATED)
		gth_folder_tree_add_children (GTH_FOLDER_TREE (source_tree), monitor_data->parent, files);
	else if (monitor_data->event == GTH_MONITOR_EVENT_CHANGED)
		gth_folder_tree_update_children (GTH_FOLDER_TREE (source_tree), monitor_data->parent, files);

	monitor_event_data_unref (monitor_data);
}


static void
monitor_folder_changed_cb (GthMonitor      *monitor,
			   GFile           *parent,
			   GList           *list,
			   int              position,
			   GthMonitorEvent  event,
			   GthSourceTree   *source_tree)
{
	GtkTreePath *path;

	path = gth_folder_tree_get_path (GTH_FOLDER_TREE (source_tree), parent);
	if (g_file_equal (parent, gth_folder_tree_get_root (GTH_FOLDER_TREE (source_tree)))
	    || ((path != NULL) && gtk_tree_view_row_expanded (GTK_TREE_VIEW (source_tree), path)))
	{
		MonitorEventData *monitor_data;
		GSettings        *settings;

		settings = g_settings_new (GTHUMB_BROWSER_SCHEMA);

		switch (event) {
		case GTH_MONITOR_EVENT_CREATED:
		case GTH_MONITOR_EVENT_CHANGED:
			monitor_data = monitor_event_data_new ();
			monitor_data->parent = g_file_dup (parent);
			monitor_data->event = event;
			monitor_data->source_tree = source_tree;
			gth_file_source_read_attributes (source_tree->priv->file_source,
						 	 list,
						 	 g_settings_get_boolean (settings, PREF_BROWSER_FAST_FILE_TYPE) ? GFILE_STANDARD_ATTRIBUTES_WITH_FAST_CONTENT_TYPE : GFILE_STANDARD_ATTRIBUTES_WITH_CONTENT_TYPE,
						 	 file_attributes_ready_cb,
						 	 monitor_data);
			break;

		case GTH_MONITOR_EVENT_DELETED:
		case GTH_MONITOR_EVENT_REMOVED:
			gth_folder_tree_delete_children (GTH_FOLDER_TREE (source_tree), parent, list);
			break;
		}

		g_object_unref (settings);
	}

	gtk_tree_path_free (path);
}


static void
monitor_file_renamed_cb (GthMonitor    *monitor,
			 GFile         *file,
			 GFile         *new_file,
			 GthSourceTree *source_tree)
{
	GFileInfo   *info;
	GthFileData *file_data;

	info = gth_file_source_get_file_info (source_tree->priv->file_source, new_file, GFILE_BASIC_ATTRIBUTES);
	file_data = gth_file_data_new (new_file, info);
	gth_folder_tree_update_child (GTH_FOLDER_TREE (source_tree), file, file_data);

	g_object_unref (file_data);
	g_object_unref (info);
}


static void
gth_source_tree_init (GthSourceTree *source_tree)
{
	source_tree->priv = g_new0 (GthSourceTreePrivate, 1);

	g_signal_connect (source_tree,
			  "list_children",
			  G_CALLBACK (source_tree_list_children_cb),
			  source_tree);
	g_signal_connect (source_tree,
			  "rename",
			  G_CALLBACK (source_tree_rename_cb),
			  source_tree);
	source_tree->priv->monitor_folder_changed_id =
		g_signal_connect (gth_main_get_default_monitor (),
				  "folder-changed",
				  G_CALLBACK (monitor_folder_changed_cb),
				  source_tree);
	source_tree->priv->monitor_file_renamed_id =
		g_signal_connect (gth_main_get_default_monitor (),
				  "file-renamed",
				  G_CALLBACK (monitor_file_renamed_cb),
				  source_tree);
}


static void
gth_source_tree_finalize (GObject *object)
{
	GthSourceTree *source_tree = GTH_SOURCE_TREE (object);

	if (source_tree->priv != NULL) {
		g_signal_handler_disconnect (gth_main_get_default_monitor (), source_tree->priv->monitor_folder_changed_id);
		g_signal_handler_disconnect (gth_main_get_default_monitor (), source_tree->priv->monitor_file_renamed_id);

		if (source_tree->priv->file_source != NULL)
			g_object_unref (source_tree->priv->file_source);
		g_free (source_tree->priv);
		source_tree->priv = NULL;
	}

	G_OBJECT_CLASS (gth_source_tree_parent_class)->finalize (object);
}


static void
gth_source_tree_class_init (GthSourceTreeClass *klass)
{
	GObjectClass *gobject_class;

	gobject_class = G_OBJECT_CLASS (klass);
	gobject_class->finalize = gth_source_tree_finalize;
}


GtkWidget *
gth_source_tree_new (GFile *root)
{
	GtkWidget *source_tree;

	source_tree = g_object_new (GTH_TYPE_SOURCE_TREE, NULL);
	gth_source_tree_set_root (GTH_SOURCE_TREE (source_tree), root);

	return source_tree;
}


static void
source_tree_file_list_ready (GthFileSource *file_source,
		 	     GList         *files,
			     GError        *error,
			     gpointer       data)
{
	LoadData      *load_data = data;
	GthSourceTree *source_tree = load_data->source_tree;

	if (error != NULL) {
		g_warning ("%s\n", error->message);
		load_data_free (load_data);
		return;
	}

	gth_folder_tree_set_list (GTH_FOLDER_TREE (source_tree), load_data->folder, files, FALSE);

	load_data_free (load_data);
}


void
gth_source_tree_set_root (GthSourceTree *source_tree,
			  GFile         *root)
{
	LoadData *load_data;

	if (source_tree->priv->file_source != NULL)
		g_object_unref (source_tree->priv->file_source);
	source_tree->priv->file_source = gth_main_get_file_source (root);

	load_data = load_data_new (source_tree, root);
	load_data_run (load_data, source_tree_file_list_ready);
}
