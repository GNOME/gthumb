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
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 */

#ifndef GTH_BROWSER_H
#define GTH_BROWSER_H

#include "gth-file-source.h"
#include "gth-file-store.h"
#include "gth-task.h"
#include "gth-window.h"
#include "typedefs.h"

G_BEGIN_DECLS

#define GTH_TYPE_BROWSER              (gth_browser_get_type ())
#define GTH_BROWSER(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_BROWSER, GthBrowser))
#define GTH_BROWSER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_BROWSER_TYPE, GthBrowserClass))
#define GTH_IS_BROWSER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_BROWSER))
#define GTH_IS_BROWSER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_BROWSER))
#define GTH_BROWSER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_BROWSER, GthBrowserClass))

typedef struct _GthBrowser            GthBrowser;
typedef struct _GthBrowserClass       GthBrowserClass;
typedef struct _GthBrowserPrivateData GthBrowserPrivateData;

typedef enum { /*< skip >*/
	GTH_BROWSER_PAGE_BROWSER = 0,
	GTH_BROWSER_PAGE_VIEWER,
	GTH_BROWSER_PAGE_EDITOR,
	GTH_BROWSER_N_PAGES
} GthBrowserPage;

struct _GthBrowser
{
	GthWindow __parent;
	GthBrowserPrivateData *priv;
};

struct _GthBrowserClass
{
	GthWindowClass __parent_class;

	void  (*location_ready)  (GthBrowser *browser,
				  GFile      *location,
				  gboolean    error);
};

GType            gth_browser_get_type               (void);
GtkWidget *      gth_browser_new                    (const char       *uri);
GFile *          gth_browser_get_location           (GthBrowser       *browser);
GthFileData *    gth_browser_get_current_file       (GthBrowser       *browser);
gboolean         gth_browser_get_file_modified      (GthBrowser       *browser);
void             gth_browser_go_to                  (GthBrowser       *browser,
						     GFile            *location);
void             gth_browser_go_back                (GthBrowser       *browser,
						     int               steps);
void             gth_browser_go_forward             (GthBrowser       *browser,
						     int               steps);
void             gth_browser_go_up                  (GthBrowser       *browser,
						     int               steps);
void             gth_browser_go_home                (GthBrowser       *browser);
void             gth_browser_clear_history          (GthBrowser       *browser);
void             gth_browser_enable_thumbnails      (GthBrowser       *browser,
						     gboolean          enable);
void             gth_browser_set_dialog             (GthBrowser       *browser,
						     const char       *dialog_name,
						     GtkWidget        *dialog);
GtkWidget *      gth_browser_get_dialog             (GthBrowser       *browser,
						     const char       *dialog_name);
GtkUIManager *   gth_browser_get_ui_manager         (GthBrowser       *browser);
GtkWidget *      gth_browser_get_statusbar          (GthBrowser       *browser);
GtkWidget *      gth_browser_get_file_list          (GthBrowser       *browser);
GtkWidget *      gth_browser_get_file_list_view     (GthBrowser       *browser);
GthFileSource *  gth_browser_get_file_source        (GthBrowser       *browser);
GthFileStore *   gth_browser_get_file_store         (GthBrowser       *browser);
GtkWidget *      gth_browser_get_folder_tree        (GthBrowser       *browser);
void             gth_browser_get_sort_order         (GthBrowser       *browser,
					 	     GthFileDataSort **sort_type,
						     gboolean         *inverse);
void             gth_browser_set_sort_order         (GthBrowser       *browser,
						     GthFileDataSort  *sort_type,
						     gboolean          inverse);
void             gth_browser_stop                   (GthBrowser       *browser);
void             gth_browser_reload                 (GthBrowser       *browser);
void             gth_browser_exec_task              (GthBrowser       *browser,
						     GthTask          *task);
void             gth_browser_set_list_extra_widget  (GthBrowser       *browser,
						     GtkWidget        *widget);
GtkWidget *      gth_browser_get_list_extra_widget  (GthBrowser       *browser);
void             gth_browser_set_current_page       (GthBrowser       *browser,
						     GthBrowserPage    page);
GthBrowserPage 	 gth_browser_get_current_page       (GthBrowser       *browser);
void             gth_browser_set_viewer_widget      (GthBrowser       *browser,
						     GtkWidget        *widget);
GtkWidget *      gth_browser_get_viewer_widget      (GthBrowser       *browser);
GtkWidget *      gth_browser_get_viewer_page        (GthBrowser       *browser);
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
void             gth_browser_update_title           (GthBrowser       *browser);
void             gth_browser_update_sensitivity     (GthBrowser       *browser);
void             gth_browser_show_viewer_properties (GthBrowser       *browser,
						     gboolean          show);
void             gth_browser_show_viewer_tools      (GthBrowser       *browser,
						     gboolean          show);
void             gth_browser_load_location          (GthBrowser       *browser,
						     GFile            *location);
void             gth_browser_enable_thumbnails      (GthBrowser       *browser,
						     gboolean          enable);
void             gth_browser_show_filterbar         (GthBrowser       *browser,
						     gboolean          show);
gpointer         gth_browser_get_image_preloader    (GthBrowser       *browser);
void             gth_browser_fullscreen             (GthBrowser       *browser);

G_END_DECLS

#endif /* GTH_BROWSER_H */
