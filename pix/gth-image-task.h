/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001-2009 The Free Software Foundation, Inc.
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

#ifndef GTH_IMAGE_TASK_H
#define GTH_IMAGE_TASK_H

#include <glib.h>
#include "gth-async-task.h"
#include "gth-image.h"

G_BEGIN_DECLS

#define GTH_TYPE_IMAGE_TASK            (gth_image_task_get_type ())
#define GTH_IMAGE_TASK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_IMAGE_TASK, GthImageTask))
#define GTH_IMAGE_TASK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_IMAGE_TASK, GthImageTaskClass))
#define GTH_IS_IMAGE_TASK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_IMAGE_TASK))
#define GTH_IS_IMAGE_TASK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_IMAGE_TASK))
#define GTH_IMAGE_TASK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_IMAGE_TASK, GthImageTaskClass))

typedef struct _GthImageTask        GthImageTask;
typedef struct _GthImageTaskClass   GthImageTaskClass;
typedef struct _GthImageTaskPrivate GthImageTaskPrivate;

struct _GthImageTask {
	GthAsyncTask __parent;
	GthImageTaskPrivate *priv;
};

struct _GthImageTaskClass {
	GthAsyncTaskClass __parent;
};

GType			gth_image_task_get_type				(void);
GthTask *		gth_image_task_new				(const char		*description,
									 GthAsyncInitFunc	 before_func,
									 GthAsyncThreadFunc	 exec_func,
									 GthAsyncReadyFunc	 after_func,
									 gpointer		 user_data,
									 GDestroyNotify		 user_data_destroy_func);
void			gth_image_task_set_source			(GthImageTask		*self,
								 	 GthImage		*source);
void			gth_image_task_set_source_surface		(GthImageTask		*self,
								 	 cairo_surface_t	*surface);
GthImage *		gth_image_task_get_source			(GthImageTask		*self);
cairo_surface_t *	gth_image_task_get_source_surface		(GthImageTask		*self);
void			gth_image_task_set_destination			(GthImageTask		*self,
								 	 GthImage		*destination);
void			gth_image_task_set_destination_surface		(GthImageTask		*self,
								 	 cairo_surface_t	*surface);
GthImage *		gth_image_task_get_destination			(GthImageTask		*self);
cairo_surface_t *	gth_image_task_get_destination_surface		(GthImageTask		*self);
void			gth_image_task_copy_source_to_destination	(GthImageTask		*self);

G_END_DECLS

#endif /* GTH_IMAGE_TASK_H */
