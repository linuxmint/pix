/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2010 Free Software Foundation, Inc.
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
#include "dlg-location.h"
#include "dlg-personalize-filters.h"
#include "dlg-preferences.h"
#include "dlg-sort-order.h"
#include "glib-utils.h"
#include "gth-browser.h"
#include "gth-file-list.h"
#include "gth-file-selection.h"
#include "gth-folder-tree.h"
#include "gth-main.h"
#include "gth-preferences.h"
#include "gth-sidebar.h"
#include "gtk-utils.h"
#include "gth-viewer-page.h"


void
gth_browser_activate_action_file_open (GtkAction  *action,
				       GthBrowser *browser)
{
	GList *items;
	GList *file_list;

	items = gth_file_selection_get_selected (GTH_FILE_SELECTION (gth_browser_get_file_list_view (browser)));
	file_list = gth_file_list_get_files (GTH_FILE_LIST (gth_browser_get_file_list (browser)), items);

	if (file_list != NULL)
		gth_browser_load_file (browser, (GthFileData *) file_list->data, TRUE);

	_g_object_list_unref (file_list);
	_gtk_tree_path_list_free (items);
}


void
gth_browser_activate_action_file_revert (GtkAction  *action,
					 GthBrowser *browser)
{
	GthFileData *file_data;

	file_data = gth_browser_get_current_file (browser);
	if (file_data == NULL)
		return;
	g_file_info_set_attribute_boolean (file_data->info, "gth::file::is-modified", FALSE);
	gth_browser_load_file (browser, file_data, TRUE);
}


void
gth_browser_activate_action_file_save (GtkAction  *action,
				       GthBrowser *browser)
{
	GtkWidget *viewer_page;

	viewer_page = gth_browser_get_viewer_page (browser);
	if (viewer_page == NULL)
		return;

	gth_viewer_page_save (GTH_VIEWER_PAGE (viewer_page), NULL, NULL, browser);
}


void
gth_browser_activate_action_file_save_as (GtkAction  *action,
					  GthBrowser *browser)
{
	GtkWidget *viewer_page;

	viewer_page = gth_browser_get_viewer_page (browser);
	if (viewer_page == NULL)
		return;

	gth_viewer_page_save_as (GTH_VIEWER_PAGE (viewer_page), NULL, NULL);
}


void
gth_browser_activate_action_file_new_window (GtkAction  *action,
					     GthBrowser *browser)
{
	GtkWidget *window;

	window = gth_browser_new (gth_browser_get_location (browser), NULL);
	gtk_window_present (GTK_WINDOW (window));
}


void
gth_browser_activate_action_edit_preferences (GtkAction  *action,
					      GthBrowser *browser)
{
	dlg_preferences (browser);
}


void
gth_browser_activate_action_go_back (GtkAction  *action,
				     GthBrowser *browser)
{
	gth_browser_go_back (browser, 1);
}


void
gth_browser_activate_action_go_forward (GtkAction  *action,
					GthBrowser *browser)
{
	gth_browser_go_forward (browser, 1);
}


void
gth_browser_activate_action_go_up (GtkAction  *action,
				   GthBrowser *browser)
{
	gth_browser_go_up (browser, 1);
}


void
gth_browser_activate_action_go_location (GtkAction  *action,
					 GthBrowser *browser)
{
	dlg_location (browser);
}


void
gth_browser_activate_action_go_home (GtkAction  *action,
				     GthBrowser *browser)
{
	gth_browser_go_home (browser);
}


void
gth_browser_activate_action_go_clear_history (GtkAction  *action,
					      GthBrowser *browser)
{
	gth_browser_clear_history (browser);
}


void
gth_browser_activate_action_view_filter (GtkAction  *action,
					 GthBrowser *browser)
{
	dlg_personalize_filters (browser);
}


void
gth_browser_activate_action_view_filterbar (GtkAction  *action,
					    GthBrowser *browser)
{
	gth_browser_show_filterbar (browser, gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)));
}


void
gth_browser_activate_action_view_fullscreen (GtkAction  *action,
					     GthBrowser *browser)
{
	gth_browser_fullscreen (browser);
}


void
gth_browser_activate_action_view_sort_by (GtkAction  *action,
					  GthBrowser *browser)
{
	dlg_sort_order (browser);
}


void
gth_browser_activate_action_view_thumbnails (GtkAction  *action,
					     GthBrowser *browser)
{
	gth_browser_enable_thumbnails (browser, gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)));
}


void
gth_browser_activate_action_view_toolbar (GtkAction  *action,
					  GthBrowser *browser)
{
	GSettings *settings;

	settings = g_settings_new (GTHUMB_BROWSER_SCHEMA);
	g_settings_set_boolean (settings, PREF_BROWSER_TOOLBAR_VISIBLE, gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)));

	g_object_unref (settings);
}


void
gth_browser_activate_action_view_show_hidden_files (GtkAction  *action,
						    GthBrowser *browser)
{
	GSettings *settings;

	settings = g_settings_new (GTHUMB_BROWSER_SCHEMA);
	g_settings_set_boolean (settings, PREF_BROWSER_SHOW_HIDDEN_FILES, gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)));

	g_object_unref (settings);
}


void
gth_browser_activate_action_view_statusbar (GtkAction  *action,
					    GthBrowser *browser)
{
	GSettings *settings;

	settings = g_settings_new (GTHUMB_BROWSER_SCHEMA);
	g_settings_set_boolean (settings, PREF_BROWSER_STATUSBAR_VISIBLE, gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)));

	g_object_unref (settings);
}


void
gth_browser_activate_action_view_sidebar (GtkAction  *action,
					  GthBrowser *browser)
{
	GSettings *settings;

	settings = g_settings_new (GTHUMB_BROWSER_SCHEMA);
	g_settings_set_boolean (settings, PREF_BROWSER_SIDEBAR_VISIBLE, gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)));

	g_object_unref (settings);
}


void
gth_browser_activate_action_view_thumbnail_list (GtkAction  *action,
						 GthBrowser *browser)
{
	GSettings *settings;

	settings = g_settings_new (GTHUMB_BROWSER_SCHEMA);
	g_settings_set_boolean (settings, PREF_BROWSER_THUMBNAIL_LIST_VISIBLE, gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)));

	g_object_unref (settings);
}


void
gth_browser_activate_action_view_stop (GtkAction  *action,
				       GthBrowser *browser)
{
	gth_browser_stop (browser);
}


void
gth_browser_activate_action_view_reload (GtkAction  *action,
					 GthBrowser *browser)
{
	gth_browser_reload (browser);
}


void
gth_browser_activate_action_view_prev (GtkAction  *action,
				       GthBrowser *browser)
{
	gth_browser_show_prev_image (browser, FALSE, FALSE);
}


void
gth_browser_activate_action_view_next (GtkAction  *action,
				       GthBrowser *browser)
{
	gth_browser_show_next_image (browser, FALSE, FALSE);
}


void
gth_browser_activate_action_folder_open (GtkAction  *action,
					 GthBrowser *browser)
{
	GthFileData *file_data;

	file_data = gth_browser_get_folder_popup_file_data (browser);
	if (file_data == NULL)
		return;

	gth_browser_load_location (browser, file_data->file);

	g_object_unref (file_data);
}


void
gth_browser_activate_action_folder_open_in_new_window (GtkAction  *action,
						       GthBrowser *browser)
{
	GthFileData *file_data;
	GtkWidget   *new_browser;

	file_data = gth_browser_get_folder_popup_file_data (browser);
	if (file_data == NULL)
		return;

	new_browser = gth_browser_new (file_data->file, NULL);
	gtk_window_present (GTK_WINDOW (new_browser));

	g_object_unref (file_data);
}


void
gth_browser_activate_action_browser_mode (GtkAction  *action,
					  GthBrowser *browser)
{
	GtkWidget *viewer_sidebar;

	viewer_sidebar = gth_browser_get_viewer_sidebar (browser);
	if (gth_sidebar_tool_is_active (GTH_SIDEBAR (viewer_sidebar)))
		gth_sidebar_deactivate_tool (GTH_SIDEBAR (viewer_sidebar));
	else
		gth_window_set_current_page (GTH_WINDOW (browser), GTH_BROWSER_PAGE_BROWSER);
}


void
gth_browser_activate_action_viewer_properties (GtkAction  *action,
						GthBrowser *browser)
{
	if (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)))
		gth_browser_show_file_properties (GTH_BROWSER (browser));
	else
		gth_browser_hide_sidebar (GTH_BROWSER (browser));
}


void
gth_browser_activate_action_browser_tools (GtkAction  *action,
					   GthBrowser *browser)
{
	gth_browser_show_viewer_tools (GTH_BROWSER (browser));
}


void
gth_browser_activate_action_viewer_tools (GtkAction  *action,
					  GthBrowser *browser)
{
	if (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)))
		gth_browser_show_viewer_tools (GTH_BROWSER (browser));
	else
		gth_browser_hide_sidebar (GTH_BROWSER (browser));
}


void
gth_browser_activate_action_view_shrink_wrap (GtkAction  *action,
					      GthBrowser *browser)
{
	gth_browser_set_shrink_wrap_viewer (GTH_BROWSER (browser), gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)));
}


void
gth_browser_activate_action_edit_select_all (GtkAction  *action,
				 	     GthBrowser *browser)
{
	gth_file_selection_select_all (GTH_FILE_SELECTION (gth_browser_get_file_list_view (browser)));
}


void
gth_browser_activate_action_help_about (GtkAction *action,
				        gpointer   data)
{
	GthWindow  *window = GTH_WINDOW (data);
	const char *authors[] = {
#include "AUTHORS.tab"
		NULL
	};
	const char *documenters [] = {
		"Paolo Bacchilega",
		"Alexander Kirillov",
		NULL
	};
	char       *license_text;
	const char *license[] = {
		N_("gthumb is free software; you can redistribute it and/or modify "
		"it under the terms of the GNU General Public License as published by "
		"the Free Software Foundation; either version 2 of the License, or "
		"(at your option) any later version."),
		N_("gthumb is distributed in the hope that it will be useful, "
		"but WITHOUT ANY WARRANTY; without even the implied warranty of "
		"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the "
		"GNU General Public License for more details."),
		N_("You should have received a copy of the GNU General Public License "
		"along with gthumb.  If not, see http://www.gnu.org/licenses/.")
	};
	GdkPixbuf *logo;

	license_text = g_strconcat (_(license[0]), "\n\n",
				    _(license[1]), "\n\n",
				    _(license[2]),
				    NULL);

	logo = gtk_icon_theme_load_icon (gtk_icon_theme_get_for_screen (gtk_widget_get_screen (GTK_WIDGET (window))),
					 "gthumb",
					 128,
					 GTK_ICON_LOOKUP_NO_SVG,
					 NULL);

	gtk_show_about_dialog (GTK_WINDOW (window),
			       "version", VERSION,
			       "copyright", "Copyright \xc2\xa9 2001-2010 Free Software Foundation, Inc.",
			       "comments", _("An image viewer and browser for GNOME."),
			       "authors", authors,
			       "documenters", documenters,
			       "translator-credits", _("translator_credits"),
			       "license", license_text,
			       "wrap-license", TRUE,
			       "website", "http://live.gnome.org/Apps/gthumb",
			       (logo != NULL ? "logo" : NULL), logo,
			       NULL);

	_g_object_unref (logo);
	g_free (license_text);
}


void
gth_browser_activate_action_help_help (GtkAction *action,
				       gpointer   data)
{
	show_help_dialog (GTK_WINDOW (data), NULL);
}


void
gth_browser_activate_action_help_shortcuts (GtkAction *action,
					    gpointer   data)
{
	show_help_dialog (GTK_WINDOW (data), "gthumb-shortcuts");
}
