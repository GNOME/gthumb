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

#ifndef FLICKR_USER_H
#define FLICKR_USER_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define FLICKR_TYPE_USER            (flickr_user_get_type ())
#define FLICKR_USER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), FLICKR_TYPE_USER, FlickrUser))
#define FLICKR_USER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), FLICKR_TYPE_USER, FlickrUserClass))
#define FLICKR_IS_USER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FLICKR_TYPE_USER))
#define FLICKR_IS_USER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), FLICKR_TYPE_USER))
#define FLICKR_USER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), FLICKR_TYPE_USER, FlickrUserClass))

typedef struct _FlickrUser FlickrUser;
typedef struct _FlickrUserClass FlickrUserClass;

struct _FlickrUser {
	GObject parent_instance;

	char     *id;
	gboolean  is_pro;
	char     *username;
	goffset   max_bandwidth;
	goffset   used_bandwidth;
	goffset   max_filesize;
	goffset   max_videosize;
	int       n_sets;
	int       n_videos;
};

struct _FlickrUserClass {
	GObjectClass parent_class;
};

GType             flickr_user_get_type             (void);
FlickrUser *      flickr_user_new                  (void);
void              flickr_user_set_id               (FlickrUser *self,
						    const char *value);
void              flickr_user_set_is_pro           (FlickrUser *self,
						    const char *value);
void              flickr_user_set_username         (FlickrUser *self,
						    const char *value);
void              flickr_user_set_max_bandwidth    (FlickrUser *self,
						    const char *value);
void              flickr_user_set_used_bandwidth   (FlickrUser *self,
						    const char *value);
void              flickr_user_set_max_filesize     (FlickrUser *self,
						    const char *value);
void              flickr_user_set_max_videosize    (FlickrUser *self,
						    const char *value);
void              flickr_user_set_n_sets           (FlickrUser *self,
						    const char *value);
void              flickr_user_set_n_videos         (FlickrUser *self,
						    const char *value);

G_END_DECLS

#endif /* FLICKR_USER_H */
