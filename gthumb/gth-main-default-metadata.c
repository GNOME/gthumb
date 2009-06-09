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
#include "glib-utils.h"
#include "gth-main.h"
#include "gth-metadata-provider.h"
#include "gth-metadata-provider-file.h"


GthMetadataCategory file_metadata_category[] = {
	{ "file", N_("File"), 1 },
	{ NULL, NULL, 0 }
};


GthMetadataInfo file_metadata_info[] = {
	{ "standard::display-name", N_("Name"), "file", 1, GTH_METADATA_ALLOW_EVERYWHERE },
	{ "file::display-size", N_("Size"), "file", 2, GTH_METADATA_ALLOW_EVERYWHERE },
	{ "file::display-ctime", N_("Created"), "file", 3, GTH_METADATA_ALLOW_EVERYWHERE },
	{ "file::display-mtime", N_("Modified"), "file", 4, GTH_METADATA_ALLOW_EVERYWHERE },
	{ "standard::fast-content-type", N_("Type"), "file", 5, GTH_METADATA_ALLOW_EVERYWHERE },
	{ "file::is-modified", NULL, "file", 6, GTH_METADATA_ALLOW_NOWHERE },

	{ "Embedded::Image::DateTime", "", "", 0, GTH_METADATA_ALLOW_NOWHERE },
	{ "Embedded::Image::Comment", "", "", 0, GTH_METADATA_ALLOW_NOWHERE },
	{ "Embedded::Image::Location", "", "", 0, GTH_METADATA_ALLOW_NOWHERE },
	{ "Embedded::Image::Keywords", "", "", 0, GTH_METADATA_ALLOW_NOWHERE },
	{ "Embedded::Image::Orientation", "", "", 0, GTH_METADATA_ALLOW_NOWHERE },

	{ NULL, NULL, NULL, 0, 0 }
};


void
gth_main_register_default_metadata (void)
{
	gth_main_register_metadata_category (file_metadata_category);
	gth_main_register_metadata_info_v (file_metadata_info);
	gth_main_register_metadata_provider (GTH_TYPE_METADATA_PROVIDER_FILE);
}
