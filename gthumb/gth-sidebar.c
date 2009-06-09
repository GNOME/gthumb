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
#include <gtk/gtk.h>
#include "gth-main.h"
#include "gth-multipage.h"
#include "gth-sidebar.h"
#include "gth-toolbox.h"
#include "gtk-utils.h"


#define GTH_SIDEBAR_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GTH_TYPE_SIDEBAR, GthSidebarPrivate))


static gpointer parent_class = NULL;


struct _GthSidebarPrivate {
	GtkWidget *properties;
	GtkWidget *tools;
	GtkWidget *options;
	GtkWidget *options_icon;
	GtkWidget *options_title;
};


static void
gth_sidebar_class_init (GthSidebarClass *klass)
{
	parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GthSidebarPrivate));
}


static void
gth_sidebar_init (GthSidebar *sidebar)
{
	GtkWidget *options_box;
	GtkWidget *options_header;
	GtkWidget *header_align;

	sidebar->priv = GTH_SIDEBAR_GET_PRIVATE (sidebar);

	gtk_notebook_set_show_tabs (GTK_NOTEBOOK (sidebar), FALSE);
	gtk_notebook_set_show_border (GTK_NOTEBOOK (sidebar), FALSE);

	sidebar->priv->properties = gth_multipage_new ();
	gtk_widget_show (sidebar->priv->properties);
	gtk_notebook_append_page (GTK_NOTEBOOK (sidebar), sidebar->priv->properties, NULL);

	sidebar->priv->tools = gth_toolbox_new ("file-tools");
	gtk_widget_show (sidebar->priv->tools);
	gtk_notebook_append_page (GTK_NOTEBOOK (sidebar), sidebar->priv->tools, NULL);

	options_box = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (options_box);
	gtk_notebook_append_page (GTK_NOTEBOOK (sidebar), options_box, NULL);

	header_align = gtk_alignment_new (0.0, 0.0, 1.0, 1.0);
	gtk_alignment_set_padding (GTK_ALIGNMENT (header_align), 5, 5, 0, 0);

	options_header = gtk_hbox_new (FALSE, 6);
	gtk_widget_show (options_header);
	gtk_container_add (GTK_CONTAINER (header_align), options_header);

	sidebar->priv->options_icon = gtk_image_new ();
	gtk_widget_show (sidebar->priv->options_icon);
	gtk_box_pack_start (GTK_BOX (options_header), sidebar->priv->options_icon, FALSE, FALSE, 0);

	sidebar->priv->options_title = gtk_label_new ("");
	gtk_label_set_use_markup (GTK_LABEL (sidebar->priv->options_title), TRUE);
	gtk_widget_show (sidebar->priv->options_title);
	gtk_box_pack_start (GTK_BOX (options_header), sidebar->priv->options_title, FALSE, FALSE, 0);

	gtk_widget_show (header_align);
	gtk_box_pack_start (GTK_BOX (options_box), header_align, FALSE, FALSE, 0);

	sidebar->priv->options = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sidebar->priv->options), GTK_SHADOW_NONE);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sidebar->priv->options), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_widget_show (sidebar->priv->options);
	gtk_box_pack_start (GTK_BOX (options_box), sidebar->priv->options, TRUE, TRUE, 0);
}


GType
gth_sidebar_get_type (void)
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (GthSidebarClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_sidebar_class_init,
			NULL,
			NULL,
			sizeof (GthSidebar),
			0,
			(GInstanceInitFunc) gth_sidebar_init
		};

		type = g_type_register_static (GTK_TYPE_NOTEBOOK,
					       "GthSidebar",
					       &type_info,
					       0);
	}

	return type;
}


static void
_gth_sidebar_add_property_views (GthSidebar *sidebar)
{
	GArray *children;
	int     i;

	children = gth_main_get_type_set ("file-properties");
	for (i = 0; i < children->len; i++) {
		GType      child_type;
		GtkWidget *child;

		child_type = g_array_index (children, GType, i);
		child = g_object_new (child_type, NULL);
		gth_multipage_add_child (GTH_MULTIPAGE (sidebar->priv->properties), GTH_MULTIPAGE_CHILD (child));
	}
	gth_multipage_set_current (GTH_MULTIPAGE (sidebar->priv->properties), 0);
}


GtkWidget *
gth_sidebar_new (void)
{
	GthSidebar *sidebar;

	sidebar = g_object_new (GTH_TYPE_SIDEBAR, NULL);
	_gth_sidebar_add_property_views (sidebar);

	return (GtkWidget *) sidebar;
}


void
gth_sidebar_set_file (GthSidebar  *sidebar,
		      GthFileData *file_data)
{
	GList *children;
	GList *scan;

	children = gth_multipage_get_children (GTH_MULTIPAGE (sidebar->priv->properties));
	for (scan = children; scan; scan = scan->next) {
		GtkWidget *child = scan->data;

		if (! GTH_IS_PROPERTY_VIEW (child))
			continue;

		gth_property_view_set_file (GTH_PROPERTY_VIEW (child), file_data);
	}

	g_list_free (children);
}


void
gth_sidebar_set_options (GthSidebar *sidebar,
			 const char *icon,
			 const char *title,
			 GtkWidget  *options)
{
	_gtk_container_remove_children (GTK_CONTAINER (sidebar->priv->options), NULL, NULL);
	if (options != NULL) {
		char *markup;

		markup = g_markup_printf_escaped ("<span size='large' weight='bold'>%s</span>", title);
		gtk_label_set_markup (GTK_LABEL (sidebar->priv->options_title), markup);
		gtk_image_set_from_stock (GTK_IMAGE (sidebar->priv->options_icon), icon, GTK_ICON_SIZE_LARGE_TOOLBAR);
		gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (sidebar->priv->options), options);

		g_free (markup);
	}
}


void
gth_sidebar_update_sensitivity (GthSidebar *sidebar)
{
	gth_toolbox_update_sensitivity (GTH_TOOLBOX (sidebar->priv->tools));
}


/* -- gth_property_view -- */


GType
gth_property_view_get_type (void) {
	static GType gth_property_view_type_id = 0;
	if (gth_property_view_type_id == 0) {
		static const GTypeInfo g_define_type_info = {
			sizeof (GthPropertyViewIface),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) NULL,
			(GClassFinalizeFunc) NULL,
			NULL,
			0,
			0,
			(GInstanceInitFunc) NULL,
			NULL
		};
		gth_property_view_type_id = g_type_register_static (G_TYPE_INTERFACE, "GthPropertyView", &g_define_type_info, 0);
	}
	return gth_property_view_type_id;
}


void
gth_property_view_set_file (GthPropertyView *self,
			    GthFileData     *file_data)
{
	GTH_PROPERTY_VIEW_GET_INTERFACE (self)->set_file (self, file_data);
}
