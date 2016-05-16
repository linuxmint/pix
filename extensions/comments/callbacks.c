/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2012 Free Software Foundation, Inc.
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


#define BROWSER_DATA_KEY "comments-data"


static const char *fixed_ui_file_tools_info =
"<ui>"
"  <popup name='ListToolsPopup'>"
"    <placeholder name='Tools_2'>"
"      <menuitem name='ImportEmbeddedMetadata' action='Tool_ImportEmbeddedMetadata'/>"
"    </placeholder>"
"  </popup>"
"</ui>";


static GthActionEntryExt comments_action_entries[] = {
	{ "Tool_ImportEmbeddedMetadata", NULL,
	  N_("Import Embedded Metadata"), NULL,
	  N_("Import the metadata stored inside files into the gThumb comment system"),
	  GTH_ACTION_FLAG_NONE,
	  G_CALLBACK (gth_browser_activate_action_tool_import_embedded_metadata) }
};


typedef struct {
	GthBrowser     *browser;
	GtkActionGroup *actions;
} BrowserData;


static void
browser_data_free (BrowserData *data)
{
	g_free (data);
}


void
comments__gth_browser_construct_cb (GthBrowser *browser)
{
	BrowserData *data;
	GError      *error = NULL;

	g_return_if_fail (GTH_IS_BROWSER (browser));

	data = g_new0 (BrowserData, 1);
	data->browser = browser;

	data->actions = gtk_action_group_new ("Comments Actions");
	gtk_action_group_set_translation_domain (data->actions, NULL);
	_gtk_action_group_add_actions_with_flags (data->actions,
						  comments_action_entries,
						  G_N_ELEMENTS (comments_action_entries),
						  browser);
	gtk_ui_manager_insert_action_group (gth_browser_get_ui_manager (browser), data->actions, 0);

	if (gth_main_extension_is_active ("list_tools") && ! gtk_ui_manager_add_ui_from_string (gth_browser_get_ui_manager (browser), fixed_ui_file_tools_info, -1, &error)) {
		g_message ("building menus failed: %s", error->message);
		g_error_free (error);
	}

	g_object_set_data_full (G_OBJECT (browser), BROWSER_DATA_KEY, data, (GDestroyNotify) browser_data_free);
}
