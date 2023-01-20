/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2012 The Free Software Foundation, Inc.
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

#ifndef GTH_SAVE_IMAGE_TASK_H
#define GTH_SAVE_IMAGE_TASK_H

#include <glib.h>
#include "gth-image.h"
#include "gth-image-task.h"
#include "gth-overwrite-dialog.h"

G_BEGIN_DECLS

#define GTH_TYPE_SAVE_IMAGE_TASK            (gth_save_image_task_get_type ())
#define GTH_SAVE_IMAGE_TASK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_SAVE_IMAGE_TASK, GthSaveImageTask))
#define GTH_SAVE_IMAGE_TASK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_SAVE_IMAGE_TASK, GthSaveImageTaskClass))
#define GTH_IS_SAVE_IMAGE_TASK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_SAVE_IMAGE_TASK))
#define GTH_IS_SAVE_IMAGE_TASK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_SAVE_IMAGE_TASK))
#define GTH_SAVE_IMAGE_TASK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_SAVE_IMAGE_TASK, GthSaveImageTaskClass))

typedef struct _GthSaveImageTask        GthSaveImageTask;
typedef struct _GthSaveImageTaskClass   GthSaveImageTaskClass;
typedef struct _GthSaveImageTaskPrivate GthSaveImageTaskPrivate;

struct _GthSaveImageTask {
	GthImageTask __parent;
	GthSaveImageTaskPrivate *priv;
};

struct _GthSaveImageTaskClass {
	GthImageTaskClass __parent;
};

GType       gth_save_image_task_get_type   (void);
GthTask *   gth_save_image_task_new        (GthImage             *image,
					    const char           *mime_type,
					    GthFileData          *file_data,
					    GthOverwriteResponse  overwrite_mode);

G_END_DECLS

#endif /* GTH_SAVE_IMAGE_TASK_H */
