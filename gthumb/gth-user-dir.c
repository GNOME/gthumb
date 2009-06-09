/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2008 Free Software Foundation, Inc.
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
#include <glib/gi18n.h>
#include <glib.h>
#include "gth-user-dir.h"


static char *
gth_user_dir_get_file_va_list (GthDir      dir_type,
			       const char *first_element,
                               va_list     var_args)
{
	const char *user_dir;
	char       *config_dir;
	GString    *path;
	const char *element;
	char       *filename;

	switch (dir_type) {
	case GTH_DIR_CONFIG:
		user_dir = g_get_user_config_dir ();
		break;
	case GTH_DIR_CACHE:
		user_dir = g_get_user_cache_dir ();
		break;
	case GTH_DIR_DATA:
		user_dir = g_get_user_data_dir ();
		break;
	}

	config_dir = g_build_path (G_DIR_SEPARATOR_S,
				   user_dir,
				   NULL);
	path = g_string_new (config_dir);

	element = first_element;
  	while (element != NULL) {
  		if (element[0] != '\0') {
  			g_string_append (path, G_DIR_SEPARATOR_S);
  			g_string_append (path, element);
  		}
		element = va_arg (var_args, const char *);
	}

	filename = path->str;

	g_string_free (path, FALSE);
	g_free (config_dir);

	return filename;
}


char *
gth_user_dir_get_file (GthDir      dir_type,
		       const char *first_element,
                       ...)
{
	va_list  var_args;
	char    *filename;

	va_start (var_args, first_element);
	filename = gth_user_dir_get_file_va_list (dir_type, first_element, var_args);
	va_end (var_args);

	return filename;
}


void
gth_user_dir_make_dir_for_file (GthDir      dir_type,
			        const char *first_element,
                                ...)
{
	va_list  var_args;
	char    *config_file;
	char    *config_dir;

	va_start (var_args, first_element);
	config_file = gth_user_dir_get_file_va_list (dir_type, first_element, var_args);
	va_end (var_args);

	config_dir = g_path_get_dirname (config_file);
	g_mkdir_with_parents (config_dir, 0700);

	g_free (config_dir);
	g_free (config_file);
}
