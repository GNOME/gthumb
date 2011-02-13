/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2011 Free Software Foundation, Inc.
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
#include <gdk/gdkkeysyms.h>
#include "gth-list-view.h"


#define IMAGE_TEXT_SPACING 0
#define DEFAULT_ICON_SPACING 12
#define SIZE_REQUEST 50


struct _GthListViewPrivate {
	GtkTreeViewColumn *column;
};


static gpointer               parent_class = NULL;
static GthFileViewIface      *gth_list_view_gth_file_view_parent_iface = NULL;
static GthFileSelectionIface *gth_list_view_gth_file_selection_parent_iface = NULL;


void
gth_list_view_real_set_model (GthFileView  *self,
			      GtkTreeModel *model)
{
	gtk_tree_view_set_model (GTK_TREE_VIEW (self), model);
}


GtkTreeModel *
gth_list_view_real_get_model (GthFileView *self)
{
	return gtk_tree_view_get_model (GTK_TREE_VIEW (self));
}


static void
gth_list_view_real_scroll_to (GthFileView *base,
			      int          pos,
			      double       yalign)
{
	GtkTreePath *path;

	path = gtk_tree_path_new_from_indices (pos, -1);
	gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (base),
				      path,
				      NULL,
				      TRUE,
				      yalign,
				      0.0);

	gtk_tree_path_free (path);
}


static GthVisibility
gth_list_view_real_get_visibility (GthFileView *base,
				   int          pos)
{
	GtkTreePath *start_path, *end_path;
	int          start_pos, end_pos;

	if (! gtk_tree_view_get_visible_range (GTK_TREE_VIEW (base), &start_path, &end_path))
		return -1;

	start_pos = gtk_tree_path_get_indices (start_path)[0];
	end_pos = gtk_tree_path_get_indices (end_path)[0];

	gtk_tree_path_free (start_path);
	gtk_tree_path_free (end_path);

	return ((pos >= start_pos) && (pos <= end_pos)) ? GTH_VISIBILITY_FULL : GTH_VISIBILITY_NONE;
}


static int
gth_list_view_real_get_at_position (GthFileView *base,
				    int          x,
				    int          y)
{
	GtkTreePath *path;
	int          pos;

	if (! gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (base),
					     x,
					     y,
					     &path,
					     NULL,
					     NULL,
					     NULL))
	{
		return -1;
	}
	pos = gtk_tree_path_get_indices (path)[0];

	gtk_tree_path_free (path);

	return pos;
}


static int
gth_list_view_real_get_first_visible (GthFileView *base)
{
	GthListView *self = GTH_LIST_VIEW (base);
	GtkTreePath *start_path;
	int          pos;

	if (! gtk_tree_view_get_visible_range (GTK_TREE_VIEW (self), &start_path, NULL))
		return -1;

	pos = gtk_tree_path_get_indices (start_path)[0];
	gtk_tree_path_free (start_path);

	return pos;
}


static int
gth_list_view_real_get_last_visible (GthFileView *base)
{
	GthListView *self = GTH_LIST_VIEW (base);
	GtkTreePath *end_path;
	int          pos;

	if (! gtk_tree_view_get_visible_range (GTK_TREE_VIEW (self), NULL, &end_path))
		return -1;

	pos = gtk_tree_path_get_indices (end_path)[0];
	gtk_tree_path_free (end_path);

	return pos;
}


static void
gth_list_view_real_activated (GthFileView *base,
			      int          pos)
{
	GthListView *self = GTH_LIST_VIEW (base);
	GtkTreePath *path;

	g_return_if_fail (pos >= 0);

	path = gtk_tree_path_new_from_indices (pos, -1);
	gtk_tree_view_row_activated (GTK_TREE_VIEW (self), path, NULL);

	gtk_tree_path_free (path);
}


static void
gth_list_view_real_set_cursor (GthFileView *base,
			       int          pos)
{
	GtkTreePath *path;

	g_return_if_fail (pos >= 0);

	path = gtk_tree_path_new_from_indices (pos, -1);
	gtk_tree_view_set_cursor (GTK_TREE_VIEW (base), path, NULL, FALSE);

	gtk_tree_path_free (path);
}


static int
gth_list_view_real_get_cursor (GthFileView *base)
{
	GtkTreePath *path;
	int          pos;

	gtk_tree_view_get_cursor (GTK_TREE_VIEW (base), &path, NULL);
	if (path == NULL)
		return -1;
	pos = gtk_tree_path_get_indices (path)[0];

	gtk_tree_path_free (path);

	return pos;
}


void
gth_list_view_real_set_spacing (GthFileView *self,
			        int          spacing)
{
	/* FIXME
	gtk_tree_view_set_margin (GTK_TREE_VIEW (self), spacing);
	gtk_tree_view_set_column_spacing (GTK_TREE_VIEW (self), spacing);
	gtk_tree_view_set_row_spacing (GTK_TREE_VIEW (self), spacing);
	*/
}


static void
gth_list_view_enable_drag_source (GthFileView          *base,
				  GdkModifierType       start_button_mask,
				  const GtkTargetEntry *targets,
				  int                   n_targets,
				  GdkDragAction         actions)
{
	GthListView *self = GTH_LIST_VIEW (base);

	gtk_tree_view_enable_model_drag_source (GTK_TREE_VIEW (self),
						start_button_mask,
						targets,
						n_targets,
						actions);
}


static void
gth_list_view_unset_drag_source (GthFileView *base)
{
	GthListView *self = GTH_LIST_VIEW (base);

	gtk_tree_view_unset_rows_drag_source (GTK_TREE_VIEW (self));
}


static void
gth_list_view_enable_drag_dest (GthFileView          *self,
				const GtkTargetEntry *targets,
				int                   n_targets,
				GdkDragAction         actions)
{
	gtk_tree_view_enable_model_drag_dest (GTK_TREE_VIEW (self),
					      targets,
					      n_targets,
					      actions);
}


static void
gth_list_view_unset_drag_dest (GthFileView *self)
{
	gtk_tree_view_unset_rows_drag_dest (GTK_TREE_VIEW (self));
}


static void
gth_list_view_set_drag_dest_pos (GthFileView    *self,
				 GdkDragContext *context,
				 int             x,
				 int             y,
				 guint           time,
				 int            *pos)
{
	GtkTreePath             *path = NULL;
	GtkTreeViewDropPosition  drop_pos;

	if ((x >= 0)
	    && (y >= 0)
	    && gtk_tree_view_get_dest_row_at_pos (GTK_TREE_VIEW (self),
			    	    	    	  x,
			    	    	    	  y,
			    	    	    	  &path,
			    	    	    	  &drop_pos))
	{
		if (pos != NULL) {
			int *indices;

			indices = gtk_tree_path_get_indices (path);
			*pos = indices[0];
			if (drop_pos == GTK_TREE_VIEW_DROP_INTO_OR_AFTER)
				drop_pos = GTK_TREE_VIEW_DROP_AFTER;
			if (drop_pos == GTK_TREE_VIEW_DROP_AFTER)
				*pos = *pos + 1;
		}
		gtk_tree_view_set_drag_dest_row (GTK_TREE_VIEW (self), path, drop_pos);
	}
	else {
		if (pos != NULL)
			*pos = -1;
		gtk_tree_view_set_drag_dest_row (GTK_TREE_VIEW (self), NULL, 0);
	}

	if (path != NULL)
		gtk_tree_path_free (path);
}


static GtkCellLayout *
gth_list_view_add_renderer (GthFileView              *base,
			    GthFileViewRendererType   renderer_type,
			    GtkCellRenderer          *renderer)
{
	GthListView *self = GTH_LIST_VIEW (base);

	switch (renderer_type) {
	case GTH_FILE_VIEW_RENDERER_CHECKBOX:
		gtk_tree_view_column_pack_start (self->priv->column, renderer, FALSE);
		break;
	case GTH_FILE_VIEW_RENDERER_THUMBNAIL:
		gtk_tree_view_column_pack_start (self->priv->column, renderer, FALSE);
		break;
	case GTH_FILE_VIEW_RENDERER_TEXT:
		gtk_tree_view_column_pack_start (self->priv->column, renderer, TRUE);
		break;
	}

	return GTK_CELL_LAYOUT (self->priv->column);
}


static void
gth_list_view_update_attributes (GthFileView     *base,
				 GtkCellRenderer *checkbox_renderer,
				 GtkCellRenderer *thumbnail_renderer,
				 GtkCellRenderer *text_renderer,
				 int              thumb_size)
{
	GthListView *self = GTH_LIST_VIEW (base);

	g_object_set (thumbnail_renderer,
		      "size", thumb_size,
		      "yalign", 0.5,
		      NULL);
	g_object_set (text_renderer,
		      "ellipsize", PANGO_ELLIPSIZE_END,
		      "yalign", 0.0,
		      "alignment", PANGO_ALIGN_LEFT,
		      NULL);

	gtk_tree_view_column_queue_resize (self->priv->column);
}


static gboolean
gth_list_view_truncate_metadata (GthFileView *base)
{
	return FALSE;
}


static void
gth_list_view_real_set_selection_mode (GthFileSelection *base,
				       GtkSelectionMode  mode)
{
	gtk_tree_selection_set_mode (gtk_tree_view_get_selection (GTK_TREE_VIEW (base)), mode);
}


static GList *
gth_list_view_real_get_selected (GthFileSelection *base)
{
	return gtk_tree_selection_get_selected_rows (gtk_tree_view_get_selection (GTK_TREE_VIEW (base)), NULL);
}


static void
gth_list_view_real_select (GthFileSelection *base,
			   int               pos)
{
	GtkTreePath *path;

	path = gtk_tree_path_new_from_indices (pos, -1);
	gtk_tree_selection_select_path (gtk_tree_view_get_selection (GTK_TREE_VIEW (base)), path);

	gtk_tree_path_free (path);
}


static void
gth_list_view_real_unselect (GthFileSelection *base,
			     int               pos)
{
	GtkTreePath *path;

	path = gtk_tree_path_new_from_indices (pos, -1);
	gtk_tree_selection_unselect_path (gtk_tree_view_get_selection (GTK_TREE_VIEW (base)), path);

	gtk_tree_path_free (path);
}


static void
gth_list_view_real_select_all (GthFileSelection *base)
{
	gtk_tree_selection_select_all (gtk_tree_view_get_selection (GTK_TREE_VIEW (base)));
}


static void
gth_list_view_real_unselect_all (GthFileSelection *base)
{
	gtk_tree_selection_unselect_all (gtk_tree_view_get_selection (GTK_TREE_VIEW (base)));
}


static gboolean
gth_list_view_real_is_selected (GthFileSelection *base,
				int               pos)
{
	GtkTreePath *path;
	gboolean     result;

	path = gtk_tree_path_new ();
	gtk_tree_path_append_index (path, pos);
	result = gtk_tree_selection_path_is_selected (gtk_tree_view_get_selection (GTK_TREE_VIEW (base)), path);

	gtk_tree_path_free (path);

	return result;
}


static GtkTreePath *
gth_list_view_real_get_first_selected (GthFileSelection *base)
{
	GList       *list;
	GtkTreePath *path;

	list = gtk_tree_selection_get_selected_rows (gtk_tree_view_get_selection (GTK_TREE_VIEW (base)), NULL);
	if (list != NULL) {
		list = g_list_sort (list, (GCompareFunc) gtk_tree_path_compare);
		path = gtk_tree_path_copy ((GtkTreePath *) g_list_first (list)->data);
	}
	else
		path = NULL;

	g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (list);

	return path;
}


static GtkTreePath *
gth_list_view_real_get_last_selected (GthFileSelection *base)
{
	GList       *list;
	GtkTreePath *path;

	list = gtk_tree_selection_get_selected_rows (gtk_tree_view_get_selection (GTK_TREE_VIEW (base)), NULL);
	if (list != NULL) {
		list = g_list_sort (list, (GCompareFunc) gtk_tree_path_compare);
		path = gtk_tree_path_copy ((GtkTreePath *) g_list_last (list)->data);
	}
	else
		path = NULL;

	g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (list);

	return path;
}


static guint
gth_list_view_real_get_n_selected (GthFileSelection *base)
{
	GthListView *self;
	GList       *selected;
	guint          n_selected;

	self = GTH_LIST_VIEW (base);

	selected = gtk_tree_selection_get_selected_rows (gtk_tree_view_get_selection (GTK_TREE_VIEW (base)), NULL);
	n_selected = (guint) g_list_length (selected);

	g_list_foreach (selected, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (selected);

	return n_selected;
}


GtkWidget *
gth_list_view_new (void)
{
	return g_object_new (GTH_TYPE_LIST_VIEW, NULL);
}


GtkWidget *
gth_list_view_new_with_model (GtkTreeModel *model)
{
	return g_object_new (GTH_TYPE_LIST_VIEW, "model", model, NULL);
}


static void
gth_list_view_finalize (GObject *object)
{
	/*GthListView *self = GTH_LIST_VIEW (object);*/

	/* FIXME */

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gth_list_view_class_init (GthListViewClass *klass)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GthListViewPrivate));

	object_class = (GObjectClass*) klass;
	object_class->finalize = gth_list_view_finalize;
}


static void
list_view_selection_changed_cb (GtkTreeSelection *treeselection,
				gpointer          user_data)
{
	gth_file_selection_changed (GTH_FILE_SELECTION (user_data));
}


static void
list_view_row_activated_cb (GtkTreeView       *tree_view,
			    GtkTreePath       *path,
			    GtkTreeViewColumn *column,
			    gpointer           user_data)
{
	gth_file_view_activate_file (GTH_FILE_VIEW (tree_view), path);
}


static void
gth_list_view_init (GthListView *list_view)
{
	list_view->priv = G_TYPE_INSTANCE_GET_PRIVATE (list_view, GTH_TYPE_LIST_VIEW, GthListViewPrivate);

	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (list_view), FALSE);

	list_view->priv->column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_expand (list_view->priv->column, TRUE);
	gtk_tree_view_append_column (GTK_TREE_VIEW (list_view), list_view->priv->column);

	gtk_widget_set_size_request (GTK_WIDGET (list_view),
				     SIZE_REQUEST,
				     SIZE_REQUEST);
	gtk_tree_selection_set_mode (gtk_tree_view_get_selection (GTK_TREE_VIEW (list_view)),
				     GTK_SELECTION_MULTIPLE);
	g_signal_connect (gtk_tree_view_get_selection (GTK_TREE_VIEW (list_view)),
			  "changed",
			  G_CALLBACK (list_view_selection_changed_cb),
			  list_view);
	g_signal_connect (GTK_TREE_VIEW (list_view),
			  "row-activated",
			  G_CALLBACK (list_view_row_activated_cb),
			  list_view);
}


static void
gth_list_view_gth_file_view_interface_init (GthFileViewIface *iface)
{
	gth_list_view_gth_file_view_parent_iface = g_type_interface_peek_parent (iface);
	iface->set_model = gth_list_view_real_set_model;
	iface->get_model = gth_list_view_real_get_model;
	iface->scroll_to = gth_list_view_real_scroll_to;
	iface->get_visibility = gth_list_view_real_get_visibility;
	iface->get_at_position = gth_list_view_real_get_at_position;
	iface->get_first_visible = gth_list_view_real_get_first_visible;
	iface->get_last_visible = gth_list_view_real_get_last_visible;
	iface->activated = gth_list_view_real_activated;
	iface->set_cursor = gth_list_view_real_set_cursor;
	iface->get_cursor = gth_list_view_real_get_cursor;
	iface->set_spacing = gth_list_view_real_set_spacing;
	iface->enable_drag_source = gth_list_view_enable_drag_source;
	iface->unset_drag_source = gth_list_view_unset_drag_source;
	iface->enable_drag_dest = gth_list_view_enable_drag_dest;
	iface->unset_drag_dest = gth_list_view_unset_drag_dest;
	iface->set_drag_dest_pos = gth_list_view_set_drag_dest_pos;
	iface->add_renderer = gth_list_view_add_renderer;
	iface->update_attributes = gth_list_view_update_attributes;
	iface->truncate_metadata = gth_list_view_truncate_metadata;
}


static void
gth_list_view_gth_file_selection_interface_init (GthFileSelectionIface *iface)
{
	gth_list_view_gth_file_selection_parent_iface = g_type_interface_peek_parent (iface);
	iface->set_selection_mode = gth_list_view_real_set_selection_mode;
	iface->get_selected = gth_list_view_real_get_selected;
	iface->select = gth_list_view_real_select;
	iface->unselect = gth_list_view_real_unselect;
	iface->select_all = gth_list_view_real_select_all;
	iface->unselect_all = gth_list_view_real_unselect_all;
	iface->is_selected = gth_list_view_real_is_selected;
	iface->get_first_selected = gth_list_view_real_get_first_selected;
	iface->get_last_selected = gth_list_view_real_get_last_selected;
	iface->get_n_selected = gth_list_view_real_get_n_selected;
}


GType
gth_list_view_get_type (void)
{
	static GType type = 0;

	if (type == 0) {
		static const GTypeInfo g_define_type_info = {
			sizeof (GthListViewClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) gth_list_view_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (GthListView),
			0,
			(GInstanceInitFunc) gth_list_view_init,
			NULL
		};
		static const GInterfaceInfo gth_file_view_info = {
			(GInterfaceInitFunc) gth_list_view_gth_file_view_interface_init,
			(GInterfaceFinalizeFunc) NULL,
			NULL
		};
		static const GInterfaceInfo gth_file_selection_info = {
			(GInterfaceInitFunc) gth_list_view_gth_file_selection_interface_init,
			(GInterfaceFinalizeFunc) NULL,
			NULL
		};
		type = g_type_register_static (GTK_TYPE_TREE_VIEW,
					       "GthListView",
					       &g_define_type_info,
					       0);
		g_type_add_interface_static (type, GTH_TYPE_FILE_VIEW, &gth_file_view_info);
		g_type_add_interface_static (type, GTH_TYPE_FILE_SELECTION, &gth_file_selection_info);
	}

	return type;
}
