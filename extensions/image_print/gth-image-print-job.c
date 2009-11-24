/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2009 The Free Software Foundation, Inc.
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
#include <math.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <gthumb.h>
#include "gth-image-info.h"
#include "gth-image-print-job.h"
#include "gth-load-image-info-task.h"
#include "preferences.h"


#define GET_WIDGET(name) _gtk_builder_get_widget (self->priv->builder, (name))
#define PREVIEW_SCALE_FACTOR 3.0 /* FIXME: why 3.0 ? */


enum {
	IMAGES_PER_PAGE_1,
	IMAGES_PER_PAGE_2,
	IMAGES_PER_PAGE_4,
	IMAGES_PER_PAGE_8,
	IMAGES_PER_PAGE_16,
	IMAGES_PER_PAGE_32,
	N_IMAGES_PER_PAGE
};
static int n_rows_for_ipp[N_IMAGES_PER_PAGE] = { 1, 2, 2, 4, 4, 8 };
static int n_cols_for_ipp[N_IMAGES_PER_PAGE] = { 1, 1, 2, 2, 4, 4 };
static gpointer parent_class = NULL;


struct _GthImagePrintJobPrivate {
	GtkPrintOperationAction  action;
	GthBrowser         *browser;
	GtkPrintOperation  *print_operation;
	GtkBuilder         *builder;
	GtkWidget          *caption_chooser;
	GthImageInfo       *selected;

	gulong              rotation_combobox_changed_event;
	gulong              scale_adjustment_value_changed_event;
	gulong              left_adjustment_value_changed_event;
	gulong              top_adjustment_value_changed_event;
	gulong              width_adjustment_value_changed_event;
	gulong              height_adjustment_value_changed_event;
	gulong              position_combobox_changed_event;
	GthMetric           unit;

	/* settings */

	GthImageInfo      **images;
	int                 n_images;
	int                 requested_images_per_page;
	int                 image_width;
	int                 image_height;
	GtkPageSetup       *page_setup;
	char               *caption_attributes;

	/* layout info */

	GthTask            *task;
	int                 real_images_per_page;
	double              max_image_width;
	double		    max_image_height;
	double              x_padding;
	double              y_padding;
	int                 n_pages;
	int                 current_page;
};


static void
gth_image_print_job_finalize (GObject *base)
{
	GthImagePrintJob *self;
	int               i;

	self = GTH_IMAGE_PRINT_JOB (base);

	_g_object_unref (self->priv->task);
	_g_object_unref (self->priv->print_operation);
	_g_object_unref (self->priv->builder);
	for (i = 0; i < self->priv->n_images; i++)
		gth_image_info_unref (self->priv->images[i]);
	g_free (self->priv->images);
	_g_object_unref (self->priv->page_setup);
	g_free (self->priv->caption_attributes);

	G_OBJECT_CLASS (parent_class)->finalize (base);
}


static void
gth_image_print_job_class_init (GthImagePrintJobClass *klass)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GthImagePrintJobPrivate));

	object_class = (GObjectClass*) klass;
	object_class->finalize = gth_image_print_job_finalize;
}


static void
gth_image_print_job_init (GthImagePrintJob *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_IMAGE_PRINT_JOB, GthImagePrintJobPrivate);
	self->priv->builder = NULL;
	self->priv->task = NULL;
	self->priv->page_setup = NULL;
	self->priv->current_page = 0;
	self->priv->caption_attributes = eel_gconf_get_string (PREF_IMAGE_PRINT_CAPTION, "");
	self->priv->selected = NULL;
	self->priv->requested_images_per_page = 1;
	self->priv->unit = GTK_UNIT_PIXEL;
}


GType
gth_image_print_job_get_type (void)
{
        static GType type = 0;

        if (! type) {
                GTypeInfo type_info = {
			sizeof (GthImagePrintJobClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_image_print_job_class_init,
			NULL,
			NULL,
			sizeof (GthImagePrintJob),
			0,
			(GInstanceInitFunc) gth_image_print_job_init
		};

		type = g_type_register_static (G_TYPE_OBJECT,
					       "GthImagePrintJob",
					       &type_info,
					       0);
	}

        return type;
}


static double
_log2 (double x)
{
	return log (x) / log (2);
}


static int
get_combo_box_index_from_ipp (int value)
{
	return (int) floor (_log2 (value) + 0.5);
}


static int
get_ipp_from_combo_box_index (int idx)
{
	return (int) pow (2, idx);
}


static void
gth_image_print_job_update_layout_info (GthImagePrintJob   *self,
				        gdouble             page_width,
				        gdouble             page_height,
				        GtkPageOrientation  orientation)
{
	int idx;
	int rows;
	int cols;
	int current_page;
	int current_row;
	int current_col;
	int i;

	self->priv->real_images_per_page = self->priv->requested_images_per_page;
	self->priv->x_padding = page_width / 40.0;
	self->priv->y_padding = page_height / 40.0;

	idx = get_combo_box_index_from_ipp (self->priv->real_images_per_page);
	rows = n_rows_for_ipp[idx];
	cols = n_cols_for_ipp[idx];
	if ((orientation == GTK_PAGE_ORIENTATION_LANDSCAPE)
	    || (orientation == GTK_PAGE_ORIENTATION_REVERSE_LANDSCAPE))
	{
		int tmp = rows;
		rows = cols;
		cols = tmp;
	}

	self->priv->max_image_width = (page_width - ((cols - 1) * self->priv->x_padding)) / cols;
	self->priv->max_image_height = (page_height - ((rows - 1) * self->priv->y_padding)) / rows;

	self->priv->n_pages = MAX ((int) ceil ((double) self->priv->n_images / self->priv->real_images_per_page), 1);
	if (self->priv->current_page >= self->priv->n_pages)
		self->priv->current_page = self->priv->n_pages - 1;

	current_page = 0;
	current_row = 1;
	current_col = 1;
	for (i = 0; i < self->priv->n_images; i++) {
		GthImageInfo *image_info = self->priv->images[i];

		image_info->page = current_page;
		image_info->col = current_col;
		image_info->row = current_row;

		current_col++;
		if (current_col > cols) {
			current_row++;
			current_col = 1;
		}

		if (current_row > rows) {
			current_page++;
			current_col = 1;
			current_row = 1;
		}
	}
}


static void
gth_image_print_job_update_image_layout (GthImagePrintJob    *self,
					 GthImageInfo        *image_info,
					 PangoLayout         *pango_layout,
					 char               **attributes_v,
					 gdouble              page_width,
					 gdouble              page_height,
					 GtkPageOrientation   orientation,
					 double               scale_factor)
{
	double max_image_width;
	double max_image_height;
	double factor;

	if (self->priv->selected == NULL)
		self->priv->selected = image_info;

	image_info->boundary.x = (image_info->col - 1) * (self->priv->max_image_width + self->priv->x_padding);
	image_info->boundary.y = (image_info->row - 1) * (self->priv->max_image_height + self->priv->y_padding);
	image_info->boundary.width = self->priv->max_image_width;
	image_info->boundary.height = self->priv->max_image_height;

	max_image_width = image_info->boundary.width;
	max_image_height = image_info->boundary.height;

	image_info->print_comment = FALSE;
	g_free (image_info->comment_text);
	image_info->comment_text = NULL;

	image_info->comment.x = 0.0;
	image_info->comment.y = 0.0;
	image_info->comment.width = 0.0;
	image_info->comment.height = 0.0;

	if (strcmp (self->priv->caption_attributes, "") != 0) {
		gboolean  comment_present = FALSE;
		GString  *text;
		int       j;

		text = g_string_new ("");
		for (j = 0; attributes_v[j] != NULL; j++) {
			char *value;

			value = gth_file_data_get_attribute_as_string (image_info->file_data, attributes_v[j]);
			if ((value != NULL) && (strcmp (value, "") != 0)) {
				if (comment_present)
					g_string_append (text, "\n");
				g_string_append (text, value);
				comment_present = TRUE;
			}

			g_free (value);
		}

		image_info->comment_text = g_string_free (text, FALSE);
		if (comment_present) {
			PangoRectangle logical_rect;

			image_info->print_comment = TRUE;

			pango_layout_set_text (pango_layout, image_info->comment_text, -1);
			pango_layout_set_width (pango_layout, max_image_width * scale_factor * PANGO_SCALE);
			pango_layout_get_pixel_extents (pango_layout, NULL, &logical_rect);

			image_info->comment.x = 0;
			image_info->comment.y = 0;
			image_info->comment.width = image_info->boundary.width;
			image_info->comment.height = logical_rect.height / scale_factor;
			max_image_height -= image_info->comment.height;
			if (max_image_height < 0) {
				image_info->print_comment = FALSE;
				max_image_height = image_info->boundary.height;
			}
		}
	}

	factor = MIN (max_image_width / image_info->pixbuf_width, max_image_height / image_info->pixbuf_height);
	image_info->maximized.width = (double) image_info->pixbuf_width * factor;
	image_info->maximized.height = (double) image_info->pixbuf_height * factor;
	image_info->maximized.x = image_info->boundary.x + ((max_image_width - image_info->maximized.width) / 2);
	image_info->maximized.y = image_info->boundary.y + ((max_image_height - image_info->maximized.height) / 2);

	if (image_info->reset) {
		/* calculate the transformation to center the image */
		image_info->transformation.x = (image_info->maximized.x - image_info->boundary.x) / self->priv->max_image_width;
		image_info->transformation.y = (image_info->maximized.y - image_info->boundary.y) / self->priv->max_image_height;
		image_info->zoom = 1.0;
		image_info->reset = FALSE;
	}

	image_info->image.x = image_info->boundary.x + (self->priv->max_image_width * image_info->transformation.x);
	image_info->image.y = image_info->boundary.y + (self->priv->max_image_height * image_info->transformation.y);
	image_info->image.width = image_info->maximized.width * image_info->zoom;
	image_info->image.height = image_info->maximized.height * image_info->zoom;

	/* check the limits */

	if (image_info->image.x - image_info->boundary.x + image_info->image.width > image_info->boundary.width) {
		image_info->image.x = image_info->boundary.x + image_info->boundary.width - image_info->image.width;
		image_info->transformation.x = (image_info->image.x - image_info->boundary.x) / self->priv->max_image_width;
	}

	if (image_info->image.y - image_info->boundary.y + image_info->image.height > image_info->boundary.height) {
		image_info->image.y = image_info->boundary.y + image_info->boundary.height - image_info->image.height;
		image_info->transformation.y = (image_info->image.y - image_info->boundary.y) / self->priv->max_image_height;
	}

	/* the comment position */

	if (image_info->print_comment) {
		image_info->comment.x += image_info->boundary.x;
		image_info->comment.y += image_info->image.y + image_info->image.height;
	}
}


static PangoLayout *
gth_image_print_job_create_pango_layout (GthImagePrintJob *self)
{
	PangoLayout          *pango_layout;
	PangoFontDescription *font_desc;

	pango_layout = gtk_widget_create_pango_layout (GTK_WIDGET (self->priv->browser), NULL);
	pango_layout_set_wrap (pango_layout, PANGO_WRAP_WORD_CHAR);
	pango_layout_set_alignment (pango_layout, PANGO_ALIGN_CENTER);
	font_desc = pango_font_description_from_string ("[sans serif] [normal] [10]"); /* FIXME: allow the user to select a font */
	pango_layout_set_font_description (pango_layout, font_desc);

	pango_font_description_free (font_desc);

	return pango_layout;
}


static void
gth_image_print_job_update_page_layout (GthImagePrintJob   *self,
					int                 page,
					gdouble             page_width,
					gdouble             page_height,
					GtkPageOrientation  orientation,
					double              scale_factor)
{
	PangoLayout  *pango_layout;
	char        **attributes_v;
	int           i;

	pango_layout = gth_image_print_job_create_pango_layout (self);
	attributes_v = g_strsplit (self->priv->caption_attributes, ",", -1);
	for (i = 0; i < self->priv->n_images; i++) {
		GthImageInfo *image_info = self->priv->images[i];

		if (image_info->page != page)
			continue;

		gth_image_print_job_update_image_layout (self,
							 image_info,
							 pango_layout,
							 attributes_v,
							 page_width,
							 page_height,
							 orientation,
							 scale_factor);
	}

	g_strfreev (attributes_v);
	g_object_unref (pango_layout);
}


static void
gth_image_print_job_update_layout (GthImagePrintJob   *self,
			  	   gdouble             page_width,
			  	   gdouble             page_height,
			  	   GtkPageOrientation  orientation)
{
	gth_image_print_job_update_layout_info (self, page_width, page_height, orientation);
	gth_image_print_job_update_page_layout (self, self->priv->current_page, page_width, page_height, orientation, PREVIEW_SCALE_FACTOR);
}


static void
gth_image_print_job_paint (GthImagePrintJob *self,
			   cairo_t          *cr,
			   PangoLayout      *pango_layout,
			   double            x_offset,
			   double            y_offset,
			   int               page,
			   gboolean          preview)
{
	PangoFontDescription *font_desc;
	int                   i;

	font_desc = pango_font_description_from_string ("[sans serif] [normal] [10]");
	pango_layout_set_font_description (pango_layout, font_desc);

	for (i = 0; i < self->priv->n_images; i++) {
		GthImageInfo *image_info = self->priv->images[i];
		GdkPixbuf    *fullsize_pixbuf;

		if (image_info->page != page)
			continue;

		if (preview) {
			cairo_save (cr);

			cairo_set_line_width (cr, 0.5);
			if (image_info->active)
				cairo_set_source_rgb (cr, 1.0, 0.0, 0.0);
			else if (image_info == self->priv->selected)
				cairo_set_source_rgb (cr, 0.5, 0.5, 0.5);
			else
				cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
			cairo_rectangle (cr,
					 x_offset + image_info->boundary.x,
					 y_offset + image_info->boundary.y,
					 image_info->boundary.width,
					 image_info->boundary.height);
			cairo_stroke (cr);

			cairo_restore (cr);
		}

#if 0
		cairo_save (cr);

		cairo_set_source_rgb (cr, 1.0, .0, .0);
		cairo_rectangle (cr,
				 x_offset + image_info->boundary.x,
				 y_offset + image_info->boundary.y,
				 image_info->boundary.width,
				 image_info->boundary.height);
		cairo_stroke (cr);

		cairo_set_source_rgb (cr, .0, .0, .0);
		cairo_rectangle (cr,
				 x_offset + image_info->image.x,
				 y_offset + image_info->image.y,
				 image_info->image.width,
				 image_info->image.height);
		cairo_stroke (cr);

		cairo_set_source_rgb (cr, .0, .0, 1.0);
		cairo_rectangle (cr,
				 x_offset + image_info->comment.x,
				 y_offset + image_info->comment.y,
				 image_info->comment.width,
				 image_info->comment.height);
		cairo_stroke (cr);

		cairo_restore (cr);

#endif

		if (! preview) {
			if (image_info->rotation != GTH_TRANSFORM_NONE)
				fullsize_pixbuf = _gdk_pixbuf_transform (image_info->pixbuf, image_info->rotation);
			else
				fullsize_pixbuf = g_object_ref (image_info->pixbuf);
		}
		else if (image_info->active)
			fullsize_pixbuf = g_object_ref (image_info->thumbnail_active);
		else
			fullsize_pixbuf = g_object_ref (image_info->thumbnail);

		if ((image_info->image.width >= 1.0) && (image_info->image.height >= 1.0)) {
			GdkPixbuf *pixbuf;

			pixbuf = gdk_pixbuf_scale_simple (fullsize_pixbuf,
							  image_info->image.width,
							  image_info->image.height,
							  preview ? GDK_INTERP_NEAREST : GDK_INTERP_BILINEAR);

			cairo_save (cr);
			gdk_cairo_set_source_pixbuf (cr,
						     pixbuf,
						     x_offset + image_info->image.x,
						     y_offset + image_info->image.y);
			cairo_rectangle (cr,
					 x_offset + image_info->image.x,
					 y_offset + image_info->image.y,
					 gdk_pixbuf_get_width (pixbuf),
					 gdk_pixbuf_get_height (pixbuf));
			cairo_clip (cr);
			cairo_paint (cr);
			cairo_restore (cr);

			g_object_unref (pixbuf);
		}

		if (image_info->print_comment) {
			cairo_save (cr);

			pango_layout_set_wrap (pango_layout, PANGO_WRAP_WORD_CHAR);
			pango_layout_set_alignment (pango_layout, PANGO_ALIGN_CENTER);
			pango_layout_set_width (pango_layout, image_info->comment.width * (preview ? PREVIEW_SCALE_FACTOR : 1.0) * PANGO_SCALE);
			pango_layout_set_text (pango_layout, image_info->comment_text, -1);

			cairo_move_to (cr, x_offset + image_info->comment.x, y_offset + image_info->comment.y);
			if (preview) {
				cairo_font_options_t *options;

				options = cairo_font_options_create ();
				cairo_font_options_set_hint_style (options, CAIRO_HINT_STYLE_NONE);
				cairo_font_options_set_hint_metrics (options, CAIRO_HINT_METRICS_OFF);
				cairo_font_options_set_antialias (options, CAIRO_ANTIALIAS_NONE);
				cairo_set_font_options (cr, options);
				cairo_font_options_destroy (options);

				cairo_scale (cr, 1.0 / PREVIEW_SCALE_FACTOR, 1.0 / PREVIEW_SCALE_FACTOR);
				cairo_set_line_width (cr, 0.5);
			}

			pango_cairo_layout_path (cr, pango_layout);
			cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
			/*cairo_set_line_width (cr, 0.5);
			cairo_stroke_preserve (cr);*/
			cairo_fill (cr);

			cairo_restore (cr);
		}

		g_object_unref (fullsize_pixbuf);
	}

	pango_font_description_free (font_desc);

#if 0
	cairo_t        *cr;
	PangoLayout    *layout;
	int             y;
	PangoRectangle  rect;


	cr = gtk_print_context_get_cairo_context (context);
	cairo_set_source_rgb (cr, 0, 0, 0);
	cairo_rectangle (cr,
			 0, 0,
			 gtk_print_context_get_width (context),
			 gtk_print_context_get_height (context));
	cairo_stroke (cr);

	layout = gtk_print_context_create_pango_layout (context);
	pango_layout_set_wrap (layout, PANGO_WRAP_WORD_CHAR);
	pango_layout_set_width (layout, 100 * PANGO_SCALE);

	/**/

	y = 20;

	pango_layout_set_text (layout, "Hello World! Printing is easy", -1);
	cairo_move_to (cr, 30, y);
	pango_cairo_layout_path (cr, layout);

	cairo_set_source_rgb (cr, 0.93, 1.0, 0.47);
	cairo_set_line_width (cr, 0.5);
	cairo_stroke_preserve (cr);

	cairo_set_source_rgb (cr, 0, 0.0, 1.0);
	cairo_fill (cr);

	pango_layout_get_extents (layout, NULL, &rect);
	y = y + (rect.height / PANGO_SCALE);

	/**/

	pango_layout_set_text (layout, "Hello World! Printing is easy 222", -1);
	cairo_move_to (cr, 30, y);
	pango_cairo_layout_path (cr, layout);

	cairo_set_source_rgb (cr, 0.93, 1.0, 0.47);
	cairo_set_line_width (cr, 0.5);
	cairo_stroke_preserve (cr);

	cairo_set_source_rgb (cr, 0, 0.0, 1.0);
	cairo_fill (cr);

	g_object_unref (layout);
#endif
}


static int
get_combo_box_index_from_rotation (GthTransform rotation)
{
	int idx = 0;

	switch (rotation) {
	case GTH_TRANSFORM_NONE:
		idx = 0;
		break;
	case GTH_TRANSFORM_ROTATE_90:
		idx = 1;
		break;
	case GTH_TRANSFORM_ROTATE_180:
		idx = 2;
		break;
	case GTH_TRANSFORM_ROTATE_270:
		idx = 3;
		break;
	default:
		break;
	}

	return idx;
}


static double
from_unit_to_pixels (GthMetric unit,
		     double    value)
{
	switch (unit) {
	case GTH_METRIC_INCHES:
		value = value * 2.54;
		break;
	case GTH_METRIC_MILLIMETERS:
		value = value;
		break;
	case GTH_METRIC_PIXELS:
		value = value;
		break;
	}

	return value;
}


static double
from_pixels_to_unit (GthMetric unit,
		     double    value)
{
	switch (unit) {
	case GTH_METRIC_INCHES:
		value = value / 2.54;
		break;
	case GTH_METRIC_MILLIMETERS:
		value = value;
		break;
	case GTH_METRIC_PIXELS:
		value = value;
		break;
	}

	return value;
}


#define TO_UNIT(x) (from_pixels_to_unit (self->priv->unit, (x)))
#define TO_PIXELS(x) (from_unit_to_pixels (self->priv->unit, (x)))


static void
gth_image_print_job_update_image_controls (GthImagePrintJob *self)
{
	gboolean centered;

	if (self->priv->selected == NULL)
		return;

	g_signal_handler_block (GET_WIDGET ("rotation_combobox"), self->priv->rotation_combobox_changed_event);
	gtk_combo_box_set_active (GTK_COMBO_BOX (GET_WIDGET ("rotation_combobox")), get_combo_box_index_from_rotation (self->priv->selected->rotation));
	g_signal_handler_unblock (GET_WIDGET ("rotation_combobox"), self->priv->rotation_combobox_changed_event);

	g_signal_handler_block (GET_WIDGET ("scale_adjustment"), self->priv->scale_adjustment_value_changed_event);
	gtk_adjustment_set_value (GTK_ADJUSTMENT (GET_WIDGET ("scale_adjustment")), self->priv->selected->zoom);
	g_signal_handler_unblock (GET_WIDGET ("scale_adjustment"), self->priv->scale_adjustment_value_changed_event);

	g_signal_handler_block (GET_WIDGET ("left_adjustment"), self->priv->left_adjustment_value_changed_event);
	gtk_adjustment_set_lower (GTK_ADJUSTMENT (GET_WIDGET ("left_adjustment")), 0.0);
	gtk_adjustment_set_upper (GTK_ADJUSTMENT (GET_WIDGET ("left_adjustment")), TO_UNIT (self->priv->selected->boundary.width - self->priv->selected->image.width));
	gtk_adjustment_set_value (GTK_ADJUSTMENT (GET_WIDGET ("left_adjustment")), TO_UNIT (self->priv->selected->image.x - self->priv->selected->boundary.x));
	g_signal_handler_unblock (GET_WIDGET ("left_adjustment"), self->priv->left_adjustment_value_changed_event);

	g_signal_handler_block (GET_WIDGET ("top_adjustment"), self->priv->top_adjustment_value_changed_event);
	gtk_adjustment_set_lower (GTK_ADJUSTMENT (GET_WIDGET ("top_adjustment")), 0.0);
	gtk_adjustment_set_upper (GTK_ADJUSTMENT (GET_WIDGET ("top_adjustment")), TO_UNIT (self->priv->selected->boundary.height - self->priv->selected->comment.height - self->priv->selected->image.height));
	gtk_adjustment_set_value (GTK_ADJUSTMENT (GET_WIDGET ("top_adjustment")), TO_UNIT (self->priv->selected->image.y - self->priv->selected->boundary.y));
	g_signal_handler_unblock (GET_WIDGET ("top_adjustment"), self->priv->top_adjustment_value_changed_event);

	g_signal_handler_block (GET_WIDGET ("width_adjustment"), self->priv->width_adjustment_value_changed_event);
	gtk_adjustment_set_lower (GTK_ADJUSTMENT (GET_WIDGET ("width_adjustment")), 0.0);
	gtk_adjustment_set_upper (GTK_ADJUSTMENT (GET_WIDGET ("width_adjustment")), TO_UNIT (self->priv->selected->maximized.width));
	gtk_adjustment_set_value (GTK_ADJUSTMENT (GET_WIDGET ("width_adjustment")), TO_UNIT (self->priv->selected->image.width));
	g_signal_handler_unblock (GET_WIDGET ("width_adjustment"), self->priv->width_adjustment_value_changed_event);

	g_signal_handler_block (GET_WIDGET ("height_adjustment"), self->priv->height_adjustment_value_changed_event);
	gtk_adjustment_set_lower (GTK_ADJUSTMENT (GET_WIDGET ("height_adjustment")), 0.0);
	gtk_adjustment_set_upper (GTK_ADJUSTMENT (GET_WIDGET ("height_adjustment")), TO_UNIT (self->priv->selected->maximized.height));
	gtk_adjustment_set_value (GTK_ADJUSTMENT (GET_WIDGET ("height_adjustment")), TO_UNIT (self->priv->selected->image.height));
	g_signal_handler_unblock (GET_WIDGET ("height_adjustment"), self->priv->height_adjustment_value_changed_event);

	g_signal_handler_block (GET_WIDGET ("position_combobox"), self->priv->position_combobox_changed_event);
	centered = (self->priv->selected->image.x == ((self->priv->selected->boundary.width - self->priv->selected->image.width) / 2.0))
		    && (self->priv->selected->image.y == ((self->priv->selected->boundary.height - self->priv->selected->comment.height - self->priv->selected->image.height) / 2.0));
	gtk_combo_box_set_active (GTK_COMBO_BOX (GET_WIDGET ("position_combobox")), centered ? 0 : 1);
	g_signal_handler_unblock (GET_WIDGET ("position_combobox"), self->priv->position_combobox_changed_event);
}


static void
gth_image_print_job_update_preview (GthImagePrintJob *self)
{
	char *text;

	gth_image_print_job_update_layout (self,
					   gtk_page_setup_get_page_width (self->priv->page_setup, GTK_UNIT_MM),
					   gtk_page_setup_get_page_height (self->priv->page_setup, GTK_UNIT_MM),
					   gtk_page_setup_get_orientation (self->priv->page_setup));
	gtk_widget_queue_draw (GET_WIDGET ("preview_drawingarea"));

	gth_image_print_job_update_image_controls (self);

	text = g_strdup_printf (_("Page %d of %d"), self->priv->current_page + 1, self->priv->n_pages);
	gtk_label_set_text (GTK_LABEL (GET_WIDGET ("page_label")), text);
	gtk_widget_set_sensitive (GET_WIDGET ("next_page_button"), self->priv->current_page < self->priv->n_pages - 1);
	gtk_widget_set_sensitive (GET_WIDGET ("prev_page_button"), self->priv->current_page > 0);

	g_free (text);
}


static void
gth_image_print_job_update_image_preview (GthImagePrintJob *self,
					  GthImageInfo     *image_info)
{
	PangoLayout  *pango_layout;
	char        **attributes_v;

	pango_layout = gth_image_print_job_create_pango_layout (self);
	attributes_v = g_strsplit (self->priv->caption_attributes, ",", -1);
	gth_image_print_job_update_image_layout (self,
						 image_info,
						 pango_layout,
						 attributes_v,
						 gtk_page_setup_get_page_width (self->priv->page_setup, GTK_UNIT_MM),
						 gtk_page_setup_get_page_height (self->priv->page_setup, GTK_UNIT_MM),
						 gtk_page_setup_get_orientation (self->priv->page_setup),
						 PREVIEW_SCALE_FACTOR);
	gtk_widget_queue_draw (GET_WIDGET ("preview_drawingarea"));
	gth_image_print_job_update_image_controls (self);

	g_strfreev (attributes_v);
	g_object_unref (pango_layout);
}


static void
preview_expose_event_cb (GtkWidget      *widget,
			 GdkEventExpose *event,
			 gpointer        user_data)
{
	GthImagePrintJob *self = user_data;
	cairo_t          *cr;
	PangoLayout      *pango_layout;

	cr = gdk_cairo_create (widget->window);

	/* paint the paper */

	cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
	cairo_rectangle (cr, 0, 0, widget->allocation.width - 1, widget->allocation.height - 1);
	cairo_fill_preserve (cr);
	cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
	cairo_stroke (cr);

	/* paint the current page */

	pango_layout = gtk_widget_create_pango_layout (GTK_WIDGET (self->priv->browser), NULL);
	gth_image_print_job_paint (self,
			           cr,
			           pango_layout,
				   gtk_page_setup_get_left_margin (self->priv->page_setup, GTK_UNIT_MM),
				   gtk_page_setup_get_top_margin (self->priv->page_setup, GTK_UNIT_MM),
				   self->priv->current_page,
				   TRUE);

	g_object_unref (pango_layout);
	cairo_destroy (cr);
}


static gboolean
preview_motion_notify_event_cb (GtkWidget      *widget,
				GdkEventMotion *event,
				gpointer        user_data)
{
	GthImagePrintJob *self = user_data;
	double            x, y;
	int               i;
	gboolean          changed = FALSE;

	x = event->x - gtk_page_setup_get_left_margin (self->priv->page_setup, GTK_UNIT_MM);
	y = event->y - gtk_page_setup_get_top_margin (self->priv->page_setup, GTK_UNIT_MM);
	for (i = 0; i < self->priv->n_images; i++) {
		GthImageInfo *image_info = self->priv->images[i];

		if (image_info->page != self->priv->current_page)
			continue;

		if ((x >= image_info->boundary.x)
		    && (x <= image_info->boundary.x + image_info->boundary.width)
		    && (y >= image_info->boundary.y )
		    && (y <= image_info->boundary.y + image_info->boundary.height))
		{
			if (! image_info->active) {
				image_info->active = TRUE;
				changed = TRUE;
			}
		}
		else if (image_info->active) {
			image_info->active = FALSE;
			changed = TRUE;
		}
	}

	if (changed)
		gtk_widget_queue_draw (GET_WIDGET ("preview_drawingarea"));

	return FALSE;
}


static gboolean
preview_leave_notify_event_cb (GtkWidget        *widget,
			       GdkEventCrossing *event,
			       gpointer          user_data)
{
	GthImagePrintJob *self = user_data;
	int               i;
	gboolean          changed = FALSE;

	for (i = 0; i < self->priv->n_images; i++) {
		GthImageInfo *image_info = self->priv->images[i];

		if (image_info->page != self->priv->current_page)
			continue;

		if (image_info->active) {
			image_info->active = FALSE;
			changed = TRUE;
		}
	}

	if (changed)
		gtk_widget_queue_draw (GET_WIDGET ("preview_drawingarea"));

	return FALSE;
}


static gboolean
preview_button_press_event_cb (GtkWidget      *widget,
			       GdkEventButton *event,
			       gpointer        user_data)
{
	GthImagePrintJob *self = user_data;
	double            x, y;
	int               i;

	x = event->x - gtk_page_setup_get_left_margin (self->priv->page_setup, GTK_UNIT_MM);
	y = event->y - gtk_page_setup_get_top_margin (self->priv->page_setup, GTK_UNIT_MM);
	for (i = 0; i < self->priv->n_images; i++) {
		GthImageInfo *image_info = self->priv->images[i];

		if (image_info->page != self->priv->current_page)
			continue;

		if ((x >= image_info->boundary.x)
		    && (x <= image_info->boundary.x + image_info->boundary.width)
		    && (y >= image_info->boundary.y )
		    && (y <= image_info->boundary.y + image_info->boundary.height))
		{
			self->priv->selected = image_info;
			gtk_widget_queue_draw (GET_WIDGET ("preview_drawingarea"));
			gth_image_print_job_update_image_controls (self);
			break;
		}
	}

	return FALSE;
}


static void
ipp_combobox_changed_cb (GtkComboBox *widget,
			 gpointer     user_data)
{
	GthImagePrintJob *self = user_data;
	int               i;

	self->priv->requested_images_per_page = get_ipp_from_combo_box_index (gtk_combo_box_get_active (widget));
	for (i = 0; i < self->priv->n_images; i++)
		gth_image_info_reset (self->priv->images[i]);
	gth_image_print_job_update_preview (self);
}


static void
next_page_button_clicked_cb (GtkWidget *widget,
			     gpointer   user_data)
{
	GthImagePrintJob *self = user_data;

	self->priv->current_page = MIN (self->priv->current_page + 1, self->priv->n_pages - 1);
	self->priv->selected = NULL;
	gth_image_print_job_update_preview (self);
}


static void
prev_page_button_clicked_cb (GtkWidget *widget,
			     gpointer   user_data)
{
	GthImagePrintJob *self = user_data;

	self->priv->current_page = MAX (0, self->priv->current_page - 1);
	self->priv->selected = NULL;
	gth_image_print_job_update_preview (self);
}


static void
metadata_ready_cb (GList    *files,
		   GError   *error,
		   gpointer  user_data)
{
	GthImagePrintJob *self = user_data;

	gth_image_print_job_update_preview (self);
}


static void
gth_image_print_job_load_metadata (GthImagePrintJob *self)
{
	GList *files;
	int    i;

	files = NULL;
	for (i = 0; i < self->priv->n_images; i++)
		files = g_list_prepend (files, self->priv->images[i]->file_data);
	files = g_list_reverse (files);

	_g_query_metadata_async (files,
				 self->priv->caption_attributes,
				 NULL,
				 metadata_ready_cb,
				 self);

	g_list_free (files);
}


static void
caption_chooser_changed_cb (GthMetadataChooser *chooser,
			    gpointer            user_data)
{
	GthImagePrintJob *self = user_data;
	char             *new_caption_attributes;
	gboolean          reload_required;

	new_caption_attributes = gth_metadata_chooser_get_selection (chooser);
	reload_required = attribute_list_reaload_required (self->priv->caption_attributes, new_caption_attributes);
	g_free (self->priv->caption_attributes);
	self->priv->caption_attributes = new_caption_attributes;
	eel_gconf_set_string (PREF_IMAGE_PRINT_CAPTION, self->priv->caption_attributes);

	if (reload_required)
		gth_image_print_job_load_metadata (self);
	else
		gth_image_print_job_update_preview (self);
}


static void
unit_combobox_changed_cb (GtkComboBox *combo_box,
			  gpointer     user_data)
{
	GthImagePrintJob *self = user_data;
	int               digits;

	self->priv->unit = gtk_combo_box_get_active (combo_box);
	switch (self->priv->unit) {
	case GTH_METRIC_INCHES:
		digits = 1;
		break;
	case GTH_METRIC_MILLIMETERS:
		digits = 0;
		break;
	case GTH_METRIC_PIXELS:
		digits = 0;
		break;
	}

	gtk_spin_button_set_digits (GTK_SPIN_BUTTON (GET_WIDGET ("img_left_spinbutton")), digits);
	gtk_spin_button_set_digits (GTK_SPIN_BUTTON (GET_WIDGET ("img_top_spinbutton")), digits);
	gtk_spin_button_set_digits (GTK_SPIN_BUTTON (GET_WIDGET ("img_width_spinbutton")), digits);
	gtk_spin_button_set_digits (GTK_SPIN_BUTTON (GET_WIDGET ("img_height_spinbutton")), digits);

	gth_image_print_job_update_image_controls (self);
}


static void
rotation_combobox_changed_cb (GtkComboBox *combo_box,
			      gpointer     user_data)
{
	GthImagePrintJob *self = user_data;

	if (self->priv->selected == NULL)
		return;

	gth_image_info_rotate (self->priv->selected, gtk_combo_box_get_active (combo_box) * 90);
	gth_image_info_reset (self->priv->selected);
	gth_image_print_job_update_preview (self);
}


static void
gth_image_print_job_set_selected_zoom (GthImagePrintJob *self,
				       double            zoom)
{
	double x, y;

	self->priv->selected->zoom = CLAMP (zoom, 0.0, 1.0);
	self->priv->selected->image.width = self->priv->selected->maximized.width * self->priv->selected->zoom;
	self->priv->selected->image.height = self->priv->selected->maximized.height * self->priv->selected->zoom;

	x = self->priv->selected->image.x - self->priv->selected->boundary.x;
	y = self->priv->selected->image.y - self->priv->selected->boundary.y;
	if (x + self->priv->selected->image.width > self->priv->selected->boundary.width)
		x = self->priv->selected->boundary.width - self->priv->selected->image.width;
	if (x + self->priv->selected->image.width > self->priv->selected->boundary.width)
		self->priv->selected->image.width = self->priv->selected->boundary.width - x;

	if (y + self->priv->selected->image.height > self->priv->selected->boundary.height - self->priv->selected->comment.height)
		y = self->priv->selected->boundary.height - self->priv->selected->comment.height - self->priv->selected->image.height;
	if (y + self->priv->selected->image.height > self->priv->selected->boundary.height - self->priv->selected->comment.height)
		self->priv->selected->image.height = self->priv->selected->boundary.height - self->priv->selected->comment.height - y;

	self->priv->selected->zoom = MIN (self->priv->selected->image.width / self->priv->selected->maximized.width, self->priv->selected->image.height / self->priv->selected->maximized.height);
	self->priv->selected->transformation.x = x / self->priv->max_image_width;
	self->priv->selected->transformation.y = y / self->priv->max_image_height;

	gth_image_print_job_update_image_preview (self, self->priv->selected);
}


static void
scale_adjustment_value_changed_cb (GtkAdjustment *adjustment,
				   gpointer       user_data)
{
	GthImagePrintJob *self = user_data;

	if (self->priv->selected == NULL)
		return;

	gth_image_print_job_set_selected_zoom (self, gtk_adjustment_get_value (adjustment));
}


static void
left_adjustment_value_changed_cb (GtkAdjustment *adjustment,
				  gpointer       user_data)
{
	GthImagePrintJob *self = user_data;

	if (self->priv->selected == NULL)
		return;

	self->priv->selected->transformation.x = TO_PIXELS (gtk_adjustment_get_value (adjustment)) / self->priv->max_image_width;
	gth_image_print_job_update_preview (self);
}


static void
top_adjustment_value_changed_cb (GtkAdjustment *adjustment,
				 gpointer       user_data)
{
	GthImagePrintJob *self = user_data;

	if (self->priv->selected == NULL)
		return;

	self->priv->selected->transformation.y = TO_PIXELS (gtk_adjustment_get_value (adjustment)) / self->priv->max_image_height;
	gth_image_print_job_update_preview (self);
}


static void
width_adjustment_value_changed_cb (GtkAdjustment *adjustment,
				   gpointer       user_data)
{
	GthImagePrintJob *self = user_data;

	if (self->priv->selected == NULL)
		return;

	gth_image_print_job_set_selected_zoom (self, TO_PIXELS (gtk_adjustment_get_value (adjustment)) / self->priv->selected->maximized.width);
}


static void
height_adjustment_value_changed_cb (GtkAdjustment *adjustment,
				    gpointer       user_data)
{
	GthImagePrintJob *self = user_data;

	if (self->priv->selected == NULL)
		return;

	gth_image_print_job_set_selected_zoom (self, TO_PIXELS (gtk_adjustment_get_value (adjustment)) / self->priv->selected->maximized.height);
}


static void
position_combobox_changed_cb (GtkComboBox *combo_box,
			      gpointer     user_data)
{
	GthImagePrintJob *self = user_data;

	if (self->priv->selected == NULL)
		return;

	if (gtk_combo_box_get_active (combo_box) == 0) {
		self->priv->selected->image.x = (self->priv->selected->boundary.width - self->priv->selected->image.width) / 2.0;
		self->priv->selected->image.y = (self->priv->selected->boundary.height - self->priv->selected->comment.height - self->priv->selected->image.height) / 2.0;
		self->priv->selected->transformation.x = self->priv->selected->image.x / self->priv->max_image_width;
		self->priv->selected->transformation.y = self->priv->selected->image.y / self->priv->max_image_height;
		gth_image_print_job_update_preview (self);
	}
}


static char *
image_scale_format_value_cb (GtkScale *scale,
			     double    value,
			     gpointer  user_data)
{
	return g_strdup_printf ("%0.0f%%", value * 100.0);
}


static GObject *
operation_create_custom_widget_cb (GtkPrintOperation *operation,
			           gpointer           user_data)
{
	GthImagePrintJob *self = user_data;

	self->priv->builder = _gtk_builder_new_from_file ("print-layout.ui", "image_print");
	self->priv->caption_chooser = gth_metadata_chooser_new (GTH_METADATA_ALLOW_IN_PRINT);
	gtk_widget_show (self->priv->caption_chooser);
	gtk_container_add (GTK_CONTAINER (GET_WIDGET ("caption_scrolledwindow")), self->priv->caption_chooser);

	gth_metadata_chooser_set_selection (GTH_METADATA_CHOOSER (self->priv->caption_chooser), self->priv->caption_attributes);
	gtk_combo_box_set_active (GTK_COMBO_BOX (GET_WIDGET ("unit_combobox")), self->priv->unit);

	g_signal_connect (GET_WIDGET ("preview_drawingarea"),
			  "expose_event",
	                  G_CALLBACK (preview_expose_event_cb),
	                  self);
	g_signal_connect (GET_WIDGET ("preview_drawingarea"),
			  "motion-notify-event",
			  G_CALLBACK (preview_motion_notify_event_cb),
			  self);
	g_signal_connect (GET_WIDGET ("preview_drawingarea"),
			  "leave-notify-event",
			  G_CALLBACK (preview_leave_notify_event_cb),
			  self);
	g_signal_connect (GET_WIDGET ("preview_drawingarea"),
			  "button-press-event",
			  G_CALLBACK (preview_button_press_event_cb),
			  self);
	g_signal_connect (GET_WIDGET ("ipp_combobox"),
			  "changed",
	                  G_CALLBACK (ipp_combobox_changed_cb),
	                  self);
	g_signal_connect (GET_WIDGET ("next_page_button"),
			  "clicked",
			  G_CALLBACK (next_page_button_clicked_cb),
			  self);
	g_signal_connect (GET_WIDGET ("prev_page_button"),
			  "clicked",
			  G_CALLBACK (prev_page_button_clicked_cb),
			  self);
	g_signal_connect (self->priv->caption_chooser,
			  "changed",
	                  G_CALLBACK (caption_chooser_changed_cb),
	                  self);
	g_signal_connect (GET_WIDGET ("unit_combobox"),
			  "changed",
	                  G_CALLBACK (unit_combobox_changed_cb),
	                  self);

	self->priv->rotation_combobox_changed_event =
			g_signal_connect (GET_WIDGET ("rotation_combobox"),
					  "changed",
			                  G_CALLBACK (rotation_combobox_changed_cb),
			                  self);
	self->priv->scale_adjustment_value_changed_event =
			g_signal_connect (GET_WIDGET ("scale_adjustment"),
					  "value-changed",
					  G_CALLBACK (scale_adjustment_value_changed_cb),
					  self);
	g_signal_connect (GET_WIDGET ("image_scale"),
			  "format-value",
			  G_CALLBACK (image_scale_format_value_cb),
			  self);
	self->priv->left_adjustment_value_changed_event =
			g_signal_connect (GET_WIDGET ("left_adjustment"),
					  "value-changed",
					  G_CALLBACK (left_adjustment_value_changed_cb),
					  self);
	self->priv->top_adjustment_value_changed_event =
			g_signal_connect (GET_WIDGET ("top_adjustment"),
					  "value-changed",
					  G_CALLBACK (top_adjustment_value_changed_cb),
					  self);
	self->priv->width_adjustment_value_changed_event =
			g_signal_connect (GET_WIDGET ("width_adjustment"),
					  "value-changed",
					  G_CALLBACK (width_adjustment_value_changed_cb),
					  self);
	self->priv->height_adjustment_value_changed_event =
			g_signal_connect (GET_WIDGET ("height_adjustment"),
					  "value-changed",
					  G_CALLBACK (height_adjustment_value_changed_cb),
					  self);
	self->priv->position_combobox_changed_event =
			g_signal_connect (GET_WIDGET ("position_combobox"),
					  "changed",
					  G_CALLBACK (position_combobox_changed_cb),
					  self);

	return gtk_builder_get_object (self->priv->builder, "print_layout");
}


static void
operation_update_custom_widget_cb (GtkPrintOperation *operation,
				   GtkWidget         *widget,
				   GtkPageSetup      *setup,
				   GtkPrintSettings  *settings,
				   gpointer           user_data)
{
	GthImagePrintJob *self = user_data;
	int               i;

	_g_object_unref (self->priv->page_setup);
	self->priv->page_setup = gtk_page_setup_copy (setup);

	gtk_widget_set_size_request (GET_WIDGET ("preview_drawingarea"),
				     gtk_page_setup_get_paper_width (setup, GTK_UNIT_MM),
				     gtk_page_setup_get_paper_height (setup, GTK_UNIT_MM));

	for (i = 0; i < self->priv->n_images; i++)
		gth_image_info_reset (self->priv->images[i]);
	gth_image_print_job_update_preview (self);
}


static void
print_operation_begin_print_cb (GtkPrintOperation *operation,
				GtkPrintContext   *context,
				gpointer           user_data)
{
	GthImagePrintJob *self = user_data;
	GtkPageSetup     *setup;

	setup = gtk_print_context_get_page_setup (context);
	gth_image_print_job_update_layout_info (self,
						gtk_print_context_get_width (context),
						gtk_print_context_get_height (context),
						gtk_page_setup_get_orientation (setup));
	gtk_print_operation_set_n_pages (operation, self->priv->n_pages);
}


static void
print_operation_draw_page_cb (GtkPrintOperation *operation,
			      GtkPrintContext   *context,
			      int                page_nr,
			      gpointer           user_data)
{
	GthImagePrintJob *self = user_data;
	GtkPageSetup     *setup;
	cairo_t          *cr;
	PangoLayout      *pango_layout;

	setup = gtk_print_context_get_page_setup (context);
	gth_image_print_job_update_page_layout (self,
						page_nr,
						gtk_print_context_get_width (context),
						gtk_print_context_get_height (context),
						gtk_page_setup_get_orientation (setup),
						1.0);

	cr = gtk_print_context_get_cairo_context (context);
	pango_layout = gtk_print_context_create_pango_layout (context);
	gth_image_print_job_paint (self,
				   cr,
				   pango_layout,
				   0,
				   0,
				   page_nr,
				   FALSE);

	g_object_unref (pango_layout);
}


static void
print_operation_done_cb (GtkPrintOperation       *operation,
			 GtkPrintOperationResult  result,
			 gpointer                 user_data)
{
	GthImagePrintJob *self = user_data;

	if (result == GTK_PRINT_OPERATION_RESULT_ERROR) {
		GError *error = NULL;

		gtk_print_operation_get_error (self->priv->print_operation, &error);
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (self->priv->browser), _("Could not print"), &error);
		return;
	}

	g_object_unref (self);
}


GthImagePrintJob *
gth_image_print_job_new (GList *file_data_list)
{
	GthImagePrintJob *self;
	GList            *scan;
	int               n;

	self = g_object_new (GTH_TYPE_IMAGE_PRINT_JOB, NULL);

	self->priv->n_images = g_list_length (file_data_list);
	self->priv->images = g_new (GthImageInfo *, self->priv->n_images + 1);
	for (scan = file_data_list, n = 0; scan; scan = scan->next)
		self->priv->images[n++] = gth_image_info_new ((GthFileData *) scan->data);
	self->priv->images[n] = NULL;
	self->priv->requested_images_per_page = 1;
	self->priv->image_width = 0;
	self->priv->image_height = 0;

	self->priv->print_operation = gtk_print_operation_new ();
	gtk_print_operation_set_allow_async (self->priv->print_operation, TRUE);
	gtk_print_operation_set_custom_tab_label (self->priv->print_operation, _("Layout"));
	gtk_print_operation_set_embed_page_setup (self->priv->print_operation, TRUE);
	gtk_print_operation_set_show_progress (self->priv->print_operation, TRUE);

	g_signal_connect (self->priv->print_operation,
			  "create-custom-widget",
			  G_CALLBACK (operation_create_custom_widget_cb),
			  self);
	g_signal_connect (self->priv->print_operation,
			  "update-custom-widget",
			  G_CALLBACK (operation_update_custom_widget_cb),
			  self);
	g_signal_connect (self->priv->print_operation,
			  "begin_print",
			  G_CALLBACK (print_operation_begin_print_cb),
			  self);
	g_signal_connect (self->priv->print_operation,
			  "draw_page",
			  G_CALLBACK (print_operation_draw_page_cb),
			  self);
	g_signal_connect (self->priv->print_operation,
			  "done",
			  G_CALLBACK (print_operation_done_cb),
			  self);

	return self;
}


static void
load_image_info_task_completed_cb (GthTask  *task,
				   GError   *error,
				   gpointer  user_data)
{
	GthImagePrintJob        *self = user_data;
	GtkPrintOperationResult  result;

	if (error != NULL) {
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (self->priv->browser), _("Could not print"), &error);
		return;
	}

	result = gtk_print_operation_run (self->priv->print_operation,
					  self->priv->action,
					  GTK_WINDOW (self->priv->browser),
					  &error);
	if (result == GTK_PRINT_OPERATION_RESULT_ERROR) {
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (self->priv->browser), _("Could not print"), &error);
		return;
	}
}


void
gth_image_print_job_run (GthImagePrintJob        *self,
			 GtkPrintOperationAction  action,
			 GthBrowser              *browser)
{
	g_return_if_fail (self->priv->task == NULL);

	self->priv->action = action;
	self->priv->browser = browser;
	self->priv->task = gth_load_image_info_task_new (self->priv->images,
							 self->priv->n_images,
							 self->priv->caption_attributes);
	g_signal_connect (self->priv->task,
			  "completed",
			  G_CALLBACK (load_image_info_task_completed_cb),
			  self);
	gth_browser_exec_task (browser, self->priv->task, FALSE);
}
