/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2010 The Free Software Foundation, Inc.
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
#include <extensions/oauth/oauth.h>
#include "dlg-export-to-photobucket.h"
#include "photobucket-account.h"
#include "photobucket-album-properties-dialog.h"
#include "photobucket-consumer.h"
#include "photobucket-service.h"


#define GET_WIDGET(x) (_gtk_builder_get_widget (data->builder, (x)))
#define _OPEN_IN_BROWSER_RESPONSE 1


enum {
	ACCOUNT_DATA_COLUMN,
	ACCOUNT_NAME_COLUMN
};


enum {
	ALBUM_DATA_COLUMN,
	ALBUM_ICON_COLUMN,
	ALBUM_TITLE_COLUMN,
	ALBUM_N_PHOTOS_COLUMN
};


enum {
	RESIZE_NAME_COLUMN,
	RESIZE_SIZE_COLUMN
};


typedef struct {
	GthBrowser          *browser;
	GthFileData         *location;
	GList               *file_list;
	GtkBuilder          *builder;
	GtkWidget           *dialog;
	GtkWidget           *progress_dialog;
	GtkWidget           *list_view;
	PhotobucketService  *service;
	GList               *albums;
	PhotobucketAlbum    *album;
	GCancellable        *cancellable;
} DialogData;


static void
destroy_dialog (DialogData *data)
{
	if (data->dialog != NULL)
		gtk_widget_destroy (data->dialog);
	if (data->service != NULL)
		gth_task_completed (GTH_TASK (data->service), NULL);
	_g_object_unref (data->cancellable);
	_g_object_unref (data->album);
	_g_object_list_unref (data->albums);
	_g_object_unref (data->builder);
	gtk_widget_destroy (data->progress_dialog);
	_g_object_list_unref (data->file_list);
	_g_object_unref (data->location);
	g_free (data);
}


static void
completed_messagedialog_response_cb (GtkDialog *dialog,
				     int        response_id,
				     gpointer   user_data)
{
	DialogData *data = user_data;

	switch (response_id) {
	case GTK_RESPONSE_DELETE_EVENT:
	case GTK_RESPONSE_CLOSE:
		gtk_widget_destroy (GTK_WIDGET (dialog));
		gtk_widget_destroy (data->dialog);
		break;

	case _OPEN_IN_BROWSER_RESPONSE:
		{
			GdkScreen    *screen;
			OAuthAccount *account;
			char         *url = NULL;
			GError       *error = NULL;

			screen = gtk_widget_get_screen (GTK_WIDGET (dialog));
			gtk_widget_destroy (GTK_WIDGET (dialog));

			account = web_service_get_current_account (WEB_SERVICE (data->service));

			if (PHOTOBUCKET_ACCOUNT (account)->album_url != NULL) {
				char *username;

				username = account->username;
				if (g_str_equal (data->album->name, username))
					url = g_strdup (PHOTOBUCKET_ACCOUNT (account)->album_url);
				else
					url = g_strconcat (PHOTOBUCKET_ACCOUNT (account)->album_url,
							   data->album->name + strlen (username) + 1,
							   NULL);
			}

			if ((url != NULL) && ! gtk_show_uri (screen, url, 0, &error)) {
				if (data->service != NULL)
					gth_task_dialog (GTH_TASK (data->service), TRUE, NULL);
				_gtk_error_dialog_from_gerror_run (GTK_WINDOW (data->browser), _("Could not connect to the server"), error);
				g_clear_error (&error);
			}

			gtk_widget_destroy (data->dialog);

			g_free (url);
		}
		break;

	default:
		break;
	}
}


static void
export_completed_with_success (DialogData *data)
{
	GtkBuilder *builder;
	GtkWidget  *dialog;

	gth_task_dialog (GTH_TASK (data->service), TRUE, NULL);

	builder = _gtk_builder_new_from_file ("photobucket-export-completed.ui", "photobucket");
	dialog = _gtk_builder_get_widget (builder, "completed_messagedialog");
	g_object_set_data_full (G_OBJECT (dialog), "builder", builder, g_object_unref);
	g_signal_connect (dialog,
			  "delete-event",
			  G_CALLBACK (gtk_true),
			  NULL);
	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (completed_messagedialog_response_cb),
			  data);

	gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (data->browser));
	gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
	gtk_window_present (GTK_WINDOW (dialog));
}


static void
upload_photos_ready_cb (GObject      *source_object,
		        GAsyncResult *result,
		        gpointer      user_data)
{
	DialogData *data = user_data;
	GError     *error = NULL;

	if (! photobucket_service_upload_photos_finish (data->service, result, &error)) {
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (data->browser), _("Could not upload the files"), error);
		g_clear_error (&error);
		gtk_widget_destroy (data->dialog);
		return;
	}

	export_completed_with_success (data);
}


static void
export_dialog_response_cb (GtkDialog *dialog,
			   int        response_id,
			   gpointer   user_data)
{
	DialogData *data = user_data;

	switch (response_id) {
	case GTK_RESPONSE_HELP:
		show_help_dialog (GTK_WINDOW (data->browser), "gthumb-export-social");
		break;

	case GTK_RESPONSE_DELETE_EVENT:
	case GTK_RESPONSE_CANCEL:
		gth_file_list_cancel (GTH_FILE_LIST (data->list_view), (DataFunc) destroy_dialog, data);
		break;

	case GTK_RESPONSE_OK:
		{
			GtkTreeIter  iter;
			GList       *file_list;
			int          size;

			gtk_widget_hide (data->dialog);
			gth_task_dialog (GTH_TASK (data->service), FALSE, NULL);

			_g_clear_object (&data->album);
			if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (GET_WIDGET ("album_combobox")), &iter)) {
				gtk_tree_model_get (gtk_combo_box_get_model (GTK_COMBO_BOX (GET_WIDGET ("album_combobox"))),
						    &iter,
						    ALBUM_DATA_COLUMN, &data->album,
						    -1);
			}

			size = 0;
			if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (GET_WIDGET ("resize_combobox")), &iter)) {
				gtk_tree_model_get (gtk_combo_box_get_model (GTK_COMBO_BOX (GET_WIDGET ("resize_combobox"))),
						    &iter,
						    RESIZE_SIZE_COLUMN, &size,
						    -1);
			}

			file_list = gth_file_data_list_to_file_list (data->file_list);
			photobucket_service_upload_photos (data->service,
							   data->album,
							   size,
							   gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("scramble_checkbutton"))),
							   file_list,
							   data->cancellable,
							   upload_photos_ready_cb,
							   data);

			_g_object_list_unref (file_list);
		}
		break;

	default:
		break;
	}
}


static void
update_account_list (DialogData *data)
{
	int           current_account_idx;
	OAuthAccount *current_account;
	int           idx;
	GList        *scan;
	GtkTreeIter   iter;
	char         *free_space;

	gtk_list_store_clear (GTK_LIST_STORE (GET_WIDGET ("account_liststore")));

	current_account_idx = 0;
	current_account = web_service_get_current_account (WEB_SERVICE (data->service));
	for (scan = web_service_get_accounts (WEB_SERVICE (data->service)), idx = 0; scan; scan = scan->next, idx++) {
		OAuthAccount *account = scan->data;

		if (oauth_account_cmp (current_account, account) == 0)
			current_account_idx = idx;

		gtk_list_store_append (GTK_LIST_STORE (GET_WIDGET ("account_liststore")), &iter);
		gtk_list_store_set (GTK_LIST_STORE (GET_WIDGET ("account_liststore")), &iter,
				    ACCOUNT_DATA_COLUMN, account,
				    ACCOUNT_NAME_COLUMN, account->username,
				    -1);
	}
	gtk_combo_box_set_active (GTK_COMBO_BOX (GET_WIDGET ("account_combobox")), current_account_idx);

	free_space = g_format_size ((PHOTOBUCKET_ACCOUNT (current_account)->megabytes_allowed - PHOTOBUCKET_ACCOUNT (current_account)->megabytes_used) * (1024 * 1024));
	gtk_label_set_text (GTK_LABEL (GET_WIDGET ("free_space_label")), free_space);
	g_free (free_space);

	if (current_account != NULL) {
		struct {
			char *name;
			int   size;
		} sizes[] = { { N_("Tiny ( 100 × 75 )"), 100 },
			      { N_("Small ( 160 × 120 )"), 160 },
			      { N_("Medium ( 320 × 240 )"), 320 },
			      { N_("Large ( 640 × 480 )"), 640 },
			      { N_("15ʺ screen ( 800 × 600 )"), 800 },
			      { N_("17ʺ screen ( 1024 × 768 )"), 1024 },
			      { N_("1 megabyte file size"), 0 },
			      { "1280 × 960", 1280 },
			      { "1600 × 1200", 1600 },
			      { "2048 × 1536", 2048 },
			      { "2592 × 1944", 2592 },
			      { "3648 × 2736", 3648 },
			      { "4000 × 3000", 4000 },
			      { N_("2 megabyte file size"), 0 } };
		int i;

		gtk_list_store_clear (GTK_LIST_STORE (GET_WIDGET ("resize_liststore")));
		for (i = 0; i <= (PHOTOBUCKET_ACCOUNT (current_account)->is_premium ? 13 : 6); i++) {
			gtk_list_store_append (GTK_LIST_STORE (GET_WIDGET ("resize_liststore")), &iter);
			gtk_list_store_set (GTK_LIST_STORE (GET_WIDGET ("resize_liststore")), &iter,
					    RESIZE_NAME_COLUMN, _(sizes[i].name),
					    RESIZE_SIZE_COLUMN, sizes[i].size,
					    -1);
		}

		gtk_combo_box_set_active (GTK_COMBO_BOX (GET_WIDGET ("resize_combobox")), (PHOTOBUCKET_ACCOUNT (current_account)->is_premium ? 13 : 5));
	}
}


static void
authentication_accounts_changed_cb (WebService *service,
				    DialogData *data)
{
	update_account_list (data);
}


static void
update_album_list (DialogData *data)
{
	int    album_idx = 0;
	GList *scan;
	int    i;

	gtk_list_store_clear (GTK_LIST_STORE (GET_WIDGET ("album_liststore")));
	for (scan = data->albums, i = 0; scan; scan = scan->next, i++) {
		PhotobucketAlbum *album = scan->data;
		char             *size;
		GtkTreeIter       iter;

		if (data->album != NULL && g_str_equal (data->album->name, album->name))
			album_idx = i;

		size = g_strdup_printf ("(%d)", album->photo_count + album->video_count);

		gtk_list_store_append (GTK_LIST_STORE (GET_WIDGET ("album_liststore")), &iter);
		gtk_list_store_set (GTK_LIST_STORE (GET_WIDGET ("album_liststore")), &iter,
				    ALBUM_DATA_COLUMN, album,
				    ALBUM_ICON_COLUMN, "file-catalog",
				    ALBUM_TITLE_COLUMN, album->name,
				    ALBUM_N_PHOTOS_COLUMN, size,
				    -1);

		g_free (size);
	}

	if (data->albums != NULL)
		gtk_combo_box_set_active (GTK_COMBO_BOX (GET_WIDGET ("album_combobox")), album_idx);
}


static void
album_list_ready_cb (GObject      *source_object,
		     GAsyncResult *res,
		     gpointer      user_data)
{
	DialogData *data = user_data;
	GError     *error = NULL;

	_g_object_list_unref (data->albums);
	data->albums = photobucket_service_get_albums_finish (data->service, res, &error);
	if (error != NULL) {
		if (data->service != NULL)
			gth_task_dialog (GTH_TASK (data->service), TRUE, NULL);
		_gtk_error_dialog_from_gerror_run (GTK_WINDOW (data->browser), _("Could not connect to the server"), error);
		g_clear_error (&error);
		gtk_widget_destroy (data->dialog);
		return;
	}
	update_album_list (data);

	gth_task_dialog (GTH_TASK (data->service), TRUE, NULL);

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), GTK_WINDOW (data->browser));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), FALSE);
	gtk_window_present (GTK_WINDOW (data->dialog));
}


static void
authentication_ready_cb (WebService *service,
			 DialogData *data)
{
	update_account_list (data);
	photobucket_service_get_albums (data->service,
				        data->cancellable,
				        album_list_ready_cb,
				        data);
}


static void
edit_accounts_button_clicked_cb (GtkButton  *button,
				 DialogData *data)
{
	web_service_edit_accounts (WEB_SERVICE (data->service), GTK_WINDOW (data->dialog));
}


static void
account_combobox_changed_cb (GtkComboBox *widget,
			     gpointer     user_data)
{
	DialogData   *data = user_data;
	GtkTreeIter   iter;
	OAuthAccount *account;

	if (! gtk_combo_box_get_active_iter (widget, &iter))
		return;

	gtk_tree_model_get (gtk_combo_box_get_model (widget),
			    &iter,
			    ACCOUNT_DATA_COLUMN, &account,
			    -1);

	if (oauth_account_cmp (account, web_service_get_current_account (WEB_SERVICE (data->service))) != 0)
		web_service_connect (WEB_SERVICE (data->service), account);

	g_object_unref (account);
}


static void
album_combobox_changed_cb (GtkComboBox *widget,
			   gpointer     user_data)
{
	DialogData  *data = user_data;
	GtkTreeIter  iter;

	gtk_widget_set_sensitive (GET_WIDGET ("upload_button"), gtk_combo_box_get_active_iter (widget, &iter));
}


static void
create_album_ready_cb (GObject      *source_object,
		       GAsyncResult *result,
		       gpointer      user_data)
{
	DialogData *data = user_data;
	GError     *error = NULL;

	_g_object_unref (data->album);
	data->album = photobucket_service_create_album_finish (data->service, result, &error);
	if (error != NULL) {
		if (data->service != NULL)
			gth_task_dialog (GTH_TASK (data->service), TRUE, NULL);
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (data->browser), _("Could not create the album"), error);
		g_clear_error (&error);
		return;
	}

	data->albums = g_list_append (data->albums, data->album);
	update_album_list (data);
}


static void
new_album_dialog_response_cb (GtkDialog *dialog,
			      int        response_id,
			      gpointer   user_data)
{
	DialogData *data = user_data;

	switch (response_id) {
	case GTK_RESPONSE_DELETE_EVENT:
	case GTK_RESPONSE_CANCEL:
		gtk_widget_destroy (GTK_WIDGET (dialog));
		break;

	case GTK_RESPONSE_OK:
		{
			PhotobucketAlbum *album;
			char             *parent_album;

			parent_album = photobucket_album_properties_dialog_get_parent_album (PHOTOBUCKET_ALBUM_PROPERTIES_DIALOG (dialog));
			if (parent_album == NULL)
				return;

			album = photobucket_album_new ();
			photobucket_album_set_name (album, photobucket_album_properties_dialog_get_name (PHOTOBUCKET_ALBUM_PROPERTIES_DIALOG (dialog)));
			photobucket_service_create_album (data->service,
							  parent_album,
						          album,
						          data->cancellable,
						          create_album_ready_cb,
						          data);

			g_object_unref (album);
			g_free (parent_album);
		}
		gtk_widget_destroy (GTK_WIDGET (dialog));
		break;

	default:
		break;
	}
}


static void
add_album_button_clicked_cb (GtkButton *button,
			     gpointer   user_data)
{
	DialogData *data = user_data;
	GtkWidget  *dialog;

	dialog = photobucket_album_properties_dialog_new (g_file_info_get_edit_name (data->location->info),
							  data->albums);
	g_signal_connect (dialog,
			  "delete-event",
			  G_CALLBACK (gtk_true),
			  NULL);
	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (new_album_dialog_response_cb),
			  data);

	gtk_window_set_title (GTK_WINDOW (dialog), _("New Album"));
	gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (data->dialog));
	gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
	gtk_window_present (GTK_WINDOW (dialog));
}


void
dlg_export_to_photobucket (GthBrowser *browser,
			   GList      *file_list)
{
	DialogData *data;
	GList      *scan;
	int         n_total;
	goffset     total_size;
	char       *total_size_formatted;
	char       *text;
	char       *title;

	data = g_new0 (DialogData, 1);
	data->browser = browser;
	data->location = gth_file_data_dup (gth_browser_get_location_data (browser));
	data->builder = _gtk_builder_new_from_file ("export-to-photobucket.ui", "photobucket");
	data->dialog = _gtk_builder_get_widget (data->builder, "export_dialog");
	data->cancellable = g_cancellable_new ();

	{
		GtkCellLayout   *cell_layout;
		GtkCellRenderer *renderer;

		cell_layout = GTK_CELL_LAYOUT (GET_WIDGET ("album_combobox"));

		renderer = gtk_cell_renderer_pixbuf_new ();
		gtk_cell_layout_pack_start (cell_layout, renderer, FALSE);
		gtk_cell_layout_set_attributes (cell_layout, renderer,
						"icon-name", ALBUM_ICON_COLUMN,
						NULL);

		renderer = gtk_cell_renderer_text_new ();
		gtk_cell_layout_pack_start (cell_layout, renderer, TRUE);
		gtk_cell_layout_set_attributes (cell_layout, renderer,
						"text", ALBUM_TITLE_COLUMN,
						NULL);

		renderer = gtk_cell_renderer_text_new ();
		gtk_cell_layout_pack_start (cell_layout, renderer, FALSE);
		gtk_cell_layout_set_attributes (cell_layout, renderer,
						"text", ALBUM_N_PHOTOS_COLUMN,
						NULL);
	}

	_gtk_window_resize_to_fit_screen_height (data->dialog, 500);

	data->file_list = NULL;
	n_total = 0;
	total_size = 0;
	for (scan = file_list; scan; scan = scan->next) {
		GthFileData *file_data = scan->data;
		const char  *mime_type;

		mime_type = gth_file_data_get_mime_type (file_data);
		if (g_content_type_equals (mime_type, "image/bmp")
		    || g_content_type_equals (mime_type, "image/gif")
		    || g_content_type_equals (mime_type, "image/jpeg")
		    || g_content_type_equals (mime_type, "image/png"))
		{
			total_size += g_file_info_get_size (file_data->info);
			n_total++;
			data->file_list = g_list_prepend (data->file_list, g_object_ref (file_data));
		}
	}

	/* upload in reverse order because photobucket orders the files from
	 * the newest to the oldest.
	  data->file_list = g_list_reverse (data->file_list);  */

	if (data->file_list == NULL) {
		GError *error;

		error = g_error_new_literal (GTH_ERROR, GTH_ERROR_GENERIC, _("No valid file selected."));
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (browser), _("Could not export the files"), error);
		g_clear_error (&error);
		destroy_dialog (data);

		return;
	}

	total_size_formatted = g_format_size (total_size);
	text = g_strdup_printf (g_dngettext (NULL, "%d file (%s)", "%d files (%s)", n_total), n_total, total_size_formatted);
	gtk_label_set_text (GTK_LABEL (GET_WIDGET ("images_info_label")), text);
	g_free (text);
	g_free (total_size_formatted);

	/* Set the widget data */

	data->list_view = gth_file_list_new (gth_grid_view_new (), GTH_FILE_LIST_MODE_NO_SELECTION, FALSE);
	gth_file_list_set_thumb_size (GTH_FILE_LIST (data->list_view), 112);
	gth_file_list_enable_thumbs (GTH_FILE_LIST (data->list_view), TRUE);
	gth_file_list_set_ignore_hidden (GTH_FILE_LIST (data->list_view), TRUE);
	gth_file_list_set_caption (GTH_FILE_LIST (data->list_view), "none");
	gth_file_list_set_sort_func (GTH_FILE_LIST (data->list_view), gth_main_get_sort_type ("file::name")->cmp_func, FALSE);
	gtk_widget_show (data->list_view);
	gtk_box_pack_start (GTK_BOX (GET_WIDGET ("images_box")), data->list_view, TRUE, TRUE, 0);
	gth_file_list_set_files (GTH_FILE_LIST (data->list_view), data->file_list);

	gtk_widget_set_sensitive (GET_WIDGET ("upload_button"), FALSE);

	title = g_strdup_printf (_("Export to %s"), "Photobucket");
	gtk_window_set_title (GTK_WINDOW (data->dialog), title);
	g_free (title);

	/* Set the signals handlers. */

	g_signal_connect (data->dialog,
			  "delete-event",
			  G_CALLBACK (gtk_true),
			  NULL);
	g_signal_connect (data->dialog,
			  "response",
			  G_CALLBACK (export_dialog_response_cb),
			  data);
	g_signal_connect (GET_WIDGET ("edit_accounts_button"),
			  "clicked",
			  G_CALLBACK (edit_accounts_button_clicked_cb),
			  data);
	g_signal_connect (GET_WIDGET ("account_combobox"),
			  "changed",
			  G_CALLBACK (account_combobox_changed_cb),
			  data);
	g_signal_connect (GET_WIDGET ("add_album_button"),
			  "clicked",
			  G_CALLBACK (add_album_button_clicked_cb),
			  data);
	g_signal_connect (GET_WIDGET ("album_combobox"),
			  "changed",
			  G_CALLBACK (album_combobox_changed_cb),
			  data);


	data->service = photobucket_service_new (data->cancellable,
						 GTK_WIDGET (data->browser),
						 data->dialog);
	g_signal_connect (data->service,
			  "account-ready",
			  G_CALLBACK (authentication_ready_cb),
			  data);
	g_signal_connect (data->service,
			  "accounts-changed",
			  G_CALLBACK (authentication_accounts_changed_cb),
			  data);

	data->progress_dialog = gth_progress_dialog_new (GTK_WINDOW (data->browser));
	gth_progress_dialog_add_task (GTH_PROGRESS_DIALOG (data->progress_dialog), GTH_TASK (data->service));

	web_service_autoconnect (WEB_SERVICE (data->service));
}
