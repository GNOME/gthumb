/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001 The Free Software Foundation, Inc.
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
#include <math.h>
#include <gconf/gconf-client.h>
#include <libgnome/libgnome.h>
#include "typedefs.h"
#include "bookmarks.h"
#include "catalog-png-exporter.h"
#include "file-utils.h"
#include "gconf-utils.h"
#include "image-viewer.h"
#include "gthumb-init.h"
#include "preferences.h"


#define SCALE(i) (((double) i) / 65535.0)
#define UNSCALE(d) ((guint16) (65535.0 * d + 0.5))


typedef struct {
	int   i_value;
	char *s_value;
} EnumStringTable;


static int
get_enum_from_string (EnumStringTable *table,
		      const char      *s_value)
{
	int i;

	/* return the first value if s_value is invalid */
	
	if (s_value == NULL)
		return table[0].i_value; 

	for (i = 0; table[i].s_value != NULL; i++)
		if (strcmp (s_value, table[i].s_value) == 0)
			return table[i].i_value;
	
	return table[0].i_value;
}


static char *
get_string_from_enum (EnumStringTable *table,
		      int              i_value)
{
	int i;

	for (i = 0; table[i].s_value != NULL; i++)
		if (i_value == table[i].i_value)
			return table[i].s_value;
	
	/* return the first value if i_value is invalid */

	return table[0].s_value;
}


/* --------------- */


static EnumStringTable click_policy_table [] = {
	{ CLICK_POLICY_FOLLOW_NAUTILUS, "nautilus" },
	{ CLICK_POLICY_SINGLE, "single" },
        { CLICK_POLICY_DOUBLE, "double" },
	{ 0, NULL }
};

static EnumStringTable arrange_type_table [] = {
	{ SORT_NONE, "none" },
	{ SORT_BY_NAME, "name" },
        { SORT_BY_PATH, "path" },
        { SORT_BY_SIZE, "size" },
        { SORT_BY_TIME, "time" },
	{ 0, NULL }
};

static EnumStringTable *rename_sort_order_table = arrange_type_table;

static EnumStringTable *exp_arrange_type_table = arrange_type_table;

static EnumStringTable sort_order_table [] = {
	{ GTK_SORT_ASCENDING, "ascending" },
	{ GTK_SORT_DESCENDING, "descending" },
	{ 0, NULL }
};

static EnumStringTable *exp_sort_order_table = sort_order_table;

static EnumStringTable zoom_quality_table [] = {
	{ ZOOM_QUALITY_HIGH, "high" },
	{ ZOOM_QUALITY_LOW, "low" },
	{ 0, NULL }
};

static EnumStringTable zoom_change_table [] = {
	{ ZOOM_CHANGE_ACTUAL_SIZE, "actual_size" },
	{ ZOOM_CHANGE_FIT, "fit" },
	{ ZOOM_CHANGE_KEEP_PREV, "keep_prev" },
	{ ZOOM_CHANGE_FIT_IF_LARGER, "fit_if_larger" },
	{ 0, NULL }
};

static EnumStringTable transp_type_table [] = {
	{ TRANSP_TYPE_WHITE, "white" },
	{ TRANSP_TYPE_BLACK, "black" },
	{ TRANSP_TYPE_CHECKED, "checked" },
	{ TRANSP_TYPE_NONE, "none" },
	{ 0, NULL }
};

static EnumStringTable check_type_table [] = {
	{ CHECK_TYPE_LIGHT, "light" },
	{ CHECK_TYPE_MIDTONE, "midtone" },
	{ CHECK_TYPE_DARK, "dark" },
	{ 0, NULL }
};

static EnumStringTable check_size_table [] = {
	{ CHECK_SIZE_SMALL, "small" },
	{ CHECK_SIZE_MEDIUM, "medium" },
	{ CHECK_SIZE_LARGE, "large" },
	{ 0, NULL }
};

static EnumStringTable slideshow_direction_table [] = {
	{ DIRECTION_FORWARD, "forward" },
	{ DIRECTION_REVERSE, "backward" },
	{ 0, NULL }
};

static EnumStringTable toolbar_style_table [] = {
	{ TOOLBAR_STYLE_SYSTEM, "system" },
	{ TOOLBAR_STYLE_TEXT_BELOW, "text_below" },
	{ TOOLBAR_STYLE_TEXT_BESIDE, "text_beside" },
	{ TOOLBAR_STYLE_ICONS, "icons" },
	{ TOOLBAR_STYLE_TEXT, "text" },
	{ 0, NULL }
};

static EnumStringTable exporter_frame_style_table [] = {
	{ FRAME_STYLE_NONE, "none" },
	{ FRAME_STYLE_SIMPLE, "simple" },
	{ FRAME_STYLE_SIMPLE_WITH_SHADOW, "simple_with_shadow" },
	{ FRAME_STYLE_SHADOW, "shadow" },
	{ FRAME_STYLE_SLIDE, "slide" },
	{ FRAME_STYLE_SHADOW_IN, "shadow_in" },
	{ FRAME_STYLE_SHADOW_OUT, "shadow_out" },
	{ 0, NULL }
};

static EnumStringTable convert_overwrite_mode_table [] = {
	{ OVERWRITE_SKIP,      "skip" },
	{ OVERWRITE_RENAME,    "rename" },
	{ OVERWRITE_ASK,       "ask" },
	{ OVERWRITE_OVERWRITE, "overwrite" },
	{ 0, NULL }
};

/* --------------- */


void 
preferences_init ()
{
	GConfClient *client;
	char        *click_policy;

	preferences.bookmarks = bookmarks_new (RC_BOOKMARKS_FILE);
	bookmarks_load_from_disk (preferences.bookmarks);

	client = gconf_client_get_default ();
	preferences.wallpaper = gconf_client_get_string (client, "/desktop/gnome/background/picture_filename", NULL);
	preferences.wallpaperAlign = gconf_client_get_string (client, "/desktop/gnome/background/picture_options", NULL);
	click_policy = gconf_client_get_string (client, "/apps/nautilus/preferences/click_policy", NULL);
	preferences.nautilus_click_policy = ((click_policy != NULL) && (strcmp (click_policy, "single") == 0)) ? CLICK_POLICY_SINGLE : CLICK_POLICY_DOUBLE;
	g_free (click_policy);

        preferences.menus_have_tearoff = gconf_client_get_bool (client, "/desktop/gnome/interface/menus_have_tearoff", NULL);
        preferences.menus_have_icons = gconf_client_get_bool (client, "/desktop/gnome/interface/menus_have_icons", NULL);
        preferences.toolbar_detachable = gconf_client_get_bool (client, "/desktop/gnome/interface/toolbar_detachable", NULL);
        preferences.nautilus_theme = gconf_client_get_string (client, "/desktop/gnome/file_views/icon_theme", NULL);

        g_object_unref (client);
}


void
preferences_release ()
{
	if (preferences.bookmarks) {
		bookmarks_write_to_disk (preferences.bookmarks);
		bookmarks_free (preferences.bookmarks);
	}

	g_free (preferences.wallpaper);
	g_free (preferences.wallpaperAlign);

	g_free (preferences.nautilus_theme);
}





const char * 
pref_util_get_file_location (const char *location)
{
	if (strlen (location) <= FILE_PREFIX_L)
		return NULL;
	return location + FILE_PREFIX_L;
}


const char * 
pref_util_get_catalog_location (const char *location)
{
	if (strlen (location) <= CATALOG_PREFIX_L)
		return NULL;
	return location + CATALOG_PREFIX_L;
}


const char * 
pref_util_get_search_location (const char *location)
{
	if (strlen (location) <= SEARCH_PREFIX_L)
		return NULL;
	return location + SEARCH_PREFIX_L;
}


gboolean
pref_util_location_is_file (const char *location)
{
	if (location == NULL)
		return FALSE;

	if (strlen (location) <= FILE_PREFIX_L)
		return FALSE;

	return strncmp (location, FILE_PREFIX, FILE_PREFIX_L) == 0;
}


gboolean
pref_util_location_is_catalog (const char *location)
{
	if (location == NULL)
		return FALSE;

	if (strlen (location) <= CATALOG_PREFIX_L)
		return FALSE;

	return strncmp (location, CATALOG_PREFIX, CATALOG_PREFIX_L) == 0;
}


gboolean
pref_util_location_is_search (const char *location)
{
	if (location == NULL)
		return FALSE;

	if (strlen (location) <= SEARCH_PREFIX_L)
		return FALSE;

	return strncmp (location, SEARCH_PREFIX, SEARCH_PREFIX_L) == 0;
}


const char *
pref_util_remove_prefix (const char *location)
{
	if (pref_util_location_is_catalog (location))
		return pref_util_get_catalog_location (location);
	else if (pref_util_location_is_search (location))
		return pref_util_get_search_location (location);
	else if (pref_util_location_is_file (location))
		return pref_util_get_file_location (location);
	else
		return location;
}


ClickPolicy
pref_get_real_click_policy ()
{
	if (pref_get_click_policy () == CLICK_POLICY_FOLLOW_NAUTILUS)
		return preferences.nautilus_click_policy;
	else
		return pref_get_click_policy ();
}


static double
scale_round (double val, double factor)
{
	val = floor (val * factor + 0.5);
	val = MAX (val, 0);
	val = MIN (val, factor);
	return val;
}


static int
dec (char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	else if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	else if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	else
		return -1;
}


void
pref_util_get_rgb_values (const char *hex,
			  guint16    *r,
			  guint16    *g,
			  guint16    *b)
{
	int i = 1;

	*r = dec (hex[i++]) * 16 + dec (hex[i++]);
	*g = dec (hex[i++]) * 16 + dec (hex[i++]);
	*b = dec (hex[i++]) * 16 + dec (hex[i++]);

	*r = UNSCALE (*r / 255.);
	*g = UNSCALE (*g / 255.);
	*b = UNSCALE (*b / 255.);
}


const char *
pref_util_get_hex_value (guint16     r,
			 guint16     g,
			 guint16     b)
{
	static char res[1 + 3 * 2 + 1];
	static char hex[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
			     'a', 'b', 'c', 'd', 'e', 'f'};
	int         n, i = 0;

	res[i++] = '#';

	n = scale_round (SCALE (r), 255);
	res[i++] = hex[n / 16];
	res[i++] = hex[n % 16];

	n = scale_round (SCALE (g), 255);
	res[i++] = hex[n / 16];
	res[i++] = hex[n % 16];

	n = scale_round (SCALE (b), 255);
	res[i++] = hex[n / 16];
	res[i++] = hex[n % 16];

	res[i++] = 0;

	return res;
}


guint32
pref_util_get_int_value (const char *hex)
{
	guint8  r, g, b;
	int     i = 1;

	r = dec (hex[i++]) * 16 + dec (hex[i++]);
	g = dec (hex[i++]) * 16 + dec (hex[i++]);
	b = dec (hex[i++]) * 16 + dec (hex[i++]);

	return (r << 24) + (g << 16) + (b << 8) + 0xFF;
}





#define GET_SET_FUNC(func_name, pref_name, type) 			\
type									\
pref_get_##func_name ()							\
{									\
	char *s_value;							\
	char  i_value;							\
									\
	s_value = eel_gconf_get_string (pref_name);			\
	i_value = get_enum_from_string (func_name##_table, s_value);	\
	g_free (s_value);						\
									\
	return (type) i_value;						\
}									\
									\
									\
void									\
pref_set_##func_name (type i_value)					\
{									\
	char *s_value;							\
									\
	s_value = get_string_from_enum (func_name##_table, i_value);	\
	eel_gconf_set_string (pref_name, s_value);			\
}


GET_SET_FUNC(click_policy,           PREF_CLICK_POLICY,        ClickPolicy)
GET_SET_FUNC(arrange_type,           PREF_ARRANGE_IMAGES,      SortMethod)
GET_SET_FUNC(sort_order,             PREF_SORT_IMAGES,         GtkSortType)
GET_SET_FUNC(zoom_quality,           PREF_ZOOM_QUALITY,        ZoomQuality)
GET_SET_FUNC(zoom_change,            PREF_ZOOM_CHANGE,         ZoomChange)
GET_SET_FUNC(transp_type,            PREF_TRANSP_TYPE,         TranspType)
GET_SET_FUNC(check_type,             PREF_CHECK_TYPE,          CheckType)
GET_SET_FUNC(check_size,             PREF_CHECK_SIZE,          CheckSize)
GET_SET_FUNC(slideshow_direction,    PREF_SLIDESHOW_DIR,       DirectionType)
GET_SET_FUNC(toolbar_style,          PREF_UI_TOOLBAR_STYLE,    ToolbarStyle)
GET_SET_FUNC(exp_arrange_type,       PREF_EXP_ARRANGE_IMAGES,  SortMethod)
GET_SET_FUNC(exp_sort_order,         PREF_EXP_SORT_IMAGES,     GtkSortType)
GET_SET_FUNC(exporter_frame_style,   PREF_EXP_FRAME_STYLE,     FrameStyle)
GET_SET_FUNC(convert_overwrite_mode, PREF_CONVERT_OVERWRITE,   OverwriteMode)
GET_SET_FUNC(rename_sort_order,      PREF_RENAME_SERIES_SORT,  SortMethod)
