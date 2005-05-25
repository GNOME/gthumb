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

#include <gconf/gconf-client.h>
#include <libgnome/gnome-exec.h>
#include <libgnome/gnome-help.h>
#include <libgnome/gnome-url.h>
#include <libgnomeui/libgnomeui.h>
#include <libbonoboui.h>
#include <libgnomevfs/gnome-vfs-ops.h>

#include "async-pixbuf-ops.h"
#include "catalog.h"
#include "comments.h"
#include "dlg-bookmarks.h"
#include "dlg-brightness-contrast.h"
#include "dlg-hue-saturation.h"
#include "dlg-catalog.h"
#include "dlg-categories.h"
#include "dlg-change-date.h"
#include "dlg-comment.h"
#include "dlg-convert.h"
#include "dlg-file-utils.h"
#include "dlg-open-with.h"
#include "dlg-posterize.h"
#include "dlg-color-balance.h"
#include "dlg-preferences.h"
#include "dlg-rename-series.h"
#include "dlg-scale-image.h"
#include "dlg-crop.h"
#include "dlg-write-to-cd.h"
#include "dlg-image-prop.h"
#include "fullscreen.h"
#include "gconf-utils.h"
#include "gth-pixbuf-op.h"
#include "gth-viewer.h"
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
	gth_window_save_pixbuf (window, image_viewer_get_current_pixbuf (image_viewer));
}


void
gth_window_activate_action_file_revert (GtkAction *action,
					GthWindow *window)
{
	gth_window_reload_current_image (window);
}


void
gth_window_activate_action_file_print (GtkAction *action,
				       GthWindow *window)
{
	GList *list = gth_window_get_file_list_selection (window);

	if (list == NULL)
		return;
	print_catalog_dlg (GTK_WINDOW (window), list);
	g_list_free (list);
}


void
gth_window_activate_action_edit_edit_comment (GtkAction *action,
					      GthWindow *window)
{
	gth_window_edit_comment (window);
}


void
gth_window_activate_action_edit_delete_comment (GtkAction *action,
						GthWindow *window)
{
	GList *list, *scan;

	list = gth_window_get_file_list_selection (window);
	for (scan = list; scan; scan = scan->next) {
		char        *filename = scan->data;
		CommentData *cdata;

		cdata = comments_load_comment (filename);
		comment_data_free_comment (cdata);
		if (! comment_data_is_void (cdata))
			comments_save_comment (filename, cdata);
		else
			comment_delete (filename);
		comment_data_free (cdata);

		all_windows_notify_update_comment (filename);
	}
	path_list_free (list);

	gth_window_update_current_image_metadata (window);

	/* FIXME
	if (window->image_prop_dlg != NULL)
		dlg_image_prop_update (window->image_prop_dlg);
	*/
}


void
gth_window_activate_action_edit_edit_categories (GtkAction *action,
						 GthWindow *window)
{
	gth_window_edit_categories (window);
}



void
gth_window_activate_action_alter_image_rotate90 (GtkAction *action,
						 gpointer   data)
{
	GthWindow   *window = GTH_WINDOW (data);
	ImageViewer *image_viewer;
	GdkPixbuf   *src_pixbuf;
	GdkPixbuf   *dest_pixbuf;

	image_viewer = gth_window_get_image_viewer (window);
	src_pixbuf = image_viewer_get_current_pixbuf (image_viewer);
	dest_pixbuf = _gdk_pixbuf_copy_rotate_90 (src_pixbuf, FALSE);
	image_viewer_set_pixbuf (image_viewer, dest_pixbuf);
	g_object_unref (dest_pixbuf);
	gth_window_set_image_modified (window, TRUE);
}


void
gth_window_activate_action_alter_image_rotate90cc (GtkAction *action,
						   gpointer   data)
{
	GthWindow   *window = GTH_WINDOW (data);
	ImageViewer *image_viewer;
	GdkPixbuf   *src_pixbuf;
	GdkPixbuf   *dest_pixbuf;

	image_viewer = gth_window_get_image_viewer (window);
	src_pixbuf = image_viewer_get_current_pixbuf (image_viewer);
	dest_pixbuf = _gdk_pixbuf_copy_rotate_90 (src_pixbuf, TRUE);
	image_viewer_set_pixbuf (image_viewer, dest_pixbuf);
	g_object_unref (dest_pixbuf);
	gth_window_set_image_modified (window, TRUE);
}


void
gth_window_activate_action_alter_image_flip (GtkAction *action,
					     gpointer   data)
{
	GthWindow   *window = GTH_WINDOW (data);
	ImageViewer *image_viewer;
	GdkPixbuf   *src_pixbuf;
	GdkPixbuf   *dest_pixbuf;

	image_viewer = gth_window_get_image_viewer (window);
	src_pixbuf = image_viewer_get_current_pixbuf (image_viewer);
	dest_pixbuf = _gdk_pixbuf_copy_mirror (src_pixbuf, FALSE, TRUE);
	image_viewer_set_pixbuf (image_viewer, dest_pixbuf);
	g_object_unref (dest_pixbuf);
	gth_window_set_image_modified (window, TRUE);
}


void
gth_window_activate_action_alter_image_mirror (GtkAction *action,
					       gpointer   data)
{
	GthWindow   *window = GTH_WINDOW (data);
	ImageViewer *image_viewer;
	GdkPixbuf   *src_pixbuf;
	GdkPixbuf   *dest_pixbuf;

	image_viewer = gth_window_get_image_viewer (window);
	src_pixbuf = image_viewer_get_current_pixbuf (image_viewer);
	dest_pixbuf = _gdk_pixbuf_copy_mirror (src_pixbuf, TRUE, FALSE);
	image_viewer_set_pixbuf (image_viewer, dest_pixbuf);
	g_object_unref (dest_pixbuf);
	gth_window_set_image_modified (window, TRUE);
}


void
gth_window_activate_action_alter_image_desaturate (GtkAction *action,
						   gpointer   data)
{
	GthWindow   *window = GTH_WINDOW (data);
	ImageViewer *image_viewer;
	GdkPixbuf   *src_pixbuf;
	GdkPixbuf   *dest_pixbuf;
	GthPixbufOp *pixop;

	image_viewer = gth_window_get_image_viewer (window);
	src_pixbuf = image_viewer_get_current_pixbuf (image_viewer);
	dest_pixbuf = gdk_pixbuf_copy (src_pixbuf);
	pixop = _gdk_pixbuf_desaturate (dest_pixbuf, dest_pixbuf);
	g_object_unref (dest_pixbuf);
	gth_window_exec_pixbuf_op (window, pixop);
	g_object_unref (pixop);
}


void
gth_window_activate_action_alter_image_invert (GtkAction *action,
					       gpointer   data)
{
	GthWindow   *window = GTH_WINDOW (data);
	ImageViewer *image_viewer;
	GdkPixbuf   *src_pixbuf;
	GdkPixbuf   *dest_pixbuf;
	GthPixbufOp *pixop;

	image_viewer = gth_window_get_image_viewer (window);
	src_pixbuf = image_viewer_get_current_pixbuf (image_viewer);
	dest_pixbuf = gdk_pixbuf_copy (src_pixbuf);
	pixop = _gdk_pixbuf_invert (dest_pixbuf, dest_pixbuf);
	g_object_unref (dest_pixbuf);
	gth_window_exec_pixbuf_op (window, pixop);
	g_object_unref (pixop);
}


void
gth_window_activate_action_alter_image_adjust_levels (GtkAction *action,
						      gpointer   data)
{
	GthWindow   *window = GTH_WINDOW (data);
	ImageViewer *image_viewer;
	GdkPixbuf   *src_pixbuf;
	GdkPixbuf   *dest_pixbuf;
	GthPixbufOp *pixop;

	image_viewer = gth_window_get_image_viewer (window);
	src_pixbuf = image_viewer_get_current_pixbuf (image_viewer);
	dest_pixbuf = gdk_pixbuf_copy (src_pixbuf);
	pixop = _gdk_pixbuf_adjust_levels (dest_pixbuf, dest_pixbuf);
	g_object_unref (dest_pixbuf);
	gth_window_exec_pixbuf_op (window, pixop);
	g_object_unref (pixop);
}


void
gth_window_activate_action_alter_image_equalize (GtkAction *action,
						 gpointer   data)
{
	GthWindow   *window = GTH_WINDOW (data);
	ImageViewer *image_viewer;
	GdkPixbuf   *src_pixbuf;
	GdkPixbuf   *dest_pixbuf;
	GthPixbufOp *pixop;

	image_viewer = gth_window_get_image_viewer (window);
	src_pixbuf = image_viewer_get_current_pixbuf (image_viewer);
	dest_pixbuf = gdk_pixbuf_copy (src_pixbuf);
	pixop = _gdk_pixbuf_eq_histogram (dest_pixbuf, dest_pixbuf);
	g_object_unref (dest_pixbuf);
	gth_window_exec_pixbuf_op (window, pixop);
	g_object_unref (pixop);
}


void
gth_window_activate_action_alter_image_normalize (GtkAction *action,
						  gpointer   data)
{
	GthWindow   *window = GTH_WINDOW (data);
	ImageViewer *image_viewer;
	GdkPixbuf   *src_pixbuf;
	GdkPixbuf   *dest_pixbuf;
	GthPixbufOp *pixop;

	image_viewer = gth_window_get_image_viewer (window);
	src_pixbuf = image_viewer_get_current_pixbuf (image_viewer);
	dest_pixbuf = gdk_pixbuf_copy (src_pixbuf);
	pixop = _gdk_pixbuf_normalize_contrast (dest_pixbuf, dest_pixbuf);
	g_object_unref (dest_pixbuf);
	gth_window_exec_pixbuf_op (window, pixop);
	g_object_unref (pixop);
}


void
gth_window_activate_action_alter_image_stretch_contrast (GtkAction *action,
							 gpointer   data)
{
	GthWindow   *window = GTH_WINDOW (data);
	ImageViewer *image_viewer;
	GdkPixbuf   *src_pixbuf;
	GdkPixbuf   *dest_pixbuf;
	GthPixbufOp *pixop;

	image_viewer = gth_window_get_image_viewer (window);
	src_pixbuf = image_viewer_get_current_pixbuf (image_viewer);
	dest_pixbuf = gdk_pixbuf_copy (src_pixbuf);
	pixop = _gdk_pixbuf_stretch_contrast (dest_pixbuf, dest_pixbuf);
	g_object_unref (dest_pixbuf);
	gth_window_exec_pixbuf_op (window, pixop);
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
gth_window_activate_action_alter_image_color_balance (GtkAction *action,
						      GthWindow *window)
{
	dlg_color_balance (window);
}


void
gth_window_activate_action_alter_image_threshold (GtkAction *action,
						  gpointer   data)
{
}


void
gth_window_activate_action_alter_image_resize (GtkAction *action,
					       GthWindow *window)
{
	dlg_scale_image (window);
}


void
gth_window_activate_action_alter_image_rotate (GtkAction *action,
					       gpointer   data)
{
}


void
gth_window_activate_action_alter_image_crop (GtkAction *action,
					     GthWindow *window)
{
	dlg_crop (window);
}


void
gth_window_activate_action_view_zoom_in (GtkAction *action,
					 gpointer   data)
{
	GthWindow *window = GTH_WINDOW (data);
	image_viewer_zoom_in (gth_window_get_image_viewer (window));
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
gth_window_activate_action_view_zoom_fit (GtkAction *action,
					  gpointer   data)
{
	GthWindow   *window = GTH_WINDOW (data);
	ImageViewer *image_viewer = gth_window_get_image_viewer (window);

	if (image_viewer->zoom_fit) 
		image_viewer->zoom_fit = FALSE;
	else 
		image_viewer_zoom_to_fit (image_viewer);
}


void
gth_window_activate_action_view_fullscreen (GtkAction *action,
					    GthWindow *window)
{
	gth_window_set_fullscreen (window, TRUE);
}


void
gth_window_activate_action_view_exit_fullscreen (GtkAction *action,
						 GthWindow *window)
{
	gth_window_set_fullscreen (window, FALSE);
}


static void
set_as_wallpaper (const gchar    *image_path,
		  WallpaperAlign  align)
{
	GConfClient *client;
	char        *options = "none";

	client = gconf_client_get_default ();

	if ((image_path == NULL) || ! path_is_file (image_path)) 
		options = "none";

	else {
		gconf_client_set_string (client, 
					 "/desktop/gnome/background/picture_filename",
					 image_path,
					 NULL);
		switch (align) {
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
}


void
gth_window_activate_action_wallpaper_centered (GtkAction *action,
					       gpointer   data)
{
	GthWindow  *window = GTH_WINDOW (data);
	set_as_wallpaper (gth_window_get_image_filename (window),  WALLPAPER_ALIGN_CENTERED);
}


void
gth_window_activate_action_wallpaper_tiled (GtkAction *action,
					    gpointer   data)
{
	GthWindow  *window = GTH_WINDOW (data);
	set_as_wallpaper (gth_window_get_image_filename (window),  WALLPAPER_ALIGN_TILED);
}


void
gth_window_activate_action_wallpaper_scaled (GtkAction *action,
					     gpointer   data)
{
	GthWindow  *window = GTH_WINDOW (data);
	set_as_wallpaper (gth_window_get_image_filename (window),  WALLPAPER_ALIGN_SCALED);
}


void
gth_window_activate_action_wallpaper_stretched (GtkAction *action,
						gpointer   data)
{
	GthWindow  *window = GTH_WINDOW (data);
	set_as_wallpaper (gth_window_get_image_filename (window),  WALLPAPER_ALIGN_STRETCHED);
}


void
gth_window_activate_action_wallpaper_restore (GtkAction *action,
					      gpointer   data)
{
	int align_type = WALLPAPER_ALIGN_CENTERED;

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

	set_as_wallpaper (preferences.wallpaper, align_type);
}


void
gth_window_activate_action_help_about (GtkAction *action,
				       gpointer   data)
{
	GthWindow *window = GTH_WINDOW (data);
	static GtkWidget *about;
	GdkPixbuf *logo;
	const char *authors[] = {
		"Paolo Bacchilega <paolo.bacchilega@libero.it>",
		NULL
	};
	const char *documenters [] = {
		"Paolo Bacchilega",
		"Alexander Kirillov", 
		NULL
	};
	const char *translator_credits = _("translator_credits");


	if (about != NULL) {
		gdk_window_show (about->window);
		gdk_window_raise (about->window);
		return;
	}

	logo = gdk_pixbuf_new_from_file (PIXMAPSDIR "/gthumb.png", NULL);

	about = gtk_about_dialog_new ();
	g_object_set (about,
		      "name", _("gThumb"), 
		      "version", VERSION,
		      "copyright",  "Copyright \xc2\xa9 2001-2004 Free Software Foundation, Inc.",
		      "comments", _("An image viewer and browser for GNOME."),
		      "authors", authors,
		      "documenters", documenters,
		      "translator_credits", strcmp (translator_credits, "translator_credits") != 0 ? translator_credits : NULL,
		      "logo", logo,
		      "website", "http://gthumb.sourceforge.net",
		      NULL);

	if (logo != NULL)
                g_object_unref (logo);

	gtk_window_set_destroy_with_parent (GTK_WINDOW (about), TRUE);
	gtk_window_set_transient_for (GTK_WINDOW (about),
				      GTK_WINDOW (window));

	g_signal_connect (G_OBJECT (about), 
			  "destroy",
			  G_CALLBACK (gtk_widget_destroyed), 
			  &about);

	gtk_widget_show (about);
}


static void
display_help (GthWindow  *window,
	      const char *section) 
{
	GError *err;

	err = NULL;  
	gnome_help_display ("gthumb", section, &err);
	
	if (err != NULL) {
		GtkWidget *dialog;
		
		dialog = gtk_message_dialog_new (GTK_WINDOW (window),
						 GTK_DIALOG_DESTROY_WITH_PARENT,
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
gth_window_activate_action_help_help (GtkAction *action,
				      gpointer   data)
{
	display_help (GTH_WINDOW (data), NULL);
}


void
gth_window_activate_action_help_shortcuts (GtkAction *action,
					   gpointer   data)
{
	display_help (GTH_WINDOW (data), "shortcuts");
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
gth_window_activate_action_view_show_info (GtkAction *action,
					   gpointer   data)
{
	/*
	GThumbWindow *window = data;

	if (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)))
		window_show_image_data (window);
	else
		window_hide_image_data (window);
	*/
}
