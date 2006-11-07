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

#include <config.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <locale.h>
#include <ctype.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <libgnomevfs/gnome-vfs-directory.h>

#include "catalog-web-exporter.h"
#include "comments.h"
#include "file-utils.h"
#include "jpegutils/jpeg-data.h"
#include "gth-exif-utils.h"
#include "dlg-file-utils.h"
#include "gthumb-init.h"
#include "gthumb-marshal.h"
#include "gtk-utils.h"
#include "pixbuf-utils.h"
#include "thumb-loader.h"
#include "image-loader.h"
#include "glib-utils.h"
#include "albumtheme-private.h"
#include "rotation-utils.h"
#include "typedefs.h"

#define DATE_FORMAT _("%d %B %Y, %H:%M")


typedef enum {
	GTH_VISIBILITY_ALWAYS = 0,
	GTH_VISIBILITY_INDEX,
	GTH_VISIBILITY_IMAGE
} GthTagVisibility;

enum {
	WEB_EXPORTER_DONE,
	WEB_EXPORTER_PROGRESS,
	WEB_EXPORTER_INFO,
	WEB_EXPORTER_START_COPYING,
	LAST_SIGNAL
};

extern int yyparse (void);

static GObjectClass *parent_class = NULL;
static guint         catalog_web_exporter_signals[LAST_SIGNAL] = { 0 };
extern FILE         *yyin;

#define DEFAULT_THUMB_SIZE 100
#define DEFAULT_INDEX_FILE "index.html"
#define SAVING_TIMEOUT 5


struct _ImageData {
	char             *comment;
	char             *place;
	char             *date_time;
	char             *src_filename;
	char             *dest_filename;
	GnomeVFSFileSize  file_size;
	time_t            file_time;
	GdkPixbuf        *image;
	int               image_width, image_height;
	GdkPixbuf        *thumb;
	int               thumb_width, thumb_height;
	GdkPixbuf        *preview;
	int               preview_width, preview_height;
	gboolean          caption_set;
	gboolean          no_preview;
};

#define IMAGE_DATA(x) ((ImageData*)(x))


static char *
zero_padded (int n)
{
	static char  s[1024];
	char        *t;

	sprintf (s, "%3d", n);
	for (t = s; (t != NULL) && (*t != 0); t++)
		if (*t == ' ')
			*t = '0';

	return s;
}


static int img_counter = 0;


static ImageData *
image_data_new (const char *filename)
{
	ImageData   *idata;
	CommentData *cdata;

	idata = g_new (ImageData, 1);

	cdata = comments_load_comment (filename, TRUE);
	if (cdata != NULL) {
		/*idata->comment = comments_get_comment_as_xml_string (cdata, "&nbsp;<br />", "&nbsp;<br />");*/
		idata->comment = g_strdup (cdata->comment);
		idata->place = g_strdup (cdata->place);
		if (cdata->time != 0) {
			struct tm *tm;
			char time_txt[50];
			tm = localtime (&(cdata->time));
			if (tm->tm_hour + tm->tm_min + tm->tm_sec == 0)
				strftime (time_txt, 50, _("%d %B %Y"), tm);
			else
				strftime (time_txt, 50, _("%d %B %Y, %H:%M"), tm);
			idata->date_time = g_locale_to_utf8 (time_txt, -1, 0, 0, 0);
		}
		else
			idata->date_time = NULL;
		comment_data_free (cdata);
	} else {
		idata->comment = NULL;
		idata->place = NULL;
		idata->date_time = NULL;
	}

	idata->src_filename = g_strdup (filename);
	idata->dest_filename = g_strconcat (
		zero_padded (img_counter++),
		"-",
		file_name_from_path (filename),
		(file_extension_is(filename, "jpg") || file_extension_is(filename, "jpeg") ? "" : ".jpg"), 
		NULL);

	idata->file_size = 0;

	idata->image = NULL;
	idata->image_width = 0;
	idata->image_height = 0;

	idata->thumb = NULL;
	idata->thumb_width = 0;
	idata->thumb_height = 0;

	idata->preview = NULL;
	idata->preview_width = 0;
	idata->preview_height = 0;

	idata->caption_set = FALSE;
	idata->no_preview = FALSE;

	return idata;
}


static void
image_data_free (ImageData *idata)
{
	g_free (idata->comment);
	g_free (idata->place);
	g_free (idata->date_time);
	g_free (idata->src_filename);
	g_free (idata->dest_filename);

	if (idata->image != NULL)
		g_object_unref (idata->image);
	if (idata->thumb != NULL)
		g_object_unref (idata->thumb);
	if (idata->preview != NULL)
		g_object_unref (idata->preview);

	g_free (idata);
}


static void
free_parsed_docs (CatalogWebExporter *ce)
{
	if (ce->index_parsed == NULL) {
		gth_parsed_doc_free (ce->index_parsed);
		ce->index_parsed = NULL;
	}

	if (ce->thumbnail_parsed == NULL) {
		gth_parsed_doc_free (ce->thumbnail_parsed);
		ce->thumbnail_parsed = NULL;
	}

	if (ce->image_parsed == NULL) {
		gth_parsed_doc_free (ce->image_parsed);
		ce->image_parsed = NULL;
	}
}


static void 
catalog_web_exporter_finalize (GObject *object)
{
	CatalogWebExporter *ce;

	g_return_if_fail (IS_CATALOG_WEB_EXPORTER (object));
  
	ce = CATALOG_WEB_EXPORTER (object);

	g_free (ce->header);
	ce->header = NULL;

	g_free (ce->footer);
	ce->footer = NULL;

	g_free (ce->style);
	ce->style = NULL;

	g_free (ce->location);
	ce->location = NULL;

	g_free (ce->tmp_location);
	ce->tmp_location = NULL;

	g_free (ce->index_file);
	ce->index_file = NULL;

	g_free (ce->info);
	ce->info = NULL;

	if (ce->file_list != NULL) {
		g_list_foreach (ce->file_list, (GFunc) image_data_free, NULL);
		g_list_free (ce->file_list);
		ce->file_list = NULL;
	}

	if (ce->album_files != NULL) {
		g_list_foreach (ce->album_files, (GFunc) g_free, NULL);
		g_list_free (ce->album_files);
		ce->album_files = NULL;
	}

	if (ce->iloader != NULL) {
		g_object_unref (ce->iloader);
		ce->iloader = NULL;
	}

	free_parsed_docs (ce);

	/* Chain up */
	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
catalog_web_exporter_class_init (CatalogWebExporterClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	parent_class = g_type_class_peek_parent (class);

	catalog_web_exporter_signals[WEB_EXPORTER_DONE] =
		g_signal_new ("web_exporter_done",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (CatalogWebExporterClass, web_exporter_done),
			      NULL, NULL,
			      gthumb_marshal_VOID__VOID,
			      G_TYPE_NONE, 
			      0);

	catalog_web_exporter_signals[WEB_EXPORTER_PROGRESS] =
		g_signal_new ("web_exporter_progress",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (CatalogWebExporterClass, web_exporter_progress),
			      NULL, NULL,
			      gthumb_marshal_VOID__FLOAT,
			      G_TYPE_NONE, 
			      1, G_TYPE_FLOAT);

	catalog_web_exporter_signals[WEB_EXPORTER_INFO] =
		g_signal_new ("web_exporter_info",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (CatalogWebExporterClass, web_exporter_info),
			      NULL, NULL,
			      gthumb_marshal_VOID__STRING,
			      G_TYPE_NONE, 
			      1, G_TYPE_STRING);

	catalog_web_exporter_signals[WEB_EXPORTER_START_COPYING] =
		g_signal_new ("web_exporter_start_copying",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (CatalogWebExporterClass, web_exporter_start_copying),
			      NULL, NULL,
			      gthumb_marshal_VOID__VOID,
			      G_TYPE_NONE, 
			      0);

	object_class->finalize = catalog_web_exporter_finalize;
}


static void
catalog_web_exporter_init (CatalogWebExporter *ce)
{
	ce->header = NULL;
	ce->footer = NULL;
	ce->style = NULL;
	ce->tmp_location = NULL;
	ce->location = NULL;
	ce->index_file = g_strdup (DEFAULT_INDEX_FILE);
	ce->file_list = NULL;
	ce->album_files = NULL;
	ce->iloader = NULL;

	ce->thumb_width = DEFAULT_THUMB_SIZE;
	ce->thumb_height = DEFAULT_THUMB_SIZE;

	ce->copy_images = FALSE;
	ce->resize_images = FALSE;
	ce->resize_max_width = 0;
	ce->resize_max_height = 0;

	ce->single_index = FALSE;
	ce->preview_max_width = 0;
	ce->preview_max_height = 0;

	ce->index_caption_mask = GTH_CAPTION_IMAGE_DIM | GTH_CAPTION_FILE_SIZE;
	ce->image_caption_mask = GTH_CAPTION_COMMENT | GTH_CAPTION_PLACE | GTH_CAPTION_EXIF_DATE_TIME;
}


GType
catalog_web_exporter_get_type ()
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (CatalogWebExporterClass),
			NULL,
			NULL,
			(GClassInitFunc) catalog_web_exporter_class_init,
			NULL,
			NULL,
			sizeof (CatalogWebExporter),
			0,
			(GInstanceInitFunc) catalog_web_exporter_init
                };

                type = g_type_register_static (G_TYPE_OBJECT,
					       "CatalogWebExporter",
					       &type_info,
					       0);
        }

	return type;
}


CatalogWebExporter *
catalog_web_exporter_new (GthWindow *window,
			  GList     *file_list)
{
	CatalogWebExporter *ce;
	GList              *scan;

	g_return_val_if_fail (window != NULL, NULL);

	ce = CATALOG_WEB_EXPORTER (g_object_new (CATALOG_WEB_EXPORTER_TYPE, NULL));

	ce->window = window;

	img_counter = 0;
	for (scan = file_list; scan; scan = scan->next) {
		const char *filename = (char*) scan->data;
		ImageData  *idata = image_data_new (filename);
		ce->file_list = g_list_prepend (ce->file_list, idata);
	}
	ce->file_list = g_list_reverse (ce->file_list);

	return ce;
}


void
catalog_web_exporter_set_header (CatalogWebExporter *ce,
				 const char         *header)
{
	g_return_if_fail (IS_CATALOG_WEB_EXPORTER (ce));
	g_free (ce->header);
	ce->header = g_strdup (header);
}


void
catalog_web_exporter_set_footer (CatalogWebExporter *ce,
				 const char         *footer)
{
	g_return_if_fail (IS_CATALOG_WEB_EXPORTER (ce));
	g_free (ce->footer);
	ce->footer = g_strdup (footer);
}


void
catalog_web_exporter_set_style (CatalogWebExporter *ce,
				const char         *style)
{
	g_return_if_fail (IS_CATALOG_WEB_EXPORTER (ce));
	g_free (ce->style);
	ce->style = g_strdup (style);
}


void
catalog_web_exporter_set_location (CatalogWebExporter *ce,
				   const char         *location)
{
	g_return_if_fail (IS_CATALOG_WEB_EXPORTER (ce));
	g_free (ce->location);
	ce->location = g_strdup (location);
}


void
catalog_web_exporter_set_index_file (CatalogWebExporter *ce,
				     const char         *index_file)
{
	g_return_if_fail (IS_CATALOG_WEB_EXPORTER (ce));
	g_free (ce->index_file);
	ce->index_file = g_strdup (index_file);
}


void
catalog_web_exporter_set_copy_images (CatalogWebExporter *ce,
				      gboolean            copy)
{
	g_return_if_fail (IS_CATALOG_WEB_EXPORTER (ce));
	ce->copy_images = copy;
}


void
catalog_web_exporter_set_resize_images (CatalogWebExporter *ce,
					gboolean            resize,
					int                 max_width,
					int                 max_height)
{
	g_return_if_fail (IS_CATALOG_WEB_EXPORTER (ce));
	ce->resize_images = resize;
	if (resize) {
		ce->resize_max_width = max_width;
		ce->resize_max_height = max_height;
	} else {
		ce->resize_max_width = 0;
		ce->resize_max_height = 0;
	}
}


void
catalog_web_exporter_set_sorted (CatalogWebExporter *ce,
				 GthSortMethod       method,
				 GtkSortType         sort_type)
{
	g_return_if_fail (IS_CATALOG_WEB_EXPORTER (ce));
	ce->sort_method = method;
	ce->sort_type = sort_type;
}


void
catalog_web_exporter_set_row_col (CatalogWebExporter *ce,
				  int                 rows,
				  int                 cols)
{
	g_return_if_fail (IS_CATALOG_WEB_EXPORTER (ce));
	ce->page_rows = rows;
	ce->page_cols = cols;
}


void
catalog_web_exporter_set_single_index (CatalogWebExporter *ce,
				       gboolean            single)
{
	g_return_if_fail (IS_CATALOG_WEB_EXPORTER (ce));
	ce->single_index = single;
}


void
catalog_web_exporter_set_thumb_size (CatalogWebExporter *ce,
				     int                 width,
				     int                 height)
{
	g_return_if_fail (IS_CATALOG_WEB_EXPORTER (ce));
	ce->thumb_width = width;
	ce->thumb_height = height;
}


void
catalog_web_exporter_set_preview_size (CatalogWebExporter *ce,
				       int                 width,
				       int                 height)
{
	g_return_if_fail (IS_CATALOG_WEB_EXPORTER (ce));

	if (ce->copy_images 
	    && ce->resize_images 
	    && (ce->resize_max_width > 0) 
	    && (ce->resize_max_height > 0)) {
		if (width > ce->resize_max_width) 
			width = ce->resize_max_width;
		if (height > ce->resize_max_height) 
			height = ce->resize_max_height;
	}

	ce->preview_max_width = width;
	ce->preview_max_height = height;
}


void
catalog_web_exporter_set_image_caption (CatalogWebExporter *ce,
					GthCaptionFields    caption)
{
	g_return_if_fail (IS_CATALOG_WEB_EXPORTER (ce));
	ce->image_caption_mask = caption;
}


guint16
catalog_web_exporter_get_image_caption (CatalogWebExporter *ce)
{
	return ce->image_caption_mask;
}


void
catalog_web_exporter_set_index_caption (CatalogWebExporter *ce,
					GthCaptionFields    caption)
{
	g_return_if_fail (IS_CATALOG_WEB_EXPORTER (ce));
	ce->index_caption_mask = caption;
}


guint16
catalog_web_exporter_get_index_caption (CatalogWebExporter *ce)
{
	return ce->index_caption_mask;
}


static int
comp_func_name (gconstpointer a, gconstpointer b)
{
	ImageData *data_a, *data_b;

	data_a = IMAGE_DATA (a);
	data_b = IMAGE_DATA (b);

	return strcasecmp (file_name_from_path (data_a->src_filename), 
			   file_name_from_path (data_b->src_filename));
}


static int
comp_func_path (gconstpointer a, gconstpointer b)
{
	ImageData *data_a, *data_b;

	data_a = IMAGE_DATA (a);
	data_b = IMAGE_DATA (b);

	return strcasecmp (data_a->src_filename, data_b->src_filename);
}


static int
comp_func_comment (gconstpointer a, gconstpointer b)
{
	ImageData *data_a, *data_b;

	data_a = IMAGE_DATA (a);
	data_b = IMAGE_DATA (b);

        if ((data_a->comment == NULL) && (data_b->comment == NULL))
		return 0;
	if (data_a->comment == NULL)
		return 1;
	if (data_b->comment == NULL)
		return -1;

	return g_utf8_collate ( g_utf8_casefold (data_a->comment,-1), g_utf8_casefold (data_b->comment,-1) );
}


static int
comp_func_time (gconstpointer a, gconstpointer b)
{
	ImageData *data_a, *data_b;

	data_a = IMAGE_DATA (a);
	data_b = IMAGE_DATA (b);

	if (data_a->file_time == data_b->file_time)
		return 0;
	else if (data_a->file_time > data_b->file_time)
		return 1;
	else
		return -1;
}


static int
comp_func_size (gconstpointer a, gconstpointer b)
{
	ImageData *data_a, *data_b;

	data_a = IMAGE_DATA (a);
	data_b = IMAGE_DATA (b);

	if (data_a->file_size == data_b->file_size)
		return 0;
	else if (data_a->file_size > data_b->file_size)
		return 1;
	else
		return -1;
}


static int
comp_func_none (gconstpointer a, gconstpointer b)
{
	return -1;
}


static GCompareFunc
get_sortfunc (CatalogWebExporter *ce)
{
	GCompareFunc func;

	switch (ce->sort_method) {
	case GTH_SORT_METHOD_BY_NAME:
		func = comp_func_name;
		break;
	case GTH_SORT_METHOD_BY_TIME:
		func = comp_func_time;
		break;
	case GTH_SORT_METHOD_BY_SIZE:
		func = comp_func_size;
		break;
	case GTH_SORT_METHOD_BY_PATH:
		func = comp_func_path;
		break;
	case GTH_SORT_METHOD_BY_COMMENT:
		func = comp_func_comment;
		break;
	case GTH_SORT_METHOD_NONE:
		func = comp_func_none;
		break;
	default:
		func = comp_func_none;
		break;
	}

	return func;
}


static int
get_var_value (const char *var_name,
	       gpointer    data)
{
	CatalogWebExporter *ce = data;

	if (strcmp (var_name, "image_idx") == 0)
		return ce->image + 1;
	else if (strcmp (var_name, "images") == 0)
		return ce->n_images;
	else if (strcmp (var_name, "page_idx") == 0)
		return ce->page + 1;
	else if (strcmp (var_name, "pages") == 0)
		return ce->n_pages;
	else if (strcmp (var_name, "index") == 0)
		return GTH_VISIBILITY_INDEX;
	else if (strcmp (var_name, "image") == 0)
		return GTH_VISIBILITY_IMAGE;
	else if (strcmp (var_name, "always") == 0)
		return GTH_VISIBILITY_ALWAYS;

	else if (strcmp (var_name, "image_width") == 0)
		return ce->eval_image->image_width;
	else if (strcmp (var_name, "image_height") == 0)
		return ce->eval_image->image_height;
	else if (strcmp (var_name, "preview_width") == 0)
		return ce->eval_image->preview_width;
	else if (strcmp (var_name, "preview_height") == 0)
		return ce->eval_image->preview_height;
	else if (strcmp (var_name, "thumb_width") == 0)
		return ce->eval_image->thumb_width;
	else if (strcmp (var_name, "thumb_height") == 0)
		return ce->eval_image->thumb_height;

	else if (strcmp (var_name, "image_dim_visibility_index") == 0)
		return ce->index_caption_mask & GTH_CAPTION_IMAGE_DIM;
	else if (strcmp (var_name, "file_name_visibility_index") == 0)
		return ce->index_caption_mask & GTH_CAPTION_FILE_NAME;
 	else if (strcmp (var_name, "file_path_visibility_index") == 0)
		return ce->index_caption_mask & GTH_CAPTION_FILE_PATH;
	else if (strcmp (var_name, "file_size_visibility_index") == 0)
		return ce->index_caption_mask & GTH_CAPTION_FILE_SIZE;
	else if (strcmp (var_name, "comment_visibility_index") == 0)
		return ce->index_caption_mask & GTH_CAPTION_COMMENT;
	else if (strcmp (var_name, "place_visibility_index") == 0)
		return ce->index_caption_mask & GTH_CAPTION_PLACE;
	else if (strcmp (var_name, "date_time_visibility_index") == 0)
		return ce->index_caption_mask & GTH_CAPTION_DATE_TIME;
	else if (strcmp (var_name, "exif_date_time_visibility_index") == 0)
		return ce->index_caption_mask & GTH_CAPTION_EXIF_DATE_TIME;
	else if (strcmp (var_name, "exif_exposure_time_visibility_index") == 0)
		return ce->index_caption_mask & GTH_CAPTION_EXIF_EXPOSURE_TIME;
	else if (strcmp (var_name, "exif_exposure_mode_visibility_index") == 0)
		return ce->index_caption_mask & GTH_CAPTION_EXIF_EXPOSURE_MODE;
	else if (strcmp (var_name, "exif_flash_visibility_index") == 0)
		return ce->index_caption_mask & GTH_CAPTION_EXIF_FLASH;
	else if (strcmp (var_name, "exif_shutter_speed_visibility_index") == 0)
		return ce->index_caption_mask & GTH_CAPTION_EXIF_SHUTTER_SPEED;
	else if (strcmp (var_name, "exif_aperture_value_visibility_index") == 0)
		return ce->index_caption_mask & GTH_CAPTION_EXIF_APERTURE_VALUE;
	else if (strcmp (var_name, "exif_focal_length_visibility_index") == 0)
		return ce->index_caption_mask & GTH_CAPTION_EXIF_FOCAL_LENGTH;
	else if (strcmp (var_name, "exif_camera_model_visibility_index") == 0)
		return ce->index_caption_mask & GTH_CAPTION_EXIF_CAMERA_MODEL;

	else if (strcmp (var_name, "image_dim_visibility_image") == 0)
		return ce->image_caption_mask & GTH_CAPTION_IMAGE_DIM;
	else if (strcmp (var_name, "file_name_visibility_image") == 0)
		return ce->image_caption_mask & GTH_CAPTION_FILE_NAME;
 	else if (strcmp (var_name, "file_path_visibility_image") == 0)
		return ce->image_caption_mask & GTH_CAPTION_FILE_PATH;
	else if (strcmp (var_name, "file_size_visibility_image") == 0)
		return ce->image_caption_mask & GTH_CAPTION_FILE_SIZE;
	else if (strcmp (var_name, "comment_visibility_image") == 0)
		return ce->image_caption_mask & GTH_CAPTION_COMMENT;
	else if (strcmp (var_name, "place_visibility_image") == 0)
		return ce->image_caption_mask & GTH_CAPTION_PLACE;
	else if (strcmp (var_name, "date_time_visibility_image") == 0)
		return ce->image_caption_mask & GTH_CAPTION_DATE_TIME;
	else if (strcmp (var_name, "exif_date_time_visibility_image") == 0)
		return ce->image_caption_mask & GTH_CAPTION_EXIF_DATE_TIME;
	else if (strcmp (var_name, "exif_exposure_time_visibility_image") == 0)
		return ce->image_caption_mask & GTH_CAPTION_EXIF_EXPOSURE_TIME;
	else if (strcmp (var_name, "exif_exposure_mode_visibility_image") == 0)
		return ce->image_caption_mask & GTH_CAPTION_EXIF_EXPOSURE_MODE;
	else if (strcmp (var_name, "exif_flash_visibility_image") == 0)
		return ce->image_caption_mask & GTH_CAPTION_EXIF_FLASH;
	else if (strcmp (var_name, "exif_shutter_speed_visibility_image") == 0)
		return ce->image_caption_mask & GTH_CAPTION_EXIF_SHUTTER_SPEED;
	else if (strcmp (var_name, "exif_aperture_value_visibility_image") == 0)
		return ce->image_caption_mask & GTH_CAPTION_EXIF_APERTURE_VALUE;
	else if (strcmp (var_name, "exif_focal_length_visibility_image") == 0)
		return ce->image_caption_mask & GTH_CAPTION_EXIF_FOCAL_LENGTH;
	else if (strcmp (var_name, "exif_camera_model_visibility_image") == 0)
		return ce->image_caption_mask & GTH_CAPTION_EXIF_CAMERA_MODEL;

	else if (strcmp (var_name, "copy_originals") == 0)
		return ce->copy_images;

	g_warning ("[GetVarValue] Unknown variable name: %s", var_name);
	
	return 0;
}


static int
expression_value (CatalogWebExporter *ce,
		  GthExpr            *expr)
{
	gth_expr_set_get_var_value_func (expr, get_var_value, ce);
	return gth_expr_eval (expr);
}


static int
gth_tag_get_idx (GthTag             *tag, 
		 CatalogWebExporter *ce,
		 int                 default_value,
		 int                 max_value)
{
	GList *scan;
	int    retval = default_value;

	for (scan = tag->value.arg_list; scan; scan = scan->next) {
		GthVar *var = scan->data;

		if (strcmp (var->name, "idx_relative") == 0) {
			retval = default_value + expression_value (ce, var->value.expr);
			break;

		} else if (strcmp (var->name, "idx") == 0) {
			retval = expression_value (ce, var->value.expr) - 1;
			break;
		}
	}

	retval = MIN (retval, max_value);
	retval = MAX (retval, 0);

	return retval;
}


static int
get_image_idx (GthTag             *tag,
	       CatalogWebExporter *ce)
{
	return gth_tag_get_idx (tag, ce, ce->image, ce->n_images - 1);	
}


static int
get_page_idx (GthTag             *tag,
	      CatalogWebExporter *ce)
{
	return gth_tag_get_idx (tag, ce, ce->page, ce->n_pages - 1);	
}


static int
gth_tag_get_var (CatalogWebExporter *ce,
		 GthTag             *tag, 
		 const char         *var_name)
{
	GList *scan;

	for (scan = tag->value.arg_list; scan; scan = scan->next) {
		GthVar *var = scan->data;
		if (strcmp (var->name, var_name) == 0) 
			return expression_value (ce, var->value.expr);
	}

	return 0;
}


static const char *
gth_tag_get_str (CatalogWebExporter *ce,
		 GthTag             *tag,
		 const char         *var_name)
{
	GList *scan;

	for (scan = tag->value.arg_list; scan; scan = scan->next) {
		GthVar *var = scan->data;
		if (strcmp (var->name, var_name) == 0) {
			GthCell *cell = gth_expr_get(var->value.expr);
			if (cell->type == GTH_CELL_TYPE_VAR)
				return cell->value.var;
		}
	}

	return NULL;
}


static int
get_page_idx_from_image_idx (CatalogWebExporter *ce,
			     int                 image_idx)
{
	if (ce->single_index)
		return 1;
	else
		return image_idx / (ce->page_rows * ce->page_cols);
}


static gboolean
line_is_void (const char *line)
{
	const char *scan;

	if (line == NULL)
		return TRUE;

	for (scan = line; *scan != '\0'; scan++)
		if ((*scan != ' ')
		    && (*scan != '\t')
		    && (*scan != '\n'))
			return FALSE;

	return TRUE;
}


static void
write_line (const char *line, FILE *fout) 
{
	if (line_is_void (line))
		return;

	fwrite (line, sizeof (char), strlen (line), fout);
}


static void
write_markup_escape_line (const char *line, FILE *fout) 
{
	char *e_line;

	e_line = _g_escape_text_for_html (line, -1);
	write_line (e_line, fout);
	g_free (e_line);
}


static void
write_locale_line (const char *line, FILE *fout) 
{
	char *utf8_line;

	if (line == NULL)
		return;
	if (*line == 0)
		return;

	utf8_line = g_locale_to_utf8 (line, -1, 0, 0, 0);
	write_line (utf8_line, fout);
	g_free (utf8_line);
}


static void
write_markup_escape_locale_line (const char *line, FILE *fout) 
{
	char *e_line;
	
	e_line = _g_escape_text_for_html (line, -1);
	write_locale_line (e_line, fout);
	g_free (e_line);
}


static char *
get_thumbnail_filename (CatalogWebExporter *ce,
			ImageData          *idata,
			const char         *location)
{
	return g_strconcat ((location ? location : ""),
			    (location ? "/" : ""),
			    file_name_from_path (idata->dest_filename), 
			    ".small", 
			    ".jpeg",
			    NULL);
}


static char *
get_image_filename (CatalogWebExporter *ce,
		    ImageData          *idata,
		    const char         *location)
{
	char *filename;

	if (ce->copy_images)
		filename = g_strconcat ((location ? location : ""),
					(location ? "/" : ""),
					file_name_from_path (idata->dest_filename),
					NULL);
	else {
		filename = get_uri_from_path (idata->src_filename);
		if (filename == NULL)
			filename = g_strdup (idata->src_filename);
	}
	
	return filename;
}


static char *
get_preview_filename (CatalogWebExporter *ce,
		      ImageData          *idata,
		      const char         *location)
{
	if (idata->no_preview)
		return get_image_filename (ce, idata, location);
	else
		return g_strconcat ((location ? location : ""),
				    (location ? "/" : ""),
				    file_name_from_path (idata->dest_filename), 
				    ".medium", 
				    ".jpeg",
				    NULL);
}


static char*
get_current_date (void)
{
	time_t     t;
	struct tm *tp;
	char       s[100];

	t = time (NULL);
	tp = localtime (&t);
	strftime (s, 99, DATE_FORMAT, tp);

	return g_locale_to_utf8 (s, -1, 0, 0, 0);
}


static int
is_alpha_string (char   *s, 
		 size_t  maxlen)
{
	if (s == NULL)
		return 0;
	
	while ((maxlen > 0) && (*s != '\0') && isalpha (*s)) {
		maxlen--;
		s++;
	}
	
	return ((maxlen == 0) || (*s == '\0'));
}


static char*
get_current_language (void)
{
	char   *language = NULL;
	char   *tmp_locale;
	char   *locale;
	char   *underline;
	size_t  len;

	tmp_locale = setlocale (LC_ALL, NULL);
	if (tmp_locale == NULL)
		return NULL;
	locale = g_strdup (tmp_locale);

	/* FIXME: complete LC_ALL -> RFC 3066 */
	
	underline = strchr (locale, '_');
	if (underline != NULL)
		*underline = '\0';

	len = strlen (locale);
	if (((len == 2) || (len == 3)) && is_alpha_string (locale, len))
		language = g_locale_to_utf8 (locale, -1, 0, 0, 0);
		
	g_free (locale);

	return language;
}


static char *
get_hf_text (const char *utf8_text)
{
	const char *s;
	GString    *r;
	char       *r_str;

	if (utf8_text == NULL)
		return NULL;

	if (g_utf8_strchr (utf8_text,  -1, '%') == NULL)
		return g_strdup (utf8_text);

	r = g_string_new (NULL);
	for (s = utf8_text; *s != 0; s = g_utf8_next_char (s)) {
		gunichar ch = g_utf8_get_char (s);

		if (ch == '%') {
			s = g_utf8_next_char (s);

			if (*s == 0) {
				g_string_append_unichar (r, ch);
				break;
			}

			ch = g_utf8_get_char (s);			
			switch (ch) {
				char *t;

			case '%':
				g_string_append (r, "%");
				break;

			case 'd':
				t = get_current_date ();
				g_string_append (r, t);
				g_free (t);
				break;
			}
		} else
			g_string_append_unichar (r, ch);
	}

	r_str = r->str;
	g_string_free (r, FALSE);

	return r_str;
}


static void
gth_parsed_doc_print (GList              *document,
		      CatalogWebExporter *ce,
		      FILE               *fout,
		      gboolean            allow_table)
{
	GList *scan;

	for (scan = document; scan; scan = scan->next) {
		GthTag     *tag = scan->data;
		ImageData  *idata;
		char       *line = NULL;
		char       *image_src, *image_src_relative;
		char       *escaped_path, *e_escaped_path;
		int         idx;
		int         image_width;
		int         image_height;
		int         max_size;
		int         r, c;
		int         value;
		const char *class;
		char       *class_attr;
		char       *filename;
		const char *alt;
		char       *alt_attr;
		const char *id;
		char       * id_attr;
		GList      *scan;


		switch (tag->type) {
		case GTH_TAG_HEADER:
			line = get_hf_text (ce->header);
			write_markup_escape_line (line, fout);
			break;

		case GTH_TAG_FOOTER:
			line = get_hf_text (ce->footer);
			write_markup_escape_line (line, fout);
			break;

		case GTH_TAG_LANGUAGE:
			line = get_current_language ();
			write_markup_escape_line (line, fout);
			break;

		case GTH_TAG_IMAGE:
			idx = get_image_idx (tag, ce);
			idata = g_list_nth (ce->file_list, idx)->data;
			ce->eval_image = idata;

			if (gth_tag_get_var (ce, tag, "thumbnail") != 0) {
				image_src = get_thumbnail_filename (ce, idata, ce->location);
				image_width = idata->thumb_width;
				image_height = idata->thumb_height;
			} else if (gth_tag_get_var (ce, tag, "preview") != 0) {
				image_src = get_preview_filename (ce, idata, ce->location);
				image_width = idata->preview_width;
				image_height = idata->preview_height;
			} else {
				image_src = get_image_filename (ce, idata, ce->location);
				image_width = idata->image_width;
				image_height = idata->image_height;
			}

			class = gth_tag_get_str (ce, tag, "class");
			if (class)
				class_attr = g_strdup_printf (" class=\"%s\"", class);
			else
				class_attr = g_strdup ("");

			max_size = gth_tag_get_var (ce, tag, "max_size");
			if (max_size > 0) 
				scale_keepping_ratio (&image_width,
						      &image_height,
						      max_size,
						      max_size);

			image_src_relative = get_path_relative_to_dir (image_src, 
								       ce->location);
			escaped_path = escape_uri (image_src_relative);
			e_escaped_path = _g_escape_text_for_html (escaped_path, -1);

			alt = gth_tag_get_str (ce, tag, "alt");
			if (alt)
				alt_attr = g_strdup (alt);
			else
				alt_attr = g_strdup (e_escaped_path);
			
			id = gth_tag_get_str (ce, tag, "id");
			if (id)
				id_attr = g_strdup_printf (" id=\"%s\"", id);
			else
				id_attr = g_strdup("");
			
			line = g_strdup_printf ("<img src=\"%s\" alt=\"%s\" width=\"%d\" height=\"%d\"%s%s />",
						e_escaped_path,
						alt_attr,
						image_width,
						image_height,
						id_attr,
						class_attr);
			g_free (id_attr);
			g_free (alt_attr);
			g_free (class_attr);
			g_free (image_src);
			g_free (image_src_relative);
			g_free (escaped_path);
			g_free (e_escaped_path);
			write_line (line, fout);
			break;

		case GTH_TAG_IMAGE_LINK:
			idx = get_image_idx (tag, ce);
			idata = g_list_nth (ce->file_list, idx)->data;
			line = g_strconcat (file_name_from_path (idata->dest_filename), 
					    ".html",
					    NULL);
			escaped_path = escape_uri (line);
			g_free (line);
			line = escaped_path;

			write_markup_escape_line (line, fout);
			break;

		case GTH_TAG_IMAGE_IDX:
			line = g_strdup_printf ("%d", get_image_idx (tag, ce) + 1);
			write_line (line, fout);
			break;

		case GTH_TAG_IMAGE_DIM:
			idx = get_image_idx (tag, ce);
			idata = g_list_nth (ce->file_list, idx)->data;
			line = g_strdup_printf ("%dx%d",
						idata->image_width,
						idata->image_height);
			write_line (line, fout);
			break;

		case GTH_TAG_IMAGES:
			line = g_strdup_printf ("%d", ce->n_images);
			write_line (line, fout);
			break;

		case GTH_TAG_FILENAME:
			idx = get_image_idx (tag, ce);
			idata = g_list_nth (ce->file_list, idx)->data;
			ce->eval_image = idata;

			if (gth_tag_get_var (ce, tag, "with_path") != 0) {
				line = get_image_filename (ce, idata, ce->location);

			} else if (gth_tag_get_var (ce, tag, "with_relative_path") != 0) {
				char *path = get_image_filename (ce, idata, ce->location);
				line = get_path_relative_to_dir (path, ce->location);
				g_free (path);

			} else {
				char *path = get_image_filename (ce, idata, ce->location);
				line = g_strdup (file_name_from_path (path));
				g_free (path);
			}

			if  (gth_tag_get_var (ce, tag, "utf8") != 0) 
				write_markup_escape_locale_line (line, fout);
			else {
				escaped_path = escape_uri (line);
				g_free (line);
				line = escaped_path;
				write_markup_escape_line (line, fout);
			}

			break;

		case GTH_TAG_FILEPATH:
			idx = get_image_idx (tag, ce);
			idata = g_list_nth (ce->file_list, idx)->data;
			ce->eval_image = idata;
			filename = get_image_filename (ce, idata, ce->location);
			if (gth_tag_get_var (ce, tag, "relative_path") != 0) {
				char *tmp;
				tmp = get_path_relative_to_dir (filename, ce->location);
				g_free (filename);
				filename = tmp;
			}
			line = remove_level_from_path (filename);
			g_free (filename);

			if  (gth_tag_get_var (ce, tag, "utf8") != 0) 
				write_markup_escape_locale_line (line, fout);
			else {
				escaped_path = escape_uri (line);
				g_free (line);
				line = escaped_path;
				write_markup_escape_line (line, fout);
			}
	
			break;

		case GTH_TAG_FILESIZE:
			idx = get_image_idx (tag, ce);
			idata = g_list_nth (ce->file_list, idx)->data;
			line = gnome_vfs_format_file_size_for_display (idata->file_size);
			write_markup_escape_line (line, fout);
			break;

		case GTH_TAG_COMMENT:
			idx = get_image_idx (tag, ce);
			idata = g_list_nth (ce->file_list, idx)->data;
			ce->eval_image = idata;
			
			if (idata->comment == NULL)
				break;

			max_size = gth_tag_get_var (ce, tag, "max_size");
			if (max_size <= 0)
				line = g_strdup (idata->comment);
			else {
				char *comment = g_strndup (idata->comment, max_size);
				if (strlen (comment) < strlen (idata->comment))
					line = g_strconcat (comment, "...", NULL);
				else
					line = g_strdup (comment);
				g_free (comment);
			}

			write_markup_escape_line (line, fout);
			break;
		
		case GTH_TAG_PLACE:
			idx = get_image_idx (tag, ce);
			idata = g_list_nth (ce->file_list, idx)->data;
			ce->eval_image = idata;
			
			if (idata->place == NULL)
				break;

			max_size = gth_tag_get_var (ce, tag, "max_size");
			if (max_size <= 0)
				line = g_strdup (idata->place);
			else {
				char *place = g_strndup (idata->place, max_size);
				if (strlen (place) < strlen (idata->place))
					line = g_strconcat (place, "...", NULL);
				else
					line = g_strdup (place);
				g_free (place);
			}

			write_markup_escape_line (line, fout);
			break;

		case GTH_TAG_DATE_TIME:
			idx = get_image_idx (tag, ce);
			idata = g_list_nth (ce->file_list, idx)->data;
			ce->eval_image = idata;
			if (idata->date_time == NULL)
				break;

			max_size = gth_tag_get_var (ce, tag, "max_size");
			if (max_size <= 0)
				line = g_strdup (idata->date_time);
			else {
				char *date_time = g_strndup (idata->date_time, max_size);
				if (strlen (date_time) < strlen (idata->date_time))
					line = g_strconcat (date_time, "...", NULL);
				else
					line = g_strdup (date_time);
				g_free (date_time);
			}

			write_markup_escape_line (line, fout);
			break;

		case GTH_TAG_PAGE_LINK:
			if (gth_tag_get_var (ce, tag, "image_idx") != 0) {
				int image_idx;
				image_idx = get_image_idx (tag, ce);
				idx = get_page_idx_from_image_idx (ce, image_idx);
			} else 
				idx = get_page_idx (tag, ce);

			if (idx == 0)
				line = g_strdup (ce->index_file);
			else
				line = g_strconcat ("page",
						    zero_padded (idx + 1),
						    ".html",
						    NULL);
			write_markup_escape_line (line, fout);
			break;

		case GTH_TAG_PAGE_IDX:
			line = g_strdup_printf ("%d", get_page_idx (tag, ce) + 1);
			write_line (line, fout);
			break;

		case GTH_TAG_PAGES:
			line = g_strdup_printf ("%d", ce->n_pages);
			write_line (line, fout);
			break;

		case GTH_TAG_TABLE:
			if (! allow_table)
				break;

			if (ce->single_index)
				ce->page_rows = (ce->n_images + ce->page_cols - 1) / ce->page_cols;
	
			/* this may not work correctly if single_index is set */
			for (r = 0; r < ce->page_rows; r++) {
				if (ce->image < ce->n_images)
					write_line ("  <tr class=\"tr_index\">\n", fout);
				else
					write_line ("  <tr class=\"tr_empty_index\">\n", fout);
				for (c = 0; c < ce->page_cols; c++) {
					if (ce->image < ce->n_images) {
						write_line ("    <td class=\"td_index\">\n", fout);
						gth_parsed_doc_print (ce->thumbnail_parsed, 
								      ce, 
								      fout, 
								      FALSE);
						write_line ("    </td>\n", fout);
						ce->image++;
					} else {
						write_line ("    <td class=\"td_empty_index\">\n", fout);
						write_line ("    &nbsp;\n", fout);
						write_line ("    </td>\n", fout);
					} 
				}
				write_line ("  </tr>\n", fout);
				
			}
			break;

		case GTH_TAG_THUMBS:
			if (! allow_table)
				break;

			for (r=0; r < (ce->single_index ? ce->n_images : ce->page_rows*ce->page_cols); r++)
			{
				if (ce->image >= ce->n_images)
					break;
				gth_parsed_doc_print(ce->thumbnail_parsed, ce, fout, FALSE);
				ce->image++;
			}
			break;

		case GTH_TAG_DATE:
			line = get_current_date ();
			write_markup_escape_line (line, fout);
			break;

		case GTH_TAG_HTML:
			write_line (tag->value.html, fout);
			break;

		case GTH_TAG_EXIF_EXPOSURE_TIME:
			idx = get_image_idx (tag, ce);
			idata = g_list_nth (ce->file_list, idx)->data;
			line = get_exif_tag (idata->src_filename, 
					     EXIF_TAG_EXPOSURE_TIME);
			write_markup_escape_line (line, fout);
			break;

		case GTH_TAG_EXIF_EXPOSURE_MODE:
			idx = get_image_idx (tag, ce);
			idata = g_list_nth (ce->file_list, idx)->data;
			line = get_exif_tag (idata->src_filename, 
					     EXIF_TAG_EXPOSURE_MODE);
			write_markup_escape_line (line, fout);
			break;

		case GTH_TAG_EXIF_FLASH:
			idx = get_image_idx (tag, ce);
			idata = g_list_nth (ce->file_list, idx)->data;
			line = get_exif_tag (idata->src_filename, EXIF_TAG_FLASH);
			write_markup_escape_line (line, fout);
			break;

		case GTH_TAG_EXIF_SHUTTER_SPEED:
			idx = get_image_idx (tag, ce);
			idata = g_list_nth (ce->file_list, idx)->data;
			line = get_exif_tag (idata->src_filename, 
					     EXIF_TAG_SHUTTER_SPEED_VALUE);
			write_markup_escape_line (line, fout);
			break;

		case GTH_TAG_EXIF_APERTURE_VALUE:
			idx = get_image_idx (tag, ce);
			idata = g_list_nth (ce->file_list, idx)->data;
			line = get_exif_aperture_value (idata->src_filename);
			write_markup_escape_line (line, fout);
			break;

		case GTH_TAG_EXIF_FOCAL_LENGTH:
			idx = get_image_idx (tag, ce);
			idata = g_list_nth (ce->file_list, idx)->data;
			line = get_exif_tag (idata->src_filename, 
					     EXIF_TAG_FOCAL_LENGTH);
			write_markup_escape_line (line, fout);
			break;

		case GTH_TAG_EXIF_DATE_TIME:
			idx = get_image_idx (tag, ce);
			idata = g_list_nth (ce->file_list, idx)->data;
			{
				time_t     t;
				struct tm *tp;
				char s[100];

				t = get_exif_time (idata->src_filename);
				if (t != 0) {
					tp = localtime (&t);
					strftime (s, 99, DATE_FORMAT, tp);
					line = g_locale_to_utf8 (s, -1, 0, 0, 0);
					write_markup_escape_line (line, fout);
				} else
					write_line ("-", fout);

			}
			break;

		case GTH_TAG_EXIF_CAMERA_MODEL:
			idx = get_image_idx (tag, ce);
			idata = g_list_nth (ce->file_list, idx)->data;
			line = get_exif_tag (idata->src_filename, 
					     EXIF_TAG_MAKE);
			write_markup_escape_line (line, fout);
			g_free (line);

			write_line (" &nbsp; ", fout);

			line = get_exif_tag (idata->src_filename, 
					     EXIF_TAG_MODEL);
			write_markup_escape_line (line, fout);
			break;

		case GTH_TAG_SET_VAR:
			break;

		case GTH_TAG_EVAL:
			idx = get_image_idx (tag, ce);
			idata = g_list_nth (ce->file_list, idx)->data;
			ce->eval_image = idata;

			value = gth_tag_get_var (ce, tag, "expr");

			line = g_strdup_printf ("%d",value);
			write_line (line, fout);
			break;

		case GTH_TAG_IF:
			for (scan = tag->value.cond_list; scan; scan = scan->next) {
				GthCondition *cond = scan->data;
				if (expression_value (ce, cond->expr) != 0) {
					gth_parsed_doc_print (cond->document,
							      ce, 
							      fout, 
							      FALSE);
					break;
				}
			}
			break;

		case GTH_TAG_TEXT:
			if ((tag->value.arg_list == NULL) && (tag->document != NULL)) {
				GthTag *child = tag->document->data;
				if (child->type != GTH_TAG_HTML)
					break;
				line = g_strdup (_(child->value.html));
				write_markup_escape_line (line, fout);
			} 
			break;

		default:
			break;
		}

		g_free (line);
	}
}


static void
exporter_set_info (CatalogWebExporter *ce,
		   const char         *info)
{
	g_free (ce->info);
	ce->info = g_strdup (info);
	g_signal_emit (G_OBJECT (ce), catalog_web_exporter_signals[WEB_EXPORTER_INFO], 
		       0,
		       ce->info);
}


static void
export__final_step (GnomeVFSResult  result,
		    gpointer        data)
{
	CatalogWebExporter *ce = data;
	free_parsed_docs (ce);
	g_signal_emit (G_OBJECT (ce), catalog_web_exporter_signals[WEB_EXPORTER_DONE], 0);
}


static void
export__copy_to_destination__step2 (GnomeVFSResult  result,
				    gpointer        data)
{
	CatalogWebExporter *ce = data;

	debug (DEBUG_INFO, "result: %s", gnome_vfs_result_to_string (result));

	if (result != GNOME_VFS_OK) 
		_gtk_error_dialog_run (GTK_WINDOW (ce->window), 
				       gnome_vfs_result_to_string (result));

	dlg_folder_delete (ce->window,
			   ce->tmp_location,
			   export__final_step,
			   ce);
}


static void
export__copy_to_destination (CatalogWebExporter *ce)
{
	g_signal_emit (G_OBJECT (ce), catalog_web_exporter_signals[WEB_EXPORTER_START_COPYING], 0);

	dlg_files_copy (ce->window,
			ce->album_files,
			ce->location,
			FALSE,
			FALSE,
			TRUE,
			export__copy_to_destination__step2,
			ce);
}


static char *
get_style_dir (CatalogWebExporter *ce)
{
	char *path;

	path = g_build_path (G_DIR_SEPARATOR_S,
			     g_get_home_dir (),
			     ".gnome2",
			     "gthumb/albumthemes",
			     ce->style,
			     NULL);

	if (path_is_dir (path))
		return path;
	g_free (path);

	path = g_build_path (G_DIR_SEPARATOR_S,
			     GTHUMB_DATADIR,
			     "gthumb/albumthemes",
			     ce->style,
			     NULL);

	if (path_is_dir (path))
		return path;

	return NULL;
}


static void
export__save_other_files (CatalogWebExporter *ce)
{
	GnomeVFSResult  result;
	GList          *file_list = NULL;
	char           *source_dir;

	source_dir = get_style_dir (ce);

	if (source_dir != NULL)
		result = gnome_vfs_directory_list_load (&file_list, source_dir, GNOME_VFS_FILE_INFO_DEFAULT);
	else
		result = GNOME_VFS_ERROR_NOT_A_DIRECTORY;

	if (result == GNOME_VFS_OK) {
		GList *scan;

		for (scan = file_list; scan; scan = scan->next) {
			GnomeVFSFileInfo *info = scan->data;
			char             *filename;

			if (info->type == GNOME_VFS_FILE_TYPE_DIRECTORY)
				continue;

			if ((strcmp (info->name, "index.gthtml") == 0)
			    || (strcmp (info->name, "thumbnail.gthtml") == 0)
			    || (strcmp (info->name, "image.gthtml") == 0))
				continue;
			
			filename = g_build_filename (G_DIR_SEPARATOR_S,
						     source_dir,
						     info->name,
						     NULL);

			debug (DEBUG_INFO, "copy %s", filename);

			ce->album_files = g_list_prepend (ce->album_files, filename);
		}
	}

	if (file_list != NULL)
		gnome_vfs_file_info_list_free (file_list);
	g_free (source_dir);

	export__copy_to_destination (ce);	
}


static void
copy_exif_from_orig (const char *src_filename, 
		     const char *dest_filename)
{
	JPEGData     *jdata_src, *jdata_dest;
	ExifData     *edata_src;

	src_filename = get_file_path_from_uri (src_filename);
	if (src_filename == NULL)
		return;

	dest_filename = get_file_path_from_uri (dest_filename);
	if (dest_filename == NULL)
		return;

	jdata_src = jpeg_data_new_from_file (src_filename);
	if (jdata_src == NULL)
		return;

	edata_src = jpeg_data_get_exif_data (jdata_src);
	if (edata_src == NULL) {
		jpeg_data_unref (jdata_src);
		return;
	}

	jdata_dest = jpeg_data_new_from_file (dest_filename);
	if (jdata_dest == NULL)
		return;

	jpeg_data_set_exif_data (jdata_dest, edata_src);

	jpeg_data_save_file (jdata_dest, dest_filename);

	exif_data_unref (edata_src);
	jpeg_data_unref (jdata_src);
	jpeg_data_unref (jdata_dest);

	/* The exif orientation tag, if present, must be reset to "top-left" */
	write_orientation_field (dest_filename, GTH_TRANSFORM_NONE);
}


static gboolean
save_thumbnail_cb (gpointer data)
{
	CatalogWebExporter *ce = data;

	if (ce->saving_timeout != 0) {
		g_source_remove (ce->saving_timeout);
		ce->saving_timeout = 0;
	}

	if (ce->current_image == NULL) 
		export__save_other_files (ce);

	else {
		ImageData *idata = ce->current_image->data;
		
		if (idata->thumb != NULL) {
			char *filename;
			
			g_signal_emit (G_OBJECT (ce), 
				       catalog_web_exporter_signals[WEB_EXPORTER_PROGRESS],
				       0,
				       (float) ce->image / ce->n_images);
			
			filename = get_thumbnail_filename (ce, idata, ce->tmp_location);
			
			debug (DEBUG_INFO, "write %s", filename);
			
			if (_gdk_pixbuf_save (idata->thumb,
					      filename,
					      "jpeg",
					      NULL, NULL)) {
				copy_exif_from_orig (idata->src_filename, filename);
				ce->album_files = g_list_prepend (ce->album_files, filename);
			} else
				g_free (filename);
			
			g_object_unref (idata->thumb);
			idata->thumb = NULL;
		}

		/**/

		ce->current_image = ce->current_image->next;
		ce->image++;

		ce->saving_timeout = g_timeout_add (SAVING_TIMEOUT,
						    save_thumbnail_cb,
						    data);
	}

	return FALSE;
}


static void
export__save_thumbnails (CatalogWebExporter *ce)
{
	exporter_set_info (ce, _("Saving thumbnails"));

	ce->image = 0;
	ce->current_image = ce->file_list;
	ce->saving_timeout = g_timeout_add (SAVING_TIMEOUT,
					    save_thumbnail_cb,
					    ce);
}


static gboolean
save_html_image_cb (gpointer data)
{
	CatalogWebExporter *ce = data;

	if (ce->saving_timeout != 0) {
		g_source_remove (ce->saving_timeout);
		ce->saving_timeout = 0;
	}

	if (ce->current_image == NULL) 
		export__save_thumbnails (ce);

	else {
		ImageData *idata = ce->current_image->data;
		char      *filename;
		FILE      *fout;

		g_signal_emit (G_OBJECT (ce), 
			       catalog_web_exporter_signals[WEB_EXPORTER_PROGRESS],
			       0,
			       (float) ce->image / ce->n_images);

		filename = g_strconcat (ce->tmp_location,
					"/",
					file_name_from_path (idata->dest_filename), 
					".html",
					NULL);

		debug (DEBUG_INFO, "write %s", filename);

		fout = fopen (filename, "w");
		if (fout != NULL) {
			gth_parsed_doc_print (ce->image_parsed, ce, fout, TRUE);
			fclose (fout);
			ce->album_files = g_list_prepend (ce->album_files, filename);
		} else
			g_free (filename);

		/**/

		ce->current_image = ce->current_image->next;
		ce->image++;

		ce->saving_timeout = g_timeout_add (SAVING_TIMEOUT,
						    save_html_image_cb,
						    data);
	}

	return FALSE;
}


static void
export__save_html_files__step2 (CatalogWebExporter *ce)
{
	exporter_set_info (ce, _("Saving HTML pages: Images"));

	ce->image = 0;
	ce->current_image = ce->file_list;
	ce->saving_timeout = g_timeout_add (SAVING_TIMEOUT,
					    save_html_image_cb,
					    ce);
}


static gboolean
save_html_index_cb (gpointer data)
{
	CatalogWebExporter *ce = data;

	if (ce->saving_timeout != 0) {
		g_source_remove (ce->saving_timeout);
		ce->saving_timeout = 0;
	}

	if (ce->page >= ce->n_pages) 
		export__save_html_files__step2 (ce);

	else { /* write index.html and pageXXX.html */
		char *filename;
		FILE *fout;

		g_signal_emit (G_OBJECT (ce), 
			       catalog_web_exporter_signals[WEB_EXPORTER_PROGRESS],
			       0,
			       (float) ce->page / ce->n_pages);

		if (ce->page == 0)
			filename = g_build_filename (ce->tmp_location, 
						     ce->index_file,
						     NULL);
		else {
			char *page_name;

			page_name = g_strconcat ("page",
						 zero_padded (ce->page + 1),
						 ".html",
						 NULL);
			filename = g_build_filename (ce->tmp_location,
						     page_name,
						     NULL);
			g_free (page_name);
		}

		debug (DEBUG_INFO, "write %s", filename);

		fout = fopen (filename, "w");
		if (fout != NULL) {
			gth_parsed_doc_print (ce->index_parsed, ce, fout, TRUE);
			fclose (fout);
			ce->album_files = g_list_prepend (ce->album_files, filename);
		} else
			g_free (filename);

		/**/

		ce->page++;

		ce->saving_timeout = g_timeout_add (SAVING_TIMEOUT,
						    save_html_index_cb,
						    data);
	}

	return FALSE;
}


static void
export__save_html_files (CatalogWebExporter *ce)
{
	exporter_set_info (ce, _("Saving HTML pages: Indexes"));

	if (ce->single_index)
		ce->n_pages = 1;
	else {
		ce->n_pages = ce->n_images / (ce->page_rows * ce->page_cols);
		if (ce->n_images % (ce->page_rows * ce->page_cols) > 0)
			ce->n_pages++;
	}

	ce->image = 0;
	ce->page = 0;
	ce->saving_timeout = g_timeout_add (SAVING_TIMEOUT,
					    save_html_index_cb,
					    ce);
}


static void
load_next_file (CatalogWebExporter *ce)
{
	if (ce->interrupted) { 
		if (ce->file_list != NULL) {
			g_list_foreach (ce->file_list, 
					(GFunc) image_data_free, 
					NULL);
			g_list_free (ce->file_list);
			ce->file_list = NULL;
		}
		dlg_folder_delete (ce->window,
				   ce->tmp_location,
				   export__final_step,
				   ce);
		return;
	}

	/**/

	if (ce->file_to_load != NULL) {
		ImageData *idata = ce->file_to_load->data;
		
		if (idata->preview != NULL) {
			g_object_unref (idata->preview);
			idata->preview = NULL;
		}

		if (idata->image != NULL) {
			g_object_unref (idata->image);
			idata->image = NULL;
		}
	}


	/**/

	g_signal_emit (G_OBJECT (ce), 
		       catalog_web_exporter_signals[WEB_EXPORTER_PROGRESS],
		       0,
		       (float) ++ce->n_images_done / ce->n_images);

	ce->file_to_load = ce->file_to_load->next;
	if (ce->file_to_load == NULL) {
		/* sort list */
		if ((ce->sort_method != GTH_SORT_METHOD_NONE)
		    && (ce->sort_method != GTH_SORT_METHOD_MANUAL))
			ce->file_list = g_list_sort (ce->file_list, get_sortfunc (ce));
		if (ce->sort_type == GTK_SORT_DESCENDING)
			ce->file_list = g_list_reverse (ce->file_list);

		export__save_html_files (ce);

	} else {
		char *filename = IMAGE_DATA (ce->file_to_load->data)->src_filename;
		image_loader_set_path (ce->iloader, filename);
		image_loader_start (ce->iloader);
	}
}


static gboolean
save_image_preview_cb (gpointer data)
{
	CatalogWebExporter *ce = data;

	if (ce->saving_timeout != 0) {
		g_source_remove (ce->saving_timeout);
		ce->saving_timeout = 0;
	}

	if (ce->file_to_load != NULL) {
		ImageData *idata = ce->file_to_load->data;
			
		if ((! idata->no_preview) && (idata->preview != NULL)) {
			char *filename = get_preview_filename (ce, idata, ce->tmp_location);
			
			debug (DEBUG_INFO, "write %s", filename);
		
			if (_gdk_pixbuf_save (idata->preview,
					      filename,
					      "jpeg",
					      NULL, NULL)) {
				copy_exif_from_orig (idata->src_filename, filename);
				ce->album_files = g_list_prepend (ce->album_files, filename);
			} else
				g_free (filename);
		}
	}

	load_next_file (ce);

	return FALSE;
}


static gboolean
save_resized_image_cb (gpointer data)
{
	CatalogWebExporter *ce = data;

	if (ce->saving_timeout != 0) {
		g_source_remove (ce->saving_timeout);
		ce->saving_timeout = 0;
	}

	if (ce->file_to_load != NULL) {
		ImageData *idata = ce->file_to_load->data;
			
		if (ce->copy_images && idata->image != NULL) {
			char *filename = get_image_filename (ce, idata, ce->tmp_location);

			exporter_set_info (ce, _("Saving images"));
			
			debug (DEBUG_INFO, "write %s", filename);
		
			if (_gdk_pixbuf_save (idata->image,
					      filename,
					      "jpeg",
					      NULL, NULL)) {
				copy_exif_from_orig (idata->src_filename, filename);
				ce->album_files = g_list_prepend (ce->album_files, filename);
				idata->file_size = get_file_size (filename);
			} else
				g_free (filename);
		}
	}

	ce->saving_timeout = g_timeout_add (SAVING_TIMEOUT,
					    save_image_preview_cb,
					    ce);

	return FALSE;
}


static int
export__copy_image__progress_update_cb (GnomeVFSAsyncHandle      *handle,
					GnomeVFSXferProgressInfo *info,
					gpointer                  data)
{
	CatalogWebExporter *ce = data;

	if (info->status != GNOME_VFS_XFER_PROGRESS_STATUS_OK) {
		ce->saving_timeout = g_timeout_add (SAVING_TIMEOUT,
						    save_image_preview_cb,
						    ce);
		return FALSE;
		
	} else if (info->phase == GNOME_VFS_XFER_PHASE_COMPLETED) {
		ce->saving_timeout = g_timeout_add (SAVING_TIMEOUT,
						    save_image_preview_cb,
						    ce);
	} 

	return TRUE;
}


static void
export__copy_image (CatalogWebExporter *ce)
{
	ImageData                 *idata;
	char                      *dest_filename;
	GList                     *src_list = NULL;
	GList                     *dest_list = NULL;
	GnomeVFSXferOptions        xfer_options;
	GnomeVFSXferErrorMode      xfer_error_mode;
	GnomeVFSXferOverwriteMode  overwrite_mode;
	GnomeVFSAsyncHandle       *handle;
	GnomeVFSResult             result;

	exporter_set_info (ce, _("Saving images"));

	idata = ce->file_to_load->data;

	dest_filename = get_image_filename (ce, idata, ce->tmp_location);
	src_list = g_list_append (src_list, new_uri_from_path (idata->src_filename));
	dest_list = g_list_append (dest_list, new_uri_from_path (dest_filename));
	ce->album_files = g_list_prepend (ce->album_files, dest_filename);

	xfer_options    = 0;
	xfer_error_mode = GNOME_VFS_XFER_ERROR_MODE_ABORT;
	overwrite_mode  = GNOME_VFS_XFER_OVERWRITE_MODE_REPLACE;

	result = gnome_vfs_async_xfer (&handle,
				       src_list,
				       dest_list,
				       xfer_options,
				       xfer_error_mode,
				       overwrite_mode,
				       GNOME_VFS_PRIORITY_DEFAULT,
				       export__copy_image__progress_update_cb,
				       ce,
				       NULL,
				       NULL);

	g_list_foreach (src_list, (GFunc) gnome_vfs_uri_unref, NULL);
	g_list_free (src_list);
	g_list_foreach (dest_list, (GFunc) gnome_vfs_uri_unref, NULL);
	g_list_free (dest_list);
	
	if (result != GNOME_VFS_OK) 
		ce->saving_timeout = g_timeout_add (SAVING_TIMEOUT,
						    save_image_preview_cb,
						    ce);
}


static GdkPixbuf *
pixbuf_scale (const GdkPixbuf *src,
	      int              dest_width,
	      int              dest_height,
	      GdkInterpType    interp_type)
{
	GdkPixbuf *dest;

	if (! gdk_pixbuf_get_has_alpha (src))
		return gdk_pixbuf_scale_simple (src, dest_width, dest_height, interp_type);
	
	g_return_val_if_fail (src != NULL, NULL);
	g_return_val_if_fail (dest_width > 0, NULL);
	g_return_val_if_fail (dest_height > 0, NULL);
	
	dest = gdk_pixbuf_new (GDK_COLORSPACE_RGB, gdk_pixbuf_get_has_alpha (src), 8, dest_width, dest_height);
	if (!dest)
		return NULL;
	
	gdk_pixbuf_composite_color (src,
				    dest,
				    0, 0, dest_width, dest_height, 0, 0,
				    (double) dest_width / gdk_pixbuf_get_width (src),
				    (double) dest_height / gdk_pixbuf_get_height (src),
				    interp_type,
				    255,
				    0, 0,
				    200,
				    0xFFFFFF,
				    0xFFFFFF);

	return dest;
}
	

static void
image_loader_done (ImageLoader *iloader, 
		   gpointer     data)
{
	CatalogWebExporter *ce = data;
	GdkPixbuf          *pixbuf;
	ImageData          *idata;

	idata = (ImageData*) ce->file_to_load->data;

	/* image */

	idata->image = pixbuf = image_loader_get_pixbuf (iloader);
	g_object_ref (idata->image);

	if (ce->copy_images && ce->resize_images) {
		int w = gdk_pixbuf_get_width (pixbuf);
		int h = gdk_pixbuf_get_height (pixbuf);
		if (scale_keepping_ratio (&w, &h, ce->resize_max_width, ce->resize_max_height)) {
			GdkPixbuf *scaled;
			scaled = pixbuf_scale (pixbuf, w, h, GDK_INTERP_BILINEAR);
			g_object_unref (idata->image);
			idata->image = scaled;
		} 
	}

	idata->image_width = gdk_pixbuf_get_width (idata->image);
	idata->image_height = gdk_pixbuf_get_height (idata->image);

	/* preview */

	idata->preview = pixbuf = image_loader_get_pixbuf (iloader);
	g_object_ref (idata->preview);

	if ((ce->preview_max_width > 0) && (ce->preview_max_height > 0)) {
		int w = gdk_pixbuf_get_width (pixbuf);
		int h = gdk_pixbuf_get_height (pixbuf);

		if (scale_keepping_ratio (&w, &h, 
					  ce->preview_max_width,
					  ce->preview_max_height)) {
			GdkPixbuf *scaled;
			scaled = pixbuf_scale (pixbuf, w, h, GDK_INTERP_BILINEAR);
			g_object_unref (idata->preview);
			idata->preview = scaled;
		}
	}

	idata->preview_width = gdk_pixbuf_get_width (idata->preview);
	idata->preview_height = gdk_pixbuf_get_height (idata->preview);

	idata->no_preview = ((idata->preview_width == idata->image_width) 
			     && (idata->preview_height == idata->image_height));
	
	if (idata->no_preview) 
		if (idata->preview != NULL) {
			g_object_unref (idata->preview);
			idata->preview = NULL;
		}

	/* thumbnail. */

	idata->thumb = pixbuf = image_loader_get_pixbuf (iloader);
	g_object_ref (idata->thumb);

	if ((ce->thumb_width > 0) && (ce->thumb_height > 0)) {
		int w = gdk_pixbuf_get_width (pixbuf);
		int h = gdk_pixbuf_get_height (pixbuf);

		if (scale_keepping_ratio (&w, &h, 
					  ce->thumb_width,
					  ce->thumb_height)) {
			GdkPixbuf *scaled;
			scaled = pixbuf_scale (pixbuf, w, h, GDK_INTERP_BILINEAR);
			g_object_unref (idata->thumb);
			idata->thumb = scaled;
		}
	}

	idata->thumb_width = gdk_pixbuf_get_width (idata->thumb);
	idata->thumb_height = gdk_pixbuf_get_height (idata->thumb);

	/**/

	idata->file_size = get_file_size (idata->src_filename);
	idata->file_time = get_file_mtime (idata->src_filename);

	/* save the image */

	if (! ce->copy_images) 
		ce->saving_timeout = g_timeout_add (SAVING_TIMEOUT,
						    save_image_preview_cb,
						    ce);

	else if (ce->copy_images && ! ce->resize_images) 
		export__copy_image (ce);

	else if (ce->copy_images && ce->resize_images) {
		exporter_set_info (ce, _("Saving images"));
		ce->saving_timeout = g_timeout_add (SAVING_TIMEOUT,
						    save_resized_image_cb,
						    ce);
	}
}


static void
image_loader_error (ImageLoader *iloader, 
		    gpointer     data)
{
	CatalogWebExporter *ce = data;
	load_next_file (ce);
}


static void
parse_theme_files (CatalogWebExporter *ce)
{
	char  *style_dir;
	char  *template;
	GList *scan;

	free_parsed_docs (ce);

	style_dir = get_style_dir (ce);

	debug (DEBUG_INFO, "style dir: %s", style_dir);

	ce->image = 0;

	/* read and parse index.gthtml */

	yy_parsed_doc = NULL;
	template = g_build_filename (style_dir, "index.gthtml", NULL);
	yyin = fopen (template, "r");

	debug (DEBUG_INFO, "load %s", template);

	if ((yyin != NULL) && (yyparse () == 0)) 
		ce->index_parsed = yy_parsed_doc;
	else 
		debug (DEBUG_INFO, "<<syntax error>>");

	if (yyin != NULL)
		fclose (yyin);

	if (ce->index_parsed == NULL) {
		GthTag *tag = gth_tag_new (GTH_TAG_TABLE, NULL);
		ce->index_parsed = g_list_prepend (NULL, tag);
	}

	g_free (template);

	/* read and parse thumbnail.gthtml */

	yy_parsed_doc = NULL;
	template = g_build_filename (style_dir, "thumbnail.gthtml", NULL);
	yyin = fopen (template, "r");

	debug (DEBUG_INFO, "load %s", template);

	if ((yyin != NULL) && (yyparse () == 0)) 
		ce->thumbnail_parsed = yy_parsed_doc;
	else 
		debug (DEBUG_INFO, "<<syntax error>>");

	if (yyin != NULL)
		fclose (yyin);

	if (ce->thumbnail_parsed == NULL) {
		GthExpr *expr;
		GthVar  *var;
		GList   *vars = NULL;
		GthTag  *tag;

		expr = gth_expr_new ();
		gth_expr_push_constant (expr, 0);
		var = gth_var_new_expression ("idx_relative", expr);
		vars = g_list_prepend (vars, var);

		expr = gth_expr_new ();
		gth_expr_push_constant (expr, 1);
		var = gth_var_new_expression ("thumbnail", expr);
		vars = g_list_prepend (vars, var);

		tag = gth_tag_new (GTH_TAG_IMAGE, vars);
		ce->thumbnail_parsed = g_list_prepend (NULL, tag);
	}

	g_free (template);

	/* Read and parse image.gthtml */

	yy_parsed_doc = NULL;
	template = g_build_filename (style_dir, "image.gthtml", NULL);
	yyin = fopen (template, "r");

	debug (DEBUG_INFO, "load %s", template);

	if ((yyin != NULL) && (yyparse () == 0)) 
		ce->image_parsed = yy_parsed_doc;
	else 
		debug (DEBUG_INFO, "<<syntax error>>");

	if (yyin != NULL)
		fclose (yyin);

	if (ce->image_parsed == NULL) {
		GthExpr *expr;
		GthVar  *var;
		GList   *vars = NULL;
		GthTag  *tag;

		expr = gth_expr_new ();
		gth_expr_push_constant (expr, 0);
		var = gth_var_new_expression ("idx_relative", expr);
		vars = g_list_prepend (vars, var);

		expr = gth_expr_new ();
		gth_expr_push_constant (expr, 0);
		var = gth_var_new_expression ("thumbnail", expr);
		vars = g_list_prepend (vars, var);

		tag = gth_tag_new (GTH_TAG_IMAGE, vars);
		ce->image_parsed = g_list_prepend (NULL, tag);
	}

	g_free (template);
	g_free (style_dir);

	/* read index.html and set variables. */

	for (scan = ce->index_parsed; scan; scan = scan->next) {
		GthTag *tag = scan->data;
		int     width, height;

		switch (tag->type) {
		case GTH_TAG_SET_VAR:
			width = gth_tag_get_var (ce, tag, "thumbnail_width");
			height = gth_tag_get_var (ce, tag, "thumbnail_height");

			if ((width != 0) && (height != 0)) {
				debug (DEBUG_INFO, "thumbnail --> %dx%d", width, height);
				catalog_web_exporter_set_thumb_size (ce, width, height);
				break;
			}

			/**/

			width = gth_tag_get_var (ce, tag, "preview_width");
			height = gth_tag_get_var (ce, tag, "preview_height");

			if ((width != 0) && (height != 0)) {
				debug (DEBUG_INFO, "preview --> %dx%d", width, height);
				catalog_web_exporter_set_preview_size (ce, width, height);
				break;
			}

			break;

		default:
			break;
		}
	}
}


void
catalog_web_exporter_export (CatalogWebExporter *ce)
{
	g_return_if_fail (IS_CATALOG_WEB_EXPORTER (ce));

	if ((ce->exporting) || (ce->file_list == NULL))
		return;
	ce->exporting = TRUE;

	/**/

	g_free (ce->tmp_location);
	ce->tmp_location = get_temp_dir_name ();

	if (ce->tmp_location == NULL) {
		_gtk_error_dialog_run (GTK_WINDOW (ce->window), _("Could not create a temporary folder"));
		g_signal_emit (G_OBJECT (ce), catalog_web_exporter_signals[WEB_EXPORTER_DONE], 0);
		return;
	}

	if (ce->album_files != NULL) {
		g_list_foreach (ce->album_files, (GFunc) g_free, NULL);
		g_list_free (ce->album_files);
		ce->album_files = NULL;
	}

	parse_theme_files (ce);

	debug (DEBUG_INFO, "temp dir: %s", ce->tmp_location);
	debug (DEBUG_INFO, "thumb size: %dx%d", ce->thumb_width, ce->thumb_height);

	/**/

	if (ce->iloader != NULL)
		g_object_unref (ce->iloader);

	ce->iloader = IMAGE_LOADER (image_loader_new (NULL, FALSE));
	g_signal_connect (G_OBJECT (ce->iloader), 
			  "image_done",
			  G_CALLBACK (image_loader_done),
			  ce);
	g_signal_connect (G_OBJECT (ce->iloader), 
			  "image_error",
			  G_CALLBACK (image_loader_error),
			  ce);

	/* Load thumbnails. */

	exporter_set_info (ce, _("Loading images"));

	ce->n_images = g_list_length (ce->file_list);
	ce->n_images_done = 0;
		
	ce->file_to_load = ce->file_list;
	image_loader_set_path (ce->iloader, 
			       IMAGE_DATA (ce->file_to_load->data)->src_filename);
	image_loader_start (ce->iloader);
}


void
catalog_web_exporter_interrupt (CatalogWebExporter *ce)
{
	g_return_if_fail (IS_CATALOG_WEB_EXPORTER (ce));

	if (! ce->exporting)
		return;
	ce->interrupted = TRUE;
}
