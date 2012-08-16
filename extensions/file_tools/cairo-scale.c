/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2012 The Free Software Foundation, Inc.
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

#include <math.h>
#include <gthumb.h>
#include "cairo-scale.h"


cairo_surface_t *
_cairo_image_surface_scale (cairo_surface_t *image,
			    int              scaled_width,
			    int              scaled_height,
			    gboolean         high_quality)
{
	GdkPixbuf       *p;
	GdkPixbuf       *scaled_p;
	cairo_surface_t *scaled;

	p = _gdk_pixbuf_new_from_cairo_surface (image);
	scaled_p = _gdk_pixbuf_scale_simple_safe (p, scaled_width, scaled_height, high_quality ? GDK_INTERP_BILINEAR : GDK_INTERP_NEAREST);
	scaled = _cairo_image_surface_create_from_pixbuf (scaled_p);

	g_object_unref (scaled_p);
	g_object_unref (p);

	return scaled;
}
