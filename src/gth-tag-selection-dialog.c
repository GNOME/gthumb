/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2007 The Free Software Foundation, Inc.
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
#include <string.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <glade/glade.h>
#include "typedefs.h"
#include "glib-utils.h"
#include "file-utils.h"
#include "bookmarks.h"
#include "gth-tag-selection-dialog.h"

enum {
        RESPONSE,
        LAST_SIGNAL
};


enum {
	C_USE_TAG_COLUMN,
	C_TAG_COLUMN,
	C_NUM_COLUMNS
};


#define TAG_SEPARATOR_C   ';'
#define TAG_SEPARATOR_STR ";"
#define SEARCH_GLADE_FILE     "gthumb_search.glade"
#define DEFAULT_DIALOG_WIDTH  540
#define DEFAULT_DIALOG_HEIGHT 480


struct _GthTagSelectionPrivate
{
	char           *tags;
	gboolean        match_all;
	gboolean        ok_clicked;

	GladeXML       *gui;

	GtkWidget      *dialog;
	GtkWidget      *c_tags_entry;
	GtkWidget      *c_tags_treeview;
	GtkWidget      *c_ok_button;
	GtkWidget      *c_cancel_button;
	GtkWidget      *s_at_least_one_cat_radiobutton;
	GtkWidget      *s_all_cat_radiobutton;

	GtkListStore   *c_tags_list_model;
};


static GObjectClass *parent_class = NULL;
static guint signals[LAST_SIGNAL] = { 0 };


static void
gth_tag_selection_finalize (GObject *object)
{
	GthTagSelection *tag_sel;

	tag_sel = GTH_TAG_SELECTION (object);

	if (tag_sel->priv != NULL) {

		if (tag_sel->priv->dialog != NULL)
			g_signal_handlers_disconnect_by_data (tag_sel->priv->dialog, tag_sel);

		g_free (tag_sel->priv->tags);
		tag_sel->priv->tags = NULL;

		if (tag_sel->priv->gui != NULL) {
			g_object_unref (tag_sel->priv->gui);
			tag_sel->priv->gui = NULL;
		}

		g_free (tag_sel->priv);
		tag_sel->priv = NULL;
	}

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gth_tag_selection_class_init (GthTagSelectionClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;

	object_class->finalize = gth_tag_selection_finalize;

	signals[RESPONSE] =
		g_signal_new ("response",
                              G_TYPE_FROM_CLASS (class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GthTagSelectionClass, response),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__INT,
                              G_TYPE_NONE,
                              1, G_TYPE_INT);
}


static void
gth_tag_selection_init (GthTagSelection *tag_sel)
{
	tag_sel->priv = g_new0 (GthTagSelectionPrivate, 1);
}


static void
update_tag_entry (GthTagSelection *tag_sel)
{
	GthTagSelectionPrivate *priv = tag_sel->priv;
	GtkTreeIter   iter;
	GtkTreeModel *model = GTK_TREE_MODEL (priv->c_tags_list_model);
	GString      *tags;

	if (! gtk_tree_model_get_iter_first (model, &iter)) {
		gtk_entry_set_text (GTK_ENTRY (priv->c_tags_entry), "");
		return;
	}

	tags = g_string_new (NULL);
	do {
		gboolean use_tag;
		gtk_tree_model_get (model, &iter, C_USE_TAG_COLUMN, &use_tag, -1);
		if (use_tag) {
			char *tag_name;

			gtk_tree_model_get (model, &iter,
					    C_TAG_COLUMN, &tag_name,
					    -1);
			if (tags->len > 0)
				tags = g_string_append (tags, TAG_SEPARATOR_STR " ");
			tags = g_string_append (tags, tag_name);
			g_free (tag_name);
		}
	} while (gtk_tree_model_iter_next (model, &iter));

	gtk_entry_set_text (GTK_ENTRY (priv->c_tags_entry), tags->str);
	g_string_free (tags, TRUE);
}


static GList *
get_tags_from_entry (GthTagSelection *tag_sel)
{
	GthTagSelectionPrivate *priv = tag_sel->priv;
	GList       *cat_list = NULL;
	const char  *utf8_text;
	char       **tags;
	int          i;

	utf8_text = gtk_entry_get_text (GTK_ENTRY (priv->c_tags_entry));
	if (utf8_text == NULL)
		return NULL;

	tags = _g_utf8_strsplit (utf8_text, TAG_SEPARATOR_C);

	for (i = 0; tags[i] != NULL; i++) {
		char *s;

		s = _g_utf8_strstrip (tags[i]);

		if (s != NULL)
			cat_list = g_list_prepend (cat_list, s);
	}
	g_strfreev (tags);

	return g_list_reverse (cat_list);
}


static void
add_saved_tags (GthTagSelection *tag_sel,
                GList           *cat_list)
{
	GthTagSelectionPrivate *priv = tag_sel->priv;
	Bookmarks *tags;
	GList     *scan;

	tags = bookmarks_new (RC_TAGS_FILE);
	bookmarks_load_from_disk (tags);

	for (scan = tags->list; scan; scan = scan->next) {
		GtkTreeIter  iter;
		GList       *scan2;
		gboolean     found = FALSE;
		char        *tag1 = scan->data;

		for (scan2 = cat_list; scan2 && !found; scan2 = scan2->next) {
			char *tag2 = scan2->data;
			if (strcmp (tag1, tag2) == 0)
				found = TRUE;
		}

		if (found)
			continue;

		gtk_list_store_append (priv->c_tags_list_model, &iter);

		gtk_list_store_set (priv->c_tags_list_model, &iter,
				    C_USE_TAG_COLUMN, FALSE,
				    C_TAG_COLUMN, tag1,
				    -1);
	}

	bookmarks_free (tags);
}


static void
update_list_from_entry (GthTagSelection *tag_sel)
{
	GthTagSelectionPrivate *priv = tag_sel->priv;
	GList *tags = NULL;
	GList *scan;

	tags = get_tags_from_entry (tag_sel);

	gtk_list_store_clear (priv->c_tags_list_model);
	for (scan = tags; scan; scan = scan->next) {
		char        *tag = scan->data;
		GtkTreeIter  iter;

		gtk_list_store_append (priv->c_tags_list_model, &iter);

		gtk_list_store_set (priv->c_tags_list_model, &iter,
				    C_USE_TAG_COLUMN, TRUE,
				    C_TAG_COLUMN, tag,
				    -1);
	}
	add_saved_tags (tag_sel, tags);
	path_list_free (tags);
}


static void
use_tag_toggled (GtkCellRendererToggle *cell,
                 gchar                 *path_string,
                 gpointer               callback_data)
{
	GthTagSelection *tag_sel = callback_data;
	GthTagSelectionPrivate *priv = tag_sel->priv;
	GtkTreeModel *model = GTK_TREE_MODEL (priv->c_tags_list_model);
	GtkTreeIter   iter;
	GtkTreePath  *path = gtk_tree_path_new_from_string (path_string);
	gboolean      value;

	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_model_get (model, &iter, C_USE_TAG_COLUMN, &value, -1);

	value = !value;
	gtk_list_store_set (GTK_LIST_STORE (model), &iter, C_USE_TAG_COLUMN, value, -1);

	gtk_tree_path_free (path);
	update_tag_entry (tag_sel);
}


static void
choose_tags_ok_cb (GtkWidget       *widget,
                   GthTagSelection *tag_sel)
{
	GthTagSelectionPrivate *priv = tag_sel->priv;

	priv->ok_clicked = TRUE;

	g_free (priv->tags);
	priv->tags = g_strdup (gtk_entry_get_text (GTK_ENTRY (priv->c_tags_entry)));
	priv->match_all = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->s_all_cat_radiobutton));

	gtk_widget_destroy (priv->dialog);
	priv->dialog = NULL;

	g_signal_emit (tag_sel, signals[RESPONSE], 0, GTK_RESPONSE_OK);
}


static gboolean
tags_dialog__destroy_cb (GtkWidget       *widget,
                         GthTagSelection *tag_sel)
{
	tag_sel->priv->dialog = NULL;

	return FALSE;

	if (! tag_sel->priv->ok_clicked) {
		g_object_ref (tag_sel);
		g_signal_emit (tag_sel, signals[RESPONSE], 0, GTK_RESPONSE_CANCEL);
		g_object_unref (tag_sel);
	}
}


static void
gth_tag_selection_construct (GthTagSelection *tag_sel,
                             GtkWindow       *parent,
                             const char      *tags,
                             gboolean         match_all)
{
	GthTagSelectionPrivate *priv;
	GtkCellRenderer             *renderer;
	GtkTreeViewColumn           *column;

	priv = tag_sel->priv;

	priv->gui = glade_xml_new (GTHUMB_GLADEDIR "/" SEARCH_GLADE_FILE,
				   NULL,
				   NULL);
	if (! priv->gui) {
		g_warning ("Could not find " SEARCH_GLADE_FILE "\n");
		return;
	}

	if (tags != NULL)
		priv->tags = g_strdup (tags);
	priv->match_all = match_all;

	/* Get the widgets. */

	priv->dialog = glade_xml_get_widget (priv->gui, "tags_dialog");
	priv->c_tags_entry = glade_xml_get_widget (priv->gui, "c_tags_entry");
	priv->c_tags_treeview = glade_xml_get_widget (priv->gui, "c_tags_treeview");
	priv->c_ok_button = glade_xml_get_widget (priv->gui, "c_ok_button");
	priv->c_cancel_button = glade_xml_get_widget (priv->gui, "c_cancel_button");
	priv->s_at_least_one_cat_radiobutton = glade_xml_get_widget (priv->gui, "s_at_least_one_cat_radiobutton");
	priv->s_all_cat_radiobutton = glade_xml_get_widget (priv->gui, "s_all_cat_radiobutton");

	/* Set widgets data. */

	priv->c_tags_list_model = gtk_list_store_new (C_NUM_COLUMNS,
							    G_TYPE_BOOLEAN,
							    G_TYPE_STRING);
	gtk_tree_view_set_model (GTK_TREE_VIEW (priv->c_tags_treeview),
				 GTK_TREE_MODEL (priv->c_tags_list_model));
	g_object_unref (priv->c_tags_list_model);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (priv->c_tags_treeview), FALSE);

	renderer = gtk_cell_renderer_toggle_new ();
	g_signal_connect (G_OBJECT (renderer),
			  "toggled",
			  G_CALLBACK (use_tag_toggled),
			  tag_sel);
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (priv->c_tags_treeview),
						     -1, "Use",
						     renderer,
						     "active", C_USE_TAG_COLUMN,
						     NULL);

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("",
							   renderer,
							   "text", C_TAG_COLUMN,
							   NULL);

	gtk_tree_view_column_set_sort_column_id (column, 0);
	gtk_tree_view_append_column (GTK_TREE_VIEW (priv->c_tags_treeview),
				     column);

	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (priv->c_tags_list_model), C_TAG_COLUMN, GTK_SORT_ASCENDING);

	gtk_entry_set_text (GTK_ENTRY (priv->c_tags_entry), tags);
	update_list_from_entry (tag_sel);

	if (priv->match_all)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->s_all_cat_radiobutton), TRUE);
	else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->s_at_least_one_cat_radiobutton), TRUE);

	/* Set the signals handlers. */

	g_signal_connect (G_OBJECT (priv->dialog),
			  "destroy",
			  G_CALLBACK (tags_dialog__destroy_cb),
			  tag_sel);
	g_signal_connect (G_OBJECT (priv->c_ok_button),
			  "clicked",
			  G_CALLBACK (choose_tags_ok_cb),
			  tag_sel);
	g_signal_connect_swapped (G_OBJECT (priv->c_cancel_button),
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  G_OBJECT (priv->dialog));

	/* Run dialog. */

	gtk_widget_grab_focus (priv->c_tags_treeview);

	if (parent != NULL) {
		gtk_window_set_transient_for (GTK_WINDOW (priv->dialog), parent);
		gtk_window_set_modal (GTK_WINDOW (priv->dialog), TRUE);
	}

	gtk_widget_show (priv->dialog);
}


GType
gth_tag_selection_get_type (void)
{
        static GType type = 0;

        if (! type) {
                GTypeInfo type_info = {
			sizeof (GthTagSelectionClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_tag_selection_class_init,
			NULL,
			NULL,
			sizeof (GthTagSelection),
			0,
			(GInstanceInitFunc) gth_tag_selection_init
		};

		type = g_type_register_static (G_TYPE_OBJECT,
					       "GthTagSelection",
					       &type_info,
					       0);
	}

        return type;
}


GthTagSelection*
gth_tag_selection_new (GtkWindow  *parent,
                       const char *tags,
                       gboolean    match_all)
{
	GthTagSelection *csel;

	csel = GTH_TAG_SELECTION (g_object_new (GTH_TYPE_TAG_SELECTION, NULL));
	gth_tag_selection_construct (csel, parent, tags, match_all);

	return csel;
}


char *
gth_tag_selection_get_tags (GthTagSelection *csel)
{
	return csel->priv->tags;
}


gboolean
gth_tag_selection_get_match_all (GthTagSelection *csel)
{
	return csel->priv->match_all;
}
