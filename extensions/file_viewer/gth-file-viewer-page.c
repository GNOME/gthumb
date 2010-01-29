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

#include "gth-file-viewer-page.h"


#define GTH_FILE_VIEWER_PAGE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GTH_TYPE_FILE_VIEWER_PAGE, GthFileViewerPagePrivate))


static const char *file_viewer_ui_info =
"<ui>"
"  <toolbar name='ViewerToolBar'>"
"    <placeholder name='ViewerCommands'>"
"    </placeholder>"
"  </toolbar>"
"</ui>";


struct _GthFileViewerPagePrivate {
	GthBrowser     *browser;
	GtkWidget      *viewer;
	GtkWidget      *icon;
	GtkWidget      *label;
	guint           merge_id;
	GthThumbLoader *thumb_loader;
	gulong          thumb_loader_ready_event;
};


static gpointer gth_file_viewer_page_parent_class = NULL;


static gboolean
viewer_scroll_event_cb (GtkWidget 	     *widget,
			GdkEventScroll       *event,
			GthFileViewerPage   *self)
{
	return gth_browser_viewer_scroll_event_cb (self->priv->browser, event);
}


static gboolean
viewer_key_press_cb (GtkWidget         *widget,
		     GdkEventKey       *event,
		     GthFileViewerPage *self)
{
	return gth_browser_viewer_key_press_cb (self->priv->browser, event);
}


static gboolean
viewer_button_press_cb (GtkWidget         *widget,
		        GdkEventButton    *event,
		        GthFileViewerPage *self)
{
	return gth_browser_viewer_button_press_cb (self->priv->browser, event);
}


static void
thumb_loader_ready_cb (GthThumbLoader    *il,
		       GError            *error,
		       GthFileViewerPage *self)
{
	gth_viewer_page_file_loaded (GTH_VIEWER_PAGE (self));
	if (error == NULL)
		gtk_image_set_from_pixbuf (GTK_IMAGE (self->priv->icon), gth_thumb_loader_get_pixbuf (self->priv->thumb_loader));
}


static void
gth_file_viewer_page_real_activate (GthViewerPage *base,
				    GthBrowser    *browser)
{
	GthFileViewerPage *self;
	GError            *error = NULL;
	GtkWidget         *vbox1;
	GtkWidget         *vbox2;

	self = (GthFileViewerPage*) base;

	self->priv->browser = browser;

	self->priv->merge_id = gtk_ui_manager_add_ui_from_string (gth_browser_get_ui_manager (browser), file_viewer_ui_info, -1, &error);
	if (self->priv->merge_id == 0) {
		g_warning ("ui building failed: %s", error->message);
		g_error_free (error);
	}

	self->priv->thumb_loader = gth_thumb_loader_new (128);
	self->priv->thumb_loader_ready_event =
			g_signal_connect (G_OBJECT (self->priv->thumb_loader),
					  "ready",
					  G_CALLBACK (thumb_loader_ready_cb),
					  self);

	self->priv->viewer = gtk_event_box_new ();
	gtk_widget_show (self->priv->viewer);

	vbox1 = gtk_vbox_new (TRUE, 0);
	gtk_widget_show (vbox1);
	gtk_container_add (GTK_CONTAINER (self->priv->viewer), vbox1);

	vbox2 = gtk_vbox_new (FALSE, 6);
	gtk_widget_show (vbox2);
	gtk_box_pack_start (GTK_BOX (vbox1), vbox2, FALSE, FALSE, 0);

	self->priv->icon = gtk_image_new ();
	gtk_widget_show (self->priv->icon);
	gtk_box_pack_start (GTK_BOX (vbox2), self->priv->icon, FALSE, FALSE, 0);

	self->priv->label = gtk_label_new ("...");
	gtk_label_set_selectable (GTK_LABEL (self->priv->label), TRUE);
	gtk_widget_show (self->priv->label);
	gtk_box_pack_start (GTK_BOX (vbox2), self->priv->label, FALSE, FALSE, 0);

	g_signal_connect (G_OBJECT (self->priv->viewer),
			  "scroll-event",
			  G_CALLBACK (viewer_scroll_event_cb),
			  self);
	g_signal_connect (G_OBJECT (self->priv->viewer),
			  "button_press_event",
			  G_CALLBACK (viewer_button_press_cb),
			  self);
	g_signal_connect (G_OBJECT (self->priv->label),
			  "key_press_event",
			  G_CALLBACK (viewer_key_press_cb),
			  self);

	gth_browser_set_viewer_widget (browser, self->priv->viewer);
}


static void
gth_file_viewer_page_real_deactivate (GthViewerPage *base)
{
	GthFileViewerPage *self;

	self = (GthFileViewerPage*) base;

	g_signal_handler_disconnect (self->priv->thumb_loader, self->priv->thumb_loader_ready_event);
	self->priv->thumb_loader_ready_event = 0;

	if (self->priv->merge_id != 0) {
		gtk_ui_manager_remove_ui (gth_browser_get_ui_manager (self->priv->browser), self->priv->merge_id);
		self->priv->merge_id = 0;
	}
	gth_browser_set_viewer_widget (self->priv->browser, NULL);
}


static void
gth_file_viewer_page_real_show (GthViewerPage *base)
{
}


static void
gth_file_viewer_page_real_hide (GthViewerPage *base)
{
}


static gboolean
gth_file_viewer_page_real_can_view (GthViewerPage *base,
				    GthFileData   *file_data)
{
	GthFileViewerPage *self;

	self = (GthFileViewerPage*) base;
	g_return_val_if_fail (file_data != NULL, FALSE);

	return TRUE;
}


static void
gth_file_viewer_page_real_view (GthViewerPage *base,
				GthFileData   *file_data)
{
	GthFileViewerPage *self;
	GIcon             *icon;

	self = (GthFileViewerPage*) base;
	g_return_if_fail (file_data != NULL);

	gtk_label_set_text (GTK_LABEL (self->priv->label), g_file_info_get_display_name (file_data->info));
	icon = g_file_info_get_icon (file_data->info);
	if (icon != NULL)
		gtk_image_set_from_gicon (GTK_IMAGE (self->priv->icon), icon, GTK_ICON_SIZE_DIALOG);

	gth_viewer_page_focus (GTH_VIEWER_PAGE (self));
	gth_thumb_loader_set_file (self->priv->thumb_loader, file_data);
	gth_thumb_loader_load (self->priv->thumb_loader);
}


static void
gth_file_viewer_page_real_focus (GthViewerPage *base)
{
	GtkWidget *widget;

	widget = GTH_FILE_VIEWER_PAGE (base)->priv->label;
	if (GTK_WIDGET_REALIZED (widget))
		gtk_widget_grab_focus (widget);
}


static void
gth_file_viewer_page_real_fullscreen (GthViewerPage *base,
				      gboolean       active)
{
	/* void */
}


static void
gth_file_viewer_page_real_show_pointer (GthViewerPage *base,
				        gboolean       show)
{
	/* void */
}


static void
gth_file_viewer_page_real_update_sensitivity (GthViewerPage *base)
{
}


static gboolean
gth_file_viewer_page_real_can_save (GthViewerPage *base)
{
	return FALSE;
}


static void
gth_file_viewer_page_finalize (GObject *obj)
{
	GthFileViewerPage *self;

	self = GTH_FILE_VIEWER_PAGE (obj);

	_g_object_unref (self->priv->thumb_loader);

	G_OBJECT_CLASS (gth_file_viewer_page_parent_class)->finalize (obj);
}


static void
gth_file_viewer_page_class_init (GthFileViewerPageClass *klass)
{
	gth_file_viewer_page_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GthFileViewerPagePrivate));

	G_OBJECT_CLASS (klass)->finalize = gth_file_viewer_page_finalize;
}


static void
gth_viewer_page_interface_init (GthViewerPageIface *iface)
{
	iface->activate = gth_file_viewer_page_real_activate;
	iface->deactivate = gth_file_viewer_page_real_deactivate;
	iface->show = gth_file_viewer_page_real_show;
	iface->hide = gth_file_viewer_page_real_hide;
	iface->can_view = gth_file_viewer_page_real_can_view;
	iface->view = gth_file_viewer_page_real_view;
	iface->focus = gth_file_viewer_page_real_focus;
	iface->fullscreen = gth_file_viewer_page_real_fullscreen;
	iface->show_pointer = gth_file_viewer_page_real_show_pointer;
	iface->update_sensitivity = gth_file_viewer_page_real_update_sensitivity;
	iface->can_save = gth_file_viewer_page_real_can_save;
}


static void
gth_file_viewer_page_instance_init (GthFileViewerPage *self)
{
	self->priv = GTH_FILE_VIEWER_PAGE_GET_PRIVATE (self);
}


GType gth_file_viewer_page_get_type (void) {
	static GType gth_file_viewer_page_type_id = 0;
	if (gth_file_viewer_page_type_id == 0) {
		static const GTypeInfo g_define_type_info = {
			sizeof (GthFileViewerPageClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) gth_file_viewer_page_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (GthFileViewerPage),
			0,
			(GInstanceInitFunc) gth_file_viewer_page_instance_init,
			NULL
		};
		static const GInterfaceInfo gth_viewer_page_info = {
			(GInterfaceInitFunc) gth_viewer_page_interface_init,
			(GInterfaceFinalizeFunc) NULL,
			NULL
		};
		gth_file_viewer_page_type_id = g_type_register_static (G_TYPE_OBJECT, "GthFileViewerPage", &g_define_type_info, 0);
		g_type_add_interface_static (gth_file_viewer_page_type_id, GTH_TYPE_VIEWER_PAGE, &gth_viewer_page_info);
	}
	return gth_file_viewer_page_type_id;
}
