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
#include "dlg-export-to-picasaweb.h"
#include "picasa-album-properties-dialog.h"
#include "picasa-web-album.h"
#include "picasa-web-service.h"
#include "preferences.h"


#define GET_WIDGET(x) (_gtk_builder_get_widget (data->builder, (x)))
#define _OPEN_IN_BROWSER_RESPONSE 1


enum {
	ACCOUNT_DATA_COLUMN,
	ACCOUNT_NAME_COLUMN
};


enum {
	ALBUM_DATA_COLUMN,
	ALBUM_NAME_COLUMN,
	ALBUM_ICON_COLUMN,
	ALBUM_REMAINING_IMAGES_COLUMN,
	ALBUM_USED_BYTES_COLUMN,
	ALBUM_EMBLEM_COLUMN
};


typedef struct {
	GthBrowser       *browser;
	GSettings        *settings;
	GthFileData      *location;
	GList            *file_list;
	GtkBuilder       *builder;
	GtkWidget        *dialog;
	GtkWidget        *list_view;
	GtkWidget        *progress_dialog;
	PicasaWebService *service;
	GList            *albums;
	PicasaWebAlbum   *album;
	GCancellable     *cancellable;
} DialogData;


static void
destroy_dialog (DialogData *data)
{
	if (data->dialog != NULL)
		gtk_widget_destroy (data->dialog);
	gth_task_completed (GTH_TASK (data->service), NULL);
	_g_object_unref (data->cancellable);
	_g_object_unref (data->album);
	_g_object_unref (data->service);
	_g_object_list_unref (data->albums);
	gtk_widget_destroy (data->progress_dialog);
	_g_object_unref (data->builder);
	_g_object_list_unref (data->file_list);
	_g_object_unref (data->location);
	g_object_unref (data->settings);
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
		gtk_dialog_response (GTK_DIALOG (data->dialog), GTK_RESPONSE_DELETE_EVENT);
		break;

	case _OPEN_IN_BROWSER_RESPONSE:
		{
			OAuthAccount *account;
			GdkScreen    *screen;
			char         *url = NULL;
			GError       *error = NULL;

			account = web_service_get_current_account (WEB_SERVICE (data->service));
			screen = gtk_widget_get_screen (GTK_WIDGET (dialog));
			gtk_widget_destroy (GTK_WIDGET (dialog));

			if (data->album != NULL) {
				if (data->album->alternate_url != NULL)
					url = g_strdup (data->album->alternate_url);
				else
					url = g_strconcat ("http://picasaweb.google.com/", account->id, "/", data->album->id, NULL);
			}
			else
				url = g_strconcat ("http://picasaweb.google.com/", account->id, NULL);

			if ((url != NULL) && ! gtk_show_uri (screen, url, 0, &error)) {
				gth_task_dialog (GTH_TASK (data->service), TRUE, NULL);
				_gtk_error_dialog_from_gerror_run (GTK_WINDOW (data->browser), _("Could not connect to the server"), error);
				g_clear_error (&error);
			}
			gtk_dialog_response (GTK_DIALOG (data->dialog), GTK_RESPONSE_DELETE_EVENT);

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

	builder = _gtk_builder_new_from_file ("picasa-web-export-completed.ui", "picasaweb");
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
post_photos_ready_cb (GObject      *source_object,
		      GAsyncResult *result,
		      gpointer      user_data)
{
	DialogData       *data = user_data;
	PicasaWebService *picasaweb = PICASA_WEB_SERVICE (source_object);
	GError           *error = NULL;

	if (! picasa_web_service_post_photos_finish (picasaweb, result, &error)) {
		gth_task_dialog (GTH_TASK (data->service), TRUE, NULL);
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (data->browser), _("Could not upload the files"), error);
		g_clear_error (&error);
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
	case GTK_RESPONSE_DELETE_EVENT:
	case GTK_RESPONSE_CANCEL:
		gth_file_list_cancel (GTH_FILE_LIST (data->list_view), (DataFunc) destroy_dialog, data);
		break;

	case GTK_RESPONSE_OK:
		{
			GtkTreeModel *tree_model;
			GtkTreeIter   iter;
			GList        *file_list;
			int           max_width;
			int           max_height;

			if (! gtk_tree_selection_get_selected (gtk_tree_view_get_selection (GTK_TREE_VIEW (GET_WIDGET ("albums_treeview"))), &tree_model, &iter)) {
				gtk_dialog_set_response_sensitive (GTK_DIALOG (data->dialog),
								   GTK_RESPONSE_OK,
								   FALSE);
				return;
			}

			_g_clear_object (&data->album);
			gtk_tree_model_get (tree_model, &iter,
					    ALBUM_DATA_COLUMN, &data->album,
					    -1);

			gtk_widget_hide (data->dialog);
			gth_task_dialog (GTH_TASK (data->service), FALSE, NULL);

			file_list = gth_file_data_list_to_file_list (data->file_list);

			max_width = -1;
			max_height = -1;
			if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("resize_checkbutton")))) {
				int idx = gtk_combo_box_get_active (GTK_COMBO_BOX (GET_WIDGET ("resize_combobox")));
				max_width = ImageSizeValues[idx].width;
				max_height = ImageSizeValues[idx].height;
			}
			g_settings_set_int (data->settings, PREF_PICASAWEB_RESIZE_WIDTH, max_width);
			g_settings_set_int (data->settings, PREF_PICASAWEB_RESIZE_HEIGHT, max_height);

			picasa_web_service_post_photos (data->service,
							data->album,
							file_list,
							max_width,
							max_height,
							data->cancellable,
							post_photos_ready_cb,
							data);

			_g_object_list_unref (file_list);
		}
		break;

	default:
		break;
	}
}


static void
update_album_list (DialogData *data)
{
	GtkTreeIter  iter;
	GList       *scan;
	char        *free_space;

	free_space = g_format_size (picasa_web_service_get_free_space (PICASA_WEB_SERVICE (data->service)));
	gtk_label_set_text (GTK_LABEL (GET_WIDGET ("free_space_label")), free_space);
	g_free (free_space);

	gtk_list_store_clear (GTK_LIST_STORE (GET_WIDGET ("album_liststore")));
	for (scan = data->albums; scan; scan = scan->next) {
		PicasaWebAlbum *album = scan->data;
		char           *n_photos_remaining;
		char           *used_bytes;

		n_photos_remaining = g_strdup_printf ("%d", album->n_photos_remaining);
		used_bytes = g_format_size (album->used_bytes);

		gtk_list_store_append (GTK_LIST_STORE (GET_WIDGET ("album_liststore")), &iter);
		gtk_list_store_set (GTK_LIST_STORE (GET_WIDGET ("album_liststore")), &iter,
				    ALBUM_DATA_COLUMN, album,
				    ALBUM_ICON_COLUMN, "file-catalog",
				    ALBUM_NAME_COLUMN, album->title,
				    ALBUM_REMAINING_IMAGES_COLUMN, n_photos_remaining,
				    ALBUM_USED_BYTES_COLUMN, used_bytes,
				    -1);

		if (album->access == PICASA_WEB_ACCESS_PRIVATE)
			gtk_list_store_set (GTK_LIST_STORE (GET_WIDGET ("album_liststore")), &iter,
					    ALBUM_EMBLEM_COLUMN, "emblem-readonly",
					    -1);

		g_free (used_bytes);
		g_free (n_photos_remaining);
	}

	gtk_dialog_set_response_sensitive (GTK_DIALOG (data->dialog),
					   GTK_RESPONSE_OK,
					   FALSE);
}


static void
create_album_ready_cb (GObject      *source_object,
		       GAsyncResult *result,
		       gpointer      user_data)
{
	DialogData       *data = user_data;
	PicasaWebService *picasaweb = PICASA_WEB_SERVICE (source_object);
	PicasaWebAlbum   *album;
	GError           *error = NULL;

	album = picasa_web_service_create_album_finish (picasaweb, result, &error);
	if (error != NULL) {
		gth_task_dialog (GTH_TASK (data->service), TRUE, NULL);
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (data->browser), _("Could not create the album"), error);
		g_clear_error (&error);
		return;
	}

	data->albums = g_list_append (data->albums, album);
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
			PicasaWebAlbum *album;

			album = picasa_web_album_new ();
			picasa_web_album_set_title (album, picasa_album_properties_dialog_get_name (PICASA_ALBUM_PROPERTIES_DIALOG (dialog)));
			album->access = picasa_album_properties_dialog_get_access (PICASA_ALBUM_PROPERTIES_DIALOG (dialog));
			picasa_web_service_create_album (data->service,
							 album,
							 data->cancellable,
							 create_album_ready_cb,
							 data);

			g_object_unref (album);
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

	dialog = picasa_album_properties_dialog_new (g_file_info_get_edit_name (data->location->info),
						     NULL,
						     PICASA_WEB_ACCESS_PUBLIC);
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


static void
edit_accounts_button_clicked_cb (GtkButton *button,
			         gpointer   user_data)
{
	DialogData *data = user_data;

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
albums_treeview_selection_changed_cb (GtkTreeSelection *treeselection,
				      gpointer          user_data)
{
	DialogData *data = user_data;
	gboolean    selected;

	selected = gtk_tree_selection_get_selected (treeselection, NULL, NULL);
	gtk_dialog_set_response_sensitive (GTK_DIALOG (data->dialog),
					   GTK_RESPONSE_OK,
					   selected);
}


static void
update_sensitivity (DialogData *data)
{
	gtk_widget_set_sensitive (GET_WIDGET ("resize_combobox"),
				  gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("resize_checkbutton"))));
}


static void
resize_checkbutton_toggled_cb (GtkToggleButton *button,
			       gpointer         user_data)
{
	update_sensitivity (user_data);
}


static void
list_albums_ready_cb (GObject      *source_object,
		      GAsyncResult *result,
		      gpointer      user_data)
{
	DialogData       *data = user_data;
	PicasaWebService *picasaweb = PICASA_WEB_SERVICE (source_object);
	GError           *error = NULL;

	_g_object_list_unref (data->albums);
	data->albums = picasa_web_service_list_albums_finish (picasaweb, result, &error);
	if (error != NULL) {
		gth_task_dialog (GTH_TASK (data->service), TRUE, NULL);
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (data->browser), _("Could not get the album list"), error);
		g_clear_error (&error);
		gtk_dialog_response (GTK_DIALOG (data->dialog), GTK_RESPONSE_DELETE_EVENT);
		return;
	}

	update_album_list (data);

	gth_task_dialog (GTH_TASK (data->service), TRUE, NULL);
	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), GTK_WINDOW (data->browser));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), FALSE);
	gtk_window_present (GTK_WINDOW (data->dialog));
}


static void
update_account_list (DialogData *data)
{
	int           current_account_idx;
	OAuthAccount *current_account;
	int           idx;
	GList        *scan;
	GtkTreeIter   iter;

	gtk_list_store_clear (GTK_LIST_STORE (GET_WIDGET ("account_liststore")));

	current_account_idx = 0;
	current_account = web_service_get_current_account (WEB_SERVICE (data->service));
	for (scan = web_service_get_accounts (WEB_SERVICE (data->service)), idx = 0; scan; scan = scan->next, idx++) {
		OAuthAccount *account = scan->data;

		if ((current_account != NULL) && (g_strcmp0 (current_account->id, account->id) == 0))
			current_account_idx = idx;

		gtk_list_store_append (GTK_LIST_STORE (GET_WIDGET ("account_liststore")), &iter);
		gtk_list_store_set (GTK_LIST_STORE (GET_WIDGET ("account_liststore")), &iter,
				    ACCOUNT_DATA_COLUMN, account,
				    ACCOUNT_NAME_COLUMN, account->name,
				    -1);
	}
	gtk_combo_box_set_active (GTK_COMBO_BOX (GET_WIDGET ("account_combobox")), current_account_idx);
}


static void
service_account_ready_cb (WebService *service,
			  gpointer    user_data)
{
	DialogData *data = user_data;

	update_account_list (data);
	picasa_web_service_list_albums (data->service,
				        data->cancellable,
				        list_albums_ready_cb,
				        data);
}


static void
service_accounts_changed_cb (WebService *service,
			     gpointer    user_data)
{
	DialogData *data = user_data;
	update_account_list (data);
}


void
dlg_export_to_picasaweb (GthBrowser *browser,
		         GList      *file_list)
{
	DialogData       *data;
	GtkTreeSelection *selection;
	GList            *scan;
	int               n_total;
	goffset           total_size;
	char             *total_size_formatted;
	char             *text;

	data = g_new0 (DialogData, 1);
	data->browser = browser;
	data->settings = g_settings_new (GTHUMB_PICASAWEB_SCHEMA);
	data->location = gth_file_data_dup (gth_browser_get_location_data (browser));
	data->builder = _gtk_builder_new_from_file ("export-to-picasaweb.ui", "picasaweb");
	data->dialog = _gtk_builder_get_widget (data->builder, "export_dialog");
	data->cancellable = g_cancellable_new ();

	{
		GtkCellLayout   *cell_layout;
		GtkCellRenderer *renderer;

		cell_layout = GTK_CELL_LAYOUT (GET_WIDGET ("name_treeviewcolumn"));

		renderer = gtk_cell_renderer_pixbuf_new ();
		gtk_cell_layout_pack_start (cell_layout, renderer, FALSE);
		gtk_cell_layout_set_attributes (cell_layout, renderer,
						"icon-name", ALBUM_ICON_COLUMN,
						NULL);

		renderer = gtk_cell_renderer_text_new ();
		gtk_cell_layout_pack_start (cell_layout, renderer, TRUE);
		gtk_cell_layout_set_attributes (cell_layout, renderer,
						"text", ALBUM_NAME_COLUMN,
						NULL);

		renderer = gtk_cell_renderer_pixbuf_new ();
		gtk_cell_layout_pack_start (cell_layout, renderer, FALSE);
		gtk_cell_layout_set_attributes (cell_layout, renderer,
						"icon-name", ALBUM_EMBLEM_COLUMN,
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
	data->file_list = g_list_reverse (data->file_list);

	if (data->file_list == NULL) {
		GError *error;

		gth_task_dialog (GTH_TASK (data->service), TRUE, NULL);

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

	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (GET_WIDGET ("album_liststore")), ALBUM_NAME_COLUMN, GTK_SORT_ASCENDING);

	gtk_widget_set_sensitive (GET_WIDGET ("upload_button"), FALSE);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("resize_checkbutton")),
				      g_settings_get_int (data->settings, PREF_PICASAWEB_RESIZE_WIDTH) != -1);

	_gtk_combo_box_add_image_sizes (GTK_COMBO_BOX (GET_WIDGET ("resize_combobox")),
					g_settings_get_int (data->settings, PREF_PICASAWEB_RESIZE_WIDTH),
					g_settings_get_int (data->settings, PREF_PICASAWEB_RESIZE_HEIGHT));

	/* Set the signals handlers. */

	g_signal_connect (data->dialog,
			  "delete-event",
			  G_CALLBACK (gtk_true),
			  NULL);
	g_signal_connect (data->dialog,
			  "response",
			  G_CALLBACK (export_dialog_response_cb),
			  data);
	g_signal_connect (GET_WIDGET ("add_album_button"),
			  "clicked",
			  G_CALLBACK (add_album_button_clicked_cb),
			  data);
	g_signal_connect (GET_WIDGET ("edit_accounts_button"),
			  "clicked",
			  G_CALLBACK (edit_accounts_button_clicked_cb),
			  data);
	g_signal_connect (GET_WIDGET ("account_combobox"),
			  "changed",
			  G_CALLBACK (account_combobox_changed_cb),
			  data);
	g_signal_connect (GET_WIDGET ("resize_checkbutton"),
			  "toggled",
			  G_CALLBACK (resize_checkbutton_toggled_cb),
			  data);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (GET_WIDGET ("albums_treeview")));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
	g_signal_connect (selection,
			  "changed",
			  G_CALLBACK (albums_treeview_selection_changed_cb),
			  data);

	update_sensitivity (data);

	data->service = picasa_web_service_new (data->cancellable,
						GTK_WIDGET (data->browser),
						data->dialog);
	g_signal_connect (data->service,
			  "account-ready",
			  G_CALLBACK (service_account_ready_cb),
			  data);
	g_signal_connect (data->service,
			  "accounts-changed",
			  G_CALLBACK (service_accounts_changed_cb),
			  data);

	data->progress_dialog = gth_progress_dialog_new (GTK_WINDOW (data->browser));
	gth_progress_dialog_add_task (GTH_PROGRESS_DIALOG (data->progress_dialog), GTH_TASK (data->service));

	web_service_autoconnect (WEB_SERVICE (data->service));
}
