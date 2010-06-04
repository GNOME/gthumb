/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2003-2010 Free Software Foundation, Inc.
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

#ifndef GTH_WEB_EXPORTER_H
#define GTH_WEB_EXPORTER_H

#include <glib.h>
#include <gio/gio.h>
#include <gtk/gtk.h>
#include <gthumb.h>

#define GTH_WEB_EXPORTER_TYPE            (gth_web_exporter_get_type ())
#define GTH_WEB_EXPORTER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_WEB_EXPORTER_TYPE, GthWebExporter))
#define GTH_WEB_EXPORTER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_WEB_EXPORTER_TYPE, GthWebExporterClass))
#define GTH_IS_WEB_EXPORTER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_WEB_EXPORTER_TYPE))
#define GTH_IS_WEB_EXPORTER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_WEB_EXPORTER_TYPE))
#define GTH_WEB_EXPORTER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_WEB_EXPORTER_TYPE, GthWebExporterClass))

typedef struct _GthWebExporter        GthWebExporter;
typedef struct _GthWebExporterClass   GthWebExporterClass;
typedef struct _GthWebExporterPrivate GthWebExporterPrivate;

struct _GthWebExporter {
	GthTask __parent;
	GthWebExporterPrivate *priv;

};

struct _GthWebExporterClass {
	GthTaskClass __parent;
};

GType      gth_web_exporter_get_type              (void);
GthTask *  gth_web_exporter_new                   (GthBrowser       *browser,
						   GList            *file_list);
void       gth_web_exporter_set_header            (GthWebExporter   *self,
						   const char       *header);
void       gth_web_exporter_set_footer            (GthWebExporter   *self,
						   const char       *footer);
void       gth_web_exporter_set_style             (GthWebExporter   *self,
						   const char       *style);
void       gth_web_exporter_set_location          (GthWebExporter   *self,
						   const char       *location);
void       gth_web_exporter_set_use_subfolders    (GthWebExporter   *self,
						   gboolean          use_subfolders);
void       gth_web_exporter_set_row_col           (GthWebExporter   *self,
						   int               rows,
						   int               cols);
void       gth_web_exporter_set_thumb_size        (GthWebExporter   *self,
						   int               width,
						   int               height);
void       gth_web_exporter_set_preview_size      (GthWebExporter   *self,
						   int               width,
						   int               height);
void       gth_web_exporter_set_copy_images       (GthWebExporter   *self,
						   gboolean          copy);
void       gth_web_exporter_set_resize_images     (GthWebExporter   *self,
						   gboolean          resize,
						   int               max_width,
						   int               max_height);
void       gth_web_exporter_set_sorted            (GthWebExporter   *self,
						   GthSortMethod     method,
						   GtkSortType       sort_type);
void       gth_web_exporter_set_row_col           (GthWebExporter   *self,
						   int               rows,
						   int               cols);
void       gth_web_exporter_set_single_index      (GthWebExporter   *self,
						   gboolean          copy);
void       gth_web_exporter_set_image_caption     (GthWebExporter   *self,
						   GthCaptionFields  caption);
guint16    gth_web_exporter_get_image_caption     (GthWebExporter   *self);
void       gth_web_exporter_set_index_caption     (GthWebExporter   *self,
						   GthCaptionFields  caption);
guint16    gth_web_exporter_get_index_caption     (GthWebExporter   *self);
void       gth_web_exporter_export                (GthWebExporter   *self);
void       gth_web_exporter_interrupt             (GthWebExporter   *self);

#endif /* GTH_WEB_EXPORTER_H */
