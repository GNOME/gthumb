/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2010 Free Software Foundation, Inc.
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

#ifndef CAIRO_UTILS_H
#define CAIRO_UTILS_H

#include <glib.h>
#include <gdk/gdk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <cairo.h>


#if G_BYTE_ORDER == G_LITTLE_ENDIAN /* BGRA */

#define CAIRO_RED   2
#define CAIRO_GREEN 1
#define CAIRO_BLUE  0
#define CAIRO_ALPHA 3

#elif G_BYTE_ORDER == G_BIG_ENDIAN /* ARGB */

#define CAIRO_RED   1
#define CAIRO_GREEN 2
#define CAIRO_BLUE  3
#define CAIRO_ALPHA 0

#else /* PDP endianness: RABG */

#define CAIRO_RED   0
#define CAIRO_GREEN 3
#define CAIRO_BLUE  2
#define CAIRO_ALPHA 1

#endif

#define CAIRO_SET_RGB(pixel, red, green, blue)					\
	do {									\
		pixel[CAIRO_RED]   = (red);					\
		pixel[CAIRO_GREEN] = (green);					\
		pixel[CAIRO_BLUE]  = (blue);					\
		pixel[CAIRO_ALPHA] = 0xff;					\
	} while (0)

#define CAIRO_SET_RGBA(pixel, red, green, blue, alpha)				\
	do {									\
		gdouble alpha_factor = (gdouble) alpha / 0xff;			\
		pixel[CAIRO_RED]   = (guchar) (alpha_factor * (red) + .5);	\
		pixel[CAIRO_GREEN] = (guchar) (alpha_factor * (green) + .5);	\
		pixel[CAIRO_BLUE]  = (guchar) (alpha_factor * (blue) + .5);	\
		pixel[CAIRO_ALPHA] = (alpha);					\
	} while (0)

#define CAIRO_GET_RGB(pixel, red, green, blue)					\
	do {									\
		red   = pixel[CAIRO_RED];					\
		green = pixel[CAIRO_GREEN];					\
		blue   = pixel[CAIRO_BLUE];					\
	} while (0)

#define CAIRO_GET_RGBA(pixel, red, green, blue, alpha)				\
	do {									\
		gdouble alpha_factor;						\
										\
		alpha = pixel[CAIRO_ALPHA];					\
		alpha_factor = (gdouble) 0xff / alpha;		 		\
		red   = (guchar) (pixel[CAIRO_RED] * alpha_factor + .5);	\
		green = (guchar) (pixel[CAIRO_GREEN] * alpha_factor + .5);	\
		blue  = (guchar) (pixel[CAIRO_BLUE] * alpha_factor + .5);	\
	} while (0)

/* types */

typedef struct {
	double r;
	double g;
	double b;
	double a;
} cairo_color_t;

typedef struct {
	guchar r;
	guchar g;
	guchar b;
	guchar a;
} cairo_color_255_t;

/* colors */

void               _gdk_color_to_cairo_color                (GdkColor          *g_color,
							     cairo_color_t     *c_color);
void               _gdk_color_to_cairo_color_255            (GdkColor          *g_color,
			         	 	 	     cairo_color_255_t *c_color);


/* surface */

void               _cairo_clear_surface                     (cairo_surface_t  **surface);
gboolean           _cairo_image_surface_get_has_alpha       (cairo_surface_t   *surface);
cairo_surface_t *  _cairo_image_surface_create_from_pixbuf  (GdkPixbuf         *pixbuf);
cairo_surface_t *  _cairo_image_surface_scale_to            (cairo_surface_t   *surface,
							     int                width,
							     int                height,
							     cairo_filter_t     filter);

/* paint / draw */

void              _cairo_paint_full_gradient                (cairo_surface_t   *surface,
				 	 	 	     GdkColor          *h_color1,
				 	 	 	     GdkColor          *h_color2,
				 	 	 	     GdkColor          *v_color1,
				 	 	 	     GdkColor          *v_color2);
void              _cairo_draw_rounded_box                   (cairo_t           *cr,
			 	 	 	 	     double             x,
			 	 	 	 	     double             y,
			 	 	 	 	     double             w,
			 	 	 	 	     double             h,
			 	 	 	 	     double             r);
void              _cairo_draw_drop_shadow                   (cairo_t           *cr,
 	 	 	 	 	 	 	     double             x,
 	 	 	 	 	 	 	     double             y,
 	 	 	 	 	 	 	     double             w,
 	 	 	 	 	 	 	     double             h,
 	 	 	 	 	 	 	     double             r);
void              _cairo_draw_frame                         (cairo_t           *cr,
 	 	 	 	 	 	 	     double             x,
 	 	 	 	 	 	 	     double             y,
 	 	 	 	 	 	 	     double             w,
 	 	 	 	 	 	 	     double             h,
 	 	 	 	 	 	 	     double             r);
void              _cairo_draw_slide                         (cairo_t           *cr,
		   	   	   	   	 	     double             frame_x,
		   	   	   	   	 	     double             frame_y,
		   	   	   	   	 	     double             frame_width,
		   	   	   	   	 	     double             frame_height,
		   	   	   	   	 	     double             image_width,
		   	   	   	   	 	     double             image_height,
		   	   	   	   	 	     GdkColor          *frame_color,
		   	   	   	   	 	     gboolean           draw_inner_border);

#endif /* CAIRO_UTILS_H */
