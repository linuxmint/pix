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

#ifndef GTH_DELETE_METADATA_TASK_H
#define GTH_DELETE_METADATA_TASK_H

#include <glib.h>
#include <gthumb.h>

G_BEGIN_DECLS

#define GTH_TYPE_DELETE_METADATA_TASK            (gth_delete_metadata_task_get_type ())
#define GTH_DELETE_METADATA_TASK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_DELETE_METADATA_TASK, GthDeleteMetadataTask))
#define GTH_DELETE_METADATA_TASK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_DELETE_METADATA_TASK, GthDeleteMetadataTaskClass))
#define GTH_IS_DELETE_METADATA_TASK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_DELETE_METADATA_TASK))
#define GTH_IS_DELETE_METADATA_TASK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_DELETE_METADATA_TASK))
#define GTH_DELETE_METADATA_TASK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_DELETE_METADATA_TASK, GthDeleteMetadataTaskClass))

typedef struct _GthDeleteMetadataTask        GthDeleteMetadataTask;
typedef struct _GthDeleteMetadataTaskClass   GthDeleteMetadataTaskClass;
typedef struct _GthDeleteMetadataTaskPrivate GthDeleteMetadataTaskPrivate;

struct _GthDeleteMetadataTask {
	GthTask __parent;
	GthDeleteMetadataTaskPrivate *priv;
};

struct _GthDeleteMetadataTaskClass {
	GthTaskClass __parent;
};

GType         gth_delete_metadata_task_get_type  (void);
GthTask *     gth_delete_metadata_task_new       (GthBrowser   *browser,
						  GList        *file_list /* GFile list */);

G_END_DECLS

#endif /* GTH_DELETE_METADATA_TASK_H */
