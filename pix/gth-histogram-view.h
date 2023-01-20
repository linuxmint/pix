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

#ifndef GTH_HISTOGRAM_VIEW_H
#define GTH_HISTOGRAM_VIEW_H

#include <glib.h>
#include <gtk/gtk.h>
#include "gth-histogram.h"

G_BEGIN_DECLS

#define GTH_TYPE_HISTOGRAM_VIEW (gth_histogram_view_get_type ())
#define GTH_HISTOGRAM_VIEW(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_HISTOGRAM_VIEW, GthHistogramView))
#define GTH_HISTOGRAM_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_HISTOGRAM_VIEW, GthHistogramViewClass))
#define GTH_IS_HISTOGRAM_VIEW(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_HISTOGRAM_VIEW))
#define GTH_IS_HISTOGRAM_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_HISTOGRAM_VIEW))
#define GTH_HISTOGRAM_VIEW_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_HISTOGRAM_VIEW, GthHistogramViewClass))

typedef struct _GthHistogramView GthHistogramView;
typedef struct _GthHistogramViewClass GthHistogramViewClass;
typedef struct _GthHistogramViewPrivate GthHistogramViewPrivate;

typedef enum {
	GTH_HISTOGRAM_MODE_ONE_CHANNEL,
	GTH_HISTOGRAM_MODE_ALL_CHANNELS
} GthHistogramMode;

typedef enum {
	GTH_HISTOGRAM_SCALE_LINEAR,
	GTH_HISTOGRAM_SCALE_LOGARITHMIC
} GthHistogramScale;

struct _GthHistogramView {
	GtkBox parent_instance;
	GthHistogramViewPrivate *priv;
};

struct _GthHistogramViewClass {
	GtkBoxClass parent_class;
};

GType              gth_histogram_view_get_type              (void);
GtkWidget *        gth_histogram_view_new                   (GthHistogram      *histogram);
void               gth_histogram_view_set_histogram         (GthHistogramView  *self,
							     GthHistogram      *histogram);
GthHistogram *     gth_histogram_view_get_histogram         (GthHistogramView  *self);
void               gth_histogram_view_set_current_channel   (GthHistogramView  *self,
							     int                n_channel);
int                gth_histogram_view_get_current_channel   (GthHistogramView  *self);
void               gth_histogram_view_set_display_mode      (GthHistogramView  *self,
							     GthHistogramMode   mode);
GthHistogramMode   gth_histogram_view_get_display_mode      (GthHistogramView  *self);
void               gth_histogram_view_set_scale_type        (GthHistogramView  *self,
							     GthHistogramScale  scale);
GthHistogramScale  gth_histogram_view_get_scale_type        (GthHistogramView  *self);
void               gth_histogram_view_set_selection         (GthHistogramView  *self,
							     guchar             start,
							     guchar             end);
void               gth_histogram_view_show_info             (GthHistogramView  *self,
							     gboolean           show_info);

G_END_DECLS

#endif /* GTH_HISTOGRAM_VIEW_H */
