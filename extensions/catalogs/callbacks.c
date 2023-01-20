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
#include <glib-object.h>
#include <gthumb.h>
#include <gth-catalog.h>
#include "dlg-catalog-properties.h"
#include "dlg-organize-files.h"
#include "gth-file-source-catalogs.h"
#include "actions.h"
#include "callbacks.h"


#define BROWSER_DATA_KEY "catalogs-browser-data"
#define UPDATE_RENAMED_FILES_DELAY 500


static const GActionEntry actions[] = {
	{ "add-to-catalog", gth_browser_activate_add_to_catalog },
	{ "go-to-container-from-catalog", gth_browser_activate_go_to_container_from_catalog },
	{ "remove-from-catalog", gth_browser_activate_remove_from_catalog },
	{ "create-catalog", gth_browser_activate_create_catalog },
	{ "create-library", gth_browser_activate_create_library },
	{ "remove-catalog", gth_browser_activate_remove_catalog },
	{ "rename-catalog", gth_browser_activate_rename_catalog },
	{ "catalog-properties", gth_browser_activate_catalog_properties },
};


static const GthMenuEntry fixed_menu_entries[] = {
	{ N_("Add to Catalog…"), "win.add-to-catalog" },
};


static const GthMenuEntry vfs_open_actions_entries[] = {
	{ N_("Open Folder"), "win.go-to-container-from-catalog" },
};


static const GthMenuEntry vfs_other_actions_entries[] = {
	{ N_("Remove from Catalog"), "win.remove-from-catalog" },
};


static const GthMenuEntry folder_popup_create_entries[] = {
	{ N_("Create Catalog"), "win.create-catalog" },
	{ N_("Create Library"), "win.create-library" }
};

static const GthMenuEntry folder_popup_edit_entries[] = {
	{ N_("Remove"), "win.remove-catalog" },
	{ N_("Rename"), "win.rename-catalog" }
};


static const GthMenuEntry folder_popup_other_entries[] = {
	{ N_("Properties"), "win.catalog-properties" }
};



typedef struct {
	GthBrowser     *browser;
	guint           folder_popup_create_merge_id;
	guint           folder_popup_edit_merge_id;
	guint           folder_popup_other_merge_id;
	guint           vfs_open_actions_merge_id;
	guint           vfs_other_actions_merge_id;
	gboolean        catalog_menu_loaded;
	guint           n_top_catalogs;
	guint           monitor_events;
	GtkWidget      *properties_button;
	GtkWidget      *organize_button;
	guint           update_renamed_files_id;
	GList          *rename_data_list;
} BrowserData;


static void rename_data_list_free (BrowserData *data);


static void
browser_data_free (BrowserData *data)
{
	if (data->monitor_events != 0) {
		g_signal_handler_disconnect (gth_main_get_default_monitor (), data->monitor_events);
		data->monitor_events = 0;
	}
	if (data->update_renamed_files_id != 0) {
		g_source_remove (data->update_renamed_files_id);
		data->update_renamed_files_id = 0;
	}
	rename_data_list_free (data);

	g_free (data);
}


void
catalogs__initialize_cb (void)
{
	gth_user_dir_mkdir_with_parents (GTH_DIR_DATA, GTHUMB_DIR, "catalogs", NULL);
}


static void
monitor_folder_changed_cb (GthMonitor      *monitor,
			   GFile           *parent,
			   GList           *list,
			   int              position,
			   GthMonitorEvent  event,
			   gpointer         user_data)
{
	BrowserData *data = user_data;

	if (event == GTH_MONITOR_EVENT_CHANGED)
		return;
	data->catalog_menu_loaded = FALSE;
}


static void
catalogs_button_clicked_cb (GtkButton *button,
			    gpointer   user_data)
{
	GthBrowser *browser = user_data;
	GFile      *location;

	location = g_file_new_for_uri ("catalog:///");
	gth_browser_go_to (browser, location, NULL);

	g_object_unref (location);
}


void
catalogs__gth_browser_construct_cb (GthBrowser *browser)
{
	BrowserData *data;

	g_return_if_fail (GTH_IS_BROWSER (browser));

	data = g_new0 (BrowserData, 1);
	g_object_set_data_full (G_OBJECT (browser), BROWSER_DATA_KEY, data, (GDestroyNotify) browser_data_free);

	data->browser = browser;
	data->n_top_catalogs = 0;

	g_action_map_add_action_entries (G_ACTION_MAP (browser),
					 actions,
					 G_N_ELEMENTS (actions),
					 browser);
	gth_menu_manager_append_entries (gth_browser_get_menu_manager (browser, GTH_BROWSER_MENU_MANAGER_FILE_LIST_OTHER_ACTIONS),
					 fixed_menu_entries,
				         G_N_ELEMENTS (fixed_menu_entries));
	gth_menu_manager_append_entries (gth_browser_get_menu_manager (browser, GTH_BROWSER_MENU_MANAGER_FILE_OTHER_ACTIONS),
					 fixed_menu_entries,
				         G_N_ELEMENTS (fixed_menu_entries));

	{
		GtkWidget  *button;

		button = _gtk_image_button_new_for_header_bar ("file-library-symbolic");
		gtk_widget_set_tooltip_text (button, _("Catalogs"));
		gtk_widget_show (button);
		g_signal_connect (button, "clicked", G_CALLBACK (catalogs_button_clicked_cb), browser);
		gtk_box_pack_start (GTK_BOX (gth_browser_get_headerbar_section (browser, GTH_BROWSER_HEADER_SECTION_BROWSER_LOCATIONS)), button, FALSE, FALSE, 0);
	}

	data->monitor_events = g_signal_connect (gth_main_get_default_monitor (),
						 "folder-changed",
						 G_CALLBACK (monitor_folder_changed_cb),
						 data);
}


void
catalogs__gth_browser_selection_changed_cb (GthBrowser *browser,
					    int         n_selected)
{
	BrowserData *data;

	data = g_object_get_data (G_OBJECT (browser), BROWSER_DATA_KEY);
	g_return_if_fail (data != NULL);

	gth_window_enable_action (GTH_WINDOW (browser), "add-to-catalog", n_selected > 0);
	gth_window_enable_action (GTH_WINDOW (browser), "remove-from-catalog", (n_selected > 0) && GTH_IS_FILE_SOURCE_CATALOGS (gth_browser_get_location_source (browser)));
	gth_window_enable_action (GTH_WINDOW (browser), "go-to-container-from-catalog", n_selected == 1);
}


GFile *
catalogs__command_line_files_cb (GList *files)
{
	GFile      *file;
	GthCatalog *catalog;
	GList      *scan;

	if (g_list_length (files) <= 1)
		return NULL;

	file = _g_file_new_for_display_name ("catalog:///", _("Command Line"), ".catalog");
	catalog = gth_catalog_new ();
	gth_catalog_set_file (catalog, file);
	gth_catalog_set_name (catalog, _("Command Line"));
	for (scan = files; scan; scan = scan->next)
		gth_catalog_insert_file (catalog, (GFile *) scan->data, -1);
	gth_catalog_save (catalog);

	g_object_unref (catalog);

	return file;
}


GthCatalog *
catalogs__gth_catalog_load_from_data_cb (const void *buffer)
{
	if ((buffer == NULL)
	    || (strcmp (buffer, "") == 0)
	    || (strncmp (buffer, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<catalog ", 48) == 0))
	{
		return gth_catalog_new ();
	}
	else
		return NULL;
}


GthCatalog *
catalogs__gth_catalog_new_for_uri_cb (const char *uri)
{
	if (g_str_has_suffix (uri, ".catalog") || g_str_has_suffix (uri, ".gqv"))
		return gth_catalog_new ();
	else
		return NULL;
}


void
catalogs__gth_browser_folder_tree_popup_before_cb (GthBrowser    *browser,
						   GthFileSource *file_source,
					           GthFileData   *folder)
{
	BrowserData *data;

	data = g_object_get_data (G_OBJECT (browser), BROWSER_DATA_KEY);
	g_return_if_fail (data != NULL);

	if (GTH_IS_FILE_SOURCE_CATALOGS (file_source)) {
		gboolean sensitive;

		if (data->folder_popup_create_merge_id == 0)
			data->folder_popup_create_merge_id =
					gth_menu_manager_append_entries (gth_browser_get_menu_manager (browser, GTH_BROWSER_MENU_MANAGER_FOLDER_CREATE_ACTIONS),
									 folder_popup_create_entries,
									 G_N_ELEMENTS (folder_popup_create_entries));
		if (data->folder_popup_edit_merge_id == 0)
			data->folder_popup_edit_merge_id =
					gth_menu_manager_append_entries (gth_browser_get_menu_manager (browser, GTH_BROWSER_MENU_MANAGER_FOLDER_EDIT_ACTIONS),
									 folder_popup_edit_entries,
									 G_N_ELEMENTS (folder_popup_edit_entries));
		if (data->folder_popup_other_merge_id == 0)
			data->folder_popup_other_merge_id =
					gth_menu_manager_append_entries (gth_browser_get_menu_manager (browser, GTH_BROWSER_MENU_MANAGER_FOLDER_OTHER_ACTIONS),
									 folder_popup_other_entries,
									 G_N_ELEMENTS (folder_popup_other_entries));

		sensitive = (folder != NULL) && g_file_info_get_attribute_boolean (folder->info, G_FILE_ATTRIBUTE_ACCESS_CAN_DELETE);
		gth_window_enable_action (GTH_WINDOW (browser), "remove-catalog", sensitive);

		sensitive = ((folder != NULL)
			     && (_g_content_type_is_a (g_file_info_get_content_type (folder->info), "gthumb/library")
				 || _g_content_type_is_a (g_file_info_get_content_type (folder->info), "gthumb/catalog")
				 || _g_content_type_is_a (g_file_info_get_content_type (folder->info), "gthumb/search"))
			     && g_file_info_get_attribute_boolean (folder->info, G_FILE_ATTRIBUTE_ACCESS_CAN_RENAME));
		gth_window_enable_action (GTH_WINDOW (browser), "rename-catalog", sensitive);

		sensitive = (folder != NULL) && (! _g_content_type_is_a (g_file_info_get_content_type (folder->info), "gthumb/library"));
		gth_window_enable_action (GTH_WINDOW (browser), "catalog-properties", sensitive);
	}
	else {
		gth_menu_manager_remove_entries (gth_browser_get_menu_manager (browser, GTH_BROWSER_MENU_MANAGER_FOLDER_CREATE_ACTIONS), data->folder_popup_create_merge_id);
		data->folder_popup_create_merge_id = 0;

		gth_menu_manager_remove_entries (gth_browser_get_menu_manager (browser, GTH_BROWSER_MENU_MANAGER_FOLDER_EDIT_ACTIONS), data->folder_popup_edit_merge_id);
		data->folder_popup_edit_merge_id = 0;

		gth_menu_manager_remove_entries (gth_browser_get_menu_manager (browser, GTH_BROWSER_MENU_MANAGER_FOLDER_OTHER_ACTIONS), data->folder_popup_other_merge_id);
		data->folder_popup_other_merge_id = 0;
	}
}


static void
properties_button_clicked_cb (GtkButton  *button,
			      GthBrowser *browser)
{
	dlg_catalog_properties (browser, gth_browser_get_location_data (browser));
}


static void
organize_button_clicked_cb (GtkButton  *button,
			    GthBrowser *browser)
{
	dlg_organize_files (browser, gth_browser_get_location (browser));
}


void
catalogs__gth_browser_load_location_after_cb (GthBrowser   *browser,
					      GthFileData  *location_data)
{
	BrowserData *data;

	data = g_object_get_data (G_OBJECT (browser), BROWSER_DATA_KEY);

	if (GTH_IS_FILE_SOURCE_CATALOGS (gth_browser_get_location_source (browser))) {
		if (data->vfs_open_actions_merge_id == 0)
			data->vfs_open_actions_merge_id =
					gth_menu_manager_append_entries (gth_browser_get_menu_manager (browser, GTH_BROWSER_MENU_MANAGER_FILE_LIST_OPEN_ACTIONS),
									 vfs_open_actions_entries,
									 G_N_ELEMENTS (vfs_open_actions_entries));
		if (data->vfs_other_actions_merge_id == 0)
			data->vfs_other_actions_merge_id =
					gth_menu_manager_append_entries (gth_browser_get_menu_manager (browser, GTH_BROWSER_MENU_MANAGER_FILE_LIST_OTHER_ACTIONS),
									 vfs_other_actions_entries,
									 G_N_ELEMENTS (vfs_other_actions_entries));
	}
	else {
		gth_menu_manager_remove_entries (gth_browser_get_menu_manager (browser, GTH_BROWSER_MENU_MANAGER_FILE_LIST_OPEN_ACTIONS), data->vfs_open_actions_merge_id);
		data->vfs_open_actions_merge_id = 0;

		gth_menu_manager_remove_entries (gth_browser_get_menu_manager (browser, GTH_BROWSER_MENU_MANAGER_FILE_LIST_OTHER_ACTIONS), data->vfs_other_actions_merge_id);
		data->vfs_other_actions_merge_id = 0;
	}
}


void
catalogs__gth_browser_update_extra_widget_cb (GthBrowser *browser)
{
	BrowserData *data;
	GthFileData *location_data;

	data = g_object_get_data (G_OBJECT (browser), BROWSER_DATA_KEY);

	location_data = gth_browser_get_location_data (browser);
	if (GTH_IS_FILE_SOURCE_CATALOGS (gth_browser_get_location_source (browser))
	    && ! _g_content_type_is_a (g_file_info_get_content_type (location_data->info), "gthumb/library"))
	{
		if (data->properties_button == NULL) {
			data->properties_button = gtk_button_new ();
			gtk_container_add (GTK_CONTAINER (data->properties_button), gtk_image_new_from_icon_name ("document-properties-symbolic", GTK_ICON_SIZE_MENU));
			g_object_add_weak_pointer (G_OBJECT (data->properties_button), (gpointer *)&data->properties_button);
			gtk_button_set_relief (GTK_BUTTON (data->properties_button), GTK_RELIEF_NONE);
			gtk_widget_set_tooltip_text (data->properties_button, _("Catalog Properties"));
			gtk_widget_show_all (data->properties_button);
			gtk_box_pack_start (GTK_BOX (gth_location_bar_get_action_area (GTH_LOCATION_BAR (gth_browser_get_location_bar (browser)))),
					    data->properties_button,
					    FALSE,
					    FALSE,
					    0);
			g_signal_connect (data->properties_button,
					  "clicked",
					  G_CALLBACK (properties_button_clicked_cb),
					  browser);
		}
	}
	else if (GTH_IS_FILE_SOURCE_VFS (gth_browser_get_location_source (browser))) {
		if (data->organize_button == NULL) {
			data->organize_button = gtk_button_new ();
			gtk_container_add (GTK_CONTAINER (data->organize_button), gtk_label_new (_("Organize")));
			gtk_widget_set_tooltip_text (data->organize_button, _("Automatically organize files by date"));
			g_object_add_weak_pointer (G_OBJECT (data->organize_button), (gpointer *)&data->organize_button);
			gtk_button_set_relief (GTK_BUTTON (data->organize_button), GTK_RELIEF_NONE);
			gtk_widget_show_all (data->organize_button);
			gtk_box_pack_start (GTK_BOX (gth_location_bar_get_action_area (GTH_LOCATION_BAR (gth_browser_get_location_bar (browser)))),
					    data->organize_button,
					    FALSE,
					    FALSE,
					    0);
			g_signal_connect (data->organize_button,
					  "clicked",
					  G_CALLBACK (organize_button_clicked_cb),
					  browser);
		}
	}
}


/* -- catalogs__gth_browser_file_renamed_cb -- */


typedef struct {
	GFile *location;
	GList *files;
	GList *new_files;
} RenameData;


static RenameData *
rename_data_new (GFile *location)
{
	RenameData *rename_data;

	rename_data = g_new0 (RenameData, 1);
	rename_data->location = g_file_dup (location);

	return rename_data;
}


static void
rename_data_free (RenameData *rename_data)
{
	_g_object_list_unref (rename_data->files);
	_g_object_list_unref (rename_data->new_files);
	g_object_unref (rename_data->location);
	g_free (rename_data);
}


static void
rename_data_list_free (BrowserData *data)
{
	g_list_foreach (data->rename_data_list, (GFunc) rename_data_free, NULL);
	g_list_free (data->rename_data_list);
	data->rename_data_list = NULL;
}


static gboolean
process_rename_data_list (gpointer user_data)
{
	BrowserData *data = user_data;
	GList       *scan;

	g_source_remove (data->update_renamed_files_id);
	data->update_renamed_files_id = 0;

	for (scan = data->rename_data_list; scan; scan = scan->next) {
		RenameData *rename_data = scan->data;
		GthCatalog *catalog;
		GList      *scan_files;
		GList      *scan_new_files;
		GFile      *gio_file;
		char       *catalog_data;
		gsize       catalog_data_size;
		GError     *error = NULL;

		catalog = gth_catalog_load_from_file (rename_data->location);
		if (catalog == NULL)
			continue;

		for (scan_files = rename_data->files, scan_new_files = rename_data->new_files;
		     scan_files && scan_new_files;
		     scan_files = scan_files->next, scan_new_files = scan_new_files->next)
		{
			GFile *file = scan_files->data;
			GFile *new_file = scan_new_files->data;
			int    pos;

			pos = gth_catalog_remove_file (catalog, file);
			gth_catalog_insert_file (catalog, new_file, pos);
		}

		gio_file = gth_catalog_file_to_gio_file (rename_data->location);
		catalog_data = gth_catalog_to_data (catalog, &catalog_data_size);
		if (! _g_file_write (gio_file,
				     FALSE,
				     G_FILE_CREATE_NONE,
				     catalog_data,
				     catalog_data_size,
				     NULL,
				     &error))
		{
			g_warning ("%s", error->message);
			g_clear_error (&error);
		}

		g_free (catalog_data);
		g_object_unref (gio_file);
		g_object_unref (catalog);
	}

	rename_data_list_free (data);

	return FALSE;
}


void
catalogs__gth_browser_file_renamed_cb (GthBrowser *browser,
				       GFile      *file,
				       GFile      *new_file)
{
	GthFileStore *file_store;
	BrowserData  *data;
	GFile        *location;
	GList        *scan;
	RenameData   *rename_data;

	if (! GTH_IS_FILE_SOURCE_CATALOGS (gth_browser_get_location_source (browser)))
		return;

	file_store = gth_browser_get_file_store (browser);
	if (! gth_file_store_find (file_store, file, NULL))
		return;

	location = gth_browser_get_location (browser);
	if (location == NULL)
		return;

	data = g_object_get_data (G_OBJECT (browser), BROWSER_DATA_KEY);

	rename_data = NULL;
	for (scan = data->rename_data_list; scan; scan = scan->next) {
		RenameData *rename_data_scan = scan->data;
		if (g_file_equal (rename_data_scan->location, location)) {
			rename_data = rename_data_scan;
			break;
		}
	}

	if (rename_data == NULL) {
		rename_data = rename_data_new (location);
		data->rename_data_list = g_list_prepend (data->rename_data_list, rename_data);
	}

	rename_data->files = g_list_prepend (rename_data->files, g_file_dup (file));
	rename_data->new_files = g_list_prepend (rename_data->new_files, g_file_dup (new_file));

	if (data->update_renamed_files_id != 0)
		g_source_remove (data->update_renamed_files_id);
	data->update_renamed_files_id = g_timeout_add (UPDATE_RENAMED_FILES_DELAY,
						       process_rename_data_list,
						       data);
}
