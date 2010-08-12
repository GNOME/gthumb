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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef FACEBOOK_USER_H
#define FACEBOOK_USER_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define FACEBOOK_TYPE_USER            (facebook_user_get_type ())
#define FACEBOOK_USER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), FACEBOOK_TYPE_USER, FacebookUser))
#define FACEBOOK_USER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), FACEBOOK_TYPE_USER, FacebookUserClass))
#define FACEBOOK_IS_USER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FACEBOOK_TYPE_USER))
#define FACEBOOK_IS_USER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), FACEBOOK_TYPE_USER))
#define FACEBOOK_USER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), FACEBOOK_TYPE_USER, FacebookUserClass))

typedef struct _FacebookUser FacebookUser;
typedef struct _FacebookUserClass FacebookUserClass;

struct _FacebookUser {
	GObject parent_instance;

	char *id;
	char *username;
};

struct _FacebookUserClass {
	GObjectClass parent_class;
};

GType          facebook_user_get_type     (void);
FacebookUser * facebook_user_new          (void);
void           facebook_user_set_id       (FacebookUser *self,
				           const char   *value);
void           facebook_user_set_username (FacebookUser *self,
				           const char   *value);

G_END_DECLS

#endif /* FACEBOOK_USER_H */
