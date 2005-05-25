/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2005 Free Software Foundation, Inc.
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

#include "gth-window.h"
#include "gth-viewer.h"
#include "image-viewer.h"
#include "comments.h"
#include "dlg-catalog.h"
#include "main.h"
#include "file-utils.h"


void
gth_viewer_activate_action_file_new_window (GtkAction *action,
					    gpointer   data)
{
	GthWindow *window = GTH_WINDOW (data);
	GtkWidget *new_viewer;

	new_viewer = gth_viewer_new (gth_window_get_image_filename (window));
	gtk_widget_show (new_viewer);
}


void
gth_viewer_activate_action_file_revert (GtkAction *action,
					gpointer   data)
{	
	GthViewer *viewer = data;
	gth_viewer_load (viewer, gth_window_get_image_filename (GTH_WINDOW (viewer)));
}


void
gth_viewer_activate_action_edit_edit_comment (GtkAction *action,
					      gpointer   data)
{
	gth_window_edit_comment ((GthWindow*)data);
}


void
gth_viewer_activate_action_edit_delete_comment (GtkAction *action,
						gpointer   data)
{
	GthWindow *window = data;
	GList     *list, *scan;

	list = gth_window_get_file_list_selection (window);
	for (scan = list; scan; scan = scan->next) {
		char        *filename = scan->data;
		CommentData *cdata;

		cdata = comments_load_comment (filename);
		comment_data_free_comment (cdata);
		if (! comment_data_is_void (cdata))
			comments_save_comment (filename, cdata);
		else
			comment_delete (filename);
		comment_data_free (cdata);

		all_windows_notify_update_comment (filename);
	}
	path_list_free (list);

	gth_window_update_current_image_metadata (window);
}


void
gth_viewer_activate_action_edit_edit_categories (GtkAction *action,
						 gpointer   data)
{
	gth_window_edit_categories ((GthWindow*)data);
}


void
gth_viewer_activate_action_edit_add_to_catalog (GtkAction *action,
						gpointer   data)
{	
	GthWindow *window = data;
	GList     *list;

	list = gth_window_get_file_list_selection (window);
	dlg_add_to_catalog (window, list);

	/* the list is deallocated when the dialog is closed. */
}


void
gth_viewer_activate_action_view_toolbar (GtkAction *action,
					 gpointer   data)
{
	/*
	eel_gconf_set_boolean (PREF_UI_TOOLBAR_VISIBLE, gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)));
	*/
}


void
gth_viewer_activate_action_view_statusbar (GtkAction *action,
				gpointer   data)
{
	/*
	eel_gconf_set_boolean (PREF_UI_STATUSBAR_VISIBLE, gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)));
	*/
}


void
gth_viewer_activate_action_view_show_info (GtkAction *action,
					   gpointer   data)
{
	/*
	GthWindow *window = data;

	if (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)))
		window_show_image_data (window);
	else
		window_hide_image_data (window);
	*/
}
