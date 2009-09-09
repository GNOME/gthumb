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


#define BROWSER_DATA_KEY "slideshow-browser-data"


static const char *ui_info =
"<ui>"
"  <menubar name='MenuBar'>"
"    <menu name='View' action='ViewMenu'>"
"      <placeholder name='View_Actions'>"
"        <menuitem action='View_Slideshow'/>"
"      </placeholder>"
"    </menu>"
"  </menubar>"
"</ui>";


static GtkActionEntry action_entries[] = {
	{ "View_Slideshow", NULL,
	  "_Slideshow", "F5",
	  N_("View as a slideshow"),
	  G_CALLBACK (gth_browser_activate_action_view_slideshow) }
};


typedef struct {
	GtkActionGroup *action_group;
	guint           actions_merge_id;
} BrowserData;


static void
browser_data_free (BrowserData *data)
{
	g_free (data);
}


void
ss__gth_browser_construct_cb (GthBrowser *browser)
{
	BrowserData *data;
	GError      *error = NULL;

	g_return_if_fail (GTH_IS_BROWSER (browser));

	data = g_new0 (BrowserData, 1);

	data->action_group = gtk_action_group_new ("Slideshow Action");
	gtk_action_group_set_translation_domain (data->action_group, NULL);
	gtk_action_group_add_actions (data->action_group,
				      action_entries,
				      G_N_ELEMENTS (action_entries),
				      browser);
	gtk_ui_manager_insert_action_group (gth_browser_get_ui_manager (browser), data->action_group, 0);

	data->actions_merge_id = gtk_ui_manager_add_ui_from_string (gth_browser_get_ui_manager (browser), ui_info, -1, &error);
	if (data->actions_merge_id == 0) {
		g_warning ("building menus failed: %s", error->message);
		g_error_free (error);
	}

	g_object_set_data_full (G_OBJECT (browser), BROWSER_DATA_KEY, data, (GDestroyNotify) browser_data_free);
}


static void
set_action_sensitive (BrowserData *data,
		      const char  *action_name,
		      gboolean     sensitive)
{
	GtkAction *action;

	action = gtk_action_group_get_action (data->action_group, action_name);
	g_object_set (action, "sensitive", sensitive, NULL);
}


void
ss__gth_browser_update_sensitivity_cb (GthBrowser *browser)
{
	BrowserData  *data;
	GtkTreeModel *file_store;
	gboolean      sensitive;

	data = g_object_get_data (G_OBJECT (browser), BROWSER_DATA_KEY);
	g_return_if_fail (data != NULL);

	file_store = gth_file_view_get_model (GTH_FILE_VIEW (gth_browser_get_file_list_view (browser)));
	sensitive = (gth_file_store_n_visibles (GTH_FILE_STORE (file_store)) > 0);
	set_action_sensitive (data, "View_Slideshow", sensitive);
}


void
ss__slideshow_cb (GthBrowser *browser)
{
	gth_browser_activate_action_view_slideshow (NULL, browser);
}
