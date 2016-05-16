/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2010-2012 Free Software Foundation, Inc.
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

#ifndef PICASA_WEB_SERVICE_H
#define PICASA_WEB_SERVICE_H

#include <gtk/gtk.h>
#include <gthumb.h>
#include <extensions/oauth/oauth.h>
#include "picasa-web-album.h"
#include "picasa-web-types.h"

#define PICASA_TYPE_WEB_SERVICE         (picasa_web_service_get_type ())
#define PICASA_WEB_SERVICE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), PICASA_TYPE_WEB_SERVICE, PicasaWebService))
#define PICASA_WEB_SERVICE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), PICASA_TYPE_WEB_SERVICE, PicasaWebServiceClass))
#define PICASA_IS_WEB_SERVICE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), PICASA_TYPE_WEB_SERVICE))
#define PICASA_IS_WEB_SERVICE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), PICASA_TYPE_WEB_SERVICE))
#define PICASA_WEB_SERVICE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), PICASA_TYPE_WEB_SERVICE, PicasaWebServiceClass))

typedef struct _PicasaWebService         PicasaWebService;
typedef struct _PicasaWebServicePrivate  PicasaWebServicePrivate;
typedef struct _PicasaWebServiceClass    PicasaWebServiceClass;

struct _PicasaWebService
{
	WebService __parent;
	PicasaWebServicePrivate *priv;
};

struct _PicasaWebServiceClass
{
	WebServiceClass __parent_class;
};

GType                picasa_web_service_get_type            (void) G_GNUC_CONST;
PicasaWebService *   picasa_web_service_new                 (GCancellable         *cancellable,
							     GtkWidget            *browser,
							     GtkWidget            *dialog);
guint64              picasa_web_service_get_free_space      (PicasaWebService     *self);
void                 picasa_web_service_list_albums         (PicasaWebService     *self,
						             GCancellable         *cancellable,
						             GAsyncReadyCallback   callback,
						             gpointer              user_data);
GList *              picasa_web_service_list_albums_finish  (PicasaWebService     *self,
						             GAsyncResult         *result,
						             GError              **error);
void                 picasa_web_service_create_album        (PicasaWebService     *self,
							     PicasaWebAlbum       *album,
							     GCancellable         *cancellable,
							     GAsyncReadyCallback   callback,
							     gpointer              user_data);
PicasaWebAlbum *     picasa_web_service_create_album_finish (PicasaWebService     *self,
							     GAsyncResult         *result,
							     GError              **error);
void                 picasa_web_service_post_photos         (PicasaWebService     *self,
							     PicasaWebAlbum       *album,
							     GList                *file_list, /* GFile list */
							     int                   max_width,
							     int                   max_height,
							     GCancellable         *cancellable,
							     GAsyncReadyCallback   callback,
							     gpointer              user_data);
gboolean             picasa_web_service_post_photos_finish  (PicasaWebService     *self,
							     GAsyncResult         *result,
							     GError              **error);
void                 picasa_web_service_list_photos         (PicasaWebService     *self,
							     PicasaWebAlbum       *album,
						             GCancellable         *cancellable,
						             GAsyncReadyCallback   callback,
						             gpointer              user_data);
GList *              picasa_web_service_list_photos_finish  (PicasaWebService     *self,
							     GAsyncResult         *result,
							     GError              **error);

#endif /* PICASA_WEB_SERVICE_H */
