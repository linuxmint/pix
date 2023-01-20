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
#include <gthumb.h>
#include "actions.h"
#include "dlg-personalize-scripts.h"
#include "gth-script.h"
#include "gth-script-file.h"
#include "gth-script-task.h"


void
gth_browser_exec_script (GthBrowser *browser,
			 GthScript  *script)
{
	GList *items;
	GList *file_list;

	items = gth_file_selection_get_selected (GTH_FILE_SELECTION (gth_browser_get_file_list_view (browser)));
	file_list = gth_file_list_get_files (GTH_FILE_LIST (gth_browser_get_file_list (browser)), items);
	if (file_list != NULL) {
		GthTask *task;

		task = gth_script_task_new (GTK_WINDOW (browser), script, file_list);
		gth_browser_exec_task (browser, task, GTH_TASK_FLAGS_DEFAULT);

		g_object_unref (task);
	}

	_g_object_list_unref (file_list);
	_gtk_tree_path_list_free (items);
}


void
gth_browser_activate_exec_script (GSimpleAction *action,
				  GVariant	*parameter,
				  gpointer	 user_data)
{
	GthBrowser *browser = GTH_BROWSER (user_data);
	const char *script_id;
	GthScript  *script;

	script_id = g_variant_get_string (parameter, NULL);
	script = gth_script_file_get_script (gth_script_file_get (), script_id);
	if (script != NULL)
		gth_browser_exec_script (browser, script);
}


void
gth_browser_activate_personalize_tools (GSimpleAction	*action,
					GVariant	*parameter,
					gpointer	 user_data)
{
	dlg_personalize_scripts (GTH_BROWSER (user_data));
}
