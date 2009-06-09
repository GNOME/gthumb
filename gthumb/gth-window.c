/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2005-2008 Free Software Foundation, Inc.
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
#include "gth-window.h"
#include "gtk-utils.h"


#define GTH_WINDOW_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GTH_TYPE_WINDOW, GthWindowPrivate))

enum  {
	GTH_WINDOW_DUMMY_PROPERTY,
	GTH_WINDOW_N_PAGES
};


static GtkWindowClass *parent_class = NULL;
static GList *window_list = NULL;


struct _GthWindowPrivate {
	int         n_pages;
	int         current_page;
	GtkWidget  *table;
	GtkWidget  *notebook;
	GtkWidget **toolbar;
	GtkWidget **content;
};


static void
gth_window_set_n_pages (GthWindow *self,
			int        n_pages)
{
	int i;

	self->priv->n_pages = n_pages;

	self->priv->table = gtk_table_new (4, 1, FALSE);
	gtk_widget_show (self->priv->table);
	gtk_container_add (GTK_CONTAINER (self), self->priv->table);

	self->priv->notebook = gtk_notebook_new ();
	gtk_notebook_set_show_tabs (GTK_NOTEBOOK (self->priv->notebook), FALSE);
	gtk_notebook_set_show_border (GTK_NOTEBOOK (self->priv->notebook), FALSE);
	gtk_widget_show (self->priv->notebook);
	gtk_table_attach (GTK_TABLE (self->priv->table),
			  self->priv->notebook,
			  0, 1,
			  2, 3,
			  GTK_EXPAND | GTK_FILL,
			  GTK_EXPAND | GTK_FILL,
			  0, 0);

	self->priv->toolbar = g_new0 (GtkWidget *, n_pages);
	self->priv->content = g_new0 (GtkWidget *, n_pages);

	for (i = 0; i < n_pages; i++) {
		GtkWidget *page;

		page = gtk_vbox_new (FALSE, 0);
		gtk_widget_show (page);
		gtk_notebook_append_page (GTK_NOTEBOOK (self->priv->notebook),
					  page,
					  NULL);

		self->priv->toolbar[i] = gtk_hbox_new (FALSE, 0);
		gtk_widget_show (self->priv->toolbar[i]);
		gtk_box_pack_start (GTK_BOX (page), self->priv->toolbar[i], FALSE, FALSE, 0);

		self->priv->content[i] = gtk_hbox_new (FALSE, 0);
		gtk_widget_show (self->priv->content[i]);
		gtk_box_pack_start (GTK_BOX (page), self->priv->content[i], TRUE, TRUE, 0);
	}
}


static void
gth_window_set_property (GObject      *object,
			 guint         property_id,
			 const GValue *value,
			 GParamSpec   *pspec)
{
	GthWindow *self;

	self = GTH_WINDOW (object);

	switch (property_id) {
	case GTH_WINDOW_N_PAGES:
		gth_window_set_n_pages (self, g_value_get_int (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}


static void
gth_window_finalize (GObject *object)
{
	GthWindow *window = GTH_WINDOW (object);

	g_free (window->priv->toolbar);
	g_free (window->priv->content);

	window_list = g_list_remove (window_list, window);

	G_OBJECT_CLASS (parent_class)->finalize (object);

	if (window_list == NULL)
		gtk_main_quit ();
}


static gboolean
gth_window_delete_event (GtkWidget   *widget,
			 GdkEventAny *event)
{
	gth_window_close ((GthWindow*) widget);
	return TRUE;
}


static void
gth_window_real_close (GthWindow *window)
{
	/* virtual */
}


static void
gth_window_real_set_current_page (GthWindow *window,
				  int        page)
{
	if (window->priv->current_page == page)
		return;
	window->priv->current_page = page;
	gtk_notebook_set_current_page (GTK_NOTEBOOK (window->priv->notebook), page);
}


static void
gth_window_class_init (GthWindowClass *klass)
{
	GObjectClass   *gobject_class;
	GtkWidgetClass *widget_class;

	parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GthWindowPrivate));

	gobject_class = (GObjectClass*) klass;
	gobject_class->set_property = gth_window_set_property;
	gobject_class->finalize = gth_window_finalize;

	widget_class = (GtkWidgetClass*) klass;
	widget_class->delete_event = gth_window_delete_event;

	klass->close = gth_window_real_close;
	klass->set_current_page = gth_window_real_set_current_page;

	g_object_class_install_property (G_OBJECT_CLASS (klass),
					 GTH_WINDOW_N_PAGES,
					 g_param_spec_int ("n-pages",
							   "n-pages",
							   "n-pages",
							   0,
							   G_MAXINT,
							   1,
							   G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
}


static void
gth_window_init (GthWindow *window)
{
	window->priv = GTH_WINDOW_GET_PRIVATE (window);
	window->priv->table = NULL;
	window->priv->content = NULL;
	window->priv->n_pages = 0;
	window->priv->current_page = -1;

	window_list = g_list_prepend (window_list, window);
}


GType
gth_window_get_type (void)
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (GthWindowClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_window_class_init,
			NULL,
			NULL,
			sizeof (GthWindow),
			0,
			(GInstanceInitFunc) gth_window_init
		};

		type = g_type_register_static (GTK_TYPE_WINDOW,
					       "GthWindow",
					       &type_info,
					       0);
	}

	return type;
}


void
gth_window_close (GthWindow *window)
{
	GTH_WINDOW_GET_CLASS (window)->close (window);
}


void
gth_window_attach (GthWindow     *window,
		   GtkWidget     *child,
		   GthWindowArea  area)
{
	int position;

	g_return_if_fail (window != NULL);
	g_return_if_fail (GTH_IS_WINDOW (window));
	g_return_if_fail (child != NULL);
	g_return_if_fail (GTK_IS_WIDGET (child));

	switch (area) {
	case GTH_WINDOW_MENUBAR:
		position = 0;
		break;
	case GTH_WINDOW_TOOLBAR:
		position = 1;
		break;
	case GTH_WINDOW_STATUSBAR:
		position = 3;
		break;
	default:
		return;
	}

	gtk_table_attach (GTK_TABLE (window->priv->table),
			  child,
			  0, 1,
			  position, position + 1,
			  GTK_EXPAND | GTK_FILL,
			  GTK_FILL,
			  0, 0);
}


void
gth_window_attach_toolbar (GthWindow *window,
			   int        page,
			   GtkWidget *child)
{
	g_return_if_fail (window != NULL);
	g_return_if_fail (GTH_IS_WINDOW (window));
	g_return_if_fail (page >= 0 && page < window->priv->n_pages);
	g_return_if_fail (child != NULL);
	g_return_if_fail (GTK_IS_WIDGET (child));

	_gtk_container_remove_children (GTK_CONTAINER (window->priv->toolbar[page]), NULL, NULL);
	gtk_container_add (GTK_CONTAINER (window->priv->toolbar[page]), child);
}


void
gth_window_attach_content (GthWindow *window,
			   int        page,
			   GtkWidget *child)
{
	g_return_if_fail (window != NULL);
	g_return_if_fail (GTH_IS_WINDOW (window));
	g_return_if_fail (page >= 0 && page < window->priv->n_pages);
	g_return_if_fail (child != NULL);
	g_return_if_fail (GTK_IS_WIDGET (child));

	_gtk_container_remove_children (GTK_CONTAINER (window->priv->content[page]), NULL, NULL);
	gtk_container_add (GTK_CONTAINER (window->priv->content[page]), child);
}


void
gth_window_set_current_page (GthWindow *window,
			     int        page)
{
	GTH_WINDOW_GET_CLASS (window)->set_current_page (window, page);
}


int
gth_window_get_current_page (GthWindow *window)
{
	return window->priv->current_page;
}


int
gth_window_get_n_windows (void)
{
	return g_list_length (window_list);
}


GList *
gth_window_get_window_list (void)
{
	return window_list;
}


GtkWidget *
gth_window_get_current_window (void)
{
	if ((window_list == NULL) || (g_list_length (window_list) > 1))
		return NULL;
	else
		return (GtkWidget *) window_list->data;
}
