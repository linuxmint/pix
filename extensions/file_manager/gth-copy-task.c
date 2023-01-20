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
#include "gth-copy-task.h"


struct _GthCopyTaskPrivate {
	GthFileData   *destination;
	GthFileSource *file_source;
	GList         *files;
	gboolean       move;
	int            destination_position;
};


G_DEFINE_TYPE_WITH_CODE (GthCopyTask,
			 gth_copy_task,
			 GTH_TYPE_TASK,
			 G_ADD_PRIVATE (GthCopyTask))


static void
gth_copy_task_finalize (GObject *object)
{
	GthCopyTask *self;

	self = GTH_COPY_TASK (object);

	_g_object_list_unref (self->priv->files);
	_g_object_unref (self->priv->file_source);
	_g_object_unref (self->priv->destination);

	G_OBJECT_CLASS (gth_copy_task_parent_class)->finalize (object);
}


static void
copy_done_cb (GObject    *object,
	      GError     *error,
	      gpointer    user_data)
{
	/* Errors with code G_IO_ERROR_EXISTS are generated when the user
	 * chooses to not overwrite the files.  There is no need to show an
	 * error dialog for this type of errors.  To do this create a
	 * GTH_TASK_ERROR_CANCELLED error, which is always ignored by
	 * GthBrowser. */
	if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_EXISTS)
		|| g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
	{
		error = g_error_new_literal (GTH_TASK_ERROR, GTH_TASK_ERROR_CANCELLED, "");
	}
	gth_task_completed (GTH_TASK (user_data), error);
}


static void
copy_dialog_cb (gboolean   opened,
		GtkWidget *dialog,
		gpointer   user_data)
{
	GthCopyTask *self = user_data;

	gth_task_dialog (GTH_TASK (self), opened, dialog);
}


static void
copy_progress_cb (GObject    *object,
		  const char *description,
		  const char *details,
		  gboolean    pulse,
		  double      fraction,
	   	  gpointer    user_data)
{
	GthCopyTask *self = user_data;

	gth_task_progress (GTH_TASK (self), description, details, pulse, fraction);
}


static void
gth_copy_task_exec (GthTask *task)
{
	GthCopyTask *self;

	g_return_if_fail (GTH_IS_COPY_TASK (task));

	self = GTH_COPY_TASK (task);

	gth_file_source_set_cancellable (self->priv->file_source, gth_task_get_cancellable (task));
	gth_file_source_copy (self->priv->file_source,
			      self->priv->destination,
			      self->priv->files,
			      self->priv->move,
			      self->priv->destination_position,
			      copy_progress_cb,
			      copy_dialog_cb,
			      copy_done_cb,
			      self);
}


static void
gth_copy_task_class_init (GthCopyTaskClass *klass)
{
	GObjectClass *object_class;
	GthTaskClass *task_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = gth_copy_task_finalize;

	task_class = GTH_TASK_CLASS (klass);
	task_class->exec = gth_copy_task_exec;
}


static void
gth_copy_task_init (GthCopyTask *self)
{
	self->priv = gth_copy_task_get_instance_private (self);
}


GthTask *
gth_copy_task_new (GthFileSource *file_source,
		   GthFileData   *destination,
		   gboolean       move,
		   GList         *files,
		   int            destination_position)
{
	GthCopyTask *self;

	self = GTH_COPY_TASK (g_object_new (GTH_TYPE_COPY_TASK, NULL));

	self->priv->file_source = g_object_ref (file_source);
	self->priv->destination = g_object_ref (destination);
	self->priv->move = move;
	self->priv->files = _g_object_list_ref (files);
	self->priv->destination_position = destination_position;

	return (GthTask *) self;
}
