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

#include <config.h>
#include <stdio.h>
#include <string.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gdk/gdkrgb.h>
#include <gio/gio.h>
#include <libgnomevfs/gnome-vfs.h>

#include "catalog-png-exporter.h"
#include "comments.h"
#include "file-utils.h"
#include "gthumb-init.h"
#include "gthumb-marshal.h"
#include "pixbuf-utils.h"
#include "thumb-loader.h"
#include "image-loader.h"
#include "gthumb-slide.h"
#include "glib-utils.h"
#include "main.h"
#include "gth-sort-utils.h"


static void begin_export       (CatalogPngExporter *ce);

static void end_export         (CatalogPngExporter *ce);

static void begin_page         (CatalogPngExporter *ce,
				int                 page_n);

static void end_page           (CatalogPngExporter *ce,
				int                 page_n);

static void paint_frame        (CatalogPngExporter *ce,
				GdkRectangle       *frame_rect,
				GdkRectangle       *image_rect,
				gchar              *filename);

static void paint_image        (CatalogPngExporter *ce,
				GdkRectangle       *image_rect,
				GdkPixbuf          *image);

static void paint_text         (CatalogPngExporter *ce,
				const char         *font_name,
				GdkColor           *color,
				int                 x,
				int                 y,
				int                 width,
				const char         *text,
				int                *height);

static void paint_comment      (CatalogPngExporter *ce,
				int                 x,
				int                 y,
				char               *text,
				int                *height);

static int  get_text_height    (CatalogPngExporter *ce,
				const char         *text,
				const char         *font_name,
				int                 width);

static int  get_header_height_with_spacing  (CatalogPngExporter *ce);

static int  get_footer_height               (CatalogPngExporter *ce);

static int  get_footer_height_with_spacing  (CatalogPngExporter *ce);

static void paint_footer                    (CatalogPngExporter *ce,
					     int                 page_n);

static char *get_footer_text                (CatalogPngExporter *ce,
					     int                 page_n);

static char *get_header_text                (CatalogPngExporter *ce,
					     int                 page_n);


#define HTML_PAGE_BEGIN "\
<?xml version=\"1.0\" encoding=\"utf-8\"?>\n\
<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\"\n\
  \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n\
<html xmlns=\"http://www.w3.org/1999/xhtml\">\n\
<head>\n\
<title></title>\n\
<style type=\"text/css\">\n\
html { margin: 0px; border: 0px; padding: 0px; }\n\
body { margin: 0px; }\n\
img  { border: 0px; }\n\
</style>\n\
</head>\n\
<body>\n\
<div>\n\
"

#define HTML_PAGE_END "\
</div>\n\
</body>\n\
</html>\n\
"


#define COL_SPACING      15
#define ROW_SPACING      15
#define THUMB_FRAME_BORDER     8
#define TEXT_SPACING     3
#define CAPTION_MAX_ROWS 4

#define DEFAULT_FONT "Sans 12"

typedef struct {
	FileData         *file;
	char             *comment;
	GdkPixbuf        *thumb;
	int               image_width;
	int               image_height;
	char             *caption_row[CAPTION_MAX_ROWS];
	gboolean          caption_set;
} ImageData;

#define IMAGE_DATA(x) ((ImageData*)(x))


static ImageData *
image_data_new (FileData *file)
{
	ImageData   *idata;
	CommentData *cdata;
	int          i;

	idata = g_new0 (ImageData, 1);

	idata->file = file_data_ref (file);
	cdata = comments_load_comment (file->path, TRUE);
	if (cdata != NULL) {
		idata->comment = comments_get_comment_as_string (cdata, "\n", "\n");
		comment_data_free (cdata);
	}
	idata->thumb = NULL;
	idata->image_width = 0;
	idata->image_height = 0;

	for (i = 0; i < CAPTION_MAX_ROWS; i++)
		idata->caption_row[i] = NULL;
	idata->caption_set = FALSE;

	return idata;
}


static void
image_data_free (ImageData *idata)
{
	int i;

	g_free (idata->comment);
	file_data_unref (idata->file);

	if (idata->thumb)
		g_object_unref (G_OBJECT (idata->thumb));

	for (i = 0; i < CAPTION_MAX_ROWS; i++)
		if (idata->caption_row[i] != NULL)
			g_free (idata->caption_row[i]);

	g_free (idata);
}


enum {
	PNG_EXPORTER_DONE,
	PNG_EXPORTER_PROGRESS,
	PNG_EXPORTER_INFO,
	LAST_SIGNAL
};

static GObjectClass *parent_class = NULL;
static guint catalog_png_exporter_signals[LAST_SIGNAL] = { 0 };


static void
catalog_png_exporter_finalize (GObject *object)
{
	CatalogPngExporter *ce;

        g_return_if_fail (IS_CATALOG_PNG_EXPORTER (object));

        ce = CATALOG_PNG_EXPORTER (object);

	if (ce->location != NULL) {
		g_free (ce->location);
		ce->location = NULL;
	}

	if (ce->info != NULL) {
		g_free (ce->info);
		ce->info = NULL;
	}

	if (ce->name_template != NULL) {
		g_free (ce->name_template);
		ce->name_template = NULL;
	}

	if (ce->templatev != NULL) {
		g_strfreev (ce->templatev);
		ce->templatev = NULL;
	}

	if (ce->file_type != NULL) {
		g_free (ce->file_type);
		ce->file_type = NULL;
	}

	if (ce->file_list != NULL) {
		g_list_foreach (ce->file_list, (GFunc) image_data_free, NULL);
		g_list_free (ce->file_list);
		ce->file_list = NULL;
	}

	if (ce->created_files != NULL) {
		path_list_free (ce->created_files);
		ce->created_files = NULL;
	}

	if (ce->pages_height != NULL) {
		g_free (ce->pages_height);
		ce->pages_height = NULL;
	}

	if (ce->layout != NULL) {
		g_object_unref (ce->layout);
		ce->layout = NULL;
	}

	if (ce->context != NULL) {
		g_object_unref (ce->context);
		ce->context = NULL;
	}

	if (ce->caption_font_name != NULL) {
		g_free (ce->caption_font_name);
		ce->caption_font_name = NULL;
	}

	if (ce->header != NULL) {
		g_free (ce->header);
		ce->header = NULL;
	}

	if (ce->header_font_name != NULL) {
		g_free (ce->header_font_name);
		ce->header_font_name = NULL;
	}

	if (ce->footer != NULL) {
		g_free (ce->footer);
		ce->footer = NULL;
	}

	if (ce->footer_font_name != NULL) {
		g_free (ce->footer_font_name);
		ce->footer_font_name = NULL;
	}

	if (ce->iloader != NULL) {
		g_object_unref (ce->iloader);
		ce->iloader = NULL;
	}

	if (ce->imap_uri != NULL) {
		g_free (ce->imap_uri);
		ce->imap_uri = NULL;
	}

	/* Chain up */

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
catalog_png_exporter_class_init (CatalogPngExporterClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	parent_class = g_type_class_peek_parent (class);

	catalog_png_exporter_signals[PNG_EXPORTER_DONE] =
		g_signal_new ("png_exporter_done",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (CatalogPngExporterClass, png_exporter_done),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);

	catalog_png_exporter_signals[PNG_EXPORTER_PROGRESS] =
		g_signal_new ("png_exporter_progress",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (CatalogPngExporterClass, png_exporter_progress),
			      NULL, NULL,
			      gthumb_marshal_VOID__FLOAT,
			      G_TYPE_NONE,
			      1, G_TYPE_FLOAT);

	catalog_png_exporter_signals[PNG_EXPORTER_INFO] =
		g_signal_new ("png_exporter_info",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (CatalogPngExporterClass, png_exporter_info),
			      NULL, NULL,
			      gthumb_marshal_VOID__STRING,
			      G_TYPE_NONE,
			      1, G_TYPE_STRING);

	object_class->finalize = catalog_png_exporter_finalize;
}


static void
catalog_png_exporter_init (CatalogPngExporter *ce)
{
	PangoFontDescription *font_desc;

	ce->file_list = NULL;
	ce->created_files = NULL;

	ce->page_width = 0;
	ce->page_height = 0;
	ce->all_pages_same_size = FALSE;

	ce->thumb_width = 0;
	ce->thumb_height = 0;

	ce->caption_font_name = NULL;
	ce->header = NULL;
	ce->header_font_name = NULL;
	ce->footer = NULL;
	ce->footer_font_name = NULL;

	ce->iloader = NULL;
	ce->file_to_load = NULL;

	ce->caption_fields = GTH_CAPTION_FILE_NAME;

	/* Colors. */

	ce->page_use_solid_color = TRUE;
	ce->page_use_hgradient = FALSE;
	ce->page_use_vgradient = FALSE;

	ce->page_bg_color = 0xFFFFFFFF;
	ce->page_hgrad1   = 0xFFFFFFFF;
	ce->page_hgrad2   = 0xFFFFFFFF;
	ce->page_vgrad1   = 0xFFFFFFFF;
	ce->page_vgrad2   = 0xFFFFFFFF;

	ce->caption_color.red   = 65535 * 0.0;
	ce->caption_color.green = 65535 * 0.0;
	ce->caption_color.blue  = 65535 * 0.0;

	ce->frame_color.red   = 65535 * 0.8;
	ce->frame_color.green = 65535 * 0.8;
	ce->frame_color.blue  = 65535 * 0.8;

	ce->header_color.red   = 65535 * 0.0;
	ce->header_color.green = 65535 * 0.0;
	ce->header_color.blue  = 65535 * 0.0;

	ce->footer_color.red   = 65535 * 0.0;
	ce->footer_color.green = 65535 * 0.0;
	ce->footer_color.blue  = 65535 * 0.0;

	ce->frame_style = GTH_FRAME_STYLE_SIMPLE;

	ce->location = NULL;
	ce->name_template = NULL;
	ce->templatev = NULL;
	ce->file_type = g_strdup ("png");
	ce->start_at = 1;

	ce->pages_height = NULL;

	ce->write_image_map = FALSE;
	ce->imap_handle = NULL;
	ce->imap_uri = NULL;

	ce->info = NULL;

	/* Context */

	ce->context = gdk_pango_context_get ();
	pango_context_set_language (ce->context, gtk_get_default_language ());

	/* Layout */

	ce->layout = pango_layout_new (ce->context);
	pango_layout_set_alignment (ce->layout, PANGO_ALIGN_CENTER);
	pango_layout_set_wrap (ce->layout, PANGO_WRAP_WORD_CHAR);

	font_desc = pango_font_description_from_string (DEFAULT_FONT);
	pango_layout_set_font_description (ce->layout, font_desc);
	pango_font_description_free (font_desc);

	/**/

	ce->exporting = FALSE;
	ce->interrupted = FALSE;
}


GType
catalog_png_exporter_get_type ()
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (CatalogPngExporterClass),
                        NULL,
                        NULL,
                        (GClassInitFunc) catalog_png_exporter_class_init,
                        NULL,
                        NULL,
                        sizeof (CatalogPngExporter),
                        0,
                        (GInstanceInitFunc) catalog_png_exporter_init
                };

                type = g_type_register_static (G_TYPE_OBJECT,
                                               "CatalogPngExporter",
                                               &type_info,
                                               0);
        }

        return type;
}


CatalogPngExporter *
catalog_png_exporter_new (GList *file_list)
{
	CatalogPngExporter *ce;
	GList              *scan;

	ce = CATALOG_PNG_EXPORTER (g_object_new (CATALOG_PNG_EXPORTER_TYPE, NULL));

	for (scan = file_list; scan; scan = scan->next) {
		ImageData *idata;

		idata = image_data_new ((FileData*) scan->data);
		ce->file_list = g_list_prepend (ce->file_list, idata);
	}
	ce->file_list = g_list_reverse (ce->file_list);

	return ce;
}


void
catalog_png_exporter_set_location (CatalogPngExporter *ce,
				   const char         *location)
{
	g_return_if_fail (IS_CATALOG_PNG_EXPORTER (ce));

	g_free (ce->location);
	ce->location = g_strdup (location);
}


void
catalog_png_exporter_set_name_template (CatalogPngExporter *ce,
					const char         *template)
{
	g_return_if_fail (IS_CATALOG_PNG_EXPORTER (ce));

	if (ce->name_template)
		g_free (ce->name_template);
	ce->name_template = g_strdup (template);

	if (ce->templatev)
		g_strfreev (ce->templatev);
	ce->templatev = _g_get_template_from_text (ce->name_template);
}


void
catalog_png_exporter_set_start_at (CatalogPngExporter *ce,
				   int                 n)
{
	g_return_if_fail (IS_CATALOG_PNG_EXPORTER (ce));
	ce->start_at = n;
}


void
catalog_png_exporter_set_file_type (CatalogPngExporter *ce,
				    const char         *file_type)
{
	g_return_if_fail (IS_CATALOG_PNG_EXPORTER (ce));
	g_return_if_fail (file_type != NULL);

	if (ce->file_type)
		g_free (ce->file_type);
	ce->file_type = g_strdup (file_type);
}


void
catalog_png_exporter_set_page_size (CatalogPngExporter *ce,
				    int                 width,
				    int                 height)
{
	g_return_if_fail (IS_CATALOG_PNG_EXPORTER (ce));

	ce->page_width = width;
	ce->page_height = height;
}


void
catalog_png_exporter_set_page_size_row_col (CatalogPngExporter *ce,
					    int                 rows,
					    int                 cols)
{
	g_return_if_fail (IS_CATALOG_PNG_EXPORTER (ce));

	ce->page_size_use_row_col = TRUE;
	ce->page_rows = rows;
	ce->page_cols = cols;
}


void
catalog_png_exporter_all_pages_same_size (CatalogPngExporter *ce,
					  gboolean same_size)
{
	g_return_if_fail (IS_CATALOG_PNG_EXPORTER (ce));
	ce->all_pages_same_size = same_size;
}


void
catalog_png_exporter_set_thumb_size (CatalogPngExporter *ce,
				     int                 width,
				     int                 height)

{
	g_return_if_fail (IS_CATALOG_PNG_EXPORTER (ce));

	ce->thumb_width = width;
	ce->thumb_height = height;

	ce->frame_width = width + THUMB_FRAME_BORDER * 2;
	ce->frame_height = height + THUMB_FRAME_BORDER * 2;
}


void
catalog_png_exporter_set_caption (CatalogPngExporter *ce,
				  GthCaptionFields caption)
{
	g_return_if_fail (IS_CATALOG_PNG_EXPORTER (ce));
	ce->caption_fields = caption;
}


void
catalog_png_exporter_set_caption_font (CatalogPngExporter *ce,
				       char               *font_name)
{
	g_return_if_fail (IS_CATALOG_PNG_EXPORTER (ce));

	if (ce->caption_font_name != NULL) {
		g_free (ce->caption_font_name);
		ce->caption_font_name = NULL;
	}
	if (font_name != NULL)
		ce->caption_font_name = g_strdup (font_name);
}


void
catalog_png_exporter_set_caption_color (CatalogPngExporter *ce,
					char               *value)
{
	g_return_if_fail (IS_CATALOG_PNG_EXPORTER (ce));

	pref_util_get_rgb_values (value,
				  &ce->caption_color.red,
				  &ce->caption_color.green,
				  &ce->caption_color.blue);
}


void
catalog_png_exporter_set_header (CatalogPngExporter *ce,
				 char               *header)
{
	g_return_if_fail (IS_CATALOG_PNG_EXPORTER (ce));

	if (ce->header != NULL) {
		g_free (ce->header);
		ce->header = NULL;
	}

	if (header != NULL)
		ce->header = g_strdup (header);
}


void
catalog_png_exporter_set_header_font (CatalogPngExporter *ce,
				      char               *font)
{
	g_return_if_fail (IS_CATALOG_PNG_EXPORTER (ce));
	g_return_if_fail (font != NULL);

	if (ce->header_font_name != NULL)
		g_free (ce->header_font_name);
	ce->header_font_name = g_strdup (font);
}


void
catalog_png_exporter_set_header_color (CatalogPngExporter *ce,
				       char               *value)
{
	g_return_if_fail (IS_CATALOG_PNG_EXPORTER (ce));

	pref_util_get_rgb_values (value,
				  &ce->header_color.red,
				  &ce->header_color.green,
				  &ce->header_color.blue);
}


void
catalog_png_exporter_set_footer (CatalogPngExporter *ce,
				 char               *footer)
{
	g_return_if_fail (IS_CATALOG_PNG_EXPORTER (ce));

	if (ce->footer != NULL) {
		g_free (ce->footer);
		ce->footer = NULL;
	}

	if (footer != NULL)
		ce->footer = g_strdup (footer);
}


void
catalog_png_exporter_set_footer_font (CatalogPngExporter *ce,
				      char               *font)
{
	g_return_if_fail (IS_CATALOG_PNG_EXPORTER (ce));
	g_return_if_fail (font != NULL);

	if (ce->footer_font_name != NULL)
		g_free (ce->footer_font_name);
	ce->footer_font_name = g_strdup (font);
}


void
catalog_png_exporter_set_footer_color (CatalogPngExporter *ce,
				       char               *value)
{
	g_return_if_fail (IS_CATALOG_PNG_EXPORTER (ce));

	pref_util_get_rgb_values (value,
				  &ce->footer_color.red,
				  &ce->footer_color.green,
				  &ce->footer_color.blue);
}


void
catalog_png_exporter_set_page_color (CatalogPngExporter *ce,
				     gboolean  use_solid_col,
				     gboolean  use_hgrad,
				     gboolean  use_vgrad,
				     guint32   bg_color,
				     guint32   hgrad1,
				     guint32   hgrad2,
				     guint32   vgrad1,
				     guint32   vgrad2)
{
	g_return_if_fail (IS_CATALOG_PNG_EXPORTER (ce));

	ce->page_use_solid_color = use_solid_col;
	ce->page_use_hgradient = use_hgrad;
	ce->page_use_vgradient = use_vgrad;
	ce->page_bg_color = bg_color;
	ce->page_hgrad1 = hgrad1;
	ce->page_hgrad2 = hgrad2;
	ce->page_vgrad1 = vgrad1;
	ce->page_vgrad2 = vgrad2;
}


void
catalog_png_exporter_set_frame_color (CatalogPngExporter *ce,
				      char               *value)
{
	g_return_if_fail (IS_CATALOG_PNG_EXPORTER (ce));

	pref_util_get_rgb_values (value,
				  &ce->frame_color.red,
				  &ce->frame_color.green,
				  &ce->frame_color.blue);
}


void
catalog_png_exporter_set_frame_style (CatalogPngExporter *ce,
				      GthFrameStyle style)
{
	g_return_if_fail (IS_CATALOG_PNG_EXPORTER (ce));
	ce->frame_style = style;
}


void
catalog_png_exporter_write_image_map (CatalogPngExporter *ce,
				      gboolean do_write)
{
	g_return_if_fail (IS_CATALOG_PNG_EXPORTER (ce));
	ce->write_image_map = do_write;
}


void
catalog_png_exporter_set_sort_method (CatalogPngExporter *ce,
				      GthSortMethod method)
{
	g_return_if_fail (IS_CATALOG_PNG_EXPORTER (ce));
	ce->sort_method = method;
}


void
catalog_png_exporter_set_sort_type (CatalogPngExporter *ce,
				    GtkSortType sort_type)
{
	g_return_if_fail (IS_CATALOG_PNG_EXPORTER (ce));
	ce->sort_type = sort_type;
}


static int
get_max_text_height (CatalogPngExporter *ce,
		     GList              *first_item,
		     GList              *last_item)
{
	int     max_height = 0;
	GList  *scan;

	for (scan = first_item; scan != last_item; scan = scan->next) {
		char  *font      = ce->caption_font_name;
		char **row       = IMAGE_DATA (scan->data)->caption_row;
		int    not_empty = 0;
		int    tmp;

		tmp = 0;
		tmp += row[0] ? not_empty++, get_text_height (ce, row[0], font, ce->thumb_width) : 0;
		tmp += row[1] ? not_empty++, get_text_height (ce, row[1], font, ce->thumb_width) : 0;
		tmp += row[2] ? not_empty++, get_text_height (ce, row[2], font, ce->thumb_width) : 0;
		tmp += row[3] ? not_empty++, get_text_height (ce, row[3], font, ce->thumb_width) : 0;
		tmp += not_empty * TEXT_SPACING;

		max_height = MAX (max_height, tmp);
	}

	return max_height;
}


static void
set_item_caption (CatalogPngExporter *ce,
		  ImageData          *idata)
{
	int row = 0;

	if (idata->caption_set)
		return;

	if ((ce->caption_fields & GTH_CAPTION_COMMENT)
	    && (idata->comment != NULL))
		idata->caption_row[row++] = g_strdup (idata->comment);

	if ((ce->caption_fields & GTH_CAPTION_FILE_PATH)
	    && (ce->caption_fields & GTH_CAPTION_FILE_NAME)) {
		char *utf8_name = get_utf8_display_name_from_uri (idata->file->path);
		idata->caption_row[row++] = utf8_name;
	} 
	else {
		if (ce->caption_fields & GTH_CAPTION_FILE_PATH) {
			char *path = remove_level_from_path (idata->file->path);
			char *utf8_name = get_utf8_display_name_from_uri (path);
			idata->caption_row[row++] = utf8_name;
			g_free (path);
		} 
		else if (ce->caption_fields & GTH_CAPTION_FILE_NAME) {
			const char *name = file_name_from_path (idata->file->path);
			char *utf8_name = get_utf8_display_name_from_uri (name);
			idata->caption_row[row++] = utf8_name;
		}
	}

	if (ce->caption_fields & GTH_CAPTION_FILE_SIZE)
		idata->caption_row[row++] = g_format_size_for_display (idata->file->size);

	if (ce->caption_fields & GTH_CAPTION_IMAGE_DIM)
		idata->caption_row[row++] = g_strdup_printf (
			_("%d x %d pixels"),
			idata->image_width,
			idata->image_height);

	idata->caption_set = TRUE;
}


static gint
get_page_height (CatalogPngExporter *ce,
		 int                 page_n)
{
	int page_height = 0;

	if (ce->page_size_use_row_col && ! ce->all_pages_same_size)
		page_height = ce->pages_height[page_n - 1];
	else
		page_height = ce->page_height;

	return page_height;

}


static void copy_current_file_to_destination (CatalogPngExporter *ce);


static void
copy_current_file_to_destination_done (const char     *uri, 
				       GnomeVFSResult  result, 
				       gpointer        data)
{
	CatalogPngExporter *ce = data;
	
	ce->current_file = ce->current_file->next;
	copy_current_file_to_destination (ce);
}


static void
copy_current_file_to_destination (CatalogPngExporter *ce)
{
	FileData *file;
	
	if (ce->current_file == NULL) {
		end_export (ce);
		return;
	}
	
	g_signal_emit (G_OBJECT (ce),
		       catalog_png_exporter_signals[PNG_EXPORTER_PROGRESS],
		       0,
		       ((float) ++ce->n_files_done) / (ce->n_files + 1));	

	file = file_data_new ((char*) ce->current_file->data, NULL);
	update_file_from_cache (file, copy_current_file_to_destination_done, ce);
	
	file_data_unref (file);
}


static void
copy_created_files_to_destination (CatalogPngExporter *ce)
{
	g_free (ce->info);
	ce->info = g_strdup (_("Saving images"));
	g_signal_emit (G_OBJECT (ce), catalog_png_exporter_signals[PNG_EXPORTER_INFO],
		       0,
		       ce->info);
		       	
	ce->n_files_done = 0;
	ce->n_files = g_list_length (ce->created_files);
	ce->current_file = ce->created_files;
	
	copy_current_file_to_destination (ce);
}


static void
export (CatalogPngExporter *ce)
{
	int        cols;
	int        i;
	int        x, y;
	GList     *scan_file, *row_first_item, *row_last_item, *scan_row;
	int        page_n = 0;
	gboolean   first_row;
	ImageData *idata;
	int        page_height;
	int        header_height;
	int        footer_height;

	cols = ((ce->page_width - COL_SPACING)
		/ (ce->frame_width + COL_SPACING));

	begin_export (ce);

	first_row = TRUE;
	begin_page (ce, ++page_n);
	page_height = get_page_height (ce, page_n);
	header_height = get_header_height_with_spacing (ce);
	footer_height = get_footer_height_with_spacing (ce);
	y = ROW_SPACING;

	scan_file = ce->file_list;
	idata = (ImageData*) scan_file->data;
	do {
		int row_height;

		if (ce->interrupted) {
			if (ce->file_list != NULL) {
				g_list_foreach (ce->file_list, (GFunc) image_data_free, NULL);
				g_list_free (ce->file_list);
				ce->file_list = NULL;
			}
			goto label_end;
		}

		/* get items to paint. */

		row_first_item = scan_file;
		row_last_item = NULL;
		for (i = 0; i < cols; i++) {
			if (scan_file == NULL) {
				cols = i;
				break;
			}

			set_item_caption (ce, idata);

			row_last_item = scan_file = scan_file->next;
			if (scan_file != NULL)
				idata = (ImageData*) scan_file->data;
		}

		if (cols == 0) {
			paint_footer (ce, page_n);
			end_page (ce, page_n);
			goto label_end;
		}

		/* check whether the row fit the current page. */

		row_height = (ce->frame_height
			      + get_max_text_height (ce,
						     row_first_item,
						     row_last_item)
			      + ROW_SPACING);

		while (y + row_height > (ce->page_height
					 - (first_row ? header_height : 0)
					 - footer_height)) {
			if (first_row == TRUE) {
				/* this row has an height greater than the
				 * page height, close and exit. */
				goto label_end;
			}

			/* the row does not fit this page, create a new
			 * page. */

			if (page_n > 0) {
				paint_footer (ce, page_n);
				end_page (ce, page_n);
			}

			first_row = TRUE;
			begin_page (ce, ++page_n);
			page_height = get_page_height (ce, page_n);
			header_height = get_header_height_with_spacing (ce);
			footer_height = get_footer_height_with_spacing (ce);
			y = ROW_SPACING;
		}

		/* paint the header. */

		if (first_row && (ce->header != NULL)) {
			char *text;

			text = get_header_text (ce, page_n);
			paint_text (ce,
				    ce->header_font_name,
				    &ce->header_color,
				    0,
				    y,
				    ce->page_width - COL_SPACING,
				    text,
				    NULL);
			g_free (text);

			y += header_height;
		}

		/* paint the row. */

		x = COL_SPACING;
		for (scan_row = row_first_item; scan_row != row_last_item; scan_row = scan_row->next) {
			GdkPixbuf     *pixbuf;
			char         **row;
			int            y1, h, i;
			GdkRectangle   frame_rect, image_rect;
			ImageData     *row_idata = IMAGE_DATA (scan_row->data);

			row = row_idata->caption_row;

			frame_rect.x = x;
			frame_rect.y = y;
			frame_rect.width = ce->frame_width;
			frame_rect.height = ce->frame_height;

			pixbuf = row_idata->thumb;
			if (pixbuf != NULL) {
				int twidth, theight;

				twidth = gdk_pixbuf_get_width (pixbuf);
				theight = gdk_pixbuf_get_height (pixbuf);

				image_rect.x = x + (ce->frame_width - twidth) / 2 + 1;
				image_rect.y = y + (ce->frame_height - theight) / 2 + 1;
				image_rect.width = twidth;
				image_rect.height = theight;

				paint_frame (ce, &frame_rect, &image_rect,
					     row_idata->file->path);
				paint_image (ce, &image_rect, pixbuf);
			}

			y1 = y + ce->frame_height + TEXT_SPACING;

			for (i = 0; i < 4; i++) {
				if (row[i] == NULL)
					continue;
				if ((i == 0)
				    && (ce->caption_fields & GTH_CAPTION_COMMENT)
				    && (row_idata->comment != NULL))
					paint_comment (ce, x, y1, row[i], &h);
				else
					paint_text (ce,
						    ce->caption_font_name,
						    &ce->caption_color,
						    x, y1,
						    ce->thumb_width,
						    row[i],
						    &h);

				y1 += h + TEXT_SPACING;
			}

			x += ce->frame_width + COL_SPACING;
		}
		y += row_height;
		first_row = FALSE;
	} while (TRUE);

 label_end:
	copy_created_files_to_destination (ce);
}


static void
compute_pages_n (CatalogPngExporter *ce)
{
	int        cols, i, y;
	GList     *scan_file, *row_first_item, *row_last_item;
	gboolean   first_row;
	ImageData *idata;
	int        page_height;
	int        header_height;
	int        footer_height;

	ce->pages_n = 0;

	cols = ((ce->page_width - COL_SPACING)
		/ (ce->frame_width + COL_SPACING));

	first_row = TRUE;
	page_height = get_page_height (ce, ce->pages_n);
	header_height = get_header_height_with_spacing (ce);
	footer_height = get_footer_height_with_spacing (ce);
	y = ROW_SPACING;

	scan_file = ce->file_list;
	idata = (ImageData*) scan_file->data;
	do {
		int row_height;

		/* get items to paint. */

		row_first_item = scan_file;
		row_last_item = NULL;
		for (i = 0; i < cols; i++) {
			if (scan_file == NULL) {
				cols = i;
				break;
			}

			set_item_caption (ce, idata);

			row_last_item = scan_file = scan_file->next;
			if (scan_file != NULL)
				idata = (ImageData*) scan_file->data;
		}

		if (cols == 0)
			break;

		/* check whether the row fit the current page. */

		row_height = (ce->frame_height
			      + get_max_text_height (ce,
						     row_first_item,
						     row_last_item)
			      + ROW_SPACING);

		while (y + row_height > (ce->page_height
					 - (first_row ? header_height : 0)
					 - footer_height)) {
			if (first_row == TRUE) {
				/* this row has an height greater than the
				 * page height, close and exit. */
				ce->pages_n = 0;
				return;
			}

			/* the row does not fit this page, create a new
			 * page. */

			ce->pages_n++;

			first_row = TRUE;
			page_height = get_page_height (ce, ce->pages_n);
			header_height = get_header_height_with_spacing (ce);
			footer_height = get_footer_height_with_spacing (ce);
			y = ROW_SPACING;
		}

		/* paint the header. */

		if (first_row && (ce->header != NULL))
			y += header_height;

		/* paint the row. */

		y += row_height;
		first_row = FALSE;

	} while (TRUE);

	ce->pages_n++;
}


static void
compute_pages_size (CatalogPngExporter *ce)
{
	GList     *scan_file, *row_first_item, *row_last_item;
	int        cols, rows;
	int        c, r;
	int        pages;
	ImageData *idata;

	ce->page_width = COL_SPACING + ce->page_cols * (ce->frame_width + COL_SPACING);
	ce->page_height = 0;

	cols = ce->page_cols;
	rows = ce->page_rows;
	pages = ce->n_files / (cols * rows) + 2;
	ce->pages_height = g_new (int, pages);
	ce->pages_n = 0;

	scan_file = ce->file_list;
	idata = (ImageData*) scan_file->data;
	do {
		int row_height;
		int page_height;

		page_height = ROW_SPACING;

		/* header */

		page_height += get_header_height_with_spacing (ce);

		/* images */

		for (r = 0; r < rows; r++) {
			/* get row items. */

			row_first_item = scan_file;
			row_last_item = NULL;
			for (c = 0; c < cols; c++) {
				if (scan_file == NULL) {
					cols = c;
					break;
				}

				set_item_caption (ce, idata);

				row_last_item = scan_file = scan_file->next;
				if (scan_file != NULL)
					idata = (ImageData*) scan_file->data;
			}

			if (cols == 0)
				break;

			row_height = (ce->frame_height
				      + get_max_text_height (ce,
							     row_first_item,
							     row_last_item)
				      + ROW_SPACING);

			page_height += row_height;
		}

		/* footer */

		page_height += get_footer_height_with_spacing (ce);

		/**/

		ce->pages_height[ce->pages_n] = page_height;
		ce->page_height = MAX (ce->page_height, page_height);
		ce->pages_n++;
	} while (scan_file != NULL);
}


static gint
comp_func_name (gconstpointer a, gconstpointer b)
{
	ImageData *data_a, *data_b;

	data_a = IMAGE_DATA (a);
	data_b = IMAGE_DATA (b);

	return gth_sort_by_filename_but_ignore_path (data_a->file->path, data_b->file->path);
}


static gint
comp_func_path (gconstpointer a, gconstpointer b)
{
	ImageData *data_a, *data_b;

	data_a = IMAGE_DATA (a);
	data_b = IMAGE_DATA (b);

	return gth_sort_by_full_path (data_a->file->path, data_b->file->path);
}


static int
comp_func_comment (gconstpointer a, gconstpointer b)
{
	ImageData *data_a, *data_b;

	data_a = IMAGE_DATA (a);
	data_b = IMAGE_DATA (b);

	return gth_sort_by_comment_then_name (data_a->comment, data_b->comment,
					data_a->file->path, data_b->file->path);
}


static gint
comp_func_time (gconstpointer a, gconstpointer b)
{
	ImageData *data_a, *data_b;

	data_a = IMAGE_DATA (a);
	data_b = IMAGE_DATA (b);

	return gth_sort_by_filetime_then_name (data_a->file->mtime, data_b->file->mtime,
						data_a->file->path, data_b->file->path);
}


static gint
comp_func_exif_date (gconstpointer a, gconstpointer b)
{
	ImageData *data_a, *data_b;

	data_a = IMAGE_DATA (a);
	data_b = IMAGE_DATA (b);

	return gth_sort_by_exiftime_then_name (data_a->file, data_b->file);
}


static gint
comp_func_size (gconstpointer a, gconstpointer b)
{
	ImageData *data_a, *data_b;

	data_a = IMAGE_DATA (a);
	data_b = IMAGE_DATA (b);

	return gth_sort_by_size_then_name (data_a->file->size, data_b->file->size, data_a->file->path, data_b->file->path);
}


static GCompareFunc
get_sortfunc (CatalogPngExporter *ce)
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
	case GTH_SORT_METHOD_BY_EXIF_DATE:
		func = comp_func_exif_date;
		break;
	case GTH_SORT_METHOD_NONE:
		func = gth_sort_none;
		break;
	default:
		func = gth_sort_none;
		break;
	}

	return func;
}


static void
load_next_file (CatalogPngExporter *ce)
{
	ImageData *idata;

	if (ce->interrupted) {
		if (ce->file_list != NULL) {
			g_list_foreach (ce->file_list, (GFunc) image_data_free, NULL);
			g_list_free (ce->file_list);
			ce->file_list = NULL;
		}
		g_signal_emit (G_OBJECT (ce), catalog_png_exporter_signals[PNG_EXPORTER_DONE], 0);
		return;
	}

	g_signal_emit (G_OBJECT (ce),
		       catalog_png_exporter_signals[PNG_EXPORTER_PROGRESS],
		       0,
		       ((float) ++ce->n_files_done) / (ce->n_files + 1));

	ce->file_to_load = ce->file_to_load->next;
	if (ce->file_to_load == NULL) {
		/* sort list */
		ce->file_list = g_list_sort (ce->file_list, get_sortfunc (ce));
		if (ce->sort_type == GTK_SORT_DESCENDING)
			ce->file_list = g_list_reverse (ce->file_list);

		/* compute the page size if needed. */
		if (ce->page_size_use_row_col)
			compute_pages_size (ce);
		else
			compute_pages_n (ce);

		export (ce);

		return;
	}

	/* load the file */

	idata = IMAGE_DATA (ce->file_to_load->data);

	g_free (ce->info);
	ce->info = g_strdup_printf (_("Loading image: %s"), idata->file->display_name);
	g_signal_emit (G_OBJECT (ce), catalog_png_exporter_signals[PNG_EXPORTER_INFO],
		       0,
		       ce->info);

	image_loader_set_file (ce->iloader, idata->file);
	image_loader_start (ce->iloader);
}


static void
image_loader_done (ImageLoader *iloader,
		   gpointer     data)
{
	CatalogPngExporter *ce = data;
	GdkPixbuf          *pixbuf;
	ImageData          *idata;

	idata = (ImageData*) ce->file_to_load->data;

	/* image width and height. */
	pixbuf = image_loader_get_pixbuf (iloader);
	idata->image_width = gdk_pixbuf_get_width (pixbuf);
	idata->image_height = gdk_pixbuf_get_height (pixbuf);

	/* thumbnail. */
	idata->thumb = pixbuf = image_loader_get_pixbuf (iloader);
	g_object_ref (idata->thumb);

	if ((ce->thumb_width > 0) && (ce->thumb_height > 0)) {
		int w = gdk_pixbuf_get_width (pixbuf);
		int h = gdk_pixbuf_get_height (pixbuf);

		if (scale_keeping_ratio (&w, &h,
					  ce->thumb_width,
					  ce->thumb_height,
					  FALSE)) {
			GdkPixbuf *scaled;
			scaled = gdk_pixbuf_scale_simple (pixbuf, w, h, GDK_INTERP_BILINEAR);
			g_object_unref (idata->thumb);
			idata->thumb = scaled;
		}
	}

	load_next_file (ce);
}


static void
image_loader_error (ImageLoader *iloader,
		    gpointer     data)
{
	CatalogPngExporter *ce = data;
	load_next_file (ce);
}


void
catalog_png_exporter_export (CatalogPngExporter *ce)
{
	g_return_if_fail (IS_CATALOG_PNG_EXPORTER (ce));
	g_return_if_fail (ce->page_size_use_row_col || ce->page_width != 0);
	g_return_if_fail (ce->page_size_use_row_col || ce->page_height != 0);
	g_return_if_fail (ce->thumb_width != 0);
	g_return_if_fail (ce->thumb_height != 0);

	if (ce->exporting)
		return;

	if (ce->file_list == NULL)
		return;

	ce->exporting = TRUE;

	if (ce->iloader != NULL)
		g_object_unref (ce->iloader);

	if (ce->created_files != NULL) {
		path_list_free (ce->created_files);
		ce->created_files = NULL;
	}

	ce->iloader = IMAGE_LOADER (image_loader_new (FALSE));
	g_signal_connect (G_OBJECT (ce->iloader),
			  "image_done",
			  G_CALLBACK (image_loader_done),
			  ce);
	g_signal_connect (G_OBJECT (ce->iloader),
			  "image_error",
			  G_CALLBACK (image_loader_error),
			  ce);

	ce->n_files = g_list_length (ce->file_list);
	ce->n_files_done = 0;

	ce->file_to_load = ce->file_list;
	image_loader_set_file (ce->iloader,
			       IMAGE_DATA (ce->file_to_load->data)->file);
	image_loader_start (ce->iloader);
}


void
catalog_png_exporter_interrupt (CatalogPngExporter *ce)
{
	g_return_if_fail (IS_CATALOG_PNG_EXPORTER (ce));

	if (! ce->exporting)
		return;
	ce->interrupted = TRUE;
}


/* ----- */


static void
begin_export (CatalogPngExporter *ce)
{
	ce->pixmap = gdk_pixmap_new (gdk_get_default_root_window (),
				     ce->page_width, ce->page_height,
				     -1);
	ce->gc = gdk_gc_new (ce->pixmap);

	gdk_color_parse ("#777777", &ce->black);
	gdk_color_parse ("#AAAAAA", &ce->dark_gray);
	gdk_color_parse ("#CCCCCC", &ce->gray);
	gdk_color_parse ("#FFFFFF", &ce->white);

	gdk_pango_context_set_colormap (ce->context, ce->gc->colormap);
}


static void
end_export (CatalogPngExporter *ce)
{	
	if (ce->created_files != NULL) {
		all_windows_remove_monitor ();
		all_windows_notify_files_created (ce->created_files);
		all_windows_add_monitor ();
		path_list_free (ce->created_files);
		ce->created_files = NULL;
	}

	g_object_unref (G_OBJECT (ce->gc));
	g_object_unref (G_OBJECT (ce->pixmap));

	g_signal_emit (G_OBJECT (ce), catalog_png_exporter_signals[PNG_EXPORTER_DONE], 0);
}


static void
paint_background (CatalogPngExporter *ce,
		  int                 width,
		  int                 height)
{
	GdkPixbuf  *pixbuf;

	pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, width, height);

	if (ce->page_use_solid_color)
		gdk_pixbuf_fill (pixbuf, ce->page_bg_color);
	else {
		GdkPixbuf *tmp;

		tmp = gdk_pixbuf_new (GDK_COLORSPACE_RGB, 1, 8, width, height);
		gdk_pixbuf_fill (tmp, 0xFFFFFFFF);

		if (ce->page_use_hgradient && ce->page_use_vgradient)
			_gdk_pixbuf_hv_gradient (tmp, ce->page_hgrad1, ce->page_hgrad2, ce->page_vgrad1, ce->page_vgrad2);

		else if (ce->page_use_hgradient)
			_gdk_pixbuf_horizontal_gradient (tmp, ce->page_hgrad1, ce->page_hgrad2);

		else if (ce->page_use_vgradient)
			_gdk_pixbuf_vertical_gradient (tmp, ce->page_vgrad1, ce->page_vgrad2);

		gdk_pixbuf_composite (tmp, pixbuf,
				      0, 0, width, height,
				      0, 0,
				      1.0, 1.0,
				      GDK_INTERP_NEAREST, 255);

		g_object_unref (tmp);
	}

	gdk_draw_rgb_32_image_dithalign (ce->pixmap,
					 ce->gc,
					 0, 0, width, height,
					 GDK_RGB_DITHER_MAX,
                                         gdk_pixbuf_get_pixels (pixbuf),
					 gdk_pixbuf_get_rowstride (pixbuf),
					 0, 0);
	g_object_unref (pixbuf);
}


static void
begin_page (CatalogPngExporter *ce,
	    int                 page_n)
{
	GnomeVFSURI      *vfs_uri;
	GnomeVFSResult    result;
	GnomeVFSFileSize  temp;
	int               width, height;
	char             *local_file;
	char             *filename;
	char             *name;
	char             *line;
	char             *utf8_name;

	g_signal_emit (G_OBJECT (ce),
		       catalog_png_exporter_signals[PNG_EXPORTER_PROGRESS],
		       0,
		       ((float) page_n) / ce->pages_n);

	width = ce->page_width;
	height = get_page_height (ce, page_n);

	paint_background (ce, width, height);

	/* info */

	g_free (ce->info);

	filename = _g_get_name_from_template (ce->templatev, ce->start_at + page_n - 1);
	utf8_name = get_utf8_display_name_from_uri (filename);
	ce->info = g_strdup_printf (_("Creating image: %s.%s"),
				    utf8_name,
				    ce->file_type);
	g_free (utf8_name);
	g_free (filename);

	g_signal_emit (G_OBJECT (ce), catalog_png_exporter_signals[PNG_EXPORTER_INFO],
		       0,
		       ce->info);

	/* image map file. */

	if (! ce->write_image_map)
		return;

	g_free (ce->imap_uri);
	ce->imap_uri = NULL;
	ce->imap_handle = NULL;
	
	name = _g_get_name_from_template (ce->templatev, ce->start_at + page_n - 1);
	ce->imap_uri = g_strconcat (ce->location, "/", name, ".html", NULL);

	local_file = get_cache_uri_from_uri (ce->imap_uri);
	vfs_uri = new_uri_from_path (local_file);
	g_free (local_file);
	
	if (vfs_uri == NULL) {
		g_warning ("URI not valid: %s", local_file);
		return;
	}

	result = gnome_vfs_create_uri (&ce->imap_handle, vfs_uri,
				       GNOME_VFS_OPEN_WRITE, FALSE, 0664);
	gnome_vfs_uri_unref (vfs_uri);
	if (result != GNOME_VFS_OK) {
		g_warning ("Cannot create file %s", ce->imap_uri);
		return;
	}

	gnome_vfs_write (ce->imap_handle, HTML_PAGE_BEGIN, strlen (HTML_PAGE_BEGIN), &temp);

	filename = g_strconcat (name, ".", ce->file_type, NULL);
	line = g_strdup_printf ("<img src=\"%s\" width=\"%d\" height=\"%d\" usemap=\"#map\" alt=\"%s\" />\n", filename, width, height, filename);
	gnome_vfs_write (ce->imap_handle, line, strlen (line), &temp);
	g_free (line);
	g_free (filename);

	line = g_strdup_printf ("<map name=\"map\" id=\"map\">\n");
	gnome_vfs_write (ce->imap_handle, line, strlen (line), &temp);
	g_free (line);
}


static void
end_page (CatalogPngExporter *ce,
	  int                 page_n)
{
	GdkPixbuf        *pixbuf;
	char             *name;
	char             *uri;
	char             *local_file;
	int               width, height;
	GnomeVFSFileSize  temp;
	char             *line;

	width = ce->page_width;
	height = get_page_height (ce, page_n);

	pixbuf = gdk_pixbuf_get_from_drawable (NULL,
					       ce->pixmap,
					       gdk_colormap_get_system (),
					       0, 0,
					       0, 0,
					       width,
					       height);

	name = _g_get_name_from_template (ce->templatev, ce->start_at + page_n - 1);
	uri = g_strconcat (ce->location, "/", name, ".", ce->file_type, NULL);
	local_file = get_cache_filename_from_uri (uri);
	
	if (strcmp (ce->file_type, "jpeg") == 0)
		_gdk_pixbuf_save (pixbuf, local_file, NULL, "jpeg", NULL, "quality", "85", NULL);
	else
		_gdk_pixbuf_save (pixbuf, local_file, NULL, ce->file_type, NULL, NULL);

	ce->created_files = g_list_prepend (ce->created_files, uri);

	g_free (local_file);
	g_free (name);
	g_object_unref (pixbuf);
	
	/* image map file. */

	if (! ce->write_image_map || (ce->imap_handle == NULL))
		return;

	line = g_strdup_printf ("</map>\n");
	gnome_vfs_write (ce->imap_handle, line, strlen (line), &temp);
	gnome_vfs_write (ce->imap_handle, HTML_PAGE_END, strlen (HTML_PAGE_END), &temp);
	gnome_vfs_close (ce->imap_handle);
	
	ce->created_files = g_list_prepend (ce->created_files, g_strdup (ce->imap_uri));
	
	g_free (line);
}


static void
paint_frame (CatalogPngExporter *ce,
	     GdkRectangle       *frame_rect,
	     GdkRectangle       *image_rect,
	     gchar              *filename)
{
	GnomeVFSFileSize  temp;
	char		 *unescaped_path, *attr_path;
	char             *line;
	char             *rel_path;
	char             *dest_dir;

	switch (ce->frame_style) {
	case GTH_FRAME_STYLE_NONE:
		break;

	case GTH_FRAME_STYLE_SLIDE:
		gthumb_draw_slide_with_colors (frame_rect->x,
					       frame_rect->y,
					       frame_rect->width,
					       frame_rect->height,
					       image_rect->width,
					       image_rect->height,
					       ce->pixmap,
					       &ce->frame_color,
					       &ce->black,
					       &ce->dark_gray,
					       &ce->gray,
					       &ce->white);
		break;

	case GTH_FRAME_STYLE_SIMPLE:
	case GTH_FRAME_STYLE_SHADOW:
	case GTH_FRAME_STYLE_SIMPLE_WITH_SHADOW:
		if (ce->frame_style == GTH_FRAME_STYLE_SHADOW)
			gthumb_draw_image_shadow (image_rect->x,
						  image_rect->y,
						  image_rect->width,
						  image_rect->height,
						  ce->pixmap);

		if (ce->frame_style == GTH_FRAME_STYLE_SIMPLE_WITH_SHADOW)
			gthumb_draw_frame_shadow (image_rect->x,
						  image_rect->y,
						  image_rect->width,
						  image_rect->height,
						  ce->pixmap);

		if ((ce->frame_style == GTH_FRAME_STYLE_SIMPLE)
		    || (ce->frame_style == GTH_FRAME_STYLE_SIMPLE_WITH_SHADOW)) {
			gthumb_draw_frame (image_rect->x,
					   image_rect->y,
					   image_rect->width,
					   image_rect->height,
					   ce->pixmap,
					   &ce->frame_color);
		}
		break;

	case GTH_FRAME_STYLE_SHADOW_IN:
		gthumb_draw_image_shadow_in (image_rect->x,
					     image_rect->y,
					     image_rect->width,
					     image_rect->height,
					     ce->pixmap);
		break;

	case GTH_FRAME_STYLE_SHADOW_OUT:
		gthumb_draw_image_shadow_out (image_rect->x,
					      image_rect->y,
					      image_rect->width,
					      image_rect->height,
					      ce->pixmap);
		break;
	}

	/* image map file. */

	if (! ce->write_image_map || (ce->imap_handle == NULL))
		return;

	dest_dir = remove_special_dirs_from_path (ce->location);
	rel_path = get_path_relative_to_uri (filename, dest_dir);
	g_free (dest_dir);

	unescaped_path = gnome_vfs_unescape_string (rel_path, NULL);
	attr_path = _g_escape_text_for_html (unescaped_path, -1);
	g_free (unescaped_path);

	line = g_strdup_printf ("<area shape=\"rect\" coords=\"%d,%d,%d,%d\" href=\"%s\" alt=\"%s\" />\n",
				frame_rect->x,
				frame_rect->y,
				frame_rect->x + frame_rect->width,
				frame_rect->y + frame_rect->height,
				rel_path,
				attr_path);
	g_free (rel_path);
	g_free (attr_path);
	gnome_vfs_write (ce->imap_handle, line, strlen (line), &temp);
	g_free (line);
}


static void
paint_image (CatalogPngExporter *ce,
	     GdkRectangle       *image_rect,
	     GdkPixbuf          *image)
{
	gint x, y, width, height;

	x = image_rect->x;
	y = image_rect->y;
	width = image_rect->width;
	height = image_rect->height;

	if (gdk_pixbuf_get_has_alpha (image)) {
		gdk_gc_set_rgb_fg_color (ce->gc, &ce->white);
		gdk_draw_rectangle (ce->pixmap,
				    ce->gc,
				    TRUE,
				    x, y,
				    width,
				    height);

		gdk_pixbuf_render_to_drawable_alpha (image,
						     ce->pixmap,
						     0, 0,
						     x, y,
						     width, height,
						     GDK_PIXBUF_ALPHA_BILEVEL,
						     112,
						     GDK_RGB_DITHER_MAX, 0, 0);

	} else
		gdk_pixbuf_render_to_drawable (image,
					       ce->pixmap,
					       ce->gc,
					       0, 0,
					       x, y,
					       width, height,
					       GDK_RGB_DITHER_MAX, 0, 0);
}


static void
paint_text (CatalogPngExporter *ce,
	    const char         *font_name,
	    GdkColor           *color,
	    int                 x,
	    int                 y,
	    int                 width,
	    const char         *utf8_text,
	    int                *height)
{
	PangoFontDescription *font_desc;
	PangoRectangle        bounds;

	if (font_name)
		font_desc = pango_font_description_from_string (font_name);
	else
		font_desc = pango_font_description_from_string (DEFAULT_FONT);
	pango_layout_set_font_description (ce->layout, font_desc);

	x += THUMB_FRAME_BORDER;

	pango_layout_set_text (ce->layout, utf8_text, -1);

	pango_layout_set_width (ce->layout, width * PANGO_SCALE);
	pango_layout_get_pixel_extents (ce->layout, NULL, &bounds);

	gdk_gc_set_rgb_fg_color (ce->gc, color);
	gdk_draw_layout_with_colors (ce->pixmap,
				     ce->gc,
				     x,
				     y,
				     ce->layout,
				     color,
				     NULL);

#if 0
	gdk_draw_rectangle (ce->pixmap,
			    ce->gc,
			    FALSE,
			    x, y,
			    width,
			    bounds.height);
#endif

	if (font_desc)
		pango_font_description_free (font_desc);

	if (height != NULL)
		*height = bounds.height;
}


static void
paint_comment (CatalogPngExporter *ce,
	       int                 x,
	       int                 y,
	       char               *utf8_text,
	       int                *height)
{
	PangoRectangle        bounds;
	const char           *font_name;
	char                 *escaped_text;
	char                 *marked_text;
	char                 *parsed_text;
	PangoAttrList        *original_attr_list;
	PangoAttrList        *attr_list;
	PangoFontDescription *font_desc;
        GError               *error = NULL;

	font_name = ce->caption_font_name;
	if (font_name)
		font_desc = pango_font_description_from_string (font_name);
	else
		font_desc = pango_font_description_from_string (DEFAULT_FONT);
	pango_layout_set_font_description (ce->layout, font_desc);

	original_attr_list = pango_layout_get_attributes (ce->layout);
	if (original_attr_list != NULL)
		pango_attr_list_ref (original_attr_list);
	x += THUMB_FRAME_BORDER;

	escaped_text = g_markup_escape_text (utf8_text, -1);
	marked_text = g_strdup_printf ("<i>%s</i>", escaped_text);
	g_free (escaped_text);

	if (! pango_parse_markup (marked_text, -1,
				  0,
				  &attr_list,
				  &parsed_text,
				  NULL,
				  &error)) {
		g_warning ("Failed to set text from markup due to error parsing markup: %s\nThis is the text that caused the error: %s",  error->message, utf8_text);
		g_error_free (error);
		g_free (marked_text);
		pango_attr_list_unref (original_attr_list);
		return;
	}
	g_free (marked_text);
	pango_layout_set_attributes (ce->layout, attr_list);
	pango_attr_list_unref (attr_list);

	/**/

	pango_layout_set_text (ce->layout, parsed_text, -1);
	g_free (parsed_text);

	pango_layout_set_width (ce->layout, ce->thumb_width * PANGO_SCALE);
	pango_layout_get_pixel_extents (ce->layout, NULL, &bounds);

	gdk_gc_set_rgb_fg_color (ce->gc, &ce->caption_color);
	gdk_draw_layout_with_colors (ce->pixmap,
				     ce->gc,
				     x,
				     y,
				     ce->layout,
				     &ce->caption_color,
				     NULL);

	*height = bounds.height;

	/**/

	if (font_desc)
		pango_font_description_free (font_desc);

	pango_layout_set_attributes (ce->layout, original_attr_list);
	if (original_attr_list != NULL)
		pango_attr_list_unref (original_attr_list);
}


static void
paint_footer (CatalogPngExporter *ce,
	      int                 page_n)
{
	if (ce->footer != NULL) {
		char *text;
		int   y;

		text = get_footer_text (ce, page_n);
		y = get_page_height (ce, page_n) - get_footer_height (ce) - ROW_SPACING / 2;
		paint_text (ce,
			    ce->footer_font_name,
			    &ce->footer_color,
			    0,
			    y,
			    ce->page_width - COL_SPACING,
			    text,
			    NULL);
		g_free (text);
	}

}


static int
get_text_height (CatalogPngExporter *ce,
		 const char         *text,
		 const char         *font_name,
		 int                 width)
{
	PangoFontDescription *font_desc;
	PangoRectangle        bounds;
	char                 *utf8_text;

	if (font_name)
		font_desc = pango_font_description_from_string (font_name);
	else
		font_desc = pango_font_description_from_string (DEFAULT_FONT);
	pango_layout_set_font_description (ce->layout, font_desc);

	pango_layout_set_width (ce->layout, width * PANGO_SCALE);

	utf8_text = g_locale_to_utf8 (text, -1, NULL, NULL, NULL);
	pango_layout_set_text (ce->layout, utf8_text, -1);
	g_free (utf8_text);

	pango_layout_get_pixel_extents (ce->layout, NULL, &bounds);

	if (font_desc)
		pango_font_description_free (font_desc);

	return bounds.height;
}


static int
get_header_height_with_spacing (CatalogPngExporter *ce)
{
	if (ce->header != NULL)
		return get_text_height (ce, ce->header, ce->header_font_name, ce->page_width) + (ROW_SPACING * 2);
	else
		return 0;
}


static int
get_footer_height (CatalogPngExporter *ce)
{
	if (ce->footer != NULL)
		return get_text_height (ce, ce->footer, ce->footer_font_name, ce->page_width);
	else
		return 0;
}


static int
get_footer_height_with_spacing (CatalogPngExporter *ce)
{
	if (ce->footer != NULL)
		return get_text_height (ce, ce->footer, ce->footer_font_name, ce->page_width) + (ROW_SPACING * 2);
	else
		return 0;
}


static char *
get_hf_text (const char *utf8_text, int n, int p)
{
	const char *s;
	GString    *r;
	char       *r_str;

	if (utf8_text == NULL)
		return NULL;

	if (g_utf8_strchr (utf8_text, -1, '%') == NULL)
		return g_strdup (utf8_text);

	r = g_string_new (NULL);
	for (s = utf8_text; *s != 0; s = g_utf8_next_char (s)) {
		gunichar ch = g_utf8_get_char (s);

		if (*s == '%') {
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

			case 'p':
				t = g_strdup_printf ("%d", p);
				g_string_append (r, t);
				g_free (t);
				break;

			case 'n':
				t = g_strdup_printf ("%d", n);
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


static char *
get_header_text (CatalogPngExporter *ce,
		 int                 page_n)
{
	return get_hf_text (ce->header, ce->pages_n, page_n);
}


static char *
get_footer_text (CatalogPngExporter *ce,
		 int                 page_n)
{
	return get_hf_text (ce->footer, ce->pages_n, page_n);
}

