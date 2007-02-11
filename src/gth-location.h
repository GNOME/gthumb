/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2005 Free Software Foundation, Inc.
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

#ifndef GTH_LOCATION_H
#define GTH_LOCATION_H

#include <gtk/gtkhbox.h>

#define GTH_TYPE_LOCATION         (gth_location_get_type ())
#define GTH_LOCATION(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GTH_TYPE_LOCATION, GthLocation))
#define GTH_LOCATION_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GTH_TYPE_LOCATION, GthLocationClass))
#define GTH_IS_LOCATION(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTH_TYPE_LOCATION))
#define GTH_IS_LOCATION_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GTH_TYPE_LOCATION))
#define GTH_LOCATION_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), GTH_TYPE_LOCATION, GthLocationClass))

typedef struct _GthLocation         GthLocation;
typedef struct _GthLocationPrivate  GthLocationPrivate;
typedef struct _GthLocationClass    GthLocationClass;

struct _GthLocation
{
	GtkHBox __parent;
	GthLocationPrivate *priv;
};

struct _GthLocationClass
{
	GtkHBoxClass __parent_class;

	/* -- Signals -- */

        void (* changed) (GthLocation *loc,
			  const char  *uri);
};

GType                gth_location_get_type         (void) G_GNUC_CONST;
GtkWidget *          gth_location_new              (gboolean     folders_only);
void                 gth_location_set_folder_uri   (GthLocation *loc,
						    const char  *uri,
						    gboolean     reset_history);
void                 gth_location_set_catalog_uri  (GthLocation *loc,
						    const char  *uri,
						    gboolean     reset_history);
G_CONST_RETURN char* gth_location_get_uri          (GthLocation *loc);
void                 gth_location_set_bookmarks    (GthLocation *loc,
						    GList       *bookmark_list,
						    int          max_size);
void                 gth_location_update_bookmarks (GthLocation *loc);
void                 gth_location_open_other       (GthLocation *location);

#endif /* GTH_LOCATION_H */
