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

#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include "gth-delete-task.h"


struct _GthDeleteTaskPrivate {
	GList *file_list;
};


G_DEFINE_TYPE_WITH_CODE (GthDeleteTask,
			 gth_delete_task,
			 GTH_TYPE_TASK,
			 G_ADD_PRIVATE (GthDeleteTask))


static void
gth_delete_task_finalize (GObject *object)
{
	GthDeleteTask *self;

	self = GTH_DELETE_TASK (object);

	_g_object_list_unref (self->priv->file_list);

	G_OBJECT_CLASS (gth_delete_task_parent_class)->finalize (object);
}


static void
delete_ready_cb (GError   *error,
		 gpointer  user_data)
{
	gth_task_completed (GTH_TASK (user_data), error);
}


static void
delete_progress_cb (GObject    *object,
		    const char *description,
		    const char *details,
		    gboolean    pulse,
		    double      fraction,
		    gpointer    user_data)
{
	GthDeleteTask *self = user_data;

	gth_task_progress (GTH_TASK (self), description, details, pulse, fraction);
}


static void
gth_delete_task_exec (GthTask *task)
{
	GthDeleteTask *self;

	self = GTH_DELETE_TASK (task);

	_g_file_list_delete_async (self->priv->file_list,
				   TRUE,
				   TRUE,
				   gth_task_get_cancellable (task),
				   delete_progress_cb,
				   delete_ready_cb,
				   self);
}


static void
gth_delete_task_class_init (GthDeleteTaskClass *klass)
{
	GObjectClass *object_class;
	GthTaskClass *task_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = gth_delete_task_finalize;

	task_class = GTH_TASK_CLASS (klass);
	task_class->exec = gth_delete_task_exec;
}


static void
gth_delete_task_init (GthDeleteTask *self)
{
	self->priv = gth_delete_task_get_instance_private (self);
}


GthTask *
gth_delete_task_new (GList *file_list)
{
	GthDeleteTask *self;

	self = GTH_DELETE_TASK (g_object_new (GTH_TYPE_DELETE_TASK, NULL));
	self->priv->file_list = _g_object_list_ref (file_list);

	return (GthTask *) self;
}
