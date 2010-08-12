/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001-2008 Free Software Foundation, Inc.
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
 * 
 *  taken from : 
 *
 *  Eye of Gnome image viewer - mouse cursors
 *  Copyright (C) 2000 The Free Software Foundation
 */

#ifndef GTH_CURSORS_H
#define GTH_CURSORS_H

#include <gdk/gdk.h>

G_BEGIN_DECLS

typedef enum { /*< skip >*/
	GTH_CURSOR_VOID,
	GTH_CURSOR_HAND_OPEN,
	GTH_CURSOR_HAND_CLOSED,
	GTH_CURSOR_NUM_CURSORS
} GthCursorType;

GdkCursor *gth_cursor_get (GdkWindow     *window, 
			   GthCursorType  type);

G_END_DECLS

#endif /* GTH_CURSORS_H */ 
