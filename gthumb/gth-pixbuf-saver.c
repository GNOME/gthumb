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
#include <gtk/gtk.h>
#include "gth-pixbuf-saver.h"


static gpointer parent_class = NULL;


static GtkWidget *
base_get_control (GthPixbufSaver *self)
{
	return gtk_label_new (_("No options available for this file type"));
}


static void
base_save_options (GthPixbufSaver *self)
{
	/* void */
}


static gboolean
base_can_save (GthPixbufSaver *self,
	       const char     *mime_type)
{
	return FALSE;
}


static gboolean
base_save_pixbuf (GthPixbufSaver  *self,
	          GdkPixbuf       *pixbuf,
	          char           **buffer,
	          gsize           *buffer_size,
	          const char      *mime_type,
	          GError         **error)
{
	return FALSE;
}


static void
gth_pixbuf_saver_class_init (GthPixbufSaverClass *klass)
{
	parent_class = g_type_class_peek_parent (klass);

	klass->id = "";
	klass->display_name = "";
	klass->get_control = base_get_control;
	klass->save_options = base_save_options;
	klass->can_save = base_can_save;
	klass->save_pixbuf = base_save_pixbuf;
}


GType
gth_pixbuf_saver_get_type (void)
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (GthPixbufSaverClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_pixbuf_saver_class_init,
			NULL,
			NULL,
			sizeof (GthPixbufSaver),
			0,
			(GInstanceInitFunc) NULL
		};

		type = g_type_register_static (G_TYPE_OBJECT,
					       "GthPixbufSaver",
					       &type_info,
					       0);
	}

	return type;
}


const char *
gth_pixbuf_saver_get_id (GthPixbufSaver *self)
{
	return GTH_PIXBUF_SAVER_GET_CLASS (self)->id;
}


const char *
gth_pixbuf_saver_get_display_name (GthPixbufSaver *self)
{
	return GTH_PIXBUF_SAVER_GET_CLASS (self)->display_name;
}


const char *
gth_pixbuf_saver_get_mime_type (GthPixbufSaver *self)
{
	return GTH_PIXBUF_SAVER_GET_CLASS (self)->mime_type;
}


const char *
gth_pixbuf_saver_get_default_ext (GthPixbufSaver *self)
{
	return GTH_PIXBUF_SAVER_GET_CLASS (self)->default_ext;
}


GtkWidget *
gth_pixbuf_saver_get_control (GthPixbufSaver *self)
{
	return GTH_PIXBUF_SAVER_GET_CLASS (self)->get_control (self);
}


void
gth_pixbuf_saver_save_options (GthPixbufSaver *self)
{
	GTH_PIXBUF_SAVER_GET_CLASS (self)->save_options (self);
}


gboolean
gth_pixbuf_saver_can_save (GthPixbufSaver *self,
			   const char     *mime_type)
{
	return GTH_PIXBUF_SAVER_GET_CLASS (self)->can_save (self, mime_type);
}


gboolean
gth_pixbuf_saver_save_pixbuf (GthPixbufSaver   *self,
			      GdkPixbuf        *pixbuf,
			      char            **buffer,
			      gsize            *buffer_size,
			      const char       *mime_type,
			      GError          **error)
{
	return GTH_PIXBUF_SAVER_GET_CLASS (self)->save_pixbuf (self,
							       pixbuf,
							       buffer,
							       buffer_size,
							       mime_type,
							       error);
}
