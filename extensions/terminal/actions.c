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
#include <gthumb.h>
#include "actions.h"
#include "preferences.h"


void
gth_browser_activate_folder_context_open_in_terminal (GSimpleAction *action,
						      GVariant      *parameter,
						      gpointer       user_data)
{
	GthBrowser  *browser = GTH_BROWSER (user_data);
	GthFileData *file_data;
	GList       *file_list;
	GSettings   *settings;
	char        *command;

	file_data = gth_browser_get_folder_popup_file_data (browser);
	if (file_data == NULL) {
		if (GTH_IS_FILE_SOURCE_VFS (gth_browser_get_location_source (browser)))
			file_data = g_object_ref (gth_browser_get_location_data (browser));

		if (file_data == NULL)
			return;
	}
	file_list = g_list_prepend (NULL, file_data->file);

	settings = g_settings_new (GTHUMB_TERMINAL_SCHEMA);
	command = g_settings_get_string (settings, PREF_TERMINAL_COMMAND);
	_g_launch_command (GTK_WIDGET (browser), command, _("Terminal"), G_APP_INFO_CREATE_NONE, file_list);

	g_free (command);
	g_object_unref (settings);
	g_list_free (file_list);
	g_object_unref (file_data);
}
