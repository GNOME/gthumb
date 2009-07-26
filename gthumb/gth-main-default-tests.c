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
#include "gth-test-simple.h"


static gint64
is_file_test (GthTest       *test,
	      GthFileData   *file,
	      gconstpointer *data)
{
	return TRUE;
}


static gint64
is_image_test (GthTest       *test,
	       GthFileData   *file,
	       gconstpointer *data)
{
	gboolean result = FALSE;

	if (file->info != NULL)
		result = _g_mime_type_is_image (gth_file_data_get_mime_type (file));

	return result;
}


static gint64
is_media_test (GthTest       *test,
	       GthFileData   *file,
	       gconstpointer *data)
{
	gboolean result = FALSE;

	if (file->info != NULL) {
		const char *content_type = gth_file_data_get_mime_type (file);
		result = (_g_mime_type_is_image (content_type)
			  || _g_mime_type_is_video (content_type)
			  || _g_mime_type_is_audio (content_type));
	}

	return result;
}


static gint64
is_text_test (GthTest       *test,
	      GthFileData   *file,
	      gconstpointer *data)
{
	gboolean result = FALSE;

	if (file->info != NULL) {
		const char *content_type = gth_file_data_get_mime_type (file);
		result = g_content_type_is_a (content_type, "text/*");
	}

	return result;
}


static gint64
get_filename_for_test (GthTest       *test,
		       GthFileData   *file,
		       gconstpointer *data)
{
	*data = g_file_info_get_display_name (file->info);
	return 0;
}


static gint64
get_filesize_for_test (GthTest       *test,
		       GthFileData   *file,
		       gconstpointer *data)
{
	return g_file_info_get_size (file->info);
}


void
gth_main_register_default_tests (void)
{
	gth_main_register_test ("file::type::is_file",
				GTH_TYPE_TEST_SIMPLE,
				"display-name", _("All Files"),
				"data-type", GTH_TEST_DATA_TYPE_NONE,
				"get-data-func", is_file_test,
				NULL);
	gth_main_register_test ("file::type::is_image",
				GTH_TYPE_TEST_SIMPLE,
				"display-name", _("Images"),
				"data-type", GTH_TEST_DATA_TYPE_NONE,
				"get-data-func", is_image_test,
				NULL);
	gth_main_register_test ("file::type::is_media",
				GTH_TYPE_TEST_SIMPLE,
				"display-name", _("Media"),
				"data-type", GTH_TEST_DATA_TYPE_NONE,
				"get-data-func", is_media_test,
				NULL);
	gth_main_register_test ("file::type::is_text",
				GTH_TYPE_TEST_SIMPLE,
				"display-name", _("Text Files"),
				"data-type", GTH_TEST_DATA_TYPE_NONE,
				"get-data-func", is_text_test,
				NULL);
	gth_main_register_test ("file::name",
				GTH_TYPE_TEST_SIMPLE,
				"attributes", "gth::file::display-name",
				"display-name", _("Filename"),
				"data-type", GTH_TEST_DATA_TYPE_STRING,
				"get-data-func", get_filename_for_test,
				NULL);
	gth_main_register_test ("file::size",
				GTH_TYPE_TEST_SIMPLE,
				"attributes", "gth::file::size",
				"display-name", _("Size"),
				"data-type", GTH_TEST_DATA_TYPE_SIZE,
				"get-data-func", get_filesize_for_test,
				NULL);
}
