/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2011 Free Software Foundation, Inc.
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

#ifndef GTH_IMAGE_ROTATOR_H
#define GTH_IMAGE_ROTATOR_H

#include <glib.h>
#include <glib-object.h>
#include <gthumb.h>

G_BEGIN_DECLS

typedef enum {
	GTH_TRANSFORM_RESIZE_CLIP,
	GTH_TRANSFORM_RESIZE_BOUNDING_BOX,
	GTH_TRANSFORM_RESIZE_CROP
} GthTransformResize;

#define GTH_TYPE_IMAGE_ROTATOR            (gth_image_rotator_get_type ())
#define GTH_IMAGE_ROTATOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_IMAGE_ROTATOR, GthImageRotator))
#define GTH_IMAGE_ROTATOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_IMAGE_ROTATOR, GthImageRotatorClass))
#define GTH_IS_IMAGE_ROTATOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_IMAGE_ROTATOR))
#define GTH_IS_IMAGE_ROTATOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_IMAGE_ROTATOR))
#define GTH_IMAGE_ROTATOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_IMAGE_ROTATOR, GthImageRotatorClass))

typedef struct _GthImageRotator         GthImageRotator;
typedef struct _GthImageRotatorClass    GthImageRotatorClass;
typedef struct _GthImageRotatorPrivate  GthImageRotatorPrivate;

struct _GthImageRotator
{
	GObject __parent;
	GthImageRotatorPrivate *priv;
};

struct _GthImageRotatorClass
{
	GObjectClass __parent_class;

	/*< signals >*/

	void (* changed)        (GthImageRotator *rotator);
	void (* center_changed) (GthImageRotator *rotator,
				 int              x,
				 int              y);
	void (* angle_changed)  (GthImageRotator *rotator,
				 double           angle);
};

GType                 gth_image_rotator_get_type        (void);
GthImageViewerTool *  gth_image_rotator_new             (void);
void                  gth_image_rotator_set_grid_type   (GthImageRotator       *self,
                                 	 	 	 GthGridType            grid_type);
GthGridType           gth_image_rotator_get_grid_type   (GthImageRotator       *self);
void                  gth_image_rotator_set_center      (GthImageRotator       *rotator,
						         int                    x,
						         int                    y);
void                  gth_image_rotator_get_center      (GthImageRotator       *rotator,
							 int                   *x,
							 int                   *y);
void                  gth_image_rotator_set_angle       (GthImageRotator       *rotator,
						         double                 angle);
double                gth_image_rotator_get_angle       (GthImageRotator       *rotator);
void                  gth_image_rotator_set_resize      (GthImageRotator       *self,
						         GthTransformResize     resize);
GthTransformResize    gth_image_rotator_get_resize      (GthImageRotator       *self);
void                  gth_image_rotator_set_crop_region (GthImageRotator       *self,
							 cairo_rectangle_int_t *region);
void                  gth_image_rotator_set_background  (GthImageRotator       *self,
							 GdkRGBA               *color);
void                  gth_image_rotator_get_background  (GthImageRotator       *self,
		 	 	 	 	 	 GdkRGBA               *color);
cairo_surface_t *     gth_image_rotator_get_result      (GthImageRotator       *self,
							 cairo_surface_t       *image,
							 GthAsyncTask          *task);

G_END_DECLS

#endif /* GTH_IMAGE_ROTATOR_H */
