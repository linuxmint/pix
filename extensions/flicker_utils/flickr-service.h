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

#ifndef FLICKR_SERVICE_H
#define FLICKR_SERVICE_H

#include <gthumb.h>
#include <extensions/oauth/oauth.h>
#include "flickr-photoset.h"
#include "flickr-types.h"

#define FLICKR_TYPE_SERVICE         (flickr_service_get_type ())
#define FLICKR_SERVICE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), FLICKR_TYPE_SERVICE, FlickrService))
#define FLICKR_SERVICE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), FLICKR_TYPE_SERVICE, FlickrServiceClass))
#define FLICKR_IS_SERVICE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), FLICKR_TYPE_SERVICE))
#define FLICKR_IS_SERVICE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), FLICKR_TYPE_SERVICE))
#define FLICKR_SERVICE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), FLICKR_TYPE_SERVICE, FlickrServiceClass))

typedef struct _FlickrService         FlickrService;
typedef struct _FlickrServicePrivate  FlickrServicePrivate;
typedef struct _FlickrServiceClass    FlickrServiceClass;

struct _FlickrService {
	OAuthService __parent;
	FlickrServicePrivate *priv;
};

struct _FlickrServiceClass {
	OAuthServiceClass __parent_class;
};

GType             flickr_service_get_type                 (void) G_GNUC_CONST;
FlickrService *   flickr_service_new                      (FlickrServer         *server,
							   GCancellable         *cancellable,
							   GtkWidget            *browser,
							   GtkWidget            *dialog);
FlickrServer *    flickr_service_get_server               (FlickrService        *self);
void              flickr_service_list_photosets           (FlickrService        *self,
						           GCancellable         *cancellable,
						           GAsyncReadyCallback   callback,
						           gpointer              user_data);
GList *           flickr_service_list_photosets_finish    (FlickrService        *self,
						           GAsyncResult         *result,
						           GError              **error);
void              flickr_service_create_photoset          (FlickrService        *self,
						           FlickrPhotoset       *photoset,
						           GCancellable         *cancellable,
						           GAsyncReadyCallback   callback,
						           gpointer              user_data);
FlickrPhotoset *  flickr_service_create_photoset_finish   (FlickrService        *self,
						           GAsyncResult         *result,
						           GError              **error);
void              flickr_service_add_photos_to_set        (FlickrService        *self,
						           FlickrPhotoset       *photoset,
						           GList                *photo_ids,
						           GCancellable         *cancellable,
						           GAsyncReadyCallback   callback,
						           gpointer              user_data);
gboolean          flickr_service_add_photos_to_set_finish (FlickrService        *self,
						           GAsyncResult         *result,
						           GError              **error);
void              flickr_service_post_photos              (FlickrService        *self,
							   FlickrPrivacy         privacy_level,
							   FlickrSafety          safety_level,
							   gboolean              hidden,
							   int                   max_width,
							   int                   max_height,
						           GList                *file_list, /* GFile list */
						           GCancellable         *cancellable,
						           GAsyncReadyCallback   callback,
						           gpointer              user_data);
GList *           flickr_service_post_photos_finish       (FlickrService        *self,
						           GAsyncResult         *result,
						           GError              **error);
void              flickr_service_list_photos              (FlickrService        *self,
							   FlickrPhotoset       *photoset,
							   const char           *extras,
						           GCancellable         *cancellable,
						           GAsyncReadyCallback   callback,
						           gpointer              user_data);
GList *           flickr_service_list_photos_finish       (FlickrService        *self,
						           GAsyncResult         *result,
						           GError              **error);

#endif /* FLICKR_SERVICE_H */
