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
#include "dlg-export-to-flickr.h"
#include "flickr-account.h"
#include "flickr-photoset.h"
#include "flickr-service.h"
#include "preferences.h"


#define GET_WIDGET(x) (_gtk_builder_get_widget (data->builder, (x)))
#define _OPEN_IN_BROWSER_RESPONSE 1


enum {
	ACCOUNT_DATA_COLUMN,
	ACCOUNT_NAME_COLUMN
};


enum {
	PHOTOSET_DATA_COLUMN,
	PHOTOSET_ICON_COLUMN,
	PHOTOSET_TITLE_COLUMN,
	PHOTOSET_N_PHOTOS_COLUMN
};


typedef struct {
	FlickrServer         *server;
	GthBrowser           *browser;
	GSettings            *settings;
	GthFileData          *location;
	GList                *file_list;
	GtkBuilder           *builder;
	GtkWidget            *dialog;
	GtkWidget            *list_view;
	GtkWidget            *progress_dialog;
	GtkWidget            *photoset_combobox;
	FlickrService        *service;
	GList                *photosets;
	FlickrPhotoset       *photoset;
	GList                *photos_ids;
	GCancellable         *cancellable;
} DialogData;


static void
destroy_dialog (DialogData *data)
{
	if (data->dialog != NULL)
		gtk_widget_destroy (data->dialog);
	if (data->service != NULL)
		gth_task_completed (GTH_TASK (data->service), NULL);
	_g_object_unref (data->cancellable);
	_g_string_list_free (data->photos_ids);
	_g_object_unref (data->photoset);
	_g_object_list_unref (data->photosets);
	_g_object_unref (data->service);
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
			char         *url = NULL;
			GError       *error = NULL;

			gtk_widget_destroy (GTK_WIDGET (dialog));

			account = web_service_get_current_account (WEB_SERVICE (data->service));

			if (data->photoset == NULL) {
				GString *ids;
				GList   *scan;

				ids = g_string_new ("");
				for (scan = data->photos_ids; scan; scan = scan->next) {
					if (scan != data->photos_ids)
						g_string_append (ids, ",");
					g_string_append (ids, (char *) scan->data);
				}
				url = g_strconcat (data->server->url, "/photos/upload/edit/?ids=", ids->str, NULL);

				g_string_free (ids, TRUE);
			}
			else if (data->photoset->url != NULL)
				url = g_strdup (data->photoset->url);
			else if (data->photoset->id != NULL)
				url = g_strconcat (data->server->url, "/photos/", account->id, "/sets/", data->photoset->id, NULL);

			if ((url != NULL) && ! gtk_show_uri_on_window (GTK_WINDOW (data->browser), url, GDK_CURRENT_TIME, &error)) {
				if (data->service != NULL)
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
	GtkWidget *dialog;

	gth_task_dialog (GTH_TASK (data->service), TRUE, NULL);

	dialog = _gtk_message_dialog_new (GTK_WINDOW (data->browser),
					  GTK_DIALOG_MODAL,
					  NULL,
					  _("Files successfully uploaded to the server."),
					  NULL,
					  _GTK_LABEL_CLOSE, GTK_RESPONSE_CLOSE,
					  _("_Open in the Browser"), _OPEN_IN_BROWSER_RESPONSE,
					  NULL);
	g_signal_connect (dialog,
			  "delete-event",
			  G_CALLBACK (gtk_true),
			  NULL);
	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (completed_messagedialog_response_cb),
			  data);

	gtk_window_present (GTK_WINDOW (dialog));
}


static void
add_photos_to_photoset_ready_cb (GObject      *source_object,
				 GAsyncResult *result,
				 gpointer      user_data)
{
	DialogData *data = user_data;
	GError     *error = NULL;

	if (! flickr_service_add_photos_to_set_finish (FLICKR_SERVICE (source_object), result, &error)) {
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (data->browser), _("Could not create the album"), error);
		g_clear_error (&error);
		gtk_dialog_response (GTK_DIALOG (data->dialog), GTK_RESPONSE_DELETE_EVENT);
		return;
	}

	export_completed_with_success (data);
}


static void
add_photos_to_photoset (DialogData *data)
{
	flickr_service_add_photos_to_set (data->service,
					  data->photoset,
					  data->photos_ids,
					  data->cancellable,
					  add_photos_to_photoset_ready_cb,
					  data);
}


static void
create_photoset_ready_cb (GObject      *source_object,
			  GAsyncResult *result,
			  gpointer      user_data)
{
	DialogData *data = user_data;
	GError     *error = NULL;
	char       *primary;

	primary = g_strdup (data->photoset->primary);
	g_object_unref (data->photoset);
	data->photoset = flickr_service_create_photoset_finish (FLICKR_SERVICE (source_object), result, &error);
	if (error != NULL) {
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (data->browser), _("Could not create the album"), error);
		g_clear_error (&error);
		gtk_dialog_response (GTK_DIALOG (data->dialog), GTK_RESPONSE_DELETE_EVENT);
	}
	else {
		flickr_photoset_set_primary (data->photoset, primary);
		add_photos_to_photoset (data);
	}

	g_free (primary);
}


static void
post_photos_ready_cb (GObject      *source_object,
		      GAsyncResult *result,
		      gpointer      user_data)
{
	DialogData *data = user_data;
	GError     *error = NULL;

	data->photos_ids = flickr_service_post_photos_finish (FLICKR_SERVICE (source_object), result, &error);
	if (error != NULL) {
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (data->browser), _("Could not upload the files"), error);
		g_clear_error (&error);
		gtk_dialog_response (GTK_DIALOG (data->dialog), GTK_RESPONSE_DELETE_EVENT);
		return;
	}

	if (data->photoset == NULL) {
		export_completed_with_success (data);
		return;
	}

	/* create the photoset if it doesn't exists */

	if (data->photoset->id == NULL) {
		char *first_id;

		first_id = data->photos_ids->data;
		flickr_photoset_set_primary (data->photoset, first_id);
		flickr_service_create_photoset (data->service,
						data->photoset,
						data->cancellable,
						create_photoset_ready_cb,
						data);
	}
	else
		add_photos_to_photoset (data);
}


static int
find_photoset_by_title (FlickrPhotoset *photoset,
		        const char     *name)
{
	return g_strcmp0 (photoset->title, name);
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
			const char *photoset_title;
			GList      *file_list;
			int         max_width;
			int         max_height;

			gtk_widget_hide (data->dialog);
			gth_task_dialog (GTH_TASK (data->service), FALSE, NULL);

			data->photoset = NULL;
			photoset_title = gtk_entry_get_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (data->photoset_combobox))));
			if ((photoset_title != NULL) && (g_strcmp0 (photoset_title, "") != 0)) {
				GList *link;

				link = g_list_find_custom (data->photosets, photoset_title, (GCompareFunc) find_photoset_by_title);
				if (link != NULL)
					data->photoset = g_object_ref (link->data);

				if (data->photoset == NULL) {
					data->photoset = flickr_photoset_new ();
					flickr_photoset_set_title (data->photoset, photoset_title);
				}
			}

			file_list = gth_file_data_list_to_file_list (data->file_list);

			max_width = -1;
			max_height = -1;
			if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("resize_checkbutton")))) {
				int idx = gtk_combo_box_get_active (GTK_COMBO_BOX (GET_WIDGET ("resize_combobox")));
				max_width = ImageSizeValues[idx].width;
				max_height = ImageSizeValues[idx].height;
			}
			g_settings_set_int (data->settings, PREF_FLICKR_RESIZE_WIDTH, max_width);
			g_settings_set_int (data->settings, PREF_FLICKR_RESIZE_HEIGHT, max_height);

			flickr_service_post_photos (data->service,
						    gtk_combo_box_get_active (GTK_COMBO_BOX (GET_WIDGET ("privacy_combobox"))),
						    gtk_combo_box_get_active (GTK_COMBO_BOX (GET_WIDGET ("safety_combobox"))),
						    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("hidden_checkbutton"))),
						    max_width,
						    max_height,
						    file_list,
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
				    ACCOUNT_NAME_COLUMN, account->name,
				    -1);
	}
	gtk_combo_box_set_active (GTK_COMBO_BOX (GET_WIDGET ("account_combobox")), current_account_idx);

	free_space = g_format_size (FLICKR_ACCOUNT (current_account)->max_bandwidth - FLICKR_ACCOUNT (current_account)->used_bandwidth);
	gtk_label_set_text (GTK_LABEL (GET_WIDGET ("free_space_label")), free_space);
	g_free (free_space);
}


static void
authentication_accounts_changed_cb (WebService *service,
				    gpointer    user_data)
{
	update_account_list ((DialogData *) user_data);
}


static void
photoset_list_ready_cb (GObject      *source_object,
			GAsyncResult *res,
			gpointer      user_data)
{
	DialogData *data = user_data;
	GError     *error = NULL;
	GList      *scan;

	_g_object_list_unref (data->photosets);
	data->photosets = flickr_service_list_photosets_finish (FLICKR_SERVICE (source_object), res, &error);
	if (error != NULL) {
		if (data->service != NULL)
			gth_task_dialog (GTH_TASK (data->service), TRUE, NULL);
		_gtk_error_dialog_from_gerror_run (GTK_WINDOW (data->browser), _("Could not connect to the server"), error);
		g_clear_error (&error);
		gtk_dialog_response (GTK_DIALOG (data->dialog), GTK_RESPONSE_DELETE_EVENT);
		return;
	}

	gtk_list_store_clear (GTK_LIST_STORE (GET_WIDGET ("photoset_liststore")));
	for (scan = data->photosets; scan; scan = scan->next) {
		FlickrPhotoset *photoset = scan->data;
		char           *n_photos;
		GtkTreeIter     iter;

		n_photos = g_strdup_printf ("(%d)", photoset->n_photos);

		gtk_list_store_append (GTK_LIST_STORE (GET_WIDGET ("photoset_liststore")), &iter);
		gtk_list_store_set (GTK_LIST_STORE (GET_WIDGET ("photoset_liststore")), &iter,
				    PHOTOSET_DATA_COLUMN, photoset,
				    PHOTOSET_ICON_COLUMN, "file-catalog-symbolic",
				    PHOTOSET_TITLE_COLUMN, photoset->title,
				    PHOTOSET_N_PHOTOS_COLUMN, n_photos,
				    -1);

		g_free (n_photos);
	}

	gtk_dialog_set_response_sensitive (GTK_DIALOG (data->dialog), GTK_RESPONSE_OK, TRUE);

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
	flickr_service_list_photosets (data->service,
				       data->cancellable,
				       photoset_list_ready_cb,
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
		web_service_connect (WEB_SERVICE (data->service), OAUTH_ACCOUNT (account));

	g_object_unref (account);
}


static void
update_sensitivity (DialogData *data)
{
	gtk_widget_set_sensitive (GET_WIDGET ("resize_combobox"), gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("resize_checkbutton"))));
}


static void
resize_checkbutton_toggled_cb (GtkToggleButton *button,
			       gpointer         user_data)
{
	update_sensitivity (user_data);
}


void
dlg_export_to_flickr (FlickrServer *server,
		      GthBrowser   *browser,
		      GList        *file_list)
{
	DialogData *data;
	GList      *scan;
	int         n_total;
	goffset     total_size;
	char       *total_size_formatted;
	char       *text;
	char       *title;

	data = g_new0 (DialogData, 1);
	data->server = server;
	data->browser = browser;
	data->settings = g_settings_new (GTHUMB_FLICKR_SCHEMA);
	data->location = gth_file_data_dup (gth_browser_get_location_data (browser));
	data->builder = _gtk_builder_new_from_file ("export-to-flickr.ui", "flicker_utils");
	data->cancellable = g_cancellable_new ();

	data->dialog = g_object_new (GTK_TYPE_DIALOG,
				     "transient-for", GTK_WINDOW (browser),
				     "modal", FALSE,
				     "destroy-with-parent", FALSE,
				     "use-header-bar", _gtk_settings_get_dialogs_use_header (),
				     NULL);
	gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (data->dialog))),
			   _gtk_builder_get_widget (data->builder, "dialog_content"));
	gtk_dialog_add_buttons (GTK_DIALOG (data->dialog),
			        _GTK_LABEL_CANCEL, GTK_RESPONSE_CANCEL,
				_GTK_LABEL_UPLOAD, GTK_RESPONSE_OK,
				NULL);
	_gtk_dialog_add_class_to_response (GTK_DIALOG (data->dialog), GTK_RESPONSE_OK, GTK_STYLE_CLASS_SUGGESTED_ACTION);

	{
		GtkCellLayout   *cell_layout;
		GtkCellRenderer *renderer;

		data->photoset_combobox = gtk_combo_box_new_with_model_and_entry (GTK_TREE_MODEL (GET_WIDGET ("photoset_liststore")));
		gtk_combo_box_set_entry_text_column (GTK_COMBO_BOX (data->photoset_combobox), PHOTOSET_TITLE_COLUMN);
		gtk_widget_show (data->photoset_combobox);
		gtk_container_add (GTK_CONTAINER (GET_WIDGET ("photoset_combobox_container")), data->photoset_combobox);

		cell_layout = GTK_CELL_LAYOUT (data->photoset_combobox);
		gtk_cell_layout_clear (cell_layout);

		renderer = gtk_cell_renderer_pixbuf_new ();
		gtk_cell_layout_pack_start (cell_layout, renderer, FALSE);
		gtk_cell_layout_set_attributes (cell_layout, renderer,
						"icon-name", PHOTOSET_ICON_COLUMN,
						NULL);

		renderer = gtk_cell_renderer_text_new ();
		gtk_cell_layout_pack_start (cell_layout, renderer, TRUE);
		gtk_cell_layout_set_attributes (cell_layout, renderer,
						"text", PHOTOSET_TITLE_COLUMN,
						NULL);

		renderer = gtk_cell_renderer_text_new ();
		gtk_cell_layout_pack_start (cell_layout, renderer, FALSE);
		gtk_cell_layout_set_attributes (cell_layout, renderer,
						"text", PHOTOSET_N_PHOTOS_COLUMN,
						NULL);
	}

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

		error = g_error_new_literal (GTH_ERROR, GTH_ERROR_GENERIC, _("No valid file selected."));
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (browser), _("Could not export the files"), error);
		g_clear_error (&error);
		gtk_dialog_response (GTK_DIALOG (data->dialog), GTK_RESPONSE_DELETE_EVENT);

		return;
	}

	total_size_formatted = g_format_size (total_size);
	text = g_strdup_printf (g_dngettext (NULL, "%d file (%s)", "%d files (%s)", n_total), n_total, total_size_formatted);
	gtk_label_set_text (GTK_LABEL (GET_WIDGET ("images_info_label")), text);
	g_free (text);
	g_free (total_size_formatted);

	_gtk_window_resize_to_fit_screen_height (data->dialog, 500);

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

	gtk_entry_set_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (data->photoset_combobox))), g_file_info_get_edit_name (data->location->info));

	gtk_dialog_set_response_sensitive (GTK_DIALOG (data->dialog), GTK_RESPONSE_OK, FALSE);

	title = g_strdup_printf (_("Export to %s"), data->server->name);
	gtk_window_set_title (GTK_WINDOW (data->dialog), title);
	g_free (title);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("resize_checkbutton")),
				      g_settings_get_int (data->settings, PREF_FLICKR_RESIZE_WIDTH) != -1);

	_gtk_combo_box_add_image_sizes (GTK_COMBO_BOX (GET_WIDGET ("resize_combobox")),
					g_settings_get_int (data->settings, PREF_FLICKR_RESIZE_WIDTH),
					g_settings_get_int (data->settings, PREF_FLICKR_RESIZE_HEIGHT));

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
	g_signal_connect (GET_WIDGET ("resize_checkbutton"),
			  "toggled",
			  G_CALLBACK (resize_checkbutton_toggled_cb),
			  data);

	update_sensitivity (data);

	data->service = flickr_service_new (server,
					    data->cancellable,
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
	gth_progress_dialog_add_task (GTH_PROGRESS_DIALOG (data->progress_dialog), GTH_TASK (data->service), GTH_TASK_FLAGS_DEFAULT);

	web_service_autoconnect (WEB_SERVICE (data->service));
}
