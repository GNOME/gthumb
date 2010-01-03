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
 
#ifndef GTH_ICON_VIEW_H
#define GTH_ICON_VIEW_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include "gth-file-view.h"
#include "gth-file-selection.h"

G_BEGIN_DECLS

#define GTH_TYPE_ICON_VIEW            (gth_icon_view_get_type ())
#define GTH_ICON_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_ICON_VIEW, GthIconView))
#define GTH_ICON_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_ICON_VIEW, GthIconViewClass))
#define GTH_IS_ICON_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_ICON_VIEW))
#define GTH_IS_ICON_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_ICON_VIEW))
#define GTH_ICON_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_ICON_VIEW, GthIconViewClass))

typedef struct _GthIconView GthIconView;
typedef struct _GthIconViewClass GthIconViewClass;
typedef struct _GthIconViewPrivate GthIconViewPrivate;

struct _GthIconView {
	GtkIconView parent_instance;
	GthIconViewPrivate *priv;
};

struct _GthIconViewClass {
	GtkIconViewClass parent_class;
};

GType        gth_icon_view_get_type        (void);
GtkWidget *  gth_icon_view_new             (void);
GtkWidget *  gth_icon_view_new_with_model  (GtkTreeModel *model);

G_END_DECLS

#endif /* GTH_ICON_VIEW_H */
