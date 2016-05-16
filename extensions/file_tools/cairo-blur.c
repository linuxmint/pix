/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2011 The Free Software Foundation, Inc.
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
#include <gthumb.h>
#include "cairo-blur.h"



static void
_cairo_image_surface_gaussian_blur (cairo_surface_t *source,
				    int              radius)
{
	/* FIXME: to do */
}


static void
box_blur (cairo_surface_t *source,
	  cairo_surface_t *destination,
	  int              radius,
	  guchar          *div_kernel_size)
{
	int     width, height, src_rowstride, dest_rowstride;
	guchar *p_src, *p_dest, *c1, *c2;
	int     x, y, i, i1, i2, width_minus_1, height_minus_1, radius_plus_1;
	int     r, g, b, a;
	guchar *p_dest_row, *p_dest_col;

	width = cairo_image_surface_get_width (source);
	height = cairo_image_surface_get_height (source);
	radius_plus_1 = radius + 1;

	/* horizontal blur */

	p_src = cairo_image_surface_get_data (source);
	p_dest = cairo_image_surface_get_data (destination);
	src_rowstride = cairo_image_surface_get_stride (source);
	dest_rowstride = cairo_image_surface_get_stride (destination);
	width_minus_1 = width - 1;
	for (y = 0; y < height; y++) {

		/* calculate the initial sums of the kernel */

		r = g = b = a = 0;

		for (i = -radius; i <= radius; i++) {
			c1 = p_src + (CLAMP (i, 0, width_minus_1) * 4);
			r += c1[CAIRO_RED];
			g += c1[CAIRO_GREEN];
			b += c1[CAIRO_BLUE];
			/*if (n_channels == 4)
				a += c1[CAIRO_ALPHA];*/
		}

		p_dest_row = p_dest;
		for (x = 0; x < width; x++) {
			/* set as the mean of the kernel */

			p_dest_row[CAIRO_RED] = div_kernel_size[r];
			p_dest_row[CAIRO_GREEN] = div_kernel_size[g];
			p_dest_row[CAIRO_BLUE] = div_kernel_size[b];
			p_dest_row[CAIRO_ALPHA] = 0xff;
			/*if (n_channels == 4)
				p_dest_row[CAIRO_ALPHA] = div_kernel_size[a];*/
			p_dest_row += 4;

			/* the pixel to add to the kernel */

			i1 = x + radius_plus_1;
			if (i1 > width_minus_1)
				i1 = width_minus_1;
			c1 = p_src + (i1 * 4);

			/* the pixel to remove from the kernel */

			i2 = x - radius;
			if (i2 < 0)
				i2 = 0;
			c2 = p_src + (i2 * 4);

			/* calculate the new sums of the kernel */

			r += c1[CAIRO_RED] - c2[CAIRO_RED];
			g += c1[CAIRO_GREEN] - c2[CAIRO_GREEN];
			b += c1[CAIRO_BLUE] - c2[CAIRO_BLUE];
			/*if (n_channels == 4)
				a += c1[CAIRO_ALPHA] - c2[CAIRO_ALPHA];*/
		}

		p_src += src_rowstride;
		p_dest += dest_rowstride;
	}

	/* vertical blur */

	p_src = cairo_image_surface_get_data (destination);
	p_dest = cairo_image_surface_get_data (source);
	src_rowstride = cairo_image_surface_get_stride (destination);
	dest_rowstride = cairo_image_surface_get_stride (source);
	height_minus_1 = height - 1;
	for (x = 0; x < width; x++) {

		/* calculate the initial sums of the kernel */

		r = g = b = a = 0;

		for (i = -radius; i <= radius; i++) {
			c1 = p_src + (CLAMP (i, 0, height_minus_1) * src_rowstride);
			r += c1[CAIRO_RED];
			g += c1[CAIRO_GREEN];
			b += c1[CAIRO_BLUE];
			/*if (n_channels == 4)
				a += c1[CAIRO_ALPHA];*/
		}

		p_dest_col = p_dest;
		for (y = 0; y < height; y++) {
			/* set as the mean of the kernel */

			p_dest_col[CAIRO_RED] = div_kernel_size[r];
			p_dest_col[CAIRO_GREEN] = div_kernel_size[g];
			p_dest_col[CAIRO_BLUE] = div_kernel_size[b];
			p_dest_col[CAIRO_ALPHA] = 0xff;
			/*if (n_channels == 4)
				p_dest_row[CAIRO_ALPHA] = div_kernel_size[a];*/
			p_dest_col += dest_rowstride;

			/* the pixel to add to the kernel */

			i1 = y + radius_plus_1;
			if (i1 > height_minus_1)
				i1 = height_minus_1;
			c1 = p_src + (i1 * src_rowstride);

			/* the pixel to remove from the kernel */

			i2 = y - radius;
			if (i2 < 0)
				i2 = 0;
			c2 = p_src + (i2 * src_rowstride);

			/* calculate the new sums of the kernel */

			r += c1[CAIRO_RED] - c2[CAIRO_RED];
			g += c1[CAIRO_GREEN] - c2[CAIRO_GREEN];
			b += c1[CAIRO_BLUE] - c2[CAIRO_BLUE];
			/*if (n_channels == 4)
				a += c1[CAIRO_ALPHA] - c2[CAIRO_ALPHA];*/
		}

		p_src += 4;
		p_dest += 4;
	}
}


static void
_cairo_image_surface_box_blur (cairo_surface_t *source,
			       int              radius,
			       int              iterations)
{
	gint64           kernel_size;
	guchar          *div_kernel_size;
	int              i;
	cairo_surface_t *tmp;

	kernel_size = 2 * radius + 1;

	/* optimization to avoid divisions: div_kernel_size[x] == x / kernel_size */
	div_kernel_size = g_new (guchar, 256 * kernel_size);
	for (i = 0; i < 256 * kernel_size; i++)
		div_kernel_size[i] = (guchar) (i / kernel_size);

	tmp = _cairo_image_surface_create_compatible (source);
	while (iterations-- > 0)
		box_blur (source, tmp, radius, div_kernel_size);

	cairo_surface_destroy (tmp);
}


void
_cairo_image_surface_blur (cairo_surface_t *source,
			   int              radius)
{
	if (radius <= 10)
		_cairo_image_surface_box_blur (source, radius, 3);
	else
		_cairo_image_surface_gaussian_blur (source, radius);
}


void
_cairo_image_surface_sharpen (cairo_surface_t *source,
			      int              radius,
			      double           amount,
			      guchar           threshold)
{
	cairo_surface_t *blurred;
	int              width, height;
	int              source_rowstride, blurred_rowstride;
	int              x, y;
	guchar          *p_src, *p_blurred;
	guchar          *p_src_row, *p_blurred_row;
	guchar           r1, g1, b1;
	guchar           r2, g2, b2;
	int              tmp;

	blurred = _cairo_image_surface_copy (source);
	_cairo_image_surface_blur (blurred, radius);

	width = cairo_image_surface_get_width (source);
	height = cairo_image_surface_get_height (source);
	source_rowstride = cairo_image_surface_get_stride (source);
	blurred_rowstride = cairo_image_surface_get_stride (blurred);

	p_src = cairo_image_surface_get_data (source);
	p_blurred = cairo_image_surface_get_data (blurred);

#define ASSIGN_INTERPOLATED_VALUE(x1, x2)			\
	if (ABS (x1 - x2) >= threshold) {			\
		tmp = interpolate_value (x1, x2, amount);	\
		x1 = CLAMP (tmp, 0, 255);			\
	}

	for (y = 0; y < height; y++) {
		p_src_row = p_src;
		p_blurred_row = p_blurred;

		for (x = 0; x < width; x++) {
			r1 = p_src_row[CAIRO_RED];
			g1 = p_src_row[CAIRO_GREEN];
			b1 = p_src_row[CAIRO_BLUE];

			r2 = p_blurred_row[CAIRO_RED];
			g2 = p_blurred_row[CAIRO_GREEN];
			b2 = p_blurred_row[CAIRO_BLUE];

			ASSIGN_INTERPOLATED_VALUE (r1, r2)
			ASSIGN_INTERPOLATED_VALUE (g1, g2)
			ASSIGN_INTERPOLATED_VALUE (b1, b2)

			p_src_row[CAIRO_RED] = r1;
			p_src_row[CAIRO_GREEN] = g1;
			p_src_row[CAIRO_BLUE] = b1;

			p_src_row += 4;
			p_blurred_row += 4;
		}

		p_src += source_rowstride;
		p_blurred += blurred_rowstride;
	}

#undef ASSIGN_INTERPOLATED_VALUE

	cairo_surface_destroy (blurred);
}
