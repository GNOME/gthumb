/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2003 Free Software Foundation, Inc.
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
#include "file-utils.h"
#include "gconf-utils.h"
#include "image-viewer.h"
#include "gthumb-init.h"
#include "preferences.h"


#define DIALOG_PREFIX "/apps/gthumb/dialogs/"
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
	{ GTH_CLICK_POLICY_FOLLOW_NAUTILUS, "nautilus" },
	{ GTH_CLICK_POLICY_SINGLE,          "single" },
        { GTH_CLICK_POLICY_DOUBLE,          "double" },
	{ 0, NULL }
};

static EnumStringTable arrange_type_table [] = {
	{ GTH_SORT_METHOD_BY_NAME, "name" },
	{ GTH_SORT_METHOD_NONE,    "none" },
        { GTH_SORT_METHOD_BY_PATH, "path" },
        { GTH_SORT_METHOD_BY_SIZE, "size" },
        { GTH_SORT_METHOD_BY_TIME, "time" },
	{ 0, NULL }
};

static EnumStringTable *rename_sort_order_table = arrange_type_table;

static EnumStringTable *exp_arrange_type_table = arrange_type_table;

static EnumStringTable *web_album_sort_order_table = arrange_type_table;

static EnumStringTable sort_order_table [] = {
	{ GTK_SORT_ASCENDING,  "ascending" },
	{ GTK_SORT_DESCENDING, "descending" },
	{ 0, NULL }
};

static EnumStringTable *exp_sort_order_table = sort_order_table;

static EnumStringTable zoom_quality_table [] = {
	{ GTH_ZOOM_QUALITY_HIGH, "high" },
	{ GTH_ZOOM_QUALITY_LOW,  "low" },
	{ 0, NULL }
};

static EnumStringTable zoom_change_table [] = {
	{ GTH_ZOOM_CHANGE_FIT_IF_LARGER, "fit_if_larger" },
	{ GTH_ZOOM_CHANGE_ACTUAL_SIZE,   "actual_size" },
	{ GTH_ZOOM_CHANGE_FIT,           "fit" },
	{ GTH_ZOOM_CHANGE_KEEP_PREV,     "keep_prev" },
	{ 0, NULL }
};

static EnumStringTable transp_type_table [] = {
	{ GTH_TRANSP_TYPE_NONE,    "none" },
	{ GTH_TRANSP_TYPE_WHITE,   "white" },
	{ GTH_TRANSP_TYPE_BLACK,   "black" },
	{ GTH_TRANSP_TYPE_CHECKED, "checked" },
	{ 0, NULL }
};

static EnumStringTable check_type_table [] = {
	{ GTH_CHECK_TYPE_MIDTONE, "midtone" },
	{ GTH_CHECK_TYPE_LIGHT,   "light" },
	{ GTH_CHECK_TYPE_DARK,    "dark" },
	{ 0, NULL }
};

static EnumStringTable check_size_table [] = {
	{ GTH_CHECK_SIZE_MEDIUM, "medium" },
	{ GTH_CHECK_SIZE_SMALL,  "small" },
	{ GTH_CHECK_SIZE_LARGE,  "large" },
	{ 0, NULL }
};

static EnumStringTable slideshow_direction_table [] = {
	{ GTH_DIRECTION_FORWARD, "forward" },
	{ GTH_DIRECTION_REVERSE, "backward" },
	{ GTH_DIRECTION_RANDOM,  "random" },
	{ 0, NULL }
};

static EnumStringTable toolbar_style_table [] = {
	{ GTH_TOOLBAR_STYLE_SYSTEM,      "system" },
	{ GTH_TOOLBAR_STYLE_TEXT_BELOW,  "text_below" },
	{ GTH_TOOLBAR_STYLE_TEXT_BESIDE, "text_beside" },
	{ GTH_TOOLBAR_STYLE_ICONS,       "icons" },
	{ GTH_TOOLBAR_STYLE_TEXT,        "text" },
	{ 0, NULL }
};

static EnumStringTable exporter_frame_style_table [] = {
	{ GTH_FRAME_STYLE_SIMPLE_WITH_SHADOW, "simple_with_shadow" },
	{ GTH_FRAME_STYLE_NONE,               "none" },
	{ GTH_FRAME_STYLE_SIMPLE,             "simple" },
	{ GTH_FRAME_STYLE_SHADOW,             "shadow" },
	{ GTH_FRAME_STYLE_SLIDE,              "slide" },
	{ GTH_FRAME_STYLE_SHADOW_IN,          "shadow_in" },
	{ GTH_FRAME_STYLE_SHADOW_OUT,         "shadow_out" },
	{ 0, NULL }
};

static EnumStringTable convert_overwrite_mode_table [] = {
	{ GTH_OVERWRITE_RENAME,    "rename" },
	{ GTH_OVERWRITE_SKIP,      "skip" },
	{ GTH_OVERWRITE_ASK,       "ask" },
	{ GTH_OVERWRITE_OVERWRITE, "overwrite" },
	{ 0, NULL }
};

static EnumStringTable view_as_table [] = {
	{ GTH_VIEW_AS_THUMBNAILS, "thumbnails" },
	{ GTH_VIEW_AS_LIST,       "list" },
	{ 0, NULL }
};

static EnumStringTable preview_content_table [] = {
	{ GTH_PREVIEW_CONTENT_DATA,      "data" },
	{ GTH_PREVIEW_CONTENT_IMAGE,     "image" },
	{ GTH_PREVIEW_CONTENT_COMMENT,   "comment" },
	{ 0, NULL }
};

static EnumStringTable print_unit_table [] = {
	{ GTH_PRINT_UNIT_MM,     "mm" },
	{ GTH_PRINT_UNIT_IN,     "in" },
	{ 0, NULL }
};

static EnumStringTable crop_ratio_table [] = {
	{ GTH_CROP_RATIO_NONE,     "none" },
	{ GTH_CROP_RATIO_SQUARE,   "square" },
	{ GTH_CROP_RATIO_IMAGE,    "image" },
	{ GTH_CROP_RATIO_DISPLAY,  "display" },
	{ GTH_CROP_RATIO_4_3,      "4x3" },
	{ GTH_CROP_RATIO_4_6,      "4x6" },
	{ GTH_CROP_RATIO_5_7,      "5x7" },
	{ GTH_CROP_RATIO_8_10,     "8x10" },
	{ GTH_CROP_RATIO_CUSTOM,   "custom" },
	{ 0, NULL }
};

/* --------------- */


void 
preferences_init (void)
{
	GConfClient *client;
	char        *click_policy;

	preferences.bookmarks = bookmarks_new (RC_BOOKMARKS_FILE);
	bookmarks_load_from_disk (preferences.bookmarks);

	client = gconf_client_get_default ();
	preferences.wallpaper = gconf_client_get_string (client, "/desktop/gnome/background/picture_filename", NULL);
	preferences.wallpaperAlign = gconf_client_get_string (client, "/desktop/gnome/background/picture_options", NULL);
	click_policy = gconf_client_get_string (client, "/apps/nautilus/preferences/click_policy", NULL);
	preferences.nautilus_click_policy = ((click_policy != NULL) && (strcmp (click_policy, "single") == 0)) ? GTH_CLICK_POLICY_SINGLE : GTH_CLICK_POLICY_DOUBLE;
	g_free (click_policy);

        preferences.menus_have_tearoff = gconf_client_get_bool (client, "/desktop/gnome/interface/menus_have_tearoff", NULL);
        preferences.menus_have_icons = gconf_client_get_bool (client, "/desktop/gnome/interface/menus_have_icons", NULL);
        preferences.toolbar_detachable = gconf_client_get_bool (client, "/desktop/gnome/interface/toolbar_detachable", NULL);
        preferences.nautilus_theme = gconf_client_get_string (client, "/desktop/gnome/file_views/icon_theme", NULL);

        g_object_unref (client);

	preferences.startup_location = NULL;

	if (eel_gconf_get_boolean (PREF_USE_STARTUP_LOCATION, FALSE) ||
	    eel_gconf_get_boolean (PREF_GO_TO_LAST_LOCATION, FALSE))
		preferences_set_startup_location (eel_gconf_get_path (PREF_STARTUP_LOCATION, NULL));
	else {
		char *current = g_get_current_dir ();
		preferences_set_startup_location (current);
		g_free (current);
	}
}


void
preferences_release (void)
{
	if (preferences.bookmarks) {
		bookmarks_write_to_disk (preferences.bookmarks);
		bookmarks_free (preferences.bookmarks);
	}

	g_free (preferences.wallpaper);
	g_free (preferences.wallpaperAlign);

	g_free (preferences.nautilus_theme);
	g_free (preferences.startup_location);
}


void
preferences_set_startup_location (const char *location)
{
	g_free (preferences.startup_location);
	preferences.startup_location = NULL;
	if (location != NULL)
		preferences.startup_location = g_strdup (location);
}


const char *
preferences_get_startup_location (void)
{
	return preferences.startup_location;
}





const char * 
pref_util_get_file_location (const char *location)
{
	if (g_utf8_strlen (location, -1) <= FILE_PREFIX_L)
		return NULL;
	return location + FILE_PREFIX_L;
}


const char * 
pref_util_get_catalog_location (const char *location)
{
	if (g_utf8_strlen (location, -1) <= CATALOG_PREFIX_L)
		return NULL;
	return location + CATALOG_PREFIX_L;
}


const char * 
pref_util_get_search_location (const char *location)
{
	if (g_utf8_strlen (location, -1) <= SEARCH_PREFIX_L)
		return NULL;
	return location + SEARCH_PREFIX_L;
}


gboolean
pref_util_location_is_file (const char *location)
{
	if (location == NULL)
		return FALSE;

	if (g_utf8_strlen (location, -1) <= FILE_PREFIX_L)
		return FALSE;

	return strncmp (location, FILE_PREFIX, FILE_PREFIX_L) == 0;
}


gboolean
pref_util_location_is_catalog (const char *location)
{
	if (location == NULL)
		return FALSE;

	if (g_utf8_strlen (location, -1) <= CATALOG_PREFIX_L)
		return FALSE;

	return strncmp (location, CATALOG_PREFIX, CATALOG_PREFIX_L) == 0;
}


gboolean
pref_util_location_is_search (const char *location)
{
	if (location == NULL)
		return FALSE;

	if (g_utf8_strlen (location, -1) <= SEARCH_PREFIX_L)
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


GthClickPolicy
pref_get_real_click_policy (void)
{
	if (pref_get_click_policy () == GTH_CLICK_POLICY_FOLLOW_NAUTILUS)
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
	if (hex == NULL) {
		*r = 0;
		*g = 0;
		*b = 0;

	} else if (strlen (hex) != 7) {
		*r = 0;
		*g = 0;
		*b = 0;

	} else {
		*r = dec (hex[1]) * 16 + dec (hex[2]);
		*g = dec (hex[3]) * 16 + dec (hex[4]);
		*b = dec (hex[5]) * 16 + dec (hex[6]);
		
		*r = UNSCALE (*r / 255.);
		*g = UNSCALE (*g / 255.);
		*b = UNSCALE (*b / 255.);
	}
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

	g_return_val_if_fail (hex != NULL, 0);
	g_return_val_if_fail (strlen (hex) == 7, 0);

	r = dec (hex[1]) * 16 + dec (hex[2]);
	g = dec (hex[3]) * 16 + dec (hex[4]);
	b = dec (hex[5]) * 16 + dec (hex[6]);

	return (r << 24) + (g << 16) + (b << 8) + 0xFF;
}





#define GET_SET_FUNC(func_name, pref_name, type)			\
type									\
pref_get_##func_name (void)						\
{									\
	char *s_value;							\
	char  i_value;							\
									\
	s_value = eel_gconf_get_string (pref_name, 			\
                                        func_name##_table[0].s_value);	\
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


GET_SET_FUNC(click_policy,           PREF_CLICK_POLICY,        GthClickPolicy)
GET_SET_FUNC(arrange_type,           PREF_ARRANGE_IMAGES,      GthSortMethod)
GET_SET_FUNC(sort_order,             PREF_SORT_IMAGES,         GtkSortType)
GET_SET_FUNC(zoom_quality,           PREF_ZOOM_QUALITY,        GthZoomQuality)
GET_SET_FUNC(zoom_change,            PREF_ZOOM_CHANGE,         GthZoomChange)
GET_SET_FUNC(transp_type,            PREF_TRANSP_TYPE,         GthTranspType)
GET_SET_FUNC(check_type,             PREF_CHECK_TYPE,          GthCheckType)
GET_SET_FUNC(check_size,             PREF_CHECK_SIZE,          GthCheckSize)
GET_SET_FUNC(slideshow_direction,    PREF_SLIDESHOW_DIR,       GthDirectionType)
GET_SET_FUNC(toolbar_style,          PREF_UI_TOOLBAR_STYLE,    GthToolbarStyle)
GET_SET_FUNC(exp_arrange_type,       PREF_EXP_ARRANGE_IMAGES,  GthSortMethod)
GET_SET_FUNC(exp_sort_order,         PREF_EXP_SORT_IMAGES,     GtkSortType)
GET_SET_FUNC(exporter_frame_style,   PREF_EXP_FRAME_STYLE,     GthFrameStyle)
GET_SET_FUNC(convert_overwrite_mode, PREF_CONVERT_OVERWRITE,   GthOverwriteMode)
GET_SET_FUNC(rename_sort_order,      PREF_RENAME_SERIES_SORT,  GthSortMethod)
GET_SET_FUNC(web_album_sort_order,   PREF_WEB_ALBUM_SORT,      GthSortMethod)
GET_SET_FUNC(view_as,                PREF_VIEW_AS,             GthViewAs)
GET_SET_FUNC(preview_content,        PREF_PREVIEW_CONTENT,     GthPreviewContent)
GET_SET_FUNC(print_unit,             PREF_PRINT_PAPER_UNIT,    GthPrintUnit)
GET_SET_FUNC(crop_ratio,             PREF_CROP_ASPECT_RATIO,   GthCropRatio)


GthViewMode
pref_get_view_mode (void)
{
	gboolean view_filenames;
	gboolean view_comments;

	view_filenames = eel_gconf_get_boolean (PREF_SHOW_FILENAMES, FALSE);
	view_comments = eel_gconf_get_boolean (PREF_SHOW_COMMENTS, TRUE);

	if (view_filenames && view_comments)
		return GTH_VIEW_MODE_ALL;
	else if (view_filenames && ! view_comments)
		return GTH_VIEW_MODE_LABEL;
	else if (! view_filenames && view_comments)
		return GTH_VIEW_MODE_COMMENTS;
	else if (! view_filenames && ! view_comments)
		return GTH_VIEW_MODE_VOID;

	return GTH_VIEW_MODE_VOID;
}


GthToolbarStyle
pref_get_real_toolbar_style (void)
{
	GthToolbarStyle toolbar_style;

	toolbar_style = pref_get_toolbar_style();

	if (toolbar_style == GTH_TOOLBAR_STYLE_SYSTEM) {
		char *system_style;

		system_style = eel_gconf_get_string ("/desktop/gnome/interface/toolbar_style", "system");

		if (system_style == NULL)
			toolbar_style = GTH_TOOLBAR_STYLE_TEXT_BELOW;
		else if (strcmp (system_style, "both") == 0)
			toolbar_style = GTH_TOOLBAR_STYLE_TEXT_BELOW;
		else if (strcmp (system_style, "both-horiz") == 0)
			toolbar_style = GTH_TOOLBAR_STYLE_TEXT_BESIDE;
		else if (strcmp (system_style, "icons") == 0)
			toolbar_style = GTH_TOOLBAR_STYLE_ICONS;
		else if (strcmp (system_style, "text") == 0)
			toolbar_style = GTH_TOOLBAR_STYLE_TEXT;
		else
			toolbar_style = GTH_TOOLBAR_STYLE_TEXT_BELOW;

		g_free (system_style);
	}

	return toolbar_style;
}


static void
set_dialog_property_int (const char *dialog, 
			 const char *property, 
			 int         value)
{
	char *key;

	key = g_strconcat (DIALOG_PREFIX, dialog, "/", property, NULL);
	eel_gconf_set_integer (key, value);
	g_free (key);
}


void
pref_util_save_window_geometry (GtkWindow  *window,
				const char *dialog)
{
	int x, y, width, height;

	gtk_window_get_position (window, &x, &y);
	set_dialog_property_int (dialog, "x", x); 
	set_dialog_property_int (dialog, "y", y); 

	gtk_window_get_size (window, &width, &height);
	set_dialog_property_int (dialog, "width", width); 
	set_dialog_property_int (dialog, "height", height); 
}


static int
get_dialog_property_int (const char *dialog, 
			 const char *property)
{
	char *key;
	int   value;

	key = g_strconcat (DIALOG_PREFIX, dialog, "/", property, NULL);
	value = eel_gconf_get_integer (key, -1);
	g_free (key);

	return value;
}


void
pref_util_restore_window_geometry (GtkWindow  *window,
				   const char *dialog)
{
	int x, y, width, height;

	x = get_dialog_property_int (dialog, "x");
	y = get_dialog_property_int (dialog, "y");
	width = get_dialog_property_int (dialog, "width");
	height = get_dialog_property_int (dialog, "height");

	if (width != -1 && height != 1) 
		gtk_window_set_default_size (window, width, height);

	gtk_window_present (window);

	if (x != -1 && y != 1)
		gtk_window_move (window, x, y);

	gtk_window_present (window);
}
