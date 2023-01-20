/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2015 The Free Software Foundation, Inc.
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

#ifndef GTH_TRASH_TASK_H
#define GTH_TRASH_TASK_H

#include <glib.h>
#include "glib-utils.h"
#include "gio-utils.h"
#include "gth-task.h"

G_BEGIN_DECLS

#define GTH_TYPE_TRASH_TASK            (gth_trash_task_get_type ())
#define GTH_TRASH_TASK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_TRASH_TASK, GthTrashTask))
#define GTH_TRASH_TASK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_TRASH_TASK, GthTrashTaskClass))
#define GTH_IS_TRASH_TASK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_TRASH_TASK))
#define GTH_IS_TRASH_TASK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_TRASH_TASK))
#define GTH_TRASH_TASK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_TRASH_TASK, GthTrashTaskClass))

typedef struct _GthTrashTask        GthTrashTask;
typedef struct _GthTrashTaskClass   GthTrashTaskClass;
typedef struct _GthTrashTaskPrivate GthTrashTaskPrivate;

struct _GthTrashTask {
	GthTask __parent;
	GthTrashTaskPrivate *priv;
};

struct _GthTrashTaskClass {
	GthTaskClass __parent;
};

GType         gth_trash_task_get_type     (void);
GthTask *     gth_trash_task_new          (GList *file_list);

G_END_DECLS

#endif /* GTH_TRASH_TASK_H */
