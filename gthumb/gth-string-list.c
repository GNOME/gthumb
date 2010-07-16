/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001-2008 Free Software Foundation, Inc.
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

#include <string.h>
#include "glib-utils.h"
#include "gth-string-list.h"


struct _GthStringListPrivate {
	GList *list;
};

#define GTH_STRING_LIST_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GTH_TYPE_STRING_LIST, GthStringListPrivate))
static gpointer gth_string_list_parent_class = NULL;


static void
gth_string_list_finalize (GObject* obj)
{
	GthStringList *self;

	self = GTH_STRING_LIST (obj);

	_g_string_list_free (self->priv->list);

	G_OBJECT_CLASS (gth_string_list_parent_class)->finalize (obj);
}


static void
gth_string_list_class_init (GthStringListClass * klass)
{
	gth_string_list_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GthStringListPrivate));

	G_OBJECT_CLASS (klass)->finalize = gth_string_list_finalize;
}


static void gth_string_list_instance_init (GthStringList * self) {
	self->priv = GTH_STRING_LIST_GET_PRIVATE (self);
}


GType gth_string_list_get_type (void) {
	static GType gth_string_list_type_id = 0;
	if (gth_string_list_type_id == 0) {
		static const GTypeInfo g_define_type_info = {
			sizeof (GthStringListClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) gth_string_list_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (GthStringList),
			0,
			(GInstanceInitFunc) gth_string_list_instance_init,
			NULL
		};
		gth_string_list_type_id = g_type_register_static (G_TYPE_OBJECT, "GthStringList", &g_define_type_info, 0);
	}
	return gth_string_list_type_id;
}


GthStringList *
gth_string_list_new (GList *list)
{
	GthStringList *string_list;

	string_list = g_object_new (GTH_TYPE_STRING_LIST, NULL);
	string_list->priv->list = _g_string_list_dup (list);

	return string_list;
}


GthStringList *
gth_string_list_new_from_strv (char **strv)
{
	GthStringList *string_list;
	int            i;

	string_list = g_object_new (GTH_TYPE_STRING_LIST, NULL);
	if (strv != NULL) {
		for (i = 0; strv[i] != NULL; i++)
			string_list->priv->list = g_list_prepend (string_list->priv->list, g_strdup (strv[i]));
		string_list->priv->list = g_list_reverse (string_list->priv->list);
	}
	else
		string_list->priv->list = NULL;

	return string_list;
}


GthStringList *
gth_string_list_new_from_ptr_array (GPtrArray *array)
{
	GthStringList *string_list;
	int            i;

	string_list = g_object_new (GTH_TYPE_STRING_LIST, NULL);
	if (array != NULL) {
		for (i = 0; i < array->len; i++)
			string_list->priv->list = g_list_prepend (string_list->priv->list, g_strdup (g_ptr_array_index (array, i)));
		string_list->priv->list = g_list_reverse (string_list->priv->list);
	}
	else
		string_list->priv->list = NULL;

	return string_list;
}


GList *
gth_string_list_get_list (GthStringList *list)
{
	return list->priv->list;
}


char *
gth_string_list_join (GthStringList *list,
		      const char    *separator)
{
	GString *str;
	GList   *scan;

	str = g_string_new ("");
	for (scan = list->priv->list; scan; scan = scan->next) {
		if (scan != list->priv->list)
			g_string_append (str, separator);
		g_string_append (str, (char *) scan->data);
	}

	return g_string_free (str, FALSE);
}


gboolean
gth_string_list_equal (GthStringList  *list1,
		       GthStringList  *list2)
{
	GList *keys1;
	GList *keys2;
	GList *scan;

	if ((list1 == NULL) && (list2 == NULL))
		return TRUE;
	if ((list1 == NULL) || (list2 == NULL))
		return FALSE;

	keys1 = list1->priv->list;
	keys2 = list2->priv->list;

	if (g_list_length (keys1) != g_list_length (keys2))
		return FALSE;

	for (scan = keys1; scan; scan = scan->next)
		if (! g_list_find_custom (keys2, scan->data, (GCompareFunc) strcmp))
			return FALSE;

	return TRUE;
}


void
gth_string_list_append (GthStringList *list1,
			GthStringList *list2)
{
	GList *scan;

	g_return_if_fail (GTH_IS_STRING_LIST (list1));

	if (list2 == NULL)
		return;

	for (scan = list2->priv->list; scan; scan = scan->next)
		if (! g_list_find_custom (list1->priv->list, scan->data, (GCompareFunc) strcmp))
			list1->priv->list = g_list_append (list1->priv->list, g_strdup (scan->data));
}
