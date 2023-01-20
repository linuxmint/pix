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
#include <gthumb.h>
#include "actions.h"
#include "dlg-edit-metadata.h"
#include "gth-delete-metadata-task.h"
#include "gth-edit-comment-dialog.h"
#include "gth-edit-tags-dialog.h"


void
gth_browser_activate_edit_metadata (GSimpleAction	*action,
				    GVariant		*parameter,
				    gpointer		 user_data)
{
	GthBrowser *browser = GTH_BROWSER (user_data);

	dlg_edit_metadata (browser,
			   GTH_TYPE_EDIT_COMMENT_DIALOG,
			   "edit-comment-dialog");
}


void
gth_browser_activate_edit_tags (GSimpleAction	*action,
				GVariant	*parameter,
				gpointer	 user_data)
{
	GthBrowser *browser = GTH_BROWSER (user_data);

	dlg_edit_metadata (browser,
			   GTH_TYPE_EDIT_TAGS_DIALOG,
			   "edit-tags-dialog");
}


void
gth_browser_activate_delete_metadata (GSimpleAction	*action,
				      GVariant		*parameter,
				      gpointer		 user_data)
{
	GthBrowser *browser = GTH_BROWSER (user_data);
	GtkWidget  *dialog;
	int         result;
	GList      *items;
	GList      *file_data_list;
	GList      *file_list;
	GthTask    *task;

	dialog =  gtk_message_dialog_new (GTK_WINDOW (browser),
					  GTK_DIALOG_MODAL,
					  GTK_MESSAGE_QUESTION,
					  GTK_BUTTONS_NONE,
					  _("Are you sure you want to permanently delete the metadata of the selected files?"));
	gtk_dialog_add_buttons (GTK_DIALOG (dialog),
			        _GTK_LABEL_CANCEL, GTK_RESPONSE_CANCEL,
			        _GTK_LABEL_DELETE, GTK_RESPONSE_YES,
			        NULL);
	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
						  "%s",
						  _("If you delete the metadata, it will be permanently lost."));

	result = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);

	if (result != GTK_RESPONSE_YES)
		return;

	items = gth_file_selection_get_selected (GTH_FILE_SELECTION (gth_browser_get_file_list_view (browser)));
	file_data_list = gth_file_list_get_files (GTH_FILE_LIST (gth_browser_get_file_list (browser)), items);
	file_list = gth_file_data_list_to_file_list (file_data_list);
	task = gth_delete_metadata_task_new (browser, file_list);
	gth_browser_exec_task (browser, task, GTH_TASK_FLAGS_DEFAULT);

	g_object_unref (task);
	_g_object_list_unref (file_list);
	_g_object_list_unref (file_data_list);
	_gtk_tree_path_list_free (items);
}
