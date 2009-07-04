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


#define BROWSER_DATA_KEY "list-tools-browser-data"


static const char *fixed_ui_info =
"<ui>"
"  <popup name='ListToolsPopup'>"
"    <placeholder name='Tools'/>"
"    <separator/>"
"    <placeholder name='Scripts'/>"
"    <separator/>"
"    <menuitem action='ListTools_EditScripts'/>"
"  </popup>"
"</ui>";


static GtkActionEntry action_entries[] = {
	{ "ListTools_EditScripts", GTK_STOCK_EDIT,
	  N_("Personalize..."), NULL,
	  NULL,
	  G_CALLBACK (gth_browser_action_list_tools_edit_scripts) }
};


typedef struct {
	GtkToolItem    *tool_item;
	GtkActionGroup *action_group;
} BrowserData;


static void
browser_data_free (BrowserData *data)
{
	g_free (data);
}


void
list_tools__gth_browser_construct_cb (GthBrowser *browser)
{
	BrowserData *data;
	GError      *error = NULL;

	g_return_if_fail (GTH_IS_BROWSER (browser));

	data = g_new0 (BrowserData, 1);

	data->action_group = gtk_action_group_new ("List Tools Manager Actions");
	gtk_action_group_set_translation_domain (data->action_group, NULL);
	gtk_action_group_add_actions (data->action_group,
				      action_entries,
				      G_N_ELEMENTS (action_entries),
				      browser);
	gtk_ui_manager_insert_action_group (gth_browser_get_ui_manager (browser), data->action_group, 0);

	if (! gtk_ui_manager_add_ui_from_string (gth_browser_get_ui_manager (browser), fixed_ui_info, -1, &error)) {
		g_message ("building menus failed: %s", error->message);
		g_clear_error (&error);
	}

	/* tools menu button */

	data->tool_item = gth_toggle_menu_tool_button_new ();
	gtk_tool_button_set_label (GTK_TOOL_BUTTON (data->tool_item), _("Tools"));
	gtk_tool_button_set_icon_name (GTK_TOOL_BUTTON (data->tool_item), GTK_STOCK_EXECUTE);
	gth_toggle_menu_tool_button_set_menu (GTH_TOGGLE_MENU_TOOL_BUTTON (data->tool_item), gtk_ui_manager_get_widget (gth_browser_get_ui_manager (browser), "/ListToolsPopup"));
	gtk_widget_show (GTK_WIDGET (data->tool_item));
	gtk_toolbar_insert (GTK_TOOLBAR (gth_browser_get_browser_toolbar (browser)), data->tool_item, -1);

	g_object_set_data_full (G_OBJECT (browser), BROWSER_DATA_KEY, data, (GDestroyNotify) browser_data_free);
}


void
list_tools__gth_browser_update_sensitivity_cb (GthBrowser *browser)
{
	/*
	BrowserData   *data;
	GthFileSource *file_source;
	int            n_selected;
	gboolean       sensitive;

	data = g_object_get_data (G_OBJECT (browser), BROWSER_DATA_KEY);
	g_return_if_fail (data != NULL);

	file_source = gth_browser_get_location_source (browser);
	n_selected = gth_file_selection_get_n_selected (GTH_FILE_SELECTION (gth_browser_get_file_list_view (browser)));

	sensitive = (n_selected > 0) && (file_source != NULL) && gth_file_source_can_cut (file_source);
	gtk_widget_set_sensitive (GTK_WIDGET (data->tool_item), sensitive);
	*/
}
