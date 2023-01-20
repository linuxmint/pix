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

#ifndef GTH_PREVIEW_TOOL_H
#define GTH_PREVIEW_TOOL_H

#include <glib.h>
#include <glib-object.h>
#include <gthumb.h>

G_BEGIN_DECLS

#define GTH_TYPE_PREVIEW_TOOL            (gth_preview_tool_get_type ())
#define GTH_PREVIEW_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_PREVIEW_TOOL, GthPreviewTool))
#define GTH_PREVIEW_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_PREVIEW_TOOL, GthPreviewToolClass))
#define GTH_IS_PREVIEW_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_PREVIEW_TOOL))
#define GTH_IS_PREVIEW_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_PREVIEW_TOOL))
#define GTH_PREVIEW_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_PREVIEW_TOOL, GthPreviewToolClass))

typedef struct _GthPreviewTool         GthPreviewTool;
typedef struct _GthPreviewToolClass    GthPreviewToolClass;
typedef struct _GthPreviewToolPrivate  GthPreviewToolPrivate;

struct _GthPreviewTool
{
	GObject __parent;
	GthPreviewToolPrivate *priv;
};

struct _GthPreviewToolClass
{
	GObjectClass __parent_class;
};

GType                 gth_preview_tool_get_type        (void);
GthImageViewerTool *  gth_preview_tool_new             (void);
void                  gth_preview_tool_set_image       (GthPreviewTool  *self,
                                 	 	 	cairo_surface_t *image);
cairo_surface_t *     gth_preview_tool_get_image       (GthPreviewTool  *self);

G_END_DECLS

#endif /* GTH_PREVIEW_TOOL_H */
