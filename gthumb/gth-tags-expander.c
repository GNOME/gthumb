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
	SEPARATOR_COLUMN,
	NAME_COLUMN,
	N_COLUMNS
};


struct _GthTagsExpanderPrivate {
	GtkEntry      *entry;
	char         **tags;
	char         **last_used;
	GtkWidget     *tree_view;
	GtkListStore  *store;
	gulong         monitor_event;
};


typedef struct {
	char     *name;
	gboolean  used;
	gboolean  suggested;
} TagData;


static gpointer parent_class = NULL;


static void
gth_tags_expander_finalize (GObject *obj)
{
	GthTagsExpander *self;

	self = GTH_TAGS_EXPANDER (obj);

	g_signal_handler_disconnect (gth_main_get_default_monitor (), self->priv->monitor_event);
	g_strfreev (self->priv->last_used);
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


static int
sort_tag_data (gconstpointer a,
	       gconstpointer b,
	       gpointer      user_data)
{
	TagData *tag_data_a = * (TagData **) a;
	TagData *tag_data_b = * (TagData **) b;

	if (tag_data_a->used && tag_data_b->used)
		return g_utf8_collate (tag_data_a->name, tag_data_b->name);
	else if (tag_data_a->used || tag_data_b->used)
		return tag_data_a->used ? -1 : 1;
	else if (tag_data_a->suggested && tag_data_b->suggested)
		return g_utf8_collate (tag_data_a->name, tag_data_b->name);
	else if (tag_data_a->suggested || tag_data_b->suggested)
		return tag_data_a->suggested ? -1 : 1;
	else
		return g_utf8_collate (tag_data_a->name, tag_data_b->name);
}


static void
update_list_view_from_entry (GthTagsExpander *self)
{
	char        **all_tags;
	char        **used_tags;
	TagData     **tag_data;
	int           i;
	GtkTreeIter   iter;
	gboolean      separator_required;

	all_tags = g_strdupv (gth_tags_file_get_tags (gth_main_get_default_tag_file ()));
	used_tags = gth_tags_entry_get_tags (GTH_TAGS_ENTRY (self->priv->entry), FALSE);

	tag_data = g_new (TagData *, g_strv_length (all_tags) + 1);
	for (i = 0; all_tags[i] != NULL; i++) {
		int j;

		tag_data[i] = g_new0 (TagData, 1);
		tag_data[i]->name = g_strdup (all_tags[i]);
		tag_data[i]->suggested = FALSE;
		tag_data[i]->used = FALSE;
		for (j = 0; ! tag_data[i]->used && (used_tags[j] != NULL); j++)
			if (g_utf8_collate (tag_data[i]->name, used_tags[j]) == 0)
				tag_data[i]->used = TRUE;

		if (! tag_data[i]->used)
			for (j = 0; ! tag_data[i]->suggested && (self->priv->last_used[j] != NULL); j++)
				if (g_utf8_collate (tag_data[i]->name, self->priv->last_used[j]) == 0)
					tag_data[i]->suggested = TRUE;
	}
	tag_data[i] = NULL;

	g_qsort_with_data (tag_data,
			   g_strv_length (all_tags),
			   sizeof (TagData *),
			   sort_tag_data,
			   NULL);

	gtk_list_store_clear (self->priv->store);

	/* used */

	separator_required = FALSE;
	for (i = 0; tag_data[i] != NULL; i++) {
		GtkTreeIter iter;

		if (! tag_data[i]->used)
			continue;

		separator_required = TRUE;

		gtk_list_store_append (self->priv->store, &iter);
		gtk_list_store_set (self->priv->store, &iter,
				    USED_COLUMN, TRUE,
				    SEPARATOR_COLUMN, FALSE,
				    NAME_COLUMN, tag_data[i]->name,
				    -1);
	}

	if (separator_required) {
		gtk_list_store_append (self->priv->store, &iter);
		gtk_list_store_set (self->priv->store, &iter,
				    USED_COLUMN, FALSE,
				    SEPARATOR_COLUMN, TRUE,
				    NAME_COLUMN, "",
				    -1);
	}

	/* suggested */

	separator_required = FALSE;
	for (i = 0; tag_data[i] != NULL; i++) {
		GtkTreeIter iter;

		if (! tag_data[i]->suggested)
			continue;

		separator_required = TRUE;

		gtk_list_store_append (self->priv->store, &iter);
		gtk_list_store_set (self->priv->store, &iter,
				    USED_COLUMN, FALSE,
				    SEPARATOR_COLUMN, FALSE,
				    NAME_COLUMN, tag_data[i]->name,
				    -1);
	}

	if (separator_required) {
		gtk_list_store_append (self->priv->store, &iter);
		gtk_list_store_set (self->priv->store, &iter,
				    USED_COLUMN, FALSE,
				    SEPARATOR_COLUMN, TRUE,
				    NAME_COLUMN, "",
				    -1);
	}

	/* others */

	for (i = 0; tag_data[i] != NULL; i++) {
		GtkTreeIter iter;

		if (tag_data[i]->used || tag_data[i]->suggested)
			continue;

		gtk_list_store_append (self->priv->store, &iter);
		gtk_list_store_set (self->priv->store, &iter,
 				    USED_COLUMN, FALSE,
 				    SEPARATOR_COLUMN, FALSE,
				    NAME_COLUMN, tag_data[i]->name,
				    -1);
	}

	g_strfreev (self->priv->last_used);
	self->priv->last_used = used_tags;

	for (i = 0; tag_data[i] != NULL; i++) {
		g_free (tag_data[i]->name);
		g_free (tag_data[i]);
	}
	g_free (tag_data);
	g_strfreev (all_tags);
}


static void
tags_changed_cb (GthMonitor      *monitor,
		 GthTagsExpander *self)
{
	update_list_view_from_entry (self);
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


static gboolean
row_separator_func (GtkTreeModel *model,
                    GtkTreeIter  *iter,
                    gpointer      data)
{
	gboolean separator;

	gtk_tree_model_get (model, iter, SEPARATOR_COLUMN, &separator, -1);

	return separator;
}


static void
gth_tags_expander_instance_init (GthTagsExpander *self)
{
	GtkTreeViewColumn *column;
	GtkCellRenderer   *renderer;
	GtkWidget         *scrolled_window;

	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_TAGS_EXPANDER, GthTagsExpanderPrivate);
	self->priv->last_used = g_new0 (char *, 1);

	/* the treeview */

	self->priv->store = gtk_list_store_new (N_COLUMNS, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_STRING);
	self->priv->tree_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (self->priv->store));
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (self->priv->tree_view), FALSE);
	gtk_tree_view_set_row_separator_func (GTK_TREE_VIEW (self->priv->tree_view),
					      row_separator_func,
					      self,
					      NULL);
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

	/* the name column. */

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
	update_list_view_from_entry (self);

	g_signal_connect_swapped (self->priv->entry,
				  "notify::text",
				  G_CALLBACK (update_list_view_from_entry),
				  self);

	return (GtkWidget *) self;
}
