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

#ifndef FACEBOOK_ALBUM_H
#define FACEBOOK_ALBUM_H

#include <glib.h>
#include <glib-object.h>
#include "facebook-types.h"

G_BEGIN_DECLS

#define FACEBOOK_TYPE_ALBUM            (facebook_album_get_type ())
#define FACEBOOK_ALBUM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), FACEBOOK_TYPE_ALBUM, FacebookAlbum))
#define FACEBOOK_ALBUM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), FACEBOOK_TYPE_ALBUM, FacebookAlbumClass))
#define FACEBOOK_IS_ALBUM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FACEBOOK_TYPE_ALBUM))
#define FACEBOOK_IS_ALBUM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), FACEBOOK_TYPE_ALBUM))
#define FACEBOOK_ALBUM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), FACEBOOK_TYPE_ALBUM, FacebookAlbumClass))

typedef struct _FacebookAlbum FacebookAlbum;
typedef struct _FacebookAlbumClass FacebookAlbumClass;

struct _FacebookAlbum {
	GObject parent_instance;

	char     *id;
	char     *name;
	char     *description;
	char     *link;
	char     *privacy;
	int       count;
	gboolean  can_upload;
};

struct _FacebookAlbumClass {
	GObjectClass parent_class;
};

GType           facebook_album_get_type  (void);
FacebookAlbum * facebook_album_new       (void);

G_END_DECLS

#endif /* FACEBOOK_ALBUM_H */
