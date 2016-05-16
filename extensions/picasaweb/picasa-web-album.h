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

#ifndef PICASA_WEB_ALBUM_H
#define PICASA_WEB_ALBUM_H

#include <glib.h>
#include <glib-object.h>
#include "picasa-web-types.h"

G_BEGIN_DECLS

#define PICASA_WEB_TYPE_ALBUM            (picasa_web_album_get_type ())
#define PICASA_WEB_ALBUM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PICASA_WEB_TYPE_ALBUM, PicasaWebAlbum))
#define PICASA_WEB_ALBUM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), PICASA_WEB_TYPE_ALBUM, PicasaWebAlbumClass))
#define PICASA_WEB_IS_ALBUM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PICASA_WEB_TYPE_ALBUM))
#define PICASA_WEB_IS_ALBUM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PICASA_WEB_TYPE_ALBUM))
#define PICASA_WEB_ALBUM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), PICASA_WEB_TYPE_ALBUM, PicasaWebAlbumClass))

typedef struct _PicasaWebAlbum PicasaWebAlbum;
typedef struct _PicasaWebAlbumClass PicasaWebAlbumClass;
typedef struct _PicasaWebAlbumPrivate PicasaWebAlbumPrivate;

struct _PicasaWebAlbum {
	GObject parent_instance;
	PicasaWebAlbumPrivate *priv;

	char            *etag;
	char            *id;
	char            *title;
	char            *summary;
	char            *location;
	char            *alternate_url;
	char            *edit_url;
	PicasaWebAccess  access;
	int              n_photos;
	int              n_photos_remaining;
	goffset          used_bytes;
	char            *keywords;
};

struct _PicasaWebAlbumClass {
	GObjectClass parent_class;
};

GType             picasa_web_album_get_type          (void);
PicasaWebAlbum *  picasa_web_album_new               (void);
void              picasa_web_album_set_etag          (PicasaWebAlbum *self,
						      const char     *value);
void              picasa_web_album_set_id            (PicasaWebAlbum *self,
						      const char     *value);
void              picasa_web_album_set_title         (PicasaWebAlbum *self,
						      const char     *value);
void              picasa_web_album_set_summary       (PicasaWebAlbum *self,
						      const char     *value);
void              picasa_web_album_set_location      (PicasaWebAlbum *self,
						      const char     *value);
void              picasa_web_album_set_alternate_url (PicasaWebAlbum *self,
						      const char     *value);
void              picasa_web_album_set_edit_url      (PicasaWebAlbum *self,
						      const char     *value);
void              picasa_web_album_set_access        (PicasaWebAlbum *self,
						      const char     *value);
void              picasa_web_album_set_used_bytes    (PicasaWebAlbum *self,
						      const char     *value);
void              picasa_web_album_set_n_photos      (PicasaWebAlbum *self,
						      const char     *value);
void              picasa_web_album_set_n_photos_remaining
						     (PicasaWebAlbum *self,
						      const char     *value);
void              picasa_web_album_set_keywords      (PicasaWebAlbum *self,
						      const char     *value);

G_END_DECLS

#endif /* PICASA_WEB_ALBUM_H */
