/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2012 Free Software Foundation, Inc.
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
#include "gth-preview-tool.h"


static void gth_preview_tool_gth_image_tool_interface_init (GthImageViewerToolInterface *iface);


G_DEFINE_TYPE_WITH_CODE (GthPreviewTool,
			 gth_preview_tool,
			 G_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (GTH_TYPE_IMAGE_VIEWER_TOOL,
					        gth_preview_tool_gth_image_tool_interface_init))



struct _GthPreviewToolPrivate {
	GthImageViewer        *viewer;
	GthFit                 original_fit_mode;
	gboolean               original_zoom_enabled;
	cairo_surface_t       *preview_image;
	cairo_rectangle_int_t  preview_image_area;
	GdkRGBA                background_color;
};


static void
gth_preview_tool_set_viewer (GthImageViewerTool *base,
			     GthImageViewer     *viewer)
{
	GthPreviewTool *self = GTH_PREVIEW_TOOL (base);
	GdkCursor      *cursor;

	self->priv->viewer = viewer;
	self->priv->original_fit_mode = gth_image_viewer_get_fit_mode (GTH_IMAGE_VIEWER (viewer));
	self->priv->original_zoom_enabled = gth_image_viewer_get_zoom_enabled (GTH_IMAGE_VIEWER (viewer));
	gth_image_viewer_set_fit_mode (GTH_IMAGE_VIEWER (viewer), GTH_FIT_SIZE_IF_LARGER);
	gth_image_viewer_set_zoom_enabled (GTH_IMAGE_VIEWER (viewer), FALSE);

	cursor = gdk_cursor_new (GDK_LEFT_PTR);
	gth_image_viewer_set_cursor (self->priv->viewer, cursor);

	g_object_unref (cursor);
}


static void
gth_preview_tool_unset_viewer (GthImageViewerTool *base,
			       GthImageViewer     *viewer)
{
	GthPreviewTool *self = GTH_PREVIEW_TOOL (base);

	gth_image_viewer_set_fit_mode (GTH_IMAGE_VIEWER (viewer), self->priv->original_fit_mode);
	gth_image_viewer_set_zoom_enabled (GTH_IMAGE_VIEWER (viewer), self->priv->original_zoom_enabled);
	self->priv->viewer = NULL;
}


static void
gth_preview_tool_realize (GthImageViewerTool *base)
{
	/* void */
}


static void
gth_preview_tool_unrealize (GthImageViewerTool *base)
{
	/* void */
}


#if 0


static void
update_preview_image (GthPreviewTool *self)
{
	cairo_surface_t *image;
	int              width;
	int              height;
	GtkAllocation    allocation;
	int              max_size;

	if (self->priv->preview_image != NULL) {
		cairo_surface_destroy (self->priv->preview_image);
		self->priv->preview_image = NULL;
	}

	image = gth_image_viewer_get_current_image (GTH_IMAGE_VIEWER (self->priv->viewer));
	if (image == NULL)
		return;

	width = cairo_image_surface_get_width (image);
	height = cairo_image_surface_get_height (image);
	gtk_widget_get_allocation (GTK_WIDGET (self->priv->viewer), &allocation);
	max_size = MAX (allocation.width, allocation.height) / G_SQRT2 + 2;
	if (scale_keeping_ratio (&width, &height, max_size, max_size, FALSE))
		self->priv->preview_image = _cairo_image_surface_scale_bilinear (image, width, height);
	else
		self->priv->preview_image = cairo_surface_reference (image);

	self->priv->preview_image_area.width = width;
	self->priv->preview_image_area.height = height;
	self->priv->preview_image_area.x = MAX ((allocation.width - self->priv->preview_image_area.width) / 2 - 0.5, 0);
	self->priv->preview_image_area.y = MAX ((allocation.height - self->priv->preview_image_area.height) / 2 - 0.5, 0);
}


#endif


static void
update_preview_image_area (GthPreviewTool *self)
{
	int           width;
	int           height;
	GtkAllocation allocation;

	if ((self->priv->preview_image == NULL) || (self->priv->viewer == NULL))
		return;

	if (! gtk_widget_get_realized (GTK_WIDGET (self->priv->viewer)))
		return;

	width = cairo_image_surface_get_width (self->priv->preview_image);
	height = cairo_image_surface_get_height (self->priv->preview_image);
	gtk_widget_get_allocation (GTK_WIDGET (self->priv->viewer), &allocation);

	self->priv->preview_image_area.width = width;
	self->priv->preview_image_area.height = height;
	self->priv->preview_image_area.x = MAX ((allocation.width - self->priv->preview_image_area.width) / 2 - 0.5, 0);
	self->priv->preview_image_area.y = MAX ((allocation.height - self->priv->preview_image_area.height) / 2 - 0.5, 0);
}


static void
gth_preview_tool_size_allocate (GthImageViewerTool *base,
				GtkAllocation      *allocation)
{
	update_preview_image_area (GTH_PREVIEW_TOOL (base));
}


static void
gth_preview_tool_map (GthImageViewerTool *base)
{
	/* void */
}


static void
gth_preview_tool_unmap (GthImageViewerTool *base)
{
	/* void */
}


static void
gth_preview_tool_draw (GthImageViewerTool *base,
		       cairo_t            *cr)
{
	GthPreviewTool *self = GTH_PREVIEW_TOOL (base);
	GtkAllocation   allocation;

  	/* background */

	cairo_save (cr);
	gtk_widget_get_allocation (GTK_WIDGET (self->priv->viewer), &allocation);
	cairo_rectangle (cr, 0, 0, allocation.width, allocation.height);

  	cairo_set_source_rgba (cr,
  			       self->priv->background_color.red,
  			       self->priv->background_color.green,
  			       self->priv->background_color.blue,
  			       self->priv->background_color.alpha);
	cairo_fill (cr);
	cairo_restore (cr);

	/* image */

	cairo_save (cr);

	if (self->priv->preview_image != NULL) {
		cairo_set_antialias (cr, CAIRO_ANTIALIAS_NONE);
		cairo_set_source_surface (cr, self->priv->preview_image,
					  self->priv->preview_image_area.x,
					  self->priv->preview_image_area.y);
		cairo_pattern_set_filter (cairo_get_source (cr), CAIRO_FILTER_FAST);
		cairo_rectangle (cr,
				 self->priv->preview_image_area.x,
				 self->priv->preview_image_area.y,
				 self->priv->preview_image_area.width,
				 self->priv->preview_image_area.height);
		cairo_fill (cr);
	}

  	cairo_restore (cr);
}


static gboolean
gth_preview_tool_button_release (GthImageViewerTool *base,
				 GdkEventButton     *event)
{
	/* void */

	return FALSE;
}


static gboolean
gth_preview_tool_button_press (GthImageViewerTool *base,
			       GdkEventButton     *event)
{
	/* void */

	return FALSE;
}


static gboolean
gth_preview_tool_motion_notify (GthImageViewerTool *base,
				GdkEventMotion     *event)
{
	/* void */

	return FALSE;
}


static void
gth_preview_tool_image_changed (GthImageViewerTool *base)
{
	/* void */
}


static void
gth_preview_tool_zoom_changed (GthImageViewerTool *base)
{
	/* void */
}


static void
gth_preview_tool_finalize (GObject *object)
{
	GthPreviewTool *self;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GTH_IS_PREVIEW_TOOL (object));

	self = (GthPreviewTool *) object;
	cairo_surface_destroy (self->priv->preview_image);

	/* Chain up */
	G_OBJECT_CLASS (gth_preview_tool_parent_class)->finalize (object);
}


static void
gth_preview_tool_class_init (GthPreviewToolClass *class)
{
	GObjectClass *gobject_class;

	g_type_class_add_private (class, sizeof (GthPreviewToolPrivate));

	gobject_class = (GObjectClass*) class;
	gobject_class->finalize = gth_preview_tool_finalize;
}


static void
gth_preview_tool_gth_image_tool_interface_init (GthImageViewerToolInterface *iface)
{
	iface->set_viewer = gth_preview_tool_set_viewer;
	iface->unset_viewer = gth_preview_tool_unset_viewer;
	iface->realize = gth_preview_tool_realize;
	iface->unrealize = gth_preview_tool_unrealize;
	iface->size_allocate = gth_preview_tool_size_allocate;
	iface->map = gth_preview_tool_map;
	iface->unmap = gth_preview_tool_unmap;
	iface->draw = gth_preview_tool_draw;
	iface->button_press = gth_preview_tool_button_press;
	iface->button_release = gth_preview_tool_button_release;
	iface->motion_notify = gth_preview_tool_motion_notify;
	iface->image_changed = gth_preview_tool_image_changed;
	iface->zoom_changed = gth_preview_tool_zoom_changed;
}


static void
gth_preview_tool_init (GthPreviewTool *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_PREVIEW_TOOL, GthPreviewToolPrivate);
	self->priv->preview_image = NULL;
	self->priv->background_color.red = 0.2;
	self->priv->background_color.green = 0.2;
	self->priv->background_color.blue = 0.2;
	self->priv->background_color.alpha = 1.0;
}


GthImageViewerTool *
gth_preview_tool_new (void)
{
	return GTH_IMAGE_VIEWER_TOOL ( g_object_new (GTH_TYPE_PREVIEW_TOOL, NULL));
}


void
gth_preview_tool_set_image (GthPreviewTool  *self,
			    cairo_surface_t *modified)
{
	_cairo_clear_surface (&self->priv->preview_image);
	if (modified != NULL) {
		self->priv->preview_image = cairo_surface_reference (modified);
		update_preview_image_area (self);
	}

	if (self->priv->viewer)
		gtk_widget_queue_draw (GTK_WIDGET (self->priv->viewer));
}


cairo_surface_t *
gth_preview_tool_get_image (GthPreviewTool *self)
{
	return self->priv->preview_image;
}
