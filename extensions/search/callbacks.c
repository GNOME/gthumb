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
#include <extensions/catalogs/gth-catalog.h>
#include "actions.h"
#include "gth-search.h"
#include "gth-search-editor.h"


#define BROWSER_DATA_KEY "search-browser-data"
#define _RESPONSE_REFRESH 2


static const char *find_ui_info =
"<ui>"
"  <menubar name='MenuBar'>"
"    <menu name='Edit' action='EditMenu'>"
"      <placeholder name='Edit_Actions'>"
"        <menuitem action='Edit_Find'/>"
"      </placeholder>"
"    </menu>"
"  </menubar>"
"  <toolbar name='ToolBar'>"
"    <placeholder name='SourceCommands'>"
"      <toolitem action='Edit_Find'/>"
"    </placeholder>"
"  </toolbar>"
"</ui>";


static GtkActionEntry find_action_entries[] = {
	{ "Edit_Find", GTK_STOCK_FIND,
	  NULL, NULL,
	  N_("Find files"),
	  G_CALLBACK (gth_browser_activate_action_edit_find) }
};
static guint find_action_entries_size = G_N_ELEMENTS (find_action_entries);


static const char *search_ui_info =
"<ui>"
"  <menubar name='MenuBar'>"
"    <menu name='Edit' action='EditMenu'>"
"      <placeholder name='Edit_Actions'>"
"        <menuitem action='Edit_Search_Edit'/>"
"        <menuitem action='Edit_Search_Update'/>"
"      </placeholder>"
"    </menu>"
"  </menubar>"
"  <toolbar name='ToolBar'>"
"    <placeholder name='SourceCommands'>"
"      <toolitem action='Edit_Search_Edit'/>"
"      <toolitem action='Edit_Search_Update'/>"
"    </placeholder>"
"  </toolbar>"
"</ui>";


static GtkActionEntry search_actions_entries[] = {
	{ "Edit_Search_Edit", GTK_STOCK_FIND_AND_REPLACE,
	  N_("Edit Search"), "<ctrl>F",
	  N_("Edit search criteria"),
	  G_CALLBACK (gth_browser_activate_action_edit_search_edit) },
	{ "Edit_Search_Update", GTK_STOCK_REFRESH,
	  N_("Redo Search"), "<shift><ctrl>R",
	  N_("Update search results"),
	  G_CALLBACK (gth_browser_activate_action_edit_search_update) }
};
static guint search_actions_entries_size = G_N_ELEMENTS (search_actions_entries);


typedef struct {
	GtkActionGroup *find_action;
	guint           find_merge_id;
	GtkActionGroup *search_actions;
	guint           search_merge_id;
	GtkWidget      *refresh_button;
} BrowserData;


static void
browser_data_free (BrowserData *data)
{
	g_free (data);
}


void
search__gth_browser_construct_cb (GthBrowser *browser)
{
	BrowserData *data;

	g_return_if_fail (GTH_IS_BROWSER (browser));

	data = g_new0 (BrowserData, 1);

	data->find_action = gtk_action_group_new ("Find Action");
	gtk_action_group_set_translation_domain (data->find_action, NULL);
	gtk_action_group_add_actions (data->find_action,
				      find_action_entries,
				      find_action_entries_size,
				      browser);
	gtk_ui_manager_insert_action_group (gth_browser_get_ui_manager (browser), data->find_action, 0);

	data->search_actions = gtk_action_group_new ("Search Actions");
	gtk_action_group_set_translation_domain (data->search_actions, NULL);
	gtk_action_group_add_actions (data->search_actions,
				      search_actions_entries,
				      search_actions_entries_size,
				      browser);
	gtk_ui_manager_insert_action_group (gth_browser_get_ui_manager (browser), data->search_actions, 0);

	g_object_set_data_full (G_OBJECT (browser), BROWSER_DATA_KEY, data, (GDestroyNotify) browser_data_free);
}


static void
refresh_button_clicked_cb (GtkButton  *button,
			   GthBrowser *browser)
{
	gth_browser_activate_action_edit_search_update (NULL, browser);
}


void
search__gth_browser_load_location_after_cb (GthBrowser   *browser,
					    GthFileData  *location_data,
					    const GError *error)
{
	BrowserData *data;

	if ((location_data == NULL) || (error != NULL))
		return;

	data = g_object_get_data (G_OBJECT (browser), BROWSER_DATA_KEY);

	if (_g_content_type_is_a (g_file_info_get_content_type (location_data->info), "gthumb/search")) {
		if (data->find_merge_id != 0) {
			gtk_ui_manager_remove_ui (gth_browser_get_ui_manager (browser), data->find_merge_id);
			data->find_merge_id = 0;
		}
		if (data->search_merge_id == 0) {
			GError *local_error = NULL;

			data->search_merge_id = gtk_ui_manager_add_ui_from_string (gth_browser_get_ui_manager (browser), search_ui_info, -1, &local_error);
			if (data->search_merge_id == 0) {
				g_warning ("building menus failed: %s", local_error->message);
				g_error_free (local_error);
			}
			/*gtk_tool_item_set_is_important (GTK_TOOL_ITEM (gtk_ui_manager_get_widget (gth_browser_get_ui_manager (browser), "/ToolBar/SourceCommands/Edit_Search_Update")), TRUE);*/
			gtk_tool_item_set_is_important (GTK_TOOL_ITEM (gtk_ui_manager_get_widget (gth_browser_get_ui_manager (browser), "/ToolBar/SourceCommands/Edit_Search_Edit")), TRUE);
		}

		if (data->refresh_button == NULL) {
			data->refresh_button = gtk_button_new_from_stock (GTK_STOCK_REFRESH);
			g_object_add_weak_pointer (G_OBJECT (data->refresh_button), (gpointer *)&data->refresh_button);
			gtk_button_set_relief (GTK_BUTTON (data->refresh_button), GTK_RELIEF_NONE);
			GTK_WIDGET_SET_FLAGS (data->refresh_button, GTK_CAN_DEFAULT);
			gtk_widget_show (data->refresh_button);
			gedit_message_area_add_action_widget (GEDIT_MESSAGE_AREA (gth_browser_get_list_extra_widget (browser)),
							      data->refresh_button,
							      _RESPONSE_REFRESH);
			g_signal_connect (data->refresh_button,
					  "clicked",
					  G_CALLBACK (refresh_button_clicked_cb),
					  browser);
		}
	}
	else {
		if (data->search_merge_id != 0) {
			gtk_ui_manager_remove_ui (gth_browser_get_ui_manager (browser), data->search_merge_id);
			data->search_merge_id = 0;
		}
		if (data->find_merge_id == 0) {
			GError *local_error = NULL;

			data->find_merge_id = gtk_ui_manager_add_ui_from_string (gth_browser_get_ui_manager (browser), find_ui_info, -1, &local_error);
			if (data->find_merge_id == 0) {
				g_warning ("building menus failed: %s", local_error->message);
				g_error_free (local_error);
			}
			gtk_tool_item_set_is_important (GTK_TOOL_ITEM (gtk_ui_manager_get_widget (gth_browser_get_ui_manager (browser), "/ToolBar/SourceCommands/Edit_Find")), TRUE);
		}
	}
}


GthCatalog *
search__gth_catalog_load_from_data_cb (const void *buffer)
{
	if ((buffer != NULL) && (strncmp (buffer, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<search ", 47) == 0))
		return (GthCatalog *) gth_search_new ();
	else
		return NULL;
}


void
search__dlg_catalog_properties (GtkBuilder  *builder,
				GthFileData *file_data,
				GthCatalog  *catalog)
{
	GtkWidget     *vbox;
	GtkWidget     *label;
	PangoAttrList *attrs;
	GtkWidget     *alignment;
	GtkWidget     *search_editor;

	if (! _g_content_type_is_a (g_file_info_get_content_type (file_data->info), "gthumb/search"))
		return;

	vbox = gtk_vbox_new (FALSE, 6);
	gtk_widget_show (vbox);
	gtk_box_pack_start (GTK_BOX (_gtk_builder_get_widget (builder, "main_vbox")), vbox, FALSE, FALSE, 0);

	label = gtk_label_new (_("Search"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	attrs = pango_attr_list_new ();
	pango_attr_list_insert (attrs, pango_attr_weight_new (PANGO_WEIGHT_BOLD));
	gtk_label_set_attributes (GTK_LABEL (label), attrs);
	pango_attr_list_unref (attrs);
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

	alignment = gtk_alignment_new (0.0, 0.0, 0.0, 0.0);
	gtk_widget_show (alignment);
	gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 0, 0, 12, 0);
	gtk_box_pack_start (GTK_BOX (vbox), alignment, FALSE, FALSE, 0);

	search_editor = gth_search_editor_new (GTH_SEARCH (catalog));
	gtk_widget_show (search_editor);
	gtk_container_add (GTK_CONTAINER (alignment), search_editor);
}
