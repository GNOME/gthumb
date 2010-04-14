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

#ifndef PHOTOBUCKET_SERVICE_H
#define PHOTOBUCKET_SERVICE_H

#include <glib-object.h>
#include <extensions/oauth/oauth-connection.h>

#define PHOTOBUCKET_TYPE_SERVICE         (photobucket_service_get_type ())
#define PHOTOBUCKET_SERVICE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), PHOTOBUCKET_TYPE_SERVICE, PhotobucketService))
#define PHOTOBUCKET_SERVICE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), PHOTOBUCKET_TYPE_SERVICE, PhotobucketServiceClass))
#define PHOTOBUCKET_IS_SERVICE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), PHOTOBUCKET_TYPE_SERVICE))
#define PHOTOBUCKET_IS_SERVICE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), PHOTOBUCKET_TYPE_SERVICE))
#define PHOTOBUCKET_SERVICE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), PHOTOBUCKET_TYPE_SERVICE, PhotobucketServiceClass))

typedef struct _PhotobucketService         PhotobucketService;
typedef struct _PhotobucketServicePrivate  PhotobucketServicePrivate;
typedef struct _PhotobucketServiceClass    PhotobucketServiceClass;

struct _PhotobucketService
{
	GObject __parent;
	PhotobucketServicePrivate *priv;
};

struct _PhotobucketServiceClass
{
	GObjectClass __parent_class;
};

GType                 photobucket_service_get_type                 (void) G_GNUC_CONST;
PhotobucketService *  photobucket_service_new                      (OAuthConnection      *conn);
void                  photobucket_service_login_request            (PhotobucketService   *self,
								    GCancellable         *cancellable,
								    GAsyncReadyCallback   callback,
								    gpointer              user_data);
gboolean              photobucket_service_login_request_finish     (PhotobucketService   *self,
						                    GAsyncResult         *result,
						                    GError              **error);
char *                photobucket_service_get_login_link           (PhotobucketService   *self);

/* utilities */

gboolean              photobucket_utils_parse_response     (SoupBuffer         *body,
							    DomDocument       **doc_p,
							    GError            **error);
GList *               photobucket_accounts_load_from_file  (void);
PhotobucketAccount *  photobucket_accounts_find_default    (GList              *accounts);
void                  photobucket_accounts_save_to_file    (GList              *accounts,
							    PhotobucketAccount *default_account);

#endif /* PHOTOBUCKET_SERVICE_H */
