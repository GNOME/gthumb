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
#include <gthumb.h>
#include "gth-image-print-job.h"


void
gth_browser_activate_action_file_print (GtkAction  *action,
					GthBrowser *browser)
{
	GList *items;
	GList *file_list;

	items = gth_file_selection_get_selected (GTH_FILE_SELECTION (gth_browser_get_file_list_view (browser)));
	file_list = gth_file_list_get_files (GTH_FILE_LIST (gth_browser_get_file_list (browser)), items);
	if (file_list != NULL) {
		GthImagePrintJob *print_job;
		GError           *error = NULL;

		print_job = gth_image_print_job_new (file_list, &error);
		if (print_job != NULL)
			gth_image_print_job_run (print_job,
						 GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG,
						 browser);
		else
			_gtk_error_dialog_from_gerror_run (GTK_WINDOW (browser), _("Could not print the selected files"), &error);
	}

	_g_object_list_unref (file_list);
	_gtk_tree_path_list_free (items);
}
