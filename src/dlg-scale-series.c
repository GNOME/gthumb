/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2005 Free Software Foundation, Inc.
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
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include <libgnomevfs/gnome-vfs-mime.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <glade/glade.h>

#include "async-pixbuf-ops.h"
#include "gth-browser.h"
#include "pixbuf-utils.h"
#include "file-data.h"
#include "file-utils.h"
#include "gconf-utils.h"
#include "gth-batch-op.h"
#include "preferences.h"


#define GLADE_FILE "gthumb_tools.glade"
#define DEF_TYPE "jpeg"


enum {
	UNIT_PIXEL,
	UNIT_PERCENT
};


typedef struct {
	GthWindow        *window;
	GladeXML         *gui;

	GList            *file_list;
	int               width, height;
	double            ratio;
	gboolean          percentage;
	char             *image_type;

	gboolean          remove_original;
	GthOverwriteMode  overwrite_mode;
	char             *destination;

	GthBatchOp       *bop;

	GtkWidget        *dialog;
	GtkWidget        *ss_width_spinbutton;
	GtkWidget        *ss_height_spinbutton;
	GtkWidget        *ss_unit_optionmenu;
	GtkWidget        *ss_keep_ratio_checkbutton;
	GtkWidget        *ss_dest_filechooserbutton;
	GtkWidget        *ss_om_combobox;
	GtkWidget        *ss_remove_orig_checkbutton;
	GtkWidget        *ss_jpeg_radiobutton;
	GtkWidget        *ss_png_radiobutton;
	GtkWidget        *ss_tga_radiobutton;
	GtkWidget        *ss_tiff_radiobutton;
} DialogData;


/* called when the main dialog is closed. */
static void
destroy_cb (GtkWidget  *widget, 
	    DialogData *data)
{
	if (data->bop != NULL)
		g_object_unref (data->bop);
	g_free (data->destination);
	file_data_list_free (data->file_list);
	g_object_unref (data->gui);
	g_free (data);
}


static void 
batch_op_done_cb (GthBatchOp *pixbuf_op,
		  gboolean    completed,
		  DialogData *data)
{
	gtk_widget_destroy (data->dialog);
}


/* called when the ok button is clicked. */
static void
ok_cb (GtkWidget  *widget, 
       DialogData *data)
{
	GthPixbufOp *pixop;
	char        *esc_path;
	gboolean     keep_ratio;
	int          width, height;

#define is_active(x) gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (x))

	if (is_active (data->ss_jpeg_radiobutton))
		data->image_type = "jpeg";
	else if (is_active (data->ss_png_radiobutton))
		data->image_type = "png";
	else if (is_active (data->ss_tga_radiobutton))
		data->image_type = "tga";
	else if (is_active (data->ss_tiff_radiobutton))
		data->image_type = "tiff";
	else
		data->image_type = DEF_TYPE;

	data->overwrite_mode = gtk_combo_box_get_active (GTK_COMBO_BOX (data->ss_om_combobox));
	data->remove_original = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->ss_remove_orig_checkbutton));

	/**/

	esc_path = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (data->ss_dest_filechooserbutton));
	data->destination = gnome_vfs_unescape_string (esc_path, "");
	g_free (esc_path);
	
	/* Save options */

	eel_gconf_set_string (PREF_CONVERT_IMAGE_TYPE, data->image_type);

	pref_set_convert_overwrite_mode (data->overwrite_mode);
	eel_gconf_set_boolean (PREF_CONVERT_REMOVE_ORIGINAL, data->remove_original);

	if (data->percentage)
		eel_gconf_set_string (PREF_SCALE_UNIT, "percentage");
	else
		eel_gconf_set_string (PREF_SCALE_UNIT, "pixels");

	keep_ratio = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->ss_keep_ratio_checkbutton));
	eel_gconf_set_boolean (PREF_SCALE_KEEP_RATIO, keep_ratio);

	width = gtk_spin_button_get_value (GTK_SPIN_BUTTON (data->ss_width_spinbutton));
	eel_gconf_set_integer (PREF_SCALE_SERIES_WIDTH, width);

	height = gtk_spin_button_get_value (GTK_SPIN_BUTTON (data->ss_height_spinbutton));
	eel_gconf_set_integer (PREF_SCALE_SERIES_HEIGHT, height);

	/**/

	pixop = _gdk_pixbuf_scale (NULL, NULL, 
				   data->percentage,
				   keep_ratio,
				   width,
				   height);
	data->bop = gth_batch_op_new (pixop, data);
	g_object_unref (pixop);

	g_signal_connect (data->bop,
			  "batch_op_done",
			  G_CALLBACK (batch_op_done_cb),
			  data);

	gth_batch_op_start (data->bop, 
			    data->image_type, 
			    data->file_list, 
			    data->destination,
			    data->overwrite_mode,
			    data->remove_original,
			    GTK_WINDOW (data->window));
}


static void
unit_changed (GtkOptionMenu *option_menu,
	      DialogData    *data)
{
	switch (gtk_option_menu_get_history (option_menu)) {
	case UNIT_PIXEL:
		if (! data->percentage)
			return;
		data->percentage = FALSE;
		break;
		
	case UNIT_PERCENT:
		if (data->percentage)
			return;
		data->percentage = TRUE;
		break;
	}
}


void
dlg_scale_series (GthBrowser *browser)
{
	GthWindow  *window = GTH_WINDOW (browser);
	DialogData *data;
	GtkWidget  *ok_button;
	GtkWidget  *cancel_button;
	GtkWidget  *button;
	char       *unit;
	char       *image_type;
	char       *esc_uri = NULL;
	int         default_width, default_height;

	data = g_new0 (DialogData, 1);
	data->window = window;
	data->gui = glade_xml_new (GTHUMB_GLADEDIR "/" GLADE_FILE, NULL,
				   NULL);

	if (! data->gui) {
		g_warning ("Could not find " GLADE_FILE "\n");
		g_free (data);
		return;
	}

	data->file_list = gth_window_get_file_list_selection_as_fd (window);
	g_return_if_fail (data->file_list != NULL);

	/* Get the widgets. */

	data->dialog = glade_xml_get_widget (data->gui, "scale_series_dialog");
	data->ss_width_spinbutton = glade_xml_get_widget (data->gui, "ss_width_spinbutton");
	data->ss_height_spinbutton = glade_xml_get_widget (data->gui, "ss_height_spinbutton");
	data->ss_keep_ratio_checkbutton = glade_xml_get_widget (data->gui, "ss_keep_ratio_checkbutton");
	data->ss_unit_optionmenu = glade_xml_get_widget (data->gui, "ss_unit_optionmenu");
	data->ss_jpeg_radiobutton = glade_xml_get_widget (data->gui, "ss_jpeg_radiobutton");
	data->ss_png_radiobutton = glade_xml_get_widget (data->gui, "ss_png_radiobutton");
	data->ss_tga_radiobutton = glade_xml_get_widget (data->gui, "ss_tga_radiobutton");
	data->ss_tiff_radiobutton = glade_xml_get_widget (data->gui, "ss_tiff_radiobutton");
	data->ss_om_combobox = glade_xml_get_widget (data->gui, "ss_om_combobox");
	data->ss_remove_orig_checkbutton = glade_xml_get_widget (data->gui, "ss_remove_orig_checkbutton");
	data->ss_dest_filechooserbutton = glade_xml_get_widget (data->gui, "ss_dest_filechooserbutton");

	ok_button = glade_xml_get_widget (data->gui, "ss_okbutton");
	cancel_button = glade_xml_get_widget (data->gui, "ss_cancelbutton");

	/* Set widgets data. */

	unit = eel_gconf_get_string (PREF_SCALE_UNIT, "pixels");

	data->percentage = strcmp (unit, "percentage") == 0;
	if (data->percentage) {
		gtk_option_menu_set_history (GTK_OPTION_MENU (data->ss_unit_optionmenu), 1);
		default_width = 100;
		default_height = 100;
	} else {
		gtk_option_menu_set_history (GTK_OPTION_MENU (data->ss_unit_optionmenu), 0);
		default_width = 640;
		default_height = 480;
	}

	gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->ss_width_spinbutton), eel_gconf_get_integer (PREF_SCALE_SERIES_WIDTH, default_width));
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->ss_height_spinbutton), eel_gconf_get_integer (PREF_SCALE_SERIES_HEIGHT, default_height));

	g_free (unit);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->ss_keep_ratio_checkbutton),
				      eel_gconf_get_boolean (PREF_SCALE_KEEP_RATIO, TRUE));

	/* image type */

	image_type = eel_gconf_get_string (PREF_CONVERT_IMAGE_TYPE, DEF_TYPE);
	if (image_type == NULL)
		button = data->ss_jpeg_radiobutton;
	else if (strcmp (image_type, "jpeg") == 0)
		button = data->ss_jpeg_radiobutton;
	else if (strcmp (image_type, "png") == 0)
		button = data->ss_png_radiobutton;
	else if (strcmp (image_type, "tga") == 0)
		button = data->ss_tga_radiobutton;
	else if (strcmp (image_type, "tiff") == 0)
		button = data->ss_tiff_radiobutton;
	else
		button = data->ss_jpeg_radiobutton;
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
	g_free (image_type);

	/**/

	gtk_combo_box_set_active (GTK_COMBO_BOX (data->ss_om_combobox),
				  pref_get_convert_overwrite_mode ());

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->ss_remove_orig_checkbutton), 
				      eel_gconf_get_boolean (PREF_CONVERT_REMOVE_ORIGINAL, FALSE));

	/**/

	esc_uri = gnome_vfs_escape_host_and_path_string (gth_browser_get_current_directory (browser));
	gtk_file_chooser_set_uri (GTK_FILE_CHOOSER (data->ss_dest_filechooserbutton), esc_uri);
	g_free (esc_uri);

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
	g_signal_connect (G_OBJECT (data->ss_unit_optionmenu),
			  "changed",
			  G_CALLBACK (unit_changed),
			  data);

	/* Run dialog. */

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog),
				      GTK_WINDOW (window));
	gtk_widget_show (data->dialog);
}
