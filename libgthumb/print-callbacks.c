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
#include <string.h>
#include <math.h>

#include <libgnome/libgnome.h>
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

#define GLADE_FILE "gthumb_print.glade"
#define PARAGRAPH_SEPARATOR 0x2029	
#define CANVAS_ZOOM 0.25
#define DEF_COMMENT_FONT "Serif 12"
#define DEF_PAPER_WIDTH  0.0
#define DEF_PAPER_HEIGHT 0.0
#define DEF_PAPER_SIZE   "A4"
#define DEF_PAPER_ORIENT "R0"
#define DEF_IMAGES_PER_PAGE 4
#define ACTIVE_THUMB_BRIGHTNESS (42.0 / 127.0)
#define THUMB_SIZE 128

#define gray50_width  1
#define gray50_height 5

static const GnomePrintUnit print_units[] = {
        {0, GNOME_PRINT_UNIT_ABSOLUTE, (72.0 / 25.4), "Millimeter", "mm", "Millimeters", "mm"},
        {0, GNOME_PRINT_UNIT_ABSOLUTE, (72.0), "Inch", "in", "Inches", "in"},
};


typedef struct {
	int                  ref_count;

	GtkWidget           *canvas;
	GnomeCanvasItem     *ci_image;
	GnomeCanvasItem     *ci_comment;

	GnomeFont           *font_comment;

	/* Paper Information. */

	double               paper_width;
	double               paper_height;
	double               paper_lmargin;
	double               paper_rmargin;
	double               paper_tmargin;
	double               paper_bmargin;

	GnomePrintConfig    *config;
	GnomePrintJob       *gpj;

	gboolean             print_comment;
	gboolean             portrait;

	/* Image Information. */

	char                *image_path;

	GdkPixbuf           *pixbuf;
	
	char                *comment;
	
	double               image_w, image_h;
	
	double               scale_x, scale_y;
	double               trans_x, trans_y;
	double               zoom;

	double               min_x, min_y;
	double               max_x, max_y;

	gboolean             use_colors;
} PrintInfo;


enum {
	PAPER_LIST_COLUMN_DATA,
	PAPER_LIST_COLUMN_NAME,
	PAPER_LIST_NUM_COLUMNS
};


typedef struct {
	GladeXML      *gui;
	GtkWidget     *dialog;

	GtkWidget     *print_comment_checkbutton;
	GtkWidget     *comment_fontpicker;
	GtkWidget     *btn_close;
	GtkWidget     *btn_center;
	GtkWidget     *btn_print;
	GtkWidget     *hscale1;
	GtkWidget     *unit_optionmenu;
	GtkWidget     *width_spinbutton;
	GtkWidget     *height_spinbutton;
	GtkWidget     *paper_size_a4_radiobutton;
	GtkWidget     *paper_size_letter_radiobutton;
	GtkWidget     *paper_size_legal_radiobutton;
	GtkWidget     *paper_size_executive_radiobutton;
	GtkWidget     *paper_size_custom_radiobutton;
	GtkWidget     *comment_font_hbox;

	GtkAdjustment *adj;

	PrintInfo     *pi;
} DialogData;


static void 
print_info_ref (PrintInfo *pi)
{
	g_return_if_fail (pi != NULL);
	pi->ref_count++;
}


static void 
print_info_unref (PrintInfo *pi)
{
	g_return_if_fail (pi != NULL);
	g_return_if_fail (pi->ref_count > 0);
	
	pi->ref_count--;

	if (pi->ref_count == 0) {
		if (pi->gpj != NULL)
			g_object_unref (pi->gpj);
		gnome_print_config_unref (pi->config);
		g_free (pi->image_path);
		if (pi->pixbuf != NULL)
			g_object_unref (pi->pixbuf);
		if (pi->font_comment != NULL)
			g_object_unref (pi->font_comment);
		g_free (pi->comment);
		g_free (pi);
	}
}


/* The following functions are from gedit-print.c
 * Copyright (C) 2000, 2001 Chema Celorio, Paolo Maggi
 * Copyright (C) 2002  Paolo Maggi  
 * Modified for gthumb by Paolo Bacchilega, 2003.
 */

static const char*
get_next_line_to_print_delimiter (PrintInfo  *pi, 
				  const char *start,
				  const char *end,
				  double     *line_width)
{
	const char  *p;
	double       current_width = 0.0;
	double       printable_page_width;
	ArtPoint     space_advance;
	int          space;
	
	printable_page_width = pi->paper_width - pi->paper_rmargin - pi->paper_lmargin;
	
	/* Find space advance */
	space = gnome_font_lookup_default (pi->font_comment, ' ');
	gnome_font_get_glyph_stdadvance (pi->font_comment, space, &space_advance);

	for (p = start; p < end; p = g_utf8_next_char (p)) {
		gunichar ch;
		int      glyph;
		
		ch = g_utf8_get_char (p);
		glyph = gnome_font_lookup_default (pi->font_comment, ch);

		if (glyph == space)
			current_width += space_advance.x;
		else {
			ArtPoint adv;
			gnome_font_get_glyph_stdadvance (pi->font_comment, 
							 glyph,
							 &adv);
			if (adv.x > 0)
				current_width += adv.x;
			else
				/* To be as conservative as possible */
				current_width += (2 * space_advance.x);
			
		}

		if (current_width > printable_page_width) {
			if (line_width != NULL)
				*line_width = printable_page_width;
			return p;
		}
	}
	
	if (line_width != NULL)
		*line_width = current_width;

	return end;
}


static void
print_line (GnomePrintContext *pc,
	    PrintInfo         *pi, 
	    const char        *start, 
	    const char        *end, 
	    double             x,
	    double             y) 
{
	GnomeGlyphList *gl;
	const char     *p;
	int             chars_in_this_line = 0;
	
	gl = gnome_glyphlist_from_text_dumb (pi->font_comment, 0x000000ff, 0.0, 0.0, "");
	gnome_glyphlist_moveto (gl, x, y - gnome_font_get_ascender (pi->font_comment));
	
	for (p = start; p < end; p = g_utf8_next_char (p)) {
		gunichar ch;
	       	int      glyph;
		
		ch = g_utf8_get_char (p);
		++chars_in_this_line;

		glyph = gnome_font_lookup_default (pi->font_comment, ch);
		gnome_glyphlist_glyph (gl, glyph);
	}

	gnome_print_moveto (pc, 0.0, 0.0);
	gnome_print_glyphlist (pc, gl);
	gnome_glyphlist_unref (gl);
}


static gdouble
print_paragraph (GnomePrintContext *pc,
		 PrintInfo         *pi,
		 const char        *start, 
		 const char        *end, 
		 double             x,
		 double             y)
{
	const char *p, *s;

	for (p = start; p < end; ) {
		s = p;
		p = get_next_line_to_print_delimiter (pi, s, end, NULL);
		print_line (pc, pi, s, p, x, y);
		y -= 1.2 * gnome_font_get_size (pi->font_comment);
	}

	return y;
}


static void
get_text_extents (PrintInfo   *pi,
		  const char  *start, 
		  const char  *text_end,
		  double      *width,
		  double      *height)
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
			*height += 1.2 * gnome_font_get_size (pi->font_comment);
		else {
			const char *p1, *s1;

			for (p1 = p; p1 < end; ) {
				double line_width;

				s1 = p1;
				p1 = get_next_line_to_print_delimiter (pi, s1, end, &line_width);
				*width = MAX (*width, line_width);
				*height += 1.2 * gnome_font_get_size (pi->font_comment);
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
print_comment (GnomePrintContext *pc,
	       PrintInfo         *pi)
{
	const char *p;
	const char *end;
	double      fontheight;
	double      x, y, width, height;
	double      printable_width;
	int         paragraph_delimiter_index;
	int         next_paragraph_start;
	char       *text_end;

	gnome_print_setfont (pc, pi->font_comment);

	p = pi->comment;
	text_end = pi->comment + strlen (pi->comment);

	get_text_extents (pi, p, text_end, &width, &height);
	printable_width = pi->paper_width - pi->paper_lmargin - pi->paper_rmargin;
	x = pi->paper_lmargin + MAX (0, (printable_width - width) / 2);
	y = pi->paper_bmargin + height;

	pango_find_paragraph_boundary (pi->comment, -1, 
				       &paragraph_delimiter_index, 
				       &next_paragraph_start);

	end = pi->comment + paragraph_delimiter_index;

	fontheight = (gnome_font_get_ascender (pi->font_comment) + 
		      gnome_font_get_descender (pi->font_comment));

	while (p < text_end) {
		gunichar wc = g_utf8_get_char (p);

		if ((wc == '\n' ||
           	     wc == '\r' ||
                     wc == PARAGRAPH_SEPARATOR)) {
	
			y -= 1.2 * gnome_font_get_size (pi->font_comment);
			
			if ((y - pi->paper_bmargin) < fontheight) {
				/* FIXME
				gnome_print_showpage (pc);
				gnome_print_beginpage (pc, NULL);

				x = pi->paper_lmargin + MAX (0, (printable_width - width) / 2);
				y = pi->paper_bmargin + height;
				*/
				/* text do not fit. */
				/*return;*/
			}
		} else {
			y = print_paragraph (pc, pi, p, end, x, y);
			
			if ((y - pi->paper_bmargin) < fontheight) {
				/* FIXME
				gnome_print_showpage (pc);
				gnome_print_beginpage (pc, NULL);

				x = pi->paper_lmargin + MAX (0, (printable_width - width) / 2);
				y = pi->paper_bmargin + height;
				*/
				/* text do not fit. */
				/*return;*/
			}
		}

		p = p + next_paragraph_start;

		if (p < text_end) {
			pango_find_paragraph_boundary (p, -1, &paragraph_delimiter_index, &next_paragraph_start);
			end = p + paragraph_delimiter_index;
		}
	}

	return pi->paper_bmargin + height;
}


static void
print_image (GnomePrintContext *pc,
	     PrintInfo         *pi, 
	     gboolean           border) 
{
	double  w, h;
	double  lmargin, rmargin, tmargin, bmargin;
	int     pw, ph, rs;
        guchar *p;

	w = pi->paper_width;
	h = pi->paper_height;
	lmargin = pi->paper_lmargin;
	rmargin = pi->paper_rmargin;
	bmargin = pi->paper_bmargin;
	tmargin = pi->paper_tmargin;

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

	if (pi->print_comment) {
		/* FIXME 
		gnome_print_showpage (pc);
		gnome_print_beginpage (pc, NULL);
		*/

		gnome_print_gsave (pc);
		print_comment (pc, pi);
		gnome_print_grestore (pc);
	}

	p = gdk_pixbuf_get_pixels (pi->pixbuf);
        pw = gdk_pixbuf_get_width (pi->pixbuf);
        ph = gdk_pixbuf_get_height (pi->pixbuf);
        rs = gdk_pixbuf_get_rowstride (pi->pixbuf);

	gnome_print_gsave (pc);
	gnome_print_scale (pc, pi->scale_x, pi->scale_y);
	gnome_print_translate (pc, pi->trans_x, pi->trans_y);
	if (pi->use_colors) {
		if (gdk_pixbuf_get_has_alpha (pi->pixbuf)) 
			gnome_print_rgbaimage (pc, p, pw, ph, rs);
		else 
			gnome_print_rgbimage  (pc, p, pw, ph, rs);
	} else
		gnome_print_grayimage (pc, p, pw, ph, rs);
        gnome_print_grestore (pc);


	gnome_print_showpage (pc);
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
check_bounds (PrintInfo *pi, double *x1, double *y1)
{
	*x1 = MAX (*x1, pi->min_x);
	*x1 = MIN (*x1, pi->max_x - pi->image_w * pi->zoom);
	*y1 = MAX (*y1, pi->min_y);
	*y1 = MIN (*y1, pi->max_y - pi->image_h * pi->zoom);
}


static int
item_event (GnomeCanvasItem *item, 
	    GdkEvent        *event, 
	    PrintInfo       *pi)
{
	static double  start_x, start_y;
	static double  img_start_x, img_start_y;
	static int     dragging;
	double         x, y;
	GdkCursor     *fleur;

	x = event->button.x;
	y = event->button.y;

	switch (event->type) {
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

			check_bounds (pi, &x1, &y1);

			gnome_canvas_item_set (item,
					       "x", x1,
					       "y", y1,
					       NULL);
		}
		break;

	case GDK_BUTTON_RELEASE:
		gnome_canvas_item_ungrab (item, event->button.time);
		dragging = FALSE;
		break;

	default:
		break;
	}

	return FALSE;
}


static void
add_image_preview (PrintInfo *pi, 
		   gboolean border)
{
	double            w, h;
	double            lmargin, rmargin, tmargin, bmargin;
	double            iw, ih;
	double            max_w, max_h;
	GnomeCanvasGroup *group;
	double            factor;
	double            comment_height;
	int               layout_width;

 	group = GNOME_CANVAS_GROUP (gnome_canvas_root (GNOME_CANVAS (pi->canvas)));

	w = pi->paper_width;
	h = pi->paper_height;
	lmargin = pi->paper_lmargin;
	rmargin = pi->paper_rmargin;
	bmargin = pi->paper_bmargin;
	tmargin = pi->paper_tmargin;
	iw = (double) gdk_pixbuf_get_width (pi->pixbuf); 
	ih = (double) gdk_pixbuf_get_height (pi->pixbuf);

	/* Insert comment */

	layout_width = (int) ((pi->paper_width - lmargin - rmargin) * CANVAS_ZOOM);

	if (pi->print_comment && (pi->comment != NULL)) {
		static char gray50_bits[] = {
			0x0,
			0x0,
			0x1,
			0x1,
			0x1,
		};
		GdkBitmap  *stipple;
		const char *p;
		const char *text_end;
		double      width, height;
	
		p = pi->comment;
		text_end = pi->comment + strlen (pi->comment);
		get_text_extents (pi, p, text_end, &width, &height);

		comment_height = height;

		stipple = gdk_bitmap_create_from_data (NULL, gray50_bits, gray50_width, gray50_height);

		pi->ci_comment = gnome_canvas_item_new (
					group,
					gnome_canvas_rect_get_type (),
					"x1",             lmargin,
					"y1",             pi->paper_height - bmargin,
					"x2",             lmargin + width,
					"y2",             pi->paper_height - bmargin - height,
					"fill_color",     "darkgray",
					"fill_stipple",   stipple,
					NULL);

		g_object_unref (stipple);
	} else 
		comment_height = 0;

	/* Scale if image does not fit in the window. */

	max_w = w - lmargin - rmargin;
	max_h = h - bmargin - tmargin - comment_height;

	factor = MIN (max_w / iw, max_h / ih);
	ih *= factor;
	iw *= factor;

	pi->image_w = iw;
	pi->image_h = ih;

	/* Center & check bounds. */

	pi->zoom = 1.0;

	pi->min_x = lmargin;
	pi->min_y = tmargin;
	pi->max_x = lmargin + max_w;
	pi->max_y = tmargin + max_h;

	pi->trans_x = MAX ((w - iw) / 2, lmargin);
	pi->trans_y = MAX ((h - ih) / 2, bmargin);
	check_bounds (pi, &pi->trans_x, &pi->trans_y);

	if (border) 
		gnome_canvas_item_new (
			gnome_canvas_root ( GNOME_CANVAS (pi->canvas)),
			gnome_canvas_rect_get_type (),
			"x1",   	 lmargin,
			"y1",   	 tmargin,
			"x2",   	 lmargin + max_w,
			"y2",   	 tmargin + max_h + comment_height,
			"outline_color", "gray",
			"width_pixels",  1,
			NULL);

	iw = MAX (1.0, iw);
	ih = MAX (1.0, ih);

	pi->ci_image = gnome_canvas_item_new (
		      group,
		      gnome_canvas_pixbuf_get_type (),
		      "pixbuf",     pi->pixbuf,
		      "x",          pi->trans_x,
		      "y",          pi->trans_y,
		      "width",      iw,
		      "width_set",  TRUE,
		      "height",     ih,
		      "height_set", TRUE,
		      "anchor",     GTK_ANCHOR_NW,
		      NULL);

	if (pi->ci_image == NULL)
		g_error ("Cannot create image preview\n");

	g_signal_connect (G_OBJECT (pi->ci_image), 
			  "event",
			  G_CALLBACK (item_event),
			  pi);
}


/* called when the main dialog is closed. */
static void
destroy_cb (GtkWidget *widget, 
	    DialogData *data)
{
	g_object_unref (data->gui);
	print_info_unref (data->pi);
	g_free (data);
}


/* called when the center button is clicked. */
static void
center_cb (GtkWidget *widget, 
	   DialogData *data)
{
	PrintInfo *pi = data->pi;
	gdouble x1, y1, w, h;

	w = pi->max_x - pi->min_x;
	h = pi->max_y - pi->min_y;
	x1 = pi->min_x + (w - pi->image_w * pi->zoom) / 2;
	y1 = pi->min_y + (h - pi->image_h * pi->zoom) / 2;

	gnome_canvas_item_set (pi->ci_image,
			       "x", x1,
			       "y", y1,
			       NULL);
}


static void 
zoom_changed (GtkAdjustment *adj,
	      DialogData *data)
{
	PrintInfo *pi = data->pi;
	gdouble w, h;
	gdouble x1, y1;

	pi->zoom = adj->value / 100.0;
	w = pi->image_w * pi->zoom;
	h = pi->image_h * pi->zoom;

	gnome_canvas_item_set (pi->ci_image,
			       "width",      w,
			       "width_set",  TRUE,
			       "height",     h,
			       "height_set", TRUE,
			       NULL);

	/* Keep image within page borders. */

	g_object_get (G_OBJECT (pi->ci_image),
		      "x", &x1,
		      "y", &y1,
		      NULL);
	x1 = MAX (x1, pi->min_x);
	x1 = MIN (x1, pi->max_x - pi->image_w * pi->zoom);
	y1 = MAX (y1, pi->min_y);
	y1 = MIN (y1, pi->max_y - pi->image_h * pi->zoom);

	gnome_canvas_item_set (pi->ci_image,
			       "x", x1,
			       "y", y1,
			       NULL);
}


static void
update_comment_font (DialogData *data)
{
	PrintInfo *pi = data->pi;
	const char *font_name;

	if (pi->font_comment != NULL)
		g_object_unref (pi->font_comment);		

	font_name = gnome_print_font_picker_get_font_name (GNOME_PRINT_FONT_PICKER (data->comment_fontpicker));

	debug (DEBUG_INFO, "Find closest: %s", font_name);

	pi->font_comment = gnome_font_find_closest_from_full_name (font_name);

	if (pi->font_comment == NULL)
		g_warning ("Could not find font %s\n", font_name);
}


static GthPrintUnit
get_current_unit (DialogData *data)
{
	return gtk_option_menu_get_history (GTK_OPTION_MENU (data->unit_optionmenu));
}


static double
get_current_unittobase (DialogData *data)
{
	const GnomePrintUnit *unit;
	unit = &print_units[gtk_option_menu_get_history (GTK_OPTION_MENU (data->unit_optionmenu))];
	return unit->unittobase;
}


static double 
get_page_width (DialogData *data)
{
	const GnomePrintUnit *unit;
	double w;

	gnome_print_config_get_length (data->pi->config, GNOME_PRINT_KEY_PAPER_WIDTH, &w, &unit);
	w = w * unit->unittobase / get_current_unittobase (data);
	
	return w;
}


static double 
get_page_height (DialogData *data)
{
	const GnomePrintUnit *unit;
	double h;

	gnome_print_config_get_length (data->pi->config, GNOME_PRINT_KEY_PAPER_HEIGHT, &h, &unit);
	h = h * unit->unittobase / get_current_unittobase (data);
	
	return h;
}


static const char *
get_current_paper_size (DialogData *data)
{
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->paper_size_letter_radiobutton)))
		return "USLetter";
	else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->paper_size_legal_radiobutton)))
		return "USLegal";
	else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->paper_size_executive_radiobutton)))
		return "Executive";
	else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->paper_size_a4_radiobutton)))
		return "A4";
	else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->paper_size_custom_radiobutton)))
		return "Custom";
	else
		return "A4";

}


/* called when the print button is clicked. */
static void 
print_cb (GtkWidget *widget,
	  DialogData *data)
{
	PrintInfo         *pi = data->pi;
	GnomePrintContext *gp_ctx;
	double             image_x, image_y;
	char              *title;
	GtkWidget         *dialog;
	int                response;
	char              *value;
	double             length;

	/* Save options */

	eel_gconf_set_string (PREF_PRINT_PAPER_SIZE, get_current_paper_size (data));

	length = get_page_width (data);
	eel_gconf_set_float (PREF_PRINT_PAPER_WIDTH, length);

	length = get_page_height (data);
	eel_gconf_set_float (PREF_PRINT_PAPER_HEIGHT, length);

	pref_set_print_unit (get_current_unit (data));

	value = gnome_print_config_get (pi->config, GNOME_PRINT_KEY_PAGE_ORIENTATION);
	eel_gconf_set_string (PREF_PRINT_PAPER_ORIENTATION, value);
	g_free (value);

	eel_gconf_set_boolean (PREF_PRINT_INCLUDE_COMMENT, !gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (data->print_comment_checkbutton)) && gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->print_comment_checkbutton)));

	eel_gconf_set_string (PREF_PRINT_COMMENT_FONT, gnome_print_font_picker_get_font_name (GNOME_PRINT_FONT_PICKER (data->comment_fontpicker)));

	/**/

	g_object_get (G_OBJECT (pi->ci_image),
		      "x", &image_x,
		      "y", &image_y,
		      NULL);
	pi->scale_x = pi->image_w * pi->zoom;
	pi->scale_y = pi->image_h * pi->zoom;
	pi->trans_x = image_x / pi->scale_x;
	pi->trans_y = (pi->paper_height - pi->scale_y - image_y) / pi->scale_y;

	/* pi is used to print, so we must add a reference. */

	print_info_ref (pi);
	gtk_widget_hide (data->dialog); 

	/* Gnome Print Dialog */

	update_comment_font (data);

	pi->gpj = gnome_print_job_new (pi->config);

	if (pi->image_path != NULL)
		title = g_strdup_printf (_("Print %s"), 
					 file_name_from_path (pi->image_path));
	else
		title = g_strdup (_("Print Image"));
	dialog = gnome_print_dialog_new (pi->gpj, title, 0);
        response = gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_destroy (dialog);

	gp_ctx = gnome_print_job_get_context (pi->gpj);
	print_image (gp_ctx, pi, FALSE);
	gnome_print_job_close (pi->gpj);

        switch (response) {
        case GNOME_PRINT_DIALOG_RESPONSE_PRINT:
		gnome_print_job_print (pi->gpj);	
                break;

        case GNOME_PRINT_DIALOG_RESPONSE_PREVIEW:
		gtk_widget_show (gnome_print_job_preview_new (pi->gpj, title));
                break;

        default:
		break;
        }
 
	g_free (title);

	gtk_widget_destroy (data->dialog); 
	print_info_unref (pi);
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
	guchar   *orientation;
	gboolean  portrait;

	orientation = gnome_print_config_get (config, GNOME_PRINT_KEY_PAGE_ORIENTATION);
	portrait = ((strcmp (orientation, "R0") == 0) || (strcmp (orientation, "R180") == 0));
	g_free (orientation);

	return portrait;
}


static void
update_page (DialogData *data)
{
	PrintInfo *pi = data->pi;
	const      GnomePrintUnit *unit;
	const      GnomePrintUnit *ps_unit;
	double     width, height, lmargin, tmargin, rmargin, bmargin;
	double     paper_width, paper_height, paper_lmargin, paper_tmargin;
	double     paper_rmargin, paper_bmargin;

	ps_unit = gnome_print_unit_get_identity (GNOME_PRINT_UNIT_DIMENSIONLESS);

	if (gnome_print_config_get_length (pi->config, 
					   GNOME_PRINT_KEY_PAPER_WIDTH,
					   &width,
					   &unit))
		gnome_print_convert_distance (&width, unit, ps_unit);
	if (gnome_print_config_get_length (pi->config, 
					   GNOME_PRINT_KEY_PAPER_HEIGHT,
					   &height,
					   &unit))
		gnome_print_convert_distance (&height, unit, ps_unit);
	if (gnome_print_config_get_length (pi->config, 
					   GNOME_PRINT_KEY_PAGE_MARGIN_LEFT,
					   &lmargin,
					   &unit))
		gnome_print_convert_distance (&lmargin, unit, ps_unit);
	if (gnome_print_config_get_length (pi->config, 
					   GNOME_PRINT_KEY_PAGE_MARGIN_RIGHT,
					   &rmargin,
					   &unit))
		gnome_print_convert_distance (&rmargin, unit, ps_unit);
	if (gnome_print_config_get_length (pi->config, 
					   GNOME_PRINT_KEY_PAGE_MARGIN_TOP,
					   &tmargin,
					   &unit))
		gnome_print_convert_distance (&tmargin, unit, ps_unit);
	if (gnome_print_config_get_length (pi->config, 
					   GNOME_PRINT_KEY_PAGE_MARGIN_BOTTOM,
					   &bmargin,
					   &unit))
		gnome_print_convert_distance (&bmargin, unit, ps_unit);

	pi->portrait = orientation_is_portrait (pi->config);

	if (pi->portrait) {
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

	pi->paper_width   = paper_width;
	pi->paper_height  = paper_height;
	pi->paper_lmargin = paper_lmargin;
	pi->paper_tmargin = paper_tmargin;
	pi->paper_rmargin = paper_rmargin;
	pi->paper_bmargin = paper_bmargin;

	clear_canvas (GNOME_CANVAS_GROUP (GNOME_CANVAS (pi->canvas)->root));
	gnome_canvas_set_scroll_region (GNOME_CANVAS (pi->canvas), 
					0, 0,
					pi->paper_width, pi->paper_height);

	add_simulated_page (GNOME_CANVAS (pi->canvas));
	add_image_preview (pi, TRUE);

	/* Reset zoom to 100%. */

	g_signal_handlers_block_by_data (G_OBJECT (data->adj), data);
	gtk_adjustment_set_value (data->adj, 100.0);
	g_signal_handlers_unblock_by_data (G_OBJECT (data->adj), data);
}


static void
print_comment_cb (GtkWidget  *widget,
		  DialogData *data)
{
	data->pi->print_comment = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));
	gtk_widget_set_sensitive (data->comment_font_hbox, data->pi->print_comment);
	update_page ((DialogData*) data);
	center_cb (NULL, data);
}


static void
portrait_toggled_cb (GtkToggleButton  *widget,
		     DialogData       *data)
{
	if (! gtk_toggle_button_get_active (widget))
		return;
	gnome_print_config_set (data->pi->config, GNOME_PRINT_KEY_PAGE_ORIENTATION, "R0");
	update_page (data);
}


static void
landscape_toggled_cb (GtkToggleButton  *widget,
		      DialogData       *data)
{
	if (! gtk_toggle_button_get_active (widget))
		return;
	gnome_print_config_set (data->pi->config, GNOME_PRINT_KEY_PAGE_ORIENTATION, "R90");
	update_page (data);
}


static void
update_custom_page_size (DialogData *data)
{
	const GnomePrintUnit *unit;
	double width, height;

	unit = &print_units[gtk_option_menu_get_history (GTK_OPTION_MENU (data->unit_optionmenu))];
	width = gtk_spin_button_get_value (GTK_SPIN_BUTTON (data->width_spinbutton));
	height = gtk_spin_button_get_value (GTK_SPIN_BUTTON (data->height_spinbutton));
	gnome_print_config_set_length (data->pi->config, GNOME_PRINT_KEY_PAPER_WIDTH, width, unit);
	gnome_print_config_set_length (data->pi->config, GNOME_PRINT_KEY_PAPER_HEIGHT, height, unit);

	update_page (data);
}


static void
custom_size_toggled_cb (GtkToggleButton  *widget,
			DialogData       *data)
{

	if (! gtk_toggle_button_get_active (widget))
		return;
	gnome_print_config_set (data->pi->config, GNOME_PRINT_KEY_PAPER_SIZE, "Custom");
	update_custom_page_size (data);
}


static void
custom_size_value_changed_cb (GtkSpinButton *spin,
			      DialogData    *data)
{
	if (! gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->paper_size_custom_radiobutton))) {
		g_signal_handlers_block_by_func (data->paper_size_custom_radiobutton, custom_size_toggled_cb, data);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->paper_size_custom_radiobutton), TRUE);
		g_signal_handlers_unblock_by_func (data->paper_size_custom_radiobutton, custom_size_toggled_cb, data);
	}

	update_custom_page_size (data);
}


static void
update_page_size_from_config (DialogData *data)
{
	g_signal_handlers_block_by_func (data->width_spinbutton, custom_size_value_changed_cb, data);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->width_spinbutton), get_page_width (data));
	g_signal_handlers_unblock_by_func (data->width_spinbutton, custom_size_value_changed_cb, data);

	g_signal_handlers_block_by_func (data->height_spinbutton, custom_size_value_changed_cb, data);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->height_spinbutton), get_page_height (data));
	g_signal_handlers_unblock_by_func (data->height_spinbutton, custom_size_value_changed_cb, data);

}


static void
unit_changed_cb (GtkOptionMenu *opt_menu,
		 DialogData    *data)
{
	update_page_size_from_config (data);
}


static void
letter_size_toggled_cb (GtkToggleButton  *widget,
			DialogData       *data)
{
	if (! gtk_toggle_button_get_active (widget))
		return;
	gnome_print_config_set (data->pi->config, GNOME_PRINT_KEY_PAPER_SIZE, "USLetter");
	update_page_size_from_config (data);
	update_page (data);
}


static void
legal_size_toggled_cb (GtkToggleButton  *widget,
		       DialogData       *data)
{
	if (! gtk_toggle_button_get_active (widget))
		return;
	gnome_print_config_set (data->pi->config, GNOME_PRINT_KEY_PAPER_SIZE, "USLegal");
	update_page_size_from_config (data);
	update_page (data);
}


static void
executive_size_toggled_cb (GtkToggleButton  *widget,
			   DialogData       *data)
{
	if (! gtk_toggle_button_get_active (widget))
		return;
	gnome_print_config_set (data->pi->config, GNOME_PRINT_KEY_PAPER_SIZE, "Executive");
	update_page_size_from_config (data);
	update_page (data);
}


static void
a4_size_toggled_cb (GtkToggleButton  *widget,
		    DialogData       *data)
{
	if (! gtk_toggle_button_get_active (widget))
		return;
	gnome_print_config_set (data->pi->config, GNOME_PRINT_KEY_PAPER_SIZE, "A4");
	update_page_size_from_config (data);
	update_page (data);
}


static void
comment_fontpicker_font_set_cb (GnomePrintFontPicker *fontpicker,
				const char           *font_name,
				DialogData           *data)
{
	update_comment_font (data);
	update_page (data);
}


#define DEFAULT_FONT_SIZE 10


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


static void
set_page_info (DialogData *data)
{
	char      *txt;
	GtkWidget *page_label;

	page_label = glade_xml_get_widget (data->gui, "page_label");

	txt = g_strdup_printf (_("Page %d of %d"), 1, 1);
	gtk_label_set_text (GTK_LABEL (page_label), txt);
	g_free (txt);
}


void
print_image_dlg (GtkWindow   *parent, 
		 ImageViewer *viewer,
		 const char  *location)
{
	PrintInfo    *pi;
	DialogData   *data;
	CommentData  *cdata = NULL;
	char         *title = NULL;
	GtkWidget    *radio_button;
	GtkWidget    *comment_fontpicker_hbox;
	GtkWidget    *print_notebook;
	char         *value;
	char         *button_name;

	if (image_viewer_is_void (viewer))
		return;

	pi = g_new0 (PrintInfo, 1);
	pi->ref_count = 1;
	pi->zoom = 1.0;

	if (location != NULL) {
		GnomeVFSURI  *uri = gnome_vfs_uri_new (location);

		if (gnome_vfs_uri_is_local (uri)) {
			pi->image_path = gnome_vfs_get_local_path_from_uri (location);
			cdata = comments_load_comment (pi->image_path);
		}

		gnome_vfs_uri_unref (uri);
	}

	if (cdata != NULL) {
		pi->comment = comments_get_comment_as_string (cdata, "\n", " - ");
		comment_data_free (cdata);
	}

	pi->config = gnome_print_config_default ();

	pi->pixbuf = image_viewer_get_current_pixbuf (viewer);
	if (pi->pixbuf == NULL) {
		g_warning ("Cannot load image %s", location);
		print_info_unref (pi);
		return;
	}
	g_object_ref (pi->pixbuf);

	pi->image_w = (double) gdk_pixbuf_get_width (pi->pixbuf);
	pi->image_h = (double) gdk_pixbuf_get_height (pi->pixbuf);
	pi->portrait = TRUE;
	pi->use_colors = TRUE;

	data = g_new (DialogData, 1);
	data->pi = pi;
	data->gui = glade_xml_new (GTHUMB_GLADEDIR "/" GLADE_FILE, NULL, NULL);
        if (! data->gui) {
                g_warning ("Cannot find " GLADE_FILE "\n");
		print_info_unref (pi);
		g_free (data);
                return;
        }

	/* Get the widgets. */

	data->dialog = glade_xml_get_widget (data->gui, "page_setup_dialog");
	data->print_comment_checkbutton = glade_xml_get_widget (data->gui, "print_comment_checkbutton");
	data->comment_font_hbox = glade_xml_get_widget (data->gui, "comment_font_hbox");
	comment_fontpicker_hbox = glade_xml_get_widget (data->gui, "comment_fontpicker_hbox");
	data->hscale1 = glade_xml_get_widget (data->gui, "hscale");
	data->unit_optionmenu = glade_xml_get_widget (data->gui, "unit_optionmenu");
	data->width_spinbutton = glade_xml_get_widget (data->gui, "width_spinbutton");
	data->height_spinbutton = glade_xml_get_widget (data->gui, "height_spinbutton");
	data->paper_size_a4_radiobutton = glade_xml_get_widget (data->gui, "paper_size_a4_radiobutton");
	data->paper_size_letter_radiobutton = glade_xml_get_widget (data->gui, "paper_size_letter_radiobutton");
	data->paper_size_legal_radiobutton = glade_xml_get_widget (data->gui, "paper_size_legal_radiobutton");
	data->paper_size_executive_radiobutton = glade_xml_get_widget (data->gui, "paper_size_executive_radiobutton");
	data->paper_size_custom_radiobutton = glade_xml_get_widget (data->gui, "paper_size_custom_radiobutton");
	data->btn_center = glade_xml_get_widget (data->gui, "btn_center");
	data->btn_close = glade_xml_get_widget (data->gui, "btn_close");
	data->btn_print = glade_xml_get_widget (data->gui, "btn_print");

	pi->canvas = glade_xml_get_widget (data->gui, "canvas");

	data->comment_fontpicker = gnome_print_font_picker_new ();
	gnome_print_font_picker_set_mode (GNOME_PRINT_FONT_PICKER (data->comment_fontpicker),
				    GNOME_FONT_PICKER_MODE_FONT_INFO);
	gnome_print_font_picker_fi_set_use_font_in_label (GNOME_PRINT_FONT_PICKER (data->comment_fontpicker), TRUE, get_desktop_default_font_size ());
	gnome_print_font_picker_fi_set_show_size (GNOME_PRINT_FONT_PICKER (data->comment_fontpicker), TRUE);

	gtk_widget_show (data->comment_fontpicker);
	gtk_container_add (GTK_CONTAINER (comment_fontpicker_hbox), data->comment_fontpicker);

	data->adj = gtk_range_get_adjustment (GTK_RANGE (data->hscale1));

	/* Set widgets data. */

	set_page_info (data);

	print_notebook = glade_xml_get_widget (data->gui, "print_notebook");

	if (pi->comment != NULL) {
		gboolean include_comment;

		gtk_widget_set_sensitive (data->print_comment_checkbutton, TRUE);

		include_comment = eel_gconf_get_boolean (PREF_PRINT_INCLUDE_COMMENT, FALSE);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->print_comment_checkbutton), include_comment);
		pi->print_comment = include_comment;
	} else {
		gtk_widget_set_sensitive (data->print_comment_checkbutton, FALSE);
		gtk_toggle_button_set_inconsistent (GTK_TOGGLE_BUTTON (data->print_comment_checkbutton), TRUE);
		pi->print_comment = FALSE;
	}

	gtk_widget_set_sensitive (data->comment_font_hbox, pi->print_comment);

	value = eel_gconf_get_string (PREF_PRINT_COMMENT_FONT, DEF_COMMENT_FONT);
	if ((value != NULL) && (*value != 0))
		gnome_print_font_picker_set_font_name (GNOME_PRINT_FONT_PICKER (data->comment_fontpicker), value);
	else
		gnome_print_font_picker_set_font_name (GNOME_PRINT_FONT_PICKER (data->comment_fontpicker), DEF_COMMENT_FONT);
	g_free (value);

	gnome_print_font_picker_fi_set_use_font_in_label (GNOME_PRINT_FONT_PICKER (data->comment_fontpicker), TRUE, get_desktop_default_font_size ());

	update_comment_font (data);

	gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->width_spinbutton), eel_gconf_get_float (PREF_PRINT_PAPER_WIDTH, DEF_PAPER_WIDTH));
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->height_spinbutton), eel_gconf_get_float (PREF_PRINT_PAPER_HEIGHT, DEF_PAPER_HEIGHT));
	gtk_option_menu_set_history (GTK_OPTION_MENU (data->unit_optionmenu), pref_get_print_unit ());

	value = eel_gconf_get_string (PREF_PRINT_PAPER_SIZE, DEF_PAPER_SIZE);
	if (strcmp (value, "Custom") == 0) 
		update_custom_page_size (data);
	else {
		gnome_print_config_set (pi->config, GNOME_PRINT_KEY_PAPER_SIZE, value);
		update_page_size_from_config (data);
	}

	if (strcmp (value, "USLetter") == 0)
		radio_button = data->paper_size_letter_radiobutton;
	else if (strcmp (value, "USLegal") == 0)
		radio_button = data->paper_size_legal_radiobutton;
	else if (strcmp (value, "Executive") == 0)
		radio_button = data->paper_size_executive_radiobutton;
	else if (strcmp (value, "A4") == 0)
		radio_button = data->paper_size_a4_radiobutton;
	else if (strcmp (value, "Custom") == 0)
		radio_button = data->paper_size_custom_radiobutton;
	else
		radio_button = data->paper_size_a4_radiobutton;
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio_button), TRUE);
	g_free (value);

	/**/

	value = eel_gconf_get_string (PREF_PRINT_PAPER_ORIENTATION, DEF_PAPER_ORIENT);
	gnome_print_config_set (pi->config, GNOME_PRINT_KEY_PAGE_ORIENTATION, value);

	if (strcmp (value, "R0") == 0)
		button_name = "print_orient_portrait_radiobutton";
	else if (strcmp (value, "R90") == 0)
		button_name = "print_orient_landscape_radiobutton";
	radio_button = glade_xml_get_widget (data->gui, button_name);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio_button), TRUE);

	g_free (value);

	/* Set the signals handlers. */

	g_signal_connect (G_OBJECT (data->dialog), 
			  "destroy",
			  G_CALLBACK (destroy_cb),
			  data);
	g_signal_connect_swapped (G_OBJECT (data->btn_close), 
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  G_OBJECT (data->dialog));
	g_signal_connect (G_OBJECT (data->btn_center), 
			  "clicked",
			  G_CALLBACK (center_cb),
			  data);
	g_signal_connect (G_OBJECT (data->btn_print), 
			  "clicked",
			  G_CALLBACK (print_cb),
			  data);
	g_signal_connect (G_OBJECT (data->print_comment_checkbutton),
			  "toggled",
			  G_CALLBACK (print_comment_cb),
			  data);
	
	radio_button = glade_xml_get_widget (data->gui, "print_orient_portrait_radiobutton");
	g_signal_connect (G_OBJECT (radio_button),
			  "toggled",
			  G_CALLBACK (portrait_toggled_cb),
			  data);

	radio_button = glade_xml_get_widget (data->gui, "print_orient_landscape_radiobutton");
	g_signal_connect (G_OBJECT (radio_button),
			  "toggled",
			  G_CALLBACK (landscape_toggled_cb),
			  data);
	
	radio_button = data->paper_size_letter_radiobutton;
	g_signal_connect (G_OBJECT (radio_button),
			  "toggled",
			  G_CALLBACK (letter_size_toggled_cb),
			  data);

	radio_button = data->paper_size_legal_radiobutton;
	g_signal_connect (G_OBJECT (radio_button),
			  "toggled",
			  G_CALLBACK (legal_size_toggled_cb),
			  data);

	radio_button = data->paper_size_executive_radiobutton;
	g_signal_connect (G_OBJECT (radio_button),
			  "toggled",
			  G_CALLBACK (executive_size_toggled_cb),
			  data);

	radio_button = data->paper_size_a4_radiobutton;
	g_signal_connect (G_OBJECT (radio_button),
			  "toggled",
			  G_CALLBACK (a4_size_toggled_cb),
			  data);

	radio_button = data->paper_size_custom_radiobutton;
	g_signal_connect (G_OBJECT (radio_button),
			  "toggled",
			  G_CALLBACK (custom_size_toggled_cb),
			  data);

	g_signal_connect (G_OBJECT (data->comment_fontpicker),
			  "font_set",
			  G_CALLBACK (comment_fontpicker_font_set_cb),
			  data);

	g_signal_connect (G_OBJECT (data->adj), 
			  "value_changed",
			  G_CALLBACK (zoom_changed),
			  data);

	g_signal_connect (G_OBJECT (data->unit_optionmenu), 
			  "changed",
			  G_CALLBACK (unit_changed_cb),
			  data);

	g_signal_connect (G_OBJECT (data->width_spinbutton), 
			  "value_changed",
			  G_CALLBACK (custom_size_value_changed_cb),
			  data);
	g_signal_connect (G_OBJECT (data->height_spinbutton), 
			  "value_changed",
			  G_CALLBACK (custom_size_value_changed_cb),
			  data);

	/**/

	if (pi->image_path != NULL)
		title = g_strdup_printf (_("Print %s"), 
					 file_name_from_path (pi->image_path));
	gtk_window_set_title (GTK_WINDOW (data->dialog), title); 
	g_free (title);

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), parent);

	gnome_canvas_set_pixels_per_unit (GNOME_CANVAS (pi->canvas), 
					  CANVAS_ZOOM);

	update_page (data);
	center_cb (NULL, data);

	gtk_widget_show (data->dialog);
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
	GtkWidget     *paper_size_a4_radiobutton;
	GtkWidget     *paper_size_letter_radiobutton;
	GtkWidget     *paper_size_legal_radiobutton;
	GtkWidget     *paper_size_executive_radiobutton;
	GtkWidget     *paper_size_custom_radiobutton;
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
} PrintCatalogDialogData;


/* called when the main dialog is closed. */
static void
print_catalog_destroy_cb (GtkWidget              *widget, 
			  PrintCatalogDialogData *data)
{
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
	
	gl = gnome_glyphlist_from_text_dumb (pci->font_comment, 0x000000ff, 0.0, 0.0, "");
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
#define IMAGE_SPACE 36


static double log2 (double x)
{
	g_print ("%f / %f\n", log(x), log(2));
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

	idx = (int) floor (log2 (pci->images_per_page) + 0.5);
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
			image_info_rotate (image, 90);

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
	guchar    *orientation;
	double     width, height, lmargin, tmargin, rmargin, bmargin;
	double     paper_width, paper_height, paper_lmargin, paper_tmargin;
	double     paper_rmargin, paper_bmargin;
	gboolean   portrait;

	ps_unit = gnome_print_unit_get_identity (GNOME_PRINT_UNIT_DIMENSIONLESS);

	if (gnome_print_config_get_length (pci->config, 
					   GNOME_PRINT_KEY_PAPER_WIDTH,
					   &width,
					   &unit))
		gnome_print_convert_distance (&width, unit, ps_unit);
	if (gnome_print_config_get_length (pci->config, 
					   GNOME_PRINT_KEY_PAPER_HEIGHT,
					   &height,
					   &unit))
		gnome_print_convert_distance (&height, unit, ps_unit);
	if (gnome_print_config_get_length (pci->config, 
					   GNOME_PRINT_KEY_PAGE_MARGIN_LEFT,
					   &lmargin,
					   &unit))
		gnome_print_convert_distance (&lmargin, unit, ps_unit);
	if (gnome_print_config_get_length (pci->config, 
					   GNOME_PRINT_KEY_PAGE_MARGIN_RIGHT,
					   &rmargin,
					   &unit))
		gnome_print_convert_distance (&rmargin, unit, ps_unit);
	if (gnome_print_config_get_length (pci->config, 
					   GNOME_PRINT_KEY_PAGE_MARGIN_TOP,
					   &tmargin,
					   &unit))
		gnome_print_convert_distance (&tmargin, unit, ps_unit);
	if (gnome_print_config_get_length (pci->config, 
					   GNOME_PRINT_KEY_PAGE_MARGIN_BOTTOM,
					   &bmargin,
					   &unit))
		gnome_print_convert_distance (&bmargin, unit, ps_unit);

	orientation = gnome_print_config_get (pci->config, GNOME_PRINT_KEY_PAGE_ORIENTATION);
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
catalog_portrait_toggled_cb (GtkToggleButton        *widget,
			     PrintCatalogDialogData *data)
{
	if (! gtk_toggle_button_get_active (widget))
		return;
	gnome_print_config_set (data->pci->config, GNOME_PRINT_KEY_PAGE_ORIENTATION, "R0");
	catalog_update_page (data);
}


static void
catalog_landscape_toggled_cb (GtkToggleButton        *widget,
			      PrintCatalogDialogData *data)
{
	if (! gtk_toggle_button_get_active (widget))
		return;
	gnome_print_config_set (data->pci->config, GNOME_PRINT_KEY_PAGE_ORIENTATION, "R90");
	catalog_update_page (data);
}


static void
catalog_update_custom_page_size (PrintCatalogDialogData *data)
{
	const GnomePrintUnit *unit;
	double width, height;

	unit = &print_units[gtk_option_menu_get_history (GTK_OPTION_MENU (data->unit_optionmenu))];
	width = gtk_spin_button_get_value (GTK_SPIN_BUTTON (data->width_spinbutton));
	height = gtk_spin_button_get_value (GTK_SPIN_BUTTON (data->height_spinbutton));
	gnome_print_config_set_length (data->pci->config, GNOME_PRINT_KEY_PAPER_WIDTH, width, unit);
	gnome_print_config_set_length (data->pci->config, GNOME_PRINT_KEY_PAPER_HEIGHT, height, unit);

	catalog_update_page (data);
}


static void
catalog_custom_size_toggled_cb (GtkToggleButton        *widget,
				PrintCatalogDialogData *data)
{

	if (! gtk_toggle_button_get_active (widget))
		return;
	gnome_print_config_set (data->pci->config, GNOME_PRINT_KEY_PAPER_SIZE, "Custom");
	catalog_update_custom_page_size (data);
}


static void
catalog_custom_size_value_changed_cb (GtkSpinButton          *spin,
				      PrintCatalogDialogData *data)
{
	if (! gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->paper_size_custom_radiobutton))) {
		g_signal_handlers_block_by_func (data->paper_size_custom_radiobutton, catalog_custom_size_toggled_cb, data);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->paper_size_custom_radiobutton), TRUE);
		g_signal_handlers_unblock_by_func (data->paper_size_custom_radiobutton, catalog_custom_size_toggled_cb, data);
	}

	catalog_update_custom_page_size (data);
}


static double
catalog_get_current_unittobase (PrintCatalogDialogData *data)
{
	const GnomePrintUnit *unit;
	unit = &print_units[gtk_option_menu_get_history (GTK_OPTION_MENU (data->unit_optionmenu))];
	return unit->unittobase;
}


static double 
catalog_get_page_width (PrintCatalogDialogData *data)
{
	const GnomePrintUnit *unit;
	double w;

	gnome_print_config_get_length (data->pci->config, GNOME_PRINT_KEY_PAPER_WIDTH, &w, &unit);
	w = w * unit->unittobase / catalog_get_current_unittobase (data);
	
	return w;
}


static double 
catalog_get_page_height (PrintCatalogDialogData *data)
{
	const GnomePrintUnit *unit;
	double h;

	gnome_print_config_get_length (data->pci->config, GNOME_PRINT_KEY_PAPER_HEIGHT, &h, &unit);
	h = h * unit->unittobase / catalog_get_current_unittobase (data);
	
	return h;
}


static void
catalog_update_page_size_from_config (PrintCatalogDialogData *data)
{
	g_signal_handlers_block_by_func (data->width_spinbutton, catalog_custom_size_value_changed_cb, data);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->width_spinbutton), catalog_get_page_width (data));
	g_signal_handlers_unblock_by_func (data->width_spinbutton, catalog_custom_size_value_changed_cb, data);

	g_signal_handlers_block_by_func (data->height_spinbutton, catalog_custom_size_value_changed_cb, data);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->height_spinbutton), catalog_get_page_height (data));
	g_signal_handlers_unblock_by_func (data->height_spinbutton, catalog_custom_size_value_changed_cb, data);

}


static void
catalog_unit_changed_cb (GtkOptionMenu          *opt_menu,
			 PrintCatalogDialogData *data)
{
	catalog_update_page_size_from_config (data);
}


static void
catalog_letter_size_toggled_cb (GtkToggleButton        *widget,
				PrintCatalogDialogData *data)
{
	if (! gtk_toggle_button_get_active (widget))
		return;
	gnome_print_config_set (data->pci->config, GNOME_PRINT_KEY_PAPER_SIZE, "USLetter");
	catalog_update_page_size_from_config (data);
	catalog_update_page (data);
}


static void
catalog_legal_size_toggled_cb (GtkToggleButton        *widget,
			       PrintCatalogDialogData *data)
{
	if (! gtk_toggle_button_get_active (widget))
		return;
	gnome_print_config_set (data->pci->config, GNOME_PRINT_KEY_PAPER_SIZE, "USLegal");
	catalog_update_page_size_from_config (data);
	catalog_update_page (data);
}


static void
catalog_executive_size_toggled_cb (GtkToggleButton        *widget,
				   PrintCatalogDialogData *data)
{
	if (! gtk_toggle_button_get_active (widget))
		return;
	gnome_print_config_set (data->pci->config, GNOME_PRINT_KEY_PAPER_SIZE, "Executive");
	catalog_update_page_size_from_config (data);
	catalog_update_page (data);
}


static void
catalog_a4_size_toggled_cb (GtkToggleButton        *widget,
			    PrintCatalogDialogData *data)
{
	if (! gtk_toggle_button_get_active (widget))
		return;
	gnome_print_config_set (data->pci->config, GNOME_PRINT_KEY_PAPER_SIZE, "A4");
	catalog_update_page_size_from_config (data);
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
			GdkPixbuf *scaled;
			guchar    *p;
			int        pw, ph, rs;
			
			pw = (int) ceil (image->scale_x);
			ph = (int) ceil (image->scale_y);
			scaled = gdk_pixbuf_scale_simple (pixbuf, pw, ph, GDK_INTERP_BILINEAR);
			g_object_unref (pixbuf);

			p = gdk_pixbuf_get_pixels (scaled);
			pw = gdk_pixbuf_get_width (scaled);
			ph = gdk_pixbuf_get_height (scaled);
			rs = gdk_pixbuf_get_rowstride (scaled);

			gnome_print_gsave (pc);
			gnome_print_scale (pc, image->scale_x, image->scale_y);
			gnome_print_translate (pc, image->trans_x, image->trans_y);
			if (pci->use_colors) {
				if (gdk_pixbuf_get_has_alpha (scaled)) 
					gnome_print_rgbaimage (pc, p, pw, ph, rs);
				else 
					gnome_print_rgbimage  (pc, p, pw, ph, rs);
			} else
				gnome_print_grayimage (pc, p, pw, ph, rs);
			gnome_print_grestore (pc);
			
			g_object_unref (scaled);
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
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->paper_size_letter_radiobutton)))
		return "USLetter";
	else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->paper_size_legal_radiobutton)))
		return "USLegal";
	else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->paper_size_executive_radiobutton)))
		return "Executive";
	else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->paper_size_a4_radiobutton)))
		return "A4";
	else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->paper_size_custom_radiobutton)))
		return "Custom";
	else
		return "A4";
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

	pci->font_comment = gnome_font_find_closest_from_full_name (font_name);

	if (pci->font_comment == NULL)
		g_warning ("Could not find font %s\n", font_name);
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

	pref_set_print_unit (catalog_get_current_unit (data));

	value = gnome_print_config_get (pci->config, GNOME_PRINT_KEY_PAGE_ORIENTATION);
	eel_gconf_set_string (PREF_PRINT_PAPER_ORIENTATION, value);
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

	dialog = gnome_print_dialog_new (pci->gpj, _("Print"), 0);
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
		gtk_widget_show (gnome_print_job_preview_new (pci->gpj, _("Print")));
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

	g_print ("IPP: %d\n", data->pci->images_per_page);

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

	cdata = comments_load_comment (image->filename);
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
print_catalog_dlg (GtkWindow *parent,
		   GList     *file_list)
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
	data->paper_size_a4_radiobutton = glade_xml_get_widget (data->gui, "paper_size_a4_radiobutton");
	data->paper_size_letter_radiobutton = glade_xml_get_widget (data->gui, "paper_size_letter_radiobutton");
	data->paper_size_legal_radiobutton = glade_xml_get_widget (data->gui, "paper_size_legal_radiobutton");
	data->paper_size_executive_radiobutton = glade_xml_get_widget (data->gui, "paper_size_executive_radiobutton");
	data->paper_size_custom_radiobutton = glade_xml_get_widget (data->gui, "paper_size_custom_radiobutton");
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
	gtk_option_menu_set_history (GTK_OPTION_MENU (data->unit_optionmenu), pref_get_print_unit ());

	value = eel_gconf_get_string (PREF_PRINT_PAPER_SIZE, DEF_PAPER_SIZE);
	if (strcmp (value, "Custom") == 0) 
		catalog_update_custom_page_size (data);
	else {
		gnome_print_config_set (pci->config, GNOME_PRINT_KEY_PAPER_SIZE, value);
		catalog_update_page_size_from_config (data);
	}

	if (strcmp (value, "USLetter") == 0)
		radio_button = data->paper_size_letter_radiobutton;
	else if (strcmp (value, "USLegal") == 0)
		radio_button = data->paper_size_legal_radiobutton;
	else if (strcmp (value, "Executive") == 0)
		radio_button = data->paper_size_executive_radiobutton;
	else if (strcmp (value, "A4") == 0)
		radio_button = data->paper_size_a4_radiobutton;
	else if (strcmp (value, "Custom") == 0)
		radio_button = data->paper_size_custom_radiobutton;
	else
		radio_button = data->paper_size_a4_radiobutton;
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio_button), TRUE);
	g_free (value);

	gtk_option_menu_set_history (GTK_OPTION_MENU (data->images_per_page_optionmenu), (int) log2 (pci->images_per_page));

	/**/

	value = eel_gconf_get_string (PREF_PRINT_PAPER_ORIENTATION, DEF_PAPER_ORIENT);
	gnome_print_config_set (pci->config, GNOME_PRINT_KEY_PAGE_ORIENTATION, value);

	if (strcmp (value, "R0") == 0)
		button_name = "print_orient_portrait_radiobutton";
	else if (strcmp (value, "R90") == 0)
		button_name = "print_orient_landscape_radiobutton";
	radio_button = glade_xml_get_widget (data->gui, button_name);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio_button), TRUE);

	g_free (value);

	/**/

	value = eel_gconf_get_string (PREF_PRINT_COMMENT_FONT, DEF_COMMENT_FONT);
	if ((value != NULL) && (*value != 0))
		gnome_print_font_picker_set_font_name (GNOME_PRINT_FONT_PICKER (data->comment_fontpicker), value);
	else
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
	
	radio_button = data->paper_size_letter_radiobutton;
	g_signal_connect (G_OBJECT (radio_button),
			  "toggled",
			  G_CALLBACK (catalog_letter_size_toggled_cb),
			  data);

	radio_button = data->paper_size_legal_radiobutton;
	g_signal_connect (G_OBJECT (radio_button),
			  "toggled",
			  G_CALLBACK (catalog_legal_size_toggled_cb),
			  data);

	radio_button = data->paper_size_executive_radiobutton;
	g_signal_connect (G_OBJECT (radio_button),
			  "toggled",
			  G_CALLBACK (catalog_executive_size_toggled_cb),
			  data);

	radio_button = data->paper_size_a4_radiobutton;
	g_signal_connect (G_OBJECT (radio_button),
			  "toggled",
			  G_CALLBACK (catalog_a4_size_toggled_cb),
			  data);

	radio_button = data->paper_size_custom_radiobutton;
	g_signal_connect (G_OBJECT (radio_button),
			  "toggled",
			  G_CALLBACK (catalog_custom_size_toggled_cb),
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
