/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2011 The Free Software Foundation, Inc.
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

#ifndef GTH_SAVE_FILE_DATA_TASK_H
#define GTH_SAVE_FILE_DATA_TASK_H

#include <glib.h>
#include "gth-file-data.h"
#include "gth-task.h"

G_BEGIN_DECLS

#define GTH_TYPE_SAVE_FILE_DATA_TASK            (gth_save_file_data_task_get_type ())
#define GTH_SAVE_FILE_DATA_TASK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_SAVE_FILE_DATA_TASK, GthSaveFileDataTask))
#define GTH_SAVE_FILE_DATA_TASK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_SAVE_FILE_DATA_TASK, GthSaveFileDataTaskClass))
#define GTH_IS_SAVE_FILE_DATA_TASK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_SAVE_FILE_DATA_TASK))
#define GTH_IS_SAVE_FILE_DATA_TASK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_SAVE_FILE_DATA_TASK))
#define GTH_SAVE_FILE_DATA_TASK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_SAVE_FILE_DATA_TASK, GthSaveFileDataTaskClass))

typedef struct _GthSaveFileDataTask        GthSaveFileDataTask;
typedef struct _GthSaveFileDataTaskClass   GthSaveFileDataTaskClass;
typedef struct _GthSaveFileDataTaskPrivate GthSaveFileDataTaskPrivate;

struct _GthSaveFileDataTask {
	GthTask __parent;
	GthSaveFileDataTaskPrivate *priv;
};

struct _GthSaveFileDataTaskClass {
	GthTaskClass __parent;
};

GType       gth_save_file_data_task_get_type   (void);
GthTask *   gth_save_file_data_task_new        (GList      *file_list, /* GthFileData list */
		     	     	     	        const char *attributes);

G_END_DECLS

#endif /* GTH_SAVE_FILE_DATA_TASK_H */
