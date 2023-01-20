/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001-2014 The Free Software Foundation, Inc.
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

#ifndef GTH_CURVE_H
#define GTH_CURVE_H

#include <glib.h>
#include <gtk/gtk.h>
#include <gthumb.h>
#include "gth-points.h"

G_BEGIN_DECLS


/* -- GthCurve -- */


#define GTH_TYPE_CURVE (gth_curve_get_type ())
#define GTH_CURVE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_CURVE, GthCurve))
#define GTH_CURVE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_CURVE, GthCurveClass))
#define GTH_IS_CURVE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_CURVE))
#define GTH_IS_CURVE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_CURVE))
#define GTH_CURVE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_CURVE, GthCurveClass))

typedef struct {
	GObject    parent_instance;
	GthPoints  points;
} GthCurve;

typedef struct {
	GObjectClass parent_class;

	/*< virtual functions >*/

	void	(*setup)	(GthCurve *curve);
	double	(*eval)		(GthCurve *curve, double x);
} GthCurveClass;

GType		gth_curve_get_type		(void);
GthCurve *      gth_curve_new        		(GType		 curve_type,
						 GthPoints	*points);
GthCurve *      gth_curve_new_for_points        (GType		 curve_type,
						 int		 n_points,
						 ...);
void		gth_curve_set_points		(GthCurve	*curve,
						 GthPoints	*points);
GthPoints *	gth_curve_get_points		(GthCurve	*curve);
void		gth_curve_setup			(GthCurve	*self);
double		gth_curve_eval			(GthCurve	*curve,
						 double		 x);


/* -- GthSpline -- */


#define GTH_TYPE_SPLINE (gth_spline_get_type ())
#define GTH_SPLINE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_SPLINE, GthSpline))
#define GTH_SPLINE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_SPLINE, GthSplineClass))
#define GTH_IS_SPLINE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_SPLINE))
#define GTH_IS_SPLINE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_SPLINE))
#define GTH_SPLINE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_SPLINE, GthSplineClass))

typedef struct {
	GthCurve   parent_instance;
	double    *k;
	gboolean   is_singular;
} GthSpline;

typedef GthCurveClass GthSplineClass;

GType		gth_spline_get_type		(void);


/* -- GthCSpline -- */


#define GTH_TYPE_CSPLINE (gth_cspline_get_type ())
#define GTH_CSPLINE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_CSPLINE, GthCSpline))
#define GTH_CSPLINE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_CSPLINE, GthCSplineClass))
#define GTH_IS_CSPLINE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_CSPLINE))
#define GTH_IS_CSPLINE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_CSPLINE))
#define GTH_CSPLINE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_CSPLINE, GthCSplineClass))

typedef struct {
	GthCurve   parent_instance;
	double    *tangents;
} GthCSpline;

typedef GthCurveClass GthCSplineClass;

GType		gth_cspline_get_type		(void);


/* -- GthBezier -- */


#define GTH_TYPE_BEZIER (gth_bezier_get_type ())
#define GTH_BEZIER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_BEZIER, GthBezier))
#define GTH_BEZIER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_BEZIER, GthBezierClass))
#define GTH_IS_BEZIER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_BEZIER))
#define GTH_IS_BEZIER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_BEZIER))
#define GTH_BEZIER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_BEZIER, GthBezierClass))

typedef struct {
	GthCurve   parent_instance;
	double    *k;
	gboolean   linear;
} GthBezier;

typedef GthCurveClass GthBezierClass;

GType		gth_bezier_get_type		(void);


G_END_DECLS

#endif /* GTH_CURVE_H */
