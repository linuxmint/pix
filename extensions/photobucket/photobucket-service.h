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

#ifndef PHOTOBUCKET_SERVICE_H
#define PHOTOBUCKET_SERVICE_H

#include <glib-object.h>
#include <extensions/oauth/oauth.h>
#include "photobucket-album.h"
#include "photobucket-account.h"

#define PHOTOBUCKET_TYPE_SERVICE         (photobucket_service_get_type ())
#define PHOTOBUCKET_SERVICE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), PHOTOBUCKET_TYPE_SERVICE, PhotobucketService))
#define PHOTOBUCKET_SERVICE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), PHOTOBUCKET_TYPE_SERVICE, PhotobucketServiceClass))
#define PHOTOBUCKET_IS_SERVICE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), PHOTOBUCKET_TYPE_SERVICE))
#define PHOTOBUCKET_IS_SERVICE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), PHOTOBUCKET_TYPE_SERVICE))
#define PHOTOBUCKET_SERVICE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), PHOTOBUCKET_TYPE_SERVICE, PhotobucketServiceClass))

typedef struct _PhotobucketService         PhotobucketService;
typedef struct _PhotobucketServicePrivate  PhotobucketServicePrivate;
typedef struct _PhotobucketServiceClass    PhotobucketServiceClass;

struct _PhotobucketService
{
	OAuthService __parent;
	PhotobucketServicePrivate *priv;
};

struct _PhotobucketServiceClass
{
	OAuthServiceClass __parent_class;
};

GType                photobucket_service_get_type                   (void) G_GNUC_CONST;
PhotobucketService * photobucket_service_new                        (GCancellable           *cancellable,
								     GtkWidget              *browser,
								     GtkWidget              *dialog);
void                 photobucket_service_get_albums                 (PhotobucketService     *self,
							             GCancellable           *cancellable,
							             GAsyncReadyCallback     callback,
							             gpointer                user_data);
GList *              photobucket_service_get_albums_finish          (PhotobucketService     *self,
							             GAsyncResult           *result,
							             GError                **error);
void                 photobucket_service_create_album               (PhotobucketService     *self,
								     const char             *parent_album,
						                     PhotobucketAlbum       *album,
						                     GCancellable           *cancellable,
						                     GAsyncReadyCallback     callback,
						                     gpointer                user_data);
PhotobucketAlbum *   photobucket_service_create_album_finish        (PhotobucketService     *self,
						                     GAsyncResult           *result,
						                     GError                **error);
void                 photobucket_service_upload_photos              (PhotobucketService     *self,
							             PhotobucketAlbum       *album,
							             int                     size,
							             gboolean                scramble,
							             GList                  *file_list, /* GFile list */
							             GCancellable           *cancellable,
							             GAsyncReadyCallback     callback,
							             gpointer                user_data);
gboolean             photobucket_service_upload_photos_finish       (PhotobucketService     *self,
						                     GAsyncResult           *result,
						                     GError                **error);

#endif /* PHOTOBUCKET_SERVICE_H */
