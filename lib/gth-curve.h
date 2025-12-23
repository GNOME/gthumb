#ifndef GTH_CURVE_H
#define GTH_CURVE_H

#include <glib.h>
#include <glib-object.h>
#include "lib/gth-point.h"

G_BEGIN_DECLS

// GthCurve

#define GTH_TYPE_CURVE (gth_curve_get_type ())
#define GTH_CURVE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_CURVE, GthCurve))
#define GTH_CURVE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_CURVE, GthCurveClass))
#define GTH_IS_CURVE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_CURVE))
#define GTH_IS_CURVE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_CURVE))
#define GTH_CURVE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_CURVE, GthCurveClass))

typedef struct {
	GObject parent_instance;
	GthPoint *p;
	int n;
} GthCurve;

typedef struct {
	GObjectClass parent_class;
	void (*setup) (GthCurve *curve);
	double (*eval) (GthCurve *curve, double x);
} GthCurveClass;

GType gth_curve_get_type (void);
void gth_curve_setup (GthCurve *self);
double gth_curve_eval (GthCurve *curve, double x);
void gth_curve_set_points (GthCurve *curve, GthPoint *points, int size);
guchar * gth_curves_get_value_map (
	GthPoint *value_points, int value_size,
	GthPoint *red_points, int red_size,
	GthPoint *green_points, int green_size,
	GthPoint *blue_points, int blue_size);

// GthSpline

#define GTH_TYPE_SPLINE (gth_spline_get_type ())
#define GTH_SPLINE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_SPLINE, GthSpline))
#define GTH_SPLINE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_SPLINE, GthSplineClass))
#define GTH_IS_SPLINE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_SPLINE))
#define GTH_IS_SPLINE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_SPLINE))
#define GTH_SPLINE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_SPLINE, GthSplineClass))

typedef struct {
	GthCurve parent_instance;
	double *k;
	gboolean is_singular;
} GthSpline;

typedef GthCurveClass GthSplineClass;

GType gth_spline_get_type (void);
GthSpline * gth_spline_new (GthPoint *points, int size);

// GthCSpline

#define GTH_TYPE_CSPLINE (gth_cspline_get_type ())
#define GTH_CSPLINE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_CSPLINE, GthCSpline))
#define GTH_CSPLINE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_CSPLINE, GthCSplineClass))
#define GTH_IS_CSPLINE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_CSPLINE))
#define GTH_IS_CSPLINE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_CSPLINE))
#define GTH_CSPLINE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_CSPLINE, GthCSplineClass))

typedef struct {
	GthCurve parent_instance;
	double *tangents;
} GthCSpline;

typedef GthCurveClass GthCSplineClass;

GType gth_cspline_get_type (void);
GthCSpline * gth_cspline_new (GthPoint *points, int size);

// GthBezier

#define GTH_TYPE_BEZIER (gth_bezier_get_type ())
#define GTH_BEZIER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_BEZIER, GthBezier))
#define GTH_BEZIER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_BEZIER, GthBezierClass))
#define GTH_IS_BEZIER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_BEZIER))
#define GTH_IS_BEZIER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_BEZIER))
#define GTH_BEZIER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_BEZIER, GthBezierClass))

typedef struct {
	GthCurve parent_instance;
	double *k;
	gboolean linear;
} GthBezier;

typedef GthCurveClass GthBezierClass;

GType gth_bezier_get_type (void);
GthBezier * gth_bezier_new (GthPoint *points, int size);

G_END_DECLS

#endif /* GTH_CURVE_H */
