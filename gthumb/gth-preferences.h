/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001-2009 Free Software Foundation, Inc.
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

#ifndef GTH_PREF_H
#define GTH_PREF_H

#include <glib.h>
#include <gtk/gtk.h>
#include "typedefs.h"

G_BEGIN_DECLS

#define PREF_DESKTOP_ICON_THEME             "/desktop/gnome/file_views/icon_theme"

#define PREF_GO_TO_LAST_LOCATION            "/apps/gthumb/general/go_to_last_location"
#define PREF_USE_STARTUP_LOCATION           "/apps/gthumb/general/use_startup_location"
#define PREF_STARTUP_LOCATION               "/apps/gthumb/general/startup_location"
#define PREF_ACTIVE_EXTENSIONS              "/apps/gthumb/general/active_extensions"
#define PREF_STORE_METADATA_IN_FILES        "/apps/gthumb/general/store_metadata_in_files"
#define PREF_GENERAL_FILTER                 "/apps/gthumb/browser/general_filter"
#define PREF_SHOW_HIDDEN_FILES              "/apps/gthumb/browser/show_hidden_files"
#define PREF_SHOW_THUMBNAILS                "/apps/gthumb/browser/show_thumbnails"
#define PREF_FAST_FILE_TYPE                 "/apps/gthumb/browser/fast_file_type"
#define PREF_SAVE_THUMBNAILS                "/apps/gthumb/browser/save_thumbnails"
#define PREF_THUMBNAIL_SIZE                 "/apps/gthumb/browser/thumbnail_size"
#define PREF_THUMBNAIL_LIMIT                "/apps/gthumb/browser/thumbnail_limit"
#define PREF_THUMBNAIL_CAPTION              "/apps/gthumb/browser/thumbnail_caption"
#define PREF_CLICK_POLICY                   "/apps/gthumb/browser/click_policy"
#define PREF_SORT_TYPE                      "/apps/gthumb/browser/sort_type"
#define PREF_SORT_INVERSE                   "/apps/gthumb/browser/sort_inverse"
#define PREF_VIEW_AS                        "/apps/gthumb/browser/view_as"

#define PREF_ZOOM_QUALITY                   "/apps/gthumb/viewer/zoom_quality"
#define PREF_ZOOM_CHANGE                    "/apps/gthumb/viewer/zoom_change"
#define PREF_TRANSP_TYPE                    "/apps/gthumb/viewer/transparency_type"
#define PREF_RESET_SCROLLBARS               "/apps/gthumb/viewer/reset_scrollbars"
#define PREF_CHECK_TYPE                     "/apps/gthumb/viewer/check_type"
#define PREF_CHECK_SIZE                     "/apps/gthumb/viewer/check_size"
#define PREF_BLACK_BACKGROUND               "/apps/gthumb/viewer/black_background"

#define PREF_UI_TOOLBAR_STYLE               "/apps/gthumb/ui/toolbar_style"
#define PREF_UI_WINDOW_WIDTH                "/apps/gthumb/ui/window_width"
#define PREF_UI_WINDOW_HEIGHT               "/apps/gthumb/ui/window_height"
#define PREF_UI_TOOLBAR_VISIBLE             "/apps/gthumb/ui/toolbar_visible"
#define PREF_UI_STATUSBAR_VISIBLE           "/apps/gthumb/ui/statusbar_visible"
#define PREF_UI_FILTERBAR_VISIBLE           "/apps/gthumb/ui/filterbar_visible"
#define PREF_UI_SIDEBAR_VISIBLE             "/apps/gthumb/ui/sidebar_visible"
#define PREF_UI_BROWSER_SIDEBAR_WIDTH       "/apps/gthumb/ui/browser_sidebar_width"
#define PREF_UI_VIEWER_SIDEBAR_WIDTH        "/apps/gthumb/ui/viewer_sidebar_width"
#define PREF_UI_PROPERTIES_HEIGHT           "/apps/gthumb/ui/properties_height"
#define PREF_UI_COMMENT_HEIGHT              "/apps/gthumb/ui/comment_height"

#define PREF_ADD_TO_CATALOG_LAST_CATALOG    "/apps/gthumb/dialogs/add_to_catalog/last_catalog"
#define PREF_ADD_TO_CATALOG_VIEW            "/apps/gthumb/dialogs/add_to_catalog/view"

#define PREF_MSG_CANNOT_MOVE_TO_TRASH       "/apps/gthumb/dialogs/messages/cannot_move_to_trash"
#define PREF_MSG_SAVE_MODIFIED_IMAGE        "/apps/gthumb/dialogs/messages/save_modified_image"

/* default values */

#define DEFAULT_GENERAL_FILTER "file::type::is_media"
#define DEFAULT_THUMBNAIL_CAPTION "standard::display-name,gth::file::display-size"
#define DEFAULT_UI_WINDOW_WIDTH 690
#define DEFAULT_UI_WINDOW_HEIGHT 460
#define DEFAULT_FAST_FILE_TYPE TRUE
#define DEFAULT_THUMBNAIL_SIZE 128
#define DEFAULT_CONFIRM_DELETION TRUE
#define DEFAULT_MSG_SAVE_MODIFIED_IMAGE TRUE


void             gth_pref_initialize                   (void);
void             gth_pref_release                      (void);
void             gth_pref_set_startup_location         (const char *location);
const char *     gth_pref_get_startup_location         (void);
const char *     gth_pref_get_wallpaper_filename       (void);
const char *     gth_pref_get_wallpaper_options        (void);
GthToolbarStyle  gth_pref_get_real_toolbar_style       (void);
void             gth_pref_save_window_geometry         (GtkWindow  *window,
							 const char *dialog);
void             gth_pref_restore_window_geometry      (GtkWindow  *window,
							const char *dialog);

G_END_DECLS

#endif /* GTH_PREF_H */
