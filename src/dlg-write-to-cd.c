/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2004 Free Software Foundation, Inc.
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
#include <glade/glade.h>
#include <libgnome/gnome-url.h>
#include <libgnome/gnome-help.h>
#include <libgnomevfs/gnome-vfs-utils.h>

#include "gthumb-window.h"
#include "gtk-utils.h"
#include "gconf-utils.h"
#include "glib-utils.h"
#include "gth-exif-utils.h"
#include "file-utils.h"
#include "dlg-file-utils.h"
#include "preferences.h"

#define GLADE_FILE "gthumb_tools.glade"

typedef struct {
	GThumbWindow  *window;
	GladeXML      *gui;

	GList         *file_list;
	char          *current_folder;

	GtkWidget     *dialog;
	GtkWidget     *wtc_folder_radiobutton;
	GtkWidget     *wtc_catalog_radiobutton;
	GtkWidget     *wtc_selection_radiobutton;
} DialogData;


/* called when the main dialog is closed. */
static void
destroy_cb (GtkWidget  *widget, 
	    DialogData *data)
{
	path_list_free (data->file_list);
	g_free (data->current_folder);
	g_object_unref (data->gui);
	g_free (data);
}


static void
write_to_cd__continue (GnomeVFSResult  result,
		       gpointer        user_data)
{
	DialogData   *data = user_data;
	GThumbWindow *window = data->window;
	
	if (result != GNOME_VFS_OK) {
		_gtk_error_dialog_run (GTK_WINDOW (window->app),
				       "%s %s",
				       _("Could not move the items:"), 
				       gnome_vfs_result_to_string (result));

	} else {
		exec_command ("nautilus --no-default-window --no-desktop --browser burn://", NULL);
		/*
		  GError *err = NULL;
		  if (! gnome_url_show ("burn:///", &err))
		  _gtk_error_dialog_from_gerror_run (GTK_WINDOW (window->app), &err);
		*/
	}

	gtk_widget_destroy (data->dialog);
}


/* called when the "ok" button is pressed. */
static void
ok_clicked_cb (GtkWidget  *widget, 
	       DialogData *data)
{
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->wtc_selection_radiobutton))) {
		dlg_copy_items (data->window, 
				data->file_list,
				"burn:///",
				FALSE,
				TRUE,
				TRUE,
				write_to_cd__continue,
				data);

	} else if (data->window->sidebar_content == GTH_SIDEBAR_CATALOG_LIST) {
		GList *file_list = gth_file_list_get_all (data->window->file_list);
		dlg_copy_items (data->window, 
				file_list,
				"burn:///",
				FALSE,
				TRUE,
				TRUE,
				write_to_cd__continue,
				data);
		path_list_free (file_list);

	} else {
		char *dest_folder = g_build_filename ("burn:///", file_name_from_path (data->current_folder), NULL);
		dlg_folder_copy (data->window,
				 data->current_folder,
				 dest_folder,
				 FALSE,
				 TRUE,
				 TRUE,
				 write_to_cd__continue,
				 data);
		g_free (dest_folder);
	}
}



void
dlg_write_to_cd (GThumbWindow *window)
{
	DialogData  *data;
	GtkWidget   *btn_ok;
	GtkWidget   *btn_cancel;

	data = g_new0 (DialogData, 1);

	data->gui = glade_xml_new (GTHUMB_GLADEDIR "/" GLADE_FILE , NULL, NULL);
	if (!data->gui) {
		g_free (data);
		g_warning ("Could not find " GLADE_FILE "\n");
		return;
	}
	data->current_folder = g_strdup (window->dir_list->path);
	data->file_list = gth_file_list_get_selection (window->file_list);
	data->window = window;

	/* Get the widgets. */

	data->dialog = glade_xml_get_widget (data->gui, "write_to_cd_dialog");
	data->wtc_folder_radiobutton = glade_xml_get_widget (data->gui, "wtc_folder_radiobutton");
	data->wtc_catalog_radiobutton = glade_xml_get_widget (data->gui, "wtc_catalog_radiobutton");
	data->wtc_selection_radiobutton = glade_xml_get_widget (data->gui, "wtc_selection_radiobutton");

        btn_ok = glade_xml_get_widget (data->gui, "wtc_okbutton");
        btn_cancel = glade_xml_get_widget (data->gui, "wtc_cancelbutton");

	/* Set widgets data. */

	if (data->window->sidebar_content == GTH_SIDEBAR_DIR_LIST)
		gtk_widget_show (data->wtc_folder_radiobutton);
	else
		gtk_widget_hide (data->wtc_folder_radiobutton);

	if (data->window->sidebar_content == GTH_SIDEBAR_CATALOG_LIST)
		gtk_widget_show (data->wtc_catalog_radiobutton);
	else
		gtk_widget_hide (data->wtc_catalog_radiobutton);

	gtk_widget_set_sensitive (data->wtc_selection_radiobutton, data->file_list != NULL);

	if (data->file_list != NULL)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->wtc_selection_radiobutton), TRUE);
	else if (data->window->sidebar_content == GTH_SIDEBAR_DIR_LIST)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->wtc_folder_radiobutton), TRUE);
	else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->wtc_catalog_radiobutton), TRUE);

	/* Set the signals handlers. */
	
	g_signal_connect (G_OBJECT (data->dialog), 
			  "destroy",
			  G_CALLBACK (destroy_cb),
			  data);
	g_signal_connect (G_OBJECT (btn_ok), 
			  "clicked",
			  G_CALLBACK (ok_clicked_cb),
			  data);
	g_signal_connect_swapped (G_OBJECT (btn_cancel), 
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  G_OBJECT (data->dialog));

	/* run dialog. */

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), 
				      GTK_WINDOW (window->app));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), TRUE);
	gtk_widget_show (data->dialog);
}
