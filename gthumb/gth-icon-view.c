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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <gdk/gdkkeysyms.h>
#include "gth-icon-view.h"


#define IMAGE_TEXT_SPACING 0
#define DEFAULT_ICON_SPACING 12
#define SIZE_REQUEST 50


struct _GthIconViewPrivate {
	/* selection */

	int              selection_range_start;
	gboolean         selection_pending;
	int              selection_pending_pos;

	/* drag-and-drop */

	gboolean         drag_source_enabled;
	GdkModifierType  drag_start_button_mask;
	GtkTargetList   *drag_target_list;
	GdkDragAction    drag_actions;

	gboolean         dragging : 1;        /* Whether the user is dragging items. */
	int              drag_button;
	gboolean         drag_started : 1;    /* Whether the drag has started. */
	int              drag_start_x;        /* The position where the drag started. */
	int              drag_start_y;
};


static gpointer               parent_class = NULL;
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
gth_icon_view_real_scroll_to (GthFileView *base,
			      int          pos,
			      double       yalign)
{
	GtkTreePath *path;

	path = gtk_tree_path_new_from_indices (pos, -1);
	gtk_icon_view_scroll_to_path (GTK_ICON_VIEW (base),
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
	GtkTreePath *start_path, *end_path;
	int          start_pos, end_pos;

	if (! gtk_icon_view_get_visible_range (GTK_ICON_VIEW (base), &start_path, &end_path))
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
	GtkTreePath *path;
	int          pos;

	path = gtk_icon_view_get_path_at_pos (GTK_ICON_VIEW (base), x, y);
	if (path == NULL)
		return -1;
	pos = gtk_tree_path_get_indices (path)[0];

	gtk_tree_path_free (path);

	return pos;
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
	GthIconView *self = GTH_ICON_VIEW (base);
	GtkTreePath *path;

	g_return_if_fail (pos >= 0);

	path = gtk_tree_path_new_from_indices (pos, -1);
	gtk_icon_view_item_activated (GTK_ICON_VIEW (self), path);

	gtk_tree_path_free (path);
}


static void
gth_icon_view_real_set_cursor (GthFileView *base,
			       int          pos)
{
	GtkTreePath *path;

	g_return_if_fail (pos >= 0);

	path = gtk_tree_path_new_from_indices (pos, -1);
	gtk_icon_view_set_cursor (GTK_ICON_VIEW (base), path, NULL, FALSE);

	gtk_tree_path_free (path);
}


static int
gth_icon_view_real_get_cursor (GthFileView *base)
{
	GtkTreePath *path;
	int          pos;

	if (! gtk_icon_view_get_cursor (GTK_ICON_VIEW (base), &path, NULL))
		return -1;
	pos = gtk_tree_path_get_indices (path)[0];

	gtk_tree_path_free (path);

	return pos;
}


void
gth_icon_view_real_set_spacing (GthFileView *self,
			        int          spacing)
{
	gtk_icon_view_set_margin (GTK_ICON_VIEW (self), spacing);
	gtk_icon_view_set_column_spacing (GTK_ICON_VIEW (self), spacing);
	gtk_icon_view_set_row_spacing (GTK_ICON_VIEW (self), spacing);
}


static void
gth_icon_view_enable_drag_source (GthFileView          *base,
				  GdkModifierType       start_button_mask,
				  const GtkTargetEntry *targets,
				  int                   n_targets,
				  GdkDragAction         actions)
{
	GthIconView *self = GTH_ICON_VIEW (base);

	if (self->priv->drag_target_list != NULL)
		gtk_target_list_unref (self->priv->drag_target_list);

	self->priv->drag_source_enabled = TRUE;
	self->priv->drag_start_button_mask = start_button_mask;
	self->priv->drag_target_list = gtk_target_list_new (targets, n_targets);
	self->priv->drag_actions = actions;
}


static void
gth_icon_view_unset_drag_source (GthFileView *base)
{
	GthIconView *self = GTH_ICON_VIEW (base);

	self->priv->drag_source_enabled = FALSE;
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


static GtkCellLayout *
gth_icon_view_add_renderer (GthFileView             *self,
			    GthFileViewRendererType  renderer_type,
			    GtkCellRenderer         *renderer)
{
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (self), renderer, FALSE);

	return GTK_CELL_LAYOUT (self);
}


static void
gth_icon_view_update_attributes (GthFileView     *self,
				 GtkCellRenderer *checkbox_renderer,
				 GtkCellRenderer *thumbnail_renderer,
				 GtkCellRenderer *text_renderer,
				 int              thumb_size)
{
	g_object_set (thumbnail_renderer,
		      "size", thumb_size,
		      "yalign", 1.0,
		      NULL);
	g_object_set (text_renderer,
		      "yalign", 0.0,
		      "alignment", PANGO_ALIGN_CENTER,
		      "width", thumb_size + THUMBNAIL_BORDER,
		      "wrap-mode", PANGO_WRAP_WORD_CHAR,
		      "wrap-width", thumb_size + THUMBNAIL_BORDER,
		      NULL);
}


static gboolean
gth_icon_view_truncate_metadata (GthFileView *base)
{
	return TRUE;
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
	GtkTreePath *path;
	gboolean     result;

	path = gtk_tree_path_new ();
	gtk_tree_path_append_index (path, pos);
	result = gtk_icon_view_path_is_selected (GTK_ICON_VIEW (base), path);

	gtk_tree_path_free (path);

	return result;
}


static GtkTreePath *
gth_icon_view_real_get_first_selected (GthFileSelection *base)
{
	GList       *list;
	GtkTreePath *path;

	list = gtk_icon_view_get_selected_items (GTK_ICON_VIEW (base));
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
gth_icon_view_real_get_last_selected (GthFileSelection *base)
{
	GList       *list;
	GtkTreePath *path;

	list = gtk_icon_view_get_selected_items (GTK_ICON_VIEW (base));
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
gth_icon_view_finalize (GObject *object)
{
	GthIconView *self = GTH_ICON_VIEW (object);

	if (self->priv->drag_target_list != NULL) {
		gtk_target_list_unref (self->priv->drag_target_list);
		self->priv->drag_target_list = NULL;
	}

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gth_icon_view_class_init (GthIconViewClass *klass)
{
	GObjectClass   *object_class;
	GtkWidgetClass *widget_class;
	GtkBindingSet  *binding_set;

	parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GthIconViewPrivate));

	object_class = (GObjectClass*) klass;
	object_class->finalize = gth_icon_view_finalize;

	widget_class = (GtkWidgetClass*) klass;
	widget_class->drag_begin = NULL;

	binding_set = gtk_binding_set_by_class (klass);

	gtk_icon_view_add_move_binding (binding_set, GDK_Right, 0,
					GTK_MOVEMENT_LOGICAL_POSITIONS, 1);
	gtk_icon_view_add_move_binding (binding_set, GDK_Left, 0,
					GTK_MOVEMENT_LOGICAL_POSITIONS, -1);
}


static void
stop_dragging (GthIconView *icon_view)
{
	if (! icon_view->priv->dragging)
		return;
	icon_view->priv->dragging = FALSE;
	icon_view->priv->drag_started = FALSE;
}


static gboolean
icon_view_button_press_event_cb (GtkWidget      *widget,
				 GdkEventButton *event,
				 gpointer        user_data)
{
	GthIconView *icon_view = user_data;
	gboolean     retval = FALSE;

	if ((event->button == 1) && (event->type == GDK_2BUTTON_PRESS)) {
		GtkTreePath *path;

		path = gtk_icon_view_get_path_at_pos (GTK_ICON_VIEW (icon_view), event->x, event->y);
		if (path != NULL) {
			if (! (event->state & GDK_CONTROL_MASK) && ! (event->state & GDK_SHIFT_MASK)) {
				stop_dragging (icon_view);
				icon_view->priv->selection_pending = FALSE;

				gtk_icon_view_item_activated (GTK_ICON_VIEW (icon_view), path);
			}
			gtk_tree_path_free (path);
		}

		return TRUE;
	}

	if ((event->button == 2) && (event->type == GDK_BUTTON_PRESS)) {
		/* This can be the start of a dragging action. */

		if (! (event->state & GDK_CONTROL_MASK)
		    && ! (event->state & GDK_SHIFT_MASK)
		    && icon_view->priv->drag_source_enabled)
		{
			icon_view->priv->dragging = TRUE;
			icon_view->priv->drag_button = 2;
			icon_view->priv->drag_start_x = event->x;
			icon_view->priv->drag_start_y = event->y;
		}
	}

	if ((event->button == 1) && (event->type == GDK_BUTTON_PRESS)) {
		GtkTreePath *path;
		int          pos;
		int          new_selection_end;

		path = gtk_icon_view_get_path_at_pos (GTK_ICON_VIEW (icon_view), event->x, event->y);
		if (path == NULL) {
			if (event->state & GDK_SHIFT_MASK)
				return TRUE;
			else
				return FALSE;
		}

		/* This can be the start of a dragging action. */

		if (! (event->state & GDK_CONTROL_MASK)
		    && ! (event->state & GDK_SHIFT_MASK)
		    && icon_view->priv->drag_source_enabled)
		{
			icon_view->priv->dragging = TRUE;
			icon_view->priv->drag_button = 1;
			icon_view->priv->drag_start_x = event->x;
			icon_view->priv->drag_start_y = event->y;
		}

		/* Selection */

		pos = gtk_tree_path_get_indices (path)[0];

		if (icon_view->priv->drag_source_enabled
		    && ! (event->state & GDK_CONTROL_MASK)
		    && ! (event->state & GDK_SHIFT_MASK)
		    && gth_file_selection_is_selected (GTH_FILE_SELECTION (icon_view), pos))
		{
			icon_view->priv->selection_pending = TRUE;
			icon_view->priv->selection_pending_pos = pos;
			retval = TRUE;
		}

		gtk_tree_path_free (path);
		path = NULL;

		new_selection_end = pos;
		if (event->state & GDK_SHIFT_MASK) {
			int    selection_start;
			int    selection_end;
			GList *list;
			GList *scan;
			int    i;

			selection_start = MIN (icon_view->priv->selection_range_start, new_selection_end);
			selection_end = MAX (new_selection_end, icon_view->priv->selection_range_start);

			/* unselect items out of the new range */

			list = gtk_icon_view_get_selected_items (GTK_ICON_VIEW (icon_view));
			for (scan = list; scan; scan = scan->next) {
				GtkTreePath *path = scan->data;
				int          pos = gtk_tree_path_get_indices (path)[0];

				if ((pos < selection_start) || (pos > selection_end)) {
					path = gtk_tree_path_new_from_indices (pos, -1);
					gtk_icon_view_unselect_path (GTK_ICON_VIEW (icon_view), path);

					gtk_tree_path_free (path);
				}
			}

			g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
			g_list_free (list);

			/* select the images in the range */

			for (i = selection_start; i <= selection_end; i++) {
				path = gtk_tree_path_new_from_indices (i, -1);
				gtk_icon_view_select_path (GTK_ICON_VIEW (icon_view), path);

				gtk_tree_path_free (path);
			}

			retval = TRUE;
		}
		else
			icon_view->priv->selection_range_start = new_selection_end;

		return retval;
	}

	return FALSE;
}


static gboolean
icon_view_motion_notify_event_cb (GtkWidget      *widget,
				  GdkEventButton *event,
				  gpointer        user_data)
{
	GthIconView *icon_view = user_data;

	if (! icon_view->priv->drag_source_enabled)
		return FALSE;

	if (icon_view->priv->dragging) {
		if (! icon_view->priv->drag_started
		    && gtk_drag_check_threshold (widget,
					         icon_view->priv->drag_start_x,
					         icon_view->priv->drag_start_y,
						 event->x,
						 event->y))
		{
			GtkTreePath    *path = NULL;
			GdkDragContext *context;
			GdkPixmap      *dnd_icon;
			int             width;
			int             height;
			int             n_selected;

			path = gtk_icon_view_get_path_at_pos (GTK_ICON_VIEW (icon_view),
							      event->x,
							      event->y);
			if (path == NULL)
				return FALSE;

			gtk_icon_view_set_cursor (GTK_ICON_VIEW (icon_view), path, NULL, FALSE);
			icon_view->priv->drag_started = TRUE;

			/**/

			context = gtk_drag_begin (widget,
						  icon_view->priv->drag_target_list,
						  icon_view->priv->drag_actions,
						  icon_view->priv->drag_button,
						  (GdkEvent *) event);
			if (icon_view->priv->drag_button == 2)
				context->suggested_action = GDK_ACTION_ASK;

			dnd_icon = gtk_icon_view_create_drag_icon (GTK_ICON_VIEW (icon_view), path);
			gdk_drawable_get_size (dnd_icon, &width, &height);

			n_selected =  gth_file_selection_get_n_selected (GTH_FILE_SELECTION (icon_view));
			if (n_selected >= 1) {
				const int  offset = 3;
				int        n_visible;
				int        border;
				GdkPixbuf *multi_dnd_icon;
				int        i;

				n_visible = MIN (n_selected, 4);
				gdk_drawable_get_size (dnd_icon, &width, &height);
				border = n_visible * offset;
				multi_dnd_icon = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, width + border, height + border);
				gdk_pixbuf_fill (multi_dnd_icon, 0x00000000);
				for (i = n_visible - 1; i >= 0; i--)
					gdk_pixbuf_get_from_drawable (multi_dnd_icon,
								      dnd_icon,
								      gdk_drawable_get_colormap (dnd_icon),
								      0, 0,
								      i * offset, i * offset,
								      width, height);
				gtk_drag_set_icon_pixbuf (context,
							  multi_dnd_icon,
							  width / 4,
							  height / 4);

				g_object_unref (multi_dnd_icon);
			}

			g_object_unref (dnd_icon);
			gtk_tree_path_free (path);
		}

		return TRUE;
	}

	return FALSE;
}


static gboolean
icon_view_button_release_event_cb (GtkWidget      *widget,
				   GdkEventButton *event,
				   gpointer        user_data)
{
	GthIconView *icon_view = user_data;

	if (icon_view->priv->dragging) {
		icon_view->priv->selection_pending = icon_view->priv->selection_pending && ! icon_view->priv->drag_started;
		stop_dragging (icon_view);
	}

	if (icon_view->priv->selection_pending) {
		gth_file_selection_unselect_all (GTH_FILE_SELECTION (icon_view));
		gth_file_selection_select (GTH_FILE_SELECTION (icon_view), icon_view->priv->selection_pending_pos);
		icon_view->priv->selection_pending = FALSE;
	}

	return FALSE;
}


static void
icon_view_selection_changed_cb (GtkIconView *widget,
				gpointer     user_data)
{
	GthIconView *icon_view = user_data;
	GList       *list;

	list = gtk_icon_view_get_selected_items (GTK_ICON_VIEW (icon_view));
	if ((list != NULL) && (list->next == NULL)) {
		GtkTreePath *path = list->data;
		icon_view->priv->selection_range_start = gtk_tree_path_get_indices (path)[0];
	}

	gth_file_selection_changed (GTH_FILE_SELECTION (icon_view));

	g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (list);
}


static void
icon_view_item_activated_cb (GtkIconView *icon_view,
			     GtkTreePath *path,
			     gpointer     user_data)
{
	gth_file_view_activate_file (GTH_FILE_VIEW (icon_view), path);
}


static void
gth_icon_view_init (GthIconView *icon_view)
{
	icon_view->priv = G_TYPE_INSTANCE_GET_PRIVATE (icon_view, GTH_TYPE_ICON_VIEW, GthIconViewPrivate);
	icon_view->priv->selection_range_start = 0;
	icon_view->priv->drag_source_enabled = FALSE;
	icon_view->priv->dragging = FALSE;
	icon_view->priv->drag_started = FALSE;
	icon_view->priv->drag_target_list = NULL;

	gtk_icon_view_set_spacing (GTK_ICON_VIEW (icon_view), IMAGE_TEXT_SPACING);
	gth_icon_view_real_set_spacing (GTH_FILE_VIEW (icon_view), DEFAULT_ICON_SPACING);
	gtk_icon_view_set_selection_mode (GTK_ICON_VIEW (icon_view), GTK_SELECTION_MULTIPLE);
	gtk_widget_set_size_request (GTK_WIDGET (icon_view), SIZE_REQUEST, SIZE_REQUEST);

	g_signal_connect (icon_view,
			  "button-press-event",
			  G_CALLBACK (icon_view_button_press_event_cb),
			  icon_view);
	g_signal_connect (icon_view,
			  "motion-notify-event",
			  G_CALLBACK (icon_view_motion_notify_event_cb),
			  icon_view);
	g_signal_connect (icon_view,
			  "button-release-event",
			  G_CALLBACK (icon_view_button_release_event_cb),
			  icon_view);
	g_signal_connect (icon_view,
			  "selection-changed",
			  G_CALLBACK (icon_view_selection_changed_cb),
			  icon_view);
	g_signal_connect (icon_view,
			  "item-activated",
			  G_CALLBACK (icon_view_item_activated_cb),
			  icon_view);
}


static void
gth_icon_view_gth_file_view_interface_init (GthFileViewIface *iface)
{
	gth_icon_view_gth_file_view_parent_iface = g_type_interface_peek_parent (iface);
	iface->set_model = gth_icon_view_real_set_model;
	iface->get_model = gth_icon_view_real_get_model;
	iface->scroll_to = gth_icon_view_real_scroll_to;
	iface->get_visibility = gth_icon_view_real_get_visibility;
	iface->get_at_position = gth_icon_view_real_get_at_position;
	iface->get_first_visible = gth_icon_view_real_get_first_visible;
	iface->get_last_visible = gth_icon_view_real_get_last_visible;
	iface->activated = gth_icon_view_real_activated;
	iface->set_cursor = gth_icon_view_real_set_cursor;
	iface->get_cursor = gth_icon_view_real_get_cursor;
	iface->set_spacing = gth_icon_view_real_set_spacing;
	iface->enable_drag_source = gth_icon_view_enable_drag_source;
	iface->unset_drag_source = gth_icon_view_unset_drag_source;
	iface->enable_drag_dest = gth_icon_view_enable_drag_dest;
	iface->unset_drag_dest = gth_icon_view_unset_drag_dest;
	iface->set_drag_dest_pos = gth_icon_view_set_drag_dest_pos;
	iface->add_renderer = gth_icon_view_add_renderer;
	iface->update_attributes = gth_icon_view_update_attributes;
	iface->truncate_metadata = gth_icon_view_truncate_metadata;
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
