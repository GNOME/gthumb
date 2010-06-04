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
#include <gthumb.h>
#include "gth-web-exporter.h"
#include "albumtheme-private.h"

#define DATE_FORMAT _("%d %B %Y, %H:%M")
#define DEFAULT_THUMB_SIZE 100
#define DEFAULT_INDEX_FILE "index.html"
#define SAVING_TIMEOUT 5

/* Default subdirectories.
 * - Used as fallback when gconf values are empty or not accessible
 * - Please keep in sync with values in gthumb.schemas.in
 */

#define DEFAULT_WEB_DIR_PREVIEWS     "previews"
#define DEFAULT_WEB_DIR_THUMBNAILS   "thumbnails"
#define DEFAULT_WEB_DIR_IMAGES       "images"
#define DEFAULT_WEB_DIR_HTML_IMAGES  "html"
#define DEFAULT_WEB_DIR_HTML_INDEXES "html"
#define DEFAULT_WEB_DIR_THEME_FILES  "theme"


typedef enum {
	GTH_VISIBILITY_ALWAYS = 0,
	GTH_VISIBILITY_INDEX,
	GTH_VISIBILITY_IMAGE
} GthTagVisibility;

typedef enum {
	GTH_IMAGE_TYPE_IMAGE = 0,
	GTH_IMAGE_TYPE_THUMBNAIL,
	GTH_IMAGE_TYPE_PREVIEW
} GthAttrImageType;


extern int yyparse (void);
extern GFileInputStream *yy_istream;

static GObjectClass *parent_class = NULL;

typedef struct {
	GthFileData *file_data;
	char        *comment;
	char        *place;
	char        *date_time;
	char        *dest_filename;
	GdkPixbuf   *image;
	int          image_width, image_height;
	GdkPixbuf   *thumb;
	int          thumb_width, thumb_height;
	GdkPixbuf   *preview;
	int          preview_width, preview_height;
	gboolean     caption_set;
	gboolean     no_preview;
} ImageData;

#define IMAGE_DATA(x) ((ImageData*)(x))


typedef struct {
	char *previews;
	char *thumbnails;
	char *images;
	char *html_images;
	char *html_indexes;
	char *theme_files;
} AlbumDirs;


struct _GthWebExporterPrivate {
	GthBrowser   *browser;

	GList        *file_list;              /* GFile elements. */

	char         *header;
	char         *footer;
	int           page_rows;              /* Number of rows and columns
	 				       * each page must have. */
	int           page_cols;
	gboolean      single_index;
	gboolean      use_subfolders;
	AlbumDirs     directories;
	char         *index_file;

	GFile        *style_dir;
	GFile        *target_dir;               /* Save files in this location. */
	GFile        *target_tmp_dir;

	char         *info;

	int           thumb_width;
	int           thumb_height;

	gboolean      copy_images;
	GthSortMethod sort_method;
	GtkSortType   sort_type;

	gboolean      resize_images;
	int           resize_max_width;
	int           resize_max_height;

	int           preview_min_width;
	int           preview_min_height;

	int           preview_max_width;
	int           preview_max_height;

	guint16       index_caption_mask;
	guint16       image_caption_mask;

	/**/

	ImageLoader  *iloader;
	GList        *current_file;          /* Next file to be loaded. */

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

	gboolean      interrupted;
};


static ImageData *
image_data_new (GthFileData *file_data,
		int          file_idx)
{
	ImageData   *idata;
	CommentData *cdata;

	idata = g_new0 (ImageData, 1);

	cdata = comments_load_comment (file->path, TRUE);
	if (cdata != NULL) {
		idata->comment = g_strdup (cdata->comment);
		idata->place = g_strdup (cdata->place);
		if (cdata->time != 0) {
			struct tm *tm;
			char   time_txt[50];
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
	}
	else {
		idata->comment = NULL;
		idata->place = NULL;
		idata->date_time = NULL;
	}

	idata->file_data = file_data_ref (file);
	idata->dest_filename = g_strdup_printf ("%03d-%s",
						file_idx,
						file_name_from_path (file->path));

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
	file_data_unref (idata->file_data);
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
free_parsed_docs (GthWebExporter *self)
{
	if (self->priv->index_parsed != NULL) {
		gth_parsed_doc_free (self->priv->index_parsed);
		self->priv->index_parsed = NULL;
	}

	if (self->priv->thumbnail_parsed != NULL) {
		gth_parsed_doc_free (self->priv->thumbnail_parsed);
		self->priv->thumbnail_parsed = NULL;
	}

	if (self->priv->image_parsed != NULL) {
		gth_parsed_doc_free (self->priv->image_parsed);
		self->priv->image_parsed = NULL;
	}
}


static GFile *
gfile_get_style_dir (GthWebExporter *self,
		     const char     *style)
{
	GFile *dir;
	GFile *style_dir;

	if (style == NULL)
		return NULL;

	dir = gfile_get_home_dir ();
	style_dir = gfile_append_path (dir,
			               ".gnome2",
			               "gthumb",
			               "albumthemes",
			               style,
			               NULL);
 	g_object_unref (dir);

	if (! gfile_path_is_dir (style_dir)) {
		g_object_unref (style_dir);

		style_dir = gfile_new_va (GTHUMB_DATADIR,
				          "gthumb",
				          "albumthemes",
				          style,
				          NULL);
		if (! gfile_path_is_dir (style_dir)) {
			g_object_unref (style_dir);
			style_dir = NULL;
		}
	}

	return style_dir;
}


/* FIXME */


#define RETURN_IMAGE_FIELD(image, field) {	\
	if (image == NULL)			\
		return 0;			\
	else					\
		return image->field;		\
}


static int
get_var_value (const char *var_name,
	       gpointer    data)
{
	GthWebExporter *self = data;

	if (strcmp (var_name, "image_idx") == 0)
		return self->priv->image + 1;
	else if (strcmp (var_name, "images") == 0)
		return self->priv->n_images;
	else if (strcmp (var_name, "page_idx") == 0)
		return self->priv->page + 1;
	else if (strcmp (var_name, "page_rows") == 0)
		return self->priv->page_rows;
	else if (strcmp (var_name, "page_cols") == 0)
		return self->priv->page_cols;
	else if (strcmp (var_name, "pages") == 0)
		return self->priv->n_pages;
	else if (strcmp (var_name, "preview_min_width") == 0)
		return self->priv->preview_min_width;
	else if (strcmp (var_name, "preview_min_height") == 0)
		return self->priv->preview_min_height;
	else if (strcmp (var_name, "index") == 0)
		return GTH_VISIBILITY_INDEX;
	else if (strcmp (var_name, "image") == 0)
		return GTH_VISIBILITY_IMAGE;
	else if (strcmp (var_name, "always") == 0)
		return GTH_VISIBILITY_ALWAYS;

	else if (strcmp (var_name, "image_width") == 0)
		RETURN_IMAGE_FIELD (self->priv->eval_image, image_width)
	else if (strcmp (var_name, "image_height") == 0)
		RETURN_IMAGE_FIELD (self->priv->eval_image, image_height)
	else if (strcmp (var_name, "preview_width") == 0)
		RETURN_IMAGE_FIELD (self->priv->eval_image, preview_width)
	else if (strcmp (var_name, "preview_height") == 0)
		RETURN_IMAGE_FIELD (self->priv->eval_image, preview_height)
	else if (strcmp (var_name, "thumb_width") == 0)
		RETURN_IMAGE_FIELD (self->priv->eval_image, thumb_width)
	else if (strcmp (var_name, "thumb_height") == 0)
		RETURN_IMAGE_FIELD (self->priv->eval_image, thumb_height)

	else if (strcmp (var_name, "image_dim_visibility_index") == 0)
		return self->priv->index_caption_mask & GTH_CAPTION_IMAGE_DIM;
	else if (strcmp (var_name, "file_name_visibility_index") == 0)
		return self->priv->index_caption_mask & GTH_CAPTION_FILE_NAME;
 	else if (strcmp (var_name, "file_path_visibility_index") == 0)
		return self->priv->index_caption_mask & GTH_CAPTION_FILE_PATH;
	else if (strcmp (var_name, "file_size_visibility_index") == 0)
		return self->priv->index_caption_mask & GTH_CAPTION_FILE_SIZE;
	else if (strcmp (var_name, "comment_visibility_index") == 0)
		return self->priv->index_caption_mask & GTH_CAPTION_COMMENT;
	else if (strcmp (var_name, "place_visibility_index") == 0)
		return self->priv->index_caption_mask & GTH_CAPTION_PLACE;
	else if (strcmp (var_name, "date_time_visibility_index") == 0)
		return self->priv->index_caption_mask & GTH_CAPTION_DATE_TIME;
	else if (strcmp (var_name, "exif_date_time_visibility_index") == 0)
		return self->priv->index_caption_mask & GTH_CAPTION_EXIF_DATE_TIME;
	else if (strcmp (var_name, "exif_exposure_time_visibility_index") == 0)
		return self->priv->index_caption_mask & GTH_CAPTION_EXIF_EXPOSURE_TIME;
	else if (strcmp (var_name, "exif_exposure_mode_visibility_index") == 0)
		return self->priv->index_caption_mask & GTH_CAPTION_EXIF_EXPOSURE_MODE;
	else if (strcmp (var_name, "exif_flash_visibility_index") == 0)
		return self->priv->index_caption_mask & GTH_CAPTION_EXIF_FLASH;
	else if (strcmp (var_name, "exif_shutter_speed_visibility_index") == 0)
		return self->priv->index_caption_mask & GTH_CAPTION_EXIF_SHUTTER_SPEED;
	else if (strcmp (var_name, "exif_aperture_value_visibility_index") == 0)
		return self->priv->index_caption_mask & GTH_CAPTION_EXIF_APERTURE_VALUE;
	else if (strcmp (var_name, "exif_focal_length_visibility_index") == 0)
		return self->priv->index_caption_mask & GTH_CAPTION_EXIF_FOCAL_LENGTH;
	else if (strcmp (var_name, "exif_camera_model_visibility_index") == 0)
		return self->priv->index_caption_mask & GTH_CAPTION_EXIF_CAMERA_MODEL;

	else if (strcmp (var_name, "image_dim_visibility_image") == 0)
		return self->priv->image_caption_mask & GTH_CAPTION_IMAGE_DIM;
	else if (strcmp (var_name, "file_name_visibility_image") == 0)
		return self->priv->image_caption_mask & GTH_CAPTION_FILE_NAME;
 	else if (strcmp (var_name, "file_path_visibility_image") == 0)
		return self->priv->image_caption_mask & GTH_CAPTION_FILE_PATH;
	else if (strcmp (var_name, "file_size_visibility_image") == 0)
		return self->priv->image_caption_mask & GTH_CAPTION_FILE_SIZE;
	else if (strcmp (var_name, "comment_visibility_image") == 0)
		return self->priv->image_caption_mask & GTH_CAPTION_COMMENT;
	else if (strcmp (var_name, "place_visibility_image") == 0)
		return self->priv->image_caption_mask & GTH_CAPTION_PLACE;
	else if (strcmp (var_name, "date_time_visibility_image") == 0)
		return self->priv->image_caption_mask & GTH_CAPTION_DATE_TIME;
	else if (strcmp (var_name, "exif_date_time_visibility_image") == 0)
		return self->priv->image_caption_mask & GTH_CAPTION_EXIF_DATE_TIME;
	else if (strcmp (var_name, "exif_exposure_time_visibility_image") == 0)
		return self->priv->image_caption_mask & GTH_CAPTION_EXIF_EXPOSURE_TIME;
	else if (strcmp (var_name, "exif_exposure_mode_visibility_image") == 0)
		return self->priv->image_caption_mask & GTH_CAPTION_EXIF_EXPOSURE_MODE;
	else if (strcmp (var_name, "exif_flash_visibility_image") == 0)
		return self->priv->image_caption_mask & GTH_CAPTION_EXIF_FLASH;
	else if (strcmp (var_name, "exif_shutter_speed_visibility_image") == 0)
		return self->priv->image_caption_mask & GTH_CAPTION_EXIF_SHUTTER_SPEED;
	else if (strcmp (var_name, "exif_aperture_value_visibility_image") == 0)
		return self->priv->image_caption_mask & GTH_CAPTION_EXIF_APERTURE_VALUE;
	else if (strcmp (var_name, "exif_focal_length_visibility_image") == 0)
		return self->priv->image_caption_mask & GTH_CAPTION_EXIF_FOCAL_LENGTH;
	else if (strcmp (var_name, "exif_camera_model_visibility_image") == 0)
		return self->priv->image_caption_mask & GTH_CAPTION_EXIF_CAMERA_MODEL;

	else if (strcmp (var_name, "copy_originals") == 0)
		return self->priv->copy_images;

	g_warning ("[GetVarValue] Unknown variable name: %s", var_name);

	return 0;
}


static int
expression_value (GthWebExporter *self,
		  GthExpr            *expr)
{
	gth_expr_set_get_var_value_func (expr, get_var_value, self);
	return gth_expr_eval (expr);
}


static int
gth_tag_get_idx (GthTag             *tag,
		 GthWebExporter *self,
		 int                 default_value,
		 int                 max_value)
{
	GList *scan;
	int    retval = default_value;

	for (scan = tag->value.arg_list; scan; scan = scan->next) {
		GthVar *var = scan->data;

		if (strcmp (var->name, "idx_relative") == 0) {
			retval = default_value + expression_value (self, var->value.expr);
			break;

		} else if (strcmp (var->name, "idx") == 0) {
			retval = expression_value (self, var->value.expr) - 1;
			break;
		}
	}

	retval = MIN (retval, max_value);
	retval = MAX (retval, 0);

	return retval;
}


static int
get_image_idx (GthTag             *tag,
	       GthWebExporter *self)
{
	return gth_tag_get_idx (tag, self, self->priv->image, self->priv->n_images - 1);
}


static int
get_page_idx (GthTag             *tag,
	      GthWebExporter *self)
{
	return gth_tag_get_idx (tag, self, self->priv->page, self->priv->n_pages - 1);
}


static int
gth_tag_get_var (GthWebExporter *self,
		 GthTag         *tag,
		 const char     *var_name)
{
	GList *scan;

	for (scan = tag->value.arg_list; scan; scan = scan->next) {
		GthVar *var = scan->data;
		if (strcmp (var->name, var_name) == 0)
			return expression_value (self, var->value.expr);
	}

	return 0;
}


static const char *
gth_tag_get_str (GthWebExporter *self,
		 GthTag         *tag,
		 const char     *var_name)
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
get_page_idx_from_image_idx (GthWebExporter *self,
			     int                 image_idx)
{
	if (self->priv->single_index)
		return 0;
	else
		return image_idx / (self->priv->page_rows * self->priv->page_cols);
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


/* write a line when no error is pending */

static void
_write_line (GFileOutputStream   *ostream,
	     GError             **error,
	     const char          *line)
{
	if (error != NULL && *error != NULL)
		return;

	gfile_output_stream_write (ostream, error, line);
}


static void
_write_locale_line (GFileOutputStream  *ostream,
		    GError            **error,
		    const char         *line)
{
	char *utf8_line;

	utf8_line = g_locale_to_utf8 (line, -1, 0, 0, 0);
	_write_line (ostream, error, utf8_line);
	g_free (utf8_line);
}


static void
write_line (GFileOutputStream  *ostream,
	    GError            **error,
	    const char         *line)
{
	if (line_is_void (line))
		return;

	_write_line (ostream, error, line);
}


static void
write_markup_escape_line (GFileOutputStream  *ostream,
                          GError            **error,
                          const char         *line)
{
	char *e_line;

	if (line_is_void (line))
		return;

	e_line = _g_escape_text_for_html (line, -1);
	_write_line (ostream, error, e_line);
	g_free (e_line);
}


static void
write_markup_escape_locale_line (GFileOutputStream  *ostream,
                                 GError            **error,
                                 const char         *line)
{
	char *e_line;

	if (line == NULL)
		return;
	if (*line == 0)
		return;

	e_line = _g_escape_text_for_html (line, -1);
	_write_locale_line (ostream, error, e_line);
	g_free (e_line);
}


/* GFile to string */

static char *
gfile_get_relative_uri (GFile *file,
		        GFile *relative_to)
{
	char  *escaped;
	char  *relative_uri;
	char  *result;

	escaped = gfile_get_uri (file);
	relative_uri = gfile_get_uri (relative_to);

	result = get_path_relative_to_uri (escaped, relative_uri);

	g_free (relative_uri);
	g_free (escaped);

	return result;
}


static char *
gfile_get_relative_path (GFile *file,
		         GFile *relative_to)
{
	char  *escaped, *unescaped;

	escaped = gfile_get_relative_uri (file, relative_to);
	unescaped = g_uri_unescape_string (escaped, NULL);

	g_free (escaped);

	return unescaped;
}


/* construct a GFile for a GthWebExporter */


GFile *
get_album_file (GthWebExporter *self,
		GFile          *target_dir,
		const char     *subdir,
		const char     *filename)
{
	return _g_file_get_child (target_dir,
				  (self->priv->use_subfolders ? subdir : filename),
				  (self->priv->use_subfolders ? filename : NULL),
				  NULL);
}


GFile *
get_html_index_dir (GthWebExporter *self,
		    const int       page,
		    GFile          *target_dir)
{
	if (page == 0)
		return g_file_dup (target_dir);
	else
		return get_album_file (self, target_dir, self->priv->directories.html_indexes, NULL);
}


GFile *
get_html_image_dir (GthWebExporter *self,
		    GFile          *target_dir)
{
	return get_album_file (self, target_dir, self->priv->directories.html_images, NULL);
}


GFile *
get_theme_file (GthWebExporter *self,
		GFile          *target_dir,
		const char     *filename)
{
	return get_album_file (self, target_dir, self->priv->directories.theme_files, filename);
}


GFile *
get_html_index_file (GthWebExporter *self,
		     const int       page,
		     GFile          *target_dir)
{
	GFile *dir, *result;
	char  *filename;

	if (page == 0)
		filename = g_strdup (self->priv->index_file);
	else
		filename = g_strdup_printf ("page%03d.htpl", page + 1);
	dir = get_html_index_dir (self, page, target_dir);
	result = get_album_file (self, dir, filename, NULL);

	g_free (filename);
	g_object_unref (dir);

	return result;
}


GFile *
get_html_image_file (GthWebExporter *self,
		     ImageData      *image_data,
		     GFile          *target_dir)
{
	char  *filename;
	GFile *result;

	filename = g_strconcat (image_data->dest_filename, ".html", NULL);
	result = get_album_file (self, target_dir, self->priv->directories.html_images, filename);

	g_free (filename);

	return result;
}


GFile *
get_thumbnail_file (GthWebExporter *self,
		    ImageData      *image_data,
		    GFile          *target_dir)
{
	char  *filename;
	GFile *result;

	filename = g_strconcat (image_data->dest_filename, ".small", ".jpeg", NULL);
	result = get_album_file (self, target_dir, self->priv->directories.thumbnails, filename);

	g_free (filename);

	return result;
}


GFile *
get_image_file (GthWebExporter *self,
		ImageData      *image_data,
		GFile          *target_dir)
{
	GFile *result;

	if (self->priv->copy_images)
		result = get_album_file (self, target_dir, self->priv->directories.images, image_data->dest_filename);
	else
		result = g_file_dup (image_data->file_data->file);

	return result;
}


GFile *
get_preview_file (GthWebExporter *self,
		  ImageData      *image_data,
		  GFile          *target_dir)
{
	GFile *result;

	if (image_data->no_preview) {
		result = get_image_file (self, image_data, target_dir);
	}
	else {
		char *filename;

		filename = g_strconcat (image_data->dest_filename, ".medium", ".jpeg", NULL);
		result = get_album_file (self, target_dir, self->priv->directories.previews, filename);

		g_free (filename);
	}

	return result;
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



static GthAttrImageType
get_attr_image_type_from_tag (GthWebExporter *self,
			      GthTag             *tag)
{
	if (gth_tag_get_var (self, tag, "thumbnail") != 0)
		return GTH_IMAGE_TYPE_THUMBNAIL;

	if (gth_tag_get_var (self, tag, "preview") != 0)
		return GTH_IMAGE_TYPE_PREVIEW;

	return GTH_IMAGE_TYPE_IMAGE;
}


static void
gth_parsed_doc_print (GList               *document,
		      GFile		  *relative_to,
		      GthWebExporter  *self,
		      GFileOutputStream   *ostream,
		      GError             **error,
		      gboolean             allow_table)
{
	GList *scan;

	for (scan = document; scan; scan = scan->next) {
		GthTag     *tag = scan->data;
		ImageData  *idata;
		GFile      *file;
		GFile      *dir;
		char       *line = NULL;
		char       *image_src = NULL;
		char       *unescaped_path = NULL;
		int         idx;
		int         image_width;
		int         image_height;
		int         max_size;
		int         r, c;
		int         value;
		const char *src;
		char       *src_attr;
		const char *class = NULL;
		char       *class_attr = NULL;
		const char *alt = NULL;
		char       *alt_attr = NULL;
		const char *id = NULL;
		char       *id_attr = NULL;
		gboolean   relative;
		GList      *scan;


		if (error != NULL && *error != NULL)
			return;

		switch (tag->type) {
		case GTH_TAG_HEADER:
			line = get_hf_text (self->priv->header);
			write_markup_escape_line (ostream, error, line);
			break;

		case GTH_TAG_FOOTER:
			line = get_hf_text (self->priv->footer);
			write_markup_escape_line (ostream, error, line);
			break;

		case GTH_TAG_LANGUAGE:
			line = get_current_language ();
			write_markup_escape_line (ostream, error, line);
			break;

		case GTH_TAG_THEME_LINK:
			src = gth_tag_get_str (self, tag, "src");
			if (src == NULL)
				break;

			file = get_theme_file (self,
					       self->priv->target_dir,
					       src);
			line = gfile_get_relative_uri (file, relative_to);

			write_markup_escape_line (ostream, error, line);

			g_object_unref (file);
			break;

		case GTH_TAG_IMAGE:
			idx = get_image_idx (tag, self);
			idata = g_list_nth (self->priv->file_list, idx)->data;
			self->priv->eval_image = idata;

			switch (get_attr_image_type_from_tag (self, tag)) {
			case GTH_IMAGE_TYPE_THUMBNAIL:
				file = get_thumbnail_file (self,
							   idata,
							   self->priv->target_dir);
				image_width = idata->thumb_width;
				image_height = idata->thumb_height;
				break;

			case GTH_IMAGE_TYPE_PREVIEW:
				file = get_preview_file (self,
							 idata,
							 self->priv->target_dir);
				image_width = idata->preview_width;
				image_height = idata->preview_height;
				break;

			case GTH_IMAGE_TYPE_IMAGE:
				file = get_image_file (self,
						       idata,
						       self->priv->target_dir);
				image_width = idata->image_width;
				image_height = idata->image_height;
				break;
			}

			image_src = gfile_get_relative_uri (file, relative_to);
			src_attr = _g_escape_text_for_html (image_src, -1);

			class = gth_tag_get_str (self, tag, "class");
			if (class)
				class_attr = g_strdup_printf (" class=\"%s\"", class);
			else
				class_attr = g_strdup ("");

			max_size = gth_tag_get_var (self, tag, "max_size");
			if (max_size > 0)
				scale_keeping_ratio (&image_width,
						     &image_height,
						     max_size,
						     max_size,
						     FALSE);

			alt = gth_tag_get_str (self, tag, "alt");
			if (alt != NULL)
				alt_attr = g_strdup (alt);
			else {
				char *unescaped_path;

				unescaped_path = g_uri_unescape_string (image_src, NULL);
				alt_attr = _g_escape_text_for_html (unescaped_path, -1);
				g_free (unescaped_path);
			}

			id = gth_tag_get_str (self, tag, "id");
			if (id != NULL)
				id_attr = g_strdup_printf (" id=\"%s\"", id);
			else
				id_attr = g_strdup ("");

			line = g_strdup_printf ("<img src=\"%s\" alt=\"%s\" width=\"%d\" height=\"%d\"%s%s />",
						src_attr,
						alt_attr,
						image_width,
						image_height,
						id_attr,
						class_attr);
			write_line (ostream, error, line);

			g_free (src_attr);
			g_free (id_attr);
			g_free (alt_attr);
			g_free (class_attr);
			g_free (image_src);
			g_object_unref (file);
			break;

		case GTH_TAG_IMAGE_LINK:
			idx = get_image_idx (tag, self);
			idata = g_list_nth (self->priv->file_list, idx)->data;
			file = get_html_image_file (self,
						    idata,
						    self->priv->target_dir);
			line = gfile_get_relative_uri (file, relative_to);
			write_markup_escape_line (ostream, error, line);

			g_object_unref (file);
			break;

		case GTH_TAG_IMAGE_IDX:
			line = g_strdup_printf ("%d", get_image_idx (tag, self) + 1);
			write_line (ostream, error, line);
			break;

		case GTH_TAG_IMAGE_DIM:
			idx = get_image_idx (tag, self);
			idata = g_list_nth (self->priv->file_list, idx)->data;
			line = g_strdup_printf ("%dx%d",
						idata->image_width,
						idata->image_height);
			write_line (ostream, error, line);
			break;

		case GTH_TAG_IMAGES:
			line = g_strdup_printf ("%d", self->priv->n_images);
			write_line (ostream, error, line);
			break;

		case GTH_TAG_FILENAME:
			idx = get_image_idx (tag, self);
			idata = g_list_nth (self->priv->file_list, idx)->data;
			self->priv->eval_image = idata;

			switch (get_attr_image_type_from_tag (self, tag)) {
			case GTH_IMAGE_TYPE_THUMBNAIL:
				file = get_thumbnail_file (self,
							   idata,
							   self->priv->target_dir);
				break;

			case GTH_IMAGE_TYPE_PREVIEW:
				file = get_preview_file (self,
							 idata,
							 self->priv->target_dir);
				break;

			case GTH_IMAGE_TYPE_IMAGE:
				file = get_image_file (self,
						       idata,
						       self->priv->target_dir);
				break;
			}

			relative = (gth_tag_get_var (self, tag, "with_relative_path") != 0);

			if (relative)
				unescaped_path = gfile_get_relative_path (file,
									 relative_to);
			else
				unescaped_path = gfile_get_path (file);


			if (relative || (gth_tag_get_var (self, tag, "with_path") != 0)) {
				line = unescaped_path;
			} else {
				line = g_strdup (file_name_from_path (unescaped_path));
				g_free (unescaped_path);
			}

			if  (gth_tag_get_var (self, tag, "utf8") != 0)
				write_markup_escape_locale_line (ostream, error, line);
			else
				write_markup_escape_line (ostream, error, line);

			g_object_unref (file);
			break;

		case GTH_TAG_FILEPATH:
			idx = get_image_idx (tag, self);
			idata = g_list_nth (self->priv->file_list, idx)->data;
			self->priv->eval_image = idata;

			switch (get_attr_image_type_from_tag (self, tag)) {
			case GTH_IMAGE_TYPE_THUMBNAIL:
				file = get_thumbnail_file (self,
							   idata,
							   self->priv->target_dir);
				break;

			case GTH_IMAGE_TYPE_PREVIEW:
				file = get_preview_file (self,
							 idata,
							 self->priv->target_dir);
				break;

			case GTH_IMAGE_TYPE_IMAGE:
				file = get_image_file (self,
						       idata,
						       self->priv->target_dir);
				break;
			}

			dir = g_file_get_parent (file);

			relative = (gth_tag_get_var (self, tag, "relative_path") != 0);

			if (relative)
				line = gfile_get_relative_path (dir,
							       relative_to);
			else
				line = gfile_get_path (dir);

			if  (gth_tag_get_var (self, tag, "utf8") != 0)
				write_markup_escape_locale_line (ostream, error, line);
			else
				write_markup_escape_line (ostream, error, line);

			g_object_unref (dir);
			g_object_unref (file);
			break;

		case GTH_TAG_FILESIZE:
			idx = get_image_idx (tag, self);
			idata = g_list_nth (self->priv->file_list, idx)->data;
			line = g_format_size_for_display (idata->file_data->size);
			write_markup_escape_line (ostream, error, line);
			break;

		case GTH_TAG_COMMENT:
			idx = get_image_idx (tag, self);
			idata = g_list_nth (self->priv->file_list, idx)->data;
			self->priv->eval_image = idata;

			if (idata->comment == NULL)
				break;

			max_size = gth_tag_get_var (self, tag, "max_size");
			if (max_size <= 0)
				line = g_strdup (idata->comment);
			else {
				char *comment;

				comment = g_strndup (idata->comment, max_size);
				if (strlen (comment) < strlen (idata->comment))
					line = g_strconcat (comment, "...", NULL);
				else
					line = g_strdup (comment);
				g_free (comment);
			}

			write_markup_escape_line (ostream, error, line);
			break;

		case GTH_TAG_PLACE:
			idx = get_image_idx (tag, self);
			idata = g_list_nth (self->priv->file_list, idx)->data;
			self->priv->eval_image = idata;

			if (idata->place == NULL)
				break;

			max_size = gth_tag_get_var (self, tag, "max_size");
			if (max_size <= 0)
				line = g_strdup (idata->place);
			else
			{
				char *place = g_strndup (idata->place, max_size);
				if (strlen (place) < strlen (idata->place))
					line = g_strconcat (place, "...", NULL);
				else
					line = g_strdup (place);
				g_free (place);
			}

			write_markup_escape_line (ostream, error, line);
			break;

		case GTH_TAG_DATE_TIME:
			idx = get_image_idx (tag, self);
			idata = g_list_nth (self->priv->file_list, idx)->data;
			self->priv->eval_image = idata;

			if (idata->date_time == NULL)
				break;

			max_size = gth_tag_get_var (self, tag, "max_size");
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

			write_markup_escape_line (ostream, error, line);
			break;

		case GTH_TAG_PAGE_LINK:
			if (gth_tag_get_var (self, tag, "image_idx") != 0) {
				int image_idx;
				image_idx = get_image_idx (tag, self);
				idx = get_page_idx_from_image_idx (self, image_idx);
			}
			else
				idx = get_page_idx (tag, self);

			file = get_html_index_file (self,
						    idx,
						    self->priv->target_dir);
			line = gfile_get_relative_uri (file, relative_to);
			write_markup_escape_line (ostream, error, line);

			g_object_unref (file);
			break;

		case GTH_TAG_PAGE_IDX:
			line = g_strdup_printf ("%d", get_page_idx (tag, self) + 1);
			write_line (ostream, error, line);
			break;

		case GTH_TAG_PAGE_ROWS:
			line = g_strdup_printf ("%d", self->priv->page_rows);
			write_line (ostream, error, line);
			break;

		case GTH_TAG_PAGE_COLS:
			line = g_strdup_printf ("%d", self->priv->page_cols);
			write_line (ostream, error, line);
			break;

		case GTH_TAG_PAGES:
			line = g_strdup_printf ("%d", self->priv->n_pages);
			write_line (ostream, error, line);
			break;

		case GTH_TAG_TABLE:
			if (! allow_table)
				break;

			if (self->priv->single_index)
				self->priv->page_rows = (self->priv->n_images + self->priv->page_cols - 1) / self->priv->page_cols;

			/* this may not work correctly if single_index is set */
			for (r = 0; r < self->priv->page_rows; r++) {
				if (self->priv->image < self->priv->n_images)
					write_line (ostream, error, "  <tr class=\"tr_index\">\n");
				else
					write_line (ostream, error, "  <tr class=\"tr_empty_index\">\n");
				for (c = 0; c < self->priv->page_cols; c++) {
					if (self->priv->image < self->priv->n_images) {
						write_line (ostream, error, "    <td class=\"td_index\">\n");
						gth_parsed_doc_print (self->priv->thumbnail_parsed,
								      relative_to,
								      self,
								      ostream,
								      error,
								      FALSE);
						write_line (ostream, error, "    </td>\n");
						self->priv->image++;
					}
					else {
						write_line (ostream, error, "    <td class=\"td_empty_index\">\n");
						write_line (ostream, error, "    &nbsp;\n");
						write_line (ostream, error, "    </td>\n");
					}
				}
				write_line (ostream, error, "  </tr>\n");
			}
			break;

		case GTH_TAG_THUMBS:
			if (! allow_table)
				break;

			for (r = 0; r < (self->priv->single_index ? self->priv->n_images : self->priv->page_rows * self->priv->page_cols); r++)
			{
				if (self->priv->image >= self->priv->n_images)
					break;
				gth_parsed_doc_print (self->priv->thumbnail_parsed,
						      relative_to,
						      self,
						      ostream,
						      error,
						      FALSE);
				self->priv->image++;
			}
			break;

		case GTH_TAG_DATE:
			line = get_current_date ();
			write_markup_escape_line (ostream, error, line);
			break;

		case GTH_TAG_HTML:
			write_line (ostream, error, tag->value.html);
			break;

		case GTH_TAG_EXIF_EXPOSURE_TIME:
			idx = get_image_idx (tag, self);
			idata = g_list_nth (self->priv->file_list, idx)->data;
			line = get_metadata_tagset_string (idata->file_data,
							    TAG_NAME_SETS[EXPTIME_TAG_NAMES]);
			write_markup_escape_line (ostream, error, line);
			break;

		case GTH_TAG_EXIF_EXPOSURE_MODE:
			idx = get_image_idx (tag, self);
			idata = g_list_nth (self->priv->file_list, idx)->data;
			line = get_metadata_tagset_string (idata->file_data,
							    TAG_NAME_SETS[EXPMODE_TAG_NAMES]);
			write_markup_escape_line (ostream, error, line);
			break;

		case GTH_TAG_EXIF_FLASH:
			idx = get_image_idx (tag, self);
			idata = g_list_nth (self->priv->file_list, idx)->data;
			line = get_metadata_tagset_string (idata->file_data,
					     		    TAG_NAME_SETS[FLASH_TAG_NAMES]);
			write_markup_escape_line (ostream, error, line);
			break;

		case GTH_TAG_EXIF_SHUTTER_SPEED:
			idx = get_image_idx (tag, self);
			idata = g_list_nth (self->priv->file_list, idx)->data;
			line = get_metadata_tagset_string (idata->file_data,
							    TAG_NAME_SETS[SHUTTERSPEED_TAG_NAMES]);
			write_markup_escape_line (ostream, error, line);
			break;

		case GTH_TAG_EXIF_APERTURE_VALUE:
			idx = get_image_idx (tag, self);
			idata = g_list_nth (self->priv->file_list, idx)->data;
			line = get_metadata_tagset_string (idata->file_data,
							    TAG_NAME_SETS[APERTURE_TAG_NAMES]);
			write_markup_escape_line (ostream, error, line);
			break;

		case GTH_TAG_EXIF_FOCAL_LENGTH:
			idx = get_image_idx (tag, self);
			idata = g_list_nth (self->priv->file_list, idx)->data;
			line = get_metadata_tagset_string (idata->file_data,
					     		    TAG_NAME_SETS[FOCAL_TAG_NAMES]);
			write_markup_escape_line (ostream, error, line);
			break;

		case GTH_TAG_EXIF_DATE_TIME:
			idx = get_image_idx (tag, self);
			idata = g_list_nth (self->priv->file_list, idx)->data;
			{
				time_t     t;
				struct tm *tp;
				char s[100];

				t = get_exif_time (idata->file_data);
				if (t != 0) {
					tp = localtime (&t);
					strftime (s, 99, DATE_FORMAT, tp);
					line = g_locale_to_utf8 (s, -1, 0, 0, 0);
					write_markup_escape_line (ostream, error, line);
				}
				else
					write_line (ostream, error, "-");

			}
			break;

		case GTH_TAG_EXIF_CAMERA_MODEL:
			idx = get_image_idx (tag, self);
			idata = g_list_nth (self->priv->file_list, idx)->data;
			line = get_metadata_tagset_string (idata->file_data,
							    TAG_NAME_SETS[MAKE_TAG_NAMES]);
			write_markup_escape_line (ostream, error, line);
			g_free (line);

			write_line (ostream, error, " &nbsp; ");

			line = get_metadata_tagset_string (idata->file_data,
					    		    TAG_NAME_SETS[MODEL_TAG_NAMES]);
			write_markup_escape_line (ostream, error, line);
			break;

		case GTH_TAG_SET_VAR:
			break;

		case GTH_TAG_EVAL:
			idx = get_image_idx (tag, self);
			idata = g_list_nth (self->priv->file_list, idx)->data;
			self->priv->eval_image = idata;

			value = gth_tag_get_var (self, tag, "expr");

			line = g_strdup_printf ("%d", value);
			write_line (ostream, error, line);
			break;

		case GTH_TAG_IF:
			idx = MIN (self->priv->image, self->priv->n_images - 1);
			idata = g_list_nth (self->priv->file_list, idx)->data;
			self->priv->eval_image = idata;

			for (scan = tag->value.cond_list; scan; scan = scan->next) {
				GthCondition *cond = scan->data;
				if (expression_value (self, cond->expr) != 0) {
					gth_parsed_doc_print (cond->document,
							      relative_to,
							      self,
							      ostream,
							      error,
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
				write_markup_escape_line (ostream, error, line);
			}
			break;

		default:
			break;
		}

		g_free (line);
	}
}



static gboolean
gfile_parsed_doc_print (GthWebExporter *self,
                        GList              *document,
                        GFile              *file,
			GFile              *relative_to)
{
        GFileOutputStream  *ostream;
        GError             *error = NULL;
        gboolean            result;

	gfile_debug (DEBUG_INFO, "save html file", file);

        ostream = g_file_replace (file, NULL, FALSE, G_FILE_CREATE_NONE, NULL, &error);

        if (error) {
                gfile_warning ("Cannot open html file for writing",
                               file,
                               error);
        }
        else {
		gth_parsed_doc_print (document,
				      relative_to,
				      self,
				      ostream,
				      &error,
				      TRUE);
	}

	g_output_stream_close (G_OUTPUT_STREAM(ostream), NULL, &error);
	g_object_unref (ostream);

	result = (error == NULL);
	g_clear_error (&error);

	return result;
}


static void
delete_temp_dir_ready_cb (GError   *error,
			  gpointer  user_data)
{
	GthWebExporter *self = user_data;

	if ((self->priv->error == NULL) && (error != NULL))
		self->priv->error = g_error_copy (error);
	gth_task_completed (GTH_TASK (self), self->priv->error);
}


static void
cleanup_and_terminate (GthWebExporter *self,
		       GError         *error)
{
	GList *file_list;

	if (error != NULL)
		self->priv->error = g_error_copy (error);

	if (self->priv->file_list != NULL) {
		g_list_foreach (self->priv->file_list, (GFunc) image_data_free, NULL);
		g_list_free (self->priv->file_list);
		self->priv->file_list = NULL;
	}

	file_list = g_list_append (NULL, self->priv->target_tmp_dir);
	_g_delete_files_async (file_list,
			       TRUE,
			       TRUE,
			       NULL,
			       delete_temp_dir_ready_cb,
			       self);
}


static void
copy_to_destination_ready_cb (GObject  *object,
			      GError   *error,
			      gpointer  user_data)
{
	cleanup_and_terminate (GTH_WEB_EXPORTER (user_data), error);
}


static void
save_other_files_progress_cb (GObject    *object,
			      const char *description,
			      const char *details,
			      gboolean    pulse,
			      double      fraction,
			      gpointer    user_data)
{
	GthWebExporter *self = user_data;

	gth_task_progress (GTH_TASK (self),
			   description,
			   details,
			   pulse,
			   fraction);
}


static void
save_other_files_dialog_cb (gboolean opened,
			    gpointer user_data)
{
	gth_task_dialog (GTH_TASK (user_data), opened);
}


static void
save_other_files_ready_cb (GObject  *object,
			   GError   *error,
			   gpointer  user_data)
{
	GthWebExporter *self = user_data;

	if (error != NULL) {
		cleanup_and_terminate (self, error);
		return;
	}

	source = gth_file_data_new (self->priv->target_tmp_dir, NULL);
	_g_copy_file_async (source,
			    self->priv->target_dir,
			    FALSE,
			    G_FILE_COPY_NONE,
			    GTH_OVERWRITE_RESPONSE_UNSPECIFIED,
			    G_PRIORITY_NORMAL,
			    gth_task_get_cancellable (GTH_TASK (self)),
			    save_other_files_progress_cb,
			    self,
			    save_other_files_dialog_cb,
			    self,
			    copy_to_destination_ready_cb,
			    self);
}


static void
save_other_files (GthWebExporter *self)
{
	GFileEnumerator *enumerator;
	GError          *error = NULL;
	GFileInfo       *info;
	GList           *files;

	enumerator = g_file_enumerate_children (self->priv->style_dir,
					        G_FILE_ATTRIBUTE_STANDARD_NAME "," G_FILE_ATTRIBUTE_STANDARD_TYPE,
					        0,
					        gth_task_get_cancellable (GTH_TASK (self)),
					        &error);

	if (error != NULL) {
		cleanup_and_terminate (self, error);
		return;
	}

	files = NULL;
	while ((error == NULL) && ((info = g_file_enumerator_next_file (enumerator, NULL, &error)) != NULL)) {
		const char *name;
		GFile      *source;
		GFile      *destination;

		if (g_file_info_get_file_type (info) == G_FILE_TYPE_DIRECTORY) {
			g_object_unref (info);
			continue;
		}

		name = g_file_info_get_name (info);

		if ((strcmp (name, "index.gthtml") == 0)
		    || (strcmp (name, "thumbnail.gthtml") == 0)
		    || (strcmp (name, "image.gthtml") == 0))
		{
			g_object_unref (info);
			continue;
		}

		source = g_file_get_child (self->priv->style_dir, name);
		files = g_list_prepend (files, source);

		g_object_unref (source);
		g_object_unref (info);
	}

	g_object_unref (enumerator);

	if (error == NULL)
		_g_copy_files_async (files,
				     self->priv->target_tmp_dir,
				     FALSE,
				     G_FILE_COPY_NONE,
				     G_PRIORITY_NORMAL,
				     gth_task_get_cancellable (GTH_TASK (self)),
				     save_other_files_progress_cb,
				     self,
				     save_other_files_dialog_cb,
				     self,
				     save_other_files_ready_cb,
				     self);
	else
		cleanup_and_terminate (self, error);

	_g_object_list_unref (files);
}


static gboolean save_thumbnail (gpointer data);


static void
save_next_thumbnail (GthWebExporter *self)
{
	self->priv->current_image = self->priv->current_image->next;
	self->priv->image++;
	self->priv->saving_timeout = g_idle_add (save_thumbnail, data);
}


static void
save_thumbnail_ready_cb (GthFileData *file_data,
		         GError      *error,
			 gpointer     data)
{
	GthWebExporter *self = data;
	ImageData      *image_data;

	if (error != NULL) {
		cleanup_and_terminate (self, error);
		return;
	}

	image_data = self->priv->current_image->data;
	g_object_unref (image_data->thumb);
	image_data->thumb = NULL;

	save_next_thumbnail (self);
}


static gboolean
save_thumbnail (gpointer data)
{
	GthWebExporter *self = data;
	ImageData      *image_data;

	if (self->priv->saving_timeout != 0) {
		g_source_remove (self->priv->saving_timeout);
		self->priv->saving_timeout = 0;
	}

	if (self->priv->current_image == NULL) {
		save_other_files (self);
		return FALSE;
	}

	image_data = self->priv->current_image->data;
	if (image_data->thumb != NULL) {
		GFile       *destination;
		GthFileData *file_data;

		gth_task_progress (GTH_TASK (self),
				   _("Saving thumbnails"),
				   NULL,
				   FALSE,
				   (double) (self->priv->image + 1) / (self->priv->n_images + 1));

		destination = get_thumbnail_file (self, image_data, self->priv->target_tmp_dir);
		file_data = gth_file_data_new (destination, NULL);
		_gdk_pixbuf_save_async (image_data->thumb,
					file_data,
					"image/jpeg",
					TRUE,
					save_thumbnail_ready_cb,
					self);

		g_object_unref (file_data);
		g_object_unref (destination);
	}

	save_next_thumbnail (self);

	return FALSE;
}


static void
save_thumbnails (GthWebExporter *self)
{
	gth_task_progress (GTH_TASK (self), _("Saving thumbnails"), NULL, TRUE, 0);

	self->priv->image = 0;
	self->priv->current_image = self->priv->file_list;
	self->priv->saving_timeout = g_idle_add (save_thumbnail, self);
}


static gboolean
save_html_image (gpointer data)
{
	GthWebExporter *self = data;
	ImageData      *image_data;
	GFile          *file;
	GFile          *relative_to;

	if (self->priv->saving_timeout != 0) {
		g_source_remove (self->priv->saving_timeout);
		self->priv->saving_timeout = 0;
	}

	if (self->priv->current_image == NULL) {
		save_thumbnails (self);
		return FALSE;
	}

	gth_task_progress (GTH_TASK (self),
			   _("Saving HTML pages: Images"),
			   NULL,
			   FALSE,
			   (double) (self->priv->image + 1) / (self->priv->n_images + 1));

	image_data = self->priv->current_image->data;
	file = get_html_image_file (self, image_data, self->priv->target_tmp_dir);
	relative_to = get_html_image_dir (self, self->priv->target_dir);
	gfile_parsed_doc_print (self, self->priv->image_parsed, file, relative_to);

	g_object_unref (file);
	g_object_unref (relative_to);

	/**/

	self->priv->current_image = self->priv->current_image->next;
	self->priv->image++;
	self->priv->saving_timeout = g_idle_add (save_html_image, data);

	return FALSE;
}


static void
save_html_images (GthWebExporter *self)
{
	/*exporter_set_info (self, _("Saving HTML pages: Images")); FIXME */

	self->priv->image = 0;
	self->priv->current_image = self->priv->file_list;
	self->priv->saving_timeout = g_idle_add (save_html_image, self);
}


static gboolean
save_html_index (gpointer data)
{
	GthWebExporter *self = data;
	GFile          *file;
	GFile          *relative_to;

	if (self->priv->saving_timeout != 0) {
		g_source_remove (self->priv->saving_timeout);
		self->priv->saving_timeout = 0;
	}

	if (self->priv->page >= self->priv->n_pages) {
		save_html_images (self);
		return FALSE;
	}

	/* write index.html and pageXXX.html */

	gth_task_progress (GTH_TASK (self),
			   _("Saving HTML pages: Indexes"),
			   NULL,
			   FALSE,
			   (double) (self->priv->page + 1) / (self->priv->n_pages + 1));

	file = get_html_index_file (self, self->priv->page, self->priv->target_tmp_dir);
	relative_to = get_html_index_dir (self, self->priv->page, self->priv->target_dir);
	gfile_parsed_doc_print (self, self->priv->index_parsed, file, relative_to);

	g_object_unref (file);
	g_object_unref (relative_to);

	/**/

	self->priv->page++;
	self->priv->saving_timeout = g_idle_add (save_html_index, data);

	return FALSE;
}


static void
save_html_files (GthWebExporter *self)
{
	gth_task_progress (GTH_TASK (self),
			   _("Saving HTML pages: Indexes"),
			   NULL,
			   TRUE,
			   0);

	self->priv->image = 0;
	self->priv->eval_image = NULL;
	self->priv->page = 0;
	self->priv->saving_timeout = g_idle_add (save_html_index, self);
}


static void load_current_file (GthWebExporter *self);


static void
load_next_file (GthWebExporter *self)
{
	if (self->priv->interrupted) {
		GError *error;

		error = g_error_new_literal (G_IO_ERROR, G_IO_ERROR_CANCELLED, "");
		cleanup_and_terminate (self, error);
		g_error_free (error);

		return;
	}

	if (self->priv->current_file != NULL) {
		ImageData *image_data = self->priv->current_file->data;

		if (image_data->preview != NULL) {
			g_object_unref (image_data->preview);
			image_data->preview = NULL;
		}

		if (image_data->image != NULL) {
			g_object_unref (image_data->image);
			image_data->image = NULL;
		}
	}

	self->priv->n_images_done++;
	self->priv->current_file = self->priv->current_file->next;
	load_current_file (self);
}


static gboolean
load_next_file_cb (gpointer data)
{
	GthWebExporter *self = data;

	if (self->priv->saving_timeout != 0) {
		g_source_remove (self->priv->saving_timeout);
		self->priv->saving_timeout = 0;
	}

	load_next_file (self);

	return FALSE;
}


static void
save_image_preview_ready_cb (GthFileData *file_data,
			     GError      *error,
			     gpointer     data)
{
	GthWebExporter *self = data;

	if (error != NULL) {
		cleanup_and_terminate (self, error);
		return;
	}

	self->priv->saving_timeout = g_idle_add (load_next_file_cb, self);
}


static gboolean
save_image_preview (gpointer data)
{
	GthWebExporter *self = data;
	ImageData      *image_data;

	if (self->priv->saving_timeout != 0) {
		g_source_remove (self->priv->saving_timeout);
		self->priv->saving_timeout = 0;
	}

	image_data = self->priv->current_file->data;
	if (! image_data->no_preview && (image_data->preview != NULL)) {
		GFile       *destination;
		GthFileData *file_data;

		/* exporter_set_info (self, _("Saving images")); FIXME */

		destination = get_preview_file (self, image_data, self->priv->target_tmp_dir);
		file_data = gth_file_data_new (destination, NULL);
		_gdk_pixbuf_save_async (image_data->preview,
					file_data,
					"image/jpeg",
					TRUE,
					save_image_preview_ready_cb,
					self);

		g_object_unref (file_data);
		g_object_unref (destination);
	}
	else
		self->priv->saving_timeout = g_idle_add (load_next_file_cb, self);

	return FALSE;
}


static void
save_resized_image_ready_cd (GthFileData *file_data,
			     GError      *error,
			     gpointer     data)
{
	GthWebExporter *self = data;

	if (error != NULL) {
		cleanup_and_terminate (self, error);
		return;
	}

	self->priv->saving_timeout = g_idle_add (save_image_preview, self);
}


static gboolean
save_resized_image (gpointer data)
{
	GthWebExporter *self = data;
	ImageData      *image_data;

	if (self->priv->saving_timeout != 0) {
		g_source_remove (self->priv->saving_timeout);
		self->priv->saving_timeout = 0;
	}

	image_data = self->priv->current_file->data;
	if (self->priv->copy_images && (image_data->image != NULL)) {
		GFile       *destination;
		GthFileData *file_data;

		/* exporter_set_info (self, _("Saving images")); FIXME */

		destination = get_image_file (self, image_data, self->priv->target_tmp_dir);
		file_data = gth_file_data_new (destination, NULL);
		_gdk_pixbuf_save_async (image_data->image,
					file_data,
					"image/jpeg",
					TRUE,
					save_resized_image_ready_cd,
					self);

		g_object_unref (file_data);
		g_object_unref (destination);
	}
	else
		self->priv->saving_timeout = g_idle_add (save_image_preview, self);

	return FALSE;
}


static gboolean
copy_current_image (GthWebExporter *self)
{
	ImageData  *image_data;
	GFile      *destination;

	if (self->priv->saving_timeout != 0) {
		g_source_remove (self->priv->saving_timeout);
		self->priv->saving_timeout = 0;
	}

	/* This function is used when "Copy originals to destination" is
	   enabled, and resizing is NOT enabled. This allows us to use a
	   lossless copy (and rotate). When resizing is enabled, a lossy
	   save has to be used. */

	/* exporter_set_info (self, _("Copying original images")); FIXME */

	image_data = self->priv->current_file->data;
	destination = get_image_file (self, image_data, self->priv->target_tmp_dir);
	if (g_file_copy (image_data->file_data->file,
			 destination,
			 G_FILE_COPY_NONE,
			 gth_task_get_cancellable (GTH_TASK (self)),
			 NULL,
			 NULL,
			 &error))
	{
		if (g_content_type_equals (gth_file_data_get_mime_type (image_data->file_data), "image/jpeg")) {
			/* FIXME

			char *uri;
			uri = gfile_get_uri (dfile);

			GthTransform  transform;

			FileData *fd;
			fd = file_data_new (uri);
			transform = get_orientation_from_fd (fd);

			if (transform > 1) {
				apply_transformation_jpeg (fd,
							   transform,
							   JPEG_MCU_ACTION_TRIM,
							   NULL);
			}

			file_data_unref (fd);
			g_free (uri);

			*/
		}

		self->priv->saving_timeout = g_idle_add (save_image_preview, self);
	}
	else
		cleanup_and_terminate (self, error);

	g_object_unref (destination);

	return FALSE;
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
	if (dest == NULL)
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
image_loader_ready_cb (GthImageLoader *iloader,
		       GError         *error,
		       gpointer        data)
{
	GthWebExporter *self = data;
	ImageData      *idata;
	GdkPixbuf      *pixbuf;

	if (error != NULL) {
		load_next_file (self);
		return;
	}

	idata = (ImageData *) self->priv->current_file->data;

	/* image */

	idata->image = pixbuf = g_object_ref (gth_image_loader_get_pixbuf (iloader));
	if (self->priv->copy_images && self->priv->resize_images) {
		int w = gdk_pixbuf_get_width (pixbuf);
		int h = gdk_pixbuf_get_height (pixbuf);
		if (scale_keeping_ratio (&w, &h,
				         self->priv->resize_max_width,
				         self->priv->resize_max_height,
				         FALSE))
		{
			GdkPixbuf *scaled;

			scaled = pixbuf_scale (pixbuf, w, h, GDK_INTERP_BILINEAR);
			g_object_unref (idata->image);
			idata->image = scaled;
		}
	}

	idata->image_width = gdk_pixbuf_get_width (idata->image);
	idata->image_height = gdk_pixbuf_get_height (idata->image);

	/* preview */

	idata->preview = pixbuf = g_object_ref (gth_image_loader_get_pixbuf (iloader));
	if ((self->priv->preview_max_width > 0) && (self->priv->preview_max_height > 0)) {
		int w = gdk_pixbuf_get_width (pixbuf);
		int h = gdk_pixbuf_get_height (pixbuf);

		if (scale_keeping_ratio_min (&w, &h,
					     self->priv->preview_min_width,
					     self->priv->preview_min_height,
					     self->priv->preview_max_width,
					     self->priv->preview_max_height,
					     FALSE))
		{
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

	if (idata->no_preview && (idata->preview != NULL)) {
		g_object_unref (idata->preview);
		idata->preview = NULL;
	}

	/* thumbnail. */

	idata->thumb = pixbuf = image_loader_get_pixbuf (iloader);
	g_object_ref (idata->thumb);

	if ((self->priv->thumb_width > 0) && (self->priv->thumb_height > 0)) {
		int w = gdk_pixbuf_get_width (pixbuf);
		int h = gdk_pixbuf_get_height (pixbuf);

		if (scale_keeping_ratio (&w, &h,
					 self->priv->thumb_width,
					 self->priv->thumb_height,
					 FALSE))
		{
			GdkPixbuf *scaled;

			scaled = pixbuf_scale (pixbuf, w, h, GDK_INTERP_BILINEAR);
			g_object_unref (idata->thumb);
			idata->thumb = scaled;
		}
	}

	idata->thumb_width = gdk_pixbuf_get_width (idata->thumb);
	idata->thumb_height = gdk_pixbuf_get_height (idata->thumb);

	/* save the image */

	if (self->priv->copy_images) {
		if (self->priv->resize_images) {
			/* exporter_set_info (self, _("Saving images")); FIXME */
			self->priv->saving_timeout = g_idle_add (save_resized_image, self);
		}
		else
			copy_current_image (self);
	}
	else
		self->priv->saving_timeout = g_idle_add (save_image_preview, self);
}


static void
load_current_file (GthWebExporter *self)
{
	GthFileData *file_data;

	if (self->priv->current_file == NULL) {
		/* FIXME
		if ((self->priv->sort_method != GTH_SORT_METHOD_NONE)
		    && (self->priv->sort_method != GTH_SORT_METHOD_MANUAL))
			self->priv->file_list = g_list_sort (self->priv->file_list, get_sortfunc (self));
		*/
		if (self->priv->sort_type == GTK_SORT_DESCENDING)
			self->priv->file_list = g_list_reverse (self->priv->file_list);
		save_html_files (self);

		return;
	}

	file_data = IMAGE_DATA (self->priv->current_file->data)->file_data;
	gth_task_progress (GTH_TASK (self),
			   _("Loading images"),
			   g_file_info_get_display_name (file_data->info),
			   FALSE,
			   (double) (self->priv->n_images_done + 1) / (self->priv->n_images + 1));
	gth_image_loader_set_file_data (self->priv->iloader, file_data);
	gth_image_loader_start (self->priv->iloader);
}


static GList *
parse_template (GFile *file)
{
	GList  *result = NULL;
	GError *error = NULL;

	yy_parsed_doc = NULL;
	yy_istream = g_file_read (file, NULL, &error);
	if (error == NULL) {
		if (yyparse () == 0)
			result = yy_parsed_doc;
		else
			debug (DEBUG_INFO, "<<syntax error>>");

		g_input_stream_close (G_INPUT_STREAM (yy_istream), NULL, &error);
		g_object_unref (yy_istream);
	}
	else {
		g_warning ("%s", error->message);
		g_clear_error (&error);
	}

	return result;
}


static void
parse_theme_files (GthWebExporter *self)
{
	GFile *template;
	GList *scan;

	free_parsed_docs (self);

	self->priv->image = 0;

	/* read and parse index.gthtml */

	template = g_file_get_child (self->priv->style_dir, "index.gthtml");
	self->priv->index_parsed = parse_template (template);
	if (self->priv->index_parsed == NULL) {
		GthTag *tag = gth_tag_new (GTH_TAG_TABLE, NULL);
		self->priv->index_parsed = g_list_prepend (NULL, tag);
	}
	g_object_unref (template);

	/* read and parse thumbnail.gthtml */

	template = g_file_get_child (self->priv->style_dir, "thumbnail.gthtml");
	self->priv->thumbnail_parsed = parse_template (template);
	if (self->priv->thumbnail_parsed == NULL) {
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
		self->priv->thumbnail_parsed = g_list_prepend (NULL, tag);
	}
	g_object_unref (template);

	/* Read and parse image.gthtml */

	template = g_file_get_child (self->priv->style_dir, "image.gthtml");
	self->priv->image_parsed = parse_template (template);
	if (self->priv->image_parsed == NULL) {
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
		self->priv->image_parsed = g_list_prepend (NULL, tag);
	}
	g_object_unref (template);

	/* read index.html and set variables. */

	for (scan = self->priv->index_parsed; scan; scan = scan->next) {
		GthTag *tag = scan->data;
		int     width, height;

		switch (tag->type) {
		case GTH_TAG_SET_VAR:

			width = gth_tag_get_var (self, tag, "thumbnail_width");
			height = gth_tag_get_var (self, tag, "thumbnail_height");

			if ((width != 0) && (height != 0)) {
				debug (DEBUG_INFO, "thumbnail --> %dx%d", width, height);
				gth_web_exporter_set_thumb_size (self, width, height);
				break;
			}

			/**/

			width = gth_tag_get_var (self, tag, "preview_width");
			height = gth_tag_get_var (self, tag, "preview_height");

			if ((width != 0) && (height != 0)) {
				debug (DEBUG_INFO, "preview --> %dx%d", width, height);
				gth_web_exporter_set_preview_size (self, width, height);
				break;
			}

			width = gth_tag_get_var (self, tag, "preview_min_width");
			height = gth_tag_get_var (self, tag, "preview_min_height");

			if ((width != 0) && (height != 0)) {
				debug (DEBUG_INFO, "preview min --> %dx%d", width, height);
				gth_web_exporter_set_preview_min_size (self, width, height);
				break;
			}

			break;

		default:
			break;
		}
	}
}


static gboolean
make_album_dir (GFile       *parent,
		const char  *child_name,
		GError     **error)
{
	GFile    *child;
	gboolean  result;

	child = g_file_get_child (parent, child_name);
	result = g_file_make_directory (child, NULL, error);

	g_object_unref (child);

	return result;
}


static void
gth_web_exporter_exec (GthTask *task)
{
	GthWebExporter *self;
	GthFileData    *file_data;
	GError         *error = NULL;

	g_return_if_fail (GTH_IS_WEB_EXPORTER (object));

	self = GTH_WEB_EXPORTER (task);

	if (self->priv->file_list == NULL) {
		cleanup_and_terminate (self, NULL);
		return;
	}

	self->priv->n_images = g_list_length (self->priv->file_list);
	if (! self->priv->single_index) {
		self->priv->n_pages = self->priv->n_images / (self->priv->page_rows * self->priv->page_cols);
		if (self->priv->n_images % (self->priv->page_rows * self->priv->page_cols) > 0)
			self->priv->n_pages++;
	}
	else
		self->priv->n_pages = 1;

	/*
	 * check that the style directory is not NULL. A NULL indicates that
	 * the folder of the selected style has been deleted or renamed
	 * before the user started the export. It is unlikely.
	 */

	if (self->priv->style_dir == NULL) { /* FIXME */
		error = g_error_new_literal (GTHUMB_ERROR, GTH_ERROR_GENERIC, _("Could not find the style folder"));
		cleanup_and_terminate (self, error);
		return;
	}

	/* get index file name and subdirs from gconf (hidden prefs) */

	self->priv->index_file = eel_gconf_get_string (PREF_EXP_WEB_INDEX_FILE, DEFAULT_INDEX_FILE);
	self->priv->directories.previews = eel_gconf_get_string (PREF_EXP_WEB_DIR_PREVIEWS, DEFAULT_WEB_DIR_PREVIEWS);
	self->priv->directories.thumbnails = eel_gconf_get_string (PREF_EXP_WEB_DIR_THUMBNAILS, DEFAULT_WEB_DIR_THUMBNAILS);
	self->priv->directories.images = eel_gconf_get_string (PREF_EXP_WEB_DIR_IMAGES, DEFAULT_WEB_DIR_IMAGES);
	self->priv->directories.html_images = eel_gconf_get_string (PREF_EXP_WEB_DIR_HTML_IMAGES, DEFAULT_WEB_DIR_HTML_IMAGES);
	self->priv->directories.html_indexes = eel_gconf_get_string (PREF_EXP_WEB_DIR_HTML_INDEXES, DEFAULT_WEB_DIR_HTML_INDEXES);
	self->priv->directories.theme_files = eel_gconf_get_string (PREF_EXP_WEB_DIR_THEME_FILES, DEFAULT_WEB_DIR_THEME_FILES);

	/* get tmp dir */

	self->priv->target_tmp_dir = gfile_get_temp_dir_name (); /* FIXME */
	if (self->priv->target_tmp_dir == NULL) {
		error = g_error_new_literal (GTHUMB_ERROR, GTH_ERROR_GENERIC, _("Could not create a temporary folder"));
		cleanup_and_terminate (self, error);
		return;
	}

	if (self->priv->use_subfolders) {
		if (! make_album_dir (self->priv->target_tmp_dir, self->priv->directories.previews, &error)) {
			cleanup_and_terminate (self, error);
			return;
		}
		if (! make_album_dir (self->priv->target_tmp_dir, self->priv->directories.thumbnails, &error)) {
			cleanup_and_terminate (self, error);
			return;
		}
		if (self->priv->copy_images && ! make_album_dir (self->priv->target_tmp_dir, self->priv->directories.images, &error)) {
			cleanup_and_terminate (self, error);
			return;
		}
		if (! make_album_dir (self->priv->target_tmp_dir, self->priv->directories.html_images, &error)) {
			cleanup_and_terminate (self, error);
			return;
		}
		if ((self->priv->n_pages > 1) && ! make_album_dir (self->priv->target_tmp_dir, self->priv->directories.html_indexes, &error)) {
			cleanup_and_terminate (self, error);
			return;
		}
		if (! make_album_dir (self->priv->target_tmp_dir, self->priv->directories.theme_files, &error)) {
			cleanup_and_terminate (self, error);
			return;
		}
	}

	parse_theme_files (self);

	/* Load thumbnails. */

	self->priv->iloader = gth_image_loader_new (FALSE);
	g_signal_connect (G_OBJECT (self->priv->iloader),
			  "ready",
			  G_CALLBACK (image_loader_ready_cb),
			  self);

	self->priv->n_images_done = 0;
	self->priv->current_file = self->priv->file_list;
	load_current_file (self);
}


static void
gth_web_exporter_finalize (GObject *object)
{
	GthWebExporter *self;

	g_return_if_fail (GTH_IS_WEB_EXPORTER (object));

	self = GTH_WEB_EXPORTER (object);
	g_free (self->priv->header);
	g_free (self->priv->footer);
	_g_object_unref (self->priv->style_dir);
	_g_object_unref (self->priv->target_dir);
	_g_object_unref (self->priv->target_tmp_dir);

	g_free (self->priv->directories.previews);
	g_free (self->priv->directories.thumbnails);
	g_free (self->priv->directories.images);
	g_free (self->priv->directories.html_images);
	g_free (self->priv->directories.html_indexes);
	g_free (self->priv->directories.theme_files);

	g_free (self->priv->index_file);
	g_free (self->priv->info);
	if (self->priv->file_list != NULL) {
		g_list_foreach (self->priv->file_list, (GFunc) image_data_free, NULL);
		g_list_free (self->priv->file_list);
	}
	_g_object_unref (self->priv->iloader);
	free_parsed_docs (self);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gth_web_exporter_class_init (GthWebExporterClass *klass)
{
	GObjectClass *object_class;
	GthTaskClass *task_class;

	parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GthWebExporterPrivate));

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = gth_web_exporter_finalize;

	task_class = GTH_TASK_CLASS (klass);
	task_class->exec = gth_web_exporter_exec;
}


static void
gth_web_exporter_init (GthWebExporter *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_WEB_EXPORTER, GthWebExporterPrivate);

	self->priv->header = NULL;
	self->priv->footer = NULL;
	self->priv->style_dir = NULL;

	self->priv->base_tmp_dir = NULL;
	self->priv->target_tmp_dir = NULL;

	self->priv->use_subfolders = TRUE;
	self->priv->directories.previews = NULL;
	self->priv->directories.thumbnails = NULL;
	self->priv->directories.images = NULL;
	self->priv->directories.html_images = NULL;
	self->priv->directories.html_indexes = NULL;
	self->priv->directories.theme_files = NULL;

	self->priv->index_file = g_strdup (DEFAULT_INDEX_FILE);
	self->priv->file_list = NULL;
	self->priv->iloader = NULL;

	self->priv->thumb_width = DEFAULT_THUMB_SIZE;
	self->priv->thumb_height = DEFAULT_THUMB_SIZE;

	self->priv->copy_images = FALSE;
	self->priv->resize_images = FALSE;
	self->priv->resize_max_width = 0;
	self->priv->resize_max_height = 0;

	self->priv->single_index = FALSE;

	self->priv->preview_min_width = 0;
	self->priv->preview_min_height = 0;
	self->priv->preview_max_width = 0;
	self->priv->preview_max_height = 0;

	self->priv->index_caption_mask = GTH_CAPTION_IMAGE_DIM | GTH_CAPTION_FILE_SIZE;
	self->priv->image_caption_mask = GTH_CAPTION_COMMENT | GTH_CAPTION_PLACE | GTH_CAPTION_EXIF_DATE_TIME;
}


GType
gth_web_exporter_get_type (void)
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (GthWebExporterClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_web_exporter_class_init,
			NULL,
			NULL,
			sizeof (GthWebExporter),
			0,
			(GInstanceInitFunc) gth_web_exporter_init
                };

                type = g_type_register_static (GTH_TYPE_TASK,
					       "GthWebExporter",
					       &type_info,
					       0);
        }

	return type;
}


GthTask *
gth_web_exporter_new (GthBrowser *browser,
		      GList      *file_list)
{
	GthWebExporter *self;
	GList          *scan;
	int             file_idx;

	g_return_val_if_fail (window != NULL, NULL);

	self = (GthWebExporter *) g_object_new (GTH_TYPE_WEB_EXPORTER, NULL);

	self->priv->browser = browser;

	file_idx = 0;
	for (scan = file_list; scan; scan = scan->next) {
		GthFileData *file = scan->data;
		self->priv->file_list = g_list_prepend (self->priv->file_list, image_data_new (file, file_idx++));
	}
	self->priv->file_list = g_list_reverse (self->priv->file_list);

	return (GthTask *) self;
}


void
gth_web_exporter_set_header (GthWebExporter *self,
			     const char     *header)
{
	g_return_if_fail (GTH_IS_WEB_EXPORTER (self));
	g_free (self->priv->header);
	self->priv->header = g_strdup (header);
}


void
gth_web_exporter_set_footer (GthWebExporter *self,
			     const char     *footer)
{
	g_return_if_fail (GTH_IS_WEB_EXPORTER (self));
	g_free (self->priv->footer);
	self->priv->footer = g_strdup (footer);
}


void
gth_web_exporter_set_style (GthWebExporter *self,
			    const char     *style)
{
	g_return_if_fail (GTH_IS_WEB_EXPORTER (self));
	UNREF (self->priv->style_dir);
	self->priv->style_dir = gfile_get_style_dir (self, style);
}


void
gth_web_exporter_set_location (GthWebExporter *self,
			       const char     *location)
{
	g_return_if_fail (GTH_IS_WEB_EXPORTER (self));
	UNREF (self->priv->target_dir);
	self->priv->target_dir = gfile_new (location);
}


void
gth_web_exporter_set_use_subfolders (GthWebExporter *self,
				     gboolean        use_subfolders)
{
	g_return_if_fail (GTH_IS_WEB_EXPORTER (self));
	self->priv->use_subfolders = use_subfolders;
}


void
gth_web_exporter_set_copy_images (GthWebExporter *self,
				  gboolean        copy)
{
	g_return_if_fail (GTH_IS_WEB_EXPORTER (self));
	self->priv->copy_images = copy;
}


void
gth_web_exporter_set_resize_images (GthWebExporter *self,
				    gboolean        resize,
				    int             max_width,
				    int             max_height)
{
	g_return_if_fail (GTH_IS_WEB_EXPORTER (self));
	self->priv->resize_images = resize;
	if (resize) {
		self->priv->resize_max_width = max_width;
		self->priv->resize_max_height = max_height;
	}
	else {
		self->priv->resize_max_width = 0;
		self->priv->resize_max_height = 0;
	}
}


void
gth_web_exporter_set_sorted (GthWebExporter *self,
			     GthSortMethod   method,
			     GtkSortType     sort_type)
{
	g_return_if_fail (GTH_IS_WEB_EXPORTER (self));
	self->priv->sort_method = method;
	self->priv->sort_type = sort_type;
}


void
gth_web_exporter_set_row_col (GthWebExporter *self,
			      int             rows,
			      int             cols)
{
	g_return_if_fail (GTH_IS_WEB_EXPORTER (self));
	self->priv->page_rows = rows;
	self->priv->page_cols = cols;
}


void
gth_web_exporter_set_single_index (GthWebExporter *self,
				   gboolean        single)
{
	g_return_if_fail (GTH_IS_WEB_EXPORTER (self));
	self->priv->single_index = single;
}


void
gth_web_exporter_set_thumb_size (GthWebExporter *self,
				 int             width,
				 int         	 height)
{
	g_return_if_fail (GTH_IS_WEB_EXPORTER (self));
	self->priv->thumb_width = width;
	self->priv->thumb_height = height;
}


void
gth_web_exporter_set_preview_size (GthWebExporter *self,
				   int             width,
				   int             height)
{
	g_return_if_fail (GTH_IS_WEB_EXPORTER (self));

	if (self->priv->copy_images
	    && self->priv->resize_images
	    && (self->priv->resize_max_width > 0)
	    && (self->priv->resize_max_height > 0))
	{
		if (width > self->priv->resize_max_width)
			width = self->priv->resize_max_width;
		if (height > self->priv->resize_max_height)
			height = self->priv->resize_max_height;
	}

	self->priv->preview_max_width = width;
	self->priv->preview_max_height = height;
}


void
gth_web_exporter_set_preview_min_size (GthWebExporter *self,
				       int             width,
				       int             height)
{
	g_return_if_fail (GTH_IS_WEB_EXPORTER (self));

	self->priv->preview_min_width = width;
	self->priv->preview_min_height = height;
}


void
gth_web_exporter_set_image_caption (GthWebExporter   *self,
				    GthCaptionFields  caption)
{
	g_return_if_fail (GTH_IS_WEB_EXPORTER (self));
	self->priv->image_caption_mask = caption;
}


guint16
gth_web_exporter_get_image_caption (GthWebExporter *self)
{
	return self->priv->image_caption_mask;
}


void
gth_web_exporter_set_index_caption (GthWebExporter   *self,
				    GthCaptionFields  caption)
{
	g_return_if_fail (GTH_IS_WEB_EXPORTER (self));
	self->priv->index_caption_mask = caption;
}


guint16
gth_web_exporter_get_index_caption (GthWebExporter *self)
{
	return self->priv->index_caption_mask;
}
