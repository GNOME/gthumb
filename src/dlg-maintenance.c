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
#include <gtk/gtk.h>
#include <libgnome/libgnome.h>
#include <glade/glade.h>
#include "comments.h"
#include "gthumb-window.h"
#include "gtk-utils.h"
#include "thumb-cache.h"


#define GLADE_FILE "gthumb_tools.glade"


typedef struct {
	GThumbWindow       *window;

	GladeXML           *gui;
	GtkWidget          *dialog;
} DialogData;


/* called when the main dialog is closed. */
static void
destroy_cb (GtkWidget  *widget, 
	    DialogData *data)
{
	g_object_unref (data->gui);
	g_free (data);
}


static void
remove_old_comments_cb (GtkWidget  *widget, 
			DialogData *data)
{
	comments_remove_old_comments_async (NULL, TRUE, FALSE);
}


static void
remove_all_comments_cb (GtkWidget  *widget, 
			DialogData *data)
{
	GtkWidget *d;
	int        r;

	d = _gtk_yesno_dialog_new (GTK_WINDOW (data->dialog),
				   GTK_DIALOG_MODAL,
				   _("All comments will be removed, are you sure?"),
				   GTK_STOCK_NO,
				   GTK_STOCK_DELETE);
	gtk_dialog_set_default_response (GTK_DIALOG (d), GTK_RESPONSE_NO);
	r = gtk_dialog_run (GTK_DIALOG (d));
	gtk_widget_destroy (GTK_WIDGET (d));
	
	if (r != GTK_RESPONSE_YES) 
		return;

	comments_remove_old_comments_async (NULL, TRUE, TRUE);
}


static void
remove_old_thumbs_cb (GtkWidget  *widget, 
		      DialogData *data)
{
	cache_remove_old_previews_async (NULL, TRUE, FALSE);
}


static void
remove_all_thumbs_cb (GtkWidget  *widget, 
		      DialogData *data)
{
	GtkWidget *d;
	int        r;

	d = _gtk_yesno_dialog_new (GTK_WINDOW (data->dialog),
				   GTK_DIALOG_MODAL,
				   _("All thumbnails will be deleted, are you sure?"),
				   GTK_STOCK_NO,
				   GTK_STOCK_DELETE);
	gtk_dialog_set_default_response (GTK_DIALOG (d), GTK_RESPONSE_NO);
	r = gtk_dialog_run (GTK_DIALOG (d));
	gtk_widget_destroy (GTK_WIDGET (d));
	
	if (r != GTK_RESPONSE_YES) 
		return;

	cache_remove_old_previews_async (NULL, TRUE, TRUE);
}


/* -- backup comments -- */


static void
backup__ok_cb (GObject  *object,
	       gpointer  data)
{
	GtkWidget *file_sel = data;
	char      *backup_file;
	char      *comment_dir;
	char      *command_line;
	GError    *err = NULL;

	backup_file = g_strdup (gtk_file_selection_get_filename (GTK_FILE_SELECTION (file_sel)));
	gtk_widget_destroy (file_sel);

	if (backup_file == NULL) 
		return;

	/**/

	comment_dir = comments_get_comment_dir (NULL, TRUE, TRUE);
	command_line = g_strconcat ("file-roller",
				    " --add-to=", backup_file, 
				    " --force",
				    " ", comment_dir,
				    NULL);
	g_free (comment_dir);

	if (! g_spawn_command_line_async (command_line, &err) || (err != NULL))
		_gtk_error_dialog_from_gerror_run (NULL, &err);

	g_free (command_line);

	/**/

	g_free (backup_file);
}


static void
backup_comments_cb (GtkWidget  *widget, 
		    DialogData *data)
{
	GtkWidget *file_sel;
	char      *home;

	file_sel = gtk_file_selection_new (_("Specify the backup file"));

	home = g_strconcat (g_get_home_dir (), "/", NULL);
	gtk_file_selection_set_filename (GTK_FILE_SELECTION (file_sel), home);
	g_free (home);

	g_signal_connect (G_OBJECT (GTK_FILE_SELECTION (file_sel)->ok_button),
			  "clicked", 
			  G_CALLBACK (backup__ok_cb), 
			  file_sel);

	g_signal_connect_swapped (G_OBJECT (GTK_FILE_SELECTION (file_sel)->cancel_button),
				  "clicked", 
				  G_CALLBACK (gtk_widget_destroy),
				  G_OBJECT (file_sel));

	gtk_window_set_transient_for (GTK_WINDOW (file_sel), 
				      GTK_WINDOW (data->dialog));
	gtk_window_set_modal (GTK_WINDOW (file_sel), TRUE);
	gtk_widget_show (file_sel);
}


/* -- restore comment -- */


static void
restore__ok_cb (GObject  *object,
		gpointer  data)
{
	GtkWidget *file_sel = data;
	char      *backup_file;
	char      *extract_dir;
	char      *command_line;
	GError    *err = NULL;

	backup_file = g_strdup (gtk_file_selection_get_filename (GTK_FILE_SELECTION (file_sel)));
	gtk_widget_destroy (file_sel);

	if (backup_file == NULL) 
		return;

	/**/

	extract_dir = g_strconcat (g_get_home_dir(), "/", RC_DIR, NULL);
	command_line = g_strconcat ("file-roller",
				    " --extract-to=", extract_dir, 
				    " --force",
				    " ", backup_file,
				    NULL);
	g_free (extract_dir);

	if (! g_spawn_command_line_async (command_line, &err) || (err != NULL))
		_gtk_error_dialog_from_gerror_run (NULL, &err);

	g_free (command_line);

	/**/

	g_free (backup_file);
}


static void
restore_comments_cb (GtkWidget  *widget, 
		     DialogData *data)
{
	GtkWidget *file_sel;
	char      *home;

	file_sel = gtk_file_selection_new (_("Specify the backup file"));

	home = g_strconcat (g_get_home_dir (), "/", NULL);
	gtk_file_selection_set_filename (GTK_FILE_SELECTION (file_sel), home);
	g_free (home);
	
	g_signal_connect (G_OBJECT (GTK_FILE_SELECTION (file_sel)->ok_button),
			  "clicked", 
			  G_CALLBACK (restore__ok_cb), 
			  file_sel);

	g_signal_connect_swapped (G_OBJECT (GTK_FILE_SELECTION (file_sel)->cancel_button),
				  "clicked", 
				  G_CALLBACK (gtk_widget_destroy),
				  G_OBJECT (file_sel));

	gtk_window_set_transient_for (GTK_WINDOW (file_sel), 
				      GTK_WINDOW (data->dialog));
	gtk_window_set_modal (GTK_WINDOW (file_sel), TRUE);
	gtk_widget_show (file_sel);
}


/* called when the "help" button is clicked. */
static void
help_cb (GtkWidget  *widget, 
	 DialogData *data)
{
	GError *err;

	err = NULL;  
	gnome_help_display ("gthumb", "maintenance", &err);
	
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


/* create the main dialog. */
void
dlg_maintenance (GThumbWindow *window)
{
	DialogData *data;
	GtkWidget  *btn_help;
	GtkWidget  *btn_close;
	GtkWidget  *btn_comment_remove_old;
	GtkWidget  *btn_comment_remove_all;
	GtkWidget  *btn_comment_backup;
	GtkWidget  *btn_comment_restore;
	GtkWidget  *btn_thumb_remove_old;
	GtkWidget  *btn_thumb_remove_all;

	data = g_new (DialogData, 1);

	data->window = window;
	data->gui = glade_xml_new (GTHUMB_GLADEDIR "/" GLADE_FILE, NULL, NULL);
        if (!data->gui) {
		g_free (data);
                g_warning ("Could not find " GLADE_FILE "\n");
                return;
        }

	/* Get the widgets. */

	data->dialog = glade_xml_get_widget (data->gui, "maintenance_dialog");

        btn_help = glade_xml_get_widget (data->gui, "m_help_button");
	btn_close = glade_xml_get_widget (data->gui, "m_close_button");

	btn_comment_remove_old = glade_xml_get_widget (data->gui, "m_comment_remove_old_button");
	btn_comment_remove_all = glade_xml_get_widget (data->gui, "m_comment_remove_all_button");
	btn_comment_backup = glade_xml_get_widget (data->gui, "m_comment_backup_button");
	btn_comment_restore = glade_xml_get_widget (data->gui, "m_comment_restore_button");
	btn_thumb_remove_old = glade_xml_get_widget (data->gui, "m_thumb_remove_old_button");
	btn_thumb_remove_all = glade_xml_get_widget (data->gui, "m_thumb_remove_all_button");

	/* Signals. */

	g_signal_connect (G_OBJECT (data->dialog), 
			  "destroy",
			  G_CALLBACK (destroy_cb),
			  data);
	g_signal_connect_swapped (G_OBJECT (btn_close), 
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  G_OBJECT (data->dialog));
	g_signal_connect (G_OBJECT (btn_help), 
			  "clicked",
			  G_CALLBACK (help_cb),
			  data);
	g_signal_connect (G_OBJECT (btn_comment_remove_old), 
			  "clicked",
			  G_CALLBACK (remove_old_comments_cb),
			  data);
	g_signal_connect (G_OBJECT (btn_comment_remove_all), 
			  "clicked",
			  G_CALLBACK (remove_all_comments_cb),
			  data);
	g_signal_connect (G_OBJECT (btn_comment_backup), 
			  "clicked",
			  G_CALLBACK (backup_comments_cb),
			  data);
	g_signal_connect (G_OBJECT (btn_comment_restore), 
			  "clicked",
			  G_CALLBACK (restore_comments_cb),
			  data);

	g_signal_connect (G_OBJECT (btn_thumb_remove_old), 
			  "clicked",
			  G_CALLBACK (remove_old_thumbs_cb),
			  data);
	g_signal_connect (G_OBJECT (btn_thumb_remove_all), 
			  "clicked",
			  G_CALLBACK (remove_all_thumbs_cb),
			  data);

	/* run dialog. */

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), 
				      GTK_WINDOW (window->app));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), TRUE);
	gtk_widget_show_all (data->dialog);
}
