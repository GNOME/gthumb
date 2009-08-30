/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2008 Free Software Foundation, Inc.
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
#include <gdk/gdkkeysyms.h>
#include "gth-icon-view.h"


#define IMAGE_TEXT_SPACING 5
#define ICON_SPACING 12


static gpointer               gth_icon_view_parent_class = NULL;
static GthFileViewIface      *gth_icon_view_gth_file_view_parent_iface = NULL;
static GthFileSelectionIface *gth_icon_view_gth_file_selection_parent_iface = NULL;


void
gth_icon_view_real_set_model (GthFileView  *self,
			      GtkTreeModel *model)
{
	gtk_icon_view_set_model (GTK_ICON_VIEW (self), model);
}


GtkTreeModel *
gth_icon_view_real_get_model (GthFileView *self)
{
	return gtk_icon_view_get_model (GTK_ICON_VIEW (self));
}


static void
gth_icon_view_real_set_view_mode (GthFileView *base,
				  GthViewMode  mode)
{
	GthIconView * self;
	self = GTH_ICON_VIEW (base);
}


static GthViewMode
gth_icon_view_real_get_view_mode (GthFileView *base)
{
	GthIconView * self;
	self = GTH_ICON_VIEW (base);
	return GTH_VIEW_MODE_NONE;
}


static void
gth_icon_view_real_scroll_to (GthFileView *base,
			      int          pos,
			      double       yalign)
{
	GthIconView *self = GTH_ICON_VIEW (base);
	GtkTreePath *path;

	path = gtk_tree_path_new_from_indices (pos, -1);
	gtk_icon_view_scroll_to_path (GTK_ICON_VIEW (self),
				      path,
				      TRUE,
				      0.5,
				      yalign);

	gtk_tree_path_free (path);
}


static GthVisibility
gth_icon_view_real_get_visibility (GthFileView *base,
				   int          pos)
{
	GthIconView *self = GTH_ICON_VIEW (base);
	GtkTreePath *start_path, *end_path;
	int          start_pos, end_pos;

	if (! gtk_icon_view_get_visible_range (GTK_ICON_VIEW (self), &start_path, &end_path))
		return -1;

	start_pos = gtk_tree_path_get_indices (start_path)[0];
	end_pos = gtk_tree_path_get_indices (end_path)[0];

	gtk_tree_path_free (start_path);
	gtk_tree_path_free (end_path);

	return ((pos >= start_pos) && (pos <= end_pos)) ? GTH_VISIBILITY_FULL : GTH_VISIBILITY_NONE;
}


static int
gth_icon_view_real_get_at_position (GthFileView *base,
				    int          x,
				    int          y)
{
	GthIconView * self;
	self = GTH_ICON_VIEW (base);
	return -1;
}


static int
gth_icon_view_real_get_first_visible (GthFileView *base)
{
	GthIconView *self = GTH_ICON_VIEW (base);
	GtkTreePath *start_path;
	int          pos;

	if (! gtk_icon_view_get_visible_range (GTK_ICON_VIEW (self), &start_path, NULL))
		return -1;

	pos = gtk_tree_path_get_indices (start_path)[0];
	gtk_tree_path_free (start_path);

	return pos;
}


static int
gth_icon_view_real_get_last_visible (GthFileView *base)
{
	GthIconView *self = GTH_ICON_VIEW (base);
	GtkTreePath *end_path;
	int          pos;

	if (! gtk_icon_view_get_visible_range (GTK_ICON_VIEW (self), NULL, &end_path))
		return -1;

	pos = gtk_tree_path_get_indices (end_path)[0];
	gtk_tree_path_free (end_path);

	return pos;
}


static void
gth_icon_view_real_activated (GthFileView *base,
			      int          pos)
{
	GthIconView * self;
	self = GTH_ICON_VIEW (base);
	g_return_if_fail (pos >= 0);
}


static void
gth_icon_view_real_set_cursor (GthFileView *base,
			       int          pos)
{
	GthIconView *self;
	GtkTreePath *path;

	self = GTH_ICON_VIEW (base);
	g_return_if_fail (pos >= 0);

	path = gtk_tree_path_new_from_indices (pos, -1);
	gtk_icon_view_set_cursor (GTK_ICON_VIEW (base), path, NULL, FALSE);

	gtk_tree_path_free (path);
}


static int
gth_icon_view_real_get_cursor (GthFileView *base)
{
	GthIconView * self;
	self = GTH_ICON_VIEW (base);
	return -1;
}


static void
gth_icon_view_enable_drag_source (GthFileView          *self,
				  GdkModifierType       start_button_mask,
				  const GtkTargetEntry *targets,
				  int                   n_targets,
				  GdkDragAction         actions)
{
	gtk_icon_view_enable_model_drag_source (GTK_ICON_VIEW (self),
						start_button_mask,
						targets,
						n_targets,
						actions);
}


static void
gth_icon_view_unset_drag_source (GthFileView *self)
{
	gtk_icon_view_unset_model_drag_source (GTK_ICON_VIEW (self));
}


static void
gth_icon_view_enable_drag_dest (GthFileView          *self,
				const GtkTargetEntry *targets,
				int                   n_targets,
				GdkDragAction         actions)
{
	gtk_icon_view_enable_model_drag_dest (GTK_ICON_VIEW (self),
					      targets,
					      n_targets,
					      actions);
}


static void
gth_icon_view_unset_drag_dest (GthFileView *self)
{
	gtk_icon_view_unset_model_drag_dest (GTK_ICON_VIEW (self));
}


static void
gth_icon_view_set_drag_dest_pos (GthFileView    *self,
				 GdkDragContext *context,
				 int             x,
				 int             y,
				 guint           time,
				 int            *pos)
{
	GtkTreePath             *path = NULL;
	GtkIconViewDropPosition  drop_pos;

	if ((x >= 0) && (y >= 0) && gtk_icon_view_get_dest_item_at_pos (GTK_ICON_VIEW (self), x, y, &path, &drop_pos)) {
		if (pos != NULL) {
			int *indices;

			indices = gtk_tree_path_get_indices (path);
			*pos = indices[0];
			if ((drop_pos == GTK_ICON_VIEW_DROP_INTO)
			    || (drop_pos == GTK_ICON_VIEW_DROP_ABOVE)
			    || (drop_pos == GTK_ICON_VIEW_DROP_BELOW))
			{
				drop_pos = GTK_ICON_VIEW_DROP_LEFT;
			}
			if (drop_pos == GTK_ICON_VIEW_DROP_RIGHT)
				*pos = *pos + 1;
		}
		gtk_icon_view_set_drag_dest_item (GTK_ICON_VIEW (self), path, drop_pos);
	}
	else {
		if (pos != NULL)
			*pos = -1;
		gtk_icon_view_set_drag_dest_item (GTK_ICON_VIEW (self), NULL, GTK_ICON_VIEW_NO_DROP);
	}

	if (path != NULL)
		gtk_tree_path_free (path);
}


static void
gth_icon_view_real_set_selection_mode (GthFileSelection *base,
				       GtkSelectionMode  mode)
{
	gtk_icon_view_set_selection_mode (GTK_ICON_VIEW (base), mode);
}


static GList *
gth_icon_view_real_get_selected (GthFileSelection *base)
{
	return gtk_icon_view_get_selected_items (GTK_ICON_VIEW (base));
}


static void
gth_icon_view_real_select (GthFileSelection *base,
			   int               pos)
{
	GtkTreePath *path;

	path = gtk_tree_path_new_from_indices (pos, -1);
	gtk_icon_view_select_path (GTK_ICON_VIEW (base), path);

	gtk_tree_path_free (path);
}


static void
gth_icon_view_real_unselect (GthFileSelection *base,
			     int               pos)
{
	GtkTreePath *path;

	path = gtk_tree_path_new_from_indices (pos, -1);
	gtk_icon_view_unselect_path (GTK_ICON_VIEW (base), path);

	gtk_tree_path_free (path);
}


static void
gth_icon_view_real_select_all (GthFileSelection *base)
{
	gtk_icon_view_select_all (GTK_ICON_VIEW (base));
}


static void
gth_icon_view_real_unselect_all (GthFileSelection *base)
{
	gtk_icon_view_unselect_all (GTK_ICON_VIEW (base));
}


static gboolean
gth_icon_view_real_is_selected (GthFileSelection *base,
				int               pos)
{
	GthIconView * self;
	self = GTH_ICON_VIEW (base);

	return FALSE;
}


static GtkTreePath *
gth_icon_view_real_get_first_selected (GthFileSelection *base)
{
	GthIconView * self;
	self = GTH_ICON_VIEW (base);
	return NULL;
}


static GtkTreePath *
gth_icon_view_real_get_last_selected (GthFileSelection *base)
{
	GthIconView * self;
	self = GTH_ICON_VIEW (base);
	return NULL;
}


static guint
gth_icon_view_real_get_n_selected (GthFileSelection *base)
{
	GthIconView *self;
	GList       *selected;
	guint          n_selected;

	self = GTH_ICON_VIEW (base);

	selected = gtk_icon_view_get_selected_items (GTK_ICON_VIEW (self));
	n_selected = (guint) g_list_length (selected);

	g_list_foreach (selected, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (selected);

	return n_selected;
}


GtkWidget *
gth_icon_view_new (void)
{
	return g_object_new (GTH_TYPE_ICON_VIEW, NULL);
}


GtkWidget *
gth_icon_view_new_with_model (GtkTreeModel *model)
{
	return g_object_new (GTH_TYPE_ICON_VIEW, "model", model, NULL);
}


static void
gtk_icon_view_add_move_binding (GtkBindingSet  *binding_set,
				guint           keyval,
				guint           modmask,
				GtkMovementStep step,
				gint            count)
{
	gtk_binding_entry_add_signal (binding_set, keyval, modmask,
				      "move_cursor", 2,
				      G_TYPE_ENUM, step,
				      G_TYPE_INT, count);
	gtk_binding_entry_add_signal (binding_set, keyval, GDK_SHIFT_MASK,
				      "move_cursor", 2,
				      G_TYPE_ENUM, step,
				      G_TYPE_INT, count);

	if ((modmask & GDK_CONTROL_MASK) == GDK_CONTROL_MASK)
		return;

	gtk_binding_entry_add_signal (binding_set, keyval, GDK_CONTROL_MASK | GDK_SHIFT_MASK,
				      "move_cursor", 2,
				      G_TYPE_ENUM, step,
				      G_TYPE_INT, count);
	gtk_binding_entry_add_signal (binding_set, keyval, GDK_CONTROL_MASK,
				      "move_cursor", 2,
				      G_TYPE_ENUM, step,
				      G_TYPE_INT, count);
}


static void
gth_icon_view_class_init (GthIconViewClass *klass)
{
	GtkBindingSet *binding_set;

	gth_icon_view_parent_class = g_type_class_peek_parent (klass);

	binding_set = gtk_binding_set_by_class (klass);

	gtk_icon_view_add_move_binding (binding_set, GDK_Right, 0,
					GTK_MOVEMENT_LOGICAL_POSITIONS, 1);
	gtk_icon_view_add_move_binding (binding_set, GDK_Left, 0,
					GTK_MOVEMENT_LOGICAL_POSITIONS, -1);
}


static void
gth_icon_view_init (GthIconView *icon_view)
{
	gtk_icon_view_set_spacing (GTK_ICON_VIEW (icon_view), IMAGE_TEXT_SPACING);
	gtk_icon_view_set_margin (GTK_ICON_VIEW (icon_view), ICON_SPACING);
	gtk_icon_view_set_column_spacing (GTK_ICON_VIEW (icon_view), ICON_SPACING);
	gtk_icon_view_set_row_spacing (GTK_ICON_VIEW (icon_view), ICON_SPACING);
	gtk_icon_view_set_selection_mode (GTK_ICON_VIEW (icon_view), GTK_SELECTION_MULTIPLE);
}


static void
gth_icon_view_gth_file_view_interface_init (GthFileViewIface *iface)
{
	gth_icon_view_gth_file_view_parent_iface = g_type_interface_peek_parent (iface);
	iface->set_model = gth_icon_view_real_set_model;
	iface->get_model = gth_icon_view_real_get_model;
	iface->set_view_mode = gth_icon_view_real_set_view_mode;
	iface->get_view_mode = gth_icon_view_real_get_view_mode;
	iface->scroll_to = gth_icon_view_real_scroll_to;
	iface->get_visibility = gth_icon_view_real_get_visibility;
	iface->get_at_position = gth_icon_view_real_get_at_position;
	iface->get_first_visible = gth_icon_view_real_get_first_visible;
	iface->get_last_visible = gth_icon_view_real_get_last_visible;
	iface->activated = gth_icon_view_real_activated;
	iface->set_cursor = gth_icon_view_real_set_cursor;
	iface->get_cursor = gth_icon_view_real_get_cursor;
	iface->enable_drag_source = gth_icon_view_enable_drag_source;
	iface->unset_drag_source = gth_icon_view_unset_drag_source;
	iface->enable_drag_dest = gth_icon_view_enable_drag_dest;
	iface->unset_drag_dest = gth_icon_view_unset_drag_dest;
	iface->set_drag_dest_pos = gth_icon_view_set_drag_dest_pos;
}


static void
gth_icon_view_gth_file_selection_interface_init (GthFileSelectionIface *iface)
{
	gth_icon_view_gth_file_selection_parent_iface = g_type_interface_peek_parent (iface);
	iface->set_selection_mode = gth_icon_view_real_set_selection_mode;
	iface->get_selected = gth_icon_view_real_get_selected;
	iface->select = gth_icon_view_real_select;
	iface->unselect = gth_icon_view_real_unselect;
	iface->select_all = gth_icon_view_real_select_all;
	iface->unselect_all = gth_icon_view_real_unselect_all;
	iface->is_selected = gth_icon_view_real_is_selected;
	iface->get_first_selected = gth_icon_view_real_get_first_selected;
	iface->get_last_selected = gth_icon_view_real_get_last_selected;
	iface->get_n_selected = gth_icon_view_real_get_n_selected;
}


GType
gth_icon_view_get_type (void)
{
	static GType type = 0;

	if (type == 0) {
		static const GTypeInfo g_define_type_info = {
			sizeof (GthIconViewClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) gth_icon_view_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (GthIconView),
			0,
			(GInstanceInitFunc) gth_icon_view_init,
			NULL
		};
		static const GInterfaceInfo gth_file_view_info = {
			(GInterfaceInitFunc) gth_icon_view_gth_file_view_interface_init,
			(GInterfaceFinalizeFunc) NULL,
			NULL
		};
		static const GInterfaceInfo gth_file_selection_info = {
			(GInterfaceInitFunc) gth_icon_view_gth_file_selection_interface_init,
			(GInterfaceFinalizeFunc) NULL,
			NULL
		};
		type = g_type_register_static (GTK_TYPE_ICON_VIEW,
					       "GthIconView",
					       &g_define_type_info,
					       0);
		g_type_add_interface_static (type, GTH_TYPE_FILE_VIEW, &gth_file_view_info);
		g_type_add_interface_static (type, GTH_TYPE_FILE_SELECTION, &gth_file_selection_info);
	}

	return type;
}
