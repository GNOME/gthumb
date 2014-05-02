/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2013 Free Software Foundation, Inc.
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
#include "actions.h"
#include "gth-image-viewer-page.h"


void
gth_browser_activate_image_zoom_in (GSimpleAction	*action,
				    GVariant		*parameter,
				    gpointer		 user_data)
{
	GthBrowser	   *browser = user_data;
	GthImageViewerPage *self = GTH_IMAGE_VIEWER_PAGE (gth_browser_get_viewer_page (browser));

	gth_image_viewer_zoom_in (GTH_IMAGE_VIEWER (gth_image_viewer_page_get_image_viewer (self)));
}


void
gth_browser_activate_image_zoom_out (GSimpleAction	*action,
				     GVariant		*parameter,
				     gpointer		 user_data)
{
	GthBrowser	   *browser = user_data;
	GthImageViewerPage *self = GTH_IMAGE_VIEWER_PAGE (gth_browser_get_viewer_page (browser));

	gth_image_viewer_zoom_out (GTH_IMAGE_VIEWER (gth_image_viewer_page_get_image_viewer (self)));
}


void
gth_browser_activate_image_zoom_100 (GSimpleAction	*action,
				     GVariant		*parameter,
				     gpointer		 user_data)
{
	GthBrowser	   *browser = user_data;
	GthImageViewerPage *self = GTH_IMAGE_VIEWER_PAGE (gth_browser_get_viewer_page (browser));

	gth_image_viewer_set_zoom (GTH_IMAGE_VIEWER (gth_image_viewer_page_get_image_viewer (self)), 1.0);
}


void
gth_browser_activate_image_zoom_fit (GSimpleAction	*action,
				     GVariant		*parameter,
				     gpointer		 user_data)
{
	GthBrowser	   *browser = user_data;
	GthImageViewerPage *self = GTH_IMAGE_VIEWER_PAGE (gth_browser_get_viewer_page (browser));

	gth_image_viewer_set_fit_mode (GTH_IMAGE_VIEWER (gth_image_viewer_page_get_image_viewer (self)), GTH_FIT_SIZE);
}


void
gth_browser_activate_image_zoom_fit_width (GSimpleAction	*action,
					   GVariant		*parameter,
					   gpointer		 user_data)
{
	GthBrowser	   *browser = user_data;
	GthImageViewerPage *self = GTH_IMAGE_VIEWER_PAGE (gth_browser_get_viewer_page (browser));

	gth_image_viewer_set_fit_mode (GTH_IMAGE_VIEWER (gth_image_viewer_page_get_image_viewer (self)), GTH_FIT_WIDTH);
}


void
gth_browser_activate_image_undo (GSimpleAction	*action,
				 GVariant	*parameter,
				 gpointer	 user_data)
{
	GthBrowser *browser = user_data;
	gth_image_viewer_page_undo (GTH_IMAGE_VIEWER_PAGE (gth_browser_get_viewer_page (browser)));
}


void
gth_browser_activate_image_redo (GSimpleAction	*action,
				 GVariant	*parameter,
				 gpointer	 user_data)
{
	GthBrowser *browser = user_data;
	gth_image_viewer_page_redo (GTH_IMAGE_VIEWER_PAGE (gth_browser_get_viewer_page (browser)));
}


void
gth_browser_activate_copy_image (GSimpleAction	*action,
				 GVariant	*parameter,
				 gpointer	 user_data)
{
	GthBrowser *browser = user_data;
	gth_image_viewer_page_copy_image (GTH_IMAGE_VIEWER_PAGE (gth_browser_get_viewer_page (browser)));
}


void
gth_browser_activate_paste_image (GSimpleAction	*action,
				  GVariant	*parameter,
				  gpointer	 user_data)
{
	GthBrowser *browser = user_data;
	gth_image_viewer_page_paste_image (GTH_IMAGE_VIEWER_PAGE (gth_browser_get_viewer_page (browser)));
}
