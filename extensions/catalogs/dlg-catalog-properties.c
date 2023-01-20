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
#include "dlg-catalog-properties.h"
#include "gth-catalog.h"


#define GET_WIDGET(name) _gtk_builder_get_widget (data->builder, (name))


typedef struct {
	GthBrowser  *browser;
	GtkBuilder  *builder;
	GtkWidget   *dialog;
	GtkWidget   *time_selector;
	GthCatalog  *catalog;
	GthFileData *file_data;
	GFile       *original_file;
} DialogData;


static void
destroy_cb (GtkWidget  *widget,
	    DialogData *data)
{
	g_object_ref (data->file_data);
	_g_object_unref (data->catalog);
	g_object_unref (data->builder);
	g_free (data);
}


static void
catalog_saved_cb (void     **buffer,
		  gsize      count,
		  GError    *error,
		  gpointer   user_data)
{
	DialogData *data = user_data;

	if (error == NULL) {
		if (! g_file_equal (data->original_file, data->file_data->file)) {
			GFile *gio_file;

			gio_file = gth_catalog_file_to_gio_file (data->original_file);
			g_file_delete (gio_file, NULL, NULL);
			g_object_unref (gio_file);

			gth_monitor_file_renamed (gth_main_get_default_monitor (),
						  data->original_file,
						  data->file_data->file);
		}

		gth_catalog_update_metadata (data->catalog, data->file_data);
		gth_monitor_metadata_changed (gth_main_get_default_monitor (), data->file_data);

		gth_hook_invoke ("dlg-catalog-properties-saved", data->browser, data->file_data, data->catalog);
	}
	else
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (data->browser), _("Could not save the catalog"), error);

	gtk_widget_destroy (data->dialog);
}


static void
save_button_clicked_cb (GtkButton  *button,
			DialogData *data)
{
	GthDateTime *date_time;
	GFile       *gio_file;
	char        *buffer;
	gsize        size;

	if (strcmp (gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("name_entry"))), "") != 0) {
		GFile *parent;
		char  *uri;
		char  *clean_name;
		char  *ext;
		char  *display_name;
		GFile *new_file;

		parent = g_file_get_parent (data->original_file);
		uri = g_file_get_uri (data->original_file);
		clean_name = _g_filename_clear_for_file (gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("name_entry"))));
		ext = _g_uri_get_extension (uri);
		display_name = g_strconcat (clean_name, ext, NULL);
		new_file = g_file_get_child_for_display_name (parent, display_name, NULL);
		if ((new_file != NULL) && ! g_file_equal (new_file, data->original_file))
			gth_file_data_set_file (data->file_data, new_file);

		_g_object_unref (new_file);
		g_free (display_name);
		g_free (ext);
		g_free (clean_name);
		g_free (uri);
		g_object_unref (parent);
	}

	gth_catalog_set_name (data->catalog, gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("name_entry"))));

	date_time = gth_datetime_new ();
	gth_time_selector_get_value (GTH_TIME_SELECTOR (data->time_selector), date_time);
	gth_catalog_set_date (data->catalog, date_time);
	gth_datetime_free (date_time);

	/* invoke the hook to save derived catalogs such as searches */

	gth_hook_invoke ("dlg-catalog-properties-save", data->builder, data->file_data, data->catalog);

	gio_file = gth_catalog_file_to_gio_file (data->file_data->file);
	buffer = gth_catalog_to_data (data->catalog, &size);
	_g_file_write_async (gio_file,
			     buffer,
			     size,
			     TRUE,
			     G_PRIORITY_DEFAULT,
			     NULL,
			     catalog_saved_cb,
			     data);

	g_object_unref (gio_file);
}


static void
catalog_ready_cb (GObject  *object,
		  GError   *error,
		  gpointer  user_data)
{
	DialogData *data = user_data;

	if (error != NULL) {
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW(data->browser), _("Could not load the catalog"), error);
		gtk_widget_destroy (data->dialog);
		return;
	}

	g_assert (object != NULL);
	data->catalog = GTH_CATALOG (g_object_ref (object));

	if (gth_catalog_get_name (data->catalog) != NULL) {
		gtk_entry_set_text (GTK_ENTRY (GET_WIDGET ("name_entry")), gth_catalog_get_name (data->catalog));
	}
	else if (! gth_datetime_valid_date (gth_catalog_get_date (data->catalog))) {
		char *basename;
		char *name;
		char *utf8_name;

		basename = g_file_get_basename (data->file_data->file);
		name = _g_path_remove_extension (basename);
		utf8_name = g_filename_to_utf8 (name, -1, NULL, NULL, NULL);
		gtk_entry_set_text (GTK_ENTRY (GET_WIDGET ("name_entry")), utf8_name);

		g_free (utf8_name);
		g_free (name);
		g_free (basename);
	}

	gth_time_selector_set_value (GTH_TIME_SELECTOR (data->time_selector), gth_catalog_get_date (data->catalog));
	gth_hook_invoke ("dlg-catalog-properties", data->builder, data->file_data, data->catalog);
	gtk_widget_show (data->dialog);

	g_object_unref (object);
}


void
dlg_catalog_properties (GthBrowser  *browser,
			GthFileData *file_data)
{
	DialogData *data;

	g_return_if_fail (file_data != NULL);

	data = g_new0 (DialogData, 1);
	data->browser = browser;
	data->file_data = gth_file_data_dup (file_data);
	data->original_file = g_file_dup (data->file_data->file);
	data->builder = _gtk_builder_new_from_file ("catalog-properties.ui", "catalogs");

	/* Set widgets data. */

	data->dialog = g_object_new (GTK_TYPE_DIALOG,
				     "title", _("Properties"),
				     "transient-for", GTK_WINDOW (browser),
				     "modal", FALSE,
				     "destroy-with-parent", FALSE,
				     "use-header-bar", _gtk_settings_get_dialogs_use_header (),
				     NULL);
	gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (data->dialog))),
			   _gtk_builder_get_widget (data->builder, "dialog_content"));
	gtk_dialog_add_buttons (GTK_DIALOG (data->dialog),
				_GTK_LABEL_CANCEL, GTK_RESPONSE_CANCEL,
				_GTK_LABEL_SAVE, GTK_RESPONSE_OK,
				NULL);
	_gtk_dialog_add_class_to_response (GTK_DIALOG (data->dialog), GTK_RESPONSE_OK, GTK_STYLE_CLASS_SUGGESTED_ACTION);

	data->time_selector = gth_time_selector_new ();
	gth_time_selector_show_time (GTH_TIME_SELECTOR (data->time_selector), FALSE, FALSE);
	gtk_widget_show (data->time_selector);
	gtk_box_pack_start (GTK_BOX (GET_WIDGET ("date_container_box")), data->time_selector, TRUE, TRUE, 0);

	/* Set the signals handlers. */

	g_signal_connect (G_OBJECT (data->dialog),
			  "destroy",
			  G_CALLBACK (destroy_cb),
			  data);
	g_signal_connect (gtk_dialog_get_widget_for_response (GTK_DIALOG (data->dialog), GTK_RESPONSE_OK),
			  "clicked",
			  G_CALLBACK (save_button_clicked_cb),
			  data);
	g_signal_connect_swapped (gtk_dialog_get_widget_for_response (GTK_DIALOG (data->dialog), GTK_RESPONSE_CANCEL),
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  data->dialog);

	/* run dialog. */

	gtk_widget_grab_focus (GET_WIDGET ("name_entry"));
	gth_catalog_load_from_file_async (file_data->file,
					  NULL,
					  catalog_ready_cb,
					  data);
}
