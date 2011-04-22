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
#include <gthumb.h>
#include "gth-image-viewer-page.h"
#include "preferences.h"

#define GTH_IMAGE_VIEWER_PAGE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GTH_TYPE_IMAGE_VIEWER_PAGE, GthImageViewerPagePrivate))
#define GCONF_NOTIFICATIONS 8


struct _GthImageViewerPagePrivate {
	GthBrowser        *browser;
	GtkWidget         *image_navigator;
	GtkWidget         *viewer;
	GthImagePreloader *preloader;
	GtkActionGroup    *actions;
	guint              merge_id;
	GthImageHistory   *history;
	GthFileData       *file_data;
	gulong             requested_ready_id;
	guint              cnxn_id[GCONF_NOTIFICATIONS];
	guint              hide_mouse_timeout;
	guint              motion_signal;
	gboolean           image_changed;
	gboolean           shrink_wrap;
	GFile             *last_loaded;
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
"    <menu name='View' action='ViewMenu'>"
"      <placeholder name='View_Actions'>"
"        <menuitem action='ImageViewer_View_ShrinkWrap'/>"
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


static void
image_viewer_activate_action_view_shrink_wrap (GtkAction          *action,
					       GthImageViewerPage *self)
{
	eel_gconf_set_boolean (PREF_VIEWER_SHRINK_WRAP, gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)));
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


static GtkToggleActionEntry image_viewer_toggle_action_entries[] = {
	{ "ImageViewer_View_ShrinkWrap", NULL,
	  N_("_Fit Window to Image"), "<control>e",
	  N_("Resize the window to the size of the image"),
	  G_CALLBACK (image_viewer_activate_action_view_shrink_wrap),
	  FALSE }
};


static void
gth_image_viewer_page_file_loaded (GthImageViewerPage *self,
				   gboolean            success)
{
	if (_g_file_equal (self->priv->last_loaded, self->priv->file_data->file))
		return;

	_g_object_unref (self->priv->last_loaded);
	self->priv->last_loaded = g_object_ref (self->priv->file_data->file);

	gth_viewer_page_file_loaded (GTH_VIEWER_PAGE (self),
				     self->priv->file_data,
				     success);
}


static gboolean
viewer_zoom_changed_cb (GtkWidget          *widget,
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
viewer_button_press_event_cb (GtkWidget          *widget,
			      GdkEventButton     *event,
			      GthImageViewerPage *self)
{
	return gth_browser_viewer_button_press_cb (self->priv->browser, event);
}


static gboolean
viewer_popup_menu_cb (GtkWidget          *widget,
		      GthImageViewerPage *self)
{
	gth_browser_file_menu_popup (self->priv->browser, NULL);
	return TRUE;
}


static gboolean
viewer_scroll_event_cb (GtkWidget 	   *widget,
		        GdkEventScroll      *event,
		        GthImageViewerPage  *self)
{
	return gth_browser_viewer_scroll_event_cb (self->priv->browser, event);
}


static gboolean
viewer_image_map_event_cb (GtkWidget          *widget,
			   GdkEvent           *event,
			   GthImageViewerPage *self)
{
	gth_viewer_page_focus (GTH_VIEWER_PAGE (self));
	return FALSE;
}


static gboolean
viewer_key_press_cb (GtkWidget          *widget,
		     GdkEventKey        *event,
		     GthImageViewerPage *self)
{
	return gth_browser_viewer_key_press_cb (self->priv->browser, event);
}


static void
image_preloader_requested_ready_cb (GthImagePreloader  *preloader,
				    GthFileData        *requested,
				    GthImage           *image,
				    int                 original_width,
				    int                 original_height,
				    GError             *error,
				    GthImageViewerPage *self)
{
	if (! _g_file_equal (requested->file, self->priv->file_data->file))
		return;

	if (error != NULL) {
		gth_image_viewer_page_file_loaded (self, FALSE);
		return;
	}

	gth_viewer_page_focus (GTH_VIEWER_PAGE (self));

	gth_image_viewer_set_image (GTH_IMAGE_VIEWER (self->priv->viewer),
				    image,
				    original_width,
				    original_height);

	if (self->priv->shrink_wrap)
		gth_image_viewer_page_shrink_wrap (self, TRUE);
	gth_image_history_clear (self->priv->history);
	gth_image_history_add_image (self->priv->history,
				     gth_image_viewer_get_current_image (GTH_IMAGE_VIEWER (self->priv->viewer)),
				     FALSE);
	gth_image_viewer_page_file_loaded (self, TRUE);
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
pref_viewer_shrink_wrap_changed (GConfClient *client,
				 guint        cnxn_id,
				 GConfEntry  *entry,
				 gpointer     user_data)
{
	GthImageViewerPage *self = user_data;

	gth_image_viewer_page_shrink_wrap (self, eel_gconf_get_boolean (PREF_VIEWER_SHRINK_WRAP, FALSE));
}


static void
paint_comment_over_image_func (GthImageViewer *image_viewer,
			       GdkEventExpose *event,
			       cairo_t        *cr,
			       gpointer        user_data)
{
	GthImageViewerPage *self = user_data;
	GthFileData        *file_data = self->priv->file_data;
	GString            *file_info;
	char               *comment;
	const char         *file_date;
	const char         *file_size;
	int                 current_position;
	int                 n_visibles;
	int                 width;
	int                 height;
	GthMetadata        *metadata;
	PangoLayout        *layout;
	PangoAttrList      *attr_list = NULL;
	GError             *error = NULL;
	char               *text;
	static GdkPixbuf   *icon = NULL;
	int                 icon_width;
	int                 icon_height;
	int                 image_width;
	int                 image_height;
	const int           x_padding = 10;
	const int           y_padding = 10;
	int                 max_text_width;
	PangoRectangle      bounds;
	int                 text_x;
	int                 text_y;
	int                 icon_x;
	int                 icon_y;

	file_info = g_string_new ("");

	comment = gth_file_data_get_attribute_as_string (file_data, "general::description");
	if (comment != NULL) {
		g_string_append_printf (file_info, "<b>%s</b>\n\n", comment);
		g_free (comment);
	}

	metadata = (GthMetadata *) g_file_info_get_attribute_object (file_data->info, "general::datetime");
	if (metadata != NULL)
		file_date = gth_metadata_get_formatted (metadata);
	else
		file_date = g_file_info_get_attribute_string (file_data->info, "gth::file::display-mtime");
	file_size = g_file_info_get_attribute_string (file_data->info, "gth::file::display-size");

	gth_browser_get_file_list_info (self->priv->browser, &current_position, &n_visibles);
	gth_image_viewer_get_original_size (GTH_IMAGE_VIEWER (self->priv->viewer), &width, &height);

	g_string_append_printf (file_info,
			        "<small><i>%s - %dx%d (%d%%) - %s</i>\n<tt>%d/%d - %s</tt></small>",
			        file_date,
			        width,
			        height,
			        (int) (gth_image_viewer_get_zoom (GTH_IMAGE_VIEWER (self->priv->viewer)) * 100),
			        file_size,
			        current_position + 1,
			        n_visibles,
				g_file_info_get_attribute_string (file_data->info, "standard::display-name"));

	layout = gtk_widget_create_pango_layout (GTK_WIDGET (self->priv->viewer), NULL);
	pango_layout_set_wrap (layout, PANGO_WRAP_WORD);
	pango_layout_set_font_description (layout, GTK_WIDGET (self->priv->viewer)->style->font_desc);
	pango_layout_set_alignment (layout, PANGO_ALIGN_LEFT);

	if (! pango_parse_markup (file_info->str,
			          -1,
				  0,
				  &attr_list,
				  &text,
				  NULL,
				  &error))
	{
		g_warning ("Failed to set text from markup due to error parsing markup: %s\nThis is the text that caused the error: %s",  error->message, file_info->str);
		g_error_free (error);
		g_object_unref (layout);
		g_string_free (file_info, TRUE);
		return;
	}

	pango_layout_set_attributes (layout, attr_list);
        pango_layout_set_text (layout, text, strlen (text));

        if (icon == NULL) {
        	GIcon *gicon;

        	gicon = g_themed_icon_new (GTK_STOCK_PROPERTIES);
        	icon = _g_icon_get_pixbuf (gicon, 24, NULL);

        	g_object_unref (gicon);
        }
	icon_width = gdk_pixbuf_get_width (icon);
	icon_height = gdk_pixbuf_get_height (icon);

	gdk_drawable_get_size (gtk_widget_get_window (self->priv->viewer), &image_width, &image_height);
	max_text_width = ((image_width * 3 / 4) - icon_width - (x_padding * 3) - (x_padding * 2));

	pango_layout_set_width (layout, max_text_width * PANGO_SCALE);
	pango_layout_get_pixel_extents (layout, NULL, &bounds);

	bounds.width += (2 * x_padding) + (icon_width + x_padding);
	bounds.height = MIN (image_height - icon_height - (y_padding * 2), bounds.height + (2 * y_padding));
	bounds.x = MAX ((image_width - bounds.width) / 2, 0);
	bounds.y = MAX (image_height - bounds.height - (y_padding * 3), 0);

	text_x = bounds.x + x_padding + icon_width + x_padding;
	text_y = bounds.y + y_padding;
	icon_x = bounds.x + x_padding;
	icon_y = bounds.y + (bounds.height - icon_height) / 2;

	cairo_save (cr);

	/* background */

	_cairo_draw_rounded_box (cr, bounds.x, bounds.y, bounds.width, bounds.height, 8.0);
	cairo_set_source_rgba (cr, 0.94, 0.94, 0.94, 0.81);
	cairo_fill (cr);
	cairo_set_line_width (cr, 1.0);
	cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
	cairo_stroke (cr);

	/* icon */

	gdk_cairo_set_source_pixbuf (cr, icon, icon_x, icon_y);
	cairo_rectangle (cr, icon_x, icon_y, icon_width, icon_height);
	cairo_fill (cr);

	/* text */

	cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
	pango_cairo_update_layout (cr, layout);
	cairo_move_to (cr, text_x, text_y);
	pango_cairo_show_layout (cr, layout);

	cairo_restore (cr);

        g_free (text);
        pango_attr_list_unref (attr_list);
	g_object_unref (layout);
	g_string_free (file_info, TRUE);
}


static void
gth_image_viewer_page_real_activate (GthViewerPage *base,
				     GthBrowser    *browser)
{
	GthImageViewerPage *self;
	GtkAction          *action;
	int                 i;

	self = (GthImageViewerPage*) base;

	self->priv->browser = browser;

	self->priv->actions = gtk_action_group_new ("Image Viewer Actions");
	gtk_action_group_set_translation_domain (self->priv->actions, NULL);
	gtk_action_group_add_actions (self->priv->actions,
				      image_viewer_action_entries,
				      G_N_ELEMENTS (image_viewer_action_entries),
				      self);
	gtk_action_group_add_toggle_actions (self->priv->actions,
					     image_viewer_toggle_action_entries,
					     G_N_ELEMENTS (image_viewer_toggle_action_entries),
					     self);
	gtk_ui_manager_insert_action_group (gth_browser_get_ui_manager (browser), self->priv->actions, 0);

	self->priv->preloader = gth_browser_get_image_preloader (browser);
	self->priv->requested_ready_id = g_signal_connect (G_OBJECT (self->priv->preloader),
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

	self->priv->shrink_wrap = eel_gconf_get_boolean (PREF_VIEWER_SHRINK_WRAP, FALSE);
	action = gtk_action_group_get_action (self->priv->actions, "ImageViewer_View_ShrinkWrap");
	if (action != NULL)
		g_object_set (action, "active", self->priv->shrink_wrap, NULL);

	gtk_widget_show (self->priv->viewer);

	g_signal_connect (G_OBJECT (self->priv->viewer),
			  "zoom_changed",
			  G_CALLBACK (viewer_zoom_changed_cb),
			  self);
	g_signal_connect (G_OBJECT (self->priv->viewer),
			  "popup-menu",
			  G_CALLBACK (viewer_popup_menu_cb),
			  self);
	g_signal_connect_after (G_OBJECT (self->priv->viewer),
				"button_press_event",
				G_CALLBACK (viewer_button_press_event_cb),
				self);
	g_signal_connect_after (G_OBJECT (self->priv->viewer),
				"scroll_event",
				G_CALLBACK (viewer_scroll_event_cb),
				self);
	g_signal_connect_after (G_OBJECT (self->priv->viewer),
				"map_event",
				G_CALLBACK (viewer_image_map_event_cb),
				self);
	g_signal_connect (G_OBJECT (self->priv->viewer),
			  "key_press_event",
			  G_CALLBACK (viewer_key_press_cb),
			  self);

	self->priv->image_navigator = gth_image_navigator_new (GTH_IMAGE_VIEWER (self->priv->viewer));
	gtk_widget_show (self->priv->image_navigator);

	gth_browser_set_viewer_widget (browser, self->priv->image_navigator);
	gth_viewer_page_focus (GTH_VIEWER_PAGE (self));

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

	self->priv->cnxn_id[i++] = eel_gconf_notification_add (
					   PREF_VIEWER_SHRINK_WRAP,
					   pref_viewer_shrink_wrap_changed,
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

	gtk_ui_manager_remove_action_group (gth_browser_get_ui_manager (self->priv->browser), self->priv->actions);
	g_object_unref (self->priv->actions);
	self->priv->actions = NULL;

	g_signal_handler_disconnect (self->priv->preloader, self->priv->requested_ready_id);
	self->priv->requested_ready_id = 0;

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

	gth_viewer_page_focus (GTH_VIEWER_PAGE (self));
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
	GthFileData        *next2_file_data = NULL;
	GthFileData        *prev_file_data = NULL;

	self = (GthImageViewerPage*) base;
	g_return_if_fail (file_data != NULL);

	gth_viewer_page_focus (GTH_VIEWER_PAGE (self));

	_g_clear_object (&self->priv->last_loaded);

	if ((self->priv->file_data != NULL)
	    && g_file_equal (file_data->file, self->priv->file_data->file)
	    && (gth_file_data_get_mtime (file_data) == gth_file_data_get_mtime (self->priv->file_data))
	    && ! self->priv->image_changed)
	{
		gth_image_viewer_page_file_loaded (self, TRUE);
		return;
	}

	_g_object_unref (self->priv->file_data);
	self->priv->file_data = gth_file_data_dup (file_data);
	self->priv->image_changed = FALSE;

	file_store = gth_browser_get_file_store (self->priv->browser);
	if (gth_file_store_find_visible (file_store, self->priv->file_data->file, &iter)) {
		GtkTreeIter iter2;
		GtkTreeIter iter3;

		iter2 = iter;
		if (gth_file_store_get_next_visible (file_store, &iter2))
			next_file_data = gth_file_store_get_file (file_store, &iter2);

		iter3 = iter2;
		if (gth_file_store_get_next_visible (file_store, &iter3))
			next2_file_data = gth_file_store_get_file (file_store, &iter3);

		iter2 = iter;
		if (gth_file_store_get_prev_visible (file_store, &iter2))
			prev_file_data = gth_file_store_get_file (file_store, &iter2);
	}

	gth_image_preloader_load (self->priv->preloader,
				  self->priv->file_data,
				  -1,
				  next_file_data,
				  next2_file_data,
				  prev_file_data,
				  NULL);
}


static void
gth_image_viewer_page_real_focus (GthViewerPage *base)
{
	GtkWidget *widget;

	widget = GTH_IMAGE_VIEWER_PAGE (base)->priv->viewer;
	if (gtk_widget_get_realized (widget) && gtk_widget_get_mapped (widget))
		gtk_widget_grab_focus (widget);
}


static void
gth_image_viewer_page_real_fullscreen (GthViewerPage *base,
				       gboolean       active)
{
	GthImageViewerPage *self;

	self = (GthImageViewerPage *) base;

	if (active) {
		gth_image_navigator_set_scrollbars_visible (GTH_IMAGE_NAVIGATOR (self->priv->image_navigator), FALSE);
		gth_image_viewer_set_black_background (GTH_IMAGE_VIEWER (self->priv->viewer), TRUE);
	}
	else {
		gth_image_navigator_set_scrollbars_visible (GTH_IMAGE_NAVIGATOR (self->priv->image_navigator), TRUE);
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
	gboolean            zoom_enabled;
	double              zoom;
	GthFit              fit_mode;

	self = (GthImageViewerPage*) base;

	_set_action_sensitive (self, "ImageViewer_Edit_Undo", gth_image_history_can_undo (self->priv->history));
	_set_action_sensitive (self, "ImageViewer_Edit_Redo", gth_image_history_can_redo (self->priv->history));

	zoom_enabled = gth_image_viewer_get_zoom_enabled (GTH_IMAGE_VIEWER (self->priv->viewer));
	zoom = gth_image_viewer_get_zoom (GTH_IMAGE_VIEWER (self->priv->viewer));

	_set_action_sensitive (self, "ImageViewer_View_Zoom100", zoom_enabled && ! FLOAT_EQUAL (zoom, 1.0));
	_set_action_sensitive (self, "ImageViewer_View_ZoomOut", zoom_enabled && (zoom > 0.05));
	_set_action_sensitive (self, "ImageViewer_View_ZoomIn", zoom_enabled && (zoom < 100.0));

	fit_mode = gth_image_viewer_get_fit_mode (GTH_IMAGE_VIEWER (self->priv->viewer));
	_set_action_sensitive (self, "ImageViewer_View_ZoomFit", zoom_enabled && (fit_mode != GTH_FIT_SIZE));
	_set_action_sensitive (self, "ImageViewer_View_ZoomFitWidth", zoom_enabled && (fit_mode != GTH_FIT_WIDTH));
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
	GthFileData        *current_file;
	GdkPixbuf          *pixbuf;

	self = (GthImageViewerPage *) base;

	data = g_new0 (SaveData, 1);
	data->self = self;
	data->func = func;
	data->user_data = user_data;

	if (mime_type == NULL)
		mime_type = gth_file_data_get_mime_type (self->priv->file_data);

	current_file = gth_browser_get_current_file (self->priv->browser);
	data->original_file = gth_file_data_dup (current_file);
	if (file != NULL)
		gth_file_data_set_file (current_file, file);
	g_file_info_set_attribute_boolean (current_file->info, "gth::file::is-modified", FALSE);

	pixbuf = gth_image_viewer_get_current_pixbuf (GTH_IMAGE_VIEWER (self->priv->viewer));
	_gdk_pixbuf_save_async (pixbuf,
			        current_file,
			        mime_type,
			        TRUE,
				image_saved_cb,
				data);

	_g_object_unref (pixbuf);
}


static gboolean
gth_image_viewer_page_real_can_save (GthViewerPage *base)
{
	GArray *savers;

	savers = gth_main_get_type_set ("pixbuf-saver");
	return (savers != NULL) && (savers->len > 0);
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
} SaveAsData;


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
	GFile      *file;
	const char *mime_type;

	if (response != GTK_RESPONSE_OK) {
		if (data->func != NULL) {
			(*data->func) ((GthViewerPage *) data->self,
				       data->file_data,
				       g_error_new_literal (G_IO_ERROR, G_IO_ERROR_CANCELLED, ""),
				       data->user_data);
		}
		gtk_widget_destroy (GTK_WIDGET (file_sel));
		return;
	}

	if (! gth_file_chooser_dialog_get_file (GTH_FILE_CHOOSER_DIALOG (file_sel), &file, &mime_type))
		return;

	gth_file_data_set_file (data->file_data, file);
	_gth_image_viewer_page_real_save ((GthViewerPage *) data->self,
					  file,
					  mime_type,
					  data->func,
					  data->user_data);
	gtk_widget_destroy (GTK_WIDGET (data->file_sel));

	g_object_unref (file);
}


static void
gth_image_viewer_page_real_save_as (GthViewerPage *base,
				    FileSavedFunc  func,
				    gpointer       user_data)
{
	GthImageViewerPage *self;
	GtkWidget          *file_sel;
	char               *uri;
	SaveAsData         *data;

	self = GTH_IMAGE_VIEWER_PAGE (base);
	file_sel = gth_file_chooser_dialog_new (_("Save Image"),
						GTK_WINDOW (self->priv->browser),
						"pixbuf-saver");

	uri = g_file_get_uri (self->priv->file_data->file);
	gtk_file_chooser_set_uri (GTK_FILE_CHOOSER (file_sel), uri);

	data = g_new0 (SaveAsData, 1);
	data->self = self;
	data->func = func;
	data->file_data = gth_file_data_dup (self->priv->file_data);
	data->user_data = user_data;
	data->file_sel = file_sel;

	g_signal_connect (GTK_DIALOG (file_sel),
			  "response",
			  G_CALLBACK (save_as_response_cb),
			  data);
	g_signal_connect (G_OBJECT (file_sel),
			  "destroy",
			  G_CALLBACK (save_as_destroy_cb),
			  data);

	gtk_window_set_transient_for (GTK_WINDOW (file_sel), GTK_WINDOW (self->priv->browser));
	gtk_window_set_modal (GTK_WINDOW (file_sel), FALSE);
	gtk_widget_show (file_sel);

	g_free (uri);
}


static void
_gth_image_viewer_page_set_image (GthImageViewerPage *self,
				  cairo_surface_t    *image,
				  gboolean            modified)
{
	GthFileData *file_data;
	int          width;
	int          height;
	char        *size;

	gth_image_viewer_set_surface (GTH_IMAGE_VIEWER (self->priv->viewer), image, -1, -1);

	file_data = gth_browser_get_current_file (GTH_BROWSER (self->priv->browser));

	g_file_info_set_attribute_boolean (file_data->info, "gth::file::is-modified", modified);

	width = cairo_image_surface_get_width (image);
	height = cairo_image_surface_get_height (image);
	g_file_info_set_attribute_int32 (file_data->info, "image::width", width);
	g_file_info_set_attribute_int32 (file_data->info, "image::height", height);

	size = g_strdup_printf (_("%d × %d"), width, height);
	g_file_info_set_attribute_string (file_data->info, "general::dimensions", size);

	gth_monitor_metadata_changed (gth_main_get_default_monitor (), file_data);

	g_free (size);
}


static void
gth_image_viewer_page_real_revert (GthViewerPage *base)
{
	GthImageViewerPage *self = GTH_IMAGE_VIEWER_PAGE (base);
	GthImageData       *idata;

	idata = gth_image_history_revert (self->priv->history);
	if (idata != NULL) {
		_gth_image_viewer_page_set_image (self, idata->image, idata->unsaved);
		gth_image_data_unref (idata);
	}
}


static void
gth_image_viewer_page_real_update_info (GthViewerPage *base,
					GthFileData   *file_data)
{
	GthImageViewerPage *self = GTH_IMAGE_VIEWER_PAGE (base);

	if (! _g_file_equal (self->priv->file_data->file, file_data->file))
		return;
	_g_object_unref (self->priv->file_data);
	self->priv->file_data = gth_file_data_dup (file_data);

	if (self->priv->viewer == NULL)
		return;

	gth_image_viewer_update_view (GTH_IMAGE_VIEWER (self->priv->viewer));
}


static void
gth_image_viewer_page_real_show_properties (GthViewerPage *base,
					    gboolean       show)
{
	GthImageViewerPage *self;

	self = GTH_IMAGE_VIEWER_PAGE (base);

	if (show)
		gth_image_viewer_add_painter (GTH_IMAGE_VIEWER (self->priv->viewer), paint_comment_over_image_func, self);
	else
		gth_image_viewer_remove_painter (GTH_IMAGE_VIEWER (self->priv->viewer), paint_comment_over_image_func, self);
	gth_image_viewer_update_view (GTH_IMAGE_VIEWER (self->priv->viewer));
}


static void
gth_image_viewer_page_finalize (GObject *obj)
{
	GthImageViewerPage *self;

	self = GTH_IMAGE_VIEWER_PAGE (obj);

	g_object_unref (self->priv->history);
	_g_object_unref (self->priv->file_data);
	_g_object_unref (self->priv->last_loaded);

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
	iface->focus = gth_image_viewer_page_real_focus;
	iface->fullscreen = gth_image_viewer_page_real_fullscreen;
	iface->show_pointer = gth_image_viewer_page_real_show_pointer;
	iface->update_sensitivity = gth_image_viewer_page_real_update_sensitivity;
	iface->can_save = gth_image_viewer_page_real_can_save;
	iface->save = gth_image_viewer_page_real_save;
	iface->save_as = gth_image_viewer_page_real_save_as;
	iface->revert = gth_image_viewer_page_real_revert;
	iface->update_info = gth_image_viewer_page_real_update_info;
	iface->show_properties = gth_image_viewer_page_real_show_properties;
}


static void
gth_image_viewer_page_instance_init (GthImageViewerPage *self)
{
	self->priv = GTH_IMAGE_VIEWER_PAGE_GET_PRIVATE (self);
	self->priv->history = gth_image_history_new ();
	self->priv->shrink_wrap = FALSE;
	self->priv->last_loaded = NULL;
	self->priv->image_changed = FALSE;
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


void
gth_image_viewer_page_set_pixbuf (GthImageViewerPage *self,
				  GdkPixbuf          *pixbuf,
				  gboolean            add_to_history)
{
	cairo_surface_t *image;

	image = _cairo_image_surface_create_from_pixbuf (pixbuf);
	gth_image_viewer_page_set_image (self, image, add_to_history);

	cairo_surface_destroy (image);
}


cairo_surface_t *
gth_image_viewer_page_get_image (GthImageViewerPage *self)
{
	return gth_image_viewer_get_current_image (GTH_IMAGE_VIEWER (self->priv->viewer));
}


void
gth_image_viewer_page_set_image (GthImageViewerPage *self,
			 	 cairo_surface_t    *image,
			 	 gboolean            add_to_history)
{
	if (add_to_history)
		gth_image_history_add_image (self->priv->history, image, TRUE);
	_gth_image_viewer_page_set_image (self, image, TRUE);
	self->priv->image_changed = TRUE;
}


void
gth_image_viewer_page_undo (GthImageViewerPage *self)
{
	GthImageData *idata;

	idata = gth_image_history_undo (self->priv->history);
	if (idata != NULL)
		_gth_image_viewer_page_set_image (self, idata->image, idata->unsaved);
}


void
gth_image_viewer_page_redo (GthImageViewerPage *self)
{
	GthImageData *idata;

	idata = gth_image_history_redo (self->priv->history);
	if (idata != NULL)
		_gth_image_viewer_page_set_image (self, idata->image, idata->unsaved);
}


GthImageHistory *
gth_image_viewer_page_get_history (GthImageViewerPage *self)
{
	return self->priv->history;
}


void
gth_image_viewer_page_reset (GthImageViewerPage *self)
{
	GthImageData *last_image;

	last_image = gth_image_history_get_last (self->priv->history);
	if (last_image == NULL)
		return;

	_gth_image_viewer_page_set_image (self, last_image->image, last_image->unsaved);
}


static int
add_non_content_width (GthImageViewerPage *self,
		       GtkWidget          *non_content)
{
	int width = 0;

	if ((non_content != NULL) && gtk_widget_get_visible (non_content)) {
		GtkAllocation allocation;

		gtk_widget_get_allocation (non_content, &allocation);
		width = allocation.width;
	}

	return width;
}


static int
add_non_content_height (GthImageViewerPage *self,
			GtkWidget          *non_content)
{
	int height = 0;

	if ((non_content != NULL) && gtk_widget_get_visible (non_content)) {
		GtkAllocation allocation;

		gtk_widget_get_allocation (non_content, &allocation);
		height = allocation.height;
	}

	return height;
}


void
gth_image_viewer_page_shrink_wrap (GthImageViewerPage *self,
				   gboolean            activate)
{
	GthFileData *file_data;
	int          width;
	int          height;
	double       ratio;
	int          other_width;
	int          other_height;
	GdkScreen   *screen;
	int          max_width;
	int          max_height;

	self->priv->shrink_wrap = activate;
	if (! self->priv->shrink_wrap) {
		int width;
		int height;

		if (gth_window_get_page_size (GTH_WINDOW (self->priv->browser),
					      GTH_BROWSER_PAGE_BROWSER,
					      &width,
					      &height))
		{
			gth_window_save_page_size (GTH_WINDOW (self->priv->browser), GTH_BROWSER_PAGE_VIEWER, width, height);
			gth_window_apply_saved_size (GTH_WINDOW (self->priv->browser), GTH_BROWSER_PAGE_VIEWER);
		}
		else
			gth_window_clear_saved_size (GTH_WINDOW (self->priv->browser), GTH_BROWSER_PAGE_VIEWER);
		gth_image_viewer_set_fit_mode (GTH_IMAGE_VIEWER (self->priv->viewer), GTH_FIT_SIZE_IF_LARGER);

		return;
	}

	file_data = gth_browser_get_current_file (self->priv->browser);
	if (file_data == NULL)
		return;

	gth_image_viewer_get_original_size (GTH_IMAGE_VIEWER (self->priv->viewer), &width, &height);
	if ((width <= 0) || (height <= 0))
		return;

	ratio = (double) width / height;

	other_width = 0;
	other_height = 0;
	other_height += add_non_content_height (self, gth_window_get_area (GTH_WINDOW (self->priv->browser), GTH_WINDOW_MENUBAR));
	other_height += add_non_content_height (self, gth_window_get_area (GTH_WINDOW (self->priv->browser), GTH_WINDOW_TOOLBAR));
	other_height += add_non_content_height (self, gth_window_get_area (GTH_WINDOW (self->priv->browser), GTH_WINDOW_STATUSBAR));
	other_height += add_non_content_height (self, gth_browser_get_viewer_toolbar (self->priv->browser));
	if (eel_gconf_get_enum (PREF_UI_VIEWER_THUMBNAILS_ORIENT, GTK_TYPE_ORIENTATION, GTK_ORIENTATION_HORIZONTAL) == GTK_ORIENTATION_HORIZONTAL)
		other_height += add_non_content_height (self, gth_browser_get_thumbnail_list (self->priv->browser));
	else
		other_width += add_non_content_width (self, gth_browser_get_thumbnail_list (self->priv->browser));
	other_width += add_non_content_width (self, gth_browser_get_viewer_sidebar (self->priv->browser));
	other_width += 2;
	other_height += 2;

	screen = gtk_widget_get_screen (GTK_WIDGET (self->priv->browser));
	max_width = round ((double) gdk_screen_get_width (screen) * 8.5 / 10.0);
	max_height = round ((double) gdk_screen_get_height (screen) * 8.5 / 10.0);

	if (width + other_width > max_width) {
		width = max_width - other_width;
		height = width / ratio;
	}

	if (height + other_height > max_height) {
		height = max_height - other_height;
		width = height * ratio;
	}

	gth_window_save_page_size (GTH_WINDOW (self->priv->browser),
				   GTH_BROWSER_PAGE_VIEWER,
				   width + other_width,
				   height + other_height);
	if (gth_window_get_current_page (GTH_WINDOW (self->priv->browser)) == GTH_BROWSER_PAGE_VIEWER)
		gth_window_apply_saved_size (GTH_WINDOW (self->priv->browser), GTH_BROWSER_PAGE_VIEWER);
	gth_image_viewer_set_fit_mode (GTH_IMAGE_VIEWER (self->priv->viewer), GTH_FIT_SIZE_IF_LARGER);
}
