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

#include <config.h>
#include <sys/types.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include <libgnomevfs/gnome-vfs-mime.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <glade/glade.h>

#include "gthumb-window.h"
#include "pixbuf-utils.h"
#include "gconf-utils.h"
#include "preferences.h"


#define DEFAULT_VALUE  2.0
#define GLADE_FILE     "gthumb_edit.glade"
#define PREVIEW_SIZE   200
#define SCALE_SIZE     120


enum {
	UNIT_PIXEL,
	UNIT_PERCENT
};


typedef struct {
	GThumbWindow *window;
	ImageViewer  *viewer;
	GladeXML     *gui;

	int           width, height;
	double        ratio;
	gboolean      percentage;

	GtkWidget    *dialog;
	GtkWidget    *s_new_width_spinbutton;
	GtkWidget    *s_new_height_spinbutton;
	GtkWidget    *s_high_quality_checkbutton;
	GtkWidget    *s_width_ratio_label;
	GtkWidget    *s_height_ratio_label;
	GtkWidget    *s_keep_ratio_checkbutton;
	GtkWidget    *s_unit_optionmenu;
} DialogData;


/* called when the main dialog is closed. */
static void
destroy_cb (GtkWidget  *widget, 
	    DialogData *data)
{
	g_object_unref (data->gui);
	g_free (data);
}


/* called when the ok button is clicked. */
static void
ok_cb (GtkWidget  *widget, 
       DialogData *data)
{
	GdkPixbuf *orig_pixbuf;
	GdkPixbuf *new_pixbuf;
	double     d_width, d_height;
	int        width, height;
	gboolean   high_quality;

	/* Save options */

	if (data->percentage)
		eel_gconf_set_string (PREF_SCALE_UNIT, "percentage");
	else
		eel_gconf_set_string (PREF_SCALE_UNIT, "pixels");

	eel_gconf_set_boolean (PREF_SCALE_KEEP_RATIO, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->s_keep_ratio_checkbutton)));
	eel_gconf_set_boolean (PREF_SCALE_HIGH_QUALITY, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->s_high_quality_checkbutton)));

	/**/

	orig_pixbuf = image_viewer_get_current_pixbuf (data->viewer);
	g_return_if_fail (orig_pixbuf != NULL);
	g_object_ref (orig_pixbuf);

	high_quality = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->s_high_quality_checkbutton));
	d_width = gtk_spin_button_get_value (GTK_SPIN_BUTTON (data->s_new_width_spinbutton));
	d_height = gtk_spin_button_get_value (GTK_SPIN_BUTTON (data->s_new_height_spinbutton));

	if (data->percentage) {
		d_width  = d_width / 100.0 * data->width;
		d_height = d_height / 100.0 * data->height;
	} 

	width  = MAX (2, (int) d_width);
	height = MAX (2, (int) d_height);

	new_pixbuf = gdk_pixbuf_scale_simple (orig_pixbuf,
					      width,
					      height,
					      high_quality ? GDK_INTERP_BILINEAR : GDK_INTERP_NEAREST);

	image_viewer_set_pixbuf (data->viewer, new_pixbuf);

	g_object_unref (orig_pixbuf);
	g_object_unref (new_pixbuf);

	window_image_set_modified (data->window, TRUE);

	gtk_widget_destroy (data->dialog);
}


static void
update_ratio (DialogData *data)
{
	double  new_width;
	double  new_height;
	char    ratio[100];

	new_width = gtk_spin_button_get_value (GTK_SPIN_BUTTON (data->s_new_width_spinbutton));
	new_height = gtk_spin_button_get_value (GTK_SPIN_BUTTON (data->s_new_height_spinbutton));

	if (data->percentage) {
		new_width = new_width / 100.0 * data->width;
		new_height = new_height / 100.0 * data->height;
	}

	snprintf (ratio, sizeof (ratio), "%0.3f", new_width / data->width);
	gtk_label_set_text (GTK_LABEL (data->s_width_ratio_label), ratio);

	snprintf (ratio, sizeof (ratio), "%0.3f", new_height / data->height);
	gtk_label_set_text (GTK_LABEL (data->s_height_ratio_label), ratio);
}


static void
w_spinbutton_value_changed (GtkSpinButton *button,
			    DialogData    *data);

static void
h_spinbutton_value_changed (GtkSpinButton *button,
			    DialogData    *data)
{
	gboolean keep_aspect_ratio;

	keep_aspect_ratio = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->s_keep_ratio_checkbutton));

	if (keep_aspect_ratio) {
		double width, height;

		height = gtk_spin_button_get_value (GTK_SPIN_BUTTON (data->s_new_height_spinbutton));

		if (data->percentage)
			width = height;
		else
			width = floor (height * data->ratio + 0.5);

		g_signal_handlers_block_by_func (data->s_new_width_spinbutton,
						 w_spinbutton_value_changed,
						 data);

		gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->s_new_width_spinbutton), width);
		
		g_signal_handlers_unblock_by_func (data->s_new_width_spinbutton,
						   w_spinbutton_value_changed,
						   data);
	}

	update_ratio (data);
}


static void
w_spinbutton_value_changed (GtkSpinButton *button,
			    DialogData    *data)
{
	gboolean keep_aspect_ratio;

	keep_aspect_ratio = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->s_keep_ratio_checkbutton));

	if (keep_aspect_ratio) {
		double width, height;

		width = gtk_spin_button_get_value (GTK_SPIN_BUTTON (data->s_new_width_spinbutton));

		if (data->percentage)
			height = width;
		else
			height = floor (width / data->ratio + 0.5);

		g_signal_handlers_block_by_func (data->s_new_height_spinbutton,
						 h_spinbutton_value_changed,
						 data);

		gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->s_new_height_spinbutton), height);

		g_signal_handlers_unblock_by_func (data->s_new_height_spinbutton,
						   h_spinbutton_value_changed,
						   data);
	}

	update_ratio (data);
}


static void
keep_aspect_ratio_toggled (GtkToggleButton *button,
			   DialogData      *data)
{
	if (gtk_toggle_button_get_active (button))
		w_spinbutton_value_changed (NULL, data);
}


static void
unit_changed (GtkOptionMenu *option_menu,
	      DialogData    *data)
{
	double w_value, width;
	double h_value, height;

	switch (gtk_option_menu_get_history (option_menu)) {
	case UNIT_PIXEL:
		if (! data->percentage)
			return;
		data->percentage = FALSE;

		w_value = gtk_spin_button_get_value (GTK_SPIN_BUTTON (data->s_new_width_spinbutton));
		h_value = gtk_spin_button_get_value (GTK_SPIN_BUTTON (data->s_new_height_spinbutton));

		width = w_value * ((double) data->width / 100.0);
		height = h_value * ((double) data->height / 100.0);

		gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->s_new_width_spinbutton), width);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->s_new_height_spinbutton), height);
		break;
		
	case UNIT_PERCENT:
		if (data->percentage)
			return;
		data->percentage = TRUE;

		width = gtk_spin_button_get_value (GTK_SPIN_BUTTON (data->s_new_width_spinbutton));
		height = gtk_spin_button_get_value (GTK_SPIN_BUTTON (data->s_new_height_spinbutton));
		
		w_value = width / data->width * 100.0;
		h_value = height / data->height * 100.0;
		
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->s_new_width_spinbutton), w_value);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->s_new_height_spinbutton), h_value);
		break;
	}
}


void
dlg_scale_image (GThumbWindow *window)
{
	DialogData *data;
	GtkWidget  *ok_button;
	GtkWidget  *cancel_button;
	GtkWidget  *label;
	GdkPixbuf  *pixbuf;
	char        size[100];
	char       *unit;

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

	data->dialog = glade_xml_get_widget (data->gui, "scale_dialog");
	data->s_high_quality_checkbutton = glade_xml_get_widget (data->gui, "s_high_quality_checkbutton");
	data->s_new_width_spinbutton = glade_xml_get_widget (data->gui, "s_new_width_spinbutton");
	data->s_new_height_spinbutton = glade_xml_get_widget (data->gui, "s_new_height_spinbutton");
	data->s_width_ratio_label = glade_xml_get_widget (data->gui, "s_width_ratio_label");
	data->s_height_ratio_label = glade_xml_get_widget (data->gui, "s_height_ratio_label");
	data->s_keep_ratio_checkbutton = glade_xml_get_widget (data->gui, "s_keep_ratio_checkbutton");
	data->s_unit_optionmenu = glade_xml_get_widget (data->gui, "s_unit_optionmenu");

	ok_button = glade_xml_get_widget (data->gui, "s_ok_button");
	cancel_button = glade_xml_get_widget (data->gui, "s_cancel_button");

	data->viewer = IMAGE_VIEWER (window->viewer);

	pixbuf = image_viewer_get_current_pixbuf (data->viewer);

	g_return_if_fail (pixbuf != NULL);

	data->width = gdk_pixbuf_get_width (pixbuf);
	data->height = gdk_pixbuf_get_height (pixbuf);
	data->ratio = ((double) data->width) / data->height;

	/* Set widgets data. */

	sprintf (size, "%d", data->width);
	label = glade_xml_get_widget (data->gui, "s_orig_width_label");
	gtk_label_set_text (GTK_LABEL (label), size);

	sprintf (size, "%d", data->height);
	label = glade_xml_get_widget (data->gui, "s_orig_height_label");
	gtk_label_set_text (GTK_LABEL (label), size);

	/**/

	unit = eel_gconf_get_string (PREF_SCALE_UNIT, "pixels");

	data->percentage = strcmp (unit, "percentage") == 0;
	if (data->percentage) {
		gtk_option_menu_set_history (GTK_OPTION_MENU (data->s_unit_optionmenu), 1);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->s_new_width_spinbutton), 100.0);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->s_new_height_spinbutton), 100.0);
	} else {
		gtk_option_menu_set_history (GTK_OPTION_MENU (data->s_unit_optionmenu), 0);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->s_new_width_spinbutton), data->width);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->s_new_height_spinbutton), data->height);
	}

	g_free (unit);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->s_keep_ratio_checkbutton),
				      eel_gconf_get_boolean (PREF_SCALE_KEEP_RATIO, TRUE));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->s_high_quality_checkbutton),
				      eel_gconf_get_boolean (PREF_SCALE_HIGH_QUALITY, TRUE));

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

	g_signal_connect (G_OBJECT (data->s_new_width_spinbutton), 
			  "value_changed",
			  G_CALLBACK (w_spinbutton_value_changed),
			  data);

	g_signal_connect (G_OBJECT (data->s_new_height_spinbutton), 
			  "value_changed",
			  G_CALLBACK (h_spinbutton_value_changed),
			  data);

	g_signal_connect (G_OBJECT (data->s_keep_ratio_checkbutton),
			  "toggled",
			  G_CALLBACK (keep_aspect_ratio_toggled),
			  data);

	g_signal_connect (G_OBJECT (data->s_unit_optionmenu),
			  "changed",
			  G_CALLBACK (unit_changed),
			  data);

	/* Run dialog. */

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog),
				      GTK_WINDOW (window->app));
	gtk_widget_show (data->dialog);

	update_ratio (data);
}
