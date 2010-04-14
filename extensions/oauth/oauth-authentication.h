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

#ifndef OAUTH_AUTHENTICATION_H
#define OAUTH_AUTHENTICATION_H

#include <glib.h>
#include <glib-object.h>
#include "oauth-account.h"
#include "oauth-connection.h"

G_BEGIN_DECLS

#define OAUTH_TYPE_AUTHENTICATION            (oauth_authentication_get_type ())
#define OAUTH_AUTHENTICATION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), OAUTH_TYPE_AUTHENTICATION, OAuthAuthentication))
#define OAUTH_AUTHENTICATION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), OAUTH_TYPE_AUTHENTICATION, OAuthAuthenticationClass))
#define OAUTH_IS_AUTHENTICATION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OAUTH_TYPE_AUTHENTICATION))
#define OAUTH_IS_AUTHENTICATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), OAUTH_TYPE_AUTHENTICATION))
#define OAUTH_AUTHENTICATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), OAUTH_TYPE_AUTHENTICATION, OAuthAuthenticationClass))

typedef struct _OAuthAuthentication OAuthAuthentication;
typedef struct _OAuthAuthenticationClass OAuthAuthenticationClass;
typedef struct _OAuthAuthenticationPrivate OAuthAuthenticationPrivate;

struct _OAuthAuthentication {
	GObject parent_instance;
	OAuthAuthenticationPrivate *priv;
};

struct _OAuthAuthenticationClass {
	GObjectClass parent_class;

	/*< signals >*/

	void  (*ready)             (OAuthAuthentication *auth);
	void  (*accounts_changed)  (OAuthAuthentication *auth);
};

GType                   oauth_authentication_get_type       (void);
OAuthAuthentication *   oauth_authentication_new            (OAuthConnection     *conn,
							     GCancellable        *cancellable,
							     GtkWidget           *browser,
							     GtkWidget           *dialog);
void                    oauth_authentication_auto_connect   (OAuthAuthentication *auth);
void                    oauth_authentication_connect        (OAuthAuthentication *auth,
							     OAuthAccount        *account);
OAuthAccount *          oauth_authentication_get_account    (OAuthAuthentication *auth);
GList *                 oauth_authentication_get_accounts   (OAuthAuthentication *auth);
void                    oauth_authentication_edit_accounts  (OAuthAuthentication *auth,
							     GtkWindow           *parent);

G_END_DECLS

#endif /* OAUTH_AUTHENTICATION_H */
