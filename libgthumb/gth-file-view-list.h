/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2003 Free Software Foundation, Inc.
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

#ifndef GTH_FILE_VIEW_LIST_H
#define GTH_FILE_VIEW_LIST_H

G_BEGIN_DECLS

#define GTH_TYPE_FILE_VIEW_LIST            (gth_file_view_list_get_type ())
#define GTH_FILE_VIEW_LIST(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_FILE_VIEW_LIST, GthFileViewList))
#define GTH_FILE_VIEW_LIST_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_FILE_VIEW_LIST, GthFileViewListClass))
#define GTH_IS_FILE_VIEW_LIST(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_FILE_VIEW_LIST))
#define GTH_IS_FILE_VIEW_LIST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_FILE_VIEW_LIST))
#define GTH_FILE_VIEW_LIST_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_FILE_VIEW_LIST, GthFileViewListClass))

typedef struct _GthFileViewListPrivate GthFileViewListPrivate;

typedef struct {
	GthFileView __parent;
	GthFileViewListPrivate *priv;
} GthFileViewList;

typedef struct {
	GthFileViewClass __parent_class;
} GthFileViewListClass;

GType          gth_file_view_list_get_type     (void);
GthFileView   *gth_file_view_list_new          (guint image_width);

G_END_DECLS

#endif /* GTH_FILE_VIEW_LIST_H */
