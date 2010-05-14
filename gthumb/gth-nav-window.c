/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2006-2009 Free Software Foundation, Inc.
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

#include <config.h>
#include <math.h>
#include <gdk/gdkkeysyms.h>
#include "gth-nav-window.h"
#include "gth-image-viewer.h"
#include "gtk-utils.h"
#include "pixbuf-utils.h"
#include "icons/nav_button.xpm"
#include "pixbuf-utils.h"

#define GTH_NAV_WINDOW_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GTH_TYPE_NAV_WINDOW, GthNavWindowPrivate))

#define PEN_WIDTH           3       /* Square border width. */
#define B                   4       /* Window border width. */
#define B2                  8       /* Window border width * 2. */
#define NAV_WIN_MAX_WIDTH   112     /* Max window size. */
#define NAV_WIN_MAX_HEIGHT  112


struct _GthNavWindowPrivate {
	GthImageViewer *viewer;
	GtkWidget      *viewer_vscr;
	GtkWidget      *viewer_hscr;
	GtkWidget      *viewer_nav_event_box;
	gboolean        scrollbars_visible;
};


static GtkHBoxClass *parent_class = NULL;


static void
gth_nav_window_class_init (GthNavWindowClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	g_type_class_add_private (class, sizeof (GthNavWindowPrivate));

	object_class = G_OBJECT_CLASS (class);
}


static void
gth_nav_window_init (GthNavWindow *nav_window)
{
	nav_window->priv = GTH_NAV_WINDOW_GET_PRIVATE (nav_window);
	nav_window->priv->scrollbars_visible = TRUE;
}


GType
gth_nav_window_get_type ()
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (GthNavWindowClass),
                        NULL,
                        NULL,
                        (GClassInitFunc) gth_nav_window_class_init,
                        NULL,
                        NULL,
                        sizeof (GthNavWindow),
                        0,
                        (GInstanceInitFunc) gth_nav_window_init
                };

                type = g_type_register_static (GTK_TYPE_HBOX,
                                               "GthNavWindow",
                                               &type_info,
                                               0);
        }

        return type;
}


static gboolean
size_changed_cb (GtkWidget    *widget,
		 GthNavWindow *nav_window)
{
	GtkAdjustment *vadj, *hadj;
	gboolean       hide_vscr, hide_hscr;

	gth_image_viewer_get_adjustments (nav_window->priv->viewer, &hadj, &vadj);

	g_return_val_if_fail (hadj != NULL, FALSE);
	g_return_val_if_fail (vadj != NULL, FALSE);

	hide_vscr = (vadj->page_size == 0) || (vadj->upper <= vadj->page_size);
	hide_hscr = (hadj->page_size == 0) || (hadj->upper <= hadj->page_size);

	if (! nav_window->priv->scrollbars_visible || (hide_vscr && hide_hscr)) {
		gtk_widget_hide (nav_window->priv->viewer_vscr);
		gtk_widget_hide (nav_window->priv->viewer_hscr);
		gtk_widget_hide (nav_window->priv->viewer_nav_event_box);
	}
	else {
		gtk_widget_show (nav_window->priv->viewer_vscr);
		gtk_widget_show (nav_window->priv->viewer_hscr);
		gtk_widget_show (nav_window->priv->viewer_nav_event_box);
	}

	return TRUE;
}


/* -- nav_button_clicked_cb -- */


typedef struct {
	GthImageViewer *viewer;
	int             x_root, y_root;
	GtkWidget      *popup_win;
	GtkWidget      *preview;
	GdkPixbuf      *pixbuf;
	GdkGC          *gc;
	int             image_width, image_height;
	int             window_max_width, window_max_height;
	int             popup_x, popup_y, popup_width, popup_height;
	int             sqr_x, sqr_y, sqr_width, sqr_height;
	double          factor;
	double          sqr_x_d, sqr_y_d;
} NavWindow;


static void
nav_window_draw_sqr (NavWindow *nav_win,
		     gboolean   undraw,
		     int        x,
		     int        y)
{
	if ((nav_win->sqr_x == x) && (nav_win->sqr_y == y) && undraw)
		return;

	if ((nav_win->sqr_x == 0)
	    && (nav_win->sqr_y == 0)
	    && (nav_win->sqr_width == nav_win->popup_width)
	    && (nav_win->sqr_height == nav_win->popup_height))
	{
		return;
	}

	if (undraw) {
		gdk_draw_rectangle (nav_win->preview->window,
				    nav_win->gc, FALSE,
				    nav_win->sqr_x + 1,
				    nav_win->sqr_y + 1,
				    nav_win->sqr_width - PEN_WIDTH,
				    nav_win->sqr_height - PEN_WIDTH);
	}

	gdk_draw_rectangle (nav_win->preview->window,
			    nav_win->gc, FALSE,
			    x + 1,
			    y + 1,
			    nav_win->sqr_width - PEN_WIDTH,
			    nav_win->sqr_height - PEN_WIDTH);

	nav_win->sqr_x = x;
	nav_win->sqr_y = y;
}


static void
get_sqr_origin_as_double (NavWindow *nav_win,
			  int        mx,
			  int        my,
			  double    *x,
			  double    *y)
{
	*x = MIN (mx - B, nav_win->window_max_width);
	*y = MIN (my - B, nav_win->window_max_height);

	if (*x - nav_win->sqr_width / 2.0 < 0.0)
		*x = nav_win->sqr_width / 2.0;

	if (*y - nav_win->sqr_height / 2.0 < 0.0)
		*y = nav_win->sqr_height / 2.0;

	if (*x + nav_win->sqr_width / 2.0 > nav_win->popup_width - 0)
		*x = nav_win->popup_width - 0 - nav_win->sqr_width / 2.0;

	if (*y + nav_win->sqr_height / 2.0 > nav_win->popup_height - 0)
		*y = nav_win->popup_height - 0 - nav_win->sqr_height / 2.0;

	*x = *x - nav_win->sqr_width / 2.0;
	*y = *y - nav_win->sqr_height / 2.0;
}


static void
update_view (NavWindow *nav_win)
{
	GdkPixbuf  *image_pixbuf;
	int         popup_x, popup_y;
	int         popup_width, popup_height;
	int         x_offset, y_offset;
	int         w, h;
	double      factor;
	int         gdk_width, gdk_height;

	w = nav_win->image_width * gth_image_viewer_get_zoom (nav_win->viewer);
	h = nav_win->image_height * gth_image_viewer_get_zoom (nav_win->viewer);

	nav_win->window_max_width = MIN (w, NAV_WIN_MAX_WIDTH);
	nav_win->window_max_height = MIN (w, NAV_WIN_MAX_HEIGHT);

	factor = MIN ((double) (nav_win->window_max_width) / w,
		      (double) (nav_win->window_max_height) / h);
	nav_win->factor = factor;

	gdk_width = GTK_WIDGET (nav_win->viewer)->allocation.width - GTH_IMAGE_VIEWER_FRAME_BORDER2;
	gdk_height = GTK_WIDGET (nav_win->viewer)->allocation.height - GTH_IMAGE_VIEWER_FRAME_BORDER2;

	/* Popup window size. */

	popup_width  = MAX ((int) floor (factor * w + 0.5), 1);
	popup_height = MAX ((int) floor (factor * h + 0.5), 1);

	image_pixbuf = gth_image_viewer_get_current_pixbuf (nav_win->viewer);
	g_return_if_fail (image_pixbuf != NULL);

	if (nav_win->pixbuf != NULL)
		g_object_unref (nav_win->pixbuf);

	nav_win->pixbuf = _gdk_pixbuf_scale_simple_safe (image_pixbuf,
						         popup_width,
							 popup_height,
							 GDK_INTERP_TILES);

	/* The square. */

	nav_win->sqr_width = gdk_width * factor;
	nav_win->sqr_width = MAX (nav_win->sqr_width, B);
	nav_win->sqr_width = MIN (nav_win->sqr_width, popup_width);

	nav_win->sqr_height = gdk_height * factor;
	nav_win->sqr_height = MAX (nav_win->sqr_height, B);
	nav_win->sqr_height = MIN (nav_win->sqr_height, popup_height);

	gth_image_viewer_get_scroll_offset (nav_win->viewer, &x_offset, &y_offset);
	nav_win->sqr_x = x_offset * factor;
	nav_win->sqr_y = y_offset * factor;

	/* Popup window position. */

	popup_x = MIN ((int) nav_win->x_root - nav_win->sqr_x
		       - B
		       - nav_win->sqr_width / 2,
		       gdk_screen_width () - popup_width - B2);
	popup_y = MIN ((int) nav_win->y_root - nav_win->sqr_y
		       - B
		       - nav_win->sqr_height / 2,
		       gdk_screen_height () - popup_height - B2);

	nav_win->popup_x = popup_x;
	nav_win->popup_y = popup_y;
	nav_win->popup_width = popup_width;
	nav_win->popup_height = popup_height;
}


static gboolean
nav_window_expose (GtkWidget      *widget,
		   GdkEventExpose *event,
		   NavWindow      *nav_win)
{
	if (nav_win->pixbuf == NULL)
		return FALSE;

	gdk_draw_pixbuf (nav_win->preview->window,
			 gdk_pixbuf_get_has_alpha (nav_win->pixbuf) ? NULL : nav_win->preview->style->white_gc,
			 nav_win->pixbuf,
			 0, 0,
			 0, 0,
			 nav_win->popup_width,
			 nav_win->popup_height,
			 GDK_RGB_DITHER_MAX,
			 0, 0);

	nav_window_draw_sqr (nav_win, FALSE,
			     nav_win->sqr_x,
			     nav_win->sqr_y);

	return TRUE;
}


static int
nav_window_events (GtkWidget *widget,
		   GdkEvent  *event,
		   gpointer   data)
{
	NavWindow       *nav_win = data;
	GdkModifierType  mask;
	int              mx, my;
	double           x, y;
	GthImageViewer  *viewer = nav_win->viewer;

	switch (event->type) {
	case GDK_BUTTON_RELEASE:
		/* Release keyboard focus. */
		gdk_keyboard_ungrab (GDK_CURRENT_TIME);
		gtk_grab_remove (nav_win->popup_win);

		g_object_unref (nav_win->gc);
		gtk_widget_destroy (nav_win->popup_win);
		g_object_unref (nav_win->pixbuf);
		g_free (nav_win);

		return TRUE;

	case GDK_MOTION_NOTIFY:
		gdk_window_get_pointer (widget->window, &mx, &my, &mask);
		get_sqr_origin_as_double (nav_win, mx, my, &x, &y);

		mx = (int) x;
		my = (int) y;
		nav_window_draw_sqr (nav_win, TRUE, mx, my);

		mx = (int) (x / nav_win->factor);
		my = (int) (y / nav_win->factor);
		gth_image_viewer_scroll_to (viewer, mx, my);

		return TRUE;

	case GDK_KEY_PRESS:
		switch (event->key.keyval) {
		case GDK_plus:
		case GDK_minus:
		case GDK_1:
			nav_window_draw_sqr (nav_win, FALSE,
					     nav_win->sqr_x,
					     nav_win->sqr_y);
			switch (event->key.keyval) {
			case GDK_plus:
				gth_image_viewer_zoom_in (viewer);
				break;
			case GDK_minus:
				gth_image_viewer_zoom_out (viewer);
				break;
			case GDK_1:
				gth_image_viewer_set_zoom (viewer, 1.0);
				break;
			}

			update_view (nav_win);

			nav_win->sqr_x = MAX (nav_win->sqr_x, 0);
			nav_win->sqr_x = MIN (nav_win->sqr_x, nav_win->popup_width - nav_win->sqr_width);
			nav_win->sqr_y = MAX (nav_win->sqr_y, 0);
			nav_win->sqr_y = MIN (nav_win->sqr_y, nav_win->popup_height - nav_win->sqr_height);

			nav_window_draw_sqr (nav_win, FALSE,
					     nav_win->sqr_x,
					     nav_win->sqr_y);
			break;

		default:
			break;
		}
		return TRUE;

	default:
		break;
	}

	return FALSE;
}


static void
nav_window_grab_pointer (NavWindow *nav_win)
{
	GdkCursor *cursor;

	gtk_grab_add (nav_win->popup_win);

	cursor = gdk_cursor_new (GDK_FLEUR);
	gdk_pointer_grab (nav_win->popup_win->window,
			  TRUE,
			  (GDK_BUTTON_RELEASE_MASK
			   | GDK_POINTER_MOTION_HINT_MASK
			   | GDK_BUTTON_MOTION_MASK
			   | GDK_EXTENSION_EVENTS_ALL),
			  nav_win->preview->window,
			  cursor,
			  0);
	gdk_cursor_unref (cursor);

	/* Capture keyboard events. */

	gdk_keyboard_grab (nav_win->popup_win->window, TRUE, GDK_CURRENT_TIME);
        gtk_widget_grab_focus (nav_win->popup_win);
}


static NavWindow *
nav_window_new (GthImageViewer *viewer)
{
	NavWindow *nav_window;
	GtkWidget *out_frame;
	GtkWidget *in_frame;

	nav_window = g_new0 (NavWindow, 1);

	nav_window->viewer = viewer;
	nav_window->popup_win = gtk_window_new (GTK_WINDOW_POPUP);
	gtk_window_set_wmclass (GTK_WINDOW (nav_window->popup_win), "", "gthumb_navigator");

	out_frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (out_frame), GTK_SHADOW_OUT);
	gtk_container_add (GTK_CONTAINER (nav_window->popup_win), out_frame);

	in_frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (in_frame), GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (out_frame), in_frame);

	nav_window->preview = gtk_drawing_area_new ();
	gtk_container_add (GTK_CONTAINER (in_frame), nav_window->preview);
	g_signal_connect (G_OBJECT (nav_window->preview),
			  "expose_event",
			  G_CALLBACK (nav_window_expose),
			  nav_window);

	/* gc needed to draw the preview square */

	nav_window->gc = gdk_gc_new (GTK_WIDGET (viewer)->window);
	gdk_gc_set_function (nav_window->gc, GDK_INVERT);
	gdk_gc_set_line_attributes (nav_window->gc,
				    PEN_WIDTH,
				    GDK_LINE_SOLID,
				    GDK_CAP_BUTT,
				    GDK_JOIN_MITER);

	return nav_window;
}


void
nav_button_clicked_cb (GtkWidget      *widget,
		       GdkEventButton *event,
		       GthImageViewer *viewer)
{
	NavWindow *nav_win;

	if (gth_image_viewer_is_void (viewer))
		return;

	nav_win = nav_window_new (viewer);

	nav_win->x_root = event->x_root;
	nav_win->y_root = event->y_root;

	nav_win->image_width = gth_image_viewer_get_image_width (viewer);
	nav_win->image_height = gth_image_viewer_get_image_height (viewer);

	update_view (nav_win);

	g_signal_connect (G_OBJECT (nav_win->popup_win),
			  "event",
			  G_CALLBACK (nav_window_events),
			  nav_win);

	gtk_window_move (GTK_WINDOW (nav_win->popup_win),
			 nav_win->popup_x,
			 nav_win->popup_y);

  	gtk_window_set_default_size (GTK_WINDOW (nav_win->popup_win),
				     nav_win->popup_width + B2,
				     nav_win->popup_height + B2);

	gtk_widget_show_all (nav_win->popup_win);

	nav_window_grab_pointer (nav_win);
}


static void
gth_nav_window_construct (GthNavWindow   *nav_window,
			  GthImageViewer *viewer)
{
	GtkAdjustment *vadj = NULL, *hadj = NULL;
	GtkWidget     *hbox;
	GtkWidget     *table;

	nav_window->priv->viewer = viewer;
	g_signal_connect (G_OBJECT (nav_window->priv->viewer),
			  "size_changed",
			  G_CALLBACK (size_changed_cb),
			  nav_window);

	gth_image_viewer_get_adjustments (nav_window->priv->viewer, &hadj, &vadj);
	nav_window->priv->viewer_hscr = gtk_hscrollbar_new (hadj);
	nav_window->priv->viewer_vscr = gtk_vscrollbar_new (vadj);

	nav_window->priv->viewer_nav_event_box = gtk_event_box_new ();
	gtk_container_add (GTK_CONTAINER (nav_window->priv->viewer_nav_event_box), _gtk_image_new_from_xpm_data (nav_button_xpm));

	g_signal_connect (G_OBJECT (nav_window->priv->viewer_nav_event_box),
			  "button_press_event",
			  G_CALLBACK (nav_button_clicked_cb),
			  nav_window->priv->viewer);

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (hbox), GTK_WIDGET (nav_window->priv->viewer));

	table = gtk_table_new (2, 2, FALSE);
	gtk_table_attach (GTK_TABLE (table), hbox, 0, 1, 0, 1,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
	gtk_table_attach (GTK_TABLE (table), nav_window->priv->viewer_vscr, 1, 2, 0, 1,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
	gtk_table_attach (GTK_TABLE (table), nav_window->priv->viewer_hscr, 0, 1, 1, 2,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL), 0, 0);
	gtk_table_attach (GTK_TABLE (table), nav_window->priv->viewer_nav_event_box, 1, 2, 1, 2,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL), 0, 0);

	gtk_widget_show_all (hbox);
	gtk_widget_show (table);

	gtk_container_add (GTK_CONTAINER (nav_window), table);
}


GtkWidget *
gth_nav_window_new (GthImageViewer *viewer)
{
	GthNavWindow *nav_window;

	g_return_val_if_fail (viewer != NULL, NULL);

	nav_window = (GthNavWindow *) g_object_new (GTH_TYPE_NAV_WINDOW, NULL);
	gth_nav_window_construct (nav_window, viewer);

	return (GtkWidget*) nav_window;
}


void
gth_nav_window_set_scrollbars_visible (GthNavWindow *window,
				       gboolean      visible)
{
	window->priv->scrollbars_visible = visible;
	size_changed_cb (NULL, window);
}
