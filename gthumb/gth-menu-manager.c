/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2009 The Free Software Foundation, Inc.
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
#include <gtk/gtk.h>
#include "gtk-utils.h"
#include "glib-utils.h"
#include "gth-menu-manager.h"


/* Properties */
enum {
        PROP_0,
        PROP_MENU
};

struct _GthMenuManagerPrivate {
	GMenu      *menu;
	guint       last_id;
	GHashTable *items;
};


G_DEFINE_TYPE (GthMenuManager, gth_menu_manager, G_TYPE_OBJECT)


static void
gth_menu_manager_finalize (GObject *object)
{
	GthMenuManager *self;

	self = GTH_MENU_MANAGER (object);

	_g_object_unref (self->priv->menu);
	g_hash_table_destroy (self->priv->items);

	G_OBJECT_CLASS (gth_menu_manager_parent_class)->finalize (object);
}


static void
gth_menu_manager_set_property (GObject      *object,
			       guint         property_id,
			       const GValue *value,
			       GParamSpec   *pspec)
{
	GthMenuManager *self;

	self = GTH_MENU_MANAGER (object);

	switch (property_id) {
	case PROP_MENU:
		_g_object_unref (self->priv->menu);
		self->priv->menu = g_value_dup_object (value);
		break;
	default:
		break;
	}
}


static void
gth_menu_manager_get_property (GObject    *object,
			       guint       property_id,
			       GValue     *value,
			       GParamSpec *pspec)
{
	GthMenuManager *self;

	self = GTH_MENU_MANAGER (object);

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
gth_menu_manager_class_init (GthMenuManagerClass *klass)
{
	GObjectClass *object_class;

	g_type_class_add_private (klass, sizeof (GthMenuManagerPrivate));

	object_class = (GObjectClass*) klass;
	object_class->set_property = gth_menu_manager_set_property;
	object_class->get_property = gth_menu_manager_get_property;
	object_class->finalize = gth_menu_manager_finalize;

	/* properties */

	g_object_class_install_property (object_class,
					 PROP_MENU,
					 g_param_spec_object ("menu",
                                                              "Menu",
                                                              "The menu to modify",
                                                              G_TYPE_MENU,
                                                              G_PARAM_READWRITE));
}


static void
gth_menu_manager_init (GthMenuManager *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_MENU_MANAGER, GthMenuManagerPrivate);
	self->priv->items = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, (GDestroyNotify) _g_string_list_free);
	self->priv->last_id = 1;
	self->priv->menu = NULL;
}


GthMenuManager *
gth_menu_manager_new (GMenu *menu)
{
	return g_object_new (GTH_TYPE_MENU_MANAGER, "menu", menu, NULL);
}


guint
gth_menu_manager_append_entries (GthMenuManager     *menu_manager,
				 const GthMenuEntry *entries,
				 int                 n_entries)
{
	guint  merge_id;
	GList *items;
	int    i;

	merge_id = menu_manager->priv->last_id++;
	items = NULL;
	for (i = 0; i < n_entries; i++) {
		const GthMenuEntry *entry = entries + i;
		GMenuItem          *item;

		item = g_menu_item_new (_(entry->label), entry->detailed_action);
		if (entry->accel != NULL)
			g_menu_item_set_attribute (item, "accel", "s", entry->accel, NULL);
		g_menu_append_item (menu_manager->priv->menu, item);

		items = g_list_prepend (items, g_strdup (entry->detailed_action));
	}

	items = g_list_reverse (items);
	g_hash_table_insert (menu_manager->priv->items, GINT_TO_POINTER (merge_id), items);

	return merge_id;
}


static int
_g_menu_model_get_item_position_from_action (GMenuModel *model,
					     const char *action)
{
	int i;

	for (i = 0; i < g_menu_model_get_n_items (model); i++) {
		char *item_action = NULL;

		if (g_menu_model_get_item_attribute (model,
						     i,
						     G_MENU_ATTRIBUTE_ACTION,
						     "s",
						     &item_action))
		{
			if (g_strcmp0 (item_action, action) == 0) {
				g_free (item_action);
				return i;
			}
		}

		g_free (item_action);
	}

	return -1;
}


void
gth_menu_manager_remove_entries (GthMenuManager     *menu_manager,
				 guint               merge_id)
{
	GList *items;
	GList *scan;

	items = g_hash_table_lookup (menu_manager->priv->items, GINT_TO_POINTER (merge_id));
	g_return_if_fail (items != NULL);

	for (scan = items; scan; scan = scan->next) {
		char *detailed_action = scan->data;
		int   pos;

		pos = _g_menu_model_get_item_position_from_action (G_MENU_MODEL (menu_manager->priv->menu), detailed_action);
		if (pos >= 0)
			g_menu_remove (menu_manager->priv->menu, pos);
	}

	g_hash_table_remove (menu_manager->priv->items, GINT_TO_POINTER (merge_id));
}