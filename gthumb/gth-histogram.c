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


#include <string.h>
#include "cairo-utils.h"
#include "gth-histogram.h"


/* Signals */
enum {
        CHANGED,
        LAST_SIGNAL
};


struct _GthHistogramPrivate {
	int **values;
	int  *values_max;
	int   n_channels;
};


static guint gth_histogram_signals[LAST_SIGNAL] = { 0 };


G_DEFINE_TYPE (GthHistogram, gth_histogram, G_TYPE_OBJECT)


static void
gth_histogram_finalize (GObject *object)
{
	GthHistogram *self;
	int           i;

	self = GTH_HISTOGRAM (object);

	for (i = 0; i < GTH_HISTOGRAM_N_CHANNELS + 1; i++)
		g_free (self->priv->values[i]);
	g_free (self->priv->values);
	g_free (self->priv->values_max);

	G_OBJECT_CLASS (gth_histogram_parent_class)->finalize (object);
}


static void
gth_histogram_class_init (GthHistogramClass *klass)
{
	GObjectClass *object_class;

	g_type_class_add_private (klass, sizeof (GthHistogramPrivate));

	object_class = (GObjectClass*) klass;
	object_class->finalize = gth_histogram_finalize;

	/* signals */

	gth_histogram_signals[CHANGED] =
                g_signal_new ("changed",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GthHistogramClass, changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);
}


static void
gth_histogram_init (GthHistogram *self)
{
	int i;

	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_HISTOGRAM, GthHistogramPrivate);
	self->priv->values = g_new0 (int *, GTH_HISTOGRAM_N_CHANNELS + 1);
	for (i = 0; i < GTH_HISTOGRAM_N_CHANNELS + 1; i++)
		self->priv->values[i] = g_new0 (int, 256);
	self->priv->values_max = g_new0 (int, GTH_HISTOGRAM_N_CHANNELS + 1);
}


GthHistogram *
gth_histogram_new (void)
{
	return (GthHistogram *) g_object_new (GTH_TYPE_HISTOGRAM, NULL);
}


static void
histogram_reset_values (GthHistogram *self)
{
	int i;

	for (i = 0; i < GTH_HISTOGRAM_N_CHANNELS + 1; i++) {
		memset (self->priv->values[i], 0, sizeof (int) * 256);
		self->priv->values_max[i] = 0;
	}
}


static void
gth_histogram_changed (GthHistogram *self)
{
	g_signal_emit (self, gth_histogram_signals[CHANGED], 0);
}


void
gth_histogram_calculate_for_image (GthHistogram    *self,
				   cairo_surface_t *image)
{
	int    **values;
	int     *values_max;
	int      width, height, has_alpha;
	int      rowstride;
	guchar  *line, *pixel;
	int      i, j, max;
	guchar   red, green, blue, alpha;

	g_return_if_fail (GTH_IS_HISTOGRAM (self));

	values = self->priv->values;
	values_max = self->priv->values_max;

	if (image == NULL) {
		self->priv->n_channels = 0;
		histogram_reset_values (self);
		gth_histogram_changed (self);
		return;
	}

	has_alpha  = _cairo_image_surface_get_has_alpha (image);
	rowstride  = cairo_image_surface_get_stride (image);
	line       = cairo_image_surface_get_data (image);
	width      = cairo_image_surface_get_width (image);
	height     = cairo_image_surface_get_height (image);

	self->priv->n_channels = (has_alpha ? 4 : 3) + 1;
	histogram_reset_values (self);

	for (i = 0; i < height; i++) {
		pixel = line;

		for (j = 0; j < width; j++) {
			CAIRO_GET_RGBA (pixel, red, green, blue, alpha);

			/* count values for each RGB channel */

			values[GTH_HISTOGRAM_CHANNEL_RED][red] += 1;
			values[GTH_HISTOGRAM_CHANNEL_GREEN][green] += 1;
			values[GTH_HISTOGRAM_CHANNEL_BLUE][blue] += 1;
			if (has_alpha)
				values[GTH_HISTOGRAM_CHANNEL_ALPHA][alpha] += 1;

			/* count value for Value channel */

			max = MAX (MAX (red, green), blue);
			values[GTH_HISTOGRAM_CHANNEL_VALUE][max] += 1;

			/* track max value for each channel */

			values_max[GTH_HISTOGRAM_CHANNEL_VALUE] = MAX (values_max[GTH_HISTOGRAM_CHANNEL_VALUE], values[GTH_HISTOGRAM_CHANNEL_VALUE][max]);
			values_max[GTH_HISTOGRAM_CHANNEL_RED] = MAX (values_max[GTH_HISTOGRAM_CHANNEL_RED], values[GTH_HISTOGRAM_CHANNEL_RED][red]);
			values_max[GTH_HISTOGRAM_CHANNEL_GREEN] = MAX (values_max[GTH_HISTOGRAM_CHANNEL_GREEN], values[GTH_HISTOGRAM_CHANNEL_GREEN][green]);
			values_max[GTH_HISTOGRAM_CHANNEL_BLUE] = MAX (values_max[GTH_HISTOGRAM_CHANNEL_BLUE], values[GTH_HISTOGRAM_CHANNEL_BLUE][blue]);
			if (has_alpha)
				values_max[GTH_HISTOGRAM_CHANNEL_ALPHA] = MAX (values_max[GTH_HISTOGRAM_CHANNEL_ALPHA], values[GTH_HISTOGRAM_CHANNEL_ALPHA][alpha]);

			pixel += 4;
		}

		line += rowstride;
	}

	gth_histogram_changed (self);
}


double
gth_histogram_get_count (GthHistogram *self,
			 int           start,
			 int           end)
{
	int    i;
	double count = 0;

	g_return_val_if_fail (self != NULL, 0.0);

	for (i = start; i <= end; i++)
		count += self->priv->values[0][i];

	return count;
}


double
gth_histogram_get_value (GthHistogram        *self,
			 GthHistogramChannel  channel,
			 int                  bin)
{
	g_return_val_if_fail (self != NULL, 0.0);

	if ((channel < self->priv->n_channels) && (bin >= 0) && (bin < 256))
		return (double) self->priv->values[channel][bin];

	return 0.0;
}


double
gth_histogram_get_channel (GthHistogram        *self,
			   GthHistogramChannel  channel,
			   int                  bin)
{
	g_return_val_if_fail (self != NULL, 0.0);

	if (self->priv->n_channels > 3)
		return gth_histogram_get_value (self, channel + 1, bin);
	else
		return gth_histogram_get_value (self, channel, bin);
}


double
gth_histogram_get_channel_max (GthHistogram        *self,
			       GthHistogramChannel  channel)
{
	g_return_val_if_fail (self != NULL, 0.0);

	if (channel < self->priv->n_channels)
		return (double) self->priv->values_max[channel];

	return 0.0;
}


double
gth_histogram_get_max (GthHistogram *self)
{
	int    i;
	double max = -1.0;

	g_return_val_if_fail (self != NULL, 0.0);

	for (i = 0; i < self->priv->n_channels; i++)
		max = MAX (max, (double) self->priv->values_max[i]);

	return max;
}


int
gth_histogram_get_nchannels (GthHistogram *self)
{
	g_return_val_if_fail (self != NULL, 0.0);
	return self->priv->n_channels - 1;
}


long **
gth_histogram_get_cumulative (GthHistogram *self)
{
	long **cumulative;
	int    c, v;

	cumulative = g_new (long *, GTH_HISTOGRAM_N_CHANNELS);
	for (c = 0; c < GTH_HISTOGRAM_N_CHANNELS; c++) {
		cumulative[c] = g_new (long, 256);
		cumulative[c][0] = gth_histogram_get_value (self, c, 0);
		for (v = 1; v < 256; v++) {
			cumulative[c][v] = gth_histogram_get_value (self, c, v) + cumulative[c][v - 1];
		}
	}

	return cumulative;
}


void
gth_cumulative_histogram_free (long **cumulative)
{
	int c;

	for (c = 0; c < GTH_HISTOGRAM_N_CHANNELS; c++)
		g_free (cumulative[c]);
	g_free (cumulative);
}
