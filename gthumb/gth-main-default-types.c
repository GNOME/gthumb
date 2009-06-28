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
#include "gth-file-properties.h"
#include "gth-main.h"
#include "pixbuf-io.h"


static void
gth_main_register_default_file_loader (void)
{
	GSList *formats;
	GSList *scan;

	formats = gdk_pixbuf_get_formats ();
	for (scan = formats; scan; scan = scan->next) {
		GdkPixbufFormat  *format = scan->data;
		char            **mime_types;
		int               i;

		if (gdk_pixbuf_format_is_disabled (format))
			continue;

		mime_types = gdk_pixbuf_format_get_mime_types (format);
		for (i = 0; mime_types[i] != NULL; i++)
			gth_main_register_file_loader (gth_pixbuf_animation_new_from_file, mime_types[i], NULL);
	}
}


void
gth_main_register_default_types (void)
{
	gth_main_register_type ("file-properties", GTH_TYPE_FILE_PROPERTIES);
	gth_main_register_default_file_loader ();
}
