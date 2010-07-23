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
#include "gth-file-tool.h"


enum {
        SHOW_OPTIONS,
        HIDE_OPTIONS,
        LAST_SIGNAL
};


static gpointer parent_class = NULL;
static guint gth_file_tool_signals[LAST_SIGNAL] = { 0 };


struct _GthFileToolPrivate {
	GtkWidget  *window;
	const char *icon_name;
	const char *button_text;
	const char *options_title;
	gboolean    separator;
};


static void
gth_file_tool_base_update_sensitivity (GthFileTool *self)
{
	/* void */
}


static void
gth_file_tool_base_activate (GthFileTool *self)
{
	/* void*/
}


static GtkWidget *
gth_file_tool_base_get_options (GthFileTool *self)
{
	return NULL;
}


static void
gth_file_tool_base_destroy_options (GthFileTool *self)
{
	/* void */
}


static void
gth_file_tool_finalize (GObject *object)
{
	GthFileTool *self;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GTH_IS_FILE_TOOL (object));

	self = (GthFileTool *) object;

	/* Chain up */
	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gth_file_tool_class_init (GthFileToolClass *klass)
{
	GObjectClass *gobject_class;

	parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GthFileToolPrivate));

	gobject_class = (GObjectClass*) klass;
	gobject_class->finalize = gth_file_tool_finalize;

	klass->update_sensitivity = gth_file_tool_base_update_sensitivity;
	klass->activate = gth_file_tool_base_activate;
	klass->get_options = gth_file_tool_base_get_options;
	klass->destroy_options = gth_file_tool_base_destroy_options;

	gth_file_tool_signals[SHOW_OPTIONS] =
	                g_signal_new ("show-options",
	                              G_TYPE_FROM_CLASS (klass),
	                              G_SIGNAL_RUN_LAST,
	                              G_STRUCT_OFFSET (GthFileToolClass, show_options),
	                              NULL, NULL,
	                              g_cclosure_marshal_VOID__VOID,
	                              G_TYPE_NONE,
	                              0);
	gth_file_tool_signals[HIDE_OPTIONS] =
			g_signal_new ("hide-options",
		                      G_TYPE_FROM_CLASS (klass),
		                      G_SIGNAL_RUN_LAST,
		                      G_STRUCT_OFFSET (GthFileToolClass, hide_options),
		                      NULL, NULL,
		                      g_cclosure_marshal_VOID__VOID,
		                      G_TYPE_NONE,
		                      0);
}


static void
gth_file_tool_instance_init (GthFileTool *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_FILE_TOOL, GthFileToolPrivate);
	self->priv->icon_name = NULL;
	self->priv->button_text = NULL;
	self->priv->options_title = NULL;

	gtk_button_set_relief (GTK_BUTTON (self), GTK_RELIEF_NONE);
}


GType
gth_file_tool_get_type (void) {
	static GType type_id = 0;
	if (type_id == 0) {
		static const GTypeInfo g_define_type_info = {
			sizeof (GthFileToolClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) gth_file_tool_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (GthFileTool),
			0,
			(GInstanceInitFunc) gth_file_tool_instance_init,
			NULL
		};
		type_id = g_type_register_static (GTK_TYPE_BUTTON, "GthFileTool", &g_define_type_info, 0);
	}
	return type_id;
}


void
gth_file_tool_construct (GthFileTool *self,
			 const char  *icon_name,
			 const char  *button_text,
			 const char  *options_title,
			 gboolean     separator)
{
	GtkWidget *hbox;
	GtkWidget *icon;
	GtkWidget *label;

	self->priv->icon_name = icon_name;
	self->priv->button_text = button_text;
	self->priv->options_title = options_title;
	self->priv->separator = separator;

	hbox = gtk_hbox_new (FALSE, 6);

	icon = gtk_image_new_from_icon_name (icon_name, GTK_ICON_SIZE_MENU);
	gtk_widget_show (icon);
	gtk_box_pack_start (GTK_BOX (hbox), icon, FALSE, FALSE, 0);

	label = gtk_label_new (button_text);
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	gtk_widget_show (hbox);
	gtk_container_add (GTK_CONTAINER (self), hbox);
}


GtkWidget *
gth_file_tool_get_window (GthFileTool *self)
{
	if (self->priv->window == NULL) {
		self->priv->window = gtk_widget_get_toplevel (GTK_WIDGET (self));
		if (! gtk_widget_is_toplevel (self->priv->window))
			self->priv->window = NULL;
	}
	return self->priv->window;
}


const char *
gth_file_tool_get_icon_name (GthFileTool *self)
{
	return self->priv->icon_name;
}


void
gth_file_tool_activate (GthFileTool *self)
{
	GTH_FILE_TOOL_GET_CLASS (self)->activate (self);
}


void
gth_file_tool_update_sensitivity (GthFileTool *self)
{
	GTH_FILE_TOOL_GET_CLASS (self)->update_sensitivity (self);
}


void
gth_file_tool_show_options (GthFileTool *self)
{
	g_signal_emit (self, gth_file_tool_signals[SHOW_OPTIONS], 0, NULL);
}


void
gth_file_tool_hide_options (GthFileTool *self)
{
	g_signal_emit (self, gth_file_tool_signals[HIDE_OPTIONS], 0, NULL);
}


GtkWidget *
gth_file_tool_get_options (GthFileTool *self)
{
	return GTH_FILE_TOOL_GET_CLASS (self)->get_options (self);
}


const char *
gth_file_tool_get_options_title (GthFileTool *self)
{
	return self->priv->options_title;
}


gboolean
gth_file_tool_has_separator (GthFileTool *self)
{
	return self->priv->separator;
}


void
gth_file_tool_destroy_options (GthFileTool *self)
{
	GTH_FILE_TOOL_GET_CLASS (self)->destroy_options (self);
}
