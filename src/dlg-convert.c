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
#include <libgnomevfs/gnome-vfs-mime-sniff-buffer.h>
#include <glade/glade.h>
#include "file-data.h"
#include "gconf-utils.h"
#include "gthumb-window.h"
#include "gtk-utils.h"
#include "image-viewer.h"
#include "main.h"
#include "icons/pixbufs.h"
#include "jpegutils/jpegtran.h"
#include "pixbuf-utils.h"
#include "file-utils.h"
#include "dlg-save-image.h"


#define CONVERT_GLADE_FILE "gthumb_convert.glade"
#define PROGRESS_GLADE_FILE "gthumb_png_exporter.glade"
#define MAX_NAME_LEN 1024


typedef struct {
	GThumbWindow *window;
	GladeXML     *gui;
	GladeXML     *progress_gui;

	GtkWidget    *dialog;
	GtkWidget    *conv_jpeg_radiobutton;
	GtkWidget    *conv_png_radiobutton;
	GtkWidget    *conv_tga_radiobutton;
	GtkWidget    *conv_tiff_radiobutton;
	GtkWidget    *conv_om_optionmenu;
	GtkWidget    *conv_remove_orig_checkbutton;

	GtkWidget    *rename_dialog;
	GtkWidget    *conv_ren_message_label;
	GtkWidget    *conv_ren_name_entry;

	GtkWidget    *progress_dialog;
	GtkWidget    *progress_label;
	GtkWidget    *progress_bar;

	GList        *file_list;
	GList        *current_image;

	int           images;
	int           image;
	gboolean      remove_original;
	gboolean      stop_convertion;
	OverwriteMode overwrite_mode;

	ImageLoader  *loader;
	GdkPixbuf    *pixbuf;
	const char   *image_type;
	const char   *ext;
	char         *new_path;	
	char        **keys;
	char        **values;
} DialogData;


/* called when the main dialog is closed. */
static void
destroy_cb (GtkWidget  *widget, 
	    DialogData *data)
{
	if (data->rename_dialog)
		gtk_widget_destroy (data->rename_dialog);

	if (data->loader != NULL)
		g_object_unref (data->loader);

	if (data->file_list != NULL) {
		g_list_foreach (data->file_list, (GFunc) file_data_unref, NULL);
		g_list_free (data->file_list);
	}

	g_strfreev (data->keys);
	g_strfreev (data->values);	

	g_free (data->new_path);

	g_object_unref (data->gui);
	g_object_unref (data->progress_gui);

	g_free (data);
}


static void load_current_image (DialogData *data);


static void
load_next_image (DialogData *data)
{
	data->image++;
	data->current_image = g_list_next (data->current_image);
	load_current_image (data);
}


static void
load_current_image (DialogData *data)
{
	FileData  *fd;
	char      *folder;
	char      *name_no_ext;
	char      *utf8_name;
	char      *message;

	if (data->stop_convertion || (data->current_image == NULL)) {
		gtk_widget_destroy (data->progress_dialog);
		gtk_widget_destroy (data->dialog);
		return;
	}

	g_free (data->new_path);
	data->new_path = NULL;

	fd = (FileData*) data->current_image->data;
	folder = remove_level_from_path (fd->path);
	name_no_ext = remove_extension_from_path (file_name_from_path (fd->path));

	data->new_path = g_strconcat (folder, "/", name_no_ext, ".", data->ext, 0);

	g_free (folder);
	g_free (name_no_ext);

	if (strcmp (fd->path, data->new_path) == 0) {
		load_next_image (data);
		return;
	}

	utf8_name = g_locale_to_utf8 (file_name_from_path (data->new_path), -1, 0, 0, 0);
	message = g_strdup_printf (_("Converting image: %s"), utf8_name);

	gtk_label_set_text (GTK_LABEL (data->progress_label), message);

	g_free (utf8_name);
	g_free (message);

	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (data->progress_bar),
				       (double) data->image / data->images);
	
	image_loader_set_path (data->loader, fd->path);
	image_loader_start (data->loader);
}


static void
show_rename_dialog (DialogData *data)
{
	char  *message;
	char  *utf8_name;

	utf8_name = g_locale_to_utf8 (file_name_from_path (data->new_path), -1, 0, 0, 0);

	message = g_strdup_printf (_("An image named \"%s\" is already present. " "Please specify a different name."), utf8_name);
	
	_gtk_label_set_locale_text (GTK_LABEL (data->conv_ren_message_label), message);
	g_free (message);
	
	gtk_entry_set_text (GTK_ENTRY (data->conv_ren_name_entry), utf8_name);

	g_free (utf8_name);

	gtk_widget_show (data->rename_dialog);
	gtk_widget_grab_focus (data->conv_ren_name_entry);
}


static void
save_image_and_remove_original (DialogData *data)
{
	GError *error = NULL;

	if (_gdk_pixbuf_savev (data->pixbuf, 
			       data->new_path, 
			       data->image_type, 
			       data->keys, 
			       data->values,
			       &error)) {
		if (data->remove_original) {
			FileData *fd = data->current_image->data;
			unlink (fd->path);
		}
	} else 
		_gtk_error_dialog_from_gerror_run (GTK_WINDOW (data->dialog), &error);
}


static void
rename_response_cb (GtkWidget  *dialog,
		    int         response_id,
		    DialogData *data)
{
	gtk_widget_hide (dialog);

	if (response_id == GTK_RESPONSE_OK) {
		char *new_name, *folder;

		new_name = _gtk_entry_get_locale_text (GTK_ENTRY (data->conv_ren_name_entry));
		folder = remove_level_from_path (data->new_path);

		g_free (data->new_path);
		data->new_path = g_strconcat (folder, "/", new_name, NULL);
		g_free (folder);
		g_free (new_name);

		if (path_is_file (data->new_path)) {
			show_rename_dialog (data);
			return;
		} else 
			save_image_and_remove_original (data);
	}
	
	g_object_unref (data->pixbuf);
	data->pixbuf = NULL;

	load_next_image (data);
}


static void
overwrite_response_cb (GtkWidget  *dialog,
		       int         response_id,
		       DialogData *data)
{
	gtk_widget_destroy (dialog);

	if (response_id == GTK_RESPONSE_YES) 
		save_image_and_remove_original (data);

	g_object_unref (data->pixbuf);
	data->pixbuf = NULL;

	load_next_image (data);
}


static void
loader_done (ImageLoader *il,
	     DialogData  *data)
{
	gboolean save_image;

	data->pixbuf = image_loader_get_pixbuf (il);

	if (data->pixbuf == NULL) {
		load_next_image (data);
		return;
	}

	g_object_ref (data->pixbuf);

	save_image = TRUE;

	if (path_is_file (data->new_path)) {
		GtkWidget *d;
		char      *message;
		char      *utf8_name;

		switch (data->overwrite_mode) {
		case OVERWRITE_SKIP:
			save_image = FALSE;
			break;

		case OVERWRITE_OVERWRITE:
			save_image = TRUE;
			break;

		case OVERWRITE_ASK:
			utf8_name = g_locale_to_utf8 (file_name_from_path (data->new_path), -1, 0, 0, 0);
			message = g_strdup_printf (_("An image named \"%s\" is already present. " "Do you want to overwrite it ?"), utf8_name);

			d = _gtk_yesno_dialog_new (GTK_WINDOW (data->dialog),
						   GTK_DIALOG_MODAL,
						   message,
						   GTK_STOCK_CANCEL,
						   _("_Overwrite"));

			g_signal_connect (G_OBJECT (d), "response",
					  G_CALLBACK (overwrite_response_cb),
					  data);

			g_free (message);
			g_free (utf8_name);

			gtk_widget_show (d);
			return;
			
		case OVERWRITE_RENAME:
			show_rename_dialog (data);
			return;
		}
	} 

	if (save_image)
		save_image_and_remove_original (data);

	g_object_unref (data->pixbuf);
	data->pixbuf = NULL;

	load_next_image (data);	
}


static void
loader_error (ImageLoader *il,
	      DialogData  *data)
{
	load_next_image (data);
}


static void
ok_cb (GtkWidget  *widget, 
       DialogData *data)
{
	data->loader = IMAGE_LOADER (image_loader_new (NULL, FALSE));

	g_signal_connect (G_OBJECT (data->loader),
			  "done",
			  G_CALLBACK (loader_done),
			  data);

	g_signal_connect (G_OBJECT (data->loader),
			  "error",
			  G_CALLBACK (loader_error),
			  data);

	/**/

#define is_active(x) gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (x))

	if (is_active (data->conv_jpeg_radiobutton))
		data->image_type = data->ext = "jpeg";
	else if (is_active (data->conv_png_radiobutton))
		data->image_type = data->ext = "png";
	else if (is_active (data->conv_tga_radiobutton))
		data->image_type = data->ext = "tga";
	else if (is_active (data->conv_tiff_radiobutton))
		data->image_type = data->ext = "tiff";

	data->overwrite_mode = gtk_option_menu_get_history (GTK_OPTION_MENU (data->conv_om_optionmenu));
	data->remove_original = is_active (data->conv_remove_orig_checkbutton);

	/* Save options. */

	eel_gconf_set_string (PREF_CONVERT_IMAGE_TYPE, data->image_type);
	pref_set_convert_overwrite_mode (data->overwrite_mode);
	eel_gconf_set_boolean (PREF_CONVERT_REMOVE_ORIGINAL, data->remove_original);

	/**/

	gtk_widget_hide (data->dialog);

	if (dlg_save_options (GTK_WINDOW (data->dialog), 
			      data->image_type, 
			      &data->keys, 
			      &data->values)) {
		data->images = g_list_length (data->file_list);
		data->image = 1;
		gtk_widget_show (data->progress_dialog);
		load_current_image (data);
	} else
		gtk_widget_destroy (data->dialog);
}


static void
stop_convertion_cb (GtkWidget  *widget, 
		    DialogData *data)
{
	data->stop_convertion = TRUE;
}


void
dlg_convert (GThumbWindow *window)
{
	DialogData  *data;
	GtkWidget   *cancel_button;
	GtkWidget   *ok_button;
	GtkWidget   *button;
	GtkWidget   *progress_cancel;
	GList       *list;
	char        *image_type;

	list = file_list_get_selection_as_fd (window->file_list);
	if (list == NULL) {
		g_warning ("No file selected.");
		return;
	}

	/**/

	data = g_new0 (DialogData, 1);

	g_list_foreach (list, (GFunc) file_data_ref, NULL);
	data->file_list = list;
	data->current_image = list;

	data->gui = glade_xml_new (GTHUMB_GLADEDIR "/" CONVERT_GLADE_FILE, NULL,
				   NULL);

	if (! data->gui) {
		g_warning ("Could not find " CONVERT_GLADE_FILE "\n");
		if (data->file_list != NULL) 
			g_list_free (data->file_list);
		g_free (data);
		return;
	}

	data->progress_gui = glade_xml_new (GTHUMB_GLADEDIR "/" PROGRESS_GLADE_FILE, 
					    NULL, NULL);

	if (! data->progress_gui) {
		g_warning ("Could not find " PROGRESS_GLADE_FILE "\n");
		if (data->file_list != NULL) 
			g_list_free (data->file_list);
		g_object_unref (data->gui);
		g_free (data);
		return;
	}

	/* Get the widgets. */

	data->dialog = glade_xml_get_widget (data->gui, "convert_dialog");
	data->conv_jpeg_radiobutton = glade_xml_get_widget (data->gui, "conv_jpeg_radiobutton");
	data->conv_png_radiobutton = glade_xml_get_widget (data->gui, "conv_png_radiobutton");
	data->conv_tga_radiobutton = glade_xml_get_widget (data->gui, "conv_tga_radiobutton");
	data->conv_tiff_radiobutton = glade_xml_get_widget (data->gui, "conv_tiff_radiobutton");
	data->conv_om_optionmenu = glade_xml_get_widget (data->gui, "conv_om_optionmenu");
	data->conv_remove_orig_checkbutton = glade_xml_get_widget (data->gui, "conv_remove_orig_checkbutton");

	ok_button = glade_xml_get_widget (data->gui, "conv_ok_button");
	cancel_button = glade_xml_get_widget (data->gui, "conv_cancel_button");

	/**/

	data->rename_dialog = glade_xml_get_widget (data->gui, "convert_rename_dialog");
	data->conv_ren_message_label = glade_xml_get_widget (data->gui, "conv_ren_message_label");
	data->conv_ren_name_entry = glade_xml_get_widget (data->gui, "conv_ren_name_entry");

	/**/

	data->progress_dialog = glade_xml_get_widget (data->progress_gui, "progress_dialog");
	data->progress_label = glade_xml_get_widget (data->progress_gui, "progress_info");
	data->progress_bar = glade_xml_get_widget (data->progress_gui, "progress_progressbar");

	progress_cancel = glade_xml_get_widget (data->progress_gui, "progress_cancel");

	/* Set default values. */

	image_type = eel_gconf_get_string (PREF_CONVERT_IMAGE_TYPE);

	if (strcmp (image_type, "jpeg") == 0)
		button = data->conv_jpeg_radiobutton;
	else if (strcmp (image_type, "png") == 0)
		button = data->conv_png_radiobutton;
	else if (strcmp (image_type, "tga") == 0)
		button = data->conv_tga_radiobutton;
	else if (strcmp (image_type, "tiff") == 0)
		button = data->conv_tiff_radiobutton;
	else
		button = NULL;

	g_free (image_type);

	if (button != NULL)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);

	gtk_option_menu_set_history (GTK_OPTION_MENU (data->conv_om_optionmenu),
				     pref_get_convert_overwrite_mode ());

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->conv_remove_orig_checkbutton), 
				      eel_gconf_get_boolean (PREF_CONVERT_REMOVE_ORIGINAL));

	/* Set the signals handlers. */
	
	g_signal_connect (G_OBJECT (data->dialog),
			  "destroy",
			  G_CALLBACK (destroy_cb),
			  data);

	g_signal_connect_swapped (G_OBJECT (cancel_button), 
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  G_OBJECT (data->dialog));

	g_signal_connect (G_OBJECT (ok_button), 
			  "clicked",
			  G_CALLBACK (ok_cb),
			  data);

	g_signal_connect (G_OBJECT (progress_cancel), 
			  "clicked",
			  G_CALLBACK (stop_convertion_cb),
			  data);

	g_signal_connect (G_OBJECT (data->rename_dialog), 
			  "response",
			  G_CALLBACK (rename_response_cb),
			  data);

	/* Run dialog. */

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), 
				      GTK_WINDOW (window->app));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), TRUE); 
	gtk_widget_show (data->dialog);
}
