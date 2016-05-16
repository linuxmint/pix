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
#include "gth-embedded-dialog.h"
#include "gth-location-chooser.h"


struct _GthEmbeddedDialogPrivate {
	GtkWidget *icon_image;
	GtkWidget *primary_text_label;
	GtkWidget *secondary_text_label;
	GtkWidget *info_box;
	GtkWidget *location_chooser;
};


G_DEFINE_TYPE (GthEmbeddedDialog, gth_embedded_dialog, GEDIT_TYPE_MESSAGE_AREA)


static void
gth_embedded_dialog_class_init (GthEmbeddedDialogClass *klass)
{
	g_type_class_add_private (klass, sizeof (GthEmbeddedDialogPrivate));
}


static void
gth_embedded_dialog_init (GthEmbeddedDialog *self)
{
	GtkWidget *hbox_content;
	GtkWidget *image;
	GtkWidget *to_center_labels_box;
	GtkWidget *label_box;
	GtkWidget *primary_label;
	GtkWidget *secondary_label;
	
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_EMBEDDED_DIALOG, GthEmbeddedDialogPrivate);

	hbox_content = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_widget_show (hbox_content);

	self->priv->info_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_box_pack_start (GTK_BOX (hbox_content), self->priv->info_box, TRUE, TRUE, 0);

	self->priv->icon_image = image = gtk_image_new ();
	gtk_box_pack_start (GTK_BOX (self->priv->info_box), image, FALSE, FALSE, 0);
	gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0.5);

	to_center_labels_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_show (to_center_labels_box);
	gtk_box_pack_start (GTK_BOX (self->priv->info_box), to_center_labels_box, TRUE, TRUE, 0);

	label_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
	gtk_widget_show (label_box);
	gtk_box_pack_start (GTK_BOX (to_center_labels_box), label_box, TRUE, FALSE, 0);

	self->priv->primary_text_label = primary_label = gtk_label_new (NULL);
	gtk_box_pack_start (GTK_BOX (label_box), primary_label, FALSE, FALSE, 0);
	gtk_label_set_use_markup (GTK_LABEL (primary_label), TRUE);
	gtk_label_set_line_wrap (GTK_LABEL (primary_label), FALSE);
	gtk_label_set_ellipsize (GTK_LABEL (primary_label), PANGO_ELLIPSIZE_MIDDLE);
	gtk_misc_set_alignment (GTK_MISC (primary_label), 0, 0.5);
	gtk_widget_set_can_focus (primary_label, TRUE);
	gtk_label_set_selectable (GTK_LABEL (primary_label), TRUE);
	
	self->priv->secondary_text_label = secondary_label = gtk_label_new (NULL);
	gtk_box_pack_start (GTK_BOX (label_box), secondary_label, FALSE, FALSE, 0);
	gtk_widget_set_can_focus (secondary_label, TRUE);
	gtk_label_set_use_markup (GTK_LABEL (secondary_label), TRUE);
	gtk_label_set_line_wrap (GTK_LABEL (secondary_label), FALSE);
	gtk_label_set_selectable (GTK_LABEL (secondary_label), TRUE);
	gtk_label_set_ellipsize (GTK_LABEL (secondary_label), PANGO_ELLIPSIZE_END);
	gtk_misc_set_alignment (GTK_MISC (secondary_label), 0, 0.5);
	
	self->priv->location_chooser = g_object_new (GTH_TYPE_LOCATION_CHOOSER,
						     "show-entry-points", FALSE,
						     "relief", GTK_RELIEF_NONE,
						     NULL);
	gtk_box_pack_start (GTK_BOX (hbox_content), self->priv->location_chooser, FALSE, FALSE, 0);

	gedit_message_area_set_contents (GEDIT_MESSAGE_AREA (self), hbox_content);
}


GtkWidget *
gth_embedded_dialog_new (void)
{
	return g_object_new (GTH_TYPE_EMBEDDED_DIALOG, NULL);
}


void
gth_embedded_dialog_set_icon (GthEmbeddedDialog *self,
			      const char        *icon_stock_id,
			      GtkIconSize        size)
{
	if (icon_stock_id == NULL) {
		gtk_widget_hide (self->priv->icon_image);
		return;
	}

	gtk_image_set_from_stock (GTK_IMAGE (self->priv->icon_image), icon_stock_id, size);
	gtk_widget_show (self->priv->icon_image);
}


void
gth_embedded_dialog_set_gicon (GthEmbeddedDialog *self,
			       GIcon             *icon,
			       GtkIconSize        size)
{
	if (icon == NULL) {
		gtk_widget_hide (self->priv->icon_image);
		return;
	}

	gtk_image_set_from_gicon (GTK_IMAGE (self->priv->icon_image), icon, size);
	gtk_widget_show (self->priv->icon_image);
}


void
gth_embedded_dialog_set_primary_text (GthEmbeddedDialog *self,
				      const char        *text)
{
	char *escaped;
	char *markup;

	gtk_widget_hide (self->priv->location_chooser);
	gtk_widget_show (self->priv->info_box);

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
gth_embedded_dialog_set_secondary_text (GthEmbeddedDialog *self,
					const char        *text)
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


void
gth_embedded_dialog_set_from_file (GthEmbeddedDialog *self,
				   GFile             *file)
{
	gtk_widget_hide (self->priv->info_box);
	gtk_widget_show (self->priv->location_chooser);
	gth_location_chooser_set_current (GTH_LOCATION_CHOOSER (self->priv->location_chooser), file);
}


GtkWidget *
gth_embedded_dialog_get_chooser (GthEmbeddedDialog *self)
{
	return self->priv->location_chooser;
}
