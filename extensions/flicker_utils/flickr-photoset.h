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

#ifndef FLICKR_PHOTOSET_H
#define FLICKR_PHOTOSET_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define FLICKR_TYPE_PHOTOSET            (flickr_photoset_get_type ())
#define FLICKR_PHOTOSET(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), FLICKR_TYPE_PHOTOSET, FlickrPhotoset))
#define FLICKR_PHOTOSET_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), FLICKR_TYPE_PHOTOSET, FlickrPhotosetClass))
#define FLICKR_IS_PHOTOSET(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FLICKR_TYPE_PHOTOSET))
#define FLICKR_IS_PHOTOSET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), FLICKR_TYPE_PHOTOSET))
#define FLICKR_PHOTOSET_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), FLICKR_TYPE_PHOTOSET, FlickrPhotosetClass))

typedef struct _FlickrPhotoset FlickrPhotoset;
typedef struct _FlickrPhotosetClass FlickrPhotosetClass;

struct _FlickrPhotoset {
	GObject parent_instance;

	char *id;
	char *title;
	char *description;
	int   n_photos;
	char *primary;
	char *secret;
	char *server;
	char *farm;
	char *url;
};

struct _FlickrPhotosetClass {
	GObjectClass parent_class;
};

GType             flickr_photoset_get_type          (void);
FlickrPhotoset *  flickr_photoset_new               (void);
void              flickr_photoset_set_id            (FlickrPhotoset *self,
						     const char     *value);
void              flickr_photoset_set_title         (FlickrPhotoset *self,
						     const char     *value);
void              flickr_photoset_set_description   (FlickrPhotoset *self,
						     const char     *value);
void              flickr_photoset_set_n_photos      (FlickrPhotoset *self,
						     const char     *value);
void              flickr_photoset_set_primary       (FlickrPhotoset *self,
						     const char     *value);
void              flickr_photoset_set_secret        (FlickrPhotoset *self,
						     const char     *value);
void              flickr_photoset_set_server        (FlickrPhotoset *self,
						     const char     *value);
void              flickr_photoset_set_farm          (FlickrPhotoset *self,
						     const char     *value);
void              flickr_photoset_set_url           (FlickrPhotoset *self,
						     const char     *value);

G_END_DECLS

#endif /* FLICKR_PHOTOSET_H */
