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
#include "callbacks.h"


#define BROWSER_DATA_KEY "export-tools-browser-data"


static const char *ui_info =
"<ui>"
"  <toolbar name='ToolBar'>"
"    <placeholder name='Edit_Actions_2'>"
"      <toolitem action='ExportTools'/>"
"    </placeholder>"
"  </toolbar>"
"  <popup name='ExportPopup'>"
"    <placeholder name='Web_Services'/>"
"    <separator/>"
"    <placeholder name='Misc_Actions'/>"
"  </popup>"
"</ui>";


typedef struct {
	GthBrowser     *browser;
	GtkActionGroup *action_group;
	gboolean        menu_initialized;
} BrowserData;


static void
browser_data_free (BrowserData *data)
{
	g_free (data);
}


static void
export_tools_show_menu_func (GtkAction *action,
			     gpointer   user_data)
{
	BrowserData *data = user_data;
	GtkWidget   *menu;

	if (data->menu_initialized)
		return;

	data->menu_initialized = TRUE;

	menu = gtk_ui_manager_get_widget (gth_browser_get_ui_manager (data->browser), "/ExportPopup");
	g_object_set (action, "menu", menu, NULL);
}


void
export_tools__gth_browser_construct_cb (GthBrowser *browser)
{
	BrowserData *data;
	GtkAction   *action;
	GError      *error = NULL;
	guint        merge_id;

	g_return_if_fail (GTH_IS_BROWSER (browser));

	data = g_new0 (BrowserData, 1);
	data->browser = browser;
	data->action_group = gtk_action_group_new ("Export Tools Actions");
	gtk_action_group_set_translation_domain (data->action_group, NULL);

	/* tools menu action */

	action = g_object_new (GTH_TYPE_TOGGLE_MENU_ACTION,
			       "name", "ExportTools",
			       "icon-name", "share",
			       "label", _("Share"),
			       /*"tooltip",  _("Export files"),*/
			       "is-important", TRUE,
			       NULL);
	gth_toggle_menu_action_set_show_menu_func (GTH_TOGGLE_MENU_ACTION (action),
						   export_tools_show_menu_func,
						   data,
						   NULL);
	gtk_action_group_add_action (data->action_group, action);
	g_object_unref (action);

	gtk_ui_manager_insert_action_group (gth_browser_get_ui_manager (browser), data->action_group, 0);

	merge_id = gtk_ui_manager_add_ui_from_string (gth_browser_get_ui_manager (browser), ui_info, -1, &error);
	if (merge_id == 0) {
		g_warning ("building ui failed: %s", error->message);
		g_clear_error (&error);
	}

	g_object_set_data_full (G_OBJECT (browser), BROWSER_DATA_KEY, data, (GDestroyNotify) browser_data_free);
}
