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

#include <config.h>
#include <stdlib.h>
#include <math.h>
#include <gthumb.h>
#include "cairo-rotate.h"
#include "gth-image-rotator.h"


#define MIN4(a,b,c,d) MIN(MIN((a),(b)),MIN((c),(d)))
#define MAX4(a,b,c,d) MAX(MAX((a),(b)),MAX((c),(d)))
#define G_2_PI (G_PI * 2)
#define RAD_TO_DEG(x) ((x) * 180 / G_PI)
#define DEG_TO_RAD(x) ((x) * G_PI / 180)


enum {
	CHANGED,
	CENTER_CHANGED,
	ANGLE_CHANGED,
	LAST_SIGNAL
};


static guint signals[LAST_SIGNAL] = { 0 };


static void gth_image_rotator_gth_image_tool_interface_init (GthImageViewerToolInterface *iface);


struct _GthImageRotatorPrivate {
	GthImageViewer        *viewer;

	/* options */

	GdkPoint               center;
	double                 angle;
	GdkRGBA                background_color;
	gboolean               enable_crop;
	cairo_rectangle_int_t  crop_region;
	GthGridType            grid_type;
	GthTransformResize     resize;

	/* utility variables */

	int                    original_width;
	int                    original_height;
	double                 preview_zoom;
	cairo_surface_t       *preview_image;
	cairo_rectangle_int_t  preview_image_area;
	GdkPoint               preview_center;
	cairo_rectangle_int_t  clip_area;
	cairo_matrix_t         matrix;
	gboolean               dragging;
	double                 angle_before_dragging;
	GdkPoint               drag_p1;
	GdkPoint               drag_p2;
	GthFit                 original_fit_mode;
	gboolean               original_zoom_enabled;
};


G_DEFINE_TYPE_WITH_CODE (GthImageRotator,
			 gth_image_rotator,
			 G_TYPE_OBJECT,
			 G_ADD_PRIVATE (GthImageRotator)
			 G_IMPLEMENT_INTERFACE (GTH_TYPE_IMAGE_VIEWER_TOOL,
						gth_image_rotator_gth_image_tool_interface_init))


static void
gth_image_rotator_set_viewer (GthImageViewerTool *base,
			      GthImageViewer     *viewer)
{
	GthImageRotator *self = GTH_IMAGE_ROTATOR (base);
	GdkCursor       *cursor;

	self->priv->viewer = viewer;
	self->priv->original_fit_mode = gth_image_viewer_get_fit_mode (GTH_IMAGE_VIEWER (viewer));
	self->priv->original_zoom_enabled = gth_image_viewer_get_zoom_enabled (GTH_IMAGE_VIEWER (viewer));
	gth_image_viewer_set_fit_mode (GTH_IMAGE_VIEWER (viewer), GTH_FIT_SIZE_IF_LARGER);
	gth_image_viewer_set_zoom_enabled (GTH_IMAGE_VIEWER (viewer), FALSE);

	cursor = _gdk_cursor_new_for_widget (GTK_WIDGET (self->priv->viewer), GDK_LEFT_PTR);
	gth_image_viewer_set_cursor (self->priv->viewer, cursor);
	g_object_unref (cursor);
}


static void
gth_image_rotator_unset_viewer (GthImageViewerTool *base,
			        GthImageViewer     *viewer)
{
	GthImageRotator *self = GTH_IMAGE_ROTATOR (base);

	gth_image_viewer_set_fit_mode (GTH_IMAGE_VIEWER (viewer), self->priv->original_fit_mode);
	gth_image_viewer_set_zoom_enabled (GTH_IMAGE_VIEWER (viewer), self->priv->original_zoom_enabled);
	self->priv->viewer = NULL;
}


static void
gth_image_rotator_realize (GthImageViewerTool *base)
{
	/* void */
}


static void
gth_image_rotator_unrealize (GthImageViewerTool *base)
{
	/* void */
}


static void
_cairo_matrix_transform_point (cairo_matrix_t *matrix,
			       double          x,
			       double          y,
			       double         *tx,
			       double         *ty)
{
	*tx = x;
	*ty = y;
	cairo_matrix_transform_point (matrix, tx, ty);
}


static void
gth_transform_resize (cairo_matrix_t        *matrix,
		      GthTransformResize     resize,
		      cairo_rectangle_int_t *original,
		      cairo_rectangle_int_t *boundary)
{
	int x1, y1, x2, y2;

	x1 = original->x;
	y1 = original->y;
	x2 = original->x + original->width;
	y2 = original->y + original->height;

	switch (resize) {
	case GTH_TRANSFORM_RESIZE_CLIP:
		/* keep the original size */
		break;

	case GTH_TRANSFORM_RESIZE_BOUNDING_BOX:
	case GTH_TRANSFORM_RESIZE_CROP:
		{
			double dx1, dx2, dx3, dx4;
			double dy1, dy2, dy3, dy4;

			_cairo_matrix_transform_point (matrix, x1, y1, &dx1, &dy1);
			_cairo_matrix_transform_point (matrix, x2, y1, &dx2, &dy2);
			_cairo_matrix_transform_point (matrix, x1, y2, &dx3, &dy3);
			_cairo_matrix_transform_point (matrix, x2, y2, &dx4, &dy4);

			x1 = (int) floor (MIN4 (dx1, dx2, dx3, dx4));
			y1 = (int) floor (MIN4 (dy1, dy2, dy3, dy4));
			x2 = (int) ceil  (MAX4 (dx1, dx2, dx3, dx4));
			y2 = (int) ceil  (MAX4 (dy1, dy2, dy3, dy4));
			break;
		}
	}

	boundary->x = x1;
	boundary->y = y1;
	boundary->width = x2 - x1;
	boundary->height = y2 - y1;
}


static void
_gth_image_rotator_update_tranformation_matrix (GthImageRotator *self)
{
	int tx, ty;

	self->priv->preview_center.x = self->priv->center.x * self->priv->preview_zoom;
	self->priv->preview_center.y = self->priv->center.y * self->priv->preview_zoom;

	tx = self->priv->preview_image_area.x + self->priv->preview_center.x;
	ty = self->priv->preview_image_area.y + self->priv->preview_center.y;

	cairo_matrix_init_identity (&self->priv->matrix);
	cairo_matrix_translate (&self->priv->matrix, tx, ty);
	cairo_matrix_rotate (&self->priv->matrix, self->priv->angle);
	cairo_matrix_translate (&self->priv->matrix, -tx, -ty);

	gth_transform_resize (&self->priv->matrix,
			      self->priv->resize,
			      &self->priv->preview_image_area,
			      &self->priv->clip_area);
}


static void
update_image_surface (GthImageRotator *self)
{
	GtkAllocation    allocation;
	cairo_surface_t *image;
	int              max_size;
	int              width;
	int              height;
	cairo_surface_t *preview_image;

	if (self->priv->preview_image != NULL) {
		cairo_surface_destroy (self->priv->preview_image);
		self->priv->preview_image = NULL;
	}

	image = gth_image_viewer_get_current_image (GTH_IMAGE_VIEWER (self->priv->viewer));
	if (image == NULL)
		return;

	if (! _cairo_image_surface_get_original_size (image, &self->priv->original_width, &self->priv->original_height)) {
		self->priv->original_width = cairo_image_surface_get_width (image);
		self->priv->original_height = cairo_image_surface_get_height (image);
	}
	width = cairo_image_surface_get_width (image);
	height = cairo_image_surface_get_height (image);
	gtk_widget_get_allocation (GTK_WIDGET (self->priv->viewer), &allocation);
	max_size = MAX (allocation.width, allocation.height) / G_SQRT2 + 2;
	if (scale_keeping_ratio (&width, &height, max_size, max_size, FALSE))
		preview_image = _cairo_image_surface_scale_fast (image, width, height);
	else
		preview_image = cairo_surface_reference (image);

	self->priv->preview_zoom = (double) width / self->priv->original_width;
	self->priv->preview_image = preview_image;
	self->priv->preview_image_area.width = width;
	self->priv->preview_image_area.height = height;
	self->priv->preview_image_area.x = MAX ((allocation.width - self->priv->preview_image_area.width) / 2 - 0.5, 0);
	self->priv->preview_image_area.y = MAX ((allocation.height - self->priv->preview_image_area.height) / 2 - 0.5, 0);

	_gth_image_rotator_update_tranformation_matrix (self);
}


static void
gth_image_rotator_size_allocate (GthImageViewerTool *base,
				 GtkAllocation      *allocation)
{
	update_image_surface (GTH_IMAGE_ROTATOR (base));
}


static void
gth_image_rotator_map (GthImageViewerTool *base)
{
	/* void */
}


static void
gth_image_rotator_unmap (GthImageViewerTool *base)
{
	/* void */
}


static void
paint_image (GthImageRotator *self,
	     cairo_t         *cr)
{
	cairo_matrix_t matrix;

	cairo_save (cr);
	cairo_get_matrix (cr, &matrix);
	cairo_matrix_multiply (&matrix, &self->priv->matrix, &matrix);
	cairo_set_matrix (cr, &matrix);
	cairo_set_source_surface (cr, self->priv->preview_image,
				  self->priv->preview_image_area.x,
				  self->priv->preview_image_area.y);
  	cairo_rectangle (cr,
  			 self->priv->preview_image_area.x,
  			 self->priv->preview_image_area.y,
  			 self->priv->preview_image_area.width,
  			 self->priv->preview_image_area.height);
  	cairo_fill (cr);
  	cairo_restore (cr);
}


static void
paint_darker_background (GthImageRotator *self,
			 cairo_t         *cr)
{
	cairo_rectangle_int_t crop_region;
	GtkAllocation         allocation;

	cairo_save (cr);
	cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 0.5);

	gtk_widget_get_allocation (GTK_WIDGET (self->priv->viewer), &allocation);

	switch (self->priv->resize) {
	case GTH_TRANSFORM_RESIZE_BOUNDING_BOX:
	case GTH_TRANSFORM_RESIZE_CLIP:
		crop_region = self->priv->clip_area;
		break;

	case GTH_TRANSFORM_RESIZE_CROP:
		/* the crop_region is not zoomed the clip_area is already zoomed */
		cairo_scale (cr, self->priv->preview_zoom, self->priv->preview_zoom);
		crop_region = self->priv->crop_region;
		crop_region.x += self->priv->clip_area.x / self->priv->preview_zoom;
		crop_region.y += self->priv->clip_area.y / self->priv->preview_zoom;
		allocation.width /= self->priv->preview_zoom;
		allocation.height /= self->priv->preview_zoom;
		break;
	default:
		g_assert_not_reached ();
	}

	/* left side */

	cairo_rectangle (cr,
			 0,
			 0,
			 crop_region.x,
			 allocation.height);

	/* right side */

	cairo_rectangle (cr,
			 crop_region.x + crop_region.width,
			 0,
			 allocation.width - crop_region.x - crop_region.width,
			 allocation.height);

	/* top */

	cairo_rectangle (cr,
			 crop_region.x,
			 0,
			 crop_region.width,
			 crop_region.y);

	/* bottom */

	cairo_rectangle (cr,
			 crop_region.x,
			 crop_region.y + crop_region.height,
			 crop_region.width,
			 allocation.height - crop_region.y - crop_region.height);

	cairo_fill (cr);
	cairo_restore (cr);
}


static void
paint_grid (GthImageRotator *self,
	    cairo_t         *cr)
{
	cairo_rectangle_int_t grid;

	cairo_save (cr);

	switch (self->priv->resize) {
	case GTH_TRANSFORM_RESIZE_BOUNDING_BOX:
	case GTH_TRANSFORM_RESIZE_CLIP:
		grid = self->priv->clip_area;
		break;

	case GTH_TRANSFORM_RESIZE_CROP:
		cairo_scale (cr, self->priv->preview_zoom, self->priv->preview_zoom);
		grid = self->priv->crop_region;
		grid.x += self->priv->clip_area.x / self->priv->preview_zoom;
		grid.y += self->priv->clip_area.y / self->priv->preview_zoom;
		break;
	}

	_cairo_paint_grid (cr, &grid, self->priv->grid_type);

	cairo_restore (cr);
}


static void
paint_point (GthImageRotator *self,
	     cairo_t         *cr,
	     GdkPoint        *p)
{
	double radius = 10.0;

	cairo_save (cr);

#if CAIRO_VERSION >= CAIRO_VERSION_ENCODE(1, 9, 2)
	cairo_set_operator (cr, CAIRO_OPERATOR_DIFFERENCE);
#endif

	cairo_move_to (cr, p->x - radius, p->y - radius);
	cairo_line_to (cr, p->x + radius, p->y + radius);
	cairo_move_to (cr, p->x - radius, p->y + radius);
	cairo_line_to (cr, p->x + radius, p->y - radius);
	cairo_stroke (cr);

	cairo_restore (cr);
}


static void
gth_image_rotator_draw (GthImageViewerTool *base,
			cairo_t            *cr)
{
	GthImageRotator *self = GTH_IMAGE_ROTATOR (base);
	GtkAllocation    allocation;

	cairo_save (cr);

  	/* background */

	/*
	GtkStyleContext *style_context;
	GdkRGBA          color;
	style_context = gtk_widget_get_style_context (GTK_WIDGET (self->priv->viewer));
	gtk_style_context_get_background_color (style_context,
						gtk_widget_get_state (GTK_WIDGET (self->priv->viewer)),
						&color);
	gdk_cairo_set_source_rgba (cr, &color);
	*/

	gtk_widget_get_allocation (GTK_WIDGET (self->priv->viewer), &allocation);
	cairo_rectangle (cr, 0, 0, allocation.width, allocation.height);

  	cairo_set_source_rgba (cr,
  			       self->priv->background_color.red,
  			       self->priv->background_color.green,
  			       self->priv->background_color.blue,
  			       self->priv->background_color.alpha);
	cairo_fill (cr);
	cairo_restore (cr);

	if (self->priv->preview_image == NULL)
		return;

	paint_image (self, cr);
	paint_darker_background (self, cr);
	paint_grid (self, cr);

	if (self->priv->dragging) {
		GdkPoint center;

		cairo_save (cr);
		cairo_set_antialias (cr, CAIRO_ANTIALIAS_DEFAULT);
		cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
		cairo_restore (cr);

		center.x = self->priv->center.x * self->priv->preview_zoom + self->priv->preview_image_area.x;
		center.y = self->priv->center.y * self->priv->preview_zoom + self->priv->preview_image_area.y;
		paint_point (self, cr, &center);

		/* used for debugging purposes
		paint_point (self, cr, &self->priv->drag_p1);
		paint_point (self, cr, &self->priv->drag_p2);
		*/
	}
}


static gboolean
gth_image_rotator_button_release (GthImageViewerTool *base,
				  GdkEventButton     *event)
{
	GthImageRotator *self = GTH_IMAGE_ROTATOR (base);
	GdkCursor       *cursor;

	self->priv->dragging = FALSE;
	self->priv->drag_p1.x = 0;
	self->priv->drag_p1.y = 0;
	self->priv->drag_p2.x = 0;
	self->priv->drag_p2.y = 0;

	cursor = _gdk_cursor_new_for_widget (GTK_WIDGET (self->priv->viewer), GDK_LEFT_PTR);
	gth_image_viewer_set_cursor (self->priv->viewer, cursor);
	g_object_unref (cursor);

	gtk_widget_queue_draw (GTK_WIDGET (self->priv->viewer));

	return FALSE;
}


static gboolean
gth_image_rotator_button_press (GthImageViewerTool *base,
				GdkEventButton     *event)
{
	GthImageRotator *self = GTH_IMAGE_ROTATOR (base);

	if (event->type == GDK_2BUTTON_PRESS) {
		double x, y;

		x = (event->x - self->priv->preview_image_area.x) / self->priv->preview_zoom;
		y = (event->y - self->priv->preview_image_area.y) / self->priv->preview_zoom;
		g_signal_emit (self, signals[CENTER_CHANGED], 0, (int) x, (int) y);
	}

	if (event->type == GDK_BUTTON_PRESS) {
		self->priv->dragging = FALSE;
		self->priv->drag_p1.x = event->x;
		self->priv->drag_p1.y = event->y;
	}

	return FALSE;
}


static double
get_angle (GdkPoint *p1,
	   GdkPoint *p2)
{
	double a = 0.0;
	int    x, y;

	x = p2->x - p1->x;
	y = p2->y - p1->y;
	if (x >= 0) {
		if  (y >= 0)
			a = atan2 (y, x);
		else
			a = G_2_PI - atan2 (- y, x);
	}
	else {
		if (y >= 0)
			a = G_PI - atan2 (y, - x);
		else
			a = G_PI + atan2 (- y, - x);
	}

	return a;
}


static gboolean
gth_image_rotator_motion_notify (GthImageViewerTool *base,
				 GdkEventMotion     *event)
{
	GthImageRotator *self = GTH_IMAGE_ROTATOR (base);

	if (! self->priv->dragging
	    && gtk_drag_check_threshold (GTK_WIDGET (self->priv->viewer),
			    	    	 self->priv->drag_p1.x,
			    	    	 self->priv->drag_p1.y,
			    	    	 self->priv->drag_p2.x,
			    	    	 self->priv->drag_p2.y))
	{
		GdkCursor *cursor;

		self->priv->angle_before_dragging = self->priv->angle;
		self->priv->dragging = TRUE;

		cursor = gdk_cursor_new_from_name (gtk_widget_get_display (GTK_WIDGET (self->priv->viewer)), "grabbing");
		gth_image_viewer_set_cursor (self->priv->viewer, cursor);
		if (cursor != NULL)
			g_object_unref (cursor);
	}

	if (self->priv->dragging) {
		GdkPoint center;
		double   angle1;
		double   angle2;
		double   angle;

		self->priv->drag_p2.x = event->x;
		self->priv->drag_p2.y = event->y;

		center.x = self->priv->center.x * self->priv->preview_zoom + self->priv->preview_image_area.x;
		center.y = self->priv->center.y * self->priv->preview_zoom + self->priv->preview_image_area.y;

		angle1 = get_angle (&center, &self->priv->drag_p1);
		angle2 = get_angle (&center, &self->priv->drag_p2);
		angle = self->priv->angle_before_dragging + (angle2 - angle1);
		if (angle <  - G_PI)
			angle = G_2_PI + angle;
		if (angle >  + G_PI)
			angle = angle - G_2_PI;

		g_signal_emit (self, signals[ANGLE_CHANGED], 0, CLAMP (RAD_TO_DEG (angle), -180.0, 180));
	}

	return FALSE;
}


static void
gth_image_rotator_image_changed (GthImageViewerTool *base)
{
	update_image_surface (GTH_IMAGE_ROTATOR (base));
}


static void
gth_image_rotator_zoom_changed (GthImageViewerTool *base)
{
	update_image_surface (GTH_IMAGE_ROTATOR (base));
}


static void
gth_image_rotator_finalize (GObject *object)
{
	GthImageRotator *self;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GTH_IS_IMAGE_ROTATOR (object));

	self = (GthImageRotator *) object;
	if (self->priv->preview_image != NULL)
		cairo_surface_destroy (self->priv->preview_image);

	/* Chain up */
	G_OBJECT_CLASS (gth_image_rotator_parent_class)->finalize (object);
}


static void
gth_image_rotator_class_init (GthImageRotatorClass *class)
{
	GObjectClass *gobject_class;

	gobject_class = (GObjectClass*) class;
	gobject_class->finalize = gth_image_rotator_finalize;

	signals[CHANGED] = g_signal_new ("changed",
					 G_TYPE_FROM_CLASS (class),
					 G_SIGNAL_RUN_LAST,
					 G_STRUCT_OFFSET (GthImageRotatorClass, changed),
					 NULL, NULL,
					 g_cclosure_marshal_VOID__VOID,
					 G_TYPE_NONE,
					 0);
	signals[CENTER_CHANGED] = g_signal_new ("center-changed",
					 	G_TYPE_FROM_CLASS (class),
					 	G_SIGNAL_RUN_LAST,
					 	G_STRUCT_OFFSET (GthImageRotatorClass, center_changed),
					 	NULL, NULL,
					 	gth_marshal_VOID__INT_INT,
					 	G_TYPE_NONE,
					 	2,
					 	G_TYPE_INT,
					 	G_TYPE_INT);
	signals[ANGLE_CHANGED] = g_signal_new ("angle-changed",
					 	G_TYPE_FROM_CLASS (class),
					 	G_SIGNAL_RUN_LAST,
					 	G_STRUCT_OFFSET (GthImageRotatorClass, angle_changed),
					 	NULL, NULL,
					 	g_cclosure_marshal_VOID__DOUBLE,
					 	G_TYPE_NONE,
					 	1,
					 	G_TYPE_DOUBLE);
}


static void
gth_image_rotator_gth_image_tool_interface_init (GthImageViewerToolInterface *iface)
{
	iface->set_viewer = gth_image_rotator_set_viewer;
	iface->unset_viewer = gth_image_rotator_unset_viewer;
	iface->realize = gth_image_rotator_realize;
	iface->unrealize = gth_image_rotator_unrealize;
	iface->size_allocate = gth_image_rotator_size_allocate;
	iface->map = gth_image_rotator_map;
	iface->unmap = gth_image_rotator_unmap;
	iface->draw = gth_image_rotator_draw;
	iface->button_press = gth_image_rotator_button_press;
	iface->button_release = gth_image_rotator_button_release;
	iface->motion_notify = gth_image_rotator_motion_notify;
	iface->image_changed = gth_image_rotator_image_changed;
	iface->zoom_changed = gth_image_rotator_zoom_changed;
}


static void
gth_image_rotator_init (GthImageRotator *self)
{
	self->priv = gth_image_rotator_get_instance_private (self);
	self->priv->preview_image = NULL;
	self->priv->grid_type = GTH_GRID_NONE;
	self->priv->resize = GTH_TRANSFORM_RESIZE_BOUNDING_BOX;
	self->priv->background_color.red = 0.0;
	self->priv->background_color.green = 0.0;
	self->priv->background_color.blue = 0.0;
	self->priv->background_color.alpha = 1.0;
	self->priv->enable_crop = FALSE;
	self->priv->crop_region.x = 0;
	self->priv->crop_region.y = 0;
	self->priv->crop_region.width = 0;
	self->priv->crop_region.height = 0;
	self->priv->dragging = FALSE;
}


GthImageViewerTool *
gth_image_rotator_new (void)
{
	GthImageRotator *rotator;

	rotator = g_object_new (GTH_TYPE_IMAGE_ROTATOR, NULL);
	rotator->priv->angle = 0.0;

	return GTH_IMAGE_VIEWER_TOOL (rotator);
}


void
gth_image_rotator_set_grid_type (GthImageRotator *self,
                                 GthGridType      grid_type)
{
        if (grid_type == self->priv->grid_type)
                return;

        self->priv->grid_type = grid_type;
        if (self->priv->viewer != NULL)
        	gtk_widget_queue_draw (GTK_WIDGET (self->priv->viewer));
}


GthGridType
gth_image_rotator_get_grid_type (GthImageRotator *self)
{
        return self->priv->grid_type;
}


void
gth_image_rotator_set_center (GthImageRotator *self,
			      int              x,
			      int              y)
{
	self->priv->center.x = x;
	self->priv->center.y = y;
	_gth_image_rotator_update_tranformation_matrix (self);

	if (self->priv->viewer != NULL)
		gtk_widget_queue_draw (GTK_WIDGET (self->priv->viewer));

	g_signal_emit (self, signals[CHANGED], 0);
}


void
gth_image_rotator_get_center (GthImageRotator *self,
			      int             *x,
			      int             *y)
{
	*x = self->priv->center.x;
	*y = self->priv->center.y;
}


void
gth_image_rotator_set_angle (GthImageRotator *self,
			     double           angle)
{
	double radiants;

	radiants = DEG_TO_RAD (angle);
	if (radiants == self->priv->angle)
		return;
	self->priv->angle = radiants;

	_gth_image_rotator_update_tranformation_matrix (self);

	if (self->priv->viewer != NULL)
		gtk_widget_queue_draw (GTK_WIDGET (self->priv->viewer));

	g_signal_emit (self, signals[CHANGED], 0);
}


double
gth_image_rotator_get_angle (GthImageRotator *self)
{
	return self->priv->angle;
}


void
gth_image_rotator_set_resize (GthImageRotator    *self,
			      GthTransformResize  resize)
{
	self->priv->resize = resize;
	_gth_image_rotator_update_tranformation_matrix (self);

	if (self->priv->viewer != NULL)
		gtk_widget_queue_draw (GTK_WIDGET (self->priv->viewer));

	g_signal_emit (self, signals[CHANGED], 0);
}


GthTransformResize
gth_image_rotator_get_resize (GthImageRotator *self)
{
	return self->priv->resize;
}


void
gth_image_rotator_set_crop_region (GthImageRotator       *self,
				   cairo_rectangle_int_t *region)
{
	self->priv->enable_crop = (region != NULL);
	if (region != NULL)
		self->priv->crop_region = *region;

	if (self->priv->viewer != NULL)
		gtk_widget_queue_draw (GTK_WIDGET (self->priv->viewer));

	g_signal_emit (self, signals[CHANGED], 0);
}


void
gth_image_rotator_set_background (GthImageRotator *self,
			          GdkRGBA         *color)
{
	self->priv->background_color = *color;

	if (self->priv->viewer != NULL)
		gtk_widget_queue_draw (GTK_WIDGET (self->priv->viewer));

	g_signal_emit (self, signals[CHANGED], 0);
}


void
gth_image_rotator_get_background (GthImageRotator *self,
			          GdkRGBA   *color)
{
	*color = self->priv->background_color;
}


G_GNUC_UNUSED static cairo_surface_t *
gth_image_rotator_get_result_fast (GthImageRotator *self)
{
	double                 tx, ty;
	cairo_matrix_t         matrix;
	cairo_rectangle_int_t  image_area;
	cairo_rectangle_int_t  clip_area;
	cairo_surface_t       *output;
	cairo_t               *cr;

	/* compute the transformation matrix and the clip area */

	tx = self->priv->center.x;
	ty = self->priv->center.y;
	cairo_matrix_init_identity (&matrix);
	cairo_matrix_translate (&matrix, tx, ty);
	cairo_matrix_rotate (&matrix, self->priv->angle);
	cairo_matrix_translate (&matrix, -tx, -ty);

	image_area.x = 0.0;
	image_area.y = 0.0;
	image_area.width = self->priv->original_width;
	image_area.height = self->priv->original_height;

	gth_transform_resize (&matrix,
			      self->priv->resize,
			      &image_area,
			      &clip_area);

	if (! self->priv->enable_crop) {
		self->priv->crop_region.x = 0;
		self->priv->crop_region.y = 0;
		self->priv->crop_region.width = clip_area.width;
		self->priv->crop_region.height = clip_area.height;
	}

	output = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
					     self->priv->crop_region.width,
					     self->priv->crop_region.height);

	/* set the device offset to make the clip area start from the top left
	 * corner of the output image */

	cairo_surface_set_device_offset (output,
					 - clip_area.x - self->priv->crop_region.x,
					 - clip_area.y - self->priv->crop_region.y);
	cr = cairo_create (output);

	/* paint the background */

  	cairo_rectangle (cr, clip_area.x, clip_area.y, clip_area.width, clip_area.height);
  	cairo_clip_preserve (cr);
  	cairo_set_source_rgba (cr,
  			       self->priv->background_color.red,
  			       self->priv->background_color.green,
  			       self->priv->background_color.blue,
  			       self->priv->background_color.alpha);
  	cairo_fill (cr);

  	/* paint the rotated image */

	cairo_set_matrix (cr, &matrix);
	cairo_set_source_surface (cr, gth_image_viewer_get_current_image (GTH_IMAGE_VIEWER (self->priv->viewer)), image_area.x, image_area.y);
  	cairo_rectangle (cr, image_area.x, image_area.y, image_area.width, image_area.height);
  	cairo_fill (cr);
  	cairo_surface_flush (output);
  	cairo_surface_set_device_offset (output, 0.0, 0.0);

	cairo_destroy (cr);

	return output;
}


static cairo_surface_t *
gth_image_rotator_get_result_high_quality (GthImageRotator *self,
					   cairo_surface_t *image,
					   GthAsyncTask    *task)
{
	cairo_surface_t *rotated;
	cairo_surface_t *result;
	double           zoom;

	rotated = _cairo_image_surface_rotate (image,
					       self->priv->angle / G_PI * 180.0,
					       TRUE,
					       &self->priv->background_color,
					       task);

	switch (self->priv->resize) {
	case GTH_TRANSFORM_RESIZE_BOUNDING_BOX:
		self->priv->crop_region.x = 0;
		self->priv->crop_region.y = 0;
		self->priv->crop_region.width = cairo_image_surface_get_width (rotated);
		self->priv->crop_region.height = cairo_image_surface_get_height (rotated);
		break;

	case GTH_TRANSFORM_RESIZE_CLIP:
		self->priv->crop_region.x = MAX (((double) cairo_image_surface_get_width (rotated) - cairo_image_surface_get_width (image)) / 2.0, 0);
		self->priv->crop_region.y = MAX (((double) cairo_image_surface_get_height (rotated) - cairo_image_surface_get_height (image)) / 2.0, 0);
		self->priv->crop_region.width = cairo_image_surface_get_width (image);
		self->priv->crop_region.height = cairo_image_surface_get_height (image);
		break;

	case GTH_TRANSFORM_RESIZE_CROP:
		/* set by the user */

		zoom = (double) cairo_image_surface_get_width (image) / self->priv->original_width;
		self->priv->crop_region.x *= zoom;
		self->priv->crop_region.width *= zoom;

		zoom = (double) cairo_image_surface_get_height (image) / self->priv->original_height;
		self->priv->crop_region.y *= zoom;
		self->priv->crop_region.height *= zoom;

		break;
	}

	result = _cairo_image_surface_copy_subsurface (rotated,
						       self->priv->crop_region.x,
						       self->priv->crop_region.y,
						       MIN (self->priv->crop_region.width, cairo_image_surface_get_width (rotated) - self->priv->crop_region.x),
						       MIN (self->priv->crop_region.height, cairo_image_surface_get_height (rotated) - self->priv->crop_region.y));

	cairo_surface_destroy (rotated);

	return result;
}


cairo_surface_t *
gth_image_rotator_get_result (GthImageRotator *self,
			      cairo_surface_t *image,
			      GthAsyncTask    *task)
{
	return gth_image_rotator_get_result_high_quality (self, image, task);
}
