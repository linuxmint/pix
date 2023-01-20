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

#include <math.h>
#include <gthumb.h>
#include "cairo-rotate.h"


void
_cairo_image_surface_rotate_get_cropping_parameters (cairo_surface_t *image,
						     double           angle,
						     double          *p1_plus_p2,
						     double          *p_min)
{
	double angle_rad;
	double cos_angle, sin_angle;
	double src_width, src_height;
	double t1, t2;

	if (angle < -90)
		angle += 180;
	else if (angle > 90)
		angle -= 180;

	angle_rad = fabs (angle) / 180.0 * G_PI;

	cos_angle = cos (angle_rad);
	sin_angle = sin (angle_rad);

	src_width  = cairo_image_surface_get_width  (image) - 1.0;
	src_height = cairo_image_surface_get_height (image) - 1.0;

	if (src_width > src_height) {
		t1 = cos_angle * src_width - sin_angle * src_height;
		t2 = sin_angle * src_width + cos_angle * src_height;

		*p1_plus_p2  = 1.0 + (t1 * src_height) / (t2 * src_width);

		*p_min = src_height / src_width * sin_angle * cos_angle + (*p1_plus_p2 - 1) * cos_angle * cos_angle;
	}
	else {
		t1 = cos_angle * src_height - sin_angle * src_width;
		t2 = sin_angle * src_height + cos_angle * src_width;

		*p1_plus_p2 = 1.0 + (t1 * src_width)  / (t2 * src_height);

		*p_min = src_width / src_height * sin_angle * cos_angle + (*p1_plus_p2 - 1) * cos_angle * cos_angle;
	}
}


void
_cairo_image_surface_rotate_get_cropping_region (cairo_surface_t       *image,
						 double                 angle,
						 double                 p1,
						 double                 p2,
						 cairo_rectangle_int_t *region)
{
	double angle_rad;
	double cos_angle, sin_angle;
	double src_width, src_height;
	double new_width;
	double xx1, yy1, xx2, yy2;

	if (angle < -90)
		angle += 180;
	else if (angle > 90)
		angle -= 180;

	p1    = CLAMP (p1,      0.0,  1.0);
	p2    = CLAMP (p2,      0.0,  1.0);

	angle_rad = fabs (angle) / 180.0 * G_PI;
	cos_angle = cos (angle_rad);
	sin_angle = sin (angle_rad);

	src_width  = cairo_image_surface_get_width  (image) - 1.0;
	src_height = cairo_image_surface_get_height (image) - 1.0;

	if (angle < 0) {

		/* This is to make the p1/p2 sliders behavour more user friendly */

		double t;

		t = p1;
		p1 = p2;
		p2 = t;
	}

	if (src_width > src_height) {
		xx1 = p1 * src_width * cos_angle + src_height * sin_angle;
		yy1 = p1 * src_width * sin_angle;
		xx2 = (1 - p2) * src_width * cos_angle;
		yy2 = (1 - p2) * src_width * sin_angle + src_height * cos_angle;
	}
	else {
		xx1 = p1       * src_height * sin_angle;
		yy1 = (1 - p1) * src_height * cos_angle;
		xx2 = (1 - p2) * src_height * sin_angle + src_width * cos_angle;
		yy2 = p2       * src_height * cos_angle + src_width * sin_angle;
	}

	if (angle < 0) {
		new_width = (cos_angle * src_width) + (sin_angle * src_height);
		xx1 = new_width - xx1;
		xx2 = new_width - xx2;
	}

	region->x = GDOUBLE_ROUND_TO_INT (MIN (xx1, xx2));
	region->y = GDOUBLE_ROUND_TO_INT (MIN (yy1, yy2));

	region->width  = GDOUBLE_ROUND_TO_INT (MAX (xx1, xx2)) - region->x + 1;
	region->height = GDOUBLE_ROUND_TO_INT (MAX (yy1, yy2)) - region->y + 1;
}


double
_cairo_image_surface_rotate_get_align_angle (gboolean  vertical,
					     GdkPoint *p1,
					     GdkPoint *p2)
{
	double angle;

	if (! vertical) {
		if (p1->y == p2->y)
			return 0.0;

		if (p2->x > p1->x)
			angle = -atan2 (p2->y - p1->y, p2->x - p1->x);
		else
			angle = -atan2 (p1->y - p2->y, p1->x - p2->x);
	}
	else {
		if (p1->x == p2->x)
			return 0.0;

		if (p2->y > p1->y)
			angle = atan2 (p2->x - p1->x, p2->y - p1->y);
		else
			angle = atan2 (p1->x - p2->x, p1->y - p2->y);
	}

	angle = angle * 180.0 / G_PI;
	angle = GDOUBLE_ROUND_TO_INT (angle * 10.0) / 10.0;

	return angle;
}


static cairo_surface_t*
rotate (cairo_surface_t *image,
	double           angle,
	gboolean         high_quality,
	guchar           r0,
	guchar           g0,
	guchar           b0,
	guchar           a0,
	GthAsyncTask    *task)
{
	cairo_surface_t *image_with_background;
	cairo_surface_t *rotated;
	double           angle_rad;
	double           cos_angle, sin_angle;
	double           src_width, src_height;
	int              new_width, new_height;
	int              src_rowstride, new_rowstride;
	int              xi, yi;
	double           x, y;
	double           x2, y2;
	int              x2min, y2min;
	int              x2max, y2max;
	double           fx, fy;
	guchar          *p_src, *p_new;
	guchar          *p_src2, *p_new2;
	guchar           r00, r01, r10, r11;
	guchar           g00, g01, g10, g11;
	guchar           b00, b01, b10, b11;
	guchar           a00, a01, a10, a11;
	double           half_new_width;
	double           half_new_height;
	double           half_src_width;
	double           half_src_height;
	int              tmp;
	guchar           r, g, b, a;
	guint32          pixel;

	angle = CLAMP (angle, -90.0, 90.0);
	angle_rad = angle / 180.0 * G_PI;
	cos_angle = cos (angle_rad);
	sin_angle = sin (angle_rad);
	src_width  = cairo_image_surface_get_width  (image);
	src_height = cairo_image_surface_get_height (image);
	new_width  = GDOUBLE_ROUND_TO_INT (      cos_angle  * src_width + fabs(sin_angle) * src_height);
	new_height = GDOUBLE_ROUND_TO_INT (fabs (sin_angle) * src_width +      cos_angle  * src_height);

	if (a0 == 0xff) {
		/* pre-multiply the background color */

		image_with_background = _cairo_image_surface_copy (image);
		p_src = _cairo_image_surface_flush_and_get_data (image);
		p_new = _cairo_image_surface_flush_and_get_data (image_with_background);
		src_rowstride = cairo_image_surface_get_stride (image);
		new_rowstride = cairo_image_surface_get_stride (image_with_background);

		cairo_surface_flush (image_with_background);
		for (yi = 0; yi < src_height; yi++) {
			p_src2 = p_src;
			p_new2 = p_new;
			for (xi = 0; xi < src_width; xi++) {
				a = p_src2[CAIRO_ALPHA];
				r = p_src2[CAIRO_RED] + _cairo_multiply_alpha (r0, 0xff - a);
				g = p_src2[CAIRO_GREEN] + _cairo_multiply_alpha (g0, 0xff - a);
				b = p_src2[CAIRO_BLUE] + _cairo_multiply_alpha (b0, 0xff - a);
				pixel = CAIRO_RGBA_TO_UINT32 (r, g, b, 0xff);
				memcpy (p_new2, &pixel, sizeof (guint32));

				p_new2 += 4;
				p_src2 += 4;
			}
			p_src += src_rowstride;
			p_new += new_rowstride;
		}
		cairo_surface_mark_dirty (image_with_background);
	}
	else
		image_with_background = cairo_surface_reference (image);

	/* create the rotated image */

	rotated = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, new_width, new_height);

	p_src = _cairo_image_surface_flush_and_get_data (image_with_background);
	p_new = _cairo_image_surface_flush_and_get_data (rotated);
	src_rowstride = cairo_image_surface_get_stride (image_with_background);
	new_rowstride = cairo_image_surface_get_stride (rotated);

/*
 * bilinear interpolation
 *            fx
 *    v00------------v01
 *    |        |      |
 * fy |--------v      |
 *    |               |
 *    |               |
 *    |               |
 *    v10------------v11
 */
#define INTERPOLATE(v, v00, v01, v10, v11, fx, fy) \
	tmp = (1.0 - (fy)) * \
	      ((1.0 - (fx)) * (v00) + (fx) * (v01)) \
              + \
              (fy) * \
              ((1.0 - (fx)) * (v10) + (fx) * (v11)); \
	v = CLAMP (tmp, 0, 255);

#define GET_VALUES(r, g, b, a, x, y) \
	if (x >= 0 && x < src_width && y >= 0 && y < src_height) { \
		p_src2 = p_src + src_rowstride * y + 4 * x; \
		r = p_src2[CAIRO_RED]; \
		g = p_src2[CAIRO_GREEN]; \
		b = p_src2[CAIRO_BLUE]; \
		a = p_src2[CAIRO_ALPHA]; \
	} \
	else { \
		r = r0; \
		g = g0; \
		b = b0; \
		a = a0; \
	}

	half_new_width = new_width / 2.0;
	half_new_height = new_height / 2.0;
	half_src_width = src_width / 2.0;
	half_src_height = src_height / 2.0;

	cairo_surface_flush (rotated);

	y = - half_new_height;
	for (yi = 0; yi < new_height; yi++) {
		if (task != NULL) {
			gboolean cancelled;
			double   progress;

			gth_async_task_get_data (task, NULL, &cancelled, NULL);
			if (cancelled)
				goto out;

			progress = (double) yi / new_height;
			gth_async_task_set_data (task, NULL, NULL, &progress);
		}

		p_new2 = p_new;

		x = - half_new_width;
		for (xi = 0; xi < new_width; xi++) {
			x2 = cos_angle * x - sin_angle * y + half_src_width;
			y2 = sin_angle * x + cos_angle * y + half_src_height;

			if (high_quality) {
				/* Bilinear interpolation. */

				x2min = (int) x2;
				y2min = (int) y2;
				x2max = x2min + 1;
				y2max = y2min + 1;

				GET_VALUES (r00, g00, b00, a00, x2min, y2min);
				GET_VALUES (r01, g01, b01, a01, x2max, y2min);
				GET_VALUES (r10, g10, b10, a10, x2min, y2max);
				GET_VALUES (r11, g11, b11, a11, x2max, y2max);

				fx = x2 - x2min;
				fy = y2 - y2min;

				INTERPOLATE (r, r00, r01, r10, r11, fx, fy);
				INTERPOLATE (g, g00, g01, g10, g11, fx, fy);
				INTERPOLATE (b, b00, b01, b10, b11, fx, fy);
				INTERPOLATE (a, a00, a01, a10, a11, fx, fy);

				pixel = CAIRO_RGBA_TO_UINT32 (r, g, b, a);
				memcpy (p_new2, &pixel, sizeof (guint32));
			}
			else {
				/* Nearest neighbor */

				x2min = GDOUBLE_ROUND_TO_INT (x2);
				y2min = GDOUBLE_ROUND_TO_INT (y2);

				GET_VALUES (p_new2[CAIRO_RED],
					    p_new2[CAIRO_GREEN],
					    p_new2[CAIRO_BLUE],
					    p_new2[CAIRO_ALPHA],
					    x2min,
					    y2min);
			}

			p_new2 += 4;
			x += 1.0;
		}

		p_new += new_rowstride;
		y += 1.0;
	}

	out:

	cairo_surface_mark_dirty (rotated);
	cairo_surface_destroy (image_with_background);

#undef INTERPOLATE
#undef GET_VALUES

	return rotated;
}


cairo_surface_t *
_cairo_image_surface_rotate (cairo_surface_t *image,
		    	     double           angle,
		    	     gboolean         high_quality,
		    	     GdkRGBA         *background_color,
		    	     GthAsyncTask    *task)
{
	cairo_surface_t *rotated;
	cairo_surface_t *tmp = NULL;

	if (angle >= 90.0) {
		image = tmp = _cairo_image_surface_transform (image, GTH_TRANSFORM_ROTATE_90);
		angle -= 90.0;
	}
	else if (angle <= -90.0) {
		image = tmp = _cairo_image_surface_transform (image, GTH_TRANSFORM_ROTATE_270);
		angle += 90.0;
	}

	if (angle != 0.0)
		rotated = rotate (image,
				  -angle,
				  high_quality,
				  background_color->red * 255.0,
				  background_color->green * 255.0,
				  background_color->blue * 255.0,
				  background_color->alpha * 255.0,
				  task);
	else
		rotated = cairo_surface_reference (image);

	if (tmp != NULL)
		cairo_surface_destroy (tmp);

	return rotated;
}
