/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2008 The Free Software Foundation, Inc.
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

#ifndef FILE_CACHE_H
#define FILE_CACHE_H

#include <glib.h>
#include <gio/gio.h>
#include "gth-file-data.h"
#include "gio-utils.h"

G_BEGIN_DECLS

char *   get_file_cache_full_path  (const char       *filename, 
		                    const char       *extension);
GFile *  _g_file_get_cache_file    (GFile            *file);
void     free_file_cache           (void);
void     check_cache_free_space    (void);
void     copy_remote_file_to_cache (GthFileData      *file_data,
				    GCancellable     *cancellable,
				    CopyDoneCallback  done_func,
				    gpointer          done_data);
GFile *  obtain_local_file         (GthFileData      *file_data);
void     update_file_from_cache    (GthFileData      *file_data,
				    GCancellable     *cancellable,
			  	    CopyDoneCallback  done_func,
				    gpointer          done_data);

G_END_DECLS

#endif /* FILE_CACHE_H */
