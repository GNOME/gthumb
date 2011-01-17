/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2010 Free Software Foundation, Inc.
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
#include <glib/gi18n.h>
#include <gthumb.h>
#include "gth-image-histogram.h"
#include "gth-image-viewer-page.h"

#define GTH_IMAGE_HISTOGRAM_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GTH_TYPE_IMAGE_HISTOGRAM, GthImageHistogramPrivate))
#define MIN_HISTOGRAM_HEIGHT 200


static gpointer parent_class = NULL;


struct _GthImageHistogramPrivate {
	GthHistogram *histogram;
	GtkWidget    *histogram_view;
};


void
gth_image_histogram_real_set_file (GthPropertyView *base,
		 		   GthFileData     *file_data)
{
	GthImageHistogram *self = GTH_IMAGE_HISTOGRAM (base);
	GdkPixbuf         *pixbuf;
	GthBrowser        *browser;
	GtkWidget         *viewer_page;

	if (file_data == NULL) {
		gth_histogram_calculate (self->priv->histogram, NULL);
		return;
	}

	browser = (GthBrowser *) gtk_widget_get_toplevel (GTK_WIDGET (base));
	if (! gtk_widget_is_toplevel (GTK_WIDGET (browser))) {
		gth_histogram_calculate (self->priv->histogram, NULL);
		return;
	}

	viewer_page = gth_browser_get_viewer_page (browser);
	if (! GTH_IS_IMAGE_VIEWER_PAGE (viewer_page)) {
		gth_histogram_calculate (self->priv->histogram, NULL);
		return;
	}

	pixbuf = gth_image_viewer_page_get_pixbuf (GTH_IMAGE_VIEWER_PAGE (viewer_page));
	gth_histogram_calculate (self->priv->histogram, pixbuf);
}


const char *
gth_image_histogram_real_get_name (GthMultipageChild *self)
{
	return _("Histogram");
}


const char *
gth_image_histogram_real_get_icon (GthMultipageChild *self)
{
	return "histogram";
}


static void
gth_image_histogram_finalize (GObject *base)
{
	GthImageHistogram *self = GTH_IMAGE_HISTOGRAM (base);

	g_object_unref (self->priv->histogram);
	G_OBJECT_CLASS (parent_class)->finalize (base);
}


static void
gth_image_histogram_class_init (GthImageHistogramClass *klass)
{
	parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GthImageHistogramPrivate));

	G_OBJECT_CLASS (klass)->finalize = gth_image_histogram_finalize;
}


static void
gth_image_histogram_init (GthImageHistogram *self)
{
	self->priv = GTH_IMAGE_HISTOGRAM_GET_PRIVATE (self);
	self->priv->histogram = gth_histogram_new ();

	gtk_box_set_spacing (GTK_BOX (self), 6);
	gtk_container_set_border_width (GTK_CONTAINER (self), 2);

	self->priv->histogram_view = gth_histogram_view_new (self->priv->histogram);
	gtk_widget_set_size_request (self->priv->histogram_view, -1, MIN_HISTOGRAM_HEIGHT);
	gtk_widget_show (self->priv->histogram_view);
	gtk_box_pack_start (GTK_BOX (self), self->priv->histogram_view, FALSE, FALSE, 0);
}


static void
gth_image_histogram_gth_multipage_child_interface_init (GthMultipageChildIface *iface)
{
	iface->get_name = gth_image_histogram_real_get_name;
	iface->get_icon = gth_image_histogram_real_get_icon;
}


static void
gth_image_histogram_gth_property_view_interface_init (GthPropertyViewIface *iface)
{
	iface->set_file = gth_image_histogram_real_set_file;
}


GType
gth_image_histogram_get_type (void)
{
	static GType type = 0;

	if (type == 0) {
		static const GTypeInfo g_define_type_info = {
			sizeof (GthImageHistogramClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) gth_image_histogram_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (GthImageHistogram),
			0,
			(GInstanceInitFunc) gth_image_histogram_init,
			NULL
		};
		static const GInterfaceInfo gth_multipage_child_info = {
			(GInterfaceInitFunc) gth_image_histogram_gth_multipage_child_interface_init,
			(GInterfaceFinalizeFunc) NULL,
			NULL
		};
		static const GInterfaceInfo gth_property_view_info = {
			(GInterfaceInitFunc) gth_image_histogram_gth_property_view_interface_init,
			(GInterfaceFinalizeFunc) NULL,
			NULL
		};
		type = g_type_register_static (GTK_TYPE_VBOX,
					       "GthImageHistogram",
					       &g_define_type_info,
					       0);
		g_type_add_interface_static (type, GTH_TYPE_MULTIPAGE_CHILD, &gth_multipage_child_info);
		g_type_add_interface_static (type, GTH_TYPE_PROPERTY_VIEW, &gth_property_view_info);
	}

	return type;
}
