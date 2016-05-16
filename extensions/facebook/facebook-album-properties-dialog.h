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

#ifndef FACEBOOK_ALBUM_PROPERTIES_DIALOG_H
#define FACEBOOK_ALBUM_PROPERTIES_DIALOG_H

#include <gtk/gtk.h>
#include <gthumb.h>
#include "facebook-album.h"

G_BEGIN_DECLS

#define FACEBOOK_TYPE_ALBUM_PROPERTIES_DIALOG            (facebook_album_properties_dialog_get_type ())
#define FACEBOOK_ALBUM_PROPERTIES_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), FACEBOOK_TYPE_ALBUM_PROPERTIES_DIALOG, FacebookAlbumPropertiesDialog))
#define FACEBOOK_ALBUM_PROPERTIES_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), FACEBOOK_TYPE_ALBUM_PROPERTIES_DIALOG, FacebookAlbumPropertiesDialogClass))
#define FACEBOOK_IS_ALBUM_PROPERTIES_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FACEBOOK_TYPE_ALBUM_PROPERTIES_DIALOG))
#define FACEBOOK_IS_ALBUM_PROPERTIES_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), FACEBOOK_TYPE_ALBUM_PROPERTIES_DIALOG))
#define FACEBOOK_ALBUM_PROPERTIES_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), FACEBOOK_TYPE_ALBUM_PROPERTIES_DIALOG, FacebookAlbumPropertiesDialogClass))

typedef struct _FacebookAlbumPropertiesDialog FacebookAlbumPropertiesDialog;
typedef struct _FacebookAlbumPropertiesDialogClass FacebookAlbumPropertiesDialogClass;
typedef struct _FacebookAlbumPropertiesDialogPrivate FacebookAlbumPropertiesDialogPrivate;

struct _FacebookAlbumPropertiesDialog {
	GtkDialog parent_instance;
	FacebookAlbumPropertiesDialogPrivate *priv;
};

struct _FacebookAlbumPropertiesDialogClass {
	GtkDialogClass parent_class;
};

GType               facebook_album_properties_dialog_get_type        (void);
GtkWidget *         facebook_album_properties_dialog_new             (const char                    *name,
								      const char                    *description,
								      FacebookVisibility             visibility);
const char *        facebook_album_properties_dialog_get_name        (FacebookAlbumPropertiesDialog *self);
const char *        facebook_album_properties_dialog_get_description (FacebookAlbumPropertiesDialog *self);
const char *        facebook_album_properties_dialog_get_visibility  (FacebookAlbumPropertiesDialog *self);

G_END_DECLS

#endif /* FACEBOOK_ALBUM_PROPERTIES_DIALOG_H */
