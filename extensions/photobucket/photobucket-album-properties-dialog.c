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
#include "photobucket-album-properties-dialog.h"


#define GET_WIDGET(x) (_gtk_builder_get_widget (self->priv->builder, (x)))


G_DEFINE_TYPE (PhotobucketAlbumPropertiesDialog, photobucket_album_properties_dialog, GTK_TYPE_DIALOG)


enum {
	ALBUM_DATA_COLUMN,
	ALBUM_ICON_COLUMN,
	ALBUM_TITLE_COLUMN,
	ALBUM_N_PHOTOS_COLUMN
};


struct _PhotobucketAlbumPropertiesDialogPrivate {
	GtkBuilder *builder;
};


static void
photobucket_album_properties_dialog_finalize (GObject *object)
{
	PhotobucketAlbumPropertiesDialog *self;

	self = PHOTOBUCKET_ALBUM_PROPERTIES_DIALOG (object);
	_g_object_unref (self->priv->builder);

	G_OBJECT_CLASS (photobucket_album_properties_dialog_parent_class)->finalize (object);
}


static void
photobucket_album_properties_dialog_class_init (PhotobucketAlbumPropertiesDialogClass *klass)
{
	GObjectClass *object_class;

	g_type_class_add_private (klass, sizeof (PhotobucketAlbumPropertiesDialogPrivate));

	object_class = (GObjectClass*) klass;
	object_class->finalize = photobucket_album_properties_dialog_finalize;
}


static void
photobucket_album_properties_dialog_init (PhotobucketAlbumPropertiesDialog *self)
{
	GtkWidget *content;

	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, PHOTOBUCKET_TYPE_ALBUM_PROPERTIES_DIALOG, PhotobucketAlbumPropertiesDialogPrivate);
	self->priv->builder = _gtk_builder_new_from_file ("photobucket-album-properties.ui", "photobucket");

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


	{
		GtkCellLayout   *cell_layout;
		GtkCellRenderer *renderer;

		cell_layout = GTK_CELL_LAYOUT (GET_WIDGET ("album_combobox"));

		renderer = gtk_cell_renderer_pixbuf_new ();
		gtk_cell_layout_pack_start (cell_layout, renderer, FALSE);
		gtk_cell_layout_set_attributes (cell_layout, renderer,
						"icon-name", ALBUM_ICON_COLUMN,
						NULL);

		renderer = gtk_cell_renderer_text_new ();
		gtk_cell_layout_pack_start (cell_layout, renderer, TRUE);
		gtk_cell_layout_set_attributes (cell_layout, renderer,
						"text", ALBUM_TITLE_COLUMN,
						NULL);

		renderer = gtk_cell_renderer_text_new ();
		gtk_cell_layout_pack_start (cell_layout, renderer, FALSE);
		gtk_cell_layout_set_attributes (cell_layout, renderer,
						"text", ALBUM_N_PHOTOS_COLUMN,
						NULL);
	}
}


static void
photobucket_album_properties_dialog_construct (PhotobucketAlbumPropertiesDialog *self,
				               const char                       *name,
				               GList                            *albums)
{
	GList *scan;

	if (name != NULL)
		gtk_entry_set_text (GTK_ENTRY (GET_WIDGET ("name_entry")), name);

	gtk_list_store_clear (GTK_LIST_STORE (GET_WIDGET ("album_liststore")));
	for (scan = albums; scan; scan = scan->next) {
		PhotobucketAlbum *album = scan->data;
		char             *size;
		GtkTreeIter       iter;

		size = g_strdup_printf ("(%d)", album->photo_count + album->video_count);

		gtk_list_store_append (GTK_LIST_STORE (GET_WIDGET ("album_liststore")), &iter);
		gtk_list_store_set (GTK_LIST_STORE (GET_WIDGET ("album_liststore")), &iter,
				    ALBUM_DATA_COLUMN, album,
				    ALBUM_ICON_COLUMN, "file-catalog",
				    ALBUM_TITLE_COLUMN, album->name,
				    ALBUM_N_PHOTOS_COLUMN, size,
				    -1);

		g_free (size);
	}

	if (albums != NULL)
		gtk_combo_box_set_active (GTK_COMBO_BOX (GET_WIDGET ("album_combobox")), 0);
}


GtkWidget *
photobucket_album_properties_dialog_new (const char *name,
					 GList      *albums)
{
	PhotobucketAlbumPropertiesDialog *self;

	self = g_object_new (PHOTOBUCKET_TYPE_ALBUM_PROPERTIES_DIALOG, NULL);
	photobucket_album_properties_dialog_construct (self, name, albums);

	return (GtkWidget *) self;
}


const char *
photobucket_album_properties_dialog_get_name (PhotobucketAlbumPropertiesDialog *self)
{
	return gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("name_entry")));
}


char *
photobucket_album_properties_dialog_get_parent_album (PhotobucketAlbumPropertiesDialog *self)
{
	GtkTreeIter       iter;
	PhotobucketAlbum *album;
	char             *name;

	album = NULL;
	if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (GET_WIDGET ("album_combobox")), &iter)) {
		gtk_tree_model_get (gtk_combo_box_get_model (GTK_COMBO_BOX (GET_WIDGET ("album_combobox"))),
				    &iter,
				    ALBUM_DATA_COLUMN, &album,
				    -1);
	}

	if (album == NULL)
		return NULL;

	name = g_strdup (album->name);

	g_object_unref (album);

	return name;
}
