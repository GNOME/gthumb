/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2003 Free Software Foundation, Inc.
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

#ifndef GTH_EXIF_DATA_VIEWER_H
#define GTH_EXIF_DATA_VIEWER_H


#include <gtk/gtkhbox.h>
#include "image-viewer.h"
#include "file-data.h"

#define GTH_TYPE_EXIF_DATA_VIEWER         (gth_exif_data_viewer_get_type ())
#define GTH_EXIF_DATA_VIEWER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GTH_TYPE_EXIF_DATA_VIEWER, GthExifDataViewer))
#define GTH_EXIF_DATA_VIEWER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GTH_TYPE_EXIF_DATA_VIEWER, GthExifDataViewerClass))
#define GTH_IS_EXIF_DATA_VIEWER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTH_TYPE_EXIF_DATA_VIEWER))
#define GTH_IS_EXIF_DATA_VIEWER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GTH_TYPE_EXIF_DATA_VIEWER))
#define GTH_EXIF_DATA_VIEWER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), GTH_TYPE_EXIF_DATA_VIEWER, GthExifDataViewerClass))


typedef struct _GthExifDataViewer         GthExifDataViewer;
typedef struct _GthExifDataViewerPrivate  GthExifDataViewerPrivate;
typedef struct _GthExifDataViewerClass    GthExifDataViewerClass;


struct _GthExifDataViewer 
{
	GtkHBox __parent;
	GthExifDataViewerPrivate *priv;
};


struct _GthExifDataViewerClass
{
	GtkHBoxClass __parent_class;
};


GType         gth_exif_data_viewer_get_type       (void) G_GNUC_CONST;

GtkWidget *   gth_exif_data_viewer_new            (gboolean view_file_info);

void          gth_exif_data_viewer_view_file_info (GthExifDataViewer *edv,
						   gboolean           view_file_info);

void          gth_exif_data_viewer_update         (GthExifDataViewer *edv,
						   ImageViewer       *viewer,
						   const char        *filename,
						   FileData	     *fd);

GtkWidget *   gth_exif_data_viewer_get_view       (GthExifDataViewer *edv);

#endif /* GTH_EXIF_DATA_VIEWER_H */
