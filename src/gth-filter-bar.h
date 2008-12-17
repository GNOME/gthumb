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

#ifndef GTH_FILTER_BAR_H
#define GTH_FILTER_BAR_H

#include <gtk/gtk.h>
#include "gth-filter.h"

#define GTH_TYPE_FILTER_BAR         (gth_filter_bar_get_type ())
#define GTH_FILTER_BAR(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GTH_TYPE_FILTER_BAR, GthFilterBar))
#define GTH_FILTER_BAR_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GTH_TYPE_FILTER_BAR, GthFilterBarClass))
#define GTH_IS_FILTER_BAR(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTH_TYPE_FILTER_BAR))
#define GTH_IS_FILTER_BAR_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GTH_TYPE_FILTER_BAR))
#define GTH_FILTER_BAR_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), GTH_TYPE_FILTER_BAR, GthFilterBarClass))

typedef struct _GthFilterBar         GthFilterBar;
typedef struct _GthFilterBarPrivate  GthFilterBarPrivate;
typedef struct _GthFilterBarClass    GthFilterBarClass;

struct _GthFilterBar
{
	GtkHBox __parent;
	GthFilterBarPrivate *priv;
};

struct _GthFilterBarClass
{
	GtkHBoxClass __parent_class;

	/* -- Signals -- */

        void (* changed)              (GthFilterBar *filter_bar);
        void (* close_button_clicked) (GthFilterBar *filter_bar);
};

GType                gth_filter_bar_get_type        (void) G_GNUC_CONST;
GtkWidget *          gth_filter_bar_new             (void);
GthFilter *          gth_filter_bar_get_filter      (GthFilterBar *filter_bar);
gboolean             gth_filter_bar_has_focus       (GthFilterBar *filter_bar);

#endif /* GTH_FILTER_BAR_H */
