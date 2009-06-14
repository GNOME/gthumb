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
	GthBrowser *browser;
	GtkWidget  *viewer;
	guint       merge_id;
};


static gpointer gth_file_viewer_page_parent_class = NULL;


static void
gth_file_viewer_page_real_activate (GthViewerPage *base,
				    GthBrowser    *browser)
{
	GthFileViewerPage *self;
	GError *error = NULL;

	self = (GthFileViewerPage*) base;

	self->priv->browser = browser;

	self->priv->merge_id = gtk_ui_manager_add_ui_from_string (gth_browser_get_ui_manager (browser), file_viewer_ui_info, -1, &error);
	if (self->priv->merge_id == 0) {
		g_warning ("ui building failed: %s", error->message);
		g_error_free (error);
	}

	self->priv->viewer = gtk_label_new ("...");
	gtk_widget_show (self->priv->viewer);
	gth_browser_set_viewer_widget (browser, self->priv->viewer);
	gtk_widget_grab_focus (self->priv->viewer);
}


static void
gth_file_viewer_page_real_deactivate (GthViewerPage *base)
{
	GthFileViewerPage *self;

	self = (GthFileViewerPage*) base;

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

	self = (GthFileViewerPage*) base;
	g_return_if_fail (file_data != NULL);

	gtk_label_set_text (GTK_LABEL (self->priv->viewer), g_file_info_get_display_name (file_data->info));
}


static void
gth_file_viewer_page_real_fullscreen (GthViewerPage *base,
				      gboolean       active)
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
	iface->fullscreen = gth_file_viewer_page_real_fullscreen;
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
