/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2009 The Free Software Foundation, Inc.
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
#include <gtk/gtk.h>
#include <gthumb.h>
#include "dlg-edit-metadata.h"
#include "gth-edit-metadata-dialog.h"


#define UPDATE_SELECTION_DELAY 300


typedef struct {
	int         ref;
	GthBrowser *browser;
	GtkWidget  *dialog;
	char       *dialog_name;
	GList      *file_list; /* GthFileData list */
	GList      *parents;
	gboolean    never_shown;
	gboolean    close_dialog;
	GthTask    *loader;
	gulong      file_selection_changed_event;
	guint       update_selectection_event;
} DialogData;


static DialogData *
dialog_data_ref (DialogData *data)
{
	g_atomic_int_inc (&data->ref);
	return data;
}


static void
cancel_file_list_loading (DialogData *data)
{
	if (data->loader == NULL)
		return;
	gth_task_cancel (data->loader);
	g_object_unref (data->loader);
	data->loader = NULL;
}


static void
dialog_data_unref (DialogData *data)
{
	if (! g_atomic_int_dec_and_test (&data->ref))
		return;

	if (data->file_selection_changed_event != 0) {
		g_signal_handler_disconnect (gth_browser_get_file_list_view (data->browser),
					     data->file_selection_changed_event);
		data->file_selection_changed_event = 0;
	}
	if (data->update_selectection_event != 0) {
		g_source_remove (data->update_selectection_event);
		data->update_selectection_event = 0;
	}
	cancel_file_list_loading (data);

	gth_browser_set_dialog (data->browser, data->dialog_name, NULL);
	gtk_widget_destroy (data->dialog);

	g_free (data->dialog_name);
	_g_object_list_unref (data->file_list);
	_g_object_list_unref (data->parents);
	g_free (data);
}


static void
close_dialog (DialogData *data)
{
	if (data->file_selection_changed_event != 0) {
		g_signal_handler_disconnect (gth_browser_get_file_list_view (data->browser),
					     data->file_selection_changed_event);
		data->file_selection_changed_event = 0;
	}
	gtk_widget_hide (data->dialog);
	dialog_data_unref (data);
}


static void
saver_completed_cb (GthTask  *task,
		    GError   *error,
		    gpointer  user_data)
{
	DialogData *data = user_data;
	GthMonitor *monitor;
	GList      *scan;

	monitor = gth_main_get_default_monitor ();
	for (scan = data->parents; scan; scan = scan->next)
		gth_monitor_resume (monitor, (GFile *) scan->data);

	if (error != NULL) {
		if (! g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
			_gtk_error_dialog_from_gerror_show (GTK_WINDOW (data->dialog), _("Could not save the file metadata"), error);
	}
	else {
		for (scan = data->file_list; scan; scan = scan->next) {
			GthFileData *file_data = scan->data;
			GList       *files;

			files = g_list_prepend (NULL, g_object_ref (file_data->file));
			gth_monitor_metadata_changed (monitor, file_data);

			_g_object_list_unref (files);
		}
	}

	if (data->close_dialog)
		close_dialog (data);

	dialog_data_unref (data);
}


static void
edit_metadata_dialog__response_cb (GtkDialog *dialog,
				   int        response,
				   gpointer   user_data)
{
	DialogData *data = user_data;
	GthMonitor *monitor;
	GHashTable *parents;
	GList      *scan;
	GthTask    *task;

	if (response == GTK_RESPONSE_HELP) {
		show_help_dialog (GTK_WINDOW (dialog), data->dialog_name);
		return;
	}

	if ((response != GTK_RESPONSE_OK) && (response != GTK_RESPONSE_APPLY)) {
		cancel_file_list_loading (data);
		close_dialog (data);
		return;
	}

	if (data->file_list == NULL)
		return;

	data->close_dialog = (response == GTK_RESPONSE_OK);

	/* get the parents list */

	parents = g_hash_table_new_full (g_file_hash, (GEqualFunc) g_file_equal, g_object_unref, NULL);
	for (scan = data->file_list; scan; scan = scan->next) {
		GthFileData *file_data = scan->data;
		GFile       *parent;

		parent = g_file_get_parent (file_data->file);
		if (G_LIKELY (parent != NULL)) {
			if (g_hash_table_lookup (parents, parent) == NULL)
				g_hash_table_insert (parents, g_object_ref (parent), GINT_TO_POINTER (1));
			g_object_unref (parent);
		}
	}
	_g_object_list_unref (data->parents);
	data->parents = g_hash_table_get_keys (parents);
	g_list_foreach (data->parents, (GFunc) g_object_ref, NULL);
	g_hash_table_unref (parents);

	/* ignore changes to all the parents */

	monitor = gth_main_get_default_monitor ();
	for (scan = data->parents; scan; scan = scan->next)
		gth_monitor_pause (monitor, (GFile *) scan->data);

	gth_edit_metadata_dialog_update_info (GTH_EDIT_METADATA_DIALOG (data->dialog), data->file_list);

	dialog_data_ref (data);
	task = gth_save_file_data_task_new (data->file_list, "*");
	g_signal_connect (task,
			  "completed",
			  G_CALLBACK (saver_completed_cb),
			  data);
	gth_browser_exec_task (data->browser, task, FALSE);

	g_object_unref (task);

}


typedef struct {
	DialogData *data;
	GList      *files;
} LoaderData;


static void
loader_data_free (LoaderData *loader_data)
{
	dialog_data_unref (loader_data->data);
	_g_object_list_unref (loader_data->files);
	g_free (loader_data);
}


static void
loader_completed_cb (GthTask  *task,
		     GError   *error,
		     gpointer  user_data)
{
	LoaderData *loader_data = user_data;
	DialogData *data = loader_data->data;

	if (error != NULL) {
		if (! g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
			_gtk_error_dialog_from_gerror_show (GTK_WINDOW (data->browser), _("Cannot read file information"), error);
		loader_data_free (loader_data);
		if (data->never_shown)
			close_dialog (data);
		return;
	}

	_g_object_list_unref (data->file_list);
	data->file_list = _g_object_list_ref (gth_load_file_data_task_get_result (GTH_LOAD_FILE_DATA_TASK (task)));

	gth_edit_metadata_dialog_set_file_list (GTH_EDIT_METADATA_DIALOG (data->dialog), data->file_list);

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), GTK_WINDOW (data->browser));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), FALSE);
	gtk_window_present (GTK_WINDOW (data->dialog));

	data->never_shown = FALSE;

	loader_data_free (loader_data);
}


static gboolean
update_file_list (gpointer user_data)
{
	DialogData *data = user_data;
	LoaderData *loader_data;
	GList      *items;
	GList      *file_data_list;

	if (data->update_selectection_event != 0) {
		g_source_remove (data->update_selectection_event);
		data->update_selectection_event = 0;
	}

	cancel_file_list_loading (data);

	loader_data = g_new0 (LoaderData, 1);
	loader_data->data = dialog_data_ref (data);

	items = gth_file_selection_get_selected (GTH_FILE_SELECTION (gth_browser_get_file_list_view (data->browser)));
	file_data_list = gth_file_list_get_files (GTH_FILE_LIST (gth_browser_get_file_list (data->browser)), items);
	loader_data->files = gth_file_data_list_to_file_list (file_data_list);

	data->loader = gth_load_file_data_task_new (loader_data->files, "*");
	g_signal_connect (data->loader,
			  "completed",
			  G_CALLBACK (loader_completed_cb),
			  loader_data);
	gth_browser_exec_task (data->browser, data->loader, FALSE);

	_g_object_list_unref (file_data_list);
	_gtk_tree_path_list_free (items);

	return FALSE;
}


static void
file_selection_changed_cb (GthFileSelection *self,
			   DialogData       *data)
{
	if (data->update_selectection_event != 0)
		g_source_remove (data->update_selectection_event);
	data->update_selectection_event = g_timeout_add (UPDATE_SELECTION_DELAY, update_file_list, data);
}


void
dlg_edit_metadata (GthBrowser *browser,
		   GType       dialog_type,
		   const char *dialog_name)
{
	DialogData *data;

	if (gth_browser_get_dialog (browser, dialog_name)) {
		gtk_window_present (GTK_WINDOW (gth_browser_get_dialog (browser, dialog_name)));
		return;
	}

	data = g_new0 (DialogData, 1);
	data->ref = 1;
	data->browser = browser;
	data->dialog = g_object_new (dialog_type, 0);
	data->dialog_name = g_strdup (dialog_name);
	data->never_shown = TRUE;

	gth_browser_set_dialog (browser, data->dialog_name, data->dialog);

	g_signal_connect (G_OBJECT (data->dialog),
			  "delete-event",
			  G_CALLBACK (gtk_true),
			  NULL);
	g_signal_connect (data->dialog,
			  "response",
			  G_CALLBACK (edit_metadata_dialog__response_cb),
			  data);
	data->file_selection_changed_event =
			g_signal_connect (gth_browser_get_file_list_view (data->browser),
					  "file-selection-changed",
					  G_CALLBACK (file_selection_changed_cb),
					  data);

	update_file_list (data);
}
