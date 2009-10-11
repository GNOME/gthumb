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

#ifndef GTH_TIFF_SAVER_H
#define GTH_TIFF_SAVER_H

#include <gtk/gtk.h>
#include <gthumb.h>

G_BEGIN_DECLS

#define GTH_TYPE_TIFF_SAVER              (gth_tiff_saver_get_type ())
#define GTH_TIFF_SAVER(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_TIFF_SAVER, GthTiffSaver))
#define GTH_TIFF_SAVER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TIFF_SAVER_TYPE, GthTiffSaverClass))
#define GTH_IS_TIFF_SAVER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_TIFF_SAVER))
#define GTH_IS_TIFF_SAVER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_TIFF_SAVER))
#define GTH_TIFF_SAVER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_TIFF_SAVER, GthTiffSaverClass))

typedef struct _GthTiffSaver         GthTiffSaver;
typedef struct _GthTiffSaverClass    GthTiffSaverClass;
typedef struct _GthTiffSaverPrivate  GthTiffSaverPrivate;

struct _GthTiffSaver
{
	GthPixbufSaver __parent;
	GthTiffSaverPrivate *priv;
};

struct _GthTiffSaverClass
{
	GthPixbufSaverClass __parent_class;
};

GType  gth_tiff_saver_get_type  (void);

G_END_DECLS

#endif /* GTH_TIFF_SAVER_H */
