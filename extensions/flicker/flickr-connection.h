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

#ifndef FLICKR_CONNECTION_H
#define FLICKR_CONNECTION_H

#include <glib-object.h>
#ifdef HAVE_LIBSOUP_GNOME
#include <libsoup/soup-gnome.h>
#else
#include <libsoup/soup.h>
#endif /* HAVE_LIBSOUP_GNOME */
#include <gthumb.h>

typedef enum {
	FLICKR_ACCESS_READ,
	FLICKR_ACCESS_WRITE,
	FLICKR_ACCESS_DELETE
} FlickrAccessType;

#define FLICKR_CONNECTION_ERROR flickr_connection_error_quark ()
GQuark flickr_connection_error_quark (void);

#define FLICKR_TYPE_CONNECTION         (flickr_connection_get_type ())
#define FLICKR_CONNECTION(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), FLICKR_TYPE_CONNECTION, FlickrConnection))
#define FLICKR_CONNECTION_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), FLICKR_TYPE_CONNECTION, FlickrConnectionClass))
#define FLICKR_IS_CONNECTION(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), FLICKR_TYPE_CONNECTION))
#define FLICKR_IS_CONNECTION_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), FLICKR_TYPE_CONNECTION))
#define FLICKR_CONNECTION_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), FLICKR_TYPE_CONNECTION, FlickrConnectionClass))

typedef struct _FlickrConnection         FlickrConnection;
typedef struct _FlickrConnectionPrivate  FlickrConnectionPrivate;
typedef struct _FlickrConnectionClass    FlickrConnectionClass;

struct _FlickrConnection
{
	GthTask __parent;
	FlickrConnectionPrivate *priv;
};

struct _FlickrConnectionClass
{
	GthTaskClass __parent_class;
};

GType                flickr_connection_get_type           (void) G_GNUC_CONST;
FlickrConnection *   flickr_connection_new                (void);
void		     flickr_connection_send_message       (FlickrConnection      *self,
						           SoupMessage           *msg,
						           GCancellable          *cancellable,
						           GAsyncReadyCallback    callback,
						           gpointer               user_data,
						           gpointer               source_tag,
						           SoupSessionCallback    soup_session_cb,
						           gpointer               soup_session_cb_data);
GSimpleAsyncResult * flickr_connection_get_result         (FlickrConnection      *self);
void                 flickr_connection_reset_result       (FlickrConnection      *self);
void                 flickr_connection_add_api_sig        (FlickrConnection      *self,
						           GHashTable            *data_set);
void                 flickr_connection_get_frob           (FlickrConnection      *self,
						           GCancellable          *cancellable,
						           GAsyncReadyCallback    callback,
						           gpointer               user_data);
gboolean             flickr_connection_get_frob_finish    (FlickrConnection      *self,
							   GAsyncResult          *result,
							   GError               **error);
char *               flickr_connection_get_login_link     (FlickrConnection      *self,
							   FlickrAccessType       access_type);
void                 flickr_connection_get_token          (FlickrConnection      *self,
						           GCancellable          *cancellable,
						           GAsyncReadyCallback    callback,
						           gpointer               user_data);
gboolean             flickr_connection_get_token_finish   (FlickrConnection      *self,
							   GAsyncResult          *result,
							   GError               **error);
void                 flickr_connection_set_auth_token     (FlickrConnection      *self,
							   const char            *value);
const char *         flickr_connection_get_auth_token     (FlickrConnection      *self);
const char *         flickr_connection_get_username       (FlickrConnection      *self);
const char *         flickr_connection_get_user_id        (FlickrConnection      *self);

/* utilities */

gboolean             flickr_utils_parse_response          (SoupBuffer            *body,
							   DomDocument          **doc_p,
							   GError               **error);

#endif /* FLICKR_CONNECTION_H */
