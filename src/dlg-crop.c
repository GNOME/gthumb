/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2003 Free Software Foundation, Inc.
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
#include <gtk/gtk.h>
#include <glade/glade.h>
#include <libgnomecanvas/libgnomecanvas.h>

#include "glib-utils.h"
#include "gthumb-window.h"
#include "gconf-utils.h"
#include "preferences.h"
#include "pixbuf-utils.h"


#define GLADE_FILE     "gthumb_crop.glade"
#define PREVIEW_SIZE   300


typedef struct {
	GThumbWindow  *window;
	GladeXML      *gui;

	double         zoom;

	double         image_x, image_y;
	double         image_width, image_height;
	GdkGC         *selection_gc;
	GdkRectangle   selection_area;

	GtkWidget     *dialog;
	GtkWidget     *crop_image;
	GtkSpinButton *crop_x_spinbutton;
	GtkSpinButton *crop_y_spinbutton;
	GtkSpinButton *crop_width_spinbutton;
	GtkSpinButton *crop_height_spinbutton;
} DialogData;


/* called when the main dialog is closed. */
static void
destroy_cb (GtkWidget  *widget, 
	    DialogData *data)
{
	g_object_unref (data->selection_gc);
	g_object_unref (data->gui);
	g_free (data);
}


/* called when the ok button is clicked. */
static void
ok_cb (GtkWidget  *widget, 
       DialogData *data)
{
	GdkPixbuf *old_pixbuf;
	GdkPixbuf *new_pixbuf;
	int        src_x, src_y, width, height;

	/* Save options */

	/**/

	g_print ("ZOOM: %f\n", data->zoom);

	old_pixbuf = image_viewer_get_current_pixbuf (IMAGE_VIEWER (data->window->viewer));

	src_x = MAX (0, ((double)data->selection_area.x - data->image_x) / data->zoom);
	src_y = MAX (0, ((double)data->selection_area.y - data->image_y) / data->zoom);
	width = MIN ((double)data->selection_area.width / data->zoom, gdk_pixbuf_get_width (old_pixbuf) - src_x);
	height = MIN ((double)data->selection_area.height / data->zoom, gdk_pixbuf_get_height (old_pixbuf) - src_y);

	new_pixbuf = gdk_pixbuf_new_subpixbuf (old_pixbuf, src_x, src_y, width, height);
	image_viewer_set_pixbuf (IMAGE_VIEWER (data->window->viewer), new_pixbuf);
	g_object_unref (new_pixbuf);
	window_image_set_modified (data->window, TRUE);

	gtk_widget_destroy (data->dialog);
}


static int
to_255 (int v)
{
	return v * 255 / 65535;
}


static void
paint_rubberband (DialogData   *data,
		  GtkWidget    *widget,
		  GdkRectangle *selection_area,
		  GdkRectangle *expose_area)
{
	GdkColor      color;
	guint32       rgba;
	GdkRectangle  rect;
	GdkPixbuf    *pixbuf;

	color = widget->style->base[GTK_STATE_SELECTED];
	rgba = ((to_255 (color.red) << 24) 
		| (to_255 (color.green) << 16) 
		| (to_255 (color.blue) << 8)
		| 0x00000040);

	if (! gdk_rectangle_intersect (expose_area, selection_area, &rect))
		return;

	pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, rect.width, rect.height);
	gdk_pixbuf_fill (pixbuf, rgba);
	gdk_pixbuf_render_to_drawable_alpha (pixbuf,
					     widget->window,
					     0, 0,
					     rect.x, rect.y,
					     rect.width, rect.height,
					     GDK_PIXBUF_ALPHA_FULL,
					     0,
					     GDK_RGB_DITHER_NONE,
					     0, 0);
	g_object_unref (pixbuf);

	/* Paint outline */

	gdk_gc_set_clip_rectangle (data->selection_gc, &rect);
	
	if ((selection_area->width > 1)
	    && (selection_area->height > 1)) 
		gdk_draw_rectangle (widget->window,
				    data->selection_gc,
				    FALSE,
				    selection_area->x + 1,
				    selection_area->y + 1,
				    selection_area->width - 2,
				    selection_area->height - 2);
}


static void
selection_x_value_changed_cb (GtkSpinButton *spin,
			      DialogData    *data)
{
	int value;

	value = gtk_spin_button_get_value_as_int (spin);
	value = CLAMP (value, 
		       0, 
		       data->image_width - data->selection_area.width);
	data->selection_area.x = value + data->image_x;

	gtk_spin_button_set_range (data->crop_width_spinbutton, 0.0, (double) data->image_width - value);

	gtk_widget_queue_draw (data->crop_image);
}


static void
selection_y_value_changed_cb (GtkSpinButton *spin,
			      DialogData    *data)
{
	int value;

	value = gtk_spin_button_get_value_as_int (spin);
	value = CLAMP (value, 
		       0, 
		       data->image_height - data->selection_area.height);
	data->selection_area.y = value + data->image_y;

	gtk_spin_button_set_range (data->crop_height_spinbutton, 0.0, (double) data->image_height - value);

	gtk_widget_queue_draw (data->crop_image);
}


static void
selection_width_value_changed_cb (GtkSpinButton *spin,
				  DialogData    *data)
{
	data->selection_area.width = gtk_spin_button_get_value_as_int (spin);

	gtk_spin_button_set_range (data->crop_x_spinbutton, 0.0, (double) data->image_width - data->selection_area.width);

	gtk_widget_queue_draw (data->crop_image);
}


static void
selection_height_value_changed_cb (GtkSpinButton *spin,
				   DialogData    *data)
{
	data->selection_area.height = gtk_spin_button_get_value_as_int (spin);

	gtk_spin_button_set_range (data->crop_y_spinbutton, 0.0, (double) data->image_height - data->selection_area.height);

	gtk_widget_queue_draw (data->crop_image);
}


/* FIXME
static void
set_spin_value (DialogData *data,
		GtkWidget  *spin, 
		int         x)
{
	g_signal_handlers_block_by_data (G_OBJECT (spin), data);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin), x);
	g_signal_handlers_unblock_by_data (G_OBJECT (spin), data);
}
*/


static gboolean
image_expose_cb (GtkWidget      *widget,
		 GdkEventExpose *event,
		 DialogData     *data)
{
	paint_rubberband (data, widget, &data->selection_area, &event->area);
	return FALSE;
}


void
dlg_crop (GThumbWindow *window)
{
	DialogData *data;
	GtkWidget  *ok_button;
	GtkWidget  *cancel_button;
	GdkPixbuf  *pixbuf;	
	int         width, height;

	data = g_new0 (DialogData, 1);
	data->window = window;
	data->gui = glade_xml_new (GTHUMB_GLADEDIR "/" GLADE_FILE, NULL,
				   NULL);

	if (! data->gui) {
		g_warning ("Could not find " GLADE_FILE "\n");
		g_free (data);
		return;
	}

	/* Get the widgets. */

	data->dialog = glade_xml_get_widget (data->gui, "crop_dialog");
	data->crop_image = glade_xml_get_widget (data->gui, "crop_image");
	data->crop_x_spinbutton = (GtkSpinButton*) glade_xml_get_widget (data->gui, "crop_x_spinbutton");
	data->crop_y_spinbutton = (GtkSpinButton*) glade_xml_get_widget (data->gui, "crop_y_spinbutton");
	data->crop_width_spinbutton = (GtkSpinButton*) glade_xml_get_widget (data->gui, "crop_width_spinbutton");
	data->crop_height_spinbutton = (GtkSpinButton*) glade_xml_get_widget (data->gui, "crop_height_spinbutton");

	ok_button = glade_xml_get_widget (data->gui, "crop_okbutton");
	cancel_button = glade_xml_get_widget (data->gui, "crop_cancelbutton");

	g_return_if_fail (pixbuf != NULL);

	/* Set widgets data. */

	gtk_widget_realize (data->dialog);

	data->selection_gc = gdk_gc_new (data->dialog->window);
	gdk_gc_copy (data->selection_gc, data->dialog->style->bg_gc[GTK_STATE_SELECTED]);

	/**/

	pixbuf = image_viewer_get_current_pixbuf (IMAGE_VIEWER (data->window->viewer));

	width = gdk_pixbuf_get_width (pixbuf);
	height = gdk_pixbuf_get_height (pixbuf);
	if (scale_keepping_ratio (&width, &height, PREVIEW_SIZE, PREVIEW_SIZE))
		data->zoom = MIN ((double)PREVIEW_SIZE / gdk_pixbuf_get_width (pixbuf), (double)PREVIEW_SIZE / gdk_pixbuf_get_height (pixbuf));
	else
		data->zoom = 1.0;

	data->image_width = width;
	data->image_height = height;
	data->image_x = data->crop_image->allocation.x + ((double) data->crop_image->allocation.width - width) / 2.0;
	data->image_y = data->crop_image->allocation.y + ((double) data->crop_image->allocation.height - height) / 2.0;

	pixbuf = gdk_pixbuf_scale_simple (pixbuf, width, height, GDK_INTERP_BILINEAR);
	gtk_image_set_from_pixbuf (GTK_IMAGE (data->crop_image), pixbuf);
	g_object_unref (pixbuf);

	/**/

	data->selection_area.x = data->image_width * 0.25;
	data->selection_area.y = data->image_height * 0.25;
	data->selection_area.width = data->image_width * 0.50;
	data->selection_area.height = data->image_height * 0.50;

	gtk_spin_button_set_range (data->crop_x_spinbutton, 0.0, data->image_width - data->selection_area.width);
	gtk_spin_button_set_range (data->crop_y_spinbutton, 0.0, data->image_height - data->selection_area.height);
	gtk_spin_button_set_range (data->crop_width_spinbutton, 0.0, data->image_width - data->selection_area.x);
	gtk_spin_button_set_range (data->crop_height_spinbutton, 0.0, data->image_height - data->selection_area.y);

	gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->crop_x_spinbutton), data->selection_area.x);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->crop_y_spinbutton), data->selection_area.y);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->crop_width_spinbutton), data->selection_area.width);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->crop_height_spinbutton), data->selection_area.height);

	data->selection_area.x += data->image_x;
	data->selection_area.y += data->image_y;

	/* Set the signals handlers. */
	
	g_signal_connect (G_OBJECT (data->dialog),
			  "destroy",
			  G_CALLBACK (destroy_cb),
			  data);
	g_signal_connect_swapped (G_OBJECT (cancel_button), 
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  data->dialog);
	g_signal_connect (G_OBJECT (ok_button), 
			  "clicked",
			  G_CALLBACK (ok_cb),
			  data);
	g_signal_connect_after (G_OBJECT (data->crop_image), 
				"expose_event",
				G_CALLBACK (image_expose_cb),
				data);
	g_signal_connect (G_OBJECT (data->crop_x_spinbutton), 
			  "value_changed",
			  G_CALLBACK (selection_x_value_changed_cb),
			  data);
	g_signal_connect (G_OBJECT (data->crop_y_spinbutton), 
			  "value_changed",
			  G_CALLBACK (selection_y_value_changed_cb),
			  data);
	g_signal_connect (G_OBJECT (data->crop_width_spinbutton), 
			  "value_changed",
			  G_CALLBACK (selection_width_value_changed_cb),
			  data);
	g_signal_connect (G_OBJECT (data->crop_height_spinbutton), 
			  "value_changed",
			  G_CALLBACK (selection_height_value_changed_cb),
			  data);

	/* Run dialog. */

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog),
				      GTK_WINDOW (window->app));
	gtk_widget_show (data->dialog);
}
