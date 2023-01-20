/* -*- Mode: C; tab-width: 8;	 indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2011 The Free Software Foundation, Inc.
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

#ifndef GTH_IMAGE_H
#define GTH_IMAGE_H

#include <glib.h>
#include <glib-object.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gio/gio.h>
#include <cairo.h>
#include "gth-file-data.h"
#include "gth-icc-profile.h"

G_BEGIN_DECLS

typedef enum {
	GTH_IMAGE_FORMAT_CAIRO_SURFACE,
	GTH_IMAGE_FORMAT_GDK_PIXBUF,
	GTH_IMAGE_FORMAT_GDK_PIXBUF_ANIMATION,
	GTH_IMAGE_N_FORMATS
} GthImageFormat;

#define GTH_TYPE_IMAGE            (gth_image_get_type ())
#define GTH_IMAGE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_IMAGE, GthImage))
#define GTH_IMAGE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_IMAGE, GthImageClass))
#define GTH_IS_IMAGE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_IMAGE))
#define GTH_IS_IMAGE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_IMAGE))
#define GTH_IMAGE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_IMAGE, GthImageClass))

typedef struct _GthImage         GthImage;
typedef struct _GthImageClass    GthImageClass;
typedef struct _GthImagePrivate  GthImagePrivate;

struct _GthImage
{
	GObject __parent;
	GthImagePrivate *priv;
};

struct _GthImageClass
{
	GObjectClass __parent_class;

	gboolean  (*get_is_zoomable)  (GthImage *image);
	gboolean  (*set_zoom)         (GthImage *image,
				       double    zoom,
				       int      *original_width,
				       int      *original_height);
};


typedef GthImage * (*GthImageLoaderFunc) (GInputStream  *istream,
					  GthFileData   *file_data,
					  int            requested_size,
					  int           *original_width,
					  int           *original_height,
					  gboolean      *loaded_original,
					  gpointer       user_data,
					  GCancellable  *cancellable,
					  GError       **error);


GType                 gth_image_get_type                    (void);
GthImage *            gth_image_new                         (void);
GthImage *            gth_image_new_for_surface             (cairo_surface_t    *surface);
GthImage *            gth_image_new_for_pixbuf              (GdkPixbuf          *value);
GthImage *            gth_image_copy                        (GthImage           *image);
void                  gth_image_set_cairo_surface           (GthImage           *image,
						             cairo_surface_t    *value);
cairo_surface_t *     gth_image_get_cairo_surface           (GthImage           *image);
gboolean              gth_image_get_original_size	    (GthImage           *image,
							     int                *width,
							     int                *height);
gboolean              gth_image_get_is_zoomable             (GthImage           *image);
gboolean              gth_image_get_is_null                 (GthImage           *image);
gboolean              gth_image_set_zoom                    (GthImage           *image,
							     double              zoom,
							     int                *original_width,
							     int                *original_height);
void                  gth_image_set_pixbuf                  (GthImage           *image,
						             GdkPixbuf          *value);
GdkPixbuf *           gth_image_get_pixbuf                  (GthImage           *image);
void                  gth_image_set_pixbuf_animation        (GthImage           *image,
						             GdkPixbufAnimation *value);
GdkPixbufAnimation *  gth_image_get_pixbuf_animation        (GthImage           *image);
gboolean              gth_image_get_is_animation            (GthImage           *image);
void		      gth_image_set_icc_profile		    (GthImage           *image,
							     GthICCProfile	*profile);
GthICCProfile *	      gth_image_get_icc_profile		    (GthImage           *image);
void		      gth_image_apply_icc_profile           (GthImage           *image,
							     GthICCProfile      *out_profile,
							     GCancellable       *cancellable);
void		      gth_image_apply_icc_profile_async     (GthImage           *image,
							     GthICCProfile	*out_profile,
							     GCancellable	*cancellable,
							     GAsyncReadyCallback callback,
							     gpointer            user_data);
gboolean	      gth_image_apply_icc_profile_finish    (GAsyncResult       *result,
							     GError            **error);

G_END_DECLS

#endif /* GTH_IMAGE_H */
