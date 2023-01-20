/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2019 The Free Software Foundation, Inc.
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
#include "gth-accel-dialog.h"
#include "gtk-utils.h"
#include "gth-shortcut.h"


struct _GthAccelDialogPrivate {
	guint           keycode;
	GdkModifierType modifiers;
	gboolean        valid;
};


G_DEFINE_TYPE_WITH_CODE (GthAccelDialog,
			 gth_accel_dialog,
			 GTK_TYPE_DIALOG,
			 G_ADD_PRIVATE (GthAccelDialog))


static void
gth_accel_dialog_finalize (GObject *object)
{
	/*GthAccelDialog *self;

	self = GTH_ACCEL_DIALOG (object);*/

	G_OBJECT_CLASS (gth_accel_dialog_parent_class)->finalize (object);
}


static void
gth_accel_dialog_class_init (GthAccelDialogClass *class)
{
	GObjectClass *object_class;

	object_class = (GObjectClass*) class;
	object_class->finalize = gth_accel_dialog_finalize;
}


static void
gth_accel_dialog_init (GthAccelDialog *self)
{
	self->priv = gth_accel_dialog_get_instance_private (self);
	self->priv->keycode = 0;
	self->priv->modifiers = 0;
	self->priv->valid = FALSE;
}


static gboolean
accel_dialog_keypress_cb (GtkWidget    *widget,
			  GdkEventKey  *event,
			  gpointer      user_data)
{
	GthAccelDialog  *self = user_data;
	GdkModifierType  modifiers;

	if (event->keyval == GDK_KEY_Escape)
		return FALSE;

	modifiers = event->state & gtk_accelerator_get_default_mod_mask ();
	if (gth_shortcut_valid (event->keyval, modifiers)) {
		self->priv->keycode = event->keyval;
		self->priv->modifiers = modifiers;
		self->priv->valid = TRUE;
		gtk_dialog_response (GTK_DIALOG (self), GTK_RESPONSE_OK);
	}
	else
		self->priv->valid = FALSE;

	return TRUE;
}


static void
gth_accel_dialog_construct (GthAccelDialog  *self,
			    const char      *title,
			    GtkWindow       *parent,
			    guint            keycode,
			    GdkModifierType  modifiers)
{
	gboolean   valid;
	GtkWidget *box;
	GtkWidget *label;
	GtkWidget *secondary_label;
	GtkWidget *content_area;

	valid = gth_shortcut_valid (keycode, modifiers);
	gtk_dialog_add_buttons (GTK_DIALOG (self),
				_GTK_LABEL_CANCEL, GTK_RESPONSE_CANCEL,
				! valid ? NULL : _GTK_LABEL_DELETE, GTH_ACCEL_BUTTON_RESPONSE_DELETE,
				NULL);

	content_area = gtk_dialog_get_content_area (GTK_DIALOG (self));

	label = gtk_label_new (_("Press a combination of keys to use as shortcut."));
	secondary_label = gtk_label_new (_("Press Esc to cancel"));
	gtk_style_context_add_class (gtk_widget_get_style_context (secondary_label), "dim-label");
	box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 50);
	gtk_widget_set_margin_top (box, 50);
	gtk_widget_set_margin_bottom (box, 50);
	gtk_widget_set_margin_start (box, 50);
	gtk_widget_set_margin_end (box, 50);
	gtk_box_pack_start (GTK_BOX (box), label, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (box), secondary_label, TRUE, TRUE, 0);

	g_signal_connect (self,
			  "key-press-event",
			  G_CALLBACK (accel_dialog_keypress_cb),
			  self);

	gtk_widget_show (box);
	gtk_widget_show (label);
	gtk_widget_show (secondary_label);
	gtk_container_add (GTK_CONTAINER (content_area), box);
}


GtkWidget *
gth_accel_dialog_new (const char      *title,
		      GtkWindow       *parent,
		      guint            keycode,
		      GdkModifierType  modifiers)
{
	GthAccelDialog *self;

	self = g_object_new (GTH_TYPE_ACCEL_DIALOG,
			     "use-header-bar", _gtk_settings_get_dialogs_use_header (),
			     "modal", TRUE,
			     "transient-for", parent,
			     "resizable", FALSE,
			     "title", title,
			     NULL);
	gth_accel_dialog_construct (self, title, parent, keycode, modifiers);

	return (GtkWidget *) self;
}


gboolean
gth_accel_dialog_get_accel (GthAccelDialog  *self,
			    guint           *keycode,
			    GdkModifierType *modifiers)
{
	if (self->priv->valid) {
		*keycode = self->priv->keycode;
		*modifiers = self->priv->modifiers;
	}
	else {
		*keycode = 0;
		*modifiers = 0;
	}

	return self->priv->valid;
}
