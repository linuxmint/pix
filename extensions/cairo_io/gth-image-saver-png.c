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
#include <png.h>
#include <glib/gi18n.h>
#include <gthumb.h>
#include "gth-image-saver-png.h"
#include "preferences.h"


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


struct _GthImageSaverPngPrivate {
	GtkBuilder *builder;
	GSettings  *settings;
};


G_DEFINE_TYPE_WITH_CODE (GthImageSaverPng,
			 gth_image_saver_png,
			 GTH_TYPE_IMAGE_SAVER,
			 G_ADD_PRIVATE (GthImageSaverPng))


static void
gth_image_saver_png_finalize (GObject *object)
{
	GthImageSaverPng *self = GTH_IMAGE_SAVER_PNG (object);

	_g_object_unref (self->priv->builder);
	_g_object_unref (self->priv->settings);
	G_OBJECT_CLASS (gth_image_saver_png_parent_class)->finalize (object);
}


static GtkWidget *
gth_image_saver_png_get_control (GthImageSaver *base)
{
	GthImageSaverPng *self = GTH_IMAGE_SAVER_PNG (base);

	_g_object_unref (self->priv->builder);
	self->priv->builder = _gtk_builder_new_from_file ("png-options.ui", "cairo_io");

	gtk_adjustment_set_value (GTK_ADJUSTMENT (_gtk_builder_get_widget (self->priv->builder, "png_compression_adjustment")),
				  g_settings_get_int (self->priv->settings, PREF_PNG_COMPRESSION_LEVEL));

	return _gtk_builder_get_widget (self->priv->builder, "png_options");
}


static void
gth_image_saver_png_save_options (GthImageSaver *base)
{
	GthImageSaverPng *self = GTH_IMAGE_SAVER_PNG (base);

	g_settings_set_int (self->priv->settings, PREF_PNG_COMPRESSION_LEVEL, (int) gtk_adjustment_get_value (GTK_ADJUSTMENT (_gtk_builder_get_widget (self->priv->builder, "png_compression_adjustment"))));
}


static gboolean
gth_image_saver_png_can_save (GthImageSaver *self,
			      const char    *mime_type)
{
	return g_content_type_equals (mime_type, "image/png");
}


typedef struct {
	GError        **error;
	png_struct     *png_ptr;
	png_info       *png_info_ptr;
	GthBufferData  *buffer_data;
} CairoPngData;


static void
_cairo_png_data_destroy (CairoPngData *cairo_png_data)
{
	png_destroy_write_struct (&cairo_png_data->png_ptr, &cairo_png_data->png_info_ptr);
	gth_buffer_data_free (cairo_png_data->buffer_data, FALSE);
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
cairo_png_write_data_func (png_structp png_ptr,
		  	   png_bytep   buffer,
		  	   png_size_t  size)
{
	CairoPngData *cairo_png_data;
	GError       *error;

	cairo_png_data = png_get_io_ptr (png_ptr);
	if (! gth_buffer_data_write (cairo_png_data->buffer_data, buffer, size, &error)) {
		png_error (png_ptr, error->message);
		g_error_free (error);
	}
}


static void
cairo_png_flush_data_func (png_structp png_ptr)
{
	/* we are saving in a buffer, no need to flush */
}


static gboolean
_cairo_surface_write_as_png (cairo_surface_t  *image,
			     char            **buffer,
			     gsize            *buffer_size,
			     char            **keys,
			     char            **values,
			     GError          **error)
{
	volatile int   compression_level;
	int            width, height;
	gboolean       alpha;
	guchar        *pixels, *ptr, *buf;
	int            rowstride;
	CairoPngData  *cairo_png_data;
	png_color_8    sig_bit;
	int            bpp;
	int            row;

	compression_level = 6;

	if (keys && *keys) {
		char **kiter = keys;
		char **viter = values;

		while (*kiter) {
			if (strcmp (*kiter, "compression") == 0) {
				if (*viter == NULL) {
					g_set_error (error,
						     G_IO_ERROR,
						     G_IO_ERROR_INVALID_DATA,
						     "Must specify a compression level");
					return FALSE;
				}

				compression_level = atoi (*viter);

				if (compression_level < 0 || compression_level > 9) {
					g_set_error (error,
						     G_IO_ERROR,
						     G_IO_ERROR_INVALID_DATA,
						     "Unsupported compression level passed to the PNG saver");
					return FALSE;
				}
			}
			else {
				g_warning ("Bad option name '%s' passed to the PNG saver", *kiter);
				return FALSE;
			}

			++kiter;
			++viter;
		}
	}

	width     = cairo_image_surface_get_width (image);
	height    = cairo_image_surface_get_height (image);
	alpha     = _cairo_image_surface_get_has_alpha (image);
	pixels    = _cairo_image_surface_flush_and_get_data (image);
	rowstride = cairo_image_surface_get_stride (image);

	cairo_png_data = g_new0 (CairoPngData, 1);
	cairo_png_data->error = error;
	cairo_png_data->buffer_data = gth_buffer_data_new ();

	cairo_png_data->png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING,
							   &cairo_png_data->error,
							   gerror_error_func,
							   gerror_warning_func);
	if (cairo_png_data->png_ptr == NULL) {
		_cairo_png_data_destroy (cairo_png_data);
	        return FALSE;
	}

	cairo_png_data->png_info_ptr = png_create_info_struct (cairo_png_data->png_ptr);
	if (cairo_png_data->png_info_ptr == NULL) {
		_cairo_png_data_destroy (cairo_png_data);
	        return FALSE;
	}

	if (PNG_SETJMP (cairo_png_data->png_ptr)) {
		_cairo_png_data_destroy (cairo_png_data);
	        return FALSE;
	}

	png_set_write_fn (cairo_png_data->png_ptr,
			  cairo_png_data,
			  cairo_png_write_data_func,
			  cairo_png_flush_data_func);

	/* Set the image information here */

	png_set_IHDR (cairo_png_data->png_ptr,
		      cairo_png_data->png_info_ptr,
		      width,
		      height,
		      8,
		      (alpha ? PNG_COLOR_TYPE_RGBA : PNG_COLOR_TYPE_RGB),
		      PNG_INTERLACE_NONE,
		      PNG_COMPRESSION_TYPE_BASE,
		      PNG_FILTER_TYPE_BASE);

	/* Options */

	sig_bit.red = 8;
	sig_bit.green = 8;
	sig_bit.blue = 8;
	if (alpha)
		sig_bit.alpha = 8;
	png_set_sBIT (cairo_png_data->png_ptr, cairo_png_data->png_info_ptr, &sig_bit);

	png_set_compression_level (cairo_png_data->png_ptr, compression_level);

	/* Write the file header information. */

	png_write_info (cairo_png_data->png_ptr, cairo_png_data->png_info_ptr);

	/* Write the image */

	bpp = alpha ? 4 : 3;
	buf = g_new (guchar, width * bpp);
	ptr = pixels;
	for (row = 0; row < height; ++row) {
		_cairo_copy_line_as_rgba_big_endian (buf, ptr, width, alpha);
		png_write_rows (cairo_png_data->png_ptr, &buf, 1);

		ptr += rowstride;
	}
	g_free (buf);

	png_write_end (cairo_png_data->png_ptr, cairo_png_data->png_info_ptr);
	gth_buffer_data_get (cairo_png_data->buffer_data, buffer, buffer_size);

	_cairo_png_data_destroy (cairo_png_data);

	return TRUE;
}


static gboolean
gth_image_saver_png_save_image (GthImageSaver  *base,
				GthImage       *image,
				char          **buffer,
				gsize          *buffer_size,
				const char     *mime_type,
				GCancellable   *cancellable,
				GError        **error)
{
	GthImageSaverPng  *self = GTH_IMAGE_SAVER_PNG (base);
	cairo_surface_t   *surface;
	char             **option_keys;
	char             **option_values;
	int                i = -1;
	int                i_value;
	gboolean           result;

	option_keys = g_malloc (sizeof (char *) * 2);
	option_values = g_malloc (sizeof (char *) * 2);

	i++;
	i_value = g_settings_get_int (self->priv->settings, PREF_PNG_COMPRESSION_LEVEL);
	option_keys[i] = g_strdup ("compression");;
	option_values[i] = g_strdup_printf ("%d", i_value);

	i++;
	option_keys[i] = NULL;
	option_values[i] = NULL;

	surface = gth_image_get_cairo_surface (image);
	result = _cairo_surface_write_as_png (surface,
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
gth_image_saver_png_class_init (GthImageSaverPngClass *klass)
{
	GObjectClass       *object_class;
	GthImageSaverClass *image_saver_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = gth_image_saver_png_finalize;

	image_saver_class = GTH_IMAGE_SAVER_CLASS (klass);
	image_saver_class->id = "png";
	image_saver_class->display_name = _("PNG");
	image_saver_class->mime_type = "image/png";
	image_saver_class->extensions = "png";
	image_saver_class->get_default_ext = NULL;
	image_saver_class->get_control = gth_image_saver_png_get_control;
	image_saver_class->save_options = gth_image_saver_png_save_options;
	image_saver_class->can_save = gth_image_saver_png_can_save;
	image_saver_class->save_image = gth_image_saver_png_save_image;
}


static void
gth_image_saver_png_init (GthImageSaverPng *self)
{
	self->priv = gth_image_saver_png_get_instance_private (self);
	self->priv->settings = g_settings_new (GTHUMB_IMAGE_SAVERS_PNG_SCHEMA);
	self->priv->builder = NULL;
}
