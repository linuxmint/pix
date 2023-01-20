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

#ifndef GTH_ASYNC_TASK_H
#define GTH_ASYNC_TASK_H

#include <glib.h>
#include "gth-task.h"

G_BEGIN_DECLS

#define GTH_TYPE_ASYNC_TASK            (gth_async_task_get_type ())
#define GTH_ASYNC_TASK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_ASYNC_TASK, GthAsyncTask))
#define GTH_ASYNC_TASK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_ASYNC_TASK, GthAsyncTaskClass))
#define GTH_IS_ASYNC_TASK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_ASYNC_TASK))
#define GTH_IS_ASYNC_TASK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_ASYNC_TASK))
#define GTH_ASYNC_TASK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_ASYNC_TASK, GthAsyncTaskClass))

typedef struct _GthAsyncTask        GthAsyncTask;
typedef struct _GthAsyncTaskClass   GthAsyncTaskClass;
typedef struct _GthAsyncTaskPrivate GthAsyncTaskPrivate;

struct _GthAsyncTask {
	GthTask __parent;
	GthAsyncTaskPrivate *priv;
};

struct _GthAsyncTaskClass {
	GthTaskClass __parent;
};

typedef void     (*GthAsyncInitFunc)   (GthAsyncTask *task,
				        gpointer      user_data);
typedef gpointer (*GthAsyncThreadFunc) (GthAsyncTask *task,
				        gpointer      user_data);
typedef void     (*GthAsyncReadyFunc)  (GthAsyncTask *task,
				        GError       *error,
				        gpointer      user_data);

GType         gth_async_task_get_type    (void);
GthTask *     gth_async_task_new         (GthAsyncInitFunc    before_func,
					  GthAsyncThreadFunc  exec_func,
					  GthAsyncReadyFunc   after_func,
					  gpointer            user_data,
					  GDestroyNotify      user_data_destroy_func);
void          gth_async_task_set_data    (GthAsyncTask       *self,
					  gboolean           *terminated,
					  gboolean           *cancelled,
					  double             *progress);
void          gth_async_task_get_data    (GthAsyncTask       *self,
					  gboolean           *terminated,
					  gboolean           *cancelled,
					  double             *progress);

G_END_DECLS

#endif /* GTH_ASYNC_TASK_H */
