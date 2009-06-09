/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2008 Free Software Foundation, Inc.
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
#include "dlg-bookmarks.h"
#include "dlg-edit-metadata.h"
#include "dlg-personalize-filters.h"
#include "dlg-preferences.h"
#include "dlg-sort-order.h"
#include "gconf-utils.h"
#include "glib-utils.h"
#include "gth-browser.h"
#include "gth-file-selection.h"
#include "gth-main.h"
#include "gth-preferences.h"
#include "gth-viewer-page.h"
#include "gtk-utils.h"


void
gth_browser_activate_action_bookmarks_add (GtkAction  *action,
					   GthBrowser *browser)
{
	GBookmarkFile *bookmarks;
	GFile         *location;
	char          *uri;

	location = gth_browser_get_location (browser);
	if (location == NULL)
		return;

	bookmarks = gth_main_get_default_bookmarks ();
	uri = g_file_get_uri (location);
	_g_bookmark_file_add_uri (bookmarks, uri);
	gth_main_bookmarks_changed ();

	g_free (uri);
}


void
gth_browser_activate_action_bookmarks_edit (GtkAction  *action,
					    GthBrowser *browser)
{
	dlg_bookmarks (browser);
}


void
gth_browser_activate_action_file_open_with (GtkAction  *action,
					    GthBrowser *browser)
{
	/* FIXME */
}


void
gth_browser_activate_action_file_revert (GtkAction  *action,
					 GthBrowser *browser)
{
	gth_browser_load_file (browser, gth_browser_get_current_file (browser), TRUE);
}


void
gth_browser_activate_action_file_save (GtkAction  *action,
				       GthBrowser *browser)
{
	GtkWidget *viewer_page;

	viewer_page = gth_browser_get_viewer_page (browser);
	if (viewer_page == NULL)
		return;

	gth_viewer_page_save (GTH_VIEWER_PAGE (viewer_page), NULL, NULL, browser);
}


void
gth_browser_activate_action_file_save_as (GtkAction  *action,
					  GthBrowser *browser)
{
	GtkWidget *viewer_page;

	viewer_page = gth_browser_get_viewer_page (browser);
	if (viewer_page == NULL)
		return;

	gth_viewer_page_save_as (GTH_VIEWER_PAGE (viewer_page), NULL, NULL);
}


void
gth_browser_activate_action_file_new_window (GtkAction  *action,
					     GthBrowser *browser)
{
	GtkWidget *window;

	window = gth_browser_new (NULL);
	gth_browser_go_to (GTH_BROWSER (window), gth_browser_get_location (browser));
	gtk_window_present (GTK_WINDOW (window));
}


void
gth_browser_activate_action_edit_preferences (GtkAction  *action,
					      GthBrowser *browser)
{
	dlg_preferences (browser);
}


void
gth_browser_activate_action_go_back (GtkAction  *action,
				     GthBrowser *browser)
{
	gth_browser_go_back (browser, 1);
}


void
gth_browser_activate_action_go_forward (GtkAction  *action,
					GthBrowser *browser)
{
	gth_browser_go_forward (browser, 1);
}


void
gth_browser_activate_action_go_up (GtkAction  *action,
				   GthBrowser *browser)
{
	gth_browser_go_up (browser, 1);
}


void
gth_browser_activate_action_go_home (GtkAction  *action,
				     GthBrowser *browser)
{
	gth_browser_go_home (browser);
}


void
gth_browser_activate_action_go_clear_history (GtkAction  *action,
					      GthBrowser *browser)
{
	gth_browser_clear_history (browser);
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
	eel_gconf_set_boolean (PREF_UI_TOOLBAR_VISIBLE, gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)));
}


void
gth_browser_activate_action_view_show_hidden_files (GtkAction  *action,
						    GthBrowser *browser)
{
	eel_gconf_set_boolean (PREF_SHOW_HIDDEN_FILES, gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)));
}


void
gth_browser_activate_action_view_statusbar (GtkAction  *action,
					    GthBrowser *browser)
{
	eel_gconf_set_boolean (PREF_UI_STATUSBAR_VISIBLE, gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)));
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
gth_browser_activate_action_view_prev (GtkAction  *action,
				       GthBrowser *browser)
{
	gth_browser_show_prev_image (browser, FALSE, FALSE);
}


void
gth_browser_activate_action_view_next (GtkAction  *action,
				       GthBrowser *browser)
{
	gth_browser_show_next_image (browser, FALSE, FALSE);
}


void
gth_browser_activate_action_folder_open (GtkAction  *action,
					 GthBrowser *browser)
{
}


void
gth_browser_activate_action_folder_open_in_new_window (GtkAction  *action,
						       GthBrowser *browser)
{
}


void
gth_browser_activate_action_browser_mode (GtkAction  *action,
					  GthBrowser *browser)
{
	gth_window_set_current_page (GTH_WINDOW (browser), GTH_BROWSER_PAGE_BROWSER);
}


void
gth_browser_activate_action_viewer_properties (GtkAction  *action,
						GthBrowser *browser)
{
	gth_browser_show_viewer_properties (GTH_BROWSER (browser), gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)));
}


void
gth_browser_activate_action_viewer_tools (GtkAction  *action,
					  GthBrowser *browser)
{
	gth_browser_show_viewer_tools (GTH_BROWSER (browser), gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)));
}


void
gth_browser_activate_action_edit_metadata (GtkAction  *action,
				 	   GthBrowser *browser)
{
	dlg_edit_metadata (browser);
}


void
gth_browser_activate_action_edit_select_all (GtkAction  *action,
				 	     GthBrowser *browser)
{
	gth_file_selection_select_all (GTH_FILE_SELECTION (gth_browser_get_file_list_view (browser)));
}


void
gth_browser_activate_action_help_about (GtkAction *action,
				        gpointer   data)
{
	GthWindow  *window = GTH_WINDOW (data);
	const char *authors[] = {
#include "AUTHORS.tab"
		"",
		NULL
	};
	const char *documenters [] = {
		"Paolo Bacchilega",
		"Alexander Kirillov",
		NULL
	};
	char       *license_text;
	const char *license[] = {
		"gthumb is free software; you can redistribute it and/or modify "
		"it under the terms of the GNU General Public License as published by "
		"the Free Software Foundation; either version 2 of the License, or "
		"(at your option) any later version.",
		"gthumb is distributed in the hope that it will be useful, "
		"but WITHOUT ANY WARRANTY; without even the implied warranty of "
		"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the "
		"GNU General Public License for more details.",
		"You should have received a copy of the GNU General Public License "
		"along with gthumb; if not, write to the Free Software Foundation, Inc., "
		"51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA"
	};

	license_text = g_strconcat (license[0], "\n\n", license[1], "\n\n",
				    license[2], "\n\n", NULL);


	gtk_show_about_dialog (GTK_WINDOW (window),
			       "version", VERSION,
			       "copyright", "Copyright \xc2\xa9 2001-2009 Free Software Foundation, Inc.",
			       "comments", _("An image viewer and browser for GNOME."),
			       "authors", authors,
			       "documenters", documenters,
			       "translator-credits", _("translator_credits"),
			       "logo-icon-name", "gthumb",
			       "license", license_text,
			       "wrap-license", TRUE,
			       "website", "http://gthumb.sourceforge.net",
			       NULL);

	g_free (license_text);
}


void
gth_browser_activate_action_help_help (GtkAction *action,
				       gpointer   data)
{
	show_help_dialog (GTK_WINDOW (data), NULL);
}


void
gth_browser_activate_action_help_shortcuts (GtkAction *action,
					    gpointer   data)
{
	show_help_dialog (GTK_WINDOW (data), "shortcuts");
}
