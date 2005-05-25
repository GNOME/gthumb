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

#ifndef __MAIN_H__
#define __MAIN_H__

#include "image-viewer.h"
#include "gth-window.h"
#include "preferences.h"
#include "fullscreen.h"

extern GthWindow        *current_window;
extern Preferences       preferences;
extern FullScreen       *fullscreen;

extern gboolean          StartInFullscreen;
extern gboolean          StartSlideshow;
extern gboolean          ViewFirstImage;
extern gboolean          HideSidebar;
extern gboolean          ExitAll;
extern char             *ImageToDisplay;
extern gboolean          FirstStart;
extern gboolean          ImportPhotos;

#define MENU_ICON_SIZE 20.0
#define LIST_ICON_SIZE 20.0


void all_windows_update_file_list              ();

void all_windows_update_catalog_list           ();

void all_windows_update_bookmark_list          ();

void all_windows_update_browser_options        ();

void all_windows_notify_files_created          (GList *list);

void all_windows_notify_files_deleted          (GList *list);

void all_windows_notify_files_changed          (GList *list);

void all_windows_notify_cat_files_added        (const char *catalog_path,
						GList      *list);

void all_windows_notify_cat_files_deleted      (const char *catalog_path,
						GList      *list);

void all_windows_notify_file_rename            (const gchar *oldname,
						const gchar *newname);

void all_windows_notify_files_rename           (GList       *old_names,
						GList       *new_names);

void all_windows_notify_directory_rename       (const gchar *oldname,
						const gchar *newname);

void all_windows_notify_directory_delete       (const gchar *path);

void all_windows_notify_directory_new          (const gchar *path);

void all_windows_notify_catalog_rename         (const gchar *oldname,
						const gchar *newname);

void all_windows_notify_catalog_new            (const gchar *path);

void all_windows_notify_catalog_delete         (const gchar *path);

void all_windows_notify_update_comment         (const gchar *filename);

void all_windows_notify_update_directory       (const gchar *dir_path);

void all_windows_notify_update_icon_theme      ();

void all_windows_remove_monitor                ();

void all_windows_add_monitor                   ();

int        get_folder_pixbuf_size_for_list     (GtkWidget *widget);

int        get_folder_pixbuf_size_for_menu     (GtkWidget *widget);

GdkPixbuf *get_folder_pixbuf                   (double icon_size);

gboolean  folder_is_film                       (const char  *full_path);

#endif /* __MAIN_H__ */
