/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2006-2010 Free Software Foundation, Inc.
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
 */

#include <config.h>
#include <math.h>
#include <gdk/gdkkeysyms.h>
#include "gth-image-navigator.h"
#include "gth-image-viewer.h"
#include "gtk-utils.h"
#include "pixbuf-utils.h"


#define VISIBLE_AREA_BORDER 2.0
#define POPUP_BORDER        4
#define POPUP_BORDER_2      8
#define POPUP_MAX_WIDTH     112
#define POPUP_MAX_HEIGHT    112


struct _GthImageNavigatorPrivate {
	GthImageViewer *viewer;
	GtkWidget      *viewer_vscr;
	GtkWidget      *viewer_hscr;
	GtkWidget      *navigator_event_area;
	gboolean        scrollbars_visible;
};


static GtkHBoxClass *parent_class = NULL;


static void
gth_image_navigator_class_init (GthImageNavigatorClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	g_type_class_add_private (class, sizeof (GthImageNavigatorPrivate));

	object_class = G_OBJECT_CLASS (class);
}


static void
gth_image_navigator_init (GthImageNavigator *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE ((self), GTH_TYPE_IMAGE_NAVIGATOR, GthImageNavigatorPrivate);
	self->priv->scrollbars_visible = TRUE;
}


GType
gth_image_navigator_get_type (void)
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (GthImageNavigatorClass),
                        NULL,
                        NULL,
                        (GClassInitFunc) gth_image_navigator_class_init,
                        NULL,
                        NULL,
                        sizeof (GthImageNavigator),
                        0,
                        (GInstanceInitFunc) gth_image_navigator_init
                };

                type = g_type_register_static (GTK_TYPE_HBOX,
                                               "GthImageNavigator",
                                               &type_info,
                                               0);
        }

        return type;
}


/* -- NavigatorPopup -- */


typedef struct {
	GthImageViewer  *viewer;
	int              x_root, y_root;
	GtkWidget       *popup_win;
	GtkWidget       *preview;
	cairo_surface_t *image;
	int              image_width, image_height;
	int              window_max_width, window_max_height;
	int              popup_x, popup_y, popup_width, popup_height;
	GdkRectangle     visible_area;
	double           zoom_factor;
} NavigatorPopup;


static void
get_visible_area_origin_as_double (NavigatorPopup *nav_popup,
				   int             mx,
				   int             my,
				   double         *x,
				   double         *y)
{
	*x = MIN (mx - POPUP_BORDER, nav_popup->window_max_width);
	*y = MIN (my - POPUP_BORDER, nav_popup->window_max_height);

	if (*x - nav_popup->visible_area.width / 2.0 < 0.0)
		*x = nav_popup->visible_area.width / 2.0;

	if (*y - nav_popup->visible_area.height / 2.0 < 0.0)
		*y = nav_popup->visible_area.height / 2.0;

	if (*x + nav_popup->visible_area.width / 2.0 > nav_popup->popup_width - 0)
		*x = nav_popup->popup_width - 0 - nav_popup->visible_area.width / 2.0;

	if (*y + nav_popup->visible_area.height / 2.0 > nav_popup->popup_height - 0)
		*y = nav_popup->popup_height - 0 - nav_popup->visible_area.height / 2.0;

	*x = *x - nav_popup->visible_area.width / 2.0;
	*y = *y - nav_popup->visible_area.height / 2.0;
}


static void
update_popup_geometry (NavigatorPopup *nav_popup)
{
	int           zoomed_width;
	int           zoomed_height;
	GtkAllocation allocation;
	int           scroll_offset_x;
	int           scroll_offset_y;

	zoomed_width = nav_popup->image_width * gth_image_viewer_get_zoom (nav_popup->viewer);
	zoomed_height = nav_popup->image_height * gth_image_viewer_get_zoom (nav_popup->viewer);

	nav_popup->window_max_width = MIN (zoomed_width, POPUP_MAX_WIDTH);
	nav_popup->window_max_height = MIN (zoomed_width, POPUP_MAX_HEIGHT);

	nav_popup->zoom_factor = MIN ((double) (nav_popup->window_max_width) / zoomed_width,
				    (double) (nav_popup->window_max_height) / zoomed_height);

	/* popup window size */

	nav_popup->popup_width  = MAX ((int) floor (nav_popup->zoom_factor * zoomed_width + 0.5), 1);
	nav_popup->popup_height = MAX ((int) floor (nav_popup->zoom_factor * zoomed_height + 0.5), 1);

	cairo_surface_destroy (nav_popup->image);
	nav_popup->image = cairo_surface_reference (gth_image_viewer_get_current_image (nav_popup->viewer));

	/* visible area size */

	gtk_widget_get_allocation (GTK_WIDGET (nav_popup->viewer), &allocation);
	nav_popup->visible_area.width = (allocation.width - GTH_IMAGE_VIEWER_FRAME_BORDER2) * nav_popup->zoom_factor;
	nav_popup->visible_area.width = MAX (nav_popup->visible_area.width, POPUP_BORDER);
	nav_popup->visible_area.width = MIN (nav_popup->visible_area.width, nav_popup->popup_width);

	nav_popup->visible_area.height = (allocation.height - GTH_IMAGE_VIEWER_FRAME_BORDER2) * nav_popup->zoom_factor;
	nav_popup->visible_area.height = MAX (nav_popup->visible_area.height, POPUP_BORDER);
	nav_popup->visible_area.height = MIN (nav_popup->visible_area.height, nav_popup->popup_height);

	/* visible area position */

	gth_image_viewer_get_scroll_offset (nav_popup->viewer, &scroll_offset_x, &scroll_offset_y);
	nav_popup->visible_area.x = scroll_offset_x * nav_popup->zoom_factor;
	nav_popup->visible_area.y = scroll_offset_y * nav_popup->zoom_factor;

	/* popup window position */

	nav_popup->popup_x = MIN ((int) nav_popup->x_root - nav_popup->visible_area.x - POPUP_BORDER - nav_popup->visible_area.width / 2,
				gdk_screen_width () - nav_popup->popup_width - POPUP_BORDER_2);
	nav_popup->popup_y = MIN ((int) nav_popup->y_root - nav_popup->visible_area.y - POPUP_BORDER - nav_popup->visible_area.height / 2,
				gdk_screen_height () - nav_popup->popup_height - POPUP_BORDER_2);
}


static int
popup_window_event_cb (GtkWidget *widget,
		       GdkEvent  *event,
		       gpointer   data)
{
	NavigatorPopup  *nav_popup = data;
	GthImageViewer  *viewer = nav_popup->viewer;
	GdkModifierType  mask;
	int              mx, my;
	double           x, y;

	switch (event->type) {
	case GDK_BUTTON_RELEASE:
		/* Release keyboard focus. */
		gdk_keyboard_ungrab (GDK_CURRENT_TIME);
		gtk_grab_remove (nav_popup->popup_win);

		gtk_widget_destroy (nav_popup->popup_win);
		cairo_surface_destroy (nav_popup->image);
		g_free (nav_popup);

		return TRUE;

	case GDK_MOTION_NOTIFY:
		gdk_window_get_pointer (gtk_widget_get_window (widget), &mx, &my, &mask);

		get_visible_area_origin_as_double (nav_popup, mx, my, &x, &y);
		nav_popup->visible_area.x = (int) x;
		nav_popup->visible_area.y = (int) y;

		mx = (int) (x / nav_popup->zoom_factor);
		my = (int) (y / nav_popup->zoom_factor);
		gth_image_viewer_scroll_to (viewer, mx, my);

		gtk_widget_queue_draw (widget);
		gdk_window_process_updates (gtk_widget_get_window (widget), TRUE);

		return TRUE;

	case GDK_KEY_PRESS:
		switch (event->key.keyval) {
		case GDK_plus:
		case GDK_minus:
		case GDK_1:
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

			update_popup_geometry (nav_popup);
			nav_popup->visible_area.x = MAX (nav_popup->visible_area.x, 0);
			nav_popup->visible_area.x = MIN (nav_popup->visible_area.x, nav_popup->popup_width - nav_popup->visible_area.width);
			nav_popup->visible_area.y = MAX (nav_popup->visible_area.y, 0);
			nav_popup->visible_area.y = MIN (nav_popup->visible_area.y, nav_popup->popup_height - nav_popup->visible_area.height);
			gtk_widget_queue_draw (widget);
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
nav_window_grab_pointer (NavigatorPopup *nav_popup)
{
	GdkCursor *cursor;

	gtk_grab_add (nav_popup->popup_win);

	cursor = gdk_cursor_new (GDK_FLEUR);
	gdk_pointer_grab (gtk_widget_get_window (nav_popup->popup_win),
			  TRUE,
			  (GDK_BUTTON_RELEASE_MASK
			   | GDK_POINTER_MOTION_HINT_MASK
			   | GDK_BUTTON_MOTION_MASK
			   | GDK_EXTENSION_EVENTS_ALL),
			   gtk_widget_get_window (nav_popup->preview),
			  cursor,
			  0);
	gdk_cursor_unref (cursor);

	/* Capture keyboard events. */

	gdk_keyboard_grab (gtk_widget_get_window (nav_popup->popup_win), TRUE, GDK_CURRENT_TIME);
        gtk_widget_grab_focus (nav_popup->popup_win);
}


static gboolean
navigator_popup_expose_event_cb (GtkWidget      *widget,
				 GdkEventExpose *event,
				 NavigatorPopup *nav_popup)
{
	cairo_t *cr;
	double   zoom;

	if (nav_popup->image == NULL)
		return FALSE;

	cr = gdk_cairo_create (gtk_widget_get_window (widget));

	cairo_set_antialias (cr, CAIRO_ANTIALIAS_NONE);
	gdk_cairo_region (cr, event->region);
	cairo_clip (cr);

	zoom = (double) nav_popup->popup_width / nav_popup->image_width;

	cairo_save (cr);
	cairo_scale (cr, zoom, zoom);
	cairo_set_source_surface (cr, nav_popup->image, 0, 0);
	cairo_pattern_set_filter (cairo_get_source (cr), CAIRO_FILTER_FAST);
	cairo_rectangle (cr, 0, 0, nav_popup->image_width, nav_popup->image_height);
  	cairo_fill (cr);
  	cairo_restore (cr);

	cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 0.5);
	cairo_rectangle (cr, 0, 0, nav_popup->popup_width, nav_popup->popup_height);
	cairo_fill (cr);

	if ((nav_popup->visible_area.width < nav_popup->popup_width)
	    || (nav_popup->visible_area.height < nav_popup->popup_height))
	{
		cairo_save (cr);
		cairo_rectangle (cr,
				 nav_popup->visible_area.x,
				 nav_popup->visible_area.y,
				 nav_popup->visible_area.width,
				 nav_popup->visible_area.height);
		cairo_clip (cr);
		cairo_scale (cr, zoom, zoom);
		cairo_set_source_surface (cr, nav_popup->image, 0, 0);
		cairo_pattern_set_filter (cairo_get_source (cr), CAIRO_FILTER_FAST);
	  	cairo_rectangle (cr, 0, 0, nav_popup->image_width, nav_popup->image_width);
	  	cairo_fill (cr);
	  	cairo_restore (cr);

		cairo_save (cr);
		cairo_set_line_width (cr, VISIBLE_AREA_BORDER);
		cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
		cairo_rectangle (cr,
				 nav_popup->visible_area.x + 1.0,
				 nav_popup->visible_area.y + 1.0,
				 nav_popup->visible_area.width - VISIBLE_AREA_BORDER,
				 nav_popup->visible_area.height - VISIBLE_AREA_BORDER);
		cairo_stroke (cr);
		cairo_restore (cr);
	}

	cairo_destroy (cr);

	return TRUE;
}


static void
navigator_event_area_button_press_event_cb (GtkWidget      *widget,
					    GdkEventButton *event,
					    GthImageViewer *viewer)
{
	NavigatorPopup *nav_popup;
	GtkWidget      *out_frame;
	GtkWidget      *in_frame;

	if (gth_image_viewer_is_void (viewer))
		return;

	nav_popup = g_new0 (NavigatorPopup, 1);
	nav_popup->viewer = viewer;
	nav_popup->popup_win = gtk_window_new (GTK_WINDOW_POPUP);
	gtk_window_set_wmclass (GTK_WINDOW (nav_popup->popup_win), "", "gthumb_navigator");

	out_frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (out_frame), GTK_SHADOW_OUT);
	gtk_container_add (GTK_CONTAINER (nav_popup->popup_win), out_frame);

	in_frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (in_frame), GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (out_frame), in_frame);

	nav_popup->preview = gtk_drawing_area_new ();
	gtk_container_add (GTK_CONTAINER (in_frame), nav_popup->preview);
	g_signal_connect (G_OBJECT (nav_popup->preview),
			  "expose_event",
			  G_CALLBACK (navigator_popup_expose_event_cb),
			  nav_popup);

	nav_popup->x_root = event->x_root;
	nav_popup->y_root = event->y_root;
	nav_popup->image_width = gth_image_viewer_get_image_width (viewer);
	nav_popup->image_height = gth_image_viewer_get_image_height (viewer);
	update_popup_geometry (nav_popup);

	g_signal_connect (G_OBJECT (nav_popup->popup_win),
			  "event",
			  G_CALLBACK (popup_window_event_cb),
			  nav_popup);

	gtk_window_move (GTK_WINDOW (nav_popup->popup_win),
			 nav_popup->popup_x,
			 nav_popup->popup_y);

  	gtk_window_set_default_size (GTK_WINDOW (nav_popup->popup_win),
				     nav_popup->popup_width + POPUP_BORDER_2,
				     nav_popup->popup_height + POPUP_BORDER_2);

	gtk_widget_show_all (nav_popup->popup_win);

	nav_window_grab_pointer (nav_popup);
}


static gboolean
image_viewer_size_changed_cb (GtkWidget         *widget,
			      GthImageNavigator *self)
{
	GtkAdjustment *vadj;
	GtkAdjustment *hadj;
	double         h_page_size;
	double         v_page_size;
	double         h_upper;
	double         v_upper;
	gboolean       hide_vscr;
	gboolean       hide_hscr;

	gth_image_viewer_get_adjustments (self->priv->viewer, &hadj, &vadj);

	g_return_val_if_fail (hadj != NULL, FALSE);
	g_return_val_if_fail (vadj != NULL, FALSE);

	h_page_size = gtk_adjustment_get_page_size (hadj);
	v_page_size = gtk_adjustment_get_page_size (vadj);
	h_upper = gtk_adjustment_get_upper (hadj);
	v_upper = gtk_adjustment_get_upper (vadj);
	hide_vscr = (v_page_size == 0) || (v_upper <= v_page_size);
	hide_hscr = (h_page_size == 0) || (h_upper <= h_page_size);

	if (! self->priv->scrollbars_visible || (hide_vscr && hide_hscr)) {
		gtk_widget_hide (self->priv->viewer_vscr);
		gtk_widget_hide (self->priv->viewer_hscr);
		gtk_widget_hide (self->priv->navigator_event_area);
	}
	else {
		gtk_widget_show (self->priv->viewer_vscr);
		gtk_widget_show (self->priv->viewer_hscr);
		gtk_widget_show (self->priv->navigator_event_area);
	}

	return TRUE;
}


static void
gth_image_navigator_construct (GthImageNavigator *self,
			       GthImageViewer    *viewer)
{
	GtkAdjustment *vadj = NULL;
	GtkAdjustment *hadj = NULL;
	GtkWidget     *hbox;
	GtkWidget     *table;

	self->priv->viewer = viewer;
	g_signal_connect (G_OBJECT (self->priv->viewer),
			  "size_changed",
			  G_CALLBACK (image_viewer_size_changed_cb),
			  self);

	gth_image_viewer_get_adjustments (self->priv->viewer, &hadj, &vadj);
	self->priv->viewer_hscr = gtk_hscrollbar_new (hadj);
	self->priv->viewer_vscr = gtk_vscrollbar_new (vadj);

	self->priv->navigator_event_area = gtk_event_box_new ();
	gtk_container_add (GTK_CONTAINER (self->priv->navigator_event_area), gtk_image_new_from_icon_name ("image-navigator", GTK_ICON_SIZE_MENU));

	g_signal_connect (G_OBJECT (self->priv->navigator_event_area),
			  "button_press_event",
			  G_CALLBACK (navigator_event_area_button_press_event_cb),
			  self->priv->viewer);

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (hbox), GTK_WIDGET (self->priv->viewer));

	table = gtk_table_new (2, 2, FALSE);
	gtk_table_attach (GTK_TABLE (table), hbox, 0, 1, 0, 1,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
	gtk_table_attach (GTK_TABLE (table), self->priv->viewer_vscr, 1, 2, 0, 1,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
	gtk_table_attach (GTK_TABLE (table), self->priv->viewer_hscr, 0, 1, 1, 2,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL), 0, 0);
	gtk_table_attach (GTK_TABLE (table), self->priv->navigator_event_area, 1, 2, 1, 2,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL), 0, 0);
	gtk_widget_show_all (table);

	gtk_container_add (GTK_CONTAINER (self), table);
}


GtkWidget *
gth_image_navigator_new (GthImageViewer *viewer)
{
	GthImageNavigator *self;

	g_return_val_if_fail (viewer != NULL, NULL);

	self = g_object_new (GTH_TYPE_IMAGE_NAVIGATOR, NULL);
	gth_image_navigator_construct (self, viewer);

	return (GtkWidget*) self;
}


void
gth_image_navigator_set_scrollbars_visible (GthImageNavigator *self,
				            gboolean           visible)
{
	self->priv->scrollbars_visible = visible;
	image_viewer_size_changed_cb (NULL, self);
}
