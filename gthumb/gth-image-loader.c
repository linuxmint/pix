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
	GthImageLoaderFunc loader_func;
	gpointer           loader_data;
};


G_DEFINE_TYPE (GthImageLoader, gth_image_loader, G_TYPE_OBJECT)


static void
gth_image_loader_finalize (GObject *object)
{
	G_OBJECT_CLASS (gth_image_loader_parent_class)->finalize (object);
}


static void
gth_image_loader_class_init (GthImageLoaderClass *class)
{
	GObjectClass *object_class;

	g_type_class_add_private (class, sizeof (GthImageLoaderPrivate));

	object_class = G_OBJECT_CLASS (class);
	object_class->finalize = gth_image_loader_finalize;

}


static void
gth_image_loader_init (GthImageLoader *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_IMAGE_LOADER, GthImageLoaderPrivate);
	self->priv->as_animation = FALSE;
	self->priv->loader_func = NULL;
	self->priv->loader_data = NULL;
	self->priv->preferred_format = GTH_IMAGE_FORMAT_CAIRO_SURFACE;
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


void
gth_image_loader_set_preferred_format (GthImageLoader *self,
				       GthImageFormat  preferred_format)
{
	g_return_if_fail (self != NULL);
	self->priv->preferred_format = preferred_format;
}


typedef struct {
	GthFileData  *file_data;
	int           requested_size;
	GCancellable *cancellable;
	GthImage     *image;
	int           original_width;
	int           original_height;
} LoadData;


static LoadData *
load_data_new (GthFileData  *file_data,
	       int           requested_size,
	       GCancellable *cancellable)
{
	LoadData *load_data;

	load_data = g_new0 (LoadData, 1);
	load_data->file_data = _g_object_ref (file_data);
	load_data->requested_size = requested_size;
	load_data->cancellable = _g_object_ref (cancellable);

	return load_data;
}


static void
load_data_unref (LoadData *load_data)
{
	_g_object_unref (load_data->file_data);
	_g_object_unref (load_data->cancellable);
	_g_object_unref (load_data->image);
	g_free (load_data);
}


static void
load_pixbuf_thread (GSimpleAsyncResult *result,
		    GObject            *object,
		    GCancellable       *cancellable)
{
	GthImageLoader *self = GTH_IMAGE_LOADER (object);
	LoadData       *load_data;
	int             original_width;
	int             original_height;
	GInputStream   *istream;
	GthImage       *image = NULL;
	GError         *error = NULL;

	load_data = g_simple_async_result_get_op_res_gpointer (result);
	original_width = -1;
	original_height = -1;

	istream = (GInputStream *) g_file_read (load_data->file_data->file, cancellable, &error);
	if (istream == NULL) {
		g_simple_async_result_set_from_error (result, error);
		g_error_free (error);
		return;
	}

	if (self->priv->loader_func != NULL) {
		image = (*self->priv->loader_func) (istream,
						    load_data->file_data,
						    load_data->requested_size,
						    &original_width,
						    &original_height,
						    self->priv->loader_data,
						    cancellable,
						    &error);
	}
	else  {
		GthImageLoaderFunc  loader_func = NULL;
		const char         *mime_type;

		mime_type = _g_content_type_get_from_stream (istream, load_data->file_data->file, cancellable, NULL);
		if (mime_type != NULL)
			loader_func = gth_main_get_image_loader_func (mime_type, self->priv->preferred_format);

		if (loader_func != NULL)
			image = loader_func (istream,
					     load_data->file_data,
					     load_data->requested_size,
					     &original_width,
					     &original_height,
					     NULL,
					     cancellable,
					     &error);
		else
			error = g_error_new_literal (G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED, _("No suitable loader available for this file type"));
	}

	_g_object_unref (istream);

	if (error != NULL) {
		_g_object_unref (image);
		g_simple_async_result_set_from_error (result, error);
		g_error_free (error);
		return;
	}

	load_data->image = image;
	load_data->original_width = original_width;
	load_data->original_height = original_height;
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
	GSimpleAsyncResult *result;

	result = g_simple_async_result_new (G_OBJECT (loader),
					    callback,
					    user_data,
					    gth_image_loader_load);
	g_simple_async_result_set_op_res_gpointer (result,
						   load_data_new (file_data,
								  requested_size,
								  cancellable),
						   (GDestroyNotify) load_data_unref);
	g_simple_async_result_run_in_thread (result,
					     load_pixbuf_thread,
					     io_priority,
					     cancellable);

	g_object_unref (result);
}


gboolean
gth_image_loader_load_finish (GthImageLoader   *loader,
			      GAsyncResult     *result,
			      GthImage        **image,
			      int              *original_width,
			      int              *original_height,
			      GError         **error)
{
	  GSimpleAsyncResult *simple;
	  LoadData           *load_data;

	  g_return_val_if_fail (g_simple_async_result_is_valid (result, G_OBJECT (loader), gth_image_loader_load), FALSE);

	  simple = G_SIMPLE_ASYNC_RESULT (result);

	  if (g_simple_async_result_propagate_error (simple, error))
		  return FALSE;

	  load_data = g_simple_async_result_get_op_res_gpointer (simple);
	  if (image != NULL)
		  *image = _g_object_ref (load_data->image);
	  if (original_width != NULL)
	  	  *original_width = load_data->original_width;
	  if (original_height != NULL)
	  	  *original_height = load_data->original_height;

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
