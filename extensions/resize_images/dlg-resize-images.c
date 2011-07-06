/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2005-2009 Free Software Foundation, Inc.
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
#include "dlg-resize-images.h"
#include "preferences.h"


#define GET_WIDGET(name) _gtk_builder_get_widget (data->builder, (name))
#define DEFAULT_WIDTH_PERCENTAGE 100
#define DEFAULT_HEIGHT_PERCENTAGE 100
#define DEFAULT_WIDTH_PIXELS 640
#define DEFAULT_HEIGHT_PIXELS 480


enum {
	MIME_TYPE_COLUMN_ICON = 0,
	MIME_TYPE_COLUMN_TYPE,
	MIME_TYPE_COLUMN_DESCRIPTION
};


GthUnit units[] = { GTH_UNIT_PIXELS, GTH_UNIT_PERCENTAGE };


typedef struct {
	GthBrowser *browser;
	GList      *file_list;
	GtkBuilder *builder;
	GtkWidget  *dialog;
	gboolean    use_destination;
	gulong      width_spinbutton_event;
	gulong      height_spinbutton_event;
	double      latest_width_in_pixel;
	double      latest_height_in_pixel;
	double      latest_width_in_percentage;
	double      latest_height_in_percentage;
} DialogData;


typedef struct {
	int       width;
	int       height;
	GthUnit   unit;
	gboolean  keep_aspect_ratio;
	gboolean  allow_swap;
} ResizeData;


static void
destroy_cb (GtkWidget  *widget,
	    DialogData *data)
{
	gth_browser_set_dialog (data->browser, "resize_images", NULL);

	g_object_unref (data->builder);
	_g_object_list_unref (data->file_list);
	g_free (data);
}


static void
resize_step (GthPixbufTask *pixbuf_task)
{
	ResizeData *resize_data = pixbuf_task->data;
	int         w, h;
	int         new_w, new_h;
	int         max_w, max_h;

	w = gdk_pixbuf_get_width (pixbuf_task->src);
	h = gdk_pixbuf_get_height (pixbuf_task->src);

	if (resize_data->allow_swap
	    && (((h > w) && (resize_data->width > resize_data->height))
		|| ((h < w) && (resize_data->width < resize_data->height))))
	{
		max_w = resize_data->height;
		max_h = resize_data->width;
	}
	else {
		max_w = resize_data->width;
		max_h = resize_data->height;
	}

	if (resize_data->unit == GTH_UNIT_PERCENTAGE) {
		new_w = w * ((double) max_w / 100.0);
		new_h = h * ((double) max_h / 100.0);
	}
	else if (resize_data->keep_aspect_ratio) {
		new_w = w;
		new_h = h;
		scale_keeping_ratio (&new_w, &new_h, max_w, max_h, TRUE);
	}
	else {
		new_w = max_w;
		new_h = max_h;
	}

	_g_object_unref (pixbuf_task->dest);
	if ((new_w > 1) && (new_h > 1))
		pixbuf_task->dest = _gdk_pixbuf_scale_simple_safe (pixbuf_task->src, new_w, new_h, GDK_INTERP_BILINEAR);
	else
		pixbuf_task->dest = NULL;
}


static void
help_clicked_cb (GtkWidget  *widget,
		 DialogData *data)
{
	show_help_dialog (GTK_WINDOW (data->dialog), "gthumb-batch-resize");
}


static void
ok_clicked_cb (GtkWidget  *widget,
	       DialogData *data)
{
	ResizeData  *resize_data;
	GtkTreeIter  iter;
	char        *mime_type;
	GthTask     *resize_task;
	GthTask     *list_task;

	resize_data = g_new0 (ResizeData, 1);
	resize_data->width = gtk_spin_button_get_value (GTK_SPIN_BUTTON (GET_WIDGET ("width_spinbutton")));
	resize_data->height = gtk_spin_button_get_value (GTK_SPIN_BUTTON (GET_WIDGET ("height_spinbutton")));
	resize_data->unit = units[gtk_combo_box_get_active (GTK_COMBO_BOX (GET_WIDGET ("unit_combobox")))];
	resize_data->keep_aspect_ratio = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("keep_ratio_checkbutton")));
	resize_data->allow_swap = FALSE;

	gtk_combo_box_get_active_iter (GTK_COMBO_BOX (GET_WIDGET ("mime_type_combobox")), &iter);
	gtk_tree_model_get (GTK_TREE_MODEL (GET_WIDGET ("mime_type_liststore")), &iter,
			    MIME_TYPE_COLUMN_TYPE, &mime_type,
			    -1);

	eel_gconf_set_integer (PREF_RESIZE_IMAGES_SERIES_WIDTH, resize_data->width);
	eel_gconf_set_integer (PREF_RESIZE_IMAGES_SERIES_HEIGHT, resize_data->height);
	eel_gconf_set_enum (PREF_RESIZE_IMAGES_UNIT, GTH_TYPE_UNIT, resize_data->unit);
	eel_gconf_set_boolean (PREF_RESIZE_IMAGES_KEEP_RATIO, resize_data->keep_aspect_ratio);
	eel_gconf_set_string (PREF_RESIZE_IMAGES_MIME_TYPE, mime_type ? mime_type : "");

	resize_task = gth_pixbuf_task_new (_("Resizing images"),
					   TRUE,
					   NULL,
					   resize_step,
					   NULL,
					   resize_data,
					   g_free);
	list_task = gth_pixbuf_list_task_new (data->browser,
					      data->file_list,
					      GTH_PIXBUF_TASK (resize_task));
	gth_pixbuf_list_task_set_overwrite_mode (GTH_PIXBUF_LIST_TASK (list_task), GTH_OVERWRITE_ASK);
	gth_pixbuf_list_task_set_output_mime_type (GTH_PIXBUF_LIST_TASK (list_task), mime_type);
	if (data->use_destination) {
		GFile *destination;

		destination = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (GET_WIDGET ("destination_filechooserbutton")));
		gth_pixbuf_list_task_set_destination (GTH_PIXBUF_LIST_TASK (list_task), destination);

		g_object_unref (destination);
	}
	gth_browser_exec_task (data->browser, list_task, FALSE);

	g_object_unref (list_task);
	g_object_unref (resize_task);
	g_free (mime_type);
	gtk_widget_destroy (data->dialog);
}


static void
update_width_height_properties (DialogData *data)
{
	GthUnit unit;

	unit = units[gtk_combo_box_get_active (GTK_COMBO_BOX (GET_WIDGET ("unit_combobox")))];
	if (unit == GTH_UNIT_PERCENTAGE) {
		gtk_spin_button_set_digits (GTK_SPIN_BUTTON (GET_WIDGET ("width_spinbutton")), 2);
		gtk_spin_button_set_digits (GTK_SPIN_BUTTON (GET_WIDGET ("height_spinbutton")), 2);

		g_signal_handler_block (GET_WIDGET ("height_spinbutton"), data->height_spinbutton_event);
		g_signal_handler_block (GET_WIDGET ("width_spinbutton"), data->width_spinbutton_event);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("height_spinbutton")), data->latest_height_in_percentage);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("width_spinbutton")), data->latest_width_in_percentage);
		g_signal_handler_unblock (GET_WIDGET ("height_spinbutton"), data->height_spinbutton_event);
		g_signal_handler_unblock (GET_WIDGET ("width_spinbutton"), data->width_spinbutton_event);
	}
	else if (unit == GTH_UNIT_PIXELS) {
		gtk_spin_button_set_digits (GTK_SPIN_BUTTON (GET_WIDGET ("width_spinbutton")), 0);
		gtk_spin_button_set_digits (GTK_SPIN_BUTTON (GET_WIDGET ("height_spinbutton")), 0);

		g_signal_handler_block (GET_WIDGET ("height_spinbutton"), data->height_spinbutton_event);
		g_signal_handler_block (GET_WIDGET ("width_spinbutton"), data->width_spinbutton_event);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("height_spinbutton")), data->latest_height_in_pixel);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("width_spinbutton")), data->latest_width_in_pixel);
		g_signal_handler_unblock (GET_WIDGET ("height_spinbutton"), data->height_spinbutton_event);
		g_signal_handler_unblock (GET_WIDGET ("width_spinbutton"), data->width_spinbutton_event);
	}
}


static void
unit_combobox_changed_cb (GtkComboBox *combobox,
			  DialogData  *data)
{
	GthUnit unit;

	unit = units[gtk_combo_box_get_active (GTK_COMBO_BOX (GET_WIDGET ("unit_combobox")))];
	if (unit == GTH_UNIT_PERCENTAGE) {
		data->latest_width_in_pixel = gtk_spin_button_get_value (GTK_SPIN_BUTTON (GET_WIDGET ("width_spinbutton")));
		data->latest_height_in_pixel = gtk_spin_button_get_value (GTK_SPIN_BUTTON (GET_WIDGET ("height_spinbutton")));
	}
	else if (unit == GTH_UNIT_PIXELS) {
		data->latest_width_in_percentage = gtk_spin_button_get_value (GTK_SPIN_BUTTON (GET_WIDGET ("width_spinbutton")));
		data->latest_height_in_percentage = gtk_spin_button_get_value (GTK_SPIN_BUTTON (GET_WIDGET ("height_spinbutton")));
	}

	update_width_height_properties (data);
}


static void
use_destination_checkbutton_toggled_cb (GtkToggleButton *button,
					gpointer         user_data)
{
	DialogData *data = user_data;

	data->use_destination = ! gtk_toggle_button_get_active (button);
	gtk_widget_set_sensitive (GET_WIDGET ("destination_filechooserbutton"), data->use_destination);
}


static void
width_spinbutton_value_changed_cb (GtkSpinButton *spinbutton,
				   gpointer       user_data)
{
	DialogData *data = user_data;
	GthUnit     unit;

	unit = units[gtk_combo_box_get_active (GTK_COMBO_BOX (GET_WIDGET ("unit_combobox")))];
	if (unit != GTH_UNIT_PERCENTAGE)
		return;

	if (! gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("keep_ratio_checkbutton"))))
		return;

	g_signal_handler_block (GET_WIDGET ("height_spinbutton"), data->height_spinbutton_event);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("height_spinbutton")), gtk_spin_button_get_value (spinbutton));
	g_signal_handler_unblock (GET_WIDGET ("height_spinbutton"), data->height_spinbutton_event);
}


static void
height_spinbutton_value_changed_cb (GtkSpinButton *spinbutton,
				    gpointer       user_data)
{
	DialogData *data = user_data;
	GthUnit     unit;

	unit = units[gtk_combo_box_get_active (GTK_COMBO_BOX (GET_WIDGET ("unit_combobox")))];
	if (unit != GTH_UNIT_PERCENTAGE)
		return;

	if (! gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("keep_ratio_checkbutton"))))
		return;

	g_signal_handler_block (GET_WIDGET ("width_spinbutton"), data->width_spinbutton_event);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("width_spinbutton")), gtk_spin_button_get_value (spinbutton));
	g_signal_handler_unblock (GET_WIDGET ("width_spinbutton"), data->width_spinbutton_event);
}


void
dlg_resize_images (GthBrowser *browser,
		   GList      *file_list)
{
	DialogData  *data;
	GArray      *savers;
	GthFileData *first_file_data;
	GthUnit      unit;

	if (gth_browser_get_dialog (browser, "resize_images") != NULL) {
		gtk_window_present (GTK_WINDOW (gth_browser_get_dialog (browser, "resize_images")));
		return;
	}

	data = g_new0 (DialogData, 1);
	data->browser = browser;
	data->builder = _gtk_builder_new_from_file ("resize-images.ui", "resize_images");
	data->file_list = gth_file_data_list_dup (file_list);
	data->use_destination = TRUE;

	/* Get the widgets. */

	data->dialog = _gtk_builder_get_widget (data->builder, "resize_images_dialog");
	gth_browser_set_dialog (browser, "resize_images", data->dialog);
	g_object_set_data (G_OBJECT (data->dialog), "dialog_data", data);

	/* Set widgets data. */

	unit = eel_gconf_get_enum (PREF_RESIZE_IMAGES_UNIT, GTH_TYPE_UNIT, GTH_UNIT_PIXELS);
	gtk_combo_box_set_active (GTK_COMBO_BOX (GET_WIDGET ("unit_combobox")), unit);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("keep_ratio_checkbutton")), eel_gconf_get_boolean (PREF_RESIZE_IMAGES_KEEP_RATIO, TRUE));

	if (unit == GTH_UNIT_PERCENTAGE) {
		data->latest_width_in_pixel = DEFAULT_WIDTH_PIXELS;
		data->latest_height_in_pixel = DEFAULT_HEIGHT_PIXELS;
		data->latest_width_in_percentage = eel_gconf_get_integer (PREF_RESIZE_IMAGES_SERIES_WIDTH, DEFAULT_WIDTH_PERCENTAGE);
		data->latest_height_in_percentage = eel_gconf_get_integer (PREF_RESIZE_IMAGES_SERIES_HEIGHT, DEFAULT_HEIGHT_PERCENTAGE);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("width_spinbutton")), data->latest_width_in_percentage);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("height_spinbutton")), data->latest_height_in_percentage);
	}
	else if (unit == GTH_UNIT_PIXELS) {
		data->latest_width_in_percentage = DEFAULT_WIDTH_PERCENTAGE;
		data->latest_height_in_percentage = DEFAULT_HEIGHT_PERCENTAGE;
		data->latest_width_in_pixel = eel_gconf_get_integer (PREF_RESIZE_IMAGES_SERIES_WIDTH, DEFAULT_WIDTH_PIXELS);
		data->latest_height_in_pixel = eel_gconf_get_integer (PREF_RESIZE_IMAGES_SERIES_HEIGHT, DEFAULT_HEIGHT_PIXELS);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("width_spinbutton")), data->latest_width_in_pixel);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("height_spinbutton")), data->latest_height_in_pixel);
	}

	update_width_height_properties (data);

	savers = gth_main_get_type_set ("pixbuf-saver");
	if (savers != NULL) {
		GtkListStore *list_store;
		GtkTreeIter   iter;
		char         *default_mime_type;
		GthIconCache *icon_cache;
		int           i;

		list_store = (GtkListStore *) GET_WIDGET ("mime_type_liststore");
		gtk_list_store_append (list_store, &iter);
		gtk_list_store_set (list_store, &iter,
				    MIME_TYPE_COLUMN_ICON, NULL,
				    MIME_TYPE_COLUMN_TYPE, NULL,
				    MIME_TYPE_COLUMN_DESCRIPTION, _("Keep the original format"),
				    -1);
		gtk_combo_box_set_active_iter (GTK_COMBO_BOX (GET_WIDGET ("mime_type_combobox")), &iter);

		default_mime_type = eel_gconf_get_string (PREF_RESIZE_IMAGES_MIME_TYPE, "");
		icon_cache = gth_icon_cache_new_for_widget (data->dialog, GTK_ICON_SIZE_MENU);

		for (i = 0; i < savers->len; i++) {
			GType           saver_type;
			GthPixbufSaver *saver;
			const char     *mime_type;
			GdkPixbuf      *pixbuf;

			saver_type = g_array_index (savers, GType, i);
			saver = g_object_new (saver_type, NULL);
			mime_type = gth_pixbuf_saver_get_mime_type (saver);
			pixbuf = gth_icon_cache_get_pixbuf (icon_cache, g_content_type_get_icon (mime_type));
			gtk_list_store_append (list_store, &iter);
			gtk_list_store_set (list_store, &iter,
					    MIME_TYPE_COLUMN_ICON, pixbuf,
					    MIME_TYPE_COLUMN_TYPE, mime_type,
					    MIME_TYPE_COLUMN_DESCRIPTION, g_content_type_get_description (mime_type),
					    -1);

			if (strcmp (default_mime_type, mime_type) == 0)
				gtk_combo_box_set_active_iter (GTK_COMBO_BOX (GET_WIDGET ("mime_type_combobox")), &iter);

			g_object_unref (pixbuf);
			g_object_unref (saver);
		}

		gth_icon_cache_free (icon_cache);
		g_free (default_mime_type);
	}

	first_file_data = (GthFileData *) data->file_list->data;
	gtk_file_chooser_set_file (GTK_FILE_CHOOSER (GET_WIDGET ("destination_filechooserbutton")),
				   first_file_data->file,
				   NULL);

	/* Set the signals handlers. */

	g_signal_connect (G_OBJECT (data->dialog),
			  "destroy",
			  G_CALLBACK (destroy_cb),
			  data);
	g_signal_connect (GET_WIDGET ("ok_button"),
			  "clicked",
			  G_CALLBACK (ok_clicked_cb),
			  data);
        g_signal_connect (GET_WIDGET ("help_button"),
                          "clicked",
                          G_CALLBACK (help_clicked_cb),
                          data);
	g_signal_connect_swapped (GET_WIDGET ("cancel_button"),
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  G_OBJECT (data->dialog));
	g_signal_connect (GET_WIDGET ("unit_combobox"),
			  "changed",
			  G_CALLBACK (unit_combobox_changed_cb),
			  data);
	g_signal_connect (GET_WIDGET ("use_destination_checkbutton"),
			  "toggled",
			  G_CALLBACK (use_destination_checkbutton_toggled_cb),
			  data);

	data->width_spinbutton_event = g_signal_connect (GET_WIDGET ("width_spinbutton"),
							 "value-changed",
							 G_CALLBACK (width_spinbutton_value_changed_cb),
							 data);
	data->height_spinbutton_event = g_signal_connect (GET_WIDGET ("height_spinbutton"),
							  "value-changed",
							  G_CALLBACK (height_spinbutton_value_changed_cb),
							  data);

	/* Run dialog. */

        if (GTH_IS_FILE_SOURCE_VFS (gth_browser_get_location_source (browser)))
        	gtk_widget_hide (GET_WIDGET ("use_destination_checkbutton"));

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), GTK_WINDOW (browser));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), FALSE);
	gtk_widget_show (data->dialog);
}
