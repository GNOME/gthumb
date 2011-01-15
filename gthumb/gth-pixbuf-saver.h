/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2009 Free Software Foundation, Inc.
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

#ifndef GTH_PIXBUF_SAVER_H
#define GTH_PIXBUF_SAVER_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GTH_TYPE_PIXBUF_SAVER              (gth_pixbuf_saver_get_type ())
#define GTH_PIXBUF_SAVER(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_PIXBUF_SAVER, GthPixbufSaver))
#define GTH_PIXBUF_SAVER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_PIXBUF_SAVER, GthPixbufSaverClass))
#define GTH_IS_PIXBUF_SAVER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_PIXBUF_SAVER))
#define GTH_IS_PIXBUF_SAVER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_PIXBUF_SAVER))
#define GTH_PIXBUF_SAVER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_PIXBUF_SAVER, GthPixbufSaverClass))

typedef struct _GthPixbufSaver         GthPixbufSaver;
typedef struct _GthPixbufSaverClass    GthPixbufSaverClass;
typedef struct _GthPixbufSaverPrivate  GthPixbufSaverPrivate;

struct _GthPixbufSaver
{
	GObject __parent;
	GthPixbufSaverPrivate *priv;
};

struct _GthPixbufSaverClass
{
	GObjectClass __parent_class;

	/*< class attributes >*/

	const char *id;
	const char *display_name;
	const char *mime_type;
	const char *extensions;

	/*< virtual functions >*/

	const char * (*get_default_ext) (GthPixbufSaver   *self);
	GtkWidget *  (*get_control)     (GthPixbufSaver   *self);
	void         (*save_options)    (GthPixbufSaver   *self);
	gboolean     (*can_save)        (GthPixbufSaver   *self,
				         const char       *mime_type);
	gboolean     (*save_pixbuf)     (GthPixbufSaver   *self,
				         GdkPixbuf        *pixbuf,
				         char            **buffer,
				         gsize            *buffer_size,
				         const char       *mime_type,
				         GError          **error);
};

GType         gth_pixbuf_saver_get_type          (void);
const char *  gth_pixbuf_saver_get_id            (GthPixbufSaver  *self);
const char *  gth_pixbuf_saver_get_display_name  (GthPixbufSaver  *self);
const char *  gth_pixbuf_saver_get_mime_type     (GthPixbufSaver  *self);
const char *  gth_pixbuf_saver_get_extensions    (GthPixbufSaver  *self);
const char *  gth_pixbuf_saver_get_default_ext   (GthPixbufSaver  *self);
GtkWidget *   gth_pixbuf_saver_get_control       (GthPixbufSaver  *self);
void          gth_pixbuf_saver_save_options      (GthPixbufSaver  *self);
gboolean      gth_pixbuf_saver_can_save          (GthPixbufSaver  *self,
					          const char      *mime_type);
gboolean      gth_pixbuf_saver_save_pixbuf       (GthPixbufSaver  *self,
						  GdkPixbuf       *pixbuf,
						  char           **buffer,
						  gsize           *buffer_size,
						  const char      *mime_type,
						  GError         **error);

G_END_DECLS

#endif /* GTH_PIXBUF_SAVER_H */
