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

/* Based upon gnome-icon-item.h, the original copyright note follows :
 *
 * Copyright (C) 1998, 1999, 2000, 2001 Free Software Foundation
 * Copyright (C) 2001 Anders Carlsson
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
 *
 * Author: Anders Carlsson <andersca@gnu.org>
 *
 * Based on the GNOME 1.0 icon item by Miguel de Icaza and Federico Mena.
 */

#ifndef _GTHUMB_TEXT_ITEM_H_
#define _GTHUMB_TEXT_ITEM_H_

#include <libgnomecanvas/gnome-canvas.h>
#include <gtk/gtkeditable.h>
#include <gtk/gtkentry.h>

G_BEGIN_DECLS

#define GTHUMB_TYPE_TEXT_ITEM            (gthumb_text_item_get_type ())
#define GTHUMB_TEXT_ITEM(obj)            (GTK_CHECK_CAST ((obj), GTHUMB_TYPE_TEXT_ITEM, GThumbTextItem))
#define GTHUMB_TEXT_ITEM_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GTHUMB_TYPE_TEXT_ITEM, GThumbTextItemClass))
#define GTHUMB_IS_TEXT_ITEM(obj)         (GTK_CHECK_TYPE ((obj), GTHUMB_TYPE_TEXT_ITEM))
#define GTHUMB_IS_TEXT_ITEM_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GTHUMB_TYPE_TEXT_ITEM))
#define GTHUMB_TEXT_ITEM_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), GTHUMB_TYPE_TEXT_ITEM, GThumbTextItemClass))

typedef struct _GThumbTextItem        GThumbTextItem;
typedef struct _GThumbTextItemClass   GThumbTextItemClass;
typedef struct _GThumbTextItemPrivate GThumbTextItemPrivate;

struct _GThumbTextItem {
	GnomeCanvasItem __parent;

	/* Position and maximum allowed width */
	int x, y;
	int width;

	/* Font name */
	char *fontname;

	/* Actual text */
	char *text;

	/* Whether the text is being edited */
	unsigned int editing : 1;

	/* Whether the text item is selected */
	unsigned int selected : 1;

        /* Whether the text item is focused */
        unsigned int focused : 1;
 
	/* Whether the text is editable */
	unsigned int is_editable : 1;

	/* Whether the text is allocated by us (FALSE if allocated by the 
	 * client) */
	unsigned int is_text_allocated : 1;

	GThumbTextItemPrivate *_priv;
};

struct _GThumbTextItemClass {
	GnomeCanvasItemClass __parent_class;

	/*< signals >*/

	gboolean  (* text_changed)      (GThumbTextItem *iti);
	void      (* height_changed)    (GThumbTextItem *iti);
	void      (* width_changed)     (GThumbTextItem *iti);
	void      (* editing_started)   (GThumbTextItem *iti);
	void      (* editing_stopped)   (GThumbTextItem *iti);
};

GType        gthumb_text_item_get_type       (void) G_GNUC_CONST;

void         gthumb_text_item_configure      (GThumbTextItem *iti,
					      int             x,
					      int             y,
					      int             width,
					      const char     *fontname,
					      const char     *utf8_text,
					      gboolean        is_editable,
					      gboolean        is_static);

void         gthumb_text_item_set_markup_str (GThumbTextItem *iti,
					      char           *markup_str);

void         gthumb_text_item_setxy          (GThumbTextItem *iti,
					      int             x,
					      int             y);

void         gthumb_text_item_setxy_width    (GThumbTextItem *iti,
					      int             x,
					      int             y,
					      int             width);

void         gthumb_text_item_set_width      (GThumbTextItem *iti,
					      int             width);

void         gthumb_text_item_select         (GThumbTextItem *iti,
					      gboolean        sel);

void         gthumb_text_item_focus          (GThumbTextItem *iti,
					      gboolean        focused);

const char  *gthumb_text_item_get_text       (GThumbTextItem *iti);

int          gthumb_text_item_get_text_width (GThumbTextItem *iti);

int          gthumb_text_item_get_text_height (GThumbTextItem *iti);

void         gthumb_text_item_start_editing  (GThumbTextItem *iti);

void         gthumb_text_item_stop_editing   (GThumbTextItem *iti,
					      gboolean        accept);


G_END_DECLS

#endif /* _GTHUMB_TEXT_ITEM_H_ */
