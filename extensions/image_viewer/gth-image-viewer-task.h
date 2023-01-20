/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2013 The Free Software Foundation, Inc.
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

#ifndef GTH_IMAGE_VIEWER_TASK_H
#define GTH_IMAGE_VIEWER_TASK_H

#include <gthumb.h>
#include "gth-image-viewer-page.h"

G_BEGIN_DECLS

#define GTH_TYPE_IMAGE_VIEWER_TASK            (gth_image_viewer_task_get_type ())
#define GTH_IMAGE_VIEWER_TASK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_IMAGE_VIEWER_TASK, GthImageViewerTask))
#define GTH_IMAGE_VIEWER_TASK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_IMAGE_VIEWER_TASK, GthImageViewerTaskClass))
#define GTH_IS_IMAGE_VIEWER_TASK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_IMAGE_VIEWER_TASK))
#define GTH_IS_IMAGE_VIEWER_TASK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_IMAGE_VIEWER_TASK))
#define GTH_IMAGE_VIEWER_TASK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_IMAGE_VIEWER_TASK, GthImageViewerTaskClass))

typedef struct _GthImageViewerTask        GthImageViewerTask;
typedef struct _GthImageViewerTaskClass   GthImageViewerTaskClass;
typedef struct _GthImageViewerTaskPrivate GthImageViewerTaskPrivate;

struct _GthImageViewerTask {
	GthImageTask __parent;
	GthImageViewerTaskPrivate *priv;
};

struct _GthImageViewerTaskClass {
	GthImageTaskClass __parent;
};

GType         gth_image_viewer_task_get_type		(void);
GthTask *     gth_image_viewer_task_new			(GthImageViewerPage	*viewer_page,
							 const char		*description,
							 GthAsyncInitFunc	 before_func,
							 GthAsyncThreadFunc	 exec_func,
							 GthAsyncReadyFunc	 after_func,
							 gpointer		 user_data,
							 GDestroyNotify		 user_data_destroy_func);
void	      gth_image_viewer_task_set_load_original   (GthImageViewerTask	*self,
							 gboolean		 load_original);
void	      gth_image_viewer_task_set_destination	(GthTask		*task,
							 GError			*error,
							 gpointer		 user_data);

G_END_DECLS

#endif /* GTH_IMAGE_VIEWER_TASK_H */
