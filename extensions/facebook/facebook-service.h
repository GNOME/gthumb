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

#ifndef FACEBOOK_SERVICE_H
#define FACEBOOK_SERVICE_H

#include <glib-object.h>
#include "facebook-account.h"
#include "facebook-connection.h"
#include "facebook-photoset.h"
#include "facebook-user.h"

typedef enum {
	FACEBOOK_PRIVACY_PUBLIC,
	FACEBOOK_PRIVACY_FRIENDS_FAMILY,
	FACEBOOK_PRIVACY_FRIENDS,
	FACEBOOK_PRIVACY_FAMILY,
	FACEBOOK_PRIVACY_PRIVATE
} FacebookPrivacyType;

typedef enum {
	FACEBOOK_SAFETY_SAFE,
	FACEBOOK_SAFETY_MODERATE,
	FACEBOOK_SAFETY_RESTRICTED
} FacebookSafetyType;

#define FACEBOOK_TYPE_SERVICE         (facebook_service_get_type ())
#define FACEBOOK_SERVICE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), FACEBOOK_TYPE_SERVICE, FacebookService))
#define FACEBOOK_SERVICE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), FACEBOOK_TYPE_SERVICE, FacebookServiceClass))
#define FACEBOOK_IS_SERVICE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), FACEBOOK_TYPE_SERVICE))
#define FACEBOOK_IS_SERVICE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), FACEBOOK_TYPE_SERVICE))
#define FACEBOOK_SERVICE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), FACEBOOK_TYPE_SERVICE, FacebookServiceClass))

typedef struct _FacebookService         FacebookService;
typedef struct _FacebookServicePrivate  FacebookServicePrivate;
typedef struct _FacebookServiceClass    FacebookServiceClass;

struct _FacebookService
{
	GObject __parent;
	FacebookServicePrivate *priv;
};

struct _FacebookServiceClass
{
	GObjectClass __parent_class;
};

GType             facebook_service_get_type                 (void) G_GNUC_CONST;
FacebookService *   facebook_service_new                      (FacebookConnection     *conn);
void              facebook_service_get_upload_status        (FacebookService        *self,
							   GCancellable         *cancellable,
							   GAsyncReadyCallback   callback,
							   gpointer              user_data);
FacebookUser *      facebook_service_get_upload_status_finish (FacebookService        *self,
						           GAsyncResult         *result,
						           GError              **error);
void              facebook_service_list_photosets           (FacebookService        *self,
							   const char           *user_id,
						           GCancellable         *cancellable,
						           GAsyncReadyCallback   callback,
						           gpointer              user_data);
GList *           facebook_service_list_photosets_finish    (FacebookService        *self,
						           GAsyncResult         *result,
						           GError              **error);
void              facebook_service_create_photoset          (FacebookService        *self,
						           FacebookPhotoset       *photoset,
						           GCancellable         *cancellable,
						           GAsyncReadyCallback   callback,
						           gpointer              user_data);
FacebookPhotoset *  facebook_service_create_photoset_finish   (FacebookService        *self,
						           GAsyncResult         *result,
						           GError              **error);
void              facebook_service_add_photos_to_set        (FacebookService        *self,
						           FacebookPhotoset       *photoset,
						           GList                *photo_ids,
						           GCancellable         *cancellable,
						           GAsyncReadyCallback   callback,
						           gpointer              user_data);
gboolean          facebook_service_add_photos_to_set_finish (FacebookService        *self,
						           GAsyncResult         *result,
						           GError              **error);
void              facebook_service_post_photos              (FacebookService        *self,
							   FacebookPrivacyType     privacy_level,
							   FacebookSafetyType      safety_level,
							   gboolean              hidden,
						           GList                *file_list, /* GFile list */
						           GCancellable         *cancellable,
						           GAsyncReadyCallback   callback,
						           gpointer              user_data);
GList *           facebook_service_post_photos_finish       (FacebookService        *self,
						           GAsyncResult         *result,
						           GError              **error);
void              facebook_service_list_photos              (FacebookService        *self,
							   FacebookPhotoset       *photoset,
							   const char           *extras,
							   int                   per_page,
							   int                   page,
						           GCancellable         *cancellable,
						           GAsyncReadyCallback   callback,
						           gpointer              user_data);
GList *           facebook_service_list_photos_finish       (FacebookService        *self,
						           GAsyncResult         *result,
						           GError              **error);

/* utilities */

GList *          facebook_accounts_load_from_file  (void);
FacebookAccount *  facebook_accounts_find_default    (GList         *accounts);
void             facebook_accounts_save_to_file    (GList         *accounts,
						  FacebookAccount *default_account);

#endif /* FACEBOOK_SERVICE_H */
