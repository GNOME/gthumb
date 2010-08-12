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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GOOGLE_CONNECTION_H
#define GOOGLE_CONNECTION_H

#include <glib-object.h>
#ifdef HAVE_LIBSOUP_GNOME
#include <libsoup/soup-gnome.h>
#else
#include <libsoup/soup.h>
#endif /* HAVE_LIBSOUP_GNOME */
#include <gthumb.h>

#define GOOGLE_SERVICE_PICASA_WEB_ALBUM "lh2"
#define ATOM_ENTRY_MIME_TYPE "application/atom+xml; charset=UTF-8; type=entry"

typedef enum {
	GOOGLE_CONNECTION_ERROR_BAD_AUTHENTICATION,
	GOOGLE_CONNECTION_ERROR_NOT_VERIFIED,
	GOOGLE_CONNECTION_ERROR_TERMS_NOT_AGREED,
	GOOGLE_CONNECTION_ERROR_CAPTCHA_REQUIRED,
	GOOGLE_CONNECTION_ERROR_UNKNOWN,
	GOOGLE_CONNECTION_ERROR_ACCOUNT_DELETED,
	GOOGLE_CONNECTION_ERROR_ACCOUNT_DISABLED,
	GOOGLE_CONNECTION_ERROR_SERVICE_DISABLED,
	GOOGLE_CONNECTION_ERROR_SERVICE_UNAVAILABLE
} GoogleConnectionError;

#define GOOGLE_CONNECTION_ERROR google_connection_error_quark ()
GQuark google_connection_error_quark (void);

#define GOOGLE_TYPE_CONNECTION         (google_connection_get_type ())
#define GOOGLE_CONNECTION(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GOOGLE_TYPE_CONNECTION, GoogleConnection))
#define GOOGLE_CONNECTION_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GOOGLE_TYPE_CONNECTION, GoogleConnectionClass))
#define GOOGLE_IS_CONNECTION(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GOOGLE_TYPE_CONNECTION))
#define GOOGLE_IS_CONNECTION_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GOOGLE_TYPE_CONNECTION))
#define GOOGLE_CONNECTION_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), GOOGLE_TYPE_CONNECTION, GoogleConnectionClass))

typedef struct _GoogleConnection         GoogleConnection;
typedef struct _GoogleConnectionPrivate  GoogleConnectionPrivate;
typedef struct _GoogleConnectionClass    GoogleConnectionClass;

struct _GoogleConnection
{
	GthTask __parent;
	GoogleConnectionPrivate *priv;
};

struct _GoogleConnectionClass
{
	GthTaskClass __parent_class;
};

GType                google_connection_get_type          (void) G_GNUC_CONST;
GoogleConnection *   google_connection_new               (const char           *service);
void		     google_connection_send_message      (GoogleConnection     *self,
							  SoupMessage          *msg,
							  GCancellable         *cancellable,
							  GAsyncReadyCallback   callback,
							  gpointer              user_data,
							  gpointer              source_tag,
							  SoupSessionCallback   soup_session_cb,
							  gpointer              soup_session_cb_data);
GSimpleAsyncResult * google_connection_get_result        (GoogleConnection     *self);
void                 google_connection_connect           (GoogleConnection     *self,
						          const char           *email,
						          const char           *password,
						          const char           *challange,
						          GCancellable         *cancellable,
						          GAsyncReadyCallback   callback,
						          gpointer              user_data);
gboolean             google_connection_connect_finish    (GoogleConnection     *self,
						          GAsyncResult         *result,
						          GError              **error);
const char *         google_connection_get_challange_url (GoogleConnection     *self);

#endif /* GOOGLE_CONNECTION_H */
