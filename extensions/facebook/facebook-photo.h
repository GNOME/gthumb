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

#ifndef FACEBOOK_PHOTO_H
#define FACEBOOK_PHOTO_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define FACEBOOK_TYPE_PHOTO            (facebook_photo_get_type ())
#define FACEBOOK_PHOTO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), FACEBOOK_TYPE_PHOTO, FacebookPhoto))
#define FACEBOOK_PHOTO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), FACEBOOK_TYPE_PHOTO, FacebookPhotoClass))
#define FACEBOOK_IS_PHOTO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FACEBOOK_TYPE_PHOTO))
#define FACEBOOK_IS_PHOTO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), FACEBOOK_TYPE_PHOTO))
#define FACEBOOK_PHOTO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), FACEBOOK_TYPE_PHOTO, FacebookPhotoClass))

typedef struct _FacebookPhoto FacebookPhoto;
typedef struct _FacebookPhotoClass FacebookPhotoClass;
typedef struct _FacebookPhotoPrivate FacebookPhotoPrivate;

struct _FacebookPhoto {
	GObject parent_instance;
	FacebookPhotoPrivate *priv;

	char            *id;
	char            *secret;
	char            *server;
	char            *title;
	gboolean         is_primary;
	char            *url_sq;
	char            *url_t;
	char            *url_s;
	char            *url_m;
	char            *url_o;
	char            *original_format;
	char            *mime_type;
	int              position;
};

struct _FacebookPhotoClass {
	GObjectClass parent_class;
};

GType             facebook_photo_get_type             (void);
FacebookPhoto *     facebook_photo_new                  (void);
void              facebook_photo_set_id               (FacebookPhoto *self,
					             const char  *value);
void              facebook_photo_set_secret           (FacebookPhoto *self,
					             const char  *value);
void              facebook_photo_set_server           (FacebookPhoto *self,
					             const char  *value);
void              facebook_photo_set_title            (FacebookPhoto *self,
					             const char  *value);
void              facebook_photo_set_is_primary       (FacebookPhoto *self,
					             const char  *value);
void              facebook_photo_set_url_sq           (FacebookPhoto *self,
					             const char  *value);
void              facebook_photo_set_url_t            (FacebookPhoto *self,
					             const char  *value);
void              facebook_photo_set_url_s            (FacebookPhoto *self,
					             const char  *value);
void              facebook_photo_set_url_m            (FacebookPhoto *self,
					             const char  *value);
void              facebook_photo_set_url_o            (FacebookPhoto *self,
					             const char  *value);
void              facebook_photo_set_original_format  (FacebookPhoto *self,
					             const char  *value);

G_END_DECLS

#endif /* FACEBOOK_PHOTO_H */
