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
#include <string.h>
#include <gtk/gtk.h>
#include <libgnomevfs/gnome-vfs-mime.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <glade/glade.h>

#include "preferences.h"
#include "gtk-utils.h"
#include "gthumb-window.h"
#include "gconf-utils.h"
#include "pixbuf-utils.h"
#include "dlg-save-image.h"

#define IMAGE_TYPE_AUTOMATIC 0

typedef enum {
	IMAGE_TYPE_JPEG = 0,
	IMAGE_TYPE_PNG,
	IMAGE_TYPE_TGA,
	IMAGE_TYPE_TIFF
} ImageType;


static void
destroy_cb (GtkWidget *w,
	    GtkWidget *file_sel)
{
	GdkPixbuf *pixbuf;
	pixbuf = g_object_get_data (G_OBJECT (file_sel), "pixbuf");
	g_object_unref (pixbuf);
}


static void 
file_save_ok_cb (GtkWidget *w,
		 GtkWidget *file_sel)
{
	static char   *mime_types[4] = {"image/jpeg", "image/png", "image/tga", "image/tiff"};
	GThumbWindow  *window;
	GtkWindow     *parent;
	GtkWidget     *opt_menu;
	GdkPixbuf     *pixbuf;
	char          *filename;
	gboolean       file_exists;
	const char    *mime_type;
	int            idx;

	window = g_object_get_data (G_OBJECT (file_sel), "gthumb_window");
	pixbuf = g_object_get_data (G_OBJECT (file_sel), "pixbuf");
	filename = g_strdup (gtk_file_selection_get_filename (GTK_FILE_SELECTION (file_sel)));

	parent = GTK_WINDOW (window->app);

	if (filename == NULL)
		return;
	
	file_exists = path_is_file (filename);

	if (file_exists) { 
		GtkWidget *d;
		char      *message;
		int        r;

		message = g_strdup_printf (_("An image named \"%s\" is already present. Do you want to overwrite it?"), file_name_from_path (filename));
		d = _gtk_yesno_dialog_new (parent, 
					   GTK_DIALOG_MODAL,
					   message,
					   GTK_STOCK_NO,
					   GTK_STOCK_YES);
		g_free (message);
		
		r = gtk_dialog_run (GTK_DIALOG (d));
		gtk_widget_destroy (d);

		if (r != GTK_RESPONSE_YES) {
			g_free (filename);
			return;
		}
	}

	gtk_widget_hide (file_sel);

	opt_menu = g_object_get_data (G_OBJECT (file_sel), "opt_menu");
	idx = gtk_option_menu_get_history (GTK_OPTION_MENU (opt_menu));

	if (idx == IMAGE_TYPE_AUTOMATIC)
		mime_type = gnome_vfs_mime_type_from_name (filename);
	else
		mime_type = mime_types [idx - 2];

	if ((mime_type != NULL)
	    && strncmp (mime_type, "image/", 6) == 0) {
		GError      *error = NULL;
		const char  *image_type = mime_type + 6;
		char       **keys = NULL;
		char       **values = NULL;
		
		if (dlg_save_options (parent,
				      image_type,
				      &keys,
				      &values))
			if (! _gdk_pixbuf_savev (pixbuf, 
						 filename, 
						 image_type, 
						 keys, values,
						 &error)) 
				_gtk_error_dialog_from_gerror_run (parent, 
								   &error);
		
		g_strfreev (keys);
		g_strfreev (values);
	} else
		_gtk_error_dialog_run (parent,
				       _("Image type not supported: %s"),
				       mime_type);
	
	g_free (filename);
	gtk_widget_destroy (file_sel);
}


static GtkWidget *
build_file_type_menu ()
{
	static char *type_name[] = { "JPEG", "PNG", "TGA", "TIFF", NULL };
        GtkWidget   *menu;
        GtkWidget   *item;
	int          i;

        menu = gtk_menu_new ();

	item = gtk_menu_item_new_with_label (_("Determine by extension"));
	gtk_widget_show (item);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

	item = gtk_menu_item_new ();
	gtk_widget_show (item);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

        for (i = 0; type_name[i] != NULL; i++) {
                item = gtk_menu_item_new_with_label (type_name[i]);
		gtk_widget_show (item);
                gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
        }

	return menu;
}


void
dlg_save_image (GThumbWindow *window, 
		GdkPixbuf    *pixbuf)
{
	GtkWidget *file_sel;
	GtkWidget *hbox;
	GtkWidget *opt_menu;
	GtkWidget *menu;
	char      *path;

	g_return_if_fail (pixbuf != NULL);

	file_sel = gtk_file_selection_new (_("Save Image"));
	
	/**/

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 0);
	gtk_box_pack_start (GTK_BOX (GTK_FILE_SELECTION (file_sel)->action_area), hbox, 
			    TRUE, TRUE, 0);

	opt_menu = gtk_option_menu_new ();
	menu = build_file_type_menu (window);
        gtk_option_menu_set_menu (GTK_OPTION_MENU (opt_menu), menu);
	gtk_box_pack_end (GTK_BOX (hbox), opt_menu, FALSE, FALSE, 0);

	gtk_box_pack_end (GTK_BOX (hbox), gtk_label_new (_("Image type:")), FALSE, FALSE, 5);

	gtk_widget_show_all (hbox);

	/**/

	if ((window != NULL) && (window->image_path != NULL))
		path = g_strdup (window->image_path);
	else if ((window != NULL) && (window->dir_list->path != NULL))
		path = g_strconcat (window->dir_list->path,
				    "/",
				    NULL);
	else
		path = g_strconcat (g_get_home_dir (),
				    "/",
				    NULL);

	gtk_file_selection_set_filename (GTK_FILE_SELECTION (file_sel), path);
	g_free (path);

	g_object_ref (pixbuf);

	g_object_set_data (G_OBJECT (file_sel), "gthumb_window", window);
	g_object_set_data (G_OBJECT (file_sel), "pixbuf", pixbuf);
	g_object_set_data (G_OBJECT (file_sel), "opt_menu", opt_menu);

	g_signal_connect (G_OBJECT (GTK_FILE_SELECTION (file_sel)->ok_button),
			  "clicked", 
			  G_CALLBACK (file_save_ok_cb), 
			  file_sel);

	g_signal_connect_swapped (G_OBJECT (GTK_FILE_SELECTION (file_sel)->cancel_button),
				  "clicked", 
				  G_CALLBACK (gtk_widget_destroy),
				  G_OBJECT (file_sel));

	g_signal_connect (G_OBJECT (file_sel),
			  "destroy", 
			  G_CALLBACK (destroy_cb),
			  file_sel);
    
	if (window != NULL) {
		gtk_window_set_transient_for (GTK_WINDOW (file_sel), 
					      GTK_WINDOW (window->app));
		gtk_window_set_modal (GTK_WINDOW (file_sel), TRUE);
	}

	gtk_widget_show (file_sel);
}




#define GLADE_FILE     "gthumb_convert.glade"


typedef struct {
	char      *name;
	ImageType  type;
	int        n_params;
} TypeEntry;


TypeEntry types_table[] = {
	{ "jpeg", IMAGE_TYPE_JPEG, 4 },
	{ "png", IMAGE_TYPE_PNG, 0 },
	{ "tiff", IMAGE_TYPE_TIFF, 3 },
	{ "tga", IMAGE_TYPE_TGA, 1 },
	{ NULL, 0, 0 }
};


static TypeEntry*
get_type_from_name (const char *name)
{
	int i;

	for (i = 0; types_table[i].name != NULL; i++)
		if (strcmp (types_table[i].name, name) == 0) 
			return types_table + i;
	
	return NULL;
}


gboolean
dlg_save_options (GtkWindow    *parent,
		  const char   *image_type,
		  char       ***keys,
		  char       ***values)
{
	GtkWidget  *dialog;
	GladeXML   *gui;
	char       *dialog_name;
	gboolean    retval = TRUE;
	TypeEntry  *type;

	*keys   = NULL;
	*values = NULL;	

	if (image_type == NULL) {
		g_warning ("Invalid image type\n");
		return FALSE;
	}

	if (strncmp (image_type, "x-", 2) == 0) 
		image_type += 2;

	type = get_type_from_name (image_type); 
	if (type == NULL) {
		g_warning ("Invalid image type\n");
		return FALSE;
	}

	if (type->n_params == 0)
		return TRUE;

	gui = glade_xml_new (GTHUMB_GLADEDIR "/" GLADE_FILE, NULL, NULL);
	if (! gui) {
		g_warning ("Could not find " GLADE_FILE "\n");
		return FALSE;
	}

	dialog_name = g_strconcat (image_type, "_options_dialog", NULL);

	dialog = glade_xml_get_widget (gui, dialog_name);
	g_free (dialog_name);

	/* Set default values */

	switch (type->type) {
		GtkWidget *widget;
		char      *svalue;
		int        ivalue;

	case IMAGE_TYPE_JPEG:
		ivalue = eel_gconf_get_integer (PREF_JPEG_QUALITY);
		widget = glade_xml_get_widget (gui, "jpeg_quality_hscale");
		gtk_range_set_value (GTK_RANGE (widget), (double) ivalue);
		
		/**/

		ivalue = eel_gconf_get_integer (PREF_JPEG_SMOOTHING);
		widget = glade_xml_get_widget (gui, "jpeg_smooth_hscale");
		gtk_range_set_value (GTK_RANGE (widget), (double) ivalue);
		
		/**/

		ivalue = eel_gconf_get_boolean (PREF_JPEG_OPTIMIZE);
		widget = glade_xml_get_widget (gui, "jpeg_optimize_checkbutton");
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), ivalue);
		
		/**/

		ivalue = eel_gconf_get_boolean (PREF_JPEG_PROGRESSIVE);
		widget = glade_xml_get_widget (gui, "jpeg_progressive_checkbutton");
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), ivalue);
		break;
		
	case IMAGE_TYPE_PNG:
		break;
		
	case IMAGE_TYPE_TIFF:
		svalue = eel_gconf_get_string (PREF_TIFF_COMPRESSION); 

		if (svalue == NULL)
			widget = NULL;
		else if (strcmp (svalue, "none") == 0) 
			widget = glade_xml_get_widget (gui, "tiff_comp_none_radiobutton");
		else if (strcmp (svalue, "deflate") == 0) 
			widget = glade_xml_get_widget (gui, "tiff_comp_deflate_radiobutton");
		else if (strcmp (svalue, "jpeg") == 0) 
			widget = glade_xml_get_widget (gui, "tiff_comp_jpeg_radiobutton");
		else
			widget = NULL;
		
		if (widget != NULL)
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), TRUE);

		g_free (svalue);
		
		/**/

		ivalue = eel_gconf_get_integer (PREF_TIFF_HORIZONTAL_RES);
		widget = glade_xml_get_widget (gui, "tiff_hdpi_spinbutton");
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (widget), (double) ivalue);

		/**/

		ivalue = eel_gconf_get_integer (PREF_TIFF_VERTICAL_RES);
		widget = glade_xml_get_widget (gui, "tiff_vdpi_spinbutton");
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (widget), (double) ivalue);
		
		break;
		
	case IMAGE_TYPE_TGA:
		ivalue = eel_gconf_get_boolean (PREF_TGA_RLE_COMPRESSION);
		widget = glade_xml_get_widget (gui, "tga_rle_compression_checkbutton");
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), ivalue);
		break;

	default:
		break;
	}

	/* Run dialog. */

	if (parent != NULL)
		gtk_window_set_transient_for (GTK_WINDOW (dialog), parent);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK) {
		GtkWidget *widget;
		int        ivalue, i = 0;

		*keys   = g_malloc (sizeof (char*) * (type->n_params + 1));
		*values = g_malloc (sizeof (char*) * (type->n_params + 1));
		
		switch (type->type) {
		case IMAGE_TYPE_JPEG:
			(*keys)[i] = g_strdup ("quality");

			widget = glade_xml_get_widget (gui, "jpeg_quality_hscale");
			ivalue = gtk_range_get_value (GTK_RANGE (widget));
			(*values)[i] = g_strdup_printf ("%d", ivalue);

			eel_gconf_set_integer (PREF_JPEG_QUALITY, ivalue);

			/**/

			i++;
			(*keys)[i] = g_strdup ("smooth");

			widget = glade_xml_get_widget (gui, "jpeg_smooth_hscale");
			ivalue = gtk_range_get_value (GTK_RANGE (widget));
			(*values)[i] = g_strdup_printf ("%d", ivalue);

			eel_gconf_set_integer (PREF_JPEG_SMOOTHING, ivalue);

			/**/

			i++;
			(*keys)[i] = g_strdup ("optimize");

			widget = glade_xml_get_widget (gui, "jpeg_optimize_checkbutton");
			ivalue = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));
			(*values)[i] = g_strdup_printf (ivalue ? "yes" : "no");

			eel_gconf_set_boolean (PREF_JPEG_OPTIMIZE, ivalue);

			/**/

			i++;
			(*keys)[i] = g_strdup ("progressive");

			widget = glade_xml_get_widget (gui, "jpeg_progressive_checkbutton");
			ivalue = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));
			(*values)[i] = g_strdup_printf (ivalue ? "yes" : "no");

			eel_gconf_set_boolean (PREF_JPEG_PROGRESSIVE, ivalue);

			break;
			
		case IMAGE_TYPE_PNG:
			break;
			
		case IMAGE_TYPE_TIFF:
			(*keys)[i] = g_strdup ("compression");

			widget = glade_xml_get_widget (gui, "tiff_comp_none_radiobutton");
			ivalue = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));
			if (ivalue)
				(*values)[i] = g_strdup_printf ("none");

			widget = glade_xml_get_widget (gui, "tiff_comp_deflate_radiobutton");
			ivalue = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));
			if (ivalue)
				(*values)[i] = g_strdup_printf ("deflate");

			widget = glade_xml_get_widget (gui, "tiff_comp_jpeg_radiobutton");
			ivalue = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));
			if (ivalue)
				(*values)[i] = g_strdup_printf ("jpeg");

			eel_gconf_set_string (PREF_TIFF_COMPRESSION, (*values)[i]); 

			/**/

			i++;
			(*keys)[i] = g_strdup ("horizontal dpi");

			widget = glade_xml_get_widget (gui, "tiff_hdpi_spinbutton");
			ivalue = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (widget));
			(*values)[i] = g_strdup_printf ("%d", ivalue);

			eel_gconf_set_integer (PREF_TIFF_HORIZONTAL_RES, ivalue);

			/**/

			i++;
			(*keys)[i] = g_strdup ("vertical dpi");

			widget = glade_xml_get_widget (gui, "tiff_vdpi_spinbutton");
			ivalue = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (widget));
			(*values)[i] = g_strdup_printf ("%d", ivalue);

			eel_gconf_set_integer (PREF_TIFF_VERTICAL_RES, ivalue);

			break;
			
		case IMAGE_TYPE_TGA:
			(*keys)[i] = g_strdup ("compression");

			widget = glade_xml_get_widget (gui, "tga_rle_compression_checkbutton");
			ivalue = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));
			if (ivalue)
				(*values)[i] = g_strdup_printf ("rle");
			else
				(*values)[i] = g_strdup_printf ("none");
			
			eel_gconf_set_boolean (PREF_TGA_RLE_COMPRESSION, strcmp ((*values)[i], "rle") == 0);

			break;

		default:
			break;
		}

		i++;
		(*keys)[i] = NULL;
		(*values)[i] = NULL;

	} else
		retval = FALSE;

	/**/
	    
	gtk_widget_destroy (dialog);
	g_object_unref (gui);

	return retval;
}
