/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2006 Free Software Foundation, Inc.
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


#include "gth-iviewer.h"


enum {
	SIZE_CHANGED,
	LAST_SIGNAL
};
static guint gth_iviewer_signals[LAST_SIGNAL] = { 0 };


static void
gth_iviewer_base_init (gpointer g_class)
{
	static gboolean initialized = FALSE;
  
	if (!initialized) {
		GType iface_type = G_TYPE_FROM_INTERFACE (g_class);

		gth_iviewer_signals[SIZE_CHANGED] =
			g_signal_new ("size-changed",
				      iface_type,
				      G_SIGNAL_RUN_LAST,
				      G_STRUCT_OFFSET (GthIViewerInterface, size_changed),
				      NULL, NULL,
				      g_cclosure_marshal_VOID__VOID,
				      G_TYPE_NONE, 0);
		
		initialized = TRUE;
	}
}


GType
gth_iviewer_get_type (void)
{
	static GType type = 0;
	if (type == 0) {
		static const GTypeInfo info = {
			sizeof (GthIViewerInterface),
			gth_iviewer_base_init,   /* base_init */
			NULL,   /* base_finalize */
			NULL,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			0,
			0,      /* n_preallocs */
			NULL    /* instance_init */
		};
		type = g_type_register_static (G_TYPE_INTERFACE, "GthIViewer", &info, 0);
	}
	return type;
}


double
gth_iviewer_get_zoom (GthIViewer *iviewer)
{
	return GTH_IVIEWER_GET_INTERFACE (iviewer)->get_zoom (iviewer);
}

 
void
gth_iviewer_set_zoom (GthIViewer *iviewer, 
		      double      zoom)
{
	GTH_IVIEWER_GET_INTERFACE (iviewer)->set_zoom (iviewer, zoom);
}


void
gth_iviewer_zoom_in (GthIViewer *iviewer)
{
	GTH_IVIEWER_GET_INTERFACE (iviewer)->zoom_in (iviewer);
}


void
gth_iviewer_zoom_out (GthIViewer *iviewer)
{
	GTH_IVIEWER_GET_INTERFACE (iviewer)->zoom_out (iviewer);
}


GdkPixbuf *
gth_iviewer_get_image (GthIViewer *iviewer)
{
	return GTH_IVIEWER_GET_INTERFACE (iviewer)->get_image (iviewer);
}


void
gth_iviewer_get_adjustments (GthIViewer     *self,
			     GtkAdjustment **hadj,
			     GtkAdjustment **vadj)
{
	GTH_IVIEWER_GET_INTERFACE (self)->get_adjustments (self, hadj, vadj);
}


void
gth_iviewer_size_changed (GthIViewer *self)
{
	g_signal_emit (G_OBJECT (self), 
		       gth_iviewer_signals[SIZE_CHANGED], 
		       0);
}



int
gth_iviewer_get_image_width (GthIViewer *iviewer)
{
	GdkPixbuf *image;

	image = gth_iviewer_get_image (iviewer);
	if (image == NULL)
		return 0;
	else
		return gdk_pixbuf_get_width (image);
}


int
gth_iviewer_get_image_height (GthIViewer *iviewer)
{
	GdkPixbuf *image;

	image = gth_iviewer_get_image (iviewer);
	if (image == NULL)
		return 0;
	else
		return gdk_pixbuf_get_height (image);
}


gboolean
gth_iviewer_is_void (GthIViewer *iviewer)
{
	return gth_iviewer_get_image (iviewer) == NULL;
}


void
gth_iviewer_scroll_to (GthIViewer *iviewer,
		       int         x_offset,
		       int         y_offset)
{
	GtkAdjustment *hadj = NULL, *vadj = NULL;
	
	gth_iviewer_get_adjustments (iviewer, &hadj, &vadj);

	if ((hadj == NULL) || (vadj == NULL))
		return;

	gtk_adjustment_set_value (hadj, x_offset);
	gtk_adjustment_set_value (vadj, y_offset);
}


void
gth_iviewer_get_scroll_offset (GthIViewer *iviewer,
			       int        *x,
			       int        *y)
{
	GtkAdjustment *hadj = NULL, *vadj = NULL;
	
	gth_iviewer_get_adjustments (iviewer, &hadj, &vadj);
	if (hadj != NULL)
		*x = (int) gtk_adjustment_get_value (hadj);
	if (vadj != NULL)
		*y = (int) gtk_adjustment_get_value (vadj);
}



