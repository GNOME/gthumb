/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2009 Free Software Foundation, Inc.
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
#include <glib-object.h>
#include <gthumb.h>
#include "actions.h"
#include "dlg-photo-importer.h"


#define BROWSER_DATA_KEY "photo-importer-browser-data"


static const char *ui_info =
"<ui>"
"  <menubar name='MenuBar'>"
"    <menu name='File' action='FileMenu'>"
"      <menu name='Import' action='ImportMenu'>"
"        <placeholder name='Misc_Actions'>"
"          <menuitem action='File_ImportFromDevice'/>"
"        </placeholder>"
"      </menu>"
"    </menu>"
"  </menubar>"
"</ui>";


static GtkActionEntry action_entries[] = {
	{ "File_ImportFromDevice", NULL,
	  N_("_Removable Device..."), NULL,
	  N_("Import photos and other files from a removable device"),
	  G_CALLBACK (gth_browser_activate_action_import_files) },
};


typedef struct {
	GtkActionGroup *action_group;
} BrowserData;


static void
browser_data_free (BrowserData *data)
{
	g_free (data);
}


void
pi__gth_browser_construct_cb (GthBrowser *browser)
{
	BrowserData *data;
	GError      *error = NULL;
	guint        merge_id;

	g_return_if_fail (GTH_IS_BROWSER (browser));

	data = g_new0 (BrowserData, 1);

	data->action_group = gtk_action_group_new ("Photo Importer Actions");
	gtk_action_group_set_translation_domain (data->action_group, NULL);
	gtk_action_group_add_actions (data->action_group,
				      action_entries,
				      G_N_ELEMENTS (action_entries),
				      browser);
	gtk_ui_manager_insert_action_group (gth_browser_get_ui_manager (browser), data->action_group, 0);

	merge_id = gtk_ui_manager_add_ui_from_string (gth_browser_get_ui_manager (browser), ui_info, -1, &error);
	if (merge_id == 0) {
		g_warning ("building ui failed: %s", error->message);
		g_clear_error (&error);
	}

	g_object_set_data_full (G_OBJECT (browser), BROWSER_DATA_KEY, data, (GDestroyNotify) browser_data_free);
}


/* -- pi__import_photos_cb -- */


typedef struct {
	GthBrowser *browser;
	GFile      *source;
} ImportData;


static void
import_data_unref (gpointer user_data)
{
	ImportData *data = user_data;

	g_object_unref (data->browser);
	g_object_unref (data->source);
	g_free (data);
}


static gboolean
import_photos_idle_cb (gpointer user_data)
{
	ImportData *data = user_data;

	dlg_photo_importer (data->browser, data->source);
	return FALSE;
}


void
pi__import_photos_cb (GthBrowser *browser,
		      GFile      *source)
{
	ImportData *data;

	data = g_new0 (ImportData, 1);
	data->browser = g_object_ref (browser);
	data->source = g_object_ref (source);
	g_idle_add_full (G_PRIORITY_DEFAULT_IDLE,
			 import_photos_idle_cb,
			 data,
			 import_data_unref);
}
