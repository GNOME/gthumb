/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2003-2008 Free Software Foundation, Inc.
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
#include <gio/gio.h>
#include "gconf-utils.h"
#include "glib-utils.h"
#include "gth-enum-types.h"
#include "gth-preferences.h"

#define DIALOG_KEY_PREFIX "/apps/gthumb/dialogs/"


typedef struct {
	char *wallpaper_filename;
	char *wallpaper_options;
	char *startup_location;
} GthPreferences;


static GthPreferences *Preferences;


void
gth_pref_initialize (void)
{
	Preferences = g_new0 (GthPreferences, 1);

	Preferences->wallpaper_filename = eel_gconf_get_string ("/desktop/gnome/background/picture_filename", NULL);
	Preferences->wallpaper_options = eel_gconf_get_string ("/desktop/gnome/background/picture_options", NULL);

	Preferences->startup_location = NULL;
	if (eel_gconf_get_boolean (PREF_USE_STARTUP_LOCATION, FALSE) ||
	    eel_gconf_get_boolean (PREF_GO_TO_LAST_LOCATION, FALSE))
	{
		char *startup_location;

		startup_location = eel_gconf_get_path (PREF_STARTUP_LOCATION, NULL);
		gth_pref_set_startup_location (startup_location);

		g_free (startup_location);
	}
	else {
		char *current_dir;
		char *current_uri;

		current_dir = g_get_current_dir ();
		current_uri = g_filename_to_uri (current_dir, NULL, NULL);

		gth_pref_set_startup_location (current_uri);

		g_free (current_uri);
		g_free (current_dir);
	}

	eel_gconf_monitor_add ("/apps/gthumb");
}


void
gth_pref_release (void)
{
	g_free (Preferences->wallpaper_filename);
	g_free (Preferences->wallpaper_options);
	g_free (Preferences->startup_location);
	g_free (Preferences);
}


void
gth_pref_set_startup_location (const char *location)
{
	g_free (Preferences->startup_location);
	Preferences->startup_location = NULL;
	if (location != NULL)
		Preferences->startup_location = g_strdup (location);
}


const char *
gth_pref_get_startup_location (void)
{
	if (Preferences->startup_location != NULL)
		return Preferences->startup_location;
	else
		return get_home_uri ();
}


const char *
gth_pref_get_wallpaper_filename (void)
{
	return Preferences->wallpaper_filename;
}


const char *
gth_pref_get_wallpaper_options (void)
{
	return Preferences->wallpaper_options;
}


static void
_gth_pref_dialog_property_set_int (const char *dialog_name,
				   const char *property,
				   int         value)
{
	char *key;

	key = g_strconcat (DIALOG_KEY_PREFIX, dialog_name, "/", property, NULL);
	eel_gconf_set_integer (key, value);
	g_free (key);
}


GthToolbarStyle
gth_pref_get_real_toolbar_style (void)
{
	GthToolbarStyle toolbar_style;

	toolbar_style = eel_gconf_get_enum (PREF_UI_TOOLBAR_STYLE, GTH_TYPE_TOOLBAR_STYLE, GTH_TOOLBAR_STYLE_SYSTEM);
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


void
gth_pref_save_window_geometry (GtkWindow  *window,
			       const char *dialog_name)
{
	int x, y, width, height;

	gtk_window_get_position (window, &x, &y);
	_gth_pref_dialog_property_set_int (dialog_name, "x", x);
	_gth_pref_dialog_property_set_int (dialog_name, "y", y);

	gtk_window_get_size (window, &width, &height);
	_gth_pref_dialog_property_set_int (dialog_name, "width", width);
	_gth_pref_dialog_property_set_int (dialog_name, "height", height);
}


static int
_gth_pref_dialog_property_get_int (const char *dialog_name,
				   const char *property)
{
	char *key;
	int   value;

	key = g_strconcat (DIALOG_KEY_PREFIX, dialog_name, "/", property, NULL);
	value = eel_gconf_get_integer (key, -1);
	g_free (key);

	return value;
}


void
gth_pref_restore_window_geometry (GtkWindow  *window,
				  const char *dialog_name)
{
	int x, y, width, height;

	x = _gth_pref_dialog_property_get_int (dialog_name, "x");
	y = _gth_pref_dialog_property_get_int (dialog_name, "y");
	width = _gth_pref_dialog_property_get_int (dialog_name, "width");
	height = _gth_pref_dialog_property_get_int (dialog_name, "height");

	if ((width != -1) && (height != 1))
		gtk_window_set_default_size (window, width, height);
	if ((x != -1) && (y != 1))
		gtk_window_move (window, x, y);
	gtk_window_present (window);
}
