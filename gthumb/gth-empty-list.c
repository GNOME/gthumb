/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2008 The Free Software Foundation, Inc.
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

#include "gth-empty-list.h"

/* Properties */
enum {
        PROP_0,
        PROP_TEXT
};

struct _GthEmptyListPrivate {
	GdkWindow   *bin_window;
	char        *text;
	PangoLayout *layout;
};

static gpointer parent_class = NULL;


static void 
gth_empty_list_finalize (GObject *obj) 
{
	GthEmptyList *self;
	
	self = GTH_EMPTY_LIST (obj);
	
	if (self->priv != NULL) { 
		g_free (self->priv->text);
		g_free (self->priv);
	}
	
	G_OBJECT_CLASS (parent_class)->finalize (obj);
}


static void
gth_empty_list_set_property (GObject      *object,
			     guint         property_id,
			     const GValue *value,
			     GParamSpec   *pspec)
{
	GthEmptyList *self;

        self = GTH_EMPTY_LIST (object);

	switch (property_id) {
	case PROP_TEXT:
		g_free (self->priv->text);
		self->priv->text = g_value_dup_string (value);
		gtk_widget_queue_resize (GTK_WIDGET (self));
		break;
	default:
		break;
	}
}


static void
gth_empty_list_get_property (GObject    *object,
			     guint       property_id,
			     GValue     *value,
			     GParamSpec *pspec)
{
	GthEmptyList *self;

        self = GTH_EMPTY_LIST (object);

	switch (property_id) {
	case PROP_TEXT:
		g_value_set_string (value, self->priv->text);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}


static void
gth_empty_list_realize (GtkWidget *widget)
{
	GthEmptyList  *self;
	GtkAllocation  allocation;
	GdkWindowAttr  attributes;
	int            attributes_mask;
	GdkWindow     *window;
	GtkStyle      *style;

	g_return_if_fail (GTH_IS_EMPTY_LIST (widget));
	self = (GthEmptyList*) widget;

	gtk_widget_set_realized (widget, TRUE);

	/**/

	gtk_widget_get_allocation (widget, &allocation);

	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.x           = allocation.x;
	attributes.y           = allocation.y;
	attributes.width       = allocation.width;
	attributes.height      = allocation.height;
	attributes.wclass      = GDK_INPUT_OUTPUT;
	attributes.visual      = gtk_widget_get_visual (widget);
	attributes.colormap    = gtk_widget_get_colormap (widget);
	attributes.event_mask  = GDK_VISIBILITY_NOTIFY_MASK;
	attributes_mask        = (GDK_WA_X
				  | GDK_WA_Y
				  | GDK_WA_VISUAL
				  | GDK_WA_COLORMAP);
	window = gdk_window_new (gtk_widget_get_parent_window (widget),
			         &attributes,
			         attributes_mask);
	gtk_widget_set_window (widget, window);
	gdk_window_set_user_data (window, widget);

	/**/

	attributes.x = 0;
	attributes.y = 0;
	attributes.width = allocation.width;
	attributes.height = allocation.height;
	attributes.event_mask = (GDK_EXPOSURE_MASK
				 | GDK_SCROLL_MASK
				 | GDK_POINTER_MOTION_MASK
				 | GDK_ENTER_NOTIFY_MASK
				 | GDK_LEAVE_NOTIFY_MASK
				 | GDK_BUTTON_PRESS_MASK
				 | GDK_BUTTON_RELEASE_MASK
				 | gtk_widget_get_events (widget));

	self->priv->bin_window = gdk_window_new (window,
						 &attributes,
						 attributes_mask);
	gdk_window_set_user_data (self->priv->bin_window, widget);

	/* Style */

	style = gtk_widget_get_style (widget);
	style = gtk_style_attach (style, window);
	gtk_widget_set_style (widget, style);
	gdk_window_set_background (window, &style->base[gtk_widget_get_state (widget)]);
	gdk_window_set_background (self->priv->bin_window, &style->base[gtk_widget_get_state (widget)]);
	
	/* 'No Image' message Layout */

	if (self->priv->layout != NULL)
		g_object_unref (self->priv->layout);

	self->priv->layout = gtk_widget_create_pango_layout (widget, NULL);
	pango_layout_set_wrap (self->priv->layout, PANGO_WRAP_WORD_CHAR);
	pango_layout_set_font_description (self->priv->layout, style->font_desc);
	pango_layout_set_alignment (self->priv->layout, PANGO_ALIGN_CENTER);
}


static void
gth_empty_list_unrealize (GtkWidget *widget)
{
	GthEmptyList *self;

	g_return_if_fail (GTH_IS_EMPTY_LIST (widget));

	self = (GthEmptyList*) widget;

	gdk_window_set_user_data (self->priv->bin_window, NULL);
	gdk_window_destroy (self->priv->bin_window);
	self->priv->bin_window = NULL;

	if (self->priv->layout != NULL) {
		g_object_unref (self->priv->layout);
		self->priv->layout = NULL;
	}

	(* GTK_WIDGET_CLASS (parent_class)->unrealize) (widget);
}


static void
gth_empty_list_map (GtkWidget *widget)
{
	gtk_widget_set_mapped (widget, TRUE);
	gdk_window_show (GTH_EMPTY_LIST (widget)->priv->bin_window);
	gdk_window_show (gtk_widget_get_window (widget));
}


static void
gth_empty_list_unmap (GtkWidget *widget)
{
	gtk_widget_set_mapped (widget, FALSE);
	gdk_window_hide (GTH_EMPTY_LIST (widget)->priv->bin_window);
	gdk_window_hide (gtk_widget_get_window (widget));
}


static void
gth_empty_list_size_allocate (GtkWidget     *widget,
			      GtkAllocation *allocation)
{
	GthEmptyList *self = (GthEmptyList*) widget;

	gtk_widget_set_allocation (widget, allocation);

	if (gtk_widget_get_realized (widget)) {
		gdk_window_move_resize (gtk_widget_get_window (widget),
					allocation->x,
					allocation->y,
					allocation->width,
					allocation->height);
		gdk_window_invalidate_rect (self->priv->bin_window, NULL, FALSE);
	}
}


static gboolean
gth_empty_list_expose_event (GtkWidget      *widget,
		             GdkEventExpose *event)
{
	GthEmptyList   *self = (GthEmptyList*) widget;
	GtkAllocation   allocation;
	PangoRectangle  bounds;
	GtkStyle       *style;
	cairo_t        *cr;
	
	if (event->window != self->priv->bin_window)
		return FALSE;

	if (self->priv->text == NULL)
		return TRUE;

	gtk_widget_get_allocation (widget, &allocation);
	pango_layout_set_width (self->priv->layout, allocation.width * PANGO_SCALE);
	pango_layout_set_text (self->priv->layout, self->priv->text, strlen (self->priv->text));
	pango_layout_get_pixel_extents (self->priv->layout, NULL, &bounds);

	cr = gdk_cairo_create (self->priv->bin_window);
	cairo_move_to (cr, 0, (allocation.height - bounds.height) / 2);
	pango_cairo_layout_path (cr, self->priv->layout);
	style = gtk_widget_get_style (widget);
	gdk_cairo_set_source_color (cr, &style->text[gtk_widget_get_state (widget)]);
	cairo_fill (cr);

	if (gtk_widget_has_focus (widget)) {
		gtk_paint_focus (style,
				 self->priv->bin_window,
				 gtk_widget_get_state (widget),
				 &event->area,
				 widget,
				 NULL,
				 1, 1,
				 allocation.width - 2,
				 allocation.height - 2);
	}

	cairo_destroy (cr);

	return FALSE;
}


static int
gth_empty_list_button_press (GtkWidget      *widget,
			     GdkEventButton *event)
{
	GthEmptyList *self = (GthEmptyList*) widget;

	if (event->window == self->priv->bin_window)
		if (! gtk_widget_has_focus (widget))
			gtk_widget_grab_focus (widget);
	
	return FALSE;
}


static void 
gth_empty_list_class_init (GthEmptyListClass *klass) 
{
	GObjectClass   *object_class;
	GtkWidgetClass *widget_class;
	
	parent_class = g_type_class_peek_parent (klass);	
	object_class = (GObjectClass*) (klass);
	widget_class = (GtkWidgetClass*) klass;
	
	object_class->set_property = gth_empty_list_set_property;
	object_class->get_property = gth_empty_list_get_property;
	object_class->finalize = gth_empty_list_finalize;
	
	widget_class->realize = gth_empty_list_realize;
	widget_class->unrealize = gth_empty_list_unrealize;
	widget_class->map = gth_empty_list_map;
	widget_class->unmap = gth_empty_list_unmap;
	widget_class->size_allocate = gth_empty_list_size_allocate;
	widget_class->expose_event = gth_empty_list_expose_event;
	widget_class->button_press_event = gth_empty_list_button_press;
	
	/* properties */
	
	g_object_class_install_property (object_class,
					 PROP_TEXT,
					 g_param_spec_string ("text",
                                                              "Text",
                                                              "The text to display",
                                                              NULL,
                                                              G_PARAM_READWRITE));   
}


static void 
gth_empty_list_instance_init (GthEmptyList *self) 
{
	gtk_widget_set_can_focus (GTK_WIDGET (self), TRUE);
	self->priv = g_new0 (GthEmptyListPrivate, 1);
}


GType 
gth_empty_list_get_type (void) 
{
	static GType type = 0;
	
	if (type == 0) {
		static const GTypeInfo g_define_type_info = { 
			sizeof (GthEmptyListClass), 
			NULL, 
			NULL, 
			(GClassInitFunc) gth_empty_list_class_init, 
			NULL, 
			NULL, 
			sizeof (GthEmptyList), 
			0, 
			(GInstanceInitFunc) gth_empty_list_instance_init, 
			NULL 
		};
		type = g_type_register_static (GTK_TYPE_VBOX, 
					       "GthEmptyList", 
					       &g_define_type_info, 
					       0);
	}
	
	return type;
}


GtkWidget *
gth_empty_list_new (const char *text) 
{
	return g_object_new (GTH_TYPE_EMPTY_LIST, "text", text, NULL);
}


void 
gth_empty_list_set_text (GthEmptyList *self,
			 const char   *text) 
{
	g_object_set (self, "text", text, NULL);
}
