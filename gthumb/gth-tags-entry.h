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

#ifndef GTH_TAGS_ENTRY_H
#define GTH_TAGS_ENTRY_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GTH_TYPE_TAGS_ENTRY            (gth_tags_entry_get_type ())
#define GTH_TAGS_ENTRY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_TAGS_ENTRY, GthTagsEntry))
#define GTH_TAGS_ENTRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_TAGS_ENTRY, GthTagsEntryClass))
#define GTH_IS_TAGS_ENTRY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_TAGS_ENTRY))
#define GTH_IS_TAGS_ENTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_TAGS_ENTRY))
#define GTH_TAGS_ENTRY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTH_TYPE_TAGS_ENTRY, GthTagsEntryClass))

typedef struct _GthTagsEntry GthTagsEntry;
typedef struct _GthTagsEntryClass GthTagsEntryClass;
typedef struct _GthTagsEntryPrivate GthTagsEntryPrivate;

struct _GthTagsEntry {
	GtkEntry parent_instance;
	GthTagsEntryPrivate *priv;
};

struct _GthTagsEntryClass {
	GtkEntryClass parent_class;
};

GType        gth_tags_entry_get_type  (void);
GtkWidget *  gth_tags_entry_new       (void);
char **      gth_tags_entry_get_tags  (GthTagsEntry  *self,
				       gboolean       update_globals);
void         gth_tags_entry_set_tags  (GthTagsEntry  *self,
				       char         **tags);

G_END_DECLS

#endif /* GTH_TAGS_ENTRY_H */
