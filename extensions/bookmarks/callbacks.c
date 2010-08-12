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


#define BROWSER_DATA_KEY "bookmarks-browser-data"


static const char *fixed_ui_info =
"<ui>"
"  <menubar name='MenuBar'>"
"    <placeholder name='OtherMenus'>"
"      <menu name='Bookmarks' action='BookmarksMenu'>"
"        <menuitem action='Bookmarks_Add'/>"
"        <menuitem action='Bookmarks_Edit'/>"
"        <separator/>"
"        <menu name='SystemBookmarks' action='SystemBookmarksMenu'>"
"        </menu>"
"        <separator name='BookmarkListSeparator'/>"
"        <placeholder name='BookmarkList'/>"
"      </menu>"
"    </placeholder>"
"  </menubar>"
"</ui>";


static GtkActionEntry bookmarks_action_entries[] = {
	{ "BookmarksMenu", NULL, N_("_Bookmarks") },
	{ "SystemBookmarksMenu", NULL, N_("_System Bookmarks") },

	{ "Bookmarks_Add", GTK_STOCK_ADD,
	  N_("_Add Bookmark"), "<control>D",
	  N_("Add current location to bookmarks"),
	  G_CALLBACK (gth_browser_activate_action_bookmarks_add) },

	{ "Bookmarks_Edit", NULL,
	  N_("_Edit Bookmarks..."), "<control>B",
	  N_("Edit bookmarks"),
	  G_CALLBACK (gth_browser_activate_action_bookmarks_edit) },
};
static guint bookmarks_action_entries_size = G_N_ELEMENTS (bookmarks_action_entries);


typedef struct {
	GthBrowser     *browser;
	GtkActionGroup *actions;
	guint           bookmarks_changed_id;
} BrowserData;


static void
browser_data_free (BrowserData *data)
{
	if (data->bookmarks_changed_id != 0) {
		g_signal_handler_disconnect (gth_main_get_default_monitor (),
					     data->bookmarks_changed_id);
		data->bookmarks_changed_id = 0;
	}
	g_free (data);
}


#define BUFFER_SIZE 4096


typedef struct {
	GthBrowser   *browser;
	GInputStream *stream;
	char          buffer[BUFFER_SIZE];
	GString      *file_content;
} UpdateBookmarksData;


static void
update_bookmakrs_data_free (UpdateBookmarksData *data)
{
	g_input_stream_close (data->stream, NULL, NULL);
	g_object_unref (data->stream);
	g_string_free (data->file_content, TRUE);
	g_object_unref (data->browser);
	g_free (data);
}


static void
update_system_bookmark_list_from_content (GthBrowser *browser,
					  const char *content)
{
	GtkWidget  *bookmark_list;
	GtkWidget  *menu;
	char      **lines;
	int         i;

	bookmark_list = gtk_ui_manager_get_widget (gth_browser_get_ui_manager (browser), "/MenuBar/OtherMenus/Bookmarks/SystemBookmarks");
	menu = gtk_menu_item_get_submenu (GTK_MENU_ITEM (bookmark_list));

	_gtk_container_remove_children (GTK_CONTAINER (menu), NULL, NULL);

	lines = g_strsplit (content, "\n", -1);
	for (i = 0; lines[i] != NULL; i++) {
		char  **line;
		char   *uri;
		GFile  *file;
		GIcon  *icon;
		char   *name;

		line = g_strsplit (lines[i], " ", 2);
		uri = line[0];
		if (uri == NULL)
			continue;

		file = g_file_new_for_uri (uri);
		icon = _g_file_get_icon (file);
		name = g_strdup (strchr (lines[i], ' '));
		if (name == NULL)
			name = _g_file_get_display_name (file);
		if (name == NULL)
			name = g_file_get_parse_name (file);

		_gth_browser_add_file_menu_item_full (browser,
						      menu,
						      file,
						      icon,
						      name,
						      GTH_ACTION_GO_TO,
						      i,
						      -1);

		g_free (name);
		g_object_unref (icon);
		g_object_unref (file);
		g_strfreev (line);
	}
	g_strfreev (lines);

	if (i > 0)
		gtk_widget_show (bookmark_list);
}


static void
update_system_bookmark_list_ready (GObject      *source_object,
				   GAsyncResult *result,
				   gpointer      user_data)
{
	UpdateBookmarksData *data = user_data;
	gssize               size;

	size = g_input_stream_read_finish (data->stream, result, NULL);
	if (size < 0) {
		update_bookmakrs_data_free (data);
		return;
	}

	if (size > 0) {
		data->buffer[size + 1] = '\0';
		g_string_append (data->file_content, data->buffer);

		g_input_stream_read_async (data->stream,
				   	   data->buffer,
				   	   BUFFER_SIZE - 1,
					   G_PRIORITY_DEFAULT,
					   NULL,
					   update_system_bookmark_list_ready,
					   data);
		return;
	}

	update_system_bookmark_list_from_content (data->browser, data->file_content->str);
	update_bookmakrs_data_free (data);
}


static void
_gth_browser_update_system_bookmark_list (GthBrowser *browser)
{
	char                *bookmark_file_path;
	GFile               *bookmark_file;
	GFileInputStream    *input_stream;
	UpdateBookmarksData *data;

	bookmark_file_path = g_build_filename (g_get_home_dir (), ".gtk-bookmarks", NULL);
	bookmark_file = g_file_new_for_path (bookmark_file_path);
	g_free (bookmark_file_path);

	input_stream = g_file_read (bookmark_file, NULL, NULL);
	g_object_unref (bookmark_file);

	if (input_stream == NULL)
		return;

	data = g_new0 (UpdateBookmarksData, 1);
	data->browser = g_object_ref (browser);
	data->stream = (GInputStream*) input_stream;
	data->file_content = g_string_new ("");

	g_input_stream_read_async (data->stream,
				   data->buffer,
				   BUFFER_SIZE - 1,
				   G_PRIORITY_DEFAULT,
				   NULL,
				   update_system_bookmark_list_ready,
				   data);
}


static void
_gth_browser_update_bookmark_list (GthBrowser *browser)
{
	GtkWidget      *menu;
	GtkWidget      *bookmark_list;
	GtkWidget      *bookmark_list_separator;
	GBookmarkFile  *bookmarks;
	char          **uris;
	gsize           length;
	int             i;

	bookmark_list = gtk_ui_manager_get_widget (gth_browser_get_ui_manager (browser), "/MenuBar/OtherMenus/Bookmarks/BookmarkList");
	menu = gtk_widget_get_parent (bookmark_list);

	_gtk_container_remove_children (GTK_CONTAINER (menu), bookmark_list, NULL);

	bookmarks = gth_main_get_default_bookmarks ();
	uris = g_bookmark_file_get_uris (bookmarks, &length);

	bookmark_list_separator = gtk_ui_manager_get_widget (gth_browser_get_ui_manager (browser), "/MenuBar/OtherMenus/Bookmarks/BookmarkListSeparator");
	if (length > 0)
		gtk_widget_show (bookmark_list_separator);
	else
		gtk_widget_hide (bookmark_list_separator);

	for (i = 0; uris[i] != NULL; i++) {
		GFile *file;
		char  *name;

		file = g_file_new_for_uri (uris[i]);
		name = g_bookmark_file_get_title (bookmarks, uris[i], NULL);
		_gth_browser_add_file_menu_item (browser,
						 menu,
						 file,
						 name,
						 GTH_ACTION_GO_TO,
						 i);

		g_free (name);
		g_object_unref (file);
	}

	_gth_browser_update_system_bookmark_list (browser);

	g_strfreev (uris);
}


static void
bookmarks_changed_cb (GthMonitor *monitor,
		      gpointer    user_data)
{
	BrowserData *data = user_data;
	_gth_browser_update_bookmark_list (data->browser);
}


void
bookmarks__gth_browser_construct_cb (GthBrowser *browser)
{
	BrowserData *data;
	GError      *error = NULL;

	g_return_if_fail (GTH_IS_BROWSER (browser));

	data = g_new0 (BrowserData, 1);
	data->browser = browser;

	data->actions = gtk_action_group_new ("Bookmarks Actions");
	gtk_action_group_set_translation_domain (data->actions, NULL);
	gtk_action_group_add_actions (data->actions,
				      bookmarks_action_entries,
				      bookmarks_action_entries_size,
				      browser);
	gtk_ui_manager_insert_action_group (gth_browser_get_ui_manager (browser), data->actions, 0);

	if (! gtk_ui_manager_add_ui_from_string (gth_browser_get_ui_manager (browser), fixed_ui_info, -1, &error)) {
		g_message ("building menus failed: %s", error->message);
		g_error_free (error);
	}

	data->bookmarks_changed_id = g_signal_connect (gth_main_get_default_monitor (),
						       "bookmarks-changed",
						       G_CALLBACK (bookmarks_changed_cb),
						       data);

	g_object_set_data_full (G_OBJECT (browser), BROWSER_DATA_KEY, data, (GDestroyNotify) browser_data_free);
}


void
bookmarks__gth_browser_construct_idle_callback_cb (GthBrowser *browser)
{
	_gth_browser_update_bookmark_list (browser);
}


void
bookmarks__gth_browser_update_sensitivity_cb (GthBrowser *browser)
{
	BrowserData *data;
	/*GtkAction   *action;*/

	data = g_object_get_data (G_OBJECT (browser), BROWSER_DATA_KEY);
	g_return_if_fail (data != NULL);

	/*action = gtk_action_group_get_action (data->actions, "Edit_AddToCatalog");
	sensitive = n_selected > 0;
	g_object_set (action, "sensitive", sensitive, NULL);*/
}
