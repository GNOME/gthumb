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
#include <unistd.h>
#include <libgnome/libgnome.h>
#include <libgnomeui/gnome-pixmap.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <glade/glade.h>
#include <gtk/gtk.h>
#include "thumb-cache.h"
#include "comments.h"
#include "dlg-file-utils.h"
#include "file-utils.h"
#include "glib-utils.h"
#include "image-viewer.h"
#include "main.h"
#include "gconf-utils.h"
#include "gtk-utils.h"
#include "gthumb-window.h"
#include "gth-folder-selection-dialog.h"

#define GLADE_EXPORTER_FILE "gthumb_png_exporter.glade"
#define FILE_NAME_MAX_LENGTH 30


enum {
	OVERWRITE_YES,
	OVERWRITE_NO,
	OVERWRITE_ALL,
	OVERWRITE_NONE
};


static int dlg_overwrite    (GThumbWindow *window,
			     int           default_overwrite_mode,
			     char         *old_filename, 
			     char         *new_filename,
			     gboolean      show_overwrite_all_none);


static void
dlg_show_error (GThumbWindow *window,
		const char   *first_line,
		GList        *error_list, 
		const char   *reason)
{
	const int   max_files = 15; /* FIXME */
	GtkWidget  *d;
	GList      *scan;
	char       *msg;
	char       *utf8_msg;
	int         n = 0;
	
	msg = g_strdup_printf ("%s\n", first_line);
	for (scan = error_list; scan && (n < max_files); scan = scan->next) {
		char *tmp = msg;
		msg = g_strconcat (tmp, "  ", scan->data, "\n", NULL);
		g_free (tmp);
		n++;
	}
	
	if (scan != NULL) {
		char *tmp = msg;
		msg = g_strconcat (tmp, "...\n", NULL);
		g_free (tmp);
	}

	if (reason != NULL) {
		char *tmp = msg;
		msg = g_strconcat (tmp, reason, NULL);
		g_free (tmp);
	}

	utf8_msg = g_locale_to_utf8 (msg, -1, NULL, NULL, NULL);

	d = _gtk_message_dialog_new (GTK_WINDOW (window->app),
				     GTK_DIALOG_MODAL,
				     GTK_STOCK_DIALOG_ERROR,
				     utf8_msg,
				     GTK_STOCK_OK, GTK_RESPONSE_CANCEL,
				     NULL);
	g_free (msg);
	g_free (utf8_msg);

	gtk_dialog_run (GTK_DIALOG (d));
	gtk_widget_destroy (d);
}


/* -- delete -- */


static void
real_file_delete (GThumbWindow *window, 
		  GList        *list)
{
	GList    *scan;
	GList    *error_list = NULL;
	gboolean  error = FALSE;

	scan = list;
	while (scan) {
		if (unlink (scan->data) == 0) {
			cache_delete (scan->data);
			comment_delete (scan->data);
			scan = scan->next;
		} else {
			list = g_list_remove_link (list, scan);
			error_list = g_list_prepend (error_list, scan->data);
			g_list_free (scan);
			scan = list;
			error = TRUE;
		}
	}

	all_windows_notify_files_deleted (list);

	if (error) {
		char *msg;

		if (g_list_length (error_list) == 1)
			msg = _("Could not delete the image:");
		else
			msg = _("Could not delete the following images:");

		dlg_show_error (window,
				msg,
				error_list,
				errno_to_string ());
	}
	path_list_free (list);
	path_list_free (error_list);
}


gboolean 
dlg_file_delete (GThumbWindow *window, 
		 GList        *list,
		 const char   *message)
{
	GtkWidget *dialog;
	int        result;

	if (! eel_gconf_get_boolean (PREF_CONFIRM_DELETION)) {
		real_file_delete (window, list);
		return TRUE;
	}

	dialog = _gtk_yesno_dialog_new (GTK_WINDOW (window->app),
					GTK_DIALOG_MODAL,
					message,
					GTK_STOCK_CANCEL,
					GTK_STOCK_DELETE);

	result = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (GTK_WIDGET (dialog));
	if (result == GTK_RESPONSE_YES) {
		real_file_delete (window, list);
		return TRUE;
	}

	return FALSE;
}


/* -- move -- */


static void
destroy_cb (GtkWidget *w,
	    GtkWidget *file_sel)
{
	GList *file_list;
	
	file_list = g_object_get_data (G_OBJECT (file_sel), "list");
	path_list_free (file_list);
}


static void 
file_move_response_cb (GtkWidget *w,
		       int        response_id,
		       GtkWidget *file_sel)
{
	char         *path;
	GList        *file_list, *scan;
	GList        *error_list = NULL;
	GThumbWindow *window;
	int           len, overwrite_result;
	gboolean      file_exists, show_ow_all_none;
	gboolean      error = FALSE;
	char         *new_name;

	if (response_id != GTK_RESPONSE_OK) {
		gtk_widget_destroy (file_sel);
		return;
	}

	window = g_object_get_data (G_OBJECT (file_sel), "gthumb_window");
	file_list = g_object_get_data (G_OBJECT (file_sel), "list");
	path = gth_folder_selection_get_folder (GTH_FOLDER_SELECTION (file_sel));

	if (path == NULL) 
		return;

	/* ignore ending slash. */

	len = strlen (path);
	if (path[len - 1] == '/')
		path[len - 1] = 0;

	if (! path_is_dir (path)) {
		g_free (path);
		return;
	}

	gtk_widget_hide (file_sel);

	show_ow_all_none = g_list_length (file_list) > 1;
	overwrite_result = OVERWRITE_NO;
	for (scan = file_list; scan;) {
		new_name = g_strconcat (path,
					"/",
					file_name_from_path (scan->data),
					NULL);

		/* handle overwrite. */

		file_exists = path_is_file (new_name);

		if ((overwrite_result != OVERWRITE_ALL)
		    && (overwrite_result != OVERWRITE_NONE)
		    && file_exists)
			overwrite_result = dlg_overwrite (window, 
							  overwrite_result,
							  scan->data,
							  new_name,
							  show_ow_all_none);

		if (file_exists 
		    && ((overwrite_result == OVERWRITE_NONE)
			|| (overwrite_result == OVERWRITE_NO))) {
			/* file will not be deleted, delete the file from 
			 * the list. */
			GList *next = scan->next;
			
			file_list = g_list_remove_link (file_list, scan);
			g_free (scan->data);
			g_list_free (scan);
			scan = next;

			g_free (new_name);
			continue;
		} 
		
		if (file_move ((char*) scan->data, new_name)) {
			cache_move ((char*) scan->data, new_name);
			comment_move ((char*) scan->data, new_name);
			scan = scan->next;
		} else {
			file_list = g_list_remove_link (file_list, scan);
			error_list = g_list_prepend (error_list, scan->data);
			g_list_free (scan);
			scan = file_list;
			error = TRUE;
		}
		g_free (new_name);
	}

	all_windows_notify_files_deleted (file_list);
	all_windows_notify_update_directory (path);
	g_free (path);

	if (error) {
		char *msg;

		if (g_list_length (error_list) == 1)
			msg = _("Could not move the image:");
		else
			msg = _("Could not move the following images:");
		
		dlg_show_error (window,
				msg,
				error_list,
				errno_to_string ());
	}
	path_list_free (error_list);

	/* file_list can change so re-set the data. */

	g_object_set_data (G_OBJECT (file_sel), "list", file_list);
	gtk_widget_destroy (file_sel);
}


void 
dlg_file_move (GThumbWindow *window, 
	       GList        *list)
{
	GtkWidget   *file_sel;
	const char  *path;

	file_sel = gth_folder_selection_new (_("Choose the destination folder"));

	if (window->dir_list->path != NULL)
		path = window->dir_list->path;
	else
		path = g_get_home_dir ();

	gth_folder_selection_set_folder (GTH_FOLDER_SELECTION (file_sel), path);

	g_object_set_data (G_OBJECT (file_sel), "list", list);
	g_object_set_data (G_OBJECT (file_sel), "gthumb_window", window);

	g_signal_connect (G_OBJECT (file_sel),
			  "response", 
			  G_CALLBACK (file_move_response_cb), 
			  file_sel);

	g_signal_connect (G_OBJECT (file_sel),
			  "destroy", 
			  G_CALLBACK (destroy_cb),
			  file_sel);
    
	gtk_window_set_transient_for (GTK_WINDOW (file_sel), GTK_WINDOW (window->app));
	gtk_window_set_modal (GTK_WINDOW (file_sel), TRUE);
	gtk_widget_show (file_sel);
}


/* -- copy -- */


static void 
file_copy_response_cb (GtkWidget *w,
		       int        response_id,
		       GtkWidget *file_sel)
{
	char         *path;
	GList        *file_list, *scan;
	GList        *error_list = NULL;
	int           len, overwrite_result;
	char         *new_name;
	gboolean      file_exists, show_ow_all_none;
	gboolean      error = FALSE;
	GThumbWindow *window;

	if (response_id != GTK_RESPONSE_OK) {
		gtk_widget_destroy (file_sel);
		return;
	}

	gtk_widget_hide (file_sel);

	window = g_object_get_data (G_OBJECT (file_sel), "gthumb_window");
	file_list = g_object_get_data (G_OBJECT (file_sel), "list");
	path = gth_folder_selection_get_folder (GTH_FOLDER_SELECTION (file_sel));

	/* ignore ending slash. */
	len = strlen (path);
	if (path[len - 1] == '/')
		path[len - 1] = 0;

	show_ow_all_none = g_list_length (file_list) > 1;
	overwrite_result = OVERWRITE_NO;
	for (scan = file_list; scan; ) {
		new_name = g_strconcat (path,
					"/",
					file_name_from_path (scan->data),
					NULL);

		/* handle overwrite. */
		file_exists = path_is_file (new_name);

		if ((overwrite_result != OVERWRITE_ALL)
		    && (overwrite_result != OVERWRITE_NONE)
		    && file_exists)
			overwrite_result = dlg_overwrite (window, 
							  overwrite_result,
							  scan->data,
							  new_name,
							  show_ow_all_none);

		if (file_exists
		    && ((overwrite_result == OVERWRITE_NONE)
			|| (overwrite_result == OVERWRITE_NO))) {
			scan = scan->next;
			g_free (new_name);
			continue;
		}

		if (file_copy ((gchar*) scan->data, new_name)) {
			cache_copy ((gchar*) scan->data, new_name);
			comment_copy ((gchar*) scan->data, new_name);
			scan = scan->next;
		} else {
			file_list = g_list_remove_link (file_list, scan);
			error_list = g_list_prepend (error_list, scan->data);
			g_list_free (scan);
			scan = file_list;
			error = TRUE;
		}

		g_free (new_name);
	}

	if (file_list != NULL)
		all_windows_notify_update_directory (path);
	g_free (path);

	if (error) {
		char *msg;

		if (g_list_length (error_list) == 1)
			msg = _("Could not copy the image:");
		else
			msg = _("Could not copy the following images:");

		dlg_show_error (window,
				msg,
				error_list,
				errno_to_string ());
	}
	path_list_free (error_list);

	/* file_list can change so re-set the data. */

	g_object_set_data (G_OBJECT (file_sel), "list", file_list);
	gtk_widget_destroy (file_sel);
}


void 
dlg_file_copy (GThumbWindow *window,
	       GList        *list)
{
	GtkWidget  *file_sel;
	const char *path;

	file_sel = gth_folder_selection_new (_("Choose the destination folder"));

	if (window->dir_list->path != NULL)
		path = window->dir_list->path;
	else
		path = g_get_home_dir ();

	gth_folder_selection_set_folder (GTH_FOLDER_SELECTION (file_sel), path);

	g_object_set_data (G_OBJECT (file_sel), "list", list);
	g_object_set_data (G_OBJECT (file_sel), "gthumb_window", window);

	g_signal_connect (G_OBJECT (file_sel),
			  "response", 
			  G_CALLBACK (file_copy_response_cb), 
			  file_sel);

	g_signal_connect (G_OBJECT (file_sel),
			  "destroy", 
			  G_CALLBACK (destroy_cb),
			  file_sel);
    
	gtk_window_set_transient_for (GTK_WINDOW (file_sel), GTK_WINDOW (window->app));
	gtk_window_set_modal (GTK_WINDOW (file_sel),TRUE);
	gtk_widget_show (file_sel);
}


/* -- overwrite dialog -- */


#define PREVIEW_SIZE 180


static void
set_filename_labels (GladeXML    *gui,
		     GtkTooltips *tooltips,
		     const char  *filename_widget,
		     const char  *filename_eventbox,
		     const char  *size_widget,
		     const char  *time_widget,
		     const char  *filename)
{
	GtkWidget  *label;
	GtkWidget  *eventbox;
	time_t      timer;
	struct tm  *tm;
	char        time_txt[50];
	char       *file_size_txt;
	char       *utf8_name;
	char       *name;

	label = glade_xml_get_widget (gui, filename_widget);
	eventbox = glade_xml_get_widget (gui, filename_eventbox);

	name = _g_strdup_with_max_size (filename, FILE_NAME_MAX_LENGTH);
	_gtk_label_set_locale_text (GTK_LABEL (label), name);
	g_free (name);

	utf8_name = g_locale_to_utf8 (filename, -1, NULL, NULL, NULL);
	gtk_tooltips_set_tip (tooltips, eventbox, utf8_name, NULL);
	g_free (utf8_name);

	label = glade_xml_get_widget (gui, size_widget);
	file_size_txt = gnome_vfs_format_file_size_for_display (get_file_size (filename));
	_gtk_label_set_locale_text (GTK_LABEL (label), file_size_txt);
	g_free (file_size_txt);

	label = glade_xml_get_widget (gui, time_widget);
	timer = get_file_mtime (filename);
	tm = localtime (&timer);
	strftime (time_txt, 50, _("%d %b %Y, %H:%M"), tm);
	_gtk_label_set_locale_text (GTK_LABEL (label), time_txt);
}


static int
dlg_overwrite (GThumbWindow *window,
	       int           default_overwrite_mode,
	       char         *old_filename, 
	       char         *new_filename,
	       gboolean      show_overwrite_all_none)
{
	GladeXML    *gui;
	GtkWidget   *dialog;
	GtkWidget   *old_image_frame;
	GtkWidget   *old_img_zoom_in_button;
	GtkWidget   *old_img_zoom_out_button;
	GtkWidget   *new_image_frame;
	GtkWidget   *new_img_zoom_in_button;
	GtkWidget   *new_img_zoom_out_button;
	GtkWidget   *overwrite_yes_radiobutton;
	GtkWidget   *overwrite_no_radiobutton;
	GtkWidget   *overwrite_all_radiobutton;
	GtkWidget   *overwrite_none_radiobutton;
	int          result;
	GtkWidget   *old_image_viewer, *new_image_viewer;
	GtkWidget   *overwrite_radiobutton;
	GtkTooltips *tooltips;

	tooltips = gtk_tooltips_new ();

	gui = glade_xml_new (GTHUMB_GLADEDIR "/" GLADE_FILE, NULL, NULL);

	if (! gui) {
		g_warning ("Could not find " GLADE_FILE "\n");
		return OVERWRITE_NO;
        }

	/* Get the widgets. */

	dialog = glade_xml_get_widget (gui, "overwrite_dialog");

	old_image_frame = glade_xml_get_widget (gui, "old_image_frame");
	old_img_zoom_in_button = glade_xml_get_widget (gui, "old_img_zoom_in_button");
	old_img_zoom_out_button = glade_xml_get_widget (gui, "old_img_zoom_out_button");
	new_image_frame = glade_xml_get_widget (gui, "new_image_frame");
	new_img_zoom_in_button = glade_xml_get_widget (gui, "new_img_zoom_in_button");
	new_img_zoom_out_button = glade_xml_get_widget (gui, "new_img_zoom_out_button");
	overwrite_yes_radiobutton = glade_xml_get_widget (gui, "overwrite_yes_radiobutton");
	overwrite_no_radiobutton = glade_xml_get_widget (gui, "overwrite_no_radiobutton");
	overwrite_all_radiobutton = glade_xml_get_widget (gui, "overwrite_all_radiobutton");
	overwrite_none_radiobutton = glade_xml_get_widget (gui, "overwrite_none_radiobutton");

	/* Set widgets data. */

	/* * set filename labels. */

	set_filename_labels (gui,
			     tooltips,
			     "old_image_filename_label",
			     "old_image_filename_eventbox",
			     "old_image_size_label",
			     "old_image_time_label",
			     old_filename);
	set_filename_labels (gui,
			     tooltips,
			     "new_image_filename_label",
			     "new_image_filename_eventbox",
			     "new_image_size_label",
			     "new_image_time_label",
			     new_filename);

	/* * set the default overwrite mode. */

	overwrite_radiobutton = overwrite_no_radiobutton;
	switch (default_overwrite_mode) {
	case OVERWRITE_YES:
		overwrite_radiobutton = overwrite_yes_radiobutton;
		break;

	case OVERWRITE_NO:
		overwrite_radiobutton = overwrite_no_radiobutton;
		break;

	case OVERWRITE_ALL:
		overwrite_radiobutton = overwrite_all_radiobutton;
		break;

	case OVERWRITE_NONE:
		overwrite_radiobutton = overwrite_none_radiobutton;
		break;
	}
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (overwrite_radiobutton), TRUE);

	if (!show_overwrite_all_none) {
		gtk_widget_hide (overwrite_all_radiobutton);
		gtk_widget_hide (overwrite_none_radiobutton);
	}

	/* * load images. */

	old_image_viewer = image_viewer_new ();
	gtk_container_add (GTK_CONTAINER (old_image_frame), old_image_viewer);
	image_viewer_size (IMAGE_VIEWER (old_image_viewer), 
			   PREVIEW_SIZE, PREVIEW_SIZE);
	image_viewer_set_zoom_quality (IMAGE_VIEWER (old_image_viewer),
				       pref_get_zoom_change ());
	image_viewer_set_check_type (IMAGE_VIEWER (old_image_viewer),
				     image_viewer_get_check_type (IMAGE_VIEWER (window->viewer)));
	image_viewer_set_check_size (IMAGE_VIEWER (old_image_viewer),
				     image_viewer_get_check_size (IMAGE_VIEWER (window->viewer)));
	image_viewer_set_transp_type (IMAGE_VIEWER (old_image_viewer),
				      image_viewer_get_transp_type (IMAGE_VIEWER (window->viewer)));
	gtk_widget_show (old_image_viewer);
	image_viewer_load_image (IMAGE_VIEWER (old_image_viewer), 
				 old_filename);

	new_image_viewer = image_viewer_new ();
	gtk_container_add (GTK_CONTAINER (new_image_frame), new_image_viewer);
	image_viewer_size (IMAGE_VIEWER (new_image_viewer), 
			   PREVIEW_SIZE, PREVIEW_SIZE);
	image_viewer_set_zoom_quality (IMAGE_VIEWER (new_image_viewer),
				       pref_get_zoom_quality ());
	image_viewer_set_check_type (IMAGE_VIEWER (new_image_viewer),
				     image_viewer_get_check_type (IMAGE_VIEWER (window->viewer)));
	image_viewer_set_check_size (IMAGE_VIEWER (new_image_viewer),
				     image_viewer_get_check_size (IMAGE_VIEWER (window->viewer)));
	image_viewer_set_transp_type (IMAGE_VIEWER (new_image_viewer),
				      image_viewer_get_transp_type (IMAGE_VIEWER (window->viewer)));
	gtk_widget_show (new_image_viewer);
	image_viewer_load_image (IMAGE_VIEWER (new_image_viewer), 
				 new_filename);

	/* signals. */

	g_signal_connect_swapped (G_OBJECT (old_img_zoom_in_button),
				  "clicked",
				  G_CALLBACK (image_viewer_zoom_in),
				  G_OBJECT (old_image_viewer));
	g_signal_connect_swapped (G_OBJECT (old_img_zoom_out_button),
				  "clicked",
				  G_CALLBACK (image_viewer_zoom_out),
				  G_OBJECT (old_image_viewer));
	
	g_signal_connect_swapped (G_OBJECT (new_img_zoom_in_button),
				  "clicked",
				  G_CALLBACK (image_viewer_zoom_in),
				  G_OBJECT (new_image_viewer));
	g_signal_connect_swapped (G_OBJECT (new_img_zoom_out_button),
				  "clicked",
				  G_CALLBACK (image_viewer_zoom_out),
				  G_OBJECT (new_image_viewer));

	g_object_set_data (G_OBJECT (dialog), "tooltips", tooltips);

	/* run dialog. */

	gtk_window_set_transient_for (GTK_WINDOW (dialog), 
				      GTK_WINDOW (window->app));
	gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
	result = gtk_dialog_run (GTK_DIALOG (dialog));

	if (result == -1)
		result = OVERWRITE_NO;
	else if (GTK_TOGGLE_BUTTON (overwrite_yes_radiobutton)->active)
		result = OVERWRITE_YES;
	else if (GTK_TOGGLE_BUTTON (overwrite_no_radiobutton)->active)
		result = OVERWRITE_NO;
	else if (GTK_TOGGLE_BUTTON (overwrite_all_radiobutton)->active)
		result = OVERWRITE_ALL;
	else if (GTK_TOGGLE_BUTTON (overwrite_none_radiobutton)->active)
		result = OVERWRITE_NONE;
	else
		result = OVERWRITE_NO;

	gtk_widget_destroy (dialog);
	gtk_object_destroy (GTK_OBJECT (tooltips));
	g_object_unref (gui);

	return result;
}


void
dlg_file_rename_series (GThumbWindow *window,
			GList        *old_names,
			GList        *new_names)
{
	GList    *o_scan, *n_scan;
	GList    *error_list = NULL;
	int       overwrite_result;
	gboolean  file_exists, show_ow_all_none;
	gboolean  error = FALSE;

	all_windows_remove_monitor ();

	show_ow_all_none = g_list_length (old_names) > 1;
	overwrite_result = OVERWRITE_NO;
	for (n_scan = new_names, o_scan = old_names; o_scan && n_scan;) {
		char *old_full_path = o_scan->data;
		char *new_full_path = n_scan->data;

		if (strcmp (old_full_path, new_full_path) == 0) {
			/* file will not be renamed, delete the file from 
			 * the list. */
			GList *next = o_scan->next;
			
			old_names = g_list_remove_link (old_names, o_scan);
			g_free (o_scan->data);
			g_list_free (o_scan);
			o_scan = next;

			n_scan = n_scan->next;

			continue;			
		}

		/* handle overwrite. */

		file_exists = path_is_file (new_full_path);

		if ((overwrite_result != OVERWRITE_ALL)
		    && (overwrite_result != OVERWRITE_NONE)
		    && file_exists)
			overwrite_result = dlg_overwrite (window, 
							  overwrite_result,
							  old_full_path,
							  new_full_path,
							  show_ow_all_none);

		if (file_exists 
		    && ((overwrite_result == OVERWRITE_NONE)
			|| (overwrite_result == OVERWRITE_NO))) {
			/* file will not be renamed, delete the file from 
			 * the list. */
			GList *next = o_scan->next;
			
			old_names = g_list_remove_link (old_names, o_scan);
			g_free (o_scan->data);
			g_list_free (o_scan);
			o_scan = next;

			n_scan = n_scan->next;

			continue;
		} 

		if (! rename (old_full_path, new_full_path)) {
			cache_move (old_full_path, new_full_path);
			comment_move (old_full_path, new_full_path);
			o_scan = o_scan->next;
		} else {
			old_names = g_list_remove_link (old_names, o_scan);
			error_list = g_list_prepend (error_list, o_scan->data);
			g_list_free (o_scan);
			o_scan = old_names;
			error = TRUE;
		}
		n_scan = n_scan->next;
	}

	all_windows_notify_files_rename (old_names, new_names);
	all_windows_add_monitor ();

	if (error) {
		char *msg;

		if (g_list_length (error_list) == 1)
			msg = _("Could not rename the image:");
		else
			msg = _("Could not rename the following images:");
		
		dlg_show_error (window,
				msg,
				error_list,
				errno_to_string ());
	}

	path_list_free (error_list);
	path_list_free (old_names);
	path_list_free (new_names);
}



/* -- dlg_folder_move -- */


typedef struct {
	char                *source;
	char                *destination;
	gboolean             remove_source;
	gboolean             copy_cache;
	GThumbWindow        *window;

	GladeXML            *gui;
	GtkWidget           *progress_dialog;
	GtkWidget           *progress_progressbar;
	GtkWidget           *progress_info;

	GnomeVFSAsyncHandle *handle;
} FolderCopyData;


static void
folder_copy_data_free (FolderCopyData *fcdata)
{
	if (fcdata == NULL)
		return;
	gtk_widget_destroy (fcdata->progress_dialog);
	g_object_unref (fcdata->gui);
	g_free (fcdata->source);
	g_free (fcdata->destination);
	g_free (fcdata);
}


static int
progress_update_cb (GnomeVFSAsyncHandle      *handle,
		    GnomeVFSXferProgressInfo *info,
		    gpointer                  data)
{
	FolderCopyData *fcdata = data;
	int             result = TRUE;
	char           *message = NULL;
	float           fraction;

	if (info->status != GNOME_VFS_XFER_PROGRESS_STATUS_OK) 
		return TRUE;

	if (info->phase == GNOME_VFS_XFER_PHASE_INITIAL) {
		gtk_widget_show_all (fcdata->progress_dialog);		

	} else if (info->phase == GNOME_VFS_XFER_PHASE_COLLECTING) {
		message = g_strdup (_("Collecting images info"));

	} else if (info->phase == GNOME_VFS_XFER_PHASE_COPYING) {
		message = g_strdup_printf (_("Copying file %ld of %ld"), 
					   info->file_index,
					   info->files_total);

	} else if (info->phase == GNOME_VFS_XFER_PHASE_COMPLETED) {
		if (fcdata->remove_source)
			all_windows_notify_directory_rename (fcdata->source, 
							     fcdata->destination);
		else
			all_windows_notify_directory_new (fcdata->destination);

		folder_copy_data_free (fcdata);

		return result;
	}

	if (message != NULL) {
		gtk_label_set_text (GTK_LABEL (fcdata->progress_info), message);
		g_free (message);
	}

	if (info->bytes_total != 0) {
		fraction = (float)info->total_bytes_copied / info->bytes_total;
		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (fcdata->progress_progressbar), fraction);
	}

	return result;
}


static int 
progress_sync_cb (GnomeVFSXferProgressInfo *info,
		  gpointer                  data)
{
	FolderCopyData *fcdata = data;	
	int             ret_val;

	if (info->status == GNOME_VFS_XFER_PROGRESS_STATUS_OK) {
		ret_val = TRUE;
		
	} else if (info->status == GNOME_VFS_XFER_PROGRESS_STATUS_VFSERROR) {
		const char *message;
		char       *utf8_name;

		message = fcdata->remove_source ? _("Could not move folder \"%s\" : %s") : _("Could not copy folder \"%s\" : %s");
		utf8_name = g_locale_to_utf8 (fcdata->source, -1, 0, 0, 0);
		_gtk_error_dialog_run (GTK_WINDOW (fcdata->window->app),
				       message, 
				       utf8_name, 
				       gnome_vfs_result_to_string (info->vfs_status));
		g_free (utf8_name);

		ret_val = GNOME_VFS_XFER_ERROR_ACTION_ABORT;

	} else if (info->status == GNOME_VFS_XFER_PROGRESS_STATUS_OVERWRITE) {
		ret_val = GNOME_VFS_XFER_OVERWRITE_ACTION_REPLACE_ALL;
	}

	return ret_val;
}


GnomeVFSURI *
new_uri_from_path (const char *path)
{
	char        *escaped;
	GnomeVFSURI *uri;

	escaped = gnome_vfs_escape_path_string (path);
	uri = gnome_vfs_uri_new (escaped);
	g_free (escaped);

	return uri;
}


void
dlg_folder_copy (GThumbWindow *window,
		 const char   *src_path,
		 const char   *dest_path,
		 gboolean      remove_source,
		 gboolean      copy_cache)
{
	FolderCopyData            *fcdata;
	GList                     *src_list = NULL;
	GList                     *dest_list = NULL;
	GnomeVFSXferOptions        xfer_options;
	GnomeVFSXferErrorMode      xfer_error_mode;
	GnomeVFSXferOverwriteMode  overwrite_mode;
	GnomeVFSResult             result;

	if (! path_is_dir (src_path))
		return;

	fcdata = g_new0 (FolderCopyData, 1);

	fcdata->window = window;
	fcdata->source = g_strdup (src_path);
	fcdata->destination = g_strdup (dest_path);
	fcdata->remove_source = remove_source;
	fcdata->copy_cache = copy_cache;

	/**/

	fcdata->gui = glade_xml_new (GTHUMB_GLADEDIR "/" GLADE_EXPORTER_FILE, NULL, NULL);
	if (! fcdata->gui) {
		folder_copy_data_free (fcdata);
		g_warning ("Could not find " GLADE_EXPORTER_FILE "\n");
		return;
	}

	fcdata->progress_dialog = glade_xml_get_widget (fcdata->gui, "progress_dialog");
	fcdata->progress_progressbar = glade_xml_get_widget (fcdata->gui, "progress_progressbar");
	fcdata->progress_info = glade_xml_get_widget (fcdata->gui, "progress_info");
	
	gtk_window_set_transient_for (GTK_WINDOW (fcdata->progress_dialog),
				      GTK_WINDOW (fcdata->window->app));
	gtk_window_set_modal (GTK_WINDOW (fcdata->progress_dialog), FALSE);

	/**/

	src_list = g_list_append (src_list, new_uri_from_path (src_path));
	dest_list = g_list_append (dest_list, new_uri_from_path (dest_path));

	if (fcdata->copy_cache) {
		char *src_cache;
		char *dest_cache;

		src_cache = comments_get_comment_dir (src_path);
		dest_cache = comments_get_comment_dir (dest_path);

		if (path_is_dir (src_cache)) {
			char *parent_dir = remove_level_from_path (dest_cache);
			ensure_dir_exists (parent_dir, 0755);
			g_free (parent_dir);

			src_list = g_list_append (src_list, 
						  new_uri_from_path (src_cache));
			dest_list = g_list_append (dest_list, 
						   new_uri_from_path (dest_cache));
		}

		g_free (src_cache);
		g_free (dest_cache);

		/**/

		src_cache = cache_get_nautilus_cache_dir (src_path);
		dest_cache = cache_get_nautilus_cache_dir (dest_path);

		if (path_is_dir (src_cache)) {
			char *parent_dir = remove_level_from_path (dest_cache);
			ensure_dir_exists (parent_dir, 0755);
			g_free (parent_dir);

			src_list = g_list_append (src_list, 
						  new_uri_from_path (src_cache));
			dest_list = g_list_append (dest_list, 
						   new_uri_from_path (dest_cache));
		}
	}

	xfer_options = (GNOME_VFS_XFER_RECURSIVE 
			| (remove_source ? GNOME_VFS_XFER_REMOVESOURCE : 0));
	xfer_error_mode = GNOME_VFS_XFER_ERROR_MODE_ABORT;
	overwrite_mode = GNOME_VFS_XFER_OVERWRITE_MODE_QUERY;

	result = gnome_vfs_async_xfer (&fcdata->handle,
				       src_list,
				       dest_list,
				       xfer_options,
				       xfer_error_mode,
				       overwrite_mode,
				       GNOME_VFS_PRIORITY_DEFAULT,
				       progress_update_cb,
				       fcdata,
				       progress_sync_cb,
				       fcdata);

	g_list_foreach (src_list, (GFunc) gnome_vfs_uri_unref, NULL);
	g_list_free (src_list);
	g_list_foreach (dest_list, (GFunc) gnome_vfs_uri_unref, NULL);
	g_list_free (dest_list);

	/**/

	if (result != GNOME_VFS_OK) {
		const char *message;
		char       *utf8_name;
		
		message = fcdata->remove_source ? _("Could not move folder \"%s\" : %s") : _("Could not copy folder \"%s\" : %s");
		utf8_name = g_locale_to_utf8 (fcdata->source, -1, 0, 0, 0);
		_gtk_error_dialog_run (GTK_WINDOW (fcdata->window->app),
				       message, 
				       utf8_name, 
				       gnome_vfs_result_to_string (result));
		g_free (utf8_name);
		folder_copy_data_free (fcdata);

		return;
	}
}
