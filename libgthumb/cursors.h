/* Eye of Gnome image viewer - mouse cursors
 *
 * Copyright (C) 2000 The Free Software Foundation
 *
 * Author: Federico Mena-Quintero <federico@gnu.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef CURSORS_H
#define CURSORS_H

#include <gdk/gdk.h>



typedef enum { /*< skip >*/
	CURSOR_VOID,
	CURSOR_HAND_OPEN,
	CURSOR_HAND_CLOSED,
	CURSOR_NUM_CURSORS
} CursorType;

GdkCursor *cursor_get (GdkWindow *window, CursorType type);



#endif
