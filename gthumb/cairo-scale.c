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

#include <config.h>
#include <string.h>
#include <math.h>
#include <cairo.h>
#include "cairo-utils.h"
#include "cairo-scale.h"
#include "gfixed.h"
#include "glib-utils.h"


#define CLAMP_PIXEL(v) (((v) <= 0) ? 0  : ((v) >= 255) ? 255 : (v));
#define EPSILON ((ScaleReal) 1.0e-16)


typedef double ScaleReal;
typedef ScaleReal (*weight_func_t) (ScaleReal distance);


#ifdef HAVE_VECTOR_OPERATIONS


typedef ScaleReal v4r __attribute__ ((vector_size(sizeof(ScaleReal)*4)));
typedef union {
	v4r       v;
	ScaleReal r[4];
} r4vector;


#endif /* HAVE_VECTOR_OPERATIONS */


/* -- _cairo_image_surface_scale_nearest -- */


cairo_surface_t *
_cairo_image_surface_scale_nearest (cairo_surface_t *image,
				    int              new_width,
				    int              new_height)
{
#if 0
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

	scaled = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
					     new_width,
					     new_height);

	src_width = cairo_image_surface_get_width  (image);
	src_height = cairo_image_surface_get_height (image);
	p_src = _cairo_image_surface_flush_and_get_data (image);
	p_dest = _cairo_image_surface_flush_and_get_data (scaled);
	src_rowstride = cairo_image_surface_get_stride (image);
	dest_rowstride = cairo_image_surface_get_stride (scaled);

	cairo_surface_flush (scaled);

	step_x = GDOUBLE_TO_FIXED ((double) src_width / new_width);
	step_y = GDOUBLE_TO_FIXED ((double) src_height / new_height);

	p_dest_row = p_dest;
	p_src_row = p_src;
	max_row = GINT_TO_FIXED (src_height - 1);
	max_col = GINT_TO_FIXED (src_width - 1);
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
#else

	cairo_surface_t *output;
	cairo_t         *cr;

	output = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
					     new_width,
					     new_height);
	cr = cairo_create (output);
	cairo_scale (cr,
		     (double) new_width / cairo_image_surface_get_width (image),
		     (double) new_height / cairo_image_surface_get_height (image));
	cairo_set_source_surface (cr, image, 0.0, 0.0);
	cairo_pattern_set_filter (cairo_get_source (cr), CAIRO_FILTER_NEAREST);
	cairo_rectangle (cr, 0.0, 0.0, cairo_image_surface_get_width (image), cairo_image_surface_get_height (image));
	cairo_fill (cr);
	cairo_surface_flush (output);

	cairo_destroy (cr);

	return output;

#endif
}


/* -- _cairo_image_surface_scale_filter --
 *
 * Based on code from ImageMagick/magick/resize.c
 *
 * */


inline static ScaleReal
box (ScaleReal x)
{
	return 1.0;
}


inline static ScaleReal
triangle (ScaleReal x)
{
	return (x < 1.0) ? 1.0 - x : 0.0;
}


static ScaleReal Cubic_P0, Cubic_P2, Cubic_P3;
static ScaleReal Cubic_Q0, Cubic_Q1, Cubic_Q2, Cubic_Q3;
static gsize coefficients_initialization = 0;


static void
initialize_coefficients (ScaleReal B,
			 ScaleReal C)
{
	Cubic_P0 = (6.0 - 2.0 * B) / 6.0;
	/*Cubic_P1 = 0.0;*/
	Cubic_P2 = (-18.0 + 12.0 * B + 6.0 * C) / 6.0;
	Cubic_P3 = ( 12.0 - 9.0 * B - 6.0 * C) / 6.0;
	Cubic_Q0 = (8.0 * B + 24.0 * C) / 6.0;
	Cubic_Q1 = (-12.0 * B - 48.0 * C) / 6.0;
	Cubic_Q2 = (6.0 * B + 30.0 * C) / 6.0;
	Cubic_Q3 = (-1.0 * B - 6.0 * C) / 6.0;
}


inline static ScaleReal
cubic (ScaleReal x)
{
	if (x < 1.0)
		return Cubic_P0 + x * (x * (Cubic_P2 + x * Cubic_P3));
	if (x < 2.0)
		return Cubic_Q0 + x * (Cubic_Q1 + x * (Cubic_Q2 + x * Cubic_Q3));
	return 0.0;
}


inline static ScaleReal
sinc_fast (ScaleReal x)
{
	if (x > 4.0) {
		const ScaleReal alpha = G_PI * x;
		return sin ((double) alpha) / alpha;
	}

	{
		/*
		 * The approximations only depend on x^2 (sinc is an even function).
		 */

		const ScaleReal xx = x*x;

		/*
		 * Maximum absolute relative error 1.2e-12 < 1/2^39.
		 */

		const ScaleReal c0 = 0.173611111110910715186413700076827593074e-2L;
		const ScaleReal c1 = -0.289105544717893415815859968653611245425e-3L;
		const ScaleReal c2 = 0.206952161241815727624413291940849294025e-4L;
		const ScaleReal c3 = -0.834446180169727178193268528095341741698e-6L;
		const ScaleReal c4 = 0.207010104171026718629622453275917944941e-7L;
		const ScaleReal c5 = -0.319724784938507108101517564300855542655e-9L;
		const ScaleReal c6 = 0.288101675249103266147006509214934493930e-11L;
		const ScaleReal c7 = -0.118218971804934245819960233886876537953e-13L;
		const ScaleReal p = c0+xx*(c1+xx*(c2+xx*(c3+xx*(c4+xx*(c5+xx*(c6+xx*c7))))));
		const ScaleReal d0 = 1.0L;
		const ScaleReal d1 = 0.547981619622284827495856984100563583948e-1L;
		const ScaleReal d2 = 0.134226268835357312626304688047086921806e-2L;
		const ScaleReal d3 = 0.178994697503371051002463656833597608689e-4L;
		const ScaleReal d4 = 0.114633394140438168641246022557689759090e-6L;
		const ScaleReal q = d0+xx*(d1+xx*(d2+xx*(d3+xx*d4)));

		return ((xx-1.0)*(xx-4.0)*(xx-9.0)*(xx-16.0)/q*p);
	}
}


inline static ScaleReal
mitchell_netravali (ScaleReal x)
{
	ScaleReal xx;

	if (x >= 2.0)
		return 0.0;

	xx = x * x;
	if ((x >= 0.0) && (x < 1.0))
		x  = (21 * xx * x) - (36 * xx) + 16;
	else /* (x >= 1.0) && (x < 2.0) */
		x  = (-7 * xx * x) + (36 * xx) - (60 * x) + 32;

	return x / 18.0;
}


static struct {
	weight_func_t weight_func;
	ScaleReal     support;
}
const filters[N_SCALE_FILTERS] = {
	{ box,			.0 },
	{ box,			.5 },
	{ triangle,		1.0 },
	{ cubic,		2.0 },
	{ sinc_fast,		2.0 },
	{ sinc_fast,		3.0 },
	{ mitchell_netravali,	2.0 }
};


/* -- resize_filter_t --  */


typedef struct {
	weight_func_t  weight_func;
	ScaleReal      support;
	GthAsyncTask  *task;
	gulong         total_lines;
	gulong         processed_lines;
	gboolean       cancelled;
} resize_filter_t;


static void
resize_filter_set_type (resize_filter_t *resize_filter,
			scale_filter_t   filter_type)
{
	resize_filter->weight_func = filters[filter_type].weight_func;
	resize_filter->support = filters[filter_type].support;
}


static resize_filter_t *
resize_filter_create (GthAsyncTask *task)
{
	resize_filter_t *resize_filter;

	resize_filter = g_new (resize_filter_t, 1);
	resize_filter->task = task;
	resize_filter->total_lines = 0;
	resize_filter->processed_lines = 0;
	resize_filter->cancelled = FALSE;
	resize_filter_set_type (resize_filter, SCALE_FILTER_TRIANGLE);

	return resize_filter;
}


inline static ScaleReal
resize_filter_get_support (resize_filter_t *resize_filter)
{
	return resize_filter->support;
}


inline static ScaleReal
resize_filter_get_weight (resize_filter_t *resize_filter,
			  ScaleReal        distance)
{
	ScaleReal window = 1.0;

	if (resize_filter->weight_func == sinc_fast) {
		ScaleReal x = fabs (distance);

		if (x == 0)
			window = 1.0;
		else if ((x > 0) && (x < resize_filter->support))
			window = resize_filter->weight_func (x / resize_filter->support);
		else
			window = 0.0;
	}

	return window * resize_filter->weight_func (fabs (distance));
}


static void
resize_filter_destroy (resize_filter_t *resize_filter)
{
	g_free (resize_filter);
}


static inline ScaleReal
reciprocal (ScaleReal x)
{
	ScaleReal sign = x < 0.0 ? -1.0 : 1.0;
	return (sign * x) >= EPSILON ? 1.0 / x : sign * (1.0 / EPSILON);
}


static void
horizontal_scale_transpose (cairo_surface_t *image,
			    cairo_surface_t *scaled,
			    ScaleReal        scale_factor,
			    resize_filter_t *resize_filter)
{
	ScaleReal  scale;
	ScaleReal  support;
	int        y;
	int        image_width;
	int        scaled_width;
	int        scaled_height;
	guchar    *p_src;
        guchar    *p_dest;
        int        src_rowstride;
        int        dest_rowstride;
        ScaleReal *weights;

	if (resize_filter->cancelled)
		return;

	scale = MAX ((ScaleReal) 1.0 / scale_factor + EPSILON, 1.0);
	support = scale * resize_filter_get_support (resize_filter);
	if (support < 0.5) {
		support = 0.5;
		scale = 1.0;
	}

	image_width = cairo_image_surface_get_width (image);
	scaled_width = cairo_image_surface_get_width (scaled);
	scaled_height = cairo_image_surface_get_height (scaled);
	p_src = _cairo_image_surface_flush_and_get_data (image);
	p_dest = _cairo_image_surface_flush_and_get_data (scaled);
	src_rowstride = cairo_image_surface_get_stride (image);
	dest_rowstride = cairo_image_surface_get_stride (scaled);
	weights = g_new (ScaleReal, 2.0 * support + 3.0);

	scale = reciprocal (scale);
	for (y = 0; y < scaled_height; y++) {
	        guchar    *p_src_row;
	        guchar    *p_dest_pixel;
		ScaleReal  bisect;
		int        start;
		int        stop;
		ScaleReal  density;
		int        n;
		int        x;
		int        i;
#ifdef HAVE_VECTOR_OPERATIONS
		r4vector   v_pixel, v_rgba;
#endif /* HAVE_VECTOR_OPERATIONS */

		if (resize_filter->task != NULL) {
			double progress;

			gth_async_task_get_data (resize_filter->task, NULL, &resize_filter->cancelled, NULL);
			if (resize_filter->cancelled)
				goto out;

			progress = (double) resize_filter->processed_lines++ / resize_filter->total_lines;
			gth_async_task_set_data (resize_filter->task, NULL, NULL, &progress);
		}

		bisect = ((ScaleReal) y + 0.5) / scale_factor + EPSILON;
		start = MAX (bisect - support + 0.5, 0);
		stop = MIN (bisect + support + 0.5, (ScaleReal) image_width);

		density = 0.0;
		for (n = 0; n < stop - start; n++) {
			weights[n] = resize_filter_get_weight (resize_filter, scale * ((ScaleReal) (start + n) - bisect + 0.5));
			density += weights[n];
		}

		/*
		g_assert (n == stop - start);
		g_assert (stop - start <= (2.0 * support) + 3);
		*/

		if ((density != 0.0) && (density != 1.0)) {
			density = reciprocal (density);
			for (i = 0; i < n; i++)
				weights[i] *= density;
		}

		p_src_row = p_src + (start * 4);
		p_dest_pixel = p_dest;
		for (x = 0; x < scaled_width; x++) {
			guchar *p_src_pixel;

			p_src_pixel = p_src_row;

#ifdef HAVE_VECTOR_OPERATIONS

			v_rgba.v = (v4r) { 0.0, 0.0, 0.0, 0.0 };
			for (i = 0; i < n; i++) {
				v_pixel.v = (v4r) { p_src_pixel[0], p_src_pixel[1], p_src_pixel[2], p_src_pixel[3] };
				v_rgba.v = v_rgba.v + (v_pixel.v * weights[i]);

				p_src_pixel += 4;
			}
			v_rgba.v = v_rgba.v + 0.5;

			p_dest_pixel[0] = CLAMP_PIXEL (v_rgba.r[0]);
			p_dest_pixel[1] = CLAMP_PIXEL (v_rgba.r[1]);
			p_dest_pixel[2] = CLAMP_PIXEL (v_rgba.r[2]);
			p_dest_pixel[3] = CLAMP_PIXEL (v_rgba.r[3]);

#else /* ! HAVE_VECTOR_OPERATIONS */

			ScaleReal r, g, b, a, w;

			r = g = b = a = 0.0;
			for (i = 0; i < n; i++) {
				w = weights[i];

				r += w * p_src_pixel[CAIRO_RED];
				g += w * p_src_pixel[CAIRO_GREEN];
				b += w * p_src_pixel[CAIRO_BLUE];
				a += w * p_src_pixel[CAIRO_ALPHA];

				p_src_pixel += 4;
			}

			p_dest_pixel[CAIRO_RED] = CLAMP_PIXEL (r + 0.5);
			p_dest_pixel[CAIRO_GREEN] = CLAMP_PIXEL (g + 0.5);
			p_dest_pixel[CAIRO_BLUE] = CLAMP_PIXEL (b + 0.5);
			p_dest_pixel[CAIRO_ALPHA] = CLAMP_PIXEL (a + 0.5);

#endif /* HAVE_VECTOR_OPERATIONS */

			p_dest_pixel += 4;
			p_src_row += src_rowstride;
		}

		p_dest += dest_rowstride;
	}

	out:

	cairo_surface_mark_dirty (scaled);

	g_free (weights);
}


cairo_surface_t *
_cairo_image_surface_scale (cairo_surface_t  *image,
			    int               scaled_width,
			    int               scaled_height,
			    scale_filter_t    filter,
			    GthAsyncTask     *task)
{
	int                       src_width;
	int                       src_height;
	cairo_surface_t          *scaled;
	cairo_surface_metadata_t *metadata;
	resize_filter_t          *resize_filter;
	ScaleReal                 x_factor;
	ScaleReal                 y_factor;
	cairo_surface_t          *tmp;

	src_width = cairo_image_surface_get_width (image);
	src_height = cairo_image_surface_get_height (image);

	if ((src_width == scaled_width) && (src_height == scaled_height))
		return _cairo_image_surface_copy (image);

	scaled = _cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
					      scaled_width,
					      scaled_height);
	_cairo_image_surface_copy_metadata (image, scaled);
	metadata = _cairo_image_surface_get_metadata (scaled);
	if (metadata->original_width <= 0) {
		metadata->original_width = src_width;
		metadata->original_height = src_height;
	}

	if (scaled == NULL)
		return NULL;

	if (g_once_init_enter (&coefficients_initialization)) {
		initialize_coefficients (1.0, 0.0);
		g_once_init_leave (&coefficients_initialization, 1);
	}

	resize_filter = resize_filter_create (task);
	resize_filter_set_type (resize_filter, filter);
	resize_filter->total_lines = scaled_width + scaled_height;
	resize_filter->processed_lines = 0;

	x_factor = (ScaleReal) scaled_width / src_width;
	y_factor = (ScaleReal) scaled_height / src_height;
	tmp = _cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
					   src_height,
					   scaled_width);

	horizontal_scale_transpose (image, tmp, x_factor, resize_filter);
	horizontal_scale_transpose (tmp, scaled, y_factor, resize_filter);

	resize_filter_destroy (resize_filter);
	cairo_surface_destroy (tmp);

	return scaled;
}


cairo_surface_t *
_cairo_image_surface_scale_squared (cairo_surface_t *image,
				    int              size,
				    scale_filter_t   quality,
				    GthAsyncTask    *task)
{
	int              width, height;
	int              scaled_width, scaled_height;
	cairo_surface_t *scaled;
	cairo_surface_t *squared;

	width = cairo_image_surface_get_width (image);
	height = cairo_image_surface_get_height (image);

	if ((width < size) && (height < size))
		return _cairo_image_surface_copy (image);

	if (width > height) {
		scaled_height = size;
		scaled_width = (int) (((double) width / height) * scaled_height);
	}
	else {
		scaled_width = size;
		scaled_height = (int) (((double) height / width) * scaled_width);
	}

	if ((scaled_width != width) || (scaled_height != height))
		scaled = _cairo_image_surface_scale (image, scaled_width, scaled_height, quality, task);
	else
		scaled = cairo_surface_reference (image);

	if ((scaled_width == size) && (scaled_height == size))
		return scaled;

	squared = _cairo_image_surface_copy_subsurface (scaled,
							(scaled_width - size) / 2,
							(scaled_height - size) / 2,
							size,
							size);
	cairo_surface_destroy (scaled);

	return squared;
}


/* -- _cairo_image_surface_scale_bilinear -- */


#if 0


#define BILINEAR_INTERPOLATE(v, v00, v01, v10, v11, fx, fy) \
	tmp = (1.0 - (fy)) * \
	      ((1.0 - (fx)) * (v00) + (fx) * (v01)) \
              + \
              (fy) * \
              ((1.0 - (fx)) * (v10) + (fx) * (v11)); \
	v = CLAMP (tmp, 0, 255);


cairo_surface_t *
_cairo_image_surface_scale_bilinear_2x2 (cairo_surface_t *image,
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
	ScaleReal           step_x, step_y;
	guchar          *p_dest_row;
	guchar          *p_src_row;
	guchar          *p_src_col;
	guchar          *p_dest_col;
	ScaleReal           x_src, y_src;
	int              x, y;
	guchar           r00, r01, r10, r11;
	guchar           g00, g01, g10, g11;
	guchar           b00, b01, b10, b11;
	guchar           a00, a01, a10, a11;
	guchar           r, g, b, a;
	guint32          pixel;
	ScaleReal           tmp;
	ScaleReal           x_fract, y_fract;
	int              col, row;

	scaled = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
					     new_width,
					     new_height);

	src_width = cairo_image_surface_get_width  (image);
	src_height = cairo_image_surface_get_height (image);
	p_src = _cairo_image_surface_flush_and_get_data (image);
	p_dest = _cairo_image_surface_flush_and_get_data (scaled);
	src_rowstride = cairo_image_surface_get_stride (image);
	dest_rowstride = cairo_image_surface_get_stride (scaled);

	cairo_surface_flush (scaled);

	step_x = (ScaleReal) src_width / new_width;
	step_y = (ScaleReal) src_height / new_height;
	p_dest_row = p_dest;
	p_src_row = p_src;
	y_src = 0;

	for (y = 0; y < new_height; y++) {
		row = floor (y_src);
		y_fract = y_src - row;
		p_src_row = p_src + (row * src_rowstride);

		p_dest_col = p_dest_row;
		p_src_col = p_src_row;
		x_src = 0;
		for (x = 0; x < new_width; x++) {
			col = floor (x_src);
			x_fract = x_src - col;
			p_src_col = p_src_row + (col * 4);

			/* x00 */

			CAIRO_COPY_RGBA (p_src_col, r00, g00, b00, a00);

			/* x01 */

			if (col + 1 < src_width - 1) {
				p_src_col += 4;
				CAIRO_COPY_RGBA (p_src_col, r01, g01, b01, a01);
			}
			else {
				r01 = r00;
				g01 = g00;
				b01 = b00;
				a01 = a00;
			}

			/* x10 */

			if (row + 1 < src_height - 1) {
				p_src_col = p_src_row + src_rowstride + (col * 4);
				CAIRO_COPY_RGBA (p_src_col, r10, g10, b10, a10);
			}
			else {
				r10 = r00;
				g10 = g00;
				b10 = b00;
				a10 = a00;
			}

			/* x11 */

			if ((row + 1 < src_height - 1) && (col + 1 < src_width - 1)) {
				p_src_col += 4;
				CAIRO_COPY_RGBA (p_src_col, r11, g11, b11, a11);
			}
			else {
				r11 = r00;
				g11 = g00;
				b11 = b00;
				a11 = a00;
			}

			BILINEAR_INTERPOLATE (r, r00, r01, r10, r11, x_fract, y_fract);
			BILINEAR_INTERPOLATE (g, g00, g01, g10, g11, x_fract, y_fract);
			BILINEAR_INTERPOLATE (b, b00, b01, b10, b11, x_fract, y_fract);
			BILINEAR_INTERPOLATE (a, a00, a01, a10, a11, x_fract, y_fract);
			pixel = CAIRO_RGBA_TO_UINT32 (r, g, b, a);
			memcpy (p_dest_col, &pixel, 4);

			p_dest_col += 4;
			x_src += step_x;
		}

		p_dest_row += dest_rowstride;
		y_src += step_y;
	}

	cairo_surface_mark_dirty (scaled);

	return scaled;
}


static void
_cairo_surface_reduce_row (guchar *dest_data,
			   guchar *src_row0,
			   guchar *src_row1,
			   guchar *src_row2,
			   int     src_width)
{
	int     x, b;
	ScaleReal  sum;
	int     col0, col1, col2;
	guchar  c[4];
	guint32 pixel;

	/*  Pre calculated gausian matrix
	 *  Standard deviation = 0.71

		12 32 12
		32 86 32
		12 32 12

		Matrix sum is = 262
		Normalize by dividing with 273

	*/

	for (x = 0; x < src_width - (src_width % 2); x += 2) {
		for (b = 0; b < 4; b++) {

			col0 = MAX (x - 1, 0) * 4 + b;
			col1 = x * 4 + b;
			col2 = MIN (x + 1, src_width - 1) * 4 + b;

			sum = 0.0;

			sum += src_row0[col0] * 12;
			sum += src_row0[col1] * 32;
			sum += src_row0[col2] * 12;

			sum += src_row1[col0] * 32;
			sum += src_row1[col1] * 86;
			sum += src_row1[col2] * 32;

			sum += src_row2[col0] * 12;
			sum += src_row2[col1] * 32;
			sum += src_row2[col2] * 12;

			sum /= 262.0;

			c[b] = CLAMP (sum, 0, 255);
		}

		pixel = CAIRO_RGBA_TO_UINT32 (c[CAIRO_RED], c[CAIRO_GREEN], c[CAIRO_BLUE], c[CAIRO_ALPHA]);
		memcpy (dest_data, &pixel, 4);

		dest_data += 4;
	}
}


static cairo_surface_t *
_cairo_surface_reduce_by_half (cairo_surface_t *src)
{
	int              src_width, src_height;
	cairo_surface_t *dest;
	int              src_rowstride, dest_rowstride;
	guchar          *row0, *row1, *row2;
	guchar          *src_data, *dest_data;
	int              y;

	src_width = cairo_image_surface_get_width (src);
	src_height = cairo_image_surface_get_height (src);
	dest = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
					   src_width / 2,
					   src_height / 2);

	dest_rowstride = cairo_image_surface_get_stride (dest);
	dest_data = _cairo_image_surface_flush_and_get_data (dest);

	src_rowstride = cairo_image_surface_get_stride (src);
	src_data = _cairo_image_surface_flush_and_get_data (src);

	for (y = 0; y < src_height - (src_height % 2); y += 2) {
		row0 = src_data + (MAX (y - 1, 0) * src_rowstride);
		row1 = src_data + (y * src_rowstride);
		row2 = src_data + (MIN (y + 1, src_height - 1) * src_rowstride);

		_cairo_surface_reduce_row (dest_data,
					   row0,
					   row1,
					   row2,
					   src_width);

		dest_data += dest_rowstride;
	}

	cairo_surface_mark_dirty (dest);

	return dest;
}


#endif


static cairo_surface_t *
_cairo_image_surface_scale_bilinear_2x2 (cairo_surface_t *image,
				         int              new_width,
				         int              new_height)
{
	cairo_surface_t *output;
	cairo_t         *cr;

	output = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
					     new_width,
					     new_height);
	cr = cairo_create (output);
	cairo_scale (cr,
		     (double) new_width / cairo_image_surface_get_width (image),
		     (double) new_height / cairo_image_surface_get_height (image));
	cairo_set_source_surface (cr, image, 0.0, 0.0);
	cairo_pattern_set_filter (cairo_get_source (cr), CAIRO_FILTER_BILINEAR);
	cairo_rectangle (cr, 0.0, 0.0, cairo_image_surface_get_width (image), cairo_image_surface_get_height (image));
	cairo_fill (cr);
	cairo_surface_flush (output);

	cairo_destroy (cr);

	return output;
}


cairo_surface_t *
_cairo_image_surface_scale_bilinear (cairo_surface_t *image,
				     int              new_width,
				     int              new_height)
{
	double           scale, last_scale, s;
	int              iterations = 0;
	cairo_surface_t *tmp;
	cairo_surface_t *tmp2;
	double           w, h;

	if ((new_width == 0) || (new_height == 0))
		return NULL;

	scale = (double) new_width / cairo_image_surface_get_width (image);
	last_scale = 1.0;
	s = scale;
	while (s < 1.0 / _CAIRO_MAX_SCALE_FACTOR) {
		s *= _CAIRO_MAX_SCALE_FACTOR;
		last_scale /= _CAIRO_MAX_SCALE_FACTOR;
		iterations++;
	}
	last_scale = last_scale / scale;

	tmp = cairo_surface_reference (image);
	w = cairo_image_surface_get_width (tmp);
	h = cairo_image_surface_get_height (tmp);

	while (iterations-- > 0) {
		w /= _CAIRO_MAX_SCALE_FACTOR;
		h /= _CAIRO_MAX_SCALE_FACTOR;
		tmp2 = _cairo_image_surface_scale_bilinear_2x2 (tmp, round (w), round (h));
		cairo_surface_destroy (tmp);
		tmp = tmp2;
	}

	w /= last_scale;
	h /= last_scale;
	tmp2 = _cairo_image_surface_scale_bilinear_2x2 (tmp, round (w), round (h));
	cairo_surface_destroy (tmp);

	return tmp2;
}


/* -- _cairo_image_surface_scale_async -- */


typedef struct {
	cairo_surface_t *original;
	int		 new_width;
	int		 new_height;
	scale_filter_t   quality;
	cairo_surface_t *scaled;
	GthTask         *task;
} ScaleData;


static ScaleData *
scale_data_new (cairo_surface_t	 *image,
		int		  new_width,
		int		  new_height,
		scale_filter_t    quality,
		GCancellable     *cancellable)
{
	ScaleData *scale_data;

	scale_data = g_new0 (ScaleData, 1);
	scale_data->original = cairo_surface_reference (image);
	scale_data->new_width = new_width;
	scale_data->new_height = new_height;
	scale_data->quality = quality;
	scale_data->scaled = NULL;
	scale_data->task = gth_async_task_new (NULL, NULL, NULL, NULL, NULL);
	gth_task_set_cancellable (scale_data->task, cancellable);

	return scale_data;
}


static void
scale_data_free (ScaleData *scale_data)
{
	_g_object_unref (scale_data->task);
	cairo_surface_destroy (scale_data->scaled);
	cairo_surface_destroy (scale_data->original);
	g_free (scale_data);
}


static void
scale_image_thread (GSimpleAsyncResult *result,
		    GObject            *object,
		    GCancellable       *cancellable)
{
	ScaleData *scale_data;

	scale_data = g_simple_async_result_get_op_res_gpointer (result);
	scale_data->scaled = _cairo_image_surface_scale (scale_data->original,
							 scale_data->new_width,
							 scale_data->new_height,
							 scale_data->quality,
							 GTH_ASYNC_TASK (scale_data->task));
}


void
_cairo_image_surface_scale_async (cairo_surface_t 	 *image,
				  int		 	  new_width,
				  int		  	  new_height,
				  scale_filter_t   	  quality,
				  GCancellable    	 *cancellable,
				  GAsyncReadyCallback	  ready_callback,
				  gpointer		  user_data)
{
	GSimpleAsyncResult *result;

	result = g_simple_async_result_new (NULL,
					    ready_callback,
					    user_data,
					    _cairo_image_surface_scale_async);
	g_simple_async_result_set_op_res_gpointer (result,
						   scale_data_new (image,
								   new_width,
								   new_height,
								   quality,
								   cancellable),
						   (GDestroyNotify) scale_data_free);
	g_simple_async_result_run_in_thread (result,
					     scale_image_thread,
					     G_PRIORITY_DEFAULT,
					     cancellable);

	g_object_unref (result);
}


cairo_surface_t *
_cairo_image_surface_scale_finish (GAsyncResult	 *result,
				   GError	**error)
{
	  GSimpleAsyncResult *simple;
	  ScaleData          *scale_data;

	  g_return_val_if_fail (g_simple_async_result_is_valid (result, NULL, _cairo_image_surface_scale_async), NULL);

	  simple = G_SIMPLE_ASYNC_RESULT (result);

	  if (g_simple_async_result_propagate_error (simple, error))
		  return NULL;

	  scale_data = g_simple_async_result_get_op_res_gpointer (simple);

	  return cairo_surface_reference (scale_data->scaled);
}
