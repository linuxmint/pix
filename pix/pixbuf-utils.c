/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001-2009 The Free Software Foundation, Inc.
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
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#define GDK_PIXBUF_ENABLE_BACKEND 1
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "cairo-utils.h"
#include "pixbuf-utils.h"
#include "gimp-op.h"


GdkPixbuf *
_gdk_pixbuf_new_from_cairo_context (cairo_t *cr)
{
	return _gdk_pixbuf_new_from_cairo_surface (cairo_get_target (cr));
}


/* Started from from http://www.gtkforums.com/about5204.html
 * Author: tadeboro */
GdkPixbuf *
_gdk_pixbuf_new_from_cairo_surface (cairo_surface_t *surface)
{
	int            width;
	int            height;
	int            s_stride;
	unsigned char *s_pixels;
	GdkPixbuf     *pixbuf;
	int            p_stride;
	guchar        *p_pixels;
	int            p_n_channels;

	if (surface == NULL)
		return NULL;

	if (cairo_surface_status (surface) != CAIRO_STATUS_SUCCESS)
		return NULL;

	width = cairo_image_surface_get_width (surface);
	height = cairo_image_surface_get_height (surface);
	s_stride = cairo_image_surface_get_stride (surface);
	s_pixels = _cairo_image_surface_flush_and_get_data (surface);

	pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, _cairo_image_surface_get_has_alpha (surface), 8, width, height);
	p_stride = gdk_pixbuf_get_rowstride (pixbuf);
	p_pixels = gdk_pixbuf_get_pixels (pixbuf);
	p_n_channels = gdk_pixbuf_get_n_channels (pixbuf);

	while (height--) {
		guchar *s_iter = s_pixels;
	        guchar *p_iter = p_pixels;
	        int     i, temp;

	        for (i = 0; i < width; i++) {
	        	gdouble alpha_factor = (gdouble) 0xff / s_iter[CAIRO_ALPHA];

	        	p_iter[0] = CLAMP_PIXEL (alpha_factor * s_iter[CAIRO_RED]);
	        	p_iter[1] = CLAMP_PIXEL (alpha_factor * s_iter[CAIRO_GREEN]);
	        	p_iter[2] = CLAMP_PIXEL (alpha_factor * s_iter[CAIRO_BLUE]);
	        	if (p_n_channels == 4)
	        		p_iter[3] = s_iter[CAIRO_ALPHA];

	        	s_iter += 4;
	        	p_iter += p_n_channels;
		}

		s_pixels += s_stride;
		p_pixels += p_stride;
	}

	/* copy the known metadata from the surface to the pixbuf */

	{
		cairo_surface_metadata_t *metadata;

		metadata = _cairo_image_surface_get_metadata (surface);
		if ((metadata->thumbnail.image_width > 0) && (metadata->thumbnail.image_height > 0)) {
			char *value;

			value = g_strdup_printf ("%d", metadata->thumbnail.image_width);
			gdk_pixbuf_set_option (pixbuf, "tEXt::Thumb::Image::Width", value);
			g_free (value);

			value = g_strdup_printf ("%d", metadata->thumbnail.image_height);
			gdk_pixbuf_set_option (pixbuf, "tEXt::Thumb::Image::Height", value);
			g_free (value);
		}
	}

	return pixbuf;
}


/* The gdk_pixbuf scaling routines do not handle large-ratio downscaling
   very well. Memory usage explodes and the application may freeze or crash.
   For scale-down ratios in excess of 100, do the scale in two steps.
   It is faster and safer that way. See bug 80925 for background info. */
GdkPixbuf*
_gdk_pixbuf_scale_simple_safe (const GdkPixbuf *src,
			       int              dest_width,
			       int              dest_height,
			       GdkInterpType    interp_type)
{
	GdkPixbuf* temp_pixbuf1;
	GdkPixbuf* temp_pixbuf2;
	int        x_ratio, y_ratio;
	int        temp_width = dest_width, temp_height = dest_height;

	g_assert (dest_width >= 1);
	g_assert (dest_height >= 1);

	x_ratio = gdk_pixbuf_get_width (src) / dest_width;
	y_ratio = gdk_pixbuf_get_height (src) / dest_height;

	if (x_ratio > 100)
		/* Scale down to 10x the requested size first. */
		temp_width = 10 * dest_width;

	if (y_ratio > 100)
		/* Scale down to 10x the requested size first. */
		temp_height = 10 * dest_height;

	if ( (temp_width != dest_width) || (temp_height != dest_height)) {
		temp_pixbuf1 = gdk_pixbuf_scale_simple (src, temp_width, temp_height, interp_type);
		temp_pixbuf2 = gdk_pixbuf_scale_simple (temp_pixbuf1, dest_width, dest_height, interp_type);
		g_object_unref (temp_pixbuf1);
	} else
		temp_pixbuf2 = gdk_pixbuf_scale_simple (src, dest_width, dest_height, interp_type);

	return temp_pixbuf2;
}


/*
 * Returns a transformed image.
 */
GdkPixbuf *
_gdk_pixbuf_transform (GdkPixbuf    *src,
		       GthTransform  transform)
{
	GdkPixbuf *temp = NULL, *dest = NULL;

	if (src == NULL)
		return NULL;

	switch (transform) {
	case GTH_TRANSFORM_NONE:
		dest = src;
		g_object_ref (dest);
		break;
	case GTH_TRANSFORM_FLIP_H:
		dest = gdk_pixbuf_flip (src, TRUE);
		break;
	case GTH_TRANSFORM_ROTATE_180:
		dest = gdk_pixbuf_rotate_simple (src, GDK_PIXBUF_ROTATE_UPSIDEDOWN);
		break;
	case GTH_TRANSFORM_FLIP_V:
		dest = gdk_pixbuf_flip (src, FALSE);
		break;
	case GTH_TRANSFORM_TRANSPOSE:
		temp = gdk_pixbuf_rotate_simple (src, GDK_PIXBUF_ROTATE_CLOCKWISE);
		dest = gdk_pixbuf_flip (temp, TRUE);
		g_object_unref (temp);
		break;
	case GTH_TRANSFORM_ROTATE_90:
		dest = gdk_pixbuf_rotate_simple (src, GDK_PIXBUF_ROTATE_CLOCKWISE);
		break;
	case GTH_TRANSFORM_TRANSVERSE:
		temp = gdk_pixbuf_rotate_simple (src, GDK_PIXBUF_ROTATE_CLOCKWISE);
		dest = gdk_pixbuf_flip (temp, FALSE);
		g_object_unref (temp);
		break;
	case GTH_TRANSFORM_ROTATE_270:
		dest = gdk_pixbuf_rotate_simple (src, GDK_PIXBUF_ROTATE_COUNTERCLOCKWISE);
		break;
	default:
		dest = src;
		g_object_ref (dest);
		break;
	}

	return dest;
}


char *
_gdk_pixbuf_get_type_from_mime_type (const char *mime_type)
{
	if (mime_type == NULL)
		return NULL;

	if (g_str_has_prefix (mime_type, "image/x-"))
		return g_strdup (mime_type + strlen ("image/x-"));
	else if (g_str_has_prefix (mime_type, "image/"))
		return g_strdup (mime_type + strlen ("image/"));
	else
		return g_strdup (mime_type);
}


gboolean
_gdk_pixbuf_mime_type_is_readable (const char *mime_type)
{
	GSList   *formats;
	GSList   *scan;
	gboolean  result;

	if (mime_type == NULL)
		return FALSE;

	result = FALSE;
	formats = gdk_pixbuf_get_formats ();
	for (scan = formats; ! result && scan; scan = scan->next) {
		GdkPixbufFormat  *format = scan->data;
		char            **mime_types;
		int               i;

		if (gdk_pixbuf_format_is_disabled (format))
			continue;

		mime_types = gdk_pixbuf_format_get_mime_types (format);
		for (i = 0; mime_types[i] != NULL; i++) {
			if (strcmp (mime_type, mime_types[i]) == 0) {
				result = TRUE;
				break;
			}
		}
	}

	g_slist_free (formats);

	return result;
}
