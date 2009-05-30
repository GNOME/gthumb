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

#include "gthumb-info-bar.h"

#define X_PADDING 5
#define Y_PADDING 2

static GtkEventBoxClass *parent_class = NULL;

struct _GThumbInfoBarPrivate {
	gboolean     focused;
	char        *tooltip;

	GtkWidget   *hbox;
	GtkWidget   *label;
};


static void
gthumb_info_bar_destroy (GtkObject *object)
{
	GThumbInfoBar *info_bar = GTHUMB_INFO_BAR (object);

	if (info_bar->priv != NULL) {
		if (info_bar->priv->tooltip) {
			g_free (info_bar->priv->tooltip);
			info_bar->priv->tooltip = NULL;
		}

		g_free (info_bar->priv);
		info_bar->priv = NULL;
	}

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		GTK_OBJECT_CLASS (parent_class)->destroy (object);
}


static void
gthumb_info_bar_size_request (GtkWidget      *widget,
			      GtkRequisition *requisition)
{	
	if (GTK_WIDGET_CLASS (parent_class)->size_request)
		(* GTK_WIDGET_CLASS (parent_class)->size_request) (widget, requisition);
	requisition->width = 0;
}


static void
gthumb_info_bar_style_set (GtkWidget *widget,
			   GtkStyle  *previous_style)
{
        static int  in_style_set = 0;
	GtkStyle   *style = widget->style;
	GtkRcStyle *rc_style;

        if (in_style_set > 0)
                return;

        in_style_set++;

	rc_style = gtk_widget_get_modifier_style (widget);

	rc_style->color_flags[GTK_STATE_NORMAL] |= GTK_RC_TEXT;
	rc_style->text[GTK_STATE_NORMAL] = style->light[GTK_STATE_NORMAL];

	gtk_widget_modify_style (widget, rc_style);

        in_style_set--;

        (* GTK_WIDGET_CLASS (parent_class)->style_set) (widget, previous_style);
}


static void
gthumb_info_bar_class_init (GThumbInfoBarClass *class)
{
	GtkObjectClass *object_class;
	GtkWidgetClass *widget_class;

	parent_class = g_type_class_peek_parent (class);
	object_class = (GtkObjectClass*) class;
	widget_class = (GtkWidgetClass*) class;

	object_class->destroy       = gthumb_info_bar_destroy;
	widget_class->size_request  = gthumb_info_bar_size_request;
	widget_class->style_set     = gthumb_info_bar_style_set;
}


static void
gthumb_info_bar_init (GThumbInfoBar *info_bar)
{
	GThumbInfoBarPrivate *priv;

	GTK_WIDGET_UNSET_FLAGS (info_bar, GTK_CAN_FOCUS);

	priv = g_new0 (GThumbInfoBarPrivate, 1);
	info_bar->priv = priv;

	priv->hbox = gtk_hbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (info_bar), priv->hbox);

	priv->label = gtk_label_new ("");
	gtk_box_pack_start (GTK_BOX (priv->hbox), priv->label, TRUE, TRUE, 0);
	gtk_misc_set_alignment (GTK_MISC (priv->label), 0.0, 0.5);
	gtk_misc_set_padding   (GTK_MISC (priv->label), X_PADDING, Y_PADDING);
	gtk_label_set_line_wrap (GTK_LABEL (priv->label), FALSE);

	info_bar->priv->tooltip = NULL;
}


GType
gthumb_info_bar_get_type (void)
{
	static GType type = 0;

	if (! type) {
		static const GTypeInfo info =
		{
			sizeof (GThumbInfoBarClass),
			NULL,
			NULL,
			(GClassInitFunc) gthumb_info_bar_class_init,
			NULL,
			NULL,
			sizeof (GThumbInfoBar),
			0,
			(GInstanceInitFunc) gthumb_info_bar_init
		};

		type = g_type_register_static (GTK_TYPE_EVENT_BOX, 
					       "GThumbInfoBar",
					       &info,
					       0);
	}
	
	return type;
}


GtkWidget*
gthumb_info_bar_new ()
{
	GtkWidget *widget;
	widget = GTK_WIDGET (g_object_new (GTHUMB_TYPE_INFO_BAR, NULL));
	return widget;
}


void
gthumb_info_bar_set_focused (GThumbInfoBar *info_bar,
			     gboolean       focused)
{
	GtkWidget  *widget = GTK_WIDGET (info_bar);

	info_bar->priv->focused = focused;
	if (focused)
		gtk_widget_set_state (widget, GTK_STATE_SELECTED);
	else
		gtk_widget_set_state (widget, GTK_STATE_NORMAL);

	gtk_widget_queue_draw (info_bar->priv->hbox);
}


static void
set_tooltip (GThumbInfoBar *info_bar,
	     const gchar   *tooltip)
{
	if (info_bar->priv->tooltip) {
		g_free (info_bar->priv->tooltip);
		info_bar->priv->tooltip = NULL;
	}

	if (tooltip != NULL) {
		info_bar->priv->tooltip = g_strdup (tooltip);
                gtk_widget_set_tooltip_text (GTK_WIDGET (info_bar),
                                             info_bar->priv->tooltip);
                gtk_widget_set_has_tooltip (GTK_WIDGET (info_bar), TRUE);
	} else 
                gtk_widget_set_has_tooltip (GTK_WIDGET (info_bar), FALSE);
}


void
gthumb_info_bar_set_text (GThumbInfoBar *info_bar,
			  const char    *text,
			  const char    *tooltip)
{
	gtk_label_set_markup (GTK_LABEL (info_bar->priv->label), text);
	set_tooltip (info_bar, tooltip);
}
