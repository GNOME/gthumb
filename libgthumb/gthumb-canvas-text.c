/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * $Id$
 * Copyright (C) 1997, 1998, 1999, 2000 Free Software Foundation
 * All rights reserved.
 *
 * This file is part of the Gnome Library.
 *
 * The Gnome Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Gnome Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Gnome Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/*
  @NOTATION@
 */
/* Text item type for GnomeCanvas widget
 *
 * GnomeCanvas is basically a port of the Tk toolkit's most excellent canvas
 * widget.  Tk is copyrighted by the Regents of the University of California,
 * Sun Microsystems, and other parties.
 *
 *
 * Author: Federico Mena <federico@nuclecu.unam.mx>
 * Port to Pango co-done by Gergõ Érdi <cactus@cactus.rulez.org>
 */

#include <config.h>
#include <math.h>
#include <string.h>
#include <libgnomecanvas/gnome-canvas-text.h>
#include <pango/pango-layout.h>
#include <pango/pangoft2.h>
#include "gthumb-canvas-text.h"



/* Object argument IDs */
enum {
	PROP_0,
	PROP_PANGO_LAYOUT_WIDTH,
	PROP_PANGO_WRAP_MODE
};


static void gthumb_canvas_text_class_init   (GthumbCanvasTextClass *class);
static void gthumb_canvas_text_init         (GthumbCanvasText      *text);
static void gthumb_canvas_text_destroy      (GtkObject             *object);
static void gthumb_canvas_text_set_property (GObject               *object,
					     guint                  param_id,
					     const GValue          *value,
					     GParamSpec            *pspec);
static void gthumb_canvas_text_get_property (GObject               *object,
					     guint                  param_id,
					     GValue                *value,
					     GParamSpec            *pspec);


static GnomeCanvasTextClass *parent_class;




GType
gthumb_canvas_text_get_type (void)
{
	static GType text_type;

	if (!text_type) {
		static const GTypeInfo object_info = {
			sizeof (GthumbCanvasTextClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) gthumb_canvas_text_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,			/* class_data */
			sizeof (GthumbCanvasText),
			0,			/* n_preallocs */
			(GInstanceInitFunc) gthumb_canvas_text_init,
			NULL			/* value_table */
		};

		text_type = g_type_register_static (GNOME_TYPE_CANVAS_TEXT, 
						    "GthumbCanvasText",
						    &object_info, 
						    0);
	}

	return text_type;
}


static void
gthumb_canvas_text_class_init (GthumbCanvasTextClass *class)
{
	GObjectClass   *gobject_class;
	GtkObjectClass *object_class;

	gobject_class = (GObjectClass *) class;
	object_class  = (GtkObjectClass *) class;
	parent_class  = g_type_class_peek_parent (class);

	gobject_class->set_property = gthumb_canvas_text_set_property;
	gobject_class->get_property = gthumb_canvas_text_get_property;

	g_object_class_install_property
		(gobject_class,
		 PROP_PANGO_LAYOUT_WIDTH,
		 g_param_spec_int ("layout_width",
				   "Pango layout width",
				   "Pango layout width",
				   0,
				   G_MAXINT,
				   0,
				   G_PARAM_READABLE | G_PARAM_WRITABLE));

	g_object_class_install_property
		(gobject_class,
		 PROP_PANGO_WRAP_MODE,
		 g_param_spec_enum ("wrap_mode",
				    "Pango layout wrap mode",
				    "Pango layout wrap mode",
				    PANGO_TYPE_WRAP_MODE,
				    PANGO_WRAP_CHAR,
				    G_PARAM_READABLE | G_PARAM_WRITABLE));

	object_class->destroy = gthumb_canvas_text_destroy;
}


static void
gthumb_canvas_text_init (GthumbCanvasText *text)
{
	text->pango_layout_width = 0; /* FIXME */
	text->pango_wrap_mode = PANGO_WRAP_CHAR;
}


static void
gthumb_canvas_text_destroy (GtkObject *object)
{
	g_return_if_fail (GNOME_IS_CANVAS_TEXT (object));
	
	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}


static void
gthumb_canvas_text_set_property (GObject            *object,
				 guint               param_id,
				 const GValue       *value,
				 GParamSpec         *pspec)
{
	GnomeCanvasItem  *item;
	GnomeCanvasText  *text;
	GthumbCanvasText *gthumb_text;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_CANVAS_TEXT (object));

	item = GNOME_CANVAS_ITEM (object);
	text = GNOME_CANVAS_TEXT (object);
	gthumb_text = GTHUMB_CANVAS_TEXT (object);

	if (!text->layout) {

	        PangoContext *gtk_context, *context;
		gtk_context = gtk_widget_get_pango_context (GTK_WIDGET (item->canvas));
		
	        if (item->canvas->aa)  {
			PangoLanguage *language;
			gint	pixels, mm;
			double	dpi_x;
			double	dpi_y;
			
			pixels = gdk_screen_width ();
			mm = gdk_screen_width_mm ();
			dpi_x = (((double) pixels * 25.4) / (double) mm);
			
			pixels = gdk_screen_height ();
			mm = gdk_screen_height_mm ();
			dpi_y = (((double) pixels * 25.4) / (double) mm);
			
		        context = pango_ft2_get_context (dpi_x, dpi_y);
			language = pango_context_get_language (gtk_context);
			pango_context_set_language (context, language);
			pango_context_set_base_dir (context,
						    pango_context_get_base_dir (gtk_context));
			pango_context_set_font_description (context,
							    pango_context_get_font_description (gtk_context));
			
		} else
			context = gtk_context;
			

		text->layout = pango_layout_new (context);
		
	        if (item->canvas->aa)
		        g_object_unref (G_OBJECT (context));
	}

	switch (param_id) {
	case PROP_PANGO_LAYOUT_WIDTH:
		gthumb_text->pango_layout_width = g_value_get_int (value);
		pango_layout_set_width (text->layout, gthumb_text->pango_layout_width * PANGO_SCALE);
		break;

	case PROP_PANGO_WRAP_MODE:
		gthumb_text->pango_wrap_mode = g_value_get_enum (value);
		pango_layout_set_wrap (text->layout, gthumb_text->pango_wrap_mode);
		break;	

	default:
		(* G_OBJECT_CLASS (parent_class)->set_property) (object, param_id, value, pspec);
	}

	/* Calculate text dimensions */

	if (text->layout)
	        pango_layout_get_pixel_size (text->layout,
					     &text->max_width,
					     &text->height);
	else {
		text->max_width = 0;
		text->height = 0;
	}
	
	gnome_canvas_item_request_update (item);
}


static void
gthumb_canvas_text_get_property (GObject            *object,
				 guint               param_id,
				 GValue             *value,
				 GParamSpec         *pspec)
{
	GnomeCanvasText *text;
	GthumbCanvasText *gthumb_text;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GTHUMB_IS_CANVAS_TEXT (object));

	text = GNOME_CANVAS_TEXT (object);
	gthumb_text = GTHUMB_CANVAS_TEXT (object);

	switch (param_id) {
	case PROP_PANGO_LAYOUT_WIDTH:
		g_value_set_int (value, gthumb_text->pango_layout_width);
		break;

	case PROP_PANGO_WRAP_MODE:
		g_value_set_enum (value, gthumb_text->pango_wrap_mode);
		break;

	default:
		(* G_OBJECT_CLASS (parent_class)->get_property) (object, param_id, value, pspec);
		break;
	}
}
