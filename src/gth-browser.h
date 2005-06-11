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

#ifndef GTH_BROWSER_H
#define GTH_BROWSER_H

#include "gth-file-list.h"
#include "gth-file-view.h"
#include "catalog-list.h"
#include "dir-list.h"
#include "gth-window.h"
#include "typedefs.h"

#define GTH_TYPE_BROWSER              (gth_browser_get_type ())
#define GTH_BROWSER(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_BROWSER, GthBrowser))
#define GTH_BROWSER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_BROWSER_TYPE, GthBrowserClass))
#define GTH_IS_BROWSER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_BROWSER))
#define GTH_IS_BROWSER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_BROWSER))
#define GTH_BROWSER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_BROWSER, GthBrowserClass))

typedef struct _GthBrowser            GthBrowser;
typedef struct _GthBrowserClass       GthBrowserClass;
typedef struct _GthBrowserPrivateData GthBrowserPrivateData;

struct _GthBrowser
{
	GthWindow __parent;
	GthBrowserPrivateData *priv;
};

struct _GthBrowserClass
{
	GthWindowClass __parent_class;
};

GType           gth_browser_get_type                     (void);
GtkWidget *     gth_browser_new                          (const char        *uri);
void            gth_browser_set_sidebar_content          (GthBrowser        *browser,
							  gint               sidebar_content);
GthSidebarContent  gth_browser_get_sidebar_content       (GthBrowser        *browser);
void            gth_browser_hide_sidebar                 (GthBrowser        *browser);
void            gth_browser_show_sidebar                 (GthBrowser        *browser);
void            gth_browser_hide_image_pane              (GthBrowser        *browser);
void            gth_browser_show_image_pane              (GthBrowser        *browser);
void            gth_browser_hide_image_data              (GthBrowser        *browser);
void            gth_browser_show_image_data              (GthBrowser        *browser);
void            gth_browser_set_preview_content          (GthBrowser        *browser,
							  GthPreviewContent  content);
void            gth_browser_stop_loading                 (GthBrowser        *browser);
void            gth_browser_refresh                      (GthBrowser        *browser);
void            gth_browser_go_to_directory              (GthBrowser        *browser,
							  const gchar       *dir_path);
const char *    gth_browser_get_current_directory        (GthBrowser        *browser);
void            gth_browser_go_to_catalog_directory      (GthBrowser        *browser,
							  const gchar       *dir_path);
void            gth_browser_go_to_catalog                (GthBrowser        *browser,
							  const gchar       *catalog_path);
const char *    gth_browser_get_current_catalog          (GthBrowser        *browser);
void            gth_browser_go_up                        (GthBrowser        *browser);
void            gth_browser_go_back                      (GthBrowser        *browser);
void            gth_browser_go_forward                   (GthBrowser        *browser);
void            gth_browser_delete_history               (GthBrowser        *browser);
gboolean        gth_browser_show_next_image              (GthBrowser        *browser,
							  gboolean           only_selected);
gboolean        gth_browser_show_prev_image              (GthBrowser        *browser,
							  gboolean           only_selected);
gboolean        gth_browser_show_first_image             (GthBrowser        *browser,
							  gboolean           only_selected);
gboolean        gth_browser_show_last_image              (GthBrowser        *browser,
							  gboolean           only_selected);
void            gth_browser_load_image                   (GthBrowser        *browser, 
							  const gchar       *filename);
void            gth_browser_reload_image                 (GthBrowser        *browser);
void            gth_browser_show_image_prop              (GthBrowser        *browser);
void            gth_browser_set_image_prop_dlg           (GthBrowser        *browser,
							  GtkWidget         *dialog);
void            gth_browser_show_comment_dlg             (GthBrowser        *browser);
void            gth_browser_show_categories_dlg          (GthBrowser        *browser);

GthFileList *   gth_browser_get_file_list                (GthBrowser        *browser);
GthFileView *   gth_browser_get_file_view                (GthBrowser        *browser);
DirList *       gth_browser_get_dir_list                 (GthBrowser        *browser);
CatalogList *   gth_browser_get_catalog_list             (GthBrowser        *browser);

/* functions used to notify a change. */

void            gth_browser_notify_update_comment        (GthBrowser        *browser,
							  const gchar       *filename);
void            gth_browser_notify_update_directory      (GthBrowser        *browser,
							  const gchar       *dir_path);
void            gth_browser_notify_update_layout         (GthBrowser        *browser);
void            gth_browser_notify_update_toolbar_style  (GthBrowser        *browser);
void            gth_browser_notify_update_icon_theme     (GthBrowser        *browser);
void            gth_browser_update_file_list             (GthBrowser        *browser);
void            gth_browser_update_catalog_list          (GthBrowser        *browser);
void            gth_browser_update_bookmark_list         (GthBrowser        *browser);

/* non-modal dialogs */

void            gth_browser_set_bookmarks_dlg            (GthBrowser        *browser,
							  GtkWidget         *dialog);
GtkWidget      *gth_browser_get_bookmarks_dlg            (GthBrowser        *browser);

#endif /* GTH_BROWSER_H */
