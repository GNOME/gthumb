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

#ifndef GTHUMB_WINDOW_H
#define GTHUMB_WINDOW_H

#include <libbonoboui.h>
#include <libgnomevfs/gnome-vfs-monitor.h>
#include <glade/glade.h>

#include "image-viewer.h"
#include "gth-file-list.h"
#include "dir-list.h"
#include "catalog-list.h"
#include "bookmarks.h"
#include "gth-pixbuf-op.h"
#include "gthumb-preloader.h"

#define GCONF_NOTIFICATIONS 17


typedef enum {
	WINDOW_GO_TO,
	WINDOW_GO_BACK,
	WINDOW_GO_FORWARD
} WindowGoOp;


typedef enum {
	MONITOR_EVENT_FILE_CREATED,
	MONITOR_EVENT_FILE_DELETED,
	MONITOR_EVENT_DIR_CREATED,
	MONITOR_EVENT_DIR_DELETED,
	MONITOR_EVENT_FILE_CHANGED,
	MONITOR_EVENT_NUM
} MonitorEventType;


typedef struct {
	/* layout */

	GtkWidget          *app;                /* The main window. */
	BonoboUIComponent  *ui_component;

	GtkWidget          *viewer;
	GtkWidget          *viewer_container;  /* Container widget for the 
						* viewer.  Used by fullscreen 
						* in order to reparent the 
						* viewer.*/
	GtkWidget          *main_pane;
	GtkWidget          *content_pane;
	GtkWidget          *image_pane;
	GtkWidget          *dir_list_pane;
	GtkWidget          *notebook;
	GtkWidget          *location_entry;
	GtkWidget          *viewer_vscr;
	GtkWidget          *viewer_hscr;
	GtkWidget          *viewer_event_box;
	GtkWidget          *info_bar;
	GtkWidget          *go_back_combo_button;
	GtkWidget          *popup_menu;

	GtkWidget          *progress;              /* statusbar widgets. */
	GtkWidget          *image_info;
	GtkWidget          *image_info_frame;

	GtkWidget          *image_prop_dlg;        /* no-modal dialogs. */
	GtkWidget          *comments_dlg;
	GtkWidget          *categories_dlg;
	GtkWidget          *bookmarks_dlg;

	char                sidebar_content;       /* SidebarContent values. */
	int                 sidebar_width;
	gboolean            sidebar_visible;
	guint               layout_type : 2;
	gboolean            image_pane_visible;
	gboolean            image_preview_visible;

	/* bookmarks & history */

	int                 bookmarks_length;
	Bookmarks          *history;
	GList              *history_current;
	int                 history_length;
	WindowGoOp          go_op;

	/* browser stuff */

	GthFileList        *file_list;
	DirList            *dir_list;
	CatalogList        *catalog_list;
	char               *catalog_path;       /* The catalog file we are 
						 * showing in the file list. */
	char               *image_path;         /* The image file we are 
						 * showing in the image 
						 * viewer. */
	time_t              image_mtime;        /* Modification time of loaded
						 * image, used to reload the 
						 * image only when needed.*/
	char               *image_catalog;      /* The catalog the current 
						 * image belongs to, NULL if 
						 * the image is not from a 
						 * catalog. */

	int                 freeze_toggle_handler;
	gfloat              dir_load_progress;
	int                 activity_ref;       /* when > 0 some activity
						 * is present. */
	gboolean            image_modified;
	gboolean            setting_file_list;
	gboolean            changing_directory;
	gboolean            refreshing;         /* true if we are refreshing
						 * the file list.  Used to 
						 * handle the refreshing case 
						 * in a special way. */

	guint               activity_timeout;   /* activity timeout handle. */
	guint               load_dir_timeout;
	guint               sel_change_timeout;
	guint               busy_cursor_timeout;

	GThumbPreloader    *preloader;

	/* viewer stuff */

	gboolean            fullscreen;         /* whether the fullscreen mode
						 * is active. */
	guint               view_image_timeout; /* timer for the 
						 * view_image_at_pos function.
						 */
	guint               slideshow_timeout;  /* slideshow timer. */
	gboolean            slideshow;          /* whether the slideshow is 
						 * active. */
	gboolean            slideshow_only_selected;

	/* monitor stuff */

	GnomeVFSMonitorHandle *monitor_handle;
	guint                  monitor_enabled : 1;
	guint                  update_changes_timeout;
	GList                 *monitor_events[MONITOR_EVENT_NUM]; /* char * lists */

	/* misc */

	guint                  cnxn_id[GCONF_NOTIFICATIONS];
	GthPixbufOp           *pixop;
	
	GladeXML              *progress_gui;
	GtkWidget             *progress_dialog;
	GtkWidget             *progress_progressbar;
	GtkWidget             *progress_info;
	guint                  progress_timeout;
} GThumbWindow;


GThumbWindow *  window_new                          (void);

void            window_close                        (GThumbWindow *window);

void            window_set_sidebar_content          (GThumbWindow *window,
						     gint sidebar_content); 

void            window_hide_sidebar                 (GThumbWindow *window);

void            window_show_sidebar                 (GThumbWindow *window);

void            window_hide_image_pane              (GThumbWindow *window);

void            window_show_image_pane              (GThumbWindow *window);

void            window_stop_loading                 (GThumbWindow *window);

void            window_refresh                      (GThumbWindow *window);

void            window_go_to_directory              (GThumbWindow *window,
						     const gchar *dir_path);

void            window_go_to_catalog_directory      (GThumbWindow *window,
						     const gchar *dir_path);

void            window_go_to_catalog                (GThumbWindow *window,
						     const gchar *catalog_path);

void            window_go_up                        (GThumbWindow *window);

void            window_go_back                      (GThumbWindow *window);

void            window_go_forward                   (GThumbWindow *window);

void            window_delete_history               (GThumbWindow *window);

gboolean        window_show_next_image              (GThumbWindow *window,
						     gboolean      only_selected);

gboolean        window_show_prev_image              (GThumbWindow *window,
						     gboolean      only_selected);

gboolean        window_show_first_image             (GThumbWindow *window,
						     gboolean      only_selected);

gboolean        window_show_last_image              (GThumbWindow *window,
						     gboolean      only_selected);

void            window_load_image                   (GThumbWindow *window, 
						     const gchar *filename);

void            window_reload_image                 (GThumbWindow *window);

void            window_start_slideshow              (GThumbWindow *window);

void            window_stop_slideshow               (GThumbWindow *window);

void            window_show_image_prop              (GThumbWindow *window);

void            window_image_modified               (GThumbWindow *window,
						     gboolean      modified);

/* functions used to notify a change. */

void            window_notify_files_created         (GThumbWindow *window,
						     GList *list);

void            window_notify_files_deleted         (GThumbWindow *window,
						     GList *list);

void            window_notify_files_changed         (GThumbWindow *window,
						     GList *list);

void            window_notify_cat_files_added       (GThumbWindow *window,
						     const char   *catalog_name,
						     GList        *list);

void            window_notify_cat_files_deleted     (GThumbWindow *window,
						     const char   *catalog_name,
						     GList        *list);

void            window_notify_file_rename           (GThumbWindow *window,
						     const gchar *old_name,
						     const gchar *new_name);

void            window_notify_directory_rename      (GThumbWindow *window,
						     const gchar *old_name,
						     const gchar *new_name);

void            window_notify_directory_delete      (GThumbWindow *window,
						     const gchar  *path);

void            window_notify_directory_new         (GThumbWindow *window, 
						     const gchar  *path);

void            window_notify_catalog_rename        (GThumbWindow *window,
						     const gchar *old_name,
						     const gchar *new_name);

void            window_notify_catalog_new           (GThumbWindow *window,
						     const gchar  *path);

void            window_notify_catalog_delete        (GThumbWindow *window,
						     const gchar  *path);

void            window_notify_update_comment        (GThumbWindow *window,
						     const gchar *filename);

void            window_notify_update_directory      (GThumbWindow *window,
						     const gchar *dir_path);

void            window_notify_update_layout         (GThumbWindow *window);

void            window_notify_update_toolbar_style  (GThumbWindow *window);

void            window_notify_update_icon_theme     (GThumbWindow *window);

void            window_update_file_list             (GThumbWindow *window);

void            window_update_catalog_list          (GThumbWindow *window);

void            window_update_bookmark_list         (GThumbWindow *window);

void            window_add_monitor                  (GThumbWindow *window);

void            window_remove_monitor               (GThumbWindow *window);

void            window_sync_menu_with_preferences   (GThumbWindow *window);

void            window_exec_pixbuf_op               (GThumbWindow *window,
						     GthPixbufOp  *pixop);

#endif /*  GTHUMB_WINDOW_H */
