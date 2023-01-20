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

#ifndef GTH_HISTOGRAM_H
#define GTH_HISTOGRAM_H

#include <glib-object.h>
#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

G_BEGIN_DECLS

#define GTH_TYPE_HISTOGRAM         (gth_histogram_get_type ())
#define GTH_HISTOGRAM(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GTH_TYPE_HISTOGRAM, GthHistogram))
#define GTH_HISTOGRAM_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GTH_TYPE_HISTOGRAM, GthHistogramClass))
#define GTH_IS_HISTOGRAM(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTH_TYPE_HISTOGRAM))
#define GTH_IS_HISTOGRAM_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GTH_TYPE_HISTOGRAM))
#define GTH_HISTOGRAM_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), GTH_TYPE_HISTOGRAM, GthHistogramClass))

typedef struct _GthHistogram         GthHistogram;
typedef struct _GthHistogramPrivate  GthHistogramPrivate;
typedef struct _GthHistogramClass    GthHistogramClass;

typedef enum {
	GTH_HISTOGRAM_CHANNEL_VALUE = 0,
	GTH_HISTOGRAM_CHANNEL_RED,
	GTH_HISTOGRAM_CHANNEL_GREEN,
	GTH_HISTOGRAM_CHANNEL_BLUE,
	GTH_HISTOGRAM_CHANNEL_ALPHA,
	GTH_HISTOGRAM_N_CHANNELS
} GthHistogramChannel;

struct _GthHistogram
{
	GObject __parent;
	GthHistogramPrivate *priv;
};

struct _GthHistogramClass {
	GObjectClass __parent_class;

	/*< signals >*/

	void  (*changed)  (GthHistogram *self);
};

GType          gth_histogram_get_type             (void) G_GNUC_CONST;
GthHistogram * gth_histogram_new                  (void);
void           gth_histogram_calculate_for_image  (GthHistogram        *self,
						   cairo_surface_t     *image);
double         gth_histogram_get_count            (GthHistogram        *self,
						   int                  start,
						   int                  end);
double         gth_histogram_get_value            (GthHistogram        *self,
						   GthHistogramChannel  channel,
						   int                  bin);
double         gth_histogram_get_channel          (GthHistogram        *self,
						   GthHistogramChannel  channel,
						   int                  bin);
double         gth_histogram_get_channel_max      (GthHistogram        *self,
						   GthHistogramChannel  channel);
double         gth_histogram_get_max              (GthHistogram        *self);
guchar         gth_histogram_get_min_value        (GthHistogram        *self,
						   GthHistogramChannel  channel);
guchar         gth_histogram_get_max_value        (GthHistogram        *self,
						   GthHistogramChannel  channel);
int            gth_histogram_get_n_pixels         (GthHistogram        *self);
int            gth_histogram_get_nchannels        (GthHistogram        *self);
long **        gth_histogram_get_cumulative       (GthHistogram        *self);
void           gth_cumulative_histogram_free      (long               **cumulative);

G_END_DECLS

#endif /* GTH_HISTOGRAM_H */
