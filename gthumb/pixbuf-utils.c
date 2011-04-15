/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001-2009 The Free Software Foundation, Inc.
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
#include <stdio.h>
#include <stdlib.h>
#include "cairo-utils.h"
#include "pixbuf-utils.h"


GdkPixbuf*
_gdk_pixbuf_new_void (int width,
		      int height)
{
	GdkPixbuf *p;

	p = gdk_pixbuf_new (GDK_COLORSPACE_RGB,
			    TRUE,
			    8,
			    width,
			    height);
	gdk_pixbuf_fill (p, 0xFFFFFF00);

	return p;
}


GdkPixbuf *
_gdk_pixbuf_new_compatible (GdkPixbuf *src)
{
	if (src == NULL)
		return NULL;

	return gdk_pixbuf_new (gdk_pixbuf_get_colorspace (src),
			       gdk_pixbuf_get_has_alpha (src),
			       gdk_pixbuf_get_bits_per_sample (src),
			       gdk_pixbuf_get_width (src),
			       gdk_pixbuf_get_height (src));
}


GdkPixbuf *
_gdk_pixbuf_new_from_cairo_context (cairo_t *cr)
{
	return _gdk_pixbuf_new_from_cairo_surface (cairo_get_target (cr));
}


/* Started from from http://www.gtkforums.com/about5204.html
 * Author: tadeboro */
GdkPixbuf *
_gdk_pixbuf_new_from_cairo_surface (cairo_surface_t *surface)
{
	int            width;
	int            height;
	int            s_stride;
	unsigned char *s_pixels;
	GdkPixbuf     *pixbuf;
	int            p_stride;
	guchar        *p_pixels;
	int            p_n_channels;

	if (cairo_surface_status (surface) != CAIRO_STATUS_SUCCESS)
		return NULL;

	width = cairo_image_surface_get_width (surface);
	height = cairo_image_surface_get_height (surface);
	s_stride = cairo_image_surface_get_stride (surface);
	s_pixels = cairo_image_surface_get_data (surface);

	pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, _cairo_image_surface_get_has_alpha (surface), 8, width, height);
	p_stride = gdk_pixbuf_get_rowstride (pixbuf);
	p_pixels = gdk_pixbuf_get_pixels (pixbuf);
	p_n_channels = gdk_pixbuf_get_n_channels (pixbuf);

	while (height--) {
		guchar *s_iter = s_pixels;
	        guchar *p_iter = p_pixels;
	        int     i;

	        for (i = 0; i < width; i++) {
	        	gdouble alpha_factor = (gdouble) 0xff / s_iter[CAIRO_ALPHA];

	        	p_iter[0] = (guchar) (alpha_factor * s_iter[CAIRO_RED]  + .5);
	        	p_iter[1] = (guchar) (alpha_factor * s_iter[CAIRO_GREEN] + .5);
	        	p_iter[2] = (guchar) (alpha_factor * s_iter[CAIRO_BLUE] + .5);
	        	if (p_n_channels == 4)
	        		p_iter[3] = s_iter[CAIRO_ALPHA];

	        	s_iter += 4;
	        	p_iter += p_n_channels;
		}

		s_pixels += s_stride;
		p_pixels += p_stride;
	}

	return pixbuf;
}


/* The gdk_pixbuf scaling routines do not handle large-ratio downscaling
   very well. Memory usage explodes and the application may freeze or crash.
   For scale-down ratios in excess of 100, do the scale in two steps.
   It is faster and safer that way. See bug 80925 for background info. */
GdkPixbuf*
_gdk_pixbuf_scale_simple_safe (const GdkPixbuf *src,
			       int              dest_width,
			       int              dest_height,
			       GdkInterpType    interp_type)
{
	GdkPixbuf* temp_pixbuf1;
	GdkPixbuf* temp_pixbuf2;
	int        x_ratio, y_ratio;
	int        temp_width = dest_width, temp_height = dest_height;

	g_assert (dest_width >= 1);
	g_assert (dest_height >= 1);

	x_ratio = gdk_pixbuf_get_width (src) / dest_width;
	y_ratio = gdk_pixbuf_get_height (src) / dest_height;

	if (x_ratio > 100)
		/* Scale down to 10x the requested size first. */
		temp_width = 10 * dest_width;

	if (y_ratio > 100)
		/* Scale down to 10x the requested size first. */
		temp_height = 10 * dest_height;

	if ( (temp_width != dest_width) || (temp_height != dest_height)) {
		temp_pixbuf1 = gdk_pixbuf_scale_simple (src, temp_width, temp_height, interp_type);
		temp_pixbuf2 = gdk_pixbuf_scale_simple (temp_pixbuf1, dest_width, dest_height, interp_type);
		g_object_unref (temp_pixbuf1);
	} else
		temp_pixbuf2 = gdk_pixbuf_scale_simple (src, dest_width, dest_height, interp_type);

	return temp_pixbuf2;
}


GdkPixbuf *
_gdk_pixbuf_scale_composite (const GdkPixbuf *src,
			     int              dest_width,
			     int              dest_height,
			     GdkInterpType    interp_type)
{
	GdkPixbuf *dest;

	if (! gdk_pixbuf_get_has_alpha (src))
		return _gdk_pixbuf_scale_simple_safe (src, dest_width, dest_height, interp_type);

	g_return_val_if_fail (src != NULL, NULL);
	g_return_val_if_fail (dest_width > 0, NULL);
	g_return_val_if_fail (dest_height > 0, NULL);

	dest = gdk_pixbuf_new (GDK_COLORSPACE_RGB, gdk_pixbuf_get_has_alpha (src), 8, dest_width, dest_height);
	if (dest == NULL)
		return NULL;

	gdk_pixbuf_composite_color (src,
				    dest,
				    0, 0, dest_width, dest_height, 0, 0,
				    (double) dest_width / gdk_pixbuf_get_width (src),
				    (double) dest_height / gdk_pixbuf_get_height (src),
				    interp_type,
				    255,
				    0, 0,
				    200,
				    0xFFFFFF,
				    0xFFFFFF);

	return dest;
}


GdkPixbuf *
_gdk_pixbuf_scale_squared (GdkPixbuf     *p,
			   int            size,
			   GdkInterpType  interp_type)
{
	int        w, h, tw, th;
	GdkPixbuf *p1;
	int        x, y;
	GdkPixbuf *p2;

	w = gdk_pixbuf_get_width (p);
	h = gdk_pixbuf_get_height (p);

	if ((w < size) && (h < size))
		return gdk_pixbuf_copy (p);

	if (w > h) {
		th = size;
		tw = (int) (((double) w / h) * th);
	}
	else {
		tw = size;
		th = (int) (((double) h / w) * tw);
	}

	if ((tw != w) || (th != h))
		p1 = _gdk_pixbuf_scale_composite (p, tw, th, interp_type);
	else
		p1 = g_object_ref (p);

	if ((tw == size) && (th == size))
		return p1;

	x = (tw - size) / 2;
	y = (th - size) / 2;
	p2 = gdk_pixbuf_new_subpixbuf (p1, x, y, size, size);

	g_object_unref (p1);

	return p2;
}


/*
 * Returns a transformed image.
 */
GdkPixbuf *
_gdk_pixbuf_transform (GdkPixbuf    *src,
		       GthTransform  transform)
{
	GdkPixbuf *temp = NULL, *dest = NULL;

	if (src == NULL)
		return NULL;

	switch (transform) {
	case GTH_TRANSFORM_NONE:
		dest = src;
		g_object_ref (dest);
		break;
	case GTH_TRANSFORM_FLIP_H:
		dest = gdk_pixbuf_flip (src, TRUE);
		break;
	case GTH_TRANSFORM_ROTATE_180:
		dest = gdk_pixbuf_rotate_simple (src, GDK_PIXBUF_ROTATE_UPSIDEDOWN);
		break;
	case GTH_TRANSFORM_FLIP_V:
		dest = gdk_pixbuf_flip (src, FALSE);
		break;
	case GTH_TRANSFORM_TRANSPOSE:
		temp = gdk_pixbuf_rotate_simple (src, GDK_PIXBUF_ROTATE_CLOCKWISE);
		dest = gdk_pixbuf_flip (temp, TRUE);
		g_object_unref (temp);
		break;
	case GTH_TRANSFORM_ROTATE_90:
		dest = gdk_pixbuf_rotate_simple (src, GDK_PIXBUF_ROTATE_CLOCKWISE);
		break;
	case GTH_TRANSFORM_TRANSVERSE:
		temp = gdk_pixbuf_rotate_simple (src, GDK_PIXBUF_ROTATE_CLOCKWISE);
		dest = gdk_pixbuf_flip (temp, FALSE);
		g_object_unref (temp);
		break;
	case GTH_TRANSFORM_ROTATE_270:
		dest = gdk_pixbuf_rotate_simple (src, GDK_PIXBUF_ROTATE_COUNTERCLOCKWISE);
		break;
	default:
		dest = src;
		g_object_ref (dest);
		break;
	}

	return dest;
}


void
_gdk_pixbuf_colorshift (GdkPixbuf *dest,
			GdkPixbuf *src,
			int        shift)
{
	int     i, j;
	int     width, height, has_alpha, srcrowstride, destrowstride;
	guchar *target_pixels;
	guchar *original_pixels;
	guchar *pixsrc;
	guchar *pixdest;
	int     val;
	guchar  r,g,b;

	has_alpha       = gdk_pixbuf_get_has_alpha (src);
	width           = gdk_pixbuf_get_width (src);
	height          = gdk_pixbuf_get_height (src);
	srcrowstride    = gdk_pixbuf_get_rowstride (src);
	destrowstride   = gdk_pixbuf_get_rowstride (dest);
	target_pixels   = gdk_pixbuf_get_pixels (dest);
	original_pixels = gdk_pixbuf_get_pixels (src);

	for (i = 0; i < height; i++) {
		pixdest = target_pixels + i*destrowstride;
		pixsrc  = original_pixels + i*srcrowstride;
		for (j = 0; j < width; j++) {
			r            = *(pixsrc++);
			g            = *(pixsrc++);
			b            = *(pixsrc++);
			val          = r + shift;
			*(pixdest++) = CLAMP (val, 0, 255);
			val          = g + shift;
			*(pixdest++) = CLAMP (val, 0, 255);
			val          = b + shift;
			*(pixdest++) = CLAMP (val, 0, 255);

			if (has_alpha)
				*(pixdest++) = *(pixsrc++);
		}
	}
}


/* From gtkcellrendererpixbuf.c
 * Copyright (C) 2000  Red Hat, Inc.,  Jonathan Blandford <jrb@redhat.com>
 *
 * modified for gthumb */
GdkPixbuf *
_gdk_pixbuf_colorize (GdkPixbuf *src,
		      GdkColor  *new_color,
		      gdouble    alpha)
{
	gint i, j;
	gint width, height, has_alpha, src_row_stride, dst_row_stride;
	gint red_value, green_value, blue_value;
	guchar *target_pixels;
	guchar *original_pixels;
	guchar *pixsrc;
	guchar *pixdest;
	GdkPixbuf *dest;

	red_value = new_color->red / 255.0;
	green_value = new_color->green / 255.0;
	blue_value = new_color->blue / 255.0;

	dest = gdk_pixbuf_new (gdk_pixbuf_get_colorspace (src),
			       TRUE /*gdk_pixbuf_get_has_alpha (src)*/,
			       gdk_pixbuf_get_bits_per_sample (src),
			       gdk_pixbuf_get_width (src),
			       gdk_pixbuf_get_height (src));

	has_alpha = gdk_pixbuf_get_has_alpha (src);
	width = gdk_pixbuf_get_width (src);
	height = gdk_pixbuf_get_height (src);
	src_row_stride = gdk_pixbuf_get_rowstride (src);
	dst_row_stride = gdk_pixbuf_get_rowstride (dest);
	target_pixels = gdk_pixbuf_get_pixels (dest);
	original_pixels = gdk_pixbuf_get_pixels (src);

	for (i = 0; i < height; i++) {
		pixdest = target_pixels + i*dst_row_stride;
		pixsrc = original_pixels + i*src_row_stride;
		for (j = 0; j < width; j++) {
			*pixdest++ = (*pixsrc++ * red_value) >> 8;
			*pixdest++ = (*pixsrc++ * green_value) >> 8;
			*pixdest++ = (*pixsrc++ * blue_value) >> 8;
			if (has_alpha)
				*pixdest++ = (*pixsrc++ * alpha);
			else
				*pixdest++ = (255 * alpha);
		}
	}

	return dest;
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
			double x_y, x_1_y, y_1_x, _1_x_1_y;

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


gboolean
scale_keeping_ratio_min (int      *width,
			 int      *height,
			 int       min_width,
			 int       min_height,
			 int       max_width,
			 int       max_height,
			 gboolean  allow_upscaling)
{
	double   w = *width;
	double   h = *height;
	double   min_w = min_width;
	double   min_h = min_height;
	double   max_w = max_width;
	double   max_h = max_height;
	double   factor;
	int      new_width, new_height;
	gboolean modified;

	if ((*width < max_width) && (*height < max_height) && ! allow_upscaling)
		return FALSE;

	if (((*width < min_width) || (*height < min_height)) && ! allow_upscaling)
		return FALSE;

	factor = MAX (MIN (max_w / w, max_h / h), MAX (min_w / w, min_h / h));
	new_width  = MAX ((int) floor (w * factor + 0.50), 1);
	new_height = MAX ((int) floor (h * factor + 0.50), 1);

	modified = (new_width != *width) || (new_height != *height);

	*width = new_width;
	*height = new_height;

	return modified;
}


gboolean
scale_keeping_ratio (int      *width,
		     int      *height,
		     int       max_width,
		     int       max_height,
		     gboolean  allow_upscaling)
{
	return scale_keeping_ratio_min (width,
					height,
					0,
					0,
					max_width,
					max_height,
					allow_upscaling);
}


gboolean
_g_mime_type_is_writable (const char *mime_type)
{
	GSList *list;
	GSList *scan;

	list = gdk_pixbuf_get_formats ();
	for (scan = list; scan; scan = scan->next) {
		GdkPixbufFormat  *format = scan->data;
		char            **mime_types;
		int               i;

		mime_types = gdk_pixbuf_format_get_mime_types (format);
		for (i = 0; mime_types[i] != NULL; i++)
			if (strcmp (mime_type, mime_types[i]) == 0)
				return ! gdk_pixbuf_format_is_disabled (format) && gdk_pixbuf_format_is_writable (format);

		g_strfreev (mime_types);
	}

	g_slist_free (list);

	return FALSE;
}
