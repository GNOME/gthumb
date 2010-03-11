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

#ifndef FLICKR_ACCOUNT_H
#define FLICKR_ACCOUNT_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define FLICKR_TYPE_ACCOUNT            (flickr_account_get_type ())
#define FLICKR_ACCOUNT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), FLICKR_TYPE_ACCOUNT, FlickrAccount))
#define FLICKR_ACCOUNT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), FLICKR_TYPE_ACCOUNT, FlickrAccountClass))
#define FLICKR_IS_ACCOUNT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FLICKR_TYPE_ACCOUNT))
#define FLICKR_IS_ACCOUNT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), FLICKR_TYPE_ACCOUNT))
#define FLICKR_ACCOUNT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), FLICKR_TYPE_ACCOUNT, FlickrAccountClass))

typedef struct _FlickrAccount FlickrAccount;
typedef struct _FlickrAccountClass FlickrAccountClass;
typedef struct _FlickrAccountPrivate FlickrAccountPrivate;

struct _FlickrAccount {
	GObject parent_instance;
	FlickrAccountPrivate *priv;

	char     *username;
	char     *token;
	gboolean  is_default;
};

struct _FlickrAccountClass {
	GObjectClass parent_class;
};

GType             flickr_account_get_type       (void);
FlickrAccount *   flickr_account_new            (void);
void              flickr_account_set_username   (FlickrAccount *self,
						 const char    *value);
void              flickr_account_set_token      (FlickrAccount *self,
						 const char    *value);
int               flickr_account_cmp            (FlickrAccount *a,
						 FlickrAccount *b);

G_END_DECLS

#endif /* FLICKR_ACCOUNT_H */
