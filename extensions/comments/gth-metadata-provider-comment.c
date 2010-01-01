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
#include <glib.h>
#include <gthumb.h>
#include "gth-comment.h"
#include "gth-metadata-provider-comment.h"


static gpointer parent_class = NULL;


static gboolean
gth_metadata_provider_comment_can_read (GthMetadataProvider  *self,
				        const char           *mime_type,
				        char                **attribute_v)
{
	return _g_file_attributes_matches_any_v ("comment::*,"
						 "general::datetime,"
						 "general::description,"
						 "general::location,"
						 "general::tags",
					         attribute_v);
}


static gboolean
gth_metadata_provider_comment_can_write (GthMetadataProvider  *self,
				         const char           *mime_type,
				         char                **attribute_v)
{
	return _g_file_attributes_matches_any_v ("comment::*,"
						 "general::datetime,"
						 "general::description,"
						 "general::location,"
						 "general::tags",
					         attribute_v);
}


static void
set_attribute_from_string (GFileInfo  *info,
			   const char *key,
			   const char *raw,
			   const char *formatted)
{
	GthMetadata *metadata;

	metadata = g_object_new (GTH_TYPE_METADATA,
				 "id", key,
				 "raw", raw,
				 "formatted", (formatted != NULL ? formatted : raw),
				 NULL);
	g_file_info_set_attribute_object (info, key, G_OBJECT (metadata));

	g_object_unref (metadata);
}


static void
gth_metadata_provider_comment_read (GthMetadataProvider *self,
				    GthFileData         *file_data,
				    const char          *attributes)
{
	GthComment            *comment;
	GFileAttributeMatcher *matcher;
	const char            *value;
	GPtrArray             *categories;
	char                  *comment_time;

	comment = gth_comment_new_for_file (file_data->file, NULL);
	if (comment == NULL)
		return;

	matcher = g_file_attribute_matcher_new (attributes);

	value = gth_comment_get_note (comment);
	if (value != NULL) {
		g_file_info_set_attribute_string (file_data->info, "comment::note", value);
		set_attribute_from_string (file_data->info, "general::description", value, NULL);
	}

	value = gth_comment_get_place (comment);
	if (value != NULL) {
		g_file_info_set_attribute_string (file_data->info, "comment::place", value);
		set_attribute_from_string (file_data->info, "general::location", value, NULL);
	}

	categories = gth_comment_get_categories (comment);
	if (categories->len > 0) {
		GObject *value;

		value = (GObject *) gth_string_list_new_from_ptr_array (categories);
		g_file_info_set_attribute_object (file_data->info, "comment::categories", value);
		g_file_info_set_attribute_object (file_data->info, "general::tags", value);
		g_object_unref (value);
	}

	comment_time = gth_comment_get_time_as_exif_format (comment);
	if (comment_time != NULL) {
		GTimeVal  time_;
		char     *formatted;

		if (_g_time_val_from_exif_date (comment_time, &time_))
			formatted = _g_time_val_strftime (&time_, "%x %X");
		else
			formatted = g_strdup (comment_time);
		set_attribute_from_string (file_data->info, "comment::time", comment_time, formatted);
		set_attribute_from_string (file_data->info, "general::datetime", comment_time, formatted);

		g_free (formatted);
		g_free (comment_time);
	}

	g_file_attribute_matcher_unref (matcher);
	g_object_unref (comment);
}


static void
gth_metadata_provider_comment_write (GthMetadataProvider *self,
				     GthFileData         *file_data,
				     const char          *attributes)
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

	/* comment */

	metadata = (GthMetadata *) g_file_info_get_attribute_object (file_data->info, "general::description");
	if (metadata == NULL)
		text = g_file_info_get_attribute_string (file_data->info, "comment::note");
	else
		text = gth_metadata_get_raw (metadata);
	gth_comment_set_note (comment, text);

	/* location */

	metadata = (GthMetadata *) g_file_info_get_attribute_object (file_data->info, "general::location");
	if (metadata == NULL)
		text = g_file_info_get_attribute_string (file_data->info, "comment::place");
	else
		text = gth_metadata_get_raw (metadata);
	gth_comment_set_place (comment, text);

	/* time */

	metadata = (GthMetadata *) g_file_info_get_attribute_object (file_data->info, "general::datetime");
	if (metadata == NULL)
		metadata = (GthMetadata *) g_file_info_get_attribute_object (file_data->info, "comment::time");
	if (metadata != NULL)
		text = gth_metadata_get_raw (metadata);
	else
		text = NULL;
	gth_comment_set_time_from_exif_format (comment, text);

	/* keywords */

	categories = (GthStringList *) g_file_info_get_attribute_object (file_data->info, "general::tags");
	if (categories == NULL)
		categories = (GthStringList *) g_file_info_get_attribute_object (file_data->info, "comment::categories");
	if (categories != NULL) {
		GList *list;
		GList *scan;

		list = gth_string_list_get_list (categories);
		for (scan = list; scan; scan = scan->next)
			gth_comment_add_category (comment, (char *) scan->data);
	}

	data = gth_comment_to_data (comment, &length);
	comment_file = gth_comment_get_comment_file (file_data->file);
	comment_folder = g_file_get_parent (comment_file);

	g_file_make_directory (comment_folder, NULL, NULL);
	g_write_file (comment_file, FALSE, 0, data, length, NULL, NULL);

	g_object_unref (comment_folder);
	g_object_unref (comment_file);
	g_free (data);
	g_object_unref (comment);
}


static void
gth_metadata_provider_comment_class_init (GthMetadataProviderCommentClass *klass)
{
	parent_class = g_type_class_peek_parent (klass);

	GTH_METADATA_PROVIDER_CLASS (klass)->can_read = gth_metadata_provider_comment_can_read;
	GTH_METADATA_PROVIDER_CLASS (klass)->can_write = gth_metadata_provider_comment_can_write;
	GTH_METADATA_PROVIDER_CLASS (klass)->read = gth_metadata_provider_comment_read;
	GTH_METADATA_PROVIDER_CLASS (klass)->write = gth_metadata_provider_comment_write;
}


GType
gth_metadata_provider_comment_get_type (void)
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (GthMetadataProviderCommentClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_metadata_provider_comment_class_init,
			NULL,
			NULL,
			sizeof (GthMetadataProviderComment),
			0,
			(GInstanceInitFunc) NULL
		};

		type = g_type_register_static (GTH_TYPE_METADATA_PROVIDER,
					       "GthMetadataProviderComment",
					       &type_info,
					       0);
	}

	return type;
}
