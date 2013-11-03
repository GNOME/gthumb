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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <glib/gi18n.h>
#include "gth-statusbar.h"


struct _GthStatusbarPrivate {
	GtkWidget *list_info;
	GtkWidget *primary_text;
	GtkWidget *secondary_text;
	GtkWidget *action_area;
};


G_DEFINE_TYPE (GthStatusbar, gth_statusbar, GTK_TYPE_BOX)


static void
gth_statusbar_class_init (GthStatusbarClass *klass)
{
	g_type_class_add_private (klass, sizeof (GthStatusbarPrivate));
}


static void
gth_statusbar_init (GthStatusbar *statusbar)
{
	statusbar->priv = G_TYPE_INSTANCE_GET_PRIVATE (statusbar, GTH_TYPE_STATUSBAR, GthStatusbarPrivate);

	gtk_box_set_spacing (GTK_BOX (statusbar), 6);
	gtk_widget_set_margin_top (GTK_WIDGET (statusbar), 3);
	gtk_widget_set_margin_right (GTK_WIDGET (statusbar), 6);
	gtk_widget_set_margin_bottom (GTK_WIDGET (statusbar), 3);
	gtk_widget_set_margin_left (GTK_WIDGET (statusbar), 6);

	/* List info */

	statusbar->priv->list_info = gtk_label_new (NULL);
	gtk_label_set_ellipsize (GTK_LABEL (statusbar->priv->list_info), PANGO_ELLIPSIZE_END);
	gtk_box_pack_start (GTK_BOX (statusbar), statusbar->priv->list_info, FALSE, FALSE, 0);

	/* Primary text */

	statusbar->priv->primary_text = gtk_label_new (NULL);
	gtk_label_set_ellipsize (GTK_LABEL (statusbar->priv->primary_text), PANGO_ELLIPSIZE_END);
	gtk_box_pack_start (GTK_BOX (statusbar), statusbar->priv->primary_text, FALSE, FALSE, 0);

	/* Secondary text */

	statusbar->priv->secondary_text = gtk_label_new (NULL);
	gtk_label_set_ellipsize (GTK_LABEL (statusbar->priv->secondary_text), PANGO_ELLIPSIZE_END);
	gtk_box_pack_start (GTK_BOX (statusbar), statusbar->priv->secondary_text, FALSE, FALSE, 12);

	/* Action area */

	statusbar->priv->action_area = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_widget_show (statusbar->priv->action_area);
	gtk_box_pack_end (GTK_BOX (statusbar), statusbar->priv->action_area, FALSE, FALSE, 0);

	gth_statusbar_show_section (statusbar, GTH_STATUSBAR_SECTION_FILE_LIST);
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
	gtk_label_set_text (GTK_LABEL (statusbar->priv->list_info), text);
}


void
gth_statusbar_set_primary_text (GthStatusbar *statusbar,
				const char   *text)
{
	gtk_label_set_text (GTK_LABEL (statusbar->priv->primary_text), text);
}


void
gth_statusbar_set_secondary_text (GthStatusbar *statusbar,
				  const char   *text)
{
	gtk_label_set_text (GTK_LABEL (statusbar->priv->secondary_text), text);
}


void
gth_statusbar_show_section (GthStatusbar		*statusbar,
			    GthStatusbarSection		 section)
{
	gtk_widget_set_visible (statusbar->priv->list_info, section == GTH_STATUSBAR_SECTION_FILE_LIST);
	gtk_widget_set_visible (statusbar->priv->primary_text, section == GTH_STATUSBAR_SECTION_FILE);
	gtk_widget_set_visible (statusbar->priv->secondary_text, section == GTH_STATUSBAR_SECTION_FILE);
}


GtkWidget *
gth_statubar_get_action_area (GthStatusbar *statusbar)
{
	return statusbar->priv->action_area;
}
