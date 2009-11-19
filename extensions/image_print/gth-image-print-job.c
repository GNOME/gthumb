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


#define GET_WIDGET(name) _gtk_builder_get_widget (self->priv->builder, (name))


static gpointer parent_class = NULL;


struct _GthImagePrintJobPrivate {
	GtkPrintOperationAction  action;
	GthBrowser         *browser;
	GtkPrintOperation  *print_operation;
	GtkBuilder         *builder;

	/* settings */

	GthImageInfo      **images;
	int                 n_images;
	int                 requested_images_per_page;
	gboolean	    auto_sizing;
	int                 image_width;
	int                 image_height;
	GtkPageSetup       *page_setup;

	/* layout info */

	GthTask            *task;
	int                 real_images_per_page;
	double              max_image_width;
	double		    max_image_height;
	int                 n_pages;
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


static GObject *
operation_create_custom_widget_cb (GtkPrintOperation *operation,
			           gpointer           user_data)
{
	GthImagePrintJob *self = user_data;

	self->priv->builder = _gtk_builder_new_from_file ("print-layout.ui", "image_print");

	return gtk_builder_get_object (self->priv->builder, "print_layout");
}


static void
operation_custom_widget_apply_cb (GtkPrintOperation *operation,
				  GtkWidget         *widget,
				  gpointer           user_data)
{
	/* FIXME */
}


static void
operation_update_custom_widget_cb (GtkPrintOperation *operation,
				   GtkWidget         *widget,
				   GtkPageSetup      *setup,
				   GtkPrintSettings  *settings,
				   gpointer           user_data)
{
}


enum {
	IMAGES_PER_PAGE_1,
	IMAGES_PER_PAGE_2,
	IMAGES_PER_PAGE_4,
	IMAGES_PER_PAGE_8,
	IMAGES_PER_PAGE_16,
	N_IMAGES_PER_PAGE
};
static int n_rows_for_ipp[N_IMAGES_PER_PAGE] = { 1, 2, 2, 4, 4 };
static int n_cols_for_ipp[N_IMAGES_PER_PAGE] = { 1, 1, 2, 2, 4 };
#define DEFAULT_PADDING 20.0


static double
_log2 (double x)
{
	return log (x) / log (2);
}


static void
print_operation_begin_print_cb (GtkPrintOperation *operation,
				GtkPrintContext   *context,
				gpointer           user_data)
{
	GthImagePrintJob *self = user_data;
	GtkPageSetup     *setup;
	gdouble           page_width;
	gdouble           page_height;
	double            x_padding;
	double            y_padding;
	int               rows;
	int               cols;
	int               current_page;
	int               current_row;
	int               current_col;
	int               i;

	setup = gtk_print_context_get_page_setup (context);
	page_width = gtk_print_context_get_width (context);
	page_height = gtk_print_context_get_height (context);
	x_padding = DEFAULT_PADDING;
	y_padding = DEFAULT_PADDING;

	if (self->priv->auto_sizing) {
		int idx;

		self->priv->real_images_per_page = self->priv->requested_images_per_page;

		idx = (int) floor (_log2 (self->priv->real_images_per_page) + 0.5);
		rows = n_rows_for_ipp[idx];
		cols = n_cols_for_ipp[idx];
		if ((gtk_page_setup_get_orientation (setup) == GTK_PAGE_ORIENTATION_LANDSCAPE)
		    || (gtk_page_setup_get_orientation (setup) == GTK_PAGE_ORIENTATION_REVERSE_LANDSCAPE))
		{
			int tmp = rows;
			rows = cols;
			cols = tmp;
		}

		self->priv->max_image_width = (page_width - ((cols - 1) * x_padding)) / cols;
		self->priv->max_image_height = (page_height - ((rows - 1) * y_padding)) / rows;
	}
	else {
		double image_width;
		double image_height;
		double tmp_cols;
		double tmp_rows;

		image_width = self->priv->image_width;
		image_height = self->priv->image_height;
		tmp_cols = (int) floor ((page_width + x_padding) / (image_height + x_padding));
		tmp_rows = (int) floor ((page_height + y_padding) / (image_width + y_padding));
		cols = (int) floor ((page_width + x_padding) / (image_width + x_padding));
		rows = (int) floor ((page_height + y_padding) / (image_height + y_padding));

		if ((tmp_rows * tmp_cols > cols * rows)
		    && (image_height <= page_width)
		    && (image_width <= page_width))
		{
			double tmp = image_width;
			image_width = image_height;
			image_height = tmp;
			rows = tmp_rows;
			cols = tmp_cols;
		}

		if (rows == 0) {
			rows = 1;
			image_height = page_height - y_padding;
		}

		if (cols == 0) {
			cols = 1;
			image_width = page_width - x_padding;
		}

		self->priv->real_images_per_page = rows * cols;

		if (cols > 1)
			x_padding = (page_width - (cols * image_width)) / (cols - 1);
		else
			x_padding = page_width - image_width;

		if (rows > 1)
			y_padding = (page_height - (rows * image_height)) / (rows - 1);
		else
			y_padding = page_height - image_height;

		self->priv->max_image_width = image_width;
		self->priv->max_image_height = image_height;
	}

	self->priv->n_pages = MAX ((int) ceil ((double) self->priv->n_images / self->priv->real_images_per_page), 1);
	gtk_print_operation_set_n_pages (operation, self->priv->n_pages);

	current_page = 0;
	current_row = 1;
	current_col = 1;
	for (i = 0; i < self->priv->n_images; i++) {
		GthImageInfo *image_info = self->priv->images[i];
		double        max_image_width;
		double        max_image_height;
		double        image_width;
		double        image_height;
		double        factor;

		image_info->n_page = current_page;

		gth_image_info_rotate (image_info, (360 - image_info->rotate) % 360);
		if (((self->priv->max_image_width > self->priv->max_image_height)
		     && (image_info->pixbuf_width < image_info->pixbuf_height))
		    || ((self->priv->max_image_width < self->priv->max_image_height)
			&& (image_info->pixbuf_width > image_info->pixbuf_height)))
		{
			gth_image_info_rotate (image_info, 270);
		}

		image_info->zoom = 1.0;
		image_info->min_x = (current_col - 1) * (self->priv->max_image_width + x_padding);
		image_info->min_y = (current_row - 1) * (self->priv->max_image_height + y_padding);
		image_info->max_x = image_info->min_x + self->priv->max_image_width;
		image_info->max_y = image_info->min_y + self->priv->max_image_height;

		current_col++;
		if (current_col > cols) {
			current_row++;
			current_col = 1;
		}

		max_image_width = self->priv->max_image_width;
		max_image_height = self->priv->max_image_height;

		/* FIXME: change max_image_width/max_image_height to make space to the comment */

		image_width = (double) image_info->pixbuf_width;
		image_height = (double) image_info->pixbuf_height;
		factor = MIN (max_image_width / image_width, max_image_height / image_height);
		image_info->width = image_width * factor;
		image_info->height = image_height * factor;
		image_info->trans_x = image_info->min_x + ((max_image_width - image_info->width) / 2);
		image_info->trans_y = image_info->min_y + ((max_image_height - image_info->height) / 2);
		image_info->scale_x = image_info->width * image_info->zoom;
		image_info->scale_y = image_info->height * image_info->zoom;

		if ((i + 1 < self->priv->n_images) && ((i + 1) % self->priv->real_images_per_page == 0)) {
			current_page++;
			current_col = 1;
			current_row = 1;
		}
	}
}


static void
print_operation_draw_page_cb (GtkPrintOperation *operation,
			      GtkPrintContext   *context,
			      int                page_nr,
			      GthImagePrintJob  *self)
{
	cairo_t *cr;
	int      i;

	cr = gtk_print_context_get_cairo_context (context);

	for (i = 0; i < self->priv->n_images; i++) {
		GthImageInfo    *image_info = self->priv->images[i];
		double           scale_factor;
		GdkPixbuf       *pixbuf;

		if (image_info->n_page != page_nr)
			continue;

#if 0
		cairo_set_source_rgb (cr, 0, 0, 0);
		cairo_rectangle (cr,
				 image_info->trans_x,
				 image_info->trans_y,
				 image_info->width,
				 image_info->height);
		cairo_stroke (cr);
#endif

		/* For higher-resolution images, cairo will render the bitmaps at a miserable
		 * 72 dpi unless we apply a scaling factor. This scaling boosts the output
		 * to 300 dpi (if required). */

		scale_factor = MIN (image_info->pixbuf_width / image_info->scale_x, gtk_print_context_get_dpi_x (context) / 72.0);
		pixbuf = gdk_pixbuf_scale_simple (image_info->pixbuf,
						  image_info->scale_x * scale_factor,
						  image_info->scale_y * scale_factor,
						  GDK_INTERP_BILINEAR);
		cairo_save (cr);
		gdk_cairo_set_source_pixbuf (cr, pixbuf, image_info->trans_x, image_info->trans_y);
		cairo_rectangle (cr,
				 image_info->trans_x,
				 image_info->trans_y,
				 gdk_pixbuf_get_width (pixbuf),
				 gdk_pixbuf_get_height (pixbuf));
		cairo_clip (cr);
		cairo_paint (cr);
		cairo_restore (cr);

		g_object_unref (pixbuf);
	}

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
	self->priv->requested_images_per_page = 4; /* FIXME: set correct default values */
	self->priv->auto_sizing = TRUE;
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
			  "custom-widget-apply",
			  G_CALLBACK (operation_custom_widget_apply_cb),
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
	self->priv->task = gth_load_image_info_task_new (self->priv->images, self->priv->n_images);
	g_signal_connect (self->priv->task,
			  "completed",
			  G_CALLBACK (load_image_info_task_completed_cb),
			  self);
	gth_browser_exec_task (browser, self->priv->task, FALSE);
}
