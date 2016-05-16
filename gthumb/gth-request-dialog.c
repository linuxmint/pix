/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2011 Free Software Foundation, Inc.
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
#include "gth-request-dialog.h"
#include "gtk-utils.h"


#define REQUEST_ENTRY_WIDTH_IN_CHARS 40


struct _GthRequestDialogPrivate {
	GtkWidget *entry;
	GtkWidget *infobar;
	GtkWidget *info_label;
};


G_DEFINE_TYPE (GthRequestDialog, gth_request_dialog, GTK_TYPE_DIALOG)


static void
gth_request_dialog_finalize (GObject *base)
{
	/* GthRequestDialog *self = (GthRequestDialog *) base; */

	G_OBJECT_CLASS (gth_request_dialog_parent_class)->finalize (base);
}


static void
gth_request_dialog_class_init (GthRequestDialogClass *klass)
{
	g_type_class_add_private (klass, sizeof (GthRequestDialogPrivate));

	G_OBJECT_CLASS (klass)->finalize = gth_request_dialog_finalize;
}


static void
gth_request_dialog_init (GthRequestDialog *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_REQUEST_DIALOG, GthRequestDialogPrivate);

	gtk_window_set_title (GTK_WINDOW (self), "");
	gtk_window_set_resizable (GTK_WINDOW (self), TRUE);
}


static void
_gth_request_dialog_construct (GthRequestDialog *self,
			       GtkWindow        *parent,
			       GtkDialogFlags    flags,
			       const char       *title,
			       const char       *prompt,
			       const char       *cancel_button_text,
			       const char       *ok_button_text)
{
	GtkWidget     *title_label;
	PangoAttrList *attributes;
	GtkWidget     *title_container;
	GtkWidget     *prompt_label;
	GtkWidget     *image;
	GtkWidget     *hbox;
	GtkWidget     *vbox;
	GtkWidget     *button;

	if (parent)
		gtk_window_set_transient_for (GTK_WINDOW (self), parent);

	if (flags & GTK_DIALOG_MODAL)
		gtk_window_set_modal (GTK_WINDOW (self), TRUE);

	if (flags & GTK_DIALOG_DESTROY_WITH_PARENT)
		gtk_window_set_destroy_with_parent (GTK_WINDOW (self), TRUE);

	gtk_window_set_resizable (GTK_WINDOW (self), FALSE);

	/* Add label and image */

	image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_QUESTION, GTK_ICON_SIZE_DIALOG);
	gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0.0);

	title_label = gtk_label_new (title);

	attributes = pango_attr_list_new ();
	pango_attr_list_insert (attributes, pango_attr_size_new (15000));
	gtk_label_set_attributes (GTK_LABEL (title_label), attributes);
	pango_attr_list_unref (attributes);

	gtk_label_set_line_wrap (GTK_LABEL (title_label), TRUE);
	gtk_label_set_selectable (GTK_LABEL (title_label), FALSE);
	gtk_misc_set_alignment (GTK_MISC (title_label), 0.0, 0.5);

	prompt_label = gtk_label_new (prompt);
	gtk_label_set_line_wrap (GTK_LABEL (prompt_label), TRUE);
	gtk_label_set_selectable (GTK_LABEL (prompt_label), FALSE);
	gtk_misc_set_alignment (GTK_MISC (prompt_label), 0.0, 0.5);

	self->priv->entry = gtk_entry_new ();
	gtk_entry_set_width_chars (GTK_ENTRY (self->priv->entry), REQUEST_ENTRY_WIDTH_IN_CHARS);
	gtk_entry_set_activates_default (GTK_ENTRY (self->priv->entry), TRUE);

	self->priv->infobar = gtk_info_bar_new ();

	self->priv->info_label = gtk_label_new ("");
	gtk_label_set_line_wrap (GTK_LABEL (self->priv->info_label), TRUE);
	gtk_container_add (GTK_CONTAINER (gtk_info_bar_get_content_area (GTK_INFO_BAR (self->priv->infobar))), self->priv->info_label);

	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);

	gtk_container_set_border_width (GTK_CONTAINER (hbox), 6);
	gtk_box_set_spacing (GTK_BOX (hbox), 12);
	gtk_box_set_spacing (GTK_BOX (vbox), 6);

	title_container = gtk_alignment_new (0.0, 0.0, 1.0, 1.0);
	gtk_alignment_set_padding (GTK_ALIGNMENT (title_container), 0, 12, 0, 0);
	gtk_container_add (GTK_CONTAINER (title_container), title_label);

	gtk_box_pack_start (GTK_BOX (vbox), title_container,
			    TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), prompt_label,
			    TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), self->priv->entry,
			    FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), self->priv->infobar,
			    FALSE, FALSE, 0);

	gtk_box_pack_start (GTK_BOX (hbox), image,
			    FALSE, FALSE, 0);

	gtk_box_pack_start (GTK_BOX (hbox), vbox,
			    TRUE, TRUE, 0);

	gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (self))),
			    hbox,
			    FALSE, FALSE, 0);

	gtk_widget_show_all (hbox);
	gtk_widget_hide (self->priv->infobar);

	/* Add buttons */

	button = _gtk_button_new_from_stock_with_text (GTK_STOCK_CANCEL, cancel_button_text);
	gtk_dialog_add_action_widget (GTK_DIALOG (self),
				      button,
				      GTK_RESPONSE_CANCEL);

	button = _gtk_button_new_from_stock_with_text (GTK_STOCK_OK, ok_button_text);
	gtk_dialog_add_action_widget (GTK_DIALOG (self),
				      button,
				      GTK_RESPONSE_OK);

	gtk_dialog_set_default_response (GTK_DIALOG (self), GTK_RESPONSE_OK);
}


GtkWidget *
gth_request_dialog_new (GtkWindow      *parent,
		    	GtkDialogFlags  flags,
		    	const char     *message,
		    	const char     *secondary_message,
		    	const char     *cancel_button_text,
		    	const char     *ok_button_text)
{
	GtkWidget *self;

	self = g_object_new (GTH_TYPE_REQUEST_DIALOG, NULL);
	_gth_request_dialog_construct (GTH_REQUEST_DIALOG (self),
				       parent,
				       flags,
				       message,
				       secondary_message,
				       cancel_button_text,
				       ok_button_text);

	return self;
}


GtkWidget *
gth_request_dialog_get_entry (GthRequestDialog *self)
{
	return self->priv->entry;
}


char *
gth_request_dialog_get_normalized_text (GthRequestDialog *self)
{
	char       *result = NULL;
	const char *text;

	text = gtk_entry_get_text (GTK_ENTRY (self->priv->entry));
	if ((text != NULL) && (strlen (text) > 0))
		/* Normalize unicode text to "NFC" form for consistency. */
		result = g_utf8_normalize (text, -1, G_NORMALIZE_NFC);
	else
		result = NULL;

	return result;
}


GtkWidget *
gth_request_dialog_get_info_bar (GthRequestDialog *self)
{
	return self->priv->infobar;
}


GtkWidget *
gth_request_dialog_get_info_label (GthRequestDialog *self)
{
	return self->priv->info_label;
}


void
gth_request_dialog_set_info_text (GthRequestDialog *self,
				  GtkMessageType    message_type,
				  const char       *text)
{
	gtk_info_bar_set_message_type (GTK_INFO_BAR (self->priv->infobar), message_type);
	gtk_label_set_text (GTK_LABEL (self->priv->info_label), text);
	gtk_widget_show (self->priv->infobar);
}


void
gth_request_dialog_set_info_markup (GthRequestDialog *self,
				    GtkMessageType    message_type,
				    const char       *markup)
{
	gtk_info_bar_set_message_type (GTK_INFO_BAR (self->priv->infobar), message_type);
	gtk_label_set_markup (GTK_LABEL (self->priv->info_label), markup);
	gtk_widget_show (self->priv->infobar);
}
