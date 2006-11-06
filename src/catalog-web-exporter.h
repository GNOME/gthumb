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

#ifndef CATALOG_WEB_EXPORTER_H
#define CATALOG_WEB_EXPORTER_H

#include <glib.h>
#include <gtk/gtk.h>
#include <gtk/gtkenums.h> /* for GtkSortType */
#include "gth-window.h"
#include "image-loader.h"
#include "typedefs.h"

#define CATALOG_WEB_EXPORTER_TYPE            (catalog_web_exporter_get_type ())
#define CATALOG_WEB_EXPORTER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CATALOG_WEB_EXPORTER_TYPE, CatalogWebExporter))
#define CATALOG_WEB_EXPORTER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CATALOG_WEB_EXPORTER_TYPE, CatalogWebExporterClass))
#define IS_CATALOG_WEB_EXPORTER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CATALOG_WEB_EXPORTER_TYPE))
#define IS_CATALOG_WEB_EXPORTER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CATALOG_WEB_EXPORTER_TYPE))
#define CATALOG_WEB_EXPORTER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), CATALOG_WEB_EXPORTER_TYPE, CatalogWebExporterClass))

typedef struct _CatalogWebExporter       CatalogWebExporter;
typedef struct _CatalogWebExporterClass  CatalogWebExporterClass;
typedef struct _ImageData                ImageData;

struct _CatalogWebExporter {
	GObject __parent;

	/*< private >*/

	GthWindow    *window;

	GList        *file_list;              /* char* elements. */
	GList        *album_files;

	char         *header;
	char         *footer;
	char         *style;

	int           page_rows;              /* Number of rows and columns
	 				       * each page must have. */
	int           page_cols;
	gboolean      single_index;

	char         *tmp_location;
	char         *location;               /* Save files in this 
					       * location. */
	char         *index_file;

	char         *info;

	int           thumb_width;
	int           thumb_height;

	gboolean      copy_images;
	GthSortMethod sort_method;
	GtkSortType   sort_type;

	gboolean      resize_images;
	int           resize_max_width;
	int           resize_max_height;

	int           preview_max_width;
	int           preview_max_height;

	guint16       index_caption_mask;
	guint16       image_caption_mask;

	/**/

	ImageLoader  *iloader;
	GList        *file_to_load;          /* Next file to be loaded. */

	int           n_images;              /* Used for the progress signal.*/
	int           n_images_done;
	int           n_pages;
	int           page;
	int           image;
	GList        *index_parsed;
	GList        *thumbnail_parsed;
	GList        *image_parsed;

	GList        *current_image;
	guint         saving_timeout;
	ImageData    *eval_image;

	gboolean      exporting;
	gboolean      interrupted;
};


struct _CatalogWebExporterClass {
	GObjectClass __parent;

	/* -- signals -- */
	
	void (*web_exporter_done)             (CatalogWebExporter *ce);

	void (*web_exporter_progress)         (CatalogWebExporter *ce,
					       float               percent);

	void (*web_exporter_info)             (CatalogWebExporter *ce,
					       const char         *info);

	void (*web_exporter_start_copying)    (CatalogWebExporter *ce);
};


GType      catalog_web_exporter_get_type              (void);

CatalogWebExporter *  catalog_web_exporter_new        (GthWindow          *window,
						       GList              *file_list);

void       catalog_web_exporter_set_header            (CatalogWebExporter *ce,
						       const char         *header);

void       catalog_web_exporter_set_footer            (CatalogWebExporter *ce,
						       const char         *footer);

void       catalog_web_exporter_set_style             (CatalogWebExporter *ce,
						       const char         *style);

void       catalog_web_exporter_set_location          (CatalogWebExporter *ce,
						       const char         *location);

void       catalog_web_exporter_set_index_file        (CatalogWebExporter *ce,
						       const char         *index_file);

void       catalog_web_exporter_set_row_col           (CatalogWebExporter *ce,
						       int                 rows,
						       int                 cols);

void       catalog_web_exporter_set_thumb_size        (CatalogWebExporter *ce,
						       int                 width,
						       int                 height);

void       catalog_web_exporter_set_preview_size      (CatalogWebExporter *ce,
						       int                 width,
						       int                 height);

void       catalog_web_exporter_set_copy_images       (CatalogWebExporter *ce,
						       gboolean            copy);

void       catalog_web_exporter_set_resize_images     (CatalogWebExporter *ce,
						       gboolean            resize,
						       int                 max_width,
						       int                 max_height);

void       catalog_web_exporter_set_sorted            (CatalogWebExporter *ce,
						       GthSortMethod       method,
						       GtkSortType         sort_type);

void       catalog_web_exporter_set_row_col           (CatalogWebExporter *ce,
						       int                 rows,
						       int                 cols);

void       catalog_web_exporter_set_single_index      (CatalogWebExporter *ce,
						       gboolean            copy);

void       catalog_web_exporter_set_image_caption     (CatalogWebExporter *ce,
						       GthCaptionFields    caption);

guint16    catalog_web_exporter_get_image_caption     (CatalogWebExporter *ce);

void       catalog_web_exporter_set_index_caption     (CatalogWebExporter *ce,
						       GthCaptionFields    caption);

guint16    catalog_web_exporter_get_index_caption     (CatalogWebExporter *ce);

void       catalog_web_exporter_export                (CatalogWebExporter *ce);

void       catalog_web_exporter_interrupt             (CatalogWebExporter *ce);


#endif /* CATALOG_WEB_EXPORTER_H */
