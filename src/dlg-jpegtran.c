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

#include <libexif/exif-data.h>
#include <libexif/exif-content.h>
#include <libexif/exif-entry.h>
#include <libexif/exif-utils.h>
#include "jpegutils/jpeg-data.h"

#include "file-utils.h"
#include "gconf-utils.h"
#include "gth-utils.h"
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
	GtkWidget    *j_reset_exif_tag_on_rotate_checkbutton;

	GList        *file_list;
	GList        *files_changed_list;
	GList        *current_image;
	ImageLoader  *loader;
	GdkPixbuf    *original_preview;
	GthTransform	transform;
} DialogData;


static void
dialog_data_free (DialogData *data)
{
	if (data->files_changed_list != NULL) {
		all_windows_notify_files_changed (data->files_changed_list);
		path_list_free (data->files_changed_list);
		data->files_changed_list = NULL;
	}

	g_idle_add ((GSourceFunc)all_windows_add_monitor, NULL);

	path_list_free (data->file_list);
	if (data->loader != NULL)
		g_object_unref (data->loader);
	if (data->gui != NULL)
		g_object_unref (data->gui);
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

	data->transform = GTH_TRANSFORM_NONE;
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
	char		 *local_file_to_modify = NULL;
	GnomeVFSFileInfo *info;
	gboolean          jpeg;
	gboolean	  is_local;
	gboolean	  remote_copy_ok;
	ExifShort         orientation;
	GtkWindow  	 *window = GTK_WINDOW (data->dialog);
	GthTransform	  required_transform;


	is_local = is_local_file (path);

	/* If the original file is stored on a remote VFS location, copy it to a local
	   temp file, modify it, then copy it back. This is easier than modifying the
	   underlying jpeg code (and other code) to handle VFS URIs. */

	if (is_local)
		local_file_to_modify = g_strdup (remove_scheme_from_uri (path));
	else 
		local_file_to_modify = make_cache_copy_of_remote_file (path);

	if (local_file_to_modify == NULL) {
		_gtk_error_dialog_run (GTK_WINDOW (window), 
			_("Could not create a local temporary copy of the remote file."));
		return;
	}

	info = gnome_vfs_file_info_new ();
	gnome_vfs_get_file_info (path, info, GNOME_VFS_FILE_INFO_GET_ACCESS_RIGHTS|GNOME_VFS_FILE_INFO_FOLLOW_LINKS);

	jpeg = image_is_jpeg (local_file_to_modify);
	orientation = get_exif_tag_short(local_file_to_modify, EXIF_TAG_ORIENTATION);
	required_transform = get_next_transformation (orientation, data->transform);

	if ((jpeg && orientation) &&
	    !eel_gconf_get_boolean (PREF_ROTATE_RESET_EXIF_ORIENTATION, TRUE)) {
		// Adjust Exif orientation tag.
		write_orientation_field (local_file_to_modify, required_transform);
	} else if (jpeg) {
 		// Lossless jpeg transform.
		apply_transformation_jpeg (window, local_file_to_modify, required_transform);
	} else {
		// Generic image transform.
		apply_transformation_generic (window, local_file_to_modify, required_transform);
	}

	if (!is_local) {
		remote_copy_ok = make_remote_copy_of_local_file (local_file_to_modify, path);
		remove_temp_file_and_dir (local_file_to_modify);
	}

	if (!is_local && !remote_copy_ok) {
		_gtk_error_dialog_run (GTK_WINDOW (window), 
				_("Could not move temporary file to remote location. Check remote permissions."));
	} else {
		gnome_vfs_set_file_info (path, info, GNOME_VFS_SET_FILE_INFO_PERMISSIONS|GNOME_VFS_SET_FILE_INFO_OWNER);
                notify_file_changed (data, path, notify_soon);
	}

	gnome_vfs_file_info_unref (info);
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

	if (to_all) {
		gtk_widget_hide (data->dialog);
		apply_transformation_to_all (data);
	} else {
		apply_transformation (data, data->current_image, TRUE);
		load_next_image (data);
	}
}


static void
revert_clicked (GtkWidget  *button,
		DialogData *data)
{
	data->transform = GTH_TRANSFORM_NONE;

	if (data->original_preview != NULL)
		gtk_image_set_from_pixbuf (GTK_IMAGE (data->j_preview_image), data->original_preview);
}


static void
transform_clicked_impl (GtkWidget  *button,
	       DialogData *data,
	       GthTransform transform)
{
	GdkPixbuf *src_pixbuf = NULL;
	GdkPixbuf *dest_pixbuf = NULL;

	data->transform = get_next_transformation (data->transform, transform);

	src_pixbuf = gtk_image_get_pixbuf (GTK_IMAGE (data->j_preview_image));

	if (src_pixbuf == NULL)
		return;

	dest_pixbuf = _gdk_pixbuf_transform (src_pixbuf, transform);
	gtk_image_set_from_pixbuf (GTK_IMAGE (data->j_preview_image), dest_pixbuf);

	if (dest_pixbuf != NULL)
		g_object_unref (dest_pixbuf);
}


static void
rot90_clicked (GtkWidget  *button,
	       DialogData *data)
{
	transform_clicked_impl (button, data, GTH_TRANSFORM_ROTATE_90);
}


static void
rot270_clicked (GtkWidget  *button,
		DialogData *data)
{
	transform_clicked_impl (button, data, GTH_TRANSFORM_ROTATE_270);
}


static void
mirror_clicked (GtkWidget  *button,
		DialogData *data)
{
	transform_clicked_impl (button, data, GTH_TRANSFORM_FLIP_H);
}


static void
flip_clicked (GtkWidget  *button,
	      DialogData *data)
{
	transform_clicked_impl (button, data, GTH_TRANSFORM_FLIP_V);
}


static void
reset_exif_tag_on_rotate_clicked (GtkWidget  *button,
		              DialogData *data)
{
	eel_gconf_set_boolean (PREF_ROTATE_RESET_EXIF_ORIENTATION,
		gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->j_reset_exif_tag_on_rotate_checkbutton)));
}

/* called when the "help" button is clicked. */
static void
help_cb (GtkWidget  *widget,
	 DialogData *data)
{
	gthumb_display_help (GTK_WINDOW (data->dialog), "gthumb-rotate-jpeg");
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

	data->transform = GTH_TRANSFORM_NONE;

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

	data->j_reset_exif_tag_on_rotate_checkbutton = glade_xml_get_widget (data->gui, "j_reset_exif_tag_on_rotate_checkbutton");

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

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->j_reset_exif_tag_on_rotate_checkbutton), eel_gconf_get_boolean (PREF_ROTATE_RESET_EXIF_ORIENTATION, TRUE));

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
        g_signal_connect (G_OBJECT (data->j_reset_exif_tag_on_rotate_checkbutton),
 			  "clicked",
                          G_CALLBACK (reset_exif_tag_on_rotate_clicked),
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
		    GthTransform  transform)
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
	data->transform = transform;

	apply_transformation_to_all (data);
}
