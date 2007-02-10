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
#include <libgnome/gnome-help.h>
#include <libgnomevfs/gnome-vfs-ops.h>
#include <glade/glade.h>

#include "file-utils.h"
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
		all_windows_notify_files_changed (data->files_changed_list);
		path_list_free (data->files_changed_list);
		data->files_changed_list = NULL;
	}

	all_windows_add_monitor ();

	path_list_free (data->file_list);
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


static void
notify_file_changed (DialogData *data,
		     const char *filename,
		     gboolean    notify_soon)
{
	if (notify_soon) {
		GList *list = g_list_prepend (NULL, (char*) filename);
		all_windows_notify_files_changed (list);
		g_list_free (list);
	} else 
		data->files_changed_list = g_list_prepend (data->files_changed_list, g_strdup (filename));
}



static void
apply_transformation (DialogData *data,
		      GList      *current_image,
		      gboolean    notify_soon)
{
	char             *path = current_image->data;
	GnomeVFSFileInfo  info;
	
	gnome_vfs_get_file_info (path, &info, GNOME_VFS_FILE_INFO_GET_ACCESS_RIGHTS|GNOME_VFS_FILE_INFO_FOLLOW_LINKS);
	write_orientation_field (path, GTH_TRANSFORM_NONE);

	gnome_vfs_set_file_info (path, &info, GNOME_VFS_SET_FILE_INFO_PERMISSIONS|GNOME_VFS_SET_FILE_INFO_OWNER);

	notify_file_changed (data, path, notify_soon);
}


static void
apply_transformation_to_all (DialogData *data)
{
	GladeXML  *gui;
	GtkWidget *dialog;
	GtkWidget *label;
	GtkWidget *bar;
	GList     *scan;
	int        i, n;

	gui = glade_xml_new (GTHUMB_GLADEDIR "/" PROGRESS_GLADE_FILE, 
			     NULL,
			     NULL);

	dialog = glade_xml_get_widget (gui, "progress_dialog");
	label = glade_xml_get_widget (gui, "progress_info");
	bar = glade_xml_get_widget (gui, "progress_progressbar");

	n = g_list_length (data->current_image);

	if (data->dialog == NULL)
		gtk_window_set_transient_for (GTK_WINDOW (dialog),
					      GTK_WINDOW (data->window));
	else {
		gtk_window_set_modal (GTK_WINDOW (data->dialog), FALSE); 
		gtk_window_set_transient_for (GTK_WINDOW (dialog),
					      GTK_WINDOW (data->dialog));
	}
	gtk_window_set_modal (GTK_WINDOW (dialog), TRUE); 
	gtk_widget_show (dialog);

	while (gtk_events_pending())
		gtk_main_iteration();

	i = 0;
	for (scan = data->current_image; scan; scan = scan->next) {
		char *path = scan->data;
		char *name;

		name = basename_for_display (path);
		_gtk_label_set_filename_text (GTK_LABEL (label), name);
		g_free (name);

		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (bar),
					       (gdouble) (i + 0.5) / n);
		
		while (gtk_events_pending())
			gtk_main_iteration();
		
		apply_transformation (data, scan, FALSE);
		i++;
	}
	
	gtk_widget_destroy (dialog);
	g_object_unref (gui);

	if (data->dialog == NULL)
		dialog_data_free (data);
	else
		gtk_widget_destroy (data->dialog);
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


	list = gth_window_get_file_list_selection (window);
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

	all_windows_remove_monitor ();

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), GTK_WINDOW (window));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), TRUE); 
	gtk_widget_show_all (data->dialog);
}


void
dlg_apply_reset_exif (GthWindow    *window,
		    GthTransform  rot_type,
		    GthTransform  tran_type)
{
	DialogData  *data;
	GList       *list;

	list = gth_window_get_file_list_selection (window);
	if (list == NULL) {
		g_warning ("No file selected.");
		return;
	}

	all_windows_remove_monitor ();

	data = g_new0 (DialogData, 1);
	data->window = window;
	data->file_list = list;
	data->current_image = list;

	apply_transformation_to_all (data);
}
