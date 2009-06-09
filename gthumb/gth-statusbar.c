/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2009 Free Software Foundation, Inc.
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
#include <glib/gi18n.h>
#include "gth-statusbar.h"


#define GTH_STATUSBAR_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GTH_TYPE_STATUSBAR, GthStatusbarPrivate))


static gpointer gth_statusbar_parent_class = NULL;


struct _GthStatusbarPrivate {
	guint      list_info_cid;
	GtkWidget *primary_text;
	GtkWidget *primary_text_frame;
	GtkWidget *secondary_text;
	GtkWidget *secondary_text_frame;
};


static void
gth_statusbar_class_init (GthStatusbarClass *klass)
{
	gth_statusbar_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GthStatusbarPrivate));
}


static void
gth_statusbar_init (GthStatusbar *statusbar)
{
	statusbar->priv = GTH_STATUSBAR_GET_PRIVATE (statusbar);
	statusbar->priv->list_info_cid = gtk_statusbar_get_context_id (GTK_STATUSBAR (statusbar), "gth_list_info");

	/* Secondary text */

	statusbar->priv->secondary_text = gtk_label_new (NULL);
	gtk_widget_show (statusbar->priv->secondary_text);

	statusbar->priv->secondary_text_frame = gtk_frame_new (NULL);
	gtk_widget_show (statusbar->priv->secondary_text_frame);
	gtk_frame_set_shadow_type (GTK_FRAME (statusbar->priv->secondary_text_frame), GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (statusbar->priv->secondary_text_frame), statusbar->priv->secondary_text);
	gtk_box_pack_start (GTK_BOX (statusbar), statusbar->priv->secondary_text_frame, FALSE, FALSE, 0);

	/* Primary text */

	statusbar->priv->primary_text = gtk_label_new (NULL);
	gtk_widget_show (statusbar->priv->primary_text);

	statusbar->priv->primary_text_frame = gtk_frame_new (NULL);
	gtk_widget_show (statusbar->priv->primary_text_frame);

	gtk_frame_set_shadow_type (GTK_FRAME (statusbar->priv->primary_text_frame), GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (statusbar->priv->primary_text_frame), statusbar->priv->primary_text);
	gtk_box_pack_start (GTK_BOX (statusbar), statusbar->priv->primary_text_frame, FALSE, FALSE, 0);
}


GType
gth_statusbar_get_type (void)
{
	static GType type = 0;

	if (type == 0) {
		static const GTypeInfo g_define_type_info = {
			sizeof (GthStatusbarClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) gth_statusbar_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (GthStatusbar),
			0,
			(GInstanceInitFunc) gth_statusbar_init,
			NULL
		};
		type = g_type_register_static (GTK_TYPE_STATUSBAR,
					       "GthStatusbar",
					       &g_define_type_info,
					       0);
	}

	return type;
}


GtkWidget *
gth_statusbar_new (void)
{
	return g_object_new (GTH_TYPE_STATUSBAR, NULL);
}


void
gth_statusbar_set_list_info (GthStatusbar *statusbar,
			     const char   *text)
{
	gtk_statusbar_pop (GTK_STATUSBAR (statusbar), statusbar->priv->list_info_cid);
	gtk_statusbar_push (GTK_STATUSBAR (statusbar), statusbar->priv->list_info_cid, text);
}


void
gth_statusbar_set_primary_text (GthStatusbar *statusbar,
				const char   *text)
{
	if (text != NULL) {
		gtk_label_set_text (GTK_LABEL (statusbar->priv->primary_text), text);
		gtk_widget_show (statusbar->priv->primary_text_frame);
	}
	else
		gtk_widget_hide (statusbar->priv->primary_text_frame);
}


void
gth_statusbar_set_secondary_text (GthStatusbar *statusbar,
				  const char   *text)
{
	if (text != NULL) {
		gtk_label_set_text (GTK_LABEL (statusbar->priv->secondary_text), text);
		gtk_widget_show (statusbar->priv->secondary_text_frame);
	}
	else
		gtk_widget_hide (statusbar->priv->secondary_text_frame);
}
