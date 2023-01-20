/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2014 Free Software Foundation, Inc.
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
#include <gthumb.h>
#include "cairo-blur.h"
#include "cairo-effects.h"
#include "gth-curve.h"
#include "gth-file-tool-lomo.h"


static void
lomo_init (GthAsyncTask *task,
	   gpointer      user_data)
{
	gimp_op_init ();
}


static gpointer
lomo_exec (GthAsyncTask *task,
	   gpointer      user_data)
{
	cairo_surface_t *original;
	cairo_surface_t *source;
	GthCurve	*curve[GTH_HISTOGRAM_N_CHANNELS];
	cairo_format_t   format;
	int              width;
	int              height;
	int              source_stride;
	cairo_surface_t *destination;
	int              destination_stride;
	cairo_surface_t *blurred;
	int              blurred_stride;
	unsigned char   *p_source_line;
	unsigned char   *p_destination_line;
	unsigned char   *p_blurred_line;
	unsigned char   *p_source;
	unsigned char   *p_destination;
	unsigned char   *p_blurred;
	gboolean         cancelled = FALSE;
	double           progress;
	int              x, y;
	double           center_x, center_y, radius, d;
	int              red, green, blue, alpha;
	int              image_red, image_green, image_blue, image_alpha;
	int              layer_red, layer_green, layer_blue, layer_alpha;
	int              temp;
	int              c;

	/* curves layer */

	original = gth_image_task_get_source_surface (GTH_IMAGE_TASK (task));
	source = _cairo_image_surface_copy (original);

	curve[GTH_HISTOGRAM_CHANNEL_VALUE] = gth_curve_new_for_points (GTH_TYPE_BEZIER, 0);
	curve[GTH_HISTOGRAM_CHANNEL_RED] = gth_curve_new_for_points (GTH_TYPE_BEZIER, 4, 0, 0, 56, 45, 195, 197, 255, 216);
	curve[GTH_HISTOGRAM_CHANNEL_GREEN] = gth_curve_new_for_points (GTH_TYPE_BEZIER, 4, 0, 0, 65, 40, 162, 174, 238, 255);
	curve[GTH_HISTOGRAM_CHANNEL_BLUE] = gth_curve_new_for_points (GTH_TYPE_BEZIER, 4, 0, 0, 68, 79, 210, 174, 255, 255);
	if (! cairo_image_surface_apply_curves (source, curve, task)) {
		cairo_surface_destroy (source);
		cairo_surface_destroy (original);
		return NULL;
	}

	format = cairo_image_surface_get_format (source);
	width = cairo_image_surface_get_width (source);
	height = cairo_image_surface_get_height (source);
	source_stride = cairo_image_surface_get_stride (source);

	cairo_surface_destroy (original);

	/* other effects */

	blurred = _cairo_image_surface_copy (source);
	blurred_stride = cairo_image_surface_get_stride (blurred);
	if (! _cairo_image_surface_blur (blurred, 1, task)) {
		cairo_surface_destroy (blurred);
		cairo_surface_destroy (source);
		return NULL;
	}

	center_x = width / 2.0;
	center_y = height / 2.0;
	radius = MAX (width, height) / 2.0;

	destination = cairo_image_surface_create (format, width, height);
	destination_stride = cairo_image_surface_get_stride (destination);

	p_source_line = _cairo_image_surface_flush_and_get_data (source);
	p_blurred_line = _cairo_image_surface_flush_and_get_data (blurred);
	p_destination_line = _cairo_image_surface_flush_and_get_data (destination);
	for (y = 0; y < height; y++) {
		gth_async_task_get_data (task, NULL, &cancelled, NULL);
		if (cancelled)
			break;

		progress = (double) y / height;
		gth_async_task_set_data (task, NULL, NULL, &progress);

		p_source = p_source_line;
		p_blurred = p_blurred_line;
		p_destination = p_destination_line;
		for (x = 0; x < width; x++) {
			d = sqrt (SQR (x - center_x) + SQR (y - center_y));
			d = CLAMP_PIXEL ((d >= radius) ? 0 : 255 - (255.0 * (d / radius)));

			CAIRO_GET_RGBA (p_source, image_red, image_green, image_blue, image_alpha);

			/* blurred image layer with a radial mask (to blur the edges) */

			CAIRO_GET_RGBA (p_blurred, layer_red, layer_green, layer_blue, layer_alpha);
			layer_alpha = (guchar) (255 - d);
			image_red = GIMP_OP_NORMAL (layer_red, image_red, layer_alpha);
			image_green = GIMP_OP_NORMAL (layer_green, image_green, layer_alpha);
			image_blue = GIMP_OP_NORMAL (layer_blue, image_blue, layer_alpha);

			/* radial grandient layer in overlay mode */

			layer_green = layer_blue = layer_red = d;
			layer_alpha = 255;

			red = GIMP_OP_SOFT_LIGHT (layer_red, image_red);
			green = GIMP_OP_SOFT_LIGHT (layer_green, image_green);
			blue = GIMP_OP_SOFT_LIGHT (layer_blue, image_blue);
			alpha = ADD_ALPHA (image_alpha, layer_alpha);

			p_destination[CAIRO_RED] = GIMP_OP_NORMAL (red, image_red, alpha);
			p_destination[CAIRO_GREEN] = GIMP_OP_NORMAL (green, image_green, alpha);
			p_destination[CAIRO_BLUE] = GIMP_OP_NORMAL (blue, image_blue, alpha);
			p_destination[CAIRO_ALPHA] = GIMP_OP_NORMAL (255, image_alpha, alpha);

			p_source += 4;
			p_blurred += 4;
			p_destination += 4;
		}
		p_source_line += source_stride;
		p_blurred_line += blurred_stride;
		p_destination_line += destination_stride;
	}

	if (! cancelled) {
		cairo_surface_mark_dirty (destination);
		gth_image_task_set_destination_surface (GTH_IMAGE_TASK (task), destination);
	}

	cairo_surface_destroy (destination);
	cairo_surface_destroy (blurred);
	cairo_surface_destroy (source);

	for (c = GTH_HISTOGRAM_CHANNEL_VALUE; c <= GTH_HISTOGRAM_CHANNEL_BLUE; c++)
		g_object_unref (curve[c]);

	return NULL;
}


void
lomo_add_to_special_effects (GthFilterGrid *grid)
{
	gth_filter_grid_add_filter (grid,
				    GTH_FILTER_GRID_NEW_FILTER_ID,
				    gth_image_task_new (_("Applying changes"), lomo_init, lomo_exec, NULL, NULL, NULL),
				    _("Lomo"),
				    NULL);
}
