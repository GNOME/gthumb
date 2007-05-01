/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001, 2003, 2004 Free Software Foundation, Inc.
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
#include <time.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <libgnome/gnome-help.h>
#include <libgnomeui/gnome-dateedit.h>
#include <glade/glade.h>

#include "typedefs.h"
#include "preferences.h"
#include "main.h"
#include "glib-utils.h"
#include "gth-utils.h"
#include "gth-window.h"
#include "gtk-utils.h"
#include "gth-file-view.h"
#include "comments.h"
#include "gth-exif-utils.h"
#include "dlg-comment.h"
#include "dlg-image-prop.h"
#include "file-utils.h"


enum {
	NO_DATE = 0,
	FOLLOWING_DATE,
	CURRENT_DATE,
	EXIF_DATE,
	LAST_MODIFIED_DATE,
	IMAGE_CREATION_DATE,
	NO_CHANGE,
	N_DATE_OPTIONS
};


#define DIALOG_NAME "comment"
#define GLADE_FILE "gthumb_comments.glade"


typedef struct {
	GthWindow     *window;
	GladeXML      *gui;
	GList         *file_list;

	GtkWidget     *dialog;
	GtkWidget     *comment_main_box;
	GtkWidget     *note_text_view;
	GtkTextBuffer *note_text_buffer;
	GtkWidget     *place_entry;
	GtkWidget     *date_optionmenu;
	GtkWidget     *date_dateedit;
	GtkWidget     *save_changed_checkbutton;
	GtkWidget     *ok_button;

	CommentData   *original_cdata;
	gboolean       have_exif_data;
	gboolean       several_images;
} DialogData;


static void
free_dialog_data (DialogData *data)
{
	if (data->file_list != NULL) {
		g_list_foreach (data->file_list, (GFunc) g_free, NULL);
		g_list_free (data->file_list);
		data->file_list = NULL;
	}
	
	if (data->original_cdata != NULL) {
		comment_data_free (data->original_cdata);
		data->original_cdata = NULL;
	}
}


/* called when the main dialog is closed. */
static void
destroy_cb (GtkWidget  *widget, 
	    DialogData *data)
{
	gth_window_set_comment_dlg (GTH_WINDOW (data->window), NULL);
	g_object_unref (data->gui);
	free_dialog_data (data);
	g_free (data);
}


static gboolean
unrealize_cb (GtkWidget  *widget, 
	  DialogData *data)
{
	pref_util_save_window_geometry (GTK_WINDOW (widget), DIALOG_NAME);
	return FALSE;
}


static int
text_field_cmp (const char *s1, const char *s2)
{
	if ((s1 != NULL) && (*s1 == 0))
		s1 = NULL;
	if ((s2 != NULL) && (*s2 == 0))
		s2 = NULL;

	return strcmp_null_tollerant (s1, s2);
}


static time_t
get_requested_time (DialogData *data, 
		    const char *filename)
{
	int idx  = gtk_option_menu_get_history (GTK_OPTION_MENU (data->date_optionmenu));
	time_t t = (time_t)0;

	switch (idx) {
	case NO_DATE:
		break;
	case FOLLOWING_DATE:
		t = gnome_date_edit_get_time (GNOME_DATE_EDIT (data->date_dateedit));
		break;
	case CURRENT_DATE:
		t = time (NULL);
		break;
	case EXIF_DATE:
		t = get_exif_time (filename);
		break;
	case LAST_MODIFIED_DATE:
		t = get_file_mtime (filename);
		break;
	case IMAGE_CREATION_DATE:
		t = get_file_ctime (filename);
		break;
	}

	return t;
}


/* called when the "save" button is pressed. */
static void
save_clicked_cb (GtkWidget  *widget, 
		 DialogData *data)
{
	GList       *scan;
	CommentData *cdata;
	const char  *text;
	char        *comment_text;
	struct tm    tm = {0};
	GtkTextIter  start_iter, end_iter;
	gboolean     save_changed;

	save_changed = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->save_changed_checkbutton));
	cdata = comment_data_new ();

	/* Place */

	text = gtk_entry_get_text (GTK_ENTRY (data->place_entry));
	if (text != NULL)
		cdata->place = g_strdup (text);
	
	/* Comment */

	gtk_text_buffer_get_bounds (data->note_text_buffer, 
				    &start_iter, 
				    &end_iter);
	comment_text = gtk_text_buffer_get_text (data->note_text_buffer, 
						 &start_iter, 
						 &end_iter, 
						 FALSE);
	if (comment_text)
		cdata->comment = comment_text;

	/**/

	if (save_changed && (data->original_cdata != NULL)) {
		CommentData *o_cdata = data->original_cdata;

		/* NULL-ify equal fields. */

		if (text_field_cmp (cdata->place, o_cdata->place) == 0) {
			g_free (cdata->place);
			cdata->place = NULL;
		}

		if (cdata->time == o_cdata->time)
			cdata->time = -1;
		
		if (text_field_cmp (cdata->comment, o_cdata->comment) == 0) {
			g_free (cdata->comment);
			cdata->comment = NULL;
		}
	}

	/* Save */

	for (scan = data->file_list; scan; scan = scan->next) {
		const char *filename = scan->data;
		int         option;

		/* Date */

		cdata->time = -1;

		option = gtk_option_menu_get_history (GTK_OPTION_MENU (data->date_optionmenu));

		if (option == NO_DATE) 
			cdata->time = 0;
		else if (option == NO_CHANGE)
			cdata->time = -1;
		else {
			time_t     t;
			struct tm *tm_date;
			
			t = get_requested_time (data, filename);
			tm_date = localtime (&t);
			tm.tm_year = tm_date->tm_year;
			tm.tm_mon = tm_date->tm_mon;
			tm.tm_mday = tm_date->tm_mday;
			tm.tm_hour = tm_date->tm_hour;
			tm.tm_min = tm_date->tm_min;
			tm.tm_sec = tm_date->tm_sec;
			tm.tm_isdst = tm_date->tm_isdst;

			cdata->time = mktime (&tm);
			if (cdata->time <= 0)
				cdata->time = 0;
		}

		/**/

		if (save_changed)
			comments_save_comment_non_null (filename, cdata);
		else
			comments_save_comment (filename, cdata);
		all_windows_notify_update_metadata (filename); 
	}

	gth_window_update_current_image_metadata (data->window);
	comment_data_free (cdata);

	/**/

	dlg_comment_close (data->dialog);
}


/* called when the "help" button in the search dialog is pressed. */
static void
help_cb (GtkWidget  *widget, 
	 DialogData *data)
{
	gthumb_display_help (GTK_WINDOW (data->dialog), "gthumb-comments");
}


static void
date_optionmenu_changed_cb (GtkOptionMenu *option_menu,
			    DialogData    *data)
{
	char *first_image = data->file_list->data;
	int   idx = gtk_option_menu_get_history (option_menu);

	gtk_widget_set_sensitive (data->date_dateedit, idx == FOLLOWING_DATE);

	switch (idx) {
	case NO_DATE:
	case FOLLOWING_DATE:
	case NO_CHANGE:
		break;
	case CURRENT_DATE:
		gnome_date_edit_set_time (GNOME_DATE_EDIT (data->date_dateedit),
					  time (NULL));

		break;
	case IMAGE_CREATION_DATE:
		gnome_date_edit_set_time (GNOME_DATE_EDIT (data->date_dateedit), 
					  get_file_ctime (first_image));
		break;
	case LAST_MODIFIED_DATE:
		gnome_date_edit_set_time (GNOME_DATE_EDIT (data->date_dateedit), 
					  get_file_mtime (first_image));
		break;
	case EXIF_DATE:
		gnome_date_edit_set_time (GNOME_DATE_EDIT (data->date_dateedit),
					  get_exif_time (first_image));
		break;
	}
}


static GtkWidget *
get_exif_date_option_item (DialogData *data)
{
	GtkWidget *menu;
	GList     *list, *scan;
	int        i = 0;
	GtkWidget *retval = NULL;

	menu = gtk_option_menu_get_menu (GTK_OPTION_MENU (data->date_optionmenu));
	list = gtk_container_get_children (GTK_CONTAINER (menu));
	for (scan = list; scan && i < EXIF_DATE; scan = scan->next)
		i++;

	retval = (GtkWidget*) scan->data;
	g_list_free (list);

	return retval;
}



GtkWidget *
dlg_comment_new (GthWindow *window)
{
	GtkWidget  *current_dialog;
	DialogData *data;
	GtkWidget  *btn_close;
	GtkWidget  *btn_help;
	
	current_dialog = gth_window_get_comment_dlg (window);
	if (current_dialog != NULL) {
		gtk_window_present (GTK_WINDOW (current_dialog));
		return current_dialog;
	}

	data = g_new0 (DialogData, 1);

	data->window = window;

	data->gui = glade_xml_new (GTHUMB_GLADEDIR "/" GLADE_FILE , NULL, NULL);
        if (!data->gui) {
                g_warning ("Could not find " GLADE_FILE "\n");
                return NULL;
        }

	/* Get the widgets. */

	data->dialog = glade_xml_get_widget (data->gui, "comments_dialog");
	gth_window_set_comment_dlg (window, data->dialog);
	data->comment_main_box = glade_xml_get_widget (data->gui, "comment_main_box");
	data->note_text_view = glade_xml_get_widget (data->gui, "note_text");
	data->place_entry = glade_xml_get_widget (data->gui, "place_entry");
	data->date_optionmenu = glade_xml_get_widget (data->gui, "date_optionmenu");
	data->date_dateedit = glade_xml_get_widget (data->gui, "date_dateedit");
	data->save_changed_checkbutton = glade_xml_get_widget (data->gui, "save_changed_checkbutton");
	data->ok_button = glade_xml_get_widget (data->gui, "ok_button");
	btn_close = glade_xml_get_widget (data->gui, "cancel_button");
	btn_help = glade_xml_get_widget (data->gui, "help_button");

	data->note_text_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (data->note_text_view));

	g_object_set_data (G_OBJECT (data->dialog), "dialog_data", data);

	/* Set the signals handlers. */
	
	g_signal_connect (G_OBJECT (data->dialog), 
			  "destroy",
			  G_CALLBACK (destroy_cb),
			  data);
	g_signal_connect (G_OBJECT (data->dialog), 
			  "unrealize",
			  G_CALLBACK (unrealize_cb),
			  data);
	g_signal_connect (G_OBJECT (data->ok_button), 
			  "clicked",
			  G_CALLBACK (save_clicked_cb),
			  data);
	g_signal_connect_swapped (G_OBJECT (btn_close), 
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  G_OBJECT (data->dialog));
	g_signal_connect (G_OBJECT (btn_help), 
			  "clicked",
			  G_CALLBACK (help_cb),
			  data);

	g_signal_connect (G_OBJECT (data->date_optionmenu), 
			  "changed",
			  G_CALLBACK (date_optionmenu_changed_cb),
			  data);

	/* run dialog. */

	gtk_widget_grab_focus (data->note_text_view);
	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), 
				      GTK_WINDOW (window));

	gtk_window_set_modal (GTK_WINDOW (data->dialog), FALSE);
	pref_util_restore_window_geometry (GTK_WINDOW (data->dialog), DIALOG_NAME);
	dlg_comment_update (data->dialog);

	return data->dialog;
}


void
dlg_comment_update (GtkWidget *dlg)
{
	DialogData    *data;
	CommentData   *cdata;
	GList         *scan;
	char          *first_image;

	g_return_if_fail (dlg != NULL);

	data = g_object_get_data (G_OBJECT (dlg), "dialog_data");
	g_return_if_fail (data != NULL);

	/**/
	
	free_dialog_data (data);

	data->file_list = gth_window_get_file_list_selection (data->window); 

	if (data->file_list == NULL) {
		gtk_widget_set_sensitive (data->ok_button, FALSE);
		gtk_widget_set_sensitive (data->comment_main_box, FALSE);
		return;
	} else 
		gtk_widget_set_sensitive (data->comment_main_box, TRUE);

	/* Set widgets data. */
	
	gtk_option_menu_set_history (GTK_OPTION_MENU (data->date_optionmenu), NO_DATE);
	gtk_text_buffer_set_text (data->note_text_buffer, "", -1);
	gtk_entry_set_text (GTK_ENTRY (data->place_entry), "");

	data->several_images = g_list_length (data->file_list) > 1;

	if (data->several_images) {
		gtk_widget_set_sensitive (data->save_changed_checkbutton, TRUE);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->save_changed_checkbutton), TRUE);
	} else {
		gtk_widget_set_sensitive (data->save_changed_checkbutton, FALSE);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->save_changed_checkbutton), FALSE);
	}

	/* Does at least one image have exif data? */
	data->have_exif_data = FALSE;
	for (scan = data->file_list; scan; scan = scan->next)
		if (have_exif_time (scan->data)) {
			data->have_exif_data = TRUE;
			break;
		}
	
	first_image = data->file_list->data;
	data->original_cdata = cdata = comments_load_comment (first_image, TRUE);

	if (cdata != NULL) {
		comment_data_free_keywords (cdata);

		/* NULL-ify a field if it is not the same in all comments. */
		for (scan = data->file_list->next; scan; scan = scan->next) {
			CommentData *scan_cdata;

			scan_cdata = comments_load_comment (scan->data, TRUE);

			/* If there is no comment then all fields must be
			 * considered to differ. */

			if ((scan_cdata == NULL) 
			    || strcmp_null_tollerant (cdata->place, 
						      scan_cdata->place) != 0) {
				g_free (cdata->place);
				cdata->place = NULL;
			}
			
			if ((scan_cdata == NULL) 
			    || cdata->time != scan_cdata->time)
				cdata->time = 0;
			
			if ((scan_cdata == NULL) 
			    || strcmp_null_tollerant (cdata->comment, 
						      scan_cdata->comment) != 0) {
				g_free (cdata->comment);
				cdata->comment = NULL;
			}

			if (scan_cdata == NULL)
				break;

			comment_data_free (scan_cdata);
		}
	}

	gtk_widget_set_sensitive (get_exif_date_option_item (data), data->have_exif_data);
	gtk_widget_set_sensitive (data->date_dateedit, FALSE);

	if (cdata != NULL) {
		if (cdata->comment != NULL) {
			GtkTextIter iter;

			gtk_text_buffer_set_text (data->note_text_buffer,
						  cdata->comment, 
						  strlen (cdata->comment));

			gtk_text_buffer_get_iter_at_line (data->note_text_buffer,
							  &iter,
							  0);
			gtk_text_buffer_place_cursor (data->note_text_buffer,
						      &iter);
		}

		/**/

		if (cdata->place != NULL) 
			gtk_entry_set_text (GTK_ENTRY (data->place_entry), cdata->place);

		/**/

		if (cdata->time > 0) {
			gtk_option_menu_set_history (GTK_OPTION_MENU (data->date_optionmenu), FOLLOWING_DATE);
			gnome_date_edit_set_time (GNOME_DATE_EDIT (data->date_dateedit), cdata->time);
			gtk_widget_set_sensitive (data->date_dateedit, TRUE);
		} else {
			if (data->several_images)
				gtk_option_menu_set_history (GTK_OPTION_MENU (data->date_optionmenu), NO_CHANGE);
			else
				gtk_option_menu_set_history (GTK_OPTION_MENU (data->date_optionmenu), NO_DATE);
			gnome_date_edit_set_time (GNOME_DATE_EDIT (data->date_dateedit), get_file_ctime (first_image));
		}
	} else {
		time_t ctime = get_file_ctime (first_image);
		gtk_option_menu_set_history (GTK_OPTION_MENU (data->date_optionmenu), NO_DATE);
		gnome_date_edit_set_time (GNOME_DATE_EDIT (data->date_dateedit), ctime);
	}
}


void
dlg_comment_close  (GtkWidget *dlg)
{
	DialogData *data;

	g_return_if_fail (dlg != NULL);

	data = g_object_get_data (G_OBJECT (dlg), "dialog_data");
	g_return_if_fail (data != NULL);

	gtk_widget_destroy (dlg);
}
