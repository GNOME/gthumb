/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001, 2003 Free Software Foundation, Inc.
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

#include <string.h>
#include <math.h>
#include <glib.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtkbindings.h>
#include <gtk/gtkdnd.h>
#include <gtk/gtkstock.h>
#include <gdk/gdkx.h>

#ifdef HAVE_RENDER
#include <X11/extensions/Xrender.h>
#endif

#include "file-data.h"
#include "glib-utils.h"
#include "gth-image-list.h"
#include "gthumb-marshal.h"
#include "gthumb-enum-types.h"
#include "gthumb-slide.h"


#define DEFAULT_FONT "Sans normal 12"

#define DEFAULT_ROW_SPACING 16    /* Default spacings */
#define DEFAULT_COLUMN_SPACING 16
#define DEFAULT_TEXT_SPACING 6
#define DEFAULT_IMAGE_BORDER 3
#define TEXT_COMMENT_SPACE 6       /* space between text and comment. */
#define KEYBOARD_SELECTION_BORDER 10
#define FRAME_SELECTION_BORDER 3

#define SCROLL_TIMEOUT 30          /* Autoscroll timeout in milliseconds */
#define LAYOUT_TIMEOUT 20

#define COMMENT_MAX_LINES 5        /* max number of lines to display. */
#define ETC " [..]"                /* string to append when the comment 
				    * doesn't fit. */

#define COLOR_WHITE 0x00ffffff
#define CHECK_SIZE 50

#define MAX_DIFF 1024.0

#define STRING_IS_VOID(s) ((s == NULL) || (*s == 0))
#define IMAGE_LINE_HEIGHT(gil, il) \
 (gil->priv->max_item_width \
  + ((((il)->comment_height > 0) || ((il)->text_height > 0)) ? (gil)->priv->text_spacing : 0) \
  + (il)->comment_height \
  + ((((il)->comment_height > 0) && ((il)->text_height > 0)) ? TEXT_COMMENT_SPACE: 0) \
  + (il)->text_height \
  + (gil)->priv->row_spacing) 


/* Signals */
enum {
	SELECTION_CHANGED,
	ITEM_ACTIVATED,
	CURSOR_CHANGED,
	MOVE_CURSOR,
	SELECT_ALL,
	UNSELECT_ALL,
	SET_CURSOR_SELECTION,
	TOGGLE_CURSOR_SELECTION,
	START_INTERACTIVE_SEARCH,
	LAST_SIGNAL
};

/* Properties */
enum {
	PROP_0,
	PROP_HADJUSTMENT,
	PROP_VADJUSTMENT,
	PROP_ENABLE_SEARCH
};

typedef enum {
	SYNC_INSERT,
	SYNC_REMOVE
} SyncType;


static GtkTargetEntry target_table[] = {
	{ "text/uri-list", 0, 0 } 
};

static guint image_list_signals[LAST_SIGNAL] = { 0 };
static GtkContainerClass *parent_class;


static void
gth_image_list_item_free_pixmap_and_mask (GthImageListItem *item)
{
	if (item->pixmap != NULL) {
		g_object_unref (item->pixmap);
		item->pixmap = NULL;
	}

	if (item->mask != NULL) {
		g_object_unref (item->mask);
		item->mask = NULL;
	}
}


static int
to_255 (int v)
{
	return v * 255 / 65535;
}


static void
gth_image_list_item_set_pixbuf (GthImageList     *image_list,
				GthImageListItem *item,
				GdkPixbuf        *pixbuf)
{
	static GdkPixbuf *last_pixbuf = NULL;
	static GdkPixmap *last_pixmap = NULL;
	static GdkBitmap *last_mask   = NULL;
	GdkPixbuf        *tmp;

	if (pixbuf != NULL) {
		item->image_area.width = gdk_pixbuf_get_width (pixbuf);
		item->image_area.height = gdk_pixbuf_get_height (pixbuf);
	}

	gth_image_list_item_free_pixmap_and_mask (item);

	if ((pixbuf != NULL) && (last_pixbuf == pixbuf)) {
		if (last_pixmap != NULL)
			item->pixmap = g_object_ref (last_pixmap);
		else
			item->pixmap = NULL;

		if (last_mask != NULL)
			item->mask = g_object_ref (last_mask); 
		else
			item->mask = NULL;

		return;
	} 

	if (last_pixbuf != NULL) {
		g_object_unref (last_pixbuf);
		last_pixbuf = NULL;
	}

	if (last_pixmap != NULL) {
		g_object_unref (last_pixmap);
		last_pixmap = NULL;
	}

	if (last_mask != NULL) {
		g_object_unref (last_mask);
		last_mask = NULL;
	}

	if (pixbuf == NULL) {
		item->pixmap = NULL;
		item->mask = NULL;
		return;
	}

	last_pixbuf = g_object_ref (pixbuf);

	if (gdk_pixbuf_get_has_alpha (pixbuf)) {
		GdkColor color;
		guint32  uint_color;

		color = GTK_WIDGET (image_list)->style->base[GTK_STATE_NORMAL];
		uint_color = (0xFF000000
			      | (to_255 (color.red) << 16) 
			      | (to_255 (color.green) << 8) 
			      | (to_255 (color.blue) << 0));

		tmp = gdk_pixbuf_composite_color_simple (
					 pixbuf, 
					 item->image_area.width,
					 item->image_area.height, 
					 GDK_INTERP_NEAREST,
					 255,
					 CHECK_SIZE, 
					 uint_color, 
					 uint_color);
	} else 
		tmp = g_object_ref (pixbuf);

	gdk_pixbuf_render_pixmap_and_mask (tmp,
					   &last_pixmap, 
					   &last_mask, 
					   112);
	
	if (last_pixmap != NULL)
		item->pixmap = g_object_ref (last_pixmap);
	else
		item->pixmap = NULL;
	
	if (last_mask != NULL)
		item->mask = g_object_ref (last_mask); 
	else
		item->mask = NULL;

	g_object_unref (tmp);
}


static GthImageListItem*
gth_image_list_item_new (GthImageList  *image_list,
			 GdkPixbuf     *image,
			 const char    *label,
			 const char    *comment)
{
	GthImageListItem *item;

	item = g_new0 (GthImageListItem, 1);

	item->ref = 1;
	item->slide_area.x = -1;
	item->image_area.x = -1;
	item->image_area.width = -1;
	item->label_area.width = -1;
	item->comment_area.width = -1;

	if (image != NULL) 
		gth_image_list_item_set_pixbuf (image_list, item, image);

	if (label != NULL)
		item->label = g_strdup (label);

	if (comment != NULL)
		item->comment = g_strdup (comment);

	return item;
}


/* FIXME
static GthImageListItem*
gth_image_list_item_ref (GthImageListItem *item)
{
	if (item != NULL)
		item->ref++;
	return item;
}
*/


static void
gth_image_list_item_unref (GthImageListItem *item)
{
	if (item == NULL)
		return;

	if (--item->ref == 0) {
		/* fake rendering to free cached data. */
		gth_image_list_item_set_pixbuf (NULL, item, NULL);

		gth_image_list_item_free_pixmap_and_mask (item);
		g_free (item->label);
		g_free (item->comment);
		if ((item->destroy != NULL) && (item->data != NULL))
			(item->destroy) (item->data);
		g_free (item);
	}
}



typedef struct {
	int    y;
	int    image_height;
	int    text_height;
	int    comment_height;
	GList *image_list;
} GthImageListLine;


struct _GthImageListPrivate {
	GList            *image_list;
	GList            *selection;
	GList            *lines;
	int               images;
	int               focused_item;
	int               old_focused_item;    /* Used to do multiple 
						* selection with the 
						* keyboard. */

	guint             dirty : 1;           /* Whether the images need to 
						* be laid out */
	guint             update_width : 1;
	guint             frozen;
	guint             dragging : 1;        /* Whether the user is 
						* dragging items. */
	guint             drag_started : 1;    /* Whether the drag has 
						* started. */
	guint             selecting : 1;       /* Whether the user is 
						* performing a 
						* rubberband selection. */
	guint             select_pending : 1;  /* Whether selection is pending
						* after a button press. */

	guint             enable_search : 1;

	guint             enable_thumbs : 1;

	int               select_pending_pos;
	GthImageListItem *select_pending_item;

	guint             sorted : 1;
	GtkSortType       sort_type;           /* Ascending or descending 
						* order. */
	GCompareFunc      compare;             /* Compare function. */ 

	GdkRectangle      selection_area;
	GtkSelectionMode  selection_mode;
	int               last_selected_pos;
	GthImageListItem *last_selected_item;
	guint             multi_selecting_with_keyboard : 1; /* Whether a multi selection
							      * with the keyboard has 
							      * started. */
	guint             selection_changed : 1;

	GtkTargetList    *target_list;

	int               width;
	int               height;
	int               max_item_width;
	int               row_spacing;
	int               col_spacing;
	int               text_spacing;
	int               image_border;

	GthViewMode       view_mode;

	guint             timer_tag; 	       /* Timeout ID for 
						* autoscrolling */
	double            value_diff;          /* Change the adjustment value 
						* by this 
						* amount when autoscrolling */

	double            event_last_x;        /* Mouse position for 
						* autoscrolling */
	double            event_last_y;

	int               sel_start_x;         /* The point where the mouse 
						* selection started. */
	int               sel_start_y;

	guint             sel_state;           /* Modifier state when the 
						* selection began. */

	int               drag_start_x;        /* The point where the drag 
						* started. */
	int               drag_start_y;

	GtkAdjustment    *hadjustment;
	GtkAdjustment    *vadjustment;
	GdkWindow        *bin_window;

	PangoLayout      *layout;
	PangoLayout      *comment_layout;
	guint             layout_timeout;

#ifdef HAVE_RENDER
	gboolean           use_render;
	XRenderPictFormat *format;
#endif
};


static void
queue_draw (GthImageList *image_list)
{
	if (image_list->priv->frozen > 0)
		return;

	if (image_list->priv->bin_window == NULL)
		return;

	gdk_window_invalidate_rect (image_list->priv->bin_window,
				    NULL,
				    FALSE);				    
}


static void
gth_image_list_adjustment_changed (GtkObject    *adj, 
				   GthImageList *image_list)
{
	gdk_window_move_resize (image_list->priv->bin_window,
				- (int) image_list->priv->hadjustment->value,
				- (int) image_list->priv->vadjustment->value,
				image_list->priv->width,
				image_list->priv->height);

	queue_draw (image_list);
}


static void
gth_image_list_adjustment_value_changed (GtkObject    *adj, 
					 GthImageList *image_list)
{
	gdk_window_move (image_list->priv->bin_window,
			 - (int) image_list->priv->hadjustment->value,
			 - (int) image_list->priv->vadjustment->value);
	gdk_window_process_updates (image_list->priv->bin_window, TRUE);
}


static void
gth_image_list_line_free (GthImageListLine *line)
{
	g_list_free (line->image_list);
	g_free (line);
}


static void
free_line_info (GthImageList *image_list)
{
	GthImageListPrivate *priv = image_list->priv;
	GList               *scan;

	for (scan = priv->lines; scan; scan = scan->next) {
		GthImageListLine *line = scan->data;
		gth_image_list_line_free (line);
	}

	g_list_free (priv->lines);
	priv->lines = NULL;
	priv->height = 0;
}


static void
gth_image_list_finalize (GObject *object)
{
	GthImageList         *image_list;
	GthImageListPrivate  *priv;

	g_return_if_fail (GTH_IS_IMAGE_LIST (object));

	image_list = (GthImageList*) object;
	priv = image_list->priv;

	if (priv->image_list != NULL) {
		GList *scan;
		for (scan = priv->image_list; scan; scan = scan->next)
			gth_image_list_item_unref (scan->data);
		g_list_free (priv->image_list);
		priv->image_list = NULL;
	}

	free_line_info (image_list);

	if (priv->selection != NULL) {
		g_list_free (priv->selection);
		priv->selection = NULL;
	}

	if (priv->hadjustment != NULL) {
		g_signal_handlers_disconnect_by_func (priv->hadjustment,
						      gth_image_list_adjustment_changed,
						      image_list);
		g_object_unref (priv->hadjustment);
		priv->hadjustment = NULL;
	}
	
	if (priv->vadjustment != NULL) {
		g_signal_handlers_disconnect_by_func (priv->vadjustment,
						      gth_image_list_adjustment_changed,
						      image_list);
		g_object_unref (priv->vadjustment);
		priv->vadjustment = NULL;
	}

	if (priv->target_list != NULL) {
		gtk_target_list_unref (priv->target_list);
		priv->target_list = NULL;
	}

	if (priv->layout != NULL) {
		g_object_unref (priv->layout);
		priv->layout = NULL;
	}

	if (priv->comment_layout != NULL) {
		g_object_unref (priv->comment_layout);
		priv->comment_layout = NULL;
	}

	g_free (image_list->priv);
	image_list->priv = NULL;

        /* Chain up */
	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gth_image_list_map (GtkWidget *widget)
{
	GthImageList *image_list;

	g_return_if_fail (GTH_IS_IMAGE_LIST (widget));

	image_list = (GthImageList*) widget;
	
	GTK_WIDGET_SET_FLAGS (widget, GTK_MAPPED);

	gdk_window_show (image_list->priv->bin_window);
	gdk_window_show (widget->window);
}



/* Layout stuff */


static void
get_text_size (GthImageList *image_list, 
	       const char   *text,
	       int          *width,
	       int          *height,
	       gboolean      comment)
{
	PangoLayout         *layout;
	GthImageListPrivate *priv = image_list->priv;
	PangoRectangle       bounds;

	if (comment) 
		layout = priv->comment_layout;
	else
		layout = priv->layout;

	pango_layout_set_text (layout, text, strlen (text));
	pango_layout_get_pixel_extents (layout, NULL, &bounds);

	if (width != NULL)
		*width = bounds.width;

	if (height != NULL)
		*height = bounds.height;
}


static void
get_label_size (GthImageList     *image_list, 
		GthImageListItem *item,
		int              *width, 
		int              *height)
{
	if ((item->label != NULL) && (*item->label != 0)) {
		if ((item->label_area.width == -1) || (item->label_area.height == -1))
			get_text_size (image_list, 
				       item->label,
				       & (item->label_area.width),
				       & (item->label_area.height),
				       FALSE);	

		if (width != NULL) 
			*width = item->label_area.width;

		if (height != NULL) 
			*height = item->label_area.height;

	} else {
		if (width != NULL)
			*width = 0;
		if (height != NULL)
			*height = 0;
	}
}


static void
get_comment_size (GthImageList     *image_list, 
		  GthImageListItem *item,
		  int              *width, 
		  int              *height)
{
	if ((item->comment != NULL) && (*item->comment != 0)) {
		if ((item->comment_area.width == -1) || (item->comment_area.height == -1))
			get_text_size (image_list, 
				       item->comment,
				       & (item->comment_area.width),
				       & (item->comment_area.height),
				       TRUE);	

		if (width != NULL) 
			*width = item->comment_area.width;

		if (height != NULL) 
			*height = item->comment_area.height;

	} else {
		if (width != NULL)
			*width = 0;
		if (height != NULL)
			*height = 0;
	}
}


static void
item_get_view_mode (GthImageList     *image_list, 
		    GthImageListItem *item,
		    gboolean         *view_label,
		    gboolean         *view_comment)
{
	GthImageListPrivate *priv = image_list->priv;

	*view_label   = TRUE;
	*view_comment = TRUE;

	if (priv->view_mode == GTH_VIEW_MODE_VOID) {
		*view_label   = FALSE;
		*view_comment = FALSE;
		return;
	}
	if (priv->view_mode == GTH_VIEW_MODE_LABEL)
		*view_comment = FALSE;
	if (priv->view_mode == GTH_VIEW_MODE_COMMENTS)
		*view_label = FALSE;
	if ((priv->view_mode == GTH_VIEW_MODE_COMMENTS_OR_TEXT)
	    && ! STRING_IS_VOID (item->comment))
		*view_label = FALSE;

	if (STRING_IS_VOID (item->comment))
		*view_comment = FALSE;
	if (STRING_IS_VOID (item->label))
		*view_label = FALSE;
}


static void
get_item_height (GthImageList     *image_list,
		 GthImageListItem *item,
		 int              *image_height, 
		 int              *text_height,
		 int              *comment_height)
{
	*image_height = image_list->priv->max_item_width;
	get_label_size (image_list, item, NULL, text_height);
	get_comment_size (image_list, item, NULL, comment_height);
}


int
gth_image_list_get_items_per_line (GthImageList *image_list)
{
	GthImageListPrivate *priv = image_list->priv;
	int                  images_per_line;

	g_return_val_if_fail (GTH_IS_IMAGE_LIST (image_list), 1);

	images_per_line = GTK_WIDGET (image_list)->allocation.width / (priv->max_item_width + priv->col_spacing);

	return MAX (1, images_per_line);
}


static void
place_item (GthImageList     *image_list, 
	    GthImageListItem *item, 
	    int               x, 
	    int               y, 
	    int               image_height,
	    gboolean          view_label,
	    gboolean          view_comment)
{
	GthImageListPrivate *priv = image_list->priv;
	int                  x_offset, y_offset;

	if (image_height > item->image_area.height)
		y_offset = (image_height - item->image_area.height) / 2;
	else
		y_offset = 0;
	x_offset = (priv->max_item_width - item->image_area.width) / 2;

	item->slide_area.x = x;
	item->slide_area.y = y;

	item->image_area.x = x + x_offset + 1;
	item->image_area.y = y + y_offset + 1;

	y = y + image_height + priv->text_spacing;

	x_offset = (priv->max_item_width - item->comment_area.width) / 2;
	if (view_comment) {
		int comment_height;

		item->comment_area.x = x + x_offset + 1;
		item->comment_area.y = y;
		get_comment_size (image_list, item, NULL, &comment_height);
		y += comment_height + TEXT_COMMENT_SPACE;
	} 

	x_offset = (priv->max_item_width - item->label_area.width) / 2;
	if (view_label) {
		item->label_area.x = x + x_offset + 1;
		item->label_area.y = y;
	} 
}


static void
layout_line (GthImageList     *image_list, 
	     GthImageListLine *line)
{
	GthImageListPrivate *priv = image_list->priv;
	GList               *scan;
	gboolean             view_label, view_comment;
	int                  x = 0;

	for (scan = line->image_list; scan; scan = scan->next) {
		GthImageListItem *item = scan->data;

		item_get_view_mode (image_list, item, &view_label, &view_comment);
		
		x += priv->col_spacing;
		place_item (image_list, 
			    item, 
			    x, 
			    line->y, 
			    line->image_height,
			    view_label, 
			    view_comment);
		x += priv->max_item_width;
	}
}


static void
add_and_layout_line (GthImageList *image_list, 
		     GList        *line_images, 
		     int           y,
		     int           image_height, 
		     int           text_height,
		     int           comment_height)
{
	GthImageListPrivate *priv = image_list->priv;
	GthImageListLine    *line;

	line = g_new (GthImageListLine, 1);
	line->image_list = line_images;
	line->y = y;
	line->image_height = image_height;
	line->text_height = text_height;
	line->comment_height = comment_height;

	layout_line (image_list, line);
	priv->lines = g_list_append (priv->lines, line);
}


static void
update_scrollbar_adjust (GthImageList *image_list)
{
	GList  *scan;
	int     height;
	int     page_size;

	if (! GTK_WIDGET_REALIZED (image_list))
		return;

	height = image_list->priv->row_spacing;
	for (scan = image_list->priv->lines; scan; scan = scan->next) {
		GthImageListLine *line = scan->data;
		height += IMAGE_LINE_HEIGHT (image_list, line);
	}

	image_list->priv->height = MAX (height, GTK_WIDGET (image_list)->allocation.height);

	if (image_list->priv->vadjustment != NULL) {
		page_size = GTK_WIDGET (image_list)->allocation.height;
		image_list->priv->vadjustment->page_size = page_size;
		image_list->priv->vadjustment->page_increment = page_size * 0.9;
		image_list->priv->vadjustment->step_increment = page_size * 0.1;
		image_list->priv->vadjustment->upper = height;
		
		if (image_list->priv->vadjustment->value + page_size > height)
			image_list->priv->vadjustment->value = MAX (0.0, height - page_size);

		gtk_adjustment_changed (image_list->priv->vadjustment);
	}
}


static void
relayout_images_at (GthImageList *image_list, 
		    int           pos, 
		    int           y)
{
	GthImageListPrivate *priv = image_list->priv;
	int    text_height = 0, image_height = 0, comment_height = 0;
	int    images_per_line, n;
	GList *line_images = NULL, *scan;
	int    max_height = 0;

	images_per_line = gth_image_list_get_items_per_line (image_list);
	scan = g_list_nth (priv->image_list, pos);
	n = pos;

	for (; scan; scan = scan->next, n++) {
		GthImageListItem *item = scan->data;
		int               ih, th, ch;
		gboolean          view_label, view_comment;

		if (! (n % images_per_line)) {
			if (line_images != NULL) {
				add_and_layout_line (image_list, 
						     line_images, 
						     y,
						     image_height, 
						     text_height,
						     comment_height);
				line_images = NULL;
				y += max_height + priv->row_spacing;
			}

			max_height     = 0;
			image_height   = 0;
			text_height    = 0;
			comment_height = 0;
		}

		get_item_height (image_list, item, &ih, &th, &ch);
		item_get_view_mode (image_list, item, &view_label, &view_comment);
		
		if (! view_label) 
			th = 0;

		if (! view_comment) 
			ch = 0;
		
		image_height   = MAX (ih, image_height);
		text_height    = MAX (th, text_height);
		comment_height = MAX (ch, comment_height);

		max_height = (image_height 
			      + ((comment_height || text_height) ? priv->text_spacing : 0)
			      + comment_height
			      + ((comment_height && text_height) ? TEXT_COMMENT_SPACE : 0)
			      + text_height);

		line_images = g_list_append (line_images, item);
	}

	if (line_images != NULL)
		add_and_layout_line (image_list, 
				     line_images, 
				     y, 
				     image_height, 
				     text_height,
				     comment_height);

	update_scrollbar_adjust (image_list);
}


static void
free_line_info_from (GthImageList *image_list, 
		     int           first_line)
{
	GthImageListPrivate *priv = image_list->priv;
	GList               *lines, *scan;
	
	lines = g_list_nth (priv->lines, first_line);

	if (lines == NULL)
		return;

	for (scan = lines; scan; scan = scan->next) {
		GthImageListLine *line = scan->data;
		gth_image_list_line_free (line);
	}
	g_list_free (lines);

	if (priv->lines != NULL) {
		if (lines->prev != NULL)
			lines->prev->next = NULL;
		else
			priv->lines = NULL;
	}
}


static void
layout_from_line (GthImageList *image_list, 
		  int           line)
{
	GthImageListPrivate *priv = image_list->priv;
	GList               *scan;
	int                  height;

	if (! GTK_WIDGET_REALIZED (image_list))
		return;

	free_line_info_from (image_list, line);

	height = priv->row_spacing;
	for (scan = priv->lines; scan; scan = scan->next) {
		GthImageListLine *line = scan->data;
		height += IMAGE_LINE_HEIGHT (image_list, line);
	}

	relayout_images_at (image_list, 
			    line * gth_image_list_get_items_per_line (image_list), 
			    height);
}


static void
reset_text_width (GthImageList *image_list)
{
	GthImageListPrivate *priv = image_list->priv;
	GList               *scan;

	pango_layout_set_width (priv->layout, priv->max_item_width * PANGO_SCALE);
	pango_layout_set_width (priv->comment_layout, priv->max_item_width * PANGO_SCALE);

	for (scan = priv->image_list; scan; scan = scan->next) {
		GthImageListItem *item = scan->data;
		item->label_area.width = -1;
		item->comment_area.width = -1;
	}
	priv->update_width = FALSE;
}


static gboolean
layout_all_images_cb (gpointer data)
{
	GthImageList        *image_list = data;
	GthImageListPrivate *priv = image_list->priv;

	if (image_list->priv->layout_timeout != 0)
		g_source_remove (image_list->priv->layout_timeout);
	
	if (priv->update_width) 
		reset_text_width (image_list);

	free_line_info (image_list);
	priv->dirty = FALSE;
	relayout_images_at (image_list, 0, priv->row_spacing);

	image_list->priv->layout_timeout = 0;

	return FALSE;
}


static void
layout_all_images (GthImageList *image_list)
{
	if (! GTK_WIDGET_REALIZED (image_list)) 
		return;

	if (image_list->priv->layout_timeout != 0)
		return;

	image_list->priv->layout_timeout = g_timeout_add (LAYOUT_TIMEOUT,
							  layout_all_images_cb,
							  image_list);
}



/**/


static void 
gth_image_list_realize (GtkWidget *widget)
{
	GthImageListPrivate *priv;
	GthImageList        *image_list;
	GdkWindowAttr        attributes;
	int                  attributes_mask;
#ifdef HAVE_RENDER
	int                  event_base, error_base;
#endif
	PangoFontDescription *font_desc;

	g_return_if_fail (GTH_IS_IMAGE_LIST (widget));

	GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

	/**/

	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.x           = widget->allocation.x;
	attributes.y           = widget->allocation.y;
	attributes.width       = widget->allocation.width;
	attributes.height      = widget->allocation.height;
	attributes.wclass      = GDK_INPUT_OUTPUT;
	attributes.visual      = gtk_widget_get_visual (widget);
	attributes.colormap    = gtk_widget_get_colormap (widget);
	attributes.event_mask  = GDK_VISIBILITY_NOTIFY_MASK;
	attributes_mask        = (GDK_WA_X 
				  | GDK_WA_Y 
				  | GDK_WA_VISUAL 
				  | GDK_WA_COLORMAP);
	widget->window = gdk_window_new (gtk_widget_get_parent_window (widget),
					 &attributes, 
					 attributes_mask);
	gdk_window_set_user_data (widget->window, widget);

	/**/

	image_list = (GthImageList*) widget;
	priv = image_list->priv;

	attributes.x = 0;
	attributes.y = 0;
	attributes.width = image_list->priv->width;
	attributes.height = image_list->priv->height;
	attributes.event_mask = (GDK_EXPOSURE_MASK 
				 | GDK_SCROLL_MASK
				 | GDK_POINTER_MOTION_MASK
				 | GDK_ENTER_NOTIFY_MASK
				 | GDK_LEAVE_NOTIFY_MASK
				 | GDK_BUTTON_PRESS_MASK
				 | GDK_BUTTON_RELEASE_MASK
				 | gtk_widget_get_events (widget));

	image_list->priv->bin_window = gdk_window_new (widget->window,
						       &attributes, 
						       attributes_mask);
	gdk_window_set_user_data (image_list->priv->bin_window, widget);

	/* Style */

	widget->style = gtk_style_attach (widget->style, widget->window);
	gdk_window_set_background (widget->window, &widget->style->base[widget->state]);
	gdk_window_set_background (image_list->priv->bin_window, &widget->style->base[widget->state]);

#ifdef HAVE_RENDER
	priv->use_render = XRenderQueryExtension (gdk_display, &event_base, &error_base);

	if (priv->use_render) {
		Display   *dpy;
		GdkVisual *gdk_visual;
		Visual    *visual;

		dpy = gdk_x11_drawable_get_xdisplay (priv->bin_window);
		gdk_visual = gtk_widget_get_visual (widget);
		visual = gdk_x11_visual_get_xvisual (gdk_visual);

		priv->format = XRenderFindVisualFormat (dpy, visual);
	}
#endif

	/* Label Layout */

	if (priv->layout != NULL)
		g_object_unref (priv->layout);

	priv->layout = gtk_widget_create_pango_layout (widget, NULL);
	pango_layout_set_wrap (priv->layout, PANGO_WRAP_WORD_CHAR);
	pango_layout_set_font_description (priv->layout, widget->style->font_desc);
	pango_layout_set_alignment (priv->layout, PANGO_ALIGN_CENTER);
	pango_layout_set_width (priv->layout, priv->max_item_width * PANGO_SCALE);

	/* Comment layout */

	if (priv->comment_layout != NULL)
		g_object_unref (priv->comment_layout);

	priv->comment_layout = pango_layout_copy (priv->layout);

	font_desc = pango_font_description_copy (pango_context_get_font_description (pango_layout_get_context (priv->comment_layout)));
	pango_font_description_set_style (font_desc, PANGO_STYLE_ITALIC);
	pango_layout_set_font_description (priv->comment_layout, font_desc);

	pango_font_description_free (font_desc);

	layout_all_images (image_list);
}


static void 
gth_image_list_unrealize (GtkWidget *widget)
{
	GthImageList  *image_list;

	g_return_if_fail (GTH_IS_IMAGE_LIST (widget));

	image_list = (GthImageList*) widget;

	gdk_window_set_user_data (image_list->priv->bin_window, NULL);
	gdk_window_destroy (image_list->priv->bin_window);
	image_list->priv->bin_window = NULL;

	if (image_list->priv->layout) {
		g_object_unref (image_list->priv->layout);
		image_list->priv->layout = NULL;
	}

	if (image_list->priv->comment_layout) {
		g_object_unref (image_list->priv->comment_layout);
		image_list->priv->comment_layout = NULL;
	}

	(* GTK_WIDGET_CLASS (parent_class)->unrealize) (widget);
}


static void
gth_image_list_style_set (GtkWidget *widget,
			  GtkStyle  *previous_style)
{
	GthImageList *image_list;

	if (! GTK_WIDGET_REALIZED (widget))
		return;

	g_return_if_fail (GTH_IS_IMAGE_LIST (widget));

	image_list = (GthImageList*) widget;

	gdk_window_set_background (widget->window, &widget->style->base[widget->state]);
	gdk_window_set_background (image_list->priv->bin_window, &widget->style->base[widget->state]);
}


static void
gth_image_list_size_allocate (GtkWidget     *widget,
			      GtkAllocation *allocation)
{
	GthImageList  *image_list;
	int            old_images_per_line;
	gboolean       width_changed = FALSE;
	double         page_size;
	gboolean       dy_changed = FALSE;

	g_return_if_fail (GTH_IS_IMAGE_LIST (widget));

	image_list = (GthImageList*) widget;

	old_images_per_line = gth_image_list_get_items_per_line (image_list);
	if (allocation->width != widget->allocation.width)
		width_changed = TRUE;

	widget->allocation = *allocation;
	image_list->priv->width = allocation->width;

	/**/

	if (image_list->priv->hadjustment != NULL) {
		page_size = allocation->width;
		image_list->priv->hadjustment->page_size = page_size;
		image_list->priv->hadjustment->page_increment = page_size * 0.9;
		image_list->priv->hadjustment->step_increment = page_size * 0.1;
		image_list->priv->hadjustment->lower = 0;
		image_list->priv->hadjustment->upper = MAX (page_size, image_list->priv->width);
		
		if (image_list->priv->hadjustment->value + allocation->width > image_list->priv->width)
			image_list->priv->hadjustment->value = MAX (image_list->priv->width - allocation->width, 0);
	}

	/**/

	if (image_list->priv->vadjustment != NULL) {
		page_size = allocation->height;
		image_list->priv->vadjustment->page_size = page_size;
		image_list->priv->vadjustment->step_increment = page_size * 0.1;
		image_list->priv->vadjustment->page_increment = page_size * 0.9;
		image_list->priv->vadjustment->lower = 0;
		image_list->priv->vadjustment->upper = MAX (page_size, image_list->priv->height);
	
		if (image_list->priv->vadjustment->value + allocation->height > image_list->priv->height) {
			dy_changed = TRUE;
			gtk_adjustment_set_value (image_list->priv->vadjustment,
						  MAX (image_list->priv->height - page_size, 0));
		}
	}
	
	/**/

	if (GTK_WIDGET_REALIZED (widget)) 
		gdk_window_move_resize (widget->window,
					allocation->x, 
					allocation->y,
					allocation->width, 
					allocation->height);
	
	if (GTK_WIDGET_REALIZED (widget)) {
		if (width_changed
		    && (old_images_per_line != gth_image_list_get_items_per_line (image_list)))
			layout_all_images (image_list);

		if (dy_changed)
			queue_draw (image_list);
	}

	update_scrollbar_adjust (image_list);
}


static int
get_first_visible_at_offset (GthImageList *image_list, 
			     double        ofs)
{
	GList *scan;
	int    pos, line_no;

	if (image_list->priv->images == 0)
		return -1;

	line_no = 0;
	for (scan = image_list->priv->lines; scan && (ofs > 0.0); scan = scan->next) {
		GthImageListLine *line = scan->data;
		ofs -= IMAGE_LINE_HEIGHT (image_list, line);
		line_no++;
	}
	
	pos = gth_image_list_get_items_per_line (image_list) * (line_no - 1);

	if (pos < 0)
		pos = 0;
	if (pos >= image_list->priv->images)
		pos = image_list->priv->images - 1;

	return pos;
}


int
gth_image_list_get_first_visible (GthImageList *image_list)
{
	g_return_val_if_fail (GTH_IS_IMAGE_LIST (image_list), -1);
	return get_first_visible_at_offset (image_list, image_list->priv->vadjustment->value);
}


static int
get_last_visible_at_offset (GthImageList *image_list, 
			    double        ofs)
{
	int    pos, line_no;
	GList *scan;

	if (image_list->priv->images == 0)
		return -1;

	line_no = 0;
	for (scan = image_list->priv->lines; scan && (ofs > 0.0); scan = scan->next) {
		GthImageListLine *line = scan->data;
		ofs -= IMAGE_LINE_HEIGHT (image_list, line);
		line_no++;
	}
	
	pos = gth_image_list_get_items_per_line (image_list) * line_no - 1;

	if (pos < 0)
		pos = 0;

	if (pos >= image_list->priv->images)
		pos = image_list->priv->images - 1;

	return pos;
}


int
gth_image_list_get_last_visible (GthImageList *image_list)
{
	g_return_val_if_fail (image_list != NULL, -1);
	return get_last_visible_at_offset (image_list, 
					   (image_list->priv->vadjustment->value 
					    + image_list->priv->vadjustment->page_size));
}


static void
get_item_bounding_box (GthImageList     *image_list,
		       GthImageListItem *item,
		       GdkRectangle     *item_rectangle)
{
	gboolean view_text, view_comment;

	item_get_view_mode (image_list, item, &view_text, &view_comment);

	*item_rectangle = item->slide_area;
	item_rectangle->width = image_list->priv->max_item_width;
	item_rectangle->height = image_list->priv->max_item_width;

	if (view_text) {
		GdkRectangle tmp_rectangle;
		tmp_rectangle = *item_rectangle;
		gdk_rectangle_union (&tmp_rectangle,
				     &item->label_area,
				     item_rectangle);
	}
				     
	if (view_comment) {
		GdkRectangle tmp_rectangle;
		tmp_rectangle = *item_rectangle;
		gdk_rectangle_union (&tmp_rectangle,
				     &item->comment_area,
				     item_rectangle);
	}

	/* include the border */

	item_rectangle->x -= 1;
	item_rectangle->y -= 1;
	item_rectangle->width += 2;
	item_rectangle->height += 2;

	/* include the selection */

	item_rectangle->x -= FRAME_SELECTION_BORDER;
	item_rectangle->y -= FRAME_SELECTION_BORDER;
	item_rectangle->width += FRAME_SELECTION_BORDER * 2;
	item_rectangle->height += FRAME_SELECTION_BORDER * 2;
}


static void
paint_item (GthImageList     *image_list,
	    GthImageListItem *item,
	    GdkRectangle     *expose_area)
{
	GtkWidget    *widget = (GtkWidget*) image_list;
	GtkStateType  state, text_state, focus_state;
	GdkRectangle  item_rectangle, rect;
	gboolean      view_label, view_comment;
	gboolean      focused;

	if ((item->image_area.x == -1) || (item->slide_area.x == -1))
		return;

	get_item_bounding_box (image_list, item, &item_rectangle);
	if (! gdk_rectangle_intersect (expose_area, &item_rectangle, &rect))
		return;

	focused = GTK_WIDGET_HAS_FOCUS (widget) && item->focused;
	if (item->selected) 
		state = (GTK_WIDGET_HAS_FOCUS (widget))? GTK_STATE_SELECTED: GTK_STATE_ACTIVE;
	else 
		state = (focused)? GTK_STATE_ACTIVE: GTK_STATE_NORMAL;

	if (item->selected)
		text_state = state;
	else
		text_state = GTK_STATE_NORMAL;

	if (item->selected && focused)
		focus_state = GTK_STATE_NORMAL;
	else
		focus_state = state;

	/* Slide */

	gthumb_draw_slide (item->slide_area.x, 
			   item->slide_area.y,
			   image_list->priv->max_item_width, 
			   image_list->priv->max_item_width, 
			   item->image_area.width, 
			   item->image_area.height,
			   image_list->priv->bin_window,
			   widget->style->base_gc[GTK_STATE_NORMAL],
			   widget->style->black_gc,
			   widget->style->dark_gc[GTK_STATE_NORMAL],
			   widget->style->mid_gc[GTK_STATE_NORMAL],
			   widget->style->light_gc[GTK_STATE_NORMAL],
			   image_list->priv->enable_thumbs);

	if (item->selected) {
                GdkGC *sel_gc;
 
		sel_gc = gdk_gc_new (image_list->priv->bin_window);
		gdk_gc_copy (sel_gc, widget->style->base_gc[state]);
		gdk_gc_set_line_attributes (sel_gc, FRAME_SELECTION_BORDER, 0, 0, 0);
 
		gdk_draw_rectangle (image_list->priv->bin_window,
				    sel_gc,
				    FALSE,
				    item->slide_area.x - FRAME_SELECTION_BORDER + 1,
				    item->slide_area.y - FRAME_SELECTION_BORDER + 1,
				    image_list->priv->max_item_width + (FRAME_SELECTION_BORDER * 2) - 2,
				    image_list->priv->max_item_width + (FRAME_SELECTION_BORDER * 2) - 2);
                 
		g_object_unref (sel_gc);
        }

	/* Image */

	if (item->pixmap != NULL) {
		GdkGC *image_gc;

		image_gc = gdk_gc_new (image_list->priv->bin_window);

		if (item->mask != NULL) {
			gdk_gc_set_clip_origin (image_gc, item->image_area.x, item->image_area.y);
			gdk_gc_set_clip_mask (image_gc, item->mask);
		}
		
		gdk_draw_drawable (image_list->priv->bin_window,
				   image_gc,
				   item->pixmap,
				   0, 0,
				   item->image_area.x,
				   item->image_area.y,
				   item->image_area.width,
				   item->image_area.height);

		g_object_unref (image_gc);
	}

	item_get_view_mode (image_list, item, &view_label, &view_comment);

	/* Label */

	if (view_label) {
		gdk_draw_rectangle (image_list->priv->bin_window,
				    widget->style->base_gc[text_state],
				    TRUE,
				    item->label_area.x - 1,
				    item->label_area.y - 1,
				    item->label_area.width + 2,
				    item->label_area.height + 2);
		
		pango_layout_set_text (image_list->priv->layout, item->label, strlen (item->label));
		gdk_draw_layout (image_list->priv->bin_window,
				 widget->style->text_gc[text_state],
				 item->label_area.x - (image_list->priv->max_item_width - item->label_area.width) / 2,
				 item->label_area.y,
				 image_list->priv->layout);
	}

	/* Comment */

	if (view_comment) {
		gdk_draw_rectangle (image_list->priv->bin_window,
				    widget->style->base_gc[text_state],
				    TRUE,
				    item->comment_area.x - 1,
				    item->comment_area.y - 1,
				    item->comment_area.width + 2,
				    item->comment_area.height + 2);
		
		pango_layout_set_text (image_list->priv->comment_layout, item->comment, strlen (item->comment));
		gdk_draw_layout (image_list->priv->bin_window,
				 widget->style->text_gc[text_state],
				 item->comment_area.x - (image_list->priv->max_item_width - item->comment_area.width) / 2,
				 item->comment_area.y,
				 image_list->priv->comment_layout);
	}

	/* Focus */

	if (focused) {
		gtk_paint_focus (widget->style, 
				 image_list->priv->bin_window, 
				 focus_state, 
				 expose_area,
				 widget, 
				 "button",
				 item->slide_area.x + 2,
				 item->slide_area.y + 2,
				 image_list->priv->max_item_width - 4,
				 image_list->priv->max_item_width - 4);
	}
}
	    

static void
paint_rubberband (GthImageList *image_list,
		  GdkRectangle *expose_area)
{
	GthImageListPrivate *priv = image_list->priv;
	GtkWidget           *widget = GTK_WIDGET (image_list);
	GdkColor             color;
	guint32              rgba;
	GdkRectangle         rect;
	GdkPixbuf           *pixbuf;
	GdkGC               *sel_gc;

	color = widget->style->base[GTK_STATE_SELECTED];
	rgba = ((to_255 (color.red) << 24) 
		| (to_255 (color.green) << 16) 
		| (to_255 (color.blue) << 8)
		| 0x00000040);

	if (! gdk_rectangle_intersect (expose_area, &priv->selection_area, &rect))
		return;

#ifdef HAVE_RENDER
	if (priv->use_render) {
		GdkDrawable              *drawable;
		int                       x_offset, y_offset;
		Display                  *dpy;
		Picture                   pict;
		guchar                    r, g, b, a;
		XRenderPictureAttributes  attributes;
		XRenderColor              color;
		
		gdk_window_get_internal_paint_info (priv->bin_window, 
						    &drawable,
						    &x_offset, 
						    &y_offset);
		
		dpy = gdk_x11_drawable_get_xdisplay (drawable);
		
		pict = XRenderCreatePicture (dpy,
					     gdk_x11_drawable_get_xid (drawable),
					     priv->format,
					     0,
					     &attributes);
		
		/* Convert to premultiplied alpha: */

		r = (rgba >> 24) & 0xff;
		g = (rgba >> 16) & 0xff;
		b = (rgba >> 8) & 0xff;
		a = (rgba >> 0) & 0xff;
		
		r = r * a / 255;
		g = g * a / 255;
		b = b * a / 255;
		
		color.red   = (r << 8) + r;
		color.green = (g << 8) + g;
		color.blue  = (b << 8) + b;
		color.alpha = (a << 8) + a;
		
		XRenderFillRectangle (dpy,
				      PictOpOver,
				      pict,
				      &color,
				      rect.x - x_offset, 
				      rect.y - y_offset,
				      rect.width, 
				      rect.height);
		XRenderFreePicture (dpy, pict);
		
	} else {
#endif

		pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, rect.width, rect.height);
		gdk_pixbuf_fill (pixbuf, rgba);
		gdk_pixbuf_render_to_drawable_alpha (pixbuf,
						     priv->bin_window,
						     0, 0,
						     rect.x, rect.y,
						     rect.width, rect.height,
						     GDK_PIXBUF_ALPHA_FULL,
						     0,
						     GDK_RGB_DITHER_NONE,
						     0, 0);
		g_object_unref (pixbuf);

#ifdef HAVE_RENDER
		}
#endif

	/* Paint outline */

	sel_gc = gdk_gc_new (image_list->priv->bin_window);
	gdk_gc_copy (sel_gc, widget->style->bg_gc[GTK_STATE_SELECTED]);
	gdk_gc_set_clip_rectangle (sel_gc, &rect);
	
	if ((priv->selection_area.width > 1)
	    && (priv->selection_area.height > 1)) 
		gdk_draw_rectangle (image_list->priv->bin_window,
				    sel_gc,
				    FALSE,
				    priv->selection_area.x,
				    priv->selection_area.y,
				    priv->selection_area.width - 1,
				    priv->selection_area.height - 1);
	
	g_object_unref (sel_gc);

}


static gboolean
gth_image_list_expose (GtkWidget      *widget, 
		       GdkEventExpose *event)
{
	GthImageList        *image_list = (GthImageList*) widget;
	GthImageListPrivate *priv = image_list->priv;
	int                  pos_start, pos_end;
	int                  i;
	GList               *scan;

	if (event->window != priv->bin_window)
		return FALSE;

	pos_start = gth_image_list_get_first_visible (image_list);
	pos_end = gth_image_list_get_last_visible (image_list);

	scan = g_list_nth (image_list->priv->image_list, pos_start);

	if (pos_start == -1) 
		return FALSE;

	for (i = pos_start; i <= pos_end && scan; i++, scan = scan->next) {
		GthImageListItem *item = scan->data;
		GdkRectangle     *rectangles;
		int               n_rectangles;
       
		gdk_region_get_rectangles (event->region,
					   &rectangles,
					   &n_rectangles);

		while (n_rectangles-- != 0)
			paint_item (image_list, item, &rectangles[n_rectangles]);

		g_free (rectangles);
	}
	
	if (priv->selecting || priv->multi_selecting_with_keyboard) {
		GdkRectangle *rectangles;
		int           n_rectangles;
       
		gdk_region_get_rectangles (event->region,
					   &rectangles,
					   &n_rectangles);
       
		while (n_rectangles-- != 0)
			paint_rubberband (image_list, &rectangles[n_rectangles]);
 
		g_free (rectangles);
	}

	return TRUE;
}


static void
set_scroll_adjustments (GthImageList  *image_list,
			GtkAdjustment *hadj,
			GtkAdjustment *vadj)
{
	g_return_if_fail (GTH_IS_IMAGE_LIST (image_list));

        if (hadj != NULL)
                g_return_if_fail (GTK_IS_ADJUSTMENT (hadj));
        else
                hadj = GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 0.0, 0.0, 
							   0.0, 0.0, 0.0));

        if (vadj != NULL)
                g_return_if_fail (GTK_IS_ADJUSTMENT (vadj));
        else
		vadj = GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 0.0, 0.0, 
							   0.0, 0.0, 0.0));

	if ((image_list->priv->hadjustment != NULL) 
	    && (image_list->priv->hadjustment != hadj)) {
		g_signal_handlers_disconnect_by_data (G_OBJECT (image_list->priv->hadjustment),
						      image_list);
		g_object_unref (image_list->priv->hadjustment);
		image_list->priv->hadjustment = NULL;
        }

	if ((image_list->priv->vadjustment != NULL) 
	    && (image_list->priv->vadjustment != vadj)) {
		g_signal_handlers_disconnect_by_data (G_OBJECT (image_list->priv->vadjustment),
						      image_list);
		g_object_unref (image_list->priv->vadjustment);
		image_list->priv->vadjustment = NULL;
        }

        if (image_list->priv->hadjustment != hadj) {
                image_list->priv->hadjustment = hadj;
                g_object_ref (image_list->priv->hadjustment);
                gtk_object_sink (GTK_OBJECT (image_list->priv->hadjustment));

		g_signal_connect (G_OBJECT (image_list->priv->hadjustment), 
				  "value_changed",
				  G_CALLBACK (gth_image_list_adjustment_value_changed),
				  image_list);
		g_signal_connect (G_OBJECT (image_list->priv->hadjustment), 
				  "changed",
				  G_CALLBACK (gth_image_list_adjustment_changed),
				  image_list);
        }

        if (image_list->priv->vadjustment != vadj) {
                image_list->priv->vadjustment = vadj;
                g_object_ref (image_list->priv->vadjustment);
                gtk_object_sink (GTK_OBJECT (image_list->priv->vadjustment));

		g_signal_connect (G_OBJECT (image_list->priv->vadjustment), 
				  "value_changed",
				  G_CALLBACK (gth_image_list_adjustment_value_changed),
				  image_list);
		g_signal_connect (G_OBJECT (image_list->priv->vadjustment), 
				  "changed",
				  G_CALLBACK (gth_image_list_adjustment_changed),
				  image_list);
        }
}


void
gth_image_list_set_hadjustment (GthImageList  *image_list,
				GtkAdjustment *adjustment)
{
	g_return_if_fail (GTH_IS_IMAGE_LIST (image_list));
	
	set_scroll_adjustments (image_list, 
				image_list->priv->vadjustment,
				adjustment);

	g_object_notify (G_OBJECT (image_list), "hadjustment");
}


GtkAdjustment *
gth_image_list_get_hadjustment (GthImageList *image_list)
{
	g_return_val_if_fail (GTH_IS_IMAGE_LIST (image_list), NULL);

	if (image_list->priv->hadjustment == NULL)
		gth_image_list_set_hadjustment (image_list, NULL);

	return image_list->priv->hadjustment;
}


void
gth_image_list_set_vadjustment (GthImageList  *image_list,
				GtkAdjustment *adjustment)
{
	g_return_if_fail (GTH_IS_IMAGE_LIST (image_list));
	
	set_scroll_adjustments (image_list, 
				adjustment, 
				image_list->priv->hadjustment);

	g_object_notify (G_OBJECT (image_list), "vadjustment");
}


GtkAdjustment *
gth_image_list_get_vadjustment (GthImageList  *image_list)
{
	g_return_val_if_fail (GTH_IS_IMAGE_LIST (image_list), NULL);

	if (image_list->priv->vadjustment == NULL)
		gth_image_list_set_vadjustment (image_list, NULL);

	return image_list->priv->vadjustment;
}


static void
keep_focus_consistent (GthImageList *image_list)
{
	if (image_list->priv->focused_item > image_list->priv->images - 1)
		image_list->priv->focused_item = - 1;
}


static gboolean 
gth_image_list_focus_in (GtkWidget     *widget,
			 GdkEventFocus *event)
{
	GthImageList *image_list = GTH_IMAGE_LIST (widget);

	GTK_WIDGET_SET_FLAGS (widget, GTK_HAS_FOCUS);

	keep_focus_consistent (GTH_IMAGE_LIST (widget));
	if ((image_list->priv->focused_item == -1) && (image_list->priv->images > 0)) 
		gth_image_list_set_cursor (image_list, 0);
	queue_draw (image_list);

	return TRUE;
}


static void
stop_dragging (GthImageList *image_list)
{
	if (! image_list->priv->dragging)
		return;
	image_list->priv->dragging = FALSE;
	image_list->priv->drag_started = FALSE;
}


static void
stop_selection (GthImageList *image_list)
{
	if (! image_list->priv->selecting)
		return;

	image_list->priv->selecting = FALSE;
	image_list->priv->sel_start_x = 0;
	image_list->priv->sel_start_y = 0;
	
	if (image_list->priv->timer_tag != 0) {
		g_source_remove (image_list->priv->timer_tag);
		image_list->priv->timer_tag = 0;
	}

	gdk_window_invalidate_rect (image_list->priv->bin_window,
				    &image_list->priv->selection_area,
				    FALSE);

}


static gboolean 
gth_image_list_focus_out (GtkWidget     *widget,
			  GdkEventFocus *event)
{
	GTK_WIDGET_UNSET_FLAGS (widget, GTK_HAS_FOCUS);
	queue_draw (GTH_IMAGE_LIST (widget));
	return TRUE;
}


static gint
gth_image_list_key_press (GtkWidget   *widget,
			  GdkEventKey *event)
{
	GthImageList *image_list = GTH_IMAGE_LIST (widget);
	gboolean      handled;

	if (! image_list->priv->multi_selecting_with_keyboard
	    && (event->state & GDK_SHIFT_MASK)
	    && ((event->keyval == GDK_Left)
		|| (event->keyval == GDK_Right)
		|| (event->keyval == GDK_Up)
		|| (event->keyval == GDK_Down)
		|| (event->keyval == GDK_Page_Up)
		|| (event->keyval == GDK_Page_Down)
		|| (event->keyval == GDK_Home)
		|| (event->keyval == GDK_End))) {
		image_list->priv->multi_selecting_with_keyboard = TRUE;
		image_list->priv->old_focused_item = image_list->priv->focused_item;

		image_list->priv->selection_area.x = 0;
		image_list->priv->selection_area.y = 0;
		image_list->priv->selection_area.width = 0;
		image_list->priv->selection_area.height = 0;
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
gth_image_list_key_release (GtkWidget   *widget,
			    GdkEventKey *event)
{
	GthImageList *image_list = GTH_IMAGE_LIST (widget);

	if (image_list->priv->multi_selecting_with_keyboard
	    && (event->state & GDK_SHIFT_MASK)
	    && ((event->keyval == GDK_Shift_L)
		|| (event->keyval == GDK_Shift_R))) 
		image_list->priv->multi_selecting_with_keyboard = FALSE;

	queue_draw (image_list);

	if (GTK_WIDGET_CLASS (parent_class)->key_press_event &&
	    GTK_WIDGET_CLASS (parent_class)->key_press_event (widget, event))
		return TRUE;

	return FALSE;
}


static gboolean
gth_image_list_scroll_event (GtkWidget      *widget, 
			     GdkEventScroll *event)
{
	gdouble        new_value;
	GtkAdjustment *adj;

	if (event->direction != GDK_SCROLL_UP &&
	    event->direction != GDK_SCROLL_DOWN)
		return FALSE;

	adj = ((GthImageList*) widget)->priv->vadjustment;

	if (event->direction == GDK_SCROLL_UP)
		new_value = adj->value - adj->page_increment / 4;
	else
		new_value = adj->value + adj->page_increment / 4;

	new_value = CLAMP (new_value, adj->lower, adj->upper - adj->page_size);
	gtk_adjustment_set_value (adj, new_value);

	return TRUE;
}


static void
queue_draw_item (GthImageList     *image_list, 
		 GthImageListItem *item)
{
	GdkRectangle rect;

	if (image_list->priv->frozen > 0)
		return;

	get_item_bounding_box (image_list, item, &rect);
	gdk_window_invalidate_rect (image_list->priv->bin_window, &rect, FALSE);
}


static void
real_select_image (GthImageList *image_list, 
		   int           pos)
{
	GthImageListPrivate *priv = image_list->priv;
	GthImageListItem    *item;
	GList               *link;
	
	g_return_if_fail ((pos >= 0) && (pos < priv->images));

	link = g_list_nth (priv->image_list, pos);
	g_return_if_fail (link != NULL);
	item = link->data;

	if (item->selected)
		return;

	item->selected = TRUE;
	priv->selection = g_list_prepend (priv->selection, GINT_TO_POINTER (pos));
	priv->selection_changed = TRUE;

	queue_draw_item (image_list, item);
}


static void
real_unselect_image (GthImageList *image_list, 
		     int           pos)
{
	GthImageListPrivate *priv = image_list->priv;
	GthImageListItem    *item;
	GList               *link;

	g_return_if_fail ((pos >= 0) && (pos < priv->images));

	link = g_list_nth (priv->image_list, pos);
	g_return_if_fail (link != NULL);
	item = link->data;

	if (! item->selected)
		return;

	item->selected = FALSE;
	priv->selection = g_list_remove (priv->selection, GINT_TO_POINTER (pos));
	priv->selection_changed = TRUE;

	queue_draw_item (image_list, item);
}


static void
real_select (GthImageList *image_list, 
	     gboolean      select, 
	     int           pos)
{
	if (select)
		real_select_image (image_list, pos);
	else
		real_unselect_image (image_list, pos);
}


static void
emit_selection_changed (GthImageList *image_list)
{
	if (image_list->priv->selection_changed) {
		g_signal_emit (image_list, image_list_signals[SELECTION_CHANGED], 0);
		image_list->priv->selection_changed = FALSE;
	}
}


static void
real_select__emit (GthImageList *image_list, 
		   gboolean      select, 
		   int           pos)
{
	real_select (image_list, select, pos);
	emit_selection_changed (image_list);
}


void
gth_image_list_select_image (GthImageList *image_list,
			     int           pos)
{
	GthImageListPrivate *priv = image_list->priv;
	GList               *list;
	int                  i;

	switch (priv->selection_mode) {
	case GTK_SELECTION_SINGLE:
		i = 0;

		for (list = priv->image_list; list; list = list->next) {
			GthImageListItem *item = list->data;

			if ((i != pos) && item->selected)
				real_select (image_list, FALSE, i);

			i++;
		}

		real_select (image_list, TRUE, pos);
		break;

	case GTK_SELECTION_MULTIPLE:
		real_select (image_list, TRUE, pos);
		break;

	default:
		break;
	}

	emit_selection_changed (image_list);
}


void
gth_image_list_unselect_image (GthImageList *image_list,
			       int           pos)
{
	g_return_if_fail (GTH_IS_IMAGE_LIST (image_list));
	real_select (image_list, FALSE, pos);
	emit_selection_changed (image_list);
}


static void
real_select_all (GthImageList *image_list)
{
	GthImageListPrivate *priv = image_list->priv;
	GList               *scan;
	int                  i;

	g_return_if_fail (GTH_IS_IMAGE_LIST (image_list));

	for (scan = priv->image_list, i = 0; scan; scan = scan->next, i++) {
		GthImageListItem *item = scan->data;

		if (! item->selected)
			real_select (image_list, TRUE, i);
	}
}


void
gth_image_list_select_all (GthImageList *image_list)
{
	g_return_if_fail (GTH_IS_IMAGE_LIST (image_list));
	real_select_all (image_list);
	emit_selection_changed (image_list);
}


static gboolean
key_binding_select_all (GthImageList *image_list)
{
	gth_image_list_select_all (image_list);
	return TRUE;
}


static gboolean
key_binding_unselect_all (GthImageList *image_list)
{
	gth_image_list_unselect_all (image_list);
	return TRUE;
}


static int
real_unselect_all (GthImageList *image_list,
		   gpointer      keep)
{
	GthImageListPrivate *priv = image_list->priv;
	GList               *scan;
	int                  i, idx = 0;

	g_return_val_if_fail (GTH_IS_IMAGE_LIST (image_list), -1);

	for (scan = priv->image_list, i = 0; scan; scan = scan->next, i++) {
		GthImageListItem *item = scan->data;

		if (item == keep)
			idx = i;
		else if (item->selected)
			real_select (image_list, FALSE, i);
	}	

	return idx;
}


void
gth_image_list_unselect_all (GthImageList *image_list)
{
	g_return_if_fail (GTH_IS_IMAGE_LIST (image_list));
	real_unselect_all (image_list, NULL);
	emit_selection_changed (image_list);
}


gboolean
gth_image_list_pos_is_selected (GthImageList     *image_list,
				int               pos)
{
	GList *scan;

	for (scan = image_list->priv->selection; scan; scan = scan->next) 
                if (GPOINTER_TO_INT (scan->data) == pos) 
			return TRUE;

        return FALSE;
}


int
gth_image_list_get_first_selected (GthImageList *image_list)
{
	GList *scan = image_list->priv->selection;
	int    pos;

	if (scan == NULL)
		return -1;

	pos = GPOINTER_TO_INT (scan->data);
        for (scan = scan->next; scan; scan = scan->next) 
		pos = MIN (pos, GPOINTER_TO_INT (scan->data));

	return pos;
}


int
gth_image_list_get_last_selected (GthImageList *image_list)
{
	GList *scan = image_list->priv->selection;
	int    pos;

	if (scan == NULL)
		return -1;

	pos = GPOINTER_TO_INT (scan->data);
        for (scan = scan->next; scan; scan = scan->next) 
		pos = MAX (pos, GPOINTER_TO_INT (scan->data));

	return pos;
}


static void
select_range (GthImageList     *image_list, 
	      GthImageListItem *item,
	      int               pos, 
	      GdkEvent         *event)
{
	GthImageListPrivate *priv = image_list->priv;
	int                  a, b;
	GList               *scan;

	if (priv->last_selected_pos == -1) {
		priv->last_selected_pos = pos;
		priv->last_selected_item = item;
	}

	if (pos < priv->last_selected_pos) {
		a = pos;
		b = priv->last_selected_pos;
	} else {
		a = priv->last_selected_pos;
		b = pos;
	}

	scan = g_list_nth (priv->image_list, a);

	for (; a <= b; a++, scan = scan->next) {
		GthImageListItem *item = scan->data;

		if (! item->selected)
			real_select (image_list, TRUE, a);
	}

	real_select (image_list, TRUE, pos);
	emit_selection_changed (image_list);

	gth_image_list_set_cursor (image_list, pos);
}


static void
do_select_many (GthImageList     *image_list, 
		GthImageListItem *item,
		int               pos, 
		GdkEvent         *event)
{
	gboolean range, additive;

	range    = (event->button.state & GDK_SHIFT_MASK) != 0;
	additive = (event->button.state & GDK_CONTROL_MASK) != 0;

	if (! additive && ! range) {

		if (item->selected) {
			/* postpone selection to handle dragging. */
			image_list->priv->select_pending = TRUE;
			image_list->priv->select_pending_pos = pos;
			image_list->priv->select_pending_item = item;

		} else {
			real_unselect_all (image_list, NULL);
			real_select__emit (image_list, TRUE, pos);
			image_list->priv->last_selected_pos = pos;
			image_list->priv->last_selected_item = item;
		}

	} else if (range) {
		real_unselect_all (image_list, item);
		select_range (image_list, item, pos, event);

	} else if (additive) {
		real_select__emit (image_list, ! item->selected, pos);
		image_list->priv->last_selected_pos = pos;
		image_list->priv->last_selected_item = item;
	} 

	gth_image_list_set_cursor (image_list, pos);
}


static void
selection_multiple_button_press (GthImageList   *image_list, 
				 GdkEventButton *event,
				 int             pos)
{
	GthImageListItem *item;

	item = g_list_nth (image_list->priv->image_list, pos)->data;
	do_select_many (image_list, item, pos, (GdkEvent*) event);
}


static void
store_temp_selection (GthImageList *image_list)
{
	GList *scan;

	for (scan = image_list->priv->image_list; scan; scan = scan->next) {
		GthImageListItem *item = scan->data;
		item->tmp_selected = item->selected;
	}
}


static gint
gth_image_list_button_press (GtkWidget      *widget, 
			     GdkEventButton *event)
{
	GthImageList        *image_list = GTH_IMAGE_LIST (widget);
	GthImageListPrivate *priv = image_list->priv;
	int                  pos;

	if (event->window == image_list->priv->bin_window) 
		if (! GTK_WIDGET_HAS_FOCUS (widget))
			gtk_widget_grab_focus (widget);

	pos = gth_image_list_get_image_at (image_list, event->x, event->y);

	if ((pos != -1) 
	    && (event->button == 1)
	    && (event->type == GDK_BUTTON_PRESS)) {

		/* This can be the start of a dragging action. */
		priv->dragging = TRUE;
		priv->drag_start_x = event->x;
		priv->drag_start_y = event->y;

		if (priv->selection_mode == GTK_SELECTION_MULTIPLE)
			selection_multiple_button_press (image_list, event, pos);
	}

	if ((pos != -1)
	    && (event->button == 1)
	    && (event->type == GDK_2BUTTON_PRESS)) {
		g_signal_emit (image_list, image_list_signals[ITEM_ACTIVATED], 0, pos);
		return TRUE;
	}

	if ((pos == -1)
	    && (event->button == 1)) {
		gboolean additive;

		additive = (event->state & GDK_CONTROL_MASK) != 0;

		if (! additive)
			gth_image_list_unselect_all (image_list);

		if (priv->selecting)
			return FALSE;

		store_temp_selection (image_list);

		priv->sel_start_x = event->x;
		priv->sel_start_y = event->y;
		priv->selection_area.x = event->x;
		priv->selection_area.y = event->y;
		priv->selection_area.width = 0;
		priv->selection_area.height = 0;
		priv->sel_state = event->state;
		priv->selecting = TRUE;
	}

	return FALSE;
}


/* Returns whether the specified image is at least partially inside the 
 * specified rectangle.
 */
static gboolean
image_is_in_area (GthImageList     *image_list,
		  GthImageListItem *item, 
		  int               x1, 
		  int               y1, 
		  int               x2, 
		  int               y2)
{
	GdkRectangle  area_rectangle;
	GdkRectangle  item_rectangle;
	GdkRectangle  tmp_rectangle;
	double        x_ofs, y_ofs;

	if ((x1 == x2) && (y1 == y2))
		return FALSE;

	area_rectangle.x = x1;
	area_rectangle.y = y1;
	area_rectangle.width = x2 - x1;
	area_rectangle.height = y2 - y1;

	get_item_bounding_box (image_list, item, &item_rectangle);
	x_ofs = item_rectangle.width / 6;
	y_ofs = item_rectangle.height / 6;
	item_rectangle.x      += x_ofs;
	item_rectangle.y      += y_ofs;
	item_rectangle.width  -= x_ofs * 2;
	item_rectangle.height -= y_ofs * 2;

	return gdk_rectangle_intersect (&item_rectangle,
					&area_rectangle,
					&tmp_rectangle);
}


static void
update_mouse_selection (GthImageList *image_list, 
			int           x, 
			int           y)
{
	GthImageListPrivate *priv = image_list->priv;
	int                  x1, y1, x2, y2;
	gboolean             additive, invert;
	int                  min_y, max_y;
	GList               *l, *begin, *end;
	int                  i, begin_idx, end_idx;
	GdkRectangle         old_area, common;
	GdkRegion           *invalid_region;

	old_area = priv->selection_area;
	invalid_region = gdk_region_rectangle (&old_area);

	if (priv->sel_start_x < x) {
		x1 = priv->sel_start_x;
		x2 = x;
	} else {
		x1 = x;
		x2 = priv->sel_start_x;
	}

	if (priv->sel_start_y < y) {
		y1 = priv->sel_start_y;
		y2 = y;
	} else {
		y1 = y;
		y2 = priv->sel_start_y;
	}

	x1 = CLAMP (x1, 0, priv->width - 1);
	y1 = CLAMP (y1, 0, priv->height - 1);
	x2 = CLAMP (x2, 0, priv->width - 1);
	y2 = CLAMP (y2, 0, priv->height - 1);

	priv->selection_area.x = x1;
	priv->selection_area.y = y1;
	priv->selection_area.width = x2 - x1;
	priv->selection_area.height = y2 - y1;

	/* taken from eggiconlist.c,  Copyright (C) 2002  Anders Carlsson */

	gdk_region_union_with_rect (invalid_region, &priv->selection_area);
	gdk_rectangle_intersect (&old_area, &priv->selection_area, &common);
	if (common.width > 0 && common.height > 0) {
		GdkRegion *common_region;
			
		/* make sure the border is invalidated */
		common.x += 1;
		common.y += 1;
		common.width -= 2;
		common.height -= 2;
		common_region = gdk_region_rectangle (&common);
		
		gdk_region_subtract (invalid_region, common_region);
		gdk_region_destroy (common_region);
	}
	gdk_window_invalidate_region (image_list->priv->bin_window, invalid_region, FALSE);
	gdk_region_destroy (invalid_region);

	/* Select or unselect images as appropriate */

	additive = priv->sel_state & GDK_SHIFT_MASK;
	invert   = priv->sel_state & GDK_CONTROL_MASK;

	/* Consider only images in the min_y --> max_y offset. */

	min_y = priv->selection_area.y;
	max_y = priv->selection_area.y + priv->selection_area.height;

	begin_idx = get_first_visible_at_offset (image_list, min_y);
	begin = g_list_nth (priv->image_list, begin_idx);
	end_idx = get_last_visible_at_offset (image_list, max_y);
	end = g_list_nth (priv->image_list, end_idx);

	if (end != NULL)
		end = end->next;

	gdk_window_freeze_updates (priv->bin_window);

	x1 = priv->selection_area.x;
	y1 = priv->selection_area.y;
	x2 = x1 + priv->selection_area.width;
	y2 = y1 + priv->selection_area.height;

	for (l = begin, i = begin_idx; l != end; l = l->next, i++) {
		GthImageListItem *item = l->data;

		if (image_is_in_area (image_list, item, x1, y1, x2, y2)) {
			if (invert) {
				if (item->selected == item->tmp_selected) 
					real_select (image_list, ! item->selected, i);
			} else if (additive) {
				if (! item->selected) 
					real_select (image_list, TRUE, i);
			} else {
				if (! item->selected) 
					real_select (image_list, TRUE, i);
			}

		} else if (item->selected != item->tmp_selected) 
			real_select (image_list, item->tmp_selected, i);
	}

	gdk_window_thaw_updates (priv->bin_window);

	emit_selection_changed (image_list);
}


static gint
gth_image_list_button_release (GtkWidget      *widget, 
			       GdkEventButton *event)
{
	GthImageList        *image_list = GTH_IMAGE_LIST (widget);
	GthImageListPrivate *priv = image_list->priv;

	if (priv->dragging) {
		priv->select_pending = priv->select_pending && ! priv->drag_started;
		stop_dragging (image_list);
	}

	if (priv->selecting) {
		update_mouse_selection (image_list, event->x, event->y);
		stop_selection (image_list);
	}

	if (priv->select_pending) {
		image_list->priv->select_pending = FALSE;

		real_unselect_all (image_list, NULL);
		real_select__emit (image_list, TRUE, image_list->priv->select_pending_pos);
		image_list->priv->last_selected_pos = image_list->priv->select_pending_pos;
		image_list->priv->last_selected_item = image_list->priv->select_pending_item;
	}

	return FALSE;
}


static gboolean
autoscroll_cb (gpointer data)
{
	GthImageList        *image_list = data;
	GthImageListPrivate *priv = image_list->priv;
	double               max_value;
	double               value;

	GDK_THREADS_ENTER ();

	max_value = priv->vadjustment->upper - priv->vadjustment->page_size;
	value = priv->vadjustment->value + priv->value_diff;

	if (value > max_value)
		value = max_value;

	gtk_adjustment_set_value (priv->vadjustment, value);
	priv->event_last_y = priv->event_last_y + priv->value_diff;
	update_mouse_selection (image_list, priv->event_last_x,	priv->event_last_y);

	GDK_THREADS_LEAVE();

	return TRUE;
}


static gboolean
gth_image_list_motion_notify (GtkWidget      *widget, 
			      GdkEventMotion *event)
{
	GthImageList        *image_list = GTH_IMAGE_LIST (widget);
	GthImageListPrivate *priv = image_list->priv;

	if (priv->dragging) {
		if (! priv->drag_started
		    && (priv->selection != NULL)
		    && gtk_drag_check_threshold (widget, 
						 priv->drag_start_x,
						 priv->drag_start_y,
						 event->x,
						 event->y)) {
			int             pos;
			GdkDragContext *context;
			gboolean        multi_dnd;

			/**/

			pos = gth_image_list_get_image_at (image_list, 
							   priv->drag_start_x,
							   priv->drag_start_y);
			if (pos != -1)
				gth_image_list_set_cursor (image_list, pos);

			/**/

			priv->drag_started = TRUE;
			context = gtk_drag_begin (widget,
						  priv->target_list,
						  GDK_ACTION_COPY | GDK_ACTION_MOVE,
						  1,
						  (GdkEvent *) event);

			multi_dnd = priv->selection->next != NULL;
			gtk_drag_set_icon_stock (context,
						 multi_dnd ? GTK_STOCK_DND_MULTIPLE : GTK_STOCK_DND,
						 -4, -4);

			return TRUE;
		}

		return TRUE;
	}
	
	if (priv->selecting) {
		double absolute_y;

		if (fabs (event->y - priv->vadjustment->value) > MAX_DIFF)
			event->y = priv->vadjustment->upper;

		update_mouse_selection (image_list, event->x, event->y);

		/* If we are out of bounds, schedule a timeout that will do 
		 * the scrolling */

		absolute_y = event->y - priv->vadjustment->value;
		if ((absolute_y < 0) || (absolute_y > widget->allocation.height)) {
			priv->event_last_x = event->x;
			priv->event_last_y = event->y;
			
			/* Make the steppings be relative to the mouse 
			 * distance from the canvas.  
			 * Also notice the timeout below is small to give a
			 * more smooth movement.
			 */
			if (absolute_y < 0)
				priv->value_diff = absolute_y;
			else
				priv->value_diff = absolute_y - widget->allocation.height;
			priv->value_diff /= 2;
			
			if (priv->timer_tag == 0)
				priv->timer_tag = g_timeout_add (SCROLL_TIMEOUT, autoscroll_cb, image_list);

		} else if (priv->timer_tag != 0) {
			g_source_remove (priv->timer_tag);
			priv->timer_tag = 0;
		}
		
		return TRUE;
	}

	return FALSE;
}


static void
get_image_center (GthImageList     *image_list,
		  GthImageListItem *item, 
		  int              *x, 
		  int              *y)
{
	int half_width = image_list->priv->max_item_width / 2;
	*x = item->slide_area.x + half_width;
	*y = item->slide_area.y + half_width;
}


static void
select_range_with_keyboard (GthImageList *image_list, 
			    int           new_focused_item)
{
	GthImageListPrivate *priv = image_list->priv;
	GList               *link;
	GthImageListItem    *item1, *item2;
	int                  x1, x2, y1, y2;
	int                  min_y, max_y;
	int                  begin_idx, end_idx, i;
	GList               *begin, *end;

	link = g_list_nth (priv->image_list, priv->old_focused_item);
	item1 = link->data;

	link = g_list_nth (priv->image_list, new_focused_item);
	item2 = link->data;

	get_image_center (image_list, item1, &x1, &y1);
	get_image_center (image_list, item2, &x2, &y2);

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

	x1 -= KEYBOARD_SELECTION_BORDER;
	y1 -= KEYBOARD_SELECTION_BORDER;
	x2 += KEYBOARD_SELECTION_BORDER;
	y2 += KEYBOARD_SELECTION_BORDER;

	min_y = MIN (priv->selection_area.y, y1);
	max_y = MAX (priv->selection_area.y + priv->selection_area.height, y2);

	priv->selection_area.x = x1;
	priv->selection_area.y = y1;
	priv->selection_area.width= x2 - x1;
	priv->selection_area.height = y2 - y1;

	queue_draw (image_list);

	/* Consider only images in the min_y --> max_y range. */

	begin_idx = get_first_visible_at_offset (image_list, min_y);
	begin = g_list_nth (priv->image_list, begin_idx);
	end_idx = get_last_visible_at_offset (image_list, max_y);
	end = g_list_nth (priv->image_list, end_idx);

	if (end != NULL)
		end = end->next;

	gdk_window_freeze_updates (priv->bin_window);

	for (link = begin, i = begin_idx; link != end; link = link->next, i++) {
		GthImageListItem *item = link->data;

		if (image_is_in_area (image_list, item, x1, y1, x2, y2)) 
			real_select_image (image_list, i);
		else
			real_unselect_image (image_list, i);
	}

	gdk_window_thaw_updates (priv->bin_window);

	emit_selection_changed (image_list);
}


static GList *
get_line_from_image (GthImageList *image_list,
		     int           focused_item)
{
	GthImageListPrivate *priv = image_list->priv;
	GthImageListItem    *item;
	GList               *scan;

	scan = g_list_nth (priv->image_list, focused_item);
	g_return_val_if_fail (scan != NULL, NULL);
	item = scan->data;

	for (scan = priv->lines; scan; scan = scan->next) {
		GthImageListLine *line = scan->data;
		
		if (g_list_find (line->image_list, item) != NULL)
			return scan;
	}

	return NULL;
}


static int
get_page_distance_image (GthImageList *image_list,
			 int           focused_item,
			 gboolean      downward)
{
	int    h, d;
	GList *line;
	int    images_per_line;
	int    old_focused_item = focused_item;

	d = downward ? 1 : -1;
	h = GTK_WIDGET (image_list)->allocation.height;
	line = get_line_from_image (image_list, focused_item);
	images_per_line = gth_image_list_get_items_per_line (image_list);

	while ((h > 0) && (line != NULL)) {
		GthImageListLine *image_line;
		int               line_height;

		image_line = line->data;
		line_height = IMAGE_LINE_HEIGHT (image_list, image_line);
		h -= line_height;

		if (h > 0) {
			int tmp;
			tmp = focused_item + d * images_per_line;
			if ((tmp > image_list->priv->images - 1) || (tmp < 0))
				return focused_item;
			else
				focused_item = tmp;
		}

		if (downward)
			line = line->next;
		else
			line = line->prev;
	}

	if (old_focused_item == focused_item) {
		int tmp = focused_item + d * images_per_line;
		
		if ((tmp >= 0) && (tmp <= image_list->priv->images - 1))
			focused_item = tmp;
	}

	return focused_item;
}


static gboolean
real_move_cursor (GthImageList       *image_list, 
		  GthCursorMovement   dir,
		  GthSelectionChange  sel_change)
{
	GthImageListPrivate *priv = image_list->priv;
	int images_per_line;
	int new_focused_item;

	if (priv->images == 0)
		return FALSE;

	if (! GTK_WIDGET_HAS_FOCUS (GTK_WIDGET (image_list)))
		return FALSE;

	images_per_line = gth_image_list_get_items_per_line (image_list);
        new_focused_item = priv->focused_item;

	if (priv->focused_item == -1) {
		priv->old_focused_item = 0;
		new_focused_item = 0;

	} else {
		switch (dir) {
		case GTH_CURSOR_MOVE_RIGHT:
			if (priv->focused_item + 1 < priv->images &&
			    priv->focused_item % images_per_line != (images_per_line - 1))
				new_focused_item++;
			break;
			
		case GTH_CURSOR_MOVE_LEFT:
			if (priv->focused_item - 1 >= 0 &&
			    priv->focused_item % images_per_line != 0)
				new_focused_item--;
			break;
			
		case GTH_CURSOR_MOVE_DOWN:
			if (priv->focused_item + images_per_line < priv->images)
				new_focused_item += images_per_line;
			break;
			
		case GTH_CURSOR_MOVE_UP:
			if (priv->focused_item - images_per_line >= 0)
				new_focused_item -= images_per_line;
			break;
			
		case GTH_CURSOR_MOVE_PAGE_UP:
			new_focused_item = get_page_distance_image (image_list,
								     new_focused_item,
								     FALSE);
			new_focused_item = CLAMP (new_focused_item, 0, priv->images - 1);
			break;
			
		case GTH_CURSOR_MOVE_PAGE_DOWN:
			new_focused_item = get_page_distance_image (image_list,
								     new_focused_item,
								     TRUE);
			new_focused_item = CLAMP (new_focused_item, 0, priv->images - 1);
			break;
			
		case GTH_CURSOR_MOVE_BEGIN:
			new_focused_item = 0;
			break;
			
		case GTH_CURSOR_MOVE_END:
			new_focused_item = priv->images - 1;
			break;
			
		default:
			break;
		}
	}

	if ((dir == GTH_CURSOR_MOVE_UP)
	    || (dir == GTH_CURSOR_MOVE_DOWN)
	    || (dir == GTH_CURSOR_MOVE_PAGE_UP)
	    || (dir == GTH_CURSOR_MOVE_PAGE_DOWN)
	    || (dir == GTH_CURSOR_MOVE_BEGIN)
	    || (dir == GTH_CURSOR_MOVE_END)) { /* Vertical movement. */
		GthVisibility visibility;
		gboolean      is_up_direction;

		visibility = gth_image_list_image_is_visible (image_list, new_focused_item);
		is_up_direction = ((dir == GTH_CURSOR_MOVE_UP)
				   || (dir == GTH_CURSOR_MOVE_PAGE_UP)
				   || (dir == GTH_CURSOR_MOVE_BEGIN));

		if (visibility != GTH_VISIBILITY_FULL) 
			gth_image_list_moveto (image_list, 
					       new_focused_item, 
					       is_up_direction ? 0.0 : 1.0);

	} else {
		GthVisibility visibility;

		visibility = gth_image_list_image_is_visible (image_list, new_focused_item);

		if (visibility != GTH_VISIBILITY_FULL) {
			double offset;

			switch (visibility) {
			case GTH_VISIBILITY_NONE:
				offset = 0.5; 
				break;

			case GTH_VISIBILITY_PARTIAL_TOP:
				offset = 0.0; 
				break;

			case GTH_VISIBILITY_PARTIAL_BOTTOM:
				offset = 1.0; 
				break;

			case GTH_VISIBILITY_PARTIAL:
			case GTH_VISIBILITY_FULL:
				offset = -1.0;
				break;
			}

			if (offset > -1.0)
				gth_image_list_moveto (image_list, 
						       new_focused_item, 
						       offset);
		}
	}

	if (sel_change == GTH_SELCHANGE_SET) {
		real_unselect_all (image_list, NULL);
		gth_image_list_select_image (image_list, new_focused_item);

	} else if (sel_change == GTH_SELCHANGE_SET_RANGE) 
		select_range_with_keyboard (image_list, new_focused_item);

	gth_image_list_set_cursor (image_list, new_focused_item);

	return TRUE;
}


static gboolean
real_set_cursor_selection (GthImageList *image_list)
{
	GthImageListPrivate *priv = image_list->priv;
	GthImageListItem    *item;

	if (priv->focused_item == -1)
		return FALSE;

	item = g_list_nth (priv->image_list, priv->focused_item)->data;
	g_return_val_if_fail (item != NULL, FALSE);

	real_unselect_all (image_list, item);
	gth_image_list_select_image (image_list, priv->focused_item);
	priv->last_selected_pos = priv->select_pending_pos;
	priv->last_selected_item = priv->select_pending_item;

	return TRUE;
}


static gboolean
real_toggle_cursor_selection (GthImageList *image_list)
{
	GthImageListPrivate *priv = image_list->priv;
	GthImageListItem    *item;
	GList               *link;

	if (priv->focused_item == -1)
		return FALSE;

	link = g_list_nth (priv->image_list, priv->focused_item);
	g_return_val_if_fail (link != NULL, FALSE);
	item = link->data;

	if (item->selected)
		gth_image_list_unselect_image (image_list, priv->focused_item);
	else
		gth_image_list_select_image (image_list, priv->focused_item);

	return TRUE;
}


static void
real_set_cursor (GthImageList *image_list, 
		 int           pos)
{
	GthImageListPrivate *priv = image_list->priv;
	GthImageListItem    *old_item = NULL;
	GthImageListItem    *new_item;
	GList               *link;

	stop_dragging (image_list);

        if (priv->focused_item >= 0) {
		link = g_list_nth (priv->image_list, priv->focused_item);
		if (link != NULL) 
			old_item = link->data;
        }

	link = g_list_nth (priv->image_list, pos);
	g_return_if_fail (link != NULL);
	new_item = link->data;

	if (old_item != NULL)
		old_item->focused = FALSE;
	new_item->focused = TRUE;

	priv->focused_item = pos;

	if (old_item != NULL)
		queue_draw_item (image_list, old_item);
	queue_draw_item (image_list, new_item);
}


static gboolean
real_start_interactive_search (GthImageList *image_list)
{
	if (! GTK_WIDGET_HAS_FOCUS (image_list))
		return FALSE;

	if (image_list->priv->enable_search == FALSE)
		return FALSE;

	/* FIXME */

	return TRUE;
}


static void
add_move_binding (GtkBindingSet     *binding_set,
                  guint              keyval,
		  GthCursorMovement  dir)
{
        gtk_binding_entry_add_signal (binding_set, keyval, 0, 
				      "move_cursor", 2,
                                      G_TYPE_ENUM, dir,
				      G_TYPE_ENUM, GTH_SELCHANGE_SET);

        gtk_binding_entry_add_signal (binding_set, keyval, GDK_CONTROL_MASK, 
				      "move_cursor", 2,
                                      G_TYPE_ENUM, dir,
				      G_TYPE_ENUM, GTH_SELCHANGE_NONE);

	gtk_binding_entry_add_signal (binding_set, keyval, GDK_SHIFT_MASK, 
				      "move_cursor", 2,
                                      G_TYPE_ENUM, dir,
				      G_TYPE_ENUM, GTH_SELCHANGE_SET_RANGE);
}


static void
gth_image_list_set_property (GObject         *object,
			     guint            prop_id,
			     const GValue    *value,
			     GParamSpec      *pspec)
{
	GthImageList *image_list;

	image_list = GTH_IMAGE_LIST (object);

	switch (prop_id) {
	case PROP_HADJUSTMENT:
		gth_image_list_set_hadjustment (image_list, g_value_get_object (value));
		break;
	case PROP_VADJUSTMENT:
		gth_image_list_set_vadjustment (image_list, g_value_get_object (value));
		break;
	case PROP_ENABLE_SEARCH:
		gth_image_list_set_enable_search (image_list, g_value_get_boolean (value));
		break;
	default:
		break;
	}
}


static void
gth_image_list_get_property (GObject    *object,
			     guint       prop_id,
			     GValue     *value,
			     GParamSpec *pspec)
{
	GthImageList *image_list;

	image_list = GTH_IMAGE_LIST (object);
	
	switch (prop_id) {
	case PROP_HADJUSTMENT:
		g_value_set_object (value, image_list->priv->hadjustment);
		break;
	case PROP_VADJUSTMENT:
		g_value_set_object (value, image_list->priv->vadjustment);
		break;
	case PROP_ENABLE_SEARCH:
		g_value_set_boolean (value, image_list->priv->enable_search);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}


static void
gth_image_list_class_init (GthImageListClass *image_list_class)
{
	GObjectClass   *gobject_class;
	GtkWidgetClass *widget_class;
	GtkBindingSet  *binding_set;

	parent_class = g_type_class_peek_parent (image_list_class);
	gobject_class = (GObjectClass*) image_list_class;
	widget_class = (GtkWidgetClass*) image_list_class;

	/* Methods */

	gobject_class->finalize = gth_image_list_finalize;
	gobject_class->set_property = gth_image_list_set_property;
	gobject_class->get_property = gth_image_list_get_property;

	widget_class->map           = gth_image_list_map;
	widget_class->realize       = gth_image_list_realize;
	widget_class->unrealize     = gth_image_list_unrealize;
	widget_class->style_set     = gth_image_list_style_set;
	widget_class->size_allocate = gth_image_list_size_allocate;
	widget_class->expose_event  = gth_image_list_expose;	

	widget_class->focus_in_event    = gth_image_list_focus_in;
	widget_class->focus_out_event   = gth_image_list_focus_out;
	widget_class->key_press_event   = gth_image_list_key_press;
	widget_class->key_release_event = gth_image_list_key_release;
	widget_class->scroll_event      = gth_image_list_scroll_event;

	widget_class->button_press_event   = gth_image_list_button_press;
	widget_class->button_release_event = gth_image_list_button_release;
	widget_class->motion_notify_event  = gth_image_list_motion_notify;

	image_list_class->set_scroll_adjustments  = set_scroll_adjustments;
	image_list_class->cursor_changed          = real_set_cursor;
	image_list_class->move_cursor             = real_move_cursor;
	image_list_class->select_all              = key_binding_select_all;
	image_list_class->unselect_all            = key_binding_unselect_all;
        image_list_class->set_cursor_selection    = real_set_cursor_selection;
        image_list_class->toggle_cursor_selection = real_toggle_cursor_selection;
	image_list_class->start_interactive_search = real_start_interactive_search;

	/* Signals */

        widget_class->set_scroll_adjustments_signal =
		g_signal_new ("set_scroll_adjustments",
			      G_TYPE_FROM_CLASS (image_list_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthImageListClass, set_scroll_adjustments),
			      NULL, NULL,
			      gthumb_marshal_VOID__POINTER_POINTER,
			      G_TYPE_NONE, 
			      2, GTK_TYPE_ADJUSTMENT, GTK_TYPE_ADJUSTMENT);

	image_list_signals[SELECTION_CHANGED] =
		g_signal_new ("selection_changed",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (GthImageListClass, selection_changed),
			      NULL, NULL,
			      gthumb_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);
	image_list_signals[ITEM_ACTIVATED] =
		g_signal_new ("item_activated",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (GthImageListClass, item_activated),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__INT,
			      G_TYPE_NONE, 1,
			      G_TYPE_INT);
	image_list_signals[CURSOR_CHANGED] =
		g_signal_new ("cursor_changed",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (GthImageListClass, cursor_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__INT,
			      G_TYPE_NONE, 1,
			      G_TYPE_INT);
        image_list_signals[MOVE_CURSOR] =
		g_signal_new ("move_cursor",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			      G_STRUCT_OFFSET (GthImageListClass, move_cursor),
			      NULL, NULL,
			      gthumb_marshal_BOOLEAN__ENUM_ENUM,
			      G_TYPE_BOOLEAN, 2,
			      GTH_TYPE_CURSOR_MOVEMENT,
			      GTH_TYPE_SELECTION_CHANGE);
	image_list_signals[SELECT_ALL] =
		g_signal_new ("select_all",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			      G_STRUCT_OFFSET (GthImageListClass, select_all),
			      NULL, NULL,
			      gthumb_marshal_BOOL__VOID,
			      G_TYPE_BOOLEAN, 0);
	image_list_signals[SELECT_ALL] =
		g_signal_new ("unselect_all",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			      G_STRUCT_OFFSET (GthImageListClass, unselect_all),
			      NULL, NULL,
			      gthumb_marshal_BOOL__VOID,
			      G_TYPE_BOOLEAN, 0);
	image_list_signals[SET_CURSOR_SELECTION] =
		g_signal_new ("set_cursor_selection",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			      G_STRUCT_OFFSET (GthImageListClass, set_cursor_selection),
			      NULL, NULL,
			      gthumb_marshal_BOOL__VOID,
			      G_TYPE_BOOLEAN, 0);
	image_list_signals[TOGGLE_CURSOR_SELECTION] =
		g_signal_new ("toggle_cursor_selection",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			      G_STRUCT_OFFSET (GthImageListClass, toggle_cursor_selection),
			      NULL, NULL,
			      gthumb_marshal_BOOL__VOID,
			      G_TYPE_BOOLEAN, 0);
	image_list_signals[START_INTERACTIVE_SEARCH] =
		g_signal_new ("start_interactive_search",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			      G_STRUCT_OFFSET (GthImageListClass, start_interactive_search),
			      NULL, NULL,
			      gthumb_marshal_BOOL__VOID,
			      G_TYPE_BOOLEAN, 0);

	/* Properties */

	g_object_class_install_property (gobject_class,
					 PROP_HADJUSTMENT,
					 g_param_spec_object ("hadjustment",
							      "Horizontal Adjustment",
							      "Horizontal Adjustment for the widget",
							      GTK_TYPE_ADJUSTMENT,
							      G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class,
					 PROP_VADJUSTMENT,
					 g_param_spec_object ("vadjustment",
							      "Vertical Adjustment",
							      "Vertical Adjustment for the widget",
							      GTK_TYPE_ADJUSTMENT,
							      G_PARAM_READWRITE));
  
	g_object_class_install_property (gobject_class,
					 PROP_ENABLE_SEARCH,
					 g_param_spec_boolean ("enable_search",
							       "Enable Search",
							       "List allows user to search through images interactively",
							       TRUE,
							       G_PARAM_READWRITE));

	/* Style properties */

	gtk_widget_class_install_style_property (widget_class,
						 g_param_spec_int ("row_spacing",
								   "Row Spacing Size",
								   "Space between rows.",
								   0,
								   G_MAXINT,
								   DEFAULT_ROW_SPACING,
								   G_PARAM_READABLE));

	gtk_widget_class_install_style_property (widget_class,
						 g_param_spec_int ("column_spacing",
								   "Column Spacing Size",
								   "Space between columns.",
								   0,
								   G_MAXINT,
								   DEFAULT_COLUMN_SPACING,
								   G_PARAM_READABLE));

	/* Key bindings */

        binding_set = gtk_binding_set_by_class (image_list_class);

	add_move_binding (binding_set, GDK_Right, GTH_CURSOR_MOVE_RIGHT);
	add_move_binding (binding_set, GDK_KP_Right, GTH_CURSOR_MOVE_RIGHT);
	add_move_binding (binding_set, GDK_Left, GTH_CURSOR_MOVE_LEFT);
	add_move_binding (binding_set, GDK_KP_Left, GTH_CURSOR_MOVE_LEFT);
	add_move_binding (binding_set, GDK_Down, GTH_CURSOR_MOVE_DOWN);
	add_move_binding (binding_set, GDK_KP_Down, GTH_CURSOR_MOVE_DOWN);
	add_move_binding (binding_set, GDK_Up, GTH_CURSOR_MOVE_UP);
	add_move_binding (binding_set, GDK_KP_Up, GTH_CURSOR_MOVE_UP);
	add_move_binding (binding_set, GDK_Page_Up, GTH_CURSOR_MOVE_PAGE_UP);
	add_move_binding (binding_set, GDK_KP_Page_Up, GTH_CURSOR_MOVE_PAGE_UP);
	add_move_binding (binding_set, GDK_Page_Down, GTH_CURSOR_MOVE_PAGE_DOWN);
	add_move_binding (binding_set, GDK_KP_Page_Down, GTH_CURSOR_MOVE_PAGE_DOWN);
	add_move_binding (binding_set, GDK_Home, GTH_CURSOR_MOVE_BEGIN);
	add_move_binding (binding_set, GDK_KP_Home, GTH_CURSOR_MOVE_BEGIN);
	add_move_binding (binding_set, GDK_End, GTH_CURSOR_MOVE_END);
	add_move_binding (binding_set, GDK_KP_End, GTH_CURSOR_MOVE_END);

        gtk_binding_entry_add_signal (binding_set, GDK_space, 0,
				      "set_cursor_selection", 0);
        gtk_binding_entry_add_signal (binding_set, GDK_space, GDK_CONTROL_MASK,
				      "toggle_cursor_selection", 0);
        gtk_binding_entry_add_signal (binding_set, GDK_a, GDK_CONTROL_MASK,
				      "select_all", 0);
        gtk_binding_entry_add_signal (binding_set, GDK_slash, GDK_CONTROL_MASK,
				      "select_all", 0);
        gtk_binding_entry_add_signal (binding_set, GDK_A, GDK_SHIFT_MASK | GDK_CONTROL_MASK,
				      "unselect_all", 0);
        gtk_binding_entry_add_signal (binding_set, GDK_backslash, GDK_CONTROL_MASK,
				      "unselect_all", 0);
        gtk_binding_entry_add_signal (binding_set, GDK_f, GDK_CONTROL_MASK,
				      "start_interactive_search", 0);
        gtk_binding_entry_add_signal (binding_set, GDK_F, GDK_CONTROL_MASK,
				      "start_interactive_search", 0);
}


static void
gth_image_list_init (GthImageList *image_list)
{
	GthImageListPrivate *priv;

	GTK_WIDGET_SET_FLAGS (image_list, GTK_CAN_FOCUS);

	priv = image_list->priv = g_new0 (GthImageListPrivate, 1);
	
	priv->focused_item = -1;
	priv->old_focused_item = -1;
	
	priv->selection_mode = GTK_SELECTION_MULTIPLE;
	priv->last_selected_pos = -1;

	priv->view_mode = GTH_VIEW_MODE_VOID;

	priv->row_spacing = DEFAULT_ROW_SPACING;
	priv->col_spacing = DEFAULT_COLUMN_SPACING;
	priv->text_spacing = DEFAULT_TEXT_SPACING;
	priv->image_border = DEFAULT_IMAGE_BORDER;

	priv->target_list = gtk_target_list_new (target_table, G_N_ELEMENTS (target_table));

	priv->enable_search = TRUE;
}


GType
gth_image_list_get_type (void)
{
	static guint type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (GthImageListClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_image_list_class_init,
			NULL,
			NULL,
			sizeof (GthImageList),
			0,
			(GInstanceInitFunc) gth_image_list_init
                };

		type = g_type_register_static (GTK_TYPE_CONTAINER,
					       "GthImageList",
					       &type_info,
                                               0);
        }

        return type;
}


GtkWidget *
gth_image_list_new (guint image_width)
{
	GthImageList *image_list;

	image_list = (GthImageList*) g_object_new (GTH_TYPE_IMAGE_LIST, NULL);
	image_list->priv->max_item_width = image_width;

	return (GtkWidget*) image_list;
}


static void
sync_selection (GthImageList *image_list, 
		int           pos, 
		SyncType      type)
{
	GList *scan;

	for (scan = image_list->priv->selection; scan; scan = scan->next) {
		if (GPOINTER_TO_INT (scan->data) >= pos) {
			int i = GPOINTER_TO_INT (scan->data);

			switch (type) {
			case SYNC_INSERT:
				scan->data = GINT_TO_POINTER (i + 1);
				break;

			case SYNC_REMOVE:
				scan->data = GINT_TO_POINTER (i - 1);
				break;

			default:
				g_assert_not_reached ();
				break;
			}
		}
	}
}


void
gth_image_list_freeze (GthImageList *image_list)
{
	g_return_if_fail (GTH_IS_IMAGE_LIST (image_list));
	image_list->priv->frozen++;
}


void
gth_image_list_thaw (GthImageList *image_list)
{
	g_return_if_fail (GTH_IS_IMAGE_LIST (image_list));
	g_return_if_fail (image_list->priv->frozen > 0);

	image_list->priv->frozen--;

	if (image_list->priv->frozen == 0) {
		if (image_list->priv->dirty) {
			layout_all_images (image_list);
			keep_focus_consistent (image_list);
		}
	}
}


gboolean
gth_image_list_is_frozen (GthImageList *image_list)
{
	g_return_val_if_fail (GTH_IS_IMAGE_LIST (image_list), FALSE);
	return image_list->priv->frozen > 0;
}


static int
image_list_append_item (GthImageList     *image_list, 
			GthImageListItem *item)
{
	GthImageListPrivate *priv = image_list->priv;
	int                  pos;

	pos = priv->images++;
	priv->image_list = g_list_append (priv->image_list, item);

	if (! priv->frozen) 
		layout_from_line (image_list, pos / gth_image_list_get_items_per_line (image_list));
	else
		priv->dirty = TRUE;

	return priv->images - 1;
}


static int
image_list_insert_item (GthImageList     *image_list, 
			GthImageListItem *item,
			int               pos)
{
	GthImageListPrivate *priv = image_list->priv;

	if (priv->sorted) 
		priv->image_list = g_list_insert_sorted (priv->image_list, 
							 item, 
							 priv->compare);
	else {
		if (pos == priv->images) 
			return image_list_append_item (image_list, item);
		priv->image_list = g_list_insert (priv->image_list, item, pos);
	}

	priv->images++;

	pos = g_list_index (priv->image_list, item);

	if (! priv->frozen) 
		layout_from_line (image_list, pos / gth_image_list_get_items_per_line (image_list));
	else
		priv->dirty = TRUE;

	sync_selection (image_list, pos, SYNC_INSERT);

	return pos;
}


static char *
truncate_comment_if_needed (GthImageList  *image_list,
                            const char    *comment)
{
	char *result;
	int   max_len, approx_char_width = 6; /* FIXME */
	int   comment_len;

	if (comment == NULL)
		return NULL;
	
        if (*comment == 0)
		return g_strdup ("");
	max_len = (image_list->priv->max_item_width / approx_char_width) * COMMENT_MAX_LINES;
	comment_len = g_utf8_strlen (comment, -1);
	
	if (comment_len > max_len) {
		char *comment_n;
		
		comment_n = _g_utf8_strndup (comment, max_len);
		result = g_strconcat (comment_n, ETC, NULL);
		
		g_free (comment_n);
	} else
		result = g_strdup (comment);
	
	return result;
}


void
gth_image_list_insert (GthImageList *image_list, 
		       int           pos, 
		       GdkPixbuf    *pixbuf, 
		       const char   *text,
		       const char   *comment)
{
	GthImageListItem *item;
	char             *comment2;

	g_return_if_fail (image_list != NULL);
	g_return_if_fail (pixbuf != NULL);
	g_return_if_fail ((pos >= 0) && (pos <= image_list->priv->images));

	comment2 = truncate_comment_if_needed (image_list, comment);
	item = gth_image_list_item_new (image_list, pixbuf, text, comment2);
	g_free (comment2);

	image_list_insert_item (image_list, item, pos);
}


int
gth_image_list_append_with_data (GthImageList  *image_list, 
				 GdkPixbuf     *pixbuf, 
				 const char    *text,
				 const char    *comment,
				 gpointer       data)
{
	GthImageListItem *item;
	char             *comment2;

	g_return_val_if_fail (image_list != NULL, -1);
	g_return_val_if_fail (pixbuf != NULL, -1);

	comment2 = truncate_comment_if_needed (image_list, comment);
	item = gth_image_list_item_new (image_list, pixbuf, text, comment2);
	g_free (comment2);

	/**/

	if (data != NULL) {
		if ((item->destroy != NULL) && (item->data != NULL))
			(item->destroy) (item->data);
		item->data = data;
		item->destroy = NULL;
	}

	/**/

	if (image_list->priv->sorted)
		return image_list_insert_item (image_list, item, -1);
	else
		return image_list_append_item (image_list, item);
}


int
gth_image_list_append (GthImageList  *image_list, 
		       GdkPixbuf     *pixbuf, 
		       const char    *text,
		       const char    *comment)
{
	return gth_image_list_append_with_data (image_list, pixbuf, text, comment, NULL);
}


void
gth_image_list_remove (GthImageList *image_list, 
		       int           pos)
{
	GthImageListPrivate *priv = image_list->priv;
	GList               *list;
	GthImageListItem    *item;
	int                  was_selected = FALSE;

	g_return_if_fail (priv != NULL);
	g_return_if_fail ((pos >= 0) && (pos < priv->images));

	list = g_list_nth (priv->image_list, pos);
	item = list->data;

	if (priv->focused_item == pos)
		priv->focused_item = -1;

	if (item->selected) {
		was_selected = TRUE;

		switch (priv->selection_mode) {
		case GTK_SELECTION_SINGLE:
		case GTK_SELECTION_MULTIPLE:
			 gth_image_list_unselect_image (image_list, pos);
			break;

		default:
			break;
		}
	}

	priv->image_list = g_list_remove_link (priv->image_list, list);
	g_list_free_1 (list);
	priv->images--;

	sync_selection (image_list, pos, SYNC_REMOVE);

	if (priv->images >= priv->last_selected_pos)
		priv->last_selected_pos = -1;

	if (priv->last_selected_item == item)
		priv->last_selected_item = NULL;

	/**/

	gth_image_list_item_unref (item);

	if (! priv->frozen) {
		layout_from_line (image_list, pos / gth_image_list_get_items_per_line (image_list));
		keep_focus_consistent (image_list);
	} else 
		priv->dirty = TRUE;
}


void
gth_image_list_clear (GthImageList *image_list)
{
	GthImageListPrivate *priv = image_list->priv;

	g_return_if_fail (image_list != NULL);

	if (priv->image_list != NULL) {
		GList *scan;

		for (scan = priv->image_list; scan; scan = scan->next)
			gth_image_list_item_unref (scan->data);
		g_list_free (priv->image_list);
		priv->image_list = NULL;
	}

	free_line_info (image_list);

	if (priv->selection != NULL) {
		g_list_free (priv->selection);
		priv->selection = NULL;
	}

	priv->images = 0;
	priv->last_selected_pos = -1;
	priv->last_selected_item = NULL;

	gtk_adjustment_set_value (priv->hadjustment, 0);
	gtk_adjustment_set_value (priv->vadjustment, 0);

	layout_all_images (image_list);
	keep_focus_consistent (image_list);
}


void
gth_image_list_set_image_pixbuf (GthImageList  *image_list,
				 int            pos,
				 GdkPixbuf     *pixbuf)
{
	GthImageListPrivate *priv;
	GthImageListItem    *item;
	int                  x_offset, y_offset;

	g_return_if_fail (image_list != NULL);

	priv = image_list->priv;

	g_return_if_fail ((pos >= 0) && (pos < priv->images));
	g_return_if_fail (pixbuf != NULL);
	
	item = g_list_nth (priv->image_list, pos)->data;
	g_return_if_fail (item != NULL);

	gth_image_list_item_set_pixbuf (image_list, item, pixbuf);

	/* update image area */

	if (priv->max_item_width > item->image_area.height)
		y_offset = (priv->max_item_width - item->image_area.height) / 2;
	else
		y_offset = 0;
	x_offset = (priv->max_item_width - item->image_area.width) / 2;
	
	item->image_area.x = item->slide_area.x + x_offset + 1;
	item->image_area.y = item->slide_area.y + y_offset + 1;

	/**/

	queue_draw_item (image_list, item);
}


void
gth_image_list_set_image_text (GthImageList  *image_list,
			       int            pos,
			       const char    *label)
{
	GthImageListItem *item;

	g_return_if_fail (image_list != NULL);
	g_return_if_fail ((pos >= 0) && (pos < image_list->priv->images));
	g_return_if_fail (label != NULL);
	
	item = g_list_nth (image_list->priv->image_list, pos)->data;
	g_return_if_fail (item != NULL);

	g_free (item->label);
	item->label = NULL;
	if (label != NULL)
		item->label = g_strdup (label);

	item->label_area.width = -1;
	item->label_area.height = -1;

	if (image_list->priv->frozen) {
		image_list->priv->dirty = TRUE;
		return;
	}

	layout_from_line (image_list, pos / gth_image_list_get_items_per_line (image_list));
}


void
gth_image_list_set_image_comment (GthImageList  *image_list,
				  int            pos,
				  const char    *comment)
{
	GthImageListItem *item;

	g_return_if_fail (image_list != NULL);
	g_return_if_fail ((pos >= 0) && (pos < image_list->priv->images));
	g_return_if_fail (comment != NULL);
	
	item = g_list_nth (image_list->priv->image_list, pos)->data;
	g_return_if_fail (item != NULL);

	g_free (item->comment);
	item->comment = NULL;
	if (comment != NULL)
		item->comment = truncate_comment_if_needed (image_list, comment);
	item->comment_area.width = -1;
	item->comment_area.height = -1;

	if (image_list->priv->frozen) {
		image_list->priv->dirty = TRUE;
		return;
	}

	layout_from_line (image_list, pos / gth_image_list_get_items_per_line (image_list));
}


const char *
gth_image_list_get_image_text (GthImageList *image_list,
			       int           pos)
{
	GthImageListItem *item;

	g_return_val_if_fail (image_list != NULL, NULL);
	g_return_val_if_fail ((pos >= 0) && (pos < image_list->priv->images), NULL);
	
	item = g_list_nth (image_list->priv->image_list, pos)->data;
	g_return_val_if_fail (item != NULL, NULL);

	return item->label;
}


const char *
gth_image_list_get_image_comment (GthImageList *image_list,
				  int           pos)
{
	GthImageListItem *item;

	g_return_val_if_fail (image_list != NULL, NULL);
	g_return_val_if_fail ((pos >= 0) && (pos < image_list->priv->images), NULL);
	
	item = g_list_nth (image_list->priv->image_list, pos)->data;
	g_return_val_if_fail (item != NULL, NULL);

	return item->comment;
}


int
gth_image_list_get_images (GthImageList  *image_list)
{
	g_return_val_if_fail (image_list != NULL, 0);
	return image_list->priv->images;
}


GList *
gth_image_list_get_list (GthImageList  *image_list)
{
	GList *scan;
	GList *list = NULL;

	g_return_val_if_fail (image_list != NULL, NULL);

	for (scan = image_list->priv->image_list; scan; scan = scan->next) {
		GthImageListItem *item = scan->data;
		if (item->data != NULL)
			list = g_list_prepend (list, item->data);
	}

	return g_list_reverse (list);
}


GList *
gth_image_list_get_selection (GthImageList  *image_list)
{
	GList *scan;
	GList *list = NULL;

	g_return_val_if_fail (image_list != NULL, NULL);

	for (scan = image_list->priv->image_list; scan; scan = scan->next) {
		GthImageListItem *item = scan->data;
		if (item->selected && (item->data != NULL)) {
			FileData *fdata = item->data;
			file_data_ref (fdata);
			list = g_list_prepend (list, fdata);
		}
	}

	return g_list_reverse (list);
}


void
gth_image_list_set_selection_mode (GthImageList     *image_list,
				   GtkSelectionMode  mode)
{
	g_return_if_fail (image_list != NULL);

	image_list->priv->selection_mode = mode;
	image_list->priv->last_selected_pos = -1;
	image_list->priv->last_selected_item = NULL;
}


void
gth_image_list_set_image_width (GthImageList *image_list,
				int           width)
{
	GthImageListPrivate *priv = image_list->priv;

	g_return_if_fail (GTH_IS_IMAGE_LIST (image_list));

	priv->max_item_width = width;
	priv->update_width = TRUE;

	if (priv->frozen) {
		priv->dirty = TRUE;
		return;
	}

	layout_all_images (image_list);
}


void
gth_image_list_set_image_data_full (GthImageList    *image_list,
				    int              pos, 
				    gpointer         data,
				    GtkDestroyNotify destroy)
{
	GthImageListItem *item;

	g_return_if_fail (GTH_IS_IMAGE_LIST (image_list));
	g_return_if_fail ((pos >= 0) && (pos < image_list->priv->images));

	item = g_list_nth (image_list->priv->image_list, pos)->data;
	g_return_if_fail (item != NULL);
	
	if ((item->destroy != NULL) && (item->data != NULL))
		(item->destroy) (item->data);

	item->data = data;
	item->destroy = destroy;
}


void
gth_image_list_set_image_data (GthImageList    *image_list,
			       int              pos, 
			       gpointer         data)
{
	gth_image_list_set_image_data_full (image_list, pos, data, NULL);
}


int
gth_image_list_find_image_from_data (GthImageList *image_list,
				     gpointer      data)
{
	GList *list;
	int    n;

	g_return_val_if_fail (GTH_IS_IMAGE_LIST (image_list), -1);

	for (n = 0, list = image_list->priv->image_list; list; n++, list = list->next) {
		GthImageListItem *item = list->data;
		if (item->data == data)
			return n;
	}

	return -1;
}


gpointer
gth_image_list_get_image_data (GthImageList    *image_list,
			       int              pos)
{
	GthImageListItem *item;
	FileData         *fdata;

	g_return_val_if_fail (GTH_IS_IMAGE_LIST (image_list), NULL);
	g_return_val_if_fail ((pos >= 0) && (pos < image_list->priv->images), NULL);

	item = g_list_nth (image_list->priv->image_list, pos)->data;
	fdata = item->data;
	file_data_ref (fdata);

	return fdata;
}


void
gth_image_list_enable_thumbs (GthImageList *image_list,
			      gboolean      enable_thumbs)
{
	image_list->priv->enable_thumbs = enable_thumbs;
	queue_draw (image_list);
}


void
gth_image_list_set_view_mode (GthImageList *image_list,
			      GthViewMode   mode)
{
	g_return_if_fail (GTH_IS_IMAGE_LIST (image_list));
	image_list->priv->view_mode = mode;
	image_list->priv->update_width = TRUE;
	layout_all_images (image_list);
}


GthViewMode
gth_image_list_get_view_mode (GthImageList *image_list)
{
	g_return_val_if_fail (GTH_IS_IMAGE_LIST (image_list), 0);
	return image_list->priv->view_mode;
}


void
gth_image_list_moveto (GthImageList *image_list,
		       int           pos, 
		       double        yalign)
{
	GthImageListPrivate *priv = image_list->priv;
	GthImageListLine *line;
	GList *l;
	int    i, y, uh, line_n;
	float  value;

	g_return_if_fail (image_list != NULL);
	g_return_if_fail ((pos >= 0) && (pos < priv->images));
	g_return_if_fail ((yalign >= 0.0) && (yalign <= 1.0));

	if (priv->lines == NULL)
		return;

	line_n = pos / gth_image_list_get_items_per_line (image_list);

	y = priv->row_spacing;
	for (i = 0, l = priv->lines; l && i < line_n; l = l->next, i++) {
		line = l->data;
		y += IMAGE_LINE_HEIGHT (image_list, line);
	}

	if (l == NULL)
		return;

	line = l->data;

	uh = GTK_WIDGET (image_list)->allocation.height - IMAGE_LINE_HEIGHT (image_list, line);
	value = CLAMP ((y 
			- uh * yalign 
			- (1.0 - yalign) * priv->row_spacing), 
		       0.0, 
		       priv->vadjustment->upper - priv->vadjustment->page_size);

	gtk_adjustment_set_value (priv->vadjustment, value);
}


/**
 * gth_image_list_image_is_visible:
 * @gil: An image list.
 * @pos: Index of an image.
 *
 * Returns whether the image at the index specified by @pos is visible.  This
 * will be %GTK_VISIBILITY_NONE if the image is not visible at all,
 * %GTK_VISIBILITY_PARTIAL if the image is at least partially shown, and
 * %GTK_VISIBILITY_FULL if the image is fully visible.
 */
GthVisibility
gth_image_list_image_is_visible (GthImageList *image_list,
				 int           pos)
{
	GthImageListPrivate *priv = image_list->priv;
	GthImageListLine *line;
	GList     *l;
	int        line_n, i;
	int        image_top, image_bottom;
	int        window_top, window_bottom;

	g_return_val_if_fail (image_list != NULL, GTH_VISIBILITY_NONE);
	g_return_val_if_fail ((pos >= 0) && (pos < priv->images), GTH_VISIBILITY_NONE);

	if (priv->lines == NULL)
		return GTH_VISIBILITY_NONE;

	line_n = pos / gth_image_list_get_items_per_line (image_list);
	image_top = priv->row_spacing;
	for (i = 0, l = priv->lines; l && i < line_n; l = l->next, i++) {
		line = l->data;
		image_top += IMAGE_LINE_HEIGHT (image_list, line);
	}

	if (l == NULL)
		return GTH_VISIBILITY_NONE;

	line = l->data;
	image_bottom = image_top + IMAGE_LINE_HEIGHT (image_list, line);

	window_top = priv->vadjustment->value;
	window_bottom = priv->vadjustment->value + GTK_WIDGET (image_list)->allocation.height;

	if (image_bottom < window_top)
		return GTH_VISIBILITY_NONE;

	if (image_top > window_bottom)
		return GTH_VISIBILITY_NONE;

	if ((image_top >= window_top) && (image_bottom <= window_bottom))
		return GTH_VISIBILITY_FULL;

	if ((image_top < window_top) && (image_bottom >= window_top))
		return GTH_VISIBILITY_PARTIAL_TOP;

	if ((image_top <= window_bottom) && (image_bottom > window_bottom))
		return GTH_VISIBILITY_PARTIAL_BOTTOM;

	return GTH_VISIBILITY_PARTIAL;
}


static gboolean
_gdk_rectangle_point_in (GdkRectangle *rect,
			 int           x,
			 int           y)
{
	return ((x >= rect->x)
		&& (y >= rect->y)
		&& (x <= rect->x + rect->width)
		&& (y <= rect->y + rect->height));
}


int
gth_image_list_get_image_at (GthImageList *image_list, 
			     int           x, 
			     int           y)
{
	GthImageListPrivate *priv = image_list->priv;
	GList               *scan;
	int                  n;

	for (n = 0, scan = priv->image_list; scan; scan = scan->next, n++) {
		GthImageListItem *item = scan->data;
		gboolean          view_text, view_comment;

		if ((x >= item->slide_area.x)
		    && (y >= item->slide_area.y)
		    && (x <= item->slide_area.x + priv->max_item_width)
		    && (y <= item->slide_area.y + priv->max_item_width)) {
			return n;
		}

		item_get_view_mode (image_list, item, &view_text, &view_comment);

		if (view_text && _gdk_rectangle_point_in (&item->label_area, x, y)) {
			return n;
		}

		if (view_comment && _gdk_rectangle_point_in (&item->comment_area, x, y)) {
			return n;
		}
	}

	return -1;
}


static int
default_compare (gconstpointer  ptr1,
                 gconstpointer  ptr2)
{
	return 0;
}


void
gth_image_list_sorted (GthImageList *image_list,
		       GCompareFunc  cmp_func,
		       GtkSortType   sort_type)
{
	GthImageListPrivate *priv;

	g_return_if_fail (GTH_IS_IMAGE_LIST (image_list));

	priv = image_list->priv;

	priv->sorted = TRUE;
	priv->compare = (cmp_func) ? cmp_func : default_compare;
	priv->sort_type = sort_type;

	priv->image_list = g_list_sort (priv->image_list, priv->compare);
	if (priv->sort_type == GTK_SORT_DESCENDING)
		priv->image_list = g_list_reverse (priv->image_list);

	if (! priv->frozen) 
		layout_all_images (image_list);
	else
		priv->dirty = TRUE;
}


void
gth_image_list_unsorted (GthImageList *image_list)
{
	g_return_if_fail (GTH_IS_IMAGE_LIST (image_list));
	image_list->priv->sorted = FALSE;
}


void
gth_image_list_image_activated (GthImageList *image_list, 
				int           pos)
{
	g_return_if_fail (GTH_IS_IMAGE_LIST (image_list));
	g_return_if_fail ((pos >= 0) && (pos < image_list->priv->images));

	g_signal_emit (image_list, image_list_signals[ITEM_ACTIVATED], 0, pos);
}


void
gth_image_list_set_cursor (GthImageList *image_list, 
			   int           pos)
{
	g_return_if_fail (GTH_IS_IMAGE_LIST (image_list));
	g_return_if_fail ((pos >= 0) && (pos < image_list->priv->images));

	g_signal_emit (image_list, image_list_signals[CURSOR_CHANGED], 0, pos);
}


int
gth_image_list_get_cursor (GthImageList *image_list)
{
	g_return_val_if_fail (GTH_IS_IMAGE_LIST (image_list), -1);

	if (! GTK_WIDGET_HAS_FOCUS (image_list))
		return -1;
	else
		return image_list->priv->focused_item;
}


void
gth_image_list_set_enable_search (GthImageList *image_list,
				  gboolean      enable_search)
{
	g_return_if_fail (GTH_IS_IMAGE_LIST (image_list));
	
	enable_search = !!enable_search;
	
	if (image_list->priv->enable_search != enable_search) {
		image_list->priv->enable_search = enable_search;
		g_object_notify (G_OBJECT (image_list), "enable_search");
	}
}


gboolean
gth_image_list_get_enable_search (GthImageList *image_list)
{
	g_return_val_if_fail (GTH_IS_IMAGE_LIST (image_list), FALSE);
	return image_list->priv->enable_search;
}

