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

#ifndef PICASA_WEB_PHOTO_H
#define PICASA_WEB_PHOTO_H

#include <glib.h>
#include <glib-object.h>
#include "picasa-web-types.h"

G_BEGIN_DECLS

#define PICASA_WEB_TYPE_PHOTO            (picasa_web_photo_get_type ())
#define PICASA_WEB_PHOTO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PICASA_WEB_TYPE_PHOTO, PicasaWebPhoto))
#define PICASA_WEB_PHOTO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), PICASA_WEB_TYPE_PHOTO, PicasaWebPhotoClass))
#define PICASA_WEB_IS_PHOTO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PICASA_WEB_TYPE_PHOTO))
#define PICASA_WEB_IS_PHOTO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PICASA_WEB_TYPE_PHOTO))
#define PICASA_WEB_PHOTO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), PICASA_WEB_TYPE_PHOTO, PicasaWebPhotoClass))

typedef struct _PicasaWebPhoto PicasaWebPhoto;
typedef struct _PicasaWebPhotoClass PicasaWebPhotoClass;
typedef struct _PicasaWebPhotoPrivate PicasaWebPhotoPrivate;

struct _PicasaWebPhoto {
	GObject parent_instance;
	PicasaWebPhotoPrivate *priv;

	char            *etag;
	char            *id;
	char            *album_id;
	char            *title;
	char            *summary;
	char            *uri;
	char            *mime_type;
	PicasaWebAccess  access;
	char            *credit;
	char            *description;
	char            *keywords;
	char            *thumbnail_72;
	char            *thumbnail_144;
	char            *thumbnail_288;
	float            position;
	guint32          rotation;
	goffset          size;
};

struct _PicasaWebPhotoClass {
	GObjectClass parent_class;
};

GType             picasa_web_photo_get_type          (void);
PicasaWebPhoto *  picasa_web_photo_new               (void);
void              picasa_web_photo_set_etag          (PicasaWebPhoto *self,
						      const char     *value);
void              picasa_web_photo_set_id            (PicasaWebPhoto *self,
						      const char     *value);
void              picasa_web_photo_set_album_id      (PicasaWebPhoto *self,
						      const char     *value);
void              picasa_web_photo_set_title         (PicasaWebPhoto *self,
						      const char     *value);
void              picasa_web_photo_set_summary       (PicasaWebPhoto *self,
						      const char     *value);
void              picasa_web_photo_set_uri           (PicasaWebPhoto *self,
						      const char     *value);
void              picasa_web_photo_set_mime_type     (PicasaWebPhoto *self,
						      const char     *value);
void              picasa_web_photo_set_access        (PicasaWebPhoto *self,
						      const char     *value);
void              picasa_web_photo_set_credit        (PicasaWebPhoto *self,
						      const char     *value);
void              picasa_web_photo_set_description   (PicasaWebPhoto *self,
						      const char     *value);
void              picasa_web_photo_set_keywords      (PicasaWebPhoto *self,
						      const char     *value);
void              picasa_web_photo_set_thumbnail_72  (PicasaWebPhoto *self,
						      const char     *value);
void              picasa_web_photo_set_thumbnail_144 (PicasaWebPhoto *self,
						      const char     *value);
void              picasa_web_photo_set_thumbnail_288 (PicasaWebPhoto *self,
						      const char     *value);
void              picasa_web_photo_set_position      (PicasaWebPhoto *self,
						      const char     *value);
void              picasa_web_photo_set_rotation      (PicasaWebPhoto *self,
						      const char     *value);
void              picasa_web_photo_set_size          (PicasaWebPhoto *self,
						      const char     *value);

G_END_DECLS

#endif /* PICASA_WEB_PHOTO_H */
