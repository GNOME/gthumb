/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2009 Free Software Foundation, Inc.
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
#include <gthumb.h>
#include <extensions/image_viewer/gth-image-viewer-page.h>


typedef struct {
	GthBrowser *browser;
	GFile      *old_file;
	GFile      *new_file;
	gulong      response_id;
} WallpaperData;


static GFile *
get_wallpaper_file_n (int n)
{
	char  *name;
	char  *filename;
	GFile *file;

	name = g_strdup_printf ("wallpaper%d.jpeg", n);
	gth_user_dir_make_dir_for_file (GTH_DIR_DATA, GTHUMB_DIR, name, NULL);
	filename = gth_user_dir_get_file (GTH_DIR_DATA, GTHUMB_DIR, name, NULL);
	file = g_file_new_for_path (filename);

	g_free (filename);
	g_free (name);

	return file;
}


static GFile *
get_wallpaper_file (void)
{
	GFile *wallpaper_file;

	wallpaper_file = get_wallpaper_file_n (1);
	if (g_file_query_exists (wallpaper_file, NULL)) {
		/* Use a new filename to force an update. */

		g_object_unref (wallpaper_file);

		wallpaper_file = get_wallpaper_file_n (2);
		if (g_file_query_exists (wallpaper_file, NULL))
			g_file_delete (wallpaper_file, NULL, NULL);
	}

	return 	wallpaper_file;
}


static WallpaperData *
wallpaper_data_new (GthBrowser *browser)
{
	WallpaperData *wdata;
	char          *path;

	wdata = g_new0 (WallpaperData, 1);
	wdata->browser = browser;

	path = eel_gconf_get_string ("/desktop/gnome/background/picture_filename", NULL);
	if (path != NULL) {
		wdata->old_file = g_file_new_for_path (path);
		g_free (path);
	}

	wdata->new_file = get_wallpaper_file ();

	return wdata;
}


static void
wallpaper_data_free (WallpaperData *wdata)
{
	g_signal_handler_disconnect (gth_browser_get_infobar (wdata->browser), wdata->response_id);
	_g_object_unref (wdata->old_file);
	_g_object_unref (wdata->new_file);
	g_free (wdata);
}


enum {
	_RESPONSE_PREFERENCES = 1,
	_RESPONSE_UNDO
};


static void
set_wallpaper_file (GFile *file)
{
	char *path;

	path = g_file_get_path (file);
	if (path != NULL)
		eel_gconf_set_string ("/desktop/gnome/background/picture_filename", path);

	g_free (path);
}


static void
infobar_response_cb (GtkInfoBar *info_bar,
		     int         response_id,
		     gpointer    user_data)
{
	WallpaperData *wdata = user_data;
	GError        *error = NULL;

	g_return_if_fail (GTH_IS_BROWSER (wdata->browser));

	switch (response_id) {
	case _RESPONSE_PREFERENCES:
		if (! g_spawn_command_line_async ("gnome-appearance-properties --show-page=background", &error))
			_gtk_error_dialog_from_gerror_run (GTK_WINDOW (wdata->browser), _("Could not show the desktop background properties"), &error);
		break;

	case _RESPONSE_UNDO:
		if (wdata->old_file != NULL)
			set_wallpaper_file (wdata->old_file);
		break;
	}

	gtk_widget_hide (GTK_WIDGET (info_bar));
	wallpaper_data_free (wdata);
}


static void
wallpaper_data_set (WallpaperData *wdata)
{
	GtkWidget *infobar;

	set_wallpaper_file (wdata->new_file);

	infobar = gth_browser_get_infobar (wdata->browser);
	gth_info_bar_set_icon (GTH_INFO_BAR (infobar), GTK_STOCK_DIALOG_INFO);

	{
		char *name;
		char *msg;

		name = _g_file_get_display_name (wdata->new_file);
		msg = g_strdup_printf ("The image \"%s\" has been set as desktop background", name);
		gth_info_bar_set_primary_text (GTH_INFO_BAR (infobar), msg);

		g_free (msg);
		g_free (name);
	}

	_gtk_info_bar_clear_action_area (GTK_INFO_BAR (infobar));
	gtk_orientable_set_orientation (GTK_ORIENTABLE (gtk_info_bar_get_action_area (GTK_INFO_BAR (infobar))), GTK_ORIENTATION_HORIZONTAL);
	gtk_info_bar_set_message_type (GTK_INFO_BAR (infobar), GTK_MESSAGE_INFO);
	gtk_info_bar_add_buttons (GTK_INFO_BAR (infobar),
				  GTK_STOCK_PREFERENCES, _RESPONSE_PREFERENCES,
				  GTK_STOCK_UNDO, _RESPONSE_UNDO,
				  GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
				  NULL);
	gtk_info_bar_set_response_sensitive (GTK_INFO_BAR (infobar),
					     _RESPONSE_UNDO,
					     wdata->old_file != NULL);
	wdata->response_id = g_signal_connect (infobar,
			  	  	       "response",
			  	  	       G_CALLBACK (infobar_response_cb),
			  	  	       wdata);

	gtk_widget_show (infobar);
}


static void
wallpaper_save_ready_cb (GthFileData *a,
			 GError      *error,
			 gpointer     user_data)
{
	WallpaperData *wdata = user_data;

	if (error != NULL) {
		_gtk_error_dialog_from_gerror_run (GTK_WINDOW (wdata->browser), _("Could not set the desktop background"), &error);
		wallpaper_data_free (wdata);
		return;
	}

	wallpaper_data_set (wdata);
}


static void
copy_wallpaper_ready_cb (GObject      *source_object,
			 GAsyncResult *res,
			 gpointer      user_data)
{
	WallpaperData *wdata = user_data;
	GError        *error = NULL;

	if (! g_file_copy_finish (G_FILE (source_object), res, &error)) {
		_gtk_error_dialog_from_gerror_run (GTK_WINDOW (wdata->browser), _("Could not set the desktop background"), &error);
		wallpaper_data_free (wdata);
		return;
	}

	wallpaper_data_set (wdata);
}


void
gth_browser_activate_action_tool_desktop_background (GtkAction  *action,
					             GthBrowser *browser)
{
	WallpaperData *wdata;
	gboolean       saving_wallpaper = FALSE;
	GthFileData   *file_data;
	GList         *items;
	GList         *file_list;

	wdata = wallpaper_data_new (browser);

	if (gth_main_extension_is_active ("image_viewer") && gth_browser_get_file_modified (browser)) {
		GtkWidget *viewer_page;

		viewer_page = gth_browser_get_viewer_page (browser);
		if (viewer_page != NULL) {
			GdkPixbuf *pixbuf;

			pixbuf = g_object_ref (gth_image_viewer_page_get_pixbuf (GTH_IMAGE_VIEWER_PAGE (viewer_page)));
			file_data = gth_file_data_new (wdata->new_file, NULL);
			_gdk_pixbuf_save_async (pixbuf,
						file_data,
						"image/jpeg",
						TRUE,
						wallpaper_save_ready_cb,
						wdata);
			saving_wallpaper = TRUE;

			g_object_unref (pixbuf);
		}
	}

	if (saving_wallpaper)
		return;

	items = gth_file_selection_get_selected (GTH_FILE_SELECTION (gth_browser_get_file_list_view (browser)));
	file_list = gth_file_list_get_files (GTH_FILE_LIST (gth_browser_get_file_list (browser)), items);
	if (file_list == NULL)
		return;

	file_data = file_list->data;
	if (file_data == NULL)
		return;

	if (g_file_is_native (file_data->file)) {
		_g_object_unref (wdata->new_file);
		wdata->new_file = g_file_dup (file_data->file);
		wallpaper_data_set (wdata);
	}
	else
		g_file_copy_async (file_data->file,
				   wdata->new_file,
				   G_FILE_COPY_OVERWRITE,
				   G_PRIORITY_DEFAULT,
				   NULL,
				   NULL,
				   NULL,
				   copy_wallpaper_ready_cb,
				   wdata);

	_g_object_list_unref (file_list);
	_gtk_tree_path_list_free (items);
}
