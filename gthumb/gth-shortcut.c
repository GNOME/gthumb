/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2019 The Free Software Foundation, Inc.
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
#include "gth-shortcut.h"


GthShortcut *
gth_shortcut_new (void)
{
	GthShortcut *shortcut;

	shortcut = g_new (GthShortcut, 1);
	shortcut->action_name = NULL;
	shortcut->description = NULL;
	shortcut->context = 0;
	shortcut->category = 0;
	shortcut->default_accelerator = NULL;
	shortcut->accelerator = NULL;
	shortcut->label = NULL;
	shortcut->keyval = 0;
	shortcut->modifiers = 0;

	return shortcut;
}


GthShortcut *
gth_shortcut_dup (const GthShortcut *shortcut)
{
	GthShortcut *new_shortcut;

	new_shortcut = gth_shortcut_new ();
	new_shortcut->action_name = g_strdup (shortcut->action_name);
	new_shortcut->description = g_strdup (shortcut->description);
	new_shortcut->context = shortcut->context;
	new_shortcut->category = shortcut->category;
	new_shortcut->default_accelerator = g_strdup (shortcut->default_accelerator);
	gth_shortcut_set_name (new_shortcut, shortcut->accelerator);

	return new_shortcut;
}


void
gth_shortcut_free (GthShortcut *shortcut)
{
	g_free (shortcut->action_name);
	g_free (shortcut->description);
	g_free (shortcut->default_accelerator);
	g_free (shortcut->accelerator);
	g_free (shortcut->label);
	g_free (shortcut);
}


void
gth_shortcut_set_name (GthShortcut *shortcut,
		       const char  *accelerator)
{
	guint           keyval;
	GdkModifierType modifiers;

	keyval = 0;
	modifiers = 0;
	if (accelerator != NULL)
		gtk_accelerator_parse (accelerator, &keyval, &modifiers);
	gth_shortcut_set_key (shortcut, keyval, modifiers);
}


void
gth_shortcut_set_key (GthShortcut       *shortcut,
		      guint              keyval,
		      GdkModifierType    modifiers)
{
	g_free (shortcut->accelerator);
	g_free (shortcut->label);

	shortcut->keyval = keyval;
	shortcut->modifiers = modifiers;
	shortcut->accelerator = gtk_accelerator_name (shortcut->keyval, shortcut->modifiers);
	shortcut->label = gtk_accelerator_get_label (shortcut->keyval, shortcut->modifiers);
}


GthShortcut *
gth_shortcut_array_find (GPtrArray       *shortcuts_v,
			 int              context,
			 guint            keycode,
			 GdkModifierType  modifiers)
{
	int i;

	for (i = 0; i < shortcuts_v->len; i++) {
		GthShortcut *shortcut = g_ptr_array_index (shortcuts_v, i);

		if (((shortcut->context & context) == context)
			&& (shortcut->keyval == keycode)
			&& (shortcut->modifiers == modifiers))
		{
			return shortcut;
		}
	}

	return NULL;
}


gboolean
gth_shortcut_valid (guint           keycode,
		    GdkModifierType modifiers)
{
	switch (keycode) {
	case GDK_KEY_Escape:
	case GDK_KEY_Tab:
		return FALSE;

	case GDK_KEY_Left:
	case GDK_KEY_Right:
	case GDK_KEY_Up:
	case GDK_KEY_Down:
		return TRUE;

	default:
		return gtk_accelerator_valid (keycode, modifiers);
	}

	return FALSE;
}
