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
#include <gtk/gtk.h>
#include <glade/glade.h>
#include <libgnomevfs/gnome-vfs-utils.h>

#ifdef HAVE_LIBEXIF
#include <libexif/exif-data.h>
#include "jpegutils/jpeg-data.h"
#endif /* HAVE_LIBEXIF */

#ifdef HAVE_LIBIPTCDATA
#include <libiptcdata/iptc-data.h>
#include <libiptcdata/iptc-jpeg.h>
#endif /* HAVE_LIBIPTCDATA */

#include "comments.h"
#include "dlg-save-image.h"
#include "dlg-categories.h"
#include "dlg-comment.h"
#include "file-data.h"
#include "file-utils.h"
#include "gconf-utils.h"
#include "glib-utils.h"
#include "gth-viewer.h"
#include "gth-viewer-ui.h"
#include "gth-exif-data-viewer.h"
#include "gth-exif-utils.h"
#include "gth-fullscreen.h"
#include "gthumb-info-bar.h"
#include "gtk-utils.h"
#include "main.h"
#include "nav-window.h"
#include "image-viewer.h"
#include "jpeg-utils.h"
#include "preferences.h"
#include "typedefs.h"

#include "icons/pixbufs.h"
#include "icons/nav_button.xpm"

#define GLADE_EXPORTER_FILE "gthumb_png_exporter.glade"
#define DEFAULT_WIN_WIDTH 200
#define DEFAULT_WIN_HEIGHT 400
#define DISPLAY_PROGRESS_DELAY 750
#define PANE_MIN_SIZE 60
#define GCONF_NOTIFICATIONS 3
#define ROTATE_TOOLITEM_POS 10


GthViewer *SingleViewer = NULL;
GthViewer *LastFocusedViewer = NULL;


enum {
	TARGET_PLAIN,
	TARGET_PLAIN_UTF8,
	TARGET_URILIST,
};

static GtkTargetEntry target_table[] = {
	{ "text/uri-list", 0, TARGET_URILIST },
};


struct _GthViewerPrivateData {
	GtkUIManager    *ui;
	GtkWidget       *toolbar;
	GtkWidget       *statusbar;
	GtkWidget       *image_popup_menu;
	GtkWidget       *info_bar;
	GtkWidget       *viewer;
	GtkWidget       *viewer_vscr;
	GtkWidget       *viewer_hscr;
	GtkWidget       *viewer_nav_event_box;
	GtkWidget       *image_data_hpaned;
	GtkWidget       *image_comment;
	GtkWidget       *exif_data_viewer;
	GtkWidget       *image_info;            /* statusbar widgets. */
	GtkWidget       *image_info_frame;
	GtkWidget       *zoom_info;
	GtkWidget       *zoom_info_frame;

	GtkWidget       *image_main_pane;

	GtkWidget       *comment_button;
	GtkWidget       *fullscreen;

	GtkActionGroup  *actions;

	GtkToolItem     *rotate_tool_item;

	GtkTooltips     *tooltips;
        guint            help_message_cid;
        guint            image_info_cid;
        guint            progress_cid;

	gboolean         first_time_show;
	guint            first_timeout_handle;

	gboolean         image_data_visible;
	char            *image_path;
	time_t           image_mtime; 
	gboolean         image_error;

#ifdef HAVE_LIBEXIF
	ExifData        *exif_data;
#endif /* HAVE_LIBEXIF */

#ifdef HAVE_LIBIPTCDATA
	IptcData        *iptc_data;
#endif /* HAVE_LIBIPTCDATA */

	/* misc */

	guint            cnxn_id[GCONF_NOTIFICATIONS];

	/* save_pixbuf data */

	gboolean         image_modified;
	gboolean         saving_modified_image;
	ImageSavedFunc   image_saved_func;
	char            *new_image_path;

	GthPixbufOp     *pixop;
	gboolean         pixop_preview;
	gboolean         closing;
       
	/* progress dialog */

	GladeXML        *progress_gui;
	GtkWidget       *progress_dialog;
	GtkWidget       *progress_progressbar;
	GtkWidget       *progress_info;
	guint            progress_timeout;
};

static GthWindowClass *parent_class = NULL;


static void
set_action_active (GthViewer   *viewer,
		   const char  *action_name, 
		   gboolean     is_active)
{
	GtkAction *action;
	action = gtk_action_group_get_action (viewer->priv->actions, action_name);
	gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), is_active);
}


static void
set_action_sensitive (GthViewer  *viewer,
	       const char *action_name, 
	       gboolean    sensitive)
{
	GtkAction *action;
	action = gtk_action_group_get_action (viewer->priv->actions, action_name);
	g_object_set (action, "sensitive", sensitive, NULL);
}


static void
viewer_update_zoom_sensitivity (GthViewer *viewer)
{
	GthViewerPrivateData  *priv = viewer->priv;
	ImageViewer           *image_viewer = (ImageViewer*) priv->viewer;
	gboolean               image_is_visible;
	gboolean               image_is_void;
	gboolean               fit;
	int                    zoom;

	image_is_visible = (priv->image_path != NULL) && ! priv->image_error;
	image_is_void = image_viewer_is_void (image_viewer);
	zoom = (int) (image_viewer->zoom_level * 100.0);
	fit = image_viewer_is_zoom_to_fit (image_viewer) || image_viewer_is_zoom_to_fit_if_larger (image_viewer);

	set_action_sensitive (viewer, 
		       "View_Zoom100",
		       image_is_visible && !image_is_void && (zoom != 100));
	set_action_sensitive (viewer, 
		       "View_ZoomIn",
		       image_is_visible && !image_is_void && (zoom != 10000));
	set_action_sensitive (viewer, 
		       "View_ZoomOut",
		       image_is_visible && !image_is_void && (zoom != 5));
	set_action_sensitive (viewer, 
		       "View_ZoomFit",
		       image_is_visible && !image_is_void);
}


static void
viewer_update_sensitivity (GthViewer *viewer)
{
	GthViewerPrivateData *priv = viewer->priv;
	gboolean image_is_void;
	gboolean image_is_ani;
	gboolean playing;

	image_is_void = image_viewer_is_void (IMAGE_VIEWER (priv->viewer));
	image_is_ani = image_viewer_is_animation (IMAGE_VIEWER (priv->viewer));
	playing = image_viewer_is_playing_animation (IMAGE_VIEWER (priv->viewer));

	set_action_sensitive (viewer, "File_Save", ! image_is_void && priv->image_modified);
	set_action_sensitive (viewer, "File_SaveAs", ! image_is_void);
	set_action_sensitive (viewer, "File_Revert", ! image_is_void && priv->image_modified);
	set_action_sensitive (viewer, "File_Print", ! image_is_void);

	set_action_sensitive (viewer, "Edit_Undo", gth_window_get_can_undo (GTH_WINDOW (viewer)));
	set_action_sensitive (viewer, "Edit_Redo", gth_window_get_can_redo (GTH_WINDOW (viewer)));

	set_action_sensitive (viewer, "AlterImage_Rotate90", ! image_is_void && ! image_is_ani);
	set_action_sensitive (viewer, "AlterImage_Rotate90CC", ! image_is_void && ! image_is_ani);
	set_action_sensitive (viewer, "AlterImage_Flip", ! image_is_void && ! image_is_ani);
	set_action_sensitive (viewer, "AlterImage_Mirror", ! image_is_void && ! image_is_ani);
	set_action_sensitive (viewer, "AlterImage_Desaturate", ! image_is_void && ! image_is_ani);
	set_action_sensitive (viewer, "AlterImage_Resize", ! image_is_void && ! image_is_ani);
	set_action_sensitive (viewer, "AlterImage_ColorBalance", ! image_is_void && ! image_is_ani);
	set_action_sensitive (viewer, "AlterImage_HueSaturation", ! image_is_void && ! image_is_ani);
	set_action_sensitive (viewer, "AlterImage_BrightnessContrast", ! image_is_void && ! image_is_ani);
	set_action_sensitive (viewer, "AlterImage_Invert", ! image_is_void && ! image_is_ani);
	set_action_sensitive (viewer, "AlterImage_Posterize", ! image_is_void && ! image_is_ani);
	set_action_sensitive (viewer, "AlterImage_Equalize", ! image_is_void && ! image_is_ani);
	set_action_sensitive (viewer, "AlterImage_AdjustLevels", ! image_is_void && ! image_is_ani);
	set_action_sensitive (viewer, "AlterImage_Crop", ! image_is_void && ! image_is_ani);
	set_action_sensitive (viewer, "AlterImage_Dither_BW", ! image_is_void && ! image_is_ani);
	set_action_sensitive (viewer, "AlterImage_Dither_Web", ! image_is_void && ! image_is_ani);

	set_action_sensitive (viewer, "View_PlayAnimation", image_is_ani);
	set_action_sensitive (viewer, "View_StepAnimation", image_is_ani && ! playing);

	viewer_update_zoom_sensitivity (viewer);
}


static void 
gth_viewer_finalize (GObject *object)
{
	GthViewer *viewer = GTH_VIEWER (object);
	int        i;

	debug (DEBUG_INFO, "Gth::Viewer::Finalize");

	for (i = 0; i < GCONF_NOTIFICATIONS; i++)
		if (viewer->priv->cnxn_id[i] != -1)
			eel_gconf_notification_remove (viewer->priv->cnxn_id[i]);

	if (viewer->priv != NULL) {
		GthViewerPrivateData *priv = viewer->priv;

#ifdef HAVE_LIBEXIF
		if (priv->exif_data != NULL) {
			exif_data_unref (priv->exif_data);
			priv->exif_data = NULL;
		}
#endif /* HAVE_LIBEXIF */
		
#ifdef HAVE_LIBIPTCDATA
		if (priv->iptc_data != NULL) {
			iptc_data_unref (priv->iptc_data);
			priv->iptc_data = NULL;
		}
#endif /* HAVE_LIBIPTCDATA */

		g_free (priv->image_path);
		g_free (priv->new_image_path);

		g_free (viewer->priv);
		viewer->priv = NULL;
	}

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
menu_item_select_cb (GtkMenuItem *proxy,
		     GthViewer   *viewer)
{
        GtkAction *action;
        char      *message;

        action = g_object_get_data (G_OBJECT (proxy),  "gtk-action");
        g_return_if_fail (action != NULL);

        g_object_get (G_OBJECT (action), "tooltip", &message, NULL);
        if (message) {
		gtk_statusbar_push (GTK_STATUSBAR (viewer->priv->statusbar),
				    viewer->priv->help_message_cid, message);
		g_free (message);
        }
}


static void
menu_item_deselect_cb (GtkMenuItem *proxy,
		       GthViewer   *viewer)
{
        gtk_statusbar_pop (GTK_STATUSBAR (viewer->priv->statusbar),
                           viewer->priv->help_message_cid);
}


static void
disconnect_proxy_cb (GtkUIManager *manager,
                     GtkAction    *action,
                     GtkWidget    *proxy,
                     GthViewer    *viewer)
{
        if (GTK_IS_MENU_ITEM (proxy)) {
                g_signal_handlers_disconnect_by_func
                        (proxy, G_CALLBACK (menu_item_select_cb), viewer);
                g_signal_handlers_disconnect_by_func
                        (proxy, G_CALLBACK (menu_item_deselect_cb), viewer);
        }
}


static void
connect_proxy_cb (GtkUIManager *manager,
                  GtkAction    *action,
                  GtkWidget    *proxy,
		  GthViewer    *viewer)
{
        if (GTK_IS_MENU_ITEM (proxy)) {
		g_signal_connect (proxy, "select",
				  G_CALLBACK (menu_item_select_cb), viewer);
		g_signal_connect (proxy, "deselect",
				  G_CALLBACK (menu_item_deselect_cb), viewer);
	}
}


static void
gth_viewer_set_toolbar_visibility (GthViewer *viewer,
				   gboolean   visible)
{
	g_return_if_fail (viewer != NULL);

	set_action_active (viewer, "View_Toolbar", visible);
	if (visible)
		gtk_widget_show (viewer->priv->toolbar->parent);
	else
		gtk_widget_hide (viewer->priv->toolbar->parent);
}


static void
pref_ui_toolbar_visible_changed (GConfClient *client,
				 guint        cnxn_id,
				 GConfEntry  *entry,
				 gpointer     user_data)
{
	GthViewer *viewer = user_data;
	gth_viewer_set_toolbar_visibility (viewer, gconf_value_get_bool (gconf_entry_get_value (entry)));
}


static void
gth_viewer_set_statusbar_visibility  (GthViewer *viewer,
				      gboolean   visible)
{
	g_return_if_fail (viewer != NULL);

	set_action_active (viewer, "View_Statusbar", visible);
	if (visible) 
		gtk_widget_show (viewer->priv->statusbar);
	else
		gtk_widget_hide (viewer->priv->statusbar);
}


static void
pref_ui_statusbar_visible_changed (GConfClient *client,
				   guint        cnxn_id,
				   GConfEntry  *entry,
				   gpointer     user_data)
{
	GthViewer *viewer = user_data;
	gth_viewer_set_statusbar_visibility (viewer, gconf_value_get_bool (gconf_entry_get_value (entry)));
}


static void
pref_ui_single_window_changed (GConfClient *client,
			       guint        cnxn_id,
			       GConfEntry  *entry,
			       gpointer     user_data)
{
	GthViewer *viewer = user_data;
	gboolean   value = gconf_value_get_bool (gconf_entry_get_value (entry));

	set_action_active (viewer, "View_SingleWindow", value);
}


static void 
gth_viewer_realize (GtkWidget *widget)
{
	GthViewer *viewer;
	GthViewerPrivateData *priv;

	viewer = GTH_VIEWER (widget);
	priv = viewer->priv;

	GTK_WIDGET_CLASS (parent_class)->realize (widget);
}


static void
save_window_size (GthViewer *viewer)
{
	int w, h;

	gdk_drawable_get_size (GTK_WIDGET (viewer)->window, &w, &h);
	eel_gconf_set_integer (PREF_UI_VIEWER_WIDTH, w);
	eel_gconf_set_integer (PREF_UI_VIEWER_HEIGHT, h);
}


static void 
gth_viewer_unrealize (GtkWidget *widget)
{
	GthViewer            *viewer;
	GthViewerPrivateData *priv;

	viewer = GTH_VIEWER (widget);
	priv = viewer->priv;

	/* save ui preferences. */

	save_window_size (viewer);
	eel_gconf_set_integer (PREF_UI_COMMENT_PANE_SIZE, gtk_paned_get_position (GTK_PANED (priv->image_main_pane)));

	GTK_WIDGET_CLASS (parent_class)->unrealize (widget);
}


static gboolean
first_time_idle (gpointer callback_data)
{
	GthViewer            *viewer = callback_data;
	GthViewerPrivateData *priv = viewer->priv;

	g_source_remove (priv->first_timeout_handle);

	gtk_widget_grab_focus (priv->viewer);
	if (priv->image_path != NULL)
		image_viewer_load_image (IMAGE_VIEWER (priv->viewer), priv->image_path);

	return FALSE;
}


static void 
gth_viewer_show (GtkWidget *widget)
{
	GthViewer *viewer = GTH_VIEWER (widget);
	gboolean   view_foobar;

	GTK_WIDGET_CLASS (parent_class)->show (widget);

	if (!viewer->priv->first_time_show) 
		return;
	viewer->priv->first_time_show = FALSE;

	view_foobar = eel_gconf_get_boolean (PREF_UI_TOOLBAR_VISIBLE, TRUE);
	gth_viewer_set_toolbar_visibility (viewer, view_foobar);

	view_foobar = eel_gconf_get_boolean (PREF_UI_STATUSBAR_VISIBLE, TRUE);
	gth_viewer_set_statusbar_visibility (viewer, view_foobar);

	if (viewer->priv->first_timeout_handle == 0)
		viewer->priv->first_timeout_handle = g_idle_add (first_time_idle, viewer);
}


static void
viewer_update_statusbar_zoom_info (GthViewer *viewer)
{
	GthViewerPrivateData *priv = viewer->priv;
	const char           *path;
	gboolean              image_is_visible;
	int                   zoom;
	char                 *text;

	viewer_update_zoom_sensitivity (viewer);

	/**/

	path = priv->image_path;

	image_is_visible = (path != NULL) && !priv->image_error;
	if (! image_is_visible) {
		if (! GTK_WIDGET_VISIBLE (priv->zoom_info_frame))
			return;
		gtk_widget_hide (priv->zoom_info_frame);
		return;
	} 

	if (! GTK_WIDGET_VISIBLE (priv->zoom_info_frame)) 
		gtk_widget_show (priv->zoom_info_frame);

	zoom = (int) (IMAGE_VIEWER (priv->viewer)->zoom_level * 100.0);
	text = g_strdup_printf (" %d%% ", zoom);
	gtk_label_set_text (GTK_LABEL (priv->zoom_info), text);
	g_free (text);
}


static void
viewer_update_statusbar_image_info (GthViewer *viewer)
{
	GthViewerPrivateData *priv = viewer->priv;
	char                 *text;
	char                  time_txt[50], *utf8_time_txt;
	char                 *size_txt;
	char                 *file_size_txt;
	const char           *path;
	int                   width, height;
	time_t                timer = 0;
	struct tm            *tm;
	gdouble               sec;

	path = priv->image_path;

	if ((path == NULL) || priv->image_error) {
		if (! GTK_WIDGET_VISIBLE (priv->image_info_frame))
			return;
		gtk_widget_hide (priv->image_info_frame);
		return;

	} else if (! GTK_WIDGET_VISIBLE (priv->image_info_frame)) 
		gtk_widget_show (priv->image_info_frame);

	if (!image_viewer_is_void (IMAGE_VIEWER (priv->viewer))) {
		width = image_viewer_get_image_width (IMAGE_VIEWER (priv->viewer));
		height = image_viewer_get_image_height (IMAGE_VIEWER (priv->viewer)); 
	} else {
		width = 0;
		height = 0;
	}

#ifdef HAVE_LIBEXIF
	timer = get_exif_time (path);
#endif
	if (timer == 0)
		timer = get_file_mtime (path);
	tm = localtime (&timer);
	strftime (time_txt, 50, _("%d %B %Y, %H:%M"), tm);
	utf8_time_txt = g_locale_to_utf8 (time_txt, -1, 0, 0, 0);
	sec = g_timer_elapsed (image_loader_get_timer (IMAGE_VIEWER (priv->viewer)->loader),  NULL);

	size_txt = g_strdup_printf (_("%d x %d pixels"), width, height);
	file_size_txt = gnome_vfs_format_file_size_for_display (get_file_size (path));

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

	gtk_label_set_text (GTK_LABEL (priv->image_info), text);

	/**/

	g_free (size_txt);
	g_free (file_size_txt);
	g_free (text);
	g_free (utf8_time_txt);
}


static void
update_image_comment (GthViewer *viewer)
{
	GthViewerPrivateData *priv = viewer->priv;
	CommentData          *cdata;
	char                 *comment;
	GtkTextBuffer        *text_buffer;

#ifdef HAVE_LIBIPTCDATA
	if (priv->iptc_data != NULL) {
		iptc_data_unref (priv->iptc_data);
		priv->iptc_data = NULL;
	}

	if (priv->image_path != NULL) 
		priv->iptc_data = iptc_data_new_from_jpeg (priv->image_path);
#endif /* HAVE_LIBIPTCDATA */

	text_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->image_comment));

	if (priv->image_path == NULL) {
		GtkTextIter start, end;
		gtk_text_buffer_get_bounds (text_buffer, &start, &end);
		gtk_text_buffer_delete (text_buffer, &start, &end);
                return;
	}

	cdata = comments_load_comment (priv->image_path, TRUE);

	if (cdata == NULL) {
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

	g_free (comment);
	comment_data_free (cdata);
}


static void
viewer_update_image_info (GthViewer *viewer)
{
	GthViewerPrivateData *priv = viewer->priv;

	viewer_update_statusbar_image_info (viewer);
	viewer_update_statusbar_zoom_info (viewer);


#ifdef HAVE_LIBEXIF
	{
		JPEGData *jdata = NULL;

		if (priv->exif_data != NULL) {
			exif_data_unref (priv->exif_data);
			priv->exif_data = NULL;
		}
		
		if (priv->image_path != NULL)
			jdata = jpeg_data_new_from_file (priv->image_path);
		if (jdata != NULL) {
			priv->exif_data = jpeg_data_get_exif_data (jdata);
			jpeg_data_unref (jdata);
		}
	}
#endif /* HAVE_LIBEXIF */

	gth_exif_data_viewer_update (GTH_EXIF_DATA_VIEWER (priv->exif_data_viewer), 
				     IMAGE_VIEWER (priv->viewer),
				     priv->image_path,
#ifdef HAVE_LIBEXIF
				     viewer->priv->exif_data
#else /* ! HAVE_LIBEXIF */
				     NULL
#endif /* ! HAVE_LIBEXIF */
				     );

	update_image_comment (viewer);
}


static void
viewer_update_infobar (GthViewer *viewer)
{
	GthViewerPrivateData *priv = viewer->priv;
	char                 *text;
	char                 *escaped_name;
	char                 *utf8_name;
	/*int                   images, current;*/

	if (priv->image_path == NULL) {
		gthumb_info_bar_set_text (GTHUMB_INFO_BAR (priv->info_bar), 
					  NULL, NULL);
		return;
	}

	utf8_name = g_filename_display_basename (priv->image_path);
	escaped_name = g_markup_escape_text (utf8_name, -1);

	text = g_strdup_printf ("<b>%s</b> %s", 
				escaped_name,
				priv->image_modified ? _("[modified]") : "");

	gthumb_info_bar_set_text (GTHUMB_INFO_BAR (priv->info_bar), 
				  text, 
				  NULL);

	g_free (utf8_name);
	g_free (escaped_name);
	g_free (text);
}


static void
viewer_update_title (GthViewer *viewer)
{
	GthViewerPrivateData *priv = viewer->priv;
	char                 *title = NULL;
	char                 *path;
	char                 *modified;

	path = priv->image_path;
	modified = priv->image_modified ? _("[modified]") : "";

	if (path == NULL) 
		title = g_strdup (_("No image"));

	else {
		char *image_name = g_filename_display_basename (path);
		title = g_strdup_printf ("%s %s", image_name, modified);
		g_free (image_name);
	}

	gtk_window_set_title (GTK_WINDOW (viewer), title);
	g_free (title);
}


/* ask_whether_to_save */


static void
save_jpeg_data (GthViewer  *viewer,
		const char *filename)
{
#ifdef HAVE_LIBEXIF
	GthViewerPrivateData  *priv = viewer->priv;
	gboolean               data_to_save = FALSE;
	JPEGData              *jdata;

	if (priv->exif_data != NULL) 
		data_to_save = TRUE;
	
#ifdef HAVE_LIBIPTCDATA
	if (priv->iptc_data != NULL) 
		data_to_save = TRUE;
#endif /* HAVE_LIBIPTCDATA */

	if (!data_to_save)
		return;

	jdata = jpeg_data_new_from_file (filename);
	if (jdata == NULL) 
		return;

#ifdef HAVE_LIBIPTCDATA
	if (priv->iptc_data != NULL) {
		unsigned char *out_buf, *iptc_buf;
		int            iptc_len, ps3_len;

		out_buf = g_malloc (256*256);
		iptc_data_save (priv->iptc_data, &iptc_buf, &iptc_len);
		ps3_len = iptc_jpeg_ps3_save_iptc (NULL, 0, iptc_buf,
						   iptc_len, out_buf, 256*256);
		iptc_data_free_buf (priv->iptc_data, iptc_buf);
		if (ps3_len > 0)
			jpeg_data_set_header_data (jdata,
						   JPEG_MARKER_APP13,
						   out_buf, ps3_len);
		g_free (out_buf);
	}
#endif /* HAVE_LIBIPTCDATA */

	if (priv->exif_data != NULL) 
		jpeg_data_set_exif_data (jdata, priv->exif_data);

	jpeg_data_save_file (jdata, filename);
	jpeg_data_unref (jdata);

#endif /* HAVE_LIBEXIF */
}


static void
save_pixbuf__image_saved_cb (const char *filename,
			     gpointer    data)
{
	GthViewer            *viewer = data;
	GthViewerPrivateData *priv = viewer->priv;

	if (filename == NULL) 
		return;

	save_jpeg_data (viewer, filename);

	/**/

	priv->image_modified = FALSE;
	priv->saving_modified_image = FALSE;

	if (priv->closing)
		return;

	if (strcmp (priv->image_path, filename) != 0) {
		/*FIXME: gtk_widget_show (gth_viewer_new (filename));*/
		gth_viewer_load (viewer, filename);

	} else {
		viewer_update_statusbar_image_info (viewer);
		viewer_update_image_info (viewer);
		viewer_update_title (viewer);
		viewer_update_infobar (viewer);
		viewer_update_sensitivity (viewer);
	}
}


static void
ask_whether_to_save__image_saved_cb (const char *filename,
				     gpointer    data)
{
	GthViewer *viewer = data;

	save_pixbuf__image_saved_cb (filename, data);

	if (viewer->priv->image_saved_func != NULL)
		(*viewer->priv->image_saved_func) (NULL, viewer);
}


static void
ask_whether_to_save__response_cb (GtkWidget *dialog,
				  int        response_id,
				  GthViewer *viewer)
{
	GthViewerPrivateData *priv = viewer->priv;

        gtk_widget_destroy (dialog);
	
        if (response_id == GTK_RESPONSE_YES) {
		dlg_save_image_as (GTK_WINDOW (viewer),
				   priv->image_path,
				   image_viewer_get_current_pixbuf (IMAGE_VIEWER (priv->viewer)),
				   ask_whether_to_save__image_saved_cb,
				   viewer);
		priv->saving_modified_image = TRUE;

	} else {
		priv->saving_modified_image = FALSE;
		priv->image_modified = FALSE;
		if (priv->image_saved_func != NULL)
			(*priv->image_saved_func) (NULL, viewer);
	}
}


static gboolean
ask_whether_to_save (GthViewer      *viewer,
		     ImageSavedFunc  image_saved_func)
{
	GthViewerPrivateData *priv = viewer->priv;
	GtkWidget            *d;

	if (! eel_gconf_get_boolean (PREF_MSG_SAVE_MODIFIED_IMAGE, TRUE)) 
		return FALSE;
		
	d = _gtk_yesno_dialog_with_checkbutton_new (
			    GTK_WINDOW (viewer),
			    GTK_DIALOG_MODAL,
			    _("The current image has been modified, do you want to save it?"),
			    _("Do Not Save"),
			    GTK_STOCK_SAVE_AS,
			    _("_Do not display this message again"),
			    PREF_MSG_SAVE_MODIFIED_IMAGE);

	priv->saving_modified_image = TRUE;
	priv->image_saved_func = image_saved_func;
	g_signal_connect (G_OBJECT (d), 
			  "response",
			  G_CALLBACK (ask_whether_to_save__response_cb),
			  viewer);

	gtk_widget_show (d);

	return TRUE;
}


static gboolean
info_bar_clicked_cb (GtkWidget      *widget,
		     GdkEventButton *event,
		     GthViewer      *viewer)
{
	gtk_widget_grab_focus (viewer->priv->viewer);
	return TRUE;
}


static void
real_set_void (const char *filename,
	       gpointer    data)
{
	GthViewer            *viewer = data;
	GthViewerPrivateData *priv = viewer->priv;

	if (!priv->image_error) {
		g_free (priv->image_path);
		priv->image_path = NULL;
		priv->image_mtime = 0;
		priv->image_modified = FALSE;
	}

	image_viewer_set_void (IMAGE_VIEWER (priv->viewer));

	viewer_update_statusbar_image_info (viewer);
 	viewer_update_image_info (viewer);
	viewer_update_title (viewer);
	viewer_update_infobar (viewer);
	viewer_update_sensitivity (viewer);
}


static void
viewer_set_void (GthViewer *viewer,
		 gboolean   error)
{
	GthViewerPrivateData *priv = viewer->priv;

	priv->image_error = error;
	if (priv->image_modified)
		if (ask_whether_to_save (viewer, real_set_void))
			return;
	real_set_void (NULL, viewer);
}


static void
image_loaded_cb (GtkWidget  *widget, 
		 GthViewer  *viewer)
{
	GthViewerPrivateData *priv = viewer->priv;

	if (image_viewer_is_void (IMAGE_VIEWER (priv->viewer))) {
		viewer_set_void (viewer, TRUE);
		return;
	}

	priv->image_mtime = get_file_mtime (priv->image_path);
	priv->image_modified = FALSE;

	viewer_update_image_info (viewer);
	viewer_update_infobar (viewer);
	viewer_update_title (viewer);
	viewer_update_sensitivity (viewer);

	if (StartInFullscreen) {
		StartInFullscreen = FALSE;
		gth_window_set_fullscreen (GTH_WINDOW (viewer), TRUE);
	}
}


static gboolean
zoom_changed_cb (GtkWidget  *widget, 
		 GthViewer  *viewer)
{
	viewer_update_statusbar_zoom_info (viewer);
	return TRUE;	
}


static void  
viewer_drag_data_get  (GtkWidget        *widget,
		       GdkDragContext   *context,
		       GtkSelectionData *selection_data,
		       guint             info,
		       guint             time,
		       gpointer          data)
{
	GthViewer            *viewer = data;
	GthViewerPrivateData *priv = viewer->priv;
	char                 *path;

	if (IMAGE_VIEWER (priv->viewer)->is_void) 
		return;

	path = image_viewer_get_image_filename (IMAGE_VIEWER (priv->viewer));
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
	GthViewer *viewer = extra_data;
	GList     *list;
	GList     *scan;

	if (! ((data->length >= 0) && (data->format == 8))) {
		gtk_drag_finish (context, FALSE, FALSE, time);
		return;
	}

	gtk_drag_finish (context, TRUE, FALSE, time);

	list = get_file_list_from_url_list ((char *) data->data);
	for (scan = list; scan; scan = scan->next) {
		char *filename = scan->data;
		if (scan == list)
			gth_viewer_load (viewer, filename);
		else 
			gtk_widget_show (gth_viewer_new (filename));
	}

	path_list_free (list);
}


static gboolean
viewer_key_press_cb (GtkWidget   *widget, 
		     GdkEventKey *event,
		     gpointer     data)
{
	GthViewer   *viewer = data;
	ImageViewer *image_viewer = (ImageViewer*)viewer->priv->viewer;
	gboolean     retval = FALSE;

	switch (event->keyval) {
		/* Toggle comment visibility. */
	case GDK_i:
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (viewer->priv->comment_button),
					      !gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (viewer->priv->comment_button)));
		retval = TRUE;
		break;

		/* Zoom in. */
	case GDK_plus:
	case GDK_equal:
	case GDK_KP_Add:
		image_viewer_zoom_in (image_viewer);
		retval = TRUE;
		break;

		/* Zoom out. */
	case GDK_minus:
	case GDK_KP_Subtract:
		image_viewer_zoom_out (image_viewer);
		retval = TRUE;
		break;
		
		/* Actual size. */
	case GDK_KP_Divide:
	case GDK_1:
	case GDK_z:
		image_viewer_set_zoom (image_viewer, 1.0);
		retval = TRUE;
		break;

		/* Set zoom to 2.0. */
	case GDK_2:
		image_viewer_set_zoom (image_viewer, 2.0);
		retval = TRUE;
		break;

		/* Set zoom to 3.0. */
	case GDK_3:
		image_viewer_set_zoom (image_viewer, 3.0);
		retval = TRUE;
		break;

		/* Zoom to fit */
	case GDK_x:
		gth_window_activate_action_view_zoom_fit (NULL, viewer);
		retval = TRUE;
		break;

		/* Rotate image */
	case GDK_bracketright:
	case GDK_r: 
		gth_window_activate_action_alter_image_rotate90 (NULL, viewer);
		retval = TRUE;
		break;
			
		/* Flip image */
	case GDK_l:
		gth_window_activate_action_alter_image_flip (NULL, viewer);
		retval = TRUE;
		break;

		/* Mirror image */
	case GDK_m:
		gth_window_activate_action_alter_image_mirror (NULL, viewer);
		retval = TRUE;
		break;

		/* Rotate image counter-clockwise */
	case GDK_bracketleft:
		gth_window_activate_action_alter_image_rotate90cc (NULL, viewer);
		retval = TRUE;
		break;

		/* Full screen view. */
	case GDK_v:
	case GDK_f:
		gth_window_set_fullscreen (GTH_WINDOW (viewer), TRUE);
		retval = TRUE;
		break;

	default:
		break;
	}

	return retval;
}


static gboolean
size_changed_cb (GtkWidget *widget, 
		 GthViewer *viewer)
{
	GthViewerPrivateData *priv = viewer->priv;
	GtkAdjustment        *vadj, *hadj;
	gboolean              hide_vscr, hide_hscr;

	vadj = IMAGE_VIEWER (priv->viewer)->vadj;
	hadj = IMAGE_VIEWER (priv->viewer)->hadj;

	hide_vscr = vadj->upper <= vadj->page_size;
	hide_hscr = hadj->upper <= hadj->page_size;

	if (hide_vscr && hide_hscr) {
		gtk_widget_hide (priv->viewer_vscr); 
		gtk_widget_hide (priv->viewer_hscr); 
		gtk_widget_hide (priv->viewer_nav_event_box);
	} else {
		gtk_widget_show (priv->viewer_vscr); 
		gtk_widget_show (priv->viewer_hscr); 
		gtk_widget_show (priv->viewer_nav_event_box);
	}

	return TRUE;	
}


static gboolean
image_focus_changed_cb (GtkWidget     *widget,
			GdkEventFocus *event,
			gpointer       data)
{
	GthViewer            *viewer = data;
	GthViewerPrivateData *priv = viewer->priv;
	
	gthumb_info_bar_set_focused (GTHUMB_INFO_BAR (priv->info_bar),
				     GTK_WIDGET_HAS_FOCUS (priv->viewer));
	
	return FALSE;
}


static int
image_comment_button_press_cb (GtkWidget      *widget, 
			       GdkEventButton *event,
			       gpointer        data)
{
	GthViewer *viewer = data;

	if ((event->button == 1) && (event->type == GDK_2BUTTON_PRESS)) {
		gth_window_edit_comment (GTH_WINDOW (viewer)); 
		return TRUE;
	}

	return FALSE;
}


void
gth_viewer_set_metadata_visible (GthViewer *viewer,
				 gboolean   visible)
{
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (viewer->priv->comment_button), visible);
}


void
gth_viewer_set_single_window (GthViewer *viewer,
			      gboolean   value)
{
	SingleViewer = NULL;
	if (value && (LastFocusedViewer != NULL)) 
		SingleViewer = LastFocusedViewer;
	eel_gconf_set_boolean (PREF_SINGLE_WINDOW, value);
}


static gboolean
comment_button_toggled_cb (GtkToggleButton *button,
			   GthViewer       *viewer)
{
	gboolean visible = gtk_toggle_button_get_active (button);

	set_action_active (viewer, "View_ShowInfo", visible);
	viewer->priv->image_data_visible = visible;
	if (visible)
		gtk_widget_show (viewer->priv->image_data_hpaned);
	else
		gtk_widget_hide (viewer->priv->image_data_hpaned);

	return TRUE;
}


static void
zoom_quality_radio_action (GtkAction      *action,
			   GtkRadioAction *current,
			   GthViewer      *viewer)
{
	GthViewerPrivateData  *priv = viewer->priv;
	GthZoomQuality         quality;

	quality = gtk_radio_action_get_current_value (current);

	gtk_radio_action_get_current_value (current);
	image_viewer_set_zoom_quality (IMAGE_VIEWER (priv->viewer), quality);
	image_viewer_update_view (IMAGE_VIEWER (priv->viewer));
	pref_set_zoom_quality (quality);
}


static gboolean
progress_cancel_cb (GtkButton *button,
		    GthViewer *viewer)
{
	if (viewer->priv->pixop != NULL)
		gth_pixbuf_op_stop (viewer->priv->pixop);
	return TRUE;
}


static gboolean
progress_delete_cb (GtkWidget    *caller, 
		    GdkEvent     *event,
		    GthViewer    *viewer)
{
	if (viewer->priv->pixop != NULL)
		gth_pixbuf_op_stop (viewer->priv->pixop);
	return TRUE;
}


static void
gth_viewer_init (GthViewer *viewer)
{
	GthViewerPrivateData *priv;

	priv = viewer->priv = g_new0 (GthViewerPrivateData, 1);
	priv->first_time_show = TRUE;
	priv->image_path = NULL;
	priv->image_error = FALSE;

	g_object_set (viewer, 
		      "close_on_save", TRUE,
		      NULL);

	if (SingleViewer == NULL) 
		SingleViewer = viewer;
}


static void
monitor_update_metadata_cb (GthMonitor *monitor,
			    const char *filename,
			    GthViewer  *viewer)
{
	g_return_if_fail (viewer != NULL);
	viewer_update_image_info (viewer);
}


static void
monitor_update_files_cb (GthMonitor      *monitor,
			 GthMonitorEvent  event,
			 GList           *list,
			 GthViewer       *viewer)
{
	g_return_if_fail (viewer != NULL);

	if (viewer->priv->image_path == NULL)
		return;

	if (g_list_find_custom (list, 
				viewer->priv->image_path, 
				(GCompareFunc) strcmp) == NULL)
		return;

	switch (event) {
	case GTH_MONITOR_EVENT_CREATED:
	case GTH_MONITOR_EVENT_CHANGED:
		gth_window_reload_current_image (GTH_WINDOW (viewer));
		break;

	case GTH_MONITOR_EVENT_DELETED:
		if (! viewer->priv->image_modified)
			gth_window_close (GTH_WINDOW (viewer));
		break;

	default:
		break;
	}
}


static void
monitor_file_renamed_cb (GthMonitor *monitor,
			 const char *old_name,
			 const char *new_name,
			 GthViewer  *viewer)
{
	if (viewer->priv->image_path == NULL)
		return;

	if (strcmp (old_name, viewer->priv->image_path) != 0)
		return;

	g_free (viewer->priv->image_path);
	viewer->priv->image_path = g_strdup (new_name);

	gth_window_reload_current_image (GTH_WINDOW (viewer));
}


static void
sync_menu_with_preferences (GthViewer *viewer)
{
	char *prop;

	set_action_active (viewer, "View_PlayAnimation", TRUE);

	switch (pref_get_zoom_quality ()) {
	case GTH_ZOOM_QUALITY_HIGH: 
		prop = "View_ZoomQualityHigh"; 
		break;
	case GTH_ZOOM_QUALITY_LOW:  
	default:
		prop = "View_ZoomQualityLow"; 
		break;
	}
	set_action_active (viewer, prop, TRUE);

	set_action_active (viewer, "View_ShowInfo", eel_gconf_get_boolean (PREF_SHOW_IMAGE_DATA, FALSE));
	set_action_active (viewer, "View_SingleWindow", eel_gconf_get_boolean (PREF_SINGLE_WINDOW, FALSE));
}


static void
add_rotate_toolbar_item (GthViewer *viewer)
{
	GthViewerPrivateData *priv = viewer->priv;	
	GtkToolItem *sep;

	gtk_ui_manager_ensure_update (priv->ui);

	sep = gtk_separator_tool_item_new ();
	gtk_widget_show (GTK_WIDGET (sep));
	gtk_toolbar_insert (GTK_TOOLBAR (priv->toolbar), 
			    sep,
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
gth_viewer_construct (GthViewer   *viewer,
		      const gchar *filename)
{
	GthViewerPrivateData *priv = viewer->priv;
	GtkWidget            *menubar, *toolbar;
	GtkWidget            *image_vbox;
	GtkWidget            *info_frame;
	GtkWidget            *info_hbox;
	GtkWidget            *button;
	GtkWidget            *image;
	GtkWidget            *image_vpaned;
	GtkWidget            *table;
	GtkWidget            *hbox;
	GtkWidget            *scrolled_window;
	GtkActionGroup       *actions;
	GtkUIManager         *ui;
	GError               *error = NULL;		
	int                   i;

	/* Create the widgets. */

	priv->tooltips = gtk_tooltips_new ();

	/* Build the menu and the toolbar. */

	priv->actions = actions = gtk_action_group_new ("Actions");
	gtk_action_group_set_translation_domain (actions, NULL);

	gtk_action_group_add_actions (actions, 
				      gth_window_action_entries, 
				      gth_window_action_entries_size, 
				      viewer);
	gtk_action_group_add_toggle_actions (actions, 
					     gth_window_action_toggle_entries, 
					     gth_window_action_toggle_entries_size, 
					     viewer);
	gtk_action_group_add_radio_actions (actions, 
					    gth_window_zoom_quality_entries, 
					    gth_window_zoom_quality_entries_size,
					    GTH_ZOOM_QUALITY_HIGH,
					    G_CALLBACK (zoom_quality_radio_action), 
					    viewer);

	gtk_action_group_add_actions (actions, 
				      gth_viewer_action_entries, 
				      gth_viewer_action_entries_size, 
				      viewer);
	gtk_action_group_add_toggle_actions (actions, 
					     gth_viewer_action_toggle_entries, 
					     gth_viewer_action_toggle_entries_size, 
					     viewer);

	priv->ui = ui = gtk_ui_manager_new ();
	
	g_signal_connect (ui, 
			  "connect_proxy",
			  G_CALLBACK (connect_proxy_cb), 
			  viewer);
	g_signal_connect (ui, 
			  "disconnect_proxy",
			  G_CALLBACK (disconnect_proxy_cb), 
			  viewer);
	
	gtk_ui_manager_insert_action_group (ui, actions, 0);
	gtk_window_add_accel_group (GTK_WINDOW (viewer), 
				    gtk_ui_manager_get_accel_group (ui));
	
	if (!gtk_ui_manager_add_ui_from_string (ui, viewer_window_ui_info, -1, &error)) {
		g_message ("building menus failed: %s", error->message);
		g_error_free (error);
	}
	menubar = gtk_ui_manager_get_widget (ui, "/MenuBar");
	gtk_widget_show (menubar);

	gnome_app_add_docked (GNOME_APP (viewer),
			      menubar,
			      "MenuBar",
			      (BONOBO_DOCK_ITEM_BEH_NEVER_VERTICAL 
			       | BONOBO_DOCK_ITEM_BEH_EXCLUSIVE 
			       | (eel_gconf_get_boolean (PREF_DESKTOP_MENUBAR_DETACHABLE, TRUE) ? BONOBO_DOCK_ITEM_BEH_NORMAL : BONOBO_DOCK_ITEM_BEH_LOCKED)),
			      BONOBO_DOCK_TOP,
			      1, 1, 0);

	priv->toolbar = toolbar = gtk_ui_manager_get_widget (ui, "/ToolBar");
	gtk_toolbar_set_show_arrow (GTK_TOOLBAR (toolbar), TRUE);

	gnome_app_add_docked (GNOME_APP (viewer),
			      toolbar,
			      "ToolBar",
			      (BONOBO_DOCK_ITEM_BEH_NEVER_VERTICAL 
			       | BONOBO_DOCK_ITEM_BEH_EXCLUSIVE 
			       | (eel_gconf_get_boolean (PREF_DESKTOP_TOOLBAR_DETACHABLE, TRUE) ? BONOBO_DOCK_ITEM_BEH_NORMAL : BONOBO_DOCK_ITEM_BEH_LOCKED)),
			      BONOBO_DOCK_TOP,
			      2, 1, 0);

	priv->image_popup_menu = gtk_ui_manager_get_widget (ui, "/ImagePopupMenu");

	add_rotate_toolbar_item (viewer);

	/* Create the statusbar. */

	priv->statusbar = gtk_statusbar_new ();
	priv->help_message_cid = gtk_statusbar_get_context_id (GTK_STATUSBAR (priv->statusbar), "help_message");
	priv->image_info_cid = gtk_statusbar_get_context_id (GTK_STATUSBAR (priv->statusbar), "image_info");
	priv->progress_cid = gtk_statusbar_get_context_id (GTK_STATUSBAR (priv->statusbar), "progress");
	gnome_app_set_statusbar (GNOME_APP (viewer), priv->statusbar);

	gtk_statusbar_set_has_resize_grip (GTK_STATUSBAR (priv->statusbar), TRUE);

	/* Statusbar: zoom info */

	priv->zoom_info = gtk_label_new (NULL);
	gtk_widget_show (priv->zoom_info);

	priv->zoom_info_frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (priv->zoom_info_frame), GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (priv->zoom_info_frame), priv->zoom_info);
	gtk_box_pack_start (GTK_BOX (priv->statusbar), priv->zoom_info_frame, FALSE, FALSE, 0);
	
	/* Statusbar: image info */

	priv->image_info = gtk_label_new (NULL);
	gtk_widget_show (priv->image_info);

	priv->image_info_frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (priv->image_info_frame), GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (priv->image_info_frame), priv->image_info);
	gtk_box_pack_start (GTK_BOX (priv->statusbar), priv->image_info_frame, FALSE, FALSE, 0);

	/* Info bar. */

	priv->info_bar = gthumb_info_bar_new ();
	gthumb_info_bar_set_focused (GTHUMB_INFO_BAR (priv->info_bar), FALSE);
	g_signal_connect (G_OBJECT (priv->info_bar), 
			  "button_press_event",
			  G_CALLBACK (info_bar_clicked_cb), 
			  viewer);

	/* Image viewer. */
	
	priv->viewer = image_viewer_new ();
	gtk_widget_set_size_request (priv->viewer, PANE_MIN_SIZE, PANE_MIN_SIZE);

	gtk_drag_source_set (priv->viewer,
			     GDK_BUTTON2_MASK,
			     target_table, G_N_ELEMENTS (target_table), 
			     GDK_ACTION_MOVE | GDK_ACTION_COPY);

	gtk_drag_dest_set (priv->viewer,
			   GTK_DEST_DEFAULT_ALL,
			   target_table, G_N_ELEMENTS (target_table),
			   GDK_ACTION_MOVE | GDK_ACTION_COPY);
	
	g_signal_connect (G_OBJECT (priv->viewer), 
			  "image_loaded",
			  G_CALLBACK (image_loaded_cb), 
			  viewer);
	g_signal_connect (G_OBJECT (priv->viewer), 
			  "zoom_changed",
			  G_CALLBACK (zoom_changed_cb), 
			  viewer);
	g_signal_connect (G_OBJECT (priv->viewer), 
			  "size_changed",
			  G_CALLBACK (size_changed_cb), 
			  viewer);
	g_signal_connect (G_OBJECT (priv->viewer), 
			  "focus_in_event",
			  G_CALLBACK (image_focus_changed_cb), 
			  viewer);
	g_signal_connect (G_OBJECT (priv->viewer), 
			  "focus_out_event",
			  G_CALLBACK (image_focus_changed_cb), 
			  viewer);

	g_signal_connect (G_OBJECT (priv->viewer),
			  "drag_data_get",
			  G_CALLBACK (viewer_drag_data_get), 
			  viewer);

	g_signal_connect (G_OBJECT (priv->viewer), 
			  "drag_data_received",
			  G_CALLBACK (viewer_drag_data_received), 
			  viewer);

	g_signal_connect (G_OBJECT (priv->viewer),
			  "key_press_event",
			  G_CALLBACK (viewer_key_press_cb),
			  viewer);

	/*
	g_signal_connect (G_OBJECT (IMAGE_VIEWER (window->viewer)->loader), 
			  "image_progress",
			  G_CALLBACK (image_loader_progress_cb), 
			  window);
	g_signal_connect (G_OBJECT (IMAGE_VIEWER (window->viewer)->loader), 
			  "image_done",
			  G_CALLBACK (image_loader_done_cb), 
			  window);
	g_signal_connect (G_OBJECT (IMAGE_VIEWER (window->viewer)->loader), 
			  "image_error",
			  G_CALLBACK (image_loader_done_cb), 
			  window);
	*/

	priv->viewer_vscr = gtk_vscrollbar_new (IMAGE_VIEWER (priv->viewer)->vadj);
	priv->viewer_hscr = gtk_hscrollbar_new (IMAGE_VIEWER (priv->viewer)->hadj);
	priv->viewer_nav_event_box = gtk_event_box_new ();
	gtk_container_add (GTK_CONTAINER (priv->viewer_nav_event_box), _gtk_image_new_from_xpm_data (nav_button_xpm));

	g_signal_connect (G_OBJECT (priv->viewer_nav_event_box), 
			  "button_press_event",
			  G_CALLBACK (nav_button_clicked_cb), 
			  priv->viewer);

	/* Image comment */

	priv->image_comment = gtk_text_view_new ();
	gtk_text_view_set_editable (GTK_TEXT_VIEW (priv->image_comment), FALSE);
	gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (priv->image_comment), GTK_WRAP_WORD);
	gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (priv->image_comment), TRUE);
	g_signal_connect (G_OBJECT (priv->image_comment), 
			  "focus_in_event",
			  G_CALLBACK (image_focus_changed_cb), 
			  viewer);
	g_signal_connect (G_OBJECT (priv->image_comment), 
			  "focus_out_event",
			  G_CALLBACK (image_focus_changed_cb), 
			  viewer);
	g_signal_connect (G_OBJECT (priv->image_comment), 
			  "button_press_event",
			  G_CALLBACK (image_comment_button_press_cb), 
			  viewer);

	/* Exif data viewer */

	priv->exif_data_viewer = gth_exif_data_viewer_new (TRUE);
	g_signal_connect (G_OBJECT (gth_exif_data_viewer_get_view (GTH_EXIF_DATA_VIEWER (priv->exif_data_viewer))), 
			  "focus_in_event",
			  G_CALLBACK (image_focus_changed_cb), 
			  viewer);
	g_signal_connect (G_OBJECT (gth_exif_data_viewer_get_view (GTH_EXIF_DATA_VIEWER (priv->exif_data_viewer))), 
			  "focus_out_event",
			  G_CALLBACK (image_focus_changed_cb), 
			  viewer);

	/* Comment button */

	image = _gtk_image_new_from_inline (preview_data_16_rgba);
	priv->comment_button = button = gtk_toggle_button_new ();
	gtk_container_add (GTK_CONTAINER (button), image);

	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
	gtk_tooltips_set_tip (priv->tooltips,
			      button,
			      _("Image comment"),
			      NULL);
	g_signal_connect (G_OBJECT (button), 
			  "toggled",
			  G_CALLBACK (comment_button_toggled_cb), 
			  viewer);

	/* Pack the widgets:

	   image_vbox
             |
             +- info_frame
             |   |
	     |	 +- info_hbox
             |       |
             |       +- priv->info_bar
	     |       |
             |       +- button
             |
             +- image_vpaned
                  |
                  +- table
                  |    |
                  |    +- hbox
                  |    |    |
                  |    |    +- priv->viewer
                  |    |
                  |    +- priv->viewer_vscr
                  |    |
                  |    +- priv->viewer_hscr
                  |    |
                  |    +- priv->image_nav_button_box
                  |
                  +- priv->image_data_hpaned
                       |
                       +- scrolled_window
                       |    |
                       |    +- priv->image_comment
                       |
                       +- priv->exif_data_viewer

	*/

	image_vbox = gtk_vbox_new (FALSE, 0);

	/* * info_frame */

	info_frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (info_frame), GTK_SHADOW_NONE);

	info_hbox = gtk_hbox_new (FALSE, 0);

	gtk_box_pack_end (GTK_BOX (info_hbox), button, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (info_hbox), priv->info_bar, TRUE, TRUE, 0);
	gtk_container_add (GTK_CONTAINER (info_frame), info_hbox);

	/* * image_vpaned */

	priv->image_main_pane = image_vpaned = gtk_vpaned_new ();
	gtk_paned_set_position (GTK_PANED (image_vpaned), eel_gconf_get_integer (PREF_UI_COMMENT_PANE_SIZE, DEFAULT_WIN_WIDTH / 3 * 2));

	/* ** table */

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (hbox), priv->viewer);

	table = gtk_table_new (2, 2, FALSE);
	gtk_table_attach (GTK_TABLE (table), hbox, 0, 1, 0, 1,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
	gtk_table_attach (GTK_TABLE (table), priv->viewer_vscr, 1, 2, 0, 1,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
	gtk_table_attach (GTK_TABLE (table), priv->viewer_hscr, 0, 1, 1, 2,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL), 0, 0);
	gtk_table_attach (GTK_TABLE (table), priv->viewer_nav_event_box, 1, 2, 1, 2,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL), 0, 0);

	/* ** priv->image_data_hpaned */

	priv->image_data_hpaned = gtk_hpaned_new ();
	gtk_paned_set_position (GTK_PANED (priv->image_data_hpaned), eel_gconf_get_integer (PREF_UI_VIEWER_WIDTH, DEFAULT_WIN_WIDTH) / 2);

	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window), GTK_SHADOW_ETCHED_IN);
	gtk_container_add (GTK_CONTAINER (scrolled_window), priv->image_comment);

	gtk_paned_pack2 (GTK_PANED (priv->image_data_hpaned), scrolled_window, TRUE, FALSE);
	gtk_paned_pack1 (GTK_PANED (priv->image_data_hpaned), priv->exif_data_viewer, FALSE, FALSE);

	/**/

	gtk_paned_pack1 (GTK_PANED (image_vpaned), table, FALSE, FALSE);
	gtk_paned_pack2 (GTK_PANED (image_vpaned), priv->image_data_hpaned, TRUE, FALSE);

	gtk_box_pack_start (GTK_BOX (image_vbox), info_frame, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (image_vbox), image_vpaned, TRUE, TRUE, 0);
        gtk_widget_show_all (image_vbox);
	gtk_widget_hide (priv->image_data_hpaned); /* FIXME */

	gnome_app_set_contents (GNOME_APP (viewer), image_vbox);

	/**/

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

	gtk_window_set_default_size (GTK_WINDOW (viewer), 
				     eel_gconf_get_integer (PREF_UI_VIEWER_WIDTH, DEFAULT_WIN_WIDTH),
				     eel_gconf_get_integer (PREF_UI_VIEWER_HEIGHT, DEFAULT_WIN_HEIGHT));

	/**/

	/* Add notification callbacks. */

	i = 0;

	priv->cnxn_id[i++] = eel_gconf_notification_add (
					   PREF_UI_TOOLBAR_VISIBLE,
					   pref_ui_toolbar_visible_changed,
					   viewer);

	priv->cnxn_id[i++] = eel_gconf_notification_add (
					   PREF_UI_STATUSBAR_VISIBLE,
					   pref_ui_statusbar_visible_changed,
					   viewer);

	priv->cnxn_id[i++] = eel_gconf_notification_add (
					   PREF_SINGLE_WINDOW,
					   pref_ui_single_window_changed,
					   viewer);

	/* Progress dialog */

	priv->progress_gui = glade_xml_new (GTHUMB_GLADEDIR "/" GLADE_EXPORTER_FILE, NULL, NULL);
	if (! priv->progress_gui) {
		priv->progress_dialog = NULL;
		priv->progress_progressbar = NULL;
		priv->progress_info = NULL;
	} else {
		GtkWidget *cancel_button;

		priv->progress_dialog = glade_xml_get_widget (priv->progress_gui, "progress_dialog");
		priv->progress_progressbar = glade_xml_get_widget (priv->progress_gui, "progress_progressbar");
		priv->progress_info = glade_xml_get_widget (priv->progress_gui, "progress_info");
		cancel_button = glade_xml_get_widget (priv->progress_gui, "progress_cancel");

		gtk_window_set_transient_for (GTK_WINDOW (priv->progress_dialog), GTK_WINDOW (viewer));
		gtk_window_set_modal (GTK_WINDOW (priv->progress_dialog), FALSE);

		g_signal_connect (G_OBJECT (cancel_button), 
				  "clicked",
				  G_CALLBACK (progress_cancel_cb), 
				  viewer);
		g_signal_connect (G_OBJECT (priv->progress_dialog), 
				  "delete_event",
				  G_CALLBACK (progress_delete_cb),
				  viewer);
	}

	sync_menu_with_preferences (viewer);

	/**/

	g_signal_connect (G_OBJECT (monitor), 
			  "update_files",
			  G_CALLBACK (monitor_update_files_cb),
			  viewer);
	g_signal_connect (G_OBJECT (monitor), 
			  "update_metadata",
			  G_CALLBACK (monitor_update_metadata_cb),
			  viewer);
	g_signal_connect (G_OBJECT (monitor), 
			  "file_renamed",
			  G_CALLBACK (monitor_file_renamed_cb),
			  viewer);
	
	/**/

	if (filename != NULL)
		priv->image_path = g_strdup (filename);
}


GtkWidget * 
gth_viewer_new (const gchar *filename)
{
	GthViewer *viewer;

	viewer = (GthViewer*) g_object_new (GTH_TYPE_VIEWER, NULL);
	gth_viewer_construct (viewer, filename);

	return (GtkWidget*) viewer;
}


static void
load_image__image_saved_cb (const char *filename,
			    gpointer    data)
{
	GthViewer             *viewer = data;
	GthViewerPrivateData  *priv = viewer->priv;

	if (priv->new_image_path == NULL) {
		g_free (priv->image_path);
		priv->image_path = NULL;
		viewer_set_void (viewer, FALSE);
		return;
	}

	if (priv->new_image_path != priv->image_path) {
		g_free (priv->image_path);
		priv->image_path = g_strdup (priv->new_image_path);
	}

	image_viewer_load_image (IMAGE_VIEWER (priv->viewer), priv->image_path);
}


void
gth_viewer_load (GthViewer   *viewer,
		 const gchar *filename)

{
	GthViewerPrivateData *priv = viewer->priv;

	g_free (priv->new_image_path);
	priv->new_image_path = NULL;
	if (filename != NULL)
		priv->new_image_path = g_strdup (filename);

	if (priv->image_modified) {
		if (priv->saving_modified_image)
			return;
		if (ask_whether_to_save (viewer, load_image__image_saved_cb))
			return;
	}

	load_image__image_saved_cb (NULL, viewer);
}


/* gth_viewer_close */


static void
close__step2 (const char *filename,
	      gpointer    data)
{
	GthViewer             *viewer = data;
	GthViewerPrivateData  *priv = viewer->priv;
	
	if (priv->pixop != NULL) 
		g_object_unref (priv->pixop);

	if (priv->progress_gui != NULL)
		g_object_unref (priv->progress_gui);

	gtk_object_destroy (GTK_OBJECT (priv->tooltips));
	if (priv->image_popup_menu != NULL) {
		gtk_widget_destroy (priv->image_popup_menu);
		priv->image_popup_menu = NULL;
	}

	if (SingleViewer == viewer) 
		SingleViewer = NULL;
	if (LastFocusedViewer == viewer) 
		LastFocusedViewer = NULL;

	gtk_widget_destroy (GTK_WIDGET (viewer));
}


static void
gth_viewer_close (GthWindow *window)
{
	GthViewer            *viewer = (GthViewer*) window;
	GthViewerPrivateData *priv = viewer->priv;

	debug(DEBUG_INFO, "Gth::Viewer::Close");

	priv->closing = TRUE;

	g_signal_handlers_disconnect_by_data (G_OBJECT (monitor), viewer);

	if (priv->fullscreen != NULL)
		g_signal_handlers_disconnect_by_data (G_OBJECT (priv->fullscreen),
						      viewer);

	if (priv->image_modified) 
		if (ask_whether_to_save (viewer, close__step2))
			return;
	close__step2 (NULL, viewer);
}


static ImageViewer *
gth_viewer_get_image_viewer (GthWindow *window)
{
	GthViewer *viewer = (GthViewer*) window;
	return (ImageViewer*) viewer->priv->viewer;
}


static const char *
gth_viewer_get_image_filename (GthWindow *window)
{
	GthViewer *viewer = (GthViewer*) window;
	return viewer->priv->image_path;
}


static gboolean
gth_viewer_get_image_modified (GthWindow *window)
{
	GthViewer *viewer = (GthViewer*) window;
	return viewer->priv->image_modified;
}


static void
gth_viewer_set_image_modified (GthWindow *window,
			       gboolean   value)
{
	GthViewer            *viewer = (GthViewer*) window;
	GthViewerPrivateData *priv = viewer->priv;

	priv->image_modified = value;
	viewer_update_infobar (viewer);
	viewer_update_statusbar_image_info (viewer);
	viewer_update_title (viewer);

	set_action_sensitive (viewer, "File_Save", ! image_viewer_is_void (IMAGE_VIEWER (priv->viewer)) && priv->image_modified);
	set_action_sensitive (viewer, "File_Revert", ! image_viewer_is_void (IMAGE_VIEWER (priv->viewer)) && priv->image_modified);
	set_action_sensitive (viewer, "Edit_Undo", gth_window_get_can_undo (window));
	set_action_sensitive (viewer, "Edit_Redo", gth_window_get_can_redo (window));
}


static void
gth_viewer_save_pixbuf (GthWindow  *window,
			GdkPixbuf  *pixbuf,
			const char *filename)
{
	GthViewer            *viewer = (GthViewer*) window;
	GthViewerPrivateData *priv = viewer->priv;
	char                 *current_folder = NULL;

	if (priv->image_path != NULL)
		current_folder = g_strdup (priv->image_path);

	if (filename == NULL)
		dlg_save_image_as (GTK_WINDOW (viewer), 
				   current_folder,
				   pixbuf,
				   save_pixbuf__image_saved_cb,
				   viewer);
	else
		dlg_save_image (GTK_WINDOW (viewer), 
				filename,
				pixbuf,
				save_pixbuf__image_saved_cb,
				viewer);

	g_free (current_folder);
}


/* -- image operations -- */


static void
pixbuf_op_done_cb (GthPixbufOp   *pixop,
		   gboolean       completed,
		   GthViewer     *viewer)
{
	GthViewerPrivateData *priv = viewer->priv;
	ImageViewer          *image_viewer = IMAGE_VIEWER (priv->viewer);

	if (completed) {
		if (priv->pixop_preview)
			image_viewer_set_pixbuf (image_viewer, priv->pixop->dest);
		else {
			gth_window_set_image_pixbuf (GTH_WINDOW (viewer), priv->pixop->dest);
			gth_window_set_image_modified (GTH_WINDOW (viewer), TRUE);
		}
	}

	g_object_unref (priv->pixop);
	priv->pixop = NULL;

	if (priv->progress_dialog != NULL) 
		gtk_widget_hide (priv->progress_dialog);
}


static void
pixbuf_op_progress_cb (GthPixbufOp  *pixop,
		       float         p, 
		       GthViewer    *viewer)
{
	if (viewer->priv->progress_dialog != NULL) 
		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (viewer->priv->progress_progressbar), p);
}


static int
viewer__display_progress_dialog (gpointer data)
{
	GthViewer            *viewer = data;
	GthViewerPrivateData *priv = viewer->priv;

	if (priv->progress_timeout != 0) {
		g_source_remove (priv->progress_timeout);
		priv->progress_timeout = 0;
	}

	if (priv->pixop != NULL)
		gtk_widget_show (priv->progress_dialog);

	return FALSE;
}


static void
gth_viewer_exec_pixbuf_op (GthWindow   *window,
			   GthPixbufOp *pixop,
			   gboolean     preview)
{
	GthViewer            *viewer = (GthViewer*) window;
	GthViewerPrivateData *priv = viewer->priv;

	priv->pixop = pixop;
	g_object_ref (priv->pixop);
	priv->pixop_preview = preview;

	gtk_label_set_text (GTK_LABEL (priv->progress_info),
			    _("Wait please..."));

	g_signal_connect (G_OBJECT (pixop),
			  "pixbuf_op_done",
			  G_CALLBACK (pixbuf_op_done_cb),
			  viewer);
	g_signal_connect (G_OBJECT (pixop),
			  "pixbuf_op_progress",
			  G_CALLBACK (pixbuf_op_progress_cb),
			  viewer);
	
	if (priv->progress_dialog != NULL)
		priv->progress_timeout = g_timeout_add (DISPLAY_PROGRESS_DELAY, viewer__display_progress_dialog, viewer);

	gth_pixbuf_op_start (priv->pixop);
}


static void
reload_current_image__step2 (const char *filename,
			     gpointer    data)
{
	GthViewer *viewer = data;
	gth_viewer_load (viewer, viewer->priv->image_path);
}


static void
gth_viewer_reload_current_image (GthWindow *window)
{
	GthViewer *viewer = GTH_VIEWER (window);

	if (viewer->priv->image_path == NULL)
		return;

	if (viewer->priv->image_modified) 
		if (ask_whether_to_save (viewer, reload_current_image__step2))
			return;

	reload_current_image__step2 (NULL, viewer);
}


static void
gth_viewer_update_current_image_metadata (GthWindow *window)
{
	GthViewer *viewer = GTH_VIEWER (window);

	if (viewer->priv->image_path == NULL)
		return;
	update_image_comment (viewer);
}


static GList *
gth_viewer_get_file_list_selection (GthWindow *window)
{
	GthViewer            *viewer = GTH_VIEWER (window);
	GthViewerPrivateData *priv = viewer->priv;

	if (priv->image_path == NULL)
		return NULL;
	return g_list_prepend (NULL, g_strdup (viewer->priv->image_path));
}


static GList *
gth_viewer_get_file_list_selection_as_fd (GthWindow *window)
{
	GthViewer            *viewer = GTH_VIEWER (window);
	GthViewerPrivateData *priv = viewer->priv;
	FileData             *fd;

	if (priv->image_path == NULL)
		return NULL;

	fd = file_data_new (priv->image_path, NULL);
	file_data_update (fd);

	return g_list_prepend (NULL, fd);
}


static void
gth_viewer_set_animation (GthWindow *window,
			  gboolean   play)
{
	GthViewer   *viewer = GTH_VIEWER (window);
	ImageViewer *image_viewer = IMAGE_VIEWER (viewer->priv->viewer);

	set_action_active (viewer, "View_PlayAnimation", play);
	set_action_sensitive (viewer, "View_StepAnimation", ! play);
	if (play)
		image_viewer_start_animation (image_viewer);
	else
		image_viewer_stop_animation (image_viewer);
}


static gboolean       
gth_viewer_get_animation (GthWindow *window)
{
	GthViewer *viewer = GTH_VIEWER (window);
	return IMAGE_VIEWER (viewer->priv->viewer)->play_animation;
}


static void           
gth_viewer_step_animation (GthWindow *window)
{
	GthViewer *viewer = GTH_VIEWER (window);
	image_viewer_step_animation (IMAGE_VIEWER (viewer->priv->viewer));
}


static gboolean
fullscreen_destroy_cb (GtkWidget *widget,
		       GthViewer *viewer)
{
	viewer->priv->fullscreen = NULL;
	gth_window_set_fullscreen (GTH_WINDOW (viewer), FALSE);
	return FALSE;
}


static void           
gth_viewer_set_fullscreen (GthWindow *window,
			   gboolean   fullscreen)
{
	GthViewer *viewer = GTH_VIEWER (window);
	GthViewerPrivateData *priv = viewer->priv;
	
	if (fullscreen && (priv->fullscreen == NULL)) {
		GdkPixbuf *image = NULL;

		if (!image_viewer_is_animation (IMAGE_VIEWER (priv->viewer)))
			image = image_viewer_get_current_pixbuf (IMAGE_VIEWER (priv->viewer));
		priv->fullscreen = gth_fullscreen_new (image, priv->image_path, gth_viewer_get_file_list_selection (window));
		g_signal_connect (priv->fullscreen, 
				  "destroy",
				  G_CALLBACK (fullscreen_destroy_cb), 
				  viewer);
		gtk_widget_show (priv->fullscreen);

	} else if (!fullscreen && (priv->fullscreen != NULL)) 
		gtk_widget_destroy (priv->fullscreen);
}


static void           
gth_viewer_set_slideshow (GthWindow *window,
			  gboolean   value)
{
}


static gboolean
gth_viewer_focus_in_event (GtkWidget     *widget,
			   GdkEventFocus *event)
{
	LastFocusedViewer = (GthViewer*) widget;
	SingleViewer = LastFocusedViewer;
	return GTK_WIDGET_CLASS (parent_class)->focus_in_event (widget, event);
}


static void
gth_viewer_class_init (GthViewerClass *class)
{
	GObjectClass   *gobject_class;
	GtkWidgetClass *widget_class;
	GthWindowClass *window_class;

	parent_class = g_type_class_peek_parent (class);
	gobject_class = (GObjectClass*) class;
	widget_class = (GtkWidgetClass*) class;
	window_class = (GthWindowClass*) class;

	gobject_class->finalize = gth_viewer_finalize;

	widget_class->realize = gth_viewer_realize;
	widget_class->unrealize = gth_viewer_unrealize;
	widget_class->show = gth_viewer_show;
	widget_class->focus_in_event = gth_viewer_focus_in_event;

	window_class->close = gth_viewer_close;
	window_class->get_image_viewer = gth_viewer_get_image_viewer;
	window_class->get_image_filename = gth_viewer_get_image_filename;
	window_class->get_image_modified = gth_viewer_get_image_modified;
	window_class->set_image_modified = gth_viewer_set_image_modified;
	window_class->save_pixbuf = gth_viewer_save_pixbuf;
	window_class->exec_pixbuf_op = gth_viewer_exec_pixbuf_op;

	window_class->reload_current_image = gth_viewer_reload_current_image;
	window_class->update_current_image_metadata = gth_viewer_update_current_image_metadata;
	window_class->get_file_list_selection = gth_viewer_get_file_list_selection;
	window_class->get_file_list_selection_as_fd = gth_viewer_get_file_list_selection_as_fd;

	window_class->set_animation = gth_viewer_set_animation;
	window_class->get_animation = gth_viewer_get_animation;
	window_class->step_animation = gth_viewer_step_animation;
	window_class->set_fullscreen = gth_viewer_set_fullscreen;
	window_class->set_slideshow = gth_viewer_set_slideshow;
}


GType
gth_viewer_get_type ()
{
        static GType type = 0;

        if (! type) {
                GTypeInfo type_info = {
			sizeof (GthViewerClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_viewer_class_init,
			NULL,
			NULL,
			sizeof (GthViewer),
			0,
			(GInstanceInitFunc) gth_viewer_init
		};

		type = g_type_register_static (GTH_TYPE_WINDOW,
					       "GthViewer",
					       &type_info,
					       0);
	}

        return type;
}


void
compute_single_viewer_window (void)
{
	GList *windows = gth_window_get_window_list ();
	GList *scan;
		
	if (SingleViewer != NULL) 
		return;

	for (scan = windows; scan; scan = scan->next) {
		GthWindow *window = scan->data;
		if (GTH_IS_VIEWER (window)) {
			SingleViewer = (GthViewer*) window;
			break;
		}
	}
}
