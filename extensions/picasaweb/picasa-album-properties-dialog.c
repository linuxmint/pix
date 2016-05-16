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
#include "picasa-album-properties-dialog.h"


#define GET_WIDGET(x) (_gtk_builder_get_widget (self->priv->builder, (x)))


G_DEFINE_TYPE (PicasaAlbumPropertiesDialog, picasa_album_properties_dialog, GTK_TYPE_DIALOG)


struct _PicasaAlbumPropertiesDialogPrivate {
	GtkBuilder *builder;
};


static void
picasa_album_properties_dialog_finalize (GObject *object)
{
	PicasaAlbumPropertiesDialog *self;

	self = PICASA_ALBUM_PROPERTIES_DIALOG (object);
	_g_object_unref (self->priv->builder);

	G_OBJECT_CLASS (picasa_album_properties_dialog_parent_class)->finalize (object);
}


static void
picasa_album_properties_dialog_class_init (PicasaAlbumPropertiesDialogClass *klass)
{
	GObjectClass *object_class;

	g_type_class_add_private (klass, sizeof (PicasaAlbumPropertiesDialogPrivate));

	object_class = (GObjectClass*) klass;
	object_class->finalize = picasa_album_properties_dialog_finalize;
}


static void
picasa_album_properties_dialog_init (PicasaAlbumPropertiesDialog *self)
{
	GtkWidget *content;

	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, PICASA_TYPE_ALBUM_PROPERTIES_DIALOG, PicasaAlbumPropertiesDialogPrivate);
	self->priv->builder = _gtk_builder_new_from_file ("picasa-web-album-properties.ui", "picasaweb");

	gtk_window_set_resizable (GTK_WINDOW (self), FALSE);
	gtk_box_set_spacing (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (self))), 5);
	gtk_container_set_border_width (GTK_CONTAINER (self), 5);

	content = _gtk_builder_get_widget (self->priv->builder, "album_properties");
	gtk_container_set_border_width (GTK_CONTAINER (content), 5);
  	gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (self))), content, TRUE, TRUE, 0);

	gtk_dialog_add_buttons (GTK_DIALOG (self),
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				GTK_STOCK_OK, GTK_RESPONSE_OK,
				NULL);
	gtk_dialog_set_default_response (GTK_DIALOG (self), GTK_RESPONSE_OK);
}


static void
picasa_album_properties_dialog_construct (PicasaAlbumPropertiesDialog *self,
				          const char                  *name,
				          const char                  *description,
				          PicasaWebAccess              access)
{
	if (name != NULL)
		gtk_entry_set_text (GTK_ENTRY (GET_WIDGET ("name_entry")), name);
	if (description != NULL)
		gtk_entry_set_text (GTK_ENTRY (GET_WIDGET ("description_entry")), description);
	gtk_combo_box_set_active (GTK_COMBO_BOX (GET_WIDGET ("visibility_combobox")), (access == PICASA_WEB_ACCESS_PUBLIC) ? 0 : 1);
}


GtkWidget *
picasa_album_properties_dialog_new (const char      *name,
				    const char      *description,
				    PicasaWebAccess  access)
{
	PicasaAlbumPropertiesDialog *self;

	self = g_object_new (PICASA_TYPE_ALBUM_PROPERTIES_DIALOG, NULL);
	picasa_album_properties_dialog_construct (self, name, description, access);

	return (GtkWidget *) self;
}


const char *
picasa_album_properties_dialog_get_name (PicasaAlbumPropertiesDialog *self)
{
	return gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("name_entry")));
}


const char *
picasa_album_properties_dialog_get_description (PicasaAlbumPropertiesDialog *self)
{
	return gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("description_entry")));
}


PicasaWebAccess
picasa_album_properties_dialog_get_access (PicasaAlbumPropertiesDialog *self)
{
	if (gtk_combo_box_get_active (GTK_COMBO_BOX (GET_WIDGET ("visibility_combobox"))) == 0)
		return PICASA_WEB_ACCESS_PUBLIC;
	else
		return PICASA_WEB_ACCESS_PRIVATE;
}
