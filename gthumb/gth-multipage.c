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
#include <gtk/gtk.h>
#include "gth-main.h"
#include "gth-multipage.h"


#define GTH_MULTIPAGE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GTH_TYPE_MULTIPAGE, GthMultipagePrivate))


enum {
	ICON_COLUMN,
	NAME_COLUMN,
	N_COLUMNS
};


static gpointer parent_class = NULL;


struct _GthMultipagePrivate {
	GtkListStore *model;
	GtkWidget    *combobox;
	GtkWidget    *notebook;
};


static void
gth_multipage_class_init (GthMultipageClass *klass)
{
	parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GthMultipagePrivate));
}


static void
combobox_changed_cb (GtkComboBox *widget,
		     gpointer     user_data)
{
	GthMultipage *multipage = user_data;

	gtk_notebook_set_current_page (GTK_NOTEBOOK (multipage->priv->notebook), gtk_combo_box_get_active (GTK_COMBO_BOX (multipage->priv->combobox)));
}


static void
gth_multipage_init (GthMultipage *multipage)
{
	GtkCellRenderer *renderer;

	multipage->priv = GTH_MULTIPAGE_GET_PRIVATE (multipage);

	gtk_box_set_spacing (GTK_BOX (multipage), 6);

	multipage->priv->model = gtk_list_store_new (N_COLUMNS,
						     G_TYPE_STRING,
						     G_TYPE_STRING);
	multipage->priv->combobox = gtk_combo_box_new_with_model (GTK_TREE_MODEL (multipage->priv->model));
	gtk_widget_show (multipage->priv->combobox);
	gtk_box_pack_start (GTK_BOX (multipage), multipage->priv->combobox, FALSE, FALSE, 0);
	g_object_unref (multipage->priv->model);

	g_signal_connect (multipage->priv->combobox,
			  "changed",
			  G_CALLBACK (combobox_changed_cb),
			  multipage);

	/* icon renderer */

	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (multipage->priv->combobox),
				    renderer,
				    FALSE);
	gtk_cell_layout_set_attributes  (GTK_CELL_LAYOUT (multipage->priv->combobox),
					 renderer,
					 "icon-name", ICON_COLUMN,
					 NULL);

	/* name renderer */

	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (multipage->priv->combobox),
				    renderer,
				    TRUE);
	gtk_cell_layout_set_attributes  (GTK_CELL_LAYOUT (multipage->priv->combobox),
					 renderer,
					 "text", NAME_COLUMN,
					 NULL);

	/* notebook */

	multipage->priv->notebook = gtk_notebook_new ();
	gtk_notebook_set_show_tabs (GTK_NOTEBOOK (multipage->priv->notebook), FALSE);
	gtk_notebook_set_show_border (GTK_NOTEBOOK (multipage->priv->notebook), FALSE);
	gtk_widget_show (multipage->priv->notebook);
	gtk_box_pack_start (GTK_BOX (multipage), multipage->priv->notebook, TRUE, TRUE, 0);
}


GType
gth_multipage_get_type (void)
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (GthMultipageClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_multipage_class_init,
			NULL,
			NULL,
			sizeof (GthMultipage),
			0,
			(GInstanceInitFunc) gth_multipage_init
		};

		type = g_type_register_static (GTK_TYPE_VBOX,
					       "GthMultipage",
					       &type_info,
					       0);
	}

	return type;
}


GtkWidget *
gth_multipage_new (void)
{
	return (GtkWidget *) g_object_new (GTH_TYPE_MULTIPAGE, NULL);
}


void
gth_multipage_add_child (GthMultipage      *multipage,
			 GthMultipageChild *child)
{
	GtkTreeIter iter;

	gtk_widget_show (GTK_WIDGET (child));
	gtk_notebook_append_page (GTK_NOTEBOOK (multipage->priv->notebook), GTK_WIDGET (child), NULL);

	gtk_list_store_append (GTK_LIST_STORE (multipage->priv->model), &iter);
	gtk_list_store_set (GTK_LIST_STORE (multipage->priv->model), &iter,
			    NAME_COLUMN, gth_multipage_child_get_name (child),
			    ICON_COLUMN, gth_multipage_child_get_icon (child),
			    -1);
}


GList *
gth_multipage_get_children (GthMultipage *multipage)
{
	return gtk_container_get_children (GTK_CONTAINER (multipage->priv->notebook));
}


void
gth_multipage_set_current (GthMultipage *multipage,
			   int           index_)
{
	gtk_combo_box_set_active (GTK_COMBO_BOX (multipage->priv->combobox), index_);
}


int
gth_multipage_get_current (GthMultipage *multipage)
{
	return gtk_combo_box_get_active (GTK_COMBO_BOX (multipage->priv->combobox));
}


/* -- gth_multipage_child -- */


GType
gth_multipage_child_get_type (void) {
	static GType gth_multipage_child_type_id = 0;
	if (gth_multipage_child_type_id == 0) {
		static const GTypeInfo g_define_type_info = {
			sizeof (GthMultipageChildIface),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) NULL,
			(GClassFinalizeFunc) NULL,
			NULL,
			0,
			0,
			(GInstanceInitFunc) NULL,
			NULL
		};
		gth_multipage_child_type_id = g_type_register_static (G_TYPE_INTERFACE, "GthMultipageChild", &g_define_type_info, 0);
	}
	return gth_multipage_child_type_id;
}


const char *
gth_multipage_child_get_name (GthMultipageChild *self)
{
	return GTH_MULTIPAGE_CHILD_GET_INTERFACE (self)->get_name (self);
}


const char *
gth_multipage_child_get_icon (GthMultipageChild *self)
{
	return GTH_MULTIPAGE_CHILD_GET_INTERFACE (self)->get_icon (self);
}
