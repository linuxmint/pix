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

#ifndef PHOTOBUCKET_ALBUM_PROPERTIES_DIALOG_H
#define PHOTOBUCKET_ALBUM_PROPERTIES_DIALOG_H

#include <gtk/gtk.h>
#include <gthumb.h>
#include "photobucket-album.h"

G_BEGIN_DECLS

#define PHOTOBUCKET_TYPE_ALBUM_PROPERTIES_DIALOG            (photobucket_album_properties_dialog_get_type ())
#define PHOTOBUCKET_ALBUM_PROPERTIES_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PHOTOBUCKET_TYPE_ALBUM_PROPERTIES_DIALOG, PhotobucketAlbumPropertiesDialog))
#define PHOTOBUCKET_ALBUM_PROPERTIES_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), PHOTOBUCKET_TYPE_ALBUM_PROPERTIES_DIALOG, PhotobucketAlbumPropertiesDialogClass))
#define PHOTOBUCKET_IS_ALBUM_PROPERTIES_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PHOTOBUCKET_TYPE_ALBUM_PROPERTIES_DIALOG))
#define PHOTOBUCKET_IS_ALBUM_PROPERTIES_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PHOTOBUCKET_TYPE_ALBUM_PROPERTIES_DIALOG))
#define PHOTOBUCKET_ALBUM_PROPERTIES_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), PHOTOBUCKET_TYPE_ALBUM_PROPERTIES_DIALOG, PhotobucketAlbumPropertiesDialogClass))

typedef struct _PhotobucketAlbumPropertiesDialog PhotobucketAlbumPropertiesDialog;
typedef struct _PhotobucketAlbumPropertiesDialogClass PhotobucketAlbumPropertiesDialogClass;
typedef struct _PhotobucketAlbumPropertiesDialogPrivate PhotobucketAlbumPropertiesDialogPrivate;

struct _PhotobucketAlbumPropertiesDialog {
	GtkDialog parent_instance;
	PhotobucketAlbumPropertiesDialogPrivate *priv;
};

struct _PhotobucketAlbumPropertiesDialogClass {
	GtkDialogClass parent_class;
};

GType          photobucket_album_properties_dialog_get_type         (void);
GtkWidget *    photobucket_album_properties_dialog_new              (const char *name,
								     GList      *albums);
const char *   photobucket_album_properties_dialog_get_name         (PhotobucketAlbumPropertiesDialog *self);
char *         photobucket_album_properties_dialog_get_parent_album (PhotobucketAlbumPropertiesDialog *self);

G_END_DECLS

#endif /* PHOTOBUCKET_ALBUM_PROPERTIES_DIALOG_H */
