/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001-2010 The Free Software Foundation, Inc.
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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <gtk/gtk.h>
#include <gthumb.h>
#include "gth-change-date-task.h"


#define GET_WIDGET(x) (_gtk_builder_get_widget (data->builder, (x)))
#define IS_ACTIVE(x) (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (x)))


typedef struct {
	GthBrowser *browser;
	GtkBuilder *builder;
	GtkWidget  *dialog;
	GtkWidget  *date_selector;
	GList      *file_list;
} DialogData;


static void
dialog_destroy_cb (GtkWidget  *widget,
		   DialogData *data)
{
	_g_object_list_unref (data->file_list);
	g_object_unref (data->builder);
	g_free (data);
}


static void
ok_button_clicked (GtkWidget  *button,
		   DialogData *data)
{
	GthChangeFields  change_fields;
	GthChangeType    change_type;
	GthDateTime     *date_time;
	int              time_adjustment;
	GthTask         *task;

	date_time = NULL;

	change_fields = 0;
	if (IS_ACTIVE (GET_WIDGET ("change_last_modified_checkbutton")))
		change_fields |= GTH_CHANGE_LAST_MODIFIED_DATE;
	if (IS_ACTIVE (GET_WIDGET ("change_comment_checkbutton")))
		change_fields |= GTH_CHANGE_COMMENT_DATE;
	if (IS_ACTIVE (GET_WIDGET ("change_datetimeoriginal_checkbutton")))
		change_fields |= GTH_CHANGE_EXIF_DATETIMEORIGINAL_TAG;

	change_type = 0;
	time_adjustment = 0;
	if (IS_ACTIVE (GET_WIDGET ("to_following_date_radiobutton"))) {
		change_type = GTH_CHANGE_TO_FOLLOWING_DATE;
		date_time = gth_datetime_new ();
		gth_time_selector_get_value (GTH_TIME_SELECTOR (data->date_selector), date_time);
	}
	else if (IS_ACTIVE (GET_WIDGET ("to_last_modified_date_radiobutton")))
		change_type = GTH_CHANGE_TO_FILE_MODIFIED_DATE;
	else if (IS_ACTIVE (GET_WIDGET ("to_creation_date_radiobutton")))
		change_type = GTH_CHANGE_TO_FILE_CREATION_DATE;
	else if (IS_ACTIVE (GET_WIDGET ("to_photo_original_date_radiobutton")))
		change_type = GTH_CHANGE_TO_PHOTO_ORIGINAL_DATE;
	else if (IS_ACTIVE (GET_WIDGET ("adjust_timezone_radiobutton"))) {
		change_type = GTH_CHANGE_ADJUST_TIMEZONE;
		time_adjustment = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (GET_WIDGET ("adjust_timezone_spinbutton")));
	}

	task = gth_change_date_task_new (gth_browser_get_location (data->browser),
					 data->file_list,
					 change_fields,
					 change_type,
					 date_time,
					 time_adjustment);
	gth_browser_exec_task (data->browser, task, FALSE);
	gtk_widget_destroy (data->dialog);

	g_object_unref (task);
	gth_datetime_free (date_time);
}


static void
help_button_cb (GtkWidget  *widget,
		DialogData *data)
{
	show_help_dialog (GTK_WINDOW (data->dialog), "gthumb-batch-change-date");
}


static void
update_sensitivity (DialogData *data)
{
	gtk_widget_set_sensitive (GET_WIDGET ("ok_button"),
				  (IS_ACTIVE (GET_WIDGET ("change_last_modified_checkbutton"))
				   || IS_ACTIVE (GET_WIDGET ("change_comment_checkbutton"))
				   || IS_ACTIVE (GET_WIDGET ("change_datetimeoriginal_checkbutton"))));
	gtk_widget_set_sensitive (data->date_selector, IS_ACTIVE (GET_WIDGET ("to_following_date_radiobutton")));
	gtk_widget_set_sensitive (GET_WIDGET ("timezone_box"), IS_ACTIVE (GET_WIDGET ("adjust_timezone_radiobutton")));

	if (IS_ACTIVE (GET_WIDGET ("change_last_modified_checkbutton"))) {
		gtk_widget_set_sensitive (GET_WIDGET ("to_last_modified_date_radiobutton"), FALSE);
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("to_last_modified_date_radiobutton"))))
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("to_following_date_radiobutton")), TRUE);
	}
	else
		gtk_widget_set_sensitive (GET_WIDGET ("to_last_modified_date_radiobutton"), TRUE);
}


static void
radio_button_clicked (GtkWidget  *button,
		      DialogData *data)
{
	update_sensitivity (data);
}


void
dlg_change_date (GthBrowser *browser,
		 GList      *file_list)
{
	DialogData  *data;
	GTimeVal     timeval;
	GthDateTime *datetime;

	data = g_new0 (DialogData, 1);
	data->browser = browser;
	data->file_list = _g_object_list_ref (file_list);
	data->builder = _gtk_builder_new_from_file ("change-date.ui", "change_date");

	/* Get the widgets. */

	data->dialog = GET_WIDGET ("change_date_dialog");
	data->date_selector = gth_time_selector_new ();
	gth_time_selector_show_time (GTH_TIME_SELECTOR (data->date_selector), TRUE, TRUE);

	datetime = gth_datetime_new ();
	g_get_current_time (&timeval);
	gth_datetime_from_timeval (datetime, &timeval);
	gth_time_selector_set_value (GTH_TIME_SELECTOR (data->date_selector), datetime);
	gtk_widget_show (data->date_selector);
	gtk_box_pack_start (GTK_BOX (GET_WIDGET ("date_selector_box")), data->date_selector, TRUE, TRUE, 0);

	gth_datetime_free (datetime);

	/* Set widgets data. */

	update_sensitivity (data);

	/* Set the signals handlers. */

	g_signal_connect (G_OBJECT (data->dialog),
			  "destroy",
			  G_CALLBACK (dialog_destroy_cb),
			  data);
	g_signal_connect_swapped (GET_WIDGET ("close_button"),
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  G_OBJECT (data->dialog));
	g_signal_connect (GET_WIDGET ("ok_button"),
			  "clicked",
			  G_CALLBACK (ok_button_clicked),
			  data);
	g_signal_connect (GET_WIDGET ("help_button"),
			  "clicked",
			  G_CALLBACK (help_button_cb),
			  data);
	g_signal_connect (GET_WIDGET ("change_last_modified_checkbutton"),
			  "clicked",
			  G_CALLBACK (radio_button_clicked),
			  data);
	g_signal_connect (GET_WIDGET ("change_comment_checkbutton"),
			  "clicked",
			  G_CALLBACK (radio_button_clicked),
			  data);
	g_signal_connect (GET_WIDGET ("change_datetimeoriginal_checkbutton"),
			  "clicked",
			  G_CALLBACK (radio_button_clicked),
			  data);
	g_signal_connect (GET_WIDGET ("to_following_date_radiobutton"),
			  "clicked",
			  G_CALLBACK (radio_button_clicked),
			  data);
	g_signal_connect (GET_WIDGET ("to_last_modified_date_radiobutton"),
			  "clicked",
			  G_CALLBACK (radio_button_clicked),
			  data);
	g_signal_connect (GET_WIDGET ("to_creation_date_radiobutton"),
			  "clicked",
			  G_CALLBACK (radio_button_clicked),
			  data);
	g_signal_connect (GET_WIDGET ("to_photo_original_date_radiobutton"),
			  "clicked",
			  G_CALLBACK (radio_button_clicked),
			  data);
	g_signal_connect (GET_WIDGET ("adjust_timezone_radiobutton"),
			  "clicked",
			  G_CALLBACK (radio_button_clicked),
			  data);

	/* run dialog. */

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog),
				      GTK_WINDOW (browser));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), FALSE);
	gtk_widget_show (data->dialog);
}
