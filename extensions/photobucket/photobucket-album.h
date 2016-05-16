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

#ifndef PHOTOBUCKET_ALBUM_H
#define PHOTOBUCKET_ALBUM_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define PHOTOBUCKET_TYPE_ALBUM            (photobucket_album_get_type ())
#define PHOTOBUCKET_ALBUM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PHOTOBUCKET_TYPE_ALBUM, PhotobucketAlbum))
#define PHOTOBUCKET_ALBUM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), PHOTOBUCKET_TYPE_ALBUM, PhotobucketAlbumClass))
#define PHOTOBUCKET_IS_ALBUM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PHOTOBUCKET_TYPE_ALBUM))
#define PHOTOBUCKET_IS_ALBUM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PHOTOBUCKET_TYPE_ALBUM))
#define PHOTOBUCKET_ALBUM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), PHOTOBUCKET_TYPE_ALBUM, PhotobucketAlbumClass))

typedef struct _PhotobucketAlbum PhotobucketAlbum;
typedef struct _PhotobucketAlbumClass PhotobucketAlbumClass;

struct _PhotobucketAlbum {
	GObject parent_instance;

	char *name;
	int   photo_count;
	int   video_count;
};

struct _PhotobucketAlbumClass {
	GObjectClass parent_class;
};

GType              photobucket_album_get_type  (void);
PhotobucketAlbum * photobucket_album_new       (void);
void               photobucket_album_set_name  (PhotobucketAlbum *self,
						const char       *value);

G_END_DECLS

#endif /* PHOTOBUCKET_ALBUM_H */
