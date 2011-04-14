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

void         _gdk_color_to_cairo_color          (GdkColor          *g_color,
			         	 	 cairo_color_t     *c_color);
void         _gdk_color_to_cairo_color_255      (GdkColor          *g_color,
			         	 	 cairo_color_255_t *c_color);
void         _cairo_paint_full_gradient         (cairo_surface_t   *surface,
				 	 	 GdkColor          *h_color1,
				 	 	 GdkColor          *h_color2,
				 	 	 GdkColor          *v_color1,
				 	 	 GdkColor          *v_color2);
void         _cairo_clear_surface               (cairo_surface_t  **surface);
cairo_surface_t *
	     _cairo_image_surface_create_from_pixbuf
	     	     	     	     	        (GdkPixbuf         *pixbuf);
void         _cairo_draw_rounded_box            (cairo_t           *cr,
			 	 	 	 double             x,
			 	 	 	 double             y,
			 	 	 	 double             w,
			 	 	 	 double             h,
			 	 	 	 double             r);
void         _cairo_draw_drop_shadow            (cairo_t           *cr,
 	 	 	 	 	 	 double             x,
 	 	 	 	 	 	 double             y,
 	 	 	 	 	 	 double             w,
 	 	 	 	 	 	 double             h,
 	 	 	 	 	 	 double             r);
void         _cairo_draw_frame                  (cairo_t           *cr,
 	 	 	 	 	 	 double             x,
 	 	 	 	 	 	 double             y,
 	 	 	 	 	 	 double             w,
 	 	 	 	 	 	 double             h,
 	 	 	 	 	 	 double             r);
void         _cairo_draw_slide                  (cairo_t           *cr,
		   	   	   	   	 double             frame_x,
		   	   	   	   	 double             frame_y,
		   	   	   	   	 double             frame_width,
		   	   	   	   	 double             frame_height,
		   	   	   	   	 double             image_width,
		   	   	   	   	 double             image_height,
		   	   	   	   	 GdkColor          *frame_color,
		   	   	   	   	 gboolean           draw_inner_border);

#endif /* CAIRO_UTILS_H */
