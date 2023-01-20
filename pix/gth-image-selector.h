/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2009 Free Software Foundation, Inc.
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

#ifndef GTH_IMAGE_SELECTOR_H
#define GTH_IMAGE_SELECTOR_H

#include <glib.h>
#include <glib-object.h>
#include "gth-image-viewer.h"

G_BEGIN_DECLS

#define GTH_TYPE_IMAGE_SELECTOR            (gth_image_selector_get_type ())
#define GTH_IMAGE_SELECTOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_IMAGE_SELECTOR, GthImageSelector))
#define GTH_IMAGE_SELECTOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_IMAGE_SELECTOR, GthImageSelectorClass))
#define GTH_IS_IMAGE_SELECTOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_IMAGE_SELECTOR))
#define GTH_IS_IMAGE_SELECTOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_IMAGE_SELECTOR))
#define GTH_IMAGE_SELECTOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_IMAGE_SELECTOR, GthImageSelectorClass))

typedef struct _GthImageSelector         GthImageSelector;
typedef struct _GthImageSelectorClass    GthImageSelectorClass;
typedef struct _GthImageSelectorPrivate  GthImageSelectorPrivate;

typedef enum {
	GTH_SELECTOR_TYPE_REGION,
	GTH_SELECTOR_TYPE_POINT
} GthSelectorType;

struct _GthImageSelector
{
	GObject __parent;
	GthImageSelectorPrivate *priv;
};

struct _GthImageSelectorClass
{
	GObjectClass __parent_class;

	/*< signals >*/

	void (* selection_changed)       (GthImageSelector *selector);
	void (* selected)                (GthImageSelector *selector,
					  int               x,
					  int               y);
	void (* motion_notify)           (GthImageSelector *selector,
					  int               x,
					  int               y);
	void (* mask_visibility_changed) (GthImageSelector *selector);
        void (* grid_visibility_changed) (GthImageSelector *selector);
};

GType                 gth_image_selector_get_type             (void);
GthImageViewerTool *  gth_image_selector_new                  (GthSelectorType        type);
gboolean              gth_image_selector_set_selection_x      (GthImageSelector      *selector,
							       int                    x);
gboolean              gth_image_selector_set_selection_y      (GthImageSelector      *selector,
							       int                    y);
gboolean              gth_image_selector_set_selection_pos    (GthImageSelector      *selector,
							       int                    x,
							       int                    y);
gboolean              gth_image_selector_set_selection_width  (GthImageSelector      *selector,
							       int                    width);
gboolean              gth_image_selector_set_selection_height (GthImageSelector      *selector,
							       int                    height);
void                  gth_image_selector_set_selection        (GthImageSelector      *selector,
							       cairo_rectangle_int_t  selection);
void                  gth_image_selector_get_selection        (GthImageSelector      *selector,
							       cairo_rectangle_int_t *selection);
void                  gth_image_selector_set_ratio            (GthImageSelector      *selector,
							       gboolean               use_ratio,
							       double                 ratio,
							       gboolean               swap_x_and_y_to_start);
double                gth_image_selector_get_ratio            (GthImageSelector      *selector);
gboolean              gth_image_selector_get_use_ratio        (GthImageSelector      *selector);
void                  gth_image_selector_set_mask_visible     (GthImageSelector      *selector,
							       gboolean               visible);
gboolean              gth_image_selector_get_mask_visible     (GthImageSelector      *selector);
void                  gth_image_selector_set_grid_type        (GthImageSelector      *selector,
                                                               GthGridType            grid_type);
GthGridType           gth_image_selector_get_grid_type        (GthImageSelector      *selector);
void                  gth_image_selector_bind_dimensions      (GthImageSelector      *selector,
							       gboolean               bind,
							       int                    factor);
void                  gth_image_selector_center               (GthImageSelector      *selector);

G_END_DECLS

#endif /* GTH_IMAGE_SELECTOR_H */
