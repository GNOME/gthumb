/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 *  GThumb
 *
 *  Copyright (C) 2001, 2003, 2004, 2005 Free Software Foundation, Inc.
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
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gio/gio.h>
#include <libgnome/gnome-help.h>
#include <glade/glade.h>

#include "file-utils.h"
#include "gfile-utils.h"
#include "gth-utils.h"
#include "gth-window.h"
#include "gtk-utils.h"
#include "main.h"
#include "gthumb-stock.h"
#include "rotation-utils.h"


#define RESET_GLADE_FILE "gthumb_tools.glade"
#define PROGRESS_GLADE_FILE "gthumb_png_exporter.glade"


typedef struct {
	GthWindow    *window;
	GladeXML     *gui;

	GtkWidget    *dialog;
	GList        *file_list;
	GList        *files_changed_list;
	GList        *current_image;
} DialogData;


static void
dialog_data_free (DialogData *data)
{
	if (data->files_changed_list != NULL) {
		gth_monitor_notify_update_gfiles (GTH_MONITOR_EVENT_CHANGED, data->files_changed_list);
		gfile_list_free (data->files_changed_list);
		data->files_changed_list = NULL;
	}

	gth_monitor_resume ();

	file_data_list_free (data->file_list);
	if (data->gui != NULL)
		g_object_unref (data->gui);
	g_free (data);
}


/* called when the main dialog is closed. */
static void
destroy_cb (GtkWidget  *widget, 
	    DialogData *data)
{
	dialog_data_free (data);
}


typedef struct {
	DialogData *data;
	GladeXML   *gui;
	GtkWidget  *dialog;
	GtkWidget  *label;
	GtkWidget  *bar;
	GList      *scan;
	int         i, n;
	gboolean    cancel;
} BatchTransformation;


static void
apply_transformation (BatchTransformation *bt_data)
{
	while (bt_data->scan && (bt_data->cancel == FALSE)) {
		FileData *file = bt_data->scan->data;
	
		gtk_label_set_text (GTK_LABEL (bt_data->label), file->utf8_name);

		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (bt_data->bar),
					       (gdouble) (bt_data->i + 0.5) / bt_data->n);

	        while (gtk_events_pending())
        	        gtk_main_iteration();
			
		if (file_data_has_local_path (file, GTK_WINDOW (bt_data->data->window))) {
			write_orientation_field (file->local_path, GTH_TRANSFORM_NONE);
			bt_data->data->files_changed_list = g_list_prepend (bt_data->data->files_changed_list, file->gfile);
			g_object_ref (file->gfile);
		}

		bt_data->i++;	
		bt_data->scan = bt_data->scan->next;
	}

	if (GTK_IS_WIDGET (bt_data->dialog))
		gtk_widget_destroy (bt_data->dialog);

	g_object_unref (bt_data->gui);

	if (bt_data->data->dialog == NULL)
		dialog_data_free (bt_data->data);
	else
		gtk_widget_destroy (bt_data->data->dialog);

	g_free (bt_data);
}


static void
cancel_cb (GtkWidget           *dialog,
	   int		        response_id,
           BatchTransformation *bt_data)
{
	/* Close-dialog and cancel-button do the same thing, and
	   there are no other buttons, so any response to this
	   dialog causes a cancel action. */
        bt_data->cancel = TRUE;
}


static void
apply_transformation_to_all (DialogData *data)
{
	BatchTransformation *bt_data;
	GtkWidget           *progress_cancel;

	bt_data = g_new0 (BatchTransformation, 1);
	bt_data->data= data;
	bt_data->gui = glade_xml_new (GTHUMB_GLADEDIR "/" PROGRESS_GLADE_FILE,
			     	      NULL,
			     	      NULL);
	bt_data->dialog = glade_xml_get_widget (bt_data->gui, "progress_dialog");
	bt_data->label = glade_xml_get_widget (bt_data->gui, "progress_info");
	bt_data->bar = glade_xml_get_widget (bt_data->gui, "progress_progressbar");

	progress_cancel = glade_xml_get_widget (bt_data->gui, "progress_cancel");
	bt_data->cancel = FALSE;

	if (data->dialog == NULL)
		gtk_window_set_transient_for (GTK_WINDOW (bt_data->dialog),
					      GTK_WINDOW (data->window));
	else {
		gtk_window_set_modal (GTK_WINDOW (data->dialog), FALSE);
		gtk_window_set_transient_for (GTK_WINDOW (bt_data->dialog),
					      GTK_WINDOW (data->dialog));
	}

        g_signal_connect (G_OBJECT (bt_data->dialog),
                          "response",
                          G_CALLBACK (cancel_cb),
                          bt_data);

	gtk_window_set_modal (GTK_WINDOW (bt_data->dialog), TRUE);
	gtk_widget_show (bt_data->dialog);

	bt_data->n = g_list_length (data->current_image);
	bt_data->i = 0;
	bt_data->scan = data->current_image;
	apply_transformation (bt_data);
}


static void
ok_clicked (GtkWidget  *button, 
	    DialogData *data)
{
	gtk_widget_hide (data->dialog);
	apply_transformation_to_all (data);
}


/* called when the "help" button is clicked. */
static void
help_cb (GtkWidget  *widget, 
	 DialogData *data)
{
	gthumb_display_help (GTK_WINDOW (data->dialog), "gthumb-reset-exif");
}


void
dlg_reset_exif (GthWindow *window)
{
	DialogData  *data;
	GtkWidget   *x_help_button;
	GtkWidget   *x_cancel_button;
	GtkWidget   *x_ok_button;
	GList       *list;

	list = gth_window_get_file_list_selection_as_fd (window);
	if (list == NULL) {
		g_warning ("No file selected.");
		return;
	}

	data = g_new0 (DialogData, 1);

	data->window = window;
	data->file_list = list;
	data->current_image = list;
	data->gui = glade_xml_new (GTHUMB_GLADEDIR "/" RESET_GLADE_FILE, NULL,
				   NULL);

	if (! data->gui) {
		g_warning ("Could not find " RESET_GLADE_FILE "\n");
		if (data->file_list != NULL) 
			path_list_free (data->file_list);
		g_free (data);
		return;
	}

	data->dialog = glade_xml_get_widget (data->gui, "reset_exif_dialog");
	x_help_button = glade_xml_get_widget (data->gui, "x_help_button");
	x_cancel_button = glade_xml_get_widget (data->gui, "x_cancel_button");
	x_ok_button = glade_xml_get_widget (data->gui, "x_ok_button");

	/* Set the signals handlers. */

	g_signal_connect (G_OBJECT (data->dialog),
			  "destroy",
			  G_CALLBACK (destroy_cb),
			  data);

	g_signal_connect_swapped (G_OBJECT (x_cancel_button), 
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  G_OBJECT (data->dialog));
	g_signal_connect (G_OBJECT (x_help_button), 
			  "clicked",
			  G_CALLBACK (help_cb),
			  data);
	g_signal_connect (G_OBJECT (x_ok_button), 
			  "clicked",
			  G_CALLBACK (ok_clicked),
			  data);

	/* Run dialog. */

	gth_monitor_pause ();

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), GTK_WINDOW (window));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), TRUE); 
	gtk_widget_show_all (data->dialog);
}

