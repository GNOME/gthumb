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

#include <config.h>
#include <math.h>
#include <string.h>
#include <gthumb.h>
#include "gdk-pixbuf-blur.h"



static void
_gdk_pixbuf_gaussian_blur (GdkPixbuf *src,
		           int        radius)
{
	/* FIXME: to do */
}


static void
box_blur (GdkPixbuf *src,
	  GdkPixbuf *dest,
	  int        radius,
	  guchar    *div_kernel_size)
{
	int     width, height, src_rowstride, dest_rowstride, n_channels;
	guchar *p_src, *p_dest, *c1, *c2;
	int     x, y, i, i1, i2, width_minus_1, height_minus_1, radius_plus_1;
	int     r, g, b, a;
	guchar *p_dest_row, *p_dest_col;

	width = gdk_pixbuf_get_width (src);
	height = gdk_pixbuf_get_height (src);
	n_channels = gdk_pixbuf_get_n_channels (src);
	radius_plus_1 = radius + 1;

	/* horizontal blur */

	p_src = gdk_pixbuf_get_pixels (src);
	p_dest = gdk_pixbuf_get_pixels (dest);
	src_rowstride = gdk_pixbuf_get_rowstride (src);
	dest_rowstride = gdk_pixbuf_get_rowstride (dest);
	width_minus_1 = width - 1;
	for (y = 0; y < height; y++) {

		/* calc the initial sums of the kernel */

		r = g = b = a = 0;

		for (i = -radius; i <= radius; i++) {
			c1 = p_src + (CLAMP (i, 0, width_minus_1) * n_channels);
			r += c1[RED_PIX];
			g += c1[GREEN_PIX];
			b += c1[BLUE_PIX];
			/*if (n_channels == 4)
				a += c1[ALPHA_PIX];*/
		}

		p_dest_row = p_dest;
		for (x = 0; x < width; x++) {
			/* set as the mean of the kernel */

			p_dest_row[RED_PIX] = div_kernel_size[r];
			p_dest_row[GREEN_PIX] = div_kernel_size[g];
			p_dest_row[BLUE_PIX] = div_kernel_size[b];
			/*if (n_channels == 4)
				p_dest_row[ALPHA_PIX] = div_kernel_size[a];*/
			p_dest_row += n_channels;

			/* the pixel to add to the kernel */

			i1 = x + radius_plus_1;
			if (i1 > width_minus_1)
				i1 = width_minus_1;
			c1 = p_src + (i1 * n_channels);

			/* the pixel to remove from the kernel */

			i2 = x - radius;
			if (i2 < 0)
				i2 = 0;
			c2 = p_src + (i2 * n_channels);

			/* calc the new sums of the kernel */

			r += c1[RED_PIX] - c2[RED_PIX];
			g += c1[GREEN_PIX] - c2[GREEN_PIX];
			b += c1[BLUE_PIX] - c2[BLUE_PIX];
			/*if (n_channels == 4)
				a += c1[ALPHA_PIX] - c2[ALPHA_PIX];*/
		}

		p_src += src_rowstride;
		p_dest += dest_rowstride;
	}

	/* vertical blur */

	p_src = gdk_pixbuf_get_pixels (dest);
	p_dest = gdk_pixbuf_get_pixels (src);
	src_rowstride = gdk_pixbuf_get_rowstride (dest);
	dest_rowstride = gdk_pixbuf_get_rowstride (src);
	height_minus_1 = height - 1;
	for (x = 0; x < width; x++) {

		/* calc the initial sums of the kernel */

		r = g = b = a = 0;

		for (i = -radius; i <= radius; i++) {
			c1 = p_src + (CLAMP (i, 0, height_minus_1) * src_rowstride);
			r += c1[RED_PIX];
			g += c1[GREEN_PIX];
			b += c1[BLUE_PIX];
			/*if (n_channels == 4)
				a += c1[ALPHA_PIX];*/
		}

		p_dest_col = p_dest;
		for (y = 0; y < height; y++) {
			/* set as the mean of the kernel */

			p_dest_col[RED_PIX] = div_kernel_size[r];
			p_dest_col[GREEN_PIX] = div_kernel_size[g];
			p_dest_col[BLUE_PIX] = div_kernel_size[b];
			/*if (n_channels == 4)
				p_dest_row[ALPHA_PIX] = div_kernel_size[a];*/
			p_dest_col += dest_rowstride;

			/* the pixel to add to the kernel */

			i1 = y + radius_plus_1;
			if (i1 > height_minus_1)
				i1 = height_minus_1;
			c1 = p_src + (i1 * src_rowstride);

			/* the pixel to remove from the kernel */

			i2 = y - radius;
			if (i2 < 0)
				i2 = 0;
			c2 = p_src + (i2 * src_rowstride);

			/* calc the new sums of the kernel */

			r += c1[RED_PIX] - c2[RED_PIX];
			g += c1[GREEN_PIX] - c2[GREEN_PIX];
			b += c1[BLUE_PIX] - c2[BLUE_PIX];
			/*if (n_channels == 4)
				a += c1[ALPHA_PIX] - c2[ALPHA_PIX];*/
		}

		p_src += n_channels;
		p_dest += n_channels;
	}
}


static void
_gdk_pixbuf_box_blur (GdkPixbuf *src,
		      int        radius,
		      int        iterations)
{
	GdkPixbuf *tmp;
	gint64     kernel_size;
	guchar    *div_kernel_size;
	int        i;

	tmp = _gdk_pixbuf_new_compatible (src);

	kernel_size = 2 * radius + 1;
	div_kernel_size = g_new (guchar, 256 * kernel_size);
	for (i = 0; i < 256 * kernel_size; i++)
		div_kernel_size[i] = (guchar) (i / kernel_size);

	while (iterations-- > 0)
		box_blur (src, tmp, radius, div_kernel_size);

	g_object_unref (tmp);
}


void
_gdk_pixbuf_blur (GdkPixbuf *src,
		  int        radius)
{
	if (radius <= 10)
		_gdk_pixbuf_box_blur (src, radius, 3);
	else
		_gdk_pixbuf_gaussian_blur (src, radius);
}


#define interpolate_value(original, reference, distance) (CLAMP (((distance) * (reference)) + ((1.0 - (distance)) * (original)), 0, 255))


void
_gdk_pixbuf_sharpen (GdkPixbuf *src,
	             int        radius,
	             double     amount,
	             guchar     threshold)
{
	GdkPixbuf *blurred;
	int        width, height, rowstride, n_channels;
	int        x, y;
	guchar    *p_src, *p_blurred;
	guchar    *p_src_row, *p_blurred_row;
	guchar     r1, g1, b1;
	guchar     r2, g2, b2;

	blurred = gdk_pixbuf_copy (src);
	_gdk_pixbuf_blur (blurred, radius);

	width = gdk_pixbuf_get_width (src);
	height = gdk_pixbuf_get_height (src);
	rowstride = gdk_pixbuf_get_rowstride (src);
	n_channels = gdk_pixbuf_get_n_channels (src);

	p_src = gdk_pixbuf_get_pixels (src);
	p_blurred = gdk_pixbuf_get_pixels (blurred);

	for (y = 0; y < height; y++) {
		p_src_row = p_src;
		p_blurred_row = p_blurred;

		for (x = 0; x < width; x++) {
			r1 = p_src_row[RED_PIX];
			g1 = p_src_row[GREEN_PIX];
			b1 = p_src_row[BLUE_PIX];

			r2 = p_blurred_row[RED_PIX];
			g2 = p_blurred_row[GREEN_PIX];
			b2 = p_blurred_row[BLUE_PIX];

			if (ABS (r1 - r2) >= threshold)
				r1 = interpolate_value (r1, r2, amount);
			if (ABS (g1 - g2) >= threshold)
				g1 = interpolate_value (g1, g2, amount);
			if (ABS (b1 - b2) >= threshold)
				b1 = interpolate_value (b1, b2, amount);

			p_src_row[RED_PIX] = r1;
			p_src_row[GREEN_PIX] = g1;
			p_src_row[BLUE_PIX] = b1;

			p_src_row += n_channels;
			p_blurred_row += n_channels;
		}

		p_src += rowstride;
		p_blurred += rowstride;
	}

	g_object_unref (blurred);
}
