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
#include "actions.h"
#include "gth-tag-task.h"


#define BROWSER_DATA_KEY "edit-metadata-data"


static const char *fixed_ui_info =
"<ui>"
"  <menubar name='MenuBar'>"
"    <menu name='Edit' action='EditMenu'>"
"      <placeholder name='Edit_Actions'>"
"        <menuitem action='Edit_Metadata'/>"
"      </placeholder>"
"    </menu>"
"  </menubar>"
"  <toolbar name='ViewerToolBar'>"
"    <placeholder name='Edit_Actions'>"
"      <toolitem action='Edit_Metadata'/>"
"    </placeholder>"
"  </toolbar>"
"  <toolbar name='Fullscreen_ToolBar'>"
"    <placeholder name='Edit_Actions'>"
"      <toolitem action='Edit_Metadata'/>"
"    </placeholder>"
"  </toolbar>"
"  <popup name='FileListPopup'>"
"    <placeholder name='File_LastActions'>"
"      <menu action='Edit_QuickTag'>"
"        <separator name='TagListSeparator'/>"
"        <menuitem action='Edit_QuickTagOther'/>"
"      </menu>"
"      <menuitem action='Edit_Metadata'/>"
"    </placeholder>"
"  </popup>"
"  <popup name='FilePopup'>"
"    <placeholder name='File_LastActions'>"
"      <menu action='Edit_QuickTag'>"
"        <separator name='TagListSeparator'/>"
"        <menuitem action='Edit_QuickTagOther'/>"
"      </menu>"
"      <menuitem action='Edit_Metadata'/>"
"    </placeholder>"
"  </popup>"
"</ui>";


static const char *browser_ui_info =
"<ui>"
"  <toolbar name='ToolBar'>"
"    <placeholder name='Edit_Actions'>"
"      <toolitem action='Edit_Metadata'/>"
"    </placeholder>"
"  </toolbar>"
"</ui>";


static const char *viewer_ui_info =
"<ui>"
"  <menubar name='MenuBar'>"
"    <menu name='Edit' action='EditMenu'>"
"      <placeholder name='Edit_Actions'>"
"        <menuitem action='Edit_Metadata'/>"
"      </placeholder>"
"    </menu>"
"  </menubar>"
"</ui>";


static GtkActionEntry edit_metadata_action_entries[] = {
	{ "Edit_QuickTag", "tag", N_("T_ags") },

	{ "Edit_Metadata", GTK_STOCK_EDIT,
	  N_("Comment"), "<control>M",
	  N_("Edit the comment an other information of the selected files"),
	  G_CALLBACK (gth_browser_activate_action_edit_metadata) },

        { "Edit_QuickTagOther", NULL,
	  N_("Other..."), NULL,
	  N_("Choose another tag"),
	  G_CALLBACK (gth_browser_activate_action_edit_tag_files) }
};


typedef struct {
	GthBrowser     *browser;
	GtkActionGroup *actions;
	guint           browser_ui_merge_id;
	guint           viewer_ui_merge_id;
	gboolean        tag_menu_loaded;
	guint           monitor_events;
} BrowserData;


static void
browser_data_free (BrowserData *data)
{
	if (data->monitor_events != 0) {
		g_signal_handler_disconnect (gth_main_get_default_monitor (), data->monitor_events);
		data->monitor_events = 0;
	}
	g_free (data);
}


static void
monitor_tags_changed_cb (GthMonitor  *monitor,
			 BrowserData *data)
{
	data->tag_menu_loaded = FALSE;
}


void
edit_metadata__gth_browser_construct_cb (GthBrowser *browser)
{
	BrowserData *data;
	GError      *error = NULL;

	g_return_if_fail (GTH_IS_BROWSER (browser));

	data = g_new0 (BrowserData, 1);
	data->browser = browser;

	data->actions = gtk_action_group_new ("Edit Metadata Actions");
	gtk_action_group_set_translation_domain (data->actions, NULL);
	gtk_action_group_add_actions (data->actions,
				      edit_metadata_action_entries,
				      G_N_ELEMENTS (edit_metadata_action_entries),
				      browser);
	gtk_ui_manager_insert_action_group (gth_browser_get_ui_manager (browser), data->actions, 0);

	if (! gtk_ui_manager_add_ui_from_string (gth_browser_get_ui_manager (browser), fixed_ui_info, -1, &error)) {
		g_message ("building menus failed: %s", error->message);
		g_error_free (error);
	}

	gtk_tool_item_set_is_important (GTK_TOOL_ITEM (gtk_ui_manager_get_widget (gth_browser_get_ui_manager (browser), "/Fullscreen_ToolBar/Edit_Actions/Edit_Metadata")), TRUE);

	data->monitor_events = g_signal_connect (gth_main_get_default_monitor (),
						 "tags-changed",
						 G_CALLBACK (monitor_tags_changed_cb),
						 data);

	g_object_set_data_full (G_OBJECT (browser), BROWSER_DATA_KEY, data, (GDestroyNotify) browser_data_free);
}


void
edit_metadata__gth_browser_set_current_page_cb (GthBrowser *browser)
{
	BrowserData *data;
	GError      *error = NULL;

	data = g_object_get_data (G_OBJECT (browser), BROWSER_DATA_KEY);
	g_return_if_fail (data != NULL);

	switch (gth_window_get_current_page (GTH_WINDOW (browser))) {
	case GTH_BROWSER_PAGE_BROWSER:
		if (data->viewer_ui_merge_id != 0) {
			gtk_ui_manager_remove_ui (gth_browser_get_ui_manager (browser), data->viewer_ui_merge_id);
			data->viewer_ui_merge_id = 0;
		}
		if (data->browser_ui_merge_id != 0)
			return;
		data->browser_ui_merge_id = gtk_ui_manager_add_ui_from_string (gth_browser_get_ui_manager (browser), browser_ui_info, -1, &error);
		if (data->browser_ui_merge_id == 0) {
			g_warning ("ui building failed: %s", error->message);
			g_clear_error (&error);
		}
		gtk_tool_item_set_is_important (GTK_TOOL_ITEM (gtk_ui_manager_get_widget (gth_browser_get_ui_manager (browser), "/ToolBar/Edit_Actions/Edit_Metadata")), TRUE);
		break;

	case GTH_BROWSER_PAGE_VIEWER:
		if (data->browser_ui_merge_id != 0) {
			gtk_ui_manager_remove_ui (gth_browser_get_ui_manager (browser), data->browser_ui_merge_id);
			data->browser_ui_merge_id = 0;
		}
		if (data->viewer_ui_merge_id != 0)
			return;
		data->viewer_ui_merge_id = gtk_ui_manager_add_ui_from_string (gth_browser_get_ui_manager (browser), viewer_ui_info, -1, &error);
		if (data->viewer_ui_merge_id == 0) {
			g_warning ("ui building failed: %s", error->message);
			g_clear_error (&error);
		}
		gtk_tool_item_set_is_important (GTK_TOOL_ITEM (gtk_ui_manager_get_widget (gth_browser_get_ui_manager (browser), "/ViewerToolBar/Edit_Actions/Edit_Metadata")), TRUE);
		break;

	default:
		break;
	}
}


void
edit_metadata__gth_browser_update_sensitivity_cb (GthBrowser *browser)
{
	BrowserData *data;
	GtkAction   *action;
	int          n_selected;
	gboolean     sensitive;

	data = g_object_get_data (G_OBJECT (browser), BROWSER_DATA_KEY);
	g_return_if_fail (data != NULL);

	n_selected = gth_file_selection_get_n_selected (GTH_FILE_SELECTION (gth_browser_get_file_list_view (browser)));

	action = gtk_action_group_get_action (data->actions, "Edit_Metadata");
	sensitive = (n_selected > 0);
	g_object_set (action, "sensitive", sensitive, NULL);
}


static void
tag_item_activate_cb (GtkMenuItem *menuitem,
		      gpointer     user_data)
{
	GthBrowser  *browser = user_data;
	GList       *items;
	GList       *file_data_list;
	GList       *file_list;
	char        *tag;
	char       **tags;
	GthTask     *task;

	if (gtk_menu_item_get_submenu (menuitem) != NULL)
		return;

	items = gth_file_selection_get_selected (GTH_FILE_SELECTION (gth_browser_get_file_list_view (browser)));
	file_data_list = gth_file_list_get_files (GTH_FILE_LIST (gth_browser_get_file_list (browser)), items);
	file_list = gth_file_data_list_to_file_list (file_data_list);

	tag = g_object_get_data (G_OBJECT (menuitem), "tag");
	tags = g_new0 (char *, 2);
	tags[0] = g_strdup (tag);
	tags[1] = NULL;

	task = gth_tag_task_new (file_list, tags);
	gth_browser_exec_task (browser, task, FALSE);

	g_object_unref (task);
	g_strfreev (tags);
	_g_object_list_unref (file_list);
	_g_object_list_unref (file_data_list);
	_gtk_tree_path_list_free (items);
}


static void
insert_tag_menu_item (BrowserData *data,
		      GtkWidget   *menu,
		      const char  *tag,
		      int          pos)
{
	GtkWidget *item;
	GtkWidget *image;

	item = gtk_image_menu_item_new_with_label (tag);
	/*gtk_image_menu_item_set_always_show_image (GTK_IMAGE_MENU_ITEM (item), TRUE);*/

	image = gtk_image_new_from_icon_name ("tag", GTK_ICON_SIZE_MENU);
	gtk_widget_show (image);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), image);
	gtk_widget_show (item);
	gtk_menu_shell_insert (GTK_MENU_SHELL (menu), item, pos);
	g_object_set_data_full (G_OBJECT (item), "tag", g_strdup (tag), g_free);
	g_signal_connect (item, "activate", G_CALLBACK (tag_item_activate_cb), data->browser);
}


static void
update_tag_menu (BrowserData *data)
{
	GtkWidget   *list_menu;
	GtkWidget   *file_menu;
	GtkWidget   *separator;
	char       **tags;
	int          i;

	list_menu = gtk_menu_item_get_submenu (GTK_MENU_ITEM (gtk_ui_manager_get_widget (gth_browser_get_ui_manager (data->browser), "/FileListPopup/File_LastActions/Edit_QuickTag")));
	separator = gtk_ui_manager_get_widget (gth_browser_get_ui_manager (data->browser), "/FileListPopup/File_LastActions/Edit_QuickTag/TagListSeparator");
	_gtk_container_remove_children (GTK_CONTAINER (list_menu), NULL, separator);

	file_menu = gtk_menu_item_get_submenu (GTK_MENU_ITEM (gtk_ui_manager_get_widget (gth_browser_get_ui_manager (data->browser), "/FilePopup/File_LastActions/Edit_QuickTag")));
	separator = gtk_ui_manager_get_widget (gth_browser_get_ui_manager (data->browser), "/FilePopup/File_LastActions/Edit_QuickTag/TagListSeparator");
	_gtk_container_remove_children (GTK_CONTAINER (file_menu), NULL, separator);

	tags = g_strdupv (gth_tags_file_get_tags (gth_main_get_default_tag_file ()));
	for (i = 0; tags[i] != NULL; i++) {
		insert_tag_menu_item (data, list_menu, tags[i], i);
		insert_tag_menu_item (data, file_menu, tags[i], i);
	}

	g_strfreev (tags);
}


void
edit_metadata__gth_browser_file_list_popup_before_cb (GthBrowser *browser)
{
	BrowserData *data;

	data = g_object_get_data (G_OBJECT (browser), BROWSER_DATA_KEY);
	g_return_if_fail (data != NULL);

	if (! data->tag_menu_loaded) {
		data->tag_menu_loaded = TRUE;
		update_tag_menu (data);
	}
}


void
edit_metadata__gth_browser_file_popup_before_cb (GthBrowser *browser)
{
	BrowserData *data;

	data = g_object_get_data (G_OBJECT (browser), BROWSER_DATA_KEY);
	g_return_if_fail (data != NULL);

	if (! data->tag_menu_loaded) {
		data->tag_menu_loaded = TRUE;
		update_tag_menu (data);
	}
}


gpointer
edit_metadata__gth_browser_file_list_key_press_cb (GthBrowser  *browser,
						   GdkEventKey *event)
{
	gpointer result = NULL;
	guint    modifiers;

	modifiers = gtk_accelerator_get_default_mod_mask ();
	if ((event->state & modifiers) != 0)
		return NULL;

	switch (gdk_keyval_to_lower (event->keyval)) {
	case GDK_c:
		gth_browser_activate_action_edit_metadata (NULL, browser);
		result = GINT_TO_POINTER (1);
		break;
	}

	return result;
}

