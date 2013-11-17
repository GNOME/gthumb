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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <config.h>
#include <glib/gi18n.h>
#include <glib-object.h>
#include <gdk/gdkkeysyms.h>
#include <gthumb.h>
#include <extensions/image_viewer/image-viewer.h>
#include <extensions/list_tools/list-tools.h>
#include "actions.h"
#include "callbacks.h"


#define BROWSER_DATA_KEY "image-rotation-browser-data"


static const GActionEntry actions[] = {
	{ "rotate-right", gth_browser_activate_rotate_right },
	{ "rotate-left", gth_browser_activate_rotate_left },
	{ "apply-orientation", gth_browser_activate_apply_orientation },
	{ "reset-orientation", gth_browser_activate_reset_orientation }
};


static const GthMenuEntry tools1_action_entries[] = {
	{ N_("Rotate Left"), "win.rotate-left", "bracketleft", "object-rotate-left-symbolic" },
	{ N_("Rotate Right"), "win.rotate-right", "bracketright", "object-rotate-right-symbolic" },
};


static const GthMenuEntry tools2_action_entries[] = {
	{ N_("Rotate Physically"), "win.apply-orientation", NULL },
	{ N_("Reset the EXIF Orientation"), "win.reset-orientation", NULL }
};


typedef struct {
	GtkWidget *buttons[2];
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

	g_return_if_fail (GTH_IS_BROWSER (browser));

	data = g_new0 (BrowserData, 1);
	g_object_set_data_full (G_OBJECT (browser), BROWSER_DATA_KEY, data, (GDestroyNotify) browser_data_free);
	data->buttons[0] = NULL;
	data->buttons[1] = NULL;

	g_action_map_add_action_entries (G_ACTION_MAP (browser),
					 actions,
					 G_N_ELEMENTS (actions),
					 browser);
	gth_menu_manager_append_entries (gth_browser_get_menu_manager (browser, GTH_BROWSER_MENU_MANAGER_TOOLS),
					 tools1_action_entries,
					 G_N_ELEMENTS (tools1_action_entries));
	gth_menu_manager_append_entries (gth_browser_get_menu_manager (browser, GTH_BROWSER_MENU_MANAGER_MORE_TOOLS),
					 tools2_action_entries,
					 G_N_ELEMENTS (tools2_action_entries));
}


void
ir__gth_browser_update_sensitivity_cb (GthBrowser *browser)
{
	int      n_selected;
	gboolean sensitive;

	n_selected = gth_file_selection_get_n_selected (GTH_FILE_SELECTION (gth_browser_get_file_list_view (browser)));
	sensitive = n_selected > 0;
	gth_window_enable_action (GTH_WINDOW (browser), "rotate-right", sensitive);
	gth_window_enable_action (GTH_WINDOW (browser), "rotate-left", sensitive);
	gth_window_enable_action (GTH_WINDOW (browser), "apply-orientation", sensitive);
	gth_window_enable_action (GTH_WINDOW (browser), "reset-orientation", sensitive);
}


gpointer
ir__gth_browser_file_list_key_press_cb (GthBrowser  *browser,
					GdkEventKey *event)
{
	gpointer result = NULL;

	switch (event->keyval) {
	case GDK_KEY_bracketright:
		gth_browser_activate_rotate_right (NULL, NULL, browser);
		result = GINT_TO_POINTER (1);
		break;

	case GDK_KEY_bracketleft:
		gth_browser_activate_rotate_left (NULL, NULL, browser);
		result = GINT_TO_POINTER (1);
		break;

	default:
		break;
	}

	return result;
}


void
ir__gth_browser_activate_viewer_page_cb (GthBrowser *browser)
{
	BrowserData *data;
	GtkWidget   *viewer_page;

	data = g_object_get_data (G_OBJECT (browser), BROWSER_DATA_KEY);
	g_return_if_fail (data != NULL);

	viewer_page = gth_browser_get_viewer_page (browser);
	if (GTH_IS_IMAGE_VIEWER_PAGE (viewer_page)) {
		if (data->buttons[0] == NULL)
			data->buttons[0] =
					gth_browser_add_header_bar_button (browser,
									   GTH_BROWSER_HEADER_SECTION_VIEWER_COMMANDS,
									   "object-rotate-left-symbolic",
									   _("Rotate Left"),
									   "win.rotate-left",
									   NULL);
		if (data->buttons[1] == NULL)
			data->buttons[1] =
					gth_browser_add_header_bar_button (browser,
									   GTH_BROWSER_HEADER_SECTION_VIEWER_COMMANDS,
									   "object-rotate-right-symbolic",
									   _("Rotate Right"),
									   "win.rotate-right",
									   NULL);
	}
}


void
ir__gth_browser_deactivate_viewer_page_cb (GthBrowser *browser)
{
	BrowserData *data;
	GtkWidget   *viewer_page;

	data = g_object_get_data (G_OBJECT (browser), BROWSER_DATA_KEY);
	g_return_if_fail (data != NULL);

	viewer_page = gth_browser_get_viewer_page (browser);
	if (GTH_IS_IMAGE_VIEWER_PAGE (viewer_page)) {
		if (data->buttons[0] != NULL)
			gtk_widget_destroy (data->buttons[0]);
		if (data->buttons[1] != NULL)
			gtk_widget_destroy (data->buttons[1]);
		data->buttons[0] = NULL;
		data->buttons[1] = NULL;
	}
}
