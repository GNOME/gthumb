/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2011 Free Software Foundation, Inc.
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

#ifndef GTH_LIST_VIEW_H
#define GTH_LIST_VIEW_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include "gth-file-view.h"
#include "gth-file-selection.h"

G_BEGIN_DECLS

#define GTH_TYPE_LIST_VIEW            (gth_list_view_get_type ())
#define GTH_LIST_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_LIST_VIEW, GthListView))
#define GTH_LIST_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_LIST_VIEW, GthListViewClass))
#define GTH_IS_LIST_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_LIST_VIEW))
#define GTH_IS_LIST_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_LIST_VIEW))
#define GTH_LIST_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_LIST_VIEW, GthListViewClass))

typedef struct _GthListView GthListView;
typedef struct _GthListViewClass GthListViewClass;
typedef struct _GthListViewPrivate GthListViewPrivate;

struct _GthListView {
	GtkTreeView parent_instance;
	GthListViewPrivate *priv;
};

struct _GthListViewClass {
	GtkTreeViewClass parent_class;
};

GType        gth_list_view_get_type        (void);
GtkWidget *  gth_list_view_new             (void);
GtkWidget *  gth_list_view_new_with_model  (GtkTreeModel *model);

G_END_DECLS

#endif /* GTH_LIST_VIEW_H */
