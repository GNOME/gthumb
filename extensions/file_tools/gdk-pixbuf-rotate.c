/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2011 The Free Software Foundation, Inc.
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

#include <gthumb.h>
#include "gdk-pixbuf-rotate.h"


GdkPixbuf*
_gdk_pixbuf_rotate_crop (GdkPixbuf *src_pixbuf,
			 int        center_x,
			 int        center_y,
			 double     angle)
{
	// TODO: implement the algorithm
	
	g_object_ref (src_pixbuf);
	return src_pixbuf;
}


GdkPixbuf*
_gdk_pixbuf_rotate_no_crop (GdkPixbuf *src_pixbuf,
			    int        center_x,
			    int        center_y,
			    double     angle)
{
	// TODO: implement the algorithm
	
	g_object_ref (src_pixbuf);
	return src_pixbuf;
}


GdkPixbuf*
_gdk_pixbuf_rotate (GdkPixbuf *src_pixbuf,
		    int        center_x,
		    int        center_y,
		    double     angle,
		    gint       auto_crop)
{
	GdkPixbuf *new_pixbuf;
	
	if (angle == 0.0) {
		new_pixbuf = src_pixbuf;
		g_object_ref (new_pixbuf);
	}
	else if (auto_crop) {
		new_pixbuf = _gdk_pixbuf_rotate_crop (src_pixbuf, center_x, center_y, angle);
	}
	else {
		new_pixbuf = _gdk_pixbuf_rotate_no_crop (src_pixbuf, center_x, center_y, angle);
	}

	return new_pixbuf;
}
