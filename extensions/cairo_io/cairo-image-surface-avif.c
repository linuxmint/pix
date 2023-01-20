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
#include <libheif/heif.h>
#if HAVE_LCMS2
#include <lcms2.h>
#endif
#include <gthumb.h>
#include "cairo-image-surface-avif.h"


GthImage *
_cairo_image_surface_create_from_avif (GInputStream   *istream,
				       GthFileData    *file_data,
				       int             requested_size,
				       int            *p_original_width,
				       int            *p_original_height,
				       gboolean       *loaded_original,
				       gpointer        user_data,
				       GCancellable   *cancellable,
				       GError        **error)
{
	GthImage                 *image;
	void                     *buffer = NULL;
	gsize                     buffer_size;
	struct heif_context      *ctx = NULL;
	struct heif_error         err;
	struct heif_image_handle *handle = NULL;
	gboolean                  has_alpha;
	struct heif_image        *img = NULL;
	int                       original_width;
	int                       original_height;
	int                       image_width;
	int                       image_height;
	int                       out_stride;
	const uint8_t            *data;
	cairo_surface_t          *surface;
	cairo_surface_metadata_t *metadata;

	image = gth_image_new ();

	if (! _g_input_stream_read_all (istream,
					&buffer,
					&buffer_size,
					cancellable,
					error))
	{
		goto stop_loading;
	}

	ctx = heif_context_alloc ();
	err = heif_context_read_from_memory_without_copy (ctx, buffer, buffer_size, NULL);
	if (err.code != heif_error_Ok) {
		g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED, err.message);
		goto stop_loading;
	}

	err = heif_context_get_primary_image_handle (ctx, &handle);
	if (err.code != heif_error_Ok) {
		g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED, err.message);
		goto stop_loading;
	}

#if HAVE_LCMS2
	if (heif_image_handle_get_color_profile_type (handle) != heif_color_profile_type_not_present) {
		size_t   icc_data_size;
		gpointer icc_data;

		icc_data_size = heif_image_handle_get_raw_color_profile_size (handle);
		icc_data = g_malloc (icc_data_size);
		err = heif_image_handle_get_raw_color_profile (handle, icc_data);
		if (err.code == heif_error_Ok) {
			GthICCProfile *profile;

			profile = gth_icc_profile_new (GTH_ICC_PROFILE_ID_UNKNOWN, cmsOpenProfileFromMem (icc_data, icc_data_size));
			if (profile != NULL) {
				gth_image_set_icc_profile (image, profile);
				g_object_unref (profile);
			}
		}

		g_free (icc_data);
	}
#endif

	has_alpha = heif_image_handle_has_alpha_channel (handle);
	err = heif_decode_image (handle, &img, heif_colorspace_RGB, has_alpha ? heif_chroma_interleaved_RGBA : heif_chroma_interleaved_RGB, NULL);
	if (err.code != heif_error_Ok) {
		g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED, err.message);
		goto stop_loading;
	}

	original_width = heif_image_get_primary_width (img);
	original_height = heif_image_get_primary_height (img);

	image_width = original_width;
	image_height = original_height;

	/* Scaling is disabled because quality is too low. */

#if 0
	if ((requested_size > 0) && (requested_size < image_width) && (requested_size < image_height)) {
		struct heif_image *scaled_img;

		scale_keeping_ratio (&image_width, &image_height, requested_size, requested_size, TRUE);

		err = heif_image_scale_image (img, &scaled_img, image_width, image_height, NULL);
		heif_image_release (img);

		if (err.code != heif_error_Ok) {
			g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED, err.message);
			goto stop_loading;
		}

		img = scaled_img;
	}
#endif

	data = heif_image_get_plane_readonly (img, heif_channel_interleaved, &out_stride);

	surface = _cairo_image_surface_create_from_rgba (data, image_width, image_height, out_stride, has_alpha);
	if (surface == NULL)
		goto stop_loading;

	metadata = _cairo_image_surface_get_metadata (surface);
	_cairo_metadata_set_original_size (metadata, original_width, original_height);

	if (p_original_width != NULL)
		*p_original_width = original_width;
	if (p_original_height != NULL)
		*p_original_height = original_height;

	gth_image_set_cairo_surface (image, surface);
	cairo_surface_destroy (surface);

stop_loading:

	if (img != NULL)
		heif_image_release (img);
	if (handle != NULL)
		heif_image_handle_release (handle);
	if (ctx != NULL)
		heif_context_free (ctx);
	if (buffer != NULL)
		g_free (buffer);

	return image;
}
