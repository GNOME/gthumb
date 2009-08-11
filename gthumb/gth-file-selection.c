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

#include "gth-file-selection.h"



void
gth_file_selection_set_selection_mode (GthFileSelection *self,
				       GtkSelectionMode  mode)
{
	GTH_FILE_SELECTION_GET_INTERFACE (self)->set_selection_mode (self, mode);
}


GList *
gth_file_selection_get_selected (GthFileSelection *self)
{
	return GTH_FILE_SELECTION_GET_INTERFACE (self)->get_selected (self);
}


void
gth_file_selection_select (GthFileSelection *self,
			   int               pos)
{
	GTH_FILE_SELECTION_GET_INTERFACE (self)->select (self, pos);
}


void
gth_file_selection_unselect (GthFileSelection *self,
			     int               pos)
{
	GTH_FILE_SELECTION_GET_INTERFACE (self)->unselect (self, pos);
}


void
gth_file_selection_select_all (GthFileSelection *self)
{
	GTH_FILE_SELECTION_GET_INTERFACE (self)->select_all (self);
}


void
gth_file_selection_unselect_all (GthFileSelection *self)
{
	GTH_FILE_SELECTION_GET_INTERFACE (self)->unselect_all (self);
}


gboolean
gth_file_selection_is_selected (GthFileSelection *self,
				int               pos)
{
	return GTH_FILE_SELECTION_GET_INTERFACE (self)->is_selected (self, pos);
}


GtkTreePath *
gth_file_selection_get_first_selected (GthFileSelection *self)
{
	return GTH_FILE_SELECTION_GET_INTERFACE (self)->get_first_selected (self);
}


GtkTreePath *
gth_file_selection_get_last_selected (GthFileSelection *self)
{
	return GTH_FILE_SELECTION_GET_INTERFACE (self)->get_last_selected (self);
}


guint
gth_file_selection_get_n_selected (GthFileSelection *self)
{
	return GTH_FILE_SELECTION_GET_INTERFACE (self)->get_n_selected (self);
}


GType
gth_file_selection_get_type (void)
{
	static GType type = 0;

	if (type == 0) {
		static const GTypeInfo g_define_type_info = {
			sizeof (GthFileSelectionIface),
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
					       "GthFileSelection",
					       &g_define_type_info,
					       0);
	}
	return type;
}
