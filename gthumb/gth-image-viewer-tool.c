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
#include "gth-image-viewer.h"


G_DEFINE_INTERFACE (GthImageViewerTool, gth_image_viewer_tool, 0)


static void
gth_image_viewer_tool_default_init (GthImageViewerToolInterface *iface)
{
	/* void */
}


void
gth_image_viewer_tool_set_viewer (GthImageViewerTool *self,
				  GthImageViewer     *viewer)
{
	GTH_IMAGE_VIEWER_TOOL_GET_INTERFACE (self)->set_viewer (self, viewer);
}


void
gth_image_viewer_tool_unset_viewer (GthImageViewerTool *self,
				    GthImageViewer     *viewer)
{
	GTH_IMAGE_VIEWER_TOOL_GET_INTERFACE (self)->unset_viewer (self, viewer);
}


void
gth_image_viewer_tool_map (GthImageViewerTool *self)
{
	GTH_IMAGE_VIEWER_TOOL_GET_INTERFACE (self)->map (self);
}


void
gth_image_viewer_tool_unmap (GthImageViewerTool *self)
{
	GTH_IMAGE_VIEWER_TOOL_GET_INTERFACE (self)->unmap (self);
}


void
gth_image_viewer_tool_realize (GthImageViewerTool *self)
{
	GTH_IMAGE_VIEWER_TOOL_GET_INTERFACE (self)->realize (self);
}


void
gth_image_viewer_tool_unrealize (GthImageViewerTool *self)
{
	GTH_IMAGE_VIEWER_TOOL_GET_INTERFACE (self)->unrealize (self);
}


void
gth_image_viewer_tool_size_allocate (GthImageViewerTool *self,
				     GtkAllocation      *allocation)
{
	GTH_IMAGE_VIEWER_TOOL_GET_INTERFACE (self)->size_allocate (self, allocation);
}


void
gth_image_viewer_tool_draw (GthImageViewerTool *self,
			    cairo_t            *cr)
{
	GTH_IMAGE_VIEWER_TOOL_GET_INTERFACE (self)->draw (self, cr);
}


gboolean
gth_image_viewer_tool_button_press (GthImageViewerTool *self,
				    GdkEventButton     *event)
{
	return GTH_IMAGE_VIEWER_TOOL_GET_INTERFACE (self)->button_press (self, event);
}


gboolean
gth_image_viewer_tool_button_release (GthImageViewerTool *self,
				      GdkEventButton     *event)
{
	return GTH_IMAGE_VIEWER_TOOL_GET_INTERFACE (self)->button_release (self, event);
}


gboolean
gth_image_viewer_tool_motion_notify (GthImageViewerTool *self,
				     GdkEventMotion     *event)
{
	return GTH_IMAGE_VIEWER_TOOL_GET_INTERFACE (self)->motion_notify (self, event);
}


void
gth_image_viewer_tool_image_changed (GthImageViewerTool *self)
{
	GTH_IMAGE_VIEWER_TOOL_GET_INTERFACE (self)->image_changed (self);
}


void
gth_image_viewer_tool_zoom_changed (GthImageViewerTool *self)
{
	GTH_IMAGE_VIEWER_TOOL_GET_INTERFACE (self)->zoom_changed (self);
}
