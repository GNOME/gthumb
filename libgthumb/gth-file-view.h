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

#ifndef GTH_FILE_VIEW_H
#define GTH_FILE_VIEW_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include "typedefs.h"

G_BEGIN_DECLS

#define GTH_TYPE_FILE_VIEW            (gth_file_view_get_type ())
#define GTH_FILE_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_FILE_VIEW, GthFileView))
#define GTH_FILE_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_FILE_VIEW, GthFileViewClass))
#define GTH_IS_FILE_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_FILE_VIEW))
#define GTH_IS_FILE_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_FILE_VIEW))
#define GTH_FILE_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_FILE_VIEW, GthFileViewClass))

typedef struct _GthFileViewPrivate GthFileViewPrivate;

typedef struct {
	GObject __parent;
} GthFileView;

typedef struct {
	GObjectClass __parent_class;

	/* -- Signals -- */

	void     (* selection_changed)       (GthFileView   *file_view);
	void     (* item_activated)          (GthFileView   *file_view,
					      int            pos);
	void     (* cursor_changed)          (GthFileView   *file_view,
					      int            pos);

	/* -- Virtual Functions -- */

	void           (* set_hadjustment)     (GthFileView   *file_view,
						GtkAdjustment *hadj);
	GtkAdjustment* (* get_hadjustment)     (GthFileView   *file_view);
	void           (* set_vadjustment)     (GthFileView   *file_view,
						GtkAdjustment *vadj);
	GtkAdjustment* (* get_vadjustment)     (GthFileView   *file_view);
	GtkWidget*     (* get_widget)          (GthFileView   *file_view);
	GtkWidget*     (* get_drag_source)     (GthFileView   *file_view);

	/* To avoid excesive recomputes during insertion/deletion */

	void           (* freeze)               (GthFileView  *file_view);
	void           (* thaw)                 (GthFileView  *file_view);
	gboolean       (* is_frozen)            (GthFileView  *file_view);

	/**/

	void           (* insert)               (GthFileView  *file_view,
						 int           pos,
						 GdkPixbuf    *pixbuf,
						 const char   *text,
						 const char   *comment,
                                                 const char   *categories);
	int            (* append)               (GthFileView  *file_view,
						 GdkPixbuf    *pixbuf,
						 const char   *text,
						 const char   *comment,
                                                 const char   *categories);
	int            (* append_with_data)     (GthFileView  *file_view,
						 GdkPixbuf    *pixbuf,
						 const char   *text,
						 const char   *comment,
                                                 const char   *categories,
						 gpointer      data);
	void           (* remove)               (GthFileView  *file_view,
						 gpointer      data);
	void           (* clear)                (GthFileView  *file_view);
	void           (* set_image_pixbuf)     (GthFileView  *file_view,
						 int           pos,
						 GdkPixbuf    *pixbuf);
	void           (* set_unknown_pixbuf)   (GthFileView  *file_view,
						 int           pos);
	void           (* set_image_text)       (GthFileView  *file_view,
						 int           pos,
						 const char   *text);
	const char*    (* get_image_text)       (GthFileView  *file_view,
						 int           pos);
	void           (* set_image_comment)    (GthFileView  *file_view,
						 int           pos,
						 const char   *comment);
	void           (* set_image_categories) (GthFileView  *file_view,
						 int           pos,
						 const char   *categories);
	const char*    (* get_image_comment)    (GthFileView  *file_view,
						 int           pos);
	int            (* get_images)           (GthFileView  *file_view);
	GList *        (* get_list)             (GthFileView  *file_view);
	GList *        (* get_selection)        (GthFileView  *file_view);

	/* Managing the selection */

	void           (* select_image)         (GthFileView     *file_view,
						 int              pos);
	void           (* unselect_image)       (GthFileView     *file_view,
						 int              pos);
	void           (* select_all)           (GthFileView     *file_view);
	void           (* unselect_all)         (GthFileView     *file_view);
	GList *        (* get_file_list_selection) (GthFileView  *file_view);
	gboolean       (* pos_is_selected)      (GthFileView     *file_view,
						 int              pos);
	int            (* get_first_selected)   (GthFileView     *file_view);
	int            (* get_last_selected)    (GthFileView     *file_view);
	int            (* get_n_selected)       (GthFileView     *file_view);

	/* Setting spacing values */

	void           (* set_image_width)      (GthFileView     *file_view,
						 int              width);

	/* Attaching information to the items */

	void           (* set_image_data)       (GthFileView     *file_view,
						 int              pos,
						 gpointer         data);
	int            (* find_image_from_data) (GthFileView     *file_view,
						 gpointer         data);
	gpointer       (* get_image_data)       (GthFileView     *file_view,
						 int              pos);

	/* Visibility */

	void           (* enable_thumbs)        (GthFileView    *file_view,
						 gboolean        enable_thumbs);
	void           (* set_view_mode)        (GthFileView    *file_view,
						 int             mode);
	int            (* get_view_mode)        (GthFileView    *file_view);
	void           (* moveto)               (GthFileView    *file_view,
						 int             pos,
						 double          yalign);
	GthVisibility  (* image_is_visible)     (GthFileView    *file_view,
						 int             pos);
	int            (* get_image_at)         (GthFileView    *file_view,
						 int             x,
						 int             y);
	int            (* get_first_visible)    (GthFileView    *file_view);
	int            (* get_last_visible)     (GthFileView    *file_view);
        void           (* set_visible_func)     (GthFileView    *file_view,
                                                 GthVisibleFunc  func,
                                                 gpointer        data);

	/* Sort */

	void           (* sorted)               (GthFileView   *file_view,
						 GthSortMethod  sort_method,
						 GtkSortType    sort_type);
	void           (* unsorted)             (GthFileView   *file_view);

	/* Misc */

	void           (* image_activated)      (GthFileView *file_view,
						 int          pos);
	void           (* set_cursor)           (GthFileView *file_view,
						 int          pos);
	int            (* get_cursor)           (GthFileView *file_view);
	void           (* set_no_image_text)    (GthFileView *file_view,
						 const char  *text);

	/* DnD */

	void           (* set_drag_dest_pos)    (GthFileView *file_view,
						 int          x,
						 int          y);
	void           (* get_drag_dest_pos)    (GthFileView *file_view,
						 int         *pos);
	void           (* set_reorderable)      (GthFileView  *file_view,
						 gboolean      value);
	gboolean       (* get_reorderable)      (GthFileView  *file_view);

	/* Interactive search */

	void           (* set_enable_search)    (GthFileView *file_view,
						 gboolean     enable_search);
	gboolean       (* get_enable_search)    (GthFileView *file_view);
} GthFileViewClass;


GType          gth_file_view_get_type            (void);
void           gth_file_view_set_hadjustment     (GthFileView   *file_view,
						  GtkAdjustment *hadj);
GtkAdjustment *gth_file_view_get_hadjustment     (GthFileView   *file_view);
void           gth_file_view_set_vadjustment     (GthFileView   *file_view,
						  GtkAdjustment *vadj);
GtkAdjustment *gth_file_view_get_vadjustment     (GthFileView   *file_view);
GtkWidget     *gth_file_view_get_widget          (GthFileView   *file_view);
GtkWidget     *gth_file_view_get_drag_source     (GthFileView   *file_view);

/* To avoid excesive recomputes during insertion/deletion */

void           gth_file_view_freeze              (GthFileView  *file_view);
void           gth_file_view_thaw                (GthFileView  *file_view);
gboolean       gth_file_view_is_frozen           (GthFileView  *file_view);

/**/

void           gth_file_view_insert              (GthFileView  *file_view,
						  int           pos,
						  GdkPixbuf    *pixbuf,
						  const char   *text,
						  const char   *comment,
                                                  const char   *categories);
int            gth_file_view_append              (GthFileView  *file_view,
						  GdkPixbuf    *pixbuf,
						  const char   *text,
						  const char   *comment,
                                                  const char   *categories);
int            gth_file_view_append_with_data    (GthFileView  *file_view,
						  GdkPixbuf    *pixbuf,
						  const char   *text,
						  const char   *comment,
                                                  const char   *categories,
						  gpointer      data);
void           gth_file_view_remove              (GthFileView  *file_view,
						  gpointer      data);
void           gth_file_view_clear               (GthFileView  *file_view);
void           gth_file_view_set_image_pixbuf    (GthFileView  *file_view,
						  int           pos,
						  GdkPixbuf    *pixbuf);
void           gth_file_view_set_unknown_pixbuf  (GthFileView  *file_view,
						  int           pos);
void           gth_file_view_set_image_text      (GthFileView  *file_view,
						  int           pos,
						  const char   *text);
const char*    gth_file_view_get_image_text      (GthFileView  *file_view,
						  int           pos);
void           gth_file_view_set_image_comment   (GthFileView  *file_view,
						  int           pos,
						  const char   *comment);
const char*    gth_file_view_get_image_comment   (GthFileView  *file_view,
						  int           pos);
void           gth_file_view_set_image_categories(GthFileView  *file_view,
						  int           pos,
						  const char   *categories);
int            gth_file_view_get_images          (GthFileView  *file_view);
GList *        gth_file_view_get_list            (GthFileView  *file_view);
GList *        gth_file_view_get_selection       (GthFileView  *file_view);

/* Managing the selection */

void           gth_file_view_select_image         (GthFileView     *file_view,
						   int              pos);
void           gth_file_view_unselect_image       (GthFileView     *file_view,
						   int              pos);
void           gth_file_view_select_all           (GthFileView     *file_view);

void           gth_file_view_unselect_all         (GthFileView     *file_view);
GList *        gth_file_view_get_file_list_selection (GthFileView *file_view);
gboolean       gth_file_view_pos_is_selected      (GthFileView     *file_view,
						   int              pos);
int            gth_file_view_get_first_selected   (GthFileView    *file_view);
int            gth_file_view_get_last_selected    (GthFileView    *file_view);
int            gth_file_view_get_n_selected       (GthFileView    *file_view);

/* Setting spacing values */

void           gth_file_view_set_image_width     (GthFileView     *file_view,
						  int              width);

/* Attaching information to the items */

void           gth_file_view_set_image_data       (GthFileView     *file_view,
						   int              pos,
						   gpointer         data);
int            gth_file_view_find_image_from_data (GthFileView     *file_view,
						   gpointer         data);
gpointer       gth_file_view_get_image_data       (GthFileView     *file_view,
						   int              pos);

/* Visibility */

void           gth_file_view_enable_thumbs        (GthFileView *file_view,
						   gboolean     enable_thumbs);
void           gth_file_view_set_view_mode        (GthFileView *file_view,
						   int          mode);
int            gth_file_view_get_view_mode        (GthFileView *file_view);
void           gth_file_view_moveto               (GthFileView *file_view,
						   int          pos,
						   double       yalign);
GthVisibility  gth_file_view_image_is_visible     (GthFileView *file_view,
						   int          pos);
int            gth_file_view_get_image_at         (GthFileView *file_view,
						   int          x,
						   int          y);
int            gth_file_view_get_first_visible    (GthFileView *file_view);
int            gth_file_view_get_last_visible     (GthFileView *file_view);

/* Filter */

void           gth_file_view_set_visible_func     (GthFileView    *file_view,
                                                   GthVisibleFunc  func,
                                                   gpointer        data);

/* Sort */

void           gth_file_view_sorted               (GthFileView   *file_view,
						   GthSortMethod  sort_method,
						   GtkSortType    sort_type);
void           gth_file_view_unsorted             (GthFileView *file_view);

/* Misc */

void           gth_file_view_image_activated      (GthFileView *file_view,
						   int          pos);
void           gth_file_view_set_cursor           (GthFileView *file_view,
						   int          pos);
int            gth_file_view_get_cursor           (GthFileView *file_view);
void           gth_file_view_set_no_image_text    (GthFileView *file_view,
						   const char  *text);

/* DnD */

void           gth_file_view_set_drag_dest_pos    (GthFileView  *file_view,
						   int           x,
						   int           y);
void           gth_file_view_get_drag_dest_pos    (GthFileView  *file_view,
						   int          *pos);
void           gth_file_view_set_reorderable      (GthFileView  *file_view,
						   gboolean      value);
gboolean       gth_file_view_get_reorderable      (GthFileView  *file_view);

/* Interactive search */

void           gth_file_view_set_enable_search    (GthFileView *file_view,
						   gboolean     enable_search);
gboolean       gth_file_view_get_enable_search    (GthFileView *file_view);

/* Protected methods */

void           gth_file_view_selection_changed    (GthFileView   *file_view);
void           gth_file_view_item_activated       (GthFileView   *file_view,
						   int            pos);
void           gth_file_view_cursor_changed       (GthFileView   *file_view,
						   int            pos);

G_END_DECLS

#endif /* GTH_FILE_VIEW_H */
