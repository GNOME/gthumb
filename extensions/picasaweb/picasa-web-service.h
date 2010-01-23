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

#ifndef PICASA_WEB_SERVICE_H
#define PICASA_WEB_SERVICE_H

#include <glib-object.h>
#include "google-connection.h"

#define PICASA_TYPE_WEB_SERVICE         (picasa_web_service_get_type ())
#define PICASA_WEB_SERVICE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), PICASA_TYPE_WEB_SERVICE, PicasaWebService))
#define PICASA_WEB_SERVICE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), PICASA_TYPE_WEB_SERVICE, PicasaWebServiceClass))
#define PICASA_IS_WEB_SERVICE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), PICASA_TYPE_WEB_SERVICE))
#define PICASA_IS_WEB_SERVICE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), PICASA_TYPE_WEB_SERVICE))
#define PICASA_WEB_SERVICE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), PICASA_TYPE_WEB_SERVICE, PicasaWebServiceClass))

typedef struct _PicasaWebService         PicasaWebService;
typedef struct _PicasaWebServicePrivate  PicasaWebServicePrivate;
typedef struct _PicasaWebServiceClass    PicasaWebServiceClass;

struct _PicasaWebService
{
	GObject __parent;
	PicasaWebServicePrivate *priv;
};

struct _PicasaWebServiceClass
{
	GObjectClass __parent_class;
};

GType                picasa_web_service_get_type           (void) G_GNUC_CONST;
PicasaWebService *   picasa_web_service_new                (GoogleConnection     *conn);
void                 picasa_web_service_list_albums        (PicasaWebService     *service,
						            const char           *user_id,
						            GCancellable         *cancellable,
						            GAsyncReadyCallback   callback,
						            gpointer              user_data);
GList *              picasa_web_service_list_albums_finish (PicasaWebService     *service,
						            GAsyncResult         *result,
						            GError              **error);

#endif /* PICASA_WEB_SERVICE_H */
