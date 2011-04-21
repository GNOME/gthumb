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


static GdkPixbuf*
rotate (GdkPixbuf *src_pixbuf,
	double     angle,
	gint       auto_crop)
{
	GdkPixbuf *new_pixbuf;

	double     angle_rad;
	double     cos_angle, sin_angle;
	int        src_width, src_height;
	int        new_width, new_height;
	int        src_rowstride, new_rowstride;
	int        n_channels;
	double     x, y;
	int        xi, yi;
	double     x2, y2;
	int        x2i, y2i;
	guchar    *p_src, *p_new;
	guchar    *p_src2, *p_new2;
	
	angle_rad = angle / 180.0 * 3.1415926535;
	
	cos_angle = cos (angle_rad);
	sin_angle = sin (angle_rad);

	src_width  = gdk_pixbuf_get_width  (src_pixbuf);
	src_height = gdk_pixbuf_get_height (src_pixbuf);

	new_width  = (int) floor (      cos_angle  * src_width + fabs(sin_angle) * src_height + 0.5);
	new_height = (int) floor (fabs (sin_angle) * src_width +      cos_angle  * src_height + 0.5);
	
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
		
		for (xi = 0; xi < new_width; xi++) {
		
			x = xi - (new_width - 1) / 2.0;
			y = yi - (new_height - 1) / 2.0;
			
			x2 = cos_angle * x - sin_angle * y + (src_width - 1) / 2.0;
			y2 = sin_angle * x + cos_angle * y + (src_height - 1) / 2.0;
			
			// TODO: interpolate
			x2i = (int) floor(x2 + 0.5);
			y2i = (int) floor(y2 + 0.5);
			
			if (x2i >= 0 && x2i < src_width && y2i >= 0 && y2i < src_height) {
			
				p_src2 = p_src + src_rowstride * y2i + n_channels * x2i;
			
				p_new2[RED_PIX]   = p_src2[RED_PIX];
				p_new2[GREEN_PIX] = p_src2[GREEN_PIX];
				p_new2[BLUE_PIX]  = p_src2[BLUE_PIX];
			}
			else {
				p_new2[RED_PIX]   = 0;
				p_new2[GREEN_PIX] = 0;
				p_new2[BLUE_PIX]  = 0;
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
		    gint       auto_crop)
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
		new_pixbuf = rotate (src_pixbuf, -angle, auto_crop);
	}

	return new_pixbuf;
}
