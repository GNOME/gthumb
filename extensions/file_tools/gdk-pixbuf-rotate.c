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


#define interpolate_value(v1, f1, v2, f2) (CLAMP ((f1) * (v1) + (f2) * (v2), 0, 255))


static GdkPixbuf*
skew(GdkPixbuf *src_pixbuf,
     double     angle,
     gint       auto_crop)
{
	GdkPixbuf *new_pixbuf;
	double     skew;
	int        src_rowstride, new_rowstride, n_channels;
	int        width;
	int        width_new;
	int        height;
	int        delta_max;
	int        x;
	int        y;
	guchar    *p_src, *p_src2;
	guchar    *p_new, *p_new_row;
	guchar     r1, g1, b1;
	guchar     r2, g2, b2;
	double     delta;
	double     f1, f2;
	int        x1, x2;
	
	if (angle != 0.0 && angle >= -45 && angle <= 45.0) {

		skew = tan (angle / 180.0 * 3.1415926535);

		n_channels = gdk_pixbuf_get_n_channels (src_pixbuf);

		width = gdk_pixbuf_get_width (src_pixbuf);
		height = gdk_pixbuf_get_height (src_pixbuf);
		delta_max = (int) floor (fabs (skew * height));
		
		if (auto_crop)
			width_new = width - delta_max;
		else
			width_new = width + delta_max;

		new_pixbuf = gdk_pixbuf_new (gdk_pixbuf_get_colorspace (src_pixbuf),
			gdk_pixbuf_get_has_alpha (src_pixbuf),
			gdk_pixbuf_get_bits_per_sample (src_pixbuf),
			width_new,
			height);

		p_src = gdk_pixbuf_get_pixels (src_pixbuf);
		p_new = gdk_pixbuf_get_pixels (new_pixbuf);

		src_rowstride = gdk_pixbuf_get_rowstride (src_pixbuf);
		new_rowstride = gdk_pixbuf_get_rowstride (new_pixbuf);

		for (y = 0; y < height; y++) {
		
			if (skew > 0)
				delta = skew * y;
			else
				delta = skew * -(height - y - 1);
			
			f1 = delta - floor (delta);
			f2 = 1.0 - f1;
			
			p_new_row = p_new;

			for (x = 0; x < width_new; x++) {
			
				x1 = (int) floor (x - delta);
				x2 = (int) ceil  (x - delta);
			
				if (auto_crop) {
					x1 += delta_max;
					x2 += delta_max;
				}
				
				if (x1 < 0 || x1 >= width) {
					r1 = 0;
					g1 = 0;
					b1 = 0;
				}
				else {
					p_src2 = p_src + x1 * n_channels;
					
					r1 = p_src2[RED_PIX];
					g1 = p_src2[GREEN_PIX];
					b1 = p_src2[BLUE_PIX];
				}

				if (x2 < 0 || x2 >= width) {
					r2 = 0;
					g2 = 0;
					b2 = 0;
				}
				else {
					p_src2 = p_src + x2 * n_channels;
					
					r2 = p_src2[RED_PIX];
					g2 = p_src2[GREEN_PIX];
					b2 = p_src2[BLUE_PIX];
				}
				
				p_new_row[RED_PIX]   = interpolate_value(r1, f1, r2, f2);
				p_new_row[GREEN_PIX] = interpolate_value(g1, f1, g2, f2);
				p_new_row[BLUE_PIX]  = interpolate_value(b1, f1, b2, f2);

				p_new_row += n_channels;
			}

			p_src += src_rowstride;
			p_new += new_rowstride;
		}
	}
	else {
		g_object_ref (src_pixbuf);
		new_pixbuf = src_pixbuf;
	}
	
	return new_pixbuf;
}


GdkPixbuf*
_gdk_pixbuf_rotate (GdkPixbuf *src_pixbuf,
		    int        center_x,
		    int        center_y,
		    double     angle,
		    gint       auto_crop)
{
	GdkPixbuf *new_pixbuf;

	// TODO: implement the correct rotate algorithm
		
	new_pixbuf = skew (src_pixbuf, angle, auto_crop);

	return new_pixbuf;
}
