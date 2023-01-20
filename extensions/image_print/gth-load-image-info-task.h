/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2009 The Free Software Foundation, Inc.
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

#ifndef GTH_LOAD_IMAGE_INFO_TASK_H
#define GTH_LOAD_IMAGE_INFO_TASK_H

#include <glib.h>
#include <gthumb.h>
#include "gth-image-info.h"

G_BEGIN_DECLS

#define GTH_TYPE_LOAD_IMAGE_INFO_TASK            (gth_load_image_info_task_get_type ())
#define GTH_LOAD_IMAGE_INFO_TASK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_LOAD_IMAGE_INFO_TASK, GthLoadImageInfoTask))
#define GTH_LOAD_IMAGE_INFO_TASK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_LOAD_IMAGE_INFO_TASK, GthLoadImageInfoTaskClass))
#define GTH_IS_LOAD_IMAGE_INFO_TASK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_LOAD_IMAGE_INFO_TASK))
#define GTH_IS_LOAD_IMAGE_INFO_TASK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_LOAD_IMAGE_INFO_TASK))
#define GTH_LOAD_IMAGE_INFO_TASK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_LOAD_IMAGE_INFO_TASK, GthLoadImageInfoTaskClass))

typedef struct _GthLoadImageInfoTask        GthLoadImageInfoTask;
typedef struct _GthLoadImageInfoTaskClass   GthLoadImageInfoTaskClass;
typedef struct _GthLoadImageInfoTaskPrivate GthLoadImageInfoTaskPrivate;

struct _GthLoadImageInfoTask {
	GthTask __parent;
	GthLoadImageInfoTaskPrivate *priv;
};

struct _GthLoadImageInfoTaskClass {
	GthTaskClass __parent;
};

GType       gth_load_image_info_task_get_type  (void);
GthTask *   gth_load_image_info_task_new       (GthImageInfo **images,
					        int            n_images,
						const char    *attributes);

G_END_DECLS

#endif /* GTH_LOAD_IMAGE_INFO_TASK_H */
