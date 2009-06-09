/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2008 Free Software Foundation, Inc.
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

#ifndef GTH_FILE_VIEW_H
#define GTH_FILE_VIEW_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GTH_TYPE_FILE_VIEW               (gth_file_view_get_type ())
#define GTH_FILE_VIEW(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_FILE_VIEW, GthFileView))
#define GTH_IS_FILE_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_FILE_VIEW))
#define GTH_FILE_VIEW_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GTH_TYPE_FILE_VIEW, GthFileViewIface))

typedef struct _GthFileView GthFileView;
typedef struct _GthFileViewIface GthFileViewIface;

typedef enum  { /*< skip >*/
	GTH_VIEW_MODE_NONE,
	GTH_VIEW_MODE_FILENAME,
	GTH_VIEW_MODE_COMMENT,
	GTH_VIEW_MODE_COMMENT_OR_FILENAME,
	GTH_VIEW_MODE_COMMENT_AND_FILENAME
} GthViewMode;

typedef enum  { /*< skip >*/
	GTH_VISIBILITY_NONE,
	GTH_VISIBILITY_FULL,
	GTH_VISIBILITY_PARTIAL,
	GTH_VISIBILITY_PARTIAL_TOP,
	GTH_VISIBILITY_PARTIAL_BOTTOM
} GthVisibility;

struct _GthFileViewIface {
	GTypeInterface parent_iface;

	/*< virtual functions >*/

	void           (*set_model)         (GthFileView  *self,
					     GtkTreeModel *model);
	GtkTreeModel * (*get_model)         (GthFileView  *self);
	void           (*set_view_mode)     (GthFileView  *self,
					     GthViewMode   mode);
	GthViewMode    (*get_view_mode)     (GthFileView  *self);
	void           (*scroll_to)         (GthFileView  *self,
					     int           pos,
					     double        yalign);
	GthVisibility  (*get_visibility)    (GthFileView  *self,
					     int           pos);
	int            (*get_at_position)   (GthFileView  *self,
					     int           x,
					     int           y);
	int            (*get_first_visible) (GthFileView  *self);
	int            (*get_last_visible)  (GthFileView  *self);
	void           (*activated)         (GthFileView  *self,
					     int           pos);
	void           (*set_cursor)        (GthFileView  *self,
					     int           pos);
	int            (*get_cursor)        (GthFileView  *self);
	void           (*set_reorderable)   (GthFileView  *self,
					     gboolean      value);
	gboolean       (*get_reorderable)   (GthFileView  *self);
};

GType          gth_file_view_get_type          (void);
void           gth_file_view_set_model         (GthFileView  *self,
						GtkTreeModel *model);
GtkTreeModel * gth_file_view_get_model         (GthFileView  *self);
void           gth_file_view_set_view_mode     (GthFileView *self,
						GthViewMode  mode);
GthViewMode    gth_file_view_get_view_mode     (GthFileView *self);
void           gth_file_view_scroll_to         (GthFileView *self,
						int          pos,
						double       yalign);
GthVisibility  gth_file_view_get_visibility    (GthFileView *self,
						int          pos);
int            gth_file_view_get_at_position   (GthFileView *self,
						int          x,
						int          y);
int            gth_file_view_get_first_visible (GthFileView *self);
int            gth_file_view_get_last_visible  (GthFileView *self);
void           gth_file_view_activated         (GthFileView *self,
						int          pos);
void           gth_file_view_set_cursor        (GthFileView *self,
						int          pos);
int            gth_file_view_get_cursor        (GthFileView *self);
void           gth_file_view_set_reorderable   (GthFileView *self,
						gboolean     value);
gboolean       gth_file_view_get_reorderable   (GthFileView *self);

G_END_DECLS

#endif /* GTH_FILE_VIEW_H */
