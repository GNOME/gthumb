/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001 The Free Software Foundation, Inc.
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

#ifndef VIEWER_CONTROL
#define VIEWER_CONTROL

#include <libbonoboui.h>
#include "file-list.h"

#define CATALOG_TYPE_CONTROL            (catalog_control_get_type ())
#define CATALOG_CONTROL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CATALOG_TYPE_CONTROL, CatalogControl))
#define CATALOG_CONTROL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CATALOG_TYPE_CONTROL, CatalogControlClass))
#define CATALOG_IS_CONTROL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CATALOG_TYPE_CONTROL))
#define CATALOG_IS_CONTROL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CATALOG_TYPE_CONTROL))
#define CATALOG_CONTROL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), CATALOG_TYPE_CONTROL, CatalogControlClass))

typedef struct _CatalogControl         CatalogControl;
typedef struct _CatalogControlClass    CatalogControlClass;

struct _CatalogControl {
	BonoboControl   control;

	/*< private >*/

	BonoboObject   *nautilus_view;
	GthFileList    *file_list;
};

struct _CatalogControlClass {
	BonoboControlClass parent_class;
};

GType           catalog_control_get_type   (void);
BonoboControl  *catalog_control_new        (GthFileList *file_list);

#endif /* CATALOG_CONTROL */
