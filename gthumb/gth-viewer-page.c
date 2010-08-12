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
#include "gth-viewer-page.h"

enum {
    FILE_LOADED,
    LAST_SIGNAL
};

static guint gth_viewer_page_signals[LAST_SIGNAL] = { 0 };


static void
gth_viewer_page_base_init (gpointer g_iface)
{
	static gboolean initialized = FALSE;

	if (initialized)
		return;

	gth_viewer_page_signals[FILE_LOADED] =
	            g_signal_new ("file-loaded",
	        		  GTH_TYPE_VIEWER_PAGE,
	                          G_SIGNAL_RUN_LAST,
	                          G_STRUCT_OFFSET (GthViewerPageIface, file_loaded),
	                          NULL, NULL,
	                          g_cclosure_marshal_VOID__BOOLEAN,
	                          G_TYPE_NONE,
	                          1,
	                          G_TYPE_BOOLEAN);

	initialized = TRUE;
}


GType
gth_viewer_page_get_type (void) {
	static GType gth_viewer_page_type_id = 0;
	if (gth_viewer_page_type_id == 0) {
		static const GTypeInfo g_define_type_info = {
			sizeof (GthViewerPageIface),
			(GBaseInitFunc) gth_viewer_page_base_init,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) NULL,
			(GClassFinalizeFunc) NULL,
			NULL,
			0,
			0,
			(GInstanceInitFunc) NULL,
			NULL
		};
		gth_viewer_page_type_id = g_type_register_static (G_TYPE_INTERFACE, "GthViewerPage", &g_define_type_info, 0);
	}
	return gth_viewer_page_type_id;
}


void
gth_viewer_page_activate (GthViewerPage *self,
			  GthBrowser    *browser)
{
	GTH_VIEWER_PAGE_GET_INTERFACE (self)->activate (self, browser);
}


void
gth_viewer_page_deactivate (GthViewerPage *self)
{
	gth_viewer_page_hide (self);
	GTH_VIEWER_PAGE_GET_INTERFACE (self)->deactivate (self);
}


void
gth_viewer_page_show (GthViewerPage  *self)
{
	GTH_VIEWER_PAGE_GET_INTERFACE (self)->show (self);
}


void
gth_viewer_page_hide (GthViewerPage  *self)
{
	GTH_VIEWER_PAGE_GET_INTERFACE (self)->hide (self);
}


gboolean
gth_viewer_page_can_view (GthViewerPage *self,
			  GthFileData   *file_data)
{
	return GTH_VIEWER_PAGE_GET_INTERFACE (self)->can_view (self, file_data);
}


void
gth_viewer_page_view (GthViewerPage *self,
		      GthFileData   *file_data)
{
	GTH_VIEWER_PAGE_GET_INTERFACE (self)->view (self, file_data);
}


void
gth_viewer_page_focus (GthViewerPage  *self)
{
	GTH_VIEWER_PAGE_GET_INTERFACE (self)->focus (self);
}


void
gth_viewer_page_fullscreen (GthViewerPage *self,
			    gboolean       active)
{
	GTH_VIEWER_PAGE_GET_INTERFACE (self)->fullscreen (self, active);
}


void
gth_viewer_page_show_pointer (GthViewerPage *self,
			     gboolean        show)
{
	GTH_VIEWER_PAGE_GET_INTERFACE (self)->show_pointer (self, show);
}


void
gth_viewer_page_update_sensitivity (GthViewerPage *self)
{
	GTH_VIEWER_PAGE_GET_INTERFACE (self)->update_sensitivity (self);
}


gboolean
gth_viewer_page_can_save (GthViewerPage *self)
{
	if (self == NULL)
		return FALSE;
	if (GTH_VIEWER_PAGE_GET_INTERFACE (self)->can_save != NULL)
		return GTH_VIEWER_PAGE_GET_INTERFACE (self)->can_save (self);
	else
		return FALSE;
}


void
gth_viewer_page_save (GthViewerPage *self,
		      GFile         *file,
		      FileSavedFunc  func,
		      gpointer       data)
{
	GTH_VIEWER_PAGE_GET_INTERFACE (self)->save (self, file, func, data);
}


void
gth_viewer_page_save_as (GthViewerPage  *self,
			 FileSavedFunc   func,
			 gpointer        data)
{
	GTH_VIEWER_PAGE_GET_INTERFACE (self)->save_as (self, func, data);
}


void
gth_viewer_page_revert (GthViewerPage *self)
{
	GTH_VIEWER_PAGE_GET_INTERFACE (self)->revert (self);
}


void
gth_viewer_page_file_loaded (GthViewerPage *self,
			     gboolean       success)
{
	g_signal_emit (self, gth_viewer_page_signals[FILE_LOADED], 0, success);
}
