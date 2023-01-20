/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2010 Free Software Foundation, Inc.
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
#include "cairo-utils.h"
#include "cairo-scale.h"
#include "gth-image-utils.h"


G_DEFINE_BOXED_TYPE (GthCairoSurface,
		     gth_cairo_surface,
		     (GBoxedCopyFunc) cairo_surface_reference,
		     (GBoxedFreeFunc) cairo_surface_destroy)


const unsigned char cairo_channel[4] = { CAIRO_RED, CAIRO_GREEN, CAIRO_BLUE, CAIRO_ALPHA };


static cairo_user_data_key_t surface_metadata_key;


static void
surface_metadata_free (void *data)
{
	cairo_surface_metadata_t *metadata = data;
	g_free (metadata);
}


inline int
_cairo_multiply_alpha (int color,
		       int alpha)
{
	int temp = (alpha * color) + 0x80;
	return ((temp + (temp >> 8)) >> 8);
}


gboolean
_cairo_rectangle_contains_point (cairo_rectangle_int_t *rect,
			 	 int                    x,
			 	 int                    y)
{
	return ((x >= rect->x)
		&& (y >= rect->y)
		&& (x <= rect->x + rect->width)
		&& (y <= rect->y + rect->height));
}


void
_gdk_color_to_cairo_color (GdkColor *g_color,
			   GdkRGBA  *c_color)
{
	c_color->red = (double) g_color->red / 65535;
	c_color->green = (double) g_color->green / 65535;
	c_color->blue = (double) g_color->blue / 65535;
	c_color->alpha = 1.0;
}


void
_gdk_color_to_cairo_color_255 (GdkColor          *g_color,
			       cairo_color_255_t *c_color)
{
	c_color->r = (guchar) 255.0 * g_color->red / 65535.0;
	c_color->g = (guchar) 255.0 * g_color->green / 65535.0;
	c_color->b = (guchar) 255.0 * g_color->blue / 65535.0;
	c_color->a = 0xff;
}


void
_gdk_rgba_to_cairo_color_255 (GdkRGBA           *g_color,
			      cairo_color_255_t *c_color)
{
	c_color->r = (guchar) 255.0 * g_color->red;
	c_color->g = (guchar) 255.0 * g_color->green;
	c_color->b = (guchar) 255.0 * g_color->blue;
	c_color->a = (guchar) 255.0 * g_color->alpha;
}


void
_cairo_metadata_set_has_alpha (cairo_surface_metadata_t	*metadata,
			       gboolean                  has_alpha)
{
	g_return_if_fail (metadata != NULL);

	metadata->valid_data |= _CAIRO_METADATA_FLAG_HAS_ALPHA;
	metadata->has_alpha = has_alpha ? TRUE : FALSE;
}


void
_cairo_metadata_set_original_size (cairo_surface_metadata_t *metadata,
				   int                       width,
				   int                       height)
{
	g_return_if_fail (metadata != NULL);

	metadata->valid_data |= _CAIRO_METADATA_FLAG_ORIGINAL_SIZE;
	metadata->original_width = width;
	metadata->original_height = height;
}


void
_cairo_metadata_set_thumbnail_size (cairo_surface_metadata_t *metadata,
				    int                       width,
				    int                       height)
{
	g_return_if_fail (metadata != NULL);

	metadata->valid_data |= _CAIRO_METADATA_FLAG_THUMBNAIL_SIZE;
	metadata->thumbnail.image_width = width;
	metadata->thumbnail.image_height = height;
}


void
_cairo_clear_surface (cairo_surface_t  **surface)
{
	if (surface == NULL)
		return;
	if (*surface == NULL)
		return;

	cairo_surface_destroy (*surface);
	*surface = NULL;
}


unsigned char *
_cairo_image_surface_flush_and_get_data (cairo_surface_t *surface)
{
	g_return_val_if_fail (surface != NULL, NULL);

	cairo_surface_flush (surface);
	return cairo_image_surface_get_data (surface);
}


static void
_cairo_surface_metadata_init (cairo_surface_metadata_t *metadata)
{
	g_return_if_fail (metadata != NULL);

	metadata->valid_data = _CAIRO_METADATA_FLAG_NONE;
	metadata->has_alpha = FALSE;
	metadata->original_width = 0;
	metadata->original_height = 0;
	metadata->thumbnail.image_width = 0;
	metadata->thumbnail.image_height = 0;
}


cairo_surface_metadata_t *
_cairo_image_surface_get_metadata (cairo_surface_t *surface)
{
	cairo_surface_metadata_t *metadata;

	g_return_val_if_fail (surface != NULL, NULL);

	metadata = cairo_surface_get_user_data (surface, &surface_metadata_key);
	if (metadata == NULL) {
		metadata = g_new0 (cairo_surface_metadata_t, 1);
		_cairo_surface_metadata_init (metadata);
		cairo_surface_set_user_data (surface, &surface_metadata_key, metadata, surface_metadata_free);
	}

	return metadata;
}


void
_cairo_image_surface_copy_metadata (cairo_surface_t *src,
				    cairo_surface_t *dest)
{
	cairo_surface_metadata_t *src_metadata;
	cairo_surface_metadata_t *dest_metadata;

	g_return_if_fail (src != NULL);
	g_return_if_fail (dest != NULL);

	src_metadata = _cairo_image_surface_get_metadata (src);
	dest_metadata = _cairo_image_surface_get_metadata (dest);

	dest_metadata->valid_data = src_metadata->valid_data;
	dest_metadata->has_alpha = src_metadata->has_alpha;
	dest_metadata->original_width = src_metadata->original_width;
	dest_metadata->original_height = src_metadata->original_height;
	dest_metadata->thumbnail.image_width = src_metadata->thumbnail.image_width;
	dest_metadata->thumbnail.image_height = src_metadata->thumbnail.image_height;

}


void
_cairo_image_surface_clear_metadata (cairo_surface_t *surface)
{
	cairo_surface_metadata_t *metadata;

	g_return_if_fail (surface != NULL);

	metadata = _cairo_image_surface_get_metadata (surface);
	_cairo_surface_metadata_init (metadata);
}


gboolean
_cairo_image_surface_get_has_alpha (cairo_surface_t *surface)
{
	cairo_surface_metadata_t *metadata;
	gboolean                  has_alpha;
	int                       width;
	int                       height;
	int                       row_stride;
	guchar                   *row;
	int                       h, w;

	if (surface == NULL)
		return FALSE;

	metadata = _cairo_image_surface_get_metadata (surface);
	if ((metadata != NULL) && (metadata->valid_data & _CAIRO_METADATA_FLAG_HAS_ALPHA))
		return metadata->has_alpha;

	has_alpha = FALSE;
	if (cairo_image_surface_get_format (surface) == CAIRO_FORMAT_ARGB32) {
		/* search an alpha value lower than 255 */

		width = cairo_image_surface_get_width (surface);
		height = cairo_image_surface_get_height (surface);
		row_stride = cairo_image_surface_get_stride (surface);
		row = _cairo_image_surface_flush_and_get_data (surface);

		for (h = 0; ! has_alpha && (h < height); h++) {
			guchar *pixel = row;
			for (w = 0; w < width; w++) {
				if (pixel[CAIRO_ALPHA] < 255) {
					has_alpha = TRUE;
					break;
				}
				pixel += 4;
			}
			row += row_stride;
		}
	}
	_cairo_metadata_set_has_alpha (metadata, has_alpha);

	return has_alpha;
}


gboolean
_cairo_image_surface_get_original_size (cairo_surface_t *surface,
					int             *original_width,
					int             *original_height)
{
	cairo_surface_metadata_t *metadata;

	if (surface == NULL)
		return FALSE;

	metadata = cairo_surface_get_user_data (surface, &surface_metadata_key);
	if (metadata == NULL)
		return FALSE;

	if ((metadata->valid_data & _CAIRO_METADATA_FLAG_ORIGINAL_SIZE) == 0)
		return FALSE;

	if (original_width)
		*original_width = metadata->original_width;
	if (original_height)
		*original_height = metadata->original_height;

	return TRUE;
}


cairo_surface_t *
_cairo_image_surface_create (cairo_format_t format,
			     int            width,
			     int            height)
{
	cairo_surface_t *result;
	cairo_status_t   status;

	result = cairo_image_surface_create (format, width, height);
	status = cairo_surface_status (result);
	if (status != CAIRO_STATUS_SUCCESS) {
		g_warning ("_cairo_image_surface_create: could not create the surface: %s", cairo_status_to_string (status));
		cairo_surface_destroy (result);
		return NULL;
	}

	return result;
}


cairo_surface_t *
_cairo_image_surface_copy (cairo_surface_t *source)
{
	cairo_surface_t *result;
	unsigned char   *p_source;
	unsigned char   *p_destination;

	if (source == NULL)
		return NULL;

	result = _cairo_image_surface_create (cairo_image_surface_get_format (source),
					      cairo_image_surface_get_width (source),
					      cairo_image_surface_get_height (source));
	if (result == NULL)
		return NULL;

	p_source = _cairo_image_surface_flush_and_get_data (source);
	p_destination = _cairo_image_surface_flush_and_get_data (result);
	memcpy (p_destination, p_source, cairo_image_surface_get_stride (source) * cairo_image_surface_get_height (source));
	cairo_surface_mark_dirty (result);

	return result;
}


cairo_surface_t *
_cairo_image_surface_copy_subsurface (cairo_surface_t *source,
				      int              src_x,
				      int              src_y,
				      int              width,
				      int              height)
{
	cairo_surface_t *destination;
	cairo_status_t   status;
	int              source_stride;
	int              destination_stride;
	unsigned char   *p_source;
	unsigned char   *p_destination;
	int              row_size;

	g_return_val_if_fail (source != NULL, NULL);
	g_return_val_if_fail (src_x + width <= cairo_image_surface_get_width (source), NULL);
	g_return_val_if_fail (src_y + height <= cairo_image_surface_get_height (source), NULL);

	destination = cairo_image_surface_create (cairo_image_surface_get_format (source), width, height);
	status = cairo_surface_status (destination);
	if (status != CAIRO_STATUS_SUCCESS) {
		g_warning ("_cairo_image_surface_copy_subsurface: could not create the surface: %s", cairo_status_to_string (status));
		cairo_surface_destroy (destination);
		return NULL;
	}

	source_stride = cairo_image_surface_get_stride (source);
	destination_stride = cairo_image_surface_get_stride (destination);
	p_source = _cairo_image_surface_flush_and_get_data (source) + (src_y * source_stride) + (src_x * 4);
	p_destination = _cairo_image_surface_flush_and_get_data (destination);
	row_size = width * 4;
	while (height-- > 0) {
		memcpy (p_destination, p_source, row_size);

		p_source += source_stride;
		p_destination += destination_stride;
	}

	cairo_surface_mark_dirty (destination);

	return destination;
}


cairo_surface_t *
_cairo_image_surface_create_from_rgba (const guchar *pixels,
				       int           width,
				       int           height,
				       int           p_stride,
				       gboolean      has_alpha)
{
	cairo_surface_t          *surface;
	int                       s_stride;
	unsigned char            *s_pixels;
	cairo_surface_metadata_t *metadata;
	int                       h, w;
	guint32                   pixel;
	guchar                    r, g, b, a;

	if (pixels == NULL)
		return NULL;

	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
	s_stride = cairo_image_surface_get_stride (surface);
	s_pixels = _cairo_image_surface_flush_and_get_data (surface);

	metadata = _cairo_image_surface_get_metadata (surface);
	_cairo_metadata_set_has_alpha (metadata, has_alpha);

	if (has_alpha) {
		guchar       *s_iter;
		const guchar *p_iter;

		for (h = 0; h < height; h++) {
			s_iter = s_pixels;
			p_iter = pixels;

			for (w = 0; w < width; w++) {
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
				memcpy (s_iter, &pixel, sizeof (guint32));

				s_iter += 4;
				p_iter += 4;
			}

			s_pixels += s_stride;
			pixels += p_stride;
		}
	}
	else {
		guchar       *s_iter;
		const guchar *p_iter;

		for (h = 0; h < height; h++) {
			s_iter = s_pixels;
			p_iter = pixels;

			for (w = 0; w < width; w++) {
				pixel = CAIRO_RGBA_TO_UINT32 (p_iter[0], p_iter[1], p_iter[2], 0xff);
				memcpy (s_iter, &pixel, sizeof (guint32));

				s_iter += 4;
				p_iter += 3;
			}

			s_pixels += s_stride;
			pixels += p_stride;
		}
	}

	cairo_surface_mark_dirty (surface);

	return surface;
}


cairo_surface_t *
_cairo_image_surface_create_from_pixbuf (GdkPixbuf *pixbuf)
{
	guchar *pixels;
	int     width;
	int     height;
	int     row_stride;
	int     n_channels;

	if (pixbuf == NULL)
		return NULL;

	g_object_get (G_OBJECT (pixbuf),
		      "pixels", &pixels,
		      "width", &width,
		      "height", &height,
		      "rowstride", &row_stride,
		      "n-channels", &n_channels,
		      NULL );

	return _cairo_image_surface_create_from_rgba (pixels, width, height, row_stride, n_channels == 4);
}


cairo_surface_t *
_cairo_image_surface_create_compatible (cairo_surface_t *surface)
{
	return cairo_image_surface_create (cairo_image_surface_get_format (surface),
					   cairo_image_surface_get_width (surface),
					   cairo_image_surface_get_height (surface));
}


void
_cairo_image_surface_transform_get_steps (cairo_format_t  format,
					  int             width,
					  int             height,
					  GthTransform    transform,
					  int            *destination_width_p,
					  int            *destination_height_p,
					  int            *line_start_p,
					  int            *line_step_p,
					  int            *pixel_step_p)
{
	int destination_stride;
	int destination_width = 0;
	int destination_height = 0;
	int line_start = 0;
	int line_step = 0;
	int pixel_step = 0;

	switch (transform) {
	case GTH_TRANSFORM_NONE:
	default:
		destination_width = width;
		destination_height = height;
		destination_stride = cairo_format_stride_for_width (format, destination_width);
		line_start = 0;
		line_step = destination_stride;
		pixel_step = 4;
		break;

	case GTH_TRANSFORM_FLIP_H:
		destination_width = width;
		destination_height = height;
		destination_stride = cairo_format_stride_for_width (format, destination_width);
		line_start = (destination_width - 1) * 4;
		line_step = destination_stride;
		pixel_step = -4;
		break;

	case GTH_TRANSFORM_ROTATE_180:
		destination_width = width;
		destination_height = height;
		destination_stride = cairo_format_stride_for_width (format, destination_width);
		line_start = ((destination_height - 1) * destination_stride) + ((destination_width - 1) * 4);
		line_step = -destination_stride;
		pixel_step = -4;
		break;

	case GTH_TRANSFORM_FLIP_V:
		destination_width = width;
		destination_height = height;
		destination_stride = cairo_format_stride_for_width (format, destination_width);
		line_start = (destination_height - 1) * destination_stride;
		line_step = -destination_stride;
		pixel_step = 4;
		break;

	case GTH_TRANSFORM_TRANSPOSE:
		destination_width = height;
		destination_height = width;
		destination_stride = cairo_format_stride_for_width (format, destination_width);
		line_start = 0;
		line_step = 4;
		pixel_step = destination_stride;
		break;

	case GTH_TRANSFORM_ROTATE_90:
		destination_width = height;
		destination_height = width;
		destination_stride = cairo_format_stride_for_width (format, destination_width);
		line_start = (destination_width - 1) * 4;
		line_step = -4;
		pixel_step = destination_stride;
		break;

	case GTH_TRANSFORM_TRANSVERSE:
		destination_width = height;
		destination_height = width;
		destination_stride = cairo_format_stride_for_width (format, destination_width);
		line_start = ((destination_height - 1) * destination_stride) + ((destination_width - 1) * 4);
		line_step = -4;
		pixel_step = -destination_stride;
		break;

	case GTH_TRANSFORM_ROTATE_270:
		destination_width = height;
		destination_height = width;
		destination_stride = cairo_format_stride_for_width (format, destination_width);
		line_start = (destination_height - 1) * destination_stride;
		line_step = 4;
		pixel_step = -destination_stride;
		break;
	}

	if (destination_width_p != NULL)
		*destination_width_p = destination_width;
	if (destination_height_p != NULL)
		*destination_height_p = destination_height;
	if (line_start_p != NULL)
		*line_start_p = line_start;
	if (line_step_p != NULL)
		*line_step_p = line_step;
	if (pixel_step_p != NULL)
		*pixel_step_p = pixel_step;
}


cairo_surface_t *
_cairo_image_surface_transform (cairo_surface_t *source,
				GthTransform     transform)
{
	cairo_surface_t *destination = NULL;
	cairo_format_t   format;
	int              width;
	int              height;
	int              source_stride;
	int              destination_width;
	int              destination_height;
	int              line_start;
	int              line_step;
	int              pixel_step;
	unsigned char   *p_source_line;
	unsigned char   *p_destination_line;
	unsigned char   *p_source;
	unsigned char   *p_destination;
	int              x;

	if (source == NULL)
		return NULL;

	format = cairo_image_surface_get_format (source);
	width = cairo_image_surface_get_width (source);
	height = cairo_image_surface_get_height (source);
	source_stride = cairo_image_surface_get_stride (source);

	_cairo_image_surface_transform_get_steps (format,
						  width,
						  height,
						  transform,
						  &destination_width,
						  &destination_height,
						  &line_start,
						  &line_step,
						  &pixel_step);

	destination = cairo_image_surface_create (format, destination_width, destination_height);
	p_source_line = _cairo_image_surface_flush_and_get_data (source);
	p_destination_line = _cairo_image_surface_flush_and_get_data (destination) + line_start;
	while (height-- > 0) {
		p_source = p_source_line;
		p_destination = p_destination_line;
		for (x = 0; x < width; x++) {
			memcpy (p_destination, p_source, 4);
			p_source += 4;
			p_destination += pixel_step;
		}
		p_source_line += source_stride;
		p_destination_line += line_step;
	}

	cairo_surface_mark_dirty (destination);

	return destination;
}


cairo_surface_t *
_cairo_image_surface_color_shift (cairo_surface_t *image,
				  int              shift)
{
	cairo_surface_t *shifted;
	int              i, j;
	int              width, height, src_stride, dest_stride;
	guchar          *src_pixels, *src_row, *src_pixel;
	guchar          *dest_pixels, *dest_row, *dest_pixel;
	int              val, temp;
	guchar           r, g, b, a;

	shifted = _cairo_image_surface_create_compatible (image);

	width       = cairo_image_surface_get_width (image);
	height      = cairo_image_surface_get_height (image);
	src_stride  = cairo_image_surface_get_stride (image);
	src_pixels  = _cairo_image_surface_flush_and_get_data (image);
	dest_stride = cairo_image_surface_get_stride (shifted);
	dest_pixels = _cairo_image_surface_flush_and_get_data (shifted);

	src_row = src_pixels;
	dest_row = dest_pixels;
	for (i = 0; i < height; i++) {
		src_pixel = src_row;
		dest_pixel  = dest_row;

		for (j = 0; j < width; j++) {
			CAIRO_GET_RGBA (src_pixel, r, g, b, a);

			val = r + shift;
			r = CLAMP (val, 0, 255);

			val = g + shift;
			g = CLAMP (val, 0, 255);

			val = b + shift;
			b = CLAMP (val, 0, 255);

			CAIRO_SET_RGBA (dest_pixel, r, g, b, a);

			src_pixel += 4;
			dest_pixel += 4;
		}

		src_row += src_stride;
		dest_row += dest_stride;
	}

	cairo_surface_mark_dirty (shifted);

	return shifted;
}


void
_cairo_copy_line_as_rgba_big_endian (guchar *dest,
				     guchar *src,
				     guint   width,
				     guint   alpha)
{
	guint x;

	if (alpha) {
		int temp;

		for (x = 0; x < width; x++) {
			CAIRO_GET_RGBA (src, dest[0], dest[1], dest[2], dest[3]);

			src += 4;
			dest += 4;
		}
	}
	else {
		for (x = 0; x < width; x++) {
			CAIRO_GET_RGB (src, dest[0], dest[1], dest[2]);

			src += 4;
			dest += 3;
		}
	}
}


void
_cairo_copy_line_as_rgba_little_endian (guchar *dest,
					guchar *src,
					guint   width,
					guint   alpha)
{
	guint x;

	if (alpha) {
		int r, g, b, a, temp;

		for (x = 0; x < width; x++) {
			CAIRO_GET_RGBA (src, r, g, b, a);
			dest[0] = b;
			dest[1] = g;
			dest[2] = r;
			dest[3] = a;

			src += 4;
			dest += 4;
		}
	}
	else {
		for (x = 0; x < width; x++) {
			dest[0] = src[CAIRO_BLUE];
			dest[1] = src[CAIRO_GREEN];
			dest[2] = src[CAIRO_RED];

			src += 4;
			dest += 3;
		}
	}
}


void
_cairo_paint_full_gradient (cairo_surface_t *surface,
			    GdkRGBA         *h_color1,
			    GdkRGBA         *h_color2,
			    GdkRGBA         *v_color1,
			    GdkRGBA         *v_color2)
{
	cairo_color_255_t  hcolor1;
	cairo_color_255_t  hcolor2;
	cairo_color_255_t  vcolor1;
	cairo_color_255_t  vcolor2;
	int                width;
	int                height;
	int                s_stride;
	unsigned char     *s_pixels;
	int                h, w;
	double             x, y;
	double             x_y, x_1_y, y_1_x, _1_x_1_y;
	guchar             red, green, blue;

	if (cairo_surface_status (surface) != CAIRO_STATUS_SUCCESS)
		return;

	_gdk_rgba_to_cairo_color_255 (h_color1, &hcolor1);
	_gdk_rgba_to_cairo_color_255 (h_color2, &hcolor2);
	_gdk_rgba_to_cairo_color_255 (v_color1, &vcolor1);
	_gdk_rgba_to_cairo_color_255 (v_color2, &vcolor2);

	width = cairo_image_surface_get_width (surface);
	height = cairo_image_surface_get_height (surface);
	s_stride = cairo_image_surface_get_stride (surface);
	s_pixels = _cairo_image_surface_flush_and_get_data (surface);

	for (h = 0; h < height; h++) {
		guchar *s_iter = s_pixels;

	        x = (double) (height - h) / height;

	        for (w = 0; w < width; w++) {
	        	y        = (double) (width - w) / width;
			x_y      = x * y;
			x_1_y    = x * (1.0 - y);
			y_1_x    = y * (1.0 - x);
			_1_x_1_y = (1.0 - x) * (1.0 - y);

			red   = hcolor1.r * x_y + hcolor2.r * x_1_y + vcolor1.r * y_1_x + vcolor2.r * _1_x_1_y;
			green = hcolor1.g * x_y + hcolor2.g * x_1_y + vcolor1.g * y_1_x + vcolor2.g * _1_x_1_y;
			blue  = hcolor1.b * x_y + hcolor2.b * x_1_y + vcolor1.b * y_1_x + vcolor2.b * _1_x_1_y;

			CAIRO_SET_RGB (s_iter, red, green, blue);

			s_iter += 4;
		}

		s_pixels += s_stride;
	}

	cairo_surface_mark_dirty (surface);
}


void
_cairo_draw_rounded_box (cairo_t *cr,
			 double   x,
			 double   y,
			 double   w,
			 double   h,
			 double   r)
{
	if (r == 0) {
		cairo_rectangle (cr, x, y, w, h);
	}
	else {
		cairo_move_to (cr, x, y + r);
		cairo_arc (cr, x + r, y + r, r, 1.0 * M_PI, 1.5 * M_PI);
		cairo_rel_line_to (cr, w - (r * 2), 0);
		cairo_arc (cr, x + w - r, y + r, r, 1.5 * M_PI, 2.0 * M_PI);
		cairo_rel_line_to (cr, 0, h - (r * 2));
		cairo_arc (cr, x + w - r, y + h - r, r, 0.0 * M_PI, 0.5 * M_PI);
		cairo_rel_line_to (cr, - (w - (r * 2)), 0);
		cairo_arc (cr, x + r, y + h - r, r, 0.5 * M_PI, 1.0 * M_PI);
		cairo_rel_line_to (cr, 0, - (h - (r * 2)));
	}
}


void
_cairo_draw_drop_shadow (cairo_t *cr,
			 double   x,
			 double   y,
			 double   w,
			 double   h,
			 double   r)
{
	int i;

	cairo_save (cr);
	cairo_set_line_width (cr, 1);
	for (i = r; i >= 0; i--) {
		cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, (0.1 / r)* (r - i + 1));
		_cairo_draw_rounded_box (cr,
					 x + r - i,
					 y + r - i,
					 w + (i * 2),
					 h + (i * 2),
					 r);
		cairo_fill (cr);
	}
	cairo_restore (cr);
}


void
_cairo_draw_frame (cairo_t *cr,
		   double   x,
		   double   y,
		   double   w,
		   double   h,
		   double   r)
{
	cairo_save (cr);
	cairo_set_line_width (cr, r);
	cairo_rectangle (cr, x - (r / 2), y - (r / 2), w + r, h + r);
	cairo_stroke (cr);
	cairo_restore (cr);
}


void
_cairo_draw_slide (cairo_t  *cr,
		   double    frame_x,
		   double    frame_y,
		   double    frame_width,
		   double    frame_height,
		   double    image_width,
		   double    image_height,
		   GdkRGBA  *frame_color,
		   gboolean  draw_inner_border)
{
	const double dark_gray = 0.60;
	const double mid_gray = 0.80;
	const double darker_gray = 0.45;
	const double light_gray = 0.99;
	double       frame_x2;
	double       frame_y2;

	frame_x += 0.5;
	frame_y += 0.5;

	frame_x2 = frame_x + frame_width;
	frame_y2 = frame_y + frame_height;

	cairo_save (cr);
	cairo_set_antialias (cr, CAIRO_ANTIALIAS_NONE);
	cairo_set_line_width (cr, 1.0);

	/* background. */

	gdk_cairo_set_source_rgba (cr, frame_color);
	cairo_rectangle (cr,
			 frame_x,
			 frame_y,
			 frame_width,
			 frame_height);
	cairo_fill (cr);

	if ((image_width > 0) && (image_height > 0)) {
		double image_x, image_y;

		image_x = frame_x + (frame_width - image_width) / 2 - 0.5;
		image_y = frame_y + (frame_height - image_height) / 2 - 0.5;

		/* inner border. */

		if (draw_inner_border) {
			double image_x2, image_y2;

			cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
			cairo_rectangle (cr,
					 image_x,
					 image_y,
					 image_width,
					 image_height);
			cairo_fill (cr);

			image_x2 = image_x + image_width + 1;
			image_y2 = image_y + image_height + 1;

			cairo_set_source_rgb (cr, dark_gray, dark_gray, dark_gray);
			cairo_move_to (cr, image_x, image_y);
			cairo_line_to (cr, image_x2, image_y);
			cairo_move_to (cr, image_x, image_y);
			cairo_line_to (cr, image_x, image_y2);
			cairo_stroke (cr);

			cairo_set_source_rgb (cr, mid_gray, mid_gray, mid_gray);
			cairo_move_to (cr, image_x2, image_y);
			cairo_line_to (cr, image_x2, image_y2);
			cairo_move_to (cr, image_x, image_y2);
			cairo_line_to (cr, image_x2, image_y2);
			cairo_stroke (cr);
		}
	}

	/* outer border. */

	cairo_set_source_rgb (cr, mid_gray, mid_gray, mid_gray);
	cairo_move_to (cr, frame_x, frame_y);
	cairo_line_to (cr, frame_x2, frame_y);
	cairo_move_to (cr, frame_x, frame_y);
	cairo_line_to (cr, frame_x, frame_y2);
	cairo_stroke (cr);

	cairo_set_source_rgb (cr, darker_gray, darker_gray, darker_gray);
	cairo_move_to (cr, frame_x2, frame_y);
	cairo_line_to (cr, frame_x2, frame_y2);
	cairo_move_to (cr, frame_x, frame_y2);
	cairo_line_to (cr, frame_x2, frame_y2);
	cairo_stroke (cr);

	cairo_set_source_rgb (cr, light_gray, light_gray, light_gray);
	cairo_move_to (cr, frame_x, frame_y);
	cairo_line_to (cr, frame_x2, frame_y);
	cairo_move_to (cr, frame_x, frame_y);
	cairo_line_to (cr, frame_x, frame_y2);
	cairo_stroke (cr);

	cairo_set_source_rgb (cr, dark_gray, dark_gray, dark_gray);
	cairo_move_to (cr, frame_x2, frame_y);
	cairo_line_to (cr, frame_x2, frame_y2);
	cairo_move_to (cr, frame_x, frame_y2);
	cairo_line_to (cr, frame_x2, frame_y2);
	cairo_stroke (cr);

	cairo_restore (cr);
}


#define GOLDEN_RATIO        1.6180339887
#define GOLDER_RATIO_FACTOR (GOLDEN_RATIO / (1.0 + 2.0 * GOLDEN_RATIO))
#define GRID_STEP_1         10
#define GRID_STEP_2         (GRID_STEP_1 * 5)
#define GRID_STEP_3         (GRID_STEP_2 * 2)
#define MAX_GRID_LINES      50


void
_cairo_paint_grid (cairo_t               *cr,
		   cairo_rectangle_int_t *rectangle,
		   GthGridType            grid_type)
{
	double ux, uy;

	cairo_save (cr);

	ux = uy = 1.0;
	cairo_device_to_user_distance (cr, &ux, &uy);
	cairo_set_line_width (cr, MAX (ux, uy));

	cairo_rectangle (cr, rectangle->x - ux + 0.5, rectangle->y - uy + 0.5, rectangle->width + (ux * 2), rectangle->height + (uy * 2));
	cairo_clip (cr);

#if CAIRO_VERSION >= CAIRO_VERSION_ENCODE(1, 9, 2)
	cairo_set_operator (cr, CAIRO_OPERATOR_DIFFERENCE);
#endif

	cairo_rectangle (cr, rectangle->x + 0.5, rectangle->y + 0.5, rectangle->width - 0.5, rectangle->height - 0.5);
	cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
	cairo_stroke (cr);

	if (grid_type == GTH_GRID_NONE) {
		cairo_restore (cr);
		return;
	}

	if (grid_type == GTH_GRID_THIRDS) {
		int i;

		cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 0.60);
		for (i = 1; i < 3; i++) {
			cairo_move_to (cr, rectangle->x + rectangle->width * i / 3 + 0.5, rectangle->y + 1.5);
			cairo_line_to (cr, rectangle->x + rectangle->width * i / 3 + 0.5, rectangle->y + rectangle->height - 0.5);

			cairo_move_to (cr, rectangle->x + 1.5, rectangle->y + rectangle->height * i / 3 + 0.5);
			cairo_line_to (cr, rectangle->x + rectangle->width - 0.5, rectangle->y + rectangle->height * i / 3 + 0.5);
		}
		cairo_stroke (cr);

		cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 0.10);
		for (i = 1; i < 9; i++) {

			if (i % 3 == 0)
				continue;

			cairo_move_to (cr, rectangle->x + rectangle->width * i / 9 + 0.5, rectangle->y + 1.5);
			cairo_line_to (cr, rectangle->x + rectangle->width * i / 9 + 0.5, rectangle->y + rectangle->height - 0.5);

			cairo_move_to (cr, rectangle->x + 1.5, rectangle->y + rectangle->height * i / 9 + 0.5);
			cairo_line_to (cr, rectangle->x + rectangle->width - 0.5, rectangle->y + rectangle->height * i / 9 + 0.5);
		}
		cairo_stroke (cr);
	}
	else if (grid_type == GTH_GRID_GOLDEN) {
		cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 0.60);

		int grid_x0, grid_x1, grid_x2, grid_x3;
	        int grid_y0, grid_y1, grid_y2, grid_y3;
		int x_delta, y_delta;

		grid_x0 = rectangle->x;
		grid_x3 = rectangle->x + rectangle->width;

                grid_y0 = rectangle->y;
                grid_y3 = rectangle->y + rectangle->height;

		x_delta = rectangle->width * GOLDER_RATIO_FACTOR;
		y_delta = rectangle->height * GOLDER_RATIO_FACTOR;

		grid_x1 = grid_x0 + x_delta;
		grid_x2 = grid_x3 - x_delta;
                grid_y1 = grid_y0 + y_delta;
                grid_y2 = grid_y3 - y_delta;

		cairo_move_to (cr, grid_x1 + 0.5, grid_y0 + 0.5);
		cairo_line_to (cr, grid_x1 + 0.5, grid_y3 + 0.5);

		if (x_delta < rectangle->width / 2) {
			cairo_move_to (cr, grid_x2 + 0.5, grid_y0 + 0.5);
			cairo_line_to (cr, grid_x2 + 0.5, grid_y3 + 0.5);
		}

		cairo_move_to (cr, grid_x0 + 0.5, grid_y1 + 0.5);
		cairo_line_to (cr, grid_x3 + 0.5, grid_y1 + 0.5);

		if (y_delta < rectangle->height / 2) {
			cairo_move_to (cr, grid_x0 + 0.5, grid_y2 + 0.5);
			cairo_line_to (cr, grid_x3 + 0.5, grid_y2 + 0.5);
		}

		cairo_stroke (cr);
	}
	else if (grid_type == GTH_GRID_CENTER) {
		int i;

		cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 0.60);
		cairo_move_to (cr, rectangle->x + rectangle->width / 2 + 0.5, rectangle->y + 1.5);
		cairo_line_to (cr, rectangle->x + rectangle->width / 2 + 0.5, rectangle->y + rectangle->height - 0.5);
		cairo_move_to (cr, rectangle->x + 1.5, rectangle->y + rectangle->height / 2 + 0.5);
		cairo_line_to (cr, rectangle->x + rectangle->width - 0.5, rectangle->y + rectangle->height / 2 + 0.5);
		cairo_stroke (cr);

		cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 0.10);
		for (i = 1; i < 4; i++) {

			if (i == 2)
				continue;

			cairo_move_to (cr, rectangle->x + rectangle->width * i / 4 + 0.5, rectangle->y + 1.5);
			cairo_line_to (cr, rectangle->x + rectangle->width * i / 4 + 0.5, rectangle->y + rectangle->height - 0.5);
			cairo_move_to (cr, rectangle->x + 1.5, rectangle->y + rectangle->height * i / 4 + 0.5);
			cairo_line_to (cr, rectangle->x + rectangle->width - 0.5, rectangle->y + rectangle->height * i / 4 + 0.5);
		}
		cairo_stroke (cr);
	}
	else if (grid_type == GTH_GRID_UNIFORM) {

		int x;
		int y;

		if (rectangle->width / GRID_STEP_3 <= MAX_GRID_LINES) {
			cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 0.40);
			for (x = GRID_STEP_3; x < rectangle->width; x += GRID_STEP_3) {
				cairo_move_to (cr, rectangle->x + x + 0.5, rectangle->y + 1.5);
				cairo_line_to (cr, rectangle->x + x + 0.5, rectangle->y + rectangle->height - 0.5);
			}
			for (y = GRID_STEP_3; y < rectangle->height; y += GRID_STEP_3) {
				cairo_move_to (cr, rectangle->x + 1.5, rectangle->y + y + 0.5);
				cairo_line_to (cr, rectangle->x + rectangle->width - 0.5, rectangle->y + y + 0.5);
			}
			cairo_stroke (cr);
		}

		if (rectangle->width / GRID_STEP_2 <= MAX_GRID_LINES) {
			cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 0.20);
			for (x = GRID_STEP_2; x < rectangle->width; x += GRID_STEP_2) {
				if (x % GRID_STEP_3 == 0)
					continue;
				cairo_move_to (cr, rectangle->x + x + 0.5, rectangle->y + 1.5);
				cairo_line_to (cr, rectangle->x + x + 0.5, rectangle->y + rectangle->height - 0.5);
			}
			for (y = GRID_STEP_2; y < rectangle->height; y += GRID_STEP_2) {
				if (y % GRID_STEP_3 == 0)
					continue;
				cairo_move_to (cr, rectangle->x + 1.5, rectangle->y + y + 0.5);
				cairo_line_to (cr, rectangle->x + rectangle->width - 0.5, rectangle->y + y + 0.5);
			}
			cairo_stroke (cr);
		}

		if (rectangle->width / GRID_STEP_1 <= MAX_GRID_LINES) {
			cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 0.10);
			for (x = GRID_STEP_1; x < rectangle->width; x += GRID_STEP_1) {
				if (x % GRID_STEP_2 == 0)
					continue;
				cairo_move_to (cr, rectangle->x + x + 0.5, rectangle->y + 1.5);
				cairo_line_to (cr, rectangle->x + x + 0.5, rectangle->y + rectangle->height - 0.5);
			}
			for (y = GRID_STEP_1; y < rectangle->height; y += GRID_STEP_1) {
				if (y % GRID_STEP_2 == 0)
					continue;
				cairo_move_to (cr, rectangle->x + 1.5, rectangle->y + y + 0.5);
				cairo_line_to (cr, rectangle->x + rectangle->width - 0.5, rectangle->y + y + 0.5);
			}
		}
		cairo_stroke (cr);
	}

	cairo_restore (cr);
}


cairo_pattern_t *
_cairo_create_checked_pattern (int size)
{
	int              h_size;
	cairo_surface_t *surface;
	cairo_t         *ctx;
	cairo_pattern_t *pattern;

	h_size = size / 2;
	surface = cairo_image_surface_create (CAIRO_FORMAT_RGB24, size, size);
	ctx = cairo_create (surface);

	cairo_set_source_rgba (ctx, 0.4, 0.4, 0.4, 1.0);
	cairo_rectangle (ctx, 0, 0, h_size, h_size);
	cairo_fill (ctx);
	cairo_rectangle (ctx, h_size, h_size, h_size, h_size);
	cairo_fill (ctx);

	cairo_set_source_rgba (ctx, 0.5, 0.5, 0.5, 1.0);
	cairo_rectangle (ctx, h_size, 0, h_size, h_size);
	cairo_fill (ctx);
	cairo_rectangle (ctx, 0, h_size, h_size, h_size);
	cairo_fill (ctx);

	pattern = cairo_pattern_create_for_surface (surface);
	cairo_pattern_set_extend (pattern, CAIRO_EXTEND_REPEAT);

	cairo_surface_destroy (surface);
	cairo_destroy (ctx);

	return pattern;
}


void
_cairo_draw_thumbnail_frame (cairo_t *cr,
			     int      x,
			     int      y,
			     int      width,
			     int      height)
{
	cairo_save (cr);
	cairo_translate (cr, 0.5, 0.5);
	cairo_set_line_width (cr, 0.5);
	cairo_set_antialias (cr, CAIRO_ANTIALIAS_NONE);

	/* the drop shadow */

	cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 0.33);
	_cairo_draw_rounded_box (cr,
				 x + 2,
				 y + 2,
				 width - 1,
				 height - 1,
				 0);
	cairo_fill (cr);

	/* the outer frame */

	cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
	_cairo_draw_rounded_box (cr,
				 x,
				 y,
				 width - 1,
				 height - 1,
				 0);
	cairo_fill_preserve (cr);

	cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 0.55);
	cairo_stroke (cr);

	cairo_restore (cr);
}


void
_cairo_draw_film_background (cairo_t *cr,
			     int      x,
			     int      y,
			     int      width,
			     int      height)
{
	cairo_save (cr);

	/* the drop shadow */

	cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 0.33);
	cairo_rectangle (cr,
			 x + 2,
			 y + 2,
			 width,
			 height);
	cairo_fill (cr);

	/* dark background */

	cairo_set_source_rgb (cr, 0.1, 0.1, 0.1);
	cairo_rectangle (cr,
			 x,
			 y ,
			 width,
			 height);
	cairo_fill (cr);

	cairo_restore (cr);
}


static cairo_pattern_t *
_cairo_film_pattern_create (void)
{
	static cairo_pattern_t *film_pattern = NULL;
	cairo_pattern_t        *pattern;
	static GMutex           mutex;

	g_mutex_lock (&mutex);
	if (film_pattern == NULL) {
		char            *filename;
		cairo_surface_t *surface;

		filename = g_build_filename (GTHUMB_ICON_DIR, "filmholes.png", NULL);
		surface = cairo_image_surface_create_from_png (filename);
		film_pattern = cairo_pattern_create_for_surface (surface);
		cairo_pattern_set_filter (film_pattern, CAIRO_FILTER_GOOD);
		cairo_pattern_set_extend (film_pattern, CAIRO_EXTEND_REPEAT);

		cairo_surface_destroy (surface);
		g_free (filename);

	}
	pattern = cairo_pattern_reference (film_pattern);
	g_mutex_unlock (&mutex);

	return pattern;
}


void
_cairo_draw_film_foreground (cairo_t *cr,
			     int      x,
			     int      y,
			     int      width,
			     int      height,
			     int      thumbnail_size)
{
	cairo_pattern_t *pattern;
	double           film_scale;
	cairo_matrix_t   matrix;
	double           film_strip;

	/* left film strip */

	pattern = _cairo_film_pattern_create ();

	if (thumbnail_size > 128)
		film_scale = 256.0 / thumbnail_size;
	else
		film_scale = 128.0 / thumbnail_size;
	film_strip = 9.0 / film_scale;

	cairo_matrix_init_identity (&matrix);
	cairo_matrix_scale (&matrix, film_scale, film_scale);
	cairo_matrix_translate (&matrix, -x, 0);
	cairo_pattern_set_matrix (pattern, &matrix);
	cairo_set_source (cr, pattern);
	cairo_rectangle (cr,
			 x,
			 y,
			 film_strip,
			 height);
	cairo_fill (cr);

	/* right film strip */

	x = x + width - film_strip;
	cairo_matrix_init_identity (&matrix);
	cairo_matrix_scale (&matrix, film_scale, film_scale);
	cairo_matrix_translate (&matrix, -x, 0);
	cairo_pattern_set_matrix (pattern, &matrix);
	cairo_set_source (cr, pattern);
	cairo_rectangle (cr,
			 x,
			 y,
			 film_strip,
			 height);
	cairo_fill (cr);

	cairo_pattern_destroy (pattern);
}


#define DRAG_ICON_THUMBNAIL_OFFSET 4


cairo_surface_t *
_cairo_create_dnd_icon (cairo_surface_t *image,
			int              icon_size,
			ItemStyle        style,
			gboolean         multi_dnd)
{
	cairo_rectangle_int_t  thumbnail_rect;
	cairo_surface_t       *thumbnail;
	cairo_rectangle_int_t  icon_rect;
	int                    icon_padding;
	cairo_rectangle_int_t  frame_rect;
	cairo_surface_t       *icon;
	cairo_t               *cr;

	thumbnail_rect.width = cairo_image_surface_get_width (image);
	thumbnail_rect.height = cairo_image_surface_get_height (image);
	if (scale_keeping_ratio (&thumbnail_rect.width, &thumbnail_rect.height, icon_size, icon_size, FALSE))
		thumbnail = _cairo_image_surface_scale_fast (image, thumbnail_rect.width, thumbnail_rect.height);
	else
		thumbnail = cairo_surface_reference (image);

	switch (style) {
	case ITEM_STYLE_ICON:
		icon_padding = 8;
		icon_rect.width = icon_size + icon_padding;
		icon_rect.height = icon_size + icon_padding;
		thumbnail_rect.x = round ((double) (icon_rect.width - thumbnail_rect.width) / 2.0);
		thumbnail_rect.y = round ((double) (icon_rect.height - thumbnail_rect.height) / 2.0);
		frame_rect.x = 0;
		frame_rect.y = 0;
		frame_rect.width = icon_rect.width;
		frame_rect.height = icon_rect.height;
		break;

	case ITEM_STYLE_IMAGE:
		icon_padding = 8; /* padding for the frame border */
		icon_rect.width = thumbnail_rect.width + icon_padding;
		icon_rect.height = thumbnail_rect.height + icon_padding;
		thumbnail_rect.x = 3;
		thumbnail_rect.y = 3;
		frame_rect.x = 0;
		frame_rect.y = 0;
		frame_rect.width = thumbnail_rect.width + icon_padding - 2;
		frame_rect.height = thumbnail_rect.height + icon_padding - 2;
		break;

	case ITEM_STYLE_VIDEO:
		icon_padding = 4; /* padding for the drop shadow effect */
		icon_rect.width = thumbnail_rect.width + icon_padding;
		icon_rect.height = icon_size + icon_padding;
		thumbnail_rect.x = 0;
		thumbnail_rect.y = round ((double) (icon_size - thumbnail_rect.height) / 2.0);
		frame_rect.x = thumbnail_rect.x;
		frame_rect.y = 0;
		frame_rect.width = thumbnail_rect.width;
		frame_rect.height = icon_size;
		break;

	default:
		icon_rect.width = 0;
		icon_rect.height = 0;
		break;
	}

	if (multi_dnd) {
		icon_rect.width += DRAG_ICON_THUMBNAIL_OFFSET;
		icon_rect.height += DRAG_ICON_THUMBNAIL_OFFSET;
	}
	icon = _cairo_image_surface_create (CAIRO_FORMAT_ARGB32, icon_rect.width, icon_rect.height);
	cr = cairo_create (icon);

	switch (style) {
	case ITEM_STYLE_ICON:
		cairo_save (cr);
		cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 0.2);
		_cairo_draw_rounded_box (cr, frame_rect.x, frame_rect.y, frame_rect.width, frame_rect.height, 4);
		cairo_fill (cr);
		cairo_restore (cr);
		break;

	case ITEM_STYLE_IMAGE:
		if (multi_dnd)
			_cairo_draw_thumbnail_frame (cr, frame_rect.x + DRAG_ICON_THUMBNAIL_OFFSET, frame_rect.y + DRAG_ICON_THUMBNAIL_OFFSET, frame_rect.width, frame_rect.height);
		_cairo_draw_thumbnail_frame (cr, frame_rect.x, frame_rect.y, frame_rect.width, frame_rect.height);
		break;

	case ITEM_STYLE_VIDEO:
		_cairo_draw_film_background (cr, frame_rect.x, frame_rect.y, frame_rect.width, frame_rect.height);
		break;
	}

	cairo_save (cr);
	cairo_set_antialias (cr, CAIRO_ANTIALIAS_NONE);
	cairo_set_source_surface (cr, thumbnail, thumbnail_rect.x, thumbnail_rect.y);
	cairo_pattern_set_filter (cairo_get_source (cr), CAIRO_FILTER_FAST);
	cairo_rectangle (cr, thumbnail_rect.x, thumbnail_rect.y, thumbnail_rect.width, thumbnail_rect.height);
	cairo_fill (cr);
	cairo_restore (cr);

	if (style == ITEM_STYLE_VIDEO)
		_cairo_draw_film_foreground (cr, frame_rect.x, frame_rect.y, frame_rect.width, frame_rect.height, icon_size);

	cairo_surface_set_device_offset (icon, -icon_rect.width / 2, -icon_rect.height / 2);

	cairo_surface_destroy (thumbnail);
	cairo_destroy (cr);

	return icon;
}
