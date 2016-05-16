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
#include "glib-utils.h"
#include "gth-image-loader.h"
#include "gth-image-viewer.h"
#include "gth-metadata-provider.h"
#include "gth-overwrite-dialog.h"
#include "gtk-utils.h"


#define ICON_SIZE 128
#define PREVIEW_SIZE 256


struct _GthOverwriteDialogPrivate {
	GtkBuilder     *builder;
	GFile          *source;
	GthImage       *source_image;
	GFile          *destination;
	GtkWidget      *old_image_viewer;
	GtkWidget      *new_image_viewer;
	GthFileData    *source_data;
	GthFileData    *destination_data;
	GthImageLoader *old_image_loader;
	GthImageLoader *new_image_loader;
};


G_DEFINE_TYPE (GthOverwriteDialog, gth_overwrite_dialog, GTK_TYPE_DIALOG)


static void
gth_overwrite_dialog_finalize (GObject *object)
{
	GthOverwriteDialog *dialog;

	dialog = GTH_OVERWRITE_DIALOG (object);

	_g_object_unref (dialog->priv->source_data);
	_g_object_unref (dialog->priv->destination_data);
	g_object_unref (dialog->priv->builder);
	_g_object_unref (dialog->priv->source);
	_g_object_unref (dialog->priv->source_image);
	g_object_unref (dialog->priv->destination);
	_g_object_unref (dialog->priv->old_image_loader);
	_g_object_unref (dialog->priv->new_image_loader);

	G_OBJECT_CLASS (gth_overwrite_dialog_parent_class)->finalize (object);
}


static void
gth_overwrite_dialog_class_init (GthOverwriteDialogClass *klass)
{
	g_type_class_add_private (klass, sizeof (GthOverwriteDialogPrivate));
	G_OBJECT_CLASS (klass)->finalize = gth_overwrite_dialog_finalize;
}


static void
gth_overwrite_dialog_init (GthOverwriteDialog *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_OVERWRITE_DIALOG, GthOverwriteDialogPrivate);
	self->priv->source_data = NULL;
	self->priv->destination_data = NULL;
}


static void
image_loader_ready_cb (GObject      *source_object,
                       GAsyncResult *result,
                       gpointer      user_data)
{
	GthOverwriteDialog *self = user_data;
	GError             *error = NULL;
	GthImage           *image = NULL;
	GdkPixbuf          *pixbuf;
	GtkWidget          *viewer;

	if (! gth_image_loader_load_finish (GTH_IMAGE_LOADER (source_object),
					    result,
					    &image,
					    NULL,
					    NULL,
					    &error))
	{
		return;
	}

	pixbuf = gth_image_get_pixbuf (image);

	if (GTH_IMAGE_LOADER (source_object) == self->priv->old_image_loader)
		viewer = self->priv->old_image_viewer;
	else
		viewer = self->priv->new_image_viewer;
	gth_image_viewer_set_pixbuf (GTH_IMAGE_VIEWER (viewer), pixbuf, -1, -1);

	g_object_unref (pixbuf);
	g_object_unref (image);
}


static void
info_ready_cb (GList    *files,
	       GError   *error,
	       gpointer  user_data)
{
	GthOverwriteDialog *self = user_data;
	char               *text;
	GTimeVal           *timeval;
	GIcon              *icon;
	GdkPixbuf          *pixbuf;

	if (error != NULL) {
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (self), _("Cannot read file information"), error);
		return;
	}

	/* new image */

	if (self->priv->source != NULL) {
		self->priv->source_data = g_object_ref (files->data);

		gtk_label_set_text (GTK_LABEL (_gtk_builder_get_widget (self->priv->builder, "new_image_filename_label")), g_file_info_get_display_name (self->priv->source_data->info));

		text = g_format_size (g_file_info_get_size (self->priv->source_data->info));
		gtk_label_set_text (GTK_LABEL (_gtk_builder_get_widget (self->priv->builder, "new_image_size_label")), text);
		g_free (text);

		timeval = gth_file_data_get_modification_time (self->priv->source_data);
		text = _g_time_val_to_exif_date (timeval);
		gtk_label_set_text (GTK_LABEL (_gtk_builder_get_widget (self->priv->builder, "new_image_time_label")), text);
		g_free (text);

		icon = (GIcon*) g_file_info_get_attribute_object (self->priv->source_data->info, "preview::icon");
		if (icon == NULL)
			icon = g_content_type_get_icon (g_file_info_get_content_type (self->priv->source_data->info));
		pixbuf = _g_icon_get_pixbuf (icon, ICON_SIZE, _gtk_widget_get_icon_theme (GTK_WIDGET (self)));
		if (pixbuf != NULL) {
			gth_image_viewer_set_pixbuf (GTH_IMAGE_VIEWER (self->priv->new_image_viewer), pixbuf, -1, -1);
			g_object_unref (pixbuf);
		}

		gtk_widget_show (_gtk_builder_get_widget (self->priv->builder, "new_filename_label"));
		gtk_widget_show (_gtk_builder_get_widget (self->priv->builder, "new_size_label"));
		gtk_widget_show (_gtk_builder_get_widget (self->priv->builder, "new_modified_label"));

		gth_image_loader_load (GTH_IMAGE_LOADER (self->priv->new_image_loader),
				       self->priv->source_data,
				       PREVIEW_SIZE,
				       G_PRIORITY_DEFAULT,
				       NULL, /* FIXME: make this cancellable */
				       image_loader_ready_cb,
				       self);
	}
	else if (self->priv->source_image != NULL) {
		gtk_widget_hide (_gtk_builder_get_widget (self->priv->builder, "new_filename_label"));
		gtk_widget_hide (_gtk_builder_get_widget (self->priv->builder, "new_size_label"));
		gtk_widget_hide (_gtk_builder_get_widget (self->priv->builder, "new_modified_label"));
		gth_image_viewer_set_image (GTH_IMAGE_VIEWER (self->priv->new_image_viewer), self->priv->source_image, -1, -1);
	}

	/* old image  */

	if (self->priv->source != NULL)
		self->priv->destination_data = g_object_ref (files->next->data);
	else
		self->priv->destination_data = g_object_ref (files->data);

	gtk_label_set_text (GTK_LABEL (_gtk_builder_get_widget (self->priv->builder, "old_image_filename_label")), g_file_info_get_display_name (self->priv->destination_data->info));

	text = g_format_size (g_file_info_get_size (self->priv->destination_data->info));
	gtk_label_set_text (GTK_LABEL (_gtk_builder_get_widget (self->priv->builder, "old_image_size_label")), text);
	g_free (text);

	timeval = gth_file_data_get_modification_time (self->priv->destination_data);
	text = _g_time_val_to_exif_date (timeval);
	gtk_label_set_text (GTK_LABEL (_gtk_builder_get_widget (self->priv->builder, "old_image_time_label")), text);
	g_free (text);

	gtk_entry_set_text (GTK_ENTRY (_gtk_builder_get_widget (self->priv->builder, "overwrite_rename_entry")), g_file_info_get_edit_name (self->priv->destination_data->info));

	icon = (GIcon*) g_file_info_get_attribute_object (self->priv->destination_data->info, "preview::icon");
	if (icon == NULL)
		icon = g_content_type_get_icon (g_file_info_get_content_type (self->priv->destination_data->info));
	pixbuf = _g_icon_get_pixbuf (icon, ICON_SIZE, _gtk_widget_get_icon_theme (GTK_WIDGET (self)));
	if (pixbuf != NULL) {
		gth_image_viewer_set_pixbuf (GTH_IMAGE_VIEWER (self->priv->old_image_viewer), pixbuf, -1, -1);
		g_object_unref (pixbuf);
	}

	gth_image_loader_load (GTH_IMAGE_LOADER (self->priv->old_image_loader),
			       self->priv->destination_data,
			       PREVIEW_SIZE,
			       G_PRIORITY_DEFAULT,
			       NULL, /* FIXME: make this cancellable */
			       image_loader_ready_cb,
			       self);
}


static void
overwrite_rename_radiobutton_toggled_cb (GtkToggleButton *button,
					 gpointer         user_data)
{
	GthOverwriteDialog *self = user_data;

	gtk_widget_set_sensitive (_gtk_builder_get_widget (self->priv->builder, "overwrite_rename_entry"),
				  gtk_toggle_button_get_active (button));
}


static void
gth_overwrite_dialog_construct (GthOverwriteDialog   *self,
				GthOverwriteResponse  default_response,
				gboolean              single_file)
{
	GtkWidget *box;
	GtkWidget *overwrite_radiobutton;
	GList     *files;

	gtk_window_set_title (GTK_WINDOW (self), "");
	gtk_window_set_resizable (GTK_WINDOW (self), TRUE);
	gtk_box_set_spacing (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (self))), 5);
	gtk_container_set_border_width (GTK_CONTAINER (self), 5);

	gtk_dialog_add_button (GTK_DIALOG (self), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
	gtk_dialog_add_button (GTK_DIALOG (self), GTK_STOCK_OK, GTK_RESPONSE_OK);
	gtk_dialog_set_default_response (GTK_DIALOG (self), GTK_RESPONSE_OK);

	self->priv->builder = _gtk_builder_new_from_file ("overwrite-dialog.ui", NULL);
	box = _gtk_builder_get_widget (self->priv->builder, "overwrite_dialog_box");
	gtk_widget_show (box);
	gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (self))), box, TRUE, TRUE, 0);

	switch (default_response) {
	case GTH_OVERWRITE_RESPONSE_YES:
	default:
		overwrite_radiobutton = _gtk_builder_get_widget (self->priv->builder, "overwrite_yes_radiobutton");
		break;
	case GTH_OVERWRITE_RESPONSE_NO:
	case GTH_OVERWRITE_RESPONSE_UNSPECIFIED:
		overwrite_radiobutton = _gtk_builder_get_widget (self->priv->builder, "overwrite_no_radiobutton");
		break;
	case GTH_OVERWRITE_RESPONSE_ALWAYS_YES:
		overwrite_radiobutton = _gtk_builder_get_widget (self->priv->builder, "overwrite_all_radiobutton");
		break;
	case GTH_OVERWRITE_RESPONSE_ALWAYS_NO:
		overwrite_radiobutton = _gtk_builder_get_widget (self->priv->builder, "overwrite_none_radiobutton");
		break;
	case GTH_OVERWRITE_RESPONSE_RENAME:
		overwrite_radiobutton = _gtk_builder_get_widget (self->priv->builder, "overwrite_rename_radiobutton");
		break;
	}
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (overwrite_radiobutton), TRUE);

	if (single_file) {
		gtk_widget_hide (_gtk_builder_get_widget (self->priv->builder, "overwrite_all_radiobutton"));
		gtk_widget_hide (_gtk_builder_get_widget (self->priv->builder, "overwrite_none_radiobutton"));
	}

	gtk_widget_set_sensitive (_gtk_builder_get_widget (self->priv->builder, "overwrite_rename_entry"), default_response == GTH_OVERWRITE_RESPONSE_RENAME);
	if (default_response == GTH_OVERWRITE_RESPONSE_RENAME)
		gtk_widget_grab_focus (_gtk_builder_get_widget (self->priv->builder, "overwrite_rename_entry"));

	gtk_widget_set_size_request (_gtk_builder_get_widget (self->priv->builder, "old_image_frame"), PREVIEW_SIZE, PREVIEW_SIZE);
	gtk_widget_set_size_request (_gtk_builder_get_widget (self->priv->builder, "new_image_frame"), PREVIEW_SIZE, PREVIEW_SIZE);

	self->priv->old_image_viewer = gth_image_viewer_new ();
	gth_image_viewer_set_transp_type (GTH_IMAGE_VIEWER (self->priv->old_image_viewer), GTH_TRANSP_TYPE_NONE);
	gth_image_viewer_set_fit_mode (GTH_IMAGE_VIEWER (self->priv->old_image_viewer), GTH_FIT_SIZE_IF_LARGER);
	gth_image_viewer_hide_frame (GTH_IMAGE_VIEWER (self->priv->old_image_viewer));
	gtk_widget_show (self->priv->old_image_viewer);
	gtk_container_add (GTK_CONTAINER (_gtk_builder_get_widget (self->priv->builder, "old_image_frame")), self->priv->old_image_viewer);

	self->priv->new_image_viewer = gth_image_viewer_new ();
	gth_image_viewer_set_transp_type (GTH_IMAGE_VIEWER (self->priv->new_image_viewer), GTH_TRANSP_TYPE_NONE);
	gth_image_viewer_set_fit_mode (GTH_IMAGE_VIEWER (self->priv->new_image_viewer), GTH_FIT_SIZE_IF_LARGER);
	gth_image_viewer_hide_frame (GTH_IMAGE_VIEWER (self->priv->new_image_viewer));
	gtk_widget_show (self->priv->new_image_viewer);
	gtk_container_add (GTK_CONTAINER (_gtk_builder_get_widget (self->priv->builder, "new_image_frame")), self->priv->new_image_viewer);

	g_signal_connect (_gtk_builder_get_widget (self->priv->builder, "overwrite_rename_radiobutton"),
			  "toggled",
			  G_CALLBACK (overwrite_rename_radiobutton_toggled_cb),
			  self);

	self->priv->old_image_loader = gth_image_loader_new (NULL, NULL);
	self->priv->new_image_loader = gth_image_loader_new (NULL, NULL);

	files = NULL;
	if (self->priv->source != NULL)
		files = g_list_append (files, self->priv->source);
	files = g_list_append (files, self->priv->destination);
	_g_query_all_metadata_async (files,
				     GTH_LIST_DEFAULT,
				     "standard::*,time::modified,time::modified-usec,preview::icon",
				     NULL,
				     info_ready_cb,
				     self);

	g_list_free (files);
}


GtkWidget *
gth_overwrite_dialog_new (GFile                *source,
			  GthImage             *source_image,
			  GFile                *destination,
			  GthOverwriteResponse  default_respose,
			  gboolean              single_file)
{
	GthOverwriteDialog *self;

	self = g_object_new (GTH_TYPE_OVERWRITE_DIALOG, NULL);
	self->priv->source = _g_object_ref (source);
	self->priv->source_image = _g_object_ref (source_image);
	self->priv->destination = g_object_ref (destination);
	gth_overwrite_dialog_construct (self, default_respose, single_file);

	return (GtkWidget *) self;
}


GthOverwriteResponse
gth_overwrite_dialog_get_response (GthOverwriteDialog *self)
{
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (_gtk_builder_get_widget (self->priv->builder, "overwrite_yes_radiobutton"))))
		return GTH_OVERWRITE_RESPONSE_YES;
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (_gtk_builder_get_widget (self->priv->builder, "overwrite_no_radiobutton"))))
		return GTH_OVERWRITE_RESPONSE_NO;
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (_gtk_builder_get_widget (self->priv->builder, "overwrite_all_radiobutton"))))
		return GTH_OVERWRITE_RESPONSE_ALWAYS_YES;
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (_gtk_builder_get_widget (self->priv->builder, "overwrite_none_radiobutton"))))
		return GTH_OVERWRITE_RESPONSE_ALWAYS_NO;
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (_gtk_builder_get_widget (self->priv->builder, "overwrite_rename_radiobutton"))))
		return GTH_OVERWRITE_RESPONSE_RENAME;
	return GTH_OVERWRITE_RESPONSE_UNSPECIFIED;
}


const char *
gth_overwrite_dialog_get_filename (GthOverwriteDialog *self)
{
	return gtk_entry_get_text (GTK_ENTRY (_gtk_builder_get_widget (self->priv->builder, "overwrite_rename_entry")));
}
