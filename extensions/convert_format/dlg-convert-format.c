/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2010 Free Software Foundation, Inc.
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
#include "dlg-convert-format.h"
#include "preferences.h"


#define GET_WIDGET(name) _gtk_builder_get_widget (data->builder, (name))
#define DEFAULT_MIME_TYPE "image/jpeg"


enum {
	MIME_TYPE_COLUMN_ICON = 0,
	MIME_TYPE_COLUMN_TYPE,
	MIME_TYPE_COLUMN_DESCRIPTION
};


typedef struct {
	GthBrowser *browser;
	GSettings  *settings;
	GList      *file_list;
	GtkBuilder *builder;
	GtkWidget  *dialog;
	gboolean    use_destination;
} DialogData;


static void
dialog_destroy_cb (GtkWidget  *widget,
		   DialogData *data)
{
	gth_browser_set_dialog (data->browser, "convert_format", NULL);

	g_object_unref (data->settings);
	g_object_unref (data->builder);
	_g_object_list_unref (data->file_list);
	g_free (data);
}


static gpointer
exec_convert (GthAsyncTask *task,
	      gpointer      user_data)
{
	gth_image_task_copy_source_to_destination (GTH_IMAGE_TASK (task));
	return NULL;
}


static void
ok_button_clicked_cb (GtkWidget  *widget,
		      DialogData *data)
{
	GtkTreeIter  iter;
	char        *mime_type;
	GthTask     *convert_task;
	GthTask     *list_task;

	gtk_combo_box_get_active_iter (GTK_COMBO_BOX (GET_WIDGET ("mime_type_combobox")), &iter);
	gtk_tree_model_get (GTK_TREE_MODEL (GET_WIDGET ("mime_type_liststore")), &iter,
			    MIME_TYPE_COLUMN_TYPE, &mime_type,
			    -1);
	g_settings_set_string (data->settings, PREF_CONVERT_FORMAT_MIME_TYPE, mime_type);

	convert_task = gth_image_task_new (_("Converting images"),
					    NULL,
					    exec_convert,
					    NULL,
					    NULL,
					    NULL);
	list_task = gth_image_list_task_new (data->browser,
					     data->file_list,
					     GTH_IMAGE_TASK (convert_task));
	gth_image_list_task_set_overwrite_mode (GTH_IMAGE_LIST_TASK (list_task), GTH_OVERWRITE_ASK);
	gth_image_list_task_set_output_mime_type (GTH_IMAGE_LIST_TASK (list_task), mime_type);
	if (data->use_destination) {
		GFile *destination;

		destination = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (GET_WIDGET ("destination_filechooserbutton")));
		gth_image_list_task_set_destination (GTH_IMAGE_LIST_TASK (list_task), destination);

		g_object_unref (destination);
	}
	gth_browser_exec_task (data->browser, list_task, GTH_TASK_FLAGS_DEFAULT);

	g_object_unref (list_task);
	g_object_unref (convert_task);
	g_free (mime_type);
	gtk_widget_destroy (data->dialog);
}


static void
use_destination_checkbutton_toggled_cb (GtkToggleButton *button,
					gpointer         user_data)
{
	DialogData *data = user_data;

	data->use_destination = ! gtk_toggle_button_get_active (button);
	gtk_widget_set_sensitive (GET_WIDGET ("destination_filechooserbutton"), data->use_destination);
}


void
dlg_convert_format (GthBrowser *browser,
		    GList      *file_list)
{
	DialogData  *data;
	GArray      *savers;
	GthFileData *first_file_data;

	if (gth_browser_get_dialog (browser, "convert_format") != NULL) {
		gtk_window_present (GTK_WINDOW (gth_browser_get_dialog (browser, "convert_format")));
		return;
	}

	data = g_new0 (DialogData, 1);
	data->browser = browser;
	data->builder = _gtk_builder_new_from_file ("convert-format.ui", "convert_format");
	data->settings = g_settings_new (GTHUMB_CONVERT_FORMAT_SCHEMA);
	data->file_list = gth_file_data_list_dup (file_list);
	data->use_destination = TRUE;

	/* Get the widgets. */

	data->dialog = g_object_new (GTK_TYPE_DIALOG,
				     "title", _("Convert Format"),
				     "transient-for", GTK_WINDOW (browser),
				     "modal", FALSE,
				     "destroy-with-parent", FALSE,
				     "use-header-bar", _gtk_settings_get_dialogs_use_header (),
				     NULL);
	gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (data->dialog))),
			   _gtk_builder_get_widget (data->builder, "dialog_content"));
	gtk_dialog_add_buttons (GTK_DIALOG (data->dialog),
				_GTK_LABEL_CANCEL, GTK_RESPONSE_CANCEL,
				_GTK_LABEL_EXECUTE, GTK_RESPONSE_OK,
				NULL);
	_gtk_dialog_add_class_to_response (GTK_DIALOG (data->dialog), GTK_RESPONSE_OK, GTK_STYLE_CLASS_SUGGESTED_ACTION);

	gth_browser_set_dialog (browser, "convert_format", data->dialog);
	g_object_set_data (G_OBJECT (data->dialog), "dialog_data", data);

	/* Set widgets data. */

	savers = gth_main_get_type_set ("image-saver");
	if (savers != NULL) {
		char         *default_mime_type;
		GthIconCache *icon_cache;
		GtkListStore *list_store;
		int           i;

		default_mime_type = g_settings_get_string (data->settings, PREF_CONVERT_FORMAT_MIME_TYPE);
		icon_cache = gth_icon_cache_new_for_widget (data->dialog, GTK_ICON_SIZE_MENU);
		list_store = (GtkListStore *) GET_WIDGET ("mime_type_liststore");
		for (i = 0; i < savers->len; i++) {
			GType           saver_type;
			GthImageSaver *saver;
			const char     *mime_type;
			GdkPixbuf      *pixbuf;
			GtkTreeIter     iter;

			saver_type = g_array_index (savers, GType, i);
			saver = g_object_new (saver_type, NULL);
			mime_type = gth_image_saver_get_mime_type (saver);
			pixbuf = gth_icon_cache_get_pixbuf (icon_cache, g_content_type_get_icon (mime_type));
			gtk_list_store_append (list_store, &iter);
			gtk_list_store_set (list_store, &iter,
					    MIME_TYPE_COLUMN_ICON, pixbuf,
					    MIME_TYPE_COLUMN_TYPE, mime_type,
					    MIME_TYPE_COLUMN_DESCRIPTION, g_content_type_get_description (mime_type),
					    -1);

			if (strcmp (default_mime_type, mime_type) == 0)
				gtk_combo_box_set_active_iter (GTK_COMBO_BOX (GET_WIDGET ("mime_type_combobox")), &iter);

			g_object_unref (pixbuf);
			g_object_unref (saver);
		}

		gth_icon_cache_free (icon_cache);
		g_free (default_mime_type);
	}

	first_file_data = (GthFileData *) data->file_list->data;
	_gtk_file_chooser_set_file_parent (GTK_FILE_CHOOSER (GET_WIDGET ("destination_filechooserbutton")),
				   	   first_file_data->file,
				   	   NULL);

	/* Set the signals handlers. */

	g_signal_connect (G_OBJECT (data->dialog),
			  "destroy",
			  G_CALLBACK (dialog_destroy_cb),
			  data);
	g_signal_connect (gtk_dialog_get_widget_for_response (GTK_DIALOG (data->dialog), GTK_RESPONSE_OK),
			  "clicked",
			  G_CALLBACK (ok_button_clicked_cb),
			  data);
	g_signal_connect_swapped (gtk_dialog_get_widget_for_response (GTK_DIALOG (data->dialog), GTK_RESPONSE_CANCEL),
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  G_OBJECT (data->dialog));
        g_signal_connect (GET_WIDGET ("use_destination_checkbutton"),
                          "toggled",
                          G_CALLBACK (use_destination_checkbutton_toggled_cb),
                          data);

	/* Run dialog. */

        if (GTH_IS_FILE_SOURCE_VFS (gth_browser_get_location_source (browser)))
        	gtk_widget_hide (GET_WIDGET ("use_destination_checkbutton"));

	gtk_widget_show (data->dialog);
}
