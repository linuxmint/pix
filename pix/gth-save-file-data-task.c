/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2011 Free Software Foundation, Inc.
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
#include <glib/gi18n.h>
#include "glib-utils.h"
#include "gth-save-file-data-task.h"
#include "gth-metadata-provider.h"


struct _GthSaveFileDataTaskPrivate {
	GList *file_data_list;
	char  *attributes;
	int    n_files;
	GList *current;
	int    n_current;
};


G_DEFINE_TYPE_WITH_CODE (GthSaveFileDataTask,
			 gth_save_file_data_task,
			 GTH_TYPE_TASK,
			 G_ADD_PRIVATE (GthSaveFileDataTask))


static void
gth_save_file_data_task_finalize (GObject *object)
{
	GthSaveFileDataTask *self;

	self = GTH_SAVE_FILE_DATA_TASK (object);

	_g_object_list_unref (self->priv->file_data_list);
	g_free (self->priv->attributes);

	G_OBJECT_CLASS (gth_save_file_data_task_parent_class)->finalize (object);
}


static void save_current_file (GthSaveFileDataTask *self);


static void
save_next_file (GthSaveFileDataTask *self)
{
	self->priv->current = self->priv->current->next;
	self->priv->n_current++;
	save_current_file (self);
}


static void
write_metadata_ready_cb (GObject      *source_object,
			 GAsyncResult *result,
			 gpointer      user_data)
{
	GthSaveFileDataTask *self = user_data;
	GError              *error = NULL;

	if (! _g_write_metadata_finish (result, &error)) {
		gth_task_completed (GTH_TASK (self), error);
		return;
	}

	save_next_file (self);
}


static void
save_current_file (GthSaveFileDataTask *self)
{
	GFile *file;
	int    n_remaining;
	char  *details;
	GList *files;

	if (self->priv->current == NULL) {
		gth_task_completed (GTH_TASK (self), NULL);
		return;
	}

	file = self->priv->current->data;

	n_remaining = self->priv->n_files - self->priv->n_current;
	details = g_strdup_printf (g_dngettext (NULL, "%d file remaining", "%d files remaining", n_remaining), n_remaining);
	gth_task_progress (GTH_TASK (self),
			   _("Saving file information"),
			   details,
			   FALSE,
			   ((double) self->priv->n_current + 0.5) / self->priv->n_files);

	files = g_list_prepend (NULL, file);
	_g_write_metadata_async (files,
				 GTH_METADATA_WRITE_DEFAULT,
				 self->priv->attributes,
				 gth_task_get_cancellable (GTH_TASK (self)),
				 write_metadata_ready_cb,
				 self);

	g_list_free (files);
	g_free (details);
}


static void
gth_save_file_data_task_exec (GthTask *task)
{
	GthSaveFileDataTask *self;

	g_return_if_fail (GTH_IS_SAVE_FILE_DATA_TASK (task));

	self = GTH_SAVE_FILE_DATA_TASK (task);

	save_current_file (self);
}


static void
gth_save_file_data_task_class_init (GthSaveFileDataTaskClass *klass)
{
	GObjectClass *object_class;
	GthTaskClass *task_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = gth_save_file_data_task_finalize;

	task_class = GTH_TASK_CLASS (klass);
	task_class->exec = gth_save_file_data_task_exec;
}


static void
gth_save_file_data_task_init (GthSaveFileDataTask *self)
{
	self->priv = gth_save_file_data_task_get_instance_private (self);
	self->priv->file_data_list = NULL;
	self->priv->attributes = NULL;
}


GthTask *
gth_save_file_data_task_new (GList      *file_list,
			     const char *attributes)
{
	GthSaveFileDataTask *self;

	self = (GthSaveFileDataTask *) g_object_new (GTH_TYPE_SAVE_FILE_DATA_TASK, NULL);
	self->priv->file_data_list = gth_file_data_list_dup (file_list);
	self->priv->attributes = g_strdup (attributes);
	self->priv->n_files = g_list_length (self->priv->file_data_list);;
	self->priv->current = self->priv->file_data_list;
	self->priv->n_current = 0;

	return (GthTask *) self;
}
