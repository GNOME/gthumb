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
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <gconf/gconf-client.h>
#include <libgnome/gnome-exec.h>
#include <libgnome/gnome-help.h>
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
#include "dlg-maintenance.h"
#include "dlg-open-with.h"
#include "dlg-posterize.h"
#include "dlg-color-balance.h"
#include "dlg-preferences.h"
#include "dlg-rename-series.h"
#include "dlg-save-image.h"
#include "dlg-scale-image.h"
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

typedef enum {
	WALLPAPER_ALIGN_TILED     = 0,
	WALLPAPER_ALIGN_CENTERED  = 1,
	WALLPAPER_ALIGN_STRETCHED = 2,
	WALLPAPER_ALIGN_SCALED    = 3
} WallpaperAlign;


void 
file_new_window_command_impl (BonoboUIComponent *uic, 
			      gpointer           user_data, 
			      const char        *verbname)
{
	GThumbWindow *window = user_data;
	ImageViewer  *viewer = IMAGE_VIEWER (window->viewer);
	GThumbWindow *new_window;
	char         *location = NULL;
	int           width, height;

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

	if (location != NULL) {
		eel_gconf_set_locale_string (PREF_STARTUP_LOCATION, location);
		g_free (location);
	}

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

	eel_gconf_set_boolean (PREF_UI_IMAGE_PANE_VISIBLE, window->image_preview_visible);

	/**/

	new_window = window_new ();
	gtk_widget_show (new_window->app);
}


void 
file_close_window_command_impl (BonoboUIComponent *uic, 
				gpointer           user_data, 
				const char        *verbname)
{
	GThumbWindow *window = user_data;
	window_close (window);
}


void 
file_open_with_command_impl (BonoboUIComponent *uic, 
			     gpointer           user_data, 
			     const char        *verbname)
{
	GThumbWindow *window = user_data;
	GList        *list;

	list = gth_file_view_get_file_list_selection (window->file_list->view);
	g_return_if_fail (list != NULL);

	open_with_cb (window, list);

	/* the list is deallocated when the dialog is closed. */
}


void 
image_open_with_command_impl (BonoboUIComponent *uic, 
			      gpointer           user_data, 
			      const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	GList        *list;

	if (window->image_path == NULL)
		return;

	list = g_list_prepend (NULL, g_strdup (window->image_path));
	open_with_cb (window, list);

	/* the list is deallocated when the dialog is closed. */
}


void 
file_save_command_impl (BonoboUIComponent *uic, 
			gpointer           user_data, 
			const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	char         *current_folder = NULL;

	if (window->image_path != NULL)
		current_folder = g_strdup (window->image_path);

	else if (window->dir_list->path != NULL)
		current_folder = g_strconcat (window->dir_list->path,
					      "/", 
					      NULL);

	dlg_save_image (GTK_WINDOW (window->app), 
			current_folder,
			image_viewer_get_current_pixbuf (IMAGE_VIEWER (window->viewer)));

	g_free (current_folder);
}


void 
file_revert_command_impl (BonoboUIComponent *uic, 
			  gpointer           user_data, 
			  const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	window_reload_image (window);
}


void 
file_print_command_impl (BonoboUIComponent *uic, 
			 gpointer           user_data, 
			 const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	char         *uri;

	if (window->image_path == NULL)
		return;

	uri = g_strconcat ("file://", window->image_path, NULL);
	print_image_dlg (GTK_WINDOW (window->app), 
			 IMAGE_VIEWER (window->viewer),
			 uri);
	g_free (uri);
}


void 
file_exit_command_impl (BonoboUIComponent *uic, 
			gpointer           user_data, 
			const gchar       *verbname)
{
	GThumbWindow *first_window = window_list->data;

	ExitAll = TRUE;
	window_close (first_window);
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
	old_name_utf8 = g_locale_to_utf8 (old_name, -1, NULL, NULL, NULL);

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

	new_name = g_locale_from_utf8 (new_name_utf8, -1, 0, 0, 0);
	g_free (new_name_utf8);

	if (strchr (new_name, '/') != NULL) {
		char *utf8_name;

		utf8_name = g_locale_to_utf8 (new_name, -1, NULL, NULL, NULL);
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

		utf8_name = g_locale_to_utf8 (new_name, -1, NULL, NULL, NULL);
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

		utf8_path = g_locale_to_utf8 (old_path, -1, NULL, NULL, NULL);
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

		utf8_path = g_locale_to_utf8 (old_path, -1, NULL, NULL, NULL);
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
edit_rename_file_command_impl (BonoboUIComponent *uic, 
			       gpointer           user_data, 
			       const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	GList        *list;

	list = gth_file_view_get_file_list_selection (window->file_list->view);
	g_return_if_fail (list != NULL);

	rename_file (window, list);
	path_list_free (list);
}


void 
image_rename_command_impl (BonoboUIComponent *uic, 
			   gpointer           user_data, 
			   const gchar       *verbname)
{
	GThumbWindow *window = user_data;
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
		cache_copy (old_path, new_path);
		comment_copy (old_path, new_path);

	} else {
		char *utf8_path;

		utf8_path = g_locale_to_utf8 (old_path, -1, NULL, NULL, NULL);
		_gtk_error_dialog_run (GTK_WINDOW (window->app),
				       _("Could not duplicate the image \"%s\": %s"),
				       utf8_path,
				       errno_to_string ());
		g_free (utf8_path);
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
edit_duplicate_file_command_impl (BonoboUIComponent *uic, 
				  gpointer           user_data, 
				  const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	GList        *list;

	list = gth_file_view_get_file_list_selection (window->file_list->view);
	g_return_if_fail (list != NULL);

	duplicate_file_list (window, list);
	path_list_free (list);
}


void 
image_duplicate_command_impl (BonoboUIComponent *uic, 
			      gpointer           user_data, 
			      const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	GList        *list;

	if (window->image_path == NULL)
		return;

	list = g_list_prepend (NULL, g_strdup (window->image_path));
	duplicate_file_list (window, list);
	path_list_free (list);
}


void 
edit_delete_files_command_impl (BonoboUIComponent *uic, 
				gpointer           user_data, 
				const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	GList        *list;

	list = gth_file_view_get_file_list_selection (window->file_list->view);
	dlg_file_delete__confirm (window, 
				  list, 
				  _("The selected images will be moved to the Trash, are you sure?"));

	/* the list is deallocated when the dialog is closed. */
}


void 
image_delete_command_impl (BonoboUIComponent *uic, 
			   gpointer           user_data, 
			   const gchar       *verbname)
{
	GThumbWindow *window = user_data;
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
edit_copy_files_command_impl (BonoboUIComponent *uic, 
			      gpointer           user_data, 
			      const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	GList        *list;

	list = gth_file_view_get_file_list_selection (window->file_list->view);
	dlg_file_copy__ask_dest (window, list);

	/* the list is deallocated when the dialog is closed. */
}


void 
image_copy_command_impl (BonoboUIComponent *uic, 
			 gpointer           user_data, 
			 const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	GList        *list;

	if (window->image_path == NULL)
		return;

	list = g_list_prepend (NULL, g_strdup (window->image_path));
	dlg_file_copy__ask_dest (window, list);

	/* the list is deallocated when the dialog is closed. */
}


void 
edit_move_files_command_impl (BonoboUIComponent *uic, 
			      gpointer           user_data, 
			      const gchar       *verbname)
{
	GThumbWindow  *window = user_data;
	GList         *list;

	list = gth_file_view_get_file_list_selection (window->file_list->view);
	dlg_file_move__ask_dest (window, list);

	/* the list is deallocated when the dialog is closed. */
}


void 
image_move_command_impl (BonoboUIComponent *uic, 
			 gpointer           user_data, 
			 const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	GList        *list;

	if (window->image_path == NULL)
		return;

	list = g_list_prepend (NULL, g_strdup (window->image_path));
	dlg_file_move__ask_dest (window, list);

	/* the list is deallocated when the dialog is closed. */
}


void 
edit_select_all_command_impl (BonoboUIComponent *uic, 
			      gpointer           user_data, 
			      const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	gth_file_view_select_all (window->file_list->view);
}


void 
edit_edit_comment_command_impl (BonoboUIComponent *uic, 
				gpointer           user_data, 
				const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	dlg_edit_comment (NULL, window);
}


void 
edit_current_edit_comment_command_impl (BonoboUIComponent *uic, 
					gpointer           user_data, 
					const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	int           pos;

	if (window->image_path == NULL)
		return;

	pos = gth_file_list_pos_from_path (window->file_list, window->image_path);
	if (pos != -1) {
		gth_file_list_select_image_by_pos (window->file_list, pos);
		dlg_edit_comment (NULL, window);
	}
}


void 
edit_delete_comment_command_impl (BonoboUIComponent *uic, 
				  gpointer           user_data, 
				  const gchar       *verbname)
{
	GThumbWindow *window = user_data;
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
}


void 
edit_edit_categories_command_impl (BonoboUIComponent *uic, 
				   gpointer           user_data, 
				   const gchar       *verbname)
{
	GThumbWindow *window = user_data;

	dlg_categories (NULL, window);
}

void 
edit_current_edit_categories_command_impl (BonoboUIComponent *uic, 
					   gpointer           user_data, 
					   const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	int           pos;

	if (window->image_path == NULL)
		return;

	pos = gth_file_list_pos_from_path (window->file_list, window->image_path);
	if (pos != -1) {
		gth_file_list_select_image_by_pos (window->file_list, pos);
		dlg_categories (NULL, window);
	}
}


void 
edit_add_to_catalog_command_impl (BonoboUIComponent *uic, 
				  gpointer           user_data, 
				  const gchar       *verbname)
{
	GThumbWindow *window = user_data;
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


void 
image_delete_from_catalog_command_impl (BonoboUIComponent *uic, 
					gpointer           user_data, 
					const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	GList        *list;
	GtkWidget    *dialog;
	gint          r;

	if (window->image_path == NULL)
		return;

	if (! eel_gconf_get_boolean (PREF_CONFIRM_DELETION)) {
		list = g_list_prepend (NULL, g_strdup (window->image_path));
		real_remove_from_catalog (window, list);
		/* the list is deallocated in real_remove_from_catalog. */
		return;
	}

	dialog = _gtk_yesno_dialog_new (GTK_WINDOW (window->app),
					GTK_DIALOG_MODAL,
					_("The image will be removed from the catalog, are you sure?"),
					GTK_STOCK_CANCEL,
					GTK_STOCK_REMOVE);

	r = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (GTK_WIDGET (dialog));
	if (r == GTK_RESPONSE_YES) {
		list = g_list_prepend (NULL, g_strdup (window->image_path));
		real_remove_from_catalog (window, list);
		/* the list is deallocated in real_remove_from_catalog. */
	}
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
edit_remove_from_catalog_command_impl (BonoboUIComponent *uic, 
				       gpointer           user_data, 
				       const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	GtkWidget    *dialog;
	int           r;

	if (! eel_gconf_get_boolean (PREF_CONFIRM_DELETION)) {
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
folder_open (GThumbWindow *window,
	     const char   *viewer,
	     const char   *path,
	     const char   *working_dir) 
{
	GError *err = NULL;
	char   *argv[5];

	argv[0] = (char*) viewer;
	argv[1] = (char*) path;
	argv[2] = NULL;
	
	if (! g_spawn_sync (working_dir, 
			    argv, NULL, 
			    G_SPAWN_SEARCH_PATH, 
			    NULL, NULL, 
			    NULL, NULL,
			    NULL, &err))
		_gtk_error_dialog_from_gerror_run (GTK_WINDOW (window->app),
						   &err);
}


void 
edit_folder_open_nautilus_command_impl (BonoboUIComponent *uic, 
					gpointer           user_data, 
					const gchar       *verbname)
{
	GThumbWindow  *window = user_data;
	char          *path;

	path = dir_list_get_selected_path (window->dir_list);
	if (path == NULL) 
		return;

	folder_open (window, "nautilus", path, NULL);

	g_free (path);
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

	new_name = g_locale_from_utf8 (new_name_utf8, -1, 0, 0, 0);
	g_free (new_name_utf8);

	if (strchr (new_name, '/') != NULL) {
		char *utf8_name;

		utf8_name = g_locale_to_utf8 (new_name, -1, NULL, NULL, NULL);
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

		utf8_name = g_locale_to_utf8 (new_name, -1, NULL, NULL, NULL);
		_gtk_error_dialog_run (GTK_WINDOW (window->app),
				       _("The name \"%s\" is already used. " "Please use a different name."), utf8_name);
		g_free (utf8_name);

	} else if (mkdir (new_path, 0755) == 0) { 
	} else {
		char *utf8_path;

		utf8_path = g_locale_to_utf8 (new_path, -1, NULL, NULL, NULL);
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
edit_current_folder_new_command_impl (BonoboUIComponent *uic, 
				      gpointer           user_data, 
				      const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	create_new_folder_or_library (window, 
				      _("New Folder"),
				      _("Enter the folder name: "),
				      _("Could not create the folder \"%s\": %s"));
}


void 
edit_current_catalog_new_library_command_impl (BonoboUIComponent *uic, 
					       gpointer           user_data, 
					       const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	create_new_folder_or_library (window, 
				      _("New Library"),
				      _("Enter the library name: "),
				      _("Could not create the library \"%s\": %s"));
}


void 
edit_current_folder_open_nautilus_command_impl (BonoboUIComponent *uic, 
						gpointer           user_data, 
						const gchar       *verbname)
{
	GThumbWindow  *window = user_data;
	char          *path;

	path = window->dir_list->path;
	if (path == NULL) 
		return;
	folder_open (window, "nautilus", path, NULL);
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
	old_name_utf8 = g_locale_to_utf8 (old_name, -1, NULL, NULL, NULL);

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

	new_name = g_locale_from_utf8 (new_name_utf8, -1, 0, 0, 0);
	g_free (new_name_utf8);

	if (strchr (new_name, '/') != NULL) {
		char *utf8_name;

		utf8_name = g_locale_to_utf8 (new_name, -1, NULL, NULL, NULL);
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
		
		utf8_path = g_locale_to_utf8 (old_path, -1, NULL, NULL, NULL);
		_gtk_error_dialog_run (GTK_WINDOW (window->app),
				       _("Could not rename the folder \"%s\": %s"),
				       utf8_path,
				       _("source and destination are the same"));
		g_free (utf8_path);

	} else if (path_is_dir (new_path)) {
		char *utf8_name;

		utf8_name = g_locale_to_utf8 (new_name, -1, NULL, NULL, NULL);
		_gtk_error_dialog_run (GTK_WINDOW (window->app),
				       _("The name \"%s\" is already used. " "Please use a different name."), utf8_name);
		g_free (utf8_name);

	} else if (rename (old_path, new_path) == 0) { 
		char *old_cache_path, *new_cache_path;

		/* Update cache info. */

		/* Comment cache. */

		old_cache_path = comments_get_comment_dir (old_path, TRUE);
		if (! path_is_dir (old_cache_path)) {
			g_free (old_cache_path);
			old_cache_path = comments_get_comment_dir (old_path, FALSE);
		}
		new_cache_path = comments_get_comment_dir (new_path, TRUE);
		rename (old_cache_path, new_cache_path);
		g_free (old_cache_path);
		g_free (new_cache_path);

		all_windows_notify_directory_rename (old_path, new_path);

	} else {
		char *utf8_path;

		utf8_path = g_locale_to_utf8 (old_path, -1, NULL, NULL, NULL);
		_gtk_error_dialog_run (GTK_WINDOW (window->app),
                                       _("Could not rename the folder \"%s\": %s"),
                                       utf8_path,
                                       errno_to_string ());
		g_free (utf8_path);
	}

	all_windows_add_monitor ();

	g_free (new_path);
	g_free (new_name); 		
}


void 
edit_folder_rename_command_impl (BonoboUIComponent *uic, 
				 gpointer           user_data, 
				 const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	char         *old_path;

	old_path = dir_list_get_selected_path (window->dir_list);
	if (old_path == NULL)
		return;

	folder_rename (window, old_path);

	g_free (old_path);
}


void 
edit_current_folder_rename_command_impl (BonoboUIComponent *uic, 
					 gpointer           user_data, 
					 const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	char         *old_path;

	old_path = window->dir_list->path;
	if (old_path == NULL) 
		return;

	folder_rename (window, old_path);
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
		utf8_name = g_locale_to_utf8 (file_name_from_path (fddata->path), -1, 0, 0, 0);

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
		GtkWidget *d;
		char      *utf8_name;
		char      *message;
		int        r;

		utf8_name = g_locale_to_utf8 (file_name_from_path (fddata->path), -1, 0, 0, 0);
		message = g_strdup_printf (_("\"%s\" cannot be moved to the Trash. Do you want to delete it permanently?"), utf8_name);

		d = _gtk_yesno_dialog_new (GTK_WINDOW (fddata->window->app),
					   GTK_DIALOG_MODAL,
					   message,
					   GTK_STOCK_CANCEL,
					   GTK_STOCK_DELETE);

		g_free (message);
		g_free (utf8_name);

		r = gtk_dialog_run (GTK_DIALOG (d));
		gtk_widget_destroy (GTK_WIDGET (d));

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
edit_folder_delete_command_impl (BonoboUIComponent *uic, 
				 gpointer           user_data, 
				 const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	char         *path;

	path = dir_list_get_selected_path (window->dir_list);
	if (path == NULL)
		return;

	folder_delete (window, path);

	g_free (path);
}


void 
edit_current_folder_delete_command_impl (BonoboUIComponent *uic, 
					 gpointer           user_data, 
					 const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	char         *path;

	path = window->dir_list->path;
	if (path == NULL)
		return;

	folder_delete (window, path);
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
		utf8_name = g_locale_to_utf8 (file_name_from_path (path), -1, 0, 0, 0);
		
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
		
		utf8_path = g_locale_to_utf8 (old_path, -1, NULL, NULL, NULL);

		_gtk_error_dialog_run (GTK_WINDOW (window->app),
				       message,
				       utf8_path,
				       _("source and destination are the same"));
		g_free (utf8_path);

	} else if (strncmp (old_path, new_path, strlen (old_path)) == 0) {
		char *utf8_path;
		
		utf8_path = g_locale_to_utf8 (old_path, -1, NULL, NULL, NULL);

		_gtk_error_dialog_run (GTK_WINDOW (window->app),
				       message,
				       utf8_path,
				       _("source contains destination"));
		g_free (utf8_path);

	} else if (path_is_dir (new_path)) {
		char *utf8_name;

		utf8_name = g_locale_to_utf8 (dir_name, -1, NULL, NULL, NULL);

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

	} else if (rename (old_path, new_path) == 0) { 
		char *old_cache_path, *new_cache_path;

		/* moving folders on the same file system can be implemeted 
		 * with rename, which is faster. */

		/* Comment cache. */

		old_cache_path = comments_get_comment_dir (old_path, TRUE);
		if (! path_is_dir (old_cache_path)) {
			g_free (old_cache_path);
			old_cache_path = comments_get_comment_dir (old_path, FALSE);
		}
		new_cache_path = comments_get_comment_dir (new_path, TRUE);

		if (path_is_dir (old_cache_path)) {
			char *parent_dir;

			parent_dir = remove_level_from_path (new_cache_path);
			ensure_dir_exists (parent_dir, 0755);
			g_free (parent_dir);

			rename (old_cache_path, new_cache_path);
		}

		g_free (old_cache_path);
		g_free (new_cache_path);

		all_windows_notify_directory_rename (old_path, new_path);

	} else {
		char *utf8_path;

		utf8_path = g_locale_to_utf8 (old_path, -1, NULL, NULL, NULL);
		_gtk_error_dialog_run (GTK_WINDOW (window->app),
				       message,
                                       utf8_path,
                                       errno_to_string ());
		g_free (utf8_path);
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
edit_folder_move_command_impl (BonoboUIComponent *uic, 
			       gpointer           user_data, 
			       const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	char         *path;

	path = dir_list_get_selected_path (window->dir_list);
	if (path == NULL)
		return;

	folder_copy (window, path, TRUE);

	g_free (path);
}


void 
edit_current_folder_move_command_impl (BonoboUIComponent *uic, 
				       gpointer           user_data, 
				       const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	char         *path;
	
	path = window->dir_list->path;
	if (path == NULL)
		return;

	folder_copy (window, path, TRUE);
}


void 
edit_folder_copy_command_impl (BonoboUIComponent *uic, 
			       gpointer           user_data, 
			       const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	char         *path;

	path = dir_list_get_selected_path (window->dir_list);
	if (path == NULL)
		return;

	folder_copy (window, path, FALSE);

	g_free (path);
}


void 
edit_current_folder_copy_command_impl (BonoboUIComponent *uic, 
				       gpointer           user_data, 
				       const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	char         *path;
	
	path = window->dir_list->path;
	if (path == NULL)
		return;

	folder_copy (window, path, FALSE);
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
	name_only_utf8 = g_locale_to_utf8 (name_only, -1, NULL, NULL, NULL);

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

	new_name = g_locale_from_utf8 (new_name_utf8, -1, 0, 0, 0);
	g_free (new_name_utf8);

	if (strchr (new_name, '/') != NULL) {
		char *utf8_name;

		utf8_name = g_locale_to_utf8 (new_name, -1, NULL, NULL, NULL);
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

		utf8_name = g_locale_to_utf8 (new_name, -1, NULL, NULL, NULL);
		_gtk_error_dialog_run (GTK_WINDOW (window->app), 
				       _("The name \"%s\" is already used. " "Please use a different name."), utf8_name);
		g_free (utf8_name);

	} else if (! rename (catalog_path, new_catalog_path)) {
		all_windows_notify_catalog_rename (catalog_path, 
						   new_catalog_path);
	} else {
		char *utf8_name;

		utf8_name = g_locale_to_utf8 (name_only, -1, NULL, NULL, NULL);
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
edit_catalog_rename_command_impl (BonoboUIComponent *uic, 
				  gpointer           user_data, 
				  const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	char         *catalog_path;

	catalog_path = catalog_list_get_selected_path (window->catalog_list);
	if (catalog_path == NULL)
		return;	

	catalog_rename (window, catalog_path);

	g_free (catalog_path);
}


void 
edit_current_catalog_new_command_impl (BonoboUIComponent *uic, 
				       gpointer           user_data, 
				       const char        *verbname)
{
	GThumbWindow *window = user_data;
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

	new_name = g_locale_from_utf8 (new_name_utf8, -1, 0, 0, 0);
	g_free (new_name_utf8);

	if (strchr (new_name, '/') != NULL) {
		char *utf8_name;

		utf8_name = g_locale_to_utf8 (new_name, -1, NULL, NULL, NULL);
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

		utf8_name = g_locale_to_utf8 (new_name, -1, NULL, NULL, NULL);
		_gtk_error_dialog_run (GTK_WINDOW (window->app), 
				       _("The name \"%s\" is already used. " "Please use a different name."), utf8_name);
		g_free (utf8_name);

	} else if ((fd = creat (new_catalog_path, 0660)) != -1) {
		all_windows_notify_catalog_new (new_catalog_path);
		close (fd);
	} else {
		char *utf8_name;

		utf8_name = g_locale_to_utf8 (new_name, -1, NULL, NULL, NULL);
		_gtk_error_dialog_run (GTK_WINDOW (window->app), 
                                       _("Could not create the catalog \"%s\": %s"), 
                                       utf8_name,
                                       errno_to_string ());
		g_free (utf8_name);
	}

	g_free (new_name); 	
	g_free (new_catalog_path);
}


void 
edit_current_catalog_rename_command_impl (BonoboUIComponent *uic, 
					  gpointer           user_data, 
					  const gchar       *verbname)
{
	GThumbWindow *window = user_data;

	if (window->catalog_path == NULL)
		return;	
	catalog_rename (window, window->catalog_path);
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

	if (! path_is_dir (catalog_path) && ! eel_gconf_get_boolean (PREF_CONFIRM_DELETION)) {
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
edit_catalog_delete_command_impl (BonoboUIComponent *uic, 
				  gpointer           user_data, 
				  const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	gchar        *catalog_path;

	catalog_path = catalog_list_get_selected_path (window->catalog_list);
	if (catalog_path == NULL)
		return;

	catalog_delete (window, catalog_path);

	g_free (catalog_path);
}


void 
edit_current_catalog_delete_command_impl (BonoboUIComponent *uic, 
					  gpointer           user_data, 
					  const gchar       *verbname)
{
	GThumbWindow *window = user_data;

	if (window->catalog_path == NULL)
		return;	
	catalog_delete (window, window->catalog_path);
}


void 
edit_catalog_move_command_impl (BonoboUIComponent *uic, 
				gpointer           user_data, 
				const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	gchar        *catalog_path;

	catalog_path = catalog_list_get_selected_path (window->catalog_list);
	if (catalog_path == NULL)
		return;

	dlg_move_to_catalog_directory (window, catalog_path);

	/* catalog_path is deallocated when the dialog is closed. */
}


void 
edit_current_catalog_move_command_impl (BonoboUIComponent *uic, 
					gpointer           user_data, 
					const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	gchar        *catalog_path;
	
	if (window->catalog_path == NULL)
		return;	

	catalog_path = g_strdup (window->catalog_path);
	dlg_move_to_catalog_directory (window, catalog_path);

	/* catalog_path is deallocated when the dialog is closed. */
}


void 
edit_catalog_edit_search_command_impl (BonoboUIComponent *uic, 
				       gpointer           user_data, 
				       const gchar       *verbname)
{
	GThumbWindow *window = user_data;
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
edit_current_catalog_edit_search_command_impl (BonoboUIComponent *uic, 
					       gpointer           user_data, 
					       const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	void (*module) (GThumbWindow *window, const char *catalog_path);

	if (window->catalog_path == NULL)
		return;	

	if (gthumb_module_get ("dlg_catalog_edit_search", (gpointer*) &module))
		(*module) (window, window->catalog_path);
}


void 
edit_catalog_redo_search_command_impl (BonoboUIComponent *uic, 
				       gpointer           user_data, 
				       const gchar       *verbname)
{
	GThumbWindow *window = user_data;
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
edit_current_catalog_redo_search_command_impl (BonoboUIComponent *uic, 
					       gpointer           user_data, 
					       const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	void (*module) (GThumbWindow *window, const char *catalog_path);

	if (window->catalog_path == NULL)
		return;	

	if (gthumb_module_get ("dlg_catalog_search", (gpointer*) &module))
		(*module) (window, window->catalog_path);
}


void 
alter_image_rotate_command_impl (BonoboUIComponent *uic, 
				 gpointer           user_data, 
				 const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	ImageViewer  *viewer = IMAGE_VIEWER (window->viewer);
	GdkPixbuf    *src_pixbuf;
	GdkPixbuf    *dest_pixbuf;

	src_pixbuf = image_viewer_get_current_pixbuf (viewer);
	dest_pixbuf = _gdk_pixbuf_copy_rotate_90 (src_pixbuf, FALSE);
	image_viewer_set_pixbuf (viewer, dest_pixbuf);
	g_object_unref (dest_pixbuf);

	window_image_modified (window, TRUE);
}


void 
alter_image_rotate_cc_command_impl (BonoboUIComponent *uic, 
				    gpointer           user_data, 
				    const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	ImageViewer  *viewer = IMAGE_VIEWER (window->viewer);
	GdkPixbuf    *src_pixbuf;
	GdkPixbuf    *dest_pixbuf;

	src_pixbuf = image_viewer_get_current_pixbuf (viewer);
	dest_pixbuf = _gdk_pixbuf_copy_rotate_90 (src_pixbuf, TRUE);
	image_viewer_set_pixbuf (viewer, dest_pixbuf);
	g_object_unref (dest_pixbuf);

	window_image_modified (window, TRUE);
}


void 
alter_image_flip_command_impl (BonoboUIComponent *uic, 
			       gpointer           user_data, 
			       const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	ImageViewer  *viewer = IMAGE_VIEWER (window->viewer);
	GdkPixbuf    *src_pixbuf;
	GdkPixbuf    *dest_pixbuf;

	src_pixbuf = image_viewer_get_current_pixbuf (viewer);
	dest_pixbuf = _gdk_pixbuf_copy_mirror (src_pixbuf, FALSE, TRUE);
	image_viewer_set_pixbuf (viewer, dest_pixbuf);
	g_object_unref (dest_pixbuf);

	window_image_modified (window, TRUE);
}


void 
alter_image_mirror_command_impl (BonoboUIComponent *uic, 
				 gpointer           user_data, 
				 const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	ImageViewer  *viewer = IMAGE_VIEWER (window->viewer);
	GdkPixbuf    *src_pixbuf;
	GdkPixbuf    *dest_pixbuf;

	src_pixbuf = image_viewer_get_current_pixbuf (viewer);
	dest_pixbuf = _gdk_pixbuf_copy_mirror (src_pixbuf, TRUE, FALSE);
	image_viewer_set_pixbuf (viewer, dest_pixbuf);
	g_object_unref (dest_pixbuf);

	window_image_modified (window, TRUE);
}


void 
alter_image_desaturate_command_impl (BonoboUIComponent *uic, 
				     gpointer           user_data, 
				     const gchar       *verbname)
{
	GThumbWindow *window = user_data;
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
alter_image_invert_command_impl (BonoboUIComponent *uic, 
				 gpointer           user_data, 
				 const gchar       *verbname)
{
	GThumbWindow *window = user_data;
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
alter_image_equalize_command_impl (BonoboUIComponent *uic, 
				   gpointer           user_data, 
				   const gchar       *verbname)
{
	GThumbWindow *window = user_data;
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
alter_image_adjust_levels_command_impl (BonoboUIComponent *uic, 
					gpointer           user_data, 
					const gchar       *verbname)
{
	GThumbWindow *window = user_data;
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
alter_image_posterize_command_impl (BonoboUIComponent *uic, 
				    gpointer           user_data, 
				    const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	dlg_posterize (window);
}


void 
alter_image_brightness_contrast_command_impl (BonoboUIComponent *uic, 
					      gpointer           user_data, 
					      const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	dlg_brightness_contrast (window);
}


void 
alter_image_hue_saturation_command_impl (BonoboUIComponent *uic, 
					 gpointer           user_data, 
					 const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	dlg_hue_saturation (window);
}


void 
alter_image_color_balance_command_impl (BonoboUIComponent *uic, 
					gpointer           user_data, 
					const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	dlg_color_balance (window);
}


void 
alter_image_scale_command_impl (BonoboUIComponent *uic, 
				gpointer           user_data, 
				const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	dlg_scale_image (window);
}


void 
view_zoom_in_command_impl (BonoboUIComponent *uic, 
			   gpointer           user_data, 
			   const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	image_viewer_zoom_in (IMAGE_VIEWER (window->viewer));
}


void 
view_zoom_out_command_impl (BonoboUIComponent *uic, 
			    gpointer           user_data, 
			    const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	image_viewer_zoom_out (IMAGE_VIEWER (window->viewer));
}


void 
view_zoom_100_command_impl (BonoboUIComponent *uic, 
			    gpointer           user_data, 
			    const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	image_viewer_set_zoom (IMAGE_VIEWER (window->viewer), 1.0);
}


void 
view_zoom_fit_command_impl (BonoboUIComponent *uic, 
			    gpointer           user_data, 
			    const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	ImageViewer  *viewer = IMAGE_VIEWER (window->viewer);

	if (window->freeze_toggle_handler > 0) {
		window->freeze_toggle_handler--;
		return;
	}

	if (viewer->zoom_fit) 
		viewer->zoom_fit = FALSE;
	else 
		image_viewer_zoom_to_fit (viewer);
}


void 
view_step_ani_command_impl (BonoboUIComponent *uic, 
			    gpointer           user_data, 
			    const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	ImageViewer *viewer = IMAGE_VIEWER (window->viewer);

	if (! viewer->is_animation)
		return;

	image_viewer_step_animation (viewer);
}


void 
view_show_folders_command_impl (BonoboUIComponent *uic, 
				gpointer           user_data, 
				const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	
	if (window->freeze_toggle_handler > 0) {
		window->freeze_toggle_handler--;
		return;
	}
	
	if (! GTK_WIDGET_VISIBLE (window->app))
		return;

	if ((window->sidebar_visible) 
	    && (window->sidebar_content == GTH_SIDEBAR_DIR_LIST))
		window_hide_sidebar (window);
	else
		window_set_sidebar_content (window, GTH_SIDEBAR_DIR_LIST);
}


void 
view_show_catalogs_command_impl (BonoboUIComponent *uic, 
				 gpointer           user_data, 
				 const gchar       *verbname)
{
	GThumbWindow *window = user_data;

	if (window->freeze_toggle_handler > 0) {
		window->freeze_toggle_handler--;
		return;
	}

	if (! GTK_WIDGET_VISIBLE (window->app))
		return;

	if ((window->sidebar_visible) 
	    && (window->sidebar_content == GTH_SIDEBAR_CATALOG_LIST))
		window_hide_sidebar (window);
	else
		window_set_sidebar_content (window, GTH_SIDEBAR_CATALOG_LIST);
}


void 
view_fullscreen_command_impl (BonoboUIComponent *uic, 
			      gpointer           user_data, 
			      const gchar       *verbname)
{
	GThumbWindow *window = user_data;

	if (window->freeze_toggle_handler > 0) {
		window->freeze_toggle_handler--;
		return;
	}

	if (! window->fullscreen)
		fullscreen_start (fullscreen, window);
	else
		fullscreen_stop (fullscreen);
}


void 
view_prev_image_command_impl (BonoboUIComponent *uic, 
			      gpointer           user_data, 
			      const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	window_show_prev_image (window, FALSE);
	
}


void 
view_next_image_command_impl (BonoboUIComponent *uic, 
			      gpointer           user_data, 
			      const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	window_show_next_image (window, FALSE);	
}


void 
view_image_prop_command_impl (BonoboUIComponent *uic, 
			      gpointer           user_data, 
			      const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	window_show_image_prop (window);
}


void 
view_sidebar_command_impl (BonoboUIComponent *uic, 
			   gpointer           user_data, 
			   const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	window_show_sidebar (window);
}


void 
go_back_command_impl (BonoboUIComponent *uic, 
		      gpointer           user_data, 
		      const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	window_go_back (window);
}


void 
go_forward_command_impl (BonoboUIComponent *uic, 
			 gpointer           user_data, 
			 const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	window_go_forward (window);
}


void 
go_up_command_impl (BonoboUIComponent *uic, 
		    gpointer           user_data, 
		    const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	window_go_up (window);
}


void 
go_stop_command_impl (BonoboUIComponent *uic, 
		      gpointer           user_data, 
		      const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	window_stop_loading (window);
}


void 
go_refresh_command_impl (BonoboUIComponent *uic, 
			 gpointer           user_data, 
			 const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	window_refresh (window);
}


void 
go_home_command_impl (BonoboUIComponent *uic, 
		      gpointer           user_data, 
		      const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	window_go_to_directory (window, g_get_home_dir ());
}


void 
go_to_container_command_impl (BonoboUIComponent *uic, 
			      gpointer           user_data, 
			      const gchar       *verbname)
{
	GThumbWindow *window = user_data;
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
go_delete_history_command_impl (BonoboUIComponent *uic, 
				gpointer           user_data, 
				const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	window_delete_history (window);
}


void 
go_location_command_impl (BonoboUIComponent *uic, 
			  gpointer           user_data, 
			  const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	if (! window->sidebar_visible)
		window_show_sidebar (window);
	gtk_widget_grab_focus (window->location_entry);
}


void 
bookmarks_add_command_impl (BonoboUIComponent *uic, 
			    gpointer           user_data, 
			    const gchar       *verbname)
{
	GThumbWindow *window = user_data;
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
bookmarks_edit_command_impl (BonoboUIComponent *uic, 
			     gpointer           user_data, 
			     const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	dlg_edit_bookmarks (window);
}


static void
set_as_wallpaper (GThumbWindow   *window,
		  const gchar    *image_path,
		  WallpaperAlign  align)
{
	GConfClient *client;
	char        *options;

	if ((image_path == NULL) || ! path_is_file (image_path))
		return;

	client = gconf_client_get_default ();
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
	}
	gconf_client_set_string (client, 
				 "/desktop/gnome/background/picture_options", 
				 options,
				 NULL);
        g_object_unref (G_OBJECT (client));
}


void 
wallpaper_centered_command_impl (BonoboUIComponent *uic, 
				 gpointer           user_data, 
				 const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	set_as_wallpaper (window, window->image_path, WALLPAPER_ALIGN_CENTERED);
}


void 
wallpaper_tiled_command_impl (BonoboUIComponent *uic, 
			      gpointer           user_data, 
			      const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	set_as_wallpaper (window, window->image_path, WALLPAPER_ALIGN_TILED);
}


void 
wallpaper_scaled_command_impl (BonoboUIComponent *uic, 
			       gpointer           user_data, 
			       const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	set_as_wallpaper (window, window->image_path, WALLPAPER_ALIGN_SCALED);
}


void 
wallpaper_stretched_command_impl (BonoboUIComponent *uic, 
				  gpointer           user_data, 
				  const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	set_as_wallpaper (window, window->image_path, WALLPAPER_ALIGN_STRETCHED);
}


void 
wallpaper_restore_command_impl (BonoboUIComponent *uic, 
				gpointer           user_data, 
				const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	gint align_type;

	if (strcmp (preferences.wallpaperAlign, "wallpaper") == 0)
		align_type = WALLPAPER_ALIGN_TILED;
	else if (strcmp (preferences.wallpaperAlign, "centered") == 0)
		align_type = WALLPAPER_ALIGN_CENTERED;
	else if (strcmp (preferences.wallpaperAlign, "stretched") == 0)
		align_type = WALLPAPER_ALIGN_STRETCHED;
	else if (strcmp (preferences.wallpaperAlign, "scaled") == 0)
		align_type = WALLPAPER_ALIGN_SCALED;

	set_as_wallpaper (window, 
			  preferences.wallpaper,
			  align_type);
}


void 
tools_slideshow_command_impl (BonoboUIComponent *uic, 
			      gpointer           user_data, 
			      const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	
	if (! window->slideshow)
		window_start_slideshow (window);
	else
		window_stop_slideshow (window);
}


void 
tools_find_images_command_impl (BonoboUIComponent *uic, 
				gpointer           user_data, 
				const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	void (*module) (GtkWidget *widget, GThumbWindow *window);

	if (gthumb_module_get ("dlg_search", (gpointer*) &module))
		(*module) (NULL, window);
}


void 
tools_index_image_command_impl (BonoboUIComponent *uic, 
				gpointer           user_data, 
				const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	void (*module) (GThumbWindow *window);

	if (gthumb_module_get ("dlg_exporter", (gpointer*) &module))
		(*module) (window);
}


void 
tools_web_exporter_command_impl (BonoboUIComponent *uic, 
				gpointer           user_data, 
				const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	void (*module) (GThumbWindow *window);

	if (gthumb_module_get ("dlg_web_exporter", (gpointer*) &module))
		(*module) (window);
}


void 
tools_maintenance_command_impl (BonoboUIComponent *uic, 
				gpointer           user_data, 
				const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	dlg_maintenance (window);
}


void 
tools_rename_series_command_impl (BonoboUIComponent *uic, 
				  gpointer           user_data, 
				  const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	dlg_rename_series (window);
}


void 
tools_preferences_command_impl (BonoboUIComponent *uic, 
				gpointer           user_data, 
				const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	dlg_preferences (window);
}


void 
tools_jpeg_rotate_command_impl (BonoboUIComponent *uic, 
				gpointer           user_data, 
				const gchar       *verbname)
{
	GThumbWindow *window = user_data;
	void (*module) (GThumbWindow *window);

	if (gthumb_module_get ("dlg_jpegtran", (gpointer*) &module))
		(*module) (window);
}


void 
tools_duplicates_command_impl (BonoboUIComponent *uic, 
                               gpointer           user_data, 
                               const gchar       *verbname)
{
        GThumbWindow *window = user_data;
	void (*module) (GThumbWindow *window);

	if (gthumb_module_get ("dlg_duplicates", (gpointer*) &module))
		(*module) (window);
}


void 
tools_convert_format_command_impl (BonoboUIComponent *uic, 
				   gpointer           user_data, 
				   const gchar       *verbname)
{
        GThumbWindow *window = user_data;
        dlg_convert (window);
}


void 
tools_change_date_command_impl (BonoboUIComponent *uic, 
				gpointer           user_data, 
				const gchar       *verbname)
{
        GThumbWindow *window = user_data;
        dlg_change_date (window);
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
help_help_command_impl (BonoboUIComponent *uic, 
			gpointer           user_data, 
			const gchar       *verbname)
{
	display_help ((GThumbWindow *) user_data, NULL);
}


void 
help_shortcuts_command_impl (BonoboUIComponent *uic, 
			     gpointer           user_data, 
			     const gchar       *verbname)
{
	display_help ((GThumbWindow *) user_data, "shortcuts");
}


void 
help_about_command_impl (BonoboUIComponent *uic, 
			 gpointer           user_data, 
			 const gchar       *verbname)
{
	GThumbWindow      *window = user_data;
	static GtkWidget  *about;
	GdkPixbuf         *logo;
	const char        *authors[] = {
		"Paolo Bacchilega <paolo.bacch@tin.it>",
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
				 "Copyright \xc2\xa9 2001-2003 Free Software Foundation, Inc.",
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

