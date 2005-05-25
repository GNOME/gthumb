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
#include <time.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <libgnome/gnome-help.h>
#include <libgnomeui/gnome-dateedit.h>
#include <glade/glade.h>

#include "file-data.h"
#include "gth-exif-utils.h"
#include "file-utils.h"
#include "gth-window.h"
#include "main.h"


#define GLADE_FILE "gthumb_tools.glade"


typedef struct {
	GthWindow    *window;
	GladeXML     *gui;

	GtkWidget    *dialog;
	GtkWidget    *cd_following_date_radiobutton;
	GtkWidget    *cd_created_radiobutton;
	GtkWidget    *cd_current_radiobutton;
	GtkWidget    *cd_exif_radiobutton;

	GtkWidget    *cd_dateedit;

	GList        *file_list;
} DialogData;


/* called when the main dialog is closed. */
static void
destroy_cb (GtkWidget  *widget, 
	    DialogData *data)
{
	file_data_list_free (data->file_list);
	g_object_unref (data->gui);
	g_free (data);
}


#ifdef HAVE_LIBEXIF

static gboolean
exif_time_available (DialogData *data)
{
	FileData *fd;

	if (data->file_list == NULL)
		return FALSE;

	if (data->file_list->next != NULL)
		return TRUE;

	fd = data->file_list->data;

	return get_exif_time (fd->path) != 0;
}


#endif /* HAVE_LIBEXIF */

#define is_active(x) (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (x)))


static void
ok_clicked (GtkWidget  *button, 
	    DialogData *data)
{
	GList  *scan;
	time_t  mtime = 0;

	if (is_active (data->cd_following_date_radiobutton)) 
		mtime = gnome_date_edit_get_time (GNOME_DATE_EDIT (data->cd_dateedit));
	else if (is_active (data->cd_current_radiobutton)) 
		time (&mtime);
	
	all_windows_remove_monitor ();

	for (scan = data->file_list; scan; scan = scan->next) {
		FileData *fdata = scan->data;
		
		if (is_active (data->cd_created_radiobutton)) 
			mtime = get_file_ctime (fdata->path);
#ifdef HAVE_LIBEXIF
		else if (is_active (data->cd_exif_radiobutton)) 
			mtime = get_exif_time (fdata->path);
#endif /* HAVE_LIBEXIF */
		
		if (mtime > 0) 
			set_file_mtime (fdata->path, mtime);
	}

	all_windows_notify_files_changed (data->file_list);
	all_windows_add_monitor ();
	
	gtk_widget_destroy (data->dialog);
}


static void
radio_button_clicked (GtkWidget  *button, 
		      DialogData *data)
{
	gtk_widget_set_sensitive (data->cd_dateedit, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->cd_following_date_radiobutton)));
}



/* called when the "help" button is clicked. */
static void
help_cb (GtkWidget  *widget, 
	 DialogData *data)
{
	GError *err;

	err = NULL;  
	gnome_help_display ("gthumb", "gthumb-change-date", &err);
	
	if (err != NULL) {
		GtkWidget *dialog;
		
		dialog = gtk_message_dialog_new (GTK_WINDOW (data->dialog),
						 0,
						 GTK_MESSAGE_ERROR,
						 GTK_BUTTONS_CLOSE,
						 _("Could not display help: %s"),
						 err->message);
		
		g_signal_connect (G_OBJECT (dialog), "response",
				  G_CALLBACK (gtk_widget_destroy),
				  NULL);
		
		gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
		
		gtk_widget_show (dialog);
		
		g_error_free (err);
	}
}


void
dlg_change_date (GthWindow *window)
{
	DialogData  *data;
	GtkWidget   *cancel_button;
	GtkWidget   *ok_button;
	GtkWidget   *help_button;
	GList       *list;

	list = gth_window_get_file_list_selection_as_fd (window);
	if (list == NULL) {
		g_warning ("No file selected.");
		return;
	}

	/**/

	data = g_new (DialogData, 1);

	data->file_list = list;
	data->gui = glade_xml_new (GTHUMB_GLADEDIR "/" GLADE_FILE, NULL,
				   NULL);

	if (! data->gui) {
		g_warning ("Could not find " GLADE_FILE "\n");
		if (data->file_list != NULL) 
			g_list_free (data->file_list);
		g_free (data);
		return;
	}

	/* Get the widgets. */

	data->dialog = glade_xml_get_widget (data->gui, "change_date_dialog");
	data->cd_following_date_radiobutton = glade_xml_get_widget (data->gui, "cd_following_date_radiobutton");
	data->cd_created_radiobutton = glade_xml_get_widget (data->gui, "cd_created_radiobutton");
	data->cd_current_radiobutton = glade_xml_get_widget (data->gui, "cd_current_radiobutton");
	data->cd_exif_radiobutton = glade_xml_get_widget (data->gui, "cd_exif_radiobutton");
	data->cd_dateedit = glade_xml_get_widget (data->gui, "cd_dateedit");

	cancel_button = glade_xml_get_widget (data->gui, "cd_cancel_button");
	ok_button = glade_xml_get_widget (data->gui, "cd_ok_button");
	help_button = glade_xml_get_widget (data->gui, "cd_help_button");

	/* Set widgets data. */

#ifndef HAVE_LIBEXIF
	gtk_widget_hide (data->cd_exif_radiobutton);
#else /* HAVE_LIBEXIF */
	gtk_widget_set_sensitive (data->cd_exif_radiobutton, exif_time_available (data));
#endif /* HAVE_LIBEXIF */

	gtk_widget_set_sensitive (data->cd_dateedit, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->cd_following_date_radiobutton)));

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
			  G_CALLBACK (ok_clicked),
			  data);
	g_signal_connect (G_OBJECT (help_button), 
			  "clicked",
			  G_CALLBACK (help_cb),
			  data);
	
	g_signal_connect (G_OBJECT (data->cd_following_date_radiobutton), 
			  "clicked",
			  G_CALLBACK (radio_button_clicked),
			  data);
	g_signal_connect (G_OBJECT (data->cd_created_radiobutton), 
			  "clicked",
			  G_CALLBACK (radio_button_clicked),
			  data);
	g_signal_connect (G_OBJECT (data->cd_current_radiobutton), 
			  "clicked",
			  G_CALLBACK (radio_button_clicked),
			  data);
	g_signal_connect (G_OBJECT (data->cd_exif_radiobutton), 
			  "clicked",
			  G_CALLBACK (radio_button_clicked),
			  data);

	/* Run dialog. */

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), 
				      GTK_WINDOW (window));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), TRUE); 
	gtk_widget_show (data->dialog);
}
