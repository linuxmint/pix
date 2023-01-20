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

#include <config.h>
#include "gth-tag-task.h"


struct _GthTagTaskPrivate {
	GList         *file_list;
	GList         *file_data_list;
	GthStringList *tags;
};


G_DEFINE_TYPE_WITH_CODE (GthTagTask,
			 gth_tag_task,
			 GTH_TYPE_TASK,
			 G_ADD_PRIVATE (GthTagTask))


static void
gth_tag_task_finalize (GObject *object)
{
	GthTagTask *self;

	self = GTH_TAG_TASK (object);

	_g_object_unref (self->priv->tags);
	_g_object_list_unref (self->priv->file_list);
	_g_object_list_unref (self->priv->file_data_list);

	G_OBJECT_CLASS (gth_tag_task_parent_class)->finalize (object);
}


static void
write_metadata_ready_cb (GObject      *source_object,
		 	 GAsyncResult *result,
		 	 gpointer      user_data)
{
	GthTagTask *self = user_data;
	GthMonitor *monitor;
	GList      *scan;
	GError     *error = NULL;

	if (! _g_write_metadata_finish (result, &error)) {
		gth_task_completed (GTH_TASK (self), error);
		return;
	}

	monitor = gth_main_get_default_monitor ();
	for (scan = self->priv->file_data_list; scan; scan = scan->next) {
		GthFileData *file_data = scan->data;
		GFile       *parent;
		GList       *files;

		parent = g_file_get_parent (file_data->file);
		files = g_list_prepend (NULL, g_object_ref (file_data->file));
		gth_monitor_folder_changed (monitor, parent, files, GTH_MONITOR_EVENT_CHANGED);
		gth_monitor_metadata_changed (monitor, file_data);

		_g_object_list_unref (files);
		g_object_unref (parent);
	}

	gth_task_completed (GTH_TASK (self), NULL);
}


static void
info_ready_cb (GList    *files,
	       GError   *error,
	       gpointer  user_data)
{
	GthTagTask *self = user_data;
	GList      *scan;

	if (error != NULL) {
		gth_task_completed (GTH_TASK (self), error);
		return;
	}

	self->priv->file_data_list = _g_object_list_ref (files);
	for (scan = self->priv->file_data_list; scan; scan = scan->next) {
		GthFileData   *file_data = scan->data;
		GthMetadata   *metadata;
		GthStringList *original_tags;
		GthStringList *new_tags;

		metadata = (GthMetadata *) g_file_info_get_attribute_object (file_data->info, "general::tags");
		original_tags = gth_metadata_get_string_list (metadata);

		new_tags = gth_string_list_new (NULL);
		gth_string_list_append (new_tags, original_tags);
		gth_string_list_append (new_tags, self->priv->tags);

		metadata = gth_metadata_new_for_string_list (new_tags);
		g_file_info_set_attribute_object (file_data->info, "general::tags", G_OBJECT (metadata));

		g_object_unref (metadata);
		g_object_unref (new_tags);
	}

	gth_task_progress (GTH_TASK (self), _("Assigning tags to the selected files"), _("Writing files"), TRUE, 0.0);
	_g_write_metadata_async (self->priv->file_data_list,
				 GTH_METADATA_WRITE_DEFAULT,
			         "general::tags",
			         gth_task_get_cancellable (GTH_TASK (self)),
			         write_metadata_ready_cb,
			         self);
}


static void
gth_tag_task_exec (GthTask *task)
{
	GthTagTask *self;

	self = GTH_TAG_TASK (task);

	gth_task_progress (task, _("Assigning tags to the selected files"), _("Reading files"), TRUE, 0.0);
	_g_query_all_metadata_async (self->priv->file_list,
				     GTH_LIST_DEFAULT,
				     "*",
				     NULL,
				     info_ready_cb,
				     self);
}


static void
gth_tag_task_class_init (GthTagTaskClass *klass)
{
	GObjectClass *object_class;
	GthTaskClass *task_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = gth_tag_task_finalize;

	task_class = GTH_TASK_CLASS (klass);
	task_class->exec = gth_tag_task_exec;
}


static void
gth_tag_task_init (GthTagTask *self)
{
	self->priv = gth_tag_task_get_instance_private (self);
	self->priv->file_list = NULL;
	self->priv->file_data_list = NULL;
}


GthTask *
gth_tag_task_new (GList  *file_list,
		  char  **tags)
{
	GthTagTask *self;

	self = GTH_TAG_TASK (g_object_new (GTH_TYPE_TAG_TASK, NULL));
	self->priv->file_list = _g_object_list_ref (file_list);
	self->priv->tags = gth_string_list_new_from_strv (tags);

	return (GthTask *) self;
}
