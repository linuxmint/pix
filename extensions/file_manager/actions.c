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
#include "gth-copy-task.h"
#include "gth-duplicate-task.h"
#include "preferences.h"


#define MAX_HISTORY_LENGTH 10


/* -- gth_browser_activate_create_folder -- */


typedef struct {
	GthBrowser *browser;
	GFile      *parent;
} NewFolderData;


static void
new_folder_data_free (NewFolderData *data)
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
		new_folder_data_free (data);
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
		gth_browser_load_location (data->browser, folder);
		new_folder_data_free (data);
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
					 _GTK_LABEL_CANCEL,
					 _("C_reate"));
	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (new_folder_dialog_response_cb),
			  data);

	gtk_widget_show (dialog);
}


void
gth_browser_activate_create_folder (GSimpleAction *action,
				    GVariant      *parameter,
				    gpointer       user_data)
{
	GthBrowser *browser = GTH_BROWSER (user_data);
	GFile      *parent;

	parent = g_object_ref (gth_browser_get_location (browser));
	_gth_browser_create_new_folder (browser, parent);
	g_object_unref (parent);
}


/* -- gth_browser_activate_edit_cut / gth_browser_activate_edit_copy -- */


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
gth_browser_activate_edit_cut (GSimpleAction *action,
			       GVariant      *parameter,
			       gpointer       user_data)
{
	GthBrowser *browser = GTH_BROWSER (user_data);
	GtkWidget  *focused_widget;
	GList      *items;
	GList      *file_list;

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
gth_browser_activate_edit_copy (GSimpleAction *action,
			        GVariant      *parameter,
			        gpointer       user_data)
{
	GthBrowser *browser = GTH_BROWSER (user_data);
	GtkWidget  *focused_widget;
	GList      *items;
	GList      *file_list;

	focused_widget = gtk_window_get_focus (GTK_WINDOW (browser));
	if ((focused_widget != NULL) && GTK_IS_EDITABLE (focused_widget))
		return;

	items = gth_file_selection_get_selected (GTH_FILE_SELECTION (gth_browser_get_file_list_view (browser)));
	file_list = gth_file_list_get_files (GTH_FILE_LIST (gth_browser_get_file_list (browser)), items);
	_gth_browser_clipboard_copy_or_cut (browser, file_list, FALSE);

	_g_object_list_unref (file_list);
	_gtk_tree_path_list_free (items);
}


/* -- gth_browser_activate_edit_paste -- */


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
	PasteData      *paste_data = user_data;
	GthBrowser     *browser = paste_data->browser;
	const char     *raw_data;
	char          **clipboard_data;
	int             i;
	GdkDragAction   actions;
	GtkTreePath    *path;
	int             position;
	GthTask        *task;

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

	actions = gth_file_source_get_drop_actions (paste_data->file_source,
						    paste_data->destination->file,
						    G_FILE (paste_data->files->data));
	if (actions == 0) {
		_gtk_error_dialog_run (GTK_WINDOW (browser),
				       "%s",
				       _("Could not perform the operation"));
		g_strfreev (clipboard_data);
		paste_data_free (paste_data);
		return;
	}

	if (paste_data->cut && ((actions & GDK_ACTION_MOVE) == 0)) {
		GtkWidget *dialog;
		int        response;

		dialog = _gtk_message_dialog_new (GTK_WINDOW (browser),
						  GTK_DIALOG_MODAL,
						  _GTK_ICON_NAME_DIALOG_QUESTION,
						  _("Could not move the files"),
						  _("Files cannot be moved to the current location, as alternative you can choose to copy them."),
						  _GTK_LABEL_CANCEL, GTK_RESPONSE_CANCEL,
						  _("Copy"), GTK_RESPONSE_OK,
						  NULL);
		response = gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);

		if (response == GTK_RESPONSE_CANCEL) {
			g_strfreev (clipboard_data);
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
	gth_browser_exec_task (browser, task, GTH_TASK_FLAGS_DEFAULT);

	g_object_unref (task);
	g_strfreev (clipboard_data);
	paste_data_free (paste_data);
}


void
gth_browser_activate_edit_paste (GSimpleAction *action,
			         GVariant      *parameter,
			         gpointer       user_data)
{
	GthBrowser *browser = GTH_BROWSER (user_data);
	GtkWidget  *focused_widget;
	PasteData  *paste_data;

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
gth_browser_activate_duplicate  (GSimpleAction *action,
				 GVariant      *parameter,
				 gpointer       user_data)
{
	GthBrowser *browser = GTH_BROWSER (user_data);
	GList      *items;
	GList      *file_list;
	GthTask    *task;

	items = gth_file_selection_get_selected (GTH_FILE_SELECTION (gth_browser_get_file_list_view (browser)));
	file_list = gth_file_list_get_files (GTH_FILE_LIST (gth_browser_get_file_list (browser)), items);
	task = gth_duplicate_task_new (file_list);
	gth_browser_exec_task (browser, task, GTH_TASK_FLAGS_DEFAULT);

	g_object_unref (task);
	_g_object_list_unref (file_list);
	_gtk_tree_path_list_free (items);
}


void
gth_browser_activate_trash  (GSimpleAction *action,
			     GVariant      *parameter,
			     gpointer       user_data)
{
	GthBrowser *browser = GTH_BROWSER (user_data);
	GList      *items;
	GList      *file_list;

	items = gth_file_selection_get_selected (GTH_FILE_SELECTION (gth_browser_get_file_list_view (browser)));
	file_list = gth_file_list_get_files (GTH_FILE_LIST (gth_browser_get_file_list (browser)), items);
	gth_file_mananger_trash_files (GTK_WINDOW (browser), file_list);

	_g_object_list_unref (file_list);
	_gtk_tree_path_list_free (items);
}


void
gth_browser_activate_delete (GSimpleAction *action,
			     GVariant      *parameter,
			     gpointer       user_data)
{
	GthBrowser *browser = GTH_BROWSER (user_data);
	GList      *items;
	GList      *file_list;

	items = gth_file_selection_get_selected (GTH_FILE_SELECTION (gth_browser_get_file_list_view (browser)));
	file_list = gth_file_list_get_files (GTH_FILE_LIST (gth_browser_get_file_list (browser)), items);
	gth_file_mananger_delete_files (GTK_WINDOW (browser), file_list);

	_g_object_list_unref (file_list);
	_gtk_tree_path_list_free (items);

}


static void
remove_from_source (GthBrowser *browser,
		    gboolean    permanently)
{
	GthFileSource *source;
	GthFileData   *location;
	GList         *items;
	GList         *file_data_list;

	if (permanently) {
		/* Use the VFS file source to delete the files from the
		 * disk. */

		source = gth_main_get_file_source_for_uri ("file:///");
		location = NULL;
	}
	else {
		/* Removes the files from the current location,
		 * for example: when viewing a catalog removes
		 * the files from the catalog; when viewing a
		 * folder removes the files from the folder. */

		source = _g_object_ref (gth_browser_get_location_source (browser));
		location = gth_browser_get_location_data (browser);
	}

	if (source == NULL)
		return;

	items = gth_file_selection_get_selected (GTH_FILE_SELECTION (gth_browser_get_file_list_view (browser)));
	if (items == NULL)
		return;

	file_data_list = gth_file_list_get_files (GTH_FILE_LIST (gth_browser_get_file_list (browser)), items);
	gth_file_source_remove (source,
				location,
				file_data_list,
				permanently,
				GTK_WINDOW (browser));

	_g_object_list_unref (file_data_list);
	_gtk_tree_path_list_free (items);
	_g_object_unref (source);
}


void
gth_browser_activate_remove_from_source (GSimpleAction *action,
					 GVariant      *parameter,
					 gpointer       user_data)
{
	remove_from_source (GTH_BROWSER (user_data), FALSE);
}


void
gth_browser_activate_remove_from_source_permanently (GSimpleAction *action,
						     GVariant      *parameter,
						     gpointer       user_data)
{
	remove_from_source (GTH_BROWSER (user_data), TRUE);
}


void
gth_browser_activate_rename (GSimpleAction *action,
			     GVariant      *parameter,
			     gpointer       user_data)
{
	GthBrowser *browser = GTH_BROWSER (user_data);
	GtkWidget  *folder_tree;
	GtkWidget  *file_list;

	folder_tree = gth_browser_get_folder_tree (browser);
	if (gtk_widget_has_focus (folder_tree)) {
		GthFileData *file_data;

		file_data = gth_folder_tree_get_selected (GTH_FOLDER_TREE (folder_tree));
		if (file_data == NULL)
			return;
		if (! g_file_info_get_attribute_boolean (file_data->info, G_FILE_ATTRIBUTE_ACCESS_CAN_RENAME))
			return;
		gth_folder_tree_start_editing (GTH_FOLDER_TREE (gth_browser_get_folder_tree (browser)), file_data->file);
		g_object_unref (file_data);

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
gth_browser_activate_file_list_rename (GSimpleAction *action,
				       GVariant      *parameter,
				       gpointer       user_data)
{
	GthBrowser *browser = GTH_BROWSER (user_data);
	gth_hook_invoke ("gth-browser-file-list-rename", browser);
}


/* -- gth_browser_activate_copy_to_folder / gth_browser_activate_move_to_folder -- */


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
	gth_browser_exec_task (browser, task, GTH_TASK_FLAGS_DEFAULT);

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
					      _GTK_LABEL_CANCEL, GTK_RESPONSE_CANCEL,
					      (move ? _("Move") : _("Copy")), GTK_RESPONSE_ACCEPT,
					      NULL);

	_gtk_dialog_add_class_to_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT, GTK_STYLE_CLASS_SUGGESTED_ACTION);

	start_uri = g_settings_get_string (settings, PREF_FILE_MANAGER_COPY_LAST_FOLDER);
	if ((start_uri == NULL) || (strcmp (start_uri, "") == 0)) {
		g_free (start_uri);
		start_uri = g_strdup (_g_uri_get_home ());
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
gth_browser_activate_copy_to_folder (GSimpleAction *action,
				     GVariant      *parameter,
				     gpointer       user_data)
{
	copy_selected_files_to_folder (GTH_BROWSER (user_data), FALSE);
}


void
gth_browser_activate_move_to_folder (GSimpleAction *action,
				     GVariant      *parameter,
				     gpointer       user_data)
{
	copy_selected_files_to_folder (GTH_BROWSER (user_data), TRUE);
}


void
gth_browser_activate_folder_context_open_in_file_manager (GSimpleAction *action,
							  GVariant      *parameter,
							  gpointer       user_data)
{
	GthBrowser  *browser = GTH_BROWSER (user_data);
	GthFileData *file_data;
	char        *uri;
	GError      *error = NULL;

	file_data = gth_browser_get_folder_popup_file_data (browser);
	if (file_data == NULL)
		return;

	uri = g_file_get_uri (file_data->file);
	if (! gtk_show_uri_on_window (GTK_WINDOW (browser),
				      uri,
				      GDK_CURRENT_TIME,
				      &error))
	{
		_gtk_error_dialog_from_gerror_run (GTK_WINDOW (browser), _("Could not open the location"), error);
		g_clear_error (&error);
	}

	g_free (uri);
	g_object_unref (file_data);
}


void
gth_browser_activate_folder_context_create (GSimpleAction *action,
					    GVariant      *parameter,
					    gpointer       user_data)
{
	GthBrowser  *browser = GTH_BROWSER (user_data);
	GthFileData *parent;

	parent = gth_browser_get_folder_popup_file_data (browser);
	if (parent != NULL) {
		_gth_browser_create_new_folder (browser, parent->file);
		g_object_unref (parent);
	}
}


void
gth_browser_activate_folder_context_rename (GSimpleAction *action,
					    GVariant      *parameter,
					    gpointer       user_data)
{
	GthBrowser  *browser = GTH_BROWSER (user_data);
	GthFileData *file_data;

	file_data = gth_browser_get_folder_popup_file_data (browser);
	if (file_data == NULL)
		return;

	if (! g_file_info_get_attribute_boolean (file_data->info, G_FILE_ATTRIBUTE_ACCESS_CAN_RENAME))
		return;

	gth_folder_tree_start_editing (GTH_FOLDER_TREE (gth_browser_get_folder_tree (browser)), file_data->file);

	g_object_unref (file_data);
}


void
gth_browser_activate_folder_context_cut (GSimpleAction *action,
					 GVariant      *parameter,
					 gpointer       user_data)
{
	GthBrowser  *browser = GTH_BROWSER (user_data);
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
gth_browser_activate_folder_context_copy (GSimpleAction *action,
					  GVariant      *parameter,
					  gpointer       user_data)
{
	GthBrowser  *browser = GTH_BROWSER (user_data);
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
gth_browser_activate_folder_context_paste_into_folder (GSimpleAction *action,
						       GVariant      *parameter,
						       gpointer       user_data)
{
	GthBrowser  *browser = GTH_BROWSER (user_data);
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


void
gth_browser_activate_folder_context_trash (GSimpleAction *action,
					   GVariant      *parameter,
					   gpointer       user_data)
{
	GthBrowser  *browser = GTH_BROWSER (user_data);
	GthFileData *file_data;
	GList       *list;

	file_data = gth_browser_get_folder_popup_file_data (browser);
	if (file_data == NULL)
		return;

	list = g_list_append (NULL, file_data);
	gth_file_mananger_trash_files (GTK_WINDOW (browser), list);

	g_list_free (list);
	_g_object_unref (file_data);
}


void
gth_browser_activate_folder_context_delete (GSimpleAction *action,
					    GVariant      *parameter,
					    gpointer       user_data)
{
	GthBrowser  *browser = GTH_BROWSER (user_data);
	GthFileData *file_data;
	GList       *list;

	file_data = gth_browser_get_folder_popup_file_data (browser);
	if (file_data == NULL)
		return;

	list = g_list_append (NULL, file_data);
	gth_file_mananger_delete_files (GTK_WINDOW (browser), list);

	g_list_free (list);
}


/* gth_browser_activate_folder_context_copy_to / gth_browser_activate_folder_context_move_to */


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
gth_browser_activate_folder_context_copy_to (GSimpleAction *action,
					     GVariant      *parameter,
					     gpointer       user_data)
{
	copy_folder_to_folder (GTH_BROWSER (user_data), FALSE);
}


void
gth_browser_activate_folder_context_move_to (GSimpleAction *action,
					     GVariant      *parameter,
					     gpointer       user_data)
{
	copy_folder_to_folder (GTH_BROWSER (user_data), TRUE);
}


void
gth_browser_activate_open_with_gimp (GSimpleAction *action,
				     GVariant      *parameter,
				     gpointer       user_data)
{
	GthBrowser *browser = user_data;
	GList      *items;
	GList      *file_data_list;
	GList      *file_list;

	items = gth_file_selection_get_selected (GTH_FILE_SELECTION (gth_browser_get_file_list_view (browser)));
	if (items == NULL)
		return;

	file_data_list = gth_file_list_get_files (GTH_FILE_LIST (gth_browser_get_file_list (browser)), items);
	file_list = gth_file_data_list_to_file_list (file_data_list);
	_g_launch_command (GTK_WIDGET (browser), "gimp %U", "Gimp", G_APP_INFO_CREATE_SUPPORTS_URIS, file_list);

	_g_object_list_unref (file_list);
	_g_object_list_unref (file_data_list);
	_gtk_tree_path_list_free (items);
}
