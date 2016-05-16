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


#define BROWSER_DATA_KEY "catalogs-browser-data"
#define _RESPONSE_PROPERTIES 1
#define _RESPONSE_ORGANIZE 2
#define UPDATE_RENAMED_FILES_DELAY 500


static const char *fixed_ui_info =
"<ui>"
"  <popup name='FileListPopup'>"
"    <placeholder name='Folder_Actions2'>"
"      <menuitem action='Edit_AddToCatalog'/>"
"      <menu action='Edit_QuickAddToCatalog'>"
"        <separator name='CatalogListSeparator'/>"
"        <menuitem action='Edit_QuickAddToCatalogOther'/>"
"      </menu>"
"    </placeholder>"
"  </popup>"
"  <popup name='FilePopup'>"
"    <placeholder name='Folder_Actions2'>"
"      <menuitem action='Edit_AddToCatalog'/>"
"      <menu action='Edit_QuickAddToCatalog'>"
"        <separator name='CatalogListSeparator'/>"
"        <menuitem action='Edit_QuickAddToCatalogOther'/>"
"      </menu>"
"    </placeholder>"
"  </popup>"
"</ui>";


static const char *vfs_ui_info =
"<ui>"
"  <popup name='FileListPopup'>"
"    <placeholder name='Open_Actions'>"
"      <menuitem action='Go_FileContainer'/>"
"    </placeholder>"
"    <placeholder name='Folder_Actions2'>"
"      <menuitem action='Edit_RemoveFromCatalog'/>"
"    </placeholder>"
"  </popup>"
"  <popup name='FilePopup'>"
"    <placeholder name='Open_Actions'>"
"      <menuitem action='Go_FileContainer'/>"
"    </placeholder>"
"    <placeholder name='Folder_Actions2'>"
"      <menuitem action='Edit_RemoveFromCatalog'/>"
"    </placeholder>"
"  </popup>"
"</ui>";



static const gchar *folder_popup_ui_info =
"<ui>"
"  <popup name='FolderListPopup'>"
"    <placeholder name='SourceCommands'>"
"      <menuitem action='Catalog_New'/>"
"      <menuitem action='Catalog_New_Library'/>"
"      <separator/>"
"      <menuitem action='Catalog_Remove'/>"
"      <menuitem action='Catalog_Rename'/>"
"      <separator/>"
"      <menuitem action='Catalog_Properties'/>"
"    </placeholder>"
"  </popup>"
"</ui>";


static GtkActionEntry catalog_action_entries[] = {
	{ "Edit_QuickAddToCatalog", GTK_STOCK_ADD, N_("_Add to Catalog") },

        { "Go_FileContainer", GTK_STOCK_JUMP_TO,
          N_("Open _Folder"), "<alt>End",
          N_("Go to the folder that contains the selected file"),
          G_CALLBACK (gth_browser_activate_action_go_to_container) },

        { "Edit_QuickAddToCatalogOther", NULL,
	  N_("Other..."), NULL,
	  N_("Choose another catalog"),
	  G_CALLBACK (gth_browser_activate_action_edit_add_to_catalog) },

	{ "Edit_AddToCatalog", GTK_STOCK_ADD,
	  N_("_Add to Catalog..."), NULL,
	  N_("Add selected images to a catalog"),
	  G_CALLBACK (gth_browser_activate_action_edit_add_to_catalog) },

	{ "Edit_RemoveFromCatalog", GTK_STOCK_REMOVE,
	  N_("Remo_ve from Catalog"), NULL,
	  N_("Remove selected images from the catalog"),
	  G_CALLBACK (gth_browser_activate_action_edit_remove_from_catalog) },

	{ "Catalog_New", NULL,
	  N_("Create _Catalog"), NULL,
	  NULL,
	  G_CALLBACK (gth_browser_activate_action_catalog_new) },

	{ "Catalog_New_Library", NULL,
	  N_("Create _Library"), NULL,
	  NULL,
	  G_CALLBACK (gth_browser_activate_action_catalog_new_library) },

	{ "Catalog_Remove", GTK_STOCK_REMOVE,
	  NULL, NULL,
	  NULL,
	  G_CALLBACK (gth_browser_activate_action_catalog_remove) },

	{ "Catalog_Rename", NULL,
	  N_("Rena_me"), NULL,
	  NULL,
	  G_CALLBACK (gth_browser_activate_action_catalog_rename) },

	{ "Catalog_Properties", GTK_STOCK_PROPERTIES,
	  NULL, NULL,
	  NULL,
	  G_CALLBACK (gth_browser_activate_action_catalog_properties) }
};
static guint catalog_action_entries_size = G_N_ELEMENTS (catalog_action_entries);


typedef struct {
	GthBrowser     *browser;
	GtkActionGroup *actions;
	guint           folder_popup_merge_id;
	guint           vfs_merge_id;
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


void
catalogs__gth_browser_construct_cb (GthBrowser *browser)
{
	BrowserData *data;
	GError      *error = NULL;

	g_return_if_fail (GTH_IS_BROWSER (browser));

	data = g_new0 (BrowserData, 1);
	data->browser = browser;
	data->n_top_catalogs = 0;

	data->actions = gtk_action_group_new ("Catalog Actions");
	gtk_action_group_set_translation_domain (data->actions, NULL);
	gtk_action_group_add_actions (data->actions,
				      catalog_action_entries,
				      catalog_action_entries_size,
				      browser);
	gtk_ui_manager_insert_action_group (gth_browser_get_ui_manager (browser), data->actions, 0);

	if (! gtk_ui_manager_add_ui_from_string (gth_browser_get_ui_manager (browser), fixed_ui_info, -1, &error)) {
		g_message ("building menus failed: %s", error->message);
		g_error_free (error);
	}

	data->monitor_events = g_signal_connect (gth_main_get_default_monitor (),
						 "folder-changed",
						 G_CALLBACK (monitor_folder_changed_cb),
						 data);

	g_object_set_data_full (G_OBJECT (browser), BROWSER_DATA_KEY, data, (GDestroyNotify) browser_data_free);
}


void
catalogs__gth_browser_update_sensitivity_cb (GthBrowser *browser)
{
	BrowserData *data;
	GtkAction   *action;
	int          n_selected;
	gboolean     sensitive;

	data = g_object_get_data (G_OBJECT (browser), BROWSER_DATA_KEY);
	g_return_if_fail (data != NULL);

	n_selected = gth_file_selection_get_n_selected (GTH_FILE_SELECTION (gth_browser_get_file_list_view (browser)));

	action = gtk_action_group_get_action (data->actions, "Edit_AddToCatalog");
	sensitive = n_selected > 0;
	g_object_set (action, "sensitive", sensitive, NULL);

	action = gtk_action_group_get_action (data->actions, "Edit_RemoveFromCatalog");
	sensitive = (n_selected > 0) && GTH_IS_FILE_SOURCE_CATALOGS (gth_browser_get_location_source (browser));
	g_object_set (action, "sensitive", sensitive, NULL);

	action = gtk_action_group_get_action (data->actions, "Go_FileContainer");
	sensitive = (n_selected == 1);
	g_object_set (action, "sensitive", sensitive, NULL);
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


/* -- update_catalog_menu -- */


typedef struct _CatalogListData CatalogListData;


struct _CatalogListData {
	CatalogListData *parent;
	BrowserData     *data;
	GthFileSource   *file_source;
	GFile           *root;
	GtkWidget       *list_menu;
	GtkWidget       *file_menu;
	GList           *children;
	GList           *current_child;
};


static void
catalog_list_data_free (CatalogListData *list_data)
{
	g_list_free (list_data->children);
	g_object_unref (list_data->root);
	g_object_unref (list_data->file_source);
	g_free (list_data);
}


static void catalog_list_load_current_child (CatalogListData *list_data);


static void
catalog_list_load_next_child (CatalogListData *list_data)
{
	if (list_data == NULL)
		return;
	list_data->current_child = list_data->current_child->next;
	catalog_list_load_current_child (list_data);
}


static void load_catalog_list (CatalogListData *list_data);


static void
catalog_list_load_current_child (CatalogListData *list_data)
{
	if (list_data->current_child == NULL) {
		catalog_list_load_next_child (list_data->parent);
		catalog_list_data_free (list_data);
		return;
	}

	load_catalog_list ((CatalogListData *) list_data->current_child->data);
}


static int
sort_catalogs (gconstpointer a,
               gconstpointer b)
{
	GthFileData *file_data_a = (GthFileData *) a;
	GthFileData *file_data_b = (GthFileData *) b;

	if (g_file_info_get_attribute_boolean (file_data_a->info, "gthumb::no-child") != g_file_info_get_attribute_boolean (file_data_b->info, "gthumb::no-child")) {
		/* put the libraries before the catalogs */
		return g_file_info_get_attribute_boolean (file_data_a->info, "gthumb::no-child") ? 1 : -1;
	}
	else if (g_file_info_get_sort_order (file_data_a->info) == g_file_info_get_sort_order (file_data_b->info))
		return g_utf8_collate (g_file_info_get_display_name (file_data_a->info),
				       g_file_info_get_display_name (file_data_b->info));
	else if (g_file_info_get_sort_order (file_data_a->info) < g_file_info_get_sort_order (file_data_b->info))
		return -1;
	else
		return 1;
}


static void
catalog_item_activate_cb (GtkMenuItem *menuitem,
			  gpointer     user_data)
{
	GthBrowser *browser = user_data;
	char       *uri;
	GFile      *file;

	if (gtk_menu_item_get_submenu (menuitem) != NULL)
		return;

	uri = g_object_get_data (G_OBJECT (menuitem), "uri");
	file = g_file_new_for_uri (uri);
	gth_browser_add_to_catalog (browser, file);

	g_object_unref (file);
}


static GtkWidget *
insert_menu_item (CatalogListData *list_data,
		  GtkWidget       *menu,
		  GthFileData     *file_data,
		  int              pos)
{
	GtkWidget *item;
	GtkWidget *image;

	item = gtk_image_menu_item_new_with_label (g_file_info_get_display_name (file_data->info));
	gtk_image_menu_item_set_always_show_image (GTK_IMAGE_MENU_ITEM (item), TRUE);

	image = gtk_image_new_from_gicon (g_file_info_get_icon (file_data->info), GTK_ICON_SIZE_MENU);
	gtk_widget_show (image);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), image);
	gtk_widget_show (item);
	gtk_menu_shell_insert (GTK_MENU_SHELL (menu), item, pos);
	g_object_set_data_full (G_OBJECT (item), "uri", g_file_get_uri (file_data->file), g_free);
	g_signal_connect (item, "activate", G_CALLBACK (catalog_item_activate_cb), list_data->data->browser);

	return item;
}


static void
update_commands_visibility (BrowserData *data)
{
	GtkAction *action;

	action = gtk_action_group_get_action (data->actions, "Edit_QuickAddToCatalog");
	gtk_action_set_visible (action, (data->n_top_catalogs > 0));

	action = gtk_action_group_get_action (data->actions, "Edit_AddToCatalog");
	gtk_action_set_visible (action, (data->n_top_catalogs == 0));
}



static void
catalog_list_ready (GthFileSource *file_source,
		    GList         *files,
		    GError        *error,
		    gpointer       user_data)
{
	CatalogListData *list_data = user_data;
	GList           *ordered;
	int              pos;
	GList           *scan;
	GFile           *root;

	ordered = g_list_sort (gth_file_data_list_dup (files), sort_catalogs);
	pos = 0;
	for (scan = ordered; scan; scan = scan->next) {
		GthFileData *file_data = scan->data;
		GtkWidget   *list_item;
		GtkWidget   *file_item;

		if (g_file_info_get_is_hidden (file_data->info))
			continue;

		list_item = insert_menu_item (list_data, list_data->list_menu, file_data, pos);
		file_item = insert_menu_item (list_data, list_data->file_menu, file_data, pos);

		if (! g_file_info_get_attribute_boolean (file_data->info, "gthumb::no-child")) {
			CatalogListData *child;

			child = g_new0 (CatalogListData, 1);
			child->parent = list_data;
			child->data = list_data->data;
			child->file_source = g_object_ref (list_data->file_source);
			child->root = g_file_dup (file_data->file);
			child->list_menu = gtk_menu_new ();
			child->file_menu = gtk_menu_new ();
			list_data->children = g_list_prepend (list_data->children, child);

			gtk_menu_item_set_submenu (GTK_MENU_ITEM (list_item), child->list_menu);
			gtk_menu_item_set_submenu (GTK_MENU_ITEM (file_item), child->file_menu);
		}

		pos++;
	}

	root = g_file_new_for_uri ("catalog:///");
	if (g_file_equal (list_data->root, root)) {
		list_data->data->n_top_catalogs = g_list_length (ordered);
		update_commands_visibility (list_data->data);
	}
	else if (ordered == NULL) {
		GtkWidget *item;

		item = gtk_menu_item_new_with_label (_("(Empty)"));
		gtk_widget_show (item);
		gtk_widget_set_sensitive (item, FALSE);
		gtk_menu_shell_insert (GTK_MENU_SHELL (list_data->list_menu), item, pos);

		item = gtk_menu_item_new_with_label (_("(Empty)"));
		gtk_widget_show (item);
		gtk_widget_set_sensitive (item, FALSE);
		gtk_menu_shell_insert (GTK_MENU_SHELL (list_data->file_menu), item, pos);
	}

	g_object_unref (root);
	_g_object_list_unref (ordered);

	list_data->children = g_list_reverse (list_data->children);
	list_data->current_child = list_data->children;
	catalog_list_load_current_child (list_data);
}


static void
load_catalog_list (CatalogListData *list_data)
{
	gth_file_source_list (list_data->file_source,
			      list_data->root,
			      GFILE_STANDARD_ATTRIBUTES,
			      catalog_list_ready,
			      list_data);
}


static void
update_catalog_menu (BrowserData *data)
{
	CatalogListData *list_data;
	GtkWidget       *list_menu;
	GtkWidget       *file_menu;
	GtkWidget       *separator;

	list_menu = gtk_menu_item_get_submenu (GTK_MENU_ITEM (gtk_ui_manager_get_widget (gth_browser_get_ui_manager (data->browser), "/FileListPopup/Folder_Actions2/Edit_QuickAddToCatalog")));
	separator = gtk_ui_manager_get_widget (gth_browser_get_ui_manager (data->browser), "/FileListPopup/Folder_Actions2/Edit_QuickAddToCatalog/CatalogListSeparator");
	_gtk_container_remove_children (GTK_CONTAINER (list_menu), NULL, separator);

	file_menu = gtk_menu_item_get_submenu (GTK_MENU_ITEM (gtk_ui_manager_get_widget (gth_browser_get_ui_manager (data->browser), "/FilePopup/Folder_Actions2/Edit_QuickAddToCatalog")));
	separator = gtk_ui_manager_get_widget (gth_browser_get_ui_manager (data->browser), "/FilePopup/Folder_Actions2/Edit_QuickAddToCatalog/CatalogListSeparator");
	_gtk_container_remove_children (GTK_CONTAINER (file_menu), NULL, separator);

	list_data = g_new0 (CatalogListData, 1);
	list_data->data = data;
	list_data->file_source = g_object_new (GTH_TYPE_FILE_SOURCE_CATALOGS, NULL);
	list_data->root = g_file_new_for_uri ("catalog:///");
	list_data->list_menu = list_menu;
	list_data->file_menu = file_menu;

	load_catalog_list (list_data);
}


void
catalogs__gth_browser_file_list_popup_before_cb (GthBrowser *browser)
{
	BrowserData *data;

	data = g_object_get_data (G_OBJECT (browser), BROWSER_DATA_KEY);
	g_return_if_fail (data != NULL);

	if (! data->catalog_menu_loaded) {
		data->catalog_menu_loaded = TRUE;
		update_catalog_menu (data);
	}
	else
		update_commands_visibility (data);
}


void
catalogs__gth_browser_file_popup_before_cb (GthBrowser *browser)
{
	BrowserData *data;

	data = g_object_get_data (G_OBJECT (browser), BROWSER_DATA_KEY);
	g_return_if_fail (data != NULL);

	if (! data->catalog_menu_loaded) {
		data->catalog_menu_loaded = TRUE;
		update_catalog_menu (data);
	}
	else
		update_commands_visibility (data);
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
		GtkAction *action;
		gboolean   sensitive;

		if (data->folder_popup_merge_id == 0) {
			GError *error = NULL;

			data->folder_popup_merge_id = gtk_ui_manager_add_ui_from_string (gth_browser_get_ui_manager (browser), folder_popup_ui_info, -1, &error);
			if (data->folder_popup_merge_id == 0) {
				g_message ("building menus failed: %s", error->message);
				g_error_free (error);
			}
		}

		action = gtk_action_group_get_action (data->actions, "Catalog_Remove");
		sensitive = (folder != NULL) && g_file_info_get_attribute_boolean (folder->info, G_FILE_ATTRIBUTE_ACCESS_CAN_DELETE);
		g_object_set (action, "sensitive", sensitive, NULL);

		action = gtk_action_group_get_action (data->actions, "Catalog_Rename");
		sensitive = ((folder != NULL)
			     && (_g_content_type_is_a (g_file_info_get_content_type (folder->info), "gthumb/library") || _g_content_type_is_a (g_file_info_get_content_type (folder->info), "gthumb/catalog"))
			     && g_file_info_get_attribute_boolean (folder->info, G_FILE_ATTRIBUTE_ACCESS_CAN_RENAME));
		g_object_set (action, "sensitive", sensitive, NULL);

		action = gtk_action_group_get_action (data->actions, "Catalog_Properties");
		sensitive = (folder != NULL) && (! _g_content_type_is_a (g_file_info_get_content_type (folder->info), "gthumb/library"));
		g_object_set (action, "sensitive", sensitive, NULL);
	}
	else {
		if (data->folder_popup_merge_id != 0) {
			gtk_ui_manager_remove_ui (gth_browser_get_ui_manager (browser), data->folder_popup_merge_id);
			data->folder_popup_merge_id = 0;
		}
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
					      GthFileData  *location_data,
					      const GError *error)
{
	BrowserData *data;

	if ((location_data == NULL) || (error != NULL))
		return;

	data = g_object_get_data (G_OBJECT (browser), BROWSER_DATA_KEY);

	if (GTH_IS_FILE_SOURCE_CATALOGS (gth_browser_get_location_source (browser))) {
		if (data->vfs_merge_id == 0) {
			GError *error = NULL;

			data->vfs_merge_id = gtk_ui_manager_add_ui_from_string (gth_browser_get_ui_manager (browser), vfs_ui_info, -1, &error);
			if (data->vfs_merge_id == 0) {
				g_message ("building menus failed: %s", error->message);
				g_error_free (error);
			}
		}
	}
	else {
		if (data->vfs_merge_id != 0) {
			gtk_ui_manager_remove_ui (gth_browser_get_ui_manager (browser), data->vfs_merge_id);
			data->vfs_merge_id = 0;
		}
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
			gtk_container_add (GTK_CONTAINER (data->properties_button), gtk_image_new_from_stock (GTK_STOCK_PROPERTIES, GTK_ICON_SIZE_MENU));
			g_object_add_weak_pointer (G_OBJECT (data->properties_button), (gpointer *)&data->properties_button);
			gtk_button_set_relief (GTK_BUTTON (data->properties_button), GTK_RELIEF_NONE);
			gtk_widget_set_tooltip_text (data->properties_button, _("Catalog Properties"));
			gtk_widget_show_all (data->properties_button);
			gedit_message_area_add_action_widget (GEDIT_MESSAGE_AREA (gth_browser_get_list_extra_widget (browser)),
							      data->properties_button,
							      _RESPONSE_PROPERTIES);
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
			gedit_message_area_add_action_widget (GEDIT_MESSAGE_AREA (gth_browser_get_list_extra_widget (browser)),
							      data->organize_button,
							      _RESPONSE_ORGANIZE);
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
