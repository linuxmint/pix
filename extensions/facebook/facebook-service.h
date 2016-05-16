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

#ifndef FACEBOOK_SERVICE_H
#define FACEBOOK_SERVICE_H

#include <gtk/gtk.h>
#include <gthumb.h>
#include <extensions/oauth/oauth.h>
#include "facebook-album.h"
#include "facebook-types.h"

#define FACEBOOK_TYPE_SERVICE         (facebook_service_get_type ())
#define FACEBOOK_SERVICE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), FACEBOOK_TYPE_SERVICE, FacebookService))
#define FACEBOOK_SERVICE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), FACEBOOK_TYPE_SERVICE, FacebookServiceClass))
#define FACEBOOK_IS_SERVICE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), FACEBOOK_TYPE_SERVICE))
#define FACEBOOK_IS_SERVICE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), FACEBOOK_TYPE_SERVICE))
#define FACEBOOK_SERVICE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), FACEBOOK_TYPE_SERVICE, FacebookServiceClass))

typedef struct _FacebookService         FacebookService;
typedef struct _FacebookServicePrivate  FacebookServicePrivate;
typedef struct _FacebookServiceClass    FacebookServiceClass;

struct _FacebookService
{
	WebService __parent;
	FacebookServicePrivate *priv;
};

struct _FacebookServiceClass
{
	WebServiceClass __parent_class;
};

GType             facebook_service_get_type                   (void) G_GNUC_CONST;
FacebookService * facebook_service_new                        (GCancellable         *cancellable,
							       GtkWidget            *browser,
							       GtkWidget            *dialog);
void              facebook_service_get_albums                 (FacebookService      *self,
							       GCancellable         *cancellable,
							       GAsyncReadyCallback   callback,
							       gpointer              user_data);
GList *           facebook_service_get_albums_finish          (FacebookService      *self,
							       GAsyncResult         *result,
							       GError              **error);
void              facebook_service_create_album               (FacebookService      *self,
						               FacebookAlbum        *album,
						               GCancellable         *cancellable,
						               GAsyncReadyCallback   callback,
						               gpointer              user_data);
FacebookAlbum *   facebook_service_create_album_finish        (FacebookService      *self,
						               GAsyncResult         *result,
						               GError              **error);
void              facebook_service_upload_photos              (FacebookService      *self,
							       FacebookAlbum        *album,
							       GList                *file_list, /* GFile list */
							       int                   max_resolution,
							       GCancellable         *cancellable,
							       GAsyncReadyCallback   callback,
							       gpointer              user_data);
GList *           facebook_service_upload_photos_finish       (FacebookService      *self,
						               GAsyncResult         *result,
						               GError              **error);
void              facebook_service_list_photos                (FacebookService      *self,
							       FacebookAlbum        *album,
							       int                   limit,
							       const char           *after,
							       GCancellable         *cancellable,
							       GAsyncReadyCallback   callback,
							       gpointer              user_data);
GList *           facebook_service_list_photos_finish         (FacebookService      *self,
							       GAsyncResult         *result,
							       GError              **error);

#endif /* FACEBOOK_SERVICE_H */
