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
#include <math.h>
#include <gtk/gtk.h>
#include <libgnomevfs/gnome-vfs-mime.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <glade/glade.h>

#include "comments.h"
#include "dlg-image-prop.h"
#include "file-data.h"
#include "gthumb-histogram.h"
#include "gthumb-window.h"
#include "gth-exif-data-viewer.h"
#include "gtk-utils.h"
#include "image-viewer.h"
#include "main.h"
#include "pixbuf-utils.h"
#include "icons/pixbufs.h"


enum {
	NAME_COLUMN,
	VALUE_COLUMN,
	POS_COLUMN,
	NUM_COLUMNS
};


enum {
	IPROP_PAGE_GENERAL = 0,
	IPROP_PAGE_COMMENT,
#ifdef HAVE_LIBEXIF
	IPROP_PAGE_EXIF,
#endif /* HAVE_LIBEXIF */
	IPROP_PAGE_HISTOGRAM,
	IPROP_NUM_PAGES
};


#define GLADE_FILE     "gthumb.glade"
#define PREVIEW_SIZE   80


typedef struct {
	GThumbWindow *window;
	ImageViewer  *window_viewer;
	GladeXML     *gui;

	GtkWidget    *dialog;
	GtkWidget    *i_notebook;
	GtkWidget    *i_name_label;
	GtkWidget    *i_type_label;
	GtkWidget    *i_image_size_label;
	GtkWidget    *i_file_size_label;
	GtkWidget    *i_location_label;
	GtkWidget    *i_date_modified_label;

	GtkWidget    *i_next_button;
	GtkWidget    *i_prev_button;

	GtkWidget    *image;

	GtkWidget     *i_comment_textview;
	GtkTextBuffer *i_comment_textbuffer;
	GtkWidget     *i_categories_label;

#ifdef HAVE_LIBEXIF
	GtkWidget     *i_exif_data_view;	
	GtkWidget     *i_exif_data_container;
#endif /* HAVE_LIBEXIF */

	GtkWidget     *i_histogram_channel;
	GtkWidget     *i_histogram_channel_alpha;
	GtkWidget     *i_histogram_graph;
	GtkWidget     *i_histogram_gradient;

	GthumbHistogram *histogram;

	gboolean       page_updated[IPROP_NUM_PAGES];
	gboolean       closing;
} DialogData;


static GdkPixbuf *unknown = NULL;
static int        unknown_ref_count = 0;


/* called when the main dialog is closed. */
static void
destroy_cb (GtkWidget  *widget, 
	    DialogData *data)
{
	data->window->image_prop_dlg = NULL;
	gthumb_histogram_free (data->histogram);
	g_object_unref (data->gui);
	g_free (data);

	if (--unknown_ref_count == 0) {
		g_object_unref (unknown);
		unknown = NULL;
	}
}


static gboolean
histogram_graph_expose_cb (GtkWidget      *widget,
			   GdkEventExpose *event,
			   DialogData     *data)
{
	GthumbHistogram *histogram = data->histogram;
	int              w, h, i, y;
	float            step;
	double           max;
	int              channel;

	w = widget->allocation.width;
	h = widget->allocation.height;
	step = w / 256.0;

	channel = gthumb_histogram_get_current_channel (histogram);

	/* Draw an X through the graph area if user wants alpha channela nd
	 * the image has no alpha channel */
	if (channel > gthumb_histogram_get_nchannels (histogram)) {
		gdk_draw_line (widget->window, 
		               widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
		               0, 0, w, h);
		gdk_draw_line (widget->window, 
		               widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
		               w, 0, 0, h);
		return FALSE;
	}
		
	max = gthumb_histogram_get_max (histogram, channel);
	if (max > 0.0)
		max = log (max);
	else
		max = 1.0;

	for (i = 0; i < 256; i++) {
		double value = gthumb_histogram_get_value (histogram, channel, i);
		y = (int) (h * log (value)) / max;

		gdk_draw_rectangle (widget->window, 
		                    widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
		                    TRUE,
		                    i * step, 
		                    h - y, 
		                    1 + step, 
				    h);
	}

	return FALSE;
}


static gboolean
histogram_gradient_expose_cb (GtkWidget      *widget,
			      GdkEventExpose *event,
			      DialogData     *data)
{
	int       i, w, h;
	float     step;
	GdkGC    *gc;
	GdkColor  color;
	int       channel, shade;

	w = widget->allocation.width;
	h = widget->allocation.height;
	step = w / 256.0;

	channel = gthumb_histogram_get_current_channel (data->histogram);

	/* display a black/white gradient for alpha channel */
	if (channel == 4)
		channel = 0;

	gc = gdk_gc_new (widget->window);

	/* draw the gradient in the necessary color for this channel */
	for (i = 0; i < 256; i++) {
		shade = (i << 8) + i;
		color.red = (channel == 0 || channel == 1) ? shade : 0;
		color.green = (channel == 0 || channel == 2) ? shade : 0;
		color.blue = (channel == 0 || channel == 3) ? shade : 0;
		gdk_gc_set_rgb_fg_color (gc, &color);
		
		gdk_draw_rectangle (widget->window, 
		                    gc,
		                    TRUE,
		                    i * step, 
		                    0, 
		                    1 + step, 
		                    h);
	}

	g_object_unref (gc);

	return FALSE;
}


static void
histogram_channel_changed_cb (GtkOptionMenu *option_menu, 
			      DialogData   *data)
{
	int channel = gtk_option_menu_get_history(option_menu);

	/* Take menu separator into account */
	if (channel > 1)
		channel--;

	gthumb_histogram_set_current_channel (data->histogram, channel);

	gtk_widget_queue_draw (GTK_WIDGET (data->i_histogram_graph));
	gtk_widget_queue_draw (GTK_WIDGET (data->i_histogram_gradient));
}


static void
dialog_delete_event_cb (GtkWidget   *caller, 
			GdkEvent    *event, 
			DialogData  *data)
{
	data->closing = TRUE;
	gtk_widget_destroy (data->dialog);
}


static void
i_close_button_clicked_cb (GtkWidget    *widget, 
			   DialogData   *data)
{
	data->closing = TRUE;
	gtk_widget_destroy (data->dialog);
}


static void
prev_image_cb (GtkWidget    *widget, 
	       DialogData   *data)
{
	window_show_prev_image (data->window, FALSE);
}


static void
next_image_cb (GtkWidget    *widget, 
	       DialogData   *data)
{
	window_show_next_image (data->window, FALSE);
}


static void
update_comment (DialogData *data)
{
	CommentData *cdata;
	char        *comment;

	g_return_if_fail (GTK_IS_WIDGET (data->i_categories_label));
	g_return_if_fail (GTK_IS_TEXT_BUFFER (data->i_comment_textbuffer));

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
		GtkTextIter iter;

		gtk_text_buffer_set_text (data->i_comment_textbuffer,
					  comment,
					  strlen (comment));

		gtk_text_buffer_get_iter_at_line (data->i_comment_textbuffer,
						  &iter,
						  0);
		gtk_text_buffer_place_cursor (data->i_comment_textbuffer,
					      &iter);

	} else {
		GtkTextIter start_iter, end_iter;
		gtk_text_buffer_get_bounds (data->i_comment_textbuffer,
					    &start_iter, 
					    &end_iter);
		gtk_text_buffer_delete     (data->i_comment_textbuffer,
					    &start_iter, 
					    &end_iter);
	}

	if (cdata->keywords_n > 0) {
		GString   *keywords;
		int        i;

		keywords = g_string_new (cdata->keywords[0]);
		for (i = 1; i < cdata->keywords_n; i++) {
			g_string_append (keywords, ", ");
			g_string_append (keywords, cdata->keywords[i]);
		}
		g_string_append (keywords, ".");

		gtk_label_set_text (GTK_LABEL (data->i_categories_label), keywords->str);

		g_string_free (keywords, TRUE);
	} else
		gtk_label_set_text (GTK_LABEL (data->i_categories_label), "");

	g_free (comment);
	comment_data_free (cdata);
}


static void
update_histogram (DialogData *data)
{
	ImageViewer  *viewer;
	GdkPixbuf    *pixbuf;

	viewer = IMAGE_VIEWER (data->window->viewer);
	pixbuf = image_viewer_get_current_pixbuf (viewer);

	gthumb_histogram_calculate (data->histogram, pixbuf);

	/* disable alpha menu option if image has no alpha channel */

	if (gthumb_histogram_get_nchannels (data->histogram) < 4)
		gtk_widget_set_sensitive (data->i_histogram_channel_alpha, FALSE);
	else
		gtk_widget_set_sensitive (data->i_histogram_channel_alpha, TRUE);

	gtk_widget_queue_draw (GTK_WIDGET (data->i_histogram_graph));
	gtk_widget_queue_draw (GTK_WIDGET (data->i_histogram_gradient));
}


static void
update_general_info (DialogData *data)
{
	GThumbWindow *window;
	ImageViewer  *viewer;
	GdkPixbuf    *pixbuf;
	int           width, height;
	char         *file_size_txt;
	char         *size_txt;
	char         *location;
	char          time_txt[50];
	time_t        timer;
	struct tm    *tm;
	char         *utf8_name;

	window = data->window;
	viewer = data->window_viewer;
	pixbuf = image_viewer_get_current_pixbuf (viewer);

	if (window->image_path == NULL) {
		gtk_label_set_text (GTK_LABEL (data->i_name_label), "");
		gtk_label_set_text (GTK_LABEL (data->i_type_label), "");
		gtk_label_set_text (GTK_LABEL (data->i_image_size_label), "");
		gtk_label_set_text (GTK_LABEL (data->i_file_size_label), "");
		gtk_label_set_text (GTK_LABEL (data->i_location_label), "");
		gtk_label_set_text (GTK_LABEL (data->i_date_modified_label), "");
	} else {
		utf8_name = g_filename_to_utf8 (file_name_from_path (window->image_path), -1, 0, 0, 0);
		gtk_label_set_text (GTK_LABEL (data->i_name_label), utf8_name);
		g_free (utf8_name);
		
		/**/
		
		gtk_label_set_text (GTK_LABEL (data->i_type_label),
				    gnome_vfs_mime_get_description (gnome_vfs_get_file_mime_type (window->image_path, NULL, FALSE)));
		
		/**/

		if (image_viewer_is_void (viewer)) {
			width = 0;
			height = 0;
		} else {
			width = image_viewer_get_image_width (viewer);
			height = image_viewer_get_image_height (viewer);
		}

		size_txt = g_strdup_printf (_("%d x %d pixels"), width, height);
		gtk_label_set_text (GTK_LABEL (data->i_image_size_label), size_txt);
		g_free (size_txt);
		
		/**/
		
		file_size_txt = gnome_vfs_format_file_size_for_display (get_file_size (window->image_path));
		gtk_label_set_text (GTK_LABEL (data->i_file_size_label), file_size_txt);
		g_free (file_size_txt);
		
		/**/
		
		location = remove_level_from_path (window->image_path);
		_gtk_label_set_filename_text (GTK_LABEL (data->i_location_label), location);
		g_free (location);
		
		/**/
		
		timer = get_file_mtime (window->image_path);
		tm = localtime (&timer);
		strftime (time_txt, 50, _("%d %B %Y, %H:%M"), tm);
		_gtk_label_set_locale_text (GTK_LABEL (data->i_date_modified_label), time_txt);
}

	pixbuf = image_viewer_get_current_pixbuf (viewer);

	if ((window->image_path != NULL) && (pixbuf != NULL)) {
		GdkPixbuf *scaled = NULL;
		int        width;
		int        height;
		
		g_object_ref (pixbuf);
		
		width = gdk_pixbuf_get_width (pixbuf);
		height = gdk_pixbuf_get_height (pixbuf);
		scale_keepping_ratio (&width, &height, PREVIEW_SIZE, PREVIEW_SIZE); 
		
		scaled = gdk_pixbuf_scale_simple (pixbuf, 
						  width, 
						  height,
						  GDK_INTERP_BILINEAR);
		gtk_image_set_from_pixbuf (GTK_IMAGE (data->image), scaled);
		gtk_widget_show (data->image);
		
		if (scaled != NULL) 
			g_object_unref (scaled);
		
		g_object_unref (pixbuf);
		
	}  else 
		gtk_image_set_from_pixbuf (GTK_IMAGE (data->image), unknown);
}


static void
update_notebook_page (DialogData *data,
		      int         page_num)
{
	if (data->closing)
		return;

	if (data->page_updated[page_num]) 
		return;

	switch (page_num) {
	case IPROP_PAGE_GENERAL:
		update_general_info (data); 
		break;

	case IPROP_PAGE_COMMENT:
		update_comment (data); 
		break;

#ifdef HAVE_LIBEXIF
	case IPROP_PAGE_EXIF:
		gth_exif_data_viewer_update (GTH_EXIF_DATA_VIEWER (data->i_exif_data_view),
					     data->window_viewer,
					     data->window->image_path,
					     NULL);
		break;
#endif /* HAVE_LIBEXIF */

	case IPROP_PAGE_HISTOGRAM:
		update_histogram (data);
		break;
		
	default:
		return;
	}

	data->page_updated[page_num] = TRUE;
}


static gboolean
i_notebook_switch_page_cb (GtkWidget       *widget,
			   GtkNotebookPage *page,
			   guint            page_num,
			   gpointer         extra_data)
{
	DialogData *data = extra_data;

	if (data->closing)
		return TRUE;

	if (page_num < IPROP_NUM_PAGES)
		update_notebook_page (data, page_num);

	return FALSE;
}



GtkWidget *
dlg_image_prop_new (GThumbWindow *window)
{
	DialogData        *data;
	GtkWidget         *i_image_vbox;
	GtkWidget         *i_close_button;
	GtkWidget         *i_field_label;
	char              *label;

	data = g_new0 (DialogData, 1);
	data->window = window;
	data->window_viewer = IMAGE_VIEWER (window->viewer);
	data->gui = glade_xml_new (GTHUMB_GLADEDIR "/" GLADE_FILE, NULL, NULL);

	if (! data->gui) {
		g_warning ("Could not find " GLADE_FILE "\n");
		g_free (data);
		return NULL;
	}

	if (unknown == NULL) {
		unknown = gdk_pixbuf_new_from_inline (-1, 
						      unknown_48_rgba, 
						      FALSE, 
						      NULL);
		unknown_ref_count = 1;
	} else 
		unknown_ref_count++;

	data->histogram = gthumb_histogram_new ();

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
	data->i_next_button = glade_xml_get_widget (data->gui, "i_next_button");
	data->i_prev_button = glade_xml_get_widget (data->gui, "i_prev_button");
	data->i_notebook = glade_xml_get_widget (data->gui, "i_notebook");

	data->i_comment_textview = glade_xml_get_widget (data->gui, "i_comment_textview");
	data->i_categories_label = glade_xml_get_widget (data->gui, "i_categories_label");

#ifdef HAVE_LIBEXIF
	data->i_exif_data_container = glade_xml_get_widget (data->gui, "i_exif_data_container");
#endif /* HAVE_LIBEXIF */

	data->i_comment_textbuffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (data->i_comment_textview));

	data->i_histogram_channel_alpha = glade_xml_get_widget (data->gui, "i_histogram_channel_alpha");
	data->i_histogram_channel = glade_xml_get_widget (data->gui, "i_histogram_channel");
	data->i_histogram_graph = glade_xml_get_widget (data->gui, "i_histogram_graph");
	data->i_histogram_gradient = glade_xml_get_widget (data->gui, "i_histogram_gradient");

	/* * set labels */

	i_field_label = glade_xml_get_widget (data->gui, "i_field1_label");
	label = g_strdup_printf ("<b>%s:</b>", _("Name"));
	gtk_label_set_markup (GTK_LABEL (i_field_label), label);
	g_free (label);

	i_field_label = glade_xml_get_widget (data->gui, "i_field2_label");
	label = g_strdup_printf ("<b>%s:</b>", _("Size"));
	gtk_label_set_markup (GTK_LABEL (i_field_label), label);
	g_free (label);

	i_field_label = glade_xml_get_widget (data->gui, "i_field3_label");
	label = g_strdup_printf ("<b>%s:</b>", _("Type"));
	gtk_label_set_markup (GTK_LABEL (i_field_label), label);
	g_free (label);

	i_field_label = glade_xml_get_widget (data->gui, "i_field4_label");
	label = g_strdup_printf ("<b>%s:</b>", _("Bytes"));
	gtk_label_set_markup (GTK_LABEL (i_field_label), label);
	g_free (label);

	i_field_label = glade_xml_get_widget (data->gui, "i_field5_label");
	label = g_strdup_printf ("<b>%s:</b>", _("Location"));
	gtk_label_set_markup (GTK_LABEL (i_field_label), label);
	g_free (label);

	i_field_label = glade_xml_get_widget (data->gui, "i_field6_label");
	label = g_strdup_printf ("<b>%s:</b>", _("Modified"));
	gtk_label_set_markup (GTK_LABEL (i_field_label), label);
	g_free (label);

	i_field_label = glade_xml_get_widget (data->gui, "i_histogram_channel_label");
	label = g_strdup_printf ("<b>%s:</b>", _("Information on Channel"));
	gtk_label_set_markup (GTK_LABEL (i_field_label), label);
	g_free (label);

	/* * comment data */

	/* * exif data */

#ifndef HAVE_LIBEXIF
	gtk_notebook_remove_page (GTK_NOTEBOOK (data->i_notebook), 2);
#endif /* ! HAVE_LIBEXIF */

	/* * image viewer */

	data->image = gtk_image_new ();
	gtk_widget_set_size_request (data->image, PREVIEW_SIZE, PREVIEW_SIZE);
	gtk_box_pack_start (GTK_BOX (i_image_vbox), data->image, FALSE, FALSE, 0);

	/* Set widgets data. */

	g_object_set_data (G_OBJECT (data->dialog), "dialog_data", data);

#ifdef HAVE_LIBEXIF
	data->i_exif_data_view = gth_exif_data_viewer_new (FALSE);
	gtk_container_add (GTK_CONTAINER (data->i_exif_data_container), 
			   data->i_exif_data_view);
#endif /* HAVE_LIBEXIF */

	/* Set the signals handlers. */
	
	g_signal_connect (G_OBJECT (data->dialog),
			  "destroy",
			  G_CALLBACK (destroy_cb),
			  data);

	g_signal_connect (G_OBJECT (i_close_button), 
			  "clicked",
			  G_CALLBACK (i_close_button_clicked_cb),
			  data);
	g_signal_connect (G_OBJECT (data->dialog), 
			  "delete_event",
			  G_CALLBACK (dialog_delete_event_cb),
			  data);
	g_signal_connect (G_OBJECT (data->i_next_button), 
			  "clicked",
			  G_CALLBACK (next_image_cb),
			  data);
	g_signal_connect (G_OBJECT (data->i_prev_button), 
			  "clicked",
			  G_CALLBACK (prev_image_cb),
			  data);

	g_signal_connect (G_OBJECT (data->i_notebook), 
			  "switch_page",
			  G_CALLBACK (i_notebook_switch_page_cb),
			  data);

	g_signal_connect (G_OBJECT (data->i_histogram_graph), 
			  "expose_event",
			  G_CALLBACK (histogram_graph_expose_cb), 
			  data);

	g_signal_connect (G_OBJECT (data->i_histogram_gradient), 
			  "expose_event",
			  G_CALLBACK (histogram_gradient_expose_cb), 
			  data);

	g_signal_connect (G_OBJECT (data->i_histogram_channel), 
			  "changed",
			  G_CALLBACK (histogram_channel_changed_cb), 
			  data);

	/* Run dialog. */

	gtk_widget_show_all (data->dialog);

	dlg_image_prop_update (data->dialog);

	return data->dialog;
}


static void
update_buttons_sensitivity (DialogData *data)
{
	GThumbWindow *window;
	int           image_pos;

	window = data->window;

	if (window->image_path != NULL)
		image_pos = gth_file_list_pos_from_path (window->file_list, 
							 window->image_path);
	else
		image_pos = -1;

	gtk_widget_set_sensitive (data->i_prev_button, image_pos > 0);
	gtk_widget_set_sensitive (data->i_next_button, (image_pos != -1) && (image_pos < gth_file_view_get_images (window->file_list->view) - 1));
}


static void
update_title (DialogData *data)
{
	GThumbWindow *window;
	ImageViewer  *viewer;

	window = data->window;
	viewer = data->window_viewer;

	if (window->image_path == NULL) 
		gtk_window_set_title (GTK_WINDOW (data->dialog), "");

	else {
		char *utf8_name;
		char *title;

		utf8_name = g_filename_to_utf8 (file_name_from_path (window->image_path), -1, 0, 0, 0);
		title = g_strdup_printf (_("%s Properties"), utf8_name); 
		gtk_window_set_title (GTK_WINDOW (data->dialog), title);
		g_free (utf8_name);
		g_free (title);
	}
}


void
dlg_image_prop_update (GtkWidget *image_prop_dlg)
{
	DialogData *data;
	int         i;

	g_return_if_fail (image_prop_dlg != NULL);

	data = g_object_get_data (G_OBJECT (image_prop_dlg), "dialog_data");
	g_return_if_fail (data != NULL);

	for (i = 0; i < IPROP_NUM_PAGES; i++)
		data->page_updated[i] = FALSE;

	update_title (data);
	update_buttons_sensitivity (data);
	update_notebook_page (data, gtk_notebook_get_current_page (GTK_NOTEBOOK (data->i_notebook)));
}


void
dlg_image_prop_close  (GtkWidget *image_prop_dlg)
{
	DialogData *data;

	g_return_if_fail (image_prop_dlg != NULL);

	data = g_object_get_data (G_OBJECT (image_prop_dlg), "dialog_data");
	g_return_if_fail (data != NULL);

	data->closing = TRUE;
	gtk_widget_destroy (image_prop_dlg);
}
