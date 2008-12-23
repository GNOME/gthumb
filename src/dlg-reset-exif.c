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


/* -- apply_transformation -- */


typedef struct {
	DialogData       *data;
	GList            *current_image;
	GFileInfo        *info;
	gboolean          notify_soon;
	CopyDoneFunc      done_func;
	gpointer          done_data;
} ApplyTransformData;


static void
notify_file_changed (DialogData *data,
		     const char *filename,
		     gboolean    notify_soon)
{
	if (notify_soon) {
		GList *list = g_list_prepend (NULL, (char*) filename);
		all_windows_notify_files_changed (list);
		g_list_free (list);
	} 
	else
		data->files_changed_list = g_list_prepend (data->files_changed_list, g_strdup (filename));
}


static void
apply_transformation_done (const char     *uri,
			   GError         *error,
		           gpointer        callback_data)
{
	ApplyTransformData *at_data = callback_data;
	FileData           *file = at_data->current_image->data;
	GFile              *gfile = g_file_new_for_uri (file->path);
		
	if (error == NULL) {
		if (at_data->info != NULL)
			g_file_set_attributes_from_info (gfile, at_data->info, G_FILE_QUERY_INFO_NONE, NULL, NULL);
		notify_file_changed (at_data->data, file->path, at_data->notify_soon);
	}
	else 
		_gtk_error_dialog_run (GTK_WINDOW (at_data->data->window), _("Could not move temporary file to remote location. Check remote permissions."));
	
	if (at_data->done_func)
		(at_data->done_func) (uri, error, at_data->done_data);

	if (at_data->info != NULL)
		g_object_unref (at_data->info);
	g_free (at_data);
	g_object_unref (gfile);
}


static void
apply_transformation__step2 (const char     *uri,
			     GError         *error,
		             gpointer        callback_data)
{
	ApplyTransformData *at_data = callback_data;
	FileData           *file = at_data->current_image->data;
	char		   *local_file = NULL;

	local_file = get_cache_filename_from_uri (file->path);
	write_orientation_field (local_file, GTH_TRANSFORM_NONE);
	g_free (local_file);
	
	update_file_from_cache (file, apply_transformation_done, at_data);
}


static void
apply_transformation (DialogData   *data,
		      GList        *current_image,
		      gboolean      notify_soon,
		      CopyDoneFunc  done_func,
		      gpointer      done_data)
{
	GFile              *gfile;
	GError             *error = NULL;
	FileData           *file = current_image->data;
	ApplyTransformData *at_data;

	at_data = g_new0 (ApplyTransformData, 1);
	at_data->data = data;
	at_data->current_image = current_image;
	at_data->notify_soon = notify_soon;
	at_data->done_func = done_func;
	at_data->done_data = done_data;
	gfile = g_file_new_for_uri (file->path);
	at_data->info = g_file_query_info (gfile, "owner::*,access::*", G_FILE_QUERY_INFO_NONE, NULL, &error);
	g_object_unref (gfile);
	if (error) {
		g_object_unref (at_data->info);
		at_data->info = NULL;
	}

	copy_remote_file_to_cache (file, apply_transformation__step2, at_data);
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


static void apply_transformation_to_all__apply_to_current (BatchTransformation *);


static void
apply_transformation_to_all_continue (const char     *uri,
				      GError         *error,
				      gpointer       data)
{
	BatchTransformation *bt_data = data;

	if ((bt_data->cancel == TRUE) || (bt_data->scan == NULL)) {
		if (GTK_IS_WIDGET (bt_data->dialog))
			gtk_widget_destroy (bt_data->dialog);
		g_object_unref (bt_data->gui);

		if (bt_data->data->dialog == NULL)
			dialog_data_free (bt_data->data);
		else
			gtk_widget_destroy (bt_data->data->dialog);
		g_free (bt_data);
	}
	else
		apply_transformation_to_all__apply_to_current (bt_data);
}


static void
apply_transformation_to_all__apply_to_current (BatchTransformation *bt_data)
{
	FileData *file = bt_data->scan->data;
	char     *name;
	
	if (bt_data->cancel == FALSE) {
		name = basename_for_display (file->path);
		_gtk_label_set_filename_text (GTK_LABEL (bt_data->label), name);
		g_free (name);

		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (bt_data->bar),
					       (gdouble) (bt_data->i + 0.5) / bt_data->n);
	
		apply_transformation (bt_data->data, bt_data->scan, FALSE, apply_transformation_to_all_continue, bt_data);
	
		bt_data->i++;	
		bt_data->scan = bt_data->scan->next;
	}
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
	apply_transformation_to_all__apply_to_current (bt_data);
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

	all_windows_remove_monitor ();

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), GTK_WINDOW (window));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), TRUE); 
	gtk_widget_show_all (data->dialog);
}


void
dlg_apply_reset_exif (GthWindow *window)
{
	DialogData  *data;
	GList       *list;

	list = gth_window_get_file_list_selection_as_fd (window);
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
