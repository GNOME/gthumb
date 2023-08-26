/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2005-2009 Free Software Foundation, Inc.
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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GTH_BROWSER_H
#define GTH_BROWSER_H

#include "gth-file-source.h"
#include "gth-file-store.h"
#include "gth-icon-cache.h"
#include "gth-image.h"
#include "gth-menu-manager.h"
#include "gth-task.h"
#include "gth-viewer-page.h"
#include "gth-window.h"
#include "typedefs.h"

G_BEGIN_DECLS

#define GTH_BROWSER_MENU_MANAGER_GEARS				"gears"
#define GTH_BROWSER_MENU_MANAGER_GEARS_FOLDER_ACTIONS		"gears.folder-actions"
#define GTH_BROWSER_MENU_MANAGER_GEARS_OTHER_ACTIONS		"gears.other-actions"
#define GTH_BROWSER_MENU_MANAGER_GEARS_APP_ACTIONS		"gears.app-actions"
#define GTH_BROWSER_MENU_MANAGER_FILE				"file"
#define GTH_BROWSER_MENU_MANAGER_FILE_SCREEN_ACTIONS		"file.screen-actions"
#define GTH_BROWSER_MENU_MANAGER_FILE_OPEN_ACTIONS		"file.open-actions"
#define GTH_BROWSER_MENU_MANAGER_FILE_EDIT_ACTIONS		"file.edit-actions"
#define GTH_BROWSER_MENU_MANAGER_FILE_FILE_ACTIONS		"file.file-actions"
#define GTH_BROWSER_MENU_MANAGER_FILE_DELETE_ACTIONS		"file.delete-actions"
#define GTH_BROWSER_MENU_MANAGER_FILE_FOLDER_ACTIONS		"file.folder-actions"
#define GTH_BROWSER_MENU_MANAGER_FILE_OTHER_ACTIONS		"file.other-actions"
#define GTH_BROWSER_MENU_MANAGER_FILE_LIST			"file-list"
#define GTH_BROWSER_MENU_MANAGER_FILE_LIST_SCREEN_ACTIONS	"file-list.screen-actions"
#define GTH_BROWSER_MENU_MANAGER_FILE_LIST_OPEN_ACTIONS		"file-list.open-actions"
#define GTH_BROWSER_MENU_MANAGER_FILE_LIST_EDIT_ACTIONS		"file-list.edit-actions"
#define GTH_BROWSER_MENU_MANAGER_FILE_LIST_FILE_ACTIONS		"file-list.file-actions"
#define GTH_BROWSER_MENU_MANAGER_FILE_LIST_DELETE_ACTIONS	"file-list.delete-actions"
#define GTH_BROWSER_MENU_MANAGER_FILE_LIST_FOLDER_ACTIONS	"file-list.folder-actions"
#define GTH_BROWSER_MENU_MANAGER_FILE_LIST_OTHER_ACTIONS	"file-list.other-actions"
#define GTH_BROWSER_MENU_MANAGER_FOLDER				"folder"
#define GTH_BROWSER_MENU_MANAGER_FOLDER_OPEN_ACTIONS		"folder.open-actions"
#define GTH_BROWSER_MENU_MANAGER_FOLDER_CREATE_ACTIONS		"folder.create-actions"
#define GTH_BROWSER_MENU_MANAGER_FOLDER_EDIT_ACTIONS		"folder.edit-actions"
#define GTH_BROWSER_MENU_MANAGER_FOLDER_FOLDER_ACTIONS		"folder.folder-actions"
#define GTH_BROWSER_MENU_MANAGER_FOLDER_OTHER_ACTIONS		"folder.other-actions"

#define GTH_BROWSER_HEADER_BAR_BIG_MARGIN 12

#define GTH_TYPE_BROWSER              (gth_browser_get_type ())
#define GTH_BROWSER(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_BROWSER, GthBrowser))
#define GTH_BROWSER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_BROWSER_TYPE, GthBrowserClass))
#define GTH_IS_BROWSER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_BROWSER))
#define GTH_IS_BROWSER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_BROWSER))
#define GTH_BROWSER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_BROWSER, GthBrowserClass))

typedef struct _GthBrowser        GthBrowser;
typedef struct _GthBrowserClass   GthBrowserClass;
typedef struct _GthBrowserPrivate GthBrowserPrivate;

typedef void (*GthBrowserCallback) (GthBrowser *, gboolean cancelled, gpointer user_data);

typedef enum { /*< skip >*/
	GTH_BROWSER_PAGE_BROWSER = 0,
	GTH_BROWSER_PAGE_VIEWER,
	GTH_BROWSER_N_PAGES
} GthBrowserPage;

typedef enum { /*< skip >*/
	GTH_BROWSER_HEADER_SECTION_BROWSER_NAVIGATION,
	GTH_BROWSER_HEADER_SECTION_BROWSER_LOCATIONS,
	GTH_BROWSER_HEADER_SECTION_BROWSER_COMMANDS,
	GTH_BROWSER_HEADER_SECTION_BROWSER_TOOLS,
	GTH_BROWSER_HEADER_SECTION_BROWSER_METADATA_TOOLS,
	GTH_BROWSER_HEADER_SECTION_BROWSER_VIEW,
	GTH_BROWSER_HEADER_SECTION_VIEWER_NAVIGATION,
	GTH_BROWSER_HEADER_SECTION_VIEWER_OTHER_VIEW,
	GTH_BROWSER_HEADER_SECTION_VIEWER_VIEW,
	GTH_BROWSER_HEADER_SECTION_VIEWER_ZOOM, /* section shared by viewer and editor */
	GTH_BROWSER_HEADER_SECTION_VIEWER_COMMANDS,
	GTH_BROWSER_HEADER_SECTION_VIEWER_OTHER_COMMANDS,
	GTH_BROWSER_HEADER_SECTION_VIEWER_SIDEBAR,
	GTH_BROWSER_HEADER_SECTION_VIEWER_EDIT,
	GTH_BROWSER_HEADER_SECTION_VIEWER_EDIT_METADATA,
	GTH_BROWSER_HEADER_SECTION_EDITOR_NAVIGATION,
	GTH_BROWSER_HEADER_SECTION_EDITOR_VIEW,
	GTH_BROWSER_HEADER_SECTION_EDITOR_COMMANDS,
	GTH_BROWSER_HEADER_SECTION_EDITOR_APPLY,
	GTH_BROWSER_N_HEADER_SECTIONS
} GthBrowserHeaderSection;

typedef enum {
	GTH_ACTION_GO_TO,
	GTH_ACTION_GO_BACK,
	GTH_ACTION_GO_FORWARD,
	GTH_ACTION_GO_UP,
	GTH_ACTION_TREE_LIST_CHILDREN,
	GTH_ACTION_TREE_OPEN
} GthAction;

typedef enum {
	GTH_SCROLL_ACTION_NONE,
	GTH_SCROLL_ACTION_CHANGE_FILE,
	GTH_SCROLL_ACTION_ZOOM
} GthScrollAction;

struct _GthBrowser
{
	GthWindow __parent;
	GthBrowserPrivate *priv;
};

struct _GthBrowserClass
{
	GthWindowClass __parent_class;

	void  (*location_ready)  (GthBrowser *browser,
				  GFile      *location,
				  gboolean    error);
};

GType            gth_browser_get_type               (void);
GtkWidget *      gth_browser_new                    (GFile            *location,
						     GFile            *file_to_select);
GFile *          gth_browser_get_location           (GthBrowser       *browser);
GthFileData *    gth_browser_get_location_data      (GthBrowser       *browser);
GthFileData *    gth_browser_get_current_file       (GthBrowser       *browser);
gboolean         gth_browser_get_file_modified      (GthBrowser       *browser);
void             gth_browser_go_to                  (GthBrowser       *browser,
						     GFile            *location,
						     GFile            *file_to_select);
void             gth_browser_go_back                (GthBrowser       *browser,
						     int               steps);
void             gth_browser_go_forward             (GthBrowser       *browser,
						     int               steps);
void             gth_browser_go_to_history_pos      (GthBrowser       *browser,
						     int               pos);
void             gth_browser_go_up                  (GthBrowser       *browser,
						     int               steps);
void             gth_browser_go_home                (GthBrowser       *browser);
void             gth_browser_clear_history          (GthBrowser       *browser);
void             gth_browser_set_dialog             (GthBrowser       *browser,
						     const char       *dialog_name,
						     GtkWidget        *dialog);
GtkWidget *      gth_browser_get_dialog             (GthBrowser       *browser,
						     const char       *dialog_name);
GthIconCache *   gth_browser_get_menu_icon_cache    (GthBrowser       *browser);
GtkWidget *      gth_browser_get_infobar            (GthBrowser       *browser);
GtkWidget *      gth_browser_get_statusbar          (GthBrowser       *browser);
GtkWidget *      gth_browser_get_filterbar          (GthBrowser       *browser);
GtkWidget *      gth_browser_get_headerbar_section  (GthBrowser			*browser,
						     GthBrowserHeaderSection	 section);
GtkWidget *	 gth_browser_add_header_bar_button  (GthBrowser			*browser,
						     GthBrowserHeaderSection	 section,
						     const char			*icon_name,
						     const char			*tooltip,
						     const char 		*action_name,
						     const char			*accelerator);
GtkWidget *	 gth_browser_add_header_bar_toggle_button
						    (GthBrowser			*browser,
						     GthBrowserHeaderSection	 section,
						     const char			*icon_name,
						     const char			*tooltip,
						     const char 		*action_name,
						     const char			*accelerator);
GtkWidget *	 gth_browser_add_header_bar_label_button
						    (GthBrowser			*browser,
						     GthBrowserHeaderSection	 section,
						     const char			*label,
						     const char			*tooltip,
						     const char			*action_name,
						     const char			*accelerator);
GtkWidget *      gth_browser_add_header_bar_menu_button
						    (GthBrowser			*browser,
						     GthBrowserHeaderSection	 section,
						     const char      	        *icon_name,
						     const char                 *tooltip,
						     GtkWidget                  *popover);
void		 gth_browser_add_menu_manager_for_menu
						    (GthBrowser			*browser,
						     const char			*menu_id,
						     GMenu			*menu);
GthMenuManager * gth_browser_get_menu_manager	    (GthBrowser			*browser,
						     const char 		*menu_id);
GtkWidget *      gth_browser_get_file_list          (GthBrowser       *browser);
GtkWidget *      gth_browser_get_file_list_view     (GthBrowser       *browser);
GtkWidget *      gth_browser_get_thumbnail_list     (GthBrowser       *browser);
GtkWidget *      gth_browser_get_thumbnail_list_view(GthBrowser       *browser);
GthFileSource *  gth_browser_get_location_source    (GthBrowser       *browser);
GthFileStore *   gth_browser_get_file_store         (GthBrowser       *browser);
GtkWidget *      gth_browser_get_folder_tree        (GthBrowser       *browser);
void             gth_browser_get_sort_order         (GthBrowser       *browser,
					 	     GthFileDataSort **sort_type,
						     gboolean         *inverse);
void             gth_browser_set_sort_order         (GthBrowser       *browser,
						     GthFileDataSort  *sort_type,
						     gboolean          inverse);
void             gth_browser_get_file_list_info     (GthBrowser       *browser,
						     int              *current_position,
						     int              *n_visibles);
void             gth_browser_stop                   (GthBrowser       *browser);
void             gth_browser_reload                 (GthBrowser       *browser);
void             gth_browser_exec_task              (GthBrowser       *browser,
						     GthTask          *task,
						     GthTaskFlags      flags);
GthTask *        gth_browser_get_foreground_task    (GthBrowser       *browser);
void             gth_browser_set_close_with_task    (GthBrowser       *browser,
						     gboolean          value);
gboolean         gth_browser_get_close_with_task    (GthBrowser       *browser);
GtkWidget *      gth_browser_get_location_bar	    (GthBrowser       *browser);
GtkWidget *      gth_browser_get_list_info_bar	    (GthBrowser       *browser);
gboolean         gth_browser_viewer_button_press_cb (GthBrowser       *browser,
						     GdkEventButton   *event);
gboolean	 gth_browser_viewer_scroll_event_cb (GthBrowser       *browser,
						     GdkEventScroll   *event);
gboolean         gth_browser_viewer_key_press_cb    (GthBrowser       *browser,
						     GdkEventKey      *event);
void             gth_browser_set_viewer_widget      (GthBrowser       *browser,
						     GtkWidget        *widget);
GtkWidget *      gth_browser_get_viewer_widget      (GthBrowser       *browser);
GthViewerPage *  gth_browser_get_viewer_page        (GthBrowser       *browser);
GtkWidget *      gth_browser_get_viewer_sidebar     (GthBrowser       *browser);
gboolean         gth_browser_show_next_image        (GthBrowser       *browser,
						     gboolean          skip_broken,
						     gboolean          only_selected);
gboolean         gth_browser_show_prev_image        (GthBrowser       *browser,
						     gboolean          skip_broken,
						     gboolean          only_selected);
gboolean         gth_browser_show_first_image       (GthBrowser       *browser,
						     gboolean          skip_broken,
						     gboolean          only_selected);
gboolean         gth_browser_show_last_image        (GthBrowser       *browser,
						     gboolean          skip_broken,
						     gboolean          only_selected);
void             gth_browser_load_file              (GthBrowser       *browser,
						     GthFileData      *file_data,
						     gboolean          view);
void             gth_browser_open_clipboard         (GthBrowser       *browser);
void             gth_browser_update_title           (GthBrowser       *browser);
void             gth_browser_update_sensitivity     (GthBrowser       *browser);
void             gth_browser_update_extra_widget    (GthBrowser       *browser);
void             gth_browser_update_statusbar_file_info
						    (GthBrowser       *browser);
void             gth_browser_show_file_properties   (GthBrowser       *browser);
void             gth_browser_show_viewer_tools      (GthBrowser       *browser);
void             gth_browser_toggle_file_properties (GthBrowser       *browser);
void             gth_browser_toggle_viewer_tools    (GthBrowser       *browser);
void             gth_browser_hide_sidebar           (GthBrowser       *browser);
void             gth_browser_load_location          (GthBrowser       *browser,
						     GFile            *location);
void             gth_browser_show_filterbar         (GthBrowser       *browser,
						     gboolean          show);
gpointer         gth_browser_get_image_preloader    (GthBrowser       *browser);
void             gth_browser_register_viewer_control
						    (GthBrowser       *browser,
					             GtkWidget        *widget);
void             gth_browser_unregister_viewer_control
						    (GthBrowser       *browser,
					             GtkWidget        *widget);
void             gth_browser_fullscreen             (GthBrowser       *browser);
void             gth_browser_unfullscreen           (GthBrowser       *browser);
gboolean         gth_browser_get_is_fullscreen      (GthBrowser       *browser);
void             gth_browser_file_menu_popup        (GthBrowser       *browser,
						     GdkEventButton   *event);
GthFileData *    gth_browser_get_folder_popup_file_data (GthBrowser   *browser);
void             gth_browser_ask_whether_to_save    (GthBrowser       *browser,
				 	 	     GthBrowserCallback
				 	 	     	     	       callback,
				 	 	     gpointer          user_data);
void             gth_browser_save_state             (GthBrowser       *browser);
gboolean         gth_browser_restore_state          (GthBrowser       *browser);
void		 gth_browser_apply_editor_changes   (GthBrowser	      *browser);
GthICCProfile *  gth_browser_get_monitor_profile    (GthBrowser	      *browser);
GtkWidget *      gth_browser_get_fullscreen_headerbar
						    (GthBrowser	      *browser);
void             gth_browser_keep_mouse_visible     (GthBrowser	      *browser,
						     gboolean          value);
void             gth_browser_show_menu              (GthBrowser	      *browser);
void             gth_browser_show_progress_dialog   (GthBrowser        *browser);

/* utilities */

G_END_DECLS

#endif /* GTH_BROWSER_H */
