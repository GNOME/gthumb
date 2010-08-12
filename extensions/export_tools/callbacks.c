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
#include <gthumb.h>
#include "callbacks.h"


static const char *ui_info =
"<ui>"
"  <popup name='ExportPopup'>"
"    <placeholder name='Web_Services'/>"
"    <separator/>"
"    <placeholder name='Misc_Actions'/>"
"  </popup>"
"</ui>";


void
export_tools__gth_browser_construct_cb (GthBrowser *browser)
{
	GError      *error = NULL;
	guint        merge_id;
	GtkToolItem *tool_item;

	g_return_if_fail (GTH_IS_BROWSER (browser));

	merge_id = gtk_ui_manager_add_ui_from_string (gth_browser_get_ui_manager (browser), ui_info, -1, &error);
	if (merge_id == 0) {
		g_warning ("building ui failed: %s", error->message);
		g_clear_error (&error);
	}

	/* export tools menu button */

	tool_item = g_object_new (GTH_TYPE_TOGGLE_MENU_TOOL_BUTTON,
				  "icon-name", "share",
				  "label", _("Share"),
				  NULL);
	/*gtk_widget_set_tooltip_text (GTK_WIDGET (tool_item), _("Export files"));*/
	gth_toggle_menu_tool_button_set_menu (GTH_TOGGLE_MENU_TOOL_BUTTON (tool_item),
					      gtk_ui_manager_get_widget (gth_browser_get_ui_manager (browser), "/ExportPopup"));
	gtk_tool_item_set_is_important (GTK_TOOL_ITEM (tool_item), TRUE);
	gtk_widget_show (GTK_WIDGET (tool_item));
	gtk_toolbar_insert (GTK_TOOLBAR (gth_browser_get_browser_toolbar (browser)), tool_item, -1);
}
