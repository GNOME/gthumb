/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2010 Free Software Foundation, Inc.
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
#include <glib.h>
#include <glib/gi18n.h>
#include <gthumb.h>
#include "picasa-web-service.h"


struct _PicasaWebServicePrivate
{
	GoogleConnection *conn;
};


static gpointer parent_class = NULL;


static void
picasa_web_service_finalize (GObject *object)
{
	PicasaWebService *self;

	self = PICASA_WEB_SERVICE (object);
	_g_object_unref (self->priv->conn);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
picasa_web_service_class_init (PicasaWebServiceClass *klass)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (PicasaWebServicePrivate));

	object_class = (GObjectClass*) klass;
	object_class->finalize = picasa_web_service_finalize;
}


static void
picasa_web_service_init (PicasaWebService *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, PICASA_TYPE_WEB_SERVICE, PicasaWebServicePrivate);
	self->priv->conn = NULL;
}


GType
picasa_web_service_get_type (void)
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (PicasaWebServiceClass),
			NULL,
			NULL,
			(GClassInitFunc) picasa_web_service_class_init,
			NULL,
			NULL,
			sizeof (PicasaWebService),
			0,
			(GInstanceInitFunc) picasa_web_service_init
		};

		type = g_type_register_static (G_TYPE_OBJECT,
					       "PicasaWebService",
					       &type_info,
					       0);
	}

	return type;
}


PicasaWebService *
picasa_web_service_new (GoogleConnection *conn)
{
	PicasaWebService *self;

	self = (PicasaWebService *) g_object_new (PICASA_TYPE_WEB_SERVICE, NULL);
	self->priv->conn = g_object_ref (conn);

	return self;
}


void
picasa_web_service_list_albums (PicasaWebService    *service,
			        const char          *user_id,
			        GCancellable        *cancellable,
			        GAsyncReadyCallback  callback,
			        gpointer             user_data)
{
	/* FIXME */
}


GList *
picasa_web_service_list_albums_finish (PicasaWebService  *service,
				       GAsyncResult      *result,
				       GError           **error)
{
	/* FIXME */
	return NULL;
}
