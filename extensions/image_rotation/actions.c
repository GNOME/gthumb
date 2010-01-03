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
#include <gthumb.h>
#include "gth-transform-task.h"


void
gth_browser_activate_action_tool_rotate_right (GtkAction  *action,
					       GthBrowser *browser)
{
	GList   *items;
	GList   *file_list;
	GthTask *task;

	items = gth_file_selection_get_selected (GTH_FILE_SELECTION (gth_browser_get_file_list_view (browser)));
	file_list = gth_file_list_get_files (GTH_FILE_LIST (gth_browser_get_file_list (browser)), items);
	task = gth_transform_task_new (browser, file_list, GTH_TRANSFORM_ROTATE_90);
	gth_browser_exec_task (browser, task, FALSE);

	g_object_unref (task);
	_g_object_list_unref (file_list);
	_gtk_tree_path_list_free (items);
}


void
gth_browser_activate_action_tool_rotate_left (GtkAction  *action,
					      GthBrowser *browser)
{
	GList   *items;
	GList   *file_list;
	GthTask *task;

	items = gth_file_selection_get_selected (GTH_FILE_SELECTION (gth_browser_get_file_list_view (browser)));
	file_list = gth_file_list_get_files (GTH_FILE_LIST (gth_browser_get_file_list (browser)), items);
	task = gth_transform_task_new (browser, file_list, GTH_TRANSFORM_ROTATE_270);
	gth_browser_exec_task (browser, task, FALSE);

	g_object_unref (task);
	_g_object_list_unref (file_list);
	_gtk_tree_path_list_free (items);
}
