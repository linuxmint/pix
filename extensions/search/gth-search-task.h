/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2009 Free Software Foundation, Inc.
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

#ifndef GTH_SEARCH_TASK_H
#define GTH_SEARCH_TASK_H

#include <glib-object.h>
#include <gthumb.h>
#include "gth-search.h"

#define GTH_TYPE_SEARCH_TASK         (gth_search_task_get_type ())
#define GTH_SEARCH_TASK(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GTH_TYPE_SEARCH_TASK, GthSearchTask))
#define GTH_SEARCH_TASK_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GTH_TYPE_SEARCH_TASK, GthSearchTaskClass))
#define GTH_IS_SEARCH_TASK(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTH_TYPE_SEARCH_TASK))
#define GTH_IS_SEARCH_TASK_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GTH_TYPE_SEARCH_TASK))
#define GTH_SEARCH_TASK_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), GTH_TYPE_SEARCH_TASK, GthSearchTaskClass))

typedef struct _GthSearchTask         GthSearchTask;
typedef struct _GthSearchTaskPrivate  GthSearchTaskPrivate;
typedef struct _GthSearchTaskClass    GthSearchTaskClass;

struct _GthSearchTask
{
	GthTask __parent;
	GthSearchTaskPrivate *priv;
};

struct _GthSearchTaskClass
{
	GthTaskClass __parent_class;
};

GType       gth_search_task_get_type     (void) G_GNUC_CONST;
GthTask *   gth_search_task_new          (GthBrowser     *browser,
					  GthSearch      *search,
					  GFile          *search_catalog);
GFile *     gth_search_task_get_catalog  (GthSearchTask  *task);

#endif /* GTH_SEARCH_TASK_H */
