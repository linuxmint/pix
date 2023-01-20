/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2021 Free Software Foundation, Inc.
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
#include <jxl/decode.h>
#include <jxl/thread_parallel_runner.h>
#if HAVE_LCMS2
#include <lcms2.h>
#endif
#include <gthumb.h>
#include "cairo-image-surface-jxl.h"

static void
convert_pixels (int     width,
		int     height,
		guchar *buffer)
{
	int     x, y;
	guchar *p = buffer, r, g, b, a;

	for (y = 0; y < height; y++)
		for (x = width; x; x--, p += 4) {
			a = p[3];
			if (a == 0) {
				*(guint32*)p = 0;
				continue;
			}

			r = p[0];
			g = p[1];
			b = p[2];
			if (a < 0xff) {
				r = _cairo_multiply_alpha(r, a);
				g = _cairo_multiply_alpha(g, a);
				b = _cairo_multiply_alpha(b, a);
			}
			*(guint32*)p = CAIRO_RGBA_TO_UINT32(r, g, b, a);
		}
}

#define BUFFER_SIZE (1024*1024)

GthImage *
_cairo_image_surface_create_from_jxl(GInputStream  *istream,
				     GthFileData   *file_data,
				     int            requested_size,
				     int           *original_width,
				     int           *original_height,
				     gboolean      *loaded_original,
				     gpointer       user_data,
				     GCancellable  *cancellable,
				     GError       **error)
{
	GthImage                  *image;
	guchar                    *filebuffer;
	gsize                      filebuffer_size, read_size, buffer_read, unprocessed_len, processed_len;
	int                        width = 0, height = 0;
	cairo_surface_t           *surface = NULL;
	cairo_surface_metadata_t  *metadata;
	JxlDecoder                *dec;
	void                      *runner;
	JxlDecoderStatus           status;
	JxlBasicInfo               info;
	JxlPixelFormat             pixel_format;
	guchar                    *surface_data = NULL;

	image = gth_image_new();

	dec = JxlDecoderCreate(NULL);
	if (dec == NULL) {
		g_error("Could not create JXL decoder.\n");
		return image;
	}

	filebuffer_size = read_size = JxlDecoderSizeHintBasicInfo(dec);
	filebuffer = g_new(guchar, filebuffer_size);
	if (! g_input_stream_read_all(istream,
				      filebuffer,
				      filebuffer_size,
				      &buffer_read,
				      cancellable,
				      error))
	{
		g_error("Could not read start of JXL file.\n");
		JxlDecoderDestroy(dec);
		g_free(filebuffer);
		return image;
	}

	if (JxlSignatureCheck(filebuffer, buffer_read) < JXL_SIG_CODESTREAM) {
		g_error("Signature does not match for JPEG XL codestream or container.\n");
		JxlDecoderDestroy(dec);
		g_free(filebuffer);
		return image;
	}

	read_size = 1048576;

	runner = JxlThreadParallelRunnerCreate(NULL, JxlThreadParallelRunnerDefaultNumWorkerThreads());
	if (runner == NULL) {
		g_error("Could not create threaded parallel runner.\n");
		JxlDecoderDestroy(dec);
		g_free(filebuffer);
		return image;
	}

	if (JxlDecoderSetParallelRunner(dec,
					JxlThreadParallelRunner,
					runner) > 0) {
		g_error("Could not set parallel runner.\n");
		JxlThreadParallelRunnerDestroy(runner);
		JxlDecoderDestroy(dec);
		g_free(filebuffer);
		return image;
	}

	if (JxlDecoderSubscribeEvents(dec,
				      JXL_DEC_BASIC_INFO
				      | JXL_DEC_COLOR_ENCODING
				      | JXL_DEC_FULL_IMAGE) > 0) {
		g_error("Could not subscribe to decoder events.\n");
		JxlThreadParallelRunnerDestroy(runner);
		JxlDecoderDestroy(dec);
		g_free(filebuffer);
		return image;
	}

	if (JxlDecoderSetInput(dec, filebuffer, buffer_read) > 0) {
		g_error("Could not set decoder input.\n");
		JxlThreadParallelRunnerDestroy(runner);
		JxlDecoderDestroy(dec);
		g_free(filebuffer);
		return image;
	}

	status = JXL_DEC_NEED_MORE_INPUT;
	while (status != JXL_DEC_SUCCESS) {
		status = JxlDecoderProcessInput(dec);

		switch (status) {
		case JXL_DEC_ERROR:
			g_error("jxl: decoder error.\n");
			JxlThreadParallelRunnerDestroy(runner);
			JxlDecoderDestroy(dec);
			g_free(filebuffer);
			return image;

		case JXL_DEC_NEED_MORE_INPUT:
			if (buffer_read == 0) {
				g_message("Reached end of file but decoder still wants more.\n");
				status = JXL_DEC_SUCCESS;
				break;
			}

			{
				unprocessed_len = JxlDecoderReleaseInput(dec);
				processed_len = filebuffer_size - unprocessed_len;

				guchar *old_filebuffer = filebuffer;
				filebuffer_size = unprocessed_len + read_size;
				filebuffer = g_new(guchar, filebuffer_size);
				if (unprocessed_len > 0)
					memcpy(filebuffer, old_filebuffer + processed_len, unprocessed_len);
				g_free(old_filebuffer);

				gssize signed_buffer_read = g_input_stream_read(istream,
										filebuffer + unprocessed_len,
										read_size,
										cancellable,
										error);
				if (signed_buffer_read <= 0) {
					buffer_read = 0;
					break;
				}
				buffer_read = signed_buffer_read;

				if (JxlDecoderSetInput(dec, filebuffer, unprocessed_len + buffer_read) > 0) {
					g_error("Could not set decoder input.\n");
					JxlThreadParallelRunnerDestroy(runner);
					JxlDecoderDestroy(dec);
					g_free(filebuffer);
					return image;
				}
			}
			break;

		case JXL_DEC_BASIC_INFO:
			if (JxlDecoderGetBasicInfo(dec, &info) > 0) {
				g_error("Could not get basic info from decoder.\n");
				JxlThreadParallelRunnerDestroy(runner);
				JxlDecoderDestroy(dec);
				g_free(filebuffer);
				return image;
			}

			pixel_format.num_channels = 4;
			pixel_format.data_type = JXL_TYPE_UINT8;
			pixel_format.endianness = JXL_NATIVE_ENDIAN;
			pixel_format.align = 0;

			width = info.xsize;
			height = info.ysize;

			if (original_width != NULL)
				*original_width = width;
			if (original_height != NULL)
				*original_height = height;

			surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
			surface_data = _cairo_image_surface_flush_and_get_data(surface);
			metadata = _cairo_image_surface_get_metadata(surface);
			_cairo_metadata_set_has_alpha(metadata, info.num_extra_channels);

			break;

		case JXL_DEC_COLOR_ENCODING:
#if HAVE_LCMS2
			if (JxlDecoderGetColorAsEncodedProfile(dec, &pixel_format, JXL_COLOR_PROFILE_TARGET_DATA, NULL) == JXL_DEC_SUCCESS)
				break;

			{
				gsize profile_size;
				if (JxlDecoderGetICCProfileSize(dec, &pixel_format, JXL_COLOR_PROFILE_TARGET_DATA, &profile_size) > 0) {
					g_message("Could not get ICC profile size.\n");
					break;
				}

				guchar *profile_data = g_new(guchar, profile_size);
				if (JxlDecoderGetColorAsICCProfile(dec, &pixel_format, JXL_COLOR_PROFILE_TARGET_DATA, profile_data, profile_size) > 0) {
					g_message("Could not get ICC profile.\n");
					g_free(profile_data);
					break;
				}

				GthICCProfile *profile = NULL;
				profile = gth_icc_profile_new(GTH_ICC_PROFILE_ID_UNKNOWN, cmsOpenProfileFromMem(profile_data, profile_size));
				if (profile != NULL) {
					gth_image_set_icc_profile(image, profile);
					g_object_unref(profile);
				}
			}
#endif
			break;

		case JXL_DEC_NEED_IMAGE_OUT_BUFFER:
			if (JxlDecoderSetImageOutBuffer(dec, &pixel_format, surface_data, width * height * 4) > 0) {
				g_error("Could not set image-out buffer.\n");
				JxlThreadParallelRunnerDestroy(runner);
				JxlDecoderDestroy(dec);
				g_free(filebuffer);
				return image;
			}

			break;

		case JXL_DEC_SUCCESS:
			continue;

		case JXL_DEC_FULL_IMAGE:
			continue;

		default:
			break;
		}
	}

	JxlThreadParallelRunnerDestroy(runner);
	JxlDecoderDestroy(dec);
	g_free(filebuffer);

	convert_pixels(width, height, surface_data);

	cairo_surface_mark_dirty(surface);
	if (cairo_surface_status(surface) == CAIRO_STATUS_SUCCESS)
		gth_image_set_cairo_surface(image, surface);
	cairo_surface_destroy (surface);

	return image;
}
