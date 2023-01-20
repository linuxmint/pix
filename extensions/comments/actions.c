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
#include <gthumb.h>
#include "actions.h"
#include "gth-import-metadata-task.h"


void
gth_browser_activate_import_embedded_metadata (GSimpleAction	*action,
					       GVariant		*parameter,
					       gpointer	 	 user_data)
{
	GthBrowser	*browser = GTH_BROWSER (user_data);
	GList		*items;
	GList		*file_data_list;
	GthTask		*task;

	items = gth_file_selection_get_selected (GTH_FILE_SELECTION (gth_browser_get_file_list_view (browser)));
	file_data_list = gth_file_list_get_files (GTH_FILE_LIST (gth_browser_get_file_list (browser)), items);
	/* use all the files if no file or only one file is selected */
	if ((file_data_list == NULL) || (file_data_list->next == NULL)) {
		_g_object_list_unref (file_data_list);
		file_data_list = gth_file_store_get_visibles (gth_browser_get_file_store (browser));
	}
	task = gth_import_metadata_task_new (browser, file_data_list);
	gth_browser_exec_task (browser, task, GTH_TASK_FLAGS_DEFAULT);

	g_object_unref (task);
	_g_object_list_unref (file_data_list);
	_gtk_tree_path_list_free (items);
}
