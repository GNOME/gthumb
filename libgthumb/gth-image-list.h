/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2003 The Free Software Foundation, Inc.
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

#ifndef GTH_IMAGE_LIST_H
#define GTH_IMAGE_LIST_H

#include <gtk/gtkcontainer.h>
#include "typedefs.h"

G_BEGIN_DECLS

#define GTH_TYPE_IMAGE_LIST            (gth_image_list_get_type ())
#define GTH_IMAGE_LIST(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_IMAGE_LIST, GthImageList))
#define GTH_IMAGE_LIST_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_IMAGE_LIST, GthImageListClass))
#define GTH_IS_IMAGE_LIST(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_IMAGE_LIST))
#define GTH_IS_IMAGE_LIST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_IMAGE_LIST))
#define GTH_IMAGE_LIST_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_IMAGE_LIST, GthImageListClass))


typedef enum {
	GTH_CURSOR_MOVE_UP,
	GTH_CURSOR_MOVE_DOWN,
	GTH_CURSOR_MOVE_RIGHT,
	GTH_CURSOR_MOVE_LEFT,
	GTH_CURSOR_MOVE_PAGE_UP,
	GTH_CURSOR_MOVE_PAGE_DOWN,
	GTH_CURSOR_MOVE_BEGIN,
	GTH_CURSOR_MOVE_END,
} GthCursorMovement;

typedef enum {
	GTH_SELCHANGE_NONE,           /* Do not change the selection. */
	GTH_SELCHANGE_SET,            /* Select the cursor image. */
	GTH_SELCHANGE_SET_RANGE       /* Select the images contained
				       * in the rectangle that has as
				       * opposite corners the last
				       * focused image and the
				       * currently focused image. */
} GthSelectionChange;


/* FIXME */
#define GTH_IMAGE_LIST_ITEM(x) ((GthImageListItem*)(x))
typedef struct {
	/*< public (read only) >*/

	char             *label;
	char             *comment;

	gpointer          data;

	guint             focused : 1;
	guint             selected : 1;

	/*< private >*/

	GType             data_type;
	guint             ref;

	GdkPixmap        *pixmap;       /* Pixmap rendered from the image */
	GdkBitmap        *mask;	        /* Mask rendered from the image */

	GdkRectangle      slide_area;
	GdkRectangle      image_area;
	GdkRectangle      label_area;
	GdkRectangle      comment_area;

	guint             tmp_selected : 1;
} GthImageListItem;


typedef struct _GthImageListPrivate GthImageListPrivate;


typedef struct {
	GtkContainer __parent;
	GthImageListPrivate *priv;
} GthImageList;


typedef struct {
	GtkContainerClass __parent_class;

	/* -- Signals -- */

	void     (* set_scroll_adjustments)  (GthImageList  *image_list,
					      GtkAdjustment *hadj,
					      GtkAdjustment *vadj);
	void     (* selection_changed)       (GthImageList  *image_list);
	void     (* item_activated)          (GthImageList  *image_list,
					      int            pos);
	void     (* cursor_changed)          (GthImageList  *image_list,
					      int            pos);

        /* -- Key Binding signals -- */

        gboolean (* move_cursor)             (GthImageList       *image_list,
					      GthCursorMovement   dir,
					      GthSelectionChange  sel_change);
	gboolean (* select_all)              (GthImageList       *image_list);
	gboolean (* unselect_all)            (GthImageList       *image_list);
	gboolean (* set_cursor_selection)    (GthImageList       *image_list);
	gboolean (* toggle_cursor_selection) (GthImageList       *image_list);
	gboolean (* start_interactive_search)(GthImageList       *image_list);
} GthImageListClass;


GType          gth_image_list_get_type             (void);
GtkWidget     *gth_image_list_new                  (guint          image_width,
						    GType          data_type);
void           gth_image_list_set_hadjustment      (GthImageList  *image_list,
						    GtkAdjustment *hadj);
GtkAdjustment *gth_image_list_get_hadjustment      (GthImageList  *image_list);
void           gth_image_list_set_vadjustment      (GthImageList  *image_list,
						    GtkAdjustment *vadj);
GtkAdjustment *gth_image_list_get_vadjustment      (GthImageList  *image_list);

/* To avoid excesive recomputes during insertion/deletion */

void           gth_image_list_freeze               (GthImageList  *image_list);
void           gth_image_list_thaw                 (GthImageList  *image_list,
		     				    gboolean       relayout_now);
gboolean       gth_image_list_is_frozen            (GthImageList  *image_list);

/**/

void           gth_image_list_insert               (GthImageList  *image_list,
						    int            pos,
						    GdkPixbuf     *pixbuf,
						    const char    *text,
						    const char    *comment);
int            gth_image_list_append               (GthImageList  *image_list,
						    GdkPixbuf     *pixbuf,
						    const char    *text,
						    const char    *comment);
int            gth_image_list_append_with_data     (GthImageList  *image_list,
						    GdkPixbuf     *pixbuf,
						    const char    *text,
						    const char    *comment,
						    gpointer       data);
void           gth_image_list_remove               (GthImageList  *image_list,
						    gpointer       data);
void           gth_image_list_clear                (GthImageList  *image_list);
void           gth_image_list_set_image_pixbuf     (GthImageList  *image_list,
						    int            pos,
						   GdkPixbuf      *pixbuf);
void           gth_image_list_set_image_text       (GthImageList  *image_list,
						    int            pos,
						    const char    *text);
const char*    gth_image_list_get_image_text       (GthImageList  *image_list,
						    int            pos);
void           gth_image_list_set_image_comment    (GthImageList  *image_list,
						    int            pos,
						    const char    *comment);
const char*    gth_image_list_get_image_comment    (GthImageList  *image_list,
						    int            pos);
int            gth_image_list_get_images           (GthImageList  *image_list);
GList *        gth_image_list_get_list             (GthImageList  *image_list);
GList *        gth_image_list_get_selection        (GthImageList  *image_list);

/* Managing the selection */

void           gth_image_list_set_selection_mode   (GthImageList     *image_list,
						    GtkSelectionMode  mode);
void           gth_image_list_select_image         (GthImageList     *image_list,
						    int               pos);
void           gth_image_list_unselect_image       (GthImageList     *image_list,
						    int               pos);
void           gth_image_list_select_all           (GthImageList     *gimage_list);
void           gth_image_list_unselect_all         (GthImageList     *image_list);
gboolean       gth_image_list_pos_is_selected      (GthImageList     *image_list,
						    int               pos);
int            gth_image_list_get_first_selected   (GthImageList     *image_list);
int            gth_image_list_get_last_selected    (GthImageList     *image_list);
int            gth_image_list_get_n_selected       (GthImageList     *image_list);

/* Setting spacing values */

void           gth_image_list_set_image_width      (GthImageList     *image_list,
						    int               width);

/* Attaching information to the items */

void           gth_image_list_set_image_data       (GthImageList    *image_list,
						    int              pos,
						    gpointer         data);
int            gth_image_list_find_image_from_data (GthImageList    *image_list,
						    gpointer         data);
gpointer       gth_image_list_get_image_data       (GthImageList    *image_list,
						    int              pos);

/* Visibility */

void           gth_image_list_enable_thumbs        (GthImageList *image_list,
						    gboolean      enable_thumbs);
void           gth_image_list_set_view_mode        (GthImageList *image_list,
						    GthViewMode   mode);
GthViewMode    gth_image_list_get_view_mode        (GthImageList *image_list);
void           gth_image_list_moveto               (GthImageList *image_list,
						    int           pos,
						    double        yalign);
GthVisibility  gth_image_list_image_is_visible     (GthImageList *image_list,
						    int           pos);
int            gth_image_list_get_image_at         (GthImageList *image_list,
						    int           x,
						    int           y);
int            gth_image_list_get_first_visible    (GthImageList *image_list);
int            gth_image_list_get_last_visible     (GthImageList *image_list);
int            gth_image_list_get_items_per_line   (GthImageList *image_list);

/* Sort */

void           gth_image_list_sorted               (GthImageList *image_list,
						    GCompareFunc  cmp_func,
						    GtkSortType   sort_type);
void           gth_image_list_unsorted             (GthImageList *image_list);

/* Misc */

void           gth_image_list_image_activated      (GthImageList *image_list,
						    int           pos);
void           gth_image_list_set_cursor           (GthImageList *image_list,
						    int           pos);
int            gth_image_list_get_cursor           (GthImageList *image_list);
void           gth_image_list_set_no_image_text    (GthImageList *image_list,
						    const char   *text);
void           gth_image_list_set_visible_func     (GthImageList   *image_list,
			                      	    GthVisibleFunc  func,
                      				    gpointer        data);

/* DnD */

void           gth_image_list_set_drag_dest_pos    (GthImageList *image_list,
						    int           x,
						    int           y);
void           gth_image_list_get_drag_dest_pos    (GthImageList *image_list,
						    int          *pos);
void           gth_image_list_set_reorderable      (GthImageList *image_list,
						    gboolean      value);
gboolean       gth_image_list_get_reorderable      (GthImageList *image_list);

/* Interactive search */

void           gth_image_list_set_enable_search    (GthImageList *image_list,
						    gboolean      enable_search);
gboolean       gth_image_list_get_enable_search    (GthImageList *image_list);

G_END_DECLS

#endif /* GTH_IMAGE_LIST_H */
