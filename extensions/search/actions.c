/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2008 Free Software Foundation, Inc.
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
#include <extensions/catalogs/gth-catalog.h>
#include "actions.h"
#include "gth-search-editor-dialog.h"
#include "gth-search-task.h"


static void
search_editor_dialog__response_cb (GtkDialog *dialog,
			           int        response,
			           gpointer   user_data)
{
	GthBrowser *browser = user_data;
	GthSearch  *search;
	GError     *error = NULL;
	GFile      *search_catalog;
	GthTask    *task;

	if (response != GTK_RESPONSE_OK) {
		gtk_widget_destroy (GTK_WIDGET (dialog));
		return;
	}

	search = gth_search_editor_dialog_get_search (GTH_SEARCH_EDITOR_DIALOG (dialog), &error);
	if (search == NULL) {
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (dialog), _("Could not perform the search"), error);
		g_clear_error (&error);
		return;
	}

        search_catalog = gth_catalog_file_from_relative_path (_("Search Result"), ".search");
        task = gth_search_task_new (browser, search, search_catalog);
	gth_browser_exec_task (browser, task, GTH_TASK_FLAGS_FOREGROUND);

	g_object_unref (task);
	g_object_unref (search_catalog);
	g_object_unref (search);
	gtk_widget_destroy (GTK_WIDGET (dialog));
}


void
gth_browser_activate_find (GSimpleAction *action,
			   GVariant      *parameter,
			   gpointer       user_data)
{
	GthBrowser *browser = GTH_BROWSER (user_data);
	GthSearch  *search;
	GtkWidget  *dialog;

	search = gth_search_new ();
	gth_search_set_source (search, gth_browser_get_location (browser), TRUE);

	dialog = gth_search_editor_dialog_new (_("Find"), search, GTK_WINDOW (browser));
	gtk_dialog_add_button (GTK_DIALOG (dialog), _GTK_LABEL_CANCEL, GTK_RESPONSE_CANCEL);
	gtk_dialog_add_button (GTK_DIALOG (dialog), _("_Find"), GTK_RESPONSE_OK);

	_gtk_dialog_add_class_to_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK, GTK_STYLE_CLASS_SUGGESTED_ACTION);

	g_signal_connect (dialog, "response",
			  G_CALLBACK (search_editor_dialog__response_cb),
			  browser);

	gtk_window_set_modal (GTK_WINDOW (dialog), FALSE);
	gtk_window_present (GTK_WINDOW (dialog));
	gth_search_editor_dialog_focus_first_rule (GTH_SEARCH_EDITOR_DIALOG (dialog));

	g_object_unref (search);
}


typedef struct {
	GthBrowser *browser;
	GFile      *file;
} SearchData;


static void
search_data_free (SearchData *search_data)
{
	g_object_unref (search_data->file);
	g_free (search_data);
}


static void
search_update_buffer_ready_cb (void     **buffer,
			       gsize      count,
			       GError    *error,
			       gpointer   user_data)
{
	SearchData *search_data = user_data;
	GError     *local_error = NULL;
	GthSearch  *search;
	GthTask    *task;

	if (error != NULL) {
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (search_data->browser), _("Could not perform the search"), error);
		return;
	}

	search = gth_search_new_from_data (*buffer, count, &local_error);
	if (search == NULL) {
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (search_data->browser), _("Could not perform the search"), local_error);
		g_clear_error (&local_error);
		return;
	}

	task = gth_search_task_new (search_data->browser, search, search_data->file);
	gth_browser_exec_task (search_data->browser, task, GTH_TASK_FLAGS_FOREGROUND);

	g_object_unref (task);
	g_object_unref (search);
	search_data_free (search_data);
}


void
gth_browser_update_search (GthBrowser *browser)
{
	GFile      *location;
	SearchData *search_data;
	GFile      *file;

	location = gth_browser_get_location (browser);

	search_data = g_new0 (SearchData, 1);
	search_data->browser = browser;
	search_data->file = g_file_dup (location);

	file = gth_main_get_gio_file (location);
	_g_file_load_async (file,
			    G_PRIORITY_DEFAULT,
			    NULL,
			    search_update_buffer_ready_cb,
			    search_data);

	g_object_unref (file);
}
