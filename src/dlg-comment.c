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

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <libgnome/libgnome.h>
#include <libgnomeui/gnome-dialog.h>
#include <libgnomeui/gnome-dialog-util.h>
#include <libgnomeui/gnome-dateedit.h>
#include <glade/glade.h>

#ifdef HAVE_LIBEXIF
#include <exif-data.h>
#include <exif-content.h>
#include <exif-entry.h>
#endif /* HAVE_LIBEXIF */

#include "typedefs.h"
#include "main.h"
#include "gthumb-window.h"
#include "gtk-utils.h"
#include "image-list.h"
#include "image-list-utils.h"
#include "comments.h"


#define COMMENT_GLADE_FILE "gthumb_comments.glade"

typedef struct {
	GThumbWindow  *window;
	GladeXML      *gui;
	GList         *file_list;

	GtkWidget     *dialog;
	GtkWidget     *place_entry;
	GtkWidget     *date_checkbutton;
	GtkWidget     *date_dateedit;

	GtkWidget     *c_date_vbox;
	GtkWidget     *c_following_date_radiobutton;
	GtkWidget     *c_current_date_radiobutton;
	GtkWidget     *c_creation_date_radiobutton;
	GtkWidget     *c_exif_date_radiobutton;

	GtkWidget     *note_text_view;
	GtkTextBuffer *note_text_buffer;

	GtkWidget     *save_changed_checkbutton;

	CommentData   *original_cdata;
	gboolean       have_exif_data;
} DialogData;


/* called when the main dialog is closed. */
static void
destroy_cb (GtkWidget  *widget, 
	    DialogData *data)
{
	data->window->comments_dlg = NULL;

	g_object_unref (data->gui);
	g_list_foreach (data->file_list, (GFunc) g_free, NULL);
	g_list_free (data->file_list);
	comment_data_free (data->original_cdata);

	g_free (data);
}


static void
date_check_toggled_cb (GtkWidget *widget,
		       DialogData *data)
{
	gtk_widget_set_sensitive (data->c_date_vbox, GTK_TOGGLE_BUTTON (widget)->active);
}


static int
strcmp_null_tollerant (const char *s1, const char *s2)
{
	if ((s1 == NULL) && (s2 == NULL))
		return 0;
	else if ((s1 != NULL) && (s2 == NULL))
		return 1;
	else if ((s1 == NULL) && (s2 != NULL))
		return -1;
	else 
		return strcmp (s1, s2);
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


/* called when the "ok" button is pressed. */
static void
ok_clicked_cb (GtkWidget  *widget, 
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
		cdata->place = g_locale_from_utf8 (text, -1, NULL, NULL, NULL);
	
	/* Date */

	if (GTK_TOGGLE_BUTTON (data->date_checkbutton)->active) {
		time_t     t;
		struct tm *tm_date;

		t = gnome_date_edit_get_time (GNOME_DATE_EDIT (data->date_dateedit));
		tm_date = localtime (&t);
		tm.tm_year = tm_date->tm_year;
		tm.tm_mon = tm_date->tm_mon;
		tm.tm_mday = tm_date->tm_mday;
		tm.tm_hour = tm_date->tm_hour;
		tm.tm_min = tm_date->tm_min;
		tm.tm_sec = tm_date->tm_sec;
		tm.tm_isdst = tm_date->tm_isdst;
	}
	cdata->time = mktime (&tm);
	if (cdata->time <= 0)
		cdata->time = 0;

	/* Comment */

	gtk_text_buffer_get_bounds (data->note_text_buffer, 
				    &start_iter, 
				    &end_iter);
	comment_text = gtk_text_buffer_get_text (data->note_text_buffer, 
					 &start_iter, 
					 &end_iter, 
					 FALSE);

	if (comment_text != NULL)
		cdata->comment = g_locale_from_utf8 (comment_text, -1, NULL, NULL, NULL);
	g_free (comment_text);

	/**/

	if (save_changed && (data->original_cdata != NULL)) {
		CommentData *o_cdata = data->original_cdata;

		/* NULL-ify equal fields. */

		if (text_field_cmp (cdata->place, o_cdata->place) == 0) 
			if (cdata->place != NULL) {
				g_free (cdata->place);
				cdata->place = NULL;
			}

		if (cdata->time == o_cdata->time)
			cdata->time = -1;
		
		if (text_field_cmp (cdata->comment, o_cdata->comment) == 0) 
			if (cdata->comment != NULL) {
				g_free (cdata->comment);
				cdata->comment = NULL;
			}
	}

	/* Save and close. */

	for (scan = data->file_list; scan; scan = scan->next) {
		const char *filename = scan->data;
		if (save_changed)
			comments_save_comment_non_null (filename, cdata);
		else
			comments_save_comment (filename, cdata);
		all_windows_notify_update_comment (filename); 
	}

	comment_data_free (cdata);
	gtk_widget_destroy (data->dialog);
}


/* called when the "help" button in the search dialog is pressed. */
static void
help_cb (GtkWidget  *widget, 
	 DialogData *data)
{
	GError *err;

	err = NULL;  
	gnome_help_display ("gthumb", "comments", &err);
	
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


static void
c_following_date_toggled_cb (GtkToggleButton  *button, 
			     DialogData       *data)
{
	gtk_widget_set_sensitive (data->date_dateedit, gtk_toggle_button_get_active (button));
}


static void
c_current_date_toggled_cb (GtkToggleButton  *button, 
			   DialogData       *data)
{
	if (! gtk_toggle_button_get_active (button))
		return;

	gnome_date_edit_set_time (GNOME_DATE_EDIT (data->date_dateedit),
				  time (NULL));
}


static void
c_creation_date_toggled_cb (GtkToggleButton  *button, 
			    DialogData       *data)
{
	char *first_image = data->file_list->data;

	if (! gtk_toggle_button_get_active (button))
		return;

	gnome_date_edit_set_time (GNOME_DATE_EDIT (data->date_dateedit), 
				  get_file_ctime (first_image));
}


#ifdef HAVE_LIBEXIF

static void
c_exif_date_toggled_cb (GtkToggleButton  *button, 
			DialogData       *data)
{
	char         *first_image = data->file_list->data;
	ExifData     *edata;
	unsigned int  i, j;
	time_t        time = 0;
	struct tm     tm = { 0, };

	if (! gtk_toggle_button_get_active (button))
		return;

	edata = exif_data_new_from_file (first_image);
	if (edata == NULL) 
		return;

	for (i = 0; i < EXIF_IFD_COUNT; i++) {
		ExifContent *content = edata->ifd[i];

		if (! edata->ifd[i] || ! edata->ifd[i]->count) 
			continue;

		for (j = 0; j < content->count; j++) {
			ExifEntry *e = content->entries[j];
			char      *data;

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

	if (time == 0)
		return;

	gnome_date_edit_set_time (GNOME_DATE_EDIT (data->date_dateedit), time);
}

#endif



void
dlg_edit_comment (GtkWidget *widget, gpointer wdata)
{
	GThumbWindow      *window = wdata;
	DialogData        *data;
	ImageList         *ilist;
	GtkWidget         *btn_ok;
	GtkWidget         *btn_cancel;
	GtkWidget         *btn_help;
	CommentData       *cdata;
	GList             *scan;
	char              *first_image;

	if (window->comments_dlg != NULL) {
		gtk_window_present (GTK_WINDOW (window->comments_dlg));
		return;
	}

	data = g_new (DialogData, 1);

	data->window = window;

	data->gui = glade_xml_new (GTHUMB_GLADEDIR "/" COMMENT_GLADE_FILE , NULL, NULL);
        if (!data->gui) {
                g_warning ("Could not find " GLADE_FILE "\n");
                return;
        }

	ilist = IMAGE_LIST (window->file_list->ilist);
	data->file_list = ilist_utils_get_file_list_selection (ilist);
	if (data->file_list == NULL) {
		g_free (data);
		return;
	}

	/* Get the widgets. */

	data->dialog = glade_xml_get_widget (data->gui, "comments_dialog");
	window->comments_dlg = data->dialog;

	data->place_entry = glade_xml_get_widget (data->gui, "place_entry");
	data->date_checkbutton = glade_xml_get_widget (data->gui, "date_checkbutton");
	data->date_dateedit = glade_xml_get_widget (data->gui, "date_dateedit");

	data->c_date_vbox = glade_xml_get_widget (data->gui, "c_date_vbox");
	data->c_following_date_radiobutton = glade_xml_get_widget (data->gui, "c_following_date_radiobutton");
	data->c_current_date_radiobutton = glade_xml_get_widget (data->gui, "c_current_date_radiobutton");
	data->c_creation_date_radiobutton = glade_xml_get_widget (data->gui, "c_creation_date_radiobutton");
	data->c_exif_date_radiobutton = glade_xml_get_widget (data->gui, "c_exif_date_radiobutton");

	data->note_text_view = glade_xml_get_widget (data->gui, "note_text");

	data->save_changed_checkbutton = glade_xml_get_widget (data->gui, "save_changed_checkbutton");

        btn_ok = glade_xml_get_widget (data->gui, "ok_button");
        btn_cancel = glade_xml_get_widget (data->gui, "close_button");
	btn_help = glade_xml_get_widget (data->gui, "help_button");

	/* Set widgets data. */

	data->note_text_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (data->note_text_view));

	if (g_list_length (data->file_list) > 1) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->save_changed_checkbutton), TRUE);
	} else
		gtk_widget_set_sensitive (data->save_changed_checkbutton, FALSE);

	/**/

#ifndef HAVE_LIBEXIF
	gtk_widget_hide (data->c_exif_date_radiobutton);
	data->have_exif_data = FALSE;
#else
	{
		ExifData *edata;
		char     *first_image = data->file_list->data;

		edata = exif_data_new_from_file (first_image);
		data->have_exif_data = edata != NULL;

		if (edata != NULL) 
			exif_data_unref (edata);
	}
#endif

	first_image = data->file_list->data;
	data->original_cdata = cdata = comments_load_comment (first_image);
	
	if (cdata != NULL) {
		comment_data_free_keywords (cdata);

		/* NULL-ify a field if it is not the same in all comments. */
		for (scan = data->file_list->next; scan; scan = scan->next) {
			CommentData *scan_cdata;


			scan_cdata = comments_load_comment (scan->data);
			if (scan_cdata == NULL)
				break;

			if (strcmp_null_tollerant (cdata->place, 
						   scan_cdata->place) != 0) 
				if (cdata->place != NULL) {
					g_free (cdata->place);
					cdata->place = NULL;
				}
			
			if (cdata->time != scan_cdata->time)
				cdata->time = 0;
			
			if (strcmp_null_tollerant (cdata->comment, 
						   scan_cdata->comment) != 0) 
				if (cdata->comment != NULL) {
					g_free (cdata->comment);
					cdata->comment = NULL;
				}
			
			comment_data_free (scan_cdata);
		}
	}

	gtk_widget_set_sensitive (data->c_date_vbox, FALSE);

	if (cdata != NULL) {
		char *utf8;

		if (cdata->comment != NULL)
			utf8 = g_locale_to_utf8 (cdata->comment, -1,
						 NULL, NULL, NULL);
		else
			utf8 = NULL;
		if (cdata->comment != NULL)
			gtk_text_buffer_set_text (data->note_text_buffer,
						  utf8, 
						  strlen (utf8));
		g_free (utf8);

		/**/

		if (cdata->place != NULL)
			_gtk_entry_set_locale_text (GTK_ENTRY (data->place_entry), cdata->place);

		/**/

		if (cdata->time > 0) {
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->date_checkbutton), TRUE);
			gnome_date_edit_set_time (GNOME_DATE_EDIT (data->date_dateedit), cdata->time);
			gtk_widget_set_sensitive (data->c_date_vbox, TRUE);

		} else 
			gnome_date_edit_set_time (GNOME_DATE_EDIT (data->date_dateedit), get_file_ctime (first_image));

	} else 
		gnome_date_edit_set_time (GNOME_DATE_EDIT (data->date_dateedit), get_file_ctime (first_image));

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
	g_signal_connect (G_OBJECT (btn_help), 
			  "clicked",
			  G_CALLBACK (help_cb),
			  data);
	g_signal_connect (G_OBJECT (data->date_checkbutton),
			  "toggled",
			  G_CALLBACK (date_check_toggled_cb),
			  data);

	g_signal_connect (G_OBJECT (data->c_following_date_radiobutton), 
			  "toggled",
			  G_CALLBACK (c_following_date_toggled_cb),
			  data);
	g_signal_connect (G_OBJECT (data->c_current_date_radiobutton), 
			  "toggled",
			  G_CALLBACK (c_current_date_toggled_cb),
			  data);
	g_signal_connect (G_OBJECT (data->c_creation_date_radiobutton), 
			  "toggled",
			  G_CALLBACK (c_creation_date_toggled_cb),
			  data);
#ifdef HAVE_LIBEXIF
	g_signal_connect (G_OBJECT (data->c_exif_date_radiobutton), 
			  "toggled",
			  G_CALLBACK (c_exif_date_toggled_cb),
			  data);
#endif

	/* run dialog. */

	gtk_widget_grab_focus (data->note_text_view);

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), 
				      GTK_WINDOW (window->app));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), FALSE);
	gtk_widget_show (data->dialog);
}
