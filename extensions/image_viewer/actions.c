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
gth_browser_activate_image_zoom_fit_if_larger (GSimpleAction	*action,
					       GVariant		*parameter,
					       gpointer		 user_data)
{
	GthBrowser	   *browser = user_data;
	GthImageViewerPage *self = GTH_IMAGE_VIEWER_PAGE (gth_browser_get_viewer_page (browser));

	gth_image_viewer_set_fit_mode (GTH_IMAGE_VIEWER (gth_image_viewer_page_get_image_viewer (self)), GTH_FIT_SIZE_IF_LARGER);
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
gth_browser_activate_image_zoom_fit_height (GSimpleAction	*action,
					    GVariant		*parameter,
					    gpointer		 user_data)
{
	GthBrowser	   *browser = user_data;
	GthImageViewerPage *self = GTH_IMAGE_VIEWER_PAGE (gth_browser_get_viewer_page (browser));

	gth_image_viewer_set_fit_mode (GTH_IMAGE_VIEWER (gth_image_viewer_page_get_image_viewer (self)), GTH_FIT_HEIGHT);
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


void
gth_browser_activate_apply_icc_profile  (GSimpleAction	*action,
					 GVariant	*state,
					 gpointer	 user_data)
{
	GthBrowser *browser = user_data;

	g_simple_action_set_state (action, state);
	gth_image_viewer_page_apply_icc_profile (GTH_IMAGE_VIEWER_PAGE (gth_browser_get_viewer_page (browser)), g_variant_get_boolean (state));
}


void
gth_browser_activate_transparency_style (GSimpleAction	*action,
					 GVariant	*parameter,
					 gpointer	 user_data)
{
	GthBrowser     *browser = user_data;
	const char     *state;
	GthImageViewer *image_viewer;

	state = g_variant_get_string (parameter, NULL);
	g_simple_action_set_state (action, g_variant_new_string (state));

	if (state == NULL)
		return;

	image_viewer = GTH_IMAGE_VIEWER (gth_image_viewer_page_get_image_viewer (GTH_IMAGE_VIEWER_PAGE (gth_browser_get_viewer_page (browser))));
	if (strcmp (state, "checkered") == 0)
		gth_image_viewer_set_transparency_style (image_viewer, GTH_TRANSPARENCY_STYLE_CHECKERED);
	else if (strcmp (state, "white") == 0)
		gth_image_viewer_set_transparency_style (image_viewer, GTH_TRANSPARENCY_STYLE_WHITE);
	else if (strcmp (state, "gray") == 0)
		gth_image_viewer_set_transparency_style (image_viewer, GTH_TRANSPARENCY_STYLE_GRAY);
	else if (strcmp (state, "black") == 0)
		gth_image_viewer_set_transparency_style (image_viewer, GTH_TRANSPARENCY_STYLE_BLACK);
}


void
gth_browser_activate_image_zoom  (GSimpleAction	*action,
				  GVariant	*parameter,
				  gpointer	 user_data)
{
	GthBrowser     *browser = user_data;
	const char     *state;
	GthImageViewer *image_viewer;

	state = g_variant_get_string (parameter, NULL);
	g_simple_action_set_state (action, g_variant_new_string (state));

	if (state == NULL)
		return;

	image_viewer = GTH_IMAGE_VIEWER (gth_image_viewer_page_get_image_viewer (GTH_IMAGE_VIEWER_PAGE (gth_browser_get_viewer_page (browser))));
	if (strcmp (state, "automatic") == 0)
		gth_image_viewer_set_fit_mode (image_viewer, GTH_FIT_SIZE_IF_LARGER);
	else if (strcmp (state, "fit") == 0)
		gth_image_viewer_set_fit_mode (image_viewer, GTH_FIT_SIZE);
	else if (strcmp (state, "fit-width") == 0)
		gth_image_viewer_set_fit_mode (image_viewer, GTH_FIT_WIDTH);
	else if (strcmp (state, "fit-height") == 0)
		gth_image_viewer_set_fit_mode (image_viewer, GTH_FIT_HEIGHT);
	else if (strcmp (state, "50") == 0)
		gth_image_viewer_set_zoom (image_viewer, 0.5);
	else if (strcmp (state, "100") == 0)
		gth_image_viewer_set_zoom (image_viewer, 1.0);
	else if (strcmp (state, "200") == 0)
		gth_image_viewer_set_zoom (image_viewer, 2.0);
	else if (strcmp (state, "300") == 0)
		gth_image_viewer_set_zoom (image_viewer, 3.0);
}


void
gth_browser_activate_toggle_animation (GSimpleAction	*action,
				       GVariant		*state,
				       gpointer	 	 user_data)
{
	GthBrowser     *browser = user_data;
	GthViewerPage  *viewer_page;
	GthImageViewer *image_viewer;

	g_simple_action_set_state (action, state);

	viewer_page = gth_browser_get_viewer_page (browser);
	image_viewer = GTH_IMAGE_VIEWER (gth_image_viewer_page_get_image_viewer (GTH_IMAGE_VIEWER_PAGE (viewer_page)));

	if (gth_image_viewer_is_playing_animation (image_viewer))
		gth_image_viewer_stop_animation (image_viewer);
	else
		gth_image_viewer_start_animation (image_viewer);
	gth_viewer_page_update_sensitivity (viewer_page);
}


void
gth_browser_activate_step_animation (GSimpleAction	*action,
				     GVariant		*state,
				     gpointer	 	 user_data)
{
	GthBrowser     *browser = user_data;
	GthViewerPage  *viewer_page;
	GthImageViewer *image_viewer;

	viewer_page = gth_browser_get_viewer_page (browser);
	image_viewer = GTH_IMAGE_VIEWER (gth_image_viewer_page_get_image_viewer (GTH_IMAGE_VIEWER_PAGE (viewer_page)));
	gth_image_viewer_step_animation (image_viewer);
}
