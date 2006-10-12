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
#include <libgnomeui/gnome-font-picker.h>
#include <libgnomeprint/gnome-print.h>
#include <libgnomeprint/gnome-print-unit.h>
#include <libgnomeprint/gnome-print-job.h>
#include <libgnomeprintui/gnome-print-preview.h>
#include <libgnomeprintui/gnome-print-job-preview.h>
#include <libgnomeprintui/gnome-print-dialog.h>
#include <libgnomeprintui/gnome-print-paper-selector.h>
#include <libgnomecanvas/libgnomecanvas.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <libgnomevfs/gnome-vfs-uri.h>
#include <libart_lgpl/libart.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glade/glade.h>
#include <pango/pango-layout.h>

#include "glib-utils.h"
#include "gconf-utils.h"
#include "file-utils.h"
#include "image-viewer.h"
#include "image-loader.h"
#include "pixbuf-utils.h"
#include "comments.h"
#include "gnome-print-font-picker.h"
#include "preferences.h"
#include "progress-dialog.h"

#define UCHAR (guchar*)
#define GLADE_FILE "gthumb_print.glade"
#define PARAGRAPH_SEPARATOR 0x2029	
#define CANVAS_ZOOM 0.25
#define DEF_COMMENT_FONT "Serif Regular 12"
#define DEF_PAPER_WIDTH  0.0
#define DEF_PAPER_HEIGHT 0.0
#define DEF_PAPER_SIZE   "A4"
#define DEF_PAPER_ORIENT "R0"
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

#define UNIT_MM 0
#define UNIT_IN 1
static const GnomePrintUnit print_units[] = {
        {0, GNOME_PRINT_UNIT_ABSOLUTE, (72.0 / 25.4), 
	 UCHAR "Millimeter", UCHAR "mm", UCHAR "Millimeters", UCHAR "mm"},
        {0, GNOME_PRINT_UNIT_ABSOLUTE, (72.0), 
	 UCHAR "Inch", UCHAR "in", UCHAR "Inches", UCHAR "in"},
};

static const char *paper_sizes[] = {"A4", "USLetter", "USLegal", "Tabloid", 
				    "Executive", "Postcard", "Custom"};
#define CUSTOM_PAPER_SIZE_IDX 6


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
orientation_is_portrait (GnomePrintConfig *config)
{
	char     *orientation;
	gboolean  portrait;

	orientation = (char*) gnome_print_config_get (config, UCHAR GNOME_PRINT_KEY_PAGE_ORIENTATION);
	portrait = ((strcmp (orientation, "R0") == 0) || (strcmp (orientation, "R180") == 0));
	g_free (orientation);

	return portrait;
}


static int
get_desktop_default_font_size (void)
{
	GConfClient          *gconf_client = NULL;
	int                   res;
        char                 *font;
        PangoFontDescription *desc;

	gconf_client = gconf_client_get_default ();
        if (gconf_client == NULL)
                return DEFAULT_FONT_SIZE;

	font = gconf_client_get_string (gconf_client,
                                        "/desktop/gnome/interface/font_name",
                                        NULL);
        if (font == NULL)
                return DEFAULT_FONT_SIZE;

        desc = pango_font_description_from_string (font);

        g_free (font);
        g_return_val_if_fail (desc != NULL, DEFAULT_FONT_SIZE);

        res = pango_font_description_get_size (desc) / PANGO_SCALE;

        pango_font_description_free (desc);

        g_object_unref (gconf_client);

        return res;
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

	image->filename = g_strdup (get_file_path_from_uri (filename));
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

	GnomeFont           *font_comment;

	double               paper_width;
	double               paper_height;
	double               paper_lmargin;
	double               paper_rmargin;
	double               paper_tmargin;
	double               paper_bmargin;

	GnomePrintConfig    *config;
	GnomePrintJob       *gpj;

	gboolean             print_comments;
	gboolean             portrait;

	gboolean             use_colors;

	int                  images_per_page;
	int                  n_images;
	ImageInfo          **image_info;

	double               max_image_width, max_image_height;
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

		if (pci->gpj != NULL)
			g_object_unref (pci->gpj);
		gnome_print_config_unref (pci->config);

		if (pci->font_comment != NULL)
			g_object_unref (pci->font_comment);

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
	GtkWidget     *comment_font_hbox;
	GtkWidget     *scale_image_box;

	GtkAdjustment *adj;

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





/* The following functions are from gedit-print.c
 * Copyright (C) 2000, 2001 Chema Celorio, Paolo Maggi
 * Copyright (C) 2002  Paolo Maggi  
 * Modified for gthumb by Paolo Bacchilega, 2003.
 */

static const char*
pci_get_next_line_to_print_delimiter (PrintCatalogInfo *pci, 
				      double            max_width,
				      const char       *start,
				      const char       *end,
				      double           *line_width)
{
	const char  *p;
	double       current_width = 0.0;
	ArtPoint     space_advance;
	int          space, new_line;
	
	/* Find space advance */
	space = gnome_font_lookup_default (pci->font_comment, ' ');
	gnome_font_get_glyph_stdadvance (pci->font_comment, space, &space_advance);

	new_line = gnome_font_lookup_default (pci->font_comment, '\n');

	for (p = start; p < end; p = g_utf8_next_char (p)) {
		gunichar ch;
		int      glyph;
		
		ch = g_utf8_get_char (p);
		glyph = gnome_font_lookup_default (pci->font_comment, ch);

		if (glyph == new_line) {
			if (line_width != NULL)
				*line_width = max_width;
			return p;

		} else if (glyph == space)
			current_width += space_advance.x;

		else {
			ArtPoint adv;
			gnome_font_get_glyph_stdadvance (pci->font_comment, 
							 glyph,
							 &adv);
			if (adv.x > 0)
				current_width += adv.x;
			else
				/* To be as conservative as possible */
				current_width += (2 * space_advance.x);
		}

		if (current_width > max_width) {
			if (line_width != NULL)
				*line_width = max_width;
			return p;
		}
	}
	
	if (line_width != NULL)
		*line_width = current_width;

	return end;
}


static void
pci_print_line (GnomePrintContext *pc,
		PrintCatalogInfo  *pci, 
		const char        *start, 
		const char        *end, 
		double             x,
		double             y) 
{
	GnomeGlyphList *gl;
	const char     *p;
	
	gl = gnome_glyphlist_from_text_dumb (pci->font_comment, 0x000000ff, 0.0, 0.0, UCHAR "");
	gnome_glyphlist_moveto (gl, x, y - gnome_font_get_ascender (pci->font_comment));
	
	for (p = start; p < end; p = g_utf8_next_char (p)) {
		gunichar ch    = g_utf8_get_char (p);
	       	int      glyph = gnome_font_lookup_default (pci->font_comment, ch);
		gnome_glyphlist_glyph (gl, glyph);
	}

	gnome_print_moveto (pc, 0.0, 0.0);
	gnome_print_glyphlist (pc, gl);
	gnome_glyphlist_unref (gl);
}


static gdouble
pci_print_paragraph (GnomePrintContext *pc,
		     PrintCatalogInfo  *pci,
		     const char        *start, 
		     const char        *end, 
		     double             max_width,
		     double             x,
		     double             y)
{
	const char *p, *s;

	for (p = start; p < end; ) {
		s = p;
		p = pci_get_next_line_to_print_delimiter (pci, max_width, s, end, NULL);
		pci_print_line (pc, pci, s, p, x, y);
		y -= 1.2 * gnome_font_get_size (pci->font_comment);
	}

	return y;
}


static void
pci_get_text_extents (PrintCatalogInfo *pci,
		      double            max_width,
		      const char       *start, 
		      const char       *text_end,
		      double           *width,
		      double           *height)
{
	const char *p;
	const char *end;
	int         paragraph_delimiter_index;
	int         next_paragraph_start;

	*width = 0.0;
	*height = 0.0;

	pango_find_paragraph_boundary (start, -1, 
				       &paragraph_delimiter_index, 
				       &next_paragraph_start);
	end = start + paragraph_delimiter_index;

	for (p = start; p < text_end;) {
		gunichar wc = g_utf8_get_char (p);

		if ((wc == '\n' ||
           	     wc == '\r' ||
                     wc == PARAGRAPH_SEPARATOR)) 
			*height += 1.2 * gnome_font_get_size (pci->font_comment);
		else {
			const char *p1, *s1;

			for (p1 = p; p1 < end; ) {
				double line_width;

				s1 = p1;
				p1 = pci_get_next_line_to_print_delimiter (pci, max_width, s1, end, &line_width);
				*width = MAX (*width, line_width);
				*height += 1.2 * gnome_font_get_size (pci->font_comment);
			}
		}

		p = p + next_paragraph_start;
		
		if (p < text_end) {
			pango_find_paragraph_boundary (p, -1, &paragraph_delimiter_index, &next_paragraph_start);
			end = p + paragraph_delimiter_index;
		}
	}
}


static double
pci_print_comment (GnomePrintContext *pc,
		   PrintCatalogInfo  *pci,
		   ImageInfo         *image)
{
	const char *p;
	const char *end;
	double      fontheight;
	double      x, y, width, height;
	double      printable_width;
	int         paragraph_delimiter_index;
	int         next_paragraph_start;
	char       *text_end;

	if (image->comment == NULL)
		return 0.0;

	if (!image->print_comment)
		return 0.0;

	gnome_print_setfont (pc, pci->font_comment);

	p = image->comment;
	text_end = image->comment + strlen (image->comment);

	pci_get_text_extents (pci, pci->max_image_width, p, text_end, &width, &height);

	printable_width = pci->max_image_width;
	x = image->min_x + MAX (0, (printable_width - width) / 2);
	y = pci->paper_height - image->max_y + height;

	pango_find_paragraph_boundary (image->comment, -1, 
				       &paragraph_delimiter_index, 
				       &next_paragraph_start);

	end = image->comment + paragraph_delimiter_index;

	fontheight = (gnome_font_get_ascender (pci->font_comment) + 
		      gnome_font_get_descender (pci->font_comment));

	while (p < text_end) {
		gunichar wc = g_utf8_get_char (p);

		if ((wc == '\n' ||
           	     wc == '\r' ||
                     wc == PARAGRAPH_SEPARATOR)) {
	
			y -= 1.2 * gnome_font_get_size (pci->font_comment);
			
			if (y - image->max_y < fontheight) {
				/* FIXME
				gnome_print_showpage (pc);
				gnome_print_beginpage (pc, NULL);

				x = pi->paper_lmargin + MAX (0, (printable_width - width) / 2);
				y = pi->paper_bmargin + height;
				*/
				/* text do not fit. */
				return image->max_y + height;
			}
		} else {
			y = pci_print_paragraph (pc, pci, p, end, printable_width, x, y);
			
			if ((y - image->max_y) < fontheight) {
				/* FIXME
				gnome_print_showpage (pc);
				gnome_print_beginpage (pc, NULL);

				x = pi->paper_lmargin + MAX (0, (printable_width - width) / 2);
				y = pi->paper_bmargin + height;
				*/
				/* text do not fit. */
				return image->max_y + height;
			}
		}

		p = p + next_paragraph_start;

		if (p < text_end) {
			pango_find_paragraph_boundary (p, -1, &paragraph_delimiter_index, &next_paragraph_start);
			end = p + paragraph_delimiter_index;
		}
	}

	return image->max_y + height;
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
		rotated = _gdk_pixbuf_copy_rotate_90 (pixbuf, FALSE);
		break;
	case 180:
		rotated = _gdk_pixbuf_copy_mirror (pixbuf, TRUE, TRUE);
		break;
	case 270:
		rotated = _gdk_pixbuf_copy_rotate_90 (pixbuf, TRUE);
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
	image->thumbnail = print__gdk_pixbuf_rotate (tmp_pixbuf, angle);
	g_object_unref (tmp_pixbuf);
	
	tmp_pixbuf = image->thumbnail_active;
	image->thumbnail_active = print__gdk_pixbuf_rotate (tmp_pixbuf, angle);
	g_object_unref (tmp_pixbuf);
	
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
	int               i, rows, cols, row, col, page = 0, idx;

	g_free (pci->pages);

	pci->n_pages = MAX ((int) ceil ((double) pci->n_images / pci->images_per_page), 1);
	pci->pages = g_new0 (GnomeCanvasItem*, pci->n_pages);
	pci->current_page = 0;

 	root = GNOME_CANVAS_GROUP (gnome_canvas_root (GNOME_CANVAS (pci->canvas)));
	pci->pages[page] = gnome_canvas_item_new (root,
						  gnome_canvas_group_get_type (),
						  NULL);

	w = pci->paper_width;
	h = pci->paper_height;
	lmargin = pci->paper_lmargin;
	rmargin = pci->paper_rmargin;
	bmargin = pci->paper_bmargin;
	tmargin = pci->paper_tmargin;

	layout_width = (int) ((pci->paper_width - lmargin - rmargin) * CANVAS_ZOOM);

	max_w = w - lmargin - rmargin;
	max_h = h - bmargin - tmargin;

	idx = (int) floor (_log2 (pci->images_per_page) + 0.5);
	rows = catalog_rows[idx];
	cols = catalog_cols[idx];

	if (!orientation_is_portrait (data->pci->config)) {
		int tmp = rows;
		rows = cols;
		cols = tmp;
	}

	pci->max_image_width = (max_w - ((cols-1) * IMAGE_SPACE)) / cols;
	pci->max_image_height = (max_h - ((rows-1) * IMAGE_SPACE)) / rows;

	row = 1;
	col = 1;
	for (i = 0; i < pci->n_images; i++) {
		ImageInfo *image = pci->image_info[i];
		double     iw, ih;
		double     factor, max_image_height;
	
		image_info_rotate (image, (360 - image->rotate) % 360);

		if (((pci->max_image_width > pci->max_image_height) 
		     && (image->pixbuf_width < image->pixbuf_height)) 
		    || ((pci->max_image_width < pci->max_image_height) 
			&& (image->pixbuf_width > image->pixbuf_height))) 
			image_info_rotate (image, 270);

		reset_zoom (data, image);

		image->min_x = lmargin + ((col-1) * (pci->max_image_width + IMAGE_SPACE));
		image->min_y = tmargin + ((row-1) * (pci->max_image_height + IMAGE_SPACE));
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

		if (pci->print_comments && (image->comment != NULL)) {
			const char *p;
			const char *text_end;
			double      comment_width;
			
			p = image->comment;
			text_end = image->comment + strlen (image->comment);
			pci_get_text_extents (pci, pci->max_image_width, p, text_end, &comment_width, &(image->comment_height));

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
	const      GnomePrintUnit *unit;
	const      GnomePrintUnit *ps_unit;
	char      *orientation;
	double     width, height, lmargin, tmargin, rmargin, bmargin;
	double     paper_width, paper_height, paper_lmargin, paper_tmargin;
	double     paper_rmargin, paper_bmargin;
	gboolean   portrait;

	ps_unit = gnome_print_unit_get_identity (GNOME_PRINT_UNIT_DIMENSIONLESS);

	if (gnome_print_config_get_length (pci->config, 
					   UCHAR GNOME_PRINT_KEY_PAPER_WIDTH,
					   &width,
					   &unit))
		gnome_print_convert_distance (&width, unit, ps_unit);
	if (gnome_print_config_get_length (pci->config, 
					   UCHAR GNOME_PRINT_KEY_PAPER_HEIGHT,
					   &height,
					   &unit))
		gnome_print_convert_distance (&height, unit, ps_unit);
	if (gnome_print_config_get_length (pci->config, 
					   UCHAR GNOME_PRINT_KEY_PAGE_MARGIN_LEFT,
					   &lmargin,
					   &unit))
		gnome_print_convert_distance (&lmargin, unit, ps_unit);
	if (gnome_print_config_get_length (pci->config, 
					   UCHAR GNOME_PRINT_KEY_PAGE_MARGIN_RIGHT,
					   &rmargin,
					   &unit))
		gnome_print_convert_distance (&rmargin, unit, ps_unit);
	if (gnome_print_config_get_length (pci->config, 
					   UCHAR GNOME_PRINT_KEY_PAGE_MARGIN_TOP,
					   &tmargin,
					   &unit))
		gnome_print_convert_distance (&tmargin, unit, ps_unit);
	if (gnome_print_config_get_length (pci->config, 
					   UCHAR GNOME_PRINT_KEY_PAGE_MARGIN_BOTTOM,
					   &bmargin,
					   &unit))
		gnome_print_convert_distance (&bmargin, unit, ps_unit);

	orientation = (char*)gnome_print_config_get (pci->config, UCHAR GNOME_PRINT_KEY_PAGE_ORIENTATION);
	portrait = ((strcmp (orientation, "R0") == 0) 
		    || (strcmp (orientation, "R180") == 0));
	g_free (orientation);

	if (portrait) {
		paper_width   = width;
		paper_height  = height;
		paper_lmargin = lmargin;
		paper_tmargin = tmargin;
		paper_rmargin = rmargin;
		paper_bmargin = bmargin;
	} else {
		paper_width   = height;
		paper_height  = width;
		paper_lmargin = tmargin;
		paper_tmargin = rmargin;
		paper_rmargin = bmargin;
		paper_bmargin = lmargin;
	}

	pci->portrait = portrait;
	pci->paper_width   = paper_width;
	pci->paper_height  = paper_height;
	pci->paper_lmargin = paper_lmargin;
	pci->paper_tmargin = paper_tmargin;
	pci->paper_rmargin = paper_rmargin;
	pci->paper_bmargin = paper_bmargin;

	clear_canvas (GNOME_CANVAS_GROUP (GNOME_CANVAS (pci->canvas)->root));
	gnome_canvas_set_scroll_region (GNOME_CANVAS (pci->canvas), 
					0, 0,
					pci->paper_width, pci->paper_height);
	add_simulated_page (GNOME_CANVAS (pci->canvas));
	add_catalog_preview (data, TRUE);

	gtk_widget_set_sensitive (data->scale_image_box, (pci->images_per_page == 1) || (pci->n_images == 1));
}


static void
catalog_update_custom_page_size (PrintCatalogDialogData *data)
{
	const GnomePrintUnit *unit;
	double width, height;

	unit = &print_units[gtk_option_menu_get_history (GTK_OPTION_MENU (data->unit_optionmenu))];
	width = gtk_spin_button_get_value (GTK_SPIN_BUTTON (data->width_spinbutton));
	height = gtk_spin_button_get_value (GTK_SPIN_BUTTON (data->height_spinbutton));
	gnome_print_config_set_length (data->pci->config, UCHAR GNOME_PRINT_KEY_PAPER_WIDTH, width, unit);
	gnome_print_config_set_length (data->pci->config, UCHAR GNOME_PRINT_KEY_PAPER_HEIGHT, height, unit);

	catalog_update_page (data);
}


static double
catalog_get_current_unittobase (PrintCatalogDialogData *data)
{
	const GnomePrintUnit *unit;
	unit = &print_units[gtk_option_menu_get_history (GTK_OPTION_MENU (data->unit_optionmenu))];
	return unit->unittobase;
}


static double 
catalog_get_config_length (PrintCatalogDialogData *data,
			   const char             *key)
{
	const GnomePrintUnit *unit;
	double                len;

	gnome_print_config_get_length (data->pci->config, UCHAR key, &len, &unit);
	len = len * unit->unittobase / catalog_get_current_unittobase (data);
	
	return len;
}


static double 
catalog_get_page_width (PrintCatalogDialogData *data)
{
	return catalog_get_config_length (data, GNOME_PRINT_KEY_PAPER_WIDTH);
}


static double 
catalog_get_page_height (PrintCatalogDialogData *data)
{
	return catalog_get_config_length (data, GNOME_PRINT_KEY_PAPER_HEIGHT);
}


static void catalog_margin_value_changed_cb (GtkSpinButton          *spin,
					     PrintCatalogDialogData *data);

static void catalog_custom_size_value_changed_cb (GtkSpinButton          *spin,
						  PrintCatalogDialogData *data);

static void
catalog_update_page_size_from_config (PrintCatalogDialogData *data)
{
	double len;

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

	len = catalog_get_config_length (data, GNOME_PRINT_KEY_PAGE_MARGIN_LEFT);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->margin_left_spinbutton), len);

	len = catalog_get_config_length (data, GNOME_PRINT_KEY_PAGE_MARGIN_RIGHT);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->margin_right_spinbutton), len);

	len = catalog_get_config_length (data, GNOME_PRINT_KEY_PAGE_MARGIN_TOP);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->margin_top_spinbutton), len);

	len = catalog_get_config_length (data, GNOME_PRINT_KEY_PAGE_MARGIN_BOTTOM);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->margin_bottom_spinbutton), len);

	g_signal_handlers_unblock_by_func (data->margin_left_spinbutton, catalog_margin_value_changed_cb, data);
	g_signal_handlers_unblock_by_func (data->margin_right_spinbutton, catalog_margin_value_changed_cb, data);
	g_signal_handlers_unblock_by_func (data->margin_top_spinbutton, catalog_margin_value_changed_cb, data);
	g_signal_handlers_unblock_by_func (data->margin_bottom_spinbutton, catalog_margin_value_changed_cb, data);
}


static void
catalog_set_standard_page_size (PrintCatalogDialogData *data,
				const char             *page_size)
{
	const GnomePrintUnit *unit;
	double width, height;

	if (strcmp (page_size, "A4") == 0) {
		unit = &print_units[UNIT_MM];
		width = 210.0;
		height = 297.0;

	} else if (strcmp (page_size, "USLetter") == 0) {
		unit = &print_units[UNIT_IN];
		width = 8.5;
		height = 11.0;

	} else if (strcmp (page_size, "USLegal") == 0) {
		unit = &print_units[UNIT_IN];
		width = 8.5;
		height = 14.0;

	} else if (strcmp (page_size, "Tabloid") == 0) {
		unit = &print_units[UNIT_IN];
		width = 11.0;
		height = 17.0;

	} else if (strcmp (page_size, "Executive") == 0) {
		unit = &print_units[UNIT_IN];
		width = 7.25;
		height = 10.50;

	} else if (strcmp (page_size, "Postcard") == 0) {
		unit = &print_units[UNIT_MM];
		width = 99.8;
		height = 146.8;

	} else
		return;

	gnome_print_config_set_length (data->pci->config, UCHAR GNOME_PRINT_KEY_PAPER_WIDTH, width, unit);
	gnome_print_config_set_length (data->pci->config, UCHAR GNOME_PRINT_KEY_PAPER_HEIGHT, height, unit);

	catalog_update_page_size_from_config (data);
	catalog_update_page (data);
}


static void
catalog_update_margins (PrintCatalogDialogData *data)
{
	const GnomePrintUnit *unit;
	double len;
	
	unit = &print_units[gtk_option_menu_get_history (GTK_OPTION_MENU (data->unit_optionmenu))];
	
	/* left margin */

	len = gtk_spin_button_get_value (GTK_SPIN_BUTTON (data->margin_left_spinbutton));
	gnome_print_config_set_length (data->pci->config, UCHAR GNOME_PRINT_KEY_PAGE_MARGIN_LEFT, len, unit);

	/* right margin */

	len = gtk_spin_button_get_value (GTK_SPIN_BUTTON (data->margin_right_spinbutton));
	gnome_print_config_set_length (data->pci->config, UCHAR GNOME_PRINT_KEY_PAGE_MARGIN_RIGHT, len, unit);

	/* top margin */

	len = gtk_spin_button_get_value (GTK_SPIN_BUTTON (data->margin_top_spinbutton));
	gnome_print_config_set_length (data->pci->config, UCHAR GNOME_PRINT_KEY_PAGE_MARGIN_TOP, len, unit);

	/* bottom margin */

	len = gtk_spin_button_get_value (GTK_SPIN_BUTTON (data->margin_bottom_spinbutton));
	gnome_print_config_set_length (data->pci->config, UCHAR GNOME_PRINT_KEY_PAGE_MARGIN_BOTTOM, len, unit);

	catalog_update_page (data);
}



static void
print_catalog (GnomePrintContext *pc,
	       PrintCatalogInfo  *pci, 
	       gboolean           border) 
{
	double w, h;
	double lmargin, rmargin, tmargin, bmargin;
	int    i;

	w = pci->paper_width;
	h = pci->paper_height;
	lmargin = pci->paper_lmargin;
	rmargin = pci->paper_rmargin;
	bmargin = pci->paper_bmargin;
	tmargin = pci->paper_tmargin;

	gnome_print_beginpage (pc, NULL);

	if (border) {
		gnome_print_gsave (pc);
		gnome_print_moveto (pc, lmargin, bmargin);
		gnome_print_lineto (pc, lmargin, h - tmargin);
		gnome_print_lineto (pc, w - rmargin, h - tmargin);
		gnome_print_lineto (pc, w - rmargin, bmargin);
		gnome_print_lineto (pc, lmargin, bmargin);
		gnome_print_stroke (pc);
		gnome_print_grestore (pc);
	}

	for (i = 0; i < pci->n_images; i++) {
		ImageInfo *image = pci->image_info[i];
		GdkPixbuf *image_pixbuf, *pixbuf = NULL;

		if (pci->print_comments) {
			gnome_print_gsave (pc);
			pci_print_comment (pc, pci, image);
			gnome_print_grestore (pc);
		}
		
		image_pixbuf = gdk_pixbuf_new_from_file (image->filename, NULL);
		pixbuf = print__gdk_pixbuf_rotate (image_pixbuf, image->rotate);
		g_object_unref (image_pixbuf);

		if (pixbuf != NULL) {
			guchar    *p;
			int        pw, ph, rs;

			p = gdk_pixbuf_get_pixels (pixbuf);
			pw = gdk_pixbuf_get_width (pixbuf);
			ph = gdk_pixbuf_get_height (pixbuf);
			rs = gdk_pixbuf_get_rowstride (pixbuf);

			gnome_print_gsave (pc);
			gnome_print_scale (pc, image->scale_x, image->scale_y);
			gnome_print_translate (pc, image->trans_x, image->trans_y);
			if (pci->use_colors) {
				if (gdk_pixbuf_get_has_alpha (pixbuf)) 
					gnome_print_rgbaimage (pc, p, pw, ph, rs);
				else 
					gnome_print_rgbimage  (pc, p, pw, ph, rs);
			} else
				gnome_print_grayimage (pc, p, pw, ph, rs);
			gnome_print_grestore (pc);
			
			g_object_unref (pixbuf);
		}

		if ((i + 1 < pci->n_images) && ((i + 1) % pci->images_per_page == 0)) {
			gnome_print_showpage (pc);
			gnome_print_beginpage (pc, NULL);
		}
	}

	gnome_print_showpage (pc);
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


static void
pci_update_comment_font (PrintCatalogDialogData *data)
{
	PrintCatalogInfo *pci = data->pci;
	const char       *font_name;

	if (pci->font_comment != NULL)
		g_object_unref (pci->font_comment);		

	font_name = gnome_print_font_picker_get_font_name (GNOME_PRINT_FONT_PICKER (data->comment_fontpicker));

	debug (DEBUG_INFO, "Find closest: %s", font_name);

	pci->font_comment = gnome_font_find_closest_from_full_name (UCHAR font_name);

	if (pci->font_comment == NULL)
		g_warning ("Could not find font %s\n", font_name);
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
	gnome_print_config_set (data->pci->config, UCHAR GNOME_PRINT_KEY_PAGE_ORIENTATION, UCHAR "R0");
	catalog_update_page (data);
}


static void
catalog_landscape_toggled_cb (GtkToggleButton        *widget,
			      PrintCatalogDialogData *data)
{
	if (! gtk_toggle_button_get_active (widget))
		return;
	gnome_print_config_set (data->pci->config, UCHAR GNOME_PRINT_KEY_PAGE_ORIENTATION, UCHAR "R90");
	catalog_update_page (data);
}


static void
catalog_margin_value_changed_cb (GtkSpinButton          *spin,
				 PrintCatalogDialogData *data)
{
	catalog_update_margins (data);
}


static void
catalog_unit_changed_cb (GtkOptionMenu          *opt_menu,
			 PrintCatalogDialogData *data)
{
	catalog_update_page_size_from_config (data);
}


static void 
print_catalog_cb (GtkWidget              *widget,
		  PrintCatalogDialogData *data)
{
	PrintCatalogInfo  *pci = data->pci; 
	GnomePrintContext *gp_ctx;
	GtkWidget         *dialog;
	int                response;
	char              *value;
	double             length;
	int                i;

	/* Save options */

	eel_gconf_set_string (PREF_PRINT_PAPER_SIZE, catalog_get_current_paper_size (data));
	eel_gconf_set_integer (PREF_PRINT_IMAGES_PER_PAGE, pci->images_per_page);
	eel_gconf_set_string (PREF_PRINT_COMMENT_FONT, gnome_print_font_picker_get_font_name (GNOME_PRINT_FONT_PICKER (data->comment_fontpicker)));
	eel_gconf_set_boolean (PREF_PRINT_INCLUDE_COMMENT, !gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (data->print_comment_checkbutton)) && gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->print_comment_checkbutton)));

	length = catalog_get_page_width (data);
	eel_gconf_set_float (PREF_PRINT_PAPER_WIDTH, length);

	length = catalog_get_page_height (data);
	eel_gconf_set_float (PREF_PRINT_PAPER_HEIGHT, length);

	length = catalog_get_config_length (data, GNOME_PRINT_KEY_PAGE_MARGIN_LEFT);
	eel_gconf_set_float (PREF_PRINT_MARGIN_LEFT, length);

	length = catalog_get_config_length (data, GNOME_PRINT_KEY_PAGE_MARGIN_RIGHT);
	eel_gconf_set_float (PREF_PRINT_MARGIN_RIGHT, length);

	length = catalog_get_config_length (data, GNOME_PRINT_KEY_PAGE_MARGIN_TOP);
	eel_gconf_set_float (PREF_PRINT_MARGIN_TOP, length);

	length = catalog_get_config_length (data, GNOME_PRINT_KEY_PAGE_MARGIN_BOTTOM);
	eel_gconf_set_float (PREF_PRINT_MARGIN_BOTTOM, length);

	pref_set_print_unit (catalog_get_current_unit (data));

	value = (char*)gnome_print_config_get (pci->config, UCHAR GNOME_PRINT_KEY_PAGE_ORIENTATION);
	eel_gconf_set_string (PREF_PRINT_PAPER_ORIENTATION, value);
	gnome_print_config_set (pci->config, UCHAR GNOME_PRINT_KEY_PAPER_ORIENTATION, UCHAR value);
	g_free (value);

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
		image->trans_x = image_x / image->scale_x;
		image->trans_y = (pci->paper_height - image->scale_y - image_y) / image->scale_y;
	}

	/* pci is used to print, so we must add a reference. */

	print_catalog_info_ref (pci);
	gtk_widget_hide (data->dialog); 

	/* Gnome Print Dialog */

	pci->gpj = gnome_print_job_new (pci->config);

	dialog = gnome_print_dialog_new (pci->gpj, UCHAR _("Print"), 0);
        response = gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_destroy (dialog);

	gp_ctx = gnome_print_job_get_context (pci->gpj);
	print_catalog (gp_ctx, pci, FALSE);
	gnome_print_job_close (pci->gpj);

        switch (response) {
        case GNOME_PRINT_DIALOG_RESPONSE_PRINT:
		gnome_print_job_print (pci->gpj);	
                break;

        case GNOME_PRINT_DIALOG_RESPONSE_PREVIEW:
		gtk_widget_show (gnome_print_job_preview_new (pci->gpj, UCHAR _("Print")));
                break;

        default:
		break;
        }
 
	gtk_widget_destroy (data->dialog); 
	print_catalog_info_unref (pci);
}


static void
images_per_page_value_changed_cb (GtkOptionMenu           *option_menu,
				  PrintCatalogDialogData  *data)
{
	data->pci->images_per_page = (int) pow (2, gtk_option_menu_get_history (option_menu));

	debug (DEBUG_INFO, "IPP: %d\n", data->pci->images_per_page);

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

	image_loader_set_path (data->loader, filename);

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
pci_comment_fontpicker_font_set_cb (GnomePrintFontPicker   *fontpicker,
				    const char             *font_name,
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
	gtk_widget_set_sensitive (data->comment_font_hbox, data->pci->print_comments);
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
	int                      n = 0;
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
	pci->config = gnome_print_config_default ();
	pci->portrait = TRUE;
	pci->use_colors = TRUE;
	pci->images_per_page = eel_gconf_get_integer (PREF_PRINT_IMAGES_PER_PAGE, DEF_IMAGES_PER_PAGE);
 
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
	data->comment_font_hbox = glade_xml_get_widget (data->gui, "comment_font_hbox");
	data->scale_image_box = glade_xml_get_widget (data->gui, "scale_image_box");
	data->btn_close = glade_xml_get_widget (data->gui, "btn_close");
	data->btn_print = glade_xml_get_widget (data->gui, "btn_print");
	center_button = glade_xml_get_widget (data->gui, "btn_center");
	pci->canvas = glade_xml_get_widget (data->gui, "canvas");
	
	data->pd = progress_dialog_new (GTK_WINDOW (data->dialog));
	progress_dialog_set_cancel_func (data->pd, cancel_image_loading, data);

	/**/

	data->comment_fontpicker = gnome_print_font_picker_new ();
	gnome_print_font_picker_set_mode (GNOME_PRINT_FONT_PICKER (data->comment_fontpicker),
				    GNOME_FONT_PICKER_MODE_FONT_INFO);
	gnome_print_font_picker_fi_set_use_font_in_label (GNOME_PRINT_FONT_PICKER (data->comment_fontpicker), TRUE, get_desktop_default_font_size ());
	gnome_print_font_picker_fi_set_show_size (GNOME_PRINT_FONT_PICKER (data->comment_fontpicker), TRUE);
	gtk_widget_show (data->comment_fontpicker);
	gtk_container_add (GTK_CONTAINER (comment_fontpicker_hbox), data->comment_fontpicker);

	hscale = glade_xml_get_widget (data->gui, "hscale");
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

	/**/

	value = eel_gconf_get_string (PREF_PRINT_PAPER_ORIENTATION, DEF_PAPER_ORIENT);
	gnome_print_config_set (pci->config, UCHAR GNOME_PRINT_KEY_PAGE_ORIENTATION, UCHAR value);

	if (strcmp (value, "R0") == 0)
		button_name = "print_orient_portrait_radiobutton";
	else if (strcmp (value, "R90") == 0)
		button_name = "print_orient_landscape_radiobutton";
	else
		button_name = "print_orient_portrait_radiobutton";
	radio_button = glade_xml_get_widget (data->gui, button_name);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio_button), TRUE);

	g_free (value);

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

	gtk_option_menu_set_history (GTK_OPTION_MENU (data->images_per_page_optionmenu), (int) _log2 (pci->images_per_page));

	/**/

	value = eel_gconf_get_string (PREF_PRINT_COMMENT_FONT, DEF_COMMENT_FONT);

	if ((value != NULL) && (*value != 0)) 
		if (!gnome_print_font_picker_set_font_name (GNOME_PRINT_FONT_PICKER (data->comment_fontpicker), PREF_PRINT_COMMENT_FONT)) {
			g_free (value);
			value = NULL;
		}
	
	if ((value == NULL) || (*value == 0)) 
		gnome_print_font_picker_set_font_name (GNOME_PRINT_FONT_PICKER (data->comment_fontpicker), DEF_COMMENT_FONT);
	
	g_free (value);

	/**/

	pci->print_comments = eel_gconf_get_boolean (PREF_PRINT_INCLUDE_COMMENT, FALSE);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->print_comment_checkbutton), pci->print_comments);
	gtk_widget_set_sensitive (data->comment_font_hbox, pci->print_comments);

	pci_update_comment_font (data);

	/* Set the signals handlers. */

	g_signal_connect (G_OBJECT (data->dialog), 
			  "destroy",
			  G_CALLBACK (print_catalog_destroy_cb),
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

	g_signal_connect (G_OBJECT (center_button),
			  "clicked",
			  G_CALLBACK (catalog_center_cb),
			  data);
	g_signal_connect (G_OBJECT (data->adj), 
			  "value_changed",
			  G_CALLBACK (catalog_zoom_changed),
			  data);

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
