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

/* Based upon gnome-icon-list.c, the original copyright note follows :
 *
 * Copyright (C) 1998, 1999, 2000 Free Software Foundation
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

#include <config.h>
#include <string.h>
#include <stdio.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtkadjustment.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkbindings.h>
#include <gtk/gtkselection.h>
#include <gtk/gtkstock.h>
#include <gtk/gtkdnd.h>
#include <libgnomecanvas/gnome-canvas-rect-ellipse.h>

#include "eel-canvas-rect.h"
#include "image-list.h"
#include "gthumb-text-item.h"
#include "gthumb-marshal.h"
#include "glib-utils.h"

#define DEFAULT_FONT "Sans normal 12"

/* Default spacings */
#define DEFAULT_ROW_SPACING   4
#define DEFAULT_COL_SPACING   4
#define DEFAULT_TEXT_SPACING  2
#define DEFAULT_IMAGE_BORDER  3
#define TEXT_COMMENT_SPACE    2  /* space between text and comment. */

/* Autoscroll timeout in milliseconds */
#define SCROLL_TIMEOUT 30

#define COMMENT_MAX_LINES  5 /* max number of lines to display. */
#define ETC " [..]"          /* string to append when the comment doesn't fit. */


/* Signals */
enum {
	SELECT_IMAGE,
	UNSELECT_IMAGE,
	FOCUS_IMAGE,
	TEXT_CHANGED,
	DOUBLE_CLICK,
	SELECT_ALL,
	MOVE_CURSOR,
	ADD_CURSOR_SELECTION,
	TOGGLE_CURSOR_SELECTION,
	LAST_SIGNAL
};

typedef enum {
	SYNC_INSERT,
	SYNC_REMOVE
} SyncType;

#define SELECTION_BORDER 10


static guint gil_signals[LAST_SIGNAL] = { 0 };
static GnomeCanvasClass *parent_class;

static GtkTargetEntry target_table[] = {
	{ "text/uri-list", 0, 0 } 
};


/* A row of images */
typedef struct {
	int y;
	int image_height, text_height, comment_height;
	GList *line_images;
} ImageLine;


/* Private data of the ImageList structure */
struct _ImageListPrivate {
	/* List of images and list of rows of images */
	GList *image_list;
	GList *lines;

	Image *editing_image;

	/* The icon that has keyboard focus */
	gint focus_image;

	/* Used to do multiple selection with the keyboard. */
	gint old_focus_image; 

	/* Whether a multi selection with the keyboard has started. */
	gboolean multi_selecting_with_keyboard;
	
	/* Max of the height of all the image rows and window height */
	int total_height;
	
	/* Selection mode */
	GtkSelectionMode selection_mode;

	/* Veiw mode */
	guint8 view_mode;

	/* Freeze count */
	int frozen;
	guint update_width : 1;

	/* Width allocated for images */
	int min_image_width;  /* This is setted by the user. */
	int max_image_width;  /* Usualy this is equal to min_image_width,
			       * but if a text item has a greater width
			       * this var stores the max width. */

	/* Spacing values */
	int row_spacing;
	int col_spacing;
	int text_spacing;
	int image_border;

	/* Separators used to wrap the text below images */
	char *separators;

	/* Index and pointer to last selected image */
	int last_selected_idx;
	Image *last_selected_image;

	/* Timeout ID for autoscrolling */
	guint timer_tag;

	/* Change the adjustment value by this amount when autoscrolling */
	int value_diff;
	
	/* Mouse position for autoscrolling */
	int event_last_x;
	int event_last_y;

	/* Selection start position */
	int sel_start_x;
	int sel_start_y;

	/* Modifier state when the selection began */
	guint sel_state;

	/* Rubberband rectangle */
	GnomeCanvasItem *sel_rect;

	/* Saved event for a pending selection */
	GdkEvent select_pending_event;

	/* Whether the image texts are editable */
	guint is_editable : 1;

	/* Whether the image texts need to be copied */
	guint static_text : 1;

	/* Whether the images need to be laid out */
	guint dirty : 1;

	/* Whether the user is performing a drag */
	guint dragging : 1;

	/* Whether the drag has started. */
	guint drag_started : 1;
	
	GtkTargetList *target_list;

	/* The point where the drag started. */
	int drag_start_x;
	int drag_start_y;

	/* Whether the user is performing a rubberband selection */
	guint selecting : 1;

	/* Whether editing an image is pending after a button press */
	guint edit_pending : 1;

	/* Whether selection is pending after a button press */
	guint select_pending : 1;

	/* Whether the image that is pending selection was selected to 
	 * begin with */
	guint select_pending_was_selected : 1;

	/* Ascending or descending order. */
	GtkSortType sort_type;

	/* Comapre function. */
	GCompareFunc compare;

	/* Whether the image list is editing. */
	gboolean has_focus;

	/* The text before starting to edit. */
	gchar * old_text;
};


static int
image_line_height (ImageList *gil, ImageLine *il)
{
	return (il->image_height 
		+ ((il->comment_height > 0 || il->text_height > 0) ? gil->priv->text_spacing : 0)
		+ il->comment_height
		+ ((il->comment_height > 0 && il->text_height > 0) ? TEXT_COMMENT_SPACE : 0)
		+ il->text_height
		+ gil->priv->row_spacing);
}


static void
get_text_item_size (GThumbTextItem *text_item, 
		    int *width, 
		    int *height)
{
	if (text_item->text != NULL) {
		if (width != NULL) 
			*width = gthumb_text_item_get_text_width (text_item);
		if (height != NULL)
			*height = gthumb_text_item_get_text_height (text_item);
	} else {
		if (width != NULL)
			*width = 0;
		if (height != NULL)
			*height = 0;
	}
}


static gboolean
string_void (const char *s)
{
	return (s == NULL) || (*s == 0);
}


static void
image_get_view_mode (ImageList *gil,
		     Image *image,
		     gboolean *view_text,
		     gboolean *view_comment)
{
	*view_text    = TRUE;
	*view_comment = TRUE;

	if (gil->priv->view_mode == IMAGE_LIST_VIEW_TEXT)
		*view_comment = FALSE;
	if (gil->priv->view_mode == IMAGE_LIST_VIEW_COMMENTS)
		*view_text = FALSE;
	if ((gil->priv->view_mode == IMAGE_LIST_VIEW_COMMENTS_OR_TEXT)
	    && ! string_void (image->comment->text))
		*view_text = FALSE;

	if (string_void (image->comment->text))
		*view_comment = FALSE;
	if (string_void (image->text->text))
		*view_text = FALSE;
}


static int
get_image_width (ImageList *gil, Image *image)
{
	gboolean view_text, view_comment;
	int      max_width, image_width, text_width, comment_width;

	image_get_view_mode (gil, image, &view_text, &view_comment);

	image_width = image->width;
	get_text_item_size (image->text, &text_width, NULL);
	get_text_item_size (image->comment, &comment_width, NULL);

	max_width = image_width;
	if (view_text)
		max_width = MAX (max_width, text_width);
	if (view_comment)
		max_width = MAX (max_width, comment_width);

	return max_width;
}


static void
image_get_height (Image *image, 
		  int   *image_height, 
		  int   *text_height,
		  int   *comment_height)
{
	*image_height = image->height;
	get_text_item_size (image->text, NULL, text_height);
	get_text_item_size (image->comment, NULL, comment_height);
}


static int
gil_get_images_per_line (ImageList *gil)
{
	int images_per_line;

	images_per_line = GTK_WIDGET (gil)->allocation.width / (gil->priv->max_image_width + gil->priv->col_spacing);
	if (images_per_line == 0)
		images_per_line = 1;

	return images_per_line;
}


int
image_list_get_images_per_line (ImageList *gil)
{
	g_return_val_if_fail (IS_IMAGE_LIST (gil), 1);
	return gil_get_images_per_line (gil);
}


static void
gil_place_image (ImageList *gil, 
		 Image     *image, 
		 int        x, 
		 int        y, 
		 int        image_height,
		 gboolean   view_text,
		 gboolean   view_comment)
{
	int x_offset, y_offset;

	if (image_height > image->height)
		y_offset = (image_height - image->height) / 2;
	else
		y_offset = 0;
	x_offset = (gil->priv->max_image_width - image->width) / 2;

	gnome_canvas_item_set (GNOME_CANVAS_ITEM (image->image),
			       "x", (gdouble) x + x_offset,
			       "y", (gdouble) y + y_offset,
			       NULL);

	y = y + image_height + gil->priv->text_spacing;

	if (view_comment) {
		int comment_height;

		gnome_canvas_item_show (GNOME_CANVAS_ITEM (image->comment));
		gthumb_text_item_setxy (image->comment,
					x + x_offset + 1,
					y);
		get_text_item_size (image->comment, NULL, &comment_height);
		y += comment_height + TEXT_COMMENT_SPACE + 2;
	} else
		gnome_canvas_item_hide (GNOME_CANVAS_ITEM (image->comment));

	if (view_text) {
		gnome_canvas_item_show (GNOME_CANVAS_ITEM (image->text));
		gthumb_text_item_setxy (image->text,
					x + x_offset + 1,
					y);
	} else
		gnome_canvas_item_hide (GNOME_CANVAS_ITEM (image->text));
}


static void
gil_layout_line (ImageList *gil, 
		 ImageLine *il)
{
	GList *l;
	gint x;
	gboolean view_text, view_comment;

	x = 0;
	for (l = il->line_images; l; l = l->next) {
		Image *image = l->data;

		image_get_view_mode (gil, image, &view_text, &view_comment);
		
		x += gil->priv->col_spacing;
		gil_place_image (gil, image, x, il->y, il->image_height,
				 view_text, view_comment);
		x += gil->priv->max_image_width;
	}
}


static void
gil_add_and_layout_line (ImageList *gil, 
			 GList *line_images, 
			 gint y,
			 gint image_height, 
			 gint text_height,
			 gint comment_height)
{
	ImageLine *il;

	il = g_new (ImageLine, 1);
	il->line_images = line_images;
	il->y = y;
	il->image_height = image_height;
	il->text_height = text_height;
	il->comment_height = comment_height;

	gil_layout_line (gil, il);
	gil->priv->lines = g_list_append (gil->priv->lines, il);
}


static void gil_layout_all_images (ImageList *gil);
static void gil_scrollbar_adjust (ImageList *gil);


static void
gil_relayout_images_at (ImageList *gil, 
			gint pos, 
			gint y)
{
	gint col, row, text_height, image_height, comment_height;
	gint images_per_line, n;
	GList *line_images, *l;
	gint max_height, tmp_max_height;
	gint max_width;
	gint old_max_image_width;

	images_per_line = gil_get_images_per_line (gil);

	col = row = text_height = image_height = comment_height = 0;
	max_width = 0;
	max_height = 0;
	line_images = NULL;

	l = g_list_nth (gil->priv->image_list, pos);

	for (n = pos; l; l = l->next, n++) {
		Image *image = l->data;
		gint ih, th, ch;
		gboolean view_text, view_comment;

		if (! (n % images_per_line)) {
			if (line_images) {
				gil_add_and_layout_line (gil, line_images, y,
							 image_height, 
							 text_height,
							 comment_height);
				line_images = NULL;

				y += max_height + gil->priv->row_spacing;
			}

			max_height = 0;
			image_height = 0;
			text_height = 0;
			comment_height = 0;
		}

		max_width = MAX (max_width, get_image_width (gil, image));

		image_get_height (image, &ih, &th, &ch);
		image_get_view_mode (gil, image, &view_text, &view_comment);

		if (! view_text) th = 0;
		if (! view_comment) ch = 0;

		tmp_max_height = ih + th + ch;
		if ((ch > 0) || (th > 0))
			tmp_max_height += gil->priv->text_spacing;
		if ((ch > 0) && (th > 0))
			tmp_max_height += TEXT_COMMENT_SPACE;
		max_height = MAX (tmp_max_height, max_height);

		image_height   = MAX (ih, image_height);
		text_height    = MAX (th, text_height);
		comment_height = MAX (ch, comment_height);

		line_images = g_list_append (line_images, image);
	}

	if (line_images)
		gil_add_and_layout_line (gil, line_images, y, 
					 image_height, 
					 text_height,
					 comment_height);

	/* Check whether we need to relayout because some text is wider
	 * than the maximum width allowed. */

	old_max_image_width = gil->priv->max_image_width;
	gil->priv->max_image_width = MAX (gil->priv->max_image_width, 
					  max_width);
	gil->priv->max_image_width = MAX (gil->priv->max_image_width, 
					  gil->priv->min_image_width);

	if (old_max_image_width == gil->priv->max_image_width) 
		return;

	gil_layout_all_images (gil);
	gil_scrollbar_adjust (gil);
}


static void
gil_free_line_info (ImageList *gil)
{
	GList *l;

	for (l = gil->priv->lines; l; l = l->next) {
		ImageLine *il = l->data;
		g_list_free (il->line_images);
		g_free (il);
	}

	g_list_free (gil->priv->lines);
	gil->priv->lines = NULL;
	gil->priv->total_height = 0;
}


static void
gil_free_line_info_from (ImageList *gil, 
			 gint first_line)
{
	GList *l, *ll;
	
	ll = g_list_nth (gil->priv->lines, first_line);

	if (ll == NULL)
		return;

	for (l = ll; l; l = l->next) {
		ImageLine *il = l->data;

		g_list_free (il->line_images);
		g_free (il);
	}

	if (gil->priv->lines) {
		if (ll->prev)
			ll->prev->next = NULL;
		else
			gil->priv->lines = NULL;
	}

	g_list_free (ll);
}


static void
gil_layout_from_line (ImageList *gil, 
		      gint line)
{
	GList *l;
	gint height;

	if (! GTK_WIDGET_REALIZED (gil))
		return;
	
	gil_free_line_info_from (gil, line);

	height = gil->priv->row_spacing;
	for (l = gil->priv->lines; l; l = l->next) {
		ImageLine *il = l->data;
		height += image_line_height (gil, il);
	}

	gil_relayout_images_at (gil, 
				line * gil_get_images_per_line (gil), 
				height);
}


static void
reset_text_width (ImageList *gil) 
{
	GList *scan;
	int w = gil->priv->max_image_width;

	for (scan = gil->priv->image_list; scan; scan = scan->next) {
		Image *image = scan->data;
		gthumb_text_item_set_width (image->text, w);
		gthumb_text_item_set_width (image->comment, w);
	}
	gil->priv->update_width = FALSE;
}


static void
gil_layout_all_images (ImageList *gil)
{
	if (! GTK_WIDGET_REALIZED (gil))
		return;

	if (gil->priv->update_width) 
		reset_text_width (gil);

	gil_free_line_info (gil);
	gil_relayout_images_at (gil, 0, gil->priv->row_spacing);
	gil->priv->dirty = FALSE;
}


static void
gil_scrollbar_adjust (ImageList *gil)
{
	GList *l;
	gdouble wy;
	gint height, step_increment;

	if (! GTK_WIDGET_REALIZED (gil))
		return;

	height = gil->priv->row_spacing;
	step_increment = 0;
	for (l = gil->priv->lines; l; l = l->next) {
		ImageLine *il = l->data;

		height += image_line_height (gil, il);

		if (l == gil->priv->lines)
			step_increment = height / 3;
	}

	if (step_increment == 0)
		step_increment = 10;

	gil->priv->total_height = MAX (height, GTK_WIDGET (gil)->allocation.height);

	wy = gil->adj->value;

	gil->adj->upper = height;
	gil->adj->step_increment = step_increment;
	gil->adj->page_increment = GTK_WIDGET (gil)->allocation.height;
	gil->adj->page_size = GTK_WIDGET (gil)->allocation.height;

	if (wy > gil->adj->upper - gil->adj->page_size)
		wy = gil->adj->upper - gil->adj->page_size;
	gil->adj->value = MAX (0.0, wy);

	gtk_adjustment_changed (gil->adj);
}


/* Emits the select_image or unselect_image signals as appropriate */
static void
emit_select (ImageList *gil, int sel, int i, GdkEvent *event)
{
	g_signal_emit (G_OBJECT (gil),
		       gil_signals[sel ? SELECT_IMAGE : UNSELECT_IMAGE],
		       0,
		       i,
		       event);
}


/**
 * image_list_select_all:
 * @gil: An image list.
 * 
 * Selects all the images in the image list. 
 *
 **/
void
image_list_select_all (ImageList *gil) 
{
	GList *l;
	Image *image;
	gint i;

	g_return_if_fail (IS_IMAGE_LIST (gil));
	
	for (l = gil->priv->image_list, i = 0; l; l = l->next, i++) {
		image = l->data;

		if (! image->selected)
			emit_select (gil, TRUE, i, NULL);
	}
}


/**
 * image_list_unselect_all:
 * @gil:   An image list.
 * @event: Unused, must be NULL.
 * @keep:  For internal use only; must be NULL.
 *
 * Unselects all the images in the image list.  The @event and @keep parameters
 * must be NULL, since they are used only internally.
 *
 * Returns: the number of images in the image list
 */
int
image_list_unselect_all (ImageList *gil, 
			 GdkEvent *event, 
			 gpointer keep)
{
	GList *l;
	Image *image;
	gint i, idx = 0;

	g_return_val_if_fail (IS_IMAGE_LIST (gil), 0);

	for (l = gil->priv->image_list, i = 0; l; l = l->next, i++) {
		image = l->data;

		if (image == keep)
			idx = i;
		else if (image->selected)
			emit_select (gil, FALSE, i, event);
	}

	return idx;
}


static void
sync_selection (ImageList *gil, 
		gint pos, 
		SyncType type)
{
	GList *list;

	for (list = gil->selection; list; list = list->next) {
		if (GPOINTER_TO_INT (list->data) >= pos) {
			gint i = GPOINTER_TO_INT (list->data);

			switch (type) {
			case SYNC_INSERT:
				list->data = GINT_TO_POINTER (i + 1);
				break;

			case SYNC_REMOVE:
				list->data = GINT_TO_POINTER (i - 1);
				break;

			default:
				g_assert_not_reached ();
				break;
			}
		}
	}
}


static gint
gil_image_to_index (ImageList *gil, Image *image)
{
	GList *l;
	gint n;

	n = 0;
	for (l = gil->priv->image_list; l; n++, l = l->next)
		if (l->data == image)
			return n;

	/*g_assert_not_reached ();*/
	return -1; /* Shut up the compiler */
}


static void
stop_dragging (ImageList *gil)
{
	if (! gil->priv->dragging)
		return;

	gil->priv->dragging = FALSE;
	gil->priv->drag_started = FALSE;
}


static void
stop_selection (ImageList *gil, guint32 time)
{
	if (! gil->priv->selecting)
		return;

	gnome_canvas_item_ungrab (gil->priv->sel_rect, time);
	gnome_canvas_item_hide (gil->priv->sel_rect);
	
	gil->priv->selecting = FALSE;

	if (gil->priv->timer_tag != 0) {
		g_source_remove (gil->priv->timer_tag);
		gil->priv->timer_tag = 0;
	}
}


/* Event handler for images when we are in SINGLE or BROWSE mode */
static gint
selection_one_image_event (ImageList *gil, 
			   Image     *image, 
			   gint       idx, 
			   gint       on_text, 
			   GdkEvent  *event)
{
	GThumbTextItem *text;
	int             retval;

	retval = FALSE;

	/* We use a separate variable and ref the object because it may be
	 * destroyed by one of the signal handlers.
	 */
	text = image->text;
	g_object_ref (G_OBJECT (text));

	switch (event->type) {
	case GDK_BUTTON_PRESS:
		gil->priv->edit_pending = FALSE;
		gil->priv->select_pending = FALSE;

		/* Ignore wheel mouse clicks for now */
		if (event->button.button > 3)
			break;

		if (! image->selected) {
			image_list_unselect_all (gil, NULL, NULL);
			emit_select (gil, TRUE, idx, event);
			image_list_focus_image (gil, idx);
		} else {
			if (gil->priv->selection_mode == GTK_SELECTION_SINGLE
			    && (event->button.state & GDK_CONTROL_MASK))
				emit_select (gil, FALSE, idx, event);
			else if (on_text 
				 && gil->priv->is_editable 
				 && event->button.button == 1)
				gil->priv->edit_pending = TRUE;
			else {
				image_list_unselect_all (gil, NULL, NULL);
				emit_select (gil, TRUE, idx, event);
				image_list_focus_image (gil, idx);
			}
		}

		retval = FALSE;
		break;

	case GDK_2BUTTON_PRESS:
	case GDK_3BUTTON_PRESS:
		if (gil->priv->select_pending) {
			g_signal_emit (G_OBJECT (gil),
				       gil_signals[DOUBLE_CLICK],
				       0,
				       idx);
			break;
		}

		stop_dragging (gil);
		stop_selection (gil, GDK_CURRENT_TIME);

		/* Ignore wheel mouse clicks for now */
		if (event->button.button > 3)
			break;

		/* emit_select (gil, TRUE, idx, event); FIXME */
		retval = FALSE;
		break;

	case GDK_BUTTON_RELEASE:
		if (gil->priv->edit_pending) {
			gil->priv->editing_image = image;
			gthumb_text_item_start_editing (text);
			gil->priv->edit_pending = FALSE;
		}

		retval = FALSE;
		break;

	default:
		break;
	}

	/* If the click was on the text and we actually did something, stop the
	 * image text item's own handler from executing.
	 */
	if (on_text && retval)
		g_signal_stop_emission_by_name (G_OBJECT (text), "event");

	g_object_unref (G_OBJECT (text));

	return retval;
}


/* Handles range selections when clicking on an image */
static void
select_range (ImageList *gil, 
	      Image     *image, 
	      gint       idx, 
	      GdkEvent  *event)
{
	gint   a, b;
	GList *l;
	Image *i;

	if (gil->priv->last_selected_idx == -1) {
		gil->priv->last_selected_idx = idx;
		gil->priv->last_selected_image = image;
	}

	if (idx < gil->priv->last_selected_idx) {
		a = idx;
		b = gil->priv->last_selected_idx;
	} else {
		a = gil->priv->last_selected_idx;
		b = idx;
	}

	l = g_list_nth (gil->priv->image_list, a);

	for (; a <= b; a++, l = l->next) {
		i = l->data;

		if (!i->selected)
			emit_select (gil, TRUE, a, NULL);
	}

	/* Actually notify the client of the event */
	emit_select (gil, TRUE, idx, event);
	image_list_focus_image (gil, idx);
}


/* Handles image selection for MULTIPLE or EXTENDED selection modes */
static void
do_select_many (ImageList *gil, 
		Image *image,
		gint idx, 
		GdkEvent *event, 
		gint use_event)
{
	gint range, additive;

	range = (event->button.state & GDK_SHIFT_MASK) != 0;
	additive = (event->button.state & GDK_CONTROL_MASK) != 0;

	if (!additive) {
		if (image->selected)
			image_list_unselect_all (gil, NULL, image);
		else
			image_list_unselect_all (gil, NULL, NULL);
	}

	if (!range) {
		if (additive)
			emit_select (gil, !image->selected, idx, use_event ? event : NULL);
		else 
			emit_select (gil, TRUE, idx, use_event ? event : NULL);

		gil->priv->last_selected_idx = idx;
		gil->priv->last_selected_image = image;
	} else
		select_range (gil, image, idx, use_event ? event : NULL);

	image_list_focus_image (gil, idx);
}


/* Event handler for images when we are in MULTIPLE or EXTENDED mode */
static gint
selection_many_image_event (ImageList *gil, 
			    Image     *image, 
			    gint       idx, 
			    gint       on_text, 
			    GdkEvent  *event)
{
	GThumbTextItem *text;
	int             retval;
	int             additive, range;
	int             do_select;

	retval = FALSE;

	/* We use a separate variable and ref the object because it may be
	 * destroyed by one of the signal handlers.
	 */
	text = image->text;
	g_object_ref (G_OBJECT (text));
	
	range = (event->button.state & GDK_SHIFT_MASK) != 0;
	additive = (event->button.state & GDK_CONTROL_MASK) != 0;
	
	switch (event->type) {
	case GDK_BUTTON_PRESS:
		gil->priv->edit_pending = FALSE;
		gil->priv->select_pending = FALSE;

		if (event->button.button != 1)
			break;

		do_select = TRUE;

		if (additive || range) {
			if (additive && !range) {
				gil->priv->select_pending = TRUE;
				gil->priv->select_pending_event = *event;
				gil->priv->select_pending_was_selected = image->selected;

				/* We have to emit this so that the client will
				 * know about the click.
				 */
				/*emit_select (gil, TRUE, idx, event); FIXME */
				do_select = FALSE;
			}
		} else if (image->selected) {
			gil->priv->select_pending = TRUE;
			gil->priv->select_pending_event = *event;
			gil->priv->select_pending_was_selected = image->selected;
			
			if (on_text 
			    && gil->priv->is_editable 
			    && (event->button.button == 1))
				gil->priv->edit_pending = TRUE;

			/* emit_select (gil, TRUE, idx, event); FIXME */
			do_select = FALSE;
		}
		if (do_select)
			do_select_many (gil, image, idx, event, TRUE);
		
		retval = FALSE;
		break;
		
	case GDK_2BUTTON_PRESS:
	case GDK_3BUTTON_PRESS:
		if (gil->priv->select_pending) {
			g_signal_emit (G_OBJECT (gil),
				       gil_signals[DOUBLE_CLICK],
				       0,
				       idx);
			break;
		}

		stop_dragging (gil);
		stop_selection (gil, GDK_CURRENT_TIME);

		/* emit_select (gil, TRUE, idx, event); FIXME */
		retval = FALSE;
		break;
		
	case GDK_BUTTON_RELEASE:
		if (gil->priv->select_pending) {
			image->selected = gil->priv->select_pending_was_selected;
			do_select_many (gil, image, idx, &gil->priv->select_pending_event, FALSE);
			gil->priv->select_pending = FALSE;
			retval = TRUE;
		}

		if (gil->priv->edit_pending) {
			gil->priv->editing_image = image;
			gthumb_text_item_start_editing (text);
			gil->priv->edit_pending = FALSE;
			retval = FALSE;
		}
		break;
		
	default:
		break;
	}
	
	/* If the click was on the text and we actually did something, stop the
	 * image text item's own handler from executing.
	 */
	if (on_text && retval)
		g_signal_stop_emission_by_name (G_OBJECT (text), "event");
	
	g_object_unref (G_OBJECT (text));

	return retval;
}


/* Event handler for images in the image list */
static gint
image_event (GnomeCanvasItem *item,
	     GdkEvent *event, 
	     gpointer data)
{
	Image *image;
	ImageList *gil;
	gint idx;
	gint on_text;

	image = data;
	gil = IMAGE_LIST (item->canvas);
	idx = gil_image_to_index (gil, image);
	g_assert (idx != -1);

	on_text = item == GNOME_CANVAS_ITEM (image->text); 

	/* Don't handle events meant for editing text items */
        if (on_text && gil->priv->is_editable
            && GTHUMB_TEXT_ITEM (item)->editing) {
                return FALSE;
        }

	if ((gil->priv->editing_image != NULL)
	    && (event->type == GDK_BUTTON_PRESS)) {
		gthumb_text_item_stop_editing (gil->priv->editing_image->text,
					       FALSE);
		gil->priv->editing_image = NULL;
	}

	switch (gil->priv->selection_mode) {
	case GTK_SELECTION_SINGLE:
	case GTK_SELECTION_BROWSE:
		return selection_one_image_event (gil, image, idx, on_text, event);

	case GTK_SELECTION_MULTIPLE:
		return selection_many_image_event (gil, image, idx, on_text, event);

	default:
		g_assert_not_reached ();
		return FALSE;
	}
}


/* Handler for the editing_started signal of an image text item.  We block the
 * event handler so that it will not be called while the text is being edited.
 */
static void
editing_started (GThumbTextItem *iti, 
		 gpointer data)
{
	ImageListPrivate *priv;
	Image *image;

	image = data;
	g_signal_handler_block (iti, image->text_event_id);
	image_list_unselect_all (IMAGE_LIST (GNOME_CANVAS_ITEM (iti)->canvas), 
				 NULL, 
				 image);
	
	priv = IMAGE_LIST (GNOME_CANVAS_ITEM (iti)->canvas)->priv;
	priv->has_focus = TRUE;
	
	if (priv->old_text)
		g_free (priv->old_text);
	priv->old_text = g_strdup (gthumb_text_item_get_text (image->text));
}


/* Handler for the editing_stopped signal of an image text item. 
 * We unblock the event handler so that we can get events from it again.
 */
static void
editing_stopped (GThumbTextItem *iti, 
		 gpointer data)
{
	ImageListPrivate *priv;
	Image *image;

	image = data;
	g_signal_handler_unblock (iti, image->text_event_id);
	gthumb_text_item_stop_editing (iti, TRUE);

	priv = IMAGE_LIST (GNOME_CANVAS_ITEM (iti)->canvas)->priv;
	priv->has_focus = FALSE;
}


static gboolean
text_changed (GnomeCanvasItem *item, 
	      Image *image)
{
	ImageList *gil;
	gboolean accept;
	gint idx;

	gil = IMAGE_LIST (item->canvas);
	accept = TRUE;

	idx = gil_image_to_index (gil, image);
	g_signal_emit (G_OBJECT (gil),
		       gil_signals[TEXT_CHANGED],
		       0,
		       idx, 
		       gthumb_text_item_get_text (image->text),
		       &accept);

	return accept;
}


static void
height_changed (GnomeCanvasItem *item, 
		Image *image)
{
	ImageList *gil;
	gint n;

	gil = IMAGE_LIST (item->canvas);

	if (! GTK_WIDGET_REALIZED (gil))
		return;

	if (gil->priv->frozen) {
		gil->priv->dirty = TRUE;
		return;
	}

	n = gil_image_to_index (gil, image);
	gil_layout_from_line (gil, n / gil_get_images_per_line (gil));
	gil_scrollbar_adjust (gil);
}


static gboolean
width_changed (GnomeCanvasItem *item, 
	       Image *image)
{
	GThumbTextItem *gti = GTHUMB_TEXT_ITEM (item);
	ImageList *gil;
	int old_image_width;

	gil = IMAGE_LIST (item->canvas);

	if (! GTK_WIDGET_REALIZED (gil))
		return FALSE;

	old_image_width = gil->priv->max_image_width;
	gil->priv->max_image_width = MAX (gil->priv->max_image_width, 
					  gti->width);
	gil->priv->max_image_width = MAX (gil->priv->max_image_width, 
					  gil->priv->min_image_width);

	if (old_image_width == gil->priv->max_image_width) 
		return FALSE;

	if (gil->priv->frozen) {
		gil->priv->dirty = TRUE;
		return TRUE;
	}

	gil_layout_all_images (gil);
	gil_scrollbar_adjust (gil);

	return TRUE;
}


static char *
truncate_comment_if_needed (ImageList  *gil, 
			    const char *comment)
{
	GString  *truncated;
	char     *result;
	int       max_len, approx_char_width = 6; /* FIXME */

	if (comment == NULL)
		return NULL;

	max_len = (gil->priv->max_image_width / approx_char_width) * COMMENT_MAX_LINES;

	truncated = g_string_new ("");
	g_string_append_len (truncated, comment, max_len);
	if (strlen (comment) > max_len) 
		g_string_append (truncated, ETC);

	result = truncated->str;

	g_string_free (truncated, FALSE);

	return result;
}


static Image *
image_new_from_pixbuf (ImageList *gil, 
		       GdkPixbuf *pixbuf, 
		       const char *text,
		       const char *comment)
{
	GnomeCanvas       *canvas;
	GnomeCanvasGroup  *group;
	Image             *image;
	double             width, height;
	char              *c;

	canvas = GNOME_CANVAS (gil);
	group = GNOME_CANVAS_GROUP (canvas->root);

	image = g_new0 (Image, 1);

	/* Image item. */

	if (pixbuf) {
		image->image = GNOME_CANVAS_THUMB (gnome_canvas_item_new (
			group,
			gnome_canvas_thumb_get_type (),
			"x", 0.0,
			"y", 0.0,
			"width", (double) gil->priv->min_image_width,
			"height", (double) gil->priv->min_image_width,
			"pixbuf", pixbuf,
			NULL));

		g_object_get (G_OBJECT (image->image),
			      "width", &width,
			      "height", &height,
			      NULL);
		image->width = (gint) width;
		image->height = (gint) height;
	} else {
		image->image = GNOME_CANVAS_THUMB (gnome_canvas_item_new (
			group,
			gnome_canvas_thumb_get_type (),
			"x", 0.0,
			"y", 0.0,
			NULL));

		image->width = 0;
		image->height = 0;
	}

	/* Comment item. */

	image->comment = GTHUMB_TEXT_ITEM (gnome_canvas_item_new (
		group,
		GTHUMB_TYPE_TEXT_ITEM,
		NULL));

	gthumb_text_item_set_markup_str (image->comment, "<i>%s</i>");

	c = truncate_comment_if_needed (gil, comment);
	gthumb_text_item_configure (image->comment,
				    0, 
				    0, 
				    gil->priv->max_image_width, 
				    DEFAULT_FONT,
				    c, 
				    FALSE,
				    gil->priv->static_text);
	g_free (c);

	/* Text item. */

	image->text = GTHUMB_TEXT_ITEM (gnome_canvas_item_new (
	       group,
	       GTHUMB_TYPE_TEXT_ITEM,
	       NULL));
	gthumb_text_item_configure (image->text,
				    0, 
				    0, 
				    gil->priv->max_image_width, 
				    DEFAULT_FONT,
				    text, 
				    gil->priv->is_editable, 
				    gil->priv->static_text);
			   
	/* Signals. */
	
	g_signal_connect (G_OBJECT (image->image), 
			  "event",
			  G_CALLBACK (image_event),
			  image);

	g_signal_connect (G_OBJECT (image->comment), 
			  "event",
			  G_CALLBACK (image_event),
			  image);
	
	image->text_event_id = 
		g_signal_connect (G_OBJECT (image->text), 
				  "event",
				  G_CALLBACK (image_event),
				  image);

	g_signal_connect (G_OBJECT (image->text), 
			  "editing_started",
			  G_CALLBACK (editing_started),
			  image);
	g_signal_connect (G_OBJECT (image->text), 
			  "editing_stopped",
			  G_CALLBACK (editing_stopped),
			  image);
	g_signal_connect (G_OBJECT (image->text), 
			  "text_changed",
			  G_CALLBACK (text_changed),
			  image);
	g_signal_connect (G_OBJECT (image->text), 
			  "height_changed",
			  G_CALLBACK (height_changed),
			  image);
	g_signal_connect (G_OBJECT (image->text), 
			  "width_changed",
			  G_CALLBACK (width_changed),
			  image);

	return image;
}


static int
image_list_append_image (ImageList *gil, 
			 Image *image)
{
	int pos;

	pos = gil->images++;
	gil->priv->image_list = g_list_append (gil->priv->image_list, image);

	switch (gil->priv->selection_mode) {
	case GTK_SELECTION_BROWSE:
		image_list_select_image (gil, 0);
		break;

	default:
		break;
	}

	if (! gil->priv->frozen) {
		gil_layout_from_line (gil, pos / gil_get_images_per_line(gil));
		gil_scrollbar_adjust (gil);
	} else
		gil->priv->dirty = TRUE;

	return gil->images - 1;
}


static void
image_list_insert_image (ImageList *gil, 
			 gint pos, 
			 Image *image)
{
	if (pos == gil->images) {
		image_list_append_image (gil, image);
		return;
	}

	gil->priv->image_list = g_list_insert (gil->priv->image_list, image, pos);
	gil->images++;

	switch (gil->priv->selection_mode) {
	case GTK_SELECTION_BROWSE:
		image_list_select_image (gil, 0);
		break;

	default:
		break;
	}

	if (! gil->priv->frozen) {
		gil_layout_from_line (gil, pos / gil_get_images_per_line(gil));
		gil_scrollbar_adjust (gil);
	} else
		gil->priv->dirty = TRUE;

	sync_selection (gil, pos, SYNC_INSERT);
}

	     
/**
 * image_list_insert:
 * @gil:  An image list.
 * @pos:  Position at which the new image should be inserted.
 * @im:   Imlib image with the image image.
 * @text: Text to be used for the image's caption.
 *
 * Inserts an image in the specified image list.  The image is created from the
 * specified Imlib image, and it is inserted at the @pos index.
 */
void
image_list_insert (ImageList *gil, 
		   gint pos, 
		   GdkPixbuf *pixbuf, 
		   const char *text,
		   const char *comment)
{
	Image *image;

	g_return_if_fail (IS_IMAGE_LIST (gil));
	g_return_if_fail (pixbuf != NULL);

	image = image_new_from_pixbuf (gil, pixbuf, text, comment);
	image_list_insert_image (gil, pos, image);
	return;
}


/**
 * image_list_append:
 * @gil:  An image list.
 * @im:   Imlib image with the image image.
 * @text: Text to be used for the image's caption.
 *
 * Appends an image to the specified image list.  The image is created from
 * the specified Imlib image.
 */
int
image_list_append (ImageList  *gil, 
		   GdkPixbuf  *pixbuf, 
		   const char *text,
		   const char *comment)
{
	Image *image;

	g_return_val_if_fail (IS_IMAGE_LIST (gil), -1);
	g_return_val_if_fail (pixbuf != NULL, -1);

	image = image_new_from_pixbuf (gil, pixbuf, text, comment);
	return image_list_append_image (gil, image);
}


static void
image_destroy (Image *image)
{
	if (image == NULL)
		return;

	if (image->destroy)
		(* image->destroy) (image->data);

	if (image->image != NULL) {
		gtk_object_destroy (GTK_OBJECT (image->image));
		image->image = NULL;
	}

	if (image->text != NULL) {
		gtk_object_destroy (GTK_OBJECT (image->text));
		image->text = NULL;
	}

	if (image->comment != NULL) {
		gtk_object_destroy (GTK_OBJECT (image->comment));
		image->comment = NULL;
	}

	g_free (image);
}

static void keep_focus_consistent (ImageList *gil);

/**
 * image_list_remove:
 * @gil: An image list.
 * @pos: Index of the image that should be removed.
 *
 * Removes the image at index position @pos.  If a destroy handler was
 * specified for that image, it will be called with the appropriate data.
 */
void
image_list_remove (ImageList *gil, gint pos)
{
	GList * list;
	Image * image;
	int     was_selected;

	g_return_if_fail (IS_IMAGE_LIST (gil));
	g_return_if_fail (pos >= 0 && pos < gil->images);

	was_selected = FALSE;
	list = g_list_nth (gil->priv->image_list, pos);
	image = list->data;

	if (gil->priv->focus_image == pos)
		gil->priv->focus_image = -1;

	if (image->selected) {
		was_selected = TRUE;

		switch (gil->priv->selection_mode) {
		case GTK_SELECTION_SINGLE:
		case GTK_SELECTION_BROWSE:
		case GTK_SELECTION_MULTIPLE:
			image_list_unselect_image (gil, pos);
			break;

		default:
			break;
		}
	}

	gil->priv->image_list = g_list_remove_link (gil->priv->image_list, 
						    list);
	g_list_free_1 (list);
	gil->images--;

	sync_selection (gil, pos, SYNC_REMOVE);

	if (was_selected) {
		switch (gil->priv->selection_mode) {
		case GTK_SELECTION_BROWSE:
			if (pos == gil->images)
				image_list_select_image (gil, pos - 1);
			else
				image_list_select_image (gil, pos);

			break;

		default:
			break;
		}
	}

	if (gil->images >= gil->priv->last_selected_idx)
		gil->priv->last_selected_idx = -1;

	if (gil->priv->last_selected_image == image)
		gil->priv->last_selected_image = NULL;

	/**/

	image_destroy (image);

	if (! gil->priv->frozen) {
		gil_layout_from_line (gil, pos / gil_get_images_per_line(gil));
		gil_scrollbar_adjust (gil);
		keep_focus_consistent (gil);
		
	} else {
		gil->priv->dirty = TRUE;
	}
}


/**
 * image_list_clear:
 * @gil: An image list.
 *
 * Clears the contents for the image list by removing all the images.  
 * If destroy handlers were specified for any of the images, they will be 
 * called with the appropriate data.
 */
void
image_list_clear (ImageList *gil)
{
	GList *l;

	g_return_if_fail (IS_IMAGE_LIST (gil));

	for (l = gil->priv->image_list; l; l = l->next)
		image_destroy (l->data);

	gil_free_line_info (gil);

	if (gil->selection != NULL) {
		g_list_free (gil->selection);
		gil->selection = NULL;
	}

	gil->priv->image_list = NULL;
	gil->images = 0;
	gil->priv->last_selected_idx = -1;
	gil->priv->last_selected_image = NULL;
	gil->priv->has_focus = FALSE;
	gil->priv->max_image_width = gil->priv->min_image_width;

	gtk_adjustment_set_value (gil->adj, 0);

	keep_focus_consistent (gil);
}


static void
gil_destroy (GtkObject *object)
{
	ImageList *gil;

	gil = IMAGE_LIST (object);

	if (gil->priv != NULL) {
		if (gil->priv->separators)
			g_free (gil->priv->separators);
		
		gil->priv->frozen = 1;
		gil->priv->dirty  = TRUE;
		image_list_clear (gil);

		if (gil->priv->timer_tag != 0) {
			g_source_remove (gil->priv->timer_tag);
			gil->priv->timer_tag = 0;
		}
		
		if (gil->adj) {
			g_object_unref (G_OBJECT (gil->adj));
			gil->adj = NULL;
		}
		
		if (gil->hadj) {
			g_object_unref (G_OBJECT (gil->hadj));
			gil->hadj = NULL;
		}
		
		if (gil->priv->old_text) {
			g_free (gil->priv->old_text);
			gil->priv->old_text = NULL;
		}
		
		if (gil->priv->target_list != NULL) {
			gtk_target_list_unref (gil->priv->target_list);
			gil->priv->target_list = NULL;
		}

		g_free (gil->priv);
		gil->priv = NULL;
	}

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(*GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}


void
image_list_focus_image (ImageList *gil, 
			gint idx)
{
        g_return_if_fail (IS_IMAGE_LIST (gil));
        g_return_if_fail (idx >= 0 && idx < gil->images);

        g_signal_emit (gil, gil_signals[FOCUS_IMAGE], 0, idx);
}


int 
image_list_get_focus_image (ImageList *gil)
{
        g_return_val_if_fail (IS_IMAGE_LIST (gil), -1);

	if (! GTK_WIDGET_HAS_FOCUS (GTK_WIDGET (gil)))
		return -1;
	else
		return gil->priv->focus_image;
}


static void
select_image (ImageList *gil, 
	      gint pos, 
	      GdkEvent *event)
{
	gint i;
	GList *list;
	Image *image;

	switch (gil->priv->selection_mode) {
	case GTK_SELECTION_SINGLE:
	case GTK_SELECTION_BROWSE:
		i = 0;

		for (list = gil->priv->image_list; list; list = list->next) {
			image = list->data;

			if (i != pos && image->selected)
				emit_select (gil, FALSE, i, event);

			i++;
		}

		emit_select (gil, TRUE, pos, event);
		break;

	case GTK_SELECTION_MULTIPLE:
		emit_select (gil, TRUE, pos, event);
		break;

	default:
		break;
	}
}


/**
 * gnome_image_list_select_image:
 * @gil: An image list.
 * @pos: Index of the image to be selected.
 *
 * Selects the image at the index specified by @pos.
 */
void
image_list_select_image (ImageList *gil, 
			 gint pos)
{
	g_return_if_fail (IS_IMAGE_LIST (gil));
	g_return_if_fail (pos >= 0 && pos < gil->images);

	select_image (gil, pos, NULL);
}


static void
unselect_image (ImageList *gil, 
		gint pos, 
		GdkEvent *event)
{
	switch (gil->priv->selection_mode) {
	case GTK_SELECTION_SINGLE:
	case GTK_SELECTION_BROWSE:
	case GTK_SELECTION_MULTIPLE:
		emit_select (gil, FALSE, pos, event);
		break;

	default:
		break;
	}
}


/**
 * gnome_image_list_unselect_image:
 * @gil: An image list.
 * @pos: Index of the image to be unselected.
 *
 * Unselects the image at the index specified by @pos.
 */
void
image_list_unselect_image (ImageList *gil, 
			   gint pos)
{
	g_return_if_fail (IS_IMAGE_LIST (gil));
	g_return_if_fail (pos >= 0 && pos < gil->images);

	unselect_image (gil, pos, NULL);
}


static void
gil_size_request (GtkWidget *widget, 
		  GtkRequisition *requisition)
{
	requisition->width = 1;
	requisition->height = 1;
}


static void
gil_size_allocate (GtkWidget *widget, 
		   GtkAllocation *allocation)
{
	ImageList *gil = IMAGE_LIST (widget);
	int        old_ipl;

	old_ipl = gil_get_images_per_line (gil);

	if (GTK_WIDGET_CLASS (parent_class)->size_allocate)
		(* GTK_WIDGET_CLASS (parent_class)->size_allocate) (widget, allocation);

	if (gil->priv->frozen)
		return;

	if (old_ipl != gil_get_images_per_line (gil))
		gil_layout_all_images (gil);
	gil_scrollbar_adjust (gil);
}


static int
to_255 (int v)
{
	return v * 255 / 65535;
}


static void
create_rubberband_rectangle (ImageList *gil)
{
	GnomeCanvasGroup *root;
	GdkColor          color;
	guint             fill_color;
	guint             outline_color;

	/*color = GTK_WIDGET (gil)->style->bg[GTK_STATE_SELECTED];*/
	color = GTK_WIDGET (gil)->style->base[GTK_STATE_SELECTED];
	fill_color = ((to_255 (color.red) << 24) 
		      | (to_255 (color.green) << 16) 
		      | (to_255 (color.blue) << 8)
		      | 0x00000040);
	outline_color = fill_color | 255;

	/**/

	gdk_colormap_alloc_color (gtk_widget_get_colormap (GTK_WIDGET (gil)),
				  &color,
				  TRUE,
				  TRUE);

	root = gnome_canvas_root (GNOME_CANVAS (gil));
	gil->priv->sel_rect = gnome_canvas_item_new (root,
		     eel_canvas_rect_get_type (),
		     "x1", 0.0,
		     "y1", 0.0,
		     "x2", 0.0,
		     "y2", 0.0,
		     "outline_color_rgba", outline_color,
		     "fill_color_rgba", fill_color,
		     "width_pixels", 1,
		     NULL);
	gnome_canvas_item_hide (GNOME_CANVAS_ITEM (gil->priv->sel_rect));
}


static void
gil_realize (GtkWidget *widget)
{
	ImageList *gil;
	GtkStyle  *style; 

	gil = IMAGE_LIST (widget);

	gil->priv->frozen++;

	if (GTK_WIDGET_CLASS (parent_class)->realize)
		(* GTK_WIDGET_CLASS (parent_class)->realize) (widget);

	gil->priv->frozen--;

	/* Change the style to use the base color as the background */

	style = gtk_style_copy (gtk_widget_get_style (widget));
	style->bg[GTK_STATE_NORMAL] = style->base[GTK_STATE_NORMAL];
	gtk_widget_set_style (widget, style);

	/**/

	create_rubberband_rectangle (gil);

	if (gil->priv->frozen)
		return;

	if (gil->priv->dirty) {
		gil_layout_all_images (gil);
		gil_scrollbar_adjust (gil);
	}
}


/* Returns whether the specified image is at least partially inside the 
 * specified rectangle.
 */
static gboolean
image_is_in_area (ImageList *gil,
		  Image     *image, 
		  int        x1, 
		  int        y1, 
		  int        x2, 
		  int        y2)
{
	double   ix1, iy1, ix2, iy2;   /* image bounds. */
	double   tx1, ty1, tx2, ty2;   /* text bounds. */
	double   cx1, cy1, cx2, cy2;   /* comment bounds. */
	double   sx1, sy1, sx2, sy2;   /* sensible rectangle. */
	double   x_ofs, y_ofs;
	gboolean view_text, view_comment;

	if (x1 == x2 && y1 == y2)
		return FALSE;

	image_get_view_mode (gil, image, &view_text, &view_comment);

	gnome_canvas_item_get_bounds (GNOME_CANVAS_ITEM (image->image), 
				      &ix1, &iy1, &ix2, &iy2);
	gnome_canvas_item_get_bounds (GNOME_CANVAS_ITEM (image->text), 
				      &tx1, &ty1, &tx2, &ty2);
	gnome_canvas_item_get_bounds (GNOME_CANVAS_ITEM (image->comment), 
				      &cx1, &cy1, &cx2, &cy2);

	sx1 = ix1;
	sy1 = iy1;
	sx2 = ix2;
	sy2 = iy2;

	if (view_text) {
		sx1 = MIN (sx1, tx1);
		sy1 = MIN (sy1, ty1);
		sx2 = MAX (sx2, tx2);
		sy2 = MAX (sy2, ty2);
	}

	if (view_comment) {
		sx1 = MIN (sx1, cx1);
		sy1 = MIN (sy1, cy1);
		sx2 = MAX (sx2, cx2);
		sy2 = MAX (sy2, cy2);
	}

	x_ofs = (sx2 - sx1) / 6;
	y_ofs = (sy2 - sy1) / 6;

	sx1 += x_ofs;
	sx2 -= x_ofs;
	sy1 += y_ofs;
	sy2 -= y_ofs;
	
	if (sx1 <= x2 && sy1 <= y2 && sx2 >= x1 && sy2 >= y1)
		return TRUE;

	return FALSE;
}


static void
get_image_center (Image *image, int *x, int *y)
{
	double x1, x2, y1, y2;
	
	gnome_canvas_item_get_bounds (GNOME_CANVAS_ITEM (image->image), 
				      &x1, &y1, &x2, &y2);

	*x = (int) ((x1 + x2) / 2);
	*y = (int) ((y1 + y2) / 2);
}

static int get_first_visible_at_offset (ImageList *gil, gdouble ofs);
static int get_last_visible_at_offset (ImageList *gil, gdouble ofs);

static void
select_range_with_keyboard (ImageList *gil, 
			    int        new_focus_image)
{
	int     x1, x2, y1, y2;
	GList  *l, *begin, *end;
	int     i, begin_idx, end_idx;
	Image  *image1, *image2;
	int     min_y, max_y;
	double  old_y1_d, old_y2_d;

	l = g_list_nth (gil->priv->image_list, gil->priv->old_focus_image);
	image1 = (Image*) l->data;

	l = g_list_nth (gil->priv->image_list, new_focus_image);
	image2 = (Image*) l->data;

	get_image_center (image1, &x1, &y1);
	get_image_center (image2, &x2, &y2);

	if (x1 > x2) {
		int t = x1;
		x1 = x2;
		x2 = t;
	}

	if (y1 > y2) {
		int t = y1;
		y1 = y2;
		y2 = t;
	}

	x1 -= SELECTION_BORDER;
	y1 -= SELECTION_BORDER;
	x2 += SELECTION_BORDER;
	y2 += SELECTION_BORDER;

	g_object_get (G_OBJECT (gil->priv->sel_rect),
		      "y1", &old_y1_d,
		      "y2", &old_y2_d,
		      NULL);
	min_y = MIN (y1, (int) old_y1_d);
	max_y = MAX (y2, (int) old_y2_d);

	gnome_canvas_item_set (gil->priv->sel_rect,
			       "x1", (double) x1,
			       "y1", (double) y1,
			       "x2", (double) x2,
			       "y2", (double) y2,
			       NULL);

	/* Consider only images in the min_y --> max_y offset. */

	begin_idx = get_first_visible_at_offset (gil, min_y);
	begin = g_list_nth (gil->priv->image_list, begin_idx);
	end_idx = get_last_visible_at_offset (gil, max_y);
	end = g_list_nth (gil->priv->image_list, end_idx);

	if (end != NULL)
		end = end->next;

	for (l = begin, i = begin_idx; l != end; l = l->next, i++) {
		Image *image = l->data;

		if (image_is_in_area (gil, image, x1, y1, x2, y2)) 
			image_list_select_image (gil, i);
		else
			image_list_unselect_image (gil, i);
	}
}


static GList *
get_line_from_image (ImageList *gil, 
		     int        focus_image)
{
	Image *image;
	GList *scan;

	scan = g_list_nth (gil->priv->image_list, focus_image);
	g_return_val_if_fail (scan != NULL, NULL);

	image = (Image*) scan->data;

	for (scan = gil->priv->lines; scan; scan = scan->next) {
		ImageLine *line = (ImageLine*) scan->data;
		
		if (g_list_find (line->line_images, image) != NULL)
			return scan;
	}

	return NULL;
}


static int
get_page_distance_image (ImageList *gil,
			 int        focus_image,
			 gboolean   downward)
{
	int    old_focus_image = focus_image;
	GList *line;
	int    h;
	int    d;

	d = downward ? 1 : -1;
	h = GTK_WIDGET (gil)->allocation.height;
	line = get_line_from_image (gil, focus_image);
	while ((h > 0) && line) {
		ImageLine *image_line;
		int        line_height;

		image_line = (ImageLine *) line->data;
		line_height = image_line_height (gil, image_line);
		h -= line_height;

		if (h > 0) {
			int tmp;
			tmp = focus_image + d * gil_get_images_per_line (gil);
			if ((tmp > gil->images - 1) || (tmp < 0))
				return focus_image;
			else
				focus_image = tmp;
		}

		if (downward)
			line = line->next;
		else
			line = line->prev;
	}

	if (old_focus_image == focus_image) {
		int tmp;
		tmp = focus_image + d * gil_get_images_per_line (gil);
		if ((tmp > gil->images - 1) || (tmp < 0))
			return focus_image;
		else
			focus_image = tmp;
	}

	return focus_image;
}


static void
real_move_cursor (ImageList             *gil, 
		  GthumbCursorMovement   dir,
		  GthumbSelectionChange  sel_change)
{
	int images_per_line;
	int new_focus_image;

	if (gil->images == 0)
		return;

	images_per_line = gil_get_images_per_line (gil);
        new_focus_image = gil->priv->focus_image;

	if (gil->priv->focus_image == -1) {
		gil->priv->old_focus_image = 0;
		new_focus_image = 0;
	} else {
		switch (dir) {
		case GTHUMB_CURSOR_MOVE_RIGHT:
			if (gil->priv->focus_image + 1 < gil->images &&
			    gil->priv->focus_image % images_per_line != (images_per_line - 1))
				new_focus_image++;
			break;
			
		case GTHUMB_CURSOR_MOVE_LEFT:
			if (gil->priv->focus_image - 1 >= 0 &&
			    gil->priv->focus_image % images_per_line != 0)
				new_focus_image--;
			break;
			
		case GTHUMB_CURSOR_MOVE_DOWN:
			if (gil->priv->focus_image + images_per_line < gil->images)
				new_focus_image += images_per_line;
			break;
			
		case GTHUMB_CURSOR_MOVE_UP:
			if (gil->priv->focus_image - images_per_line >= 0)
				new_focus_image -= images_per_line;
			break;
			
		case GTHUMB_CURSOR_MOVE_PAGE_UP:
			new_focus_image = get_page_distance_image (gil,
								   new_focus_image,
								   FALSE);
			new_focus_image = CLAMP (new_focus_image, 0, gil->images - 1);
			break;
			
		case GTHUMB_CURSOR_MOVE_PAGE_DOWN:
			new_focus_image = get_page_distance_image (gil,
								   new_focus_image,
								   TRUE);
			new_focus_image = CLAMP (new_focus_image, 0, gil->images - 1);
			break;
			
		case GTHUMB_CURSOR_MOVE_BEGIN:
			new_focus_image = 0;
			break;
			
		case GTHUMB_CURSOR_MOVE_END:
			new_focus_image = gil->images - 1;
			break;
			
		default:
			break;
		}
	}
	
	if ((dir == GTHUMB_CURSOR_MOVE_UP)
	    || (dir == GTHUMB_CURSOR_MOVE_DOWN)
	    || (dir == GTHUMB_CURSOR_MOVE_PAGE_UP)
	    || (dir == GTHUMB_CURSOR_MOVE_PAGE_DOWN)
	    || (dir == GTHUMB_CURSOR_MOVE_BEGIN)
	    || (dir == GTHUMB_CURSOR_MOVE_END)) { /* Vertical Movement */
		GthumbVisibility visibility;
		gboolean is_up_direction;

		visibility = image_list_image_is_visible (gil, new_focus_image);
		is_up_direction = ((dir == GTHUMB_CURSOR_MOVE_UP)
				   || (dir == GTHUMB_CURSOR_MOVE_PAGE_UP)
				   || (dir == GTHUMB_CURSOR_MOVE_BEGIN));

		if (visibility != GTHUMB_VISIBILITY_FULL) 
			image_list_moveto (gil, new_focus_image, 
					   is_up_direction ? 0.0 : 1.0);
	} else {
		GthumbVisibility visibility;
		visibility = image_list_image_is_visible (gil, new_focus_image);

		if (visibility != GTHUMB_VISIBILITY_FULL) {
			double offset;

			switch (visibility) {
			case GTHUMB_VISIBILITY_NONE:
				offset = 0.5; 
				break;
			case GTHUMB_VISIBILITY_PARTIAL_TOP:
				offset = 0.0; 
				break;
			case GTHUMB_VISIBILITY_PARTIAL_BOTTOM:
				offset = 1.0; 
				break;
			case GTHUMB_VISIBILITY_PARTIAL:
			case GTHUMB_VISIBILITY_FULL:
				offset = -1.0;
				break;
			}
			if (offset > -1.0)
				image_list_moveto (gil, 
						   new_focus_image, 
						   offset);
		}
	}

	if (sel_change == GTHUMB_SELCHANGE_SET) {
		image_list_unselect_all (gil, NULL, NULL);
		image_list_select_image (gil, new_focus_image);
	} else if (sel_change == GTHUMB_SELCHANGE_SET_RANGE) 
		select_range_with_keyboard (gil, new_focus_image);

	image_list_focus_image (gil, new_focus_image);
}


static void
real_add_cursor_selection (ImageList *gil)
{
	if (gil->priv->focus_image == -1)
		return;
	image_list_select_image (gil, gil->priv->focus_image);
}


static void
real_toggle_cursor_selection (ImageList *gil)
{
	GList *scan;
	Image *image;

	if (gil->priv->focus_image == -1)
		return;

	scan = g_list_nth (gil->priv->image_list, gil->priv->focus_image);
	g_return_if_fail (scan != NULL);
	image = scan->data;

	if (image->selected)
		image_list_unselect_image (gil, gil->priv->focus_image);
	else
		image_list_select_image (gil, gil->priv->focus_image);
}


static void
real_select_image (ImageList *gil, 
		   int        num, 
		   GdkEvent  *event)
{
	GList *scan;
	Image *image;

	g_return_if_fail (IS_IMAGE_LIST (gil));
	g_return_if_fail (num >= 0 && num < gil->images);

	scan = g_list_nth (gil->priv->image_list, num);
	g_return_if_fail (scan != NULL);
	image = scan->data;

	if (image->selected)
		return;
	image->selected = TRUE;

	gthumb_text_item_select (image->text, TRUE);
	gthumb_text_item_select (image->comment, TRUE);
	gnome_canvas_thumb_select (image->image, TRUE);

	gil->selection = g_list_prepend (gil->selection, GINT_TO_POINTER (num));
}


static void
real_unselect_image (ImageList *gil, 
		     int        num, 
		     GdkEvent  *event)
{
	Image *image;
	GList *scan;

	g_return_if_fail (IS_IMAGE_LIST (gil));
	g_return_if_fail (num >= 0 && num < gil->images);

	scan = g_list_nth (gil->priv->image_list, num);
	g_return_if_fail (scan != NULL);
	image = scan->data;

	if (!image->selected)
		return;
	image->selected = FALSE;

	gthumb_text_item_select (image->text, FALSE);
	gthumb_text_item_select (image->comment, FALSE); 
	gnome_canvas_thumb_select (image->image, FALSE);

	gil->selection = g_list_remove (gil->selection, GINT_TO_POINTER (num));
}


static void
real_focus_image (ImageList *gil, 
		  int        num)
{
        Image *image;
	GList *scan;

        if (gil->priv->focus_image >= 0) {
		scan = g_list_nth (gil->priv->image_list, gil->priv->focus_image);
		if (scan != NULL) {
			image = (Image *) scan->data;
			gthumb_text_item_focus (image->text, FALSE);
		}
        }

	scan = g_list_nth (gil->priv->image_list, num);
	g_return_if_fail (scan != NULL);
	image = (Image *) scan->data;

	gthumb_text_item_focus (image->text, TRUE);

	gil->priv->focus_image = num;
}


/* Saves the selection of the image list to temporary storage */
static void
store_temp_selection (ImageList *gil)
{
	GList *l;

	for (l = gil->priv->image_list; l; l = l->next) {
		Image *image = l->data;
		image->tmp_selected = image->selected;
	}
}


/* Button press handler for the image list */
static gint
gil_button_press (GtkWidget      *widget, 
		  GdkEventButton *event)
{
	ImageList *gil;
	int        only_one;
	double     tx, ty;
	int        pos;

	gil = IMAGE_LIST (widget);

	if (event->window == GTK_LAYOUT (widget)->bin_window) {
		if (! GTK_WIDGET_HAS_FOCUS (widget))
			gtk_widget_grab_focus (widget);
        }

	/* Invoke the canvas event handler and see if an item picks up the 
	 * event */
	(* GTK_WIDGET_CLASS (parent_class)->button_press_event) (widget, event);

	if ((event->type == GDK_2BUTTON_PRESS) || 
	    (event->type == GDK_3BUTTON_PRESS)) {
		/* stop_selection (gil, event->time); */
		return FALSE;
	}

	pos = image_list_get_image_at (gil, event->x, event->y);

	if (pos != -1) {
		if (event->button == 1) {
			/* This can be the start of a dragging action. */
			gil->priv->dragging = TRUE;
			gil->priv->drag_start_x = event->x;
			gil->priv->drag_start_y = event->y;
		}

		return FALSE;
	}

	if ((gil->priv->editing_image != NULL) 
	    && (event->type == GDK_BUTTON_PRESS)) {
		gthumb_text_item_stop_editing (gil->priv->editing_image->text, 
					       FALSE);
		gil->priv->editing_image = NULL;
	}

	/**/

	if (! (event->type == GDK_BUTTON_PRESS
	       && event->button == 1
	       && gil->priv->selection_mode != GTK_SELECTION_BROWSE))
		return FALSE;

	only_one = gil->priv->selection_mode == GTK_SELECTION_SINGLE;

	if (only_one || 
	    (event->state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK)) == 0)
		image_list_unselect_all (gil, NULL, NULL);

	if (only_one)
		return FALSE;

	if (gil->priv->selecting)
		return FALSE;

	gnome_canvas_window_to_world (GNOME_CANVAS (gil), 
				      event->x, event->y, 
				      &tx, &ty);
	gil->priv->sel_start_x = tx;
	gil->priv->sel_start_y = ty;
	gil->priv->sel_state = event->state;
	gil->priv->selecting = TRUE;

	store_temp_selection (gil);

	gnome_canvas_item_set (gil->priv->sel_rect,
			       "x1", tx,
			       "y1", ty,
			       "x2", tx,
			       "y2", ty,
			       NULL);
	gnome_canvas_item_raise_to_top (gil->priv->sel_rect);
	gnome_canvas_item_show (gil->priv->sel_rect);

	gnome_canvas_item_grab (gil->priv->sel_rect, 
				(GDK_POINTER_MOTION_MASK 
				 | GDK_BUTTON_RELEASE_MASK),
				NULL, 
				event->time);

	return FALSE;
}


/* Updates the rubberband selection to the specified point */
static void
update_drag_selection (ImageList *gil, 
		       int        x, 
		       int        y)
{
	gint    x1, x2, y1, y2;
	GList  *l, *begin, *end;
	gint    i, begin_idx, end_idx;
	Image  *image;
	gint    additive, invert;
	gint    min_y, max_y;
	double  old_y1_d, old_y2_d;

	/* Update rubberband */

	if (gil->priv->sel_start_x < x) {
		x1 = gil->priv->sel_start_x;
		x2 = x;
	} else {
		x1 = x;
		x2 = gil->priv->sel_start_x;
	}

	if (gil->priv->sel_start_y < y) {
		y1 = gil->priv->sel_start_y;
		y2 = y;
	} else {
		y1 = y;
		y2 = gil->priv->sel_start_y;
	}

	if (x1 < 0)
		x1 = 0;

	if (y1 < 0)
		y1 = 0;

	if (x2 >= GTK_WIDGET (gil)->allocation.width)
		x2 = GTK_WIDGET (gil)->allocation.width - 1;

	if (y2 >= gil->priv->total_height)
		y2 = gil->priv->total_height - 1;

	g_object_get (G_OBJECT (gil->priv->sel_rect),
		      "y1", &old_y1_d,
		      "y2", &old_y2_d,
		      NULL);
	min_y = MIN (y1, (int) old_y1_d);
	max_y = MAX (y2, (int) old_y2_d);

	gnome_canvas_item_set (gil->priv->sel_rect,
			       "x1", (double) x1,
			       "y1", (double) y1,
			       "x2", (double) x2,
			       "y2", (double) y2,
			       NULL);

	/* Select or unselect images as appropriate */

	additive = gil->priv->sel_state & GDK_SHIFT_MASK;
	invert = gil->priv->sel_state & GDK_CONTROL_MASK;

	/* Consider only images in the min_y --> max_y offset. */

	begin_idx = get_first_visible_at_offset (gil, min_y);
	begin = g_list_nth (gil->priv->image_list, begin_idx);
	end_idx = get_last_visible_at_offset (gil, max_y);
	end = g_list_nth (gil->priv->image_list, end_idx);

	if (end != NULL)
		end = end->next;

	for (l = begin, i = begin_idx; l != end; l = l->next, i++) {
		image = l->data;

		if (image_is_in_area (gil, image, x1, y1, x2, y2)) {
			if (invert) {
				if (image->selected == image->tmp_selected)
					emit_select (gil, !image->selected, i, NULL);
			} else if (additive) {
				if (!image->selected)
					emit_select (gil, TRUE, i, NULL);
			} else {
				if (!image->selected)
					emit_select (gil, TRUE, i, NULL);
			}
		} else if (image->selected != image->tmp_selected)
			emit_select (gil, image->tmp_selected, i, NULL);
	}
}


/* Button release handler for the image list */
static gint
gil_button_release (GtkWidget *widget, GdkEventButton *event)
{
	ImageList *gil;
	gdouble    x, y;

	gil = IMAGE_LIST (widget);

	if (! gil->priv->selecting)
		(* GTK_WIDGET_CLASS (parent_class)->button_release_event) (widget, event);

	if (gil->priv->dragging) {
		gil->priv->dragging = FALSE;
		gil->priv->drag_started = FALSE;
		return FALSE;
	}

	if ((event->button != 1) || ! gil->priv->selecting)
		return FALSE;

	gnome_canvas_window_to_world (GNOME_CANVAS (gil), event->x, event->y, 
				      &x, &y);

	update_drag_selection (gil, x, y);
	stop_selection (gil, event->time);

	return FALSE;
}


/* Autoscroll timeout handler for the image list */
static gint
scroll_timeout (gpointer data)
{
	ImageList *gil;
	double     x, y;
	int        value;

	GDK_THREADS_ENTER ();

	gil = data;

	value = gil->adj->value + gil->priv->value_diff;
	if (value > gil->adj->upper - gil->adj->page_size)
		value = gil->adj->upper - gil->adj->page_size;

	gtk_adjustment_set_value (gil->adj, value);

	gil->priv->event_last_y = gil->priv->event_last_y + gil->priv->value_diff;
	gnome_canvas_window_to_world (GNOME_CANVAS (gil),
				      gil->priv->event_last_x, gil->priv->event_last_y,
				      &x, &y);
	update_drag_selection (gil, x, y);

	GDK_THREADS_LEAVE();

	return TRUE;
}


/* Motion event handler for the image list */
static gint
gil_motion_notify (GtkWidget      *widget, 
		   GdkEventMotion *event)
{
	ImageList     *gil;
	gdouble        x, y;
	gint           absolute_y;
	GtkAdjustment *adj;

	gil = IMAGE_LIST (widget);

	if (! gil->priv->selecting && ! gil->priv->dragging)
		return (* GTK_WIDGET_CLASS (parent_class)->motion_notify_event) (widget, event);

	if (gil->priv->dragging) {
		if (! gil->priv->drag_started
		    && gtk_drag_check_threshold (widget, 
						 gil->priv->drag_start_x,
						 gil->priv->drag_start_y,
						 event->x,
						 event->y)) {
			int             pos;
			GdkDragContext *context;
			gboolean        multi_dnd;

			/**/

			pos = image_list_get_image_at (gil, 
						       gil->priv->drag_start_x,
						       gil->priv->drag_start_y);
			if (pos != -1)
				image_list_focus_image (gil, pos);

			/**/

			gil->priv->drag_started = TRUE;
			context = gtk_drag_begin (widget,
						  gil->priv->target_list,
						  GDK_ACTION_MOVE,
						  1,
						  (GdkEvent *) event);

			multi_dnd = gil->selection->next != NULL;
			gtk_drag_set_icon_stock (context,
						 multi_dnd ? GTK_STOCK_DND_MULTIPLE : GTK_STOCK_DND,
						 -4, -4);

			return TRUE;
		}
		return TRUE;
	}

	gnome_canvas_window_to_world (GNOME_CANVAS (gil), event->x, event->y, &x, &y);
	update_drag_selection (gil, x, y);

	adj = gtk_layout_get_vadjustment (GTK_LAYOUT (gil));
	absolute_y = event->y - adj->value;

	/* If we are out of bounds, schedule a timeout that will do the 
	 * scrolling */

	if (absolute_y < 0 || absolute_y > widget->allocation.height) {
		gil->priv->event_last_x = event->x;
		gil->priv->event_last_y = event->y;

		/* Make the steppings be relative to the mouse distance from 
		 * the canvas.  
		 * Also notice the timeout below is small to give a
		 * more smooth movement.
		 */
		if (absolute_y < 0)
			gil->priv->value_diff = absolute_y;
		else
			gil->priv->value_diff = absolute_y - widget->allocation.height;
		gil->priv->value_diff /= 2;

		if (gil->priv->timer_tag == 0)
			gil->priv->timer_tag = g_timeout_add (SCROLL_TIMEOUT, scroll_timeout, gil);
	} else if (gil->priv->timer_tag != 0) {
		g_source_remove (gil->priv->timer_tag);
		gil->priv->timer_tag = 0;
	}

	return TRUE;
}


static void
keep_focus_consistent (ImageList *gil)
{
	if (gil->priv->focus_image > gil->images - 1)
		gil->priv->focus_image = - 1;

	/* FIXME
	if (gil->priv->focus_image > gil->images - 1)
		gil->priv->focus_image = gil->images - 1;
	
	if (gil->priv->focus_image != -1)
		image_list_focus_image (gil, gil->priv->focus_image);
	*/
}


static gint 
gil_focus_in (GtkWidget *widget,
	      GdkEventFocus *event)
{
	ImageList *gil = IMAGE_LIST (widget);

	GTK_WIDGET_SET_FLAGS (widget, GTK_HAS_FOCUS);

	keep_focus_consistent (IMAGE_LIST (widget));
	if ((gil->priv->focus_image == -1) && (gil->images > 0)) 
		image_list_focus_image (gil, 0);
	gtk_widget_queue_draw (widget);

	return TRUE;
}


static gint 
gil_focus_out (GtkWidget *widget,
	       GdkEventFocus *event)
{
	GTK_WIDGET_UNSET_FLAGS (widget, GTK_HAS_FOCUS);
	gtk_widget_queue_draw (widget);

	return TRUE;
}


static gint
gil_key_press (GtkWidget *widget,
               GdkEventKey *event)
{
	ImageList *gil = IMAGE_LIST (widget);
	gboolean handled;

	if (! gil->priv->multi_selecting_with_keyboard
	    && (event->state & GDK_SHIFT_MASK)
	    && ((event->keyval == GDK_Left)
		|| (event->keyval == GDK_Right)
		|| (event->keyval == GDK_Up)
		|| (event->keyval == GDK_Down)
		|| (event->keyval == GDK_Page_Up)
		|| (event->keyval == GDK_Page_Down)
		|| (event->keyval == GDK_Home)
		|| (event->keyval == GDK_End))) {
		gil->priv->multi_selecting_with_keyboard = TRUE;
		gil->priv->old_focus_image = gil->priv->focus_image;

		gnome_canvas_item_set (gil->priv->sel_rect,
				       "x1", 0.0,
				       "y1", 0.0,
				       "x2", 0.0,
				       "y2", 0.0,
				       NULL);
		gnome_canvas_item_raise_to_top (gil->priv->sel_rect);
		gnome_canvas_item_show (gil->priv->sel_rect);

	}

	handled = gtk_bindings_activate (GTK_OBJECT (widget),
					 event->keyval,
					 event->state);

	if (handled) 
		return TRUE;

	if (GTK_WIDGET_CLASS (parent_class)->key_press_event &&
	    GTK_WIDGET_CLASS (parent_class)->key_press_event (widget, event))
		return TRUE;

	return FALSE;
}


static gint
gil_key_release (GtkWidget *widget,
		 GdkEventKey *event)
{
	ImageList *gil = IMAGE_LIST (widget);

	if (gil->priv->multi_selecting_with_keyboard
	    && (event->state & GDK_SHIFT_MASK)
	    && ((event->keyval == GDK_Shift_L)
		|| (event->keyval == GDK_Shift_R))) {
		gil->priv->multi_selecting_with_keyboard = FALSE;
		gnome_canvas_item_hide (gil->priv->sel_rect);
	}

	if (GTK_WIDGET_CLASS (parent_class)->key_press_event &&
	    GTK_WIDGET_CLASS (parent_class)->key_press_event (widget, event))
		return TRUE;

	return FALSE;
}


static void
gil_set_scroll_adjustments (GtkLayout *layout,
			    GtkAdjustment *hadjustment,
			    GtkAdjustment *vadjustment)
{
	ImageList *gil = IMAGE_LIST (layout);

	image_list_set_hadjustment (gil, hadjustment);
	image_list_set_vadjustment (gil, vadjustment);
}


static gint
gil_scroll (GtkWidget *widget, 
	    GdkEventScroll *event)
{
	gdouble new_value;
	GtkAdjustment *adj;

	if (event->direction != GDK_SCROLL_UP &&
	    event->direction != GDK_SCROLL_DOWN)
		return FALSE;

	adj = IMAGE_LIST (widget)->adj;

	if (event->direction == GDK_SCROLL_UP)
		new_value = adj->value - adj->page_increment / 2;
	else
		new_value = adj->value + adj->page_increment / 2;

	new_value = CLAMP (new_value, adj->lower, adj->upper - adj->page_size);
	gtk_adjustment_set_value (adj, new_value);

	return TRUE;
}


static void
add_move_binding (GtkBindingSet *binding_set,
                  guint keyval,
		  GthumbCursorMovement dir)
{
        gtk_binding_entry_add_signal (binding_set, keyval, 0, 
				      "move_cursor", 2,
                                      G_TYPE_ENUM, dir,
				      G_TYPE_ENUM, GTHUMB_SELCHANGE_SET);

        gtk_binding_entry_add_signal (binding_set, keyval, GDK_CONTROL_MASK, 
				      "move_cursor", 2,
                                      G_TYPE_ENUM, dir,
				      G_TYPE_ENUM, GTHUMB_SELCHANGE_NONE);

        gtk_binding_entry_add_signal (binding_set, keyval, GDK_SHIFT_MASK, 
				      "move_cursor", 2,
                                      G_TYPE_ENUM, dir,
				      G_TYPE_ENUM, GTHUMB_SELCHANGE_SET_RANGE);
}


static GObject *
gil_constructor (GType type,
		 guint n_properties,
		 GObjectConstructParam *properties)
{
        GObject *gil;

        gil = G_OBJECT_CLASS (parent_class)->constructor (type,
                                                          n_properties,
                                                          properties);

	gtk_widget_add_events (GTK_WIDGET (gil), GDK_BUTTON_RELEASE_MASK);

        gnome_canvas_set_scroll_region (GNOME_CANVAS (gil),
					0.0, 0.0, 
					1000000.0, 1000000.0);
        gnome_canvas_scroll_to (GNOME_CANVAS (gil), 0, 0);

        return gil;
}


static void
gil_class_init (ImageListClass *gil_class)
{
	GObjectClass     *gobject_class;
	GtkObjectClass   *object_class;
	GtkWidgetClass   *widget_class;
	GtkLayoutClass   *layout_class;
	GnomeCanvasClass *canvas_class;
	GtkBindingSet    *binding_set;

	gobject_class = (GObjectClass *)     gil_class;
	object_class  = (GtkObjectClass *)   gil_class;
	widget_class  = (GtkWidgetClass *)   gil_class;
	layout_class  = (GtkLayoutClass *)   gil_class;
	canvas_class  = (GnomeCanvasClass *) gil_class;

	parent_class = g_type_class_peek_parent (gil_class);

	gil_signals[SELECT_IMAGE] =
		g_signal_new (
			"select_image",
			G_TYPE_FROM_CLASS (gobject_class),
			G_SIGNAL_RUN_FIRST,
			G_STRUCT_OFFSET (ImageListClass, select_image),
			NULL, NULL,
			gthumb_marshal_VOID__INT_BOXED,
			G_TYPE_NONE, 2,
			G_TYPE_INT,
			GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
	gil_signals[UNSELECT_IMAGE] =
		g_signal_new (
			"unselect_image",
			G_TYPE_FROM_CLASS (gobject_class),
			G_SIGNAL_RUN_FIRST,
			G_STRUCT_OFFSET (ImageListClass, unselect_image),
			NULL, NULL,
			gthumb_marshal_VOID__INT_BOXED,
			G_TYPE_NONE, 2,
			G_TYPE_INT,
			GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
	gil_signals[FOCUS_IMAGE] =
		g_signal_new (
		      "focus_image",
		      G_TYPE_FROM_CLASS (gobject_class),
		      G_SIGNAL_RUN_FIRST,
		      G_STRUCT_OFFSET (ImageListClass, focus_image),
		      NULL, NULL,
		      g_cclosure_marshal_VOID__INT,
		      G_TYPE_NONE, 1,
		      G_TYPE_INT);
	gil_signals[TEXT_CHANGED] =
		g_signal_new (
			"text_changed",
			G_TYPE_FROM_CLASS (gobject_class),
			G_SIGNAL_RUN_LAST,
			G_STRUCT_OFFSET (ImageListClass, text_changed),
			NULL, NULL,
			gthumb_marshal_BOOLEAN__INT_POINTER,
			G_TYPE_BOOLEAN, 2,
			G_TYPE_INT,
			G_TYPE_POINTER);
	gil_signals[DOUBLE_CLICK] =
		g_signal_new (
			"double_click",
			G_TYPE_FROM_CLASS (gobject_class),
			G_SIGNAL_RUN_LAST,
			G_STRUCT_OFFSET (ImageListClass, double_click),
			NULL, NULL,
			g_cclosure_marshal_VOID__INT,
			G_TYPE_NONE, 1,
			G_TYPE_INT);
	gil_signals[SELECT_ALL] =
		g_signal_new (
			"select_all",
			G_TYPE_FROM_CLASS (gobject_class),
			G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
			G_STRUCT_OFFSET (ImageListClass, select_all),
			NULL, NULL,
			gthumb_marshal_VOID__VOID,
			G_TYPE_NONE, 0);
        gil_signals[MOVE_CURSOR] =
                g_signal_new ("move_cursor",
                              G_TYPE_FROM_CLASS (gobject_class),
                              G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                              G_STRUCT_OFFSET (ImageListClass, move_cursor),
                              NULL, NULL,
                              gthumb_marshal_VOID__ENUM_ENUM,
			      G_TYPE_NONE, 2,
			      GTHUMB_TYPE_CURSOR_MOVEMENT,
                              GTHUMB_TYPE_SELECTION_CHANGE);
        gil_signals[ADD_CURSOR_SELECTION] =
                g_signal_new ("add_cursor_selection",
                              G_TYPE_FROM_CLASS (gobject_class),
                              G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                              G_STRUCT_OFFSET (ImageListClass, add_cursor_selection),
                              NULL, NULL,
                              gthumb_marshal_VOID__VOID,
                              G_TYPE_NONE, 0);
        gil_signals[TOGGLE_CURSOR_SELECTION] =
                g_signal_new ("toggle_cursor_selection",
                              G_TYPE_FROM_CLASS (gobject_class),
                              G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                              G_STRUCT_OFFSET (ImageListClass, toggle_cursor_selection),
                              NULL, NULL,
                              gthumb_marshal_VOID__VOID,
                              G_TYPE_NONE, 0);

	object_class->destroy = gil_destroy;

	gobject_class->constructor = gil_constructor;

	widget_class->size_request         = gil_size_request;
	widget_class->size_allocate        = gil_size_allocate;
	widget_class->realize              = gil_realize;
	widget_class->button_press_event   = gil_button_press;
	widget_class->button_release_event = gil_button_release;
	widget_class->motion_notify_event  = gil_motion_notify;
	widget_class->focus_in_event       = gil_focus_in;
	widget_class->focus_out_event      = gil_focus_out;
	widget_class->key_press_event      = gil_key_press;
	widget_class->key_release_event    = gil_key_release;
	widget_class->scroll_event         = gil_scroll;

	/* we override GtkLayout's set_scroll_adjustments signal instead
	 * of creating a new signal so as to keep binary compatibility.
	 * Anyway, a widget class only needs one of these signals, and
	 * this gives the correct implementation for ImageList */
	layout_class->set_scroll_adjustments = gil_set_scroll_adjustments;

	gil_class->select_image            = real_select_image;
	gil_class->unselect_image          = real_unselect_image;
	gil_class->focus_image             = real_focus_image;
	gil_class->select_all              = image_list_select_all;
	gil_class->move_cursor             = real_move_cursor;
        gil_class->add_cursor_selection    = real_add_cursor_selection;
        gil_class->toggle_cursor_selection = real_toggle_cursor_selection;

	/* Add key bindings */

        binding_set = gtk_binding_set_by_class (gil_class);

	add_move_binding (binding_set, GDK_Right, GTHUMB_CURSOR_MOVE_RIGHT);
	add_move_binding (binding_set, GDK_KP_Right, GTHUMB_CURSOR_MOVE_RIGHT);
	add_move_binding (binding_set, GDK_Left, GTHUMB_CURSOR_MOVE_LEFT);
	add_move_binding (binding_set, GDK_KP_Left, GTHUMB_CURSOR_MOVE_LEFT);
	add_move_binding (binding_set, GDK_Down, GTHUMB_CURSOR_MOVE_DOWN);
	add_move_binding (binding_set, GDK_KP_Down, GTHUMB_CURSOR_MOVE_DOWN);
	add_move_binding (binding_set, GDK_Up, GTHUMB_CURSOR_MOVE_UP);
	add_move_binding (binding_set, GDK_KP_Up, GTHUMB_CURSOR_MOVE_UP);
	add_move_binding (binding_set, GDK_Page_Up, GTHUMB_CURSOR_MOVE_PAGE_UP);
	add_move_binding (binding_set, GDK_KP_Page_Up, GTHUMB_CURSOR_MOVE_PAGE_UP);
	add_move_binding (binding_set, GDK_Page_Down, GTHUMB_CURSOR_MOVE_PAGE_DOWN);
	add_move_binding (binding_set, GDK_KP_Page_Down, GTHUMB_CURSOR_MOVE_PAGE_DOWN);
	add_move_binding (binding_set, GDK_Home, GTHUMB_CURSOR_MOVE_BEGIN);
	add_move_binding (binding_set, GDK_KP_Home, GTHUMB_CURSOR_MOVE_BEGIN);
	add_move_binding (binding_set, GDK_End, GTHUMB_CURSOR_MOVE_END);
	add_move_binding (binding_set, GDK_KP_End, GTHUMB_CURSOR_MOVE_END);

        gtk_binding_entry_add_signal (binding_set, GDK_space, 0, 
				      "add_cursor_selection", 0);
        gtk_binding_entry_add_signal (binding_set, GDK_space, GDK_CONTROL_MASK,
				      "toggle_cursor_selection", 0);
        gtk_binding_entry_add_signal (binding_set, GDK_a, GDK_CONTROL_MASK,
				      "select_all", 0);
}


static gint
default_compare (gconstpointer  ptr1,
                 gconstpointer  ptr2)
{
	return 0;
}


static void
gil_init (ImageList *gil)
{
	GTK_WIDGET_SET_FLAGS (gil, GTK_CAN_FOCUS);

	gil->priv = g_new0 (ImageListPrivate, 1);

	gil->priv->row_spacing = DEFAULT_ROW_SPACING;
	gil->priv->col_spacing = DEFAULT_COL_SPACING;
	gil->priv->text_spacing = DEFAULT_TEXT_SPACING;
	gil->priv->image_border = DEFAULT_IMAGE_BORDER;
	gil->priv->separators = g_strdup (" ");

	gil->priv->selection_mode = GTK_SELECTION_SINGLE;
	gil->priv->view_mode = IMAGE_LIST_VIEW_ALL /*COMMENTS_OR_TEXT*/ /*ALL*/;
	gil->priv->dirty = TRUE;
	gil->priv->update_width = FALSE;

	gil->priv->compare = default_compare;
	gil->priv->sort_type = GTK_SORT_ASCENDING;

	gil->priv->has_focus = FALSE;
	gil->priv->focus_image = -1;
	gil->priv->old_text = NULL;

	gil->priv->old_focus_image = -1;
	gil->priv->multi_selecting_with_keyboard = FALSE;

	gil->priv->target_list = gtk_target_list_new (target_table, G_N_ELEMENTS (target_table));

	gil->priv->sel_rect = NULL;
}


/**
 * gnome_image_list_get_type:
 *
 * Registers the &ImageList class if necessary, and returns the type ID
 * associated to it.
 *
 * Returns: The type ID of the &ImageList class.
 */
GType
image_list_get_type (void)
{
	static guint type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (ImageListClass),
			NULL,
			NULL,
			(GClassInitFunc) gil_class_init,
			NULL,
			NULL,
			sizeof (ImageList),
			0,
			(GInstanceInitFunc) gil_init
                };

		type = g_type_register_static (GNOME_TYPE_CANVAS,
					       "ImageList",
					       &type_info,
                                               0);
        }

        return type;
}


/**
 * gnome_image_list_set_image_width:
 * @gil: An image list.
 * @w:   New width for the image columns.
 *
 * Sets the amount of horizontal space allocated to the images, i.e. the column
 * width of the image list.
 */
void
image_list_set_image_width (ImageList *gil, int w)
{
	g_return_if_fail (IS_IMAGE_LIST (gil));

	gil->priv->min_image_width = w;
	gil->priv->max_image_width = w;

	if (gil->priv->frozen) {
		gil->priv->update_width = TRUE;
		gil->priv->dirty = TRUE;
		return;
	}

	reset_text_width (gil);
	gil_layout_all_images (gil);
	gil_scrollbar_adjust (gil);
}


static void
gil_adj_value_changed (GtkAdjustment *adj, 
		       ImageList *gil)
{	
	g_return_if_fail (IS_IMAGE_LIST (gil));
	gnome_canvas_scroll_to (GNOME_CANVAS (gil), 0, adj->value);
}


/**
 * gnome_image_list_set_hadjustment:
 * @gil: An image list.
 * @hadj: Adjustment to be used for horizontal scrolling.
 *
 * Sets the adjustment to be used for horizontal scrolling.  This is normally
 * not required, as the image list can be simply inserted in a &GtkScrolledWindow
 * and scrolling will be handled automatically.
 **/
void
image_list_set_hadjustment (ImageList *gil, 
			    GtkAdjustment *hadj)
{
	GtkAdjustment *old_adjustment;

	/* hadj isn't used but is here for compatibility with 
	 * GtkScrolledWindow */

	g_return_if_fail (IS_IMAGE_LIST (gil));

	if (hadj)
		g_return_if_fail (GTK_IS_ADJUSTMENT (hadj));

	if (gil->hadj == hadj)
		return;

	old_adjustment = gil->hadj;

	if (gil->hadj)
		g_object_unref (G_OBJECT (gil->hadj));
	gil->hadj = hadj;

	if (gil->hadj) {
		g_object_ref (G_OBJECT (gil->hadj));
		/* The horizontal adjustment is not used, so set some default
		 * values to indicate that everything is visible horizontally.
		 */
		gil->hadj->lower = 0.0;
		gil->hadj->upper = 1.0;
		gil->hadj->value = 0.0;
		gil->hadj->step_increment = 1.0;
		gil->hadj->page_increment = 1.0;
		gil->hadj->page_size = 1.0;
		gtk_adjustment_changed (gil->hadj);
	}

	if (! gil->hadj || ! old_adjustment)
		gtk_widget_queue_resize (GTK_WIDGET (gil));
}


/**
 * gnome_image_list_set_vadjustment:
 * @gil: An image list.
 * @hadj: Adjustment to be used for horizontal scrolling.
 *
 * Sets the adjustment to be used for vertical scrolling.  This is normally not
 * required, as the image list can be simply inserted in a &GtkScrolledWindow 
 * and scrolling will be handled automatically.
 **/
void
image_list_set_vadjustment (ImageList *gil, 
			    GtkAdjustment *vadj)
{
	GtkAdjustment *old_adjustment;

	g_return_if_fail (IS_IMAGE_LIST (gil));

	if (vadj)
		g_return_if_fail (GTK_IS_ADJUSTMENT (vadj));

	if (gil->adj == vadj)
		return;

	old_adjustment = gil->adj;

	if (gil->adj) {
		g_signal_handlers_disconnect_by_data (G_OBJECT(gil->adj), gil);
		g_object_unref (G_OBJECT (gil->adj));
	}

	gil->adj = vadj;

	if (gil->adj) {
		g_object_ref (G_OBJECT (gil->adj));
		gtk_object_sink (GTK_OBJECT (gil->adj));
		g_signal_connect (G_OBJECT (gil->adj), 
				  "value_changed",
				  G_CALLBACK (gil_adj_value_changed),
				  gil);
		g_signal_connect (G_OBJECT (gil->adj), "changed",
				  G_CALLBACK (gil_adj_value_changed), 
				  gil);
	}

	if (! gil->adj || ! old_adjustment)
		gtk_widget_queue_resize (GTK_WIDGET (gil));
}


/**
 * gnome_image_list_construct:
 * @gil: An image list.
 * @image_width: Width for the image columns.
 * @adj: Adjustment to be used for vertical scrolling.
 * @flags: A combination of %GNOME_IMAGE_LIST_IS_EDITABLE and %GNOME_IMAGE_LIST_STATIC_TEXT.
 *
 * Constructor for the image list, to be used by derived classes.
 **/
void
image_list_construct (ImageList *gil, 
		      guint image_width, 
		      GtkAdjustment *adj, 
		      gint flags)
{
	g_return_if_fail (IS_IMAGE_LIST (gil));

	image_list_set_image_width (gil, image_width);
	gil->priv->is_editable = (flags & IMAGE_LIST_IS_EDITABLE) != 0;
	gil->priv->static_text = (flags & IMAGE_LIST_STATIC_TEXT) != 0;

	if (!adj)
		adj = GTK_ADJUSTMENT (gtk_adjustment_new (0, 0, 1, 0.1, 0.1, 0.1));

	image_list_set_vadjustment (gil, adj);
}


/**
 * gnome_image_list_new_flags: [constructor]
 * @image_width: Width for the image columns.
 * @adj:        Adjustment to be used for vertical scrolling.
 * @flags:      A combination of %GNOME_IMAGE_LIST_IS_EDITABLE and 
 *              %GNOME_IMAGE_LIST_STATIC_TEXT.
 *
 * Creates a new image list widget.  The image columns are allocated a width of
 * @image_width pixels.  Image captions will be word-wrapped to this width as
 * well.
 *
 * The adjustment is used to pass an existing adjustment to be used to control
 * the image list's vertical scrolling.  Normally NULL can be passed here; if
 *  the
 * image list is inserted into a &GtkScrolledWindow, it will handle scrolling
 * automatically.
 *
 * If @flags has the %GNOME_IMAGE_LIST_IS_EDITABLE flag set, then the user 
 * will be
 * able to edit the text in the image captions, and the "text_changed" signal
 * will be emitted when an image's text is changed.
 *
 * If @flags has the %GNOME_IMAGE_LIST_STATIC_TEXT flags set, then the text
 * for the image captions will not be copied inside the image list; it will 
 * only
 * store the pointers to the original text strings specified by the 
 * application.
 * This is intended to save memory.  If this flag is not set, then the text
 * strings will be copied and managed internally.
 *
 * Returns: a newly-created image list widget
 */
GtkWidget *
image_list_new_flags (guint image_width, 
		      GtkAdjustment *adj, 
		      gint flags)
{
	ImageList *gil;

	gil = IMAGE_LIST (g_object_new (IMAGE_LIST_TYPE, NULL));
	image_list_construct (gil, image_width, adj, flags);

	return GTK_WIDGET (gil);
}


/**
 * image_list_new:
 * @image_width: Width for the image columns.
 * @adj: Adjustment to be used for vertical scrolling.
 * @flags: A combination of %GNOME_IMAGE_LIST_IS_EDITABLE and 
 *         %GNOME_IMAGE_LIST_STATIC_TEXT.
 *
 * This function is kept for binary compatibility with old applications.  It is
 * similar in purpose to gnome_image_list_new_flags(), but it will always turn
 * on the %GNOME_IMAGE_LIST_IS_EDITABLE flag.
 *
 * Return value: a newly-created image list widget
 **/
GtkWidget *
image_list_new (guint image_width, 
		GtkAdjustment *adj, 
		gint flags)
{
	return image_list_new_flags (image_width, adj, flags & IMAGE_LIST_IS_EDITABLE);
}


/**
 * gnome_image_list_freeze:
 * @gil:  An image list.
 *
 * Freezes an image list so that any changes made to it will not be
 * reflected on the screen until it is thawed with gnome_image_list_thaw().
 * It is recommended to freeze the image list before inserting or deleting
 * many images, for example, so that the layout process will only be executed
 * once, when the image list is finally thawed.
 *
 * You can call this function multiple times, but it must be balanced with the
 * same number of calls to gnome_image_list_thaw() before the changes will take
 * effect.
 */
void
image_list_freeze (ImageList *gil)
{
	g_return_if_fail (IS_IMAGE_LIST (gil));

	gil->priv->frozen++;

	/* We hide the root so that the user will not see any changes while the
	 * image list is doing stuff.
	 */

	if (gil->priv->frozen == 1)
		gnome_canvas_item_hide (GNOME_CANVAS (gil)->root);
}


/**
 * gnome_image_list_thaw:
 * @gil:  An image list.
 *
 * Thaws the image list and performs any pending layout operations.  This
 * is to be used in conjunction with gnome_image_list_freeze().
 */
void
image_list_thaw (ImageList *gil)
{
	g_return_if_fail (IS_IMAGE_LIST (gil));
	g_return_if_fail (gil->priv->frozen > 0);

	gil->priv->frozen--;

	if (gil->priv->frozen == 0) {
		if (gil->priv->dirty) {
			if (gil->priv->update_width) 
				reset_text_width (gil);
			
			gil_layout_all_images (gil);
			gil_scrollbar_adjust (gil);
			keep_focus_consistent (gil);
		}
	
		gnome_canvas_item_show (GNOME_CANVAS (gil)->root);
	}
}


/**
 * gnome_image_list_set_selection_mode
 * @gil:  An image list.
 * @mode: New selection mode.
 *
 * Sets the selection mode for an image list.  The %GTK_SELECTION_MULTIPLE and
 * %GTK_SELECTION_EXTENDED modes are considered equivalent.
 */
void
image_list_set_selection_mode (ImageList *gil, GtkSelectionMode mode)
{
	g_return_if_fail (IS_IMAGE_LIST (gil));

	gil->priv->selection_mode = mode;
	gil->priv->last_selected_idx = -1;
	gil->priv->last_selected_image = NULL;
}


/**
 * gnome_image_list_set_image_data_full:
 * @gil:     An image list.
 * @pos:     Index of an image.
 * @data:    User data to set on the image.
 * @destroy: Destroy notification handler for the image.
 *
 * Associates the @data pointer to the image at the index specified by @pos.
 * The
 * @destroy argument points to a function that will be called when the image is
 * destroyed, or NULL if no function is to be called when this happens.
 */
void
image_list_set_image_data_full (ImageList *gil,
				gint pos, 
				gpointer data,
				GtkDestroyNotify destroy)
{
	Image *image;

	g_return_if_fail (IS_IMAGE_LIST (gil));
	g_return_if_fail (pos >= 0 && pos < gil->images);

	image = g_list_nth (gil->priv->image_list, pos)->data;
	image->data = data;
	image->destroy = destroy;
}


/**
 * gnome_image_list_get_image_data:
 * @gil:  An image list.
 * @pos:  Index of an image.
 * @data: User data to set on the image.
 *
 * Associates the @data pointer to the image at the index specified by @pos.
 */
void
image_list_set_image_data (ImageList *gil, 
			   gint pos, 
			   gpointer data)
{
	image_list_set_image_data_full (gil, pos, data, NULL);
}


/**
 * gnome_image_list_get_image_data:
 * @gil: An image list.
 * @pos: Index of an image.
 *
 * Returns the user data pointer associated to the image at the index specified
 * by @pos.
 */
gpointer
image_list_get_image_data (ImageList *gil, 
			   gint pos)
{
	Image *image;

	g_return_val_if_fail (IS_IMAGE_LIST (gil), NULL);
	g_return_val_if_fail (pos >= 0 && pos < gil->images, NULL);

	/* FIXME
	if (gil->priv->frozen > 0)
		return NULL;
	*/

	image = g_list_nth (gil->priv->image_list, pos)->data;
	return image->data;
}


/**
 * gnome_image_list_find_image_from_data:
 * @gil:    An image list.
 * @data:   Data pointer associated to an image.
 *
 * Returns the index of the image whose user data has been set to @data,
 * or -1 if no image has this data associated to it.
 */
gint
image_list_find_image_from_data (ImageList *gil, 
				 gpointer data)
{
	GList *list;
	gint n;
	Image *image;

	g_return_val_if_fail (IS_IMAGE_LIST (gil), -1);

	for (n = 0, list = gil->priv->image_list; list; n++, list = list->next) {
		image = list->data;
		if (image->data == data)
			return n;
	}

	return -1;
}


/* Sets an integer value in the private structure of the image list, and updates it */
static void
set_value (ImageList *gil, 
	   gint *dest, 
	   gint val)
{
	if (val == *dest)
		return;

	*dest = val;

	if (!gil->priv->frozen) {
		gil_layout_all_images (gil);
		gil_scrollbar_adjust (gil);
	} else
		gil->priv->dirty = TRUE;
}


/**
 * gnome_image_list_set_row_spacing:
 * @gil:    An image list.
 * @pixels: Number of pixels for inter-row spacing.
 *
 * Sets the spacing to be used between rows of images.
 */
void
image_list_set_row_spacing (ImageList *gil, 
			    gint pixels)
{
	g_return_if_fail (IS_IMAGE_LIST (gil));
	set_value (gil, &gil->priv->row_spacing, pixels);
}


/**
 * gnome_image_list_set_col_spacing:
 * @gil:    An image list.
 * @pixels: Number of pixels for inter-column spacing.
 *
 * Sets the spacing to be used between columns of images.
 */
void
image_list_set_col_spacing (ImageList *gil, 
			    gint pixels)
{
	g_return_if_fail (IS_IMAGE_LIST (gil));
	set_value (gil, &gil->priv->col_spacing, pixels);
}


/**
 * gnome_image_list_set_text_spacing:
 * @gil:    An image list.
 * @pixels: Number of pixels between an image's image and its caption.
 *
 * Sets the spacing to be used between an image's image and its text caption.
 */
void
image_list_set_text_spacing (ImageList *gil, 
			     gint pixels)
{
	g_return_if_fail (IS_IMAGE_LIST (gil));
	set_value (gil, &gil->priv->text_spacing, pixels);
}


/**
 * gnome_image_list_set_image_border:
 * @gil:    An image list.
 * @pixels: Number of border pixels to be used around an image's image.
 *
 * Sets the width of the border to be displayed around an image's image.  This is
 * currently not implemented.
 */
void
image_list_set_image_border (ImageList *gil, 
			     gint pixels)
{
	g_return_if_fail (IS_IMAGE_LIST (gil));
	set_value (gil, &gil->priv->image_border, pixels);
}


/**
 * gnome_image_list_set_separators:
 * @gil: An image list.
 * @sep: String with characters to be used as word separators.
 *
 * Sets the characters that can be used as word separators when doing
 * word-wrapping in the image text captions.
 */
void
image_list_set_separators (ImageList *gil, 
			   const gchar *sep)
{
	g_return_if_fail (IS_IMAGE_LIST (gil));
	g_return_if_fail (sep != NULL);

	if (gil->priv->separators)
		g_free (gil->priv->separators);

	gil->priv->separators = g_strdup (sep);

	if (gil->priv->frozen) {
		gil->priv->dirty = TRUE;
		return;
	}

	gil_layout_all_images (gil);
	gil_scrollbar_adjust (gil);
}


/**
 * gnome_image_list_moveto:
 * @gil:    An image list.
 * @pos:    Index of an image.
 * @yalign: Vertical alignment of the image.
 *
 * Makes the image whose index is @pos be visible on the screen.  The image 
 * list
 * gets scrolled so that the image is visible.  An alignment of 0.0 represents
 * the top of the visible part of the image list, and 1.0 represents the 
 * bottom.
 * An image can be centered on the image list.
 */
void
image_list_moveto (ImageList *gil, 
		   gint pos, 
		   gdouble yalign)
{
	ImageLine *il;
	GList *l;
	gint i, y, uh, line;
	gfloat value;

	g_return_if_fail (IS_IMAGE_LIST (gil));
	g_return_if_fail (pos >= 0 && pos < gil->images);
	g_return_if_fail (yalign >= 0.0 && yalign <= 1.0);

	if (gil->priv->lines == NULL)
		return;

	line = pos / gil_get_images_per_line (gil);

	y = gil->priv->row_spacing;
	for (i = 0, l = gil->priv->lines; l && i < line; l = l->next, i++) {
		il = l->data;
		y += image_line_height (gil, il);
	}

	il = l->data;

	uh = GTK_WIDGET (gil)->allocation.height - image_line_height (gil,il);
	value = CLAMP ((y 
			- uh * yalign 
			- (1.0 - yalign) * gil->priv->row_spacing), 
		       0.0, 
		       gil->adj->upper - gil->adj->page_size);
	gtk_adjustment_set_value (gil->adj, value);
}


/**
 * gnome_image_list_is_visible:
 * @gil: An image list.
 * @pos: Index of an image.
 *
 * Returns whether the image at the index specified by @pos is visible.  This
 * will be %GTK_VISIBILITY_NONE if the image is not visible at all,
 * %GTK_VISIBILITY_PARTIAL if the image is at least partially shown, and
 * %GTK_VISIBILITY_FULL if the image is fully visible.
 */
GthumbVisibility
image_list_image_is_visible (ImageList *gil, 
			     gint pos)
{
	ImageLine *il;
	GList     *l;
	int        line, i;
	int        image_top, image_bottom;
	int        window_top, window_bottom;

	g_return_val_if_fail (IS_IMAGE_LIST (gil), GTHUMB_VISIBILITY_NONE);
	g_return_val_if_fail (pos >= 0 && pos < gil->images, GTHUMB_VISIBILITY_NONE);

	if (gil->priv->lines == NULL)
		return GTHUMB_VISIBILITY_NONE;

	line = pos / gil_get_images_per_line (gil);
	image_top = gil->priv->row_spacing;
	for (i = 0, l = gil->priv->lines; l && i < line; l = l->next, i++) {
		il = l->data;
		image_top += image_line_height (gil, il);
	}
	image_bottom = image_top + image_line_height (gil, (ImageLine *) l->data);

	window_top = gil->adj->value;
	window_bottom = gil->adj->value + GTK_WIDGET (gil)->allocation.height;

	if (image_bottom < window_top)
		return GTHUMB_VISIBILITY_NONE;

	if (image_top > window_bottom)
		return GTHUMB_VISIBILITY_NONE;

	if ((image_top >= window_top) && (image_bottom <= window_bottom))
		return GTHUMB_VISIBILITY_FULL;

	if ((image_top < window_top) && (image_bottom >= window_top))
		return GTHUMB_VISIBILITY_PARTIAL_TOP;

	if ((image_top <= window_bottom) && (image_bottom > window_bottom))
		return GTHUMB_VISIBILITY_PARTIAL_BOTTOM;

	return GTHUMB_VISIBILITY_PARTIAL;
}


/**
 * gnome_image_list_get_image_at:
 * @gil: An image list.
 * @x:   X position in the image list window.
 * @y:   Y position in the image list window.
 *
 * Returns the index of the image that is under the specified coordinates, 
 * which
 * are relative to the image list's window.  If there is no image in that
 * position, -1 is returned.
 */
gint
image_list_get_image_at (ImageList *gil, 
			 gint x, 
			 gint y)
{
	GList *l;
	gdouble wx, wy;
	gdouble dx, dy;
	gint cx, cy;
	gint n;
	GnomeCanvasItem *item;
	gdouble dist;

	g_return_val_if_fail (IS_IMAGE_LIST (gil), -1);

	dx = x;
	dy = y;

	gnome_canvas_window_to_world (GNOME_CANVAS (gil), dx, dy, &wx, &wy);
	gnome_canvas_w2c (GNOME_CANVAS (gil), wx, wy, &cx, &cy);

	for (n = 0, l = gil->priv->image_list; l; l = l->next, n++) {
		Image *image = l->data;
		GnomeCanvasItem *cimage = GNOME_CANVAS_ITEM (image->image);
		GnomeCanvasItem *text = GNOME_CANVAS_ITEM (image->text);
		GnomeCanvasItem *comment = GNOME_CANVAS_ITEM (image->comment);
		gboolean view_text, view_comment;

		image_get_view_mode (gil, image, &view_text, &view_comment);

		/* on image. */
		if ((wx >= cimage->x1) && (wx <= cimage->x2)
		    && (wy >= cimage->y1) && (wy <= cimage->y2)) {
			dist = GNOME_CANVAS_ITEM_GET_CLASS (cimage)->point (cimage, wx, wy, cx, cy, &item);

			if ((gint) (dist * GNOME_CANVAS (gil)->pixels_per_unit + 0.5) <= GNOME_CANVAS (gil)->close_enough)
				return n;
		}

		/* on text. */
		if (view_text &&
		    ((wx >= text->x1) && (wx <= text->x2) 
		     && (wy >= text->y1) && (wy <= text->y2))) {
			dist = GNOME_CANVAS_ITEM_GET_CLASS (text)->point (text, wx, wy, cx, cy, &item);

			if ((gint) (dist * GNOME_CANVAS (gil)->pixels_per_unit + 0.5) <= GNOME_CANVAS (gil)->close_enough)
				return n;
		}

		/* on comment. */
		if (view_comment &&
		    ((wx >= comment->x1) && (wx <= comment->x2) 
		     && (wy >= comment->y1) && (wy <= comment->y2))) {
			dist = GNOME_CANVAS_ITEM_GET_CLASS (comment)->point (comment, wx, wy, cx, cy, &item);

			if ((gint) (dist * GNOME_CANVAS (gil)->pixels_per_unit + 0.5) <= GNOME_CANVAS (gil)->close_enough)
				return n;
		}
	}

	return -1;
}


void
image_list_set_compare_func (ImageList *gil,
			     GCompareFunc cmp_func)
{
	g_return_if_fail (IS_IMAGE_LIST (gil));
	gil->priv->compare = (cmp_func) ? cmp_func : default_compare;
}


void
image_list_set_sort_type (ImageList *gil,
			  GtkSortType sort_type)
{
	g_return_if_fail (IS_IMAGE_LIST (gil));
	gil->priv->sort_type = sort_type;
}


void
image_list_sort (ImageList *gil)
{
	Image *focused_image = NULL;
	GList *scan;
	gint num;

	g_return_if_fail (IS_IMAGE_LIST (gil));

	if (gil->priv->focus_image > -1) 
		focused_image = g_list_nth (gil->priv->image_list, 
					    gil->priv->focus_image)->data;

	gil->priv->image_list = g_list_sort (gil->priv->image_list, 
					     gil->priv->compare);
	if (gil->priv->sort_type == GTK_SORT_DESCENDING)
		gil->priv->image_list = g_list_reverse (gil->priv->image_list);

	/* update selection list. */

	if (gil->selection != NULL) {
		g_list_free (gil->selection);
		gil->selection = NULL;
	}

	scan = gil->priv->image_list;
	num = 0;
	while (scan) {
		Image *image = scan->data;

		if (image->selected)
			gil->selection = g_list_prepend (gil->selection, GINT_TO_POINTER (num));

		if (focused_image == image) 
			gil->priv->focus_image = num;

		num++;
		scan = scan->next;
	}

	if (!gil->priv->frozen) {
		gil_layout_all_images (gil);
		gil_scrollbar_adjust (gil);
	} else
		gil->priv->dirty = TRUE;
}


void
image_list_set_image_pixbuf (ImageList *gil,
			     gint pos,
			     GdkPixbuf *pixbuf)
{
	Image *image;

	g_return_if_fail (IS_IMAGE_LIST (gil));
	g_return_if_fail (pos >= 0 && pos < gil->images);
	g_return_if_fail (pixbuf != NULL);
	
	image = g_list_nth (gil->priv->image_list, pos)->data;

	g_object_set (G_OBJECT (image->image),
		      "pixbuf", pixbuf,
		      NULL);
}


void
image_list_set_image_text (ImageList *gil,
			   gint pos,
			   const gchar *text)
{
	Image *image;

	g_return_if_fail (IS_IMAGE_LIST (gil));
	g_return_if_fail (pos >= 0 && pos < gil->images);

	if (!GTK_WIDGET_REALIZED (gil))
		return;
	
	image = g_list_nth (gil->priv->image_list, pos)->data;
	gthumb_text_item_configure (image->text,
				    0, 
				    0, 
				    gil->priv->max_image_width, 
				    DEFAULT_FONT,
				    text, 
				    FALSE,
				    gil->priv->static_text);

	if (gil->priv->frozen) {
		gil->priv->dirty = TRUE;
		return;
	}

	gil_layout_from_line (gil, pos / gil_get_images_per_line (gil));
	gil_scrollbar_adjust (gil);
}


void
image_list_set_image_comment (ImageList *gil,
			      gint pos,
			      const gchar *comment)
{
	Image *image;
	char  *c;

	g_return_if_fail (IS_IMAGE_LIST (gil));
	g_return_if_fail (pos >= 0 && pos < gil->images);

	if (!GTK_WIDGET_REALIZED (gil))
		return;
	
	image = g_list_nth (gil->priv->image_list, pos)->data;

	c = truncate_comment_if_needed (gil, comment);
	gthumb_text_item_configure (image->comment,
				    0, 
				    0, 
				    gil->priv->max_image_width, 
				    DEFAULT_FONT,
				    c, 
				    FALSE,
				    gil->priv->static_text);
	g_free (c);

	if (gil->priv->frozen) {
		gil->priv->dirty = TRUE;
		return;
	}

	gil_layout_from_line (gil, pos / gil_get_images_per_line (gil));
	gil_scrollbar_adjust (gil);
}


GdkPixbuf*
image_list_get_image_pixbuf (ImageList *gil,
			     gint pos)
{
	Image *image;

	g_return_val_if_fail (IS_IMAGE_LIST (gil), NULL);
	g_return_val_if_fail (pos >= 0 && pos < gil->images, NULL);
	
	image = g_list_nth (gil->priv->image_list, pos)->data;

	return image->image->pixbuf;
}


const gchar*
image_list_get_image_text (ImageList *gil,
			   gint pos)
{
	Image *image;

	g_return_val_if_fail (IS_IMAGE_LIST (gil), NULL);
	g_return_val_if_fail (pos >= 0 && pos < gil->images, NULL);
	
	image = g_list_nth (gil->priv->image_list, pos)->data;

	return gthumb_text_item_get_text (image->text);
}


const gchar*
image_list_get_image_comment (ImageList *gil,
			      gint pos)
{
	Image *image;

	g_return_val_if_fail (IS_IMAGE_LIST (gil), NULL);
	g_return_val_if_fail (pos >= 0 && pos < gil->images, NULL);
	
	image = g_list_nth (gil->priv->image_list, pos)->data;

	return gthumb_text_item_get_text (image->comment);
}


gchar*
image_list_get_old_text (ImageList *gil)
{
	g_return_val_if_fail (IS_IMAGE_LIST (gil), NULL);
	return gil->priv->old_text;
}


GList*
image_list_get_list (ImageList *gil)
{
	g_return_val_if_fail (IS_IMAGE_LIST (gil), NULL);
	return gil->priv->image_list;
}


static int
get_first_visible_at_offset (ImageList *gil, gdouble ofs)
{
	gint   pos, line;
	GList *l;

	if (gil->images == 0)
		return -1;

	line = 0;
	for (l = gil->priv->lines; l && (ofs > 0.0); l = l->next) {
		ofs -= image_line_height (gil, l->data);
		line++;
	}
	
	pos = image_list_get_images_per_line (gil) * (line - 1);

	if (pos < 0)
		pos = 0;
	if (pos >= gil->images)
		pos = gil->images - 1;

	return pos;
}


int
image_list_get_first_visible (ImageList *gil)
{
	g_return_val_if_fail (IS_IMAGE_LIST (gil), -1);
	return get_first_visible_at_offset (gil, gil->adj->value);
}


int
get_last_visible_at_offset (ImageList *gil, double ofs)
{
	int    pos, line;
	GList *l;

	if (gil->images == 0)
		return -1;

	line = 0;
	for (l = gil->priv->lines; l && (ofs > 0.0); l = l->next) {
		ofs -= image_line_height (gil, l->data);
		line++;
	}
	
	pos = image_list_get_images_per_line (gil) * line - 1;

	if (pos < 0)
		pos = 0;
	if (pos >= gil->images)
		pos = gil->images - 1;

	return pos;
}


int
image_list_get_last_visible (ImageList *gil)
{
	g_return_val_if_fail (IS_IMAGE_LIST (gil), -1);
	return get_last_visible_at_offset (gil, (gil->adj->value 
						 + gil->adj->page_size));
}


gboolean
image_list_has_focus (ImageList *gil)
{
	g_return_val_if_fail (IS_IMAGE_LIST (gil), FALSE);
	return ((ImageListPrivate*) gil->priv)->has_focus;
}


void
image_list_start_editing (ImageList *gil,
			  int        pos)
{
	Image *image;

	g_return_if_fail (IS_IMAGE_LIST (gil));
	g_return_if_fail (pos >= 0 && pos < gil->images);

	image = g_list_nth (gil->priv->image_list, pos)->data;

	gthumb_text_item_start_editing (image->text);
}


gboolean
image_list_editing (ImageList *gil)
{
	Image *image;
	GList *scan;

	g_return_val_if_fail (IS_IMAGE_LIST (gil), FALSE);

	for (scan = gil->priv->image_list; scan; scan = scan->next) {
		image = (Image*) scan->data;
		if (image->text->editing)
			return TRUE;
	}

	return FALSE;
}


void
image_list_set_view_mode (ImageList *gil,
			  guint8 mode)
{
	g_return_if_fail (IS_IMAGE_LIST (gil));

	if (gil->priv->view_mode == mode)
		return;
	gil->priv->view_mode = mode;
}


guint8
image_list_get_view_mode (ImageList *gil)
{
	g_return_val_if_fail (IS_IMAGE_LIST (gil), 0);	
	return gil->priv->view_mode;
}
