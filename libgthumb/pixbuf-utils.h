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

#ifndef PIXBUF_UTILS_H
#define PIXBUF_UTILS_H

#include <glib.h>
#include <gdk/gdk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>


void        pixmap_from_xpm                   (const char **data, 
					       GdkPixmap  **pixmap, 
					       GdkBitmap  **mask);

/* -- */

void       _gdk_pixbuf_vertical_gradient      (GdkPixbuf *pixbuf, 
					       guint32    color1,
					       guint32    color2);

void       _gdk_pixbuf_horizontal_gradient    (GdkPixbuf *pixbuf, 
					       guint32    color1,
					       guint32    color2);

void       _gdk_pixbuf_hv_gradient            (GdkPixbuf *pixbuf, 
					       guint32    hcolor1,
					       guint32    hcolor2,
					       guint32    vcolor1,
					       guint32    vcolor2);

/* -- */

GdkPixbuf *_gdk_pixbuf_copy_rotate_90         (GdkPixbuf *src, 
					       gboolean   counter_clockwise);

GdkPixbuf *_gdk_pixbuf_copy_mirror            (GdkPixbuf *src, 
					       gboolean   mirror, 
					       gboolean   flip);

/* -- */

void       _gdk_pixbuf_desaturate               (const GdkPixbuf *src,
						 GdkPixbuf       *dest);

void       _gdk_pixbuf_invert                   (const GdkPixbuf *src,
						 GdkPixbuf       *dest);

void       _gdk_pixbuf_brightness_contrast      (const GdkPixbuf *src,
						 GdkPixbuf       *dest,
						 double           brightness,
						 double           contrast);

void       _gdk_pixbuf_posterize                (const GdkPixbuf *src,
						 GdkPixbuf       *dest, 
						 int              levels);

void       _gdk_pixbuf_hue_lightness_saturation (const GdkPixbuf *src,
						 GdkPixbuf       *dest,
						 double           hue,
						 double           lightness,
						 double           saturation);

void       _gdk_pixbuf_color_balance           (const GdkPixbuf *src,
						GdkPixbuf       *dest,
						double           cyan_red,
						double           magenta_green,
						double           yellow_blue,
						gboolean         preserve_luminosity);

void       _gdk_pixbuf_eq_histogram            (const GdkPixbuf *src,
						GdkPixbuf       *dest);

void       _gdk_pixbuf_adjust_levels           (const GdkPixbuf *src,
						GdkPixbuf       *dest);

/**/

gboolean   _gdk_pixbuf_save                    (GdkPixbuf       *pixbuf,
						const char      *filename,
						const char      *type,
						GError         **error,
						...);

gboolean   _gdk_pixbuf_savev                   (GdkPixbuf    *pixbuf,
						const char   *filename,
						const char   *type,
						char        **keys,
						char        **values,
						GError      **error);
						
#endif
