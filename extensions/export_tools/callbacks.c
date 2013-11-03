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
#include "export-tools.h"


void
export_tools__gth_browser_construct_cb (GthBrowser *browser)
{
	GtkBuilder *builder;
	GMenuModel *menu;
	GtkWidget  *button;

	g_return_if_fail (GTH_IS_BROWSER (browser));

	builder = gtk_builder_new_from_resource ("/org/gnome/gThumb/export_tools/data/ui/export-menu.ui");
	gth_browser_add_menu_manager_for_menu (browser, GTH_BROWSER_MENU_MANAGER_WEB_EXPORTERS, G_MENU (gtk_builder_get_object (builder, "web-exporters")));
	gth_browser_add_menu_manager_for_menu (browser, GTH_BROWSER_MENU_MANAGER_OTHER_EXPORTERS, G_MENU (gtk_builder_get_object (builder, "other-exporters")));
	menu = G_MENU_MODEL (gtk_builder_get_object (builder, "export-menu"));

	button = _gtk_menu_button_new_for_header_bar ();
	gtk_widget_set_tooltip_text (button, _("Export"));
	gtk_container_add (GTK_CONTAINER (button), gtk_image_new_from_icon_name ("export-symbolic", GTK_ICON_SIZE_MENU));
	gtk_menu_button_set_menu_model (GTK_MENU_BUTTON (button), menu);
	gtk_widget_show_all (button);
	gtk_box_pack_start (GTK_BOX (gth_browser_get_headerbar_section (browser, GTH_BROWSER_HEADER_SECTION_BROWSER_TOOLS)), button, FALSE, FALSE, 0);
}
