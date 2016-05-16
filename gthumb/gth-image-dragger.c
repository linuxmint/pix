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
#include <math.h>
#include "cairo-scale.h"
#include "cairo-utils.h"
#include "glib-utils.h"
#include "gth-image-dragger.h"


struct _GthImageDraggerPrivate {
	GthImageViewer  *viewer;
	gboolean         draggable;
	cairo_surface_t *scaled;
};


static void gth_image_dragger_gth_image_tool_interface_init (GthImageViewerToolInterface *iface);


G_DEFINE_TYPE_WITH_CODE (GthImageDragger,
			 gth_image_dragger,
			 G_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (GTH_TYPE_IMAGE_VIEWER_TOOL,
					        gth_image_dragger_gth_image_tool_interface_init))


static void
gth_image_dragger_finalize (GObject *object)
{
	GthImageDragger *self;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GTH_IS_IMAGE_DRAGGER (object));

	self = GTH_IMAGE_DRAGGER (object);
	_cairo_clear_surface (&self->priv->scaled);

	/* Chain up */
	G_OBJECT_CLASS (gth_image_dragger_parent_class)->finalize (object);
}


static void
gth_image_dragger_class_init (GthImageDraggerClass *class)
{
	GObjectClass *gobject_class;

	g_type_class_add_private (class, sizeof (GthImageDraggerPrivate));

	gobject_class = (GObjectClass*) class;
	gobject_class->finalize = gth_image_dragger_finalize;
}


static void
gth_image_dragger_init (GthImageDragger *dragger)
{
	dragger->priv = G_TYPE_INSTANCE_GET_PRIVATE (dragger, GTH_TYPE_IMAGE_DRAGGER, GthImageDraggerPrivate);
	dragger->priv->scaled = NULL;
}


static void
gth_image_dragger_set_viewer (GthImageViewerTool *base,
			      GthImageViewer     *image_viewer)
{
	GthImageDragger *self = GTH_IMAGE_DRAGGER (base);
	self->priv->viewer = image_viewer;
}


static void
gth_image_dragger_unset_viewer (GthImageViewerTool *base,
		       	        GthImageViewer     *image_viewer)
{
	GthImageDragger *self = GTH_IMAGE_DRAGGER (base);
	self->priv->viewer = NULL;
}


static void
gth_image_dragger_realize (GthImageViewerTool *base)
{
	/* void */
}


static void
gth_image_dragger_unrealize (GthImageViewerTool *base)
{
	/* void */
}


static void
_gth_image_dragger_update_cursor (GthImageDragger *self)
{
	GdkCursor *cursor;

	cursor = gdk_cursor_new (GDK_LEFT_PTR);
	gth_image_viewer_set_cursor (self->priv->viewer, cursor);

	g_object_unref (cursor);
}


static void
gth_image_dragger_size_allocate (GthImageViewerTool *base,
				 GtkAllocation      *allocation)
{
	GthImageDragger *self;
	GthImageViewer  *viewer;
	double           h_page_size;
	double           v_page_size;
	double           h_upper;
	double           v_upper;

	self = (GthImageDragger *) base;
	viewer = (GthImageViewer *) self->priv->viewer;

	h_page_size = gtk_adjustment_get_page_size (viewer->hadj);
	v_page_size = gtk_adjustment_get_page_size (viewer->vadj);
	h_upper = gtk_adjustment_get_upper (viewer->hadj);
	v_upper = gtk_adjustment_get_upper (viewer->vadj);

	self->priv->draggable = (h_page_size > 0) && (v_page_size > 0) && ((h_upper > h_page_size) || (v_upper > v_page_size));
	if (gtk_widget_get_realized (GTK_WIDGET (viewer)))
		_gth_image_dragger_update_cursor (self);
}


static void
gth_image_dragger_map (GthImageViewerTool *base)
{
	/* void */
}


static void
gth_image_dragger_unmap (GthImageViewerTool *base)
{
	/* void */
}


static void
gth_image_dragger_draw (GthImageViewerTool *self,
		        cairo_t            *cr)
{
	GthImageDragger *dragger;
	GthImageViewer  *viewer;

	dragger = (GthImageDragger *) self;
	viewer = dragger->priv->viewer;

	gth_image_viewer_paint_background (viewer, cr);

	if (gth_image_viewer_get_current_image (viewer) == NULL)
		return;

	if (dragger->priv->scaled != NULL)
		gth_image_viewer_paint_region (viewer,
					       cr,
					       dragger->priv->scaled,
					       viewer->x_offset - viewer->image_area.x,
					       viewer->y_offset - viewer->image_area.y,
					       &viewer->image_area,
					       NULL,
					       CAIRO_FILTER_FAST);
	else
		gth_image_viewer_paint_region (viewer,
					       cr,
					       gth_image_viewer_get_current_image (viewer),
					       viewer->x_offset - viewer->image_area.x,
					       viewer->y_offset - viewer->image_area.y,
					       &viewer->image_area,
					       NULL,
					       gth_image_viewer_get_zoom_quality_filter (viewer));

	gth_image_viewer_apply_painters (viewer, cr);
}


static gboolean
gth_image_dragger_button_press (GthImageViewerTool *self,
				GdkEventButton     *event)
{
	GthImageDragger *dragger;
	GthImageViewer  *viewer;
	GtkWidget       *widget;

	dragger = (GthImageDragger *) self;
	viewer = dragger->priv->viewer;
	widget = GTK_WIDGET (viewer);

	if (! dragger->priv->draggable)
		return FALSE;

	if (((event->button == 1) || (event->button == 2)) &&
			! viewer->dragging) {
		GdkCursor     *cursor;
		GdkGrabStatus  retval;

		cursor = gdk_cursor_new_from_name (gtk_widget_get_display (widget), "grabbing");
		retval = gdk_device_grab (event->device,
					  gtk_widget_get_window (widget),
					  GDK_OWNERSHIP_WINDOW,
					  FALSE,
					  (GDK_POINTER_MOTION_MASK
					   | GDK_POINTER_MOTION_HINT_MASK
					   | GDK_BUTTON_RELEASE_MASK),
					  cursor,
					  event->time);

		if (cursor != NULL)
			g_object_unref (cursor);

		if (retval != GDK_GRAB_SUCCESS)
			return FALSE;

		viewer->pressed = TRUE;
		viewer->dragging = TRUE;

		return TRUE;
	}

	return FALSE;
}


static gboolean
gth_image_dragger_button_release (GthImageViewerTool *self,
				  GdkEventButton     *event)
{
	GthImageDragger *dragger;
	GthImageViewer  *viewer;

	if ((event->button != 1) && (event->button != 2))
		return FALSE;

	dragger = (GthImageDragger *) self;
	viewer = dragger->priv->viewer;

	if (viewer->dragging)
		gdk_device_ungrab (event->device, event->time);

	return TRUE;
}


static gboolean
gth_image_dragger_motion_notify (GthImageViewerTool *self,
				 GdkEventMotion     *event)
{
	GthImageDragger *dragger;
	GthImageViewer  *viewer;

	dragger = (GthImageDragger *) self;
	viewer = dragger->priv->viewer;

	if (! viewer->pressed)
		return FALSE;

	viewer->dragging = TRUE;

	if (! event->is_hint)
		return FALSE;

	gth_image_viewer_scroll_to (viewer,
				    viewer->drag_x_start - event->x,
				    viewer->drag_y_start - event->y);

	return TRUE;
}


static void
_gth_image_dragger_update_scaled_image (GthImageDragger *self)
{
	cairo_surface_t *image;
	int              image_width, image_height;
	int              original_width, original_height;
	double           zoom;

	_cairo_clear_surface (&self->priv->scaled);
	self->priv->scaled = NULL;

	if (gth_image_viewer_is_animation (self->priv->viewer))
		return;

	image = gth_image_viewer_get_current_image (self->priv->viewer);
	if (image == NULL)
		return;

	image_width = cairo_image_surface_get_width (image);
	image_height = cairo_image_surface_get_height (image);
	gth_image_viewer_get_original_size (self->priv->viewer, &original_width, &original_height);

	if ((original_width != image_width) || (original_height != image_height))
		return;

	if (gth_image_viewer_get_zoom_quality (self->priv->viewer) == GTH_ZOOM_QUALITY_LOW)
		return;

	zoom = gth_image_viewer_get_zoom (self->priv->viewer);
	if (zoom >= 1.0 / _CAIRO_MAX_SCALE_FACTOR)
		return;

	self->priv->scaled = _cairo_image_surface_scale_bilinear (image,
								  zoom * image_width,
								  zoom * image_height);
}


static void
gth_image_dragger_image_changed (GthImageViewerTool *self)
{
	_gth_image_dragger_update_scaled_image (GTH_IMAGE_DRAGGER (self));
}


static void
gth_image_dragger_zoom_changed (GthImageViewerTool *self)
{
	_gth_image_dragger_update_scaled_image (GTH_IMAGE_DRAGGER (self));
}


static void
gth_image_dragger_gth_image_tool_interface_init (GthImageViewerToolInterface *iface)
{
	iface->set_viewer = gth_image_dragger_set_viewer;
	iface->unset_viewer = gth_image_dragger_unset_viewer;
	iface->realize = gth_image_dragger_realize;
	iface->unrealize = gth_image_dragger_unrealize;
	iface->size_allocate = gth_image_dragger_size_allocate;
	iface->map = gth_image_dragger_map;
	iface->unmap = gth_image_dragger_unmap;
	iface->draw = gth_image_dragger_draw;
	iface->button_press = gth_image_dragger_button_press;
	iface->button_release = gth_image_dragger_button_release;
	iface->motion_notify = gth_image_dragger_motion_notify;
	iface->image_changed = gth_image_dragger_image_changed;
	iface->zoom_changed = gth_image_dragger_zoom_changed;
}


GthImageViewerTool *
gth_image_dragger_new (void)
{
	return (GthImageViewerTool *) g_object_new (GTH_TYPE_IMAGE_DRAGGER, NULL);
}
