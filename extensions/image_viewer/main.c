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
#include <gtk/gtk.h>
#include <gthumb.h>
#include "gth-image-viewer-page.h"
#include "gth-metadata-provider-image.h"
#include "preferences.h"


GthMetadataCategory image_metadata_category[] = {
	{ "image", N_("Image"), 5 },
	{ NULL, NULL, 0 }
};


GthMetadataInfo image_metadata_info[] = {
	{ "image::size", N_("Dimensions"), "image", 1, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW | GTH_METADATA_ALLOW_IN_FILE_LIST },
	{ "image::format", N_("Format"), "image", 2, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ NULL, NULL, NULL, 0, 0 }
};


G_MODULE_EXPORT void
gthumb_extension_activate (void)
{
	gth_main_register_metadata_category (image_metadata_category);
	gth_main_register_metadata_info_v (image_metadata_info);
	gth_main_register_metadata_provider (GTH_TYPE_METADATA_PROVIDER_IMAGE);
	gth_main_register_object (GTH_TYPE_VIEWER_PAGE, NULL, GTH_TYPE_IMAGE_VIEWER_PAGE, NULL);
	gth_hook_add_callback ("dlg-preferences-construct", 10, G_CALLBACK (image_viewer__dlg_preferences_construct_cb), NULL);
}


G_MODULE_EXPORT void
gthumb_extension_deactivate (void)
{
}


G_MODULE_EXPORT gboolean
gthumb_extension_is_configurable (void)
{
	return FALSE;
}


G_MODULE_EXPORT void
gthumb_extension_configure (GtkWindow *parent)
{
}
