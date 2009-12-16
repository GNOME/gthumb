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
	{ "comment::note", "", "comment", 1, GTH_METADATA_ALLOW_NOWHERE },
	{ "comment::place", "", "comment", 2, GTH_METADATA_ALLOW_NOWHERE },
	{ "comment::time", "", "comment", 3, GTH_METADATA_ALLOW_NOWHERE },
	{ "comment::categories", "", "comment", 4, GTH_METADATA_ALLOW_NOWHERE },
	{ "comment::rating", "", "comment", 5, GTH_METADATA_ALLOW_NOWHERE },
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


void
comments__read_metadata_ready_cb (GthFileData *file_data,
				  const char  *attributes)
{
	gboolean       write_comment = FALSE;
	GthMetadata   *metadata;
	const char    *text;
	GthComment    *comment;
	GPtrArray     *keywords;
	int            i;
	GthStringList *categories;

	comment = gth_comment_new ();
	gth_comment_set_note (comment, g_file_info_get_attribute_string (file_data->info, "comment::note"));
	gth_comment_set_place (comment, g_file_info_get_attribute_string (file_data->info, "comment::place"));

	metadata = (GthMetadata *) g_file_info_get_attribute_object (file_data->info, "comment::time");
	if (metadata != NULL)
		gth_comment_set_time_from_exif_format (comment, gth_metadata_get_raw (metadata));

	keywords = gth_comment_get_categories (comment);
	for (i = 0; i < keywords->len; i++)
		gth_comment_add_category (comment, g_ptr_array_index (keywords, i));

	/* sync embedded data and .comment data if required */

	metadata = (GthMetadata *) g_file_info_get_attribute_object (file_data->info, "general::comment");
	if (metadata != NULL) {
		text = g_file_info_get_attribute_string (file_data->info, "comment::note");
		if (g_strcmp0 (gth_metadata_get_raw (metadata), text) != 0) {
			gth_comment_set_note (comment, gth_metadata_get_raw (metadata));
			write_comment = TRUE;
		}
	}

	metadata = (GthMetadata *) g_file_info_get_attribute_object (file_data->info, "general::location");
	if (metadata != NULL) {
		text = g_file_info_get_attribute_string (file_data->info, "comment::place");
		if (g_strcmp0 (gth_metadata_get_raw (metadata), text) != 0) {
			gth_comment_set_place (comment, gth_metadata_get_raw (metadata));
			write_comment = TRUE;
		}
	}

	metadata = (GthMetadata *) g_file_info_get_attribute_object (file_data->info, "general::datetime");
	if (metadata != NULL) {
		metadata = (GthMetadata *) g_file_info_get_attribute_object (file_data->info, "comment::time");
		if (metadata != NULL) {
			text = gth_metadata_get_raw (metadata);
			if (g_strcmp0 (gth_metadata_get_raw (metadata), text) != 0) {
				gth_comment_set_time_from_exif_format (comment, gth_metadata_get_raw (metadata));
				write_comment = TRUE;
			}
		}
	}

	categories = (GthStringList *) g_file_info_get_attribute_object (file_data->info, "general::tags");
	if (categories != NULL) {
		GthStringList *comment_categories;

		comment_categories = (GthStringList *) g_file_info_get_attribute_object (file_data->info, "comment::categories");
		if (! gth_string_list_equal (categories, comment_categories)) {
			GList *list;
			GList *scan;

			gth_comment_clear_categories (comment);
			list = gth_string_list_get_list (categories);
			for (scan = list; scan; scan = scan->next)
				gth_comment_add_category (comment, scan->data);
			write_comment = TRUE;
		}
	}

	if (write_comment) {
		GFile *comment_file;
		char  *buffer;
		gsize  size;

		buffer = gth_comment_to_data (comment, &size);
		comment_file = gth_comment_get_comment_file (file_data->file);
		g_write_file (comment_file,
			      FALSE,
			      G_FILE_CREATE_NONE,
			      buffer,
			      size,
			      NULL,
			      NULL);

		g_object_unref (comment_file);
		g_free (buffer);
	}

	g_object_unref (comment);
}


G_MODULE_EXPORT void
gthumb_extension_activate (void)
{
	gth_main_register_metadata_category (comments_metadata_category);
	gth_main_register_metadata_info_v (comments_metadata_info);
	gth_main_register_metadata_provider (GTH_TYPE_METADATA_PROVIDER_COMMENT);
	gth_main_register_type ("edit-metadata-dialog-page", GTH_TYPE_EDIT_COMMENT_PAGE);
	gth_main_register_object (GTH_TYPE_TEST,
				  "comment::note",
				  GTH_TYPE_TEST_SIMPLE,
				  "attributes", "comment::note",
				  "display-name", _("Comment"),
				  "data-type", GTH_TEST_DATA_TYPE_STRING,
				  "get-data-func", get_comment_for_test,
				  NULL);
	gth_main_register_object (GTH_TYPE_TEST,
				  "comment::place",
				  GTH_TYPE_TEST_SIMPLE,
				  "attributes", "comment::place",
				  "display-name", _("Place"),
				  "data-type", GTH_TEST_DATA_TYPE_STRING,
				  "get-data-func", get_place_for_test,
				  NULL);
	gth_main_register_object (GTH_TYPE_TEST,
				  "comment::category",
				  GTH_TYPE_TEST_CATEGORY,
				  "display-name", _("Tag"),
				  NULL);
	gth_hook_add_callback ("add-sidecars", 10, G_CALLBACK (comments__add_sidecars_cb), NULL);
	gth_hook_add_callback ("read-metadata-ready", 10, G_CALLBACK (comments__read_metadata_ready_cb), NULL);
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
