/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001 The Free Software Foundation, Inc.
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
#include <errno.h>
#include <math.h>
#include <string.h>
#include <unistd.h>

#include <gdk/gdkkeysyms.h>
#include <libgnome/libgnome.h>
#include <libgnomeui/gnome-window-icon.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <libgnomevfs/gnome-vfs-async-ops.h>
#include <libgnomevfs/gnome-vfs-result.h>
#include <libgnomevfs/gnome-vfs-ops.h>
#include <libgnomevfs/gnome-vfs-mime.h>
#include <libbonoboui.h>

#include "auto-completion.h"
#include "catalog.h"
#include "catalog-list.h"
#include "commands-impl.h"
#include "dlg-image-prop.h"
#include "dlg-bookmarks.h"
#include "dlg-file-utils.h"
#include "e-combo-button.h"
#include "file-utils.h"
#include "file-list.h"
#include "gconf-utils.h"
#include "glib-utils.h"
#include "gthumb-window.h"
#include "gthumb-info-bar.h"
#include "gthumb-stock.h"
#include "gtk-utils.h"
#include "image-list-utils.h"
#include "image-viewer.h"
#include "main.h"
#include "nav-window.h"
#include "pixbuf-utils.h"
#include "thumb-cache.h"

#include "icons/pixbufs.h"
#include "icons/nav_button.xpm"

#define ACTIVITY_DELAY         200
#define BUSY_CURSOR_DELAY      200
#define DISPLAY_PROGRESS_DELAY 1000
#define HIDE_SIDEBAR_DELAY     150
#define LOAD_DIR_DELAY         25
#define PANE_MIN_SIZE          60
#define PROGRESS_BAR_WIDTH     60
#define SEL_CHANGED_DELAY      150
#define UPDATE_DIR_DELAY       250
#define VIEW_IMAGE_DELAY       25

#define GLADE_EXPORTER_FILE    "gthumb_png_exporter.glade"
#define HISTORY_LIST_MENU      "/menu/Go/HistoryList/"
#define HISTORY_LIST_POPUP     "/popups/HistoryListPopup/"

enum {
	TARGET_PLAIN,
	TARGET_URILIST,
};

static GtkTargetEntry target_table[] = {
	{ "text/uri-list", 0, TARGET_URILIST },
	{ "text/plain",    0, TARGET_PLAIN }
};
static guint n_targets = sizeof (target_table) / sizeof (target_table[0]);

static GtkTreePath  *dir_list_tree_path = NULL;
static GtkTreePath  *catalog_list_tree_path = NULL;

static const BonoboUIVerb gthumb_verbs [] = {
	BONOBO_UI_VERB ("File_NewWindow", file_new_window_command_impl),
	BONOBO_UI_VERB ("File_CloseWindow", file_close_window_command_impl),
	BONOBO_UI_VERB ("File_OpenWith", file_open_with_command_impl),
	BONOBO_UI_VERB ("File_OpenWithPopup", file_open_with_command_impl),
	BONOBO_UI_VERB ("File_Print", file_print_command_impl),
	BONOBO_UI_VERB ("Image_Print", file_print_command_impl),
	BONOBO_UI_VERB ("File_Save", file_save_command_impl),
	BONOBO_UI_VERB ("Image_Save", file_save_command_impl),
	BONOBO_UI_VERB ("File_Revert", file_revert_command_impl),
	BONOBO_UI_VERB ("File_Exit", file_exit_command_impl),
	BONOBO_UI_VERB ("Image_OpenWith", image_open_with_command_impl),
	BONOBO_UI_VERB ("Image_Rename", image_rename_command_impl),
	BONOBO_UI_VERB ("Image_RenamePopup", image_rename_command_impl),
	BONOBO_UI_VERB ("Image_Duplicate", image_duplicate_command_impl),
	BONOBO_UI_VERB ("Image_Delete", image_delete_command_impl),
	BONOBO_UI_VERB ("Image_Copy", image_copy_command_impl),
	BONOBO_UI_VERB ("Image_Move", image_move_command_impl),
	BONOBO_UI_VERB ("Edit_RenameFile", edit_rename_file_command_impl),
	BONOBO_UI_VERB ("Edit_RenameFilePopup", edit_rename_file_command_impl),
	BONOBO_UI_VERB ("Edit_DuplicateFile", edit_duplicate_file_command_impl),
	BONOBO_UI_VERB ("Edit_DeleteFiles", edit_delete_files_command_impl),
	BONOBO_UI_VERB ("Edit_CopyFiles", edit_copy_files_command_impl),
	BONOBO_UI_VERB ("Edit_MoveFiles", edit_move_files_command_impl),
	BONOBO_UI_VERB ("Edit_SelectAll", edit_select_all_command_impl),
	BONOBO_UI_VERB ("Edit_EditComment", edit_edit_comment_command_impl),
	BONOBO_UI_VERB ("Edit_CurrentEditComment", edit_current_edit_comment_command_impl),
	BONOBO_UI_VERB ("Edit_DeleteComment", edit_delete_comment_command_impl),
	BONOBO_UI_VERB ("Edit_EditCategories", edit_edit_categories_command_impl),
	BONOBO_UI_VERB ("Edit_CurrentEditCategories", edit_current_edit_categories_command_impl),
	BONOBO_UI_VERB ("Edit_AddToCatalog", edit_add_to_catalog_command_impl),
	BONOBO_UI_VERB ("Edit_RemoveFromCatalog", edit_remove_from_catalog_command_impl),
	BONOBO_UI_VERB ("EditDir_Open", edit_folder_open_nautilus_command_impl),
	BONOBO_UI_VERB ("EditDir_Rename", edit_folder_rename_command_impl),
	BONOBO_UI_VERB ("EditDir_Delete", edit_folder_delete_command_impl),
	BONOBO_UI_VERB ("EditDir_Copy", edit_folder_copy_command_impl),
	BONOBO_UI_VERB ("EditDir_Move", edit_folder_move_command_impl),
	BONOBO_UI_VERB ("EditCurrentDir_New", edit_current_folder_new_command_impl),
	BONOBO_UI_VERB ("EditCurrentDir_Open", edit_current_folder_open_nautilus_command_impl),
	BONOBO_UI_VERB ("EditCurrentDir_Rename", edit_current_folder_rename_command_impl),
	BONOBO_UI_VERB ("EditCurrentDir_Delete", edit_current_folder_delete_command_impl),
	BONOBO_UI_VERB ("EditCurrentDir_Copy", edit_current_folder_copy_command_impl),
	BONOBO_UI_VERB ("EditCurrentDir_Move", edit_current_folder_move_command_impl),
	BONOBO_UI_VERB ("EditCatalog_Rename", edit_catalog_rename_command_impl),
	BONOBO_UI_VERB ("EditCatalog_Delete", edit_catalog_delete_command_impl),
	BONOBO_UI_VERB ("EditCatalog_Move", edit_catalog_move_command_impl),
	BONOBO_UI_VERB ("EditCatalog_EditSearch", edit_catalog_edit_search_command_impl),
	BONOBO_UI_VERB ("EditCatalog_RedoSearch", edit_catalog_redo_search_command_impl),
	BONOBO_UI_VERB ("EditCurrentCatalog_NewLibrary", edit_current_catalog_new_library_command_impl),
	BONOBO_UI_VERB ("EditCurrentCatalog_New", edit_current_catalog_new_command_impl),
	BONOBO_UI_VERB ("EditCurrentCatalog_Rename", edit_current_catalog_rename_command_impl),
	BONOBO_UI_VERB ("EditCurrentCatalog_Delete", edit_current_catalog_delete_command_impl),
	BONOBO_UI_VERB ("EditCurrentCatalog_Move", edit_current_catalog_move_command_impl),
	BONOBO_UI_VERB ("EditCurrentCatalog_EditSearch", edit_current_catalog_edit_search_command_impl),
	BONOBO_UI_VERB ("EditCurrentCatalog_RedoSearch", edit_current_catalog_redo_search_command_impl),
	BONOBO_UI_VERB ("AlterImage_Rotate90", alter_image_rotate_command_impl),
	BONOBO_UI_VERB ("AlterImage_Rotate90CC", alter_image_rotate_cc_command_impl),
	BONOBO_UI_VERB ("AlterImage_Flip", alter_image_flip_command_impl),
	BONOBO_UI_VERB ("AlterImage_Mirror", alter_image_mirror_command_impl),
	BONOBO_UI_VERB ("AlterImage_Desaturate", alter_image_desaturate_command_impl),
	BONOBO_UI_VERB ("AlterImage_Invert", alter_image_invert_command_impl),
	BONOBO_UI_VERB ("AlterImage_Equalize", alter_image_equalize_command_impl),
	BONOBO_UI_VERB ("AlterImage_AdjustLevels", alter_image_adjust_levels_command_impl),
	BONOBO_UI_VERB ("AlterImage_Posterize", alter_image_posterize_command_impl),
	BONOBO_UI_VERB ("AlterImage_BrightnessContrast", alter_image_brightness_contrast_command_impl),
	BONOBO_UI_VERB ("AlterImage_HueSaturation", alter_image_hue_saturation_command_impl),
	BONOBO_UI_VERB ("AlterImage_ColorBalance", alter_image_color_balance_command_impl),
	BONOBO_UI_VERB ("AlterImage_Resize", alter_image_scale_command_impl),
	BONOBO_UI_VERB ("View_ZoomIn", view_zoom_in_command_impl),
	BONOBO_UI_VERB ("View_ZoomOut", view_zoom_out_command_impl),
	BONOBO_UI_VERB ("View_Zoom100", view_zoom_100_command_impl),
	BONOBO_UI_VERB ("View_ZoomFit", view_zoom_fit_command_impl),
	BONOBO_UI_VERB ("View_StepAnimation", view_step_ani_command_impl),
	BONOBO_UI_VERB ("View_ShowFolders", view_show_folders_command_impl),
	BONOBO_UI_VERB ("View_ShowCatalogs", view_show_catalogs_command_impl),
	BONOBO_UI_VERB ("View_Fullscreen", view_fullscreen_command_impl),
	BONOBO_UI_VERB ("View_FullscreenPopup", view_fullscreen_command_impl),
	BONOBO_UI_VERB ("View_ExitFullscreenPopup", view_fullscreen_command_impl),
	BONOBO_UI_VERB ("View_PrevImage", view_prev_image_command_impl),
	BONOBO_UI_VERB ("View_NextImage", view_next_image_command_impl),
	BONOBO_UI_VERB ("View_ImageProp", view_image_prop_command_impl),
	BONOBO_UI_VERB ("View_Sidebar_Folders", view_sidebar_command_impl),
	BONOBO_UI_VERB ("View_Sidebar_Catalogs", view_sidebar_command_impl),
	BONOBO_UI_VERB ("Go_Back", go_back_command_impl),
	BONOBO_UI_VERB ("Go_Forward", go_forward_command_impl),
	BONOBO_UI_VERB ("Go_Up", go_up_command_impl),
	BONOBO_UI_VERB ("Go_Stop", go_stop_command_impl),
	BONOBO_UI_VERB ("Go_Refresh", go_refresh_command_impl),
	BONOBO_UI_VERB ("Go_Home", go_home_command_impl),
	BONOBO_UI_VERB ("Go_ToContainer", go_to_container_command_impl),
	BONOBO_UI_VERB ("Go_ToContainerPopup", go_to_container_command_impl),
	BONOBO_UI_VERB ("Go_DeleteHistory", go_delete_history_command_impl),
	BONOBO_UI_VERB ("Go_Location", go_location_command_impl),
	BONOBO_UI_VERB ("Bookmarks_Add", bookmarks_add_command_impl),
	BONOBO_UI_VERB ("Bookmarks_Edit", bookmarks_edit_command_impl),
	BONOBO_UI_VERB ("Wallpaper_Centered", wallpaper_centered_command_impl),
	BONOBO_UI_VERB ("Wallpaper_Tiled", wallpaper_tiled_command_impl),
	BONOBO_UI_VERB ("Wallpaper_Scaled", wallpaper_scaled_command_impl),
	BONOBO_UI_VERB ("Wallpaper_Stretched", wallpaper_stretched_command_impl),
	BONOBO_UI_VERB ("Wallpaper_Restore", wallpaper_restore_command_impl),
	BONOBO_UI_VERB ("Tools_FindImages", tools_find_images_command_impl),
	BONOBO_UI_VERB ("Tools_IndexImage", tools_index_image_command_impl),
	BONOBO_UI_VERB ("Tools_Maintenance", tools_maintenance_command_impl),
	BONOBO_UI_VERB ("Tools_RenameSeries", tools_rename_series_command_impl),
	BONOBO_UI_VERB ("Tools_Preferences", tools_preferences_command_impl),
	BONOBO_UI_VERB ("Tools_JPEGRotate", tools_jpeg_rotate_command_impl),
	BONOBO_UI_VERB ("Tools_FindDuplicates", tools_duplicates_command_impl),
	BONOBO_UI_VERB ("Tools_ConvertFormat", tools_convert_format_command_impl),
	BONOBO_UI_VERB ("Tools_ChangeDate", tools_change_date_command_impl),
	BONOBO_UI_VERB ("Tools_Slideshow", tools_slideshow_command_impl),
	BONOBO_UI_VERB ("Help_Help", help_help_command_impl),
	BONOBO_UI_VERB ("Help_Shortcuts", help_shortcuts_command_impl),
	BONOBO_UI_VERB ("Help_About", help_about_command_impl),
	BONOBO_UI_VERB_END
};





static void 
set_command_sensitive (GThumbWindow *window,
		       const char   *cname,
		       gboolean      sensitive)
{
	char *full_cname = g_strconcat ("/commands/", cname, NULL);
	bonobo_ui_component_set_prop (window->ui_component, 
				      full_cname, 
				      "sensitive", 
				      sensitive ? "1" : "0", 
				      NULL);
	g_free (full_cname);
}


static void 
set_command_visible (GThumbWindow *window,
		     const char   *cname,
		     gboolean      visible)
{
	char *full_cname = g_strconcat ("/commands/", cname, NULL);
	bonobo_ui_component_set_prop (window->ui_component, 
				      full_cname, 
				      "hidden", 
				      visible ? "0" : "1", 
				      NULL);
	g_free (full_cname);
}


static void
set_command_state_without_notifing (GThumbWindow *window,
				    char         *cname,
				    gboolean      state)
{
	char *full_cname;
	char *new_value;
	char *old_value;

	full_cname = g_strconcat ("/commands/", cname, NULL);
	new_value = state ? "1" : "0";
	old_value = bonobo_ui_component_get_prop (window->ui_component, 
						  full_cname,
						  "state",
						  NULL);

	if ((old_value != NULL) && (strcmp (old_value, new_value) == 0)) {
		g_free (full_cname);
		g_free (old_value);
		return;
	}

	if (old_value != NULL)
		g_free (old_value);

	window->freeze_toggle_handler = 1;
	bonobo_ui_component_set_prop (window->ui_component, 
				      full_cname, 
				      "state", new_value,
				      NULL);
	g_free (full_cname);
}


static void
set_command_state_if_different (GThumbWindow *window,
				char         *cname,
				gboolean      setted,
				gboolean      notify)
{
	char *old_value;
	char *new_value;

	new_value = setted ? "1" : "0";

	old_value = bonobo_ui_component_get_prop (window->ui_component, 
						  cname,
						  "state",
						  NULL);

	if ((old_value != NULL) && (strcmp (old_value, new_value) == 0)) {
		g_free (old_value);
		return;
	}
	if (old_value != NULL)
		g_free (old_value);

	if (! notify)
		window->freeze_toggle_handler = 1;
	bonobo_ui_component_set_prop (window->ui_component, 
				      cname, 
				      "state", new_value,
				      NULL);
}





static void
window_update_statusbar_image_info (GThumbWindow *window)
{
	char       *text;
	char        time_txt[50];
	char       *size_txt;
	char       *file_size_txt;
	const char *path;
	char       *escaped_name;
	char       *utf8_name;
	int         width, height;
	int         zoom;
	time_t      timer;
	struct tm  *tm;
	gdouble     sec;

	path = window->image_path;

	if (path == NULL) {
		if (! GTK_WIDGET_VISIBLE (window->image_info_frame))
			return;
		
		bonobo_ui_component_set_prop (window->ui_component, "/status/ImageInfo", "hidden", "1", NULL);
		gtk_widget_hide (window->image_info_frame);
		return;

	} else if (! GTK_WIDGET_VISIBLE (window->image_info_frame)) {
		bonobo_ui_component_set_prop (window->ui_component, "/status/ImageInfo", "hidden", "0", NULL);
		gtk_widget_show (window->image_info_frame);
	}

	utf8_name = g_locale_to_utf8 (file_name_from_path (path), -1, 
				      NULL, NULL, NULL);

	width = image_viewer_get_image_width (IMAGE_VIEWER (window->viewer));
	height = image_viewer_get_image_height (IMAGE_VIEWER (window->viewer));
	
	timer = get_file_mtime (path);
	tm = localtime (&timer);
	strftime (time_txt, 50, _("%d %b %Y, %H:%M"), tm);
	sec = g_timer_elapsed (image_loader_get_timer (IMAGE_VIEWER (window->viewer)->loader),  NULL);

	zoom = (int) (IMAGE_VIEWER (window->viewer)->zoom_level * 100.0);
	escaped_name = g_markup_escape_text (utf8_name, -1);

	size_txt = g_strdup_printf (_("%d x %d pixels"), width, height);
	file_size_txt = gnome_vfs_format_file_size_for_display (get_file_size (path));

	/**/

	text = g_strdup_printf (" %s (%d%%) - %s - %s ",
				size_txt,
				zoom,
				file_size_txt,
				time_txt);
	gtk_label_set_markup (GTK_LABEL (window->image_info), text);

	g_free (utf8_name);
	g_free (escaped_name);
	g_free (size_txt);
	g_free (file_size_txt);
	g_free (text);
}


static void
window_update_infobar (GThumbWindow *window)
{
	char       *text;
	const char *path;
	char       *escaped_name;
	char       *utf8_name;
	int         images, current;

	path = window->image_path;
	if (path == NULL) {
		gthumb_info_bar_set_text (GTHUMB_INFO_BAR (window->info_bar), 
					  NULL, NULL);
		return;
	}

	images = gth_file_list_get_length (window->file_list);

	current = gth_file_list_pos_from_path (window->file_list, path) + 1;

	utf8_name = g_locale_to_utf8 (file_name_from_path (path), -1, 0, 0, 0);
	escaped_name = g_markup_escape_text (utf8_name, -1);

	text = g_strdup_printf ("%d/%d - <b>%s</b>%s", 
				current, 
				images, 
				escaped_name,
				window->image_modified ? "*" : "");

	gthumb_info_bar_set_text (GTHUMB_INFO_BAR (window->info_bar), 
				  text, 
				  NULL);

	g_free (utf8_name);
	g_free (escaped_name);
	g_free (text);
}


static void 
window_update_title (GThumbWindow *window)
{
	char *info_txt      = NULL;
	char *info_txt_utf8 = NULL;
	char *path;
	char *modified;

	g_return_if_fail (window != NULL);

	path = window->image_path;
	modified = window->image_modified ? "*" : "";

	if (path == NULL) {
		if ((window->sidebar_content == DIR_LIST)
		    && (window->dir_list->path != NULL)) {

			info_txt = g_strdup_printf ("%s%s - %s",
						    window->dir_list->path,
						    modified,
						    _("gThumb"));
		} else if ((window->sidebar_content == CATALOG_LIST)
			   && (window->catalog_path != NULL)) {
			const char *cat_name;
			char       *cat_name_no_ext;

			cat_name = file_name_from_path (window->catalog_path);
			cat_name_no_ext = g_strdup (cat_name);
			
			/* Cut out the file extension. */
			cat_name_no_ext[strlen (cat_name_no_ext) - 4] = 0;
			
			info_txt = g_strdup_printf ("%s - %s",
						    cat_name_no_ext,
						    _("gThumb"));
			g_free (cat_name_no_ext);
		} else
			info_txt = g_strdup_printf ("%s", _("gThumb"));
	} else {
		const char *image_name = file_name_from_path (path);

		if (image_name == NULL)
			image_name = "";

		if (window->image_catalog != NULL) {
			char *cat_name = g_strdup (file_name_from_path (window->image_catalog));

			/* Cut out the file extension. */
			cat_name[strlen (cat_name) - 4] = 0;
			
			info_txt = g_strdup_printf ("%s%s - %s - %s",
						    image_name,
						    modified,
						    cat_name,
						    _("gThumb"));
			g_free (cat_name);
		} else 
			info_txt = g_strdup_printf ("%s%s - %s",
						    image_name,
						    modified,
						    _("gThumb"));
	}

	info_txt_utf8 = g_locale_to_utf8 (info_txt, -1, NULL, NULL, NULL);
	gtk_window_set_title (GTK_WINDOW (window->app), info_txt_utf8);
	g_free (info_txt_utf8);
	g_free (info_txt);
}


static void
window_update_statusbar_list_info (GThumbWindow *window)
{
	char             *info, *size_txt, *sel_size_txt;
	char             *total_info, *selected_info;
	int               tot_n, sel_n;
	GnomeVFSFileSize  tot_size, sel_size;
	GList            *scan;
	GList            *selection;

	tot_n = 0;
	tot_size = 0;

	for (scan = window->file_list->list; scan; scan = scan->next) {
		FileData *fd = scan->data;
		tot_n++;
		tot_size += fd->size;
	}

	sel_n = 0;
	sel_size = 0;
	selection = gth_file_list_get_selection_as_fd (window->file_list);

	for (scan = selection; scan; scan = scan->next) {
		FileData *fd = scan->data;
		sel_n++;
		sel_size += fd->size;
	}

	g_list_free (selection);

	size_txt = gnome_vfs_format_file_size_for_display (tot_size);
	sel_size_txt = gnome_vfs_format_file_size_for_display (sel_size);

	if (tot_n == 0)
		total_info = g_strdup (_("No image"));
	else if (tot_n == 1)
		total_info = g_strdup_printf (_("1 image (%s)"),
					      size_txt);
	else
		total_info = g_strdup_printf (_("%d images (%s)"),
					      tot_n,
					      size_txt); 

	if (sel_n == 0)
		selected_info = g_strdup (" ");
	else if (sel_n == 1)
		selected_info = g_strdup_printf (_("1 selected (%s)"), 
						 sel_size_txt);
	else
		selected_info = g_strdup_printf (_("%d selected (%s)"), 
						 sel_n, 
						 sel_size_txt);

	info = g_strconcat (total_info, 
			    ((sel_n == 0) ? NULL : ", "),
			    selected_info, 
			    NULL);

	bonobo_ui_component_set_status (window->ui_component, info, NULL);

	g_free (total_info);
	g_free (selected_info);
	g_free (size_txt);
	g_free (sel_size_txt);
	g_free (info);
}


static void
window_update_go_sensitivity (GThumbWindow *window)
{
	set_command_sensitive (window, 
			       "Go_Back",
			       (window->history_current != NULL) && (window->history_current->next != NULL));
	gtk_widget_set_sensitive (window->go_back_combo_button, 
				  (window->history_current != NULL) && (window->history_current->next != NULL));
	
	set_command_sensitive (window, 
			       "Go_Forward",
			       (window->history_current != NULL) && (window->history_current->prev != NULL));
}


static void
window_update_sensitivity (GThumbWindow *window)
{
	GtkTreeIter iter;
	int         sidebar_content = window->sidebar_content;
	gboolean    sel_not_null;
	gboolean    only_one_is_selected;
	gboolean    image_is_void;
	gboolean    image_is_ani;
	gboolean    playing;
	gboolean    viewing_dir;
	gboolean    viewing_catalog;
	gboolean    is_catalog;
	gboolean    is_search;
	gboolean    not_fullscreen;

	sel_not_null = ilist_utils_selection_not_null (IMAGE_LIST (window->file_list->ilist));
	only_one_is_selected = ilist_utils_only_one_is_selected (IMAGE_LIST (window->file_list->ilist));
	image_is_void = image_viewer_is_void (IMAGE_VIEWER (window->viewer));
	image_is_ani = image_viewer_is_animation (IMAGE_VIEWER (window->viewer));
	playing = image_viewer_is_playing_animation (IMAGE_VIEWER (window->viewer));
	viewing_dir = sidebar_content == DIR_LIST;
	viewing_catalog = sidebar_content == CATALOG_LIST; 
	not_fullscreen = ! window->fullscreen;

	window_update_go_sensitivity (window);

	/* Image popup menu. */

	set_command_sensitive (window, "Image_OpenWith", ! image_is_void && not_fullscreen);
	set_command_sensitive (window, "Image_Rename", ! image_is_void && not_fullscreen);
	set_command_sensitive (window, "Image_RenamePopup", ! image_is_void && not_fullscreen);
	set_command_sensitive (window, "Image_Duplicate", ! image_is_void && not_fullscreen);
	set_command_sensitive (window, "Image_Delete", ! image_is_void && not_fullscreen);
	set_command_sensitive (window, "Image_Copy", ! image_is_void && not_fullscreen);
	set_command_sensitive (window, "Image_Move", ! image_is_void && not_fullscreen);

	/* File menu. */

	set_command_sensitive (window, "File_OpenWith", sel_not_null && not_fullscreen);
	set_command_sensitive (window, "File_OpenWithPopup", sel_not_null && not_fullscreen);

	set_command_sensitive (window, "File_Save", ! image_is_void);
	set_command_sensitive (window, "Image_Save", ! image_is_void);
	set_command_sensitive (window, "File_Revert", ! image_is_void && window->image_modified);
	set_command_sensitive (window, "File_Print", ! image_is_void);
	set_command_sensitive (window, "Image_Print", ! image_is_void);

	/* Edit menu. */

	set_command_sensitive (window, "Edit_RenameFile", only_one_is_selected && not_fullscreen);
	set_command_sensitive (window, "Edit_RenameFilePopup", only_one_is_selected && not_fullscreen);
	set_command_sensitive (window, "Edit_DuplicateFile", sel_not_null && not_fullscreen);
	set_command_sensitive (window, "Edit_DeleteFiles", sel_not_null && not_fullscreen);
	set_command_sensitive (window, "Edit_CopyFiles", sel_not_null && not_fullscreen);
	set_command_sensitive (window, "Edit_MoveFiles", sel_not_null && not_fullscreen);

	set_command_sensitive (window, "AlterImage_Rotate90", ! image_is_void && ! image_is_ani);
	set_command_sensitive (window, "AlterImage_Rotate90CC", ! image_is_void && ! image_is_ani);
	set_command_sensitive (window, "AlterImage_Flip", ! image_is_void && ! image_is_ani);
	set_command_sensitive (window, "AlterImage_Mirror", ! image_is_void && ! image_is_ani);
	set_command_sensitive (window, "AlterImage_Desaturate", ! image_is_void && ! image_is_ani);
	set_command_sensitive (window, "AlterImage_Resize", ! image_is_void && ! image_is_ani);
	set_command_sensitive (window, "AlterImage_ColorBalance", ! image_is_void && ! image_is_ani);
	set_command_sensitive (window, "AlterImage_HueSaturation", ! image_is_void && ! image_is_ani);
	set_command_sensitive (window, "AlterImage_BrightnessContrast", ! image_is_void && ! image_is_ani);
	set_command_sensitive (window, "AlterImage_Invert", ! image_is_void && ! image_is_ani);
	set_command_sensitive (window, "AlterImage_Posterize", ! image_is_void && ! image_is_ani);
	set_command_sensitive (window, "AlterImage_Equalize", ! image_is_void && ! image_is_ani);
	set_command_sensitive (window, "AlterImage_AdjustLevels", ! image_is_void && ! image_is_ani);

	set_command_sensitive (window, "View_ZoomIn", ! image_is_void);
	set_command_sensitive (window, "View_ZoomOut", ! image_is_void);
	set_command_sensitive (window, "View_Zoom100", ! image_is_void);
	set_command_sensitive (window, "View_ZoomFit", ! image_is_void);
	set_command_sensitive (window, "View_PlayAnimation", image_is_ani);
	set_command_sensitive (window, "View_StepAnimation", image_is_ani && ! playing);

	set_command_sensitive (window, "Edit_EditComment", sel_not_null);
	set_command_sensitive (window, "Edit_DeleteComment", sel_not_null);
	set_command_sensitive (window, "Edit_EditCategories", sel_not_null);

	set_command_sensitive (window, "Edit_CurrentEditComment", ! image_is_void);
	set_command_sensitive (window, "Edit_CurrentDeleteComment", ! image_is_void);
	set_command_sensitive (window, "Edit_CurrentEditCategories", ! image_is_void);

	set_command_sensitive (window, "Edit_AddToCatalog", sel_not_null);
	set_command_sensitive (window, "Edit_RemoveFromCatalog", viewing_catalog && sel_not_null);

	set_command_sensitive (window, "Go_ToContainer", not_fullscreen && viewing_catalog && only_one_is_selected);
	set_command_sensitive (window, "Go_ToContainerPopup", not_fullscreen && viewing_catalog && only_one_is_selected);

	set_command_sensitive (window, "Go_Stop", 
			       ((window->activity_ref > 0) 
				|| window->setting_file_list
				|| window->changing_directory
				|| window->file_list->doing_thumbs));

	/* Edit Catalog menu. */

	if (window->sidebar_content == CATALOG_LIST) { 
		char *view_catalog;
		char *view_search;

		bonobo_ui_component_set_prop (window->ui_component, 
					      "/menu/File/Folder", 
					      "hidden", "1",
					      NULL);

		set_command_visible (window, "EditCurrentDir_New", FALSE);
		set_command_visible (window, "EditCurrentCatalog_New", TRUE);
		set_command_visible (window, "EditCurrentCatalog_NewLibrary", TRUE);

		/**/

		is_catalog = (viewing_catalog && catalog_list_get_selected_iter (window->catalog_list, &iter));

		set_command_sensitive (window, "EditCatalog_Rename", is_catalog);
		set_command_sensitive (window, "EditCatalog_Delete", is_catalog);
		set_command_sensitive (window, "EditCatalog_Move", is_catalog && ! catalog_list_is_dir (window->catalog_list, &iter));
		
		is_search = (is_catalog && (catalog_list_is_search (window->catalog_list, &iter)));
		set_command_sensitive (window, "EditCatalog_EditSearch", is_search);
		set_command_sensitive (window, "EditCatalog_RedoSearch", is_search);

		/**/

		is_catalog = (window->catalog_path != NULL) && (viewing_catalog && catalog_list_get_iter_from_path (window->catalog_list, window->catalog_path, &iter));

		set_command_sensitive (window, "EditCurrentCatalog_Rename", is_catalog);
		set_command_sensitive (window, "EditCurrentCatalog_Delete", is_catalog);
		set_command_sensitive (window, "EditCurrentCatalog_Move", is_catalog && ! catalog_list_is_dir (window->catalog_list, &iter));
		
		is_search = (is_catalog && (catalog_list_is_search (window->catalog_list, &iter)));
		set_command_sensitive (window, "EditCurrentCatalog_EditSearch", is_search);
		set_command_sensitive (window, "EditCurrentCatalog_RedoSearch", is_search);

		if (is_search) {
			view_catalog = "1";
			view_search = "0";
		} else {
			view_catalog = "0";
			view_search = "1";
		}
		
		bonobo_ui_component_set_prop (window->ui_component, 
					      "/menu/File/Catalog", 
					      "hidden", view_catalog,
					      NULL);

		bonobo_ui_component_set_prop (window->ui_component, 
					      "/menu/File/Search", 
					      "hidden", view_search,
					      NULL);
	} else {
		bonobo_ui_component_set_prop (window->ui_component, 
					      "/menu/File/Folder", 
					      "hidden", "0",
					      NULL);
		bonobo_ui_component_set_prop (window->ui_component, 
					      "/menu/File/Catalog", 
					      "hidden", "1",
					      NULL);
		bonobo_ui_component_set_prop (window->ui_component, 
					      "/menu/File/Search", 
					      "hidden", "1",
					      NULL);

		set_command_visible (window, "EditCurrentDir_New", TRUE);
		set_command_visible (window, "EditCurrentCatalog_New", FALSE);
		set_command_visible (window, "EditCurrentCatalog_NewLibrary", FALSE);
	}

	/* View menu. */

	set_command_sensitive (window, "View_ImageProp", ! image_is_void);
	set_command_sensitive (window, "View_Fullscreen", ! image_is_void);

	/* Tools menu. */

	set_command_sensitive (window, "Tools_Slideshow", window->file_list->list != NULL);
	set_command_sensitive (window, "Tools_IndexImage", sel_not_null);
	set_command_sensitive (window, "Tools_RenameSeries", sel_not_null);
	set_command_sensitive (window, "Tools_ConvertFormat", sel_not_null);
	set_command_sensitive (window, "Tools_ChangeDate", sel_not_null);
	set_command_sensitive (window, "Tools_JPEGRotate", sel_not_null);
	set_command_sensitive (window, "Wallpaper_Centered", ! image_is_void);
	set_command_sensitive (window, "Wallpaper_Tiled", ! image_is_void);
	set_command_sensitive (window, "Wallpaper_Scaled", ! image_is_void);
	set_command_sensitive (window, "Wallpaper_Stretched", ! image_is_void);

	/* Rotate Tool */

	if (! window->setting_file_list && ! window->changing_directory) {
		GList *list, *scan;
		list = gth_file_list_get_selection_as_fd (window->file_list);
		for (scan = list; scan; scan = scan->next) {
			FileData *fd = scan->data;
			if (image_is_jpeg (fd->path)) 
				break;
		}
		g_list_free (list);
		set_command_sensitive (window, "Tools_JPEGRotate", scan != NULL);

	} else
		set_command_sensitive (window, "Tools_JPEGRotate", FALSE);
}


static void
window_progress (gfloat   percent, 
		 gpointer data)
{
	GThumbWindow *window = data;

	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (window->progress), 
				       percent);

	if (percent == 0.0) 
		set_command_sensitive (window, "Go_Stop", 
				       (window->activity_ref > 0) 
				       || window->setting_file_list
				       || window->changing_directory
				       || window->file_list->doing_thumbs);
}


static gboolean
load_progress (gpointer data)
{
	GThumbWindow *window = data;
	gtk_progress_bar_pulse (GTK_PROGRESS_BAR (window->progress));
	return TRUE;
}


static void
window_start_activity_mode (GThumbWindow *window)
{
	g_return_if_fail (window != NULL);

	if (window->activity_ref++ > 0)
		return;

	window->activity_timeout = g_timeout_add (ACTIVITY_DELAY, 
						  load_progress, 
						  window);
}


static void
window_stop_activity_mode (GThumbWindow *window)
{
	g_return_if_fail (window != NULL);

	if (--window->activity_ref > 0)
		return;

	if (window->activity_timeout == 0)
		return;

	g_source_remove (window->activity_timeout);
	window->activity_timeout = 0;
	window_progress (0.0, window);
}


/* -- set file list -- */


typedef struct {
	GThumbWindow *window;
	DoneFunc      done_func;
	gpointer      done_func_data;
} WindowSetListData;


static void
window_set_file_list_continue (gpointer callback_data)
{
	WindowSetListData *data = callback_data;
	GThumbWindow      *window = data->window;

	window_stop_activity_mode (window);
	window_update_statusbar_list_info (window);
	window->setting_file_list = FALSE;

	if (StartInFullscreen) {
		StartInFullscreen = FALSE;
		fullscreen_start (fullscreen, window);
	}

	if (StartSlideshow) {
		StartSlideshow = FALSE;
		window_start_slideshow (window);
	} 

	if (HideSidebar) {
		HideSidebar = FALSE;
		window_hide_sidebar (window);
	}

	if (ViewFirstImage) {
		ViewFirstImage = FALSE;
		window_show_first_image (window);
	}

	window_update_sensitivity (window);
		
	if (data->done_func != NULL)
		(*data->done_func) (data->done_func_data);
	g_free (data);
}


typedef struct {
	WindowSetListData *wsl_data;
	GList             *list;
	GThumbWindow      *window;
} SetListInterruptedData; 


static void
set_list_interrupted_cb (gpointer callback_data)
{
	SetListInterruptedData *sli_data = callback_data;

	sli_data->window->setting_file_list = TRUE;
	gth_file_list_set_list (sli_data->window->file_list, 
				sli_data->list, 
				window_set_file_list_continue, 
				sli_data->wsl_data);
	
	g_list_foreach (sli_data->list, (GFunc) g_free, NULL);
	g_list_free (sli_data->list);
	g_free (sli_data);
}


static void
window_set_file_list (GThumbWindow *window, 
		      GList        *list,
		      DoneFunc      done_func,
		      gpointer      done_func_data)
{
	WindowSetListData *data;

	if (window->slideshow)
		window_stop_slideshow (window);

	data = g_new (WindowSetListData, 1);
	data->window = window;
	data->done_func = done_func;
	data->done_func_data = done_func_data;

	if (window->setting_file_list) {
		SetListInterruptedData *sli_data;
		GList                  *scan;

		sli_data = g_new (SetListInterruptedData, 1);

		sli_data->wsl_data = data;
		sli_data->window = window;

		sli_data->list = NULL;
		for (scan = list; scan; scan = scan->next) {
			char *path = g_strdup ((char*)(scan->data));
			sli_data->list = g_list_prepend (sli_data->list, path);
		}

		gth_file_list_interrupt_set_list (window->file_list,
						  set_list_interrupted_cb,
						  sli_data);
		return;
	}

	window->setting_file_list = TRUE;
	window_start_activity_mode (window);
	gth_file_list_set_list (window->file_list, list, 
				window_set_file_list_continue, data);
}





void
window_update_file_list (GThumbWindow *window)
{
	if (window->sidebar_content == DIR_LIST)
		window_go_to_directory (window, window->dir_list->path);

	else if (window->sidebar_content == CATALOG_LIST) {
		char *catalog_path;

		catalog_path = catalog_list_get_selected_path (window->catalog_list);
		if (catalog_path == NULL)
			return;

		window_go_to_catalog (window, catalog_path);
		g_free (catalog_path);
	}
}


void
window_update_catalog_list (GThumbWindow *window)
{
	char *catalog_dir;
	char *base_dir;

	if (window->sidebar_content != CATALOG_LIST) 
		return;

	/* If the catalog still exists, show the directory it belongs to. */

	if ((window->catalog_path != NULL) 
	    && path_is_file (window->catalog_path)) {
		GtkTreeIter  iter;
		GtkTreePath *path;

		catalog_dir = remove_level_from_path (window->catalog_path);
		window_go_to_catalog_directory (window, catalog_dir);
		g_free (catalog_dir);

		if (! catalog_list_get_iter_from_path (window->catalog_list,
						       window->catalog_path, 
						       &iter))
			return;

		/* Select (without updating the file list) and view 
		 * the catalog. */

		catalog_list_select_iter (window->catalog_list, &iter);
		path = gtk_tree_model_get_path (GTK_TREE_MODEL (window->catalog_list->list_store), &iter);
		gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (window->catalog_list->list_view),
					      path,
					      NULL,
					      TRUE,
					      0.5,
					      0.0);
		gtk_tree_path_free (path);
		return;
	} 

	/* No catalog selected. */

	if (window->catalog_path != NULL) {
		g_free (window->catalog_path);
		window->catalog_path = NULL;

		/* Update file list. */

		window_set_file_list (window, NULL, NULL, NULL);
	}

	g_return_if_fail (window->catalog_list->path != NULL);

	/* If directory exists then update. */

	if (path_is_dir (window->catalog_list->path)) {
		catalog_list_refresh (window->catalog_list);
		return;
	}

	/* Else go up one level until a directory exists. */

	base_dir = g_strconcat (g_get_home_dir(),
				"/",
				RC_CATALOG_DIR,
				NULL);
	catalog_dir = g_strdup (window->catalog_list->path);
	
	while ((strcmp (base_dir, catalog_dir) != 0)
	       && ! path_is_dir (catalog_dir)) {
		char *new_dir;
		
		new_dir = remove_level_from_path (catalog_dir);
		g_free (catalog_dir);
		catalog_dir = new_dir;
	}

	window_go_to_catalog_directory (window, catalog_dir);
	
	g_free (catalog_dir);
	g_free (base_dir);
}


/* -- bookmarks & history -- */


typedef struct {
	GThumbWindow *window;
	char         *path;
} BookmarkData;


static BookmarkData *
BookmarkData_new (GThumbWindow *window,
		  const char   *path)
{
	BookmarkData *data;

	data = g_new (BookmarkData, 1);
	data->window = window;
	data->path = g_strdup (path);

	return data;
}


static void
BookmarkData_free (gpointer  user_data,
		   GClosure *closure)
{
	BookmarkData *data = user_data;
	g_free (data->path);
	g_free (data);
}


static void 
bookmark_cb (BonoboUIComponent *uic, 
	     gpointer           user_data, 
	     const char       *verbname)
{
	BookmarkData *data = user_data;	
	GThumbWindow *window = data->window;
	const char   *path = data->path;
	const char  *no_prefix_path;

	no_prefix_path = pref_util_remove_prefix (path);
	
	if (pref_util_location_is_catalog (path) 
	    || pref_util_location_is_search (path)) 
		window_go_to_catalog (window, no_prefix_path);
	else 
		window_go_to_directory (window, no_prefix_path);
}


static void
add_bookmark_menu_item (GThumbWindow *window,
			Bookmarks    *bookmarks,
			char         *prefix,
			int           id,
			const char   *base_path,
			const char   *path)
{
	BonoboUIComponent *ui_component = window->ui_component;
	char              *menu_name;
	char              *cmd;
	char              *cmd_name;
	char              *full_cmd_name;
	char              *label;
	char              *e_label;
	char              *e_tip;
	char              *xml;
	char              *full_path;
	char              *utf8_s;
	BookmarkData      *bookmark_data;
	GdkPixbuf         *pixbuf;

	full_path = g_strdup_printf ("%s%s%d", base_path, prefix, id);
	if (bonobo_ui_component_path_exists (ui_component, full_path, NULL)) {
		g_free (full_path);
		return;
	}
	g_free (full_path);

	/* label */

	menu_name = escape_underscore (bookmarks_get_menu_name (bookmarks, path));
	if (menu_name == NULL)
		menu_name = g_strdup ("???");
	
	label = _g_strdup_with_max_size (menu_name, BOOKMARKS_MENU_MAX_LENGTH);
	utf8_s = g_locale_to_utf8 (label, -1, NULL, NULL, NULL);
	g_free (label);

	e_label = g_markup_escape_text (utf8_s, -1);
	g_free (utf8_s);

	utf8_s = g_locale_to_utf8 (bookmarks_get_menu_tip (bookmarks, path),
				   -1, NULL, NULL, NULL);
	e_tip = g_markup_escape_text (utf8_s, -1);
	g_free (utf8_s);

	/* command */
	
	cmd_name = g_strdup_printf ("%s%d", prefix, id);

	xml = g_strdup_printf ("<menuitem name=\"%s\" verb=\"%s\"\n"
			       " label=\"%s\"\n"
			       " tip=\"%s\" hident=\"0\" />\n", 
			       cmd_name,
			       cmd_name,
			       e_label, 
			       e_tip);
	g_free (e_label);
	g_free (e_tip);

	bonobo_ui_component_set (ui_component, 
				 base_path,
				 xml, 
				 NULL);
	g_free (xml);

	cmd = g_strdup_printf ("<cmd name=\"%s\" />", cmd_name);
	bonobo_ui_component_set (ui_component, 
				 "/commands/",
				 cmd, 
				 NULL);
	g_free (cmd);

	bookmark_data = BookmarkData_new (window, path);
	bonobo_ui_component_add_verb_full (ui_component, 
					   cmd_name, 
					   g_cclosure_new (G_CALLBACK (bookmark_cb), 
							   bookmark_data, 
							   BookmarkData_free));
	g_free (cmd_name);

	/* Add the icon */

	full_cmd_name = g_strdup_printf ("/commands/%s%d", prefix, id);

	if (strcmp (menu_name, g_get_home_dir ()) == 0) 
		pixbuf = gtk_widget_render_icon (window->app,
						 GTK_STOCK_HOME,
						 GTK_ICON_SIZE_MENU,
						 NULL);
	else {
		if (pref_util_location_is_catalog (path)) 
			pixbuf = gdk_pixbuf_new_from_inline (-1, catalog_19_rgba, FALSE, NULL);
		else if (pref_util_location_is_search (path))
			pixbuf = gdk_pixbuf_new_from_inline (-1, catalog_search_17_rgba, FALSE, NULL);
		else
			pixbuf = get_folder_pixbuf (MENU_ICON_SIZE);
	}

	bonobo_ui_util_set_pixbuf (ui_component, 
				   full_cmd_name,
				   pixbuf,
				   NULL);
	g_object_unref (pixbuf);
	g_free (full_cmd_name);
	g_free (menu_name);
}


void
remove_bookmark_menu_item (GThumbWindow *window,
			   char         *prefix,
			   int           id,
			   const char   *base_path)
{
        BonoboUIComponent *ui_component = window->ui_component;
	char              *full_path;

	full_path = g_strdup_printf ("%s%s%d", base_path, prefix, id);

        if (bonobo_ui_component_path_exists (ui_component, full_path, NULL)) {
                char *cmd;

                cmd = g_strdup_printf ("/commands/%s%d", prefix, id);

                bonobo_ui_component_rm (ui_component, full_path, NULL);
                bonobo_ui_component_rm (ui_component, cmd, NULL);
                
                g_free (cmd);
        }

	g_free (full_path);
}


void
window_update_bookmark_list (GThumbWindow *window)
{
	GList *scan, *names;
	int    i;

	/* Delete bookmarks menu. */

	for (i = 0; i < window->bookmarks_length; i++)
		remove_bookmark_menu_item (window, 
					   "Bookmark", 
					   i, 
					   "/menu/Bookmarks/BookmarkList/");

	/* Load and sort bookmarks */

	bookmarks_load_from_disk (preferences.bookmarks);
	if (preferences.bookmarks->list == NULL)
		return;

	names = g_list_copy (preferences.bookmarks->list);

	/* Update bookmarks menu. */

	for (i = 0, scan = names; scan; scan = scan->next) {
		add_bookmark_menu_item (window,
					preferences.bookmarks,
					"Bookmark",
					i++,
					"/menu/Bookmarks/BookmarkList/",
					scan->data);
	}
	window->bookmarks_length = i;

	g_list_free (names);

	if (window->bookmarks_dlg != NULL)
		dlg_edit_bookmarks_update (window->bookmarks_dlg);
}


static void
window_update_history_list (GThumbWindow *window)
{
	GList *scan;
	int    i;

	window_update_go_sensitivity (window);

	/* Delete history menu. */

	for (i = 0; i < window->history_length; i++) {
		remove_bookmark_menu_item (window, "History", i, HISTORY_LIST_MENU);
		remove_bookmark_menu_item (window, "HistoryPopup", i, HISTORY_LIST_POPUP);
	}

	/* Update history menu. */

	if (window->history->list == NULL)
		return;

	i = 0;
	for (scan = window->history_current; scan && (i < eel_gconf_get_integer (PREF_MAX_HISTORY_LENGTH)); scan = scan->next) {
		add_bookmark_menu_item (window,
					window->history,
					"History",
					i,
					HISTORY_LIST_MENU,
					scan->data);

		/* this is the history of previous locations, so do not 
		 * insert the current location. */
		if (scan != window->history_current)
			add_bookmark_menu_item (window,
						window->history,
						"HistoryPopup",
						i,
						HISTORY_LIST_POPUP,
						scan->data);
		i++;
	}
	window->history_length = i;
}


/**/

static void window_make_current_image_visible (GThumbWindow *window);


static void
view_image_at_pos (GThumbWindow *window, 
		   int           pos)
{
	char *path;

	path = gth_file_list_path_from_pos (window->file_list, pos);
	if (path == NULL) 
		return;
	window_load_image (window, path);
	g_free (path);
}


/* -- activity -- */


static void
image_loader_progress_cb (ImageLoader *loader,
			  float        p, 
			  gpointer     data)
{
	GThumbWindow *window = data;
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (window->progress), p);
}


static void
image_loader_done_cb (ImageLoader *loader,
		      gpointer     data)
{
	GThumbWindow *window = data;
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (window->progress), 0.0);
}


static char *
get_command_name_from_sidebar_content (GThumbWindow *window)
{
	switch (window->sidebar_content) {
	case DIR_LIST:
		return "/commands/View_ShowFolders";
	case CATALOG_LIST:
		return "/commands/View_ShowCatalogs";
	default:
		return NULL;
	}

	return NULL;
}


static void
_window_set_sidebar (GThumbWindow *window,
		     int           sidebar_content)
{
	char *cname;

	cname = get_command_name_from_sidebar_content (window);
	if (cname != NULL) 
		set_command_state_if_different (window, cname, FALSE, FALSE);

	window->sidebar_content = sidebar_content;

	cname = get_command_name_from_sidebar_content (window);
	if ((cname != NULL) && window->sidebar_visible)
		set_command_state_if_different (window, cname, TRUE, FALSE);

	gtk_notebook_set_current_page (GTK_NOTEBOOK (window->notebook), 
				       sidebar_content - 1);

	window_update_sensitivity (window);
}


static void
make_image_visible (GThumbWindow *window, 
		    int           pos)
{
	GthumbVisibility  visibility;
	ImageList        *ilist = IMAGE_LIST (window->file_list->ilist);

	if ((pos < 0) || (pos >= ilist->images))
		return;

	visibility = image_list_image_is_visible (ilist, pos);
	if (visibility != GTHUMB_VISIBILITY_FULL) {
		double offset;
		
		switch (visibility) {
		case GTHUMB_VISIBILITY_NONE:
			offset = 0.5; 
			break;
		case GTHUMB_VISIBILITY_PARTIAL_TOP:
			offset = 0.0; 
			break;
		case GTHUMB_VISIBILITY_PARTIAL_BOTTOM:
			offset = 1.0; 
			break;
		case GTHUMB_VISIBILITY_PARTIAL:
		case GTHUMB_VISIBILITY_FULL:
			offset = -1.0;
			break;
		}
		if (offset > -1.0)
			image_list_moveto (ilist, pos, offset);
	}
}


static void
window_make_current_image_visible (GThumbWindow *window)
{
	char *path;
	int   pos;

	if (window->setting_file_list || window->changing_directory)
		return;

	path = image_viewer_get_image_filename (IMAGE_VIEWER (window->viewer));
	if (path == NULL)
		return;

	pos = gth_file_list_pos_from_path (window->file_list, path);
	g_free (path);

	if (pos == -1)
		return;
	
	make_image_visible (window, pos);
}



/* -- callbacks -- */


static void
close_window_cb (GtkWidget    *caller, 
		 GdkEvent     *event, 
		 GThumbWindow *window)
{
	window_close (window);
}


static gboolean
sel_change_update_cb (gpointer data)
{
	GThumbWindow *window = data;

	g_source_remove (window->sel_change_timeout);
	window->sel_change_timeout = 0;

	window_update_sensitivity (window);
	window_update_statusbar_list_info (window);

	return FALSE;
}


static int
file_selection_changed_cb (GtkWidget *widget, 
			   gint       pos, 
			   GdkEvent  *event,
			   gpointer   data)
{
	GThumbWindow *window = data;

	if (window->sel_change_timeout != 0)
		g_source_remove (window->sel_change_timeout);

	window->sel_change_timeout = g_timeout_add (SEL_CHANGED_DELAY,
						    sel_change_update_cb,
						    window);

	return TRUE;
}


static void
gth_file_list_focus_image_cb (GtkWidget *widget,
			  int        pos,
			  gpointer   data)
{
	GThumbWindow *window = data;	
	char         *focused_image;

	if (window->setting_file_list) 
		return;

	focused_image = gth_file_list_path_from_pos (window->file_list, pos);

	if (focused_image == NULL)
		return;

	if (window->image_path == NULL) {
		g_free (focused_image);
		return;
	}

	if ((window->image_path != NULL) 
	    && (strcmp (focused_image, window->image_path) != 0))
		view_image_at_pos (window, pos);

	g_free (focused_image);
}


static int
gth_file_list_button_press_cb (GtkWidget      *widget, 
			   GdkEventButton *event,
			   gpointer        data)
{
	GThumbWindow *window = data;

	if (event->type == GDK_3BUTTON_PRESS) 
		return FALSE;

	if ((event->button != 1) && (event->button != 3))
		return FALSE;

	if ((event->state & GDK_SHIFT_MASK)
	    || (event->state & GDK_CONTROL_MASK))
		return FALSE;

	if (event->button == 1) {
		ImageList *ilist = IMAGE_LIST (window->file_list->ilist);
		int        pos;

		pos = image_list_get_image_at (ilist, event->x, event->y);
		if (pos == -1)
			return FALSE;

		if (event->type == GDK_2BUTTON_PRESS) {
			return TRUE;
		}

		if (event->type == GDK_BUTTON_PRESS) {
			make_image_visible (window, pos);
			view_image_at_pos (window, pos);
			return TRUE;
		}

	} else if (event->button == 3) {
		ImageList *ilist = IMAGE_LIST (window->file_list->ilist);
		GtkWidget *popup_menu;
		int        pos;

		if (window->popup_menu != NULL)
			gtk_widget_destroy (window->popup_menu);
		window->popup_menu = popup_menu = gtk_menu_new ();

		pos = image_list_get_image_at (ilist, event->x, event->y);
		
		if (pos != -1) {
			if (! gth_file_list_is_selected (window->file_list, pos))
				gth_file_list_select_image_by_pos (window->file_list, pos);
		} else
			gth_file_list_unselect_all (window->file_list);

		window_update_sensitivity (window);

		bonobo_window_add_popup (BONOBO_WINDOW (window->app), 
					 GTK_MENU (popup_menu), 
					 "/popups/FilePopup");

		gtk_menu_popup (GTK_MENU (popup_menu),
				NULL,                                   
				NULL,                                   
				NULL,
				NULL,
				3,                               
				event->time);

		return TRUE;
	}

	return FALSE;
}


static gboolean 
hide_sidebar_idle (gpointer data) 
{
	GThumbWindow *window = data;
	window_hide_sidebar (window);
	return FALSE;
}


static int 
gth_file_list_double_click_cb (GtkWidget  *widget, 
			   int         idx,
			   gpointer    data)
{
	GThumbWindow *window = data;

	/* use a timeout to avoid that the viewer gets
	 * the button press event. */
	g_timeout_add (HIDE_SIDEBAR_DELAY, hide_sidebar_idle, window);
	
	return TRUE;
}


static void
dir_list_activated_cb (GtkTreeView       *tree_view,
		       GtkTreePath       *path,
		       GtkTreeViewColumn *column,
		       gpointer           data)
{
	GThumbWindow *window = data;
	char         *new_dir;

	new_dir = dir_list_get_path_from_tree_path (window->dir_list, path);
	window_go_to_directory (window, new_dir);
	g_free (new_dir);
}


/**/


static int
dir_list_button_press_cb (GtkWidget      *widget,
			  GdkEventButton *event,
			  gpointer        data)
{
	GThumbWindow *window     = data;
	GtkWidget    *treeview   = window->dir_list->list_view;
	GtkListStore *list_store = window->dir_list->list_store;
	GtkTreePath  *path;
	GtkTreeIter   iter;

	if (dir_list_tree_path != NULL) {
		gtk_tree_path_free (dir_list_tree_path);
		dir_list_tree_path = NULL;
	}

	if ((event->state & GDK_SHIFT_MASK) 
	    || (event->state & GDK_CONTROL_MASK))
		return FALSE;

	if ((event->button != 1) & (event->button != 3))
		return FALSE;

	if (! gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (treeview),
					     event->x, event->y,
					     &path, NULL, NULL, NULL))
		return FALSE;
	
	if (! gtk_tree_model_get_iter (GTK_TREE_MODEL (list_store), 
				       &iter, 
				       path)) {
		gtk_tree_path_free (path);
		return FALSE;
	}

	dir_list_tree_path = gtk_tree_path_copy (path);
	gtk_tree_path_free (path);

	return FALSE;
}


static int
dir_list_button_release_cb (GtkWidget      *widget,
			    GdkEventButton *event,
			    gpointer        data)
{
	GThumbWindow *window     = data;
	GtkWidget    *treeview   = window->dir_list->list_view;
	GtkListStore *list_store = window->dir_list->list_store;
	GtkTreePath  *path;
	GtkTreeIter   iter;

	if ((event->state & GDK_SHIFT_MASK) 
	    || (event->state & GDK_CONTROL_MASK))
		return FALSE;

	if ((event->button != 1) & (event->button != 3))
		return FALSE;

	if (! gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (treeview),
					     event->x, event->y,
					     &path, NULL, NULL, NULL))
		return FALSE;
	
	if (! gtk_tree_model_get_iter (GTK_TREE_MODEL (list_store), 
				       &iter, 
				       path)) {
		gtk_tree_path_free (path);
		return FALSE;
	}

	if ((dir_list_tree_path == NULL)
	    || gtk_tree_path_compare (dir_list_tree_path, path) != 0) {
		gtk_tree_path_free (path);
		return FALSE;
	}

	gtk_tree_path_free (path);

	if ((event->button == 1) 
	    && (pref_get_real_click_policy () == CLICK_POLICY_SINGLE)) {
		char *new_dir;

		new_dir = dir_list_get_path_from_iter (window->dir_list, 
						       &iter);
		window_go_to_directory (window, new_dir);
		g_free (new_dir);

		return FALSE;

	} else if (event->button == 3) {
		GtkTreeSelection *selection;
		GtkWidget        *popup_menu;
		char             *utf8_name;
		char             *name;

		utf8_name = dir_list_get_name_from_iter (window->dir_list, &iter);
		name = g_locale_from_utf8 (utf8_name, -1, NULL, NULL, NULL);
		g_free (utf8_name);

		if (strcmp (name, "..") == 0) {
			g_free (name);
			return TRUE;
		}
		g_free (name);

		/* Update selection. */

		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (window->dir_list->list_view));
		if (selection == NULL)
			return FALSE;

		if (! gtk_tree_selection_iter_is_selected (selection, &iter)) {
			gtk_tree_selection_unselect_all (selection);
			gtk_tree_selection_select_iter (selection, &iter);
                }

		/* Popup menu. */

		if (window->popup_menu != NULL)
			gtk_widget_destroy (window->popup_menu);
		window->popup_menu = popup_menu = gtk_menu_new ();

		window_update_sensitivity (window);
		bonobo_window_add_popup (BONOBO_WINDOW (window->app), 
					 GTK_MENU (popup_menu), 
					 "/popups/DirPopup");
		gtk_menu_popup (GTK_MENU (popup_menu),
				NULL,
				NULL,
				NULL,
				NULL,
				3,
				event->time);
		return TRUE;
	}

	return FALSE;
}


/* directory or catalog list selection changed. */

static void
dir_or_catalog_sel_changed_cb (GtkTreeSelection *selection,
			       gpointer          p)
{
	GThumbWindow *window = p;
	window_update_sensitivity (window);
}


static void
add_history_item (GThumbWindow *window,
		  const char   *path,
		  const char   *prefix)
{
	char *location;

	bookmarks_remove_from (window->history, window->history_current);

	location = g_strconcat (prefix, path, NULL);
	bookmarks_remove_all_instances (window->history, location);
	bookmarks_add (window->history, location, FALSE, FALSE);
	g_free (location);

	window->history_current = window->history->list;
}


static void
catalog_activate_continue (gpointer data)
{
	GThumbWindow *window = data;

	window_update_sensitivity (window);

	/* Add to history list if not present as last entry. */

	if ((window->go_op == WINDOW_GO_TO)
	    && ((window->history_current == NULL) 
		|| ((window->catalog_path != NULL)
		    && (strcmp (window->catalog_path, pref_util_remove_prefix (window->history_current->data)) != 0)))) {
		GtkTreeIter iter;
		gboolean    is_search;

		if (! catalog_list_get_iter_from_path (window->catalog_list,
						       window->catalog_path,
						       &iter)) 
			return;
		is_search = catalog_list_is_search (window->catalog_list, &iter);
		add_history_item (window,
				  window->catalog_path,
				  is_search ? SEARCH_PREFIX : CATALOG_PREFIX);
	} else 
		window->go_op = WINDOW_GO_TO;

	window_update_history_list (window);
	window_update_title (window);
}


static void
catalog_activate (GThumbWindow *window, 
		  const char   *cat_path)
{
	if (path_is_dir (cat_path)) {
		window_go_to_catalog (window, NULL);
		window_go_to_catalog_directory (window, cat_path);
	} else {
		Catalog *catalog = catalog_new ();
		GError  *gerror;

		if (! catalog_load_from_disk (catalog, cat_path, &gerror)) {
			_gtk_error_dialog_from_gerror_run (GTK_WINDOW (window->app), &gerror);
			catalog_free (catalog);

			return;
		}

		window_set_file_list (window, 
				      catalog->list,
				      catalog_activate_continue,
				      window);

		catalog_free (catalog);
		if (window->catalog_path != cat_path) {
			if (window->catalog_path)
				g_free (window->catalog_path);
			window->catalog_path = g_strdup (cat_path);
		}
	}
}


static void
catalog_list_activated_cb (GtkTreeView       *tree_view,
			   GtkTreePath       *path,
			   GtkTreeViewColumn *column,
			   gpointer           data)
{
	GThumbWindow *window = data;
	char         *cat_path;

	cat_path = catalog_list_get_path_from_tree_path (window->catalog_list,
							 path);
	if (cat_path == NULL)
		return;
	catalog_activate (window, cat_path);
	g_free (cat_path);
}


static int
catalog_list_button_press_cb (GtkWidget      *widget, 
			      GdkEventButton *event,
			      gpointer        data)
{
	GThumbWindow *window     = data;
	GtkWidget    *treeview   = window->catalog_list->list_view;
	GtkListStore *list_store = window->catalog_list->list_store;
	GtkTreeIter   iter;
	GtkTreePath  *path;

	if (catalog_list_tree_path != NULL) {
		gtk_tree_path_free (catalog_list_tree_path);
		catalog_list_tree_path = NULL;
	}

	if ((event->state & GDK_SHIFT_MASK) 
	    || (event->state & GDK_CONTROL_MASK))
		return FALSE;

	if ((event->button != 1) & (event->button != 3))
		return FALSE;

	/* Get the path. */

	if (! gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (treeview),
					     event->x, event->y,
					     &path, NULL, NULL, NULL))
		return FALSE;

	if (! gtk_tree_model_get_iter (GTK_TREE_MODEL (list_store), 
				       &iter, 
				       path)) {
		gtk_tree_path_free (path);
		return FALSE;
	}

	catalog_list_tree_path = gtk_tree_path_copy (path);
	gtk_tree_path_free (path);

	return FALSE;
}


static int
catalog_list_button_release_cb (GtkWidget      *widget, 
				GdkEventButton *event,
				gpointer        data)
{
	GThumbWindow *window     = data;
	GtkWidget    *treeview   = window->catalog_list->list_view;
	GtkListStore *list_store = window->catalog_list->list_store;
	GtkTreeIter   iter;
	GtkTreePath  *path;

	if ((event->state & GDK_SHIFT_MASK) 
	    || (event->state & GDK_CONTROL_MASK))
		return FALSE;

	if ((event->button != 1) & (event->button != 3))
		return FALSE;

	/* Get the path. */

	if (! gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (treeview),
					     event->x, event->y,
					     &path, NULL, NULL, NULL))
		return FALSE;

	if (! gtk_tree_model_get_iter (GTK_TREE_MODEL (list_store), 
				       &iter, 
				       path)) {
		gtk_tree_path_free (path);
		return FALSE;
	}

	if (gtk_tree_path_compare (catalog_list_tree_path, path) != 0) {
		gtk_tree_path_free (path);
		return FALSE;
	}

	gtk_tree_path_free (path);

	/**/

	if ((event->button == 1) && 
	    (pref_get_real_click_policy () == CLICK_POLICY_SINGLE)) {
		char *cat_path;

		cat_path = catalog_list_get_path_from_iter (window->catalog_list, &iter);
		g_return_val_if_fail (cat_path != NULL, FALSE);
		catalog_activate (window, cat_path);
		g_free (cat_path);

		return FALSE;

	} else if (event->button == 3) {
		GtkTreeSelection *selection;
		GtkWidget        *popup_menu;
		char             *utf8_name;
		char             *name;

		utf8_name = catalog_list_get_name_from_iter (window->catalog_list, &iter);
		name = g_locale_from_utf8 (utf8_name, -1, NULL, NULL, NULL);
		g_free (utf8_name);

		if (strcmp (name, "..") == 0) {
			g_free (name);
			return TRUE;
		}
		g_free (name);

		/* Update selection. */

		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (window->catalog_list->list_view));
		if (selection == NULL)
			return FALSE;

		if (! gtk_tree_selection_iter_is_selected (selection, &iter)) {
			gtk_tree_selection_unselect_all (selection);
			gtk_tree_selection_select_iter (selection, &iter);
                }

		/* Popup menu. */
		
		if (window->popup_menu != NULL)
			gtk_widget_destroy (window->popup_menu);
		window->popup_menu = popup_menu = gtk_menu_new ();

		window_update_sensitivity (window);

		if (catalog_list_is_dir (window->catalog_list, &iter))
			bonobo_window_add_popup (BONOBO_WINDOW (window->app), 
						 GTK_MENU (popup_menu), 
						 "/popups/LibraryPopup");
		else
			bonobo_window_add_popup (BONOBO_WINDOW (window->app), 
						 GTK_MENU (popup_menu), 
						 "/popups/CatalogPopup");

		gtk_menu_popup (GTK_MENU (popup_menu),
				NULL,
				NULL,
				NULL,
				NULL,
				3,
				event->time);
		
		return TRUE;
	}

	return FALSE;
}


/* -- location entry stuff --*/


static char *
get_location (GThumbWindow *window)
{
	char *text;
	char *text2;
	char *l;

	text = _gtk_entry_get_locale_text (GTK_ENTRY (window->location_entry));
	text2 = remove_special_dirs_from_path (text);
	g_free (text);

	if (text2 == NULL)
		return NULL;

	if (window->sidebar_content == DIR_LIST)
		l = g_strdup (text2);
	else {
		if (strcmp (text2, "/") == 0) {
			char *base = get_catalog_full_path (NULL);
			l = g_strconcat (base, "/", NULL);
			g_free (base);
		} else {
			if (*text2 == '/')
				l = get_catalog_full_path (text2 + 1);
			else
				l = get_catalog_full_path (text2);
		}
	}
	g_free (text2);

	return l;
}


static void
set_location (GThumbWindow *window,
	      const char   *location)
{
	const char *l;
	char       *abs_location;

	abs_location = remove_special_dirs_from_path (location);
	if (abs_location == NULL)
		return;

	if (window->sidebar_content == DIR_LIST)
		l = abs_location;
	else {
		char *base = get_catalog_full_path (NULL);

		if (strlen (abs_location) == strlen (base))
			l = "/";
		else
			l = abs_location + strlen (base);
		g_free (base);
	}

	if (l) {
		char *utf8_l;
		utf8_l = g_locale_to_utf8 (l, -1, NULL, NULL, NULL);
		gtk_entry_set_text (GTK_ENTRY (window->location_entry), utf8_l);
		gtk_editable_set_position (GTK_EDITABLE (window->location_entry), g_utf8_strlen (utf8_l, -1));
		g_free (utf8_l);
	} else
		gtk_entry_set_text (GTK_ENTRY (window->location_entry), NULL);

	g_free (abs_location);
}


static gboolean
location_is_new (GThumbWindow *window, 
		 const char   *text)
{
	if (window->sidebar_content == DIR_LIST)
		return (window->dir_list->path != NULL)
			&& strcmp (window->dir_list->path, text);
	else
		return (window->catalog_list->path != NULL)
			&& strcmp (window->catalog_list->path, text);
}


static void
go_to_location (GThumbWindow *window, 
		const char   *text)
{
	if (window->sidebar_content == DIR_LIST)
		window_go_to_directory (window, text);
	else {
		window_go_to_catalog (window, NULL);
		window_go_to_catalog_directory (window, text);
	}
}


static gint
location_entry_key_press_cb (GtkWidget    *widget, 
			     GdkEventKey  *event,
			     GThumbWindow *window)
{
	char *path;
	int   n;

	g_return_val_if_fail (window != NULL, FALSE);
	
	switch (event->keyval) {
	case GDK_Return:
	case GDK_KP_Enter:
		path = get_location (window);
		if (path != NULL) {
			go_to_location (window, path);
			g_free (path);
		}
                return FALSE;

	case GDK_Tab:
		if ((event->state & GDK_CONTROL_MASK) != GDK_CONTROL_MASK)
			return FALSE;
		
		path = get_location (window);
		n = auto_compl_get_n_alternatives (path);

		if (n > 0) { 
			char *text;
			text = auto_compl_get_common_prefix ();

			if (n == 1) {
				auto_compl_hide_alternatives ();
				if (location_is_new (window, text))
					go_to_location (window, text);
				else {
					/* Add a separator at the end. */
					char *new_path;
					int   len = strlen (path);

					if (strcmp (path, text) != 0) {
						/* Reset the right name. */
						set_location (window, text);
						g_free (path);
						return TRUE;
					}
					
					/* Ending separator, do nothing. */
					if ((len <= 1) 
					    || (path[len - 1] == '/')) {
						g_free (path);
						return TRUE;
					}

					new_path = g_strconcat (path,
								"/",
								NULL);
					set_location (window, new_path);
					g_free (new_path);

					/* Re-Tab */
					gtk_widget_event (widget, (GdkEvent*)event);
				}
			} else {
				set_location (window, text);
				auto_compl_show_alternatives (window, widget);
			}

			if (text)
				g_free (text);
		}
		g_free (path);

		return TRUE;
	}

	return FALSE;
}


/* -- */


static void
image_loaded_cb (GtkWidget    *widget, 
		 GThumbWindow *window)
{
	window->image_mtime = get_file_mtime (window->image_path);
	window->image_modified = FALSE;

	window_update_infobar (window);
	window_update_statusbar_image_info (window);
	window_update_title (window);
	window_update_sensitivity (window);

	if (window->image_prop_dlg != NULL)
		dlg_image_prop_update (window->image_prop_dlg);
}


static void
image_requested_error_cb (GtkWidget    *widget, 
			  GThumbWindow *window)
{
	window->image_mtime = 0;
	window->image_modified = FALSE;

	image_viewer_set_void (IMAGE_VIEWER (window->viewer));

	window_update_infobar (window);
	window_update_title (window);
	window_update_statusbar_image_info (window);
	window_update_sensitivity (window);

	if (window->image_prop_dlg != NULL)
		dlg_image_prop_update (window->image_prop_dlg);
}


static void
image_requested_done_cb (GtkWidget    *widget, 
			 GThumbWindow *window)
{
	ImageLoader *loader;

	loader = gthumb_preloader_get_loader (window->preloader, window->image_path);
	if (loader != NULL) 
		image_viewer_load_from_image_loader (IMAGE_VIEWER (window->viewer), loader);
}


static gint
zoom_changed_cb (GtkWidget    *widget, 
		 GThumbWindow *window)
{
	window_update_statusbar_image_info (window);
	return TRUE;	
}


static gint
size_changed_cb (GtkWidget    *widget, 
		 GThumbWindow *window)
{
	GtkAdjustment *vadj, *hadj;
	gboolean       hide_vscr, hide_hscr;

	vadj = IMAGE_VIEWER (window->viewer)->vadj;
	hadj = IMAGE_VIEWER (window->viewer)->hadj;

	hide_vscr = vadj->upper <= vadj->page_size;
	hide_hscr = hadj->upper <= hadj->page_size;

	if (hide_vscr && hide_hscr) {
		gtk_widget_hide (window->viewer_vscr); 
		gtk_widget_hide (window->viewer_hscr); 
		gtk_widget_hide (window->viewer_event_box);
	} else {
		gtk_widget_show (window->viewer_vscr); 
		gtk_widget_show (window->viewer_hscr); 
		gtk_widget_show (window->viewer_event_box);
	}

	return TRUE;	
}


static void
toggle_image_preview_visibility (GThumbWindow *window)
{
	if (window->sidebar_visible) {
		if (window->image_pane_visible) 
			window_hide_image_pane (window);
		else
			window_show_image_pane (window);
	} else {
		window->image_preview_visible = ! window->image_preview_visible;
		/* Sync menu and toolbar. */
		set_command_state_if_different (window, 
						"/commands/View_ShowPreview", 
						window->image_preview_visible, 
						FALSE);
	}
}


static void
window_enable_thumbs (GThumbWindow *window,
		      gboolean      enable)
{
	set_command_state_without_notifing (window, "View_Thumbnails", enable);
	gth_file_list_enable_thumbs (window->file_list, enable);
	set_command_sensitive (window, "Go_Stop", 
			       ((window->activity_ref > 0) 
				|| window->setting_file_list
				|| window->changing_directory
				|| window->file_list->doing_thumbs));
}


static gint
key_press_cb (GtkWidget   *widget, 
	      GdkEventKey *event,
	      gpointer     data)
{
	GThumbWindow *window = data;
	ImageViewer  *viewer = IMAGE_VIEWER (window->viewer);
	gboolean      sel_not_null;
	gboolean      image_is_void;

	if (image_list_editing (IMAGE_LIST (window->file_list->ilist)))
		return FALSE;

	if (GTK_WIDGET_HAS_FOCUS (window->location_entry))
		return FALSE;

	if ((event->state & GDK_CONTROL_MASK) || (event->state & GDK_MOD1_MASK))
		return FALSE;

	sel_not_null = ilist_utils_selection_not_null (IMAGE_LIST (window->file_list->ilist));
	image_is_void = image_viewer_is_void (IMAGE_VIEWER (window->viewer));

	switch (event->keyval) {
		/* Hide/Show sidebar. */
	case GDK_Return:
	case GDK_KP_Enter:
		if (window->sidebar_visible) 
			window_hide_sidebar (window);
		else
			window_show_sidebar (window);
		return TRUE;

		/* Hide/Show image pane. */
	case GDK_q:
		toggle_image_preview_visibility (window);
		return TRUE;

		/* Full screen view. */
	case GDK_v:
	case GDK_F11:
		fullscreen_start (fullscreen, window);
		return TRUE;

		/* View/hide thumbnails. */
	case GDK_t:
		window_enable_thumbs (window, ! window->file_list->enable_thumbs);
		return TRUE;

		/* Zoom in. */
	case GDK_plus:
	case GDK_equal:
	case GDK_KP_Add:
		image_viewer_zoom_in (viewer);
		return TRUE;

		/* Zoom out. */
	case GDK_minus:
	case GDK_KP_Subtract:
		image_viewer_zoom_out (viewer);
		return TRUE;
		
		/* Actual size. */
	case GDK_KP_Divide:
	case GDK_1:
	case GDK_z:
		image_viewer_set_zoom (viewer, 1.0);
		return TRUE;

		/* Zoom to fit */
	case GDK_x:
		view_zoom_fit_command_impl (NULL, window, NULL);
		return TRUE;

		/* Start/Stop Slideshow. */
        case GDK_s:
		if (! window->slideshow)
			window_start_slideshow (window);
		else
			window_stop_slideshow (window);
                break;

		/* Play animation */
	case GDK_g:
		set_command_state_if_different (window, 
						"/commands/View_PlayAnimation",
						! viewer->play_animation,
						TRUE);
		return TRUE;

		/* Step animation */
	case GDK_j:
		view_step_ani_command_impl (NULL, window, NULL);
		return TRUE;

		/* Show previous image. */
	case GDK_p:
	case GDK_b:
	case GDK_BackSpace:
		window_show_prev_image (window);
		return TRUE;

		/* Show next image. */
	case GDK_n:
		window_show_next_image (window);
		return TRUE;

	case GDK_space:
		if (! GTK_WIDGET_HAS_FOCUS (window->dir_list->list_view)
		    && ! GTK_WIDGET_HAS_FOCUS (window->catalog_list->list_view)) {
			window_show_next_image (window);
			return TRUE;
		}
		break;

		/* Rotate image */
	case GDK_bracketright:
	case GDK_r: 
		alter_image_rotate_command_impl (NULL, window, NULL);
		return TRUE;
			
		/* Flip image */
	case GDK_f:
		alter_image_flip_command_impl (NULL, window, NULL);
		return TRUE;

		/* Mirror image */
	case GDK_m:
		alter_image_mirror_command_impl (NULL, window, NULL);
		return TRUE;

		/* Rotate image counter-clockwise */
	case GDK_bracketleft:
		alter_image_rotate_cc_command_impl (NULL, window, NULL);
		return TRUE;
			
		/* Delete selection. */
	case GDK_Delete: 
	case GDK_KP_Delete:
		if (! sel_not_null)
			break;
		
		if (window->sidebar_content == DIR_LIST)
			edit_delete_files_command_impl (NULL, window, NULL);
		else if (window->sidebar_content == CATALOG_LIST)
			edit_remove_from_catalog_command_impl (NULL, window, NULL);
		return TRUE;

		/* Open images. */
	case GDK_o:
		if (! sel_not_null)
			break;

		file_open_with_command_impl  (NULL, window, NULL);
		return TRUE;

		/* Go up one level */
	case GDK_u:
		window_go_up (window);
		return TRUE;

		/* Go home */
	case GDK_h:
		go_home_command_impl (NULL, window, NULL);
		return TRUE;

		/* Edit comment */
	case GDK_c:
		if (! sel_not_null)
			break;

		edit_edit_comment_command_impl (NULL, window, NULL);
		return TRUE;

		/* Edit categories */
	case GDK_k:
		if (! sel_not_null)
			break;

		edit_edit_categories_command_impl (NULL, window, NULL);
		return TRUE;

		/* Image Properties */
	case GDK_i:
		if (image_is_void)
			break;

		window_show_image_prop (window);
		return TRUE;

	default:
		break;
	}

	if ((event->keyval == GDK_F10) 
	    && (event->state & GDK_SHIFT_MASK)) {

		if ((window->sidebar_content == CATALOG_LIST)
		    && GTK_WIDGET_HAS_FOCUS (window->catalog_list->list_view)) {
			GtkTreeSelection *selection;
			GtkTreeIter       iter;
			GtkWidget        *popup_menu;
			char             *name, *utf8_name;

			selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (window->catalog_list->list_view));
			if (! gtk_tree_selection_get_selected (selection, NULL, &iter))
				return FALSE;

			utf8_name = catalog_list_get_name_from_iter (window->catalog_list, &iter);
			name = g_locale_from_utf8 (utf8_name, -1, NULL, NULL, NULL);
			g_free (utf8_name);

			if (strcmp (name, "..") == 0) 
				return TRUE;

			if (window->popup_menu != NULL)
				gtk_widget_destroy (window->popup_menu);
			window->popup_menu = popup_menu = gtk_menu_new ();
			
			if (catalog_list_is_dir (window->catalog_list, &iter))
				bonobo_window_add_popup (BONOBO_WINDOW (window->app), 
							 GTK_MENU (popup_menu), 
							 "/popups/LibraryPopup");
			else
				bonobo_window_add_popup (BONOBO_WINDOW (window->app), 
							 GTK_MENU (popup_menu), 
							 "/popups/CatalogPopup");
			
			gtk_menu_popup (GTK_MENU (popup_menu),
					NULL,
					NULL,
					NULL,
					NULL,
					3,
					event->time);
			
			return TRUE;
			
		} else if ((window->sidebar_content == DIR_LIST)
			   && GTK_WIDGET_HAS_FOCUS (window->dir_list->list_view)) {
			GtkTreeSelection *selection;
			GtkTreeIter       iter;
			GtkWidget        *popup_menu;
			char             *name, *utf8_name;

			selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (window->dir_list->list_view));
			if (! gtk_tree_selection_get_selected (selection, NULL, &iter))
				return FALSE;

			utf8_name = dir_list_get_name_from_iter (window->dir_list, &iter);
			name = g_locale_from_utf8 (utf8_name, -1, NULL, NULL, NULL);
			g_free (utf8_name);

			if (strcmp (name, "..") == 0) 
				return TRUE;

			if (window->popup_menu != NULL)
				gtk_widget_destroy (window->popup_menu);
			window->popup_menu = popup_menu = gtk_menu_new ();

			bonobo_window_add_popup (BONOBO_WINDOW (window->app), 
						 GTK_MENU (popup_menu), 
						 "/popups/DirPopup");
			
			gtk_menu_popup (GTK_MENU (popup_menu),
					NULL,
					NULL,
					NULL,
					NULL,
					3,
					event->time);
			
			return TRUE;
		} else if (GTK_WIDGET_HAS_FOCUS (window->file_list->ilist)) {
			GtkWidget *popup_menu;
			
			if (window->popup_menu != NULL)
				gtk_widget_destroy (window->popup_menu);
			window->popup_menu = popup_menu = gtk_menu_new ();
			
			window_update_sensitivity (window);
			
			bonobo_window_add_popup (BONOBO_WINDOW (window->app), 
						 GTK_MENU (popup_menu), 
						 "/popups/FilePopup");

			gtk_menu_popup (GTK_MENU (popup_menu),
					NULL,                                   
					NULL,                                   
					NULL,
					NULL,
					3,                               
					event->time);
			
			return TRUE;
		}
	}

	return FALSE;
}


static gboolean
image_clicked_cb (GtkWidget    *widget, 
		  GThumbWindow *window)
{
	window_show_next_image (window);
	return TRUE;
}


static int
image_button_press_cb (GtkWidget      *widget, 
		       GdkEventButton *event,
		       gpointer        data)
{
	GThumbWindow *window = data;
	GtkWidget    *popup_menu;

	switch (event->button) {
	case 1:
		break;
	case 2:
		break;
	case 3:
		if (window->popup_menu != NULL)
			gtk_widget_destroy (window->popup_menu);
		window->popup_menu = popup_menu = gtk_menu_new ();

		if (window->fullscreen)
			bonobo_window_add_popup (BONOBO_WINDOW (window->app), 
						 GTK_MENU (popup_menu), 
						 "/popups/FullscreenImagePopup");
		else
			bonobo_window_add_popup (BONOBO_WINDOW (window->app), 
						 GTK_MENU (popup_menu), 
						 "/popups/ImagePopup");
		gtk_menu_popup (GTK_MENU (popup_menu),
				NULL,
				NULL,
				NULL,
				NULL,
				3,
				event->time);
		return TRUE;
	}

	return FALSE;
}


static int
image_button_release_cb (GtkWidget      *widget, 
			 GdkEventButton *event,
			 gpointer        data)
{
	GThumbWindow *window = data;

	switch (event->button) {
	case 2:
		window_show_prev_image (window);
		return TRUE;
	default:
		break;
	}

	return FALSE;
}


static gboolean
image_focus_changed_cb (GtkWidget     *widget,
			GdkEventFocus *event,
			gpointer       data)
{
	GThumbWindow *window = data;

	gthumb_info_bar_set_focused (GTHUMB_INFO_BAR (window->info_bar),
				     GTK_WIDGET_HAS_FOCUS (window->viewer));

	return FALSE;
}


/* -- drag & drop -- */


static GList *
get_file_list_from_url_list (char *url_list)
{
	GList *list = NULL;
	int    i;
	char  *url_start, *url_end;

	url_start = url_list;
	while (url_start[0] != '\0') {
		char *escaped;
		char *unescaped;

		if (strncmp (url_start, "file:", 5) == 0) {
			url_start += 5;
			if ((url_start[0] == '/') 
			    && (url_start[1] == '/')) url_start += 2;
		}

		i = 0;
		while ((url_start[i] != '\0')
		       && (url_start[i] != '\r')
		       && (url_start[i] != '\n')) i++;
		url_end = url_start + i;

		escaped = g_strndup (url_start, url_end - url_start);
		unescaped = gnome_vfs_unescape_string_for_display (escaped);
		g_free (escaped);

		list = g_list_prepend (list, unescaped);

		url_start = url_end;
		i = 0;
		while ((url_start[i] != '\0')
		       && ((url_start[i] == '\r')
			   || (url_start[i] == '\n'))) i++;
		url_start += i;
	}
	
	return g_list_reverse (list);
}


static void  
viewer_drag_data_get  (GtkWidget        *widget,
		       GdkDragContext   *context,
		       GtkSelectionData *selection_data,
		       guint             info,
		       guint             time,
		       gpointer          data)
{
	GThumbWindow *window = data;
	char         *path;

	if (IMAGE_VIEWER (window->viewer)->is_void) 
		return;

	path = image_viewer_get_image_filename (IMAGE_VIEWER (window->viewer));

	gtk_selection_data_set (selection_data,
				selection_data->target,
				8, 
				path, strlen (path));
	g_free (path);
}


static void  
viewer_drag_data_received  (GtkWidget          *widget,
			    GdkDragContext     *context,
			    int                 x,
			    int                 y,
			    GtkSelectionData   *data,
			    guint               info,
			    guint               time,
			    gpointer            extra_data)
{
	GThumbWindow *window = extra_data;
	Catalog      *catalog;
	char         *catalog_path;
	char         *catalog_name;
	GList        *list;
	GList        *scan;
	GError       *gerror;
	gboolean      empty = TRUE;

	if (! ((data->length >= 0) && (data->format == 8))) {
		gtk_drag_finish (context, FALSE, FALSE, time);
		return;
	}

	gtk_drag_finish (context, TRUE, FALSE, time);

	list = get_file_list_from_url_list ((char *) data->data);

	/* Create a catalog with the Drag&Drop list. */

	catalog = catalog_new ();
	catalog_name = g_strconcat (_("Dragged Images"),
				    CATALOG_EXT,
				    NULL);
	catalog_path = get_catalog_full_path (catalog_name);
	g_free (catalog_name);

	catalog_set_path (catalog, catalog_path);

	for (scan = list; scan; scan = scan->next) {
		char *filename = scan->data;
		if (path_is_file (filename)) {
			catalog_add_item (catalog, filename);
			empty = FALSE;
		}
	}

	if (! empty) {
		if (! catalog_write_to_disk (catalog, &gerror)) 
			_gtk_error_dialog_from_gerror_run (GTK_WINDOW (window->app), &gerror);
		else {
			/* View the Drag&Drop catalog. */
			ViewFirstImage = TRUE;
			window_go_to_catalog (window, catalog_path);
		}
	}

	catalog_free (catalog);
	path_list_free (list);
	g_free (catalog_path);
}


static gint
viewer_key_press_cb (GtkWidget   *widget, 
		     GdkEventKey *event,
		     gpointer     data)
{
	GThumbWindow *window = data;

	switch (event->keyval) {
	case GDK_Page_Up:
		window_show_prev_image (window);
		return TRUE;

	case GDK_Page_Down:
		window_show_next_image (window);
		return TRUE;

	case GDK_Home:
		window_show_first_image (window);
		return TRUE;

	case GDK_End:
		window_show_last_image (window);
		return TRUE;

	case GDK_F10:
		if (event->state & GDK_SHIFT_MASK) {
			GtkWidget *popup_menu;

			if (window->popup_menu != NULL)
				gtk_widget_destroy (window->popup_menu);
			window->popup_menu = popup_menu = gtk_menu_new ();

			bonobo_window_add_popup (BONOBO_WINDOW (window->app), 
						 GTK_MENU (popup_menu), 
						 "/popups/ImagePopup");
			gtk_menu_popup (GTK_MENU (popup_menu),
					NULL,
					NULL,
					NULL,
					NULL,
					3,
					event->time);
			return TRUE;
		}
	}
	
	return FALSE;
}


static gboolean
info_bar_clicked_cb (GtkWidget      *widget,
		     GdkEventButton *event,
		     GThumbWindow   *window)
{
	gtk_widget_grab_focus (window->viewer);
	return TRUE;
}


static GString*
make_url_list (GList    *list, 
	       int       target)
{
	GList      *scan;
	GString    *result;
	const char *url_sep;
	const char *prefix;

	if (list == NULL)
		return NULL;

	switch (target) {
	case TARGET_PLAIN:
	case TARGET_URILIST:
		prefix = "file://";
		url_sep = "\r\n"; 
		break;
	}

	result = g_string_new (NULL);
	for (scan = list; scan; scan = scan->next) {
		g_string_append (result, prefix);
		g_string_append (result, scan->data);
		g_string_append (result, url_sep);
	}

	return result;
}


static void  
gth_file_list_drag_data_get  (GtkWidget        *widget,
			      GdkDragContext   *context,
			      GtkSelectionData *selection_data,
			      guint             info,
			      guint             time,
			      gpointer          data)
{
	GThumbWindow *window = data;
	ImageList    *ilist;
	GList        *list;
	GString      *url_list;
	char         *target;
	int           target_id;

	ilist = IMAGE_LIST (window->file_list->ilist);

	target = gdk_atom_name (selection_data->target);
	if (strcmp (target, "text/uri-list") == 0)
		target_id = TARGET_URILIST;
	else if (strcmp (target, "text/plain") == 0)
		target_id = TARGET_PLAIN;
	g_free (target);

	list = ilist_utils_get_file_list_selection (ilist);
	url_list = make_url_list (list, target_id);
	path_list_free (list);

	if (url_list == NULL) 
		return;

	gtk_selection_data_set (selection_data, 
				selection_data->target,
				8, 
				url_list->str, 
				url_list->len);

	g_string_free (url_list, TRUE);	
}


static void
move_items__continue (GnomeVFSResult result,
		      gpointer       data)
{
	GThumbWindow *window = data;

	if (result != GNOME_VFS_OK) 
		_gtk_error_dialog_run (GTK_WINDOW (window->app),
				       "%s %s",
				       _("Could not move the items:"), 
				       gnome_vfs_result_to_string (result));
}


static void  
image_list_drag_data_received  (GtkWidget          *widget,
				GdkDragContext     *context,
				int                 x,
				int                 y,
				GtkSelectionData   *data,
				guint               info,
				guint               time,
				gpointer            extra_data)
{
	GThumbWindow *window = extra_data;
	char         *dest_dir = NULL;
	
	if (! ((data->length >= 0) && (data->format == 8))
	    || (window->sidebar_content != DIR_LIST)) {
		gtk_drag_finish (context, FALSE, FALSE, time);
		return;
	}

	gtk_drag_finish (context, TRUE, FALSE, time);

	dest_dir = window->dir_list->path;

	if (dest_dir != NULL) {
		GList *list;
		list = get_file_list_from_url_list ((char*) data->data);
		dlg_copy_items (window, 
				list,
				dest_dir,
				TRUE,
				TRUE,
				move_items__continue,
				window);
		path_list_free (list);
	}
}


static void  
dir_list_drag_data_received  (GtkWidget          *widget,
			      GdkDragContext     *context,
			      int                 x,
			      int                 y,
			      GtkSelectionData   *data,
			      guint               info,
			      guint               time,
			      gpointer            extra_data)
{
	GThumbWindow            *window = extra_data;
	GtkTreePath             *pos_path;
	GtkTreeViewDropPosition  drop_pos;
	int                      pos;
	char                    *dest_dir = NULL;
	const char              *current_dir;

	if (! ((data->length >= 0) && (data->format == 8))) {
		gtk_drag_finish (context, FALSE, FALSE, time);
		return;
	}

	gtk_drag_finish (context, TRUE, FALSE, time);

	g_return_if_fail (window->sidebar_content == DIR_LIST);

	/**/

	if (gtk_tree_view_get_dest_row_at_pos (GTK_TREE_VIEW (window->dir_list->list_view),
					       x, y,
					       &pos_path,
					       &drop_pos)) {
		pos = gtk_tree_path_get_indices (pos_path)[0];
		gtk_tree_path_free (pos_path);
	} else
		pos = -1;
	
	current_dir = window->dir_list->path;
	
	if (pos == -1) {
		if (current_dir != NULL)
			dest_dir = g_strdup (current_dir);
	} else
		dest_dir = dir_list_get_path_from_row (window->dir_list,
						       pos);
	
	/**/
	
	if (dest_dir != NULL) {
		GList *list;
		list = get_file_list_from_url_list ((char*) data->data);
		dlg_copy_items (window, 
				list,
				dest_dir,
				TRUE,
				TRUE,
				move_items__continue,
				window);
		path_list_free (list);
	}
	
	g_free (dest_dir);
}


static gboolean
dir_list_drag_motion (GtkWidget          *widget,
		      GdkDragContext     *context,
		      gint                x,
		      gint                y,
		      guint               time,
		      gpointer            extra_data)
{
	GThumbWindow            *window = extra_data;
	GtkTreePath             *pos_path;
	GtkTreeViewDropPosition  drop_pos;
	GtkTreeView             *list_view;

	list_view = GTK_TREE_VIEW (window->dir_list->list_view);

	if (! gtk_tree_view_get_dest_row_at_pos (list_view,
						 x, y,
						 &pos_path,
						 &drop_pos)) 
		pos_path = gtk_tree_path_new_first ();

	switch (drop_pos) {
	case GTK_TREE_VIEW_DROP_BEFORE:
	case GTK_TREE_VIEW_DROP_INTO_OR_BEFORE:
		drop_pos = GTK_TREE_VIEW_DROP_INTO_OR_BEFORE;
		break;

	case GTK_TREE_VIEW_DROP_AFTER:
	case GTK_TREE_VIEW_DROP_INTO_OR_AFTER:
		drop_pos = GTK_TREE_VIEW_DROP_INTO_OR_AFTER;
		break;
	}

	gtk_tree_view_set_drag_dest_row  (list_view, pos_path, drop_pos);
	gtk_tree_path_free (pos_path);

	return TRUE;
}


static void
dir_list_drag_begin (GtkWidget          *widget,
		     GdkDragContext     *context,
		     gpointer            extra_data)
{	
	if (dir_list_tree_path != NULL) {
		gtk_tree_path_free (dir_list_tree_path);
		dir_list_tree_path = NULL;
	}
}


static void
dir_list_drag_leave (GtkWidget          *widget,
		     GdkDragContext     *context,
		     guint               time,
		     gpointer            extra_data)
{	
	GThumbWindow  *window = extra_data;
	GtkTreeView   *list_view;

	list_view = GTK_TREE_VIEW (window->dir_list->list_view);
	gtk_tree_view_set_drag_dest_row  (list_view, NULL, 0);
}


static void  
dir_list_drag_data_get  (GtkWidget        *widget,
			 GdkDragContext   *context,
			 GtkSelectionData *selection_data,
			 guint             info,
			 guint             time,
			 gpointer          data)
{
	GThumbWindow     *window = data;
        char             *target;
	char             *path, *uri;
	GtkTreeIter       iter;
	GtkTreeSelection *selection;

        target = gdk_atom_name (selection_data->target);
        if (strcmp (target, "text/uri-list") != 0) {
		g_free (target);
		return;
	}
        g_free (target);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (window->dir_list->list_view));
	if (! gtk_tree_selection_get_selected (selection, NULL, &iter))
		return;

	path = dir_list_get_path_from_iter (window->dir_list, &iter);
	uri = g_strconcat ("file://", path, "\n", NULL);
        gtk_selection_data_set (selection_data, 
                                selection_data->target,
                                8, 
				path,
                                strlen (uri));
	g_free (path);
	g_free (uri);
}


static void  
catalog_list_drag_data_received  (GtkWidget          *widget,
				  GdkDragContext     *context,
				  int                 x,
				  int                 y,
				  GtkSelectionData   *data,
				  guint               info,
				  guint               time,
				  gpointer            extra_data)
{
	GThumbWindow            *window = extra_data;
	GtkTreePath             *pos_path;
	GtkTreeViewDropPosition  drop_pos;
	int                      pos;
	char                    *dest_catalog = NULL;
	const char              *current_catalog;

	if (! ((data->length >= 0) && (data->format == 8))) {
		gtk_drag_finish (context, FALSE, FALSE, time);
		return;
	}

	gtk_drag_finish (context, TRUE, FALSE, time);

	g_return_if_fail (window->sidebar_content == CATALOG_LIST);

	/**/

	if (gtk_tree_view_get_dest_row_at_pos (GTK_TREE_VIEW (window->catalog_list->list_view),
					       x, y,
					       &pos_path,
					       &drop_pos)) {
		pos = gtk_tree_path_get_indices (pos_path)[0];
		gtk_tree_path_free (pos_path);
	} else
		pos = -1;
	
	current_catalog = window->catalog_path;
	
	if (pos == -1) {
		if (current_catalog != NULL)
			dest_catalog = g_strdup (current_catalog);
	} else
		dest_catalog = catalog_list_get_path_from_row (window->catalog_list, pos);
	
	/**/
	
	if (path_is_file (dest_catalog)) {
		GList   *list;
		Catalog *catalog;
		GError  *gerror;
		
		list = get_file_list_from_url_list ((char*) data->data);
		
		catalog = catalog_new ();
		
		if (! catalog_load_from_disk (catalog, dest_catalog, &gerror)) 
			_gtk_error_dialog_from_gerror_run (GTK_WINDOW (window->app), &gerror);
		
		else {
			GList    *scan;
			GList    *files_added = NULL;
			
			for (scan = list; scan; scan = scan->next) {
				char *filename = scan->data;
				if (path_is_file (filename)) {
					catalog_add_item (catalog, filename);
					files_added = g_list_prepend (files_added, filename);
				}
			}
			
			if (! catalog_write_to_disk (catalog, &gerror)) 
				_gtk_error_dialog_from_gerror_run (GTK_WINDOW (window->app), &gerror);
			else 
				all_windows_notify_cat_files_added (dest_catalog, files_added);
			
			g_list_free (files_added);
		}

		catalog_free (catalog);
		path_list_free (list);
	}
	
	g_free (dest_catalog);
}


static gboolean
catalog_list_drag_motion (GtkWidget          *widget,
			  GdkDragContext     *context,
			  gint                x,
			  gint                y,
			  guint               time,
			  gpointer            extra_data)
{
	GThumbWindow            *window = extra_data;
	GtkTreePath             *pos_path;
	GtkTreeViewDropPosition  drop_pos;
	GtkTreeView             *list_view;

	list_view = GTK_TREE_VIEW (window->catalog_list->list_view);

	if (! gtk_tree_view_get_dest_row_at_pos (list_view,
						 x, y,
						 &pos_path,
						 &drop_pos)) 
		pos_path = gtk_tree_path_new_first ();

	switch (drop_pos) {
	case GTK_TREE_VIEW_DROP_BEFORE:
	case GTK_TREE_VIEW_DROP_INTO_OR_BEFORE:
		drop_pos = GTK_TREE_VIEW_DROP_INTO_OR_BEFORE;
		break;

	case GTK_TREE_VIEW_DROP_AFTER:
	case GTK_TREE_VIEW_DROP_INTO_OR_AFTER:
		drop_pos = GTK_TREE_VIEW_DROP_INTO_OR_AFTER;
		break;
	}

	gtk_tree_view_set_drag_dest_row  (list_view, pos_path, drop_pos);
	gtk_tree_path_free (pos_path);

	return TRUE;
}


static void
catalog_list_drag_begin (GtkWidget          *widget,
			 GdkDragContext     *context,
			 gpointer            extra_data)
{	
	if (catalog_list_tree_path != NULL) {
		gtk_tree_path_free (catalog_list_tree_path);
		catalog_list_tree_path = NULL;
	}
}


static void
catalog_list_drag_leave (GtkWidget          *widget,
			 GdkDragContext     *context,
			 guint               time,
			 gpointer            extra_data)
{	
	GThumbWindow            *window = extra_data;
	GtkTreeView             *list_view;

	list_view = GTK_TREE_VIEW (window->catalog_list->list_view);
	gtk_tree_view_set_drag_dest_row  (list_view, NULL, 0);
}


/* -- */


static gboolean
set_busy_cursor_cb (gpointer data)
{
	GThumbWindow *window = data;
	GdkCursor    *cursor;

	if (window->busy_cursor_timeout != 0) {
		g_source_remove (window->busy_cursor_timeout);
		window->busy_cursor_timeout = 0;
	}

	cursor = gdk_cursor_new (GDK_WATCH);
	gdk_window_set_cursor (window->app->window, cursor);
	gdk_cursor_unref (cursor);

	return FALSE;
}


static void
file_list_busy_cb (GthFileList  *file_list,
		   GThumbWindow *window)
{
	if (! GTK_WIDGET_REALIZED (window->app))
		return;

	if (window->busy_cursor_timeout != 0)
		g_source_remove (window->busy_cursor_timeout);

	window->busy_cursor_timeout = g_timeout_add (BUSY_CURSOR_DELAY,
						     set_busy_cursor_cb,
						     window);
}


static void
file_list_idle_cb (GthFileList  *file_list,
		   GThumbWindow *window)
{
	GdkCursor *cursor;

	if (! GTK_WIDGET_REALIZED (window->app))
		return;

	if (window->busy_cursor_timeout != 0) {
		g_source_remove (window->busy_cursor_timeout);
		window->busy_cursor_timeout = 0;
	}

	cursor = gdk_cursor_new (GDK_LEFT_PTR);
	gdk_window_set_cursor (window->app->window, cursor);
	gdk_cursor_unref (cursor);
}


static gboolean
progress_cancel_cb (GtkButton    *button,
		    GThumbWindow *window)
{
	if (window->pixop != NULL)
		gth_pixbuf_op_stop (window->pixop);
	return TRUE;
}


static gboolean
progress_delete_cb (GtkWidget    *caller, 
		    GdkEvent     *event,
		    GThumbWindow *window)
{
	if (window->pixop != NULL)
		gth_pixbuf_op_stop (window->pixop);
	return TRUE;
}


/* -- */


static void
item_toggled_handler (BonoboUIComponent            *ui_component,
		      const char                   *path,
		      Bonobo_UIComponent_EventType  type,
		      const char                   *state,
		      gpointer                      user_data)
{
	GThumbWindow *window = user_data;
        gboolean      s;

	if (window->freeze_toggle_handler > 0) {
		window->freeze_toggle_handler = 0;
		return;
	}

        s = (strcmp (state, "1") == 0);

	if (strcmp (path, "View_Toolbar") == 0) 
		eel_gconf_set_boolean (PREF_UI_TOOLBAR_VISIBLE, s);

	if (strcmp (path, "View_Statusbar") == 0) 
		eel_gconf_set_boolean (PREF_UI_STATUSBAR_VISIBLE, s);

	if ((strcmp (path, "View_ZoomQualityHigh") == 0) && s) {
		image_viewer_set_zoom_quality (IMAGE_VIEWER (window->viewer),
					       ZOOM_QUALITY_HIGH);
		image_viewer_update_view (IMAGE_VIEWER (window->viewer));
	}

	if ((strcmp (path, "View_ZoomQualityLow") == 0) && s) {
		image_viewer_set_zoom_quality (IMAGE_VIEWER (window->viewer),
					       ZOOM_QUALITY_LOW);
		image_viewer_update_view (IMAGE_VIEWER (window->viewer));
	}

	if ((strcmp (path, "SortByName") == 0) && s) 
		gth_file_list_set_sort_method (window->file_list, SORT_BY_NAME);
	if ((strcmp (path, "SortBySize") == 0) && s) 
		gth_file_list_set_sort_method (window->file_list, SORT_BY_SIZE);
	if ((strcmp (path, "SortByTime") == 0) && s) 
		gth_file_list_set_sort_method (window->file_list, SORT_BY_TIME);
	if ((strcmp (path, "SortByPath") == 0) && s) 
		gth_file_list_set_sort_method (window->file_list, SORT_BY_PATH);

	if (strcmp (path, "SortReversed") == 0) {
		GtkSortType new_type;
		
		if (window->file_list->sort_type == GTK_SORT_ASCENDING)
			new_type = GTK_SORT_DESCENDING;
		else
			new_type = GTK_SORT_ASCENDING;
		gth_file_list_set_sort_type (window->file_list, new_type);
	}

	if ((strncmp (path, "TranspType", 10) == 0) && s) {
		TranspType transp_type;

		if (strcmp (path, "TranspTypeWhite") == 0)
			transp_type = TRANSP_TYPE_WHITE;
		else if (strcmp (path, "TranspTypeNone") == 0)
			transp_type = TRANSP_TYPE_NONE;
		else if (strcmp (path, "TranspTypeBlack") == 0)
			transp_type = TRANSP_TYPE_BLACK;
		else if (strcmp (path, "TranspTypeChecked") == 0)
			transp_type = TRANSP_TYPE_CHECKED;

		pref_set_transp_type (transp_type);
	}

	if (strcmp (path, "View_PlayAnimation") == 0) {
		ImageViewer *viewer = IMAGE_VIEWER (window->viewer);
		
		if (! viewer->play_animation)
			image_viewer_start_animation (viewer);
		else
			image_viewer_stop_animation (viewer);
		
		set_command_sensitive (window, "View_StepAnimation", 
				       (image_viewer_is_animation (viewer) 
					&& ! image_viewer_is_playing_animation (viewer)));
	}

	if (strcmp (path, "View_Thumbnails") == 0) 
		window_enable_thumbs (window, ! window->file_list->enable_thumbs);

	if (strcmp (path, "View_ShowPreview") == 0) 
		toggle_image_preview_visibility (window);
}


static void
setup_commands_pixbufs (BonoboUIComponent *ui_component)
{
	GdkPixbuf *pixbuf;
	int        i;
	struct {
		const guint8 *rgba_data;
		char         *command;
	} comm_list_rgba [] = {
		{ add_to_catalog_16_rgba,    "/commands/Edit_AddToCatalog" },
		{ catalog_24_rgba,           "/commands/View_ShowCatalogs" },
		{ catalog_24_rgba,           "/ImageToolbar/View_Sidebar_Catalogs" },
		{ change_date_16_rgba,       "/menu/Tools/Tools_ChangeDate" },
		{ dir_24_rgba,               "/commands/View_ShowFolders" },
		{ dir_24_rgba,               "/ImageToolbar/View_Sidebar_Folders" },
		{ index_image_16_rgba,       "/menu/Tools/Tools_IndexImage" },
		{ maintenance_16_rgba,       "/menu/Tools/Tools_Maintenance" },
		{ next_image_24_rgba,        "/commands/View_NextImage" },
		{ prev_image_24_rgba,        "/commands/View_PrevImage" },
		{ search_duplicates_16_rgba, "/menu/Edit/Tools_FindDuplicates" },
		{ NULL, NULL }
	};

	for (i = 0; comm_list_rgba[i].rgba_data != NULL; i++) {
		pixbuf = gdk_pixbuf_new_from_inline (-1, comm_list_rgba[i].rgba_data, FALSE, NULL);
		bonobo_ui_util_set_pixbuf (ui_component, 
					   comm_list_rgba[i].command,
					   pixbuf,
					   NULL);
		g_object_unref (pixbuf);
	}
}


/* -- setup_toolbar_combo_button -- */


static gboolean
combo_button_activate_default_callback (EComboButton *combo_button,
					void *data)
{
	GThumbWindow *window = data;
	go_back_command_impl (NULL, window, NULL);
	return TRUE;
}


static void
setup_toolbar_combo_button (GThumbWindow *window)
{
	GtkWidget         *combo_button;
	GtkWidget         *menu;
	BonoboControl     *control;
	GdkPixbuf         *icon;

	menu = gtk_menu_new ();

	icon = gtk_widget_render_icon (window->app,
				       GTK_STOCK_GO_BACK,
				       GTK_ICON_SIZE_LARGE_TOOLBAR,
				       "");

	combo_button = e_combo_button_new ();

	e_combo_button_set_menu (E_COMBO_BUTTON (combo_button), GTK_MENU (menu));
	e_combo_button_set_label (E_COMBO_BUTTON (combo_button), _("Back"));

	e_combo_button_set_icon (E_COMBO_BUTTON (combo_button), icon);
	g_object_unref (icon);

	gtk_widget_show (combo_button);

	g_signal_connect (combo_button, "activate_default",
			  G_CALLBACK (combo_button_activate_default_callback),
			  window);

	bonobo_window_add_popup (BONOBO_WINDOW (window->app), GTK_MENU (menu), HISTORY_LIST_POPUP);

	control = bonobo_control_new (combo_button);

	bonobo_ui_component_object_set (window->ui_component, "/Toolbar/GoBackComboButton", BONOBO_OBJREF (control), NULL);

	window->go_back_combo_button = combo_button;
}


void
add_listener_for_toggle_items (GThumbWindow *window)
{
	BonoboUIComponent *ui_component = window->ui_component;
	char * toggle_commands [] = {
		"SortByName",
		"SortByPath",
		"SortBySize",
		"SortByTime",
		"SortReversed",
		"TranspTypeWhite",
		"TranspTypeNone",
		"TranspTypeBlack",
		"TranspTypeChecked",
		"View_ZoomQualityHigh",
		"View_ZoomQualityLow",
		"View_Thumbnails",
		"View_PlayAnimation",
		"View_Toolbar",
		"View_Statusbar",
		"View_ShowPreview",
		"Tools_Slideshow"
	};
	int i, n = sizeof (toggle_commands) / sizeof (char*);

	for (i = 0; i < n; i++)
		bonobo_ui_component_add_listener (ui_component, 
						  toggle_commands[i],
						  (BonoboUIListenerFn)item_toggled_handler,
						  (gpointer) window);
}


void
window_sync_menu_with_preferences (GThumbWindow *window)
{
	char *prop;

	set_command_state_without_notifing (window, 
					    "View_Thumbnails",
					    eel_gconf_get_boolean (PREF_SHOW_THUMBNAILS));

	set_command_state_without_notifing (window, "View_PlayAnimation", 1);
	set_command_state_without_notifing (window, "View_Toolbar", 1);
	set_command_state_without_notifing (window, "View_Statusbar", 1);

	switch (pref_get_transp_type ()) {
	case TRANSP_TYPE_WHITE:   prop = "TranspTypeWhite"; break;
	case TRANSP_TYPE_NONE:    prop = "TranspTypeNone"; break;
	case TRANSP_TYPE_BLACK:   prop = "TranspTypeBlack"; break;
	case TRANSP_TYPE_CHECKED: prop = "TranspTypeChecked"; break;
	}
	set_command_state_without_notifing (window, prop, TRUE);

	switch (pref_get_zoom_quality ()) {
	case ZOOM_QUALITY_HIGH: prop = "View_ZoomQualityHigh"; break;
	case ZOOM_QUALITY_LOW:  prop = "View_ZoomQualityLow"; break;
	}
	set_command_state_without_notifing (window, prop, TRUE);

	set_command_state_without_notifing (window, 
					    "View_ShowPreview",
					    eel_gconf_get_boolean (PREF_UI_IMAGE_PANE_VISIBLE));

	/* Toolbar & Statusbar */

	set_command_state_without_notifing (window, 
					    "View_Toolbar",
					    eel_gconf_get_boolean (PREF_UI_TOOLBAR_VISIBLE));
	set_command_state_without_notifing (window, 
					    "View_Statusbar",
					    eel_gconf_get_boolean (PREF_UI_STATUSBAR_VISIBLE));
	bonobo_ui_component_set_prop (window->ui_component, 
				      "/Toolbar",
				      "hidden", eel_gconf_get_boolean (PREF_UI_TOOLBAR_VISIBLE) ? "0" : "1",
				      NULL);
	bonobo_ui_component_set_prop (window->ui_component, 
				      "/ImageToolbar",
				      "hidden", "1", 
				      NULL);
	bonobo_ui_component_set_prop (window->ui_component, 
				      "/status",
				      "hidden", eel_gconf_get_boolean (PREF_UI_STATUSBAR_VISIBLE) ? "0" : "1",
				      NULL);

	/* Sort type item. */

	switch (window->file_list->sort_method) {
	case SORT_BY_NAME: prop = "SortByName"; break;
	case SORT_BY_PATH: prop = "SortByPath"; break;
	case SORT_BY_SIZE: prop = "SortBySize"; break;
	case SORT_BY_TIME: prop = "SortByTime"; break;
	default: prop = "X"; break;
	}
	set_command_state_without_notifing (window, prop, TRUE);

	/* 'reversed' item. */

	set_command_state_without_notifing (window, prop, TRUE);
	set_command_state_without_notifing (window, 
					    "SortReversed",
					    (window->file_list->sort_type != GTK_SORT_ASCENDING));
}


/* preferences change notification callbacks */


static void
pref_ui_layout_changed (GConfClient *client,
			guint        cnxn_id,
			GConfEntry  *entry,
			gpointer     user_data)
{
	GThumbWindow *window = user_data;
	window_notify_update_layout (window);
}


static void
pref_ui_toolbar_style_changed (GConfClient *client,
			       guint        cnxn_id,
			       GConfEntry  *entry,
			       gpointer     user_data)
{
	GThumbWindow *window = user_data;
	window_notify_update_toolbar_style (window);
}


static void
pref_ui_toolbar_visible_changed (GConfClient *client,
				 guint        cnxn_id,
				 GConfEntry  *entry,
				 gpointer     user_data)
{
	GThumbWindow *window = user_data;
	gboolean      hidden;

	hidden = ! eel_gconf_get_boolean (PREF_UI_TOOLBAR_VISIBLE);

	if (window->sidebar_visible)
		bonobo_ui_component_set_prop (window->ui_component, 
					      "/Toolbar",
					      "hidden", hidden ? "1" : "0",
					      NULL);
	else
		bonobo_ui_component_set_prop (window->ui_component, 
					      "/ImageToolbar",
					      "hidden", hidden ? "1" : "0",
					      NULL);
}


static void
pref_ui_statusbar_visible_changed (GConfClient *client,
				 guint        cnxn_id,
				 GConfEntry  *entry,
				 gpointer     user_data)
{
	GThumbWindow *window = user_data;
	gboolean      hidden;

	hidden = ! eel_gconf_get_boolean (PREF_UI_STATUSBAR_VISIBLE);
	bonobo_ui_component_set_prop (window->ui_component, 
				      "/status",
				      "hidden", hidden ? "1" : "0",
				      NULL);
}


static void
pref_show_thumbnails_changed (GConfClient *client,
			       guint        cnxn_id,
			       GConfEntry  *entry,
			       gpointer     user_data)
{
	GThumbWindow *window = user_data;

	window->file_list->enable_thumbs = eel_gconf_get_boolean (PREF_SHOW_THUMBNAILS);
	window_enable_thumbs (window, window->file_list->enable_thumbs);
}


static void
pref_show_comments_changed (GConfClient *client,
			    guint        cnxn_id,
			    GConfEntry  *entry,
			    gpointer     user_data)
{
	GThumbWindow      *window = user_data;
	ImageListViewMode  view_mode;

	if (eel_gconf_get_boolean (PREF_SHOW_COMMENTS))
		view_mode = IMAGE_LIST_VIEW_ALL;
	else
		view_mode = IMAGE_LIST_VIEW_TEXT;

	image_list_set_view_mode (IMAGE_LIST (window->file_list->ilist),
				  view_mode);
	window_update_file_list (window);
}


static void
pref_show_hidden_files_changed (GConfClient *client,
				guint        cnxn_id,
				GConfEntry  *entry,
				gpointer     user_data)
{
	GThumbWindow *window = user_data;
	window_update_file_list (window);
}


static void
pref_thumbnail_size_changed (GConfClient *client,
			     guint        cnxn_id,
			     GConfEntry  *entry,
			     gpointer     user_data)
{
	GThumbWindow *window = user_data;

	gth_file_list_set_thumbs_size (window->file_list, eel_gconf_get_integer (PREF_THUMBNAIL_SIZE));
	window_update_file_list (window);	
}


static void
pref_click_policy_changed (GConfClient *client,
			   guint        cnxn_id,
			   GConfEntry  *entry,
			   gpointer     user_data)
{
	GThumbWindow *window = user_data;
	dir_list_update_underline (window->dir_list);
	catalog_list_update_underline (window->catalog_list);
}


static void
pref_zoom_quality_changed (GConfClient *client,
			   guint        cnxn_id,
			   GConfEntry  *entry,
			   gpointer     user_data)
{
	GThumbWindow *window = user_data;

	image_viewer_set_zoom_quality (IMAGE_VIEWER (window->viewer),
				       pref_get_zoom_quality ());
	image_viewer_update_view (IMAGE_VIEWER (window->viewer));
}


static void
pref_zoom_change_changed (GConfClient *client,
			  guint        cnxn_id,
			  GConfEntry  *entry,
			  gpointer     user_data)
{
	GThumbWindow *window = user_data;

	image_viewer_set_zoom_change (IMAGE_VIEWER (window->viewer),
				      pref_get_zoom_change ());
	image_viewer_update_view (IMAGE_VIEWER (window->viewer));
}


static void
pref_transp_type_changed (GConfClient *client,
			  guint        cnxn_id,
			  GConfEntry  *entry,
			  gpointer     user_data)
{
	GThumbWindow *window = user_data;

	image_viewer_set_transp_type (IMAGE_VIEWER (window->viewer),
				      pref_get_transp_type ());
	image_viewer_update_view (IMAGE_VIEWER (window->viewer));
}


static void
pref_check_type_changed (GConfClient *client,
			 guint        cnxn_id,
			 GConfEntry  *entry,
			 gpointer     user_data)
{
	GThumbWindow *window = user_data;

	image_viewer_set_check_type (IMAGE_VIEWER (window->viewer),
				     pref_get_check_type ());
	image_viewer_update_view (IMAGE_VIEWER (window->viewer));
}


static void
pref_check_size_changed (GConfClient *client,
			 guint        cnxn_id,
			 GConfEntry  *entry,
			 gpointer     user_data)
{
	GThumbWindow *window = user_data;

	image_viewer_set_check_size (IMAGE_VIEWER (window->viewer),
				     pref_get_check_size ());
	image_viewer_update_view (IMAGE_VIEWER (window->viewer));
}





GThumbWindow *
window_new (void)
{
	GThumbWindow      *window;
	GtkWidget         *paned1;      /* Main paned widget. */
	GtkWidget         *paned2;      /* Secondary paned widget. */
	GtkWidget         *table;
	GtkWidget         *frame;
	GtkWidget         *image_vbox;
	GtkWidget         *dir_list_vbox;
	GtkWidget         *info_frame;
        BonoboUIContainer *ui_container;
        BonoboControl     *control;
	BonoboWindow      *win;
	GtkTreeSelection  *selection;
	int                i; 
	char              *starting_location;

	window = g_new0 (GThumbWindow, 1);

	window->app = bonobo_window_new (GETTEXT_PACKAGE, _("gThumb"));
	win = BONOBO_WINDOW (window->app);
	bonobo_ui_engine_config_set_path (bonobo_window_get_ui_engine (win), 
					  "/apps/gthumb/UIConfig/kvps");
	ui_container = bonobo_window_get_ui_container (win);
	window->ui_component = bonobo_ui_component_new_default ();
	bonobo_ui_component_set_container (window->ui_component, 
					   BONOBO_OBJREF (ui_container), 
					   NULL);
	bonobo_ui_util_set_ui (window->ui_component, 
			       GTHUMB_DATADIR,
			       BONOBO_UIDIR "gthumb-ui.xml", 
			       "gthumb", 
			       NULL);
	bonobo_ui_component_add_verb_list_with_data (window->ui_component, 
						     gthumb_verbs,
						     window);

	setup_commands_pixbufs (window->ui_component);
	setup_toolbar_combo_button (window);
	add_listener_for_toggle_items (window);

	gnome_window_icon_set_from_default (GTK_WINDOW (window->app));
	gtk_window_set_default_size (GTK_WINDOW (window->app), 
				     eel_gconf_get_integer (PREF_UI_WINDOW_WIDTH),
				     eel_gconf_get_integer (PREF_UI_WINDOW_HEIGHT));
	g_signal_connect (G_OBJECT (window->app), 
			  "delete_event",
			  G_CALLBACK (close_window_cb),
			  window);
	g_signal_connect (G_OBJECT (window->app), 
			  "key_press_event",
			  G_CALLBACK (key_press_cb), 
			  window);

	/* Create the widgets. */

	/* File list. */

	window->file_list = gth_file_list_new ();
	gtk_widget_set_size_request (window->file_list->ilist, 
				     PANE_MIN_SIZE,
				     PANE_MIN_SIZE);
	gth_file_list_set_progress_func (window->file_list, window_progress, window);

	gtk_drag_dest_set (window->file_list->ilist,
			   GTK_DEST_DEFAULT_ALL,
			   target_table, n_targets,
			   GDK_ACTION_MOVE);

	g_signal_connect (G_OBJECT (window->file_list->ilist), 
			  "drag_data_received",
			  G_CALLBACK (image_list_drag_data_received), 
			  window);
	g_signal_connect (G_OBJECT (window->file_list->ilist), 
			  "select_image",
			  G_CALLBACK (file_selection_changed_cb), 
			  window);
	g_signal_connect (G_OBJECT (window->file_list->ilist), 
			  "unselect_image",
			  G_CALLBACK (file_selection_changed_cb), 
			  window);
	g_signal_connect (G_OBJECT (window->file_list->ilist), 
			  "focus_image",
			  G_CALLBACK (gth_file_list_focus_image_cb), 
			  window);
	g_signal_connect_after (G_OBJECT (window->file_list->ilist), 
				"button_press_event",
				G_CALLBACK (gth_file_list_button_press_cb), 
				window);
	g_signal_connect (G_OBJECT (window->file_list->ilist), 
			  "double_click",
			  G_CALLBACK (gth_file_list_double_click_cb), 
			  window);
	g_signal_connect (G_OBJECT (window->file_list->ilist), 
			  "drag_data_get",
			  G_CALLBACK (gth_file_list_drag_data_get), 
			  window);
	g_signal_connect (G_OBJECT (window->file_list),
			  "busy",
			  G_CALLBACK (file_list_busy_cb),
			  window);
	g_signal_connect (G_OBJECT (window->file_list),
			  "idle",
			  G_CALLBACK (file_list_idle_cb),
			  window);

	/* Dir list. */

	window->dir_list = dir_list_new ();
	gtk_drag_dest_set (window->dir_list->root_widget,
			   GTK_DEST_DEFAULT_ALL,
			   target_table, n_targets,
			   GDK_ACTION_MOVE);
	gtk_drag_source_set (window->dir_list->list_view,
			     GDK_BUTTON1_MASK,
			     target_table, n_targets, 
			     GDK_ACTION_MOVE);
	g_signal_connect (G_OBJECT (window->dir_list->root_widget),
			  "drag_data_received",
			  G_CALLBACK (dir_list_drag_data_received), 
			  window);
	g_signal_connect (G_OBJECT (window->dir_list->list_view), 
			  "drag_begin",
			  G_CALLBACK (dir_list_drag_begin), 
			  window);
	g_signal_connect (G_OBJECT (window->dir_list->root_widget), 
			  "drag_motion",
			  G_CALLBACK (dir_list_drag_motion), 
			  window);
	g_signal_connect (G_OBJECT (window->dir_list->root_widget), 
			  "drag_leave",
			  G_CALLBACK (dir_list_drag_leave), 
			  window);
	g_signal_connect (G_OBJECT (window->dir_list->list_view),
			  "drag_data_get",
			  G_CALLBACK (dir_list_drag_data_get), 
			  window);
	g_signal_connect (G_OBJECT (window->dir_list->list_view), 
			  "button_press_event",
			  G_CALLBACK (dir_list_button_press_cb), 
			  window);
	g_signal_connect (G_OBJECT (window->dir_list->list_view), 
			  "button_release_event",
			  G_CALLBACK (dir_list_button_release_cb), 
			  window);
	g_signal_connect (G_OBJECT (window->dir_list->list_view),
                          "row_activated",
                          G_CALLBACK (dir_list_activated_cb),
                          window);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (window->dir_list->list_view));
	g_signal_connect (G_OBJECT (selection),
                          "changed",
                          G_CALLBACK (dir_or_catalog_sel_changed_cb),
                          window);

	/* Catalog list. */

	window->catalog_list = catalog_list_new (pref_get_real_click_policy () == CLICK_POLICY_SINGLE);
	gtk_drag_dest_set (window->catalog_list->root_widget,
			   GTK_DEST_DEFAULT_ALL,
			   target_table, n_targets,
			   GDK_ACTION_MOVE);
	g_signal_connect (G_OBJECT (window->catalog_list->root_widget),
			  "drag_data_received",
			  G_CALLBACK (catalog_list_drag_data_received), 
			  window);
	g_signal_connect (G_OBJECT (window->catalog_list->list_view), 
			  "drag_begin",
			  G_CALLBACK (catalog_list_drag_begin), 
			  window);
	g_signal_connect (G_OBJECT (window->catalog_list->root_widget), 
			  "drag_motion",
			  G_CALLBACK (catalog_list_drag_motion), 
			  window);
	g_signal_connect (G_OBJECT (window->catalog_list->root_widget), 
			  "drag_leave",
			  G_CALLBACK (catalog_list_drag_leave), 
			  window);
	g_signal_connect (G_OBJECT (window->catalog_list->list_view), 
			  "button_press_event",
			  G_CALLBACK (catalog_list_button_press_cb), 
			  window);
	g_signal_connect (G_OBJECT (window->catalog_list->list_view), 
			  "button_release_event",
			  G_CALLBACK (catalog_list_button_release_cb), 
			  window);
	g_signal_connect (G_OBJECT (window->catalog_list->list_view),
                          "row_activated",
                          G_CALLBACK (catalog_list_activated_cb),
                          window);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (window->catalog_list->list_view));
	g_signal_connect (G_OBJECT (selection),
                          "changed",
                          G_CALLBACK (dir_or_catalog_sel_changed_cb),
                          window);

	/* Location entry. */

	window->location_entry = gtk_entry_new ();

	g_signal_connect (G_OBJECT (window->location_entry),
			  "key_press_event",
			  G_CALLBACK (location_entry_key_press_cb),
			  window);

	/* Info bar. */

	window->info_bar = gthumb_info_bar_new ();
	gthumb_info_bar_set_focused (GTHUMB_INFO_BAR (window->info_bar), FALSE);
	g_signal_connect (G_OBJECT (window->info_bar), 
			  "button_press_event",
			  G_CALLBACK (info_bar_clicked_cb), 
			  window);

	/* Image viewer. */
	
	window->viewer = image_viewer_new ();
	gtk_widget_set_size_request (window->viewer, 
				     PANE_MIN_SIZE,
				     PANE_MIN_SIZE);

	/* FIXME 
	gtk_drag_source_set (window->viewer,
			     GDK_BUTTON2_MASK,
			     target_table, n_targets, 
			     GDK_ACTION_MOVE);
	*/
	gtk_drag_dest_set (window->viewer,
			   GTK_DEST_DEFAULT_ALL,
			   target_table, n_targets,
			   GDK_ACTION_MOVE);

	g_signal_connect (G_OBJECT (window->viewer), 
			  "image_loaded",
			  G_CALLBACK (image_loaded_cb), 
			  window);
	g_signal_connect (G_OBJECT (window->viewer), 
			  "zoom_changed",
			  G_CALLBACK (zoom_changed_cb), 
			  window);
	g_signal_connect (G_OBJECT (window->viewer), 
			  "size_changed",
			  G_CALLBACK (size_changed_cb), 
			  window);
	g_signal_connect_after (G_OBJECT (window->viewer), 
				"button_press_event",
				G_CALLBACK (image_button_press_cb), 
				window);
	g_signal_connect_after (G_OBJECT (window->viewer), 
				"button_release_event",
				G_CALLBACK (image_button_release_cb), 
				window);
	g_signal_connect_after (G_OBJECT (window->viewer), 
				"clicked",
				G_CALLBACK (image_clicked_cb), 
				window);
	g_signal_connect (G_OBJECT (window->viewer), 
			  "focus_in_event",
			  G_CALLBACK (image_focus_changed_cb), 
			  window);
	g_signal_connect (G_OBJECT (window->viewer), 
			  "focus_out_event",
			  G_CALLBACK (image_focus_changed_cb), 
			  window);

	g_signal_connect (G_OBJECT (window->viewer),
			  "drag_data_get",
			  G_CALLBACK (viewer_drag_data_get), 
			  window);

	g_signal_connect (G_OBJECT (window->viewer), 
			  "drag_data_received",
			  G_CALLBACK (viewer_drag_data_received), 
			  window);

	g_signal_connect (G_OBJECT (window->viewer),
			  "key_press_event",
			  G_CALLBACK (viewer_key_press_cb),
			  window);

	g_signal_connect (G_OBJECT (IMAGE_VIEWER (window->viewer)->loader), 
			  "progress",
			  G_CALLBACK (image_loader_progress_cb), 
			  window);
	g_signal_connect (G_OBJECT (IMAGE_VIEWER (window->viewer)->loader), 
			  "done",
			  G_CALLBACK (image_loader_done_cb), 
			  window);
	g_signal_connect (G_OBJECT (IMAGE_VIEWER (window->viewer)->loader), 
			  "error",
			  G_CALLBACK (image_loader_done_cb), 
			  window);

	window->viewer_vscr = gtk_vscrollbar_new (IMAGE_VIEWER (window->viewer)->vadj);
	window->viewer_hscr = gtk_hscrollbar_new (IMAGE_VIEWER (window->viewer)->hadj);
	window->viewer_event_box = gtk_event_box_new ();
	gtk_container_add (GTK_CONTAINER (window->viewer_event_box), _gtk_image_new_from_xpm_data (nav_button_xpm));

	g_signal_connect (G_OBJECT (window->viewer_event_box), 
			  "button_press_event",
			  G_CALLBACK (nav_button_clicked_cb), 
			  window->viewer);

	/* Pack the widgets */

	window->layout_type = eel_gconf_get_integer (PREF_UI_LAYOUT);

	if (window->layout_type == 1) {
		window->main_pane = paned1 = gtk_vpaned_new (); 
		window->content_pane = paned2 = gtk_hpaned_new ();
	} else {
		window->main_pane = paned1 = gtk_hpaned_new (); 
		window->content_pane = paned2 = gtk_vpaned_new (); 
	}

	bonobo_window_set_contents (BONOBO_WINDOW (window->app), 
				    window->main_pane);

	if (window->layout_type == 3)
		gtk_paned_pack2 (GTK_PANED (paned1), paned2, TRUE, FALSE);
	else
		gtk_paned_pack1 (GTK_PANED (paned1), paned2, TRUE, FALSE);

	window->notebook = gtk_notebook_new ();
	gtk_notebook_set_show_tabs (GTK_NOTEBOOK (window->notebook), FALSE);
	gtk_notebook_set_show_border (GTK_NOTEBOOK (window->notebook), FALSE);
	gtk_notebook_append_page (GTK_NOTEBOOK (window->notebook), 
				  window->dir_list->root_widget,
				  NULL);
	gtk_notebook_append_page (GTK_NOTEBOOK (window->notebook), 
				  window->catalog_list->root_widget,
				  NULL);

	window->dir_list_pane = dir_list_vbox = gtk_vbox_new (FALSE, 3);

	gtk_box_pack_start (GTK_BOX (dir_list_vbox), window->location_entry, 
			    FALSE, FALSE, 0);

	gtk_box_pack_start (GTK_BOX (dir_list_vbox), window->notebook, 
			    TRUE, TRUE, 0);

	if (window->layout_type == 3) 
		gtk_paned_pack1 (GTK_PANED (paned1), dir_list_vbox, TRUE, FALSE);
	else 
		gtk_paned_pack1 (GTK_PANED (paned2), dir_list_vbox, TRUE, FALSE);

	if (window->layout_type <= 1) 
		gtk_paned_pack2 (GTK_PANED (paned2), window->file_list->root_widget, TRUE, FALSE);
	else if (window->layout_type == 2)
		gtk_paned_pack2 (GTK_PANED (paned1), window->file_list->root_widget, TRUE, FALSE);
	else if (window->layout_type == 3)
		gtk_paned_pack1 (GTK_PANED (paned2), window->file_list->root_widget, TRUE, FALSE);

	/**/

	image_vbox = window->image_pane = gtk_vbox_new (FALSE, 0);
	if (window->layout_type <= 1)
		gtk_paned_pack2 (GTK_PANED (paned1), image_vbox, TRUE, FALSE);
	else
		gtk_paned_pack2 (GTK_PANED (paned2), image_vbox, TRUE, FALSE);

	/**/

	info_frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (info_frame), GTK_SHADOW_NONE);
	gtk_container_add (GTK_CONTAINER (info_frame), window->info_bar);

	gtk_box_pack_start (GTK_BOX (image_vbox), info_frame, FALSE, FALSE, 0);

	/**/

	window->viewer_container = frame = gtk_hbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (frame), window->viewer);

	table = gtk_table_new (2, 2, FALSE);

	gtk_table_attach (GTK_TABLE (table), frame, 0, 1, 0, 1,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
	gtk_table_attach (GTK_TABLE (table), window->viewer_vscr, 1, 2, 0, 1,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
	gtk_table_attach (GTK_TABLE (table), window->viewer_hscr, 0, 1, 1, 2,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL), 0, 0);
	gtk_table_attach (GTK_TABLE (table), window->viewer_event_box, 1, 2, 1, 2,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL), 0, 0);

	gtk_box_pack_start (GTK_BOX (image_vbox), table, TRUE, TRUE, 0);

	/* Progress bar. */

	window->image_info = gtk_label_new (NULL);

	window->image_info_frame = info_frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (info_frame), GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (info_frame), window->image_info);
	gtk_widget_show_all (info_frame);

	control = bonobo_control_new (info_frame);
	bonobo_ui_component_object_set (window->ui_component,
                                        "/status/ImageInfo",
                                        BONOBO_OBJREF (control),
                                        NULL);
        bonobo_object_unref (BONOBO_OBJECT (control));

	window->progress = gtk_progress_bar_new ();
	gtk_widget_set_size_request (window->progress, PROGRESS_BAR_WIDTH, -1);
	gtk_widget_show (window->progress);
	control = bonobo_control_new (window->progress);
	bonobo_ui_component_object_set (window->ui_component,
                                        "/status/ActivityProgress",
                                        BONOBO_OBJREF (control),
                                        NULL);
        bonobo_object_unref (BONOBO_OBJECT (control));

	/* Progress dialog */

	window->progress_gui = glade_xml_new (GTHUMB_GLADEDIR "/" GLADE_EXPORTER_FILE, NULL, NULL);
	if (! window->progress_gui) {
		window->progress_dialog = NULL;
		window->progress_progressbar = NULL;
		window->progress_info = NULL;
	} else {
		GtkWidget *cancel_button;

		window->progress_dialog = glade_xml_get_widget (window->progress_gui, "progress_dialog");
		window->progress_progressbar = glade_xml_get_widget (window->progress_gui, "progress_progressbar");
		window->progress_info = glade_xml_get_widget (window->progress_gui, "progress_info");
		cancel_button = glade_xml_get_widget (window->progress_gui, "progress_cancel");

		gtk_window_set_transient_for (GTK_WINDOW (window->progress_dialog), GTK_WINDOW (window->app));
		gtk_window_set_modal (GTK_WINDOW (window->progress_dialog), FALSE);

		g_signal_connect (G_OBJECT (cancel_button), 
				  "clicked",
				  G_CALLBACK (progress_cancel_cb), 
				  window);
		g_signal_connect (G_OBJECT (window->progress_dialog), 
				  "delete_event",
				  G_CALLBACK (progress_delete_cb),
				  window);
	}

	/* Update data. */

	window->sidebar_content = NO_LIST;
	window->catalog_path = NULL;
	window->image_path = NULL;
	window->image_mtime = 0;
	window->image_catalog = NULL;
	window->image_modified = FALSE;

	window->fullscreen = FALSE;
	window->slideshow = FALSE;
	window->slideshow_timeout = 0;

	window->bookmarks_length = 0;
	window_update_bookmark_list (window);

	window->history = bookmarks_new (NULL);
	window->history_current = NULL;
	window->history_length = 0;
	window->go_op = WINDOW_GO_TO;
	window_update_history_list (window);

	window->activity_timeout = 0;
	window->activity_ref = 0;
	window->setting_file_list = FALSE;
	window->changing_directory = FALSE;

	window->monitor_handle = NULL;
	window->monitor_enabled = FALSE;
	window->update_changes_timeout = 0;
	for (i = 0; i < MONITOR_EVENT_NUM; i++)
		window->monitor_events[i] = NULL;

	window->image_prop_dlg = NULL;

	window->busy_cursor_timeout = 0;

	window->view_image_timeout = 0;
	window->load_dir_timeout = 0;

	window->freeze_toggle_handler = 0;

	window->sel_change_timeout = 0;

	/* preloader */

	window->preloader = gthumb_preloader_new ();

	g_signal_connect (G_OBJECT (window->preloader), 
			  "requested_done",
			  G_CALLBACK (image_requested_done_cb), 
			  window);

	g_signal_connect (G_OBJECT (window->preloader), 
			  "requested_error",
			  G_CALLBACK (image_requested_error_cb), 
			  window);

	/* FIXME 
	for (i = 0; i < N_LOADERS; i++) {
		PreLoader *ploader = window->preloader->loader[i];
		g_signal_connect (G_OBJECT (ploader->loader), 
				  "progress",
				  G_CALLBACK (image_loader_progress_cb), 
				  window);
		g_signal_connect (G_OBJECT (ploader->loader), 
				  "done",
				  G_CALLBACK (image_loader_done_cb), 
				  window);
		g_signal_connect (G_OBJECT (ploader->loader), 
				  "error",
				  G_CALLBACK (image_loader_done_cb), 
				  window);
	}
	*/

	for (i = 0; i < GCONF_NOTIFICATIONS; i++)
		window->cnxn_id[i] = -1;

	window->pixop = NULL;

	/* Sync widgets and visualization options. */

	gtk_widget_realize (window->app);

	window_sync_menu_with_preferences (window);

	window->sidebar_visible = TRUE;
	window->sidebar_width = eel_gconf_get_integer (PREF_UI_SIDEBAR_SIZE);
	gtk_paned_set_position (GTK_PANED (paned1), eel_gconf_get_integer (PREF_UI_SIDEBAR_SIZE));
	gtk_paned_set_position (GTK_PANED (paned2), eel_gconf_get_integer (PREF_UI_SIDEBAR_CONTENT_SIZE));

	gtk_widget_show_all (window->main_pane);

	if (eel_gconf_get_boolean (PREF_UI_IMAGE_PANE_VISIBLE))
		window_show_image_pane (window);
	else 
		window_hide_image_pane (window);

	window_notify_update_toolbar_style (window);
	window_update_statusbar_image_info (window);

	image_viewer_set_zoom_quality (IMAGE_VIEWER (window->viewer),
				       pref_get_zoom_quality ());
	image_viewer_set_zoom_change  (IMAGE_VIEWER (window->viewer),
				       pref_get_zoom_change ());
	image_viewer_set_check_type   (IMAGE_VIEWER (window->viewer),
				       pref_get_check_type ());
	image_viewer_set_check_size   (IMAGE_VIEWER (window->viewer),
				       pref_get_check_size ());
	image_viewer_set_transp_type  (IMAGE_VIEWER (window->viewer),
				       pref_get_transp_type ());

	/* Add the window to the window's list */

	window_list = g_list_prepend (window_list, window);

	/* Add notification callbacks. */

	i = 0;

	window->cnxn_id[i++] = eel_gconf_notification_add (
					   PREF_UI_LAYOUT,
					   pref_ui_layout_changed,
					   window);

	window->cnxn_id[i++] = eel_gconf_notification_add (
					   PREF_UI_TOOLBAR_STYLE,
					   pref_ui_toolbar_style_changed,
					   window);
	window->cnxn_id[i++] = eel_gconf_notification_add (
					   PREF_UI_TOOLBAR_STYLE,
					   pref_ui_toolbar_style_changed,
					   window);
	window->cnxn_id[i++] = eel_gconf_notification_add (
					   "/desktop/gnome/interface/toolbar_style",
					   pref_ui_toolbar_style_changed,
					   window);

	window->cnxn_id[i++] = eel_gconf_notification_add (
					   PREF_UI_TOOLBAR_VISIBLE,
					   pref_ui_toolbar_visible_changed,
					   window);

	window->cnxn_id[i++] = eel_gconf_notification_add (
					   PREF_UI_STATUSBAR_VISIBLE,
					   pref_ui_statusbar_visible_changed,
					   window);

	window->cnxn_id[i++] = eel_gconf_notification_add (
					   PREF_SHOW_THUMBNAILS,
					   pref_show_thumbnails_changed,
					   window);

	window->cnxn_id[i++] = eel_gconf_notification_add (
					   PREF_SHOW_COMMENTS,
					   pref_show_comments_changed,
					   window);

	window->cnxn_id[i++] = eel_gconf_notification_add (
					   PREF_SHOW_HIDDEN_FILES,
					   pref_show_hidden_files_changed,
					   window);

	window->cnxn_id[i++] = eel_gconf_notification_add (
					   PREF_THUMBNAIL_SIZE,
					   pref_thumbnail_size_changed,
					   window);

	window->cnxn_id[i++] = eel_gconf_notification_add (
					   PREF_CLICK_POLICY,
					   pref_click_policy_changed,
					   window);

	window->cnxn_id[i++] = eel_gconf_notification_add (
					   PREF_ZOOM_QUALITY,
					   pref_zoom_quality_changed,
					   window);

	window->cnxn_id[i++] = eel_gconf_notification_add (
					   PREF_ZOOM_CHANGE,
					   pref_zoom_change_changed,
					   window);

	window->cnxn_id[i++] = eel_gconf_notification_add (
					   PREF_TRANSP_TYPE,
					   pref_transp_type_changed,
					   window);

	window->cnxn_id[i++] = eel_gconf_notification_add (
					   PREF_CHECK_TYPE,
					   pref_check_type_changed,
					   window);

	window->cnxn_id[i++] = eel_gconf_notification_add (
					   PREF_CHECK_SIZE,
					   pref_check_size_changed,
					   window);

	/* Initial location. */

	starting_location = eel_gconf_get_locale_string (PREF_STARTUP_LOCATION);

	if (pref_util_location_is_catalog (starting_location)) 
		window_go_to_catalog (window, pref_util_get_catalog_location (starting_location));
	else {
		const char *path;
		
		if (pref_util_location_is_file (starting_location))
			path = pref_util_get_file_location (starting_location);
		else {  /* we suppose it is a directory name without prefix. */
			path = starting_location;
			if (! path_is_dir (path))
				path = g_get_home_dir ();
		}

		window_go_to_directory (window, path);
	}

	g_free (starting_location);

	gtk_widget_grab_focus (window->file_list->ilist);

	return window;
}


static void
window_free (GThumbWindow *window)
{
	int i;

	g_return_if_fail (window != NULL);

	g_object_unref (G_OBJECT (window->file_list));
	dir_list_free     (window->dir_list);
	catalog_list_free (window->catalog_list);

	if (window->catalog_path) {
		g_free (window->catalog_path);
		window->catalog_path = NULL;
	}

	if (window->image_path) {
		g_free (window->image_path);
		window->image_path = NULL;
	}

	if (window->image_catalog) {
		g_free (window->image_catalog);	
		window->image_catalog = NULL;
	}

	if (window->history) {
		bookmarks_free (window->history);
		window->history = NULL;
	}

	if (window->preloader) {
		g_object_unref (window->preloader);
		window->preloader = NULL;
	}

	for (i = 0; i < MONITOR_EVENT_NUM; i++)
		path_list_free (window->monitor_events[i]);

	g_free (window);
}


/* -- window_close -- */


static void
_window_remove_notifications (GThumbWindow *window)
{
	int i;

	for (i = 0; i < GCONF_NOTIFICATIONS; i++)
		if (window->cnxn_id[i] != -1)
			eel_gconf_notification_remove (window->cnxn_id[i]);
}


void
close__step5 (GThumbWindow *window)
{
	ImageViewer *viewer = IMAGE_VIEWER (window->viewer);
	int          width, height;
	gboolean     last_window;

	last_window = window_list->next == NULL;

	/* Save visualization options. */

	if (window->sidebar_visible) {
		eel_gconf_set_integer (PREF_UI_SIDEBAR_SIZE, gtk_paned_get_position (GTK_PANED (window->main_pane)));
		eel_gconf_set_integer (PREF_UI_SIDEBAR_CONTENT_SIZE, gtk_paned_get_position (GTK_PANED (window->content_pane)));
	} else
		eel_gconf_set_integer (PREF_UI_SIDEBAR_SIZE, window->sidebar_width);
	
	gdk_drawable_get_size (window->app->window, &width, &height);

	eel_gconf_set_integer (PREF_UI_WINDOW_WIDTH, width);
	eel_gconf_set_integer (PREF_UI_WINDOW_HEIGHT, height);

	if (last_window)
		eel_gconf_set_boolean (PREF_SHOW_THUMBNAILS, window->file_list->enable_thumbs);

	pref_set_arrange_type (window->file_list->sort_method);
	pref_set_sort_order (window->file_list->sort_type);

	if (eel_gconf_get_boolean (PREF_GO_TO_LAST_LOCATION)) {
		char *location = NULL;

		if (window->sidebar_content == DIR_LIST) 
			location = g_strconcat ("file://",
						window->dir_list->path,
						NULL);
		else if ((window->sidebar_content == CATALOG_LIST) 
			 && (window->catalog_path != NULL))
			location = g_strconcat ("catalog://",
						window->catalog_path,
						NULL);

		if (location != NULL) {
			eel_gconf_set_locale_string (PREF_STARTUP_LOCATION, location);
			g_free (location);
		}
	}

	if (last_window) {
		pref_set_zoom_quality (image_viewer_get_zoom_quality (viewer));
		pref_set_transp_type (image_viewer_get_transp_type (viewer));
	}
	pref_set_check_type (image_viewer_get_check_type (viewer));
	pref_set_check_size (image_viewer_get_check_size (viewer));

	eel_gconf_set_boolean (PREF_UI_IMAGE_PANE_VISIBLE, window->image_preview_visible);

	/* Destroy the main window. */

	if (window->progress_timeout != 0) {
		g_source_remove (window->progress_timeout);
		window->progress_timeout = 0;
	}
	if (window->activity_timeout != 0) {
		g_source_remove (window->activity_timeout);
		window->activity_timeout = 0;
	}
	if (window->load_dir_timeout != 0) {
		g_source_remove (window->load_dir_timeout);
		window->load_dir_timeout = 0;
	}
	if (window->sel_change_timeout != 0) {
		g_source_remove (window->sel_change_timeout);
		window->sel_change_timeout = 0;
	}
	if (window->busy_cursor_timeout != 0) {
		g_source_remove (window->busy_cursor_timeout);
		window->busy_cursor_timeout = 0;
	}
	if (window->view_image_timeout != 0) {
 		g_source_remove (window->view_image_timeout);
		window->view_image_timeout = 0;
	}
	if (window->update_changes_timeout != 0) {
		g_source_remove (window->update_changes_timeout);
		window->update_changes_timeout = 0;
	}

	if (window->pixop != NULL) 
		g_object_unref (window->pixop);

	if (window->progress_gui != NULL)
		g_object_unref (window->progress_gui);

	if (window->popup_menu != NULL) {
		gtk_widget_destroy (window->popup_menu);
		window->popup_menu = NULL;
	}

	gtk_widget_destroy (window->app);

	window_list = g_list_remove (window_list, window);
	window_free (window);

	if (window_list == NULL) {
		if (dir_list_tree_path != NULL) {
			gtk_tree_path_free (dir_list_tree_path);
			dir_list_tree_path = NULL;
		}

		if (catalog_list_tree_path != NULL) {
			gtk_tree_path_free (catalog_list_tree_path);
			catalog_list_tree_path = NULL;
		}

		bonobo_main_quit ();

	} else if (ExitAll) {
		GThumbWindow *first_window = window_list->data;
		window_close (first_window);
	}
}


void
close__step4 (GThumbWindow *window)
{
	if (window->slideshow)
		window_stop_slideshow (window);
	gthumb_preloader_stop (window->preloader, 
			       (DoneFunc) close__step5, 
			       window);
}


void
close__step3 (GThumbWindow *window)
{
	window->setting_file_list = FALSE;
	if (window->file_list->doing_thumbs)
		gth_file_list_interrupt_thumbs (window->file_list, 
					    (DoneFunc) close__step4, 
					    window);
	else
		close__step4 (window);
}


void
close__step2 (GThumbWindow *window)
{
	window->changing_directory = FALSE;
	if (window->setting_file_list) 
		gth_file_list_interrupt_set_list (window->file_list,
					      (DoneFunc) close__step3,
					      window);
	else
		close__step3 (window);
}


void
window_close (GThumbWindow *window)
{
	g_return_if_fail (window != NULL);

	/* Interrupt any activity. */

	_window_remove_notifications (window);
	window_stop_activity_mode (window);
	window_remove_monitor (window);

	if (window->image_prop_dlg != NULL) {
		dlg_image_prop_close (window->image_prop_dlg);
		window->image_prop_dlg = NULL;
	}

	if (window->changing_directory) 
		dir_list_interrupt_change_to (window->dir_list, 
					      (DoneFunc) close__step2,
					      window);
	else
		close__step2 (window);
}


void
window_set_sidebar_content (GThumbWindow *window,
			    int           sidebar_content)
{
	char  old_content = window->sidebar_content;
	char *path;

	_window_set_sidebar (window, sidebar_content);
	window_show_sidebar (window);

	if (old_content == sidebar_content) 
		return;

	switch (sidebar_content) {
	case DIR_LIST: 
		if (window->dir_list->path == NULL) 
			window_go_to_directory (window, g_get_home_dir ());
		else 
			window_go_to_directory (window, window->dir_list->path);
		break;

	case CATALOG_LIST:
		if (window->catalog_path == NULL)
			path = get_catalog_full_path (NULL);
		else 
			path = remove_level_from_path (window->catalog_path);
		window_go_to_catalog_directory (window, path);
		
		g_free (path);
		
		if (window->catalog_path != NULL) {
			GtkTreeIter iter;
			
			if (window->slideshow)
				window_stop_slideshow (window);
			
			if (! catalog_list_get_iter_from_path (window->catalog_list, window->catalog_path, &iter))
				return;
			catalog_list_select_iter (window->catalog_list, &iter);
			catalog_activate (window, window->catalog_path);
		} else 
			window_set_file_list (window, NULL, NULL, NULL);

		window_update_title (window);
		break;

	default:
		break;
	}
}


void
window_hide_sidebar (GThumbWindow *window)
{
	char *cname;

	if (! window->sidebar_visible)
		return;

	window->sidebar_visible = FALSE;
	window->sidebar_width = gtk_paned_get_position (GTK_PANED (window->main_pane));

	if (window->layout_type <= 1)
		gtk_widget_hide (GTK_PANED (window->main_pane)->child1);
	else if (window->layout_type == 2) {
		gtk_widget_hide (GTK_PANED (window->main_pane)->child2);
		gtk_widget_hide (GTK_PANED (window->content_pane)->child1);
	} else if (window->layout_type == 3) {
		gtk_widget_hide (GTK_PANED (window->main_pane)->child1);
		gtk_widget_hide (GTK_PANED (window->content_pane)->child1);
	}

	gtk_widget_grab_focus (window->viewer);

	/* Sync menu and toolbar. */

	cname = get_command_name_from_sidebar_content (window);
	if (cname != NULL)
		set_command_state_if_different (window, cname, FALSE, FALSE);

	/**/

	if (eel_gconf_get_boolean (PREF_UI_TOOLBAR_VISIBLE)) {
		bonobo_ui_component_set_prop (window->ui_component, 
					      "/Toolbar",
					      "hidden", "1",
					      NULL);
		bonobo_ui_component_set_prop (window->ui_component, 
					      "/ImageToolbar",
					      "hidden", "0",
					      NULL);

		set_command_visible (window, "View_Sidebar_Catalogs", window->sidebar_content == CATALOG_LIST);
		set_command_visible (window, "View_Sidebar_Folders", window->sidebar_content == DIR_LIST);
	}

	/**/

	if (! window->image_pane_visible)
		window_show_image_pane (window);
}


void
window_show_sidebar (GThumbWindow *window)
{
	char *cname;

	if (window->sidebar_visible)
		return;

	window->sidebar_visible = TRUE;
	gtk_paned_set_position (GTK_PANED (window->main_pane), 
				window->sidebar_width); 

	if (window->layout_type < 2)
		gtk_widget_show (GTK_PANED (window->main_pane)->child1);
	else if (window->layout_type == 2) {
		gtk_widget_show (GTK_PANED (window->main_pane)->child2);
		gtk_widget_show (GTK_PANED (window->content_pane)->child1);
	} else if (window->layout_type == 3) {
		gtk_widget_show (GTK_PANED (window->main_pane)->child1);
		gtk_widget_show (GTK_PANED (window->content_pane)->child1);
	}

	/* Sync menu and toolbar. */

	cname = get_command_name_from_sidebar_content (window);
	if (cname != NULL)
		set_command_state_if_different (window, cname, TRUE, FALSE);

	/**/

	if (eel_gconf_get_boolean (PREF_UI_TOOLBAR_VISIBLE)) {
		bonobo_ui_component_set_prop (window->ui_component, 
					      "/Toolbar",
					      "hidden", "0",
					      NULL);
		bonobo_ui_component_set_prop (window->ui_component, 
					      "/ImageToolbar",
					      "hidden", "1",
					      NULL);
	}

	/**/

	if (window->image_preview_visible)
		window_show_image_pane (window);
	else
		window_hide_image_pane (window);

	gtk_widget_grab_focus (window->file_list->ilist);
}


void
window_hide_image_pane (GThumbWindow *window)
{
	window->image_pane_visible = FALSE;
	gtk_widget_hide (window->image_pane);

	gtk_widget_grab_focus (window->file_list->ilist);

	/**/

	if (! window->sidebar_visible)
		window_show_sidebar (window);
	else {
		window->image_preview_visible = FALSE;
		/* Sync menu and toolbar. */
		set_command_state_if_different (window, 
						"/commands/View_ShowPreview", 
						FALSE, 
						FALSE);
	}
}


void
window_show_image_pane (GThumbWindow *window)
{
	window->image_pane_visible = TRUE;

	if (window->sidebar_visible) {
		window->image_preview_visible = TRUE;
		/* Sync menu and toolbar. */
		set_command_state_if_different (window, 
						"/commands/View_ShowPreview", 
						TRUE, 
						FALSE);
	}

	gtk_widget_show (window->image_pane);

	gtk_widget_grab_focus (window->viewer);


}


/* -- window_stop_loading -- */


void
stop__step5 (GThumbWindow *window)
{
	set_command_sensitive (window, "Go_Stop", 
			       (window->activity_ref > 0) 
			       || window->setting_file_list
			       || window->changing_directory
			       || window->file_list->doing_thumbs);
}


void
stop__step4 (GThumbWindow *window)
{
	if (window->slideshow)
		window_stop_slideshow (window);

	gthumb_preloader_stop (window->preloader, 
			       (DoneFunc) stop__step5, 
			       window);

	set_command_sensitive (window, "Go_Stop", 
			       (window->activity_ref > 0) 
			       || window->setting_file_list
			       || window->changing_directory
			       || window->file_list->doing_thumbs);
}


void
stop__step3 (GThumbWindow *window)
{
	window->setting_file_list = FALSE;
	if (window->file_list->doing_thumbs)
		gth_file_list_interrupt_thumbs (window->file_list, 
						(DoneFunc) stop__step4, 
						window);
	else
		stop__step4 (window);
}


void
stop__step2 (GThumbWindow *window)
{
	window->changing_directory = FALSE;
	if (window->setting_file_list) 
		gth_file_list_interrupt_set_list (window->file_list,
						  (DoneFunc) stop__step3,
						  window);
	else
		stop__step3 (window);
}


void
window_stop_loading (GThumbWindow *window)
{
	window_stop_activity_mode (window);

	if (window->changing_directory) 
		dir_list_interrupt_change_to (window->dir_list,
					      (DoneFunc) stop__step2,
					      window);
	else
		stop__step2 (window);
}


/* -- window_refresh -- */


void
window_refresh (GThumbWindow *window)
{
	window->refreshing = TRUE;
	window_update_file_list (window);
}


/* -- file system monitor -- */


static void notify_files_added (GThumbWindow *window, GList *list);
static void notify_files_deleted (GThumbWindow *window, GList *list);


static gboolean
_proc_monitor_events (gpointer data)
{
	GThumbWindow       *window = data;
	GList              *scan;
		
	if (window->update_changes_timeout != 0) {
		g_source_remove (window->update_changes_timeout);
		window->update_changes_timeout = 0;
	}

	/**/

	scan = window->monitor_events[MONITOR_EVENT_DIR_CREATED];
	for (; scan; scan = scan->next) {
		char *path = scan->data;
		dir_list_add_directory (window->dir_list, path);
	}
	path_list_free (window->monitor_events[MONITOR_EVENT_DIR_CREATED]);
	window->monitor_events[MONITOR_EVENT_DIR_CREATED] = NULL;

	/**/

	scan = window->monitor_events[MONITOR_EVENT_DIR_DELETED];
	for (; scan; scan = scan->next) {
		char *path = scan->data;
		dir_list_remove_directory (window->dir_list, path);
	}
	path_list_free (window->monitor_events[MONITOR_EVENT_DIR_DELETED]);
	window->monitor_events[MONITOR_EVENT_DIR_DELETED] = NULL;

	/**/

	if (window->monitor_events[MONITOR_EVENT_FILE_CREATED] != NULL) {
		notify_files_added (window, window->monitor_events[MONITOR_EVENT_FILE_CREATED]);
		path_list_free (window->monitor_events[MONITOR_EVENT_FILE_CREATED]);
		window->monitor_events[MONITOR_EVENT_FILE_CREATED] = NULL;
	}

	/**/

	if (window->monitor_events[MONITOR_EVENT_FILE_DELETED] != NULL) {
		notify_files_deleted (window, window->monitor_events[MONITOR_EVENT_FILE_DELETED]);
		path_list_free (window->monitor_events[MONITOR_EVENT_FILE_DELETED]);
		window->monitor_events[MONITOR_EVENT_FILE_DELETED] = NULL;
	}

	/**/

	if (window->monitor_events[MONITOR_EVENT_FILE_CHANGED] != NULL) {
		window_notify_files_changed (window, window->monitor_events[MONITOR_EVENT_FILE_CHANGED]);
		path_list_free (window->monitor_events[MONITOR_EVENT_FILE_CHANGED]);
		window->monitor_events[MONITOR_EVENT_FILE_CHANGED] = NULL;
	}

	/**/

	return FALSE;
}


static gboolean
remove_if_present (GThumbWindow     *window,
		   MonitorEventType  type,
		   const char       *path)
{
	GList *list, *link;

	list = window->monitor_events[type];
	link = path_list_find_path (list, path);
	if (link != NULL) {
		window->monitor_events[type] = g_list_remove_link (list, link);
		path_list_free (link);
		return TRUE;
	}

	return FALSE;
}


static void
_window_add_monitor_event (GThumbWindow             *window,
			   GnomeVFSMonitorEventType  event_type,
			   char                     *path)
{
	MonitorEventType type;

#ifdef DEBUG
	{
		char *op;

		if (event_type == GNOME_VFS_MONITOR_EVENT_CREATED)
			op = "CREATED";
		else if (event_type == GNOME_VFS_MONITOR_EVENT_DELETED)
			op = "DELETED";
		else 
			op = "CHANGED";

		g_print ("[%s] %s\n", op, path);
	}
#endif

	if (event_type == GNOME_VFS_MONITOR_EVENT_CREATED) {
		if (path_is_dir (path))
			type = MONITOR_EVENT_DIR_CREATED;
		else
			type = MONITOR_EVENT_FILE_CREATED;

	} else if (event_type == GNOME_VFS_MONITOR_EVENT_DELETED) {
		if (dir_list_get_row_from_path (window->dir_list, path) != -1)
			type = MONITOR_EVENT_DIR_DELETED;
		else
			type = MONITOR_EVENT_FILE_DELETED;

	} else {
		if (! path_is_dir (path))
			type = MONITOR_EVENT_FILE_CHANGED;
		else
			return;
	}

	if (type == MONITOR_EVENT_FILE_CREATED) {
		if (remove_if_present (window, 
				       MONITOR_EVENT_FILE_DELETED, 
				       path))
			type = MONITOR_EVENT_FILE_CHANGED;
		
	} else if (type == MONITOR_EVENT_FILE_DELETED) {
		remove_if_present (window, MONITOR_EVENT_FILE_CREATED, path);
		remove_if_present (window, MONITOR_EVENT_FILE_CHANGED, path);

	} else if (type == MONITOR_EVENT_DIR_CREATED) {
		remove_if_present (window, MONITOR_EVENT_DIR_DELETED, path);

	} else if (type == MONITOR_EVENT_DIR_DELETED) 
		remove_if_present (window, MONITOR_EVENT_DIR_CREATED, path);

	window->monitor_events[type] = g_list_append (window->monitor_events[type], g_strdup (path));
}


static void
directory_changed (GnomeVFSMonitorHandle    *handle,
		   const char               *monitor_uri,
		   const char               *info_uri,
		   GnomeVFSMonitorEventType  event_type,
		   gpointer                  user_data)
{
	GThumbWindow *window = user_data; 
	char         *path;

	if (window->sidebar_content != DIR_LIST)
		return;

	path = gnome_vfs_unescape_string (info_uri + strlen ("file://"), NULL);
	_window_add_monitor_event (window, event_type, path);
	g_free (path);

	if (window->update_changes_timeout != 0) 
		g_source_remove (window->update_changes_timeout);
	
	window->update_changes_timeout = g_timeout_add (UPDATE_DIR_DELAY,
							_proc_monitor_events,
							window);
}


/* -- go to directory -- */


void
window_add_monitor (GThumbWindow *window)
{
	GnomeVFSResult  result;
	char           *uri;

	if (window->sidebar_content != DIR_LIST)
		return;

	if (window->dir_list->path == NULL)
		return;

	if (window->monitor_handle != NULL)
		gnome_vfs_monitor_cancel (window->monitor_handle);

	uri = g_strconcat ("file://", window->dir_list->path, NULL);
	result = gnome_vfs_monitor_add (&window->monitor_handle,
					uri,
					GNOME_VFS_MONITOR_DIRECTORY,
					directory_changed,
					window);
	g_free (uri);
	window->monitor_enabled = (result == GNOME_VFS_OK);
}


void
window_remove_monitor (GThumbWindow *window)
{
	if (window->monitor_handle != NULL) {
		gnome_vfs_monitor_cancel (window->monitor_handle);
		window->monitor_handle = NULL;
	}
	window->monitor_enabled = FALSE;
}


static void
set_dir_list_continue (gpointer data)
{
	GThumbWindow   *window = data;

	window_update_title (window);
	window_update_sensitivity (window);
	window_make_current_image_visible (window);

	window_add_monitor (window);
}


static void
go_to_directory_continue (DirList  *dir_list,
			  gpointer  data)
{
	GThumbWindow *window = data;
	char         *path;
	GList        *file_list;

	window_stop_activity_mode (window);

	if (dir_list->result != GNOME_VFS_ERROR_EOF) {
		char *utf8_path;

		utf8_path = g_locale_to_utf8 (dir_list->try_path, -1,
					      NULL, NULL, NULL);
		_gtk_error_dialog_run (GTK_WINDOW (window->app),
				       _("Cannot load folder \"%s\": %s\n"), 
				       utf8_path, 
				       gnome_vfs_result_to_string (dir_list->result));
		g_free (utf8_path);

		window->changing_directory = FALSE;
		return;
	}

	path = dir_list->path;

	set_command_sensitive (window, "Go_Up", strcmp (path, "/") != 0);
	
	/* Add to history list if not present as last entry. */

	if ((window->go_op == WINDOW_GO_TO)
	    && ((window->history_current == NULL) 
		|| (strcmp (path, pref_util_remove_prefix (window->history_current->data)) != 0))) 
		add_history_item (window, path, FILE_PREFIX);
	else
		window->go_op = WINDOW_GO_TO;
	window_update_history_list (window);

	set_location (window, path);

	window->changing_directory = FALSE;

	/**/

	file_list = dir_list_get_file_list (window->dir_list);
	window_set_file_list (window, file_list, 
			      set_dir_list_continue, window);
	g_list_free (file_list);
}


/* -- real_go_to_directory -- */


typedef struct {
	GThumbWindow *window;
	char         *path;
} GoToData;


void
go_to_directory__step4 (GoToData *gt_data)
{
	GThumbWindow *window = gt_data->window;
	char         *dir_path = gt_data->path;
	char         *path;

	if (window->slideshow)
		window_stop_slideshow (window);

	path = remove_ending_separator (dir_path);

	window_start_activity_mode (window);
	dir_list_change_to (window->dir_list, 
			    path, 
			    go_to_directory_continue,
			    window);
	g_free (path);

	g_free (gt_data->path);
	g_free (gt_data);
}


void
go_to_directory__step3 (GoToData *gt_data)
{
	GThumbWindow *window = gt_data->window;

	window->setting_file_list = FALSE;

	/* Select the directory view. */

	_window_set_sidebar (window, DIR_LIST);
	if (! window->refreshing)
		window_show_sidebar (window);
	else
		window->refreshing = FALSE;

	/**/

	window->changing_directory = TRUE;

	if (window->file_list->doing_thumbs)
		gth_file_list_interrupt_thumbs (window->file_list, 
					    (DoneFunc) go_to_directory__step4,
					    gt_data);
	else
		go_to_directory__step4 (gt_data);
}


void
go_to_directory__step2 (GoToData *gt_data)
{
	GThumbWindow *window = gt_data->window;

	if (window->setting_file_list)
		gth_file_list_interrupt_set_list (window->file_list,
						  (DoneFunc) go_to_directory__step3,
						  gt_data);
	else
		go_to_directory__step3 (gt_data);
}


static gboolean
go_to_directory_cb (gpointer data)
{
	GoToData     *gt_data = data;
	GThumbWindow *window = gt_data->window;

	g_source_remove (window->load_dir_timeout);
	window->load_dir_timeout = 0;

	if (window->changing_directory) {
		window_stop_activity_mode (window);
		dir_list_interrupt_change_to (window->dir_list,
					      (DoneFunc)go_to_directory__step2,
					      gt_data);
	} else
		go_to_directory__step2 (gt_data);

	return FALSE;
}


/* used in : goto_dir_set_list_interrupted, window_go_to_directory. */
static void
real_go_to_directory (GThumbWindow *window,
		      const char   *dir_path)
{
	GoToData *gt_data;

	gt_data = g_new (GoToData, 1);
	gt_data->window = window;
	gt_data->path = g_strdup (dir_path);

	if (window->load_dir_timeout != 0) {
		g_source_remove (window->load_dir_timeout);
		window->load_dir_timeout = 0;
	}

	window->load_dir_timeout = g_timeout_add (LOAD_DIR_DELAY,
						  go_to_directory_cb, 
						  gt_data);

	/**/

}


typedef struct {
	GThumbWindow *window;
	char         *dir_path;
} GoToDir_SetListInterruptedData;


static void
go_to_dir_set_list_interrupted (gpointer callback_data)
{
	GoToDir_SetListInterruptedData *data = callback_data;

	data->window->setting_file_list = FALSE;
	window_stop_activity_mode (data->window);

	real_go_to_directory (data->window, data->dir_path);
	g_free (data->dir_path);
	g_free (data);
}


void
window_go_to_directory (GThumbWindow *window,
			const char   *dir_path)
{
	g_return_if_fail (window != NULL);

	if (window->slideshow)
		window_stop_slideshow (window);

	if (window->monitor_handle != NULL) {
		gnome_vfs_monitor_cancel (window->monitor_handle);
		window->monitor_handle = NULL;
	}

	if (window->setting_file_list) {
		GoToDir_SetListInterruptedData *sli_data;

		sli_data = g_new (GoToDir_SetListInterruptedData, 1);
		sli_data->window = window;
		sli_data->dir_path = g_strdup (dir_path);
		gth_file_list_interrupt_set_list (window->file_list,
					      go_to_dir_set_list_interrupted,
					      sli_data);
		return;
	}

	real_go_to_directory (window, dir_path);
}


/* -- */


void
window_go_to_catalog_directory (GThumbWindow *window,
				const char   *catalog_dir)
{
	char *base_dir;
	char *catalog_dir2;
	char *catalog_dir3;
	char *current_path;

	catalog_dir2 = remove_special_dirs_from_path (catalog_dir);
	catalog_dir3 = remove_ending_separator (catalog_dir2);
	g_free (catalog_dir2);

	if (catalog_dir3 == NULL)
		return; /* FIXME */

	catalog_list_change_to (window->catalog_list, catalog_dir3);
	set_location (window, catalog_dir3);
	g_free (catalog_dir3);

	/* Update Go_Up command sensibility */

	current_path = window->catalog_list->path;
	base_dir = get_catalog_full_path (NULL);
	set_command_sensitive (window, "Go_Up", 
			       ((current_path != NULL)
				&& strcmp (current_path, base_dir)) != 0);
	g_free (base_dir);
}


/* -- window_go_to_catalog -- */


void
go_to_catalog__step2 (GoToData *gt_data)
{
	GThumbWindow *window = gt_data->window;
	char         *catalog_path = gt_data->path;
	GtkTreeIter   iter;
	char         *catalog_dir;

	if (window->slideshow)
		window_stop_slideshow (window);

	if (catalog_path == NULL) {
		window_set_file_list (window, NULL, NULL, NULL);
		if (window->catalog_path)
			g_free (window->catalog_path);
		window->catalog_path = NULL;
		g_free (gt_data->path);
		g_free (gt_data);
		window_update_title (window);
		return;
	}

	if (! path_is_file (catalog_path)) {
		_gtk_error_dialog_run (GTK_WINDOW (window->app),
				       _("The specified catalog does not exist."));

		/* window_go_to_directory (window, g_get_home_dir ()); FIXME */
		g_free (gt_data->path);
		g_free (gt_data);
		window_update_title (window);
		return;
	}

	if (window->catalog_path != catalog_path) {
		if (window->catalog_path)
			g_free (window->catalog_path);
		window->catalog_path = g_strdup (catalog_path);
	}

	_window_set_sidebar (window, CATALOG_LIST); 
	if (! window->refreshing && ! ViewFirstImage)
		window_show_sidebar (window);
	else
		window->refreshing = FALSE;

	catalog_dir = remove_level_from_path (catalog_path);
	window_go_to_catalog_directory (window, catalog_dir);
	g_free (catalog_dir);

	if (! catalog_list_get_iter_from_path (window->catalog_list,
					       catalog_path,
					       &iter)) {
		g_free (gt_data->path);
		g_free (gt_data);
		return;
	}

	catalog_list_select_iter (window->catalog_list, &iter);
	catalog_activate (window, catalog_path);

	g_free (gt_data->path);
	g_free (gt_data);
}


void
window_go_to_catalog (GThumbWindow *window,
		      const char  *catalog_path)
{
	GoToData *gt_data;

	g_return_if_fail (window != NULL);

	gt_data = g_new (GoToData, 1);
	gt_data->window = window;
	gt_data->path = g_strdup (catalog_path);
	
	if (window->file_list->doing_thumbs)
		gth_file_list_interrupt_thumbs (window->file_list, 
					    (DoneFunc) go_to_catalog__step2,
					    gt_data);
	else
		go_to_catalog__step2 (gt_data);
}


gboolean
window_go_up__is_base_dir (GThumbWindow *window,
			   const char   *dir)
{
	if (dir == NULL)
		return FALSE;

	if (window->sidebar_content == DIR_LIST)
		return (strcmp (dir, "/") == 0);
	else {
		char *catalog_base = get_catalog_full_path (NULL);
		gboolean is_base_dir;

		is_base_dir = strcmp (dir, catalog_base) == 0;
		g_free (catalog_base);
		return is_base_dir;
	}
	
	return FALSE;
}


void
window_go_up (GThumbWindow *window)
{
	char *current_path;
	char *up_dir;

	g_return_if_fail (window != NULL);

	if (window->sidebar_content == DIR_LIST) 
		current_path = window->dir_list->path;
	else 
		current_path = window->catalog_list->path;

	if (current_path == NULL)
		return;

	if (window_go_up__is_base_dir (window, current_path))
		return;

	up_dir = g_strdup (current_path);
	do {
		char *tmp = up_dir;
		up_dir = remove_level_from_path (tmp);
		g_free (tmp);
	} while (! window_go_up__is_base_dir (window, up_dir) 
		 && ! path_is_dir (up_dir));

	if (window->sidebar_content == DIR_LIST)
		window_go_to_directory (window, up_dir);
	else {
		window_go_to_catalog (window, NULL);
		window_go_to_catalog_directory (window, up_dir);
	}

	g_free (up_dir);
}


static void 
go_to_current_location (GThumbWindow *window,
			WindowGoOp go_op)
{
	char *location;

	if (window->history_current == NULL)
		return;

	window->go_op = go_op;

	location = window->history_current->data;
	if (pref_util_location_is_catalog (location))
		window_go_to_catalog (window, pref_util_get_catalog_location (location));
	else if (pref_util_location_is_search (location))
		window_go_to_catalog (window, pref_util_get_search_location (location));
	else if (pref_util_location_is_file (location)) 
		window_go_to_directory (window, pref_util_get_file_location (location));
}


void
window_go_back (GThumbWindow *window)
{
	g_return_if_fail (window != NULL);
	
	if (window->history_current == NULL)
		return;

	if (window->history_current->next == NULL)
		return;

	window->history_current = window->history_current->next;
	go_to_current_location (window, WINDOW_GO_BACK);
}


void
window_go_forward (GThumbWindow *window)
{
	if (window->history_current == NULL)
		return;

	if (window->history_current->prev == NULL)
		return;

	window->history_current = window->history_current->prev;
	go_to_current_location (window, WINDOW_GO_FORWARD);
}


void
window_delete_history (GThumbWindow *window)
{
	bookmarks_remove_all (window->history);
	window->history_current = NULL;
	window_update_history_list (window);
}


static gboolean
view_focused_image (GThumbWindow *window)
{
	int       pos;
	char     *focused;
	gboolean  not_focused;

	pos = image_list_get_focus_image (IMAGE_LIST (window->file_list->ilist));
	if (pos == -1)
		return FALSE;

	focused = gth_file_list_path_from_pos (window->file_list, pos);
	if (focused == NULL)
		return FALSE;

	not_focused = strcmp (window->image_path, focused) != 0;
	g_free (focused);

	return not_focused;
}


gboolean
window_show_next_image (GThumbWindow *window)
{
	int pos;

	g_return_val_if_fail (window != NULL, FALSE);

	if ((window->setting_file_list) || (window->changing_directory)) 
		return FALSE;

	if (window->image_path == NULL) {
		pos = gth_file_list_next_image (window->file_list, -1, TRUE);

	} else if (view_focused_image (window)) {
		pos = image_list_get_focus_image (IMAGE_LIST (window->file_list->ilist));
		if (pos == -1)
			pos = gth_file_list_next_image (window->file_list, pos, TRUE);

	} else {
		pos = gth_file_list_pos_from_path (window->file_list, window->image_path);
		pos = gth_file_list_next_image (window->file_list, pos, TRUE);
	}

	if (pos == -1) 
		return FALSE;

	gth_file_list_select_image_by_pos (window->file_list, pos); 
	view_image_at_pos (window, pos); 

	return TRUE;
}


gboolean
window_show_prev_image (GThumbWindow *window)
{
	int pos;

	g_return_val_if_fail (window != NULL, FALSE);

	if ((window->setting_file_list) || (window->changing_directory))
		return FALSE;
	
	if (window->image_path == NULL) {
		pos = IMAGE_LIST (window->file_list->ilist)->images;
		pos = gth_file_list_prev_image (window->file_list, pos, TRUE);
			
	} else if (view_focused_image (window)) {
		pos = image_list_get_focus_image (IMAGE_LIST (window->file_list->ilist));
		if (pos == -1) {
			pos = IMAGE_LIST (window->file_list->ilist)->images;
			pos = gth_file_list_prev_image (window->file_list, pos, TRUE);
		}

	} else {
		pos = gth_file_list_pos_from_path (window->file_list, window->image_path);
		pos = gth_file_list_prev_image (window->file_list, pos, TRUE);
	}

	if (pos == -1)
		return FALSE;

	gth_file_list_select_image_by_pos (window->file_list, pos); 
	view_image_at_pos (window, pos); 

	return TRUE;
}


gboolean
window_show_first_image (GThumbWindow *window)
{
	if (IMAGE_LIST (window->file_list->ilist)->images == 0)
		return FALSE;

	if (window->image_path) {
		g_free (window->image_path);
		window->image_path = NULL;
	}

	return window_show_next_image (window);
}


gboolean
window_show_last_image (GThumbWindow *window)
{
	if (IMAGE_LIST (window->file_list->ilist)->images == 0)
		return FALSE;

	if (window->image_path) {
		g_free (window->image_path);
		window->image_path = NULL;
	}

	return window_show_prev_image (window);
}


/* -- slideshow -- */


static gboolean
slideshow_timeout_cb (gpointer data)
{
	GThumbWindow *window = data;
	gboolean      go_on;

	if (window->slideshow_timeout != 0) {
		g_source_remove (window->slideshow_timeout);
		window->slideshow_timeout = 0;
	}

	if (pref_get_slideshow_direction () == DIRECTION_FORWARD)
		go_on = window_show_next_image (window);
	else
		go_on = window_show_prev_image (window);

	if (! go_on) {
		if (! eel_gconf_get_boolean (PREF_SLIDESHOW_WRAP_AROUND)) 
			window_stop_slideshow (window);
		else {
			if (pref_get_slideshow_direction () == DIRECTION_FORWARD) {
				go_on = window_show_first_image (window);
			} else
				go_on = window_show_last_image (window);
			
			if (! go_on) 
				/* No image to show, stop. */
				window_stop_slideshow (window);
		}
	}

	if (go_on)
		window->slideshow_timeout = g_timeout_add (eel_gconf_get_integer (PREF_SLIDESHOW_DELAY) * 1000, slideshow_timeout_cb, window);

	return FALSE;
}


void
window_start_slideshow (GThumbWindow *window)
{
	gboolean go_on = TRUE;
	int      pos;

	g_return_if_fail (window != NULL);

	if (window->file_list->list == NULL)
		return;

	if (window->slideshow)
		return;

	window->slideshow = TRUE;

	if (eel_gconf_get_boolean (PREF_SLIDESHOW_FULLSCREEN))
		fullscreen_start (fullscreen, window);

	pos = gth_file_list_pos_from_path (window->file_list, window->image_path);
	if (pos == -1) {
		if (pref_get_slideshow_direction () == DIRECTION_FORWARD)
			go_on = window_show_next_image (window);
		else
			go_on = window_show_prev_image (window);
	}

	if (! go_on) {
		window_stop_slideshow (window);
		return;
	}

	window->slideshow_timeout = g_timeout_add (eel_gconf_get_integer (PREF_SLIDESHOW_DELAY) * 1000, slideshow_timeout_cb, window);
}


void
window_stop_slideshow (GThumbWindow *window)
{
	if (! window->slideshow)
		return;

	window->slideshow = FALSE;
	if (window->slideshow_timeout != 0) {
		g_source_remove (window->slideshow_timeout);
		window->slideshow_timeout = 0;
	}

	if (eel_gconf_get_boolean (PREF_SLIDESHOW_FULLSCREEN) && window->fullscreen)
		fullscreen_stop (fullscreen);
}


void
window_show_image_prop (GThumbWindow *window)
{
	if (window->image_prop_dlg == NULL) 
		window->image_prop_dlg = dlg_image_prop_new (window);
	else
		gtk_window_present (GTK_WINDOW (window->image_prop_dlg));
}


void
window_image_modified (GThumbWindow *window,
		       gboolean      modified)
{
	window->image_modified = modified;
	window_update_infobar (window);
	window_update_title (window);

	set_command_sensitive (window, "File_Revert", ! image_viewer_is_void (IMAGE_VIEWER (window->viewer)) && window->image_modified);

	if (modified && window->image_prop_dlg != NULL)
		dlg_image_prop_update (window->image_prop_dlg);
}



/* -- load image -- */


static gboolean
load_timeout_cb (gpointer data)
{
	GThumbWindow *window = data;
	char         *prev1;
	char         *next1;
	char         *next2;
	int           pos;

	if (window->view_image_timeout != 0) {
		g_source_remove (window->view_image_timeout);
		window->view_image_timeout = 0;
	}

	pos = gth_file_list_pos_from_path (window->file_list, window->image_path);
	prev1 = gth_file_list_path_from_pos (window->file_list, pos - 1);
	next1 = gth_file_list_path_from_pos (window->file_list, pos + 1);
	next2 = gth_file_list_path_from_pos (window->file_list, pos + 2);

	gthumb_preloader_start (window->preloader, 
				window->image_path, 
				next1, 
				prev1, 
				next2);

	g_free (prev1);
	g_free (next1);
	g_free (next2);

	return FALSE;
}


void
window_reload_image (GThumbWindow *window)
{
	g_return_if_fail (window != NULL);
	
	if (window->image_path != NULL)
		load_timeout_cb (window);
}



void
window_load_image (GThumbWindow *window, 
		   const char   *filename)
{
	g_return_if_fail (window != NULL);

	if (filename == window->image_path) {
		window_reload_image (window);
		return;
	}

	if (! window->image_modified
	    && (window->image_path != NULL) 
	    && (filename != NULL)
	    && (strcmp (filename, window->image_path) == 0)
	    && (window->image_mtime == get_file_mtime (window->image_path))) 
		return;

	if (window->view_image_timeout != 0) {
		g_source_remove (window->view_image_timeout);
		window->view_image_timeout = 0;
	}
	
	/* If the image is from a catalog remember the catalog name. */

	if (window->image_catalog != NULL) {
		g_free (window->image_catalog);
		window->image_catalog = NULL;
	}
	if (window->sidebar_content == CATALOG_LIST)
		window->image_catalog = g_strdup (window->catalog_path);

	/**/

	if (window->image_path)
		g_free (window->image_path);
	window->image_path = NULL;

	if (filename == NULL) {
		image_viewer_set_void (IMAGE_VIEWER (window->viewer));
		window_update_title (window);
		window_update_statusbar_image_info (window);
		window_update_infobar (window);
		window_update_sensitivity (window);
		return;
	}
		
	window->image_path = g_strdup (filename);

	window->view_image_timeout = g_timeout_add (VIEW_IMAGE_DELAY,
						    load_timeout_cb, 
						    window);
}


/* -- changes notification functions -- */


void
notify_files_added__step2 (gpointer data)
{
	GThumbWindow *window = data;
	window_update_statusbar_list_info (window);
	window_update_infobar (window);
}


static void
notify_files_added (GThumbWindow *window,
		    GList        *list)
{
	ImageList *ilist;

	ilist = IMAGE_LIST (window->file_list->ilist);
	gth_file_list_add_list (window->file_list, 
			    list, 
			    notify_files_added__step2,
			    window);
}


void
window_notify_files_created (GThumbWindow *window,
			     GList        *list)
{
	GList *scan;
	GList *created_in_current_dir = NULL;
	char  *current_dir;

	if (window->sidebar_content != DIR_LIST)
		return;

	current_dir = window->dir_list->path;
	if (current_dir == NULL)
		return;

	for (scan = list; scan; scan = scan->next) {
		char *path = scan->data;
		char *parent_dir;
		
		parent_dir = remove_level_from_path (path);

		if (strcmp (parent_dir, current_dir) == 0)
			created_in_current_dir = g_list_prepend (created_in_current_dir, path);

		g_free (parent_dir);
	}

	if (created_in_current_dir != NULL) {
		notify_files_added (window, created_in_current_dir);
		g_list_free (created_in_current_dir);
	}
}


typedef struct {
	GThumbWindow *window;
	GList        *list;
	gboolean      restart_thumbs;
} FilesDeletedData;


static void
notify_files_deleted__step2 (FilesDeletedData *data)
{
	GThumbWindow *window = data->window;
	GList        *list = data->list;
	GList        *scan;
	char         *filename;
	int           pos, smallest_pos, image_pos;
	gboolean      current_image_deleted = FALSE;
	gboolean      no_image_viewed;
	ImageList    *ilist;

	ilist = IMAGE_LIST (window->file_list->ilist);
	image_list_freeze (ilist);

	pos = -1;
	smallest_pos = -1;
	image_pos = -1;
	if (window->image_path)
		image_pos = gth_file_list_pos_from_path (window->file_list, 
						     window->image_path);
	no_image_viewed = (image_pos == -1);

	for (scan = list; scan; scan = scan->next) {
		filename = scan->data;

		pos = gth_file_list_pos_from_path (window->file_list, filename);
		if (pos == -1) 
			continue;

		if (image_pos == pos) {
			/* the current image will be deleted. */
			image_pos = -1;
			current_image_deleted = TRUE;
		} else if (image_pos > pos)
			/* a previous image will be deleted, so image_pos 
			 * decrements its value. */
			image_pos--;

		if (scan == list)
			smallest_pos = pos;
		else
			smallest_pos = MIN (smallest_pos, pos);

		gth_file_list_delete_pos (window->file_list, pos);
	}
	image_list_thaw (ilist);

	/* Try to visualize the smallest pos. */
	if (smallest_pos != -1) {
		pos = smallest_pos;

		if (pos > ilist->images - 1)
			pos = ilist->images - 1;
		if (pos < 0)
			pos = 0;

		image_list_moveto (ilist, pos, 0.5);
	}

	if (! no_image_viewed) {
		if (current_image_deleted) {
			/* delete the image from the viewer. */
			window_load_image (window, NULL);

			if ((ilist->images > 0) && (smallest_pos != -1)) {
				pos = smallest_pos;
				
				if (pos > ilist->images - 1)
					pos = ilist->images - 1;
				if (pos < 0)
					pos = 0;
				
				view_image_at_pos (window, pos);
				gth_file_list_select_image_by_pos (window->file_list, pos);
			}
		}
	}

	window_update_statusbar_list_info (window);

	if (data->restart_thumbs)
		gth_file_list_restart_thumbs (data->window->file_list, TRUE);

	path_list_free (data->list);
	g_free (data);
}


static void
notify_files_deleted (GThumbWindow *window,
		      GList        *list)
{
	FilesDeletedData *data;

	if (list == NULL)
		return;

	data = g_new (FilesDeletedData, 1);

	data->window = window;
	data->list = path_list_dup (list);
	data->restart_thumbs = window->file_list->doing_thumbs;
	
	gth_file_list_interrupt_thumbs (window->file_list,
				    (DoneFunc) notify_files_deleted__step2,
				    data);
}


void
window_notify_files_deleted (GThumbWindow *window,
			     GList        *list)
{
	g_return_if_fail (window != NULL);

	if ((window->sidebar_content == CATALOG_LIST)
	    && (window->catalog_path != NULL)) { /* update the catalog. */
		Catalog *catalog;
		GList   *scan;

		catalog = catalog_new ();
		if (catalog_load_from_disk (catalog, window->catalog_path, NULL)) {
			for (scan = list; scan; scan = scan->next)
				catalog_remove_item (catalog, (char*) scan->data);
			catalog_write_to_disk (catalog, NULL);
		}
		catalog_free (catalog);
	} else if (window->monitor_enabled)
		return;

	notify_files_deleted (window, list);
}


void
window_notify_files_changed (GThumbWindow *window,
			     GList        *list)
{
	if (! window->file_list->doing_thumbs)
		gth_file_list_update_thumb_list (window->file_list, list);

	if (window->image_path != NULL) {
		int pos;
		pos = gth_file_list_pos_from_path (window->file_list,
					       window->image_path);
		if (pos != -1)
			view_image_at_pos (window, pos);
	}
}


void
window_notify_cat_files_added (GThumbWindow *window,
			       const char   *catalog_name,
			       GList        *list)
{
	g_return_if_fail (window != NULL);

	if (window->sidebar_content != CATALOG_LIST)
		return;
	if (window->catalog_path == NULL)
		return;
	if (strcmp (window->catalog_path, catalog_name) != 0)
		return;

	notify_files_added (window, list);
}


void
window_notify_cat_files_deleted (GThumbWindow *window,
				 const char   *catalog_name,
				 GList        *list)
{
	g_return_if_fail (window != NULL);

	if (window->sidebar_content != CATALOG_LIST)
		return;
	if (window->catalog_path == NULL)
		return;
	if (strcmp (window->catalog_path, catalog_name) != 0)
		return;

	notify_files_deleted (window, list);
}

	
void
window_notify_file_rename (GThumbWindow *window,
			   const char   *old_name,
			   const char   *new_name)
{
	int pos;

	g_return_if_fail (window != NULL);
	if ((old_name == NULL) || (new_name == NULL))
		return;

	if ((window->sidebar_content == CATALOG_LIST)
	    && (window->catalog_path != NULL)) { /* update the catalog. */
		Catalog  *catalog;
		GList    *scan;
		gboolean  changed = FALSE;

		catalog = catalog_new ();
		if (catalog_load_from_disk (catalog, window->catalog_path, NULL)) {
			for (scan = catalog->list; scan; scan = scan->next) {
				char *entry = scan->data;
				if (strcmp (entry, old_name) == 0) {
					catalog_remove_item (catalog, old_name);
					catalog_add_item (catalog, new_name);
					changed = TRUE;
				}
			}
			if (changed)
				catalog_write_to_disk (catalog, NULL);
		}
		catalog_free (catalog);
	}

	pos = gth_file_list_pos_from_path (window->file_list, new_name);
	if (pos != -1)
		gth_file_list_delete_pos (window->file_list, pos);

	pos = gth_file_list_pos_from_path (window->file_list, old_name);
	if (pos != -1)
		gth_file_list_rename_pos (window->file_list, pos, new_name);

	if ((window->image_path != NULL) 
	    && strcmp (old_name, window->image_path) == 0) 
		window_load_image (window, new_name);
}


static gboolean
first_level_sub_directory (GThumbWindow *window,
			   const char   *current,
			   const char   *old_path)
{
	const char *old_name;
	int         current_l;
	int         old_path_l;

	current_l = strlen (current);
	old_path_l = strlen (old_path);

	if (old_path_l <= current_l + 1)
		return FALSE;

	old_name = old_path + current_l + 1;

	return (strchr (old_name, '/') == NULL);
}


void
window_notify_directory_rename (GThumbWindow *window,
				const char  *old_name,
				const char  *new_name)
{
	if (window->sidebar_content == DIR_LIST) {
		if (strcmp (window->dir_list->path, old_name) == 0) 
			window_go_to_directory (window, new_name);
		else {
			const char *current = window->dir_list->path;

			/* a sub directory got renamed, refresh. */
			if (first_level_sub_directory (window, current, old_name))  {
				dir_list_remove_directory (window->dir_list, 
							   file_name_from_path (old_name));
				dir_list_add_directory (window->dir_list, 
							file_name_from_path (new_name));
			}
		}
		
	} else if (window->sidebar_content == CATALOG_LIST) {
		if (strcmp (window->catalog_list->path, old_name) == 0) 
			window_go_to_catalog_directory (window, new_name);
		else {
			const char *current = window->catalog_list->path;
			if (first_level_sub_directory (window, current, old_name))  
				window_update_catalog_list (window);
		}
	}

	if ((window->image_path != NULL) 
	    && (window->sidebar_content == DIR_LIST)
	    && (strncmp (window->image_path, 
			 old_name,
			 strlen (old_name)) == 0)) {
		char *new_image_name;

		new_image_name = g_strconcat (new_name,
					      window->image_path + strlen (old_name),
					      NULL);
		window_notify_file_rename (window, 
					   window->image_path,
					   new_image_name);
		g_free (new_image_name);
	}
}


void
window_notify_directory_delete (GThumbWindow *window,
				const char   *path)
{
	if (window->sidebar_content == DIR_LIST) {
		if (strcmp (window->dir_list->path, path) == 0) 
			window_go_up (window);
		else {
			const char *current = window->dir_list->path;
			if (first_level_sub_directory (window, current, path))
				dir_list_remove_directory (window->dir_list, 
							   file_name_from_path (path));
		}

	} else if (window->sidebar_content == CATALOG_LIST) {
		if (strcmp (window->catalog_list->path, path) == 0) 
			window_go_up (window);
		else {
			const char *current = window->catalog_list->path;
			if (strncmp (current, path, strlen (current)) == 0)
				/* a sub directory got deleted, refresh. */
				window_update_catalog_list (window);
		}
	}

	if ((window->image_path != NULL) 
	    && (strncmp (window->image_path, 
			 path,
			 strlen (path)) == 0)) {
		GList *list;
		
		list = g_list_append (NULL, window->image_path);
		window_notify_files_deleted (window, list);
		g_list_free (list);
	}
}


void
window_notify_directory_new (GThumbWindow *window,
			     const char  *path)
{
	if (window->sidebar_content == DIR_LIST) {
		const char *current = window->dir_list->path;
		if (first_level_sub_directory (window, current, path))
			dir_list_add_directory (window->dir_list, 
						file_name_from_path (path));

	} else if (window->sidebar_content == CATALOG_LIST) {
		const char *current = window->catalog_list->path;
		if (strncmp (current, path, strlen (current)) == 0)
			/* a sub directory was created, refresh. */
			window_update_catalog_list (window);
	}
}


void
window_notify_catalog_rename (GThumbWindow *window,
			      const char  *old_path,
			      const char  *new_path)
{
	char     *catalog_dir;
	gboolean  viewing_a_catalog;
	gboolean  current_cat_renamed;
	gboolean  renamed_cat_is_in_current_dir;

	if (window->sidebar_content != CATALOG_LIST) 
		return;

	if (window->catalog_list->path == NULL)
		return;

	catalog_dir = remove_level_from_path (window->catalog_list->path);
	viewing_a_catalog = (window->catalog_path != NULL);
	current_cat_renamed = ((window->catalog_path != NULL) && (strcmp (window->catalog_path, old_path) == 0));
	renamed_cat_is_in_current_dir = strncmp (catalog_dir, new_path, strlen (catalog_dir)) == 0;

	if (! renamed_cat_is_in_current_dir) {
		g_free (catalog_dir);
		return;
	}

	if (! viewing_a_catalog) 
		window_go_to_catalog_directory (window, window->catalog_list->path);
	else {
		if (current_cat_renamed)
			window_go_to_catalog (window, new_path);
		else {
			GtkTreeIter iter;
			window_go_to_catalog_directory (window, window->catalog_list->path);

			/* reselect the current catalog. */
			if (catalog_list_get_iter_from_path (window->catalog_list, 
							     window->catalog_path, &iter))
				catalog_list_select_iter (window->catalog_list, &iter);
		}
	}

	g_free (catalog_dir);
}


void
window_notify_catalog_new (GThumbWindow *window,
			   const char   *path)
{
	char     *catalog_dir;
	gboolean  viewing_a_catalog;
	gboolean  created_cat_is_in_current_dir;

	if (window->sidebar_content != CATALOG_LIST) 
		return;

	if (window->catalog_list->path == NULL)
		return;

	viewing_a_catalog = (window->catalog_path != NULL);
	catalog_dir = remove_level_from_path (window->catalog_list->path);
	created_cat_is_in_current_dir = strncmp (catalog_dir, path, strlen (catalog_dir)) == 0;

	if (! created_cat_is_in_current_dir) {
		g_free (catalog_dir);
		return;
	}

	window_go_to_catalog_directory (window, window->catalog_list->path);

	if (viewing_a_catalog) {
		GtkTreeIter iter;
			
		/* reselect the current catalog. */
		if (catalog_list_get_iter_from_path (window->catalog_list, 
						     window->catalog_path, 
						     &iter))
			catalog_list_select_iter (window->catalog_list, &iter);
	}

	g_free (catalog_dir);
}


void
window_notify_catalog_delete (GThumbWindow *window,
			      const char   *path)
{
	char     *catalog_dir;
	gboolean  viewing_a_catalog;
	gboolean  current_cat_deleted;
	gboolean  deleted_cat_is_in_current_dir;

	if (window->sidebar_content != CATALOG_LIST) 
		return;

	if (window->catalog_list->path == NULL)
		return;

	catalog_dir = remove_level_from_path (window->catalog_list->path);
	viewing_a_catalog = (window->catalog_path != NULL);
	current_cat_deleted = ((window->catalog_path != NULL) && (strcmp (window->catalog_path, path) == 0));
	deleted_cat_is_in_current_dir = strncmp (catalog_dir, path, strlen (catalog_dir)) == 0;

	if (! deleted_cat_is_in_current_dir) {
		g_free (catalog_dir);
		return;
	}

	if (! viewing_a_catalog) 
		window_go_to_catalog_directory (window, window->catalog_list->path);
	else {
		if (current_cat_deleted) {
			window_go_to_catalog (window, NULL);
			window_go_to_catalog_directory (window, window->catalog_list->path);
		} else {
			GtkTreeIter iter;
			window_go_to_catalog_directory (window, window->catalog_list->path);
			
			/* reselect the current catalog. */
			if (catalog_list_get_iter_from_path (window->catalog_list, 
							     window->catalog_path, &iter))
				catalog_list_select_iter (window->catalog_list, &iter);
		}
	}

	g_free (catalog_dir);
}


void
window_notify_update_comment (GThumbWindow *window,
			      const char   *filename)
{
	int pos;

	g_return_if_fail (window != NULL);

	pos = gth_file_list_pos_from_path (window->file_list, filename);
	if (pos != -1)
		gth_file_list_update_comment (window->file_list, pos);
}


void
window_notify_update_directory (GThumbWindow *window,
				const char   *dir_path)
{
	g_return_if_fail (window != NULL);

	if (window->monitor_enabled)
		return;
	
	if ((window->dir_list->path == NULL) 
	    || (strcmp (window->dir_list->path, dir_path) != 0)) 
		return;

	window_update_file_list (window);
}


void
window_notify_update_layout (GThumbWindow *window)
{
	GtkWidget *paned1;      /* Main paned widget. */
	GtkWidget *paned2;      /* Secondary paned widget. */
	int        paned1_pos;
	int        paned2_pos;
	gboolean   sidebar_visible;

	sidebar_visible = window->sidebar_visible;
	if (! sidebar_visible) 
		window_show_sidebar (window);

	window->layout_type = eel_gconf_get_integer (PREF_UI_LAYOUT);

	paned1_pos = gtk_paned_get_position (GTK_PANED (window->main_pane));
	paned2_pos = gtk_paned_get_position (GTK_PANED (window->content_pane));

	if (window->layout_type == 1) {
		paned1 = gtk_vpaned_new (); 
		paned2 = gtk_hpaned_new ();
	} else {
		paned1 = gtk_hpaned_new (); 
		paned2 = gtk_vpaned_new (); 
	}

	if (window->layout_type == 3)
		gtk_paned_pack2 (GTK_PANED (paned1), paned2, TRUE, FALSE);
	else
		gtk_paned_pack1 (GTK_PANED (paned1), paned2, TRUE, FALSE);

	if (window->layout_type == 3)
		gtk_widget_reparent (window->dir_list_pane, paned1);
	else
		gtk_widget_reparent (window->dir_list_pane, paned2);

	if (window->layout_type == 2) 
		gtk_widget_reparent (window->file_list->root_widget, paned1);
	else 
		gtk_widget_reparent (window->file_list->root_widget, paned2);

	if (window->layout_type <= 1) 
		gtk_widget_reparent (window->image_pane, paned1);
	else 
		gtk_widget_reparent (window->image_pane, paned2);

	gtk_paned_set_position (GTK_PANED (paned1), paned1_pos);
	gtk_paned_set_position (GTK_PANED (paned2), paned2_pos);

	bonobo_window_set_contents (BONOBO_WINDOW (window->app), paned1);

	gtk_widget_destroy (window->main_pane);

	gtk_widget_show (paned1);
	gtk_widget_show (paned2);

	window->main_pane = paned1;
	window->content_pane = paned2;

	if (! sidebar_visible) 
		window_hide_sidebar (window);
}


void
window_notify_update_toolbar_style (GThumbWindow *window)
{
	ToolbarStyle  toolbar_style;
	char         *prop;

	toolbar_style = pref_get_toolbar_style ();

	switch (toolbar_style) {
	case TOOLBAR_STYLE_SYSTEM:
		prop = "system"; break;
	case TOOLBAR_STYLE_TEXT_BELOW:
		prop = "both"; break;
	case TOOLBAR_STYLE_TEXT_BESIDE:
		prop = "both_horiz"; break;
	case TOOLBAR_STYLE_ICONS:
		prop = "icon"; break;
	case TOOLBAR_STYLE_TEXT:
		prop = "text"; break;
	}

	bonobo_ui_component_set_prop (window->ui_component, "/Toolbar", "look", prop, NULL);
	bonobo_ui_component_set_prop (window->ui_component, "/ImageToolbar", "look", prop, NULL);
	e_combo_button_set_style (E_COMBO_BUTTON (window->go_back_combo_button), toolbar_style); 
}


/* -- image operations -- */


static void
pixbuf_op_done_cb (GthPixbufOp   *pixop,
		   gboolean       completed,
		   GThumbWindow  *window)
{
	ImageViewer *viewer = IMAGE_VIEWER (window->viewer);

	if (completed) {
		image_viewer_set_pixbuf (viewer, window->pixop->dest);
		window_image_modified (window, TRUE);
	}

	g_object_unref (window->pixop);
	window->pixop = NULL;

	if (window->progress_dialog != NULL) 
		gtk_widget_hide (window->progress_dialog);
}


static void
pixbuf_op_progress_cb (GthPixbufOp  *pixop,
		       float         p, 
		       GThumbWindow *window)
{
	if (window->progress_dialog != NULL) 
		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (window->progress_progressbar), p);
}


static int
window__display_progress_dialog (gpointer data)
{
	GThumbWindow *window = data;

	if (window->progress_timeout != 0) {
		g_source_remove (window->progress_timeout);
		window->progress_timeout = 0;
	}

	if (window->pixop != NULL)
		gtk_widget_show (window->progress_dialog);

	return FALSE;
}


void
window_exec_pixbuf_op (GThumbWindow *window,
		       GthPixbufOp  *pixop)
{
	window->pixop = pixop;
	g_object_ref (window->pixop);

	gtk_label_set_text (GTK_LABEL (window->progress_info),
			    _("Wait please..."));

	g_signal_connect (G_OBJECT (pixop),
			  "done",
			  G_CALLBACK (pixbuf_op_done_cb),
			  window);
	g_signal_connect (G_OBJECT (pixop),
			  "progress",
			  G_CALLBACK (pixbuf_op_progress_cb),
			  window);

	if (window->progress_dialog != NULL)
		window->progress_timeout = g_timeout_add (DISPLAY_PROGRESS_DELAY, window__display_progress_dialog, window);

	gth_pixbuf_op_start (window->pixop);
}
