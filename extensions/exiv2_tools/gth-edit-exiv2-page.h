/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2009 Free Software Foundation, Inc.
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

#ifndef GTH_EDIT_EXIV2_PAGE_H
#define GTH_EDIT_EXIV2_PAGE_H

#include <glib.h>
#include <glib-object.h>
#include <gthumb.h>

#define GTH_TYPE_EDIT_EXIV2_PAGE         (gth_edit_exiv2_page_get_type ())
#define GTH_EDIT_EXIV2_PAGE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GTH_TYPE_EDIT_EXIV2_PAGE, GthEditExiv2Page))
#define GTH_EDIT_EXIV2_PAGE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GTH_TYPE_EDIT_EXIV2_PAGE, GthEditExiv2PageClass))
#define GTH_IS_EDIT_EXIV2_PAGE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTH_TYPE_EDIT_EXIV2_PAGE))
#define GTH_IS_EDIT_EXIV2_PAGE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GTH_TYPE_EDIT_EXIV2_PAGE))
#define GTH_EDIT_EXIV2_PAGE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), GTH_TYPE_EDIT_EXIV2_PAGE, GthEditExiv2PageClass))

typedef struct _GthEditExiv2Page         GthEditExiv2Page;
typedef struct _GthEditExiv2PagePrivate  GthEditExiv2PagePrivate;
typedef struct _GthEditExiv2PageClass    GthEditExiv2PageClass;

struct _GthEditExiv2Page
{
	GtkVBox __parent;
	GthEditExiv2PagePrivate *priv;
};

struct _GthEditExiv2PageClass
{
	GtkVBoxClass __parent_class;	
};

GType gth_edit_exiv2_page_get_type (void) G_GNUC_CONST;

#endif /* GTH_EDIT_EXIV2_PAGE_H */
