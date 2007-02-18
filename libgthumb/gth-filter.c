/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2006 Free Software Foundation, Inc.
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
#include <string.h>
#include <glib/gi18n.h>
#include <glib.h>
#include "gth-filter.h"


/* GthTest */


struct _GthTest {
	int           ref_count;
	GthTestScope  scope;
	GthTestOp     op;
	gboolean      negative;
	union {
		char  *s;
		int    i;
		GDate *date;
	} data;
	GPatternSpec *pattern;
};


GthTest *
gth_test_new (GthTestScope scope,
	      GthTestOp    op,
	      gboolean     negative)
{
	GthTest *test;

	test = g_new0 (GthTest, 1);
	test->ref_count = 1;
	test->scope = scope;
	test->op = op;
	test->negative = negative;

	return test;
}


GthTest *
gth_test_new_with_int (GthTestScope scope,
	               GthTestOp    op,
	               gboolean     negavite,
	               int          data)
{
	GthTest *test;

	test = gth_test_new (scope, op, negavite);
	test->data.i = data;

	return test;
}


GthTest *
gth_test_new_with_string (GthTestScope  scope,
	                  GthTestOp     op,
	                  gboolean      negavite,
	                  const char   *data)
{
	GthTest *test;

	test = gth_test_new (scope, op, negavite);
	if (data != NULL)
		test->data.s = g_utf8_casefold (data, -1);
	else
		test->data.s = NULL;

	return test;
}


GthTest *
gth_test_new_with_date (GthTestScope  scope,
	                GthTestOp     op,
	                gboolean      negavite,
	                GDate        *date)
{
	GthTest *test;

	g_return_val_if_fail (date != NULL, NULL);

	test = gth_test_new (scope, op, negavite);
	test->data.date = g_date_new_dmy (g_date_get_day (date),
					  g_date_get_month (date),
					  g_date_get_year (date));

	return test;
}


void
gth_test_ref (GthTest *test)
{
	test->ref_count++;
}


void
gth_test_unref (GthTest *test)
{
	test->ref_count--;
	if (test->ref_count > 0)
		return;

	switch (test->scope) {
	case GTH_TEST_SCOPE_FILENAME:
	case GTH_TEST_SCOPE_COMMENT:
	case GTH_TEST_SCOPE_PLACE:
	case GTH_TEST_SCOPE_KEYWORDS:
	case GTH_TEST_SCOPE_ALL:
		g_free (test->data.s);
		break;
	case GTH_TEST_SCOPE_DATE:
		g_date_free (test->data.date);
		break;
	default:
		break;
	}
	if (test->pattern != NULL)
		g_pattern_spec_free (test->pattern);
	g_free (test);
}


static gboolean
test_string (GthTest    *test,
	     const char *value)
{
	gboolean  result = FALSE;
	char     *value2;

	if ((test->data.s == NULL) || (value == NULL))
		return FALSE;

	value2 = g_utf8_casefold (value, -1);

	switch (test->op) {
	case GTH_TEST_OP_EQUAL:
		result = g_utf8_collate (value2, test->data.s) == 0;
		break;
	case GTH_TEST_OP_LOWER:
		result = g_utf8_collate (value2, test->data.s) < 0;
		break;
	case GTH_TEST_OP_GREATER:
		result = g_utf8_collate (value2, test->data.s) > 0;
		break;
	case GTH_TEST_OP_CONTAINS:
		result = g_strstr_len (value2, -1, test->data.s) != NULL;
		break;
	case GTH_TEST_OP_STARTS_WITH:
		result = g_str_has_prefix (value2, test->data.s);
		break;
	case GTH_TEST_OP_ENDS_WITH:
		result = g_str_has_suffix (value2, test->data.s);
		break;
	case GTH_TEST_OP_MATCHES:
		if (test->pattern == NULL)
			test->pattern = g_pattern_spec_new (test->data.s);
		result = g_pattern_match_string (test->pattern, value2);
		break;
	default:
		break;
	}

	if (test->negative)
		result = ! result;

	g_free (value2);

	return result;
}


static gboolean
test_integer (GthTest *test,
	      int      value)
{
	gboolean result = FALSE;

	switch (test->op) {
	case GTH_TEST_OP_EQUAL:
		result = value == test->data.i;
		break;
	case GTH_TEST_OP_LOWER:
		result = value < test->data.i;
		break;
	case GTH_TEST_OP_GREATER:
		result = value > test->data.i;
		break;
	default:
		break;
	}

	return result;
}


static gboolean
test_keywords (GthTest  *test,
 	       char    **keywords,
 	       int       keywords_n)
{
	gboolean result;
	int      i;

	if ((test->data.s == NULL) || (keywords == NULL) || (keywords_n == 0))
		return test->negative;

	if ((test->op != GTH_TEST_OP_CONTAINS)
	    && (test->op != GTH_TEST_OP_CONTAINS_ALL))
		return test->negative;

	result = (test->op == GTH_TEST_OP_CONTAINS_ALL);
	for (i = 0; i < keywords_n; i++) {
		char     *value2 = g_utf8_casefold (keywords[i], -1);
		gboolean  partial_result;

		partial_result = g_utf8_collate (value2, test->data.s) == 0;
		g_free (value2);

		if (partial_result) {
			if (test->op == GTH_TEST_OP_CONTAINS) {
				result = TRUE;
				break;
			}
		}
		else if (test->op == GTH_TEST_OP_CONTAINS_ALL) {
			result = FALSE;
			break;
		}
	}

	if (test->negative)
		result = ! result;

	return result;
}


static gboolean
test_date (GthTest *test,
	   time_t   time)
{
	gboolean result = FALSE;
	GDate    *date;
	int       compare;

	date = g_date_new ();
	g_date_set_time_t (date, time);

	compare = g_date_compare (date, test->data.date);

	switch (test->op) {
	case GTH_TEST_OP_EQUAL:
		result = (compare == 0);
		break;
	case GTH_TEST_OP_BEFORE:
		result = (compare < 0);
		break;
	case GTH_TEST_OP_AFTER:
		result = (compare > 0);
		break;
	default:
		break;
	}

	return result;
}


gboolean
gth_test_match (GthTest  *test,
		FileData *fdata)
{
	gboolean result = FALSE;

	switch (test->scope) {
	case GTH_TEST_SCOPE_FILENAME:
		result = test_string (test, fdata->display_name);
		break;
	case GTH_TEST_SCOPE_COMMENT:
		file_data_load_comment_data (fdata);
		if (fdata->comment_data != NULL)
			result = test_string (test, fdata->comment_data->comment);
		else
			result = test->negative;
		break;
	case GTH_TEST_SCOPE_PLACE:
		file_data_load_comment_data (fdata);
		if (fdata->comment_data != NULL)
			result = test_string (test, fdata->comment_data->place);
		break;
	case GTH_TEST_SCOPE_ALL:
		file_data_load_comment_data (fdata);
		if (fdata->comment_data != NULL) {
			result = (test_string (test, fdata->display_name)
		        	  || test_string (test, fdata->comment_data->comment)
		          	  || test_string (test, fdata->comment_data->place));
		        if (! result && (fdata->comment_data != NULL))
		        	result = test_keywords (test, fdata->comment_data->keywords, fdata->comment_data->keywords_n);
		} else
			result = test->negative;
		break;
	case GTH_TEST_SCOPE_SIZE:
		result = test_integer (test, fdata->size);
		break;
	case GTH_TEST_SCOPE_KEYWORDS:
		if (fdata->comment_data != NULL)
			result = test_keywords (test, fdata->comment_data->keywords, fdata->comment_data->keywords_n);
		else
			result = test->negative;
		break;
	case GTH_TEST_SCOPE_DATE:
		file_data_load_exif_data (fdata);
		if (fdata->exif_time != 0)
			result = test_date (test, fdata->exif_time);
		else
			result = test_date (test, fdata->mtime);
		break;
	case GTH_TEST_SCOPE_WIDTH:
	case GTH_TEST_SCOPE_HEIGHT:
		break;
	}

	return result;
}


/* GthFilter */


struct _GthFilterPrivate
{
	gboolean          match_all_tests;
	int               max_images, current_images;
	GnomeVFSFileSize  max_size, current_size;
	GList            *tests;
};


static GObjectClass *parent_class = NULL;


static void
gth_filter_finalize (GObject *object)
{
	GthFilter *filter;

	filter = GTH_FILTER (object);

	if (filter->priv != NULL) {
		g_list_foreach (filter->priv->tests, (GFunc) gth_test_unref, NULL);
		g_list_free (filter->priv->tests);

		g_free (filter->priv);
		filter->priv = NULL;
	}

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gth_filter_class_init (GthFilterClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;

	object_class->finalize = gth_filter_finalize;
}


static void
gth_filter_init (GthFilter *filter)
{
	filter->priv = g_new0 (GthFilterPrivate, 1);
	filter->priv->max_images = 0;
	filter->priv->max_size = 0;
	gth_filter_reset (filter);
}


GType
gth_filter_get_type (void)
{
        static GType type = 0;

        if (! type) {
                GTypeInfo type_info = {
			sizeof (GthFilterClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_filter_class_init,
			NULL,
			NULL,
			sizeof (GthFilter),
			0,
			(GInstanceInitFunc) gth_filter_init
		};

		type = g_type_register_static (G_TYPE_OBJECT,
					       "GthFilter",
					       &type_info,
					       0);
	}

        return type;
}


GthFilter*
gth_filter_new (void)
{
	return GTH_FILTER (g_object_new (GTH_TYPE_FILTER, NULL));
}


void
gth_filter_set_max_images (GthFilter *filter,
			   int        max_images)
{
	filter->priv->max_images = max_images;
}


int
gth_filter_get_max_images (GthFilter *filter)
{
	return filter->priv->max_images;
}


void
gth_filter_set_max_size (GthFilter        *filter,
			 GnomeVFSFileSize  max_size)
{
	filter->priv->max_size = max_size;
}


GnomeVFSFileSize
gth_filter_get_max_size (GthFilter *filter)
{
	return filter->priv->max_size;
}


void
gth_filter_set_match_all (GthFilter *filter,
			  gboolean   match_all)
{
	filter->priv->match_all_tests = match_all;
}


gboolean
gth_filter_get_match_all (GthFilter *filter)
{
	return filter->priv->match_all_tests;
}


void
gth_filter_add_test (GthFilter *filter,
		     GthTest   *test)
{
	filter->priv->tests = g_list_append (filter->priv->tests, test);
}


void
gth_filter_reset (GthFilter *filter)
{
	filter->priv->current_images = 0;
	filter->priv->current_size = 0;
}


gboolean
gth_filter_match (GthFilter *filter,
		  FileData  *fdata)
{
	gboolean  filter_matched;
	GList    *scan;

	if ((filter->priv->max_images > 0)
	    && (filter->priv->current_images > filter->priv->max_images))
		return FALSE;

	if ((filter->priv->max_size > 0)
	    && (filter->priv->current_size > filter->priv->max_size))
		return FALSE;

	filter_matched = filter->priv->match_all_tests;
	for (scan = filter->priv->tests; scan; scan = scan->next) {
		GthTest *test = scan->data;
		if (gth_test_match (test, fdata)) {
			if (! filter->priv->match_all_tests) {
				filter_matched = TRUE;
				break;
			}
			filter->priv->current_images++;
			filter->priv->current_size += fdata->size;
		}
		else if (filter->priv->match_all_tests) {
			filter_matched = FALSE;
			break;
		}
	}

	return filter_matched;
}
