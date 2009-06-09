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
#include "gth-toolbox.h"


#define GTH_TOOLBOX_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GTH_TYPE_TOOLBOX, GthToolboxPrivate))


enum  {
	GTH_TOOLBOX_DUMMY_PROPERTY,
	GTH_TOOLBOX_NAME
};


static gpointer parent_class = NULL;


struct _GthToolboxPrivate {
	char      *name;
	GtkWidget *box;
};


static void
gth_toolbox_set_name (GthToolbox *self,
		      const char *name)
{
	g_free (self->priv->name);
	self->priv->name = NULL;

	if (name != NULL)
		self->priv->name = g_strdup (name);
}


static void
gth_toolbox_set_property (GObject      *object,
			  guint         property_id,
			  const GValue *value,
			  GParamSpec   *pspec)
{
	GthToolbox *self;

	self = GTH_TOOLBOX (object);

	switch (property_id) {
	case GTH_TOOLBOX_NAME:
		gth_toolbox_set_name (self, g_value_get_string (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}


static void
gth_toolbox_finalize (GObject *object)
{
	GthToolbox *self;

	self = GTH_TOOLBOX (object);

	g_free (self->priv->name);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gth_toolbox_class_init (GthToolboxClass *klass)
{
	GObjectClass *gobject_class;

	parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GthToolboxPrivate));

	gobject_class = (GObjectClass*) klass;
	gobject_class->finalize = gth_toolbox_finalize;
	gobject_class->set_property = gth_toolbox_set_property;

	g_object_class_install_property (G_OBJECT_CLASS (klass),
					 GTH_TOOLBOX_NAME,
					 g_param_spec_string ("name",
							      "name",
							      "name",
							      NULL,
							      G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
}


static void
gth_toolbox_init (GthToolbox *toolbox)
{
	GtkWidget *scrolled;

	toolbox->priv = GTH_TOOLBOX_GET_PRIVATE (toolbox);
	gtk_box_set_spacing (GTK_BOX (toolbox), 0);

	scrolled = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled), GTK_SHADOW_NONE);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	gtk_widget_show (scrolled);
	gtk_box_pack_start (GTK_BOX (toolbox), scrolled, TRUE, TRUE, 0);

	toolbox->priv->box = gtk_vbox_new (FALSE, 0);
	gtk_box_set_spacing (GTK_BOX (toolbox->priv->box), 0);
	gtk_widget_show (toolbox->priv->box);
	gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled), toolbox->priv->box);
}


GType
gth_toolbox_get_type (void)
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (GthToolboxClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_toolbox_class_init,
			NULL,
			NULL,
			sizeof (GthToolbox),
			0,
			(GInstanceInitFunc) gth_toolbox_init
		};

		type = g_type_register_static (GTK_TYPE_VBOX,
					       "GthToolbox",
					       &type_info,
					       0);
	}

	return type;
}


static void
_gth_toolbox_add_childs (GthToolbox *toolbox)
{
	GArray *children;
	int     i;

	children = gth_main_get_type_set (toolbox->priv->name);
	if (children == NULL)
		return;

	for (i = 0; i < children->len; i++) {
		GType      child_type;
		GtkWidget *child;

		child_type = g_array_index (children, GType, i);
		child = g_object_new (child_type, NULL);
		gtk_widget_show (child);
		gtk_box_pack_start (GTK_BOX (toolbox->priv->box), child, FALSE, FALSE, 0);
	}
}


GtkWidget *
gth_toolbox_new (const char *name)
{
	GtkWidget *toolbox;

	toolbox = g_object_new (GTH_TYPE_TOOLBOX, "name", name, NULL);
	_gth_toolbox_add_childs (GTH_TOOLBOX (toolbox));

	return toolbox;
}


void
gth_toolbox_update_sensitivity (GthToolbox *toolbox)
{
	GtkWidget *window;
	GList     *children;
	GList     *scan;

	window = gtk_widget_get_toplevel (GTK_WIDGET (toolbox));
	children = gtk_container_get_children (GTK_CONTAINER (toolbox->priv->box));
	for (scan = children; scan; scan = scan->next) {
		GtkWidget *child = scan->data;

		if (! GTH_IS_FILE_TOOL (child))
			continue;

		gth_file_tool_update_sensitivity (GTH_FILE_TOOL (child), window);
	}

	g_list_free (children);
}


/* -- gth_file_tool -- */


GType
gth_file_tool_get_type (void) {
	static GType gth_file_tool_type_id = 0;
	if (gth_file_tool_type_id == 0) {
		static const GTypeInfo g_define_type_info = {
			sizeof (GthFileToolIface),
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
		gth_file_tool_type_id = g_type_register_static (G_TYPE_INTERFACE, "GthFileTool", &g_define_type_info, 0);
	}
	return gth_file_tool_type_id;
}


void
gth_file_tool_update_sensitivity (GthFileTool *self,
				  GtkWidget   *window)
{
	GTH_FILE_TOOL_GET_INTERFACE (self)->update_sensitivity (self, window);
}
