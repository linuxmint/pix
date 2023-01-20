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
#include <glib/gi18n.h>
#include "gth-import-destination-button.h"


struct _GthImportDestinationButtonPrivate {
	GtkWidget *destination_icon;
	GtkWidget *destination_label;
	GtkWidget *subfolder_label;
};


G_DEFINE_TYPE_WITH_CODE (GthImportDestinationButton,
			 gth_import_destination_button,
			 GTK_TYPE_BUTTON,
			 G_ADD_PRIVATE (GthImportDestinationButton))


static void
gth_import_destination_button_class_init (GthImportDestinationButtonClass *klass)
{
	/* void */
}


static void
gth_import_destination_button_init (GthImportDestinationButton *self)
{
	GtkWidget *box;
	GtkWidget *label_box;

	self->priv = gth_import_destination_button_get_instance_private (self);

	box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_widget_show (box);
	gtk_container_add (GTK_CONTAINER (self), box);

	self->priv->destination_icon = gtk_image_new ();
	gtk_widget_show (self->priv->destination_icon);
	gtk_box_pack_start (GTK_BOX (box), self->priv->destination_icon, FALSE, FALSE, 0);

	label_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_show (label_box);
	gtk_box_pack_start (GTK_BOX (box), label_box, TRUE, TRUE, 0);

	self->priv->destination_label = gtk_label_new ("");
	gtk_widget_show (self->priv->destination_label);
	gtk_box_pack_start (GTK_BOX (label_box), self->priv->destination_label, FALSE, FALSE, 0);

	self->priv->subfolder_label = gtk_label_new ("");
	gtk_style_context_add_class (gtk_widget_get_style_context (self->priv->subfolder_label), "highlighted-text");
	gtk_label_set_ellipsize (GTK_LABEL (self->priv->subfolder_label), PANGO_ELLIPSIZE_END);
	gtk_label_set_xalign (GTK_LABEL (self->priv->subfolder_label), 0.0);
	gtk_label_set_yalign (GTK_LABEL (self->priv->subfolder_label), 0.5);

	gtk_widget_show (self->priv->subfolder_label);
	gtk_box_pack_start (GTK_BOX (label_box), self->priv->subfolder_label, TRUE, TRUE, 0);
}


static void
preferences_dialog_destination_changed_cb (GthImportPreferencesDialog *dialog,
					   GthImportDestinationButton *self)
{
	GFile *destination;
	GFile *subfolder_example;

	destination = gth_import_preferences_dialog_get_destination (dialog);
	subfolder_example = gth_import_preferences_dialog_get_subfolder_example (dialog);
	if (destination != NULL) {
		char *destination_name;
		char *subfolder_name;

		gtk_image_set_from_icon_name (GTK_IMAGE (self->priv->destination_icon), "folder-symbolic", GTK_ICON_SIZE_MENU);

		destination_name = g_file_get_parse_name (destination);
		gtk_label_set_text (GTK_LABEL (self->priv->destination_label), destination_name);

		subfolder_name = g_file_get_parse_name (subfolder_example);
		if (! _g_str_empty (subfolder_name) && ! _g_str_equal (subfolder_name, "/")) {
			const char *name = g_str_has_suffix (destination_name, "/") ? subfolder_name + 1 : subfolder_name;
			gtk_label_set_text (GTK_LABEL (self->priv->subfolder_label), name);
		}
		else
			gtk_label_set_text (GTK_LABEL (self->priv->subfolder_label), "");

		g_free (subfolder_name);
		g_free (destination_name);
	}
	else {
		gtk_image_set_from_icon_name (GTK_IMAGE (self->priv->destination_icon), "dialog-error", GTK_ICON_SIZE_MENU);
		gtk_label_set_text (GTK_LABEL (self->priv->destination_label), _("Invalid Destination"));
		gtk_label_set_text (GTK_LABEL (self->priv->subfolder_label), "");
	}

	_g_object_unref (subfolder_example);
	_g_object_unref (destination);
}


static void
gth_import_destination_button_construct (GthImportDestinationButton *button,
					 GthImportPreferencesDialog *dialog)
{
	g_signal_connect (dialog,
			  "destination_changed",
			  G_CALLBACK (preferences_dialog_destination_changed_cb),
			  button);
	g_signal_connect_swapped (button,
			  	  "clicked",
			  	  G_CALLBACK (gtk_window_present),
			  	  dialog);
}


GtkWidget *
gth_import_destination_button_new (GthImportPreferencesDialog *dialog)
{
	GtkWidget *button;

	button = (GtkWidget *) g_object_new (GTH_TYPE_IMPORT_DESTINATION_BUTTON, NULL);
	gth_import_destination_button_construct (GTH_IMPORT_DESTINATION_BUTTON (button), dialog);

	return button;
}
