/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2004 Free Software Foundation, Inc.
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
#include "fullscreen.h"
#include "gconf-utils.h"
#include "gth-pixbuf-op.h"
#include "gthumb-error.h"
#include "gthumb-module.h"
#include "gthumb-window.h"
#include "gtk-utils.h"
#include "gth-file-view.h"
#include "image-viewer.h"
#include "main.h"
#include "pixbuf-utils.h"
#include "print-callbacks.h"
#include "thumb-cache.h"
#include "typedefs.h"
#include "gth-folder-selection-dialog.h"

#define MAX_NAME_LEN 1024
#define DEF_CONFIRM_DEL TRUE

typedef enum {
	WALLPAPER_ALIGN_TILED     = 0,
	WALLPAPER_ALIGN_CENTERED  = 1,
	WALLPAPER_ALIGN_STRETCHED = 2,
	WALLPAPER_ALIGN_SCALED    = 3,
	WALLPAPER_NONE            = 4
} WallpaperAlign;


static void
open_new_window_at_location (GThumbWindow *window,
                             const char   *location)
{
        ImageViewer  *viewer = IMAGE_VIEWER (window->viewer);
        GThumbWindow *new_window;
        int           width, height;

        if (location != NULL)
                preferences_set_startup_location (location);

        /* Save visualization options. */

        gdk_drawable_get_size (window->app->window, &width, &height);
        eel_gconf_set_integer (PREF_UI_WINDOW_WIDTH, width);
        eel_gconf_set_integer (PREF_UI_WINDOW_HEIGHT, height);

        eel_gconf_set_boolean (PREF_SHOW_THUMBNAILS, window->file_list->enable_thumbs);

	pref_set_arrange_type (window->file_list->sort_method);
        pref_set_sort_order (window->file_list->sort_type);

        pref_set_zoom_quality (image_viewer_get_zoom_quality (viewer));
        pref_set_transp_type (image_viewer_get_transp_type (viewer));

        pref_set_check_type (image_viewer_get_check_type (viewer));
        pref_set_check_size (image_viewer_get_check_size (viewer));

        eel_gconf_set_boolean (PREF_UI_IMAGE_PANE_VISIBLE, window->preview_visible);

        /**/

        new_window = window_new ();
        gtk_widget_show (new_window->app);
}


void
activate_action_file_new_window (GtkAction *action,
				 gpointer   data)
{
	GThumbWindow *window = data;
        char         *location = NULL;

        if ((window->sidebar_content == GTH_SIDEBAR_DIR_LIST)
            && (window->dir_list->path != NULL))
                location = g_strconcat ("file://",
                                        window->dir_list->path,
                                        NULL);
        else if ((window->sidebar_content == GTH_SIDEBAR_CATALOG_LIST)
                 && (window->catalog_path != NULL))
                location = g_strconcat ("catalog://",
                                        window->catalog_path,
                                        NULL);

        open_new_window_at_location (window, location);
        g_free (location);
}


void
activate_action_file_close_window (GtkAction *action,
				   gpointer   data)
{
	GThumbWindow *window = data;
	window_close (window);
}


void
activate_action_file_open_with (GtkAction *action,
				gpointer   data)
{
	GThumbWindow *window = data;
	GList        *list;

	list = gth_file_view_get_file_list_selection (window->file_list->view);
	g_return_if_fail (list != NULL);

	open_with_cb (window, list);

	/* the list is deallocated when the dialog is closed. */
}


void
activate_action_file_save (GtkAction *action,
			   gpointer   data)
{
	GThumbWindow *window = data;
	ImageViewer  *viewer = IMAGE_VIEWER (window->viewer);

	window_save_pixbuf (window, image_viewer_get_current_pixbuf (viewer));
}


void
activate_action_file_revert (GtkAction *action,
			     gpointer   data)
{
	GThumbWindow *window = data;
	window_reload_image (window);
}


void
activate_action_file_image_properties (GtkAction *action,
				       gpointer   data)
{
}


void
activate_action_file_print (GtkAction *action,
			    gpointer   data)
{
	GThumbWindow *window = data;
	GList        *list;
	
	list = gth_file_view_get_file_list_selection (window->file_list->view);
	if (list != NULL)
		print_catalog_dlg (GTK_WINDOW (window->app), list);
	path_list_free (list);
}


void
activate_action_file_camera_import (GtkAction *action,
				    gpointer   data)
{
#ifdef HAVE_LIBGPHOTO

	GThumbWindow *window = data;
	void (*module) (GThumbWindow *window);

	if (gthumb_module_get ("dlg_photo_importer", (gpointer*) &module))
		(*module) (window);

#endif /*HAVE_LIBGPHOTO */
}


void
activate_action_file_write_to_cd (GtkAction *action,
				  gpointer   data)
{
	GThumbWindow *window = data;
	dlg_write_to_cd (window);

	/*
	GList        *list;

	list = gth_file_list_get_selection (window->file_list);
	if (list != NULL)
		dlg_copy_items (window, 
				list,
				"burn:///",
				FALSE,
				TRUE,
				write_to_cd__continue,
				window);
	path_list_free (list);
	*/
}


void
activate_action_image_open_with (GtkAction *action,
				 gpointer   data)
{
	GThumbWindow *window = data;
	GList        *list;

	if (window->image_path == NULL)
		return;

	list = g_list_prepend (NULL, g_strdup (window->image_path));
	open_with_cb (window, list);

	/* the list is deallocated when the dialog is closed. */
}


static void 
rename_file (GThumbWindow *window,
	     const GList  *list)
{
	const char   *old_name, *old_path;
	char         *old_name_utf8, *new_name_utf8;
	char         *new_name, *new_path;
	char         *dir;

	g_return_if_fail (list != NULL);
		
	old_path = list->data;
	old_name = file_name_from_path (old_path);
	old_name_utf8 = g_filename_to_utf8 (old_name, -1, NULL, NULL, NULL);

	new_name_utf8 = _gtk_request_dialog_run (GTK_WINDOW (window->app),
						 GTK_DIALOG_MODAL,
						 _("Enter the new name: "),
						 old_name_utf8,
						 MAX_NAME_LEN,
						 GTK_STOCK_CANCEL,
						 _("_Rename"));
	g_free (old_name_utf8);

	if (new_name_utf8 == NULL) 
		return;

	new_name = g_filename_from_utf8 (new_name_utf8, -1, 0, 0, 0);
	g_free (new_name_utf8);

	if (strchr (new_name, '/') != NULL) {
		char *utf8_name;

		utf8_name = g_filename_to_utf8 (new_name, -1, NULL, NULL, NULL);
		_gtk_error_dialog_run (GTK_WINDOW (window->app),
				       _("The name \"%s\" is not valid because it contains the character \"/\". " "Please use a different name."), utf8_name);
		g_free (utf8_name);
		g_free (new_name);

		return;
	}

	/* Rename */

	dir = remove_level_from_path (old_path);
	new_path = g_build_path ("/", dir, new_name, NULL);
	g_free (dir);

	if (path_is_file (new_path)) {
		GtkWidget *d;
		char      *message;
		char      *utf8_name;
		int        r;

		utf8_name = g_filename_to_utf8 (new_name, -1, NULL, NULL, NULL);
		message = g_strdup_printf (_("An image named \"%s\" is already present. " "Do you want to overwrite it?"), utf8_name);
		g_free (utf8_name);

		d = _gtk_yesno_dialog_new (GTK_WINDOW (window->app),
					   GTK_DIALOG_MODAL,
					   message,
					   GTK_STOCK_CANCEL,
					   _("_Overwrite"));
		g_free (message);

		r = gtk_dialog_run (GTK_DIALOG (d));
		gtk_widget_destroy (GTK_WIDGET (d));

		if (r != GTK_RESPONSE_YES) {
			g_free (new_path);
			g_free (new_name);
			return;
		}
	} 

	all_windows_remove_monitor ();

	if (strcmp (old_path, new_path) == 0) {
		char *utf8_path;

		utf8_path = g_filename_to_utf8 (old_path, -1, NULL, NULL, NULL);
		_gtk_error_dialog_run (GTK_WINDOW (window->app),
				       _("Could not rename the image \"%s\": %s"),
				       utf8_path,
				       _("source and destination are the same"));
		g_free (utf8_path);

	} else if (file_move (old_path, new_path)) {
		cache_move (old_path, new_path);
		comment_move (old_path, new_path);
		all_windows_notify_file_rename (old_path, new_path);

	} else {
		char *utf8_path;

		utf8_path = g_filename_to_utf8 (old_path, -1, NULL, NULL, NULL);
		_gtk_error_dialog_run (GTK_WINDOW (window->app),
				       _("Could not rename the image \"%s\": %s"),
				       utf8_path,
				       errno_to_string ());
		g_free (utf8_path);
	}

	all_windows_add_monitor ();

	g_free (new_path);
	g_free (new_name);
}


void
activate_action_image_rename (GtkAction *action,
			      gpointer   data)
{
	GThumbWindow *window = data;
	GList        *list;

	if (window->image_path == NULL)
		return;

	list = g_list_prepend (NULL, g_strdup (window->image_path));
	rename_file (window, list);
	path_list_free (list);
}


static void 
duplicate_file (GThumbWindow *window,
		const char   *old_path)
{
	const char *old_name, *ext;
	char       *old_name_no_ext;
	char       *new_name, *new_path;
	char       *dir;
	int         try;

	g_return_if_fail (old_path != NULL);
		
	old_name = file_name_from_path (old_path);
	old_name_no_ext = remove_extension_from_path (old_name);
	ext = strrchr (old_name, '.');

	dir = remove_level_from_path (old_path);

	for (try = 2; TRUE; try++) {
		new_name = g_strdup_printf ("%s (%d)%s", 
					    old_name_no_ext, 
					    try,
					    (ext == NULL) ? "" : ext);

		new_path = g_build_path ("/", dir, new_name, NULL);
		if (! path_is_file (new_path))
			break;
		g_free (new_name);
		g_free (new_path);
	}
	
	g_free (dir);
	g_free (old_name_no_ext);

	if (file_copy (old_path, new_path)) {
		GList *file_list;

		cache_copy (old_path, new_path);
		comment_copy (old_path, new_path);

		all_windows_remove_monitor ();

		file_list = g_list_prepend (NULL, new_path);
		all_windows_notify_files_created (file_list);
		g_list_free (file_list);

		all_windows_add_monitor ();

	} else {
		char      *utf8_path;
		char      *msg;
		GtkWidget *d;
		int        r;

		utf8_path = g_filename_to_utf8 (old_path, -1, NULL, NULL, NULL);
		msg = g_strdup_printf (_("Could not duplicate the image \"%s\": %s"),
				       utf8_path,
				       errno_to_string ());
		g_free (utf8_path);

		d = _gtk_yesno_dialog_new (GTK_WINDOW (window->app),
					   GTK_DIALOG_MODAL,
					   msg,
					   _("Stop"),
					   _("Continue"));
		g_free (msg);

		r = gtk_dialog_run (GTK_DIALOG (d));
		gtk_widget_destroy (GTK_WIDGET (d));

		if (r != GTK_RESPONSE_YES) {
			g_free (new_path);
			g_free (new_name);
			return;
		}
	}

	g_free (new_name);
	g_free (new_path);
}


static void 
duplicate_file_list (GThumbWindow *window,
		     const GList  *list)
{
	const GList *scan;

	for (scan = list; scan; scan = scan->next) {
		char *filename = scan->data;
		duplicate_file (window, filename);
	}
}


void
activate_action_image_duplicate (GtkAction *action,
				 gpointer   data)
{
	GThumbWindow *window = data;
	GList        *list;

	if (window->image_path == NULL)
		return;

	list = g_list_prepend (NULL, g_strdup (window->image_path));
	duplicate_file_list (window, list);
	path_list_free (list);
}


void
activate_action_image_delete (GtkAction *action,
			      gpointer   data)
{
	GThumbWindow *window = data;
	GList        *list;

	if (window->image_path == NULL)
		return;

	list = g_list_prepend (NULL, g_strdup (window->image_path));
	dlg_file_delete__confirm (window, 
				  list,
				  _("The image will be moved to the Trash, are you sure?"));

	/* the list is deallocated when the dialog is closed. */
}


void
activate_action_image_copy (GtkAction *action,
			    gpointer   data)
{
	GThumbWindow *window = data;
	GList        *list;

	if (window->image_path == NULL)
		return;

	list = g_list_prepend (NULL, g_strdup (window->image_path));
	dlg_file_copy__ask_dest (window, list);

	/* the list is deallocated when the dialog is closed. */
}


void
activate_action_image_move (GtkAction *action,
			    gpointer   data)
{
	GThumbWindow *window = data;
	GList        *list;

	if (window->image_path == NULL)
		return;

	list = g_list_prepend (NULL, g_strdup (window->image_path));
	dlg_file_move__ask_dest (window, list);

	/* the list is deallocated when the dialog is closed. */
}


void
activate_action_edit_rename_file (GtkAction *action,
				  gpointer   data)
{
	GThumbWindow *window = data;
	GList        *list;

	list = gth_file_view_get_file_list_selection (window->file_list->view);
	g_return_if_fail (list != NULL);

	rename_file (window, list);
	path_list_free (list);
}


void
activate_action_edit_duplicate_file (GtkAction *action,
				     gpointer   data)
{
	GThumbWindow *window = data;
	GList        *list;

	list = gth_file_view_get_file_list_selection (window->file_list->view);
	g_return_if_fail (list != NULL);

	duplicate_file_list (window, list);
	path_list_free (list);
}


void
activate_action_edit_delete_files (GtkAction *action,
				   gpointer   data)
{
	GThumbWindow *window = data;
	GList        *list;

	list = gth_file_view_get_file_list_selection (window->file_list->view);
	dlg_file_delete__confirm (window, 
				  list, 
				  _("The selected images will be moved to the Trash, are you sure?"));

	/* the list is deallocated when the dialog is closed. */
}


void
activate_action_edit_copy_files (GtkAction *action,
				 gpointer   data)
{
	GThumbWindow *window = data;
	GList        *list;

	list = gth_file_view_get_file_list_selection (window->file_list->view);
	dlg_file_copy__ask_dest (window, list);

	/* the list is deallocated when the dialog is closed. */
}


void
activate_action_edit_move_files (GtkAction *action,
				 gpointer   data)
{
	GThumbWindow  *window = data;
	GList         *list;

	list = gth_file_view_get_file_list_selection (window->file_list->view);
	dlg_file_move__ask_dest (window, list);

	/* the list is deallocated when the dialog is closed. */
}


void
activate_action_edit_select_all (GtkAction *action,
				 gpointer   data)
{
	GThumbWindow *window = data;
	gth_file_view_select_all (window->file_list->view);
}


void
activate_action_edit_edit_comment (GtkAction *action,
				   gpointer   data)
{
	GThumbWindow *window = data;
	window_show_comment_dlg (window);
}


void
activate_action_edit_delete_comment (GtkAction *action,
				     gpointer   data)
{
	GThumbWindow *window = data;
	GList        *list, *scan;

	list = gth_file_view_get_file_list_selection (window->file_list->view);
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

	if (window->image_prop_dlg != NULL)
		dlg_image_prop_update (window->image_prop_dlg);
}


void
activate_action_edit_edit_categories (GtkAction *action,
				      gpointer   data)
{
	GThumbWindow *window = data;
	window_show_categories_dlg (window);
}


void
activate_action_edit_add_to_catalog (GtkAction *action,
				     gpointer   data)
{
	GThumbWindow *window = data;
	GList        *list;

	list = gth_file_view_get_file_list_selection (window->file_list->view);
	dlg_add_to_catalog (window, list);

	/* the list is deallocated when the dialog is closed. */
}


static void
real_remove_from_catalog (GThumbWindow *window, GList *list)
{
	GList     *scan;
	Catalog   *catalog;
	GError    *gerror;

	catalog = catalog_new ();
	if (! catalog_load_from_disk (catalog, window->catalog_path, &gerror)) {
		_gtk_error_dialog_from_gerror_run (GTK_WINDOW (window->app), &gerror);
		path_list_free (list);
		catalog_free (catalog);
		return;
	}

	for (scan = list; scan; scan = scan->next) 
		catalog_remove_item (catalog, (gchar*) scan->data);

	if (! catalog_write_to_disk (catalog, &gerror)) {
		_gtk_error_dialog_from_gerror_run (GTK_WINDOW (window->app), &gerror);
		path_list_free (list);
		catalog_free (catalog);
		return;
	}

	all_windows_notify_cat_files_deleted (window->catalog_path, list);
	path_list_free (list);
	catalog_free (catalog);
}


static void
remove_selection_from_catalog (GThumbWindow *window)
{
	GList     *list;

	list = gth_file_view_get_file_list_selection (window->file_list->view);
	real_remove_from_catalog (window, list);

	/* the list is deallocated in real_remove_from_catalog. */
}


void
activate_action_edit_remove_from_catalog (GtkAction *action,
					  gpointer   data)
{
	GThumbWindow *window = data;
	GtkWidget    *dialog;
	int           r;

	if (! eel_gconf_get_boolean (PREF_CONFIRM_DELETION, DEF_CONFIRM_DEL)) {
		remove_selection_from_catalog (window);
		return;
	}

	dialog = _gtk_yesno_dialog_new (GTK_WINDOW (window->app),
					GTK_DIALOG_MODAL,
					_("The selected images will be removed from the catalog, are you sure?"),
					GTK_STOCK_CANCEL,
					GTK_STOCK_REMOVE);

	r = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (GTK_WIDGET (dialog));
	if (r == GTK_RESPONSE_YES) 
		remove_selection_from_catalog (window);
}


static void
catalog_rename (GThumbWindow *window,
		const char   *catalog_path)
{
	char         *name_only;
	char         *name_only_utf8;
	char         *new_name;
	char         *new_name_utf8;
	char         *new_catalog_path;
	char         *path_only;
	gboolean      is_dir;

	is_dir = path_is_dir (catalog_path);
	
	if (! is_dir)
		name_only = remove_extension_from_path (file_name_from_path (catalog_path));
	else
		name_only = g_strdup (file_name_from_path (catalog_path));
	name_only_utf8 = g_filename_to_utf8 (name_only, -1, NULL, NULL, NULL);

	new_name_utf8 = _gtk_request_dialog_run (GTK_WINDOW (window->app),
						 GTK_DIALOG_MODAL,
						 _("Enter the new name: "),
						 name_only_utf8,
						 MAX_NAME_LEN,
						 GTK_STOCK_CANCEL,
						 _("_Rename"));
	g_free (name_only_utf8);
	
	if (new_name_utf8 == NULL) {
		g_free (name_only);
		return;
	}

	new_name = g_filename_from_utf8 (new_name_utf8, -1, 0, 0, 0);
	g_free (new_name_utf8);

	if (strchr (new_name, '/') != NULL) {
		char *utf8_name;

		utf8_name = g_filename_to_utf8 (new_name, -1, NULL, NULL, NULL);
		_gtk_error_dialog_run (GTK_WINDOW (window->app),
				       _("The name \"%s\" is not valid because it contains the character \"/\". " "Please use a different name."), utf8_name);

		g_free (utf8_name);
		g_free (new_name);
		g_free (name_only);

		return;
	}

	path_only = remove_level_from_path (catalog_path);
	new_catalog_path = g_strconcat (path_only,
					"/",
					new_name,
					! is_dir ? CATALOG_EXT : NULL,
					NULL);
	g_free (path_only);

	if (path_is_file (new_catalog_path)) {
		char *utf8_name;

		utf8_name = g_filename_to_utf8 (new_name, -1, NULL, NULL, NULL);
		_gtk_error_dialog_run (GTK_WINDOW (window->app), 
				       _("The name \"%s\" is already used. " "Please use a different name."), utf8_name);
		g_free (utf8_name);

	} else if (! rename (catalog_path, new_catalog_path)) {
		all_windows_notify_catalog_rename (catalog_path, 
						   new_catalog_path);
	} else {
		char *utf8_name;

		utf8_name = g_filename_to_utf8 (name_only, -1, NULL, NULL, NULL);
		_gtk_error_dialog_run (GTK_WINDOW (window->app), 
                                       is_dir ? _("Could not rename the library \"%s\": %s") : _("Could not rename the catalog \"%s\": %s"),
                                       utf8_name,
                                       errno_to_string ());
		g_free (utf8_name);
	}

	g_free (new_name); 	
	g_free (new_catalog_path);
	g_free (name_only);	
}


void
activate_action_edit_catalog_rename (GtkAction *action,
				     gpointer   data)
{
	GThumbWindow *window = data;
	char         *catalog_path;

	catalog_path = catalog_list_get_selected_path (window->catalog_list);
	if (catalog_path == NULL)
		return;	

	catalog_rename (window, catalog_path);

	g_free (catalog_path);
}


static void
real_catalog_delete (GThumbWindow *window)
{
	char     *catalog_path;
	GError   *gerror;
	gboolean  error;

	catalog_path = catalog_list_get_selected_path (window->catalog_list);
	if (catalog_path == NULL)
		return;

	if (path_is_dir (catalog_path)) 
		error = ! delete_catalog_dir (catalog_path, TRUE, &gerror);
	else 
		error = ! delete_catalog (catalog_path, &gerror);

	if (error) 
		_gtk_error_dialog_from_gerror_run (GTK_WINDOW (window->app), &gerror);

	all_windows_notify_catalog_delete (catalog_path);
	g_free (catalog_path);
}


static void
catalog_delete (GThumbWindow *window,
		const char   *catalog_path)
{
	GtkWidget    *dialog;
	gchar        *message;
	int           r;

	/* Always ask before deleting folders. */

	if (! path_is_dir (catalog_path) && ! eel_gconf_get_boolean (PREF_CONFIRM_DELETION, DEF_CONFIRM_DEL)) {
		real_catalog_delete (window);
		return;
	}

	if (path_is_dir (catalog_path)) 
		message = g_strdup (_("The selected library will be removed, are you sure?"));
	else
		message = g_strdup (_("The selected catalog will be removed, are you sure?"));

	dialog = _gtk_yesno_dialog_new (GTK_WINDOW (window->app),
					GTK_DIALOG_MODAL,
					message,
					GTK_STOCK_CANCEL,
					GTK_STOCK_REMOVE);
	g_free (message);

	r = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (GTK_WIDGET (dialog));
	if (r == GTK_RESPONSE_YES) 
		real_catalog_delete (window);
}


void
activate_action_edit_catalog_delete (GtkAction *action,
				     gpointer   data)
{
	GThumbWindow *window = data;
	gchar        *catalog_path;

	catalog_path = catalog_list_get_selected_path (window->catalog_list);
	if (catalog_path == NULL)
		return;

	catalog_delete (window, catalog_path);

	g_free (catalog_path);
}


void
activate_action_edit_catalog_move (GtkAction *action,
				   gpointer   data)
{
	GThumbWindow *window = data;
	gchar        *catalog_path;

	catalog_path = catalog_list_get_selected_path (window->catalog_list);
	if (catalog_path == NULL)
		return;

	dlg_move_to_catalog_directory (window, catalog_path);

	/* catalog_path is deallocated when the dialog is closed. */
}


void
activate_action_edit_catalog_edit_search (GtkAction *action,
					  gpointer   data)
{
	GThumbWindow *window = data;
	char         *catalog_path;
	void (*module) (GThumbWindow *window, const char *catalog_path);

	catalog_path = catalog_list_get_selected_path (window->catalog_list);
	if (catalog_path == NULL)
		return;

	if (gthumb_module_get ("dlg_catalog_edit_search", (gpointer*) &module))
		(*module) (window, catalog_path);

	g_free (catalog_path);
}


void
activate_action_edit_catalog_redo_search (GtkAction *action,
					  gpointer   data)
{
	GThumbWindow *window = data;
	char         *catalog_path;
	void (*module) (GThumbWindow *window, const char *catalog_path);

	catalog_path = catalog_list_get_selected_path (window->catalog_list);
	if (catalog_path == NULL)
		return;

	if (gthumb_module_get ("dlg_catalog_search", (gpointer*) &module))
		(*module) (window, catalog_path);

	g_free (catalog_path);
}


void
activate_action_edit_current_catalog_new (GtkAction *action,
					  gpointer   data)
{
	GThumbWindow *window = data;
	char         *new_name;
	char         *new_name_utf8;
	char         *new_catalog_path;
	int           fd;

	if (window->catalog_list->path == NULL)
		return;

	new_name_utf8 = _gtk_request_dialog_run (GTK_WINDOW (window->app),
						 GTK_DIALOG_MODAL,
						 _("Enter the catalog name: "),
						 _("New Catalog"),
						 MAX_NAME_LEN,
						 GTK_STOCK_CANCEL,
						 GTK_STOCK_OK);
	if (new_name_utf8 == NULL) 
		return;

	new_name = g_filename_from_utf8 (new_name_utf8, -1, 0, 0, 0);
	g_free (new_name_utf8);

	if (strchr (new_name, '/') != NULL) {
		char *utf8_name;

		utf8_name = g_filename_to_utf8 (new_name, -1, NULL, NULL, NULL);
		_gtk_error_dialog_run (GTK_WINDOW (window->app),
				       _("The name \"%s\" is not valid because it contains the character \"/\". " "Please use a different name."), utf8_name);

		g_free (utf8_name);
		g_free (new_name);

		return;
	}

	new_catalog_path = g_strconcat (window->catalog_list->path,
					"/",
					new_name,
					CATALOG_EXT,
					NULL);

	if (path_is_file (new_catalog_path)) {
		char *utf8_name;

		utf8_name = g_filename_to_utf8 (new_name, -1, NULL, NULL, NULL);
		_gtk_error_dialog_run (GTK_WINDOW (window->app), 
				       _("The name \"%s\" is already used. " "Please use a different name."), utf8_name);
		g_free (utf8_name);

	} else if ((fd = creat (new_catalog_path, 0660)) != -1) {
		all_windows_notify_catalog_new (new_catalog_path);
		close (fd);
	} else {
		char *utf8_name;

		utf8_name = g_filename_to_utf8 (new_name, -1, NULL, NULL, NULL);
		_gtk_error_dialog_run (GTK_WINDOW (window->app), 
                                       _("Could not create the catalog \"%s\": %s"), 
                                       utf8_name,
                                       errno_to_string ());
		g_free (utf8_name);
	}

	g_free (new_name); 	
	g_free (new_catalog_path);
}


static void
create_new_folder_or_library (GThumbWindow *window,
			      char         *str_old_name,
			      char         *str_prompt,
			      char         *str_error)
{
	const char   *current_path;
	char         *new_name, *new_name_utf8;
	char         *new_path;

	if (window->sidebar_content == GTH_SIDEBAR_DIR_LIST)
		current_path = window->dir_list->path;
	else if (window->sidebar_content == GTH_SIDEBAR_CATALOG_LIST)
		current_path = window->catalog_list->path;
	else
		return;

	new_name_utf8 = _gtk_request_dialog_run (GTK_WINDOW (window->app),
						 GTK_DIALOG_MODAL,
						 str_prompt,
						 str_old_name,
						 MAX_NAME_LEN,
						 GTK_STOCK_CANCEL,
						 _("C_reate"));
	
	if (new_name_utf8 == NULL) 
		return;

	new_name = g_filename_from_utf8 (new_name_utf8, -1, 0, 0, 0);
	g_free (new_name_utf8);

	if (strchr (new_name, '/') != NULL) {
		char *utf8_name;

		utf8_name = g_filename_to_utf8 (new_name, -1, NULL, NULL, NULL);
		_gtk_error_dialog_run (GTK_WINDOW (window->app),
				       _("The name \"%s\" is not valid because it contains the character \"/\". " "Please use a different name."), utf8_name);
		g_free (utf8_name);
		g_free (new_name);

		return;
	}

	/* Create folder */

	new_path = g_build_path ("/", current_path, new_name, NULL);
	
	if (path_is_dir (new_path)) {
		char *utf8_name;

		utf8_name = g_filename_to_utf8 (new_name, -1, NULL, NULL, NULL);
		_gtk_error_dialog_run (GTK_WINDOW (window->app),
				       _("The name \"%s\" is already used. " "Please use a different name."), utf8_name);
		g_free (utf8_name);

	} else if (mkdir (new_path, 0755) == 0) { 
	} else {
		char *utf8_path;

		utf8_path = g_filename_to_utf8 (new_path, -1, NULL, NULL, NULL);
		_gtk_error_dialog_run (GTK_WINDOW (window->app),
                                       str_error,
                                       utf8_path,
                                       errno_to_string ());
		g_free (utf8_path);
	}
	all_windows_notify_directory_new (new_path);
	
	g_free (new_path);
	g_free (new_name);
}


void
activate_action_edit_current_catalog_new_library (GtkAction *action,
						  gpointer   data)
{
	GThumbWindow *window = data;
	create_new_folder_or_library (window, 
				      _("New Library"),
				      _("Enter the library name: "),
				      _("Could not create the library \"%s\": %s"));
}


void
activate_action_edit_current_catalog_rename (GtkAction *action,
					     gpointer   data)
{
	GThumbWindow *window = data;

	if (window->catalog_path == NULL)
		return;	
	catalog_rename (window, window->catalog_path);
}


void
activate_action_edit_current_catalog_delete (GtkAction *action,
					     gpointer   data)
{
	GThumbWindow *window = data;

	if (window->catalog_path == NULL)
		return;	
	catalog_delete (window, window->catalog_path);
}


void
activate_action_edit_current_catalog_move (GtkAction *action,
					   gpointer   data)
{
	GThumbWindow *window = data;
	gchar        *catalog_path;
	
	if (window->catalog_path == NULL)
		return;	

	catalog_path = g_strdup (window->catalog_path);
	dlg_move_to_catalog_directory (window, catalog_path);

	/* catalog_path is deallocated when the dialog is closed. */
}


void
activate_action_edit_current_catalog_edit_search (GtkAction *action,
						  gpointer   data)
{
	GThumbWindow *window = data;
	void (*module) (GThumbWindow *window, const char *catalog_path);

	if (window->catalog_path == NULL)
		return;	

	if (gthumb_module_get ("dlg_catalog_edit_search", (gpointer*) &module))
		(*module) (window, window->catalog_path);
}


void
activate_action_edit_current_catalog_redo_search (GtkAction *action,
						  gpointer   data)
{
	GThumbWindow *window = data;
	void (*module) (GThumbWindow *window, const char *catalog_path);

	if (window->catalog_path == NULL)
		return;	

	if (gthumb_module_get ("dlg_catalog_search", (gpointer*) &module))
		(*module) (window, window->catalog_path);
}


void
activate_action_edit_dir_view (GtkAction *action,
			       gpointer   data)
{
	GThumbWindow  *window = data;
	char          *path;

	path = dir_list_get_selected_path (window->dir_list);
	if (path == NULL) 
		return;

	window_go_to_directory (window, path);

	g_free (path);
}


void
activate_action_edit_dir_view_new_window (GtkAction *action,
					  gpointer   data)
{
	GThumbWindow  *window = data;
	char          *path;
	char          *location;

	path = dir_list_get_selected_path (window->dir_list);
	if (path == NULL) 
		return;

	location = g_strconcat ("file://", path, NULL);
	open_new_window_at_location (window, location);
	g_free (location);

	g_free (path);
}


static void
show_folder (GThumbWindow *window,
	     const char   *path)
{
	GError *err = NULL;
	char   *uri;

	uri = g_strconcat ("file://", path, NULL);
	if (! gnome_url_show (uri, &err))
		_gtk_error_dialog_from_gerror_run (GTK_WINDOW (window->app),
						   &err);
	g_free (uri);
}


void
activate_action_edit_dir_open (GtkAction *action,
			       gpointer   data)
{
	GThumbWindow  *window = data;
	char          *path;

	path = dir_list_get_selected_path (window->dir_list);
	if (path == NULL) 
		return;

	show_folder (window, path);
	g_free (path);
}


static void
folder_rename (GThumbWindow *window,
	       const char   *old_path)
{
	const char   *old_name;
	char         *old_name_utf8;
	char         *new_name;
	char         *new_name_utf8;
	char         *new_path;
	char         *parent_path;

	old_name = file_name_from_path (old_path);
	old_name_utf8 = g_filename_to_utf8 (old_name, -1, NULL, NULL, NULL);

	new_name_utf8 = _gtk_request_dialog_run (GTK_WINDOW (window->app),
						 GTK_DIALOG_MODAL,
						 _("Enter the new name: "),
						 old_name_utf8,
						 MAX_NAME_LEN,
						 GTK_STOCK_CANCEL,
						 _("_Rename"));
	g_free (old_name_utf8);
	
	if (new_name_utf8 == NULL) 
		return;

	new_name = g_filename_from_utf8 (new_name_utf8, -1, 0, 0, 0);
	g_free (new_name_utf8);

	if (strchr (new_name, '/') != NULL) {
		char *utf8_name;

		utf8_name = g_filename_to_utf8 (new_name, -1, NULL, NULL, NULL);
		_gtk_error_dialog_run (GTK_WINDOW (window->app),
				       _("The name \"%s\" is not valid because it contains the character \"/\". " "Please use a different name."), utf8_name);
		g_free (utf8_name);
		g_free (new_name);

		return;
	}

	/* Rename */

	parent_path = remove_level_from_path (old_path);
	new_path = g_build_path ("/", parent_path, new_name, NULL);
	g_free (parent_path);

	all_windows_remove_monitor ();
	
	if (strcmp (old_path, new_path) == 0) {
		char *utf8_path;
		
		utf8_path = g_filename_to_utf8 (old_path, -1, NULL, NULL, NULL);
		_gtk_error_dialog_run (GTK_WINDOW (window->app),
				       _("Could not rename the folder \"%s\": %s"),
				       utf8_path,
				       _("source and destination are the same"));
		g_free (utf8_path);

	} else if (path_is_dir (new_path)) {
		char *utf8_name;

		utf8_name = g_filename_to_utf8 (new_name, -1, NULL, NULL, NULL);
		_gtk_error_dialog_run (GTK_WINDOW (window->app),
				       _("The name \"%s\" is already used. " "Please use a different name."), utf8_name);
		g_free (utf8_name);

	} else {
		char *old_folder_comment;


		old_folder_comment = comments_get_comment_filename (old_path, TRUE, TRUE);

		if (rename (old_path, new_path) == 0) { 
			char *new_folder_comment;

			/* Comment cache. */

			new_folder_comment = comments_get_comment_filename (new_path, TRUE, TRUE);
			rename (old_folder_comment, new_folder_comment);
			g_free (new_folder_comment);
			
			all_windows_notify_directory_rename (old_path, new_path);

		} else {
			char *utf8_path;
			utf8_path = g_filename_to_utf8 (old_path, -1, NULL, NULL, NULL);
			_gtk_error_dialog_run (GTK_WINDOW (window->app),
					       _("Could not rename the folder \"%s\": %s"),
					       utf8_path,
					       errno_to_string ());
			g_free (utf8_path);
		}

		g_free (old_folder_comment);
	}

	all_windows_add_monitor ();

	g_free (new_path);
	g_free (new_name); 		
}


void
activate_action_edit_dir_rename (GtkAction *action,
				 gpointer   data)
{
	GThumbWindow *window = data;
	char         *old_path;

	old_path = dir_list_get_selected_path (window->dir_list);
	if (old_path == NULL)
		return;

	folder_rename (window, old_path);

	g_free (old_path);
}


/* -- folder delete -- */


typedef struct {
	GThumbWindow *window;
	char         *path;
} FolderDeleteData;


static void
folder_delete__continue2 (GnomeVFSResult result,
			  gpointer       data)
{
	FolderDeleteData *fddata = data;

	if (result != GNOME_VFS_OK) {
		const char *message;
		char       *utf8_name;
		
		message = _("Could not delete the folder \"%s\": %s");
		utf8_name = g_filename_to_utf8 (file_name_from_path (fddata->path), -1, 0, 0, 0);

		_gtk_error_dialog_run (GTK_WINDOW (fddata->window->app),
				       message, 
				       utf8_name, 
				       gnome_vfs_result_to_string (result));
		g_free (utf8_name);

	}

	g_free (fddata->path);
	g_free (fddata);	
}


static void
folder_delete__continue (GnomeVFSResult result,
			 gpointer       data)
{
	FolderDeleteData *fddata = data;

	if (result == GNOME_VFS_ERROR_NOT_FOUND) {
		int r = GTK_RESPONSE_YES;

		if (eel_gconf_get_boolean (PREF_MSG_CANNOT_MOVE_TO_TRASH, TRUE)) {
			GtkWidget *d;
			char      *utf8_name;
			char      *message;

			utf8_name = g_filename_to_utf8 (file_name_from_path (fddata->path), -1, 0, 0, 0);
			message = g_strdup_printf (_("\"%s\" cannot be moved to the Trash. Do you want to delete it permanently?"), utf8_name);

			d = _gtk_yesno_dialog_with_checkbutton_new (
				    GTK_WINDOW (fddata->window->app),
				    GTK_DIALOG_MODAL,
				    message,
				    GTK_STOCK_CANCEL,
				    GTK_STOCK_DELETE,
				    _("_Do not display this message again"),
				    PREF_MSG_CANNOT_MOVE_TO_TRASH);

			g_free (message);
			g_free (utf8_name);
		
			r = gtk_dialog_run (GTK_DIALOG (d));
			gtk_widget_destroy (GTK_WIDGET (d));
		} 

		if (r == GTK_RESPONSE_YES) 
			dlg_folder_delete (fddata->window, 
					   fddata->path, 
					   folder_delete__continue2, 
					   fddata);

	} else 
		folder_delete__continue2 (result, data);
}


static void
folder_delete (GThumbWindow *window,
	       const char   *path)
{
	FolderDeleteData *fddata;
	GtkWidget        *dialog;
	int               r;

	/* Always ask before deleting folders. */

	dialog = _gtk_yesno_dialog_new (GTK_WINDOW (window->app),
					GTK_DIALOG_MODAL,
					_("The selected folder will be moved to the Trash, are you sure?"),
					GTK_STOCK_CANCEL,
					_("_Move"));


	r = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (GTK_WIDGET (dialog));

	if (r != GTK_RESPONSE_YES) 
		return;

	/* Delete */

	fddata = g_new0 (FolderDeleteData, 1);
	fddata->window = window;
	fddata->path = g_strdup (path);

	dlg_folder_move_to_trash (window, 
				  path, 
				  folder_delete__continue, 
				  fddata);
}


void
activate_action_edit_dir_delete (GtkAction *action,
				 gpointer   data)
{
	GThumbWindow *window = data;
	char         *path;

	path = dir_list_get_selected_path (window->dir_list);
	if (path == NULL)
		return;

	folder_delete (window, path);

	g_free (path);
}


static void
folder_copy__continue (GnomeVFSResult result,
		       gpointer       data)
{
	char *path = data;

	if (result != GNOME_VFS_OK) {
		const char *message;
		char       *utf8_name;
		
		message = _("Could not copy the folder \"%s\": %s");
		utf8_name = g_filename_to_utf8 (file_name_from_path (path), -1, 0, 0, 0);
		
		_gtk_error_dialog_run (NULL,
				       message, 
				       utf8_name, 
				       gnome_vfs_result_to_string (result));
		g_free (utf8_name);
	}

	g_free (path);
}


static void
folder_copy__response_cb (GObject *object,
			  int      response_id,
			  gpointer data)
{
	GtkWidget    *file_sel = data;
	GThumbWindow *window;
	char         *old_path;
	char         *dest_dir;
	char         *new_path;
	const char   *dir_name;
	gboolean      same_fs;
	gboolean      move;
	const char   *message;

	if (response_id != GTK_RESPONSE_OK) {
		gtk_widget_destroy (file_sel);
		return;
	}

	window = g_object_get_data (G_OBJECT (file_sel), "gthumb_window");
	old_path = g_object_get_data (G_OBJECT (file_sel), "path");
	dest_dir = gth_folder_selection_get_folder (GTH_FOLDER_SELECTION (file_sel));
	move = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (file_sel), "folder_op"));

	if (dest_dir == NULL)
		return;

	if (! dlg_check_folder (window, dest_dir)) {
		g_free (dest_dir);
		return;
	}

	gtk_widget_hide (file_sel);

	dir_name = file_name_from_path (old_path);
	new_path = g_build_path ("/", dest_dir, dir_name, NULL);
	 
	if (gnome_vfs_check_same_fs (old_path, dest_dir, &same_fs) != GNOME_VFS_OK)
		same_fs = FALSE;

	message = move ? _("Could not move the folder \"%s\": %s") : _("Could not copy the folder \"%s\": %s");

	if (strcmp (old_path, new_path) == 0) {
		char *utf8_path;
		
		utf8_path = g_filename_to_utf8 (old_path, -1, NULL, NULL, NULL);

		_gtk_error_dialog_run (GTK_WINDOW (window->app),
				       message,
				       utf8_path,
				       _("source and destination are the same"));
		g_free (utf8_path);

	} else if (path_in_path (old_path, new_path)) {
		char *utf8_path;
		
		utf8_path = g_filename_to_utf8 (old_path, -1, NULL, NULL, NULL);

		_gtk_error_dialog_run (GTK_WINDOW (window->app),
				       message,
				       utf8_path,
				       _("source contains destination"));
		g_free (utf8_path);

	} else if (path_is_dir (new_path)) {
		char *utf8_name;

		utf8_name = g_filename_to_utf8 (dir_name, -1, NULL, NULL, NULL);

		_gtk_error_dialog_run (GTK_WINDOW (window->app),
				       message,
				       utf8_name,
				       _("a folder with that name is already present."));
		g_free (utf8_name);

	} else if (! (move && same_fs)) {
		dlg_folder_copy (window, 
				 old_path, 
				 new_path, 
				 move, 
				 TRUE,
				 FALSE,
				 folder_copy__continue, 
				 g_strdup (old_path));

	} else {
		char *old_folder_comment = NULL;

		/* Comment cache. */
			
		old_folder_comment= comments_get_comment_filename (old_path, TRUE, TRUE);

		if (rename (old_path, new_path) == 0) { 
			char *new_folder_comment;

			/* moving folders on the same file system can be 
			 * implemeted with rename, which is faster. */

			new_folder_comment = comments_get_comment_filename (new_path, TRUE, TRUE);
			rename (old_folder_comment, new_folder_comment);
			g_free (new_folder_comment);

			all_windows_notify_directory_rename (old_path, new_path);

		} else {
			char *utf8_path;

			utf8_path = g_filename_to_utf8 (old_path, -1, NULL, NULL, NULL);
			_gtk_error_dialog_run (GTK_WINDOW (window->app),
					       message,
					       utf8_path,
					       errno_to_string ());
			g_free (utf8_path);
		}

		g_free (old_folder_comment);
	}

	g_free (dest_dir);
	g_free (new_path);
	gtk_widget_destroy (file_sel);
}


static void
folder_copy__destroy_cb (GObject *object,
			 gpointer data)
{
	GtkWidget *file_sel = data;
	char      *old_path;

	old_path = g_object_get_data (G_OBJECT (file_sel), "path");
	g_free (old_path);
}


static void
folder_copy (GThumbWindow *window,
	     const char   *path,
	     gboolean      move)
{
	GtkWidget *file_sel;
	char      *parent;
	char      *start_from;

	file_sel = gth_folder_selection_new (_("Choose the destination folder"));

	parent = remove_level_from_path (path);
	start_from = g_strconcat (parent, "/", NULL);
	g_free (parent);

	gth_folder_selection_set_folder (GTH_FOLDER_SELECTION (file_sel), start_from);
	g_free (start_from);

	g_object_set_data (G_OBJECT (file_sel), "path", g_strdup (path));
	g_object_set_data (G_OBJECT (file_sel), "gthumb_window", window);
	g_object_set_data (G_OBJECT (file_sel), "folder_op", GINT_TO_POINTER (move));

	g_signal_connect (G_OBJECT (file_sel),
			  "response", 
			  G_CALLBACK (folder_copy__response_cb), 
			  file_sel);

	g_signal_connect (G_OBJECT (file_sel),
			  "destroy", 
			  G_CALLBACK (folder_copy__destroy_cb),
			  file_sel);

	gtk_window_set_transient_for (GTK_WINDOW (file_sel), GTK_WINDOW (window->app));
	gtk_window_set_modal (GTK_WINDOW (file_sel), TRUE);
	gtk_widget_show (file_sel);
}


void
activate_action_edit_dir_copy (GtkAction *action,
			       gpointer   data)
{
	GThumbWindow *window = data;
	char         *path;

	path = dir_list_get_selected_path (window->dir_list);
	if (path == NULL)
		return;

	folder_copy (window, path, FALSE);

	g_free (path);
}


void
activate_action_edit_dir_move (GtkAction *action,
			       gpointer   data)
{
	GThumbWindow *window = data;
	char         *path;

	path = dir_list_get_selected_path (window->dir_list);
	if (path == NULL)
		return;

	folder_copy (window, path, TRUE);

	g_free (path);
}


/**/

typedef struct {
	GThumbWindow  *window;
	GList         *file_list;
	GList         *add_list, *remove_list;
} FolderCategoriesData;


static void
edit_current_folder_categories__done (gpointer data)
{
	FolderCategoriesData *fcdata = data;
	GList                *scan;
	
	for (scan = fcdata->file_list; scan; scan = scan->next) {
		const char  *filename = scan->data;
		CommentData *cdata;
		GList       *scan2;

		cdata = comments_load_comment (filename);
		if (cdata == NULL)
			cdata = comment_data_new ();
		else 
			for (scan2 = fcdata->remove_list; scan2; scan2 = scan2->next) {
				const char *k = scan2->data;
				comment_data_remove_keyword (cdata, k);
			}

		for (scan2 = fcdata->add_list; scan2; scan2 = scan2->next) {
			const char *k = scan2->data;
			comment_data_add_keyword (cdata, k);
		}

		comments_save_categories (filename, cdata);
		comment_data_free (cdata);

		if (path_in_path (fcdata->window->dir_list->path, filename)) {
			dir_list_remove_directory (fcdata->window->dir_list, filename);
			dir_list_add_directory (fcdata->window->dir_list, filename);
		}
	}

	path_list_free (fcdata->file_list);
	path_list_free (fcdata->add_list);
	path_list_free (fcdata->remove_list);
	g_free (fcdata);
}


static void 
edit_current_folder_categories (GThumbWindow  *window,
				const char    *path)
{
	FolderCategoriesData *fcdata;

	fcdata = g_new0 (FolderCategoriesData, 1);
	fcdata->window = window;
	fcdata->add_list = NULL;
	fcdata->remove_list = NULL;
	fcdata->file_list = g_list_prepend (NULL, g_strdup (path));

	dlg_choose_categories (GTK_WINDOW (window->app),
			       fcdata->file_list,
			       NULL,
			       &(fcdata->add_list),
			       &(fcdata->remove_list),
			       edit_current_folder_categories__done,
			       fcdata);
}


void
activate_action_edit_dir_categories (GtkAction *action,
				     gpointer   data)
{
	GThumbWindow *window = data;
	char         *path;

	path = dir_list_get_selected_path (window->dir_list);
	if (path == NULL)
		return;

	edit_current_folder_categories (window, path);

	g_free (path);
}


void
activate_action_edit_current_dir_open (GtkAction *action,
				       gpointer   data)
{
	GThumbWindow  *window = data;
	char          *path;

	path = window->dir_list->path;
	if (path == NULL) 
		return;
	show_folder (window, path);
}


void
activate_action_edit_current_dir_rename (GtkAction *action,
					 gpointer   data)
{
	GThumbWindow *window = data;
	char         *old_path;

	old_path = window->dir_list->path;
	if (old_path == NULL) 
		return;

	folder_rename (window, old_path);
}


void
activate_action_edit_current_dir_delete (GtkAction *action,
					 gpointer   data)
{
	GThumbWindow *window = data;
	char         *path;

	path = window->dir_list->path;
	if (path == NULL)
		return;

	folder_delete (window, path);
}


void
activate_action_edit_current_dir_copy (GtkAction *action,
				       gpointer   data)
{
	GThumbWindow *window = data;
	char         *path;
	
	path = window->dir_list->path;
	if (path == NULL)
		return;

	folder_copy (window, path, FALSE);
}


void
activate_action_edit_current_dir_move (GtkAction *action,
				       gpointer   data)
{
	GThumbWindow *window = data;
	char         *path;
	
	path = window->dir_list->path;
	if (path == NULL)
		return;

	folder_copy (window, path, TRUE);
}


void
activate_action_edit_current_dir_categories (GtkAction *action,
					     gpointer   data)
{
	GThumbWindow         *window = data;
	char                 *path;

	path = window->dir_list->path;
	if (path == NULL)
		return;

	edit_current_folder_categories (window, path);
}


void
activate_action_edit_current_dir_new (GtkAction *action,
				      gpointer   data)
{
	GThumbWindow *window = data;
	create_new_folder_or_library (window, 
				      _("New Folder"),
				      _("Enter the folder name: "),
				      _("Could not create the folder \"%s\": %s"));
}


void
activate_action_alter_image_rotate90 (GtkAction *action,
				      gpointer   data)
{
	GThumbWindow *window = data;
	ImageViewer  *viewer = IMAGE_VIEWER (window->viewer);
	GdkPixbuf    *src_pixbuf;
	GdkPixbuf    *dest_pixbuf;

	src_pixbuf = image_viewer_get_current_pixbuf (viewer);
	dest_pixbuf = _gdk_pixbuf_copy_rotate_90 (src_pixbuf, FALSE);
	image_viewer_set_pixbuf (viewer, dest_pixbuf);
	g_object_unref (dest_pixbuf);

	window_image_set_modified (window, TRUE);
}


void
activate_action_alter_image_rotate90cc (GtkAction *action,
					gpointer   data)
{
	GThumbWindow *window = data;
	ImageViewer  *viewer = IMAGE_VIEWER (window->viewer);
	GdkPixbuf    *src_pixbuf;
	GdkPixbuf    *dest_pixbuf;

	src_pixbuf = image_viewer_get_current_pixbuf (viewer);
	dest_pixbuf = _gdk_pixbuf_copy_rotate_90 (src_pixbuf, TRUE);
	image_viewer_set_pixbuf (viewer, dest_pixbuf);
	g_object_unref (dest_pixbuf);

	window_image_set_modified (window, TRUE);
}


void
activate_action_alter_image_flip (GtkAction *action,
				  gpointer   data)
{
	GThumbWindow *window = data;
	ImageViewer  *viewer = IMAGE_VIEWER (window->viewer);
	GdkPixbuf    *src_pixbuf;
	GdkPixbuf    *dest_pixbuf;

	src_pixbuf = image_viewer_get_current_pixbuf (viewer);
	dest_pixbuf = _gdk_pixbuf_copy_mirror (src_pixbuf, FALSE, TRUE);
	image_viewer_set_pixbuf (viewer, dest_pixbuf);
	g_object_unref (dest_pixbuf);

	window_image_set_modified (window, TRUE);
}


void
activate_action_alter_image_mirror (GtkAction *action,
				    gpointer   data)
{
	GThumbWindow *window = data;
	ImageViewer  *viewer = IMAGE_VIEWER (window->viewer);
	GdkPixbuf    *src_pixbuf;
	GdkPixbuf    *dest_pixbuf;

	src_pixbuf = image_viewer_get_current_pixbuf (viewer);
	dest_pixbuf = _gdk_pixbuf_copy_mirror (src_pixbuf, TRUE, FALSE);
	image_viewer_set_pixbuf (viewer, dest_pixbuf);
	g_object_unref (dest_pixbuf);

	window_image_set_modified (window, TRUE);
}


void
activate_action_alter_image_desaturate (GtkAction *action,
					gpointer   data)
{
	GThumbWindow *window = data;
	ImageViewer  *viewer = IMAGE_VIEWER (window->viewer);
	GdkPixbuf    *src_pixbuf;
	GdkPixbuf    *dest_pixbuf;
	GthPixbufOp  *pixop;
	
	src_pixbuf = image_viewer_get_current_pixbuf (viewer);
	dest_pixbuf = gdk_pixbuf_copy (src_pixbuf);
	pixop = _gdk_pixbuf_desaturate (dest_pixbuf, dest_pixbuf);
	g_object_unref (dest_pixbuf);

	window_exec_pixbuf_op (window, pixop);
	g_object_unref (pixop);
}


void
activate_action_alter_image_invert (GtkAction *action,
				    gpointer   data)
{
	GThumbWindow *window = data;
	ImageViewer  *viewer = IMAGE_VIEWER (window->viewer);
	GdkPixbuf    *src_pixbuf;
	GdkPixbuf    *dest_pixbuf;
	GthPixbufOp  *pixop;

	src_pixbuf = image_viewer_get_current_pixbuf (viewer);
	dest_pixbuf = gdk_pixbuf_copy (src_pixbuf);
	pixop = _gdk_pixbuf_invert (dest_pixbuf, dest_pixbuf);
	g_object_unref (dest_pixbuf);

	window_exec_pixbuf_op (window, pixop);
	g_object_unref (pixop);
}


void
activate_action_alter_image_adjust_levels (GtkAction *action,
					   gpointer   data)
{
	GThumbWindow *window = data;
	ImageViewer  *viewer = IMAGE_VIEWER (window->viewer);
	GdkPixbuf    *src_pixbuf;
	GdkPixbuf    *dest_pixbuf;
	GthPixbufOp  *pixop;

	src_pixbuf = image_viewer_get_current_pixbuf (viewer);
	dest_pixbuf = gdk_pixbuf_copy (src_pixbuf);
	pixop = _gdk_pixbuf_adjust_levels (dest_pixbuf, dest_pixbuf);
	g_object_unref (dest_pixbuf);

	window_exec_pixbuf_op (window, pixop);
	g_object_unref (pixop);
}


void
activate_action_alter_image_equalize (GtkAction *action,
				      gpointer   data)
{
	GThumbWindow *window = data;
	ImageViewer  *viewer = IMAGE_VIEWER (window->viewer);
	GdkPixbuf    *src_pixbuf;
	GdkPixbuf    *dest_pixbuf;
	GthPixbufOp  *pixop;

	src_pixbuf = image_viewer_get_current_pixbuf (viewer);
	dest_pixbuf = gdk_pixbuf_copy (src_pixbuf);
	pixop = _gdk_pixbuf_eq_histogram (dest_pixbuf, dest_pixbuf);
	g_object_unref (dest_pixbuf);

	window_exec_pixbuf_op (window, pixop);
	g_object_unref (pixop);
}


void
activate_action_alter_image_normalize (GtkAction *action,
				       gpointer   data)
{
	GThumbWindow *window = data;
	ImageViewer  *viewer = IMAGE_VIEWER (window->viewer);
	GdkPixbuf    *src_pixbuf;
	GdkPixbuf    *dest_pixbuf;
	GthPixbufOp  *pixop;

	src_pixbuf = image_viewer_get_current_pixbuf (viewer);
	dest_pixbuf = gdk_pixbuf_copy (src_pixbuf);
	pixop = _gdk_pixbuf_normalize_contrast (dest_pixbuf, dest_pixbuf);
	g_object_unref (dest_pixbuf);

	window_exec_pixbuf_op (window, pixop);
	g_object_unref (pixop);
}


void
activate_action_alter_image_stretch_contrast (GtkAction *action,
					      gpointer   data)
{
	GThumbWindow *window = data;
	ImageViewer  *viewer = IMAGE_VIEWER (window->viewer);
	GdkPixbuf    *src_pixbuf;
	GdkPixbuf    *dest_pixbuf;
	GthPixbufOp  *pixop;

	src_pixbuf = image_viewer_get_current_pixbuf (viewer);
	dest_pixbuf = gdk_pixbuf_copy (src_pixbuf);
	pixop = _gdk_pixbuf_stretch_contrast (dest_pixbuf, dest_pixbuf);
	g_object_unref (dest_pixbuf);

	window_exec_pixbuf_op (window, pixop);
	g_object_unref (pixop);
}


void
activate_action_alter_image_posterize (GtkAction *action,
				       gpointer   data)
{
	GThumbWindow *window = data;
	dlg_posterize (window);
}


void
activate_action_alter_image_brightness_contrast (GtkAction *action,
						 gpointer   data)
{
	GThumbWindow *window = data;
	dlg_brightness_contrast (window);
}


void
activate_action_alter_image_hue_saturation (GtkAction *action,
					    gpointer   data)
{
	GThumbWindow *window = data;
	dlg_hue_saturation (window);
}


void
activate_action_alter_image_color_balance (GtkAction *action,
					   gpointer   data)
{
	GThumbWindow *window = data;
	dlg_color_balance (window);
}


void
activate_action_alter_image_threshold (GtkAction *action,
				       gpointer   data)
{
}


void
activate_action_alter_image_resize (GtkAction *action,
				    gpointer   data)
{
	GThumbWindow *window = data;
	dlg_scale_image (window);
}


void
activate_action_alter_image_rotate (GtkAction *action,
				    gpointer   data)
{
}


void
activate_action_alter_image_crop (GtkAction *action,
				  gpointer   data)
{
	GThumbWindow *window = data;
	dlg_crop (window);
}


void
activate_action_view_next_image (GtkAction *action,
				 gpointer   data)
{
	GThumbWindow *window = data;
	window_show_next_image (window, FALSE);	
}


void
activate_action_view_prev_image (GtkAction *action,
				 gpointer   data)
{
	GThumbWindow *window = data;
	window_show_prev_image (window, FALSE);
}


void
activate_action_view_zoom_in (GtkAction *action,
			      gpointer   data)
{
	GThumbWindow *window = data;
	image_viewer_zoom_in (IMAGE_VIEWER (window->viewer));
}


void
activate_action_view_zoom_out (GtkAction *action,
			       gpointer   data)
{
	GThumbWindow *window = data;
	image_viewer_zoom_out (IMAGE_VIEWER (window->viewer));
}


void
activate_action_view_zoom_100 (GtkAction *action,
			       gpointer   data)
{
	GThumbWindow *window = data;
	image_viewer_set_zoom (IMAGE_VIEWER (window->viewer), 1.0);
}


void
activate_action_view_zoom_fit (GtkAction *action,
			       gpointer   data)
{
	GThumbWindow *window = data;
	ImageViewer  *viewer = IMAGE_VIEWER (window->viewer);

	if (viewer->zoom_fit) 
		viewer->zoom_fit = FALSE;
	else 
		image_viewer_zoom_to_fit (viewer);
}


void
activate_action_view_step_animation (GtkAction *action,
				     gpointer   data)
{
	GThumbWindow *window = data;
	ImageViewer *viewer = IMAGE_VIEWER (window->viewer);

	if (! viewer->is_animation)
		return;

	image_viewer_step_animation (viewer);
}


void
activate_action_view_fullscreen (GtkAction *action,
				 gpointer   data)
{
	GThumbWindow *window = data;

	if (! window->fullscreen)
		fullscreen_start (fullscreen, window);
	else
		fullscreen_stop (fullscreen);
}


void
activate_action_view_exit_fullscreen (GtkAction *action,
				      gpointer   data)
{
	GThumbWindow *window = data;

	if (window->fullscreen)
		fullscreen_stop (fullscreen);
}


void
activate_action_view_image_prop (GtkAction *action,
				 gpointer   data)
{
	GThumbWindow *window = data;
	window_show_image_prop (window);
}


void
activate_action_go_back (GtkAction *action,
			 gpointer   data)
{
	GThumbWindow *window = data;
	window_go_back (window);
}


void
activate_action_go_forward (GtkAction *action,
			    gpointer   data)
{
	GThumbWindow *window = data;
	window_go_forward (window);
}


void
activate_action_go_up (GtkAction *action,
		       gpointer   data)
{
	GThumbWindow *window = data;
	window_go_up (window);
}


void
activate_action_go_refresh (GtkAction *action,
			    gpointer   data)
{
	GThumbWindow *window = data;
	window_refresh (window);
}


void
activate_action_go_stop (GtkAction *action,
			 gpointer   data)
{
	GThumbWindow *window = data;
	window_stop_loading (window);
}


void
activate_action_go_home (GtkAction *action,
			 gpointer   data)
{
	GThumbWindow *window = data;
	window_go_to_directory (window, g_get_home_dir ());
}


void
activate_action_go_to_container (GtkAction *action,
				 gpointer   data)
{
	GThumbWindow *window = data;
	GList        *list;
	char         *path;

	list = gth_file_view_get_file_list_selection (window->file_list->view);

	g_return_if_fail (list != NULL);

	path = remove_level_from_path (list->data);
	path_list_free (list);
	window_go_to_directory (window, path);
	g_free (path);
}


void
activate_action_go_delete_history (GtkAction *action,
				   gpointer   data)
{
	GThumbWindow *window = data;
	window_delete_history (window);
}


void
activate_action_go_location (GtkAction *action,
			     gpointer   data)
{
	GThumbWindow *window = data;
	if (! window->sidebar_visible)
		window_show_sidebar (window);
	gtk_widget_grab_focus (window->location_entry);
}


void
activate_action_bookmarks_add (GtkAction *action,
			       gpointer   data)
{
	GThumbWindow *window = data;
	char         *path = NULL;

	if (window->sidebar_content == GTH_SIDEBAR_CATALOG_LIST) {
		GtkTreeIter  iter;
		char        *prefix, *catalog_path;

		if (! catalog_list_get_selected_iter (window->catalog_list,
						      &iter))
			return;
		if (catalog_list_is_search (window->catalog_list, &iter))
			prefix = g_strdup (SEARCH_PREFIX);
		else
			prefix = g_strdup (CATALOG_PREFIX);
		catalog_path = catalog_list_get_path_from_iter (window->catalog_list, &iter);

		path = g_strconcat (prefix,
				    catalog_path,
				    NULL);
		g_free (catalog_path);
		g_free (prefix);
	} else {
		if (window->dir_list->path == NULL)
			return;

		path = g_strconcat (FILE_PREFIX,
				    window->dir_list->path,
				    NULL);
	}

	bookmarks_add (preferences.bookmarks, path, TRUE, TRUE);
	bookmarks_write_to_disk (preferences.bookmarks);
	all_windows_update_bookmark_list ();

	g_free (path);
}


void
activate_action_bookmarks_edit (GtkAction *action,
				gpointer   data)
{
	GThumbWindow *window = data;
	dlg_edit_bookmarks (window);
}


static void
set_as_wallpaper (GThumbWindow   *window,
		  const gchar    *image_path,
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
activate_action_wallpaper_centered (GtkAction *action,
				    gpointer   data)
{
	GThumbWindow *window = data;
	set_as_wallpaper (window, window->image_path, WALLPAPER_ALIGN_CENTERED);
}


void
activate_action_wallpaper_tiled (GtkAction *action,
				 gpointer   data)
{
	GThumbWindow *window = data;
	set_as_wallpaper (window, window->image_path, WALLPAPER_ALIGN_TILED);
}


void
activate_action_wallpaper_scaled (GtkAction *action,
				  gpointer   data)
{
	GThumbWindow *window = data;
	set_as_wallpaper (window, window->image_path, WALLPAPER_ALIGN_SCALED);
}


void
activate_action_wallpaper_stretched (GtkAction *action,
				     gpointer   data)
{
	GThumbWindow *window = data;
	set_as_wallpaper (window, window->image_path, WALLPAPER_ALIGN_STRETCHED);
}


void
activate_action_wallpaper_restore (GtkAction *action,
				   gpointer   data)
{
	GThumbWindow *window = data;
	int           align_type = WALLPAPER_ALIGN_CENTERED;

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

	set_as_wallpaper (window, 
			  preferences.wallpaper,
			  align_type);
}


void
activate_action_tools_slideshow (GtkAction *action,
				 gpointer   data)
{
	GThumbWindow *window = data;
	
	if (! window->slideshow)
		window_start_slideshow (window);
	else
		window_stop_slideshow (window);
}


void
activate_action_tools_find_images (GtkAction *action,
				   gpointer   data)
{
	GThumbWindow *window = data;
	void (*module) (GtkWidget *widget, GThumbWindow *window);

	if (gthumb_module_get ("dlg_search", (gpointer*) &module))
		(*module) (NULL, window);
}


void
activate_action_tools_index_image (GtkAction *action,
				   gpointer   data)
{
	GThumbWindow *window = data;
	void (*module) (GThumbWindow *window);

	if (gthumb_module_get ("dlg_exporter", (gpointer*) &module))
		(*module) (window);
}


void
activate_action_tools_web_exporter (GtkAction *action,
				    gpointer   data)
{
	GThumbWindow *window = data;
	void (*module) (GThumbWindow *window);

	if (gthumb_module_get ("dlg_web_exporter", (gpointer*) &module))
		(*module) (window);
}


void
activate_action_tools_rename_series (GtkAction *action,
				     gpointer   data)
{
	GThumbWindow *window = data;
	dlg_rename_series (window);
}


void
activate_action_tools_convert_format (GtkAction *action,
				      gpointer   data)
{
        GThumbWindow *window = data;
        dlg_convert (window);
}


void
activate_action_tools_find_duplicates (GtkAction *action,
				       gpointer   data)
{
        GThumbWindow *window = data;
	void (*module) (GThumbWindow *window);

	if (gthumb_module_get ("dlg_duplicates", (gpointer*) &module))
		(*module) (window);
}


void
activate_action_tools_jpeg_rotate (GtkAction *action,
				   gpointer   data)
{
	GThumbWindow *window = data;
	void (*module) (GThumbWindow *window);

	if (gthumb_module_get ("dlg_jpegtran", (gpointer*) &module))
		(*module) (window);
}


void
activate_action_tools_preferences (GtkAction *action,
				   gpointer   data)
{
	GThumbWindow *window = data;
	dlg_preferences (window);
}


void
activate_action_tools_change_date (GtkAction *action,
				   gpointer   data)
{
        GThumbWindow *window = data;
        dlg_change_date (window);
}


void
activate_action_help_about (GtkAction *action,
			    gpointer   data)
{
	GThumbWindow      *window = data;
	static GtkWidget  *about;
	GdkPixbuf         *logo;
	const char        *authors[] = {
		"Paolo Bacchilega <paolo.bacchilega@libero.it>",
		NULL
	};
	const char       *documenters [] = {
		"Paolo Bacchilega",
		"Alexander Kirillov", 
		NULL
	};
	const char       *translator_credits = _("translator_credits");

	if (about != NULL) {
		gdk_window_show (about->window);
		gdk_window_raise (about->window);
		return;
	}

	logo = gdk_pixbuf_new_from_file (PIXMAPSDIR "/gthumb.png", NULL);
	about = gnome_about_new (_("gThumb"), 
				 VERSION,
				 "Copyright \xc2\xa9 2001-2004 Free Software Foundation, Inc.",
				 _("An image viewer and browser for GNOME."),
				 authors,
				 documenters,
				 strcmp (translator_credits, "translator_credits") != 0 ? translator_credits : NULL,
				 logo);
	if (logo != NULL)
                g_object_unref (logo);

	gtk_window_set_destroy_with_parent (GTK_WINDOW (about), TRUE);
	gtk_window_set_transient_for (GTK_WINDOW (about),
				      GTK_WINDOW (window->app));

	g_signal_connect (G_OBJECT (about), 
			  "destroy",
			  G_CALLBACK (gtk_widget_destroyed), 
			  &about);

	gtk_widget_show_all (about);
}


static void
display_help (GThumbWindow *window,
	      const char   *section) 
{
	GError *err;

	err = NULL;  
	gnome_help_display ("gthumb", section, &err);
	
	if (err != NULL) {
		GtkWidget *dialog;
		
		dialog = gtk_message_dialog_new (GTK_WINDOW (window->app),
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
activate_action_help_help (GtkAction *action,
			   gpointer   data)
{
	display_help ((GThumbWindow *) data, NULL);
}


void
activate_action_help_shortcuts (GtkAction *action,
				gpointer   data)
{
	display_help ((GThumbWindow *) data, "shortcuts");
}


void
activate_action_view_toolbar (GtkAction *action,
			      gpointer   data)
{
	eel_gconf_set_boolean (PREF_UI_TOOLBAR_VISIBLE, gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)));
}


void
activate_action_view_statusbar (GtkAction *action,
				gpointer   data)
{
	eel_gconf_set_boolean (PREF_UI_STATUSBAR_VISIBLE, gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)));
}


void
activate_action_view_thumbnails (GtkAction *action,
				 gpointer   data)
{
	eel_gconf_set_boolean (PREF_SHOW_THUMBNAILS, gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)));
}


static void 
set_action_sensitive (GThumbWindow *window,
		      const char   *action_name,
		      gboolean      sensitive)
{
	GtkAction *action;
	action = gtk_action_group_get_action (window->actions, action_name);
	if (action == NULL) 
		g_print ("%s NON ESISTE\n", action_name);
	else
		g_object_set (action, "sensitive", sensitive, NULL);
}


void
activate_action_view_play_animation (GtkAction *action,
				     gpointer   data)
{
	GThumbWindow *window = data;
	ImageViewer  *viewer = IMAGE_VIEWER (window->viewer);
	
	if (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)))
		image_viewer_start_animation (viewer);
	else
		image_viewer_stop_animation (viewer);

	set_action_sensitive (window, "View_StepAnimation", 
			      (image_viewer_is_animation (viewer) 
			       && ! image_viewer_is_playing_animation (viewer)));
}


void
activate_action_view_show_preview (GtkAction *action,
				   gpointer   data)
{
	GThumbWindow *window = data;

	if (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)))
		window_show_image_pane (window);
	else
		window_hide_image_pane (window);
}


void
activate_action_view_show_info (GtkAction *action,
				gpointer   data)
{
	GThumbWindow *window = data;

	if (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)))
		window_show_image_data (window);
	else
		window_hide_image_data (window);
}


void
activate_action_sort_reversed (GtkAction *action,
			       gpointer   data)
{
	GThumbWindow *window = data;
	GtkSortType   new_type;

	if (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)))
		new_type = GTK_SORT_DESCENDING;
	else
		new_type = GTK_SORT_ASCENDING;
	gth_file_list_set_sort_type (window->file_list, new_type);
}
