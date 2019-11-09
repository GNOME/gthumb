/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2019 Free Software Foundation, Inc.
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
#include "callbacks.h"


static const GthShortcut shortcuts[] = {
	{ "image-zoom-in", N_("Zoom In"), GTH_SHORTCUT_CONTEXT_VIEWER, GTH_SHORTCUT_CATEGORY_IMAGE_VIEW, "plus" },
	{ "image-zoom-out", N_("Zoom Out"), GTH_SHORTCUT_CONTEXT_VIEWER, GTH_SHORTCUT_CATEGORY_IMAGE_VIEW, "minus" },
	{ "image-zoom-100", N_("Zoom 100%"), GTH_SHORTCUT_CONTEXT_VIEWER, GTH_SHORTCUT_CATEGORY_IMAGE_VIEW, "1" },
	{ "image-zoom-200", N_("Zoom 200%"), GTH_SHORTCUT_CONTEXT_VIEWER, GTH_SHORTCUT_CATEGORY_IMAGE_VIEW, "2" },
	{ "image-zoom-300", N_("Zoom 300%"), GTH_SHORTCUT_CONTEXT_VIEWER, GTH_SHORTCUT_CATEGORY_IMAGE_VIEW, "3" },

	{ "image-zoom-fit", N_("Zoom To Fit"), GTH_SHORTCUT_CONTEXT_VIEWER, GTH_SHORTCUT_CATEGORY_IMAGE_VIEW, "<shift>X" },
	{ "image-zoom-fit-if-larger", N_("Zoom To Fit If Larger"), GTH_SHORTCUT_CONTEXT_VIEWER, GTH_SHORTCUT_CATEGORY_IMAGE_VIEW, "X" },
	{ "image-zoom-fit-width", N_("Zoom To Fit Width"), GTH_SHORTCUT_CONTEXT_VIEWER, GTH_SHORTCUT_CATEGORY_IMAGE_VIEW, "<shift>W" },
	{ "image-zoom-fit-width-if-larger", N_("Zoom To Fit Width If Larger"), GTH_SHORTCUT_CONTEXT_VIEWER, GTH_SHORTCUT_CATEGORY_IMAGE_VIEW, "W" },
	{ "image-zoom-fit-height", N_("Zoom To Fit Height"), GTH_SHORTCUT_CONTEXT_VIEWER, GTH_SHORTCUT_CATEGORY_IMAGE_VIEW, "<shift>H" },
	{ "image-zoom-fit-height-if-larger", N_("Zoom To Fit Height If Larger"), GTH_SHORTCUT_CONTEXT_VIEWER, GTH_SHORTCUT_CATEGORY_IMAGE_VIEW, "H" },

	{ "image-undo", N_("Undo Edit"), GTH_SHORTCUT_CONTEXT_VIEWER, GTH_SHORTCUT_CATEGORY_IMAGE_VIEW, "<control>Z" },
	{ "image-redo", N_("Redo Edit"), GTH_SHORTCUT_CONTEXT_VIEWER, GTH_SHORTCUT_CATEGORY_IMAGE_VIEW, "<shift><control>Z" },
};


void
image_viewer__gth_browser_construct_cb (GthBrowser *browser)
{
	g_return_if_fail (GTH_IS_BROWSER (browser));

	gth_window_add_shortcuts (GTH_WINDOW (browser),
				  shortcuts,
				  G_N_ELEMENTS (shortcuts));
}
