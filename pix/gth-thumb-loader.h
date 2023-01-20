/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001, 2003, 2007, 2008 The Free Software Foundation, Inc.
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

#ifndef GTH_THUMB_LOADER_H
#define GTH_THUMB_LOADER_H

#include <glib.h>
#include <gdk/gdk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "gth-image-loader.h"
#include "gth-file-data.h"

G_BEGIN_DECLS

#define GTH_TYPE_THUMB_LOADER            (gth_thumb_loader_get_type ())
#define GTH_THUMB_LOADER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_THUMB_LOADER, GthThumbLoader))
#define GTH_THUMB_LOADER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_THUMB_LOADER, GthThumbLoaderClass))
#define GTH_IS_THUMB_LOADER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_THUMB_LOADER))
#define GTH_IS_THUMB_LOADER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_THUMB_LOADER))
#define GTH_THUMB_LOADER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_THUMB_LOADER, GthThumbLoaderClass))

typedef struct _GthThumbLoader        GthThumbLoader;
typedef struct _GthThumbLoaderClass   GthThumbLoaderClass;
typedef struct _GthThumbLoaderPrivate GthThumbLoaderPrivate;

struct _GthThumbLoader
{
	GObject  __parent;
	GthThumbLoaderPrivate *priv;
};

struct _GthThumbLoaderClass
{
	GObjectClass __parent_class;
};

GType             gth_thumb_loader_get_type             (void);
GthThumbLoader *  gth_thumb_loader_new                  (int                   requested_size);
GthThumbLoader *  gth_thumb_loader_copy                 (GthThumbLoader       *self);
void              gth_thumb_loader_set_loader_func      (GthThumbLoader       *self,
							 GthImageLoaderFunc    loader_func);
void              gth_thumb_loader_set_requested_size   (GthThumbLoader       *self,
					                 int                   size);
int               gth_thumb_loader_get_requested_size   (GthThumbLoader       *self);
void              gth_thumb_loader_set_use_cache        (GthThumbLoader       *self,
					                 gboolean              use);
void              gth_thumb_loader_set_save_thumbnails  (GthThumbLoader       *self,
					                 gboolean              save);
void              gth_thumb_loader_set_max_file_size    (GthThumbLoader       *self,
					                 goffset               size);
void              gth_thumb_loader_load                 (GthThumbLoader       *self,
						         GthFileData          *file_data,
						         GCancellable         *cancellable,
						         GAsyncReadyCallback   callback,
						         gpointer              user_data);
gboolean          gth_thumb_loader_load_finish          (GthThumbLoader       *self,
						  	 GAsyncResult         *res,
						  	 cairo_surface_t     **image,
							 GError              **error);
gboolean          gth_thumb_loader_has_valid_thumbnail  (GthThumbLoader       *self,
				      	      	         GthFileData          *file_data);
gboolean          gth_thumb_loader_has_failed_thumbnail (GthThumbLoader       *self,
				      	      	         GthFileData          *file_data);

G_END_DECLS

#endif /* GTH_THUMB_LOADER_H */
