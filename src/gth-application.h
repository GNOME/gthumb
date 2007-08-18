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

#ifndef GTH_APPLICATION_H
#define GTH_APPLICATION_H

#include <gtk/gtk.h>
#include <bonobo/bonobo-object.h>
#include "GNOME_GThumb.h"

#define GTH_TYPE_APPLICATION              (gth_application_get_type ())
#define GTH_APPLICATION(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_APPLICATION, GthApplication))
#define GTH_APPLICATION_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_APPLICATION_TYPE, GthApplicationClass))
#define GTH_IS_APPLICATION(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_APPLICATION))
#define GTH_IS_APPLICATION_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_APPLICATION))
#define GTH_APPLICATION_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_APPLICATION, GthApplicationClass))

typedef struct _GthApplication            GthApplication;
typedef struct _GthApplicationClass       GthApplicationClass;

struct _GthApplication
{
	BonoboObject __parent;
};

struct _GthApplicationClass
{
	BonoboObjectClass __parent_class;
	POA_GNOME_GThumb_Application__epv epv;
};

GType          gth_application_get_type          (void);
BonoboObject * gth_application_new               (GdkScreen *screen);

#endif /* GTH_APPLICATION_H */
