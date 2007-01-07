/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2007 The Free Software Foundation, Inc.
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

#ifndef GTH_CATEGORY_SELECTION_DIALOG_H
#define GTH_CATEGORY_SELECTION_DIALOG_H

#include <gtk/gtkdialog.h>

#define GTH_TYPE_CATEGORY_SELECTION         (gth_category_selection_get_type ())
#define GTH_CATEGORY_SELECTION(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GTH_TYPE_CATEGORY_SELECTION, GthCategorySelection))
#define GTH_CATEGORY_SELECTION_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GTH_TYPE_CATEGORY_SELECTION, GthCategorySelectionClass))
#define GTH_IS_CATEGORY_SELECTION(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTH_TYPE_CATEGORY_SELECTION))
#define GTH_IS_CATEGORY_SELECTION_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GTH_TYPE_CATEGORY_SELECTION))
#define GTH_CATEGORY_SELECTION_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), GTH_TYPE_CATEGORY_SELECTION, GthCategorySelectionClass))

typedef struct _GthCategorySelection         GthCategorySelection;
typedef struct _GthCategorySelectionPrivate  GthCategorySelectionPrivate;
typedef struct _GthCategorySelectionClass    GthCategorySelectionClass;

struct _GthCategorySelection
{
	GObject __parent;
	GthCategorySelectionPrivate *priv;
};

struct _GthCategorySelectionClass
{
	GObjectClass __parent_class;

	/* -- Signals -- */

        void (* response) (GthCategorySelection *csel,
        		   GtkResponseType       response);
};

GType                  gth_category_selection_get_type       (void) G_GNUC_CONST;
GthCategorySelection * gth_category_selection_new            (GtkWindow            *parent,
							      const char           *categories,
							      gboolean              match_all);
char *                 gth_category_selection_get_categories (GthCategorySelection *csel);
gboolean               gth_category_selection_get_match_all  (GthCategorySelection *csel);

#endif /* GTH_CATEGORY_SELECTION_DIALOG_H */
