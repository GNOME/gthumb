/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2010 Free Software Foundation, Inc.
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
#include "gth-tag-chooser-dialog.h"

#define GTH_TAG_CHOOSER_DIALOG_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GTH_TYPE_TAG_CHOOSER_DIALOG, GthTagChooserDialogPrivate))
#define GET_WIDGET(name) _gtk_builder_get_widget (self->priv->builder, (name))


enum {
	NAME_COLUMN = 0,
	SELECTED_COLUMN
};


static gpointer parent_class = NULL;


struct _GthTagChooserDialogPrivate {
	GtkBuilder *builder;
};


static void
gth_tag_chooser_dialog_finalize (GObject *object)
{
	GthTagChooserDialog *self;

	self = GTH_TAG_CHOOSER_DIALOG (object);

	_g_object_unref (self->priv->builder);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gth_tag_chooser_dialog_class_init (GthTagChooserDialogClass *klass)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GthTagChooserDialogPrivate));

	object_class = (GObjectClass*) klass;
	object_class->finalize = gth_tag_chooser_dialog_finalize;
}


static void
new_button_clicked_cb (GtkButton *button,
		       gpointer   user_data)
{
	GthTagChooserDialog *self = user_data;
	GtkTreeIter          iter;
	GtkTreePath         *path;

	gtk_list_store_append (GTK_LIST_STORE (GET_WIDGET ("tags_liststore")), &iter);
	gtk_list_store_set (GTK_LIST_STORE (GET_WIDGET ("tags_liststore")), &iter,
			    SELECTED_COLUMN, TRUE,
			    NAME_COLUMN, _("New tag"),
			    -1);

	path = gtk_tree_model_get_path (GTK_TREE_MODEL (GET_WIDGET ("tags_liststore")), &iter);
	gtk_tree_view_set_cursor_on_cell (GTK_TREE_VIEW (GET_WIDGET ("tags_treeview")),
					  path,
					  GTK_TREE_VIEW_COLUMN (GET_WIDGET ("treeviewcolumn2")),
					  GTK_CELL_RENDERER (GET_WIDGET ("name_cellrenderertext")),
					  TRUE);

	gtk_tree_path_free (path);
}


static void
delete_button_clicked_cb (GtkButton *button,
			  gpointer   user_data)
{
	GthTagChooserDialog *self = user_data;
	GtkTreeIter          iter;
	char                *selected_tag;
	GthTagsFile         *tags;

	if (! gtk_tree_selection_get_selected (gtk_tree_view_get_selection (GTK_TREE_VIEW (GET_WIDGET ("tags_treeview"))), NULL, &iter))
		return;

	gtk_tree_model_get (GTK_TREE_MODEL (GET_WIDGET ("tags_liststore")), &iter,
			    NAME_COLUMN, &selected_tag,
			    -1);

	tags = gth_main_get_default_tag_file ();
	gth_tags_file_remove (tags, selected_tag);
	gth_main_tags_changed ();

	gtk_list_store_remove (GTK_LIST_STORE (GET_WIDGET ("tags_liststore")), &iter);

	g_free (selected_tag);
}


static void
selected_cellrenderertoggle_toggled_cb (GtkCellRendererToggle *cell_renderer,
					char                  *path,
					gpointer               user_data)
{
	GthTagChooserDialog *self = user_data;
	GtkTreePath         *tpath;
	GtkTreeIter          iter;

	tpath = gtk_tree_path_new_from_string (path);
	if (tpath == NULL)
		return;

	if (gtk_tree_model_get_iter (GTK_TREE_MODEL (GET_WIDGET ("tags_liststore")), &iter, tpath)) {
		gboolean selected;

		gtk_tree_model_get (GTK_TREE_MODEL (GET_WIDGET ("tags_liststore")), &iter,
				    SELECTED_COLUMN, &selected,
				    -1);
		gtk_list_store_set (GTK_LIST_STORE (GET_WIDGET ("tags_liststore")), &iter,
				    SELECTED_COLUMN, ! selected,
				    -1);
	}

	gtk_tree_path_free (tpath);
}


static void
name_cellrenderertext_edited_cb (GtkCellRendererText *renderer,
				 char                *path,
				 char                *new_text,
				 gpointer             user_data)
{
	GthTagChooserDialog *self = user_data;
	GtkTreePath         *tree_path;
	GtkTreeIter          iter;
	char                *old_text;
	GthTagsFile         *tags;

	tree_path = gtk_tree_path_new_from_string (path);
	if (! gtk_tree_model_get_iter (GTK_TREE_MODEL (GET_WIDGET ("tags_liststore")),
				       &iter,
				       tree_path))
	{
		gtk_tree_path_free (tree_path);
		return;
	}
	gtk_tree_path_free (tree_path);

	gtk_tree_model_get (GTK_TREE_MODEL (GET_WIDGET ("tags_liststore")),
			    &iter,
			    NAME_COLUMN, &old_text,
			    -1);

	tags = gth_main_get_default_tag_file ();
	if (old_text != NULL)
		gth_tags_file_remove (tags, old_text);
	if (new_text != NULL)
		gth_tags_file_add (tags, new_text);
	gth_main_tags_changed ();

	gtk_list_store_set (GTK_LIST_STORE (GET_WIDGET ("tags_liststore")),
			    &iter,
			    NAME_COLUMN, new_text,
			    -1);

	g_free (old_text);
}


static int
tags_liststore_sort_func (GtkTreeModel *model,
			  GtkTreeIter  *a,
			  GtkTreeIter  *b,
			  gpointer      user_data)
{
	char *name_a;
	char *name_b;
	int   result;

	gtk_tree_model_get (model, a, NAME_COLUMN, &name_a, -1);
	gtk_tree_model_get (model, b, NAME_COLUMN, &name_b, -1);
	result = g_utf8_collate (name_a, name_b);

	g_free (name_a);
	g_free (name_b);

	return result;
}


static void
gth_tag_chooser_dialog_init (GthTagChooserDialog *self)
{
	GtkWidget  *content;
	char      **tags;
	int         i;

	self->priv = GTH_TAG_CHOOSER_DIALOG_GET_PRIVATE (self);
	self->priv->builder = _gtk_builder_new_from_file ("tag-chooser.ui", "edit_metadata");

	gtk_window_set_title (GTK_WINDOW (self), _("Assign Tags"));
	gtk_window_set_resizable (GTK_WINDOW (self), FALSE);
	gtk_box_set_spacing (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (self))), 5);
	gtk_container_set_border_width (GTK_CONTAINER (self), 5);

	gtk_dialog_add_button (GTK_DIALOG (self), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
	gtk_dialog_add_button (GTK_DIALOG (self), GTK_STOCK_OK, GTK_RESPONSE_OK);
	gtk_dialog_add_button (GTK_DIALOG (self), GTK_STOCK_HELP, GTK_RESPONSE_HELP);

	content = _gtk_builder_get_widget (self->priv->builder, "content");
	gtk_container_set_border_width (GTK_CONTAINER (content), 5);
	gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (self))), content, TRUE, TRUE, 0);

	gtk_tree_sortable_set_default_sort_func (GTK_TREE_SORTABLE (GET_WIDGET ("tags_liststore")), tags_liststore_sort_func, self, NULL);
	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (GET_WIDGET ("tags_liststore")), GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID, GTK_SORT_ASCENDING);

	g_signal_connect (GET_WIDGET ("selected_cellrenderertoggle"),
			  "toggled",
			  G_CALLBACK (selected_cellrenderertoggle_toggled_cb),
			  self);
	g_signal_connect (GET_WIDGET ("name_cellrenderertext"),
			  "edited",
			  G_CALLBACK (name_cellrenderertext_edited_cb),
			  self);
	g_signal_connect (GET_WIDGET ("new_button"),
			  "clicked",
			  G_CALLBACK (new_button_clicked_cb),
			  self);
	g_signal_connect (GET_WIDGET ("delete_button"),
			  "clicked",
			  G_CALLBACK (delete_button_clicked_cb),
			  self);

	tags = g_strdupv (gth_tags_file_get_tags (gth_main_get_default_tag_file ()));
	for (i = 0; tags[i] != NULL; i++) {
		GtkTreeIter iter;

		gtk_list_store_append (GTK_LIST_STORE (GET_WIDGET ("tags_liststore")), &iter);
		gtk_list_store_set (GTK_LIST_STORE (GET_WIDGET ("tags_liststore")), &iter,
				    NAME_COLUMN, tags[i],
				    SELECTED_COLUMN, 0,
				    -1);
	}

	g_strfreev (tags);
}


GType
gth_tag_chooser_dialog_get_type (void)
{
	static GType type = 0;

	if (type == 0) {
		static const GTypeInfo g_define_type_info = {
			sizeof (GthTagChooserDialogClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) gth_tag_chooser_dialog_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (GthTagChooserDialog),
			0,
			(GInstanceInitFunc) gth_tag_chooser_dialog_init,
			NULL
		};
		type = g_type_register_static (GTK_TYPE_DIALOG,
					       "GthTagChooserDialog",
					       &g_define_type_info,
					       0);
	}

	return type;
}


GtkWidget *
gth_tag_chooser_dialog_new (void)
{
	return (GtkWidget *) g_object_new (GTH_TYPE_TAG_CHOOSER_DIALOG, NULL);
}


char **
gth_tag_chooser_dialog_get_tags (GthTagChooserDialog *self)
{
	GtkTreeModel  *model;
	GtkTreeIter    iter;
	GList         *tag_list;
	char         **tags;

	tag_list = NULL;
	model = GTK_TREE_MODEL (GET_WIDGET ("tags_liststore"));
	if (gtk_tree_model_get_iter_first (model, &iter)) {
		do {
			gboolean  selected;
			char     *name;

			gtk_tree_model_get (model, &iter,
					    SELECTED_COLUMN, &selected,
					    NAME_COLUMN, &name,
					    -1);
			if (selected)
				tag_list = g_list_append (tag_list, name);
			else
				g_free (name);
		}
		while (gtk_tree_model_iter_next (model, &iter));
	}

	tags = _g_string_list_to_strv (tag_list);
	_g_string_list_free (tag_list);

	return tags;
}
