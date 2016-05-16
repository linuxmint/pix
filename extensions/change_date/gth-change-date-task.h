/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2010 The Free Software Foundation, Inc.
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

#ifndef GTH_CHANGE_DATE_TASK_H
#define GTH_CHANGE_DATE_TASK_H

#include <glib.h>
#include <gthumb.h>

G_BEGIN_DECLS

typedef enum {
	GTH_CHANGE_LAST_MODIFIED_DATE = 1 << 0,
	GTH_CHANGE_COMMENT_DATE = 1 << 1
} GthChangeFields;

typedef enum {
	GTH_CHANGE_TO_FOLLOWING_DATE,
	GTH_CHANGE_TO_FILE_MODIFIED_DATE,
	GTH_CHANGE_TO_FILE_CREATION_DATE,
	GTH_CHANGE_TO_PHOTO_ORIGINAL_DATE,
	GTH_CHANGE_ADJUST_TIME
} GthChangeType;

#define GTH_TYPE_CHANGE_DATE_TASK            (gth_change_date_task_get_type ())
#define GTH_CHANGE_DATE_TASK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_CHANGE_DATE_TASK, GthChangeDateTask))
#define GTH_CHANGE_DATE_TASK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_CHANGE_DATE_TASK, GthChangeDateTaskClass))
#define GTH_IS_CHANGE_DATE_TASK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_CHANGE_DATE_TASK))
#define GTH_IS_CHANGE_DATE_TASK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_CHANGE_DATE_TASK))
#define GTH_CHANGE_DATE_TASK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_CHANGE_DATE_TASK, GthChangeDateTaskClass))

typedef struct _GthChangeDateTask        GthChangeDateTask;
typedef struct _GthChangeDateTaskClass   GthChangeDateTaskClass;
typedef struct _GthChangeDateTaskPrivate GthChangeDateTaskPrivate;

struct _GthChangeDateTask {
	GthTask __parent;
	GthChangeDateTaskPrivate *priv;
};

struct _GthChangeDateTaskClass {
	GthTaskClass __parent;
};

GType         gth_change_date_task_get_type  (void);
GthTask *     gth_change_date_task_new       (GFile             *location,
					      GList             *files, /* GthFileData */
					      GthChangeFields    fields,
					      GthChangeType      change_type,
					      GthDateTime       *date_time,
					      int                time_offset);

G_END_DECLS

#endif /* GTH_CHANGE_DATE_TASK_H */
