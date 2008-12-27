/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2005 Free Software Foundation, Inc.
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
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <glib/gi18n.h>
#include <gconf/gconf-client.h>
#include <libgnome/gnome-exec.h>
#include <libgnome/gnome-help.h>
#include <libgnome/gnome-url.h>
#include <libgnomevfs/gnome-vfs-ops.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "async-pixbuf-ops.h"
#include "catalog.h"
#include "comments.h"
#include "dlg-bookmarks.h"
#include "dlg-brightness-contrast.h"
#include "dlg-hue-saturation.h"
#include "dlg-catalog.h"
#include "dlg-categories.h"
#include "dlg-change-date.h"
#include "dlg-reset-exif.h"
#include "dlg-comment.h"
#include "dlg-convert.h"
#include "dlg-file-utils.h"
#include "dlg-open-with.h"
#include "dlg-posterize.h"
#include "dlg-color-balance.h"
#include "dlg-preferences.h"
#include "dlg-redeye-removal.h"
#include "dlg-rename-series.h"
#include "dlg-scale-image.h"
#include "dlg-crop.h"
#include "dlg-write-to-cd.h"
#include "dlg-image-prop.h"
#include "gconf-utils.h"
#include "gth-pixbuf-op.h"
#include "gth-viewer.h"
#include "gth-utils.h"
#include "gthumb-error.h"
#include "gthumb-module.h"
#include "gtk-utils.h"
#include "gth-file-view.h"
#include "image-viewer.h"
#include "main.h"
#include "pixbuf-utils.h"
#include "print-callbacks.h"
#include "thumb-cache.h"
#include "typedefs.h"
#include "gth-folder-selection-dialog.h"
#include "file-utils.h"

#define MAX_NAME_LEN 1024
#define DEF_CONFIRM_DEL TRUE

typedef enum {
	WALLPAPER_ALIGN_TILED     = 0,
	WALLPAPER_ALIGN_CENTERED  = 1,
	WALLPAPER_ALIGN_STRETCHED = 2,
	WALLPAPER_ALIGN_SCALED    = 3,
	WALLPAPER_NONE            = 4
} WallpaperAlign;


void
gth_window_activate_action_file_close_window (GtkAction *action,
					      gpointer   data)
{
	gth_window_close ((GthWindow*)data);
}


void
gth_window_activate_action_file_open_with (GtkAction  *action,
					   GthWindow  *window)
{
	GList *list;

	list = gth_window_get_file_list_selection (window);
	g_return_if_fail (list != NULL);

	dlg_open_with (GTK_WINDOW (window), list);

	/* the list is deallocated when the dialog is closed. */
}


void
gth_window_activate_action_image_open_with (GtkAction  *action,
					    GthWindow  *window)
{
	const char *image_filename;
	GList      *list;

	image_filename = gth_window_get_image_filename (window);
	if (image_filename == NULL)
		return;

	list = g_list_append (NULL, g_strdup (image_filename));
	g_return_if_fail (list != NULL);

	dlg_open_with (GTK_WINDOW (window), list);

	/* the list is deallocated when the dialog is closed. */
}


void
gth_window_activate_action_file_save (GtkAction *action,
				      gpointer   data)
{
	GthWindow   *window = GTH_WINDOW (data);
	ImageViewer *image_viewer = gth_window_get_image_viewer (window);

	gth_window_save_pixbuf (window,
				image_viewer_get_current_pixbuf (image_viewer),
				gth_window_get_image_data (window));
}


void
gth_window_activate_action_file_save_as (GtkAction *action,
					 gpointer   data)
{
	GthWindow   *window = GTH_WINDOW (data);
	ImageViewer *image_viewer = gth_window_get_image_viewer (window);

	gth_window_save_pixbuf (window, image_viewer_get_current_pixbuf (image_viewer), NULL);
}


void
gth_window_activate_action_file_revert (GtkAction *action,
					GthWindow *window)
{
	gth_window_reload_current_image (window);
}


static void
print_done_cb (gpointer data)
{
	char *tmp_filename = data;
	char *tmp_comment;
	char *tmp_dir;

	if (tmp_filename == NULL)
		return;

	tmp_comment = comments_get_comment_filename (tmp_filename, TRUE);
	tmp_dir = remove_level_from_path (tmp_comment);
	local_dir_remove_recursive (tmp_dir);
	g_free (tmp_comment);
	g_free (tmp_dir);

	tmp_dir = remove_level_from_path (tmp_filename);
	local_dir_remove_recursive (tmp_dir);	
	g_free (tmp_dir);

	g_free (tmp_filename);
}


void
gth_window_activate_action_file_print (GtkAction *action,
				       GthWindow *window)
{
	GList *list;

	list = gth_window_get_file_list_selection_as_fd (window);
	if (list == NULL)
		return;

	if (gth_window_get_image_modified (window)) {
		char        *tmp_dir = NULL;
		char        *tmp_filename = NULL;
		ImageViewer *image_viewer;
		GdkPixbuf   *pixbuf;
		GError      *error = NULL;
		const char  *image_filename;
		GList       *current;

		image_viewer = gth_window_get_image_viewer (window);
		pixbuf = image_viewer_get_current_pixbuf (image_viewer);
		if (pixbuf == NULL) 
			return;
			
		g_object_ref (pixbuf);
		tmp_dir = get_temp_dir_name ();
		if (tmp_dir == NULL) {
			_gtk_error_dialog_run (GTK_WINDOW (window), _("Could not create a temporary folder"));

			g_object_unref (pixbuf);
			file_data_list_free (list);

			return;
		}
			
		tmp_filename = get_temp_file_name (tmp_dir, "jpeg");
		if (! _gdk_pixbuf_save (pixbuf,
					tmp_filename,
					NULL,
					"jpeg",
					&error,
					NULL)) 
		{
			_gtk_error_dialog_from_gerror_run (GTK_WINDOW (window), &error);
			local_dir_remove_recursive (tmp_dir);

			g_object_unref (pixbuf);				
			g_free (tmp_filename);
			g_free (tmp_dir);
			file_data_list_free (list);
				
			return;
		}

		g_object_unref (pixbuf);

		image_filename = gth_window_get_image_filename (window);
		current = file_data_list_find_path (list, image_filename);
		if (current != NULL) {
			FileData *fd;
					
			fd = file_data_dup (current->data);
			g_free (fd->path);
			fd->path =  get_uri_from_local_path (tmp_filename);
			comment_copy (image_filename, fd->path);
		}

		print_catalog_dlg_full (GTK_WINDOW (window), list, print_done_cb, get_uri_from_local_path (tmp_filename));

		g_free (tmp_filename);		
		g_free (tmp_dir);
	}
	else
		print_catalog_dlg (GTK_WINDOW (window), list);

	file_data_list_free (list);
}


void
gth_window_activate_action_edit_edit_comment (GtkAction *action,
					      GthWindow *window)
{
	gth_window_edit_comment (window);
}


void
gth_window_activate_action_edit_edit_categories (GtkAction *action,
						 GthWindow *window)
{
	gth_window_edit_categories (window);
}


void
gth_window_activate_action_edit_undo (GtkAction *action,
				      GthWindow *window)
{
	gth_window_undo (window);
}


void
gth_window_activate_action_edit_redo (GtkAction *action,
				      GthWindow *window)
{
	gth_window_redo (window);
}


void
gth_window_activate_action_alter_image_rotate90 (GtkAction *action,
						 GthWindow *window)
{
	GdkPixbuf *src_pixbuf;
	GdkPixbuf *dest_pixbuf;

	src_pixbuf = gth_window_get_image_pixbuf (window);

        if (src_pixbuf == NULL)
                return;

	dest_pixbuf = gdk_pixbuf_rotate_simple (src_pixbuf, GDK_PIXBUF_ROTATE_CLOCKWISE);
	gth_window_set_image_pixbuf (window, dest_pixbuf);
	g_object_unref (dest_pixbuf);
}


void
gth_window_activate_action_alter_image_rotate90cc (GtkAction *action,
						   GthWindow *window)
{
	GdkPixbuf *src_pixbuf;
	GdkPixbuf *dest_pixbuf;

	src_pixbuf = gth_window_get_image_pixbuf (window);

        if (src_pixbuf == NULL)
                return;

	dest_pixbuf = gdk_pixbuf_rotate_simple (src_pixbuf, GDK_PIXBUF_ROTATE_COUNTERCLOCKWISE);
	gth_window_set_image_pixbuf (window, dest_pixbuf);
	g_object_unref (dest_pixbuf);
}


void
gth_window_activate_action_alter_image_flip (GtkAction *action,
					     GthWindow *window)
{
	GdkPixbuf *src_pixbuf;
	GdkPixbuf *dest_pixbuf;

	src_pixbuf = gth_window_get_image_pixbuf (window);

        if (src_pixbuf == NULL)
                return;

	dest_pixbuf = gdk_pixbuf_flip (src_pixbuf, FALSE);
	gth_window_set_image_pixbuf (window, dest_pixbuf);
	g_object_unref (dest_pixbuf);
}


void
gth_window_activate_action_alter_image_mirror (GtkAction *action,
					       GthWindow *window)
{
	GdkPixbuf *src_pixbuf;
	GdkPixbuf *dest_pixbuf;

	src_pixbuf = gth_window_get_image_pixbuf (window);

        if (src_pixbuf == NULL)
                return;

	dest_pixbuf = gdk_pixbuf_flip (src_pixbuf, TRUE);
	gth_window_set_image_pixbuf (window, dest_pixbuf);
	g_object_unref (dest_pixbuf);
}


void
gth_window_activate_action_alter_image_desaturate (GtkAction *action,
						   GthWindow *window)
{
	GdkPixbuf   *src_pixbuf;
	GdkPixbuf   *dest_pixbuf;
	GthPixbufOp *pixop;

	src_pixbuf = gth_window_get_image_pixbuf (window);

        if (src_pixbuf == NULL)
                return;

	dest_pixbuf = gdk_pixbuf_copy (src_pixbuf);
	pixop = _gdk_pixbuf_desaturate (dest_pixbuf, dest_pixbuf);
	g_object_unref (dest_pixbuf);
	gth_window_exec_pixbuf_op (window, pixop, FALSE);
	g_object_unref (pixop);
}


void
gth_window_activate_action_alter_image_invert (GtkAction *action,
					       GthWindow *window)
{
	GdkPixbuf   *src_pixbuf;
	GdkPixbuf   *dest_pixbuf;
	GthPixbufOp *pixop;

	src_pixbuf = gth_window_get_image_pixbuf (window);

        if (src_pixbuf == NULL)
                return;

	dest_pixbuf = gdk_pixbuf_copy (src_pixbuf);
	pixop = _gdk_pixbuf_invert (dest_pixbuf, dest_pixbuf);
	g_object_unref (dest_pixbuf);
	gth_window_exec_pixbuf_op (window, pixop, FALSE);
	g_object_unref (pixop);
}


void
gth_window_activate_action_alter_image_adjust_levels (GtkAction *action,
						      GthWindow *window)
{
	GdkPixbuf   *src_pixbuf;
	GdkPixbuf   *dest_pixbuf;
	GthPixbufOp *pixop;

	src_pixbuf = gth_window_get_image_pixbuf (window);

        if (src_pixbuf == NULL)
                return;

	dest_pixbuf = gdk_pixbuf_copy (src_pixbuf);
	pixop = _gdk_pixbuf_adjust_levels (dest_pixbuf, dest_pixbuf);
	g_object_unref (dest_pixbuf);
	gth_window_exec_pixbuf_op (window, pixop, FALSE);
	g_object_unref (pixop);
}


void
gth_window_activate_action_alter_image_equalize (GtkAction *action,
						 GthWindow *window)
{
	GdkPixbuf   *src_pixbuf;
	GdkPixbuf   *dest_pixbuf;
	GthPixbufOp *pixop;

	src_pixbuf = gth_window_get_image_pixbuf (window);

        if (src_pixbuf == NULL)
                return;

	dest_pixbuf = gdk_pixbuf_copy (src_pixbuf);
	pixop = _gdk_pixbuf_eq_histogram (dest_pixbuf, dest_pixbuf);
	g_object_unref (dest_pixbuf);
	gth_window_exec_pixbuf_op (window, pixop, FALSE);
	g_object_unref (pixop);
}


void
gth_window_activate_action_alter_image_posterize (GtkAction *action,
						  GthWindow *window)
{
	dlg_posterize (window);
}


void
gth_window_activate_action_alter_image_brightness_contrast (GtkAction *action,
							    GthWindow *window)
{
	dlg_brightness_contrast (window);
}


void
gth_window_activate_action_alter_image_hue_saturation (GtkAction *action,
						       GthWindow *window)
{
	dlg_hue_saturation (window);
}


void
gth_window_activate_action_alter_image_redeye_removal (GtkAction *action,
						   GthWindow *window)
{
	dlg_redeye_removal (window);
}

void
gth_window_activate_action_alter_image_color_balance (GtkAction *action,
						      GthWindow *window)
{
	dlg_color_balance (window);
}


void
gth_window_activate_action_alter_image_resize (GtkAction *action,
					       GthWindow *window)
{
	dlg_scale_image (window);
}


void
gth_window_activate_action_alter_image_crop (GtkAction *action,
					     GthWindow *window)
{
	dlg_crop (window);
}


void
gth_window_activate_action_alter_image_dither_bw (GtkAction *action,
						  GthWindow *window)
{
	GdkPixbuf   *src_pixbuf;
	GdkPixbuf   *dest_pixbuf;
	GthPixbufOp *pixop;

	src_pixbuf = gth_window_get_image_pixbuf (window);

        if (src_pixbuf == NULL)
                return;

	dest_pixbuf = gdk_pixbuf_copy (src_pixbuf);
	pixop = _gdk_pixbuf_dither (dest_pixbuf, dest_pixbuf, GTH_DITHER_BLACK_WHITE);
	g_object_unref (dest_pixbuf);
	gth_window_exec_pixbuf_op (window, pixop, FALSE);
	g_object_unref (pixop);
}


void
gth_window_activate_action_alter_image_dither_web (GtkAction *action,
						   GthWindow *window)
{
	GdkPixbuf   *src_pixbuf;
	GdkPixbuf   *dest_pixbuf;
	GthPixbufOp *pixop;

	src_pixbuf = gth_window_get_image_pixbuf (window);

        if (src_pixbuf == NULL)
                return;

	dest_pixbuf = gdk_pixbuf_copy (src_pixbuf);
	pixop = _gdk_pixbuf_dither (dest_pixbuf, dest_pixbuf, GTH_DITHER_WEB_PALETTE);
	g_object_unref (dest_pixbuf);
	gth_window_exec_pixbuf_op (window, pixop, FALSE);
	g_object_unref (pixop);
}


void
gth_window_activate_action_view_zoom_in (GtkAction *action,
					 gpointer   data)
{
	GthWindow *window = GTH_WINDOW (data);
	image_viewer_zoom_in (gth_window_get_image_viewer (window));
}


void
gth_window_activate_action_view_zoom_to_width (GtkAction *action,
                                               gpointer   data)
{
        GthWindow *window = GTH_WINDOW (data);
	image_viewer_set_fit_mode (gth_window_get_image_viewer (window), GTH_FIT_WIDTH_IF_LARGER);
}


void
gth_window_activate_action_view_zoom_out (GtkAction *action,
					  gpointer   data)
{
	GthWindow *window = GTH_WINDOW (data);
	image_viewer_zoom_out (gth_window_get_image_viewer (window));
}


void
gth_window_activate_action_view_zoom_100 (GtkAction *action,
					  gpointer   data)
{
	GthWindow *window = GTH_WINDOW (data);
	image_viewer_set_zoom (gth_window_get_image_viewer (window), 1.0);
}


void
gth_window_activate_action_view_zoom_fit_if_larger (GtkAction *action,
					            gpointer   data)
{
	GthWindow   *window = GTH_WINDOW (data);
	image_viewer_set_fit_mode (gth_window_get_image_viewer (window), GTH_FIT_SIZE_IF_LARGER);
}


void
gth_window_activate_action_view_zoom_fit (GtkAction *action,
					  gpointer   data)
{
	GthWindow   *window = GTH_WINDOW (data);
	image_viewer_set_fit_mode (gth_window_get_image_viewer (window), GTH_FIT_SIZE);
}


void
gth_window_activate_action_view_fullscreen (GtkAction *action,
					    GthWindow *window)
{
	gth_window_set_fullscreen (window, ! gth_window_get_fullscreen (window));
}


void
gth_window_activate_action_view_exit_fullscreen (GtkAction *action,
						 GthWindow *window)
{
	gth_window_set_fullscreen (window, FALSE);
}


typedef struct {
	GthWindow      *window;
	WallpaperAlign  align;
} SetWallpaperData;


static void 
set_wallpaper_step_2 (const char     *uri, 
		      GError         *error, 
		      gpointer        callback_data)
{
	SetWallpaperData *data = callback_data;
	GConfClient      *client;
	char             *options = "none";

	if (error) {
		_gtk_error_dialog_run (GTK_WINDOW (data->window), "%s", error->message);
		g_free (data);
		return;
	}

	client = gconf_client_get_default ();

	if (path_is_file (uri)) {
		char *image_path;
		
		image_path = get_local_path_from_uri (uri);
		gconf_client_set_string (client,
					 "/desktop/gnome/background/picture_filename",
					 image_path,
					 NULL);
		g_free (image_path);
		
		switch (data->align) {
		case WALLPAPER_ALIGN_TILED:
			options = "wallpaper";
			break;
		case WALLPAPER_ALIGN_CENTERED:
			options = "centered";
			break;
		case WALLPAPER_ALIGN_STRETCHED:
			options = "stretched";
			break;
		case WALLPAPER_ALIGN_SCALED:
			options = "scaled";
			break;
		case WALLPAPER_NONE:
			options = "none";
			break;
		}
	}

	gconf_client_set_string (client,
				 "/desktop/gnome/background/picture_options",
				 options,
				 NULL);
        g_object_unref (G_OBJECT (client));
        
	g_free (data);	
}


static char *
get_wallpaper_filename (int n)
{
	char *name, *filename;

	name = g_strdup_printf (".temp_wallpaper_%d.png", n);
	filename = g_strconcat ("file://",
				g_get_home_dir (),
				"/",
				name,
				NULL);
	g_free (name);

	return filename;
}


static char *
get_new_wallpaper_filename (void) 
{
	char *wallpaper_filename;
	
	wallpaper_filename = get_wallpaper_filename (1);
	if (path_is_file (wallpaper_filename)) {
		/* Use a new filename to force an update. */
		file_unlink (wallpaper_filename);
		g_free (wallpaper_filename);
		wallpaper_filename = get_wallpaper_filename (2);
		if (path_is_file (wallpaper_filename))
			file_unlink (wallpaper_filename);
	}
	
	return 	wallpaper_filename;
}


static void
set_wallpaper (GthWindow      *window,
	       const char     *image_path,
	       WallpaperAlign  align)
{
	SetWallpaperData *data;
	
	data = g_new0 (SetWallpaperData, 1);
	data->window = window;
	data->align = align;
	
	if (is_local_file (image_path))
		set_wallpaper_step_2 (image_path, GNOME_VFS_OK, data);
	else {
		char *wallpaper_filename;
		
		wallpaper_filename = get_new_wallpaper_filename ();
		copy_file_async (image_path, wallpaper_filename, set_wallpaper_step_2, data);
		g_free (wallpaper_filename);
	}
}


static void
set_wallpaper_from_window (GthWindow      *window,
			   WallpaperAlign  align)
{
	char *image_path = NULL;

	if (! gth_window_get_image_modified (window)) {
		const char *filename = gth_window_get_image_filename (window);
		if (filename != NULL)
			image_path = g_strdup (filename);
	} 
	else {
		ImageViewer *image_viewer;
		GdkPixbuf   *pixbuf;
		char        *wallpaper_filename = NULL;
		char        *local_file;
		GError      *error = NULL;

		image_viewer = gth_window_get_image_viewer (window);
		pixbuf = image_viewer_get_current_pixbuf (image_viewer);
		if (pixbuf == NULL)
			return;

		g_object_ref (pixbuf);

		wallpaper_filename = get_new_wallpaper_filename ();
		local_file = get_local_path_from_uri (wallpaper_filename);
		if (! _gdk_pixbuf_save (pixbuf,
					local_file,
					NULL,
					"jpeg",
					&error,
					NULL)) 
		{
			_gtk_error_dialog_from_gerror_run (GTK_WINDOW (window), &error);
			g_object_unref (pixbuf);
			g_free (local_file);			
			g_free (wallpaper_filename);
			return;
		}

		image_path = wallpaper_filename;
		
		g_object_unref (pixbuf);
		g_free (local_file);
	}

	set_wallpaper (window, image_path, align);
	g_free (image_path);
}


void
gth_window_activate_action_wallpaper_centered (GtkAction *action,
					       gpointer   data)
{
	GthWindow *window = GTH_WINDOW (data);
	set_wallpaper_from_window (window, WALLPAPER_ALIGN_CENTERED);
}


void
gth_window_activate_action_wallpaper_tiled (GtkAction *action,
					    gpointer   data)
{
	GthWindow *window = GTH_WINDOW (data);
	set_wallpaper_from_window (window, WALLPAPER_ALIGN_TILED);
}


void
gth_window_activate_action_wallpaper_scaled (GtkAction *action,
					     gpointer   data)
{
	GthWindow *window = GTH_WINDOW (data);
	set_wallpaper_from_window (window, WALLPAPER_ALIGN_SCALED);
}


void
gth_window_activate_action_wallpaper_stretched (GtkAction *action,
						gpointer   data)
{
	GthWindow *window = GTH_WINDOW (data);
	set_wallpaper_from_window (window, WALLPAPER_ALIGN_STRETCHED);
}


void
gth_window_activate_action_wallpaper_restore (GtkAction *action,
					      gpointer   data)
{
	GthWindow *window = GTH_WINDOW (data);
	int        align_type = WALLPAPER_ALIGN_CENTERED;

	if (strcmp (preferences.wallpaperAlign, "wallpaper") == 0)
		align_type = WALLPAPER_ALIGN_TILED;
	else if (strcmp (preferences.wallpaperAlign, "centered") == 0)
		align_type = WALLPAPER_ALIGN_CENTERED;
	else if (strcmp (preferences.wallpaperAlign, "stretched") == 0)
		align_type = WALLPAPER_ALIGN_STRETCHED;
	else if (strcmp (preferences.wallpaperAlign, "scaled") == 0)
		align_type = WALLPAPER_ALIGN_SCALED;
	else if (strcmp (preferences.wallpaperAlign, "none") == 0)
		align_type = WALLPAPER_NONE;

	set_wallpaper (window, preferences.wallpaper, align_type);
}


void
gth_window_activate_action_help_about (GtkAction *action,
				       gpointer   data)
{
	GthWindow  *window = GTH_WINDOW (data);
	const char *authors[] = {
		"Paolo Bacchilega <paobac@svn.gnome.org>",
		"Michael J. Chudobiak <mjc@svn.gnome.org>",
		"",
		"Josua Groeger <fractalus@gmx.net>",
		"Chauk-Mean Proum <chaukmean@free.fr>",
		"Jason Tackaberry <tack@auc.ca>",
		"Martin Ebourne <martin@zepler.org>",
		"Manuel Braga <mnulbga@gmail.com>",
		"Jef Driesen <jefdriesen@hotmail.com>",
		"Rennie deGraaf <degraaf@cpsc.ucalgary.ca>",
		"Joakim Larsson <joakim@bildrulle.nu>",
		"Christophe Bisière <bisiere@univ-tlse1.fr>",
		"Ulisse Perusin <uli.peru@gmail.com>",
		"Ignacy Gawędzki <ibugs@qult.net>",
		"Roalt Aalmoes <gnome@roalt.com>",
		"Geoffrey Antos <dynamotwain@aim.com>",
		"Michael Lester <element3260@gmail.com>",
		"Martin Schmeißer <Waschbaehr@gmx.de>",
		"Sean Hunt <rideau3@gmail.com>",
		"Natan Yellin <aantny@gmail.com>",
		"Adrien Bustany <madcat@mymadcat.com>",
		"David Turner <cillian64@googlemail.com>",
		"Dor Fire <dorfire@gmail.com>",
		"Gabriel Falcão <gabriel@nacaolivre.org>",
		NULL
	};
	const char *documenters [] = {
		"Paolo Bacchilega",
		"Alexander Kirillov",
		NULL
	};
	char       *license_text;
	const char *license[] = {
                "gThumb is free software; you can redistribute it and/or modify "
                "it under the terms of the GNU General Public License as published by "
                "the Free Software Foundation; either version 2 of the License, or "
                "(at your option) any later version.",
                "gThumb is distributed in the hope that it will be useful, "
                "but WITHOUT ANY WARRANTY; without even the implied warranty of "
                "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the "
                "GNU General Public License for more details.",
                "You should have received a copy of the GNU General Public License "
                "along with Nautilus; if not, write to the Free Software Foundation, Inc., "
                "51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA"
        };

        license_text = g_strconcat (license[0], "\n\n", license[1], "\n\n",
                                    license[2], "\n\n", NULL);


	gtk_show_about_dialog (GTK_WINDOW (window),
                               "version", VERSION,
                               "copyright", "Copyright \xc2\xa9 2001-2008 Free Software Foundation, Inc.",
                               "comments", _("An image viewer and browser for GNOME."),
                               "authors", authors,
                               "documenters", documenters,
                               "translator-credits", _("translator_credits"),
                               "logo-icon-name", "gthumb",
                               "license", license_text,
                               "wrap-license", TRUE,
			       "website", "http://gthumb.sourceforge.net",
                               NULL);

        g_free (license_text);
}


void
gth_window_activate_action_help_help (GtkAction *action,
				      gpointer   data)
{
	gthumb_display_help (GTK_WINDOW (data), NULL);
}


void
gth_window_activate_action_help_shortcuts (GtkAction *action,
					   gpointer   data)
{
	gthumb_display_help (GTK_WINDOW (data), "shortcuts");
}


void
gth_window_activate_action_view_toggle_animation (GtkAction *action,
						  GthWindow *window)
{
	gth_window_set_animation (window, gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)));
}


void
gth_window_activate_action_view_step_animation (GtkAction *action,
						GthWindow *window)
{
	gth_window_step_animation (window);
}


void
gth_window_activate_action_tools_change_date (GtkAction *action,
					      GthWindow *window)
{
        dlg_change_date (window);
}


void
gth_window_activate_action_tools_reset_exif (GtkAction *action,
                                             GthWindow *window)
{
        dlg_reset_exif (window);
}


void
gth_window_activate_action_tools_jpeg_rotate (GtkAction *action,
					      GthWindow *window)
{
	void (*module) (GthWindow *window);

	if (gthumb_module_get ("dlg_jpegtran", (gpointer*) &module))
		(*module) (window);
}


void
gth_window_activate_action_tools_jpeg_rotate_right (GtkAction *action,
						    GthWindow *window)
{
	void (*module) (GthWindow *, GthTransform);

	if (gthumb_module_get ("dlg_apply_jpegtran", (gpointer*) &module))
		(*module) (window, GTH_TRANSFORM_ROTATE_90);
}


void
gth_window_activate_action_tools_jpeg_rotate_left (GtkAction *action,
						   GthWindow *window)
{
	void (*module) (GthWindow *, GthTransform);

	if (gthumb_module_get ("dlg_apply_jpegtran", (gpointer*) &module))
		(*module) (window, GTH_TRANSFORM_ROTATE_270);
}

