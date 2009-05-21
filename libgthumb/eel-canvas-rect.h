/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* eel-canvas-rect.h: rectangle canvas item with AA support.

   Copyright (C) 2002 Alexander Larsson.

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Authors: Alexander Larsson <alla@lysator.liu.se>
*/

#ifndef EEL_CANVAS_RECT_H
#define EEL_CANVAS_RECT_H

#include <libgnomecanvas/gnome-canvas.h>

G_BEGIN_DECLS

#define EEL_TYPE_CANVAS_RECT            (eel_canvas_rect_get_type ())
#define EEL_CANVAS_RECT(obj)            (GTK_CHECK_CAST ((obj), EEL_TYPE_CANVAS_RECT, EelCanvasRect))
#define EEL_CANVAS_RECT_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), EEL_TYPE_CANVAS_RECT, EelCanvasRectClass))
#define EEL_IS_CANVAS_RECT(obj)         (GTK_CHECK_TYPE ((obj), EEL_TYPE_CANVAS_RECT))

typedef struct EelCanvasRectDetails EelCanvasRectDetails;

typedef struct
{
	GnomeCanvasItem item;
	EelCanvasRectDetails *details;
} EelCanvasRect;

typedef struct
{
	GnomeCanvasItemClass parent_class;
} EelCanvasRectClass;

GType      eel_canvas_rect_get_type (void);

G_END_DECLS

#endif /* EEL_CANVAS_RECT_H */
