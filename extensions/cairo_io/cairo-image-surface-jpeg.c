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
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <setjmp.h>
#include <jpeglib.h>
#include <gthumb.h>
#include <extensions/jpeg_utils/jmemorysrc.h>
#include <extensions/jpeg_utils/jpeg-info.h>
#include "cairo-image-surface-jpeg.h"


/* error handler data */


struct error_handler_data {
	struct jpeg_error_mgr   pub;
	sigjmp_buf              setjmp_buffer;
        GError                **error;
};


static void
fatal_error_handler (j_common_ptr cinfo)
{
	struct error_handler_data *errmgr;
        char buffer[JMSG_LENGTH_MAX];

	errmgr = (struct error_handler_data *) cinfo->err;

        /* Create the message */
        (* cinfo->err->format_message) (cinfo, buffer);

        /* broken check for *error == NULL for robustness against
         * crappy JPEG library
         */
        if (errmgr->error && *errmgr->error == NULL) {
                g_set_error (errmgr->error,
                             GDK_PIXBUF_ERROR,
                             GDK_PIXBUF_ERROR_CORRUPT_IMAGE,
			     _("Error interpreting JPEG image file: %s"),
                             buffer);
        }

	siglongjmp (errmgr->setjmp_buffer, 1);

        g_assert_not_reached ();
}


static void
output_message_handler (j_common_ptr cinfo)
{
	/* This method keeps libjpeg from dumping crap to stderr */
	/* do nothing */
}


/* tables with pre-multiplied values */


static unsigned char *CMYK_Tab = NULL;
static int           *YCbCr_R_Cr_Tab = NULL;
static int           *YCbCr_G_Cb_Tab = NULL;
static int           *YCbCr_G_Cr_Tab = NULL;
static int           *YCbCr_B_Cb_Tab = NULL;
static GStaticMutex   Tables_Mutex = G_STATIC_MUTEX_INIT;


#define SCALE_FACTOR   16
#define SCALE_UP(x)    ((gint32) ((x) * (1L << SCALE_FACTOR) + 0.5))
#define SCALE_DOWN(x)  ((x) >> SCALE_FACTOR)
#define ONE_HALF       ((gint32) (1 << (SCALE_FACTOR - 1)))


static void
CMYK_table_init (void)
{
	g_static_mutex_lock (&Tables_Mutex);

	if (CMYK_Tab == NULL) {
		int    v, k, i;
		double k1;

		/* tab[k * 256 + v] = v * k / 255.0 */

		CMYK_Tab = g_new (unsigned char, 256 * 256);
		i = 0;
		for (k = 0; k <= 255; k++) {
			k1 = (double) k / 255.0;
			for (v = 0; v <= 255; v++)
				CMYK_Tab[i++] = (double) v * k1;
		}
	}

	g_static_mutex_unlock (&Tables_Mutex);
}


static void
YCbCr_tables_init (void)
{
	g_static_mutex_lock (&Tables_Mutex);

	if (YCbCr_R_Cr_Tab == NULL) {
		int i, v;

		YCbCr_R_Cr_Tab = g_new (int, 256);
		YCbCr_G_Cb_Tab = g_new (int, 256);
		YCbCr_G_Cr_Tab = g_new (int, 256);
		YCbCr_B_Cb_Tab = g_new (int, 256);

		for (i = 0, v = -128; i <= 255; i++, v++) {
			YCbCr_R_Cr_Tab[i] = SCALE_DOWN (SCALE_UP (1.402) * v + ONE_HALF);
			YCbCr_G_Cb_Tab[i] = - SCALE_UP (0.34414) * v;
			YCbCr_G_Cr_Tab[i] = - SCALE_UP (0.71414) * v + ONE_HALF;
			YCbCr_B_Cb_Tab[i] = SCALE_DOWN (SCALE_UP (1.77200) * v + ONE_HALF);
		}
	}

	g_static_mutex_unlock (&Tables_Mutex);
}


GthImage *
_cairo_image_surface_create_from_jpeg (GInputStream  *istream,
				       GthFileData   *file_data,
				       int            requested_size,
				       int           *original_width,
				       int           *original_height,
				       gpointer       user_data,
				       GCancellable  *cancellable,
				       GError       **error)
{
	GthImage                      *image;
	gboolean                       load_scaled;
	GthTransform                   orientation;
	int                            destination_width;
	int                            destination_height;
	int                            line_start;
	int                            line_step;
	int                            pixel_step;
	void                          *in_buffer;
	gsize                          in_buffer_size;
	struct error_handler_data      jsrcerr;
	struct jpeg_decompress_struct  srcinfo;
	cairo_surface_t               *surface;
	cairo_surface_metadata_t      *metadata;
	unsigned char                 *surface_row;
	JSAMPARRAY                     buffer;
	int                            buffer_stride;
	JDIMENSION                     n_lines;
	JSAMPARRAY                     buffer_row;
	int                            l;
	unsigned char                 *p_surface;
	guchar                         r, g, b;
	guint32                        pixel;
	unsigned char                 *p_buffer;
	int                            x;

	image = gth_image_new ();

	if (! _g_input_stream_read_all (istream,
			      	        &in_buffer,
			      	        &in_buffer_size,
			      	        cancellable,
			      	        error))
	{
		return image;
	}

	srcinfo.err = jpeg_std_error (&(jsrcerr.pub));
	jsrcerr.pub.error_exit = fatal_error_handler;
	jsrcerr.pub.output_message = output_message_handler;
	jsrcerr.error = error;

	jpeg_create_decompress (&srcinfo);

	if (sigsetjmp (jsrcerr.setjmp_buffer, 1)) {
		g_free (in_buffer);
		jpeg_destroy_decompress (&srcinfo);
		return image;
	}

	_jpeg_memory_src (&srcinfo, in_buffer, in_buffer_size);

	jpeg_read_header (&srcinfo, TRUE);

	srcinfo.out_color_space = srcinfo.jpeg_color_space; /* make all the color space conversions manually */

	load_scaled = (requested_size > 0) && (requested_size < srcinfo.image_width) && (requested_size < srcinfo.image_height);
	if (load_scaled) {
		for (srcinfo.scale_denom = 1; srcinfo.scale_denom <= 16; srcinfo.scale_denom++) {
			jpeg_calc_output_dimensions (&srcinfo);
			if ((srcinfo.output_width < requested_size) || (srcinfo.output_height < requested_size)) {
				srcinfo.scale_denom -= 1;
				break;
			}
		}

		if (srcinfo.scale_denom == 0)
			srcinfo.scale_denom = srcinfo.scale_num;

		jpeg_calc_output_dimensions (&srcinfo);
	}

	jpeg_start_decompress (&srcinfo);

	orientation = _jpeg_exif_orientation (in_buffer, in_buffer_size);
	_cairo_image_surface_transform_get_steps (CAIRO_FORMAT_ARGB32,
						  MIN (srcinfo.output_width, CAIRO_MAX_IMAGE_SIZE),
						  MIN (srcinfo.output_height, CAIRO_MAX_IMAGE_SIZE),
						  orientation,
						  &destination_width,
						  &destination_height,
						  &line_start,
						  &line_step,
						  &pixel_step);

#if 0
	g_print ("requested: %d, original [%d, %d] ==> load at [%d, %d]\n",
			requested_size,
			srcinfo.image_width,
			srcinfo.image_height,
			destination_width,
			destination_height);
#endif

	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, destination_width, destination_height);
	if (cairo_surface_status (surface) != CAIRO_STATUS_SUCCESS) {
		/* g_warning ("%s", cairo_status_to_string (cairo_surface_status (surface))); */

		jpeg_destroy ((j_common_ptr) &srcinfo);
		cairo_surface_destroy (surface);
		g_free (in_buffer);

		return image;
	}
	metadata = _cairo_image_surface_get_metadata (surface);
	metadata->has_alpha = FALSE;
	cairo_surface_flush (surface);
	surface_row = cairo_image_surface_get_data (surface) + line_start;

	buffer_stride = srcinfo.output_width * srcinfo.output_components;
	buffer = (*srcinfo.mem->alloc_sarray) ((j_common_ptr) &srcinfo, JPOOL_IMAGE, buffer_stride, srcinfo.rec_outbuf_height);

	switch (srcinfo.out_color_space) {
	case JCS_CMYK:
		{
			register unsigned char *cmyk_tab;
			int c, m, y, k, ki;

			CMYK_table_init ();
			cmyk_tab = CMYK_Tab;

			while (srcinfo.output_scanline < srcinfo.output_height) {
				if (g_cancellable_is_cancelled (cancellable))
					break;

				n_lines = jpeg_read_scanlines (&srcinfo, buffer, srcinfo.rec_outbuf_height);

				buffer_row = buffer;
				for (l = 0; l < n_lines; l++) {
					p_surface = surface_row;
					p_buffer = buffer_row[l];

					for (x = 0; x < srcinfo.output_width; x++) {
						if (srcinfo.saw_Adobe_marker) {
							c = p_buffer[0];
							m = p_buffer[1];
							y = p_buffer[2];
							k = p_buffer[3];
						}
						else {
							c = 255 - p_buffer[0];
							m = 255 - p_buffer[1];
							y = 255 - p_buffer[2];
							k = 255 - p_buffer[3];
						}

						ki = k << 8; /* ki = k * 256 */
						r = cmyk_tab[ki + c];
						g = cmyk_tab[ki + m];
						b = cmyk_tab[ki + y];
						pixel = CAIRO_RGBA_TO_UINT32 (r, g, b, 0xff);
						memcpy (p_surface, &pixel, sizeof (guint32));

						p_surface += pixel_step;
						p_buffer += 4 /*srcinfo.output_components*/;
					}

					surface_row += line_step;
					buffer_row += buffer_stride;
				}
			}
		}
		break;

	case JCS_GRAYSCALE:
		{
			while (srcinfo.output_scanline < srcinfo.output_height) {
				if (g_cancellable_is_cancelled (cancellable))
					break;

				n_lines = jpeg_read_scanlines (&srcinfo, buffer, srcinfo.rec_outbuf_height);

				buffer_row = buffer;
				for (l = 0; l < n_lines; l++) {
					p_surface = surface_row;
					p_buffer = buffer_row[l];

					for (x = 0; x < srcinfo.output_width; x++) {
						r = g = b = p_buffer[0];
						pixel = CAIRO_RGBA_TO_UINT32 (r, g, b, 0xff);
						memcpy (p_surface, &pixel, sizeof (guint32));

						p_surface += pixel_step;
						p_buffer += 1 /*srcinfo.output_components*/;
					}

					surface_row += line_step;
					buffer_row += buffer_stride;
				}
			}
		}
		break;

	case JCS_RGB:
		{
			while (srcinfo.output_scanline < srcinfo.output_height) {
				if (g_cancellable_is_cancelled (cancellable))
					break;

				n_lines = jpeg_read_scanlines (&srcinfo, buffer, srcinfo.rec_outbuf_height);

				buffer_row = buffer;
				for (l = 0; l < n_lines; l++) {
					p_surface = surface_row;
					p_buffer = buffer_row[l];

					for (x = 0; x < srcinfo.output_width; x++) {
						r = p_buffer[0];
						g = p_buffer[1];
						b = p_buffer[2];
						pixel = CAIRO_RGBA_TO_UINT32 (r, g, b, 0xff);
						memcpy (p_surface, &pixel, sizeof (guint32));

						p_surface += pixel_step;
						p_buffer += 3 /*srcinfo.output_components*/;
					}

					surface_row += line_step;
					buffer_row += buffer_stride;
				}
			}
		}
		break;

	case JCS_YCbCr:
		{
			register JSAMPLE *range_limit = srcinfo.sample_range_limit;
			register int     *r_cr_tab;
			register int     *g_cb_tab;
			register int     *g_cr_tab;
			register int     *b_cb_tab;
			int               Y, Cb, Cr;

			YCbCr_tables_init ();
			r_cr_tab = YCbCr_R_Cr_Tab;
			g_cb_tab = YCbCr_G_Cb_Tab;
			g_cr_tab = YCbCr_G_Cr_Tab;
			b_cb_tab = YCbCr_B_Cb_Tab;

			while (srcinfo.output_scanline < srcinfo.output_height) {
				if (g_cancellable_is_cancelled (cancellable))
					break;

				n_lines = jpeg_read_scanlines (&srcinfo, buffer, srcinfo.rec_outbuf_height);

				buffer_row = buffer;
				for (l = 0; l < n_lines; l++) {
					p_surface = surface_row;
					p_buffer = buffer_row[l];

					for (x = 0; x < srcinfo.output_width; x++) {
						Y = p_buffer[0];
						Cb = p_buffer[1];
						Cr = p_buffer[2];

						r = range_limit[Y + r_cr_tab[Cr]];
						g = range_limit[Y + SCALE_DOWN (g_cb_tab[Cb] + g_cr_tab[Cr])];
						b = range_limit[Y + b_cb_tab[Cb]];
						pixel = CAIRO_RGBA_TO_UINT32 (r, g, b, 0xff);
						memcpy (p_surface, &pixel, sizeof (guint32));

						p_surface += pixel_step;
						p_buffer += 3 /*srcinfo.output_components*/;
					}

					surface_row += line_step;
					buffer_row += buffer_stride;
				}
			}
		}
		break;

	case JCS_YCCK:
		{
			register JSAMPLE *range_limit = srcinfo.sample_range_limit;
			register int     *r_cr_tab;
			register int     *g_cb_tab;
			register int     *g_cr_tab;
			register int     *b_cb_tab;
			register guchar  *cmyk_tab;
			int               Y, Cb, Cr, K, Ki, c, m , y;

			YCbCr_tables_init ();
			r_cr_tab = YCbCr_R_Cr_Tab;
			g_cb_tab = YCbCr_G_Cb_Tab;
			g_cr_tab = YCbCr_G_Cr_Tab;
			b_cb_tab = YCbCr_B_Cb_Tab;

			CMYK_table_init ();
			cmyk_tab = CMYK_Tab;

			while (srcinfo.output_scanline < srcinfo.output_height) {
				if (g_cancellable_is_cancelled (cancellable))
					break;

				n_lines = jpeg_read_scanlines (&srcinfo, buffer, srcinfo.rec_outbuf_height);

				buffer_row = buffer;
				for (l = 0; l < n_lines; l++) {
					p_surface = surface_row;
					p_buffer = buffer_row[l];

					for (x = 0; x < srcinfo.output_width; x++) {
						Y = p_buffer[0];
						Cb = p_buffer[1];
						Cr = p_buffer[2];
						K = p_buffer[3];

						c = range_limit[255 - (Y + r_cr_tab[Cr])];
						m = range_limit[255 - (Y + SCALE_DOWN (g_cb_tab[Cb] + g_cr_tab[Cr]))];
						y = range_limit[255 - (Y + b_cb_tab[Cb])];

						Ki = K << 8; /* ki = K * 256 */

						r = cmyk_tab[Ki + c];
						g = cmyk_tab[Ki + m];
						b = cmyk_tab[Ki + y];
						pixel = CAIRO_RGBA_TO_UINT32 (r, g, b, 0xff);
						memcpy (p_surface, &pixel, sizeof (guint32));

						p_surface += pixel_step;
						p_buffer += 4 /*srcinfo.output_components*/;
					}

					surface_row += line_step;
					buffer_row += buffer_stride;
				}
			}
		}
		break;

	case JCS_UNKNOWN:
	default:
		g_set_error (error,
                             GDK_PIXBUF_ERROR,
                             GDK_PIXBUF_ERROR_UNKNOWN_TYPE,
                             _("Unknown JPEG color space (%d)"),
                             srcinfo.out_color_space);
		break;
	}

	if (! g_cancellable_is_cancelled (cancellable)) {
		cairo_surface_mark_dirty (surface);

		/* Scale to the requested size */

		if (load_scaled) {
			cairo_surface_t *scaled;
			int              width;
			int              height;

			width = destination_width;
			height = destination_height;
			scale_keeping_ratio (&width, &height, requested_size, requested_size, TRUE);
			scaled = _cairo_image_surface_scale (surface, width, height, SCALE_FILTER_BEST, NULL);

			cairo_surface_destroy (surface);
			surface = scaled;
		}

		/* Set the original dimensions */

		if ((orientation == GTH_TRANSFORM_ROTATE_90)
		     ||	(orientation == GTH_TRANSFORM_ROTATE_270)
		     ||	(orientation == GTH_TRANSFORM_TRANSPOSE)
		     ||	(orientation == GTH_TRANSFORM_TRANSVERSE))
		{
			if (original_width != NULL)
				*original_width = srcinfo.image_height;
			if (original_height != NULL)
				*original_height = srcinfo.image_width;
		}
		else {
			if (original_width != NULL)
				*original_width = srcinfo.image_width;
			if (original_height != NULL)
				*original_height = srcinfo.image_height;
		}

		/*_cairo_image_surface_set_attribute_int (surface, "Image::Rotation", rotation); FIXME*/

		/* FIXME _cairo_image_surface_set_attribute (surface, "Jpeg::ColorSpace", jpeg_color_space_name (srcinfo.jpeg_color_space)); */

		gth_image_set_cairo_surface (image, surface);

		jpeg_finish_decompress (&srcinfo);
		jpeg_destroy_decompress (&srcinfo);
	}
	else {
		jpeg_destroy ((j_common_ptr) &srcinfo);
		g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_CANCELLED, "");
	}

	cairo_surface_destroy (surface);
	g_free (in_buffer);

	return image;
}


#undef SCALE_FACTOR
#undef SCALE_UP
#undef SCALE_DOWN
#undef ONE_HALF
