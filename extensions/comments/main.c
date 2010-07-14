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
#include "dlg-comments-preferences.h"
#include "gth-comment.h"
#include "gth-metadata-provider-comment.h"
#include "preferences.h"


GthMetadataCategory comments_metadata_category[] = {
	{ "comment", N_("Comment"), 20 },
	{ NULL, NULL, 0 }
};


GthMetadataInfo comments_metadata_info[] = {
	{ "comment::caption", "", "comment", 1, NULL, GTH_METADATA_ALLOW_NOWHERE },
	{ "comment::note", "", "comment", 2, NULL, GTH_METADATA_ALLOW_NOWHERE },
	{ "comment::place", "", "comment", 3, NULL, GTH_METADATA_ALLOW_NOWHERE },
	{ "comment::time", "", "comment", 4, NULL, GTH_METADATA_ALLOW_NOWHERE },
	{ "comment::categories", "", "comment", 5, NULL, GTH_METADATA_ALLOW_NOWHERE },
	{ "comment::rating", "", "comment", 6, NULL, GTH_METADATA_ALLOW_NOWHERE },
	{ NULL, NULL, NULL, 0, 0 }
};


static gint64
get_comment_for_test (GthTest        *test,
		      GthFileData    *file,
		      gconstpointer  *data,
		      GDestroyNotify *data_destroy_func)
{
	*data = g_file_info_get_attribute_string (file->info, "comment::note");
	return 0;
}


static gint64
get_place_for_test (GthTest        *test,
		    GthFileData    *file,
		    gconstpointer  *data,
		    GDestroyNotify *data_destroy_func)
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
	GthStringList *comment_categories;
	GList         *scan;
	const char    *text;
	GthComment    *comment;
	GthStringList *categories;

	if (! eel_gconf_get_boolean (PREF_STORE_METADATA_IN_FILES, TRUE)) {
		/* if PREF_STORE_METADATA_IN_FILES is false, avoid to
		 * synchronize the .comment metadata because the embedded
		 * metadata is likely to be out-of-date.
		 * Give priority to the .comment metadata which, if present,
		 * is the most up-to-date. */

		gth_comment_update_general_attributes (file_data);

		return;
	}

	if (! eel_gconf_get_boolean (PREF_COMMENTS_SYNCHRONIZE, TRUE))
		return;

	comment = gth_comment_new ();
	gth_comment_set_note (comment, g_file_info_get_attribute_string (file_data->info, "comment::note"));
	gth_comment_set_caption (comment, g_file_info_get_attribute_string (file_data->info, "comment::caption"));
	gth_comment_set_place (comment, g_file_info_get_attribute_string (file_data->info, "comment::place"));

	metadata = (GthMetadata *) g_file_info_get_attribute_object (file_data->info, "comment::time");
	if (metadata != NULL)
		gth_comment_set_time_from_exif_format (comment, gth_metadata_get_raw (metadata));

	comment_categories = (GthStringList *) g_file_info_get_attribute_object (file_data->info, "comment::categories");
	if (comment_categories != NULL)
		for (scan = gth_string_list_get_list (comment_categories); scan; scan = scan->next)
			gth_comment_add_category (comment, (char *) scan->data);

	/* sync embedded data and .comment data if required */

	metadata = (GthMetadata *) g_file_info_get_attribute_object (file_data->info, "general::description");
	if (metadata != NULL) {
		text = g_file_info_get_attribute_string (file_data->info, "comment::note");
		if (g_strcmp0 (gth_metadata_get_formatted (metadata), text) != 0) {
			gth_comment_set_note (comment, gth_metadata_get_formatted (metadata));
			write_comment = TRUE;
		}
	}

	metadata = (GthMetadata *) g_file_info_get_attribute_object (file_data->info, "general::title");
	if (metadata != NULL) {
		text = g_file_info_get_attribute_string (file_data->info, "comment::caption");
		if (g_strcmp0 (gth_metadata_get_formatted (metadata), text) != 0) {
			gth_comment_set_caption (comment, gth_metadata_get_formatted (metadata));
			write_comment = TRUE;
		}
	}

	metadata = (GthMetadata *) g_file_info_get_attribute_object (file_data->info, "general::location");
	if (metadata != NULL) {
		text = g_file_info_get_attribute_string (file_data->info, "comment::place");
		if (g_strcmp0 (gth_metadata_get_formatted (metadata), text) != 0) {
			gth_comment_set_place (comment, gth_metadata_get_formatted (metadata));
			write_comment = TRUE;
		}
	}

	metadata = (GthMetadata *) g_file_info_get_attribute_object (file_data->info, "general::datetime");
	if (metadata != NULL) {
		text = gth_metadata_get_raw (metadata);
		metadata = (GthMetadata *) g_file_info_get_attribute_object (file_data->info, "comment::time");
		if (metadata != NULL) {
			if (g_strcmp0 (gth_metadata_get_raw (metadata), text) != 0) {
				gth_comment_set_time_from_exif_format (comment, gth_metadata_get_raw (metadata));
				write_comment = TRUE;
			}
		}
	}

	categories = (GthStringList *) g_file_info_get_attribute_object (file_data->info, "general::tags");
	if (categories != NULL) {
		comment_categories = (GthStringList *) g_file_info_get_attribute_object (file_data->info, "comment::categories");
		if (! gth_string_list_equal (categories, comment_categories)) {
			GList *scan;

			gth_comment_clear_categories (comment);
			for (scan = gth_string_list_get_list (categories); scan; scan = scan->next)
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
	gth_main_register_object (GTH_TYPE_TEST,
				  "comment::note",
				  GTH_TYPE_TEST_SIMPLE,
				  "attributes", "comment::note",
				  "display-name", _("Description"),
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
				  "attributes", "comment::categories",
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
	return TRUE;
}


G_MODULE_EXPORT void
gthumb_extension_configure (GtkWindow *parent)
{
	dlg_comments_preferences (parent);
}
