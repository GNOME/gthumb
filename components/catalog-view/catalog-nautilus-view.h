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

#ifndef _VIEWER_NAUTILUS_VIEW_H_
#define _VIEWER_NAUTILUS_VIEW_H_

#include <bonobo/bonobo-object.h>
#include "catalog-control.h"
#include "nautilus-view-component.h"

G_BEGIN_DECLS

#define CATALOG_TYPE_NAUTILUS_VIEW         (catalog_nautilus_view_get_type ())
#define CATALOG_NAUTILUS_VIEW(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), CATALOG_TYPE_NAUTILUS_VIEW, CatalogNautilusView))
#define CATALOG_NAUTILUS_VIEW_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), CATALOG_TYPE_NAUTILUS_VIEW, CatalogNautilusViewClass))
#define CATALOG_IS_NAUTILUS_VIEW(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), CATALOG_TYPE_NAUTILUS_VIEW))
#define CATALOG_IS_NAUTILUS_VIEW_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), CATALOG_TYPE_NAUTILUS_VIEW))
#define CATALOG_NAUTILUS_VIEW_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), CATALOG_TYPE_NAUTILUS_VIEW, CatalogNautilusViewClass))

typedef struct {
	BonoboObject __parent;
	
	/*< private >*/

	CatalogControl        *control;
	Nautilus_ViewFrame     view_frame;
	char                  *location;
} CatalogNautilusView;

typedef struct {
	BonoboObjectClass __parent_class;
	POA_Nautilus_View__epv epv;

	void (*load_location) (CatalogNautilusView *nautilus_view,
			       const char         *uri,
			       CORBA_Environment  *ev);

	void (*stop_loading)  (CatalogNautilusView *nautilus_view,
			       CORBA_Environment  *ev);
} CatalogNautilusViewClass;

GType          catalog_nautilus_view_get_type (void) G_GNUC_CONST;
BonoboObject * catalog_nautilus_view_new      (CatalogControl *control);

G_END_DECLS

#endif /* _CATALOG_NAUTILUS_VIEW_H_ */
