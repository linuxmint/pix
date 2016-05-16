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

#include <glib.h>
#include "glib-utils.h"
#include "gth-async-task.h"
#include "typedefs.h"


#define PROGRESS_DELAY 100 /* delay between progress notifications */


/* Properties */
enum {
        PROP_0,
        PROP_BEFORE_THREAD,
        PROP_THREAD_FUNC,
        PROP_AFTER_THREAD,
        PROP_USER_DATA,
        PROP_USER_DATA_DESTROY_FUNC
};


struct _GthAsyncTaskPrivate {
	GthAsyncInitFunc    before_func;
	GthAsyncThreadFunc  exec_func;
	GthAsyncReadyFunc   after_func;
	gpointer            user_data;
	GDestroyNotify      user_data_destroy_func;
	GMutex              data_mutex;
	guint               progress_event;
	gboolean            cancelled;
	gboolean            terminated;
	double              progress;
};


G_DEFINE_TYPE (GthAsyncTask, gth_async_task, GTH_TYPE_TASK)


static void
gth_async_task_finalize (GObject *object)
{
	GthAsyncTask *self;

	g_return_if_fail (GTH_IS_ASYNC_TASK (object));
	self = GTH_ASYNC_TASK (object);

	if (self->priv->progress_event != 0) {
		g_source_remove (self->priv->progress_event);
		self->priv->progress_event = 0;
	}

	if ((self->priv->user_data != NULL) && (self->priv->user_data_destroy_func))
		(*self->priv->user_data_destroy_func) (self->priv->user_data);

	g_mutex_clear (&self->priv->data_mutex);

	G_OBJECT_CLASS (gth_async_task_parent_class)->finalize (object);
}


static void
task_completed (GError  *error,
		gpointer user_data)
{
	gth_task_completed (GTH_TASK (user_data), error);
}


static gboolean
update_progress (gpointer data)
{
	GthAsyncTask *self = data;
	gboolean      terminated;
	gboolean      cancelled;
	double        progress;

	g_mutex_lock (&self->priv->data_mutex);
	terminated = self->priv->terminated;
	cancelled = self->priv->cancelled;
	progress = self->priv->progress;
	g_mutex_unlock (&self->priv->data_mutex);

	if (terminated) {
		GError *error = NULL;

		g_source_remove (self->priv->progress_event);
		self->priv->progress_event = 0;

		if (cancelled)
			error = g_error_new_literal (GTH_TASK_ERROR, GTH_TASK_ERROR_CANCELLED, "");

		if (self->priv->after_func != NULL)
			self->priv->after_func (self, error, self->priv->user_data);

		ready_with_error (task_completed, self, error);

		return FALSE;
	}

	gth_task_progress (GTH_TASK (self),
			   NULL,
			   NULL,
			   FALSE,
			   progress);

	return TRUE;
}


static gpointer
exec_task (gpointer user_data)
{
	GthAsyncTask *self = user_data;
	gpointer      result;

	result = self->priv->exec_func (self, self->priv->user_data);

	g_mutex_lock (&self->priv->data_mutex);
	self->priv->terminated = TRUE;
	g_mutex_unlock (&self->priv->data_mutex);

	return result;
}


static void
gth_async_task_exec (GthTask *task)
{
	GthAsyncTask *self;

	self = GTH_ASYNC_TASK (task);

	g_mutex_lock (&self->priv->data_mutex);
	self->priv->terminated = FALSE;
	self->priv->cancelled = FALSE;
	g_mutex_unlock (&self->priv->data_mutex);

	if (self->priv->before_func != NULL)
		self->priv->before_func (self, self->priv->user_data);
	g_thread_new ("gth_async_task_exec", exec_task, self);

	if (self->priv->progress_event == 0)
		self->priv->progress_event = g_timeout_add (PROGRESS_DELAY, update_progress, self);
}


static void
gth_async_task_cancelled (GthTask *task)
{
	GthAsyncTask *self;

	g_return_if_fail (GTH_IS_ASYNC_TASK (task));

	self = GTH_ASYNC_TASK (task);

	g_mutex_lock (&self->priv->data_mutex);
	self->priv->cancelled = TRUE;
	g_mutex_unlock (&self->priv->data_mutex);
}


static void
gth_async_task_set_property (GObject      *object,
			     guint         property_id,
			     const GValue *value,
			     GParamSpec   *pspec)
{
	GthAsyncTask *self;

	self = GTH_ASYNC_TASK (object);

	switch (property_id) {
	case PROP_BEFORE_THREAD:
		self->priv->before_func = g_value_get_pointer (value);
		break;
	case PROP_THREAD_FUNC:
		self->priv->exec_func = g_value_get_pointer (value);
		break;
	case PROP_AFTER_THREAD:
		self->priv->after_func = g_value_get_pointer (value);
		break;
	case PROP_USER_DATA:
		self->priv->user_data = g_value_get_pointer (value);
		break;
	case PROP_USER_DATA_DESTROY_FUNC:
		self->priv->user_data_destroy_func = g_value_get_pointer (value);
		break;
	default:
		break;
	}
}


static void
gth_async_task_get_property (GObject    *object,
			     guint       property_id,
			     GValue     *value,
			     GParamSpec *pspec)
{
	GthAsyncTask *self;

	self = GTH_ASYNC_TASK (object);

	switch (property_id) {
	case PROP_BEFORE_THREAD:
		g_value_set_pointer (value, self->priv->before_func);
		break;
	case PROP_THREAD_FUNC:
		g_value_set_pointer (value, self->priv->exec_func);
		break;
	case PROP_AFTER_THREAD:
		g_value_set_pointer (value, self->priv->after_func);
		break;
	case PROP_USER_DATA:
		g_value_set_pointer (value, self->priv->user_data);
		break;
	case PROP_USER_DATA_DESTROY_FUNC:
		g_value_set_pointer (value, self->priv->user_data_destroy_func);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}


static void
gth_async_task_class_init (GthAsyncTaskClass *class)
{
	GObjectClass *object_class;
	GthTaskClass *task_class;

	g_type_class_add_private (class, sizeof (GthAsyncTaskPrivate));

	object_class = G_OBJECT_CLASS (class);
	object_class->set_property = gth_async_task_set_property;
	object_class->get_property = gth_async_task_get_property;
	object_class->finalize = gth_async_task_finalize;

	task_class = GTH_TASK_CLASS (class);
	task_class->exec = gth_async_task_exec;
	task_class->cancelled = gth_async_task_cancelled;

	g_object_class_install_property (object_class,
					 PROP_BEFORE_THREAD,
					 g_param_spec_pointer ("before-thread",
							       "Before",
							       "The function to execute before the thread",
							       G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_THREAD_FUNC,
					 g_param_spec_pointer ("thread-func",
							       "Function",
							       "The function to execute inside the thread",
							       G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_AFTER_THREAD,
					 g_param_spec_pointer ("after-thread",
							       "After",
							       "The function to execute after the thread",
							       G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_USER_DATA,
					 g_param_spec_pointer ("user-data",
							       "User Data",
							       "The extra data to pass to the functions",
							       G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_USER_DATA_DESTROY_FUNC,
					 g_param_spec_pointer ("user-data-destroy-func",
							       "User data destroy function",
							       "The optional function to free the user data when no longer needed",
							       G_PARAM_READWRITE));
}


static void
gth_async_task_init (GthAsyncTask *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_ASYNC_TASK, GthAsyncTaskPrivate);
	self->priv->cancelled = FALSE;
	self->priv->terminated = FALSE;
	self->priv->progress_event = 0;
	g_mutex_init (&self->priv->data_mutex);
	self->priv->user_data = NULL;
	self->priv->user_data_destroy_func = NULL;
}


GthTask *
gth_async_task_new (GthAsyncInitFunc    before_func,
		    GthAsyncThreadFunc  exec_func,
		    GthAsyncReadyFunc   after_func,
		    gpointer            user_data,
		    GDestroyNotify      user_data_destroy_func)
{
	return (GthTask *) g_object_new (GTH_TYPE_ASYNC_TASK,
				         "before-thread", before_func,
				         "thread-func", exec_func,
				         "after-thread", after_func,
				         "user-data", user_data,
				         "user-data-destroy-func", user_data_destroy_func,
				         NULL);
}


void
gth_async_task_set_data (GthAsyncTask *self,
			 gboolean     *terminated,
			 gboolean     *cancelled,
			 double       *progress)
{
	g_mutex_lock (&self->priv->data_mutex);
	if (terminated != NULL)
		self->priv->terminated = *terminated;
	if (cancelled != NULL)
		self->priv->cancelled = *cancelled;
	if (progress != NULL)
		self->priv->progress = *progress;
	g_mutex_unlock (&self->priv->data_mutex);
}


void
gth_async_task_get_data (GthAsyncTask *self,
			 gboolean     *terminated,
			 gboolean     *cancelled,
			 double       *progress)
{
	g_mutex_lock (&self->priv->data_mutex);
	if (terminated != NULL)
		*terminated = self->priv->terminated;
	if (cancelled != NULL)
		*cancelled = self->priv->cancelled;
	if (progress != NULL)
		*progress = self->priv->progress;
	g_mutex_unlock (&self->priv->data_mutex);
}
