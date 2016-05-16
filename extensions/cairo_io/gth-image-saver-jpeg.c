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
#ifdef HAVE_LIBJPEG
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>
#include <jpeglib.h>
#include <extensions/jpeg_utils/jmemorydest.h>
#endif /* HAVE_LIBJPEG */
#include <glib/gi18n.h>
#include <gthumb.h>
#include "gth-image-saver-jpeg.h"
#include "preferences.h"


G_DEFINE_TYPE (GthImageSaverJpeg, gth_image_saver_jpeg, GTH_TYPE_IMAGE_SAVER)


struct _GthImageSaverJpegPrivate {
	GtkBuilder *builder;
	GSettings  *settings;
	char       *default_ext;
};


static void
gth_image_saver_jpeg_finalize (GObject *object)
{
	GthImageSaverJpeg *self = GTH_IMAGE_SAVER_JPEG (object);

	_g_object_unref (self->priv->settings);
	_g_object_unref (self->priv->builder);
	g_free (self->priv->default_ext);

	G_OBJECT_CLASS (gth_image_saver_jpeg_parent_class)->finalize (object);
}


static const char *
gth_image_saver_jpeg_get_default_ext (GthImageSaver *base)
{
	GthImageSaverJpeg *self = GTH_IMAGE_SAVER_JPEG (base);

	if (self->priv->default_ext == NULL)
		self->priv->default_ext = g_settings_get_string (self->priv->settings, PREF_JPEG_DEFAULT_EXT);

	return self->priv->default_ext;
}


static GtkWidget *
gth_image_saver_jpeg_get_control (GthImageSaver *base)
{
	GthImageSaverJpeg  *self = GTH_IMAGE_SAVER_JPEG (base);
	char         **extensions;
	int            i;
	int            active_idx;

	_g_object_unref (self->priv->builder);
	self->priv->builder = _gtk_builder_new_from_file ("jpeg-options.ui", "cairo_io");

	active_idx = 0;
	extensions = g_strsplit (gth_image_saver_get_extensions (base), " ", -1);
	for (i = 0; extensions[i] != NULL; i++) {
		GtkTreeIter iter;

		gtk_list_store_append (GTK_LIST_STORE (gtk_builder_get_object (self->priv->builder, "jpeg_default_ext_liststore")), &iter);
		gtk_list_store_set (GTK_LIST_STORE (gtk_builder_get_object (self->priv->builder, "jpeg_default_ext_liststore")),
				    &iter,
				    0, extensions[i],
				    -1);
		if (g_str_equal (extensions[i], gth_image_saver_get_default_ext (base)))
			active_idx = i;
	}
	gtk_combo_box_set_active (GTK_COMBO_BOX (_gtk_builder_get_widget (self->priv->builder, "jpeg_default_extension_combobox")), active_idx);
	g_strfreev (extensions);

	gtk_adjustment_set_value (GTK_ADJUSTMENT (_gtk_builder_get_widget (self->priv->builder, "jpeg_quality_adjustment")),
				  g_settings_get_int (self->priv->settings, PREF_JPEG_QUALITY));
	gtk_adjustment_set_value (GTK_ADJUSTMENT (_gtk_builder_get_widget (self->priv->builder, "jpeg_smooth_adjustment")),
				  g_settings_get_int (self->priv->settings, PREF_JPEG_SMOOTHING));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (_gtk_builder_get_widget (self->priv->builder, "jpeg_optimize_checkbutton")),
				      g_settings_get_boolean (self->priv->settings, PREF_JPEG_OPTIMIZE));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (_gtk_builder_get_widget (self->priv->builder, "jpeg_progressive_checkbutton")),
				      g_settings_get_boolean (self->priv->settings, PREF_JPEG_PROGRESSIVE));

	return _gtk_builder_get_widget (self->priv->builder, "jpeg_options");
}


static void
gth_image_saver_jpeg_save_options (GthImageSaver *base)
{
	GthImageSaverJpeg *self = GTH_IMAGE_SAVER_JPEG (base);
	GtkTreeIter   iter;

	if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (_gtk_builder_get_widget (self->priv->builder, "jpeg_default_extension_combobox")), &iter)) {
		g_free (self->priv->default_ext);
		gtk_tree_model_get (GTK_TREE_MODEL (gtk_builder_get_object (self->priv->builder, "jpeg_default_ext_liststore")),
				    &iter,
				    0, &self->priv->default_ext,
				    -1);
		g_settings_set_string (self->priv->settings, PREF_JPEG_DEFAULT_EXT, self->priv->default_ext);
	}
	g_settings_set_int (self->priv->settings, PREF_JPEG_QUALITY, (int) gtk_adjustment_get_value (GTK_ADJUSTMENT (_gtk_builder_get_widget (self->priv->builder, "jpeg_quality_adjustment"))));
	g_settings_set_int (self->priv->settings, PREF_JPEG_SMOOTHING, (int) gtk_adjustment_get_value (GTK_ADJUSTMENT (_gtk_builder_get_widget (self->priv->builder, "jpeg_smooth_adjustment"))));
	g_settings_set_boolean (self->priv->settings, PREF_JPEG_OPTIMIZE, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (_gtk_builder_get_widget (self->priv->builder, "jpeg_optimize_checkbutton"))));
	g_settings_set_boolean (self->priv->settings, PREF_JPEG_PROGRESSIVE, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (_gtk_builder_get_widget (self->priv->builder, "jpeg_progressive_checkbutton"))));
}


static gboolean
gth_image_saver_jpeg_can_save (GthImageSaver *self,
			       const char    *mime_type)
{
#ifdef HAVE_LIBJPEG

	return g_content_type_equals (mime_type, "image/jpeg");

#else /* ! HAVE_LIBJPEG */

	GSList          *formats;
	GSList          *scan;
	GdkPixbufFormat *jpeg_format;

	if (! g_content_type_equals (mime_type, "image/jpeg"))
		return FALSE;

	formats = gdk_pixbuf_get_formats ();
	jpeg_format = NULL;
	for (scan = formats; (jpeg_format == NULL) && (scan != NULL); scan = g_slist_next (scan)) {
		GdkPixbufFormat  *format = scan->data;
		char            **mime_types;
		int               i;

		mime_types = gdk_pixbuf_format_get_mime_types (format);
		for (i = 0; mime_types[i] != NULL; i++)
			if (g_content_type_equals (mime_types[i], "image/jpeg"))
				break;

		if (mime_types[i] == NULL)
			continue;

		if (! gdk_pixbuf_format_is_writable (format))
			continue;

		jpeg_format = format;
	}

	return jpeg_format != NULL;

#endif /* HAVE_LIBJPEG */
}


#ifdef HAVE_LIBJPEG


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
			     "Error interpreting JPEG image file (%s)",
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


static gboolean
_cairo_surface_write_as_jpeg (cairo_surface_t  *image,
			      char            **buffer,
			      gsize            *buffer_size,
			      char            **keys,
			      char            **values,
			      GError          **error)
{
	struct jpeg_compress_struct cinfo;
	struct error_handler_data jerr;
	guchar            *buf = NULL;
	guchar            *pixels = NULL;
	volatile int       quality = 85; /* default; must be between 0 and 100 */
	volatile int       smoothing = 0;
	volatile gboolean  optimize = FALSE;
#ifdef HAVE_PROGRESSIVE_JPEG
	volatile gboolean  progressive = FALSE;
#endif
	int                w, h = 0;
	int                rowstride = 0;

	if (keys && *keys) {
		char **kiter = keys;
		char **viter = values;

		while (*kiter) {
			if (strcmp (*kiter, "quality") == 0) {
				char *endptr = NULL;
				quality = strtol (*viter, &endptr, 10);

				if (endptr == *viter) {
					g_set_error (error,
						     GDK_PIXBUF_ERROR,
						     GDK_PIXBUF_ERROR_BAD_OPTION,
						     "JPEG quality must be a value between 0 and 100; value '%s' could not be parsed.",
						     *viter);

					return FALSE;
				}

				if (quality < 0 || quality > 100) {
					g_set_error (error,
						     GDK_PIXBUF_ERROR,
						     GDK_PIXBUF_ERROR_BAD_OPTION,
						     "JPEG quality must be a value between 0 and 100; value '%d' is not allowed.",
						     quality);

					return FALSE;
				}
			}
			else if (strcmp (*kiter, "smooth") == 0) {
				char *endptr = NULL;
				smoothing = strtol (*viter, &endptr, 10);

				if (endptr == *viter) {
					g_set_error (error,
						     GDK_PIXBUF_ERROR,
						     GDK_PIXBUF_ERROR_BAD_OPTION,
						     "JPEG smoothing must be a value between 0 and 100; value '%s' could not be parsed.",
						     *viter);

					return FALSE;
				}

				if (smoothing < 0 || smoothing > 100) {
					g_set_error (error,
						     GDK_PIXBUF_ERROR,
						     GDK_PIXBUF_ERROR_BAD_OPTION,
						     "JPEG smoothing must be a value between 0 and 100; value '%d' is not allowed.",
						     smoothing);

					return FALSE;
				}
			}
			else if (strcmp (*kiter, "optimize") == 0) {
				if (strcmp (*viter, "yes") == 0)
					optimize = TRUE;
				else if (strcmp (*viter, "no") == 0)
					optimize = FALSE;
				else {
					g_set_error (error,
						     GDK_PIXBUF_ERROR,
						     GDK_PIXBUF_ERROR_BAD_OPTION,
						     "JPEG optimize option must be 'yes' or 'no', value is: %s", *viter);

					return FALSE;
				}
			}
#ifdef HAVE_PROGRESSIVE_JPEG
			else if (strcmp (*kiter, "progressive") == 0) {
				if (strcmp (*viter, "yes") == 0)
					progressive = TRUE;
				else if (strcmp (*viter, "no") == 0)
					progressive = FALSE;
				else {
					g_set_error (error,
						     GDK_PIXBUF_ERROR,
						     GDK_PIXBUF_ERROR_BAD_OPTION,
						     "JPEG progressive option must be 'yes' or 'no', value is: %s", *viter);

					return FALSE;
				}
			}
#endif
			else {
				g_warning ("Bad option name '%s' passed to JPEG saver", *kiter);
				return FALSE;
			}

			++kiter;
			++viter;
		}
	}

	rowstride = cairo_image_surface_get_stride (image);
	w = cairo_image_surface_get_width (image);
	h = cairo_image_surface_get_height (image);
	pixels = cairo_image_surface_get_data (image);
	g_return_val_if_fail (pixels != NULL, FALSE);

	/* allocate a small buffer to convert image data */

	buf = g_try_malloc (w * 3 * sizeof (guchar));
	if (! buf) {
		g_set_error (error,
			     GDK_PIXBUF_ERROR,
			     GDK_PIXBUF_ERROR_INSUFFICIENT_MEMORY,
			     "Couldn't allocate memory for loading JPEG file");
		return FALSE;
	}

	/* set up error handling */

	cinfo.err = jpeg_std_error (&(jerr.pub));
	jerr.pub.error_exit = fatal_error_handler;
	jerr.pub.output_message = output_message_handler;
	jerr.error = error;
	if (sigsetjmp (jerr.setjmp_buffer, 1)) {
		jpeg_destroy_compress (&cinfo);
		g_free (buf);
		return FALSE;
	}

	/* setup compress params */

	jpeg_create_compress (&cinfo);
	_jpeg_memory_dest (&cinfo, (void **)buffer, buffer_size);

	cinfo.image_width      = w;
	cinfo.image_height     = h;
	cinfo.input_components = 3;
	cinfo.in_color_space   = JCS_RGB;

	/* set up jepg compression parameters */

	jpeg_set_defaults (&cinfo);
	jpeg_set_quality (&cinfo, quality, TRUE);
	cinfo.smoothing_factor = smoothing;
	cinfo.optimize_coding = optimize;

#ifdef HAVE_PROGRESSIVE_JPEG
	if (progressive)
		jpeg_simple_progression (&cinfo);
#endif /* HAVE_PROGRESSIVE_JPEG */

	jpeg_start_compress (&cinfo, TRUE);

	/* go one scanline at a time... and save */
	while (cinfo.next_scanline < cinfo.image_height) {
		JSAMPROW *jbuf;

		/* convert scanline from RGBA to RGB packed */

		_cairo_copy_line_as_rgba_big_endian (buf, pixels, w, FALSE);

		/* write scanline */
		jbuf = (JSAMPROW *)(&buf);
		jpeg_write_scanlines (&cinfo, jbuf, 1);

		pixels += rowstride;
	}

	/* finish off */

	jpeg_finish_compress (&cinfo);
	jpeg_destroy_compress (&cinfo);
	g_free (buf);

	return TRUE;
}


#endif /* HAVE_LIBJPEG */


static gboolean
gth_image_saver_jpeg_save_image (GthImageSaver  *base,
				 GthImage       *image,
				 char          **buffer,
				 gsize          *buffer_size,
				 const char     *mime_type,
				 GCancellable   *cancellable,
				 GError        **error)
{
#ifdef HAVE_LIBJPEG
	GthImageSaverJpeg  *self = GTH_IMAGE_SAVER_JPEG (base);
	char              **option_keys;
	char              **option_values;
	int                 i = -1;
	int                 i_value;
	cairo_surface_t    *surface;
	gboolean            result;

	option_keys = g_malloc (sizeof (char *) * 5);
	option_values = g_malloc (sizeof (char *) * 5);

	i++;
	i_value = g_settings_get_int (self->priv->settings, PREF_JPEG_QUALITY);
	option_keys[i] = g_strdup ("quality");
	option_values[i] = g_strdup_printf ("%d", i_value);

	i++;
	i_value = g_settings_get_int (self->priv->settings, PREF_JPEG_SMOOTHING);
	option_keys[i] = g_strdup ("smooth");
	option_values[i] = g_strdup_printf ("%d", i_value);

	i++;
	i_value = g_settings_get_boolean (self->priv->settings, PREF_JPEG_OPTIMIZE);
	option_keys[i] = g_strdup ("optimize");
	option_values[i] = g_strdup (i_value != 0 ? "yes" : "no");

	i++;
	i_value = g_settings_get_boolean (self->priv->settings, PREF_JPEG_PROGRESSIVE);
	option_keys[i] = g_strdup ("progressive");
	option_values[i] = g_strdup (i_value != 0 ? "yes" : "no");

	i++;
	option_keys[i] = NULL;
	option_values[i] = NULL;

	surface = gth_image_get_cairo_surface (image);
	result = _cairo_surface_write_as_jpeg (surface,
					       buffer,
					       buffer_size,
					       option_keys,
					       option_values,
					       error);

	cairo_surface_destroy (surface);
	g_strfreev (option_keys);
	g_strfreev (option_values);

#else /* ! HAVE_LIBJPEG */

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

#endif /* HAVE_LIBJPEG */

	return result;
}


static void
gth_image_saver_jpeg_class_init (GthImageSaverJpegClass *klass)
{
	GObjectClass        *object_class;
	GthImageSaverClass *image_saver_class;

	g_type_class_add_private (klass, sizeof (GthImageSaverJpegPrivate));

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = gth_image_saver_jpeg_finalize;

	image_saver_class = GTH_IMAGE_SAVER_CLASS (klass);
	image_saver_class->id = "jpeg";
	image_saver_class->display_name = _("JPEG");
	image_saver_class->mime_type = "image/jpeg";
	image_saver_class->extensions = "jpeg jpg";
	image_saver_class->get_default_ext = gth_image_saver_jpeg_get_default_ext;
	image_saver_class->get_control = gth_image_saver_jpeg_get_control;
	image_saver_class->save_options = gth_image_saver_jpeg_save_options;
	image_saver_class->can_save = gth_image_saver_jpeg_can_save;
	image_saver_class->save_image = gth_image_saver_jpeg_save_image;
}


static void
gth_image_saver_jpeg_init (GthImageSaverJpeg *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_IMAGE_SAVER_JPEG, GthImageSaverJpegPrivate);
	self->priv->settings = g_settings_new (GTHUMB_IMAGE_SAVERS_JPEG_SCHEMA);
	self->priv->builder = NULL;
	self->priv->default_ext = NULL;
}
