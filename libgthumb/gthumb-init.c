/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001, 2003 Free Software Foundation, Inc.
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
#include <unistd.h>
#include <libgnome/libgnome.h>

#include "catalog.h"
#include "file-utils.h"
#include "gconf-utils.h"
#include "gthumb-init.h"
#include "gthumb-stock.h"
#include "preferences.h"
#include "image-viewer.h"
#include "gth-image-list.h"
#include "typedefs.h"

Preferences preferences;


#define get_home_relative_dir(x) 	\
	g_strconcat (g_get_home_dir (),	\
		     "/",		\
		     (x),		\
		     NULL)


static void
ensure_directories_exist (void)
{
	char *path;

        /* before the gconf port this was a file, now it's folder. */
        path = get_home_relative_dir (RC_DIR);
        if (path_is_file (path))
                unlink (path);
	g_free (path);

	path = get_home_relative_dir (RC_CATALOG_DIR);
	ensure_dir_exists (path, 0700);
	g_free (path);

	path = get_home_relative_dir (RC_COMMENTS_DIR);
	ensure_dir_exists (path, 0700);
	g_free (path);
}


static void
migrate_dir_from_to (const char *from_dir,
		     const char *to_dir)
{
	char *from_path;
	char *to_path;

	from_path = get_home_relative_dir (from_dir);
	to_path = get_home_relative_dir (to_dir);

	if (path_is_dir (from_path) && ! path_is_dir (to_path)) {
		char *line;
		char *e1;
		char *e2;

		e1 = shell_escape (from_path);
		e2 = shell_escape (to_path);
		line = g_strdup_printf ("mv -f %s %s", e1, e2);
		g_free (e1);
		g_free (e2);

		g_spawn_command_line_sync (line, NULL, NULL, NULL, NULL);  
		g_free (line);
	}

	g_free (from_path);
	g_free (to_path);
}


static void
migrate_file_from_to (const char *from_file,
		      const char *to_file)
{
	char *from_path;
	char *to_path;

	from_path = get_home_relative_dir (from_file);
	to_path = get_home_relative_dir (to_file);

	if (path_is_file (from_path) && ! path_is_file (to_path)) {
		char *line;
		char *e1;
		char *e2;

		e1 = shell_escape (from_path);
		e2 = shell_escape (to_path);
		line = g_strdup_printf ("mv -f %s %s", e1, e2);
		g_free (e1);
		g_free (e2);

		g_spawn_command_line_sync (line, NULL, NULL, NULL, NULL);  
		g_free (line);
	}

	g_free (from_path);
	g_free (to_path);
}


static void
migrate_to_new_directories (void)
{
	migrate_dir_from_to (OLD_RC_CATALOG_DIR, RC_CATALOG_DIR);
	migrate_dir_from_to (OLD_RC_COMMENTS_DIR, RC_COMMENTS_DIR);
	migrate_file_from_to (OLD_RC_BOOKMARKS_FILE, RC_BOOKMARKS_FILE);
	migrate_file_from_to (OLD_RC_HISTORY_FILE, RC_HISTORY_FILE);
	migrate_file_from_to (OLD_RC_CATEGORIES_FILE, RC_CATEGORIES_FILE);

	eel_gconf_set_boolean (PREF_MIGRATE_DIRECTORIES, FALSE);
}


void
gthumb_init ()
{
	char *path;

	path = get_home_relative_dir (RC_DIR);
	ensure_dir_exists (path, 0700);
	g_free (path);

	if (eel_gconf_get_boolean (PREF_MIGRATE_DIRECTORIES, TRUE))
		migrate_to_new_directories ();

	ensure_directories_exist ();

	eel_gconf_monitor_add ("/apps/gthumb/browser");
	eel_gconf_monitor_add ("/apps/gthumb/ui");
	eel_gconf_monitor_add ("/apps/gthumb/viewer");

	eel_gconf_preload_cache ("/apps/gthumb/browser",
				 GCONF_CLIENT_PRELOAD_ONELEVEL);
	eel_gconf_preload_cache ("/apps/gthumb/ui",
				 GCONF_CLIENT_PRELOAD_ONELEVEL);
	eel_gconf_preload_cache ("/apps/gthumb/viewer",
				 GCONF_CLIENT_PRELOAD_ONELEVEL);

	preferences_init ();
	gthumb_stock_init ();
}


void
gthumb_release ()
{
	eel_gconf_monitor_remove ("/apps/gthumb/browser");
	eel_gconf_monitor_remove ("/apps/gthumb/ui");
	eel_gconf_monitor_remove ("/apps/gthumb/viewer");

	preferences_release ();
}
