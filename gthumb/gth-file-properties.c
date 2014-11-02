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
#include "glib-utils.h"
#include "gth-file-properties.h"
#include "gth-main.h"
#include "gth-multipage.h"
#include "gth-sidebar.h"
#include "gth-string-list.h"
#include "gth-time.h"
#include "gtk-utils.h"


#define GTH_FILE_PROPERTIES_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GTH_TYPE_FILE_PROPERTIES, GthFilePropertiesPrivate))
#define FONT_SCALE (0.85)
#define MIN_HEIGHT 100
#define COMMENT_DEFAULT_HEIGHT 100
#define CATEGORY_SIZE 1000
#define MAX_ATTRIBUTE_LENGTH 128
#define GTH_STYLE_CLASS_COMMENT "comment"

/* Properties */
enum {
        PROP_0,
        PROP_SHOW_DETAILS
};

enum {
	SCALE_SET_COLUMN,
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


struct _GthFilePropertiesPrivate {
	GtkWidget     *tree_view;
	GtkWidget     *comment_view;
	GtkWidget     *comment_win;
	GtkListStore  *tree_model;
	GtkWidget     *popup_menu;
	gboolean       show_details;
	gboolean       details_available;
	GthFileData   *last_file_data;
};


static void gth_file_properties_gth_multipage_child_interface_init (GthMultipageChildInterface *iface);
static void gth_file_properties_gth_property_view_interface_init (GthPropertyViewInterface *iface);


G_DEFINE_TYPE_WITH_CODE (GthFileProperties,
			 gth_file_properties,
			 GTK_TYPE_BOX,
			 G_IMPLEMENT_INTERFACE (GTH_TYPE_MULTIPAGE_CHILD,
					 	gth_file_properties_gth_multipage_child_interface_init)
		         G_IMPLEMENT_INTERFACE (GTH_TYPE_PROPERTY_VIEW,
		        		 	gth_file_properties_gth_property_view_interface_init))


static char *
get_comment (GthFileData *file_data)
{
	GString     *string;
	GthMetadata *value;
	gboolean     not_void = FALSE;

	string = g_string_new (NULL);

	value = (GthMetadata *) g_file_info_get_attribute_object (file_data->info, "general::description");
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


static void
gth_file_properties_real_set_file (GthPropertyView *base,
		 		   GthFileData     *file_data)
{
	GthFileProperties *self;
	GHashTable        *category_hash;
	GList             *metadata_info;
	GList             *scan;
	GtkTextBuffer     *text_buffer;
	char              *comment;

	self = GTH_FILE_PROPERTIES (base);

	if (file_data != self->priv->last_file_data) {
		_g_object_unref (self->priv->last_file_data);
		self->priv->last_file_data = gth_file_data_dup (file_data);
	}

	gtk_list_store_clear (self->priv->tree_model);

	if (file_data == NULL) {
		gtk_widget_hide (self->priv->comment_win);
		return;
	}

	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (self->priv->tree_model), GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID, 0);

	self->priv->details_available = FALSE;
	category_hash = g_hash_table_new_full (g_str_hash, g_str_equal, (GDestroyNotify) g_free, NULL);
	metadata_info = gth_main_get_all_metadata_info ();
	for (scan = metadata_info; scan; scan = scan->next) {
		GthMetadataInfo     *info = scan->data;
		char                *value;
		char                *tooltip;
		GthMetadataCategory *category;
		GtkTreeIter          iter;

		if ((info->flags & GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW) != GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW)
			continue;

		value = gth_file_data_get_attribute_as_string (file_data, info->id);
		if ((value == NULL) || (*value == '\0'))
			continue;

		if (info->id != NULL) {
			if (g_str_has_prefix (info->id, "Exif")) {
				self->priv->details_available = TRUE;
				if (! self->priv->show_details)
					continue;
			}
			if (g_str_has_prefix (info->id, "Iptc")) {
				self->priv->details_available = TRUE;
				if (! self->priv->show_details)
					continue;
			}
			if (g_str_has_prefix (info->id, "Xmp")) {
				self->priv->details_available = TRUE;
				if (! self->priv->show_details)
					continue;
			}
		}

		if (value != NULL) {
			char *utf8_value;
			char *tmp_value;

			utf8_value = _g_utf8_from_any (value);
			if (g_utf8_strlen (utf8_value, -1) > MAX_ATTRIBUTE_LENGTH)
				g_utf8_strncpy (g_utf8_offset_to_pointer (utf8_value, MAX_ATTRIBUTE_LENGTH - 1), "…", 1);
			tmp_value = _g_utf8_replace (utf8_value, "[\r\n]", " ");
			g_free (value);
			value = tmp_value;

			g_free (utf8_value);
		}
		tooltip = g_markup_printf_escaped ("%s: %s", _(info->display_name), value);

		category = g_hash_table_lookup (category_hash, info->category);
		if (category == NULL) {
			category = gth_main_get_metadata_category (info->category);
			gtk_list_store_append (self->priv->tree_model, &iter);
			gtk_list_store_set (self->priv->tree_model, &iter,
					    SCALE_SET_COLUMN, FALSE,
					    WEIGHT_COLUMN, PANGO_WEIGHT_BOLD,
					    ID_COLUMN, category->id,
					    DISPLAY_NAME_COLUMN, _(category->display_name),
					    POS_COLUMN, category->sort_order * CATEGORY_SIZE,
					    -1);
			g_hash_table_insert (category_hash, g_strdup (info->category), category);
		}

		gtk_list_store_append (self->priv->tree_model, &iter);
		gtk_list_store_set (self->priv->tree_model,
				    &iter,
				    SCALE_SET_COLUMN, TRUE,
				    WEIGHT_COLUMN, PANGO_WEIGHT_NORMAL,
				    ID_COLUMN, info->id,
				    DISPLAY_NAME_COLUMN, _(info->display_name),
				    VALUE_COLUMN, value,
				    TOOLTIP_COLUMN, tooltip,
				    POS_COLUMN, (category->sort_order * CATEGORY_SIZE) + info->sort_order,
				    -1);

		g_free (tooltip);
		g_free (value);
	}
	g_list_free (metadata_info);

	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (self->priv->tree_model), POS_COLUMN, GTK_SORT_ASCENDING);
	gtk_tree_view_expand_all (GTK_TREE_VIEW (self->priv->tree_view));

	g_hash_table_destroy (category_hash);

	/* comment */

	comment = NULL;
	if (! self->priv->show_details) {
		text_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (self->priv->comment_view));
		comment = get_comment (file_data);
	}
	if (comment != NULL) {
		GtkTextIter    iter;
		GtkAdjustment *vadj;

		gtk_text_buffer_set_text (text_buffer, comment, strlen (comment));
		gtk_text_buffer_get_iter_at_line (text_buffer, &iter, 0);
		gtk_text_buffer_place_cursor (text_buffer, &iter);

		vadj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (self->priv->comment_win));
		gtk_adjustment_set_value (vadj, 0.0);

		gtk_widget_show (self->priv->comment_win);

		g_free (comment);
	}
	else
		gtk_widget_hide (self->priv->comment_win);
}


static const char *
gth_file_properties_real_get_name (GthMultipageChild *self)
{
	return _("Properties");
}


static const char *
gth_file_properties_real_get_icon (GthMultipageChild *self)
{
	return "document-properties-symbolic";
}


static void
gth_file_properties_finalize (GObject *base)
{
	GthFileProperties *self;

	self = (GthFileProperties *) base;

	if (self->priv->popup_menu != NULL)
		gtk_widget_destroy (self->priv->popup_menu);
	_g_object_unref (self->priv->last_file_data);

	G_OBJECT_CLASS (gth_file_properties_parent_class)->finalize (base);
}


static void
gth_file_properties_set_property (GObject      *object,
				  guint         property_id,
				  const GValue *value,
				  GParamSpec   *pspec)
{
	GthFileProperties *self;

        self = GTH_FILE_PROPERTIES (object);

	switch (property_id) {
	case PROP_SHOW_DETAILS:
		self->priv->show_details = g_value_get_boolean (value);
		break;
	default:
		break;
	}
}


static void
gth_file_properties_get_property (GObject    *object,
				  guint       property_id,
				  GValue     *value,
				  GParamSpec *pspec)
{
	GthFileProperties *self;

        self = GTH_FILE_PROPERTIES (object);

	switch (property_id) {
	case PROP_SHOW_DETAILS:
		g_value_set_boolean (value, self->priv->show_details);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}


static void
gth_file_properties_class_init (GthFilePropertiesClass *klass)
{
	GObjectClass *object_class;

	g_type_class_add_private (klass, sizeof (GthFilePropertiesPrivate));

	object_class = G_OBJECT_CLASS (klass);
	object_class->set_property = gth_file_properties_set_property;
	object_class->get_property = gth_file_properties_get_property;
	object_class->finalize = gth_file_properties_finalize;

	/* properties */

	g_object_class_install_property (object_class,
					 PROP_SHOW_DETAILS,
					 g_param_spec_boolean ("show-details",
							       "Show details",
							       "Whether to show all the file properties",
							       FALSE,
							       G_PARAM_READWRITE));
}


static void
copy_menu_item_activate_cb (GtkMenuItem *menuitem,
			    gpointer     user_data)
{
	GthFileProperties *self = user_data;
	GtkTreeModel      *model;
	GtkTreeIter        iter;
	char              *value;

	if (! gtk_tree_selection_get_selected (GTK_TREE_SELECTION (gtk_tree_view_get_selection (GTK_TREE_VIEW (self->priv->tree_view))), &model, &iter))
		return;

	gtk_tree_model_get (model, &iter, VALUE_COLUMN, &value, -1);
	if (value != NULL)
		gtk_clipboard_set_text (gtk_clipboard_get_for_display (gtk_widget_get_display (GTK_WIDGET (menuitem)), GDK_SELECTION_CLIPBOARD),
					value,
					-1);

	g_free (value);
}


static gboolean
tree_view_button_press_event_cb (GtkWidget      *widget,
				 GdkEventButton *event,
				 gpointer        user_data)
{
	GthFileProperties *self = user_data;

	if ((event->type == GDK_BUTTON_PRESS) && (event->button == 3)) {
		GtkTreePath *path;

		if (! gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (self->priv->tree_view),
						     event->x,
						     event->y,
						     &path,
						     NULL,
						     NULL,
						     NULL))
			return FALSE;

		gtk_tree_selection_select_path (GTK_TREE_SELECTION (gtk_tree_view_get_selection (GTK_TREE_VIEW (self->priv->tree_view))), path);
		gtk_menu_popup (GTK_MENU (self->priv->popup_menu),
				NULL,
				NULL,
				NULL,
				NULL,
				event->button,
				event->time);

		gtk_tree_path_free (path);

		return TRUE;
	}

	return FALSE;
}


static gboolean
tree_view_popup_menu_cb (GtkWidget *widget,
			 gpointer   user_data)
{
	GthFileProperties *self = user_data;

	gtk_menu_popup (GTK_MENU (self->priv->popup_menu),
			NULL,
			NULL,
			NULL,
			NULL,
			0,
			gtk_get_current_event_time ());

	return TRUE;
}


static void
gth_file_properties_init (GthFileProperties *self)
{
	GtkWidget         *vpaned;
	GtkWidget         *properties_box;
	GtkWidget         *scrolled_win;
	GtkWidget         *button_box;
	GtkWidget         *menu_item;
	GtkCellRenderer   *renderer;
	GtkTreeViewColumn *column;


	self->priv = GTH_FILE_PROPERTIES_GET_PRIVATE (self);
	self->priv->show_details = FALSE;
	self->priv->last_file_data = NULL;

	gtk_orientable_set_orientation (GTK_ORIENTABLE (self), GTK_ORIENTATION_VERTICAL);
	gtk_box_set_spacing (GTK_BOX (self), 6);

	vpaned = gtk_paned_new (GTK_ORIENTATION_VERTICAL);
	gtk_widget_show (vpaned);
	gtk_box_pack_start (GTK_BOX (self), vpaned, TRUE, TRUE, 0);

	properties_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 5);
	gtk_widget_show (properties_box);
	gtk_paned_pack1 (GTK_PANED (vpaned), properties_box, TRUE, FALSE);

	scrolled_win = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_win), GTK_SHADOW_NONE);
	gtk_widget_show (scrolled_win);
	gtk_widget_set_size_request (scrolled_win, -1, MIN_HEIGHT);
	gtk_box_pack_start (GTK_BOX (properties_box), scrolled_win, TRUE, TRUE, 0);

	button_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_widget_show (button_box);
	gtk_box_pack_start (GTK_BOX (properties_box), button_box, FALSE, FALSE, 2);

	self->priv->tree_view = gtk_tree_view_new ();
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (self->priv->tree_view), FALSE);
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (self->priv->tree_view), TRUE);
	gtk_tree_view_set_tooltip_column (GTK_TREE_VIEW (self->priv->tree_view), TOOLTIP_COLUMN);
	self->priv->tree_model = gtk_list_store_new (NUM_COLUMNS,
						     G_TYPE_BOOLEAN,
						     PANGO_TYPE_WEIGHT,
						     G_TYPE_STRING,
						     G_TYPE_STRING,
						     G_TYPE_STRING,
						     G_TYPE_STRING,
						     G_TYPE_STRING,
						     G_TYPE_INT,
						     G_TYPE_BOOLEAN);
	gtk_tree_view_set_model (GTK_TREE_VIEW (self->priv->tree_view),
				 GTK_TREE_MODEL (self->priv->tree_model));
	g_object_unref (self->priv->tree_model);
	gtk_widget_show (self->priv->tree_view);
	gtk_container_add (GTK_CONTAINER (scrolled_win), self->priv->tree_view);

	/* popup menu */

	self->priv->popup_menu = gtk_menu_new ();
	menu_item = gtk_menu_item_new_with_label (_GTK_LABEL_COPY);
	gtk_widget_show (menu_item);
	gtk_menu_shell_append (GTK_MENU_SHELL (self->priv->popup_menu),
			       menu_item);
	g_signal_connect (menu_item,
			  "activate",
			  G_CALLBACK (copy_menu_item_activate_cb),
			  self);
	g_signal_connect (self->priv->tree_view,
			  "button-press-event",
			  G_CALLBACK (tree_view_button_press_event_cb),
			  self);
	g_signal_connect (self->priv->tree_view,
			  "popup-menu",
			  G_CALLBACK (tree_view_popup_menu_cb),
			  self);

	/**/

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("",
							   renderer,
							   "text", DISPLAY_NAME_COLUMN,
							   "scale-set", SCALE_SET_COLUMN,
							   "weight", WEIGHT_COLUMN,
							   NULL);
	g_object_set (renderer,
		      "ellipsize", PANGO_ELLIPSIZE_END,
		      "scale", FONT_SCALE,
		      NULL);

	gtk_tree_view_column_set_expand (column, TRUE);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_append_column (GTK_TREE_VIEW (self->priv->tree_view),
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
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_append_column (GTK_TREE_VIEW (self->priv->tree_view),
				     column);

	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (self->priv->tree_model), POS_COLUMN, GTK_SORT_ASCENDING);

	/* comment */

	self->priv->comment_win = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (self->priv->comment_win), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (self->priv->comment_win), GTK_SHADOW_ETCHED_IN);
	gtk_widget_set_size_request (self->priv->comment_win, -1, COMMENT_DEFAULT_HEIGHT);
	gtk_style_context_add_class (gtk_widget_get_style_context (self->priv->comment_win), GTH_STYLE_CLASS_COMMENT);
	gtk_paned_pack2 (GTK_PANED (vpaned), self->priv->comment_win, FALSE, FALSE);

	self->priv->comment_view = gtk_text_view_new ();
	gtk_style_context_add_class (gtk_widget_get_style_context (self->priv->comment_view), GTH_STYLE_CLASS_COMMENT);
	gtk_text_view_set_editable (GTK_TEXT_VIEW (self->priv->comment_view), FALSE);
	gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (self->priv->comment_view), GTK_WRAP_WORD);
	gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (self->priv->comment_view), TRUE);

	gtk_widget_show (self->priv->comment_view);
	gtk_container_add (GTK_CONTAINER (self->priv->comment_win), self->priv->comment_view);
}


static void
gth_file_properties_gth_multipage_child_interface_init (GthMultipageChildInterface *iface)
{
	iface->get_name = gth_file_properties_real_get_name;
	iface->get_icon = gth_file_properties_real_get_icon;
}


static void
gth_file_properties_gth_property_view_interface_init (GthPropertyViewInterface *iface)
{
	iface->set_file = gth_file_properties_real_set_file;
}
