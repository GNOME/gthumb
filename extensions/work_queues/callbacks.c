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
#include "gth-file-source-work-queues.h"
#include "gth-queue-manager.h"


static void
gth_browser_activate_action_add_to_work_queue (GthBrowser *browser,
					       int         n_queue)
{
	char  *uri;
	GFile *folder;
	GList *items;
	GList *file_list = NULL;
	GList *files;

	uri = g_strdup_printf ("queue:///%d", n_queue);
	folder = g_file_new_for_uri (uri);
	items = gth_file_selection_get_selected (GTH_FILE_SELECTION (gth_browser_get_file_list_view (browser)));
	file_list = gth_file_list_get_files (GTH_FILE_LIST (gth_browser_get_file_list (browser)), items);
	files = gth_file_data_list_to_file_list (file_list);
	gth_queue_manager_add_files (folder, files, -1);

	_g_object_list_unref (files);
	_g_object_list_unref (file_list);
	_gtk_tree_path_list_free (items);
	g_object_unref (folder);
	g_free (uri);
}


gpointer
work_queues__gth_browser_file_list_key_press_cb (GthBrowser  *browser,
						 GdkEventKey *event)
{
	gpointer result = NULL;
	guint    modifiers;

	modifiers = gtk_accelerator_get_default_mod_mask ();
	if ((event->state & modifiers) != GDK_MOD1_MASK)
		return NULL;

	switch (gdk_keyval_to_lower (event->keyval)) {
	case GDK_KEY_1:
	case GDK_KEY_2:
	case GDK_KEY_3:
		gth_browser_activate_action_add_to_work_queue (browser, gdk_keyval_to_lower (event->keyval) - GDK_KEY_1 + 1);
		result = GINT_TO_POINTER (1);
		break;
	}

	return result;
}
