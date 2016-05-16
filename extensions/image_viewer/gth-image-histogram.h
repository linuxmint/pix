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

#ifndef GTH_IMAGE_HISTOGRAM_H
#define GTH_IMAGE_HISTOGRAM_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GTH_TYPE_IMAGE_HISTOGRAM            (gth_image_histogram_get_type ())
#define GTH_IMAGE_HISTOGRAM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_IMAGE_HISTOGRAM, GthImageHistogram))
#define GTH_IMAGE_HISTOGRAM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_IMAGE_HISTOGRAM, GthImageHistogramClass))
#define GTH_IS_IMAGE_HISTOGRAM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_IMAGE_HISTOGRAM))
#define GTH_IS_IMAGE_HISTOGRAM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_IMAGE_HISTOGRAM))
#define GTH_IMAGE_HISTOGRAM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_IMAGE_HISTOGRAM, GthImageHistogramClass))

typedef struct _GthImageHistogram GthImageHistogram;
typedef struct _GthImageHistogramClass GthImageHistogramClass;
typedef struct _GthImageHistogramPrivate GthImageHistogramPrivate;

struct _GthImageHistogram {
	GtkBox parent_instance;
	GthImageHistogramPrivate *priv;
};

struct _GthImageHistogramClass {
	GtkBoxClass parent_class;
};

GType gth_image_histogram_get_type (void);

G_END_DECLS

#endif /* GTH_IMAGE_HISTOGRAM_H */
