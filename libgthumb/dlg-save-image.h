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

#ifndef DLG_SAVE_IMAGE_H
#define DLG_SAVE_IMAGE_H

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtkwindow.h>
#include "file-data.h"

typedef void (*ImageSavedFunc) (FileData  *file,
				gpointer   data);

void       dlg_save_image_as (GtkWindow        *parent,
			      const char       *uri,
			      GList            *metadata,
			      GdkPixbuf        *pixbuf,
			      ImageSavedFunc    done_func,
			      gpointer          done_data);
void       dlg_save_image    (GtkWindow        *parent,
			      FileData         *file,
			      GList	       *metadata,
			      GdkPixbuf        *pixbuf,
			      ImageSavedFunc    done_func,
			      gpointer          done_data);
gboolean   dlg_save_options  (GtkWindow        *parent,
			      const char       *image_type,
			      char           ***keys,
			      char           ***values);

#endif /* DLG_SAVE_IMAGE_H */
