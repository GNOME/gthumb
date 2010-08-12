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

#include "gth-file-view.h"


void
gth_file_view_set_model (GthFileView  *self,
		         GtkTreeModel *model)
{
	GTH_FILE_VIEW_GET_INTERFACE (self)->set_model (self, model);
}


GtkTreeModel *
gth_file_view_get_model (GthFileView *self)
{
	return GTH_FILE_VIEW_GET_INTERFACE (self)->get_model (self);
}


void
gth_file_view_scroll_to (GthFileView *self,
			 int          pos,
			 double       yalign)
{
	GTH_FILE_VIEW_GET_INTERFACE (self)->scroll_to (self, pos, yalign);
}


GthVisibility
gth_file_view_get_visibility (GthFileView *self,
			      int          pos)
{
	return GTH_FILE_VIEW_GET_INTERFACE (self)->get_visibility (self, pos);
}


int
gth_file_view_get_at_position (GthFileView *self,
			       int          x,
			       int          y)
{
	return GTH_FILE_VIEW_GET_INTERFACE (self)->get_at_position (self, x, y);
}


int
gth_file_view_get_first_visible (GthFileView *self)
{
	return GTH_FILE_VIEW_GET_INTERFACE (self)->get_first_visible (self);
}


int
gth_file_view_get_last_visible (GthFileView *self)
{
	return GTH_FILE_VIEW_GET_INTERFACE (self)->get_last_visible (self);
}


void
gth_file_view_activated (GthFileView *self,
			 int          pos)
{
	GTH_FILE_VIEW_GET_INTERFACE (self)->activated (self, pos);
}


void
gth_file_view_set_cursor (GthFileView *self,
			  int          pos)
{
	GTH_FILE_VIEW_GET_INTERFACE (self)->set_cursor (self, pos);
}


int
gth_file_view_get_cursor (GthFileView *self)
{
	return GTH_FILE_VIEW_GET_INTERFACE (self)->get_cursor (self);
}


void
gth_file_view_set_spacing (GthFileView *self,
			   int          spacing)
{
	GTH_FILE_VIEW_GET_INTERFACE (self)->set_spacing (self, spacing);
}


void
gth_file_view_enable_drag_source (GthFileView          *self,
				  GdkModifierType       start_button_mask,
				  const GtkTargetEntry *targets,
				  int                   n_targets,
				  GdkDragAction         actions)
{
	GTH_FILE_VIEW_GET_INTERFACE (self)->enable_drag_source (self, start_button_mask, targets, n_targets, actions);
}


void
gth_file_view_unset_drag_source (GthFileView *self)
{
	GTH_FILE_VIEW_GET_INTERFACE (self)->unset_drag_source (self);
}


void
gth_file_view_enable_drag_dest (GthFileView          *self,
				const GtkTargetEntry *targets,
				int                   n_targets,
				GdkDragAction         actions)
{
	GTH_FILE_VIEW_GET_INTERFACE (self)->enable_drag_dest (self, targets, n_targets, actions);
}


void
gth_file_view_unset_drag_dest (GthFileView *self)
{
	GTH_FILE_VIEW_GET_INTERFACE (self)->unset_drag_dest (self);
}


void
gth_file_view_set_drag_dest_pos (GthFileView    *self,
				 GdkDragContext *context,
			         int             x,
			         int             y,
			         guint           time,
		                 int            *pos)
{
	GTH_FILE_VIEW_GET_INTERFACE (self)->set_drag_dest_pos (self, context, x, y, time, pos);
}


GType
gth_file_view_get_type (void)
{
	static GType type = 0;

	if (type == 0) {
		static const GTypeInfo g_define_type_info = {
			sizeof (GthFileViewIface),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) NULL,
			(GClassFinalizeFunc) NULL,
			NULL,
			0,
			0,
			(GInstanceInitFunc) NULL,
			NULL
		};
		type = g_type_register_static (G_TYPE_INTERFACE,
					       "GthFileView",
					       &g_define_type_info,
					       0);
	}

	return type;
}
