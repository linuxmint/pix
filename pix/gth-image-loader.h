/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001-2010 The Free Software Foundation, Inc.
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

#ifndef GTH_IMAGE_LOADER_H
#define GTH_IMAGE_LOADER_H

#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "gth-file-data.h"
#include "gth-image.h"

G_BEGIN_DECLS

#define GTH_TYPE_IMAGE_LOADER            (gth_image_loader_get_type ())
#define GTH_IMAGE_LOADER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_IMAGE_LOADER, GthImageLoader))
#define GTH_IMAGE_LOADER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_IMAGE_LOADER, GthImageLoaderClass))
#define GTH_IS_IMAGE_LOADER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_IMAGE_LOADER))
#define GTH_IS_IMAGE_LOADER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_IMAGE_LOADER))
#define GTH_IMAGE_LOADER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_IMAGE_LOADER, GthImageLoaderClass))

typedef struct _GthImageLoader        GthImageLoader;
typedef struct _GthImageLoaderClass   GthImageLoaderClass;
typedef struct _GthImageLoaderPrivate GthImageLoaderPrivate;

struct _GthImageLoader {
	GObject __parent;
	GthImageLoaderPrivate *priv;
};

struct _GthImageLoaderClass {
	GObjectClass __parent_class;
};

GType             gth_image_loader_get_type               (void);
GthImageLoader *  gth_image_loader_new                    (GthImageLoaderFunc    loader_func,
							   gpointer              loader_data);
void              gth_image_loader_set_loader_func        (GthImageLoader       *loader,
							   GthImageLoaderFunc    loader_func,
						           gpointer              loader_data);
GthImageLoaderFunc gth_image_loader_get_loader_func       (GthImageLoader       *loader);
void              gth_image_loader_set_preferred_format   (GthImageLoader       *loader,
							   GthImageFormat        preferred_format);
void              gth_image_loader_set_out_profile        (GthImageLoader       *loader,
							   GthICCProfile        *profile);
void              gth_image_loader_load                   (GthImageLoader       *loader,
							   GthFileData          *file_data,
							   int                   requested_size,
							   int                   io_priority,
							   GCancellable         *cancellable,
							   GAsyncReadyCallback   callback,
							   gpointer              user_data);
gboolean          gth_image_loader_load_finish            (GthImageLoader       *loader,
							   GAsyncResult         *res,
							   GthImage            **image,
							   int                  *original_width,
							   int                  *original_height,
							   gboolean             *loaded_original,
							   GError              **error);
GthImage *        gth_image_new_from_stream               (GInputStream         *istream,
							   int                   requested_size,
							   int                  *original_width,
							   int                  *original_height,
							   GCancellable         *cancellable,
							   GError              **error);

G_END_DECLS

#endif /* GTH_IMAGE_LOADER_H */
