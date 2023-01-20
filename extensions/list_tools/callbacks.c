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
#include "actions.h"
#include "callbacks.h"
#include "gth-script-file.h"
#include "gth-script-task.h"
#include "list-tools.h"
#include "shortcuts.h"


#define BROWSER_DATA_KEY "list-tools-browser-data"


static const GActionEntry actions[] = {
	{ "exec-script", gth_browser_activate_exec_script, "s" },
	{ "personalize-tools", gth_browser_activate_personalize_tools }
};


typedef struct {
	GthBrowser *browser;
	gulong      scripts_changed_id;
	guint       menu_merge_id;
} BrowserData;


static void
browser_data_free (BrowserData *data)
{
	if (data->scripts_changed_id != 0)
		g_signal_handler_disconnect (gth_script_file_get (), data->scripts_changed_id);
	g_free (data);
}


static void
update_scripts (BrowserData *data)
{
	GthMenuManager	*menu_manager;
	GList		*script_list;
	GList		*scan;

	menu_manager = gth_browser_get_menu_manager (data->browser, GTH_BROWSER_MENU_MANAGER_TOOLS3);
	if (data->menu_merge_id != 0)
		gth_menu_manager_remove_entries (menu_manager, data->menu_merge_id);
	data->menu_merge_id = gth_menu_manager_new_merge_id (menu_manager);

	gth_window_remove_shortcuts (GTH_WINDOW (data->browser), SCRIPTS_GROUP);

	script_list = gth_script_file_get_scripts (gth_script_file_get ());
	for (scan = script_list; scan; scan = scan->next) {
		GthScript   *script = scan->data;
		GthShortcut *shortcut;

		shortcut = gth_script_create_shortcut (script);
		gth_window_add_removable_shortcut (GTH_WINDOW (data->browser),
						   SCRIPTS_GROUP,
						   shortcut);

		if (gth_script_is_visible (script)) {
			const char *script_action;
			char       *detailed_action;

			script_action = gth_script_get_detailed_action (script);
			if (! g_str_has_prefix (script_action, "win."))
				detailed_action = g_strdup_printf ("win.%s", script_action);
			else
				detailed_action = g_strdup (script_action);

			gth_menu_manager_append_entry (menu_manager,
						       data->menu_merge_id,
						       gth_script_get_display_name (script),
						       detailed_action,
						       "",
						       NULL);

			g_free (detailed_action);
		}

		gth_shortcut_free (shortcut);
	}

	_g_object_list_unref (script_list);
}


static void
scripts_changed_cb (GthScriptFile *script_file,
		    BrowserData   *data)
{
	update_scripts (data);
}


void
list_tools__gth_browser_construct_cb (GthBrowser *browser)
{
	BrowserData *data;
	GtkBuilder  *builder;
	GMenuModel  *menu;
	GtkWidget   *button;

	g_return_if_fail (GTH_IS_BROWSER (browser));

	data = g_new0 (BrowserData, 1);
	data->browser = browser;
	g_object_set_data_full (G_OBJECT (browser), BROWSER_DATA_KEY, data, (GDestroyNotify) browser_data_free);

	g_action_map_add_action_entries (G_ACTION_MAP (browser),
					 actions,
					 G_N_ELEMENTS (actions),
					 browser);

	builder = gtk_builder_new_from_resource ("/org/gnome/gThumb/list_tools/data/ui/tools-menu.ui");
	gth_browser_add_menu_manager_for_menu (browser, GTH_BROWSER_MENU_MANAGER_TOOLS, G_MENU (gtk_builder_get_object (builder, "tools1")));
	gth_browser_add_menu_manager_for_menu (browser, GTH_BROWSER_MENU_MANAGER_MORE_TOOLS, G_MENU (gtk_builder_get_object (builder, "tools2")));
	gth_browser_add_menu_manager_for_menu (browser, GTH_BROWSER_MENU_MANAGER_TOOLS3, G_MENU (gtk_builder_get_object (builder, "tools3")));
	gth_browser_add_menu_manager_for_menu (browser, GTH_BROWSER_MENU_MANAGER_TOOLS4, G_MENU (gtk_builder_get_object (builder, "tools4")));
	menu = G_MENU_MODEL (gtk_builder_get_object (builder, "tools-menu"));

	/* browser tools */

	button = _gtk_menu_button_new_for_header_bar ("tools-symbolic");
	gtk_widget_set_tooltip_text (button, _("Tools"));
	gtk_menu_button_set_menu_model (GTK_MENU_BUTTON (button), menu);
	gtk_widget_set_halign (GTK_WIDGET (gtk_menu_button_get_popup (GTK_MENU_BUTTON (button))), GTK_ALIGN_CENTER);
	gtk_widget_show (button);
	gtk_box_pack_end (GTK_BOX (gth_browser_get_headerbar_section (browser, GTH_BROWSER_HEADER_SECTION_BROWSER_TOOLS)), button, FALSE, FALSE, 0);

	/* viewer edit */

	button = _gtk_menu_button_new_for_header_bar ("tools-symbolic");
	gtk_widget_set_tooltip_text (button, _("Tools"));
	gtk_menu_button_set_menu_model (GTK_MENU_BUTTON (button), menu);
	gtk_widget_set_halign (GTK_WIDGET (gtk_menu_button_get_popup (GTK_MENU_BUTTON (button))), GTK_ALIGN_CENTER);
	gtk_widget_show (button);
	gtk_box_pack_end (GTK_BOX (gth_browser_get_headerbar_section (browser, GTH_BROWSER_HEADER_SECTION_VIEWER_EDIT)), button, FALSE, FALSE, 0);

	g_object_unref (builder);

	update_scripts (data);
	data->scripts_changed_id = g_signal_connect (gth_script_file_get (),
						     "changed",
						     G_CALLBACK (scripts_changed_cb),
						     data);
}


void
list_tools__gth_browser_selection_changed_cb (GthBrowser *browser,
					      int         n_selected)
{
	BrowserData *data;

	data = g_object_get_data (G_OBJECT (browser), BROWSER_DATA_KEY);
	g_return_if_fail (data != NULL);

	gth_window_enable_action (GTH_WINDOW (browser), "exec-script", n_selected > 0);
}
