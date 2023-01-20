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
#include "gth-load-image-info-task.h"


struct _GthLoadImageInfoTaskPrivate {
	GthImageInfo   **images;
	int              n_images;
	int              current;
	char            *attributes;
	GthImageLoader  *loader;
};


G_DEFINE_TYPE_WITH_CODE (GthLoadImageInfoTask,
			 gth_load_image_info_task,
			 GTH_TYPE_TASK,
			 G_ADD_PRIVATE (GthLoadImageInfoTask))


static void
gth_load_image_info_task_finalize (GObject *object)
{
	GthLoadImageInfoTask *self;
	int                   i;

	self = GTH_LOAD_IMAGE_INFO_TASK (object);

	for (i = 0; i < self->priv->n_images; i++)
		gth_image_info_unref (self->priv->images[i]);
	g_free (self->priv->images);
	g_free (self->priv->attributes);
	g_object_unref (self->priv->loader);

	G_OBJECT_CLASS (gth_load_image_info_task_parent_class)->finalize (object);
}


static void load_current_image (GthLoadImageInfoTask *self);


static void
load_next_image (GthLoadImageInfoTask *self)
{
	self->priv->current++;
	load_current_image (self);
}


static void
metadata_ready_cb (GObject      *source_object,
                   GAsyncResult *result,
                   gpointer      user_data)
{
	GthLoadImageInfoTask *self = user_data;
	GError               *error = NULL;

	_g_query_metadata_finish (result, &error);
	if (error != NULL) {
		gth_task_completed (GTH_TASK (self), error);
		return;
	}

	load_next_image (self);
}


static void
continue_loading_image (GthLoadImageInfoTask *self)
{
	if (strcmp (self->priv->attributes, "") != 0) {
		GthImageInfo *image_info;
		GList        *files;

		image_info = self->priv->images[self->priv->current];
		files = g_list_prepend (NULL, image_info->file_data);
		_g_query_metadata_async (files,
					 self->priv->attributes,
					 gth_task_get_cancellable (GTH_TASK (self)),
					 metadata_ready_cb,
					 self);

		g_list_free (files);
	}
	else
		load_next_image (self);
}


static void
image_loader_ready_cb (GObject      *source_object,
                       GAsyncResult *result,
                       gpointer      user_data)
{
	GthLoadImageInfoTask *self = user_data;
	GthImageInfo         *image_info;
	GthImage             *image = NULL;
	GError               *error = NULL;

	gth_image_loader_load_finish (GTH_IMAGE_LOADER (source_object),
				      result,
				      &image,
				      NULL,
				      NULL,
				      NULL,
				      &error);

	if (error == NULL)
		g_cancellable_set_error_if_cancelled (gth_task_get_cancellable (GTH_TASK (self)), &error);

	if (error == NULL) {
		cairo_surface_t *surface;

		image_info = self->priv->images[self->priv->current];
		surface = gth_image_get_cairo_surface (image);
		if (surface != NULL) {
			gth_image_info_set_image  (image_info, surface);
			cairo_surface_destroy (surface);
		}
	}
	else if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
		g_object_unref (image);
		gth_task_completed (GTH_TASK (self), error);
		return;
	}
	else
		g_clear_error (&error);

	_g_object_unref (image);
	continue_loading_image (self);
}


static void
load_current_image (GthLoadImageInfoTask *self)
{
	GthImageInfo *image_info;
	char         *details;

	if (self->priv->current >= self->priv->n_images) {
		gth_task_completed (GTH_TASK (self), NULL);
		return;
	}

	image_info = self->priv->images[self->priv->current];

	/* translators: %s is a filename */
	details = g_strdup_printf (_("Loading “%s”"), g_file_info_get_display_name (image_info->file_data->info));
	gth_task_progress (GTH_TASK (self),
			   _("Loading images"),
			   details,
			   FALSE,
			   ((double) self->priv->current + 0.5) / self->priv->n_images);

	if (image_info->image == NULL)
		gth_image_loader_load (self->priv->loader,
				       image_info->file_data,
				       -1,
				       G_PRIORITY_DEFAULT,
				       gth_task_get_cancellable (GTH_TASK (self)),
				       image_loader_ready_cb,
				       self);
	else
		call_when_idle ((DataFunc) continue_loading_image, self);

	g_free (details);
}


static void
gth_load_image_info_task_exec (GthTask *task)
{
	GthLoadImageInfoTask *self;

	g_return_if_fail (GTH_IS_LOAD_IMAGE_INFO_TASK (task));

	self = GTH_LOAD_IMAGE_INFO_TASK (task);

	load_current_image (self);
}


static void
gth_load_image_info_task_cancelled (GthTask *task)
{
	/* FIXME */
}


static void
gth_load_image_info_task_class_init (GthLoadImageInfoTaskClass *klass)
{
	GObjectClass *object_class;
	GthTaskClass *task_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = gth_load_image_info_task_finalize;

	task_class = GTH_TASK_CLASS (klass);
	task_class->exec = gth_load_image_info_task_exec;
	task_class->cancelled = gth_load_image_info_task_cancelled;
}


static void
gth_load_image_info_task_init (GthLoadImageInfoTask *self)
{
	self->priv = gth_load_image_info_task_get_instance_private (self);
	self->priv->loader = gth_image_loader_new (NULL, NULL);
}


GthTask *
gth_load_image_info_task_new (GthImageInfo **images,
			      int            n_images,
			      const char    *attributes)
{
	GthLoadImageInfoTask *self;
	int                   n;

	self = (GthLoadImageInfoTask *) g_object_new (GTH_TYPE_LOAD_IMAGE_INFO_TASK, NULL);
	self->priv->images = g_new0 (GthImageInfo *, n_images + 1);
	for (n = 0; n < n_images; n++)
		self->priv->images[n] = gth_image_info_ref (images[n]);
	self->priv->images[n] = NULL;
	self->priv->n_images = n;
	self->priv->attributes = g_strdup (attributes);
	self->priv->current = 0;

	return (GthTask *) self;
}
