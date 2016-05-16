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
#include <glib-object.h>
#include <gthumb.h>
#include "actions.h"


#define BROWSER_DATA_KEY "webalbums-browser-data"


static const char *ui_info =
"<ui>"
/* FIXME
"  <menubar name='MenuBar'>"
"    <menu name='File' action='FileMenu'>"
"      <placeholder name='Misc_Actions'>"
"        <menuitem action='Tool_CreateWebAlbum'/>"
"      </placeholder>"
"    </menu>"
"  </menubar>"
*/
"  <menubar name='MenuBar'>"
"    <menu name='File' action='FileMenu'>"
"      <menu name='Export' action='ExportMenu'>"
"        <placeholder name='Web_Services'>"
"          <menuitem action='Tool_CreateWebAlbum'/>"
"        </placeholder>"
"      </menu>"
"    </menu>"
"  </menubar>"
/*
"  <popup name='ListToolsPopup'>"
"    <placeholder name='Tools'>"
"      <menuitem name='CreateWebAlbum' action='Tool_CreateWebAlbum'/>"
"    </placeholder>"
"  </popup>"
*/

"  <popup name='ExportPopup'>"
"    <placeholder name='Web_Services'>"
"      <menuitem action='Tool_CreateWebAlbum'/>"
"    </placeholder>"
"  </popup>"
"</ui>";


static GtkActionEntry action_entries[] = {
	{ "Tool_CreateWebAlbum", "webalbums",
	  N_("_Web Album..."), NULL,
	  N_("Create a static web album"),
	  G_CALLBACK (gth_browser_activate_action_export_webalbum) },
};


typedef struct {
	GtkActionGroup *action_group;
} BrowserData;


static void
browser_data_free (BrowserData *data)
{
	g_free (data);
}


void
wa__gth_browser_construct_cb (GthBrowser *browser)
{
	BrowserData *data;
	GError      *error = NULL;
	guint        merge_id;

	g_return_if_fail (GTH_IS_BROWSER (browser));

	data = g_new0 (BrowserData, 1);

	data->action_group = gtk_action_group_new ("Web Albums Actions");
	gtk_action_group_set_translation_domain (data->action_group, NULL);
	gtk_action_group_add_actions (data->action_group,
				      action_entries,
				      G_N_ELEMENTS (action_entries),
				      browser);
	gtk_ui_manager_insert_action_group (gth_browser_get_ui_manager (browser), data->action_group, 0);

	merge_id = gtk_ui_manager_add_ui_from_string (gth_browser_get_ui_manager (browser), ui_info, -1, &error);
	if (merge_id == 0) {
		g_warning ("building ui failed: %s", error->message);
		g_clear_error (&error);
	}

	g_object_set_data_full (G_OBJECT (browser), BROWSER_DATA_KEY, data, (GDestroyNotify) browser_data_free);
}


void
wa__gth_browser_update_sensitivity_cb (GthBrowser *browser)
{
	BrowserData *data;
	GtkAction   *action;
	int          n_selected;
	gboolean     sensitive;

	data = g_object_get_data (G_OBJECT (browser), BROWSER_DATA_KEY);
	g_return_if_fail (data != NULL);

	n_selected = gth_file_selection_get_n_selected (GTH_FILE_SELECTION (gth_browser_get_file_list_view (browser)));
	sensitive = n_selected > 0;

	action = gtk_action_group_get_action (data->action_group, "File_CreateWebAlbum");
	g_object_set (action, "sensitive", sensitive, NULL);
}
