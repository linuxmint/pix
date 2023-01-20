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
#include <extensions/catalogs/gth-catalog.h>
#include "actions.h"
#include "callbacks.h"
#include "gth-search.h"
#include "gth-search-editor.h"
#include "gth-search-task.h"


#define BROWSER_DATA_KEY "search-browser-data"


static const GActionEntry actions[] = {
	{ "find", gth_browser_activate_find }
};


static const GthShortcut shortcuts[] = {
	{ "find", N_("Find files"), GTH_SHORTCUT_CONTEXT_BROWSER_VIEWER, GTH_SHORTCUT_CATEGORY_FILE_MANAGER, "<Primary>f" },
};


typedef struct {
	GtkWidget *refresh_button;
} BrowserData;


static void
browser_data_free (BrowserData *data)
{
	g_free (data);
}


void
search__gth_browser_construct_cb (GthBrowser *browser)
{
	BrowserData *data;

	g_return_if_fail (GTH_IS_BROWSER (browser));

	g_action_map_add_action_entries (G_ACTION_MAP (browser),
					 actions,
					 G_N_ELEMENTS (actions),
					 browser);

	gth_window_add_shortcuts (GTH_WINDOW (browser),
				  shortcuts,
				  G_N_ELEMENTS (shortcuts));

	gth_browser_add_header_bar_button (browser,
					   GTH_BROWSER_HEADER_SECTION_BROWSER_COMMANDS,
					   "edit-find-symbolic",
					   _("Find files"),
					   "win.find",
					   NULL);

	data = g_new0 (BrowserData, 1);
	g_object_set_data_full (G_OBJECT (browser), BROWSER_DATA_KEY, data, (GDestroyNotify) browser_data_free);
}


static void
refresh_button_clicked_cb (GtkButton  *button,
			   GthBrowser *browser)
{
	gth_browser_update_search (browser);
}


void
search__gth_browser_update_extra_widget_cb (GthBrowser *browser)
{
	GthFileData *location_data;
	BrowserData *data;

	location_data = gth_browser_get_location_data (browser);
	if (! _g_content_type_is_a (g_file_info_get_content_type (location_data->info), "gthumb/search"))
		return;

	data = g_object_get_data (G_OBJECT (browser), BROWSER_DATA_KEY);

	if (data->refresh_button == NULL) {
		data->refresh_button = gtk_button_new ();
		gtk_container_add (GTK_CONTAINER (data->refresh_button), gtk_image_new_from_icon_name ("view-refresh-symbolic", GTK_ICON_SIZE_MENU));
		g_object_add_weak_pointer (G_OBJECT (data->refresh_button), (gpointer *)&data->refresh_button);
		gtk_button_set_relief (GTK_BUTTON (data->refresh_button), GTK_RELIEF_NONE);
		gtk_widget_set_tooltip_text (data->refresh_button, _("Search again"));
		gtk_widget_show_all (data->refresh_button);
		gtk_box_pack_start (GTK_BOX (gth_location_bar_get_action_area (GTH_LOCATION_BAR (gth_browser_get_location_bar (browser)))),
				    data->refresh_button,
				    FALSE,
				    FALSE,
				    0);
		g_signal_connect (data->refresh_button,
				  "clicked",
				  G_CALLBACK (refresh_button_clicked_cb),
				  browser);
	}
}


void
search__gth_browser_load_location_before_cb (GthBrowser *browser,
					     GFile      *next_location)
{
	GFile   *current_location;
	GthTask *task;
	GFile   *catalog;

	/* Stop the search task if the user changes location. */

	current_location = gth_browser_get_location (browser);
	if (current_location == NULL)
		return;

	if (_g_file_equal (current_location, next_location))
		return;

	task = gth_browser_get_foreground_task (browser);
	if ((task == NULL) || ! GTH_IS_SEARCH_TASK (task))
		return;

	catalog = gth_search_task_get_catalog (GTH_SEARCH_TASK (task));

	if (_g_file_equal (current_location, catalog))
		gth_task_cancel (task);
}


GthCatalog *
search__gth_catalog_load_from_data_cb (const void *buffer)
{
	if ((buffer != NULL) && (strncmp (buffer, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<search ", 47) == 0))
		return (GthCatalog *) gth_search_new ();
	else
		return NULL;
}


GthCatalog *
search__gth_catalog_new_for_uri_cb (const char *uri)
{
	if (g_str_has_suffix (uri, ".search"))
		return (GthCatalog *) gth_search_new ();
	else
		return NULL;
}


void
search__dlg_catalog_properties (GtkBuilder  *builder,
				GthFileData *file_data,
				GthCatalog  *catalog)
{
	GtkWidget     *vbox;
	GtkWidget     *label;
	PangoAttrList *attrs;
	GtkWidget     *search_editor;

	if (! _g_content_type_is_a (g_file_info_get_content_type (file_data->info), "gthumb/search"))
		return;

	if (! GTH_IS_SEARCH (catalog))
		return;

	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
	gtk_widget_show (vbox);
	gtk_box_pack_start (GTK_BOX (_gtk_builder_get_widget (builder, "general_page")), vbox, FALSE, FALSE, 0);

	/* Translators: This is not a verb, it's a name as in "the search properties". */
	label = gtk_label_new (_("Search"));
	gtk_widget_set_halign (label, GTK_ALIGN_START);
	gtk_widget_set_valign (label, GTK_ALIGN_CENTER);

	attrs = pango_attr_list_new ();
	pango_attr_list_insert (attrs, pango_attr_weight_new (PANGO_WEIGHT_BOLD));
	gtk_label_set_attributes (GTK_LABEL (label), attrs);
	pango_attr_list_unref (attrs);
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

	search_editor = gth_search_editor_new (GTH_SEARCH (catalog));
	gtk_widget_set_margin_start (search_editor, 12);
	gtk_widget_show (search_editor);
	gtk_box_pack_start (GTK_BOX (vbox), search_editor, FALSE, FALSE, 0);
	g_object_set_data (G_OBJECT (builder), "search_editor", search_editor);
}


void
search__dlg_catalog_properties_save (GtkBuilder  *builder,
				     GthFileData *file_data,
				     GthCatalog  *catalog)
{
	GthSearch *search;

	if (! _g_content_type_is_a (g_file_info_get_content_type (file_data->info), "gthumb/search"))
		return;

	g_return_if_fail (GTH_IS_SEARCH (catalog));

	search = gth_search_editor_get_search (GTH_SEARCH_EDITOR (g_object_get_data (G_OBJECT(builder), "search_editor")), NULL);
	if (search != NULL) {
		g_file_info_set_attribute_boolean (file_data->info, "gthumb::search-modified", ! gth_search_equal (GTH_SEARCH (catalog), search));
		gth_search_set_sources (GTH_SEARCH (catalog), gth_search_get_sources (search));
		gth_search_set_test (GTH_SEARCH (catalog), gth_search_get_test (search));
	}
}


void
search__dlg_catalog_properties_saved (GthBrowser  *browser,
				      GthFileData *file_data,
				      GthCatalog  *catalog)
{
	GthTask *task;

	if (! _g_content_type_is_a (g_file_info_get_content_type (file_data->info), "gthumb/search"))
		return;

	/* Search only if the search parameters changed */
	if (! g_file_info_get_attribute_boolean (file_data->info, "gthumb::search-modified"))
		return;

	task = gth_search_task_new (browser, GTH_SEARCH (catalog), file_data->file);
	gth_browser_exec_task (browser, task, GTH_TASK_FLAGS_FOREGROUND);

	g_object_unref (task);
}


void
search__gth_organize_task_create_catalog (GthGroupPolicyData *data)
{
	GthGroupPolicy policy;

	policy = gth_organize_task_get_group_policy (data->task);
	switch (policy) {
	case GTH_GROUP_POLICY_DIGITALIZED_DATE:
	case GTH_GROUP_POLICY_MODIFIED_DATE:

		/* delete the .catalog file to avoid to have two catalogs with
		 * the same name, which can be confusing for the user. */

		{
			GFile *catalog_file;
			GFile *gio_file;

			catalog_file = gth_catalog_get_file_for_date (data->date_time, ".catalog");
			gio_file = gth_catalog_file_to_gio_file (catalog_file);
			if (g_file_delete (gio_file, NULL, NULL)) {
				GFile *parent;
				GList *files;

				parent = g_file_get_parent (catalog_file);
				files = g_list_prepend (NULL, g_object_ref (catalog_file));
				gth_monitor_folder_changed (gth_main_get_default_monitor (),
							    parent,
							    files,
							    GTH_MONITOR_EVENT_DELETED);

				_g_object_list_unref (files);
				_g_object_unref (parent);
			}

			g_object_unref (gio_file);
			g_object_unref (catalog_file);
		}

		data->catalog_file = gth_catalog_get_file_for_date (data->date_time, ".search");
		data->catalog = gth_catalog_load_from_file (data->catalog_file);
		if (data->catalog == NULL) {
			GthTest *date_test;
			GthTest *test_chain;

			data->catalog = (GthCatalog *) gth_search_new ();
			gth_search_set_source (GTH_SEARCH (data->catalog),
					       gth_organize_task_get_folder (data->task),
					       gth_organize_task_get_recursive (data->task));

			date_test = gth_main_get_registered_object (GTH_TYPE_TEST, (policy == GTH_GROUP_POLICY_MODIFIED_DATE) ? "file::mtime" : "Embedded::Photo::DateTimeOriginal");
			gth_test_simple_set_data_as_date (GTH_TEST_SIMPLE (date_test), data->date_time->date);
			g_object_set (GTH_TEST_SIMPLE (date_test), "op", GTH_TEST_OP_EQUAL, "negative", FALSE, NULL);
			test_chain = gth_test_chain_new (GTH_MATCH_TYPE_ALL, date_test, NULL);
			gth_search_set_test (GTH_SEARCH (data->catalog), GTH_TEST_CHAIN (test_chain));

			g_object_unref (test_chain);
			g_object_unref (date_test);
		}
		break;

	case GTH_GROUP_POLICY_TAG:
	case GTH_GROUP_POLICY_TAG_EMBEDDED:

		/* delete the .catalog file to avoid to have two catalogs with
		 * the same name, which can be confusing for the user. */

		{
			GFile *catalog_file;
			GFile *gio_file;

			catalog_file = gth_catalog_get_file_for_tag (data->tag, ".catalog");
			gio_file = gth_catalog_file_to_gio_file (catalog_file);
			if (g_file_delete (gio_file, NULL, NULL)) {
				GFile *parent;
				GList *files;

				parent = g_file_get_parent (catalog_file);
				files = g_list_prepend (NULL, g_object_ref (catalog_file));
				gth_monitor_folder_changed (gth_main_get_default_monitor (),
							    parent,
							    files,
							    GTH_MONITOR_EVENT_DELETED);

				_g_object_list_unref (files);
				_g_object_unref (parent);
			}

			g_object_unref (gio_file);
			g_object_unref (catalog_file);
		}

		data->catalog_file = gth_catalog_get_file_for_tag (data->tag, ".search");
		data->catalog = gth_catalog_load_from_file (data->catalog_file);
		if (data->catalog == NULL) {
			GthTest *tag_test;
			GthTest *test_chain;

			data->catalog = (GthCatalog *) gth_search_new ();
			gth_search_set_source (GTH_SEARCH (data->catalog),
					       gth_organize_task_get_folder (data->task),
					       gth_organize_task_get_recursive (data->task));

			tag_test = gth_main_get_registered_object (GTH_TYPE_TEST, (policy == GTH_GROUP_POLICY_TAG) ? "comment::category" : "general::tags");
			gth_test_category_set (GTH_TEST_CATEGORY (tag_test), GTH_TEST_OP_CONTAINS, FALSE, data->tag);
			test_chain = gth_test_chain_new (GTH_MATCH_TYPE_ALL, tag_test, NULL);
			gth_search_set_test (GTH_SEARCH (data->catalog), GTH_TEST_CHAIN (test_chain));

			g_object_unref (test_chain);
			g_object_unref (tag_test);
		}

		break;
	}
}
