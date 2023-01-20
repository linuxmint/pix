/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001-2009 The Free Software Foundation, Inc.
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

#ifndef GTH_IMAGE_PRELOADER_H
#define GTH_IMAGE_PRELOADER_H

#include "gth-image-loader.h"
#include "gth-file-data.h"

#define GTH_ORIGINAL_SIZE -1
#define GTH_NO_PRELOADERS 0
#define GTH_MODIFIED_IMAGE NULL

#define GTH_TYPE_IMAGE_PRELOADER            (gth_image_preloader_get_type ())
#define GTH_IMAGE_PRELOADER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_IMAGE_PRELOADER, GthImagePreloader))
#define GTH_IMAGE_PRELOADER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_IMAGE_PRELOADER, GthImagePreloaderClass))
#define GTH_IS_IMAGE_PRELOADER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_IMAGE_PRELOADER))
#define GTH_IS_IMAGE_PRELOADER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_IMAGE_PRELOADER))
#define GTH_IMAGE_PRELOADER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_IMAGE_PRELOADER, GthImagePreloaderClass))

typedef struct _GthImagePreloader        GthImagePreloader;
typedef struct _GthImagePreloaderClass   GthImagePreloaderClass;
typedef struct _GthImagePreloaderPrivate GthImagePreloaderPrivate;

struct _GthImagePreloader {
	GObject __parent;
	GthImagePreloaderPrivate *priv;
};

struct _GthImagePreloaderClass {
	GObjectClass __parent_class;
};

GType               gth_image_preloader_get_type		 (void) G_GNUC_CONST;
GthImagePreloader * gth_image_preloader_new			 (void);
void                gth_image_preloader_set_out_profile		 (GthImagePreloader		 *loader,
								  GthICCProfile			 *profile);
void                gth_image_preloader_load			 (GthImagePreloader		 *self,
								  GthFileData			 *requested,
								  int				  requested_size,
								  GCancellable			 *cancellable,
								  GAsyncReadyCallback		  callback,
								  gpointer			  user_data,
								  int				  n_files,
								  ...);
gboolean            gth_image_preloader_load_finish    		 (GthImagePreloader		 *self,
							  	  GAsyncResult			 *result,
							  	  GthFileData			**requested,
							  	  GthImage			**image,
							  	  int				 *requested_size,
							  	  int				 *original_width,
							  	  int				 *original_height,
							  	  GError			**error);
void                gth_image_preloader_set_modified_image	 (GthImagePreloader		 *self,
								  GthImage		 	 *image);
cairo_surface_t *   gth_image_preloader_get_modified_image	 (GthImagePreloader		 *self);
void		    gth_image_preloader_clear_cache		 (GthImagePreloader		 *self);

#endif /* GTH_IMAGE_PRELOADER_H */
