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
#include <gthumb.h>
#include "gth-image-viewer-page.h"


#define GTH_IMAGE_VIEWER_PAGE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GTH_TYPE_IMAGE_VIEWER_PAGE, GthImageViewerPagePrivate))
#define GCONF_NOTIFICATIONS 7


struct _GthImageViewerPagePrivate {
	GthBrowser        *browser;
	GtkWidget         *nav_window;
	GtkWidget         *viewer;
	GthImagePreloader *preloader;
	GtkActionGroup    *actions;
	guint              merge_id;
	GthImageHistory   *history;
	GthFileData       *file_data;
	gulong             preloader_sig_id;
	guint              cnxn_id[GCONF_NOTIFICATIONS];
	guint              hide_mouse_timeout;
	guint              motion_signal;

};

static gpointer gth_image_viewer_page_parent_class = NULL;

static const char *image_viewer_ui_info =
"<ui>"
"  <menubar name='MenuBar'>"
"    <menu name='Edit' action='EditMenu'>"
"      <placeholder name='File_Actions_1'>"
"        <menuitem action='ImageViewer_Edit_Undo'/>"
"        <menuitem action='ImageViewer_Edit_Redo'/>"
"      </placeholder>"
"    </menu>"
"  </menubar>"
"  <toolbar name='ViewerToolBar'>"
"    <placeholder name='ViewerCommands'>"
"      <toolitem action='ImageViewer_View_ZoomIn'/>"
"      <toolitem action='ImageViewer_View_ZoomOut'/>"
"      <toolitem action='ImageViewer_View_Zoom100'/>"
"      <toolitem action='ImageViewer_View_ZoomFit'/>"
"      <toolitem action='ImageViewer_View_ZoomFitWidth'/>"
"    </placeholder>"
"    <placeholder name='ViewerCommandsSecondary'>"
"      <toolitem action='Viewer_Tools'/>"
"    </placeholder>"
"  </toolbar>"
"  <toolbar name='Fullscreen_ToolBar'>"
"    <placeholder name='ViewerCommands'>"
"      <toolitem action='ImageViewer_View_ZoomIn'/>"
"      <toolitem action='ImageViewer_View_ZoomOut'/>"
"      <toolitem action='ImageViewer_View_Zoom100'/>"
"      <toolitem action='ImageViewer_View_ZoomFit'/>"
"      <toolitem action='ImageViewer_View_ZoomFitWidth'/>"
"    </placeholder>"
"    <placeholder name='ViewerCommandsSecondary'>"
"      <toolitem action='Viewer_Tools'/>"
"    </placeholder>"
"  </toolbar>"
"</ui>";


static void
image_viewer_activate_action_view_zoom_in (GtkAction          *action,
					   GthImageViewerPage *self)
{
	gth_image_viewer_zoom_in (GTH_IMAGE_VIEWER (self->priv->viewer));
}


static void
image_viewer_activate_action_view_zoom_out (GtkAction          *action,
					    GthImageViewerPage *self)
{
	gth_image_viewer_zoom_out (GTH_IMAGE_VIEWER (self->priv->viewer));
}


static void
image_viewer_activate_action_view_zoom_100 (GtkAction          *action,
					    GthImageViewerPage *self)
{
	gth_image_viewer_set_zoom (GTH_IMAGE_VIEWER (self->priv->viewer), 1.0);
}


static void
image_viewer_activate_action_view_zoom_fit (GtkAction          *action,
					    GthImageViewerPage *self)
{
	gth_image_viewer_set_fit_mode (GTH_IMAGE_VIEWER (self->priv->viewer), GTH_FIT_SIZE);
}


static void
image_viewer_activate_action_view_zoom_fit_width (GtkAction          *action,
						  GthImageViewerPage *self)
{
	gth_image_viewer_set_fit_mode (GTH_IMAGE_VIEWER (self->priv->viewer), GTH_FIT_WIDTH);
}


static void
image_viewer_activate_action_edit_undo (GtkAction          *action,
					GthImageViewerPage *self)
{
	gth_image_viewer_page_undo (self);
}


static void
image_viewer_activate_action_edit_redo (GtkAction          *action,
					GthImageViewerPage *self)
{
	gth_image_viewer_page_redo (self);
}


static GtkActionEntry image_viewer_action_entries[] = {
	{ "ImageViewer_Edit_Undo", GTK_STOCK_UNDO,
	  NULL, "<control>z",
	  NULL,
	  G_CALLBACK (image_viewer_activate_action_edit_undo) },

	{ "ImageViewer_Edit_Redo", GTK_STOCK_REDO,
	  NULL, "<shift><control>z",
	  NULL,
	  G_CALLBACK (image_viewer_activate_action_edit_redo) },

	{ "ImageViewer_View_ZoomIn", GTK_STOCK_ZOOM_IN,
	  N_("In"), "<control>plus",
	  N_("Zoom in"),
	  G_CALLBACK (image_viewer_activate_action_view_zoom_in) },

	{ "ImageViewer_View_ZoomOut", GTK_STOCK_ZOOM_OUT,
	  N_("Out"), "<control>minus",
	  N_("Zoom out"),
	  G_CALLBACK (image_viewer_activate_action_view_zoom_out) },

	{ "ImageViewer_View_Zoom100", GTK_STOCK_ZOOM_100,
	  N_("1:1"), "<control>0",
	  N_("Actual size"),
	  G_CALLBACK (image_viewer_activate_action_view_zoom_100) },

	{ "ImageViewer_View_ZoomFit", GTK_STOCK_ZOOM_FIT,
	  N_("Fit"), "",
	  N_("Zoom to fit window"),
	  G_CALLBACK (image_viewer_activate_action_view_zoom_fit) },

	{ "ImageViewer_View_ZoomFitWidth", GTH_STOCK_ZOOM_FIT_WIDTH,
	  N_("Width"), "",
	  N_("Zoom to fit width"),
	  G_CALLBACK (image_viewer_activate_action_view_zoom_fit_width) },
};


static void
image_ready_cb (GtkWidget          *widget,
		GthImageViewerPage *self)
{
	gth_image_history_clear (self->priv->history);

	g_file_info_set_attribute_boolean (self->priv->file_data->info, "gth::file::is-modified", FALSE);
	gth_monitor_metadata_changed (gth_main_get_default_monitor (), self->priv->file_data);

}


static gboolean
zoom_changed_cb (GtkWidget          *widget,
		 GthImageViewerPage *self)
{
	double  zoom;
	char   *text;

	gth_viewer_page_update_sensitivity (GTH_VIEWER_PAGE (self));

	zoom = gth_image_viewer_get_zoom (GTH_IMAGE_VIEWER (self->priv->viewer));
	text = g_strdup_printf ("  %d%%  ", (int) (zoom * 100));
	gth_statusbar_set_secondary_text (GTH_STATUSBAR (gth_browser_get_statusbar (self->priv->browser)), text);

	g_free (text);

	return TRUE;
}


static gboolean
image_button_press_cb (GtkWidget          *widget,
		       GdkEventButton     *event,
		       GthImageViewerPage *self)
{
	if (event->button == 3) {
		gth_browser_file_menu_popup (self->priv->browser, event);
		return TRUE;
	}

	return FALSE;
}


static gboolean
mouse_wheel_scrolled_cb (GtkWidget 	     *widget,
  		   	 GdkScrollDirection   direction,
			 GthImageViewerPage  *self)
{
	if (direction == GDK_SCROLL_UP)
		gth_browser_show_prev_image (self->priv->browser, FALSE, FALSE);
	else
		gth_browser_show_next_image (self->priv->browser, FALSE, FALSE);

	return TRUE;
}


static gboolean
viewer_key_press_cb (GtkWidget          *widget,
		     GdkEventKey        *event,
		     GthImageViewerPage *self)
{
	switch (gdk_keyval_to_lower (event->keyval)) {
	case GDK_Page_Up:
		gth_browser_show_prev_image (self->priv->browser, TRUE, FALSE);
		return TRUE;

	case GDK_Page_Down:
		gth_browser_show_next_image (self->priv->browser, TRUE, FALSE);
		return TRUE;

	case GDK_Home:
		gth_browser_show_first_image (self->priv->browser, TRUE, FALSE);
		return TRUE;

	case GDK_End:
		gth_browser_show_last_image (self->priv->browser, TRUE, FALSE);
		return TRUE;
	}

	return gth_hook_invoke_get ("gth-browser-file-list-key-press", self->priv->browser, event) != NULL;
}


static void
image_preloader_requested_ready_cb (GthImagePreloader  *preloader,
				    GError             *error,
				    GthImageViewerPage *self)
{
	GthImageLoader *image_loader;

	image_loader = gth_image_preloader_get_loader (self->priv->preloader, gth_image_preloader_get_requested (self->priv->preloader));
	if (image_loader == NULL)
		return;

	gth_image_viewer_load_from_image_loader (GTH_IMAGE_VIEWER (self->priv->viewer), image_loader);
}


static void
pref_zoom_quality_changed (GConfClient *client,
			   guint        cnxn_id,
			   GConfEntry  *entry,
			   gpointer     user_data)
{
	GthImageViewerPage *self = user_data;
	GthImageViewer     *image_viewer = GTH_IMAGE_VIEWER (self->priv->viewer);

	gth_image_viewer_set_zoom_quality (image_viewer, eel_gconf_get_enum (PREF_ZOOM_QUALITY, GTH_TYPE_ZOOM_QUALITY, GTH_ZOOM_QUALITY_HIGH));
	gth_image_viewer_update_view (image_viewer);
}


static void
pref_zoom_change_changed (GConfClient *client,
			  guint        cnxn_id,
			  GConfEntry  *entry,
			  gpointer     user_data)
{
	GthImageViewerPage *self = user_data;
	GthImageViewer     *image_viewer = GTH_IMAGE_VIEWER (self->priv->viewer);

	gth_image_viewer_set_zoom_change (image_viewer, eel_gconf_get_enum (PREF_ZOOM_CHANGE, GTH_TYPE_ZOOM_CHANGE, GTH_ZOOM_CHANGE_FIT_SIZE_IF_LARGER));
	gth_image_viewer_update_view (image_viewer);
}


static void
pref_transp_type_changed (GConfClient *client,
			  guint        cnxn_id,
			  GConfEntry  *entry,
			  gpointer     user_data)
{
	GthImageViewerPage *self = user_data;
	GthImageViewer     *image_viewer = GTH_IMAGE_VIEWER (self->priv->viewer);

	gth_image_viewer_set_transp_type (image_viewer, eel_gconf_get_enum (PREF_TRANSP_TYPE, GTH_TYPE_TRANSP_TYPE, GTH_TRANSP_TYPE_NONE));
	gth_image_viewer_update_view (image_viewer);
}


static void
pref_check_type_changed (GConfClient *client,
			 guint        cnxn_id,
			 GConfEntry  *entry,
			 gpointer     user_data)
{
	GthImageViewerPage *self = user_data;
	GthImageViewer     *image_viewer = GTH_IMAGE_VIEWER (self->priv->viewer);

	gth_image_viewer_set_check_type (image_viewer, eel_gconf_get_enum (PREF_CHECK_TYPE, GTH_TYPE_CHECK_TYPE, GTH_CHECK_TYPE_MIDTONE));
	gth_image_viewer_update_view (image_viewer);
}


static void
pref_check_size_changed (GConfClient *client,
			 guint        cnxn_id,
			 GConfEntry  *entry,
			 gpointer     user_data)
{
	GthImageViewerPage *self = user_data;
	GthImageViewer     *image_viewer = GTH_IMAGE_VIEWER (self->priv->viewer);

	gth_image_viewer_set_check_size (image_viewer, eel_gconf_get_enum (PREF_CHECK_SIZE, GTH_TYPE_CHECK_SIZE, GTH_CHECK_SIZE_MEDIUM));
	gth_image_viewer_update_view (image_viewer);
}


static void
pref_black_background_changed (GConfClient *client,
			       guint        cnxn_id,
			       GConfEntry  *entry,
			       gpointer     user_data)
{
	GthImageViewerPage *self = user_data;
	GthImageViewer     *image_viewer = GTH_IMAGE_VIEWER (self->priv->viewer);

	gth_image_viewer_set_black_background (image_viewer, eel_gconf_get_boolean (PREF_BLACK_BACKGROUND, FALSE));
}


static void
pref_reset_scrollbars_changed (GConfClient *client,
			       guint        cnxn_id,
			       GConfEntry  *entry,
			       gpointer     user_data)
{
	GthImageViewerPage *self = user_data;
	GthImageViewer     *image_viewer = GTH_IMAGE_VIEWER (self->priv->viewer);

	gth_image_viewer_set_reset_scrollbars (image_viewer, eel_gconf_get_boolean (PREF_RESET_SCROLLBARS, TRUE));
}


static void
gth_image_viewer_page_real_activate (GthViewerPage *base,
				     GthBrowser    *browser)
{
	GthImageViewerPage *self;
	int                 i;

	self = (GthImageViewerPage*) base;

	self->priv->browser = browser;

	self->priv->actions = gtk_action_group_new ("Image Viewer Actions");
	gtk_action_group_set_translation_domain (self->priv->actions, NULL);
	gtk_action_group_add_actions (self->priv->actions,
				      image_viewer_action_entries,
				      G_N_ELEMENTS (image_viewer_action_entries),
				      self);
	gtk_ui_manager_insert_action_group (gth_browser_get_ui_manager (browser), self->priv->actions, 0);

	self->priv->preloader = gth_browser_get_image_preloader (browser);
	self->priv->preloader_sig_id = g_signal_connect (G_OBJECT (self->priv->preloader),
							 "requested_ready",
							 G_CALLBACK (image_preloader_requested_ready_cb),
							 self);

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
	g_signal_connect_after (G_OBJECT (self->priv->viewer),
				"button_press_event",
				G_CALLBACK (image_button_press_cb),
				self);
	g_signal_connect_after (G_OBJECT (self->priv->viewer),
				"mouse_wheel_scroll",
				G_CALLBACK (mouse_wheel_scrolled_cb),
				self);
	g_signal_connect (G_OBJECT (self->priv->viewer),
			  "key_press_event",
			  G_CALLBACK (viewer_key_press_cb),
			  self);

	self->priv->nav_window = gth_nav_window_new (GTH_IMAGE_VIEWER (self->priv->viewer));
	gtk_widget_show (self->priv->nav_window);

	gth_browser_set_viewer_widget (browser, self->priv->nav_window);

	/* gconf notifications */

	for (i = 0; i < GCONF_NOTIFICATIONS; i++)
		self->priv->cnxn_id[i] = 0;

	i = 0;
	self->priv->cnxn_id[i++] = eel_gconf_notification_add (
					   PREF_ZOOM_QUALITY,
					   pref_zoom_quality_changed,
					   self);

	self->priv->cnxn_id[i++] = eel_gconf_notification_add (
					   PREF_ZOOM_CHANGE,
					   pref_zoom_change_changed,
					   self);

	self->priv->cnxn_id[i++] = eel_gconf_notification_add (
					   PREF_TRANSP_TYPE,
					   pref_transp_type_changed,
					   self);

	self->priv->cnxn_id[i++] = eel_gconf_notification_add (
					   PREF_CHECK_TYPE,
					   pref_check_type_changed,
					   self);

	self->priv->cnxn_id[i++] = eel_gconf_notification_add (
					   PREF_CHECK_SIZE,
					   pref_check_size_changed,
					   self);

	self->priv->cnxn_id[i++] = eel_gconf_notification_add (
					   PREF_BLACK_BACKGROUND,
					   pref_black_background_changed,
					   self);

	self->priv->cnxn_id[i++] = eel_gconf_notification_add (
					   PREF_RESET_SCROLLBARS,
					   pref_reset_scrollbars_changed,
					   self);
}


static void
gth_image_viewer_page_real_deactivate (GthViewerPage *base)
{
	GthImageViewerPage *self;
	int                 i;

	self = (GthImageViewerPage*) base;

	/* remove gconf notifications */

	for (i = 0; i < GCONF_NOTIFICATIONS; i++)
		if (self->priv->cnxn_id[i] != 0)
			eel_gconf_notification_remove (self->priv->cnxn_id[i]);

	/**/

	g_signal_handler_disconnect (self->priv->preloader, self->priv->preloader_sig_id);
	self->priv->preloader_sig_id = 0;
	g_object_unref (self->priv->preloader);
	self->priv->preloader = NULL;

	gth_browser_set_viewer_widget (self->priv->browser, NULL);
}


static void
gth_image_viewer_page_real_show (GthViewerPage *base)
{
	GthImageViewerPage *self;
	GError             *error = NULL;

	self = (GthImageViewerPage*) base;

	if (self->priv->merge_id != 0)
		return;

	self->priv->merge_id = gtk_ui_manager_add_ui_from_string (gth_browser_get_ui_manager (self->priv->browser), image_viewer_ui_info, -1, &error);
	if (self->priv->merge_id == 0) {
		g_warning ("ui building failed: %s", error->message);
		g_error_free (error);
	}

	gtk_widget_grab_focus (self->priv->viewer);
}


static void
gth_image_viewer_page_real_hide (GthViewerPage *base)
{
	GthImageViewerPage *self;

	self = (GthImageViewerPage*) base;

	if (self->priv->merge_id != 0) {
		gtk_ui_manager_remove_ui (gth_browser_get_ui_manager (self->priv->browser), self->priv->merge_id);
		self->priv->merge_id = 0;
	}
}


static gboolean
gth_image_viewer_page_real_can_view (GthViewerPage *base,
				     GthFileData   *file_data)
{
	GthImageViewerPage *self;

	self = (GthImageViewerPage*) base;
	g_return_val_if_fail (file_data != NULL, FALSE);

	return _g_mime_type_is_image (gth_file_data_get_mime_type (file_data));
}


static void
gth_image_viewer_page_real_view (GthViewerPage *base,
				 GthFileData   *file_data)
{
	GthImageViewerPage *self;
	GthFileStore       *file_store;
	GtkTreeIter         iter;
	GthFileData        *next_file_data = NULL;
	GthFileData        *prev_file_data = NULL;

	self = (GthImageViewerPage*) base;
	g_return_if_fail (file_data != NULL);

	_g_object_unref (self->priv->file_data);
	self->priv->file_data = gth_file_data_dup (file_data);

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
}


static void
gth_image_viewer_page_real_fullscreen (GthViewerPage *base,
				       gboolean       active)
{
	GthImageViewerPage *self;

	self = (GthImageViewerPage *) base;
	if (active) {
		gth_nav_window_set_scrollbars_visible (GTH_NAV_WINDOW (self->priv->nav_window), FALSE);
		gth_image_viewer_set_black_background (GTH_IMAGE_VIEWER (self->priv->viewer), TRUE);
	}
	else {
		gth_nav_window_set_scrollbars_visible (GTH_NAV_WINDOW (self->priv->nav_window), TRUE);
		gth_image_viewer_set_black_background (GTH_IMAGE_VIEWER (self->priv->viewer), eel_gconf_get_boolean (PREF_BLACK_BACKGROUND, FALSE));
	}
}


static void
gth_image_viewer_page_real_show_pointer (GthViewerPage *base,
				         gboolean       show)
{
	GthImageViewerPage *self;

	self = (GthImageViewerPage *) base;

	if (show)
		gth_image_viewer_show_cursor (GTH_IMAGE_VIEWER (self->priv->viewer));
	else
		gth_image_viewer_hide_cursor (GTH_IMAGE_VIEWER (self->priv->viewer));
}


static void
_set_action_sensitive (GthImageViewerPage *self,
		       const char         *action_name,
		       gboolean            sensitive)
{
	GtkAction *action;

	action = gtk_action_group_get_action (self->priv->actions, action_name);
	g_object_set (action, "sensitive", sensitive, NULL);
}


static void
gth_image_viewer_page_real_update_sensitivity (GthViewerPage *base)
{
	GthImageViewerPage *self;
	double              zoom;
	GthFit              fit_mode;

	self = (GthImageViewerPage*) base;

	_set_action_sensitive (self, "ImageViewer_Edit_Undo", gth_image_history_can_undo (self->priv->history));
	_set_action_sensitive (self, "ImageViewer_Edit_Redo", gth_image_history_can_redo (self->priv->history));

	zoom = gth_image_viewer_get_zoom (GTH_IMAGE_VIEWER (self->priv->viewer));
	_set_action_sensitive (self, "ImageViewer_View_Zoom100", ! FLOAT_EQUAL (zoom, 1.0));
	_set_action_sensitive (self, "ImageViewer_View_ZoomOut", zoom > 0.05);
	_set_action_sensitive (self, "ImageViewer_View_ZoomIn", zoom < 100.0);

	fit_mode = gth_image_viewer_get_fit_mode (GTH_IMAGE_VIEWER (self->priv->viewer));
	_set_action_sensitive (self, "ImageViewer_View_ZoomFit", fit_mode != GTH_FIT_SIZE);
	_set_action_sensitive (self, "ImageViewer_View_ZoomFitWidth", fit_mode != GTH_FIT_WIDTH);
}


typedef struct {
	GthImageViewerPage *self;
	GthFileData        *original_file;
	FileSavedFunc       func;
	gpointer            user_data;
} SaveData;


static void
image_saved_cb (GthFileData *file_data,
		GError      *error,
		gpointer     user_data)
{
	SaveData           *data = user_data;
	GthImageViewerPage *self = data->self;
	gboolean            error_occurred;

	error_occurred = error != NULL;

	if (error_occurred) {
		GthFileData *current_file;

		current_file = gth_browser_get_current_file (self->priv->browser);
		if (current_file != NULL) {
			gth_file_data_set_file (current_file, data->original_file->file);
			g_file_info_set_attribute_boolean (current_file->info, "gth::file::is-modified", FALSE);
		}
	}

	if (data->func != NULL)
		(data->func) ((GthViewerPage *) self, self->priv->file_data, error, data->user_data);
	else if (error != NULL)
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (self->priv->browser), _("Could not save the file"), &error);

	if (! error_occurred) {
		GFile *folder;
		GList *file_list;

		folder = g_file_get_parent (self->priv->file_data->file);
		file_list = g_list_prepend (NULL, g_object_ref (self->priv->file_data->file));
		gth_monitor_folder_changed (gth_main_get_default_monitor (),
					    folder,
					    file_list,
					    GTH_MONITOR_EVENT_CHANGED);

		_g_object_list_unref (file_list);
		g_object_unref (folder);
	}

	g_object_unref (data->original_file);
	g_free (data);
}


static void
_gth_image_viewer_page_real_save (GthViewerPage *base,
				  GFile         *file,
				  const char    *mime_type,
				  FileSavedFunc  func,
				  gpointer       user_data)
{
	GthImageViewerPage *self;
	SaveData           *data;
	char               *pixbuf_type;
	GthFileData        *current_file;

	self = (GthImageViewerPage *) base;

	data = g_new0 (SaveData, 1);
	data->self = self;
	data->func = func;
	data->user_data = user_data;

	if (mime_type == NULL)
		mime_type = gth_file_data_get_mime_type (self->priv->file_data);
	pixbuf_type = get_pixbuf_type_from_mime_type (mime_type);

	current_file = gth_browser_get_current_file (self->priv->browser);
	data->original_file = gth_file_data_dup (current_file);
	if (file != NULL)
		gth_file_data_set_file (current_file, file);
	g_file_info_set_attribute_boolean (current_file->info, "gth::file::is-modified", FALSE);

	_gdk_pixbuf_save_async (gth_image_viewer_get_current_pixbuf (GTH_IMAGE_VIEWER (self->priv->viewer)),
			        current_file,
				pixbuf_type,
				NULL,
				NULL,
				image_saved_cb,
				data);

	g_free (pixbuf_type);
}


static gboolean
gth_image_viewer_page_real_can_save (GthViewerPage *base)
{
	return TRUE;
}


static void
gth_image_viewer_page_real_save (GthViewerPage *base,
				 GFile         *file,
				 FileSavedFunc  func,
				 gpointer       user_data)
{
	_gth_image_viewer_page_real_save (base, file, NULL, func, user_data);
}


/* -- gth_image_viewer_page_real_save_as -- */


typedef struct {
	GthImageViewerPage *self;
	FileSavedFunc       func;
	gpointer            user_data;
	GthFileData        *file_data;
	GtkWidget          *file_sel;
	GtkWidget          *format_chooser;
} SaveAsData;


static struct {
	const char *type;
	const char *extensions;
	const char *default_ext;
}
supported_formats[] = {
	{ "image/jpeg", "jpeg jpg jpe", "jpeg" },
	{ "image/png", "png", "png" },
	{ "image/tiff", "tiff tif", "tiff" },
	{ NULL, NULL }
};


static void
save_as_destroy_cb (GtkWidget  *w,
		    SaveAsData *data)
{
	g_object_unref (data->file_data);
	g_free (data);
}


static void
save_as_response_cb (GtkDialog  *file_sel,
		     int         response,
		     SaveAsData *data)
{
	char  *filename;
	int    format;
	GFile *file;

	if (response != GTK_RESPONSE_ACCEPT) {
		if (data->func != NULL) {
			(*data->func) ((GthViewerPage *) data->self,
				       data->file_data,
				       g_error_new_literal (G_IO_ERROR, G_IO_ERROR_CANCELLED, ""),
				       data->user_data);
		}
		gtk_widget_destroy (GTK_WIDGET (file_sel));
		return;
	}

	filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (file_sel));
	format = egg_file_format_chooser_get_format (EGG_FILE_FORMAT_CHOOSER (data->format_chooser), filename);
	g_free (filename);

	if ((format < 1) || (format > G_N_ELEMENTS (supported_formats)))
		return;

	file = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (file_sel));
	gth_file_data_set_file (data->file_data, file);
	_gth_image_viewer_page_real_save ((GthViewerPage *) data->self,
					  file,
					  supported_formats[format - 1].type,
					  data->func,
					  data->user_data);
	gtk_widget_destroy (GTK_WIDGET (data->file_sel));

	g_object_unref (file);
}


static void
format_chooser_selection_changed_cb (EggFileFormatChooser *self,
				     SaveAsData           *data)
{
	char *filename;
	int   format;
	char *basename;
	char *basename_noext;
	char *new_basename;

	filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (data->file_sel));
	format = egg_file_format_chooser_get_format (EGG_FILE_FORMAT_CHOOSER (data->format_chooser), filename);

	if ((format < 1) || (format > G_N_ELEMENTS (supported_formats))) {
		g_free (filename);
		return;
	}

	basename = g_path_get_basename (filename);
	basename_noext = _g_uri_remove_extension (basename);
	new_basename = g_strconcat (basename_noext, ".", supported_formats[format - 1].default_ext, NULL);
	gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (data->file_sel), new_basename);

	g_free (new_basename);
	g_free (basename_noext);
	g_free (basename);
	g_free (filename);
}


static char *
get_icon_name_for_type (const char *mime_type)
{
	char *name = NULL;

	if (mime_type != NULL) {
		char *s;

		name = g_strconcat ("gnome-mime-", mime_type, NULL);
		for (s = name; *s; ++s)
			if (! g_ascii_isalpha (*s))
				*s = '-';
	}

	if ((name == NULL) || ! gtk_icon_theme_has_icon (gtk_icon_theme_get_default (), name)) {
		g_free (name);
		name = g_strdup ("image-x-generic");
	}

	return name;
}


static void
gth_image_viewer_page_real_save_as (GthViewerPage *base,
				    FileSavedFunc  func,
				    gpointer       user_data)
{
	GthImageViewerPage   *self;
	GtkWidget            *file_sel;
	char                 *uri;
	EggFileFormatChooser *format_chooser;
	SaveAsData           *data;
	int                   i;

	self = GTH_IMAGE_VIEWER_PAGE (base);
	file_sel = gtk_file_chooser_dialog_new (_("Save Image"),
						GTK_WINDOW (self->priv->browser),
						GTK_FILE_CHOOSER_ACTION_SAVE,
						GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
						NULL);
	gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (file_sel), FALSE);
	gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (file_sel), TRUE);
	gtk_dialog_set_default_response (GTK_DIALOG (file_sel), GTK_RESPONSE_ACCEPT);
	uri = g_file_get_uri (self->priv->file_data->file);
	gtk_file_chooser_set_uri (GTK_FILE_CHOOSER (file_sel), uri);

	/**/

	format_chooser = (EggFileFormatChooser *) egg_file_format_chooser_new ();
	for (i = 0; supported_formats[i].type != NULL; i++) {
		char  *icon_name;
		char **extensions;

		icon_name = get_icon_name_for_type (supported_formats[i].type);
		extensions = g_strsplit (supported_formats[i].extensions, " ", -1);
		egg_file_format_chooser_add_format (format_chooser,
						    0,
						    g_content_type_get_description (supported_formats[i].type),
						    icon_name,
						    extensions[0],
						    extensions[1],
						    extensions[2],
						    NULL);

		g_strfreev (extensions);
		g_free (icon_name);
	}

	gtk_file_chooser_set_extra_widget (GTK_FILE_CHOOSER (file_sel),
					   GTK_WIDGET (format_chooser));

	/**/

	data = g_new0 (SaveAsData, 1);
	data->self = self;
	data->func = func;
	data->file_data = gth_file_data_dup (self->priv->file_data);
	data->user_data = user_data;
	data->file_sel = file_sel;
	data->format_chooser = (GtkWidget *) format_chooser;

	g_signal_connect (GTK_DIALOG (file_sel),
			  "response",
			  G_CALLBACK (save_as_response_cb),
			  data);
	g_signal_connect (G_OBJECT (file_sel),
			  "destroy",
			  G_CALLBACK (save_as_destroy_cb),
			  data);
	g_signal_connect (G_OBJECT (format_chooser),
			  "selection-changed",
			  G_CALLBACK (format_chooser_selection_changed_cb),
			  data);

	gtk_window_set_transient_for (GTK_WINDOW (file_sel), GTK_WINDOW (self->priv->browser));
	gtk_window_set_modal (GTK_WINDOW (file_sel), TRUE);
	gtk_widget_show (file_sel);

	g_free (uri);
}


static void
gth_image_viewer_page_finalize (GObject *obj)
{
	GthImageViewerPage *self;

	self = GTH_IMAGE_VIEWER_PAGE (obj);

	g_object_unref (self->priv->history);
	_g_object_unref (self->priv->file_data);

	G_OBJECT_CLASS (gth_image_viewer_page_parent_class)->finalize (obj);
}


static void
gth_image_viewer_page_class_init (GthImageViewerPageClass *klass)
{
	gth_image_viewer_page_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GthImageViewerPagePrivate));

	G_OBJECT_CLASS (klass)->finalize = gth_image_viewer_page_finalize;
}


static void
gth_viewer_page_interface_init (GthViewerPageIface *iface)
{
	iface->activate = gth_image_viewer_page_real_activate;
	iface->deactivate = gth_image_viewer_page_real_deactivate;
	iface->show = gth_image_viewer_page_real_show;
	iface->hide = gth_image_viewer_page_real_hide;
	iface->can_view = gth_image_viewer_page_real_can_view;
	iface->view = gth_image_viewer_page_real_view;
	iface->fullscreen = gth_image_viewer_page_real_fullscreen;
	iface->show_pointer = gth_image_viewer_page_real_show_pointer;
	iface->update_sensitivity = gth_image_viewer_page_real_update_sensitivity;
	iface->can_save = gth_image_viewer_page_real_can_save;
	iface->save = gth_image_viewer_page_real_save;
	iface->save_as = gth_image_viewer_page_real_save_as;
}


static void
gth_image_viewer_page_instance_init (GthImageViewerPage *self)
{
	self->priv = GTH_IMAGE_VIEWER_PAGE_GET_PRIVATE (self);
	self->priv->history = gth_image_history_new ();
}


GType
gth_image_viewer_page_get_type (void) {
	static GType gth_image_viewer_page_type_id = 0;
	if (gth_image_viewer_page_type_id == 0) {
		static const GTypeInfo g_define_type_info = {
			sizeof (GthImageViewerPageClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) gth_image_viewer_page_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (GthImageViewerPage),
			0,
			(GInstanceInitFunc) gth_image_viewer_page_instance_init,
			NULL
		};
		static const GInterfaceInfo gth_viewer_page_info = {
			(GInterfaceInitFunc) gth_viewer_page_interface_init,
			(GInterfaceFinalizeFunc) NULL,
			NULL
		};
		gth_image_viewer_page_type_id = g_type_register_static (G_TYPE_OBJECT, "GthImageViewerPage", &g_define_type_info, 0);
		g_type_add_interface_static (gth_image_viewer_page_type_id, GTH_TYPE_VIEWER_PAGE, &gth_viewer_page_info);
	}
	return gth_image_viewer_page_type_id;
}


GtkWidget *
gth_image_viewer_page_get_image_viewer (GthImageViewerPage *self)
{
	return self->priv->viewer;
}


GdkPixbuf *
gth_image_viewer_page_get_pixbuf (GthImageViewerPage *self)
{
	return gth_image_viewer_get_current_pixbuf (GTH_IMAGE_VIEWER (self->priv->viewer));
}


static void
_gth_image_viewer_page_set_pixbuf (GthImageViewerPage *self,
				   GdkPixbuf          *pixbuf,
				   gboolean            modified)
{
	GthFileData *file_data;
	int          width;
	int          height;
	char        *size;

	gth_image_viewer_set_pixbuf (GTH_IMAGE_VIEWER (self->priv->viewer), pixbuf);

	file_data = gth_browser_get_current_file (GTH_BROWSER (self->priv->browser));

	g_file_info_set_attribute_boolean (file_data->info, "gth::file::is-modified", modified);

	width = gdk_pixbuf_get_width (pixbuf);
	height = gdk_pixbuf_get_height (pixbuf);
	g_file_info_set_attribute_int32 (file_data->info, "image::width", width);
	g_file_info_set_attribute_int32 (file_data->info, "image::height", height);

	size = g_strdup_printf ("%d x %d", width, height);
	g_file_info_set_attribute_string (file_data->info, "image::size", size);

	gth_monitor_metadata_changed (gth_main_get_default_monitor (), file_data);

	g_free (size);
}


void
gth_image_viewer_page_set_pixbuf (GthImageViewerPage *self,
				  GdkPixbuf          *pixbuf)
{
	gth_image_history_add_image (self->priv->history,
				     gth_image_viewer_page_get_pixbuf (self),
				     gth_browser_get_file_modified (GTH_BROWSER (self->priv->browser)));
	_gth_image_viewer_page_set_pixbuf (self, pixbuf, TRUE);
}


void
gth_image_viewer_page_undo (GthImageViewerPage *self)
{
	GthImageData *idata;

	idata = gth_image_history_undo (self->priv->history,
					gth_image_viewer_page_get_pixbuf (self),
					gth_browser_get_file_modified (GTH_BROWSER (self->priv->browser)));
	if (idata != NULL) {
		_gth_image_viewer_page_set_pixbuf (self, idata->image, idata->unsaved);
		gth_image_data_unref (idata);
	}
}


void
gth_image_viewer_page_redo (GthImageViewerPage *self)
{
	GthImageData *idata;

	idata = gth_image_history_redo (self->priv->history,
					gth_image_viewer_page_get_pixbuf (self),
					gth_browser_get_file_modified (GTH_BROWSER (self->priv->browser)));
	if (idata != NULL) {
		_gth_image_viewer_page_set_pixbuf (self, idata->image, idata->unsaved);
		gth_image_data_unref (idata);
	}
}


GthImageHistory *
gth_image_viewer_page_get_history (GthImageViewerPage *self)
{
	return self->priv->history;
}
