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
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <gtk/gtk.h>
#include <libgnome/libgnome.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <libgnomevfs/gnome-vfs-directory.h>
#include "catalog-web-exporter.h"
#include "comments.h"
#include "file-utils.h"
#include "dlg-file-utils.h"
#include "gthumb-init.h"
#include "gthumb-marshal.h"
#include "gtk-utils.h"
#include "pixbuf-utils.h"
#include "thumb-loader.h"
#include "image-loader.h"
#include "glib-utils.h"
#include "albumtheme-private.h"


enum {
	DONE,
	PROGRESS,
	INFO,
	START_COPYING,
	LAST_SIGNAL
};

extern int yyparse (void);

static GObjectClass *parent_class = NULL;
static guint         catalog_web_exporter_signals[LAST_SIGNAL] = { 0 };
extern FILE         *yyin;

#define DEFAULT_THUMB_SIZE 100
#define DEFAULT_INDEX_FILE "index.html"
#define SAVING_TIMEOUT 5
#define DEFAULT_MAX_WIDTH 640
#define DEFAULT_MAX_HEIGHT 480


typedef struct {
	char             *comment;
	char             *filename;
	GnomeVFSFileSize  file_size;
	time_t            file_time;
	GdkPixbuf        *image;
	GdkPixbuf        *thumb;
	int               image_width;
	int               image_height;
	gboolean          caption_set;
} ImageData;

#define IMAGE_DATA(x) ((ImageData*)(x))


static ImageData *
image_data_new (char *filename)
{
	ImageData   *idata;
	CommentData *cdata;

	idata = g_new (ImageData, 1);

	cdata = comments_load_comment (filename);
	idata->comment = comments_get_comment_as_string (cdata, "&nbsp;<BR>", "&nbsp;<BR>");
	if (cdata != NULL)
		comment_data_free (cdata);

	idata->filename = g_strdup (filename);
	idata->file_size = 0;
	idata->image = NULL;
	idata->thumb = NULL;
	idata->image_width = 0;
	idata->image_height = 0;

	idata->caption_set = FALSE;

	return idata;
}


static void
image_data_free (ImageData *idata)
{
	g_free (idata->comment);
	g_free (idata->filename);

	if (idata->image != NULL)
		g_object_unref (idata->image);
	if (idata->thumb != NULL)
		g_object_unref (idata->thumb);

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

	g_free (ce->title);
	ce->title = NULL;

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

	if (ce->tloader != NULL) {
		g_object_unref (G_OBJECT (ce->tloader));
		ce->tloader = NULL;
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

	catalog_web_exporter_signals[DONE] =
		g_signal_new ("done",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (CatalogWebExporterClass, done),
			      NULL, NULL,
			      gthumb_marshal_VOID__VOID,
			      G_TYPE_NONE, 
			      0);

	catalog_web_exporter_signals[PROGRESS] =
		g_signal_new ("progress",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (CatalogWebExporterClass, progress),
			      NULL, NULL,
			      gthumb_marshal_VOID__FLOAT,
			      G_TYPE_NONE, 
			      1, G_TYPE_FLOAT);

	catalog_web_exporter_signals[INFO] =
		g_signal_new ("info",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (CatalogWebExporterClass, info),
			      NULL, NULL,
			      gthumb_marshal_VOID__STRING,
			      G_TYPE_NONE, 
			      1, G_TYPE_STRING);

	catalog_web_exporter_signals[START_COPYING] =
		g_signal_new ("start_copying",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (CatalogWebExporterClass, start_copying),
			      NULL, NULL,
			      gthumb_marshal_VOID__VOID,
			      G_TYPE_NONE, 
			      0);

	object_class->finalize = catalog_web_exporter_finalize;
}


static void
catalog_web_exporter_init (CatalogWebExporter *ce)
{
	ce->title = NULL;
	ce->style = NULL;
	ce->tmp_location = NULL;
	ce->location = NULL;
	ce->index_file = g_strdup (DEFAULT_INDEX_FILE);
	ce->file_list = NULL;
	ce->album_files = NULL;
	ce->tloader = NULL;

	ce->thumb_width = DEFAULT_THUMB_SIZE;
	ce->thumb_height = DEFAULT_THUMB_SIZE;

	ce->resize_images = FALSE;
	ce->resize_max_width = DEFAULT_MAX_WIDTH;
	ce->resize_max_height = DEFAULT_MAX_HEIGHT;
}


GType
catalog_web_exporter_get_type ()
{
	static guint type = 0;

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
catalog_web_exporter_new (GThumbWindow *window,
			  GList        *file_list)
{
	CatalogWebExporter *ce;
	GList              *scan;

	g_return_val_if_fail (window != NULL, NULL);

	ce = CATALOG_WEB_EXPORTER (g_object_new (CATALOG_WEB_EXPORTER_TYPE, NULL));

	ce->window = window;

	for (scan = file_list; scan; scan = scan->next) {
		ImageData *idata;

		idata = image_data_new ((char*) scan->data);
		ce->file_list = g_list_prepend (ce->file_list, idata);
	}
	ce->file_list = g_list_reverse (ce->file_list);

	return ce;
}


void
catalog_web_exporter_set_title (CatalogWebExporter *ce,
				const char         *title)
{
	g_return_if_fail (IS_CATALOG_WEB_EXPORTER (ce));
	g_free (ce->title);
	ce->title = g_strdup (title);
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
catalog_web_exporter_set_thumb_size (CatalogWebExporter *ce,
				     int                 width,
				     int                 height)
{
	g_return_if_fail (IS_CATALOG_WEB_EXPORTER (ce));
	ce->thumb_width = width;
	ce->thumb_height = height;
}


void
catalog_web_exporter_set_caption (CatalogWebExporter *ce,
				  GthCaptionFields    caption)
{
	g_return_if_fail (IS_CATALOG_WEB_EXPORTER (ce));
	ce->caption_fields = caption;
}


static int
comp_func_name (gconstpointer a, gconstpointer b)
{
	ImageData *data_a, *data_b;

	data_a = IMAGE_DATA (a);
	data_b = IMAGE_DATA (b);

	return strcasecmp (file_name_from_path (data_a->filename), 
			   file_name_from_path (data_b->filename));
}


static int
comp_func_path (gconstpointer a, gconstpointer b)
{
	ImageData *data_a, *data_b;

	data_a = IMAGE_DATA (a);
	data_b = IMAGE_DATA (b);

	return strcasecmp (data_a->filename, data_b->filename);
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
	return 0;
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
	case GTH_SORT_METHOD_NONE:
		func = comp_func_none;
		break;
	default:
		func = comp_func_none;
		break;
	}

	return func;
}


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
			g_print ("default value: %d\n", default_value);
			retval = default_value + expression_value (ce, var->expr);
			g_print ("return value: %d\n", retval);
			break;

		} else if (strcmp (var->name, "idx") == 0) {
			retval = expression_value (ce, var->expr) - 1;
			break;
		}
	}

	retval = MIN (retval, max_value - 1);
	retval = MAX (retval, 0);

	return retval;
}


static int
get_image_idx (GthTag             *tag,
	       CatalogWebExporter *ce)
{
	return gth_tag_get_idx (tag, ce, ce->image, ce->n_images);	
}


static int
get_page_idx (GthTag             *tag,
	      CatalogWebExporter *ce)
{
	return gth_tag_get_idx (tag, ce, ce->page, ce->n_pages);	
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
			return expression_value (ce, var->expr);
	}

	return 0;
}


static int
get_page_idx_from_image_idx (CatalogWebExporter *ce,
			     int                 image_idx)
{
	return image_idx / (ce->page_rows * ce->page_cols);
}


static void
write_line (const char *line, FILE *fout) 
{
	if (line == NULL)
		return;
	fwrite (line, sizeof (char), strlen (line), fout);
}


static char *
get_thumbnail_filename (CatalogWebExporter *ce,
			ImageData          *idata)
{
	return g_strconcat (ce->location,
			    "/",
			    "small.", 
			    file_name_from_path (idata->filename), 
			    ".jpeg",
			    NULL);
}


static char *
get_filename_with_other_ext (const char *filename,
			     const char *new_ext)
{
	char *name_wo_ext;
	char *newname;

	name_wo_ext = remove_extension_from_path (file_name_from_path (filename));
	newname = g_strconcat (name_wo_ext, new_ext, NULL);

	return newname;
}


static char *
get_image_filename (CatalogWebExporter *ce,
		    ImageData          *idata)
{
	if (ce->copy_images && ! ce->resize_images) {
		char *filename;
		filename = g_strconcat (ce->location,
					"/",
					file_name_from_path (idata->filename),
					NULL);
		return filename;
	} 

	if (ce->copy_images && ce->resize_images) {
		char *jpeg_name = get_filename_with_other_ext (idata->filename, ".jpeg");
		char *filename;
		filename = g_strconcat (ce->location,
					"/",
					jpeg_name,
					NULL);
		g_free (jpeg_name);
		return filename;
	} 
	
	return g_strdup (idata->filename);
}


static void
gth_parsed_doc_print (GList              *index_parsed,
		      CatalogWebExporter *ce,
		      FILE               *fout,
		      gboolean            allow_table)
{
	GList *scan;

	for (scan = index_parsed; scan; scan = scan->next) {
		GthTag    *tag = scan->data;
		ImageData *idata;
		char      *line = NULL;
		char      *image_src, *image_src_relative;
		int        idx;
		int        image_width;
		int        image_height;
		int        max_size;
		int        border;
		int        r, c;
		char      *filename;
		GList     *scan;

		switch (tag->type) {
		case GTH_TAG_TITLE:
			write_line (ce->title, fout);
			break;

		case GTH_TAG_IMAGE:
			idx = get_image_idx (tag, ce);
			idata = g_list_nth (ce->file_list, idx)->data;

			if (gth_tag_get_var (ce, tag, "thumbnail") != 0) {
				image_src = get_thumbnail_filename (ce, idata);
				image_width = gdk_pixbuf_get_width (idata->thumb);
				image_height = gdk_pixbuf_get_height (idata->thumb);
			} else {
				image_src = get_image_filename (ce, idata);
				image_width = idata->image_width;
				image_height = idata->image_height;
			}

			border = gth_tag_get_var (ce, tag, "border");
			max_size = gth_tag_get_var (ce, tag, "max_size");

			if ((max_size > 0) 
			    && ((image_width > max_size)
				 || (image_height > max_size))) {
				double factor;

				factor = MIN ((double) max_size / image_width,
					      (double) max_size / image_height);
				
				image_width = factor * image_width - 0.5;
				image_height = factor * image_height - 0.5;
			}

			image_src_relative = get_path_relative_to_dir (image_src, 
								       ce->location);

			line = g_strdup_printf ("<img src=\"%s\" width=\"%d\" height=\"%d\" border=\"%d\" align=\"middle\">\n", 
						image_src_relative,
						image_width,
						image_height,
						border);
			g_free (image_src);
			g_free (image_src_relative);
			write_line (line, fout);
			break;

		case GTH_TAG_IMAGE_LINK:
			idx = get_image_idx (tag, ce);
			idata = g_list_nth (ce->file_list, idx)->data;
			line = g_strconcat (file_name_from_path (idata->filename), 
					    ".html",
					    NULL);
			write_line (line, fout);
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

			if (gth_tag_get_var (ce, tag, "with_path") != 0) {
				line = get_image_filename (ce, idata);

			} else if (gth_tag_get_var (ce, tag, "with_relative_path") != 0) {
				char *path = get_image_filename (ce, idata);
				line = get_path_relative_to_dir (path, ce->location);
				g_free (path);

			} else
				line = g_strdup (file_name_from_path (idata->filename));

			write_line (line, fout);
			break;

		case GTH_TAG_FILEPATH:
			idx = get_image_idx (tag, ce);
			idata = g_list_nth (ce->file_list, idx)->data;
			filename = get_image_filename (ce, idata);
			if (gth_tag_get_var (ce, tag, "relative_path") != 0) {
				char *tmp;
				tmp = get_path_relative_to_dir (filename, ce->location);
				g_free (filename);
				filename = tmp;
			}
			line = remove_level_from_path (filename);
			g_free (filename);
			
			write_line (line, fout);
			break;

		case GTH_TAG_FILESIZE:
			idx = get_image_idx (tag, ce);
			idata = g_list_nth (ce->file_list, idx)->data;
			line = gnome_vfs_format_file_size_for_display (idata->file_size);
			write_line (line, fout);
			break;

		case GTH_TAG_COMMENT:
			idx = get_image_idx (tag, ce);
			idata = g_list_nth (ce->file_list, idx)->data;
			
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

			if (line != NULL) {
				char *escaped;
				escaped = g_locale_from_utf8 (line, -1, 0, 0, 0);
				g_free (line);
				line = escaped;
			}

			write_line (line, fout);
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
			write_line (line, fout);
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

			for (r = 0; r < ce->page_rows; r++) {
				write_line ("  <tr height=\"100%\">\n", fout);
				for (c = 0; c < ce->page_cols; c++) {
					if (ce->image < ce->n_images) {
						write_line ("    <td  align=\"center\" valign=\"top\">\n", fout);
						gth_parsed_doc_print (ce->thumbnail_parsed, 
								      ce, 
								      fout, 
								      FALSE);
						write_line ("    </td>\n", fout);
						ce->image++;
					} 
				}
				write_line ("  </tr>\n", fout);
			}
			break;

		case GTH_TAG_DATE:
			{
				time_t     t;
				struct tm *tp;
				char       s[100];

				t = time (NULL);
				tp = localtime (&t);
				strftime (s, 99, "%A %x, %X", tp);
				write_line (s, fout);
			}
			break;

		case GTH_TAG_TEXT:
			write_line (tag->value.text, fout);
			break;

		case GTH_TAG_SET_VAR:
			break;

		case GTH_TAG_IF:
			for (scan = tag->value.cond_list; scan; scan = scan->next) {
				GthCondition *cond = scan->data;
				if (expression_value (ce, cond->expr) != 0) {
					gth_parsed_doc_print (cond->parsed_doc,
							      ce, 
							      fout, 
							      FALSE);
					break;
				}
			}
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
	g_signal_emit (G_OBJECT (ce), catalog_web_exporter_signals[INFO], 
		       0,
		       ce->info);
}


static void
export__final_step (GnomeVFSResult  result,
		    gpointer        data)
{
	CatalogWebExporter *ce = data;
	free_parsed_docs (ce);
	g_signal_emit (G_OBJECT (ce), catalog_web_exporter_signals[DONE], 0);
}


static void
export__copy_to_destination__step2 (GnomeVFSResult  result,
				    gpointer        data)
{
	CatalogWebExporter *ce = data;

#ifdef DEBUG
	g_print ("result: %s\n", gnome_vfs_result_to_string (result));
#endif

	dlg_folder_delete (ce->window,
			   ce->tmp_location,
			   export__final_step,
			   ce);
}


static void
export__copy_to_destination (CatalogWebExporter *ce)
{
	g_signal_emit (G_OBJECT (ce), catalog_web_exporter_signals[START_COPYING], 0);

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

#ifdef DEBUG
			g_print ("copy %s\n", filename);
#endif

			ce->album_files = g_list_prepend (ce->album_files, filename);
		}
	}

	if (file_list != NULL)
		gnome_vfs_file_info_list_free (file_list);
	g_free (source_dir);

	export__copy_to_destination (ce);	
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
		char      *filename;
		
		g_signal_emit (G_OBJECT (ce), 
			       catalog_web_exporter_signals[PROGRESS],
			       0,
			       (float) ce->image / ce->n_images);

		filename = g_strconcat (ce->tmp_location,
					"/",
					"small.", 
					file_name_from_path (idata->filename), 
					".jpeg",
					NULL);

#ifdef DEBUG
		g_print ("write %s\n", filename);
#endif

		if (_gdk_pixbuf_save (idata->thumb,
				      filename,
				      "jpeg",
				      NULL, NULL))
			ce->album_files = g_list_prepend (ce->album_files, filename);
		else
			g_free (filename);

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
			       catalog_web_exporter_signals[PROGRESS],
			       0,
			       (float) ce->image / ce->n_images);

		filename = g_strconcat (ce->tmp_location,
					"/",
					file_name_from_path (idata->filename), 
					".html",
					NULL);

#ifdef DEBUG
		g_print ("write %s\n", filename);
#endif

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
			       catalog_web_exporter_signals[PROGRESS],
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

#ifdef DEBUG
		g_print ("write %s\n", filename);
#endif		

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

	ce->n_pages = ce->n_images / (ce->page_rows * ce->page_cols);
	if (ce->n_images % (ce->page_rows * ce->page_cols) > 0)
		ce->n_pages++;

	ce->image = 0;
	ce->page = 0;
	ce->saving_timeout = g_timeout_add (SAVING_TIMEOUT,
					    save_html_index_cb,
					    ce);
}


static gboolean
save_resized_image_cb (gpointer data)
{
	CatalogWebExporter *ce = data;

	if (ce->saving_timeout != 0) {
		g_source_remove (ce->saving_timeout);
		ce->saving_timeout = 0;
	}

	if (ce->current_image == NULL) 
		export__save_html_files (ce);

	else {
		ImageData *idata = ce->current_image->data;
		char      *jpeg_name = get_filename_with_other_ext (idata->filename, ".jpeg");
		char      *filename;
		
		g_signal_emit (G_OBJECT (ce), 
			       catalog_web_exporter_signals[PROGRESS],
			       0,
			       (float) ce->image / ce->n_images);

		filename = g_strconcat (ce->tmp_location,
					"/",
					jpeg_name,
					NULL);
		g_free (jpeg_name);

#ifdef DEBUG
		g_print ("write %s\n", filename);
#endif

		if (_gdk_pixbuf_save (idata->image,
				      filename,
				      "jpeg",
				      NULL, NULL))
			ce->album_files = g_list_prepend (ce->album_files, filename);
		else
			g_free (filename);

		/**/

		ce->current_image = ce->current_image->next;
		ce->image++;

		ce->saving_timeout = g_timeout_add (SAVING_TIMEOUT,
						    save_resized_image_cb,
						    data);
	}

	return FALSE;
}


static void
export__save_images (CatalogWebExporter *ce)
{
	if (! ce->copy_images) {
		export__save_html_files (ce);

	} else if (ce->copy_images && ! ce->resize_images) {
		GList *scan;
		for (scan = ce->file_list; scan; scan = scan->next) {
			ImageData *idata = scan->data;
			ce->album_files = g_list_prepend (ce->album_files, g_strdup (idata->filename));
		}
		export__save_html_files (ce);

	} else if (ce->copy_images && ce->resize_images) {
		exporter_set_info (ce, _("Saving images"));
		ce->image = 0;
		ce->current_image = ce->file_list;
		ce->saving_timeout = g_timeout_add (SAVING_TIMEOUT,
						    save_resized_image_cb,
						    ce);
	}
}


static void
load_next_file (CatalogWebExporter *ce)
{
	char *filename;

	if (ce->interrupted) {
		if (ce->file_list != NULL) {
			g_list_foreach (ce->file_list, 
					(GFunc) image_data_free, 
					NULL);
			g_list_free (ce->file_list);
			ce->file_list = NULL;
		}
		g_signal_emit (G_OBJECT (ce), catalog_web_exporter_signals[DONE], 0);
		return;
	}

	g_signal_emit (G_OBJECT (ce), 
		       catalog_web_exporter_signals[PROGRESS],
		       0,
		       (float) ++ce->n_images_done / ce->n_images);

	ce->file_to_load = ce->file_to_load->next;
	if (ce->file_to_load == NULL) {
		/* sort list */
		ce->file_list = g_list_sort (ce->file_list, get_sortfunc (ce));
		if (ce->sort_type == GTK_SORT_DESCENDING)
			ce->file_list = g_list_reverse (ce->file_list);

		export__save_images (ce);

		return;
	}

	filename = IMAGE_DATA (ce->file_to_load->data)->filename;
	thumb_loader_set_path (ce->tloader, filename);
	thumb_loader_start (ce->tloader);
}


static void
thumb_loader_done (ThumbLoader *tloader, 
		   gpointer     data)
{
	CatalogWebExporter *ce = data;
	ImageLoader        *il;
	GdkPixbuf          *pixbuf;
	ImageData          *idata;

	idata = (ImageData*) ce->file_to_load->data;

	/* thumbnail. */

	pixbuf = thumb_loader_get_pixbuf (tloader);
	g_object_ref (pixbuf);
	idata->thumb = pixbuf;
	
	/* image width and height. */

	il = thumb_loader_get_image_loader (tloader);
	pixbuf = image_loader_get_pixbuf (il);
	idata->image_width = gdk_pixbuf_get_width (pixbuf);
	idata->image_height = gdk_pixbuf_get_height (pixbuf);

	/* resize if requested. */

	if (ce->resize_images) {
		int w = gdk_pixbuf_get_width (pixbuf);
		int h = gdk_pixbuf_get_height (pixbuf);
		if ((w > ce->resize_max_width) || (h > ce->resize_max_height)) {
			float      max_w = ce->resize_max_width;
			float      max_h = ce->resize_max_height;
			float      factor;
			int        new_w, new_h;
			GdkPixbuf *scaled;

			factor = MIN (max_w / w, max_h / h);
			new_w  = MAX ((int) (w * factor + 0.5), 1);
			new_h  = MAX ((int) (h * factor + 0.5), 1);
			scaled = gdk_pixbuf_scale_simple (pixbuf, new_w, new_h, GDK_INTERP_BILINEAR);
			idata->image = scaled;
			idata->image_width = gdk_pixbuf_get_width (idata->image);
			idata->image_height = gdk_pixbuf_get_height (idata->image);
		} else {
			idata->image = pixbuf;
			g_object_ref (idata->image);
		}
	}

	/* file size. */
	idata->file_size = get_file_size (idata->filename);

	/* file time. */
	idata->file_time = get_file_mtime (idata->filename);

	load_next_file (ce);
}


static void
thumb_loader_error (ThumbLoader *tloader, 
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

#ifdef DEBUG
	g_print ("style dir: %s\n", style_dir);
#endif

	ce->image = 0;

	/* read and parse index.gthtml */

	yy_parsed_doc = NULL;
	template = g_build_filename (style_dir, "index.gthtml", NULL);
	yyin = fopen (template, "r");

#ifdef DEBUG
	g_print ("load %s\n", template);
#endif

	if ((yyin != NULL) && (yyparse () == 0)) 
		ce->index_parsed = yy_parsed_doc;
	else {
#ifdef DEBUG
		g_print ("<<syntax error>>\n");
#endif
	}

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

#ifdef DEBUG
	g_print ("load %s\n", template);
#endif

	if ((yyin != NULL) && (yyparse () == 0)) 
		ce->thumbnail_parsed = yy_parsed_doc;
	else {
#ifdef DEBUG
		g_print ("<<syntax error>>\n");
#endif
	}

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

#ifdef DEBUG
	g_print ("load %s\n", template);
#endif

	if ((yyin != NULL) && (yyparse () == 0)) 
		ce->image_parsed = yy_parsed_doc;
	else {
#ifdef DEBUG
		g_print ("<<syntax error>>\n");
#endif
	}

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
#ifdef DEBUG
			g_print ("--> %dx%d\n", width, height);
#endif
			if ((width != 0) && (height != 0))
				catalog_web_exporter_set_thumb_size (ce, width, height);
			break;

		default:
			break;
		}
	}
}


static char *
get_temp_dir_name (void)
{
	static int  count = 0;
	char       *tmp_dir = NULL;
	
	do {
		g_free (tmp_dir);
		tmp_dir = g_strdup_printf ("%s%s.%d.%d",
					   g_get_tmp_dir (),
					   "/gthumb",
					   getpid (),
					   count++);

	} while (path_is_dir (tmp_dir));

	if (mkdir (tmp_dir, 0700) != 0) {
		g_free (tmp_dir);
		return NULL;
	}

	return tmp_dir;
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
		_gtk_error_dialog_run (GTK_WINDOW (ce->window->app), _("Could not create a temporary folder"));
		g_signal_emit (G_OBJECT (ce), catalog_web_exporter_signals[DONE], 0);
		return;
	}

	if (ce->album_files != NULL) {
		g_list_foreach (ce->album_files, (GFunc) g_free, NULL);
		g_list_free (ce->album_files);
		ce->album_files = NULL;
	}

	parse_theme_files (ce);

#ifdef DEBUG
	g_print ("temp dir: %s\n", ce->tmp_location);
	g_print ("thumb size: %dx%d\n", ce->thumb_width, ce->thumb_height);
#endif

	/**/

	if (ce->tloader != NULL)
		g_object_unref (G_OBJECT (ce->tloader));

	ce->tloader = THUMB_LOADER (thumb_loader_new (NULL, 
						      ce->thumb_width,
						      ce->thumb_height));
	thumb_loader_use_cache (ce->tloader, FALSE);

	g_signal_connect (G_OBJECT (ce->tloader), 
			  "done",
			  G_CALLBACK (thumb_loader_done),
			  ce);
	g_signal_connect (G_OBJECT (ce->tloader), 
			  "error",
			  G_CALLBACK (thumb_loader_error),
			  ce);

	/* Load thumbnails. */

	exporter_set_info (ce, _("Loading images"));

	ce->n_images = g_list_length (ce->file_list);
	ce->n_images_done = 0;
		
	ce->file_to_load = ce->file_list;
	thumb_loader_set_path (ce->tloader, 
			       IMAGE_DATA (ce->file_to_load->data)->filename);
	thumb_loader_start (ce->tloader);
}


void
catalog_web_exporter_interrupt (CatalogWebExporter *ce)
{
	g_return_if_fail (IS_CATALOG_WEB_EXPORTER (ce));

	if (! ce->exporting)
		return;
	ce->interrupted = TRUE;
}
