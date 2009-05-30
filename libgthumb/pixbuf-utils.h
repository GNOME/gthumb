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

#ifndef PIXBUF_UTILS_H
#define PIXBUF_UTILS_H

#include <glib.h>
#include <gdk/gdk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "typedefs.h"

void       _gdk_pixbuf_vertical_gradient      (GdkPixbuf       *pixbuf, 
					       guint32          color1,
					       guint32          color2);
void       _gdk_pixbuf_horizontal_gradient    (GdkPixbuf       *pixbuf, 
					       guint32          color1,
					       guint32          color2);
void       _gdk_pixbuf_hv_gradient            (GdkPixbuf       *pixbuf, 
					       guint32          hcolor1,
					       guint32          hcolor2,
					       guint32          vcolor1,
					       guint32          vcolor2);
GdkPixbuf *_gdk_pixbuf_transform              (GdkPixbuf       *src, 
					       GthTransform     transform);
gboolean   _gdk_pixbuf_save                   (GdkPixbuf       *pixbuf,
					       const char      *filename,
					       const char      *original_local_file,
					       const char      *type,
					       GError         **error,
					       ...);
gboolean   _gdk_pixbuf_savev                  (GdkPixbuf       *pixbuf,
					       const char      *filename,
					       const char      *original_local_file,
					       const char      *type,
					       char           **keys,
					       char           **values,
					       GError         **error);
gboolean   scale_keeping_ratio_min	      (int      *width,
					       int      *height,
					       int       min_width,
					       int       min_height,
					       int       max_width,
					       int       max_height,
					       gboolean  allow_upscaling);
gboolean   scale_keeping_ratio                (int             *width,
					       int             *height,
					       int              max_width,
					       int              max_height,
					       gboolean         allow_upscaling);
GdkPixbuf* create_void_pixbuf                 (int              width,
					       int              height);
GdkPixbuf* _gdk_pixbuf_scale_simple_safe      (const GdkPixbuf *src,
                                               int              dest_width,
                                               int              dest_height,
                                               GdkInterpType    interp_type);

#endif
