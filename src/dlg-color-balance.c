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

#include <config.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include <libgnomevfs/gnome-vfs-mime.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <glade/glade.h>

#include "async-pixbuf-ops.h"
#include "gth-pixbuf-op.h"
#include "gthumb-window.h"
#include "pixbuf-utils.h"


#define GLADE_FILE     "gthumb_edit.glade"
#define PREVIEW_SIZE   200
#define SCALE_SIZE     120


typedef struct {
	GThumbWindow *window;
	ImageViewer  *viewer;
	GdkPixbuf    *image;
	GdkPixbuf    *orig_pixbuf;
	GdkPixbuf    *new_pixbuf;

	gboolean      original_modified;
	gboolean      modified;

	GladeXML     *gui;

	GtkWidget    *dialog;
	GtkWidget    *cb_cyan_red_hscale;
	GtkWidget    *cb_magenta_green_hscale;
	GtkWidget    *cb_yellow_blue_hscale;
	GtkWidget    *cb_preview_image;
	GtkWidget    *cb_luminosity_checkbutton;
} DialogData;


/* called when the main dialog is closed. */
static void
destroy_cb (GtkWidget  *widget, 
	    DialogData *data)
{
	g_object_unref (data->image);
	g_object_unref (data->orig_pixbuf);
	g_object_unref (data->new_pixbuf);
	g_object_unref (data->gui);
	g_free (data);
}


static gboolean
preview_done_cb (GthPixbufOp *pixop,
		 gboolean     completed,
		 DialogData  *data)
{
	gtk_widget_queue_draw (data->cb_preview_image);
	g_object_unref (pixop);
	return FALSE;
}


static void
apply_changes (DialogData *data, 
	       GdkPixbuf  *src, 
	       GdkPixbuf  *dest, 
	       gboolean    preview)
{
	double       cyan_red;
	double       magenta_green;
	double       yellow_blue;
	gboolean     preserve_luminosity;
	GthPixbufOp *pixop;

	cyan_red = gtk_range_get_value (GTK_RANGE (data->cb_cyan_red_hscale));
	magenta_green = gtk_range_get_value (GTK_RANGE (data->cb_magenta_green_hscale));
	yellow_blue = gtk_range_get_value (GTK_RANGE (data->cb_yellow_blue_hscale));
	preserve_luminosity = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->cb_luminosity_checkbutton));

	pixop = _gdk_pixbuf_color_balance (src,
					   dest,
					   cyan_red,
					   magenta_green,
					   yellow_blue,
					   preserve_luminosity);

	if (preview) {
		g_signal_connect (G_OBJECT (pixop),
				  "pixbuf_op_done",
				  G_CALLBACK (preview_done_cb),
				  data);
		gth_pixbuf_op_start (pixop);
	} else {
		window_exec_pixbuf_op (data->window, pixop);
		g_object_unref (pixop);
	}
}


/* called when the ok button is clicked. */
static void
ok_cb (GtkWidget  *widget, 
       DialogData *data)
{
	apply_changes (data, data->image, data->image, FALSE);
	image_viewer_set_pixbuf (data->viewer, data->image);
	window_image_set_modified (data->window, TRUE);
	gtk_widget_destroy (data->dialog);
}


/* called when the cancel button is clicked. */
static void
cancel_cb (GtkWidget  *widget, 
	   DialogData *data)
{
	if (data->modified) {
		image_viewer_set_pixbuf (data->viewer, data->image);
		window_image_set_modified (data->window, data->original_modified);
	}
	gtk_widget_destroy (data->dialog);
}


/* called when the preview button is clicked. */
static void
preview_cb (GtkWidget  *widget, 
	    DialogData *data)
{
	GdkPixbuf *preview;

	preview = gdk_pixbuf_copy (data->image);
	apply_changes (data, preview, preview, FALSE);
	g_object_unref (preview);

	data->modified = TRUE;
}


/* called when the reset button is clicked. */
static void
reset_cb (GtkWidget  *widget, 
	  DialogData *data)
{
	gtk_range_set_value (GTK_RANGE (data->cb_cyan_red_hscale), 0.0);
	gtk_range_set_value (GTK_RANGE (data->cb_magenta_green_hscale), 0.0);
	gtk_range_set_value (GTK_RANGE (data->cb_yellow_blue_hscale), 0.0);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->cb_luminosity_checkbutton), TRUE);
}


static void
range_value_changed (GtkRange   *range,
		     DialogData *data)
{
	apply_changes (data, data->orig_pixbuf, data->new_pixbuf, TRUE);
}


static void
luminosity_toggled (GtkWidget  *widget,
		    DialogData *data)
{
	apply_changes (data, data->orig_pixbuf, data->new_pixbuf, TRUE);
}


static GtkWidget*
gimp_scale_entry_new (GtkWidget  *parent_box,
		      gfloat      value,
                      gfloat      lower,
                      gfloat      upper,
                      gfloat      step_increment,
                      gfloat      page_increment)
{
	GtkWidget *hbox;
	GtkWidget *scale;
	GtkWidget *spinbutton;
	GtkObject *adj;

	adj = gtk_adjustment_new (value, lower, upper,
				  step_increment, page_increment,
				  0.0);
	
	spinbutton = gtk_spin_button_new  (GTK_ADJUSTMENT (adj), 1.0, 0);
	scale = gtk_hscale_new (GTK_ADJUSTMENT (adj));
	gtk_scale_set_draw_value (GTK_SCALE (scale), FALSE);

	hbox = gtk_hbox_new (FALSE, 5);
	gtk_box_pack_start (GTK_BOX (hbox), scale, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);
	gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
	gtk_widget_set_size_request (GTK_WIDGET (scale), SCALE_SIZE, -1);

	gtk_box_pack_start (GTK_BOX (parent_box), hbox, TRUE, TRUE, 0);
	gtk_widget_show_all (hbox);

	return scale;
}


void
dlg_color_balance (GThumbWindow *window)
{
	DialogData *data;
	GtkWidget  *ok_button;
	GtkWidget  *reset_button;
	GtkWidget  *cancel_button;
	GtkWidget  *preview_button;
	GtkWidget  *hbox;
	GdkPixbuf  *image;
	int         image_width, image_height;
	int         preview_width, preview_height;

	data = g_new0 (DialogData, 1);
	data->window = window;
	data->original_modified = window_image_get_modified (window);
	data->gui = glade_xml_new (GTHUMB_GLADEDIR "/" GLADE_FILE, NULL,
				   NULL);

	if (! data->gui) {
		g_warning ("Could not find " GLADE_FILE "\n");
		g_free (data);
		return;
	}

	/* Get the widgets. */

	data->dialog = glade_xml_get_widget (data->gui, "color_balance_dialog");
	data->cb_luminosity_checkbutton = glade_xml_get_widget (data->gui, "cb_luminosity_checkbutton");
	data->cb_preview_image = glade_xml_get_widget (data->gui, "cb_preview_image");

	ok_button = glade_xml_get_widget (data->gui, "cb_ok_button");
	reset_button = glade_xml_get_widget (data->gui, "cb_reset_button");
	cancel_button = glade_xml_get_widget (data->gui, "cb_cancel_button");
	preview_button = glade_xml_get_widget (data->gui, "cb_preview_button");

	hbox = glade_xml_get_widget (data->gui, "cb_cyan_red_hbox");
	data->cb_cyan_red_hscale = gimp_scale_entry_new (hbox, 
							 0.0, 
							 -100.0, 100.0,
							 1.0,
							 10.0);

	hbox = glade_xml_get_widget (data->gui, "cb_magenta_green_hbox");
	data->cb_magenta_green_hscale = gimp_scale_entry_new (hbox, 
							      0.0, 
							      -100.0, 100.0,
							      1.0,
							      10.0);

	hbox = glade_xml_get_widget (data->gui, "cb_yellow_blue_hbox");
	data->cb_yellow_blue_hscale = gimp_scale_entry_new (hbox, 
							    0.0, 
							    -100.0, 100.0,
							    1.0,
							    10.0);

	data->viewer = IMAGE_VIEWER (window->viewer);

	/**/

	image = image_viewer_get_current_pixbuf (data->viewer);
	data->image = gdk_pixbuf_copy (image);

	image_width = gdk_pixbuf_get_width (image);
	image_height = gdk_pixbuf_get_height (image);

	if (image_width > PREVIEW_SIZE || image_height > PREVIEW_SIZE) {
		double factor;

		factor = MIN ((double) (PREVIEW_SIZE) / image_width, 
			      (double) (PREVIEW_SIZE) / image_height);
		preview_width  = MAX ((int) (factor * image_width), 1);
		preview_height = MAX ((int) (factor * image_height), 1);
	} else {
		preview_width  = image_width;
		preview_height = image_height;
	}
	
	data->orig_pixbuf = gdk_pixbuf_scale_simple (image, 
						     preview_width, 
						     preview_height,
						     GDK_INTERP_BILINEAR);
	data->new_pixbuf = gdk_pixbuf_copy (data->orig_pixbuf);

	gtk_image_set_from_pixbuf (GTK_IMAGE (data->cb_preview_image), 
				   data->new_pixbuf);

	/* Set widgets data. */

	/* Set the signals handlers. */
	
	g_signal_connect (G_OBJECT (data->dialog),
			  "destroy",
			  G_CALLBACK (destroy_cb),
			  data);

	g_signal_connect (G_OBJECT (cancel_button), 
			  "clicked",
			  G_CALLBACK (cancel_cb),
			  data);

	g_signal_connect (G_OBJECT (ok_button), 
			  "clicked",
			  G_CALLBACK (ok_cb),
			  data);

	g_signal_connect (G_OBJECT (reset_button), 
			  "clicked",
			  G_CALLBACK (reset_cb),
			  data);

	g_signal_connect (G_OBJECT (preview_button), 
			  "clicked",
			  G_CALLBACK (preview_cb),
			  data);

	g_signal_connect (G_OBJECT (data->cb_luminosity_checkbutton),
			  "toggled",
			  G_CALLBACK (luminosity_toggled),
			  data);

	/* -- */

	g_signal_connect (G_OBJECT (data->cb_cyan_red_hscale),
			  "value_changed",
			  G_CALLBACK (range_value_changed),
			  data);

	g_signal_connect (G_OBJECT (data->cb_magenta_green_hscale),
			  "value_changed",
			  G_CALLBACK (range_value_changed),
			  data);

	g_signal_connect (G_OBJECT (data->cb_yellow_blue_hscale),
			  "value_changed",
			  G_CALLBACK (range_value_changed),
			  data);

	/* Run dialog. */

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog),
				      GTK_WINDOW (window->app));
	gtk_widget_show (data->dialog);

	apply_changes (data, data->orig_pixbuf, data->new_pixbuf, TRUE);
}
