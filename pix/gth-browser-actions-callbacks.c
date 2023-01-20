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
#include "gth-browser-actions-callbacks.h"
#include "gth-file-list.h"
#include "gth-file-selection.h"
#include "gth-filterbar.h"
#include "gth-folder-tree.h"
#include "gth-main.h"
#include "gth-preferences.h"
#include "gth-shortcuts-window.h"
#include "gth-sidebar.h"
#include "gtk-utils.h"
#include "gth-viewer-page.h"
#include "main.h"


void
gth_browser_activate_new_window (GSimpleAction *action,
				 GVariant      *parameter,
				 gpointer       user_data)
{
	GtkWidget *browser = user_data;
	GtkWidget *window;

	window = gth_browser_new (gth_browser_get_location (GTH_BROWSER (browser)), NULL);
	gtk_window_present (GTK_WINDOW (window));
}


void
gth_browser_activate_preferences (GSimpleAction *action,
				  GVariant      *parameter,
				  gpointer       user_data)
{
	dlg_preferences (GTH_BROWSER (user_data));
}


void
gth_browser_activate_show_help (GSimpleAction *action,
				GVariant      *parameter,
				gpointer       user_data)
{
	show_help_dialog (GTK_WINDOW (user_data), NULL);
}


void
gth_browser_activate_show_shortcuts (GSimpleAction *action,
				     GVariant      *parameter,
				     gpointer       user_data)
{
	gth_shortcuts_window_new (GTH_WINDOW (user_data));
}


void
gth_browser_activate_about (GSimpleAction *action,
			    GVariant      *parameter,
			    gpointer       user_data)
{
	GthBrowser   *browser = user_data;
	const char   *authors[] = {
#include "AUTHORS.tab"
		NULL
	};
	const char   *documenters [] = {
		"Paolo Bacchilega",
		"Alexander Kirillov",
		NULL
	};
	char       *license_text;
	const char *license[] = {
		N_("gThumb is free software; you can redistribute it and/or modify "
		"it under the terms of the GNU General Public License as published by "
		"the Free Software Foundation; either version 2 of the License, or "
		"(at your option) any later version."),
		N_("gThumb is distributed in the hope that it will be useful, "
		"but WITHOUT ANY WARRANTY; without even the implied warranty of "
		"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the "
		"GNU General Public License for more details."),
		N_("You should have received a copy of the GNU General Public License "
		"along with gThumb.  If not, see http://www.gnu.org/licenses/.")
	};
	GdkPixbuf *logo;

	license_text = g_strconcat (_(license[0]), "\n\n",
				    _(license[1]), "\n\n",
				    _(license[2]),
				    NULL);

	logo = gtk_icon_theme_load_icon (gtk_icon_theme_get_for_screen (gtk_widget_get_screen (GTK_WIDGET (browser))),
					 "org.gnome.gThumb",
					 128,
					 0,
					 NULL);

	gtk_show_about_dialog (GTK_WINDOW (browser),
			       "version", PACKAGE_VERSION,
			       "copyright", "Copyright \xc2\xa9 2001-2020 Free Software Foundation, Inc.",
			       "comments", _("An image viewer and browser for GNOME."),
			       "authors", authors,
			       "documenters", documenters,
			       "translator-credits", _("translator-credits"),
			       "license", license_text,
			       "wrap-license", TRUE,
			       "website", "https://wiki.gnome.org/Apps/Gthumb",
			       (logo != NULL ? "logo" : NULL), logo,
			       NULL);

	_g_object_unref (logo);
	g_free (license_text);
}


void
gth_browser_activate_quit (GSimpleAction *action,
			   GVariant      *parameter,
			   gpointer       user_data)
{
	gth_quit (FALSE);
}


void
gth_browser_activate_browser_mode (GSimpleAction *action,
				   GVariant      *parameter,
				   gpointer       user_data)
{
	GthBrowser *browser = user_data;
	GtkWidget  *viewer_sidebar;

	gth_browser_stop (browser);

	viewer_sidebar = gth_browser_get_viewer_sidebar (browser);
	if (gth_sidebar_tool_is_active (GTH_SIDEBAR (viewer_sidebar)))
		gth_sidebar_deactivate_tool (GTH_SIDEBAR (viewer_sidebar));
	else if (gth_browser_get_is_fullscreen (browser))
		gth_browser_unfullscreen (browser);
	else
		gth_window_set_current_page (GTH_WINDOW (browser), GTH_BROWSER_PAGE_BROWSER);
}


void
gth_browser_activate_browser_edit_file (GSimpleAction *action,
					GVariant      *parameter,
					gpointer       user_data)
{
	gth_browser_show_viewer_tools (GTH_BROWSER (user_data));
}


void
gth_browser_activate_browser_properties (GSimpleAction *action,
					 GVariant      *state,
					 gpointer       user_data)
{
	GthBrowser *browser = user_data;

	g_simple_action_set_state (action, state);
	if (g_variant_get_boolean (state))
		gth_browser_show_file_properties (GTH_BROWSER (browser));
	else
		gth_browser_hide_sidebar (GTH_BROWSER (browser));
}


void
gth_browser_activate_clear_history (GSimpleAction *action,
				    GVariant      *parameter,
				    gpointer       user_data)
{
	gth_browser_clear_history (GTH_BROWSER (user_data));
}


void
gth_browser_activate_close (GSimpleAction *action,
			    GVariant      *parameter,
			    gpointer       user_data)
{
	GthBrowser *browser = user_data;

	gth_window_close (GTH_WINDOW (browser));
}


void
gth_browser_activate_fullscreen (GSimpleAction *action,
				 GVariant      *parameter,
				 gpointer       user_data)
{
	gth_browser_fullscreen (GTH_BROWSER (user_data));
}


void
gth_browser_activate_go_back (GSimpleAction *action,
			      GVariant      *parameter,
			      gpointer       user_data)
{
	gth_browser_go_back (GTH_BROWSER (user_data), 1);
}


void
gth_browser_activate_go_forward (GSimpleAction *action,
			         GVariant      *parameter,
			         gpointer       user_data)
{
	gth_browser_go_forward (GTH_BROWSER (user_data), 1);
}


void
gth_browser_activate_go_to_history_pos (GSimpleAction *action,
					GVariant      *parameter,
					gpointer       user_data)
{
	gth_browser_go_to_history_pos (GTH_BROWSER (user_data), atoi (g_variant_get_string (parameter, NULL)));
}


void
gth_browser_activate_go_to_location (GSimpleAction *action,
				     GVariant      *parameter,
				     gpointer       user_data)
{
	GFile *file;

	file = g_file_new_for_uri (g_variant_get_string (parameter, NULL));
	gth_browser_go_to (GTH_BROWSER (user_data), file, NULL);

	g_object_unref (file);
}


void
gth_browser_activate_go_home (GSimpleAction *action,
			      GVariant      *parameter,
			      gpointer       user_data)
{
	gth_browser_go_home (GTH_BROWSER (user_data));
}


void
gth_browser_activate_go_up (GSimpleAction *action,
			    GVariant      *parameter,
			    gpointer       user_data)
{
	gth_browser_go_up (GTH_BROWSER (user_data), 1);
}


void
gth_browser_activate_reload (GSimpleAction *action,
			     GVariant      *parameter,
			     gpointer       user_data)
{
	gth_browser_reload (GTH_BROWSER (user_data));
}


void
gth_browser_activate_open_location (GSimpleAction *action,
				    GVariant      *parameter,
				    gpointer       user_data)
{
	dlg_location (GTH_BROWSER (user_data));
}


void
gth_browser_activate_revert_to_saved (GSimpleAction *action,
				      GVariant      *parameter,
				      gpointer       user_data)
{
	GthBrowser    *browser = user_data;
	GthViewerPage *viewer_page;

	viewer_page = gth_browser_get_viewer_page (browser);
	if (viewer_page == NULL)
		return;

	gth_viewer_page_revert (viewer_page);
}


void
gth_browser_activate_save (GSimpleAction *action,
			   GVariant      *parameter,
			   gpointer       user_data)
{
	GthBrowser    *browser = user_data;
	GthViewerPage *viewer_page;

	viewer_page = gth_browser_get_viewer_page (browser);
	if (viewer_page == NULL)
		return;

	gth_viewer_page_save (viewer_page, NULL, NULL, browser);
}


void
gth_browser_activate_save_as (GSimpleAction *action,
			      GVariant      *parameter,
			      gpointer       user_data)
{
	GthBrowser    *browser = user_data;
	GthViewerPage *viewer_page;

	viewer_page = gth_browser_get_viewer_page (browser);
	if (viewer_page == NULL)
		return;

	gth_viewer_page_save_as (viewer_page, NULL, NULL);
}


void
gth_browser_activate_toggle_edit_file (GSimpleAction *action,
					GVariant      *state,
					gpointer       user_data)
{
	GthBrowser *browser = user_data;
	gth_browser_toggle_viewer_tools (browser);
}


void
gth_browser_activate_toggle_file_properties (GSimpleAction *action,
					     GVariant      *state,
					     gpointer       user_data)
{
	GthBrowser *browser = user_data;
	gth_browser_toggle_file_properties (browser);
}


void
gth_browser_activate_viewer_edit_file (GSimpleAction *action,
				       GVariant      *state,
				       gpointer       user_data)
{
	GthBrowser *browser = user_data;

	g_simple_action_set_state (action, state);
	if (g_variant_get_boolean (state))
		gth_browser_show_viewer_tools (browser);
	else
		gth_browser_hide_sidebar (browser);
}


void
gth_browser_activate_viewer_properties (GSimpleAction *action,
				        GVariant      *state,
				        gpointer       user_data)
{
	GthBrowser *browser = user_data;

	g_simple_action_set_state (action, state);
	if (g_variant_get_boolean (state))
		gth_browser_show_file_properties (browser);
	else
		gth_browser_hide_sidebar (browser);
}


void
gth_browser_activate_unfullscreen (GSimpleAction *action,
				   GVariant      *parameter,
				   gpointer       user_data)
{
	gth_browser_unfullscreen (GTH_BROWSER (user_data));
}


void
gth_browser_activate_open_folder_in_new_window (GSimpleAction *action,
						GVariant      *parameter,
						gpointer       user_data)
{
	GthBrowser  *browser = GTH_BROWSER (user_data);
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
gth_browser_activate_show_progress_dialog (GSimpleAction *action,
					   GVariant      *parameter,
					   gpointer       user_data)
{
	gth_browser_show_progress_dialog (GTH_BROWSER (user_data));
}


void
gth_browser_activate_show_hidden_files (GSimpleAction *action,
					GVariant      *state,
					gpointer       user_data)
{
	GSettings *settings;

	g_simple_action_set_state (action, state);

	settings = g_settings_new (GTHUMB_BROWSER_SCHEMA);
	g_settings_set_boolean (settings, PREF_BROWSER_SHOW_HIDDEN_FILES, g_variant_get_boolean (state));
	g_object_unref (settings);
}


void
gth_browser_activate_sort_by (GSimpleAction *action,
			      GVariant      *parameter,
			      gpointer       user_data)
{
	dlg_sort_order (GTH_BROWSER (user_data));
}


void
gth_browser_activate_show_statusbar (GSimpleAction *action,
				     GVariant      *state,
				     gpointer       user_data)
{
	GSettings *settings;

	g_simple_action_set_state (action, state);

	settings = g_settings_new (GTHUMB_BROWSER_SCHEMA);
	g_settings_set_boolean (settings, PREF_BROWSER_STATUSBAR_VISIBLE, g_variant_get_boolean (state));
	g_object_unref (settings);
}


void
gth_browser_activate_toggle_statusbar (GSimpleAction *action,
				       GVariant      *state,
				       gpointer       user_data)
{
	GSettings *settings;

	settings = g_settings_new (GTHUMB_BROWSER_SCHEMA);
	g_settings_set_boolean (settings, PREF_BROWSER_STATUSBAR_VISIBLE, ! g_settings_get_boolean (settings, PREF_BROWSER_STATUSBAR_VISIBLE));
	g_object_unref (settings);
}


void
gth_browser_activate_show_sidebar (GSimpleAction *action,
				   GVariant      *state,
				   gpointer       user_data)
{
	GSettings *settings;

	g_simple_action_set_state (action, state);

	settings = g_settings_new (GTHUMB_BROWSER_SCHEMA);
	g_settings_set_boolean (settings, PREF_BROWSER_SIDEBAR_VISIBLE, g_variant_get_boolean (state));
	g_object_unref (settings);
}


void
gth_browser_activate_toggle_sidebar (GSimpleAction *action,
				     GVariant      *state,
				     gpointer       user_data)
{
	GSettings *settings;

	settings = g_settings_new (GTHUMB_BROWSER_SCHEMA);
	g_settings_set_boolean (settings, PREF_BROWSER_SIDEBAR_VISIBLE, ! g_settings_get_boolean (settings, PREF_BROWSER_SIDEBAR_VISIBLE));
	g_object_unref (settings);
}


void
gth_browser_activate_show_thumbnail_list (GSimpleAction *action,
					  GVariant      *state,
					  gpointer       user_data)
{
	GSettings *settings;

	g_simple_action_set_state (action, state);

	settings = g_settings_new (GTHUMB_BROWSER_SCHEMA);
	g_settings_set_boolean (settings, PREF_BROWSER_THUMBNAIL_LIST_VISIBLE, g_variant_get_boolean (state));
	g_object_unref (settings);
}


void
gth_browser_activate_toggle_thumbnail_list (GSimpleAction *action,
					    GVariant      *state,
					    gpointer       user_data)
{
	GSettings *settings;

	settings = g_settings_new (GTHUMB_BROWSER_SCHEMA);
	g_settings_set_boolean (settings, PREF_BROWSER_THUMBNAIL_LIST_VISIBLE, ! g_settings_get_boolean (settings, PREF_BROWSER_THUMBNAIL_LIST_VISIBLE));
	g_object_unref (settings);
}


void
gth_browser_activate_show_first_image (GSimpleAction *action,
				       GVariant      *state,
				       gpointer       user_data)
{
	GthBrowser *browser = GTH_BROWSER (user_data);
	gth_browser_show_first_image (browser, FALSE, FALSE);
}


void
gth_browser_activate_show_last_image (GSimpleAction *action,
				      GVariant      *state,
				      gpointer       user_data)
{
	GthBrowser *browser = GTH_BROWSER (user_data);
	gth_browser_show_last_image (browser, FALSE, FALSE);
}


void
gth_browser_activate_show_previous_image (GSimpleAction *action,
					  GVariant      *state,
					  gpointer       user_data)
{
	GthBrowser *browser = GTH_BROWSER (user_data);
	gth_browser_show_prev_image (browser, FALSE, FALSE);
}


void
gth_browser_activate_show_next_image (GSimpleAction *action,
				      GVariant      *state,
				      gpointer       user_data)
{
	GthBrowser *browser = GTH_BROWSER (user_data);
	gth_browser_show_next_image (browser, FALSE, FALSE);
}


void
gth_browser_activate_apply_editor_changes (GSimpleAction *action,
					   GVariant      *state,
					   gpointer       user_data)
{
	GthBrowser *browser = GTH_BROWSER (user_data);
	gth_browser_apply_editor_changes (browser);
}


void
gth_browser_activate_select_all (GSimpleAction *action,
				 GVariant      *state,
				 gpointer       user_data)
{
	GthBrowser *browser = GTH_BROWSER (user_data);
	GtkWidget  *file_view;

	file_view = gth_browser_get_file_list_view (browser);
	gth_file_selection_select_all (GTH_FILE_SELECTION (file_view));
}


void
gth_browser_activate_unselect_all (GSimpleAction *action,
				   GVariant      *state,
				   gpointer       user_data)
{
	GthBrowser *browser = GTH_BROWSER (user_data);
	GtkWidget  *file_view;

	file_view = gth_browser_get_file_list_view (browser);
	gth_file_selection_unselect_all (GTH_FILE_SELECTION (file_view));
}


void
gth_browser_activate_show_menu (GSimpleAction *action,
				GVariant      *state,
				gpointer       user_data)
{
	gth_browser_show_menu (GTH_BROWSER (user_data));
}


void
gth_browser_activate_set_filter (GSimpleAction *action,
				 GVariant	*parameter,
				 gpointer	 user_data)
{
	GthBrowser *browser = GTH_BROWSER (user_data);
	const char *test_id;
	GtkWidget  *filterbar;

	test_id = g_variant_get_string (parameter, NULL);
	filterbar = gth_browser_get_filterbar (browser);
	gth_filterbar_set_test_by_id (GTH_FILTERBAR (filterbar), test_id);
}


void
gth_browser_activate_set_filter_all (GSimpleAction *action,
				     GVariant      *parameter,
				     gpointer        user_data)
{
	GthBrowser *browser = GTH_BROWSER (user_data);
	GtkWidget  *filterbar;

	filterbar = gth_browser_get_filterbar (browser);
	gth_filterbar_set_show_all (GTH_FILTERBAR (filterbar));
}
