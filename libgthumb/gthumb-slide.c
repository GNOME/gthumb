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


#include <gdk/gdk.h>

#define FRAME_WIDTH         3
#define FRAME_SHADOW_OFFSET 4
#define IMAGE_SHADOW_OFFSET 8
#define INOUT_SHADOW_OFFSET 3


void
gthumb_draw_slide (int          slide_x, 
		   int          slide_y,
		   int          slide_w,
		   int          slide_h,
		   int          image_w,
		   int          image_h,
		   GdkDrawable *drawable,
		   GdkGC       *gc,
		   GdkGC       *black_gc,
		   GdkGC       *dark_gc,
		   GdkGC       *mid_gc,
		   GdkGC       *light_gc)
{
	GdkGC    *white_gc;
	GdkColor  white_color;
	int       slide_x2, slide_y2;

	white_gc = gdk_gc_new (drawable);
	gdk_color_parse ("#FFFFFF", &white_color);
	gdk_gc_set_rgb_fg_color (white_gc, &white_color);

	slide_x2 = slide_x + slide_w;
	slide_y2 = slide_y + slide_h;

	if ((image_w > 0) && (image_h > 0)) {
		int image_x, image_y;
		int image_x2, image_y2;
		
		image_x = slide_x + (slide_w - image_w) / 2 + 1;
		image_y = slide_y + (slide_h - image_h) / 2 + 1;
		
		/* background. */
		
		gdk_draw_rectangle (drawable,
				    gc,
				    TRUE,
				    slide_x,
				    slide_y,
				    slide_w,
				    image_y - slide_y);
		gdk_draw_rectangle (drawable,
				    gc,
				    TRUE,
				    slide_x,
				    image_y + image_h - 1,
				    slide_w,
				    image_y - slide_y);
		gdk_draw_rectangle (drawable,
				    gc,
				    TRUE,
				    slide_x,
				    slide_y,
				    image_x - slide_x,
				    slide_h);
		gdk_draw_rectangle (drawable,
				    gc,
				    TRUE,
				    image_x + image_w - 1,
				    slide_y,
				    image_x - slide_x,
				    slide_h);

		/* Inner border. */

		image_x2 = image_x + image_w + 1;
		image_y2 = image_y + image_h + 1;
		image_x--;
		image_y--;

		gdk_draw_rectangle (drawable,
				    white_gc,
				    TRUE,
				    image_x,
				    image_y,
				    image_w,
				    image_h);

		gdk_draw_line (drawable,
			       black_gc,
			       image_x,
			       image_y,
			       image_x2 - 1,
			       image_y);
		gdk_draw_line (drawable,
			       black_gc,
			       image_x,
			       image_y,
			       image_x,
			       image_y2 - 1);
		gdk_draw_line (drawable,
			       mid_gc,
			       image_x2 - 1,
			       image_y,
			       image_x2 - 1,
			       image_y2 - 1);
		gdk_draw_line (drawable,
			       mid_gc,
			       image_x,
			       image_y2 - 1,
			       image_x2 - 1,
			       image_y2 - 1);

		gdk_draw_line (drawable,
			       dark_gc,
			       image_x - 1,
			       image_y - 1,
			       image_x2 - 1,
			       image_y - 1);
		gdk_draw_line (drawable,
			       dark_gc,
			       image_x - 1,
			       image_y - 1,
			       image_x - 1,
			       image_y2 - 1);
		gdk_draw_line (drawable,
			       light_gc,
			       image_x2,
			       image_y - 1,
			       image_x2,
			       image_y2);
		gdk_draw_line (drawable,
			       light_gc,
			       image_x - 1,
			       image_y2,
			       image_x2,
			       image_y2);
	} else
		gdk_draw_rectangle (drawable,
				    gc,
				    TRUE,
				    slide_x,
				    slide_y,
				    slide_w,
				    slide_h);

	/* Outter border. */

	gdk_draw_line (drawable,
		       mid_gc,
		       slide_x,
		       slide_y,
		       slide_x2,
		       slide_y);
	gdk_draw_line (drawable,
		       mid_gc,
		       slide_x,
		       slide_y,
		       slide_x,
		       slide_y2);
	gdk_draw_line (drawable,
		       black_gc,
		       slide_x2,
		       slide_y,
		       slide_x2,
		       slide_y2);
	gdk_draw_line (drawable,
		       black_gc,
		       slide_x,
		       slide_y2,
		       slide_x2,
		       slide_y2);

	gdk_draw_line (drawable,
		       light_gc,
		       slide_x + 1,
		       slide_y + 1,
		       slide_x2 - 1,
		       slide_y + 1);
	gdk_draw_line (drawable,
		       light_gc,
		       slide_x + 1,
		       slide_y + 1,
		       slide_x + 1,
		       slide_y2 - 1);
	gdk_draw_line (drawable,
		       dark_gc,
		       slide_x2 - 1,
		       slide_y + 1,
		       slide_x2 - 1,
		       slide_y2 - 1);
	gdk_draw_line (drawable,
		       dark_gc,
		       slide_x + 1,
		       slide_y2 - 1,
		       slide_x2 - 1,
		       slide_y2 - 1);

	g_object_unref (white_gc);
}


void
gthumb_draw_slide_with_colors (int          slide_x, 
			       int          slide_y,
			       int          slide_w,
			       int          slide_h,
			       int          image_w,
			       int          image_h,
			       GdkDrawable *drawable,
			       GdkColor    *slide_color,
			       GdkColor    *black_color,
			       GdkColor    *dark_color,
			       GdkColor    *mid_color,
			       GdkColor    *light_color)
{
	GdkGC *slide_gc;
	GdkGC *black_gc;
	GdkGC *dark_gc;
	GdkGC *mid_gc;
	GdkGC *light_gc;

	slide_gc = gdk_gc_new (drawable);
	black_gc = gdk_gc_new (drawable);
	dark_gc = gdk_gc_new (drawable);
	mid_gc = gdk_gc_new (drawable);
	light_gc = gdk_gc_new (drawable);

	gdk_gc_set_rgb_fg_color (slide_gc, slide_color);
	gdk_gc_set_rgb_fg_color (black_gc, black_color);
	gdk_gc_set_rgb_fg_color (dark_gc, dark_color);
	gdk_gc_set_rgb_fg_color (mid_gc, mid_color);
	gdk_gc_set_rgb_fg_color (light_gc, light_color);

	gthumb_draw_slide (slide_x, slide_y,
			   slide_w, slide_h,
			   image_w, image_h,
			   drawable,
			   slide_gc,
			   black_gc,
			   dark_gc,
			   mid_gc,
			   light_gc);	

	g_object_unref (slide_gc);
	g_object_unref (black_gc);
	g_object_unref (dark_gc);
	g_object_unref (mid_gc);
	g_object_unref (light_gc);
}


void
gthumb_draw_frame (int          image_x,
		   int          image_y,
		   int          image_w,
		   int          image_h,
		   GdkDrawable *drawable,
		   GdkColor    *frame_color)
{
	GdkGC    *gc;
	GdkColor  white;
	int       frame_width;

	gc = gdk_gc_new (drawable);

	gdk_color_parse ("#FFFFFF", &white);
	gdk_gc_set_rgb_fg_color (gc, &white);
	gdk_draw_rectangle (drawable,
			    gc,
			    TRUE,
			    image_x, image_y,
			    image_w, image_h);
	
	gdk_gc_set_rgb_fg_color (gc, frame_color); 
	gdk_gc_set_line_attributes (gc, FRAME_WIDTH, 0, 0, 0);

	frame_width = FRAME_WIDTH - 2;
	gdk_draw_rectangle (drawable,
			    gc,
			    FALSE,
			    image_x - frame_width, 
			    image_y - frame_width,
			    image_w + (frame_width * 2) - 1,
			    image_h + (frame_width * 2) - 1);

	g_object_unref (gc);
}


/* ---- */


static void
_gdk_pixbuf_draw_rectangle (GdkPixbuf *pixbuf, 
			    int        offset,
			    guchar     alpha)
{
	guchar   *pixels;
	guchar   *p;
	guint     width, height;
	int       n_channels, rowstride;
	int       w, h;
	
	g_return_if_fail (GDK_IS_PIXBUF (pixbuf));

	width = gdk_pixbuf_get_width (pixbuf);
	height = gdk_pixbuf_get_height (pixbuf);

	if (width == 0 || height == 0)
		return;

	pixels = gdk_pixbuf_get_pixels (pixbuf);
	n_channels = gdk_pixbuf_get_n_channels (pixbuf);
	rowstride = gdk_pixbuf_get_rowstride (pixbuf);

	offset = MIN (offset, width / 2);
	offset = MIN (offset, height / 2);

	width -= offset * 2;
	height -= offset * 2;

	width = MAX (0, width);
	height = MAX (0, height);

	p = pixels + (rowstride * offset) + (offset * n_channels);
	for (w = 0; w <= width; w++) {
		p[0] = 0x00;
		p[1] = 0x00;
		p[2] = 0x00;
		p[3] = alpha;
		p += n_channels;
	}

	p = pixels + (rowstride * (height + offset)) + (offset * n_channels);
	for (w = 0; w <= width; w++) {
		p[0] = 0x00;
		p[1] = 0x00;
		p[2] = 0x00;
		p[3] = alpha;
		p += n_channels;
	}

	p = pixels + (rowstride * offset) + (offset * n_channels);
	for (h = offset; h <= height + offset; h++) {
		p[0] = 0x00;
		p[1] = 0x00;
		p[2] = 0x00;
		p[3] = alpha;
		p += rowstride;
	}

	p = pixels + (rowstride * offset) + ((offset + width) * n_channels);
	for (h = offset; h <= height + offset; h++) {
		p[0] = 0x00;
		p[1] = 0x00;
		p[2] = 0x00;
		p[3] = alpha;
		p += rowstride;
	}
}


static void
draw_shadow (GdkPixmap *pixmap, int x, int y, int width, int height)
{
	GdkPixbuf *shadow;
	int        i, alpha = 0, max;

	shadow = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, width, height);

	gdk_pixbuf_fill (shadow, 0x00000000);

	max = MAX (width / 2, height / 2);
	for (i = 1; i < max; i++) {
		_gdk_pixbuf_draw_rectangle (shadow, i, alpha);
		alpha += 12;
		alpha = MIN (alpha, 255);
	}

	gdk_pixbuf_render_to_drawable_alpha (shadow,
					     pixmap,
					     0, 0,
					     x, y,
					     width, height,
					     GDK_PIXBUF_ALPHA_FULL, 
					     255,
					     GDK_RGB_DITHER_MAX, 0, 0);

	g_object_unref (shadow);
}


void
gthumb_draw_frame_shadow (int          image_x,
			  int          image_y,
			  int          image_w,
			  int          image_h,
			  GdkDrawable *drawable)
{
	image_x += FRAME_SHADOW_OFFSET;
	image_y += FRAME_SHADOW_OFFSET;
	image_w += FRAME_WIDTH * 2;
	image_h += FRAME_WIDTH * 2;

	draw_shadow (drawable, image_x, image_y, image_w, image_h);
}


void
gthumb_draw_image_shadow (int          image_x,
			  int          image_y,
			  int          image_w,
			  int          image_h,
			  GdkDrawable *drawable)
{
	image_x += IMAGE_SHADOW_OFFSET;
	image_y += IMAGE_SHADOW_OFFSET;
	image_w += 1;
	image_h += 1;

	draw_shadow (drawable, image_x, image_y, image_w, image_h);
}


static void
_gdk_pixbuf_fill_triangle (GdkPixbuf *shadow, 
			   guint32    color1,
			   guint32    color2)
{
	guint32   r, g, b, a;
	guint32   r1, g1, b1, a1;
	guint32   r2, g2, b2, a2;
	int       i, j;
	int       w, h;
	double    x, dx;
	int       n_channels, rowstride;
	guchar   *p;
	guchar   *pixels;

	r1 = (color1 & 0xff000000) >> 24;
	g1 = (color1 & 0x00ff0000) >> 16;
	b1 = (color1 & 0x0000ff00) >> 8;
	a1 = (color1 & 0x000000ff);

	r2 = (color2 & 0xff000000) >> 24;
	g2 = (color2 & 0x00ff0000) >> 16;
	b2 = (color2 & 0x0000ff00) >> 8;
	a2 = (color2 & 0x000000ff);

	w = gdk_pixbuf_get_width (shadow);
	h = gdk_pixbuf_get_height (shadow);
	n_channels = gdk_pixbuf_get_n_channels (shadow);
	rowstride = gdk_pixbuf_get_rowstride (shadow);
	pixels = gdk_pixbuf_get_pixels (shadow);

	dx = ((double) w) / h;
	x  = w;

	for (j = 0; j < h; j++) {
		p = pixels;

		for (i = 0; i < w; i++) {
			if (i < (int) x) {
				r = r1;	
				g = g1;	
				b = b1;	
				a = a1;	
			} else {
				r = r2;	
				g = g2;	
				b = b2;	
				a = a2;	
			}

			p[0] = r; 
			p[1] = g; 
			p[2] = b;

			switch (n_channels) {
			case 3:
				p += 3;
				break;
			case 4:
				p[3] = a;
				p += 4;
				break;
			default:
				break;
			}
		}

		x -= dx;
		pixels += rowstride;
	}
}


void
gthumb_draw_image_shadow_in (int          image_x,
			     int          image_y,
			     int          image_w,
			     int          image_h,
			     GdkDrawable *drawable)
{
	GdkPixbuf *shadow;

	shadow = gdk_pixbuf_new (GDK_COLORSPACE_RGB, 1, 8, image_w, image_h);

	gdk_pixbuf_fill (shadow,  0x00000080);
	gdk_pixbuf_render_to_drawable_alpha (shadow,
					     drawable,
					     0, 0,
					     image_x - INOUT_SHADOW_OFFSET,
					     image_y - INOUT_SHADOW_OFFSET,
					     image_w, image_h,
					     GDK_PIXBUF_ALPHA_FULL, 
					     255,
					     GDK_RGB_DITHER_MAX, 0, 0);

	gdk_pixbuf_fill (shadow,  0xFFFFFF80);
	gdk_pixbuf_render_to_drawable_alpha (shadow,
					     drawable,
					     0, 0,
					     image_x + INOUT_SHADOW_OFFSET,
					     image_y + INOUT_SHADOW_OFFSET,
					     image_w, image_h,
					     GDK_PIXBUF_ALPHA_FULL, 
					     255,
					     GDK_RGB_DITHER_MAX, 0, 0);

	g_object_unref (shadow);
	
	/**/

	shadow = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, 
				 INOUT_SHADOW_OFFSET * 2, 
				 INOUT_SHADOW_OFFSET * 2);

	_gdk_pixbuf_fill_triangle (shadow, 0x00000080, 0xFFFFFF80);
	
	gdk_pixbuf_render_to_drawable_alpha (shadow,
					     drawable,
					     0, 0,
					     image_x + image_w - INOUT_SHADOW_OFFSET,
					     image_y - INOUT_SHADOW_OFFSET,
					     INOUT_SHADOW_OFFSET * 2, 
					     INOUT_SHADOW_OFFSET * 2,
					     GDK_PIXBUF_ALPHA_FULL, 
					     255,
					     GDK_RGB_DITHER_MAX, 0, 0);

	gdk_pixbuf_render_to_drawable_alpha (shadow,
					     drawable,
					     0, 0,
					     image_x - INOUT_SHADOW_OFFSET,
					     image_y + image_h - INOUT_SHADOW_OFFSET,
					     INOUT_SHADOW_OFFSET * 2, 
					     INOUT_SHADOW_OFFSET * 2,
					     GDK_PIXBUF_ALPHA_FULL, 
					     255,
					     GDK_RGB_DITHER_MAX, 0, 0);


	g_object_unref (shadow);
}


void
gthumb_draw_image_shadow_out (int          image_x,
			      int          image_y,
			      int          image_w,
			      int          image_h,
			      GdkDrawable *drawable)
{
	GdkPixbuf *shadow;

	shadow = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, image_w, image_h);

	gdk_pixbuf_fill (shadow,  0xFFFFFF80);
	gdk_pixbuf_render_to_drawable_alpha (shadow,
					     drawable,
					     0, 0,
					     image_x - INOUT_SHADOW_OFFSET,
					     image_y - INOUT_SHADOW_OFFSET,
					     image_w, image_h,
					     GDK_PIXBUF_ALPHA_FULL, 
					     255,
					     GDK_RGB_DITHER_MAX, 0, 0);

	gdk_pixbuf_fill (shadow,  0x00000080);
	gdk_pixbuf_render_to_drawable_alpha (shadow,
					     drawable,
					     0, 0,
					     image_x + INOUT_SHADOW_OFFSET,
					     image_y + INOUT_SHADOW_OFFSET,
					     image_w, image_h,
					     GDK_PIXBUF_ALPHA_FULL, 
					     255,
					     GDK_RGB_DITHER_MAX, 0, 0);

	g_object_unref (shadow);

	/**/

	shadow = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, 
				 INOUT_SHADOW_OFFSET * 2, 
				 INOUT_SHADOW_OFFSET * 2);

	_gdk_pixbuf_fill_triangle (shadow, 0xFFFFFF80, 0x00000080);
	
	gdk_pixbuf_render_to_drawable_alpha (shadow,
					     drawable,
					     0, 0,
					     image_x + image_w - INOUT_SHADOW_OFFSET,
					     image_y - INOUT_SHADOW_OFFSET,
					     INOUT_SHADOW_OFFSET * 2, 
					     INOUT_SHADOW_OFFSET * 2,
					     GDK_PIXBUF_ALPHA_FULL, 
					     255,
					     GDK_RGB_DITHER_MAX, 0, 0);

	gdk_pixbuf_render_to_drawable_alpha (shadow,
					     drawable,
					     0, 0,
					     image_x - INOUT_SHADOW_OFFSET,
					     image_y + image_h - INOUT_SHADOW_OFFSET,
					     INOUT_SHADOW_OFFSET * 2, 
					     INOUT_SHADOW_OFFSET * 2,
					     GDK_PIXBUF_ALPHA_FULL, 
					     255,
					     GDK_RGB_DITHER_MAX, 0, 0);


	g_object_unref (shadow);
}
