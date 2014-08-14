/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2009 Free Software Foundation, Inc.
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

#include <config.h>
#include <math.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdkx.h>
#include <gst/gst.h>
#include <gst/video/videooverlay.h>
#include <gthumb.h>
#include <extensions/gstreamer_utils/gstreamer-utils.h>
#include "actions.h"
#include "gth-media-viewer-page.h"
#include "preferences.h"


#define GET_WIDGET(x) (_gtk_builder_get_widget (self->priv->builder, (x)))
#define PROGRESS_DELAY 500


static void gth_viewer_page_interface_init (GthViewerPageInterface *iface);


G_DEFINE_TYPE_WITH_CODE (GthMediaViewerPage,
			 gth_media_viewer_page,
			 G_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (GTH_TYPE_VIEWER_PAGE,
					 	gth_viewer_page_interface_init))


struct _GthMediaViewerPagePrivate {
	GthBrowser     *browser;
	GthFileData    *file_data;
	GstElement     *playbin;
	GtkBuilder     *builder;
	GtkWidget      *area;
	GtkWidget      *area_box;
	gboolean        visible;
	gboolean        playing;
	gboolean        paused;
	gdouble         last_volume;
	gint64          duration;
	int             video_fps_n;
	int             video_fps_d;
	gboolean        has_video;
	gboolean        has_audio;
	gulong          update_progress_id;
	gulong          update_volume_id;
	gdouble         rate;
	GtkWidget      *mediabar;
	GtkWidget      *mediabar_revealer;
	gulong          video_window_xid;
	gboolean        xwin_assigned;
	GdkPixbuf      *icon;
	PangoLayout    *caption_layout;
	GdkCursor      *cursor;
	GdkCursor      *cursor_void;
	gboolean        cursor_visible;
	GthScreensaver *screensaver;
	GtkWidget      *screenshot_button;
	gboolean        background_painted;
};


static double default_rates[] = { 0.03, 0.06, 0.12, 0.25, 0.33, 0.50, 0.66, 1.0, 1.50, 2.0, 3.0, 4.0, 8.0, 16.0, 32.0 };


static const GActionEntry actions[] = {
	{ "video-screenshot", gth_browser_activate_video_screenshot }
};


static void
_gth_media_viewer_page_update_caption (GthMediaViewerPage *self)
{
	if (self->priv->caption_layout == NULL)
		return;

	if (self->priv->file_data != NULL) {
		GString     *description;
		GthMetadata *metadata;

		description = g_string_new ("");
		metadata = (GthMetadata *) g_file_info_get_attribute_object (self->priv->file_data->info, "general::title");
		if (metadata != NULL) {
			g_string_append (description, gth_metadata_get_formatted (metadata));
			metadata = (GthMetadata *) g_file_info_get_attribute_object (self->priv->file_data->info, "audio-video::general::artist");
			if (metadata != NULL) {
				g_string_append (description, "\n");
				g_string_append (description, gth_metadata_get_formatted (metadata));
			}
		}
		else
			g_string_append (description, g_file_info_get_display_name (self->priv->file_data->info));

		pango_layout_set_text (self->priv->caption_layout, description->str, -1);

		g_string_free (description, TRUE);
	}
	else
		pango_layout_set_text (self->priv->caption_layout, "", -1);

	gtk_widget_queue_draw (GTK_WIDGET (self->priv->area));
}


static void
video_area_realize_cb (GtkWidget *widget,
		       gpointer   user_data)
{
	GthMediaViewerPage *self = user_data;

	self->priv->video_window_xid = GDK_WINDOW_XID (gtk_widget_get_window (widget));
	self->priv->cursor = gdk_cursor_new (GDK_LEFT_PTR);
	self->priv->cursor_void = gdk_cursor_new_for_display (gtk_widget_get_display (self->priv->area), GDK_BLANK_CURSOR);
	if (self->priv->cursor_visible)
		gdk_window_set_cursor (gtk_widget_get_window (self->priv->area), self->priv->cursor);
	else
		gdk_window_set_cursor (gtk_widget_get_window (self->priv->area), self->priv->cursor_void);

	self->priv->caption_layout = gtk_widget_create_pango_layout (widget, "");
	pango_layout_set_alignment (self->priv->caption_layout, PANGO_ALIGN_CENTER);
	_gth_media_viewer_page_update_caption (self);

	self->priv->background_painted = FALSE;
}


static void
video_area_unrealize_cb (GtkWidget *widget,
			 gpointer   user_data)
{
	GthMediaViewerPage *self = user_data;

	self->priv->video_window_xid = 0;
	self->priv->xwin_assigned = FALSE;

	if (self->priv->cursor) {
		g_object_unref (self->priv->cursor);
		self->priv->cursor = NULL;
	}

	if (self->priv->cursor_void) {
		g_object_unref (self->priv->cursor_void);
		self->priv->cursor_void = NULL;
	}

	g_object_unref (self->priv->caption_layout);
	self->priv->caption_layout = NULL;
}


static gboolean
video_area_draw_cb (GtkWidget *widget,
		    cairo_t   *cr,
		    gpointer   user_data)
{
	GthMediaViewerPage *self = user_data;
	GtkAllocation       allocation;
	GtkStyleContext    *style_context;

	if (self->priv->xwin_assigned && self->priv->has_video && self->priv->background_painted)
		return FALSE;

	gtk_widget_get_allocation (widget, &allocation);
	style_context = gtk_widget_get_style_context (widget);

	if (self->priv->icon == NULL) {
		char  *type;
		GIcon *icon;
		int    size;

		type = NULL;
		if (self->priv->file_data != NULL)
			type = g_content_type_from_mime_type (gth_file_data_get_mime_type (self->priv->file_data));
		if (type == NULL)
			type = g_content_type_from_mime_type ("text/plain");
		icon = g_content_type_get_icon (type);
		size = allocation.width;
		if (size > allocation.height)
			size = allocation.height;
		size = size / 3;
		self->priv->icon = _g_icon_get_pixbuf (icon, size, _gtk_widget_get_icon_theme (widget));

		g_object_unref (icon);
		g_free (type);
	}

	cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
	cairo_rectangle (cr, 0, 0, allocation.width, allocation.height);
	cairo_fill (cr);

	if (self->priv->icon != NULL) {
		int                   icon_w, icon_h;
		int                   text_w;
		int                   icon_x, icon_y;
		PangoRectangle        logical_rect;
		int                   x, y;
		PangoFontDescription *font;

		icon_w = gdk_pixbuf_get_width (self->priv->icon);
		icon_h = gdk_pixbuf_get_height (self->priv->icon);

		text_w = (icon_w * 3 / 2);
		pango_layout_set_width (self->priv->caption_layout, PANGO_SCALE * text_w);
		pango_layout_get_extents (self->priv->caption_layout, NULL, &logical_rect);

		icon_x = (allocation.width - icon_w) / 2;
		x = (allocation.width - text_w) / 2;

		icon_y = (allocation.height - (icon_h + PANGO_PIXELS (logical_rect.height))) / 2;
		y = icon_y + icon_h;

		gdk_cairo_set_source_pixbuf (cr, self->priv->icon, icon_x, icon_y);
		cairo_rectangle (cr, icon_x, icon_y, icon_w, icon_h);
		cairo_fill (cr);

		cairo_move_to (cr, x, y);
		gtk_style_context_get (style_context, gtk_widget_get_state_flags (widget), "font", &font, NULL);
		pango_layout_set_font_description (self->priv->caption_layout, font);
		pango_cairo_layout_path (cr, self->priv->caption_layout);
		cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
		cairo_fill (cr);
	}

	self->priv->background_painted = TRUE;

	return TRUE;
}


static gboolean
video_area_button_press_cb (GtkWidget          *widget,
			    GdkEventButton     *event,
			    GthMediaViewerPage *self)
{
	if ((event->type == GDK_BUTTON_PRESS) && (event->button == 1) ) {
		gtk_button_clicked (GTK_BUTTON (GET_WIDGET ("play_button")));
		return TRUE;
	}

	return gth_browser_viewer_button_press_cb (self->priv->browser, event);
}


static gboolean
video_area_popup_menu_cb (GtkWidget          *widget,
			  GthMediaViewerPage *self)
{
	gth_browser_file_menu_popup (self->priv->browser, NULL);
	return TRUE;
}


static gboolean
video_area_scroll_event_cb (GtkWidget 	       *widget,
			    GdkEventScroll     *event,
			    GthMediaViewerPage *self)
{
	return gth_browser_viewer_scroll_event_cb (self->priv->browser, event);
}


static gboolean
video_area_key_press_cb (GtkWidget          *widget,
			 GdkEventKey        *event,
			 GthMediaViewerPage *self)
{
	return gth_browser_viewer_key_press_cb (self->priv->browser, event);
}


static void
volume_value_changed_cb (GtkAdjustment *adjustment,
			 gpointer       user_data)
{
	GthMediaViewerPage *self = user_data;

	if (self->priv->playbin != NULL)
		g_object_set (self->priv->playbin,
			      "volume",
			      gtk_adjustment_get_value (adjustment) / 100.0,
			      NULL);
}


static void position_value_changed_cb (GtkAdjustment *adjustment,
				       gpointer       user_data);


static void
update_current_position_bar (GthMediaViewerPage *self)
{
	GstFormat format;
        gint64    current_value = 0;

        format = GST_FORMAT_TIME;
        if (gst_element_query_position (self->priv->playbin, format, &current_value)) {
        	char *s;

		if (self->priv->duration <= 0) {
			gst_element_query_duration (self->priv->playbin, format, &self->priv->duration);
			s = _g_format_duration_for_display (GST_TIME_AS_MSECONDS (self->priv->duration));
			gtk_label_set_text (GTK_LABEL (GET_WIDGET ("label_duration")), s);

			g_free (s);
		}

		/*
		g_print ("==> %" G_GINT64_FORMAT " / %" G_GINT64_FORMAT " (%0.3g)\n" ,
			 current_value,
			 self->priv->duration,
			 ((double) current_value / self->priv->duration) * 100.0);
		*/

		g_signal_handlers_block_by_func(GET_WIDGET ("position_adjustment"), position_value_changed_cb, self);
		gtk_adjustment_set_value (GTK_ADJUSTMENT (GET_WIDGET ("position_adjustment")), (self->priv->duration > 0) ? ((double) current_value / self->priv->duration) * 100.0 : 0.0);
		g_signal_handlers_unblock_by_func(GET_WIDGET ("position_adjustment"), position_value_changed_cb, self);

		s = _g_format_duration_for_display (GST_TIME_AS_MSECONDS (current_value));
		gtk_label_set_text (GTK_LABEL (GET_WIDGET ("label_position")), s);

		g_free (s);
        }
}


static void
position_value_changed_cb (GtkAdjustment *adjustment,
			   gpointer       user_data)
{
	GthMediaViewerPage *self = user_data;
	gint64              current_value;
	char               *s;

	if (self->priv->playbin == NULL)
		return;

	current_value = (gint64) (gtk_adjustment_get_value (adjustment) / 100.0 * self->priv->duration);
	gst_element_seek (self->priv->playbin,
			  self->priv->rate,
			  GST_FORMAT_TIME,
			  GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT,
			  GST_SEEK_TYPE_SET,
			  current_value,
			  GST_SEEK_TYPE_NONE,
			  0.0);

	s = _g_format_duration_for_display (GST_TIME_AS_MSECONDS (current_value));
	gtk_label_set_text (GTK_LABEL (GET_WIDGET ("label_position")), s);

	g_free (s);
}


static void
update_playback_info (GthMediaViewerPage *self)
{
	char *playback_info;

	playback_info = g_strdup_printf ("@%2.2f", self->priv->rate);
	g_file_info_set_attribute_string (gth_browser_get_current_file (self->priv->browser)->info, "gthumb::statusbar-extra-info", playback_info);
	gth_browser_update_statusbar_file_info (self->priv->browser);

	g_free (playback_info);
}


static void
update_player_rate (GthMediaViewerPage *self)
{
	gint64 current_value;

	self->priv->rate = CLAMP (self->priv->rate,
				  default_rates[0],
				  default_rates[G_N_ELEMENTS (default_rates) - 1]);

	if (self->priv->playbin == NULL)
		return;

	update_playback_info (self);

	if (! self->priv->playing)
		return;

	current_value = (gint64) (gtk_adjustment_get_value (GTK_ADJUSTMENT (GET_WIDGET ("position_adjustment"))) / 100.0 * self->priv->duration);
	if (! gst_element_seek (self->priv->playbin,
				self->priv->rate,
			        GST_FORMAT_TIME,
			        GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE,
			        GST_SEEK_TYPE_SET,
			        current_value,
			        GST_SEEK_TYPE_NONE,
			        0.0))
	{
		g_warning ("seek failed");
	}
}


static void
play_button_clicked_cb (GtkButton *button,
			gpointer   user_data)
{
	GthMediaViewerPage *self = user_data;

	if (self->priv->playbin == NULL)
		return;
	if (! self->priv->playing) {
		if (! self->priv->paused) {
			gst_element_set_state (self->priv->playbin, GST_STATE_PAUSED);
			gst_element_seek (self->priv->playbin,
					  self->priv->rate,
					  GST_FORMAT_TIME,
					  GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE,
					  GST_SEEK_TYPE_SET,
					  0.0,
					  GST_SEEK_TYPE_NONE,
					  0.0);
		}
		else {
			gint64 current_value;

			current_value = (gint64) (gtk_adjustment_get_value (GTK_ADJUSTMENT (GET_WIDGET ("position_adjustment"))) / 100.0 * self->priv->duration);
			gst_element_seek (self->priv->playbin,
					  self->priv->rate,
					  GST_FORMAT_TIME,
					  GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE,
					  GST_SEEK_TYPE_SET,
					  current_value,
					  GST_SEEK_TYPE_NONE,
					  0.0);
		}
		gst_element_set_state (self->priv->playbin, GST_STATE_PLAYING);
	}
	else
		gst_element_set_state (self->priv->playbin, GST_STATE_PAUSED);
}


static int
get_nearest_rate (double rate)
{
	int    min_idx = -1;
	double min_delta = 0;
	int    i;

	for (i = 0; i < G_N_ELEMENTS (default_rates); i++) {
		double delta;

		delta = fabs (default_rates[i] - rate);
		if ((i == 0) || (delta < min_delta)) {
			min_delta = delta;
			min_idx = i;
		}
	}

	return min_idx;
}


static void
play_slower_button_clicked_cb (GtkButton *button,
			       gpointer   user_data)
{
	GthMediaViewerPage *self = user_data;
	int                 i;

	i = get_nearest_rate (self->priv->rate);
	if (i > 0)
		self->priv->rate = default_rates[i - 1];
	else
		self->priv->rate = default_rates[0];

	update_player_rate (self);
}


static void
play_faster_button_clicked_cb (GtkButton *button,
			       gpointer   user_data)
{
	GthMediaViewerPage *self = user_data;
	int                 i;

	i = get_nearest_rate (self->priv->rate);
	if (i < G_N_ELEMENTS (default_rates) - 1)
		self->priv->rate = default_rates[i + 1];
	else
		self->priv->rate = default_rates[G_N_ELEMENTS (default_rates) - 1];

	update_player_rate (self);
}


static gboolean
update_volume_from_playbin (GthMediaViewerPage *self)
{
	double volume;

	if (self->priv->update_volume_id != 0) {
		g_source_remove (self->priv->update_volume_id);
		self->priv->update_volume_id = 0;
	}

	if ((self->priv->builder == NULL) || (self->priv->playbin == NULL))
		return FALSE;

	g_object_get (self->priv->playbin, "volume", &volume, NULL);

	g_signal_handlers_block_by_func (GET_WIDGET ("volume_adjustment"), volume_value_changed_cb, self);
	gtk_adjustment_set_value (GTK_ADJUSTMENT (GET_WIDGET ("volume_adjustment")), volume * 100.0);
	g_signal_handlers_unblock_by_func (GET_WIDGET ("volume_adjustment"), volume_value_changed_cb, self);

	return FALSE;
}


static gboolean
update_progress_cb (gpointer user_data)
{
	GthMediaViewerPage *self = user_data;

        if (self->priv->update_progress_id != 0) {
                g_source_remove (self->priv->update_progress_id);
                self->priv->update_progress_id = 0;
        }

        update_current_position_bar (self);

        self->priv->update_progress_id = gdk_threads_add_timeout (PROGRESS_DELAY, update_progress_cb, self);

        return FALSE;
}


static void
set_playing_state (GthMediaViewerPage *self,
	           gboolean            playing)
{
	self->priv->playing = playing;
	if (self->priv->playing)
		gth_screensaver_inhibit (self->priv->screensaver,
					 GTK_WINDOW (self->priv->browser),
					 _("Playing video"));
	else
		gth_screensaver_uninhibit (self->priv->screensaver);
}


static void
update_play_button (GthMediaViewerPage *self,
		    GstState            new_state)
{
	if (! self->priv->playing && (new_state == GST_STATE_PLAYING)) {
		set_playing_state (self, TRUE);
		gtk_image_set_from_icon_name (GTK_IMAGE (GET_WIDGET ("play_button_image")), "media-playback-pause-symbolic", GTK_ICON_SIZE_LARGE_TOOLBAR);
		gtk_widget_set_tooltip_text (GET_WIDGET ("play_button_image"), _("Pause"));

		if (self->priv->update_progress_id == 0)
			self->priv->update_progress_id = gdk_threads_add_timeout (PROGRESS_DELAY, update_progress_cb, self);

		update_playback_info (self);
	}
	else if (self->priv->playing && (new_state != GST_STATE_PLAYING)) {
		GtkWidget *play_button = GET_WIDGET ("play_button_image");
		gboolean   rtl;

		rtl = gtk_widget_get_direction (play_button) == GTK_TEXT_DIR_RTL;

		set_playing_state (self, FALSE);
		gtk_image_set_from_icon_name (GTK_IMAGE (play_button),
					      rtl ? "media-playback-start-rtl-symbolic" : "media-playback-start-symbolic",
					      GTK_ICON_SIZE_LARGE_TOOLBAR);
		gtk_widget_set_tooltip_text (GET_WIDGET ("play_button_image"), _("Play"));

		if (self->priv->update_progress_id != 0) {
			 g_source_remove (self->priv->update_progress_id);
			 self->priv->update_progress_id = 0;
		}

		update_playback_info (self);
	}

	gth_viewer_page_update_sensitivity (GTH_VIEWER_PAGE (self));
}


static void
gth_media_viewer_page_real_activate (GthViewerPage *base,
				     GthBrowser    *browser)
{
	GthMediaViewerPage *self;
	gboolean            rtl;

	if (! gstreamer_init ())
		return;

	self = (GthMediaViewerPage*) base;

	self->priv->browser = browser;
	g_action_map_add_action_entries (G_ACTION_MAP (browser),
					 actions,
					 G_N_ELEMENTS (actions),
					 browser);
	self->priv->screenshot_button =
			gth_browser_add_header_bar_button (browser,
							   GTH_BROWSER_HEADER_SECTION_VIEWER_VIEW,
							   "camera-photo-symbolic",
							   _("Take a screenshot"),
							   "win.video-screenshot",
							   NULL);

	rtl = gtk_widget_get_default_direction () == GTK_TEXT_DIR_RTL;

	/* video area */

	self->priv->area = gtk_drawing_area_new ();

	gtk_style_context_add_class (gtk_widget_get_style_context (self->priv->area), "video-player");

	/* do not use the rgba visual on the drawing area */
	{
	        GdkVisual *visual;
	        visual = gdk_screen_get_system_visual (gtk_widget_get_screen (GTK_WIDGET (self->priv->area)));
        	if (visual != NULL)
                	gtk_widget_set_visual (GTK_WIDGET (self->priv->area), visual);
	}

	gtk_widget_set_double_buffered (self->priv->area, FALSE);
	gtk_widget_add_events (self->priv->area, (gtk_widget_get_events (self->priv->area)
						  | GDK_EXPOSURE_MASK
						  | GDK_BUTTON_PRESS_MASK
						  | GDK_BUTTON_RELEASE_MASK
						  | GDK_POINTER_MOTION_MASK
						  | GDK_POINTER_MOTION_HINT_MASK
						  | GDK_BUTTON_MOTION_MASK
						  | GDK_SCROLL_MASK));
	gtk_widget_set_can_focus (self->priv->area, TRUE);
	gtk_widget_show (self->priv->area);

	g_signal_connect (G_OBJECT (self->priv->area),
			  "realize",
			  G_CALLBACK (video_area_realize_cb),
			  self);
	g_signal_connect (G_OBJECT (self->priv->area),
			  "unrealize",
			  G_CALLBACK (video_area_unrealize_cb),
			  self);
	g_signal_connect (G_OBJECT (self->priv->area),
			  "draw",
			  G_CALLBACK (video_area_draw_cb),
			  self);
	g_signal_connect (G_OBJECT (self->priv->area),
			  "button_press_event",
			  G_CALLBACK (video_area_button_press_cb),
			  self);
	g_signal_connect (G_OBJECT (self->priv->area),
			  "popup-menu",
			  G_CALLBACK (video_area_popup_menu_cb),
			  self);
	g_signal_connect (G_OBJECT (self->priv->area),
			  "scroll_event",
			  G_CALLBACK (video_area_scroll_event_cb),
			  self);
	g_signal_connect (G_OBJECT (self->priv->area),
			  "key_press_event",
			  G_CALLBACK (video_area_key_press_cb),
			  self);

	/* mediabar */

	self->priv->builder = _gtk_builder_new_from_file ("mediabar.ui", "gstreamer_tools");
	self->priv->mediabar = GET_WIDGET ("mediabar");
	gtk_widget_set_halign (self->priv->mediabar, GTK_ALIGN_FILL);
	gtk_widget_set_valign (self->priv->mediabar, GTK_ALIGN_END);

	gtk_image_set_from_icon_name (GTK_IMAGE (GET_WIDGET ("play_slower_image")),
				      rtl ? "media-seek-backward-rtl-symbolic" : "media-seek-backward-symbolic",
				      GTK_ICON_SIZE_MENU);
	gtk_image_set_from_icon_name (GTK_IMAGE (GET_WIDGET ("play_faster_image")),
				      rtl ? "media-seek-forward-rtl-symbolic" : "media-seek-forward-symbolic",
				      GTK_ICON_SIZE_MENU);

	g_signal_connect (GET_WIDGET ("volume_adjustment"),
			  "value-changed",
			  G_CALLBACK (volume_value_changed_cb),
			  self);
	g_signal_connect (GET_WIDGET ("position_adjustment"),
			  "value-changed",
			  G_CALLBACK (position_value_changed_cb),
			  self);
	g_signal_connect (GET_WIDGET ("play_button"),
			  "clicked",
			  G_CALLBACK (play_button_clicked_cb),
			  self);
	g_signal_connect (GET_WIDGET ("play_slower_button"),
			  "clicked",
			  G_CALLBACK (play_slower_button_clicked_cb),
			  self);
	g_signal_connect (GET_WIDGET ("play_faster_button"),
			  "clicked",
			  G_CALLBACK (play_faster_button_clicked_cb),
			  self);

	self->priv->mediabar_revealer = gtk_revealer_new ();
	gtk_revealer_set_transition_type (GTK_REVEALER (self->priv->mediabar_revealer), GTK_REVEALER_TRANSITION_TYPE_SLIDE_UP);
	gtk_widget_set_halign (self->priv->mediabar_revealer, GTK_ALIGN_FILL);
	gtk_widget_set_valign (self->priv->mediabar_revealer, GTK_ALIGN_END);
	gtk_widget_show (self->priv->mediabar_revealer);
	gtk_container_add (GTK_CONTAINER (self->priv->mediabar_revealer), self->priv->mediabar);

	self->priv->area_box = gtk_overlay_new ();
	gtk_container_add (GTK_CONTAINER (self->priv->area_box), self->priv->area);
	gtk_overlay_add_overlay (GTK_OVERLAY (self->priv->area_box), self->priv->mediabar_revealer);
	gtk_widget_show (self->priv->area_box);
	gth_browser_set_viewer_widget (browser, self->priv->area_box);

	gtk_widget_realize (self->priv->area);
	gth_browser_register_viewer_control (self->priv->browser, self->priv->mediabar_revealer);
	gth_browser_register_viewer_control (self->priv->browser, gtk_scale_button_get_popup (GTK_SCALE_BUTTON (GET_WIDGET ("volumebutton"))));

	gth_viewer_page_focus (GTH_VIEWER_PAGE (self));
}


static void
save_volume (GthMediaViewerPage *self)
{
	GSettings *settings;
	double     volume;

	settings = g_settings_new (GTHUMB_GSTREAMER_TOOLS_SCHEMA);
	g_object_get (self->priv->playbin, "volume", &volume, NULL);
	g_settings_set_int (settings, PREF_GSTREAMER_TOOLS_VOLUME, (int) (volume * 100.0));

	g_object_unref (settings);
}


static void
gth_media_viewer_page_real_deactivate (GthViewerPage *base)
{
	GthMediaViewerPage *self;

	self = (GthMediaViewerPage*) base;

	gth_browser_unregister_viewer_control (self->priv->browser, gtk_scale_button_get_popup (GTK_SCALE_BUTTON (GET_WIDGET ("volumebutton"))));
	gth_browser_unregister_viewer_control (self->priv->browser, self->priv->mediabar_revealer);

	if (self->priv->builder != NULL) {
		g_object_unref (self->priv->builder);
		self->priv->builder = NULL;
	}

        if (self->priv->update_progress_id != 0) {
                g_source_remove (self->priv->update_progress_id);
                self->priv->update_progress_id = 0;
        }

        if (self->priv->update_volume_id != 0) {
                g_source_remove (self->priv->update_volume_id);
                self->priv->update_volume_id = 0;
        }

	if (self->priv->playbin != NULL) {
		save_volume (self);
		gst_element_set_state (self->priv->playbin, GST_STATE_NULL);
		gst_object_unref (GST_OBJECT (self->priv->playbin));
		self->priv->playbin = NULL;
	}

	gtk_widget_destroy (self->priv->screenshot_button);
	self->priv->screenshot_button = NULL;

	gth_browser_set_viewer_widget (self->priv->browser, NULL);
}


static GstBusSyncReply
set_playbin_window (GstBus             *bus,
		    GstMessage         *message,
		    GthMediaViewerPage *self)
{
	/* ignore anything but 'prepare-xwindow-id' element messages */
	if (! gst_is_video_overlay_prepare_window_handle_message (message))
		return GST_BUS_PASS;

	if (self->priv->video_window_xid != 0) {
		GstVideoOverlay *video_overlay;

		video_overlay = GST_VIDEO_OVERLAY (GST_MESSAGE_SRC (message));
		gst_video_overlay_set_window_handle (video_overlay, self->priv->video_window_xid);
		self->priv->xwin_assigned = TRUE;
	}
	else
		g_warning ("Should have obtained video_window_xid by now!");

	gst_message_unref (message);

	return GST_BUS_DROP;
}


static void
reset_player_state (GthMediaViewerPage *self)
{
        if (self->priv->update_progress_id != 0) {
                g_source_remove (self->priv->update_progress_id);
                self->priv->update_progress_id = 0;
        }

	update_play_button (self, GST_STATE_NULL);
	self->priv->rate = 1.0;
	set_playing_state (self, FALSE);
}


static void
update_stream_info (GthMediaViewerPage *self)
{
	GstElement *audio_sink;
	GstElement *video_sink;
	GstPad     *audio_pad;
	GstPad     *video_pad;

	g_object_get (self->priv->playbin,
		      "audio-sink", &audio_sink,
		      "video-sink", &video_sink,
		      NULL);

	if (audio_sink != NULL) {
		audio_pad = gst_element_get_static_pad (GST_ELEMENT (audio_sink), "sink");
		if (audio_pad != NULL)
			self->priv->has_audio = TRUE;
	}

	if (video_sink != NULL) {
		video_pad = gst_element_get_static_pad (GST_ELEMENT (video_sink), "sink");
		if (video_pad != NULL) {
			GstCaps *caps;

			self->priv->has_video = TRUE;

			if ((caps = gst_pad_get_current_caps (video_pad)) != NULL) {
				GstStructure *structure;
				int           video_width;
				int           video_height;

				structure = gst_caps_get_structure (caps, 0);
				gst_structure_get_fraction (structure, "framerate", &self->priv->video_fps_n, &self->priv->video_fps_d);
				gst_structure_get_int (structure, "width", &video_width);
				gst_structure_get_int (structure, "height", &video_height);

				g_file_info_set_attribute_int32 (self->priv->file_data->info, "frame::width", video_width);
				g_file_info_set_attribute_int32 (self->priv->file_data->info, "frame::height", video_height);

				gst_caps_unref (caps);
			}
		}
	}
}


static void
bus_message_cb (GstBus     *bus,
                GstMessage *message,
                gpointer    user_data)
{
	GthMediaViewerPage *self = user_data;

	if (GST_MESSAGE_SRC (message) != GST_OBJECT (self->priv->playbin))
		return;

	switch (GST_MESSAGE_TYPE (message)) {
	case GST_MESSAGE_STATE_CHANGED: {
		GstState old_state;
		GstState new_state;
		GstState pending_state;

		old_state = new_state = GST_STATE_NULL;
		gst_message_parse_state_changed (message, &old_state, &new_state, &pending_state);
		if (old_state == new_state)
			break;

		self->priv->paused = (new_state == GST_STATE_PAUSED);
		update_current_position_bar (self);

		if ((old_state == GST_STATE_NULL) && (new_state == GST_STATE_READY) && (pending_state != GST_STATE_PAUSED)) {
			update_stream_info (self);
			gth_viewer_page_update_sensitivity (GTH_VIEWER_PAGE (self));
			gth_viewer_page_file_loaded (GTH_VIEWER_PAGE (self), self->priv->file_data, TRUE);
		}
		if ((old_state == GST_STATE_READY) && (new_state == GST_STATE_PAUSED)) {
			update_stream_info (self);
			gth_viewer_page_update_sensitivity (GTH_VIEWER_PAGE (self));
			gth_viewer_page_file_loaded (GTH_VIEWER_PAGE (self), self->priv->file_data, TRUE);
		}
		if ((old_state == GST_STATE_READY) || (new_state == GST_STATE_PAUSED))
			update_volume_from_playbin (self);
		if ((old_state == GST_STATE_PLAYING) || (new_state == GST_STATE_PLAYING))
			update_play_button (self, new_state);
		break;
	}

	case GST_MESSAGE_DURATION_CHANGED:
		self->priv->duration = 0;
		update_current_position_bar (self);
		break;

	case GST_MESSAGE_EOS:
		reset_player_state (self);
		break;

	case GST_MESSAGE_BUFFERING: {
		int percent = 0;
		gst_message_parse_buffering (message, &percent);
		gst_element_set_state (self->priv->playbin, (percent == 100) ? GST_STATE_PLAYING : GST_STATE_PAUSED);
		break;
	}

	case GST_MESSAGE_ERROR:
		gth_viewer_page_file_loaded (GTH_VIEWER_PAGE (self), self->priv->file_data, FALSE);
		break;

	default:
		break;
	}
}


static void
playbin_notify_volume_cb (GObject    *playbin,
			  GParamSpec *pspec,
			  gpointer    user_data)
{
	GthMediaViewerPage *self = user_data;

	if (self->priv->update_volume_id == 0)
		self->priv->update_volume_id = g_idle_add ((GSourceFunc) update_volume_from_playbin, user_data);
}


static void
create_playbin (GthMediaViewerPage *self)
{
	GSettings *settings;
	GstBus    *bus;

	if (self->priv->playbin != NULL)
		return;

	self->priv->playbin = gst_element_factory_make ("playbin", "playbin");

	settings = g_settings_new (GTHUMB_GSTREAMER_TOOLS_SCHEMA);
	g_object_set (self->priv->playbin,
		      "volume", (double) g_settings_get_int (settings, PREF_GSTREAMER_TOOLS_VOLUME) / 100.0,
		      "force-aspect-ratio", TRUE,
		      NULL);
	g_object_unref (settings);

	bus = gst_pipeline_get_bus (GST_PIPELINE (self->priv->playbin));
	gst_bus_enable_sync_message_emission (bus);
	gst_bus_set_sync_handler (bus, (GstBusSyncHandler) set_playbin_window, self, NULL);
	gst_bus_add_signal_watch (bus);

	g_signal_connect (self->priv->playbin,
			  "notify::volume",
			  G_CALLBACK (playbin_notify_volume_cb),
			  self);
	g_signal_connect (bus,
			  "message",
			  G_CALLBACK (bus_message_cb),
			  self);
}


static void
_gth_media_viewer_page_set_uri (GthMediaViewerPage *self,
				const char         *uri,
				GstState            state)
{
	g_return_if_fail (self->priv->playbin != NULL);

	gst_element_set_state (self->priv->playbin, GST_STATE_NULL);

	g_object_set (G_OBJECT (self->priv->playbin), "uri", uri, NULL);
	gst_element_set_state (self->priv->playbin, state);
	gst_element_get_state (self->priv->playbin, NULL, NULL, GST_CLOCK_TIME_NONE);
}


static void
gth_media_viewer_page_real_show (GthViewerPage *base)
{
	GthMediaViewerPage *self = GTH_MEDIA_VIEWER_PAGE (base);

	self->priv->visible = TRUE;
	self->priv->background_painted = FALSE;
	gth_viewer_page_focus (GTH_VIEWER_PAGE (self));

	create_playbin (self);
	if (self->priv->file_data != NULL) {
		char *uri;

		uri = g_file_get_uri (self->priv->file_data->file);
		_gth_media_viewer_page_set_uri (self, uri, GST_STATE_PLAYING);

		g_free (uri);
	}
}


static void
gth_media_viewer_page_real_hide (GthViewerPage *base)
{
	GthMediaViewerPage *self;

	self = (GthMediaViewerPage*) base;

	self->priv->visible = FALSE;
	if ((self->priv->playbin != NULL) && self->priv->playing)
		gst_element_set_state (self->priv->playbin, GST_STATE_PAUSED);
}


static gboolean
gth_media_viewer_page_real_can_view (GthViewerPage *base,
				     GthFileData   *file_data)
{
	g_return_val_if_fail (file_data != NULL, FALSE);

	return _g_mime_type_is_video (gth_file_data_get_mime_type (file_data)) || _g_mime_type_is_audio (gth_file_data_get_mime_type (file_data));
}


static void
gth_media_viewer_page_real_view (GthViewerPage *base,
				 GthFileData   *file_data)
{
	GthMediaViewerPage *self;
	char               *uri;

	self = (GthMediaViewerPage*) base;
	g_return_if_fail (file_data != NULL);

	if (! gstreamer_init ())
		return;

	gth_viewer_page_focus (GTH_VIEWER_PAGE (self));

	if ((self->priv->file_data != NULL)
	    && g_file_equal (file_data->file, self->priv->file_data->file)
	    && (gth_file_data_get_mtime (file_data) == gth_file_data_get_mtime (self->priv->file_data)))
	{
		return;
	}

	/**/

	_g_object_unref (self->priv->file_data);
	self->priv->file_data = gth_file_data_dup (file_data);

	self->priv->duration = 0;
	self->priv->has_audio = FALSE;
	self->priv->has_video = FALSE;
	self->priv->background_painted = FALSE;

	_g_object_unref (self->priv->icon);
	self->priv->icon = NULL;

	_gth_media_viewer_page_update_caption (self);

	/**/

	g_signal_handlers_block_by_func(GET_WIDGET ("position_adjustment"), position_value_changed_cb, self);
	gtk_adjustment_set_value (GTK_ADJUSTMENT (GET_WIDGET ("position_adjustment")), 0.0);
	g_signal_handlers_unblock_by_func(GET_WIDGET ("position_adjustment"), position_value_changed_cb, self);
	reset_player_state (self);

	create_playbin (self);
	if (self->priv->playbin == NULL)
		return;

	uri = g_file_get_uri (self->priv->file_data->file);
	_gth_media_viewer_page_set_uri (self, uri, self->priv->visible ? GST_STATE_PLAYING : GST_STATE_PAUSED);

	g_free (uri);
}


static void
gth_media_viewer_page_real_focus (GthViewerPage *base)
{
	GtkWidget *widget;

	widget = GTH_MEDIA_VIEWER_PAGE (base)->priv->area;
	if (gtk_widget_get_realized (widget) && gtk_widget_get_mapped (widget))
		gtk_widget_grab_focus (widget);
}


static void
gth_media_viewer_page_real_fullscreen (GthViewerPage *base,
				       gboolean       active)
{
	/* void */
}


static void
gth_media_viewer_page_real_show_pointer (GthViewerPage *base,
				         gboolean       show)
{
	GthMediaViewerPage *self = (GthMediaViewerPage*) base;

	if (show == self->priv->cursor_visible)
		return;

	self->priv->cursor_visible = show;

	if (show && (self->priv->cursor != NULL))
		gdk_window_set_cursor (gtk_widget_get_window (self->priv->area), self->priv->cursor);

	if (! show && gth_browser_get_is_fullscreen (self->priv->browser) && (self->priv->cursor_void != NULL))
		gdk_window_set_cursor (gtk_widget_get_window (self->priv->area), self->priv->cursor_void);

	gtk_revealer_set_reveal_child (GTK_REVEALER (self->priv->mediabar_revealer), show);
}


static void
gth_media_viewer_page_real_update_sensitivity (GthViewerPage *base)
{
	GthMediaViewerPage *self = (GthMediaViewerPage *) base;

	gtk_widget_set_sensitive (GET_WIDGET ("volume_box"), self->priv->has_audio);
	gth_window_enable_action (GTH_WINDOW (self->priv->browser), "video-screenshot", self->priv->has_video);
}


static gboolean
gth_media_viewer_page_real_can_save (GthViewerPage *base)
{
	return FALSE;
}


static void
gth_media_viewer_page_real_save (GthViewerPage *base,
				 GFile         *file,
				 FileSavedFunc  func,
				 gpointer       user_data)
{
	/* void */
}


static void
gth_media_viewer_page_real_save_as (GthViewerPage *base,
				    FileSavedFunc  func,
				    gpointer       user_data)
{
	/* void */
}


static void
gth_media_viewer_page_real_revert (GthViewerPage *base)
{
	/* void */
}


static void
gth_media_viewer_page_real_update_info (GthViewerPage *base,
				        GthFileData   *file_data)
{
	GthMediaViewerPage *self = GTH_MEDIA_VIEWER_PAGE (base);

	if (! _g_file_equal (self->priv->file_data->file, file_data->file))
		return;
	_g_object_unref (self->priv->file_data);
	self->priv->file_data = gth_file_data_dup (file_data);
}


static void
gth_media_viewer_page_finalize (GObject *obj)
{
	GthMediaViewerPage *self;

	self = GTH_MEDIA_VIEWER_PAGE (obj);

        if (self->priv->update_progress_id != 0) {
                g_source_remove (self->priv->update_progress_id);
                self->priv->update_progress_id = 0;
        }

        if (self->priv->update_volume_id != 0) {
                g_source_remove (self->priv->update_volume_id);
                self->priv->update_volume_id = 0;
        }

	if (self->priv->playbin != NULL) {
		save_volume (self);
		gst_element_set_state (self->priv->playbin, GST_STATE_NULL);
		gst_object_unref (GST_OBJECT (self->priv->playbin));
		self->priv->playbin = NULL;
	}
	_g_object_unref (self->priv->icon);
	_g_object_unref (self->priv->file_data);
	if (self->priv->screensaver != NULL) {
		gth_screensaver_uninhibit (self->priv->screensaver);
		g_object_unref (self->priv->screensaver);
	}

	G_OBJECT_CLASS (gth_media_viewer_page_parent_class)->finalize (obj);
}


static void
gth_media_viewer_page_class_init (GthMediaViewerPageClass *klass)
{
	g_type_class_add_private (klass, sizeof (GthMediaViewerPagePrivate));

	G_OBJECT_CLASS (klass)->finalize = gth_media_viewer_page_finalize;
}


static void
gth_viewer_page_interface_init (GthViewerPageInterface *iface)
{
	iface->activate = gth_media_viewer_page_real_activate;
	iface->deactivate = gth_media_viewer_page_real_deactivate;
	iface->show = gth_media_viewer_page_real_show;
	iface->hide = gth_media_viewer_page_real_hide;
	iface->can_view = gth_media_viewer_page_real_can_view;
	iface->view = gth_media_viewer_page_real_view;
	iface->focus = gth_media_viewer_page_real_focus;
	iface->fullscreen = gth_media_viewer_page_real_fullscreen;
	iface->show_pointer = gth_media_viewer_page_real_show_pointer;
	iface->update_sensitivity = gth_media_viewer_page_real_update_sensitivity;
	iface->can_save = gth_media_viewer_page_real_can_save;
	iface->save = gth_media_viewer_page_real_save;
	iface->save_as = gth_media_viewer_page_real_save_as;
	iface->revert = gth_media_viewer_page_real_revert;
	iface->update_info = gth_media_viewer_page_real_update_info;
}


static void
gth_media_viewer_page_init (GthMediaViewerPage *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_MEDIA_VIEWER_PAGE, GthMediaViewerPagePrivate);
	self->priv->update_progress_id = 0;
	self->priv->update_volume_id = 0;
	self->priv->video_window_xid = 0;
	self->priv->xwin_assigned = FALSE;
	self->priv->has_video = FALSE;
	self->priv->has_audio = FALSE;
	self->priv->video_fps_n = 0;
	self->priv->video_fps_d = 0;
	self->priv->icon = NULL;
	self->priv->cursor_visible = TRUE;
	self->priv->screensaver = gth_screensaver_new (NULL);
	self->priv->visible = FALSE;
	self->priv->screenshot_button = NULL;
	self->priv->background_painted = FALSE;
}


GthBrowser *
gth_media_viewer_page_get_browser (GthMediaViewerPage *self)
{
	return self->priv->browser;
}


GstElement *
gth_media_viewer_page_get_playbin (GthMediaViewerPage *self)
{
	return self->priv->playbin;
}


gboolean
gth_media_viewer_page_is_playing (GthMediaViewerPage *self)
{
	return self->priv->playing;
}


void
gth_media_viewer_page_get_video_fps (GthMediaViewerPage *self,
				     int                *video_fps_n,
				     int                *video_fps_d)
{
	if (video_fps_n != NULL)
		*video_fps_n = self->priv->video_fps_n;
	if (video_fps_d != NULL)
		*video_fps_d = self->priv->video_fps_d;
}


GthFileData *
gth_media_viewer_page_get_file_data (GthMediaViewerPage *self)
{
	return self->priv->file_data;
}
