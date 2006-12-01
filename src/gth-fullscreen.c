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

#include <stdlib.h>
#include <string.h>

#include <glib/gi18n.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <glade/glade.h>
#include <libgnomevfs/gnome-vfs-utils.h>

#include "comments.h"
#include "dlg-file-utils.h"
#include "file-utils.h"
#include "gconf-utils.h"
#include "glib-utils.h"
#include "gtk-utils.h"
#include "gth-exif-utils.h"
#include "gth-fullscreen.h"
#include "gth-fullscreen-ui.h"
#include "gth-window.h"
#include "gth-window-utils.h"
#include "gthumb-preloader.h"
#include "gthumb-stock.h"
#include "image-viewer.h"
#include "main.h"
#include "preferences.h"
#include "totem-scrsaver.h"
#include "gs-fade.h"

#include "icons/pixbufs.h"

#define HIDE_DELAY 1000
#define X_PADDING  12
#define Y_PADDING  6
#define MOTION_THRESHOLD 3
#define DEF_SLIDESHOW_DELAY 4
#define SECONDS 1000


struct _GthFullscreenPrivateData {
	GtkUIManager    *ui;
	GtkActionGroup  *actions;
	GdkPixbuf       *image;
	char            *image_path;
	GList           *file_list;
	int              files;
	int              viewed;
	char            *catalog_path;
	GList           *current;
	gboolean         slideshow;
	GthDirectionType slideshow_direction;
	int              slideshow_delay;
	gboolean         slideshow_wrap_around;
	guint            slideshow_timeout;
	guint            slideshow_paused;

	gboolean         first_time_show;
	guint            mouse_hide_id;

	GThumbPreloader *preloader;
	char            *requested_path;

	GtkWidget       *viewer;
	GtkWidget       *toolbar_window;

	TotemScrsaver   *screensaver;
	GSFade          *fade;
	gboolean         use_fade;

	/* comment */

	gboolean        comment_visible;
	gboolean        image_data_visible;
	GdkPixmap      *buffer;
	GdkPixmap      *original_buffer;
	PangoRectangle  bounds;
};

static GthWindowClass *parent_class = NULL;


static void
gth_fullscreen_finalize (GObject *object)
{
	GthFullscreen *fullscreen = GTH_FULLSCREEN (object);

	debug (DEBUG_INFO, "Gth::Fullscreen::Finalize");

	if (fullscreen->priv != NULL) {
		GthFullscreenPrivateData *priv = fullscreen->priv;

		g_signal_handlers_disconnect_by_data (G_OBJECT (monitor), fullscreen);

		if (priv->slideshow_timeout != 0) {
			g_source_remove (priv->slideshow_timeout);
			priv->slideshow_timeout = 0;
		}

		if (priv->toolbar_window != NULL) {
			gtk_widget_destroy (priv->toolbar_window);
			priv->toolbar_window = NULL;
		}

		if (priv->mouse_hide_id != 0)
			g_source_remove (priv->mouse_hide_id);

		if (priv->image != NULL) {
			g_object_unref (priv->image);
			priv->image = NULL;
		}

		g_object_unref (priv->screensaver);

		gs_fade_reset (priv->fade);
		g_object_unref (priv->fade);

		g_free (priv->image_path);
		g_free (priv->requested_path);
		path_list_free (priv->file_list);
		g_free (priv->catalog_path);

		g_object_unref (priv->preloader);

		if (priv->buffer != NULL) {
			g_object_unref (priv->buffer);
			g_object_unref (priv->original_buffer);
			priv->buffer = NULL;
			priv->original_buffer = NULL;
		}

		/**/

		g_free (fullscreen->priv);
		fullscreen->priv = NULL;
	}

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void load_next_image (GthFullscreen *fullscreen);


static gboolean
slideshow_timeout_cb (gpointer data)
{
	GthFullscreen            *fullscreen = data;
	GthFullscreenPrivateData *priv = fullscreen->priv;

	if (priv->slideshow_timeout != 0) {
		g_source_remove (priv->slideshow_timeout);
		priv->slideshow_timeout = 0;
	}

	if (!priv->slideshow_paused) {
		priv->use_fade = TRUE;
		load_next_image (fullscreen);
	}

	return FALSE;
}


static void
continue_slideshow (GthFullscreen *fullscreen)
{
	GthFullscreenPrivateData *priv = fullscreen->priv;

	if (priv->slideshow_timeout != 0) {
		g_source_remove (priv->slideshow_timeout);
		priv->slideshow_timeout = 0;
	}

	priv->slideshow_timeout = g_timeout_add (priv->slideshow_delay * SECONDS,
						 slideshow_timeout_cb,
						 fullscreen);
}


static void
viewer_image_loaded_cb (ImageViewer   *iviewer,
			GthFullscreen *fullscreen)
{
	GthFullscreenPrivateData *priv = fullscreen->priv;

	if (priv->use_fade) {
		gs_fade_in (priv->fade);
		return;
	}

	gs_fade_reset (priv->fade);
	if (priv->slideshow)
		continue_slideshow (fullscreen);
}


static void
image_loaded (GthFullscreen *fullscreen)
{
	g_signal_emit_by_name (fullscreen->priv->viewer, "image_loaded", 0);
}



static void
preloader_requested_error_cb (GThumbPreloader *gploader,
			      GthFullscreen   *fullscreen)
{
	image_viewer_set_void (IMAGE_VIEWER (fullscreen->priv->viewer));
	image_loaded (fullscreen);
}


static void
preloader_requested_done_cb (GThumbPreloader *gploader,
			     GthFullscreen   *fullscreen)
{
	GthFullscreenPrivateData *priv = fullscreen->priv;
	ImageLoader              *loader;

	loader = gthumb_preloader_get_loader (priv->preloader, priv->requested_path);
	if (loader != NULL)
		image_viewer_load_from_image_loader (IMAGE_VIEWER (priv->viewer), loader);
}


static void real_load_current_image (GthFullscreen *fullscreen);


static void
fade_faded_cb (GSFade          *fade,
	       GsFadeDirection  direction,
	       GthFullscreen   *fullscreen)
{
	switch (direction) {
	case GS_FADE_DIRECTION_OUT:
		real_load_current_image (fullscreen);
		break;

	case GS_FADE_DIRECTION_IN:
		if (fullscreen->priv->slideshow)
			continue_slideshow (fullscreen);
		fullscreen->priv->use_fade = FALSE;
		break;
	}
}


static void
gth_fullscreen_init (GthFullscreen *fullscreen)
{
	GthFullscreenPrivateData *priv;

	priv = fullscreen->priv = g_new0 (GthFullscreenPrivateData, 1);
	priv->first_time_show = TRUE;
	priv->mouse_hide_id = 0;

	priv->preloader = gthumb_preloader_new ();
	g_signal_connect (G_OBJECT (priv->preloader),
			  "requested_done",
			  G_CALLBACK (preloader_requested_done_cb),
			  fullscreen);
	g_signal_connect (G_OBJECT (priv->preloader),
			  "requested_error",
			  G_CALLBACK (preloader_requested_error_cb),
			  fullscreen);

	priv->screensaver = totem_scrsaver_new ();

	priv->fade = gs_fade_new ();
	g_signal_connect (G_OBJECT (priv->fade),
			  "faded",
			  G_CALLBACK (fade_faded_cb),
			  fullscreen);
	priv->use_fade = FALSE;
}


static gboolean
hide_mouse_pointer_cb (gpointer data)
{
	GthFullscreen            *fullscreen = data;
	GthFullscreenPrivateData *priv = fullscreen->priv;
	int                       x, y, w, h, px, py;

	gdk_window_get_pointer (priv->toolbar_window->window, &px, &py, 0);
	gdk_window_get_geometry (priv->toolbar_window->window, &x, &y, &w, &h, NULL);

	if ((px >= x) && (px <= x + w) && (py >= y) && (py <= y + h))
		return TRUE;

	gtk_widget_hide (priv->toolbar_window);

	image_viewer_hide_cursor (IMAGE_VIEWER (priv->viewer));
	priv->mouse_hide_id = 0;

	if (priv->image_data_visible)
		priv->comment_visible = TRUE;
	if (priv->comment_visible)
		gtk_widget_queue_draw (fullscreen->priv->viewer);

        return FALSE;
}


static gboolean
motion_notify_event_cb (GtkWidget      *widget,
			GdkEventMotion *event,
			gpointer        data)
{
	GthFullscreen            *fullscreen = GTH_FULLSCREEN (widget);
	GthFullscreenPrivateData *priv = fullscreen->priv;
	static int                last_px = 0, last_py = 0;
	int                       px, py;

	gdk_window_get_pointer (widget->window, &px, &py, NULL);

	if (last_px == 0)
		last_px = px;
	if (last_py == 0)
		last_py = py;

	if ((abs (last_px - px) > MOTION_THRESHOLD)
	    || (abs (last_py - py) > MOTION_THRESHOLD))
		if (! GTK_WIDGET_VISIBLE (priv->toolbar_window)) {
			gtk_widget_show (priv->toolbar_window);
			image_viewer_show_cursor (IMAGE_VIEWER (priv->viewer));
		}

	if (priv->mouse_hide_id != 0)
		g_source_remove (priv->mouse_hide_id);
	priv->mouse_hide_id = g_timeout_add (HIDE_DELAY,
					     hide_mouse_pointer_cb,
					     fullscreen);

	last_px = px;
	last_py = py;

	return FALSE;
}


static const char*
get_image_filename (GList *current)
{
	if (current == NULL)
		return NULL;
	return current->data;
}


static gboolean
all_images_viewed_at_least_once (GthFullscreen *fullscreen)
{
	GthFullscreenPrivateData *priv = fullscreen->priv;
	return (priv->slideshow
		&& (!priv->slideshow_wrap_around)
		&& (priv->viewed >= priv->files));
}


static GList*
get_next_image (GthFullscreen *fullscreen)
{
	GthFullscreenPrivateData *priv = fullscreen->priv;
	gboolean  reverse;
	GList    *next;

	if (priv->current == NULL)
		return NULL;

	if (all_images_viewed_at_least_once (fullscreen))
		return NULL;

	reverse = priv->slideshow && (priv->slideshow_direction == GTH_DIRECTION_REVERSE);
	if (reverse)
		next = priv->current->prev;
	else
		next = priv->current->next;

	if ((next == NULL)
	    && priv->slideshow
	    && !all_images_viewed_at_least_once (fullscreen)) {
		if (reverse)
			next = g_list_last (priv->file_list);
		else
			next = priv->file_list;
	}

	return next;
}


static GList*
get_prev_image (GthFullscreen *fullscreen)
{
	GthFullscreenPrivateData *priv = fullscreen->priv;
	gboolean  reverse;
	GList    *next;

	if (priv->current == NULL)
		return NULL;

	if (all_images_viewed_at_least_once (fullscreen))
		return NULL;

	reverse = priv->slideshow && (priv->slideshow_direction == GTH_DIRECTION_REVERSE);
	if (reverse)
		next = priv->current->next;
	else
		next = priv->current->prev;

	if ((next == NULL)
	    && priv->slideshow
	    && !all_images_viewed_at_least_once (fullscreen)) {
		if (reverse)
			next = priv->file_list;
		else
			next = g_list_last (priv->file_list);
	}

	return next;
}


static void
set_action_sensitive (GthFullscreen *fullscreen,
		      const char    *action_name,
		      gboolean       sensitive)
{
	GtkAction *action;

	action = gtk_action_group_get_action (fullscreen->priv->actions, action_name);
	g_object_set (action, "sensitive", sensitive, NULL);
}


static void
update_zoom_sensitivity (GthFullscreen *fullscreen)
{
	GthFullscreenPrivateData *priv = fullscreen->priv;
	gboolean                  image_is_void;
	int                       zoom;

	image_is_void = image_viewer_is_void (IMAGE_VIEWER (priv->viewer));
	zoom = (int) (IMAGE_VIEWER (priv->viewer)->zoom_level * 100.0);

	set_action_sensitive (fullscreen,
			      "View_Zoom100",
			      !image_is_void && (zoom != 100));
	set_action_sensitive (fullscreen,
			      "View_ZoomIn",
			      !image_is_void && (zoom != 10000));
	set_action_sensitive (fullscreen,
			      "View_ZoomOut",
			      !image_is_void && (zoom != 5));
	set_action_sensitive (fullscreen,
			      "View_ZoomFit",
			      !image_is_void);
        set_action_sensitive (fullscreen,
                              "View_ZoomWidth",
                              !image_is_void);
}


static void
update_sensitivity (GthFullscreen *fullscreen)
{
	set_action_sensitive (fullscreen,
			      "View_PrevImage",
			      get_prev_image (fullscreen) != NULL);
	set_action_sensitive (fullscreen,
			      "View_NextImage",
			      get_next_image (fullscreen) != NULL);
	update_zoom_sensitivity (fullscreen);
}


static gboolean
zoom_changed_cb (GtkWidget     *widget,
		 GthFullscreen *fullscreen)
{
	update_zoom_sensitivity (fullscreen);
	return TRUE;
}


static void
real_load_current_image (GthFullscreen *fullscreen)
{
	GthFullscreenPrivateData *priv = fullscreen->priv;
	gboolean from_pixbuf = TRUE;

	if (priv->current != NULL) {
		char *filename = (char*) priv->current->data;

		if ((priv->image != NULL)
		    && (priv->image_path != NULL)
		    && same_uri (priv->image_path, filename))
			image_viewer_set_pixbuf (IMAGE_VIEWER (priv->viewer), priv->image);

		else {
			g_free (priv->requested_path);
			priv->requested_path = g_strdup (filename);
			gthumb_preloader_start (priv->preloader,
						priv->requested_path,
						get_image_filename (get_next_image (fullscreen)),
						get_image_filename (get_prev_image (fullscreen)));
			from_pixbuf = FALSE;
		}

	} else if (priv->image != NULL)
		image_viewer_set_pixbuf (IMAGE_VIEWER (priv->viewer), priv->image);

	priv->viewed++;

	update_sensitivity (fullscreen);

	if (from_pixbuf)
		image_loaded (fullscreen);
}


static void
load_current_image (GthFullscreen *fullscreen)
{
	GthFullscreenPrivateData *priv = fullscreen->priv;

	if (priv->use_fade) {
		gs_fade_out (fullscreen->priv->fade);
		return;
	}

	gs_fade_reset (fullscreen->priv->fade);
	real_load_current_image (fullscreen);
}


static int
random_list_func (gconstpointer a,
		  gconstpointer b)
{
	return g_random_int_range (-1, 2);
}


static void
load_first_or_last_image (GthFullscreen *fullscreen,
			  gboolean       first,
			  gboolean       first_time)
{
	GthFullscreenPrivateData *priv = fullscreen->priv;

	if (priv->file_list != NULL) {
		if (priv->slideshow) {
			if (priv->slideshow_direction == GTH_DIRECTION_RANDOM)
				priv->file_list = g_list_sort (priv->file_list, random_list_func);

			if (priv->slideshow_direction == GTH_DIRECTION_REVERSE) {
				if (first)
					priv->current = g_list_last (priv->file_list);
				else
					priv->current = priv->file_list;
			} else {
				if (first)
					priv->current = priv->file_list;
				else
					priv->current = g_list_last (priv->file_list);
			}

		} else if (priv->image_path != NULL)
			priv->current = g_list_find_custom (priv->file_list,
							    priv->image_path,
							    (GCompareFunc) uricmp);

		if (priv->current == NULL)
			priv->current = priv->file_list;
	}

	if (first_time
	    && (priv->image_path != NULL)
	    && (priv->slideshow_direction != GTH_DIRECTION_RANDOM))
		priv->current = g_list_find_custom (priv->file_list,
						    priv->image_path,
						    (GCompareFunc) uricmp);

	load_current_image (fullscreen);
}


static void
load_first_image (GthFullscreen *fullscreen)
{
	load_first_or_last_image (fullscreen, TRUE, FALSE);
}


static void
load_last_image (GthFullscreen *fullscreen)
{
	load_first_or_last_image (fullscreen, FALSE, FALSE);
}


static void
load_next_image (GthFullscreen *fullscreen)
{
	GthFullscreenPrivateData *priv = fullscreen->priv;
	GList *next;

	next = get_next_image (fullscreen);

	if (next != NULL) {
		priv->current = next;
		load_current_image (fullscreen);
	}
	else if (priv->slideshow) {
		if (priv->slideshow_wrap_around)
			load_first_image (fullscreen);
		else
			gth_window_close (GTH_WINDOW (fullscreen));
	}
}


static void
load_prev_image (GthFullscreen *fullscreen)
{
	GthFullscreenPrivateData *priv = fullscreen->priv;
	GList *next;

	next = get_prev_image (fullscreen);

	if (next != NULL) {
		priv->current = next;
		load_current_image (fullscreen);
	}
	else if (priv->slideshow) {
		if (priv->slideshow_wrap_around)
			load_last_image (fullscreen);
		else
			gth_window_close (GTH_WINDOW (fullscreen));
	}
}


static void
make_transparent (guchar *data, int i, guchar alpha)
{
	i *= 4;

	data[i++] = 0x00;
	data[i++] = 0x00;
	data[i++] = 0x00;
	data[i++] = alpha;
}


static void
render_background (GdkDrawable    *drawable,
		   PangoRectangle *bounds)
{
	GdkPixbuf *pixbuf;
	guchar    *data;
	int        rowstride;

	pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8,
				 bounds->width, bounds->height);
	gdk_pixbuf_fill (pixbuf, 0xF0F0F0D0);

	/**/

	data = gdk_pixbuf_get_pixels (pixbuf);
	rowstride = gdk_pixbuf_get_rowstride (pixbuf);

	/* first corner */
	make_transparent (data, 0, 0x00);
	make_transparent (data, 1, 0x33);
	make_transparent (data, bounds->width, 0x33);

	/* second corner */
	make_transparent (data, bounds->width - 2, 0x33);
	make_transparent (data, bounds->width - 1, 0x00);
	make_transparent (data, bounds->width * 2 - 1, 0x33);

	/* third corner */
	make_transparent (data, bounds->width * (bounds->height - 2), 0x33);
	make_transparent (data, bounds->width * (bounds->height - 1), 0x00);
	make_transparent (data, bounds->width * (bounds->height - 1) + 1, 0x33);

	/* forth corner */
	make_transparent (data, bounds->width * (bounds->height - 1) - 1, 0x33);
	make_transparent (data, bounds->width * bounds->height - 2, 0x33);
	make_transparent (data, bounds->width * bounds->height - 1, 0x00);

	/**/

	gdk_pixbuf_render_to_drawable_alpha (pixbuf,
					     drawable,
					     0, 0,
					     0, 0,
					     bounds->width, bounds->height,
					     GDK_PIXBUF_ALPHA_FULL,
					     255,
					     GDK_RGB_DITHER_NONE, 0, 0);
	g_object_unref (pixbuf);
}


static void
render_frame (GdkDrawable    *drawable,
	      GdkGC          *gc,
	      PangoRectangle *bounds)
{
	GdkPoint p[9];

	p[0].x = 2;
	p[0].y = 0;
	p[1].x = bounds->width - 3;
	p[1].y = 0;
	p[2].x = bounds->width - 1;
	p[2].y = 2;
	p[3].x = bounds->width - 1;
	p[3].y = bounds->height - 3;
	p[4].x = bounds->width - 3;
	p[4].y = bounds->height - 1;
	p[5].x = 2;
	p[5].y = bounds->height - 1;
	p[6].x = 0;
	p[6].y = bounds->height - 3;
	p[7].x = 0;
	p[7].y = 2;
	p[8].x = 2;
	p[8].y = 0;

	gdk_draw_lines (drawable, gc, p, 9);
}


static char *
escape_filename (const char *text)
{
	char *utf8_text;
	char *escaped_text;

	utf8_text = g_filename_display_name (text);
	escaped_text = g_markup_escape_text (utf8_text, -1);
	g_free (utf8_text);

	return escaped_text;
}


static GdkPixmap*
get_pixmap (GdkDrawable    *drawable,
	    GdkGC          *gc,
	    PangoRectangle *rect)
{
	GdkPixmap *pixmap;

	pixmap = gdk_pixmap_new (drawable, rect->width, rect->height, -1);
	gdk_draw_drawable (pixmap,
			   gc,
			   drawable,
			   rect->x, rect->y,
			   0, 0,
			   rect->width, rect->height);

	return pixmap;
}


static int
pos_from_path (GList      *list,
	       const char *path)
{
	GList *scan;
	int    i = 0;

	if ((list == NULL) || (path == NULL))
		return 0;

	for (scan = list; scan; scan = scan->next) {
		char *l_path = scan->data;
		if (same_uri (l_path, path))
			return i;
		i++;
	}

	return 0;
}


static char *
get_file_info (GthFullscreen *fullscreen)
{
	GthFullscreenPrivateData *priv = fullscreen->priv;
	const char  *image_filename;
	ImageViewer *image_viewer = (ImageViewer*) fullscreen->priv->viewer;
	char        *e_filename;
	int          width, height;
	char        *size_txt;
	char        *file_size_txt;
	int          zoom;
	char        *file_info;
	char         time_txt[50], *utf8_time_txt;
	time_t       timer = 0;
	struct tm   *tm;

	image_filename = gth_window_get_image_filename (GTH_WINDOW (fullscreen));
	e_filename = escape_filename (file_name_from_path (image_filename));

	width = image_viewer_get_image_width (image_viewer);
	height = image_viewer_get_image_height (image_viewer);

	size_txt = g_strdup_printf (_("%d x %d pixels"), width, height);
	file_size_txt = gnome_vfs_format_file_size_for_display (get_file_size (image_filename));

	zoom = (int) (image_viewer->zoom_level * 100.0);

	timer = get_exif_time (image_filename);
	if (timer == 0)
		timer = get_file_mtime (image_filename);
	tm = localtime (&timer);
	strftime (time_txt, 50, _("%d %B %Y, %H:%M"), tm);
	utf8_time_txt = g_locale_to_utf8 (time_txt, -1, 0, 0, 0);

	file_info = g_strdup_printf ("<small><i>%s - %s (%d%%) - %s</i>\n%d/%d - <tt>%s</tt></small>",
				     utf8_time_txt,
				     size_txt,
				     zoom,
				     file_size_txt,
				     pos_from_path (priv->file_list, image_filename) + 1,
				     priv->files,
				     e_filename);

	g_free (utf8_time_txt);
	g_free (e_filename);
	g_free (size_txt);
	g_free (file_size_txt);

	return file_info;
}


static void
show_comment_on_image (GthFullscreen *fullscreen)
{
	GthWindow      *window = GTH_WINDOW (fullscreen);
	GthFullscreenPrivateData *priv = fullscreen->priv;
	ImageViewer    *viewer = (ImageViewer*) priv->viewer;
	CommentData    *cdata = NULL;
	char           *comment, *e_comment, *file_info;
	char           *marked_text, *parsed_text;
	PangoLayout    *layout;
	PangoAttrList  *attr_list = NULL;
        GError         *error = NULL;
	int             text_x, text_y;
	GdkPixbuf      *icon;
	int             icon_x, icon_y, icon_w, icon_h;
	int             max_text_width;

	if (priv->comment_visible)
		return;

	if (priv->buffer != NULL) {
		g_object_unref (priv->buffer);
		g_object_unref (priv->original_buffer);
		priv->buffer = NULL;
		priv->original_buffer = NULL;
	}

	if (gth_window_get_image_filename (window) != NULL)
		cdata = comments_load_comment (gth_window_get_image_filename (window), TRUE);
	else
		return;

	priv->comment_visible = TRUE;

	comment = NULL;
	if (cdata != NULL) {
		comment = comments_get_comment_as_string (cdata, "\n", " - ");
		comment_data_free (cdata);
	}

	file_info = get_file_info (fullscreen);

	if (comment == NULL)
		marked_text = g_strdup (file_info);
	else {
		e_comment = g_markup_escape_text (comment, -1);
		marked_text = g_strdup_printf ("<b>%s</b>\n\n%s",
					       e_comment,
					       file_info);
		g_free (e_comment);
	}

	g_free (file_info);

	/**/

	layout = gtk_widget_create_pango_layout (GTK_WIDGET (viewer), NULL);
	pango_layout_set_wrap (layout, PANGO_WRAP_WORD);
	pango_layout_set_font_description (layout, GTK_WIDGET (viewer)->style->font_desc);
	pango_layout_set_alignment (layout, PANGO_ALIGN_LEFT);

	if (! pango_parse_markup (marked_text, -1,
				  0,
				  &attr_list,
				  &parsed_text,
				  NULL,
				  &error)) {
		g_warning ("Failed to set text from markup due to error parsing markup: %s\nThis is the text that caused the error: %s",  error->message, marked_text);
		g_error_free (error);
		g_free (marked_text);
		g_object_unref (layout);
		return;
	}
	g_free (marked_text);

	pango_layout_set_attributes (layout, attr_list);
        pango_attr_list_unref (attr_list);

	pango_layout_set_text (layout, parsed_text, strlen (parsed_text));
	g_free (parsed_text);

	icon = gdk_pixbuf_new_from_inline (-1, add_comment_24_rgba, FALSE, NULL);
	icon_w = gdk_pixbuf_get_width (icon);
	icon_h = gdk_pixbuf_get_height (icon);

	max_text_width = ((gdk_screen_width () * 3 / 4)
			  - icon_w
			  - (X_PADDING * 3)
			  - (X_PADDING * 2));

	pango_layout_set_width (layout, max_text_width * PANGO_SCALE);
	pango_layout_get_pixel_extents (layout, NULL, &priv->bounds);

	priv->bounds.width += (2 * X_PADDING) + (icon_w + X_PADDING);
	priv->bounds.height += 2 * Y_PADDING;
	priv->bounds.height = MIN (gdk_screen_height () - icon_h - (Y_PADDING * 2),
				   priv->bounds.height);

	priv->bounds.x = (gdk_screen_width () - priv->bounds.width) / 2;
	priv->bounds.y = gdk_screen_height () - priv->bounds.height - (Y_PADDING * 3);
	priv->bounds.x = MAX (priv->bounds.x, 0);
	priv->bounds.y = MAX (priv->bounds.y, 0);

	text_x = X_PADDING + icon_w + X_PADDING;
	text_y = Y_PADDING;
	icon_x = X_PADDING;
	icon_y = (priv->bounds.height - icon_h) / 2;

	priv->buffer = get_pixmap (GTK_WIDGET (viewer)->window,
				   GTK_WIDGET (viewer)->style->black_gc,
				   &priv->bounds);
	priv->original_buffer = get_pixmap (GTK_WIDGET (viewer)->window,
					    GTK_WIDGET (viewer)->style->black_gc,
					    &priv->bounds);
	render_background (priv->buffer, &priv->bounds);
	render_frame      (priv->buffer,
			   GTK_WIDGET (viewer)->style->black_gc,
			   &priv->bounds);
	gdk_draw_layout (priv->buffer,
			 GTK_WIDGET (viewer)->style->black_gc,
			 text_x,
			 text_y,
			 layout);
	gdk_pixbuf_render_to_drawable (icon,
				       priv->buffer,
				       GTK_WIDGET (viewer)->style->black_gc,
				       0, 0,
				       icon_x, icon_y,
				       icon_w, icon_h,
				       GDK_RGB_DITHER_NONE,
				       0, 0);
	g_object_unref (icon);
	g_object_unref (layout);

	gdk_draw_drawable (GTK_WIDGET (viewer)->window,
			   GTK_WIDGET (viewer)->style->black_gc,
			   priv->buffer,
			   0, 0,
			   priv->bounds.x, priv->bounds.y,
			   priv->bounds.width, priv->bounds.height);

	viewer->next_scroll_repaint = TRUE;
}


static void
hide_comment_on_image (GthFullscreen *fullscreen)
{
	GthFullscreenPrivateData *priv = fullscreen->priv;
	ImageViewer *viewer = (ImageViewer*) priv->viewer;

	viewer->next_scroll_repaint = FALSE;

	if (priv->original_buffer == NULL)
		return;

	priv->comment_visible = FALSE;

	gdk_draw_drawable (priv->viewer->window,
			   priv->viewer->style->black_gc,
			   priv->original_buffer,
			   0, 0,
			   priv->bounds.x, priv->bounds.y,
			   priv->bounds.width, priv->bounds.height);
	gdk_flush ();
}


static void
_show_cursor__hide_comment (GthFullscreen *fullscreen)
{
	GthFullscreenPrivateData *priv = fullscreen->priv;

	if (priv->slideshow)
		priv->slideshow_paused = TRUE;

	if (priv->mouse_hide_id != 0)
		g_source_remove (priv->mouse_hide_id);
	priv->mouse_hide_id = 0;

	image_viewer_show_cursor ((ImageViewer*) priv->viewer);
	if (priv->comment_visible)
		hide_comment_on_image (fullscreen);
}


static void
delete_current_image (GthFullscreen *fullscreen)
{
	const char *image_filename;
	GList      *list;

	image_filename = gth_window_get_image_filename (GTH_WINDOW (fullscreen));
	if (image_filename == NULL)
		return;
	list = g_list_prepend (NULL, g_strdup (image_filename));

	if (fullscreen->priv->catalog_path == NULL)
		dlg_file_delete__confirm (GTH_WINDOW (fullscreen),
					  list,
					  _("The image will be moved to the Trash, are you sure?"));
	else
		remove_files_from_catalog (GTH_WINDOW (fullscreen),
					   fullscreen->priv->catalog_path,
					   list);
}


static gboolean
mouse_wheel_scrolled_cb (GtkWidget          *widget,
                         GdkScrollDirection  direction,
                         gpointer            data)
{
	GthFullscreen *fullscreen = data;

        if (direction == GDK_SCROLL_UP)
                gth_fullscreen_show_prev_image (fullscreen);
        else
                gth_fullscreen_show_next_image (fullscreen);

        return TRUE;
}


static gboolean
viewer_key_press_cb (GtkWidget   *widget,
		     GdkEventKey *event,
		     gpointer     data)
{
	GthFullscreen *fullscreen = data;
	GthFullscreenPrivateData *priv = fullscreen->priv;
	GthWindow     *window = (GthWindow*) fullscreen;
	ImageViewer   *viewer = (ImageViewer*) fullscreen->priv->viewer;
	gboolean       retval = TRUE;
	GtkAction     *a;

	if ((event->state & GDK_CONTROL_MASK) == GDK_CONTROL_MASK) {
		switch (gdk_keyval_to_lower (event->keyval)) {
			/* Exit fullscreen mode. */
		case GDK_w:
			gth_window_close (window);
			break;
		default:
			retval = FALSE;
			break;
		}

		if (retval)
			return retval;
	}


	switch (gdk_keyval_to_lower (event->keyval)) {

		/* Exit fullscreen mode. */
	case GDK_Escape:
	case GDK_q:
	case GDK_v:
	case GDK_f:
	case GDK_F11:
		gth_window_close (window);
		break;

	case GDK_s:
		if (fullscreen->priv->slideshow)
			gth_window_close (window);
		else
			retval = FALSE;
		break;

		/* Zoom in. */
	case GDK_plus:
	case GDK_equal:
	case GDK_KP_Add:
		image_viewer_zoom_in (viewer);
		break;

		/* Zoom out. */
	case GDK_minus:
	case GDK_KP_Subtract:
		image_viewer_zoom_out (viewer);
		break;

		/* Actual size. */
	case GDK_KP_Divide:
	case GDK_1:
	case GDK_z:
		image_viewer_set_zoom (viewer, 1.0);
		break;

		/* Set zoom to 2.0. */
	case GDK_2:
		image_viewer_set_zoom (viewer, 2.0);
		break;

		/* Set zoom to 3.0. */
	case GDK_3:
		image_viewer_set_zoom (viewer, 3.0);
		break;

		/* Zoom to fit. */
	case GDK_x:
		image_viewer_set_fit_mode (viewer, GTH_FIT_SIZE_IF_LARGER);
		break;

		/* Toggle animation. */
	case GDK_a:
		gth_window_set_animation (window, ! gth_window_get_animation (window));
		break;

		/* Step animation. */
	case GDK_j:
        	gth_window_step_animation (window);
		break;

		/* Load next image. */
	case GDK_space:
	case GDK_n:
	case GDK_Page_Down:
		load_next_image (fullscreen);
		break;

		/* Load previous image. */
	case GDK_b:
	case GDK_BackSpace:
	case GDK_Page_Up:
		load_prev_image (fullscreen);
		break;

		/* Show first image. */
	case GDK_Home:
	case GDK_KP_Home:
		priv->current = priv->file_list;
		load_current_image (fullscreen);
		break;

		/* Show last image. */
	case GDK_End:
	case GDK_KP_End:
		priv->current = g_list_last (priv->file_list);
		load_current_image (fullscreen);
		break;

	case GDK_p:
		a = gtk_ui_manager_get_action (priv->ui, "/ToolBar/View_PauseSlideshow");
		if (a != NULL)
			gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (a), !priv->slideshow_paused);
		break;

		/* Edit comment. */
	case GDK_c:
		_show_cursor__hide_comment (fullscreen);
		gth_window_edit_comment (GTH_WINDOW (fullscreen));
		break;

		/* Edit categories. */
	case GDK_k:
		_show_cursor__hide_comment (fullscreen);
		gth_window_edit_categories (window);
		break;

		/* View/Hide comment */
	case GDK_i:
		if (priv->comment_visible) {
			priv->image_data_visible = FALSE;
			hide_comment_on_image (fullscreen);
		} else {
			priv->image_data_visible = TRUE;
			show_comment_on_image (fullscreen);
		}
		break;

		/* Rotate clockwise without saving */
	case GDK_r:
		gth_window_activate_action_alter_image_rotate90 (NULL, window);
		break;

		/* Rotate counter-clockwise without saving */
	case GDK_e:
		gth_window_activate_action_alter_image_rotate90cc (NULL, window);
		break;

		/* Lossless clockwise rotation. */
	case GDK_bracketright:
		gth_window_activate_action_tools_jpeg_rotate_right (NULL, window);
		break;

		/* Lossless counter-clockwise rotation */
	case GDK_bracketleft:
		gth_window_activate_action_tools_jpeg_rotate_left (NULL, window);
		break;

		/* Flip image */
	case GDK_l:
		gth_window_activate_action_alter_image_flip (NULL, window);
		break;

		/* Mirror image */
	case GDK_m:
		gth_window_activate_action_alter_image_mirror (NULL, window);
		break;

		/* Delete selection. */
	case GDK_Delete:
	case GDK_KP_Delete:
		_show_cursor__hide_comment (fullscreen);
		delete_current_image (fullscreen);
		break;

	default:
		if (priv->comment_visible)
			hide_comment_on_image (fullscreen);
		retval = FALSE;
		break;
	}

	return retval;
}


static gboolean
fs_expose_event_cb (GtkWidget        *widget,
		    GdkEventExpose   *event,
		    GthFullscreen    *fullscreen)
{
	if (fullscreen->priv->comment_visible) {
		fullscreen->priv->comment_visible = FALSE;
		show_comment_on_image (fullscreen);
	}

	return FALSE;
}


static gboolean
fs_repainted_cb (GtkWidget     *widget,
		 GthFullscreen *fullscreen)
{
	fullscreen->priv->comment_visible = FALSE;
	return TRUE;
}


static int
image_button_release_cb (GtkWidget      *widget,
			 GdkEventButton *event,
			 gpointer        data)
{
	GthFullscreen *fullscreen = data;

	switch (event->button) {
	case 2:
		load_prev_image (fullscreen);
		return TRUE;
	default:
		break;
	}

	return FALSE;
}


static void
monitor_update_metadata_cb (GthMonitor    *monitor,
			    const char    *filename,
			    GthFullscreen *fullscreen)
{
	g_return_if_fail (fullscreen != NULL);
	load_current_image (fullscreen);
}


static void
update_current_image_link (GthFullscreen *fullscreen)
{
	GthFullscreenPrivateData *priv = fullscreen->priv;

	if (priv->current == NULL)
		return;

	if (priv->current->next != NULL)
		priv->current = priv->current->next;

	else if (priv->current->prev != NULL)
		priv->current = priv->current->prev;

	else {
		priv->current = NULL;
		return;
	}

	g_free (priv->image_path);
	priv->image_path = NULL;
}


static void
delete_list_from_file_list (GthFullscreen *fullscreen,
			    GList         *list)
{
	gboolean  reload_current_image = FALSE;
	GList    *scan;

	for (scan = list; scan; scan = scan->next) {
		char  *filename = scan->data;
		GList *deleted;

		deleted = g_list_find_custom (fullscreen->priv->file_list,
					      filename,
					      (GCompareFunc) uricmp);
		if (deleted != NULL) {
			if (fullscreen->priv->current == deleted) {
				reload_current_image = TRUE;
				update_current_image_link (fullscreen);
			}

			fullscreen->priv->file_list = g_list_remove_link (fullscreen->priv->file_list, deleted);
			path_list_free (deleted);
			fullscreen->priv->files = g_list_length (fullscreen->priv->file_list);
		}
	}

	if (reload_current_image) {
		if (fullscreen->priv->current != NULL)
			load_current_image (fullscreen);
		else
			gth_window_close (GTH_WINDOW (fullscreen));
	}
}


static void
monitor_update_files_cb (GthMonitor      *monitor,
			 GthMonitorEvent  event,
			 GList           *list,
			 GthFullscreen   *fullscreen)
{
	g_return_if_fail (fullscreen != NULL);

	if (fullscreen->priv->current == NULL)
		return;

	switch (event) {
	case GTH_MONITOR_EVENT_CREATED:
	case GTH_MONITOR_EVENT_CHANGED:
		if ((fullscreen->priv->image_path != NULL)
		    && (g_list_find_custom (list,
					    fullscreen->priv->image_path,
					    (GCompareFunc) uricmp) != NULL)) {
			g_free (fullscreen->priv->image_path);
			fullscreen->priv->image_path = NULL;
			if (fullscreen->priv->image != NULL)
				g_object_unref (fullscreen->priv->image);
		}
		if ((fullscreen->priv->current != NULL)
		    && (g_list_find_custom (list,
					    fullscreen->priv->current->data,
					    (GCompareFunc) uricmp) != NULL))
			load_current_image (fullscreen);
		break;

	case GTH_MONITOR_EVENT_DELETED:
		delete_list_from_file_list (fullscreen, list);
		break;

	default:
		break;
	}
}


static void
monitor_file_renamed_cb (GthMonitor    *monitor,
			 const char    *old_name,
			 const char    *new_name,
			 GthFullscreen *fullscreen)
{
	GList *renamed_image;

	renamed_image = g_list_find_custom (fullscreen->priv->file_list,
					    old_name,
					    (GCompareFunc) uricmp);

	if (renamed_image == NULL)
		return;

	g_free (renamed_image->data);
	renamed_image->data = g_strdup (new_name);

	if (fullscreen->priv->image_path == NULL)
		return;

	if (! same_uri (old_name, fullscreen->priv->image_path))
		return;

	g_free (fullscreen->priv->image_path);
	fullscreen->priv->image_path = g_strdup (new_name);

	load_current_image (fullscreen);
}


static void
gth_fullscreen_construct (GthFullscreen *fullscreen,
			  GdkPixbuf     *image,
			  const char    *image_path,
			  GList         *file_list)
{
	GthFullscreenPrivateData *priv = fullscreen->priv;
	GdkScreen *screen = gtk_widget_get_screen (GTK_WIDGET (fullscreen));
	GthZoomChange zoom_change = pref_get_zoom_change ();

	gtk_window_set_default_size (GTK_WINDOW (fullscreen),
				     gdk_screen_get_width (screen),
				     gdk_screen_get_height (screen));

	if (image != NULL) {
		priv->image = image;
		g_object_ref (image);
	}

	if (image_path != NULL)
		priv->image_path = g_strdup (image_path);
	else
		priv->image_path = NULL;

	priv->file_list = file_list;
	if (file_list != NULL)
		priv->files = MAX (g_list_length (priv->file_list), 1);
	else
		priv->files = 1;
	priv->current = NULL;
	priv->viewed = 0;

	priv->viewer = image_viewer_new ();
	image_viewer_set_zoom_quality (IMAGE_VIEWER (priv->viewer),
				       pref_get_zoom_quality ());
	image_viewer_set_zoom_change  (IMAGE_VIEWER (priv->viewer),
				       zoom_change);
	image_viewer_set_check_type   (IMAGE_VIEWER (priv->viewer),
				       pref_get_check_type ());
	image_viewer_set_check_size   (IMAGE_VIEWER (priv->viewer),
				       pref_get_check_size ());
	image_viewer_set_transp_type  (IMAGE_VIEWER (priv->viewer),
				       pref_get_transp_type ());
	image_viewer_set_black_background (IMAGE_VIEWER (priv->viewer), TRUE);
	image_viewer_hide_frame (IMAGE_VIEWER (priv->viewer));

	if (zoom_change == GTH_ZOOM_CHANGE_FIT_SIZE)
		image_viewer_set_fit_mode (IMAGE_VIEWER (priv->viewer), GTH_FIT_SIZE);
	else if ((zoom_change == GTH_ZOOM_CHANGE_FIT_SIZE_IF_LARGER) || (zoom_change == GTH_ZOOM_CHANGE_KEEP_PREV))
		image_viewer_set_fit_mode (IMAGE_VIEWER (priv->viewer), GTH_FIT_SIZE_IF_LARGER);
	else if (zoom_change == GTH_ZOOM_CHANGE_FIT_WIDTH_IF_LARGER)
		image_viewer_set_fit_mode (IMAGE_VIEWER (priv->viewer), GTH_FIT_WIDTH_IF_LARGER);
	else
		image_viewer_set_zoom (IMAGE_VIEWER (priv->viewer), 1.0);

	g_signal_connect (G_OBJECT (priv->viewer),
			  "key_press_event",
			  G_CALLBACK (viewer_key_press_cb),
			  fullscreen);
        g_signal_connect_after (G_OBJECT (priv->viewer),
                          "mouse_wheel_scroll",
                          G_CALLBACK (mouse_wheel_scrolled_cb),
                          fullscreen);
	g_signal_connect_after (G_OBJECT (priv->viewer),
				"expose_event",
				G_CALLBACK (fs_expose_event_cb),
				fullscreen);
	g_signal_connect_after (G_OBJECT (priv->viewer),
				"repainted",
				G_CALLBACK (fs_repainted_cb),
				fullscreen);
	g_signal_connect_swapped (G_OBJECT (priv->viewer),
				  "clicked",
				  G_CALLBACK (load_next_image),
				  fullscreen);
	g_signal_connect (G_OBJECT (priv->viewer),
			  "button_release_event",
			  G_CALLBACK (image_button_release_cb),
			  fullscreen);
	g_signal_connect (G_OBJECT (priv->viewer),
			  "zoom_changed",
			  G_CALLBACK (zoom_changed_cb),
			  fullscreen);
	g_signal_connect (G_OBJECT (priv->viewer),
			  "image_loaded",
			  G_CALLBACK (viewer_image_loaded_cb),
			  fullscreen);

	gtk_widget_show (priv->viewer);
	gth_window_attach(GTH_WINDOW (fullscreen), priv->viewer, GTH_WINDOW_CONTENTS);

	/**/

	g_signal_connect (G_OBJECT (monitor),
			  "update_files",
			  G_CALLBACK (monitor_update_files_cb),
			  fullscreen);
	g_signal_connect (G_OBJECT (monitor),
			  "update_metadata",
			  G_CALLBACK (monitor_update_metadata_cb),
			  fullscreen);
	g_signal_connect (G_OBJECT (monitor),
			  "file_renamed",
			  G_CALLBACK (monitor_file_renamed_cb),
			  fullscreen);

	/**/

	g_signal_connect (G_OBJECT (fullscreen),
			  "motion_notify_event",
			  G_CALLBACK (motion_notify_event_cb),
			  fullscreen);
}


GtkWidget *
gth_fullscreen_new (GdkPixbuf  *image,
		    const char *image_path,
		    GList      *file_list)
{
	GthFullscreen *fullscreen;

	fullscreen = (GthFullscreen*) g_object_new (GTH_TYPE_FULLSCREEN, NULL);
	gth_fullscreen_construct (fullscreen, image, image_path, file_list);

	return (GtkWidget*) fullscreen;
}


static void
create_toolbar_window (GthFullscreen *fullscreen)
{
	GthFullscreenPrivateData *priv = fullscreen->priv;
	GtkActionGroup           *actions;
	GtkUIManager             *ui;
	GError                   *error = NULL;
	const gchar              *ui_info;

	priv->actions = actions = gtk_action_group_new ("ToolbarActions");
	gtk_action_group_set_translation_domain (actions, NULL);

	gtk_action_group_add_actions (actions,
				      gth_window_action_entries,
				      gth_window_action_entries_size,
				      fullscreen);
	gtk_action_group_add_toggle_actions (actions,
					     gth_window_action_toggle_entries,
					     gth_window_action_toggle_entries_size,
					     fullscreen);
	gtk_action_group_add_actions (actions,
				      gth_fullscreen_action_entries,
				      gth_fullscreen_action_entries_size,
				      fullscreen);
	gtk_action_group_add_toggle_actions (actions,
					     gth_fullscreen_action_toggle_entries,
					     gth_fullscreen_action_toggle_entries_size,
					     fullscreen);

	priv->ui = ui = gtk_ui_manager_new ();
	gtk_ui_manager_insert_action_group (ui, actions, 0);

	if (priv->slideshow)
		ui_info = slideshow_ui_info;
	else
		ui_info = fullscreen_ui_info;
	if (!gtk_ui_manager_add_ui_from_string (ui, ui_info, -1, &error)) {
		g_message ("building menus failed: %s", error->message);
		g_error_free (error);
	}

	/**/

	priv->toolbar_window = gtk_window_new (GTK_WINDOW_POPUP);
	gtk_window_set_screen (GTK_WINDOW (priv->toolbar_window),
			       gtk_widget_get_screen (priv->viewer));
	gtk_window_set_default_size (GTK_WINDOW (priv->toolbar_window),
				     gdk_screen_get_width (gtk_widget_get_screen (priv->toolbar_window)),
				     -1);
	gtk_container_set_border_width (GTK_CONTAINER (priv->toolbar_window), 0);

	gtk_container_add (GTK_CONTAINER (priv->toolbar_window),
			   gtk_ui_manager_get_widget (ui, "/ToolBar"));
}


static void
gth_fullscreen_show (GtkWidget *widget)
{
	GthFullscreen            *fullscreen = GTH_FULLSCREEN (widget);
	GthFullscreenPrivateData *priv = fullscreen->priv;

	GTK_WIDGET_CLASS (parent_class)->show (widget);

	if (!priv->first_time_show)
		return;
	priv->first_time_show = FALSE;

	create_toolbar_window (fullscreen);

	priv->slideshow_direction = pref_get_slideshow_direction ();
	priv->slideshow_delay = eel_gconf_get_integer (PREF_SLIDESHOW_DELAY, DEF_SLIDESHOW_DELAY);
	priv->slideshow_wrap_around = eel_gconf_get_boolean (PREF_SLIDESHOW_WRAP_AROUND, FALSE);

	image_viewer_hide_cursor (IMAGE_VIEWER (priv->viewer));
	gtk_window_fullscreen (GTK_WINDOW (widget));

	if (fullscreen->priv->slideshow)
		totem_scrsaver_disable (fullscreen->priv->screensaver);
	else
		totem_scrsaver_enable (fullscreen->priv->screensaver);

	load_first_or_last_image (fullscreen, TRUE, TRUE);
}


static void
gth_fullscreen_close (GthWindow *window)
{
	gtk_widget_destroy (GTK_WIDGET (window));
}


static ImageViewer*
gth_fullscreen_get_image_viewer (GthWindow *window)
{
	GthFullscreen *fullscreen = (GthFullscreen*) window;
	return (ImageViewer*) fullscreen->priv->viewer;
}


static const char *
gth_fullscreen_get_image_filename (GthWindow *window)
{
	GthFullscreen *fullscreen = (GthFullscreen*) window;
	return get_image_filename (fullscreen->priv->current);
}


static GList *
gth_fullscreen_get_file_list_selection (GthWindow *window)
{
	GthFullscreen            *fullscreen = GTH_FULLSCREEN (window);
	GthFullscreenPrivateData *priv = fullscreen->priv;

	if (priv->current == NULL)
		return NULL;
	return g_list_prepend (NULL, g_strdup (priv->current->data));
}


static void
gth_fullscreen_set_animation (GthWindow *window,
			       gboolean   play)
{
	GthFullscreen *fullscreen = GTH_FULLSCREEN (window);
	ImageViewer   *image_viewer = IMAGE_VIEWER (fullscreen->priv->viewer);

	if (play)
		image_viewer_start_animation (image_viewer);
	else
		image_viewer_stop_animation (image_viewer);
}


static gboolean
gth_fullscreen_get_animation (GthWindow *window)
{
	GthFullscreen *fullscreen = GTH_FULLSCREEN (window);
	return IMAGE_VIEWER (fullscreen->priv->viewer)->play_animation;
}


static void
gth_fullscreen_step_animation (GthWindow *window)
{
	GthFullscreen *fullscreen = GTH_FULLSCREEN (window);
	image_viewer_step_animation (IMAGE_VIEWER (fullscreen->priv->viewer));
}


static void
gth_fullscreen_class_init (GthFullscreenClass *class)
{
	GObjectClass   *gobject_class;
	GtkWidgetClass *widget_class;
	GthWindowClass *window_class;

	parent_class = g_type_class_peek_parent (class);
	gobject_class = (GObjectClass*) class;
	widget_class = (GtkWidgetClass*) class;
	window_class = (GthWindowClass*) class;

	gobject_class->finalize = gth_fullscreen_finalize;

	widget_class->show = gth_fullscreen_show;

	window_class->close = gth_fullscreen_close;
	window_class->get_image_viewer = gth_fullscreen_get_image_viewer;

	window_class->get_image_filename = gth_fullscreen_get_image_filename;
	/*
	window_class->get_image_modified = gth_fullscreen_get_image_modified;
	window_class->set_image_modified = gth_fullscreen_set_image_modified;
	window_class->save_pixbuf = gth_fullscreen_save_pixbuf;
	window_class->exec_pixbuf_op = gth_fullscreen_exec_pixbuf_op;

	window_class->set_categories_dlg = gth_fullscreen_set_categories_dlg;
	window_class->get_categories_dlg = gth_fullscreen_get_categories_dlg;
	window_class->set_comment_dlg = gth_fullscreen_set_comment_dlg;
	window_class->get_comment_dlg = gth_fullscreen_get_comment_dlg;
	window_class->reload_current_image = gth_fullscreen_reload_current_image;
	window_class->update_current_image_metadata = gth_fullscreen_update_current_image_metadata;
	*/
	window_class->get_file_list_selection = gth_fullscreen_get_file_list_selection;
	/*
	window_class->get_file_list_selection_as_fd = gth_fullscreen_get_file_list_selection_as_fd;

	*/
	window_class->set_animation = gth_fullscreen_set_animation;
	window_class->get_animation = gth_fullscreen_get_animation;
	window_class->step_animation = gth_fullscreen_step_animation;
	/*
	window_class->edit_comment = gth_fullscreen_edit_comment;
	window_class->edit_categories = gth_fullscreen_edit_categories;
	window_class->set_fullscreen = gth_fullscreen_set_fullscreen;
	window_class->get_fullscreen = gth_fullscreen_get_fullscreen;
	window_class->set_slideshow = gth_fullscreen_set_slideshow;
	window_class->get_slideshow = gth_fullscreen_get_slideshow;
	*/
}


GType
gth_fullscreen_get_type ()
{
        static GType type = 0;

        if (! type) {
                GTypeInfo type_info = {
			sizeof (GthFullscreenClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_fullscreen_class_init,
			NULL,
			NULL,
			sizeof (GthFullscreen),
			0,
			(GInstanceInitFunc) gth_fullscreen_init
		};

		type = g_type_register_static (GTH_TYPE_WINDOW,
					       "GthFullscreen",
					       &type_info,
					       0);
	}

        return type;
}


void
gth_fullscreen_set_slideshow (GthFullscreen *fullscreen,
			      gboolean       slideshow)
{
	g_return_if_fail (GTH_IS_FULLSCREEN (fullscreen));
	fullscreen->priv->slideshow = slideshow;
}


void
gth_fullscreen_set_catalog (GthFullscreen *fullscreen,
			    const char    *catalog_path)
{
	g_return_if_fail (GTH_IS_FULLSCREEN (fullscreen));

	g_free (fullscreen->priv->catalog_path);
	fullscreen->priv->catalog_path = NULL;

	if (catalog_path != NULL)
		fullscreen->priv->catalog_path = g_strdup (catalog_path);
}


void
gth_fullscreen_show_prev_image (GthFullscreen *fullscreen)
{
	load_prev_image (fullscreen);
}


void
gth_fullscreen_show_next_image (GthFullscreen *fullscreen)
{
	load_next_image (fullscreen);
}


void
gth_fullscreen_pause_slideshow (GthFullscreen *fullscreen,
				gboolean       value)
{
	fullscreen->priv->slideshow_paused = value;

	if (fullscreen->priv->slideshow_paused)
		totem_scrsaver_enable (fullscreen->priv->screensaver);
	else {
		totem_scrsaver_disable (fullscreen->priv->screensaver);
		continue_slideshow (fullscreen);
	}
}


void
gth_fullscreen_show_comment (GthFullscreen *fullscreen,
			     gboolean       value)
{
	fullscreen->priv->image_data_visible = value;

	if (fullscreen->priv->image_data_visible)
		show_comment_on_image (fullscreen);
	else
		hide_comment_on_image (fullscreen);
}
