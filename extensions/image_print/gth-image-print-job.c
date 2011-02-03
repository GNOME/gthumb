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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
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


#define DEFAULT_FONT_NAME "sans normal 10"
#define GET_WIDGET(name) _gtk_builder_get_widget (self->priv->builder, (name))


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
	int                 n_rows;
	int                 n_columns;
	int                 image_width;
	int                 image_height;
	GtkPageSetup       *page_setup;
	char               *caption_attributes;
	char               *font_name;
	double              scale_factor;
	int                 dpi;

	/* layout info */

	GthTask            *task;
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
	g_free (self->priv->font_name);

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
	self->priv->font_name = eel_gconf_get_string (PREF_IMAGE_PRINT_FONT_NAME, DEFAULT_FONT_NAME);
	self->priv->selected = NULL;
	self->priv->n_rows = eel_gconf_get_integer (PREF_IMAGE_PRINT_N_ROWS, 1);
	self->priv->n_columns = eel_gconf_get_integer (PREF_IMAGE_PRINT_N_COLUMNS, 1);
	self->priv->unit = eel_gconf_get_enum (PREF_IMAGE_PRINT_UNIT, GTH_TYPE_METRIC, GTH_METRIC_PIXELS);
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


static void
gth_image_print_job_update_layout_info (GthImagePrintJob   *self,
				        gdouble             page_width,
				        gdouble             page_height,
				        GtkPageOrientation  orientation)
{
	int rows;
	int columns;
	int current_page;
	int current_row;
	int current_column;
	int i;

	self->priv->x_padding = page_width / 40.0;
	self->priv->y_padding = page_height / 40.0;

	rows = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (GET_WIDGET ("rows_spinbutton")));
	columns = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (GET_WIDGET ("columns_spinbutton")));
	if ((orientation == GTK_PAGE_ORIENTATION_LANDSCAPE)
	    || (orientation == GTK_PAGE_ORIENTATION_REVERSE_LANDSCAPE))
	{
		int tmp = rows;
		rows = columns;
		columns = tmp;
	}

	self->priv->n_rows = rows;
	self->priv->n_columns = columns;
	self->priv->max_image_width = (page_width - ((columns - 1) * self->priv->x_padding)) / columns;
	self->priv->max_image_height = (page_height - ((rows - 1) * self->priv->y_padding)) / rows;

	self->priv->n_pages = MAX ((int) ceil ((double) self->priv->n_images / (self->priv->n_rows * self->priv->n_columns)), 1);
	if (self->priv->current_page >= self->priv->n_pages)
		self->priv->current_page = self->priv->n_pages - 1;

	current_page = 0;
	current_row = 1;
	current_column = 1;
	for (i = 0; i < self->priv->n_images; i++) {
		GthImageInfo *image_info = self->priv->images[i];

		image_info->page = current_page;
		image_info->col = current_column;
		image_info->row = current_row;

		current_column++;
		if (current_column > columns) {
			current_row++;
			current_column = 1;
		}

		if (current_row > rows) {
			current_page++;
			current_column = 1;
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
					 gboolean             preview)
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
			pango_layout_set_width (pango_layout, max_image_width * self->priv->scale_factor * PANGO_SCALE);
			pango_layout_get_pixel_extents (pango_layout, NULL, &logical_rect);

			image_info->comment.x = 0;
			image_info->comment.y = 0;
			image_info->comment.width = image_info->boundary.width;
			image_info->comment.height = logical_rect.height / self->priv->scale_factor;
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


static void
gth_image_print_job_set_font_options (GthImagePrintJob *self,
				      PangoLayout      *pango_layout,
				      gboolean          preview)
{
	PangoFontDescription *font_desc;
	double                size_in_points;
	cairo_font_options_t *options;
	PangoContext         *pango_context;

	pango_layout_set_wrap (pango_layout, PANGO_WRAP_WORD_CHAR);
	pango_layout_set_justify (pango_layout, FALSE);
	pango_layout_set_alignment (pango_layout, PANGO_ALIGN_CENTER);

	font_desc = pango_font_description_from_string (self->priv->font_name);
	if (preview)
		self->priv->scale_factor = 2.83;
	else
		self->priv->scale_factor = 1.0;

	size_in_points = (double) pango_font_description_get_size (font_desc) / PANGO_SCALE;
	pango_font_description_set_absolute_size (font_desc, size_in_points * PANGO_SCALE);
	pango_layout_set_font_description (pango_layout, font_desc);

	options = cairo_font_options_create ();
	cairo_font_options_set_hint_metrics (options, CAIRO_HINT_METRICS_OFF);
	pango_context = pango_layout_get_context (pango_layout);
	pango_cairo_context_set_font_options (pango_context, options);

	cairo_font_options_destroy (options);
	pango_font_description_free (font_desc);
}


static PangoLayout *
gth_image_print_job_create_pango_layout (GthImagePrintJob *self,
					 gboolean          preview)
{
	PangoLayout *pango_layout;

	pango_layout = gtk_widget_create_pango_layout (GTK_WIDGET (self->priv->browser), NULL);
	gth_image_print_job_set_font_options (self, pango_layout, preview);

	return pango_layout;
}


static void
gth_image_print_job_update_page_layout (GthImagePrintJob   *self,
					int                 page,
					gdouble             page_width,
					gdouble             page_height,
					GtkPageOrientation  orientation,
					gboolean            preview)
{
	PangoLayout  *pango_layout;
	char        **attributes_v;
	int           i;

	pango_layout = gth_image_print_job_create_pango_layout (self, preview);
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
							 preview);
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
	gth_image_print_job_update_page_layout (self, self->priv->current_page, page_width, page_height, orientation, TRUE);
}


static void
_cairo_paint_pixbuf (cairo_t   *cr,
		     double     x,
		     double     y,
		     double     width,
		     double     height,
		     GdkPixbuf *original_pixbuf,
		     int        dpi)
{
	double            scale_factor;
	GdkPixbuf        *pixbuf;
	guchar		 *p;
	int		  pw, ph, rs;
	guchar           *np;
	cairo_surface_t  *s;
	cairo_pattern_t	 *pattern;
	cairo_matrix_t    matrix;

	/* For higher-resolution images, cairo will render the bitmaps at a miserable
	   72 dpi unless we apply a scaling factor. This scaling boosts the output
	   to 300 dpi (if required). */

	scale_factor = MIN ((double) gdk_pixbuf_get_width (original_pixbuf) / width, (double) dpi / 72.0);
	pixbuf = gdk_pixbuf_scale_simple (original_pixbuf,
					  width * scale_factor,
					  height * scale_factor,
					  GDK_INTERP_BILINEAR);

	p = gdk_pixbuf_get_pixels (pixbuf);
	pw = gdk_pixbuf_get_width (pixbuf);
	ph = gdk_pixbuf_get_height (pixbuf);
	rs = gdk_pixbuf_get_rowstride (pixbuf);
	if (gdk_pixbuf_get_has_alpha (pixbuf)) {
		guchar *kk;
		guchar *kp;
		int     i;

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
		int     i;

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
	pattern = cairo_pattern_create_for_surface (s);
	cairo_matrix_init_translate (&matrix, -x * scale_factor, -y * scale_factor);
	cairo_matrix_scale (&matrix, scale_factor, scale_factor);
	cairo_pattern_set_matrix (pattern, &matrix);
	cairo_pattern_set_extend (pattern, CAIRO_EXTEND_NONE);
	cairo_pattern_set_filter (pattern, CAIRO_FILTER_BEST);
	cairo_set_source (cr, pattern);
	cairo_paint (cr);
	cairo_restore (cr);

	cairo_pattern_destroy (pattern);
	cairo_surface_destroy (s);
	g_free (np);
	g_object_unref (pixbuf);
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
	int i;

	gth_image_print_job_set_font_options (self, pango_layout, preview);
	for (i = 0; i < self->priv->n_images; i++) {
		GthImageInfo *image_info = self->priv->images[i];
		GdkPixbuf    *fullsize_pixbuf;

		if (image_info->page != page)
			continue;

		if (preview) {
#if 0
			cairo_save (cr);
			cairo_set_line_width (cr, 0.5);
			cairo_set_source_rgb (cr, 1.0, 0.0, 0.0);
			cairo_rectangle (cr,
					 x_offset + image_info->comment.x,
					 y_offset + image_info->comment.y,
					 image_info->comment.width,
					 image_info->comment.height);
			cairo_stroke (cr);
			cairo_restore (cr);
#endif

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
			if (preview) {
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
			else
				_cairo_paint_pixbuf (cr,
						     x_offset + image_info->image.x,
						     y_offset + image_info->image.y,
						     image_info->image.width,
						     image_info->image.height,
						     fullsize_pixbuf,
						     self->priv->dpi);
		}

		if (image_info->print_comment) {
			cairo_save (cr);

			pango_layout_set_width (pango_layout, image_info->comment.width * self->priv->scale_factor * PANGO_SCALE);
			pango_layout_set_text (pango_layout, image_info->comment_text, -1);

			cairo_move_to (cr, x_offset + image_info->comment.x, y_offset + image_info->comment.y);
			if (preview)
				cairo_scale (cr, 1.0 / self->priv->scale_factor, 1.0 / self->priv->scale_factor);

			pango_cairo_layout_path (cr, pango_layout);
			cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
			cairo_fill (cr);

			cairo_restore (cr);
		}

		g_object_unref (fullsize_pixbuf);
	}
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

	g_return_if_fail (GTK_IS_PAGE_SETUP (self->priv->page_setup));

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

	pango_layout = gth_image_print_job_create_pango_layout (self, TRUE);
	attributes_v = g_strsplit (self->priv->caption_attributes, ",", -1);
	gth_image_print_job_update_image_layout (self,
						 image_info,
						 pango_layout,
						 attributes_v,
						 gtk_page_setup_get_page_width (self->priv->page_setup, GTK_UNIT_MM),
						 gtk_page_setup_get_page_height (self->priv->page_setup, GTK_UNIT_MM),
						 gtk_page_setup_get_orientation (self->priv->page_setup),
						 TRUE);
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
	GtkAllocation     allocation;
	PangoLayout      *pango_layout;

	g_return_if_fail (GTH_IS_IMAGE_PRINT_JOB (self));
	g_return_if_fail ((self->priv->page_setup != NULL) && GTK_IS_PAGE_SETUP (self->priv->page_setup));

	cr = gdk_cairo_create (gtk_widget_get_window (widget));

	/* paint the paper */

	cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
	gtk_widget_get_allocation (widget, &allocation);
	cairo_rectangle (cr, 0, 0, allocation.width - 1, allocation.height - 1);
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
rows_spinbutton_changed_cb (GtkSpinButton *widget,
			    gpointer       user_data)
{
	GthImagePrintJob *self = user_data;
	int               i;

	self->priv->n_rows = gtk_spin_button_get_value_as_int (widget);
	for (i = 0; i < self->priv->n_images; i++)
		gth_image_info_reset (self->priv->images[i]);
	gth_image_print_job_update_preview (self);
}


static void
columns_spinbutton_changed_cb (GtkSpinButton *widget,
			       gpointer       user_data)
{
	GthImagePrintJob *self = user_data;
	int               i;

	self->priv->n_columns = gtk_spin_button_get_value_as_int (widget);
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
metadata_ready_cb (GObject      *source_object,
                   GAsyncResult *result,
                   gpointer      user_data)
{
	GthImagePrintJob *self = user_data;
	GError           *error;

	_g_query_metadata_finish (result, &error);
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
	reload_required = attribute_list_reload_required (self->priv->caption_attributes, new_caption_attributes);
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
	default:
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
caption_fontbutton_font_set_cb (GtkFontButton *font_button,
				gpointer       user_data)
{
	GthImagePrintJob *self = user_data;

	g_free (self->priv->font_name);
	self->priv->font_name = g_strdup (gtk_font_button_get_font_name (font_button));
	eel_gconf_set_string (PREF_IMAGE_PRINT_FONT_NAME, self->priv->font_name);

	gth_image_print_job_update_preview (self);
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
	gtk_font_button_set_font_name (GTK_FONT_BUTTON (GET_WIDGET ("caption_fontbutton")), self->priv->font_name);

	gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("rows_spinbutton")), self->priv->n_rows);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("columns_spinbutton")), self->priv->n_columns);

	gtk_combo_box_set_active (GTK_COMBO_BOX (GET_WIDGET ("unit_combobox")), eel_gconf_get_enum (PREF_IMAGE_PRINT_UNIT, GTH_TYPE_METRIC, GTH_METRIC_PIXELS));

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
	g_signal_connect (GET_WIDGET ("rows_spinbutton"),
			  "value-changed",
	                  G_CALLBACK (rows_spinbutton_changed_cb),
	                  self);
	g_signal_connect (GET_WIDGET ("columns_spinbutton"),
			  "value-changed",
	                  G_CALLBACK (columns_spinbutton_changed_cb),
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
	g_signal_connect (GET_WIDGET ("caption_fontbutton"),
			  "font-set",
			  G_CALLBACK (caption_fontbutton_font_set_cb),
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

	if (self->priv->page_setup != NULL) {
		int i;

		gtk_widget_set_size_request (GET_WIDGET ("preview_drawingarea"),
					     gtk_page_setup_get_paper_width (self->priv->page_setup, GTK_UNIT_MM),
					     gtk_page_setup_get_paper_height (self->priv->page_setup, GTK_UNIT_MM));
		for (i = 0; i < self->priv->n_images; i++)
			gth_image_info_reset (self->priv->images[i]);
		gth_image_print_job_update_preview (self);
	}

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
	self->priv->page_setup = NULL;

	if (setup == NULL)
		return;

	self->priv->page_setup = gtk_page_setup_copy (setup);
	self->priv->dpi = gtk_print_settings_get_resolution (settings);
	gtk_widget_set_size_request (GET_WIDGET ("preview_drawingarea"),
				     gtk_page_setup_get_paper_width (setup, GTK_UNIT_MM),
				     gtk_page_setup_get_paper_height (setup, GTK_UNIT_MM));
	for (i = 0; i < self->priv->n_images; i++)
		gth_image_info_reset (self->priv->images[i]);
	gth_image_print_job_update_preview (self);
}


static void
operation_custom_widget_apply_cb (GtkPrintOperation *operation,
				  GtkWidget         *widget,
				  gpointer           user_data)
{
	GthImagePrintJob *self = user_data;

	eel_gconf_set_integer (PREF_IMAGE_PRINT_N_ROWS, self->priv->n_rows);
	eel_gconf_set_integer (PREF_IMAGE_PRINT_N_COLUMNS, self->priv->n_columns);
	eel_gconf_set_enum (PREF_IMAGE_PRINT_UNIT, GTH_TYPE_METRIC, gtk_combo_box_get_active (GTK_COMBO_BOX (GET_WIDGET ("unit_combobox"))));
}


static void
print_operation_begin_print_cb (GtkPrintOperation *operation,
				GtkPrintContext   *context,
				gpointer           user_data)
{
	GthImagePrintJob *self = user_data;
	GtkPrintSettings *settings;
	char             *filename;

	_g_object_unref (self->priv->page_setup);
	self->priv->page_setup = gtk_page_setup_copy (gtk_print_context_get_page_setup (context));

	settings = gtk_print_operation_get_print_settings (operation);
	self->priv->dpi = gtk_print_settings_get_resolution (settings);

	/* save the page setup */

	filename = gth_user_dir_get_file (GTH_DIR_CONFIG, "gthumb", "page_setup", NULL);
	gtk_page_setup_to_file (self->priv->page_setup, filename, NULL);
	g_free (filename);

	gth_image_print_job_update_layout_info (self,
						gtk_print_context_get_width (context),
						gtk_print_context_get_height (context),
						gtk_page_setup_get_orientation (self->priv->page_setup));
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
						FALSE);

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
	else if (result == GTK_PRINT_OPERATION_RESULT_APPLY) {
		GtkPrintSettings *settings;
		char             *filename;

		settings = gtk_print_operation_get_print_settings (operation);
		filename = gth_user_dir_get_file (GTH_DIR_CONFIG, "gthumb", "print_settings", NULL);
		gtk_print_settings_to_file (settings, filename, NULL);
		g_free (filename);
	}

	g_object_unref (self);
}


GthImagePrintJob *
gth_image_print_job_new (GList        *file_data_list,
			 GthFileData  *current,
			 GdkPixbuf    *current_image,
			 GError      **error)
{
	GthImagePrintJob *self;
	GList            *scan;
	int               n;

	self = g_object_new (GTH_TYPE_IMAGE_PRINT_JOB, NULL);

	self->priv->n_images = g_list_length (file_data_list);
	self->priv->images = g_new (GthImageInfo *, self->priv->n_images + 1);
	for (scan = file_data_list, n = 0; scan; scan = scan->next) {
		GthFileData *file_data = scan->data;

		if (_g_mime_type_is_image (gth_file_data_get_mime_type (file_data))) {
			GthImageInfo *image_info;

			image_info = gth_image_info_new (file_data);
			if ((current_image != NULL) && g_file_equal (file_data->file, current->file))
				gth_image_info_set_pixbuf (image_info, current_image);

			self->priv->images[n++] = image_info;
		}
	}
	self->priv->images[n] = NULL;
	self->priv->n_images = n;
	self->priv->image_width = 0;
	self->priv->image_height = 0;

	if (self->priv->n_images == 0) {
		if (error != NULL)
			*error = g_error_new_literal (GTH_ERROR, GTH_ERROR_GENERIC, _("No valid file selected."));
		g_object_unref (self);
		return NULL;
	}

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
			  "custom-widget-apply",
			  G_CALLBACK (operation_custom_widget_apply_cb),
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
	GthImagePrintJob         *self = user_data;
	int                       n_loaded_images;
	GthImageInfo            **loaded_images;
	int                       i, j;
	GtkPrintOperationResult   result;
	char                     *filename;
	GtkPrintSettings         *settings;

	if (error != NULL) {
		g_object_unref (self);
		return;
	}

	n_loaded_images = 0;
	for (i = 0; i < self->priv->n_images; i++) {
		GthImageInfo *image_info = self->priv->images[i];

		if (image_info->thumbnail == NULL) {
			gth_image_info_unref (self->priv->images[i]);
			self->priv->images[i] = NULL;
		}
		else
			n_loaded_images += 1;
	}

	if (n_loaded_images == 0) {
		_gtk_error_dialog_show (GTK_WINDOW (self->priv->browser),
					_("Could not print"),
					"%s",
					_("No suitable loader available for this file type"));
		g_object_unref (self);
		return;
	}

	loaded_images = g_new (GthImageInfo *, n_loaded_images + 1);
	for (i = 0, j = 0; i < self->priv->n_images; i++) {
		if (self->priv->images[i] != NULL) {
			loaded_images[j] = self->priv->images[i];
			j += 1;
		}
	}
	loaded_images[j] = NULL;

	g_free (self->priv->images);
	self->priv->images = loaded_images;
	self->priv->n_images = n_loaded_images;

	filename = gth_user_dir_get_file (GTH_DIR_CONFIG, "gthumb", "print_settings", NULL);
	settings = gtk_print_settings_new_from_file (filename, NULL);
	if (settings != NULL)
		gtk_print_operation_set_print_settings (self->priv->print_operation, settings);
	g_free (filename);

	filename = gth_user_dir_get_file (GTH_DIR_CONFIG, "gthumb", "page_setup", NULL);
	self->priv->page_setup = gtk_page_setup_new_from_file (filename, NULL);
	if (self->priv->page_setup != NULL)
		gtk_print_operation_set_default_page_setup (self->priv->print_operation, self->priv->page_setup);
	g_free (filename);

	result = gtk_print_operation_run (self->priv->print_operation,
					  self->priv->action,
					  GTK_WINDOW (self->priv->browser),
					  &error);
	if (result == GTK_PRINT_OPERATION_RESULT_ERROR)
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (self->priv->browser), _("Could not print"), &error);

	_g_object_unref (settings);
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
