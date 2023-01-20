/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001, 2009, 2012 Free Software Foundation, Inc.
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


struct _GthImageTaskPrivate {
	GthImage *source;
	GthImage *destination;
};


G_DEFINE_TYPE_WITH_CODE (GthImageTask,
			 gth_image_task,
			 GTH_TYPE_ASYNC_TASK,
			 G_ADD_PRIVATE (GthImageTask))


static void
gth_image_task_finalize (GObject *object)
{
	GthImageTask *self;

	g_return_if_fail (GTH_IS_IMAGE_TASK (object));

	self = GTH_IMAGE_TASK (object);
	_g_object_unref (self->priv->source);
	_g_object_unref (self->priv->destination);

	G_OBJECT_CLASS (gth_image_task_parent_class)->finalize (object);
}


static void
gth_image_task_class_init (GthImageTaskClass *class)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (class);
	object_class->finalize = gth_image_task_finalize;
}


static void
gth_image_task_init (GthImageTask *self)
{
	self->priv = gth_image_task_get_instance_private (self);
	self->priv->source = NULL;
	self->priv->destination = NULL;
	gth_task_set_for_viewer (GTH_TASK (self), TRUE);
}


GthTask *
gth_image_task_new (const char         *description,
		    GthAsyncInitFunc    before_func,
		    GthAsyncThreadFunc  exec_func,
		    GthAsyncReadyFunc   after_func,
		    gpointer            user_data,
		    GDestroyNotify      user_data_destroy_func)
{
	GthImageTask *self;

	self = (GthImageTask *) g_object_new (GTH_TYPE_IMAGE_TASK,
					      "before-thread", before_func,
					      "thread-func", exec_func,
					      "after-thread", after_func,
					      "user-data", user_data,
					      "user-data-destroy-func", user_data_destroy_func,
					      "description", description,
					      NULL);

	return (GthTask *) self;
}


void
gth_image_task_set_source (GthImageTask *self,
			   GthImage     *source)
{
	_g_object_ref (source);
	_g_object_unref (self->priv->source);
	self->priv->source = source;
}


void
gth_image_task_set_source_surface (GthImageTask    *self,
				   cairo_surface_t *surface)
{
	GthImage *image;

	image = gth_image_new_for_surface (surface);
	gth_image_task_set_source (self, image);

	g_object_unref (image);
}


GthImage *
gth_image_task_get_source (GthImageTask *self)
{
	return self->priv->source;
}


cairo_surface_t *
gth_image_task_get_source_surface (GthImageTask *self)
{
	return gth_image_get_cairo_surface (self->priv->source);
}


void
gth_image_task_set_destination (GthImageTask *self,
				GthImage     *destination)
{
	_g_object_ref (destination);
	_g_object_unref (self->priv->destination);
	self->priv->destination = destination;
}


void
gth_image_task_set_destination_surface (GthImageTask    *self,
					cairo_surface_t *surface)
{
	GthImage *image;

	image = gth_image_new_for_surface (surface);
	gth_image_task_set_destination (self, image);

	g_object_unref (image);
}


GthImage *
gth_image_task_get_destination (GthImageTask *self)
{
	return self->priv->destination;
}


cairo_surface_t *
gth_image_task_get_destination_surface (GthImageTask *self)
{
	return gth_image_get_cairo_surface (self->priv->destination);
}


void
gth_image_task_copy_source_to_destination (GthImageTask *self)
{
	g_return_if_fail (self->priv->source != NULL);

	_g_object_unref (self->priv->destination);
	self->priv->destination = gth_image_copy (self->priv->source);
}
