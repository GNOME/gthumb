/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001, 2003 The Free Software Foundation, Inc.
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

/* Some bits are based upon the gimp source code, the original copyright
 * note follows:
 *
 * The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/* The jpeg saver is based upon the gdk-pixbuf library, which has the
 * following copyright note : 
 *
 * Copyright (C) 1999 Michael Zucchi
 * Copyright (C) 1999 The Free Software Foundation
 * 
 * Progressive loading code Copyright (C) 1999 Red Hat, Inc.
 *
 * Authors: Michael Zucchi <zucchi@zedzone.mmc.com.au>
 *          Federico Mena-Quintero <federico@gimp.org>
 *          Michael Fulbright <drmike@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include "pixbuf-utils.h"

#ifdef HAVE_LIBTIFF
#include <tiffio.h>
#endif /* HAVE_LIBTIFF */

#ifdef HAVE_LIBJPEG
#include <stdlib.h>
#include <setjmp.h>
#include <jpeglib.h>
#endif /* HAVE_LIBJPEG */


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
_gdk_pixbuf_copy_rotate_90 (GdkPixbuf *src, 
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
_gdk_pixbuf_copy_mirror (GdkPixbuf *src, 
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





#ifdef HAVE_LIBJPEG

/* error handler data */
struct error_handler_data {
	struct jpeg_error_mgr pub;
	sigjmp_buf setjmp_buffer;
        GError **error;
};


static void
fatal_error_handler (j_common_ptr cinfo)
{
	struct error_handler_data *errmgr;
        char buffer[JMSG_LENGTH_MAX];
        
	errmgr = (struct error_handler_data *) cinfo->err;
        
        /* Create the message */
        (* cinfo->err->format_message) (cinfo, buffer);

        /* broken check for *error == NULL for robustness against
         * crappy JPEG library
         */
        if (errmgr->error && *errmgr->error == NULL) {
                g_set_error (errmgr->error,
                             GDK_PIXBUF_ERROR,
                             GDK_PIXBUF_ERROR_CORRUPT_IMAGE,
			     "Error interpreting JPEG image file (%s)",
                             buffer);
        }
        
	siglongjmp (errmgr->setjmp_buffer, 1);

        g_assert_not_reached ();
}


static void
output_message_handler (j_common_ptr cinfo)
{
	/* This method keeps libjpeg from dumping crap to stderr */
	/* do nothing */
}


static gboolean
_gdk_pixbuf_save_as_jpeg (GdkPixbuf     *pixbuf, 
			  const char    *filename,
			  char         **keys,
			  char         **values,
			  GError       **error)
{
	struct jpeg_compress_struct cinfo;
	struct error_handler_data jerr;
	FILE              *file;
	guchar            *buf = NULL;
	guchar            *ptr;
	guchar            *pixels = NULL;
	volatile int       quality = 75; /* default; must be between 0 and 100 */
	volatile int       smoothing = 0;
	volatile gboolean  optimize = FALSE;
	volatile gboolean  progressive = FALSE;
	int                i, j;
	int                w, h = 0;
	int                rowstride = 0;
	volatile int       bpp;

	if (keys && *keys) {
		char **kiter = keys;
		char **viter = values;
		
		while (*kiter) {
			if (strcmp (*kiter, "quality") == 0) {
				char *endptr = NULL;
				quality = strtol (*viter, &endptr, 10);

				if (endptr == *viter) {
					g_set_error (error,
						     GDK_PIXBUF_ERROR,
						     GDK_PIXBUF_ERROR_BAD_OPTION,
						     "JPEG quality must be a value between 0 and 100; value '%s' could not be parsed.",
						     *viter);
					
					return FALSE;
				}
				
				if (quality < 0 || quality > 100) {
					g_set_error (error,
						     GDK_PIXBUF_ERROR,
						     GDK_PIXBUF_ERROR_BAD_OPTION,
						     "JPEG quality must be a value between 0 and 100; value '%d' is not allowed.",
						     quality);
					
					return FALSE;
				}

			} else if (strcmp (*kiter, "smooth") == 0) {
				char *endptr = NULL;
				smoothing = strtol (*viter, &endptr, 10);

				if (endptr == *viter) {
					g_set_error (error,
						     GDK_PIXBUF_ERROR,
						     GDK_PIXBUF_ERROR_BAD_OPTION,
						     "JPEG smoothing must be a value between 0 and 100; value '%s' could not be parsed.",
						     *viter);
					
					return FALSE;
				}
				
				if (smoothing < 0 || smoothing > 100) {
					g_set_error (error,
						     GDK_PIXBUF_ERROR,
						     GDK_PIXBUF_ERROR_BAD_OPTION,
						     "JPEG smoothing must be a value between 0 and 100; value '%d' is not allowed.",
						     smoothing);
					
					return FALSE;
				}

			} else if (strcmp (*kiter, "optimize") == 0) {
				if (strcmp (*viter, "yes") == 0)
					optimize = TRUE;
				else if (strcmp (*viter, "no") == 0)
					optimize = FALSE;
				else {
					g_set_error (error,
						     GDK_PIXBUF_ERROR,
						     GDK_PIXBUF_ERROR_BAD_OPTION,
						     "JPEG optimize option must be 'yes' or 'no', value is: %s", *viter);
					
					return FALSE;
				}

			} else if (strcmp (*kiter, "progressive") == 0) {
				if (strcmp (*viter, "yes") == 0)
					progressive = TRUE;
				else if (strcmp (*viter, "no") == 0)
					progressive = FALSE;
				else {
					g_set_error (error,
						     GDK_PIXBUF_ERROR,
						     GDK_PIXBUF_ERROR_BAD_OPTION,
						     "JPEG progressive option must be 'yes' or 'no', value is: %s", *viter);
					
					return FALSE;
				}
				
			} else {
				g_warning ("Bad option name '%s' passed to JPEG saver", *kiter);
				return FALSE;
			}
			
			++kiter;
			++viter;
		}
	}
	
	file = fopen (filename, "wb");
	
	if (file == NULL) {
		g_set_error (error,
			     GDK_PIXBUF_ERROR,
			     GDK_PIXBUF_ERROR_FAILED,
			     "Can't write image to file '%s'", 
			     filename);
		return FALSE;
	}
       
	rowstride = gdk_pixbuf_get_rowstride (pixbuf);
	
	w = gdk_pixbuf_get_width (pixbuf);
	h = gdk_pixbuf_get_height (pixbuf);

	if (gdk_pixbuf_get_has_alpha (pixbuf))
		bpp = 4;
	else
		bpp = 3;
	
	/* no image data? abort */
	pixels = gdk_pixbuf_get_pixels (pixbuf);
	g_return_val_if_fail (pixels != NULL, FALSE);
	
	/* allocate a small buffer to convert image data */
	buf = g_try_malloc (w * bpp * sizeof (guchar));
	if (! buf) {
		g_set_error (error,
			     GDK_PIXBUF_ERROR,
			     GDK_PIXBUF_ERROR_INSUFFICIENT_MEMORY,
			     "Couldn't allocate memory for loading JPEG file");
		return FALSE;
	}

	/* set up error handling */
	cinfo.err = jpeg_std_error (&(jerr.pub));
	jerr.pub.error_exit = fatal_error_handler;
	jerr.pub.output_message = output_message_handler;
	jerr.error = error;

	if (sigsetjmp (jerr.setjmp_buffer, 1)) {
		jpeg_destroy_compress (&cinfo);
		g_free (buf);
		return FALSE;
	}

	/* setup compress params */
	jpeg_create_compress (&cinfo);
	jpeg_stdio_dest (&cinfo, file);
	cinfo.image_width      = w;
	cinfo.image_height     = h;
	cinfo.input_components = 3; 
	cinfo.in_color_space   = JCS_RGB;
	
	/* set up jepg compression parameters */
	jpeg_set_defaults (&cinfo);
	jpeg_set_quality (&cinfo, quality, TRUE);
	cinfo.smoothing_factor = smoothing;
	cinfo.optimize_coding = optimize;

#ifdef HAVE_PROGRESSIVE_JPEG
	if (progressive) 
		jpeg_simple_progression (&cinfo);
#endif /* HAVE_PROGRESSIVE_JPEG */

	jpeg_start_compress (&cinfo, TRUE);
	/* get the start pointer */
	ptr = pixels;
	/* go one scanline at a time... and save */
	i = 0;
	while (cinfo.next_scanline < cinfo.image_height) {
		JSAMPROW *jbuf;

		/* convert scanline from ARGB to RGB packed */
		for (j = 0; j < w; j++)
			memcpy (&(buf[j * 3]), 
				&(ptr[i * rowstride + j * bpp]), 
				3);
		
		/* write scanline */
		jbuf = (JSAMPROW *)(&buf);
		jpeg_write_scanlines (&cinfo, jbuf, 1);
		i++;
	}
	
	/* finish off */
	jpeg_finish_compress (&cinfo);
	fclose (file);

	jpeg_destroy_compress(&cinfo);
	g_free (buf);
	
	return TRUE;
}

#endif





#ifdef HAVE_LIBTIFF

#define  TILE_HEIGHT 40   /* FIXME */

static gboolean
_gdk_pixbuf_save_as_tiff (GdkPixbuf   *pixbuf,
			  const char  *filename,
			  char       **keys,
			  char       **values,
			  GError     **error)
{
	TIFF          *tif;
	int            cols, col, rows, row;
	glong          rowsperstrip;
	gushort        compression;
	/*gushort        extra_samples[1]; FIXME*/
	int            alpha;
	gshort         predictor;
	gshort         photometric;
	gshort         samplesperpixel;
	gshort         bitspersample;
	int            rowstride;
	guchar        *pixels, *buf, *ptr;
	int            success;
	int            horizontal_dpi = 72, vertical_dpi = 72;
	gboolean       save_resolution = FALSE;

	compression = COMPRESSION_DEFLATE;

	if (keys && *keys) {
		char **kiter = keys;
		char **viter = values;
		
		while (*kiter) {
			if (strcmp (*kiter, "compression") == 0) {
				if (*viter == NULL) {
					g_set_error (error,
						     GDK_PIXBUF_ERROR,
						     GDK_PIXBUF_ERROR_BAD_OPTION, 
						     "Must specify a compression type");
					return FALSE;
				}

				if (strcmp (*viter, "none") == 0)
					compression = COMPRESSION_NONE;
				else if (strcmp (*viter, "pack bits") == 0)
					compression = COMPRESSION_PACKBITS;
				else if (strcmp (*viter, "lzw") == 0)
					compression = COMPRESSION_LZW;
				else if (strcmp (*viter, "deflate") == 0)
					compression = COMPRESSION_DEFLATE;
				else if (strcmp (*viter, "jpeg") == 0)
					compression = COMPRESSION_JPEG;
				else {
					g_set_error (error,
						     GDK_PIXBUF_ERROR,
						     GDK_PIXBUF_ERROR_BAD_OPTION,       
						     "Unsupported compression type passed to the TIFF saver");
					return FALSE;
				}
				
			} else if (strcmp (*kiter, "vertical dpi") == 0) {
				char *endptr = NULL;
				vertical_dpi = strtol (*viter, &endptr, 10);
				save_resolution = TRUE;

				if (endptr == *viter) {
					g_set_error (error,
						     GDK_PIXBUF_ERROR,
						     GDK_PIXBUF_ERROR_BAD_OPTION,
						     "TIFF vertical dpi must be a value greater than 0; value '%s' could not be parsed.",
						     *viter);
					
					return FALSE;
				}
				
				if (vertical_dpi < 0) {
					g_set_error (error,
						     GDK_PIXBUF_ERROR,
						     GDK_PIXBUF_ERROR_BAD_OPTION,
						     "TIFF vertical dpi must be a value greater than 0; value '%d' is not allowed.",
						     vertical_dpi);
					
					return FALSE;
				}
				
			} else if (strcmp (*kiter, "horizontal dpi") == 0) {
				char *endptr = NULL;
				horizontal_dpi = strtol (*viter, &endptr, 10);
				save_resolution = TRUE;
				
				if (endptr == *viter) {
					g_set_error (error,
						     GDK_PIXBUF_ERROR,
						     GDK_PIXBUF_ERROR_BAD_OPTION,
						     "TIFF horizontal dpi must be a value greater than 0; value '%s' could not be parsed.",
						     *viter);
					
					return FALSE;
				}
				
				if (horizontal_dpi < 0) {
					g_set_error (error,
						     GDK_PIXBUF_ERROR,
						     GDK_PIXBUF_ERROR_BAD_OPTION,
						     "TIFF horizontal dpi must be a value greater than 0; value '%d' is not allowed.",
						     horizontal_dpi);
					
					return FALSE;
				}
			} else {
				g_warning ("Bad option name '%s' passed to the TIFF saver", *kiter);
				return FALSE;
			}
			
			++kiter;
			++viter;
		}
	}
				
	predictor    = 0;
	rowsperstrip = TILE_HEIGHT;

	tif = TIFFOpen (filename, "w");

	if (tif == NULL) {
		g_set_error (error,
                             GDK_PIXBUF_ERROR,
                             GDK_PIXBUF_ERROR_FAILED,
			     "Can't write image to file '%s'", 
			     filename);
		return FALSE;
	}

	cols      = gdk_pixbuf_get_width (pixbuf);
	rows      = gdk_pixbuf_get_height (pixbuf);
	alpha     = gdk_pixbuf_get_has_alpha (pixbuf);
	pixels    = gdk_pixbuf_get_pixels (pixbuf);
	rowstride = gdk_pixbuf_get_rowstride (pixbuf);

	predictor       = 2;
	bitspersample   = 8;
	photometric     = PHOTOMETRIC_RGB;

	if (alpha)
		samplesperpixel = 4;
	else
		samplesperpixel = 3;
	
	/* Set TIFF parameters. */

	TIFFSetField (tif, TIFFTAG_SUBFILETYPE,   0);
	TIFFSetField (tif, TIFFTAG_IMAGEWIDTH,    cols);
	TIFFSetField (tif, TIFFTAG_IMAGELENGTH,   rows);
	TIFFSetField (tif, TIFFTAG_BITSPERSAMPLE, bitspersample);
	TIFFSetField (tif, TIFFTAG_ORIENTATION,   ORIENTATION_TOPLEFT);
	TIFFSetField (tif, TIFFTAG_COMPRESSION,   compression);

	if ((compression == COMPRESSION_LZW
	     || compression == COMPRESSION_DEFLATE)
	    && (predictor != 0)) {
		TIFFSetField (tif, TIFFTAG_PREDICTOR, predictor);
	}

	/* FIXME: alpha in a TIFF ?
	if (alpha) {
		extra_samples [0] = EXTRASAMPLE_ASSOCALPHA;
		TIFFSetField (tif, TIFFTAG_EXTRASAMPLES, 1, extra_samples);
	}
	*/

	TIFFSetField (tif, TIFFTAG_PHOTOMETRIC,     photometric);
	TIFFSetField (tif, TIFFTAG_DOCUMENTNAME,    filename);
	TIFFSetField (tif, TIFFTAG_SAMPLESPERPIXEL, 3 /*samplesperpixel*/);
	TIFFSetField (tif, TIFFTAG_ROWSPERSTRIP,    rowsperstrip);
	TIFFSetField (tif, TIFFTAG_PLANARCONFIG,    PLANARCONFIG_CONTIG);

	if (save_resolution) {
		TIFFSetField (tif, TIFFTAG_XRESOLUTION, (double) horizontal_dpi);
		TIFFSetField (tif, TIFFTAG_YRESOLUTION, (double) vertical_dpi);
		TIFFSetField (tif, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH);
	}

	/* allocate a small buffer to convert image data */
	buf = g_try_malloc (cols * 3 /*samplesperpixel*/ * sizeof (guchar));
	if (! buf) {
		g_set_error (error,
                             GDK_PIXBUF_ERROR,
                             GDK_PIXBUF_ERROR_INSUFFICIENT_MEMORY,
			     "Couldn't allocate memory for writing TIFF file '%s'",
			     filename);
		return FALSE;
	}

	ptr = pixels;

	/* Now write the TIFF data. */
	for (row = 0; row < rows; row++) {
		/* convert scanline from ARGB to RGB packed */
		for (col = 0; col < cols; col++)
			memcpy (&(buf[col * 3]), &(ptr[col * samplesperpixel /*3*/]), 3);
		
		success = TIFFWriteScanline (tif, buf, row, 0) >= 0;
		
		if (! success) {
			g_set_error (error,
				     GDK_PIXBUF_ERROR,
				     GDK_PIXBUF_ERROR_FAILED,
				     "TIFF Failed a scanline write on row %d",
				     row);
			return FALSE;
		}

		ptr += rowstride;
	}
	
	TIFFFlushData (tif);
	TIFFClose (tif);

	g_free (buf);
	
	return TRUE;
}

#endif





/* TRUEVISION-XFILE magic signature string */
static guchar magic[18] = {
	0x54, 0x52, 0x55, 0x45, 0x56, 0x49, 0x53, 0x49, 0x4f,
	0x4e, 0x2d, 0x58, 0x46, 0x49, 0x4c, 0x45, 0x2e, 0x0
};


static void
rle_write (FILE   *fp,
	   guchar *buffer,
	   guint   width,
	   guint   bytes)
{
	int     repeat = 0;
	int     direct = 0;
	guchar *from   = buffer;
	guint   x;
	
	for (x = 1; x < width; ++x) {
		if (memcmp (buffer, buffer + bytes, bytes)) {
			/* next pixel is different */
			if (repeat) {
				putc (128 + repeat, fp);
				fwrite (from, bytes, 1, fp);
				from = buffer + bytes; /* point to first different pixel */
				repeat = 0;
				direct = 0;
			} else
				direct += 1;
			
		} else {
			/* next pixel is the same */
			if (direct) {
				putc (direct - 1, fp);
				fwrite (from, bytes, direct, fp);
				from = buffer; /* point to first identical pixel */
				direct = 0;
				repeat = 1;
			} else 
				repeat += 1;
		}

		if (repeat == 128) {
			putc (255, fp);
			fwrite (from, bytes, 1, fp);
			from = buffer + bytes;
			direct = 0;
			repeat = 0;
		} else if (direct == 128) {
			putc (127, fp);
			fwrite (from, bytes, direct, fp);
			from = buffer + bytes;
			direct = 0;
			repeat = 0;
		}
		
		buffer += bytes;
	}
	
	if (repeat > 0) {
		putc (128 + repeat, fp);
		fwrite (from, bytes, 1, fp);
	} else {
		putc (direct, fp);
		fwrite (from, bytes, direct + 1, fp);
	}
}


static void
bgr2rgb (guchar *dest,
	 guchar *src,
	 guint   width,
	 guint   bytes,
	 guint   alpha)
{
	guint x;
	
	if (alpha) 
		for (x = 0; x < width; x++) {
			*(dest++) = src[2];
			*(dest++) = src[1];
			*(dest++) = src[0];
			*(dest++) = src[3];
			
			src += bytes;
		}
	else 
		for (x = 0; x < width; x++) {
			*(dest++) = src[2];
			*(dest++) = src[1];
			*(dest++) = src[0];
			
			src += bytes;
		}
}


static gboolean
_gdk_pixbuf_save_as_tga (GdkPixbuf   *pixbuf,
			 const char  *filename,
			 char       **keys,
			 char       **values,
			 GError     **error)
{
	FILE      *fp;
	int        out_bpp = 0;
	int        row;
	guchar     header[18];
	guchar     footer[26];
	gboolean   rle_compression;
	gboolean   alpha;
	guchar    *pixels, *ptr, *buf;
	int        width, height;
	int        rowstride;

	rle_compression = TRUE;

	if (keys && *keys) {
		char **kiter = keys;
		char **viter = values;
		
		while (*kiter) {
			if (strcmp (*kiter, "compression") == 0) {
				if (*viter == NULL) {
					g_set_error (error,
						     GDK_PIXBUF_ERROR,
						     GDK_PIXBUF_ERROR_BAD_OPTION,   
						     "Must specify a compression type");
					return FALSE;
				}

				if (strcmp (*viter, "none") == 0)
					rle_compression = FALSE;

				else if (strcmp (*viter, "rle") == 0)
					rle_compression = TRUE;

				else {
					g_set_error (error,
						     GDK_PIXBUF_ERROR,
						     GDK_PIXBUF_ERROR_BAD_OPTION,    
						     "Unsupported compression type passed to the TGA saver");
					return FALSE;
				}
			} else {
				g_warning ("Bad option name '%s' passed to the TGA saver", *kiter);
				return FALSE;
			}
			
			++kiter;
			++viter;
		}
	}

	width     = gdk_pixbuf_get_width (pixbuf);
	height    = gdk_pixbuf_get_height (pixbuf);
	alpha     = gdk_pixbuf_get_has_alpha (pixbuf);
	pixels    = gdk_pixbuf_get_pixels (pixbuf);
	rowstride = gdk_pixbuf_get_rowstride (pixbuf);

	if ((fp = fopen (filename, "wb")) == NULL) {
		g_set_error (error,
                             GDK_PIXBUF_ERROR,
                             GDK_PIXBUF_ERROR_FAILED,
			     "Can't write image to file '%s'", 
			     filename);
		return FALSE;
	}
	
	header[0] = 0; /* No image identifier / description */

	header[1]= 0;
	header[2]= rle_compression ? 10 : 2;
	header[3] = header[4] = header[5] = header[6] = header[7] = 0;

	header[8]  = header[9]  = 0; /* xorigin */
	header[10] = header[11] = 0; /* yorigin */

	header[12] = width % 256;
	header[13] = width / 256;

	header[14] = height % 256;
	header[15] = height / 256;

	if (alpha) {
		out_bpp = 4;
		header[16] = 32; /* bpp */
		header[17] = 0x28; /* alpha + orientation */
	} else {
		out_bpp = 3;
		header[16] = 24; /* bpp */
		header[17] = 0x20; /* alpha + orientation */
	}

	/* write header to front of file */
	fwrite (header, sizeof (header), 1, fp);

	/* allocate a small buffer to convert image data */
	buf = g_try_malloc (width * out_bpp * sizeof (guchar));
	if (! buf) {
		g_set_error (error,
                             GDK_PIXBUF_ERROR,
                             GDK_PIXBUF_ERROR_INSUFFICIENT_MEMORY,
			     "Couldn't allocate memory for writing TGA file '%s'",
			     filename);
		return FALSE;
	}

	ptr = pixels;

	for (row = 0; row < height; ++row) {
		bgr2rgb (buf, ptr, width, out_bpp, alpha);

		if (rle_compression) 
			rle_write (fp, buf, width, out_bpp);
		else
			fwrite (buf, width * out_bpp, 1, fp);

		ptr += rowstride;
	}

	g_free (buf);

	/* footer must be the last thing written to file */
	memset (footer, 0, 8); /* No extensions, no developer directory */
	memcpy (footer + 8, magic, sizeof (magic)); /* magic signature */
	fwrite (footer, sizeof (footer), 1, fp);
	
	fclose (fp);
	
	return TRUE;
}


/**/


gboolean
_gdk_pixbuf_savev (GdkPixbuf    *pixbuf,
		   const char   *filename,
		   const char   *type,
		   char        **keys,
		   char        **values,
		   GError      **error)
{
	gboolean   result;

	g_return_val_if_fail (pixbuf != NULL, TRUE);
	g_return_val_if_fail (filename != NULL, TRUE);
	g_return_val_if_fail (type != NULL, TRUE);

#ifdef HAVE_LIBTIFF
	if (strcmp (type, "tiff") == 0) 
		result = _gdk_pixbuf_save_as_tiff (pixbuf, 
						   filename, 
						   keys, values, 
						   error);
	else
#endif
#ifdef HAVE_LIBJPEG
	if (strcmp (type, "jpeg") == 0) 
		result = _gdk_pixbuf_save_as_jpeg (pixbuf, 
						   filename, 
						   keys, values, 
						   error);
	else
#endif
	if ((strcmp (type, "x-tga") == 0) || (strcmp (type, "tga") == 0))
		result = _gdk_pixbuf_save_as_tga (pixbuf, 
						  filename, 
						  keys, values, 
						  error);
	else
		result = gdk_pixbuf_savev (pixbuf, filename, type,
					   keys, values,
					   error);

	return result;
}


static void
collect_save_options (va_list   opts,
		      char    ***keys,
		      char    ***vals)
{
	char *key;
	char *val;
	char *next;
	int   count;

	count = 0;
	*keys = NULL;
	*vals = NULL;
	
	next = va_arg (opts, gchar*);
	while (next) {
		key = next;
		val = va_arg (opts, gchar*);

		++count;

		/* woo, slow */
		*keys = g_realloc (*keys, sizeof(char*) * (count + 1));
		*vals = g_realloc (*vals, sizeof(char*) * (count + 1));
		
		(*keys)[count-1] = g_strdup (key);
		(*vals)[count-1] = g_strdup (val);

		(*keys)[count] = NULL;
		(*vals)[count] = NULL;
		
		next = va_arg (opts, gchar*);
	}
}


gboolean
_gdk_pixbuf_save (GdkPixbuf    *pixbuf,
		  const char   *filename,
		  const char   *type,
		  GError      **error,
		  ...)
{
	va_list    args;
	char     **keys = NULL;
	char     **values = NULL;
	gboolean   result;

	g_return_val_if_fail (pixbuf != NULL, TRUE);
	g_return_val_if_fail (filename != NULL, TRUE);
	g_return_val_if_fail (type != NULL, TRUE);

	va_start (args, error);
	collect_save_options (args, &keys, &values);
	va_end (args);

	result = _gdk_pixbuf_savev (pixbuf, filename, type,
				    keys, values,
				    error);
	
	g_strfreev (keys);
	g_strfreev (values);

	return result;
}


gboolean
scale_keepping_ratio (int *width,
		      int *height,
		      int  max_width,
		      int  max_height)
{
	double   w = *width;
	double   h = *height;
	double   max_w = max_width;
	double   max_h = max_height;
	double   factor;
	int      new_width, new_height;
	gboolean modified;

	if ((*width < max_width) && (*height < max_height)) 
		return FALSE;

	factor = MIN (max_w / w, max_h / h);
	new_width  = MAX ((int) floor (w * factor + 0.50), 1);
	new_height = MAX ((int) floor (h * factor + 0.50), 1);
	
	modified = (new_width != *width) || (new_height != *height);

	*width = new_width;
	*height = new_height;

	return modified;
}

