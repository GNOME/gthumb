/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2009 The Free Software Foundation, Inc.
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

#ifndef GTH_TAGS_EXPANDER_H
#define GTH_TAGS_EXPANDER_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GTH_TYPE_TAGS_EXPANDER            (gth_tags_expander_get_type ())
#define GTH_TAGS_EXPANDER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_TAGS_EXPANDER, GthTagsExpander))
#define GTH_TAGS_EXPANDER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_TAGS_EXPANDER, GthTagsExpanderClass))
#define GTH_IS_TAGS_EXPANDER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_TAGS_EXPANDER))
#define GTH_IS_TAGS_EXPANDER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_TAGS_EXPANDER))
#define GTH_TAGS_EXPANDER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_TAGS_EXPANDER, GthTagsExpanderClass))

typedef struct _GthTagsExpander GthTagsExpander;
typedef struct _GthTagsExpanderClass GthTagsExpanderClass;
typedef struct _GthTagsExpanderPrivate GthTagsExpanderPrivate;

struct _GthTagsExpander {
	GtkExpander parent_instance;
	GthTagsExpanderPrivate *priv;
};

struct _GthTagsExpanderClass {
	GtkExpanderClass parent_class;
};

GType        gth_tags_expander_get_type  (void);
GtkWidget *  gth_tags_expander_new       (GtkEntry *entry);

G_END_DECLS

#endif /* GTH_TAGS_EXPANDER_H */
