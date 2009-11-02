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
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include <math.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdkx.h>
#include <gst/gst.h>
#include <gst/interfaces/xoverlay.h>
#include <gthumb.h>
#include "gstreamer-utils.h"
#include "gth-media-viewer-page.h"


#define GTH_MEDIA_VIEWER_PAGE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GTH_TYPE_MEDIA_VIEWER_PAGE, GthMediaViewerPagePrivate))
#define GCONF_NOTIFICATIONS 0


struct _GthMediaViewerPagePrivate {
	GthBrowser     *browser;
	GtkActionGroup *actions;
	guint           merge_id;
	GthFileData    *file_data;
	guint           cnxn_id[GCONF_NOTIFICATIONS];
	GstElement     *playbin;
	GtkWidget      *area;
};

static gpointer gth_media_viewer_page_parent_class = NULL;

static const char *media_viewer_ui_info =
"<ui>"
"  <toolbar name='ViewerToolBar'>"
"    <placeholder name='ViewerCommands'>"
"      <toolitem action='MediaViewer_Play'/>"
"      <toolitem action='MediaViewer_Pause'/>"
"    </placeholder>"
"  </toolbar>"
"  <toolbar name='Fullscreen_ToolBar'>"
"    <placeholder name='ViewerCommands'>"
"      <toolitem action='MediaViewer_Play'/>"
"      <toolitem action='MediaViewer_Pause'/>"
"    </placeholder>"
"  </toolbar>"
"</ui>";


static void
media_viewer_activate_action_play (GtkAction          *action,
				   GthMediaViewerPage *self)
{
	if (self->priv->playbin == NULL)
		return;
	gst_element_set_state (self->priv->playbin, GST_STATE_PLAYING);
}


static void
media_viewer_activate_action_pause (GtkAction          *action,
				    GthMediaViewerPage *self)
{
	if (self->priv->playbin == NULL)
		return;
	gst_element_set_state (self->priv->playbin, GST_STATE_PAUSED);
}


static GtkActionEntry image_viewer_action_entries[] = {
	{ "MediaViewer_Play", GTK_STOCK_MEDIA_PLAY,
	  NULL, NULL,
	  NULL,
	  G_CALLBACK (media_viewer_activate_action_play) },
	{ "MediaViewer_Pause", GTK_STOCK_MEDIA_PAUSE,
	  NULL, NULL,
	  NULL,
	  G_CALLBACK (media_viewer_activate_action_pause) },
};


static gboolean
expose_event_cb (GtkWidget      *widget,
                 GdkEventExpose *event,
                 gpointer        user_data)
{
	GthMediaViewerPage *self = user_data;

	if (self->priv->playbin != NULL)
		return FALSE;

	gdk_draw_rectangle (gtk_widget_get_window (widget),
			    widget->style->black_gc,
			    TRUE,
			    event->area.x,
			    event->area.y,
			    event->area.width,
			    event->area.height);

	return TRUE;
}


static gboolean
image_button_press_cb (GtkWidget          *widget,
		       GdkEventButton     *event,
		       GthMediaViewerPage *self)
{
	return gth_browser_viewer_button_press_cb (self->priv->browser, event);
}


static gboolean
scroll_event_cb (GtkWidget 	   *widget,
		 GdkEventScroll      *event,
		 GthMediaViewerPage  *self)
{
	return gth_browser_viewer_scroll_event_cb (self->priv->browser, event);
}


static gboolean
viewer_key_press_cb (GtkWidget          *widget,
		     GdkEventKey        *event,
		     GthMediaViewerPage *self)
{
	return gth_browser_viewer_key_press_cb (self->priv->browser, event);
}


static void
gth_media_viewer_page_real_activate (GthViewerPage *base,
				     GthBrowser    *browser)
{
	GthMediaViewerPage *self;
	int                 i;

	if (! gstreamer_init ())
		return;

	self = (GthMediaViewerPage*) base;

	self->priv->browser = browser;

	self->priv->actions = gtk_action_group_new ("Video Viewer Actions");
	gtk_action_group_set_translation_domain (self->priv->actions, NULL);
	gtk_action_group_add_actions (self->priv->actions,
				      image_viewer_action_entries,
				      G_N_ELEMENTS (image_viewer_action_entries),
				      self);
	gtk_ui_manager_insert_action_group (gth_browser_get_ui_manager (browser), self->priv->actions, 0);

	self->priv->area = gtk_drawing_area_new ();
	gtk_widget_set_double_buffered (self->priv->area, FALSE);
	gtk_widget_add_events (self->priv->area, (gtk_widget_get_events (self->priv->area)
						  | GDK_EXPOSURE_MASK
						  | GDK_BUTTON_PRESS_MASK
						  | GDK_BUTTON_RELEASE_MASK
						  | GDK_POINTER_MOTION_MASK
						  | GDK_POINTER_MOTION_HINT_MASK
						  | GDK_BUTTON_MOTION_MASK));
	GTK_WIDGET_SET_FLAGS (self->priv->area, GTK_CAN_FOCUS);

	gtk_widget_show (self->priv->area);

	/*
	self->priv->viewer = gth_image_viewer_new ();
	gth_image_viewer_set_zoom_quality (GTH_IMAGE_VIEWER (self->priv->viewer), eel_gconf_get_enum (PREF_ZOOM_QUALITY, GTH_TYPE_ZOOM_QUALITY, GTH_ZOOM_QUALITY_HIGH));
	gth_image_viewer_set_zoom_change (GTH_IMAGE_VIEWER (self->priv->viewer), eel_gconf_get_enum (PREF_ZOOM_CHANGE, GTH_TYPE_ZOOM_CHANGE, GTH_ZOOM_CHANGE_FIT_SIZE_IF_LARGER));
	gth_image_viewer_set_transp_type (GTH_IMAGE_VIEWER (self->priv->viewer), eel_gconf_get_enum (PREF_TRANSP_TYPE, GTH_TYPE_TRANSP_TYPE, GTH_TRANSP_TYPE_NONE));
	gth_image_viewer_set_check_type (GTH_IMAGE_VIEWER (self->priv->viewer), eel_gconf_get_enum (PREF_CHECK_TYPE, GTH_TYPE_CHECK_TYPE, GTH_CHECK_TYPE_MIDTONE));
	gth_image_viewer_set_check_size (GTH_IMAGE_VIEWER (self->priv->viewer), eel_gconf_get_enum (PREF_CHECK_SIZE, GTH_TYPE_CHECK_SIZE, GTH_CHECK_SIZE_MEDIUM));
	gth_image_viewer_set_black_background (GTH_IMAGE_VIEWER (self->priv->viewer), eel_gconf_get_boolean (PREF_BLACK_BACKGROUND, FALSE));
	gth_image_viewer_set_reset_scrollbars (GTH_IMAGE_VIEWER (self->priv->viewer), eel_gconf_get_boolean (PREF_RESET_SCROLLBARS, TRUE));

	gtk_widget_show (self->priv->viewer);

	g_signal_connect (G_OBJECT (self->priv->viewer),
			  "image_ready",
			  G_CALLBACK (image_ready_cb),
			  self);
	g_signal_connect (G_OBJECT (self->priv->viewer),
			  "zoom_changed",
			  G_CALLBACK (zoom_changed_cb),
			  self);
	*/
	g_signal_connect_after (G_OBJECT (self->priv->area),
				"expose_event",
				G_CALLBACK (expose_event_cb),
				self);
	g_signal_connect_after (G_OBJECT (self->priv->area),
				"button_press_event",
				G_CALLBACK (image_button_press_cb),
				self);
	g_signal_connect_after (G_OBJECT (self->priv->area),
				"scroll_event",
				G_CALLBACK (scroll_event_cb),
				self);
	g_signal_connect (G_OBJECT (self->priv->area),
			  "key_press_event",
			  G_CALLBACK (viewer_key_press_cb),
			  self);

	/*
	self->priv->nav_window = gth_nav_window_new (GTH_IMAGE_VIEWER (self->priv->viewer));
	gtk_widget_show (self->priv->nav_window);

	*/

	gth_browser_set_viewer_widget (browser, self->priv->area);
	gtk_widget_realize (self->priv->area);
	gdk_window_ensure_native (gtk_widget_get_window (self->priv->area));
	gth_viewer_page_focus (GTH_VIEWER_PAGE (self));


	/* gconf notifications */

	for (i = 0; i < GCONF_NOTIFICATIONS; i++)
		self->priv->cnxn_id[i] = 0;

	i = 0;
	/*
	self->priv->cnxn_id[i++] = eel_gconf_notification_add (
					   PREF_ZOOM_QUALITY,
					   pref_zoom_quality_changed,
					   self);
	*/
}


static void
gth_media_viewer_page_real_deactivate (GthViewerPage *base)
{
	GthMediaViewerPage *self;
	int                 i;

	self = (GthMediaViewerPage*) base;

	if (self->priv->playbin != NULL) {
		gst_element_set_state (self->priv->playbin, GST_STATE_NULL);
		gst_object_unref (GST_OBJECT (self->priv->playbin));
		self->priv->playbin = NULL;
	}

	for (i = 0; i < GCONF_NOTIFICATIONS; i++)
		if (self->priv->cnxn_id[i] != 0)
			eel_gconf_notification_remove (self->priv->cnxn_id[i]);

	/**/

	gth_browser_set_viewer_widget (self->priv->browser, NULL);
}


static GstBusSyncReply
set_playbin_window (GstBus             *bus,
		    GstMessage         *message,
		    GthMediaViewerPage *self)
{
	GstXOverlay *image_sink;

	/* ignore anything but 'prepare-xwindow-id' element messages */

	if (GST_MESSAGE_TYPE (message) != GST_MESSAGE_ELEMENT)
		return GST_BUS_PASS;
	if (! gst_structure_has_name (message->structure, "prepare-xwindow-id"))
		return GST_BUS_PASS;

	image_sink = GST_X_OVERLAY (GST_MESSAGE_SRC (message));
	gst_x_overlay_set_xwindow_id (image_sink, GDK_WINDOW_XID (gtk_widget_get_window (self->priv->area)));
	g_object_set (image_sink, "force-aspect-ratio", TRUE, NULL);

	gst_message_unref (message);

	return GST_BUS_DROP;
}


static void
bus_message_cb (GstBus     *bus,
                GstMessage *message,
                gpointer    user_data)
{
	switch (GST_MESSAGE_TYPE (message)) {
	case GST_MESSAGE_BUFFERING: {
		int percent = 0;
		gst_message_parse_buffering (message, &percent);
		g_print ("Buffering (%%%u percent done)", percent);
		break;
	}
	default:
		break;
	}
}


static void
gth_media_viewer_page_real_show (GthViewerPage *base)
{
	GthMediaViewerPage *self;
	GError             *error = NULL;
	GstBus             *bus;
	char               *uri;

	self = (GthMediaViewerPage*) base;

	if (self->priv->merge_id != 0)
		return;

	self->priv->merge_id = gtk_ui_manager_add_ui_from_string (gth_browser_get_ui_manager (self->priv->browser), media_viewer_ui_info, -1, &error);
	if (self->priv->merge_id == 0) {
		g_warning ("ui building failed: %s", error->message);
		g_error_free (error);
	}

	gth_viewer_page_focus (GTH_VIEWER_PAGE (self));

	if (self->priv->playbin != NULL) {
		gst_element_set_state (self->priv->playbin, GST_STATE_NULL);
		gst_object_unref (GST_OBJECT (self->priv->playbin));
	}
	self->priv->playbin = gst_element_factory_make ("playbin", "playbin");

	bus = gst_pipeline_get_bus (GST_PIPELINE (self->priv->playbin));
	/*gst_bus_enable_sync_message_emission (bus);*/
	gst_bus_set_sync_handler (bus, (GstBusSyncHandler) set_playbin_window, self);
	gst_bus_add_signal_watch (bus);
	g_signal_connect (bus, "message", G_CALLBACK (bus_message_cb), self);

	if (self->priv->file_data == NULL)
		return;

	uri = g_file_get_uri (self->priv->file_data->file);
	g_object_set (G_OBJECT (self->priv->playbin),
		      "uri", uri,
		      NULL);
	gst_element_set_state (self->priv->playbin, GST_STATE_PAUSED);

	g_free (uri);
}


static void
gth_media_viewer_page_real_hide (GthViewerPage *base)
{
	GthMediaViewerPage *self;

	self = (GthMediaViewerPage*) base;

	if (self->priv->merge_id != 0) {
		gtk_ui_manager_remove_ui (gth_browser_get_ui_manager (self->priv->browser), self->priv->merge_id);
		self->priv->merge_id = 0;
	}
}


static gboolean
gth_media_viewer_page_real_can_view (GthViewerPage *base,
				     GthFileData   *file_data)
{
	GthMediaViewerPage *self;

	self = (GthMediaViewerPage*) base;
	g_return_val_if_fail (file_data != NULL, FALSE);

	return _g_mime_type_is_video (gth_file_data_get_mime_type (file_data)) || _g_mime_type_is_audio (gth_file_data_get_mime_type (file_data));
}


static void
gth_media_viewer_page_real_view (GthViewerPage *base,
				 GthFileData   *file_data)
{
	GthMediaViewerPage *self;
	char               *uri;

	/* FIXME
	GthFileStore       *file_store;
	GtkTreeIter         iter;
	GthFileData        *next_file_data = NULL;
	GthFileData        *prev_file_data = NULL;
	*/

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

	_g_object_unref (self->priv->file_data);
	self->priv->file_data = gth_file_data_dup (file_data);

	/* FIXME
	file_store = gth_browser_get_file_store (self->priv->browser);
	if (gth_file_store_find_visible (file_store, file_data->file, &iter)) {
		GtkTreeIter iter2;

		iter2 = iter;
		if (gth_file_store_get_next_visible (file_store, &iter2))
			next_file_data = gth_file_store_get_file (file_store, &iter2);

		iter2 = iter;
		if (gth_file_store_get_prev_visible (file_store, &iter2))
			prev_file_data = gth_file_store_get_file (file_store, &iter2);
	}

	gth_image_preloader_load (self->priv->preloader,
				  file_data,
				  next_file_data,
				  prev_file_data);
	*/

	if (self->priv->playbin == NULL)
		return;

	uri = g_file_get_uri (self->priv->file_data->file);
	g_object_set (G_OBJECT (self->priv->playbin),
		      "uri", uri,
		      NULL);
	gst_element_set_state (self->priv->playbin, GST_STATE_PAUSED);

	g_free (uri);
}


static void
gth_media_viewer_page_real_focus (GthViewerPage *base)
{
	GtkWidget *widget;

	widget = GTH_MEDIA_VIEWER_PAGE (base)->priv->area;
	if (GTK_WIDGET_REALIZED (widget))
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
	/* FIXME */
}


static void
_set_action_sensitive (GthMediaViewerPage *self,
		       const char         *action_name,
		       gboolean            sensitive)
{
	GtkAction *action;

	action = gtk_action_group_get_action (self->priv->actions, action_name);
	g_object_set (action, "sensitive", sensitive, NULL);
}


static void
gth_media_viewer_page_real_update_sensitivity (GthViewerPage *base)
{
	/* FIXME */
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
gth_media_viewer_page_finalize (GObject *obj)
{
	GthMediaViewerPage *self;

	self = GTH_MEDIA_VIEWER_PAGE (obj);

	if (self->priv->playbin != NULL) {
		gst_element_set_state (self->priv->playbin, GST_STATE_NULL);
		gst_object_unref (GST_OBJECT (self->priv->playbin));
	}
	_g_object_unref (self->priv->file_data);

	G_OBJECT_CLASS (gth_media_viewer_page_parent_class)->finalize (obj);
}


static void
gth_media_viewer_page_class_init (GthMediaViewerPageClass *klass)
{
	gth_media_viewer_page_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GthMediaViewerPagePrivate));

	G_OBJECT_CLASS (klass)->finalize = gth_media_viewer_page_finalize;
}


static void
gth_viewer_page_interface_init (GthViewerPageIface *iface)
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
}


static void
gth_media_viewer_page_instance_init (GthMediaViewerPage *self)
{
	self->priv = GTH_MEDIA_VIEWER_PAGE_GET_PRIVATE (self);
}


GType
gth_media_viewer_page_get_type (void) {
	static GType gth_media_viewer_page_type_id = 0;
	if (gth_media_viewer_page_type_id == 0) {
		static const GTypeInfo g_define_type_info = {
			sizeof (GthMediaViewerPageClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) gth_media_viewer_page_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (GthMediaViewerPage),
			0,
			(GInstanceInitFunc) gth_media_viewer_page_instance_init,
			NULL
		};
		static const GInterfaceInfo gth_viewer_page_info = {
			(GInterfaceInitFunc) gth_viewer_page_interface_init,
			(GInterfaceFinalizeFunc) NULL,
			NULL
		};
		gth_media_viewer_page_type_id = g_type_register_static (G_TYPE_OBJECT, "GthMediaViewerPage", &g_define_type_info, 0);
		g_type_add_interface_static (gth_media_viewer_page_type_id, GTH_TYPE_VIEWER_PAGE, &gth_viewer_page_info);
	}
	return gth_media_viewer_page_type_id;
}
