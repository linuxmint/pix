/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Reorderright (C) 2009 The Free Software Foundation, Inc.
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
 *  You should have received a reorder of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GTH_REORDER_TASK_H
#define GTH_REORDER_TASK_H

#include <glib.h>
#include <gthumb.h>

G_BEGIN_DECLS

#define GTH_TYPE_REORDER_TASK            (gth_reorder_task_get_type ())
#define GTH_REORDER_TASK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_REORDER_TASK, GthReorderTask))
#define GTH_REORDER_TASK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_REORDER_TASK, GthReorderTaskClass))
#define GTH_IS_REORDER_TASK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_REORDER_TASK))
#define GTH_IS_REORDER_TASK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_REORDER_TASK))
#define GTH_REORDER_TASK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_REORDER_TASK, GthReorderTaskClass))

typedef struct _GthReorderTask        GthReorderTask;
typedef struct _GthReorderTaskClass   GthReorderTaskClass;
typedef struct _GthReorderTaskPrivate GthReorderTaskPrivate;

struct _GthReorderTask {
	GthTask __parent;
	GthReorderTaskPrivate *priv;
};

struct _GthReorderTaskClass {
	GthTaskClass __parent;
};

GType         gth_reorder_task_get_type  (void);
GthTask *     gth_reorder_task_new       (GthFileSource *file_source,
					  GthFileData   *destination,
					  GList         *visible_files, /* GFile list */
					  GList         *files_to_move, /* GFile list */
					  int            new_pos);

G_END_DECLS

#endif /* GTH_REORDER_TASK_H */
