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
#include "actions.h"
#include "callbacks.h"
#include "gth-script-file.h"
#include "gth-script-task.h"
#include "list-tools.h"


#define BROWSER_DATA_KEY "list-tools-browser-data"


static const GActionEntry actions[] = {
	{ "exec-script", gth_browser_activate_exec_script, "s" },
	{ "personalize-tools", gth_browser_activate_personalize_tools }
};


static const char *fixed_ui_info =
"<ui>"
"  <toolbar name='ToolBar'>"
"    <placeholder name='Edit_Actions_2'>"
"      <toolitem action='ListTools'/>"
"    </placeholder>"
"  </toolbar>"
"  <toolbar name='ViewerToolBar'>"
"    <placeholder name='Edit_Actions_2'>"
"      <toolitem action='ListTools'/>"
"    </placeholder>"
"  </toolbar>"
/*
"  <popup name='FileListPopup'>"
"    <placeholder name='Open_Actions'>"
"      <menu name='ExecWith' action='ExecWithMenu'>"
"        <placeholder name='Tools'/>"
"        <placeholder name='Scripts'/>"
"        <separator name='ScriptsListSeparator'/>"
"        <menuitem name='EditScripts' action='ListTools_EditScripts'/>"
"      </menu>"
"    </placeholder>"
"  </popup>"
*/
"  <popup name='ListToolsPopup'>"
"    <placeholder name='Tools'/>"
"    <separator/>"
"    <placeholder name='Tools_2'/>"
"    <separator name='ToolsSeparator'/>"
"    <placeholder name='Scripts'/>"
"    <separator name='ScriptsListSeparator'/>"
"    <menuitem name='EditScripts' action='ListTools_EditScripts'/>"
"  </popup>"
"</ui>";


static GtkActionEntry action_entries[] = {
	/*{ "ExecWithMenu", GTK_STOCK_EXECUTE, N_("_Tools") },*/

	{ "ListTools_EditScripts", GTK_STOCK_EDIT,
	  N_("Personalize..."), NULL,
	  NULL,
	  G_CALLBACK (gth_browser_action_edit_scripts) }
};


typedef struct {
	GthBrowser     *browser;
	GtkActionGroup *action_group;
	gulong          scripts_changed_id;
	gboolean        menu_initialized;
	guint           menu_merge_id;
} BrowserData;


static void
browser_data_free (BrowserData *data)
{
	if (data->scripts_changed_id != 0)
		g_signal_handler_disconnect (gth_script_file_get (), data->scripts_changed_id);
	g_free (data);
}


static GtkWidget *
get_widget_with_prefix (BrowserData *data,
			const char  *prefix,
			const char  *path)
{
	char      *full_path;
	GtkWidget *widget;

	full_path = g_strconcat (prefix, path, NULL);
	widget = gtk_ui_manager_get_widget (gth_browser_get_ui_manager (data->browser), full_path);

	g_free (full_path);

	return widget;
}


static void
_update_sensitivity (GthBrowser *browser,
		     const char *prefix)
{
	BrowserData *data;
	int          n_selected;
	gboolean     sensitive;
	GtkWidget   *separator1;
	GtkWidget   *separator2;
	GtkWidget   *menu;

	data = g_object_get_data (G_OBJECT (browser), BROWSER_DATA_KEY);
	g_return_if_fail (data != NULL);

	n_selected = gth_file_selection_get_n_selected (GTH_FILE_SELECTION (gth_browser_get_file_list_view (browser)));
	sensitive = (n_selected > 0);

	separator1 = get_widget_with_prefix (data, prefix, "/ToolsSeparator");
	separator2 = get_widget_with_prefix (data, prefix, "/Scripts");
	menu = gtk_widget_get_parent (separator1);
	{
		GList *children;
		GList *scan;

		children = gtk_container_get_children (GTK_CONTAINER (menu));

		if (separator1 != NULL) {
			for (scan = children; scan; scan = scan->next)
				if (scan->data == separator1) {
					scan = scan->next;
					break;
				}
		}
		else
			scan = children;

		for (/* void */; scan && (scan->data != separator2); scan = scan->next)
			gtk_widget_set_sensitive (scan->data, sensitive);
	}
}


static void
list_tools__gth_browser_update_sensitivity_cb (GthBrowser *browser)
{
	_update_sensitivity (browser, "/ListToolsPopup");
	/*_update_sensitivity (browser, "/FileListPopup/Open_Actions/ExecWith");*/
}


static void
activate_script_menu_item (GtkMenuItem *menuitem,
			   gpointer     user_data)
{
	BrowserData *data = user_data;
	GthScript   *script;

	script = gth_script_file_get_script (gth_script_file_get (), g_object_get_data (G_OBJECT (menuitem), "script_id"));
	if (script != NULL)
		gth_browser_exec_script (data->browser, script);
}


static void
_update_scripts_menu (BrowserData *data,
		      const char  *prefix)
{
	GtkWidget	*separator1;
	GtkWidget	*separator2;
	GtkWidget	*menu;
	GthMenuManager	*menu_manager;
	GList		*script_list;
	GList		*scan;
	int		 pos;
	gboolean	 script_present = FALSE;

	separator1 = get_widget_with_prefix (data, prefix, "/ToolsSeparator");
	separator2 = get_widget_with_prefix (data, prefix, "/Scripts");
	menu = gtk_widget_get_parent (separator1);
	_gtk_container_remove_children (GTK_CONTAINER (menu), separator1, separator2);

	menu_manager = gth_browser_get_menu_manager (data->browser, GTH_BROWSER_MENU_MANAGER_TOOLS3);
	if (data->menu_merge_id != 0)
		gth_menu_manager_remove_entries (menu_manager, data->menu_merge_id);
	data->menu_merge_id = gth_menu_manager_new_merge_id (menu_manager);

	script_list = gth_script_file_get_scripts (gth_script_file_get ());
	pos = _gtk_container_get_pos (GTK_CONTAINER (menu), separator2);
	for (scan = script_list; scan; scan = scan->next) {
		GthScript *script = scan->data;
		GtkWidget *menu_item;
		char      *detailed_action;

		if (! gth_script_is_visible (script))
			continue;

		menu_item = gtk_image_menu_item_new_with_label (gth_script_get_display_name (script));
		/*gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item), gtk_image_new_from_stock (GTK_STOCK_EXECUTE, GTK_ICON_SIZE_MENU));*/
		gtk_widget_show (menu_item);
		gtk_menu_shell_insert (GTK_MENU_SHELL (menu), menu_item, pos++);

		g_object_set_data_full (G_OBJECT (menu_item),
					"script_id",
					g_strdup (gth_script_get_id (script)),
					(GDestroyNotify) g_free);
		g_signal_connect (menu_item,
				  "activate",
				  G_CALLBACK (activate_script_menu_item),
			  	  data);

		detailed_action = g_strdup_printf ("win.exec-script('%s')", gth_script_get_id (script));
		gth_menu_manager_append_entry (menu_manager,
					       data->menu_merge_id,
					       gth_script_get_display_name (script),
					       detailed_action,
					       NULL,
					       NULL);
		script_present = TRUE;

		g_free (detailed_action);
	}

	separator1 = get_widget_with_prefix (data, prefix, "/ScriptsListSeparator");
	if (script_present)
		gtk_widget_show (separator1);
	else
		gtk_widget_hide (separator1);

	list_tools__gth_browser_update_sensitivity_cb (data->browser);

	_g_object_list_unref (script_list);
}


static void
update_scripts_menu (BrowserData *data)
{
	_update_scripts_menu (data, "/ListToolsPopup");
	/*_update_scripts_menu (data, "/FileListPopup/Open_Actions/ExecWith");*/
}


static void
scripts_changed_cb (GthScriptFile *script_file,
		     BrowserData   *data)
{
	update_scripts_menu (data);
}


static void
list_tools_show_menu_func (GtkAction *action,
		           gpointer   user_data)
{
	BrowserData *data = user_data;
	GtkWidget   *menu;

	if (! data->menu_initialized) {
		data->menu_initialized = TRUE;

		menu = gtk_ui_manager_get_widget (gth_browser_get_ui_manager (data->browser), "/ListToolsPopup");
		g_object_set (action, "menu", menu, NULL);
		update_scripts_menu (data);

		data->scripts_changed_id = g_signal_connect (gth_script_file_get (),
							     "changed",
							     G_CALLBACK (scripts_changed_cb),
							     data);
	}

	list_tools__gth_browser_update_sensitivity_cb (data->browser);
}


static void
tools_menu_button_toggled_cb (GtkToggleButton *togglebutton,
			      gpointer         user_data)
{
	BrowserData *data = user_data;

	if (! gtk_toggle_button_get_active (togglebutton))
		return;

	if (! data->menu_initialized) {
		data->menu_initialized = TRUE;
		update_scripts_menu (data);
	}
}


void
list_tools__gth_browser_construct_cb (GthBrowser *browser)
{
	BrowserData *data;
	GtkAction   *action;
	GError      *error = NULL;

	g_return_if_fail (GTH_IS_BROWSER (browser));

	g_action_map_add_action_entries (G_ACTION_MAP (browser),
					 actions,
					 G_N_ELEMENTS (actions),
					 browser);

	data = g_new0 (BrowserData, 1);
	data->browser = browser;
	data->action_group = gtk_action_group_new ("List Tools Manager Actions");
	gtk_action_group_set_translation_domain (data->action_group, NULL);
	gtk_action_group_add_actions (data->action_group,
				      action_entries,
				      G_N_ELEMENTS (action_entries),
				      browser);

	/* tools menu action */

	action = g_object_new (GTH_TYPE_TOGGLE_MENU_ACTION,
			       "name", "ListTools",
			       "stock-id", GTK_STOCK_EXECUTE,
			       "label", _("Tools"),
			       "tooltip",  _("Batch tools for multiple files"),
			       "is-important", TRUE,
			       NULL);
	gth_toggle_menu_action_set_show_menu_func (GTH_TOGGLE_MENU_ACTION (action),
						   list_tools_show_menu_func,
						   data,
						   NULL);
	gtk_action_group_add_action (data->action_group, action);
	g_object_unref (action);

	gtk_ui_manager_insert_action_group (gth_browser_get_ui_manager (browser), data->action_group, 0);

	if (! gtk_ui_manager_add_ui_from_string (gth_browser_get_ui_manager (browser), fixed_ui_info, -1, &error)) {
		g_message ("building menus failed: %s", error->message);
		g_clear_error (&error);
	}

	g_object_set_data_full (G_OBJECT (browser), BROWSER_DATA_KEY, data, (GDestroyNotify) browser_data_free);

	{
		GtkWidget  *button;
		GtkBuilder *builder;
		GMenuModel *menu;

		builder = gtk_builder_new_from_resource ("/org/gnome/gThumb/list_tools/data/ui/tools-menu.ui");
		gth_browser_add_menu_manager_for_menu (browser, GTH_BROWSER_MENU_MANAGER_TOOLS1, G_MENU (gtk_builder_get_object (builder, "tools1")));
		gth_browser_add_menu_manager_for_menu (browser, GTH_BROWSER_MENU_MANAGER_TOOLS2, G_MENU (gtk_builder_get_object (builder, "tools2")));
		gth_browser_add_menu_manager_for_menu (browser, GTH_BROWSER_MENU_MANAGER_TOOLS3, G_MENU (gtk_builder_get_object (builder, "tools3")));
		menu = G_MENU_MODEL (gtk_builder_get_object (builder, "tools-menu"));

		/* browser tools */

		button = _gtk_menu_button_new_for_header_bar ();
		g_signal_connect (button, "toggled", G_CALLBACK (tools_menu_button_toggled_cb), data);
		gtk_widget_set_tooltip_text (button, _("Tools"));
		gtk_container_add (GTK_CONTAINER (button), gtk_image_new_from_icon_name ("system-run-symbolic", GTK_ICON_SIZE_MENU));
		gtk_menu_button_set_menu_model (GTK_MENU_BUTTON (button), menu);
		gtk_widget_show_all (button);
		gtk_box_pack_start (GTK_BOX (gth_browser_get_headerbar_section (browser, GTH_BROWSER_HEADER_SECTION_BROWSER_TOOLS)), button, FALSE, FALSE, 0);

		/* viewer tools */

		button = _gtk_menu_button_new_for_header_bar ();
		g_signal_connect (button, "toggled", G_CALLBACK (tools_menu_button_toggled_cb), data);
		gtk_widget_set_tooltip_text (button, _("Tools"));
		gtk_container_add (GTK_CONTAINER (button), gtk_image_new_from_icon_name ("system-run-symbolic", GTK_ICON_SIZE_MENU));
		gtk_menu_button_set_menu_model (GTK_MENU_BUTTON (button), menu);
		gtk_widget_show_all (button);
		gtk_box_pack_start (GTK_BOX (gth_browser_get_headerbar_section (browser, GTH_BROWSER_HEADER_SECTION_VIEWER_TOOLS)), button, FALSE, FALSE, 0);

		g_object_unref (builder);
	}
}


gpointer
list_tools__gth_browser_file_list_key_press_cb (GthBrowser  *browser,
						GdkEventKey *event)
{
	gpointer  result = NULL;
	GList    *script_list;
	GList    *scan;

	script_list = gth_script_file_get_scripts (gth_script_file_get ());
	for (scan = script_list; scan; scan = scan->next) {
		GthScript *script = scan->data;

		if (gth_script_get_shortcut (script) == event->keyval) {
			gth_browser_exec_script (browser, script);
			result = GINT_TO_POINTER (1);
			break;
		}
	}

	_g_object_list_unref (script_list);

	return result;
}
