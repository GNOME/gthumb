/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001 Free Software Foundation, Inc.
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

#include <stdlib.h>
#include <math.h>

#include <glib/gi18n.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtkadjustment.h>
#include <gtk/gtkhscrollbar.h>
#include <gtk/gtkvscrollbar.h>
#include <libgnomevfs/gnome-vfs-types.h>
#include <libgnomevfs/gnome-vfs-uri.h>
#include <libgnomevfs/gnome-vfs-utils.h>

#include "gth-iviewer.h"
#include "image-viewer.h"
#include "image-viewer-image.h"
#include "cursors.h"
#include "pixbuf-utils.h"
#include "gthumb-marshal.h"
#include "gthumb-enum-types.h"
#include "glib-utils.h"

#define COLOR_GRAY_00   0x00000000
#define COLOR_GRAY_33   0x00333333
#define COLOR_GRAY_66   0x00666666
#define COLOR_GRAY_99   0x00999999
#define COLOR_GRAY_CC   0x00cccccc
#define COLOR_GRAY_FF   0x00ffffff

#define DRAG_THRESHOLD  1     /* When dragging the image ignores movements
			       * smaller than this value. */
#define STEP_INCREMENT  20.0  /* Scroll increment. */

#define FLOAT_EQUAL(a,b) (fabs ((a) - (b)) < 1e-6)

typedef struct _ImageViewerPrivate ImageViewerPrivate;

struct _ImageViewerPrivate {
	ImageLoader      *loader;
	ImageViewerImage *image;

	gboolean         play_animation;

	gboolean         rendering;
	gboolean         cursor_visible;

	gboolean         frame_visible;
	int              frame_border;
	int              frame_border2;

	GthTranspType    transp_type;
	GthCheckType     check_type;
	gint             check_size;

	GdkCursor       *cursor;
	GdkCursor       *cursor_void;

	GthZoomQuality   zoom_quality;
	GthZoomChange    zoom_change;

	GthFit           fit;

	gboolean         pressed;
	gboolean         dragging;
	gboolean         double_click;
	gboolean         just_focused;
	gint             drag_x;
	gint             drag_y;
	gint             drag_realx;
	gint             drag_realy;

	GtkAdjustment   *vadj, *hadj;

	gboolean         black_bg;

	gboolean         skip_zoom_change;
	gboolean         skip_size_change;

	gboolean         reset_scrollbars;	
};


enum {
	CLICKED,
	IMAGE_LOADED,
	IMAGE_ERROR,
	IMAGE_PROGRESS,
	ZOOM_IN,
	ZOOM_OUT,
	SET_ZOOM,
	SET_FIT_MODE,
	ZOOM_CHANGED,
	MOUSE_WHEEL_SCROLL,
	SCROLL,
	LAST_SIGNAL
};

static void
gth_iviewer_interface_init (gpointer g_iface, gpointer iface_data);

G_DEFINE_TYPE_EXTENDED (ImageViewer, 
                	image_viewer, 
                        GTK_TYPE_WIDGET,
                        0, 
                        G_IMPLEMENT_INTERFACE (GTH_TYPE_IVIEWER, 
                                               gth_iviewer_interface_init));

#define IMAGE_VIEWER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), \
				     IMAGE_VIEWER_TYPE, \
				     ImageViewerPrivate))

static gboolean image_viewer_expose           (GtkWidget        *widget,
					       GdkEventExpose   *event);
static gboolean image_viewer_button_press     (GtkWidget        *widget,
					       GdkEventButton   *event);
static gboolean image_viewer_button_release   (GtkWidget        *widget,
					       GdkEventButton   *event);
static gboolean image_viewer_motion_notify    (GtkWidget        *widget,
					       GdkEventMotion   *event);
static void     image_viewer_finalize         (GObject          *viewer);
static void     image_viewer_realize          (GtkWidget        *widget);
static void     image_viewer_unrealize        (GtkWidget        *widget);
static void     image_viewer_size_allocate    (GtkWidget        *widget,
					       GtkAllocation    *allocation);
static gint     image_viewer_focus_in         (GtkWidget        *widget,
					       GdkEventFocus    *event);
static gint     image_viewer_focus_out        (GtkWidget        *widget,
					       GdkEventFocus    *event);
static gint     image_viewer_scroll_event     (GtkWidget        *widget,
					       GdkEventScroll   *event);
static void     scroll_relative               (ImageViewer      *viewer,
					       gdouble           delta_x,
					       gdouble           delta_y);
static void     scroll_relative_no_scrollbar  (ImageViewer      *viewer,
					       gdouble           delta_x,
					       gdouble           delta_y);
static void     set_scroll_adjustments        (GtkWidget        *widget,
                                               GtkAdjustment    *hadj,
					       GtkAdjustment    *vadj);
static void     scroll_signal                 (GtkWidget        *widget,
					       GtkScrollType     xscroll_type,
					       GtkScrollType     yscroll_type);

static void     zoom_changed_cb               (ImageViewerImage *image,
					       gpointer          data);

static guint image_viewer_signals[LAST_SIGNAL] = { 0 };


static gint
image_viewer_key_press (GtkWidget *widget,
			GdkEventKey *event)
{
	gboolean handled;

	handled = gtk_bindings_activate (GTK_OBJECT (widget),
					 event->keyval,
					 event->state);
	if (handled)
		return TRUE;

	if (GTK_WIDGET_CLASS (image_viewer_parent_class)->key_press_event &&
	    GTK_WIDGET_CLASS (image_viewer_parent_class)->key_press_event (widget, event))
		return TRUE;

	return FALSE;
}


static void
image_viewer_class_init (ImageViewerClass *klass)
{
	GObjectClass   *gobject_class;
	GtkWidgetClass *widget_class;
	GtkBindingSet  *binding_set;

	g_type_class_add_private (klass, sizeof (ImageViewerPrivate));

	widget_class = GTK_WIDGET_CLASS (klass);
	gobject_class = G_OBJECT_CLASS (klass);

	image_viewer_signals[CLICKED] =
		g_signal_new ("clicked",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (ImageViewerClass, clicked),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
	image_viewer_signals[IMAGE_LOADED] =
                g_signal_new ("image_loaded",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (ImageViewerClass, image_loaded),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
	image_viewer_signals[IMAGE_ERROR] =
                g_signal_new ("image_error",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (ImageViewerClass, image_error),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
	image_viewer_signals[IMAGE_PROGRESS] =
                g_signal_new ("image_progress",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (ImageViewerClass, image_progress),
			      NULL, NULL,
			      gthumb_marshal_VOID__FLOAT,
			      G_TYPE_NONE,
			      1, G_TYPE_FLOAT);
	image_viewer_signals[ZOOM_IN] =
		g_signal_new ("zoom_in",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			      G_STRUCT_OFFSET (ImageViewerClass, zoom_in),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
	image_viewer_signals[ZOOM_OUT] =
		g_signal_new ("zoom_out",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			      G_STRUCT_OFFSET (ImageViewerClass, zoom_out),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
	image_viewer_signals[SET_ZOOM] =
		g_signal_new ("set_zoom",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			      G_STRUCT_OFFSET (ImageViewerClass, set_zoom),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__DOUBLE,
			      G_TYPE_NONE,
			      1, G_TYPE_DOUBLE);
	image_viewer_signals[SET_FIT_MODE] =
		g_signal_new ("set_fit_mode",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			      G_STRUCT_OFFSET (ImageViewerClass, set_fit_mode),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__ENUM,
			      G_TYPE_NONE,
			      1, GTH_TYPE_FIT);
	image_viewer_signals[ZOOM_CHANGED] =
		g_signal_new ("zoom_changed",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (ImageViewerClass, zoom_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
        image_viewer_signals[MOUSE_WHEEL_SCROLL] =
		g_signal_new ("mouse_wheel_scroll",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (ImageViewerClass, mouse_wheel_scroll),
			      NULL, NULL,
			      gthumb_marshal_VOID__ENUM,
			      G_TYPE_NONE,
			      1, GDK_TYPE_SCROLL_DIRECTION);

	klass->set_scroll_adjustments = set_scroll_adjustments;
        widget_class->set_scroll_adjustments_signal =
		g_signal_new ("set_scroll_adjustments",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (ImageViewerClass, set_scroll_adjustments),
			      NULL, NULL,
			      gthumb_marshal_VOID__OBJECT_OBJECT,
			      G_TYPE_NONE,
			      2, GTK_TYPE_ADJUSTMENT, GTK_TYPE_ADJUSTMENT);
	image_viewer_signals[SCROLL] =
		g_signal_new ("scroll",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			      G_STRUCT_OFFSET (ImageViewerClass, scroll),
			      NULL, NULL,
			      gthumb_marshal_VOID__ENUM_ENUM,
			      G_TYPE_NONE,
			      2, GTK_TYPE_SCROLL_TYPE, GTK_TYPE_SCROLL_TYPE);

	gobject_class->finalize = image_viewer_finalize;

	widget_class->realize         = image_viewer_realize;
	widget_class->unrealize       = image_viewer_unrealize;
	widget_class->size_allocate   = image_viewer_size_allocate;
	widget_class->focus_in_event  = image_viewer_focus_in;
	widget_class->focus_out_event = image_viewer_focus_out;
	widget_class->key_press_event = image_viewer_key_press;

	widget_class->expose_event = image_viewer_expose;
	widget_class->button_press_event = image_viewer_button_press;
	widget_class->button_release_event = image_viewer_button_release;
	widget_class->motion_notify_event = image_viewer_motion_notify;

	widget_class->scroll_event = image_viewer_scroll_event;

	klass->clicked        = NULL;
	klass->image_loaded   = NULL;
	klass->image_error    = NULL;
	klass->image_progress = NULL;
	klass->zoom_changed   = NULL;
	klass->scroll         = scroll_signal;
	klass->zoom_in        = image_viewer_zoom_in;
	klass->zoom_out       = image_viewer_zoom_out;
	klass->set_zoom       = image_viewer_set_zoom;
	klass->set_fit_mode   = image_viewer_set_fit_mode;

	/* Add key bindings */

        binding_set = gtk_binding_set_by_class (klass);

	/* For scrolling */

	gtk_binding_entry_add_signal (binding_set, GDK_Right, 0,
			      "scroll", 2,
			      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_STEP_RIGHT,
			      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_NONE);
	gtk_binding_entry_add_signal (binding_set, GDK_Left, 0,
			      "scroll", 2,
			      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_STEP_LEFT,
			      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_NONE);
	gtk_binding_entry_add_signal (binding_set, GDK_Down, 0,
			      "scroll", 2,
			      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_NONE,
			      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_STEP_DOWN);
	gtk_binding_entry_add_signal (binding_set, GDK_Up, 0,
			      "scroll", 2,
			      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_NONE,
			      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_STEP_UP);

	gtk_binding_entry_add_signal (binding_set, GDK_Right, GDK_SHIFT_MASK,
			      "scroll", 2,
			      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_PAGE_RIGHT,
			      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_NONE);
	gtk_binding_entry_add_signal (binding_set, GDK_Left, GDK_SHIFT_MASK,
			      "scroll", 2,
			      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_PAGE_LEFT,
			      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_NONE);
	gtk_binding_entry_add_signal (binding_set, GDK_Down, GDK_SHIFT_MASK,
			      "scroll", 2,
			      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_NONE,
			      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_PAGE_DOWN);
	gtk_binding_entry_add_signal (binding_set, GDK_Up, GDK_SHIFT_MASK,
			      "scroll", 2,
			      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_NONE,
			      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_PAGE_UP);

	gtk_binding_entry_add_signal (binding_set, GDK_Page_Down, 0,
			      "scroll", 2,
			      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_NONE,
			      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_PAGE_DOWN);
	gtk_binding_entry_add_signal (binding_set, GDK_Page_Up, 0,
			      "scroll", 2,
			      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_NONE,
			      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_PAGE_UP);

	/* Zoom in */
	
	gtk_binding_entry_add_signal (binding_set, GDK_plus, 0,
				      "zoom_in", 0);
	gtk_binding_entry_add_signal (binding_set, GDK_equal, 0,
				      "zoom_in", 0);
	gtk_binding_entry_add_signal (binding_set, GDK_KP_Add, 0,
				      "zoom_in", 0);

	/* Zoom out */
	
	gtk_binding_entry_add_signal (binding_set, GDK_minus, 0,
				      "zoom_out", 0);
	gtk_binding_entry_add_signal (binding_set, GDK_KP_Subtract, 0,
				      "zoom_out", 0);

	/* Set zoom */
	
	gtk_binding_entry_add_signal (binding_set, GDK_KP_Divide, 0,
				      "set_zoom", 1,
				      G_TYPE_DOUBLE, 1.0);
	gtk_binding_entry_add_signal (binding_set, GDK_1, 0,
				      "set_zoom", 1,
				      G_TYPE_DOUBLE, 1.0);
	gtk_binding_entry_add_signal (binding_set, GDK_z, 0,
				      "set_zoom", 1,
				      G_TYPE_DOUBLE, 1.0);
	gtk_binding_entry_add_signal (binding_set, GDK_2, 0,
				      "set_zoom", 1,
				      G_TYPE_DOUBLE, 2.0);
	gtk_binding_entry_add_signal (binding_set, GDK_3, 0,
				      "set_zoom", 1,
				      G_TYPE_DOUBLE, 3.0);

	/* Set fit mode */
	
	gtk_binding_entry_add_signal (binding_set, GDK_x, 0,
				      "set_fit_mode", 1,
				      GTH_TYPE_FIT, GTH_FIT_SIZE_IF_LARGER);
	gtk_binding_entry_add_signal (binding_set, GDK_KP_Multiply, 0,
                                      "set_fit_mode", 1,
                                      GTH_TYPE_FIT, GTH_FIT_SIZE_IF_LARGER);
	gtk_binding_entry_add_signal (binding_set, GDK_x, GDK_SHIFT_MASK,
				      "set_fit_mode", 1,
				      GTH_TYPE_FIT, GTH_FIT_SIZE);
	gtk_binding_entry_add_signal (binding_set, GDK_w, 0,
				      "set_fit_mode", 1,
				      GTH_TYPE_FIT, GTH_FIT_WIDTH_IF_LARGER);
	gtk_binding_entry_add_signal (binding_set, GDK_w, GDK_SHIFT_MASK,
				      "set_fit_mode", 1,
				      GTH_TYPE_FIT, GTH_FIT_WIDTH);
}


static void create_image   (ImageLoader *il, gpointer data);
static void image_loaded   (ImageLoader *il, gpointer data);
static void image_error    (ImageLoader *il, gpointer data);
static void image_progress (ImageLoader *il, float progress, gpointer data);


static gboolean
hadj_value_changed (GtkAdjustment *adj,
		    ImageViewer   *viewer)
{
	ImageViewerPrivate  *priv = IMAGE_VIEWER_GET_PRIVATE (viewer);
	gdouble              x_ofs, y_ofs;

	g_return_val_if_fail (priv->image != NULL, FALSE);

	x_ofs = floor (adj->value + adj->page_size / 2) -
		image_viewer_image_get_x_offset (priv->image);
	y_ofs = 0;
	scroll_relative_no_scrollbar (viewer, x_ofs, y_ofs);

	return FALSE;
}


static gboolean
vadj_value_changed (GtkAdjustment *adj,
		    ImageViewer   *viewer)
{
	ImageViewerPrivate  *priv = IMAGE_VIEWER_GET_PRIVATE (viewer);
	gdouble              x_ofs, y_ofs;
	
	g_return_val_if_fail (priv->image != NULL, FALSE);

	x_ofs = 0;
	y_ofs = floor (adj->value + adj->page_size / 2) -
		image_viewer_image_get_y_offset (priv->image);
	scroll_relative_no_scrollbar (viewer, x_ofs, y_ofs);

	return FALSE;
}


static void
image_viewer_init (ImageViewer *viewer)
{
	ImageViewerPrivate  *priv = IMAGE_VIEWER_GET_PRIVATE (viewer);

	GTK_WIDGET_SET_FLAGS (viewer, GTK_CAN_FOCUS);
	/*GTK_WIDGET_UNSET_FLAGS (viewer, GTK_DOUBLE_BUFFERED);*/

	/* Initialize data. */

	priv->check_type = GTH_CHECK_TYPE_MIDTONE;
	priv->check_size = GTH_CHECK_SIZE_LARGE;

	priv->rendering = FALSE;
	priv->cursor_visible = TRUE;

	priv->frame_visible = TRUE;
	priv->frame_border = FRAME_BORDER;
	priv->frame_border2 = FRAME_BORDER2;

	priv->play_animation = TRUE;

	priv->loader = IMAGE_LOADER (image_loader_new (TRUE));
	g_signal_connect (G_OBJECT (priv->loader),
			  "image_done",
			  G_CALLBACK (image_loaded),
			  viewer);
	g_signal_connect (G_OBJECT (priv->loader),
			  "image_error",
			  G_CALLBACK (image_error),
			  viewer);
	g_signal_connect (G_OBJECT (priv->loader),
			  "image_progress",
			  G_CALLBACK (image_progress),
			  viewer);

	priv->skip_zoom_change = FALSE;
	priv->skip_size_change = FALSE;

	priv->dragging = FALSE;
	priv->double_click = FALSE;
	priv->just_focused = FALSE;

	priv->black_bg = FALSE;

	priv->cursor = NULL;
	priv->cursor_void = NULL;

	priv->reset_scrollbars = TRUE;

	/* Create the widget. */

	priv->hadj = GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 1.0, 0.0,
							   1.0, 1.0, 1.0));
	priv->vadj = GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 1.0, 0.0,
							   1.0, 1.0, 1.0));

	g_object_ref_sink (priv->hadj);
	g_object_ref_sink (priv->vadj);

	g_signal_connect (G_OBJECT (priv->hadj),
			  "value_changed",
			  G_CALLBACK (hadj_value_changed),
			  viewer);
	g_signal_connect (G_OBJECT (priv->vadj),
			  "value_changed",
			  G_CALLBACK (vadj_value_changed),
			  viewer);
}


static gint
image_viewer_expose (GtkWidget      *widget,
		     GdkEventExpose *event)
{
	ImageViewer        *viewer;
	ImageViewerPrivate *priv;
	int                 gdk_width,   gdk_height;
	int                 alloc_width, alloc_height;

	viewer = IMAGE_VIEWER (widget);
	priv = IMAGE_VIEWER_GET_PRIVATE (viewer);

	if (priv->rendering)
		return FALSE;

	priv->rendering = TRUE;

	alloc_width  = widget->allocation.width;
	alloc_height = widget->allocation.height;
	gdk_width    = alloc_width - priv->frame_border2;
	gdk_height   = alloc_height - priv->frame_border2;

	if (priv->image != NULL)
	{
		cairo_t *cr;
		cr = gdk_cairo_create (GTK_WIDGET (viewer)->window);

		/* Only actually paint the regions that need redrawn */
		gdk_cairo_region (cr, event->region);
		cairo_clip (cr);

		/* Paint the image */
		image_viewer_image_paint (priv->image, cr);

		cairo_destroy (cr);
	}

	priv->rendering = FALSE;

	/* Draw the focus. */

#if 0
	if (GTK_WIDGET_HAS_FOCUS (widget)) {
		GdkRectangle r;

		r.x = 0;
		r.y = 0;
		r.width = gdk_width + 2;
		r.height = gdk_height + 2;

		gtk_paint_focus (widget->style,
				 widget->window,
				 widget->state,
				 &r,
				 widget, NULL,
				 0, 0, gdk_width + 2, gdk_height + 2);
	}
#endif

	return FALSE;
}


void
image_viewer_scroll_to (ImageViewer *viewer,
			int          x_offset,
			int          y_offset)
{
	ImageViewerPrivate  *priv;
	gdouble x, y;

	g_return_if_fail (viewer != NULL);

	priv = IMAGE_VIEWER_GET_PRIVATE (viewer);

	g_return_if_fail (priv->image != NULL);

	if (priv->rendering)
		return;

	image_viewer_image_get_scroll_offsets (priv->image, &x, &y);

	/* Convert absolute offset to relative change */
	x = x_offset - x;
	y = y_offset - y;

	scroll_relative (viewer, x, y);
}

static void
scroll_relative (ImageViewer *viewer,
		 gdouble      delta_x,
		 gdouble      delta_y)
{
	ImageViewerPrivate *priv = IMAGE_VIEWER_GET_PRIVATE (viewer);
	gdouble             orig_x, orig_y;

	orig_x = gtk_adjustment_get_value (priv->hadj);
	orig_y = gtk_adjustment_get_value (priv->vadj);

	/* Get offsets after clamping */
	delta_x = CLAMP (orig_x + delta_x, priv->hadj->lower, priv->hadj->upper - priv->hadj->page_size) - orig_x;
	delta_y = CLAMP (orig_y + delta_y, priv->vadj->lower, priv->vadj->upper - priv->vadj->page_size) - orig_y;

	g_signal_handlers_block_by_data (G_OBJECT (priv->hadj), viewer);
	g_signal_handlers_block_by_data (G_OBJECT (priv->vadj), viewer);
	gtk_adjustment_set_value (priv->hadj, orig_x + delta_x);
	gtk_adjustment_set_value (priv->vadj, orig_y + delta_y);
	g_signal_handlers_unblock_by_data (G_OBJECT (priv->hadj), viewer);
	g_signal_handlers_unblock_by_data (G_OBJECT (priv->vadj), viewer);

	scroll_relative_no_scrollbar (viewer, delta_x, delta_y);
}

static void
scroll_relative_no_scrollbar (ImageViewer *viewer,
			      gdouble      delta_x,
			      gdouble      delta_y)
{
	ImageViewerPrivate *priv     = IMAGE_VIEWER_GET_PRIVATE (viewer);

	image_viewer_image_scroll_relative (priv->image, delta_x, delta_y);

	gtk_widget_queue_draw (GTK_WIDGET (viewer));
}


static gint
image_viewer_button_press (GtkWidget      *widget,
			   GdkEventButton *event)
{
	ImageViewer *viewer = IMAGE_VIEWER (widget);
	ImageViewerPrivate  *priv = IMAGE_VIEWER_GET_PRIVATE (viewer);

	if (! GTK_WIDGET_HAS_FOCUS (widget)) {
		gtk_widget_grab_focus (widget);
		priv->just_focused = TRUE;
	}

	if (priv->dragging)
		return FALSE;

	if ((event->type == GDK_2BUTTON_PRESS) ||
	    (event->type == GDK_3BUTTON_PRESS)) {
		priv->double_click = TRUE;
		return FALSE;
	} else
		priv->double_click = FALSE;

	if (event->button == 1) {
		GdkCursor *cursor;
		int        retval;

		cursor = cursor_get (widget->window, CURSOR_HAND_CLOSED);
		retval = gdk_pointer_grab (widget->window,
					   FALSE,
					   (GDK_POINTER_MOTION_MASK
					    | GDK_POINTER_MOTION_HINT_MASK
					    | GDK_BUTTON_RELEASE_MASK),
					   NULL,
					   cursor,
					   event->time);
		gdk_cursor_unref (cursor);

		if (retval != 0)
			return FALSE;

		priv->drag_realx = priv->drag_x = event->x;
		priv->drag_realy = priv->drag_y = event->y;
		priv->pressed = TRUE;

		return TRUE;
	}

	return FALSE;
}


static gint
image_viewer_button_release  (GtkWidget *widget,
			      GdkEventButton *event)
{
	ImageViewer *viewer = IMAGE_VIEWER (widget);
	ImageViewerPrivate  *priv = IMAGE_VIEWER_GET_PRIVATE (viewer);

	if (event->button != 1) {
		priv->just_focused = FALSE;
		return FALSE;
	}

	gdk_pointer_ungrab (event->time);

	if (! priv->dragging
	    && ! priv->double_click
	    && ! priv->just_focused) {
		g_signal_emit (G_OBJECT (viewer),
			       image_viewer_signals[CLICKED],
			       0);

	}

	priv->just_focused = FALSE;
	priv->pressed = FALSE;
	priv->dragging = FALSE;

	return FALSE;
}


static gint
image_viewer_motion_notify (GtkWidget *widget,
			    GdkEventMotion *event)
{
	ImageViewer     *viewer = IMAGE_VIEWER (widget);
	ImageViewerPrivate  *priv = IMAGE_VIEWER_GET_PRIVATE (viewer);
	GdkModifierType  mods;
	gint             x, y;

	if (! priv->pressed)
		return FALSE;

	if (priv->rendering)
		return FALSE;

	priv->dragging = TRUE;

	if (event->is_hint)
                gdk_window_get_pointer (widget->window, &x, &y, &mods);
        else
                return FALSE;

	priv->drag_realx = x;
	priv->drag_realy = y;

	if ((abs (priv->drag_realx - priv->drag_x) < DRAG_THRESHOLD)
	    && (abs (priv->drag_realy - priv->drag_y) < DRAG_THRESHOLD))
		return FALSE;

	scroll_relative (viewer,
			 priv->drag_x - priv->drag_realx,
			 priv->drag_y - priv->drag_realy);

	priv->drag_x = priv->drag_realx;
	priv->drag_y = priv->drag_realy;

	return FALSE;
}


static void
image_viewer_finalize (GObject *object)
{
        ImageViewer        *viewer;
	ImageViewerPrivate *priv;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_IMAGE_VIEWER (object));

        viewer = IMAGE_VIEWER (object);
	priv = IMAGE_VIEWER_GET_PRIVATE (viewer);

	if (priv->loader != NULL) {
		g_object_unref (priv->loader);
		priv->loader = NULL;
	}

	if (priv->image != NULL) {
		g_object_unref (G_OBJECT (priv->image));
		priv->image = NULL;
	}

	if (priv->cursor != NULL) {
		gdk_cursor_unref (priv->cursor);
		priv->cursor = NULL;
	}

	if (priv->cursor_void != NULL) {
		gdk_cursor_unref (priv->cursor_void);
		priv->cursor_void = NULL;
	}

	if (priv->hadj != NULL) {
		g_signal_handlers_disconnect_by_data (G_OBJECT (priv->hadj),
						      viewer);
		g_object_unref (priv->hadj);
		priv->hadj = NULL;
	}
	if (priv->vadj != NULL) {
		g_signal_handlers_disconnect_by_data (G_OBJECT (priv->vadj),
						      viewer);
		g_object_unref (priv->vadj);
		priv->vadj = NULL;
	}

	/*if (priv->area_pixbuf != NULL) {
		g_object_unref (priv->area_pixbuf);
		priv->area_pixbuf = NULL;
	}*/

        /* Chain up */
	G_OBJECT_CLASS (image_viewer_parent_class)->finalize (object);
}



static void
image_viewer_realize (GtkWidget *widget)
{
	ImageViewer         *viewer;
	ImageViewerPrivate  *priv;
	GdkWindowAttr  attributes;
	int            attributes_mask;

	g_return_if_fail (IS_IMAGE_VIEWER (widget));

	viewer = IMAGE_VIEWER (widget);
	priv = IMAGE_VIEWER_GET_PRIVATE (viewer);

	GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.x           = widget->allocation.x;
	attributes.y           = widget->allocation.y;
	attributes.width       = widget->allocation.width;
	attributes.height      = widget->allocation.height;
	attributes.wclass      = GDK_INPUT_OUTPUT;
	attributes.visual      = gtk_widget_get_visual (widget);
	attributes.colormap    = gtk_widget_get_colormap (widget);
	attributes.event_mask  = (gtk_widget_get_events (widget)
				  | GDK_EXPOSURE_MASK
				  | GDK_BUTTON_PRESS_MASK
				  | GDK_BUTTON_RELEASE_MASK
				  | GDK_POINTER_MOTION_MASK
				  | GDK_POINTER_MOTION_HINT_MASK
				  | GDK_BUTTON_MOTION_MASK);

	attributes_mask        = (GDK_WA_X
				  | GDK_WA_Y
				  | GDK_WA_VISUAL
				  | GDK_WA_COLORMAP);
	widget->window = gdk_window_new (gtk_widget_get_parent_window (widget),
					 &attributes,
					 attributes_mask);
	gdk_window_set_user_data (widget->window, viewer);

	widget->style = gtk_style_attach (widget->style, widget->window);
	gtk_style_set_background (widget->style, widget->window, GTK_STATE_NORMAL);

	priv->cursor = cursor_get (widget->window, CURSOR_HAND_OPEN);
	priv->cursor_void = cursor_get (widget->window, CURSOR_VOID);
	gdk_window_set_cursor (widget->window, priv->cursor);
}


static void
image_viewer_unrealize (GtkWidget *widget)
{
	ImageViewer         *viewer;
	ImageViewerPrivate  *priv;

	g_return_if_fail (IS_IMAGE_VIEWER (widget));

	viewer = IMAGE_VIEWER (widget);
	priv = IMAGE_VIEWER_GET_PRIVATE (viewer);

	if (priv->cursor) {
		gdk_cursor_unref (priv->cursor);
		priv->cursor = NULL;
	}
	if (priv->cursor_void) {
		gdk_cursor_unref (priv->cursor_void);
		priv->cursor_void = NULL;
	}

	GTK_WIDGET_CLASS (image_viewer_parent_class)->unrealize (widget);
}


GdkPixbuf*
image_viewer_get_current_pixbuf (ImageViewer *viewer)
{
	ImageViewerPrivate  *priv;
	g_return_val_if_fail (viewer != NULL, NULL);

	priv = IMAGE_VIEWER_GET_PRIVATE (viewer);

	if (priv->image == NULL) return NULL;

	return image_viewer_image_get_pixbuf (priv->image);
}


static void
image_viewer_size_allocate (GtkWidget       *widget,
			    GtkAllocation   *allocation)
{
	ImageViewer        *viewer = IMAGE_VIEWER (widget);
	ImageViewerPrivate *priv = IMAGE_VIEWER_GET_PRIVATE (viewer);
	gdouble             gdk_width, gdk_height;

	widget->allocation = *allocation;
	gdk_width = allocation->width - priv->frame_border2;
	gdk_height = allocation->height - priv->frame_border2;

	/* If a fit type is active update the zoom level. */

	if (priv->image != NULL) {
		gdouble    width, height;
		gdouble	   off_x, off_y;

		/* Re-fit the image to the new widget size if a fit-mode is active */
		image_viewer_image_set_fit_mode (priv->image,
				image_viewer_image_get_fit_mode (priv->image));

		image_viewer_image_get_scroll_offsets (priv->image, &off_x, &off_y);

		image_viewer_image_get_zoomed_size (priv->image, &width, &height);

		if (!FLOAT_EQUAL(width,  priv->hadj->upper - priv->hadj->lower) ||
		    !FLOAT_EQUAL(height, priv->vadj->upper - priv->vadj->lower))
			gth_iviewer_size_changed (GTH_IVIEWER (viewer));

		/* Change adjustment values. */

		priv->hadj->lower          = floor (-width / 2 - priv->frame_border);
		priv->hadj->upper          = floor ( width / 2 + priv->frame_border);
		priv->hadj->value          = floor (off_x - gdk_width / 2);
		priv->hadj->step_increment = STEP_INCREMENT;
		priv->hadj->page_increment = floor (gdk_width / 2);
		priv->hadj->page_size      = gdk_width;

		priv->vadj->lower          = floor (-height / 2 - priv->frame_border);
		priv->vadj->upper          = floor ( height / 2 + priv->frame_border);
		priv->vadj->value          = floor ( off_y - gdk_height / 2);
		priv->vadj->step_increment = STEP_INCREMENT;
		priv->vadj->page_increment = floor (gdk_height / 2);
		priv->vadj->page_size      = gdk_height;
	} else {
		priv->hadj->lower     = 0.0;
		priv->hadj->upper     = 1.0;
		priv->hadj->value     = 0.0;
		priv->hadj->page_size = 1.0;

		priv->vadj->lower     = 0.0;
		priv->vadj->upper     = 1.0;
		priv->vadj->value     = 0.0;
		priv->vadj->page_size = 1.0;
	}

	g_signal_handlers_block_by_data (G_OBJECT (priv->hadj), viewer);
	g_signal_handlers_block_by_data (G_OBJECT (priv->vadj), viewer);
	gtk_adjustment_changed (priv->hadj);
	gtk_adjustment_changed (priv->vadj);
	g_signal_handlers_unblock_by_data (G_OBJECT (priv->hadj), viewer);
	g_signal_handlers_unblock_by_data (G_OBJECT (priv->vadj), viewer);

	/**/

	if (GTK_WIDGET_REALIZED (widget))
		gdk_window_move_resize (widget->window,
					allocation->x, allocation->y,
					allocation->width, allocation->height);

	if (! priv->skip_size_change)
		gth_iviewer_size_changed (GTH_IVIEWER (viewer));
	else
		priv->skip_size_change = FALSE;
}


static gint
image_viewer_focus_in (GtkWidget     *widget,
		       GdkEventFocus *event)
{
	GTK_WIDGET_SET_FLAGS (widget, GTK_HAS_FOCUS);
	gtk_widget_queue_draw (widget);
	return TRUE;
}


static gint
image_viewer_focus_out (GtkWidget     *widget,
			GdkEventFocus *event)
{
	GTK_WIDGET_UNSET_FLAGS (widget, GTK_HAS_FOCUS);
	gtk_widget_queue_draw (widget);
	return TRUE;
}


static void
set_zoom (ImageViewer *viewer,
	  gdouble      zoom_level,
	  int          center_x,
	  int          center_y)
{
	ImageViewerPrivate *priv = IMAGE_VIEWER_GET_PRIVATE (viewer);

	image_viewer_image_zoom_at_point (priv->image, zoom_level, center_x, center_y);

	if (priv->fit != GTH_FIT_NONE) {
		/*gtk_widget_queue_resize (GTK_WIDGET (viewer));*/
		gtk_widget_queue_draw (GTK_WIDGET (viewer));
	}

	if (! priv->skip_zoom_change)
		g_signal_emit (G_OBJECT (viewer),
			       image_viewer_signals[ZOOM_CHANGED],
			       0);
	else
		priv->skip_zoom_change = FALSE;
}


static void
zoom_changed_cb (ImageViewerImage *image,
		 gpointer          data)
{
	ImageViewer        *viewer = data;
	ImageViewerPrivate *priv  = IMAGE_VIEWER_GET_PRIVATE (viewer);

	if (! priv->skip_zoom_change)
		g_signal_emit (G_OBJECT (viewer),
			       image_viewer_signals[ZOOM_CHANGED],
			       0);
	else
		priv->skip_zoom_change = FALSE;
}


void
image_viewer_set_zoom (ImageViewer *viewer,
		       gdouble      zoom_level)
{
	GtkWidget *widget = (GtkWidget*) viewer;
	ImageViewerPrivate* priv = IMAGE_VIEWER_GET_PRIVATE (viewer);

	set_zoom (viewer,
		  zoom_level,
		  (widget->allocation.width - priv->frame_border2) / 2,
		  (widget->allocation.height - priv->frame_border2) / 2);
}


gdouble
image_viewer_get_zoom (ImageViewer *viewer)
{
	ImageViewerPrivate* priv;

	g_return_val_if_fail (viewer != NULL, 0);

	priv = IMAGE_VIEWER_GET_PRIVATE (viewer);

	if (priv->image == NULL) return 1.0;

	return image_viewer_image_get_zoom_level (priv->image);
}


static gint
image_viewer_scroll_event (GtkWidget        *widget,
			   GdkEventScroll   *event)
{
	ImageViewer         *viewer = IMAGE_VIEWER (widget);
	ImageViewerPrivate *priv;
	GtkAdjustment *adj;
	gdouble        new_value = 0.0;

	g_return_val_if_fail (IS_IMAGE_VIEWER (widget), FALSE);
	g_return_val_if_fail (event != NULL, FALSE);

	priv = IMAGE_VIEWER_GET_PRIVATE (viewer);

	if (event->state & GDK_CONTROL_MASK) {
		if (event->direction == GDK_SCROLL_UP) {
			image_viewer_image_zoom_in_at_point(
					priv->image,
					event->x,
					event->y);
			return TRUE;
		}
		if (event->direction == GDK_SCROLL_DOWN) {
			image_viewer_image_zoom_out_at_point(
					priv->image,
					event->x,
					event->y);
			return TRUE;
		}
	}

	if (event->direction == GDK_SCROLL_UP || event->direction == GDK_SCROLL_DOWN) {
		g_signal_emit (G_OBJECT (viewer),
			            image_viewer_signals[MOUSE_WHEEL_SCROLL],
		 	            0, event->direction);
		return TRUE;
	}

	adj = priv->hadj;

	if (event->direction == GDK_SCROLL_LEFT)
		new_value = adj->value - adj->page_increment / 2;
	else if (event->direction == GDK_SCROLL_RIGHT)
		new_value = adj->value + adj->page_increment / 2;

	new_value = CLAMP (new_value, adj->lower, adj->upper - adj->page_size);
	gtk_adjustment_set_value (adj, new_value);

	return TRUE;
}


void
image_viewer_get_adjustments (GthIViewer     *self,
			      GtkAdjustment **hadj,
			      GtkAdjustment **vadj)
{
	ImageViewer *viewer = IMAGE_VIEWER (self);
	ImageViewerPrivate  *priv = IMAGE_VIEWER_GET_PRIVATE (viewer);
	if (hadj != NULL)
		*hadj = priv->hadj;
	if (vadj != NULL)
		*vadj = priv->vadj;
}


static void
image_viewer_zoom_to_fit (ImageViewer *viewer)
{
	image_viewer_set_fit_mode (viewer, GTH_FIT_SIZE_IF_LARGER);
}


static void
gth_iviewer_interface_init (gpointer   g_iface,
			    gpointer   iface_data)
{
	GthIViewerInterface *iface = (GthIViewerInterface *)g_iface;

	iface->get_zoom = (double (*) (GthIViewer *)) image_viewer_get_zoom;
	iface->set_zoom = (void (*) (GthIViewer *, double)) image_viewer_set_zoom;
	iface->zoom_in = (void (*) (GthIViewer *)) image_viewer_zoom_in;
	iface->zoom_out = (void (*) (GthIViewer *)) image_viewer_zoom_out;
	iface->zoom_to_fit = (void (*) (GthIViewer *)) image_viewer_zoom_to_fit;
	iface->get_image = (GdkPixbuf * (*) (GthIViewer *)) image_viewer_get_current_pixbuf;
	iface->get_adjustments = (void (*) (GthIViewer *, GtkAdjustment **, GtkAdjustment **)) image_viewer_get_adjustments;
}

GtkWidget*
image_viewer_new (void)
{
	return GTK_WIDGET (g_object_new (IMAGE_VIEWER_TYPE, NULL));
}


/* -- image_viewer_load_image -- */


typedef struct {
	ImageViewer *viewer;
	FileData    *file;
} LoadImageData;


static void
load_image_data_free (LoadImageData *lidata)
{
	file_data_unref (lidata->file);
	g_free (lidata);
}


void
load_image__step2 (LoadImageData *lidata)
{
	ImageViewerPrivate* priv = IMAGE_VIEWER_GET_PRIVATE (lidata->viewer);
	FileData *file;
	
	file_data_update_mime_type (file, FALSE);  /* always slow mime type ? */
	image_loader_set_file (priv->loader, file);
	
	image_loader_start (priv->loader);
	load_image_data_free (lidata);
}


void
image_viewer_load_image_from_uri (ImageViewer *viewer,
			          const char  *path)
{
	ImageViewerPrivate* priv;
	LoadImageData *lidata;

	g_return_if_fail (viewer != NULL);
	g_return_if_fail (path != NULL);

	priv = IMAGE_VIEWER_GET_PRIVATE (viewer);

	lidata = g_new (LoadImageData, 1);
	lidata->viewer = viewer;
	lidata->file = file_data_new (path, NULL);
	image_loader_stop (priv->loader, (DoneFunc) load_image__step2, lidata);
}


void
image_viewer_load_image (ImageViewer *viewer,
			 FileData    *file)
{
	LoadImageData *lidata;
	ImageViewerPrivate* priv = IMAGE_VIEWER_GET_PRIVATE (viewer);

	g_return_if_fail (viewer != NULL);
	g_return_if_fail (file != NULL);

	lidata = g_new (LoadImageData, 1);
	lidata->viewer = viewer;
	lidata->file = file_data_ref (file);
	image_loader_stop (priv->loader, (DoneFunc) load_image__step2, lidata);
}


/* -- image_viewer_load_from_pixbuf_loader -- */


typedef struct {
	ImageViewer *viewer;
	gpointer     data;
} ImageViewerLoadData;


void
load_from_pixbuf_loader__step2 (ImageViewerLoadData *ivl_data)
{
	ImageViewerPrivate* priv = IMAGE_VIEWER_GET_PRIVATE (ivl_data->viewer);
	GdkPixbufLoader *pixbuf_loader = ivl_data->data;

	image_loader_load_from_pixbuf_loader (priv->loader, pixbuf_loader);
	g_object_unref (pixbuf_loader);
	g_free (ivl_data);
}


void
image_viewer_load_from_pixbuf_loader (ImageViewer *viewer,
				      GdkPixbufLoader *pixbuf_loader)
{
	ImageViewerLoadData *ivl_data;
	ImageViewerPrivate  *priv;

	g_return_if_fail (viewer != NULL);
	g_return_if_fail (pixbuf_loader != NULL);

	priv = IMAGE_VIEWER_GET_PRIVATE (viewer);

	g_object_ref (pixbuf_loader);

	ivl_data = g_new (ImageViewerLoadData, 1);
	ivl_data->viewer = viewer;
	ivl_data->data = pixbuf_loader;
	image_loader_stop (priv->loader,
			   (DoneFunc) load_from_pixbuf_loader__step2,
			   ivl_data);
}


/* -- image_viewer_load_from_image_loader -- */


void
load_from_image_loader__step2 (ImageViewerLoadData *ivl_data)
{
	ImageViewerPrivate* priv = IMAGE_VIEWER_GET_PRIVATE (ivl_data->viewer);
	ImageLoader *image_loader = ivl_data->data;

	image_loader_load_from_image_loader (priv->loader, image_loader);
	g_object_unref (image_loader);
	g_free (ivl_data);
}


void
image_viewer_load_from_image_loader (ImageViewer *viewer,
				     ImageLoader *image_loader)
{
	ImageViewerPrivate* priv = IMAGE_VIEWER_GET_PRIVATE (viewer);
	ImageViewerLoadData *ivl_data;

	g_return_if_fail (viewer != NULL);
	g_return_if_fail (image_loader != NULL);

	ivl_data = g_new0 (ImageViewerLoadData, 1);
	ivl_data->viewer = viewer;
	ivl_data->data = image_loader;
	g_object_ref (image_loader);
	
	image_loader_stop (priv->loader,
			   (DoneFunc) load_from_image_loader__step2,
			   ivl_data);
}


static void
set_pixbuf__step2 (ImageViewerLoadData *ivl_data)
{
	ImageViewerPrivate *priv   = IMAGE_VIEWER_GET_PRIVATE (ivl_data->viewer);
	GdkPixbuf          *pixbuf = ivl_data->data;

	image_loader_set_file   (priv->loader, NULL);
	image_loader_set_pixbuf (priv->loader, pixbuf);
	create_image (priv->loader, ivl_data->viewer);

	g_object_unref (G_OBJECT (pixbuf));
	g_free (ivl_data);

}


void
image_viewer_set_pixbuf (ImageViewer *viewer,
			 GdkPixbuf   *pixbuf)
{
	ImageViewerPrivate  *priv;
	ImageViewerLoadData *ivl_data;

	g_return_if_fail (viewer != NULL);

	priv = IMAGE_VIEWER_GET_PRIVATE (viewer);

	ivl_data = g_new0 (ImageViewerLoadData, 1);
	ivl_data->viewer = viewer;
	ivl_data->data = pixbuf;
	g_object_ref (G_OBJECT (pixbuf));

	image_loader_stop (priv->loader,
			   (DoneFunc) set_pixbuf__step2,
			   ivl_data);

}


void
image_viewer_set_void (ImageViewer *viewer)
{
	ImageViewerPrivate* priv;

	g_return_if_fail (viewer != NULL);

	priv = IMAGE_VIEWER_GET_PRIVATE (viewer);

	g_object_unref (priv->image);
	priv->image = NULL;

	gtk_widget_queue_resize (GTK_WIDGET (viewer));
}


gboolean
image_viewer_is_void (ImageViewer *viewer)
{
	ImageViewerPrivate *priv;

	g_return_val_if_fail (viewer != NULL, TRUE);

	priv = IMAGE_VIEWER_GET_PRIVATE (viewer);

	return (priv->image == NULL);
}


void
image_viewer_update_view (ImageViewer *viewer)
{
	g_return_if_fail (viewer != NULL);

	gtk_widget_queue_draw (GTK_WIDGET (viewer));
}


char*
image_viewer_get_image_filename (ImageViewer *viewer)
{
	ImageViewerPrivate *priv;

	g_return_val_if_fail (viewer != NULL, NULL);

	priv = IMAGE_VIEWER_GET_PRIVATE (viewer);

	if (priv->image == NULL) return NULL;

	return image_viewer_image_get_path (priv->image);
}


int
image_viewer_get_image_width (ImageViewer *viewer)
{
	ImageViewerPrivate *priv;

	g_return_val_if_fail (viewer != NULL, 0);

	priv = IMAGE_VIEWER_GET_PRIVATE (viewer);

	if (priv->image == NULL) return 0;

	return image_viewer_image_get_width (priv->image);
}


gint
image_viewer_get_image_height (ImageViewer *viewer)
{
	ImageViewerPrivate *priv;

	g_return_val_if_fail (viewer != NULL, 0);

	priv = IMAGE_VIEWER_GET_PRIVATE (viewer);

	if (priv->image == NULL) return 0;

	return image_viewer_image_get_height (priv->image);
}


gint
image_viewer_get_image_bps (ImageViewer *viewer)
{
	ImageViewerPrivate *priv;

	g_return_val_if_fail (viewer != NULL, 0);

	priv = IMAGE_VIEWER_GET_PRIVATE (viewer);

	if (priv->image == NULL) return 0;

	return image_viewer_image_get_bps (priv->image);
}


gboolean
image_viewer_get_has_alpha (ImageViewer *viewer)
{
	ImageViewerPrivate *priv;

	g_return_val_if_fail (viewer != NULL, 0);

	priv = IMAGE_VIEWER_GET_PRIVATE (viewer);

	if (priv->image == NULL) return FALSE;

	return image_viewer_image_get_has_alpha (priv->image);
}


void
image_viewer_start_animation (ImageViewer *viewer)
{
	ImageViewerPrivate *priv;

	g_return_if_fail (viewer != NULL);

	priv = IMAGE_VIEWER_GET_PRIVATE (viewer);

	priv->play_animation = TRUE;
	image_viewer_image_start_animation (priv->image);
}


void
image_viewer_stop_animation (ImageViewer *viewer)
{
	ImageViewerPrivate *priv;

	g_return_if_fail (viewer != NULL);

	priv = IMAGE_VIEWER_GET_PRIVATE (viewer);

	priv->play_animation = FALSE;
	image_viewer_image_stop_animation (priv->image);
}


void
image_viewer_step_animation (ImageViewer *viewer)
{
	ImageViewerPrivate *priv;

	g_return_if_fail (viewer != NULL);

	priv = IMAGE_VIEWER_GET_PRIVATE (viewer);

	g_return_if_fail (priv->play_animation == FALSE);
	g_return_if_fail (priv->rendering == FALSE);

	image_viewer_image_step_animation (priv->image);
}


gboolean
image_viewer_get_play_animation (ImageViewer *viewer)
{
	ImageViewerPrivate *priv;

	g_return_val_if_fail (viewer != NULL, FALSE);

	priv = IMAGE_VIEWER_GET_PRIVATE (viewer);

	return priv->play_animation;
}


gboolean
image_viewer_is_animation (ImageViewer *viewer)
{
	ImageViewerPrivate *priv;

	g_return_val_if_fail (viewer != NULL, FALSE);

	priv = IMAGE_VIEWER_GET_PRIVATE (viewer);

	if (priv->image == NULL) return FALSE;

	return image_viewer_image_get_is_animation(priv->image);
}


gboolean
image_viewer_is_playing_animation (ImageViewer *viewer)
{
	ImageViewerPrivate *priv;

	g_return_val_if_fail (viewer != NULL, FALSE);

	priv = IMAGE_VIEWER_GET_PRIVATE (viewer);

	return image_viewer_is_animation (viewer) &&
			priv->play_animation;
}


static void
image_error (ImageLoader *il,
	     gpointer data)
{
	ImageViewer *viewer = data;

	image_viewer_set_void (viewer);

	g_signal_emit (G_OBJECT (viewer),
		       image_viewer_signals[IMAGE_ERROR],
		       0);
}


static void
image_progress (ImageLoader *il,
		float        progress,
		gpointer     data)
{
	ImageViewer *viewer = data;

	g_signal_emit (G_OBJECT (viewer),
		       image_viewer_signals[IMAGE_PROGRESS],
		       0,
		       progress);
}


static void
image_loaded (ImageLoader *il,
	      gpointer data)
{
	ImageViewer        *viewer = data;

	create_image (il, data);

	g_signal_emit (G_OBJECT (viewer),
		       image_viewer_signals[IMAGE_LOADED],
		       0);
}


static void
create_image (ImageLoader *il,
	      gpointer data)
{
	ImageViewer        *viewer = data;
	ImageViewerPrivate *priv = IMAGE_VIEWER_GET_PRIVATE (viewer);
	ImageViewerImage   *next;

	next = image_viewer_image_new (viewer, priv->loader);

	if (priv->reset_scrollbars == FALSE && priv->image != NULL) {
		gdouble x_ofs, y_ofs;
		image_viewer_image_get_scroll_offsets (priv->image, &x_ofs, &y_ofs);
		image_viewer_image_set_scroll_offsets (next, x_ofs, y_ofs);
	}

	switch (priv->zoom_change) {
	case GTH_ZOOM_CHANGE_ACTUAL_SIZE:
		break;

	case GTH_ZOOM_CHANGE_KEEP_PREV:
		if (priv->image != NULL)
			image_viewer_image_zoom_centered (next,
				image_viewer_image_get_zoom_level (priv->image));
		break;

	case GTH_ZOOM_CHANGE_FIT_SIZE:
		image_viewer_image_set_fit_mode (next, GTH_FIT_SIZE);
		break;

	case GTH_ZOOM_CHANGE_FIT_SIZE_IF_LARGER:
		image_viewer_image_set_fit_mode (next, GTH_FIT_SIZE_IF_LARGER);
		break;

	case GTH_ZOOM_CHANGE_FIT_WIDTH:
		image_viewer_image_set_fit_mode (next, GTH_FIT_WIDTH);
		break;

	case GTH_ZOOM_CHANGE_FIT_WIDTH_IF_LARGER:
		image_viewer_image_set_fit_mode (next, GTH_FIT_WIDTH_IF_LARGER);
		break;
	}

	if (priv->play_animation)
		image_viewer_image_start_animation (next);

	if (priv->image != NULL)
		g_object_unref (priv->image);

	priv->image = next;

	g_signal_connect (G_OBJECT (next), "zoom_changed",
			  G_CALLBACK (zoom_changed_cb), viewer);

	gtk_widget_queue_resize (GTK_WIDGET (viewer));
}


void
image_viewer_set_zoom_quality (ImageViewer    *viewer,
			       GthZoomQuality  quality)
{
	ImageViewerPrivate *priv;

	g_return_if_fail (viewer != NULL);

	priv = IMAGE_VIEWER_GET_PRIVATE (viewer);

	priv->zoom_quality = quality;
}


GthZoomQuality
image_viewer_get_zoom_quality (ImageViewer *viewer)
{
	ImageViewerPrivate *priv;

	g_return_val_if_fail (viewer != NULL, -1);

	priv = IMAGE_VIEWER_GET_PRIVATE (viewer);

	return priv->zoom_quality;
}


void
image_viewer_set_zoom_change (ImageViewer   *viewer,
			      GthZoomChange  zoom_change)
{
	ImageViewerPrivate *priv;

	g_return_if_fail (viewer != NULL);

	priv = IMAGE_VIEWER_GET_PRIVATE (viewer);

	if (zoom_change == priv->zoom_change)
		return;

	priv->zoom_change = zoom_change;
}


GthZoomChange
image_viewer_get_zoom_change (ImageViewer *viewer)
{
	ImageViewerPrivate *priv;

	g_return_val_if_fail (viewer != NULL, -1);

	priv = IMAGE_VIEWER_GET_PRIVATE (viewer);

	return priv->zoom_change;
}


void
image_viewer_zoom_in (ImageViewer *viewer)
{
	ImageViewerPrivate *priv;

	g_return_if_fail (viewer != NULL);

	priv = IMAGE_VIEWER_GET_PRIVATE (viewer);

	g_return_if_fail (priv->image != NULL);

	image_viewer_image_zoom_in_centered (priv->image);
}


void
image_viewer_zoom_out (ImageViewer *viewer)
{
	ImageViewerPrivate *priv;

	g_return_if_fail (viewer != NULL);

	priv = IMAGE_VIEWER_GET_PRIVATE (viewer);

	g_return_if_fail (priv->image != NULL);

	image_viewer_image_zoom_out_centered (priv->image);
}


void
image_viewer_set_fit_mode (ImageViewer     *viewer,
			   GthFit           fit_mode)
{
	ImageViewerPrivate *priv;

	g_return_if_fail (viewer != NULL);

	priv = IMAGE_VIEWER_GET_PRIVATE (viewer);

	priv->fit = fit_mode;

	if (priv->image)
	{
		image_viewer_image_set_fit_mode (priv->image, fit_mode);

		/*if (fit_mode != GTH_FIT_NONE)
			gtk_widget_queue_resize (GTK_WIDGET (viewer));*/
	}
}


void
image_viewer_set_transp_type (ImageViewer   *viewer,
			      GthTranspType  transp_type)
{
	ImageViewerPrivate *priv;

	g_return_if_fail (viewer != NULL);

	priv = IMAGE_VIEWER_GET_PRIVATE (viewer);

	priv->transp_type = transp_type;
}


GthTranspType
image_viewer_get_transp_type (ImageViewer *viewer)
{
	ImageViewerPrivate *priv;

	g_return_val_if_fail (viewer != NULL, FALSE);

	priv = IMAGE_VIEWER_GET_PRIVATE (viewer);
	
	return priv->transp_type;
}


void
image_viewer_set_check_type (ImageViewer  *viewer,
			     GthCheckType  check_type)
{
	ImageViewerPrivate *priv;

	g_return_if_fail (viewer != NULL);

	priv = IMAGE_VIEWER_GET_PRIVATE (viewer);

	priv->check_type = check_type;
}


GthCheckType
image_viewer_get_check_type (ImageViewer *viewer)
{
	ImageViewerPrivate *priv;

	g_return_val_if_fail (viewer != NULL, -1);

	priv = IMAGE_VIEWER_GET_PRIVATE (viewer);

	return priv->check_type;
}


void
image_viewer_set_check_size (ImageViewer  *viewer,
			     GthCheckSize  check_size)
{
	ImageViewerPrivate *priv;

	g_return_if_fail (viewer != NULL);

	priv = IMAGE_VIEWER_GET_PRIVATE (viewer);

	priv->check_size = check_size;
}


GthCheckSize
image_viewer_get_check_size (ImageViewer *viewer)
{
	ImageViewerPrivate *priv;

	g_return_val_if_fail (viewer != NULL, FALSE);

	priv = IMAGE_VIEWER_GET_PRIVATE (viewer);

	return priv->check_size;
}


void
image_viewer_clicked (ImageViewer *viewer)
{
	g_return_if_fail (viewer != NULL);
	g_return_if_fail (IS_IMAGE_VIEWER (viewer));
	g_signal_emit (G_OBJECT (viewer), image_viewer_signals[CLICKED], 0);
}


/* set_scroll_adjustments handler for the image view */
static void
set_scroll_adjustments (GtkWidget *widget,
			GtkAdjustment *hadj,
			GtkAdjustment *vadj)
{
        ImageViewer *viewer;
	ImageViewerPrivate *priv;

        g_return_if_fail (widget != NULL);
        g_return_if_fail (IS_IMAGE_VIEWER (widget));

        viewer = IMAGE_VIEWER (widget);
        priv = IMAGE_VIEWER_GET_PRIVATE (viewer);

        if (hadj)
                g_return_if_fail (GTK_IS_ADJUSTMENT (hadj));
        else
                hadj = GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 0.0, 0.0,
							   0.0, 0.0, 0.0));

        if (vadj)
                g_return_if_fail (GTK_IS_ADJUSTMENT (vadj));
        else
                vadj = GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 0.0, 0.0,
							   0.0, 0.0, 0.0));

        if (priv->hadj && priv->hadj != hadj) {
		g_signal_handlers_disconnect_by_data (G_OBJECT (priv->hadj),
						      viewer);
		g_object_unref (priv->hadj);

		priv->hadj = NULL;
        }

        if (priv->vadj && priv->vadj != vadj) {
		g_signal_handlers_disconnect_by_data (G_OBJECT (priv->vadj),
						      viewer);
		g_object_unref (priv->vadj);
		priv->vadj = NULL;
        }

        if (priv->hadj != hadj) {
                priv->hadj = hadj;
                g_object_ref_sink (GTK_OBJECT (priv->hadj));

		g_signal_connect (G_OBJECT (priv->hadj),
				  "value_changed",
				  G_CALLBACK (hadj_value_changed),
				  viewer);
        }

        if (priv->vadj != vadj) {
		priv->vadj = vadj;
		g_object_ref_sink (GTK_OBJECT (priv->vadj));

		g_signal_connect (G_OBJECT (priv->vadj),
				  "value_changed",
				  G_CALLBACK (vadj_value_changed),
				  viewer);
        }
}

static void
scroll_signal (GtkWidget     *widget,
	       GtkScrollType  xscroll_type,
	       GtkScrollType  yscroll_type)
{
	ImageViewer        *viewer = IMAGE_VIEWER (widget);
	ImageViewerPrivate *priv = IMAGE_VIEWER_GET_PRIVATE (viewer);
	gdouble             xstep, ystep;

	switch (xscroll_type) {
	case GTK_SCROLL_STEP_LEFT:
		xstep = -priv->hadj->step_increment;
		break;
	case GTK_SCROLL_STEP_RIGHT:
		xstep = priv->hadj->step_increment;
		break;
	case GTK_SCROLL_PAGE_LEFT:
		xstep = -priv->hadj->page_increment;
		break;
	case GTK_SCROLL_PAGE_RIGHT:
		xstep = priv->hadj->page_increment;
		break;
	default:
		xstep = 0;
		break;
	}

	switch (yscroll_type) {
	case GTK_SCROLL_STEP_UP:
		ystep = -priv->vadj->step_increment;
		break;
	case GTK_SCROLL_STEP_DOWN:
		ystep = priv->vadj->step_increment;
		break;
	case GTK_SCROLL_PAGE_UP:
		ystep = -priv->vadj->page_increment;
		break;
	case GTK_SCROLL_PAGE_DOWN:
		ystep = priv->vadj->page_increment;
		break;
	default:
		ystep = 0;
		break;
	}

	scroll_relative (viewer, xstep, ystep);
}

void
image_viewer_scroll_step_x (ImageViewer *viewer,
			    gboolean     increment)
{
	ImageViewerPrivate *priv;

	g_return_if_fail (IS_IMAGE_VIEWER (viewer));

	priv = IMAGE_VIEWER_GET_PRIVATE (viewer);

	scroll_relative (viewer,
			 (increment ? 1 : -1) * priv->hadj->step_increment,
			 0);
}


void
image_viewer_scroll_step_y (ImageViewer *viewer,
			    gboolean     increment)
{
	ImageViewerPrivate* priv;

	g_return_if_fail (IS_IMAGE_VIEWER (viewer));

	priv = IMAGE_VIEWER_GET_PRIVATE (viewer);

	scroll_relative (viewer,
			 0,
			 (increment ? 1 : -1) * priv->vadj->step_increment);
}


void
image_viewer_scroll_page_x (ImageViewer *viewer,
			    gboolean     increment)
{
	ImageViewerPrivate *priv;

	g_return_if_fail (IS_IMAGE_VIEWER (viewer));

	priv = IMAGE_VIEWER_GET_PRIVATE (viewer);

	scroll_relative (viewer,
			 (increment ? 1 : -1) * priv->hadj->page_increment,
			 0);
}


void
image_viewer_scroll_page_y (ImageViewer *viewer,
			    gboolean     increment)
{
	ImageViewerPrivate* priv;

	g_return_if_fail (IS_IMAGE_VIEWER (viewer));

	priv = IMAGE_VIEWER_GET_PRIVATE (viewer);

	scroll_relative (viewer,
			 0,
			 (increment ? 1 : -1) * priv->vadj->page_increment);
}


void
image_viewer_get_scroll_offset  (ImageViewer *viewer,
				 int         *x,
				 int         *y)
{
	ImageViewerPrivate* priv;
	gdouble xd, yd;

	*x = *y = 0;

	g_return_if_fail (IS_IMAGE_VIEWER (viewer));

	priv = IMAGE_VIEWER_GET_PRIVATE (viewer);

	g_return_if_fail (priv->image != NULL);

	image_viewer_image_get_scroll_offsets (priv->image, &xd, &yd);

	*x = (int) xd;
	*y = (int) yd;
}


void
image_viewer_set_reset_scrollbars (ImageViewer *viewer,
  				   gboolean     reset)
{
	ImageViewerPrivate *priv = IMAGE_VIEWER_GET_PRIVATE (viewer);

	priv->reset_scrollbars = reset;
}


gboolean
image_viewer_get_reset_scrollbars (ImageViewer *viewer)
{
	ImageViewerPrivate *priv = IMAGE_VIEWER_GET_PRIVATE (viewer);
	return priv->reset_scrollbars;
}


void
image_viewer_size (ImageViewer *viewer,
		   int          width,
		   int          height)
{
	g_return_if_fail (IS_IMAGE_VIEWER (viewer));

	GTK_WIDGET (viewer)->requisition.width = width;
	GTK_WIDGET (viewer)->requisition.height = height;

	gtk_widget_queue_resize (GTK_WIDGET (viewer));
}


void
image_viewer_show_cursor (ImageViewer *viewer)
{
	ImageViewerPrivate *priv;

	g_return_if_fail (IS_IMAGE_VIEWER (viewer));

	priv = IMAGE_VIEWER_GET_PRIVATE (viewer);

	priv->cursor_visible = TRUE;
	gdk_window_set_cursor (GTK_WIDGET (viewer)->window, priv->cursor);
}


void
image_viewer_hide_cursor (ImageViewer *viewer)
{
	ImageViewerPrivate *priv;

	g_return_if_fail (IS_IMAGE_VIEWER (viewer));

	priv = IMAGE_VIEWER_GET_PRIVATE (viewer);

	priv->cursor_visible = FALSE;
	gdk_window_set_cursor (GTK_WIDGET (viewer)->window, priv->cursor_void);
}


gboolean
image_viewer_is_cursor_visible (ImageViewer *viewer)
{
	ImageViewerPrivate* priv;

	g_return_val_if_fail (viewer != NULL, FALSE);

	priv = IMAGE_VIEWER_GET_PRIVATE (viewer);

	return priv->cursor_visible;
}


void
image_viewer_set_black_background (ImageViewer *viewer,
				   gboolean     set_black)
{
	ImageViewerPrivate *priv;
	GtkWidget          *widget;

	g_return_if_fail (IS_IMAGE_VIEWER (viewer));

	priv   = IMAGE_VIEWER_GET_PRIVATE (viewer);
	widget = GTK_WIDGET (viewer);

	priv->black_bg = set_black;

	if (set_black) {
		gtk_widget_modify_bg (widget, GTK_STATE_NORMAL, &widget->style->black);
	} else {
		gtk_widget_modify_bg (widget, GTK_STATE_NORMAL, NULL);
	}

	gtk_widget_queue_draw (widget);
}


gboolean
image_viewer_is_black_background (ImageViewer *viewer)
{
	ImageViewerPrivate *priv;

	g_return_val_if_fail (viewer != NULL, FALSE);

	priv = IMAGE_VIEWER_GET_PRIVATE (viewer);
	return priv->black_bg;
}


void
image_viewer_show_frame (ImageViewer *viewer)
{
	ImageViewerPrivate *priv;

	g_return_if_fail (viewer != NULL);

	priv = IMAGE_VIEWER_GET_PRIVATE (viewer);

	priv->frame_visible = TRUE;
	priv->frame_border = FRAME_BORDER;
	priv->frame_border2 = FRAME_BORDER2;
	gtk_widget_queue_resize (GTK_WIDGET (viewer));
}


void
image_viewer_hide_frame (ImageViewer *viewer)
{
	ImageViewerPrivate *priv;

	g_return_if_fail (viewer != NULL);

	priv = IMAGE_VIEWER_GET_PRIVATE (viewer);

	priv->frame_visible = FALSE;
	priv->frame_border = 0;
	priv->frame_border2 = 0;
	gtk_widget_queue_resize (GTK_WIDGET (viewer));
}


gboolean
image_viewer_is_frame_visible (ImageViewer *viewer)
{
	ImageViewerPrivate *priv;

	g_return_val_if_fail (viewer != NULL, FALSE);

	priv = IMAGE_VIEWER_GET_PRIVATE (viewer);

	return priv->frame_visible;
}

