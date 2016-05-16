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

#ifndef GTH_RENAME_TASK_H
#define GTH_RENAME_TASK_H

#include <glib.h>
#include <gthumb.h>

G_BEGIN_DECLS

#define GTH_TYPE_RENAME_TASK            (gth_rename_task_get_type ())
#define GTH_RENAME_TASK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_RENAME_TASK, GthRenameTask))
#define GTH_RENAME_TASK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_RENAME_TASK, GthRenameTaskClass))
#define GTH_IS_RENAME_TASK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_RENAME_TASK))
#define GTH_IS_RENAME_TASK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_RENAME_TASK))
#define GTH_RENAME_TASK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_RENAME_TASK, GthRenameTaskClass))

typedef struct _GthRenameTask        GthRenameTask;
typedef struct _GthRenameTaskClass   GthRenameTaskClass;
typedef struct _GthRenameTaskPrivate GthRenameTaskPrivate;

struct _GthRenameTask {
	GthTask __parent;
	GthRenameTaskPrivate *priv;
};

struct _GthRenameTaskClass {
	GthTaskClass __parent;
};

GType         gth_rename_task_get_type     (void);
GthTask *     gth_rename_task_new          (GList  *old_files, /* GFile */
					    GList  *new_files  /* GFile */);

G_END_DECLS

#endif /* GTH_RENAME_TASK_H */
