/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001, 2003 Free Software Foundation, Inc.
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
#include <libgnome/libgnome.h>
#include <libgnomevfs/gnome-vfs-mime.h>
#include <glade/glade.h>

#ifdef HAVE_LIBEXIF
#include <exif-data.h>
#include <exif-content.h>
#include <exif-entry.h>
#include "jpegutils/jpeg-data.h"
#endif /* HAVE_LIBEXIF */

#include "file-data.h"
#include "gconf-utils.h"
#include "gthumb-window.h"
#include "gtk-utils.h"
#include "image-viewer.h"
#include "main.h"
#include "icons/pixbufs.h"
#include "jpegutils/jpegtran.h"
#include "pixbuf-utils.h"


#define ROTATE_GLADE_FILE "gthumb_tools.glade"
#define PROGRESS_GLADE_FILE "gthumb_png_exporter.glade"

#define PREVIEW_SIZE 256

enum {
	TRAN_ROTATE_0,
	TRAN_ROTATE_90,
	TRAN_ROTATE_180,
	TRAN_ROTATE_270,
	TRAN_NONE,
	TRAN_MIRROR,
	TRAN_FLIP
};

typedef struct {
	GThumbWindow *window;
	GladeXML     *gui;

	GtkWidget    *dialog;
	GtkWidget    *j_button_box;
	GtkWidget    *j_apply_to_all_checkbutton;

	int           rot_type;    /* 0, 90 ,180, 270 */
	int           tran_type;   /* mirror, flip */
	GList        *file_list;
	GList        *current_image;
	GtkWidget    *viewer;
} DialogData;


/* called when the main dialog is closed. */
static void
destroy_cb (GtkWidget  *widget, 
	    DialogData *data)
{
	file_data_list_free (data->file_list);
	g_object_unref (data->gui);
	g_free (data);
}


static void
add_image_to_button (GtkWidget    *button, 
		     const guint8 *rgba)
{
	GdkPixbuf *pixbuf;
	GtkWidget *image;

	pixbuf = gdk_pixbuf_new_from_inline (-1, rgba, FALSE, NULL);
	image = gtk_image_new_from_pixbuf (pixbuf);
	g_object_unref (pixbuf);
	gtk_container_add (GTK_CONTAINER (button), image);
}


static void
image_loaded_cb (ImageViewer *viewer,
		 DialogData  *data)
{
	gtk_widget_set_sensitive (data->j_button_box, TRUE);
}


static void
load_current_image (DialogData *data)
{
	FileData *fd;

	if (data->current_image == NULL) {
		gtk_widget_destroy (data->dialog);
		return;
	}
	
	gtk_widget_set_sensitive (data->j_button_box, FALSE);

	fd = data->current_image->data;
	image_viewer_load_image (IMAGE_VIEWER (data->viewer), fd->path);

	data->rot_type = TRAN_ROTATE_0;
	data->tran_type = TRAN_NONE;
}


static void
load_next_image (DialogData *data)
{
	if (data->current_image == NULL) {
		gtk_widget_destroy (data->dialog);
		return;
	}
	
	data->current_image = data->current_image->next;
	if (data->current_image == NULL) {
		gtk_widget_destroy (data->dialog);
		return;
	}

	load_current_image (data);
}


#ifdef HAVE_LIBEXIF


static gboolean
swap_fields (ExifContent *content,
	     ExifTag      tag1,
	     ExifTag      tag2)
{
	ExifEntry     *entry1;
	ExifEntry     *entry2;
	unsigned char *data;
	unsigned int   size;
	
	entry1 = exif_content_get_entry (content, tag1);
	if (entry1 == NULL) 
		return FALSE;
	
	entry2 = exif_content_get_entry (content, tag2);
	if (entry2 == NULL)
		return FALSE;

	data = entry1->data;
	size = entry1->size;
	
	entry1->data = entry2->data;
	entry1->size = entry2->size;
	
	entry2->data = data;
	entry2->size = size;

	return TRUE;
}


static void
swap_xy_exif_fields (const char *filename)
{
	JPEGData     *jdata;
	ExifData     *edata;
	unsigned int  i;

	jdata = jpeg_data_new_from_file (filename);
	if (jdata == NULL)
		return;

	edata = jpeg_data_get_exif_data (jdata);
	if (edata == NULL) {
		jpeg_data_unref (jdata);
		return;
	}

	for (i = 0; i < EXIF_IFD_COUNT; i++) {
		ExifContent *content = edata->ifd[i];

		if ((content == NULL) || (content->count == 0)) 
			continue;
		
		swap_fields (content, 
			     EXIF_TAG_RELATED_IMAGE_WIDTH,
			     EXIF_TAG_RELATED_IMAGE_LENGTH);
		swap_fields (content, 
			     EXIF_TAG_IMAGE_WIDTH,
			     EXIF_TAG_IMAGE_LENGTH);
		swap_fields (content, 
			     EXIF_TAG_PIXEL_X_DIMENSION,
			     EXIF_TAG_PIXEL_Y_DIMENSION);
		swap_fields (content, 
			     EXIF_TAG_X_RESOLUTION,
			     EXIF_TAG_Y_RESOLUTION);
		swap_fields (content, 
			     EXIF_TAG_FOCAL_PLANE_X_RESOLUTION,
			     EXIF_TAG_FOCAL_PLANE_Y_RESOLUTION);
	}

	jpeg_data_save_file (jdata, filename);

	exif_data_unref (edata);
	jpeg_data_unref (jdata);
}


#endif


static void
apply_tran (DialogData *data,
	    GList      *current_image)
{
	FileData   *fd = current_image->data;
	int         rot_type = data->rot_type; 
	int         tran_type = data->tran_type; 
	char       *line;
	char       *tmp1, *tmp2;
	static int  count = 0;
	GError     *err = NULL;
	GtkWindow  *parent = GTK_WINDOW (data->dialog);
#ifdef HAVE_LIBJPEG
	JXFORM_CODE transf;
#else
	char       *command;
#endif
	char       *e1, *e2;

	if ((rot_type == TRAN_ROTATE_0) && (tran_type == TRAN_NONE))
		return;

	if (rot_type == TRAN_ROTATE_0)
		tmp1 = g_strdup (fd->path);
	else {
		tmp1 = g_strdup_printf ("%s/gthumb.%d.%d",
					g_get_tmp_dir (), 
					getpid (),
					count++);

#ifdef HAVE_LIBJPEG
		switch (rot_type) {
		case TRAN_ROTATE_90:
			transf = JXFORM_ROT_90;
			break;
		case TRAN_ROTATE_180:
			transf = JXFORM_ROT_180;
			break;
		case TRAN_ROTATE_270:
			transf = JXFORM_ROT_270;
			break;
		default:
			transf = JXFORM_NONE;
			break;
		}

		if (jpegtran (fd->path, tmp1, transf, &err) != 0) {
			g_free (tmp1);
			if (err != NULL) 
				_gtk_error_dialog_from_gerror_run (parent, &err);
			return;
		}

#else
		switch (rot_type) {
		case TRAN_ROTATE_90:
			command = "-rotate 90"; 
			break;
		case TRAN_ROTATE_180:
			command = "-rotate 180"; 
			break;
		case TRAN_ROTATE_270:
			command = "-rotate 270"; 
			break;
		default:
			break;
		}

		e1 = shell_escape (tmp1);
		e2 = shell_escape (fd->path);
	
		line = g_strdup_printf ("jpegtran -copy all %s -outfile %s %s",
					command, e1, e2);
		g_spawn_command_line_sync (line, NULL, NULL, NULL, &err);  

		g_free (e1);
		g_free (e2);
		g_free (line);

		if (err != NULL) {
			g_free (tmp1);
			_gtk_error_dialog_from_gerror_run (parent, &err);
			return;
		}
#endif
	}

	if (tran_type == TRAN_NONE)
		tmp2 = g_strdup (tmp1);
	else {
		tmp2 = g_strdup_printf ("%s/gthumb.%d.%d",
					g_get_tmp_dir (), 
					getpid (),
					count++);

#ifdef HAVE_LIBJPEG
		switch (tran_type) {
		case TRAN_MIRROR:
			transf = JXFORM_FLIP_H;
			break;
		case TRAN_FLIP:
			transf = JXFORM_FLIP_V;
			break;
		default:
			transf = JXFORM_NONE;
			break;
		}

		if (jpegtran (tmp1, tmp2, transf, &err) != 0) {
			g_free (tmp1);
			if (err != NULL) 
				_gtk_error_dialog_from_gerror_run (parent, &err);
			return;
		}

#else
		switch (tran_type) {
		case TRAN_MIRROR:
			command = "-flip horizontal"; 
			break;
		case TRAN_FLIP:
			command = "-flip vertical"; 
			break;
		default:
			return;
		}

		e1 = shell_escape (tmp2);
		e2 = shell_escape (tmp1);

		line = g_strdup_printf ("jpegtran -copy all %s -outfile %s %s",
					command, e1, e2);
		g_spawn_command_line_sync (line, NULL, NULL, NULL, &err);  

		g_free (e1);
		g_free (e2);
		g_free (line);

		if (err != NULL) {
			g_free (tmp1);
			g_free (tmp2);
			_gtk_error_dialog_from_gerror_run (parent, &err);
			return;
		}
#endif
	}

	e1 = shell_escape (tmp2);
	e2 = shell_escape (fd->path);

	line = g_strdup_printf ("mv -f %s %s", e1, e2);
	g_spawn_command_line_sync (line, NULL, NULL, NULL, &err);  

	if (err != NULL) {
		_gtk_error_dialog_from_gerror_run (parent, &err);

	} else {
		GList *list;

#ifdef HAVE_LIBEXIF
		if ((rot_type == TRAN_ROTATE_90) || (rot_type == TRAN_ROTATE_270))
			swap_xy_exif_fields (fd->path);
#endif

		list = g_list_prepend (NULL, fd->path);
		all_windows_notify_files_changed (list);
		g_list_free (list);
	}

	g_free (e1);
	g_free (e2);
	g_free (line);
	g_free (tmp1);
	g_free (tmp2);
}


static void
ok_clicked (GtkWidget  *button, 
	    DialogData *data)
{
	gboolean to_all;

	to_all = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->j_apply_to_all_checkbutton));

	if (to_all) {
		GladeXML  *gui;
		GtkWidget *dialog;
		GtkWidget *label;
		GtkWidget *bar;
		GList     *scan;
		int        i, n;

		gtk_widget_hide (data->dialog);

		gui = glade_xml_new (GTHUMB_GLADEDIR "/" PROGRESS_GLADE_FILE, 
				     NULL,
				     NULL);

		dialog = glade_xml_get_widget (gui, "progress_dialog");
		label = glade_xml_get_widget (gui, "progress_info");
		bar = glade_xml_get_widget (gui, "progress_progressbar");

		n = g_list_length (data->current_image);

		gtk_widget_show (dialog);

		while (gtk_events_pending())
			gtk_main_iteration();

		i = 0;
		for (scan = data->current_image; scan; scan = scan->next) {
			FileData *fd = scan->data;

			_gtk_label_set_locale_text (GTK_LABEL (label), fd->name);
			gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (bar),
						       (gdouble) i / n);

			while (gtk_events_pending())
				gtk_main_iteration();

			apply_tran (data, scan);

			i++;
		}

		gtk_widget_destroy (dialog);
		g_object_unref (gui);

		gtk_widget_destroy (data->dialog);

	} else {
		apply_tran (data, data->current_image);
		load_next_image (data);
	}
}


static int 
get_next_rot (int rot)
{
	int next_rot [4] = {TRAN_ROTATE_90, 
			    TRAN_ROTATE_180, 
			    TRAN_ROTATE_270,
			    TRAN_ROTATE_0};
	return next_rot [rot];
}


static void
revert_clicked (GtkWidget  *button, 
		DialogData *data)
{
	load_current_image (data);
}


static void
rot90_clicked (GtkWidget  *button, 
	       DialogData *data)
{
	ImageViewer  *viewer = IMAGE_VIEWER (data->viewer);
	GdkPixbuf    *src_pixbuf;
	GdkPixbuf    *dest_pixbuf;

	if (data->tran_type == TRAN_NONE)
		data->rot_type = get_next_rot (data->rot_type);
	else {
		data->rot_type = get_next_rot (data->rot_type);
		data->rot_type = get_next_rot (data->rot_type);
		data->rot_type = get_next_rot (data->rot_type);
	}

	src_pixbuf = image_viewer_get_current_pixbuf (viewer);

	if (src_pixbuf == NULL) 
		return;

	dest_pixbuf = _gdk_pixbuf_copy_rotate_90 (src_pixbuf, FALSE);
	image_viewer_set_pixbuf (viewer, dest_pixbuf);

	if (dest_pixbuf != NULL) 
		g_object_unref (dest_pixbuf);
}


static void
rot270_clicked (GtkWidget  *button, 
		DialogData *data)
{
	ImageViewer  *viewer = IMAGE_VIEWER (data->viewer);
	GdkPixbuf    *src_pixbuf;
	GdkPixbuf    *dest_pixbuf;

	if (data->tran_type == TRAN_NONE) {
		data->rot_type = get_next_rot (data->rot_type);
		data->rot_type = get_next_rot (data->rot_type);
		data->rot_type = get_next_rot (data->rot_type);
	} else
		data->rot_type = get_next_rot (data->rot_type);

	src_pixbuf = image_viewer_get_current_pixbuf (viewer);

	if (src_pixbuf == NULL) 
		return;

	dest_pixbuf = _gdk_pixbuf_copy_rotate_90 (src_pixbuf, TRUE);
	image_viewer_set_pixbuf (viewer, dest_pixbuf);

	if (dest_pixbuf != NULL) 
		g_object_unref (dest_pixbuf);
}


static void
mirror_clicked (GtkWidget  *button, 
		DialogData *data)
{
	ImageViewer *viewer = IMAGE_VIEWER (data->viewer);
	GdkPixbuf   *src_pixbuf;
	GdkPixbuf   *dest_pixbuf;

	src_pixbuf = image_viewer_get_current_pixbuf (viewer);

	if (src_pixbuf == NULL) 
		return;

	if (data->tran_type == TRAN_FLIP) {
		data->tran_type = TRAN_NONE;
		data->rot_type = get_next_rot (data->rot_type);
		data->rot_type = get_next_rot (data->rot_type);

	} else if (data->tran_type == TRAN_MIRROR)
		data->tran_type = TRAN_NONE;

	else
		data->tran_type = TRAN_MIRROR;
	
	dest_pixbuf = _gdk_pixbuf_copy_mirror (src_pixbuf, TRUE, FALSE);
	image_viewer_set_pixbuf (viewer, dest_pixbuf);
	
	if (dest_pixbuf != NULL) 
		g_object_unref (dest_pixbuf);
}


static void
flip_clicked (GtkWidget  *button, 
	      DialogData *data)
{
	ImageViewer  *viewer = IMAGE_VIEWER (data->viewer);
	GdkPixbuf    *src_pixbuf;
	GdkPixbuf    *dest_pixbuf;

	if (data->tran_type == TRAN_MIRROR) {
		data->tran_type = TRAN_NONE;
		data->rot_type = get_next_rot (data->rot_type);
		data->rot_type = get_next_rot (data->rot_type);

	} else if (data->tran_type == TRAN_FLIP)
		data->tran_type = TRAN_NONE;

	else
		data->tran_type = TRAN_FLIP;

	src_pixbuf = image_viewer_get_current_pixbuf (viewer);

	if (src_pixbuf == NULL) 
		return;

	dest_pixbuf = _gdk_pixbuf_copy_mirror (src_pixbuf, FALSE, TRUE);
	image_viewer_set_pixbuf (viewer, dest_pixbuf);

	if (dest_pixbuf != NULL) 
		g_object_unref (dest_pixbuf);
}


/* called when the "help" button is clicked. */
static void
help_cb (GtkWidget  *widget, 
	 DialogData *data)
{
	GError *err;

	err = NULL;  
	gnome_help_display ("gthumb", "rotate-jpeg", &err);
	
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


void
dlg_jpegtran (GThumbWindow *window)
{
	DialogData  *data;
	GtkWidget   *j_image_vbox;
	GtkWidget   *j_revert_button;
	GtkWidget   *j_rot_90_button;
	GtkWidget   *j_rot_270_button;
	GtkWidget   *j_v_flip_button;
	GtkWidget   *j_h_flip_button;
	GtkWidget   *j_help_button;
	GtkWidget   *j_cancel_button;
	GtkWidget   *j_ok_button;
	GList       *list, *scan;

	list = gth_file_list_get_selection_as_fd (window->file_list);
	if (list == NULL) {
		g_warning ("No file selected.");
		return;
	}

	/* remove non jpeg images */

	for (scan = list; scan; ) {
		FileData *fd = scan->data;
		GList    *next = scan->next;

		if (! image_is_jpeg (fd->path)) {
			list = g_list_remove_link (list, scan);
			g_list_free (scan);
			file_data_unref (fd);
		}

		scan = next;
	}

	if (list == NULL) {
		g_warning ("No JPEG image selected");
		return;
	}

	/**/

	data = g_new (DialogData, 1);

	data->file_list = list;
	data->current_image = list;
	data->gui = glade_xml_new (GTHUMB_GLADEDIR "/" ROTATE_GLADE_FILE, NULL,
				   NULL);

	if (! data->gui) {
		g_warning ("Could not find " ROTATE_GLADE_FILE "\n");
		if (data->file_list != NULL) 
			g_list_free (data->file_list);
		g_free (data);
		return;
	}

	/* Get the widgets. */

	data->dialog = glade_xml_get_widget (data->gui, "jpeg_rotate_dialog");
	data->j_apply_to_all_checkbutton = glade_xml_get_widget (data->gui, "j_apply_to_all_checkbutton");
	data->j_button_box = glade_xml_get_widget (data->gui, "j_button_box");

	j_image_vbox = glade_xml_get_widget (data->gui, "j_image_vbox");
	j_revert_button = glade_xml_get_widget (data->gui, "j_revert_button");
	j_rot_90_button = glade_xml_get_widget (data->gui, "j_rot_90_button");
	j_rot_270_button = glade_xml_get_widget (data->gui, "j_rot_270_button");
	j_v_flip_button = glade_xml_get_widget (data->gui, "j_v_flip_button");
	j_h_flip_button = glade_xml_get_widget (data->gui, "j_h_flip_button");

	j_help_button = glade_xml_get_widget (data->gui, "j_help_button");
	j_cancel_button = glade_xml_get_widget (data->gui, "j_cancel_button");
	j_ok_button = glade_xml_get_widget (data->gui, "j_ok_button");

	/* image viewer */

	data->viewer = image_viewer_new ();
	image_viewer_size (IMAGE_VIEWER (data->viewer), PREVIEW_SIZE, PREVIEW_SIZE);
	gtk_container_add (GTK_CONTAINER (j_image_vbox), data->viewer);
	image_viewer_set_zoom_change (IMAGE_VIEWER (data->viewer), GTH_ZOOM_CHANGE_FIT_IF_LARGER);
	image_viewer_set_zoom_quality (IMAGE_VIEWER (data->viewer),
				       pref_get_zoom_quality ());
	image_viewer_set_check_type (IMAGE_VIEWER (data->viewer),
				     image_viewer_get_check_type (IMAGE_VIEWER (window->viewer)));
	image_viewer_set_check_size (IMAGE_VIEWER (data->viewer),
				     image_viewer_get_check_size (IMAGE_VIEWER (window->viewer)));
	image_viewer_set_transp_type (IMAGE_VIEWER (data->viewer),
				      image_viewer_get_transp_type (IMAGE_VIEWER (window->viewer)));

	/* Set widgets data. */

	add_image_to_button (j_rot_90_button, rotate_90_24_rgba);
	add_image_to_button (j_rot_270_button, rotate_270_24_rgba);
	add_image_to_button (j_v_flip_button, mirror_24_rgba);
	add_image_to_button (j_h_flip_button, flip_24_rgba);

	gtk_widget_set_sensitive (data->j_apply_to_all_checkbutton,
				  data->file_list->next != NULL);

	/* Set the signals handlers. */
	
	g_signal_connect (G_OBJECT (data->dialog),
			  "destroy",
			  G_CALLBACK (destroy_cb),
			  data);

	g_signal_connect_swapped (G_OBJECT (j_cancel_button), 
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  G_OBJECT (data->dialog));
	g_signal_connect (G_OBJECT (j_help_button), 
			  "clicked",
			  G_CALLBACK (help_cb),
			  data);
	g_signal_connect (G_OBJECT (j_ok_button), 
			  "clicked",
			  G_CALLBACK (ok_clicked),
			  data);
	g_signal_connect (G_OBJECT (j_revert_button), 
			  "clicked",
			  G_CALLBACK (revert_clicked),
			  data);
	g_signal_connect (G_OBJECT (j_rot_90_button), 
			  "clicked",
			  G_CALLBACK (rot90_clicked),
			  data);
	g_signal_connect (G_OBJECT (j_rot_270_button), 
			  "clicked",
			  G_CALLBACK (rot270_clicked),
			  data);
	g_signal_connect (G_OBJECT (j_v_flip_button), 
			  "clicked",
			  G_CALLBACK (mirror_clicked),
			  data);
	g_signal_connect (G_OBJECT (j_h_flip_button), 
			  "clicked",
			  G_CALLBACK (flip_clicked),
			  data);

	g_signal_connect (G_OBJECT (data->viewer),
			  "image_loaded",
			  G_CALLBACK (image_loaded_cb),
			  data);

	/* Run dialog. */

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), GTK_WINDOW (window->app));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), TRUE); 
	gtk_widget_show_all (data->dialog);

	load_current_image (data);
}
