/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001-2009 The Free Software Foundation, Inc.
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

#ifndef PIXBUF_IO_H
#define PIXBUF_IO_H

#include <glib.h>
#include <gdk/gdk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "gth-file-data.h"
#include "typedefs.h"

G_BEGIN_DECLS


typedef struct {
	GFile *file;
	void  *buffer;
	gsize  buffer_size;
} SavePixbufFile;


typedef struct {
	GthFileData  *file_data;
	GdkPixbuf    *pixbuf;
	const char   *mime_type;
	void         *buffer;
	gsize         buffer_size;
	GList        *files; 		/* SavePixbufFile list */
	GError      **error;
} SavePixbufData;

char *      get_pixbuf_type_from_mime_type     (const char       *mime_type);
void        _gdk_pixbuf_save_async             (GdkPixbuf        *pixbuf,
						GthFileData      *file_data,
						const char       *mime_type,
						GthFileDataFunc   ready_func,
						gpointer          data);
GdkPixbuf * gth_pixbuf_new_from_file           (GthFileData      *file,
						GError          **error,
						int               requested_width,
						int               requested_height);
GdkPixbufAnimation*
	    gth_pixbuf_animation_new_from_file (GthFileData      *file_data,
						GError          **error,
						int               requested_width,
						int               requested_height);

G_END_DECLS

#endif /* PIXBUF_IO_H */
