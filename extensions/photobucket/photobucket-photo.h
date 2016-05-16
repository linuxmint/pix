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

#ifndef PHOTOBUCKET_PHOTO_H
#define PHOTOBUCKET_PHOTO_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define PHOTOBUCKET_TYPE_PHOTO            (photobucket_photo_get_type ())
#define PHOTOBUCKET_PHOTO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PHOTOBUCKET_TYPE_PHOTO, PhotobucketPhoto))
#define PHOTOBUCKET_PHOTO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), PHOTOBUCKET_TYPE_PHOTO, PhotobucketPhotoClass))
#define PHOTOBUCKET_IS_PHOTO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PHOTOBUCKET_TYPE_PHOTO))
#define PHOTOBUCKET_IS_PHOTO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PHOTOBUCKET_TYPE_PHOTO))
#define PHOTOBUCKET_PHOTO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), PHOTOBUCKET_TYPE_PHOTO, PhotobucketPhotoClass))

typedef struct _PhotobucketPhoto PhotobucketPhoto;
typedef struct _PhotobucketPhotoClass PhotobucketPhotoClass;
typedef struct _PhotobucketPhotoPrivate PhotobucketPhotoPrivate;

struct _PhotobucketPhoto {
	GObject parent_instance;
	PhotobucketPhotoPrivate *priv;

	char     *name;
	gboolean  is_public;
	char     *browse_url;
	char     *url;
	char     *thumb_url;
	char     *description;
	char     *title;
	gboolean  is_sponsored;
};

struct _PhotobucketPhotoClass {
	GObjectClass parent_class;
};

GType                photobucket_photo_get_type          (void);
PhotobucketPhoto *   photobucket_photo_new               (void);
void                 photobucket_photo_set_name          (PhotobucketPhoto *self,
					                  const char       *value);
void                 photobucket_photo_set_is_public     (PhotobucketPhoto *self,
			                                  const char       *value);
void                 photobucket_photo_set_browse_url    (PhotobucketPhoto *self,
					                  const char       *value);
void                 photobucket_photo_set_url           (PhotobucketPhoto *self,
					                  const char       *value);
void                 photobucket_photo_set_thumb_url     (PhotobucketPhoto *self,
					                  const char       *value);
void                 photobucket_photo_set_description   (PhotobucketPhoto *self,
					                  const char       *value);
void                 photobucket_photo_set_title         (PhotobucketPhoto *self,
					                  const char       *value);
void                 photobucket_photo_set_is_sponsored  (PhotobucketPhoto *self,
			                                  const char       *value);

G_END_DECLS

#endif /* PHOTOBUCKET_PHOTO_H */
