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

#include <glib.h>
#include "glib-utils.h"
#include "gth-image-task.h"
#include "gth-image-task-chain.h"


struct _GthImageTaskChainPrivate {
	GList	*tasks;
	GList	*current;
	gulong	 task_completed;
	gulong	 task_progress;
	gulong	 task_dialog;

};


G_DEFINE_TYPE_WITH_CODE (GthImageTaskChain,
			 gth_image_task_chain,
			 GTH_TYPE_TASK,
			 G_ADD_PRIVATE (GthImageTaskChain))


static void
gth_image_task_chain_finalize (GObject *object)
{
	GthImageTaskChain *self;

	g_return_if_fail (GTH_IS_IMAGE_TASK_CHAIN (object));

	self = GTH_IMAGE_TASK_CHAIN (object);
	_g_object_list_unref (self->priv->tasks);

	G_OBJECT_CLASS (gth_image_task_chain_parent_class)->finalize (object);
}


static void _gth_image_task_chain_exec_current_task (GthImageTaskChain *self);


static void
image_task_completed_cb (GthTask  *task,
			 GError   *error,
			 gpointer  user_data)
{
	GthImageTaskChain *self = user_data;

	g_signal_handler_disconnect (task, self->priv->task_completed);
	g_signal_handler_disconnect (task, self->priv->task_progress);
	g_signal_handler_disconnect (task, self->priv->task_dialog);

	if (error != NULL) {
		gth_task_completed (GTH_TASK (self), error);
		return;
	}

	self->priv->current = self->priv->current->next;
	_gth_image_task_chain_exec_current_task (self);
}


static void
image_task_progress_cb (GthTask    *task,
		        const char *description,
		        const char *details,
		        gboolean    pulse,
		        double      fraction,
		        gpointer    user_data)
{
	GthImageTaskChain *self = user_data;

	gth_task_progress (GTH_TASK (self),
			   NULL,
			   description,
			   pulse,
			   fraction);
}


static void
image_task_dialog_cb (GthTask   *task,
		      gboolean   opened,
		      GtkWidget *dialog,
		      gpointer   user_data)
{
	gth_task_dialog (GTH_TASK (user_data), opened, dialog);
}


static void
_gth_image_task_chain_exec_current_task (GthImageTaskChain *self)
{
	GthTask *task;

	if (self->priv->current == NULL) {
		gth_task_completed (GTH_TASK (self), NULL);
		return;
	}

	task = (GthTask *) self->priv->current->data;
	self->priv->task_completed =
			g_signal_connect (task,
				          "completed",
				          G_CALLBACK (image_task_completed_cb),
				          self);
	self->priv->task_progress =
			g_signal_connect (task,
					  "progress",
					  G_CALLBACK (image_task_progress_cb),
					  self);
	self->priv->task_dialog =
			g_signal_connect (task,
					  "dialog",
					  G_CALLBACK (image_task_dialog_cb),
					  self);

	if (self->priv->current != self->priv->tasks) {
		GthTask  *previous_task;
		GthImage *image;

		previous_task = (GthTask *) self->priv->current->prev->data;
		image = gth_image_task_get_destination (GTH_IMAGE_TASK (previous_task));
		gth_image_task_set_source (GTH_IMAGE_TASK (task), image);
	}

	gth_task_exec (task, gth_task_get_cancellable (GTH_TASK (self)));
}


static void
gth_image_task_chain_exec (GthTask *task)
{
	GthImageTaskChain *self;

	g_return_if_fail (GTH_IS_IMAGE_TASK_CHAIN (task));

	self = GTH_IMAGE_TASK_CHAIN (task);

	self->priv->current = self->priv->tasks;
	_gth_image_task_chain_exec_current_task (self);
}


static void
gth_image_task_chain_class_init (GthImageTaskChainClass *klass)
{
	GObjectClass *object_class;
	GthTaskClass *task_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = gth_image_task_chain_finalize;

	task_class = GTH_TASK_CLASS (klass);
	task_class->exec = gth_image_task_chain_exec;
}


static void
gth_image_task_chain_init (GthImageTaskChain *self)
{
	self->priv = gth_image_task_chain_get_instance_private (self);
	self->priv->tasks = NULL;
	self->priv->task_completed = 0;
	self->priv->task_dialog = 0;
	self->priv->task_progress = 0;
	gth_task_set_for_viewer (GTH_TASK (self), TRUE);
}


GthTask *
gth_image_task_chain_new (const char *description,
			  GthTask    *task,
			  ...)
{
	GthImageTaskChain *self;
	va_list            args;

	self = (GthImageTaskChain *) g_object_new (GTH_TYPE_IMAGE_TASK_CHAIN,
						   "description", description,
						   NULL);

	va_start (args, task);
	while (task != NULL) {
		self->priv->tasks = g_list_prepend (self->priv->tasks, task);
		task = va_arg (args, GthTask *);
	}
	va_end (args);
	self->priv->tasks = g_list_reverse (self->priv->tasks);

	return (GthTask *) self;
}
