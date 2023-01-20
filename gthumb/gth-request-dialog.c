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


G_DEFINE_TYPE_WITH_CODE (GthRequestDialog,
			 gth_request_dialog,
			 GTK_TYPE_DIALOG,
			 G_ADD_PRIVATE (GthRequestDialog))


static void
gth_request_dialog_class_init (GthRequestDialogClass *klass)
{
	/* void */
}


static void
gth_request_dialog_init (GthRequestDialog *self)
{
	self->priv = gth_request_dialog_get_instance_private (self);
	gtk_window_set_title (GTK_WINDOW (self), "");
}


static void
_gth_request_dialog_construct (GthRequestDialog *self,
			       const char       *prompt,
			       const char       *cancel_button_text,
			       const char       *ok_button_text)
{
	GtkWidget     *prompt_label;
	GtkWidget     *hbox;
	GtkWidget     *vbox_entry;
	GtkWidget     *vbox;
	GtkWidget     *button;

	prompt_label = gtk_label_new (prompt);
	gtk_label_set_line_wrap (GTK_LABEL (prompt_label), TRUE);
	gtk_label_set_selectable (GTK_LABEL (prompt_label), FALSE);
	gtk_label_set_xalign (GTK_LABEL (prompt_label), 0.0);
	gtk_label_set_yalign (GTK_LABEL (prompt_label), 0.5);
	self->priv->entry = gtk_entry_new ();
	gtk_entry_set_width_chars (GTK_ENTRY (self->priv->entry), REQUEST_ENTRY_WIDTH_IN_CHARS);
	gtk_entry_set_activates_default (GTK_ENTRY (self->priv->entry), TRUE);

	self->priv->infobar = gtk_info_bar_new ();

	self->priv->info_label = gtk_label_new ("");
	gtk_label_set_line_wrap (GTK_LABEL (self->priv->info_label), TRUE);
	gtk_container_add (GTK_CONTAINER (gtk_info_bar_get_content_area (GTK_INFO_BAR (self->priv->infobar))), self->priv->info_label);

	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
	vbox_entry = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);

	gtk_container_set_border_width (GTK_CONTAINER (hbox), 15);
	gtk_box_set_spacing (GTK_BOX (hbox), 12);
	gtk_box_set_spacing (GTK_BOX (vbox), 6);

	gtk_box_pack_start (GTK_BOX (vbox_entry), prompt_label, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox_entry), self->priv->entry, FALSE, FALSE, 0);

	gtk_box_pack_start (GTK_BOX (vbox), vbox_entry, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), self->priv->infobar, FALSE, FALSE, 0);

	gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);

	gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (self))),
			    hbox,
			    FALSE,
			    FALSE,
			    0);

	gtk_widget_show_all (hbox);
	gtk_widget_hide (self->priv->infobar);

	/* Add buttons */

	button = gtk_button_new_with_mnemonic (cancel_button_text);
	gtk_widget_show (button);
	gtk_dialog_add_action_widget (GTK_DIALOG (self), button, GTK_RESPONSE_CANCEL);

	button = gtk_button_new_with_mnemonic (ok_button_text);
	gtk_widget_set_can_default (button, TRUE);
	gtk_widget_show (button);
	gtk_dialog_add_action_widget (GTK_DIALOG (self), button, GTK_RESPONSE_OK);

	gtk_dialog_set_default_response (GTK_DIALOG (self), GTK_RESPONSE_OK);
}


GtkWidget *
gth_request_dialog_new (GtkWindow      *parent,
		    	GtkDialogFlags  flags,
		    	const char     *title,
		    	const char     *secondary_message,
		    	const char     *cancel_button_text,
		    	const char     *ok_button_text)
{
	GtkWidget *self;

	self = g_object_new (GTH_TYPE_REQUEST_DIALOG,
			     "title", title,
			     "transient-for", parent,
			     "modal", (flags & GTK_DIALOG_MODAL),
			     "destroy-with-parent", (flags & GTK_DIALOG_DESTROY_WITH_PARENT),
			     "use-header-bar", _gtk_settings_get_dialogs_use_header (),
			     "resizable", FALSE,
			     NULL);
	_gth_request_dialog_construct (GTH_REQUEST_DIALOG (self),
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
