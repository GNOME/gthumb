/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001, 2003, 2004, 2005 Free Software Foundation, Inc.
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
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <libgnome/gnome-help.h>
#include <libgnomevfs/gnome-vfs-ops.h>
#include <libgnomevfs/gnome-vfs-mime.h>
#include <glade/glade.h>

#ifdef HAVE_LIBEXIF
#include <libexif/exif-data.h>
#include <libexif/exif-content.h>
#include <libexif/exif-entry.h>
#include <libexif/exif-utils.h>
#include "jpegutils/jpeg-data.h"
#endif /* HAVE_LIBEXIF */

#include "file-utils.h"
#include "gconf-utils.h"
#include "gth-window.h"
#include "gtk-utils.h"
#include "image-loader.h"
#include "main.h"
#include "icons/pixbufs.h"
#include "jpegutils/jpegtran.h"
#include "pixbuf-utils.h"
#include "gthumb-stock.h"
#include "gth-exif-utils.h"
#include "rotation-utils.h"


#define ROTATE_GLADE_FILE "gthumb_tools.glade"
#define PROGRESS_GLADE_FILE "gthumb_png_exporter.glade"

#define PREVIEW_SIZE 200

typedef struct {
	GthWindow    *window;
	GladeXML     *gui;

	GtkWidget    *dialog;
	GtkWidget    *j_button_box;
	GtkWidget    *j_button_vbox;
	GtkWidget    *j_revert_button;;
	GtkWidget    *j_apply_to_all_checkbutton;
	GtkWidget    *j_preview_image;
	GtkWidget    *j_from_exif_checkbutton;

	gboolean      from_exif_data;
	GList        *file_list;
	GList        *files_changed_list;
	GList        *current_image;
	ImageLoader  *loader;
	GdkPixbuf    *original_preview;
	RotationData *rot_data;
} DialogData;


static void
dialog_data_free (DialogData *data)
{
	if (data->files_changed_list != NULL) {
		all_windows_notify_files_changed (data->files_changed_list);
		path_list_free (data->files_changed_list);
		data->files_changed_list = NULL;
	}

	all_windows_add_monitor ();

	path_list_free (data->file_list);
	if (data->loader != NULL)
		g_object_unref (data->loader);
	if (data->gui != NULL)
		g_object_unref (data->gui);
	g_free (data->rot_data);
	g_free (data);
}


/* called when the main dialog is closed. */
static void
destroy_cb (GtkWidget  *widget, 
	    DialogData *data)
{
	dialog_data_free (data);
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


static GdkPixbuf *
_gdk_pixbuf_scale_keep_aspect_ratio (GdkPixbuf *pixbuf, 
				     int        max_width, 
				     int        max_height)
{
	int        width, height;
	GdkPixbuf *result = NULL;

	if (pixbuf == NULL)
		return NULL;

	width = gdk_pixbuf_get_width (pixbuf);
	height = gdk_pixbuf_get_height (pixbuf);
	if (scale_keepping_ratio (&width, &height, max_width, max_height))
		result = gdk_pixbuf_scale_simple (pixbuf, width, height, GDK_INTERP_BILINEAR);
	else {
		result = pixbuf;
		g_object_ref (result);
	}

	return result;
}



static void
update_from_exif_data (DialogData *data)
{
#ifdef HAVE_LIBEXIF
	gboolean   from_exif;
	GdkPixbuf *src_pixbuf, *dest_pixbuf;
	
	from_exif = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->j_from_exif_checkbutton));
	gtk_widget_set_sensitive (data->j_button_box, !from_exif);

	if (! from_exif)
		return;
	
	update_rotation_from_exif_data (data->current_image->data, data->rot_data);

	src_pixbuf = data->original_preview;
	if (src_pixbuf == NULL) 
		return;

	switch (data->rot_data->rot_type) {
	case GTH_TRANSFORM_ROTATE_90:
		dest_pixbuf = _gdk_pixbuf_copy_rotate_90 (src_pixbuf, FALSE);
		break;
	case GTH_TRANSFORM_ROTATE_180:
		dest_pixbuf = _gdk_pixbuf_copy_mirror (src_pixbuf, TRUE, TRUE);
		break;
	case GTH_TRANSFORM_ROTATE_270:
		dest_pixbuf = _gdk_pixbuf_copy_rotate_90 (src_pixbuf, TRUE);
		break;
	default:
		dest_pixbuf = src_pixbuf;
		g_object_ref (dest_pixbuf);
		break;
	}

	src_pixbuf = dest_pixbuf;

	switch (data->rot_data->tran_type) {
	case GTH_TRANSFORM_MIRROR:
		dest_pixbuf = _gdk_pixbuf_copy_mirror (src_pixbuf, TRUE, FALSE);
		break;
	case GTH_TRANSFORM_FLIP:
		dest_pixbuf = _gdk_pixbuf_copy_mirror (src_pixbuf, FALSE, TRUE);
		break;
	default:
		dest_pixbuf = src_pixbuf;
		g_object_ref (dest_pixbuf);
		break;
	}
	
	g_object_unref (src_pixbuf);

	gtk_image_set_from_pixbuf (GTK_IMAGE (data->j_preview_image), dest_pixbuf);
		
	g_object_unref (dest_pixbuf);

#endif /* HAVE_LIBEXIF */
}


static void 
image_loader_done_cb (ImageLoader  *il,
		      DialogData   *data)
{
	GdkPixbuf *pixbuf;

	pixbuf = image_loader_get_pixbuf (il);
	if (pixbuf != NULL) {
		if (data->original_preview != NULL)
			g_object_unref (data->original_preview);
		data->original_preview = _gdk_pixbuf_scale_keep_aspect_ratio (pixbuf, PREVIEW_SIZE, PREVIEW_SIZE);
		gtk_image_set_from_pixbuf (GTK_IMAGE (data->j_preview_image), data->original_preview);

		gtk_widget_set_sensitive (data->j_button_vbox, TRUE);
		gtk_widget_set_sensitive (data->j_revert_button, TRUE);

		update_from_exif_data (data);
	}
}


static void 
image_loader_error_cb (ImageLoader  *il,
		       DialogData   *data)
{
	gtk_widget_set_sensitive (data->j_button_vbox, TRUE);
	gtk_widget_set_sensitive (data->j_revert_button, TRUE);
}


static void
load_current_image (DialogData *data)
{
	char *path;

	if (data->current_image == NULL) {
		gtk_widget_destroy (data->dialog);
		return;
	}
	
	gtk_widget_set_sensitive (data->j_button_vbox, FALSE);
	gtk_widget_set_sensitive (data->j_revert_button, FALSE);

	path = data->current_image->data;
	image_loader_set_path (data->loader, path);
	image_loader_start (data->loader);

	data->rot_data->rot_type = GTH_TRANSFORM_ROTATE_0;
	data->rot_data->tran_type = GTH_TRANSFORM_NONE;
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


static void
notify_file_changed (DialogData *data,
		     const char *filename,
		     gboolean    notify_soon)
{
	if (notify_soon) {
		GList *list = g_list_prepend (NULL, (char*) filename);
		all_windows_notify_files_changed (list);
		g_list_free (list);
	} else 
		data->files_changed_list = g_list_prepend (data->files_changed_list, g_strdup (filename));
}



static void
apply_transformation (DialogData *data,
		      GList      *current_image,
		      gboolean    notify_soon)
{
	char             *path = current_image->data;
	char             *dir;
	GnomeVFSFileInfo  info;
	GtkWindow  	 *window = GTK_WINDOW (data->dialog);
        
	/* Check directory permissions. */

	dir = remove_level_from_path (path);
	if (! check_permissions (dir, R_OK | W_OK | X_OK)) {
		char *utf8_path = g_filename_display_name (dir);
		_gtk_error_dialog_run (GTK_WINDOW (data->dialog),
				       _("You don't have the right permissions to create images in the folder \"%s\""),
				       utf8_path);
		g_free (utf8_path);
		g_free (dir);
		return;
	} 
	g_free (dir);

	/**/

	if (data->from_exif_data)
		update_rotation_from_exif_data (path, data->rot_data);
	gnome_vfs_get_file_info (path, &info, GNOME_VFS_FILE_INFO_GET_ACCESS_RIGHTS|GNOME_VFS_FILE_INFO_FOLLOW_LINKS);
	if (image_is_jpeg (path)) 
		apply_transformation_jpeg (window, path, data->rot_data);
	else 
		apply_transformation_generic (window, path, data->rot_data);
	gnome_vfs_set_file_info (path, &info, GNOME_VFS_SET_FILE_INFO_PERMISSIONS|GNOME_VFS_SET_FILE_INFO_OWNER);

	notify_file_changed (data, path, notify_soon);
}


static void
apply_transformation_to_all (DialogData *data)
{
	GladeXML  *gui;
	GtkWidget *dialog;
	GtkWidget *label;
	GtkWidget *bar;
	GList     *scan;
	int        i, n;

	gui = glade_xml_new (GTHUMB_GLADEDIR "/" PROGRESS_GLADE_FILE, 
			     NULL,
			     NULL);

	dialog = glade_xml_get_widget (gui, "progress_dialog");
	label = glade_xml_get_widget (gui, "progress_info");
	bar = glade_xml_get_widget (gui, "progress_progressbar");

	n = g_list_length (data->current_image);

	if (data->dialog == NULL)
		gtk_window_set_transient_for (GTK_WINDOW (dialog),
					      GTK_WINDOW (data->window));
	else {
		gtk_window_set_modal (GTK_WINDOW (data->dialog), FALSE); 
		gtk_window_set_transient_for (GTK_WINDOW (dialog),
					      GTK_WINDOW (data->dialog));
	}
	gtk_window_set_modal (GTK_WINDOW (dialog), TRUE); 
	gtk_widget_show (dialog);

	while (gtk_events_pending())
		gtk_main_iteration();

	i = 0;
	for (scan = data->current_image; scan; scan = scan->next) {
		char *path = scan->data;
		char *name;

		name = g_filename_display_basename (path);
		_gtk_label_set_filename_text (GTK_LABEL (label), name);
		g_free (name);

		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (bar),
					       (gdouble) (i + 0.5) / n);
		
		while (gtk_events_pending())
			gtk_main_iteration();
		
		apply_transformation (data, scan, FALSE);
		i++;
	}
	
	gtk_widget_destroy (dialog);
	g_object_unref (gui);

	if (data->dialog == NULL)
		dialog_data_free (data);
	else
		gtk_widget_destroy (data->dialog);
}


static void
ok_clicked (GtkWidget  *button, 
	    DialogData *data)
{
	gboolean to_all;

	to_all = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->j_apply_to_all_checkbutton));
	data->from_exif_data = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->j_from_exif_checkbutton));

	if (to_all) {
		gtk_widget_hide (data->dialog);
		apply_transformation_to_all (data);
	} else {
		apply_transformation (data, data->current_image, TRUE);
		load_next_image (data);
	}
}


static int 
get_next_rot (int rot)
{
	int next_rot [4] = {GTH_TRANSFORM_ROTATE_90, 
			    GTH_TRANSFORM_ROTATE_180, 
			    GTH_TRANSFORM_ROTATE_270,
			    GTH_TRANSFORM_ROTATE_0};
	return next_rot [rot];
}


static void
revert_clicked (GtkWidget  *button, 
		DialogData *data)
{
	data->rot_data->rot_type = GTH_TRANSFORM_ROTATE_0;
	data->rot_data->tran_type = GTH_TRANSFORM_NONE;

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->j_from_exif_checkbutton), FALSE);

	if (data->original_preview != NULL)
		gtk_image_set_from_pixbuf (GTK_IMAGE (data->j_preview_image), data->original_preview);
}


static void
rot90_clicked (GtkWidget  *button, 
	       DialogData *data)
{
	GdkPixbuf *src_pixbuf;
	GdkPixbuf *dest_pixbuf;

	if (data->rot_data->tran_type == GTH_TRANSFORM_NONE)
		data->rot_data->rot_type = get_next_rot (data->rot_data->rot_type);
	else {
		data->rot_data->rot_type = get_next_rot (data->rot_data->rot_type);
		data->rot_data->rot_type = get_next_rot (data->rot_data->rot_type);
		data->rot_data->rot_type = get_next_rot (data->rot_data->rot_type);
	}

	src_pixbuf = gtk_image_get_pixbuf (GTK_IMAGE (data->j_preview_image));

	if (src_pixbuf == NULL) 
		return;

	dest_pixbuf = _gdk_pixbuf_copy_rotate_90 (src_pixbuf, FALSE);
	gtk_image_set_from_pixbuf (GTK_IMAGE (data->j_preview_image), dest_pixbuf);

	if (dest_pixbuf != NULL) 
		g_object_unref (dest_pixbuf);
}


static void
rot270_clicked (GtkWidget  *button, 
		DialogData *data)
{
	GdkPixbuf *src_pixbuf;
	GdkPixbuf *dest_pixbuf;

	if (data->rot_data->tran_type == GTH_TRANSFORM_NONE) {
		data->rot_data->rot_type = get_next_rot (data->rot_data->rot_type);
		data->rot_data->rot_type = get_next_rot (data->rot_data->rot_type);
		data->rot_data->rot_type = get_next_rot (data->rot_data->rot_type);
	} else
		data->rot_data->rot_type = get_next_rot (data->rot_data->rot_type);

	src_pixbuf = gtk_image_get_pixbuf (GTK_IMAGE (data->j_preview_image));

	if (src_pixbuf == NULL) 
		return;

	dest_pixbuf = _gdk_pixbuf_copy_rotate_90 (src_pixbuf, TRUE);
	gtk_image_set_from_pixbuf (GTK_IMAGE (data->j_preview_image), dest_pixbuf);

	if (dest_pixbuf != NULL) 
		g_object_unref (dest_pixbuf);
}


static void
mirror_clicked (GtkWidget  *button, 
		DialogData *data)
{
	GdkPixbuf *src_pixbuf;
	GdkPixbuf *dest_pixbuf;

	src_pixbuf = gtk_image_get_pixbuf (GTK_IMAGE (data->j_preview_image));

	if (src_pixbuf == NULL) 
		return;

	if (data->rot_data->tran_type == GTH_TRANSFORM_FLIP) {
		data->rot_data->tran_type = GTH_TRANSFORM_NONE;
		data->rot_data->rot_type = get_next_rot (data->rot_data->rot_type);
		data->rot_data->rot_type = get_next_rot (data->rot_data->rot_type);

	} else if (data->rot_data->tran_type == GTH_TRANSFORM_MIRROR)
		data->rot_data->tran_type = GTH_TRANSFORM_NONE;

	else
		data->rot_data->tran_type = GTH_TRANSFORM_MIRROR;
	
	dest_pixbuf = _gdk_pixbuf_copy_mirror (src_pixbuf, TRUE, FALSE);
	gtk_image_set_from_pixbuf (GTK_IMAGE (data->j_preview_image), dest_pixbuf);
	
	if (dest_pixbuf != NULL) 
		g_object_unref (dest_pixbuf);
}


static void
flip_clicked (GtkWidget  *button, 
	      DialogData *data)
{
	GdkPixbuf *src_pixbuf;
	GdkPixbuf *dest_pixbuf;

	if (data->rot_data->tran_type == GTH_TRANSFORM_MIRROR) {
		data->rot_data->tran_type = GTH_TRANSFORM_NONE;
		data->rot_data->rot_type = get_next_rot (data->rot_data->rot_type);
		data->rot_data->rot_type = get_next_rot (data->rot_data->rot_type);

	} else if (data->rot_data->tran_type == GTH_TRANSFORM_FLIP)
		data->rot_data->tran_type = GTH_TRANSFORM_NONE;

	else
		data->rot_data->tran_type = GTH_TRANSFORM_FLIP;

	src_pixbuf = gtk_image_get_pixbuf (GTK_IMAGE (data->j_preview_image));

	if (src_pixbuf == NULL) 
		return;

	dest_pixbuf = _gdk_pixbuf_copy_mirror (src_pixbuf, FALSE, TRUE);
	gtk_image_set_from_pixbuf (GTK_IMAGE (data->j_preview_image), dest_pixbuf);

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
	gnome_help_display ("gthumb", "gthumb-rotate-jpeg", &err);
	
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
from_exif_toggled_cb (GtkToggleButton *button,
		      DialogData      *data)
{
	update_from_exif_data (data);
}


void
dlg_jpegtran (GthWindow *window)
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
	GtkWidget   *reset_image;
	GList       *list;


	list = gth_window_get_file_list_selection (window);
	if (list == NULL) {
		g_warning ("No file selected.");
		return;
	}

	/**/

	data = g_new0 (DialogData, 1);

	data->window = window;
	data->file_list = list;
	data->current_image = list;
	data->gui = glade_xml_new (GTHUMB_GLADEDIR "/" ROTATE_GLADE_FILE, NULL,
				   NULL);

	if (! data->gui) {
		g_warning ("Could not find " ROTATE_GLADE_FILE "\n");
		if (data->file_list != NULL) 
			path_list_free (data->file_list);
		g_free (data);
		return;
	}

	data->rot_data = rotation_data_new ();

	/* Get the widgets. */

	data->dialog = glade_xml_get_widget (data->gui, "jpeg_rotate_dialog");
	data->j_apply_to_all_checkbutton = glade_xml_get_widget (data->gui, "j_apply_to_all_checkbutton");
	data->j_button_box = glade_xml_get_widget (data->gui, "j_button_box");
	data->j_button_vbox = glade_xml_get_widget (data->gui, "j_button_vbox");
	data->j_revert_button = glade_xml_get_widget (data->gui, "j_revert_button");
	data->j_preview_image = glade_xml_get_widget (data->gui, "j_preview_image");

	j_image_vbox = glade_xml_get_widget (data->gui, "j_image_vbox");
	j_revert_button = glade_xml_get_widget (data->gui, "j_revert_button");
	j_rot_90_button = glade_xml_get_widget (data->gui, "j_rot_90_button");
	j_rot_270_button = glade_xml_get_widget (data->gui, "j_rot_270_button");
	j_v_flip_button = glade_xml_get_widget (data->gui, "j_v_flip_button");
	j_h_flip_button = glade_xml_get_widget (data->gui, "j_h_flip_button");

	data->j_from_exif_checkbutton = glade_xml_get_widget (data->gui, "j_from_exif_checkbutton");

	j_help_button = glade_xml_get_widget (data->gui, "j_help_button");
	j_cancel_button = glade_xml_get_widget (data->gui, "j_cancel_button");
	j_ok_button = glade_xml_get_widget (data->gui, "j_ok_button");

	/* Set widgets data. */

	reset_image = glade_xml_get_widget (data->gui, "j_reset_image");
	gtk_image_set_from_stock (GTK_IMAGE (reset_image), GTHUMB_STOCK_RESET, GTK_ICON_SIZE_MENU);

	add_image_to_button (j_rot_90_button, rotate_90_24_rgba);
	add_image_to_button (j_rot_270_button, rotate_270_24_rgba);
	add_image_to_button (j_v_flip_button, mirror_24_rgba);
	add_image_to_button (j_h_flip_button, flip_24_rgba);

	gtk_widget_set_sensitive (data->j_apply_to_all_checkbutton,
				  data->file_list->next != NULL);

#ifndef HAVE_LIBEXIF
	gtk_widget_set_sensitive (data->j_from_exif_checkbutton, FALSE);
#endif /* ! HAVE_LIBEXIF */

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
	g_signal_connect (G_OBJECT (data->j_from_exif_checkbutton),
			  "toggled",
			  G_CALLBACK (from_exif_toggled_cb),
			  data);

	data->loader = (ImageLoader*)image_loader_new (NULL, FALSE);

	g_signal_connect (G_OBJECT (data->loader),
			  "image_done",
			  G_CALLBACK (image_loader_done_cb),
			  data);
	g_signal_connect (G_OBJECT (data->loader),
			  "image_error",
			  G_CALLBACK (image_loader_error_cb),
			  data);

	/* Run dialog. */

	all_windows_remove_monitor ();

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), GTK_WINDOW (window));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), TRUE); 
	gtk_widget_show_all (data->dialog);

	load_current_image (data);
}


void
dlg_apply_jpegtran (GthWindow    *window,
		    GthTransform  rot_type,
		    GthTransform  tran_type)
{
	DialogData  *data;
	GList       *list;

	list = gth_window_get_file_list_selection (window);
	if (list == NULL) {
		g_warning ("No file selected.");
		return;
	}

	all_windows_remove_monitor ();

	data = g_new0 (DialogData, 1);
	data->window = window;
	data->file_list = list;
	data->current_image = list;

	data->rot_data = rotation_data_new();
	data->rot_data->rot_type = rot_type;
	data->rot_data->tran_type = tran_type;

	apply_transformation_to_all (data);
}



void
dlg_apply_jpegtran_from_exif (GthWindow *window)
{
	DialogData  *data;
	GList       *list;

	list = gth_window_get_file_list_selection (window);
	if (list == NULL) {
		g_warning ("No file selected.");
		return;
	}

	all_windows_remove_monitor ();

	data = g_new0 (DialogData, 1);
	data->window = window;
	data->file_list = list;
	data->current_image = list;
	data->from_exif_data = TRUE;
	data->rot_data = rotation_data_new();

	apply_transformation_to_all (data);
}
