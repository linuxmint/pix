/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2012 The Free Software Foundation, Inc.
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
#include <string.h>
#include <math.h>
#include <cairo.h>
#include "cairo-utils.h"
#include "cairo-scale.h"
#include "gfixed.h"


#if 1


/* -- _cairo_image_surface_scale_nearest -- */


cairo_surface_t *
_cairo_image_surface_scale_nearest (cairo_surface_t *image,
				    int              new_width,
				    int              new_height)
{
	cairo_surface_t *scaled;
	int              src_width;
	int              src_height;
	guchar          *p_src;
	guchar          *p_dest;
	int              src_rowstride;
	int              dest_rowstride;
	gfixed           step_x, step_y;
	guchar          *p_src_row;
	guchar          *p_src_col;
	guchar          *p_dest_row;
	guchar          *p_dest_col;
	gfixed           max_row, max_col;
	gfixed           x_src, y_src;
	int              x, y;

	g_return_val_if_fail (cairo_image_surface_get_format (image) == CAIRO_FORMAT_ARGB32, NULL);

	scaled = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
					     new_width,
					     new_height);

	src_width = cairo_image_surface_get_width  (image);
	src_height = cairo_image_surface_get_height (image);
	p_src = cairo_image_surface_get_data (image);
	p_dest = cairo_image_surface_get_data (scaled);
	src_rowstride = cairo_image_surface_get_stride (image);
	dest_rowstride = cairo_image_surface_get_stride (scaled);

	cairo_surface_flush (scaled);

	step_x = GDOUBLE_TO_FIXED ((double) src_width / new_width);
	step_y = GDOUBLE_TO_FIXED ((double) src_height / new_height);

	p_dest_row = p_dest;
	p_src_row = p_src;
	max_row = GINT_TO_FIXED (src_height - 1);
	max_col= GINT_TO_FIXED (src_width - 1);
	/* pick the pixel in the middle to avoid the shift effect. */
	y_src = gfixed_div (step_y, GFIXED_2);
	for (y = 0; y < new_height; y++) {
		p_dest_col = p_dest_row;
		p_src_col = p_src_row;

		x_src = gfixed_div (step_x, GFIXED_2);
		for (x = 0; x < new_width; x++) {
			p_src_col = p_src_row + (GFIXED_TO_INT (MIN (x_src, max_col)) << 2); /* p_src_col = p_src_row + x_src * 4 */
			memcpy (p_dest_col, p_src_col, 4);

			p_dest_col += 4;
			x_src += step_x;
		}

		p_dest_row += dest_rowstride;
		y_src += step_y;
		p_src_row = p_src + (GFIXED_TO_INT (MIN (y_src, max_row)) * src_rowstride);
	}

	cairo_surface_mark_dirty (scaled);

	return scaled;
}


#endif


/* -- _cairo_image_surface_scale_filter --
 *
 * Based on code from ImageMagick/magick/resize.c
 *
 * */


#define EPSILON ((double) 1.0e-8)


typedef double (*weight_func_t) (double distance);


static double
box (double x)
{
	return 1.0;
}


static double
triangle (double x)
{
	return (x < 1.0) ? 1.0 - x : 0.0;
}


static double
sinc_fast (double x)
{
	if (x > 4.0) {
		const double alpha = G_PI * x;
		return sin (alpha) / alpha;
	}

	{
		/*
		 * The approximations only depend on x^2 (sinc is an even function).
		 */

		const double xx = x*x;

		/*
		 * Maximum absolute relative error 6.3e-6 < 1/2^17.
		 */
		const double c0 = 0.173610016489197553621906385078711564924e-2L;
		const double c1 = -0.384186115075660162081071290162149315834e-3L;
		const double c2 = 0.393684603287860108352720146121813443561e-4L;
		const double c3 = -0.248947210682259168029030370205389323899e-5L;
		const double c4 = 0.107791837839662283066379987646635416692e-6L;
		const double c5 = -0.324874073895735800961260474028013982211e-8L;
		const double c6 = 0.628155216606695311524920882748052490116e-10L;
		const double c7 = -0.586110644039348333520104379959307242711e-12L;
		const double p = c0+xx*(c1+xx*(c2+xx*(c3+xx*(c4+xx*(c5+xx*(c6+xx*c7))))));

		return (xx-1.0)*(xx-4.0)*(xx-9.0)*(xx-16.0)*p;
	}
}


static struct {
	weight_func_t weight_func;
	double        support;
}
const filters[N_SCALE_FILTERS] = {
	{ box,		 .0 },
	{ box,		 .5 },
	{ triangle,	1.0 },
	{ sinc_fast,	3.0 }
};


/* -- resize_filter_t --  */


typedef struct {
	weight_func_t  weight_func;
	double         support;
	GthAsyncTask  *task;
	gulong         total_lines;
	gulong         processed_lines;
	gboolean       cancelled;
} resize_filter_t;


static resize_filter_t *
resize_filter_create (scale_filter_t  filter_type,
		      GthAsyncTask   *task)
{
	resize_filter_t *resize_filter;

	resize_filter = g_slice_new (resize_filter_t);
	resize_filter->weight_func = filters[filter_type].weight_func;
	resize_filter->support = filters[filter_type].support;
	resize_filter->task = task;
	resize_filter->total_lines = 0;
	resize_filter->processed_lines = 0;
	resize_filter->cancelled = FALSE;

	return resize_filter;
}


static double inline
resize_filter_get_support (resize_filter_t *resize_filter)
{
	return resize_filter->support;
}


static double inline
resize_filter_get_weight (resize_filter_t *resize_filter,
			  double           distance)
{
	double scale = 1.0;

	if (resize_filter->weight_func == sinc_fast)
		scale = resize_filter->weight_func (fabs (distance));

	return scale * resize_filter->weight_func (fabs (distance));
}


static void
resize_filter_destroy (resize_filter_t *resize_filter)
{
	g_slice_free (resize_filter_t, resize_filter);
}


static inline double
reciprocal (double x)
{
	double sign = x < 0.0 ? -1.0 : 1.0;
	return (sign * x) >= EPSILON ? 1.0 / x : sign * (1.0 / EPSILON);
}


#define CLAMP_PIXEL(v) (((v) <= 0) ? 0  : ((v) >= 255) ? 255 : (v));


static void
horizontal_scale_transpose (cairo_surface_t *image,
			    cairo_surface_t *scaled,
			    double           scale_factor,
			    resize_filter_t *resize_filter)
{
	double    scale;
	double    support;
	int       y;
	guchar   *p_src;
        guchar   *p_dest;
        int       src_rowstride;
        int       dest_rowstride;
        double   *weights;
        gfixed   *fixed_weights;

	if (resize_filter->cancelled)
		return;

        cairo_surface_flush (scaled);

	scale = MAX (1.0 / scale_factor + EPSILON, 1.0);
	support = scale * resize_filter_get_support (resize_filter);
	if (support < 0.5) {
		support = 0.5;
		scale = 1.0;
	}

	p_src = cairo_image_surface_get_data (image);
	p_dest = cairo_image_surface_get_data (scaled);
	src_rowstride = cairo_image_surface_get_stride (image);
	dest_rowstride = cairo_image_surface_get_stride (scaled);
	weights = g_new (double, 2.0 * support + 3.0);
	fixed_weights = g_new (gfixed, 2.0 * support + 3.0);

	scale = reciprocal (scale);
	for (y = 0; ! resize_filter->cancelled && (y < cairo_image_surface_get_height (scaled)); y++) {
	        guchar *p_src_row;
	        guchar *p_dest_pixel;
		double  bisect;
		int     start;
		int     stop;
		double  density;
		int     n;
		int     x;
		int     i;

		if (resize_filter->task != NULL) {
			double progress = (double) resize_filter->processed_lines++ / resize_filter->total_lines;
			gth_async_task_set_data (resize_filter->task, NULL, NULL, &progress);
		}

		bisect = (y + 0.5) / scale_factor + EPSILON;
		start = MAX (bisect - support + 0.5, 0.0);
		stop = MIN (bisect + support + 0.5, cairo_image_surface_get_width (image));

		density = 0.0;
		for (n = 0; n < stop - start; n++) {
			weights[n] = resize_filter_get_weight (resize_filter, scale * ((double) (start + n) - bisect + 0.5));
			density += weights[n];
		}

		density = reciprocal (density);
		for (i = 0; i < n; i++) {
			double w = weights[i] * density;
			fixed_weights[i] = GDOUBLE_TO_FIXED (w);
		}

		p_src_row = p_src;
		p_dest_pixel = p_dest + (y * dest_rowstride);
		for (x = 0; x < cairo_image_surface_get_width (scaled); x++) {
			guchar *p_src_pixel;
			gfixed  r, g, b, a;
			gfixed  w;

			if (resize_filter->task != NULL) {
				gth_async_task_get_data (resize_filter->task, NULL, &resize_filter->cancelled, NULL);
				if (resize_filter->cancelled)
					break;
			}

			p_src_pixel = p_src_row + (start * 4);
			r = g = b = a = GFIXED_0;

			for (i = 0; i < n; i++) {
				w = fixed_weights[i];
				r += gfixed_mul (w, GINT_TO_FIXED (p_src_pixel[CAIRO_RED]));
				g += gfixed_mul (w, GINT_TO_FIXED (p_src_pixel[CAIRO_GREEN]));
				b += gfixed_mul (w, GINT_TO_FIXED (p_src_pixel[CAIRO_BLUE]));
				a += gfixed_mul (w, GINT_TO_FIXED (p_src_pixel[CAIRO_ALPHA]));

				p_src_pixel += 4;
			}

			r = GFIXED_ROUND_TO_INT (r);
			g = GFIXED_ROUND_TO_INT (g);
			b = GFIXED_ROUND_TO_INT (b);
			a = GFIXED_ROUND_TO_INT (a);

			p_dest_pixel[CAIRO_RED] = CLAMP_PIXEL (r);
			p_dest_pixel[CAIRO_GREEN] = CLAMP_PIXEL (g);
			p_dest_pixel[CAIRO_BLUE] = CLAMP_PIXEL (b);
			p_dest_pixel[CAIRO_ALPHA] = CLAMP_PIXEL (a);

			p_dest_pixel += 4;
			p_src_row += src_rowstride;
		}
	}

	cairo_surface_mark_dirty (scaled);

	g_free (weights);
	g_free (fixed_weights);
}


static cairo_surface_t *
_cairo_image_surface_scale_filter (cairo_surface_t *image,
				   int              new_width,
				   int              new_height,
				   scale_filter_t   filter,
				   GthAsyncTask    *task)
{
	int              src_width;
	int              src_height;
	cairo_surface_t *scaled;
	resize_filter_t *resize_filter;
	double           x_factor;
	double           y_factor;
	cairo_surface_t *tmp;

	src_width = cairo_image_surface_get_width  (image);
	src_height = cairo_image_surface_get_height (image);

	if ((src_width == new_width) && (src_height == new_height))
		return _cairo_image_surface_copy (image);

	scaled = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
					     new_width,
					     new_height);
	if (scaled == NULL)
		return NULL;

	resize_filter = resize_filter_create (filter, task);
	resize_filter->total_lines = new_width + new_height;
	resize_filter->processed_lines = 0;

	x_factor = (double) new_width / src_width;
	y_factor = (double) new_height / src_height;
	tmp = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
					  src_height,
					  new_width);
	horizontal_scale_transpose (image, tmp, x_factor, resize_filter);
	horizontal_scale_transpose (tmp, scaled, y_factor, resize_filter);

	resize_filter_destroy (resize_filter);
	cairo_surface_destroy (tmp);

	return scaled;
}


cairo_surface_t *
_cairo_image_surface_scale (cairo_surface_t  *image,
			    int               scaled_width,
			    int               scaled_height,
			    scale_filter_t    filter,
			    GthAsyncTask     *task)
{
	return _cairo_image_surface_scale_filter (image,
						  scaled_width,
						  scaled_height,
						  filter,
						  task);
}


cairo_surface_t *
_cairo_image_surface_scale_squared (cairo_surface_t *image,
				    int              size,
				    scale_filter_t   quality,
				    GthAsyncTask    *task)
{
	int              width, height;
	int              scaled_width, scaled_height;
	cairo_surface_t *scaled;
	cairo_surface_t *squared;

	width = cairo_image_surface_get_width (image);
	height = cairo_image_surface_get_height (image);

	if ((width < size) && (height < size))
		return _cairo_image_surface_copy (image);

	if (width > height) {
		scaled_height = size;
		scaled_width = (int) (((double) width / height) * scaled_height);
	}
	else {
		scaled_width = size;
		scaled_height = (int) (((double) height / width) * scaled_width);
	}

	if ((scaled_width != width) || (scaled_height != height))
		scaled = _cairo_image_surface_scale (image, scaled_width, scaled_height, quality, task);
	else
		scaled = cairo_surface_reference (image);

	if ((scaled_width == size) && (scaled_height == size))
		return scaled;

	squared = _cairo_image_surface_copy_subsurface (scaled,
							(scaled_width - size) / 2,
							(scaled_height - size) / 2,
							size,
							size);
	cairo_surface_destroy (scaled);

	return squared;
}


/* -- _cairo_image_surface_scale_bilinear -- */


#if 0


#define BILINEAR_INTERPOLATE(v, v00, v01, v10, v11, fx, fy) \
	tmp = (1.0 - (fy)) * \
	      ((1.0 - (fx)) * (v00) + (fx) * (v01)) \
              + \
              (fy) * \
              ((1.0 - (fx)) * (v10) + (fx) * (v11)); \
	v = CLAMP (tmp, 0, 255);


cairo_surface_t *
_cairo_image_surface_scale_bilinear_2x2 (cairo_surface_t *image,
					 int              new_width,
					 int              new_height)
{
	cairo_surface_t *scaled;
	int              src_width;
	int              src_height;
	guchar          *p_src;
	guchar          *p_dest;
	int              src_rowstride;
	int              dest_rowstride;
	double           step_x, step_y;
	guchar          *p_dest_row;
	guchar          *p_src_row;
	guchar          *p_src_col;
	guchar          *p_dest_col;
	double           x_src, y_src;
	int              x, y;
	guchar           r00, r01, r10, r11;
	guchar           g00, g01, g10, g11;
	guchar           b00, b01, b10, b11;
	guchar           a00, a01, a10, a11;
	guchar           r, g, b, a;
	guint32          pixel;
	double           tmp;
	double           x_fract, y_fract;
	int              col, row;

	scaled = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
					     new_width,
					     new_height);

	src_width = cairo_image_surface_get_width  (image);
	src_height = cairo_image_surface_get_height (image);
	p_src = cairo_image_surface_get_data (image);
	p_dest = cairo_image_surface_get_data (scaled);
	src_rowstride = cairo_image_surface_get_stride (image);
	dest_rowstride = cairo_image_surface_get_stride (scaled);

	cairo_surface_flush (scaled);

	step_x = (double) src_width / new_width;
	step_y = (double) src_height / new_height;
	p_dest_row = p_dest;
	p_src_row = p_src;
	y_src = 0;

	for (y = 0; y < new_height; y++) {
		row = floor (y_src);
		y_fract = y_src - row;
		p_src_row = p_src + (row * src_rowstride);

		p_dest_col = p_dest_row;
		p_src_col = p_src_row;
		x_src = 0;
		for (x = 0; x < new_width; x++) {
			col = floor (x_src);
			x_fract = x_src - col;
			p_src_col = p_src_row + (col * 4);

			/* x00 */

			CAIRO_COPY_RGBA (p_src_col, r00, g00, b00, a00);

			/* x01 */

			if (col + 1 < src_width - 1) {
				p_src_col += 4;
				CAIRO_COPY_RGBA (p_src_col, r01, g01, b01, a01);
			}
			else {
				r01 = r00;
				g01 = g00;
				b01 = b00;
				a01 = a00;
			}

			/* x10 */

			if (row + 1 < src_height - 1) {
				p_src_col = p_src_row + src_rowstride + (col * 4);
				CAIRO_COPY_RGBA (p_src_col, r10, g10, b10, a10);
			}
			else {
				r10 = r00;
				g10 = g00;
				b10 = b00;
				a10 = a00;
			}

			/* x11 */

			if ((row + 1 < src_height - 1) && (col + 1 < src_width - 1)) {
				p_src_col += 4;
				CAIRO_COPY_RGBA (p_src_col, r11, g11, b11, a11);
			}
			else {
				r11 = r00;
				g11 = g00;
				b11 = b00;
				a11 = a00;
			}

			BILINEAR_INTERPOLATE (r, r00, r01, r10, r11, x_fract, y_fract);
			BILINEAR_INTERPOLATE (g, g00, g01, g10, g11, x_fract, y_fract);
			BILINEAR_INTERPOLATE (b, b00, b01, b10, b11, x_fract, y_fract);
			BILINEAR_INTERPOLATE (a, a00, a01, a10, a11, x_fract, y_fract);
			pixel = CAIRO_RGBA_TO_UINT32 (r, g, b, a);
			memcpy (p_dest_col, &pixel, 4);

			p_dest_col += 4;
			x_src += step_x;
		}

		p_dest_row += dest_rowstride;
		y_src += step_y;
	}

	cairo_surface_mark_dirty (scaled);

	return scaled;
}


static void
_cairo_surface_reduce_row (guchar *dest_data,
			   guchar *src_row0,
			   guchar *src_row1,
			   guchar *src_row2,
			   int     src_width)
{
	int     x, b;
	double  sum;
	int     col0, col1, col2;
	guchar  c[4];
	guint32 pixel;

	/*  Pre calculated gausian matrix
	 *  Standard deviation = 0.71

		12 32 12
		32 86 32
		12 32 12

		Matrix sum is = 262
		Normalize by dividing with 273

	*/

	for (x = 0; x < src_width - (src_width % 2); x += 2) {
		for (b = 0; b < 4; b++) {

			col0 = MAX (x - 1, 0) * 4 + b;
			col1 = x * 4 + b;
			col2 = MIN (x + 1, src_width - 1) * 4 + b;

			sum = 0.0;

			sum += src_row0[col0] * 12;
			sum += src_row0[col1] * 32;
			sum += src_row0[col2] * 12;

			sum += src_row1[col0] * 32;
			sum += src_row1[col1] * 86;
			sum += src_row1[col2] * 32;

			sum += src_row2[col0] * 12;
			sum += src_row2[col1] * 32;
			sum += src_row2[col2] * 12;

			sum /= 262.0;

			c[b] = CLAMP (sum, 0, 255);
		}

		pixel = CAIRO_RGBA_TO_UINT32 (c[CAIRO_RED], c[CAIRO_GREEN], c[CAIRO_BLUE], c[CAIRO_ALPHA]);
		memcpy (dest_data, &pixel, 4);

		dest_data += 4;
	}
}


static cairo_surface_t *
_cairo_surface_reduce_by_half (cairo_surface_t *src)
{
	int              src_width, src_height;
	cairo_surface_t *dest;
	int              src_rowstride, dest_rowstride;
	guchar          *row0, *row1, *row2;
	guchar          *src_data, *dest_data;
	int              y;

	src_width = cairo_image_surface_get_width (src);
	src_height = cairo_image_surface_get_height (src);
	dest = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
					   src_width / 2,
					   src_height / 2);

	cairo_surface_flush (dest);

	dest_rowstride = cairo_image_surface_get_stride (dest);
	dest_data = cairo_image_surface_get_data (dest);

	src_rowstride = cairo_image_surface_get_stride (src);
	src_data = cairo_image_surface_get_data (src);

	for (y = 0; y < src_height - (src_height % 2); y += 2) {
		row0 = src_data + (MAX (y - 1, 0) * src_rowstride);
		row1 = src_data + (y * src_rowstride);
		row2 = src_data + (MIN (y + 1, src_height - 1) * src_rowstride);

		_cairo_surface_reduce_row (dest_data,
					   row0,
					   row1,
					   row2,
					   src_width);

		dest_data += dest_rowstride;
	}

	cairo_surface_mark_dirty (dest);

	return dest;
}


#endif


cairo_surface_t *
_cairo_image_surface_scale_bilinear_2x2 (cairo_surface_t *image,
				         int              new_width,
				         int              new_height)
{
	cairo_surface_t *output;
	cairo_t         *cr;

	output = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
					     new_width,
					     new_height);
	cr = cairo_create (output);
	cairo_scale (cr,
		     (double) new_width / cairo_image_surface_get_width (image),
		     (double) new_height / cairo_image_surface_get_height (image));
	cairo_set_source_surface (cr, image, 0.0, 0.0);
	cairo_pattern_set_filter (cairo_get_source (cr), CAIRO_FILTER_BILINEAR);
	cairo_rectangle (cr, 0.0, 0.0, cairo_image_surface_get_width (image), cairo_image_surface_get_height (image));
	cairo_fill (cr);
	cairo_surface_flush (output);

	cairo_destroy (cr);

	return output;
}


cairo_surface_t *
_cairo_image_surface_scale_bilinear (cairo_surface_t *image,
				     int              new_width,
				     int              new_height)
{
	double           scale, last_scale, s;
	int              iterations = 0;
	cairo_surface_t *tmp;
	cairo_surface_t *tmp2;
	double           w, h;

	scale = (double) new_width / cairo_image_surface_get_width (image);
	last_scale = 1.0;
	s = scale;
	while (s < 1.0 / _CAIRO_MAX_SCALE_FACTOR) {
		s *= _CAIRO_MAX_SCALE_FACTOR;
		last_scale /= _CAIRO_MAX_SCALE_FACTOR;
		iterations++;
	}
	last_scale = last_scale / scale;

	tmp = cairo_surface_reference (image);
	w = cairo_image_surface_get_width (tmp);
	h = cairo_image_surface_get_height (tmp);

	while (iterations-- > 0) {
		w /= _CAIRO_MAX_SCALE_FACTOR;
		h /= _CAIRO_MAX_SCALE_FACTOR;
		tmp2 = _cairo_image_surface_scale_bilinear_2x2 (tmp, round (w), round (h));
		cairo_surface_destroy (tmp);
		tmp = tmp2;
	}

	w /= last_scale;
	h /= last_scale;
	tmp2 = _cairo_image_surface_scale_bilinear_2x2 (tmp, round (w), round (h));
	cairo_surface_destroy (tmp);

	return tmp2;
}
