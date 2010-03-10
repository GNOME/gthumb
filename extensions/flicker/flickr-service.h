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

#ifndef FLICKR_SERVICE_H
#define FLICKR_SERVICE_H

#include <glib-object.h>
#include "flickr-account.h"
#include "flickr-connection.h"
#include "flickr-photoset.h"
#include "flickr-user.h"

typedef enum {
	FLICKR_PRIVACY_PUBLIC,
	FLICKR_PRIVACY_FRIENDS_FAMILY,
	FLICKR_PRIVACY_FRIENDS,
	FLICKR_PRIVACY_FAMILY,
	FLICKR_PRIVACY_PRIVATE
} FlickrPrivacyType;

typedef enum {
	FLICKR_SAFETY_SAFE,
	FLICKR_SAFETY_MODERATE,
	FLICKR_SAFETY_RESTRICTED
} FlickrSafetyType;

#define FLICKR_TYPE_SERVICE         (flickr_service_get_type ())
#define FLICKR_SERVICE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), FLICKR_TYPE_SERVICE, FlickrService))
#define FLICKR_SERVICE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), FLICKR_TYPE_SERVICE, FlickrServiceClass))
#define FLICKR_IS_SERVICE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), FLICKR_TYPE_SERVICE))
#define FLICKR_IS_SERVICE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), FLICKR_TYPE_SERVICE))
#define FLICKR_SERVICE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), FLICKR_TYPE_SERVICE, FlickrServiceClass))

typedef struct _FlickrService         FlickrService;
typedef struct _FlickrServicePrivate  FlickrServicePrivate;
typedef struct _FlickrServiceClass    FlickrServiceClass;

struct _FlickrService
{
	GObject __parent;
	FlickrServicePrivate *priv;
};

struct _FlickrServiceClass
{
	GObjectClass __parent_class;
};

GType             flickr_service_get_type                 (void) G_GNUC_CONST;
FlickrService *   flickr_service_new                      (FlickrConnection     *conn);
void              flickr_service_get_upload_status        (FlickrService        *self,
							   GCancellable         *cancellable,
							   GAsyncReadyCallback   callback,
							   gpointer              user_data);
FlickrUser *      flickr_service_get_upload_status_finish (FlickrService        *self,
						           GAsyncResult         *result,
						           GError              **error);
void              flickr_service_list_photosets           (FlickrService        *self,
							   const char           *user_id,
						           GCancellable         *cancellable,
						           GAsyncReadyCallback   callback,
						           gpointer              user_data);
GList *           flickr_service_list_photosets_finish    (FlickrService        *self,
						           GAsyncResult         *result,
						           GError              **error);
void              flickr_service_create_photoset          (FlickrService        *self,
						           FlickrPhotoset       *photoset,
						           GCancellable         *cancellable,
						           GAsyncReadyCallback   callback,
						           gpointer              user_data);
FlickrPhotoset *  flickr_service_create_photoset_finish   (FlickrService        *self,
						           GAsyncResult         *result,
						           GError              **error);
void              flickr_service_add_photos_to_set        (FlickrService        *self,
						           FlickrPhotoset       *photoset,
						           GList                *photo_ids,
						           GCancellable         *cancellable,
						           GAsyncReadyCallback   callback,
						           gpointer              user_data);
gboolean          flickr_service_add_photos_to_set_finish (FlickrService        *self,
						           GAsyncResult         *result,
						           GError              **error);
void              flickr_service_post_photos              (FlickrService        *self,
							   FlickrPrivacyType     privacy_level,
							   FlickrSafetyType      safety_level,
							   gboolean              hidden,
						           GList                *file_list, /* GFile list */
						           GCancellable         *cancellable,
						           GAsyncReadyCallback   callback,
						           gpointer              user_data);
GList *           flickr_service_post_photos_finish       (FlickrService        *self,
						           GAsyncResult         *result,
						           GError              **error);
#if 0
void              flickr_service_list_photos              (FlickrService        *self,
							   FlickrPhotoset       *photoset,
						           GCancellable         *cancellable,
						           GAsyncReadyCallback   callback,
						           gpointer              user_data);
GList *           flickr_service_list_photos_finish       (FlickrService        *self,
						           GAsyncResult         *result,
						           GError              **error);
#endif

/* utilities */

GList *          flickr_accounts_load_from_file  (void);
FlickrAccount *  flickr_accounts_find_default    (GList         *accounts);
void             flickr_accounts_save_to_file    (GList         *accounts,
						  FlickrAccount *default_account);

#endif /* FLICKR_SERVICE_H */
