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

#ifndef GTH_LOAD_FILE_DATA_TASK_H
#define GTH_LOAD_FILE_DATA_TASK_H

#include <glib.h>
#include "gth-file-data.h"
#include "gth-task.h"

G_BEGIN_DECLS

#define GTH_TYPE_LOAD_FILE_DATA_TASK            (gth_load_file_data_task_get_type ())
#define GTH_LOAD_FILE_DATA_TASK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_LOAD_FILE_DATA_TASK, GthLoadFileDataTask))
#define GTH_LOAD_FILE_DATA_TASK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_LOAD_FILE_DATA_TASK, GthLoadFileDataTaskClass))
#define GTH_IS_LOAD_FILE_DATA_TASK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_LOAD_FILE_DATA_TASK))
#define GTH_IS_LOAD_FILE_DATA_TASK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_LOAD_FILE_DATA_TASK))
#define GTH_LOAD_FILE_DATA_TASK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_LOAD_FILE_DATA_TASK, GthLoadFileDataTaskClass))

typedef struct _GthLoadFileDataTask        GthLoadFileDataTask;
typedef struct _GthLoadFileDataTaskClass   GthLoadFileDataTaskClass;
typedef struct _GthLoadFileDataTaskPrivate GthLoadFileDataTaskPrivate;

struct _GthLoadFileDataTask {
	GthTask __parent;
	GthLoadFileDataTaskPrivate *priv;
};

struct _GthLoadFileDataTaskClass {
	GthTaskClass __parent;
};

GType       gth_load_file_data_task_get_type   (void);
GthTask *   gth_load_file_data_task_new        (GList               *file_list, /* GFile list */
					        const char          *attributes);
GList *     gth_load_file_data_task_get_result (GthLoadFileDataTask *self);

G_END_DECLS

#endif /* GTH_LOAD_FILE_DATA_TASK_H */
