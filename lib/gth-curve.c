#include <config.h>
#include <math.h>
#include "lib/gth-curve.h"

// GthCurve

G_DEFINE_TYPE (GthCurve, gth_curve, G_TYPE_OBJECT)

static void gth_curve_finalize (GObject *object) {
	GthCurve *self = GTH_CURVE (object);
	g_free (self->p);
	self->p = NULL;
	self->n = 0;
	G_OBJECT_CLASS (gth_curve_parent_class)->finalize (object);
}

static void gth_curve_setup_base (GthCurve *self) {
	// void
}

static double gth_curve_eval_base (GthCurve *self, double x) {
	return x;
}

static void gth_curve_class_init (GthCurveClass *class) {
	GObjectClass *object_class = (GObjectClass*) class;
	object_class->finalize = gth_curve_finalize;

	class->setup = gth_curve_setup_base;
	class->eval = gth_curve_eval_base;
}

static void gth_curve_init (GthCurve *self) {
	self->p = NULL;
	self->n = 0;
}

void gth_curve_setup (GthCurve *self) {
	return GTH_CURVE_GET_CLASS (self)->setup (self);
}

double gth_curve_eval (GthCurve *self, double x) {
	if (self->n > 0) {
		x = MAX (x, self->p[0].x);
		x = MIN (x, self->p[self->n - 1].x);
	}
	return GTH_CURVE_GET_CLASS (self)->eval (self, x);
}

void gth_curve_set_points (GthCurve *curve, GthPoint *points, int size) {
	if (curve->p != NULL) {
		g_free (curve->p);
	}
	curve->p = g_memdup2 (points, sizeof (GthPoint) * size);
	curve->n = size;
	gth_curve_setup (curve);
}


guchar * gth_curves_get_value_map (
	GthPoint *value_points, int value_size,
	GthPoint *red_points, int red_size,
	GthPoint *green_points, int green_size,
	GthPoint *blue_points, int blue_size)
{
	GthCurve *curve[4];
	curve[0] = (GthCurve *) gth_bezier_new (value_points, value_size);
	curve[1] = (GthCurve *) gth_bezier_new (red_points, red_size);
	curve[2] = (GthCurve *) gth_bezier_new (green_points, green_size);
	curve[3] = (GthCurve *) gth_bezier_new (blue_points, blue_size);

	guchar *value_map = g_new (guchar, 256 * 4);
	int idx = 0;
	for (int channel = 0; channel < 4; channel++) {
		for (int value = 0; value < 256; value++) {
			double u = gth_curve_eval (curve[channel], value);
			if (channel != 0) {
				u = value_map[(int) u];
			}
			value_map[idx] = (guchar) u;
			idx++;
		}
		g_object_unref (curve[channel]);
	}
	return value_map;
}


// Gauss-Jordan linear equation solver (for splines)

typedef struct {
	double **v;
	int r;
	int c;
} Matrix;

static Matrix * GJ_matrix_new (int r, int c) {
	Matrix *m = g_new (Matrix, 1);
	m->r = r;
	m->c = c;
	m->v = g_new (double*, r);
	for (int i = 0; i < r; i++) {
		m->v[i] = g_new (double, c);
		for (int j = 0; j < c; j++) {
			m->v[i][j] = 0.0;
		}
	}
	return m;
}

static void GJ_matrix_free (Matrix *m) {
	for (int i = 0; i < m->r; i++) {
		g_free (m->v[i]);
	}
	g_free (m->v);
	g_free (m);
}

static void GJ_swap_rows (double **m, int k, int l) {
	double *t = m[k];
	m[k] = m[l];
	m[l] = t;
}

static gboolean GJ_matrix_solve (Matrix *m, double *x) {
	double **A = m->v;
	int r = m->r;
	int k, i, j;

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
			//g_print ("matrix is singular!\n");
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

// GthSpline (http://en.wikipedia.org/wiki/Spline_interpolation#Algorithm_to_find_the_interpolating_cubic_spline)

G_DEFINE_TYPE (GthSpline, gth_spline, GTH_TYPE_CURVE)

static void gth_spline_finalize (GObject *object) {
	GthSpline *spline = GTH_SPLINE (object);
	g_free (spline->k);
	G_OBJECT_CLASS (gth_spline_parent_class)->finalize (object);
}

static void gth_spline_setup (GthCurve *curve) {
	GthPoint *p = curve->p;
	int n = curve->n;

	GthSpline *spline = GTH_SPLINE (curve);
	spline->k = g_new (double, n + 1);
	int i;
	for (i = 0; i < n + 1; i++) {
		spline->k[i] = 1.0;
	}

	Matrix *m = GJ_matrix_new (n+1, n+2);
	double **A = m->v;
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

static double gth_spline_eval (GthCurve *curve, double x) {
	GthSpline *spline = GTH_SPLINE (curve);
	if (spline->is_singular) {
		return x;
	}
	double *k = spline->k;
	GthPoint *p = curve->p;
	int n = curve->n;
	int i = 1;
	while (p[i].x < x) {
		i++;
	}
	double t = (x - p[i-1].x) / (p[i].x - p[i-1].x);
	double a = k[i-1] * (p[i].x - p[i-1].x) - (p[i].y - p[i-1].y);
	double b = - k[i] * (p[i].x - p[i-1].x) + (p[i].y - p[i-1].y);
	double y = round ( ((1-t) * p[i-1].y) + (t * p[i].y) + (t * (1-t) * (a * (1-t) + b * t)) );
	return CLAMP (y, 0, 255);
}


static void gth_spline_class_init (GthSplineClass *class) {
	GObjectClass *object_class = (GObjectClass*) class;
	object_class->finalize = gth_spline_finalize;

	GthCurveClass *curve_class = (GthCurveClass*) class;
	curve_class->setup = gth_spline_setup;
	curve_class->eval = gth_spline_eval;
}

static void gth_spline_init (GthSpline *spline) {
	spline->k = NULL;
	spline->is_singular = FALSE;
}

GthSpline * gth_spline_new (GthPoint *points, int size) {
	GthSpline *spline = g_object_new (GTH_TYPE_SPLINE, NULL);
	gth_curve_set_points (GTH_CURVE (spline), points, size);
	return spline;
}


// GthCSpline (https://en.wikipedia.org/wiki/Cubic_Hermite_spline)

G_DEFINE_TYPE (GthCSpline, gth_cspline, GTH_TYPE_CURVE)

static void gth_cspline_finalize (GObject *object) {
	GthCSpline *spline = GTH_CSPLINE (object);
	g_free (spline->tangents);
	G_OBJECT_CLASS (gth_cspline_parent_class)->finalize (object);
}

#if 0
static int number_sign (double x) {
	return (x < 0) ? -1 : (x > 0) ? 1 : 0;
}
#endif

static void gth_cspline_setup (GthCurve *curve) {
	GthPoint *p = curve->p;
	int n = curve->n;

	GthCSpline *spline = GTH_CSPLINE (curve);
	double *t = spline->tangents = g_new (double, n);
	int k;

#if 0
	// Finite difference

	for (k = 0; k < n; k++) {
		t[k] = 0;
		if (k > 0)
			t[k] += (p[k].y - p[k-1].y) / (2 * (p[k].x - p[k-1].x));
		if (k < n - 1)
			t[k] += (p[k+1].y - p[k].y) / (2 * (p[k+1].x - p[k].x));
	}
#endif

#if 1
	// Catmull–Rom spline

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

	// Monotonic spline (Fritsch–Carlson method) (https://en.wikipedia.org/wiki/Monotone_cubic_interpolation)

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

static inline double h00 (double x, double x2, double x3) {
	return 2*x3 - 3*x2 + 1;
}

static inline double h10 (double x, double x2, double x3) {
	return x3 - 2*x2 + x;
}

static inline double h01 (double x, double x2, double x3) {
	return -2*x3 + 3*x2;
}

static inline double h11 (double x, double x2, double x3) {
	return x3 - x2;
}

static double gth_cspline_eval (GthCurve *curve, double x) {
	GthCSpline *spline = GTH_CSPLINE (curve);
	double *t = spline->tangents;
	GthPoint *p = curve->p;
	int n = curve->n;
	int k = 1;
	while (p[k].x < x) {
		k++;
	}
	k--;
	double d = p[k+1].x - p[k].x;
	double z = (x - p[k].x) / d;
	double z2 = z * z;
	double z3 = z2 * z;
	double y = (h00(z, z2, z3) * p[k].y) + (h10(z, z2, z3) * d * t[k]) + (h01(z, z2, z3) * p[k+1].y) + (h11(z, z2, z3) * d * t[k+1]);
	return CLAMP (y, 0, 255);
}

static void gth_cspline_class_init (GthCSplineClass *class) {
	GObjectClass *object_class = (GObjectClass*) class;
	object_class->finalize = gth_cspline_finalize;

	GthCurveClass *curve_class = (GthCurveClass*) class;
	curve_class->setup = gth_cspline_setup;
	curve_class->eval = gth_cspline_eval;
}

static void gth_cspline_init (GthCSpline *spline) {
	spline->tangents = NULL;
}

GthCSpline * gth_cspline_new (GthPoint *points, int size) {
	GthCSpline *spline = g_object_new (GTH_TYPE_CSPLINE, NULL);
	gth_curve_set_points (GTH_CURVE (spline), points, size);
	return spline;
}

// GthBezier

G_DEFINE_TYPE (GthBezier, gth_bezier, GTH_TYPE_CURVE)

static void gth_bezier_finalize (GObject *object) {
	GthBezier *bezier = GTH_BEZIER (object);
	g_free (bezier->k);
	bezier->k = NULL;
	G_OBJECT_CLASS (gth_bezier_parent_class)->finalize (object);
}

// Calculate the control points coordinates (only the y) relative to
// the interval [p1, p2]
//  a: the point to the left of p1
//  b: the point to the right of p2
//  k: (output) the y values of the 4 points in the [p1, p2] interval
static void bezier_set_control_points (GthPoint *a, GthPoint *p1, GthPoint *p2, GthPoint *b, double *k) {
	double slope;
	double c1_y = 0;
	double c2_y = 0;

	//  The x coordinates of the control points is fixed to:
	//  c1_x = 2/3 * p1->x + 1/3 * p2->x;
	//  c2_x = 1/3 * p1->x + 2/3 * p2->x;
	//  For a more complete explanation see here: https://git.gnome.org/browse/gimp/tree/app/core/gimpcurve.c?h=gimp-2-8#n976

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
	else { // if ((a == NULL) && (b == NULL))
		c1_y = p1->y + (p2->y - p1->y) / 3.0;
		c2_y = p1->y + (p2->y - p1->y) * 2.0 / 3.0;
	}

	k[0] = p1->y;
	k[1] = c1_y;
	k[2] = c2_y;
	k[3] = p2->y;
}

static void gth_bezier_setup (GthCurve *curve) {
	GthBezier *bezier = GTH_BEZIER (curve);
	bezier->linear = (curve->n <= 1);
	if (bezier->linear) {
		return;
	}
	GthPoint *p = curve->p;
	int n = curve->n;
	bezier->k = g_new (double, 4 * (n - 1)); // 4 points for each interval
	for (int i = 0; i < n - 1; i++) {
		double *k = bezier->k + (i * 4);
		GthPoint *a = (i == 0) ? NULL: p + (i-1);
		GthPoint *b = (i == n-2) ? NULL : p + (i+2);
		bezier_set_control_points (a, p + i, p + (i+1), b, k);
	}
}

static double gth_bezier_eval (GthCurve *curve, double x) {
	GthBezier *bezier = GTH_BEZIER (curve);
	if (bezier->linear) {
		return x;
	}
	GthPoint *p = curve->p;
	int n = curve->n;
	int i = 1;
	while (p[i].x < x) {
		i++;
	}
	i--;
	double *k = bezier->k + (i * 4); // k: the 4 bezier points of the interval i
	double t = (x - p[i].x) / (p[i+1].x - p[i].x);
	double t2 = t*t;
	double s = 1.0 - t;
	double s2 = s * s;
	double y = round (s2*s*k[0] + 3*s2*t*k[1] + 3*s*t2*k[2] + t2*t*k[3]);
	return CLAMP (y, 0, 255);
}

static void gth_bezier_class_init (GthBezierClass *class) {
	GObjectClass *object_class = (GObjectClass*) class;
	object_class->finalize = gth_bezier_finalize;

	GthCurveClass *curve_class = (GthCurveClass*) class;
	curve_class->setup = gth_bezier_setup;
	curve_class->eval = gth_bezier_eval;
}

static void gth_bezier_init (GthBezier *bezier) {
	bezier->k = NULL;
	bezier->linear = TRUE;
}

GthBezier * gth_bezier_new (GthPoint *points, int size) {
	GthBezier *bezier = g_object_new (GTH_TYPE_BEZIER, NULL);
	gth_curve_set_points (GTH_CURVE (bezier), points, size);
	return bezier;
}
