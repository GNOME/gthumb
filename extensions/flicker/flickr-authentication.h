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

#ifndef FLICKR_AUTHENTICATION_H
#define FLICKR_AUTHENTICATION_H

#include <glib.h>
#include <glib-object.h>
#include "flickr-account.h"
#include "flickr-connection.h"
#include "flickr-service.h"
#include "flickr-user.h"

G_BEGIN_DECLS

#define FLICKR_TYPE_AUTHENTICATION            (flickr_authentication_get_type ())
#define FLICKR_AUTHENTICATION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), FLICKR_TYPE_AUTHENTICATION, FlickrAuthentication))
#define FLICKR_AUTHENTICATION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), FLICKR_TYPE_AUTHENTICATION, FlickrAuthenticationClass))
#define FLICKR_IS_AUTHENTICATION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FLICKR_TYPE_AUTHENTICATION))
#define FLICKR_IS_AUTHENTICATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), FLICKR_TYPE_AUTHENTICATION))
#define FLICKR_AUTHENTICATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), FLICKR_TYPE_AUTHENTICATION, FlickrAuthenticationClass))

typedef struct _FlickrAuthentication FlickrAuthentication;
typedef struct _FlickrAuthenticationClass FlickrAuthenticationClass;
typedef struct _FlickrAuthenticationPrivate FlickrAuthenticationPrivate;

struct _FlickrAuthentication {
	GObject parent_instance;
	FlickrAuthenticationPrivate *priv;
};

struct _FlickrAuthenticationClass {
	GObjectClass parent_class;

	/*< signals >*/

	void  (*ready)             (FlickrAuthentication *auth,
				    FlickrUser           *user);
	void  (*accounts_changed)  (FlickrAuthentication *auth);
};

GType                   flickr_authentication_get_type       (void);
FlickrAuthentication *  flickr_authentication_new            (FlickrConnection     *conn,
							      FlickrService        *service,
							      GCancellable         *cancellable,
							      GtkWidget            *browser,
							      GtkWidget            *dialog);
void                    flickr_authentication_auto_connect   (FlickrAuthentication *auth);
void                    flickr_authentication_connect        (FlickrAuthentication *auth,
							      FlickrAccount        *account);
FlickrAccount *         flickr_authentication_get_account    (FlickrAuthentication *auth);
GList *                 flickr_authentication_get_accounts   (FlickrAuthentication *auth);
void                    flickr_authentication_edit_accounts  (FlickrAuthentication *auth,
							      GtkWindow            *parent);

G_END_DECLS

#endif /* FLICKR_AUTHENTICATION_H */
