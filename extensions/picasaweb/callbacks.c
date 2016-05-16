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


#define BROWSER_DATA_KEY "picasaweb-browser-data"


static const char *ui_info =
"<ui>"
"  <menubar name='MenuBar'>"
"    <menu name='File' action='FileMenu'>"
"      <menu name='Import' action='ImportMenu'>"
"        <placeholder name='Web_Services'>"
"          <menuitem action='File_Import_PicasaWeb'/>"
"        </placeholder>"
"      </menu>"
"      <menu name='Export' action='ExportMenu'>"
"        <placeholder name='Web_Services'>"
"          <menuitem action='File_Export_PicasaWeb'/>"
"        </placeholder>"
"      </menu>"
"    </menu>"
"  </menubar>"
"  <popup name='ExportPopup'>"
"    <placeholder name='Web_Services'>"
"      <menuitem action='File_Export_PicasaWeb'/>"
"    </placeholder>"
"  </popup>"
"</ui>";


static GthActionEntryExt action_entries[] = {
	{ "File_Import_PicasaWeb", "site-picasaweb",
	  N_("_Picasa Web Album..."), NULL,
	  N_("Download photos from Picasa Web Album"),
	  GTH_ACTION_FLAG_ALWAYS_SHOW_IMAGE,
	  G_CALLBACK (gth_browser_activate_action_import_picasaweb) },
	{ "File_Export_PicasaWeb", "site-picasaweb",
	  N_("_Picasa Web Album..."), NULL,
	  N_("Upload photos to Picasa Web Album"),
	  GTH_ACTION_FLAG_ALWAYS_SHOW_IMAGE,
	  G_CALLBACK (gth_browser_activate_action_export_picasaweb) },
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
pw__gth_browser_construct_cb (GthBrowser *browser)
{
	BrowserData *data;
	GError      *error = NULL;
	guint        merge_id;

	g_return_if_fail (GTH_IS_BROWSER (browser));

	data = g_new0 (BrowserData, 1);

	data->action_group = gtk_action_group_new ("Picasa Web Actions");
	gtk_action_group_set_translation_domain (data->action_group, NULL);
	_gtk_action_group_add_actions_with_flags (data->action_group,
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
