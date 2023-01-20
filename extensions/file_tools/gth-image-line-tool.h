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

#ifndef GTH_IMAGE_LINE_TOOL_H
#define GTH_IMAGE_LINE_TOOL_H

#include <glib.h>
#include <glib-object.h>
#include <gthumb.h>

G_BEGIN_DECLS

#define GTH_TYPE_IMAGE_LINE_TOOL            (gth_image_line_tool_get_type ())
#define GTH_IMAGE_LINE_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_IMAGE_LINE_TOOL, GthImageLineTool))
#define GTH_IMAGE_LINE_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_IMAGE_LINE_TOOL, GthImageLineToolClass))
#define GTH_IS_IMAGE_LINE_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_IMAGE_LINE_TOOL))
#define GTH_IS_IMAGE_LINE_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_IMAGE_LINE_TOOL))
#define GTH_IMAGE_LINE_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_IMAGE_LINE_TOOL, GthImageLineToolClass))

typedef struct _GthImageLineTool         GthImageLineTool;
typedef struct _GthImageLineToolClass    GthImageLineToolClass;
typedef struct _GthImageLineToolPrivate  GthImageLineToolPrivate;

struct _GthImageLineTool
{
	GObject __parent;
	GthImageLineToolPrivate *priv;
};

struct _GthImageLineToolClass
{
	GObjectClass __parent_class;

	/*< signals >*/

	void (* changed) (GthImageLineTool *rotator);
};

GType                 gth_image_line_tool_get_type    (void);
GthImageViewerTool *  gth_image_line_tool_new         (void);
void                  gth_image_line_tool_get_points  (GthImageLineTool  *self,
						       GdkPoint          *p1,
						       GdkPoint          *p2);

G_END_DECLS

#endif /* GTH_IMAGE_LINE_TOOL_H */
