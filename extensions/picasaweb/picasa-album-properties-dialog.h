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
 
#ifndef PICASA_ALBUM_PROPERTIES_DIALOG_H
#define PICASA_ALBUM_PROPERTIES_DIALOG_H

#include <gtk/gtk.h>
#include <gthumb.h>
#include "picasa-web-album.h"

G_BEGIN_DECLS

#define PICASA_TYPE_ALBUM_PROPERTIES_DIALOG            (picasa_album_properties_dialog_get_type ())
#define PICASA_ALBUM_PROPERTIES_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PICASA_TYPE_ALBUM_PROPERTIES_DIALOG, PicasaAlbumPropertiesDialog))
#define PICASA_ALBUM_PROPERTIES_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), PICASA_TYPE_ALBUM_PROPERTIES_DIALOG, PicasaAlbumPropertiesDialogClass))
#define PICASA_IS_ALBUM_PROPERTIES_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PICASA_TYPE_ALBUM_PROPERTIES_DIALOG))
#define PICASA_IS_ALBUM_PROPERTIES_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PICASA_TYPE_ALBUM_PROPERTIES_DIALOG))
#define PICASA_ALBUM_PROPERTIES_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), PICASA_TYPE_ALBUM_PROPERTIES_DIALOG, PicasaAlbumPropertiesDialogClass))

typedef struct _PicasaAlbumPropertiesDialog PicasaAlbumPropertiesDialog;
typedef struct _PicasaAlbumPropertiesDialogClass PicasaAlbumPropertiesDialogClass;
typedef struct _PicasaAlbumPropertiesDialogPrivate PicasaAlbumPropertiesDialogPrivate;

struct _PicasaAlbumPropertiesDialog {
	GtkDialog parent_instance;
	PicasaAlbumPropertiesDialogPrivate *priv;
};

struct _PicasaAlbumPropertiesDialogClass {
	GtkDialogClass parent_class;
};

GType            picasa_album_properties_dialog_get_type        (void);
GtkWidget *      picasa_album_properties_dialog_new             (const char                  *name,
								 const char                  *description,
							         PicasaWebAccess              access);
const char *     picasa_album_properties_dialog_get_name        (PicasaAlbumPropertiesDialog *self);
const char *     picasa_album_properties_dialog_get_description (PicasaAlbumPropertiesDialog *self);
PicasaWebAccess  picasa_album_properties_dialog_get_access      (PicasaAlbumPropertiesDialog *self);

G_END_DECLS

#endif /* PICASA_ALBUM_PROPERTIES_DIALOG_H */
