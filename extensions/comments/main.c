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
#include "gth-comment.h"
#include "gth-edit-comment-page.h"
#include "gth-metadata-provider-comment.h"
#include "gth-test-category.h"


GthMetadataCategory comments_metadata_category[] = {
	{ "comment", N_("Comment"), 20 },
	{ NULL, NULL, 0 }
};


GthMetadataInfo comments_metadata_info[] = {
	{ "comment::note", N_("Comment"), "comment", 1, GTH_METADATA_ALLOW_NOWHERE },
	{ "comment::place", N_("Place"), "comment", 2, GTH_METADATA_ALLOW_EVERYWHERE },
	{ "comment::time", N_("Date"), "comment", 3, GTH_METADATA_ALLOW_EVERYWHERE },
	{ "comment::categories", N_("Tags"), "comment", 4, GTH_METADATA_ALLOW_EVERYWHERE },
	{ "comment::rating", N_("Rating"), "comment", 5, GTH_METADATA_ALLOW_EVERYWHERE },
	{ NULL, NULL, NULL, 0, 0 }
};


static gint64
get_comment_for_test (GthTest       *test,
		      GthFileData   *file,
		      gconstpointer *data)
{
	*data = g_file_info_get_attribute_string (file->info, "comment::note");
	return 0;
}


static gint64
get_place_for_test (GthTest       *test,
		    GthFileData   *file,
		    gconstpointer *data)
{
	*data = g_file_info_get_attribute_string (file->info, "comment::place");
	return 0;
}


void
comments__add_sidecars_cb (GFile  *file,
			   GList **sidecars)
{
	*sidecars = g_list_prepend (*sidecars, gth_comment_get_comment_file (file));
}


G_MODULE_EXPORT void
gthumb_extension_activate (void)
{
	gth_main_register_metadata_category (comments_metadata_category);
	gth_main_register_metadata_info_v (comments_metadata_info);
	gth_main_register_metadata_provider (GTH_TYPE_METADATA_PROVIDER_COMMENT);
	gth_main_register_type ("edit-metadata-dialog-page", GTH_TYPE_EDIT_COMMENT_PAGE);
	gth_main_register_test ("comment::note",
				GTH_TYPE_TEST_SIMPLE,
				"display-name", _("Comment"),
				"data-type", GTH_TEST_DATA_TYPE_STRING,
				"get-data-func", get_comment_for_test,
				NULL);
	gth_main_register_test ("comment::place",
				GTH_TYPE_TEST_SIMPLE,
				"display-name", _("Place"),
				"data-type", GTH_TEST_DATA_TYPE_STRING,
				"get-data-func", get_place_for_test,
				NULL);
	gth_main_register_test ("comment::category",
				GTH_TYPE_TEST_CATEGORY,
				"display-name", _("Tag"),
				NULL);
	gth_hook_add_callback ("add-sidecars", 10, G_CALLBACK (comments__add_sidecars_cb), NULL);
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
