/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  gThumb
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
#include <libheif/heif.h>
#include <glib/gi18n.h>
#include <gthumb.h>
#include "gth-image-saver-avif.h"
#include "preferences.h"


#define THUMBNAIL_SIZE 256
#define GET_WIDGET(x) (_gtk_builder_get_widget (self->priv->builder, (x)))


struct _GthImageSaverAvifPrivate {
	GtkBuilder *builder;
	GSettings  *settings;
};


G_DEFINE_TYPE_WITH_CODE (GthImageSaverAvif,
			 gth_image_saver_avif,
			 GTH_TYPE_IMAGE_SAVER,
			 G_ADD_PRIVATE (GthImageSaverAvif))


static void
gth_image_saver_avif_finalize (GObject *object)
{
	GthImageSaverAvif *self = GTH_IMAGE_SAVER_AVIF (object);

	_g_object_unref (self->priv->settings);
	_g_object_unref (self->priv->builder);

	G_OBJECT_CLASS (gth_image_saver_avif_parent_class)->finalize (object);
}


static GtkWidget *
gth_image_saver_avif_get_control (GthImageSaver *base)
{
	GthImageSaverAvif  *self = GTH_IMAGE_SAVER_AVIF (base);

	_g_object_unref (self->priv->builder);
	self->priv->builder = _gtk_builder_new_from_file ("avif-options.ui", "cairo_io");

	gtk_adjustment_set_value (GTK_ADJUSTMENT (GET_WIDGET ("quality_adjustment")),
				  g_settings_get_int (self->priv->settings, PREF_WEBP_QUALITY));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("lossless_checkbutton")),
				      g_settings_get_boolean (self->priv->settings, PREF_WEBP_LOSSLESS));

	return GET_WIDGET ("avif_options");
}


static void
gth_image_saver_avif_save_options (GthImageSaver *base)
{
	GthImageSaverAvif *self = GTH_IMAGE_SAVER_AVIF (base);

	g_settings_set_int (self->priv->settings, PREF_AVIF_QUALITY, (int) gtk_adjustment_get_value (GTK_ADJUSTMENT (GET_WIDGET ("quality_adjustment"))));
	g_settings_set_boolean (self->priv->settings, PREF_AVIF_LOSSLESS, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("lossless_checkbutton"))));
}


static gboolean
gth_image_saver_avif_can_save (GthImageSaver *self,
			       const char    *mime_type)
{
	return g_content_type_equals (mime_type, "image/avif");
}


typedef struct {
	GthBufferData       *buffer_data;
	GError             **error;
	struct heif_error    err;
} WriterData;


static struct heif_error
write_fn (struct heif_context *ctx,
	  const void          *data,
	  size_t               size,
	  void                *userdata)
{
	WriterData *writer_data = userdata;

	writer_data->err.code = gth_buffer_data_write (writer_data->buffer_data,
						       data,
						       size,
						       writer_data->error) ? heif_error_Ok : heif_error_Encoding_error;
	writer_data->err.subcode = heif_suberror_Unspecified;
	writer_data->err.message = "";

	return writer_data->err;
}


static gboolean
_cairo_surface_write_as_avif (cairo_surface_t  *surface,
			      char            **buffer,
			      gsize            *buffer_size,
			      char            **keys,
			      char            **values,
			      GError          **error)
{
	gboolean                  success;
	gboolean                  lossless;
	int                       quality;
	int                       rows, columns;
	int                       in_stride;
	gboolean                  has_alpha;
	guchar                   *pixels;
	struct heif_context      *ctx = NULL;
	struct heif_encoder      *encoder = NULL;
	struct heif_image        *image = NULL;
	struct heif_error         err;
	uint8_t                  *plane = NULL;
	int                       out_stride;
	struct heif_image_handle *handle = NULL;
	struct heif_writer        writer;
	WriterData                writer_data;

	success = FALSE;
	lossless = TRUE;
	quality = 50;

	if (keys && *keys) {
		char **kiter = keys;
		char **viter = values;

		while (*kiter) {
			if (strcmp (*kiter, "lossless") == 0) {
				if (*viter == NULL) {
					g_set_error_literal (error,
							     G_IO_ERROR,
							     G_IO_ERROR_INVALID_DATA,
							     "Must specify a value for the 'lossless' option.");
					return FALSE;
				}

				lossless = atoi (*viter);

				if (lossless < 0 || lossless > 1) {
					g_set_error_literal (error,
							     G_IO_ERROR,
							     G_IO_ERROR_INVALID_DATA,
							     "Invalid value set for the 'lossless' option.");
					return FALSE;
				}
			}
			else if (strcmp (*kiter, "quality") == 0) {
				if (*viter == NULL) {
					g_set_error_literal (error,
							     G_IO_ERROR,
							     G_IO_ERROR_INVALID_DATA,
							     "Must specify a quality value.");
					return FALSE;
				}

				quality = atoi (*viter);

				if (quality < 0 || quality > 100) {
					g_set_error_literal (error,
							     G_IO_ERROR,
							     G_IO_ERROR_INVALID_DATA,
							     "Unsupported quality value passed.");
					return FALSE;
				}
			}
			else {
				g_warning ("Bad option name '%s' passed to the HEIF/AVIF saver.", *kiter);
				return FALSE;
			}

			++kiter;
			++viter;
		}
	}

	columns = cairo_image_surface_get_width (surface);
	rows = cairo_image_surface_get_height (surface);
	in_stride = cairo_image_surface_get_stride (surface);
	has_alpha = _cairo_image_surface_get_has_alpha (surface);
	pixels = _cairo_image_surface_flush_and_get_data (surface);

	ctx = heif_context_alloc ();

	heif_context_get_encoder_for_format (ctx, heif_compression_AV1, &encoder);
	heif_encoder_set_lossless (encoder, lossless);
	heif_encoder_set_lossy_quality (encoder, quality);

	err = heif_image_create (columns,
				 rows,
				 heif_colorspace_RGB,
				 has_alpha ? heif_chroma_interleaved_RGBA : heif_chroma_interleaved_RGB,
				 &image);
	if (err.code != heif_error_Ok) {
		g_set_error (error,
			     G_IO_ERROR,
			     G_IO_ERROR_FAILED,
			     "Could not create the image: %s",
			     err.message);
		goto cleanup;
	}

	err = heif_image_add_plane (image, heif_channel_interleaved, columns, rows, 8);
	if (err.code != heif_error_Ok) {
		g_set_error (error,
			     G_IO_ERROR,
			     G_IO_ERROR_FAILED,
			     "Could not add plane to the image: %s",
			     err.message);
		goto cleanup;
	}

	plane = heif_image_get_plane (image, heif_channel_interleaved, &out_stride);
	while (rows > 0) {
		_cairo_copy_line_as_rgba_big_endian (plane, pixels, columns, has_alpha);
		plane += out_stride;
		pixels += in_stride;
		rows -= 1;
	}

	err = heif_context_encode_image (ctx, image, encoder, NULL, &handle);
	if (err.code != heif_error_Ok) {
		g_set_error (error,
			     G_IO_ERROR,
			     G_IO_ERROR_FAILED,
			     "Could not encode the image: %s",
			     err.message);
		goto cleanup;
	}

#if GENERATE_THUMBNAIL
	struct heif_image_handle *thumbnail_handle = NULL;

	err = heif_context_encode_thumbnail (ctx, image, handle, encoder, NULL, THUMBNAIL_SIZE, &thumbnail_handle);
	if (thumbnail_handle != NULL)
		heif_image_handle_release (thumbnail_handle);

	if (err.code != heif_error_Ok) {
		g_set_error (error,
			     G_IO_ERROR,
			     G_IO_ERROR_FAILED,
			     "Could not generate thumbnail: %s",
			     err.message);
		goto cleanup;
	}
#endif

	heif_image_handle_release (handle);
	handle = NULL;

	heif_encoder_release (encoder);
	encoder = NULL;

	writer.writer_api_version = 1;
	writer.write = write_fn;
	writer_data.buffer_data = gth_buffer_data_new ();
	writer_data.error = error;
	err = heif_context_write (ctx, &writer, &writer_data);

	success = (err.code == heif_error_Ok);
	if (success)
		gth_buffer_data_get (writer_data.buffer_data, buffer, buffer_size);
	gth_buffer_data_free (writer_data.buffer_data, ! success);

cleanup:

	if (handle != NULL)
		heif_image_handle_release (handle);
	if (image != NULL)
		heif_image_release (image);
	if (encoder != NULL)
		heif_encoder_release (encoder);
	if (ctx != NULL)
		heif_context_free (ctx);

	return success;
}


static gboolean
gth_image_saver_avif_save_image (GthImageSaver  *base,
				 GthImage       *image,
				 char          **buffer,
				 gsize          *buffer_size,
				 const char     *mime_type,
				 GCancellable   *cancellable,
				 GError        **error)
{
	GthImageSaverAvif  *self = GTH_IMAGE_SAVER_AVIF (base);
	char              **option_keys;
	char              **option_values;
	int                 i = -1;
	int                 i_value;
	cairo_surface_t    *surface;
	gboolean            result;

	option_keys = g_malloc (sizeof (char *) * 5);
	option_values = g_malloc (sizeof (char *) * 5);

	i++;
	i_value = g_settings_get_boolean (self->priv->settings, PREF_AVIF_LOSSLESS);
	option_keys[i] = g_strdup ("lossless");;
	option_values[i] = g_strdup_printf ("%d", i_value);

	i++;
	i_value = g_settings_get_int (self->priv->settings, PREF_AVIF_QUALITY);
	option_keys[i] = g_strdup ("quality");;
	option_values[i] = g_strdup_printf ("%d", i_value);

	i++;
	option_keys[i] = NULL;
	option_values[i] = NULL;

	surface = gth_image_get_cairo_surface (image);
	result = _cairo_surface_write_as_avif (surface,
					       buffer,
					       buffer_size,
					       option_keys,
					       option_values,
					       error);

	cairo_surface_destroy (surface);
	g_strfreev (option_keys);
	g_strfreev (option_values);

	return result;
}


static void
gth_image_saver_avif_class_init (GthImageSaverAvifClass *klass)
{
	GObjectClass       *object_class;
	GthImageSaverClass *image_saver_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = gth_image_saver_avif_finalize;

	image_saver_class = GTH_IMAGE_SAVER_CLASS (klass);
	image_saver_class->id = "avif";
	image_saver_class->display_name = _("AVIF");
	image_saver_class->mime_type = "image/avif";
	image_saver_class->extensions = "avif";
	image_saver_class->get_control = gth_image_saver_avif_get_control;
	image_saver_class->save_options = gth_image_saver_avif_save_options;
	image_saver_class->can_save = gth_image_saver_avif_can_save;
	image_saver_class->save_image = gth_image_saver_avif_save_image;
}


static void
gth_image_saver_avif_init (GthImageSaverAvif *self)
{
	self->priv = gth_image_saver_avif_get_instance_private (self);
	self->priv->settings = g_settings_new (GTHUMB_IMAGE_SAVERS_AVIF_SCHEMA);
	self->priv->builder = NULL;
}
