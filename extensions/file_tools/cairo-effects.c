/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2014 The Free Software Foundation, Inc.
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
#include "cairo-effects.h"
#include "gth-curve.h"


gboolean
cairo_image_surface_apply_curves (cairo_surface_t  *source,
				  GthCurve        **curve,
				  GthAsyncTask     *task)
{
	long		*value_map[GTH_HISTOGRAM_N_CHANNELS];
	int		 c, v;
	int              width;
	int              height;
	int              source_stride;
	unsigned char   *p_source_line;
	int              x, y, temp;
	gboolean         cancelled = FALSE;
	double           progress;
	unsigned char   *p_source;
	unsigned char    image_red, image_green, image_blue, image_alpha;

	for (c = GTH_HISTOGRAM_CHANNEL_VALUE; c <= GTH_HISTOGRAM_CHANNEL_BLUE; c++) {
		value_map[c] = g_new (long, 256);
		for (v = 0; v <= 255; v++) {
			double u = gth_curve_eval (curve[c], v);
			if (c > GTH_HISTOGRAM_CHANNEL_VALUE)
				u = value_map[GTH_HISTOGRAM_CHANNEL_VALUE][(int)u];
			value_map[c][v] = u;
		}
	}

	width = cairo_image_surface_get_width (source);
	height = cairo_image_surface_get_height (source);
	source_stride = cairo_image_surface_get_stride (source);

	p_source_line = _cairo_image_surface_flush_and_get_data (source);
	for (y = 0; y < height; y++) {
		gth_async_task_get_data (task, NULL, &cancelled, NULL);
		if (cancelled)
			break;

		progress = (double) y / height;
		gth_async_task_set_data (task, NULL, NULL, &progress);

		p_source = p_source_line;
		for (x = 0; x < width; x++) {
			CAIRO_GET_RGBA (p_source, image_red, image_green, image_blue, image_alpha);

			image_red   = value_map[GTH_HISTOGRAM_CHANNEL_RED][image_red];
			image_green = value_map[GTH_HISTOGRAM_CHANNEL_GREEN][image_green];
			image_blue  = value_map[GTH_HISTOGRAM_CHANNEL_BLUE][image_blue];

			CAIRO_SET_RGBA (p_source, image_red, image_green, image_blue, image_alpha);

			p_source += 4;
		}
		p_source_line += source_stride;
	}
	cairo_surface_mark_dirty (source);

	for (c = GTH_HISTOGRAM_CHANNEL_VALUE; c <= GTH_HISTOGRAM_CHANNEL_BLUE; c++)
		g_free (value_map[c]);

	return ! cancelled;
}


gboolean
cairo_image_surface_apply_vignette (cairo_surface_t  *source,
				    GthCurve        **curve,
				    guchar	      vignette_alpha,
				    GthAsyncTask     *task)
{
	gboolean         local_curves;
	long		*value_map[GTH_HISTOGRAM_N_CHANNELS];
	int		 c, v;
	int              width;
	int              height;
	int              source_stride;
	unsigned char   *p_source_line;
	int              x, y;
	gboolean         cancelled = FALSE;
	double           progress;
	unsigned char   *p_source;
	int              image_red, image_green, image_blue, image_alpha;
	int              red, green, blue, alpha;
	double           center_x, center_y,  d, min_d, max_d;
	int              temp;
	GthPoint         f1, f2, p;

	gimp_op_init ();

	local_curves = (curve == NULL);
	if (local_curves) {
		curve = g_new (GthCurve *, GTH_HISTOGRAM_N_CHANNELS);
		curve[GTH_HISTOGRAM_CHANNEL_VALUE] = gth_curve_new_for_points (GTH_TYPE_BEZIER, 3, 0,0, 158,95, /*152,103,*/ 255,255);
		curve[GTH_HISTOGRAM_CHANNEL_RED] = gth_curve_new_for_points (GTH_TYPE_BEZIER, 0);
		curve[GTH_HISTOGRAM_CHANNEL_GREEN] = gth_curve_new_for_points (GTH_TYPE_BEZIER, 0);
		curve[GTH_HISTOGRAM_CHANNEL_BLUE] = gth_curve_new_for_points (GTH_TYPE_BEZIER, 0);
	}

	for (c = GTH_HISTOGRAM_CHANNEL_VALUE; c <= GTH_HISTOGRAM_CHANNEL_BLUE; c++) {
		value_map[c] = g_new (long, 256);
		for (v = 0; v <= 255; v++) {
			double u = gth_curve_eval (curve[c], v);
			if (c > GTH_HISTOGRAM_CHANNEL_VALUE)
				u = value_map[GTH_HISTOGRAM_CHANNEL_VALUE][(int)u];
			value_map[c][v] = u;
		}
	}

	width = cairo_image_surface_get_width (source);
	height = cairo_image_surface_get_height (source);
	source_stride = cairo_image_surface_get_stride (source);

	center_x = width / 2.0;
	center_y = height / 2.0;

	{
		double a = MAX (width, height) / 2.0;
		double b = MIN (height, width) / 2.0;

		a = a - (a / 1.5);
		b = b - (b / 1.5);

		double e = sqrt (1.0 - SQR (b) / SQR (a));
		double c = a * e;
		min_d = 2 * sqrt (SQR (b) + SQR (c));

		if (width > height) {
			f1.x = center_x - c;
			f1.y = center_y;
			f2.x = center_x + c;
			f2.y = center_y;
		}
		else {
			f1.x = center_x;
			f1.y = center_y - c;
			f2.x = center_x;
			f2.y = center_y + c;
		}

		p.x = 0;
		p.y = 0;
		max_d = gth_point_distance (&p, &f1) + gth_point_distance (&p, &f2);
	}

	p_source_line = _cairo_image_surface_flush_and_get_data (source);
	for (y = 0; y < height; y++) {
		gth_async_task_get_data (task, NULL, &cancelled, NULL);
		if (cancelled)
			break;

		progress = (double) y / height;
		gth_async_task_set_data (task, NULL, NULL, &progress);

		p_source = p_source_line;
		for (x = 0; x < width; x++) {
			p.x = x;
			p.y = y;
			d = gth_point_distance (&p, &f1) + gth_point_distance (&p, &f2);
			if (d >= min_d) {
				CAIRO_GET_RGBA (p_source, image_red, image_green, image_blue, image_alpha);
				red   = value_map[GTH_HISTOGRAM_CHANNEL_RED][image_red];
				green = value_map[GTH_HISTOGRAM_CHANNEL_GREEN][image_green];
				blue  = value_map[GTH_HISTOGRAM_CHANNEL_BLUE][image_blue];

				if (d <= max_d)
					alpha = 255 * ((d - min_d) / (max_d - min_d));
				else
					alpha = 255;
				alpha = ADD_ALPHA (alpha, vignette_alpha);

				p_source[CAIRO_RED] = GIMP_OP_NORMAL (red, image_red, alpha);
				p_source[CAIRO_GREEN] = GIMP_OP_NORMAL (green, image_green, alpha);
				p_source[CAIRO_BLUE] = GIMP_OP_NORMAL (blue, image_blue, alpha);
				p_source[CAIRO_ALPHA] = GIMP_OP_NORMAL (255, image_alpha, alpha);
			}

			p_source += 4;
		}
		p_source_line += source_stride;
	}
	cairo_surface_mark_dirty (source);

	if (local_curves) {
		for (c = GTH_HISTOGRAM_CHANNEL_VALUE; c <= GTH_HISTOGRAM_CHANNEL_BLUE; c++) {
			g_object_unref (curve[c]);
			g_free (value_map[c]);
		}
	}

	return ! cancelled;
}


gboolean
cairo_image_surface_apply_bcs (cairo_surface_t  *source,
			       double            brightness,
			       double            contrast,
			       double            saturation,
			       GthAsyncTask     *task)
{
	PixbufCache     *cache;
	int              width;
	int              height;
	int              source_stride;
	unsigned char   *p_source_line;
	int              x, y;
	gboolean         cancelled = FALSE;
	double           progress;
	unsigned char   *p_source;
	unsigned char    values[4];
	int              temp, value;

	gimp_op_init ();
	cache = pixbuf_cache_new ();

	if (saturation < 0)
		saturation = tan (saturation * G_PI_2);

	width = cairo_image_surface_get_width (source);
	height = cairo_image_surface_get_height (source);
	source_stride = cairo_image_surface_get_stride (source);
	p_source_line = _cairo_image_surface_flush_and_get_data (source);

	for (y = 0; y < height; y++) {
		gth_async_task_get_data (task, NULL, &cancelled, NULL);
		if (cancelled)
			break;

		progress = (double) y / height;
		gth_async_task_set_data (task, NULL, NULL, &progress);

		p_source = p_source_line;
		for (x = 0; x < width; x++) {
			int channel;

			CAIRO_GET_RGBA (p_source, values[0], values[1], values[2], values[3]);

			/* brightness / contrast */

			for (channel = 0; channel < 3; channel++) {
				value = values[channel];

				if (! pixbuf_cache_get (cache, channel + 1, &value)) {
					int tmp = value;

					if (brightness > 0)
						tmp = interpolate_value (value, 0, brightness);
					else if (brightness < 0)
						tmp = interpolate_value (value, 255, - brightness);
					value = CLAMP (tmp, 0, 255);

					if (contrast < 0)
						tmp = interpolate_value (value, 127, tan (contrast * G_PI_2));
					else if (contrast > 0)
						tmp = interpolate_value (value, 127, contrast);
					value = CLAMP (tmp, 0, 255);

					pixbuf_cache_set (cache, channel + 1, values[channel], value);
				}

				values[channel] = value;
			}

			/* saturation */

			if (saturation != 0.0) {
				guchar min, max, lightness;
				int    tmp;

				max = MAX (MAX (values[0], values[1]), values[2]);
				min = MIN (MIN (values[0], values[1]), values[2]);
				lightness = (max + min) / 2;

				tmp = interpolate_value (values[0], lightness, saturation);
				values[0] = CLAMP (tmp, 0, 255);

				tmp = interpolate_value (values[1], lightness, saturation);
				values[1] = CLAMP (tmp, 0, 255);

				tmp = interpolate_value (values[2], lightness, saturation);
				values[2] = CLAMP (tmp, 0, 255);
			}

			CAIRO_SET_RGBA (p_source, values[0], values[1], values[2], values[3]);

			p_source += 4;
		}
		p_source_line += source_stride;
	}
	cairo_surface_mark_dirty (source);

	pixbuf_cache_free (cache);

	return ! cancelled;
}


gboolean
cairo_image_surface_colorize (cairo_surface_t  *source,
			      guchar            color_red,
			      guchar            color_green,
			      guchar            color_blue,
			      guchar            color_alpha,
			      GthAsyncTask     *task)
{
	int		 i;
	double           midtone_distance[256];
	int              width;
	int              height;
	int              source_stride;
	unsigned char   *p_source_line;
	int              x, y;
	gboolean         cancelled = FALSE;
	double           progress;
	unsigned char   *p_source;
	int              image_red, image_green, image_blue, image_alpha;
	int              red, green, blue, alpha;
	int              temp, min, max, lightness;

	gimp_op_init ();
	for (i = 0; i < 256; i++)
		midtone_distance[i] = 0.667 * (1 - SQR (((double) i - 127.0) / 127.0));

	width = cairo_image_surface_get_width (source);
	height = cairo_image_surface_get_height (source);
	source_stride = cairo_image_surface_get_stride (source);
	p_source_line = _cairo_image_surface_flush_and_get_data (source);

	for (y = 0; y < height; y++) {
		gth_async_task_get_data (task, NULL, &cancelled, NULL);
		if (cancelled)
			break;

		progress = (double) y / height;
		gth_async_task_set_data (task, NULL, NULL, &progress);

		p_source = p_source_line;
		for (x = 0; x < width; x++) {
			CAIRO_GET_RGBA (p_source, image_red, image_green, image_blue, image_alpha);

			/* desaturate */

			max = MAX (MAX (image_red, image_green), image_blue);
			min = MIN (MIN (image_red, image_green), image_blue);
			lightness = (max + min) / 2;

			/* colorize */

			red = lightness + color_red * midtone_distance[lightness];
			green = lightness + color_green * midtone_distance[lightness];
			blue = lightness + color_blue * midtone_distance[lightness];
			alpha = ADD_ALPHA (image_alpha, color_alpha);

			p_source[CAIRO_RED] = GIMP_OP_NORMAL (red, image_red, alpha);
			p_source[CAIRO_GREEN] = GIMP_OP_NORMAL (green, image_green, alpha);
			p_source[CAIRO_BLUE] = GIMP_OP_NORMAL (blue, image_blue, alpha);
			p_source[CAIRO_ALPHA] = GIMP_OP_NORMAL (255, image_alpha, alpha);

			p_source += 4;
		}
		p_source_line += source_stride;
	}
	cairo_surface_mark_dirty (source);

	return ! cancelled;
}


gboolean
cairo_image_surface_add_color (cairo_surface_t  *source,
			       guchar            color_red,
			       guchar            color_green,
			       guchar            color_blue,
			       guchar            color_alpha,
			       GthAsyncTask     *task)
{
	int              width;
	int              height;
	int              source_stride;
	unsigned char   *p_source_line;
	int              x, y;
	gboolean         cancelled = FALSE;
	double           progress;
	unsigned char   *p_source;
	int              image_red, image_green, image_blue, image_alpha;
	int              temp, alpha;

	gimp_op_init ();

	width = cairo_image_surface_get_width (source);
	height = cairo_image_surface_get_height (source);
	source_stride = cairo_image_surface_get_stride (source);
	p_source_line = _cairo_image_surface_flush_and_get_data (source);

	for (y = 0; y < height; y++) {
		gth_async_task_get_data (task, NULL, &cancelled, NULL);
		if (cancelled)
			break;

		progress = (double) y / height;
		gth_async_task_set_data (task, NULL, NULL, &progress);

		p_source = p_source_line;
		for (x = 0; x < width; x++) {
			CAIRO_GET_RGBA (p_source, image_red, image_green, image_blue, image_alpha);

			alpha = ADD_ALPHA (image_alpha, color_alpha);

			p_source[CAIRO_RED] = GIMP_OP_NORMAL (color_red, image_red, alpha);
			p_source[CAIRO_GREEN] = GIMP_OP_NORMAL (color_green, image_green, alpha);
			p_source[CAIRO_BLUE] = GIMP_OP_NORMAL (color_blue, image_blue, alpha);
			p_source[CAIRO_ALPHA] = GIMP_OP_NORMAL (255, image_alpha, alpha);

			p_source += 4;
		}
		p_source_line += source_stride;
	}
	cairo_surface_mark_dirty (source);

	return ! cancelled;
}

