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

#ifndef GTH_IMAGE_VIEWER_TOOL_H
#define GTH_IMAGE_VIEWER_TOOL_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GTH_TYPE_IMAGE_VIEWER_TOOL               (gth_image_viewer_tool_get_type ())
#define GTH_IMAGE_VIEWER_TOOL(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_IMAGE_VIEWER_TOOL, GthImageViewerTool))
#define GTH_IS_IMAGE_VIEWER_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_IMAGE_VIEWER_TOOL))
#define GTH_IMAGE_VIEWER_TOOL_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GTH_TYPE_IMAGE_VIEWER_TOOL, GthImageViewerToolInterface))

typedef struct _GthImageViewerTool GthImageViewerTool;
typedef struct _GthImageViewerToolInterface GthImageViewerToolInterface;

struct _GthImageViewerToolInterface {
	GTypeInterface parent_iface;

	void      (*set_viewer)     (GthImageViewerTool   *self,
	  	  		     GthImageViewer       *viewer);
	void      (*unset_viewer)   (GthImageViewerTool   *self,
	  	  		     GthImageViewer       *viewer);
	void      (*map)            (GthImageViewerTool   *self);
	void      (*unmap)          (GthImageViewerTool   *self);
	void      (*realize)        (GthImageViewerTool   *self);
	void      (*unrealize)      (GthImageViewerTool   *self);
	void      (*size_allocate)  (GthImageViewerTool   *self,
				     GtkAllocation        *allocation);
	void      (*draw)           (GthImageViewerTool   *self,
				     cairo_t              *cr);
	gboolean  (*button_press)   (GthImageViewerTool   *self,
				     GdkEventButton       *event);
	gboolean  (*button_release) (GthImageViewerTool   *self,
				     GdkEventButton       *event);
	gboolean  (*motion_notify)  (GthImageViewerTool   *self,
				     GdkEventMotion       *event);
	void      (*image_changed)  (GthImageViewerTool   *self);
	void      (*zoom_changed)   (GthImageViewerTool   *self);
};

GType      gth_image_viewer_tool_get_type         (void);
void       gth_image_viewer_tool_set_viewer       (GthImageViewerTool *self,
				  	  	   GthImageViewer     *viewer);
void       gth_image_viewer_tool_unset_viewer     (GthImageViewerTool *self,
				  	  	   GthImageViewer     *viewer);
void       gth_image_viewer_tool_map              (GthImageViewerTool *self);
void       gth_image_viewer_tool_unmap            (GthImageViewerTool *self);
void       gth_image_viewer_tool_realize          (GthImageViewerTool *self);
void       gth_image_viewer_tool_unrealize        (GthImageViewerTool *self);
void       gth_image_viewer_tool_size_allocate    (GthImageViewerTool *self,
						   GtkAllocation      *allocation);
void       gth_image_viewer_tool_draw             (GthImageViewerTool *self,
						   cairo_t            *cr);
gboolean   gth_image_viewer_tool_button_press     (GthImageViewerTool *self,
						   GdkEventButton     *event);
gboolean   gth_image_viewer_tool_button_release   (GthImageViewerTool *self,
						   GdkEventButton     *event);
gboolean   gth_image_viewer_tool_motion_notify    (GthImageViewerTool *self,
						   GdkEventMotion     *event);
void       gth_image_viewer_tool_image_changed    (GthImageViewerTool *self);
void       gth_image_viewer_tool_zoom_changed     (GthImageViewerTool *self);

G_END_DECLS

#endif /* GTH_IMAGE_VIEWER_TOOL_H */
