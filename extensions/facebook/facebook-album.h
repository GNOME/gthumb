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

#ifndef FACEBOOK_PHOTOSET_H
#define FACEBOOK_PHOTOSET_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define FACEBOOK_TYPE_PHOTOSET            (facebook_photoset_get_type ())
#define FACEBOOK_PHOTOSET(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), FACEBOOK_TYPE_PHOTOSET, FacebookPhotoset))
#define FACEBOOK_PHOTOSET_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), FACEBOOK_TYPE_PHOTOSET, FacebookPhotosetClass))
#define FACEBOOK_IS_PHOTOSET(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FACEBOOK_TYPE_PHOTOSET))
#define FACEBOOK_IS_PHOTOSET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), FACEBOOK_TYPE_PHOTOSET))
#define FACEBOOK_PHOTOSET_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), FACEBOOK_TYPE_PHOTOSET, FacebookPhotosetClass))

typedef struct _FacebookPhotoset FacebookPhotoset;
typedef struct _FacebookPhotosetClass FacebookPhotosetClass;

struct _FacebookPhotoset {
	GObject parent_instance;

	char *id;
	char *title;
	char *description;
	int   n_photos;
	char *primary;
	char *secret;
	char *server;
	char *farm;
	char *url;
};

struct _FacebookPhotosetClass {
	GObjectClass parent_class;
};

GType             facebook_photoset_get_type          (void);
FacebookPhotoset *  facebook_photoset_new               (void);
void              facebook_photoset_set_id            (FacebookPhotoset *self,
						     const char     *value);
void              facebook_photoset_set_title         (FacebookPhotoset *self,
						     const char     *value);
void              facebook_photoset_set_description   (FacebookPhotoset *self,
						     const char     *value);
void              facebook_photoset_set_n_photos      (FacebookPhotoset *self,
						     const char     *value);
void              facebook_photoset_set_primary       (FacebookPhotoset *self,
						     const char     *value);
void              facebook_photoset_set_secret        (FacebookPhotoset *self,
						     const char     *value);
void              facebook_photoset_set_server        (FacebookPhotoset *self,
						     const char     *value);
void              facebook_photoset_set_farm          (FacebookPhotoset *self,
						     const char     *value);
void              facebook_photoset_set_url           (FacebookPhotoset *self,
						     const char     *value);

G_END_DECLS

#endif /* FACEBOOK_PHOTOSET_H */
