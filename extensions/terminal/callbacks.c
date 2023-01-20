/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2020 Free Software Foundation, Inc.
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


#define BROWSER_DATA_KEY "terminal-browser-data"


static const GActionEntry actions[] = {
	{ "folder-context-open-in-terminal", gth_browser_activate_folder_context_open_in_terminal }
};


static const GthMenuEntry folder_context_open_entries[] = {
	{ N_("Open in Terminal"), "win.folder-context-open-in-terminal" }
};


static const GthShortcut shortcuts[] = {
	{ "folder-context-open-in-terminal", N_("Open in Terminal"), GTH_SHORTCUT_CONTEXT_BROWSER_VIEWER, GTH_SHORTCUT_CATEGORY_FILE_MANAGER, "<Primary>t" },
};


typedef struct {
	guint folder_context_open_id;
} BrowserData;


static void
browser_data_free (BrowserData *data)
{
	g_free (data);
}


void
terminal__gth_browser_construct_cb (GthBrowser *browser)
{
	BrowserData *data;

	g_return_if_fail (GTH_IS_BROWSER (browser));

	data = g_new0 (BrowserData, 1);
	g_object_set_data_full (G_OBJECT (browser),
				BROWSER_DATA_KEY,
				data,
				(GDestroyNotify) browser_data_free);

	g_action_map_add_action_entries (G_ACTION_MAP (browser),
					 actions,
					 G_N_ELEMENTS (actions),
					 browser);
	gth_window_add_shortcuts (GTH_WINDOW (browser),
				  shortcuts,
				  G_N_ELEMENTS (shortcuts));
}


void
terminal__gth_browser_folder_tree_popup_before_cb (GthBrowser    *browser,
						   GthFileSource *file_source,
						   GthFileData   *folder)
{
	BrowserData *data;

	data = g_object_get_data (G_OBJECT (browser), BROWSER_DATA_KEY);
	g_return_if_fail (data != NULL);

	if (GTH_IS_FILE_SOURCE_VFS (file_source)) {
		if (data->folder_context_open_id == 0)
			data->folder_context_open_id =
					gth_menu_manager_append_entries (gth_browser_get_menu_manager (browser, GTH_BROWSER_MENU_MANAGER_FOLDER_OPEN_ACTIONS),
									 folder_context_open_entries,
									 G_N_ELEMENTS (folder_context_open_entries));
		terminal__gth_browser_update_sensitivity_cb (browser);
	}
	else {
		if (data->folder_context_open_id != 0)
			gth_menu_manager_remove_entries (gth_browser_get_menu_manager (browser, GTH_BROWSER_MENU_MANAGER_FOLDER_OPEN_ACTIONS), data->folder_context_open_id);
		data->folder_context_open_id = 0;
	}
}


void
terminal__gth_browser_update_sensitivity_cb (GthBrowser *browser)
{
	BrowserData   *data;
	GthFileData   *folder;

	data = g_object_get_data (G_OBJECT (browser), BROWSER_DATA_KEY);
	g_return_if_fail (data != NULL);

	folder = gth_browser_get_folder_popup_file_data (browser);
	gth_window_enable_action (GTH_WINDOW (browser),
				  "folder-context-open-in-terminal",
				  ((folder != NULL)
				   && _g_file_has_scheme (folder->file, "file")
				   && (g_file_info_get_file_type (folder->info) == G_FILE_TYPE_DIRECTORY)));

	_g_object_unref (folder);
}
