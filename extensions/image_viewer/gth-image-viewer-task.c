/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2013 Free Software Foundation, Inc.
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
#include "gth-image-viewer-page.h"
#include "gth-image-viewer-task.h"


struct _GthImageViewerTaskPrivate {
	GthImageViewerPage	*viewer_page;
	GthTask			*original_image_task;
	gboolean		 load_original;
	gboolean		 loading_image;
};


G_DEFINE_TYPE_WITH_CODE (GthImageViewerTask,
			 gth_image_viewer_task,
			 GTH_TYPE_IMAGE_TASK,
			 G_ADD_PRIVATE (GthImageViewerTask))


static void
gth_image_viewer_task_finalize (GObject *object)
{
	GthImageViewerTask *self;

	g_return_if_fail (GTH_IS_IMAGE_VIEWER_TASK (object));

	self = GTH_IMAGE_VIEWER_TASK (object);
	_g_object_unref (self->priv->original_image_task);
	_g_object_unref (self->priv->viewer_page);

	G_OBJECT_CLASS (gth_image_viewer_task_parent_class)->finalize (object);
}


static void
original_image_task_completed_cb (GthTask	*task,
				  GError	*error,
				  gpointer	 user_data)
{
	GthImageViewerTask *self = user_data;
	cairo_surface_t    *image;

	if (error != NULL) {
		gth_task_completed (task, error);
		return;
	}

	image = gth_original_image_task_get_image (task);
	gth_image_task_set_source_surface (GTH_IMAGE_TASK (self), image);
	gth_task_progress (GTH_TASK (self), NULL, "", TRUE, 0.0);
	GTH_TASK_CLASS (gth_image_viewer_task_parent_class)->exec (GTH_TASK (self));

	cairo_surface_destroy (image);
}


static void
original_image_task_progress_cb (GthTask    *task,
				 const char *description,
				 const char *details,
				 gboolean    pulse,
				 double      fraction,
				 gpointer    user_data)
{
	GthImageViewerTask *self = user_data;
	gth_task_progress (GTH_TASK (self), NULL, description, pulse, fraction);
}


static void
gth_image_viewer_task_exec (GthTask *task)
{
	GthImageViewerTask *self;

	self = GTH_IMAGE_VIEWER_TASK (task);

	if (self->priv->load_original) {
		self->priv->original_image_task = gth_original_image_task_new (self->priv->viewer_page);
		g_signal_connect (self->priv->original_image_task,
				  "completed",
				  G_CALLBACK (original_image_task_completed_cb),
				  self);
		g_signal_connect (self->priv->original_image_task,
				  "progress",
				  G_CALLBACK (original_image_task_progress_cb),
				  self);

		gth_task_exec (self->priv->original_image_task, gth_task_get_cancellable (GTH_TASK (self)));

		return;
	}

	GTH_TASK_CLASS (gth_image_viewer_task_parent_class)->exec (GTH_TASK (self));
}


static void
gth_image_viewer_task_class_init (GthImageViewerTaskClass *class)
{
	GObjectClass *object_class;
	GthTaskClass *task_class;

	object_class = G_OBJECT_CLASS (class);
	object_class->finalize = gth_image_viewer_task_finalize;

	task_class = GTH_TASK_CLASS (class);
	task_class->exec = gth_image_viewer_task_exec;
}


static void
gth_image_viewer_task_init (GthImageViewerTask *self)
{
	self->priv = gth_image_viewer_task_get_instance_private (self);
	self->priv->viewer_page = NULL;
	self->priv->original_image_task = NULL;
	self->priv->load_original = TRUE;
	self->priv->loading_image = FALSE;
	gth_task_set_for_viewer (GTH_TASK (self), TRUE);
}


GthTask *
gth_image_viewer_task_new (GthImageViewerPage *viewer_page,
			   const char         *description,
			   GthAsyncInitFunc    before_func,
			   GthAsyncThreadFunc  exec_func,
			   GthAsyncReadyFunc   after_func,
			   gpointer            user_data,
			   GDestroyNotify      user_data_destroy_func)
{
	GthImageViewerTask *self;

	g_return_val_if_fail (viewer_page != NULL, NULL);

	self = (GthImageViewerTask *) g_object_new (GTH_TYPE_IMAGE_VIEWER_TASK,
						    "before-thread", before_func,
						    "thread-func", exec_func,
						    "after-thread", after_func,
						    "user-data", user_data,
						    "user-data-destroy-func", user_data_destroy_func,
						    "description", description,
						    NULL);
	self->priv->viewer_page = g_object_ref (viewer_page);

	return (GthTask *) self;
}


void
gth_image_viewer_task_set_load_original (GthImageViewerTask *self,
					 gboolean            load_original)
{
	self->priv->load_original = load_original;
}


void
gth_image_viewer_task_set_destination  (GthTask  *task,
					GError   *error,
					gpointer  user_data)
{
	cairo_surface_t *destination;

	if (error != NULL) {
		g_object_unref (task);
		return;
	}

	destination = gth_image_task_get_destination_surface (GTH_IMAGE_TASK (task));
	if (destination == NULL) {
		g_object_unref (task);
		return;
	}

	gth_image_viewer_page_set_image (GTH_IMAGE_VIEWER_TASK (task)->priv->viewer_page, destination, TRUE);

	cairo_surface_destroy (destination);
	g_object_unref (task);
}
