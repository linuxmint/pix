/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001-2011 Free Software Foundation, Inc.
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
#include <glib/gi18n.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include "cairo-utils.h"
#include "gth-enum-types.h"
#include "gth-image-dragger.h"
#include "gth-image-viewer.h"
#include "gth-marshal.h"
#include "gtk-utils.h"
#include "glib-utils.h"
#include "pixbuf-utils.h"


#define DRAG_THRESHOLD  1     /* When dragging the image ignores movements
			       * smaller than this value. */
#define MINIMUM_DELAY   10    /* When an animation frame has a 0 milli seconds
			       * delay use this delay instead. */
#define STEP_INCREMENT  20.0  /* Scroll increment. */
#define GRAY_VALUE 0.2
#define CHECKED_PATTERN_SIZE 20


enum {
	PROP_0,
	PROP_HADJUSTMENT,
	PROP_VADJUSTMENT,
	PROP_HSCROLL_POLICY,
	PROP_VSCROLL_POLICY
};


enum {
	CLICKED,
	ZOOM_IN,
	ZOOM_OUT,
	SET_ZOOM,
	SET_FIT_MODE,
	IMAGE_CHANGED,
	ZOOM_CHANGED,
	BETTER_QUALITY,
	SCROLL,
	LAST_SIGNAL
};


struct _GthImageViewerPrivate {
	GtkScrollablePolicy     hscroll_policy;
	GtkScrollablePolicy     vscroll_policy;

	GthImage               *image;
	cairo_surface_t        *surface;
	cairo_pattern_t        *background_pattern;
	GdkPixbufAnimation     *animation;
	int                     original_width;
	int                     original_height;
	int                     requested_size;

	GdkPixbufAnimationIter *iter;
	GTimeVal                time;               /* Timer used to get the current frame. */
	guint                   anim_id;
	cairo_surface_t        *iter_surface;

	gboolean                is_animation;
	gboolean                play_animation;
	gboolean                cursor_visible;

	gboolean                frame_visible;
	int                     frame_border;

	GthImageViewerTool     *tool;

	GdkCursor              *cursor;
	GdkCursor              *cursor_void;

	gboolean                zoom_enabled;
	gboolean                enable_key_bindings;
	double                  zoom_level;
	guint                   zoom_quality : 1;   /* A ZoomQualityType value. */
	guint                   zoom_change : 3;    /* A ZoomChangeType value. */

	GthFit                  fit;
	gboolean                doing_zoom_fit;     /* Whether we are performing
					             * a zoom to fit the window. */
	gboolean                is_void;            /* If TRUE do not show anything.
					             * It is reset to FALSE we an
					             * image is loaded. */
	gboolean                double_click;
	gboolean                just_focused;
	gboolean                skip_zoom_change;
	gboolean                update_image_after_zoom;
	gboolean                reset_scrollbars;
	GthTransparencyStyle    transparency_style;
	GList                  *painters;
};


static guint gth_image_viewer_signals[LAST_SIGNAL] = { 0 };


G_DEFINE_TYPE_WITH_CODE (GthImageViewer,
			 gth_image_viewer,
			 GTK_TYPE_WIDGET,
			 G_ADD_PRIVATE (GthImageViewer)
			 G_IMPLEMENT_INTERFACE (GTK_TYPE_SCROLLABLE, NULL))


typedef struct {
	GthImageViewerPaintFunc func;
	gpointer                user_data;
} PainterData;


static void
painter_data_free (PainterData *painter_data)
{
	g_free (painter_data);
}


static void
gth_image_viewer_finalize (GObject *object)
{
	GthImageViewer *self;

	g_return_if_fail (GTH_IS_IMAGE_VIEWER (object));

	self = GTH_IMAGE_VIEWER (object);

	if (self->priv->anim_id != 0)
		g_source_remove (self->priv->anim_id);

	if (self->priv->cursor != NULL)
		g_object_unref (self->priv->cursor);

	if (self->priv->cursor_void != NULL)
		g_object_unref (self->priv->cursor_void);

	if (self->hadj != NULL) {
		_g_signal_handlers_disconnect_by_data (G_OBJECT (self->hadj), self);
		g_object_unref (self->hadj);
	}

	if (self->vadj != NULL) {
		_g_signal_handlers_disconnect_by_data (G_OBJECT (self->vadj), self);
		g_object_unref (self->vadj);
	}

	g_list_foreach (self->priv->painters, (GFunc) painter_data_free, NULL);
	_g_object_unref (self->priv->tool);

	_g_clear_object (&self->priv->image);
	_g_clear_object (&self->priv->animation);
	_g_clear_object (&self->priv->iter);
	_cairo_clear_surface (&self->priv->iter_surface);
	_cairo_clear_surface (&self->priv->surface);
	cairo_pattern_destroy (self->priv->background_pattern);

	G_OBJECT_CLASS (gth_image_viewer_parent_class)->finalize (object);
}


#define MIN_ZOOM 0.05
#define MAX_ZOOM 100.0
#define ZOOM_STEP 0.1


static double
get_next_zoom (gdouble zoom)
{
	zoom = zoom + (zoom * ZOOM_STEP);
	return CLAMP (zoom, MIN_ZOOM, MAX_ZOOM);
}


static double
get_prev_zoom (double zoom)
{
	zoom = zoom - (zoom * ZOOM_STEP);
	return CLAMP (zoom, MIN_ZOOM, MAX_ZOOM);
}


static double
_gth_image_viewer_get_quality_zoom (GthImageViewer *self)
{
	cairo_surface_t *image;

	image = gth_image_viewer_get_current_image (self);
	if (self->priv->original_width <= 0)
		return 1.0;

	return (double) cairo_image_surface_get_width (image) / self->priv->original_width;
}


static int
_gth_image_viewer_get_frame_border (GthImageViewer *self)
{
	if (! self->priv->frame_visible)
		return 0;

	return self->priv->frame_border;
}


static void
_gth_image_viewer_get_zoomed_size_for_zoom (GthImageViewer *self,
					    int            *width,
					    int            *height,
					    double          zoom_level)
{
	if (gth_image_viewer_get_current_image (self) == NULL) {
		if (width != NULL) *width = 0;
		if (height != NULL) *height = 0;
	}
	else {
		int frame_border = _gth_image_viewer_get_frame_border (self);
		if (width != NULL) *width  = (int) round ((double) self->priv->original_width * zoom_level) + (frame_border * 2);
		if (height != NULL) *height = (int) round ((double) self->priv->original_height * zoom_level) + (frame_border * 2);
	}
}


static void
_gth_image_viewer_get_zoomed_size (GthImageViewer *self,
		 	 	   int            *width,
		 	 	   int            *height)
{
	_gth_image_viewer_get_zoomed_size_for_zoom (self, width, height, self->priv->zoom_level);
}


static void
_gth_image_viewer_update_visible_area (GthImageViewer *self,
				       GtkAllocation  *allocation)
{
	GtkAllocation local_allocation;

	if (allocation != NULL)
		local_allocation = *allocation;
	else
		gtk_widget_get_allocation (GTK_WIDGET (self), &local_allocation);

	self->visible_area.width = local_allocation.width;
	self->visible_area.height = local_allocation.height;
}


static void
_gth_image_viewer_update_image_area (GthImageViewer *self)
{
	int zoomed_width;
	int zoomed_height;

	_gth_image_viewer_get_zoomed_size (self, &zoomed_width, &zoomed_height);

	self->frame_area.x = MAX (0, (self->visible_area.width - zoomed_width) / 2);
	self->frame_area.y = MAX (0, (self->visible_area.height - zoomed_height) / 2);
	self->frame_area.width = zoomed_width;
	self->frame_area.height = zoomed_height;

	self->image_area.x = self->frame_area.x + self->priv->frame_border;
	self->image_area.y = self->frame_area.y + self->priv->frame_border;
	self->image_area.width = self->frame_area.width - (self->priv->frame_border * 2);
	self->image_area.height = self->frame_area.height - (self->priv->frame_border * 2);
}


static void _set_surface (GthImageViewer  *self,
			  cairo_surface_t *surface,
			  int              original_width,
			  int              original_height,
			  gboolean         better_quality);


static void
set_zoom (GthImageViewer *self,
	  gdouble         zoom_level,
	  int             pointer_x,
	  int             pointer_y)
{
	g_return_if_fail (self != NULL);

	if (gth_image_get_is_zoomable (self->priv->image)) {
		int original_width;
		int original_height;

		if (gth_image_set_zoom (self->priv->image,
				        zoom_level,
				        &original_width,
				        &original_height))
		{
			cairo_surface_t *surface;

			surface = gth_image_get_cairo_surface (self->priv->image);
			if (surface != NULL) {
				_set_surface (self,
					      surface,
					      original_width,
					      original_height,
					      TRUE);
				cairo_surface_destroy (surface);
			}
		}
	}

	/* Try to keep the same pixel under the pointer after the zoom. */

	{
		cairo_surface_t *image;

		image = gth_image_viewer_get_current_image (self);
		if (image != NULL) {
			int    frame_border;
			double quality_zoom;
			double old_zoom_level;
			int    x, y;
			double x0, y0;
			double dx, dy;
			int    new_center_x, new_center_y;

			frame_border = _gth_image_viewer_get_frame_border (self);
			quality_zoom = (double) self->priv->original_width / cairo_image_surface_get_width (image);
			old_zoom_level = self->priv->zoom_level * quality_zoom;

			/* Pixel under the pointer. */
			x = (pointer_x + self->visible_area.x - self->image_area.x) / old_zoom_level;
			y = (pointer_y + self->visible_area.y - self->image_area.y) / old_zoom_level;

			/* Pixel at the center of the visibile area. */
			x0 = ((self->visible_area.width / 2) + self->visible_area.x - self->image_area.x) / old_zoom_level;
			y0 = ((self->visible_area.height / 2) + self->visible_area.y - self->image_area.y) / old_zoom_level;

			/* Delta to keep the (x, y) pixel under the pointer.  */
			dx = ((x - x0) * zoom_level) - ((x - x0) * old_zoom_level);
			dy = ((y - y0) * zoom_level) - ((y - y0) * old_zoom_level);

			/* Center on (x0, y0) and add (dx, dy) */
			new_center_x = round ((x0 * zoom_level * quality_zoom) + frame_border + dx);
			new_center_y = round ((y0 * zoom_level * quality_zoom) + frame_border + dy);

			self->visible_area.x = new_center_x - (self->visible_area.width / 2);
			self->visible_area.y = new_center_y - (self->visible_area.height / 2);
		}
	}

	self->priv->zoom_level = zoom_level;

	_gth_image_viewer_update_image_area (self);
	if (self->priv->update_image_after_zoom) {
		g_signal_emit (G_OBJECT (self), gth_image_viewer_signals[IMAGE_CHANGED], 0);
		gth_image_viewer_tool_image_changed (self->priv->tool);
		self->priv->update_image_after_zoom = FALSE;
		self->priv->skip_zoom_change = TRUE;
	}
	else
		gth_image_viewer_tool_zoom_changed (self->priv->tool);

	if (! self->priv->skip_zoom_change)
		g_signal_emit (G_OBJECT (self), gth_image_viewer_signals[ZOOM_CHANGED], 0);
	else
		self->priv->skip_zoom_change = FALSE;
}


static void
set_zoom_centered_at (GthImageViewer *self,
		      double          zoom_level,
		      gboolean        zoom_to_fit,
		      int             center_x,
		      int             center_y)
{
	/* reset zoom_fit unless we are performing a zoom to fit. */
	if (! zoom_to_fit)
		self->priv->fit = GTH_FIT_NONE;
	set_zoom (self, zoom_level, center_x, center_y);
}


static void
set_zoom_centered (GthImageViewer *self,
		   gdouble         zoom_level,
		   gboolean        zoom_to_fit)
{
	set_zoom_centered_at (self,
			      zoom_level,
			      zoom_to_fit,
			      self->visible_area.width / 2,
			      self->visible_area.height / 2);
}


static void
gth_image_viewer_realize (GtkWidget *widget)
{
	GthImageViewer *self;
	GtkAllocation   allocation;
	GdkWindowAttr   attributes;
	int             attributes_mask;
	GdkWindow      *window;

	g_return_if_fail (GTH_IS_IMAGE_VIEWER (widget));

	self = GTH_IMAGE_VIEWER (widget);
	gtk_widget_set_realized (widget, TRUE);
	gtk_widget_get_allocation (widget, &allocation);

	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.x           = allocation.x;
	attributes.y           = allocation.y;
	attributes.width       = allocation.width;
	attributes.height      = allocation.height;
	attributes.wclass      = GDK_INPUT_OUTPUT;
	attributes.visual      = gtk_widget_get_visual (widget);
	attributes.event_mask  = (gtk_widget_get_events (widget)
				  | GDK_EXPOSURE_MASK
				  | GDK_BUTTON_PRESS_MASK
				  | GDK_BUTTON_RELEASE_MASK
				  | GDK_POINTER_MOTION_MASK
				  | GDK_POINTER_MOTION_HINT_MASK
				  | GDK_BUTTON_MOTION_MASK
				  | GDK_SCROLL_MASK
				  | GDK_STRUCTURE_MASK);
	attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_WMCLASS;
	window = gdk_window_new (gtk_widget_get_parent_window (widget),
				 &attributes,
				 attributes_mask);
	gtk_widget_register_window (widget, window);
	gtk_widget_set_window (widget, window);
	gtk_style_context_set_background (gtk_widget_get_style_context (widget), window);

	self->priv->cursor = _gdk_cursor_new_for_widget (widget, GDK_LEFT_PTR);
	self->priv->cursor_void = _gdk_cursor_new_for_widget (widget, GDK_BLANK_CURSOR);
	if (self->priv->cursor_visible)
		gdk_window_set_cursor (window, self->priv->cursor);
	else
		gdk_window_set_cursor (window, self->priv->cursor_void);

	gth_image_viewer_tool_realize (self->priv->tool);
}


static void
gth_image_viewer_unrealize (GtkWidget *widget)
{
	GthImageViewer *self;

	g_return_if_fail (GTH_IS_IMAGE_VIEWER (widget));

	self = GTH_IMAGE_VIEWER (widget);

	if (self->priv->cursor) {
		g_object_unref (self->priv->cursor);
		self->priv->cursor = NULL;
	}
	if (self->priv->cursor_void) {
		g_object_unref (self->priv->cursor_void);
		self->priv->cursor_void = NULL;
	}

	gth_image_viewer_tool_unrealize (self->priv->tool);

	GTK_WIDGET_CLASS (gth_image_viewer_parent_class)->unrealize (widget);
}


static void
gth_image_viewer_map (GtkWidget *widget)
{
	GthImageViewer *self;

	g_return_if_fail (GTH_IS_IMAGE_VIEWER (widget));

	GTK_WIDGET_CLASS (gth_image_viewer_parent_class)->map (widget);

	self = GTH_IMAGE_VIEWER (widget);
	gth_image_viewer_tool_map (self->priv->tool);
}


static void
gth_image_viewer_unmap (GtkWidget *widget)
{
	GthImageViewer *self;

	g_return_if_fail (GTH_IS_IMAGE_VIEWER (widget));

	self = GTH_IMAGE_VIEWER (widget);
	gth_image_viewer_tool_unmap (self->priv->tool);

	GTK_WIDGET_CLASS (gth_image_viewer_parent_class)->unmap (widget);
}


static void
_gth_image_viewer_configure_hadjustment (GthImageViewer *self)
{
	int zoomed_width;

	_gth_image_viewer_get_zoomed_size (self, &zoomed_width, NULL);
	gtk_adjustment_configure (self->hadj,
				  self->visible_area.x,
				  0.0,
				  zoomed_width,
				  STEP_INCREMENT,
				  self->visible_area.width / 2,
				  self->visible_area.width);
}


static void
_gth_image_viewer_configure_vadjustment (GthImageViewer *self)
{
	int zoomed_height;

	_gth_image_viewer_get_zoomed_size (self, NULL, &zoomed_height);
	gtk_adjustment_configure (self->vadj,
				  self->visible_area.y,
				  0.0,
				  zoomed_height,
				  STEP_INCREMENT,
				  self->visible_area.height / 2,
				  self->visible_area.height);
}


static GtkSizeRequestMode
gth_image_viewer_get_request_mode (GtkWidget *widget)
{
	return GTK_SIZE_REQUEST_CONSTANT_SIZE;
}


static void
gth_image_viewer_get_preferred_width (GtkWidget *widget,
				      int       *minimum_width,
				      int       *natural_width)
{
	GthImageViewer *self = GTH_IMAGE_VIEWER (widget);
	int             zoomed_width;

	_gth_image_viewer_get_zoomed_size (self, &zoomed_width, NULL);
	*minimum_width = 0;
	*natural_width = zoomed_width;
}


static void
gth_image_viewer_get_preferred_height (GtkWidget *widget,
				       int       *minimum_height,
				       int       *natural_height)
{
	GthImageViewer *self = GTH_IMAGE_VIEWER (widget);
	int             zoomed_height;

	_gth_image_viewer_get_zoomed_size (self, NULL, &zoomed_height);
	*minimum_height = 0;
	*natural_height = zoomed_height;
}


static double
get_zoom_to_fit (GthImageViewer *self,
		 GtkAllocation  *allocation)
{
	int    original_width = 0;
	int    original_height = 0;
	int    frame_border_2;
	double x_level;
	double y_level;

	gth_image_viewer_get_original_size (self, &original_width, &original_height);
	frame_border_2 = _gth_image_viewer_get_frame_border (self) * 2;

	if ((original_width == 0) || (original_height == 0))
		return 1.0;

	x_level = (double) (allocation->width - frame_border_2) / original_width;
	y_level = (double) (allocation->height - frame_border_2) / original_height;

	return (x_level < y_level) ? x_level : y_level;
}


static double
get_zoom_to_fit_width (GthImageViewer *self,
		       GtkAllocation  *allocation)
{
	int original_width = 0;
	int frame_border_2;

	gth_image_viewer_get_original_size (self, &original_width, NULL);
	frame_border_2 = _gth_image_viewer_get_frame_border (self) * 2;

	if (original_width > 0)
		return (double) (allocation->width - frame_border_2) / original_width;
	else
		return 0;
}


static double
get_zoom_to_fit_height (GthImageViewer *self,
		        GtkAllocation  *allocation)
{
	int original_height = 0;
	int frame_border_2;

	gth_image_viewer_get_original_size (self, NULL, &original_height);
	frame_border_2 = _gth_image_viewer_get_frame_border (self) * 2;

	if (original_height > 0)
		return (double) (allocation->height - frame_border_2) / original_height;
	else
		return 0;
}


static double
get_zoom_level_for_allocation (GthImageViewer *self,
			       GtkAllocation  *allocation)
{
	double           zoom_level;
	cairo_surface_t *current_image;
	int              original_width;
	int              original_height;
	int              frame_border_2;

	zoom_level = self->priv->zoom_level;
	current_image = gth_image_viewer_get_current_image (self);
	if (self->priv->is_void || (current_image == NULL))
		return zoom_level;

	gth_image_viewer_get_original_size (self, &original_width, &original_height);
	frame_border_2 = _gth_image_viewer_get_frame_border (self) * 2;

	switch (self->priv->fit) {
	case GTH_FIT_SIZE:
		zoom_level = get_zoom_to_fit (self, allocation);
		break;
	case GTH_FIT_SIZE_IF_LARGER:
		if ((allocation->width < original_width + frame_border_2) || (allocation->height < original_height + frame_border_2))
			zoom_level = get_zoom_to_fit (self, allocation);
		else
			zoom_level = 1.0;
		break;
	case GTH_FIT_WIDTH:
		zoom_level = get_zoom_to_fit_width (self, allocation);
		break;
	case GTH_FIT_WIDTH_IF_LARGER:
		if (allocation->width < original_width + frame_border_2)
			zoom_level = get_zoom_to_fit_width (self, allocation);
		else
			zoom_level = 1.0;
		break;
	case GTH_FIT_HEIGHT:
		zoom_level = get_zoom_to_fit_height (self, allocation);
		break;
	case GTH_FIT_HEIGHT_IF_LARGER:
		if (allocation->height < original_height + frame_border_2)
			zoom_level = get_zoom_to_fit_height (self, allocation);
		else
			zoom_level = 1.0;
		break;
	default:
		break;
	}

	return zoom_level;
}


static void
gth_image_viewer_size_allocate (GtkWidget     *widget,
				GtkAllocation *allocation)
{
	GthImageViewer  *self = GTH_IMAGE_VIEWER (widget);
	double           zoom_level;
	int              zoomed_width;
	int              zoomed_height;
	cairo_surface_t *current_image;

	_gth_image_viewer_update_visible_area (self, allocation);

	gtk_widget_set_allocation (widget, allocation);
	if (gtk_widget_get_realized (widget))
		gdk_window_move_resize (gtk_widget_get_window (widget),
					allocation->x,
					allocation->y,
					allocation->width,
					allocation->height);

	/* update the zoom level if the automatic fit mode is active */

	zoom_level = get_zoom_level_for_allocation (self, allocation);
	if (self->priv->fit != GTH_FIT_NONE)
		set_zoom_centered (self, zoom_level, TRUE);

	/* Keep the scrollbars offset in a valid range */

	current_image = gth_image_viewer_get_current_image (self);
	if (current_image != NULL) {
		_gth_image_viewer_get_zoomed_size (self, &zoomed_width, &zoomed_height);
		self->visible_area.x = (zoomed_width <= self->visible_area.width) ? 0 : CLAMP (self->visible_area.x, 0, zoomed_width - self->visible_area.width);
		self->visible_area.y = (zoomed_height <= self->visible_area.height) ? 0 : CLAMP (self->visible_area.y, 0, zoomed_height - self->visible_area.height);
	}

	_gth_image_viewer_configure_hadjustment (self);
	_gth_image_viewer_configure_vadjustment (self);
	_gth_image_viewer_update_image_area (self);
	gth_image_viewer_tool_size_allocate (self->priv->tool, allocation);
}


static gboolean
gth_image_viewer_focus_in (GtkWidget     *widget,
			   GdkEventFocus *event)
{
	gtk_widget_queue_draw (widget);
	return TRUE;
}


static gboolean
gth_image_viewer_focus_out (GtkWidget     *widget,
			    GdkEventFocus *event)
{
	gtk_widget_queue_draw (widget);
	return TRUE;
}


static int
gth_image_viewer_key_press (GtkWidget   *widget,
			    GdkEventKey *event)
{
	gboolean handled;

	handled = gtk_bindings_activate (G_OBJECT (widget),
					 event->keyval,
					 event->state);
	if (handled)
		return TRUE;

	if (GTK_WIDGET_CLASS (gth_image_viewer_parent_class)->key_press_event &&
	    GTK_WIDGET_CLASS (gth_image_viewer_parent_class)->key_press_event (widget, event))
		return TRUE;

	return FALSE;
}


static gboolean
change_animation_frame (gpointer data)
{
	GthImageViewer *self = data;

	if (self->priv->anim_id != 0) {
		g_source_remove (self->priv->anim_id);
		self->priv->anim_id = 0;
	}

	if (self->priv->is_void
	    || ! self->priv->is_animation
	    || (self->priv->iter == NULL))
	{
		return FALSE;
	}

	g_time_val_add (&self->priv->time, (glong) gdk_pixbuf_animation_iter_get_delay_time (self->priv->iter) * 1000);
	gdk_pixbuf_animation_iter_advance (self->priv->iter, &self->priv->time);

	_cairo_clear_surface (&self->priv->iter_surface);
	gtk_widget_queue_draw (GTK_WIDGET (self));

	return FALSE;
}


static void
queue_animation_frame_change (GthImageViewer *self)
{
	if (! self->priv->is_void
	    && self->priv->is_animation
	    && self->priv->play_animation
	    && (self->priv->anim_id == 0)
	    && (self->priv->iter != NULL))
	{
		self->priv->anim_id = g_timeout_add (MAX (gdk_pixbuf_animation_iter_get_delay_time (self->priv->iter), MINIMUM_DELAY),
						     change_animation_frame,
						     self);
	}
}


static gboolean
gth_image_viewer_draw (GtkWidget *widget,
		       cairo_t   *cr)
{
	GthImageViewer *self = GTH_IMAGE_VIEWER (widget);

	cairo_save (cr);

	/* set the default values of the cairo context */

	cairo_set_line_width (cr, 0.5);
	cairo_set_antialias (cr, CAIRO_ANTIALIAS_NONE);

	/* delegate the rest to the tool  */

	gth_image_viewer_tool_draw (self->priv->tool, cr);

	cairo_restore (cr);

	queue_animation_frame_change (self);

	return TRUE;
}


static gboolean
gth_image_viewer_button_press (GtkWidget      *widget,
			       GdkEventButton *event)
{
	GthImageViewer *self = GTH_IMAGE_VIEWER (widget);
	int             retval;

	if (! gtk_widget_has_focus (widget)) {
		gtk_widget_grab_focus (widget);
		self->priv->just_focused = TRUE;
	}

	if (self->dragging)
		return FALSE;

	self->priv->double_click = ((event->type == GDK_2BUTTON_PRESS) || (event->type == GDK_3BUTTON_PRESS));

	retval = gth_image_viewer_tool_button_press (self->priv->tool, event);

	if (self->pressed && ! self->priv->double_click) {
		self->event_x_start = self->event_x_prev = event->x;
		self->event_y_start = self->event_y_prev = event->y;
		self->drag_x = self->drag_x_start = self->drag_x_prev = event->x + self->visible_area.x;
		self->drag_y = self->drag_y_start = self->drag_y_prev = event->y + self->visible_area.y;
	}

	return retval;
}

static void
_gth_image_viewer_button_release (GthImageViewer *self,
		 	 	  GdkEventButton *event)
{
	gth_image_viewer_tool_button_release (self->priv->tool, event);

	self->priv->just_focused = FALSE;
	self->pressed = FALSE;
	self->dragging = FALSE;
}


static gboolean
gth_image_viewer_button_release (GtkWidget      *widget,
				 GdkEventButton *event)
{
	GthImageViewer *self = GTH_IMAGE_VIEWER (widget);

	if ((event->button == 1)
	    && ! self->dragging
	    && ! self->priv->double_click
	    && ! self->priv->just_focused)
	{
		g_signal_emit (G_OBJECT (self),
			       gth_image_viewer_signals[CLICKED],
			       0);
	}

	_gth_image_viewer_button_release (self, event);

	return FALSE;
}


static void
scroll_to (GthImageViewer *self,
	   int             x_offset,
	   int             y_offset)
{
	int        delta_x, delta_y;
	GdkWindow *window;

	g_return_if_fail (self != NULL);

	if (gth_image_viewer_get_current_image (self) == NULL)
		return;

	if (self->frame_area.width > self->visible_area.width)
		x_offset = CLAMP (x_offset, 0, self->frame_area.width - self->visible_area.width);
	else
		x_offset = self->visible_area.x;

	if (self->frame_area.height > self->visible_area.height)
		y_offset = CLAMP (y_offset, 0, self->frame_area.height - self->visible_area.height);
	else
		y_offset = self->visible_area.y;

	if ((x_offset == self->visible_area.x) && (y_offset == self->visible_area.y))
		return;

	delta_x = x_offset - self->visible_area.x;
	delta_y = y_offset - self->visible_area.y;

	self->visible_area.x = x_offset;
	self->visible_area.y = y_offset;

	window = gtk_widget_get_window (GTK_WIDGET (self));
	gdk_window_scroll (window, -delta_x, -delta_y);
}


static gboolean
gth_image_viewer_motion_notify (GtkWidget      *widget,
				GdkEventMotion *event)
{
	GthImageViewer *self = GTH_IMAGE_VIEWER (widget);

	if (self->pressed) {
		self->drag_x = event->x + self->visible_area.x;
		self->drag_y = event->y + self->visible_area.y;
	}

	gth_image_viewer_tool_motion_notify (self->priv->tool, event);

	if (self->pressed) {
		self->event_x_prev = event->x;
		self->event_y_prev = event->y;
		self->drag_x_prev = self->drag_x;
		self->drag_y_prev = self->drag_y;
	}

	return FALSE;
}


static gboolean
gth_image_viewer_scroll_event (GtkWidget      *widget,
			       GdkEventScroll *event)
{
	GthImageViewer *self = GTH_IMAGE_VIEWER (widget);
	gdouble         new_value = 0.0;
	gboolean        retval = FALSE;

	g_return_val_if_fail (GTH_IS_IMAGE_VIEWER (widget), FALSE);
	g_return_val_if_fail (event != NULL, FALSE);

	/* Control + Scroll-Up / Control + Scroll-Down ==> Zoom In / Zoom Out */

	if (event->state & GDK_CONTROL_MASK) {
		if (self->priv->zoom_enabled && gth_image_viewer_zoom_from_scroll (self, event))
			return TRUE;
	}

	/* Scroll Left / Scroll Right ==> Scroll the image horizontally */

	switch (event->direction) {
	case GDK_SCROLL_LEFT:
	case GDK_SCROLL_RIGHT:
		if (event->direction == GDK_SCROLL_LEFT)
			new_value = gtk_adjustment_get_value (self->hadj) - gtk_adjustment_get_page_increment (self->hadj) / 2;
		else
			new_value = gtk_adjustment_get_value (self->hadj) + gtk_adjustment_get_page_increment (self->hadj) / 2;
		new_value = CLAMP (new_value, gtk_adjustment_get_lower (self->hadj), gtk_adjustment_get_upper (self->hadj) - gtk_adjustment_get_page_size (self->hadj));
		gtk_adjustment_set_value (self->hadj, new_value);
		retval = TRUE;
		break;

	default:
		break;
	}

	return retval;
}


static void
gth_image_viewer_drag_end (GtkWidget      *widget,
			   GdkDragContext *context)
{
	GthImageViewer *self;

	g_return_if_fail (GTH_IS_IMAGE_VIEWER (widget));

	self = GTH_IMAGE_VIEWER (widget);
	_gth_image_viewer_button_release (self, NULL);
}


static void
scroll_relative (GthImageViewer *self,
		 int             delta_x,
		 int             delta_y)
{
	gth_image_viewer_scroll_to (self,
				    self->visible_area.x + delta_x,
				    self->visible_area.y + delta_y);
}


static void
scroll_signal (GtkWidget     *widget,
	       GtkScrollType  xscroll_type,
	       GtkScrollType  yscroll_type)
{
	GthImageViewer *self = GTH_IMAGE_VIEWER (widget);
	int             xstep, ystep;

	if (! self->priv->enable_key_bindings)
		return;

	switch (xscroll_type) {
	case GTK_SCROLL_STEP_LEFT:
		xstep = - gtk_adjustment_get_step_increment (self->hadj);
		break;
	case GTK_SCROLL_STEP_RIGHT:
		xstep = gtk_adjustment_get_step_increment (self->hadj);
		break;
	case GTK_SCROLL_PAGE_LEFT:
		xstep = - gtk_adjustment_get_page_increment (self->hadj);
		break;
	case GTK_SCROLL_PAGE_RIGHT:
		xstep = gtk_adjustment_get_page_increment (self->hadj);
		break;
	default:
		xstep = 0;
		break;
	}

	switch (yscroll_type) {
	case GTK_SCROLL_STEP_UP:
		ystep = - gtk_adjustment_get_step_increment (self->vadj);
		break;
	case GTK_SCROLL_STEP_DOWN:
		ystep = gtk_adjustment_get_step_increment (self->vadj);
		break;
	case GTK_SCROLL_PAGE_UP:
		ystep = - gtk_adjustment_get_page_increment (self->vadj);
		break;
	case GTK_SCROLL_PAGE_DOWN:
		ystep = gtk_adjustment_get_page_increment (self->vadj);
		break;
	default:
		ystep = 0;
		break;
	}

	scroll_relative (self, xstep, ystep);
}


static gboolean
hadj_value_changed (GtkAdjustment  *adj,
		    GthImageViewer *self)
{
	scroll_to (self, (int) gtk_adjustment_get_value (adj), self->visible_area.y);
	return FALSE;
}


static gboolean
vadj_value_changed (GtkAdjustment  *adj,
		    GthImageViewer *self)
{
	scroll_to (self, self->visible_area.x, (int) gtk_adjustment_get_value (adj));
	return FALSE;
}


static void
_gth_image_viewer_set_hadjustment (GthImageViewer *self,
				   GtkAdjustment  *hadj)
{
	if (hadj != NULL)
		g_return_if_fail (GTK_IS_ADJUSTMENT (hadj));
	else
		hadj = gtk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0);

	if ((self->hadj != NULL) && (self->hadj != hadj)) {
		_g_signal_handlers_disconnect_by_data (G_OBJECT (self->hadj), self);
		g_object_unref (self->hadj);
		self->hadj = NULL;
	}

	if (self->hadj != hadj) {
		self->hadj = hadj;
		g_object_ref (self->hadj);
		g_object_ref_sink (self->hadj);

		_gth_image_viewer_configure_hadjustment (self);

		g_signal_connect (G_OBJECT (self->hadj),
				  "value-changed",
				  G_CALLBACK (hadj_value_changed),
				  self);
	}
}


static void
_gth_image_viewer_set_vadjustment (GthImageViewer *self,
				   GtkAdjustment  *vadj)
{
	if (vadj != NULL)
		g_return_if_fail (GTK_IS_ADJUSTMENT (vadj));
	else
		vadj = gtk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0);

	if ((self->vadj != NULL) && (self->vadj != vadj)) {
		_g_signal_handlers_disconnect_by_data (G_OBJECT (self->vadj), self);
		g_object_unref (self->vadj);
		self->vadj = NULL;
	}

	if (self->vadj != vadj) {
		self->vadj = vadj;
		g_object_ref (self->vadj);
		g_object_ref_sink (self->vadj);

		_gth_image_viewer_configure_vadjustment (self);

		g_signal_connect (G_OBJECT (self->vadj),
				  "value-changed",
				  G_CALLBACK (vadj_value_changed),
				  self);
	}
}


static void
gth_image_viewer_zoom_in__key_binding (GthImageViewer *self)
{
	if (self->priv->enable_key_bindings)
		gth_image_viewer_zoom_in (self);
}


static void
gth_image_viewer_zoom_out__key_binding (GthImageViewer *self)
{
	if (self->priv->enable_key_bindings)
		gth_image_viewer_zoom_out (self);
}

static void
gth_image_viewer_set_fit_mode__key_binding (GthImageViewer *self,
					    GthFit          fit_mode)
{
	if (self->priv->enable_key_bindings)
		gth_image_viewer_set_fit_mode (self, fit_mode);
}


static void
gth_image_viewer_set_zoom__key_binding (GthImageViewer *self,
					gdouble         zoom_level)
{
	if (self->priv->enable_key_bindings)
		gth_image_viewer_set_zoom (self, zoom_level);
}


static void
gth_image_viewer_get_property (GObject     *object,
			       guint        prop_id,
			       GValue      *value,
			       GParamSpec  *pspec)
{
	GthImageViewer *self = GTH_IMAGE_VIEWER (object);

	switch (prop_id) {
	case PROP_HADJUSTMENT:
		g_value_set_object (value, self->hadj);
		break;
	case PROP_VADJUSTMENT:
		g_value_set_object (value, self->vadj);
		break;
	case PROP_HSCROLL_POLICY:
		g_value_set_enum (value, self->priv->hscroll_policy);
		break;
	case PROP_VSCROLL_POLICY:
		g_value_set_enum (value, self->priv->vscroll_policy);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}


static void
gth_image_viewer_set_property (GObject      *object,
			       guint         prop_id,
			       const GValue *value,
			       GParamSpec   *pspec)
{
	GthImageViewer *self = GTH_IMAGE_VIEWER (object);

	switch (prop_id) {
	case PROP_HADJUSTMENT:
		_gth_image_viewer_set_hadjustment (self, (GtkAdjustment *) g_value_get_object (value));
		break;
	case PROP_VADJUSTMENT:
		_gth_image_viewer_set_vadjustment (self, (GtkAdjustment *) g_value_get_object (value));
		break;
	case PROP_HSCROLL_POLICY:
		self->priv->hscroll_policy = g_value_get_enum (value);
		break;
	case PROP_VSCROLL_POLICY:
		self->priv->vscroll_policy = g_value_get_enum (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}


static void
gth_image_viewer_class_init (GthImageViewerClass *class)
{
	GObjectClass   *gobject_class;
	GtkWidgetClass *widget_class;
	GtkBindingSet  *binding_set;

	/* signals */

	gth_image_viewer_signals[CLICKED] =
		g_signal_new ("clicked",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthImageViewerClass, clicked),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
	gth_image_viewer_signals[ZOOM_IN] =
		g_signal_new ("zoom_in",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			      G_STRUCT_OFFSET (GthImageViewerClass, zoom_in),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
	gth_image_viewer_signals[ZOOM_OUT] =
		g_signal_new ("zoom_out",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			      G_STRUCT_OFFSET (GthImageViewerClass, zoom_out),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
	gth_image_viewer_signals[SET_ZOOM] =
		g_signal_new ("set-zoom",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			      G_STRUCT_OFFSET (GthImageViewerClass, set_zoom),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__DOUBLE,
			      G_TYPE_NONE,
			      1,
			      G_TYPE_DOUBLE);
	gth_image_viewer_signals[SET_FIT_MODE] =
		g_signal_new ("set-fit-mode",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			      G_STRUCT_OFFSET (GthImageViewerClass, set_fit_mode),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__ENUM,
			      G_TYPE_NONE,
			      1,
			      GTH_TYPE_FIT);
	gth_image_viewer_signals[IMAGE_CHANGED] =
		g_signal_new ("image-changed",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthImageViewerClass, image_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
	gth_image_viewer_signals[ZOOM_CHANGED] =
		g_signal_new ("zoom-changed",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthImageViewerClass, zoom_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
	gth_image_viewer_signals[BETTER_QUALITY] =
		g_signal_new ("better-quality",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthImageViewerClass, better_quality),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
	gth_image_viewer_signals[SCROLL] =
		g_signal_new ("scroll",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			      G_STRUCT_OFFSET (GthImageViewerClass, scroll),
			      NULL, NULL,
			      gth_marshal_VOID__ENUM_ENUM,
			      G_TYPE_NONE,
			      2,
			      GTK_TYPE_SCROLL_TYPE,
			      GTK_TYPE_SCROLL_TYPE);

	/**/

	gobject_class = (GObjectClass*) class;
	gobject_class->finalize = gth_image_viewer_finalize;
	gobject_class->set_property = gth_image_viewer_set_property;
	gobject_class->get_property = gth_image_viewer_get_property;

	widget_class = (GtkWidgetClass*) class;
	widget_class->realize = gth_image_viewer_realize;
	widget_class->unrealize = gth_image_viewer_unrealize;
	widget_class->map = gth_image_viewer_map;
	widget_class->unmap = gth_image_viewer_unmap;
	widget_class->get_request_mode = gth_image_viewer_get_request_mode;
	widget_class->get_preferred_width = gth_image_viewer_get_preferred_width;
	widget_class->get_preferred_height = gth_image_viewer_get_preferred_height;
	widget_class->size_allocate = gth_image_viewer_size_allocate;
	widget_class->focus_in_event = gth_image_viewer_focus_in;
	widget_class->focus_out_event = gth_image_viewer_focus_out;
	widget_class->key_press_event = gth_image_viewer_key_press;
	widget_class->draw = gth_image_viewer_draw;
	widget_class->button_press_event = gth_image_viewer_button_press;
	widget_class->button_release_event = gth_image_viewer_button_release;
	widget_class->motion_notify_event = gth_image_viewer_motion_notify;
	widget_class->scroll_event = gth_image_viewer_scroll_event;
	widget_class->drag_end = gth_image_viewer_drag_end;

	class->clicked      = NULL;
	class->zoom_changed = NULL;
	class->scroll       = scroll_signal;
	class->zoom_in      = gth_image_viewer_zoom_in__key_binding;
	class->zoom_out     = gth_image_viewer_zoom_out__key_binding;
	class->set_zoom     = gth_image_viewer_set_zoom__key_binding;
	class->set_fit_mode = gth_image_viewer_set_fit_mode__key_binding;

	/* GtkScrollable interface */

	g_object_class_override_property (gobject_class, PROP_HADJUSTMENT, "hadjustment");
	g_object_class_override_property (gobject_class, PROP_VADJUSTMENT, "vadjustment");
	g_object_class_override_property (gobject_class, PROP_HSCROLL_POLICY, "hscroll-policy");
	g_object_class_override_property (gobject_class, PROP_VSCROLL_POLICY, "vscroll-policy");

	/* Add key bindings */

	binding_set = gtk_binding_set_by_class (class);

	/* For scrolling */

	gtk_binding_entry_add_signal (binding_set, GDK_KEY_Right, 0,
			      "scroll", 2,
			      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_STEP_RIGHT,
			      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_NONE);
	gtk_binding_entry_add_signal (binding_set, GDK_KEY_Left, 0,
			      "scroll", 2,
			      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_STEP_LEFT,
			      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_NONE);
	gtk_binding_entry_add_signal (binding_set, GDK_KEY_Down, 0,
			      "scroll", 2,
			      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_NONE,
			      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_STEP_DOWN);
	gtk_binding_entry_add_signal (binding_set, GDK_KEY_Up, 0,
			      "scroll", 2,
			      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_NONE,
			      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_STEP_UP);

	gtk_binding_entry_add_signal (binding_set, GDK_KEY_Right, GDK_SHIFT_MASK,
			      "scroll", 2,
			      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_PAGE_RIGHT,
			      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_NONE);
	gtk_binding_entry_add_signal (binding_set, GDK_KEY_Left, GDK_SHIFT_MASK,
			      "scroll", 2,
			      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_PAGE_LEFT,
			      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_NONE);
	gtk_binding_entry_add_signal (binding_set, GDK_KEY_Down, GDK_SHIFT_MASK,
			      "scroll", 2,
			      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_NONE,
			      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_PAGE_DOWN);
	gtk_binding_entry_add_signal (binding_set, GDK_KEY_Up, GDK_SHIFT_MASK,
			      "scroll", 2,
			      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_NONE,
			      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_PAGE_UP);

	gtk_binding_entry_add_signal (binding_set, GDK_KEY_Page_Down, 0,
			      "scroll", 2,
			      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_NONE,
			      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_PAGE_DOWN);
	gtk_binding_entry_add_signal (binding_set, GDK_KEY_Page_Up, 0,
			      "scroll", 2,
			      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_NONE,
			      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_PAGE_UP);

	/* For scrolling (Keypad) */

	gtk_binding_entry_add_signal (binding_set, GDK_KEY_KP_Right, 0,
			      "scroll", 2,
			      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_STEP_RIGHT,
			      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_NONE);
	gtk_binding_entry_add_signal (binding_set, GDK_KEY_KP_Left, 0,
			      "scroll", 2,
			      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_STEP_LEFT,
			      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_NONE);
	gtk_binding_entry_add_signal (binding_set, GDK_KEY_KP_Down, 0,
			      "scroll", 2,
			      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_NONE,
			      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_STEP_DOWN);
	gtk_binding_entry_add_signal (binding_set, GDK_KEY_KP_Up, 0,
			      "scroll", 2,
			      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_NONE,
			      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_STEP_UP);

	/* Zoom in */

	gtk_binding_entry_add_signal (binding_set, GDK_KEY_plus, 0,
				      "zoom_in", 0);
	gtk_binding_entry_add_signal (binding_set, GDK_KEY_equal, 0,
				      "zoom_in", 0);
	gtk_binding_entry_add_signal (binding_set, GDK_KEY_KP_Add, 0,
				      "zoom_in", 0);

	/* Zoom out */

	gtk_binding_entry_add_signal (binding_set, GDK_KEY_minus, 0,
				      "zoom_out", 0);
	gtk_binding_entry_add_signal (binding_set, GDK_KEY_KP_Subtract, 0,
				      "zoom_out", 0);

	/* Set zoom */

	gtk_binding_entry_add_signal (binding_set, GDK_KEY_KP_Divide, 0,
				      "set_zoom", 1,
				      G_TYPE_DOUBLE, 1.0);
	gtk_binding_entry_add_signal (binding_set, GDK_KEY_1, 0,
				      "set_zoom", 1,
				      G_TYPE_DOUBLE, 1.0);
	gtk_binding_entry_add_signal (binding_set, GDK_KEY_z, 0,
				      "set_zoom", 1,
				      G_TYPE_DOUBLE, 1.0);
	gtk_binding_entry_add_signal (binding_set, GDK_KEY_2, 0,
				      "set_zoom", 1,
				      G_TYPE_DOUBLE, 2.0);
	gtk_binding_entry_add_signal (binding_set, GDK_KEY_3, 0,
				      "set_zoom", 1,
				      G_TYPE_DOUBLE, 3.0);

	/* Set fit mode */

	gtk_binding_entry_add_signal (binding_set, GDK_KEY_x, 0,
				      "set_fit_mode", 1,
				      GTH_TYPE_FIT, GTH_FIT_SIZE_IF_LARGER);
	gtk_binding_entry_add_signal (binding_set, GDK_KEY_KP_Multiply, 0,
				      "set_fit_mode", 1,
				      GTH_TYPE_FIT, GTH_FIT_SIZE_IF_LARGER);
	gtk_binding_entry_add_signal (binding_set, GDK_KEY_x, GDK_SHIFT_MASK,
				      "set_fit_mode", 1,
				      GTH_TYPE_FIT, GTH_FIT_SIZE);
	gtk_binding_entry_add_signal (binding_set, GDK_KEY_w, 0,
				      "set_fit_mode", 1,
				      GTH_TYPE_FIT, GTH_FIT_WIDTH_IF_LARGER);
	gtk_binding_entry_add_signal (binding_set, GDK_KEY_w, GDK_SHIFT_MASK,
				      "set_fit_mode", 1,
				      GTH_TYPE_FIT, GTH_FIT_WIDTH);
	gtk_binding_entry_add_signal (binding_set, GDK_KEY_h, 0,
				      "set_fit_mode", 1,
				      GTH_TYPE_FIT, GTH_FIT_HEIGHT_IF_LARGER);
	gtk_binding_entry_add_signal (binding_set, GDK_KEY_h, GDK_SHIFT_MASK,
				      "set_fit_mode", 1,
				      GTH_TYPE_FIT, GTH_FIT_HEIGHT);
}


static void
halt_animation (GthImageViewer *self)
{
	g_return_if_fail (self != NULL);

	if (self->priv->anim_id != 0) {
		g_source_remove (self->priv->anim_id);
		self->priv->anim_id = 0;
	}
}


static void
gth_image_viewer_init (GthImageViewer *self)
{
	gtk_widget_set_can_focus (GTK_WIDGET (self), TRUE);
	gtk_widget_set_has_window (GTK_WIDGET (self), TRUE);

	/* Initialize data. */

	self->priv = gth_image_viewer_get_instance_private (self);

	self->priv->background_pattern = _cairo_create_checked_pattern (CHECKED_PATTERN_SIZE);
	self->priv->is_animation = FALSE;
	self->priv->play_animation = TRUE;
	self->priv->cursor_visible = TRUE;

	self->priv->frame_visible = FALSE;
	self->priv->frame_border = 0;

	self->priv->anim_id = 0;
	self->priv->iter = NULL;
	self->priv->iter_surface = NULL;

	self->priv->zoom_enabled = TRUE;
	self->priv->enable_key_bindings = TRUE;
	self->priv->zoom_level = 1.0;
	self->priv->zoom_quality = GTH_ZOOM_QUALITY_HIGH;
	self->priv->zoom_change = GTH_ZOOM_CHANGE_KEEP_PREV;
	self->priv->fit = GTH_FIT_SIZE_IF_LARGER;
	self->priv->doing_zoom_fit = FALSE;

	self->priv->skip_zoom_change = FALSE;
	self->priv->update_image_after_zoom = FALSE;

	self->priv->is_void = TRUE;
	self->visible_area.x = 0;
	self->visible_area.y = 0;
	self->dragging = FALSE;
	self->priv->double_click = FALSE;
	self->priv->just_focused = FALSE;

	self->priv->cursor = NULL;
	self->priv->cursor_void = NULL;

	self->priv->reset_scrollbars = TRUE;
	self->priv->transparency_style = GTH_TRANSPARENCY_STYLE_CHECKERED;

	gth_image_viewer_set_tool (self, NULL);

	/* Create the widget. */

	self->hadj = GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 1.0, 0.0, 0.1, 1.0, 1.0));
	self->vadj = GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 1.0, 0.0, 0.1, 1.0, 1.0));

	g_object_ref (self->hadj);
	g_object_ref_sink (self->hadj);
	g_object_ref (self->vadj);
	g_object_ref_sink (self->vadj);

	g_signal_connect (G_OBJECT (self->hadj),
			  "value_changed",
			  G_CALLBACK (hadj_value_changed),
			  self);
	g_signal_connect (G_OBJECT (self->vadj),
			  "value_changed",
			  G_CALLBACK (vadj_value_changed),
			  self);

	/* do not use the rgba visual on the drawing area */
	{
		GdkVisual *visual;
		visual = gdk_screen_get_system_visual (gtk_widget_get_screen (GTK_WIDGET (self)));
		if (visual != NULL)
			gtk_widget_set_visual (GTK_WIDGET (self), visual);
	}
}


GtkWidget *
gth_image_viewer_new (void)
{
	return GTK_WIDGET (g_object_new (GTH_TYPE_IMAGE_VIEWER, NULL));
}


static void
_gth_image_viewer_set_original_size (GthImageViewer *self,
				     int             original_width,
				     int             original_height)
{
	cairo_surface_t *image;
	int              image_width;
	int              image_height;

	image = gth_image_viewer_get_current_image (self);
	if (image == NULL)
		return;
	image_width = cairo_image_surface_get_width (image);
	image_height = cairo_image_surface_get_height (image);

	if (original_width <= 0 || original_height <= 0)
		_cairo_image_surface_get_original_size (image, &original_width, &original_height);

	if (original_width > 0)
		self->priv->original_width = original_width;
	else
		self->priv->original_width = (image != NULL) ? image_width : 0;

	if (original_height > 0)
		self->priv->original_height = original_height;
	else
		self->priv->original_height = (image != NULL) ? image_height : 0;

	if ((self->priv->original_width > image_width)
	    && (self->priv->original_height > image_height))
	{
		self->priv->requested_size = MAX (image_width, image_height);
	}
	else
		self->priv->requested_size = -1;
}


static void
_gth_image_viewer_content_changed (GthImageViewer *self,
				   gboolean        better_quality)
{
	halt_animation (self);

	if (better_quality)
		g_signal_emit (G_OBJECT (self), gth_image_viewer_signals[BETTER_QUALITY], 0);
	else if (! self->priv->zoom_enabled || (self->priv->zoom_change == GTH_ZOOM_CHANGE_KEEP_PREV))
		g_signal_emit (G_OBJECT (self), gth_image_viewer_signals[IMAGE_CHANGED], 0);

	if (! better_quality && self->priv->reset_scrollbars) {
		self->visible_area.x = 0;
		self->visible_area.y = 0;
	}

	if (better_quality || ! self->priv->zoom_enabled) {
		gth_image_viewer_tool_image_changed (self->priv->tool);
		return;
	}

	self->priv->update_image_after_zoom = TRUE;

	switch (self->priv->zoom_change) {
	case GTH_ZOOM_CHANGE_ACTUAL_SIZE:
		gth_image_viewer_set_zoom (self, 1.0);
		queue_animation_frame_change (self);
		break;

	case GTH_ZOOM_CHANGE_KEEP_PREV:
		gth_image_viewer_tool_image_changed (self->priv->tool);
		gtk_widget_queue_resize (GTK_WIDGET (self));
		break;

	case GTH_ZOOM_CHANGE_FIT_SIZE:
		gth_image_viewer_set_fit_mode (self, GTH_FIT_SIZE);
		queue_animation_frame_change (self);
		break;

	case GTH_ZOOM_CHANGE_FIT_SIZE_IF_LARGER:
		gth_image_viewer_set_fit_mode (self, GTH_FIT_SIZE_IF_LARGER);
		queue_animation_frame_change (self);
		break;

	case GTH_ZOOM_CHANGE_FIT_WIDTH:
		gth_image_viewer_set_fit_mode (self, GTH_FIT_WIDTH);
		queue_animation_frame_change (self);
		break;

	case GTH_ZOOM_CHANGE_FIT_WIDTH_IF_LARGER:
		gth_image_viewer_set_fit_mode (self, GTH_FIT_WIDTH_IF_LARGER);
		queue_animation_frame_change (self);
		break;

	case GTH_ZOOM_CHANGE_FIT_HEIGHT:
		gth_image_viewer_set_fit_mode (self, GTH_FIT_HEIGHT);
		queue_animation_frame_change (self);
		break;

	case GTH_ZOOM_CHANGE_FIT_HEIGHT_IF_LARGER:
		gth_image_viewer_set_fit_mode (self, GTH_FIT_HEIGHT_IF_LARGER);
		queue_animation_frame_change (self);
		break;
	}
}


static void
_set_animation (GthImageViewer     *self,
		GdkPixbufAnimation *animation,
		int                 original_width,
		int                 original_height,
		gboolean            better_quality)
{
	g_return_if_fail (self != NULL);

	_cairo_clear_surface (&self->priv->surface);
	_cairo_clear_surface (&self->priv->iter_surface);
	_g_clear_object (&self->priv->animation);
	_g_clear_object (&self->priv->iter);
	_g_clear_object (&self->priv->image);

	self->priv->animation = _g_object_ref (animation);
	self->priv->is_void = (self->priv->animation == NULL);
	self->priv->is_animation = (self->priv->animation != NULL) ? ! gdk_pixbuf_animation_is_static_image (self->priv->animation) : FALSE;
	if (self->priv->animation != NULL) {
		g_get_current_time (&self->priv->time);
		_g_object_unref (self->priv->iter);
		self->priv->iter = gdk_pixbuf_animation_get_iter (self->priv->animation, &self->priv->time);
	}
	_gth_image_viewer_set_original_size (self, original_width, original_height);

	_gth_image_viewer_content_changed (self, better_quality);
}


void
gth_image_viewer_set_animation (GthImageViewer     *self,
				GdkPixbufAnimation *animation,
				int                 original_width,
				int                 original_height)
{
	_set_animation (self, animation, original_width, original_height, FALSE);
}


static void
_set_surface (GthImageViewer  *self,
	      cairo_surface_t *surface,
	      int              original_width,
	      int              original_height,
	      gboolean         better_quality)
{
	if (self->priv->surface != surface) {
		_cairo_clear_surface (&self->priv->surface);
		self->priv->surface = cairo_surface_reference (surface);
	}

	_cairo_clear_surface (&self->priv->iter_surface);
	_g_clear_object (&self->priv->animation);
	_g_clear_object (&self->priv->iter);

	self->priv->is_void = (self->priv->surface == NULL);
	self->priv->is_animation = FALSE;
	_gth_image_viewer_set_original_size (self, original_width, original_height);

	_gth_image_viewer_content_changed (self, better_quality);
}


void
gth_image_viewer_set_better_quality (GthImageViewer *self,
				     GthImage       *image,
				     int             original_width,
				     int             original_height)
{
	if (gth_image_get_is_animation (image)) {
		GdkPixbufAnimation *animation;

		animation = gth_image_get_pixbuf_animation (image);
		_set_animation (self, animation, original_width, original_height, TRUE);

		g_object_unref (animation);
	}
	else {
		cairo_surface_t *surface;

		surface = gth_image_get_cairo_surface (image);
		_set_surface (self, surface, original_width, original_height, TRUE);

		cairo_surface_destroy (surface);
	}
}


void
gth_image_viewer_set_pixbuf (GthImageViewer *self,
			     GdkPixbuf      *pixbuf,
			     int             original_width,
			     int             original_height)
{
	cairo_surface_t *image;

	g_return_if_fail (self != NULL);

	image = _cairo_image_surface_create_from_pixbuf (pixbuf);
	gth_image_viewer_set_surface (self, image, original_width, original_height);

	cairo_surface_destroy (image);
}


void
gth_image_viewer_set_surface (GthImageViewer  *self,
			      cairo_surface_t *surface,
			      int              original_width,
			      int              original_height)
{
	_g_clear_object (&self->priv->image);
	_set_surface (self, surface, original_width, original_height, FALSE);
}


void
gth_image_viewer_set_image (GthImageViewer *self,
			    GthImage       *image,
			    int             original_width,
			    int             original_height)
{
	if (self->priv->image != image)
		_g_clear_object (&self->priv->image);

	self->priv->image = g_object_ref (image);

	if (gth_image_get_is_animation (image)) {
		GdkPixbufAnimation *animation;

		animation = gth_image_get_pixbuf_animation (image);
		gth_image_viewer_set_animation (self,	animation, original_width, original_height);

		g_object_unref (animation);
	}
	else {
		cairo_surface_t *surface;

		if (gth_image_get_is_zoomable (image)) {
			gth_image_set_zoom (self->priv->image,
					    self->priv->zoom_level,
					    &original_width,
					    &original_height);
		}
		surface = gth_image_get_cairo_surface (image);
		_set_surface (self, surface, original_width, original_height, FALSE);

		cairo_surface_destroy (surface);
	}
}


void
gth_image_viewer_set_void (GthImageViewer *self)
{
	g_return_if_fail (self != NULL);

	_cairo_clear_surface (&self->priv->surface);
	_cairo_clear_surface (&self->priv->iter_surface);
	_g_clear_object (&self->priv->animation);
	_g_clear_object (&self->priv->iter);
	_g_clear_object (&self->priv->image);

	self->priv->is_void = TRUE;
	self->priv->is_animation = FALSE;

	_gth_image_viewer_content_changed (self, FALSE);
}


gboolean
gth_image_viewer_is_void (GthImageViewer *self)
{
	return self->priv->is_void;
}


void
gth_image_viewer_add_painter (GthImageViewer          *self,
			      GthImageViewerPaintFunc  func,
			      gpointer                 user_data)
{
	GList       *link;
	GList       *scan;
	PainterData *painter_data;

	g_return_if_fail (self != NULL);

	link = NULL;
	for (scan = self->priv->painters; scan; scan = scan->next) {
		PainterData *painter_data = scan->data;
		if ((painter_data->func == func) && (painter_data->user_data == user_data)) {
			link = scan;
			break;
		}
	}

	if (link != NULL)
		return;

	painter_data = g_new0 (PainterData, 1);
	painter_data->func = func;
	painter_data->user_data = user_data;
	self->priv->painters = g_list_append (self->priv->painters, painter_data);
}


void
gth_image_viewer_remove_painter (GthImageViewer          *self,
				 GthImageViewerPaintFunc  func,
				 gpointer                 user_data)
{
	GList *link;
	GList *scan;

	link = NULL;
	for (scan = self->priv->painters; scan; scan = scan->next) {
		PainterData *painter_data = scan->data;
		if ((painter_data->func == func) && (painter_data->user_data == user_data)) {
			link = scan;
			break;
		}
	}

	if (link == NULL)
		return;

	self->priv->painters = g_list_remove_link (self->priv->painters, link);
	painter_data_free (link->data);
	g_list_free (link);
}


int
gth_image_viewer_get_image_width (GthImageViewer *self)
{
	cairo_surface_t *image;

	g_return_val_if_fail (self != NULL, 0);

	image = gth_image_viewer_get_current_image (self);
	if (image != NULL)
		return cairo_image_surface_get_width (image);

	return 0;
}


int
gth_image_viewer_get_image_height (GthImageViewer *self)
{
	cairo_surface_t *image;

	g_return_val_if_fail (self != NULL, 0);

	image = gth_image_viewer_get_current_image (self);
	if (image != NULL)
		return cairo_image_surface_get_height (image);

	return 0;
}


void
gth_image_viewer_get_original_size (GthImageViewer *self,
				    int            *width,
				    int            *height)
{
	g_return_if_fail (self != NULL);

	if (width != NULL)
		*width = self->priv->original_width;
	if (height != NULL)
		*height = self->priv->original_height;
}


void
gth_image_viewer_set_requested_size (GthImageViewer *self,
				     int             requested_size)
{
	self->priv->requested_size = requested_size;
}


int
gth_image_viewer_get_requested_size (GthImageViewer *self)
{
	return self->priv->requested_size;
}


gboolean
gth_image_viewer_get_has_alpha (GthImageViewer *self)
{
	return _cairo_image_surface_get_has_alpha (self->priv->surface);
}


GthImage *
gth_image_viewer_get_image (GthImageViewer *self)
{
	return self->priv->image;
}


GdkPixbuf *
gth_image_viewer_get_current_pixbuf (GthImageViewer *self)
{
	g_return_val_if_fail (self != NULL, NULL);

	if (self->priv->is_void)
		return NULL;

	if (self->priv->surface != NULL)
		return _gdk_pixbuf_new_from_cairo_surface (self->priv->surface);

	if (self->priv->iter != NULL)
		return gdk_pixbuf_copy (gdk_pixbuf_animation_iter_get_pixbuf (self->priv->iter));

	return NULL;
}


cairo_surface_t *
gth_image_viewer_get_current_image (GthImageViewer *self)
{
	g_return_val_if_fail (self != NULL, NULL);

	if (self->priv->is_void)
		return NULL;

	if (self->priv->surface != NULL)
		return self->priv->surface;

	if (self->priv->iter != NULL) {
		if (self->priv->iter_surface == NULL)
			self->priv->iter_surface = _cairo_image_surface_create_from_pixbuf (gdk_pixbuf_animation_iter_get_pixbuf (self->priv->iter));
		return self->priv->iter_surface;
	}

	return NULL;
}


void
gth_image_viewer_start_animation (GthImageViewer *self)
{
	g_return_if_fail (self != NULL);

	self->priv->play_animation = TRUE;
	gtk_widget_queue_resize (GTK_WIDGET (self));
}


void
gth_image_viewer_stop_animation (GthImageViewer *self)
{
	g_return_if_fail (self != NULL);

	self->priv->play_animation = FALSE;
	halt_animation (self);
}


void
gth_image_viewer_step_animation (GthImageViewer *self)
{
	g_return_if_fail (self != NULL);

	if (! self->priv->is_animation)
		return;
	if (self->priv->play_animation)
		return;

	change_animation_frame (self);
}


gboolean
gth_image_viewer_is_animation (GthImageViewer *self)
{
	g_return_val_if_fail (self != NULL, FALSE);

	return self->priv->is_animation;
}


gboolean
gth_image_viewer_is_playing_animation (GthImageViewer *self)
{
	g_return_val_if_fail (self != NULL, FALSE);

	return self->priv->is_animation && self->priv->play_animation;
}


void
gth_image_viewer_set_zoom (GthImageViewer *self,
			   gdouble         zoom_level)
{
	if (! self->priv->zoom_enabled)
		return;

	set_zoom_centered (self, zoom_level, FALSE);
	gtk_widget_queue_resize (GTK_WIDGET (self));
}


gdouble
gth_image_viewer_get_zoom (GthImageViewer *self)
{
	return self->priv->zoom_level;
}


void
gth_image_viewer_set_zoom_quality (GthImageViewer *self,
				   GthZoomQuality  quality)
{
	self->priv->zoom_quality = quality;

	/* to update the zoom filter */

	if (self->priv->tool != NULL)
		gth_image_viewer_tool_zoom_changed (self->priv->tool);
}


GthZoomQuality
gth_image_viewer_get_zoom_quality (GthImageViewer *self)
{
	return self->priv->zoom_quality;
}


cairo_filter_t
gth_image_viewer_get_zoom_quality_filter (GthImageViewer *viewer)
{
	cairo_filter_t filter;

	if (gth_image_viewer_get_zoom_quality (viewer) == GTH_ZOOM_QUALITY_LOW)
		filter = CAIRO_FILTER_FAST;
	else
		filter = CAIRO_FILTER_BEST;

	if (viewer->priv->zoom_level >= 1.0)
		filter = CAIRO_FILTER_FAST;

	return filter;
}


void
gth_image_viewer_set_zoom_change (GthImageViewer *self,
				  GthZoomChange   zoom_change)
{
	self->priv->zoom_change = zoom_change;
}


GthZoomChange
gth_image_viewer_get_zoom_change (GthImageViewer *self)
{
	return self->priv->zoom_change;
}


void
gth_image_viewer_zoom_in (GthImageViewer *self)
{
	if (! self->priv->is_void)
		gth_image_viewer_set_zoom (self, get_next_zoom (self->priv->zoom_level));
}


void
gth_image_viewer_zoom_out (GthImageViewer *self)
{
	if (! self->priv->is_void)
		gth_image_viewer_set_zoom (self, get_prev_zoom (self->priv->zoom_level));
}


gboolean
gth_image_viewer_zoom_from_scroll (GthImageViewer *self,
				   GdkEventScroll *event)
{
	gboolean handled;
	double   new_zoom_level;

	handled = FALSE;
	switch (event->direction) {
	case GDK_SCROLL_UP:
	case GDK_SCROLL_DOWN:
		if (event->direction == GDK_SCROLL_UP)
			new_zoom_level = get_next_zoom (self->priv->zoom_level);
		else
			new_zoom_level = get_prev_zoom (self->priv->zoom_level);
		set_zoom_centered_at (self, new_zoom_level, FALSE, (int) event->x, (int) event->y);
		gtk_widget_queue_resize (GTK_WIDGET (self));
		handled = TRUE;
		break;

	default:
		break;
	}

	return handled;
}


void
gth_image_viewer_set_fit_mode (GthImageViewer *self,
			       GthFit          fit_mode)
{
	if (! self->priv->zoom_enabled)
		return;

	self->priv->fit = fit_mode;
	if (! self->priv->is_void)
		gtk_widget_queue_resize (GTK_WIDGET (self));
}


GthFit
gth_image_viewer_get_fit_mode (GthImageViewer *self)
{
	return self->priv->fit;
}


void
gth_image_viewer_set_zoom_enabled (GthImageViewer *self,
				   gboolean        value)
{
	g_return_if_fail (GTH_IS_IMAGE_VIEWER (self));

	self->priv->zoom_enabled = value;
}


gboolean
gth_image_viewer_get_zoom_enabled (GthImageViewer *self)
{
	return self->priv->zoom_enabled;
}


void
gth_image_viewer_clicked (GthImageViewer *self)
{
	g_signal_emit (G_OBJECT (self), gth_image_viewer_signals[CLICKED], 0);
}


void
gth_image_viewer_set_tool (GthImageViewer     *self,
			   GthImageViewerTool *tool)
{
	if (self->priv->tool != NULL) {
		gth_image_viewer_tool_unset_viewer (self->priv->tool, self);
		g_object_unref (self->priv->tool);
	}
	if (tool == NULL)
		self->priv->tool = gth_image_dragger_new (FALSE);
	else
		self->priv->tool = g_object_ref (tool);
	gth_image_viewer_tool_set_viewer (self->priv->tool, self);
	if (gtk_widget_get_realized (GTK_WIDGET (self)))
		gth_image_viewer_tool_realize (self->priv->tool);
	gth_image_viewer_tool_image_changed (self->priv->tool);
	gtk_widget_queue_resize (GTK_WIDGET (self));
}


GthImageViewerTool *
gth_image_viewer_get_tool (GthImageViewer *self)
{
	return self->priv->tool;
}


void
gth_image_viewer_set_transparency_style (GthImageViewer       *self,
					 GthTransparencyStyle  style)
{
	g_return_if_fail (GTH_IS_IMAGE_VIEWER (self));

	self->priv->transparency_style = style;
	gtk_widget_queue_draw (GTK_WIDGET (self));
}


GthTransparencyStyle
gth_image_viewer_get_transparency_style (GthImageViewer *self)
{
	g_return_val_if_fail (GTH_IS_IMAGE_VIEWER (self), 0);

	return self->priv->transparency_style;
}


void
gth_image_viewer_enable_key_bindings (GthImageViewer *self,
				      gboolean        value)
{
	g_return_if_fail (GTH_IS_IMAGE_VIEWER (self));

	self->priv->enable_key_bindings = value;
}


void
gth_image_viewer_scroll_to (GthImageViewer *self,
			    int             x_offset,
			    int             y_offset)
{
	g_return_if_fail (self != NULL);

	if (gth_image_viewer_get_current_image (self) == NULL)
		return;

	scroll_to (self, x_offset, y_offset);

	/* update the adjustments value */

	_g_signal_handlers_block_by_data (G_OBJECT (self->hadj), self);
	_g_signal_handlers_block_by_data (G_OBJECT (self->vadj), self);
	gtk_adjustment_set_value (self->hadj, self->visible_area.x);
	gtk_adjustment_set_value (self->vadj, self->visible_area.y);
	_g_signal_handlers_unblock_by_data (G_OBJECT (self->hadj), self);
	_g_signal_handlers_unblock_by_data (G_OBJECT (self->vadj), self);
}


void
gth_image_viewer_scroll_to_center (GthImageViewer *self)
{
	int zoomed_width;
	int zoomed_height;

	_gth_image_viewer_get_zoomed_size (self, &zoomed_width, &zoomed_height);

	gth_image_viewer_scroll_to (self,
				    (zoomed_width - self->visible_area.width) / 2,
				    (zoomed_height - self->visible_area.height) / 2);
}


void
gth_image_viewer_scroll_step_x (GthImageViewer *self,
				gboolean        increment)
{
	scroll_relative (self,
			 (increment ? 1 : -1) * gtk_adjustment_get_step_increment (self->hadj),
			 0);
}


void
gth_image_viewer_scroll_step_y (GthImageViewer *self,
				gboolean        increment)
{
	scroll_relative (self,
			 0,
			 (increment ? 1 : -1) * gtk_adjustment_get_step_increment (self->vadj));
}


void
gth_image_viewer_scroll_page_x (GthImageViewer *self,
				gboolean        increment)
{
	scroll_relative (self,
			 (increment ? 1 : -1) * gtk_adjustment_get_page_increment (self->hadj),
			 0);
}


void
gth_image_viewer_scroll_page_y (GthImageViewer *self,
				gboolean        increment)
{
	scroll_relative (self,
			 0,
			 (increment ? 1 : -1) * gtk_adjustment_get_page_increment (self->vadj));
}


void
gth_image_viewer_set_scroll_offset (GthImageViewer *self,
				    int             x,
				    int             y)
{
	double quality_zoom;

	quality_zoom = _gth_image_viewer_get_quality_zoom (self);
	gth_image_viewer_scroll_to (self, x / quality_zoom, y / quality_zoom);
}


void
gth_image_viewer_get_scroll_offset (GthImageViewer *self,
				    int            *x,
				    int            *y)
{
	double quality_zoom;

	quality_zoom = _gth_image_viewer_get_quality_zoom (self);
	*x = self->visible_area.x * quality_zoom;
	*y = self->visible_area.y * quality_zoom;
}


void
gth_image_viewer_set_reset_scrollbars (GthImageViewer *self,
				       gboolean        reset)
{
	self->priv->reset_scrollbars = reset;
}


gboolean
gth_image_viewer_get_reset_scrollbars (GthImageViewer *self)
{
	return self->priv->reset_scrollbars;
}


void
gth_image_viewer_needs_scrollbars (GthImageViewer *self,
				   GtkAllocation  *allocation,
				   GtkWidget      *hscrollbar,
				   GtkWidget      *vscrollbar,
				   gboolean       *hscrollbar_visible_p,
				   gboolean       *vscrollbar_visible_p)
{
	double   zoom_level;
	int      zoomed_width;
	int      zoomed_height;
	int      visible_width;
	int      visible_height;
	gboolean hscrollbar_visible;
	gboolean vscrollbar_visible;

	zoom_level = get_zoom_level_for_allocation (self, allocation);
	_gth_image_viewer_get_zoomed_size_for_zoom (self, &zoomed_width, &zoomed_height, zoom_level);
	visible_width = allocation->width;
	visible_height = allocation->height;

	hscrollbar_visible = (zoomed_width > visible_width);
	vscrollbar_visible = (zoomed_height > visible_height);

	switch (self->priv->fit) {
	case GTH_FIT_SIZE:
	case GTH_FIT_SIZE_IF_LARGER:
		hscrollbar_visible = FALSE;
		vscrollbar_visible = FALSE;
		break;

	case GTH_FIT_WIDTH:
	case GTH_FIT_WIDTH_IF_LARGER:
		hscrollbar_visible = FALSE;
		break;

	case GTH_FIT_HEIGHT:
	case GTH_FIT_HEIGHT_IF_LARGER:
		vscrollbar_visible = FALSE;
		break;

	case GTH_FIT_NONE:
		if (hscrollbar_visible != vscrollbar_visible) {
			if (vscrollbar_visible) {
				GtkRequisition vscrollbar_requisition;

				gtk_widget_get_preferred_size (vscrollbar, &vscrollbar_requisition, NULL);
				visible_width -= vscrollbar_requisition.width;
				hscrollbar_visible = (zoomed_width > visible_width);
			}
			else if (hscrollbar_visible) {
				GtkRequisition hscrollbar_requisition;

				gtk_widget_get_preferred_size (hscrollbar, &hscrollbar_requisition, NULL);
				visible_height -= hscrollbar_requisition.height;
				vscrollbar_visible = (zoomed_height > visible_height);
			}
		}
		break;
	}

	if (hscrollbar_visible_p != NULL)
		*hscrollbar_visible_p = hscrollbar_visible;
	if (vscrollbar_visible_p != NULL)
		*vscrollbar_visible_p = vscrollbar_visible;
}


gboolean
gth_image_viewer_has_scrollbars (GthImageViewer *self)
{
	int zoomed_width, zoomed_height;
	_gth_image_viewer_get_zoomed_size (self, &zoomed_width, &zoomed_height);
	return (self->visible_area.width < zoomed_width) || (self->visible_area.height < zoomed_height);
}


void
gth_image_viewer_show_cursor (GthImageViewer *self)
{
	if (self->priv->cursor_visible)
		return;

	self->priv->cursor_visible = TRUE;
	if (self->priv->cursor != NULL)
		gdk_window_set_cursor (gtk_widget_get_window (GTK_WIDGET (self)), self->priv->cursor);
}


void
gth_image_viewer_hide_cursor (GthImageViewer *self)
{
	if (! self->priv->cursor_visible)
		return;

	self->priv->cursor_visible = FALSE;
	if (self->priv->cursor_void != NULL)
		gdk_window_set_cursor (gtk_widget_get_window (GTK_WIDGET (self)), self->priv->cursor_void);
}


void
gth_image_viewer_set_cursor (GthImageViewer *self,
			     GdkCursor      *cursor)
{
	if (cursor != NULL)
		g_object_ref (cursor);

	if (self->priv->cursor != NULL) {
		g_object_unref (self->priv->cursor);
		self->priv->cursor = NULL;
	}
	if (cursor != NULL)
		self->priv->cursor = cursor;
	else
		self->priv->cursor = g_object_ref (self->priv->cursor_void);

	if (! gtk_widget_get_realized (GTK_WIDGET (self)))
		return;

	if (self->priv->cursor_visible)
		gdk_window_set_cursor (gtk_widget_get_window (GTK_WIDGET (self)), self->priv->cursor);
}


gboolean
gth_image_viewer_is_cursor_visible (GthImageViewer *self)
{
	return self->priv->cursor_visible;
}


void
gth_image_viewer_show_frame (GthImageViewer *self,
			     int             frame_border)
{
	self->priv->frame_visible = TRUE;
	self->priv->frame_border = frame_border;

	gtk_widget_queue_resize (GTK_WIDGET (self));
}


void
gth_image_viewer_hide_frame (GthImageViewer *self)
{
	self->priv->frame_visible = FALSE;
	self->priv->frame_border = 0;

	gtk_widget_queue_resize (GTK_WIDGET (self));
}


gboolean
gth_image_viewer_is_frame_visible (GthImageViewer *self)
{
	return self->priv->frame_visible;
}


int
gth_image_viewer_get_frame_border (GthImageViewer *self)
{
	return _gth_image_viewer_get_frame_border (self);
}


void
gth_image_viewer_paint (GthImageViewer  *self,
			cairo_t         *cr,
			cairo_surface_t *surface,
			int              src_x,
			int              src_y,
			int              dest_x,
			int              dest_y,
			int              width,
			int              height,
			cairo_filter_t   filter)
{
	int    original_width = 0;
	int    surface_width;
	double zoom_level;
	double src_dx;
	double src_dy;
	double dest_dx;
	double dest_dy;
	double dwidth;
	double dheight;

	surface_width = cairo_image_surface_get_width (surface);
	if (surface_width <= 0)
		return;

	gth_image_viewer_get_original_size (self, &original_width, NULL);
	zoom_level = self->priv->zoom_level * ((double) original_width / surface_width);
	src_dx = (double) src_x / zoom_level;
	src_dy = (double) src_y / zoom_level;
	dest_dx = (double) dest_x / zoom_level;
	dest_dy = (double) dest_y / zoom_level;
	dwidth = (double) width / zoom_level;
	dheight = (double) height / zoom_level;

	if ((dwidth < 1) || (dheight < 1))
		return;

	cairo_save (cr);

	cairo_rectangle (cr, 0, 0, self->visible_area.width, self->visible_area.height);
	cairo_clip (cr);
	cairo_scale (cr, zoom_level, zoom_level);
	cairo_set_source_surface (cr, surface, dest_dx - src_dx, dest_dy - src_dy);
	cairo_pattern_set_filter (cairo_get_source (cr), filter);
	cairo_rectangle (cr, dest_dx, dest_dy, dwidth, dheight);
	cairo_fill (cr);

  	cairo_restore (cr);
}


void
gth_image_viewer_paint_region (GthImageViewer        *self,
			       cairo_t               *cr,
			       cairo_surface_t       *surface,
			       int                    dest_x,
			       int                    dest_y,
			       cairo_rectangle_int_t *paint_area,
			       cairo_filter_t         filter)
{
	int frame_border;

	frame_border = _gth_image_viewer_get_frame_border (self);
	gth_image_viewer_paint (self,
				cr,
				surface,
				paint_area->x - frame_border,
				paint_area->y - frame_border,
				dest_x - frame_border,
				dest_y - frame_border,
				MIN (paint_area->width, self->image_area.width + frame_border),
				MIN (paint_area->height, self->image_area.height + frame_border),
				filter);
}


void
gth_image_viewer_paint_background (GthImageViewer *self,
				   cairo_t        *cr)
{
	GtkAllocation allocation;

	cairo_save (cr);
	gtk_widget_get_allocation (GTK_WIDGET (self), &allocation);
	cairo_set_source_rgb (cr, GRAY_VALUE, GRAY_VALUE, GRAY_VALUE);
	cairo_rectangle (cr,
			 0,
			 0,
			 allocation.width,
			 allocation.height);
	cairo_fill (cr);
	cairo_restore (cr);
}


#define FRAME_SHADOW_OFS 1


void
gth_image_viewer_paint_frame (GthImageViewer *self,
			      cairo_t        *cr)
{
	gboolean background_only;

	background_only = ! gth_image_viewer_is_frame_visible (self);

	cairo_save (cr);

	cairo_translate (cr, -self->visible_area.x, -self->visible_area.y);

	if (! background_only) {
		/* drop shadow */

		cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 0.5);
		cairo_rectangle (cr,
				 self->image_area.x + FRAME_SHADOW_OFS + 0.5,
				 self->image_area.y + FRAME_SHADOW_OFS + 0.5,
				 self->image_area.width + 2 + FRAME_SHADOW_OFS,
				 self->image_area.height + 2 + FRAME_SHADOW_OFS);
		cairo_fill (cr);
	}

	/* background */

	switch (self->priv->transparency_style) {
	case GTH_TRANSPARENCY_STYLE_CHECKERED:
		cairo_set_source (cr, self->priv->background_pattern);
		break;
	case GTH_TRANSPARENCY_STYLE_WHITE:
		cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
		break;
	case GTH_TRANSPARENCY_STYLE_GRAY:
		cairo_set_source_rgb (cr, GRAY_VALUE, GRAY_VALUE, GRAY_VALUE);
		break;
	case GTH_TRANSPARENCY_STYLE_BLACK:
		cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
		break;
	}

	cairo_rectangle (cr,
			 self->image_area.x,
			 self->image_area.y,
			 self->image_area.width,
			 self->image_area.height);
	cairo_fill (cr);

	if (! background_only) {
		/* frame */

		cairo_set_line_width (cr, 2.0);
		cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
		cairo_rectangle (cr,
				 self->image_area.x - 1,
				 self->image_area.y - 1,
				 self->image_area.width + 2,
				 self->image_area.height + 2);
		cairo_stroke (cr);

		cairo_set_line_width (cr, 1.0);
		cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
		cairo_rectangle (cr,
				 self->image_area.x - 2,
				 self->image_area.y - 2,
				 self->image_area.width + 5,
				 self->image_area.height + 5);
		cairo_stroke (cr);
	}

	cairo_restore (cr);
}


void
gth_image_viewer_apply_painters (GthImageViewer *self,
				 cairo_t        *cr)
{
	GList *scan;

	for (scan = self->priv->painters; scan; scan = scan->next) {
		PainterData *painter_data = scan->data;

		cairo_save (cr);
		painter_data->func (self, cr, painter_data->user_data);
		cairo_restore (cr);
	}
}


void
gth_image_viewer_crop_area (GthImageViewer        *self,
			    cairo_rectangle_int_t *area)
{
	area->width = MIN (area->width, self->visible_area.width);
	area->width = MIN (area->height, self->visible_area.height);
}
