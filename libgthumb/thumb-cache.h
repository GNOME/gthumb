/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001 The Free Software Foundation, Inc.
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

#include <glib.h>

#ifndef THUMB_CACHE_H
#define THUMB_CACHE_H


#include "typedefs.h"


gchar *  cache_get_nautilus_thumbnail_file (const gchar *source);

/*gchar *  cache_get_gthumb_cache_name       (const gchar *source);*/

gchar *  cache_get_nautilus_cache_name     (const gchar *source);

gchar *  cache_get_nautilus_cache_dir      (const gchar *source);

void     cache_copy                        (const gchar *src,
					    const gchar *dest);

void     cache_move                        (const gchar *src,
					    const gchar *dest);

void     cache_delete                      (const gchar *filename);

/*
void     cache_remove_old_previews         (const gchar *dir, 
					    gboolean recursive, 
					    gboolean clear_all);
*/

void     cache_remove_old_previews_async   (const gchar *dir, 
					    gboolean recursive, 
					    gboolean clear_all);


#endif /* THUMB_CACHE_H */
