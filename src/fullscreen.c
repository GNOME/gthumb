/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001, 2003, 2004 Free Software Foundation, Inc.
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
#include <math.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdkx.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include "comments.h"
#include "commands-impl.h"
#include "fullscreen.h"
#include "glib-utils.h"
#include "gconf-utils.h"
#include "gthumb-stock.h"
#include "main.h"
#include "pixbuf-utils.h"
#include "gth-exif-utils.h"
#include "preferences.h"

#include "icons/pixbufs.h"

#define HIDE_DELAY 2000
#define X_PADDING  12
#define Y_PADDING  6
#define MOTION_THRESHOLD 5

static gboolean        comment_visible = FALSE;
static GdkPixmap      *buffer = NULL;
static GdkPixmap      *original_buffer = NULL;
static PangoRectangle  bounds;
static GtkWidget      *popup_window = NULL;
static GtkWidget      *prop_button = NULL;
static FullScreen     *current_fullscreen;


static void
set_command_state (GThumbWindow *window,
		   char         *cname,
		   gboolean      setted)
{
	char *full_cname = g_strconcat ("/commands/", cname, NULL);
	char *new_value  = setted ? "1" : "0";
	bonobo_ui_component_set_prop (window->ui_component, 
				      full_cname, 
				      "state", new_value,
				      NULL);
	g_free (full_cname);
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

	utf8_text = g_filename_to_utf8 (text, -1, NULL, NULL, NULL);
	g_return_val_if_fail (utf8_text != NULL, NULL);
	
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


static char *
get_file_info (GThumbWindow *window)
{
	char       *e_filename;
	int         width, height;
	char       *size_txt;
	char       *file_size_txt;
	int         zoom;
	char       *file_info;
	char        time_txt[50], *utf8_time_txt;
	time_t      timer = 0;
	struct tm  *tm;

	e_filename = escape_filename (file_name_from_path (window->image_path));

	width = image_viewer_get_image_width (IMAGE_VIEWER (window->viewer));
	height = image_viewer_get_image_height (IMAGE_VIEWER (window->viewer));

	size_txt = g_strdup_printf (_("%d x %d pixels"), width, height);
	file_size_txt = gnome_vfs_format_file_size_for_display (get_file_size (window->image_path));

	zoom = (int) (IMAGE_VIEWER (window->viewer)->zoom_level * 100.0);

#ifdef HAVE_LIBEXIF
	timer = get_exif_time (window->image_path);
#endif
	if (timer == 0)
		timer = get_file_mtime (window->image_path);
	tm = localtime (&timer);
	strftime (time_txt, 50, _("%d %B %Y, %H:%M"), tm);
	utf8_time_txt = g_locale_to_utf8 (time_txt, -1, 0, 0, 0);

	file_info = g_strdup_printf ("<small><i>%s - %s (%d%%) - %s</i>\n%d/%d - <tt>%s</tt></small>",
				     utf8_time_txt,
				     size_txt,
				     zoom,
				     file_size_txt,
				     gth_file_list_pos_from_path (window->file_list, window->image_path) + 1,
				     gth_file_list_get_length (window->file_list),
				     e_filename);

	g_free (utf8_time_txt);
	g_free (e_filename);
	g_free (size_txt);
	g_free (file_size_txt);

	return file_info;
}


static void
set_button_active_no_notify (gpointer      data,
			     GtkWidget    *button,
			     gboolean      active)
{
	g_signal_handlers_block_by_data (button, data);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), active);
	g_signal_handlers_unblock_by_data (button, data);
}


static void
show_comment_on_image (GThumbWindow *window,
		       ImageViewer  *viewer)
{
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

	if (comment_visible) 
		return;

	if (buffer != NULL) {
		g_object_unref (buffer);
		g_object_unref (original_buffer);
		buffer = NULL;
		original_buffer = NULL;
	}

	if (window->image_path != NULL)
		cdata = comments_load_comment (window->image_path);
	else
		return;

	comment_visible = TRUE;
	set_button_active_no_notify (NULL, prop_button, TRUE);
	
	comment = NULL;
	if (cdata != NULL) {
		comment = comments_get_comment_as_string (cdata, "\n", " - ");
		comment_data_free (cdata);
	}

	file_info = get_file_info (window);

	if (comment == NULL)
		marked_text = g_strdup (file_info); /*g_strdup_printf ("<i>%s</i>", file_info);*/
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
	pango_layout_get_pixel_extents (layout, NULL, &bounds);

	bounds.width += (2 * X_PADDING) + (icon_w + X_PADDING);
	bounds.height += 2 * Y_PADDING;
	bounds.height = MIN (gdk_screen_height () - icon_h - (Y_PADDING * 2), 
			     bounds.height);

	bounds.x = (gdk_screen_width () - bounds.width) / 2;
	bounds.y = gdk_screen_height () - bounds.height - (Y_PADDING * 3);
	bounds.x = MAX (bounds.x, 0);
	bounds.y = MAX (bounds.y, 0);

	text_x = X_PADDING + icon_w + X_PADDING;
	text_y = Y_PADDING;
	icon_x = X_PADDING;
	icon_y = (bounds.height - icon_h) / 2;

	buffer = get_pixmap (GTK_WIDGET (viewer)->window,
			     GTK_WIDGET (viewer)->style->black_gc,
			     &bounds);
	original_buffer = get_pixmap (GTK_WIDGET (viewer)->window,
				      GTK_WIDGET (viewer)->style->black_gc,
				      &bounds);
	render_background (buffer, &bounds);
	render_frame      (buffer, 
			   GTK_WIDGET (viewer)->style->black_gc,
			   &bounds);
	gdk_draw_layout (buffer,
			 GTK_WIDGET (viewer)->style->black_gc,
			 text_x, 
			 text_y,
			 layout);
	gdk_pixbuf_render_to_drawable (icon,
				       buffer,
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
			   buffer,
			   0, 0,
			   bounds.x, bounds.y,
			   bounds.width, bounds.height);

	viewer->next_scroll_repaint = TRUE;
}


static void
hide_comment_on_image ()
{
	GThumbWindow *window = fullscreen->related_win;
	GtkWidget    *viewer = window->viewer;

	IMAGE_VIEWER (viewer)->next_scroll_repaint = FALSE;

	if (original_buffer == NULL)
		return;

	comment_visible = FALSE;
	set_button_active_no_notify (NULL, prop_button, FALSE);

	gdk_draw_drawable (viewer->window,
			   viewer->style->black_gc,
			   original_buffer,
			   0, 0,
			   bounds.x, bounds.y,
			   bounds.width, bounds.height);
	gdk_flush ();
}


int
image_key_press_cb (GtkWidget   *widget, 
		    GdkEventKey *event,
		    gpointer     data)
{
	FullScreen   *fullscreen = data;
	GThumbWindow *window = fullscreen->related_win;
        ImageViewer  *viewer = IMAGE_VIEWER (window->viewer);

	switch (event->keyval) {
		
		/* Exit fullscreen mode. */
	case GDK_Escape:
	case GDK_q:
	case GDK_v:
	case GDK_f:
	case GDK_F11:
		fullscreen_stop (fullscreen);
		break;

		/* Next image. */
	case GDK_n:
	case GDK_space:
	case GDK_Page_Down:
		if (fullscreen->related_win->image_data_visible)
			comment_visible = TRUE;
		window_show_next_image (window, FALSE);
		break;

		/* Previous image. */
	case GDK_p:
	case GDK_b:
	case GDK_BackSpace:
	case GDK_Page_Up:
		if (fullscreen->related_win->image_data_visible)
			comment_visible = TRUE;
		window_show_prev_image (window, FALSE);
		break;

		/* Show first image. */
	case GDK_Home: 
	case GDK_KP_Home:
		if (fullscreen->related_win->image_data_visible)
			comment_visible = TRUE;
		window_show_first_image (window, FALSE);
		break;
		
		/* Show last image. */
	case GDK_End: 
	case GDK_KP_End:
		if (fullscreen->related_win->image_data_visible)
			comment_visible = TRUE;
		window_show_last_image (window, FALSE);
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
		image_viewer_zoom_to_fit (viewer);
		break;

		/* Start/Stop Slideshow. */
	case GDK_s:
		if (! window->slideshow)
			window_start_slideshow (window);
		else
			window_stop_slideshow (window);
		break;

		/* Toggle animation. */
	case GDK_g:
		set_command_state (window, "View_PlayAnimation", ! viewer->play_animation);
		break;

		/* Step animation. */
	case GDK_j:
		view_step_ani_command_impl (NULL, window, NULL);
		break;

		/* Delete selection. */
	case GDK_Delete: 
	case GDK_KP_Delete:
		if (! fullscreen->wm_state_fullscreen_support)
			return FALSE;

		/* show the mouse pointer. */

		if (fullscreen->mouse_hide_id != 0)
			g_source_remove (fullscreen->mouse_hide_id);
		image_viewer_show_cursor (IMAGE_VIEWER (fullscreen->viewer));

		/* Delete. */

		if (window->sidebar_content == GTH_SIDEBAR_DIR_LIST)
			image_delete_command_impl (NULL, window, NULL);
		else if (window->sidebar_content == GTH_SIDEBAR_CATALOG_LIST)
			image_delete_from_catalog_command_impl (NULL, window, NULL);
		break;

		/* Edit comment. */
	case GDK_c: 
		if (! fullscreen->wm_state_fullscreen_support)
			return FALSE;

		if (fullscreen->mouse_hide_id != 0)
			g_source_remove (fullscreen->mouse_hide_id);
		image_viewer_show_cursor (IMAGE_VIEWER (fullscreen->viewer));

		if (comment_visible)
			hide_comment_on_image ();

		edit_current_edit_comment_command_impl (NULL, window, NULL);
		break;

		/* Edit categories. */
	case GDK_k: 
		if (! fullscreen->wm_state_fullscreen_support)
			return FALSE;

		if (fullscreen->mouse_hide_id != 0)
			g_source_remove (fullscreen->mouse_hide_id);
		image_viewer_show_cursor (IMAGE_VIEWER (fullscreen->viewer));

		if (comment_visible)
			hide_comment_on_image ();

		edit_current_edit_categories_command_impl (NULL, window, NULL);
		break;

		/* Flip image. */
	case GDK_l:
	case GDK_L:
		alter_image_flip_command_impl (NULL, window, NULL);
		break;

		/* Rotate. */
	case GDK_r:
	case GDK_R:
	case GDK_bracketright:
		alter_image_rotate_90_command_impl (NULL, window, NULL);
		break;

	case GDK_bracketleft:
		alter_image_rotate_90_cc_command_impl (NULL, window, NULL);
		break;

		/* Mirror. */
	case GDK_m:
	case GDK_M:
		alter_image_mirror_command_impl (NULL, window, NULL);
		break;

		/* View/Hide comment */
	case GDK_i:
		if (comment_visible) {
			window->image_data_visible = FALSE;
			hide_comment_on_image ();
		} else {
			window->image_data_visible = TRUE;
			show_comment_on_image (window, viewer);
		}
		break;

	default:
		if (comment_visible) 
			hide_comment_on_image ();
		return FALSE;
	}

	return TRUE;
}


static gboolean
hide_mouse_pointer_cb (gpointer data)
{
        FullScreen *fullscreen = data;
	int         x, y, w, h, px, py;

	gdk_window_get_pointer (popup_window->window, &px, &py, 0);
	gdk_window_get_geometry (popup_window->window, &x, &y, &w, &h, NULL);

	if ((px >= x) && (px <= x + w) && (py >= y) && (py <= y + h)) 
		return TRUE;

	gtk_widget_hide (popup_window);
	image_viewer_hide_cursor (IMAGE_VIEWER (fullscreen->viewer));
	fullscreen->mouse_hide_id = 0;

	if (fullscreen->related_win->image_data_visible)
		comment_visible = TRUE;
	if (comment_visible) 
		gtk_widget_queue_draw (fullscreen->viewer);

        return FALSE;
}


static gboolean
fs_motion_notify_cb (GtkWidget      *widget, 
		     GdkEventButton *bevent, 
		     gpointer        data)
{
        FullScreen *fullscreen = data;
	static int  last_px = 0, last_py = 0;
	int         px, py;

	gdk_window_get_pointer (widget->window, &px, &py, NULL);

	if (last_px == 0)
		last_px = px;
	if (last_py == 0)
		last_py = py;

	if ((abs (last_px - px) > MOTION_THRESHOLD) 
	    || (abs (last_py - py) > MOTION_THRESHOLD)) 
		if (! GTK_WIDGET_VISIBLE (popup_window)) {
			gtk_widget_show (popup_window);
			image_viewer_show_cursor (IMAGE_VIEWER (fullscreen->viewer));
		}

	if (fullscreen->mouse_hide_id != 0)
		g_source_remove (fullscreen->mouse_hide_id);
	fullscreen->mouse_hide_id = g_timeout_add (HIDE_DELAY,
						   hide_mouse_pointer_cb,
						   fullscreen);

	last_px = px;
	last_py = py;

	return FALSE;
}


static gint
fs_button_press_cb (GtkWidget      *widget, 
		    GdkEventButton *bevent, 
		    gpointer        data)
{
	if (comment_visible) 
		hide_comment_on_image ();
	return FALSE;
}


static gboolean 
fs_expose_event_cb (GtkWidget        *widget, 
		    GdkEventExpose   *event,
		    FullScreen       *fullscreen)
{
	if (comment_visible) {
		GThumbWindow *window = fullscreen->related_win;
		ImageViewer  *viewer = IMAGE_VIEWER (window->viewer);

		comment_visible = FALSE;
		show_comment_on_image (window, viewer);
	}

	return FALSE;
}


static gboolean 
fs_repainted_cb (GtkWidget        *widget, 
		 FullScreen       *fullscreen)
{
	comment_visible = FALSE;
	return TRUE;
}


static void
exit_fs_clicked_cb (GtkWidget *button)
{
	if (current_fullscreen != NULL)
		fullscreen_stop (current_fullscreen);
}


static void
show_prev_image_cb (GtkWidget  *button)
{
	if (current_fullscreen->related_win->image_data_visible)
		comment_visible = TRUE;
	window_show_prev_image (current_fullscreen->related_win, FALSE);
}


static void
show_next_image_cb (GtkWidget  *button)
{
	if (current_fullscreen->related_win->image_data_visible)
		comment_visible = TRUE;
	window_show_next_image (current_fullscreen->related_win, FALSE);
}


static void
image_comment_toggled_cb (GtkToggleButton  *button)
{
	GThumbWindow *window = current_fullscreen->related_win;
        ImageViewer  *viewer = IMAGE_VIEWER (window->viewer);

	if (gtk_toggle_button_get_active (button)) {
		window->image_data_visible = TRUE;
		show_comment_on_image (window, viewer);
	} else {
		window->image_data_visible = FALSE;
		hide_comment_on_image ();
	}
}


static void
zoom_in_cb (GtkWidget  *button)
{
	GThumbWindow *window = current_fullscreen->related_win;
        ImageViewer  *viewer = IMAGE_VIEWER (window->viewer);
	if (window->image_data_visible)
		comment_visible = TRUE;
	image_viewer_zoom_in (viewer);
}


static void
zoom_out_cb (GtkWidget  *button)
{
	GThumbWindow *window = current_fullscreen->related_win;
        ImageViewer  *viewer = IMAGE_VIEWER (window->viewer);
	if (window->image_data_visible)
		comment_visible = TRUE;
	image_viewer_zoom_out (viewer);
}


static void
zoom_100_cb (GtkWidget  *button)
{
	GThumbWindow *window = current_fullscreen->related_win;
        ImageViewer  *viewer = IMAGE_VIEWER (window->viewer);
	if (window->image_data_visible)
		comment_visible = TRUE;
	image_viewer_set_zoom (viewer, 1.0);
}


static void
zoom_fit_cb (GtkWidget  *button)
{
	GThumbWindow *window = current_fullscreen->related_win;
        ImageViewer  *viewer = IMAGE_VIEWER (window->viewer);
	if (window->image_data_visible)
		comment_visible = TRUE;
	image_viewer_zoom_to_fit (viewer);
}


static GtkWidget*
create_button (const char *stock_icon,
	       const char *label,
	       gboolean    toggle)
{
	GtkWidget *button;
	GtkWidget *image;
	GtkWidget *button_box;

	if (toggle)
		button = gtk_toggle_button_new ();
	else
		button = gtk_button_new ();
	if (pref_get_real_toolbar_style() == GTH_TOOLBAR_STYLE_TEXT_BELOW)
		button_box = gtk_vbox_new (FALSE, 5);
	else
		button_box = gtk_hbox_new (FALSE, 5);
	gtk_container_add (GTK_CONTAINER (button), button_box);

	image = gtk_image_new_from_stock (stock_icon, GTK_ICON_SIZE_BUTTON);
	gtk_box_pack_start (GTK_BOX (button_box), image, FALSE, FALSE, 0);

	if (label != NULL)
		gtk_box_pack_start (GTK_BOX (button_box), gtk_label_new (label), FALSE, FALSE, 0);

	return button;
}


static void
create_popup_window (void)
{
	GtkWidget *button;
	GtkWidget *frame;
	GtkWidget *hbox;


	popup_window = gtk_window_new (GTK_WINDOW_POPUP);

	frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);
	gtk_container_set_border_width (GTK_CONTAINER (frame), 1);

	hbox = gtk_hbox_new (FALSE, 6);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 0);
	gtk_container_add (GTK_CONTAINER (frame), hbox);

	/* restore normal view */

	button = create_button (GTHUMB_STOCK_FULLSCREEN, _("Restore Normal View"), FALSE);
	gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
	g_signal_connect (G_OBJECT (button),
			  "clicked",
			  G_CALLBACK (exit_fs_clicked_cb),
			  NULL);

	/* image info */

	prop_button = button = create_button (GTHUMB_STOCK_PROPERTIES, _("Image Info"), TRUE);
	gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
	g_signal_connect (G_OBJECT (button),
			  "toggled",
			  G_CALLBACK (image_comment_toggled_cb),
			  fullscreen);

	/* back */

	button = create_button (GTK_STOCK_GO_BACK, _("Back"), FALSE);
	gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
	g_signal_connect (G_OBJECT (button),
			  "clicked",
			  G_CALLBACK (show_prev_image_cb),
			  fullscreen);

	/* forward */

	button = create_button (GTK_STOCK_GO_FORWARD, _("Forward"), FALSE);
	gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
	g_signal_connect (G_OBJECT (button),
			  "clicked",
			  G_CALLBACK (show_next_image_cb),
			  fullscreen);

	/* zoom in */

	button = create_button (GTK_STOCK_ZOOM_IN, _("In"), FALSE);
	gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
	g_signal_connect (G_OBJECT (button),
			  "clicked",
			  G_CALLBACK (zoom_in_cb),
			  fullscreen);

	/* zoom out */

	button = create_button (GTK_STOCK_ZOOM_OUT, _("Out"), FALSE);
	gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
	g_signal_connect (G_OBJECT (button),
			  "clicked",
			  G_CALLBACK (zoom_out_cb),
			  fullscreen);

	/* zoom 100 */

	button = create_button (GTK_STOCK_ZOOM_100, _("1:1"), FALSE);
	gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
	g_signal_connect (G_OBJECT (button),
			  "clicked",
			  G_CALLBACK (zoom_100_cb),
			  fullscreen);

	/* zoom fit */

	button = create_button (GTK_STOCK_ZOOM_FIT, _("Fit"), FALSE);
	gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
	g_signal_connect (G_OBJECT (button),
			  "clicked",
			  G_CALLBACK (zoom_fit_cb),
			  fullscreen);

	/**/

	gtk_container_add (GTK_CONTAINER (popup_window), frame);

	gtk_widget_show_all (frame);
}


/* Stolen from profterm, Copyright (C) 2001 Havoc Pennington */


static void
wmspec_change_state (gboolean   add,
                     GdkWindow *window,
                     GdkAtom    state1,
                     GdkAtom    state2)
{
  XEvent xev;

#define _NET_WM_STATE_REMOVE        0    /* remove/unset property */
#define _NET_WM_STATE_ADD           1    /* add/set property */
#define _NET_WM_STATE_TOGGLE        2    /* toggle property  */  
  
  xev.xclient.type = ClientMessage;
  xev.xclient.serial = 0;
  xev.xclient.send_event = True;
  xev.xclient.display = gdk_display;
  xev.xclient.window = GDK_WINDOW_XID (window);
  xev.xclient.message_type = gdk_x11_get_xatom_by_name ("_NET_WM_STATE");
  xev.xclient.format = 32;
  xev.xclient.data.l[0] = add ? _NET_WM_STATE_ADD : _NET_WM_STATE_REMOVE;
  xev.xclient.data.l[1] = gdk_x11_atom_to_xatom (state1);
  xev.xclient.data.l[2] = gdk_x11_atom_to_xatom (state2);
  
  XSendEvent (gdk_display, GDK_WINDOW_XID (gdk_get_default_root_window ()),
              False,
              SubstructureRedirectMask | SubstructureNotifyMask,
              &xev);
}


static void
close_fullscreen_window_cb (GtkWidget    *caller, 
			    GdkEvent     *event, 
			    FullScreen *fullscreen)
{
	fullscreen_stop (fullscreen);
}


FullScreen *
fullscreen_new (void)
{
	FullScreen *fullscreen;

	fullscreen = g_new0 (FullScreen, 1);
	fullscreen->motion_id = 0;
	fullscreen->mouse_hide_id = 0;
	fullscreen->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

	g_signal_connect (G_OBJECT (fullscreen->window), 
			  "delete_event",
			  G_CALLBACK (close_fullscreen_window_cb),
			  fullscreen);

	fullscreen->wm_state_fullscreen_support = gdk_net_wm_supports (gdk_atom_intern ("_NET_WM_STATE_FULLSCREEN", FALSE));

	if (! fullscreen->wm_state_fullscreen_support) {
		gtk_widget_destroy (fullscreen->window);
		fullscreen->window = gtk_window_new (GTK_WINDOW_POPUP);
		gtk_window_set_default_size (GTK_WINDOW (fullscreen->window), 
					     gdk_screen_width (), 
					     gdk_screen_height ());
	}

	gtk_window_set_wmclass (GTK_WINDOW (fullscreen->window), "",
				"gthumb_fullscreen");

	fullscreen->related_win = NULL;
	fullscreen->viewer = NULL;

	g_signal_connect (G_OBJECT (fullscreen->window), 
			  "key_press_event",
			  G_CALLBACK (image_key_press_cb), 
			  fullscreen);

	g_signal_connect (G_OBJECT (fullscreen->window),
			  "motion_notify_event",
			  G_CALLBACK (fs_motion_notify_cb),
			  fullscreen);

	if (popup_window == NULL)
		create_popup_window ();

	return fullscreen;
}


void
fullscreen_close (FullScreen *fullscreen)
{
	g_return_if_fail (fullscreen != NULL);

	if (buffer != NULL) {
		g_object_unref (buffer);
		g_object_unref (original_buffer);
		buffer = NULL;
		original_buffer = NULL;
	}
	gtk_widget_destroy (fullscreen->window);
	g_free (fullscreen);
}


void
fullscreen_start (FullScreen   *fullscreen,
		  GThumbWindow *window)
{
	g_return_if_fail (fullscreen != NULL);

	if (fullscreen->related_win != NULL)
		return;

	current_fullscreen = fullscreen;

	gtk_window_set_screen (GTK_WINDOW (fullscreen->window), gtk_widget_get_screen (window->app));
	gtk_window_present (GTK_WINDOW (fullscreen->window));

	window->fullscreen = TRUE;
	fullscreen->related_win = window;
	fullscreen->viewer = window->viewer;

	/* FIXME
	fullscreen->msg_save_modified_image = eel_gconf_get_boolean (PREF_MSG_SAVE_MODIFIED_IMAGE, TRUE);
	eel_gconf_set_boolean (PREF_MSG_SAVE_MODIFIED_IMAGE, FALSE);*/

	wmspec_change_state (TRUE,
			     fullscreen->window->window,
			     gdk_atom_intern ("_NET_WM_STATE_FULLSCREEN", 
					      FALSE),
			     GDK_NONE);

	gtk_widget_reparent (window->viewer, fullscreen->window); 

	if (! eel_gconf_get_boolean (PREF_BLACK_BACKGROUND, FALSE))
		image_viewer_set_black_background (IMAGE_VIEWER (fullscreen->viewer), TRUE);

	gtk_widget_set_sensitive (fullscreen->related_win->app, FALSE);

	/* capture keyboard events. */

	if (! fullscreen->wm_state_fullscreen_support) {
		gdk_keyboard_grab (fullscreen->window->window, TRUE, GDK_CURRENT_TIME);
		gtk_grab_add (fullscreen->window);
		gtk_widget_grab_focus (fullscreen->window);
	}

	/* hide mouse pointer. */

	fullscreen->mouse_hide_id = 0;
	image_viewer_hide_cursor (IMAGE_VIEWER (fullscreen->viewer));

	/**/

	g_signal_connect_after (G_OBJECT (fullscreen->viewer),
				"expose_event",
				G_CALLBACK (fs_expose_event_cb),
				fullscreen);
	g_signal_connect (G_OBJECT (fullscreen->viewer),
			  "button_press_event",
			  G_CALLBACK (fs_button_press_cb),
			  fullscreen);
	g_signal_connect_after (G_OBJECT (fullscreen->viewer),
				"repainted",
				G_CALLBACK (fs_repainted_cb),
				fullscreen);

	if (window->image_data_visible)
		show_comment_on_image (window, IMAGE_VIEWER (fullscreen->viewer));
}


void
fullscreen_stop (FullScreen *fullscreen)
{
	GThumbWindow *window;

	g_return_if_fail (fullscreen != NULL);

	if (fullscreen->related_win == NULL)
		return;

	if (GTK_WIDGET_VISIBLE (popup_window))
		gtk_widget_hide (popup_window);

	current_fullscreen = NULL;

	g_signal_handlers_disconnect_by_data (G_OBJECT (fullscreen->viewer),
					      fullscreen);

	window = fullscreen->related_win;

	/* auto hide mouse pointer staff. */

	image_viewer_show_cursor (IMAGE_VIEWER (fullscreen->viewer));
	if (fullscreen->mouse_hide_id)
		g_source_remove (fullscreen->mouse_hide_id);

	if (! eel_gconf_get_boolean (PREF_BLACK_BACKGROUND, FALSE))
		image_viewer_set_black_background (IMAGE_VIEWER (fullscreen->viewer), FALSE);

	gtk_widget_hide (fullscreen->window);
	wmspec_change_state (FALSE,
			     fullscreen->window->window,
			     gdk_atom_intern ("_NET_WM_STATE_FULLSCREEN", 
					      FALSE),
			     GDK_NONE);

	g_signal_handlers_disconnect_by_func (G_OBJECT (fullscreen->viewer),
					      fs_expose_event_cb,
					      fullscreen);
	g_signal_handlers_disconnect_by_func (G_OBJECT (fullscreen->viewer),
					      fs_button_press_cb,
					      fullscreen);
	g_signal_handlers_disconnect_by_func (G_OBJECT (fullscreen->viewer),
					      fs_repainted_cb,
					      fullscreen);

	window->fullscreen = FALSE;
	fullscreen->related_win = NULL;
	comment_visible = FALSE;

	/* FIXME
	eel_gconf_set_boolean (PREF_MSG_SAVE_MODIFIED_IMAGE, fullscreen->msg_save_modified_image);
	*/

	/* release keyboard focus. */ 

	if (! fullscreen->wm_state_fullscreen_support) {
		gdk_keyboard_ungrab (GDK_CURRENT_TIME);
		gtk_grab_remove (fullscreen->window);
	}

	/* stop the slideshow if the user wants so. */

	if (eel_gconf_get_boolean (PREF_SLIDESHOW_FULLSCREEN, TRUE))
		window_stop_slideshow (window);

	gtk_widget_reparent (fullscreen->viewer, window->viewer_container);
	gtk_widget_realize (window->viewer);
	fullscreen->viewer = NULL;

	gtk_widget_set_sensitive (window->app, TRUE);
	gtk_widget_show_all (window->app);

	/* restore widgets visiblity */

	window_set_preview_content (window, window->preview_content);
	if (! window->image_pane_visible)
		window_hide_image_pane (window);
	if (window->sidebar_visible) 
		window_show_sidebar (window);
	else 
		window_hide_sidebar (window);
}
