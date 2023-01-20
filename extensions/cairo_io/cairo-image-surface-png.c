/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2011 Free Software Foundation, Inc.
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
#include <png.h>
#if HAVE_LCMS2
#include <lcms2.h>
#endif
#include <gthumb.h>
#include "cairo-image-surface-png.h"

/* starting from libpng version 1.5 it is not possible
 * to access inside the PNG struct directly
 */
#define PNG_SETJMP(ptr) setjmp(png_jmpbuf(ptr))

#ifdef PNG_LIBPNG_VER
#if PNG_LIBPNG_VER < 10400
#ifdef PNG_SETJMP
#undef PNG_SETJMP
#endif
#define PNG_SETJMP(ptr) setjmp(ptr->jmpbuf)
#endif
#endif

typedef struct {
	GInputStream      *stream;
	GCancellable      *cancellable;
	GError           **error;
	png_struct        *png_ptr;
	png_info          *png_info_ptr;
	cairo_surface_t   *surface;
} CairoPngData;


static void
_cairo_png_data_destroy (CairoPngData *cairo_png_data)
{
	png_destroy_read_struct (&cairo_png_data->png_ptr, &cairo_png_data->png_info_ptr, NULL);
	g_object_unref (cairo_png_data->stream);
	cairo_surface_destroy (cairo_png_data->surface);
	g_free (cairo_png_data);
}


static void
gerror_error_func (png_structp     png_ptr,
		   png_const_charp message)
{
	GError ***error_p = png_get_error_ptr (png_ptr);
	GError  **error = *error_p;

	if (error != NULL)
		*error = g_error_new (G_IO_ERROR, G_IO_ERROR_INVALID_DATA, "%s", message);
}


static void
gerror_warning_func (png_structp     png_ptr,
		     png_const_charp message)
{
	/* void: we don't care about warnings */
}


static void
cairo_png_read_data_func (png_structp png_ptr,
			  png_bytep   buffer,
			  png_size_t  size)
{
	CairoPngData *cairo_png_data;
	GError       *error = NULL;

	cairo_png_data = png_get_io_ptr (png_ptr);
	if(! g_input_stream_read_all (cairo_png_data->stream,
				      buffer,
				      size,
				      NULL,
				      cairo_png_data->cancellable,
				      &error))
	{
		png_error (png_ptr, error->message);
		g_error_free (error);
	}
}


static void
transform_to_argb32_format_func (png_structp   png,
				 png_row_infop row_info,
				 png_bytep     data)
{
	guint   i;
	guint32 pixel;

	for (i = 0; i < row_info->rowbytes; i += 4) {
		guchar *p_iter = data + i;
		guchar  r, g, b, a;

		a = p_iter[3];
		if (a == 0xff) {
			pixel = CAIRO_RGBA_TO_UINT32 (p_iter[0], p_iter[1], p_iter[2], 0xff);
		}
		else if (a == 0) {
			pixel = 0;
		}
		else {
			r = _cairo_multiply_alpha (p_iter[0], a);
			g = _cairo_multiply_alpha (p_iter[1], a);
			b = _cairo_multiply_alpha (p_iter[2], a);
			pixel = CAIRO_RGBA_TO_UINT32 (r, g, b, a);
		}
		memcpy (p_iter, &pixel, sizeof (guint32));
	}
}


GthImage *
_cairo_image_surface_create_from_png (GInputStream  *istream,
		       	       	      GthFileData   *file_data,
				      int            requested_size,
				      int           *original_width,
				      int           *original_height,
				      gboolean      *loaded_original,
				      gpointer       user_data,
				      GCancellable  *cancellable,
				      GError       **error)
{
	GthImage                 *image;
	CairoPngData             *cairo_png_data;
	png_uint_32               width, height;
	int                       bit_depth, color_type, interlace_type;
	cairo_surface_metadata_t *metadata;
	unsigned char            *surface_row;
	int                       rowstride;
	png_bytep                *row_pointers;
	int                       row;
	png_textp                 text_ptr;
	int                       num_texts;

	image = gth_image_new ();

	cairo_png_data = g_new0 (CairoPngData, 1);
	cairo_png_data->cancellable = cancellable;
	cairo_png_data->error = error;
	cairo_png_data->stream = _g_object_ref (istream);
	if (cairo_png_data->stream == NULL) {
		_cairo_png_data_destroy (cairo_png_data);
		return image;
	}

	cairo_png_data->png_ptr = png_create_read_struct (PNG_LIBPNG_VER_STRING,
							  &cairo_png_data->error,
							  gerror_error_func,
							  gerror_warning_func);
	if (cairo_png_data->png_ptr == NULL) {
		_cairo_png_data_destroy (cairo_png_data);
	        return image;
	}

	cairo_png_data->png_info_ptr = png_create_info_struct (cairo_png_data->png_ptr);
	if (cairo_png_data->png_info_ptr == NULL) {
		_cairo_png_data_destroy (cairo_png_data);
	        return image;
	}

	if (PNG_SETJMP (cairo_png_data->png_ptr)) {
		_cairo_png_data_destroy (cairo_png_data);
	        return image;
	}

	png_set_read_fn (cairo_png_data->png_ptr, cairo_png_data, cairo_png_read_data_func);
	png_read_info (cairo_png_data->png_ptr, cairo_png_data->png_info_ptr);
	png_get_IHDR (cairo_png_data->png_ptr,
		      cairo_png_data->png_info_ptr,
		      &width,
		      &height,
		      &bit_depth,
		      &color_type,
		      &interlace_type,
		      NULL,
		      NULL);

	cairo_png_data->surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
	if (cairo_surface_status (cairo_png_data->surface) != CAIRO_STATUS_SUCCESS) {
		/* g_warning ("%s", cairo_status_to_string (cairo_surface_status (surface))); */
		_cairo_png_data_destroy (cairo_png_data);
	        return image;
	}

	metadata = _cairo_image_surface_get_metadata (cairo_png_data->surface);
	_cairo_metadata_set_has_alpha (metadata, (color_type & PNG_COLOR_MASK_ALPHA) || (color_type & PNG_COLOR_MASK_PALETTE));
	_cairo_metadata_set_original_size (metadata, width, height);

	/* Set the data transformations */

	png_set_strip_16 (cairo_png_data->png_ptr);

	png_set_packing (cairo_png_data->png_ptr);

	if (color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_palette_to_rgb (cairo_png_data->png_ptr);

	if ((color_type == PNG_COLOR_TYPE_GRAY) && (bit_depth < 8))
		png_set_expand_gray_1_2_4_to_8 (cairo_png_data->png_ptr);

	if (png_get_valid (cairo_png_data->png_ptr, cairo_png_data->png_info_ptr, PNG_INFO_tRNS))
	      png_set_tRNS_to_alpha (cairo_png_data->png_ptr);

	png_set_filler (cairo_png_data->png_ptr, 0xff, PNG_FILLER_AFTER);

	if ((color_type == PNG_COLOR_TYPE_GRAY) || (color_type == PNG_COLOR_TYPE_GRAY_ALPHA))
		png_set_gray_to_rgb (cairo_png_data->png_ptr);

	if (interlace_type != PNG_INTERLACE_NONE)
		png_set_interlace_handling (cairo_png_data->png_ptr);

	png_set_read_user_transform_fn (cairo_png_data->png_ptr, transform_to_argb32_format_func);

	png_read_update_info (cairo_png_data->png_ptr, cairo_png_data->png_info_ptr);

	/* Read the image */

	surface_row = _cairo_image_surface_flush_and_get_data (cairo_png_data->surface);
	rowstride = cairo_image_surface_get_stride (cairo_png_data->surface);
	row_pointers = g_new (png_bytep, height);
	for (row = 0; row < height; row++) {
		row_pointers[row] = surface_row;
		surface_row += rowstride;
	}
	png_read_image (cairo_png_data->png_ptr, row_pointers);
	png_read_end (cairo_png_data->png_ptr, cairo_png_data->png_info_ptr);
	cairo_surface_mark_dirty (cairo_png_data->surface);
	if (cairo_surface_status (cairo_png_data->surface) == CAIRO_STATUS_SUCCESS)
		gth_image_set_cairo_surface (image, cairo_png_data->surface);

	if (original_width != NULL)
		*original_width = png_get_image_width (cairo_png_data->png_ptr, cairo_png_data->png_info_ptr);
	if (original_height != NULL)
		*original_height = png_get_image_height (cairo_png_data->png_ptr, cairo_png_data->png_info_ptr);

	if (png_get_text (cairo_png_data->png_ptr,
			  cairo_png_data->png_info_ptr,
			  &text_ptr,
			  &num_texts))
	{
		int i;

		for (i = 0; i < num_texts; i++) {
			if (strcmp (text_ptr[i].key, "Thumb::Image::Width") == 0)
				metadata->thumbnail.image_width = atoi (text_ptr[i].text);
			else if (strcmp (text_ptr[i].key, "Thumb::Image::Height") == 0)
				metadata->thumbnail.image_height = atoi (text_ptr[i].text);
		}
	}
	g_free (row_pointers);

#if HAVE_LCMS2
	{
		GthICCProfile *profile = NULL;
		int            intent;
		png_charp      name;
		int            compression_type;
		png_bytep      icc_data;
		png_uint_32    icc_data_size;

		if (png_get_sRGB (cairo_png_data->png_ptr,
				  cairo_png_data->png_info_ptr,
				  &intent) == PNG_INFO_sRGB)
		{
			profile = gth_icc_profile_new_srgb ();
		}
		else if (png_get_iCCP (cairo_png_data->png_ptr,
				       cairo_png_data->png_info_ptr,
				       &name,
				       &compression_type,
				       &icc_data,
				       &icc_data_size) == PNG_INFO_iCCP)
		{
			if ((icc_data_size > 0) && (icc_data != NULL))
				profile = gth_icc_profile_new (GTH_ICC_PROFILE_ID_UNKNOWN, cmsOpenProfileFromMem (icc_data, icc_data_size));
		}

		if (profile != NULL) {
			gth_image_set_icc_profile (image, profile);
			g_object_unref (profile);
		}
	}
#endif

	_cairo_png_data_destroy (cairo_png_data);

	return image;
}
