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
 *  You should have received a reorder of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include "gth-reorder-task.h"


struct _GthReorderTaskPrivate {
	GthFileSource *file_source;
	GthFileData   *destination;
	GList         *visible_files;
	GList         *files_to_move;
	int            new_pos;
};


G_DEFINE_TYPE_WITH_CODE (GthReorderTask,
			 gth_reorder_task,
			 GTH_TYPE_TASK,
			 G_ADD_PRIVATE (GthReorderTask))


static void
gth_reorder_task_finalize (GObject *object)
{
	GthReorderTask *self;

	self = GTH_REORDER_TASK (object);

	_g_object_list_unref (self->priv->visible_files);
	_g_object_list_unref (self->priv->files_to_move);
	_g_object_unref (self->priv->destination);
	_g_object_unref (self->priv->file_source);

	G_OBJECT_CLASS (gth_reorder_task_parent_class)->finalize (object);
}


static void
reorder_done_cb (GObject  *object,
	         GError   *error,
	         gpointer  user_data)
{
	gth_task_completed (GTH_TASK (user_data), error);
}


static void
gth_reorder_task_exec (GthTask *task)
{
	GthReorderTask *self;

	g_return_if_fail (GTH_IS_REORDER_TASK (task));

	self = GTH_REORDER_TASK (task);

	gth_file_source_reorder (self->priv->file_source,
				 self->priv->destination,
			         self->priv->visible_files,
			         self->priv->files_to_move,
			         self->priv->new_pos,
			         reorder_done_cb,
			         self);
}


static void
gth_reorder_task_cancelled (GthTask *task)
{
	gth_file_source_cancel (GTH_REORDER_TASK (task)->priv->file_source);
}


static void
gth_reorder_task_class_init (GthReorderTaskClass *klass)
{
	GObjectClass *object_class;
	GthTaskClass *task_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = gth_reorder_task_finalize;

	task_class = GTH_TASK_CLASS (klass);
	task_class->exec = gth_reorder_task_exec;
	task_class->cancelled = gth_reorder_task_cancelled;
}


static void
gth_reorder_task_init (GthReorderTask *self)
{
	self->priv = gth_reorder_task_get_instance_private (self);
}


GthTask *
gth_reorder_task_new (GthFileSource *file_source,
		      GthFileData   *destination,
		      GList         *visible_files, /* GFile list */
		      GList         *files_to_move, /* GFile list */
		      int            new_pos)
{
	GthReorderTask *self;

	self = GTH_REORDER_TASK (g_object_new (GTH_TYPE_REORDER_TASK, NULL));

	self->priv->file_source = g_object_ref (file_source);
	self->priv->destination = g_object_ref (destination);
	self->priv->new_pos = new_pos;
	self->priv->visible_files = _g_object_list_ref (visible_files);
	self->priv->files_to_move = _g_object_list_ref (files_to_move);

	return (GthTask *) self;
}
