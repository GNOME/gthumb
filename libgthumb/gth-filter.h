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

#ifndef GTH_FILTER_H
#define GTH_FILTER_H

#include <glib-object.h>
#include "file-data.h"

#define GTH_TYPE_FILTER         (gth_filter_get_type ())
#define GTH_FILTER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GTH_TYPE_FILTER, GthFilter))
#define GTH_FILTER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GTH_TYPE_FILTER, GthFilterClass))
#define GTH_IS_FILTER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTH_TYPE_FILTER))
#define GTH_IS_FILTER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GTH_TYPE_FILTER))
#define GTH_FILTER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), GTH_TYPE_FILTER, GthFilterClass))

typedef enum { /*< skip >*/
	GTH_TEST_SCOPE_FILENAME,
	GTH_TEST_SCOPE_COMMENT,
	GTH_TEST_SCOPE_PLACE,
	GTH_TEST_SCOPE_DATE,
	GTH_TEST_SCOPE_SIZE,
	GTH_TEST_SCOPE_WIDTH,
	GTH_TEST_SCOPE_HEIGHT,
	GTH_TEST_SCOPE_KEYWORDS,
	GTH_TEST_SCOPE_ALL
} GthTestScope;

typedef enum { /*< skip >*/
	GTH_TEST_OP_EQUAL,
	GTH_TEST_OP_LOWER,
	GTH_TEST_OP_GREATER,
	GTH_TEST_OP_CONTAINS,
	GTH_TEST_OP_CONTAINS_ALL,
	GTH_TEST_OP_STARTS_WITH,
	GTH_TEST_OP_ENDS_WITH,
	GTH_TEST_OP_MATCHES,
	GTH_TEST_OP_BEFORE,
	GTH_TEST_OP_AFTER
} GthTestOp;

typedef struct _GthTest           GthTest;
typedef struct _GthFilter         GthFilter;
typedef struct _GthFilterPrivate  GthFilterPrivate;
typedef struct _GthFilterClass    GthFilterClass;

struct _GthFilter
{
	GObject __parent;
	GthFilterPrivate *priv;
};

struct _GthFilterClass
{
	GObjectClass __parent_class;
};

/* GthTest */

GthTest *            gth_test_new               (GthTestScope      scope,
				   		 GthTestOp         op,
						 gboolean          negavite);
GthTest *            gth_test_new_with_int 	(GthTestScope      scope,
	               				 GthTestOp         op,
	               				 gboolean          negavite,
	               				 int               data);
GthTest *            gth_test_new_with_string   (GthTestScope      scope,
	                  			 GthTestOp         op,
	                  			 gboolean          negavite,
	                  			 const char       *data);
GthTest *            gth_test_new_with_date     (GthTestScope      scope,
	                			 GthTestOp         op,
	                			 gboolean          negavite,
	                			 GDate            *date);
void                 gth_test_ref               (GthTest          *test);
void                 gth_test_unref             (GthTest          *test);
gboolean             gth_test_match             (GthTest          *test,
						 FileData         *fdata);

/* GthFilter */

GType                gth_filter_get_type        (void) G_GNUC_CONST;
GthFilter *          gth_filter_new             (void);
void                 gth_filter_set_max_images  (GthFilter        *filter,
						 int               max_images);
int                  gth_filter_get_max_images  (GthFilter        *filter);
void                 gth_filter_set_max_size    (GthFilter        *filter,
						 goffset           max_size);
goffset              gth_filter_get_max_size    (GthFilter        *filter);
void                 gth_filter_set_match_all   (GthFilter        *filter,
						 gboolean          match_all);
gboolean             gth_filter_get_match_all   (GthFilter        *filter);
void                 gth_filter_add_test        (GthFilter        *filter,
						 GthTest          *test);
void                 gth_filter_reset           (GthFilter        *filter);
gboolean             gth_filter_match           (GthFilter        *filter,
						 FileData         *fdata);

#endif /* GTH_FILTER_H */
