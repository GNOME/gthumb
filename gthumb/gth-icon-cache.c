/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2008 The Free Software Foundation, Inc.
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
#include "glib-utils.h"
#include "gth-icon-cache.h"
#include "gtk-utils.h"
#include "pixbuf-utils.h"


#define VOID_PIXBUF_KEY "void-pixbuf"


struct _GthIconCache {
	GtkIconTheme *icon_theme;
	int           icon_size;
	GHashTable   *cache;
};


GthIconCache *
gth_icon_cache_new (GtkIconTheme *icon_theme,
		    int           icon_size)
{
	GthIconCache *icon_cache;

	g_return_val_if_fail (icon_theme != NULL, NULL);

	icon_cache = g_new0 (GthIconCache, 1);
	icon_cache->icon_theme = icon_theme;
	icon_cache->icon_size = icon_size;
	icon_cache->cache = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, g_object_unref);

	g_hash_table_insert (icon_cache->cache, VOID_PIXBUF_KEY, create_void_pixbuf (icon_cache->icon_size, icon_cache->icon_size));

	return icon_cache;
}


GthIconCache *
gth_icon_cache_new_for_widget (GtkWidget   *widget,
	                       GtkIconSize  icon_size)
{
	GtkIconTheme *icon_theme;
	int           pixel_size;

	icon_theme = gtk_icon_theme_get_for_screen (gtk_widget_get_screen (widget));
	pixel_size = _gtk_icon_get_pixel_size (widget, GTK_ICON_SIZE_MENU);

	return gth_icon_cache_new (icon_theme, pixel_size);
}


void
gth_icon_cache_free (GthIconCache *icon_cache)
{
	if (icon_cache == NULL)
		return;
	g_hash_table_destroy (icon_cache->cache);
	g_free (icon_cache);
}


static const char *
_gth_icon_cache_get_icon_key (GIcon *icon)
{
	const char *key = NULL;

	if (G_IS_THEMED_ICON (icon)) {
		char **icon_names;
		char  *name;

		g_object_get (icon, "names", &icon_names, NULL);
		name = g_strjoinv (",", icon_names);
		key = get_static_string (name);

		g_free (name);
		g_strfreev (icon_names);
	}
	else if (G_IS_FILE_ICON (icon)) {
		GFile *file;

		file = g_file_icon_get_file (G_FILE_ICON (icon));
		if (file != NULL) {
			char *uri;

			uri = g_file_get_uri (file);
			key = get_static_string (uri);

			g_free (uri);
		}
	}

	return key;
}


GdkPixbuf *
gth_icon_cache_get_pixbuf (GthIconCache *icon_cache,
			   GIcon        *icon)
{
	const char *key;
	GdkPixbuf  *pixbuf;

	key = NULL;
	if (icon != NULL)
		key = _gth_icon_cache_get_icon_key (icon);

	if (key == NULL)
		key = VOID_PIXBUF_KEY;

	pixbuf = g_hash_table_lookup (icon_cache->cache, key);
	if (pixbuf != NULL) {
		g_object_ref (pixbuf);
		return pixbuf;
	}

	if (icon != NULL)
		pixbuf = _g_icon_get_pixbuf (icon, icon_cache->icon_size, icon_cache->icon_theme);

	if (pixbuf == NULL) {
		GIcon *unknown_icon;

		unknown_icon = g_themed_icon_new ("missing-image");
		pixbuf = _g_icon_get_pixbuf (unknown_icon, icon_cache->icon_size, icon_cache->icon_theme);

		g_object_unref (unknown_icon);
	}

	g_hash_table_insert (icon_cache->cache, (gpointer) key, g_object_ref (pixbuf));

	return pixbuf;
}
