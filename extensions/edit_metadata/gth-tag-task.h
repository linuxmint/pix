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

#ifndef GTH_TAG_TASK_H
#define GTH_TAG_TASK_H

#include <glib.h>
#include <gthumb.h>

G_BEGIN_DECLS

#define GTH_TYPE_TAG_TASK            (gth_tag_task_get_type ())
#define GTH_TAG_TASK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_TAG_TASK, GthTagTask))
#define GTH_TAG_TASK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_TAG_TASK, GthTagTaskClass))
#define GTH_IS_TAG_TASK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_TAG_TASK))
#define GTH_IS_TAG_TASK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_TAG_TASK))
#define GTH_TAG_TASK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_TAG_TASK, GthTagTaskClass))

typedef struct _GthTagTask        GthTagTask;
typedef struct _GthTagTaskClass   GthTagTaskClass;
typedef struct _GthTagTaskPrivate GthTagTaskPrivate;

struct _GthTagTask {
	GthTask __parent;
	GthTagTaskPrivate *priv;
};

struct _GthTagTaskClass {
	GthTaskClass __parent;
};

GType         gth_tag_task_get_type     (void);
GthTask *     gth_tag_task_new          (GList  *files, /* GFile */
					 char  **tags);

G_END_DECLS

#endif /* GTH_TAG_TASK_H */
