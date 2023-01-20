/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2010 Free Software Foundation, Inc.
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

#ifndef GTH_BURN_TASK_H
#define GTH_BURN_TASK_H

#include <glib-object.h>
#include <gthumb.h>

#define GTH_TYPE_BURN_TASK         (gth_burn_task_get_type ())
#define GTH_BURN_TASK(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GTH_TYPE_BURN_TASK, GthBurnTask))
#define GTH_BURN_TASK_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GTH_TYPE_BURN_TASK, GthBurnTaskClass))
#define GTH_IS_BURN_TASK(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTH_TYPE_BURN_TASK))
#define GTH_IS_BURN_TASK_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GTH_TYPE_BURN_TASK))
#define GTH_BURN_TASK_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), GTH_TYPE_BURN_TASK, GthBurnTaskClass))

typedef struct _GthBurnTask         GthBurnTask;
typedef struct _GthBurnTaskPrivate  GthBurnTaskPrivate;
typedef struct _GthBurnTaskClass    GthBurnTaskClass;

struct _GthBurnTask
{
	GthTask __parent;
	GthBurnTaskPrivate *priv;
};

struct _GthBurnTaskClass
{
	GthTaskClass __parent_class;
};

GType       gth_burn_task_get_type   (void) G_GNUC_CONST;
GthTask *   gth_burn_task_new        (GthBrowser *browser,
		                      GList      *selected_files);

#endif /* GTH_BURN_TASK_H */
