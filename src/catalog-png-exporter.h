/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001, 2003 Free Software Foundation, Inc.
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

#ifndef CATALOG_PNG_EXPORTER_H
#define CATALOG_PNG_EXPORTER_H

#include <glib.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h> /* for GtkSortType */
#include <gio/gio.h>
#include "image-loader.h"
#include "typedefs.h"

#define CATALOG_PNG_EXPORTER_TYPE            (catalog_png_exporter_get_type ())
#define CATALOG_PNG_EXPORTER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CATALOG_PNG_EXPORTER_TYPE, CatalogPngExporter))
#define CATALOG_PNG_EXPORTER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CATALOG_PNG_EXPORTER_TYPE, CatalogPngExporterClass))
#define IS_CATALOG_PNG_EXPORTER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CATALOG_PNG_EXPORTER_TYPE))
#define IS_CATALOG_PNG_EXPORTER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CATALOG_PNG_EXPORTER_TYPE))
#define CATALOG_PNG_EXPORTER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), CATALOG_PNG_EXPORTER_TYPE, CatalogPngExporterClass))

typedef struct _CatalogPngExporter       CatalogPngExporter;
typedef struct _CatalogPngExporterClass  CatalogPngExporterClass;

struct _CatalogPngExporter {
	GObject __parent;

	/*< private >*/

	GList        *file_list;              /* gchar* elements. */
	GList        *created_files;
	GList         *current_file;
	
	int           thumb_width;
	int           thumb_height;
	int           frame_width;
	int           frame_height;
	int           page_width;             
	int           page_height;
	int           page_rows;              /* Number of rows and columns
	 				       * each page must have. */
	int           page_cols;
	gboolean      page_size_use_row_col;  /* Set the page size from the
					       * number of rows and columns. */
	gboolean      all_pages_same_size;    /* All pages with the same 
					       * size. */
	int          *pages_height;           /* Array containing the height 
					       * of the pages if the user 
					       * specify the size of the page 
					       * by the number of rows and 
					       * columns. */
	int           pages_n;                /* Number of pages. */
	char         *location;               /* Save files in this 
					       * directory. */
	char         *name_template;          /* Name template : #'s will be
					       * replaced with the enumerator
					       * value. */
	char        **templatev;
	int           start_at;
	char         *file_type;              /* Supported types : png, jpeg */

	char         *info;

	guint8        caption_fields;
	char         *caption_font_name;
	GdkColor      caption_color;

	char         *header;
	char         *header_font_name;
	GdkColor      header_color;

	char         *footer;
	char         *footer_font_name;
	GdkColor      footer_color;

	gboolean      page_use_solid_color;
	gboolean      page_use_hgradient;
	gboolean      page_use_vgradient;
	guint32       page_bg_color;
	guint32       page_hgrad1;
	guint32       page_hgrad2;
	guint32       page_vgrad1;
	guint32       page_vgrad2;

	GdkColor      frame_color;
	GthFrameStyle frame_style;
	GthSortMethod sort_method;
	GtkSortType   sort_type;
	
	gboolean      write_image_map;

	/*< very private >*/

	PangoContext *context;
	PangoLayout  *layout;

	GdkColormap  *colormap;

	ImageLoader  *iloader;
	GList        *file_to_load;          /* Next file to be loaded. */

	gint          n_files;              /* Used for the progress signal.*/
	gint          n_files_done;

	GdkPixmap    *pixmap;
	GdkGC        *gc;
	GdkColor      white;
	GdkColor      black;
	GdkColor      gray;
	GdkColor      dark_gray;

	char           *imap_uri;
	GFile          *imap_gfile;          /* GFile parent of OutputStream below */	
	GFileOutputStream *ostream;          /* Output stream to handle partial file writing */

	gboolean exporting;
	gboolean interrupted;
};


struct _CatalogPngExporterClass {
	GObjectClass __parent;

	/* -- signals -- */
	
	void (*png_exporter_done)             (CatalogPngExporter *ce);

	void (*png_exporter_progress)         (CatalogPngExporter *ce,
					       float               percent);

	void (*png_exporter_info)             (CatalogPngExporter *ce,
					       const char         *info);
};


GType      catalog_png_exporter_get_type              (void);
CatalogPngExporter *  catalog_png_exporter_new        (GList *file_list);
void       catalog_png_exporter_set_location          (CatalogPngExporter *ce,
						       const char *location);
void       catalog_png_exporter_set_name_template     (CatalogPngExporter *ce,
						       const char *template);
void       catalog_png_exporter_set_start_at          (CatalogPngExporter *ce,
						       int                 n);
void       catalog_png_exporter_set_file_type         (CatalogPngExporter *ce,
						       const char *file_type);
void       catalog_png_exporter_set_page_size         (CatalogPngExporter *ce,
						       int width,
						       int height);
void       catalog_png_exporter_set_page_size_row_col (CatalogPngExporter *ce,
						       int rows,
						       int cols);
void       catalog_png_exporter_all_pages_same_size   (CatalogPngExporter *ce,
						       gboolean same_size);
void       catalog_png_exporter_set_thumb_size        (CatalogPngExporter *ce,
						       int width,
						       int height);
void       catalog_png_exporter_set_caption           (CatalogPngExporter *ce,
						       GthCaptionFields caption);
void       catalog_png_exporter_set_caption_font      (CatalogPngExporter *ce,
						       char *font);
void       catalog_png_exporter_set_caption_color     (CatalogPngExporter *ce,
						       char *value);
void       catalog_png_exporter_set_header            (CatalogPngExporter *ce,
						       char *header);
void       catalog_png_exporter_set_header_font       (CatalogPngExporter *ce,
						       char *font);
void       catalog_png_exporter_set_header_color      (CatalogPngExporter *ce,
						       char *value);
void       catalog_png_exporter_set_footer            (CatalogPngExporter *ce,
						       char *header);
void       catalog_png_exporter_set_footer_font       (CatalogPngExporter *ce,
						       char *font);
void       catalog_png_exporter_set_footer_color      (CatalogPngExporter *ce,
						       char *value);
void       catalog_png_exporter_set_page_color        (CatalogPngExporter *ce,
						       gboolean  use_solid_col,
						       gboolean  use_hgrad,
						       gboolean  use_vgrad,
						       guint32   bg_color,
						       guint32   hgrad1,
						       guint32   hgrad2,
						       guint32   vgrad1,
						       guint32   vgrad2);
void       catalog_png_exporter_set_frame_color       (CatalogPngExporter *ce,
						       char *value);
void       catalog_png_exporter_set_frame_style       (CatalogPngExporter *ce,
						       GthFrameStyle style);
void       catalog_png_exporter_write_image_map       (CatalogPngExporter *ce,
						       gboolean do_write);
void       catalog_png_exporter_set_sort_method       (CatalogPngExporter *ce,
						       GthSortMethod method);
void       catalog_png_exporter_set_sort_type         (CatalogPngExporter *ce,
						       GtkSortType sort_type);
void       catalog_png_exporter_export                (CatalogPngExporter *ce);
void       catalog_png_exporter_interrupt             (CatalogPngExporter *ce);


#endif /* CATALOG_PNG_EXPORTER_H */
