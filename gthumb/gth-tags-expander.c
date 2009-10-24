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
#include <glib/gi18n.h>
#include "gth-main.h"
#include "gth-tags-entry.h"
#include "gth-tags-expander.h"
#include "gth-tags-file.h"
#include "glib-utils.h"


enum {
	USED_COLUMN,
	NAME_COLUMN,
	N_COLUMNS
};


struct _GthTagsExpanderPrivate {
	GtkEntry            *entry;
	char               **tags;
	GtkWidget           *tree_view;
	GtkListStore        *store;
	gulong               monitor_event;
};


static gpointer parent_class = NULL;


static void
gth_tags_expander_finalize (GObject *obj)
{
	GthTagsExpander *self;

	self = GTH_TAGS_EXPANDER (obj);

	g_signal_handler_disconnect (gth_main_get_default_monitor (), self->priv->monitor_event);
	g_strfreev (self->priv->tags);

	G_OBJECT_CLASS (parent_class)->finalize (obj);
}


static void
gth_tags_expander_class_init (GthTagsExpanderClass *klass)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GthTagsExpanderPrivate));

	object_class = (GObjectClass*) (klass);
	object_class->finalize = gth_tags_expander_finalize;
}


static void
update_tag_list (GthTagsExpander *self)
{
	GthTagsFile *tags;
	int          i;

	tags = gth_main_get_default_tag_file ();

	g_strfreev (self->priv->tags);
	self->priv->tags = g_strdupv (gth_tags_file_get_tags (tags));

	for (i = 0; self->priv->tags[i] != NULL; i++) {
		GtkTreeIter iter;

		gtk_list_store_append (self->priv->store, &iter);
		gtk_list_store_set (self->priv->store, &iter,
				    NAME_COLUMN, self->priv->tags[i],
				    -1);
	}

	gtk_list_store_clear (self->priv->store);
	for (i = 0; self->priv->tags[i] != NULL; i++) {
		GtkTreeIter iter;

		gtk_list_store_append (self->priv->store, &iter);
		gtk_list_store_set (self->priv->store, &iter,
				    NAME_COLUMN, self->priv->tags[i],
				    -1);
	}
}


static void
tags_changed_cb (GthMonitor      *monitor,
		 GthTagsExpander *self)
{
	update_tag_list (self);
}


static void
update_list_view_from_entry (GthTagsExpander *self)
{
	char        **tags;
	GtkTreeIter   iter;

	tags = gth_tags_entry_get_tags (GTH_TAGS_ENTRY (self->priv->entry), FALSE);
	if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (self->priv->store), &iter))
		do {
			char     *name;
			gboolean  used = FALSE;
			int       i;

			gtk_tree_model_get (GTK_TREE_MODEL (self->priv->store), &iter,
					    NAME_COLUMN, &name,
					    -1);

			for (i = 0; ! used && (tags[i] != NULL); i++)
				if (g_utf8_collate (name, tags[i]) == 0)
					used = TRUE;

			gtk_list_store_set (self->priv->store, &iter,
					    USED_COLUMN, used,
					    -1);

			g_free (name);
		}
		while (gtk_tree_model_iter_next (GTK_TREE_MODEL (self->priv->store), &iter));

	g_strfreev (tags);
}



static void
update_entry_from_list_view (GthTagsExpander *self)
{
	GtkTreeIter   iter;
	GList        *name_list;
	char        **tags;
	GList        *scan;
	int           i;

	if (! gtk_tree_model_get_iter_first (GTK_TREE_MODEL (self->priv->store), &iter))
		return;

	name_list = NULL;
	do {
		char     *name;
		gboolean  used;

		gtk_tree_model_get (GTK_TREE_MODEL (self->priv->store), &iter,
				    NAME_COLUMN, &name,
				    USED_COLUMN, &used,
				    -1);

		if (used)
			name_list = g_list_prepend (name_list, name);
		else
			g_free (name);
	}
	while (gtk_tree_model_iter_next (GTK_TREE_MODEL (self->priv->store), &iter));

	if (name_list == NULL)
		return;

	name_list = g_list_reverse (name_list);
	tags = g_new (char *, g_list_length (name_list) + 1);
	for (i = 0, scan = name_list; scan; scan = scan->next)
		tags[i++] = scan->data;
	tags[i] = NULL;
	gth_tags_entry_set_tags (GTH_TAGS_ENTRY (self->priv->entry), tags);

	g_free (tags);
	_g_string_list_free (name_list);
}


static void
cell_renderer_toggle_toggled_cb (GtkCellRendererToggle *cell_renderer,
				 char                  *path,
                                 gpointer               user_data)
{
	GthTagsExpander *self = user_data;
	GtkTreePath     *tpath;
	GtkTreeModel    *tree_model;
	GtkTreeIter      iter;

	tpath = gtk_tree_path_new_from_string (path);
	if (tpath == NULL)
		return;

	tree_model = gtk_tree_view_get_model (GTK_TREE_VIEW (self->priv->tree_view));
	if (gtk_tree_model_get_iter (tree_model, &iter, tpath)) {
		gboolean used;

		gtk_tree_model_get (tree_model, &iter,
				    USED_COLUMN, &used,
				    -1);
		gtk_list_store_set (GTK_LIST_STORE (tree_model), &iter,
				    USED_COLUMN, ! used,
				    -1);
	}

	update_entry_from_list_view (self);

	gtk_tree_path_free (tpath);
}


static void
tag_list_map_cb (GtkWidget       *widget,
		 GthTagsExpander *self)
{
	GtkWidget   *toplevel;
        GdkGeometry  geometry;

return; /* FIXME */

        toplevel = gtk_widget_get_toplevel (widget);
        if (! GTK_WIDGET_TOPLEVEL (toplevel))
        	return;

        geometry.min_width = -1;
        geometry.min_height = 230;
        gtk_window_set_geometry_hints (GTK_WINDOW (toplevel),
				       widget,
				       &geometry,
                                       GDK_HINT_MIN_SIZE);
}


static void
tag_list_unmap_cb (GtkWidget       *widget,
	           GthTagsExpander *self)
{
	GtkWidget   *toplevel;
        GdkGeometry  geometry;

        toplevel = gtk_widget_get_toplevel (widget);
        if (! GTK_WIDGET_TOPLEVEL (toplevel))
        	return;

        geometry.max_height = -1;
        geometry.max_width = G_MAXINT;
        gtk_window_set_geometry_hints (GTK_WINDOW (toplevel),
				       toplevel,
                                       &geometry,
                                       GDK_HINT_MAX_SIZE);
}


static void
gth_tags_expander_instance_init (GthTagsExpander *self)
{
	GtkTreeViewColumn *column;
	GtkCellRenderer   *renderer;
	GtkWidget         *scrolled_window;

	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_TAGS_EXPANDER, GthTagsExpanderPrivate);

	/* the treeview */

	self->priv->store = gtk_list_store_new (N_COLUMNS, G_TYPE_BOOLEAN, G_TYPE_STRING);
	self->priv->tree_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (self->priv->store));
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (self->priv->tree_view), FALSE);
	g_object_unref (self->priv->store);

	/* the checkbox column */

	column = gtk_tree_view_column_new ();
	renderer = gtk_cell_renderer_toggle_new ();
	g_signal_connect (renderer,
			  "toggled",
			  G_CALLBACK (cell_renderer_toggle_toggled_cb),
			  self);
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_attributes (column, renderer,
					     "active", USED_COLUMN,
					     NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (self->priv->tree_view), column);

	/* the name column. */

	column = gtk_tree_view_column_new ();
	renderer = gtk_cell_renderer_text_new ();
        gtk_tree_view_column_pack_start (column, renderer, TRUE);
        gtk_tree_view_column_set_attributes (column, renderer,
                                             "text", NAME_COLUMN,
                                             NULL);
        gtk_tree_view_column_set_expand (column, TRUE);
	gtk_tree_view_append_column (GTK_TREE_VIEW (self->priv->tree_view), column);

	self->priv->monitor_event = g_signal_connect (gth_main_get_default_monitor (),
						      "tags-changed",
						      G_CALLBACK (tags_changed_cb),
						      self);
	gtk_widget_show (self->priv->tree_view);

	/* the scrolled window */

	scrolled_window = g_object_new (GTK_TYPE_SCROLLED_WINDOW,
					"hadjustment", NULL,
					"vadjustment", NULL,
					"hscrollbar_policy", GTK_POLICY_AUTOMATIC,
					"vscrollbar_policy", GTK_POLICY_AUTOMATIC,
					"shadow_type", GTK_SHADOW_IN,
					NULL);
	gtk_widget_set_size_request (scrolled_window, -1, 230);
	gtk_container_add (GTK_CONTAINER (scrolled_window), self->priv->tree_view);
	g_signal_connect (scrolled_window, "map", G_CALLBACK (tag_list_map_cb), self);
	g_signal_connect (scrolled_window, "unmap", G_CALLBACK (tag_list_unmap_cb), self);
	gtk_widget_show (scrolled_window);
	gtk_container_add (GTK_CONTAINER (self), scrolled_window);
}


GType
gth_tags_expander_get_type (void)
{
	static GType type = 0;

	if (type == 0) {
		static const GTypeInfo g_define_type_info = {
			sizeof (GthTagsExpanderClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_tags_expander_class_init,
			NULL,
			NULL,
			sizeof (GthTagsExpander),
			0,
			(GInstanceInitFunc) gth_tags_expander_instance_init,
			NULL
		};
		type = g_type_register_static (GTK_TYPE_EXPANDER,
					       "GthTagsExpander",
					       &g_define_type_info,
					       0);
	}

	return type;
}


GtkWidget *
gth_tags_expander_new (GtkEntry *entry)
{
	GthTagsExpander *self;

	self = g_object_new (GTH_TYPE_TAGS_EXPANDER, "label", _("_Show all the tags"), "use-underline", TRUE, NULL);
	self->priv->entry = entry;
	update_tag_list (self);

	g_signal_connect_swapped (self->priv->entry,
				  "notify::text",
				  G_CALLBACK (update_list_view_from_entry),
				  self);

	return (GtkWidget *) self;
}
