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

#ifndef GTH_COLOR_SCALE_H
#define GTH_COLOR_SCALE_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GTH_TYPE_COLOR_SCALE            (gth_color_scale_get_type ())
#define GTH_COLOR_SCALE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_COLOR_SCALE, GthColorScale))
#define GTH_COLOR_SCALE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_COLOR_SCALE, GthColorScaleClass))
#define GTH_IS_COLOR_SCALE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_COLOR_SCALE))
#define GTH_IS_COLOR_SCALE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_COLOR_SCALE))
#define GTH_COLOR_SCALE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_COLOR_SCALE, GthColorScaleClass))

typedef struct _GthColorScale        GthColorScale;
typedef struct _GthColorScaleClass   GthColorScaleClass;
typedef struct _GthColorScalePrivate GthColorScalePrivate;

struct _GthColorScale {
	GtkScale __parent;
	GthColorScalePrivate *priv;
};

struct _GthColorScaleClass {
	GtkScaleClass __parent;
};

typedef enum {
	GTH_COLOR_SCALE_DEFAULT,
	GTH_COLOR_SCALE_WHITE_BLACK,
	GTH_COLOR_SCALE_BLACK_WHITE,
	GTH_COLOR_SCALE_GRAY_BLACK,
	GTH_COLOR_SCALE_GRAY_WHITE,
	GTH_COLOR_SCALE_CYAN_RED,
	GTH_COLOR_SCALE_MAGENTA_GREEN,
	GTH_COLOR_SCALE_YELLOW_BLUE
} GthColorScaleType;

GType            gth_color_scale_get_type    (void);
GtkWidget *      gth_color_scale_new         (GtkAdjustment      *adjustment,
					      GthColorScaleType   scale_type);
GtkAdjustment *  gth_color_scale_label_new   (GtkWidget          *parent_box,
					      GtkLabel           *related_label,
					      GthColorScaleType   scale_type,
					      float               value,
					      float               lower,
					      float               upper,
					      float               step_increment,
					      float               page_increment,
					      const char         *format);

G_END_DECLS

#endif /* GTH_COLOR_SCALE_H */
