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
#include "gth-image-selector.h"
#include "gthumb-stock.h"
#include "typedefs.h"

#define GLADE_FILE     "gthumb_crop.glade"


/**/

typedef struct {
	GThumbWindow  *window;
	GladeXML      *gui;

	int            image_width, image_height;
	int            display_width, display_height;

	GtkWidget     *dialog;
	GtkWidget     *crop_image;
	GtkSpinButton *crop_x_spinbutton;
	GtkSpinButton *crop_y_spinbutton;
	GtkSpinButton *crop_width_spinbutton;
	GtkSpinButton *crop_height_spinbutton;
	GtkWidget     *ratio_optionmenu;
	GtkSpinButton *ratio_w_spinbutton;
	GtkSpinButton *ratio_h_spinbutton;
	GtkWidget     *custom_ratio_box;
	GtkWidget     *ratio_swap_button;
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
	GdkPixbuf     *old_pixbuf;
	GdkPixbuf     *new_pixbuf;
	GdkRectangle   selection;

	/* Save options */

	eel_gconf_set_integer (PREF_CROP_ASPECT_RATIO_WIDTH, gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (data->ratio_w_spinbutton)));
	eel_gconf_set_integer (PREF_CROP_ASPECT_RATIO_HEIGHT, gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (data->ratio_h_spinbutton)));
	pref_set_crop_ratio (gtk_option_menu_get_history (GTK_OPTION_MENU (data->ratio_optionmenu)));

	/**/

	gth_image_selector_get_selection (GTH_IMAGE_SELECTOR (data->crop_image), &selection);

	if ((selection.width > 0) && (selection.height > 0)) {
		old_pixbuf = image_viewer_get_current_pixbuf (IMAGE_VIEWER (data->window->viewer));
		g_object_ref (old_pixbuf);
		new_pixbuf = gdk_pixbuf_new_subpixbuf (old_pixbuf, 
						       selection.x, 
						       selection.y, 
						       selection.width, 
						       selection.height);
		if (new_pixbuf != NULL) {
			image_viewer_set_pixbuf (IMAGE_VIEWER (data->window->viewer), new_pixbuf);
			g_object_unref (new_pixbuf);
		}
		g_object_unref (old_pixbuf);

		window_image_set_modified (data->window, TRUE);
	}

	gtk_widget_destroy (data->dialog);
}


static void
selection_x_value_changed_cb (GtkSpinButton *spin,
			      DialogData    *data)
{
	GthImageSelector *selector = GTH_IMAGE_SELECTOR (data->crop_image);
	GdkRectangle selection;

	gth_image_selector_get_selection (selector, &selection);
	selection.x = gtk_spin_button_get_value_as_int (spin);
	gth_image_selector_set_selection (selector, selection);
}


static void
selection_y_value_changed_cb (GtkSpinButton *spin,
			      DialogData    *data)
{
	GthImageSelector *selector = GTH_IMAGE_SELECTOR (data->crop_image);
	GdkRectangle selection;

	gth_image_selector_get_selection (selector, &selection);
	selection.y = gtk_spin_button_get_value_as_int (spin);
	gth_image_selector_set_selection (selector, selection);
}


static void
selection_width_value_changed_cb (GtkSpinButton *spin,
				  DialogData    *data)
{
	GthImageSelector *selector = GTH_IMAGE_SELECTOR (data->crop_image);
	gth_image_selector_set_selection_width (selector, gtk_spin_button_get_value_as_int (spin));
}


static void
selection_height_value_changed_cb (GtkSpinButton *spin,
				   DialogData    *data)
{
	GthImageSelector *selector = GTH_IMAGE_SELECTOR (data->crop_image);
	gth_image_selector_set_selection_height (selector, gtk_spin_button_get_value_as_int (spin));
}


static void
set_spin_range_value (DialogData    *data,
		      GtkSpinButton *spin, 
		      int            min,
		      int            max,
		      int            x)
{
	g_signal_handlers_block_by_data (G_OBJECT (spin), data);
	gtk_spin_button_set_range (GTK_SPIN_BUTTON (spin), min, max);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin), x);
	g_signal_handlers_unblock_by_data (G_OBJECT (spin), data);
}


static void
selection_changed_cb (GthImageSelector *selector,
		      DialogData       *data)
{
	GdkRectangle selection;
	int          min, max;

	gth_image_selector_get_selection (selector, &selection);

	min = 0;
	max = data->image_width - selection.width;
	set_spin_range_value (data, data->crop_x_spinbutton, 
			      min, max, selection.x);

	min = 0;
	max = data->image_height - selection.height;
	set_spin_range_value (data, data->crop_y_spinbutton, 
			      min, max, selection.y);

	min = 0;
	max = data->image_width - selection.x;
	set_spin_range_value (data, data->crop_width_spinbutton, 
			      min, max, selection.width);

	min = 0;
	max = data->image_height - selection.y;
	set_spin_range_value (data, data->crop_height_spinbutton, 
			      min, max, selection.height);
}


static void
set_spin_value (DialogData    *data,
		GtkSpinButton *spin, 
		int            x)
{
	g_signal_handlers_block_by_data (G_OBJECT (spin), data);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin), x);
	g_signal_handlers_unblock_by_data (G_OBJECT (spin), data);
}


static void
ratio_optionmenu_changed_cb (GtkOptionMenu *optionmenu,
			     DialogData    *data)
{
	int      idx = gtk_option_menu_get_history (optionmenu);
	int      w = 1, h = 1;
	gboolean use_ratio = TRUE;

	switch (idx) {
	case GTH_CROP_RATIO_NONE:
		use_ratio = FALSE;
		break;
	case GTH_CROP_RATIO_SQUARE:
		w = h = 1;
		break;
	case GTH_CROP_RATIO_IMAGE:
		w = data->image_width;
		h = data->image_height;
		break;
	case GTH_CROP_RATIO_DISPLAY:
		w = data->display_width;
		h = data->display_height;
		break;
	case GTH_CROP_RATIO_4_3:
		w = 4;
		h = 3;
		break;
	case GTH_CROP_RATIO_4_6:
		w = 4;
		h = 6;
		break;
	case GTH_CROP_RATIO_5_7:
		w = 5;
		h = 7;
		break;
	case GTH_CROP_RATIO_8_10:
		w = 8;
		h = 10;
		break;
	case GTH_CROP_RATIO_CUSTOM:
	default:
		w = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (data->ratio_w_spinbutton));
		h = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (data->ratio_h_spinbutton));
		break;
	}

	gtk_widget_set_sensitive (data->custom_ratio_box, idx == GTH_CROP_RATIO_CUSTOM);
	set_spin_value (data, data->ratio_w_spinbutton, w);
	set_spin_value (data, data->ratio_h_spinbutton, h);

	gth_image_selector_set_ratio (GTH_IMAGE_SELECTOR (data->crop_image),
				      use_ratio,
				      (double) w / h);
}


static void
ratio_value_changed_cb (GtkSpinButton *spin,
			DialogData    *data)
{
	int w, h;

	w = gtk_spin_button_get_value_as_int (data->ratio_w_spinbutton);
	h = gtk_spin_button_get_value_as_int (data->ratio_h_spinbutton);
			
	gth_image_selector_set_ratio (GTH_IMAGE_SELECTOR (data->crop_image),
				      TRUE,
				      (double) w / h);
}


static void
ratio_swap_button_cb (GtkButton *button,
		      DialogData *data)
{
	int w, h;

	w = gtk_spin_button_get_value_as_int (data->ratio_w_spinbutton);
	h = gtk_spin_button_get_value_as_int (data->ratio_h_spinbutton);

	set_spin_value (data, data->ratio_w_spinbutton, h);
	set_spin_value (data, data->ratio_h_spinbutton, w);

	gth_image_selector_set_ratio (GTH_IMAGE_SELECTOR (data->crop_image),
				      TRUE,
				      (double) h / w);
}


void
dlg_crop (GThumbWindow *window)
{
	DialogData   *data;
	GtkWidget    *crop_frame;
	GtkWidget    *ok_button;
	GtkWidget    *cancel_button;
	GdkPixbuf    *pixbuf;	
	GtkWidget    *menu, *item;
	GtkWidget    *image;
	char         *label;
	GthCropRatio  crop_ratio;
	int           ratio_w, ratio_h;

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
	crop_frame = glade_xml_get_widget (data->gui, "crop_frame");
	data->crop_x_spinbutton = (GtkSpinButton*) glade_xml_get_widget (data->gui, "crop_x_spinbutton");
	data->crop_y_spinbutton = (GtkSpinButton*) glade_xml_get_widget (data->gui, "crop_y_spinbutton");
	data->crop_width_spinbutton = (GtkSpinButton*) glade_xml_get_widget (data->gui, "crop_width_spinbutton");
	data->crop_height_spinbutton = (GtkSpinButton*) glade_xml_get_widget (data->gui, "crop_height_spinbutton");
	data->ratio_optionmenu = glade_xml_get_widget (data->gui, "ratio_optionmenu");
	data->ratio_w_spinbutton = (GtkSpinButton*) glade_xml_get_widget (data->gui, "ratio_w_spinbutton");
	data->ratio_h_spinbutton = (GtkSpinButton*) glade_xml_get_widget (data->gui, "ratio_h_spinbutton");
	data->custom_ratio_box = glade_xml_get_widget (data->gui, "custom_ratio_box");
	data->ratio_swap_button = glade_xml_get_widget (data->gui, "ratio_swap_button");

	ok_button = glade_xml_get_widget (data->gui, "crop_okbutton");
	cancel_button = glade_xml_get_widget (data->gui, "crop_cancelbutton");

	data->crop_image = gth_image_selector_new (NULL);
	gtk_container_add (GTK_CONTAINER (crop_frame), data->crop_image);

	image = glade_xml_get_widget (data->gui, "ratio_swap_image");
	gtk_image_set_from_stock (GTK_IMAGE (image), GTHUMB_STOCK_SWAP, GTK_ICON_SIZE_MENU);

	image = glade_xml_get_widget (data->gui, "crop_image");
	gtk_image_set_from_stock (GTK_IMAGE (image), GTHUMB_STOCK_CROP, GTK_ICON_SIZE_MENU);

	/* Set widgets data. */

	pixbuf = image_viewer_get_current_pixbuf (IMAGE_VIEWER (data->window->viewer));
	data->image_width = gdk_pixbuf_get_width (pixbuf);
	data->image_height = gdk_pixbuf_get_height (pixbuf);
	gth_image_selector_set_pixbuf (GTH_IMAGE_SELECTOR (data->crop_image), pixbuf);
	gtk_widget_show_all (data->crop_image);

	/**/

	menu = gtk_option_menu_get_menu (GTK_OPTION_MENU (data->ratio_optionmenu));
	g_object_ref (menu);

	label = g_strdup_printf (_("%d x %d (Image)"), data->image_width, data->image_height);
	item = gtk_menu_item_new_with_label (label);
	gtk_widget_show (item);
	g_free (label);
	gtk_menu_shell_insert (GTK_MENU_SHELL (menu), item, 2);

	data->display_width = gdk_screen_get_width (gdk_screen_get_default ());
	data->display_height = gdk_screen_get_height (gdk_screen_get_default ());
	label = g_strdup_printf (_("%d x %d (Display)"), data->display_width, data->display_height);
	item = gtk_menu_item_new_with_label (label);
	gtk_widget_show (item);
	g_free (label);
	gtk_menu_shell_insert (GTK_MENU_SHELL (menu), item, 3);

	gtk_option_menu_remove_menu (GTK_OPTION_MENU (data->ratio_optionmenu));
	gtk_option_menu_set_menu (GTK_OPTION_MENU (data->ratio_optionmenu), menu);
	g_object_unref (menu);

	ratio_w = eel_gconf_get_integer (PREF_CROP_ASPECT_RATIO_WIDTH, 1);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->ratio_w_spinbutton), 
				   ratio_w);
	ratio_h = eel_gconf_get_integer (PREF_CROP_ASPECT_RATIO_HEIGHT, 1);
	ratio_h = MAX (ratio_h, 1);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->ratio_h_spinbutton),
				   ratio_h);

	crop_ratio = pref_get_crop_ratio ();
	gtk_option_menu_set_history (GTK_OPTION_MENU (data->ratio_optionmenu),
				     crop_ratio);
	gtk_widget_set_sensitive (data->custom_ratio_box, 
				  (crop_ratio == GTH_CROP_RATIO_CUSTOM));

	gth_image_selector_set_ratio (GTH_IMAGE_SELECTOR (data->crop_image),
				      (crop_ratio != GTH_CROP_RATIO_NONE),
				      (double) ratio_w / ratio_h);

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
	g_signal_connect (G_OBJECT (data->crop_image), 
			  "selection_changed",
			  G_CALLBACK (selection_changed_cb),
			  data);
	g_signal_connect (G_OBJECT (data->ratio_optionmenu), 
			  "changed",
			  G_CALLBACK (ratio_optionmenu_changed_cb),
			  data);
	g_signal_connect (G_OBJECT (data->ratio_w_spinbutton), 
			  "value_changed",
			  G_CALLBACK (ratio_value_changed_cb),
			  data);
	g_signal_connect (G_OBJECT (data->ratio_h_spinbutton), 
			  "value_changed",
			  G_CALLBACK (ratio_value_changed_cb),
			  data);
	g_signal_connect (G_OBJECT (data->ratio_swap_button), 
			  "clicked",
			  G_CALLBACK (ratio_swap_button_cb),
			  data);

	/* Run dialog. */

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog),
				      GTK_WINDOW (window->app));
	gtk_widget_show (data->dialog);
}
