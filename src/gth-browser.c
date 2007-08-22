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

#include <string.h>

#include <gdk/gdkkeysyms.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtk.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <libgnomevfs/gnome-vfs-async-ops.h>
#include <libgnomevfs/gnome-vfs-result.h>
#include <libgnomevfs/gnome-vfs-ops.h>
#include <libgnomevfs/gnome-vfs-mime.h>
#include <glade/glade.h>

#include "bookmarks.h"
#include "catalog.h"
#include "catalog-list.h"
#include "comments.h"
#include "dlg-bookmarks.h"
#include "dlg-categories.h"
#include "dlg-comment.h"
#include "dlg-file-utils.h"
#include "dlg-image-prop.h"
#include "dlg-save-image.h"
#include "file-utils.h"
#include "file-data.h"
#include "gconf-utils.h"
#include "glib-utils.h"
#include "gth-browser.h"
#include "gth-browser-ui.h"
#include "gth-dir-list.h"
#include "gth-exif-data-viewer.h"
#include "gth-exif-utils.h"
#include "gth-file-list.h"
#include "gth-file-view.h"
#include "gth-filter-bar.h"
#include "gth-fullscreen.h"
#include "gth-location.h"
#include "gth-nav-window.h"
#include "gth-pixbuf-op.h"
#include "gthumb-info-bar.h"
#include "gthumb-preloader.h"
#include "gthumb-stock.h"
#include "gtk-utils.h"
#include "image-viewer.h"
#include "main.h"
#include "pixbuf-utils.h"
#include "thumb-cache.h"
#include "rotation-utils.h"

#include <libexif/exif-data.h>
#include "jpegutils/jpeg-data.h"

#ifdef HAVE_LIBIPTCDATA
#include <libiptcdata/iptc-data.h>
#include <libiptcdata/iptc-jpeg.h>
#endif /* HAVE_LIBIPTCDATA */

#include "icons/pixbufs.h"

#define GCONF_NOTIFICATIONS 20

#define VIEW_AS_DELAY 500


typedef enum {
	GTH_BROWSER_GO_TO,
	GTH_BROWSER_GO_BACK,
	GTH_BROWSER_GO_FORWARD
} WindowGoOp;

struct _GthBrowserPrivateData {
	GtkUIManager       *ui;
	GtkActionGroup     *actions;
	GtkActionGroup     *bookmark_actions;
	GtkActionGroup     *history_actions;
	guint               sidebar_merge_id;
	guint               toolbar_merge_id;
	guint               bookmarks_merge_id;
	guint               history_merge_id;

	GtkToolItem        *rotate_tool_item;
	GtkToolItem        *sep_rotate_tool_item;

	GtkWidget          *toolbar;
	GtkWidget          *statusbar;

	GtkWidget          *viewer;

	GtkWidget          *main_pane;
	GtkWidget          *content_pane;
	GtkWidget          *image_pane;
	GtkWidget          *dir_list_pane;
	GtkWidget          *file_list_pane;
	GtkWidget          *notebook;
	GtkWidget          *location;
	GtkWidget          *show_folders_toolbar_button;
	GtkWidget          *show_catalog_toolbar_button;

	GtkWidget          *file_popup_menu;
	GtkWidget          *image_popup_menu;
	GtkWidget          *catalog_popup_menu;
	GtkWidget          *library_popup_menu;
	GtkWidget          *dir_popup_menu;
	GtkWidget          *dir_list_popup_menu;
	GtkWidget          *catalog_list_popup_menu;
	GtkWidget          *history_list_popup_menu;

	GtkWidget          *image_comment;
	GtkWidget          *exif_data_viewer;

	GtkWidget          *progress;              /* statusbar widgets. */
	GtkWidget          *image_info;
	GtkWidget          *image_info_frame;
	GtkWidget          *zoom_info;
	GtkWidget          *zoom_info_frame;

	GtkWidget          *image_prop_dlg;        /* no-modal dialogs. */
	GtkWidget          *bookmarks_dlg;

	GtkWidget          *info_frame;
	GtkWidget          *info_bar;
	GtkWidget          *info_combo;
	GtkWidget          *info_icon;

	char                sidebar_content;       /* SidebarContent values. */
	int                 sidebar_width;
	gboolean            sidebar_visible;
	guint               layout_type : 2;

	gboolean            image_pane_visible;

	GtkWidget          *image_main_pane;

	GtkWidget          *preview_widget_image;
	GtkWidget          *preview_widget_data_comment;
	GtkWidget          *preview_widget_data;
	GtkWidget          *preview_widget_comment;

	GtkWidget          *preview_button_image;
	GtkWidget          *preview_button_data;
	GtkWidget          *preview_button_comment;

	gboolean            preview_visible;
	GthPreviewContent   preview_content;
	gboolean            image_data_visible;

	GtkWidget          *fullscreen;

	/* bookmarks & history */

	int                 bookmarks_length;
	Bookmarks          *history;
	GList              *history_current;
	int                 history_length;
	WindowGoOp          go_op;

	/* browser stuff */

	FileData           *image;
	FileData           *new_image;          /* The image to load after
						 * asking whether to save
						 * the current image. */

	GtkWidget          *filterbar;
	GthFileList        *file_list;
	GthDirList         *dir_list;
	CatalogList        *catalog_list;
	char               *catalog_path;       /* The catalog file we are
						 * showing in the file list. */
	char               *image_path_saved;   /* The filename of the saved
						 * image. */
	char               *image_catalog;      /* The catalog the current
						 * image belongs to, NULL if
						 * the image is not from a
						 * catalog. */
	gboolean            image_error;        /* An error occurred loading
						 * the current image. */

	int                 image_position;

	gfloat              dir_load_progress;
	int                 activity_ref;       /* when > 0 some activity
						 * is present. */

	char               *initial_location;

	gboolean            image_modified;
	gboolean            loading_image;
	ImageSavedFunc      image_saved_func;
	gboolean            refreshing;         /* true if we are refreshing
						 * the file list.  Used to
						 * handle the refreshing case
						 * in a special way. */
	gboolean            saving_modified_image;
	gboolean            load_image_folder_after_image;
	gboolean            focus_current_image;
	gboolean            show_hidden_files;
	gboolean            fast_file_type;
	
	guint               activity_timeout;   /* activity timeout handle. */
	guint               load_dir_timeout;
	guint               sel_change_timeout;
	guint               auto_load_timeout;
	guint               update_layout_timeout;

	gboolean            busy_cursor_active;
	gboolean            closing;

	GThumbPreloader    *preloader;

	GthSortMethod       sort_method;
	GtkSortType         sort_type;

	GthViewAs   	    new_view_type;
	guint               view_as_timeout;

	/* viewer stuff */

	guint               view_image_timeout; /* timer for the
						 * view_image_at_pos function.
						 */
	ExifData           *exif_data;

#ifdef HAVE_LIBIPTCDATA
	IptcData           *iptc_data;
#endif /* HAVE_LIBIPTCDATA */

	/* misc */

	char               *monitor_uri;

	guint               cnxn_id[GCONF_NOTIFICATIONS];
	GthPixbufOp        *pixop;
	gboolean            pixop_preview;

	GladeXML           *progress_gui;
	GtkWidget          *progress_dialog;
	GtkWidget          *progress_progressbar;
	GtkWidget          *progress_info;
	guint               progress_timeout;

	GtkTooltips        *tooltips;
	guint               help_message_cid;
	guint               list_info_cid;

	gboolean            first_time_show;
};

static GthWindowClass *parent_class = NULL;


#define ACTIVITY_DELAY         200
#define BUSY_CURSOR_DELAY      200
#define DISPLAY_PROGRESS_DELAY 750
#define HIDE_SIDEBAR_DELAY     150
#define LOAD_DIR_DELAY         75
#define PANE_MIN_SIZE          60
#define SEL_CHANGED_DELAY      150
#define AUTO_LOAD_DELAY        750
#define UPDATE_LAYOUT_DELAY    250
#define PROGRESS_BAR_WIDTH     60
#define PROGRESS_BAR_HEIGHT    10

#define DEF_WIN_WIDTH          690
#define DEF_WIN_HEIGHT         460
#define DEFAULT_COMMENT_PANE_SIZE 100
#define DEF_SIDEBAR_SIZE       255
#define DEF_SIDEBAR_CONT_SIZE  190
#define PRELOADED_IMAGE_MAX_DIM1 (3000*3000)
#define PRELOADED_IMAGE_MAX_DIM2 (1500*1500)

#define ROTATE_TOOLITEM_POS    11

#define GLADE_EXPORTER_FILE    "gthumb_png_exporter.glade"
#define HISTORY_LIST_MENU      "/MenuBar/Go/HistoryList"
#define HISTORY_LIST_POPUP     "/HistoryListPopup"

enum {
	NAME_COLUMN,
	VALUE_COLUMN,
	POS_COLUMN,
	NUM_COLUMNS
};

enum {
	TARGET_PLAIN,
	TARGET_PLAIN_UTF8,
	TARGET_URILIST,
};


static GtkTargetEntry target_table[] = {
	{ "text/uri-list", 0, TARGET_URILIST },
	/*
	  { "text/plain;charset=UTF-8",    0, TARGET_PLAIN_UTF8 },
	  { "text/plain",    0, TARGET_PLAIN }
	*/
};

/* FIXME
   static GtkTargetEntry same_app_target_table[] = {
   { "text/uri-list", GTK_TARGET_SAME_APP, TARGET_URILIST },
   { "text/plain",    GTK_TARGET_SAME_APP, TARGET_PLAIN }
   };
*/


static GtkTreePath  *dir_list_tree_path = NULL;
static GtkTreePath  *catalog_list_tree_path = NULL;
static GList        *gth_file_list_drag_data = NULL;
static char         *dir_list_drag_data = NULL;


static void
set_action_sensitive (GthBrowser  *browser,
		      const char  *action_name,
		      gboolean     sensitive)
{
	GtkAction *action;

	action = gtk_action_group_get_action (browser->priv->actions, action_name);
	g_object_set (action, "sensitive", sensitive, NULL);
}


static void
set_action_active (GthBrowser  *browser,
		   char        *action_name,
		   gboolean     is_active)
{
	GtkAction *action;

	action = gtk_action_group_get_action (browser->priv->actions, action_name);
	gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), is_active);
}


static void
set_action_active_if_different (GthBrowser  *browser,
				char        *action_name,
				gboolean     is_active)
{
	GtkAction       *action;
	GtkToggleAction *toggle_action;

	action = gtk_action_group_get_action (browser->priv->actions, action_name);
	toggle_action = GTK_TOGGLE_ACTION (action);

	if (gtk_toggle_action_get_active (toggle_action) != is_active)
		gtk_toggle_action_set_active (toggle_action, is_active);
}


static void
set_action_important (GthBrowser  *browser,
		      char        *action_name,
		      gboolean     is_important)
{
	GtkAction *action;

	action = gtk_action_group_get_action (browser->priv->actions, action_name); /*gtk_ui_manager_get_action (browser->priv->ui, action_name);*/
	if (action != NULL)
		g_object_set (action, "is_important", is_important, NULL);
}


static void
window_update_zoom_sensitivity (GthBrowser *browser)
{
	GthBrowserPrivateData *priv = browser->priv;
	gboolean               image_is_visible;
	gboolean               image_is_void;
	int                    zoom;

	image_is_visible = (priv->image != NULL) && ((priv->sidebar_visible && priv->image_pane_visible && priv->preview_content == GTH_PREVIEW_CONTENT_IMAGE) || ! priv->sidebar_visible);
	image_is_void = image_viewer_is_void (IMAGE_VIEWER (priv->viewer));
	zoom = (int) (IMAGE_VIEWER (priv->viewer)->zoom_level * 100.0);

	set_action_sensitive (browser,
			      "View_Zoom100",
			      image_is_visible && !image_is_void && (zoom != 100));
	set_action_sensitive (browser,
			      "View_ZoomIn",
			      image_is_visible && !image_is_void && (zoom != 10000));
	set_action_sensitive (browser,
			      "View_ZoomOut",
			      image_is_visible && !image_is_void && (zoom != 5));
	set_action_sensitive (browser,
			      "View_ZoomFit",
			      image_is_visible && !image_is_void);
	set_action_sensitive (browser,
			      "View_ZoomWidth",
			      image_is_visible && !image_is_void);
}


static void
window_update_statusbar_zoom_info (GthBrowser *browser)
{
	gboolean  image_is_visible;
	int       zoom;
	char     *text;

	window_update_zoom_sensitivity (browser);

	image_is_visible = ((browser->priv->image != NULL) 
			    && ! browser->priv->image_error 
			    && ((browser->priv->sidebar_visible 
			         && browser->priv->image_pane_visible 
			         && browser->priv->preview_content == GTH_PREVIEW_CONTENT_IMAGE) 
			        || ! browser->priv->sidebar_visible));
	if (! image_is_visible) {
		if (! GTK_WIDGET_VISIBLE (browser->priv->zoom_info_frame))
			return;
		gtk_widget_hide (browser->priv->zoom_info_frame);
		return;
	}

	if (! GTK_WIDGET_VISIBLE (browser->priv->zoom_info_frame))
		gtk_widget_show (browser->priv->zoom_info_frame);

	zoom = (int) (IMAGE_VIEWER (browser->priv->viewer)->zoom_level * 100.0);
	text = g_strdup_printf (" %d%% ", zoom);
	gtk_label_set_markup (GTK_LABEL (browser->priv->zoom_info), text);
	g_free (text);
}


static void
window_update_statusbar_image_info (GthBrowser *browser)
{
	GthBrowserPrivateData *priv = browser->priv;
	char                  *text;
	char                   time_txt[50], *utf8_time_txt;
	char                  *size_txt;
	char                  *file_size_txt;
	int                    width, height;
	time_t                 timer = 0;
	struct tm             *tm;

	if ((priv->image == NULL) || priv->image_error) {
		gtk_label_set_text (GTK_LABEL (priv->image_info), "");
		return;
	}

	if (!image_viewer_is_void (IMAGE_VIEWER (priv->viewer))) {
		width = image_viewer_get_image_width (IMAGE_VIEWER (priv->viewer));
		height = image_viewer_get_image_height (IMAGE_VIEWER (priv->viewer));
	} 
	else {
		width = 0;
		height = 0;
	}

	timer = get_metadata_time (NULL, priv->image->path);
	if (timer == 0)
		timer = priv->image->mtime;
	tm = localtime (&timer);
	strftime (time_txt, 50, _("%d %B %Y, %H:%M"), tm);
	utf8_time_txt = g_locale_to_utf8 (time_txt, -1, 0, 0, 0);

	size_txt = g_strdup_printf (_("%d x %d pixels"), width, height);
	file_size_txt = gnome_vfs_format_file_size_for_display (priv->image->size);

	/**/

	if (! priv->image_modified)
		text = g_strdup_printf (" %s - %s - %s     ",
					size_txt,
					file_size_txt,
					utf8_time_txt);
	else
		text = g_strdup_printf (" %s - %s     ",
					_("Modified"),
					size_txt);

	gtk_label_set_markup (GTK_LABEL (priv->image_info), text);

	/**/

	g_free (size_txt);
	g_free (file_size_txt);
	g_free (text);
	g_free (utf8_time_txt);
}


static void
update_image_comment (GthBrowser *browser)
{
	GthBrowserPrivateData *priv = browser->priv;
	CommentData           *cdata;
	char                  *comment = NULL;
	GtkTextBuffer         *text_buffer;

#ifdef HAVE_LIBIPTCDATA
	if (priv->iptc_data != NULL) {
		iptc_data_unref (priv->iptc_data);
		priv->iptc_data = NULL;
	}
#endif /* HAVE_LIBIPTCDATA */

	text_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->image_comment));

	if (priv->image == NULL) {
		GtkTextIter start, end;
		gtk_text_buffer_get_bounds (text_buffer, &start, &end);
		gtk_text_buffer_delete (text_buffer, &start, &end);
		return;
	}

	cdata = comments_load_comment (priv->image->path, TRUE);

#ifdef HAVE_LIBIPTCDATA
	if (cdata != NULL) {
		priv->iptc_data = cdata->iptc_data;
		if (priv->iptc_data != NULL)
			iptc_data_ref (priv->iptc_data);
	}
#endif /* HAVE_LIBIPTCDATA */

	if (comment_text_is_void (cdata)) {
		GtkTextIter  iter;
		const char  *click_here = _("[Press 'c' to add a comment]");
		GtkTextIter  start, end;
		GtkTextTag  *tag;

		gtk_text_buffer_set_text (text_buffer, click_here, strlen (click_here));
		gtk_text_buffer_get_iter_at_line (text_buffer, &iter, 0);
		gtk_text_buffer_place_cursor (text_buffer, &iter);

		gtk_text_buffer_get_bounds (text_buffer, &start, &end);
		tag = gtk_text_buffer_create_tag (text_buffer, NULL,
						  "style", PANGO_STYLE_ITALIC,
						  NULL);
		gtk_text_buffer_apply_tag (text_buffer, tag, &start, &end);

		return;
	}

	comment = comments_get_comment_as_string (cdata, "\n\n", " - ");

	if (comment != NULL) {
		GtkTextIter iter;
		gtk_text_buffer_set_text (text_buffer, comment, strlen (comment));
		gtk_text_buffer_get_iter_at_line (text_buffer, &iter, 0);
		gtk_text_buffer_place_cursor (text_buffer, &iter);
	} else {
		GtkTextIter start, end;
		gtk_text_buffer_get_bounds (text_buffer, &start, &end);
		gtk_text_buffer_delete (text_buffer, &start, &end);
	}

	/**/

	if (cdata->changed)
		gth_file_list_update_comment (priv->file_list, priv->image->path);

	g_free (comment);
	comment_data_free (cdata);
}


static void
window_update_image_info (GthBrowser *browser)
{
	GthBrowserPrivateData *priv = browser->priv;
	JPEGData              *jdata = NULL;
	char                  *cache_uri = NULL;

	window_update_statusbar_image_info (browser);
	window_update_statusbar_zoom_info (browser);
		
	if (priv->exif_data != NULL) {
		exif_data_unref (priv->exif_data);
		priv->exif_data = NULL;
	}

	if (priv->image != NULL)
		cache_uri = get_cache_filename_from_uri (priv->image->path);

	if ((cache_uri != NULL) && (image_is_jpeg (cache_uri))) {
		char *local_file = NULL;
			
		local_file = get_cache_filename_from_uri (priv->image->path);
		jdata = jpeg_data_new_from_file (local_file);
		g_free (local_file);
	}

	g_free (cache_uri);

	if (jdata != NULL) {
		priv->exif_data = jpeg_data_get_exif_data (jdata);
		jpeg_data_unref (jdata);
	}

	gth_exif_data_viewer_update (GTH_EXIF_DATA_VIEWER (browser->priv->exif_data_viewer),
				     IMAGE_VIEWER (browser->priv->viewer),
				     browser->priv->image,
				     browser->priv->exif_data);

	update_image_comment (browser);
}


static void
window_update_infobar (GthBrowser *browser)
{
	GthBrowserPrivateData *priv = browser->priv;
	char       *text;
	char       *escaped_name;
	char       *display_name;
	int         images, current;

	if (priv->image == NULL) {
		gthumb_info_bar_set_text (GTHUMB_INFO_BAR (priv->info_bar), NULL, NULL);
		return;
	}

	images = gth_file_view_get_images (priv->file_list->view);
	current = gth_file_list_pos_from_path (priv->file_list, priv->image->path) + 1;

	display_name = gnome_vfs_unescape_string_for_display (file_name_from_path (priv->image->path));
	escaped_name = g_markup_escape_text (display_name, -1);

	text = g_strdup_printf ("%d/%d - <b>%s</b> %s",
				current,
				images,
				escaped_name,
				priv->image_modified ? _("[modified]") : "");

	gthumb_info_bar_set_text (GTHUMB_INFO_BAR (priv->info_bar), text, NULL);

	g_free (display_name);
	g_free (escaped_name);
	g_free (text);
}


static void
window_update_title (GthBrowser *browser)
{
	GthBrowserPrivateData *priv = browser->priv;
	char *info_txt = NULL;
	char *modified;

	modified = priv->image_modified ? _("[modified]") : "";

	if (priv->image == NULL) {
		if ((priv->sidebar_content == GTH_SIDEBAR_DIR_LIST)
		    && (priv->dir_list->path != NULL)) {			
			char *dir_name = get_uri_display_name (priv->dir_list->path);
			char *display_name = gnome_vfs_unescape_string_for_display (dir_name);
			info_txt = g_strconcat (display_name, " ", modified, NULL);
			g_free (dir_name);
			g_free (display_name);
		} 
		else if ((priv->sidebar_content == GTH_SIDEBAR_CATALOG_LIST)
			   && (priv->catalog_path != NULL)) {
			const char *cat_name;
			char       *cat_name_no_ext;

			cat_name = file_name_from_path (priv->catalog_path);
			cat_name_no_ext = g_strdup (cat_name);

			/* Cut out the file extension. */
			cat_name_no_ext[strlen (cat_name_no_ext) - 4] = 0;

			info_txt = gnome_vfs_unescape_string_for_display (cat_name_no_ext);
			g_free (cat_name_no_ext);
		} 
		else
			info_txt = g_strdup_printf ("%s", _("gThumb"));
	} 
	else {
		char *image_name;
		int   images, current;

		image_name = gnome_vfs_unescape_string_for_display (file_name_from_path (priv->image->path));
		images = gth_file_view_get_images (priv->file_list->view);
		current = gth_file_list_pos_from_path (priv->file_list, priv->image->path) + 1;

		if (priv->image_catalog != NULL) {
			char *cat_name = gnome_vfs_unescape_string_for_display (file_name_from_path (priv->image_catalog));

			/* Cut out the file extension. */
			cat_name[strlen (cat_name) - 4] = 0;

			info_txt = g_strdup_printf ("%s %s (%d/%d) - %s",
						    image_name,
						    modified,
						    current,
						    images,
						    cat_name);
			g_free (cat_name);
		}
		else
			info_txt = g_strdup_printf ("%s %s (%d/%d)",
						    image_name,
						    modified,
						    current,
						    images);

		g_free (image_name);
	}

	gtk_window_set_title (GTK_WINDOW (browser), info_txt);

	g_free (info_txt);
}


static void
window_update_statusbar_list_info (GthBrowser *browser)
{
	GthBrowserPrivateData *priv = browser->priv;
	char                  *info, *size_txt, *sel_size_txt;
	char                  *total_info, *selected_info;
	int                    tot_n, sel_n;
	GnomeVFSFileSize       tot_size, sel_size;
	GList                 *file_list, *scan;
	GList                 *selection;

	tot_n = 0;
	tot_size = 0;

	file_list = gth_file_view_get_list (priv->file_list->view);
	for (scan = file_list; scan; scan = scan->next) {
		FileData *fd = scan->data;
		tot_n++;
		tot_size += fd->size;
	}
	file_data_list_free (file_list);

	sel_n = 0;
	sel_size = 0;
	selection = gth_file_list_get_selection_as_fd (priv->file_list);

	for (scan = selection; scan; scan = scan->next) {
		FileData *fd = scan->data;
		sel_n++;
		sel_size += fd->size;
	}

	file_data_list_free (selection);

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

	gtk_statusbar_pop (GTK_STATUSBAR (priv->statusbar),
			   priv->list_info_cid);
	gtk_statusbar_push (GTK_STATUSBAR (priv->statusbar),
			    priv->list_info_cid, info);

	g_free (total_info);
	g_free (selected_info);
	g_free (size_txt);
	g_free (sel_size_txt);
	g_free (info);
}


static void
window_update_go_sensitivity (GthBrowser *browser)
{
	GthBrowserPrivateData *priv = browser->priv;
	gboolean               sensitive;

	sensitive = (priv->history_current != NULL) && (priv->history_current->next != NULL);
	set_action_sensitive (browser, "Go_Back", sensitive);

	sensitive = (priv->history_current != NULL) && (priv->history_current->prev != NULL);
	set_action_sensitive (browser, "Go_Forward", sensitive);

	set_action_sensitive (browser, "Go_Stop", (priv->activity_ref > 0));
}


static gboolean
window_image_pane_is_visible (GthBrowser *browser)
{
	GthBrowserPrivateData *priv = browser->priv;
	return ((priv->sidebar_visible
		 && priv->image_pane_visible
		 && (priv->preview_content == GTH_PREVIEW_CONTENT_IMAGE))
		|| ! priv->sidebar_visible);
}


static gboolean
at_least_an_image_is_present (GthBrowser *browser)
{
	GList *scan;
	
	for (scan = browser->priv->file_list->list; scan; scan = scan->next) {
		FileData *file = scan->data;
		
		if (mime_type_is_image (file->mime_type))
			return TRUE;
	}
	
	return FALSE;
}


static void
window_update_sensitivity (GthBrowser *browser)
{
	GthBrowserPrivateData *priv = browser->priv;
	GtkTreeIter iter;
	int         sidebar_content = priv->sidebar_content;
	int         n_selected;
	gboolean    sel_not_null;
	gboolean    only_one_is_selected;
	gboolean    image_is_void;
	gboolean    image_is_ani;
	gboolean    playing;
	gboolean    viewing_dir;
	gboolean    viewing_catalog;
	gboolean    is_catalog;
	gboolean    is_search;
	gboolean    image_is_visible;
	gboolean    file_list_contains_images;
	gboolean    image_is_image;
	int         image_pos;

	n_selected = gth_file_view_get_n_selected (priv->file_list->view);
	sel_not_null = n_selected > 0;
	only_one_is_selected = n_selected == 0;
	image_is_void = image_viewer_is_void (IMAGE_VIEWER (priv->viewer));
	image_is_ani = image_viewer_is_animation (IMAGE_VIEWER (priv->viewer));
	playing = image_viewer_is_playing_animation (IMAGE_VIEWER (priv->viewer));
	viewing_dir = sidebar_content == GTH_SIDEBAR_DIR_LIST;
	viewing_catalog = sidebar_content == GTH_SIDEBAR_CATALOG_LIST;
	image_is_visible = ! image_is_void && window_image_pane_is_visible (browser);
	file_list_contains_images = at_least_an_image_is_present (browser); 
	image_is_image = (browser->priv->image != NULL) && mime_type_is_image (browser->priv->image->mime_type);
	
	if (priv->image != NULL)
		image_pos = gth_file_list_pos_from_path (priv->file_list, priv->image->path);
	else
		image_pos = -1;

	window_update_go_sensitivity (browser);

	/* Image popup menu. */

	set_action_sensitive (browser, "Image_OpenWith", ! image_is_void);
	set_action_sensitive (browser, "Image_Rename", ! image_is_void);
	set_action_sensitive (browser, "Image_Delete", ! image_is_void);
	set_action_sensitive (browser, "Image_Copy", ! image_is_void);
	set_action_sensitive (browser, "Image_Move", ! image_is_void);

	/* File menu. */

	set_action_sensitive (browser, "File_ViewImage", ! image_is_void);
	set_action_sensitive (browser, "File_OpenWith", sel_not_null);

	set_action_sensitive (browser, "File_Save", ! image_is_void && priv->image_modified);
	set_action_sensitive (browser, "File_SaveAs", ! image_is_void);
	set_action_sensitive (browser, "File_Revert", ! image_is_void && priv->image_modified);
	set_action_sensitive (browser, "File_Print", ! image_is_void || sel_not_null);

	/* Edit menu. */

	set_action_sensitive (browser, "Edit_Undo", gth_window_get_can_undo (GTH_WINDOW (browser)));
	set_action_sensitive (browser, "Edit_Redo", gth_window_get_can_redo (GTH_WINDOW (browser)));
	set_action_sensitive (browser, "Edit_RenameFile", sel_not_null);
	set_action_sensitive (browser, "Edit_DuplicateFile", sel_not_null);
	set_action_sensitive (browser, "Edit_DeleteFiles", sel_not_null);
	set_action_sensitive (browser, "Edit_CopyFiles", sel_not_null);
	set_action_sensitive (browser, "Edit_MoveFiles", sel_not_null);

	set_action_sensitive (browser, "AlterImage_Rotate90", ! image_is_void && ! image_is_ani && image_is_visible);
	set_action_sensitive (browser, "AlterImage_Rotate90CC", ! image_is_void && ! image_is_ani && image_is_visible);
	set_action_sensitive (browser, "AlterImage_Flip", ! image_is_void && ! image_is_ani && image_is_visible);
	set_action_sensitive (browser, "AlterImage_Mirror", ! image_is_void && ! image_is_ani && image_is_visible);
	set_action_sensitive (browser, "AlterImage_Desaturate", ! image_is_void && ! image_is_ani && image_is_visible);
	set_action_sensitive (browser, "AlterImage_Resize", ! image_is_void && ! image_is_ani && image_is_visible);
	set_action_sensitive (browser, "AlterImage_ColorBalance", ! image_is_void && ! image_is_ani && image_is_visible);
	set_action_sensitive (browser, "AlterImage_HueSaturation", ! image_is_void && ! image_is_ani && image_is_visible);
	set_action_sensitive (browser, "AlterImage_BrightnessContrast", ! image_is_void && ! image_is_ani && image_is_visible);
	set_action_sensitive (browser, "AlterImage_Invert", ! image_is_void && ! image_is_ani && image_is_visible);
	set_action_sensitive (browser, "AlterImage_Posterize", ! image_is_void && ! image_is_ani && image_is_visible);
	set_action_sensitive (browser, "AlterImage_Equalize", ! image_is_void && ! image_is_ani && image_is_visible);
	set_action_sensitive (browser, "AlterImage_AdjustLevels", ! image_is_void && ! image_is_ani && image_is_visible);
	set_action_sensitive (browser, "AlterImage_Crop", ! image_is_void && ! image_is_ani && image_is_visible);
	set_action_sensitive (browser, "AlterImage_Dither_BW", ! image_is_void && ! image_is_ani && image_is_visible);
	set_action_sensitive (browser, "AlterImage_Dither_Web", ! image_is_void && ! image_is_ani && image_is_visible);
	set_action_sensitive (browser, "AlterImage_RedeyeRemoval", ! image_is_void && ! image_is_ani && image_is_visible);

	set_action_sensitive (browser, "View_PlayAnimation", image_is_ani);
	set_action_sensitive (browser, "View_StepAnimation", image_is_ani && ! playing);

	set_action_sensitive (browser, "Edit_EditComment", sel_not_null);
	set_action_sensitive (browser, "ToolBar_EditComment", sel_not_null);
	set_action_sensitive (browser, "Edit_DeleteComment", sel_not_null);
	set_action_sensitive (browser, "Edit_EditCategories", sel_not_null);
	set_action_sensitive (browser, "ToolBar_EditCategories", sel_not_null);

	set_action_sensitive (browser, "Edit_AddToCatalog", sel_not_null);
	set_action_sensitive (browser, "Edit_RemoveFromCatalog", viewing_catalog && sel_not_null);

	set_action_sensitive (browser, "Go_ToContainer", viewing_catalog && only_one_is_selected);

	/* Edit Catalog menu. */

	if (priv->sidebar_content == GTH_SIDEBAR_CATALOG_LIST) {
		is_catalog = (viewing_catalog && catalog_list_get_selected_iter (priv->catalog_list, &iter));

		set_action_sensitive (browser, "EditCatalog_Rename", is_catalog);
		set_action_sensitive (browser, "EditCatalog_Delete", is_catalog);
		set_action_sensitive (browser, "EditCatalog_Move", is_catalog && ! catalog_list_is_dir (priv->catalog_list, &iter));

		is_search = (is_catalog && (catalog_list_is_search (priv->catalog_list, &iter)));
		set_action_sensitive (browser, "EditCatalog_EditSearch", is_search);
		set_action_sensitive (browser, "EditCatalog_RedoSearch", is_search);

		/**/

		is_catalog = (priv->catalog_path != NULL) && (viewing_catalog && catalog_list_get_iter_from_path (priv->catalog_list, priv->catalog_path, &iter));

		set_action_sensitive (browser, "EditCurrentCatalog_Rename", is_catalog);
		set_action_sensitive (browser, "EditCurrentCatalog_Delete", is_catalog);
		set_action_sensitive (browser, "EditCurrentCatalog_Move", is_catalog && ! catalog_list_is_dir (priv->catalog_list, &iter));

		is_search = (is_catalog && (catalog_list_is_search (priv->catalog_list, &iter)));
		set_action_sensitive (browser, "EditCurrentCatalog_EditSearch", is_search);
		set_action_sensitive (browser, "EditCurrentCatalog_RedoSearch", is_search);
	}

	/* View menu. */

	set_action_sensitive (browser, "View_ShowImage", priv->image != NULL);
	set_action_sensitive (browser, "File_ImageProp", priv->image != NULL);
	set_action_sensitive (browser, "View_Fullscreen", (image_is_image && ! image_is_void) || file_list_contains_images);
	set_action_sensitive (browser, "View_ShowPreview", priv->sidebar_visible);
	set_action_sensitive (browser, "View_ShowMetadata", ! priv->sidebar_visible);
	set_action_sensitive (browser, "View_PrevImage", image_pos > 0);
	set_action_sensitive (browser, "View_NextImage", (image_pos != -1) && (image_pos < gth_file_view_get_images (priv->file_list->view) - 1));
	set_action_sensitive (browser, "SortManual", viewing_catalog);

	/* Tools menu. */

	set_action_sensitive (browser, "Tools_Slideshow", (image_is_image && ! image_is_void) || file_list_contains_images);
	set_action_sensitive (browser, "Tools_IndexImage", sel_not_null);
	set_action_sensitive (browser, "Tools_WebExporter", sel_not_null);
	set_action_sensitive (browser, "Tools_ConvertFormat", sel_not_null);
	set_action_sensitive (browser, "Tools_ChangeDate", sel_not_null);
	set_action_sensitive (browser, "Tools_ResetExif", sel_not_null);
	set_action_sensitive (browser, "Tools_JPEGRotate", sel_not_null);
	set_action_sensitive (browser, "Tools_ScaleSeries", sel_not_null);
	set_action_sensitive (browser, "Wallpaper_Centered", ! image_is_void);
	set_action_sensitive (browser, "Wallpaper_Tiled", ! image_is_void);
	set_action_sensitive (browser, "Wallpaper_Scaled", ! image_is_void);
	set_action_sensitive (browser, "Wallpaper_Stretched", ! image_is_void);

	window_update_zoom_sensitivity (browser);
}


static void
window_progress (gfloat   percent,
		 gpointer data)
{
	GthBrowser            *browser = data;
	GthBrowserPrivateData *priv = browser->priv;

	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (priv->progress),
				       percent);

	if (percent == 0.0)
		set_action_sensitive (browser, "Go_Stop",
				       ((priv->activity_ref > 0)
					|| priv->file_list->busy));
}


static gboolean
load_progress (gpointer data)
{
	GthBrowser *browser = data;
	gtk_progress_bar_pulse (GTK_PROGRESS_BAR (browser->priv->progress));
	return TRUE;
}


static void
gth_browser_start_activity_mode (GthBrowser *browser)
{
	GthBrowserPrivateData *priv = browser->priv;

	g_return_if_fail (browser != NULL);

	if (priv->activity_ref++ > 0)
		return;

	gtk_widget_show (priv->progress);

	priv->activity_timeout = g_timeout_add (ACTIVITY_DELAY,
						load_progress,
						browser);
}


static void
gth_browser_stop_activity_mode (GthBrowser *browser)
{
	GthBrowserPrivateData *priv = browser->priv;

	g_return_if_fail (browser != NULL);

	priv->activity_ref = 0;

	gtk_widget_hide (priv->progress);

	if (priv->activity_timeout == 0)
		return;

	g_source_remove (priv->activity_timeout);
	priv->activity_timeout = 0;
	window_progress (0.0, browser);
}


static void
set_cursor_busy (GthBrowser *browser, gboolean track_state)
{
	GdkCursor *cursor;

	/* Track file-list busy states so that the preloader can only 
	   stop a busy cursor if no file-list-triggered busy cursors 
	   are active. */
	if (track_state)
		browser->priv->busy_cursor_active = TRUE;

	cursor = gdk_cursor_new (GDK_WATCH);
	gdk_window_set_cursor (GTK_WIDGET (browser)->window, cursor);
	gdk_cursor_unref (cursor);
}


static void
set_cursor_not_busy (GthBrowser *browser, gboolean track_state)
{
	GdkCursor *cursor;

	if (track_state)
		browser->priv->busy_cursor_active = FALSE;

	if (browser->priv->busy_cursor_active == FALSE) {
        	cursor = gdk_cursor_new (GDK_LEFT_PTR);
	        gdk_window_set_cursor (GTK_WIDGET (browser)->window, cursor);
        	gdk_cursor_unref (cursor);
	}
}


/* -- set file list -- */


static void
window_sync_sort_menu (GthBrowser    *browser,
		       GthSortMethod  sort_method,
		       GtkSortType    sort_type)
{
	char *prop;

	/* Sort type item. */

	switch (sort_method) {
	case GTH_SORT_METHOD_BY_NAME: prop = "SortByName"; break;
	case GTH_SORT_METHOD_BY_PATH: prop = "SortByPath"; break;
	case GTH_SORT_METHOD_BY_SIZE: prop = "SortBySize"; break;
	case GTH_SORT_METHOD_BY_TIME: prop = "SortByTime"; break;
	case GTH_SORT_METHOD_BY_EXIF_DATE: prop = "SortByExifDate"; break;
	case GTH_SORT_METHOD_BY_COMMENT: prop = "SortByComment"; break;
	case GTH_SORT_METHOD_MANUAL:  prop = "SortManual"; break;
	default: prop = "X"; break;
	}
	set_action_active (browser, prop, TRUE);

	/* 'reversed' item. */

	set_action_active (browser, "SortReversed", (sort_type != GTK_SORT_ASCENDING));
}


static void
window_set_file_list (GthBrowser    *browser,
		      GList         *list,
		      GthSortMethod  sort_method,
		      GtkSortType    sort_type)
{
	gboolean remote_folder;
	
	gth_browser_start_activity_mode (browser);
	window_update_sensitivity (browser);
	window_sync_sort_menu (browser, sort_method, sort_type);
	
	remote_folder = ((browser->priv->sidebar_content == GTH_SIDEBAR_DIR_LIST)
			 && ! is_local_file (browser->priv->dir_list->path));
	gth_file_list_ignore_hidden_thumbs (browser->priv->file_list, remote_folder);
	gth_file_list_set_list (browser->priv->file_list,
				list,
				sort_method,
				sort_type);
}


static void
window_update_file_list (GthBrowser *browser)
{
	GthBrowserPrivateData *priv = browser->priv;

	if (priv->sidebar_content == GTH_SIDEBAR_DIR_LIST)
		gth_browser_go_to_directory (browser, priv->dir_list->path);

	else if (priv->sidebar_content == GTH_SIDEBAR_CATALOG_LIST) {
		char *catalog_path = catalog_list_get_selected_path (priv->catalog_list);
		if (catalog_path != NULL) {
			gth_browser_go_to_catalog (browser, catalog_path);
			g_free (catalog_path);
		}
	}
}


static void
window_update_catalog_list (GthBrowser *browser)
{
	GthBrowserPrivateData *priv = browser->priv;

	if (priv->sidebar_content != GTH_SIDEBAR_CATALOG_LIST)
		return;

	/* If the catalog still exists, show the directory it belongs to. */

	if ((priv->catalog_path != NULL) && path_is_file (priv->catalog_path)) {
		GtkTreeIter  iter;
		GtkTreePath *path;
		char        *catalog_dir;

		catalog_dir = remove_level_from_path (priv->catalog_path);
		gth_browser_go_to_catalog_directory (browser, catalog_dir);
		g_free (catalog_dir);

		if (! catalog_list_get_iter_from_path (priv->catalog_list,
						       priv->catalog_path,
						       &iter))
			return;

		/* Select (without updating the file list) and view
		 * the catalog. */

		catalog_list_select_iter (priv->catalog_list, &iter);
		path = gtk_tree_model_get_path (GTK_TREE_MODEL (priv->catalog_list->list_store), &iter);
		gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (priv->catalog_list->list_view),
					      path,
					      NULL,
					      TRUE,
					      0.5,
					      0.0);
		gtk_tree_path_free (path);
		return;
	}

	/* No catalog selected. */

	if (priv->catalog_path != NULL) {
		g_free (priv->catalog_path);
		priv->catalog_path = NULL;

		/* clear the image list. */

		window_set_file_list (browser, NULL, priv->sort_method, priv->sort_type);
	}

	g_return_if_fail (priv->catalog_list->path != NULL);

	/* If directory exists then update. */

	if (path_is_dir (priv->catalog_list->path)) {
		catalog_list_refresh (priv->catalog_list);
		return;
	}

	/* ...else go up one level until a directory exists (done in gth_browser_go_to_catalog_directory). */

	gth_browser_go_to_catalog_directory (browser, priv->catalog_list->path);
}


/* -- bookmarks & history -- */


static void set_mode_specific_ui_info (GthBrowser        *browser,
				       GthSidebarContent  content,
				       gboolean           first_time);


typedef struct {
	GthBrowser *browser;
	char       *uri;
	gboolean    check_if_uri_is_file;
} GoToUriData;


static gboolean
go_to_uri__step_2 (gpointer callback_data)
{
	GoToUriData *data = callback_data;
	GthBrowser  *browser;
	const char  *uri;
	
	browser = data->browser;
	uri = data->uri;

	if (uri_scheme_is_catalog (uri) || uri_scheme_is_search (uri)) {
		char *file_uri = add_scheme_if_absent (remove_host_from_uri (uri));

		if ((file_uri != NULL) && file_extension_is (file_uri, CATALOG_EXT))
			gth_browser_go_to_catalog (browser, file_uri);
		else
			gth_browser_show_catalog_directory (browser, file_uri);

		g_free (file_uri);
	} 
	else if (data->check_if_uri_is_file && path_is_file (uri)) {
		browser->priv->load_image_folder_after_image = TRUE;
		gth_browser_load_image_from_uri (browser, uri);
		gth_browser_hide_sidebar (browser);
	} 
	else
		gth_browser_go_to_directory (browser, uri);
		
	g_free (data->uri);
	g_free (data);
	
	return FALSE;
}


static void
go_to_uri (GthBrowser *browser,
	   const char *uri,
	   gboolean    check_if_uri_is_file)
{
	GoToUriData *data;
	
	if (uri == NULL)
		return;
	
	data = g_new0 (GoToUriData, 1);
	data->browser = browser;
	data->uri = g_strdup (uri);
	data->check_if_uri_is_file = check_if_uri_is_file;
	
	g_idle_add (go_to_uri__step_2, data);
}


static void
activate_action_bookmark (GtkAction *action,
			  gpointer   data)
{
	GthBrowser  *browser = data;
	go_to_uri (browser, g_object_get_data (G_OBJECT (action), "path"), FALSE);
}


static void
add_bookmark_menu_item (GthBrowser     *browser,
			GtkActionGroup *actions,
			guint           merge_id,
			Bookmarks      *bookmarks,
			char           *prefix,
			int             id,
			const char     *base_path,
			const char     *path)
{
	GthBrowserPrivateData *priv = browser->priv;
	GtkAction             *action;
	char                  *name;
	char                  *label;
	char                  *menu_name;
	char                  *e_label;
	char                  *e_tip;
	char                  *utf8_s;

	name = g_strdup_printf ("%s%d", prefix, id);

	menu_name = escape_underscore (bookmarks_get_menu_name (bookmarks, path));
	if (menu_name == NULL)
		menu_name = g_strdup (_("(Invalid Name)"));
	label = _g_strdup_with_max_size (menu_name, BOOKMARKS_MENU_MAX_LENGTH);
	utf8_s = g_locale_to_utf8 (label, -1, NULL, NULL, NULL);
	if (utf8_s == NULL)
		utf8_s = g_strdup (_("(Invalid Name)"));
	e_label = g_markup_escape_text (utf8_s, -1);
	g_free (utf8_s);

	action = g_object_new (GTK_TYPE_ACTION,
			       "name", name,
			       "label", e_label,
			       "stock_id", get_stock_id_for_uri (path),
			       NULL);

	utf8_s = g_locale_to_utf8 (bookmarks_get_menu_tip (bookmarks, path), -1, NULL, NULL, NULL);
	if (utf8_s == NULL)
		utf8_s = g_strdup (_("(Invalid Name)"));
	e_tip = g_markup_escape_text (utf8_s, -1);
	g_free (utf8_s);

	if (e_tip != NULL)
		g_object_set (action, "tooltip", e_tip, NULL);

	g_free (label);
	g_free (menu_name);
	g_free (e_label);
	g_free (e_tip);

	g_object_set_data_full (G_OBJECT (action),
				"path",
				g_strdup (path),
				(GFreeFunc)g_free);
	g_signal_connect (action, "activate",
			  G_CALLBACK (activate_action_bookmark),
			  browser);

	gtk_action_group_add_action (actions, action);
	g_object_unref (action);

	gtk_ui_manager_add_ui (priv->ui,
			       merge_id,
			       base_path,
			       name,
			       name,
			       GTK_UI_MANAGER_AUTO,
			       FALSE);

	g_free (name);
}


static void
window_update_location (GthBrowser *browser)
{
	GthBrowserPrivateData *priv = browser->priv;
	char                  *uri;

	if (priv->history_current == NULL)
		return;

	uri = priv->history_current->data;

	if (uri_scheme_is_catalog (uri)
	    || uri_scheme_is_search (uri)) {
		char *parent_uri = remove_level_from_path (uri);
		gth_location_set_catalog_uri (GTH_LOCATION (priv->location), parent_uri, FALSE);
		g_free (parent_uri);
	} else
		gth_location_set_folder_uri (GTH_LOCATION (priv->location), uri, FALSE);
}


static void
window_update_bookmark_list (GthBrowser *browser)
{
	GthBrowserPrivateData *priv = browser->priv;
	GList                 *scan, *names;
	int                    i;

	/* Delete bookmarks menu. */

	if (priv->bookmarks_merge_id != 0) {
		gtk_ui_manager_remove_ui (priv->ui, priv->bookmarks_merge_id);
		priv->bookmarks_merge_id = 0;
	}

	gtk_ui_manager_ensure_update (priv->ui);

	if (priv->bookmark_actions != NULL) {
		gtk_ui_manager_remove_action_group (priv->ui, priv->bookmark_actions);
		g_object_unref (priv->bookmark_actions);
		priv->bookmark_actions = NULL;
	}

	/* Load and sort bookmarks */

	bookmarks_load_from_disk (preferences.bookmarks);
	if (preferences.bookmarks->list == NULL) {
		gth_location_set_bookmarks (GTH_LOCATION (priv->location), NULL, 0);
		return;
	}

	names = g_list_copy (preferences.bookmarks->list);

	/* Update bookmarks menu. */

	if (priv->bookmark_actions == NULL) {
		priv->bookmark_actions = gtk_action_group_new ("BookmarkActions");
		gtk_ui_manager_insert_action_group (priv->ui, priv->bookmark_actions, 0);
	}

	if (priv->bookmarks_merge_id == 0)
		priv->bookmarks_merge_id = gtk_ui_manager_new_merge_id (priv->ui);

	for (i = 0, scan = names; scan; scan = scan->next, i++)
		add_bookmark_menu_item (browser,
					priv->bookmark_actions,
					priv->bookmarks_merge_id,
					preferences.bookmarks,
					"Bookmark",
					i,
					"/MenuBar/Bookmarks/BookmarkList",
					scan->data);

	priv->bookmarks_length = i;
	gth_location_set_bookmarks (GTH_LOCATION (priv->location), names, i);

	g_list_free (names);

	if (priv->bookmarks_dlg != NULL)
		dlg_edit_bookmarks_update (priv->bookmarks_dlg);
}


static void
window_update_history_list (GthBrowser *browser)
{
	GthBrowserPrivateData *priv = browser->priv;
	GList                 *scan;
	int                    i;

	window_update_go_sensitivity (browser);

	/* Delete history menu. */

	if (priv->history_merge_id != 0) {
		gtk_ui_manager_remove_ui (priv->ui, priv->history_merge_id);
		priv->history_merge_id = 0;
	}

	gtk_ui_manager_ensure_update (priv->ui);

	if (priv->history_actions != NULL) {
		gtk_ui_manager_remove_action_group (priv->ui, priv->history_actions);
		g_object_unref (priv->history_actions);
		priv->history_actions = NULL;
	}

	/* Update history menu. */

	if (priv->history->list == NULL)
		return;

	if (priv->history_actions == NULL) {
		priv->history_actions = gtk_action_group_new ("HistoryActions");
		gtk_ui_manager_insert_action_group (priv->ui, priv->history_actions, 0);
	}

	if (priv->history_merge_id == 0)
		priv->history_merge_id = gtk_ui_manager_new_merge_id (priv->ui);

	for (i = 0, scan = priv->history_current; scan && (i < eel_gconf_get_integer (PREF_MAX_HISTORY_LENGTH, 15)); scan = scan->next) {
		add_bookmark_menu_item (browser,
					priv->history_actions,
					priv->history_merge_id,
					priv->history,
					"History",
					i,
					HISTORY_LIST_MENU,
					scan->data);
		if (i > 0)
			add_bookmark_menu_item (browser,
						priv->history_actions,
						priv->history_merge_id,
						priv->history,
						"History",
						i,
						HISTORY_LIST_POPUP,
						scan->data);
		i++;
	}
	priv->history_length = i;
}


/**/


static void
view_image_at_pos (GthBrowser *browser,
		   int         pos)
{
	FileData *file;

	if ((pos < 0) || (pos >= gth_file_view_get_images (browser->priv->file_list->view)))
		return;

	file = gth_file_view_get_image_data (browser->priv->file_list->view, pos);
	if (file != NULL) {
		gth_browser_load_image (browser, file);
		file_data_unref (file);
	}
}


/* -- activity -- */


static void
image_loader_progress_cb (ImageLoader *loader,
			  float        p,
			  gpointer     data)
{
	GthBrowser *browser = data;
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (browser->priv->progress), p);
}


static void
image_loader_done_cb (ImageLoader *loader,
		      gpointer     data)
{
	GthBrowser *browser = data;
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (browser->priv->progress), 0.0);
}


static char *
get_command_name_from_sidebar_content (GthBrowser *browser)
{
	switch (browser->priv->sidebar_content) {
	case GTH_SIDEBAR_DIR_LIST:
		return "View_ShowFolders";
		break;
	case GTH_SIDEBAR_CATALOG_LIST:
		return "View_ShowCatalogs";
		break;
	default:
		return NULL;
		break;
	}

	return NULL;
}


static void
set_button_active_no_notify (GthBrowser *browser,
			     GtkWidget  *button,
			     gboolean    active)
{
	if (button == NULL)
		return;
	g_signal_handlers_block_by_data (button, browser);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), active);
	g_signal_handlers_unblock_by_data (button, browser);
}


static GtkWidget *
get_button_from_sidebar_content (GthBrowser *browser)
{
	GthBrowserPrivateData *priv = browser->priv;

	switch (priv->sidebar_content) {
	case GTH_SIDEBAR_DIR_LIST:
		return priv->show_folders_toolbar_button;
	case GTH_SIDEBAR_CATALOG_LIST:
		return priv->show_catalog_toolbar_button;
	default:
		return NULL;
	}

	return NULL;
}


static void
window_update_folder_ui (GthBrowser *browser)
{
	GthBrowserPrivateData *priv = browser->priv;
	GError *error = NULL;

	if (priv->sidebar_merge_id != 0) {
		gtk_ui_manager_remove_ui (priv->ui, priv->sidebar_merge_id);
		gtk_ui_manager_ensure_update (priv->ui);
	}

	priv->sidebar_merge_id = gtk_ui_manager_add_ui_from_string (priv->ui, folder_ui_info, -1, &error);
	if (error != NULL) {
		g_warning ("%s\n", error->message);
		g_error_free (error);
	}
	gtk_ui_manager_ensure_update (priv->ui);
}


static void
window_update_catalog_ui (GthBrowser *browser)
{
	GthBrowserPrivateData *priv = browser->priv;
	gboolean               is_catalog;
	gboolean               is_search;
	GtkTreeIter            iter;
	GError                *error = NULL;

	if (priv->sidebar_merge_id != 0) {
		gtk_ui_manager_remove_ui (priv->ui, priv->sidebar_merge_id);
		gtk_ui_manager_ensure_update (priv->ui);
	}

	/**/

	is_catalog = (priv->catalog_path != NULL) && (catalog_list_get_iter_from_path (priv->catalog_list, priv->catalog_path, &iter));
	is_search = (is_catalog && (catalog_list_is_search (priv->catalog_list, &iter)));

	if (is_search)
		priv->sidebar_merge_id = gtk_ui_manager_add_ui_from_string (priv->ui, search_ui_info, -1, &error);
	else
		priv->sidebar_merge_id = gtk_ui_manager_add_ui_from_string (priv->ui, catalog_ui_info, -1, &error);
	if (error != NULL) {
		g_warning ("%s\n", error->message);
		g_error_free (error);
	}

	gtk_ui_manager_ensure_update (priv->ui);
}


static void
gth_browser_set_sidebar (GthBrowser *browser,
			 int         sidebar_content)
{
	GthBrowserPrivateData *priv = browser->priv;

	priv->sidebar_content = sidebar_content;
	gtk_notebook_set_current_page (GTK_NOTEBOOK (priv->notebook),
				       sidebar_content - 1);

	if (priv->sidebar_content == GTH_SIDEBAR_CATALOG_LIST)
		window_update_catalog_ui (browser);
	else
		window_update_folder_ui (browser);

	window_update_sensitivity (browser);
}


/* -- window_save_pixbuf -- */


static void
save_pixbuf__jpeg_data_saved_cb (const char     *uri,
				 GnomeVFSResult  result,
    				 gpointer        data)
{
	GthBrowser *browser = data;
	gboolean    closing = browser->priv->closing;
			
	g_free (browser->priv->image_path_saved);
	browser->priv->image_path_saved = NULL;
	if (uri != NULL)
		browser->priv->image_path_saved = g_strdup (uri);

	browser->priv->image_modified = FALSE;
	browser->priv->saving_modified_image = FALSE;
	if (browser->priv->image_saved_func != NULL)
		(*browser->priv->image_saved_func) (NULL, browser);

	if (! closing) {
		GList *file_list;
		
		file_list = g_list_prepend (NULL, (char*) uri);
		if (gth_file_list_pos_from_path (browser->priv->file_list, uri) != -1)
			all_windows_notify_files_changed (file_list);
		else
			all_windows_notify_files_created (file_list);
		g_list_free (file_list);
	}
}


static CopyData*
save_jpeg_data (GthBrowser   *browser,
		FileData     *file,
		CopyDoneFunc  done_func,
		gpointer      done_data)
{
	GthBrowserPrivateData *priv = browser->priv;
	gboolean               data_to_save = FALSE;
	JPEGData              *jdata;
	char                  *local_file = NULL;

	local_file = get_cache_filename_from_uri (file->path);
	if (local_file == NULL)
		return update_file_from_cache (file, done_func, done_data);

	if (! image_is_jpeg (file->path))
		return update_file_from_cache (file, done_func, done_data);

	if (priv->exif_data != NULL)
		data_to_save = TRUE;

#ifdef HAVE_LIBIPTCDATA
	if (priv->iptc_data != NULL)
		data_to_save = TRUE;
#endif /* HAVE_LIBIPTCDATA */

	if (! data_to_save)
		return update_file_from_cache (file, done_func, done_data);

	jdata = jpeg_data_new_from_file (local_file);
	if (jdata == NULL)
		return update_file_from_cache (file, done_func, done_data);

#ifdef HAVE_LIBIPTCDATA
	if (priv->iptc_data != NULL) {
		unsigned char *out_buf, *iptc_buf;
		unsigned int   iptc_len, ps3_len;

		out_buf = g_malloc (256*256);
		iptc_data_save (priv->iptc_data, &iptc_buf, &iptc_len);
		ps3_len = iptc_jpeg_ps3_save_iptc (NULL, 0, iptc_buf,
						   iptc_len, out_buf, 256*256);
		iptc_data_free_buf (priv->iptc_data, iptc_buf);
		if (ps3_len > 0)
			jpeg_data_set_header_data (jdata,
						   JPEG_MARKER_APP13,
						   out_buf, 
						   ps3_len);
		g_free (out_buf);
	}
#endif /* HAVE_LIBIPTCDATA */

	if (priv->exif_data != NULL)
		jpeg_data_set_exif_data (jdata, priv->exif_data);

	jpeg_data_save_file (jdata, local_file);
	jpeg_data_unref (jdata);

	/* The exif orientation tag, if present, must be reset to "top-left",
   	   because the jpeg was saved from a gthumb-generated pixbuf, and
   	   the pixbuf image loader always rotates the pixbuf to account for
   	   the orientation tag. */
	write_orientation_field (local_file, GTH_TRANSFORM_NONE);
	g_free (local_file);
	
	return update_file_from_cache (file, done_func, done_data);
}


static void
save_pixbuf__image_saved_cb (FileData *file,
			     gpointer  data)
{
	GthBrowser *browser = data;
	
	if (file != NULL)
		save_jpeg_data (browser, 
				file, 
				save_pixbuf__jpeg_data_saved_cb,
				browser);
}


static void
gth_browser_save_pixbuf (GthWindow *window,
			 GdkPixbuf *pixbuf,
			 FileData  *file)
{
	GthBrowser            *browser = GTH_BROWSER (window);
	GthBrowserPrivateData *priv = browser->priv;
	char                  *current_folder = NULL;

	if (priv->image != NULL)
		current_folder = g_strdup (priv->image->path);
	else if (priv->dir_list->path != NULL)
		current_folder = g_strconcat (priv->dir_list->path,
					      "/",
					      NULL);

	if (file == NULL)
		dlg_save_image_as (GTK_WINDOW (browser),
				   current_folder,
				   pixbuf,
				   save_pixbuf__image_saved_cb,
				   browser);
	else
		dlg_save_image (GTK_WINDOW (browser),
				file,
				pixbuf,
				save_pixbuf__image_saved_cb,
				browser);

	g_free (current_folder);
}


static void
ask_whether_to_save__response_cb (GtkWidget  *dialog,
				  int         response_id,
				  GthBrowser *browser)
{
	GthBrowserPrivateData *priv = browser->priv;

	gtk_widget_destroy (dialog);

	if (response_id == GTK_RESPONSE_YES) {
		dlg_save_image_as (GTK_WINDOW (browser),
				   priv->image->path,
				   image_viewer_get_current_pixbuf (IMAGE_VIEWER (priv->viewer)),
				   save_pixbuf__image_saved_cb,
				   browser);
		priv->saving_modified_image = TRUE;
	} 
	else {
		priv->saving_modified_image = FALSE;
		priv->image_modified = FALSE;
		if (priv->image_saved_func != NULL)
			(*priv->image_saved_func) (NULL, browser);
	}
}


static gboolean
ask_whether_to_save (GthBrowser     *browser,
		     ImageSavedFunc  image_saved_func)
{
	GtkWidget *d;

	if (! eel_gconf_get_boolean (PREF_MSG_SAVE_MODIFIED_IMAGE, TRUE))
		return FALSE;

	d = _gtk_yesno_dialog_with_checkbutton_new (
			    GTK_WINDOW (browser),
			    GTK_DIALOG_MODAL,
			    _("The current image has been modified, do you want to save it?"),
			    _("Do Not Save"),
			    GTK_STOCK_SAVE_AS,
			    _("_Do not display this message again"),
			    PREF_MSG_SAVE_MODIFIED_IMAGE);

	browser->priv->saving_modified_image = TRUE;
	browser->priv->image_saved_func = image_saved_func;
	g_signal_connect (G_OBJECT (d), "response",
			  G_CALLBACK (ask_whether_to_save__response_cb),
			  browser);

	gtk_widget_show (d);

	return TRUE;
}


static void
real_set_void (FileData *file,
	       gpointer  data)
{
	GthBrowser *browser = data;

	if (! browser->priv->image_error) {
		file_data_unref (browser->priv->image);
		browser->priv->image = NULL;
		browser->priv->image_modified = FALSE;
		browser->priv->image_position = -1;
	}

	image_viewer_set_void (IMAGE_VIEWER (browser->priv->viewer));
	gth_window_clear_undo_history (GTH_WINDOW (browser));

	window_update_statusbar_image_info (browser);
 	window_update_image_info (browser);
	window_update_title (browser);
	window_update_infobar (browser);
	window_update_sensitivity (browser);

	if (browser->priv->image_prop_dlg != NULL)
		dlg_image_prop_update (browser->priv->image_prop_dlg);
}


static void
window_image_viewer_set_void (GthBrowser *browser)
{
	GthBrowserPrivateData *priv = browser->priv;

	priv->image_error = FALSE;
	if (priv->image_modified)
		if (ask_whether_to_save (browser, real_set_void))
			return;
	real_set_void (NULL, browser);
}


static void
window_image_viewer_set_error (GthBrowser *browser)
{
	GthBrowserPrivateData *priv = browser->priv;

	priv->image_error = TRUE;
	if (priv->image_modified)
		if (ask_whether_to_save (browser, real_set_void))
			return;
	real_set_void (NULL, browser);
}


static void
make_image_visible (GthBrowser *browser,
		    int         pos)
{
	GthBrowserPrivateData *priv = browser->priv;
	GthVisibility          visibility;

	if ((pos < 0) || (pos >= gth_file_view_get_images (priv->file_list->view)))
		return;

	visibility = gth_file_view_image_is_visible (priv->file_list->view, pos);
	if (visibility != GTH_VISIBILITY_FULL) {
		double offset = 0.5;

		switch (visibility) {
		case GTH_VISIBILITY_NONE:
			offset = 0.5;
			break;
		case GTH_VISIBILITY_PARTIAL_TOP:
			offset = 0.0;
			break;
		case GTH_VISIBILITY_PARTIAL_BOTTOM:
			offset = 1.0;
			break;
		case GTH_VISIBILITY_PARTIAL:
		case GTH_VISIBILITY_FULL:
			offset = -1.0;
			break;
		}

		if (offset > -1.0)
			gth_file_view_moveto (priv->file_list->view, pos, offset);
	}
}


static void
window_make_current_image_visible (GthBrowser *browser,
				   gboolean    reset_if_not_found)
{
	GthBrowserPrivateData *priv = browser->priv;
	char                  *path;
	int                    pos, n_images;

	path = image_viewer_get_image_filename (IMAGE_VIEWER (priv->viewer));
	if (path == NULL)
		return;

	pos = gth_file_list_pos_from_path (priv->file_list, path);

	if (pos >= 0) {
		priv->image_position = pos;
		make_image_visible (browser, pos);
		g_free (path);
		return;
	}

	if (reset_if_not_found) {
		window_image_viewer_set_void (browser);
		g_free (path);
		return;
	}

	n_images = gth_file_view_get_images (priv->file_list->view);
	if (priv->image_position >= n_images)
		priv->image_position = n_images - 1;
	if (priv->image_position >= 0) {
		gth_file_view_select_image (priv->file_list->view, priv->image_position);
		gth_file_view_set_cursor (priv->file_list->view, priv->image_position);
	}
	else if (! path_is_file (path))
		window_image_viewer_set_void (browser);

	g_free (path);
}


/* -- callbacks -- */


static void
close_window_cb (GtkWidget  *caller,
		 GdkEvent   *event,
		 GthBrowser *browser)
{
	gth_window_close (GTH_WINDOW (browser));
}


static gboolean
sel_change_update_cb (gpointer data)
{
	GthBrowser *browser = data;

	g_source_remove (browser->priv->sel_change_timeout);
	browser->priv->sel_change_timeout = 0;

	window_update_sensitivity (browser);
	window_update_statusbar_list_info (browser);

	return FALSE;
}


static int
file_selection_changed_cb (GtkWidget *widget,
			   gpointer   data)
{
	GthBrowser            *browser = data;
	GthBrowserPrivateData *priv = browser->priv;

	if (priv->sel_change_timeout != 0)
		g_source_remove (priv->sel_change_timeout);

	priv->sel_change_timeout = g_timeout_add (SEL_CHANGED_DELAY,
						  sel_change_update_cb,
						  browser);

	gth_window_update_comment_categories_dlg (GTH_WINDOW (browser));

	return TRUE;
}


static void
gth_file_list_cursor_changed_cb (GtkWidget *widget,
				 int        pos,
				 gpointer   data)
{
	GthBrowser            *browser = data;
	GthBrowserPrivateData *priv = browser->priv;
	char                  *focused_image;

	focused_image = gth_file_list_path_from_pos (priv->file_list, pos);

	if (focused_image == NULL)
		return;

	if ((priv->image == NULL) || ! same_uri (focused_image, priv->image->path))
		view_image_at_pos (browser, pos);

	g_free (focused_image);
}


static int
gth_file_list_button_press_cb (GtkWidget      *widget,
			       GdkEventButton *event,
			       gpointer        data)
{
	GthBrowser            *browser = data;
	GthBrowserPrivateData *priv = browser->priv;

	if (event->type == GDK_3BUTTON_PRESS)
		return FALSE;

	if ((event->button != 1) && (event->button != 3))
		return FALSE;

	if ((event->state & GDK_SHIFT_MASK)
	    || (event->state & GDK_CONTROL_MASK))
		return FALSE;

	if (event->button == 1) {
		int pos;

		pos = gth_file_view_get_image_at (priv->file_list->view,
						  event->x,
						  event->y);

		if (pos == -1)
			return FALSE;

		if (event->type == GDK_2BUTTON_PRESS)
			return FALSE;

		if (event->type == GDK_BUTTON_PRESS) {
			make_image_visible (browser, pos);
			view_image_at_pos (browser, pos);
			return FALSE;
		}
	} 
	else if (event->button == 3) {
		int  pos;

		pos = gth_file_view_get_image_at (priv->file_list->view,
						  event->x,
						  event->y);

		if (pos != -1) {
			if (! gth_file_list_is_selected (priv->file_list, pos))
				gth_file_list_select_image_by_pos (priv->file_list, pos);
		} 
		else
			gth_file_list_unselect_all (priv->file_list);

		window_update_sensitivity (browser);

		gtk_menu_popup (GTK_MENU (priv->file_popup_menu),
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
	gth_browser_hide_sidebar ((GthBrowser*) data);
	return FALSE;
}


static int
gth_file_list_item_activated_cb (GtkWidget *widget,
				 int        idx,
				 gpointer   data)
{
	GthBrowser *browser = data;

	/* use a timeout to avoid that the viewer gets
	 * the button press event. */
	g_timeout_add (HIDE_SIDEBAR_DELAY, hide_sidebar_idle, browser);

	return TRUE;
}


static void
dir_list_activated_cb (GtkTreeView       *tree_view,
		       GtkTreePath       *path,
		       GtkTreeViewColumn *column,
		       gpointer           data)
{
	GthBrowser *browser = data;
	char       *new_dir;
	GthBrowserPrivateData *priv = browser->priv;

	new_dir = gth_dir_list_get_path_from_tree_path (browser->priv->dir_list, path);
	gth_browser_go_to_directory (browser, new_dir);
	g_free (new_dir);

	if (priv->sidebar_content == GTH_SIDEBAR_DIR_LIST)
		gtk_widget_grab_focus (priv->dir_list->list_view);
	else
		gtk_widget_grab_focus (priv->catalog_list->list_view);
}


/**/

static int
dir_list_key_press_cb ( GtkWidget *widget,
			GdkEventKey *event,
			gpointer data)
{
	GthBrowser       *browser = data;
	GtkWidget	 *treeview = browser->priv->dir_list->list_view;
	GtkTreeIter	  iter;
	GtkTreeSelection *tree_selection;
	gboolean          has_selected;
	char	         *utf8_name;
	char	         *name;

  	switch (gdk_keyval_to_lower (event->keyval)) {
	case GDK_Delete:
		tree_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));

		if (gtk_tree_selection_get_mode(tree_selection) == GTK_SELECTION_MULTIPLE)
			return TRUE;

		has_selected = gtk_tree_selection_get_selected (tree_selection,
								NULL,
								&iter);
		if (has_selected == FALSE)
			return TRUE;

		utf8_name = gth_dir_list_get_name_from_iter (browser->priv->dir_list, &iter);
		name = gnome_vfs_escape_string (utf8_name);
		g_free (utf8_name);

		if (strcmp (name, "..") == 0) {
			g_free (name);
			return TRUE;
		}
		g_free (name);
		gth_browser_activate_action_edit_dir_delete (NULL, browser);
      		break;

    	default:
      		break;
    	}

	return FALSE;
}


static int
dir_list_button_press_cb (GtkWidget      *widget,
			  GdkEventButton *event,
			  gpointer        data)
{
	GthBrowser            *browser = data;
	GthBrowserPrivateData *priv = browser->priv;
	GtkWidget             *treeview = priv->dir_list->list_view;
	GtkListStore          *list_store = priv->dir_list->list_store;
	GtkTreePath           *path;
	GtkTreeIter            iter;

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
	GthBrowser            *browser = data;
	GthBrowserPrivateData *priv = browser->priv;
	GtkWidget             *treeview = priv->dir_list->list_view;
	GtkListStore          *list_store = priv->dir_list->list_store;
	GtkTreePath           *path;
	GtkTreeIter            iter;

	if ((event->state & GDK_SHIFT_MASK)
	    || (event->state & GDK_CONTROL_MASK))
		return FALSE;

	if ((event->button != 1) && (event->button != 3))
		return FALSE;

	if (! gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (treeview),
					     event->x, event->y,
					     &path, NULL, NULL, NULL)) {
		if (event->button != 3)
			return FALSE;

		window_update_sensitivity (browser);
		gtk_menu_popup (GTK_MENU (priv->dir_list_popup_menu),
				NULL,
				NULL,
				NULL,
				NULL,
				3,
				event->time);

		return TRUE;
	}

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
	    && (pref_get_real_click_policy () == GTH_CLICK_POLICY_SINGLE)) {
		char *new_dir;

		new_dir = gth_dir_list_get_path_from_iter (priv->dir_list,
							   &iter);
		gth_browser_go_to_directory (browser, new_dir);
		g_free (new_dir);

		return FALSE;

	} else if (event->button == 3) {
		GtkTreeSelection *selection;
		char             *utf8_name;
		char             *name;

		utf8_name = gth_dir_list_get_name_from_iter (priv->dir_list, &iter);
		name = gnome_vfs_escape_string (utf8_name);
		g_free (utf8_name);

		if (strcmp (name, "..") == 0) {
			g_free (name);
			return TRUE;
		}
		g_free (name);

		/* Update selection. */

		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->dir_list->list_view));
		if (selection == NULL)
			return FALSE;

		if (! gtk_tree_selection_iter_is_selected (selection, &iter)) {
			gtk_tree_selection_unselect_all (selection);
			gtk_tree_selection_select_iter (selection, &iter);
		}

		/* Popup menu. */

		window_update_sensitivity (browser);
		gtk_menu_popup (GTK_MENU (priv->dir_popup_menu),
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
	window_update_sensitivity ((GthBrowser *) p);
}


static void
add_history_item (GthBrowser *browser,
		  const char *path,
		  const char *prefix)
{
	GthBrowserPrivateData *priv = browser->priv;
	char                  *location;

	bookmarks_remove_from (priv->history, priv->history_current);

	if (prefix == NULL)
		location = add_scheme_if_absent (path);
	else
		location = g_strconcat (prefix, path, NULL);
	bookmarks_remove_all_instances (priv->history, location);
	bookmarks_add (priv->history, location, FALSE, FALSE);
	g_free (location);

	priv->history_current = priv->history->list;
}


static void
catalog_get_file_data_list_done (Catalog  *catalog,
				 GList    *file_list,
				 gpointer  data)
{
	GthBrowser    *browser = data;
	GthSortMethod  sort_method;
	GtkSortType    sort_type;
	
	sort_method = catalog->sort_method;
	sort_type = catalog->sort_type;
	if (sort_method == GTH_SORT_METHOD_NONE) {
		sort_method = browser->priv->sort_method;
		sort_type = browser->priv->sort_type;
	}
	catalog_free (catalog);
	
	window_set_file_list (browser, file_list, sort_method, sort_type);
}


static gboolean
catalog_activate (GthBrowser *browser,
		  const char *cat_path)
{
	GthBrowserPrivateData *priv = browser->priv;
	Catalog               *catalog;
	GError                *gerror;
	GtkTreeIter            iter;

	if (cat_path == NULL)
		return FALSE;

	/* catalog directory */

	if (path_is_dir (cat_path)) {
		gth_browser_go_to_catalog (browser, NULL);
		gth_browser_go_to_catalog_directory (browser, cat_path);
		return TRUE;
	}

	/* catalog */

	if (! catalog_list_get_iter_from_path (priv->catalog_list,
					       cat_path,
					       &iter))
		return FALSE;
	else
		catalog_list_select_iter (priv->catalog_list, &iter);

	if (priv->catalog_path != cat_path) {
		if (priv->catalog_path)
			g_free (priv->catalog_path);
		priv->catalog_path = g_strdup (cat_path);
	}

	gth_browser_set_sidebar (browser, GTH_SIDEBAR_CATALOG_LIST);

	catalog = catalog_new ();
	if (! catalog_load_from_disk (catalog, cat_path, &gerror)) {
		_gtk_error_dialog_from_gerror_run (GTK_WINDOW (browser), &gerror);
		catalog_free (catalog);
		return FALSE;
	}

	set_cursor_busy (browser, TRUE);
	catalog_get_file_data_list (catalog, catalog_get_file_data_list_done, browser);

	return TRUE;
}


static void
catalog_list_activated_cb (GtkTreeView       *tree_view,
			   GtkTreePath       *path,
			   GtkTreeViewColumn *column,
			   gpointer           data)
{
	GthBrowser *browser = data;
	char       *cat_path;

	cat_path = catalog_list_get_path_from_tree_path (browser->priv->catalog_list, path);
	if (cat_path == NULL)
		return;
	catalog_activate (browser, cat_path);
	g_free (cat_path);
}


static int
catalog_list_button_press_cb (GtkWidget      *widget,
			      GdkEventButton *event,
			      gpointer        data)
{
	GthBrowser            *browser = data;
	GthBrowserPrivateData *priv = browser->priv;
	GtkWidget             *treeview = priv->catalog_list->list_view;
	GtkListStore          *list_store = priv->catalog_list->list_store;
	GtkTreeIter            iter;
	GtkTreePath           *path;

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
	GthBrowser            *browser = data;
	GthBrowserPrivateData *priv = browser->priv;
	GtkWidget             *treeview = priv->catalog_list->list_view;
	GtkListStore          *list_store = priv->catalog_list->list_store;
	GtkTreeIter            iter;
	GtkTreePath           *path;

	if ((event->state & GDK_SHIFT_MASK)
	    || (event->state & GDK_CONTROL_MASK))
		return FALSE;

	if ((event->button != 1) & (event->button != 3))
		return FALSE;

	/* Get the path. */

	if (! gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (treeview),
					     event->x, event->y,
					     &path, NULL, NULL, NULL)) {
		if (event->button != 3)
			return FALSE;

		window_update_sensitivity (browser);
		gtk_menu_popup (GTK_MENU (priv->catalog_list_popup_menu),
				NULL,
				NULL,
				NULL,
				NULL,
				3,
				event->time);

		return TRUE;
	}

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
	    (pref_get_real_click_policy () == GTH_CLICK_POLICY_SINGLE)) {
		char *cat_path;

		cat_path = catalog_list_get_path_from_iter (priv->catalog_list, &iter);
		g_return_val_if_fail (cat_path != NULL, FALSE);
		catalog_activate (browser, cat_path);
		g_free (cat_path);

		return FALSE;

	} else if (event->button == 3) {
		GtkTreeSelection *selection;
		char             *utf8_name;
		char             *name;

		utf8_name = catalog_list_get_name_from_iter (priv->catalog_list, &iter);
		name = gnome_vfs_escape_string (utf8_name);
		g_free (utf8_name);

		if (strcmp (name, "..") == 0) {
			g_free (name);
			return TRUE;
		}
		g_free (name);

		/* Update selection. */

		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->catalog_list->list_view));
		if (selection == NULL)
			return FALSE;

		if (! gtk_tree_selection_iter_is_selected (selection, &iter)) {
			gtk_tree_selection_unselect_all (selection);
			gtk_tree_selection_select_iter (selection, &iter);
		}

		/* Popup menu. */

		window_update_sensitivity (browser);
		if (catalog_list_is_dir (priv->catalog_list, &iter))
			gtk_menu_popup (GTK_MENU (priv->library_popup_menu),
					NULL,
					NULL,
					NULL,
					NULL,
					3,
					event->time);
		else
			gtk_menu_popup (GTK_MENU (priv->catalog_popup_menu),
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


/* -- */


static void
location_changed_cb (GthLocation *loc,
		     const char  *uri,
		     GthBrowser  *browser)
{
	go_to_uri (browser, uri, FALSE);
}


static void
go_to_folder_after_image_loaded (GthBrowser *browser)
{
	GthBrowserPrivateData *priv = browser->priv;
	char *folder_uri = remove_level_from_path (priv->image->path);

	priv->focus_current_image = TRUE;
	gth_browser_hide_sidebar (browser);

	if (! same_uri (folder_uri, priv->dir_list->path)) {
		priv->refreshing = TRUE;
		gth_browser_go_to_directory (browser, folder_uri);
	}

	g_free (folder_uri);
}


static void
image_loaded_cb (GtkWidget  *widget,
		 GthBrowser *browser)
{
	GthBrowserPrivateData *priv = browser->priv;

	priv->image_modified = FALSE;
	priv->loading_image = FALSE;
	gth_window_clear_undo_history (GTH_WINDOW (browser));

	window_update_image_info (browser);
	window_update_infobar (browser);
	window_update_title (browser);
	window_update_sensitivity (browser);

	if (priv->image_prop_dlg != NULL)
		dlg_image_prop_update (priv->image_prop_dlg);

	if (browser->priv->load_image_folder_after_image)
		go_to_folder_after_image_loaded (browser);
}


static void
image_requested_error_cb (GThumbPreloader *gploader,
			  GthBrowser      *browser)
{
	set_cursor_not_busy (browser, FALSE);

	window_image_viewer_set_error (browser);
	if (browser->priv->load_image_folder_after_image)
		go_to_folder_after_image_loaded (browser);
}


static void
image_requested_done_cb (GThumbPreloader *gploader,
			 GthBrowser      *browser)
{
	GthBrowserPrivateData *priv = browser->priv;
	ImageLoader           *loader;

	set_cursor_not_busy (browser, FALSE);

	priv->image_error = FALSE;
	if (priv->image == NULL)
		return;

	priv->loading_image = TRUE;

	loader = gthumb_preloader_get_loader (priv->preloader, priv->image->path);
	if (loader != NULL)
		image_viewer_load_from_image_loader (IMAGE_VIEWER (priv->viewer), loader);
}


static gint
zoom_changed_cb (GtkWidget  *widget,
		 GthBrowser *browser)
{
	window_update_statusbar_zoom_info (browser);
	return TRUE;
}


void
gth_browser_show_image_data (GthBrowser *browser)
{
	GthBrowserPrivateData *priv = browser->priv;

	gtk_widget_show (priv->preview_widget_data);
	gtk_widget_show (priv->preview_widget_comment);
	gtk_widget_show (priv->preview_widget_data_comment);
	set_button_active_no_notify (browser, priv->preview_button_data, TRUE);

	priv->image_data_visible = TRUE;
	set_action_active_if_different (browser, "View_ShowMetadata", TRUE);
}


void
gth_browser_hide_image_data (GthBrowser *browser)
{
	GthBrowserPrivateData *priv = browser->priv;

	gtk_widget_hide (priv->preview_widget_data_comment);
	set_button_active_no_notify (browser, priv->preview_button_data, FALSE);
	priv->image_data_visible = FALSE;
	set_action_active_if_different (browser, "View_ShowMetadata", FALSE);
}


void
gth_browser_show_filterbar (GthBrowser *browser)
{
	gtk_widget_show (browser->priv->filterbar);
	set_action_active_if_different (browser, "View_Filterbar", TRUE);
}


void
gth_browser_hide_filterbar (GthBrowser *browser)
{
	gtk_widget_hide (browser->priv->filterbar);
	set_action_active_if_different (browser, "View_Filterbar", FALSE);
	gth_file_list_set_filter (browser->priv->file_list, NULL);
}


static void
toggle_image_data_visibility (GthBrowser *browser)
{
	GthBrowserPrivateData *priv = browser->priv;

	if (priv->sidebar_visible)
		return;

	if (priv->image_data_visible)
		gth_browser_hide_image_data (browser);
	else
		gth_browser_show_image_data (browser);
}


static void
change_image_preview_content (GthBrowser *browser)
{
	GthBrowserPrivateData *priv = browser->priv;

	if (! priv->sidebar_visible)
		return;

	if (! priv->image_pane_visible) {
		gth_browser_show_image_pane (browser);
		return;
	}

	if (priv->preview_content == GTH_PREVIEW_CONTENT_IMAGE)
		gth_browser_set_preview_content (browser, GTH_PREVIEW_CONTENT_DATA);

	else if (priv->preview_content == GTH_PREVIEW_CONTENT_DATA)
		gth_browser_set_preview_content (browser, GTH_PREVIEW_CONTENT_COMMENT);

	else if (priv->preview_content == GTH_PREVIEW_CONTENT_COMMENT)
		gth_browser_set_preview_content (browser, GTH_PREVIEW_CONTENT_IMAGE);
}


static void
toggle_image_preview_visibility (GthBrowser *browser)
{
	if (! browser->priv->sidebar_visible)
		return;

	if (browser->priv->preview_visible)
		gth_browser_hide_image_pane (browser);
	 else
		gth_browser_show_image_pane (browser);
}


static void
window_enable_thumbs (GthBrowser *browser,
		      gboolean    enable)
{
	GthBrowserPrivateData *priv = browser->priv;

	gth_file_list_enable_thumbs (priv->file_list, enable, TRUE);
	set_action_sensitive (browser, "Go_Stop",
			       ((priv->activity_ref > 0)
				|| priv->file_list->busy));
}


static gboolean
sidebar_list_key_press (GthBrowser  *browser,
			GdkEventKey *event)
{
	GthBrowserPrivateData *priv = browser->priv;
	gboolean               retval = FALSE;
	GtkTreeIter            iter;
	GtkTreeSelection      *tree_selection;
	GtkWidget             *list_view;
	char                  *new_path;

	switch (gdk_keyval_to_lower (event->keyval)) {
	case GDK_Return:
 	case GDK_KP_Enter:
		if (priv->sidebar_content == GTH_SIDEBAR_DIR_LIST)
			list_view = priv->dir_list->list_view;
		else
			list_view = priv->catalog_list->list_view;

		tree_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (list_view));

		if (gtk_tree_selection_get_mode (tree_selection) == GTK_SELECTION_MULTIPLE)
			break;
		if (gtk_tree_selection_get_selected (tree_selection, NULL, &iter) == FALSE)
			break;

		if (priv->sidebar_content == GTH_SIDEBAR_DIR_LIST) {
			new_path = gth_dir_list_get_path_from_iter (priv->dir_list, &iter);
			gth_browser_go_to_directory (browser, new_path);
		} else {
			new_path = catalog_list_get_path_from_iter (priv->catalog_list, &iter);
			catalog_activate (browser, new_path);
		}

		g_free (new_path);
		retval = TRUE;

		if (priv->sidebar_content == GTH_SIDEBAR_DIR_LIST)
			gtk_widget_grab_focus (priv->dir_list->list_view);
		else
			gtk_widget_grab_focus (priv->catalog_list->list_view);

		break;

	default:
		break;
	}

	return retval;
}


static gboolean
launch_selected_videos_or_audio (GthBrowser *browser)
{
	gboolean                 result = FALSE;
	GnomeVFSMimeApplication *image_app;
	const char              *image_mime_type;
	GList		        *video_list = NULL;
	GList			*scan;

	if (browser->priv->image == NULL)
		return FALSE;

	/* Images are handled by gThumb directly. Other supported media (video,
	   audio) require external players, which are launched by this function. */
	if (mime_type_is_image (browser->priv->image->mime_type))
		return FALSE;

	image_mime_type = browser->priv->image->mime_type;
	image_app = gnome_vfs_mime_get_default_application_for_uri (browser->priv->image->path, image_mime_type);

	if (image_app == NULL)
		return FALSE;

	if (! image_app->can_open_multiple_files) {
		/* just pass the current (single) item */
		video_list = g_list_append (video_list, g_strdup (browser->priv->image->path));
	} 
	else {
		GList *selection;
		
		/* Scan through the list of selected items, and identify those that have the
		   same mime_type, or can be launched by the same application. */
		
		selection = gth_window_get_file_list_selection_as_fd (GTH_WINDOW (browser));
		for (scan = selection; scan; scan = scan->next) {
			FileData *file = scan->data;

			if (mime_type_is (file->mime_type, image_mime_type)) {
				video_list = g_list_append (video_list, g_strdup (file->path));
			}
			else {
				GnomeVFSMimeApplication *selected_app;
				
				selected_app = gnome_vfs_mime_get_default_application_for_uri (file->path, file->mime_type);
				if (gnome_vfs_mime_application_equal (image_app, selected_app))
					video_list = g_list_append (video_list, g_strdup (file->path));
				gnome_vfs_mime_application_free (selected_app);
			}
		}
		file_data_list_free (selection);
	}

	result = gnome_vfs_mime_application_launch (image_app, video_list) == GNOME_VFS_OK;
	
	gnome_vfs_mime_application_free (image_app);
	path_list_free (video_list);

	return result;
}


static gint
key_press_cb (GtkWidget   *widget,
	      GdkEventKey *event,
	      gpointer     data)
{
	GthBrowser            *browser = data;
	GthWindow             *window = (GthWindow*) browser;
	GthBrowserPrivateData *priv = browser->priv;
	gboolean               sel_not_null;
	gboolean               image_is_void;
	GList		      *list = NULL;

	if (GTK_WIDGET_HAS_FOCUS (priv->preview_button_image)
	    || GTK_WIDGET_HAS_FOCUS (priv->preview_button_data)
	    || GTK_WIDGET_HAS_FOCUS (priv->preview_button_comment))
		if (event->keyval == GDK_space)
			return FALSE;

	if (gth_filter_bar_has_focus (GTH_FILTER_BAR (priv->filterbar)))
		return FALSE;

	if (GTK_WIDGET_HAS_FOCUS (priv->dir_list->list_view)
	    || GTK_WIDGET_HAS_FOCUS (priv->catalog_list->list_view))
		if (sidebar_list_key_press (browser, event))
			return FALSE;

	if (priv->sidebar_visible
	    && (event->state & GDK_CONTROL_MASK)
	    && ((event->keyval == GDK_1)
		|| (event->keyval == GDK_2)
		|| (event->keyval == GDK_3))) {
		GthPreviewContent content;

		switch (event->keyval) {
		case GDK_1:
			content = GTH_PREVIEW_CONTENT_IMAGE;
			break;
		case GDK_2:
			content = GTH_PREVIEW_CONTENT_DATA;
			break;
		case GDK_3:
		default:
			content = GTH_PREVIEW_CONTENT_COMMENT;
			break;
		}

		if (priv->preview_content == content)
			toggle_image_preview_visibility (browser);
		 else {
			if (! priv->preview_visible)
				gth_browser_show_image_pane (browser);
			gth_browser_set_preview_content (browser, content);
		}
	}

	if (((event->state & GDK_CONTROL_MASK)

		/* Let pass the ctrl+keypad_add and ctrl+keypad_subtract. */
		&& (event->keyval != GDK_KP_Add) && (event->keyval != GDK_KP_Subtract)

		) || (event->state & GDK_MOD1_MASK))
			return FALSE;

	sel_not_null = gth_file_view_get_n_selected (priv->file_list->view) > 0;
	image_is_void = image_viewer_is_void (IMAGE_VIEWER (priv->viewer));

	switch (gdk_keyval_to_lower (event->keyval)) {
		/* Hide/Show sidebar. */
	case GDK_Return:
	case GDK_KP_Enter:
		if (priv->sidebar_visible) {
			/* When files are selected in the browser view and you press enter,
			   launch the video viewer if videos are selected. If no videos are
			   selected, go to image viewer mode. */
			gth_browser_hide_sidebar (browser);
		}
		else {
			/* When in the image viewer mode and you press enter, launch the video
			   viewer if a video thumbnail is shown, and then return to the browser
			   mode in the normal fashion. */
			launch_selected_videos_or_audio (browser);
			gth_browser_show_sidebar (browser);
		}
		return TRUE;

		/* Show sidebar */
	case GDK_Escape:
		if (!priv->sidebar_visible)
			gth_browser_show_sidebar (browser);
		return TRUE;

		/* Show / hide image info */
	case GDK_i:
		toggle_image_preview_visibility (browser);
		toggle_image_data_visibility (browser);
		return TRUE;

		/* Cycle through the content shown in preview pane */
	case GDK_q:
		change_image_preview_content (browser);
		return TRUE;

		/* Full screen view. */
	case GDK_v:
	case GDK_f:
		gth_window_set_fullscreen (GTH_WINDOW (browser), TRUE);
		return TRUE;

		/* View/hide thumbnails. */
	case GDK_t:
		eel_gconf_set_boolean (PREF_SHOW_THUMBNAILS, !priv->file_list->enable_thumbs);
		return TRUE;

		/* Toggle animation. */
	case GDK_a:
		gth_window_set_animation (window, ! gth_window_get_animation (window));
		break;

		/* Step animation. */
	case GDK_j:
		gth_window_step_animation (window);
		break;

		/* Show previous image. */
	case GDK_b:
	case GDK_BackSpace:
		gth_browser_show_prev_image (browser, FALSE);
		return TRUE;

		/* Show next image. */
	case GDK_n:
		gth_browser_show_next_image (browser, FALSE);
		return TRUE;

	case GDK_space:
		if (! GTK_WIDGET_HAS_FOCUS (priv->dir_list->list_view)
		    && ! GTK_WIDGET_HAS_FOCUS (priv->catalog_list->list_view)) {
			gth_browser_show_next_image (browser, FALSE);
			return TRUE;
		}
		break;

		/* Rotate clockwise without saving */
	case GDK_r:
		gth_window_activate_action_alter_image_rotate90 (NULL, window);
		return TRUE;

		/* Rotate counter-clockwise without saving */
	case GDK_e:
		gth_window_activate_action_alter_image_rotate90cc (NULL, window);
		return TRUE;

		/* Lossless clockwise rotation. */
	case GDK_bracketright:
		gth_window_activate_action_tools_jpeg_rotate_right (NULL, window);
		return TRUE;

		/* Lossless counter-clockwise rotation */
	case GDK_bracketleft:
		gth_window_activate_action_tools_jpeg_rotate_left (NULL, window);
		return TRUE;

		/* Flip image */
	case GDK_l:
		gth_window_activate_action_alter_image_flip (NULL, window);
		return TRUE;

		/* Mirror image */
	case GDK_m:
		gth_window_activate_action_alter_image_mirror (NULL, window);
		return TRUE;

		/* Delete selection. */
	case GDK_Delete:
	case GDK_KP_Delete:
		if (! sel_not_null)
			break;

		if (priv->sidebar_content == GTH_SIDEBAR_DIR_LIST)
			gth_browser_activate_action_edit_delete_files (NULL, browser);
		else if (priv->sidebar_content == GTH_SIDEBAR_CATALOG_LIST)
			gth_browser_activate_action_edit_remove_from_catalog (NULL, browser);
		return TRUE;

		/* Open images. */
	case GDK_o:
		if (! sel_not_null)
			break;
		gth_window_activate_action_file_open_with (NULL, browser);
		return TRUE;

		/* Edit comment */
	case GDK_c:
		if (! sel_not_null)
			break;
		gth_window_edit_comment (GTH_WINDOW (browser));
		return TRUE;

		/* Edit keywords */
	case GDK_k:
		if (! sel_not_null)
			break;
		gth_window_edit_categories (GTH_WINDOW (browser));
		return TRUE;

		/* gimp hotkey */
	case GDK_g:
		list = gth_window_get_file_list_selection (window);
		if (list != NULL) {
			exec_command ("gimp-remote", list);
			path_list_free (list);
		}

		return TRUE;

		/* hot keys */
	case GDK_KP_0:
	case GDK_KP_Insert:
		list = gth_window_get_file_list_selection (window);
		if (list != NULL) {
			exec_shell_script ( GTK_WINDOW (browser), eel_gconf_get_string (PREF_HOTKEY0, NULL), list);
			path_list_free (list);
		}
		return TRUE;

	case GDK_KP_1:
	case GDK_KP_End:
		list = gth_window_get_file_list_selection (window);
		if (list != NULL) {
			exec_shell_script ( GTK_WINDOW (browser), eel_gconf_get_string (PREF_HOTKEY1, NULL), list);
			path_list_free (list);
		}
		return TRUE;

	case GDK_KP_2:
	case GDK_KP_Down:
		list = gth_window_get_file_list_selection (window);
		if (list != NULL) {
			exec_shell_script ( GTK_WINDOW (browser), eel_gconf_get_string (PREF_HOTKEY2, NULL), list);
			path_list_free (list);
		}
		return TRUE;

	case GDK_KP_3:
	case GDK_KP_Page_Down:
		list = gth_window_get_file_list_selection (window);
		if (list != NULL) {
			exec_shell_script ( GTK_WINDOW (browser), eel_gconf_get_string (PREF_HOTKEY3, NULL), list);
			path_list_free (list);
		}
		return TRUE;

	case GDK_KP_4:
	case GDK_KP_Left:
		list = gth_window_get_file_list_selection (window);
		if (list != NULL) {
			exec_shell_script ( GTK_WINDOW (browser), eel_gconf_get_string (PREF_HOTKEY4, NULL), list);
			path_list_free (list);
		}
		return TRUE;

	case GDK_KP_5:
		list = gth_window_get_file_list_selection (window);
		if (list != NULL) {
			exec_shell_script ( GTK_WINDOW (browser), eel_gconf_get_string (PREF_HOTKEY5, NULL), list);
			path_list_free (list);
		}
		return TRUE;

	case GDK_KP_6:
	case GDK_KP_Right:
		list = gth_window_get_file_list_selection (window);
		if (list != NULL) {
			exec_shell_script ( GTK_WINDOW (browser), eel_gconf_get_string (PREF_HOTKEY6, NULL), list);
			path_list_free (list);
		}
		return TRUE;

	case GDK_KP_7:
	case GDK_KP_Home:
		list = gth_window_get_file_list_selection (window);
		if (list != NULL) {
			exec_shell_script ( GTK_WINDOW (browser), eel_gconf_get_string (PREF_HOTKEY7, NULL), list);
			path_list_free (list);
		}
		return TRUE;

	case GDK_KP_8:
	case GDK_KP_Up:
		list = gth_window_get_file_list_selection (window);
		if (list != NULL) {
			exec_shell_script ( GTK_WINDOW (browser), eel_gconf_get_string (PREF_HOTKEY8, NULL), list);
			path_list_free (list);
		}
		return TRUE;

	case GDK_KP_9:
	case GDK_KP_Page_Up:
		list = gth_window_get_file_list_selection (window);
		if (list != NULL) {
			exec_shell_script ( GTK_WINDOW (browser), eel_gconf_get_string (PREF_HOTKEY9, NULL), list);
			path_list_free (list);
		}
		return TRUE;


		/* Go up one level */
	case GDK_u:
		gth_browser_go_up (browser);
		return TRUE;

		/* Go home */
	case GDK_h:
		gth_browser_activate_action_go_home (NULL, browser);
		return TRUE;

	case GDK_F5:
		gth_browser_refresh (browser);
		return TRUE;

	default:
		break;
	}

	if ((event->keyval == GDK_F10)
	    && (event->state & GDK_SHIFT_MASK)) {

		if ((priv->sidebar_content == GTH_SIDEBAR_CATALOG_LIST)
		    && GTK_WIDGET_HAS_FOCUS (priv->catalog_list->list_view)) {
			GtkTreeSelection *selection;
			GtkTreeIter       iter;
			char             *name, *utf8_name;

			selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->catalog_list->list_view));
			if (! gtk_tree_selection_get_selected (selection, NULL, &iter))
				return FALSE;

			utf8_name = catalog_list_get_name_from_iter (priv->catalog_list, &iter);
			name = gnome_vfs_escape_string (utf8_name);
			g_free (utf8_name);

			if (strcmp (name, "..") == 0)
				return TRUE;

			if (catalog_list_is_dir (priv->catalog_list, &iter))
				gtk_menu_popup (GTK_MENU (priv->library_popup_menu),
						NULL,
						NULL,
						NULL,
						NULL,
						3,
						event->time);
			else
				gtk_menu_popup (GTK_MENU (priv->catalog_popup_menu),
						NULL,
						NULL,
						NULL,
						NULL,
						3,
						event->time);

			return TRUE;

		} else if ((priv->sidebar_content == GTH_SIDEBAR_DIR_LIST)
			   && GTK_WIDGET_HAS_FOCUS (priv->dir_list->list_view)) {
			GtkTreeSelection *selection;
			GtkTreeIter       iter;
			char             *name, *utf8_name;

			selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->dir_list->list_view));
			if (! gtk_tree_selection_get_selected (selection, NULL, &iter))
				return FALSE;

			utf8_name = gth_dir_list_get_name_from_iter (priv->dir_list, &iter);
			name = gnome_vfs_escape_string (utf8_name);
			g_free (utf8_name);

			if (strcmp (name, "..") == 0)
				return TRUE;

			gtk_menu_popup (GTK_MENU (priv->dir_popup_menu),
					NULL,
					NULL,
					NULL,
					NULL,
					3,
					event->time);

			return TRUE;
		} else if (GTK_WIDGET_HAS_FOCUS (gth_file_view_get_widget (priv->file_list->view))) {
			window_update_sensitivity (browser);

			gtk_menu_popup (GTK_MENU (priv->file_popup_menu),
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
image_clicked_cb (GtkWidget  *widget,
		  GthBrowser *browser)
{
	gth_browser_show_next_image (browser, FALSE);
	return TRUE;
}


static gboolean
mouse_wheel_scrolled_cb (GtkWidget 		*widget,
  		   	 GdkScrollDirection 	 direction,
			 GthBrowser		*browser)
{
	if (direction == GDK_SCROLL_UP)
		gth_browser_show_prev_image (browser, FALSE);
	else
		gth_browser_show_next_image (browser, FALSE);

	return TRUE;
}


static int
image_button_press_cb (GtkWidget      *widget,
		       GdkEventButton *event,
		       gpointer        data)
{
	GthBrowser            *browser = data;
	GthBrowserPrivateData *priv = browser->priv;

	switch (event->button) {
	case 1:
		break;
	case 2:
		break;
	case 3:
		gtk_menu_popup (GTK_MENU (priv->image_popup_menu),
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
	GthBrowser *browser = data;

	switch (event->button) {
	case 2:
		gth_browser_show_prev_image (browser, FALSE);
		return TRUE;
	default:
		break;
	}

	return FALSE;
}


static int
image_comment_button_press_cb (GtkWidget      *widget,
			       GdkEventButton *event,
			       gpointer        data)
{
	GthBrowser *browser = data;

	if ((event->button == 1) && (event->type == GDK_2BUTTON_PRESS))
		if (gth_file_view_get_n_selected (browser->priv->file_list->view) > 0) {
			gth_window_edit_comment (GTH_WINDOW (browser));
			return TRUE;
		}
	return FALSE;
}


static gboolean
image_focus_changed_cb (GtkWidget     *widget,
			GdkEventFocus *event,
			gpointer       data)
{
	GthBrowser            *browser = data;
	GthBrowserPrivateData *priv = browser->priv;
	gboolean               viewer_visible;
	gboolean               data_visible;
	gboolean               comment_visible;

	viewer_visible  = ((priv->sidebar_visible
			    && priv->preview_visible
			    && (priv->preview_content == GTH_PREVIEW_CONTENT_IMAGE))
			   || ! priv->sidebar_visible);
	data_visible    = (priv->sidebar_visible
			   && priv->preview_visible
			   && (priv->preview_content == GTH_PREVIEW_CONTENT_DATA));
	comment_visible = (priv->sidebar_visible
			   && priv->preview_visible
			   && (priv->preview_content == GTH_PREVIEW_CONTENT_COMMENT));

	if (viewer_visible)
		gthumb_info_bar_set_focused (GTHUMB_INFO_BAR (priv->info_bar),
					     GTK_WIDGET_HAS_FOCUS (priv->viewer));
	else if (data_visible)
		gthumb_info_bar_set_focused (GTHUMB_INFO_BAR (priv->info_bar),
					     GTK_WIDGET_HAS_FOCUS (gth_exif_data_viewer_get_view (GTH_EXIF_DATA_VIEWER (priv->exif_data_viewer))));
	else if (comment_visible)
		gthumb_info_bar_set_focused (GTHUMB_INFO_BAR (priv->info_bar),
					     GTK_WIDGET_HAS_FOCUS (priv->image_comment));

	return FALSE;
}


/* -- drag & drop -- */


static void
viewer_drag_data_get  (GtkWidget        *widget,
		       GdkDragContext   *context,
		       GtkSelectionData *selection_data,
		       guint             info,
		       guint             time,
		       gpointer          data)
{
	GthBrowser            *browser = data;
	GthBrowserPrivateData *priv = browser->priv;
	char                  *path;

	if (IMAGE_VIEWER (priv->viewer)->is_void)
		return;

	path = image_viewer_get_image_filename (IMAGE_VIEWER (priv->viewer));

	gtk_selection_data_set (selection_data,
				selection_data->target,
				8,
				(unsigned char*) path, strlen (path));
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
	GthBrowser            *browser = extra_data;
	Catalog               *catalog;
	char                  *catalog_path;
	char                  *catalog_name, *catalog_name_utf8;
	GList                 *list;
	GList                 *scan;
	GError                *gerror = NULL;
	gboolean               empty = TRUE;

	if (! ((data->length >= 0) && (data->format == 8))) {
		gtk_drag_finish (context, FALSE, FALSE, time);
		return;
	}

	gtk_drag_finish (context, TRUE, FALSE, time);

	list = get_file_list_from_url_list ((char *) data->data);

	/* Create a catalog with the Drag&Drop list. */

	catalog = catalog_new ();
	catalog_name_utf8 = g_strconcat (_("Dragged Images"),
					 CATALOG_EXT,
					 NULL);
	catalog_name = gnome_vfs_escape_string (catalog_name_utf8);
	catalog_path = get_catalog_full_path (catalog_name);
	g_free (catalog_name);
	g_free (catalog_name_utf8);

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
			_gtk_error_dialog_from_gerror_run (GTK_WINDOW (browser), &gerror);
		else {
			/* View the Drag&Drop catalog. */
			ViewFirstImage = TRUE;
			gth_browser_go_to_catalog (browser, catalog_path);
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
	GthBrowser *browser = data;

	switch (gdk_keyval_to_lower (event->keyval)) {
	case GDK_Page_Up:
		gth_browser_show_prev_image (browser, FALSE);
		return TRUE;

	case GDK_Page_Down:
		gth_browser_show_next_image (browser, FALSE);
		return TRUE;

	case GDK_Home:
		gth_browser_show_first_image (browser, FALSE);
		return TRUE;

	case GDK_End:
		gth_browser_show_last_image (browser, FALSE);
		return TRUE;

	case GDK_F10:
		if (event->state & GDK_SHIFT_MASK) {
			gtk_menu_popup (GTK_MENU (browser->priv->image_popup_menu),
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
		     GthBrowser     *browser)
{
	GthBrowserPrivateData *priv = browser->priv;
	GtkWidget             *widget_to_focus = priv->viewer;

	if (priv->sidebar_visible)
		switch (priv->preview_content) {
		case GTH_PREVIEW_CONTENT_IMAGE:
			widget_to_focus = priv->viewer;
			break;
		case GTH_PREVIEW_CONTENT_DATA:
			widget_to_focus = gth_exif_data_viewer_get_view (GTH_EXIF_DATA_VIEWER (priv->exif_data_viewer));
			break;
		case GTH_PREVIEW_CONTENT_COMMENT:
			widget_to_focus = priv->image_comment;
			break;
		}

	gtk_widget_grab_focus (widget_to_focus);

	return TRUE;
}


static GString*
make_url_list (GList *list,
	       int    target)
{
	GList      *scan;
	GString    *result;

	if (list == NULL)
		return NULL;

	result = g_string_new (NULL);
	for (scan = list; scan; scan = scan->next) {
		char *url;

		switch (target) {
		case TARGET_PLAIN:
			url = g_locale_from_utf8 (scan->data, -1, NULL, NULL, NULL);
			if (url == NULL)
				continue;
			g_string_append (result, url);
			g_free (url);
			break;

		case TARGET_PLAIN_UTF8:
			g_string_append (result, scan->data);
			break;

		case TARGET_URILIST:
			url = add_scheme_if_absent (scan->data);
			if (url == NULL)
				continue;
			g_string_append (result, url);
			g_free (url);
			break;
		}

		g_string_append (result, "\r\n");
	}

	return result;
}


static void
gth_file_list_drag_begin (GtkWidget      *widget,
			  GdkDragContext *context,
			  gpointer        extra_data)
{
	GthBrowser *browser = extra_data;

	debug (DEBUG_INFO, "Gth::FileList::DragBegin");

	if (gth_file_list_drag_data != NULL)
		path_list_free (gth_file_list_drag_data);
	gth_file_list_drag_data = gth_file_view_get_file_list_selection (browser->priv->file_list->view);
}


static void
gth_file_list_drag_end (GtkWidget      *widget,
			GdkDragContext *context,
			gpointer        extra_data)
{
	debug (DEBUG_INFO, "Gth::FileList::DragEnd");

	if (gth_file_list_drag_data != NULL)
		path_list_free (gth_file_list_drag_data);
	gth_file_list_drag_data = NULL;
}


static void
gth_file_list_drag_data_get  (GtkWidget        *widget,
			      GdkDragContext   *context,
			      GtkSelectionData *selection_data,
			      guint             info,
			      guint             time,
			      gpointer          extra_data)
{
	GString *url_list;

	debug (DEBUG_INFO, "Gth::FileList::DragDataGet");

	url_list = make_url_list (gth_file_list_drag_data, info);
	if (url_list == NULL)
		return;

	gtk_selection_data_set (selection_data,
				selection_data->target,
				8,
				(unsigned char*)url_list->str,
				url_list->len);
	g_string_free (url_list, TRUE);
}


static gboolean
gth_file_list_drag_motion (GtkWidget          *widget,
			   GdkDragContext     *context,
			   gint                x,
			   gint                y,
			   guint               time,
			   gpointer            extra_data)
{
	GthBrowser            *browser = extra_data;
	GthBrowserPrivateData *priv = browser->priv;

	if (gtk_drag_get_source_widget (context) == priv->file_list->drag_source)
		gth_file_view_set_drag_dest_pos (priv->file_list->view, x, y);

	return TRUE;
}


static gboolean
gth_file_list_drag_leave (GtkWidget          *widget,
			  GdkDragContext     *context,
			  guint               time,
			  gpointer            extra_data)
{
	GthBrowser            *browser = extra_data;
	GthBrowserPrivateData *priv = browser->priv;

	if (gtk_drag_get_source_widget (context) == priv->file_list->root_widget)
		gth_file_view_set_drag_dest_pos (priv->file_list->view, -1, -1);

	return TRUE;
}


static void
move_items__continue (GnomeVFSResult result,
		      gpointer       data)
{
	GthBrowser *browser = data;

	if (result != GNOME_VFS_OK)
		_gtk_error_dialog_run (GTK_WINDOW (browser),
				       "%s %s",
				       _("Could not move the items:"),
				       gnome_vfs_result_to_string (result));
}


static void
add_image_list_to_catalog (GthBrowser *browser,
			   const char *catalog_path,
			   GList      *list)
{
	Catalog *catalog;
	GError  *gerror;

	if ((catalog_path == NULL) || ! path_is_file (catalog_path))
		return;

	catalog = catalog_new ();

	if (! catalog_load_from_disk (catalog, catalog_path, &gerror))
		_gtk_error_dialog_from_gerror_run (GTK_WINDOW (browser), &gerror);

	else {
		GList *scan;
		GList *files_added = NULL;

		for (scan = list; scan; scan = scan->next) {
			char *filename = scan->data;
			if (path_is_file (filename)) {
				catalog_add_item (catalog, filename);
				files_added = g_list_prepend (files_added, filename);
			}
		}

		if (! catalog_write_to_disk (catalog, &gerror))
			_gtk_error_dialog_from_gerror_run (GTK_WINDOW (browser), &gerror);
		else
			all_windows_notify_cat_files_created (catalog_path, files_added);

		g_list_free (files_added);
	}

	catalog_free (catalog);
}


static void
reorder_current_catalog (GthBrowser *browser,
			 int         pos)
{
	GthBrowserPrivateData *priv = browser->priv;
	GList                 *selection;

	if (pos < 0)
		return;

	selection = gth_file_list_get_selection (priv->file_list);
	if (selection == NULL)
		return;

	if ((priv->sidebar_content == GTH_SIDEBAR_CATALOG_LIST)
	    && (priv->catalog_path != NULL)) {
		Catalog *catalog = catalog_new ();
		GList   *list, *scan;

		if (file_is_search_result (priv->catalog_path))
			catalog_load_search_data_from_disk (catalog, priv->catalog_path, NULL);

		list = gth_file_list_get_all_from_view (priv->file_list);
		catalog_insert_items (catalog, list, 0);

		for (scan = selection; scan; scan = scan->next) {
			int item_pos = catalog_remove_item (catalog, scan->data);
			if (item_pos < pos)
				pos--;
		}

		catalog_insert_items (catalog, selection, pos);
		selection = NULL;

		catalog->sort_method = GTH_SORT_METHOD_MANUAL;
		catalog_set_path (catalog, priv->catalog_path);
		if (catalog_write_to_disk (catalog, NULL))
			all_windows_notify_catalog_reordered (priv->catalog_path);
		else {
			/* FIXME: error dialog? */
		}

		catalog_free (catalog);
	}

	path_list_free (selection);
}


static void
image_list_drag_data_received  (GtkWidget        *widget,
				GdkDragContext   *context,
				int               x,
				int               y,
				GtkSelectionData *data,
				guint             info,
				guint             time,
				gpointer          extra_data)
{
	GthBrowser            *browser = extra_data;
	GthBrowserPrivateData *priv = browser->priv;

	if (gtk_drag_get_source_widget (context) == priv->file_list->drag_source) {
		int pos;
		gth_file_view_get_drag_dest_pos (priv->file_list->view, &pos);
		gtk_drag_finish (context, FALSE, FALSE, time);
		reorder_current_catalog (browser, pos);
		return;
	}

	if (! ((data->length >= 0) && (data->format == 8))
	    || (((context->action & GDK_ACTION_COPY) != GDK_ACTION_COPY)
		&& ((context->action & GDK_ACTION_MOVE) != GDK_ACTION_MOVE))) {
		gtk_drag_finish (context, FALSE, FALSE, time);
		return;
	}

	gtk_drag_finish (context, TRUE, FALSE, time);

	if (priv->sidebar_content == GTH_SIDEBAR_DIR_LIST) {
		char *dest_dir = priv->dir_list->path;
		if (dest_dir != NULL) {
			GList *list;
			list = get_file_list_from_url_list ((char*) data->data);
			dlg_copy_items (GTH_WINDOW (browser),
					list,
					dest_dir,
					((context->action & GDK_ACTION_MOVE) == GDK_ACTION_MOVE),
					TRUE,
					FALSE,
					move_items__continue,
					browser);
			path_list_free (list);
		}

	} else if (priv->sidebar_content == GTH_SIDEBAR_CATALOG_LIST) {
		GList *list = get_file_list_from_url_list ((char*) data->data);
		add_image_list_to_catalog (browser, priv->catalog_path, list);
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
	GthBrowser              *browser = extra_data;
	GthBrowserPrivateData   *priv = browser->priv;
	GtkTreePath             *pos_path;
	GtkTreeViewDropPosition  drop_pos;
	int                      pos;
	char                    *dest_dir = NULL;
	const char              *current_dir;

	debug (DEBUG_INFO, "DirList::DragDataReceived");

	if ((data->length < 0)
	    || (data->format != 8)
	    || (((context->action & GDK_ACTION_COPY) != GDK_ACTION_COPY)
		&& ((context->action & GDK_ACTION_MOVE) != GDK_ACTION_MOVE))) {
		gtk_drag_finish (context, FALSE, FALSE, time);
		return;
	}

	gtk_drag_finish (context, TRUE, FALSE, time);

	g_return_if_fail (priv->sidebar_content == GTH_SIDEBAR_DIR_LIST);

	/**/

	if (gtk_tree_view_get_dest_row_at_pos (GTK_TREE_VIEW (priv->dir_list->list_view),
					       x, y,
					       &pos_path,
					       &drop_pos)) {
		pos = gtk_tree_path_get_indices (pos_path)[0];
		gtk_tree_path_free (pos_path);
	} else
		pos = -1;

	current_dir = priv->dir_list->path;

	if (pos == -1) {
		if (current_dir != NULL)
			dest_dir = g_strdup (current_dir);
	} else
		dest_dir = gth_dir_list_get_path_from_row (priv->dir_list, pos);

	/**/

	debug (DEBUG_INFO, "DirList::DragDataReceived: %s", dest_dir);

	if (dest_dir != NULL) {
		GList *list;
		list = get_file_list_from_url_list ((char*) data->data);

		dlg_copy_items (GTH_WINDOW (browser),
				list,
				dest_dir,
				((context->action & GDK_ACTION_MOVE) == GDK_ACTION_MOVE),
				TRUE,
				FALSE,
				move_items__continue,
				browser);
		path_list_free (list);
	}

	g_free (dest_dir);
}


static gboolean
auto_load_directory_cb (gpointer data)
{
	GthBrowser              *browser = data;
	GthBrowserPrivateData   *priv = browser->priv;
	GtkTreePath             *pos_path;
	GtkTreeViewDropPosition  drop_pos;
	GtkTreeView             *list_view;
	char                    *new_path;

	list_view = GTK_TREE_VIEW (priv->dir_list->list_view);

	g_source_remove (priv->auto_load_timeout);

	gtk_tree_view_get_drag_dest_row (list_view, &pos_path, &drop_pos);
	new_path = gth_dir_list_get_path_from_tree_path (priv->dir_list, pos_path);
	if (new_path)  {
		gth_browser_go_to_directory (browser, new_path);
		g_free(new_path);
	}

	gtk_tree_path_free (pos_path);

	priv->auto_load_timeout = 0;

	return FALSE;
}


static gboolean
dir_list_drag_motion (GtkWidget          *widget,
		      GdkDragContext     *context,
		      gint                x,
		      gint                y,
		      guint               time,
		      gpointer            extra_data)
{
	GthBrowser              *browser = extra_data;
	GthBrowserPrivateData   *priv = browser->priv;
	GtkTreePath             *pos_path;
	GtkTreeViewDropPosition  drop_pos;
	GtkTreeView             *list_view;

	debug (DEBUG_INFO, "DirList::DragMotion");

	list_view = GTK_TREE_VIEW (priv->dir_list->list_view);

	if (priv->auto_load_timeout != 0) {
		g_source_remove (priv->auto_load_timeout);
		priv->auto_load_timeout = 0;
	}

	if (! gtk_tree_view_get_dest_row_at_pos (list_view,
						 x, y,
						 &pos_path,
						 &drop_pos))
		pos_path = gtk_tree_path_new_first ();

	else
		priv->auto_load_timeout = g_timeout_add (AUTO_LOAD_DELAY,
							   auto_load_directory_cb,
							   browser);

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

	gtk_tree_view_set_drag_dest_row (list_view, pos_path, drop_pos);
	gtk_tree_path_free (pos_path);

	return TRUE;
}


static void
dir_list_drag_begin (GtkWidget          *widget,
		     GdkDragContext     *context,
		     gpointer            extra_data)
{
	GthBrowser            *browser = extra_data;
	GthBrowserPrivateData *priv = browser->priv;
	GtkTreeIter            iter;
	GtkTreeSelection      *selection;

	debug (DEBUG_INFO, "DirList::DragBegin");

	if (dir_list_tree_path != NULL) {
		gtk_tree_path_free (dir_list_tree_path);
		dir_list_tree_path = NULL;
	}

	if (dir_list_drag_data != NULL) {
		g_free (dir_list_drag_data);
		dir_list_drag_data = NULL;
	}

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->dir_list->list_view));
	if (! gtk_tree_selection_get_selected (selection, NULL, &iter))
		return;

	dir_list_drag_data = gth_dir_list_get_path_from_iter (priv->dir_list, &iter);
}


static void
dir_list_drag_end (GtkWidget          *widget,
		   GdkDragContext     *context,
		   gpointer            extra_data)
{
	if (dir_list_drag_data != NULL) {
		g_free (dir_list_drag_data);
		dir_list_drag_data = NULL;
	}
}


static void
dir_list_drag_leave (GtkWidget          *widget,
		     GdkDragContext     *context,
		     guint               time,
		     gpointer            extra_data)
{
	GthBrowser            *browser = extra_data;
	GthBrowserPrivateData *priv = browser->priv;
	GtkTreeView           *list_view;

	debug (DEBUG_INFO, "DirList::DragLeave");

	if (priv->auto_load_timeout != 0) {
		g_source_remove (priv->auto_load_timeout);
		priv->auto_load_timeout = 0;
	}

	list_view = GTK_TREE_VIEW (priv->dir_list->list_view);
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
	char *target;
	char *uri, *uri_data;

	debug (DEBUG_INFO, "DirList::DragDataGet");

	if (dir_list_drag_data == NULL)
		return;

	target = gdk_atom_name (selection_data->target);
	if (strcmp (target, "text/uri-list") != 0) {
		g_free (target);
		return;
	}
	g_free (target);

	uri = add_scheme_if_absent (dir_list_drag_data);
	uri_data = g_strconcat (uri, "\n", NULL);
	g_free (uri);

	gtk_selection_data_set (selection_data,
				selection_data->target,
				8,
				(unsigned char*)dir_list_drag_data,
				strlen (uri_data));

	g_free (dir_list_drag_data);
	dir_list_drag_data = NULL;
	g_free (uri_data);
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
	GthBrowser              *browser = extra_data;
	GthBrowserPrivateData   *priv = browser->priv;
	GtkTreePath             *pos_path;
	GtkTreeViewDropPosition  drop_pos;
	int                      pos;
	char                    *catalog_path = NULL;

	if (! ((data->length >= 0) && (data->format == 8))) {
		gtk_drag_finish (context, FALSE, FALSE, time);
		return;
	}

	gtk_drag_finish (context, TRUE, FALSE, time);

	g_return_if_fail (priv->sidebar_content == GTH_SIDEBAR_CATALOG_LIST);

	/**/

	if (gtk_tree_view_get_dest_row_at_pos (GTK_TREE_VIEW (priv->catalog_list->list_view),
					       x, y,
					       &pos_path,
					       &drop_pos)) {
		pos = gtk_tree_path_get_indices (pos_path)[0];
		gtk_tree_path_free (pos_path);
	} else
		pos = -1;

	if (pos == -1) {
		if (priv->catalog_path != NULL)
			catalog_path = g_strdup (priv->catalog_path);
	} else
		catalog_path = catalog_list_get_path_from_row (priv->catalog_list, pos);

	if (catalog_path != NULL) {
		GList *list = get_file_list_from_url_list ((char*) data->data);
		add_image_list_to_catalog (browser, catalog_path, list);
		path_list_free (list);
		g_free (catalog_path);
	}
}


static gboolean
auto_load_library_cb (gpointer data)
{
	GthBrowser              *browser = data;
	GthBrowserPrivateData   *priv = browser->priv;
	GtkTreePath             *pos_path;
	GtkTreeViewDropPosition  drop_pos;
	GtkTreeView             *list_view;
	char                    *new_path;

	list_view = GTK_TREE_VIEW (priv->catalog_list->list_view);

	g_source_remove (priv->auto_load_timeout);
	priv->auto_load_timeout = 0;

	gtk_tree_view_get_drag_dest_row (list_view, &pos_path, &drop_pos);
	new_path = catalog_list_get_path_from_tree_path (priv->catalog_list, pos_path);

	if (new_path)  {
		if (path_is_dir (new_path))
			gth_browser_go_to_catalog_directory (browser, new_path);
		g_free(new_path);
	}

	gtk_tree_path_free (pos_path);

	return FALSE;
}


static gboolean
catalog_list_drag_motion (GtkWidget          *widget,
			  GdkDragContext     *context,
			  gint                x,
			  gint                y,
			  guint               time,
			  gpointer            extra_data)
{
	GthBrowser              *browser = extra_data;
	GthBrowserPrivateData   *priv = browser->priv;
	GtkTreePath             *pos_path;
	GtkTreeViewDropPosition  drop_pos;
	GtkTreeView             *list_view;

	list_view = GTK_TREE_VIEW (priv->catalog_list->list_view);

	if (priv->auto_load_timeout != 0) {
		g_source_remove (priv->auto_load_timeout);
		priv->auto_load_timeout = 0;
	}

	if (! gtk_tree_view_get_dest_row_at_pos (list_view,
						 x, y,
						 &pos_path,
						 &drop_pos))
		pos_path = gtk_tree_path_new_first ();
	else
		priv->auto_load_timeout = g_timeout_add (AUTO_LOAD_DELAY,
							   auto_load_library_cb,
							   browser);

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
	GthBrowser            *browser = extra_data;
	GthBrowserPrivateData *priv = browser->priv;
	GtkTreeView           *list_view;

	if (priv->auto_load_timeout != 0) {
		g_source_remove (priv->auto_load_timeout);
		priv->auto_load_timeout = 0;
	}

	list_view = GTK_TREE_VIEW (priv->catalog_list->list_view);
	gtk_tree_view_set_drag_dest_row  (list_view, NULL, 0);
}


/* -- */


static void
gth_browser_remove_monitor (GthBrowser *browser)
{
	GthBrowserPrivateData *priv = browser->priv;

	if (priv->monitor_uri == NULL)
		return;

	gth_monitor_remove_uri (monitor, priv->monitor_uri);
	g_free (priv->monitor_uri);
	priv->monitor_uri = NULL;
}


static void
gth_browser_add_monitor (GthBrowser *browser)
{
	GthBrowserPrivateData *priv = browser->priv;

	if (priv->sidebar_content != GTH_SIDEBAR_DIR_LIST)
		return;

	gth_browser_remove_monitor (browser);

	if (priv->dir_list->path == NULL)
		return;

	if (priv->dir_list->path[0] == '/')
		priv->monitor_uri = add_scheme_if_absent (priv->dir_list->path);
	else
		priv->monitor_uri = g_strdup (priv->dir_list->path);
	gth_monitor_add_uri (monitor, priv->monitor_uri);
}


static void
activate_catalog_done (GthBrowser *browser)
{
	GthBrowserPrivateData *priv = browser->priv;

	/* Add to history list if not present as last entry. */

	if ((priv->go_op == GTH_BROWSER_GO_TO)
	    && ((priv->history_current == NULL)
		|| ((priv->catalog_path != NULL)
		    && (strcmp (priv->catalog_path, remove_host_from_uri (priv->history_current->data)) != 0)))) {
		GtkTreeIter iter;
		gboolean    is_search;

		if ((priv->catalog_path == NULL) 
		    || ! catalog_list_get_iter_from_path (priv->catalog_list,
						          priv->catalog_path,
						          &iter)) {
			window_image_viewer_set_void (browser);
			return;
		}
		is_search = catalog_list_is_search (priv->catalog_list, &iter);
		add_history_item (browser,
				  remove_host_from_uri (priv->catalog_path),
				  is_search ? SEARCH_PREFIX : CATALOG_PREFIX);
	} 
	else
		priv->go_op = GTH_BROWSER_GO_TO;

	window_update_history_list (browser);
	window_update_title (browser);
	window_make_current_image_visible (browser, TRUE);
}


static void
file_list_done_cb (GthFileList *file_list,
		   GthBrowser  *browser)
{
	GthBrowserPrivateData *priv = browser->priv;

	gth_file_view_set_no_image_text (file_list->view, _("No image"));

	gth_browser_stop_activity_mode (browser);
	window_update_statusbar_list_info (browser);
	window_update_title (browser);
	window_update_infobar (browser);
	window_update_sensitivity (browser);

	if (browser->priv->sidebar_content == GTH_SIDEBAR_CATALOG_LIST)
		activate_catalog_done (browser);

	/* selected the saved image */

	if (priv->image_path_saved != NULL) {
		int pos = gth_file_list_pos_from_path (priv->file_list, priv->image_path_saved);
		if (pos != -1) {
			view_image_at_pos (browser, pos);
			gth_file_list_select_image_by_pos (priv->file_list, pos);
		}
		g_free (priv->image_path_saved);
		priv->image_path_saved = NULL;
	}

	/**/

	if (StartInFullscreen) {
		StartInFullscreen = FALSE;
		gth_window_set_fullscreen (GTH_WINDOW (browser), TRUE);
	}

	if (StartSlideshow) {
		StartSlideshow = FALSE;
		gth_window_set_slideshow (GTH_WINDOW (browser), TRUE);
	}

	if (HideSidebar) {
		HideSidebar = FALSE;
		gth_browser_hide_sidebar (browser);
	}

	if (ImageToDisplay != NULL) {
		int pos = gth_file_list_pos_from_path (priv->file_list, ImageToDisplay);
		if (pos != -1)
			gth_file_view_set_cursor (priv->file_list->view, pos);
		g_free (ImageToDisplay);
		ImageToDisplay = NULL;
	}
	else if (ViewFirstImage) {
		ViewFirstImage = FALSE;
		gth_browser_show_first_image (browser, FALSE);
	}
	else if (priv->focus_current_image) {
		char *path = image_viewer_get_image_filename (IMAGE_VIEWER (priv->viewer));
		int   pos = gth_file_list_pos_from_path (priv->file_list, path);
		if (pos != -1) {
			gth_file_view_set_cursor (priv->file_list->view, pos);
			gth_file_list_select_image_by_pos (priv->file_list, pos);
		}
		g_free (path);
		priv->focus_current_image = FALSE;
	}

	if (FirstStart)
		FirstStart = FALSE;

	window_make_current_image_visible (browser, ! priv->load_image_folder_after_image);
	priv->refreshing = FALSE;
	priv->load_image_folder_after_image = FALSE;
	
	gth_browser_add_monitor (browser);

	set_cursor_not_busy (browser, TRUE);
}


static gboolean
progress_cancel_cb (GtkButton  *button,
		    GthBrowser *browser)
{
	if (browser->priv->pixop != NULL)
		gth_pixbuf_op_stop (browser->priv->pixop);
	return TRUE;
}


static gboolean
progress_delete_cb (GtkWidget  *caller,
		    GdkEvent   *event,
		    GthBrowser *browser)
{
	if (browser->priv->pixop != NULL)
		gth_pixbuf_op_stop (browser->priv->pixop);
	return TRUE;
}


static void
window_sync_menu_with_preferences (GthBrowser *browser)
{
	GthBrowserPrivateData *priv = browser->priv;
	char                  *prop = "TranspTypeNone";

	set_action_active (browser, "View_PlayAnimation", TRUE);

	switch (pref_get_zoom_quality ()) {
	case GTH_ZOOM_QUALITY_HIGH: prop = "View_ZoomQualityHigh"; break;
	case GTH_ZOOM_QUALITY_LOW:  prop = "View_ZoomQualityLow"; break;
	}
	set_action_active (browser, prop, TRUE);

	set_action_active (browser, "View_ShowFolders", FALSE);
	set_action_active (browser, "View_ShowCatalogs", FALSE);

	set_action_active (browser, "View_ShowPreview", eel_gconf_get_boolean (PREF_SHOW_PREVIEW, FALSE));
	set_action_active (browser, "View_ShowMetadata", eel_gconf_get_boolean (PREF_SHOW_IMAGE_DATA, FALSE));
	set_action_active (browser, "View_ShowHiddenFiles", eel_gconf_get_boolean (PREF_SHOW_HIDDEN_FILES, FALSE));

	window_sync_sort_menu (browser, priv->sort_method, priv->sort_type);
}


/* preferences change notification callbacks */


static gboolean
gth_browser_notify_update_layout_cb (gpointer data)
{
	GthBrowser   *browser = data;
	GthBrowserPrivateData *priv = browser->priv;
	GtkWidget    *paned1;      /* Main paned widget. */
	GtkWidget    *paned2;      /* Secondary paned widget. */
	int           paned1_pos;
	int           paned2_pos;
	gboolean      sidebar_visible;

	if (! GTK_IS_WIDGET (priv->main_pane))
		return TRUE;

	if (priv->update_layout_timeout != 0) {
		g_source_remove (priv->update_layout_timeout);
		priv->update_layout_timeout = 0;
	}

	sidebar_visible = priv->sidebar_visible;
	if (! sidebar_visible)
		gth_browser_show_sidebar (browser);

	priv->layout_type = eel_gconf_get_integer (PREF_UI_LAYOUT, 2);

	paned1_pos = gtk_paned_get_position (GTK_PANED (priv->main_pane));
	paned2_pos = gtk_paned_get_position (GTK_PANED (priv->content_pane));

	if (priv->layout_type == 1) {
		paned1 = gtk_vpaned_new ();
		paned2 = gtk_hpaned_new ();
	} else {
		paned1 = gtk_hpaned_new ();
		paned2 = gtk_vpaned_new ();
	}

	if (priv->layout_type == 3)
		gtk_paned_pack2 (GTK_PANED (paned1), paned2, TRUE, FALSE);
	else
		gtk_paned_pack1 (GTK_PANED (paned1), paned2, FALSE, FALSE);

	if (priv->layout_type == 3)
		gtk_widget_reparent (priv->dir_list_pane, paned1);
	else
		gtk_widget_reparent (priv->dir_list_pane, paned2);

	if (priv->layout_type == 2)
		gtk_widget_reparent (priv->file_list_pane, paned1);
	else
		gtk_widget_reparent (priv->file_list_pane, paned2);

	if (priv->layout_type <= 1)
		gtk_widget_reparent (priv->image_pane, paned1);
	else
		gtk_widget_reparent (priv->image_pane, paned2);

	gtk_paned_set_position (GTK_PANED (paned1), paned1_pos);
	gtk_paned_set_position (GTK_PANED (paned2), paned2_pos);

	gth_window_attach (GTH_WINDOW (browser), paned1, GTH_WINDOW_CONTENTS);

	gtk_widget_show (paned1);
	gtk_widget_show (paned2);

	priv->main_pane = paned1;
	priv->content_pane = paned2;

	if (! sidebar_visible)
		gth_browser_hide_sidebar (browser);

	return FALSE;
}


static void
gth_browser_notify_update_layout (GthBrowser *browser)
{
	GthBrowserPrivateData *priv = browser->priv;

	if (priv->update_layout_timeout != 0) {
		g_source_remove (priv->update_layout_timeout);
		priv->update_layout_timeout = 0;
	}

	priv->update_layout_timeout = g_timeout_add (UPDATE_LAYOUT_DELAY,
						       gth_browser_notify_update_layout_cb,
						       browser);
}


static void
pref_ui_layout_changed (GConfClient *client,
			guint        cnxn_id,
			GConfEntry  *entry,
			gpointer     user_data)
{
	GthBrowser *browser = user_data;
	gth_browser_notify_update_layout (browser);
}


static void
gth_browser_notify_update_toolbar_style (GthBrowser *browser)
{
	GthToolbarStyle toolbar_style;
	GtkToolbarStyle prop = GTK_TOOLBAR_BOTH;

	toolbar_style = pref_get_real_toolbar_style ();

	switch (toolbar_style) {
	case GTH_TOOLBAR_STYLE_TEXT_BELOW:
		prop = GTK_TOOLBAR_BOTH; break;
	case GTH_TOOLBAR_STYLE_TEXT_BESIDE:
		prop = GTK_TOOLBAR_BOTH_HORIZ; break;
	case GTH_TOOLBAR_STYLE_ICONS:
		prop = GTK_TOOLBAR_ICONS; break;
	case GTH_TOOLBAR_STYLE_TEXT:
		prop = GTK_TOOLBAR_TEXT; break;
	default:
		break;
	}

	gtk_toolbar_set_style (GTK_TOOLBAR (browser->priv->toolbar), prop);
}


static void
pref_ui_toolbar_style_changed (GConfClient *client,
			       guint        cnxn_id,
			       GConfEntry  *entry,
			       gpointer     user_data)
{
	GthBrowser *browser = user_data;
	gth_browser_notify_update_toolbar_style (browser);
}


static void
gth_browser_set_toolbar_visibility (GthBrowser *browser,
				    gboolean    visible)
{
	g_return_if_fail (browser != NULL);

	set_action_active (browser, "View_Toolbar", visible);
	if (visible)
		gtk_widget_show (browser->priv->toolbar);
	else
		gtk_widget_hide (browser->priv->toolbar);
}


static void
pref_ui_toolbar_visible_changed (GConfClient *client,
				 guint        cnxn_id,
				 GConfEntry  *entry,
				 gpointer     user_data)
{
	GthBrowser *browser = user_data;
	gth_browser_set_toolbar_visibility (browser, gconf_value_get_bool (gconf_entry_get_value (entry)));
}


static void
gth_browser_set_statusbar_visibility  (GthBrowser *browser,
				       gboolean    visible)
{
	g_return_if_fail (browser != NULL);

	set_action_active (browser, "View_Statusbar", visible);
	if (visible)
		gtk_widget_show (browser->priv->statusbar);
	else
		gtk_widget_hide (browser->priv->statusbar);
}


static void
pref_ui_statusbar_visible_changed (GConfClient *client,
				   guint        cnxn_id,
				   GConfEntry  *entry,
				   gpointer     user_data)
{
	GthBrowser *browser = user_data;
	gth_browser_set_statusbar_visibility (browser, gconf_value_get_bool (gconf_entry_get_value (entry)));
}


static void
pref_show_thumbnails_changed (GConfClient *client,
			      guint        cnxn_id,
			      GConfEntry  *entry,
			      gpointer     user_data)
{
	GthBrowser            *browser = user_data;
	GthBrowserPrivateData *priv = browser->priv;

	priv->file_list->enable_thumbs = eel_gconf_get_boolean (PREF_SHOW_THUMBNAILS, TRUE);
	window_enable_thumbs (browser, priv->file_list->enable_thumbs);
}


static void
pref_show_filenames_changed (GConfClient *client,
			     guint        cnxn_id,
			     GConfEntry  *entry,
			     gpointer     user_data)
{
	GthBrowser  *browser = user_data;
	gth_file_view_set_view_mode (browser->priv->file_list->view, pref_get_view_mode ());
}


static void
pref_show_comments_changed (GConfClient *client,
			    guint        cnxn_id,
			    GConfEntry  *entry,
			    gpointer     user_data)
{
	GthBrowser  *browser = user_data;
	gth_file_view_set_view_mode (browser->priv->file_list->view, pref_get_view_mode ());
}


static void
pref_show_hidden_files_changed (GConfClient *client,
				guint        cnxn_id,
				GConfEntry  *entry,
				gpointer     user_data)
{
	GthBrowser *browser = user_data;
	gboolean    show_hidden_files;
	
	show_hidden_files = eel_gconf_get_boolean (PREF_SHOW_HIDDEN_FILES, FALSE);
	if (show_hidden_files == browser->priv->show_hidden_files) 
		return;

	set_action_active (browser, "View_ShowHiddenFiles", show_hidden_files);
	browser->priv->show_hidden_files = show_hidden_files;
	gth_dir_list_show_hidden_files (browser->priv->dir_list, browser->priv->show_hidden_files);
	window_update_file_list (browser);
}


static void
pref_fast_file_type_changed (GConfClient *client,
			     guint        cnxn_id,
			     GConfEntry  *entry,
			     gpointer     user_data)
{
	GthBrowser *browser = user_data;
	
	browser->priv->fast_file_type = eel_gconf_get_boolean (PREF_FAST_FILE_TYPE, TRUE);
	window_update_file_list (browser);
}


static void
pref_thumbnail_size_changed (GConfClient *client,
			     guint        cnxn_id,
			     GConfEntry  *entry,
			     gpointer     user_data)
{
	GthBrowser *browser = user_data;
	gth_file_list_set_thumbs_size (browser->priv->file_list, eel_gconf_get_integer (PREF_THUMBNAIL_SIZE, 95));
}


static void
pref_click_policy_changed (GConfClient *client,
			   guint        cnxn_id,
			   GConfEntry  *entry,
			   gpointer     user_data)
{
	GthBrowser *browser = user_data;
	gth_dir_list_update_underline (browser->priv->dir_list);
	catalog_list_update_click_policy (browser->priv->catalog_list);
}


static void
pref_zoom_quality_changed (GConfClient *client,
			   guint        cnxn_id,
			   GConfEntry  *entry,
			   gpointer     user_data)
{
	GthBrowser  *browser = user_data;
	ImageViewer *image_viewer = IMAGE_VIEWER (browser->priv->viewer);

	if (pref_get_zoom_quality () == GTH_ZOOM_QUALITY_HIGH) {
		set_action_active_if_different (browser, "View_ZoomQualityHigh", 1);
		image_viewer_set_zoom_quality (image_viewer, GTH_ZOOM_QUALITY_HIGH);
	} else {
		set_action_active_if_different (browser, "View_ZoomQualityLow", 1);
		image_viewer_set_zoom_quality (image_viewer, GTH_ZOOM_QUALITY_LOW);
	}

	image_viewer_update_view (image_viewer);
}


static void
pref_zoom_change_changed (GConfClient *client,
			  guint        cnxn_id,
			  GConfEntry  *entry,
			  gpointer     user_data)
{
	GthBrowser  *browser = user_data;
	ImageViewer *image_viewer = IMAGE_VIEWER (browser->priv->viewer);

	image_viewer_set_zoom_change (image_viewer, pref_get_zoom_change ());
	image_viewer_update_view (image_viewer);
}


static void
pref_transp_type_changed (GConfClient *client,
			  guint        cnxn_id,
			  GConfEntry  *entry,
			  gpointer     user_data)
{
	GthBrowser  *browser = user_data;
	ImageViewer *image_viewer = IMAGE_VIEWER (browser->priv->viewer);

	image_viewer_set_transp_type (image_viewer, pref_get_transp_type ());
	image_viewer_update_view (image_viewer);
}


static void
pref_check_type_changed (GConfClient *client,
			 guint        cnxn_id,
			 GConfEntry  *entry,
			 gpointer     user_data)
{
	GthBrowser  *browser = user_data;
	ImageViewer *image_viewer = IMAGE_VIEWER (browser->priv->viewer);

	image_viewer_set_check_type (image_viewer, pref_get_check_type ());
	image_viewer_update_view (image_viewer);
}


static void
pref_check_size_changed (GConfClient *client,
			 guint        cnxn_id,
			 GConfEntry  *entry,
			 gpointer     user_data)
{
	GthBrowser  *browser = user_data;
	ImageViewer *image_viewer = IMAGE_VIEWER (browser->priv->viewer);

	image_viewer_set_check_size (image_viewer, pref_get_check_size ());
	image_viewer_update_view (image_viewer);
}


static void
pref_black_background_changed (GConfClient *client,
			       guint        cnxn_id,
			       GConfEntry  *entry,
			       gpointer     user_data)
{
	GthBrowser  *browser = user_data;
	ImageViewer *image_viewer = IMAGE_VIEWER (browser->priv->viewer);

	image_viewer_set_black_background (image_viewer,
					   eel_gconf_get_boolean (PREF_BLACK_BACKGROUND, FALSE));
}


static void
pref_reset_scrollbars_changed (GConfClient *client,
			       guint        cnxn_id,
			       GConfEntry  *entry,
			       gpointer     user_data)
{
	GthBrowser  *browser = user_data;
	ImageViewer *image_viewer = IMAGE_VIEWER (browser->priv->viewer);

	image_viewer_set_reset_scrollbars (image_viewer, eel_gconf_get_boolean (PREF_RESET_SCROLLBARS, TRUE));
}


static GthFileList *
create_new_file_list (GthBrowser *browser)
{
	GthFileList *file_list;
	GtkWidget   *view_widget;

	file_list = gth_file_list_new ();
	view_widget = gth_file_view_get_widget (file_list->view);

	gth_file_view_set_reorderable (file_list->view, (browser->priv->sidebar_content == GTH_SIDEBAR_CATALOG_LIST));

	gtk_widget_set_size_request (file_list->root_widget,
				     PANE_MIN_SIZE,
				     PANE_MIN_SIZE);
	/* gth_file_list_set_progress_func (file_list, window_progress, browser); FIXME */

	gtk_drag_dest_set (file_list->root_widget,
			   GTK_DEST_DEFAULT_ALL,
			   target_table, G_N_ELEMENTS (target_table),
			   GDK_ACTION_COPY);
	gtk_drag_source_set (file_list->drag_source,
			     GDK_BUTTON1_MASK,
			     target_table, G_N_ELEMENTS (target_table),
			     GDK_ACTION_MOVE | GDK_ACTION_COPY);
	/* FIXME
	gtk_drag_dest_set (file_list->root_widget,
			   GTK_DEST_DEFAULT_ALL,
			   same_app_target_table, G_N_ELEMENTS (same_app_target_table),
			   GDK_ACTION_MOVE);

	gtk_drag_source_set (file_list->root_widget,
			     GDK_BUTTON1_MASK,
			     same_app_target_table, G_N_ELEMENTS (same_app_target_table),
			     GDK_ACTION_MOVE);*/
	g_signal_connect (G_OBJECT (file_list->view),
			  "selection_changed",
			  G_CALLBACK (file_selection_changed_cb),
			  browser);
	g_signal_connect (G_OBJECT (file_list->view),
			  "cursor_changed",
			  G_CALLBACK (gth_file_list_cursor_changed_cb),
			  browser);
	g_signal_connect (G_OBJECT (file_list->view),
			  "item_activated",
			  G_CALLBACK (gth_file_list_item_activated_cb),
			  browser);

	/* FIXME
	g_signal_connect_after (G_OBJECT (view_widget),
				"button_press_event",
				G_CALLBACK (gth_file_list_button_press_cb),
				browser);
	*/

	g_signal_connect (G_OBJECT (view_widget),
			  "button_press_event",
			  G_CALLBACK (gth_file_list_button_press_cb),
			  browser);

	g_signal_connect (G_OBJECT (file_list->root_widget),
			  "drag_data_received",
			  G_CALLBACK (image_list_drag_data_received),
			  browser);
	g_signal_connect (G_OBJECT (file_list->drag_source),
			  "drag_begin",
			  G_CALLBACK (gth_file_list_drag_begin),
			  browser);
	g_signal_connect (G_OBJECT (file_list->drag_source),
			  "drag_end",
			  G_CALLBACK (gth_file_list_drag_end),
			  browser);
	g_signal_connect (G_OBJECT (file_list->drag_source),
			  "drag_data_get",
			  G_CALLBACK (gth_file_list_drag_data_get),
			  browser);
	g_signal_connect (G_OBJECT (file_list->root_widget),
			  "drag_motion",
			  G_CALLBACK (gth_file_list_drag_motion),
			  browser);
	g_signal_connect (G_OBJECT (file_list->root_widget),
			  "drag_leave",
			  G_CALLBACK (gth_file_list_drag_leave),
			  browser);

	g_signal_connect (G_OBJECT (file_list),
			  "done",
			  G_CALLBACK (file_list_done_cb),
			  browser);

	return file_list;
}


static void
pref_view_as_changed (GConfClient *client,
		      guint        cnxn_id,
		      GConfEntry  *entry,
		      gpointer     user_data)
{
	GthBrowser            *browser = user_data;
	GthBrowserPrivateData *priv = browser->priv;
	GthFileList           *file_list;
	gboolean               enable_thumbs;
	GthFilter             *filter;

	enable_thumbs = priv->file_list->enable_thumbs;

	/**/

	file_list = create_new_file_list (browser);
	gth_file_list_set_sort_method (file_list, priv->sort_method, FALSE);
	gth_file_list_set_sort_type (file_list, priv->sort_type, FALSE);
	gth_file_list_enable_thumbs (file_list, enable_thumbs, FALSE);

	gtk_widget_destroy (priv->file_list->root_widget);
	gtk_box_pack_start (GTK_BOX (priv->file_list_pane), file_list->root_widget, TRUE, TRUE, 0);

	g_object_unref (priv->file_list);
	priv->file_list = file_list;

	gtk_widget_show_all (priv->file_list->root_widget);

	filter = gth_filter_bar_get_filter (GTH_FILTER_BAR (browser->priv->filterbar));
	gth_file_list_set_filter (browser->priv->file_list, filter);
	if (filter != NULL)
		g_object_unref (filter);

	window_update_file_list (browser);
}


void
gth_browser_set_preview_content (GthBrowser        *browser,
				 GthPreviewContent  content)
{
	GthBrowserPrivateData *priv = browser->priv;
	GtkWidget             *widget_to_focus = priv->viewer;

	priv->preview_content = content;

	set_button_active_no_notify (browser, priv->preview_button_image, FALSE);
	set_button_active_no_notify (browser, priv->preview_button_data, FALSE);
	set_button_active_no_notify (browser, priv->preview_button_comment, FALSE);

	if (priv->preview_content == GTH_PREVIEW_CONTENT_IMAGE) {
		gtk_widget_hide (priv->preview_widget_data);
		gtk_widget_hide (priv->preview_widget_comment);
		gtk_widget_hide (priv->preview_widget_data_comment);
		gtk_widget_show (priv->preview_widget_image);
		set_button_active_no_notify (browser, priv->preview_button_image, TRUE);

	} else if (priv->preview_content == GTH_PREVIEW_CONTENT_DATA) {
		gtk_widget_hide (priv->preview_widget_image);
		gtk_widget_hide (priv->preview_widget_comment);
		gtk_widget_show (priv->preview_widget_data);
		gtk_widget_show (priv->preview_widget_data_comment);
		set_button_active_no_notify (browser, priv->preview_button_data, TRUE);

	} else if (priv->preview_content == GTH_PREVIEW_CONTENT_COMMENT) {
		gtk_widget_hide (priv->preview_widget_image);
		gtk_widget_hide (priv->preview_widget_data);
		gtk_widget_show (priv->preview_widget_comment);
		gtk_widget_show (priv->preview_widget_data_comment);
		set_button_active_no_notify (browser, priv->preview_button_comment, TRUE);
	}

	switch (priv->preview_content) {
	case GTH_PREVIEW_CONTENT_IMAGE:
		widget_to_focus = priv->viewer;
		break;
	case GTH_PREVIEW_CONTENT_DATA:
		widget_to_focus = gth_exif_data_viewer_get_view (GTH_EXIF_DATA_VIEWER (priv->exif_data_viewer));
		break;
	case GTH_PREVIEW_CONTENT_COMMENT:
		widget_to_focus = priv->image_comment;
		break;
	}

	gtk_widget_grab_focus (widget_to_focus);

	window_update_statusbar_zoom_info (browser);
	window_update_sensitivity (browser);
}


static void
close_preview_image_button_cb (GtkToggleButton *button,
			       GthBrowser      *browser)
{
	if (browser->priv->sidebar_visible) {
		if (browser->priv->image_pane_visible)
			gth_browser_hide_image_pane (browser);
	} else
		gth_browser_show_sidebar (browser);
}


static void
preview_button_cb (GthBrowser        *browser,
		   GtkToggleButton   *button,
		   GthPreviewContent  content)
{
	if (gtk_toggle_button_get_active (button))
		gth_browser_set_preview_content (browser, content);
	else if (browser->priv->preview_content == content)
		gtk_toggle_button_set_active (button, TRUE);
}


static void
preview_image_button_cb (GtkToggleButton *button,
			 GthBrowser      *browser)
{
	preview_button_cb (browser, button, GTH_PREVIEW_CONTENT_IMAGE);
}


static void
preview_data_button_cb (GtkToggleButton *button,
			GthBrowser      *browser)
{
	if (browser->priv->sidebar_visible) {
		preview_button_cb (browser, button, GTH_PREVIEW_CONTENT_DATA);
	} else {
		if (gtk_toggle_button_get_active (button))
			gth_browser_show_image_data (browser);
		else
			gth_browser_hide_image_data (browser);
	}
}


static void
preview_comment_button_cb (GtkToggleButton *button,
			   GthBrowser    *browser)
{
	preview_button_cb (browser, button, GTH_PREVIEW_CONTENT_COMMENT);
}


static gboolean
initial_location_cb (gpointer data)
{
	GthBrowser *browser = data;
	go_to_uri (browser, browser->priv->initial_location, TRUE);
	gtk_widget_grab_focus (gth_file_view_get_widget (browser->priv->file_list->view));
	return FALSE;
}


static void
menu_item_select_cb (GtkMenuItem *proxy,
		     GthBrowser  *browser)
{
	GtkAction *action;
	char      *message;

	action = g_object_get_data (G_OBJECT (proxy),  "gtk-action");
	g_return_if_fail (action != NULL);

	g_object_get (G_OBJECT (action), "tooltip", &message, NULL);
	if (message != NULL) {
		gtk_statusbar_push (GTK_STATUSBAR (browser->priv->statusbar),
				    browser->priv->help_message_cid, message);
		g_free (message);
	}
}


static void
menu_item_deselect_cb (GtkMenuItem *proxy,
		       GthBrowser  *browser)
{
	gtk_statusbar_pop (GTK_STATUSBAR (browser->priv->statusbar),
			   browser->priv->help_message_cid);
}


static void
disconnect_proxy_cb (GtkUIManager *manager,
		     GtkAction    *action,
		     GtkWidget    *proxy,
		     GthBrowser   *browser)
{
	if (GTK_IS_MENU_ITEM (proxy)) {
		g_signal_handlers_disconnect_by_func
			(proxy, G_CALLBACK (menu_item_select_cb), browser);
		g_signal_handlers_disconnect_by_func
			(proxy, G_CALLBACK (menu_item_deselect_cb), browser);
	}
}


static void
connect_proxy_cb (GtkUIManager *manager,
		  GtkAction    *action,
		  GtkWidget    *proxy,
		  GthBrowser   *browser)
{
	if (GTK_IS_MENU_ITEM (proxy)) {
		g_signal_connect (proxy, "select",
				  G_CALLBACK (menu_item_select_cb), browser);
		g_signal_connect (proxy, "deselect",
				  G_CALLBACK (menu_item_deselect_cb), browser);
	}
}


static void
sort_by_radio_action (GtkAction      *action,
		      GtkRadioAction *current,
		      gpointer        data)
{
	GthBrowser            *browser = data;
	GthBrowserPrivateData *priv = browser->priv;
	GthSortMethod          sort_method;

	sort_method = gtk_radio_action_get_current_value (current);

	if ((priv->sidebar_content == GTH_SIDEBAR_CATALOG_LIST)
	    && (priv->catalog_path != NULL)) {
		Catalog *catalog = catalog_new ();
		if (catalog_load_from_disk (catalog,
					    priv->catalog_path,
					    NULL)) {
			if ( !((catalog->sort_method == GTH_SORT_METHOD_NONE)
			       && (sort_method == priv->sort_method))
			     && (catalog->sort_method != sort_method)) {
				catalog->sort_method = sort_method;
				catalog_write_to_disk (catalog, NULL);
			}
		}
	} 
	else if (priv->sidebar_content == GTH_SIDEBAR_DIR_LIST)
		priv->sort_method = sort_method;

	gth_file_list_set_sort_method (browser->priv->file_list, sort_method, TRUE);
}


static gboolean
view_as_cb (gpointer data)
{
	GthBrowser            *browser = data;
	GthBrowserPrivateData *priv = browser->priv;

	g_source_remove (priv->view_as_timeout);
	pref_set_view_as (priv->new_view_type);
	priv->view_as_timeout = 0;

	return FALSE;
}


static void
view_as_radio_action (GtkAction      *action,
		      GtkRadioAction *current,
		      gpointer        data)
{
	GthBrowser            *browser = data;
	GthBrowserPrivateData *priv = browser->priv;
	GthViewAs              view_as_choice;

	view_as_choice = gtk_radio_action_get_current_value (current);

	if (priv->view_as_timeout != 0)
		g_source_remove (priv->view_as_timeout);
	priv->new_view_type = view_as_choice;
	priv->view_as_timeout = g_timeout_add (VIEW_AS_DELAY, view_as_cb, data);
}


void
gth_browser_set_sort_type (GthBrowser  *browser,
			   GtkSortType  sort_type)
{
	GthBrowserPrivateData *priv = browser->priv;

	priv->sort_type = sort_type;

	/* FIXME: save catalog order. */

	gth_file_list_set_sort_type (priv->file_list, sort_type, TRUE);
}


static void
zoom_quality_radio_action (GtkAction      *action,
			   GtkRadioAction *current,
			   gpointer        data)
{
	GthBrowser     *browser = data;
	ImageViewer    *image_viewer = IMAGE_VIEWER (browser->priv->viewer);
	GthZoomQuality  quality;

	quality = gtk_radio_action_get_current_value (current);
	gtk_radio_action_get_current_value (current);
	image_viewer_set_zoom_quality (image_viewer, quality);
	image_viewer_update_view (image_viewer);
	pref_set_zoom_quality (quality);
}


static void
add_rotate_toolbar_item (GthBrowser *browser)
{
	GthBrowserPrivateData *priv = browser->priv;

	priv->sep_rotate_tool_item = gtk_separator_tool_item_new ();
	gtk_widget_show (GTK_WIDGET (priv->sep_rotate_tool_item));
	gtk_toolbar_insert (GTK_TOOLBAR (priv->toolbar),
			    priv->sep_rotate_tool_item,
			    ROTATE_TOOLITEM_POS);

	if (priv->rotate_tool_item != NULL) {
		gtk_toolbar_insert (GTK_TOOLBAR (priv->toolbar),
				    priv->rotate_tool_item,
				    ROTATE_TOOLITEM_POS + 1);
		return;
	}

	priv->rotate_tool_item = gtk_menu_tool_button_new_from_stock (GTHUMB_STOCK_TRANSFORM);
	g_object_ref (priv->rotate_tool_item);
	gtk_menu_tool_button_set_menu (GTK_MENU_TOOL_BUTTON (priv->rotate_tool_item),
				       gtk_ui_manager_get_widget (priv->ui, "/RotateImageMenu"));
	gtk_tool_item_set_homogeneous (priv->rotate_tool_item, FALSE);
	gtk_tool_item_set_tooltip (priv->rotate_tool_item, priv->tooltips, _("Rotate images without loss of quality"), NULL);
	gtk_menu_tool_button_set_arrow_tooltip (GTK_MENU_TOOL_BUTTON (priv->rotate_tool_item), priv->tooltips,	_("Rotate images without loss of quality"), NULL);
	gtk_action_connect_proxy (gtk_ui_manager_get_action (priv->ui, "/MenuBar/Tools/Tools_JPEGRotate"),
				  GTK_WIDGET (priv->rotate_tool_item));
	gtk_widget_show (GTK_WIDGET (priv->rotate_tool_item));
	gtk_toolbar_insert (GTK_TOOLBAR (priv->toolbar),
			    priv->rotate_tool_item,
			    ROTATE_TOOLITEM_POS + 1);
}


static void
remove_custom_tool_item (GthBrowser  *browser,
			 GtkToolItem *tool_item)
{
	if (tool_item == NULL)
		return;
	if (gtk_widget_get_parent ((GtkWidget*) tool_item) != NULL)
		gtk_container_remove (GTK_CONTAINER (browser->priv->toolbar),
				      (GtkWidget*) tool_item);
}


static void
set_mode_specific_ui_info (GthBrowser        *browser,
			   GthSidebarContent  content,
			   gboolean           first_time)
{
	GthBrowserPrivateData *priv = browser->priv;

	remove_custom_tool_item (browser, priv->sep_rotate_tool_item);
	priv->sep_rotate_tool_item = NULL;
	remove_custom_tool_item (browser, priv->rotate_tool_item);
	if (priv->toolbar_merge_id != 0)
		gtk_ui_manager_remove_ui (priv->ui, priv->toolbar_merge_id);
	gtk_ui_manager_ensure_update (priv->ui);

	if (content != GTH_SIDEBAR_NO_LIST) {
		if (!first_time)
			gth_browser_set_sidebar_content (browser, content);
		priv->toolbar_merge_id = gtk_ui_manager_add_ui_from_string (priv->ui, browser_ui_info, -1, NULL);
		gtk_ui_manager_ensure_update (priv->ui);
		add_rotate_toolbar_item (browser);

		set_action_important (browser, "View_ShowFolders", TRUE);
		set_action_important (browser, "View_ShowCatalogs", TRUE);
		set_action_important (browser, "View_Fullscreen", TRUE);
		set_action_important (browser, "Tools_Slideshow", TRUE);
	}
	else {
		if (!first_time)
			gth_browser_hide_sidebar (browser);
		priv->toolbar_merge_id = gtk_ui_manager_add_ui_from_string (priv->ui, viewer_ui_info, -1, NULL);
		gtk_ui_manager_ensure_update (priv->ui);

		set_action_important (browser, "View_Fullscreen", TRUE);
		set_action_important (browser, "View_CloseImageMode", TRUE);
		set_action_important (browser, "File_Save", TRUE);
		set_action_important (browser, "View_ShowMetadata", TRUE);
	}

	gtk_ui_manager_ensure_update (priv->ui);
}


static void
content_radio_action (GtkAction      *action,
		      GtkRadioAction *current,
		      gpointer        data)
{
	GthBrowser        *browser = data;
	GthSidebarContent  content = gtk_radio_action_get_current_value (current);

	set_mode_specific_ui_info (browser, content, FALSE);
}


static void
gth_browser_show (GtkWidget  *widget)
{
	GthBrowser            *browser = GTH_BROWSER (widget);
	GthBrowserPrivateData *priv = browser->priv;

	GTK_WIDGET_CLASS (parent_class)->show (widget);

	if (!browser->priv->first_time_show)
		return;

	browser->priv->first_time_show = FALSE;

	/* toolbar */

	if (eel_gconf_get_boolean (PREF_UI_TOOLBAR_VISIBLE, TRUE))
		gtk_widget_show (priv->toolbar);
	else
		gtk_widget_hide (priv->toolbar);

	set_action_active (browser,
			   "View_Toolbar",
			   eel_gconf_get_boolean (PREF_UI_TOOLBAR_VISIBLE, TRUE));

	/* statusbar */

	if (eel_gconf_get_boolean (PREF_UI_STATUSBAR_VISIBLE, TRUE))
		gtk_widget_show (priv->statusbar);
	else
		gtk_widget_hide (priv->statusbar);

	set_action_active (browser,
			   "View_Statusbar",
			   eel_gconf_get_boolean (PREF_UI_STATUSBAR_VISIBLE, TRUE));

	/* filterbar */

	if (eel_gconf_get_boolean (PREF_UI_FILTERBAR_VISIBLE, TRUE))
		gtk_widget_show (priv->filterbar);
	else
		gtk_widget_hide (priv->filterbar);

	set_action_active (browser,
			   "View_Filterbar",
			   eel_gconf_get_boolean (PREF_UI_FILTERBAR_VISIBLE, TRUE));
}


static void
gth_browser_finalize (GObject *object)
{
	GthBrowser *browser = GTH_BROWSER (object);

	debug (DEBUG_INFO, "Gth::Browser::Finalize");

	if (browser->priv != NULL) {
		GthBrowserPrivateData *priv = browser->priv;

		g_object_unref (priv->file_list);
		g_object_unref (priv->dir_list);
		catalog_list_free (priv->catalog_list);

		if (priv->catalog_path) {
			g_free (priv->catalog_path);
			priv->catalog_path = NULL;
		}

		if (priv->image) {
			file_data_unref (priv->image);
			priv->image = NULL;
		}

		if (priv->image_path_saved) {
			g_free (priv->image_path_saved);
			priv->image_path_saved = NULL;
		}

		if (priv->exif_data != NULL) {
			exif_data_unref (priv->exif_data);
			priv->exif_data = NULL;
		}

#ifdef HAVE_LIBIPTCDATA
		if (priv->iptc_data != NULL) {
			iptc_data_unref (priv->iptc_data);
			priv->iptc_data = NULL;
		}
#endif /* HAVE_LIBIPTCDATA */

		if (priv->new_image) {
			file_data_unref (priv->new_image);
			priv->new_image = NULL;
		}

		if (priv->image_catalog) {
			g_free (priv->image_catalog);
			priv->image_catalog = NULL;
		}

		if (priv->history) {
			bookmarks_free (priv->history);
			priv->history = NULL;
		}

		if (priv->preloader) {
			g_object_unref (priv->preloader);
			priv->preloader = NULL;
		}

		g_free (browser->priv);
		browser->priv = NULL;
	}

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gth_browser_init (GthBrowser *browser)
{
	GthBrowserPrivateData *priv;

	priv = browser->priv = g_new0 (GthBrowserPrivateData, 1);
	priv->first_time_show = TRUE;
	priv->sort_method = pref_get_arrange_type ();
	priv->sort_type = pref_get_sort_order ();
}


/* -- changes notification functions -- */


static void
gth_browser_notify_files_created (GthBrowser *browser,
				  GList      *list)
{
	GthBrowserPrivateData *priv = browser->priv;
	char                  *current_dir;
	GList                 *created_in_current_dir = NULL;
	GList                 *scan;
	
	if (priv->sidebar_content != GTH_SIDEBAR_DIR_LIST)
		return;

	current_dir = add_scheme_if_absent (priv->dir_list->path);
	if (current_dir == NULL)
		return;

	for (scan = list; scan; scan = scan->next) {
		char *path = scan->data;
		char *parent_dir;

		parent_dir = remove_level_from_path (path);
		if (parent_dir == NULL)
			continue;

		if (same_uri (parent_dir, current_dir)) {
			FileData *file;
			
			file = file_data_new (path, NULL);
			file_data_update_all (file, browser->priv->fast_file_type);
			if (file_filter (file, browser->priv->show_hidden_files))
				created_in_current_dir = g_list_prepend (created_in_current_dir, file);
			else
				file_data_unref (file);
		}

		g_free (parent_dir);
	}

	if (created_in_current_dir != NULL) {
		gth_file_list_add_list (browser->priv->file_list, created_in_current_dir);
		file_data_list_free (created_in_current_dir);
	}
}


static void
gth_browser_notify_files_deleted (GthBrowser *browser,
				  GList      *list)
{
	GthBrowserPrivateData *priv = browser->priv;

	g_return_if_fail (browser != NULL);

	if ((priv->sidebar_content == GTH_SIDEBAR_CATALOG_LIST)
	    && (priv->catalog_path != NULL)) { /* update the catalog. */
		Catalog *catalog;
		GList   *scan;

		catalog = catalog_new ();
		if (catalog_load_from_disk (catalog, priv->catalog_path, NULL)) {
			for (scan = list; scan; scan = scan->next)
				catalog_remove_item (catalog, (char*) scan->data);
			catalog_write_to_disk (catalog, NULL);
		}
		catalog_free (catalog);
	}

	gth_file_list_delete_list (browser->priv->file_list, list);
}


static void
gth_browser_notify_files_changed (GthBrowser *browser,
				  GList      *list)
{
	GthBrowserPrivateData *priv = browser->priv;
	GList                 *absent_files = NULL, *scan;

	gth_file_list_update_thumb_list (priv->file_list, list);

	/* update the current image if has changed. */

	if ((priv->image != NULL)
	    && ! priv->image_modified
	    && (path_list_find_path (list, priv->image->path) != NULL)) {
		int pos;
		
		pos = gth_file_list_pos_from_path (priv->file_list, priv->image->path);
		if (pos != -1)
			view_image_at_pos (browser, pos);
	}

	/* add to the file list the changed files not included in the list. */

	for (scan = list; scan; scan = scan->next) {
		char     *filename = scan->data;
		FileData *fd;
		
		fd = gth_file_list_filedata_from_path (priv->file_list, filename); 
		if (fd != NULL) {
			file_data_unref (fd);
			continue;
		}

		absent_files = g_list_prepend (absent_files, filename);
	}

	if (absent_files != NULL) {
		gth_browser_notify_files_created (browser, absent_files);
		g_list_free (absent_files);
	}
}


static void
monitor_update_files_cb (GthMonitor      *monitor,
			 GthMonitorEvent  event,
			 GList           *list,
			 GthBrowser      *browser)
{
	g_return_if_fail (browser != NULL);

	GList *scan;
	char *event_name[] = {"CREATED", "DELETED", "CHANGED", "RENAMED"};
	debug (DEBUG_INFO, "%s:\n", event_name[event]);

	for (scan = list; scan; scan = scan->next) {
		char *filename = scan->data;
		debug (DEBUG_INFO, "%s\n", filename);
	}

	switch (event) {
	case GTH_MONITOR_EVENT_CREATED:
		gth_browser_notify_files_created (browser, list);
		break;

	case GTH_MONITOR_EVENT_DELETED:
		gth_browser_notify_files_deleted (browser, list);
		break;

	case GTH_MONITOR_EVENT_CHANGED:
		gth_browser_notify_files_changed (browser, list);
		break;

	default:
		break;
	}
}


static void
monitor_update_cat_files_cb (GthMonitor      *monitor,
			     const char      *catalog_name,
			     GthMonitorEvent  event,
			     GList           *list,
			     GthBrowser      *browser)
{
	GList *fd_list;
	
	g_return_if_fail (browser != NULL);

	if (browser->priv->sidebar_content != GTH_SIDEBAR_CATALOG_LIST)
		return;
	if (browser->priv->catalog_path == NULL)
		return;
	if (! same_uri (browser->priv->catalog_path, catalog_name))
		return;

	switch (event) {
	case GTH_MONITOR_EVENT_CREATED:
		fd_list = file_data_list_from_uri_list (list);
		gth_file_list_add_list (browser->priv->file_list, fd_list);
		file_data_list_free (fd_list);
		break;

	case GTH_MONITOR_EVENT_DELETED:
		gth_file_list_delete_list (browser->priv->file_list, list);
		break;

	default:
		break;
	}
}


static gboolean
first_level_sub_directory (GthBrowser *browser,
			   const char *current,
			   const char *old_path)
{
	const char *old_name;
	int         current_l;
	int         old_path_l;

	current_l = strlen (current);
	old_path_l = strlen (old_path);

	if (old_path_l <= current_l + 1)
		return FALSE;

	if (strncmp (current, old_path, current_l) != 0)
		return FALSE;

	old_name = old_path + current_l + 1;

	return (strchr (old_name, '/') == NULL);
}


static void
gth_browser_notify_file_rename (GthBrowser *browser,
				const char *old_name,
				const char *new_name)
{
	GthBrowserPrivateData *priv = browser->priv;

	g_return_if_fail (browser != NULL);

	if ((old_name == NULL) || (new_name == NULL))
		return;

	if ((priv->sidebar_content == GTH_SIDEBAR_CATALOG_LIST)
	    && (priv->catalog_path != NULL)) { /* update the catalog. */
		Catalog  *catalog;
		GList    *scan;
		gboolean  changed = FALSE;

		catalog = catalog_new ();
		if (catalog_load_from_disk (catalog, priv->catalog_path, NULL)) {
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

	gth_file_list_delete (priv->file_list, new_name);
	gth_file_list_rename (priv->file_list, old_name, new_name);

	if (same_uri (old_name, priv->image->path))
		gth_browser_load_image_from_uri (browser, new_name);
}


static void
gth_browser_notify_directory_rename (GthBrowser *browser,
				     const char *old_name,
				     const char *new_name)
{
	GthBrowserPrivateData *priv = browser->priv;

	if (priv->sidebar_content == GTH_SIDEBAR_DIR_LIST) {
		if (same_uri (priv->dir_list->path, old_name))
			gth_browser_go_to_directory (browser, new_name);
		else {
			const char *current = priv->dir_list->path;

			/* a sub directory was renamed, refresh. */
			if (first_level_sub_directory (browser, current, old_name))
				gth_dir_list_remove_directory (priv->dir_list,
							       file_name_from_path (old_name));

			if (first_level_sub_directory (browser, current, new_name))
				gth_dir_list_add_directory (priv->dir_list,
							    file_name_from_path (new_name));
		}

	} else if (priv->sidebar_content == GTH_SIDEBAR_CATALOG_LIST) {
		if (same_uri (priv->catalog_list->path, old_name))
			gth_browser_go_to_catalog_directory (browser, new_name);
		else {
			const char *current = priv->catalog_list->path;
			if (first_level_sub_directory (browser, current, old_name))
				window_update_catalog_list (browser);
		}
	}

	if ((priv->image != NULL)
	    && (priv->sidebar_content == GTH_SIDEBAR_DIR_LIST)
	    && (strncmp (priv->image->path,
			 old_name,
			 strlen (old_name)) == 0)) {
		char *new_image_name;

		new_image_name = g_strconcat (new_name,
					      priv->image->path + strlen (old_name),
					      NULL);
		gth_browser_notify_file_rename (browser,
						priv->image->path,
						new_image_name);
		g_free (new_image_name);
	}
}


static void
gth_browser_notify_directory_delete (GthBrowser *browser,
				     const char *path)
{
	GthBrowserPrivateData *priv = browser->priv;

	if (priv->sidebar_content == GTH_SIDEBAR_DIR_LIST) {
		if (same_uri (priv->dir_list->path, path))
			gth_browser_go_up (browser);
		else {
			const char *current = priv->dir_list->path;
			if (first_level_sub_directory (browser, current, path))
				gth_dir_list_remove_directory (priv->dir_list,
							       file_name_from_path (path));
		}

	} else if (priv->sidebar_content == GTH_SIDEBAR_CATALOG_LIST) {
		if (same_uri (priv->catalog_list->path, path))
			gth_browser_go_up (browser);
		else {
			const char *current = priv->catalog_list->path;
			if (path_in_path (current, path))
				/* a sub directory got deleted, refresh. */
				window_update_catalog_list (browser);
		}
	}

	if ((priv->image != NULL)
	    && (path_in_path (priv->image->path, path))) {
		GList *list;

		list = g_list_append (NULL, priv->image->path);
		gth_browser_notify_files_deleted (browser, list);
		g_list_free (list);
	}
}


static void
gth_browser_notify_directory_new (GthBrowser *browser,
				  const char *path)
{
	GthBrowserPrivateData *priv = browser->priv;

	if (priv->sidebar_content == GTH_SIDEBAR_DIR_LIST) {
		const char *current = priv->dir_list->path;
		if (first_level_sub_directory (browser, current, path))
			gth_dir_list_add_directory (priv->dir_list,
						    file_name_from_path (path));

	} else if (priv->sidebar_content == GTH_SIDEBAR_CATALOG_LIST) {
		const char *current = priv->catalog_list->path;
		if (path_in_path (current, path))
			/* a sub directory was created, refresh. */
			window_update_catalog_list (browser);
	}
}


static void
monitor_update_directory_cb (GthMonitor      *monitor,
			     const char      *dir_path,
			     GthMonitorEvent  event,
			     GthBrowser      *browser)
{
	switch (event) {
	case GTH_MONITOR_EVENT_CREATED:
		gth_browser_notify_directory_new (browser, dir_path);
		break;

	case GTH_MONITOR_EVENT_DELETED:
		gth_browser_notify_directory_delete (browser, dir_path);
		break;

	default:
		break;
	}
}


static void
monitor_file_renamed_cb (GthMonitor      *monitor,
			 const char      *old_name,
			 const char      *new_name,
			 GthBrowser      *browser)
{
	gth_browser_notify_file_rename (browser, old_name, new_name);
}


static void
monitor_directory_renamed_cb (GthMonitor      *monitor,
			      const char      *old_name,
			      const char      *new_name,
			      GthBrowser      *browser)
{
	gth_browser_notify_directory_rename (browser, old_name, new_name);
}


static void
gth_browser_notify_catalog_rename (GthBrowser *browser,
				   const char *old_path,
				   const char *new_path)
{
	GthBrowserPrivateData *priv = browser->priv;
	char     *catalog_dir;
	gboolean  viewing_a_catalog;
	gboolean  current_cat_renamed;
	gboolean  renamed_cat_is_in_current_dir;

	if (priv->sidebar_content != GTH_SIDEBAR_CATALOG_LIST)
		return;

	if (priv->catalog_list->path == NULL)
		return;

	catalog_dir = remove_level_from_path (priv->catalog_list->path);
	viewing_a_catalog = (priv->catalog_path != NULL);
	current_cat_renamed = same_uri (priv->catalog_path, old_path);
	renamed_cat_is_in_current_dir = path_in_path (catalog_dir, new_path);

	if (! renamed_cat_is_in_current_dir) {
		g_free (catalog_dir);
		return;
	}

	if (! viewing_a_catalog)
		gth_browser_go_to_catalog_directory (browser, priv->catalog_list->path);
	else {
		if (current_cat_renamed)
			gth_browser_go_to_catalog (browser, new_path);
		else {
			GtkTreeIter iter;
			gth_browser_go_to_catalog_directory (browser, priv->catalog_list->path);

			/* reselect the current catalog. */
			if (catalog_list_get_iter_from_path (priv->catalog_list,
							     priv->catalog_path, &iter))
				catalog_list_select_iter (priv->catalog_list, &iter);
		}
	}

	g_free (catalog_dir);
}


static void
monitor_catalog_renamed_cb (GthMonitor      *monitor,
			    const char      *old_name,
			    const char      *new_name,
			    GthBrowser      *browser)
{
	gth_browser_notify_catalog_rename (browser, old_name, new_name);
}


static void
monitor_reload_catalogs_cb (GthMonitor *monitor,
			    GthBrowser *browser)
{
	window_update_catalog_list (browser);
}


static void
gth_browser_notify_catalog_new (GthBrowser *browser,
				const char *path)
{
	GthBrowserPrivateData *priv = browser->priv;
	char     *catalog_dir;
	gboolean  viewing_a_catalog;
	gboolean  created_cat_is_in_current_dir;

	if (priv->sidebar_content != GTH_SIDEBAR_CATALOG_LIST)
		return;

	if (priv->catalog_list->path == NULL)
		return;

	viewing_a_catalog = (priv->catalog_path != NULL);
	catalog_dir = remove_level_from_path (priv->catalog_list->path);
	created_cat_is_in_current_dir = path_in_path (catalog_dir, path);

	if (! created_cat_is_in_current_dir) {
		g_free (catalog_dir);
		return;
	}

	gth_browser_go_to_catalog_directory (browser, priv->catalog_list->path);

	if (viewing_a_catalog) {
		GtkTreeIter iter;

		/* reselect the current catalog. */
		if (catalog_list_get_iter_from_path (priv->catalog_list,
						     priv->catalog_path,
						     &iter))
			catalog_list_select_iter (priv->catalog_list, &iter);
	}

	g_free (catalog_dir);
}


static void
gth_browser_notify_catalog_delete (GthBrowser *browser,
				   const char *path)
{
	GthBrowserPrivateData *priv = browser->priv;
	char     *catalog_dir;
	gboolean  viewing_a_catalog;
	gboolean  current_cat_deleted;
	gboolean  deleted_cat_is_in_current_dir;

	if (priv->sidebar_content != GTH_SIDEBAR_CATALOG_LIST)
		return;

	if (priv->catalog_list->path == NULL)
		return;

	catalog_dir = remove_level_from_path (priv->catalog_list->path);
	viewing_a_catalog = (priv->catalog_path != NULL);
	current_cat_deleted = same_uri (priv->catalog_path, path);
	deleted_cat_is_in_current_dir = path_in_path (catalog_dir, path);

	if (! deleted_cat_is_in_current_dir) {
		g_free (catalog_dir);
		return;
	}

	if (! viewing_a_catalog)
		gth_browser_go_to_catalog_directory (browser, priv->catalog_list->path);
	else {
		if (current_cat_deleted) {
			gth_browser_go_to_catalog (browser, NULL);
			gth_browser_go_to_catalog_directory (browser, priv->catalog_list->path);
		} 
		else {
			GtkTreeIter iter;
			gth_browser_go_to_catalog_directory (browser, priv->catalog_list->path);

			/* reselect the current catalog. */
			if (catalog_list_get_iter_from_path (priv->catalog_list,
							     priv->catalog_path, &iter))
				catalog_list_select_iter (priv->catalog_list, &iter);
		}
	}

	g_free (catalog_dir);
}


static void
monitor_update_catalog_cb (GthMonitor      *monitor,
			   const char      *catalog_path,
			   GthMonitorEvent  event,
			   GthBrowser      *browser)
{
	switch (event) {
	case GTH_MONITOR_EVENT_CREATED:
		gth_browser_notify_catalog_new (browser, catalog_path);
		break;

	case GTH_MONITOR_EVENT_DELETED:
		gth_browser_notify_catalog_delete (browser, catalog_path);
		break;

	case GTH_MONITOR_EVENT_CHANGED:
		if (same_uri (browser->priv->catalog_path, catalog_path))
			catalog_activate (browser, browser->priv->catalog_path);
		break;

	default:
		break;
	}
}


static void
gth_browser_notify_update_comment (GthBrowser *browser,
				   const char *filename)
{
	GthBrowserPrivateData *priv = browser->priv;

	g_return_if_fail (browser != NULL);

	update_image_comment (browser);
	gth_file_list_update_comment (priv->file_list, filename);
}


static void
monitor_update_metadata_cb (GthMonitor *monitor,
			    const char *filename,
			    GthBrowser *browser)
{
	gth_browser_notify_update_comment (browser, filename);
}


static void
gth_browser_notify_update_icon_theme (GthBrowser *browser)
{
	GthBrowserPrivateData *priv = browser->priv;

	gth_file_list_update_icon_theme (priv->file_list);
	gth_dir_list_update_icon_theme (priv->dir_list);

	window_update_bookmark_list (browser);
	window_update_history_list (browser);
	window_update_file_list (browser);

	if (priv->bookmarks_dlg != NULL)
		dlg_edit_bookmarks_update (priv->bookmarks_dlg);
}


static void
monitor_update_icon_theme_cb (GthMonitor *monitor,
			      GthBrowser *browser)
{
	gth_browser_notify_update_icon_theme (browser);
}


static void
_hide_sidebar (GthBrowser *browser)
{
	GthBrowserPrivateData *priv = browser->priv;

	priv->sidebar_visible = FALSE;
	priv->sidebar_width = gtk_paned_get_position (GTK_PANED (priv->main_pane));

	if (priv->layout_type <= 1)
		gtk_widget_hide (GTK_PANED (priv->main_pane)->child1);

	else if (priv->layout_type == 2) {
		gtk_widget_hide (GTK_PANED (priv->main_pane)->child2);
		gtk_widget_hide (GTK_PANED (priv->content_pane)->child1);

	} else if (priv->layout_type == 3) {
		gtk_widget_hide (GTK_PANED (priv->main_pane)->child1);
		gtk_widget_hide (GTK_PANED (priv->content_pane)->child1);
	}

	if (priv->image_data_visible)
		gth_browser_show_image_data (browser);
	else
		gth_browser_hide_image_data (browser);

	gtk_widget_show (priv->preview_widget_image);

	/* Sync menu and toolbar. */

	set_action_active_if_different (browser, "View_ShowImage", TRUE);
	set_button_active_no_notify (browser,
				     get_button_from_sidebar_content (browser),
				     FALSE);

	/**/

	gtk_widget_hide (priv->preview_button_image);
	gtk_widget_hide (priv->preview_button_comment);
	if (! priv->image_pane_visible)
		gth_browser_show_image_pane (browser);

	gtk_widget_hide (priv->info_frame);
}


static void
dir_list_started_cb (GthDirList  *dir_list,
		     gpointer     data)
{
	GthBrowser *browser = data;
	
	gth_file_view_set_no_image_text (browser->priv->file_list->view, _("Getting folder listing..."));
	gth_file_list_set_empty_list (browser->priv->file_list);
}


static void
dir_list_done_cb (GthDirList     *dir_list,
		  GnomeVFSResult  result,
		  gpointer        data)
{
	GthBrowser            *browser = data;
	GthBrowserPrivateData *priv = browser->priv;
	GList                 *file_list;
	
	gth_browser_stop_activity_mode (browser);

	if (result != GNOME_VFS_ERROR_EOF) {
		char *utf8_path;

		utf8_path = gnome_vfs_unescape_string_for_display (dir_list->try_path);
		_gtk_error_dialog_run (GTK_WINDOW (browser),
				       _("Cannot load folder \"%s\": %s\n"),
				       utf8_path,
				       gnome_vfs_result_to_string (result));
		g_free (utf8_path);

		set_cursor_not_busy (browser, TRUE);
		priv->refreshing = FALSE;

		if (priv->history_current == NULL)
			gth_browser_go_to_directory (browser, get_home_uri ());
		else
			gth_browser_go_to_directory (browser, priv->history_current->data);

		return;
	}

	set_action_sensitive (browser, "Go_Up", ! uri_is_root (dir_list->path));

	/* Add to history list if not present as last entry. */

	if (priv->go_op == GTH_BROWSER_GO_TO) {
		char *uri = add_scheme_if_absent (dir_list->path);
		if ((priv->history_current == NULL) || ! same_uri (uri, priv->history_current->data))
			add_history_item (browser, uri, NULL);
		g_free (uri);
	} 
	else
		priv->go_op = GTH_BROWSER_GO_TO;

	window_update_history_list (browser);
	gth_location_set_folder_uri (GTH_LOCATION (priv->location), dir_list->path, FALSE);

	/**/

	file_list = gth_dir_list_get_file_list (priv->dir_list);
	if ((file_list == NULL) && (browser->priv->image != NULL)) {
		char *image_dir;
		
		image_dir = remove_level_from_path (browser->priv->image->path);
		if (uricmp (image_dir, browser->priv->dir_list->path) == 0)
			file_list = g_list_append (NULL, file_data_dup (browser->priv->image));
		g_free (image_dir);
	}
	window_set_file_list (browser, file_list, priv->sort_method, priv->sort_type);
	file_data_list_free (file_list);
}


static void
filter_bar_changed_cb (GthFilterBar *filter_bar,
		       GthBrowser   *browser)
{
	g_return_if_fail (GTH_IS_BROWSER (browser));
	gth_file_list_set_filter (browser->priv->file_list, gth_filter_bar_get_filter (filter_bar));
}


static void
filter_bar_close_button_clicked_cb (GthFilterBar *filter_bar,
					   GthBrowser   *browser)
{
	eel_gconf_set_boolean (PREF_UI_FILTERBAR_VISIBLE, FALSE);
	gth_browser_hide_filterbar (browser);
}


static void
gth_browser_construct (GthBrowser  *browser,
		       const gchar *uri)
{
	GthBrowserPrivateData *priv = browser->priv;
	GtkWidget             *paned1;      /* Main paned widget. */
	GtkWidget             *paned2;      /* Secondary paned widget. */
	GtkWidget             *image_vbox;
	GtkWidget             *dir_list_vbox;
	GtkWidget             *info_box;
	GtkWidget             *info_frame;
	GtkWidget             *image_pane_paned1;
	GtkWidget             *image_pane_paned2;
	GtkWidget             *scrolled_win;
	GtkTreeSelection      *selection;
	int                    i;
	GtkActionGroup        *actions;
	GtkUIManager          *ui;
	GError                *error = NULL;
	GtkWidget             *toolbar;

	gtk_window_set_default_size (GTK_WINDOW (browser),
				     eel_gconf_get_integer (PREF_UI_WINDOW_WIDTH, DEF_WIN_WIDTH),
				     eel_gconf_get_integer (PREF_UI_WINDOW_HEIGHT, DEF_WIN_HEIGHT));

	priv->tooltips = gtk_tooltips_new ();

	/* Build the menu and the toolbar. */

	priv->actions = actions = gtk_action_group_new ("Actions");
	gtk_action_group_set_translation_domain (actions, NULL);

	gtk_action_group_add_actions (actions,
				      gth_window_action_entries,
				      gth_window_action_entries_size,
				      browser);
	gtk_action_group_add_toggle_actions (actions,
					     gth_window_action_toggle_entries,
					     gth_window_action_toggle_entries_size,
					     browser);
	gtk_action_group_add_radio_actions (actions,
					    gth_window_zoom_quality_entries,
					    gth_window_zoom_quality_entries_size,
					    GTH_ZOOM_QUALITY_HIGH,
					    G_CALLBACK (zoom_quality_radio_action),
					    browser);

	gtk_action_group_add_actions (actions,
				      gth_browser_action_entries,
				      gth_browser_action_entries_size,
				      browser);
	gtk_action_group_add_toggle_actions (actions,
					     gth_browser_action_toggle_entries,
					     gth_browser_action_toggle_entries_size,
					     browser);
	gtk_action_group_add_radio_actions (actions,
					    gth_browser_sort_by_entries,
					    gth_browser_sort_by_entries_size,
					    GTH_SORT_METHOD_BY_NAME,
					    G_CALLBACK (sort_by_radio_action),
					    browser);
	gtk_action_group_add_radio_actions (actions,
					    gth_browser_content_entries,
					    gth_browser_content_entries_size,
					    GTH_SIDEBAR_DIR_LIST,
					    G_CALLBACK (content_radio_action),
					    browser);
	gtk_action_group_add_radio_actions (actions,
					    gth_browser_view_as_entries,
					    gth_browser_view_as_entries_size,
					    pref_get_view_as (),
					    G_CALLBACK (view_as_radio_action),
					    browser);

	priv->ui = ui = gtk_ui_manager_new ();

	g_signal_connect (ui, "connect_proxy",
			  G_CALLBACK (connect_proxy_cb), browser);
	g_signal_connect (ui, "disconnect_proxy",
			  G_CALLBACK (disconnect_proxy_cb), browser);

	gtk_ui_manager_insert_action_group (ui, actions, 0);
	gtk_window_add_accel_group (GTK_WINDOW (browser),
				    gtk_ui_manager_get_accel_group (ui));
	if (!gtk_ui_manager_add_ui_from_string (ui, main_ui_info, -1, &error)) {
		g_message ("building menus failed: %s", error->message);
		g_error_free (error);
	}
	gth_window_attach (GTH_WINDOW (browser), gtk_ui_manager_get_widget (ui, "/MenuBar"), GTH_WINDOW_MENUBAR);

	priv->toolbar = toolbar = gtk_ui_manager_get_widget (ui, "/ToolBar");
	gtk_toolbar_set_show_arrow (GTK_TOOLBAR (toolbar), TRUE);
	gth_window_attach (GTH_WINDOW (browser), toolbar, GTH_WINDOW_TOOLBAR);

	priv->statusbar = gtk_statusbar_new ();
	gtk_statusbar_set_has_resize_grip (GTK_STATUSBAR (priv->statusbar), TRUE);
	priv->help_message_cid = gtk_statusbar_get_context_id (GTK_STATUSBAR (priv->statusbar), "gth_help_message");
	priv->list_info_cid = gtk_statusbar_get_context_id (GTK_STATUSBAR (priv->statusbar), "gth_list_info");
	gth_window_attach (GTH_WINDOW (browser), priv->statusbar, GTH_WINDOW_STATUSBAR);

	g_signal_connect (G_OBJECT (browser),
			  "delete_event",
			  G_CALLBACK (close_window_cb),
			  browser);
	g_signal_connect (G_OBJECT (browser),
			  "key_press_event",
			  G_CALLBACK (key_press_cb),
			  browser);

	/* Popup menus */

	priv->file_popup_menu = gtk_ui_manager_get_widget (ui, "/FilePopup");
	priv->image_popup_menu = gtk_ui_manager_get_widget (ui, "/ImagePopup");
	priv->catalog_popup_menu = gtk_ui_manager_get_widget (ui, "/CatalogPopup");
	priv->library_popup_menu = gtk_ui_manager_get_widget (ui, "/LibraryPopup");
	priv->dir_popup_menu = gtk_ui_manager_get_widget (ui, "/DirPopup");
	priv->dir_list_popup_menu = gtk_ui_manager_get_widget (ui, "/DirListPopup");
	priv->catalog_list_popup_menu = gtk_ui_manager_get_widget (ui, "/CatalogListPopup");
	priv->history_list_popup_menu = gtk_ui_manager_get_widget (ui, "/HistoryListPopup");

	/* Create the widgets. */

	priv->filterbar = gth_filter_bar_new ();
	gtk_widget_show (priv->filterbar);
	g_signal_connect (G_OBJECT (priv->filterbar),
			  "changed",
			  G_CALLBACK (filter_bar_changed_cb),
			  browser);
	g_signal_connect (G_OBJECT (priv->filterbar),
			  "close_button_clicked",
			  G_CALLBACK (filter_bar_close_button_clicked_cb),
			  browser);

	priv->file_list = create_new_file_list (browser);

	/* Dir list. */

	priv->dir_list = gth_dir_list_new ();
	gtk_widget_show_all (priv->dir_list->root_widget);
	g_signal_connect (G_OBJECT (priv->dir_list),
			  "started",
			  G_CALLBACK (dir_list_started_cb),
			  browser);
	g_signal_connect (G_OBJECT (priv->dir_list),
			  "done",
			  G_CALLBACK (dir_list_done_cb),
			  browser);
	gtk_drag_dest_set (priv->dir_list->root_widget,
			   GTK_DEST_DEFAULT_ALL,
			   target_table, G_N_ELEMENTS (target_table),
			   GDK_ACTION_MOVE);
	gtk_drag_source_set (priv->dir_list->list_view,
			     GDK_BUTTON1_MASK,
			     target_table, G_N_ELEMENTS (target_table),
			     GDK_ACTION_MOVE | GDK_ACTION_COPY);
	g_signal_connect (G_OBJECT (priv->dir_list->root_widget),
			  "drag_data_received",
			  G_CALLBACK (dir_list_drag_data_received),
			  browser);
	g_signal_connect (G_OBJECT (priv->dir_list->list_view),
			  "drag_begin",
			  G_CALLBACK (dir_list_drag_begin),
			  browser);
	g_signal_connect (G_OBJECT (priv->dir_list->list_view),
			  "drag_end",
			  G_CALLBACK (dir_list_drag_end),
			  browser);
	g_signal_connect (G_OBJECT (priv->dir_list->root_widget),
			  "drag_motion",
			  G_CALLBACK (dir_list_drag_motion),
			  browser);
	g_signal_connect (G_OBJECT (priv->dir_list->root_widget),
			  "drag_leave",
			  G_CALLBACK (dir_list_drag_leave),
			  browser);
	g_signal_connect (G_OBJECT (priv->dir_list->list_view),
			  "drag_data_get",
			  G_CALLBACK (dir_list_drag_data_get),
			  browser);
	g_signal_connect (G_OBJECT (priv->dir_list->list_view),
			  "key_press_event",
			  G_CALLBACK (dir_list_key_press_cb),
			  browser);
	g_signal_connect (G_OBJECT (priv->dir_list->list_view),
			  "button_press_event",
			  G_CALLBACK (dir_list_button_press_cb),
			  browser);
	g_signal_connect (G_OBJECT (priv->dir_list->list_view),
			  "button_release_event",
			  G_CALLBACK (dir_list_button_release_cb),
			  browser);
	g_signal_connect (G_OBJECT (priv->dir_list->list_view),
			  "row_activated",
			  G_CALLBACK (dir_list_activated_cb),
			  browser);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->dir_list->list_view));
	g_signal_connect (G_OBJECT (selection),
			  "changed",
			  G_CALLBACK (dir_or_catalog_sel_changed_cb),
			  browser);

	/* Catalog list. */

	priv->catalog_list = catalog_list_new (pref_get_real_click_policy () == GTH_CLICK_POLICY_SINGLE);
	gtk_widget_show_all (priv->catalog_list->root_widget);

	gtk_drag_dest_set (priv->catalog_list->root_widget,
			   GTK_DEST_DEFAULT_ALL,
			   target_table, G_N_ELEMENTS(target_table),
			   GDK_ACTION_MOVE);
	g_signal_connect (G_OBJECT (priv->catalog_list->root_widget),
			  "drag_data_received",
			  G_CALLBACK (catalog_list_drag_data_received),
			  browser);
	g_signal_connect (G_OBJECT (priv->catalog_list->list_view),
			  "drag_begin",
			  G_CALLBACK (catalog_list_drag_begin),
			  browser);
	g_signal_connect (G_OBJECT (priv->catalog_list->root_widget),
			  "drag_motion",
			  G_CALLBACK (catalog_list_drag_motion),
			  browser);
	g_signal_connect (G_OBJECT (priv->catalog_list->root_widget),
			  "drag_leave",
			  G_CALLBACK (catalog_list_drag_leave),
			  browser);
	g_signal_connect (G_OBJECT (priv->catalog_list->list_view),
			  "button_press_event",
			  G_CALLBACK (catalog_list_button_press_cb),
			  browser);
	g_signal_connect (G_OBJECT (priv->catalog_list->list_view),
			  "button_release_event",
			  G_CALLBACK (catalog_list_button_release_cb),
			  browser);
	g_signal_connect (G_OBJECT (priv->catalog_list->list_view),
			  "row_activated",
			  G_CALLBACK (catalog_list_activated_cb),
			  browser);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->catalog_list->list_view));
	g_signal_connect (G_OBJECT (selection),
			  "changed",
			  G_CALLBACK (dir_or_catalog_sel_changed_cb),
			  browser);

	/* Location. */

	priv->location = gth_location_new (FALSE);
	gtk_widget_show_all (priv->location);
	g_signal_connect (G_OBJECT (priv->location),
			  "changed",
			  G_CALLBACK (location_changed_cb),
			  browser);

	/* Info bar. */

	priv->info_bar = gthumb_info_bar_new ();
	gtk_widget_show (priv->info_bar);
	gthumb_info_bar_set_focused (GTHUMB_INFO_BAR (priv->info_bar), FALSE);
	g_signal_connect (G_OBJECT (priv->info_bar),
			  "button_press_event",
			  G_CALLBACK (info_bar_clicked_cb),
			  browser);

	/* Image viewer. */

	priv->viewer = image_viewer_new ();
	gtk_widget_show (priv->viewer);
	gtk_widget_set_size_request (priv->viewer,
				     PANE_MIN_SIZE,
				     PANE_MIN_SIZE);

	/* FIXME
	gtk_drag_source_set (priv->viewer,
			     GDK_BUTTON2_MASK,
			     target_table, G_N_ELEMENTS(target_table),
			     GDK_ACTION_MOVE);
	*/

	gtk_drag_dest_set (priv->viewer,
			   GTK_DEST_DEFAULT_ALL,
			   target_table, G_N_ELEMENTS(target_table),
			   GDK_ACTION_MOVE);

	g_signal_connect (G_OBJECT (priv->viewer),
			  "image_loaded",
			  G_CALLBACK (image_loaded_cb),
			  browser);
	g_signal_connect (G_OBJECT (priv->viewer),
			  "zoom_changed",
			  G_CALLBACK (zoom_changed_cb),
			  browser);
	g_signal_connect_after (G_OBJECT (priv->viewer),
				"button_press_event",
				G_CALLBACK (image_button_press_cb),
				browser);
	g_signal_connect_after (G_OBJECT (priv->viewer),
				"button_release_event",
				G_CALLBACK (image_button_release_cb),
				browser);
	g_signal_connect_after (G_OBJECT (priv->viewer),
				"clicked",
				G_CALLBACK (image_clicked_cb),
				browser);
	g_signal_connect_after (G_OBJECT (priv->viewer),
				"mouse_wheel_scroll",
				G_CALLBACK (mouse_wheel_scrolled_cb),
				browser);
	g_signal_connect (G_OBJECT (priv->viewer),
			  "focus_in_event",
			  G_CALLBACK (image_focus_changed_cb),
			  browser);
	g_signal_connect (G_OBJECT (priv->viewer),
			  "focus_out_event",
			  G_CALLBACK (image_focus_changed_cb),
			  browser);

	g_signal_connect (G_OBJECT (priv->viewer),
			  "drag_data_get",
			  G_CALLBACK (viewer_drag_data_get),
			  browser);

	g_signal_connect (G_OBJECT (priv->viewer),
			  "drag_data_received",
			  G_CALLBACK (viewer_drag_data_received),
			  browser);

	g_signal_connect (G_OBJECT (priv->viewer),
			  "key_press_event",
			  G_CALLBACK (viewer_key_press_cb),
			  browser);

	g_signal_connect (G_OBJECT (IMAGE_VIEWER (priv->viewer)->loader),
			  "image_progress",
			  G_CALLBACK (image_loader_progress_cb),
			  browser);
	g_signal_connect (G_OBJECT (IMAGE_VIEWER (priv->viewer)->loader),
			  "image_done",
			  G_CALLBACK (image_loader_done_cb),
			  browser);
	g_signal_connect (G_OBJECT (IMAGE_VIEWER (priv->viewer)->loader),
			  "image_error",
			  G_CALLBACK (image_loader_done_cb),
			  browser);

	/* Pack the widgets */

	priv->file_list_pane = gtk_vbox_new (0, FALSE);
	gtk_widget_show (priv->file_list_pane);
	gtk_widget_show_all (priv->file_list->root_widget);

	gtk_box_pack_start (GTK_BOX (priv->file_list_pane), priv->file_list->root_widget, TRUE, TRUE, 0);
	gtk_box_pack_end (GTK_BOX (priv->file_list_pane), priv->filterbar, FALSE, FALSE, 0);

	priv->layout_type = eel_gconf_get_integer (PREF_UI_LAYOUT, 2);

	if (priv->layout_type == 1) {
		priv->main_pane = paned1 = gtk_vpaned_new ();
		priv->content_pane = paned2 = gtk_hpaned_new ();
	} 
	else {
		priv->main_pane = paned1 = gtk_hpaned_new ();
		priv->content_pane = paned2 = gtk_vpaned_new ();
	}

	gtk_widget_show (priv->main_pane);
	gtk_widget_show (priv->content_pane);

	gth_window_attach (GTH_WINDOW (browser), priv->main_pane, GTH_WINDOW_CONTENTS);

	if (priv->layout_type == 3)
		gtk_paned_pack2 (GTK_PANED (paned1), paned2, TRUE, FALSE);
	else
		gtk_paned_pack1 (GTK_PANED (paned1), paned2, FALSE, FALSE);

	priv->notebook = gtk_notebook_new ();
	gtk_widget_show (priv->notebook);
	gtk_notebook_set_show_tabs (GTK_NOTEBOOK (priv->notebook), FALSE);
	gtk_notebook_set_show_border (GTK_NOTEBOOK (priv->notebook), FALSE);
	gtk_notebook_append_page (GTK_NOTEBOOK (priv->notebook),
				  priv->dir_list->root_widget,
				  NULL);
	gtk_notebook_append_page (GTK_NOTEBOOK (priv->notebook),
				  priv->catalog_list->root_widget,
				  NULL);

	priv->dir_list_pane = dir_list_vbox = gtk_vbox_new (FALSE, 6);
	gtk_widget_show (priv->dir_list_pane);

	gtk_box_pack_start (GTK_BOX (dir_list_vbox), priv->location,
			    FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (dir_list_vbox), priv->notebook,
			    TRUE, TRUE, 0);

	if (priv->layout_type == 3)
		gtk_paned_pack1 (GTK_PANED (paned1), dir_list_vbox, FALSE, FALSE);
	else
		gtk_paned_pack1 (GTK_PANED (paned2), dir_list_vbox, FALSE, FALSE);

	if (priv->layout_type <= 1)
		gtk_paned_pack2 (GTK_PANED (paned2), priv->file_list_pane, TRUE, FALSE);
	else if (priv->layout_type == 2)
		gtk_paned_pack2 (GTK_PANED (paned1), priv->file_list_pane, TRUE, FALSE);
	else if (priv->layout_type == 3)
		gtk_paned_pack1 (GTK_PANED (paned2), priv->file_list_pane, FALSE, FALSE);

	/**/

	image_vbox = priv->image_pane = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (image_vbox);

	if (priv->layout_type <= 1)
		gtk_paned_pack2 (GTK_PANED (paned1), image_vbox, TRUE, FALSE);
	else
		gtk_paned_pack2 (GTK_PANED (paned2), image_vbox, TRUE, FALSE);

	/* image info bar */

	priv->info_frame = gtk_frame_new (NULL);
	gtk_widget_show (priv->info_frame);
	gtk_frame_set_shadow_type (GTK_FRAME (priv->info_frame), GTK_SHADOW_NONE);
	gtk_box_pack_start (GTK_BOX (image_vbox), priv->info_frame, FALSE, FALSE, 0);

	info_box = gtk_hbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (priv->info_frame), info_box);

	gtk_box_pack_start (GTK_BOX (info_box), priv->info_bar, TRUE, TRUE, 0);

	{
		GtkWidget *button;
		GtkWidget *image;

		button = gtk_button_new ();
		image = gtk_image_new_from_stock (GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU);
		gtk_container_add (GTK_CONTAINER (button), image);
		gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
		gtk_box_pack_end (GTK_BOX (info_box), button, FALSE, FALSE, 0);
		g_signal_connect (G_OBJECT (button),
				  "clicked",
				  G_CALLBACK (close_preview_image_button_cb),
				  browser);
		gtk_tooltips_set_tip (priv->tooltips,
				      button,
				      _("Close"),
				      NULL);

		image = _gtk_image_new_from_inline (preview_comment_16_rgba);
		priv->preview_button_comment = button = gtk_toggle_button_new ();
		gtk_container_add (GTK_CONTAINER (button), image);
		gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
		gtk_box_pack_end (GTK_BOX (info_box), button, FALSE, FALSE, 0);
		g_signal_connect (G_OBJECT (button),
				  "toggled",
				  G_CALLBACK (preview_comment_button_cb),
				  browser);
		gtk_tooltips_set_tip (priv->tooltips,
				      button,
				      _("Image comment"),
				      NULL);

		image = _gtk_image_new_from_inline (preview_data_16_rgba);
		priv->preview_button_data = button = gtk_toggle_button_new ();
		gtk_container_add (GTK_CONTAINER (button), image);
		gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
		gtk_box_pack_end (GTK_BOX (info_box), button, FALSE, FALSE, 0);
		g_signal_connect (G_OBJECT (button),
				  "toggled",
				  G_CALLBACK (preview_data_button_cb),
				  browser);
		gtk_tooltips_set_tip (priv->tooltips,
				      button,
				      _("Image data"),
				      NULL);

		image = _gtk_image_new_from_inline (preview_image_16_rgba);
		priv->preview_button_image = button = gtk_toggle_button_new ();
		gtk_container_add (GTK_CONTAINER (button), image);
		gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
		gtk_box_pack_end (GTK_BOX (info_box), button, FALSE, FALSE, 0);
		g_signal_connect (G_OBJECT (button),
				  "toggled",
				  G_CALLBACK (preview_image_button_cb),
				  browser);
		gtk_tooltips_set_tip (priv->tooltips,
				      button,
				      _("Image preview"),
				      NULL);
	}

	gtk_widget_show_all (info_box);

	/* image preview, comment, exif data. */

	priv->image_main_pane = image_pane_paned1 = gtk_vpaned_new ();
	gtk_widget_show (priv->image_main_pane);

	gtk_box_pack_start (GTK_BOX (image_vbox), image_pane_paned1, TRUE, TRUE, 0);

	/**/

	priv->preview_widget_image = gth_nav_window_new (GTH_IVIEWER (priv->viewer));
	gtk_paned_pack1 (GTK_PANED (image_pane_paned1), priv->preview_widget_image, FALSE, FALSE);

	/**/

	priv->preview_widget_data_comment = image_pane_paned2 = gtk_hpaned_new ();
	gtk_widget_show (priv->preview_widget_data_comment);
	gtk_paned_pack2 (GTK_PANED (image_pane_paned1), image_pane_paned2, TRUE, FALSE);

	priv->preview_widget_comment = scrolled_win = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_win), GTK_SHADOW_ETCHED_IN);

	priv->image_comment = gtk_text_view_new ();
	gtk_text_view_set_editable (GTK_TEXT_VIEW (priv->image_comment), FALSE);
	gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (priv->image_comment), GTK_WRAP_WORD);
	gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (priv->image_comment), TRUE);
	gtk_container_add (GTK_CONTAINER (scrolled_win), priv->image_comment);

	gtk_paned_pack2 (GTK_PANED (image_pane_paned2), scrolled_win, TRUE, FALSE);

	g_signal_connect (G_OBJECT (priv->image_comment),
			  "focus_in_event",
			  G_CALLBACK (image_focus_changed_cb),
			  browser);
	g_signal_connect (G_OBJECT (priv->image_comment),
			  "focus_out_event",
			  G_CALLBACK (image_focus_changed_cb),
			  browser);

	g_signal_connect (G_OBJECT (priv->image_comment),
			  "button_press_event",
			  G_CALLBACK (image_comment_button_press_cb),
			  browser);

	/* exif data */

	priv->preview_widget_data = priv->exif_data_viewer = gth_exif_data_viewer_new (TRUE);
	gtk_paned_pack1 (GTK_PANED (image_pane_paned2), priv->exif_data_viewer, FALSE, FALSE);

	g_signal_connect (G_OBJECT (gth_exif_data_viewer_get_view (GTH_EXIF_DATA_VIEWER (priv->exif_data_viewer))),
			  "focus_in_event",
			  G_CALLBACK (image_focus_changed_cb),
			  browser);
	g_signal_connect (G_OBJECT (gth_exif_data_viewer_get_view (GTH_EXIF_DATA_VIEWER (priv->exif_data_viewer))),
			  "focus_out_event",
			  G_CALLBACK (image_focus_changed_cb),
			  browser);

	gtk_widget_show_all (priv->image_main_pane);

	/* Progress bar. */

	priv->progress = gtk_progress_bar_new ();
	gtk_widget_set_size_request (priv->progress, -1, PROGRESS_BAR_HEIGHT);
	{
		GtkWidget *vbox;
		
		vbox = gtk_vbox_new (FALSE, 0);
		gtk_box_pack_start (GTK_BOX (priv->statusbar), vbox, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (vbox), priv->progress, TRUE, FALSE, 0);
		gtk_widget_show_all (vbox);
	}

	/* Zoom info */

	priv->zoom_info = gtk_label_new (NULL);
	gtk_widget_show (priv->zoom_info);

	priv->zoom_info_frame = gtk_frame_new (NULL);
	gtk_widget_show (priv->zoom_info_frame);
	gtk_frame_set_shadow_type (GTK_FRAME (priv->zoom_info_frame), GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (priv->zoom_info_frame), priv->zoom_info);
	gtk_box_pack_start (GTK_BOX (priv->statusbar), priv->zoom_info_frame, FALSE, FALSE, 0);

	/* Image info */

	priv->image_info = gtk_label_new (NULL);
	gtk_widget_show (priv->image_info);

	priv->image_info_frame = info_frame = gtk_frame_new (NULL);
	gtk_widget_show (priv->image_info_frame);

	gtk_frame_set_shadow_type (GTK_FRAME (info_frame), GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (info_frame), priv->image_info);
	gtk_box_pack_start (GTK_BOX (priv->statusbar), info_frame, FALSE, FALSE, 0);

	/* Progress dialog */

	priv->progress_gui = glade_xml_new (GTHUMB_GLADEDIR "/" GLADE_EXPORTER_FILE, NULL, NULL);
	if (! priv->progress_gui) {
		priv->progress_dialog = NULL;
		priv->progress_progressbar = NULL;
		priv->progress_info = NULL;
	} 
	else {
		GtkWidget *cancel_button;

		priv->progress_dialog = glade_xml_get_widget (priv->progress_gui, "progress_dialog");
		priv->progress_progressbar = glade_xml_get_widget (priv->progress_gui, "progress_progressbar");
		priv->progress_info = glade_xml_get_widget (priv->progress_gui, "progress_info");
		cancel_button = glade_xml_get_widget (priv->progress_gui, "progress_cancel");

		gtk_window_set_transient_for (GTK_WINDOW (priv->progress_dialog), GTK_WINDOW (browser));
		gtk_window_set_modal (GTK_WINDOW (priv->progress_dialog), FALSE);

		g_signal_connect (G_OBJECT (cancel_button),
				  "clicked",
				  G_CALLBACK (progress_cancel_cb),
				  browser);
		g_signal_connect (G_OBJECT (priv->progress_dialog),
				  "delete_event",
				  G_CALLBACK (progress_delete_cb),
				  browser);
	}

	/* Update data. */

	priv->sidebar_content = GTH_SIDEBAR_NO_LIST;
	priv->sidebar_visible = TRUE;
	priv->preview_visible = eel_gconf_get_boolean (PREF_SHOW_PREVIEW, FALSE);
	priv->image_pane_visible = priv->preview_visible;
	priv->image_data_visible = eel_gconf_get_boolean (PREF_SHOW_IMAGE_DATA, FALSE);
	priv->catalog_path = NULL;
	priv->image = NULL;
	priv->image_catalog = NULL;
	priv->image_modified = FALSE;
	priv->image_position = -1;

	priv->bookmarks_length = 0;
	window_update_bookmark_list (browser);

	priv->history = bookmarks_new (NULL);
	priv->history_current = NULL;
	priv->history_length = 0;
	priv->go_op = GTH_BROWSER_GO_TO;
	window_update_history_list (browser);

	priv->activity_timeout = 0;
	priv->activity_ref = 0;

	priv->image_prop_dlg = NULL;

	priv->busy_cursor_active = FALSE;

	priv->view_image_timeout = 0;
	priv->load_dir_timeout = 0;
	priv->auto_load_timeout = 0;

	priv->sel_change_timeout = 0;
	
	priv->show_hidden_files = eel_gconf_get_boolean (PREF_SHOW_HIDDEN_FILES, FALSE);
	priv->fast_file_type = eel_gconf_get_boolean (PREF_FAST_FILE_TYPE, TRUE);
	
	/* preloader */

	priv->preloader = gthumb_preloader_new ();

	g_signal_connect (G_OBJECT (priv->preloader),
			  "requested_done",
			  G_CALLBACK (image_requested_done_cb),
			  browser);

	g_signal_connect (G_OBJECT (priv->preloader),
			  "requested_error",
			  G_CALLBACK (image_requested_error_cb),
			  browser);

	for (i = 0; i < GCONF_NOTIFICATIONS; i++)
		priv->cnxn_id[i] = 0;

	priv->pixop = NULL;

	/* Sync widgets and visualization options. */

	gtk_widget_realize (GTK_WIDGET (browser));

	/**/

#ifndef HAVE_LIBGPHOTO
	set_action_sensitive (browser, "File_CameraImport", FALSE);
#endif /* HAVE_LIBGPHOTO */

	/**/

	priv->sidebar_width = eel_gconf_get_integer (PREF_UI_SIDEBAR_SIZE, DEF_SIDEBAR_SIZE);
	gtk_paned_set_position (GTK_PANED (paned1), eel_gconf_get_integer (PREF_UI_SIDEBAR_SIZE, DEF_SIDEBAR_SIZE));
	gtk_paned_set_position (GTK_PANED (paned2), eel_gconf_get_integer (PREF_UI_SIDEBAR_CONTENT_SIZE, DEF_SIDEBAR_CONT_SIZE));

	gtk_widget_show (priv->main_pane);

	window_sync_menu_with_preferences (browser);

	if (priv->preview_visible)
		gth_browser_show_image_pane (browser);
	else
		gth_browser_hide_image_pane (browser);

	gth_browser_set_preview_content (browser, pref_get_preview_content ());

	gth_browser_notify_update_toolbar_style (browser);
	window_update_statusbar_image_info (browser);
	window_update_statusbar_zoom_info (browser);

	image_viewer_set_zoom_quality (IMAGE_VIEWER (priv->viewer),
				       pref_get_zoom_quality ());
	image_viewer_set_zoom_change  (IMAGE_VIEWER (priv->viewer),
				       pref_get_zoom_change ());
	image_viewer_set_check_type   (IMAGE_VIEWER (priv->viewer),
				       pref_get_check_type ());
	image_viewer_set_check_size   (IMAGE_VIEWER (priv->viewer),
				       pref_get_check_size ());
	image_viewer_set_transp_type  (IMAGE_VIEWER (priv->viewer),
				       pref_get_transp_type ());
	image_viewer_set_black_background (IMAGE_VIEWER (priv->viewer),
					   eel_gconf_get_boolean (PREF_BLACK_BACKGROUND, FALSE));
	image_viewer_set_reset_scrollbars (IMAGE_VIEWER (priv->viewer), eel_gconf_get_boolean (PREF_RESET_SCROLLBARS, TRUE));

	set_mode_specific_ui_info (browser, GTH_SIDEBAR_DIR_LIST, TRUE);

	gtk_paned_set_position (GTK_PANED (browser->priv->image_main_pane), eel_gconf_get_integer (PREF_UI_WINDOW_HEIGHT, DEF_WIN_HEIGHT) - eel_gconf_get_integer (PREF_UI_COMMENT_PANE_SIZE, DEFAULT_COMMENT_PANE_SIZE));
	gtk_paned_set_position (GTK_PANED (image_pane_paned2), eel_gconf_get_integer (PREF_UI_WINDOW_WIDTH, DEF_WIN_WIDTH) / 2);

	/* Add notification callbacks. */

	i = 0;

	priv->cnxn_id[i++] = eel_gconf_notification_add (
					   PREF_UI_LAYOUT,
					   pref_ui_layout_changed,
					   browser);

	priv->cnxn_id[i++] = eel_gconf_notification_add (
					   PREF_UI_TOOLBAR_STYLE,
					   pref_ui_toolbar_style_changed,
					   browser);
	priv->cnxn_id[i++] = eel_gconf_notification_add (
					   "/desktop/gnome/interface/toolbar_style",
					   pref_ui_toolbar_style_changed,
					   browser);

	priv->cnxn_id[i++] = eel_gconf_notification_add (
					   PREF_UI_TOOLBAR_VISIBLE,
					   pref_ui_toolbar_visible_changed,
					   browser);

	priv->cnxn_id[i++] = eel_gconf_notification_add (
					   PREF_UI_STATUSBAR_VISIBLE,
					   pref_ui_statusbar_visible_changed,
					   browser);

	priv->cnxn_id[i++] = eel_gconf_notification_add (
					   PREF_SHOW_THUMBNAILS,
					   pref_show_thumbnails_changed,
					   browser);

	priv->cnxn_id[i++] = eel_gconf_notification_add (
					   PREF_SHOW_FILENAMES,
					   pref_show_filenames_changed,
					   browser);

	priv->cnxn_id[i++] = eel_gconf_notification_add (
					   PREF_SHOW_COMMENTS,
					   pref_show_comments_changed,
					   browser);

	priv->cnxn_id[i++] = eel_gconf_notification_add (
					   PREF_SHOW_HIDDEN_FILES,
					   pref_show_hidden_files_changed,
					   browser);

	priv->cnxn_id[i++] = eel_gconf_notification_add (
					   PREF_FAST_FILE_TYPE,
					   pref_fast_file_type_changed,
					   browser);

	priv->cnxn_id[i++] = eel_gconf_notification_add (
					   PREF_THUMBNAIL_SIZE,
					   pref_thumbnail_size_changed,
					   browser);

	priv->cnxn_id[i++] = eel_gconf_notification_add (
					   PREF_CLICK_POLICY,
					   pref_click_policy_changed,
					   browser);

	priv->cnxn_id[i++] = eel_gconf_notification_add (
					   PREF_ZOOM_QUALITY,
					   pref_zoom_quality_changed,
					   browser);

	priv->cnxn_id[i++] = eel_gconf_notification_add (
					   PREF_ZOOM_CHANGE,
					   pref_zoom_change_changed,
					   browser);

	priv->cnxn_id[i++] = eel_gconf_notification_add (
					   PREF_TRANSP_TYPE,
					   pref_transp_type_changed,
					   browser);

	priv->cnxn_id[i++] = eel_gconf_notification_add (
					   PREF_CHECK_TYPE,
					   pref_check_type_changed,
					   browser);

	priv->cnxn_id[i++] = eel_gconf_notification_add (
					   PREF_CHECK_SIZE,
					   pref_check_size_changed,
					   browser);

	priv->cnxn_id[i++] = eel_gconf_notification_add (
					   PREF_BLACK_BACKGROUND,
					   pref_black_background_changed,
					   browser);

	priv->cnxn_id[i++] = eel_gconf_notification_add (
					   PREF_RESET_SCROLLBARS,
					   pref_reset_scrollbars_changed,
					   browser);

	priv->cnxn_id[i++] = eel_gconf_notification_add (
					   PREF_VIEW_AS,
					   pref_view_as_changed,
					   browser);

	/**/

	g_signal_connect_swapped (G_OBJECT (monitor),
				  "update_bookmarks",
				  G_CALLBACK (window_update_bookmark_list),
				  browser);
	g_signal_connect (G_OBJECT (monitor),
			  "update_cat_files",
			  G_CALLBACK (monitor_update_cat_files_cb),
			  browser);
	g_signal_connect (G_OBJECT (monitor),
			  "update_files",
			  G_CALLBACK (monitor_update_files_cb),
			  browser);
	g_signal_connect (G_OBJECT (monitor),
			  "update_directory",
			  G_CALLBACK (monitor_update_directory_cb),
			  browser);
	g_signal_connect (G_OBJECT (monitor),
			  "update_catalog",
			  G_CALLBACK (monitor_update_catalog_cb),
			  browser);
	g_signal_connect (G_OBJECT (monitor),
			  "update_metadata",
			  G_CALLBACK (monitor_update_metadata_cb),
			  browser);
	g_signal_connect (G_OBJECT (monitor),
			  "file_renamed",
			  G_CALLBACK (monitor_file_renamed_cb),
			  browser);
	g_signal_connect (G_OBJECT (monitor),
			  "directory_renamed",
			  G_CALLBACK (monitor_directory_renamed_cb),
			  browser);
	g_signal_connect (G_OBJECT (monitor),
			  "catalog_renamed",
			  G_CALLBACK (monitor_catalog_renamed_cb),
			  browser);
	g_signal_connect (G_OBJECT (monitor),
			  "reload_catalogs",
			  G_CALLBACK (monitor_reload_catalogs_cb),
			  browser);
	g_signal_connect (G_OBJECT (monitor),
			  "update_icon_theme",
			  G_CALLBACK (monitor_update_icon_theme_cb),
			  browser);

	/* Initial location. */

	if (uri != NULL)
		priv->initial_location = g_strdup (uri);
	else {
		const char *startup_uri = preferences_get_startup_location ();
		if (startup_uri != NULL)
			priv->initial_location = g_strdup (startup_uri);
	}

	g_idle_add (initial_location_cb, browser);
}


static GList *browser_list = NULL;


GtkWidget *
gth_browser_new (const char *uri)
{
	GthBrowser *browser;

	browser = (GthBrowser*) g_object_new (GTH_TYPE_BROWSER, NULL);
	gth_browser_construct (browser, uri);

	browser_list = g_list_prepend (browser_list, browser);

	return (GtkWidget*) browser;
}


/* -- gth_browser_close -- */


static void
_window_remove_notifications (GthBrowser *browser)
{
	int i;

	for (i = 0; i < GCONF_NOTIFICATIONS; i++)
		if (browser->priv->cnxn_id[i] != 0)
			eel_gconf_notification_remove (browser->priv->cnxn_id[i]);
}


static void
close__step6 (FileData *file,
	      gpointer  data)
{
	GthBrowser            *browser = data;
	GthBrowserPrivateData *priv = browser->priv;
	ImageViewer           *viewer = IMAGE_VIEWER (priv->viewer);
	gboolean               last_window;
	GdkWindowState         state;
	gboolean               maximized;

	browser_list = g_list_remove (browser_list, browser);
	last_window = gth_window_get_n_windows () == 1;

	/* Save visualization options only if the window is not maximized. */

	state = gdk_window_get_state (GTK_WIDGET (browser)->window);
	maximized = (state & GDK_WINDOW_STATE_MAXIMIZED) != 0;
	if (! maximized && GTK_WIDGET_VISIBLE (browser)) {
		int width, height;

		gdk_drawable_get_size (GTK_WIDGET (browser)->window, &width, &height);
		eel_gconf_set_integer (PREF_UI_WINDOW_WIDTH, width);
		eel_gconf_set_integer (PREF_UI_WINDOW_HEIGHT, height);
	}

	if (priv->sidebar_visible) {
		eel_gconf_set_integer (PREF_UI_SIDEBAR_SIZE, gtk_paned_get_position (GTK_PANED (priv->main_pane)));
		eel_gconf_set_integer (PREF_UI_SIDEBAR_CONTENT_SIZE, gtk_paned_get_position (GTK_PANED (priv->content_pane)));
	} 
	else
		eel_gconf_set_integer (PREF_UI_SIDEBAR_SIZE, priv->sidebar_width);

	eel_gconf_set_integer (PREF_UI_COMMENT_PANE_SIZE, _gtk_widget_get_height (GTK_WIDGET (browser)) - gtk_paned_get_position (GTK_PANED (priv->image_main_pane)));

	if (last_window)
		eel_gconf_set_boolean (PREF_SHOW_THUMBNAILS, priv->file_list->enable_thumbs);

	pref_set_arrange_type (priv->sort_method);
	pref_set_sort_order (priv->sort_type);

	if (last_window
	    && eel_gconf_get_boolean (PREF_GO_TO_LAST_LOCATION, TRUE)) {
		char *location = NULL;

		if (priv->sidebar_content == GTH_SIDEBAR_DIR_LIST)
			location = add_scheme_if_absent (priv->dir_list->path);
		else if ((priv->sidebar_content == GTH_SIDEBAR_CATALOG_LIST)
			 && (priv->catalog_path != NULL))
			location = g_strconcat (CATALOG_PREFIX,
						remove_host_from_uri (priv->catalog_path),
						NULL);

		if (location != NULL) {
			eel_gconf_set_path (PREF_STARTUP_LOCATION, location);
			g_free (location);
		}
	}

	if (last_window) {
		pref_set_zoom_quality (image_viewer_get_zoom_quality (viewer));
		pref_set_transp_type (image_viewer_get_transp_type (viewer));
	}
	pref_set_check_type (image_viewer_get_check_type (viewer));
	pref_set_check_size (image_viewer_get_check_size (viewer));

	eel_gconf_set_boolean (PREF_UI_IMAGE_PANE_VISIBLE, priv->image_pane_visible);
	eel_gconf_set_boolean (PREF_SHOW_PREVIEW, priv->preview_visible);
	eel_gconf_set_boolean (PREF_SHOW_IMAGE_DATA, priv->image_data_visible);
	pref_set_preview_content (priv->preview_content);

	if (priv->fullscreen != NULL)
		g_signal_handlers_disconnect_by_data (G_OBJECT (priv->fullscreen), browser);

	g_signal_handlers_disconnect_by_data (G_OBJECT (monitor), browser);

	/* Destroy the main window. */

	if (priv->progress_timeout != 0) {
		g_source_remove (priv->progress_timeout);
		priv->progress_timeout = 0;
	}

	if (priv->activity_timeout != 0) {
		g_source_remove (priv->activity_timeout);
		priv->activity_timeout = 0;
	}

	if (priv->load_dir_timeout != 0) {
		g_source_remove (priv->load_dir_timeout);
		priv->load_dir_timeout = 0;
	}

	if (priv->sel_change_timeout != 0) {
		g_source_remove (priv->sel_change_timeout);
		priv->sel_change_timeout = 0;
	}

	if (priv->view_image_timeout != 0) {
 		g_source_remove (priv->view_image_timeout);
		priv->view_image_timeout = 0;
	}

	if (priv->auto_load_timeout != 0) {
		g_source_remove (priv->auto_load_timeout);
		priv->auto_load_timeout = 0;
	}

	if (priv->pixop != NULL)
		g_object_unref (priv->pixop);

	if (priv->progress_gui != NULL)
		g_object_unref (priv->progress_gui);

	gtk_widget_destroy (priv->file_popup_menu);
	gtk_widget_destroy (priv->image_popup_menu);
	gtk_widget_destroy (priv->catalog_popup_menu);
	gtk_widget_destroy (priv->library_popup_menu);
	gtk_widget_destroy (priv->dir_popup_menu);
	gtk_widget_destroy (priv->dir_list_popup_menu);
	gtk_widget_destroy (priv->catalog_list_popup_menu);
	gtk_widget_destroy (priv->history_list_popup_menu);

	if (gth_file_list_drag_data != NULL) {
		path_list_free (gth_file_list_drag_data);
		gth_file_list_drag_data = NULL;
	}

	if (dir_list_drag_data != NULL) {
		g_free (dir_list_drag_data);
		dir_list_drag_data = NULL;
	}

	g_free (priv->initial_location);

	gtk_object_destroy (GTK_OBJECT (priv->tooltips));

	if (last_window) { /* FIXME */
		if (dir_list_tree_path != NULL) {
			gtk_tree_path_free (dir_list_tree_path);
			dir_list_tree_path = NULL;
		}

		if (catalog_list_tree_path != NULL) {
			gtk_tree_path_free (catalog_list_tree_path);
			catalog_list_tree_path = NULL;
		}

	}

	if (priv->view_as_timeout != 0)
		g_source_remove (priv->view_as_timeout);

	gtk_widget_destroy (GTK_WIDGET (browser));
}


static void
close__step5 (GthBrowser *browser)
{
	if (browser->priv->image_modified)
		if (ask_whether_to_save (browser, close__step6))
			return;
	close__step6 (NULL, browser);
}


static void
close__step4 (GthBrowser *browser)
{
	if (browser->priv->preloader != NULL)
		gthumb_preloader_stop (browser->priv->preloader,
				       (DoneFunc) close__step5,
				       browser);
	else
		close__step5 (browser);
}


static void
close__step2 (GthBrowser *browser)
{
	gth_file_list_stop (browser->priv->file_list);
	close__step4 (browser);
}


static void
gth_browser_close (GthWindow *window)
{
	GthBrowser            *browser = (GthBrowser*) window;
	GthBrowserPrivateData *priv = browser->priv;

	browser->priv->closing = TRUE;

	/* Interrupt any activity. */

	if (priv->update_layout_timeout != 0) {
		g_source_remove (priv->update_layout_timeout);
		priv->update_layout_timeout = 0;
	}

	_window_remove_notifications (browser);
	gth_browser_stop_activity_mode (browser);
	gth_browser_remove_monitor (browser);

	if (priv->image_prop_dlg != NULL) {
		dlg_image_prop_close (priv->image_prop_dlg);
		priv->image_prop_dlg = NULL;
	}

	close__step2 (browser);
}


void
gth_browser_set_sidebar_content (GthBrowser *browser,
				 int         sidebar_content)
{
	GthBrowserPrivateData *priv = browser->priv;
	char                   old_content = priv->sidebar_content;
	char                  *path;

	gth_browser_set_sidebar (browser, sidebar_content);

	if (sidebar_content == GTH_SIDEBAR_NO_LIST)
		gth_browser_hide_sidebar (browser);
	else
		gth_browser_show_sidebar (browser);

	gth_file_view_set_reorderable (priv->file_list->view, (sidebar_content == GTH_SIDEBAR_CATALOG_LIST));

	if (old_content == sidebar_content)
		return;

	switch (sidebar_content) {
	case GTH_SIDEBAR_DIR_LIST:
		if (priv->dir_list->path == NULL) {
			char *folder_uri = NULL;
			if (priv->image != NULL) {
				ImageToDisplay = g_strdup (priv->image->path);
				folder_uri = remove_level_from_path (priv->image->path);
			} else
				folder_uri = g_strdup (get_home_uri ());

			gth_browser_go_to_directory (browser, folder_uri);

			g_free (folder_uri);
		} 
		else
			gth_browser_go_to_directory (browser, priv->dir_list->path);
		break;

	case GTH_SIDEBAR_CATALOG_LIST:
		if (priv->catalog_path == NULL)
			path = get_catalog_full_path (NULL);
		else
			path = remove_level_from_path (priv->catalog_path);
		gth_browser_go_to_catalog_directory (browser, path);

		g_free (path);

		debug (DEBUG_INFO, "catalog path: %s", priv->catalog_path);

		if (priv->catalog_path != NULL) {
			GtkTreeIter iter;

			if (! catalog_list_get_iter_from_path (priv->catalog_list, priv->catalog_path, &iter)) {
				window_image_viewer_set_void (browser);
				return;
			}
			catalog_list_select_iter (priv->catalog_list, &iter);
			catalog_activate (browser, priv->catalog_path);
		} 
		else {
			window_set_file_list (browser, NULL, priv->sort_method, priv->sort_type);
			window_image_viewer_set_void (browser);
		}

		window_update_title (browser);
		break;

	default:
		break;
	}
}


GthSidebarContent
gth_browser_get_sidebar_content (GthBrowser *browser)
{
	g_return_val_if_fail (browser != NULL, 0);
	return browser->priv->sidebar_content;
}


void
gth_browser_hide_sidebar (GthBrowser *browser)
{
	GthBrowserPrivateData *priv = browser->priv;
	GtkWidget             *widget_to_focus = priv->viewer;

	if (priv->image == NULL)
		return;

	/* If the list of selected files contains video files, launch the
	   external video viewer. Otherwise, for normal image files, just
	   display the image by its self. */

	if (launch_selected_videos_or_audio (browser))
		return;

	_hide_sidebar (browser);
	gtk_widget_grab_focus (widget_to_focus);
	window_update_sensitivity (browser);
	window_update_statusbar_zoom_info (browser);
}


void
gth_browser_show_sidebar (GthBrowser *browser)
{
	GthBrowserPrivateData *priv = browser->priv;
	char                  *cname;

	if (! priv->sidebar_visible)
		gtk_paned_set_position (GTK_PANED (priv->main_pane),
					priv->sidebar_width);

	priv->sidebar_visible = TRUE;

	/**/

	if (priv->layout_type < 2)
		gtk_widget_show (GTK_PANED (priv->main_pane)->child1);

	else if (priv->layout_type == 2) {
		gtk_widget_show (GTK_PANED (priv->main_pane)->child2);
		gtk_widget_show (GTK_PANED (priv->content_pane)->child1);

	} else if (priv->layout_type == 3) {
		gtk_widget_show (GTK_PANED (priv->main_pane)->child1);
		gtk_widget_show (GTK_PANED (priv->content_pane)->child1);
	}

	/**/

	gth_browser_set_preview_content (browser, priv->preview_content);

	/* Sync menu and toolbar. */

	cname = get_command_name_from_sidebar_content (browser);
	if (cname != NULL)
		set_action_active_if_different (browser, cname, TRUE);
	set_button_active_no_notify (browser,
				     get_button_from_sidebar_content (browser),
				     TRUE);

	/**/

	gtk_widget_show (priv->preview_button_image);
	gtk_widget_show (priv->preview_button_comment);

	if (priv->preview_visible)
		gth_browser_show_image_pane (browser);
	else
		gth_browser_hide_image_pane (browser);

	gtk_widget_show (priv->info_frame);

	/**/

	gtk_widget_grab_focus (gth_file_view_get_widget (priv->file_list->view));
	window_update_sensitivity (browser);
}


void
gth_browser_hide_image_pane (GthBrowser *browser)
{
	GthBrowserPrivateData *priv = browser->priv;

	priv->image_pane_visible = FALSE;
	gtk_widget_hide (priv->image_pane);

	gtk_widget_grab_focus (gth_file_view_get_widget (priv->file_list->view));

	/**/

	if (! priv->sidebar_visible)
		gth_browser_show_sidebar (browser);
	else {
		priv->preview_visible = FALSE;
		/* Sync menu and toolbar. */
		set_action_active_if_different (browser,
						"View_ShowPreview",
						FALSE);
	}

	window_update_statusbar_zoom_info (browser);
	window_update_sensitivity (browser);
}


void
gth_browser_show_image_pane (GthBrowser *browser)
{
	GthBrowserPrivateData *priv = browser->priv;

	priv->image_pane_visible = TRUE;

	if (priv->sidebar_visible) {
		priv->preview_visible = TRUE;

		/* Sync menu and toolbar. */
		set_action_active_if_different (browser,
						"View_ShowPreview",
						TRUE);

		gtk_widget_show (priv->info_frame);
	}

	gtk_widget_show (priv->image_pane);
	gtk_widget_grab_focus (gth_file_view_get_widget (priv->file_list->view));

	window_update_statusbar_zoom_info (browser);
	window_update_sensitivity (browser);
}


/* -- gth_browser_stop_loading -- */


static void
stop__step5 (GthBrowser *browser)
{
	GthBrowserPrivateData *priv = browser->priv;

	set_action_sensitive (browser, "Go_Stop",
			       (priv->activity_ref > 0)
			       || priv->file_list->busy);
}


static void
stop__step4 (GthBrowser *browser)
{
	GthBrowserPrivateData *priv = browser->priv;

	gthumb_preloader_stop (priv->preloader,
			       (DoneFunc) stop__step5,
			       browser);

	set_action_sensitive (browser, "Go_Stop",
			       (priv->activity_ref > 0)
			       || priv->file_list->busy);
}


static void
stop__step2 (GthBrowser *browser)
{
	GthBrowserPrivateData *priv = browser->priv;

	gth_file_list_stop (priv->file_list);

	stop__step4 (browser);
}


void
gth_browser_stop_loading (GthBrowser *browser)
{
	GthBrowserPrivateData *priv = browser->priv;

	gth_browser_stop_activity_mode (browser);

	gth_dir_list_stop (priv->dir_list);
	stop__step2 (browser);
}


void
gth_browser_refresh (GthBrowser *browser)
{
	browser->priv->refreshing = TRUE;
	window_update_file_list (browser);
}


void
gth_browser_go_to_directory (GthBrowser *browser,
			     const char *dir_path)
{
	g_return_if_fail (browser != NULL);

	set_cursor_busy (browser, TRUE);

	gth_browser_set_sidebar (browser, GTH_SIDEBAR_DIR_LIST);
	if (! browser->priv->refreshing)
		gth_browser_show_sidebar (browser);

	gth_browser_start_activity_mode (browser);
	gth_dir_list_change_to (browser->priv->dir_list, dir_path);
}


const char *
gth_browser_get_current_directory (GthBrowser *browser)
{
	const char *path;

	path = browser->priv->dir_list->path;
	if (path == NULL)
		path = get_home_uri ();

	return path;
}


/* -- */


void
gth_browser_show_catalog_directory (GthBrowser *browser,
				    const char *catalog_dir)
{
	char *catalog_dir2;

	if ((catalog_dir == NULL) 
	    || (strlen (catalog_dir) == 0) 
	    || (strcmp (catalog_dir, "file:///") == 0))
	{
		catalog_dir2 = g_strconcat (get_home_uri (),
					    "/",
					    RC_CATALOG_DIR,
					    NULL);
	}
	else
		catalog_dir2 = g_strdup (catalog_dir);

	gth_browser_set_sidebar_content (browser, GTH_SIDEBAR_CATALOG_LIST);
	gth_browser_go_to_catalog_directory (browser, catalog_dir2);
	gth_browser_go_to_catalog (browser, NULL);

	g_free (catalog_dir2);
}


void
gth_browser_go_to_catalog_directory (GthBrowser *browser,
				     const char *catalog_dir)
{
	GthBrowserPrivateData *priv = browser->priv;
	char                  *base_dir;
	char                  *catalog_dir2 = NULL;
	char                  *catalog_dir3 = NULL;
	char                  *current_path;
	gboolean               reset_history = FALSE;

	if (catalog_dir == NULL)
		return;

	catalog_dir2 = remove_special_dirs_from_path (catalog_dir);
	catalog_dir3 = remove_ending_separator (catalog_dir2);
	g_free (catalog_dir2);

	if (catalog_dir3 == NULL)
		return;

	base_dir = g_strconcat (get_home_uri (),
				"/",
				RC_CATALOG_DIR,
				NULL);

	/* Go up one level until a directory exists. */

	while ((catalog_dir3 != NULL) && ! same_uri (base_dir, catalog_dir3) && ! path_is_dir (catalog_dir3)) {
		char *new_dir = remove_level_from_path (catalog_dir3);
		g_free (catalog_dir3);
		catalog_dir3 = new_dir;
		reset_history = TRUE;
	}

	if (catalog_dir3 == NULL)
		return;

	/**/

	catalog_list_change_to (priv->catalog_list, catalog_dir3);
	gth_location_set_catalog_uri (GTH_LOCATION (priv->location), catalog_dir3, reset_history);
	g_free (catalog_dir3);

	/* Update Go_Up command sensibility */

	current_path = priv->catalog_list->path;
	base_dir = get_catalog_full_path (NULL);
	set_action_sensitive (browser, "Go_Up", ! same_uri (current_path, base_dir));
	g_free (base_dir);
}


/* -- gth_browser_go_to_catalog -- */


void
gth_browser_go_to_catalog (GthBrowser *browser,
			   const char *catalog_path)
{
	GthBrowserPrivateData *priv;
	char                  *catalog_dir;

	g_return_if_fail (browser != NULL);
	g_return_if_fail (GTH_IS_BROWSER (browser));

	priv = browser->priv;

	/* go to the catalog directory */

	gth_browser_set_sidebar (browser, GTH_SIDEBAR_CATALOG_LIST);
	if (! priv->refreshing && ! ViewFirstImage)
		gth_browser_show_sidebar (browser);
	else
		priv->refreshing = FALSE;

	catalog_dir = remove_level_from_path (catalog_path);
	gth_browser_go_to_catalog_directory (browser, catalog_dir);
	g_free (catalog_dir);

	/* display the catalog */

	if ((catalog_path != NULL) && ! path_is_file (catalog_path)) {
		_gtk_error_dialog_run (GTK_WINDOW (browser),
				       _("The specified catalog does not exist."));
		window_update_location (browser);
		return;
	}

	if (catalog_path == NULL) {
		window_set_file_list (browser, NULL, priv->sort_method, priv->sort_type);
		if (priv->catalog_path)
			g_free (priv->catalog_path);
		priv->catalog_path = NULL;
		window_update_title (browser);
		window_image_viewer_set_void (browser);
		return;
	}

	if (priv->catalog_path != catalog_path) {
		if (priv->catalog_path)
			g_free (priv->catalog_path);
		priv->catalog_path = g_strdup (catalog_path);
	}

	catalog_activate (browser, catalog_path);
}


const char *
gth_browser_get_current_catalog (GthBrowser *browser)
{
	return browser->priv->catalog_path;
}


static gboolean
gth_browser_go_up__is_base_dir (GthBrowser *browser,
				const char *dir)
{
	GthBrowserPrivateData *priv = browser->priv;

	if (dir == NULL)
		return FALSE;

	if (priv->sidebar_content == GTH_SIDEBAR_DIR_LIST)
		return same_uri (dir, "file://");
	else {
		char     *catalog_base;
		gboolean  is_base_dir;

		catalog_base = get_catalog_full_path (NULL);
		is_base_dir = same_uri (dir, catalog_base);
		g_free (catalog_base);

		return is_base_dir;
	}

	return FALSE;
}


void
gth_browser_go_up (GthBrowser *browser)
{
	GthBrowserPrivateData *priv = browser->priv;
	char                  *current_path;
	char                  *up_dir;

	g_return_if_fail (browser != NULL);

	if (priv->sidebar_content == GTH_SIDEBAR_DIR_LIST)
		current_path = priv->dir_list->path;
	else
		current_path = priv->catalog_list->path;

	if (current_path == NULL)
		return;

	if (gth_browser_go_up__is_base_dir (browser, current_path))
		return;

	up_dir = g_strdup (current_path);
	do {
		char *tmp = up_dir;
		up_dir = remove_level_from_path (tmp);
		g_free (tmp);
	} while (! gth_browser_go_up__is_base_dir (browser, up_dir)
		 && ! path_is_dir (up_dir));

	if (priv->sidebar_content == GTH_SIDEBAR_DIR_LIST)
		gth_browser_go_to_directory (browser, up_dir);
	else {
		gth_browser_go_to_catalog (browser, NULL);
		gth_browser_go_to_catalog_directory (browser, up_dir);
	}

	g_free (up_dir);
}


static void
go_to_current_location (GthBrowser *browser,
			WindowGoOp  go_op)
{
	if (browser->priv->history_current == NULL)
		return;
	browser->priv->go_op = go_op;
	go_to_uri (browser, browser->priv->history_current->data, FALSE);
}


void
gth_browser_go_back (GthBrowser *browser)
{
	if (browser->priv->history_current == NULL)
		return;
	if (browser->priv->history_current->next == NULL)
		return;

	browser->priv->history_current = browser->priv->history_current->next;
	go_to_current_location (browser, GTH_BROWSER_GO_BACK);
}


void
gth_browser_go_forward (GthBrowser *browser)
{
	if (browser->priv->history_current == NULL)
		return;
	if (browser->priv->history_current->prev == NULL)
		return;

	browser->priv->history_current = browser->priv->history_current->prev;
	go_to_current_location (browser, GTH_BROWSER_GO_FORWARD);
}


void
gth_browser_delete_history (GthBrowser *browser)
{
	bookmarks_remove_all (browser->priv->history);
	browser->priv->history_current = NULL;
	window_update_history_list (browser);
}


/**/


static gboolean
view_focused_image (GthBrowser *browser)
{
	GthBrowserPrivateData *priv = browser->priv;
	int                    pos;
	char                  *focused;
	gboolean               not_focused;

	pos = gth_file_view_get_cursor (priv->file_list->view);
	if (pos == -1)
		return FALSE;

	focused = gth_file_list_path_from_pos (priv->file_list, pos);
	if (focused == NULL)
		return FALSE;

	not_focused = (priv->image == NULL) || ! same_uri (priv->image->path, focused);
	g_free (focused);

	return not_focused;
}


gboolean
gth_browser_show_next_image (GthBrowser *browser,
			     gboolean    only_selected)
{
	GthBrowserPrivateData *priv = browser->priv;
	gboolean               skip_broken;
	int                    pos;

	g_return_val_if_fail (browser != NULL, FALSE);

	skip_broken = FALSE;

	if (priv->image == NULL) {
		pos = gth_file_list_next_image (priv->file_list, -1, skip_broken, only_selected);
	} 
	else if (view_focused_image (browser)) {
		pos = gth_file_view_get_cursor (priv->file_list->view);
		if (pos == -1)
			pos = gth_file_list_next_image (priv->file_list, pos, skip_broken, only_selected);
	} 
	else {
		pos = gth_file_list_pos_from_path (priv->file_list, priv->image->path);
		pos = gth_file_list_next_image (priv->file_list, pos, skip_broken, only_selected);
	}

	if (pos != -1) {
		if (! only_selected)
			gth_file_list_select_image_by_pos (priv->file_list, pos);
		gth_file_view_set_cursor (priv->file_list->view, pos);
	}

	return (pos != -1);
}


gboolean
gth_browser_show_prev_image (GthBrowser *browser,
			     gboolean    only_selected)
{
	GthBrowserPrivateData *priv = browser->priv;
	gboolean               skip_broken;
	int                    pos;

	g_return_val_if_fail (browser != NULL, FALSE);

	skip_broken = FALSE;

	if (priv->image == NULL) {
		pos = gth_file_view_get_images (priv->file_list->view);
		pos = gth_file_list_prev_image (priv->file_list, pos, skip_broken, only_selected);
	} 
	else if (view_focused_image (browser)) {
		pos = gth_file_view_get_cursor (priv->file_list->view);
		if (pos == -1) {
			pos = gth_file_view_get_images (priv->file_list->view);
			pos = gth_file_list_prev_image (priv->file_list, pos, skip_broken, only_selected);
		}
	} 
	else {
		pos = gth_file_list_pos_from_path (priv->file_list, priv->image->path);
		pos = gth_file_list_prev_image (priv->file_list, pos, skip_broken, only_selected);
	}

	if (pos != -1) {
		if (! only_selected)
			gth_file_list_select_image_by_pos (priv->file_list, pos);
		gth_file_view_set_cursor (priv->file_list->view, pos);
	}

	return (pos != -1);
}


gboolean
gth_browser_show_first_image (GthBrowser *browser,
			      gboolean    only_selected)
{
	GthBrowserPrivateData *priv = browser->priv;

	if (gth_file_view_get_images (priv->file_list->view) == 0)
		return FALSE;

	if (priv->image != NULL) {
		file_data_unref (priv->image);
		priv->image = NULL;
		priv->image_position = -1;
	}

	return gth_browser_show_next_image (browser, only_selected);
}


gboolean
gth_browser_show_last_image (GthBrowser *browser,
			     gboolean    only_selected)
{
	GthBrowserPrivateData *priv = browser->priv;

	if (gth_file_view_get_images (priv->file_list->view) == 0)
		return FALSE;

	if (priv->image != NULL) {
		file_data_unref (priv->image);
		priv->image = NULL;
		priv->image_position = -1;
	}

	return gth_browser_show_prev_image (browser, only_selected);
}


void
gth_browser_show_image_prop (GthBrowser *browser)
{
	GthBrowserPrivateData *priv = browser->priv;

	if (priv->image_prop_dlg == NULL)
		priv->image_prop_dlg = dlg_image_prop_new (browser);
	else
		gtk_window_present (GTK_WINDOW (priv->image_prop_dlg));
}


void
gth_browser_set_image_prop_dlg (GthBrowser *browser,
				GtkWidget  *dialog)
{
	browser->priv->image_prop_dlg = dialog;
}


static ImageViewer *
gth_browser_get_image_viewer (GthWindow *window)
{
	GthBrowser *browser = GTH_BROWSER (window);
	return IMAGE_VIEWER (browser->priv->viewer);
}


static GThumbPreloader *
gth_browser_get_preloader (GthWindow *window)
{
	GthBrowser *browser = GTH_BROWSER (window);
	return browser->priv->preloader;
}


static FileData *
gth_browser_get_image_data (GthWindow *window)
{
	GthBrowser *browser = GTH_BROWSER (window);
	return browser->priv->image;
}


static gboolean
gth_browser_get_image_modified (GthWindow *window)
{
	GthBrowser *browser = GTH_BROWSER (window);
	return browser->priv->image_modified;
}


static void
gth_browser_set_image_modified (GthWindow *window,
				gboolean   modified)
{
	GthBrowser            *browser = GTH_BROWSER (window);
	GthBrowserPrivateData *priv = browser->priv;

	priv->image_modified = modified;
	window_update_infobar (browser);
	window_update_statusbar_image_info (browser);
	window_update_title (browser);

	set_action_sensitive (browser, "File_Save", ! image_viewer_is_void (IMAGE_VIEWER (priv->viewer)) && priv->image_modified);
	set_action_sensitive (browser, "File_Revert", ! image_viewer_is_void (IMAGE_VIEWER (priv->viewer)) && priv->image_modified);
	set_action_sensitive (browser, "Edit_Undo", gth_window_get_can_undo (window));
	set_action_sensitive (browser, "Edit_Redo", gth_window_get_can_redo (window));

	if (modified && priv->image_prop_dlg != NULL)
		dlg_image_prop_update (priv->image_prop_dlg);
}


/* -- load image -- */


static FileData*
get_image_to_preload (GthBrowser *browser,
		      int         pos,
		      int         priority)
{
	FileData  *fdata;
	int        max_size;
	char      *local_file;
	int        width = 0, height = 0;

	if (pos < 0)
		return NULL;
	if (pos >= gth_file_view_get_images (browser->priv->file_list->view))
		return NULL;

	fdata = gth_file_view_get_image_data (browser->priv->file_list->view, pos);
	if ((fdata == NULL) || ! is_local_file (fdata->path) || ! mime_type_is_image (fdata->mime_type)) {
		file_data_unref (fdata); 
		return NULL;
	}

	if (priority == 1)
		max_size = PRELOADED_IMAGE_MAX_DIM1;
	else
		max_size = PRELOADED_IMAGE_MAX_DIM2;

	if (fdata->size > max_size) {
		debug (DEBUG_INFO, "image %s filesize too large for preloading\n", fdata->path);
		file_data_unref (fdata);
		return NULL;
	}

	local_file = get_cache_filename_from_uri (fdata->path);
	gdk_pixbuf_get_file_info (local_file, &width, &height);

	debug (DEBUG_INFO, "%s dimensions: [%dx%d] <-> %d\n", local_file, width, height, max_size);

	if (width * height > max_size) {
		debug (DEBUG_INFO, "image %s dimensions are too large for preloading\n", local_file);
		file_data_unref (fdata);
		fdata = NULL;
	}
	g_free (local_file);

	return fdata;
}


static gboolean
load_timeout_cb (gpointer data)
{
	GthBrowser *browser = data;
	FileData   *prev1 = NULL;
	FileData   *next1 = NULL;

	if (browser->priv->view_image_timeout != 0) {
		g_source_remove (browser->priv->view_image_timeout);
		browser->priv->view_image_timeout = 0;
	}

	if (browser->priv->image == NULL)
		return FALSE;

	/* update the mtime in order to reload the image if required. */
	file_data_update_info (browser->priv->image);

	browser->priv->image_position = gth_file_list_pos_from_path (browser->priv->file_list, browser->priv->image->path);
	if (browser->priv->image_position >= 0) {
		prev1 = get_image_to_preload (browser, browser->priv->image_position - 1, 1);
		next1 = get_image_to_preload (browser, browser->priv->image_position + 1, 1);
	}

	set_cursor_busy (browser, FALSE);

	gthumb_preloader_load (browser->priv->preloader,
			       browser->priv->image,
			       next1,
			       prev1);

	file_data_unref (prev1);
	file_data_unref (next1);

	return FALSE;
}


void
gth_browser_reload_image (GthBrowser *browser)
{
	GthBrowserPrivateData *priv = browser->priv;

	g_return_if_fail (browser != NULL);

	if (priv->image == NULL)
		return;

	if (priv->view_image_timeout != 0)
		g_source_remove (priv->view_image_timeout);
	priv->view_image_timeout = g_idle_add (load_timeout_cb, browser);
}


static void
load_image__image_saved_cb (FileData *file,
			    gpointer  data)
{
	GthBrowser *browser = data;

	browser->priv->image_modified = FALSE;
	browser->priv->saving_modified_image = FALSE;
	gth_browser_load_image (browser, browser->priv->new_image);
}


void
gth_browser_load_image (GthBrowser *browser,
		        FileData   *file)
{
	GthBrowserPrivateData *priv = browser->priv;

	g_return_if_fail (browser != NULL);

	if (priv->image_modified) {
		if (priv->saving_modified_image)
			return;
		file_data_unref (priv->new_image);
		priv->new_image = file_data_dup (file);
		if (ask_whether_to_save (browser, load_image__image_saved_cb))
			return;
	}

	if (file == NULL) {
		window_image_viewer_set_void (browser);
		return;
	}

	if ((priv->image != NULL) && same_uri (file->path, priv->image->path)) {
		gth_browser_reload_image (browser);
		return;
	}

	file_data_update_info (file);

	if (! priv->image_modified
	    && (priv->image != NULL)
	    && (file != NULL)
	    && same_uri (file->path, priv->image->path)
	    && (priv->image->mtime == file->mtime))
		return;

	if (priv->view_image_timeout != 0) {
		g_source_remove (priv->view_image_timeout);
		priv->view_image_timeout = 0;
	}

	/* If the image is from a catalog remember the catalog name. */

	if (priv->image_catalog != NULL) {
		g_free (priv->image_catalog);
		priv->image_catalog = NULL;
	}
	if (priv->sidebar_content == GTH_SIDEBAR_CATALOG_LIST)
		priv->image_catalog = g_strdup (priv->catalog_path);

	/**/

	if (priv->image != NULL)
		file_data_unref (priv->image);
	priv->image = file_data_dup (file);
	priv->image_position = -1;

	priv->view_image_timeout = g_idle_add (load_timeout_cb, browser);
}


void
gth_browser_load_image_from_uri (GthBrowser *browser,
		                 const char *filename)
{
	FileData *file;
	
	file = file_data_new (filename, NULL);
	file_data_update_all (file, FALSE);
	gth_browser_load_image (browser, file);
	file_data_unref (file);
}


/* -- image operations -- */


static void
pixbuf_op_done_cb (GthPixbufOp *pixop,
		   gboolean     completed,
		   GthBrowser  *browser)
{
	GthBrowserPrivateData *priv = browser->priv;
	ImageViewer *viewer = IMAGE_VIEWER (priv->viewer);

	if (completed) {
		if (priv->pixop_preview)
			image_viewer_set_pixbuf (viewer, priv->pixop->dest);
		else {
			gth_window_set_image_pixbuf (GTH_WINDOW (browser), priv->pixop->dest);
			gth_window_set_image_modified (GTH_WINDOW (browser), TRUE);
		}
	}

	g_object_unref (priv->pixop);
	priv->pixop = NULL;

	if (priv->progress_dialog != NULL)
		gtk_widget_hide (priv->progress_dialog);
}


static void
pixbuf_op_progress_cb (GthPixbufOp *pixop,
		       float        p,
		       GthBrowser  *browser)
{
	GthBrowserPrivateData *priv = browser->priv;
	if (priv->progress_dialog != NULL)
		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (priv->progress_progressbar), p);
}


static int
window__display_progress_dialog (gpointer data)
{
	GthBrowser *browser = data;
	GthBrowserPrivateData *priv = browser->priv;

	if (priv->progress_timeout != 0) {
		g_source_remove (priv->progress_timeout);
		priv->progress_timeout = 0;
	}

	if (priv->pixop != NULL)
		gtk_widget_show (priv->progress_dialog);

	return FALSE;
}


static void
gth_browser_exec_pixbuf_op (GthWindow   *window,
			    GthPixbufOp *pixop,
			    gboolean     preview)
{
	GthBrowser            *browser = GTH_BROWSER (window);
	GthBrowserPrivateData *priv = browser->priv;

	if (priv->pixop != NULL)
		return;

	priv->pixop = pixop;
	g_object_ref (priv->pixop);
	priv->pixop_preview = preview;

	gtk_label_set_text (GTK_LABEL (priv->progress_info),
			    _("Wait please..."));

	g_signal_connect (G_OBJECT (pixop),
			  "pixbuf_op_done",
			  G_CALLBACK (pixbuf_op_done_cb),
			  browser);
	g_signal_connect (G_OBJECT (pixop),
			  "pixbuf_op_progress",
			  G_CALLBACK (pixbuf_op_progress_cb),
			  browser);

	if (priv->progress_dialog != NULL)
		priv->progress_timeout = g_timeout_add (DISPLAY_PROGRESS_DELAY, window__display_progress_dialog, browser);

	gth_pixbuf_op_start (priv->pixop);
}


static void
gth_browser_reload_current_image (GthWindow *window)
{
	GthBrowser *browser = GTH_BROWSER (window);
	gth_browser_reload_image (browser);
}


static void
gth_browser_update_current_image_metadata (GthWindow *window)
{
	GthBrowser *browser = GTH_BROWSER (window);

	if (browser->priv->image == NULL)
		return;
	gth_browser_notify_update_comment (browser, browser->priv->image->path);

	if (browser->priv->image_prop_dlg != NULL)
		dlg_image_prop_update (browser->priv->image_prop_dlg);
}


static GList *
gth_browser_get_file_list_selection (GthWindow *window)
{
	GthBrowser *browser = GTH_BROWSER (window);
	return gth_file_list_get_selection (browser->priv->file_list);
}


static GList *
gth_browser_get_file_list_selection_as_fd (GthWindow *window)
{
	GthBrowser *browser = GTH_BROWSER (window);
	return gth_file_list_get_selection_as_fd (browser->priv->file_list);
}


static void
gth_browser_set_animation (GthWindow *window,
			   gboolean   play)
{
	GthBrowser  *browser = GTH_BROWSER (window);
	ImageViewer *viewer = IMAGE_VIEWER (browser->priv->viewer);

	set_action_active (browser, "View_PlayAnimation", play);
	set_action_sensitive (browser, "View_StepAnimation", ! play);
	if (play)
		image_viewer_start_animation (viewer);
	else
		image_viewer_stop_animation (viewer);
}


static gboolean
gth_browser_get_animation (GthWindow *window)
{
	GthBrowser  *browser = GTH_BROWSER (window);
	ImageViewer *viewer = IMAGE_VIEWER (browser->priv->viewer);
	return viewer->play_animation;
}


static void
gth_browser_step_animation (GthWindow *window)
{
	GthBrowser  *browser = GTH_BROWSER (window);
	ImageViewer *viewer = IMAGE_VIEWER (browser->priv->viewer);
	image_viewer_step_animation (viewer);
}


static gboolean
fullscreen_destroy_cb (GtkWidget  *widget,
		       GthBrowser *browser)
{
	const char *current_image;
	int         pos;

	current_image = gth_window_get_image_filename (GTH_WINDOW (widget));

	browser->priv->fullscreen = NULL;
	gth_window_set_fullscreen (GTH_WINDOW (browser), FALSE);

	if ((current_image == NULL || browser->priv->image == NULL))
		return FALSE;

	if (same_uri (browser->priv->image->path, current_image))
		return FALSE;

	pos = gth_file_list_pos_from_path (browser->priv->file_list, current_image);
	if (pos != -1) {
		gthumb_preloader_set (browser->priv->preloader, gth_window_get_preloader (GTH_WINDOW (widget)));
		view_image_at_pos (browser, pos);
		gth_file_list_select_image_by_pos (browser->priv->file_list, pos);
	}

	return FALSE;
}


static void
_set_fullscreen_or_slideshow (GthWindow *window,
			      gboolean   _set,
			      gboolean   _slideshow)
{
	GthBrowser            *browser = GTH_BROWSER (window);
	GthBrowserPrivateData *priv = browser->priv;
	GdkPixbuf             *image;
	GList                 *file_list;

	if (!_set) {
		if (priv->fullscreen != NULL)
			gtk_widget_destroy (priv->fullscreen);
		return;
	}

	if (priv->fullscreen != NULL)
		return;

	file_list = gth_file_view_get_selection (priv->file_list->view);
	if ((file_list == NULL) || (g_list_length (file_list) == 1)) {
		file_data_list_free (file_list);
		file_list = gth_file_view_get_list (priv->file_list->view);
	}
	
	if (! priv->loading_image 
	    && ! image_viewer_is_animation (IMAGE_VIEWER (priv->viewer)) 
	    && ((priv->image != NULL) && mime_type_is_image (priv->image->mime_type)))
	{
		image = image_viewer_get_current_pixbuf (IMAGE_VIEWER (priv->viewer));
	}
	else 
		image = NULL;
	
	priv->fullscreen = gth_fullscreen_new (image, priv->image, file_list);
	file_data_list_free (file_list);
	
	g_signal_connect (priv->fullscreen,
			  "destroy",
			  G_CALLBACK (fullscreen_destroy_cb),
			  browser);
	
	gth_fullscreen_set_slideshow (GTH_FULLSCREEN (priv->fullscreen), _slideshow);
	if (priv->sidebar_content == GTH_SIDEBAR_CATALOG_LIST)
		gth_fullscreen_set_catalog (GTH_FULLSCREEN (priv->fullscreen), gth_browser_get_current_catalog (browser));

	gthumb_preloader_set (gth_window_get_preloader (GTH_WINDOW (priv->fullscreen)), browser->priv->preloader);

	gtk_widget_show (priv->fullscreen);
}


static void
gth_browser_set_fullscreen (GthWindow *window,
			    gboolean   fullscreen)
{
	_set_fullscreen_or_slideshow (window, fullscreen, FALSE);
}


static void
gth_browser_set_slideshow (GthWindow *window,
			   gboolean   slideshow)
{
	_set_fullscreen_or_slideshow (window, slideshow, TRUE);
}


static void
gth_browser_class_init (GthBrowserClass *class)
{
	GObjectClass   *gobject_class;
	GtkWidgetClass *widget_class;
	GthWindowClass *window_class;

	parent_class = g_type_class_peek_parent (class);
	gobject_class = (GObjectClass*) class;
	widget_class = (GtkWidgetClass*) class;
	window_class = (GthWindowClass*) class;

	gobject_class->finalize = gth_browser_finalize;

	widget_class->show = gth_browser_show;

	window_class->close = gth_browser_close;
	window_class->get_image_viewer = gth_browser_get_image_viewer;
	window_class->get_preloader = gth_browser_get_preloader;
	window_class->get_image_data = gth_browser_get_image_data;
	window_class->get_image_modified = gth_browser_get_image_modified;
	window_class->set_image_modified = gth_browser_set_image_modified;
	window_class->save_pixbuf = gth_browser_save_pixbuf;
	window_class->exec_pixbuf_op = gth_browser_exec_pixbuf_op;

	window_class->reload_current_image = gth_browser_reload_current_image;
	window_class->update_current_image_metadata = gth_browser_update_current_image_metadata;
	window_class->get_file_list_selection = gth_browser_get_file_list_selection;
	window_class->get_file_list_selection_as_fd = gth_browser_get_file_list_selection_as_fd;

	window_class->set_animation = gth_browser_set_animation;
	window_class->get_animation = gth_browser_get_animation;
	window_class->step_animation = gth_browser_step_animation;
	window_class->set_fullscreen = gth_browser_set_fullscreen;
	window_class->set_slideshow = gth_browser_set_slideshow;
}


GType
gth_browser_get_type (void)
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (GthBrowserClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_browser_class_init,
			NULL,
			NULL,
			sizeof (GthBrowser),
			0,
			(GInstanceInitFunc) gth_browser_init
		};

		type = g_type_register_static (GTH_TYPE_WINDOW,
					       "GthBrowser",
					       &type_info,
					       0);
	}

	return type;
}


void
gth_browser_set_bookmarks_dlg (GthBrowser *browser,
			       GtkWidget  *dialog)
{
	browser->priv->bookmarks_dlg = dialog;
}


GtkWidget *
gth_browser_get_bookmarks_dlg (GthBrowser *browser)
{
	return browser->priv->bookmarks_dlg;
}


GthFileList*
gth_browser_get_file_list (GthBrowser *browser)
{
	return browser->priv->file_list;
}


GthFileView*
gth_browser_get_file_view (GthBrowser *browser)
{
	return browser->priv->file_list->view;
}


GthDirList*
gth_browser_get_dir_list (GthBrowser *browser)
{
	return browser->priv->dir_list;
}


CatalogList*
gth_browser_get_catalog_list (GthBrowser *browser)
{
	return browser->priv->catalog_list;
}


void
gth_browser_load_uri (GthBrowser *browser,
		      const char *uri)
{
	go_to_uri (browser, uri, TRUE);
}


GtkWidget *
gth_browser_get_current_browser (void)
{
	if ((browser_list == NULL) || (g_list_length (browser_list) > 1))
		return NULL;
	else
		return (GtkWidget *) browser_list->data;
}
