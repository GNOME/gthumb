/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2012 Free Software Foundation, Inc.
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
#include <gthumb.h>


static void
gth_browser_activate_action_show_work_queue (GthBrowser *browser,
					     int         n_queue)
{
	char  *uri;
	GFile *location;

	uri = g_strdup_printf ("queue:///%d", n_queue);
	location = g_file_new_for_uri (uri);
	gth_browser_load_location (browser, location);

	g_free (uri);
	g_object_unref (location);
}


void
gth_browser_activate_action_go_queue_1 (GtkAction  *action,
					GthBrowser *browser)
{
	gth_browser_activate_action_show_work_queue (browser, 1);
}


void
gth_browser_activate_action_go_queue_2 (GtkAction  *action,
					GthBrowser *browser)
{
	gth_browser_activate_action_show_work_queue (browser, 2);
}


void
gth_browser_activate_action_go_queue_3 (GtkAction  *action,
					GthBrowser *browser)
{
	gth_browser_activate_action_show_work_queue (browser, 3);
}
