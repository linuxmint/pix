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

#ifndef GTH_DUPLICATE_TASK_H
#define GTH_DUPLICATE_TASK_H

#include <glib.h>
#include <gthumb.h>

G_BEGIN_DECLS

#define GTH_TYPE_DUPLICATE_TASK            (gth_duplicate_task_get_type ())
#define GTH_DUPLICATE_TASK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_DUPLICATE_TASK, GthDuplicateTask))
#define GTH_DUPLICATE_TASK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_DUPLICATE_TASK, GthDuplicateTaskClass))
#define GTH_IS_DUPLICATE_TASK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_DUPLICATE_TASK))
#define GTH_IS_DUPLICATE_TASK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_DUPLICATE_TASK))
#define GTH_DUPLICATE_TASK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_DUPLICATE_TASK, GthDuplicateTaskClass))

typedef struct _GthDuplicateTask        GthDuplicateTask;
typedef struct _GthDuplicateTaskClass   GthDuplicateTaskClass;
typedef struct _GthDuplicateTaskPrivate GthDuplicateTaskPrivate;

struct _GthDuplicateTask {
	GthTask __parent;
	GthDuplicateTaskPrivate *priv;
};

struct _GthDuplicateTaskClass {
	GthTaskClass __parent;
};

GType         gth_duplicate_task_get_type     (void);
GthTask *     gth_duplicate_task_new          (GList *file_list);

G_END_DECLS

#endif /* GTH_DUPLICATE_TASK_H */
