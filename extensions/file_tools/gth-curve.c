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
#include "gth-curve.h"


/* -- GthCurve -- */


G_DEFINE_TYPE (GthCurve, gth_curve, G_TYPE_OBJECT)


GthCurve *
gth_curve_new (GType		 curve_type,
	       GthPoints	*points)
{
	GthCurve *curve;

	curve = g_object_new (curve_type, NULL);
	gth_curve_set_points (curve, points);

	return curve;
}


GthCurve *
gth_curve_new_for_points (GType		 curve_type,
			  int		 n_points,
			  ...)
{
	GthCurve  *curve;
	va_list    args;
	GthPoints  points;

	curve = g_object_new (curve_type, NULL);

	va_start (args, n_points);
	gth_points_init (&points, 0);
	gth_points_set_pointv (&points, args, n_points);
	gth_curve_set_points (curve, &points);
	va_end (args);

	return curve;
}


void
gth_curve_set_points (GthCurve	*curve,
		      GthPoints	*points)
{
	gth_points_copy (points, gth_curve_get_points (curve));
	gth_curve_setup (curve);
}


static void
gth_curve_finalize (GObject *object)
{
	GthCurve *spline;

	spline = GTH_CURVE (object);
	gth_points_dispose (&spline->points);

	G_OBJECT_CLASS (gth_curve_parent_class)->finalize (object);
}


static void
gth_curve_setup_base (GthCurve *self)
{
	/* void */
}


static double
gth_curve_eval_base (GthCurve *self,
		     double    x)
{
	return x;
}


static void
gth_curve_class_init (GthCurveClass *class)
{
        GObjectClass *object_class;

        object_class = (GObjectClass*) class;
        object_class->finalize = gth_curve_finalize;

        class->setup = gth_curve_setup_base;
        class->eval = gth_curve_eval_base;
}


static void
gth_curve_init (GthCurve *self)
{
	gth_points_init (&self->points, 0);
}


GthPoints *
gth_curve_get_points (GthCurve *curve)
{
	return &curve->points;
}


void
gth_curve_setup (GthCurve *self)
{
	return GTH_CURVE_GET_CLASS (self)->setup (self);
}


double
gth_curve_eval (GthCurve *self,
		double    x)
{
	if (self->points.n > 0) {
		x = MAX (x, self->points.p[0].x);
		x = MIN (x, self->points.p[self->points.n - 1].x);
	}
	return GTH_CURVE_GET_CLASS (self)->eval (self, x);
}


/* -- Gauss-Jordan linear equation solver (for splines) -- */


typedef struct {
	double **v;
	int      r;
	int      c;
} Matrix;


static Matrix *
GJ_matrix_new (int r, int c)
{
	Matrix *m;
	int     i, j;

	m = g_new (Matrix, 1);
	m->r = r;
	m->c = c;
	m->v = g_new (double *, r);
	for (i = 0; i < r; i++) {
		m->v[i] = g_new (double, c);
		for (j = 0; j < c; j++)
			m->v[i][j] = 0.0;
	}

	return m;
}


static void
GJ_matrix_free (Matrix *m)
{
	int i;
	for (i = 0; i < m->r; i++)
		g_free (m->v[i]);
	g_free (m->v);
	g_free (m);
}


static void
GJ_swap_rows (double **m, int k, int l)
{
	double *t = m[k];
	m[k] = m[l];
	m[l] = t;
}


static gboolean
GJ_matrix_solve (Matrix *m, double *x)
{
	double **A = m->v;
	int      r = m->r;
	int      k, i, j;

	for (k = 0; k < r; k++) { // column
		// pivot for column
		int    i_max = 0;
		double vali = 0;

		for (i = k; i < r; i++) {
			if ((i == k) || (A[i][k] > vali)) {
				i_max = i;
				vali = A[i][k];
			}
		}
		GJ_swap_rows (A, k, i_max);

		if (A[i_max][i] == 0) {
			g_print ("matrix is singular!\n");
			return TRUE;
		}

		// for all rows below pivot
		for (i = k + 1; i < r; i++) {
			for (j = k + 1; j < r + 1; j++)
				A[i][j] = A[i][j] - A[k][j] * (A[i][k] / A[k][k]);
			A[i][k] = 0.0;
		}
	}

	for (i = r - 1; i >= 0; i--) { // rows = columns
		double v = A[i][r] / A[i][i];

		x[i] = v;
		for (j = i - 1; j >= 0; j--) { // rows
			A[j][r] -= A[j][i] * v;
			A[j][i] = 0.0;
		}
	}

	return FALSE;
}


/* -- GthSpline (http://en.wikipedia.org/wiki/Spline_interpolation#Algorithm_to_find_the_interpolating_cubic_spline) -- */


G_DEFINE_TYPE (GthSpline, gth_spline, GTH_TYPE_CURVE)


static void
gth_spline_finalize (GObject *object)
{
	GthSpline *spline;

	spline = GTH_SPLINE (object);
	g_free (spline->k);

	G_OBJECT_CLASS (gth_spline_parent_class)->finalize (object);
}


static void
gth_spline_setup (GthCurve *curve)
{
	GthSpline  *spline;
	GthPoints  *points;
	int         n;
	GthPoint   *p;
	Matrix     *m;
	double    **A;
	int         i;

	spline = GTH_SPLINE (curve);
	points = gth_curve_get_points (GTH_CURVE (spline));
	n = points->n;
	p = points->p;

	spline->k = g_new (double, n + 1);
	for (i = 0; i < n + 1; i++)
		spline->k[i] = 1.0;

	m = GJ_matrix_new (n+1, n+2);
	A = m->v;
	for (i = 1; i < n; i++) {
		A[i][i-1] = 1.0 / (p[i].x - p[i-1].x);
		A[i][i  ] = 2.0 * (1.0 / (p[i].x - p[i-1].x) + 1.0 / (p[i+1].x - p[i].x));
		A[i][i+1] = 1.0 / (p[i+1].x - p[i].x);
		A[i][n+1] = 3.0 * ( (p[i].y - p[i-1].y) / ((p[i].x - p[i-1].x) * (p[i].x - p[i-1].x)) + (p[i+1].y - p[i].y) / ((p[i+1].x - p[i].x) * (p[i+1].x - p[i].x)) );
	}

        A[0][0  ] = 2.0 / (p[1].x - p[0].x);
        A[0][1  ] = 1.0 / (p[1].x - p[0].x);
        A[0][n+1] = 3.0 * (p[1].y - p[0].y) / ((p[1].x - p[0].x) * (p[1].x - p[0].x));

        A[n][n-1] = 1.0 / (p[n].x - p[n-1].x);
        A[n][n  ] = 2.0 / (p[n].x - p[n-1].x);
        A[n][n+1] = 3.0 * (p[n].y - p[n-1].y) / ((p[n].x - p[n-1].x) * (p[n].x - p[n-1].x));

        spline->is_singular = GJ_matrix_solve (m, spline->k);

	GJ_matrix_free (m);
}


static double
gth_spline_eval (GthCurve *curve,
		 double    x)
{
	GthSpline *spline;
	GthPoints *points;
	GthPoint  *p;
	double    *k;
	int        i;

	spline = GTH_SPLINE (curve);
	points = gth_curve_get_points (GTH_CURVE (spline));
	p = points->p;
	k = spline->k;

	if (spline->is_singular)
		return x;

	for (i = 1; p[i].x < x; i++)
		/* void */;
	double t = (x - p[i-1].x) / (p[i].x - p[i-1].x);
	double a = k[i-1] * (p[i].x - p[i-1].x) - (p[i].y - p[i-1].y);
	double b = - k[i] * (p[i].x - p[i-1].x) + (p[i].y - p[i-1].y);
	double y = round ( ((1-t) * p[i-1].y) + (t * p[i].y) + (t * (1-t) * (a * (1-t) + b * t)) );

	return CLAMP (y, 0, 255);
}


static void
gth_spline_class_init (GthSplineClass *class)
{
        GObjectClass  *object_class;
        GthCurveClass *curve_class;

        object_class = (GObjectClass*) class;
        object_class->finalize = gth_spline_finalize;

        curve_class = (GthCurveClass*) class;
        curve_class->setup = gth_spline_setup;
        curve_class->eval = gth_spline_eval;
}


static void
gth_spline_init (GthSpline *spline)
{
	spline->k = NULL;
	spline->is_singular = FALSE;
}


/* -- GthCSpline (https://en.wikipedia.org/wiki/Cubic_Hermite_spline) -- */


G_DEFINE_TYPE (GthCSpline, gth_cspline, GTH_TYPE_CURVE)


static void
gth_cspline_finalize (GObject *object)
{
	GthCSpline *spline;

	spline = GTH_CSPLINE (object);
	g_free (spline->tangents);

	G_OBJECT_CLASS (gth_cspline_parent_class)->finalize (object);
}


#if 0
static int number_sign (double x) {
	return (x < 0) ? -1 : (x > 0) ? 1 : 0;
}
#endif


static void
gth_cspline_setup (GthCurve *curve)
{
	GthCSpline *spline;
	GthPoints  *points;
	int         n;
	GthPoint   *p;
	double     *t;
	int         k;

	spline = GTH_CSPLINE (curve);
	points = gth_curve_get_points (GTH_CURVE (spline));
	n = points->n;
	p = points->p;

	t = spline->tangents = g_new (double, n);

#if 0
	/* Finite difference */

	for (k = 0; k < n; k++) {
		t[k] = 0;
		if (k > 0)
			t[k] += (p[k].y - p[k-1].y) / (2 * (p[k].x - p[k-1].x));
		if (k < n - 1)
			t[k] += (p[k+1].y - p[k].y) / (2 * (p[k+1].x - p[k].x));
	}
#endif

#if 1
	/* Catmull–Rom spline */

	for (k = 0; k < n; k++) {
		t[k] = 0;
		if (k == 0) {
			t[k] = (p[k+1].y - p[k].y) / (p[k+1].x - p[k].x);
		}
		else if (k == n - 1) {
			t[k] = (p[k].y - p[k-1].y) / (p[k].x - p[k-1].x);
		}
		else
			t[k] = (p[k+1].y - p[k-1].y) / (p[k+1].x - p[k-1].x);
	}
#endif

#if 0

	/* Monotonic spline (Fritsch–Carlson method) (https://en.wikipedia.org/wiki/Monotone_cubic_interpolation) */

	double *delta = g_new (double, n);
	double *alfa = g_new (double, n);
	double *beta = g_new (double, n);

	for (k = 0; k < n - 1; k++)
		delta[k] = (p[k+1].y - p[k].y) / (p[k+1].x - p[k].x);

	t[0] = delta[0];
	for (k = 1; k < n - 1; k++) {
		if (number_sign (delta[k-1]) == number_sign (delta[k]))
			t[k] = (delta[k-1] + delta[k]) / 2.0;
		else
			t[k] = 0;
	}
	t[n-1] = delta[n-2];

	for (k = 0; k < n - 1; k++) {
		if (delta[k] == 0) {
			t[k] = 0;
			t[k+1] = 0;
		}
	}

	for (k = 0; k < n - 1; k++) {
		if (delta[k] == 0)
			continue;
		alfa[k] = t[k] / delta[k];
		beta[k] = t[k+1] / delta[k];

		if (alfa[k] > 3) {
			t[k] = 3 * delta[k];
			alfa[k] = 3;
		}

		if (beta[k] > 3) {
			t[k+1] = 3 * delta[k];
			beta[k] = 3;
		}
	}

	for (k = 0; k < n - 1; k++) {
		if (delta[k] == 0)
			continue;

		if ((alfa[k] < 0) && (beta[k-1] < 0))
			t[k] = 0;

		double v = SQR (alfa[k]) + SQR (beta[k]);
		if (v > 9) {
			double tau = 3.0 / sqrt (v);
			t[k] = tau * alfa[k] * delta[k];
			t[k+1] = tau * beta[k] * delta[k];
		}
	}

	g_free (beta);
	g_free (alfa);
	g_free (delta);

#endif
}


static inline double h00 (double x, double x2, double x3)
{
	return 2*x3 - 3*x2 + 1;
}


static inline double h10 (double x, double x2, double x3)
{
	return x3 - 2*x2 + x;
}


static inline double h01 (double x, double x2, double x3)
{
	return -2*x3 + 3*x2;
}


static inline double h11 (double x, double x2, double x3)
{
	return x3 - x2;
}


static double
gth_cspline_eval (GthCurve *curve,
		  double    x)
{
	GthCSpline *spline;
	GthPoints  *points;
	GthPoint   *p;
	double     *t;
	int         k;
	double      d, z, z2, z3;
	double      y;

	spline = GTH_CSPLINE (curve);
	points = gth_curve_get_points (GTH_CURVE (spline));
	p = points->p;
	t = spline->tangents;

	for (k = 1; p[k].x < x; k++)
		/* void */;
	k--;

	d = p[k+1].x - p[k].x;
	z = (x - p[k].x) / d;
	z2 = z * z;
	z3 = z2 * z;
	y = (h00(z, z2, z3) * p[k].y) + (h10(z, z2, z3) * d * t[k]) + (h01(z, z2, z3) * p[k+1].y) + (h11(z, z2, z3) * d * t[k+1]);

	return CLAMP (y, 0, 255);
}


static void
gth_cspline_class_init (GthCSplineClass *class)
{
        GObjectClass  *object_class;
        GthCurveClass *curve_class;

        object_class = (GObjectClass*) class;
        object_class->finalize = gth_cspline_finalize;

        curve_class = (GthCurveClass*) class;
        curve_class->setup = gth_cspline_setup;
        curve_class->eval = gth_cspline_eval;
}


static void
gth_cspline_init (GthCSpline *spline)
{
	spline->tangents = NULL;
}


/* -- GthBezier -- */


G_DEFINE_TYPE (GthBezier, gth_bezier, GTH_TYPE_CURVE)


/* ---- */


static void
gth_bezier_finalize (GObject *object)
{
	GthBezier *spline;

	spline = GTH_BEZIER (object);
	g_free (spline->k);

	G_OBJECT_CLASS (gth_bezier_parent_class)->finalize (object);
}


/*
 * calculate the control points coordinates (only the y) relative to
 * the interval [p1, p2]
 *
 * a: the point to the left of p1
 * b: the point to the right of p2
 * k: (output) the y values of the 4 points in the [p1, p2] interval
 *
 **/

static void
bezier_set_control_points (GthPoint *a,
			   GthPoint *p1,
			   GthPoint *p2,
			   GthPoint *b,
			   double   *k)
{
	double slope;
	double c1_y = 0;
	double c2_y = 0;

	/*
	 * the x coordinates of the control points is fixed to:
	 *
	 * c1_x = 2/3 * p1->x + 1/3 * p2->x;
	 * c2_x = 1/3 * p1->x + 2/3 * p2->x;
	 *
	 * for a more complete explanation see here: https://git.gnome.org/browse/gimp/tree/app/core/gimpcurve.c?h=gimp-2-8#n976
	 *
	 **/

	if ((a != NULL) && (b != NULL)) {
		slope = (p2->y - a->y) / (p2->x - a->x);
		c1_y = p1->y + slope * (p2->x - p1->x) / 3.0;

		slope = (b->y - p1->y) / (b->x - p1->x);
		c2_y = p2->y - slope * (p2->x - p1->x) / 3.0;
	}
	else if ((a == NULL) && (b != NULL)) {
		slope = (b->y - p1->y) / (b->x - p1->x);
		c2_y = p2->y - slope * (p2->x - p1->x) / 3.0;
		c1_y = p1->y + (c2_y - p1->y) / 2.0;
	}
	else if ((a != NULL) && (b == NULL)) {
		slope = (p2->y - a->y) / (p2->x - a->x);
		c1_y = p1->y + slope * (p2->x - p1->x) / 3.0;
		c2_y = p2->y + (c1_y - p2->y) / 2.0;
	}
	else { /* if ((a == NULL) && (b == NULL)) */
		c1_y = p1->y + (p2->y - p1->y) / 3.0;
		c2_y = p1->y + (p2->y - p1->y) * 2.0 / 3.0;
	}

	k[0] = p1->y;
	k[1] = c1_y;
	k[2] = c2_y;
	k[3] = p2->y;
}


static void
gth_bezier_setup (GthCurve *curve)
{
	GthBezier  *spline;
	GthPoints  *points;
	int         n;
	GthPoint   *p;
	int         i;

	spline = GTH_BEZIER (curve);
	points = gth_curve_get_points (GTH_CURVE (spline));
	n = points->n;
	p = points->p;

	spline->linear = (n <= 1);
	if (spline->linear)
		return;

	spline->k = g_new (double, 4 * (n - 1)); /* 4 points for each interval */
	for (i = 0; i < n - 1; i++) {
		double   *k = spline->k + (i*4);
		GthPoint *a;
		GthPoint *b;

		a = (i == 0) ? NULL: p + (i-1);
		b = (i == n-2) ? NULL : p + (i+2);
		bezier_set_control_points (a, p + i, p + (i+1), b, k);
	}
}


static double
gth_bezier_eval (GthCurve *curve,
		 double    x)
{
	GthBezier *spline;
	GthPoints *points;
	GthPoint  *p;
	double    *k;
	int        i;

	spline = GTH_BEZIER (curve);

	if (spline->linear)
		return x;

	points = gth_curve_get_points (GTH_CURVE (spline));
	p = points->p;

	for (i = 1; p[i].x < x; i++)
		/* void */;
	i--;
	k = spline->k + (i*4); /* k: the 4 bezier points of the interval i */

	double t = (x - p[i].x) / (p[i+1].x - p[i].x);
	double t2 = t*t;
	double s = 1.0 - t;
	double s2 = s * s;
	double y = round (s2*s*k[0] + 3*s2*t*k[1] + 3*s*t2*k[2] + t2*t*k[3]);

	return CLAMP (y, 0, 255);
}


static void
gth_bezier_class_init (GthBezierClass *class)
{
        GObjectClass  *object_class;
        GthCurveClass *curve_class;

        object_class = (GObjectClass*) class;
        object_class->finalize = gth_bezier_finalize;

        curve_class = (GthCurveClass*) class;
        curve_class->setup = gth_bezier_setup;
        curve_class->eval = gth_bezier_eval;
}


static void
gth_bezier_init (GthBezier *spline)
{
	spline->k = NULL;
	spline->linear = TRUE;
}
