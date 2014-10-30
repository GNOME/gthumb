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
#include <gtk/gtk.h>
#include "gth-main.h"
#include "gth-multipage.h"


#define GTH_MULTIPAGE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GTH_TYPE_MULTIPAGE, GthMultipagePrivate))


enum {
	CHANGED,
	LAST_SIGNAL
};


static guint gth_multipage_signals[LAST_SIGNAL] = { 0 };


struct _GthMultipagePrivate {
	GtkWidget *stack;
	GtkWidget *switcher;
	GList     *children;
	GList     *boxes;
};


G_DEFINE_TYPE (GthMultipage, gth_multipage, GTK_TYPE_BOX)


static void
gth_multipage_finalize (GObject *object)
{
	GthMultipage *multipage;

	multipage = GTH_MULTIPAGE (object);

	g_list_free (multipage->priv->children);
	g_list_free (multipage->priv->boxes);

	G_OBJECT_CLASS (gth_multipage_parent_class)->finalize (object);
}


static void
gth_multipage_class_init (GthMultipageClass *klass)
{
	GObjectClass *object_class;

	g_type_class_add_private (klass, sizeof (GthMultipagePrivate));

	object_class = (GObjectClass*) klass;
	object_class->finalize = gth_multipage_finalize;

	/* signals */

	gth_multipage_signals[CHANGED] =
		g_signal_new ("changed",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthMultipageClass, changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
}


static void
multipage_realize_cb (GtkWidget *widget,
		      gpointer   user_data)
{
	GthMultipage *multipage = user_data;
	GtkWidget    *orientable_parent;

	orientable_parent = gtk_widget_get_parent (widget);
	while ((orientable_parent != NULL ) && ! GTK_IS_ORIENTABLE (orientable_parent)) {
		orientable_parent = gtk_widget_get_parent (orientable_parent);
	}

	if (orientable_parent == NULL)
		return;

	switch (gtk_orientable_get_orientation (GTK_ORIENTABLE (orientable_parent))) {
	case GTK_ORIENTATION_HORIZONTAL:
		gtk_box_set_spacing (GTK_BOX (multipage), 0);
		gtk_widget_set_margin_top (multipage->priv->switcher, 4);
		gtk_widget_set_margin_bottom (multipage->priv->switcher, 4);
		break;

	case GTK_ORIENTATION_VERTICAL:
		gtk_box_set_spacing (GTK_BOX (multipage), 0);
		gtk_widget_set_margin_top (multipage->priv->switcher, 4);
		gtk_widget_set_margin_bottom (multipage->priv->switcher, 4);
		break;
	}
}


static void
visible_child_changed_cb (GObject    *playbin,
			  GParamSpec *pspec,
			  gpointer    user_data)
{
	GthMultipage *multipage = user_data;
	g_signal_emit (G_OBJECT (multipage), gth_multipage_signals[CHANGED], 0);
}


static void
gth_multipage_init (GthMultipage *multipage)
{
	GtkWidget       *switcher_box;
	GtkCellRenderer *renderer;

	multipage->priv = GTH_MULTIPAGE_GET_PRIVATE (multipage);
	multipage->priv->children = NULL;
	multipage->priv->boxes = NULL;

	gtk_orientable_set_orientation (GTK_ORIENTABLE (multipage), GTK_ORIENTATION_VERTICAL);

	g_signal_connect (multipage,
			  "realize",
			  G_CALLBACK (multipage_realize_cb),
			  multipage);

	/* stack */

	multipage->priv->stack = gtk_stack_new ();
	gtk_widget_show (multipage->priv->stack);

	g_signal_connect (multipage->priv->stack,
			  "notify::visible-child",
			  G_CALLBACK (visible_child_changed_cb),
			  multipage);

	/* switcher */

	multipage->priv->switcher = gtk_stack_switcher_new ();
	gtk_widget_show (multipage->priv->switcher);
	gtk_stack_switcher_set_stack (GTK_STACK_SWITCHER (multipage->priv->switcher), GTK_STACK (multipage->priv->stack));

	switcher_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_show (switcher_box);
	gtk_box_pack_start (GTK_BOX (switcher_box), multipage->priv->switcher, TRUE, FALSE, 0);

	gtk_box_pack_end (GTK_BOX (multipage), switcher_box, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (multipage), multipage->priv->stack, TRUE, TRUE, 0);
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
	GtkWidget   *box;
	GtkTreeIter  iter;

	multipage->priv->children = g_list_append (multipage->priv->children, child);

	box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_box_pack_start (GTK_BOX (box), GTK_WIDGET (child), TRUE, TRUE, 0);
	gtk_widget_show (GTK_WIDGET (child));
	gtk_widget_show (box);
	gtk_container_add_with_properties (GTK_CONTAINER (multipage->priv->stack),
					   box,
					   "name", gth_multipage_child_get_name (child),
					   "title", gth_multipage_child_get_name (child),
					   "icon-name", gth_multipage_child_get_icon (child),
					   NULL);

	multipage->priv->boxes = g_list_append (multipage->priv->boxes, box);
}


GList *
gth_multipage_get_children (GthMultipage *multipage)
{
	return multipage->priv->children;
}


void
gth_multipage_set_current (GthMultipage *multipage,
			   int           index_)
{
	GtkWidget *child;

	child = g_list_nth_data (multipage->priv->boxes, index_);
	if (child != NULL)
		gtk_stack_set_visible_child (GTK_STACK (multipage->priv->stack), child);
}


int
gth_multipage_get_current (GthMultipage *multipage)
{
	GtkWidget *child;

	child = gtk_stack_get_visible_child (GTK_STACK (multipage->priv->stack));
	return g_list_index (multipage->priv->boxes, child);
}


/* -- gth_multipage_child -- */


G_DEFINE_INTERFACE (GthMultipageChild, gth_multipage_child, 0)


static void
gth_multipage_child_default_init (GthMultipageChildInterface *iface)
{
	/* void */
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
