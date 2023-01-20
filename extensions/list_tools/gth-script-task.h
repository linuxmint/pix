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

#ifndef GTH_SCRIPT_TASK_H
#define GTH_SCRIPT_TASK_H

#include <glib.h>
#include <gthumb.h>
#include "gth-script.h"

G_BEGIN_DECLS

#define GTH_TYPE_SCRIPT_TASK            (gth_script_task_get_type ())
#define GTH_SCRIPT_TASK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_SCRIPT_TASK, GthScriptTask))
#define GTH_SCRIPT_TASK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_SCRIPT_TASK, GthScriptTaskClass))
#define GTH_IS_SCRIPT_TASK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_SCRIPT_TASK))
#define GTH_IS_SCRIPT_TASK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_SCRIPT_TASK))
#define GTH_SCRIPT_TASK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_SCRIPT_TASK, GthScriptTaskClass))

typedef struct _GthScriptTask        GthScriptTask;
typedef struct _GthScriptTaskClass   GthScriptTaskClass;
typedef struct _GthScriptTaskPrivate GthScriptTaskPrivate;

struct _GthScriptTask {
	GthTask __parent;
	GthScriptTaskPrivate *priv;
};

struct _GthScriptTaskClass {
	GthTaskClass __parent;
};

GType         gth_script_task_get_type     (void);
GthTask *     gth_script_task_new          (GtkWindow *parent,
					    GthScript *script,
					    GList     *file_list /* GthFileData */);

G_END_DECLS

#endif /* GTH_SCRIPT_TASK_H */
