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

#ifndef GTHUMB_SLIDE_H
#define GTHUMB_SLIDE_H

#include <gdk/gdk.h>

void  gthumb_draw_slide             (int          slide_x, 
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
				     GdkGC       *light_gc);

void gthumb_draw_slide_with_colors  (int          slide_x, 
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
				     GdkColor    *light_color);

void gthumb_draw_frame              (int          x,
				     int          y,
				     int          image_w,
				     int          image_h,
				     GdkDrawable *drawable,
				     GdkColor    *frame_color);

void gthumb_draw_frame_shadow       (int          x,
				     int          y,
				     int          image_w,
				     int          image_h,
				     GdkDrawable *drawable);

void gthumb_draw_image_shadow       (int          x,
				     int          y,
				     int          image_w,
				     int          image_h,
				     GdkDrawable *drawable);

void gthumb_draw_image_shadow_in    (int          x,
				     int          y,
				     int          image_w,
				     int          image_h,
				     GdkDrawable *drawable);

void gthumb_draw_image_shadow_out   (int          x,
				     int          y,
				     int          image_w,
				     int          image_h,
				     GdkDrawable *drawable);

#endif /* GTHUMB_SLIDE_H */
