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

/* Based upon gnome-icon-list.h, the original copyright note follows :
 *
 * Copyright (C) 1998, 1999 Free Software Foundation
 * Copyright (C) 2000 Red Hat, Inc.
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


#ifndef IMAGE_LIST_H
#define IMAGE_LIST_H

#include <glib.h>
#include <libgnomeui/gnome-icon-item.h>
#include <libgnomecanvas/gnome-canvas.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "gthumb-enum-types.h"
#include "gnome-canvas-thumb.h"
#include "gthumb-text-item.h"

G_BEGIN_DECLS

#define IMAGE_LIST_TYPE            (image_list_get_type ())
#define IMAGE_LIST(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), IMAGE_LIST_TYPE, ImageList))
#define IMAGE_LIST_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), IMAGE_LIST_TYPE, ImageListClass))
#define IS_IMAGE_LIST(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IMAGE_LIST_TYPE))
#define IS_IMAGE_LIST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), IMAGE_LIST_TYPE))
#define IMAGE_LIST_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), IMAGE_LIST_TYPE, ImageListClass))

#define IMAGE(o) ((Image*)o)

/* Image structure */
typedef struct {
	/* Image pixbuf and text items */
	GnomeCanvasThumb *image;
	GThumbTextItem   *text;
	GThumbTextItem   *comment;

	/* Image size. */
	gint width, height;

	/* User data and destroy notify function */
	gpointer data;
	GtkDestroyNotify destroy;

	/* ID for the text item's event signal handler */
	guint text_event_id;

	/* Whether the image is selected, and temporary storage for rubberband
         * selections.
	 */
	guint selected : 1;
	guint tmp_selected : 1;
} Image;


typedef enum { /*< skip >*/
	IMAGE_LIST_VIEW_TEXT,                /* Show text. */
	IMAGE_LIST_VIEW_COMMENTS,            /* Show comment. */
 	IMAGE_LIST_VIEW_COMMENTS_OR_TEXT,    /* When a comment is present do
					      * not show text. */
	IMAGE_LIST_VIEW_ALL                  /* Show comment and text. */
} ImageListViewMode;


typedef enum {
	GTHUMB_CURSOR_MOVE_UP,
	GTHUMB_CURSOR_MOVE_DOWN,
	GTHUMB_CURSOR_MOVE_RIGHT,
	GTHUMB_CURSOR_MOVE_LEFT,
	GTHUMB_CURSOR_MOVE_PAGE_UP,
	GTHUMB_CURSOR_MOVE_PAGE_DOWN,
	GTHUMB_CURSOR_MOVE_BEGIN,
	GTHUMB_CURSOR_MOVE_END,
} GthumbCursorMovement;


typedef enum { /*< skip >*/
	GTHUMB_VISIBILITY_NONE,
	GTHUMB_VISIBILITY_FULL,
	GTHUMB_VISIBILITY_PARTIAL,
	GTHUMB_VISIBILITY_PARTIAL_TOP,
	GTHUMB_VISIBILITY_PARTIAL_BOTTOM
} GthumbVisibility;


typedef enum { 
	GTHUMB_SELCHANGE_NONE,           /* Do not change the selection. */
	GTHUMB_SELCHANGE_SET,            /* Set the focused image as the
					  * selection. */
	GTHUMB_SELCHANGE_SET_RANGE       /* Set as selection the 
					  * images contained in the
					  * rectangle that has as opposite
					  * corners the last focused image
					  * and the currently focused 
					  * image. */
} GthumbSelectionChange;


typedef struct _ImageListPrivate ImageListPrivate;

typedef struct {
	GnomeCanvas canvas;

	/* Scroll adjustments */
	GtkAdjustment *adj;
	GtkAdjustment *hadj;

	/* Number of images in the list */
	int images;

	/* A list of integers with the indices of the currently selected 
	 * icons */
	GList *selection;

	/* Private data */
	ImageListPrivate *priv; 
} ImageList;


typedef struct {
	GnomeCanvasClass parent_class;

	void     (*select_image)             (ImageList  *gil, 
					      int         num, 
					      GdkEvent   *event);
	void     (*unselect_image)           (ImageList  *gil, 
					      int         num, 
					      GdkEvent   *event);
	void     (*focus_image)              (ImageList  *gil, 
					      int         num);
	gboolean (*text_changed)             (ImageList  *gil, 
					      int         num, 
					      const char *new_text);

        /* Key Binding signals */

	void     (*select_all)               (ImageList             *gil);
        void     (*move_cursor)              (ImageList             *gil, 
					      GthumbCursorMovement   dir,
					      GthumbSelectionChange  sel_change);
	void     (*add_cursor_selection)     (ImageList *gil);
	void     (*toggle_cursor_selection)  (ImageList *gil);
} ImageListClass;


enum { /*< skip >*/
	IMAGE_LIST_IS_EDITABLE = 1 << 0,
	IMAGE_LIST_STATIC_TEXT = 1 << 1
};


GType          image_list_get_type            (void);

GtkWidget     *image_list_new                 (guint          image_width,
					       GtkAdjustment *adj,
					       int            flags);

void           image_list_set_hadjustment     (ImageList *gil,
					       GtkAdjustment *hadj);

void           image_list_set_vadjustment     (ImageList *gil,
					       GtkAdjustment *vadj);


/* To avoid excesive recomputes during insertion/deletion */
void           image_list_freeze              (ImageList *gil);
void           image_list_thaw                (ImageList *gil);

void           image_list_insert              (ImageList *gil,
					       gint pos, 
					       GdkPixbuf *pixbuf,
					       const gchar *text,
					       const gchar *comment);
int            image_list_append              (ImageList *gil,
					       GdkPixbuf *pixbuf,
					       const gchar *text,
					       const gchar *comment);

void           image_list_clear               (ImageList *gil);
void           image_list_remove              (ImageList *gil, 
					       gint pos);

void           image_list_set_image_pixbuf    (ImageList *gil,
					       gint pos,
					       GdkPixbuf *pixbuf);
void           image_list_set_image_text      (ImageList *gil,
					       gint pos,
					       const gchar *text);
void           image_list_set_image_comment   (ImageList *gil,
					       gint pos,
					       const gchar *comment);

GdkPixbuf*     image_list_get_image_pixbuf    (ImageList *gil,
					       gint pos);
const gchar*   image_list_get_image_text      (ImageList *gil,
					       gint pos);
const gchar*   image_list_get_image_comment   (ImageList *gil,
					       gint pos);

gchar*         image_list_get_old_text        (ImageList *gil);

GList *        image_list_get_list            (ImageList *gil);


/* Managing the selection */
void           image_list_set_selection_mode  (ImageList        *gil,
					       GtkSelectionMode  mode);
void           image_list_select_image        (ImageList        *gil,
					       int               pos);
void           image_list_unselect_image      (ImageList        *gil,
					       int               pos);
void           image_list_select_all          (ImageList        *gil);
int            image_list_unselect_all        (ImageList        *gil,
					       GdkEvent         *event, 
					       gpointer          keep);

/* Setting the spacing values */
void           image_list_set_image_width     (ImageList        *gil,
					       int               w);
void           image_list_set_row_spacing     (ImageList        *gil,
					       int               pixels);
void           image_list_set_col_spacing     (ImageList        *gil,
					       int               pixels);
void           image_list_set_text_spacing    (ImageList        *gil,
					       int               pixels);
void           image_list_set_image_border    (ImageList        *gil,
					       int               pixels);
void           image_list_set_separators      (ImageList        *gil,
					       const char       *sep);

/* Attaching information to the items */
void           image_list_set_image_data       (ImageList       *gil,
						int              pos, 
						gpointer         data);
void           image_list_set_image_data_full  (ImageList       *gil,
						int              pos, 
						gpointer         data,
						GtkDestroyNotify destroy);
int            image_list_find_image_from_data (ImageList       *gil,
						gpointer         data);
gpointer       image_list_get_image_data       (ImageList       *gil,
						int              pos);


/* Visibility */
void           image_list_set_view_mode        (ImageList *gil,
						guint8 mode);

guint8         image_list_get_view_mode        (ImageList *gil);

void           image_list_moveto               (ImageList *gil,
						int pos, 
						double yalign);
GthumbVisibility  image_list_image_is_visible     (ImageList *gil,
						int pos);

int            image_list_get_image_at         (ImageList *gil, 
						int x, 
						int y);

int            image_list_get_first_visible    (ImageList *gil);

int            image_list_get_last_visible     (ImageList *gil);

int            image_list_get_images_per_line  (ImageList *gil);


/* Sort */
void           image_list_set_compare_func     (ImageList *gil,
						GCompareFunc cmp_func);

void           image_list_set_sort_type        (ImageList *gil,
						GtkSortType sort_type);

void           image_list_sort                 (ImageList *gil);


/* Editing and Focus */

void           image_list_focus_image          (ImageList *gil, 
						gint pos);

int            image_list_get_focus_image      (ImageList *gil);

gboolean       image_list_has_focus            (ImageList *gil);

void           image_list_start_editing        (ImageList *gil,
						gint pos);

gboolean       image_list_editing              (ImageList *gil);


/* Accessibility functions */
/* FIXME
GnomeIconTextItem *image_list_get_icon_text_item (ImageList *gil,
						  int idx);

GnomeCanvasPixbuf *image_list_get_icon_pixbuf_item (ImageList *gil,
						    int idx);
*/


G_END_DECLS

#endif /* IMAGE_LIST_H */
