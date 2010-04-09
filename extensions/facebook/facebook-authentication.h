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

#ifndef FACEBOOK_AUTHENTICATION_H
#define FACEBOOK_AUTHENTICATION_H

#include <glib.h>
#include <glib-object.h>
#include "facebook-account.h"
#include "facebook-connection.h"
#include "facebook-service.h"
#include "facebook-user.h"

G_BEGIN_DECLS

#define FACEBOOK_TYPE_AUTHENTICATION            (facebook_authentication_get_type ())
#define FACEBOOK_AUTHENTICATION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), FACEBOOK_TYPE_AUTHENTICATION, FacebookAuthentication))
#define FACEBOOK_AUTHENTICATION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), FACEBOOK_TYPE_AUTHENTICATION, FacebookAuthenticationClass))
#define FACEBOOK_IS_AUTHENTICATION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FACEBOOK_TYPE_AUTHENTICATION))
#define FACEBOOK_IS_AUTHENTICATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), FACEBOOK_TYPE_AUTHENTICATION))
#define FACEBOOK_AUTHENTICATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), FACEBOOK_TYPE_AUTHENTICATION, FacebookAuthenticationClass))

typedef struct _FacebookAuthentication FacebookAuthentication;
typedef struct _FacebookAuthenticationClass FacebookAuthenticationClass;
typedef struct _FacebookAuthenticationPrivate FacebookAuthenticationPrivate;

struct _FacebookAuthentication {
	GObject parent_instance;
	FacebookAuthenticationPrivate *priv;
};

struct _FacebookAuthenticationClass {
	GObjectClass parent_class;

	/*< signals >*/

	void  (*ready)             (FacebookAuthentication *auth,
				    FacebookUser           *user);
	void  (*accounts_changed)  (FacebookAuthentication *auth);
};

GType                     facebook_authentication_get_type       (void);
FacebookAuthentication *  facebook_authentication_new            (FacebookConnection     *conn,
								  FacebookService        *service,
								  GCancellable           *cancellable,
								  GtkWidget              *browser,
								  GtkWidget              *dialog);
void                      facebook_authentication_auto_connect   (FacebookAuthentication *auth);
void                      facebook_authentication_connect        (FacebookAuthentication *auth,
								  FacebookAccount        *account);
FacebookAccount *         facebook_authentication_get_account    (FacebookAuthentication *auth);
GList *                   facebook_authentication_get_accounts   (FacebookAuthentication *auth);
void                      facebook_authentication_edit_accounts  (FacebookAuthentication *auth,
								  GtkWindow              *parent);

G_END_DECLS

#endif /* FACEBOOK_AUTHENTICATION_H */
