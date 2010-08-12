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

#ifndef FACEBOOK_ACCOUNT_H
#define FACEBOOK_ACCOUNT_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define FACEBOOK_TYPE_ACCOUNT            (facebook_account_get_type ())
#define FACEBOOK_ACCOUNT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), FACEBOOK_TYPE_ACCOUNT, FacebookAccount))
#define FACEBOOK_ACCOUNT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), FACEBOOK_TYPE_ACCOUNT, FacebookAccountClass))
#define FACEBOOK_IS_ACCOUNT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FACEBOOK_TYPE_ACCOUNT))
#define FACEBOOK_IS_ACCOUNT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), FACEBOOK_TYPE_ACCOUNT))
#define FACEBOOK_ACCOUNT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), FACEBOOK_TYPE_ACCOUNT, FacebookAccountClass))

typedef struct _FacebookAccount FacebookAccount;
typedef struct _FacebookAccountClass FacebookAccountClass;
typedef struct _FacebookAccountPrivate FacebookAccountPrivate;

struct _FacebookAccount {
	GObject parent_instance;
	FacebookAccountPrivate *priv;

	char     *user_id;
	char     *username;
	char     *session_key;
	char     *secret;
	gboolean  is_default;
};

struct _FacebookAccountClass {
	GObjectClass parent_class;
};

GType             facebook_account_get_type         (void);
FacebookAccount * facebook_account_new              (void);
void              facebook_account_set_session_key  (FacebookAccount *self,
						     const char      *value);
void              facebook_account_set_secret       (FacebookAccount *self,
						     const char      *value);
void              facebook_account_set_user_id      (FacebookAccount *self,
						     const char      *value);
void              facebook_account_set_username     (FacebookAccount *self,
						     const char      *value);
int               facebook_account_cmp              (FacebookAccount *a,
						     FacebookAccount *b);

G_END_DECLS

#endif /* FACEBOOK_ACCOUNT_H */
