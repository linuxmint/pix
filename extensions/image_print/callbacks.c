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
#include <gdk/gdkkeysyms.h>
#include <gthumb.h>
#include "actions.h"


#define BROWSER_DATA_KEY "image-print-browser-data"


static const char *fixed_ui_info =
"<ui>"
"  <menubar name='MenuBar'>"
"    <menu name='File' action='FileMenu'>"
"      <placeholder name='File_Actions_2'>"
"        <menuitem action='File_Print'/>"
"      </placeholder>"
"    </menu>"
"  </menubar>"
"  <toolbar name='ToolBar'>"
"      <placeholder name='Export_Actions'>"
"        <toolitem action='File_Print'/>"
"      </placeholder>"
"  </toolbar>"
"  <popup name='FileListPopup'>"
"    <placeholder name='Open_Actions'>"
"      <menuitem action='File_Print'/>"
"    </placeholder>"
"  </popup>"

"  <popup name='FilePopup'>"
"    <placeholder name='Open_Actions'>"
"      <menuitem action='File_Print'/>"
"    </placeholder>"
"  </popup>"
"</ui>";


static GthActionEntryExt action_entries[] = {
	{ "File_Print", GTK_STOCK_PRINT,
	  NULL, "<control>P",
	  N_("Print the selected images"),
	  GTH_ACTION_FLAG_IS_IMPORTANT,
	  G_CALLBACK (gth_browser_activate_action_file_print) },
};


typedef struct {
	GtkActionGroup *action_group;
	guint           fixed_merge_id;
} BrowserData;


static void
browser_data_free (BrowserData *data)
{
	g_free (data);
}


void
ip__gth_browser_construct_cb (GthBrowser *browser)
{
	BrowserData *data;
	GError      *error = NULL;

	g_return_if_fail (GTH_IS_BROWSER (browser));

	data = g_new0 (BrowserData, 1);

	data->action_group = gtk_action_group_new ("Image Print Actions");
	gtk_action_group_set_translation_domain (data->action_group, NULL);
	_gtk_action_group_add_actions_with_flags (data->action_group,
						  action_entries,
						  G_N_ELEMENTS (action_entries),
						  browser);
	gtk_ui_manager_insert_action_group (gth_browser_get_ui_manager (browser), data->action_group, 0);

	data->fixed_merge_id = gtk_ui_manager_add_ui_from_string (gth_browser_get_ui_manager (browser), fixed_ui_info, -1, &error);
	if (data->fixed_merge_id == 0) {
		g_warning ("building ui failed: %s", error->message);
		g_error_free (error);
	}

	g_object_set_data_full (G_OBJECT (browser), BROWSER_DATA_KEY, data, (GDestroyNotify) browser_data_free);
}


static void
set_action_sensitive (BrowserData *data,
		      const char  *action_name,
		      gboolean     sensitive)
{
	GtkAction *action;

	action = gtk_action_group_get_action (data->action_group, action_name);
	g_object_set (action, "sensitive", sensitive, NULL);
}


void
ip__gth_browser_update_sensitivity_cb (GthBrowser *browser)
{
	BrowserData *data;
	int          n_selected;

	data = g_object_get_data (G_OBJECT (browser), BROWSER_DATA_KEY);
	g_return_if_fail (data != NULL);

	n_selected = gth_file_selection_get_n_selected (GTH_FILE_SELECTION (gth_browser_get_file_list_view (browser)));
	set_action_sensitive (data, "File_Print", n_selected > 0);
}
