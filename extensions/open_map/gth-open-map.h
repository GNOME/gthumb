/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2025 Free Software Foundation, Inc.
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

#ifndef GTH_OPEN_MAP_H
#define GTH_OPEN_MAP_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GTH_TYPE_OPEN_MAP            (gth_open_map_get_type ())
#define GTH_OPEN_MAP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_OPEN_MAP, GthOpenMap))
#define GTH_OPEN_MAP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_OPEN_MAP, GthOpenMapClass))
#define GTH_IS_OPEN_MAP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_OPEN_MAP))
#define GTH_IS_OPEN_MAP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_OPEN_MAP))
#define GTH_OPEN_MAP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_OPEN_MAP, GthOpenMapClass))

typedef struct _GthOpenMap GthOpenMap;
typedef struct _GthOpenMapClass GthOpenMapClass;
typedef struct _GthOpenMapPrivate GthOpenMapPrivate;

struct _GthOpenMap {
	GtkBox parent_instance;
	GthOpenMapPrivate *priv;
};

struct _GthOpenMapClass {
	GtkBoxClass parent_class;
};

GType gth_open_map_get_type (void);

G_END_DECLS

#endif /* GTH_OPEN_MAP_H */
