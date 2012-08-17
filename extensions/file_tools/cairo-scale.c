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

#include <math.h>
#include <gthumb.h>
#include "cairo-scale.h"


/* -- _cairo_image_surface_scale_nearest -- */


typedef long gfixed;
#define GINT_TO_FIXED(x)         ((gfixed) ((x) << 16))
#define GDOUBLE_TO_FIXED(x)      ((gfixed) ((x) * (1 << 16) + 0.5))
#define GFIXED_TO_INT(x)         ((x) >> 16)
#define GFIXED_TO_DOUBLE(x)      (((double) (x)) / (1 << 16))
#define GFIXED_ROUND_TO_INT(x)   (((x) + (1 << (16-1))) >> 16)
#define GFIXED_0                 0L
#define GFIXED_1                 65536L
#define GFIXED_2                 131072L
#define gfixed_mul(x, y)         (((x) * (y)) >> 16)
#define gfixed_div(x, y)         (((x) << 16) / (y))


cairo_surface_t *
_cairo_image_surface_scale_nearest (cairo_surface_t *image,
				    int              new_width,
				    int              new_height)
{
	cairo_surface_t *scaled;
	int              src_width;
	int              src_height;
	guchar          *p_src;
	guchar          *p_dest;
	int              src_rowstride;
	int              dest_rowstride;
	gfixed           step_x, step_y;
	guchar          *p_src_row;
	guchar          *p_src_col;
	guchar          *p_dest_row;
	guchar          *p_dest_col;
	gfixed           max_row, max_col;
	gfixed           x_src, y_src;
	int              x, y;

	g_return_val_if_fail (cairo_image_surface_get_format (image) == CAIRO_FORMAT_ARGB32, NULL);

	scaled = cairo_surface_create_similar_image (image,
						     CAIRO_FORMAT_ARGB32,
						     new_width,
						     new_height);

	src_width = cairo_image_surface_get_width  (image);
	src_height = cairo_image_surface_get_height (image);
	p_src = cairo_image_surface_get_data (image);
	p_dest = cairo_image_surface_get_data (scaled);
	src_rowstride = cairo_image_surface_get_stride (image);
	dest_rowstride = cairo_image_surface_get_stride (scaled);

	cairo_surface_flush (scaled);

	step_x = GDOUBLE_TO_FIXED ((double) src_width / new_width);
	step_y = GDOUBLE_TO_FIXED ((double) src_height / new_height);

	p_dest_row = p_dest;
	p_src_row = p_src;
	max_row = GINT_TO_FIXED (src_height - 1);
	max_col= GINT_TO_FIXED (src_width - 1);
	/* pick the pixel in the middle to avoid the shift effect. */
	y_src = gfixed_div (step_y, GFIXED_2);
	for (y = 0; y < new_height; y++) {
		p_dest_col = p_dest_row;
		p_src_col = p_src_row;

		x_src = gfixed_div (step_x, GFIXED_2);
		for (x = 0; x < new_width; x++) {
			p_src_col = p_src_row + (GFIXED_TO_INT (MIN (x_src, max_col)) << 2); /* p_src_col = p_src_row + x_src * 4 */
			memcpy (p_dest_col, p_src_col, 4);

			p_dest_col += 4;
			x_src += step_x;
		}

		p_dest_row += dest_rowstride;
		y_src += step_y;
		p_src_row = p_src + (GFIXED_TO_INT (MIN (y_src, max_row)) * src_rowstride);
	}

	cairo_surface_mark_dirty (scaled);

	return scaled;
}


/* -- _cairo_image_surface_scale_filter -- */


#define EPSILON ((double) 1.0e-16)


typedef double (*weight_func_t) (double distance);


typedef enum {
	FILTER_POINT = 0,
	FILTER_BOX,
	FILTER_TRIANGLE,
	FILTER_QUADRATIC,
	N_FILTERS
} filter_type_t;


static double
box (double x)
{
	return 1.0;
}


static double
triangle (double x)
{
	return (x < 1.0) ? 1.0 - x : 0.0;
}


static double
quadratic (double x)
{
	/*
	 * 2rd order (quadratic) B-Spline approximation of Gaussian.
	 */
	if (x < 0.5)
		return 0.75 - x * x;
	if (x < 1.5)
		return 0.5 * (x - 1.5) * (x - 1.5);
	return 0.0;
}


static struct {
	weight_func_t weight_func;
	double        support;
}
const filters[N_FILTERS] = {
	{ box,		 .0 },
	{ box,		 .5 },
	{ triangle,	1.0 },
	{ quadratic,	1.5 }
};


/* -- resize_filter_t --  */


typedef struct {
	weight_func_t weight_func;
	double        support;
} resize_filter_t;


static resize_filter_t *
resize_filter_create (filter_type_t filter)
{
	resize_filter_t *resize_filter;

	resize_filter = g_slice_new (resize_filter_t);
	resize_filter->weight_func = filters[filter].weight_func;
	resize_filter->support = filters[filter].support;

	return resize_filter;
}


static double inline
resize_filter_get_support (resize_filter_t *resize_filter)
{
	return resize_filter->support;
}


static double inline
resize_filter_get_weight (resize_filter_t *resize_filter,
			  double           distance)
{
	return resize_filter->weight_func (fabs (distance));
}


static void
resize_filter_destroy (resize_filter_t *resize_filter)
{
	g_slice_free (resize_filter_t, resize_filter);
}


static inline double
reciprocal (double x)
{
	double sign = x < 0.0 ? -1.0 : 1.0;
	return (sign * x) >= EPSILON ? 1.0 / x : sign * (1.0 / EPSILON);
}


#define CLAMP_PIXEL(v) (((v) <= 0) ? 0  : ((v) >= 255) ? 255 : (v));


static void
horizontal_scale_transpose (cairo_surface_t *image,
			    cairo_surface_t *scaled,
			    double           scale_factor,
			    resize_filter_t *resize_filter)
{
	double  scale;
	double  support;
	int     y;
	guchar *p_src;
        guchar *p_dest;
        int     src_rowstride;
        int     dest_rowstride;
        double *weights;
        gfixed *fixed_weights;

        cairo_surface_flush (scaled);

	scale = MAX (1.0 / scale_factor + EPSILON, 1.0);
	support = scale * resize_filter_get_support (resize_filter);
	if (support < 0.5) {
		support = 0.5;
		scale = 1.0;
	}

	p_src = cairo_image_surface_get_data (image);
	p_dest = cairo_image_surface_get_data (scaled);
	src_rowstride = cairo_image_surface_get_stride (image);
	dest_rowstride = cairo_image_surface_get_stride (scaled);
	weights = g_new (double, 2.0 * support + 3.0);
	fixed_weights = g_new (gfixed, 2.0 * support + 3.0);

	scale = reciprocal (scale);
	for (y = 0; y < cairo_image_surface_get_height (scaled); y++) {
	        guchar *p_src_row;
	        guchar *p_dest_pixel;
		double  bisect;
		int     start;
		int     stop;
		double  density;
		int     n;
		int     x;
		int     i;

		bisect = (y + 0.5) / scale_factor + EPSILON;
		start = MAX (bisect - support + 0.5, 0.0);
		stop = MIN (bisect + support + 0.5, cairo_image_surface_get_width (image));

		density = 0.0;
		for (n = 0; n < stop - start; n++) {
			weights[n] = resize_filter_get_weight (resize_filter, scale * ((double) (start + n) - bisect + 0.5));
			density += weights[n];
		}

		density = reciprocal (density);
		for (i = 0; i < n; i++)
			fixed_weights[i] = GDOUBLE_TO_FIXED (weights[i] * density);

		p_src_row = p_src;
		p_dest_pixel = p_dest + (y * dest_rowstride);
		for (x = 0; x < cairo_image_surface_get_width (scaled); x++) {
			guchar *p_src_pixel;
			gfixed  r, g, b, a;
			gfixed  w;

			p_src_pixel = p_src_row + (start * 4);
			r = g = b = a = GFIXED_0;

			for (i = 0; i < n; i++) {
				w = fixed_weights[i];
				r += gfixed_mul (w, GINT_TO_FIXED (p_src_pixel[CAIRO_RED]));
				g += gfixed_mul (w, GINT_TO_FIXED (p_src_pixel[CAIRO_GREEN]));
				b += gfixed_mul (w, GINT_TO_FIXED (p_src_pixel[CAIRO_BLUE]));
				a += gfixed_mul (w, GINT_TO_FIXED (p_src_pixel[CAIRO_ALPHA]));

				p_src_pixel += 4;
			}

			r = GFIXED_TO_INT (r);
			g = GFIXED_TO_INT (g);
			b = GFIXED_TO_INT (b);
			a = GFIXED_TO_INT (a);

			p_dest_pixel[CAIRO_RED] = CLAMP_PIXEL (r);
			p_dest_pixel[CAIRO_GREEN] = CLAMP_PIXEL (g);
			p_dest_pixel[CAIRO_BLUE] = CLAMP_PIXEL (b);
			p_dest_pixel[CAIRO_ALPHA] = CLAMP_PIXEL (a);

			p_dest_pixel += 4;
			p_src_row += src_rowstride;
		}
	}

	cairo_surface_mark_dirty (scaled);

	g_free (weights);
	g_free (fixed_weights);
}


cairo_surface_t *
_cairo_image_surface_scale_filter (cairo_surface_t *image,
				   int              new_width,
				   int              new_height,
				   filter_type_t    filter)
{
	int              src_width;
	int              src_height;
	cairo_surface_t *scaled;
	resize_filter_t *resize_filter;
	double           x_factor;
	double           y_factor;
	cairo_surface_t *tmp;

	src_width = cairo_image_surface_get_width  (image);
	src_height = cairo_image_surface_get_height (image);

	if ((src_width == new_width) && (src_height == new_height))
		return _cairo_image_surface_copy (image);

	scaled = cairo_surface_create_similar_image (image,
						     CAIRO_FORMAT_ARGB32,
						     new_width,
						     new_height);
	if (scaled == NULL)
		return NULL;

	resize_filter = resize_filter_create (filter);
	x_factor = (double) new_width / src_width;
	y_factor = (double) new_height / src_height;
	tmp = cairo_surface_create_similar_image (image,
						  CAIRO_FORMAT_ARGB32,
						  src_height,
						  new_width);
	horizontal_scale_transpose (image, tmp, x_factor, resize_filter);
	horizontal_scale_transpose (tmp, scaled, y_factor, resize_filter);

	resize_filter_destroy (resize_filter);
	cairo_surface_destroy (tmp);

	return scaled;
}


cairo_surface_t *
_cairo_image_surface_scale (cairo_surface_t *image,
			    int              scaled_width,
			    int              scaled_height,
			    gboolean         high_quality)
{
#if 0

	GdkPixbuf       *p;
	GdkPixbuf       *scaled_p;
	cairo_surface_t *scaled;

	p = _gdk_pixbuf_new_from_cairo_surface (image);
	scaled_p = _gdk_pixbuf_scale_simple_safe (p, scaled_width, scaled_height, high_quality ? GDK_INTERP_BILINEAR : GDK_INTERP_NEAREST);
	scaled = _cairo_image_surface_create_from_pixbuf (scaled_p);

	g_object_unref (scaled_p);
	g_object_unref (p);

	return scaled;

#else

	if (high_quality)
		return _cairo_image_surface_scale_filter (image, scaled_width, scaled_height, FILTER_TRIANGLE);
	else
		return _cairo_image_surface_scale_nearest (image, scaled_width, scaled_height);

#endif
}
