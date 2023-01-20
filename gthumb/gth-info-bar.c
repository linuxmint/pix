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
#include "gth-info-bar.h"
#include "gtk-utils.h"


struct _GthInfoBarPrivate {
	GtkWidget *icon_image;
	GtkWidget *primary_text_label;
	GtkWidget *secondary_text_label;
};


G_DEFINE_TYPE_WITH_CODE (GthInfoBar,
			 gth_info_bar,
			 GTK_TYPE_INFO_BAR,
			 G_ADD_PRIVATE (GthInfoBar))


static void
gth_info_bar_class_init (GthInfoBarClass *klass)
{
	/* void */
}


static void
infobar_response_cb (GtkInfoBar *self,
		     int         response_id,
		     gpointer    user_data)
{
	switch (response_id) {
	case GTK_RESPONSE_CLOSE:
		gtk_widget_hide (GTK_WIDGET (self));
		break;
	}
}


static void
gth_info_bar_init (GthInfoBar *self)
{
	GtkWidget *hbox_content;
	GtkWidget *image;
	GtkWidget *vbox;
	GtkWidget *primary_label;
	GtkWidget *secondary_label;
	GtkWidget *area;

	self->priv = gth_info_bar_get_instance_private (self);

	hbox_content = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 8);
	gtk_widget_show (hbox_content);

	self->priv->icon_image = image = gtk_image_new ();
	gtk_box_pack_start (GTK_BOX (hbox_content), image, FALSE, FALSE, 0);
	gtk_widget_set_halign (image, GTK_ALIGN_CENTER);
	gtk_widget_set_valign (image, GTK_ALIGN_CENTER);

	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
	gtk_widget_show (vbox);
	gtk_box_pack_start (GTK_BOX (hbox_content), vbox, TRUE, TRUE, 0);

	self->priv->primary_text_label = primary_label = gtk_label_new (NULL);
	gtk_box_pack_start (GTK_BOX (vbox), primary_label, TRUE, TRUE, 0);
	gtk_widget_set_can_focus (primary_label, TRUE);
	gtk_label_set_use_markup (GTK_LABEL (primary_label), TRUE);
	gtk_label_set_line_wrap (GTK_LABEL (primary_label), TRUE);
	_gtk_widget_set_margin (primary_label, 0, 0, 0, 0);
	gtk_label_set_ellipsize (GTK_LABEL (primary_label), PANGO_ELLIPSIZE_MIDDLE);
	gtk_label_set_xalign (GTK_LABEL (primary_label), 0.0);
	gtk_label_set_yalign (GTK_LABEL (primary_label), 0.5);
	gtk_label_set_selectable (GTK_LABEL (primary_label), TRUE);

	self->priv->secondary_text_label = secondary_label = gtk_label_new (NULL);
	gtk_box_pack_start (GTK_BOX (vbox), secondary_label, TRUE, TRUE, 0);
	gtk_widget_set_can_focus (secondary_label, TRUE);
	gtk_label_set_use_markup (GTK_LABEL (secondary_label), TRUE);
	gtk_label_set_line_wrap (GTK_LABEL (secondary_label), TRUE);
	_gtk_widget_set_margin (secondary_label, 0, 0, 0, 0);
	gtk_label_set_ellipsize (GTK_LABEL (secondary_label), PANGO_ELLIPSIZE_END);
	gtk_label_set_xalign (GTK_LABEL (secondary_label), 0.0);
	gtk_label_set_yalign (GTK_LABEL (secondary_label), 0.5);
	gtk_label_set_selectable (GTK_LABEL (secondary_label), TRUE);

	area = gtk_info_bar_get_action_area (GTK_INFO_BAR (self));
	gtk_container_set_border_width (GTK_CONTAINER (self), 0);
	gtk_box_set_homogeneous (GTK_BOX (area), FALSE);
	gtk_orientable_set_orientation (GTK_ORIENTABLE (area), GTK_ORIENTATION_HORIZONTAL);

	area = gtk_info_bar_get_content_area (GTK_INFO_BAR (self));
	gtk_container_set_border_width (GTK_CONTAINER (self), 0);
	gtk_container_add (GTK_CONTAINER (area), hbox_content);

	gtk_widget_set_name (GTK_WIDGET (self), "GthInfoBar");
	gtk_info_bar_set_message_type (GTK_INFO_BAR (self), GTK_MESSAGE_OTHER);

	g_signal_connect (self,
			  "response",
			  G_CALLBACK (infobar_response_cb),
			  NULL);
}


GtkWidget *
gth_info_bar_new (void)
{
	return GTK_WIDGET (g_object_new (GTH_TYPE_INFO_BAR, NULL));
}


GtkWidget *
gth_info_bar_get_primary_label (GthInfoBar *dialog)
{
	return dialog->priv->primary_text_label;
}


void
gth_info_bar_set_icon_name (GthInfoBar  *self,
			    const char  *icon_name,
			    GtkIconSize  icon_size)
{
	if (icon_name == NULL) {
		gtk_widget_hide (self->priv->icon_image);
		return;
	}

	gtk_image_set_from_icon_name (GTK_IMAGE (self->priv->icon_image), icon_name, icon_size);
	gtk_widget_show (self->priv->icon_image);
}


void
gth_info_bar_set_gicon (GthInfoBar  *self,
			GIcon       *icon,
			GtkIconSize  icon_size)
{
	if (icon == NULL) {
		gtk_widget_hide (self->priv->icon_image);
		return;
	}

	gtk_image_set_from_gicon (GTK_IMAGE (self->priv->icon_image), icon, icon_size);
	gtk_widget_show (self->priv->icon_image);
}


void
gth_info_bar_set_primary_text (GthInfoBar *self,
			       const char *text)
{
	char *escaped;
	char *markup;

	if (text == NULL) {
		gtk_widget_hide (self->priv->primary_text_label);
		return;
	}

	escaped = g_markup_escape_text (text, -1);
	markup = g_strdup_printf ("<b>%s</b>", escaped);
	gtk_label_set_markup (GTK_LABEL (self->priv->primary_text_label), markup);
	gtk_widget_show (self->priv->primary_text_label);

	g_free (markup);
	g_free (escaped);
}


void
gth_info_bar_set_secondary_text (GthInfoBar *self,
				 const char *text)
{
	char *escaped;
	char *markup;

	if (text == NULL) {
		gtk_widget_hide (self->priv->secondary_text_label);
		return;
	}

	escaped = g_markup_escape_text (text, -1);
	markup = g_strdup_printf ("<small>%s</small>", escaped);
	gtk_label_set_markup (GTK_LABEL (self->priv->secondary_text_label), markup);
	gtk_widget_show (self->priv->secondary_text_label);

	g_free (markup);
	g_free (escaped);
}
