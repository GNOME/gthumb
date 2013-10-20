/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2010 Free Software Foundation, Inc.
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
#include "dlg-location.h"
#include "dlg-personalize-filters.h"
#include "dlg-preferences.h"
#include "dlg-sort-order.h"
#include "glib-utils.h"
#include "gth-browser.h"
#include "gth-file-list.h"
#include "gth-file-selection.h"
#include "gth-folder-tree.h"
#include "gth-main.h"
#include "gth-preferences.h"
#include "gth-sidebar.h"
#include "gtk-utils.h"
#include "gth-viewer-page.h"
#include "main.h"


void
gth_browser_activate_action_file_open (GtkAction  *action,
				       GthBrowser *browser)
{
	GList *items;
	GList *file_list;

	items = gth_file_selection_get_selected (GTH_FILE_SELECTION (gth_browser_get_file_list_view (browser)));
	file_list = gth_file_list_get_files (GTH_FILE_LIST (gth_browser_get_file_list (browser)), items);

	if (file_list != NULL)
		gth_browser_load_file (browser, (GthFileData *) file_list->data, TRUE);

	_g_object_list_unref (file_list);
	_gtk_tree_path_list_free (items);
}


void
gth_browser_activate_action_view_filter (GtkAction  *action,
					 GthBrowser *browser)
{
	dlg_personalize_filters (browser);
}


void
gth_browser_activate_action_view_filterbar (GtkAction  *action,
					    GthBrowser *browser)
{
	gth_browser_show_filterbar (browser, gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)));
}


void
gth_browser_activate_action_view_fullscreen (GtkAction  *action,
					     GthBrowser *browser)
{
	gth_browser_fullscreen (browser);
}


void
gth_browser_activate_action_view_sort_by (GtkAction  *action,
					  GthBrowser *browser)
{
	dlg_sort_order (browser);
}


void
gth_browser_activate_action_view_thumbnails (GtkAction  *action,
					     GthBrowser *browser)
{
	gth_browser_enable_thumbnails (browser, gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)));
}


void
gth_browser_activate_action_view_toolbar (GtkAction  *action,
					  GthBrowser *browser)
{
	GSettings *settings;

	settings = g_settings_new (GTHUMB_BROWSER_SCHEMA);
	g_settings_set_boolean (settings, PREF_BROWSER_TOOLBAR_VISIBLE, gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)));

	g_object_unref (settings);
}


void
gth_browser_activate_action_view_show_hidden_files (GtkAction  *action,
						    GthBrowser *browser)
{
	GSettings *settings;

	settings = g_settings_new (GTHUMB_BROWSER_SCHEMA);
	g_settings_set_boolean (settings, PREF_BROWSER_SHOW_HIDDEN_FILES, gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)));

	g_object_unref (settings);
}


void
gth_browser_activate_action_view_statusbar (GtkAction  *action,
					    GthBrowser *browser)
{
	GSettings *settings;

	settings = g_settings_new (GTHUMB_BROWSER_SCHEMA);
	g_settings_set_boolean (settings, PREF_BROWSER_STATUSBAR_VISIBLE, gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)));

	g_object_unref (settings);
}


void
gth_browser_activate_action_view_sidebar (GtkAction  *action,
					  GthBrowser *browser)
{
	GSettings *settings;

	settings = g_settings_new (GTHUMB_BROWSER_SCHEMA);
	g_settings_set_boolean (settings, PREF_BROWSER_SIDEBAR_VISIBLE, gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)));

	g_object_unref (settings);
}


void
gth_browser_activate_action_view_thumbnail_list (GtkAction  *action,
						 GthBrowser *browser)
{
	GSettings *settings;

	settings = g_settings_new (GTHUMB_BROWSER_SCHEMA);
	g_settings_set_boolean (settings, PREF_BROWSER_THUMBNAIL_LIST_VISIBLE, gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)));

	g_object_unref (settings);
}


void
gth_browser_activate_action_view_stop (GtkAction  *action,
				       GthBrowser *browser)
{
	gth_browser_stop (browser);
}


void
gth_browser_activate_action_view_reload (GtkAction  *action,
					 GthBrowser *browser)
{
	gth_browser_reload (browser);
}


void
gth_browser_activate_action_folder_open (GtkAction  *action,
					 GthBrowser *browser)
{
	GthFileData *file_data;

	file_data = gth_browser_get_folder_popup_file_data (browser);
	if (file_data == NULL)
		return;

	gth_browser_load_location (browser, file_data->file);

	g_object_unref (file_data);
}


void
gth_browser_activate_action_folder_open_in_new_window (GtkAction  *action,
						       GthBrowser *browser)
{
	GthFileData *file_data;
	GtkWidget   *new_browser;

	file_data = gth_browser_get_folder_popup_file_data (browser);
	if (file_data == NULL)
		return;

	new_browser = gth_browser_new (file_data->file, NULL);
	gtk_window_present (GTK_WINDOW (new_browser));

	g_object_unref (file_data);
}


void
gth_browser_activate_action_view_shrink_wrap (GtkAction  *action,
					      GthBrowser *browser)
{
	gth_browser_set_shrink_wrap_viewer (GTH_BROWSER (browser), gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)));
}


void
gth_browser_activate_action_edit_select_all (GtkAction  *action,
				 	     GthBrowser *browser)
{
	gth_file_selection_select_all (GTH_FILE_SELECTION (gth_browser_get_file_list_view (browser)));
}


/* -- GAction callbacks -- */


void
toggle_action_activated (GSimpleAction *action,
			 GVariant      *parameter,
			 gpointer       data)
{
	GVariant *state;

	state = g_action_get_state (G_ACTION (action));
	g_action_change_state (G_ACTION (action), g_variant_new_boolean (! g_variant_get_boolean (state)));

	g_variant_unref (state);
}


GtkWidget *
_gth_application_get_current_window (GApplication *application)
{
        GList *windows;

        windows = gtk_application_get_windows (GTK_APPLICATION (application));
        if (windows == NULL)
        	return NULL;

        return GTK_WIDGET (windows->data);
}


void
gth_application_activate_new_window (GSimpleAction *action,
				     GVariant      *parameter,
				     gpointer       user_data)
{
        GApplication *application = user_data;
        GtkWidget    *browser;
        GtkWidget    *window;

        browser = _gth_application_get_current_window (application);
        window = gth_browser_new (gth_browser_get_location (GTH_BROWSER (browser)), NULL);
        gtk_window_present (GTK_WINDOW (window));
}


void
gth_application_activate_preferences (GSimpleAction *action,
				      GVariant      *parameter,
				      gpointer       user_data)
{
        GApplication *application = user_data;
        GtkWidget    *browser;

        browser = _gth_application_get_current_window (application);
        dlg_preferences (GTH_BROWSER (browser));
}


void
gth_application_activate_show_help (GSimpleAction *action,
				    GVariant      *parameter,
				    gpointer       user_data)
{
        GApplication *application = user_data;
        GtkWidget    *browser;

        browser = _gth_application_get_current_window (application);
        show_help_dialog (GTK_WINDOW (browser), NULL);
}


void
gth_application_activate_show_shortcuts (GSimpleAction *action,
					 GVariant      *parameter,
					 gpointer       user_data)
{
        GApplication *application = user_data;
        GtkWidget    *browser;

        browser = _gth_application_get_current_window (application);
        show_help_dialog (GTK_WINDOW (browser), "gthumb-shortcuts");
}


void
gth_application_activate_about (GSimpleAction *action,
				GVariant      *parameter,
				gpointer       user_data)
{
        GApplication *application = user_data;
	GthWindow    *window;
	const char   *authors[] = {
#include "AUTHORS.tab"
		NULL
	};
	const char   *documenters [] = {
		"Paolo Bacchilega",
		"Alexander Kirillov",
		NULL
	};
	char       *license_text;
	const char *license[] = {
		N_("gthumb is free software; you can redistribute it and/or modify "
		"it under the terms of the GNU General Public License as published by "
		"the Free Software Foundation; either version 2 of the License, or "
		"(at your option) any later version."),
		N_("gthumb is distributed in the hope that it will be useful, "
		"but WITHOUT ANY WARRANTY; without even the implied warranty of "
		"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the "
		"GNU General Public License for more details."),
		N_("You should have received a copy of the GNU General Public License "
		"along with gthumb.  If not, see http://www.gnu.org/licenses/.")
	};
	GdkPixbuf *logo;

	window = (GthWindow *) _gth_application_get_current_window (application);
	license_text = g_strconcat (_(license[0]), "\n\n",
				    _(license[1]), "\n\n",
				    _(license[2]),
				    NULL);

	logo = gtk_icon_theme_load_icon (gtk_icon_theme_get_for_screen (gtk_widget_get_screen (GTK_WIDGET (window))),
					 "gthumb",
					 128,
					 GTK_ICON_LOOKUP_NO_SVG,
					 NULL);

	gtk_show_about_dialog (GTK_WINDOW (window),
			       "version", VERSION,
			       "copyright", "Copyright \xc2\xa9 2001-2010 Free Software Foundation, Inc.",
			       "comments", _("An image viewer and browser for GNOME."),
			       "authors", authors,
			       "documenters", documenters,
			       "translator-credits", _("translator_credits"),
			       "license", license_text,
			       "wrap-license", TRUE,
			       "website", "http://live.gnome.org/gthumb",
			       (logo != NULL ? "logo" : NULL), logo,
			       NULL);

	_g_object_unref (logo);
	g_free (license_text);
}


void
gth_application_activate_quit (GSimpleAction *action,
			       GVariant      *parameter,
			       gpointer       user_data)
{
        GApplication *application = user_data;
        GList        *windows;

        windows = gtk_application_get_windows (GTK_APPLICATION (application));
        if (windows != NULL)
        	gth_quit (FALSE);
}


void
gth_browser_activate_browser_mode (GSimpleAction *action,
				   GVariant      *parameter,
				   gpointer       user_data)
{
	GthBrowser *browser = user_data;
	GtkWidget  *viewer_sidebar;

	gth_browser_stop (browser);

	viewer_sidebar = gth_browser_get_viewer_sidebar (browser);
	if (gth_sidebar_tool_is_active (GTH_SIDEBAR (viewer_sidebar)))
		gth_sidebar_deactivate_tool (GTH_SIDEBAR (viewer_sidebar));
	else if (gth_browser_get_is_fullscreen (browser))
		gth_browser_unfullscreen (browser);
	else
		gth_window_set_current_page (GTH_WINDOW (browser), GTH_BROWSER_PAGE_BROWSER);
}


void
gth_browser_activate_browser_edit_file (GSimpleAction *action,
					GVariant      *parameter,
					gpointer       user_data)
{
	gth_browser_show_viewer_tools (GTH_BROWSER (user_data));
}


void
gth_browser_activate_browser_properties (GSimpleAction *action,
					 GVariant      *state,
					 gpointer       user_data)
{
	GthBrowser *browser = user_data;

	g_simple_action_set_state (action, state);
	if (g_variant_get_boolean (state))
		gth_browser_show_file_properties (GTH_BROWSER (browser));
	else
		gth_browser_hide_sidebar (GTH_BROWSER (browser));
}


void
gth_browser_activate_clear_history (GSimpleAction *action,
				    GVariant      *parameter,
				    gpointer       user_data)
{
	gth_browser_clear_history (GTH_BROWSER (user_data));
}


void
gth_browser_activate_close (GSimpleAction *action,
			    GVariant      *parameter,
			    gpointer       user_data)
{
	GthBrowser *browser = user_data;

	gth_window_close (GTH_WINDOW (browser));
}


void
gth_browser_activate_fullscreen (GSimpleAction *action,
				 GVariant      *parameter,
				 gpointer       user_data)
{
	gth_browser_fullscreen (GTH_BROWSER (user_data));
}


void
gth_browser_activate_go_back (GSimpleAction *action,
			      GVariant      *parameter,
			      gpointer       user_data)
{
	gth_browser_go_back (GTH_BROWSER (user_data), 1);
}


void
gth_browser_activate_go_forward (GSimpleAction *action,
			         GVariant      *parameter,
			         gpointer       user_data)
{
	gth_browser_go_forward (GTH_BROWSER (user_data), 1);
}


void
gth_browser_activate_go_to_history_pos (GSimpleAction *action,
					GVariant      *parameter,
					gpointer       user_data)
{
	gth_browser_go_to_history_pos (GTH_BROWSER (user_data), atoi (g_variant_get_string (parameter, NULL)));
}


void
gth_browser_activate_go_to_location (GSimpleAction *action,
				     GVariant      *parameter,
				     gpointer       user_data)
{
	GFile *file;

	file = g_file_new_for_uri (g_variant_get_string (parameter, NULL));
	gth_browser_go_to (GTH_BROWSER (user_data), file, NULL);

	g_object_unref (file);
}


void
gth_browser_activate_go_home (GSimpleAction *action,
			      GVariant      *parameter,
			      gpointer       user_data)
{
	gth_browser_go_home (GTH_BROWSER (user_data));
}


void
gth_browser_activate_go_up (GSimpleAction *action,
			    GVariant      *parameter,
			    gpointer       user_data)
{
	gth_browser_go_up (GTH_BROWSER (user_data), 1);
}


void
gth_browser_activate_open_location (GSimpleAction *action,
				    GVariant      *parameter,
				    gpointer       user_data)
{
	dlg_location (GTH_BROWSER (user_data));
}


void
gth_browser_activate_revert_to_saved (GSimpleAction *action,
				      GVariant      *parameter,
				      gpointer       user_data)
{
	GthBrowser *browser = user_data;
	GtkWidget  *viewer_page;

	viewer_page = gth_browser_get_viewer_page (browser);
	if (viewer_page == NULL)
		return;

	gth_viewer_page_revert (GTH_VIEWER_PAGE (viewer_page));
}


void
gth_browser_activate_save (GSimpleAction *action,
			   GVariant      *parameter,
			   gpointer       user_data)
{
	GthBrowser *browser = user_data;
	GtkWidget  *viewer_page;

	viewer_page = gth_browser_get_viewer_page (browser);
	if (viewer_page == NULL)
		return;

	gth_viewer_page_save (GTH_VIEWER_PAGE (viewer_page), NULL, NULL, browser);
}


void
gth_browser_activate_save_as (GSimpleAction *action,
			      GVariant      *parameter,
			      gpointer       user_data)
{
	GthBrowser *browser = user_data;
	GtkWidget  *viewer_page;

	viewer_page = gth_browser_get_viewer_page (browser);
	if (viewer_page == NULL)
		return;

	gth_viewer_page_save_as (GTH_VIEWER_PAGE (viewer_page), NULL, NULL);
}


void
gth_browser_activate_viewer_edit_file (GSimpleAction *action,
				       GVariant      *state,
				       gpointer       user_data)
{
	GthBrowser *browser = user_data;

	g_simple_action_set_state (action, state);
	if (g_variant_get_boolean (state))
		gth_browser_show_viewer_tools (browser);
	else
		gth_browser_hide_sidebar (browser);
}


void
gth_browser_activate_viewer_properties (GSimpleAction *action,
				        GVariant      *state,
				        gpointer       user_data)
{
	GthBrowser *browser = user_data;

	g_simple_action_set_state (action, state);
	if (g_variant_get_boolean (state))
		gth_browser_show_file_properties (browser);
	else
		gth_browser_hide_sidebar (browser);
}


void
gth_browser_activate_unfullscreen (GSimpleAction *action,
				   GVariant      *parameter,
				   gpointer       user_data)
{
	gth_browser_unfullscreen (GTH_BROWSER (user_data));
}
