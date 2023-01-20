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
#include <webp/encode.h>
#include <glib/gi18n.h>
#include <gthumb.h>
#include "gth-image-saver-webp.h"
#include "preferences.h"


#define GET_WIDGET(x) (_gtk_builder_get_widget (self->priv->builder, (x)))


struct _GthImageSaverWebpPrivate {
	GtkBuilder *builder;
	GSettings  *settings;
};


G_DEFINE_TYPE_WITH_CODE (GthImageSaverWebp,
			 gth_image_saver_webp,
			 GTH_TYPE_IMAGE_SAVER,
			 G_ADD_PRIVATE (GthImageSaverWebp))


static void
gth_image_saver_webp_finalize (GObject *object)
{
	GthImageSaverWebp *self = GTH_IMAGE_SAVER_WEBP (object);

	_g_object_unref (self->priv->builder);
	_g_object_unref (self->priv->settings);
	G_OBJECT_CLASS (gth_image_saver_webp_parent_class)->finalize (object);
}


static GtkWidget *
gth_image_saver_webp_get_control (GthImageSaver *base)
{
	GthImageSaverWebp *self = GTH_IMAGE_SAVER_WEBP (base);

	_g_object_unref (self->priv->builder);
	self->priv->builder = _gtk_builder_new_from_file ("webp-options.ui", "cairo_io");

	gtk_adjustment_set_value (GTK_ADJUSTMENT (GET_WIDGET ("quality_adjustment")),
				  g_settings_get_int (self->priv->settings, PREF_WEBP_QUALITY));
	gtk_adjustment_set_value (GTK_ADJUSTMENT (GET_WIDGET ("method_adjustment")),
				  g_settings_get_int (self->priv->settings, PREF_WEBP_METHOD));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("lossless_checkbutton")),
				      g_settings_get_boolean (self->priv->settings, PREF_WEBP_LOSSLESS));

	return GET_WIDGET ("webp_options");
}


static void
gth_image_saver_webp_save_options (GthImageSaver *base)
{
	GthImageSaverWebp *self = GTH_IMAGE_SAVER_WEBP (base);

	g_settings_set_int (self->priv->settings, PREF_WEBP_QUALITY, (int) gtk_adjustment_get_value (GTK_ADJUSTMENT (GET_WIDGET ("quality_adjustment"))));
	g_settings_set_int (self->priv->settings, PREF_WEBP_METHOD, (int) gtk_adjustment_get_value (GTK_ADJUSTMENT (GET_WIDGET ("method_adjustment"))));
	g_settings_set_boolean (self->priv->settings, PREF_WEBP_LOSSLESS, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("lossless_checkbutton"))));
}


static gboolean
gth_image_saver_webp_can_save (GthImageSaver *self,
			      const char    *mime_type)
{
	return g_content_type_equals (mime_type, "image/webp");
}


typedef struct {
	GError        **error;
	GthBufferData  *buffer_data;
	int             success;
} CairoWebpData;


static void
_cairo_webp_data_destroy (CairoWebpData *cairo_webp_data)
{
	gth_buffer_data_free (cairo_webp_data->buffer_data, ! cairo_webp_data->success);
	g_free (cairo_webp_data);
}


static int
cairo_webp_writer_func (const uint8_t     *data,
			size_t             data_size,
			const WebPPicture *picture)
{
	CairoWebpData *cairo_webp_data = picture->custom_ptr;

	cairo_webp_data->success = gth_buffer_data_write (cairo_webp_data->buffer_data,
							  (void *) data,
							  data_size,
							  cairo_webp_data->error);

	return cairo_webp_data->success;
}


static int
_WebPPictureImportCairoSurface (WebPPicture     *const picture,
				cairo_surface_t *image)
{
	int       stride;
	guchar   *src_row;
	uint32_t *dest_row;
	int       y, x, temp;
	guchar    r, g, b, a;

	if (_cairo_image_surface_get_has_alpha (image))
		picture->colorspace |= WEBP_CSP_ALPHA_BIT;
	else
		picture->colorspace &= ~WEBP_CSP_ALPHA_BIT;

	if (! WebPPictureAlloc (picture))
		return 0;

	stride = cairo_image_surface_get_stride (image);
	src_row = _cairo_image_surface_flush_and_get_data (image);
	dest_row = picture->argb;

	for (y= 0; y < cairo_image_surface_get_height (image); y++) {
		guchar *pixel = src_row;

		for (x = 0; x < cairo_image_surface_get_width (image); x++) {
			CAIRO_GET_RGBA (pixel, r, g, b, a);
			dest_row[x] = ((a << 24) | (r << 16) | (g <<  8) | b);

			pixel += 4;
		}

		src_row += stride;
		dest_row += picture->argb_stride;
	}

	return 1;
}


static gboolean
_cairo_surface_write_as_webp (cairo_surface_t  *image,
			      char            **buffer,
			      gsize            *buffer_size,
			      char            **keys,
			      char            **values,
			      GError          **error)
{
	gboolean       lossless;
	int            quality;
	int            method;
	WebPConfig     config;
	CairoWebpData *cairo_webp_data;
	WebPPicture    pic;

	lossless = TRUE;
	quality = 75;
	method = 4;

	if (keys && *keys) {
		char **kiter = keys;
		char **viter = values;

		while (*kiter) {
			if (strcmp (*kiter, "lossless") == 0) {
				if (*viter == NULL) {
					g_set_error (error,
						     G_IO_ERROR,
						     G_IO_ERROR_INVALID_DATA,
						     "Must specify a value for the 'lossless' option");
					return FALSE;
				}

				lossless = atoi (*viter);

				if (lossless < 0 || lossless > 1) {
					g_set_error (error,
						     G_IO_ERROR,
						     G_IO_ERROR_INVALID_DATA,
						     "Invalid value set for the 'lossless' option of the WebP saver");
					return FALSE;
				}
			}
			else if (strcmp (*kiter, "quality") == 0) {
				if (*viter == NULL) {
					g_set_error (error,
						     G_IO_ERROR,
						     G_IO_ERROR_INVALID_DATA,
						     "Must specify a quality value to the WebP saver");
					return FALSE;
				}

				quality = atoi (*viter);

				if (quality < 0 || quality > 100) {
					g_set_error (error,
						     G_IO_ERROR,
						     G_IO_ERROR_INVALID_DATA,
						     "Unsupported quality value passed to the WebP saver");
					return FALSE;
				}
			}
			else if (strcmp (*kiter, "method") == 0) {
				if (*viter == NULL) {
					g_set_error (error,
						     G_IO_ERROR,
						     G_IO_ERROR_INVALID_DATA,
						     "Must specify a method value to the WebP saver");
					return FALSE;
				}

				method = atoi (*viter);

				if (method < 0 || method > 6) {
					g_set_error (error,
						     G_IO_ERROR,
						     G_IO_ERROR_INVALID_DATA,
						     "Unsupported method value passed to the WebP saver");
					return FALSE;
				}
			}
			else {
				g_warning ("Bad option name '%s' passed to the WebP saver", *kiter);
				return FALSE;
			}

			++kiter;
			++viter;
		}
	}

	if (! WebPConfigInit (&config)) {
		g_set_error (error,
			     G_IO_ERROR,
			     G_IO_ERROR_INVALID_DATA,
			     "Version error");
		return FALSE;
	}

	config.lossless = lossless;
	config.quality = quality;
	config.method = method;

	if (! WebPValidateConfig (&config)) {
		g_set_error (error,
			     G_IO_ERROR,
			     G_IO_ERROR_INVALID_DATA,
			     "Config error");
		return FALSE;
	}

	cairo_webp_data = g_new0 (CairoWebpData, 1);
	cairo_webp_data->error = error;
	cairo_webp_data->buffer_data = gth_buffer_data_new ();
	cairo_webp_data->success = FALSE;

	WebPPictureInit (&pic);
	pic.width = cairo_image_surface_get_width (image);
	pic.height = cairo_image_surface_get_height (image);
	pic.writer = cairo_webp_writer_func;
	pic.custom_ptr = cairo_webp_data;
	pic.use_argb = TRUE;

	if (_WebPPictureImportCairoSurface (&pic, image)) {
		int ok = WebPEncode (&config, &pic);
		WebPPictureFree (&pic);

		if (cairo_webp_data->success && ! ok) {
			g_set_error (cairo_webp_data->error,
				     G_IO_ERROR,
				     G_IO_ERROR_INVALID_DATA,
				     "Encoding error: %d", pic.error_code);
			cairo_webp_data->success = FALSE;
		}
	}
	else {
		g_set_error (cairo_webp_data->error,
			     G_IO_ERROR,
			     G_IO_ERROR_INVALID_DATA,
			     "Memory error");
		cairo_webp_data->success = FALSE;
	}

	if (cairo_webp_data->success)
		gth_buffer_data_get (cairo_webp_data->buffer_data, buffer, buffer_size);

	_cairo_webp_data_destroy (cairo_webp_data);

	return TRUE;
}


static gboolean
gth_image_saver_webp_save_image (GthImageSaver   *base,
				 GthImage        *image,
				 char           **buffer,
				 gsize           *buffer_size,
				 const char      *mime_type,
				 GCancellable    *cancellable,
				 GError         **error)
{
	GthImageSaverWebp  *self = GTH_IMAGE_SAVER_WEBP (base);
	cairo_surface_t    *surface;
	char              **option_keys;
	char              **option_values;
	int                 i = -1;
	int                 i_value;
	gboolean            result;

	option_keys = g_new (char *, 4);
	option_values = g_new (char *, 4);

	i++;
	i_value = g_settings_get_boolean (self->priv->settings, PREF_WEBP_LOSSLESS);
	option_keys[i] = g_strdup ("lossless");;
	option_values[i] = g_strdup_printf ("%d", i_value);

	i++;
	i_value = g_settings_get_int (self->priv->settings, PREF_WEBP_QUALITY);
	option_keys[i] = g_strdup ("quality");;
	option_values[i] = g_strdup_printf ("%d", i_value);

	i++;
	i_value = g_settings_get_int (self->priv->settings, PREF_WEBP_METHOD);
	option_keys[i] = g_strdup ("method");;
	option_values[i] = g_strdup_printf ("%d", i_value);

	i++;
	option_keys[i] = NULL;
	option_values[i] = NULL;

	surface = gth_image_get_cairo_surface (image);
	result = _cairo_surface_write_as_webp (surface,
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
gth_image_saver_webp_class_init (GthImageSaverWebpClass *klass)
{
	GObjectClass       *object_class;
	GthImageSaverClass *image_saver_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = gth_image_saver_webp_finalize;

	image_saver_class = GTH_IMAGE_SAVER_CLASS (klass);
	image_saver_class->id = "webp";
	image_saver_class->display_name = _("WebP");
	image_saver_class->mime_type = "image/webp";
	image_saver_class->extensions = "webp";
	image_saver_class->get_default_ext = NULL;
	image_saver_class->get_control = gth_image_saver_webp_get_control;
	image_saver_class->save_options = gth_image_saver_webp_save_options;
	image_saver_class->can_save = gth_image_saver_webp_can_save;
	image_saver_class->save_image = gth_image_saver_webp_save_image;
}


static void
gth_image_saver_webp_init (GthImageSaverWebp *self)
{
	self->priv = gth_image_saver_webp_get_instance_private (self);
	self->priv->settings = g_settings_new (GTHUMB_IMAGE_SAVERS_WEBP_SCHEMA);
	self->priv->builder = NULL;
}
