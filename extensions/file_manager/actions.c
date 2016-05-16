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
#include "gth-copy-task.h"
#include "gth-delete-task.h"
#include "gth-duplicate-task.h"
#include "preferences.h"


#define MAX_HISTORY_LENGTH 10


typedef struct {
	GthBrowser *browser;
	GFile      *parent;
} NewFolderData;


static void
new_fodler_data_free (NewFolderData *data)
{
	g_object_unref (data->parent);
	g_free (data);
}


static void
new_folder_dialog_response_cb (GtkWidget *dialog,
			       int        response_id,
			       gpointer   user_data)
{
	NewFolderData *data = user_data;
	char          *name;
	GFile         *folder;
	GError        *error = NULL;

	if (response_id != GTK_RESPONSE_OK) {
		new_fodler_data_free (data);
		gtk_widget_destroy (dialog);
		return;
	}

	name = gth_request_dialog_get_normalized_text (GTH_REQUEST_DIALOG (dialog));
	if (_g_utf8_all_spaces (name)) {
		g_free (name);
		gth_request_dialog_set_info_text (GTH_REQUEST_DIALOG (dialog), GTK_MESSAGE_ERROR, _("No name specified"));
		return;
	}

	if (g_regex_match_simple ("/", name, 0, 0)) {
		char *message;

		message = g_strdup_printf (_("Invalid name. The following characters are not allowed: %s"), "/");
		gth_request_dialog_set_info_text (GTH_REQUEST_DIALOG (dialog), GTK_MESSAGE_ERROR, message);

		g_free (message);
		g_free (name);

		return;
	}

	folder = g_file_get_child_for_display_name (data->parent, name, &error);
	if ((folder != NULL) && g_file_make_directory (folder, NULL, &error)) {
		GList       *list;
		GtkWidget   *folder_tree;
		GtkTreePath *path;

		list = g_list_prepend (NULL, folder);
		gth_monitor_folder_changed (gth_main_get_default_monitor (),
					    data->parent,
					    list,
					    GTH_MONITOR_EVENT_CREATED);

		folder_tree = gth_browser_get_folder_tree (data->browser);
		path = gth_folder_tree_get_path (GTH_FOLDER_TREE (folder_tree), data->parent);
		gth_folder_tree_expand_row (GTH_FOLDER_TREE (folder_tree), path, FALSE);

		gtk_tree_path_free (path);
		g_list_free (list);
	}

	if (error != NULL) {
		if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_EXISTS))
			gth_request_dialog_set_info_text (GTH_REQUEST_DIALOG (dialog), GTK_MESSAGE_ERROR, _("Name already used"));
		else
			gth_request_dialog_set_info_text (GTH_REQUEST_DIALOG (dialog), GTK_MESSAGE_ERROR, error->message);
		g_clear_error (&error);
	}
	else {
		new_fodler_data_free (data);
		gtk_widget_destroy (dialog);
	}

	g_object_unref (folder);
}


static void
_gth_browser_create_new_folder (GthBrowser *browser,
				GFile      *parent)
{
	NewFolderData *data;
	GtkWidget     *dialog;

	data = g_new0 (NewFolderData, 1);
	data->browser = browser;
	data->parent = g_object_ref (parent);

	dialog = gth_request_dialog_new (GTK_WINDOW (browser),
					 GTK_DIALOG_MODAL,
					 _("New folder"),
					 _("Enter the folder name:"),
					 GTK_STOCK_CANCEL,
					 _("C_reate"));
	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (new_folder_dialog_response_cb),
			  data);

	gtk_widget_show (dialog);
}


void
gth_browser_action_new_folder (GtkAction  *action,
			       GthBrowser *browser)
{
	GFile *parent;

	parent = g_object_ref (gth_browser_get_location (browser));
	_gth_browser_create_new_folder (browser, parent);
	g_object_unref (parent);
}


/* -- gth_browser_clipboard_copy / gth_browser_clipboard_cut -- */


typedef struct {
	char     **uris;
	int        n_uris;
	gboolean   cut;
} ClipboardData;


static char *
clipboard_data_convert_to_text (ClipboardData *clipboard_data,
				gboolean       formatted,
				gsize         *len)
{
	GString *uris;
	int      i;

	if (formatted)
		uris = g_string_new (clipboard_data->cut ? "cut" : "copy");
	else
		uris = g_string_new (NULL);

	for (i = 0; i < clipboard_data->n_uris; i++) {
		if (formatted) {
			g_string_append_c (uris, '\n');
			g_string_append (uris, clipboard_data->uris[i]);
		}
		else {
			GFile *file;
			char  *name;

			if (i > 0)
				g_string_append_c (uris, '\n');
			file = g_file_new_for_uri (clipboard_data->uris[i]);
			name = g_file_get_parse_name (file);
			g_string_append (uris, name);

			g_free (name);
			g_object_unref (file);
		}
	}

	if (len != NULL)
		*len = uris->len;

	return g_string_free (uris, FALSE);
}


static void
clipboard_get_cb (GtkClipboard     *clipboard,
		  GtkSelectionData *selection_data,
		  guint             info,
		  gpointer          user_data_or_owner)
{
	ClipboardData *clipboard_data = user_data_or_owner;
	GdkAtom        targets[1];
	int            n_targets;

	targets[0] = gtk_selection_data_get_target (selection_data);
	n_targets = 1;

	if (gtk_targets_include_uri (targets, n_targets)) {
		gtk_selection_data_set_uris (selection_data, clipboard_data->uris);
	}
	else if (gtk_targets_include_text (targets, n_targets)) {
		char  *str;
		gsize  len;

		str = clipboard_data_convert_to_text (clipboard_data, FALSE, &len);
		gtk_selection_data_set_text (selection_data, str, len);
		g_free (str);
	}
	else if (gtk_selection_data_get_target (selection_data) == GNOME_COPIED_FILES) {
		char  *str;
		gsize  len;

		str = clipboard_data_convert_to_text (clipboard_data, TRUE, &len);
		gtk_selection_data_set (selection_data, GNOME_COPIED_FILES, 8, (guchar *) str, len);
		g_free (str);
	}
}


static void
clipboard_clear_cb (GtkClipboard *clipboard,
		    gpointer      user_data_or_owner)
{
	ClipboardData *data = user_data_or_owner;

	g_strfreev (data->uris);
	g_free (data);
}


static void
_gth_browser_clipboard_copy_or_cut (GthBrowser *browser,
				    GList      *file_list,
				    gboolean    cut)
{
	ClipboardData  *data;
	GtkTargetList  *target_list;
	GtkTargetEntry *targets;
	int             n_targets;
	GList          *scan;
	int             i;

	data = g_new0 (ClipboardData, 1);
	data->cut = cut;
	data->n_uris = g_list_length (file_list);
	data->uris = g_new (char *, data->n_uris + 1);
	for (scan = file_list, i = 0; scan; scan = scan->next, i++) {
		GthFileData *file_data = scan->data;
		data->uris[i] = g_file_get_uri (file_data->file);
	}
	data->uris[data->n_uris] = NULL;

	target_list = gtk_target_list_new (NULL, 0);
	gtk_target_list_add (target_list, GNOME_COPIED_FILES, 0, 0);
	gtk_target_list_add_uri_targets (target_list, 0);
	gtk_target_list_add_text_targets (target_list, 0);
	targets = gtk_target_table_new_from_list (target_list, &n_targets);
	gtk_clipboard_set_with_data (gtk_clipboard_get_for_display (gtk_widget_get_display (GTK_WIDGET (browser)), GDK_SELECTION_CLIPBOARD),
				     targets,
				     n_targets,
				     clipboard_get_cb,
				     clipboard_clear_cb,
				     data);

	gtk_target_list_unref (target_list);
	gtk_target_table_free (targets, n_targets);
}


void
gth_browser_activate_action_edit_cut_files (GtkAction  *action,
					    GthBrowser *browser)
{
	GtkWidget *focused_widget;
	GList     *items;
	GList     *file_list;

	focused_widget = gtk_window_get_focus (GTK_WINDOW (browser));
	if ((focused_widget != NULL) && GTK_IS_EDITABLE (focused_widget))
		return;

	items = gth_file_selection_get_selected (GTH_FILE_SELECTION (gth_browser_get_file_list_view (browser)));
	file_list = gth_file_list_get_files (GTH_FILE_LIST (gth_browser_get_file_list (browser)), items);
	_gth_browser_clipboard_copy_or_cut (browser, file_list, TRUE);

	_g_object_list_unref (file_list);
	_gtk_tree_path_list_free (items);
}


void
gth_browser_activate_action_edit_copy_files (GtkAction  *action,
					     GthBrowser *browser)
{
	GtkWidget *focused_widget;
	GList     *items;
	GList     *file_list;

	focused_widget = gtk_window_get_focus (GTK_WINDOW (browser));
	if ((focused_widget != NULL) && GTK_IS_EDITABLE (focused_widget))
		return;

	items = gth_file_selection_get_selected (GTH_FILE_SELECTION (gth_browser_get_file_list_view (browser)));
	file_list = gth_file_list_get_files (GTH_FILE_LIST (gth_browser_get_file_list (browser)), items);
	_gth_browser_clipboard_copy_or_cut (browser, file_list, FALSE);

	_g_object_list_unref (file_list);
	_gtk_tree_path_list_free (items);
}


/* -- gth_browser_clipboard_paste -- */


typedef struct {
	GthBrowser    *browser;
	GthFileData   *destination;
	GthFileSource *file_source;
	GList         *files;
	gboolean       cut;
} PasteData;


static void
paste_data_free (PasteData *paste_data)
{
	_g_object_list_unref (paste_data->files);
	_g_object_unref (paste_data->file_source);
	g_object_unref (paste_data->destination);
	g_object_unref (paste_data->browser);
	g_free (paste_data);
}


static void
clipboard_received_cb (GtkClipboard     *clipboard,
		       GtkSelectionData *selection_data,
		       gpointer          user_data)
{
	PasteData   *paste_data = user_data;
	GthBrowser  *browser = paste_data->browser;
	const char  *raw_data;
	char       **clipboard_data;
	int          i;
	GtkTreePath *path;
	int          position;
	GthTask     *task;

	raw_data = (const char *) gtk_selection_data_get_data (selection_data);
	if (raw_data == NULL) {
		paste_data_free (paste_data);
		return;
	}

	clipboard_data = g_strsplit_set (raw_data, "\n\r", -1);
	if ((clipboard_data == NULL) || (clipboard_data[0] == NULL)) {
		g_strfreev (clipboard_data);
		paste_data_free (paste_data);
		return;
	}

	paste_data->cut = strcmp (clipboard_data[0], "cut") == 0;
	paste_data->files = NULL;
	for (i = 1; clipboard_data[i] != NULL; i++)
		if (strcmp (clipboard_data[i], "") != 0)
			paste_data->files = g_list_prepend (paste_data->files, g_file_new_for_uri (clipboard_data[i]));
	paste_data->files = g_list_reverse (paste_data->files);
	paste_data->file_source = gth_main_get_file_source (paste_data->destination->file);

	if (paste_data->cut && ! gth_file_source_can_cut (paste_data->file_source, paste_data->files->data)) {
		GtkWidget *dialog;
		int        response;

		dialog = _gtk_message_dialog_new (GTK_WINDOW (browser),
						  GTK_DIALOG_MODAL,
						  GTK_STOCK_DIALOG_QUESTION,
						  _("Could not move the files"),
						  _("Files cannot be moved to the current location, as alternative you can choose to copy them."),
						  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						  GTK_STOCK_COPY, GTK_RESPONSE_OK,
						  NULL);
		response = gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);

		if (response == GTK_RESPONSE_CANCEL) {
			paste_data_free (paste_data);
			return;
		}

		paste_data->cut = FALSE;
	}

	position = -1;
	path = gth_file_selection_get_last_selected (GTH_FILE_SELECTION (gth_browser_get_file_list_view (browser)));
	if (path != NULL) {
		int *indices;

		indices = gtk_tree_path_get_indices (path);
		if (indices != NULL)
			position = indices[0] + 1;
		gtk_tree_path_free (path);
	}

	task = gth_copy_task_new (paste_data->file_source,
				  paste_data->destination,
				  paste_data->cut,
				  paste_data->files,
				  position);
	gth_browser_exec_task (browser, task, FALSE);

	g_object_unref (task);
	paste_data_free (paste_data);
}


void
gth_browser_activate_action_edit_paste (GtkAction  *action,
					GthBrowser *browser)
{
	GtkWidget *focused_widget;
	PasteData *paste_data;

	focused_widget = gtk_window_get_focus (GTK_WINDOW (browser));
	if ((focused_widget != NULL) && GTK_IS_EDITABLE (focused_widget))
		return;

	paste_data = g_new0 (PasteData, 1);
	paste_data->browser = g_object_ref (browser);
	paste_data->destination = g_object_ref (gth_browser_get_location_data (browser));

	gtk_clipboard_request_contents (gtk_widget_get_clipboard (GTK_WIDGET (browser), GDK_SELECTION_CLIPBOARD),
					GNOME_COPIED_FILES,
					clipboard_received_cb,
					paste_data);
}


void
gth_browser_activate_action_edit_duplicate (GtkAction  *action,
					    GthBrowser *browser)
{
	GList   *items;
	GList   *file_list;
	GthTask *task;

	items = gth_file_selection_get_selected (GTH_FILE_SELECTION (gth_browser_get_file_list_view (browser)));
	file_list = gth_file_list_get_files (GTH_FILE_LIST (gth_browser_get_file_list (browser)), items);
	task = gth_duplicate_task_new (file_list);
	gth_browser_exec_task (browser, task, FALSE);

	g_object_unref (task);
	_g_object_list_unref (file_list);
	_gtk_tree_path_list_free (items);
}


void
gth_browser_activate_action_edit_trash (GtkAction  *action,
					GthBrowser *browser)
{
	GList *items;
	GList *file_list;

	items = gth_file_selection_get_selected (GTH_FILE_SELECTION (gth_browser_get_file_list_view (browser)));
	file_list = gth_file_list_get_files (GTH_FILE_LIST (gth_browser_get_file_list (browser)), items);
	gth_file_mananger_trash_files (GTK_WINDOW (browser), file_list);

	_g_object_list_unref (file_list);
	_gtk_tree_path_list_free (items);
}


void
gth_browser_activate_action_edit_delete (GtkAction  *action,
					 GthBrowser *browser)
{
	GList *items;
	GList *file_list;

	items = gth_file_selection_get_selected (GTH_FILE_SELECTION (gth_browser_get_file_list_view (browser)));
	file_list = gth_file_list_get_files (GTH_FILE_LIST (gth_browser_get_file_list (browser)), items);
	gth_file_mananger_delete_files (GTK_WINDOW (browser), file_list);

	_g_object_list_unref (file_list);
	_gtk_tree_path_list_free (items);
}


void
gth_browser_activate_action_edit_rename (GtkAction  *action,
					 GthBrowser *browser)
{
	GtkWidget *folder_tree;
	GtkWidget *file_list;

	folder_tree = gth_browser_get_folder_tree (browser);
	if (gtk_widget_has_focus (folder_tree)) {
		GthFileData *file_data;

		file_data = gth_folder_tree_get_selected (GTH_FOLDER_TREE (folder_tree));
		if ((file_data != NULL) && g_file_info_get_attribute_boolean (file_data->info, G_FILE_ATTRIBUTE_ACCESS_CAN_RENAME))
			gth_folder_tree_start_editing (GTH_FOLDER_TREE (folder_tree), file_data->file);

		_g_object_unref (file_data);

		return;
	}

	file_list = gth_browser_get_file_list_view (browser);
	if (gtk_widget_has_focus (file_list)) {
		gth_hook_invoke ("gth-browser-file-list-rename", browser);
		return;
	}

	if (gth_window_get_current_page (GTH_WINDOW (browser)) == GTH_BROWSER_PAGE_VIEWER) {
		gth_hook_invoke ("gth-browser-file-list-rename", browser);
		return;
	}
}


void
gth_browser_activate_action_folder_open_in_file_manager (GtkAction  *action,
						         GthBrowser *browser)
{
	GthFileData *file_data;
	char        *uri;
	GError      *error = NULL;

	file_data = gth_browser_get_folder_popup_file_data (browser);
	if (file_data == NULL)
		return;

	uri = g_file_get_uri (file_data->file);
	if (! gtk_show_uri (gtk_window_get_screen (GTK_WINDOW (browser)),
			    uri,
                            gtk_get_current_event_time (),
                            &error))
	{
		_gtk_error_dialog_from_gerror_run (GTK_WINDOW (browser), _("Could not open the location"), error);
		g_clear_error (&error);
	}

	g_free (uri);
	g_object_unref (file_data);
}


void
gth_browser_activate_action_folder_create (GtkAction  *action,
					   GthBrowser *browser)
{
	GthFileData *parent;

	parent = gth_browser_get_folder_popup_file_data (browser);
	if (parent != NULL) {
		_gth_browser_create_new_folder (browser, parent->file);
		g_object_unref (parent);
	}
}


void
gth_browser_activate_action_folder_rename (GtkAction  *action,
					   GthBrowser *browser)
{
	GthFileData *file_data;

	file_data = gth_browser_get_folder_popup_file_data (browser);
	if (file_data == NULL)
		return;

	gth_folder_tree_start_editing (GTH_FOLDER_TREE (gth_browser_get_folder_tree (browser)), file_data->file);

	g_object_unref (file_data);
}


void
gth_browser_activate_action_folder_cut (GtkAction  *action,
					GthBrowser *browser)
{
	GthFileData *file_data;
	GList       *file_list;

	file_data = gth_browser_get_folder_popup_file_data (browser);
	if (file_data == NULL)
		return;

	file_list = g_list_prepend (NULL, file_data);
	_gth_browser_clipboard_copy_or_cut (browser, file_list, TRUE);

	g_list_free (file_list);
}


void
gth_browser_activate_action_folder_copy (GtkAction  *action,
					 GthBrowser *browser)
{
	GthFileData *file_data;
	GList       *file_list;

	file_data = gth_browser_get_folder_popup_file_data (browser);
	if (file_data == NULL)
		return;

	file_list = g_list_prepend (NULL, file_data);
	_gth_browser_clipboard_copy_or_cut (browser, file_list, FALSE);

	_g_object_list_unref (file_list);
}


void
gth_browser_activate_action_folder_paste (GtkAction  *action,
					  GthBrowser *browser)
{
	GthFileData *file_data;
	PasteData   *paste_data;

	file_data = gth_browser_get_folder_popup_file_data (browser);
	if (file_data == NULL)
		return;

	paste_data = g_new0 (PasteData, 1);
	paste_data->browser = g_object_ref (browser);
	paste_data->destination = gth_file_data_dup (file_data);

	gtk_clipboard_request_contents (gtk_widget_get_clipboard (GTK_WIDGET (browser), GDK_SELECTION_CLIPBOARD),
					GNOME_COPIED_FILES,
					clipboard_received_cb,
					paste_data);

	g_object_unref (file_data);
}


/* -- gth_browser_activate_action_folder_trash -- */


typedef struct {
	GthBrowser  *browser;
	GthFileData *file_data;
} DeleteFolderData;


static void
delete_data_free (DeleteFolderData *delete_data)
{
	_g_object_unref (delete_data->browser);
	_g_object_unref (delete_data->file_data);
	g_free (delete_data);
}


static void
delete_folder_permanently (GtkWindow        *window,
			   DeleteFolderData *delete_data)
{
	GthFileData *file_data = delete_data->file_data;
	GError      *error = NULL;
	GList       *files;

	files = g_list_prepend (NULL, file_data->file);
	if (! _g_delete_files (files, TRUE, &error)) {
		if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_NOT_EMPTY)) {
			GtkWidget *d;
			int        response;

			d = _gtk_yesno_dialog_new (GTK_WINDOW (delete_data->browser),
					           GTK_DIALOG_MODAL,
					           _("The folder is not empty, do you want to delete the folder and its content permanently?"),
					           GTK_STOCK_CANCEL,
					           GTK_STOCK_DELETE);
			response = gtk_dialog_run (GTK_DIALOG (d));
			if (response == GTK_RESPONSE_YES) {
				GthTask *task;

				task = gth_delete_task_new (files);
				gth_browser_exec_task (delete_data->browser, task, FALSE);

				g_object_unref (task);
			}

			gtk_widget_destroy (d);
		}
		else {
			_gtk_error_dialog_from_gerror_show (window, _("Could not delete the folder"), error);
			g_clear_error (&error);
		}
	}
	else {
		GFile *parent;

		parent = g_file_get_parent (file_data->file);
		gth_monitor_folder_changed (gth_main_get_default_monitor (),
					    parent,
					    files,
					    GTH_MONITOR_EVENT_DELETED);

		g_object_unref (parent);
	}

	g_list_free (files);
	delete_data_free (delete_data);
}


static void
delete_folder_permanently_response_cb (GtkDialog *dialog,
				       int        response_id,
				       gpointer   user_data)
{
	DeleteFolderData *delete_data = user_data;

	gtk_widget_destroy (GTK_WIDGET (dialog));

	if (response_id == GTK_RESPONSE_YES)
		delete_folder_permanently (GTK_WINDOW (delete_data->browser), delete_data);
	else
		delete_data_free (delete_data);
}


void
gth_browser_activate_action_folder_trash (GtkAction  *action,
					  GthBrowser *browser)
{
	GthFileData *file_data;
	GError      *error = NULL;

	file_data = gth_browser_get_folder_popup_file_data (browser);
	if (file_data == NULL)
		return;

	if (! g_file_trash (file_data->file, NULL, &error)) {
		if (g_error_matches (error, G_IO_ERROR,  G_IO_ERROR_NOT_SUPPORTED)) {
			DeleteFolderData *delete_data;
			GtkWidget       *d;

			g_clear_error (&error);

			delete_data = g_new0 (DeleteFolderData, 1);
			delete_data->browser = g_object_ref (browser);
			delete_data->file_data = g_object_ref (file_data);

			d = _gtk_yesno_dialog_new (GTK_WINDOW (browser),
						   GTK_DIALOG_MODAL,
						   _("The folder cannot be moved to the Trash. Do you want to delete it permanently?"),
						   GTK_STOCK_CANCEL,
						   GTK_STOCK_DELETE);
			g_signal_connect (d, "response", G_CALLBACK (delete_folder_permanently_response_cb), delete_data);
			gtk_widget_show (d);
		}
		else {
			_gtk_error_dialog_from_gerror_show (GTK_WINDOW (browser), _("Could not move the folder to the Trash"), error);
			g_clear_error (&error);
		}
	}
	else {
		GFile *parent;
		GList *files;

		parent = g_file_get_parent (file_data->file);
		files = g_list_prepend (NULL, file_data->file);
		gth_monitor_folder_changed (gth_main_get_default_monitor (),
					    parent,
					    files,
					    GTH_MONITOR_EVENT_DELETED);

		g_list_free (files);
		g_object_unref (parent);
	}

	_g_object_unref (file_data);
}


void
gth_browser_activate_action_folder_delete (GtkAction  *action,
					   GthBrowser *browser)
{
	GthFileData      *file_data;
	char             *prompt;
	DeleteFolderData *delete_data;
	GtkWidget        *d;

	file_data = gth_browser_get_folder_popup_file_data (browser);
	if (file_data == NULL)
		return;

	prompt = g_strdup_printf (_("Are you sure you want to permanently delete \"%s\"?"), g_file_info_get_display_name (file_data->info));

	delete_data = g_new0 (DeleteFolderData, 1);
	delete_data->browser = g_object_ref (browser);
	delete_data->file_data = g_object_ref (file_data);

	d = _gtk_message_dialog_new (GTK_WINDOW (browser),
				     GTK_DIALOG_MODAL,
				     GTK_STOCK_DIALOG_QUESTION,
				     prompt,
				     _("If you delete a file, it will be permanently lost."),
				     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				     GTK_STOCK_DELETE, GTK_RESPONSE_YES,
				     NULL);
	g_signal_connect (d, "response", G_CALLBACK (delete_folder_permanently_response_cb), delete_data);
	gtk_widget_show (d);

	g_free (prompt);
}


/* Copy/Move to folder */


typedef struct {
	GthBrowser *browser;
	gboolean    move;
	GFile      *destination;
	gboolean    view_destination;
} CopyToFolderData;


static void
copy_to_folder_data_free (CopyToFolderData *data)
{
	g_object_unref (data->destination);
	g_object_unref (data->browser);
	g_free (data);
}


static void
copy_complete_cb (GthTask  *task,
		  GError   *error,
		  gpointer  user_data)
{
	CopyToFolderData *data = user_data;

	if ((error == NULL) && (data->view_destination))
		gth_browser_load_location (data->browser, data->destination);

	g_object_unref (task);
	copy_to_folder_data_free (data);
}


static void
copy_files_to_folder (GthBrowser *browser,
		      GList      *files,
		      gboolean    move,
		      char       *destination_uri,
		      gboolean    view_destination)
{
	GthFileData      *destination_data;
	GthFileSource    *file_source;
	CopyToFolderData *data;
	GthTask          *task;

	destination_data = gth_file_data_new_for_uri (destination_uri, NULL);
	file_source = gth_main_get_file_source (destination_data->file);

	data = g_new0 (CopyToFolderData, 1);
	data->browser = g_object_ref (browser);
	data->move = move;
	data->destination = g_file_dup (destination_data->file);
	data->view_destination = view_destination;

	task = gth_copy_task_new (file_source, destination_data, move, files, -1);
	g_signal_connect (task,
			  "completed",
			  G_CALLBACK (copy_complete_cb),
			  data);
	gth_browser_exec_task (browser, task, FALSE);

	g_object_unref (file_source);
}


static void
copy_to_folder_dialog (GthBrowser *browser,
		       GList      *files,
		       gboolean    move)
{
	GSettings *settings;
	GtkWidget *dialog;
	char      *start_uri;
	GList     *history;
	GList     *scan;
	GtkWidget *box;
	GtkWidget *view_destination_button;

	settings = g_settings_new (GTHUMB_FILE_MANAGER_SCHEMA);

	dialog = gtk_file_chooser_dialog_new (move ? _("Move To") : _("Copy To"),
					      GTK_WINDOW (browser),
					      GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
					      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					      (move ? _("Move") : _("Copy")), GTK_RESPONSE_ACCEPT,
					      NULL);

	start_uri = g_settings_get_string (settings, PREF_FILE_MANAGER_COPY_LAST_FOLDER);
	if ((start_uri == NULL) || (strcmp (start_uri, "") == 0)) {
		g_free (start_uri);
		start_uri = g_strdup (get_home_uri ());
	}
	gtk_file_chooser_set_uri (GTK_FILE_CHOOSER (dialog), start_uri);
	g_free(start_uri);

	history = _g_settings_get_string_list (settings, PREF_FILE_MANAGER_COPY_HISTORY);
	for (scan = history; scan; scan = scan->next) {
		char *uri = scan->data;
		gtk_file_chooser_add_shortcut_folder_uri (GTK_FILE_CHOOSER (dialog), uri, NULL);
	}

	box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
	gtk_container_set_border_width (GTK_CONTAINER (box), 6);
	gtk_widget_show (box);

	view_destination_button = gtk_check_button_new_with_mnemonic (_("_View the destination"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (view_destination_button),
				      g_settings_get_boolean (settings, PREF_FILE_MANAGER_COPY_VIEW_DESTINATION));
	gtk_widget_show (view_destination_button);
	gtk_box_pack_start (GTK_BOX (box), view_destination_button, FALSE, FALSE, 0);

	gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), box, FALSE, FALSE, 0);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
		char *destination_uri;

		destination_uri = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (dialog));
		if (destination_uri != NULL) {
			gboolean view_destination;

			/* save the options */

			view_destination = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (view_destination_button));
			g_settings_set_boolean (settings, PREF_FILE_MANAGER_COPY_VIEW_DESTINATION, view_destination);
			g_settings_set_string (settings, PREF_FILE_MANAGER_COPY_LAST_FOLDER, destination_uri);

			/* save the destination in the history list, prevent
			 * the list from growing without limit.  */

			history = g_list_prepend (history, g_strdup (destination_uri));
			while (g_list_length (history) > MAX_HISTORY_LENGTH) {
				GList *link = g_list_last (history);
				history = g_list_remove_link (history, link);
				_g_string_list_free (link);
			}
			_g_settings_set_string_list (settings, PREF_FILE_MANAGER_COPY_HISTORY, history);

			/* copy / move the files */

			copy_files_to_folder (browser, files, move, destination_uri, view_destination);
		}

		g_free (destination_uri);
	}

	_g_string_list_free (history);
	gtk_widget_destroy (dialog);
	g_object_unref (settings);
}


static void
copy_selected_files_to_folder (GthBrowser *browser,
			       gboolean    move)
{
	GList *items;
	GList *file_list;
	GList *files;

	items = gth_file_selection_get_selected (GTH_FILE_SELECTION (gth_browser_get_file_list_view (browser)));
	file_list = gth_file_list_get_files (GTH_FILE_LIST (gth_browser_get_file_list (browser)), items);
	files = gth_file_data_list_to_file_list (file_list);

	copy_to_folder_dialog (browser, files, move);

	_g_object_list_unref (files);
	_g_object_list_unref (file_list);
	_gtk_tree_path_list_free (items);
}


void
gth_browser_activate_action_tool_copy_to_folder (GtkAction  *action,
						 GthBrowser *browser)
{
	copy_selected_files_to_folder (browser, FALSE);
}


void
gth_browser_activate_action_tool_move_to_folder (GtkAction  *action,
						 GthBrowser *browser)
{
	copy_selected_files_to_folder (browser, TRUE);
}


static void
copy_folder_to_folder (GthBrowser *browser,
		       gboolean    move)
{
	GthFileData *file_data;
	GList       *files;

	file_data = gth_browser_get_folder_popup_file_data (browser);
	if (file_data == NULL)
		return;

	files = g_list_prepend (NULL, g_object_ref (file_data->file));
	copy_to_folder_dialog (browser, files, move);

	_g_object_list_unref (files);
}


void
gth_browser_activate_action_folder_copy_to_folder (GtkAction  *action,
						   GthBrowser *browser)
{
	copy_folder_to_folder (browser, FALSE);
}


void
gth_browser_activate_action_folder_move_to_folder (GtkAction  *action,
						   GthBrowser *browser)
{
	copy_folder_to_folder (browser, TRUE);
}
