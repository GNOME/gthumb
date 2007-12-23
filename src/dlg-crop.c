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

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <glade/glade.h>
#include <libgnomecanvas/libgnomecanvas.h>

#include "glib-utils.h"
#include "gtk-utils.h"
#include "gth-utils.h"
#include "gth-window.h"
#include "gconf-utils.h"
#include "preferences.h"
#include "pixbuf-utils.h"
#include "gth-iviewer.h"
#include "gth-image-history.h"
#include "gth-image-selector.h"
#include "image-viewer.h"
#include "gthumb-stock.h"
#include "typedefs.h"
#include "gth-nav-window.h"

#define GLADE_FILE     "gthumb_crop.glade"

/**/

typedef struct {
	GthWindow     *window;
	GladeXML      *gui;

	GdkPixbuf     *pixbuf;
	int            image_width, image_height;
	int            screen_width, screen_height;
	GthImageHistory *history;

	GtkWidget     *dialog;
	GtkWidget     *nav_win;
	GtkWidget     *nav_container;
	GtkWidget     *image_selector;
	GtkSpinButton *crop_x_spinbutton;
	GtkSpinButton *crop_y_spinbutton;
	GtkSpinButton *crop_width_spinbutton;
	GtkSpinButton *crop_height_spinbutton;
	GtkWidget     *ratio_optionmenu;
	GtkSpinButton *ratio_w_spinbutton;
	GtkSpinButton *ratio_h_spinbutton;
	GtkWidget     *custom_ratio_box;
	GtkWidget     *invert_ratio_checkbutton;
	GtkWidget     *mask_button;
	GtkWidget     *undo_button;
	GtkWidget     *redo_button;
	GtkAdjustment *hadj, *vadj;
} DialogData;


/* called when the main dialog is closed. */
static void
destroy_cb (GtkWidget  *widget,
	    DialogData *data)
{
	g_object_unref (data->history);
	g_object_unref (data->gui);
	g_free (data);
}



static void
apply_cb (DialogData *data,
	  gboolean    save)
{
	GdkPixbuf *old_pixbuf;
	GdkPixbuf *new_pixbuf;

	/* Save options */

	eel_gconf_set_integer (PREF_CROP_ASPECT_RATIO_WIDTH, gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (data->ratio_w_spinbutton)));
	eel_gconf_set_integer (PREF_CROP_ASPECT_RATIO_HEIGHT, gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (data->ratio_h_spinbutton)));
	pref_set_crop_ratio (gtk_option_menu_get_history (GTK_OPTION_MENU (data->ratio_optionmenu)));

	/**/

	new_pixbuf = gth_image_selector_get_pixbuf (GTH_IMAGE_SELECTOR (data->image_selector));
	old_pixbuf = gth_window_get_image_pixbuf (data->window);
	if (new_pixbuf != old_pixbuf) {
		gth_window_set_image_pixbuf (data->window, new_pixbuf);
		if (save)
			gth_window_save_pixbuf (data->window, new_pixbuf, gth_window_get_image_data (data->window));
		else
			gth_window_set_image_modified (data->window, TRUE);
	}

	gtk_widget_destroy (data->dialog);
}


static void
ok_cb (GtkWidget  *widget,
       DialogData *data)
{
	apply_cb (data, FALSE);
}


static void
save_cb (GtkWidget  *widget,
	 DialogData *data)
{
	apply_cb (data, TRUE);
}


static void
help_cb (GtkWidget  *widget,
	 DialogData *data)
{
	gthumb_display_help (GTK_WINDOW (data->dialog), "gthumb-crop");
}


static void
selection_x_value_changed_cb (GtkSpinButton *spin,
			      DialogData    *data)
{
	GthImageSelector *selector = GTH_IMAGE_SELECTOR (data->image_selector);
	gth_image_selector_set_selection_x (selector, gtk_spin_button_get_value_as_int (spin));
}


static void
selection_y_value_changed_cb (GtkSpinButton *spin,
			      DialogData    *data)
{
	GthImageSelector *selector = GTH_IMAGE_SELECTOR (data->image_selector);
	gth_image_selector_set_selection_y (selector, gtk_spin_button_get_value_as_int (spin));
}


static void
selection_width_value_changed_cb (GtkSpinButton *spin,
				  DialogData    *data)
{
	GthImageSelector *selector = GTH_IMAGE_SELECTOR (data->image_selector);
	gth_image_selector_set_selection_width (selector, gtk_spin_button_get_value_as_int (spin));
}


static void
selection_height_value_changed_cb (GtkSpinButton *spin,
				   DialogData    *data)
{
	GthImageSelector *selector = GTH_IMAGE_SELECTOR (data->image_selector);
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

	gth_image_selector_set_mask_visible (GTH_IMAGE_SELECTOR (data->image_selector), (selection.width != 0 || selection.height != 0));
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
		w = data->screen_width;
		h = data->screen_height;
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
	gtk_widget_set_sensitive (data->invert_ratio_checkbutton, use_ratio);
	set_spin_value (data, data->ratio_w_spinbutton, w);
	set_spin_value (data, data->ratio_h_spinbutton, h);

	gth_image_selector_set_ratio (GTH_IMAGE_SELECTOR (data->image_selector),
				      use_ratio,
				      (double) w / h);
}


static void
ratio_value_changed_cb (GtkSpinButton *spin,
			DialogData    *data)
{
	gboolean use_ratio;
	int      w, h;
	double   ratio;

	use_ratio = gtk_option_menu_get_history (GTK_OPTION_MENU (data->ratio_optionmenu)) != GTH_CROP_RATIO_NONE;

	w = gtk_spin_button_get_value_as_int (data->ratio_w_spinbutton);
	h = gtk_spin_button_get_value_as_int (data->ratio_h_spinbutton);

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->invert_ratio_checkbutton)))
		ratio = (double) h / w;
	else
		ratio = (double) w / h;
	gth_image_selector_set_ratio (GTH_IMAGE_SELECTOR (data->image_selector),
				      use_ratio,
				      ratio);
}


static void
zoom_in_button_clicked_cb (GtkButton  *button,
			   DialogData *data)
{
	gth_iviewer_zoom_in (GTH_IVIEWER (data->image_selector));
}


static void
zoom_out_button_clicked_cb (GtkButton  *button,
			    DialogData *data)
{
	gth_iviewer_zoom_out (GTH_IVIEWER (data->image_selector));
}


static void
zoom_100_button_clicked_cb (GtkButton  *button,
			    DialogData *data)
{
	gth_iviewer_set_zoom (GTH_IVIEWER (data->image_selector), 1.0);
}


static void
zoom_fit_button_clicked_cb (GtkButton  *button,
			    DialogData *data)
{
	gth_iviewer_zoom_to_fit (GTH_IVIEWER (data->image_selector));
}


static void
undo_button_clicked_cb (GtkButton  *button,
	   		DialogData *data)
{
	GthImageData *idata;

	idata = gth_image_history_undo (data->history,
					gth_image_selector_get_pixbuf (GTH_IMAGE_SELECTOR (data->image_selector)),
					FALSE);
	if (idata == NULL)
		return;
	gth_image_selector_set_pixbuf (GTH_IMAGE_SELECTOR (data->image_selector), idata->image);
	gth_image_data_unref (idata);

	gth_iviewer_zoom_to_fit (GTH_IVIEWER (data->image_selector));
}


static void
redo_button_clicked_cb (GtkButton  *button,
	   		DialogData *data)
{
	GthImageData *idata;

	idata = gth_image_history_redo (data->history,
					gth_image_selector_get_pixbuf (GTH_IMAGE_SELECTOR (data->image_selector)),
					FALSE);
	if (idata == NULL)
		return;
	gth_image_selector_set_pixbuf (GTH_IMAGE_SELECTOR (data->image_selector), idata->image);
	gth_image_data_unref (idata);

	gth_iviewer_zoom_to_fit (GTH_IVIEWER (data->image_selector));
}


static void
crop_button_clicked_cb (GtkButton  *button,
	   		DialogData *data)
{
	GdkRectangle selection;

	gth_image_selector_get_selection (GTH_IMAGE_SELECTOR (data->image_selector), &selection);

	if ((selection.width > 0) && (selection.height > 0)) {
		GdkPixbuf *old_pixbuf;
		GdkPixbuf *new_pixbuf;

		old_pixbuf = gth_image_selector_get_pixbuf (GTH_IMAGE_SELECTOR (data->image_selector));
		gth_image_history_add_image (data->history, old_pixbuf, FALSE);
		g_object_ref (old_pixbuf);

		debug (DEBUG_INFO,
		       "(%d, %d) [%d, %d]\n",
		       selection.x,
		       selection.y,
		       selection.width,
		       selection.height);

		new_pixbuf = gdk_pixbuf_new_subpixbuf (old_pixbuf,
						       selection.x,
						       selection.y,
						       selection.width,
						       selection.height);
		if (new_pixbuf != NULL) {
			gth_image_selector_set_pixbuf (GTH_IMAGE_SELECTOR (data->image_selector), new_pixbuf);
			g_object_unref (new_pixbuf);
			gth_iviewer_zoom_to_fit (GTH_IVIEWER (data->image_selector));
		}
		g_object_unref (old_pixbuf);
	}
}


static void
mask_button_toggled_cb (GtkToggleButton *button,
			DialogData      *data)
{
	gth_image_selector_set_mask_visible (GTH_IMAGE_SELECTOR (data->image_selector), gtk_toggle_button_get_active (button));
}


static void
mask_visibility_changed_cb (GthImageSelector *selector,
			    DialogData       *data)
{
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->mask_button), gth_image_selector_get_mask_visible (GTH_IMAGE_SELECTOR (data->image_selector)));
}


static void
history_changed_cb (GthImageHistory *history,
		    DialogData      *data)
{
	gtk_widget_set_sensitive (data->undo_button, gth_image_history_can_undo (history));
	gtk_widget_set_sensitive (data->redo_button, gth_image_history_can_redo (history));
}


void
dlg_crop (GthWindow *window)
{
	DialogData   *data;
	GtkWidget    *ok_button;
	GtkWidget    *save_button;
	GtkWidget    *cancel_button;
	GtkWidget    *help_button;
	GtkWidget    *menu, *item;
	GtkWidget    *image;
	GtkWidget    *crop_button, *zoom_in_button, *zoom_out_button, *zoom_100_button, *zoom_fit_button;
	char         *label;
	GthCropRatio  crop_ratio;
	int           ratio_w, ratio_h;

	data = g_new0 (DialogData, 1);
	data->window = window;
	data->gui = glade_xml_new (GTHUMB_GLADEDIR "/" GLADE_FILE, NULL, NULL);

	if (! data->gui) {
		g_warning ("Could not find " GLADE_FILE "\n");
		g_free (data);
		return;
	}

	data->history = gth_image_history_new ();

	/* Get the widgets. */

	data->dialog = glade_xml_get_widget (data->gui, "crop_dialog");
	data->nav_container = glade_xml_get_widget (data->gui, "nav_container");

	data->crop_x_spinbutton = (GtkSpinButton*) glade_xml_get_widget (data->gui, "crop_x_spinbutton");
	data->crop_y_spinbutton = (GtkSpinButton*) glade_xml_get_widget (data->gui, "crop_y_spinbutton");
	data->crop_width_spinbutton = (GtkSpinButton*) glade_xml_get_widget (data->gui, "crop_width_spinbutton");
	data->crop_height_spinbutton = (GtkSpinButton*) glade_xml_get_widget (data->gui, "crop_height_spinbutton");
	data->ratio_optionmenu = glade_xml_get_widget (data->gui, "ratio_optionmenu");
	data->ratio_w_spinbutton = (GtkSpinButton*) glade_xml_get_widget (data->gui, "ratio_w_spinbutton");
	data->ratio_h_spinbutton = (GtkSpinButton*) glade_xml_get_widget (data->gui, "ratio_h_spinbutton");
	data->custom_ratio_box = glade_xml_get_widget (data->gui, "custom_ratio_box");
	data->invert_ratio_checkbutton = glade_xml_get_widget (data->gui, "invert_ratio_checkbutton");

	crop_button = glade_xml_get_widget (data->gui, "crop_crop_button");
	data->undo_button = glade_xml_get_widget (data->gui, "crop_undo_button");
	data->redo_button = glade_xml_get_widget (data->gui, "crop_redo_button");
	zoom_in_button = glade_xml_get_widget (data->gui, "crop_zoom_in_button");
	zoom_out_button = glade_xml_get_widget (data->gui, "crop_zoom_out_button");
	zoom_100_button = glade_xml_get_widget (data->gui, "crop_zoom_100_button");
	zoom_fit_button = glade_xml_get_widget (data->gui, "crop_zoom_fit_button");
	data->mask_button = glade_xml_get_widget (data->gui, "crop_showmask_togglebutton");

	ok_button = glade_xml_get_widget (data->gui, "crop_okbutton");
	save_button = glade_xml_get_widget (data->gui, "crop_savebutton");
	cancel_button = glade_xml_get_widget (data->gui, "crop_cancelbutton");
	help_button = glade_xml_get_widget (data->gui, "crop_helpbutton");

	data->image_selector = gth_image_selector_new (GTH_SELECTOR_TYPE_REGION, NULL);

	image = glade_xml_get_widget (data->gui, "crop_image");
	gtk_image_set_from_stock (GTK_IMAGE (image), GTHUMB_STOCK_CROP, GTK_ICON_SIZE_MENU);

	/* Set widgets data. */

	data->pixbuf = image_viewer_get_current_pixbuf (gth_window_get_image_viewer (data->window));
	data->image_width = gdk_pixbuf_get_width (data->pixbuf);
	data->image_height = gdk_pixbuf_get_height (data->pixbuf);
	gth_image_selector_set_pixbuf (GTH_IMAGE_SELECTOR (data->image_selector), data->pixbuf);

	data->nav_win = gth_nav_window_new (GTH_IVIEWER (data->image_selector));
	gtk_container_add (GTK_CONTAINER (data->nav_container), data->nav_win);

	gtk_widget_show_all (data->nav_container);

	/**/

	menu = gtk_option_menu_get_menu (GTK_OPTION_MENU (data->ratio_optionmenu));
	g_object_ref (menu);

	label = g_strdup_printf (_("%d x %d (Image)"), data->image_width, data->image_height);
	item = gtk_menu_item_new_with_label (label);
	gtk_widget_show (item);
	g_free (label);
	gtk_menu_shell_insert (GTK_MENU_SHELL (menu), item, 2);

	get_screen_size (GTK_WINDOW (window), &data->screen_width, &data->screen_height);

	label = g_strdup_printf (_("%d x %d (Screen)"), data->screen_width, data->screen_height);
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

	/* Update the crop viewer with all the crop ratio settings */
	ratio_optionmenu_changed_cb (GTK_OPTION_MENU (data->ratio_optionmenu),
				     data);

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
	g_signal_connect (G_OBJECT (save_button),
			  "clicked",
			  G_CALLBACK (save_cb),
			  data);
	g_signal_connect (G_OBJECT (help_button),
			  "clicked",
			  G_CALLBACK (help_cb),
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
	g_signal_connect (G_OBJECT (data->image_selector),
			  "selection_changed",
			  G_CALLBACK (selection_changed_cb),
			  data);
	g_signal_connect (G_OBJECT (data->image_selector),
			  "mask_visibility_changed",
			  G_CALLBACK (mask_visibility_changed_cb),
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
	g_signal_connect (G_OBJECT (data->invert_ratio_checkbutton),
			  "toggled",
			  G_CALLBACK (ratio_value_changed_cb),
			  data);

	g_signal_connect (G_OBJECT (crop_button),
			  "clicked",
			  G_CALLBACK (crop_button_clicked_cb),
			  data);
	g_signal_connect (G_OBJECT (data->undo_button),
			  "clicked",
			  G_CALLBACK (undo_button_clicked_cb),
			  data);
	g_signal_connect (G_OBJECT (data->redo_button),
			  "clicked",
			  G_CALLBACK (redo_button_clicked_cb),
			  data);
	g_signal_connect (G_OBJECT (zoom_in_button),
			  "clicked",
			  G_CALLBACK (zoom_in_button_clicked_cb),
			  data);
	g_signal_connect (G_OBJECT (zoom_out_button),
			  "clicked",
			  G_CALLBACK (zoom_out_button_clicked_cb),
			  data);
	g_signal_connect (G_OBJECT (zoom_100_button),
			  "clicked",
			  G_CALLBACK (zoom_100_button_clicked_cb),
			  data);
	g_signal_connect (G_OBJECT (zoom_fit_button),
			  "clicked",
			  G_CALLBACK (zoom_fit_button_clicked_cb),
			  data);
	g_signal_connect (G_OBJECT (data->mask_button),
			  "toggled",
			  G_CALLBACK (mask_button_toggled_cb),
			  data);
	g_signal_connect (G_OBJECT (data->history),
			  "changed",
			  G_CALLBACK (history_changed_cb),
			  data);

	/* Run dialog. */

	gtk_widget_realize (data->dialog);

	gtk_window_set_default_size (GTK_WINDOW (data->dialog),
				     data->screen_width * 7 / 10,
				     data->screen_height * 6 / 10);

	gth_iviewer_zoom_to_fit (GTH_IVIEWER (data->image_selector));
	gtk_widget_grab_focus (data->image_selector);

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog),
				      GTK_WINDOW (window));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), TRUE);
	gtk_widget_show (data->dialog);
}
