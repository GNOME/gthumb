/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2011 The Free Software Foundation, Inc.
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
#include "gdk-pixbuf-rotate.h"


#define PI 3.1415926535

#define ROUND(x) ((int) floor ((x) + 0.5))

#define INTERPOLATE(v00, v10, v01, v11, fx, fy) ((v00) + ((v10) - (v00)) * (fx) + ((v01) - (v00)) * (fy) + ((v00) - (v10) - (v01) + (v11)) * (fx) * (fy))

#define GET_VALUES(r, g, b, x, y) \
			if (x >= 0 && x < src_width && y >= 0 && y < src_height) { \
				p_src2 = p_src + src_rowstride * y + n_channels * x; \
				r = p_src2[RED_PIX]; \
				g = p_src2[GREEN_PIX]; \
				b = p_src2[BLUE_PIX]; \
			} \
			else { \
				r = R0; \
				g = G0; \
				b = B0; \
			}


void
_gdk_pixbuf_rotate_get_cropping_parameters (GdkPixbuf *src_pixbuf,
					    double     angle,
					    double    *alpha_plus_beta,
					    double    *gamma_plus_delta)
{
	double angle_rad;
	double cos_angle, sin_angle;
	double src_width, src_height;
	double px, py, pz;

	angle = CLAMP (angle, -90.0, 90.0);

	angle_rad = fabs (angle) / 180.0 * PI;
	
	cos_angle = cos (angle_rad);
	sin_angle = sin (angle_rad);

	src_width  = gdk_pixbuf_get_width  (src_pixbuf) - 1;
	src_height = gdk_pixbuf_get_height (src_pixbuf) - 1;

	px =   cos_angle * src_width - sin_angle * src_height;
	py =   sin_angle * src_width + cos_angle * src_height;
	pz = - sin_angle * src_width + cos_angle * src_height;

	*alpha_plus_beta  = 1.0 + (px * src_height) / (py * src_width);
	*gamma_plus_delta = *alpha_plus_beta; // 1.0 + (pz * src_width)  / (px * src_height);
}


void
_gdk_pixbuf_rotate_get_cropping_region (GdkPixbuf *src_pixbuf,
					double     angle,
					double     alpha,
					double     beta,
					double     gamma,
					double     delta,
					int       *x1,
					int       *y1,
					int       *x2,
					int       *y2)
{
	double angle_rad;
	double cos_angle, sin_angle;
	double src_width, src_height;
	double new_width;

	double xx1, yy1, xx2, yy2;
	
	angle = CLAMP (angle, -90.0, 90.0);
	alpha = CLAMP (alpha,   0.0,  1.0);
	beta  = CLAMP (beta,    0.0,  1.0);
	gamma = CLAMP (gamma,   0.0,  1.0);
	delta = CLAMP (delta,   0.0,  1.0);

	angle_rad = fabs (angle) / 180.0 * PI;
	
	cos_angle = cos (angle_rad);
	sin_angle = sin (angle_rad);

	src_width  = gdk_pixbuf_get_width  (src_pixbuf) - 1;
	src_height = gdk_pixbuf_get_height (src_pixbuf) - 1;

	if (src_width > src_height) {
	
		xx1 = alpha * src_width * cos_angle + src_height * sin_angle;
		yy1 = alpha * src_width * sin_angle;
	
		xx2 = (1 - beta) * src_width * cos_angle;
		yy2 = (1 - beta) * src_width * sin_angle + src_height * cos_angle;
	}
	else {
		xx1 = gamma       * src_height * sin_angle;
		yy1 = (1 - gamma) * src_height * cos_angle;
	
		xx2 = (1 - delta) * src_height * sin_angle + src_width * cos_angle;
		yy2 = delta       * src_height * cos_angle + src_width * sin_angle;
	}
	
	if (angle < 0) {

		new_width  = cos_angle * src_width + sin_angle * src_height;

		xx1 = new_width - xx1;
		xx2 = new_width - xx2;
	}

	*x1 = ROUND (MIN (xx1, xx2));
	*y1 = ROUND (MIN (yy1, yy2));
	
	*x2 = ROUND (MAX (xx1, xx2));
	*y2 = ROUND (MAX (yy1, yy2));
}


static GdkPixbuf*
rotate (GdkPixbuf *src_pixbuf,
	double     angle,
	gboolean   high_quality)
{
	const guchar R0 = 0;
	const guchar G0 = 0;
	const guchar B0 = 0;
	
	GdkPixbuf *new_pixbuf;

	double     angle_rad;
	double     cos_angle, sin_angle;
	double     src_width, src_height;
	int        new_width, new_height;
	int        src_rowstride, new_rowstride;
	int        n_channels;
	int        xi, yi;
	double     x, y;
	double     x2, y2;
	int        x2min, y2min;
	int        x2max, y2max;
	double     fx, fy;
	guchar    *p_src, *p_new;
	guchar    *p_src2, *p_new2;
	
	guchar     r00, r01, r10, r11;
	guchar     g00, g01, g10, g11;
	guchar     b00, b01, b10, b11;

	angle = CLAMP (angle, -90.0, 90.0);

	angle_rad = angle / 180.0 * PI;
	
	cos_angle = cos (angle_rad);
	sin_angle = sin (angle_rad);

	src_width  = gdk_pixbuf_get_width  (src_pixbuf) - 1;
	src_height = gdk_pixbuf_get_height (src_pixbuf) - 1;

	new_width  = ROUND (      cos_angle  * src_width + fabs(sin_angle) * src_height);
	new_height = ROUND (fabs (sin_angle) * src_width +      cos_angle  * src_height);
	
	new_pixbuf = gdk_pixbuf_new (gdk_pixbuf_get_colorspace (src_pixbuf),
				     gdk_pixbuf_get_has_alpha (src_pixbuf),
				     gdk_pixbuf_get_bits_per_sample (src_pixbuf),
				     new_width,
				     new_height);

	p_src = gdk_pixbuf_get_pixels (src_pixbuf);
	p_new = gdk_pixbuf_get_pixels (new_pixbuf);

	src_rowstride = gdk_pixbuf_get_rowstride (src_pixbuf);
	new_rowstride = gdk_pixbuf_get_rowstride (new_pixbuf);

	n_channels = gdk_pixbuf_get_n_channels (src_pixbuf);
	
	for (yi = 0; yi < new_height; yi++) {
	
		p_new2 = p_new;
		
		y = yi - new_height / 2.0;
		
		for (xi = 0; xi < new_width; xi++) {
		
			x = xi - new_width / 2.0;
			
			x2 = cos_angle * x - sin_angle * y + src_width  / 2.0;
			y2 = sin_angle * x + cos_angle * y + src_height / 2.0;
			
			if (high_quality) {
			
				// Bilinear interpolation
			
				x2min = (int) floor (x2);
				y2min = (int) floor (y2);
			
				x2max = x2min + 1;
				y2max = y2min + 1;
			
				fx = x2 - x2min;
				fy = y2 - y2min;
			
				GET_VALUES (r00, g00, b00, x2min, y2min);
				GET_VALUES (r01, g01, b01, x2max, y2min);
				GET_VALUES (r10, g10, b10, x2min, y2max);
				GET_VALUES (r11, g11, b11, x2max, y2max);
			
				p_new2[RED_PIX]   = CLAMP (INTERPOLATE (r00, r01, r10, r11, fx, fy), 0, 255);
				p_new2[GREEN_PIX] = CLAMP (INTERPOLATE (g00, g01, g10, g11, fx, fy), 0, 255);
				p_new2[BLUE_PIX]  = CLAMP (INTERPOLATE (b00, b01, b10, b11, fx, fy), 0, 255);
			}
			else {
				// Nearest neighbor
			
				x2min = ROUND (x2);
				y2min = ROUND (y2);
				
				GET_VALUES (p_new2[RED_PIX], p_new2[GREEN_PIX], p_new2[BLUE_PIX], x2min, y2min);
			}
			
			p_new2 += n_channels;
		}
		
		p_new += new_rowstride;
	}
	
	return new_pixbuf;
}


GdkPixbuf*
_gdk_pixbuf_rotate (GdkPixbuf *src_pixbuf,
		    double     angle,
		    gboolean   high_quality)
{
	GdkPixbuf *new_pixbuf;
	
	if (angle == 0.0) {
		new_pixbuf = src_pixbuf;
		g_object_ref (new_pixbuf);
	}
	else if (angle >= 90.0) {
		new_pixbuf = gdk_pixbuf_rotate_simple (src_pixbuf, GDK_PIXBUF_ROTATE_CLOCKWISE);
	}
	else if (angle <= -90.0) {
		new_pixbuf = gdk_pixbuf_rotate_simple (src_pixbuf, GDK_PIXBUF_ROTATE_COUNTERCLOCKWISE);
	}
	else {
		new_pixbuf = rotate (src_pixbuf, -angle, high_quality);
	}

	return new_pixbuf;
}
