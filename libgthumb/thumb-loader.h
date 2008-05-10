/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001, 2003, 2007 The Free Software Foundation, Inc.
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

#ifndef THUMB_LOADER_H
#define THUMB_LOADER_H

#include <gdk/gdk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <libgnomevfs/gnome-vfs-uri.h>
#include "image-loader.h"
#include "file-data.h"

#define THUMB_LOADER_TYPE            (thumb_loader_get_type ())
#define THUMB_LOADER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), THUMB_LOADER_TYPE, ThumbLoader))
#define THUMB_LOADER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), THUMB_LOADER_TYPE, ThumbLoaderClass))
#define IS_THUMB_LOADER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), THUMB_LOADER_TYPE))
#define IS_THUMB_LOADER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), THUMB_LOADER_TYPE))
#define THUMB_LOADER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), THUMB_LOADER_TYPE, ThumbLoaderClass))

typedef struct _ThumbLoader            ThumbLoader;
typedef struct _ThumbLoaderClass       ThumbLoaderClass;
typedef struct _ThumbLoaderPrivateData ThumbLoaderPrivateData;

struct _ThumbLoader
{
	GObject  __parent;
	ThumbLoaderPrivateData *priv;
};

struct _ThumbLoaderClass
{
	GObjectClass __parent_class;

	/* -- Signals -- */

	void (* thumb_error)       (ThumbLoader *il);
	void (* thumb_done)        (ThumbLoader *il);
	void (* thumb_progress)    (ThumbLoader *il,
				    float        percent);
};

GType      thumb_loader_get_type           (void);
GObject   *thumb_loader_new                (int                width,
					    int                height);
void       thumb_loader_set_thumb_size     (ThumbLoader       *tl,
					    int                width,
					    int                height);					   
void       thumb_loader_use_cache          (ThumbLoader       *tl,
					    gboolean           use);
void       thumb_loader_save_thumbnails    (ThumbLoader       *tl,
					    gboolean           save);
void       thumb_loader_set_max_file_size  (ThumbLoader       *tl,
					    goffset            size);
void       thumb_loader_set_file           (ThumbLoader       *tl,
					    FileData          *fd);
void       thumb_loader_set_path           (ThumbLoader       *tl,
					    const char        *path);
GdkPixbuf *thumb_loader_get_pixbuf         (ThumbLoader       *tl);
void       thumb_loader_start              (ThumbLoader       *tl);
void       thumb_loader_stop               (ThumbLoader       *tl,
					    DoneFunc           done_func,
					    gpointer           done_func_data);

#endif /* THUMB_LOADER_H */
