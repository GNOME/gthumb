/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001 The Free Software Foundation, Inc.
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
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 */

#include <math.h>
#include "pixbuf-utils.h"


enum {
	RED_PIX   = 0,
	GREEN_PIX = 1,
	BLUE_PIX  = 2,
	ALPHA_PIX = 3
};


/*
 * Returns a copy of pixbuf src rotated 90 degrees clockwise or 90 
 * counterclockwise.
 */
GdkPixbuf *
pixbuf_copy_rotate_90 (GdkPixbuf *src, 
		       gboolean counter_clockwise)
{
	GdkPixbuf *dest;
	int        has_alpha;
	int        sw, sh, srs;
	int        dw, dh, drs;
	guchar    *s_pix;
        guchar    *d_pix;
	guchar    *sp;
        guchar    *dp;
	int        i, j;
	int        a;

	if (!src) return NULL;

	sw = gdk_pixbuf_get_width (src);
	sh = gdk_pixbuf_get_height (src);
	has_alpha = gdk_pixbuf_get_has_alpha (src);
	srs = gdk_pixbuf_get_rowstride (src);
	s_pix = gdk_pixbuf_get_pixels (src);

	dw = sh;
	dh = sw;
	dest = gdk_pixbuf_new (GDK_COLORSPACE_RGB, has_alpha, 8, dw, dh);
	drs = gdk_pixbuf_get_rowstride (dest);
	d_pix = gdk_pixbuf_get_pixels (dest);

	a = (has_alpha ? 4 : 3);

	for (i = 0; i < sh; i++) {
		sp = s_pix + (i * srs);
		for (j = 0; j < sw; j++) {
			if (counter_clockwise)
				dp = d_pix + ((dh - j - 1) * drs) + (i * a);
			else
				dp = d_pix + (j * drs) + ((dw - i - 1) * a);

			*(dp++) = *(sp++);	/* r */
			*(dp++) = *(sp++);	/* g */
			*(dp++) = *(sp++);	/* b */
			if (has_alpha) *(dp) = *(sp++);	/* a */
		}
	}

	return dest;
}


/*
 * Returns a copy of pixbuf mirrored and or flipped.
 * TO do a 180 degree rotations set both mirror and flipped TRUE
 * if mirror and flip are FALSE, result is a simple copy.
 */
GdkPixbuf *
pixbuf_copy_mirror (GdkPixbuf *src, 
		    gboolean mirror, 
		    gboolean flip)
{
	GdkPixbuf *dest;
	int        has_alpha;
	int        w, h, srs;
	int        drs;
	guchar    *s_pix;
        guchar    *d_pix;
	guchar    *sp;
        guchar    *dp;
	int        i, j;
	int        a;

	if (!src) return NULL;

	w = gdk_pixbuf_get_width (src);
	h = gdk_pixbuf_get_height (src);
	has_alpha = gdk_pixbuf_get_has_alpha (src);
	srs = gdk_pixbuf_get_rowstride (src);
	s_pix = gdk_pixbuf_get_pixels (src);

	dest = gdk_pixbuf_new (GDK_COLORSPACE_RGB, has_alpha, 8, w, h);
	drs = gdk_pixbuf_get_rowstride (dest);
	d_pix = gdk_pixbuf_get_pixels (dest);

	a = has_alpha ? 4 : 3;

	for (i = 0; i < h; i++)	{
		sp = s_pix + (i * srs);
		if (flip)
			dp = d_pix + ((h - i - 1) * drs);
		else
			dp = d_pix + (i * drs);

		if (mirror) {
			dp += (w - 1) * a;
			for (j = 0; j < w; j++) {
				*(dp++) = *(sp++);	/* r */
				*(dp++) = *(sp++);	/* g */
				*(dp++) = *(sp++);	/* b */
				if (has_alpha) *(dp) = *(sp++);	/* a */
				dp -= (a + 3);
			}
		} else {
			for (j = 0; j < w; j++) {
				*(dp++) = *(sp++);	/* r */
				*(dp++) = *(sp++);	/* g */
				*(dp++) = *(sp++);	/* b */
				if (has_alpha) *(dp++) = *(sp++);	/* a */
			}
		}
	}
	
	return dest;
}


GdkPixbuf *
pixbuf_copy_gray (GdkPixbuf *src)
{
	GdkPixbuf *dest;
	int        i, j, t;
	int        width, height, has_alpha, bytes_per_pixel; 
	int        src_rowstride, dest_rowstride;
	guchar    *src_line, *dest_line;
	guchar    *src_pixel, *dest_pixel;
	guchar     min, max, lightness;
	
	has_alpha = gdk_pixbuf_get_has_alpha (src);
	bytes_per_pixel = has_alpha ? 4 : 3;
	width = gdk_pixbuf_get_width (src);
	height = gdk_pixbuf_get_height (src);
	src_rowstride = gdk_pixbuf_get_rowstride (src);

	dest = gdk_pixbuf_new (GDK_COLORSPACE_RGB, has_alpha, 8, width, height);
	dest_rowstride = gdk_pixbuf_get_rowstride (dest);
	
	src_line = gdk_pixbuf_get_pixels (src);
	dest_line = gdk_pixbuf_get_pixels (dest);
	
#define CLAMP_UCHAR(v) (t = (v), CLAMP (t, 0, 255))

	for (i = 0 ; i < height ; i++) {
		src_pixel = src_line;
		src_line += src_rowstride;
		dest_pixel = dest_line;
		dest_line += dest_rowstride;
		
		for (j = 0 ; j < width ; j++) {
			max = MAX (src_pixel[RED_PIX], src_pixel[GREEN_PIX]);
			max = MAX (max, src_pixel[BLUE_PIX]);
			min = MIN (src_pixel[RED_PIX], src_pixel[GREEN_PIX]);
			min = MIN (min, src_pixel[BLUE_PIX]);

			lightness = (max + min) / 2;

			dest_pixel[RED_PIX] = CLAMP_UCHAR (lightness);
			dest_pixel[GREEN_PIX] = CLAMP_UCHAR (lightness);
			dest_pixel[BLUE_PIX] = CLAMP_UCHAR (lightness);
			
			if (has_alpha)
				dest_pixel[ALPHA_PIX] = src_pixel[ALPHA_PIX];
			
			src_pixel += bytes_per_pixel;
			dest_pixel += bytes_per_pixel;
		}
	}

	return dest;
}


void
pixmap_from_xpm (const char **data, 
		 GdkPixmap **pixmap, 
		 GdkBitmap **mask)
{
	GdkPixbuf *pixbuf;

	pixbuf = gdk_pixbuf_new_from_xpm_data (data);
	gdk_pixbuf_render_pixmap_and_mask (pixbuf, pixmap, mask, 127);
	g_object_unref (pixbuf);
}


void
_gdk_pixbuf_vertical_gradient (GdkPixbuf *pixbuf, 
			       guint32    color1,
			       guint32    color2)
{
	guchar   *pixels;
	guint32   r1, g1, b1, a1;
	guint32   r2, g2, b2, a2;
	double    r, g, b, a;
	double    rd, gd, bd, ad;
	guint32   ri, gi, bi, ai;
	guchar   *p;
	guint     width, height;
	guint     w, h;
	int       n_channels, rowstride;
	
	g_return_if_fail (GDK_IS_PIXBUF (pixbuf));

	width = gdk_pixbuf_get_width (pixbuf);
	height = gdk_pixbuf_get_height (pixbuf);

	if (width == 0 || height == 0)
		return;

	pixels = gdk_pixbuf_get_pixels (pixbuf);

	r1 = (color1 & 0xff000000) >> 24;
	g1 = (color1 & 0x00ff0000) >> 16;
	b1 = (color1 & 0x0000ff00) >> 8;
	a1 = (color1 & 0x000000ff);

	r2 = (color2 & 0xff000000) >> 24;
	g2 = (color2 & 0x00ff0000) >> 16;
	b2 = (color2 & 0x0000ff00) >> 8;
	a2 = (color2 & 0x000000ff);

	rd = ((double) (r2) - r1) / height;
	gd = ((double) (g2) - g1) / height;
	bd = ((double) (b2) - b1) / height;
	ad = ((double) (a2) - a1) / height;

	n_channels = gdk_pixbuf_get_n_channels (pixbuf);
	rowstride = gdk_pixbuf_get_rowstride (pixbuf);

	r = r1;
	g = g1;
	b = b1;
	a = a1;

	for (h = height; h > 0; h--) {
		w = width;
		p = pixels;

		ri = (int) r;
		gi = (int) g;
		bi = (int) b;
		ai = (int) a;

		switch (n_channels) {
		case 3:
			while (w--) {
				p[0] = ri;
				p[1] = gi;
				p[2] = bi;
				p += 3;
                        }
			break;
		case 4:
			while (w--) {
				p[0] = ri;
				p[1] = gi;
				p[2] = bi;
				p[3] = ai;
				p += 4;
			}
			break;
		default:
			break;
		}

		r += rd;
		g += gd;
		b += bd;
		a += ad;

		pixels += rowstride;
	}
}


void
_gdk_pixbuf_horizontal_gradient (GdkPixbuf *pixbuf, 
				 guint32    color1,
				 guint32    color2)
{
	guchar   *pixels;
	guint32   r1, g1, b1, a1;
	guint32   r2, g2, b2, a2;
	double    r, g, b, a;
	double    rd, gd, bd, ad;
	guint32   ri, gi, bi, ai;
	guchar   *p;
	guint     width, height;
	guint     w, h;
	int       n_channels, rowstride;
	
	g_return_if_fail (GDK_IS_PIXBUF (pixbuf));

	width = gdk_pixbuf_get_width (pixbuf);
	height = gdk_pixbuf_get_height (pixbuf);

	if (width == 0 || height == 0)
		return;

	pixels = gdk_pixbuf_get_pixels (pixbuf);

	r1 = (color1 & 0xff000000) >> 24;
	g1 = (color1 & 0x00ff0000) >> 16;
	b1 = (color1 & 0x0000ff00) >> 8;
	a1 = (color1 & 0x000000ff);

	r2 = (color2 & 0xff000000) >> 24;
	g2 = (color2 & 0x00ff0000) >> 16;
	b2 = (color2 & 0x0000ff00) >> 8;
	a2 = (color2 & 0x000000ff);

	rd = ((double) (r2) - r1) / width;
	gd = ((double) (g2) - g1) / width;
	bd = ((double) (b2) - b1) / width;
	ad = ((double) (a2) - a1) / width;

	n_channels = gdk_pixbuf_get_n_channels (pixbuf);
	rowstride = gdk_pixbuf_get_rowstride (pixbuf);

	r = r1;
	g = g1;
	b = b1;
	a = a1;

	for (w = 0; w < width; w++) {
		h = height;
		p = pixels;

		ri = (int) rint (r);
		gi = (int) rint (g);
		bi = (int) rint (b);
		ai = (int) rint (a);

		switch (n_channels) {
		case 3:
			while (h--) {
				p[0] = ri;
				p[1] = gi;
				p[2] = bi;
				p += rowstride;
                        }
			break;
		case 4:
			while (h--) {
				p[0] = ri;
				p[1] = gi;
				p[2] = bi;
				p[3] = ai;
				p += rowstride;
			}
			break;
		default:
			break;
		}

		r += rd;
		g += gd;
		b += bd;
		a += ad;

		pixels += n_channels;
	}
}


void
_gdk_pixbuf_hv_gradient (GdkPixbuf *pixbuf, 
			 guint32    hcolor1,
			 guint32    hcolor2,
			 guint32    vcolor1,
			 guint32    vcolor2)
{
	guchar   *pixels;
	guint32   hr1, hg1, hb1, ha1;
	guint32   hr2, hg2, hb2, ha2;
	guint32   vr1, vg1, vb1, va1;
	guint32   vr2, vg2, vb2, va2;
	double    r, g, b, a;
	guint32   ri, gi, bi, ai;
	guchar   *p;
	guint     width, height;
	guint     w, h;
	int       n_channels, rowstride;
	double    x, y;

	g_return_if_fail (GDK_IS_PIXBUF (pixbuf));

	width = gdk_pixbuf_get_width (pixbuf);
	height = gdk_pixbuf_get_height (pixbuf);

	if (width == 0 || height == 0)
		return;

	pixels = gdk_pixbuf_get_pixels (pixbuf);

	hr1 = (hcolor1 & 0xff000000) >> 24;
	hg1 = (hcolor1 & 0x00ff0000) >> 16;
	hb1 = (hcolor1 & 0x0000ff00) >> 8;
	ha1 = (hcolor1 & 0x000000ff);

	hr2 = (hcolor2 & 0xff000000) >> 24;
	hg2 = (hcolor2 & 0x00ff0000) >> 16;
	hb2 = (hcolor2 & 0x0000ff00) >> 8;
	ha2 = (hcolor2 & 0x000000ff);

	vr1 = (vcolor1 & 0xff000000) >> 24;
	vg1 = (vcolor1 & 0x00ff0000) >> 16;
	vb1 = (vcolor1 & 0x0000ff00) >> 8;
	va1 = (vcolor1 & 0x000000ff);

	vr2 = (vcolor2 & 0xff000000) >> 24;
	vg2 = (vcolor2 & 0x00ff0000) >> 16;
	vb2 = (vcolor2 & 0x0000ff00) >> 8;
	va2 = (vcolor2 & 0x000000ff);

	n_channels = gdk_pixbuf_get_n_channels (pixbuf);
	rowstride = gdk_pixbuf_get_rowstride (pixbuf);

	for (h = 0; h < height; h++) {
		p = pixels;

		x = (((double) height) - h) / height;

		for (w = 0; w < width; w++) {
			gdouble x_y, x_1_y, y_1_x, _1_x_1_y;

			y = (((double) width) - w) / width;;

			x_y      = x * y;
			x_1_y    = x * (1.0 - y);
			y_1_x    = y * (1.0 - x);
			_1_x_1_y = (1.0 - x) * (1.0 - y);

			r = hr1 * x_y + hr2 * x_1_y + vr1 * y_1_x + vr2 * _1_x_1_y;
			g = hg1 * x_y + hg2 * x_1_y + vg1 * y_1_x + vg2 * _1_x_1_y;
			b = hb1 * x_y + hb2 * x_1_y + vb1 * y_1_x + vb2 * _1_x_1_y;
			a = ha1 * x_y + ha2 * x_1_y + va1 * y_1_x + va2 * _1_x_1_y;

			ri = (int) r;
			gi = (int) g;
			bi = (int) b;
			ai = (int) a;

			switch (n_channels) {
			case 3:
				p[0] = ri;
				p[1] = gi;
				p[2] = bi;
				p += 3;
				break;
			case 4:
				p[0] = ri;
				p[1] = gi;
				p[2] = bi;
				p[3] = ai;
				p += 4;
				break;
			default:
				break;
			}
		}

		pixels += rowstride;
	}
}
