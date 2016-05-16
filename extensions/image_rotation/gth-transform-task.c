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
#include "gth-transform-task.h"
#include "rotation-utils.h"


G_DEFINE_TYPE (GthTransformTask, gth_transform_task, GTH_TYPE_TASK)


struct _GthTransformTaskPrivate {
	GthBrowser    *browser;
	GList         *file_list;
	GList         *current;
	GthFileData   *file_data;
	GthTransform   transform;
	JpegMcuAction  default_action;
	int            n_image;
	int            n_images;
};


static void
gth_transform_task_finalize (GObject *object)
{
	GthTransformTask *self;

	self = GTH_TRANSFORM_TASK (object);

	_g_object_unref (self->priv->file_data);
	_g_object_list_unref (self->priv->file_list);

	G_OBJECT_CLASS (gth_transform_task_parent_class)->finalize (object);
}


static void transform_current_file (GthTransformTask *self);


static void
transform_next_file (GthTransformTask *self)
{
	self->priv->n_image++;
	/*self->priv->default_action = JPEG_MCU_ACTION_ABORT;*/
	self->priv->current = self->priv->current->next;
	transform_current_file (self);
}


static void
trim_response_cb (JpegMcuAction action,
		  gpointer      user_data)
{
	GthTransformTask *self = user_data;

	gth_task_dialog (GTH_TASK (self), FALSE, NULL);

	if (action != JPEG_MCU_ACTION_ABORT) {
		self->priv->default_action = action;
		transform_current_file (self);
	}
	else
		transform_next_file (self);
}


static void
transform_file_ready_cb (GError   *error,
			 gpointer  user_data)
{
	GthTransformTask *self = user_data;
	GFile            *parent;
	GList            *file_list;

	if (error != NULL) {
		if (g_error_matches (error, JPEG_ERROR, JPEG_ERROR_MCU)) {
			GtkWidget *dialog;

			g_clear_error (&error);

			dialog = ask_whether_to_trim (GTK_WINDOW (self->priv->browser),
						      self->priv->file_data,
						      trim_response_cb,
						      self);
			gth_task_dialog (GTH_TASK (self), TRUE, dialog);

			return;
		}

		gth_task_completed (GTH_TASK (self), error);
		return;
	}

	parent = g_file_get_parent (self->priv->file_data->file);
	file_list = g_list_append (NULL, self->priv->file_data->file);
	gth_monitor_folder_changed (gth_main_get_default_monitor (),
				    parent,
				    file_list,
				    GTH_MONITOR_EVENT_CHANGED);

	g_list_free (file_list);
	g_object_unref (parent);

	transform_next_file (self);
}


static void
file_info_ready_cb (GList    *files,
		    GError   *error,
		    gpointer  user_data)
{
	GthTransformTask *self = user_data;

	if (error != NULL) {
		gth_task_completed (GTH_TASK (self), error);
		return;
	}

	_g_object_unref (self->priv->file_data);
	self->priv->file_data = g_object_ref ((GthFileData *) files->data);

	gth_task_progress (GTH_TASK (self),
			   _("Saving images"),
			   g_file_info_get_display_name (self->priv->file_data->info),
			   FALSE,
			   (double) (self->priv->n_image + 1) / (self->priv->n_images + 1));

	apply_transformation_async (self->priv->file_data,
				    self->priv->transform,
				    self->priv->default_action,
				    gth_task_get_cancellable (GTH_TASK (self)),
				    transform_file_ready_cb,
				    self);
}


static void
transform_current_file (GthTransformTask *self)
{
	GFile *file;
	GList *singleton;

	if (self->priv->current == NULL) {
		gth_task_completed (GTH_TASK (self), NULL);
		return;
	}

	file = self->priv->current->data;
	singleton = g_list_append (NULL, g_object_ref (file));
	_g_query_all_metadata_async (singleton,
				     GTH_LIST_DEFAULT,
				     "*",
				     gth_task_get_cancellable (GTH_TASK (self)),
				     file_info_ready_cb,
				     self);

	_g_object_list_unref (singleton);
}


static void
gth_transform_task_exec (GthTask *task)
{
	GthTransformTask *self;

	g_return_if_fail (GTH_IS_TRANSFORM_TASK (task));

	self = GTH_TRANSFORM_TASK (task);

	self->priv->n_images = g_list_length (self->priv->file_list);
	self->priv->n_image = 0;
	self->priv->current = self->priv->file_list;
	transform_current_file (self);
}


static void
gth_transform_task_class_init (GthTransformTaskClass *klass)
{
	GObjectClass *object_class;
	GthTaskClass *task_class;

	g_type_class_add_private (klass, sizeof (GthTransformTaskPrivate));

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = gth_transform_task_finalize;

	task_class = GTH_TASK_CLASS (klass);
	task_class->exec = gth_transform_task_exec;
}


static void
gth_transform_task_init (GthTransformTask *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_TRANSFORM_TASK, GthTransformTaskPrivate);
	self->priv->default_action = JPEG_MCU_ACTION_ABORT;
	self->priv->file_data = NULL;
}


GthTask *
gth_transform_task_new (GthBrowser   *browser,
			GList        *file_list,
			GthTransform  transform)
{
	GthTransformTask *self;

	self = GTH_TRANSFORM_TASK (g_object_new (GTH_TYPE_TRANSFORM_TASK, NULL));
	self->priv->browser = browser;
	self->priv->file_list = _g_object_list_ref (file_list);
	self->priv->transform = transform;

	return (GthTask *) self;
}
