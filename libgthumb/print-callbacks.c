/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001 The Free Software Foundation, Inc.
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

#include <libgnome/libgnome.h>
#include <libgnomeprint/gnome-print.h>
#include <libgnomeprint/gnome-print-master.h>
#include <libgnomeprintui/gnome-print-preview.h>
#include <libgnomeprintui/gnome-print-master-preview.h>
#include <libgnomeprintui/gnome-print-dialog.h>
#include <libgnomeprintui/gnome-printer-dialog.h>
#include <libgnomeprintui/gnome-print-paper-selector.h>
#include <libgnomecanvas/libgnomecanvas.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <libgnomevfs/gnome-vfs-uri.h>
#include <libart_lgpl/libart.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glade/glade.h>
#include <pango/pango-layout.h>

#include "glib-utils.h"
#include "image-viewer.h"
#include "pixbuf-utils.h"
#include "comments.h"
#include "gthumb-canvas-text.h"

#define GLADE_PRINT_FILE "gthumb_print.glade"
#define PARAGRAPH_SEPARATOR 0x2029	
#define CANVAS_ZOOM 0.30 /*0.25*/

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
	GnomePrintMaster    *gpm;

	gboolean             print_comment;
	gboolean             portrait;

	/* Image Information. */

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


typedef struct {
	GladeXML      *gui;
	GtkWidget     *dialog;

	GtkWidget     *print_comment_checkbutton;
	GtkWidget     *btn_close;
	GtkWidget     *btn_center;
	GtkWidget     *btn_print;
	GtkWidget     *hscale1;

	GtkAdjustment *adj;

	PrintInfo     *pi;
} DialogData;


void 
print_info_unref (PrintInfo *pi)
{
	g_return_if_fail (pi != NULL);
	g_return_if_fail (pi->ref_count > 0);
	
	pi->ref_count--;

	if (pi->ref_count == 0) {
		if (pi->gpm != NULL)
			g_object_unref (pi->gpm);
		gnome_print_config_unref (pi->config);
		g_object_unref (pi->pixbuf);
		g_object_unref (pi->font_comment);
		g_free (pi->comment);
		g_free (pi);
	}
}


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
				double      line_width;

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


static void
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
				/* text do not fit. */
				/*return;*/
			}
		} else {
			y = print_paragraph (pc, pi, p, end, x, y);
			
			if ((y - pi->paper_bmargin) < fontheight) {
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

	gnome_print_beginpage (pc, "1");

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

	if (pi->print_comment) {
		gnome_print_gsave (pc);
		print_comment (pc, pi);
		gnome_print_grestore (pc);
	}

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


static gint
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
		pi->ci_comment = gnome_canvas_item_new (
					group,
					gthumb_canvas_text_get_type (),
					"text",          pi->comment,
					"x",             pi->paper_width / 2,
					"y",             pi->paper_height - bmargin,
					"layout_width",  layout_width,
					"wrap_mode",     PANGO_WRAP_CHAR,
					"font",          "Sans 12",
					"size_points",   CANVAS_ZOOM * 12,
					"anchor",        GTK_ANCHOR_S,
					"justification", GTK_JUSTIFY_LEFT,
					"clip_width",    pi->paper_width - lmargin - rmargin,
					"clip_height",   pi->paper_height - tmargin - bmargin,
					"clip",          TRUE,
					NULL);

		g_object_get (G_OBJECT (pi->ci_comment),
			      "text_height", &comment_height,
			      NULL);
	} else 
		comment_height = 0;

	/* Scale if image does not fit on the window. */

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


/* called when the print button is clicked. */
static void 
print_cb (GtkWidget *widget,
	  DialogData *data)
{
	PrintInfo         *pi = data->pi;
	GnomePrintContext *gp_ctx;
	gdouble            x, y;

	g_object_get (G_OBJECT (pi->ci_image),
		      "x", &x,
		      "y", &y,
		      NULL);
	pi->scale_x = pi->image_w * pi->zoom;
	pi->scale_y = pi->image_h * pi->zoom;
	pi->trans_x = x / pi->scale_x;
	pi->trans_y = (pi->paper_height - pi->scale_y - y) / pi->scale_y;

	/* pi is used to print, so we must add a reference. */

	pi->ref_count++;
	gtk_widget_destroy (data->dialog); 

	/* Print !! */

	pi->gpm = gnome_print_master_new_from_config (pi->config);
	gp_ctx = gnome_print_master_get_context (pi->gpm);
	print_image (gp_ctx, pi, FALSE);
	gnome_print_master_close (pi->gpm);
	gnome_print_master_print (pi->gpm);	
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


static void
page_update (DialogData *data)
{
	PrintInfo *pi = data->pi;
	const      GnomePrintUnit *unit;
	const      GnomePrintUnit *ps_unit;
	guchar    *orientation;
	double     width, height, lmargin, tmargin, rmargin, bmargin;
	double     paper_width, paper_height, paper_lmargin, paper_tmargin;
	double     paper_rmargin, paper_bmargin;
	gboolean   portrait;

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

	orientation = gnome_print_config_get (pi->config, GNOME_PRINT_KEY_PAGE_ORIENTATION);
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

	/* FIXME 
	if ((pi->portrait == portrait)
	    && (pi->paper_width == paper_width)
	    && (pi->paper_height == paper_height)
	    && (pi->paper_lmargin == paper_lmargin)
	    && (pi->paper_tmargin == paper_tmargin)
	    && (pi->paper_rmargin == paper_rmargin)
	    && (pi->paper_bmargin == paper_bmargin))
		return;
	*/

	pi->portrait = portrait;
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
switch_page_cb (GtkWidget       *widget,
		GtkNotebookPage *page,
		guint            page_num,
		gpointer         data)
{
	if (page_num != 0)
		return;
	page_update ((DialogData*) data);
}


static void
print_comment_cb (GtkWidget  *widget,
		  DialogData *data)
{
	data->pi->print_comment = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));
	page_update ((DialogData*) data);
}


void
print_image_dlg (GtkWindow   *parent, 
		 ImageViewer *viewer,
		 const char  *location)
{
	PrintInfo    *pi;
	DialogData   *data;
	GtkWidget    *paper_vbox, *printer_vbox;
	GtkWidget    *notebook;
	CommentData  *cdata = NULL;

	if (image_viewer_is_void (viewer))
		return;

	pi = g_new0 (PrintInfo, 1);
	pi->ref_count = 1;
	pi->zoom = 1.0;

	if (location != NULL) {
		GnomeVFSURI  *uri = gnome_vfs_uri_new (location);

		if (gnome_vfs_uri_is_local (uri)) {
			char *path;

			path = gnome_vfs_get_local_path_from_uri (location);
			cdata = comments_load_comment (path);
			g_free (path);
		}

		gnome_vfs_uri_unref (uri);
	}

	if (cdata != NULL) {
		char *comment;

		comment = comments_get_comment_as_string (cdata, "\n", " - ");
		pi->comment = g_locale_to_utf8 (comment, -1, 0, 0, 0);

		g_free (comment);
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

	pi->image_w = (gdouble) gdk_pixbuf_get_width (pi->pixbuf);
	pi->image_h = (gdouble) gdk_pixbuf_get_height (pi->pixbuf);
	pi->portrait = TRUE;
	pi->use_colors = TRUE;

	/* FIXME */
	pi->font_comment = gnome_font_find_closest_from_full_name ("Sans 12"); 

	data = g_new (DialogData, 1);
	data->pi = pi;
	data->gui = glade_xml_new (GTHUMB_GLADEDIR "/" GLADE_PRINT_FILE, NULL, NULL);
        if (! data->gui) {
                g_warning ("Cannot find " GLADE_PRINT_FILE "\n");
		print_info_unref (pi);
		g_free (data);
                return;
        }

	/* Get the widgets. */

	data->dialog = glade_xml_get_widget (data->gui, "page_setup_dialog");
	data->print_comment_checkbutton = glade_xml_get_widget (data->gui, "print_comment_checkbutton");
	data->btn_close = glade_xml_get_widget (data->gui, "btn_close");
	data->hscale1 = glade_xml_get_widget (data->gui, "hscale1");
	data->btn_center = glade_xml_get_widget (data->gui, "btn_center");
	data->btn_print = glade_xml_get_widget (data->gui, "btn_print");

	printer_vbox = glade_xml_get_widget (data->gui, "printer_vbox");
	paper_vbox   = glade_xml_get_widget (data->gui, "paper_vbox");
	notebook     = glade_xml_get_widget (data->gui, "notebook");

	gtk_box_pack_start (GTK_BOX (printer_vbox),
			    gnome_printer_selection_new (pi->config),
			    FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (paper_vbox),
			    gnome_paper_selector_new (pi->config),
			    FALSE, FALSE, 0);

	pi->canvas = glade_xml_get_widget (data->gui, "canvas");

	/* Set widgets data. */

	gtk_widget_set_sensitive (data->print_comment_checkbutton, pi->comment != NULL);

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
	g_signal_connect (G_OBJECT (notebook), 
			  "switch_page",
			  G_CALLBACK (switch_page_cb),
			  data);
	g_signal_connect (G_OBJECT (data->print_comment_checkbutton),
			  "toggled",
			  G_CALLBACK (print_comment_cb),
			  data);

	data->adj = gtk_range_get_adjustment (GTK_RANGE (data->hscale1));
	g_signal_connect (G_OBJECT (data->adj), 
			  "value_changed",
			  G_CALLBACK (zoom_changed),
			  data);

	/**/

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), parent);

	gnome_canvas_set_pixels_per_unit (GNOME_CANVAS (pi->canvas), 
					  CANVAS_ZOOM);
	page_update (data);

	gtk_widget_show_all (data->dialog);
}
