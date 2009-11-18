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
#include <stdlib.h>
#include <gtk/gtk.h>
#include <gthumb.h>
#include "gth-image-print-job.h"


#define GET_WIDGET(name) _gtk_builder_get_widget (self->priv->builder, (name))


static gpointer parent_class = NULL;


typedef struct {
	GthFileData *file_data;
	char        *comment;
	int          pixbuf_width;
	int          pixbuf_height;
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
	gboolean     print_comment;
} ImageInfo;


static ImageInfo *
image_info_new (GthFileData *file_data)
{
	ImageInfo *image = g_new0 (ImageInfo, 1);

	image->file_data = g_object_ref (file_data);
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

	g_object_unref (image->file_data);
	g_free (image->comment);
	_g_object_unref (image->thumbnail);
	_g_object_unref (image->thumbnail_active);
	g_free (image);
}



struct _GthImagePrintJobPrivate {
	GtkWindow          *parent;
	GtkPrintOperation  *print_operation;
	GtkBuilder         *builder;
	ImageInfo         **images;
	int                 n_images;
	int                 images_per_page;
	gboolean	    auto_sizing;
	double              max_image_width;
	double		    max_image_height;
};


static void
gth_image_print_job_finalize (GObject *base)
{
	GthImagePrintJob *self;
	int               i;

	self = GTH_IMAGE_PRINT_JOB (base);

	_g_object_unref (self->priv->print_operation);
	_g_object_unref (self->priv->builder);
	for (i = 0; i < self->priv->n_images; i++)
		image_info_free (self->priv->images[i]);
	g_free (self->priv->images);

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
	/* FIXME */
}


static void
print_operation_begin_print_cb (GtkPrintOperation *operation,
				GtkPrintContext   *context,
				GthImagePrintJob  *self)
{
	gtk_print_operation_set_n_pages (operation, 1);

	/* FIXME
	gtk_print_operation_set_n_pages (operation, (pci->n_images + pci->images_per_page - 1) / pci->images_per_page);
	gtk_print_operation_set_default_page_setup (operation, pci->page_setup);
	gtk_print_operation_set_show_progress (operation, TRUE);
	*/
}


static void
print_operation_draw_page_cb (GtkPrintOperation *operation,
			      GtkPrintContext   *context,
			      int                page_nr,
			      GthImagePrintJob  *self)
{
	cairo_t        *cr;
	PangoLayout    *layout;
	int             y;
	PangoRectangle  rect;

	cr = gtk_print_context_get_cairo_context (context);
	cairo_set_source_rgb (cr, 0, 0, 0);
	cairo_rectangle (cr, 0, 0, gtk_print_context_get_width (context), gtk_print_context_get_height (context));
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
		_gtk_error_dialog_from_gerror_show (self->priv->parent, _("Could not print"), &error);
		return;
	}

	g_object_unref (self);
}


GthImagePrintJob *
gth_image_print_job_new (void)
{
	GthImagePrintJob *self;

	self = g_object_new (GTH_TYPE_IMAGE_PRINT_JOB, NULL);

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


void
gth_image_print_job_run (GthImagePrintJob        *self,
			 GtkPrintOperationAction  action,
			 GtkWindow               *parent)
{
	GtkPrintOperationResult  result;
	GError                  *error = NULL;

	self->priv->parent = parent;
	result = gtk_print_operation_run (self->priv->print_operation,
					  action,
					  parent,
					  &error);
	if (result == GTK_PRINT_OPERATION_RESULT_ERROR) {
		_gtk_error_dialog_from_gerror_show (parent, _("Could not print"), &error);
		return;
	}
}

