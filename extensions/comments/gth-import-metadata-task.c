/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2012 Free Software Foundation, Inc.
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
#include "gth-comment.h"
#include "gth-import-metadata-task.h"
#include "preferences.h"


struct _GthImportMetadataTaskPrivate {
	GthBrowser    *browser;
	GList         *file_list;
	GList         *current;
	GthFileData   *file_data;
};


G_DEFINE_TYPE_WITH_CODE (GthImportMetadataTask,
			 gth_import_metadata_task,
			 GTH_TYPE_TASK,
			 G_ADD_PRIVATE (GthImportMetadataTask))


static void
gth_import_metadata_task_finalize (GObject *object)
{
	GthImportMetadataTask *self;

	self = GTH_IMPORT_METADATA_TASK (object);

	_g_object_unref (self->priv->file_data);
	_g_object_list_unref (self->priv->file_list);

	G_OBJECT_CLASS (gth_import_metadata_task_parent_class)->finalize (object);
}


static void import_current_file (GthImportMetadataTask *self);


static void
transform_next_file (GthImportMetadataTask *self)
{
	self->priv->current = self->priv->current->next;
	import_current_file (self);
}


static void
write_file_ready_cb (void     **buffer,
		     gsize      count,
		     GError    *error,
		     gpointer   user_data)
{
	GthImportMetadataTask *self = user_data;

	if (error != NULL) {
		gth_task_completed (GTH_TASK (self), error);
		return;
	}

	{
		GFile *file;
		GFile *parent;
		GList *list;

		file = self->priv->current->data;
		parent = g_file_get_parent (file);
		list = g_list_prepend (NULL, file);
		gth_monitor_folder_changed (gth_main_get_default_monitor (),
					    parent,
					    list,
					    GTH_MONITOR_EVENT_CHANGED);

		g_list_free (list);
		g_object_unref (parent);
	}

	transform_next_file (self);
}


static void
load_file_ready_cb (void     **buffer,
		    gsize      count,
		    GError    *error,
		    gpointer   user_data)
{
	GthImportMetadataTask *self = user_data;
	GFile                 *file;
	void                  *tmp_buffer;

	if (error != NULL) {
		gth_task_completed (GTH_TASK (self), error);
		return;
	}

	file = self->priv->current->data;

	tmp_buffer = *buffer;
	*buffer = NULL;

	gth_hook_invoke ("delete-metadata", file, &tmp_buffer, &count);

	_g_file_write_async (file,
			     tmp_buffer,
	    		     count,
	    		     TRUE,
	    		     G_PRIORITY_DEFAULT,
	    		     gth_task_get_cancellable (GTH_TASK (self)),
			     write_file_ready_cb,
			     self);
}


static void
import_current_file (GthImportMetadataTask *self)
{
	GFile *file;

	if (self->priv->current == NULL) {
		gth_task_completed (GTH_TASK (self), NULL);
		return;
	}

	file = self->priv->current->data;
	_g_file_load_async (file,
			    G_PRIORITY_DEFAULT,
			    gth_task_get_cancellable (GTH_TASK (self)),
			    load_file_ready_cb,
			    self);
}


static void
metadata_ready_cb (GObject      *source_object,
		   GAsyncResult *res,
		   gpointer      user_data)
{
	GthImportMetadataTask *self = user_data;
	GList                 *file_data_list;
	GError                *error = NULL;
	GSettings             *settings;
	gboolean               store_metadata_in_files;
	gboolean               synchronize;

	file_data_list = _g_query_metadata_finish (res, &error);
	if (error != NULL) {
		gth_task_completed (GTH_TASK (self), error);
		return;
	}

	settings = g_settings_new (GTHUMB_GENERAL_SCHEMA);
	store_metadata_in_files = g_settings_get_boolean (settings, PREF_GENERAL_STORE_METADATA_IN_FILES);
	g_object_unref (settings);

	settings = g_settings_new (GTHUMB_COMMENTS_SCHEMA);
	synchronize = g_settings_get_boolean (settings, PREF_COMMENTS_SYNCHRONIZE);
	g_object_unref (settings);

	/* Synchronization is done in _g_query_metadata_async if both
	 * PREF_GENERAL_STORE_METADATA_IN_FILES and PREF_COMMENTS_SYNCHRONIZE
	 * are true. */
	if (! store_metadata_in_files || ! synchronize) {
		GList *scan;

		for (scan = file_data_list; scan; scan = scan->next)
			gth_comment_update_from_general_attributes ((GthFileData *) scan->data);
	}

	gth_task_completed (GTH_TASK (self), NULL);
}


static void
gth_import_metadata_task_exec (GthTask *task)
{
	GthImportMetadataTask *self;

	g_return_if_fail (GTH_IS_IMPORT_METADATA_TASK (task));

	self = GTH_IMPORT_METADATA_TASK (task);

	_g_query_metadata_async (self->priv->file_list,
				 "*",
				 gth_task_get_cancellable (task),
				 metadata_ready_cb,
				 self);
}


static void
gth_import_metadata_task_class_init (GthImportMetadataTaskClass *klass)
{
	GObjectClass *object_class;
	GthTaskClass *task_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = gth_import_metadata_task_finalize;

	task_class = GTH_TASK_CLASS (klass);
	task_class->exec = gth_import_metadata_task_exec;
}


static void
gth_import_metadata_task_init (GthImportMetadataTask *self)
{
	self->priv = gth_import_metadata_task_get_instance_private (self);
	self->priv->file_data = NULL;
}


GthTask *
gth_import_metadata_task_new (GthBrowser *browser,
			      GList      *file_list)
{
	GthImportMetadataTask *self;

	self = GTH_IMPORT_METADATA_TASK (g_object_new (GTH_TYPE_IMPORT_METADATA_TASK, NULL));
	self->priv->browser = browser;
	self->priv->file_list = _g_object_list_ref (file_list);

	return (GthTask *) self;
}
