/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001, 2003 The Free Software Foundation, Inc.
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
#include <glib.h>
#include <gmodule.h>


static struct {
	char     *module_name;
	char     *symbol_name;
} symbol_module_table [] = {
	{ "pngexporter",   "dlg_exporter" },
	{ "webexporter",   "dlg_web_exporter" },
	{ "search",        "dlg_search" },
	{ "search",        "dlg_catalog_edit_search" },
	{ "search",        "dlg_catalog_search" },
	{ "jpegtran",      "dlg_jpegtran" },
	{ "jpegtran",      "dlg_apply_jpegtran" },
	{ "jpegtran",      "dlg_apply_jpegtran_from_exif" },
	{ "duplicates",    "dlg_duplicates" },
	{ "photoimporter", "dlg_photo_importer" },
	{ NULL, NULL }
};


static struct {
	char    *module_name;
	GModule *module;
} module_table [] = {
	{ "pngexporter", NULL },
	{ "webexporter", NULL },
	{ "search", NULL },
	{ "jpegtran", NULL },
	{ "duplicates", NULL },
	{ "photoimporter", NULL },
	{ NULL, NULL }
};


static GModule *
get_module (const char *module_name)
{
	int i;

	for (i = 0; module_table[i].module_name != NULL; i++)
		if (strcmp (module_table[i].module_name, module_name) == 0) 
			break;
	
	g_assert (module_table[i].module_name != NULL);
	
	if (module_table[i].module == NULL) {
		char *filename = g_module_build_path (GTHUMB_MODULEDIR, module_table[i].module_name);
		module_table[i].module = g_module_open (filename, G_MODULE_BIND_LAZY);
		g_free (filename);
	}
	
	return module_table[i].module;
}


static const char*
get_module_name_from_symbol_name (const char *symbol_name) 
{
	int i;

	for (i = 0; symbol_module_table[i].module_name != NULL; i++)
		if (strcmp (symbol_module_table[i].symbol_name, symbol_name) == 0)
			return symbol_module_table[i].module_name;

	return NULL;
}


gboolean
gthumb_module_get (const char  *symbol_name,
		   gpointer    *symbol)
{
	const char *module_name;
	GModule    *module;

	if (! g_module_supported ())
                return FALSE;

	module_name = get_module_name_from_symbol_name (symbol_name);
	if (module_name == NULL) 
		return FALSE;

	module = get_module (module_name);
	if (module == NULL) {
		g_warning ("Error, unable to open module file '%s'\n",
                           g_module_error ());
		return FALSE;
	}

	return g_module_symbol (module, symbol_name, symbol);
}
