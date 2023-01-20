/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001-2008 The Free Software Foundation, Inc.
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
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtk.h>
#include "glib-utils.h"
#include "gth-file-data.h"
#include "gth-image-loader.h"
#include "gth-main.h"


struct _GthImageLoaderPrivate {
	gboolean           as_animation;  /* Whether to load the image in a
				           * GdkPixbufAnimation structure. */
	GthImageFormat     preferred_format;
	GthICCProfile     *out_profile;
	GthImageLoaderFunc loader_func;
	gpointer           loader_data;
};


G_DEFINE_TYPE_WITH_CODE (GthImageLoader,
			 gth_image_loader,
			 G_TYPE_OBJECT,
			 G_ADD_PRIVATE (GthImageLoader))


static void
gth_image_loader_finalize (GObject *object)
{
	GthImageLoader *self = GTH_IMAGE_LOADER (object);

	_g_object_unref (self->priv->out_profile);

	G_OBJECT_CLASS (gth_image_loader_parent_class)->finalize (object);
}


static void
gth_image_loader_class_init (GthImageLoaderClass *class)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (class);
	object_class->finalize = gth_image_loader_finalize;

}


static void
gth_image_loader_init (GthImageLoader *self)
{
	self->priv = gth_image_loader_get_instance_private (self);
	self->priv->as_animation = FALSE;
	self->priv->loader_func = NULL;
	self->priv->loader_data = NULL;
	self->priv->preferred_format = GTH_IMAGE_FORMAT_CAIRO_SURFACE;
	self->priv->out_profile = NULL;
}


GthImageLoader *
gth_image_loader_new (GthImageLoaderFunc loader_func,
		      gpointer           loader_data)
{
	GthImageLoader *self;

	self = g_object_new (GTH_TYPE_IMAGE_LOADER, NULL);
	gth_image_loader_set_loader_func (self, loader_func, loader_data);

	return self;
}


void
gth_image_loader_set_loader_func (GthImageLoader     *self,
				  GthImageLoaderFunc  loader_func,
				  gpointer            loader_data)
{
	g_return_if_fail (self != NULL);

	self->priv->loader_func = loader_func;
	self->priv->loader_data = loader_data;
}


GthImageLoaderFunc
gth_image_loader_get_loader_func (GthImageLoader *self)
{
	return self->priv->loader_func;
}


void
gth_image_loader_set_preferred_format (GthImageLoader *self,
				       GthImageFormat  preferred_format)
{
	g_return_if_fail (self != NULL);
	self->priv->preferred_format = preferred_format;
}


void
gth_image_loader_set_out_profile (GthImageLoader *self,
				  GthICCProfile  *out_profile)
{
	g_return_if_fail (self != NULL);

	_g_object_ref (out_profile);
	_g_object_unref (self->priv->out_profile);
	self->priv->out_profile = out_profile;
}


/* -- gth_image_loader_load -- */


typedef struct {
	GthFileData  *file_data;
	int           requested_size;
	GthImage     *image;
	int           original_width;
	int           original_height;
} LoaderOptions;


static LoaderOptions *
loader_options_new (GthFileData  *file_data,
	            int           requested_size)
{
	LoaderOptions *load_data;

	load_data = g_new0 (LoaderOptions, 1);
	load_data->file_data = _g_object_ref (file_data);
	load_data->requested_size = requested_size;

	return load_data;
}


static void
loader_options_free (LoaderOptions *options)
{
	_g_object_unref (options->file_data);
	g_free (options);
}


typedef struct {
	GthImage *image;
	int       original_width;
	int       original_height;
	gboolean  loaded_original;
} LoaderResult;


static LoaderResult *
loader_result_new (void)
{
	LoaderResult *result;

	result = g_new0 (LoaderResult, 1);
	result->image = NULL;
	result->original_width = -1;
	result->original_height = -1;
	result->loaded_original = FALSE;

	return result;
}


static void
loader_result_free (LoaderResult *result)
{
	_g_object_unref (result->image);
	g_free (result);
}


static void
load_image_thread (GTask        *task,
                   gpointer      source_object,
		   gpointer      task_data,
		   GCancellable *cancellable)
{
	GthImageLoader *self = GTH_IMAGE_LOADER (source_object);
	LoaderOptions  *options;
	int             original_width;
	int             original_height;
	gboolean        loaded_original;
	GInputStream   *istream;
	GthImage       *image = NULL;
	GError         *error = NULL;
	LoaderResult   *result;

	options = g_task_get_task_data (task);
	original_width = -1;
	original_height = -1;

	istream = (GInputStream *) g_file_read (options->file_data->file, cancellable, &error);
	if (istream == NULL) {
		g_task_return_error (task, error);
		return;
	}

	loaded_original = TRUE;

	if (self->priv->loader_func != NULL) {
		image = (*self->priv->loader_func) (istream,
						    options->file_data,
						    options->requested_size,
						    &original_width,
						    &original_height,
						    &loaded_original,
						    self->priv->loader_data,
						    cancellable,
						    &error);
	}
	else  {
		GthImageLoaderFunc  loader_func = NULL;
		const char         *mime_type;

		mime_type = _g_content_type_get_from_stream (istream, options->file_data->file, cancellable, NULL);
		if (mime_type != NULL)
			loader_func = gth_main_get_image_loader_func (mime_type, self->priv->preferred_format);

		if (loader_func != NULL)
			image = loader_func (istream,
					     options->file_data,
					     options->requested_size,
					     &original_width,
					     &original_height,
					     &loaded_original,
					     NULL,
					     cancellable,
					     &error);
		else
			error = g_error_new_literal (G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED, _("No suitable loader available for this file type"));
	}

	_g_object_unref (istream);

	if ((image != NULL) && gth_image_get_is_null (image)) {
		_g_object_unref (image);
		if (error == NULL)
			error = g_error_new_literal (G_IO_ERROR, G_IO_ERROR_FAILED, "Error");
		g_task_return_error (task, error);
		return;
	}

	if ((image != NULL)
	    && ! g_cancellable_is_cancelled (cancellable)
	    && (self->priv->out_profile != NULL)
	    && gth_image_get_icc_profile (image) != NULL)
	{
		gth_image_apply_icc_profile (image, self->priv->out_profile, cancellable);
	}

	if (g_cancellable_is_cancelled (cancellable)) {
		_g_clear_object (&image);
		g_clear_error (&error);
		g_set_error_literal (&error, G_IO_ERROR, G_IO_ERROR_CANCELLED, "");
	}

	if (error != NULL) {
		if ((image == NULL) || gth_image_get_is_null (image)) {
			_g_object_unref (image);
			g_task_return_error (task, error);
			return;
		}

		/* ignore the error if the image is not null. */
		g_clear_error (&error);
	}

	result = loader_result_new ();
	result->image = image;
	result->original_width = original_width;
	result->original_height = original_height;
	result->loaded_original = loaded_original;
	g_task_return_pointer (task, result, (GDestroyNotify) loader_result_free);
}


void
gth_image_loader_load (GthImageLoader      *loader,
		       GthFileData         *file_data,
		       int                  requested_size,
		       int                  io_priority,
		       GCancellable        *cancellable,
		       GAsyncReadyCallback  callback,
		       gpointer             user_data)
{
	GTask *task;

	task = g_task_new (G_OBJECT (loader), cancellable, callback, user_data);
	g_task_set_task_data (task,
			      loader_options_new (file_data, requested_size),
			      (GDestroyNotify) loader_options_free);
	g_task_run_in_thread (task, load_image_thread);

	g_object_unref (task);
}


gboolean
gth_image_loader_load_finish (GthImageLoader   *loader,
			      GAsyncResult     *task,
			      GthImage        **image,
			      int              *original_width,
			      int              *original_height,
			      gboolean         *loaded_original,
			      GError         **error)
{
	  LoaderResult *result;

	  g_return_val_if_fail (g_task_is_valid (G_TASK (task), G_OBJECT (loader)), FALSE);

	  result = g_task_propagate_pointer (G_TASK (task), error);
	  if (result == NULL)
		  return FALSE;

	  if (image != NULL)
		  *image = _g_object_ref (result->image);
	  if (original_width != NULL)
	  	  *original_width = result->original_width;
	  if (original_height != NULL)
	  	  *original_height = result->original_height;
	  if (loaded_original != NULL)
	  	  *loaded_original = result->loaded_original;

	  loader_result_free (result);

	  return TRUE;
}


GthImage *
gth_image_new_from_stream (GInputStream  *istream,
			   int            requested_size,
			   int           *p_original_width,
			   int           *p_original_height,
			   GCancellable  *cancellable,
			   GError       **p_error)
{
	const char         *mime_type;
	GthImageLoaderFunc  loader_func;
	GthImage           *image;
	int                 original_width;
	int                 original_height;
	GError             *error = NULL;

	image = NULL;
	mime_type = _g_content_type_get_from_stream (istream, NULL, cancellable, &error);
	if (mime_type != NULL) {
		loader_func = gth_main_get_image_loader_func (mime_type, GTH_IMAGE_FORMAT_CAIRO_SURFACE);
		if (loader_func != NULL)
			image = loader_func (istream,
					     NULL,
					     requested_size,
					     &original_width,
					     &original_height,
					     NULL,
					     NULL,
					     cancellable,
					     &error);
	}

	if ((image == NULL) && (error == NULL))
		error = g_error_new_literal (G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED, _("No suitable loader available for this file type"));

	if (p_error != NULL)
		*p_error = error;
	else
		_g_error_free (error);

	if (p_original_width != NULL)
		*p_original_width = original_width;
	if (p_original_height != NULL)
		*p_original_height = original_height;

	return image;
}
