/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 1997, 1998, 1999, 2000 Free Software Foundation
 * All rights reserved.
 *
 * This file is part of the Gnome Library.
 *
 * The Gnome Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Gnome Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Gnome Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/*
  @NOTATION@
 */
/* Text item type for GnomeCanvas widget
 *
 * GnomeCanvas is basically a port of the Tk toolkit's most excellent canvas widget.  Tk is
 * copyrighted by the Regents of the University of California, Sun Microsystems, and other parties.
 *
 *
 * Author: Federico Mena <federico@nuclecu.unam.mx>
 * Port to Pango co-done by Gergõ Érdi <cactus@cactus.rulez.org>
 */

#ifndef GTHUMB_CANVAS_TEXT_H
#define GTHUMB_CANVAS_TEXT_H


#include <libgnomecanvas/gnome-canvas.h>
#include <libgnomecanvas/gnome-canvas-text.h>
#include <pango/pango-layout.h>

G_BEGIN_DECLS


#define GTHUMB_TYPE_CANVAS_TEXT            (gthumb_canvas_text_get_type ())
#define GTHUMB_CANVAS_TEXT(obj)            (GTK_CHECK_CAST ((obj), GTHUMB_TYPE_CANVAS_TEXT, GthumbCanvasText))
#define GTHUMB_CANVAS_TEXT_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GTHUMB_TYPE_CANVAS_TEXT, GthumbCanvasTextClass))
#define GTHUMB_IS_CANVAS_TEXT(obj)         (GTK_CHECK_TYPE ((obj), GTHUMB_TYPE_CANVAS_TEXT))
#define GTHUMB_IS_CANVAS_TEXT_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GTHUMB_TYPE_CANVAS_TEXT))
#define GTHUMB_CANVAS_TEXT_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), GTHUMB_TYPE_CANVAS_TEXT, GthumbCanvasTextClass))


typedef struct _GthumbCanvasText      GthumbCanvasText;
typedef struct _GthumbCanvasTextClass GthumbCanvasTextClass;

struct _GthumbCanvasText {
	GnomeCanvasText __parent;

	int             pango_layout_width;
	PangoWrapMode   pango_wrap_mode;
};

struct _GthumbCanvasTextClass {
	GnomeCanvasTextClass __parent_class;
};


/* Standard Gtk function */
GType gthumb_canvas_text_get_type (void) G_GNUC_CONST;


G_END_DECLS

#endif
