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
#include <gtk/gtk.h>
#include <libgnomeui/gnome-dateedit.h>
#include <glade/glade.h>
#include "file-data.h"
#include "file-utils.h"
#include "gthumb-window.h"

#ifdef HAVE_LIBEXIF
#include <exif-data.h>
#include <exif-content.h>
#include <exif-entry.h>
#endif /* HAVE_LIBEXIF */


#define GLADE_FILE "gthumb_tools.glade"


typedef struct {
	GThumbWindow *window;
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

static time_t
get_exif_time (const char *filename)
{
	ExifData     *edata;
	unsigned int  i, j;
	time_t        time = 0;
	struct tm     tm = { 0, };

	edata = exif_data_new_from_file (filename);

	if (edata == NULL) 
                return (time_t)0;

	for (i = 0; i < EXIF_IFD_COUNT; i++) {
		ExifContent *content = edata->ifd[i];

		if (! edata->ifd[i] || ! edata->ifd[i]->count) 
			continue;

		for (j = 0; j < content->count; j++) {
			ExifEntry   *e = content->entries[j];
			char        *data;

			if (! content->entries[j]) 
				continue;

			if ((e->tag != EXIF_TAG_DATE_TIME) &&
			    (e->tag != EXIF_TAG_DATE_TIME_ORIGINAL) &&
			    (e->tag != EXIF_TAG_DATE_TIME_DIGITIZED))
				continue;

			data = g_strdup (e->data);
			data[4] = data[7] = data[10] = data[13] = data[16] = '\0';

			tm.tm_year = atoi (data) - 1900;
			tm.tm_mon  = atoi (data + 5) - 1;
			tm.tm_mday = atoi (data + 8);
			tm.tm_hour = atoi (data + 11);
			tm.tm_min  = atoi (data + 14);
			tm.tm_sec  = atoi (data + 17);
			time = mktime (&tm);

			g_free (data);

			break;
		}
	}

	exif_data_unref (edata);

	return time;
}


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
	
	gtk_widget_destroy (data->dialog);
}


static void
radio_button_clicked (GtkWidget  *button, 
		      DialogData *data)
{
	gtk_widget_set_sensitive (data->cd_dateedit, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->cd_following_date_radiobutton)));
}


void
dlg_change_date (GThumbWindow *window)
{
	DialogData  *data;
	GtkWidget   *cancel_button;
	GtkWidget   *ok_button;
	GList       *list;

	list = gth_file_list_get_selection_as_fd (window->file_list);
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
				      GTK_WINDOW (window->app));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), TRUE); 
	gtk_widget_show (data->dialog);
}
