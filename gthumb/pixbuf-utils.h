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

#ifndef PIXBUF_UTILS_H
#define PIXBUF_UTILS_H

#include <glib.h>
#include <gdk/gdk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <cairo.h>
#include "typedefs.h"

G_BEGIN_DECLS

GdkPixbuf * _gdk_pixbuf_new_from_cairo_context   (cairo_t         *cr);
GdkPixbuf * _gdk_pixbuf_new_from_cairo_surface   (cairo_surface_t *surface);
GdkPixbuf * _gdk_pixbuf_scale_simple_safe        (const GdkPixbuf *src,
					          int              dest_width,
					          int              dest_height,
					          GdkInterpType    interp_type);
GdkPixbuf * _gdk_pixbuf_scale_composite          (const GdkPixbuf *src,
			     	     	          int              dest_width,
			     	     	          int              dest_height,
			     	     	          GdkInterpType    interp_type);
GdkPixbuf * _gdk_pixbuf_transform                (GdkPixbuf       *src,
					          GthTransform     transform);
void        _gdk_pixbuf_colorshift               (GdkPixbuf       *dest,
					          GdkPixbuf       *src,
					          int              shift);
char *      _gdk_pixbuf_get_type_from_mime_type  (const char      *mime_type);
gboolean    scale_keeping_ratio_min              (int             *width,
					          int             *height,
					          int              min_width,
					          int              min_height,
					          int              max_width,
					          int              max_height,
					          gboolean         allow_upscaling);
gboolean    scale_keeping_ratio                  (int             *width,
					          int             *height,
					          int              max_width,
					          int              max_height,
					          gboolean         allow_upscaling);

G_END_DECLS

#endif
