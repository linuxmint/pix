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
#ifdef HAVE_LIBTIFF
#include <tiffio.h>
#endif /* HAVE_LIBTIFF */
#include <glib/gi18n.h>
#include <gthumb.h>
#include "cairo-io-enum-types.h"
#include "gth-image-saver-tiff.h"
#include "preferences.h"


struct _GthImageSaverTiffPrivate {
	GSettings  *settings;
	GtkBuilder *builder;
	char       *default_ext;
};


G_DEFINE_TYPE_WITH_CODE (GthImageSaverTiff,
			 gth_image_saver_tiff,
			 GTH_TYPE_IMAGE_SAVER,
			 G_ADD_PRIVATE (GthImageSaverTiff))


static void
gth_image_saver_tiff_finalize (GObject *object)
{
	GthImageSaverTiff *self = GTH_IMAGE_SAVER_TIFF (object);

	_g_object_unref (self->priv->settings);
	_g_object_unref (self->priv->builder);
	g_free (self->priv->default_ext);

	G_OBJECT_CLASS (gth_image_saver_tiff_parent_class)->finalize (object);
}


static const char *
gth_image_saver_tiff_get_default_ext (GthImageSaver *base)
{
	GthImageSaverTiff *self = GTH_IMAGE_SAVER_TIFF (base);

	if (self->priv->default_ext == NULL)
		self->priv->default_ext = g_settings_get_string (self->priv->settings, PREF_TIFF_DEFAULT_EXT);

	return self->priv->default_ext;
}


static GtkWidget *
gth_image_saver_tiff_get_control (GthImageSaver *base)
{
#ifdef HAVE_LIBTIFF

	GthImageSaverTiff  *self = GTH_IMAGE_SAVER_TIFF (base);
	char              **extensions;
	int                 i;
	int                 active_idx;
	GthTiffCompression  compression_type;

	_g_object_unref (self->priv->builder);
	self->priv->builder = _gtk_builder_new_from_file ("tiff-options.ui", "cairo_io");

	active_idx = 0;
	extensions = g_strsplit (gth_image_saver_get_extensions (base), " ", -1);
	for (i = 0; extensions[i] != NULL; i++) {
		GtkTreeIter iter;

		gtk_list_store_append (GTK_LIST_STORE (gtk_builder_get_object (self->priv->builder, "tiff_default_ext_liststore")), &iter);
		gtk_list_store_set (GTK_LIST_STORE (gtk_builder_get_object (self->priv->builder, "tiff_default_ext_liststore")),
				    &iter,
				    0, extensions[i],
				    -1);
		if (g_str_equal (extensions[i], gth_image_saver_get_default_ext (base)))
			active_idx = i;
	}
	gtk_combo_box_set_active (GTK_COMBO_BOX (_gtk_builder_get_widget (self->priv->builder, "tiff_default_extension_combobox")), active_idx);
	g_strfreev (extensions);

	compression_type = g_settings_get_enum (self->priv->settings, PREF_TIFF_COMPRESSION);
	switch (compression_type) {
	case GTH_TIFF_COMPRESSION_NONE:
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (_gtk_builder_get_widget (self->priv->builder, "tiff_comp_none_radiobutton")), TRUE);
		break;
	case GTH_TIFF_COMPRESSION_DEFLATE:
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (_gtk_builder_get_widget (self->priv->builder, "tiff_comp_deflate_radiobutton")), TRUE);
		break;
	case GTH_TIFF_COMPRESSION_JPEG:
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (_gtk_builder_get_widget (self->priv->builder, "tiff_comp_jpeg_radiobutton")), TRUE);
		break;
	}

	gtk_spin_button_set_value (GTK_SPIN_BUTTON (_gtk_builder_get_widget (self->priv->builder, "tiff_hdpi_spinbutton")),
				   g_settings_get_int (self->priv->settings, PREF_TIFF_HORIZONTAL_RES));
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (_gtk_builder_get_widget (self->priv->builder, "tiff_vdpi_spinbutton")),
				   g_settings_get_int (self->priv->settings, PREF_TIFF_VERTICAL_RES));

	return _gtk_builder_get_widget (self->priv->builder, "tiff_options");

#else /* ! HAVE_LIBTIFF */

	return GTH_IMAGE_SAVER_CLASS (gth_image_saver_tiff_parent_class)->get_control (base);

#endif /* HAVE_LIBTIFF */
}


static void
gth_image_saver_tiff_save_options (GthImageSaver *base)
{
#ifdef HAVE_LIBTIFF

	GthImageSaverTiff  *self = GTH_IMAGE_SAVER_TIFF (base);
	GtkTreeIter         iter;
	GthTiffCompression  compression_type;

	if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (_gtk_builder_get_widget (self->priv->builder, "tiff_default_extension_combobox")), &iter)) {
		g_free (self->priv->default_ext);
		gtk_tree_model_get (GTK_TREE_MODEL (gtk_builder_get_object (self->priv->builder, "tiff_default_ext_liststore")),
				    &iter,
				    0, &self->priv->default_ext,
				    -1);
		g_settings_set_string (self->priv->settings, PREF_TIFF_DEFAULT_EXT, self->priv->default_ext);
	}

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (_gtk_builder_get_widget (self->priv->builder, "tiff_comp_none_radiobutton"))))
		compression_type = GTH_TIFF_COMPRESSION_NONE;
	else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (_gtk_builder_get_widget (self->priv->builder, "tiff_comp_deflate_radiobutton"))))
		compression_type = GTH_TIFF_COMPRESSION_DEFLATE;
	else
		compression_type = GTH_TIFF_COMPRESSION_JPEG;
	g_settings_set_enum (self->priv->settings, PREF_TIFF_COMPRESSION, compression_type);

	g_settings_set_int (self->priv->settings, PREF_TIFF_HORIZONTAL_RES, (int) gtk_spin_button_get_value (GTK_SPIN_BUTTON (_gtk_builder_get_widget (self->priv->builder, "tiff_hdpi_spinbutton"))));
	g_settings_set_int (self->priv->settings, PREF_TIFF_VERTICAL_RES, (int) gtk_spin_button_get_value (GTK_SPIN_BUTTON (_gtk_builder_get_widget (self->priv->builder, "tiff_vdpi_spinbutton"))));

#endif /* HAVE_LIBTIFF */
}


static gboolean
gth_image_saver_tiff_can_save (GthImageSaver *self,
			       const char    *mime_type)
{
#ifdef HAVE_LIBTIFF

	return g_content_type_equals (mime_type, "image/tiff");

#else /* ! HAVE_LIBTIFF */

	GSList          *formats;
	GSList          *scan;
	GdkPixbufFormat *tiff_format;

	if (! g_content_type_equals (mime_type, "image/tiff"))
		return FALSE;

	formats = gdk_pixbuf_get_formats ();
	tiff_format = NULL;
	for (scan = formats; (tiff_format == NULL) && (scan != NULL); scan = g_slist_next (scan)) {
		GdkPixbufFormat  *format = scan->data;
		char            **mime_types;
		int               i;

		mime_types = gdk_pixbuf_format_get_mime_types (format);
		for (i = 0; mime_types[i] != NULL; i++)
			if (g_content_type_equals (mime_types[i], "image/tiff"))
				break;

		if (mime_types[i] == NULL)
			continue;

		if (! gdk_pixbuf_format_is_writable (format))
			continue;

		tiff_format = format;
	}

	return tiff_format != NULL;

#endif /* HAVE_LIBTIFF */
}


#ifdef HAVE_LIBTIFF


/* -- gth_image_saver_tiff_save_pixbuf -- */


#define TILE_HEIGHT 40   /* FIXME */


static tsize_t
tiff_read (thandle_t handle, tdata_t buf, tsize_t size)
{
	return -1;
}


static tsize_t
tiff_write (thandle_t handle, tdata_t buf, tsize_t size)
{
        GthBufferData *buffer_data = (GthBufferData *)handle;

        gth_buffer_data_write (buffer_data, buf, size, NULL);
        return size;
}


static toff_t
tiff_seek (thandle_t handle, toff_t offset, int whence)
{
	GthBufferData *buffer_data = (GthBufferData *)handle;

	return gth_buffer_data_seek (buffer_data, offset, whence);
}


static int
tiff_close (thandle_t context)
{
        return 0;
}


static toff_t
tiff_size (thandle_t handle)
{
        return -1;
}


static gboolean
_cairo_surface_write_as_tiff (cairo_surface_t  *image,
			      char            **buffer,
			      gsize            *buffer_size,
			      char            **keys,
			      char            **values,
			      GError          **error)
{
	GthBufferData *buffer_data;
	TIFF          *tif;
	int            cols, rows, row;
	glong          rowsperstrip;
	gushort        compression;
	int            alpha;
	gshort         predictor;
	gshort         photometric;
	gshort         samplesperpixel;
	gshort         bitspersample;
	gushort        extra_samples[1];
	int            rowstride;
	guchar        *pixels, *ptr, *buf;
	int            horizontal_dpi = 72, vertical_dpi = 72;
	gboolean       save_resolution = FALSE;

	compression = COMPRESSION_DEFLATE;

	if (keys && *keys) {
		char **kiter = keys;
		char **viter = values;

		while (*kiter) {
			if (strcmp (*kiter, "compression") == 0) {
				if (*viter == NULL) {
					g_set_error (error,
						     GDK_PIXBUF_ERROR,
						     GDK_PIXBUF_ERROR_BAD_OPTION,
						     "Must specify a compression type");
					return FALSE;
				}

				if (strcmp (*viter, "none") == 0)
					compression = COMPRESSION_NONE;
				else if (strcmp (*viter, "pack bits") == 0)
					compression = COMPRESSION_PACKBITS;
				else if (strcmp (*viter, "lzw") == 0)
					compression = COMPRESSION_LZW;
				else if (strcmp (*viter, "deflate") == 0)
					compression = COMPRESSION_DEFLATE;
				else if (strcmp (*viter, "jpeg") == 0)
					compression = COMPRESSION_JPEG;
				else {
					g_set_error (error,
						     GDK_PIXBUF_ERROR,
						     GDK_PIXBUF_ERROR_BAD_OPTION,
						     "Unsupported compression type passed to the TIFF saver");
					return FALSE;
				}
			}
			else if (strcmp (*kiter, "vertical dpi") == 0) {
				char *endptr = NULL;
				vertical_dpi = strtol (*viter, &endptr, 10);
				save_resolution = TRUE;

				if (endptr == *viter) {
					g_set_error (error,
						     GDK_PIXBUF_ERROR,
						     GDK_PIXBUF_ERROR_BAD_OPTION,
						     "TIFF vertical dpi must be a value greater than 0; value '%s' could not be parsed.",
						     *viter);
					return FALSE;
				}

				if (vertical_dpi < 0) {
					g_set_error (error,
						     GDK_PIXBUF_ERROR,
						     GDK_PIXBUF_ERROR_BAD_OPTION,
						     "TIFF vertical dpi must be a value greater than 0; value '%d' is not allowed.",
						     vertical_dpi);
					return FALSE;
				}
			}
			else if (strcmp (*kiter, "horizontal dpi") == 0) {
				char *endptr = NULL;
				horizontal_dpi = strtol (*viter, &endptr, 10);
				save_resolution = TRUE;

				if (endptr == *viter) {
					g_set_error (error,
						     GDK_PIXBUF_ERROR,
						     GDK_PIXBUF_ERROR_BAD_OPTION,
						     "TIFF horizontal dpi must be a value greater than 0; value '%s' could not be parsed.",
						     *viter);
					return FALSE;
				}

				if (horizontal_dpi < 0) {
					g_set_error (error,
						     GDK_PIXBUF_ERROR,
						     GDK_PIXBUF_ERROR_BAD_OPTION,
						     "TIFF horizontal dpi must be a value greater than 0; value '%d' is not allowed.",
						     horizontal_dpi);
					return FALSE;
				}
			}
			else {
				g_warning ("Bad option name '%s' passed to the TIFF saver", *kiter);
				return FALSE;
			}

			++kiter;
			++viter;
		}
	}

	predictor    = 0;
	rowsperstrip = TILE_HEIGHT;

	buffer_data = gth_buffer_data_new ();
	tif = TIFFClientOpen ("gth-tiff-writer", "w",
			      buffer_data,
	                      tiff_read,
	                      tiff_write,
	                      tiff_seek,
	                      tiff_close,
	                      tiff_size,
	                      NULL,
	                      NULL);
	if (tif == NULL) {
		g_set_error_literal (error,
				     GDK_PIXBUF_ERROR,
				     GDK_PIXBUF_ERROR_INSUFFICIENT_MEMORY,
				     "Couldn't allocate memory for writing TIFF file");
		return FALSE;
	}

	cols      = cairo_image_surface_get_width (image);
	rows      = cairo_image_surface_get_height (image);
	alpha     = _cairo_image_surface_get_has_alpha (image);
	pixels    = _cairo_image_surface_flush_and_get_data (image);
	rowstride = cairo_image_surface_get_stride (image);

	predictor       = 2;
	bitspersample   = 8;
	photometric     = PHOTOMETRIC_RGB;

	if (alpha)
		samplesperpixel = 4;
	else
		samplesperpixel = 3;

	/* Set TIFF parameters. */

	TIFFSetField (tif, TIFFTAG_SUBFILETYPE,   0);
	TIFFSetField (tif, TIFFTAG_IMAGEWIDTH,    cols);
	TIFFSetField (tif, TIFFTAG_IMAGELENGTH,   rows);
	TIFFSetField (tif, TIFFTAG_BITSPERSAMPLE, bitspersample);
	TIFFSetField (tif, TIFFTAG_ORIENTATION,   ORIENTATION_TOPLEFT);
	TIFFSetField (tif, TIFFTAG_COMPRESSION,   compression);

	if ((compression == COMPRESSION_LZW || compression == COMPRESSION_DEFLATE)
	    && (predictor != 0))
	{
		TIFFSetField (tif, TIFFTAG_PREDICTOR, predictor);
	}

	if (alpha) {
		extra_samples [0] = EXTRASAMPLE_ASSOCALPHA;
		TIFFSetField (tif, TIFFTAG_EXTRASAMPLES, 1, extra_samples);
	}

	TIFFSetField (tif, TIFFTAG_PHOTOMETRIC,     photometric);
	/*TIFFSetField (tif, TIFFTAG_DOCUMENTNAME,    filename);*/
	TIFFSetField (tif, TIFFTAG_SAMPLESPERPIXEL, samplesperpixel);
	TIFFSetField (tif, TIFFTAG_ROWSPERSTRIP,    rowsperstrip);
	TIFFSetField (tif, TIFFTAG_PLANARCONFIG,    PLANARCONFIG_CONTIG);

	if (save_resolution) {
		TIFFSetField (tif, TIFFTAG_XRESOLUTION, (double) horizontal_dpi);
		TIFFSetField (tif, TIFFTAG_YRESOLUTION, (double) vertical_dpi);
		TIFFSetField (tif, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH);
	}

	buf = g_try_malloc (cols * samplesperpixel * sizeof (guchar));
	if (! buf) {
		g_set_error_literal (error,
				     GDK_PIXBUF_ERROR,
				     GDK_PIXBUF_ERROR_INSUFFICIENT_MEMORY,
				     _("Insufficient memory"));
		return FALSE;
	}

	/* Now write the TIFF data. */
	ptr = pixels;
	for (row = 0; row < rows; row++) {
		_cairo_copy_line_as_rgba_big_endian (buf, ptr, cols, alpha);
		if (TIFFWriteScanline (tif, buf, row, 0) < 0) {
			g_set_error (error,
				     GDK_PIXBUF_ERROR,
				     GDK_PIXBUF_ERROR_FAILED,
				     "TIFF Failed a scanline write on row %d",
				     row);
			return FALSE;
		}

		ptr += rowstride;
	}

	g_free (buf);

	TIFFFlushData (tif);
	TIFFClose (tif);

	gth_buffer_data_get (buffer_data, buffer, buffer_size);
	gth_buffer_data_free (buffer_data, FALSE);

	return TRUE;
}


#endif /* HAVE_LIBTIFF */


static gboolean
gth_image_saver_tiff_save_image (GthImageSaver  *base,
				 GthImage       *image,
				 char          **buffer,
				 gsize          *buffer_size,
				 const char     *mime_type,
				 GCancellable   *cancellable,
				 GError        **error)
{
#ifdef HAVE_LIBTIFF
	GthImageSaverTiff  *self = GTH_IMAGE_SAVER_TIFF (base);
	char    	  **option_keys;
	char              **option_values;
	int                 i = -1;
	int                 i_value;
	cairo_surface_t    *surface;
	gboolean            result;

	option_keys = g_malloc (sizeof (char *) * 4);
	option_values = g_malloc (sizeof (char *) * 4);

	i++;
	option_keys[i] = g_strdup ("compression");;
	option_values[i] = g_settings_get_string (self->priv->settings, PREF_TIFF_COMPRESSION);

	i++;
	i_value = g_settings_get_int (self->priv->settings, PREF_TIFF_VERTICAL_RES);
	option_keys[i] = g_strdup ("vertical dpi");;
	option_values[i] = g_strdup_printf ("%d", i_value);

	i++;
	i_value = g_settings_get_int (self->priv->settings, PREF_TIFF_HORIZONTAL_RES);
	option_keys[i] = g_strdup ("horizontal dpi");;
	option_values[i] = g_strdup_printf ("%d", i_value);

	i++;
	option_keys[i] = NULL;
	option_values[i] = NULL;

	surface = gth_image_get_cairo_surface (image);
	result = _cairo_surface_write_as_tiff (surface,
					       buffer,
					       buffer_size,
					       option_keys,
					       option_values,
					       error);

	cairo_surface_destroy (surface);
	g_strfreev (option_keys);
	g_strfreev (option_values);

#else /* ! HAVE_LIBTIFF */

	GdkPixbuf *pixbuf;
	char      *pixbuf_type;
	gboolean   result;

	pixbuf = gth_image_get_pixbuf (image);
	pixbuf_type = _gdk_pixbuf_get_type_from_mime_type (mime_type);
	result = gdk_pixbuf_save_to_bufferv (pixbuf,
					     buffer,
					     buffer_size,
					     pixbuf_type,
					     NULL,
					     NULL,
					     error);

	g_free (pixbuf_type);
	g_object_unref (pixbuf);

#endif /* HAVE_LIBTIFF */

	return result;
}


static void
gth_image_saver_tiff_class_init (GthImageSaverTiffClass *klass)
{
	GObjectClass       *object_class;
	GthImageSaverClass *image_saver_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = gth_image_saver_tiff_finalize;

	image_saver_class = GTH_IMAGE_SAVER_CLASS (klass);
	image_saver_class->id = "tiff";
	image_saver_class->display_name = _("TIFF");
	image_saver_class->mime_type = "image/tiff";
	image_saver_class->extensions = "tiff tif";
	image_saver_class->get_default_ext = gth_image_saver_tiff_get_default_ext;
	image_saver_class->get_control = gth_image_saver_tiff_get_control;
	image_saver_class->save_options = gth_image_saver_tiff_save_options;
	image_saver_class->can_save = gth_image_saver_tiff_can_save;
	image_saver_class->save_image = gth_image_saver_tiff_save_image;
}


static void
gth_image_saver_tiff_init (GthImageSaverTiff *self)
{
	self->priv = gth_image_saver_tiff_get_instance_private (self);
	self->priv->settings = g_settings_new (GTHUMB_IMAGE_SAVERS_TIFF_SCHEMA);
	self->priv->builder = NULL;
	self->priv->default_ext = NULL;
}
