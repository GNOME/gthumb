/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2011 Free Software Foundation, Inc.
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
#include <gtk/gtk.h>
#include "gth-toggle-menu-action.h"
#include "gth-toggle-menu-tool-button.h"


/* Properties */
enum {
        PROP_0,
        PROP_MENU
};


static gpointer parent_class = NULL;


struct _GthToggleMenuActionPrivate {
	GtkWidget *menu;
};


static void
gth_toggle_menu_action_init (GthToggleMenuAction *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_TOGGLE_MENU_ACTION, GthToggleMenuActionPrivate);
	self->priv->menu = NULL;
}



static void
gth_toggle_menu_action_set_property (GObject      *object,
				     guint         property_id,
				     const GValue *value,
				     GParamSpec   *pspec)
{
	GthToggleMenuAction *self = GTH_TOGGLE_MENU_ACTION (object);

	switch (property_id) {
	case PROP_MENU:
		if (self->priv->menu != NULL)
			g_object_unref (self->priv->menu);
		self->priv->menu = g_value_dup_object (value);
		break;

	default:
		break;
	}
}


static void
gth_toggle_menu_action_get_property (GObject    *object,
				     guint       property_id,
				     GValue     *value,
				     GParamSpec *pspec)
{
	GthToggleMenuAction *self = GTH_TOGGLE_MENU_ACTION (object);

	switch (property_id) {
	case PROP_MENU:
		g_value_set_object (value, self->priv->menu);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}


static void
gth_toggle_menu_action_class_init (GthToggleMenuActionClass *klass)
{
	GObjectClass   *object_class;
	GtkActionClass *action_class;

	parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GthToggleMenuActionPrivate));

	object_class = (GObjectClass *) klass;
	object_class->set_property = gth_toggle_menu_action_set_property;
	object_class->get_property = gth_toggle_menu_action_get_property;

	action_class = (GtkActionClass *) klass;
	action_class->toolbar_item_type = GTH_TYPE_TOGGLE_MENU_TOOL_BUTTON;

	/* properties */

	g_object_class_install_property (object_class,
					 PROP_MENU,
					 g_param_spec_object ("menu",
                                                              "Menu",
                                                              "The menu to show",
                                                              GTK_TYPE_MENU,
                                                              G_PARAM_READWRITE));
}


G_DEFINE_TYPE (GthToggleMenuAction, gth_toggle_menu_action, GTK_TYPE_TOGGLE_ACTION)


GtkWidget *
gth_toggle_menu_action_get_menu (GthToggleMenuAction *self)
{
	return self->priv->menu;
}
