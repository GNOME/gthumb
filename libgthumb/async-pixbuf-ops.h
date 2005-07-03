/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001, 2003 Free Software Foundation, Inc.
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

#ifndef ASYNC_PIXBUF_OPS_H
#define ASYNC_PIXBUF_OPS_H

#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "gth-pixbuf-op.h"

typedef enum { /*< skip >*/
	GTH_DITHER_BLACK_WHITE,
	GTH_DITHER_WEB_PALETTE
} GthDither;


GthPixbufOp* _gdk_pixbuf_desaturate               (GdkPixbuf *src,
						   GdkPixbuf *dest);
GthPixbufOp* _gdk_pixbuf_invert                   (GdkPixbuf *src,
						   GdkPixbuf *dest);
GthPixbufOp* _gdk_pixbuf_brightness_contrast      (GdkPixbuf *src,
						   GdkPixbuf *dest,
						   double     brightness,
						   double     contrast);
GthPixbufOp* _gdk_pixbuf_posterize                (GdkPixbuf *src,
						   GdkPixbuf *dest, 
						   int        levels);
GthPixbufOp* _gdk_pixbuf_hue_lightness_saturation (GdkPixbuf *src,
						   GdkPixbuf *dest,
						   double     hue,
						   double     lightness,
						   double     saturation);
GthPixbufOp* _gdk_pixbuf_color_balance            (GdkPixbuf *src,
						   GdkPixbuf *dest,
						   double     cyan_red,
						   double     magenta_green,
						   double     yellow_blue,
						   gboolean   preserve_luminosity);
GthPixbufOp* _gdk_pixbuf_eq_histogram             (GdkPixbuf *src,
						   GdkPixbuf *dest);
GthPixbufOp* _gdk_pixbuf_adjust_levels            (GdkPixbuf *src,
						   GdkPixbuf *dest);
GthPixbufOp* _gdk_pixbuf_stretch_contrast         (GdkPixbuf *src,
						   GdkPixbuf *dest);
GthPixbufOp* _gdk_pixbuf_normalize_contrast       (GdkPixbuf *src,
						   GdkPixbuf *dest);
GthPixbufOp* _gdk_pixbuf_dither                   (GdkPixbuf *src,
						   GdkPixbuf *dest,
						   GthDither  dither_type);

#endif /* ASYNC_PIXBUF_OPS_H */
