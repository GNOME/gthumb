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
#include "gth-duplicable.h"


GObject * 
gth_duplicable_duplicate (GthDuplicable *self) 
{
	return GTH_DUPLICABLE_GET_INTERFACE (self)->duplicate (self);
}


GType 
gth_duplicable_get_type (void) 
{
	static GType type_id = 0;
	if (type_id == 0) {
		static const GTypeInfo g_define_type_info = { 
			sizeof (GthDuplicableIface), 
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
		type_id = g_type_register_static (G_TYPE_INTERFACE, 
						  "GthDuplicable", 
						  &g_define_type_info, 
						  0);
	}
	return type_id;
}
