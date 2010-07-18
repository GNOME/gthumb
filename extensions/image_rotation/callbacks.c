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
#include <gdk/gdkkeysyms.h>
#include <gthumb.h>
#include "actions.h"

#define BROWSER_DATA_KEY "image-rotation-browser-data"


static const char *fixed_ui_info =
"<ui>"
"  <popup name='ListToolsPopup'>"
"    <placeholder name='Tools'>"
"      <menuitem name='RotateRight' action='Tool_RotateRight'/>"
"      <menuitem name='RotateLeft' action='Tool_RotateLeft'/>"
"    </placeholder>"
"  </popup>"
"</ui>";


static GtkActionEntry action_entries[] = {
	{ "Tool_RotateRight", "object-rotate-right",
	  N_("Rotate Right"), "<control><alt>R",
	  N_("Rotate the selected images 90° to the right"),
	  G_CALLBACK (gth_browser_activate_action_tool_rotate_right) },

	{ "Tool_RotateLeft", "object-rotate-left",
	  N_("Rotate Left"), "<control><alt>L",
	  N_("Rotate the selected images 90° to the left"),
	  G_CALLBACK (gth_browser_activate_action_tool_rotate_left) },
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
ir__gth_browser_construct_cb (GthBrowser *browser)
{
	BrowserData *data;
	GError      *error = NULL;

	g_return_if_fail (GTH_IS_BROWSER (browser));

	data = g_new0 (BrowserData, 1);
	data->action_group = gtk_action_group_new ("Image Rotation Actions");
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

	gtk_image_menu_item_set_always_show_image (GTK_IMAGE_MENU_ITEM (gtk_ui_manager_get_widget (gth_browser_get_ui_manager (browser), "/ListToolsPopup/Tools/RotateRight")), TRUE);
	gtk_image_menu_item_set_always_show_image (GTK_IMAGE_MENU_ITEM (gtk_ui_manager_get_widget (gth_browser_get_ui_manager (browser), "/ListToolsPopup/Tools/RotateLeft")), TRUE);

	g_object_set_data_full (G_OBJECT (browser), BROWSER_DATA_KEY, data, (GDestroyNotify) browser_data_free);
}


void
ir__gth_browser_update_sensitivity_cb (GthBrowser *browser)
{
	BrowserData *data;
	GtkAction   *action;
	int          n_selected;
	gboolean     sensitive;

	data = g_object_get_data (G_OBJECT (browser), BROWSER_DATA_KEY);
	g_return_if_fail (data != NULL);

	n_selected = gth_file_selection_get_n_selected (GTH_FILE_SELECTION (gth_browser_get_file_list_view (browser)));
	sensitive = n_selected > 0;

	action = gtk_action_group_get_action (data->action_group, "Tool_RotateRight");
	g_object_set (action, "sensitive", sensitive, NULL);

	action = gtk_action_group_get_action (data->action_group, "Tool_RotateLeft");
	g_object_set (action, "sensitive", sensitive, NULL);
}


gpointer
ir__gth_browser_file_list_key_press_cb (GthBrowser  *browser,
					GdkEventKey *event)
{
	gpointer result = NULL;

	switch (event->keyval) {
	case GDK_bracketright:
		gth_browser_activate_action_tool_rotate_right (NULL, browser);
		result = GINT_TO_POINTER (1);
		break;

	case GDK_bracketleft:
		gth_browser_activate_action_tool_rotate_left (NULL, browser);
		result = GINT_TO_POINTER (1);
		break;

	default:
		break;
	}

	return result;
}
