/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001-2005 Free Software Foundation, Inc.
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
#include <math.h>

#include <glib/gi18n.h>
#include <libgnomecanvas/libgnomecanvas.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <libgnomevfs/gnome-vfs-uri.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glade/glade.h>
#include <pango/pango-layout.h>
#include <gtk/gtkprintoperation.h>
#include <pango/pangocairo.h>

#include "glib-utils.h"
#include "gconf-utils.h"
#include "file-utils.h"
#include "image-viewer.h"
#include "image-loader.h"
#include "pixbuf-utils.h"
#include "comments.h"
#include "preferences.h"
#include "progress-dialog.h"
#include "gthumb-stock.h"

#define UCHAR (guchar*)
#define GLADE_FILE "gthumb_print.glade"
#define PARAGRAPH_SEPARATOR 0x2029
#define CANVAS_ZOOM 0.25
#define DEF_COMMENT_FONT "Sans 12"
#define DEF_PAPER_WIDTH  0.0
#define DEF_PAPER_HEIGHT 0.0
#define DEF_PAPER_SIZE   "A4"
#define DEF_PAPER_ORIENT GTK_PAGE_ORIENTATION_PORTRAIT
#define DEF_MARGIN_LEFT 20.0
#define DEF_MARGIN_RIGHT 20.0
#define DEF_MARGIN_TOP 30.0
#define DEF_MARGIN_BOTTOM 30.0
#define DEF_IMAGES_PER_PAGE 4
#define ACTIVE_THUMB_BRIGHTNESS (42.0 / 127.0)
#define THUMB_SIZE 128
#define DEFAULT_FONT_SIZE 10

#define gray50_width  1
#define gray50_height 5

#define DEF_IMAGE_SIZING 0
#define DEF_IMAGE_WIDTH 100
#define DEF_IMAGE_HEIGHT 100

static const GtkUnit print_units[] = { GTK_UNIT_MM, GTK_UNIT_INCH };
static const int image_resolution[] = { 72, 150, 300, 600 };
static const char *paper_sizes[] = {"A4", "USLetter", "USLegal", "Tabloid",
				    "Executive", "Postcard", "Custom"};
#define CUSTOM_PAPER_SIZE_IDX 6

#define MM_PER_INCH 25.4
#define POINTS_PER_INCH 72


static double
convert_to_points (double  value,
		   GtkUnit unit)
{
	double points = 0.0;

	switch (unit) {
		case GTK_UNIT_MM:
			points = value * POINTS_PER_INCH / MM_PER_INCH;
			break;
		case GTK_UNIT_INCH:
			points = value * POINTS_PER_INCH;
			break;
		default:
			points = 0.0;
			break;
	}

	return points;
}


static double
convert_from_points (double  points,
		     GtkUnit unit)
{
	double value = 0.0;

	switch (unit) {
		case GTK_UNIT_MM:
			value = points * MM_PER_INCH / POINTS_PER_INCH;
			break;
		case GTK_UNIT_INCH:
			value = points / POINTS_PER_INCH;
			break;
		default:
			value = 0.0;
			break;
	}

	return value;
}


static int
get_history_from_paper_size (const char *paper_size)
{
	int i;
	for (i = 0; i < G_N_ELEMENTS (paper_sizes); i++)
		if (strcmp (paper_size, paper_sizes[i]) == 0)
			return i;
	return 0;
}


static void
add_simulated_page (GnomeCanvas *canvas)
{
	GnomeCanvasItem *i;
	double x1, y1, x2, y2, width, height;

	gnome_canvas_get_scroll_region (canvas, &x1, &y1, &x2, &y2);
	width = x2 - x1;
	height = y2 - y1;

	i = gnome_canvas_item_new (
		GNOME_CANVAS_GROUP (gnome_canvas_root (canvas)),
		gnome_canvas_rect_get_type (),
		"x1",   	 0.0,
		"y1",   	 0.0,
		"x2",   	 width,
		"y2",   	 height,
		"fill_color",    "white",
		"outline_color", "black",
		"width_pixels",  1,
		NULL);
	gnome_canvas_item_lower_to_bottom (i);
	i = gnome_canvas_item_new (
		GNOME_CANVAS_GROUP (gnome_canvas_root (canvas)),
		gnome_canvas_rect_get_type (),
		"x1",   	 3.0,
		"y1",   	 3.0,
		"x2",   	 width + 3.0,
		"y2",   	 height + 3.0,
		"fill_color",    "black",
		NULL);
	gnome_canvas_item_lower_to_bottom (i);
}


static void
clear_canvas (GnomeCanvasGroup *group)
{
	GList *list = group->item_list;

	while (list) {
		gtk_object_destroy (GTK_OBJECT (list->data));
		list = group->item_list;
	}
}


static gboolean
orientation_is_portrait (GtkPageSetup *page_setup)
{
	return gtk_page_setup_get_orientation (page_setup) == GTK_PAGE_ORIENTATION_PORTRAIT;
}


typedef struct {
	char        *filename;
	char        *comment;
	int          pixbuf_width, pixbuf_height;
	GdkPixbuf   *thumbnail;
	GdkPixbuf   *thumbnail_active;
	double       width, height;
	double       scale_x, scale_y;
	double       trans_x, trans_y;
	int          rotate;
	double       zoom;
	double       min_x, min_y;
	double       max_x, max_y;
	double       comment_height;

	GnomeCanvasItem     *ci_image;
	GnomeCanvasItem     *ci_comment;

	gboolean     print_comment;
} ImageInfo;


static ImageInfo *
image_info_new (const char *filename)
{
	ImageInfo *image = g_new0(ImageInfo, 1);

	image->filename = g_strdup (filename);
	image->comment = NULL;
	image->thumbnail = NULL;
	image->thumbnail_active = NULL;
	image->width = 0.0;
	image->height = 0.0;
	image->scale_x = 0.0;
	image->scale_y = 0.0;
	image->trans_x = 0.0;
	image->trans_y = 0.0;
	image->rotate = 0;
	image->zoom = 0.0;
	image->min_x = 0.0;
	image->min_y = 0.0;
	image->max_x = 0.0;
	image->max_y = 0.0;
	image->comment_height = 0.0;
	image->print_comment = FALSE;

	return image;
}


static void
image_info_free (ImageInfo *image)
{
	g_return_if_fail (image != NULL);

	g_free (image->filename);
	g_free (image->comment);
	if (image->thumbnail != NULL)
		g_object_unref (image->thumbnail);
	if (image->thumbnail_active != NULL)
		g_object_unref (image->thumbnail_active);
	g_free (image);
}


typedef struct {
	int                  ref_count;

	GtkWidget           *canvas;
	GnomeCanvasItem    **pages;
	int                  n_pages, current_page;

	GtkPrintOperation    *print_operation;
	GtkPageSetup	     *page_setup;

	PangoFontDescription *comment_font_desc;
	PangoFont	     *font;
	PangoFontMap	     *fontmap;
	PangoContext         *context;

	double                paper_width;
	double                paper_height;
	double                paper_lmargin;
	double                paper_rmargin;
	double                paper_tmargin;
	double                paper_bmargin;

	gboolean              print_comments;
	gboolean              print_filenames;
	gboolean              portrait;

	gboolean              use_colors;
	gboolean	      is_preview;


	int                   images_per_page;
	int                   n_images;
	ImageInfo           **image_info;

	int                   auto_images_per_page;
	gboolean	      is_auto_sizing;
	double		      image_width;
	double		      image_height;
	int		      image_unit;
	int		      image_resolution;

	double                max_image_width;
	double		      max_image_height;
} PrintCatalogInfo;


static void
print_catalog_info_ref (PrintCatalogInfo *pci)
{
	g_return_if_fail (pci != NULL);
	pci->ref_count++;
}


static void
print_catalog_info_unref (PrintCatalogInfo *pci)
{
	g_return_if_fail (pci != NULL);
	g_return_if_fail (pci->ref_count > 0);

	pci->ref_count--;

	if (pci->ref_count == 0) {
		int i;

		if (pci->page_setup) {
			g_object_unref(pci->page_setup);
			pci->page_setup = NULL;
		}

		if (pci->print_operation) {
			g_object_unref (pci->print_operation);
			pci->print_operation = NULL;
		}

 		if (pci->font != NULL) {
 			g_object_unref (pci->font);
			pci->font = NULL;
		}

 		if (pci->context != NULL) {
 			g_object_unref (pci->context);
			pci->context = NULL;
		}

		pci->fontmap = NULL;

		if (pci->comment_font_desc) {
			pango_font_description_free (pci->comment_font_desc);
			pci->comment_font_desc = NULL;
		}

		for (i = 0; i < pci->n_images; i++)
			image_info_free (pci->image_info[i]);
		g_free (pci->image_info);

		g_free (pci);
	}
}


typedef struct {
	GladeXML      *gui;
	GtkWidget     *dialog;

	GtkWidget     *btn_close;
	GtkWidget     *btn_print;

	GtkWidget     *img_size_auto_radiobutton;
	GtkWidget     *img_size_auto_box;
	GtkWidget     *img_size_manual_radiobutton;
	GtkWidget     *img_size_manual_box;

	GtkWidget     *img_width_spinbutton;
	GtkWidget     *img_height_spinbutton;
	GtkWidget     *img_unit_optionmenu;

	GtkWidget     *unit_optionmenu;
	GtkWidget     *width_spinbutton;
	GtkWidget     *height_spinbutton;
	GtkWidget     *margin_left_spinbutton;
	GtkWidget     *margin_right_spinbutton;
	GtkWidget     *margin_top_spinbutton;
	GtkWidget     *margin_bottom_spinbutton;
	GtkWidget     *paper_size_optionmenu;
	GtkWidget     *images_per_page_optionmenu;
	GtkWidget     *next_page_button;
	GtkWidget     *prev_page_button;
	GtkWidget     *page_label;
	GtkWidget     *comment_fontpicker;
	GtkWidget     *print_comment_checkbutton;
	GtkWidget     *print_filename_checkbutton;
	GtkWidget     *comment_font_hbox;
	GtkWidget     *scale_image_box;
	GtkWidget     *resolution_optionmenu;

	GtkWindow	  *parent;

	GtkAdjustment     *adj;

	PrintCatalogInfo  *pci;
	ProgressDialog    *pd;

	int                current_image;
	ImageLoader       *loader;
	gboolean           interrupted;

	DoneFunc           done_func;
	gpointer           done_data;
} PrintCatalogDialogData;


/* called when the main dialog is closed. */
static void
print_catalog_destroy_cb (GtkWidget              *widget,
			  PrintCatalogDialogData *data)
{
	if (data->done_func != NULL) {
		(*data->done_func) (data->done_data);
	}

	g_object_unref (data->gui);
	print_catalog_info_unref (data->pci);
	progress_dialog_destroy (data->pd);
	g_object_unref (G_OBJECT (data->loader));
	g_free (data);
}

static PangoGlyphInfo
pango_font_get_glyph (PangoFont	    *font,
		      PangoLanguage *language,
		      const char    *ch)
{
	PangoAnalysis	   analysis;
	gunichar	   wc = g_utf8_get_char (ch);
	PangoGlyphInfo	   gl;
	PangoGlyphString  *glyphs = pango_glyph_string_new ();
	char		  *next_ch = g_utf8_next_char (ch);

	analysis.shape_engine = pango_font_find_shaper (font, language, wc);
	analysis.lang_engine = NULL;
	analysis.font = font;
	analysis.language = language;
	analysis.level = 0;
	analysis.extra_attrs = NULL;

	pango_shape (ch, next_ch - ch, &analysis, glyphs);

	gl = glyphs->glyphs[0];

	pango_glyph_string_free (glyphs);

	return gl;
}

static const char*
pci_get_next_line_to_print_delimiter (PrintCatalogInfo *pci,
				      double            max_width,
				      const char       *start,
				      double           *line_width)
{
	const char  *p = start;
	const char  *last_space = NULL;
	double	     last_space_width = 0.0;
	double       current_width = 0.0;

	PangoGlyphInfo space = pango_font_get_glyph (pci->font, pango_language_from_string ("en_US"), " ");

	while ( *p ) {
		gunichar ch;

		ch = g_utf8_get_char (p);

		if (ch == '\n' || ch == PARAGRAPH_SEPARATOR) {

			if (line_width != NULL)
				*line_width = max_width;

			return p;
		} else if (g_unichar_isspace (ch)) {
			current_width += (double)space.geometry.width / PANGO_SCALE;
			last_space = p;
			last_space_width = current_width;
		} else {
			PangoGlyphInfo gi;

			gi = pango_font_get_glyph (pci->font, NULL, p);
			current_width += (double)gi.geometry.width / PANGO_SCALE;
		}


		if (current_width > max_width) {

			if (last_space_width > 0) {
				if (line_width != NULL)
					*line_width = last_space_width;
				if (g_utf8_next_char(last_space)) {
					/* Move just past last space, so that the next line
					   doesn't have an odd indent, if possible. */
					return g_utf8_next_char(last_space);
				}
				else {
					/* If it isn't possible, just return the position
					   of the last space. */
					return last_space;
				}
			} else {/* otherwise, chop off at the current character */
				if (line_width != NULL)
					*line_width = max_width;

				return p;
			}
		}
		p = g_utf8_next_char (p);
	}

	if (line_width != NULL)
		*line_width = current_width;

	return p;
}

static GSList*
pango_font_get_glyphs (PangoFont       *font,
		       PangoLanguage   *language,
		       const char      *text,
		       const char      *text_end)
{

	PangoEngineShape  *shaper = NULL;
	PangoEngineShape  *last_shaper = NULL;
	const char	  *p, *start;
	gboolean	   finished = FALSE;
	GSList		  *slist = NULL;


	p = start = text;

	while (!finished) {
		gunichar wc;

		if (p < text_end) {
			wc = g_utf8_get_char (p);
			shaper = pango_font_find_shaper (font, language, wc);
		} else {
			finished = TRUE;
			shaper = NULL;
		}

		if (p > start && (finished || shaper != last_shaper)) {
			PangoAnalysis	   analysis;
			PangoGlyphString  *glyphs = pango_glyph_string_new();

			analysis.shape_engine = last_shaper;
			analysis.lang_engine = NULL;
			analysis.font = font;
			analysis.language = language;
			analysis.level = 0;
			analysis.extra_attrs = NULL;

			pango_shape (start, p - start, &analysis, glyphs);
			start = p;
			slist = g_slist_append (slist, glyphs);
		}

		if (!finished) {
			p = g_utf8_next_char (p);
			last_shaper = shaper;
		}
	}

	return slist;
}

static void
pci_get_text_extents (PrintCatalogInfo *pci,
		      double            max_width,
		      const char       *text,
		      const char       *text_end,
		      double           *width,
		      double           *height)
{
	const char *p;
	const char *end;
	int         paragraph_delimiter_index;
	int         next_paragraph_start;
	double	    line_width;
	int	    font_height;

	*width = 0.0;
	*height = 0.0;

	pango_find_paragraph_boundary (text, text_end - text, &paragraph_delimiter_index, &next_paragraph_start);

	end = text + paragraph_delimiter_index;
	font_height = pango_font_description_get_size (pci->comment_font_desc);

	p = text;
	line_width = 0;

	while (p < text_end) {
		gunichar wc = g_utf8_get_char (p);

		if (wc == '\n' || wc == PARAGRAPH_SEPARATOR || p == end) {
			*height += 1.2 * font_height / PANGO_SCALE;

		} else {

			const char *p1 = p;
			const char *e1 = NULL;

			while (p1 < end) {
				double line_width;
				e1 = pci_get_next_line_to_print_delimiter (pci, max_width, p1, &line_width);

				if (p1 == e1) {
					*width = 0.0;
					*height = 0.0;
					return;
				}

				*width = MAX (*width, line_width);
				*height += 1.2 * font_height / PANGO_SCALE;

				p1 = e1;
			}
		}

		p = p + next_paragraph_start;

		if (next_paragraph_start == 0)
			break;

		if (p < text_end) {
			pango_find_paragraph_boundary (p, text_end - p, &paragraph_delimiter_index, &next_paragraph_start);
			end = p + paragraph_delimiter_index;
		}
	}

	//g_message("font height: %f paragraph width: %f height: %f", (double)font_height / PANGO_SCALE, *width, *height);
}



static char *
construct_comment (PrintCatalogInfo *pci,
		   ImageInfo        *image)
{
	GString *s;
	char    *comment = NULL;

	s = g_string_new ("");
	if (pci->print_comments  && (image->comment != NULL)) {
		const gchar* end = NULL;
		g_utf8_validate (image->comment, -1, &end);
		if (end > image->comment)
			g_string_append_len (s, image->comment, end - image->comment);
	}

	if (pci->print_filenames) {
		const gchar* end = NULL;
		g_utf8_validate (image->filename, -1, &end);
		if (end > image->filename) {
			if (s->len > 0)
				g_string_append (s, "\n");
			g_string_append_len (s, image->filename, end - image->filename);
		}
	}

	if (s->len > 0) {
		comment = s->str;
		g_string_free (s, FALSE);
	}
	else
		g_string_free (s, TRUE);

	return comment;
}


static void
pci_print_line (PrintCatalogInfo  *pci,
		cairo_t		  *cr,
		const char	  *start,
		const char	  *end,
		double		   x,
		double		   y)
{

	GSList* slist = pango_font_get_glyphs (pci->font, pango_language_from_string ("en_US"), start, end);
	GSList* p = slist;

	cairo_move_to (cr, x, y);

	while (p) {
		PangoGlyphString* g = (PangoGlyphString*)p->data;
		int i=0;
		pango_cairo_show_glyph_string (cr, pci->font, g);
		while (i < g->num_glyphs) {
			x += (double)g->glyphs[i++].geometry.width / PANGO_SCALE;
		}
		pango_glyph_string_free (g);
		p = p->next;
		cairo_move_to (cr, x, y);
	}

	if (slist)
		g_slist_free (slist);
}


static gdouble
pci_print_paragraph (PrintCatalogInfo  *pci,
		     cairo_t	       *cr,
		     const char        *start,
		     const char        *end,
		     double             max_width,
		     double             x,
		     double             y)
{
	const char *p, *s;
	int font_size = pango_font_description_get_size(pci->comment_font_desc) / PANGO_SCALE;

	for (p = start; p < end;) {
		s = p;
		p = pci_get_next_line_to_print_delimiter (pci, max_width, s, NULL);

		if (p == s)
			return y;

		pci_print_line (pci, cr, s, p, x, y);
		y += 1.2 * font_size;
	}

	return y;
}


static void
pci_print_comment (PrintCatalogInfo *pci,
		   cairo_t	    *cr,
		   ImageInfo	    *image)
{

	char       *comment;
	const char *p;
	const char *end;

	double      x, y, width, height;
	double      printable_width;
	int         paragraph_delimiter_index;
	int         next_paragraph_start;
	char       *text_end;
	int	    font_height;

	comment = construct_comment (pci, image);
	if (comment == NULL)
 		return;

	p = comment;
	text_end = comment + strlen (comment);

	pci_get_text_extents (pci, pci->max_image_width, p, text_end, &width, &height);

	printable_width = pci->max_image_width;


	pango_find_paragraph_boundary (comment, -1,
				       &paragraph_delimiter_index,
				       &next_paragraph_start);

	end = comment + paragraph_delimiter_index;

	font_height = pango_font_description_get_size (pci->comment_font_desc);

	x = image->min_x + MAX (0, (printable_width - width) / 2) - pci->paper_lmargin;
	y = image->max_y - height + (double)font_height / PANGO_SCALE - pci->paper_tmargin;

	while (p < text_end) {
		gunichar wc = g_utf8_get_char (p);

		if (wc == '\n' || wc == PARAGRAPH_SEPARATOR)
			y += 1.2 * (double)font_height / PANGO_SCALE;
		else
			y = pci_print_paragraph (pci, cr, p, end, printable_width, x, y);

		p = p + next_paragraph_start;

		if (p < text_end) {
			pango_find_paragraph_boundary (p, -1, &paragraph_delimiter_index, &next_paragraph_start);
			end = p + paragraph_delimiter_index;
		}
	}

	g_free (comment);
}





static int catalog_rows[5] = {1, 2, 2, 4, 4};
static int catalog_cols[5] = {1, 1, 2, 2, 4};
#define IMAGE_SPACE 20


static double
_log2 (double x)
{
	return log(x) / log(2);
}


static void
show_current_page (PrintCatalogDialogData *data)
{
	int   i;
	char *txt;

	for (i = 0; i < data->pci->n_pages; i++)
		if (i == data->pci->current_page)
			gnome_canvas_item_show (GNOME_CANVAS_ITEM (data->pci->pages[i]));
		else
			gnome_canvas_item_hide (GNOME_CANVAS_ITEM (data->pci->pages[i]));
	gtk_widget_set_sensitive (data->next_page_button, data->pci->current_page < data->pci->n_pages - 1);
	gtk_widget_set_sensitive (data->prev_page_button, data->pci->current_page > 0);

	txt = g_strdup_printf (_("Page %d of %d"), data->pci->current_page + 1, data->pci->n_pages);
	gtk_label_set_text (GTK_LABEL (data->page_label), txt);
	g_free (txt);
}


static GdkPixbuf *
print__gdk_pixbuf_rotate (GdkPixbuf *pixbuf,
		    int        angle)
{
	GdkPixbuf *rotated = NULL;

	switch (angle) {
	case 0:
	default:
		rotated = pixbuf;
		g_object_ref (rotated);
		break;
	case 90:
		rotated = gdk_pixbuf_rotate_simple (pixbuf, GDK_PIXBUF_ROTATE_CLOCKWISE);
		break;
	case 180:
		rotated = gdk_pixbuf_rotate_simple (pixbuf, GDK_PIXBUF_ROTATE_UPSIDEDOWN);
		break;
	case 270:
		rotated = gdk_pixbuf_rotate_simple (pixbuf, GDK_PIXBUF_ROTATE_COUNTERCLOCKWISE);
		break;
	}

	return rotated;
}


static void
image_info_rotate (ImageInfo *image,
		   int        angle)
{
	GdkPixbuf *tmp_pixbuf;

	if (angle != 90 && angle != 180 && angle != 270)
		return;

	tmp_pixbuf = image->thumbnail;
	if (tmp_pixbuf) {
		image->thumbnail = print__gdk_pixbuf_rotate (tmp_pixbuf, angle);
		g_object_unref (tmp_pixbuf);
	}

	tmp_pixbuf = image->thumbnail_active;
	if (tmp_pixbuf) {
		image->thumbnail_active = print__gdk_pixbuf_rotate (tmp_pixbuf, angle);
		g_object_unref (tmp_pixbuf);
	}

	image->rotate = (image->rotate + angle) % 360;

	if (angle == 90 || angle == 270) {
		int tmp = image->pixbuf_width;
		image->pixbuf_width = image->pixbuf_height;
		image->pixbuf_height = tmp;
	}
}


static gboolean
is_parent (GnomeCanvasItem *item_parent,
	   GnomeCanvasItem *item)
{
	while ((item->parent != NULL) && (item->parent != item_parent))
		item = item->parent;
	return (item->parent == item_parent);
}


static ImageInfo*
get_first_image_on_current_page (PrintCatalogDialogData *data)
{
	PrintCatalogInfo *pci = data->pci;
	ImageInfo        *image = NULL;
	int i;

	for (i = 0; i < pci->n_images; i++)
		if (is_parent (pci->pages[pci->current_page], pci->image_info[i]->ci_image)) {
			image = pci->image_info[i];
			break;
		}

	return image;
}


static void
center_image (ImageInfo *image)
{
	gdouble x1, y1, w, h;

	w = image->max_x - image->min_x;
	h = image->max_y - image->min_y;
	x1 = image->min_x + (w - image->width * image->zoom) / 2;
	y1 = image->min_y + (h - image->comment_height - image->height * image->zoom) / 2;

	gnome_canvas_item_set (image->ci_image,
			       "x", x1,
			       "y", y1,
			       NULL);
}


/* called when the center button is clicked. */
static void
catalog_center_cb (GtkWidget              *widget,
		   PrintCatalogDialogData *data)
{
	ImageInfo *image;

	image = get_first_image_on_current_page (data);
	center_image (image);
}


static void
catalog_check_bounds (ImageInfo *image, double *x1, double *y1)
{
	*x1 = MAX (*x1, image->min_x);
	*x1 = MIN (*x1, image->max_x - image->width * image->zoom);
	*y1 = MAX (*y1, image->min_y);
	*y1 = MIN (*y1, image->max_y - image->comment_height - image->height * image->zoom);
}


static void
catalog_zoom_changed (GtkAdjustment          *adj,
		      PrintCatalogDialogData *data)
{
	gdouble     w, h;
	gdouble     x1, y1;
	ImageInfo  *image;

	image = get_first_image_on_current_page (data);

	image->zoom = adj->value / 100.0;
	w = image->width * image->zoom;
	h = image->height * image->zoom;

	gnome_canvas_item_set (image->ci_image,
			       "width",      w,
			       "width_set",  TRUE,
			       "height",     h,
			       "height_set", TRUE,
			       NULL);

	/* Keep image within page borders. */

	g_object_get (G_OBJECT (image->ci_image),
		      "x", &x1,
		      "y", &y1,
		      NULL);
	catalog_check_bounds (image, &x1, &y1);
	gnome_canvas_item_set (image->ci_image,
			       "x", x1,
			       "y", y1,
			       NULL);
}


static void
reset_zoom (PrintCatalogDialogData *data,
	    ImageInfo              *image)
{
	image->zoom = 1.0;
	g_signal_handlers_block_by_data (G_OBJECT (data->adj), data);
	gtk_adjustment_set_value (data->adj, 100.0);
	g_signal_handlers_unblock_by_data (G_OBJECT (data->adj), data);
}


static int
catalog_item_event (GnomeCanvasItem        *item,
		    GdkEvent               *event,
		    PrintCatalogDialogData *data)
{
	PrintCatalogInfo *pci = data->pci;
	static double     start_x, start_y;
	static double     img_start_x, img_start_y;
	static int        dragging = FALSE, moved = FALSE;
	double            x, y;
	GdkCursor        *fleur;
	ImageInfo        *image = NULL;
	double            iw, ih, factor, max_image_height;
	int               i;

	for (i = 0; i < pci->n_images; i++)
		if (item == pci->image_info[i]->ci_image) {
			image = pci->image_info[i];
			break;
		}

	if (image == NULL)
		return FALSE;

	x = event->button.x;
	y = event->button.y;

	switch (event->type) {
	case GDK_ENTER_NOTIFY:
		g_object_set (image->ci_image,
			      "pixbuf", image->thumbnail_active,
			      NULL);
		break;

	case GDK_LEAVE_NOTIFY:
		g_object_set (image->ci_image,
			      "pixbuf", image->thumbnail,
			      NULL);
		break;

	case GDK_BUTTON_PRESS:
		switch (event->button.button) {
		case 1:
			start_x = x;
			start_y = y;
			g_object_get (G_OBJECT (item),
				      "x", &img_start_x,
				      "y", &img_start_y,
				      NULL);

			fleur = gdk_cursor_new (GDK_FLEUR);
			gnome_canvas_item_grab (item,
						(GDK_POINTER_MOTION_MASK
						 | GDK_BUTTON_RELEASE_MASK),
						fleur,
						event->button.time);
			gdk_cursor_unref (fleur);
			dragging = TRUE;
			moved = FALSE;
			break;

		default:
			break;
		}
		break;

	case GDK_MOTION_NOTIFY:
		if (dragging && (event->motion.state & GDK_BUTTON1_MASK)) {
			double x1, y1;

			x1 = img_start_x + (x - start_x);
			y1 = img_start_y + (y - start_y);

			catalog_check_bounds (image, &x1, &y1);

			gnome_canvas_item_set (item,
					       "x", x1,
					       "y", y1,
					       NULL);
			moved = TRUE;
		}
		break;

	case GDK_BUTTON_RELEASE:
		gnome_canvas_item_ungrab (item, event->button.time);

		if (dragging && moved) {
			dragging = FALSE;
			moved = FALSE;
			break;
		}

		switch (event->button.button) {
		case 1:
			image_info_rotate (image, 90);

			max_image_height = pci->max_image_height - image->comment_height;

			reset_zoom (data, image);

			iw = (double) image->pixbuf_width;
			ih = (double) image->pixbuf_height;
			factor = MIN (pci->max_image_width / iw, max_image_height / ih);
			ih *= factor;
			iw *= factor;
			image->width = iw;
			image->height = ih;

			image->trans_x = image->min_x + ((pci->max_image_width - iw) / 2);
			image->trans_y = image->min_y + ((max_image_height - ih) / 2);

			g_object_set (image->ci_image,
				      "pixbuf",     image->thumbnail_active,
				      "x",          image->trans_x,
				      "y",          image->trans_y,
				      "width",      iw,
				      "width_set",  TRUE,
				      "height",     ih,
				      "height_set", TRUE,
				      "anchor",     GTK_ANCHOR_NW,
				      NULL);
			break;

		default:
			break;
		}

		dragging = FALSE;
		moved = FALSE;

	default:
		break;
	}

	return FALSE;
}


static void
add_catalog_preview (PrintCatalogDialogData *data,
		     gboolean          border)
{
	PrintCatalogInfo *pci = data->pci;
	double            w, h;
	double            lmargin, rmargin, tmargin, bmargin;
	double            max_w, max_h;
	GnomeCanvasGroup *root;
	int               layout_width;
	double            image_space_x = IMAGE_SPACE;
	double            image_space_y = IMAGE_SPACE;
	int               i, rows, cols, row, col, page = 0, idx;

	g_free (pci->pages);
	w = pci->paper_width;
	h = pci->paper_height;
	lmargin = pci->paper_lmargin;
	rmargin = pci->paper_rmargin;
	bmargin = pci->paper_bmargin;
	tmargin = pci->paper_tmargin;

	layout_width = (int) ((pci->paper_width - lmargin - rmargin) * CANVAS_ZOOM);

	max_w = w - lmargin - rmargin;
	max_h = h - bmargin - tmargin;

	//g_message("add_catalog_preview() w:%f h:%f lm:%f rm%f tm:%f bm:%f", w, h, lmargin, rmargin, tmargin, bmargin);

	if (pci->is_auto_sizing == FALSE) {
		double image_width = pci->image_width;
		double image_height = pci->image_height;

		double tmpcols = (int)floor((max_w + image_space_x) / (image_space_x + image_height ));
		double tmprows = (int)floor((max_h + image_space_y) / (image_space_y + image_width ));

		cols = (int)floor((max_w + image_space_x) / (image_space_x + image_width ));
		rows = (int)floor((max_h + image_space_y) / (image_space_y + image_height ));

		if (tmprows*tmpcols > cols*rows && image_height <= max_w && image_width <= max_w)
		{
			double tmp = image_width;
			image_width = image_height;
			image_height = tmp;
			rows = tmprows;
			cols = tmpcols;
		}

		if (rows == 0) {
			rows = 1;
			image_height = max_h - image_space_y;
		}

		if (cols == 0) {
			cols = 1;
			image_width = max_w - image_space_x;
		}

		pci->images_per_page = rows * cols;


		if (cols > 1)
			image_space_x = (max_w - cols * image_width) / (cols - 1);
		else
			image_space_x = max_w - image_width;

		if (rows > 1)
			image_space_y = (max_h - rows * image_height) / (rows - 1);
		else
			image_space_y = max_h - image_height;

		pci->max_image_width = image_width;
		pci->max_image_height = image_height;


	} else {
		pci->images_per_page = pci->auto_images_per_page;

		idx = (int) floor (_log2 (pci->images_per_page) + 0.5);
		rows = catalog_rows[idx];
		cols = catalog_cols[idx];

		if (!orientation_is_portrait (data->pci->page_setup)) {
			int tmp = rows;
			rows = cols;
			cols = tmp;
		}

		pci->max_image_width = (max_w - ((cols-1) * image_space_x)) / cols;
		pci->max_image_height = (max_h - ((rows-1) * image_space_y)) / rows;

	}


	pci->n_pages = MAX ((int) ceil ((double) pci->n_images / pci->images_per_page), 1);
	pci->pages = g_new0 (GnomeCanvasItem*, pci->n_pages);
	pci->current_page = 0;

 	root = GNOME_CANVAS_GROUP (gnome_canvas_root (GNOME_CANVAS (pci->canvas)));
	pci->pages[page] = gnome_canvas_item_new (root,
						  gnome_canvas_group_get_type (),
						  NULL);

	row = 1;
	col = 1;
	for (i = 0; i < pci->n_images; i++) {
		ImageInfo *image = pci->image_info[i];
		double     iw, ih;
		double     factor, max_image_height;
		char      *comment;

		image_info_rotate (image, (360 - image->rotate) % 360);

		if (((pci->max_image_width > pci->max_image_height)
		     && (image->pixbuf_width < image->pixbuf_height))
		    || ((pci->max_image_width < pci->max_image_height)
			&& (image->pixbuf_width > image->pixbuf_height)) )
			image_info_rotate (image, 270);

		reset_zoom (data, image);

		image->min_x = lmargin + ((col-1) * (pci->max_image_width + image_space_x));
		image->min_y = tmargin + ((row-1) * (pci->max_image_height + image_space_y));

		image->max_x = image->min_x + pci->max_image_width;
		image->max_y = image->min_y + pci->max_image_height;

		gnome_canvas_item_new (
		       GNOME_CANVAS_GROUP (pci->pages[page]),
		       gnome_canvas_rect_get_type (),
		       "x1",   	 image->min_x,
		       "y1",   	 image->min_y,
		       "x2",   	 image->min_x + pci->max_image_width,
		       "y2",   	 image->min_y + pci->max_image_height,
		       "outline_color", "gray",
		       "width_pixels",  1,
		       NULL);

		col++;
		if (col > cols) {
			row++;
			col = 1;
		}

		image->comment_height = 0.0;
		image->print_comment = FALSE;

		comment = construct_comment (pci, image);
		if (comment != NULL) {
			const char *p;
			const char *text_end;
			double      comment_width;

			p = comment;
			text_end = comment + strlen (comment);
			pci_get_text_extents (pci, pci->max_image_width, p, text_end, &comment_width, &(image->comment_height));
			//g_message ("comment height: %f width: %f", image->comment_height, comment_width);
			image->print_comment = image->comment_height < (pci->max_image_width * 0.66);
			if (image->print_comment) {
				static char gray50_bits[] = {
					0x0,
					0x0,
					0x1,
					0x1,
					0x1,
				};
				GdkBitmap  *stipple;

				stipple = gdk_bitmap_create_from_data (NULL, gray50_bits, gray50_width, gray50_height);
				image->ci_comment = gnome_canvas_item_new (
						   GNOME_CANVAS_GROUP (pci->pages[page]),
						   gnome_canvas_rect_get_type (),
						   "x1",             image->min_x,
						   "y1",             image->max_y,
						   "x2",             image->max_x,
						   "y2",             image->max_y - image->comment_height,
						   "fill_color",     "darkgray",
						   "fill_stipple",   stipple,
						   NULL);

				g_object_unref (stipple);
			}
		}

		g_free (comment);

		max_image_height = pci->max_image_height;
		if (image->print_comment)
			max_image_height -= image->comment_height;

		iw = (double) image->pixbuf_width;
		ih = (double) image->pixbuf_height;
		factor = MIN (pci->max_image_width / iw, max_image_height / ih);
		ih *= factor;
		iw *= factor;
		image->width = iw;
		image->height = ih;
		image->trans_x = image->min_x + ((pci->max_image_width - iw) / 2);
		image->trans_y = image->min_y + ((max_image_height - ih) / 2);

		if (image->thumbnail != NULL) {
			iw = MAX (1.0, iw);
			ih = MAX (1.0, ih);

			image->ci_image = gnome_canvas_item_new (
			      GNOME_CANVAS_GROUP (pci->pages[page]),
			      gnome_canvas_pixbuf_get_type (),
			      "pixbuf",     image->thumbnail,
			      "x",          image->trans_x,
			      "y",          image->trans_y,
			      "width",      iw,
			      "width_set",  TRUE,
			      "height",     ih,
			      "height_set", TRUE,
			      "anchor",     GTK_ANCHOR_NW,
			      NULL);

			if (image->ci_image == NULL)
				g_error ("Cannot create image preview\n");

			g_signal_connect (G_OBJECT (image->ci_image),
					  "event",
					  G_CALLBACK (catalog_item_event),
					  data);
		}

		if ((i + 1 < pci->n_images) && (i + 1) % pci->images_per_page == 0) {
			page++;
			col = 1;
			row = 1;
			pci->pages[page] = gnome_canvas_item_new (
					       root,
					       gnome_canvas_group_get_type (),
					       NULL);
		}
	}

	show_current_page (data);
}

static void
catalog_update_page (PrintCatalogDialogData *data)
{
	PrintCatalogInfo *pci = data->pci;

	pci->paper_width = gtk_page_setup_get_paper_width(pci->page_setup, GTK_UNIT_POINTS);
	pci->paper_height = gtk_page_setup_get_paper_height(pci->page_setup, GTK_UNIT_POINTS);
	pci->paper_lmargin = gtk_page_setup_get_left_margin(pci->page_setup, GTK_UNIT_POINTS);
	pci->paper_rmargin = gtk_page_setup_get_right_margin(pci->page_setup, GTK_UNIT_POINTS);
	pci->paper_tmargin = gtk_page_setup_get_top_margin(pci->page_setup, GTK_UNIT_POINTS);
	pci->paper_bmargin = gtk_page_setup_get_bottom_margin(pci->page_setup, GTK_UNIT_POINTS);
	pci->portrait = gtk_page_setup_get_orientation(pci->page_setup) == GTK_PAGE_ORIENTATION_PORTRAIT;

	pci->image_unit = print_units[gtk_option_menu_get_history (GTK_OPTION_MENU (data->img_unit_optionmenu))];
	pci->image_width = convert_to_points (gtk_spin_button_get_value (GTK_SPIN_BUTTON(data->img_width_spinbutton)), pci->image_unit);
	pci->image_height = convert_to_points (gtk_spin_button_get_value (GTK_SPIN_BUTTON(data->img_height_spinbutton)), pci->image_unit);
	pci->image_resolution = image_resolution[gtk_option_menu_get_history (GTK_OPTION_MENU(data->resolution_optionmenu))];

	clear_canvas (GNOME_CANVAS_GROUP (GNOME_CANVAS (pci->canvas)->root));
	gnome_canvas_set_scroll_region (GNOME_CANVAS (pci->canvas),
					0, 0,
					pci->paper_width, pci->paper_height);
	add_simulated_page (GNOME_CANVAS (pci->canvas));
	add_catalog_preview (data, TRUE);

	gtk_widget_set_sensitive (data->scale_image_box, (pci->images_per_page == 1) || (pci->n_images == 1));
}

static void
catalog_update_image_size (PrintCatalogDialogData *data)
{
	PrintCatalogInfo* pci = data->pci;

	pci->image_unit = print_units[gtk_option_menu_get_history (GTK_OPTION_MENU (data->img_unit_optionmenu))];
	pci->image_width = convert_to_points (gtk_spin_button_get_value (GTK_SPIN_BUTTON (data->img_width_spinbutton)), pci->image_unit);
	pci->image_height = convert_to_points (gtk_spin_button_get_value (GTK_SPIN_BUTTON (data->img_height_spinbutton)),
					       pci->image_unit);

	catalog_update_page (data);
}

static void
catalog_update_custom_page_size (PrintCatalogDialogData *data)
{
	GtkUnit unit;
	GtkPaperSize *size;
	double width, height;

	unit = print_units[gtk_option_menu_get_history (GTK_OPTION_MENU (data->unit_optionmenu))];
	width = gtk_spin_button_get_value (GTK_SPIN_BUTTON (data->width_spinbutton));
	height = gtk_spin_button_get_value (GTK_SPIN_BUTTON (data->height_spinbutton));

	size = gtk_paper_size_new_custom("customX", "customY", width, height, unit);
	gtk_page_setup_set_paper_size (data->pci->page_setup, size);
	g_free(size);

	catalog_update_page (data);
}


static double
catalog_get_image_width (PrintCatalogDialogData *data)
{
	return gtk_spin_button_get_value (GTK_SPIN_BUTTON (data->img_width_spinbutton));
}


static double
catalog_get_image_height (PrintCatalogDialogData *data)
{
	return gtk_spin_button_get_value (GTK_SPIN_BUTTON (data->img_height_spinbutton));
}

static GthPrintUnit
catalog_get_image_unit (PrintCatalogDialogData *data)
{
	return gtk_option_menu_get_history (GTK_OPTION_MENU (data->img_unit_optionmenu));
}

static double
catalog_get_page_width (PrintCatalogDialogData *data)
{
	GtkUnit unit;
	unit = print_units[gtk_option_menu_get_history (GTK_OPTION_MENU (data->unit_optionmenu))];
	return gtk_page_setup_get_paper_width (data->pci->page_setup, unit);
}


static double
catalog_get_page_height (PrintCatalogDialogData *data)
{
	GtkUnit unit;
	unit = print_units[gtk_option_menu_get_history (GTK_OPTION_MENU (data->unit_optionmenu))];
	return gtk_page_setup_get_paper_height (data->pci->page_setup, unit);
}


static double
catalog_get_page_left_margin (PrintCatalogDialogData *data)
{
	GtkUnit unit;
	unit = print_units[gtk_option_menu_get_history (GTK_OPTION_MENU (data->unit_optionmenu))];
	return gtk_page_setup_get_left_margin (data->pci->page_setup, unit);
}

static double
catalog_get_page_right_margin (PrintCatalogDialogData *data)
{
	GtkUnit unit;
	unit = print_units[gtk_option_menu_get_history (GTK_OPTION_MENU (data->unit_optionmenu))];
	return gtk_page_setup_get_right_margin (data->pci->page_setup, unit);
}

static double
catalog_get_page_top_margin (PrintCatalogDialogData *data)
{
	GtkUnit unit;
	unit = print_units[gtk_option_menu_get_history (GTK_OPTION_MENU (data->unit_optionmenu))];
	return gtk_page_setup_get_top_margin (data->pci->page_setup, unit);
}

static double
catalog_get_page_bottom_margin (PrintCatalogDialogData *data)
{
	GtkUnit unit;
	unit = print_units[gtk_option_menu_get_history (GTK_OPTION_MENU (data->unit_optionmenu))];
	return gtk_page_setup_get_bottom_margin (data->pci->page_setup, unit);
}

static void catalog_margin_value_changed_cb (GtkSpinButton          *spin,
					     PrintCatalogDialogData *data);

static void catalog_custom_size_value_changed_cb (GtkSpinButton          *spin,
						  PrintCatalogDialogData *data);

static void
catalog_update_page_size_from_config (PrintCatalogDialogData *data)
{
	double len;
	GtkUnit unit;

	g_signal_handlers_block_by_func (data->width_spinbutton, catalog_custom_size_value_changed_cb, data);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->width_spinbutton), catalog_get_page_width (data));
	g_signal_handlers_unblock_by_func (data->width_spinbutton, catalog_custom_size_value_changed_cb, data);

	/**/

	g_signal_handlers_block_by_func (data->height_spinbutton, catalog_custom_size_value_changed_cb, data);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->height_spinbutton), catalog_get_page_height (data));
	g_signal_handlers_unblock_by_func (data->height_spinbutton, catalog_custom_size_value_changed_cb, data);

	/**/

	g_signal_handlers_block_by_func (data->margin_left_spinbutton, catalog_margin_value_changed_cb, data);
	g_signal_handlers_block_by_func (data->margin_right_spinbutton, catalog_margin_value_changed_cb, data);
	g_signal_handlers_block_by_func (data->margin_top_spinbutton, catalog_margin_value_changed_cb, data);
	g_signal_handlers_block_by_func (data->margin_bottom_spinbutton, catalog_margin_value_changed_cb, data);


	unit = print_units[gtk_option_menu_get_history (GTK_OPTION_MENU (data->unit_optionmenu))];

	len = gtk_page_setup_get_left_margin (data->pci->page_setup, unit);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->margin_left_spinbutton), len);

	len = gtk_page_setup_get_right_margin (data->pci->page_setup, unit);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->margin_right_spinbutton), len);

	len = gtk_page_setup_get_top_margin (data->pci->page_setup, unit);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->margin_top_spinbutton), len);

	len = gtk_page_setup_get_bottom_margin (data->pci->page_setup, unit);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->margin_bottom_spinbutton), len);

	if (unit == GTK_UNIT_MM) {
		gtk_spin_button_set_increments (GTK_SPIN_BUTTON (data->margin_left_spinbutton), 1, 10);
		gtk_spin_button_set_digits (GTK_SPIN_BUTTON (data->margin_left_spinbutton), 0);
		gtk_spin_button_set_increments (GTK_SPIN_BUTTON (data->margin_right_spinbutton), 1, 10);
		gtk_spin_button_set_digits (GTK_SPIN_BUTTON (data->margin_right_spinbutton), 0);
		gtk_spin_button_set_increments (GTK_SPIN_BUTTON (data->margin_top_spinbutton), 1, 10);
		gtk_spin_button_set_digits (GTK_SPIN_BUTTON (data->margin_top_spinbutton), 0);
		gtk_spin_button_set_increments (GTK_SPIN_BUTTON (data->margin_bottom_spinbutton), 1, 10);
		gtk_spin_button_set_digits (GTK_SPIN_BUTTON (data->margin_bottom_spinbutton), 0);
	} else {
		gtk_spin_button_set_increments (GTK_SPIN_BUTTON (data->margin_left_spinbutton), .1, 1);
		gtk_spin_button_set_digits (GTK_SPIN_BUTTON (data->margin_left_spinbutton), 1);
		gtk_spin_button_set_increments (GTK_SPIN_BUTTON (data->margin_right_spinbutton), .1, 1);
		gtk_spin_button_set_digits (GTK_SPIN_BUTTON (data->margin_right_spinbutton), 1);
		gtk_spin_button_set_increments (GTK_SPIN_BUTTON (data->margin_top_spinbutton), .1, 1);
		gtk_spin_button_set_digits (GTK_SPIN_BUTTON (data->margin_top_spinbutton), 1);
		gtk_spin_button_set_increments (GTK_SPIN_BUTTON (data->margin_bottom_spinbutton), .1, 1);
		gtk_spin_button_set_digits (GTK_SPIN_BUTTON (data->margin_bottom_spinbutton), 1);
	}


	g_signal_handlers_unblock_by_func (data->margin_left_spinbutton, catalog_margin_value_changed_cb, data);
	g_signal_handlers_unblock_by_func (data->margin_right_spinbutton, catalog_margin_value_changed_cb, data);
	g_signal_handlers_unblock_by_func (data->margin_top_spinbutton, catalog_margin_value_changed_cb, data);
	g_signal_handlers_unblock_by_func (data->margin_bottom_spinbutton, catalog_margin_value_changed_cb, data);
}


static void catalog_image_width_changed_cb (GtkSpinButton          *spin,
					    PrintCatalogDialogData *data);

static void catalog_image_height_changed_cb (GtkSpinButton          *spin,
					     PrintCatalogDialogData *data);

static void
catalog_update_image_size_from_config (PrintCatalogDialogData *data)
{
	PrintCatalogInfo* pci = data->pci;
	GtkUnit unit;
	double max_width, max_height;
	double width, height;

	g_signal_handlers_block_by_func (data->img_width_spinbutton, catalog_image_width_changed_cb, data);
	g_signal_handlers_block_by_func (data->img_height_spinbutton, catalog_image_height_changed_cb, data);

	unit = print_units[gtk_option_menu_get_history (GTK_OPTION_MENU (data->img_unit_optionmenu))];
	width = convert_from_points (pci->image_width, unit);
	height = convert_from_points (pci->image_height, unit);

	max_width = gtk_page_setup_get_paper_width(pci->page_setup, unit) -
		gtk_page_setup_get_left_margin(pci->page_setup, unit) -
		gtk_page_setup_get_right_margin(pci->page_setup, unit);

	max_height = gtk_page_setup_get_paper_height(pci->page_setup, unit) -
		gtk_page_setup_get_top_margin(pci->page_setup, unit) -
		gtk_page_setup_get_bottom_margin(pci->page_setup, unit);

	//g_message("catalog_update_image_size_from_config() mw:%f mh:%f w:%f h:%f", max_width, max_height, width, height);

	if (unit == GTK_UNIT_MM) {
		gtk_spin_button_set_increments (GTK_SPIN_BUTTON (data->img_width_spinbutton), 1, 10);
		gtk_spin_button_set_increments (GTK_SPIN_BUTTON (data->img_height_spinbutton), 1, 10);
	} else {
		gtk_spin_button_set_increments (GTK_SPIN_BUTTON (data->img_width_spinbutton), .1, 1);
		gtk_spin_button_set_increments (GTK_SPIN_BUTTON (data->img_height_spinbutton), .1, 1);
	}


	if (width > max_width)
		width = max_width;
	if (height > max_height)
		height = max_height;

	gtk_spin_button_set_range (GTK_SPIN_BUTTON (data->img_width_spinbutton), 1, max_width);
	gtk_spin_button_set_range (GTK_SPIN_BUTTON (data->img_height_spinbutton), 1, max_height);

	gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->img_width_spinbutton), width);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->img_height_spinbutton), height);

	g_signal_handlers_unblock_by_func (data->img_width_spinbutton, catalog_image_width_changed_cb, data);
	g_signal_handlers_unblock_by_func (data->img_height_spinbutton, catalog_image_height_changed_cb, data);

}


static void
catalog_set_standard_page_size (PrintCatalogDialogData *data,
				const char             *page_size)
{
	//const GnomePrintUnit *unit;
	GtkPaperSize* size;
	GtkUnit unit;
	double width, height;

	if (strcmp (page_size, "A4") == 0) {
		unit = GTK_UNIT_MM;
		width = 210.0;
		height = 297.0;

	} else if (strcmp (page_size, "USLetter") == 0) {
		unit = GTK_UNIT_INCH;
		width = 8.5;
		height = 11.0;

	} else if (strcmp (page_size, "USLegal") == 0) {
		unit = GTK_UNIT_INCH;
		width = 8.5;
		height = 14.0;

	} else if (strcmp (page_size, "Tabloid") == 0) {
		unit = GTK_UNIT_INCH;
		width = 11.0;
		height = 17.0;

	} else if (strcmp (page_size, "Executive") == 0) {
		unit = GTK_UNIT_INCH;
		width = 7.25;
		height = 10.50;

	} else if (strcmp (page_size, "Postcard") == 0) {
		unit = GTK_UNIT_MM;
		width = 99.8;
		height = 146.8;

	} else
		return;

	size = gtk_paper_size_new_custom (page_size, "", width, height, unit);
	gtk_page_setup_set_paper_size (data->pci->page_setup, size);
	g_free (size);

	catalog_update_page_size_from_config (data);
	catalog_update_image_size_from_config (data);
	catalog_update_page (data);
}


static void
catalog_update_margins (PrintCatalogDialogData *data)
{
	GtkUnit unit;
	double len;

	unit = print_units[gtk_option_menu_get_history (GTK_OPTION_MENU (data->unit_optionmenu))];

	/* left margin */

	len = gtk_spin_button_get_value (GTK_SPIN_BUTTON (data->margin_left_spinbutton));
	gtk_page_setup_set_left_margin (data->pci->page_setup, len, unit);

	/* right margin */

	len = gtk_spin_button_get_value (GTK_SPIN_BUTTON (data->margin_right_spinbutton));
	gtk_page_setup_set_right_margin (data->pci->page_setup, len, unit);

	/* top margin */

	len = gtk_spin_button_get_value (GTK_SPIN_BUTTON (data->margin_top_spinbutton));
	gtk_page_setup_set_top_margin (data->pci->page_setup, len, unit);

	/* bottom margin */

	len = gtk_spin_button_get_value (GTK_SPIN_BUTTON (data->margin_bottom_spinbutton));
	gtk_page_setup_set_bottom_margin (data->pci->page_setup, len, unit);

	catalog_update_image_size_from_config (data);
	catalog_update_page (data);
}

static void
done_print (GtkPrintOperation       *operation,
	    GtkPrintOperationResult  result,
	    PrintCatalogInfo	    *pci)
{
	print_catalog_info_unref (pci);
}

static void
begin_print (GtkPrintOperation *operation,
	     GtkPrintContext   *context,
	     PrintCatalogInfo  *pci)
{
	gtk_print_operation_set_n_pages (operation, (pci->n_images + pci->images_per_page - 1) / pci->images_per_page);
	gtk_print_operation_set_default_page_setup(operation, pci->page_setup);
	gtk_print_operation_set_show_progress(operation, TRUE);

}

static gboolean
preview (GtkPrintOperation        *operation,
	 GtkPrintOperationPreview *preview,
	 GtkPrintContext          *context,
	 GtkWindow                *parent,
	 PrintCatalogInfo	  *pci)
{
	pci->is_preview = TRUE;
	return FALSE;
}

static void
draw_page (GtkPrintOperation *operation,
	   GtkPrintContext   *context,
	   gint               page_nr,
	   PrintCatalogInfo  *pci)
{
	double		width, height;
	gboolean	border = FALSE;
	cairo_t	       *cr;
	int		i;

	width = pci->paper_width - pci->paper_lmargin - pci->paper_rmargin;
	height = pci->paper_height - pci->paper_tmargin - pci->paper_bmargin;
	cr = gtk_print_context_get_cairo_context (context);
	//g_message (" %f x %f l:%f r:%f t:%f d:%f", width, height, pci->paper_lmargin, pci->paper_rmargin, pci->paper_tmargin, pci->paper_bmargin );

	if (border) {
		cairo_set_source_rgb (cr, 0, 0, 0);
		cairo_move_to (cr, 0, 0);
		cairo_line_to (cr, width, 0);
		cairo_line_to (cr, width, height);
		cairo_line_to (cr, 0, height);
		cairo_line_to (cr, 0, 0);
		cairo_stroke (cr);
	}

	for (i = page_nr * pci->images_per_page; i < pci->n_images && i < (page_nr + 1) * pci->images_per_page; i++) {

		ImageInfo *image = pci->image_info[i];
		GdkPixbuf *image_pixbuf, *pixbuf = NULL;
		double scale_factor;

		if (pci->print_comments || pci->print_filenames) {
			cairo_save (cr);
			pci_print_comment (pci, cr, image);
			cairo_restore (cr);
		}

		image_pixbuf = gth_pixbuf_new_from_uri (image->filename, NULL, 0, 0, NULL);

		pixbuf = print__gdk_pixbuf_rotate (image_pixbuf, image->rotate);
		g_object_unref (image_pixbuf);

		/* For higher-resolution images, cairo will render the bitmaps at a miserable
		   72 dpi unless we apply a scaling factor. This scaling boosts the output
		   to 300 dpi (if required). */
		scale_factor = MIN (image->pixbuf_width / image->scale_x, (double)pci->image_resolution/72);

		image_pixbuf = pixbuf;
		pixbuf = gdk_pixbuf_scale_simple (image_pixbuf, image->scale_x * scale_factor,
						  image->scale_y * scale_factor, GDK_INTERP_BILINEAR);
		g_object_unref (image_pixbuf);


		if (pixbuf != NULL) {
			guchar		 *p;
			guchar		 *np;
			int		  pw, ph, rs, i;
			cairo_surface_t  *s;
			cairo_pattern_t	 *pattern;
			cairo_matrix_t    matrix;

			p = gdk_pixbuf_get_pixels (pixbuf);
			pw = gdk_pixbuf_get_width (pixbuf);
			ph = gdk_pixbuf_get_height (pixbuf);
			rs = gdk_pixbuf_get_rowstride (pixbuf);

 			if (gdk_pixbuf_get_has_alpha (pixbuf)) {
				guchar* kk;
				guchar* kp;

				np = g_malloc (pw*ph*4);
				for (i=0; i<ph; i++) {
					int j = 0;
					kk = p + rs*i;
					kp = np + pw*4*i;
					for (j=0; j<pw; j++) {
						if (kk[3] == 0) {
							*((unsigned int *)kp) = 0;
						}
						else {
							if (kk[3] != 0xff) {
								int t = (kk[3] * kk[0]) + 0x80;
								kk[0] = ((t+(t>>8))>>8);
								t = (kk[3] * kk[1]) + 0x80;
								kk[1] = ((t+(t>>8))>>8);
								t = (kk[3] * kk[2]) + 0x80;
								kk[2] = ((t+(t>>8))>>8);
							}
							*((unsigned int *)kp) = kk[2] + (kk[1] << 8) + (kk[0] << 16) + (kk[3] << 24);
						}
						kk += 4;
						kp += 4;
					}
				}

				s = cairo_image_surface_create_for_data (np, CAIRO_FORMAT_ARGB32, pw, ph, pw*4);
			}
			else {
				guchar* kk;
				guchar* kp;

				np = g_malloc (pw*ph*4);

				for (i=0; i<ph; i++) {
					int j = 0;
					kk = p + rs*i;
					kp = np + pw*4*i;
					for (j=0; j<pw; j++) {
						*((unsigned int *)kp) = kk[2] + (kk[1] << 8) + (kk[0] << 16);
						kk += 3;
						kp += 4;
					}
				}
				s = cairo_image_surface_create_for_data (np, CAIRO_FORMAT_RGB24, pw, ph, pw*4);
			}
			cairo_save (cr);
			cairo_rectangle (cr, image->trans_x - pci->paper_lmargin, image->trans_y - pci->paper_tmargin,
					 image->scale_x, image->scale_y);
			cairo_clip (cr);
			pattern = cairo_pattern_create_for_surface (s);
			cairo_matrix_init_translate (&matrix,
						     (pci->paper_lmargin - image->trans_x) * scale_factor,
						     (pci->paper_tmargin - image->trans_y) * scale_factor);
			cairo_matrix_scale (&matrix, scale_factor, scale_factor);
			cairo_pattern_set_matrix (pattern, &matrix);
			cairo_pattern_set_extend (pattern, CAIRO_EXTEND_NONE);
			cairo_pattern_set_filter (pattern, CAIRO_FILTER_BEST);
			cairo_set_source (cr, pattern);
			cairo_paint (cr);
			cairo_pattern_destroy (pattern);
			cairo_surface_destroy (s);
			g_free (np);
			cairo_restore (cr);
			g_object_unref (pixbuf);
		}
	}
}

static const char *
catalog_get_current_paper_size (PrintCatalogDialogData *data)
{
	return paper_sizes[gtk_option_menu_get_history (GTK_OPTION_MENU (data->paper_size_optionmenu))];
}


static GthPrintUnit
catalog_get_current_unit (PrintCatalogDialogData *data)
{
	return gtk_option_menu_get_history (GTK_OPTION_MENU (data->unit_optionmenu));
}

static GthImageResolution
catalog_get_current_image_resolution (PrintCatalogDialogData* data)
{
	return gtk_option_menu_get_history (GTK_OPTION_MENU (data->resolution_optionmenu));
}

static GthImageSizing
catalog_get_current_image_sizing (PrintCatalogDialogData* data)
{
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->img_size_auto_radiobutton)))
		return GTH_IMAGE_SIZING_AUTO;
	else
		return GTH_IMAGE_SIZING_MANUAL;
}

static void
pci_update_comment_font (PrintCatalogDialogData *data)
{
	PrintCatalogInfo *pci = data->pci;
	const char       *font_name;

	font_name = gtk_font_button_get_font_name (GTK_FONT_BUTTON (data->comment_fontpicker));

	debug (DEBUG_INFO, "Font name: %s", font_name);

	pci->comment_font_desc = pango_font_description_from_string(font_name);

	if (pci->fontmap == NULL)
	{
		pci->fontmap = pango_cairo_font_map_get_default ();
		pango_cairo_font_map_set_resolution (PANGO_CAIRO_FONT_MAP (pci->fontmap), 72);
		pci->context = pango_cairo_font_map_create_context (PANGO_CAIRO_FONT_MAP (pci->fontmap));
	}

	if (pci->font)
		g_object_unref(pci->font);

	pci->font = pango_font_map_load_font (pci->fontmap, pci->context, pci->comment_font_desc);
}


static void
catalog_image_width_changed_cb (GtkSpinButton          *spin,
				PrintCatalogDialogData *data)
{
	catalog_update_image_size (data);
}

static void
catalog_image_height_changed_cb (GtkSpinButton          *spin,
				 PrintCatalogDialogData *data)
{
	catalog_update_image_size (data);
}

static void
catalog_paper_size_optionmenu_changed_cb (GtkOptionMenu          *opt,
					  PrintCatalogDialogData *data)
{
	int history = gtk_option_menu_get_history (opt);

	if (history != CUSTOM_PAPER_SIZE_IDX)
		catalog_set_standard_page_size (data, paper_sizes[history]);
	else
		catalog_update_custom_page_size (data);
}


static void
catalog_custom_size_value_changed_cb (GtkSpinButton          *spin,
				      PrintCatalogDialogData *data)
{
	GtkOptionMenu *opt;

	opt = GTK_OPTION_MENU (data->paper_size_optionmenu);
	if (gtk_option_menu_get_history (opt) != CUSTOM_PAPER_SIZE_IDX) {
		g_signal_handlers_block_by_func (data->paper_size_optionmenu, catalog_paper_size_optionmenu_changed_cb, data);
		gtk_option_menu_set_history (opt, CUSTOM_PAPER_SIZE_IDX);
		g_signal_handlers_unblock_by_func (data->paper_size_optionmenu, catalog_paper_size_optionmenu_changed_cb, data);
	}

	catalog_update_custom_page_size (data);
}


static void
catalog_portrait_toggled_cb (GtkToggleButton        *widget,
			     PrintCatalogDialogData *data)
{
	if (! gtk_toggle_button_get_active (widget))
		return;
	gtk_page_setup_set_orientation (data->pci->page_setup, GTK_PAGE_ORIENTATION_PORTRAIT);
	catalog_update_image_size_from_config (data);
	catalog_update_page (data);
}


static void
catalog_landscape_toggled_cb (GtkToggleButton        *widget,
			      PrintCatalogDialogData *data)
{
	if (! gtk_toggle_button_get_active (widget))
		return;
	gtk_page_setup_set_orientation (data->pci->page_setup, GTK_PAGE_ORIENTATION_LANDSCAPE);
	catalog_update_image_size_from_config (data);
	catalog_update_page (data);
}


static void
catalog_margin_value_changed_cb (GtkSpinButton          *spin,
				 PrintCatalogDialogData *data)
{
	catalog_update_margins (data);
}

static void
catalog_image_unit_changed_cb (GtkOptionMenu          *opt_menu,
			       PrintCatalogDialogData *data)
{
	catalog_update_image_size_from_config (data);
}


static void
catalog_unit_changed_cb (GtkOptionMenu          *opt_menu,
			 PrintCatalogDialogData *data)
{
	catalog_update_page_size_from_config (data);
}


static void
catalog_image_resolution_changed_cb (GtkOptionMenu          *opt_menu,
				     PrintCatalogDialogData *data)
{
	data->pci->image_resolution = image_resolution[gtk_option_menu_get_history (opt_menu)];
}

static void
catalog_auto_size_toggled_cb (GtkToggleButton        *widget,
			     PrintCatalogDialogData *data)
{
	gboolean active = gtk_toggle_button_get_active (widget);
	gtk_widget_set_sensitive(data->img_size_auto_box, active);
	if (active) {
		data->pci->is_auto_sizing = TRUE;
		catalog_update_page (data);
	}
}


static void
catalog_manual_size_toggled_cb (GtkToggleButton        *widget,
			      PrintCatalogDialogData *data)
{
	gboolean active = gtk_toggle_button_get_active (widget);
	gtk_widget_set_sensitive(data->img_size_manual_box, active);
	if (active) {
		data->pci->is_auto_sizing = FALSE;
		catalog_update_page (data);
	}
}




static void
print_catalog_cb (GtkWidget              *widget,
		  PrintCatalogDialogData *data)
{
	PrintCatalogInfo  *pci = data->pci;
	double             length;
	int                value, i;

	GtkPrintOperationResult result;
	GError* error;

	/* Save options */

	eel_gconf_set_string (PREF_PRINT_PAPER_SIZE, catalog_get_current_paper_size (data));
	eel_gconf_set_integer (PREF_PRINT_IMAGES_PER_PAGE, pci->auto_images_per_page);

	eel_gconf_set_string (PREF_PRINT_COMMENT_FONT, gtk_font_button_get_font_name (GTK_FONT_BUTTON (data->comment_fontpicker)));

	eel_gconf_set_boolean (PREF_PRINT_INCLUDE_COMMENT,
			       !gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (data->print_comment_checkbutton)) &&
			       gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->print_comment_checkbutton)));

	eel_gconf_set_boolean (PREF_PRINT_INCLUDE_FILENAME,
			       !gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (data->print_filename_checkbutton)) &&
			       gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->print_filename_checkbutton)));

	length = catalog_get_page_width (data);
	eel_gconf_set_float (PREF_PRINT_PAPER_WIDTH, length);

	length = catalog_get_page_height (data);
	eel_gconf_set_float (PREF_PRINT_PAPER_HEIGHT, length);

	length = catalog_get_page_left_margin (data);
	eel_gconf_set_float (PREF_PRINT_MARGIN_LEFT, length);

	length = catalog_get_page_right_margin (data);
	eel_gconf_set_float (PREF_PRINT_MARGIN_RIGHT, length);

	length = catalog_get_page_top_margin (data);
	eel_gconf_set_float (PREF_PRINT_MARGIN_TOP, length);

	length = catalog_get_page_bottom_margin (data);
	eel_gconf_set_float (PREF_PRINT_MARGIN_BOTTOM, length);

	pref_set_print_unit (catalog_get_current_unit (data));

	value = gtk_page_setup_get_orientation (pci->page_setup);
	eel_gconf_set_integer (PREF_PRINT_PAPER_ORIENTATION, value);


	pref_set_image_resolution (catalog_get_current_image_resolution (data));
	pref_set_image_sizing (catalog_get_current_image_sizing (data));
	length = catalog_get_image_width (data);
	eel_gconf_set_float (PREF_PRINT_IMAGE_WIDTH, length);
	length = catalog_get_image_height (data);
	eel_gconf_set_float (PREF_PRINT_IMAGE_HEIGHT, length);
	pref_set_image_unit (catalog_get_image_unit (data));

	/**/

	for (i = 0; i < pci->n_images; i++) {
		ImageInfo *image = pci->image_info[i];
		double     image_x, image_y;

		g_object_get (G_OBJECT (image->ci_image),
			      "x", &image_x,
			      "y", &image_y,
			      NULL);
		image->scale_x = image->width * image->zoom;
		image->scale_y = image->height * image->zoom;
		image->trans_x = image_x;
		image->trans_y = image_y;
	}

	/* pci is used to print, so we must add a reference. */

	print_catalog_info_ref (pci);
	gtk_widget_hide (data->dialog);


	/* GTK Printing */
	if (!pci->print_operation)
		pci->print_operation = gtk_print_operation_new ();

	gtk_print_operation_set_show_progress (pci->print_operation, TRUE);

	g_signal_connect (pci->print_operation, "begin_print", G_CALLBACK (begin_print), pci);
	g_signal_connect (pci->print_operation, "draw_page", G_CALLBACK (draw_page), pci);
	g_signal_connect (pci->print_operation, "done", G_CALLBACK (done_print), pci);
	g_signal_connect (pci->print_operation, "preview", G_CALLBACK (preview), pci);

	pci->is_preview = FALSE;

	result = gtk_print_operation_run (pci->print_operation, GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG, GTK_WINDOW (data->parent), &error);

	g_object_unref (pci->print_operation);
	pci->print_operation = NULL;

	if (result == GTK_PRINT_OPERATION_RESULT_ERROR)
	{
		GtkWidget* error_dialog = gtk_message_dialog_new (GTK_WINDOW (data->parent),
						       GTK_DIALOG_DESTROY_WITH_PARENT,
						       GTK_MESSAGE_ERROR,
						       GTK_BUTTONS_CLOSE,
						       "Printing error:\n%s",
						       error->message);
		g_signal_connect (error_dialog, "response",
				  G_CALLBACK (gtk_widget_destroy), NULL);
		gtk_widget_show (error_dialog);
		g_error_free (error);
	}
	else if (result == GTK_PRINT_OPERATION_RESULT_APPLY)
	{
		/* printing completed successfully  */
	}
	else if (result == GTK_PRINT_OPERATION_RESULT_CANCEL)
	{
		gtk_widget_show (data->dialog);
		return;
	}
	else {
		/* print in progress */
	}

	if (pci->is_preview)
		gtk_widget_show (data->dialog);
	else
		gtk_widget_destroy (data->dialog);
}


static void
images_per_page_value_changed_cb (GtkOptionMenu           *option_menu,
				  PrintCatalogDialogData  *data)
{
	data->pci->auto_images_per_page = (int) pow (2, gtk_option_menu_get_history (option_menu));

	debug (DEBUG_INFO, "IPP: %d\n", data->pci->auto_images_per_page);

	catalog_update_page (data);
}


static void
next_page_cb (GtkWidget              *widget,
	      PrintCatalogDialogData *data)
{
	data->pci->current_page = MIN (data->pci->n_pages - 1, data->pci->current_page + 1);
	show_current_page (data);
}


static void
prev_page_cb (GtkWidget              *widget,
	      PrintCatalogDialogData *data)
{
	data->pci->current_page = MAX (0, data->pci->current_page - 1);
	show_current_page (data);
}


static void
load_current_image (PrintCatalogDialogData *data)
{
	const char *filename;
	char       *msg;

	if (data->current_image >= data->pci->n_images) {
		progress_dialog_hide (data->pd);
		catalog_update_page (data);
		gtk_widget_show (data->dialog);

		return;
	}

	progress_dialog_set_progress (data->pd, (double) data->current_image / data->pci->n_images);

	filename = data->pci->image_info[data->current_image]->filename;

	msg = g_strdup_printf (_("Loading image: %s"), file_name_from_path (filename));
	progress_dialog_set_info (data->pd, msg);
	g_free (msg);

	image_loader_set_path (data->loader, filename, NULL);

	image_loader_start (data->loader);
}


static void
do_colorshift (GdkPixbuf *dest,
	       GdkPixbuf *src,
	       int        shift)
{
	int     i, j;
	int     width, height, has_alpha, srcrowstride, destrowstride;
	guchar *target_pixels;
	guchar *original_pixels;
	guchar *pixsrc;
	guchar *pixdest;
	int     val;
	guchar  r,g,b;

	has_alpha       = gdk_pixbuf_get_has_alpha (src);
	width           = gdk_pixbuf_get_width (src);
	height          = gdk_pixbuf_get_height (src);
	srcrowstride    = gdk_pixbuf_get_rowstride (src);
	destrowstride   = gdk_pixbuf_get_rowstride (dest);
	target_pixels   = gdk_pixbuf_get_pixels (dest);
	original_pixels = gdk_pixbuf_get_pixels (src);

	for (i = 0; i < height; i++) {
		pixdest = target_pixels + i*destrowstride;
		pixsrc  = original_pixels + i*srcrowstride;
		for (j = 0; j < width; j++) {
			r            = *(pixsrc++);
			g            = *(pixsrc++);
			b            = *(pixsrc++);
			val          = r + shift;
			*(pixdest++) = CLAMP (val, 0, 255);
			val          = g + shift;
			*(pixdest++) = CLAMP (val, 0, 255);
			val          = b + shift;
			*(pixdest++) = CLAMP (val, 0, 255);

			if (has_alpha)
				*(pixdest++) = *(pixsrc++);
		}
	}
}


static void
image_loader_done_cb (ImageLoader            *il,
		      PrintCatalogDialogData *data)
{
	GdkPixbuf   *pixbuf = image_loader_get_pixbuf (il);
	ImageInfo   *image  = data->pci->image_info[data->current_image];
	CommentData *cdata;

	if (data->interrupted) {
		gtk_widget_destroy (data->dialog);
		return;
	}

	if (pixbuf != NULL) {
		int thumb_w, thumb_h;

		thumb_w = image->pixbuf_width = gdk_pixbuf_get_width (pixbuf);
		thumb_h = image->pixbuf_height = gdk_pixbuf_get_height (pixbuf);

		if (scale_keepping_ratio (&thumb_w, &thumb_h, THUMB_SIZE, THUMB_SIZE))
			image->thumbnail = gdk_pixbuf_scale_simple (pixbuf,
								    thumb_w,
								    thumb_h,
								    GDK_INTERP_BILINEAR);
		else {
			image->thumbnail = pixbuf;
			g_object_ref (image->thumbnail);
		}

		if (image->thumbnail != NULL) {
			image->thumbnail_active = gdk_pixbuf_copy (image->thumbnail);
			do_colorshift (image->thumbnail_active, image->thumbnail_active, 30);
		}
	}

	cdata = comments_load_comment (image->filename, TRUE);
	if (cdata != NULL) {
		image->comment = comments_get_comment_as_string (cdata, "\n", " - ");
		comment_data_free (cdata);
	}

	data->current_image++;
	load_current_image (data);
}


static void
image_loader_error_cb (ImageLoader            *il,
		       PrintCatalogDialogData *data)
{
	data->current_image++;
	load_current_image (data);
}


static void
cancel_image_loading (gpointer callback_data)
{
	PrintCatalogDialogData *data = callback_data;
	data->interrupted = TRUE;
}


static void
pci_comment_fontpicker_font_set_cb (GtkFontButton   *fontpicker,
				    PrintCatalogDialogData *data)
{
	pci_update_comment_font (data);
	catalog_update_page (data);
}


static void
pci_print_comments_cb (GtkWidget              *widget,
		       PrintCatalogDialogData *data)
{
	data->pci->print_comments = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));
	gtk_widget_set_sensitive (data->comment_font_hbox, data->pci->print_comments || data->pci->print_filenames);
	catalog_update_page (data);
}


static void
pci_print_filenames_cb (GtkWidget              *widget,
		        PrintCatalogDialogData *data)
{
	data->pci->print_filenames = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));
	gtk_widget_set_sensitive (data->comment_font_hbox, data->pci->print_comments || data->pci->print_filenames);
	catalog_update_page (data);
}


void
print_catalog_dlg_full (GtkWindow *parent,
			GList     *file_list,
			DoneFunc   done_func,
			gpointer   done_data)
{
	PrintCatalogInfo        *pci;
	PrintCatalogDialogData  *data;
	GList                   *scan;
	int                      n = 0, v;
	GtkWidget               *radio_button;
	GtkWidget               *print_notebook;
	GtkWidget               *comment_fontpicker_hbox;
	GtkWidget               *center_button;
	GtkWidget               *hscale;
	char                    *value;
	char                    *button_name;

	if (file_list == NULL)
		return;

	pci = g_new0 (PrintCatalogInfo, 1);
	pci->ref_count = 1;
	pci->page_setup = gtk_page_setup_new();
	pci->portrait = TRUE;
	pci->use_colors = TRUE;
	pci->auto_images_per_page = eel_gconf_get_integer (PREF_PRINT_IMAGES_PER_PAGE, DEF_IMAGES_PER_PAGE);

	pci->n_images = g_list_length (file_list);
	pci->image_info = g_new (ImageInfo*, pci->n_images);
	for (scan = file_list; scan; scan = scan->next) {
		char *filename = scan->data;
		pci->image_info[n++] = image_info_new (filename);
	}

	data = g_new0 (PrintCatalogDialogData, 1);
	data->pci = pci;
	data->gui = glade_xml_new (GTHUMB_GLADEDIR "/" GLADE_FILE, NULL, NULL);
        if (! data->gui) {
                g_warning ("Cannot find " GLADE_FILE "\n");
		print_catalog_info_unref (pci);
		g_free (data);
                return;
        }

	data->done_func = done_func;
	data->done_data = done_data;

	data->loader = IMAGE_LOADER (image_loader_new (NULL, FALSE));
	g_signal_connect (G_OBJECT (data->loader),
			  "image_done",
			  G_CALLBACK (image_loader_done_cb),
			  data);
	g_signal_connect (G_OBJECT (data->loader),
			  "image_error",
			  G_CALLBACK (image_loader_error_cb),
			  data);

	data->current_image = 0;

	/* Get the widgets. */

	data->dialog = glade_xml_get_widget (data->gui, "page_setup_dialog");
	data->img_size_auto_radiobutton = glade_xml_get_widget (data->gui, "img_size_auto_radiobutton");
	data->img_size_auto_box = glade_xml_get_widget (data->gui, "img_size_auto_box");
	data->img_size_manual_radiobutton = glade_xml_get_widget (data->gui, "img_size_manual_radiobutton");
	data->img_size_manual_box = glade_xml_get_widget (data->gui, "img_size_manual_box");
	data->img_width_spinbutton = glade_xml_get_widget (data->gui, "img_width_spinbutton");
	data->img_height_spinbutton = glade_xml_get_widget (data->gui, "img_height_spinbutton");
	data->img_unit_optionmenu = glade_xml_get_widget (data->gui, "img_unit_optionmenu");

	data->resolution_optionmenu = glade_xml_get_widget (data->gui, "resolution_optionmenu");
	data->unit_optionmenu = glade_xml_get_widget (data->gui, "unit_optionmenu");
	data->width_spinbutton = glade_xml_get_widget (data->gui, "width_spinbutton");
	data->height_spinbutton = glade_xml_get_widget (data->gui, "height_spinbutton");
	data->margin_left_spinbutton = glade_xml_get_widget (data->gui, "margin_left_spinbutton");
	data->margin_right_spinbutton = glade_xml_get_widget (data->gui, "margin_right_spinbutton");
	data->margin_top_spinbutton = glade_xml_get_widget (data->gui, "margin_top_spinbutton");
	data->margin_bottom_spinbutton = glade_xml_get_widget (data->gui, "margin_bottom_spinbutton");
	data->paper_size_optionmenu = glade_xml_get_widget (data->gui, "paper_size_optionmenu");
	data->images_per_page_optionmenu = glade_xml_get_widget (data->gui, "images_per_page_optionmenu");
	data->next_page_button = glade_xml_get_widget (data->gui, "next_page_button");
	data->prev_page_button = glade_xml_get_widget (data->gui, "prev_page_button");
	data->page_label = glade_xml_get_widget (data->gui, "page_label");
	comment_fontpicker_hbox = glade_xml_get_widget (data->gui, "comment_fontpicker_hbox");
	data->print_comment_checkbutton = glade_xml_get_widget (data->gui, "print_comment_checkbutton");
	data->print_filename_checkbutton = glade_xml_get_widget (data->gui, "print_filename_checkbutton");
	data->comment_font_hbox = glade_xml_get_widget (data->gui, "comment_font_hbox");
	data->scale_image_box = glade_xml_get_widget (data->gui, "scale_image_box");
	data->btn_close = glade_xml_get_widget (data->gui, "btn_close");
	data->btn_print = glade_xml_get_widget (data->gui, "btn_print");
	center_button = glade_xml_get_widget (data->gui, "btn_center");
	pci->canvas = glade_xml_get_widget (data->gui, "canvas");

	data->pd = progress_dialog_new (GTK_WINDOW (data->dialog));
	progress_dialog_set_cancel_func (data->pd, cancel_image_loading, data);

	/**/

	data->comment_fontpicker = gtk_font_button_new ();
	gtk_font_button_set_use_font (GTK_FONT_BUTTON (data->comment_fontpicker), TRUE);
	gtk_font_button_set_use_size (GTK_FONT_BUTTON (data->comment_fontpicker), TRUE);
	gtk_font_button_set_show_size (GTK_FONT_BUTTON (data->comment_fontpicker), TRUE);
	gtk_font_button_set_show_style (GTK_FONT_BUTTON (data->comment_fontpicker), TRUE);

	gtk_widget_show (data->comment_fontpicker);
	gtk_container_add (GTK_CONTAINER (comment_fontpicker_hbox), data->comment_fontpicker);

	hscale = glade_xml_get_widget (data->gui, "image_scale");
	data->adj = gtk_range_get_adjustment (GTK_RANGE (hscale));

	/* Set widgets data. */

	print_notebook = glade_xml_get_widget (data->gui, "print_notebook");

	gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->width_spinbutton), eel_gconf_get_float (PREF_PRINT_PAPER_WIDTH, DEF_PAPER_WIDTH));
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->height_spinbutton), eel_gconf_get_float (PREF_PRINT_PAPER_HEIGHT, DEF_PAPER_HEIGHT));
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->margin_left_spinbutton), eel_gconf_get_float (PREF_PRINT_MARGIN_LEFT, DEF_MARGIN_LEFT));
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->margin_right_spinbutton), eel_gconf_get_float (PREF_PRINT_MARGIN_RIGHT, DEF_MARGIN_RIGHT));
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->margin_top_spinbutton), eel_gconf_get_float (PREF_PRINT_MARGIN_TOP, DEF_MARGIN_TOP));
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->margin_bottom_spinbutton), eel_gconf_get_float (PREF_PRINT_MARGIN_BOTTOM, DEF_MARGIN_BOTTOM));
	gtk_option_menu_set_history (GTK_OPTION_MENU (data->unit_optionmenu), pref_get_print_unit ());


	gtk_option_menu_set_history (GTK_OPTION_MENU (data->resolution_optionmenu), pref_get_image_resolution());
	gtk_option_menu_set_history (GTK_OPTION_MENU (data->img_unit_optionmenu), pref_get_image_unit ());
	switch (pref_get_image_sizing()) {
		case GTH_IMAGE_SIZING_AUTO:
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->img_size_auto_radiobutton), TRUE);
			gtk_widget_set_sensitive (data->img_size_manual_box, FALSE);
			pci->is_auto_sizing = TRUE;
			break;
		case GTH_IMAGE_SIZING_MANUAL:
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->img_size_manual_radiobutton), TRUE);
			gtk_widget_set_sensitive (data->img_size_auto_box, FALSE);
			pci->is_auto_sizing = FALSE;
			break;
	};

	/* image size */

	pci->image_unit = print_units[gtk_option_menu_get_history (GTK_OPTION_MENU (data->img_unit_optionmenu))];
	pci->image_width = convert_to_points (eel_gconf_get_float (PREF_PRINT_IMAGE_WIDTH, DEF_IMAGE_WIDTH), pci->image_unit);
	pci->image_height = convert_to_points (eel_gconf_get_float (PREF_PRINT_IMAGE_HEIGHT, DEF_IMAGE_HEIGHT), pci->image_unit);

	/* page orientation */

	v = eel_gconf_get_integer (PREF_PRINT_PAPER_ORIENTATION, DEF_PAPER_ORIENT);
	switch (v) {
		case GTK_PAGE_ORIENTATION_PORTRAIT:
			button_name = "print_orient_portrait_radiobutton";
			gtk_page_setup_set_orientation(pci->page_setup, GTK_PAGE_ORIENTATION_PORTRAIT);
			break;
		case GTK_PAGE_ORIENTATION_LANDSCAPE:
			button_name = "print_orient_landscape_radiobutton";
			gtk_page_setup_set_orientation(pci->page_setup, GTK_PAGE_ORIENTATION_LANDSCAPE);
			break;
		default:
			button_name = "print_orient_portrait_radiobutton";
			gtk_page_setup_set_orientation(pci->page_setup, GTK_PAGE_ORIENTATION_PORTRAIT);
	}
	radio_button = glade_xml_get_widget (data->gui, button_name);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio_button), TRUE);

	/**/

	catalog_update_margins (data);

	value = eel_gconf_get_string (PREF_PRINT_PAPER_SIZE, DEF_PAPER_SIZE);
	if (strcmp (value, "Custom") == 0)
		catalog_update_custom_page_size (data);
	else
		catalog_set_standard_page_size (data, value);

	gtk_option_menu_set_history (GTK_OPTION_MENU (data->paper_size_optionmenu),
				     get_history_from_paper_size (value));
	g_free (value);

	gtk_option_menu_set_history (GTK_OPTION_MENU (data->images_per_page_optionmenu), (int) _log2 (pci->auto_images_per_page));

	/**/

	gtk_font_button_set_font_name (GTK_FONT_BUTTON (data->comment_fontpicker), eel_gconf_get_string (PREF_PRINT_COMMENT_FONT, DEF_COMMENT_FONT));

	/**/

	pci->print_comments = eel_gconf_get_boolean (PREF_PRINT_INCLUDE_COMMENT, FALSE);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->print_comment_checkbutton), pci->print_comments);

	pci->print_filenames = eel_gconf_get_boolean (PREF_PRINT_INCLUDE_FILENAME, FALSE);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->print_filename_checkbutton), pci->print_filenames);

	gtk_widget_set_sensitive (data->comment_font_hbox, pci->print_comments || pci->print_filenames);

	pci_update_comment_font (data);

	/* Set the signals handlers. */

	g_signal_connect (G_OBJECT (data->dialog),
			  "destroy",
			  G_CALLBACK (print_catalog_destroy_cb),
			  data);
	radio_button = glade_xml_get_widget (data->gui, "img_size_auto_radiobutton");
	g_signal_connect (G_OBJECT (radio_button),
			  "toggled",
			  G_CALLBACK (catalog_auto_size_toggled_cb),
			  data);
	radio_button = glade_xml_get_widget (data->gui, "img_size_manual_radiobutton");
	g_signal_connect (G_OBJECT (radio_button),
			  "toggled",
			  G_CALLBACK (catalog_manual_size_toggled_cb),
			  data);
	g_signal_connect (G_OBJECT (data->img_unit_optionmenu),
			  "changed",
			  G_CALLBACK (catalog_image_unit_changed_cb),
			  data);
	g_signal_connect (G_OBJECT (data->img_width_spinbutton),
			  "value_changed",
			  G_CALLBACK (catalog_image_width_changed_cb),
			  data);
	g_signal_connect (G_OBJECT (data->img_height_spinbutton),
			  "value_changed",
			  G_CALLBACK (catalog_image_height_changed_cb),
			  data);
	g_signal_connect (G_OBJECT (data->resolution_optionmenu),
			  "changed",
			  G_CALLBACK (catalog_image_resolution_changed_cb),
			  data);
	g_signal_connect_swapped (G_OBJECT (data->btn_close),
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  G_OBJECT (data->dialog));
	g_signal_connect (G_OBJECT (data->btn_print),
			  "clicked",
			  G_CALLBACK (print_catalog_cb),
			  data);

	radio_button = glade_xml_get_widget (data->gui, "print_orient_portrait_radiobutton");
	g_signal_connect (G_OBJECT (radio_button),
			  "toggled",
			  G_CALLBACK (catalog_portrait_toggled_cb),
			  data);

	radio_button = glade_xml_get_widget (data->gui, "print_orient_landscape_radiobutton");
	g_signal_connect (G_OBJECT (radio_button),
			  "toggled",
			  G_CALLBACK (catalog_landscape_toggled_cb),
			  data);

	g_signal_connect (G_OBJECT (data->paper_size_optionmenu),
			  "changed",
			  G_CALLBACK (catalog_paper_size_optionmenu_changed_cb),
			  data);

	g_signal_connect (G_OBJECT (data->unit_optionmenu),
			  "changed",
			  G_CALLBACK (catalog_unit_changed_cb),
			  data);

	g_signal_connect (G_OBJECT (data->width_spinbutton),
			  "value_changed",
			  G_CALLBACK (catalog_custom_size_value_changed_cb),
			  data);
	g_signal_connect (G_OBJECT (data->height_spinbutton),
			  "value_changed",
			  G_CALLBACK (catalog_custom_size_value_changed_cb),
			  data);

	g_signal_connect (G_OBJECT (data->margin_left_spinbutton),
			  "value_changed",
			  G_CALLBACK (catalog_margin_value_changed_cb),
			  data);
	g_signal_connect (G_OBJECT (data->margin_right_spinbutton),
			  "value_changed",
			  G_CALLBACK (catalog_margin_value_changed_cb),
			  data);
	g_signal_connect (G_OBJECT (data->margin_top_spinbutton),
			  "value_changed",
			  G_CALLBACK (catalog_margin_value_changed_cb),
			  data);
	g_signal_connect (G_OBJECT (data->margin_bottom_spinbutton),
			  "value_changed",
			  G_CALLBACK (catalog_margin_value_changed_cb),
			  data);

	g_signal_connect (G_OBJECT (data->images_per_page_optionmenu),
			  "changed",
			  G_CALLBACK (images_per_page_value_changed_cb),
			  data);
	g_signal_connect (G_OBJECT (data->next_page_button),
			  "clicked",
			  G_CALLBACK (next_page_cb),
			  data);
	g_signal_connect (G_OBJECT (data->prev_page_button),
			  "clicked",
			  G_CALLBACK (prev_page_cb),
			  data);

	g_signal_connect (G_OBJECT (data->comment_fontpicker),
			  "font_set",
			  G_CALLBACK (pci_comment_fontpicker_font_set_cb),
			  data);
	g_signal_connect (G_OBJECT (data->print_comment_checkbutton),
			  "toggled",
			  G_CALLBACK (pci_print_comments_cb),
			  data);
	g_signal_connect (G_OBJECT (data->print_filename_checkbutton),
			  "toggled",
			  G_CALLBACK (pci_print_filenames_cb),
			  data);

	g_signal_connect (G_OBJECT (center_button),
			  "clicked",
			  G_CALLBACK (catalog_center_cb),
			  data);
	g_signal_connect (G_OBJECT (data->adj),
			  "value_changed",
			  G_CALLBACK (catalog_zoom_changed),
			  data);


	/**/
	data->parent = parent;

	/**/

	gtk_window_set_title (GTK_WINDOW (data->dialog), _("Print"));
	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), parent);
	gnome_canvas_set_pixels_per_unit (GNOME_CANVAS (pci->canvas),
					  CANVAS_ZOOM);

	load_current_image (data);
	progress_dialog_show (data->pd);
}


void
print_catalog_dlg (GtkWindow *parent,
		   GList     *file_list)
{
	print_catalog_dlg_full (parent, file_list, NULL, NULL);
}
