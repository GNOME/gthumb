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
#include "google-connection.h"


GQuark
google_connection_error_quark (void)
{
	static GQuark quark;

        if (!quark)
                quark = g_quark_from_static_string ("google_connection");

        return quark;
}


struct _GoogleConnectionPrivate
{
	char *service;
	char *token;
};


static gpointer parent_class = NULL;


static void
google_connection_finalize (GObject *object)
{
	GoogleConnection *self;

	self = GOOGLE_CONNECTION (object);
	g_free (self->priv->service);
	g_free (self->priv->token);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
google_connection_exec (GthTask *base)
{
	/* void */
}


static void
google_connection_cancelled (GthTask *base)
{
	/* void */
}


static void
google_connection_class_init (GoogleConnectionClass *klass)
{
	GObjectClass *object_class;
	GthTaskClass *task_class;

	parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GoogleConnectionPrivate));

	object_class = (GObjectClass*) klass;
	object_class->finalize = google_connection_finalize;

	task_class = (GthTaskClass*) klass;
	task_class->exec = google_connection_exec;
	task_class->cancelled = google_connection_cancelled;
}


static void
google_connection_init (GoogleConnection *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GOOGLE_TYPE_CONNECTION, GoogleConnectionPrivate);
	self->priv->service = NULL;
	self->priv->token = NULL;
}


GType
google_connection_get_type (void)
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (GoogleConnectionClass),
			NULL,
			NULL,
			(GClassInitFunc) google_connection_class_init,
			NULL,
			NULL,
			sizeof (GoogleConnection),
			0,
			(GInstanceInitFunc) google_connection_init
		};

		type = g_type_register_static (GTH_TYPE_TASK,
					       "GoogleConnection",
					       &type_info,
					       0);
	}

	return type;
}


GoogleConnection *
google_connection_new (const char *service)
{
	GoogleConnection *self;

	self = (GoogleConnection *) g_object_new (GOOGLE_TYPE_CONNECTION, NULL);
	self->priv->service = g_strdup (service);

	return self;
}


void
google_connection_connect (GoogleConnection     *conn,
			   const char           *email,
			   const char           *password,
			   const char           *challange,
			   GCancellable         *cancellable,
			   GAsyncReadyCallback   callback,
			   gpointer              user_data)
{
	/* FIXME */
}


gboolean
google_connection_connect_finish (GoogleConnection  *conn,
				  GAsyncResult      *result,
				  GError           **error)
{
	/* FIXME */
	return FALSE;
}


const char *
google_connection_get_token (GoogleConnection*conn)
{
	return conn->priv->token;
}
