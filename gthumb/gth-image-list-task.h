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

#ifndef GTH_IMAGE_LIST_TASK_H
#define GTH_IMAGE_LIST_TASK_H

#include <glib.h>
#include "gth-browser.h"
#include "gth-image-task.h"
#include "typedefs.h"

G_BEGIN_DECLS

#define GTH_TYPE_IMAGE_LIST_TASK            (gth_image_list_task_get_type ())
#define GTH_IMAGE_LIST_TASK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_IMAGE_LIST_TASK, GthImageListTask))
#define GTH_IMAGE_LIST_TASK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_IMAGE_LIST_TASK, GthImageListTaskClass))
#define GTH_IS_IMAGE_LIST_TASK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_IMAGE_LIST_TASK))
#define GTH_IS_IMAGE_LIST_TASK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_IMAGE_LIST_TASK))
#define GTH_IMAGE_LIST_TASK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_IMAGE_LIST_TASK, GthImageListTaskClass))

typedef struct _GthImageListTask        GthImageListTask;
typedef struct _GthImageListTaskClass   GthImageListTaskClass;
typedef struct _GthImageListTaskPrivate GthImageListTaskPrivate;

struct _GthImageListTask {
	GthTask __parent;
	GthImageListTaskPrivate *priv;
};

struct _GthImageListTaskClass {
	GthTaskClass __parent;
};

GType      gth_image_list_task_get_type             (void);
GthTask *  gth_image_list_task_new                  (GthBrowser          *browser,
						     GList               *file_list, /* GthFileData list */
						     GthImageTask        *task);
void       gth_image_list_task_set_destination      (GthImageListTask    *self,
						     GFile               *folder);
void       gth_image_list_task_set_overwrite_mode   (GthImageListTask    *self,
						     GthOverwriteMode     overwrite_mode);
void       gth_image_list_task_set_output_mime_type (GthImageListTask    *self,
						     const char          *mime_type);

G_END_DECLS

#endif /* GTH_IMAGE_LIST_TASK_H */
