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
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include <libgnomevfs/gnome-vfs-mime.h>
#include <libgnomevfs/gnome-vfs-mime-sniff-buffer.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <glade/glade.h>

#ifdef HAVE_LIBEXIF
#include <exif-data.h>
#include <exif-content.h>
#include <exif-entry.h>
#endif /* HAVE_LIBEXIF */

#include "comments.h"
#include "dlg-image-prop.h"
#include "file-data.h"
#include "gthumb-window.h"
#include "gtk-utils.h"
#include "image-viewer.h"
#include "main.h"
#include "icons/pixbufs.h"


enum {
	NAME_COLUMN,
	VALUE_COLUMN,
	NUM_COLUMNS
};


#define GLADE_FILE "gthumb.glade"
#define PREVIEW_SIZE 80


typedef struct {
	GThumbWindow *window;
	GladeXML     *gui;

	GtkWidget    *dialog;
	GtkWidget    *i_name_label;
	GtkWidget    *i_type_label;
	GtkWidget    *i_image_size_label;
	GtkWidget    *i_file_size_label;
	GtkWidget    *i_location_label;
	GtkWidget    *i_date_modified_label;

	GtkWidget    *viewer;

	GtkWidget     *i_comment_textview;
	GtkTextBuffer *i_comment_textbuffer;
	GtkWidget     *i_categories_label;

#ifdef HAVE_LIBEXIF

	GtkWidget     *i_exif_treeview;
	GtkListStore  *i_exif_model;

#endif /* HAVE_LIBEXIF */

} DialogData;


/* called when the main dialog is closed. */
static void
destroy_cb (GtkWidget  *widget, 
	    DialogData *data)
{
	data->window->image_prop_dlg = NULL;
	g_object_unref (data->gui);
	g_free (data);
}


static void
image_loaded_cb (GtkWidget    *widget, 
		 DialogData   *data)
{
	gtk_widget_queue_resize (data->viewer);
}


static void
prev_image_cb (GtkWidget    *widget, 
	       DialogData   *data)
{
	window_show_prev_image (data->window);
}


static void
next_image_cb (GtkWidget    *widget, 
	       DialogData   *data)
{
	window_show_next_image (data->window);
}


GtkWidget *
dlg_image_prop_new (GThumbWindow *window)
{
	DialogData  *data;
	GtkWidget   *i_image_vbox;
	GtkWidget   *i_close_button;
	GtkWidget   *i_next_button;
	GtkWidget   *i_prev_button;
	GtkWidget   *i_field_label;
	GtkWidget   *i_notebook;

#ifdef HAVE_LIBEXIF

	GtkCellRenderer   *renderer;
	GtkTreeViewColumn *column;

#endif /* HAVE_LIBEXIF */

	char        *label;

	data = g_new0 (DialogData, 1);
	data->window = window;
	data->gui = glade_xml_new (GTHUMB_GLADEDIR "/" GLADE_FILE, NULL,
				   NULL);

	if (! data->gui) {
		g_warning ("Could not find " GLADE_FILE "\n");
		g_free (data);
		return NULL;
	}

	/* Get the widgets. */

	data->dialog = glade_xml_get_widget (data->gui, "image_prop_window");
	data->i_name_label = glade_xml_get_widget (data->gui, "i_name_label");
	data->i_type_label = glade_xml_get_widget (data->gui, "i_type_label");
	data->i_image_size_label = glade_xml_get_widget (data->gui, "i_image_size_label");
	data->i_file_size_label = glade_xml_get_widget (data->gui, "i_file_size_label");
	data->i_location_label = glade_xml_get_widget (data->gui, "i_location_label");
	data->i_date_modified_label = glade_xml_get_widget (data->gui, "i_date_modified_label");

	i_image_vbox = glade_xml_get_widget (data->gui, "i_image_vbox");
	i_close_button = glade_xml_get_widget (data->gui, "i_close_button");
	i_next_button = glade_xml_get_widget (data->gui, "i_next_button");
	i_prev_button = glade_xml_get_widget (data->gui, "i_prev_button");
	i_notebook = glade_xml_get_widget (data->gui, "i_notebook");

	data->i_comment_textview = glade_xml_get_widget (data->gui, "i_comment_textview");
	data->i_categories_label = glade_xml_get_widget (data->gui, "i_categories_label");

#ifdef HAVE_LIBEXIF
	data->i_exif_treeview = glade_xml_get_widget (data->gui, "i_exif_treeview");
#endif /* HAVE_LIBEXIF */

	data->i_comment_textbuffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (data->i_comment_textview));

	/* * set labels */

	i_field_label = glade_xml_get_widget (data->gui, "i_field1_label");
	label = g_strdup_printf ("<b>%s:</b>", _("Name"));
	gtk_label_set_markup (GTK_LABEL (i_field_label), label);
	g_free (label);

	i_field_label = glade_xml_get_widget (data->gui, "i_field2_label");
	label = g_strdup_printf ("<b>%s:</b>", _("Image Dimensions"));
	gtk_label_set_markup (GTK_LABEL (i_field_label), label);
	g_free (label);

	i_field_label = glade_xml_get_widget (data->gui, "i_field3_label");
	label = g_strdup_printf ("<b>%s:</b>", _("Type"));
	gtk_label_set_markup (GTK_LABEL (i_field_label), label);
	g_free (label);

	i_field_label = glade_xml_get_widget (data->gui, "i_field4_label");
	label = g_strdup_printf ("<b>%s:</b>", _("File Size"));
	gtk_label_set_markup (GTK_LABEL (i_field_label), label);
	g_free (label);

	i_field_label = glade_xml_get_widget (data->gui, "i_field5_label");
	label = g_strdup_printf ("<b>%s:</b>", _("Location"));
	gtk_label_set_markup (GTK_LABEL (i_field_label), label);
	g_free (label);

	i_field_label = glade_xml_get_widget (data->gui, "i_field6_label");
	label = g_strdup_printf ("<b>%s:</b>", _("Last Modified"));
	gtk_label_set_markup (GTK_LABEL (i_field_label), label);
	g_free (label);

	/* * comment data */

	/* * exif data */

#ifndef HAVE_LIBEXIF

	gtk_notebook_remove_page (GTK_NOTEBOOK (i_notebook), 2);

#endif /* ! HAVE_LIBEXIF */

	/* * image viewer */

	data->viewer = image_viewer_new ();
	image_viewer_size (IMAGE_VIEWER (data->viewer), PREVIEW_SIZE, PREVIEW_SIZE);
	gtk_box_pack_start (GTK_BOX (i_image_vbox), data->viewer, FALSE, FALSE, 0);
	image_viewer_set_zoom_change (IMAGE_VIEWER (data->viewer), ZOOM_CHANGE_FIT_IF_LARGER);
	image_viewer_set_zoom_quality (IMAGE_VIEWER (data->viewer),
				       pref_get_zoom_quality ());
	image_viewer_set_check_type (IMAGE_VIEWER (data->viewer),
				     image_viewer_get_check_type (IMAGE_VIEWER (window->viewer)));
	image_viewer_set_check_size (IMAGE_VIEWER (data->viewer),
				     image_viewer_get_check_size (IMAGE_VIEWER (window->viewer)));
	image_viewer_set_transp_type (IMAGE_VIEWER (data->viewer),
				      image_viewer_get_transp_type (IMAGE_VIEWER (window->viewer)));
	image_viewer_stop_animation (IMAGE_VIEWER (data->viewer));
	GTK_WIDGET_UNSET_FLAGS (data->viewer, GTK_CAN_FOCUS);

	/* Set widgets data. */

	g_object_set_data (G_OBJECT (data->dialog), "dialog_data", data);

#ifdef HAVE_LIBEXIF

	data->i_exif_model = gtk_list_store_new (NUM_COLUMNS,
						 G_TYPE_STRING, 
						 G_TYPE_STRING);
	gtk_tree_view_set_model (GTK_TREE_VIEW (data->i_exif_treeview),
				 GTK_TREE_MODEL (data->i_exif_model));
	g_object_unref (data->i_exif_model);

	/**/

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Field"),
							   renderer,
							   "text", NAME_COLUMN,
							   NULL);
	gtk_tree_view_column_set_sort_column_id (column, NAME_COLUMN);
	gtk_tree_view_append_column (GTK_TREE_VIEW (data->i_exif_treeview),
				     column);

	/**/

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Value"),
							   renderer,
							   "text", VALUE_COLUMN,
							   NULL);
	gtk_tree_view_column_set_sort_column_id (column, VALUE_COLUMN);
	gtk_tree_view_append_column (GTK_TREE_VIEW (data->i_exif_treeview),
				     column);

	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (data->i_exif_model), NAME_COLUMN, GTK_SORT_ASCENDING);

#endif /* HAVE_LIBEXIF */

	/* Set the signals handlers. */
	
	g_signal_connect (G_OBJECT (data->dialog),
			  "destroy",
			  G_CALLBACK (destroy_cb),
			  data);

	g_signal_connect_swapped (G_OBJECT (i_close_button), 
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  G_OBJECT (data->dialog));
	g_signal_connect (G_OBJECT (i_next_button), 
			  "clicked",
			  G_CALLBACK (next_image_cb),
			  data);
	g_signal_connect (G_OBJECT (i_prev_button), 
			  "clicked",
			  G_CALLBACK (prev_image_cb),
			  data);

	g_signal_connect (G_OBJECT (data->viewer), 
			  "image_loaded",
			  G_CALLBACK (image_loaded_cb), 
			  data);
	
	/* Run dialog. */

	gtk_widget_show_all (data->dialog);

	dlg_image_prop_update (data->dialog);

	return data->dialog;
}


#ifdef HAVE_LIBEXIF


static void
update_exif_data (DialogData *data)
{
	ExifData     *edata;
	unsigned int  i, j;

	gtk_list_store_clear (data->i_exif_model);
	
	edata = exif_data_new_from_file (data->window->image_path);

	if (edata == NULL) 
                return;

	for (i = 0; i < EXIF_IFD_COUNT; i++) {
		ExifContent *content = edata->ifd[i];

		if (! edata->ifd[i] || ! edata->ifd[i]->count) 
			continue;

		for (j = 0; j < content->count; j++) {
			ExifEntry   *e = content->entries[j];
			GtkTreeIter  iter;
			char        *utf8_name;
			char        *utf8_value;

			if (! content->entries[j]) 
				continue;

			gtk_list_store_append (data->i_exif_model, &iter);

			utf8_name = g_locale_to_utf8 (exif_tag_get_name (e->tag), -1, 0, 0, 0);
			utf8_value = g_locale_to_utf8 (exif_entry_get_value (e), -1, 0, 0, 0);

			gtk_list_store_set (data->i_exif_model, &iter,
					    NAME_COLUMN, utf8_name,
					    VALUE_COLUMN, utf8_value,
					    -1);

			g_free (utf8_name);
			g_free (utf8_value);
		}
	}

	exif_data_unref (edata);
}


#endif /* HAVE_LIBEXIF */


static void
update_comment (DialogData *data)
{
	CommentData *cdata;
	char        *comment;

	cdata = comments_load_comment (data->window->image_path);

	if (cdata == NULL) {
		GtkTextIter  start_iter, end_iter;
		gtk_text_buffer_get_bounds (data->i_comment_textbuffer,
					    &start_iter, 
					    &end_iter);
		gtk_text_buffer_delete     (data->i_comment_textbuffer,
					    &start_iter, 
					    &end_iter);
		gtk_label_set_text (GTK_LABEL (data->i_categories_label), "");
                return;
	}

	comment = comments_get_comment_as_string (cdata, "\n", " - ");

	if (comment != NULL) {
		char          *utf8_text;

		utf8_text = g_locale_to_utf8 (comment, -1, 0, 0, 0);
		gtk_text_buffer_set_text (data->i_comment_textbuffer,
					  utf8_text, 
					  strlen (utf8_text));
		g_free (utf8_text);
	} else {
		GtkTextIter  start_iter, end_iter;
		gtk_text_buffer_get_bounds (data->i_comment_textbuffer,
					    &start_iter, 
					    &end_iter);
		gtk_text_buffer_delete     (data->i_comment_textbuffer,
					    &start_iter, 
					    &end_iter);
	}

	if (cdata->keywords_n > 0) {
		GString   *keywords;
		char      *utf8_text;
		int        i;

		keywords = g_string_new (cdata->keywords[0]);
		for (i = 1; i < cdata->keywords_n; i++) {
			g_string_append (keywords, ", ");
			g_string_append (keywords, cdata->keywords[i]);
		}
		g_string_append (keywords, ".");
		utf8_text = g_locale_to_utf8 (keywords->str, -1, 0, 0, 0);

		gtk_label_set_text (GTK_LABEL (data->i_categories_label), utf8_text);

		g_string_free (keywords, TRUE);
		g_free (utf8_text);
	} else
		gtk_label_set_text (GTK_LABEL (data->i_categories_label), "");

	g_free (comment);
	comment_data_free (cdata);
}


void
dlg_image_prop_update (GtkWidget *image_prop_dlg)
{
	DialogData   *data;
	GThumbWindow *window;
	int           width, height;
	char         *file_size_txt;
	char         *size_txt;
	char         *location;
	char          time_txt[50];
	time_t        timer;
	struct tm    *tm;
	char         *utf8_name;
	char         *title;

	g_return_if_fail (image_prop_dlg != NULL);

	data = g_object_get_data (G_OBJECT (image_prop_dlg), "dialog_data");
	g_return_if_fail (data != NULL);

	window = data->window;

	if (window->image_path == NULL) {
		image_viewer_set_void (IMAGE_VIEWER (data->viewer));
		gtk_label_set_text (GTK_LABEL (data->i_name_label), "");
		gtk_label_set_text (GTK_LABEL (data->i_type_label), "");
		gtk_label_set_text (GTK_LABEL (data->i_image_size_label), "");
		gtk_label_set_text (GTK_LABEL (data->i_file_size_label), "");
		gtk_label_set_text (GTK_LABEL (data->i_location_label), "");
		gtk_label_set_text (GTK_LABEL (data->i_date_modified_label), "");
	} else {

		utf8_name = g_locale_to_utf8 (file_name_from_path (window->image_path), -1, 0, 0, 0);
		gtk_label_set_text (GTK_LABEL (data->i_name_label), utf8_name);
		
		title = g_strdup_printf (_("%s Properties"), utf8_name); 
		gtk_window_set_title (GTK_WINDOW (data->dialog), title);
		g_free (utf8_name);
		g_free (title);
		
		/**/
		
		gtk_label_set_text (GTK_LABEL (data->i_type_label),
				    gnome_vfs_mime_get_description (gnome_vfs_get_file_mime_type (window->image_path, NULL, FALSE)));
		
		/**/
		
		width = image_viewer_get_image_width (IMAGE_VIEWER (window->viewer));
		height = image_viewer_get_image_height (IMAGE_VIEWER (window->viewer));
		size_txt = g_strdup_printf (_("%d x %d pixels"), width, height);
		gtk_label_set_text (GTK_LABEL (data->i_image_size_label), size_txt);
		g_free (size_txt);
		
		/**/
		
		file_size_txt = gnome_vfs_format_file_size_for_display (get_file_size (window->image_path));
		gtk_label_set_text (GTK_LABEL (data->i_file_size_label), file_size_txt);
		g_free (file_size_txt);
		
		/**/
		
		location = remove_level_from_path (window->image_path);
		_gtk_label_set_locale_text (GTK_LABEL (data->i_location_label), location);
		g_free (location);
		
		/**/
		
		timer = get_file_mtime (window->image_path);
		tm = localtime (&timer);
		strftime (time_txt, 50, _("%d %B %Y, %H:%M"), tm);
		_gtk_label_set_locale_text (GTK_LABEL (data->i_date_modified_label), time_txt);
	}
		
	update_comment (data);

#ifdef HAVE_LIBEXIF

	update_exif_data (data);

#endif /* HAVE_LIBEXIF */

	if (window->image_path != NULL) 
		image_viewer_load_from_image_loader (IMAGE_VIEWER (data->viewer), IMAGE_VIEWER (window->viewer)->loader);
}
