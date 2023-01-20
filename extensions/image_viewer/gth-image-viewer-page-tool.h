/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2013 Free Software Foundation, Inc.
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

#ifndef GTH_IMAGE_VIEWER_PAGE_PAGE_TOOL_H
#define GTH_IMAGE_VIEWER_PAGE_PAGE_TOOL_H

#include <gthumb.h>
#include <extensions/image_viewer/image-viewer.h>

G_BEGIN_DECLS

#define GTH_TYPE_IMAGE_VIEWER_PAGE_TOOL (gth_image_viewer_page_tool_get_type ())
#define GTH_IMAGE_VIEWER_PAGE_TOOL(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_IMAGE_VIEWER_PAGE_TOOL, GthImageViewerPageTool))
#define GTH_IMAGE_VIEWER_PAGE_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_IMAGE_VIEWER_PAGE_TOOL, GthImageViewerPageToolClass))
#define GTH_IS_IMAGE_VIEWER_PAGE_TOOL(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_IMAGE_VIEWER_PAGE_TOOL))
#define GTH_IS_IMAGE_VIEWER_PAGE_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_IMAGE_VIEWER_PAGE_TOOL))
#define GTH_IMAGE_VIEWER_PAGE_TOOL_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_IMAGE_VIEWER_PAGE_TOOL, GthImageViewerPageToolClass))

typedef struct _GthImageViewerPageTool GthImageViewerPageTool;
typedef struct _GthImageViewerPageToolClass GthImageViewerPageToolClass;
typedef struct _GthImageViewerPageToolPrivate GthImageViewerPageToolPrivate;

struct _GthImageViewerPageTool {
	GthFileTool parent_instance;
	GthImageViewerPageToolPrivate *priv;
};

struct _GthImageViewerPageToolClass {
	GthFileToolClass parent_class;

	/* virtual functions */

	void         (*modify_image)      (GthImageViewerPageTool *self);
	void         (*reset_image)       (GthImageViewerPageTool *self);
};

GType			gth_image_viewer_page_tool_get_type		(void);
cairo_surface_t *	gth_image_viewer_page_tool_get_source		(GthImageViewerPageTool *self);
GthTask *		gth_image_viewer_page_tool_get_task		(GthImageViewerPageTool *self);
GthViewerPage *		gth_image_viewer_page_tool_get_page		(GthImageViewerPageTool *self);
void			gth_image_viewer_page_tool_reset_image		(GthImageViewerPageTool *self);

G_END_DECLS

#endif /* GTH_IMAGE_VIEWER_PAGE_PAGE_TOOL_H */
