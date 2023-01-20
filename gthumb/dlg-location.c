/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001-2012 The Free Software Foundation, Inc.
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
#include "dlg-location.h"
#include "glib-utils.h"
#include "gth-browser.h"
#include "gth-file-source-vfs.h"
#include "gth-main.h"
#include "gtk-utils.h"


#define GET_WIDGET(name) _gtk_builder_get_widget (data->builder, (name))
#define UPDATE_DELAY 200


enum {
	COMPLETION_NAME_COLUMN = 0,
	COMPLETION_N_COLUMNS
};


typedef struct _DialogData DialogData;


typedef struct {
	guint          ref_count;
	char          *uri;
	GFile         *folder;
	GthFileSource *source;
	DialogData    *data;
	gboolean       keep_scheme;
	gboolean       expanded_tilde;
} CompletionJob;


struct _DialogData {
	guint               ref_count;
	GthBrowser         *browser;
	GtkBuilder         *builder;
	GtkWidget          *dialog;
	GtkEntryCompletion *completion;
	GtkListStore       *completion_store;
	guint               update_event;
	CompletionJob      *job;
	GFile              *last_folder;
};


static DialogData *
dialog_data_ref (DialogData *data)
{
	data->ref_count++;
	return data;
}


static void completion_job_unref (CompletionJob *job);


static void
dialog_data_unref (DialogData *data)
{
	if (data == NULL)
		return;

	data->ref_count--;
	if (data->ref_count > 0)
		return;

	if (data->update_event != 0)
		g_source_remove (data->update_event);
	data->update_event = 0;
	g_object_unref (data->builder);
	completion_job_unref (data->job);
	_g_object_unref (data->last_folder);
	g_free (data);
}


static CompletionJob *
completion_job_new_for_uri (DialogData *data,
			    const char *uri)
{
	CompletionJob *job = NULL;

	job = g_new0 (CompletionJob, 1);
	job->ref_count = 1;
	job->data = dialog_data_ref (data);
	job->uri = g_strdup (uri);
	job->source = gth_main_get_file_source_for_uri (job->uri);
	job->folder = NULL;
	if (job->source == NULL) {
		completion_job_unref (job);
		job = NULL;
	}

	return job;
}


static void
completion_job_ref (CompletionJob *job)
{
	job->ref_count++;
}


static void
completion_job_unref (CompletionJob *job)
{
	if (job == NULL)
		return;
	job->ref_count--;
	if (job->ref_count > 0)
		return;

	_g_object_unref (job->source);
	job->source = NULL;
	dialog_data_unref (job->data);
	_g_object_unref (job->folder);
	g_free (job->uri);
	g_free (job);
}


static void
destroy_cb (GtkWidget  *widget,
	    DialogData *data)
{
	gth_browser_set_dialog (data->browser, "location", NULL);
	if ((data->job != NULL) && (data->job->source != NULL))
		gth_file_source_cancel (data->job->source);
	dialog_data_unref (data);
}


static void
cancel_button_clicked_cb (GtkWidget  *widget,
			  DialogData *data)
{
	gtk_widget_destroy (data->dialog);
}


static char *
_g_file_get_path_with_tilde (GFile *file)
{
	char       *path;
	const char *home;

	path = g_file_get_path (file);

	home = g_get_home_dir ();
	if ((home != NULL) && g_str_has_prefix (path, home)) {
		char *tmp = _g_path_join ("~", path + strlen (home), NULL);
		g_free (path);
		path = tmp;
	}

	return path;
}


static char *
_expand_tilde (const char *path,
	       gboolean   *expanded)
{
	if (_g_str_equal (path, "~")) {
		if (expanded != NULL) *expanded = TRUE;
		return g_strdup (g_get_home_dir ());
	}
	else if (g_str_has_prefix (path, "~/")) {
		if (expanded != NULL) *expanded = TRUE;
		return _g_path_join (g_get_home_dir (), path + 2, NULL);
	}

	if (expanded != NULL) *expanded = FALSE;
	return g_strdup (path);
}


static char *
get_location_uri (DialogData  *data,
		  gboolean    *has_scheme,
		  gboolean    *expanded_tilde,
		  GError     **error)
{
	char *uri;
	char *scheme;

	uri = _expand_tilde (gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("location_entry"))), expanded_tilde);
	scheme = _g_uri_get_scheme (uri);
	if (scheme == NULL) {
		char *tmp = uri;
		uri = g_filename_to_uri (tmp, NULL, error);
		g_free (tmp);
	}
	if (has_scheme != NULL)
		*has_scheme = (scheme != NULL);

	g_free (scheme);

	return uri;
}


static void
ok_button_clicked_cb (GtkWidget  *widget,
		      DialogData *data)
{
	char   *uri;
	GError *error = NULL;
	GFile  *location;

	uri = get_location_uri (data, NULL, NULL, &error);
	if (uri == NULL) {
		char *title;

		title = g_strdup_printf (_("Could not load the position “%s”"), gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("location_entry"))));
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (GET_WIDGET ("location_dialog")), title, error);

		g_free (title);
		g_clear_error (&error);

		return;
	}

	location = g_file_new_for_uri (uri);
	gth_browser_load_location (data->browser, location);

	g_object_unref (location);
	g_free (uri);
	gtk_widget_destroy (data->dialog);
}


static int
file_data_compare_by_name (gconstpointer  a,
			   gconstpointer  b)
{
	GthFileData *fa = (GthFileData *) a;
	GthFileData *fb = (GthFileData *) b;
	const char  *namea;
	const char  *nameb;
	int          result;

	namea = g_file_info_get_name (fa->info);
	nameb = g_file_info_get_name (fb->info);
	if ((namea == NULL) || (nameb == NULL)) {
		if ((namea == NULL) && (nameb == NULL))
			return 0;
		else if (namea == NULL)
			return -1;
		else
			return 1;
	}
	else {
		char *sa = g_utf8_collate_key_for_filename (namea, -1);
		char *sb = g_utf8_collate_key_for_filename (nameb, -1);
		result = strcmp (sa, sb);

		g_free (sa);
		g_free (sb);
	}

	return result;
}


static void
completion_job_list_ready_cb (GthFileSource *file_source,
			      GList         *files,
			      GError        *error,
			      gpointer       user_data)
{
	CompletionJob *job = user_data;
	DialogData    *data;
	GList         *ordered;
	GList         *scan;

	if (error != NULL) {
		completion_job_unref (job);
		return;
	}

	data = job->data;

	ordered = g_list_copy (files);
	ordered = g_list_sort (ordered, file_data_compare_by_name);

	gtk_list_store_clear (data->completion_store);
	for (scan = ordered; scan; scan = scan->next) {
		GthFileData *file_data = scan->data;

		if (g_file_info_get_file_type (file_data->info) == G_FILE_TYPE_DIRECTORY) {
			GtkTreeIter  iter;
			GFile       *child;
			char        *uri;

			child = g_file_get_child (job->folder, g_file_info_get_name (file_data->info));
			if (job->keep_scheme)
				uri = g_file_get_uri (child);
			else if (job->expanded_tilde)
				uri = _g_file_get_path_with_tilde (child);
			else
				uri = g_file_get_path (child);

			gtk_list_store_append (data->completion_store, &iter);
			gtk_list_store_set (data->completion_store, &iter,
					    COMPLETION_NAME_COLUMN, uri,
					    -1);

			g_free (uri);
			g_object_unref (child);
		}
	}

	g_list_free (ordered);

	if (job == data->job) {
		completion_job_unref (data->job);
		data->job = NULL;
	}
	completion_job_unref (job);
}


static void
update_completion_list (DialogData *data)
{
	gboolean  has_scheme;
	gboolean  expanded_tilde;
	char     *uri;

	uri = get_location_uri (data, &has_scheme, &expanded_tilde, NULL);
	if (uri == NULL) {
		_g_object_unref (data->last_folder);
		data->last_folder = NULL;
		gtk_list_store_clear (data->completion_store);
		return;
	}

	if (data->job != NULL) {
		if (data->job->source != NULL)
			gth_file_source_cancel (data->job->source);
		completion_job_unref (data->job);
	}

	data->job = completion_job_new_for_uri (data, uri);
	if (data->job != NULL) {
		data->job->keep_scheme = has_scheme;
		data->job->expanded_tilde = expanded_tilde;
		data->job->folder = g_file_new_for_uri (uri);
		if (! g_str_has_suffix (uri, "/")) {
			GFile *parent;

			parent = g_file_get_parent (data->job->folder);
			if (parent != NULL) {
				g_object_unref (data->job->folder);
				data->job->folder = parent;
			}
		}

		if ((data->last_folder == NULL) || ! g_file_equal (data->last_folder, data->job->folder)) {
			_g_object_unref (data->last_folder);
			data->last_folder = g_object_ref (data->job->folder);

			completion_job_ref (data->job);
			gth_file_source_list (data->job->source,
					      data->job->folder,
					      "standard::type,standard::name",
					      completion_job_list_ready_cb,
					      data->job);
		}
		else {
			completion_job_unref (data->job);
			data->job = NULL;
		}

	}

	g_free (uri);
}


static gboolean
update_completion_list_cb (gpointer user_data)
{
	DialogData *data = user_data;

	data->update_event = 0;
	update_completion_list (data);

	return FALSE;
}


static void
location_entry_changed_cb (GtkEntry   *entry,
			   GParamSpec *pspec,
			   gpointer    user_data)
{
	DialogData *data = user_data;

	if (data->update_event != 0)
		g_source_remove (data->update_event);
	data->update_event = g_timeout_add (UPDATE_DELAY, update_completion_list_cb, data);
}


void
dlg_location (GthBrowser *browser)
{
	DialogData    *data;
	char          *text;
	GFile         *location;
	GthFileSource *source;

	if (gth_browser_get_dialog (browser, "location") != NULL) {
		gtk_window_present (GTK_WINDOW (gth_browser_get_dialog (browser, "location")));
		return;
	}

	data = g_new0 (DialogData, 1);
	data->ref_count = 1;
	data->browser = browser;
	data->builder = _gtk_builder_new_from_file ("location.ui", NULL);

	data->dialog = g_object_new (GTK_TYPE_DIALOG,
				     "title", _("Open"),
				     "transient-for", GTK_WINDOW (browser),
				     "modal", FALSE,
				     "destroy-with-parent", FALSE,
				     "use-header-bar", _gtk_settings_get_dialogs_use_header (),
				     NULL);
	gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (data->dialog))),
			   _gtk_builder_get_widget (data->builder, "dialog_content"));
	gtk_dialog_add_buttons (GTK_DIALOG (data->dialog),
				_GTK_LABEL_CANCEL, GTK_RESPONSE_CANCEL,
				_("Open"), GTK_RESPONSE_OK,
				NULL);
	_gtk_dialog_add_class_to_response (GTK_DIALOG (data->dialog), GTK_RESPONSE_OK, GTK_STYLE_CLASS_SUGGESTED_ACTION);
	gtk_widget_grab_default (gtk_dialog_get_widget_for_response (GTK_DIALOG (data->dialog), GTK_RESPONSE_OK));
	gth_browser_set_dialog (browser, "location", data->dialog);

	/* set the widget data */

	data->completion = gtk_entry_completion_new ();
	gtk_entry_completion_set_popup_completion (data->completion, TRUE);
	gtk_entry_completion_set_popup_single_match (data->completion, FALSE);
	gtk_entry_completion_set_inline_completion (data->completion, TRUE);
	data->completion_store = gtk_list_store_new (COMPLETION_N_COLUMNS, G_TYPE_STRING);
	gtk_entry_completion_set_model (data->completion, GTK_TREE_MODEL (data->completion_store));
	g_object_unref (data->completion_store);
	gtk_entry_completion_set_text_column (data->completion, COMPLETION_NAME_COLUMN);
	gtk_entry_set_completion (GTK_ENTRY (GET_WIDGET ("location_entry")), data->completion);

	text = NULL;
	location = gth_browser_get_location (browser);
	source = gth_main_get_file_source (location);
	if (GTH_IS_FILE_SOURCE_VFS (source))
		text = g_file_get_path (location);
	if (text == NULL)
		text = g_file_get_uri (location);
	gtk_entry_set_text (GTK_ENTRY (GET_WIDGET ("location_entry")), text);

	g_free (text);
	g_object_unref (source);

	/* set the signals handlers. */

	g_signal_connect (G_OBJECT (data->dialog),
			  "destroy",
			  G_CALLBACK (destroy_cb),
			  data);
	g_signal_connect (gtk_dialog_get_widget_for_response (GTK_DIALOG (data->dialog), GTK_RESPONSE_CANCEL),
			  "clicked",
			  G_CALLBACK (cancel_button_clicked_cb),
			  data);
	g_signal_connect (gtk_dialog_get_widget_for_response (GTK_DIALOG (data->dialog), GTK_RESPONSE_OK),
			  "clicked",
			  G_CALLBACK (ok_button_clicked_cb),
			  data);
	g_signal_connect (GET_WIDGET ("location_entry"),
			  "notify::text",
			  G_CALLBACK (location_entry_changed_cb),
			  data);

	/* run dialog. */

	update_completion_list (data);
	gtk_widget_show (data->dialog);
}
