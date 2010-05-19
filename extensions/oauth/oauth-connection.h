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

#ifndef OAUTH_CONNECTION_H
#define OAUTH_CONNECTION_H

#include <glib-object.h>
#ifdef HAVE_LIBSOUP_GNOME
#include <libsoup/soup-gnome.h>
#else
#include <libsoup/soup.h>
#endif /* HAVE_LIBSOUP_GNOME */
#include <gthumb.h>
#include "oauth-account.h"

#define OAUTH_CONNECTION_ERROR oauth_connection_error_quark ()
GQuark oauth_connection_error_quark (void);

#define OAUTH_CONNECTION_ERROR_INVALID_TOKEN 100

#define OAUTH_TYPE_CONNECTION         (oauth_connection_get_type ())
#define OAUTH_CONNECTION(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), OAUTH_TYPE_CONNECTION, OAuthConnection))
#define OAUTH_CONNECTION_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), OAUTH_TYPE_CONNECTION, OAuthConnectionClass))
#define OAUTH_IS_CONNECTION(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), OAUTH_TYPE_CONNECTION))
#define OAUTH_IS_CONNECTION_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), OAUTH_TYPE_CONNECTION))
#define OAUTH_CONNECTION_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), OAUTH_TYPE_CONNECTION, OAuthConnectionClass))

typedef struct _OAuthConnection         OAuthConnection;
typedef struct _OAuthConnectionPrivate  OAuthConnectionPrivate;
typedef struct _OAuthConnectionClass    OAuthConnectionClass;
typedef struct _OAuthConsumer           OAuthConsumer;

struct _OAuthConnection
{
	GthTask __parent;
	OAuthConsumer *consumer;
	OAuthConnectionPrivate *priv;
};

struct _OAuthConnectionClass
{
	GthTaskClass __parent_class;
};

GType                oauth_connection_get_type                 (void) G_GNUC_CONST;
OAuthConnection *    oauth_connection_new                      (OAuthConsumer        *consumer);
void		     oauth_connection_send_message             (OAuthConnection      *self,
						                SoupMessage          *msg,
						                GCancellable         *cancellable,
						                GAsyncReadyCallback   callback,
						                gpointer              user_data,
						                gpointer              source_tag,
						                SoupSessionCallback   soup_session_cb,
						                gpointer              soup_session_cb_data);
GSimpleAsyncResult * oauth_connection_get_result               (OAuthConnection      *self);
void                 oauth_connection_reset_result             (OAuthConnection      *self);
void                 oauth_connection_add_signature            (OAuthConnection      *self,
								const char           *method,
								const char           *url,
								GHashTable           *parameters);
void                 oauth_connection_get_request_token        (OAuthConnection      *self,
							        GCancellable         *cancellable,
							        GAsyncReadyCallback   callback,
							        gpointer              user_data);
gboolean             oauth_connection_get_request_token_finish (OAuthConnection      *self,
						                GAsyncResult         *result,
						                GError              **error);
char *               oauth_connection_get_login_link           (OAuthConnection      *self);
void                 oauth_connection_get_access_token         (OAuthConnection      *self,
								GCancellable         *cancellable,
							        GAsyncReadyCallback   callback,
							        gpointer              user_data);
OAuthAccount *       oauth_connection_get_access_token_finish  (OAuthConnection      *self,
								GAsyncResult         *result,
								GError              **error);
void                 oauth_connection_set_token                (OAuthConnection      *self,
							        const char           *token,
								const char           *token_secret);
const char *         oauth_connection_get_token                (OAuthConnection      *self);
const char *         oauth_connection_get_token_secret         (OAuthConnection      *self);
void                 oauth_connection_check_token              (OAuthConnection      *self,
								OAuthAccount         *account,
							        GCancellable         *cancellable,
							        GAsyncReadyCallback   callback,
							        gpointer              user_data);
gboolean             oauth_connection_check_token_finish       (OAuthConnection      *self,
						                GAsyncResult         *result,
						                GError              **error);

/* -- OAuthConsumer -- */

typedef void   (*OAuthResponseFunc)        (OAuthConnection    *self,
				            SoupMessage        *msg,
				            SoupBuffer         *body,
				            GSimpleAsyncResult *result);
typedef void   (*OAuthAccountResponseFunc) (OAuthConnection    *self,
				            SoupMessage        *msg,
				            GSimpleAsyncResult *result,
				            OAuthAccount       *account);
typedef char * (*OAuthStringFunc)          (OAuthConnection    *self);
typedef char * (*OAuthUrlFunc)             (OAuthConnection    *self,
				            OAuthAccount       *account,
				            gboolean            for_signature);

struct _OAuthConsumer {
	const char               *display_name;
	const char               *name;
	const char               *url;
	const char               *protocol;
	const char               *consumer_key;
	const char               *consumer_secret;
	const char               *request_token_url;
	OAuthResponseFunc         get_request_token_response;
	OAuthStringFunc           get_login_link;
	const char               *access_token_url;
	OAuthResponseFunc         get_access_token_response;
	OAuthUrlFunc              get_check_token_url;
	OAuthAccountResponseFunc  check_token_response;
};

#endif /* OAUTH_CONNECTION_H */
