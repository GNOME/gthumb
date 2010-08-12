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

#ifndef PICASA_WEB_USER_H
#define PICASA_WEB_USER_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define PICASA_WEB_TYPE_USER            (picasa_web_user_get_type ())
#define PICASA_WEB_USER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PICASA_WEB_TYPE_USER, PicasaWebUser))
#define PICASA_WEB_USER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), PICASA_WEB_TYPE_USER, PicasaWebUserClass))
#define PICASA_WEB_IS_USER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PICASA_WEB_TYPE_USER))
#define PICASA_WEB_IS_USER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PICASA_WEB_TYPE_USER))
#define PICASA_WEB_USER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), PICASA_WEB_TYPE_USER, PicasaWebUserClass))

typedef struct _PicasaWebUser PicasaWebUser;
typedef struct _PicasaWebUserClass PicasaWebUserClass;
typedef struct _PicasaWebUserPrivate PicasaWebUserPrivate;

struct _PicasaWebUser {
	GObject parent_instance;
	PicasaWebUserPrivate *priv;

	char    *id;
	char    *nickname;
	char    *icon;
	goffset  quota_limit;
	goffset  quota_current;
	goffset  max_photos_per_album;
};

struct _PicasaWebUserClass {
	GObjectClass parent_class;
};

GType             picasa_web_user_get_type          (void);
PicasaWebUser *   picasa_web_user_new               (void);
void              picasa_web_user_set_id            (PicasaWebUser *self,
						     const char    *value);
void              picasa_web_user_set_nickname      (PicasaWebUser *self,
						     const char    *value);
void              picasa_web_user_set_icon          (PicasaWebUser *self,
						     const char    *value);
void              picasa_web_user_set_quota_limit   (PicasaWebUser *self,
						     const char    *value);
void              picasa_web_user_set_quota_current (PicasaWebUser *self,
						     const char    *value);
void              picasa_web_user_set_max_photos    (PicasaWebUser *self,
						     const char    *value);

G_END_DECLS

#endif /* PICASA_WEB_USER_H */
