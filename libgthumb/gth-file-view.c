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

#include <glib.h>

#include "gth-file-view.h"
#include "gthumb-marshal.h"
#include "gthumb-enum-types.h"


/* Signals */
enum {
	SELECTION_CHANGED,
	ITEM_ACTIVATED,
	CURSOR_CHANGED,
	LAST_SIGNAL
};

static guint file_view_signals[LAST_SIGNAL] = { 0 };
static GObjectClass *parent_class;


static void
gfv_set_hadjustment (GthFileView   *file_view,
		     GtkAdjustment *hadj)
{
}


static GtkAdjustment *
gfv_get_hadjustment (GthFileView   *file_view)
{
	return NULL;
}


static void
gfv_set_vadjustment (GthFileView   *file_view,
		     GtkAdjustment *vadj)
{
}


static GtkAdjustment *
gfv_get_vadjustment (GthFileView   *file_view)
{
	return NULL;
}


static GtkWidget *
gfv_get_widget (GthFileView   *file_view)
{
	return NULL;
}


/* To avoid excesive recomputes during insertion/deletion */


static void
gfv_freeze (GthFileView  *file_view)
{
}


static void
gfv_thaw (GthFileView  *file_view)
{
}


static gboolean
gfv_is_frozen (GthFileView  *file_view)
{
	return FALSE;
}


/**/


static void
gfv_insert (GthFileView  *file_view,
	    int           pos, 
	    GdkPixbuf    *pixbuf,
	    const char   *text,
	    const char   *comment)
{
}


static int
gfv_append (GthFileView  *file_view,
	    GdkPixbuf    *pixbuf,
	    const char   *text,
	    const char   *comment)
{
	return -1;
}


static int
gfv_append_with_data (GthFileView  *file_view,
		      GdkPixbuf    *pixbuf,
		      const char   *text,
		      const char   *comment,
		      gpointer      data)
{
	return -1;
}


static void
gfv_remove (GthFileView  *file_view, 
	    int           pos)
{
}


static void
gfv_clear (GthFileView  *file_view)
{
}


static void
gfv_set_image_pixbuf (GthFileView  *file_view,
		      int           pos,
		      GdkPixbuf    *pixbuf)
{
}


static void
gfv_set_unknown_pixbuf (GthFileView  *file_view,
			int           pos)
{
}


static void
gfv_set_image_text (GthFileView  *file_view,
		    int           pos,
		    const char   *text)
{
}


static const char*
gfv_get_image_text (GthFileView  *file_view,
		    int           pos)
{
	return NULL;
}


static void
gfv_set_image_comment (GthFileView  *file_view,
		       int           pos,
		       const char   *comment)
{
}


static const char*
gfv_get_image_comment (GthFileView  *file_view,
		       int           pos)
{
	return NULL;
}


static int
gfv_get_images (GthFileView  *file_view)
{
	return 0;
}


static GList *
gfv_get_list (GthFileView  *file_view)
{
	return NULL;
}


static GList *
gfv_get_selection (GthFileView  *file_view)
{
	return NULL;
}


/* Managing the selection */


static void
gfv_select_image (GthFileView     *file_view,
		  int              pos)
{
}


static void
gfv_unselect_image (GthFileView     *file_view,
		    int              pos)
{
}


static void
gfv_select_all (GthFileView     *file_view)
{
}


static void
gfv_unselect_all (GthFileView     *file_view)
{
}


static GList *
gfv_get_file_list_selection (GthFileView *file_view)
{
	return NULL;
}


static gboolean
gfv_pos_is_selected (GthFileView     *file_view,
		     int              pos)
{
	return FALSE;
}


static gboolean
gfv_only_one_is_selected (GthFileView    *file_view)
{
	return FALSE;
}


static gboolean
gfv_selection_not_null (GthFileView    *file_view)
{
	return FALSE;
}


static int
gfv_get_first_selected (GthFileView *file_view)
{
	return -1;
}


static int
gfv_get_last_selected (GthFileView *file_view)
{
	return -1;
}


/* Setting spacing values */


static void
gfv_set_image_width (GthFileView     *file_view,
		     int              width)
{
}


/* Attaching information to the items */


static void
gfv_set_image_data (GthFileView     *file_view,
		    int              pos, 
		    gpointer         data)
{
}


static void
gfv_set_image_data_full (GthFileView     *file_view,
			 int              pos, 
			 gpointer         data,
			 GtkDestroyNotify destroy)
{
}


static int
gfv_find_image_from_data (GthFileView     *file_view,
			  gpointer         data)
{
	return -1;
}


static gpointer
gfv_get_image_data (GthFileView     *file_view,
		    int              pos)
{
	return NULL;
}


/* Visibility */


static void
gfv_enable_thumbs (GthFileView *file_view,
		   gboolean     enable_thumbs)
{
}


static void
gfv_set_view_mode (GthFileView *file_view,
		   GthViewMode  mode)
{
}


static GthViewMode
gfv_get_view_mode (GthFileView *file_view)
{
	return GTH_VIEW_MODE_COMMENTS;
}


static void
gfv_moveto (GthFileView *file_view,
	    int          pos, 
	    double       yalign)
{
}


static GthVisibility
gfv_image_is_visible (GthFileView *file_view,
		      int          pos)
{
	return GTH_VISIBILITY_NONE;
}


static int
gfv_get_image_at (GthFileView *file_view, 
		  int          x, 
		  int          y)
{
	return -1;
}


static int
gfv_get_first_visible (GthFileView *file_view)
{
	return -1;
}


static int
gfv_get_last_visible (GthFileView *file_view)
{
	return -1;
}


/* Sort */


static void
gfv_sorted (GthFileView   *file_view,
	    GthSortMethod  sort_method,
	    GtkSortType    sort_type)
{
}


static void
gfv_unsorted (GthFileView *file_view)
{
}


/* Misc */


static void
gfv_image_activated (GthFileView *file_view, 
		     int          pos)
{
}


static void
gfv_set_cursor (GthFileView *file_view, 
		int          pos)
{
}


static int
gfv_get_cursor (GthFileView *file_view)
{
	return -1;
}


static void
gfv_update_icon_theme (GthFileView *file_view)
{
}


static void
gfv_set_no_image_text (GthFileView *file_view,
		       const char  *text)
{
}


/* Interactive search */


static void
gfv_set_enable_search (GthFileView *file_view,
		       gboolean     enable_search)
{
}


static gboolean
gfv_get_enable_search (GthFileView *file_view)
{
	return FALSE;
}


/**/


static void
gth_file_view_class_init (GthFileViewClass *file_view_class)
{
	GObjectClass *gobject_class;

	parent_class = g_type_class_peek_parent (file_view_class);
	gobject_class = (GObjectClass*) file_view_class;

	/* Methods */

	file_view_class->set_hadjustment      = gfv_set_hadjustment;
	file_view_class->get_hadjustment      = gfv_get_hadjustment;
	file_view_class->set_vadjustment      = gfv_set_vadjustment;
	file_view_class->get_vadjustment      = gfv_get_vadjustment;
	file_view_class->get_widget           = gfv_get_widget;
	file_view_class->freeze               = gfv_freeze;
	file_view_class->thaw                 = gfv_thaw;
	file_view_class->is_frozen            = gfv_is_frozen;
	file_view_class->insert               = gfv_insert;
	file_view_class->append               = gfv_append;
	file_view_class->append_with_data     = gfv_append_with_data;
	file_view_class->remove               = gfv_remove;
	file_view_class->clear                = gfv_clear;
	file_view_class->set_image_pixbuf     = gfv_set_image_pixbuf;
	file_view_class->set_unknown_pixbuf   = gfv_set_unknown_pixbuf;
	file_view_class->set_image_text       = gfv_set_image_text;
	file_view_class->get_image_text       = gfv_get_image_text;
	file_view_class->set_image_comment    = gfv_set_image_comment;
	file_view_class->get_image_comment    = gfv_get_image_comment;
	file_view_class->get_images           = gfv_get_images;
	file_view_class->get_list             = gfv_get_list;
	file_view_class->get_selection        = gfv_get_selection;
	file_view_class->select_image         = gfv_select_image;
	file_view_class->unselect_image       = gfv_unselect_image;
	file_view_class->select_all           = gfv_select_all;
	file_view_class->unselect_all         = gfv_unselect_all;
	file_view_class->get_file_list_selection = gfv_get_file_list_selection;
	file_view_class->pos_is_selected      = gfv_pos_is_selected;
	file_view_class->only_one_is_selected = gfv_only_one_is_selected;
	file_view_class->selection_not_null   = gfv_selection_not_null;
	file_view_class->get_first_selected   = gfv_get_first_selected;
	file_view_class->get_last_selected    = gfv_get_last_selected;
	file_view_class->set_image_width      = gfv_set_image_width;
	file_view_class->set_image_data       = gfv_set_image_data;
	file_view_class->set_image_data_full  = gfv_set_image_data_full;
	file_view_class->find_image_from_data = gfv_find_image_from_data;
	file_view_class->get_image_data       = gfv_get_image_data;
	file_view_class->enable_thumbs        = gfv_enable_thumbs;
	file_view_class->set_view_mode        = gfv_set_view_mode;
	file_view_class->get_view_mode        = gfv_get_view_mode;
	file_view_class->moveto               = gfv_moveto;
	file_view_class->image_is_visible     = gfv_image_is_visible;
	file_view_class->get_image_at         = gfv_get_image_at;
	file_view_class->get_first_visible    = gfv_get_first_visible;
	file_view_class->get_last_visible     = gfv_get_last_visible;
	file_view_class->sorted               = gfv_sorted;
	file_view_class->unsorted             = gfv_unsorted;
	file_view_class->image_activated      = gfv_image_activated;
	file_view_class->set_cursor           = gfv_set_cursor;
	file_view_class->get_cursor           = gfv_get_cursor;
	file_view_class->update_icon_theme    = gfv_update_icon_theme;
	file_view_class->set_no_image_text    = gfv_set_no_image_text;
	file_view_class->set_enable_search    = gfv_set_enable_search;
	file_view_class->get_enable_search    = gfv_get_enable_search;

	/* Signals */

	file_view_signals[SELECTION_CHANGED] =
		g_signal_new ("selection_changed",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (GthFileViewClass, selection_changed),
			      NULL, NULL,
			      gthumb_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);
	file_view_signals[ITEM_ACTIVATED] =
		g_signal_new ("item_activated",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (GthFileViewClass, item_activated),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__INT,
			      G_TYPE_NONE, 1,
			      G_TYPE_INT);
	file_view_signals[CURSOR_CHANGED] =
		g_signal_new ("cursor_changed",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (GthFileViewClass, cursor_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__INT,
			      G_TYPE_NONE, 1,
			      G_TYPE_INT);
}


static void
gth_file_view_init (GthFileView *file_view)
{
}


GType
gth_file_view_get_type (void)
{
	static guint type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (GthFileViewClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_file_view_class_init,
			NULL,
			NULL,
			sizeof (GthFileView),
			0,
			(GInstanceInitFunc) gth_file_view_init
                };

		type = g_type_register_static (G_TYPE_OBJECT,
					       "GthFileView",
					       &type_info,
					       0);
        }

	return type;
}


/**/


void
gth_file_view_set_hadjustment (GthFileView   *file_view,
			       GtkAdjustment *hadj)
{
	GTH_FILE_VIEW_GET_CLASS (file_view)->set_hadjustment (file_view, hadj);
}


GtkAdjustment *
gth_file_view_get_hadjustment (GthFileView *file_view)
{
	return 	GTH_FILE_VIEW_GET_CLASS (file_view)->get_hadjustment (file_view);
}


void
gth_file_view_set_vadjustment (GthFileView   *file_view,
			       GtkAdjustment *vadj)
{
	GTH_FILE_VIEW_GET_CLASS (file_view)->set_vadjustment (file_view, vadj);
}


GtkAdjustment *
gth_file_view_get_vadjustment (GthFileView *file_view)
{
	return 	GTH_FILE_VIEW_GET_CLASS (file_view)->get_vadjustment (file_view);
}


GtkWidget *
gth_file_view_get_widget (GthFileView *file_view)
{
	return 	GTH_FILE_VIEW_GET_CLASS (file_view)->get_widget (file_view);
}


/* To avoid excesive recomputes during insertion/deletion */


void
gth_file_view_freeze (GthFileView  *file_view)
{
	GTH_FILE_VIEW_GET_CLASS (file_view)->freeze (file_view);
}


void
gth_file_view_thaw (GthFileView  *file_view)
{
	GTH_FILE_VIEW_GET_CLASS (file_view)->thaw (file_view);
}


gboolean
gth_file_view_is_frozen (GthFileView  *file_view)
{
	return GTH_FILE_VIEW_GET_CLASS (file_view)->is_frozen (file_view);
}


/**/


void
gth_file_view_insert (GthFileView  *file_view,
		      int           pos, 
		      GdkPixbuf    *pixbuf,
		      const char   *text,
		      const char   *comment)
{
	GTH_FILE_VIEW_GET_CLASS (file_view)->insert (file_view, pos, pixbuf, text, comment);
}


int
gth_file_view_append (GthFileView  *file_view,
		      GdkPixbuf    *pixbuf,
		      const char   *text,
		      const char   *comment)
{
	return GTH_FILE_VIEW_GET_CLASS (file_view)->append (file_view, pixbuf, text, comment);
}


int
gth_file_view_append_with_data (GthFileView  *file_view,
				GdkPixbuf    *pixbuf,
				const char   *text,
				const char   *comment,
				gpointer      data)
{
	return GTH_FILE_VIEW_GET_CLASS (file_view)->append_with_data (file_view, pixbuf, text, comment, data);
}


void
gth_file_view_remove (GthFileView  *file_view, 
		      int           pos)
{
	GTH_FILE_VIEW_GET_CLASS (file_view)->remove (file_view, pos);
}


void
gth_file_view_clear (GthFileView  *file_view)
{
	GTH_FILE_VIEW_GET_CLASS (file_view)->clear (file_view);
}


void
gth_file_view_set_image_pixbuf (GthFileView  *file_view,
				int           pos,
				GdkPixbuf    *pixbuf)
{
	GTH_FILE_VIEW_GET_CLASS (file_view)->set_image_pixbuf (file_view, pos, pixbuf);
}


void
gth_file_view_set_unknown_pixbuf (GthFileView  *file_view,
				  int           pos)
{
	GTH_FILE_VIEW_GET_CLASS (file_view)->set_unknown_pixbuf (file_view, pos);
}


void
gth_file_view_set_image_text (GthFileView  *file_view,
			      int           pos,
			      const char   *text)
{
	GTH_FILE_VIEW_GET_CLASS (file_view)->set_image_text (file_view, pos, text);
}


const char*
gth_file_view_get_image_text (GthFileView  *file_view,
			      int           pos)
{
	return 	GTH_FILE_VIEW_GET_CLASS (file_view)->get_image_text (file_view, pos);
}


void
gth_file_view_set_image_comment (GthFileView  *file_view,
				 int           pos,
				 const char   *comment)
{
	GTH_FILE_VIEW_GET_CLASS (file_view)->set_image_comment (file_view, pos, comment);
}


const char*
gth_file_view_get_image_comment (GthFileView  *file_view,
				 int           pos)
{
	return 	GTH_FILE_VIEW_GET_CLASS (file_view)->get_image_comment (file_view, pos);
}


int
gth_file_view_get_images (GthFileView *file_view)
{
	return 	GTH_FILE_VIEW_GET_CLASS (file_view)->get_images (file_view);
}


GList *
gth_file_view_get_list (GthFileView *file_view)
{
	return 	GTH_FILE_VIEW_GET_CLASS (file_view)->get_list (file_view);
}


GList *
gth_file_view_get_selection (GthFileView *file_view)
{
	return 	GTH_FILE_VIEW_GET_CLASS (file_view)->get_selection (file_view);
}


/* Managing the selection */


void
gth_file_view_select_image (GthFileView *file_view,
			    int          pos)
{
	GTH_FILE_VIEW_GET_CLASS (file_view)->select_image (file_view, pos);
}


void
gth_file_view_unselect_image (GthFileView *file_view,
			      int          pos)
{
	GTH_FILE_VIEW_GET_CLASS (file_view)->unselect_image (file_view, pos);
}


void
gth_file_view_select_all (GthFileView *file_view)
{
	GTH_FILE_VIEW_GET_CLASS (file_view)->select_all (file_view);
}


void
gth_file_view_unselect_all (GthFileView *file_view)
{
	GTH_FILE_VIEW_GET_CLASS (file_view)->unselect_all (file_view);
}


GList *
gth_file_view_get_file_list_selection (GthFileView *file_view)
{
	return GTH_FILE_VIEW_GET_CLASS (file_view)->get_file_list_selection (file_view);
}


gboolean
gth_file_view_pos_is_selected (GthFileView     *file_view,
			       int              pos)
{
	return GTH_FILE_VIEW_GET_CLASS (file_view)->pos_is_selected (file_view, pos);
}


gboolean
gth_file_view_only_one_is_selected (GthFileView    *file_view)
{
	return GTH_FILE_VIEW_GET_CLASS (file_view)->only_one_is_selected (file_view);
}


gboolean
gth_file_view_selection_not_null (GthFileView    *file_view)
{
	return GTH_FILE_VIEW_GET_CLASS (file_view)->selection_not_null (file_view);
}


int
gth_file_view_get_first_selected (GthFileView *file_view)
{
	return GTH_FILE_VIEW_GET_CLASS (file_view)->get_first_selected (file_view);
}


int
gth_file_view_get_last_selected (GthFileView *file_view)
{
	return GTH_FILE_VIEW_GET_CLASS (file_view)->get_last_selected (file_view);
}


/* Setting spacing values */


void
gth_file_view_set_image_width (GthFileView *file_view,
			       int          width)
{
	GTH_FILE_VIEW_GET_CLASS (file_view)->set_image_width (file_view, width);
}


/* Attaching information to the items */


void
gth_file_view_set_image_data (GthFileView *file_view,
			      int          pos, 
			      gpointer     data)
{
	GTH_FILE_VIEW_GET_CLASS (file_view)->set_image_data (file_view, pos, data);
}


void
gth_file_view_set_image_data_full (GthFileView     *file_view,
				   int              pos, 
				   gpointer         data,
				   GtkDestroyNotify destroy)
{
	GTH_FILE_VIEW_GET_CLASS (file_view)->set_image_data_full (file_view, pos, data, destroy);
}


int
gth_file_view_find_image_from_data (GthFileView *file_view,
				    gpointer     data)
{
	return GTH_FILE_VIEW_GET_CLASS (file_view)->find_image_from_data (file_view, data);
}


gpointer
gth_file_view_get_image_data (GthFileView *file_view,
			      int          pos)
{
	return GTH_FILE_VIEW_GET_CLASS (file_view)->get_image_data (file_view, pos);
}


/* Visibility */


void
gth_file_view_enable_thumbs (GthFileView *file_view,
			     gboolean     enable_thumbs)
{
	GTH_FILE_VIEW_GET_CLASS (file_view)->enable_thumbs (file_view, enable_thumbs);
}


void
gth_file_view_set_view_mode (GthFileView *file_view,
			     GthViewMode  mode)
{
	GTH_FILE_VIEW_GET_CLASS (file_view)->set_view_mode (file_view, mode);
}


GthViewMode
gth_file_view_get_view_mode (GthFileView *file_view)
{
	return GTH_FILE_VIEW_GET_CLASS (file_view)->get_view_mode (file_view);
}


void
gth_file_view_moveto (GthFileView *file_view,
		      int          pos, 
		      double       yalign)
{
	GTH_FILE_VIEW_GET_CLASS (file_view)->moveto (file_view, pos, yalign);
}


GthVisibility
gth_file_view_image_is_visible (GthFileView *file_view,
				int          pos)
{
	return GTH_FILE_VIEW_GET_CLASS (file_view)->image_is_visible (file_view, pos);
}


int
gth_file_view_get_image_at (GthFileView *file_view, 
			    int          x, 
			    int          y)
{
	return GTH_FILE_VIEW_GET_CLASS (file_view)->get_image_at (file_view, x, y);
}


int
gth_file_view_get_first_visible (GthFileView *file_view)
{
	return GTH_FILE_VIEW_GET_CLASS (file_view)->get_first_visible (file_view);
}


int
gth_file_view_get_last_visible (GthFileView *file_view)
{
	return GTH_FILE_VIEW_GET_CLASS (file_view)->get_last_visible (file_view);
}


/* Sort */


void
gth_file_view_sorted (GthFileView   *file_view,
		      GthSortMethod  sort_method,
		      GtkSortType    sort_type)
{
	GTH_FILE_VIEW_GET_CLASS (file_view)->sorted (file_view, sort_method, sort_type);
}


void
gth_file_view_unsorted (GthFileView *file_view)
{
	GTH_FILE_VIEW_GET_CLASS (file_view)->unsorted (file_view);
}


/* Misc */


void
gth_file_view_image_activated (GthFileView *file_view, 
			       int          pos)
{
	GTH_FILE_VIEW_GET_CLASS (file_view)->image_activated (file_view, pos);
}


void
gth_file_view_set_cursor (GthFileView *file_view, 
			  int          pos)
{
	GTH_FILE_VIEW_GET_CLASS (file_view)->set_cursor (file_view, pos);
}


int
gth_file_view_get_cursor (GthFileView *file_view)
{
	return GTH_FILE_VIEW_GET_CLASS (file_view)->get_cursor (file_view);
}


void
gth_file_view_update_icon_theme (GthFileView *file_view)
{
	GTH_FILE_VIEW_GET_CLASS (file_view)->update_icon_theme (file_view);
}


void
gth_file_view_set_no_image_text (GthFileView *file_view,
				 const char  *text)
{
	GTH_FILE_VIEW_GET_CLASS (file_view)->set_no_image_text (file_view, text);
}


/* Interactive search */


void
gth_file_view_set_enable_search (GthFileView *file_view,
				 gboolean     enable_search)
{
	GTH_FILE_VIEW_GET_CLASS (file_view)->set_enable_search (file_view, enable_search);
}


gboolean
gth_file_view_get_enable_search (GthFileView *file_view)
{
	return GTH_FILE_VIEW_GET_CLASS (file_view)->get_enable_search (file_view);
}


void
gth_file_view_selection_changed (GthFileView   *file_view)
{
	g_signal_emit (file_view, file_view_signals[SELECTION_CHANGED], 0);
}


void
gth_file_view_item_activated (GthFileView   *file_view, 
			      int            pos)
{
	g_signal_emit (file_view, file_view_signals[ITEM_ACTIVATED], 0, pos);
}


void
gth_file_view_cursor_changed (GthFileView   *file_view, 
			      int            pos)
{
	g_signal_emit (file_view, file_view_signals[CURSOR_CHANGED], 0, pos);
}
