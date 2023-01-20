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

#ifndef GTH_FILE_TOOL_CROP_H
#define GTH_FILE_TOOL_CROP_H

#include <gthumb.h>
#include <extensions/image_viewer/image-viewer.h>

G_BEGIN_DECLS

#define GTH_TYPE_FILE_TOOL_CROP (gth_file_tool_crop_get_type ())
#define GTH_FILE_TOOL_CROP(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_FILE_TOOL_CROP, GthFileToolCrop))
#define GTH_FILE_TOOL_CROP_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_FILE_TOOL_CROP, GthFileToolCropClass))
#define GTH_IS_FILE_TOOL_CROP(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_FILE_TOOL_CROP))
#define GTH_IS_FILE_TOOL_CROP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_FILE_TOOL_CROP))
#define GTH_FILE_TOOL_CROP_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_FILE_TOOL_CROP, GthFileToolCropClass))

typedef struct _GthFileToolCrop GthFileToolCrop;
typedef struct _GthFileToolCropClass GthFileToolCropClass;
typedef struct _GthFileToolCropPrivate GthFileToolCropPrivate;

struct _GthFileToolCrop {
	GthImageViewerPageTool parent_instance;
	GthFileToolCropPrivate *priv;
};

struct _GthFileToolCropClass {
	GthImageViewerPageToolClass parent_class;
};

GType  gth_file_tool_crop_get_type  (void);

G_END_DECLS

#endif /* GTH_FILE_TOOL_CROP_H */
