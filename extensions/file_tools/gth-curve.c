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


G_DEFINE_INTERFACE (GthCurve, gth_curve, 0)


static void
gth_curve_default_init (GthCurveInterface *iface)
{
	/* void */
}


double
gth_curve_eval (GthCurve *self,
		double    x)
{
	return GTH_CURVE_GET_INTERFACE (self)->eval (self, x);
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


static void gth_spline_gth_curve_interface_init (GthCurveInterface *iface);


G_DEFINE_TYPE_WITH_CODE (GthSpline,
                         gth_spline,
                         G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GTH_TYPE_CURVE,
                        		 	gth_spline_gth_curve_interface_init))


static void
gth_spline_finalize (GObject *object)
{
	GthSpline *spline;

	spline = GTH_SPLINE (object);

	gth_points_dispose (&spline->points);
	g_free (spline->k);

	G_OBJECT_CLASS (gth_spline_parent_class)->finalize (object);
}


static void
gth_spline_class_init (GthSplineClass *class)
{
        GObjectClass *object_class;

        object_class = (GObjectClass*) class;
        object_class->finalize = gth_spline_finalize;
}


static double
gth_spline_eval (GthCurve *curve,
		 double    x)
{
	GthSpline *spline;
	GthPoint  *p;
	double    *k;
	int        i;

	spline = GTH_SPLINE (curve);
	p = spline->points.p;
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


void
gth_spline_gth_curve_interface_init (GthCurveInterface *iface)
{
	iface->eval = gth_spline_eval;
}


static void
gth_spline_init (GthSpline *spline)
{
	gth_points_init (&spline->points, 0);
	spline->k = NULL;
	spline->is_singular = FALSE;
}


static void
gth_spline_setup (GthSpline *spline)
{
	int        n;
	GthPoint  *p;
	Matrix    *m;
	double   **A;
	int        i;

	n = spline->points.n;
	p = spline->points.p;
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


GthCurve *
gth_spline_new (GthPoints *points)
{
	GthSpline *spline;
	int        i;

	spline = g_object_new (GTH_TYPE_SPLINE, NULL);
	gth_points_copy (points, &spline->points);
	spline->k = g_new (double, points->n + 1);
	for (i = 0; i < points->n + 1; i++)
		spline->k[i] = 1.0;
	gth_spline_setup (spline);

	return GTH_CURVE (spline);
}


/* -- GthCSpline (https://en.wikipedia.org/wiki/Cubic_Hermite_spline) -- */


static void gth_cspline_gth_curve_interface_init (GthCurveInterface *iface);


G_DEFINE_TYPE_WITH_CODE (GthCSpline,
                         gth_cspline,
                         G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GTH_TYPE_CURVE,
                        		 	gth_cspline_gth_curve_interface_init))


static void
gth_cspline_finalize (GObject *object)
{
	GthCSpline *spline;

	spline = GTH_CSPLINE (object);

	gth_points_dispose (&spline->points);
	g_free (spline->tangents);

	G_OBJECT_CLASS (gth_cspline_parent_class)->finalize (object);
}


static void
gth_cspline_class_init (GthCSplineClass *class)
{
        GObjectClass *object_class;

        object_class = (GObjectClass*) class;
        object_class->finalize = gth_cspline_finalize;
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
	GthPoint   *p;
	double     *t;
	int         k;
	double      d, z, z2, z3;
	double      y;

	spline = GTH_CSPLINE (curve);
	p = spline->points.p;
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


void
gth_cspline_gth_curve_interface_init (GthCurveInterface *iface)
{
	iface->eval = gth_cspline_eval;
}


static void
gth_cspline_init (GthCSpline *spline)
{
	gth_points_init (&spline->points, 0);
	spline->tangents = NULL;
}


static void
gth_cspline_setup (GthCSpline *spline)
{
	double   *t;
	GthPoint *p;
	int       k;

	/* calculate the tangents using the three-point difference */

	t = spline->tangents = g_new (double, spline->points.n);
	p = spline->points.p;
	for (k = 0; k < spline->points.n; k++) {
		t[k] = 0;
		if (k > 0)
			t[k] += (p[k].y - p[k-1].y) / (2 * (p[k].x - p[k-1].x));
		if (k < spline->points.n - 1)
			t[k] += (p[k+1].y - p[k].y) / (2 * (p[k+1].x - p[k].x));
	}
}


GthCurve *
gth_cspline_new (GthPoints *points)
{
	GthCSpline *spline;
	int         i;

	spline = g_object_new (GTH_TYPE_CSPLINE, NULL);
	gth_points_copy (points, &spline->points);
	gth_cspline_setup (spline);

	return GTH_CURVE (spline);
}
