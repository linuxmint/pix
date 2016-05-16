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
#include <stdlib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include "glib-utils.h"
#include "gth-file-chooser-dialog.h"
#include "gth-main.h"
#include "gth-preferences.h"
#include "gtk-utils.h"


#define FORMAT_KEY "gthumb-format"


G_DEFINE_TYPE (GthFileChooserDialog, gth_file_chooser_dialog, GTK_TYPE_FILE_CHOOSER_DIALOG)


typedef struct {
	GthImageSaver  *saver;
	char          **extensions;
} Format;


static void
format_free (Format *format)
{
	g_object_unref (format->saver);
	g_strfreev (format->extensions);
	g_free (format);
}


struct _GthFileChooserDialogPrivate {
	GList     *supported_formats;
	GtkWidget *options_checkbutton;
};


static void
gth_file_chooser_dialog_finalize (GObject *object)
{
	GthFileChooserDialog *self;

	self = GTH_FILE_CHOOSER_DIALOG (object);

	g_list_foreach (self->priv->supported_formats, (GFunc) format_free, NULL);
	g_list_free (self->priv->supported_formats);

	G_OBJECT_CLASS (gth_file_chooser_dialog_parent_class)->finalize (object);
}


static void
gth_file_chooser_dialog_unmap (GtkWidget *widget)
{
	GthFileChooserDialog *self;
	GSettings            *settings;

	self = GTH_FILE_CHOOSER_DIALOG (widget);

	settings = g_settings_new (GTHUMB_SAVE_FILE_SCHEMA);
	g_settings_set_boolean (settings, PREF_SAVE_FILE_SHOW_OPTIONS, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->priv->options_checkbutton)));
	g_object_unref (settings);

	GTK_WIDGET_CLASS (gth_file_chooser_dialog_parent_class)->unmap (widget);
}


static void
gth_file_chooser_dialog_class_init (GthFileChooserDialogClass *class)
{
	GObjectClass   *object_class;
	GtkWidgetClass *widget_class;

	g_type_class_add_private (class, sizeof (GthFileChooserDialogPrivate));

	object_class = (GObjectClass*) class;
	object_class->finalize = gth_file_chooser_dialog_finalize;

	widget_class = (GtkWidgetClass*) class;
	widget_class->unmap = gth_file_chooser_dialog_unmap;
}


static void
gth_file_chooser_dialog_init (GthFileChooserDialog *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_FILE_CHOOSER_DIALOG, GthFileChooserDialogPrivate);
	self->priv->supported_formats = NULL;
}


static void
gth_file_chooser_dialog_construct (GthFileChooserDialog *self,
				   const char           *title,
			           GtkWindow            *parent,
				   const char           *allowed_savers)
{
	GtkFileFilter *filter;
	GArray        *savers;
	int            i;
	GList         *scan;
	GSettings     *settings;

	if (title != NULL)
    		gtk_window_set_title (GTK_WINDOW (self), title);
  	if (parent != NULL)
    		gtk_window_set_transient_for (GTK_WINDOW (self), parent);

  	gtk_file_chooser_set_action (GTK_FILE_CHOOSER (self), GTK_FILE_CHOOSER_ACTION_SAVE);
  	gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (self), FALSE);
	gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (self), TRUE);
	gtk_dialog_set_default_response (GTK_DIALOG (self), GTK_RESPONSE_ACCEPT);

	_gtk_dialog_add_to_window_group (GTK_DIALOG (self));

	gtk_dialog_add_button (GTK_DIALOG (self), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
	gtk_dialog_add_button (GTK_DIALOG (self), GTK_STOCK_SAVE, GTK_RESPONSE_OK);

	/* filters */

	filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, _("All Supported Files"));

	savers = gth_main_get_type_set (allowed_savers);
	for (i = 0; (savers != NULL) && (i < savers->len); i++) {
		Format  *format;
		int      e;

		format = g_new (Format, 1);
		format->saver = g_object_new (g_array_index (savers, GType, i), NULL);
		format->extensions = g_strsplit (gth_image_saver_get_extensions (format->saver), " ", -1);
		self->priv->supported_formats = g_list_prepend (self->priv->supported_formats, format);

		for (e = 0; format->extensions[e] != NULL; e++) {
			char *pattern = g_strconcat ("*.", format->extensions[e], NULL);
			gtk_file_filter_add_pattern (filter, pattern);

			g_free (pattern);
		}

		gtk_file_filter_add_mime_type (filter, gth_image_saver_get_mime_type (format->saver));
	}

	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (self), filter);
	gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (self), filter);

	self->priv->supported_formats = g_list_reverse (self->priv->supported_formats);
	for (scan = self->priv->supported_formats; scan; scan = scan->next) {
		Format *format = scan->data;
		int     i;

		filter = gtk_file_filter_new ();
		gtk_file_filter_set_name (filter, gth_image_saver_get_display_name (format->saver));

		for (i = 0; format->extensions[i] != NULL; i++) {
			char *pattern = g_strconcat ("*.", format->extensions[i], NULL);
			gtk_file_filter_add_pattern (filter, pattern);

			g_free (pattern);
		}

		gtk_file_filter_add_mime_type (filter, gth_image_saver_get_mime_type (format->saver));
		gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (self), filter);

		g_object_set_data (G_OBJECT (filter), FORMAT_KEY, format);
	}

	/* extra widget */

	settings = g_settings_new (GTHUMB_SAVE_FILE_SCHEMA);

	self->priv->options_checkbutton = gtk_check_button_new_with_mnemonic ("_Show Format Options");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->priv->options_checkbutton), g_settings_get_boolean (settings, PREF_SAVE_FILE_SHOW_OPTIONS));
	gtk_widget_show (self->priv->options_checkbutton);
	gtk_file_chooser_set_extra_widget (GTK_FILE_CHOOSER (self), self->priv->options_checkbutton);

	g_object_unref (settings);
}


GtkWidget *
gth_file_chooser_dialog_new (const char *title,
			     GtkWindow  *parent,
			     const char *allowed_savers)
{
	GthFileChooserDialog *self;

	self = g_object_new (GTH_TYPE_FILE_CHOOSER_DIALOG, NULL);
	gth_file_chooser_dialog_construct (self, title, parent, allowed_savers);

	return (GtkWidget *) self;
}


static Format *
get_format_from_extension (GthFileChooserDialog *self,
			   const char           *filename)
{
	const char *ext;
	GList      *scan;

	ext = _g_uri_get_file_extension (filename);
	if (ext == NULL)
		return NULL;

	if (ext[0] == '.')
		ext++;

	for (scan = self->priv->supported_formats; scan; scan = scan->next) {
		Format *format = scan->data;
		int     i;

		for (i = 0; format->extensions[i] != NULL; i++)
			if (g_ascii_strcasecmp (ext, format->extensions[i]) == 0)
				return format;
	}

	return NULL;
}


static gboolean
_gth_file_chooser_change_format_options (GthFileChooserDialog *self,
					 GthImageSaver        *saver)
{
	GtkWidget *d;
	GtkWidget *control;
	gboolean   result;

	d = gtk_dialog_new_with_buttons (_("Options"),
					 GTK_WINDOW (self),
					 GTK_DIALOG_MODAL,
					 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					 GTK_STOCK_OK, GTK_RESPONSE_OK,
					 NULL);
	_gtk_dialog_add_to_window_group (GTK_DIALOG (d));

	gtk_container_set_border_width (GTK_CONTAINER (d), 6);
	gtk_container_set_border_width (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (d))), 0);
	gtk_box_set_spacing (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (d))), 8);
	gtk_window_set_default_size (GTK_WINDOW (d), 400, -1);

	control = gth_image_saver_get_control (saver);
	if (control == NULL) {
		gtk_widget_destroy (d);
		return TRUE;
	}

	gtk_widget_show (control);
	gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (d))), control);

	result = gtk_dialog_run (GTK_DIALOG (d)) == GTK_RESPONSE_OK;
	if (result)
		gth_image_saver_save_options (saver);

	gtk_widget_destroy (d);

	return result;
}


gboolean
gth_file_chooser_dialog_get_file (GthFileChooserDialog  *self,
				  GFile                **file,
				  const char           **mime_type)
{
	char   *filename;
	Format *format;

	filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (self));
	format = get_format_from_extension (self, filename);
	g_free (filename);

	if (format == NULL) {
		GtkFileFilter *filter;

		filter = gtk_file_chooser_get_filter (GTK_FILE_CHOOSER (self));
		format = g_object_get_data (G_OBJECT (filter), FORMAT_KEY);
	}

	if (format == NULL)
		return FALSE;

	if (file != NULL)
		*file = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (self));
	if (mime_type != NULL)
		*mime_type = gth_image_saver_get_mime_type (format->saver);

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->priv->options_checkbutton)))
		return _gth_file_chooser_change_format_options (self, format->saver);
	else
		return TRUE;
}
