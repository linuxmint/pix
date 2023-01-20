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
#include <glib.h>
#include <gthumb.h>
#include <cairo.h>
#include <gtk/gtk.h>
#include <libraw.h>
#include "main.h"
#include "gth-metadata-provider-raw.h"


typedef enum {
	RAW_OUTPUT_COLOR_RAW = 0,
	RAW_OUTPUT_COLOR_SRGB = 1,
	RAW_OUTPUT_COLOR_ADOBE = 2,
	RAW_OUTPUT_COLOR_WIDE = 3,
	RAW_OUTPUT_COLOR_PROPHOTO = 4,
	RAW_OUTPUT_COLOR_XYZ = 5
} RawOutputColor;


static void
_libraw_set_gerror (GError **error,
		    int      error_code)
{
	g_set_error_literal (error,
			     G_IO_ERROR,
			     G_IO_ERROR_FAILED,
			     libraw_strerror (error_code));
}


static GthImage *
_libraw_read_jpeg_data (void           *buffer,
			gsize           buffer_size,
			int             requested_size,
			GCancellable   *cancellable,
			GError        **error)
{
	GthImageLoaderFunc  loader_func;
	GInputStream       *istream;
	GthImage           *image;

	loader_func = gth_main_get_image_loader_func ("image/jpeg", GTH_IMAGE_FORMAT_CAIRO_SURFACE);
	if (loader_func == NULL)
		return NULL;

	istream = g_memory_input_stream_new_from_data (buffer, buffer_size, NULL);
	if (istream == NULL)
		return NULL;

	image = loader_func (istream,
			     NULL,
			     requested_size,
			     NULL,
			     NULL,
			     NULL,
			     NULL,
			     cancellable,
			     error);

	g_object_unref (istream);

	return image;
}


static cairo_surface_t *
_cairo_surface_create_from_ppm (int     width,
				int     height,
				int     colors,
				int     bits,
				guchar *buffer,
				gsize   buffer_size)
{
	cairo_surface_t *surface;
	int              stride;
	guchar          *buffer_p;
	int              r, c;
	guchar          *row;
	guchar          *column;
	guint32          pixel;

	if (bits != 8)
		return NULL;

	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
	stride = cairo_image_surface_get_stride (surface);

	buffer_p = buffer;
	row = _cairo_image_surface_flush_and_get_data (surface);
	for (r = 0; r < height; r++) {
		column = row;
		for (c = 0; c < width; c++) {
			switch (colors) {
			case 4:
				pixel = CAIRO_RGBA_TO_UINT32 (buffer_p[0], buffer_p[1], buffer_p[2], buffer_p[3]);
				break;
			case 3:
				pixel = CAIRO_RGBA_TO_UINT32 (buffer_p[0], buffer_p[1], buffer_p[2], 0xff);
				break;
			case 1:
				pixel = CAIRO_RGBA_TO_UINT32 (buffer_p[0], buffer_p[0], buffer_p[0], 0xff);
				break;
			default:
				g_assert_not_reached ();
			}

			memcpy (column, &pixel, sizeof (guint32));

			column += 4;
			buffer_p += colors;
		}
		row += stride;
	}

	cairo_surface_mark_dirty (surface);

	return surface;
}


static GthImage *
_libraw_read_bitmap_data (int     width,
			  int     height,
			  int     colors,
			  int     bits,
			  guchar *buffer,
			  gsize   buffer_size)
{
	GthImage        *image = NULL;
	cairo_surface_t *surface = NULL;

	surface = _cairo_surface_create_from_ppm (width, height, colors, bits, buffer, buffer_size);
	if (surface != NULL) {
		image = gth_image_new_for_surface (surface);
		cairo_surface_destroy (surface);
	}

	return image;
}


static GthTransform
_libraw_get_tranform (libraw_data_t *raw_data)
{
	GthTransform transform;

	switch (raw_data->sizes.flip) {
	case 3:
		transform = GTH_TRANSFORM_ROTATE_180;
		break;
	case 5:
		transform = GTH_TRANSFORM_ROTATE_270;
		break;
	case 6:
		transform = GTH_TRANSFORM_ROTATE_90;
		break;
	default:
		transform = GTH_TRANSFORM_NONE;
		break;
	}

	return transform;
}


static int
_libraw_progress_cb (void                 *callback_data,
		     enum LibRaw_progress  stage,
		     int                   iteration,
		     int                   expected)
{
	return g_cancellable_is_cancelled ((GCancellable *) callback_data) ? 1 : 0;
}


static GthImage *
_cairo_image_surface_create_from_raw (GInputStream  *istream,
				      GthFileData   *file_data,
				      int            requested_size,
				      int           *original_width,
				      int           *original_height,
				      gboolean      *loaded_original,
				      gpointer       user_data,
				      GCancellable  *cancellable,
				      GError       **error)
{
	libraw_data_t *raw_data;
	int            result;
	void          *buffer = NULL;
	size_t         size;
	GthImage      *image = NULL;

	raw_data = libraw_init (LIBRAW_OPIONS_NO_MEMERR_CALLBACK | LIBRAW_OPIONS_NO_DATAERR_CALLBACK);
	if (raw_data == NULL) {
		_libraw_set_gerror (error, errno);
		goto fatal_error;
	}

	libraw_set_progress_handler (raw_data, _libraw_progress_cb, cancellable);

	if (! _g_input_stream_read_all (istream, &buffer, &size, cancellable, error))
		goto fatal_error;

	raw_data->params.output_tiff = FALSE;
	raw_data->params.use_camera_wb = TRUE;

#if LIBRAW_COMPILE_CHECK_VERSION_NOTLESS(0, 21)
	raw_data->rawparams.use_rawspeed = TRUE;
#else
	raw_data->params.use_rawspeed = TRUE;
#endif
	raw_data->params.highlight = FALSE;
	raw_data->params.use_camera_matrix = TRUE;
	raw_data->params.output_color = RAW_OUTPUT_COLOR_SRGB;
	raw_data->params.output_bps = 8;
	raw_data->params.half_size = (requested_size > 0);

	result = libraw_open_buffer (raw_data, buffer, size);
	if (LIBRAW_FATAL_ERROR (result)) {
		_libraw_set_gerror (error, result);
		goto fatal_error;
	}


	if (requested_size > 0) {

		if (loaded_original != NULL)
			*loaded_original = FALSE;

		/* read the thumbnail */

		result = libraw_unpack_thumb (raw_data);
		if (result != LIBRAW_SUCCESS) {
			_libraw_set_gerror (error, result);
			goto fatal_error;
		}

		switch (raw_data->thumbnail.tformat) {
		case LIBRAW_THUMBNAIL_JPEG:
			image = _libraw_read_jpeg_data (raw_data->thumbnail.thumb,
							raw_data->thumbnail.tlength,
							requested_size,
							cancellable,
							error);
			break;
		case LIBRAW_THUMBNAIL_BITMAP:
			if ((raw_data->thumbnail.tcolors > 0) && (raw_data->thumbnail.tcolors <= 4)) {
				image = _libraw_read_bitmap_data (raw_data->thumbnail.twidth,
								  raw_data->thumbnail.theight,
								  raw_data->thumbnail.tcolors,
								  8,
								  (guchar *) raw_data->thumbnail.thumb,
								  raw_data->thumbnail.tlength);
			}
			else
				g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA, "Unsupported data format");
			break;
		default:
			g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA, "Unsupported data format");
			break;
		}

		if ((image != NULL) && (_libraw_get_tranform (raw_data) != GTH_TRANSFORM_NONE)) {
			cairo_surface_t *surface;
			cairo_surface_t *rotated;

			surface = gth_image_get_cairo_surface (image);
			rotated = _cairo_image_surface_transform (surface, _libraw_get_tranform (raw_data));
			gth_image_set_cairo_surface (image, rotated);

			cairo_surface_destroy (rotated);
			cairo_surface_destroy (surface);
		}

		/* get the original size */

		if ((original_width != NULL) && (original_height != NULL)) {
			libraw_close (raw_data);

			raw_data = libraw_init (LIBRAW_OPIONS_NO_MEMERR_CALLBACK | LIBRAW_OPIONS_NO_DATAERR_CALLBACK);
			if (raw_data == NULL)
				goto fatal_error;

			result = libraw_open_buffer (raw_data, buffer, size);
			if (LIBRAW_FATAL_ERROR (result))
				goto fatal_error;

			result = libraw_unpack (raw_data);
			if (result != LIBRAW_SUCCESS) {
				_libraw_set_gerror (error, result);
				goto fatal_error;
			}

			result = libraw_adjust_sizes_info_only (raw_data);
			if (result != LIBRAW_SUCCESS) {
				_libraw_set_gerror (error, result);
				goto fatal_error;
			}

			*original_width = raw_data->sizes.iwidth;
			*original_height = raw_data->sizes.iheight;
		}
	} else {
		/* read the image */

		libraw_processed_image_t *processed_image;

		result = libraw_unpack (raw_data);
		if (result != LIBRAW_SUCCESS) {
			_libraw_set_gerror (error, result);
			goto fatal_error;
		}

		result = libraw_dcraw_process (raw_data);
		if (result != LIBRAW_SUCCESS) {
			_libraw_set_gerror (error, result);
			goto fatal_error;
		}

		processed_image = libraw_dcraw_make_mem_image (raw_data, &result);
		if (result != LIBRAW_SUCCESS) {
			_libraw_set_gerror (error, result);
			goto fatal_error;
		}

		switch (processed_image->type) {
		case LIBRAW_IMAGE_JPEG:
			image = _libraw_read_jpeg_data (processed_image->data,
							processed_image->data_size,
							-1,
							cancellable,
							error);
			break;
		case LIBRAW_IMAGE_BITMAP:
			image = _libraw_read_bitmap_data (processed_image->width,
							  processed_image->height,
							  processed_image->colors,
							  processed_image->bits,
							  processed_image->data,
							  processed_image->data_size);
			break;
		default:
			g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA, "Unsupported data format");
			break;
		}

		libraw_dcraw_clear_mem (processed_image);

		/* get the original size */

		if ((original_width != NULL) && (original_height != NULL)) {
			result = libraw_adjust_sizes_info_only (raw_data);
			if (result != LIBRAW_SUCCESS) {
				_libraw_set_gerror (error, result);
				goto fatal_error;
			}

			*original_width = raw_data->sizes.iwidth;
			*original_height = raw_data->sizes.iheight;
		}
	}

	fatal_error:

	if (raw_data != NULL)
		libraw_close (raw_data);
	g_free (buffer);

	return image;
}


G_MODULE_EXPORT void
gthumb_extension_activate (void)
{
	GList *mime_types;
	mime_types = g_content_types_get_registered ();

	GList *l = mime_types;
	while (l != NULL) {
		GList *next = l->next;
		if (!_g_mime_type_is_raw (l->data)) {
			g_free (l->data);
			mime_types = g_list_delete_link (mime_types, l);
		}
		l = next;
	}

	int count_of_raw_types, i;
	count_of_raw_types = g_list_length (mime_types);

	gchar *raw_mime_types[count_of_raw_types+1];

	i = 0;
	l = mime_types;
	while (l != NULL) {
		GList *next = l->next;
		raw_mime_types[i] = (gchar *) l->data;
		i++;
		l = next;
	}
	raw_mime_types[i] = NULL;

	gth_main_register_metadata_provider (GTH_TYPE_METADATA_PROVIDER_RAW);
	gth_main_register_image_loader_func_v (_cairo_image_surface_create_from_raw,
					       GTH_IMAGE_FORMAT_CAIRO_SURFACE,
					       (const gchar **) raw_mime_types);

	g_list_free_full (mime_types, g_free);
}


G_MODULE_EXPORT void
gthumb_extension_deactivate (void)
{
}


G_MODULE_EXPORT gboolean
gthumb_extension_is_configurable (void)
{
	return FALSE;
}


G_MODULE_EXPORT void
gthumb_extension_configure (GtkWindow *parent)
{
}
