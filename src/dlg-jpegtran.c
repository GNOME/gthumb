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
#include <sys/types.h>
#include <sys/stat.h>
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
#include <exif-utils.h>
#include "jpegutils/jpeg-data.h"
#endif /* HAVE_LIBEXIF */

#include "file-data.h"
#include "gconf-utils.h"
#include "gthumb-window.h"
#include "gtk-utils.h"
#include "image-loader.h"
#include "main.h"
#include "icons/pixbufs.h"
#include "jpegutils/jpegtran.h"
#include "pixbuf-utils.h"
#include "gthumb-stock.h"
#include "gth-exif-utils.h"


#define ROTATE_GLADE_FILE "gthumb_tools.glade"
#define PROGRESS_GLADE_FILE "gthumb_png_exporter.glade"

#define PREVIEW_SIZE 200

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
	GtkWidget    *j_button_vbox;
	GtkWidget    *j_revert_button;;
	GtkWidget    *j_apply_to_all_checkbutton;
	GtkWidget    *j_preview_image;
	GtkWidget    *j_from_exif_checkbutton;

	int           rot_type;    /* 0, 90 ,180, 270 */
	int           tran_type;   /* mirror, flip */
	GList        *file_list;
	GList        *files_changed_list;
	GList        *current_image;
	ImageLoader  *loader;
	GdkPixbuf    *original_preview;
} DialogData;


/* called when the main dialog is closed. */
static void
destroy_cb (GtkWidget  *widget, 
	    DialogData *data)
{
	if (data->files_changed_list != NULL) {
		all_windows_notify_files_changed (data->files_changed_list);
		path_list_free (data->files_changed_list);
	}

	all_windows_add_monitor ();

	file_data_list_free (data->file_list);
	g_object_unref (data->loader);
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
update_rotation_from_exif_data (DialogData *data,
				GList      *current_image)
{
#ifdef HAVE_LIBEXIF
	FileData           *fd = current_image->data;
	GthExifOrientation  orientation = get_exif_tag_short (fd->path, EXIF_TAG_ORIENTATION);

	switch (orientation) {
	case GTH_EXIF_ORIENTATION_TOP_LEFT:
		data->rot_type = TRAN_ROTATE_0;
		data->tran_type = TRAN_NONE;
		break;
	case GTH_EXIF_ORIENTATION_TOP_RIGHT:
		data->rot_type = TRAN_ROTATE_0;
		data->tran_type = TRAN_MIRROR;
		break;
	case GTH_EXIF_ORIENTATION_BOTTOM_RIGHT:
		data->rot_type = TRAN_ROTATE_180;
		data->tran_type = TRAN_NONE;
		break;
	case GTH_EXIF_ORIENTATION_BOTTOM_LEFT:
		data->rot_type = TRAN_ROTATE_180;
		data->tran_type = TRAN_MIRROR;
		break;
	case GTH_EXIF_ORIENTATION_LEFT_TOP:
		data->rot_type = TRAN_ROTATE_90;
		data->tran_type = TRAN_MIRROR;
		break;
	case GTH_EXIF_ORIENTATION_RIGHT_TOP:
		data->rot_type = TRAN_ROTATE_90;
		data->tran_type = TRAN_NONE;
		break;
	case GTH_EXIF_ORIENTATION_RIGHT_BOTTOM:
		data->rot_type = TRAN_ROTATE_90;
		data->tran_type = TRAN_FLIP;
		break;
	case GTH_EXIF_ORIENTATION_LEFT_BOTTOM:
		data->rot_type = TRAN_ROTATE_270;
		data->tran_type = TRAN_NONE;
		break;
	default:
		data->rot_type = TRAN_ROTATE_0;
		data->tran_type = TRAN_NONE;
		break;
	}
#endif /* HAVE_LIBEXIF */
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
	
	update_rotation_from_exif_data (data, data->current_image);

	src_pixbuf = data->original_preview;
	
	if (src_pixbuf == NULL) 
		return;

	switch (data->rot_type) {
	case TRAN_ROTATE_90:
		dest_pixbuf = _gdk_pixbuf_copy_rotate_90 (src_pixbuf, FALSE);
		break;
	case TRAN_ROTATE_180:
		dest_pixbuf = _gdk_pixbuf_copy_mirror (src_pixbuf, TRUE, TRUE);
		break;
	case TRAN_ROTATE_270:
		dest_pixbuf = _gdk_pixbuf_copy_rotate_90 (src_pixbuf, TRUE);
		break;
	default:
		dest_pixbuf = src_pixbuf;
		g_object_ref (dest_pixbuf);
			break;
	}

	src_pixbuf = dest_pixbuf;

	switch (data->tran_type) {
	case TRAN_MIRROR:
		dest_pixbuf = _gdk_pixbuf_copy_mirror (src_pixbuf, TRUE, FALSE);
		break;
	case TRAN_FLIP:
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
	FileData *fd;

	if (data->current_image == NULL) {
		gtk_widget_destroy (data->dialog);
		return;
	}
	
	gtk_widget_set_sensitive (data->j_button_vbox, FALSE);
	gtk_widget_set_sensitive (data->j_revert_button, FALSE);

	fd = data->current_image->data;
	image_loader_set_path (data->loader, fd->path);
	image_loader_start (data->loader);

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


static ExifShort
get_next_value_rotation_90 (int value)
{
	static ExifShort new_value [8] = {8, 7, 6, 5, 2, 1, 4, 3};
	return new_value[value - 1];
}


static ExifShort
get_next_value_mirror (int value)
{
	static ExifShort new_value [8] = {2, 1, 4, 3, 8, 7, 6, 5};
	return new_value[value - 1];
}


static ExifShort
get_next_value_flip (int value)
{
	static ExifShort new_value [8] = {4, 3, 2, 1, 6, 5, 8, 7};
	return new_value[value - 1];
}


static void
swap_xy_exif_fields (const char *filename,
		     DialogData *data)
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


static void
update_orientation_field (const char *filename,
			  DialogData *data)
{
	JPEGData     *jdata;
	ExifData     *edata;
	unsigned int  i;
	gboolean      orientation_changed = FALSE;

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
		ExifEntry   *entry;

		if ((content == NULL) || (content->count == 0)) 
			continue;

		entry = exif_content_get_entry (content, EXIF_TAG_ORIENTATION);
		if (!orientation_changed && (entry != NULL)) {
			ExifByteOrder byte_order;
			ExifShort     value;

			byte_order = exif_data_get_byte_order (edata);
			value = exif_get_short (entry->data, byte_order);

			switch (data->rot_type) {
			case TRAN_ROTATE_90:
				value = get_next_value_rotation_90 (value);
				break;
			case TRAN_ROTATE_180:
				value = get_next_value_rotation_90 (value);
				value = get_next_value_rotation_90 (value);
				break;
			case TRAN_ROTATE_270:
				value = get_next_value_rotation_90 (value);
				value = get_next_value_rotation_90 (value);
				value = get_next_value_rotation_90 (value);
				break;
			default:
				break;
			}
			
			switch (data->tran_type) {
			case TRAN_MIRROR:
				value = get_next_value_mirror (value);
				break;
			case TRAN_FLIP:
				value = get_next_value_flip (value);
				break;
			default:
				break;
			}

			exif_set_short (entry->data, byte_order, value);

			orientation_changed = TRUE;
		}
	}

	jpeg_data_save_file (jdata, filename);

	exif_data_unref (edata);
	jpeg_data_unref (jdata);
}


#endif


static void
apply_tranformation_jpeg (DialogData *data,
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
#ifdef HAVE_LIBEXIF
		if ((rot_type == TRAN_ROTATE_90) || (rot_type == TRAN_ROTATE_270))
			swap_xy_exif_fields (fd->path, data);
		update_orientation_field (fd->path, data);
#endif

		data->files_changed_list = g_list_prepend (data->files_changed_list, 
							   g_strdup (fd->path));
	}

	g_free (e1);
	g_free (e2);
	g_free (line);
	g_free (tmp1);
	g_free (tmp2);
}


static void
apply_transformation_generic (DialogData *data,
			      GList      *current_image)
{
	FileData   *fd = current_image->data;
	int         rot_type = data->rot_type; 
	int         tran_type = data->tran_type; 
	GdkPixbuf  *pixbuf1, *pixbuf2;
	const char *mime_type;

	if ((rot_type == TRAN_ROTATE_0) && (tran_type == TRAN_NONE))
		return;

	pixbuf1 = gdk_pixbuf_new_from_file (fd->path, NULL);
	if (pixbuf1 == NULL)
		return;

	switch (rot_type) {
	case TRAN_ROTATE_90:
		pixbuf2 = _gdk_pixbuf_copy_rotate_90 (pixbuf1, FALSE);
		break;
	case TRAN_ROTATE_180:
		pixbuf2 = _gdk_pixbuf_copy_mirror (pixbuf1, TRUE, TRUE);
		break;
	case TRAN_ROTATE_270:
		pixbuf2 = _gdk_pixbuf_copy_rotate_90 (pixbuf1, TRUE);
		break;
	default:
		pixbuf2 = pixbuf1;
		g_object_ref (pixbuf2);
		break;
	}
	g_object_unref (pixbuf1);

	switch (tran_type) {
	case TRAN_MIRROR:
		pixbuf1 = _gdk_pixbuf_copy_mirror (pixbuf2, TRUE, FALSE);
		break;
	case TRAN_FLIP:
		pixbuf1 = _gdk_pixbuf_copy_mirror (pixbuf2, FALSE, TRUE);
		break;
	default:
		pixbuf1 = pixbuf2;
		g_object_ref (pixbuf1);
		break;
	}
	g_object_unref (pixbuf2);

	mime_type = gnome_vfs_mime_type_from_name (fd->path);
	if ((mime_type != NULL) && is_mime_type_writable (mime_type)) {
		GError      *error = NULL;
		const char  *image_type = mime_type + 6;
		if (! _gdk_pixbuf_save (pixbuf1, 
					fd->path, 
					image_type, 
					&error, 
					NULL))
			_gtk_error_dialog_from_gerror_run (GTK_WINDOW (data->window->app), &error);
	}
	g_object_unref (pixbuf1);

	data->files_changed_list = g_list_prepend (data->files_changed_list, 
						   g_strdup (fd->path));
}


static void
apply_transformation (DialogData *data,
		      GList      *current_image)
{
	FileData    *fd = current_image->data;
	char        *dir;
	struct stat  buf;

	/* Check directory permissions. */

	dir = remove_level_from_path (fd->path);
	if (access (dir, R_OK | W_OK | X_OK) != 0) {
		char *utf8_path;
		utf8_path = g_filename_to_utf8 (dir, -1, NULL, NULL, NULL);
		_gtk_error_dialog_run (GTK_WINDOW (data->dialog),
				       _("You don't have the right permissions to create images in the folder \"%s\""),
				       utf8_path);
		g_free (utf8_path);
		g_free (dir);

		return;
	} 
	g_free (dir);

	/**/

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->j_from_exif_checkbutton)))
		update_rotation_from_exif_data (data, current_image);

	stat (fd->path, &buf);

	if (image_is_jpeg (fd->path)) 
		apply_tranformation_jpeg (data, current_image);
	else 
		apply_transformation_generic (data, current_image);

	chmod (fd->path, buf.st_mode);
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

			_gtk_label_set_filename_text (GTK_LABEL (label), fd->name);
			gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (bar),
						       (gdouble) (i + 1) / n);

			while (gtk_events_pending())
				gtk_main_iteration();

			apply_transformation (data, scan);

			i++;
		}

		gtk_widget_destroy (dialog);
		g_object_unref (gui);

		gtk_widget_destroy (data->dialog);

	} else {
		apply_transformation (data, data->current_image);
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
	data->rot_type = TRAN_ROTATE_0;
	data->tran_type = TRAN_NONE;

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

	if (data->tran_type == TRAN_NONE)
		data->rot_type = get_next_rot (data->rot_type);
	else {
		data->rot_type = get_next_rot (data->rot_type);
		data->rot_type = get_next_rot (data->rot_type);
		data->rot_type = get_next_rot (data->rot_type);
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

	if (data->tran_type == TRAN_NONE) {
		data->rot_type = get_next_rot (data->rot_type);
		data->rot_type = get_next_rot (data->rot_type);
		data->rot_type = get_next_rot (data->rot_type);
	} else
		data->rot_type = get_next_rot (data->rot_type);

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

	if (data->tran_type == TRAN_FLIP) {
		data->tran_type = TRAN_NONE;
		data->rot_type = get_next_rot (data->rot_type);
		data->rot_type = get_next_rot (data->rot_type);

	} else if (data->tran_type == TRAN_MIRROR)
		data->tran_type = TRAN_NONE;

	else
		data->tran_type = TRAN_MIRROR;
	
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

	if (data->tran_type == TRAN_MIRROR) {
		data->tran_type = TRAN_NONE;
		data->rot_type = get_next_rot (data->rot_type);
		data->rot_type = get_next_rot (data->rot_type);

	} else if (data->tran_type == TRAN_FLIP)
		data->tran_type = TRAN_NONE;

	else
		data->tran_type = TRAN_FLIP;

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
	GtkWidget   *reset_image;
	GList       *list;

	list = gth_file_list_get_selection_as_fd (window->file_list);
	if (list == NULL) {
		g_warning ("No file selected.");
		return;
	}

	/**/

	data = g_new0 (DialogData, 1);

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

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), GTK_WINDOW (window->app));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), TRUE); 
	gtk_widget_show_all (data->dialog);

	load_current_image (data);
}
