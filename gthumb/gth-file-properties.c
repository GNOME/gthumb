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
#include "dlg-edit-metadata.h"
#include "glib-utils.h"
#include "gth-file-properties.h"
#include "gth-main.h"
#include "gth-multipage.h"
#include "gth-sidebar.h"
#include "gth-string-list.h"
#include "gth-time.h"


#define GTH_FILE_PROPERTIES_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GTH_TYPE_FILE_PROPERTIES, GthFilePropertiesPrivate))
#define FONT_SCALE (0.85)
#define COMMENT_HEIGHT 100
#define CATEGORY_SIZE 1000
#define MAX_ATTRIBUTE_LENGTH 128


enum {
	WEIGHT_COLUMN,
	ID_COLUMN,
	DISPLAY_NAME_COLUMN,
	VALUE_COLUMN,
	TOOLTIP_COLUMN,
	RAW_COLUMN,
	POS_COLUMN,
	WRITEABLE_COLUMN,
	NUM_COLUMNS
};


static gpointer gth_file_properties_parent_class = NULL;


struct _GthFilePropertiesPrivate {
	GtkWidget     *tree_view;
	GtkWidget     *comment_view;
	GtkWidget     *comment_win;
	GtkListStore  *tree_model;
};


static char *
get_comment (GthFileData *file_data)
{
	GString     *string;
	GthMetadata *value;
	gboolean     not_void = FALSE;

	string = g_string_new (NULL);

	value = (GthMetadata *) g_file_info_get_attribute_object (file_data->info, "general::comment");
	if (value != NULL) {
		const char *formatted;

		formatted = gth_metadata_get_formatted (value);
		if ((formatted != NULL) && (*formatted != '\0')) {
			g_string_append (string, formatted);
			not_void = TRUE;
		}
	}

	return g_string_free (string, ! not_void);
}


void
gth_file_properties_real_set_file (GthPropertyView *self,
		 		   GthFileData     *file_data)
{
	GthFileProperties *file_properties;
	GHashTable        *category_hash;
	GPtrArray         *metadata_info;
	int                i;
	GtkTextBuffer     *text_buffer;
	char              *comment;

	file_properties = GTH_FILE_PROPERTIES (self);
	gtk_list_store_clear (file_properties->priv->tree_model);

	if (file_data == NULL)
		return;

	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (file_properties->priv->tree_model), GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID, 0);

	category_hash = g_hash_table_new_full (g_str_hash, g_str_equal, (GDestroyNotify) g_free, NULL);
	metadata_info = gth_main_get_all_metadata_info ();
	for (i = 0; i < metadata_info->len; i++) {
		GthMetadataInfo     *info;
		char                *value;
		char                *tooltip;
		GthMetadataCategory *category;
		GtkTreeIter          iter;

		info = g_ptr_array_index (metadata_info, i);
		if ((info->flags & GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW) != GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW)
			continue;

		if ((info->display_name == NULL) || (strncmp (info->display_name, "0x", 2) == 0))
			continue;

		value = gth_file_data_get_attribute_as_string (file_data, info->id);
		if ((value == NULL) || (*value == '\0'))
			continue;

		if (value != NULL) {
			char *tmp_value;

			if (g_utf8_strlen (value, -1) > MAX_ATTRIBUTE_LENGTH) {
				g_utf8_strncpy (value, value, MAX_ATTRIBUTE_LENGTH - 3);
				g_utf8_strncpy (g_utf8_offset_to_pointer (value, MAX_ATTRIBUTE_LENGTH - 3), "...", 3);
			}

			tmp_value = _g_utf8_replace (value, "[\r\n]", " ");
			g_free (value);
			value = tmp_value;
		}
		tooltip = g_markup_printf_escaped ("%s: %s", info->display_name, value);

		category = g_hash_table_lookup (category_hash, info->category);
		if (category == NULL) {
			category = gth_main_get_metadata_category (info->category);
			gtk_list_store_append (file_properties->priv->tree_model, &iter);
			gtk_list_store_set (file_properties->priv->tree_model, &iter,
					    WEIGHT_COLUMN, PANGO_WEIGHT_BOLD,
					    ID_COLUMN, category->id,
					    DISPLAY_NAME_COLUMN, category->display_name,
					    POS_COLUMN, category->sort_order * CATEGORY_SIZE,
					    -1);
			g_hash_table_insert (category_hash, g_strdup (info->category), category);
		}

		gtk_list_store_append (file_properties->priv->tree_model, &iter);
		gtk_list_store_set (file_properties->priv->tree_model,
				    &iter,
				    WEIGHT_COLUMN, PANGO_WEIGHT_NORMAL,
				    ID_COLUMN, info->id,
				    DISPLAY_NAME_COLUMN, info->display_name,
				    VALUE_COLUMN, value,
				    TOOLTIP_COLUMN, tooltip,
				    POS_COLUMN, (category->sort_order * CATEGORY_SIZE) + info->sort_order,
				    -1);

		g_free (tooltip);
		g_free (value);
	}

	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (file_properties->priv->tree_model), POS_COLUMN, GTK_SORT_ASCENDING);
	gtk_tree_view_expand_all (GTK_TREE_VIEW (file_properties->priv->tree_view));

	g_hash_table_destroy (category_hash);

	/* comment */

	text_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (file_properties->priv->comment_view));
	comment = get_comment (file_data);
	if (comment != NULL) {
		GtkTextIter    iter;
		GtkAdjustment *vadj;

		gtk_text_buffer_set_text (text_buffer, comment, strlen (comment));
		gtk_text_buffer_get_iter_at_line (text_buffer, &iter, 0);
		gtk_text_buffer_place_cursor (text_buffer, &iter);

		vadj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (file_properties->priv->comment_win));
		gtk_adjustment_set_value (vadj, 0.0);

		gtk_widget_show (file_properties->priv->comment_win);

		g_free (comment);
	}
	else
		gtk_widget_hide (file_properties->priv->comment_win);
}


const char *
gth_file_properties_real_get_name (GthMultipageChild *self)
{
	return _("Properties");
}


const char *
gth_file_properties_real_get_icon (GthMultipageChild *self)
{
	return GTK_STOCK_PROPERTIES;
}


static void
gth_file_properties_class_init (GthFilePropertiesClass *klass)
{
	gth_file_properties_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GthFilePropertiesPrivate));
}


static void
edit_properties_clicked_cb (GtkButton *button,
			    gpointer   user_data)
{
	GthFileProperties *file_properties = user_data;

	dlg_edit_metadata (GTH_BROWSER (gtk_widget_get_toplevel (GTK_WIDGET (file_properties))));
}


static void
gth_file_properties_init (GthFileProperties *file_properties)
{
	GtkWidget         *vpaned;
	GtkWidget         *scrolled_win;
	GtkCellRenderer   *renderer;
	GtkTreeViewColumn *column;
	GtkWidget         *button;

	file_properties->priv = GTH_FILE_PROPERTIES_GET_PRIVATE (file_properties);

	gtk_box_set_spacing (GTK_BOX (file_properties), 6);

	vpaned = gtk_vpaned_new ();
	gtk_widget_show (vpaned);
	gtk_box_pack_start (GTK_BOX (file_properties), vpaned, TRUE, TRUE, 0);

	scrolled_win = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_win), GTK_SHADOW_ETCHED_IN);
	gtk_widget_show (scrolled_win);
	gtk_paned_pack1 (GTK_PANED (vpaned), scrolled_win, TRUE, TRUE);

	file_properties->priv->tree_view = gtk_tree_view_new ();
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (file_properties->priv->tree_view), FALSE);
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (file_properties->priv->tree_view), TRUE);
	gtk_tree_view_set_tooltip_column (GTK_TREE_VIEW (file_properties->priv->tree_view), TOOLTIP_COLUMN);
	file_properties->priv->tree_model = gtk_list_store_new (NUM_COLUMNS,
								PANGO_TYPE_WEIGHT,
								G_TYPE_STRING,
								G_TYPE_STRING,
								G_TYPE_STRING,
								G_TYPE_STRING,
								G_TYPE_STRING,
							  	G_TYPE_INT,
							  	G_TYPE_BOOLEAN);
	gtk_tree_view_set_model (GTK_TREE_VIEW (file_properties->priv->tree_view),
				 GTK_TREE_MODEL (file_properties->priv->tree_model));
	g_object_unref (file_properties->priv->tree_model);
	gtk_widget_show (file_properties->priv->tree_view);
	gtk_container_add (GTK_CONTAINER (scrolled_win), file_properties->priv->tree_view);

	/**/

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("",
							   renderer,
							   "text", DISPLAY_NAME_COLUMN,
							   "weight", WEIGHT_COLUMN,
							   NULL);
	g_object_set (renderer,
		      "ellipsize", PANGO_ELLIPSIZE_END,
		      "scale", FONT_SCALE,
		      NULL);

	gtk_tree_view_column_set_expand (column, TRUE);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_append_column (GTK_TREE_VIEW (file_properties->priv->tree_view),
				     column);

	/**/

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("",
							   renderer,
							   "text", VALUE_COLUMN,
							   NULL);
	g_object_set (renderer,
		      "ellipsize", PANGO_ELLIPSIZE_END,
		      "scale", FONT_SCALE,
		      NULL);

	gtk_tree_view_column_set_expand (column, TRUE);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_append_column (GTK_TREE_VIEW (file_properties->priv->tree_view),
				     column);

	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (file_properties->priv->tree_model), POS_COLUMN, GTK_SORT_ASCENDING);

	/* comment */

	file_properties->priv->comment_win = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (file_properties->priv->comment_win), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (file_properties->priv->comment_win), GTK_SHADOW_ETCHED_IN);
	gtk_paned_pack2 (GTK_PANED (vpaned), file_properties->priv->comment_win, FALSE, TRUE);

	file_properties->priv->comment_view = gtk_text_view_new ();
	gtk_text_view_set_editable (GTK_TEXT_VIEW (file_properties->priv->comment_view), FALSE);
	gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (file_properties->priv->comment_view), GTK_WRAP_WORD);
	gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (file_properties->priv->comment_view), TRUE);
	gtk_widget_set_size_request (file_properties->priv->comment_view, -1, COMMENT_HEIGHT);
	gtk_widget_show (file_properties->priv->comment_view);
	gtk_container_add (GTK_CONTAINER (file_properties->priv->comment_win), file_properties->priv->comment_view);

	/* edit button */

	button = gtk_button_new_from_stock (GTK_STOCK_EDIT);
	/*gtk_widget_show (button); FIXME */
	gtk_box_pack_start (GTK_BOX (file_properties), button, FALSE, FALSE, 0);

	g_signal_connect (button,
			  "clicked",
			  G_CALLBACK (edit_properties_clicked_cb),
			  file_properties);
}


static void
gth_file_properties_gth_multipage_child_interface_init (GthMultipageChildIface *iface)
{
	iface->get_name = gth_file_properties_real_get_name;
	iface->get_icon = gth_file_properties_real_get_icon;
}


static void
gth_file_properties_gth_property_view_interface_init (GthPropertyViewIface *iface)
{
	iface->set_file = gth_file_properties_real_set_file;
}


GType
gth_file_properties_get_type (void)
{
	static GType type = 0;

	if (type == 0) {
		static const GTypeInfo g_define_type_info = {
			sizeof (GthFilePropertiesClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) gth_file_properties_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (GthFileProperties),
			0,
			(GInstanceInitFunc) gth_file_properties_init,
			NULL
		};
		static const GInterfaceInfo gth_multipage_child_info = {
			(GInterfaceInitFunc) gth_file_properties_gth_multipage_child_interface_init,
			(GInterfaceFinalizeFunc) NULL,
			NULL
		};
		static const GInterfaceInfo gth_property_view_info = {
			(GInterfaceInitFunc) gth_file_properties_gth_property_view_interface_init,
			(GInterfaceFinalizeFunc) NULL,
			NULL
		};
		type = g_type_register_static (GTK_TYPE_VBOX,
					       "GthFileProperties",
					       &g_define_type_info,
					       0);
		g_type_add_interface_static (type, GTH_TYPE_MULTIPAGE_CHILD, &gth_multipage_child_info);
		g_type_add_interface_static (type, GTH_TYPE_PROPERTY_VIEW, &gth_property_view_info);
	}

	return type;
}
