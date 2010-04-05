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

#ifndef FACEBOOK_CONNECTION_H
#define FACEBOOK_CONNECTION_H

#include <glib-object.h>
#ifdef HAVE_LIBSOUP_GNOME
#include <libsoup/soup-gnome.h>
#else
#include <libsoup/soup.h>
#endif /* HAVE_LIBSOUP_GNOME */
#include <gthumb.h>

typedef enum {
	FACEBOOK_ACCESS_READ,
	FACEBOOK_ACCESS_WRITE,
	FACEBOOK_ACCESS_DELETE
} FacebookAccessType;

#define FACEBOOK_CONNECTION_ERROR facebook_connection_error_quark ()
GQuark facebook_connection_error_quark (void);

#define FACEBOOK_TYPE_CONNECTION         (facebook_connection_get_type ())
#define FACEBOOK_CONNECTION(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), FACEBOOK_TYPE_CONNECTION, FacebookConnection))
#define FACEBOOK_CONNECTION_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), FACEBOOK_TYPE_CONNECTION, FacebookConnectionClass))
#define FACEBOOK_IS_CONNECTION(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), FACEBOOK_TYPE_CONNECTION))
#define FACEBOOK_IS_CONNECTION_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), FACEBOOK_TYPE_CONNECTION))
#define FACEBOOK_CONNECTION_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), FACEBOOK_TYPE_CONNECTION, FacebookConnectionClass))

typedef struct _FacebookConnection         FacebookConnection;
typedef struct _FacebookConnectionPrivate  FacebookConnectionPrivate;
typedef struct _FacebookConnectionClass    FacebookConnectionClass;

struct _FacebookConnection
{
	GthTask __parent;
	FacebookConnectionPrivate *priv;
};

struct _FacebookConnectionClass
{
	GthTaskClass __parent_class;
};

GType                facebook_connection_get_type           (void) G_GNUC_CONST;
FacebookConnection *   facebook_connection_new                (void);
void		     facebook_connection_send_message       (FacebookConnection      *self,
						           SoupMessage           *msg,
						           GCancellable          *cancellable,
						           GAsyncReadyCallback    callback,
						           gpointer               user_data,
						           gpointer               source_tag,
						           SoupSessionCallback    soup_session_cb,
						           gpointer               soup_session_cb_data);
GSimpleAsyncResult * facebook_connection_get_result         (FacebookConnection      *self);
void                 facebook_connection_reset_result       (FacebookConnection      *self);
void                 facebook_connection_add_api_sig        (FacebookConnection      *self,
						           GHashTable            *data_set);
void                 facebook_connection_get_frob           (FacebookConnection      *self,
						           GCancellable          *cancellable,
						           GAsyncReadyCallback    callback,
						           gpointer               user_data);
gboolean             facebook_connection_get_frob_finish    (FacebookConnection      *self,
							   GAsyncResult          *result,
							   GError               **error);
char *               facebook_connection_get_login_link     (FacebookConnection      *self,
							   FacebookAccessType       access_type);
void                 facebook_connection_get_token          (FacebookConnection      *self,
						           GCancellable          *cancellable,
						           GAsyncReadyCallback    callback,
						           gpointer               user_data);
gboolean             facebook_connection_get_token_finish   (FacebookConnection      *self,
							   GAsyncResult          *result,
							   GError               **error);
void                 facebook_connection_set_auth_token     (FacebookConnection      *self,
							   const char            *value);
const char *         facebook_connection_get_auth_token     (FacebookConnection      *self);
const char *         facebook_connection_get_username       (FacebookConnection      *self);
const char *         facebook_connection_get_user_id        (FacebookConnection      *self);

/* utilities */

gboolean             facebook_utils_parse_response          (SoupBuffer            *body,
							   DomDocument          **doc_p,
							   GError               **error);

#endif /* FACEBOOK_CONNECTION_H */
