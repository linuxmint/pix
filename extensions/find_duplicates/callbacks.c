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


#define BROWSER_DATA_KEY "find-duplicates-browser-data"


static const char *find_ui_info =
"<ui>"
"  <menubar name='MenuBar'>"
"    <menu name='Edit' action='EditMenu'>"
"      <placeholder name='Edit_Actions'>"
"        <menuitem action='Edit_Find_Duplicates'/>"
"      </placeholder>"
"    </menu>"
"  </menubar>"
"</ui>";


static GtkActionEntry find_action_entries[] = {
	{ "Edit_Find_Duplicates", NULL,
	  N_("Find _Duplicates..."), NULL,
	  N_("Find duplicated files in the current location"),
	  G_CALLBACK (gth_browser_activate_action_edit_find_duplicates) }
};
static guint find_action_entries_size = G_N_ELEMENTS (find_action_entries);


typedef struct {
	GtkActionGroup *find_action;
	guint           find_merge_id;
	GtkWidget      *refresh_button;
} BrowserData;


static void
browser_data_free (BrowserData *data)
{
	g_free (data);
}


void
find_dup__gth_browser_construct_cb (GthBrowser *browser)
{
	BrowserData *data;
	GError      *error = NULL;

	g_return_if_fail (GTH_IS_BROWSER (browser));

	data = g_new0 (BrowserData, 1);

	data->find_action = gtk_action_group_new ("Find Duplicates Action");
	gtk_action_group_set_translation_domain (data->find_action, NULL);
	gtk_action_group_add_actions (data->find_action,
				      find_action_entries,
				      find_action_entries_size,
				      browser);
	gtk_ui_manager_insert_action_group (gth_browser_get_ui_manager (browser), data->find_action, 0);

	data->find_merge_id = gtk_ui_manager_add_ui_from_string (gth_browser_get_ui_manager (browser), find_ui_info, -1, &error);
	if (data->find_merge_id == 0) {
		g_warning ("building menus failed: %s", error->message);
		g_error_free (error);
	}

	g_object_set_data_full (G_OBJECT (browser), BROWSER_DATA_KEY, data, (GDestroyNotify) browser_data_free);
}
