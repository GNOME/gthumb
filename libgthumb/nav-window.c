/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001, 2005 The Free Software Foundation, Inc.
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

#include <math.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include "image-viewer.h"
#include "image-loader.h"


#define PEN_WIDTH         3       /* Square border width. */ 
#define B                 4       /* Window border width. */
#define B2                8       /* Window border width * 2. */

#define NAV_WIN_MAX_WIDTH     112     /* Max size of the window. */
#define NAV_WIN_MAX_HEIGHT    112


typedef struct {
	ImageViewer * viewer;

	gint          x_root, y_root;

	GtkWidget *   popup_win;
	GtkWidget *   preview;
	GdkPixbuf *   pixbuf;

	GdkGC *       gc;

	gint          image_width, image_height;
	gint          window_max_width, window_max_height;

	gint          popup_x, popup_y, popup_width, popup_height;
	gint          sqr_x, sqr_y, sqr_width, sqr_height;

	gdouble       factor;
	gdouble       sqr_x_d, sqr_y_d;
} NavWindow; 


static void
nav_window_draw_sqr (NavWindow *nav_win,
		     gboolean undraw,
		     gint x,
		     gint y)
{
	if ((nav_win->sqr_x == x) && (nav_win->sqr_y == y) && undraw)
		return;

	if ((nav_win->sqr_x == 0) 
	    && (nav_win->sqr_y == 0)
	    && (nav_win->sqr_width == nav_win->popup_width) 
	    && (nav_win->sqr_height == nav_win->popup_height))
		return;

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
			  gint mx,
			  gint my,
			  gdouble *x,
			  gdouble *y)
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
	ImageViewer     *viewer = nav_win->viewer;
	GdkPixbuf       *image_pixbuf;
	gint             popup_x, popup_y;
	gint             popup_width, popup_height;
	gint             w, h;
	gdouble          factor;
	gint             gdk_width, gdk_height;

	w = nav_win->image_width * image_viewer_get_zoom (viewer);
	h = nav_win->image_height * image_viewer_get_zoom (viewer);

	nav_win->window_max_width = MIN (w, NAV_WIN_MAX_WIDTH);
	nav_win->window_max_height = MIN (w, NAV_WIN_MAX_HEIGHT);

	factor = MIN ((gdouble) (nav_win->window_max_width) / w, 
		      (gdouble) (nav_win->window_max_height) / h);
	nav_win->factor = factor;

	gdk_width = GTK_WIDGET (viewer)->allocation.width - FRAME_BORDER2;
	gdk_height = GTK_WIDGET (viewer)->allocation.height - FRAME_BORDER2;

	/* Popup window size. */

	popup_width  = MAX ((gint) floor (factor * w + 0.5), 1);
	popup_height = MAX ((gint) floor (factor * h + 0.5), 1);

	image_pixbuf = image_viewer_get_current_pixbuf (viewer);
	g_return_if_fail (image_pixbuf != NULL);

	if (nav_win->pixbuf != NULL) 
		g_object_unref (nav_win->pixbuf);
	nav_win->pixbuf = gdk_pixbuf_scale_simple (image_pixbuf,
						   popup_width,
						   popup_height,
						   GDK_INTERP_BILINEAR);

	/* The square. */

	nav_win->sqr_width = gdk_width * factor;
	nav_win->sqr_width = MAX (nav_win->sqr_width, B);
	nav_win->sqr_width = MIN (nav_win->sqr_width, popup_width);

	nav_win->sqr_height = gdk_height * factor;
	nav_win->sqr_height = MAX (nav_win->sqr_height, B); 
	nav_win->sqr_height = MIN (nav_win->sqr_height, popup_height); 

	nav_win->sqr_x = viewer->x_offset * factor;
	nav_win->sqr_y = viewer->y_offset * factor;

	/* Popup window position. */

	popup_x = MIN ((gint) nav_win->x_root - nav_win->sqr_x 
		       - B 
		       - nav_win->sqr_width / 2,
		       gdk_screen_width () - popup_width - B2);
	popup_y = MIN ((gint) nav_win->y_root - nav_win->sqr_y 
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

	if (! gdk_pixbuf_get_has_alpha (nav_win->pixbuf))
		gdk_pixbuf_render_to_drawable (
		       nav_win->pixbuf,
		       nav_win->preview->window,
		       nav_win->preview->style->white_gc,
		       0, 0,
		       0, 0,
		       nav_win->popup_width,
		       nav_win->popup_height,
		       GDK_RGB_DITHER_MAX,
		       0, 0);
	else
		gdk_pixbuf_render_to_drawable_alpha (
			nav_win->pixbuf,
			nav_win->preview->window,
			0, 0,
			0, 0,
			nav_win->popup_width,
			nav_win->popup_height,
			GDK_PIXBUF_ALPHA_BILEVEL,
			112, /* FIXME */
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
	gint             mx, my;
	gdouble          x, y;
	ImageViewer     *viewer = nav_win->viewer;

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

		mx = (gint) x;
		my = (gint) y;
		nav_window_draw_sqr (nav_win, TRUE, mx, my);

		mx = (gint) (x / nav_win->factor);
		my = (gint) (y / nav_win->factor);
		image_viewer_scroll_to (viewer, mx, my);

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
				image_viewer_zoom_in (viewer); 
				break;
			case GDK_minus:
				image_viewer_zoom_out (viewer); 
				break;
			case GDK_1:
				image_viewer_set_zoom (viewer, 1.0);
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
nav_window_new (ImageViewer *viewer)
{
	NavWindow *nav_window;
	GtkWidget *out_frame;
	GtkWidget *in_frame;

	nav_window = g_new0 (NavWindow, 1);

	nav_window->viewer = viewer;
	nav_window->popup_win = gtk_window_new (GTK_WINDOW_POPUP);
	gtk_window_set_wmclass (GTK_WINDOW (nav_window->popup_win), "",
                                "gthumb_navigator");

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
		       ImageViewer    *viewer)
{
	NavWindow *nav_win;

	if (image_viewer_is_void (viewer))
		return;

	nav_win = nav_window_new (viewer);

	nav_win->x_root = event->x_root;
	nav_win->y_root = event->y_root;

	nav_win->image_width = image_viewer_get_image_width (viewer);
	nav_win->image_height = image_viewer_get_image_height (viewer);

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
