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

#ifndef FLICKR_PHOTO_H
#define FLICKR_PHOTO_H

#include <glib.h>
#include <glib-object.h>
#include "flickr-types.h"

G_BEGIN_DECLS

typedef enum {
	FLICKR_URL_SQ,
	FLICKR_URL_T,
	FLICKR_URL_S,
	FLICKR_URL_M,
	FLICKR_URL_Z,
	FLICKR_URL_B,
	FLICKR_URL_O,
	FLICKR_URLS
} FlickrUrl;

#define FLICKR_TYPE_PHOTO            (flickr_photo_get_type ())
#define FLICKR_PHOTO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), FLICKR_TYPE_PHOTO, FlickrPhoto))
#define FLICKR_PHOTO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), FLICKR_TYPE_PHOTO, FlickrPhotoClass))
#define FLICKR_IS_PHOTO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FLICKR_TYPE_PHOTO))
#define FLICKR_IS_PHOTO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), FLICKR_TYPE_PHOTO))
#define FLICKR_PHOTO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), FLICKR_TYPE_PHOTO, FlickrPhotoClass))

typedef struct _FlickrPhoto FlickrPhoto;
typedef struct _FlickrPhotoClass FlickrPhotoClass;
typedef struct _FlickrPhotoPrivate FlickrPhotoPrivate;

struct _FlickrPhoto {
	GObject parent_instance;
	FlickrPhotoPrivate *priv;

	char            *id;
	char            *secret;
	char            *server;
	char            *farm;
	char            *title;
	gboolean         is_primary;
	char            *url[FLICKR_URLS];
	char            *original_format;
	char            *original_secret;
	char            *mime_type;
	int              position;
};

struct _FlickrPhotoClass {
	GObjectClass parent_class;
};

GType             flickr_photo_get_type             (void);
FlickrPhoto *     flickr_photo_new                  (FlickrServer *server);
void              flickr_photo_set_id               (FlickrPhoto  *self,
					             const char   *value);
void              flickr_photo_set_secret           (FlickrPhoto  *self,
					             const char   *value);
void              flickr_photo_set_server           (FlickrPhoto  *self,
					             const char   *value);
void              flickr_photo_set_farm             (FlickrPhoto  *self,
					             const char   *value);
void              flickr_photo_set_title            (FlickrPhoto  *self,
					             const char   *value);
void              flickr_photo_set_is_primary       (FlickrPhoto  *self,
					             const char   *value);
void              flickr_photo_set_url              (FlickrPhoto  *self,
						     FlickrUrl     size,
					             const char   *value);
void              flickr_photo_set_original_format  (FlickrPhoto  *self,
					             const char   *value);
void              flickr_photo_set_original_secret  (FlickrPhoto  *self,
						     const char   *value);

G_END_DECLS

#endif /* FLICKR_PHOTO_H */
