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

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "catalog.h"
#include "comments.h"
#include "dlg-bookmarks.h"
#include "dlg-catalog.h"
#include "dlg-tags.h"
#include "dlg-convert.h"
#include "dlg-duplicates.h"
#include "dlg-file-utils.h"
#include "dlg-jpegtran.h"
#include "dlg-photo-importer.h"
#include "dlg-preferences.h"
#include "dlg-rename-series.h"
#include "dlg-scale-series.h"
#include "dlg-scripts.h"
#include "dlg-search.h"
#include "dlg-write-to-cd.h"
#include "file-utils.h"
#include "gconf-utils.h"
#include "gfile-utils.h"
#include "gth-browser.h"
#include "gth-folder-selection-dialog.h"
#include "gth-viewer.h"
#include "gth-window-utils.h"
#include "gtk-utils.h"
#include "main.h"
#include "thumb-cache.h"
#include "dlg-png-exporter.h"
#include "dlg-web-exporter.h"

#define MAX_NAME_LEN 1024
#define DEF_CONFIRM_DEL TRUE
#define CATALOG_PERMISSIONS 0600

void
gth_browser_activate_action_file_new_window (GtkAction  *action,
					     GthBrowser *browser)
{
	const char *path;
	char       *uri = NULL;
	GtkWidget  *new_browser;

	switch (gth_browser_get_sidebar_content (browser)) {
	case GTH_SIDEBAR_DIR_LIST:
		path = gth_browser_get_current_directory (browser);
		uri = add_scheme_if_absent (path);
		break;

	case GTH_SIDEBAR_CATALOG_LIST:
		path = gth_browser_get_current_catalog (browser);
		uri = g_strconcat ("catalog://", remove_host_from_uri (path), NULL);
		break;

	default:
		break;
	}

	new_browser = gth_browser_new (uri);
	gtk_widget_show (new_browser);

	g_free (uri);
}


void
gth_browser_activate_action_file_view_image (GtkAction  *action,
					     GthBrowser *browser)
{
	FileData  *file;
	GtkWidget *new_viewer;

	file = gth_window_get_image_data (GTH_WINDOW (browser));
	if (file == NULL)
		return;

	if (eel_gconf_get_boolean (PREF_SINGLE_WINDOW, FALSE)) {
		GtkWidget *viewer = gth_viewer_get_current_viewer ();
		if (viewer != NULL) {
			gth_viewer_load (GTH_VIEWER (viewer), file);
			gtk_window_present (GTK_WINDOW (viewer));
			return;
		}
	}

	new_viewer = gth_viewer_new (NULL);
	gth_viewer_load (GTH_VIEWER (new_viewer), file);
	gtk_window_present (GTK_WINDOW (new_viewer));
}


void
gth_browser_activate_action_file_image_properties (GtkAction  *action,
						   GthBrowser *browser)
{
	gth_browser_show_image_prop (browser);
}


void
gth_browser_activate_action_file_camera_import (GtkAction  *action,
						GthBrowser *browser)
{
        dlg_photo_importer (browser, NULL);
}


void
gth_browser_activate_action_file_write_to_cd (GtkAction  *action,
					      GthBrowser *browser)
{
	dlg_write_to_cd (browser);
}


void
gth_browser_activate_action_image_rename (GtkAction  *action,
					  GthBrowser *browser)
{
	dlg_rename_series (browser);
}


void
gth_browser_activate_action_image_delete (GtkAction  *action,
					  GthBrowser *browser)
{
	const char *image_filename;
	GList      *list;

	image_filename = gth_window_get_image_filename (GTH_WINDOW (browser));
	if (image_filename == NULL)
		return;

	list = g_list_prepend (NULL, g_strdup (image_filename));
	dlg_file_delete__confirm (GTH_WINDOW (browser),
				  list,
				  _("The image will be moved to the Trash, are you sure?"));

	/* the list is deallocated when the dialog is closed. */
}


void
gth_browser_activate_action_image_copy (GtkAction  *action,
					GthBrowser *browser)
{
	const char *image_filename;
	GList      *list;

	image_filename = gth_window_get_image_filename (GTH_WINDOW (browser));
	if (image_filename == NULL)
		return;

	list = g_list_prepend (NULL, g_strdup (image_filename));
	dlg_file_copy__ask_dest (GTH_WINDOW (browser),
				 gth_browser_get_current_directory (browser),
				 list);

	/* the list is deallocated when the dialog is closed. */
}


void
gth_browser_activate_action_image_move (GtkAction  *action,
					GthBrowser *browser)
{
	const char *image_filename;
	GList      *list;

	image_filename = gth_window_get_image_filename (GTH_WINDOW (browser));
	if (image_filename == NULL)
		return;

	list = g_list_prepend (NULL, g_strdup (image_filename));
	dlg_file_move__ask_dest (GTH_WINDOW (browser),
				 gth_browser_get_current_directory (browser),
				 list);

	/* the list is deallocated when the dialog is closed. */
}


void
gth_browser_activate_action_edit_rename_file (GtkAction  *action,
					      GthBrowser *browser)
{
	dlg_rename_series (browser);
}


static gboolean
duplicate_file (GtkWindow  *window,
		const char *old_path)
{
	const char *old_name, *ext;
	char       *old_name_no_ext;
	char       *new_name, *new_path;
	char       *dir;
	int         try;

	g_return_val_if_fail (old_path != NULL, FALSE);

	old_name = file_name_from_path (old_path);
	old_name_no_ext = remove_extension_from_path (old_name);
	ext = strrchr (old_name, '.');

	dir = remove_level_from_path (old_path);

	for (try = 2; TRUE; try++) {
		new_name = g_strdup_printf ("%s (%d)%s",
					    old_name_no_ext,
					    try,
					    (ext == NULL) ? "" : ext);

		new_path = build_uri (dir, new_name, NULL);
		if (! path_is_file (new_path))
			break;
		g_free (new_name);
		g_free (new_path);
	}

	g_free (dir);
	g_free (old_name_no_ext);

	if (file_copy (old_path, new_path, FALSE, NULL)) {
		cache_copy (old_path, new_path);
		comment_copy (old_path, new_path);
	} 
	else {
		char      *utf8_path;
		char      *msg;
		GtkWidget *d;
		int        r;

		utf8_path = get_utf8_display_name_from_uri (old_path);
		msg = g_strdup_printf (_("Could not duplicate the image \"%s\": %s"),
				       utf8_path,
				       errno_to_string ());
		g_free (utf8_path);

		d = _gtk_yesno_dialog_new (GTK_WINDOW (window),
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
			return FALSE;
		}
	}

	g_free (new_name);
	g_free (new_path);

	return TRUE;
}


static void
duplicate_file_list (GtkWindow   *window,
		     const GList *list)
{
	const GList *scan;

	for (scan = list; scan; scan = scan->next) {
		char *filename = scan->data;
		if (! duplicate_file (window, filename))
			break;
	}
}


void
gth_browser_activate_action_edit_duplicate_file (GtkAction  *action,
						 GthWindow  *window)
{
	GList *list;

	list = gth_window_get_file_list_selection (window);
	duplicate_file_list (GTK_WINDOW (window), list);
	path_list_free (list);
}


void
gth_browser_activate_action_edit_delete_files (GtkAction *action,
					       GthWindow *window)
{
	GList *list;

	list = gth_window_get_file_list_selection (window);
	dlg_file_delete__confirm (window,
				  list,
				  _("The selected images will be moved to the Trash, are you sure?"));

	/* the list is deallocated when the dialog is closed. */
}


void
gth_browser_activate_action_edit_copy_files (GtkAction *action,
					     GthWindow *window)
{
	GList *list;

	list = gth_window_get_file_list_selection (window);
	dlg_file_copy__ask_dest (window,
				 gth_browser_get_current_directory (GTH_BROWSER (window)),
				 list);

	/* the list is deallocated when the dialog is closed. */
}


void
gth_browser_activate_action_edit_move_files (GtkAction *action,
					     GthWindow *window)
{
	GList *list;

	list = gth_window_get_file_list_selection (window);
	dlg_file_move__ask_dest (window,
				 gth_browser_get_current_directory (GTH_BROWSER (window)),
				 list);

	/* the list is deallocated when the dialog is closed. */
}


void
gth_browser_activate_action_edit_select_all (GtkAction  *action,
					     GthBrowser *browser)
{
	gth_file_view_select_all (gth_browser_get_file_view (browser));
}


void
gth_browser_activate_action_edit_add_to_catalog (GtkAction *action,
						 GthWindow *window)
{
	GList *list;

	list = gth_window_get_file_list_selection (window);
	dlg_add_to_catalog (window, list);

	/* the list is deallocated when the dialog is closed. */
}


void
gth_browser_activate_action_edit_remove_from_catalog (GtkAction  *action,
						      GthBrowser *browser)
{
	GList *list;

	list = gth_window_get_file_list_selection (GTH_WINDOW (browser));
	remove_files_from_catalog (GTH_WINDOW (browser),
				   gth_browser_get_current_catalog (browser),
				   list);

	/* the list is deallocated in remove_files_from_catalog. */
}


void
gth_browser_activate_action_edit_catalog_view (GtkAction  *action,
					       GthBrowser *browser)
{
	char *path;

	path = catalog_list_get_selected_path (gth_browser_get_catalog_list (browser));
	if (path == NULL)
		return;
	if (path_is_dir (path)) {
		gth_browser_go_to_catalog (browser, NULL);
		gth_browser_go_to_catalog_directory (browser, path);
	} else
		gth_browser_go_to_catalog (browser, path);

	g_free (path);
}


void
gth_browser_activate_action_edit_catalog_view_new_window (GtkAction  *action,
							  GthBrowser *browser)
{
	char *path;
	char *uri;

	path = catalog_list_get_selected_path (gth_browser_get_catalog_list (browser));
	if (path == NULL)
		return;
	uri = g_strconcat ("catalog://", remove_host_from_uri (path), NULL);

	gtk_widget_show (gth_browser_new (uri));

	g_free (uri);
	g_free (path);
}


static void
catalog_rename (GthBrowser *browser,
		const char *catalog_path)
{
	char         *name_only;
	char         *new_name;
	char         *new_catalog_path;
	char         *path_only;
	gboolean      is_dir;
	FileData     *old_fd;
	FileData     *new_fd;

	if (catalog_path == NULL)
		return;

	is_dir = path_is_dir (catalog_path);
	old_fd = file_data_new (catalog_path);

	if (! is_dir)
		name_only = remove_extension_from_path (old_fd->utf8_name);
	else
		name_only = g_strdup (old_fd->utf8_name);

	new_name = _gtk_request_dialog_run (GTK_WINDOW (browser),
					    GTK_DIALOG_MODAL,
					    _("Enter the new name: "),
					    name_only,
					    MAX_NAME_LEN,
					    GTK_STOCK_CANCEL,
					    _("_Rename"));
	g_free (name_only);

	if (new_name == NULL) {
		file_data_unref (old_fd);
		return;
	}

	if (strchr (new_name, '/') != NULL) {
		_gtk_error_dialog_run (GTK_WINDOW (browser),
				       _("The name \"%s\" is not valid because it contains the character \"/\". " "Please use a different name."), new_name);

		file_data_unref (old_fd);
		return;
	}

	path_only = remove_level_from_path (old_fd->utf8_path);
	new_catalog_path = g_strconcat (path_only,
					"/",
					new_name,
					! is_dir ? CATALOG_EXT : NULL,
					NULL);
	g_free (path_only);
	g_free (new_name);

	new_fd = file_data_new (new_catalog_path);
	g_free (new_catalog_path);

	if (path_is_file (new_fd->utf8_path)) {
		_gtk_error_dialog_run (GTK_WINDOW (browser),
				       _("The name \"%s\" is already used. " "Please use a different name."), new_fd->utf8_name);
	} 
	else if (file_move (old_fd->utf8_path,new_fd->utf8_path, FALSE, NULL)) {
		gth_monitor_notify_catalog_renamed (old_fd->utf8_path,new_fd->utf8_path);
	} 
	else {
		_gtk_error_dialog_run (GTK_WINDOW (browser),
				       is_dir ? _("Could not rename the library \"%s\"") : _("Could not rename the catalog \"%s\""),
				       new_fd->utf8_name);
	}

	file_data_unref (old_fd);
	file_data_unref (new_fd);
}


void
gth_browser_activate_action_edit_catalog_rename (GtkAction  *action,
						 GthBrowser *browser)
{
	char *catalog_path;

	catalog_path = catalog_list_get_selected_path (gth_browser_get_catalog_list (browser));
	if (catalog_path == NULL)
		return;
	catalog_rename (browser, catalog_path);
	g_free (catalog_path);
}



static void
real_catalog_delete (GthBrowser *browser)
{
	char     *catalog_path;
	GError   *error = NULL;

	catalog_path = catalog_list_get_selected_path (gth_browser_get_catalog_list (browser));
	if (catalog_path == NULL)
		return;

	file_unlink_with_gerror (catalog_path, &error);

	if (error)
		_gtk_error_dialog_from_gerror_run (GTK_WINDOW (browser), &error);

        gth_monitor_notify_update_catalog (catalog_path, GTH_MONITOR_EVENT_DELETED);
	g_free (catalog_path);
}


static void
catalog_delete (GthBrowser *browser,
		const char *catalog_path)
{
	GtkWidget    *dialog;
	gchar        *message;
	int           r;

	if (catalog_path == NULL)
		return;

	/* Always ask before deleting folders. */

	if (! path_is_dir (catalog_path) && ! eel_gconf_get_boolean (PREF_CONFIRM_DELETION, DEF_CONFIRM_DEL)) {
		real_catalog_delete (browser);
		return;
	}

	if (path_is_dir (catalog_path))
		message = g_strdup (_("The selected library will be removed, are you sure?"));
	else
		message = g_strdup (_("The selected catalog will be removed, are you sure?"));

	dialog = _gtk_yesno_dialog_new (GTK_WINDOW (browser),
					GTK_DIALOG_MODAL,
					message,
					GTK_STOCK_CANCEL,
					GTK_STOCK_REMOVE);
	g_free (message);

	r = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (GTK_WIDGET (dialog));
	if (r == GTK_RESPONSE_YES)
		real_catalog_delete (browser);
}


void
gth_browser_activate_action_edit_catalog_delete (GtkAction  *action,
						 GthBrowser *browser)
{
	char *catalog_path;

	catalog_path = catalog_list_get_selected_path (gth_browser_get_catalog_list (browser));
	if (catalog_path == NULL)
		return;
	catalog_delete (browser, catalog_path);
	g_free (catalog_path);
}


void
gth_browser_activate_action_edit_catalog_move (GtkAction  *action,
					       GthBrowser *browser)
{
	char *catalog_path;

	catalog_path = catalog_list_get_selected_path (gth_browser_get_catalog_list (browser));
	if (catalog_path == NULL)
		return;

	dlg_move_to_catalog_directory (GTH_WINDOW (browser), catalog_path);

	/* catalog_path is deallocated when the dialog is closed. */
}


void
gth_browser_activate_action_edit_catalog_edit_search (GtkAction  *action,
						      GthBrowser *browser)
{
	char *catalog_path;

	catalog_path = catalog_list_get_selected_path (gth_browser_get_catalog_list (browser));
	if (catalog_path == NULL)
		return;
        dlg_catalog_edit_search (browser, catalog_path);
	g_free (catalog_path);
}


void
gth_browser_activate_action_edit_catalog_redo_search (GtkAction  *action,
						      GthBrowser *browser)
{
	char *catalog_path;

	catalog_path = catalog_list_get_selected_path (gth_browser_get_catalog_list (browser));
	if (catalog_path == NULL)
		return;
        dlg_catalog_search (browser, catalog_path);
	g_free (catalog_path);
}


void
gth_browser_activate_action_edit_current_catalog_new (GtkAction  *action,
						      GthBrowser *browser)
{
	CatalogList       *catalog_list;
	char              *new_name;
	char              *new_catalog_path;
	FileData          *fd;
	GFileOutputStream *handle;
	GError		  *error;
	GFile		  *gfile;

	catalog_list = gth_browser_get_catalog_list (browser);
	if (catalog_list->path == NULL)
		return;

	new_name = _gtk_request_dialog_run (GTK_WINDOW (browser),
					    GTK_DIALOG_MODAL,
					    _("Enter the catalog name: "),
					    _("New Catalog"),
					    MAX_NAME_LEN,
					    GTK_STOCK_CANCEL,
					    GTK_STOCK_OK);
	if (new_name == NULL)
		return;

	if (strchr (new_name, '/') != NULL) {
		_gtk_error_dialog_run (GTK_WINDOW (browser),
				       _("The name \"%s\" is not valid because it contains the character \"/\". " "Please use a different name."), new_name);
		g_free (new_name);
		return;
	}

	new_catalog_path = g_strconcat (catalog_list->path,
					"/",
					new_name,
					CATALOG_EXT,
					NULL);
	fd = file_data_new (new_catalog_path);
	gfile = gfile_new (fd->utf8_path);
	g_free (new_name);
	g_free (new_catalog_path);
	
	if (path_is_file (fd->utf8_path)) {
		_gtk_error_dialog_run (GTK_WINDOW (browser),
				       _("The name \"%s\" is already used. " "Please use a different name."), fd->utf8_name);
	} else if ((handle = g_file_create (gfile, G_FILE_CREATE_PRIVATE, NULL, &error)) != NULL) {
		gth_monitor_notify_update_catalog (fd->utf8_path, GTH_MONITOR_EVENT_CREATED);
		gth_monitor_notify_reload_catalogs ();
		g_object_unref (handle);
	} else {
		_gtk_error_dialog_run (GTK_WINDOW (browser),
				       _("Could not create the catalog \"%s\": %s"),
				       fd->utf8_name,
				       error->message);
	        g_error_free (error);
	}

	g_object_unref (gfile);
	file_data_unref (fd);
}


static void
create_new_folder_or_library (GthBrowser *browser,
			      char       *str_old_name,
			      char       *str_prompt,
			      char       *str_error)
{
	const char *current_path;
	char       *new_name;
	char       *new_path;
	FileData   *fd;

	switch (gth_browser_get_sidebar_content (browser)) {
	case GTH_SIDEBAR_DIR_LIST:
		current_path = gth_browser_get_dir_list (browser)->path;
		break;
	case GTH_SIDEBAR_CATALOG_LIST:
		current_path = gth_browser_get_catalog_list (browser)->path;
		break;
	default:
		return;
		break;
	}

	new_name = _gtk_request_dialog_run (GTK_WINDOW (browser),
					    GTK_DIALOG_MODAL,
					    str_prompt,
					    str_old_name,
					    MAX_NAME_LEN,
					    GTK_STOCK_CANCEL,
					    _("C_reate"));

	if (new_name == NULL)
		return;

	if (strchr (new_name, '/') != NULL) {
		_gtk_error_dialog_run (GTK_WINDOW (browser),
				       _("The name \"%s\" is not valid because it contains the character \"/\". " "Please use a different name."), new_name);
		g_free (new_name);
		return;
	}

	/* Create folder */

	new_path = build_uri (current_path, new_name, NULL);
	fd = file_data_new (new_path);
	g_free (new_path);	

	if (path_is_dir (fd->utf8_path)) {
		_gtk_error_dialog_run (GTK_WINDOW (browser),
				       _("The name \"%s\" is already used. " "Please use a different name."), fd->utf8_name);
	} 
	else if (! dir_make (fd->utf8_path)) {
		_gtk_error_dialog_run (GTK_WINDOW (browser),
				       str_error,
				       fd->utf8_name);
	} 
	else
                gth_monitor_notify_update_directory (fd->utf8_path, GTH_MONITOR_EVENT_CREATED);

	file_data_unref (fd);
}


void
gth_browser_activate_action_edit_current_catalog_new_library (GtkAction  *action,
							      GthBrowser *browser)
{
	create_new_folder_or_library (browser,
				      _("New Library"),
				      _("Enter the library name: "),
				      _("Could not create the library \"%s\""));
}


void
gth_browser_activate_action_edit_current_catalog_rename (GtkAction  *action,
							 GthBrowser *browser)
{
	catalog_rename (browser, gth_browser_get_current_catalog (browser));
}


void
gth_browser_activate_action_edit_current_catalog_delete (GtkAction  *action,
							 GthBrowser *browser)
{
	catalog_delete (browser, gth_browser_get_current_catalog (browser));
}


void
gth_browser_activate_action_edit_current_catalog_move (GtkAction  *action,
						       GthBrowser *browser)
{
	char *catalog_path;

	if (gth_browser_get_current_catalog (browser) == NULL)
		return;
	catalog_path = g_strdup (gth_browser_get_current_catalog (browser));
	dlg_move_to_catalog_directory (GTH_WINDOW (browser), catalog_path);

	/* catalog_path is deallocated when the dialog is closed. */
}


void
gth_browser_activate_action_edit_current_catalog_edit_search (GtkAction  *action,
							      GthBrowser *browser)
{
	const char *catalog_path = gth_browser_get_current_catalog (browser);

	if (catalog_path == NULL)
		return;
        dlg_catalog_edit_search (browser, catalog_path);
}


void
gth_browser_activate_action_edit_current_catalog_redo_search (GtkAction  *action,
							      GthBrowser *browser)
{
	const char *catalog_path = gth_browser_get_current_catalog (browser);

	if (catalog_path == NULL)
		return;
        dlg_catalog_search (browser, catalog_path);
}


void
gth_browser_activate_action_edit_dir_view (GtkAction  *action,
					   GthBrowser *browser)
{
	char *path;

	path = gth_dir_list_get_selected_path (gth_browser_get_dir_list (browser));
	if (path == NULL)
		return;
	gth_browser_go_to_directory (browser, path);
	g_free (path);
}


void
gth_browser_activate_action_edit_dir_view_new_window (GtkAction  *action,
						      GthBrowser *browser)
{
	char *path;
	char *uri;

	path = gth_dir_list_get_selected_path (gth_browser_get_dir_list (browser));
	if (path == NULL)
		return;

	uri = add_scheme_if_absent (path);
	gtk_widget_show (gth_browser_new (uri));

	g_free (uri);
	g_free (path);
}


static void
show_folder (GtkWindow  *window,
	     const char *path)
{
	char   *uri;
	GError *error = NULL;

	if (path == NULL)
		return;

	uri = add_scheme_if_absent (path);

	gtk_show_uri (gtk_window_get_screen (window),
		      uri,
		      gtk_get_current_event_time (),
		      &error);

        if (error)
                _gtk_error_dialog_from_gerror_run (window, &error);

	g_free (uri);
}


void
gth_browser_activate_action_edit_dir_open (GtkAction  *action,
					   GthBrowser *browser)
{
	char *path;

	path = gth_dir_list_get_selected_path (gth_browser_get_dir_list (browser));
	if (path == NULL)
		return;
	show_folder (GTK_WINDOW (browser), path);
	g_free (path);
}


static void
folder_rename (GtkWindow  *window,
	       const char *old_path)
{
	char         *new_name;
	char         *new_path;
	char         *parent_path;
	FileData     *old_fd;
	FileData     *new_fd;
	
	if (old_path == NULL)
		return;

	old_fd = file_data_new (old_path);

	new_name = _gtk_request_dialog_run (window,
					    GTK_DIALOG_MODAL,
					    _("Enter the new name: "),
					    old_fd->utf8_name,
					    MAX_NAME_LEN,
					    GTK_STOCK_CANCEL,
					    _("_Rename"));
	if (new_name == NULL) {
		file_data_unref (old_fd);
		return;
	}

	if (strchr (new_name, '/') != NULL) {
		_gtk_error_dialog_run (window,
				       _("The name \"%s\" is not valid because it contains the character \"/\". " "Please use a different name."), old_fd->utf8_name);
		g_free (new_name);
		file_data_unref (old_fd);
		return;
	}

	/* Rename */

	parent_path = remove_level_from_path (old_fd->utf8_path);
	new_path = build_uri (parent_path, new_name, NULL);
	g_free (new_name);
	g_free (parent_path);
	new_fd = file_data_new (new_path);
	g_free (new_path);

	gth_monitor_pause ();

	if (file_data_same (old_fd, new_fd)) {
		_gtk_error_dialog_run (window,
				       _("Could not rename the folder \"%s\": %s"),
				       old_fd->utf8_name,
				       _("source and destination are the same"));
	} 
	else if (path_is_dir (new_fd->utf8_path)) {
		_gtk_error_dialog_run (window,
				       _("The name \"%s\" is already used. " "Please use a different name."),
				       new_fd->utf8_name);
	} 
	else {
		gboolean        result;

		result = file_move (old_fd->utf8_path, new_fd->utf8_path, FALSE, NULL);
		if (result) {
			comment_move (old_path, new_path);
			gth_monitor_notify_directory_renamed (old_fd->utf8_path, new_fd->utf8_path);
		} 
		else {
			_gtk_error_dialog_run (window,
					       _("Could not rename the folder \"%s\""),
					       old_fd->utf8_name);
		}
	}

	gth_monitor_resume ();

	file_data_unref (old_fd);
	file_data_unref (new_fd);
}


void
gth_browser_activate_action_edit_dir_rename (GtkAction  *action,
					     GthBrowser *browser)
{
	char *old_path;

	old_path = gth_dir_list_get_selected_path (gth_browser_get_dir_list (browser));
	if (old_path == NULL)
		return;

	folder_rename (GTK_WINDOW (browser), old_path);
	g_free (old_path);
}


/* -- folder delete -- */


typedef struct {
	GtkWindow *window;
	char      *path;
} FolderDeleteData;


static void
folder_delete__continue2 (GError 	*error,
			  gpointer       data)
{
	FolderDeleteData *fddata = data;

	if (error != NULL) {
		const char *message;
		char       *utf8_name;

		message = _("Could not delete the folder \"%s\": %s");
		utf8_name = basename_for_display (fddata->path);

		_gtk_error_dialog_run (fddata->window,
				       message,
				       utf8_name,
				       error->message);
		g_free (utf8_name);

	}

	g_free (fddata->path);
	g_free (fddata);
}


static void
folder_delete__continue (GError 	*error,
			 gpointer        data)
{
	FolderDeleteData *fddata = data;

	if ((error != NULL) && (g_error_matches (error, G_FILE_ERROR, G_FILE_ERROR_NOENT))) {
		int r = GTK_RESPONSE_YES;

		if (eel_gconf_get_boolean (PREF_MSG_CANNOT_MOVE_TO_TRASH, TRUE)) {
			GtkWidget *d;
			char      *utf8_name;
			char      *message;

			utf8_name = basename_for_display (fddata->path);
			message = g_strdup_printf (_("\"%s\" cannot be moved to the Trash. Do you want to delete it permanently?"), utf8_name);

			d = _gtk_yesno_dialog_with_checkbutton_new (
				    fddata->window,
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
			dlg_folder_delete (GTH_WINDOW (fddata->window),
					   fddata->path,
					   folder_delete__continue2,
					   fddata);
	} 
	else
		folder_delete__continue2 (error, data);
}


static void
folder_delete (GtkWindow  *window,
	       const char *path)
{
	FolderDeleteData *fddata;
	GtkWidget        *dialog;
	int               r;

	if (path == NULL)
		return;

	/* Always ask before deleting folders. */

	dialog = _gtk_yesno_dialog_new (window,
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

	dlg_folder_move_to_trash (GTH_WINDOW (window),
				  path,
				  folder_delete__continue,
				  fddata);
}


void
gth_browser_activate_action_edit_dir_delete (GtkAction  *action,
					     GthBrowser *browser)
{
	char *path;

	path = gth_dir_list_get_selected_path (gth_browser_get_dir_list (browser));
	if (path == NULL)
		return;
	folder_delete (GTK_WINDOW (browser), path);
	g_free (path);
}


static void
folder_copy__continue (GError        *error,
		       gpointer       data)
{
	GtkWidget *file_sel = data;
	GthWindow *window;
	char      *new_path;

	window = g_object_get_data (G_OBJECT (file_sel), "gthumb_window");
	new_path = g_object_get_data (G_OBJECT (file_sel), "new_path");

	if (error != NULL) {
		const char *message;
		char       *utf8_name;

		message = _("Could not copy the folder \"%s\": %s");
		utf8_name = basename_for_display (new_path);

		_gtk_error_dialog_run (NULL,
				       message,
				       utf8_name,
				       error->message);
		g_free (utf8_name);
	}
	else if (gth_folder_selection_get_goto_destination (GTH_FOLDER_SELECTION (file_sel)))
		gth_browser_go_to_directory (GTH_BROWSER (window), new_path);

	if (file_sel != NULL)
		gtk_widget_destroy (file_sel);
}


static void
folder_copy__response_cb (GObject *object,
			  int      response_id,
			  gpointer data)
{
	GtkWidget    *file_sel = data;
	GthWindow    *window;
	char         *old_path;
	char         *dest_dir;
	char         *new_path;
	const char   *dir_name;
	gboolean      move;
	const char   *message;
	FileData     *old_fd;
	FileData     *new_fd;

	if (response_id != GTK_RESPONSE_OK) {
		gtk_widget_destroy (file_sel);
		return;
	}

	window = g_object_get_data (G_OBJECT (file_sel), "gthumb_window");
	old_path = g_object_get_data (G_OBJECT (file_sel), "path");
	dest_dir = g_strdup (gth_folder_selection_get_folder (GTH_FOLDER_SELECTION (file_sel)));
	move = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (file_sel), "folder_op"));

	if (dest_dir == NULL)
		return;

	if (! dlg_check_folder (window, dest_dir)) {
		g_free (dest_dir);
		return;
	}

	gtk_widget_hide (file_sel);

	dir_name = file_name_from_path (old_path);
	new_path = build_uri (dest_dir, dir_name, NULL);

	old_fd = file_data_new (old_path);
	new_fd = file_data_new (new_path);

	message = move ? _("Could not move the folder \"%s\": %s") : _("Could not copy the folder \"%s\": %s");

	if (file_data_same (old_fd, new_fd)) {
		_gtk_error_dialog_run (GTK_WINDOW (window),
				       message,
				       old_fd->utf8_name,
				       _("source and destination are the same"));
	} 
	else if (path_in_path (old_fd->utf8_path, new_fd->utf8_path)) {
		_gtk_error_dialog_run (GTK_WINDOW (window),
				       message,
				       old_fd->utf8_name,
				       _("source contains destination"));
	} 
	else if (path_is_dir (new_path)) {
		_gtk_error_dialog_run (GTK_WINDOW (window),
				       message,
				       new_fd->utf8_name,
				       _("a folder with that name is already present."));
	} 
	else if (!move) {
		g_object_set_data_full (G_OBJECT (file_sel),
					"new_path",
					g_strdup (new_path),
					g_free);
		dlg_folder_copy (window,
				 old_path,
				 new_path,
				 move,
				 TRUE,
				 FALSE,
				 folder_copy__continue,
				 file_sel);
		file_sel = NULL;
	} 
	else {
		char   *old_folder_comment = NULL;
		GError *error = NULL;

		/* Comment cache. */

		old_folder_comment = comments_get_comment_filename (old_path, TRUE);

		file_move (old_fd->utf8_path, new_fd->utf8_path, FALSE, &error);
		if (!error) {
			char *new_folder_comment;

			/* moving folders on the same file system can be
			 * implemeted with rename, which is faster. */

			new_folder_comment = comments_get_comment_filename (new_path, TRUE);
			file_move (old_folder_comment, new_folder_comment, TRUE, NULL);
			g_free (new_folder_comment);

			gth_monitor_notify_directory_renamed (old_path, new_path);

			if (gth_folder_selection_get_goto_destination (GTH_FOLDER_SELECTION (file_sel)))
				gth_browser_go_to_directory (GTH_BROWSER (window), new_path);
		} 
		else {
			_gtk_error_dialog_run (GTK_WINDOW (window),
					       message,
					       old_fd->utf8_name,
					       error->message);
			g_error_free (error);
		}

		g_free (old_folder_comment);
	}

	g_free (dest_dir);
	g_free (new_path);
	file_data_unref (old_fd);
	file_data_unref (new_fd);

	if (file_sel != NULL)
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
folder_copy (GthWindow   *window,
	     const char  *path,
	     gboolean     move)
{
	GtkWidget *file_sel;
	char      *parent;

	if (path == NULL)
		return;

	file_sel = gth_folder_selection_new (_("Choose the destination folder"));

	parent = remove_level_from_path (path);
	gth_folder_selection_set_folder (GTH_FOLDER_SELECTION (file_sel), parent);
	g_free (parent);

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

	gtk_window_set_transient_for (GTK_WINDOW (file_sel), GTK_WINDOW (window));
	gtk_window_set_modal (GTK_WINDOW (file_sel), TRUE);
	gtk_widget_show (file_sel);
}


void
gth_browser_activate_action_edit_dir_copy (GtkAction  *action,
					   GthBrowser *browser)
{
	char *path;

	path = gth_dir_list_get_selected_path (gth_browser_get_dir_list (browser));
	if (path == NULL)
		return;
	folder_copy (GTH_WINDOW (browser), path, FALSE);
	g_free (path);
}


void
gth_browser_activate_action_edit_dir_move (GtkAction  *action,
					   GthBrowser *browser)
{
	char *path;

	path = gth_dir_list_get_selected_path (gth_browser_get_dir_list (browser));
	if (path == NULL)
		return;
	folder_copy (GTH_WINDOW (browser), path, TRUE);
	g_free (path);
}


/**/

typedef struct {
	GthBrowser *browser;
	GList      *file_list;
	GList      *add_list, *remove_list;
} FolderTagsData;


static void
edit_current_folder_tags__done (gpointer data)
{
	FolderTagsData *fcdata = data;
	GList                *scan;

	for (scan = fcdata->file_list; scan; scan = scan->next) {
		const char  *filename = scan->data;
		CommentData *cdata;
		GList       *scan2;
		GthDirList  *dir_list;

		cdata = comments_load_comment (filename, FALSE);
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

		comments_save_tags (filename, cdata);
		comment_data_free (cdata);

		dir_list = gth_browser_get_dir_list (fcdata->browser);
		if (path_in_path (dir_list->path, filename)) {
			gth_dir_list_remove_directory (dir_list, filename);
			gth_dir_list_add_directory (dir_list, filename);
		}
	}

	path_list_free (fcdata->file_list);
	path_list_free (fcdata->add_list);
	path_list_free (fcdata->remove_list);
	g_free (fcdata);
}


static void
edit_current_folder_tags (GthBrowser *browser,
                          const char *path)
{
	FolderTagsData *fcdata;

	if (path == NULL)
		return;

	fcdata = g_new0 (FolderTagsData, 1);
	fcdata->browser = browser;
	fcdata->add_list = NULL;
	fcdata->remove_list = NULL;
	fcdata->file_list = g_list_prepend (NULL, g_strdup (path));

	dlg_choose_tags (GTK_WINDOW (browser),
                         fcdata->file_list,
                         NULL,
                         &(fcdata->add_list),
                         &(fcdata->remove_list),
                         edit_current_folder_tags__done,
                         fcdata);
}


void
gth_browser_activate_action_edit_dir_tags (GtkAction  *action,
                                           GthBrowser *browser)
{
	char *path;

	path = gth_dir_list_get_selected_path (gth_browser_get_dir_list (browser));
	if (path == NULL)
		return;
	edit_current_folder_tags (browser, path);
	g_free (path);
}


void
gth_browser_activate_action_edit_current_dir_open (GtkAction  *action,
						   GthBrowser *browser)
{
	show_folder (GTK_WINDOW (browser), gth_browser_get_dir_list (browser)->path);
}


void
gth_browser_activate_action_edit_current_dir_rename (GtkAction  *action,
						     GthBrowser *browser)
{
	folder_rename (GTK_WINDOW (browser), gth_browser_get_dir_list (browser)->path);
}


void
gth_browser_activate_action_edit_current_dir_delete (GtkAction  *action,
						     GthBrowser *browser)
{
	folder_delete (GTK_WINDOW (browser), gth_browser_get_dir_list (browser)->path);
}


void
gth_browser_activate_action_edit_current_dir_copy (GtkAction  *action,
						   GthBrowser *browser)
{
	folder_copy (GTH_WINDOW (browser), gth_browser_get_dir_list (browser)->path, FALSE);
}


void
gth_browser_activate_action_edit_current_dir_move (GtkAction  *action,
						   GthBrowser *browser)
{
	folder_copy (GTH_WINDOW (browser), gth_browser_get_dir_list (browser)->path, TRUE);
}


void
gth_browser_activate_action_edit_current_dir_tags (GtkAction  *action,
                                                   GthBrowser *browser)
{
	edit_current_folder_tags (browser, gth_browser_get_dir_list (browser)->path);
}


void
gth_browser_activate_action_edit_current_dir_new (GtkAction  *action,
						  GthBrowser *browser)
{
	create_new_folder_or_library (browser,
				      _("New Folder"),
				      _("Enter the folder name: "),
				      _("Could not create the folder \"%s\""));
}


void
gth_browser_activate_action_view_next_image (GtkAction  *action,
					     GthBrowser *browser)
{
	gth_browser_show_next_image (browser, FALSE);
}


void
gth_browser_activate_action_view_prev_image (GtkAction  *action,
					     GthBrowser *browser)
{
	gth_browser_show_prev_image (browser, FALSE);
}


void
gth_browser_activate_action_view_image_prop (GtkAction  *action,
					     GthBrowser *browser)
{
	gth_browser_show_image_prop (browser);
}


void
gth_browser_activate_action_go_back (GtkAction  *action,
				     GthBrowser *browser)
{
	gth_browser_go_back (browser);
}


void
gth_browser_activate_action_go_forward (GtkAction  *action,
					GthBrowser *browser)
{
	gth_browser_go_forward (browser);
}


void
gth_browser_activate_action_go_up (GtkAction  *action,
				   GthBrowser *browser)
{
	gth_browser_go_up (browser);
}


void
gth_browser_activate_action_go_refresh (GtkAction  *action,
					GthBrowser *browser)
{
	gth_browser_refresh (browser);
}


void
gth_browser_activate_action_go_stop (GtkAction  *action,
				     GthBrowser *browser)
{
	gth_browser_stop_loading (browser);
}


void
gth_browser_activate_action_go_home (GtkAction  *action,
				     GthBrowser *browser)
{
	gth_browser_go_to_directory (browser, get_home_uri ());
}


void
gth_browser_activate_action_go_to_container (GtkAction  *action,
					     GthBrowser *browser)
{
	GList *list;
	char  *path;

	list = gth_window_get_file_list_selection (GTH_WINDOW (browser));
	g_return_if_fail (list != NULL);
	path = remove_level_from_path (list->data);

	gth_browser_go_to_directory (browser, path);

	g_free (path);
	path_list_free (list);
}


void
gth_browser_activate_action_go_delete_history (GtkAction  *action,
					       GthBrowser *browser)
{
	gth_browser_delete_history (browser);
}


static void
open_location_response_cb (GtkDialog  *file_sel,
			   int         button_number,
			   gpointer    data)
{
	GthBrowser *browser = data;
	char       *folder;

	if (button_number != GTK_RESPONSE_OK) {
		gtk_widget_destroy (GTK_WIDGET (file_sel));
		return;
	}

	folder = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (file_sel));
	if (folder != NULL)
		gth_browser_go_to_directory (browser, folder);

	gtk_widget_destroy (GTK_WIDGET (file_sel));
}


void
gth_browser_activate_action_go_location (GtkAction  *action,
					 GthBrowser *browser)
{
	GtkWidget *chooser;

	chooser = gtk_file_chooser_dialog_new (_("Open Location"),
					       GTK_WINDOW (browser),
					       GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
					       GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					       GTK_STOCK_OPEN, GTK_RESPONSE_OK,
					       NULL);
	/* Permit VFS URIs */
	gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (chooser), FALSE);
	gtk_file_chooser_set_current_folder_uri (GTK_FILE_CHOOSER (chooser), gth_browser_get_current_directory (browser));
	gtk_dialog_set_default_response(GTK_DIALOG (chooser), GTK_RESPONSE_OK);

	g_signal_connect (G_OBJECT (chooser),
			  "response",
			  G_CALLBACK (open_location_response_cb),
			  browser);

	gtk_window_set_modal (GTK_WINDOW (chooser), TRUE);
	gtk_widget_show (chooser);
}


void
gth_browser_activate_action_bookmarks_add (GtkAction  *action,
					   GthBrowser *browser)
{
	char *path = NULL;

	if (gth_browser_get_sidebar_content (browser) == GTH_SIDEBAR_CATALOG_LIST) {
		CatalogList *catalog_list = gth_browser_get_catalog_list (browser);
		GtkTreeIter  iter;
		char        *prefix, *catalog_path;

		if (! catalog_list_get_selected_iter (catalog_list, &iter))
			return;
		if (catalog_list_is_search (catalog_list, &iter))
			prefix = g_strdup (SEARCH_PREFIX);
		else
			prefix = g_strdup (CATALOG_PREFIX);
		catalog_path = catalog_list_get_path_from_iter (catalog_list, &iter);

		path = g_strconcat (prefix, remove_host_from_uri (catalog_path), NULL);

		g_free (catalog_path);
		g_free (prefix);

	} else {
		GthDirList *dir_list = gth_browser_get_dir_list (browser);

		if (dir_list->path == NULL)
			return;
		path = add_scheme_if_absent (dir_list->path);
	}

	if (path == NULL)
		return;

	bookmarks_add (preferences.bookmarks, path, TRUE, TRUE);
	bookmarks_write_to_disk (preferences.bookmarks);
	g_free (path);

	gth_monitor_notify_update_bookmarks ();
}


void
gth_browser_activate_action_bookmarks_edit (GtkAction  *action,
					    GthBrowser *browser)
{
	dlg_edit_bookmarks (browser);
}


void
gth_browser_activate_action_tools_slideshow (GtkAction  *action,
					     GthBrowser *browser)
{
	gth_window_set_slideshow (GTH_WINDOW (browser), TRUE);
}


void
gth_browser_activate_action_tools_find_images (GtkAction  *action,
					       GthBrowser *browser)
{
        dlg_search (NULL, browser);
}


void
gth_browser_activate_action_tools_index_image (GtkAction  *action,
					       GthBrowser *browser)
{
        dlg_exporter (browser);
}


void
gth_browser_activate_action_tools_web_exporter (GtkAction  *action,
						GthBrowser *browser)
{
        dlg_web_exporter (browser);
}


void
gth_browser_activate_action_tools_convert_format (GtkAction  *action,
						  GthBrowser *browser)
{
	dlg_convert (browser);
}


void
gth_browser_activate_action_tools_find_duplicates (GtkAction  *action,
						   GthBrowser *browser)
{
        dlg_duplicates (browser);
}


void
gth_browser_activate_action_tools_preferences (GtkAction  *action,
					       GthBrowser *browser)
{
	dlg_preferences (browser);
}


void
gth_browser_activate_action_view_toolbar (GtkAction *action,
					  gpointer   data)
{
	eel_gconf_set_boolean (PREF_UI_TOOLBAR_VISIBLE, gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)));
}


void
gth_browser_activate_action_view_statusbar (GtkAction *action,
					    gpointer   data)
{
	eel_gconf_set_boolean (PREF_UI_STATUSBAR_VISIBLE, gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)));
}


void
gth_browser_activate_action_view_filterbar (GtkAction *action,
					    gpointer   data)
{
	GthBrowser *browser = data;

	eel_gconf_set_boolean (PREF_UI_FILTERBAR_VISIBLE, gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)));
	if (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)))
		gth_browser_show_filterbar (browser);
	else
		gth_browser_hide_filterbar (browser);
}


void
gth_browser_activate_action_view_thumbnails (GtkAction *action,
					     gpointer   data)
{
	eel_gconf_set_boolean (PREF_SHOW_THUMBNAILS, gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)));
}


void
gth_browser_activate_action_view_show_preview (GtkAction  *action,
					       GthBrowser *browser)
{
	if (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)))
		gth_browser_show_image_pane (browser);
	else
		gth_browser_hide_image_pane (browser);
}


void
gth_browser_activate_action_view_show_info (GtkAction  *action,
					    GthBrowser *browser)
{
	if (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)))
		gth_browser_show_image_data (browser);
	else
		gth_browser_hide_image_data (browser);
}


void
gth_browser_activate_action_view_show_hidden_files (GtkAction  *action,
						    GthBrowser *browser)
{
	eel_gconf_set_boolean (PREF_SHOW_HIDDEN_FILES, gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)));
}


void
gth_browser_activate_action_close_image_mode (GtkAction  *action,
					      GthBrowser *browser)
{
	gth_browser_show_sidebar (browser);
}


void
gth_browser_activate_action_sort_reversed (GtkAction  *action,
					   GthBrowser *browser)
{
	GtkSortType new_type;

	if (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)))
		new_type = GTK_SORT_DESCENDING;
	else
		new_type = GTK_SORT_ASCENDING;
	gth_browser_set_sort_type (browser, new_type);
}


void
gth_browser_activate_action_tools_resize_images (GtkAction  *action,
						 GthBrowser *browser)
{
	dlg_scale_series (browser);
}


void
gth_browser_activate_action_scripts (GtkAction  *action,
                                     GthBrowser *browser) {
	dlg_scripts (GTH_WINDOW (browser), (DoneFunc) gth_browser_update_script_menu, browser);
}

