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

#include <config.h>
#include <stdlib.h>
#include <math.h>
#include "cairo-utils.h"
#include "glib-utils.h"
#include "gth-image-selector.h"
#include "gth-marshal.h"
#include "gtk-utils.h"


#define DEFAULT_BORDER 40
#define MIN_BORDER 3
#define DRAG_THRESHOLD 1
#define STEP_INCREMENT 20.0  /* scroll increment. */
#define SCROLL_TIMEOUT 30    /* autoscroll timeout in milliseconds */


typedef struct {
	int                    ref_count;
	int                    id;
	cairo_rectangle_int_t  area;
	GdkCursor             *cursor;
} EventArea;


static EventArea *
event_area_new (GtkWidget        *widget,
		int               id,
		GdkCursorType     cursor_type)
{
	EventArea *event_area;

	event_area = g_new0 (EventArea, 1);

	event_area->ref_count = 1;
	event_area->id = id;
	event_area->area.x = 0;
	event_area->area.y = 0;
	event_area->area.width = 0;
	event_area->area.height = 0;
	event_area->cursor = _gdk_cursor_new_for_widget (widget, cursor_type);

	return event_area;
}


G_GNUC_UNUSED
static void
event_area_ref (EventArea *event_area)
{
	event_area->ref_count++;
}


static void
event_area_unref (EventArea *event_area)
{
	event_area->ref_count--;

	if (event_area->ref_count > 0)
		return;

	if (event_area->cursor != NULL)
		g_object_unref (event_area->cursor);
	g_free (event_area);
}


/**/


typedef enum {
	C_SELECTION_AREA,
	C_TOP_AREA,
	C_BOTTOM_AREA,
	C_LEFT_AREA,
	C_RIGHT_AREA,
	C_TOP_LEFT_AREA,
	C_TOP_RIGHT_AREA,
	C_BOTTOM_LEFT_AREA,
	C_BOTTOM_RIGHT_AREA
} GthEventAreaType;


static GthEventAreaType
get_opposite_event_area_on_x (GthEventAreaType type)
{
	GthEventAreaType opposite_type = C_SELECTION_AREA;
	switch (type) {
		case C_SELECTION_AREA:
			opposite_type = C_SELECTION_AREA;
			break;
		case C_TOP_AREA:
			opposite_type = C_TOP_AREA;
			break;
		case C_BOTTOM_AREA:
			opposite_type = C_BOTTOM_AREA;
			break;
		case C_LEFT_AREA:
			opposite_type = C_RIGHT_AREA;
			break;
		case C_RIGHT_AREA:
			opposite_type = C_LEFT_AREA;
			break;
		case C_TOP_LEFT_AREA:
			opposite_type = C_TOP_RIGHT_AREA;
			break;
		case C_TOP_RIGHT_AREA:
			opposite_type = C_TOP_LEFT_AREA;
			break;
		case C_BOTTOM_LEFT_AREA:
			opposite_type = C_BOTTOM_RIGHT_AREA;
			break;
		case C_BOTTOM_RIGHT_AREA:
			opposite_type = C_BOTTOM_LEFT_AREA;
			break;
	}
	return opposite_type;
}


static GthEventAreaType
get_opposite_event_area_on_y (GthEventAreaType type)
{
	GthEventAreaType opposite_type = C_SELECTION_AREA;
	switch (type) {
		case C_SELECTION_AREA:
			opposite_type = C_SELECTION_AREA;
			break;
		case C_TOP_AREA:
			opposite_type = C_BOTTOM_AREA;
			break;
		case C_BOTTOM_AREA:
			opposite_type = C_TOP_AREA;
			break;
		case C_LEFT_AREA:
			opposite_type = C_LEFT_AREA;
			break;
		case C_RIGHT_AREA:
			opposite_type = C_RIGHT_AREA;
			break;
		case C_TOP_LEFT_AREA:
			opposite_type = C_BOTTOM_LEFT_AREA;
			break;
		case C_TOP_RIGHT_AREA:
			opposite_type = C_BOTTOM_RIGHT_AREA;
			break;
		case C_BOTTOM_LEFT_AREA:
			opposite_type = C_TOP_LEFT_AREA;
			break;
		case C_BOTTOM_RIGHT_AREA:
			opposite_type = C_TOP_RIGHT_AREA;
			break;
	}
	return opposite_type;
}


enum {
	SELECTION_CHANGED,
	SELECTED,
	MOTION_NOTIFY,
	MASK_VISIBILITY_CHANGED,
	GRID_VISIBILITY_CHANGED,
	LAST_SIGNAL
};


static guint signals[LAST_SIGNAL] = { 0 };


struct _GthImageSelectorPrivate {
	GthImageViewer         *viewer;
	GthSelectorType         type;

	cairo_surface_t        *surface;
	cairo_rectangle_int_t   surface_area;

	gboolean                use_ratio;
	double                  ratio;
	gboolean                mask_visible;
	GthGridType             grid_type;
	gboolean                active;
	gboolean                bind_dimensions;
	int                     bind_factor;

	cairo_rectangle_int_t   drag_start_selection_area;
	cairo_rectangle_int_t   selection_area;
	cairo_rectangle_int_t   selection;

	GdkCursor              *default_cursor;
	GdkCursor              *current_cursor;
	GList                  *event_list;
	EventArea              *current_area;

	guint                   timer_id; 	    /* Timeout ID for
	 	 	 	 	 	     * autoscrolling */
	double                  x_value_diff;       /* Change the adjustment value
	 	 	 	 	 	     * by this
	 	 	 	 	 	     * amount when autoscrolling */
	double                  y_value_diff;
};


static void gth_image_selector_gth_image_tool_interface_init (GthImageViewerToolInterface *iface);


G_DEFINE_TYPE_WITH_CODE (GthImageSelector,
			 gth_image_selector,
			 G_TYPE_OBJECT,
			 G_ADD_PRIVATE (GthImageSelector)
			 G_IMPLEMENT_INTERFACE (GTH_TYPE_IMAGE_VIEWER_TOOL,
						gth_image_selector_gth_image_tool_interface_init))


static gboolean
point_in_rectangle (int                   x,
		    int                   y,
		    cairo_rectangle_int_t rect)
{
	return ((x >= rect.x)
		&& (x <= rect.x + rect.width)
		&& (y >= rect.y)
		&& (y <= rect.y + rect.height));
}


static gboolean
rectangle_in_rectangle (cairo_rectangle_int_t r1,
			cairo_rectangle_int_t r2)
{
	return (point_in_rectangle (r1.x, r1.y, r2)
		&& point_in_rectangle (r1.x + r1.width,
				       r1.y + r1.height,
				       r2));
}


static gboolean
rectangle_equal (cairo_rectangle_int_t r1,
		 cairo_rectangle_int_t r2)
{
	return ((r1.x == r2.x)
		&& (r1.y == r2.y)
		&& (r1.width == r2.width)
		&& (r1.height == r2.height));
}


static int
real_to_selector (GthImageSelector *self,
		  int               value)
{
	return IROUND (gth_image_viewer_get_zoom (self->priv->viewer) * value);
}


static void
convert_to_selection_area (GthImageSelector      *self,
			   cairo_rectangle_int_t  real_area,
			   cairo_rectangle_int_t *selection_area)
{
	selection_area->x = real_to_selector (self, real_area.x);
	selection_area->y = real_to_selector (self, real_area.y);
	selection_area->width = real_to_selector (self, real_area.width);
	selection_area->height = real_to_selector (self, real_area.height);
}


static void
add_event_area (GthImageSelector *self,
		int               area_id,
		GdkCursorType     cursor_type)
{
	EventArea *event_area;

	event_area = event_area_new (GTK_WIDGET (self->priv->viewer), area_id, cursor_type);
	self->priv->event_list = g_list_prepend (self->priv->event_list, event_area);
}


static void
free_event_area_list (GthImageSelector *self)
{
	if (self->priv->event_list != NULL) {
		g_list_foreach (self->priv->event_list, (GFunc) event_area_unref, NULL);
		g_list_free (self->priv->event_list);
		self->priv->event_list = NULL;
	}
}


static EventArea *
get_event_area_from_position (GthImageSelector *self,
			      int               x,
			      int               y)
{
	GList *scan;

	for (scan = self->priv->event_list; scan; scan = scan->next) {
		EventArea             *event_area = scan->data;
		cairo_rectangle_int_t  widget_area;

		widget_area = event_area->area;
		widget_area.x += MAX (self->priv->viewer->image_area.x, gth_image_viewer_get_frame_border (self->priv->viewer));
		widget_area.y += MAX (self->priv->viewer->image_area.y, gth_image_viewer_get_frame_border (self->priv->viewer));

		if (point_in_rectangle (x, y, widget_area))
			return event_area;
	}

	return NULL;
}


static EventArea *
get_event_area_from_id (GthImageSelector *self,
			int               event_id)
{
	GList *scan;

	for (scan = self->priv->event_list; scan; scan = scan->next) {
		EventArea *event_area = scan->data;
		if (event_area->id == event_id)
			return event_area;
	}

	return NULL;
}


typedef enum {
	SIDE_NONE = 0,
	SIDE_TOP = 1 << 0,
	SIDE_RIGHT = 1 << 1,
	SIDE_BOTTOM = 1 << 2,
	SIDE_LEFT = 1 << 3
} Sides;


static void
_cairo_rectangle_partial (cairo_t *cr,
			  double   x,
			  double   y,
			  double   w,
			  double   h,
			  Sides    sides)
{
	if (sides & SIDE_TOP) {
		cairo_move_to (cr, x, y);
		cairo_rel_line_to (cr, w, 0);
	}

	if (sides & SIDE_RIGHT) {
		cairo_move_to (cr, x + w, y);
		cairo_rel_line_to (cr, 0, h);
	}

	if (sides & SIDE_BOTTOM) {
		cairo_move_to (cr, x, y + h);
		cairo_rel_line_to (cr, w, 0);
	}

	if (sides & SIDE_LEFT) {
		cairo_move_to (cr, x, y);
		cairo_rel_line_to (cr, 0, h);
	}
}


static void
event_area_paint (GthImageSelector *self,
		  EventArea        *event_area,
		  cairo_t          *cr)
{
	Sides sides = SIDE_NONE;

	switch (event_area->id) {
	case C_SELECTION_AREA:
		break;
	case C_TOP_AREA:
		sides = SIDE_RIGHT | SIDE_BOTTOM | SIDE_LEFT;
		break;
	case C_BOTTOM_AREA:
		sides = SIDE_TOP | SIDE_RIGHT | SIDE_LEFT;
		break;
	case C_LEFT_AREA:
		sides = SIDE_TOP | SIDE_RIGHT | SIDE_BOTTOM;
		break;
	case C_RIGHT_AREA:
		sides = SIDE_TOP | SIDE_BOTTOM | SIDE_LEFT;
		break;
	case C_TOP_LEFT_AREA:
		sides = SIDE_RIGHT | SIDE_BOTTOM;
		break;
	case C_TOP_RIGHT_AREA:
		sides = SIDE_BOTTOM | SIDE_LEFT;
		break;
	case C_BOTTOM_LEFT_AREA:
		sides = SIDE_TOP | SIDE_RIGHT;
		break;
	case C_BOTTOM_RIGHT_AREA:
		sides = SIDE_TOP | SIDE_LEFT;
		break;
	}

	_cairo_rectangle_partial (cr,
				  self->priv->viewer->image_area.x + event_area->area.x - self->priv->viewer->visible_area.x + 0.5,
				  self->priv->viewer->image_area.y + event_area->area.y - self->priv->viewer->visible_area.y + 0.5,
				  event_area->area.width - 1.0,
				  event_area->area.height - 1.0,
				  sides);
}


/**/


static void
update_event_areas (GthImageSelector *self)
{
	EventArea *event_area;
	int        x, y, width, height;
	int        border;
	int        border2;

	if (! gtk_widget_get_realized (GTK_WIDGET (self->priv->viewer)))
		return;

	border = DEFAULT_BORDER;
	if (self->priv->selection_area.width < DEFAULT_BORDER * 3)
		border = self->priv->selection_area.width / 3;
	if (self->priv->selection_area.height < DEFAULT_BORDER * 3)
		border = self->priv->selection_area.height / 3;
	if (border < MIN_BORDER)
		border = MIN_BORDER;

	border2 = border * 2;

	x = self->priv->selection_area.x - 1;
	y = self->priv->selection_area.y - 1;
	width = self->priv->selection_area.width + 1;
	height = self->priv->selection_area.height + 1;

	event_area = get_event_area_from_id (self, C_SELECTION_AREA);
	event_area->area.x = x;
	event_area->area.y = y;
	event_area->area.width = width;
	event_area->area.height = height;

	event_area = get_event_area_from_id (self, C_TOP_AREA);
	event_area->area.x = x + border;
	event_area->area.y = y;
	event_area->area.width = width - border2;
	event_area->area.height = border;

	event_area = get_event_area_from_id (self, C_BOTTOM_AREA);
	event_area->area.x = x + border;
	event_area->area.y = y + height - border;
	event_area->area.width = width - border2;
	event_area->area.height = border;

	event_area = get_event_area_from_id (self, C_LEFT_AREA);
	event_area->area.x = x;
	event_area->area.y = y + border;
	event_area->area.width = border;
	event_area->area.height = height - border2;

	event_area = get_event_area_from_id (self, C_RIGHT_AREA);
	event_area->area.x = x + width - border;
	event_area->area.y = y + border;
	event_area->area.width = border;
	event_area->area.height = height - border2;

	event_area = get_event_area_from_id (self, C_TOP_LEFT_AREA);
	event_area->area.x = x;
	event_area->area.y = y;
	event_area->area.width = border;
	event_area->area.height = border;

	event_area = get_event_area_from_id (self, C_TOP_RIGHT_AREA);
	event_area->area.x = x + width - border;
	event_area->area.y = y;
	event_area->area.width = border;
	event_area->area.height = border;

	event_area = get_event_area_from_id (self, C_BOTTOM_LEFT_AREA);
	event_area->area.x = x;
	event_area->area.y = y + height - border;
	event_area->area.width = border;
	event_area->area.height = border;

	event_area = get_event_area_from_id (self, C_BOTTOM_RIGHT_AREA);
	event_area->area.x = x + width - border;
	event_area->area.y = y + height - border;
	event_area->area.width = border;
	event_area->area.height = border;
}


static void
queue_draw (GthImageSelector      *self,
	    cairo_rectangle_int_t  area)
{
	if (! gtk_widget_get_realized (GTK_WIDGET (self->priv->viewer)))
		return;

	gtk_widget_queue_draw_area (GTK_WIDGET (self->priv->viewer),
				    self->priv->viewer->image_area.x + area.x - self->priv->viewer->visible_area.x,
				    self->priv->viewer->image_area.y + area.y - self->priv->viewer->visible_area.y,
				    area.width,
				    area.height);
}


static void
selection_changed (GthImageSelector *self)
{
	update_event_areas (self);
	g_signal_emit (G_OBJECT (self), signals[SELECTION_CHANGED], 0);
}


static void
set_selection_area (GthImageSelector      *self,
		    cairo_rectangle_int_t  new_selection,
		    gboolean               force_update)
{
	cairo_rectangle_int_t old_selection_area;
	cairo_rectangle_int_t dirty_region;

	if (! force_update && rectangle_equal (self->priv->selection_area, new_selection))
		return;

	old_selection_area = self->priv->selection_area;
	self->priv->selection_area = new_selection;
	gdk_rectangle_union (&old_selection_area, &self->priv->selection_area, &dirty_region);
	queue_draw (self, dirty_region);

	selection_changed (self);
}


static void
set_selection (GthImageSelector      *self,
	       cairo_rectangle_int_t  new_selection,
	       gboolean               force_update)
{
	cairo_rectangle_int_t new_area;

	if (! force_update && rectangle_equal (self->priv->selection, new_selection))
		return;

	self->priv->selection = new_selection;
	convert_to_selection_area (self, new_selection, &new_area);
	set_selection_area (self, new_area, force_update);
}


static void
init_selection (GthImageSelector *self)
{
	cairo_rectangle_int_t area;

	/*
	if (! self->priv->use_ratio) {
		area.width = IROUND (self->priv->surface_area.width * 0.5);
		area.height = IROUND (self->priv->surface_area.height * 0.5);
	}
	else {
		if (self->priv->ratio > 1.0) {
			area.width = IROUND (self->priv->surface_area.width * 0.5);
			area.height = IROUND (area.width / self->priv->ratio);
		}
		else {
			area.height = IROUND (self->priv->surface_area.height * 0.5);
			area.width = IROUND (area.height * self->priv->ratio);
		}
	}
	area.x = IROUND ((self->priv->surface_area.width - area.width) / 2.0);
	area.y = IROUND ((self->priv->surface_area.height - area.height) / 2.0);
	*/

	area.x = 0;
	area.y = 0;
	area.height = 0;
	area.width = 0;
	set_selection (self, area, FALSE);
}


static void
gth_image_selector_set_viewer (GthImageViewerTool *base,
			       GthImageViewer     *image_viewer)
{
	GthImageSelector *self = GTH_IMAGE_SELECTOR (base);

	self->priv->viewer = image_viewer;
	gth_image_viewer_show_frame (self->priv->viewer, 25);
}


static void
gth_image_selector_unset_viewer (GthImageViewerTool *base,
		       	         GthImageViewer     *image_viewer)
{
	GthImageSelector *self = GTH_IMAGE_SELECTOR (base);

	if (self->priv->viewer != NULL)
		gth_image_viewer_hide_frame (self->priv->viewer);
	self->priv->viewer = NULL;
}


static void
gth_image_selector_realize (GthImageViewerTool *base)
{
	GthImageSelector *self = GTH_IMAGE_SELECTOR (base);

	if (self->priv->type == GTH_SELECTOR_TYPE_REGION)
		self->priv->default_cursor = _gdk_cursor_new_for_widget (GTK_WIDGET (self->priv->viewer), GDK_CROSSHAIR /*GDK_LEFT_PTR*/);
	else if (self->priv->type == GTH_SELECTOR_TYPE_POINT)
		self->priv->default_cursor = _gdk_cursor_new_for_widget (GTK_WIDGET (self->priv->viewer), GDK_CROSSHAIR);
	gth_image_viewer_set_cursor (self->priv->viewer, self->priv->default_cursor);

	add_event_area (self, C_SELECTION_AREA, GDK_FLEUR);
	add_event_area (self, C_TOP_AREA, GDK_TOP_SIDE);
	add_event_area (self, C_BOTTOM_AREA, GDK_BOTTOM_SIDE);
	add_event_area (self, C_LEFT_AREA, GDK_LEFT_SIDE);
	add_event_area (self, C_RIGHT_AREA, GDK_RIGHT_SIDE);
	add_event_area (self, C_TOP_LEFT_AREA, GDK_TOP_LEFT_CORNER);
	add_event_area (self, C_TOP_RIGHT_AREA, GDK_TOP_RIGHT_CORNER);
	add_event_area (self, C_BOTTOM_LEFT_AREA, GDK_BOTTOM_LEFT_CORNER);
	add_event_area (self, C_BOTTOM_RIGHT_AREA, GDK_BOTTOM_RIGHT_CORNER);

	init_selection (self);
	update_event_areas (self);
}


static void
gth_image_selector_unrealize (GthImageViewerTool *base)
{
	GthImageSelector *self = GTH_IMAGE_SELECTOR (base);

	if (self->priv->default_cursor != NULL) {
		g_object_unref (self->priv->default_cursor);
		self->priv->default_cursor = NULL;
	}

	free_event_area_list (self);
}


static void
gth_image_selector_size_allocate (GthImageViewerTool *base,
				  GtkAllocation      *allocation)
{
	GthImageSelector *self = GTH_IMAGE_SELECTOR (base);

	if (self->priv->surface != NULL)
		selection_changed (self);
}


static void
gth_image_selector_map (GthImageViewerTool *base)
{
	/* void */
}


static void
gth_image_selector_unmap (GthImageViewerTool *base)
{
	/* void */
}


static void
paint_background (GthImageSelector *self,
		  cairo_t          *cr)
{
	gth_image_viewer_paint_region (self->priv->viewer,
				       cr,
				       self->priv->surface,
				       self->priv->viewer->image_area.x,
				       self->priv->viewer->image_area.y,
				       &self->priv->viewer->visible_area,
				       gth_image_viewer_get_zoom_quality_filter (self->priv->viewer));

	/* make the background darker */

	cairo_save (cr);
	cairo_rectangle (cr,
			 self->priv->viewer->image_area.x - self->priv->viewer->visible_area.x,
			 self->priv->viewer->image_area.y - self->priv->viewer->visible_area.y,
			 self->priv->viewer->image_area.width,
			 self->priv->viewer->image_area.height);
	cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 0.5);
	cairo_fill (cr);
	cairo_restore (cr);
}


static void
paint_selection (GthImageSelector *self,
		 cairo_t          *cr)
{
	cairo_rectangle_int_t selection_area;

	if (! self->priv->viewer->dragging) {
		int frame_border = gth_image_viewer_get_frame_border (self->priv->viewer);

		selection_area = self->priv->selection_area;
		selection_area.x += frame_border;
		selection_area.y += frame_border;
		gth_image_viewer_paint_region (self->priv->viewer,
					       cr,
					       self->priv->surface,
					       selection_area.x + self->priv->viewer->image_area.x - self->priv->viewer->visible_area.x,
					       selection_area.y + self->priv->viewer->image_area.y - self->priv->viewer->visible_area.y,
					       &selection_area,
					       gth_image_viewer_get_zoom_quality_filter (self->priv->viewer));
	}

	cairo_save (cr);

	selection_area = self->priv->selection_area;
	selection_area.x += self->priv->viewer->image_area.x - self->priv->viewer->visible_area.x;
	selection_area.y += self->priv->viewer->image_area.y - self->priv->viewer->visible_area.y;
	cairo_rectangle (cr,
			 selection_area.x + 0.5,
			 selection_area.y + 0.5,
			 selection_area.width,
			 selection_area.height);
	cairo_clip (cr);

	_cairo_paint_grid (cr, &selection_area, self->priv->grid_type);

	if ((self->priv->current_area != NULL) && (self->priv->current_area->id != C_SELECTION_AREA)) {
#if CAIRO_VERSION >= CAIRO_VERSION_ENCODE(1, 9, 2)
		cairo_set_operator (cr, CAIRO_OPERATOR_DIFFERENCE);
#endif
		cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 1.0);
		event_area_paint (self, self->priv->current_area, cr);
	}
	cairo_stroke (cr);

	cairo_restore (cr);
}


static void
paint_image (GthImageSelector *self,
	     cairo_t          *cr)
{
	gth_image_viewer_paint_region (self->priv->viewer,
				       cr,
				       self->priv->surface,
				       self->priv->viewer->image_area.x,
				       self->priv->viewer->image_area.y,
				       &self->priv->viewer->visible_area,
				       gth_image_viewer_get_zoom_quality_filter (self->priv->viewer));
}


static void
gth_image_selector_draw (GthImageViewerTool *base,
			 cairo_t            *cr)
{
	GthImageSelector *self = GTH_IMAGE_SELECTOR (base);

	gth_image_viewer_paint_background (self->priv->viewer, cr);

	if (self->priv->surface == NULL)
		return;

	gth_image_viewer_paint_frame (self->priv->viewer, cr);

	if (self->priv->mask_visible) {
		if (self->priv->viewer->dragging)
			paint_image (self, cr);
		else
			paint_background (self, cr);
		paint_selection (self, cr);
	}
	else
		paint_image (self, cr);
}


static int
selector_to_real (GthImageSelector *self,
		  int               value)
{
	return IROUND ((double) value / gth_image_viewer_get_zoom (self->priv->viewer));
}


static void
convert_to_real_selection (GthImageSelector      *self,
			   cairo_rectangle_int_t  selection_area,
			   cairo_rectangle_int_t *real_area)
{
	real_area->x = selector_to_real (self, selection_area.x);
	real_area->y = selector_to_real (self, selection_area.y);
	real_area->width = selector_to_real (self, selection_area.width);
	real_area->height = selector_to_real (self, selection_area.height);
}


static void
set_active_area (GthImageSelector *self,
		 EventArea        *event_area)
{
	if (self->priv->active != (event_area != NULL))
		self->priv->active = ! self->priv->active;

	if (self->priv->current_area != event_area)
		self->priv->current_area = event_area;

	if (self->priv->current_area == NULL)
		gth_image_viewer_set_cursor (self->priv->viewer, self->priv->default_cursor);
	else
		gth_image_viewer_set_cursor (self->priv->viewer, self->priv->current_area->cursor);

	queue_draw (self, self->priv->selection_area);
}


static void
update_cursor (GthImageSelector *self,
	       int               x,
	       int               y)
{
	if (! self->priv->mask_visible)
		return;
	set_active_area (self, get_event_area_from_position (self, x, y));
}


static gboolean
gth_image_selector_button_release (GthImageViewerTool *base,
				   GdkEventButton     *event)
{
	GthImageSelector *self = GTH_IMAGE_SELECTOR (base);

	if (self->priv->timer_id != 0) {
		g_source_remove (self->priv->timer_id);
		self->priv->timer_id = 0;
	}

	update_cursor (self,
		       event->x + self->priv->viewer->visible_area.x,
		       event->y + self->priv->viewer->visible_area.y);
	gtk_widget_queue_draw (GTK_WIDGET (self->priv->viewer));

	return FALSE;
}


static void
grow_upward (cairo_rectangle_int_t *bound,
	     cairo_rectangle_int_t *r,
	     int                    dy,
	     gboolean               check)
{
	if (check && (r->y + dy < 0))
		dy = -r->y;
	r->y += dy;
	r->height -= dy;
}


static void
grow_downward (cairo_rectangle_int_t *bound,
	       cairo_rectangle_int_t *r,
	       int                    dy,
	       gboolean               check)
{
	if (check && (r->y + r->height + dy > bound->height))
		dy = bound->height - (r->y + r->height);
	r->height += dy;
}


static void
grow_leftward (cairo_rectangle_int_t *bound,
	       cairo_rectangle_int_t *r,
	       int                    dx,
	       gboolean               check)
{
	if (check && (r->x + dx < 0))
		dx = -r->x;
	r->x += dx;
	r->width -= dx;
}


static void
grow_rightward (cairo_rectangle_int_t *bound,
		cairo_rectangle_int_t *r,
		int                    dx,
		gboolean               check)
{
	if (check && (r->x + r->width + dx > bound->width))
		dx = bound->width - (r->x + r->width);
	r->width += dx;
}


static int
get_semiplane_no (int x1,
		  int y1,
		  int x2,
		  int y2,
		  int px,
		  int py)
{
	double a, b;

	a = atan ((double) (y1 - y2) / (x2 - x1));
	b = atan ((double) (y1 - py) / (px - x1));

	return (a <= b) && (b <= a + G_PI);
}


static int
bind_dimension (int dimension,
		int factor)
{
	int d;
	int d_next;

	d = (dimension / factor) * factor;
	d_next = d + factor;
	if (dimension - d <= d_next - dimension)
		dimension = d;
	else
		dimension = d_next;

	return dimension;
}


static gboolean
check_and_set_new_selection (GthImageSelector      *self,
			     cairo_rectangle_int_t  new_selection)
{
	new_selection.width = MAX (0, new_selection.width);
	new_selection.height = MAX (0, new_selection.height);

	if (self->priv->bind_dimensions && (self->priv->bind_factor > 1)) {
		new_selection.width = bind_dimension (new_selection.width, self->priv->bind_factor);
		new_selection.height = bind_dimension (new_selection.height, self->priv->bind_factor);
	}

	if (((self->priv->current_area == NULL)
	     || (self->priv->current_area->id != C_SELECTION_AREA))
	    && self->priv->use_ratio)
	{
		if (! rectangle_in_rectangle (new_selection, self->priv->surface_area))
			return FALSE;

		set_selection (self, new_selection, FALSE);
		return TRUE;
	}

	/* self->priv->current_area->id == C_SELECTION_AREA */

	if (new_selection.x < 0)
		new_selection.x = 0;
	if (new_selection.y < 0)
		new_selection.y = 0;
	if (new_selection.width > self->priv->surface_area.width)
		new_selection.width = self->priv->surface_area.width;
	if (new_selection.height > self->priv->surface_area.height)
		new_selection.height = self->priv->surface_area.height;

	if (new_selection.x + new_selection.width > self->priv->surface_area.width)
		new_selection.x = self->priv->surface_area.width - new_selection.width;
	if (new_selection.y + new_selection.height > self->priv->surface_area.height)
		new_selection.y = self->priv->surface_area.height - new_selection.height;

	set_selection (self, new_selection, FALSE);

	return TRUE;
}


static gboolean
gth_image_selector_button_press (GthImageViewerTool *base,
				 GdkEventButton     *event)
{
	GthImageSelector *self = GTH_IMAGE_SELECTOR (base);
	gboolean          retval = FALSE;
	int               x, y;

	if (event->button != 1)
		return FALSE;

	if (! point_in_rectangle (event->x, event->y, self->priv->viewer->image_area))
		return FALSE;

	x = event->x + self->priv->viewer->visible_area.x;
	y = event->y + self->priv->viewer->visible_area.y;

	if (self->priv->current_area == NULL) {
		cairo_rectangle_int_t new_selection;

		new_selection.x = selector_to_real (self, x - self->priv->viewer->image_area.x);
		new_selection.y = selector_to_real (self, y - self->priv->viewer->image_area.y);
		new_selection.width = selector_to_real (self, 1);
		new_selection.height = selector_to_real (self, 1);

		if (self->priv->type == GTH_SELECTOR_TYPE_REGION) {
			check_and_set_new_selection (self, new_selection);
			set_active_area (self, get_event_area_from_id (self, C_BOTTOM_RIGHT_AREA));
		}
		else if (self->priv->type == GTH_SELECTOR_TYPE_POINT) {
			g_signal_emit (G_OBJECT (self),
				       signals[SELECTED],
				       0,
				       new_selection.x,
				       new_selection.y);
			return TRUE;
		}
	}

	if (self->priv->current_area != NULL) {
		self->priv->viewer->pressed = TRUE;
		self->priv->viewer->dragging = TRUE;
		self->priv->drag_start_selection_area = self->priv->selection_area;
		gtk_widget_queue_draw (GTK_WIDGET (self->priv->viewer));
		retval = TRUE;
	}

	return retval;
}


static void
update_mouse_selection (GthImageSelector *self)
{
	gboolean               check = ! self->priv->use_ratio;
	int                    dx, dy;
	cairo_rectangle_int_t  new_selection, tmp;
	int                    semiplane;
	GthEventAreaType       area_type = self->priv->current_area->id;
	EventArea             *event_area;

	dx = selector_to_real (self, self->priv->viewer->drag_x - self->priv->viewer->drag_x_start);
	dy = selector_to_real (self, self->priv->viewer->drag_y - self->priv->viewer->drag_y_start);

	convert_to_real_selection (self,
				   self->priv->drag_start_selection_area,
				   &new_selection);

	if (((area_type == C_LEFT_AREA)
	     || (area_type == C_TOP_LEFT_AREA)
	     || (area_type == C_BOTTOM_LEFT_AREA))
	    && (dx > new_selection.width))
	{
		new_selection.x += new_selection.width;
       		dx = - (2 * new_selection.width) + dx;
       		area_type = get_opposite_event_area_on_x (area_type);
	}
	else if (((area_type == C_RIGHT_AREA)
		  || (area_type == C_TOP_RIGHT_AREA)
		  || (area_type == C_BOTTOM_RIGHT_AREA))
		 && (-dx > new_selection.width))
	{
	    	new_selection.x -= new_selection.width;
       		dx = (2 * new_selection.width) + dx;
       		area_type = get_opposite_event_area_on_x (area_type);
	}

	if (((area_type == C_TOP_AREA)
	     || (area_type == C_TOP_LEFT_AREA)
	     || (area_type == C_TOP_RIGHT_AREA))
	    && (dy > new_selection.height))
	{
	    	new_selection.y += new_selection.height;
       		dy = - (2 * new_selection.height) + dy;
       		area_type = get_opposite_event_area_on_y (area_type);
	}
	else if (((area_type == C_BOTTOM_AREA)
		  || (area_type == C_BOTTOM_LEFT_AREA)
		  || (area_type == C_BOTTOM_RIGHT_AREA))
		 && (-dy > new_selection.height))
	{
       		new_selection.y -= new_selection.height;
       		dy = (2 * new_selection.height) + dy;
       		area_type = get_opposite_event_area_on_y (area_type);
       	}

	event_area = get_event_area_from_id (self, area_type);
	if (event_area != NULL)
		gth_image_viewer_set_cursor (self->priv->viewer, event_area->cursor);

	switch (area_type) {
	case C_SELECTION_AREA:
		new_selection.x += dx;
		new_selection.y += dy;
		break;

	case C_TOP_AREA:
		grow_upward (&self->priv->surface_area, &new_selection, dy, check);
		if (self->priv->use_ratio)
			grow_rightward (&self->priv->surface_area,
					&new_selection,
					IROUND (-dy * self->priv->ratio),
					check);
		break;

	case C_BOTTOM_AREA:
		grow_downward (&self->priv->surface_area, &new_selection, dy, check);
		if (self->priv->use_ratio)
			grow_rightward (&self->priv->surface_area,
				        &new_selection,
				        IROUND (dy * self->priv->ratio),
				        check);
		break;

	case C_LEFT_AREA:
		grow_leftward (&self->priv->surface_area, &new_selection, dx, check);
		if (self->priv->use_ratio)
			grow_downward (&self->priv->surface_area,
				       &new_selection,
				       IROUND (-dx / self->priv->ratio),
				       check);
		break;

	case C_RIGHT_AREA:
		grow_rightward (&self->priv->surface_area, &new_selection, dx, check);
		if (self->priv->use_ratio)
			grow_downward (&self->priv->surface_area,
				       &new_selection,
				       IROUND (dx / self->priv->ratio),
				       check);
		break;

	case C_TOP_LEFT_AREA:
		if (self->priv->use_ratio) {
			tmp = self->priv->selection_area;
			semiplane = get_semiplane_no (tmp.x + tmp.width,
						      tmp.y + tmp.height,
						      tmp.x,
						      tmp.y,
						      self->priv->viewer->drag_x - self->priv->viewer->image_area.x,
						      self->priv->viewer->drag_y - self->priv->viewer->image_area.y);
			if (semiplane == 1)
				dy = IROUND (dx / self->priv->ratio);
			else
				dx = IROUND (dy * self->priv->ratio);
		}
		grow_upward (&self->priv->surface_area, &new_selection, dy, check);
		grow_leftward (&self->priv->surface_area, &new_selection, dx, check);
		break;

	case C_TOP_RIGHT_AREA:
		if (self->priv->use_ratio) {
			tmp = self->priv->selection_area;
			semiplane = get_semiplane_no (tmp.x,
						      tmp.y + tmp.height,
						      tmp.x + tmp.width,
						      tmp.y,
						      self->priv->viewer->drag_x - self->priv->viewer->image_area.x,
						      self->priv->viewer->drag_y - self->priv->viewer->image_area.y);
			if (semiplane == 1)
				dx = IROUND (-dy * self->priv->ratio);
			else
				dy = IROUND (-dx / self->priv->ratio);
		}
		grow_upward (&self->priv->surface_area, &new_selection, dy, check);
		grow_rightward (&self->priv->surface_area, &new_selection, dx, check);
		break;

	case C_BOTTOM_LEFT_AREA:
		if (self->priv->use_ratio) {
			tmp = self->priv->selection_area;
			semiplane = get_semiplane_no (tmp.x + tmp.width,
						      tmp.y,
						      tmp.x,
						      tmp.y + tmp.height,
						      self->priv->viewer->drag_x - self->priv->viewer->image_area.x,
						      self->priv->viewer->drag_y - self->priv->viewer->image_area.y);
			if (semiplane == 1)
				dx = IROUND (-dy * self->priv->ratio);
			else
				dy = IROUND (-dx / self->priv->ratio);
		}
		grow_downward (&self->priv->surface_area, &new_selection, dy, check);
		grow_leftward (&self->priv->surface_area, &new_selection, dx, check);
		break;

	case C_BOTTOM_RIGHT_AREA:
		if (self->priv->use_ratio) {
			tmp = self->priv->selection_area;
			semiplane = get_semiplane_no (tmp.x,
						      tmp.y,
						      tmp.x + tmp.width,
						      tmp.y + tmp.height,
						      self->priv->viewer->drag_x - self->priv->viewer->image_area.x,
						      self->priv->viewer->drag_y - self->priv->viewer->image_area.y);

			if (semiplane == 1)
				dy = IROUND (dx / self->priv->ratio);
			else
				dx = IROUND (dy * self->priv->ratio);
		}
		grow_downward (&self->priv->surface_area, &new_selection, dy, check);
		grow_rightward (&self->priv->surface_area, &new_selection, dx, check);
		break;

	default:
		break;
	}

	/* if the user is changing selection size and the selection has a fixed
	 * ratio and it goes out of the image bounds, resize the selection in
	 * order to stay inside the image bounds. */

	if ((area_type != C_SELECTION_AREA)
	    && self->priv->use_ratio
	    && ! rectangle_in_rectangle (new_selection, self->priv->surface_area))
	{
		switch (area_type) {
		case C_TOP_AREA:
			if (new_selection.y < self->priv->surface_area.y) {
				dy = new_selection.y + new_selection.height;
				dx = IROUND (dy * self->priv->ratio);
				new_selection.y = new_selection.y + new_selection.height - dy;
				new_selection.width = dx;
				new_selection.height = dy;
			}
			if (new_selection.x + new_selection.width > self->priv->surface_area.width) {
				dx = self->priv->surface_area.width - new_selection.x;
				dy = IROUND (dx / self->priv->ratio);
				new_selection.y = new_selection.y + new_selection.height - dy;
				new_selection.width = dx;
				new_selection.height = dy;
			}
			break;

		case C_RIGHT_AREA:
			if (new_selection.x + new_selection.width > self->priv->surface_area.width) {
				dx = self->priv->surface_area.width - new_selection.x;
				dy = IROUND (dx / self->priv->ratio);
				new_selection.width = dx;
				new_selection.height = dy;
			}
			if (new_selection.y + new_selection.height > self->priv->surface_area.height) {
				dy = self->priv->surface_area.height - new_selection.y;
				dx = IROUND (dy * self->priv->ratio);
				new_selection.width = dx;
				new_selection.height = dy;
			}
			break;

		case C_TOP_RIGHT_AREA:
			if (new_selection.x + new_selection.width > self->priv->surface_area.width) {
				dx = self->priv->surface_area.width - new_selection.x;
				dy = IROUND (dx / self->priv->ratio);
				new_selection.y = new_selection.y + new_selection.height - dy;
				new_selection.width = dx;
				new_selection.height = dy;
			}
			if (new_selection.y < self->priv->surface_area.y) {
				dy = new_selection.y + new_selection.height;
				dx = IROUND (dy * self->priv->ratio);
				new_selection.y = new_selection.y + new_selection.height - dy;
				new_selection.width = dx;
				new_selection.height = dy;
			}
			if (new_selection.x + new_selection.width > self->priv->surface_area.width) {
				dx = self->priv->surface_area.width - new_selection.x;
				dy = IROUND (dx / self->priv->ratio);
				new_selection.y = new_selection.y + new_selection.height - dy;
				new_selection.width = dx;
				new_selection.height = dy;
			}
			break;

		case C_TOP_LEFT_AREA:
			if (new_selection.x < self->priv->surface_area.y) {
				dx = new_selection.x + new_selection.width;
				dy = IROUND (dx / self->priv->ratio);
				new_selection.x = new_selection.x + new_selection.width - dx;
				new_selection.y = new_selection.y + new_selection.height - dy;
				new_selection.width = dx;
				new_selection.height = dy;
			}
			if (new_selection.y < self->priv->surface_area.y) {
				dy = new_selection.y + new_selection.height;
				dx = IROUND (dy * self->priv->ratio);
				new_selection.x = new_selection.x + new_selection.width - dx;
				new_selection.y = new_selection.y + new_selection.height - dy;
				new_selection.width = dx;
				new_selection.height = dy;
			}
			if (new_selection.x < self->priv->surface_area.y) {
				dx = new_selection.x + new_selection.width;
				dy = IROUND (dx / self->priv->ratio);
				new_selection.x = new_selection.x + new_selection.width - dx;
				new_selection.y = new_selection.y + new_selection.height - dy;
				new_selection.width = dx;
				new_selection.height = dy;
			}
			break;

		case C_BOTTOM_RIGHT_AREA:
			if (new_selection.x + new_selection.width > self->priv->surface_area.width) {
				dx = self->priv->surface_area.width - new_selection.x;
				dy = IROUND (dx / self->priv->ratio);
				new_selection.width = dx;
				new_selection.height = dy;
			}
			if (new_selection.y + new_selection.height > self->priv->surface_area.height) {
				dy = self->priv->surface_area.height - new_selection.y;
				dx = IROUND (dy * self->priv->ratio);
				new_selection.width = dx;
				new_selection.height = dy;
			}
			if (new_selection.x + new_selection.width > self->priv->surface_area.width) {
				dx = self->priv->surface_area.width - new_selection.x;
				dy = IROUND (dx / self->priv->ratio);
				new_selection.width = dx;
				new_selection.height = dy;
			}
			break;

		case C_BOTTOM_AREA:
			if (new_selection.y + new_selection.height > self->priv->surface_area.height) {
				dy = self->priv->surface_area.height - new_selection.y;
				dx = IROUND (dy * self->priv->ratio);
				new_selection.width = dx;
				new_selection.height = dy;
			}
			if (new_selection.x + new_selection.width > self->priv->surface_area.width) {
				dx = self->priv->surface_area.width - new_selection.x;
				dy = IROUND (dx / self->priv->ratio);
				new_selection.width = dx;
				new_selection.height = dy;
			}
			break;

		case C_LEFT_AREA:
			if (new_selection.x < self->priv->surface_area.y) {
				dx = new_selection.x + new_selection.width;
				dy = IROUND (dx / self->priv->ratio);
				new_selection.x = new_selection.x + new_selection.width - dx;
				new_selection.width = dx;
				new_selection.height = dy;
			}
			if (new_selection.y + new_selection.height > self->priv->surface_area.height) {
				dy = self->priv->surface_area.height - new_selection.y;
				dx = IROUND (dy * self->priv->ratio);
				new_selection.x = new_selection.x + new_selection.width - dx;
				new_selection.width = dx;
				new_selection.height = dy;
			}
			break;

		case C_BOTTOM_LEFT_AREA:
			if (new_selection.x < self->priv->surface_area.y) {
				dx = new_selection.x + new_selection.width;
				dy = IROUND (dx / self->priv->ratio);
				new_selection.x = new_selection.x + new_selection.width - dx;
				new_selection.width = dx;
				new_selection.height = dy;
			}
			if (new_selection.y + new_selection.height > self->priv->surface_area.height) {
				dy = self->priv->surface_area.height - new_selection.y;
				dx = IROUND (dy * self->priv->ratio);
				new_selection.x = new_selection.x + new_selection.width - dx;
				new_selection.width = dx;
				new_selection.height = dy;
			}
			if (new_selection.x < self->priv->surface_area.y) {
				dx = new_selection.x + new_selection.width;
				dy = IROUND (dx / self->priv->ratio);
				new_selection.x = new_selection.x + new_selection.width - dx;
				new_selection.width = dx;
				new_selection.height = dy;
			}
			break;

		default:
			break;
		}
	}

	check_and_set_new_selection (self, new_selection);
}


static gboolean
autoscroll_cb (gpointer data)
{
	GthImageSelector *self = GTH_IMAGE_SELECTOR (data);
	double            value;
	double            max_value;

	/* drag x */

	value = gtk_adjustment_get_value (self->priv->viewer->hadj) + self->priv->x_value_diff;
	max_value = gtk_adjustment_get_upper (self->priv->viewer->hadj) - gtk_adjustment_get_page_size (self->priv->viewer->hadj);
	if (value > max_value)
		value = max_value;
	gtk_adjustment_set_value (self->priv->viewer->hadj, value);
	self->priv->viewer->drag_x = self->priv->viewer->drag_x + self->priv->x_value_diff;

	/* drag y */

	value = gtk_adjustment_get_value (self->priv->viewer->vadj) + self->priv->y_value_diff;
	max_value = gtk_adjustment_get_upper (self->priv->viewer->vadj) - gtk_adjustment_get_page_size (self->priv->viewer->vadj);
	if (value > max_value)
		value = max_value;
	gtk_adjustment_set_value (self->priv->viewer->vadj, value);
	self->priv->viewer->drag_y = self->priv->viewer->drag_y + self->priv->y_value_diff;

	update_mouse_selection (self);
	gtk_widget_queue_draw (GTK_WIDGET (self->priv->viewer));

	return TRUE;
}


static gboolean
gth_image_selector_motion_notify (GthImageViewerTool *base,
				  GdkEventMotion     *event)
{
	GthImageSelector *self = GTH_IMAGE_SELECTOR (base);
	GtkWidget        *widget;
	int               x, y;
	GtkAllocation     allocation;
	int               absolute_x, absolute_y;

	widget = GTK_WIDGET (self->priv->viewer);
	x = event->x + self->priv->viewer->visible_area.x;
	y = event->y + self->priv->viewer->visible_area.y;

	if (self->priv->type == GTH_SELECTOR_TYPE_POINT) {
		x = selector_to_real (self, x - self->priv->viewer->image_area.x);
		y = selector_to_real (self, y - self->priv->viewer->image_area.y);
		if (point_in_rectangle (x, y, self->priv->surface_area))
			g_signal_emit (G_OBJECT (self), signals[MOTION_NOTIFY], 0, x, y);
		return TRUE;
	}

	/* type == GTH_SELECTOR_TYPE_REGION */

	if (! self->priv->viewer->dragging
	    && self->priv->viewer->pressed
	    && ((abs (self->priv->viewer->drag_x - self->priv->viewer->drag_x_prev) > DRAG_THRESHOLD)
		|| (abs (self->priv->viewer->drag_y - self->priv->viewer->drag_y_prev) > DRAG_THRESHOLD))
	    && (self->priv->current_area != NULL))
	{
		GdkGrabStatus status;

		status = gdk_seat_grab (gdk_device_get_seat (gdk_event_get_device ((GdkEvent *) event)),
					gtk_widget_get_window (widget),
					GDK_SEAT_CAPABILITY_ALL_POINTING,
					TRUE,
					self->priv->current_area->cursor,
					(GdkEvent *) event,
					NULL,
					NULL);
		if (status == GDK_GRAB_SUCCESS)
			self->priv->viewer->dragging = TRUE;

		return FALSE;
	}

	if (! self->priv->viewer->dragging) {
		update_cursor (self, x, y);
		return FALSE;
	}

	/* dragging == TRUE */

	update_mouse_selection (self);

	/* If we are out of bounds, schedule a timeout that will do
	 * the scrolling */

	gtk_widget_get_allocation (widget, &allocation);
	absolute_x = event->x;
	absolute_y = event->y;
	if ((absolute_y < 0) || (absolute_y > allocation.height)
	    || (absolute_x < 0) || (absolute_x > allocation.width))
	{

		/* Make the steppings be relative to the mouse
		 * distance from the canvas.
		 * Also notice the timeout is small to give a smoother
		 * movement.
		 */
		if (absolute_x < 0)
			self->priv->x_value_diff = absolute_x;
		else if (absolute_x > allocation.width)
			self->priv->x_value_diff = absolute_x - allocation.width;
		else
			self->priv->x_value_diff = 0.0;
		self->priv->x_value_diff /= 2;

		if (absolute_y < 0)
			self->priv->y_value_diff = absolute_y;
		else if (absolute_y > allocation.height)
			self->priv->y_value_diff = absolute_y -allocation.height;
		else
			self->priv->y_value_diff = 0.0;
		self->priv->y_value_diff /= 2;

		if (self->priv->timer_id == 0)
			self->priv->timer_id = gdk_threads_add_timeout (SCROLL_TIMEOUT,
									autoscroll_cb,
									self);
	}
	else if (self->priv->timer_id != 0) {
		g_source_remove (self->priv->timer_id);
		self->priv->timer_id = 0;
	}

	return FALSE;
}


static gboolean
selection_is_valid (GthImageSelector *self)
{
	cairo_region_t *surface_region;
	gboolean        valid;

	surface_region = cairo_region_create_rectangle (&self->priv->surface_area);
	valid = cairo_region_contains_rectangle (surface_region, &self->priv->selection_area) == CAIRO_REGION_OVERLAP_IN;
	cairo_region_destroy (surface_region);

	return valid;
}


static void
update_selection (GthImageSelector *self)
{
	cairo_rectangle_int_t selection;

	gth_image_selector_get_selection (self, &selection);
	set_selection (self, selection, TRUE);
}


static void
gth_image_selector_image_changed (GthImageViewerTool *base)
{
	GthImageSelector *self = GTH_IMAGE_SELECTOR (base);

	cairo_surface_destroy (self->priv->surface);
	self->priv->surface = gth_image_viewer_get_current_image (self->priv->viewer);

	if (self->priv->surface == NULL) {
		self->priv->surface_area.width = 0;
		self->priv->surface_area.height = 0;
		return;
	}

	self->priv->surface = cairo_surface_reference (self->priv->surface);
	if (! _cairo_image_surface_get_original_size (self->priv->surface,
						      &self->priv->surface_area.width,
						      &self->priv->surface_area.height))
	{
		self->priv->surface_area.width = cairo_image_surface_get_width (self->priv->surface);
		self->priv->surface_area.height = cairo_image_surface_get_height (self->priv->surface);
	}

	if (! selection_is_valid (self))
		init_selection (self);
	else
		update_selection (self);
}


static void
gth_image_selector_zoom_changed (GthImageViewerTool *base)
{
	update_selection (GTH_IMAGE_SELECTOR (base));
}


static void
gth_image_selector_init (GthImageSelector *self)
{
	self->priv = gth_image_selector_get_instance_private (self);

	self->priv->type = GTH_SELECTOR_TYPE_REGION;
	self->priv->ratio = 1.0;
	self->priv->mask_visible = TRUE;
	self->priv->grid_type = GTH_GRID_NONE;
	self->priv->bind_dimensions = FALSE;
	self->priv->bind_factor = 1;
}


static void
gth_image_selector_finalize (GObject *object)
{
	GthImageSelector *self;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GTH_IS_IMAGE_SELECTOR (object));

	self = (GthImageSelector *) object;
	cairo_surface_destroy (self->priv->surface);

	/* Chain up */
	G_OBJECT_CLASS (gth_image_selector_parent_class)->finalize (object);
}


static void
gth_image_selector_class_init (GthImageSelectorClass *class)
{
	GObjectClass *gobject_class;

	gobject_class = (GObjectClass*) class;
	gobject_class->finalize = gth_image_selector_finalize;

	signals[SELECTION_CHANGED] = g_signal_new ("selection_changed",
						   G_TYPE_FROM_CLASS (class),
						   G_SIGNAL_RUN_LAST,
			 			   G_STRUCT_OFFSET (GthImageSelectorClass, selection_changed),
			 		 	   NULL, NULL,
			 			   g_cclosure_marshal_VOID__VOID,
			 			   G_TYPE_NONE,
			 			   0);
	signals[MOTION_NOTIFY] = g_signal_new ("motion_notify",
					       G_TYPE_FROM_CLASS (class),
					       G_SIGNAL_RUN_LAST,
					       G_STRUCT_OFFSET (GthImageSelectorClass, motion_notify),
					       NULL, NULL,
					       gth_marshal_VOID__INT_INT,
					       G_TYPE_NONE,
					       2,
					       G_TYPE_INT,
					       G_TYPE_INT);
	signals[SELECTED] = g_signal_new ("selected",
					  G_TYPE_FROM_CLASS (class),
					  G_SIGNAL_RUN_LAST,
					  G_STRUCT_OFFSET (GthImageSelectorClass, selected),
					  NULL, NULL,
					  gth_marshal_VOID__INT_INT,
					  G_TYPE_NONE,
					  2,
					  G_TYPE_INT,
					  G_TYPE_INT);
	signals[MASK_VISIBILITY_CHANGED] = g_signal_new ("mask_visibility_changed",
							 G_TYPE_FROM_CLASS (class),
							 G_SIGNAL_RUN_LAST,
							 G_STRUCT_OFFSET (GthImageSelectorClass, mask_visibility_changed),
							 NULL, NULL,
							 g_cclosure_marshal_VOID__VOID,
							 G_TYPE_NONE,
							 0);
        signals[GRID_VISIBILITY_CHANGED] = g_signal_new ("grid_visibility_changed",
                                                         G_TYPE_FROM_CLASS (class),
                                                         G_SIGNAL_RUN_LAST,
                                                         G_STRUCT_OFFSET (GthImageSelectorClass, grid_visibility_changed),
                                                         NULL, NULL,
                                                         g_cclosure_marshal_VOID__VOID,
                                                         G_TYPE_NONE,
                                                         0);
}


static void
gth_image_selector_gth_image_tool_interface_init (GthImageViewerToolInterface *iface)
{
	iface->set_viewer = gth_image_selector_set_viewer;
	iface->unset_viewer = gth_image_selector_unset_viewer;
	iface->realize = gth_image_selector_realize;
	iface->unrealize = gth_image_selector_unrealize;
	iface->size_allocate = gth_image_selector_size_allocate;
	iface->map = gth_image_selector_map;
	iface->unmap = gth_image_selector_unmap;
	iface->draw = gth_image_selector_draw;
	iface->button_press = gth_image_selector_button_press;
	iface->button_release = gth_image_selector_button_release;
	iface->motion_notify = gth_image_selector_motion_notify;
	iface->image_changed = gth_image_selector_image_changed;
	iface->zoom_changed = gth_image_selector_zoom_changed;
}


GthImageViewerTool *
gth_image_selector_new (GthSelectorType type)
{
	GthImageSelector *selector;

	selector = g_object_new (GTH_TYPE_IMAGE_SELECTOR, NULL);
	selector->priv->type = type;

	return GTH_IMAGE_VIEWER_TOOL (selector);
}


gboolean
gth_image_selector_set_selection_x (GthImageSelector *self,
				    int               x)
{
	cairo_rectangle_int_t new_selection;

	new_selection = self->priv->selection;
	new_selection.x = x;
	return check_and_set_new_selection (self, new_selection);
}


gboolean
gth_image_selector_set_selection_y (GthImageSelector *self,
				    int               y)
{
	cairo_rectangle_int_t new_selection;

	new_selection = self->priv->selection;
	new_selection.y = y;
	return check_and_set_new_selection (self, new_selection);
}


gboolean
gth_image_selector_set_selection_pos (GthImageSelector *self,
				      int               x,
				      int               y)
{
	cairo_rectangle_int_t new_selection;

	new_selection = self->priv->selection;
	new_selection.x = x;
	new_selection.y = y;
	return check_and_set_new_selection (self, new_selection);
}


gboolean
gth_image_selector_set_selection_width (GthImageSelector *self,
					int               width)
{
	cairo_rectangle_int_t new_selection;

	new_selection = self->priv->selection;
	new_selection.width = width;
	if (self->priv->use_ratio)
		new_selection.height = IROUND (width / self->priv->ratio);
	return check_and_set_new_selection (self, new_selection);
}


gboolean
gth_image_selector_set_selection_height (GthImageSelector *self,
					 int               height)
{
	cairo_rectangle_int_t new_selection;

	new_selection = self->priv->selection;
	new_selection.height = height;
	if (self->priv->use_ratio)
		new_selection.width = IROUND (height * self->priv->ratio);
	return check_and_set_new_selection (self, new_selection);
}


void
gth_image_selector_set_selection (GthImageSelector      *self,
				  cairo_rectangle_int_t  selection)
{
	set_selection (self, selection, FALSE);
}


void
gth_image_selector_get_selection (GthImageSelector      *self,
				  cairo_rectangle_int_t *selection)
{
	selection->x = MAX (self->priv->selection.x, 0);
	selection->y = MAX (self->priv->selection.y, 0);
	selection->width = MIN (self->priv->selection.width, self->priv->surface_area.width - self->priv->selection.x);
	selection->height = MIN (self->priv->selection.height, self->priv->surface_area.height - self->priv->selection.y);
}


void
gth_image_selector_set_ratio (GthImageSelector *self,
			      gboolean          use_ratio,
			      double            ratio,
			      gboolean		swap_x_and_y_to_start)
{
	int new_starting_width;

	self->priv->use_ratio = use_ratio;
	self->priv->ratio = ratio;

	if (self->priv->use_ratio) {
		/* When changing the cropping aspect ratio, it looks more natural
		   to swap the height and width, rather than (for example) keeping
		   the width constant and shrinking the height. */
		if (swap_x_and_y_to_start == TRUE)
			new_starting_width = self->priv->selection.height;
		else
	       		new_starting_width = self->priv->selection.width;

		gth_image_selector_set_selection_width (self, new_starting_width);
		gth_image_selector_set_selection_height (self, self->priv->selection.height);

		/* However, if swapping the height and width fails because it exceeds
		   the image size, then revert to keeping the width constant and shrinking
		   the height. That is guaranteed to fit inside the old selection. */
		if ( (swap_x_and_y_to_start == TRUE) &&
		     (self->priv->selection.width != new_starting_width))
		{
			gth_image_selector_set_selection_width (self, self->priv->selection.width);
			gth_image_selector_set_selection_height (self, self->priv->selection.height);
		}
	}
}


double
gth_image_selector_get_ratio (GthImageSelector *self)
{
	return self->priv->ratio;
}


gboolean
gth_image_selector_get_use_ratio (GthImageSelector *self)
{
	return self->priv->use_ratio;
}


void
gth_image_selector_set_mask_visible (GthImageSelector *self,
				     gboolean          visible)
{
	if (visible == self->priv->mask_visible)
		return;

	self->priv->mask_visible = visible;
	if (self->priv->viewer != NULL)
		gtk_widget_queue_draw (GTK_WIDGET (self->priv->viewer));
	g_signal_emit (G_OBJECT (self),
		       signals[MASK_VISIBILITY_CHANGED],
		       0);
}


void
gth_image_selector_set_grid_type (GthImageSelector *self,
                                  GthGridType       grid_type)
{
	if (grid_type == self->priv->grid_type)
		return;

	self->priv->grid_type = grid_type;
	if (self->priv->viewer != NULL)
		gtk_widget_queue_draw (GTK_WIDGET (self->priv->viewer));
	g_signal_emit (G_OBJECT (self),
		       signals[GRID_VISIBILITY_CHANGED],
		       0);
}


gboolean
gth_image_selector_get_mask_visible (GthImageSelector *self)
{
	return self->priv->mask_visible;
}


GthGridType
gth_image_selector_get_grid_type (GthImageSelector *self)
{
	return self->priv->grid_type;
}


void
gth_image_selector_bind_dimensions (GthImageSelector *self,
				    gboolean          bind,
				    int               factor)
{
	self->priv->bind_dimensions = bind;
	self->priv->bind_factor = factor;
}


void
gth_image_selector_center (GthImageSelector *self)
{
	cairo_rectangle_int_t new_selection;

	new_selection = self->priv->selection;
	new_selection.x = (self->priv->surface_area.width - new_selection.width) / 2;
	new_selection.y = (self->priv->surface_area.height - new_selection.height) / 2;
	check_and_set_new_selection (self, new_selection);
}
