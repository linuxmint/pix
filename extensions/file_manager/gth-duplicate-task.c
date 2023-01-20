/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001-2009 Free Software Foundation, Inc.
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
#include "gth-duplicate-task.h"


struct _GthDuplicateTaskPrivate {
	GList *file_list;
	GList *current;
	GFile *destination;
};


G_DEFINE_TYPE_WITH_CODE (GthDuplicateTask,
			 gth_duplicate_task,
			 GTH_TYPE_TASK,
			 G_ADD_PRIVATE (GthDuplicateTask))


static void
gth_duplicate_task_finalize (GObject *object)
{
	GthDuplicateTask *self;

	self = GTH_DUPLICATE_TASK (object);

	_g_object_list_unref (self->priv->file_list);
	_g_object_unref (self->priv->destination);

	G_OBJECT_CLASS (gth_duplicate_task_parent_class)->finalize (object);
}


static void
copy_progress_cb (GObject    *object,
		  const char *description,
		  const char *details,
		  gboolean    pulse,
		  double      fraction,
		  gpointer    user_data)
{
	GthDuplicateTask *self = user_data;

	gth_task_progress (GTH_TASK (self), description, details, pulse, fraction);
}


static void
copy_dialog_cb (gboolean   opened,
		GtkWidget *dialog,
		gpointer   user_data)
{
	GthDuplicateTask *self = user_data;

	gth_task_dialog (GTH_TASK (self), opened, dialog);
}


static void duplicate_current_file (GthDuplicateTask *self);


static void
copy_ready_cb (GthOverwriteResponse  response,
	       GList                *other_files,
	       GError               *error,
               gpointer              user_data)
{
	GthDuplicateTask *self = user_data;

	if (error != NULL) {
		if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_EXISTS)) {
			g_clear_error (&error);
			duplicate_current_file (self);
		}
		else
			gth_task_completed (GTH_TASK (self), error);
		return;
	}

	self->priv->current = self->priv->current->next;
	_g_clear_object (&self->priv->destination);
	duplicate_current_file (self);
}


static void
duplicate_current_file (GthDuplicateTask *self)
{
	GthFileData *file_data;

	if (self->priv->current == NULL) {
		gth_task_completed (GTH_TASK (self), NULL);
		return;
	}

	file_data = self->priv->current->data;
	if (self->priv->destination == NULL) {
		self->priv->destination = _g_file_get_duplicated (file_data->file);
	}
	else {
		GFile *tmp = self->priv->destination;
		self->priv->destination = _g_file_get_duplicated (tmp);
		g_object_unref (tmp);
	}

	_gth_file_data_copy_async (file_data,
				   self->priv->destination,
				   FALSE,
				   GTH_FILE_COPY_ALL_METADATA,
				   GTH_OVERWRITE_RESPONSE_ALWAYS_NO,
				   G_PRIORITY_DEFAULT,
				   gth_task_get_cancellable (GTH_TASK (self)),
				   copy_progress_cb,
				   self,
				   copy_dialog_cb,
				   self,
				   copy_ready_cb,
				   self);
}


static void
gth_duplicate_task_exec (GthTask *task)
{
	GthDuplicateTask *self;

	g_return_if_fail (GTH_IS_DUPLICATE_TASK (task));

	self = GTH_DUPLICATE_TASK (task);

	self->priv->current = self->priv->file_list;
	duplicate_current_file (self);
}


static void
gth_duplicate_task_class_init (GthDuplicateTaskClass *klass)
{
	GObjectClass *object_class;
	GthTaskClass *task_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = gth_duplicate_task_finalize;

	task_class = GTH_TASK_CLASS (klass);
	task_class->exec = gth_duplicate_task_exec;
}


static void
gth_duplicate_task_init (GthDuplicateTask *self)
{
	self->priv = gth_duplicate_task_get_instance_private (self);
	self->priv->destination = NULL;
}


GthTask *
gth_duplicate_task_new (GList *file_list)
{
	GthDuplicateTask *self;

	self = GTH_DUPLICATE_TASK (g_object_new (GTH_TYPE_DUPLICATE_TASK, NULL));
	self->priv->file_list = _g_object_list_ref (file_list);

	return (GthTask *) self;
}
