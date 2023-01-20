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
#include <tiff.h>
#include <tiffio.h>
#include "cairo-image-surface-tiff.h"


#define HANDLE(x) ((Handle *) (x))


typedef struct {
	GInputStream *istream;
	GCancellable *cancellable;
	goffset       size;
} Handle;


static tsize_t
tiff_read (thandle_t handle, tdata_t buf, tsize_t size)
{
	Handle *h = HANDLE (handle);
	return g_input_stream_read (G_INPUT_STREAM (h->istream), buf, size, h->cancellable, NULL);
}


static tsize_t
tiff_write (thandle_t handle, tdata_t buf, tsize_t size)
{
        return -1;
}


static toff_t
tiff_seek (thandle_t handle, toff_t offset, int whence)
{
	Handle    *h = HANDLE (handle);
	GSeekType  seek_type;

	seek_type = G_SEEK_SET;
        switch (whence) {
        case SEEK_SET:
        	seek_type = G_SEEK_SET;
                break;
        case SEEK_CUR:
        	seek_type = G_SEEK_CUR;
                break;
        case SEEK_END:
        	seek_type = G_SEEK_END;
                break;
        }

	if (! g_seekable_seek (G_SEEKABLE (h->istream), offset, seek_type, h->cancellable, NULL))
		return -1;

	return g_seekable_tell (G_SEEKABLE (h->istream));
}


static int
tiff_close (thandle_t context)
{
        return 0;
}


static toff_t
tiff_size (thandle_t handle)
{
	return (toff_t) HANDLE (handle)->size;
}


static void
tiff_error_handler (const char *module, const char *fmt, va_list ap)
{
	/* do not print warnings and errors */
}


GthImage *
_cairo_image_surface_create_from_tiff (GInputStream  *istream,
				       GthFileData   *file_data,
				       int            requested_size,
				       int           *original_width_p,
				       int           *original_height_p,
				       gboolean      *loaded_original_p,
				       gpointer       user_data,
				       GCancellable  *cancellable,
				       GError       **error)
{
	GthImage		*image;
	Handle			 handle;
	TIFF			*tif;
	gboolean		 first_directory;
	int			 best_directory;
	int        		 max_width, max_height, min_diff;
	uint32			 image_width;
	uint32			 image_height;
	uint32			 spp;
	uint16			 extrasamples;
	uint16			*sampleinfo;
	uint16			 orientation;
	char			 emsg[1024];
	cairo_surface_t		*surface;
	cairo_surface_metadata_t*metadata;
	uint32			*raster;

	image = gth_image_new ();
	handle.cancellable = cancellable;
	handle.size = 0;

	if ((file_data != NULL) && (file_data->info != NULL)) {
		handle.istream = g_buffered_input_stream_new (istream);
		handle.size = g_file_info_get_size (file_data->info);
	}
	else {
		void  *data;
		gsize  size;

		/* read the whole stream to get the file size */

		if (! _g_input_stream_read_all (istream, &data, &size, cancellable, error))
			return image;
		handle.istream = g_memory_input_stream_new_from_data (data, size, g_free);
		handle.size = size;
	}


	TIFFSetErrorHandler (tiff_error_handler);
	TIFFSetWarningHandler (tiff_error_handler);

	tif = TIFFClientOpen ("gth-tiff-reader", "r",
			      &handle,
	                      tiff_read,
	                      tiff_write,
	                      tiff_seek,
	                      tiff_close,
	                      tiff_size,
	                      NULL,
	                      NULL);

	if (tif == NULL) {
		g_object_unref (handle.istream);
		g_set_error_literal (error,
				     GDK_PIXBUF_ERROR,
				     GDK_PIXBUF_ERROR_INSUFFICIENT_MEMORY,
				     "Couldn't allocate memory for writing TIFF file");
		return image;
	}

	/* find the best image to load */

	first_directory = TRUE;
	best_directory = -1;
	max_width = -1;
	max_height = -1;
	min_diff = 0;
	do {
		int width;
		int height;

		if (TIFFGetField (tif, TIFFTAG_IMAGEWIDTH, &width) != 1)
			continue;
		if (TIFFGetField (tif, TIFFTAG_IMAGELENGTH, &height) != 1)
			continue;

		if (! TIFFRGBAImageOK (tif, emsg))
			continue;

		if (width > max_width) {
			max_width = width;
			max_height = height;
			if (requested_size <= 0)
				best_directory = TIFFCurrentDirectory (tif);
		}

		if (requested_size > 0) {
			int diff = abs (requested_size - width);

			if (first_directory) {
				min_diff = diff;
				best_directory = TIFFCurrentDirectory (tif);
			}
			else if (diff < min_diff) {
				min_diff = diff;
				best_directory = TIFFCurrentDirectory (tif);
			}
		}

		first_directory = FALSE;
	}
	while (TIFFReadDirectory (tif));

	if (best_directory == -1) {
		TIFFClose (tif);
		g_object_unref (handle.istream);
		g_set_error_literal (error,
				     G_IO_ERROR,
				     G_IO_ERROR_INVALID_DATA,
				     "Invalid TIFF format");
		return image;
	}

	/* read the image */

	TIFFSetDirectory (tif, best_directory);
	TIFFGetField (tif, TIFFTAG_IMAGEWIDTH, &image_width);
	TIFFGetField (tif, TIFFTAG_IMAGELENGTH, &image_height);
	TIFFGetField (tif, TIFFTAG_SAMPLESPERPIXEL, &spp);
	TIFFGetFieldDefaulted (tif, TIFFTAG_EXTRASAMPLES, &extrasamples, &sampleinfo);
	if (TIFFGetFieldDefaulted (tif, TIFFTAG_ORIENTATION, &orientation) != 1)
		orientation = ORIENTATION_TOPLEFT;

	if (original_width_p)
		*original_width_p = max_width;
	if (original_height_p)
		*original_height_p = max_height;
	if (loaded_original_p)
		*loaded_original_p = (max_width == image_width);

	surface = _cairo_image_surface_create (CAIRO_FORMAT_ARGB32, image_width, image_height);
	if (surface == NULL) {
		TIFFClose (tif);
		g_object_unref (handle.istream);
		g_set_error_literal (error,
				     GDK_PIXBUF_ERROR,
				     GDK_PIXBUF_ERROR_INSUFFICIENT_MEMORY,
				     "Couldn't allocate memory for writing TIFF file");
		return image;
	}

	metadata = _cairo_image_surface_get_metadata (surface);
	_cairo_metadata_set_has_alpha (metadata, (extrasamples == 1) || (spp == 4));
	_cairo_metadata_set_original_size (metadata, max_width, max_height);

	raster = (uint32*) _TIFFmalloc (image_width * image_height * sizeof (uint32));
	if (raster == NULL) {
		cairo_surface_destroy (surface);
		TIFFClose (tif);
		g_object_unref (handle.istream);
		g_set_error_literal (error,
				     GDK_PIXBUF_ERROR,
				     GDK_PIXBUF_ERROR_INSUFFICIENT_MEMORY,
				     "Couldn't allocate memory for writing TIFF file");
		return image;
	}

	if (TIFFReadRGBAImageOriented (tif, image_width, image_height, raster, orientation, 0)) {
		guchar *surface_row;
		int     line_step;
		int     x, y, temp;
		guchar  r, g, b, a;
		uint32 *src_pixel;

		surface_row = _cairo_image_surface_flush_and_get_data (surface);
		line_step = cairo_image_surface_get_stride (surface);
		src_pixel = raster;
		for (y = 0; y < image_height; y++) {
			guchar *dest_pixel = surface_row;

			if (g_cancellable_is_cancelled (cancellable))
				goto stop_loading;

			for (x = 0; x < image_width; x++) {
				r = TIFFGetR (*src_pixel);
				g = TIFFGetG (*src_pixel);
				b = TIFFGetB (*src_pixel);
				a = TIFFGetA (*src_pixel);
				CAIRO_SET_RGBA (dest_pixel, r, g, b, a);

				dest_pixel += 4;
				src_pixel += 1;
			}

			surface_row += line_step;
		}
	}

stop_loading:

	cairo_surface_mark_dirty (surface);
	if (! g_cancellable_is_cancelled (cancellable))
		gth_image_set_cairo_surface (image, surface);

	_TIFFfree (raster);
	cairo_surface_destroy (surface);
	TIFFClose (tif);
	g_object_unref (handle.istream);

	return image;
}
