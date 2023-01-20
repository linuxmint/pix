/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001-2009 The Free Software Foundation, Inc.
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
#include <string.h>
#include <glib/gi18n.h>
#define GDK_PIXBUF_ENABLE_BACKEND 1
#include "cairo-utils.h"
#include "gio-utils.h"
#include "glib-utils.h"
#include "gth-error.h"
#include "gth-hook.h"
#include "gth-main.h"
#include "gth-image-utils.h"
#include "gth-image-saver.h"
#include "pixbuf-io.h"
#include "pixbuf-utils.h"


#define LOAD_BUFFER_SIZE (64*1024)


typedef struct {
	int requested_size;
	int original_width;
	int original_height;
	int loader_width;
	int loader_height;
} ScaleData;


static GdkPixbuf *
load_from_stream (GdkPixbufLoader  *loader,
		  GInputStream     *stream,
		  int               requested_size,
		  GCancellable     *cancellable,
		  GError          **error)
{
	GdkPixbuf *pixbuf;
	gssize     n_read;
	guchar     buffer[LOAD_BUFFER_SIZE];
	gboolean   res;

	res = TRUE;
	while (1) {
		n_read = g_input_stream_read (stream,
					      buffer,
					      sizeof (buffer),
					      cancellable,
					      error);

		if (n_read < 0) {
			res = FALSE;
			error = NULL; /* Ignore further errors */
			break;
		}

		if (n_read == 0)
			break;

		if (! gdk_pixbuf_loader_write (loader,
					       buffer,
					       n_read,
					       error))
		{
			res = FALSE;
			error = NULL;
			break;
		}
	}

	if (! gdk_pixbuf_loader_close (loader, error)) {
		res = FALSE;
		error = NULL;
	}

	pixbuf = NULL;
	if (res) {
		pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
		if (pixbuf)
			g_object_ref (pixbuf);
	}

	return pixbuf;
}


static void
pixbuf_loader_size_prepared_cb (GdkPixbufLoader *loader,
				int              width,
				int              height,
				gpointer         user_data)
{
	ScaleData *scale_data = user_data;

	scale_data->original_width = width;
	scale_data->original_height = height;
	scale_data->loader_width = width;
	scale_data->loader_height = height;

	if (scale_data->requested_size == -1)
		return;

	if (scale_keeping_ratio (&scale_data->loader_width,
				 &scale_data->loader_height,
				 scale_data->requested_size,
				 scale_data->requested_size,
				 FALSE))
	{
		gdk_pixbuf_loader_set_size (loader,
					    scale_data->loader_width,
					    scale_data->loader_height);
	}
}


GthImage *
gth_pixbuf_new_from_file (GInputStream  *istream,
			  GthFileData   *file_data,
			  int            requested_size,
			  int           *original_width,
			  int           *original_height,
			  gboolean      *loaded_original,
			  gboolean       scale_to_original,
			  GCancellable  *cancellable,
			  GError       **error)
{
	ScaleData        scale_data;
	GdkPixbufLoader *pixbuf_loader;
	GdkPixbuf       *pixbuf;
	GthImage        *image;
	gboolean         original_size_rotated = FALSE;

	if (original_width != NULL)
		*original_width = -1;
	if (original_height != NULL)
		*original_height = -1;

	scale_data.requested_size = requested_size;
	scale_data.original_width = -1;
	scale_data.original_height = -1;
	scale_data.loader_width = -1;
	scale_data.loader_height = -1;

	pixbuf_loader = gdk_pixbuf_loader_new ();
	g_signal_connect (pixbuf_loader,
			  "size-prepared",
			  G_CALLBACK (pixbuf_loader_size_prepared_cb),
			  &scale_data);
	pixbuf = load_from_stream (pixbuf_loader, istream, requested_size, cancellable, error);

	g_object_unref (pixbuf_loader);

	if ((pixbuf != NULL) && scale_to_original) {
		GdkPixbuf *tmp;

		tmp = _gdk_pixbuf_scale_simple_safe (pixbuf, scale_data.original_width, scale_data.original_height, GDK_INTERP_NEAREST);
		g_object_unref (pixbuf);
		pixbuf = tmp;
	}

	if ((original_width != NULL) && (original_height != NULL)) {
		if (file_data != NULL) {
			char *path;

			path = g_file_get_path (file_data->file);
			if (path != NULL) {
				gdk_pixbuf_get_file_info (path, &scale_data.original_width, &scale_data.original_height);
				original_size_rotated = TRUE;
				g_free (path);
			}
		}
	}

	if (pixbuf != NULL) {
		GdkPixbuf *rotated;

		rotated = gdk_pixbuf_apply_embedded_orientation (pixbuf);
		if (rotated != NULL) {
			if (! original_size_rotated) {
				/* swap width and height */
				int tmp = scale_data.original_width;
				scale_data.original_width = scale_data.original_height;
				scale_data.original_height =tmp;
			}

			g_object_unref (pixbuf);
			pixbuf = rotated;
		}
	}

	image = gth_image_new ();
	if (pixbuf != NULL) {
		cairo_surface_t          *surface;
		cairo_surface_metadata_t *metadata;

		surface = _cairo_image_surface_create_from_pixbuf (pixbuf);
		metadata = _cairo_image_surface_get_metadata (surface);
		_cairo_metadata_set_has_alpha (metadata, gdk_pixbuf_get_has_alpha (pixbuf));
		_cairo_metadata_set_original_size (metadata, scale_data.original_width, scale_data.original_height);
		gth_image_set_cairo_surface (image, surface);
	}

	if (original_width != NULL)
		*original_width = scale_data.original_width;
	if (original_height != NULL)
		*original_height = scale_data.original_height;
	if (loaded_original != NULL)
		*loaded_original = (pixbuf != NULL) && (scale_data.original_width == gdk_pixbuf_get_width (pixbuf)) && (scale_data.original_height == gdk_pixbuf_get_height (pixbuf));

	_g_object_unref (pixbuf);

	return image;
}


GthImage *
gth_pixbuf_animation_new_from_file (GInputStream  *istream,
				    GthFileData   *file_data,
				    int            requested_size,
				    int           *original_width,
				    int           *original_height,
				    gboolean      *loaded_original,
				    gpointer       user_data,
				    GCancellable  *cancellable,
				    GError       **error)
{
	const char *mime_type;
	GthImage   *image;

	mime_type = _g_content_type_get_from_stream (istream, (file_data != NULL ? file_data->file : NULL), cancellable, error);
	if (mime_type == NULL)
		return NULL;

	image = NULL;
	if (g_content_type_equals (mime_type, "image/gif")) {
		GdkPixbufAnimation *animation;

		animation = gdk_pixbuf_animation_new_from_stream (istream, cancellable, error);
		image = gth_image_new ();
		gth_image_set_pixbuf_animation (image, animation);

		_g_object_unref (animation);
	}
	else
		image = gth_pixbuf_new_from_file (istream,
						  file_data,
						  requested_size,
						  original_width,
						  original_height,
						  loaded_original,
						  FALSE,
						  cancellable,
						  error);

	return image;
}
