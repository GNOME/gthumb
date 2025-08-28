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
#include <glib.h>
#include <gthumb.h>
#include "gth-comment.h"
#include "gth-metadata-provider-comment.h"


struct _GthMetadataProviderCommentPrivate {
	GHashTable *checked_folders;
};


G_DEFINE_TYPE_WITH_CODE (GthMetadataProviderComment,
			 gth_metadata_provider_comment,
			 GTH_TYPE_METADATA_PROVIDER,
			 G_ADD_PRIVATE (GthMetadataProviderComment))


static gboolean
gth_metadata_provider_comment_can_read (GthMetadataProvider  *base,
					GthFileData          *file_data,
					const char           *mime_type,
					char                **attribute_v)

{
	return _g_file_attributes_matches_any_v ("comment::*,"
						 "general::datetime,"
						 "general::title,"
						 "general::description,"
						 "general::location,"
						 "general::tags,"
						 "general::rating",
						 attribute_v);
}


static gboolean
gth_metadata_provider_comment_can_write (GthMetadataProvider  *self,
				         const char           *mime_type,
				         char                **attribute_v)
{
	return _g_file_attributes_matches_any_v ("comment::*,"
						 "general::datetime,"
						 "general::title,"
						 "general::description,"
						 "general::location,"
						 "general::tags,"
						 "general::rating",
					         attribute_v);
}


static void
gth_metadata_provider_comment_read (GthMetadataProvider *base,
				    GthFileData         *file_data,
				    const char          *attributes,
				    GCancellable        *cancellable)
{
	GthComment *comment;
	const char *value;
	GPtrArray  *categories;
	char       *comment_time;

	// Check if the .comment directory exists
	GFile *comment_file = gth_comment_get_comment_file (file_data->file);
	if (comment_file != NULL) {
		GFile *comment_folder = g_file_get_parent (comment_file);
		if (comment_folder != NULL) {
			GthMetadataProviderComment *self = GTH_METADATA_PROVIDER_COMMENT (base);
			gpointer value = g_hash_table_lookup (self->priv->checked_folders, comment_folder);
			gboolean comment_folder_exists;
			if (value == NULL) {
				comment_folder_exists = g_file_query_exists (comment_folder, NULL);
				g_hash_table_insert (self->priv->checked_folders,
					g_object_ref (comment_folder),
					GINT_TO_POINTER (comment_folder_exists ? 1 : 2));
			}
			else {
				comment_folder_exists = GPOINTER_TO_INT (value) == 1;
			}
			if (!comment_folder_exists) {
				g_file_info_set_attribute_boolean (file_data->info, "comment::no-comment-file", TRUE);
				return;
			}
		}
	}

	comment = gth_comment_new_for_file (file_data->file, cancellable, NULL);
	g_file_info_set_attribute_boolean (file_data->info, "comment::no-comment-file", (comment == NULL));

	if (comment == NULL)
		return;

	value = gth_comment_get_note (comment);
	if (value != NULL)
		g_file_info_set_attribute_string (file_data->info, "comment::note", value);

	value = gth_comment_get_caption (comment);
	if (value != NULL)
		g_file_info_set_attribute_string (file_data->info, "comment::caption", value);

	value = gth_comment_get_place (comment);
	if (value != NULL)
		g_file_info_set_attribute_string (file_data->info, "comment::place", value);

	if (gth_comment_get_rating (comment) > 0)
		g_file_info_set_attribute_int32 (file_data->info, "comment::rating", gth_comment_get_rating (comment));
	else
		g_file_info_remove_attribute (file_data->info, "comment::rating");

	categories = gth_comment_get_categories (comment);
	if (categories->len > 0) {
		GthStringList *list;
		GthMetadata   *metadata;

		list =  gth_string_list_new_from_ptr_array (categories);
		metadata = gth_metadata_new_for_string_list (list);
		g_file_info_set_attribute_object (file_data->info, "comment::categories", G_OBJECT (metadata));

		g_object_unref (metadata);
		g_object_unref (list);
	}
	else
		g_file_info_remove_attribute (file_data->info, "comment::categories");

	comment_time = gth_comment_get_time_as_exif_format (comment);
	if (comment_time != NULL) {
		GTimeVal  time_;
		char     *formatted;

		if (_g_time_val_from_exif_date (comment_time, &time_))
			formatted = _g_time_val_strftime (&time_, "%x %X");
		else
			formatted = g_strdup (comment_time);
		set_attribute_from_string (file_data->info, "comment::time", comment_time, formatted);

		g_free (formatted);
		g_free (comment_time);
	}
	else
		g_file_info_remove_attribute (file_data->info, "comment::time");

	// Update the general attributes if the comment file is not older
	// than the image file or the attributes were not updated from the
	// embedded data.
	gboolean update_general_attributes = TRUE;
	if (g_file_info_has_attribute (file_data->info, "embedded::updated-general-attributes")) {
		if (g_file_info_get_attribute_boolean (file_data->info, "embedded::updated-general-attributes")) {
			GDateTime *file_modification_time = g_file_info_get_modification_date_time (file_data->info);
			if (gth_comment_file_is_older (comment, file_modification_time)) {
				update_general_attributes = FALSE;
			}
		}
	}
	if (update_general_attributes) {
		gth_comment_update_general_attributes (file_data);
	}

	g_object_unref (comment);
}


static void
gth_metadata_provider_comment_write (GthMetadataProvider   *self,
				     GthMetadataWriteFlags  flags,
				     GthFileData           *file_data,
				     const char            *attributes,
				     GCancellable          *cancellable)
{
	GthComment    *comment;
	GthMetadata   *metadata;
	const char    *text;
	char          *data;
	gsize          length;
	GthStringList *categories;
	GFile         *comment_file;
	GFile         *comment_folder;

	comment = gth_comment_new ();

	/* caption */

	text = NULL;
	metadata = (GthMetadata *) g_file_info_get_attribute_object (file_data->info, "general::title");
	if (metadata != NULL)
		text = gth_metadata_get_raw (metadata);
	gth_comment_set_caption (comment, text);

	/* comment */

	text = NULL;
	metadata = (GthMetadata *) g_file_info_get_attribute_object (file_data->info, "general::description");
	if (metadata != NULL)
		text = gth_metadata_get_raw (metadata);
	gth_comment_set_note (comment, text);

	/* location */

	text = NULL;
	metadata = (GthMetadata *) g_file_info_get_attribute_object (file_data->info, "general::location");
	if (metadata != NULL)
		text = gth_metadata_get_raw (metadata);
	gth_comment_set_place (comment, text);

	/* time */

	text = NULL;
	metadata = (GthMetadata *) g_file_info_get_attribute_object (file_data->info, "general::datetime");
	if (metadata != NULL)
		text = gth_metadata_get_raw (metadata);
	gth_comment_set_time_from_exif_format (comment, text);

	/* keywords */

	metadata = (GthMetadata *) g_file_info_get_attribute_object (file_data->info, "general::tags");
	categories = gth_metadata_get_string_list (metadata);
	if (categories != NULL) {
		GList *list;
		GList *scan;

		list = gth_string_list_get_list (categories);
		for (scan = list; scan; scan = scan->next)
			gth_comment_add_category (comment, (char *) scan->data);
	}

	/* rating */

	metadata = (GthMetadata *) g_file_info_get_attribute_object (file_data->info, "general::rating");
	if (metadata != NULL) {
		int rating;

		sscanf (gth_metadata_get_raw (metadata), "%d", &rating);
		gth_comment_set_rating (comment, rating);
	}

	data = gth_comment_to_data (comment, &length);
	comment_file = gth_comment_get_comment_file (file_data->file);
	comment_folder = g_file_get_parent (comment_file);

	g_file_make_directory (comment_folder, NULL, NULL);
	_g_file_write (comment_file, FALSE, 0, data, length, cancellable, NULL);

	g_object_unref (comment_folder);
	g_object_unref (comment_file);
	g_free (data);
	g_object_unref (comment);
}


static int
gth_metadata_provider_comment_get_priority (GthMetadataProvider *self)
{
	// Lower than exiv2, and other embedded metadata readers, to make
	// sure the comment reader is executed after all other readers.
	return 20;
}


static void
gth_metadata_provider_comment_finalize (GObject *object)
{
	GthMetadataProviderComment *self;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GTH_IS_METADATA_PROVIDER_COMMENT (object));

	self = GTH_METADATA_PROVIDER_COMMENT (object);
	g_hash_table_unref (self->priv->checked_folders);

	/* Chain up */
	G_OBJECT_CLASS (gth_metadata_provider_comment_parent_class)->finalize (object);
}


static void
gth_metadata_provider_comment_class_init (GthMetadataProviderCommentClass *klass)

{
	GObjectClass             *gobject_class;
	GthMetadataProviderClass *mp_class;

	gobject_class = G_OBJECT_CLASS (klass);
	gobject_class->finalize = gth_metadata_provider_comment_finalize;

	mp_class = GTH_METADATA_PROVIDER_CLASS (klass);
	mp_class->can_read = gth_metadata_provider_comment_can_read;
	mp_class->can_write = gth_metadata_provider_comment_can_write;
	mp_class->read = gth_metadata_provider_comment_read;
	mp_class->write = gth_metadata_provider_comment_write;
	mp_class->get_priority = gth_metadata_provider_comment_get_priority;
}


static void
gth_metadata_provider_comment_init (GthMetadataProviderComment *self)
{
	self->priv = gth_metadata_provider_comment_get_instance_private (self);
	self->priv->checked_folders = g_hash_table_new_full (g_file_hash, (GEqualFunc) g_file_equal, g_object_unref, NULL);
}
