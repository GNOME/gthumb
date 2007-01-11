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

#include <string.h>
#include <glib.h>
#include <gnome.h>
#include "gth-file-view.h"
#include "gth-file-view-thumbs.h"
#include "gthumb-marshal.h"
#include "gthumb-enum-types.h"
#include "gth-image-list.h"
#include "file-utils.h"
#include "file-data.h"
#include "icons/pixbufs.h"
#include "pixbuf-utils.h"
#include "gth-sort-utils.h"


struct _GthFileViewThumbsPrivate {
	GthImageList   *ilist;
	GnomeIconTheme *icon_theme;
	GdkPixbuf      *unknown_pixbuf;
};


static GthFileViewClass *parent_class;


static void
gfv_set_hadjustment (GthFileView   *file_view,
		     GtkAdjustment *hadj)
{
	GthFileViewThumbs *gfv_thumbs = (GthFileViewThumbs *) file_view;
	GthImageList      *ilist = gfv_thumbs->priv->ilist;

	gth_image_list_set_hadjustment (ilist, hadj);
}


static GtkAdjustment *
gfv_get_hadjustment (GthFileView   *file_view)
{
	GthFileViewThumbs *gfv_thumbs = (GthFileViewThumbs *) file_view;
	GthImageList      *ilist = gfv_thumbs->priv->ilist;

	return gth_image_list_get_hadjustment (ilist);
}


static void
gfv_set_vadjustment (GthFileView   *file_view,
		     GtkAdjustment *vadj)
{
	GthFileViewThumbs *gfv_thumbs = (GthFileViewThumbs *) file_view;
	GthImageList      *ilist = gfv_thumbs->priv->ilist;

	gth_image_list_set_vadjustment (ilist, vadj);
}


static GtkAdjustment *
gfv_get_vadjustment (GthFileView   *file_view)
{
	GthFileViewThumbs *gfv_thumbs = (GthFileViewThumbs *) file_view;
	GthImageList      *ilist = gfv_thumbs->priv->ilist;

	return gth_image_list_get_vadjustment (ilist);
}


static GtkWidget *
gfv_get_widget (GthFileView   *file_view)
{
	GthFileViewThumbs *gfv_thumbs = (GthFileViewThumbs *) file_view;

	return GTK_WIDGET (gfv_thumbs->priv->ilist);
}


/* To avoid excesive recomputes during insertion/deletion */


static void
gfv_freeze (GthFileView  *file_view)
{
	GthFileViewThumbs *gfv_thumbs = (GthFileViewThumbs *) file_view;
	GthImageList      *ilist = gfv_thumbs->priv->ilist;

	gth_image_list_freeze (ilist);
}


static void
gfv_thaw (GthFileView  *file_view)
{
	GthFileViewThumbs *gfv_thumbs = (GthFileViewThumbs *) file_view;
	GthImageList      *ilist = gfv_thumbs->priv->ilist;

	gth_image_list_thaw (ilist);
}


static gboolean
gfv_is_frozen (GthFileView  *file_view)
{
	GthFileViewThumbs *gfv_thumbs = (GthFileViewThumbs *) file_view;
	GthImageList      *ilist = gfv_thumbs->priv->ilist;

	return gth_image_list_is_frozen (ilist);
}


/**/


static void
gfv_insert (GthFileView  *file_view,
	    int           pos,
	    GdkPixbuf    *pixbuf,
	    const char   *text,
	    const char   *comment)
{
	GthFileViewThumbs *gfv_thumbs = (GthFileViewThumbs *) file_view;
	GthImageList      *ilist = gfv_thumbs->priv->ilist;
	GdkPixbuf         *real_pixbuf;

	if (pixbuf == NULL)
		real_pixbuf = gfv_thumbs->priv->unknown_pixbuf;
	else
		real_pixbuf = pixbuf;

	gth_image_list_insert (ilist, pos, real_pixbuf, text, comment);
}


static int
gfv_append (GthFileView  *file_view,
	    GdkPixbuf    *pixbuf,
	    const char   *text,
	    const char   *comment)
{
	GthFileViewThumbs *gfv_thumbs = (GthFileViewThumbs *) file_view;
	GthImageList      *ilist = gfv_thumbs->priv->ilist;
	GdkPixbuf         *real_pixbuf;

	if (pixbuf == NULL)
		real_pixbuf = gfv_thumbs->priv->unknown_pixbuf;
	else
		real_pixbuf = pixbuf;

	return 	gth_image_list_append (ilist, real_pixbuf, text, comment);
}


static int
gfv_append_with_data (GthFileView  *file_view,
		      GdkPixbuf    *pixbuf,
		      const char   *text,
		      const char   *comment,
		      gpointer      data)
{
	GthFileViewThumbs *gfv_thumbs = (GthFileViewThumbs *) file_view;
	GthImageList      *ilist = gfv_thumbs->priv->ilist;
	GdkPixbuf         *real_pixbuf;

	if (pixbuf == NULL)
		real_pixbuf = gfv_thumbs->priv->unknown_pixbuf;
	else
		real_pixbuf = pixbuf;

	return 	gth_image_list_append_with_data (ilist, real_pixbuf, text, comment, data);
}


static void
gfv_remove (GthFileView  *file_view,
	    gpointer      data)
{
	GthFileViewThumbs *gfv_thumbs = (GthFileViewThumbs *) file_view;
	GthImageList      *ilist = gfv_thumbs->priv->ilist;

	gth_image_list_remove (ilist, data);
}


static void
gfv_clear (GthFileView  *file_view)
{
	GthFileViewThumbs *gfv_thumbs = (GthFileViewThumbs *) file_view;
	GthImageList      *ilist = gfv_thumbs->priv->ilist;

	gth_image_list_clear (ilist);
}


static void
gfv_set_image_pixbuf (GthFileView  *file_view,
		      int           pos,
		      GdkPixbuf    *pixbuf)
{
	GthFileViewThumbs *gfv_thumbs = (GthFileViewThumbs *) file_view;
	GthImageList      *ilist = gfv_thumbs->priv->ilist;

	gth_image_list_set_image_pixbuf (ilist, pos, pixbuf);
}


static void
gfv_set_unknown_pixbuf (GthFileView  *file_view,
			int           pos)
{
	GthFileViewThumbs *gfv_thumbs = (GthFileViewThumbs *) file_view;
	GthImageList      *ilist = gfv_thumbs->priv->ilist;

	gth_image_list_set_image_pixbuf (ilist, pos, gfv_thumbs->priv->unknown_pixbuf);
}


static void
gfv_set_image_text (GthFileView  *file_view,
		    int           pos,
		    const char   *text)
{
	GthFileViewThumbs *gfv_thumbs = (GthFileViewThumbs *) file_view;
	GthImageList      *ilist = gfv_thumbs->priv->ilist;

	gth_image_list_set_image_text (ilist, pos, text);
}


static const char*
gfv_get_image_text (GthFileView  *file_view,
		    int           pos)
{
	GthFileViewThumbs *gfv_thumbs = (GthFileViewThumbs *) file_view;
	GthImageList      *ilist = gfv_thumbs->priv->ilist;

	return gth_image_list_get_image_text (ilist, pos);
}


static void
gfv_set_image_comment (GthFileView  *file_view,
		       int           pos,
		       const char   *comment)
{
	GthFileViewThumbs *gfv_thumbs = (GthFileViewThumbs *) file_view;
	GthImageList      *ilist = gfv_thumbs->priv->ilist;

	gth_image_list_set_image_comment (ilist, pos, comment);
}


static const char*
gfv_get_image_comment (GthFileView  *file_view,
		       int           pos)
{
	GthFileViewThumbs *gfv_thumbs = (GthFileViewThumbs *) file_view;
	GthImageList      *ilist = gfv_thumbs->priv->ilist;

	return gth_image_list_get_image_comment (ilist, pos);
}


static int
gfv_get_images (GthFileView  *file_view)
{
	GthFileViewThumbs *gfv_thumbs = (GthFileViewThumbs *) file_view;
	GthImageList      *ilist = gfv_thumbs->priv->ilist;

	return gth_image_list_get_images (ilist);
}


static GList *
gfv_get_list (GthFileView  *file_view)
{
	GthFileViewThumbs *gfv_thumbs = (GthFileViewThumbs *) file_view;
	GthImageList      *ilist = gfv_thumbs->priv->ilist;

	return gth_image_list_get_list (ilist);
}


static GList *
gfv_get_selection (GthFileView  *file_view)
{
	GthFileViewThumbs *gfv_thumbs = (GthFileViewThumbs *) file_view;
	GthImageList      *ilist = gfv_thumbs->priv->ilist;

	return gth_image_list_get_selection (ilist);
}


/* Managing the selection */


static void
gfv_select_image (GthFileView     *file_view,
		  int              pos)
{
	GthFileViewThumbs *gfv_thumbs = (GthFileViewThumbs *) file_view;
	GthImageList      *ilist = gfv_thumbs->priv->ilist;

	gth_image_list_select_image (ilist, pos);
}


static void
gfv_unselect_image (GthFileView     *file_view,
		    int              pos)
{
	GthFileViewThumbs *gfv_thumbs = (GthFileViewThumbs *) file_view;
	GthImageList      *ilist = gfv_thumbs->priv->ilist;

	gth_image_list_unselect_image (ilist, pos);
}


static void
gfv_select_all (GthFileView     *file_view)
{
	GthFileViewThumbs *gfv_thumbs = (GthFileViewThumbs *) file_view;
	GthImageList      *ilist = gfv_thumbs->priv->ilist;

	gth_image_list_select_all (ilist);
}


static void
gfv_unselect_all (GthFileView     *file_view)
{
	GthFileViewThumbs *gfv_thumbs = (GthFileViewThumbs *) file_view;
	GthImageList      *ilist = gfv_thumbs->priv->ilist;

	gth_image_list_unselect_all (ilist);
}


static GList *
gfv_get_file_list_selection (GthFileView *file_view)
{
	GthFileViewThumbs *gfv_thumbs = (GthFileViewThumbs *) file_view;
	GthImageList      *ilist = gfv_thumbs->priv->ilist;
	GList             *scan, *list, *file_list = NULL;

	list = gth_image_list_get_selection (ilist);
	for (scan = list; scan; scan = scan->next) {
		FileData *fd = scan->data;

		if ((fd != NULL) && (fd->path != NULL))
			file_list = g_list_prepend (file_list,
						    g_strdup (fd->path));
	}
	file_data_list_free (list);

	return g_list_reverse (file_list);
}


static gboolean
gfv_pos_is_selected (GthFileView     *file_view,
		     int              pos)
{
	GthFileViewThumbs *gfv_thumbs = (GthFileViewThumbs *) file_view;
	GthImageList      *ilist = gfv_thumbs->priv->ilist;

	return gth_image_list_pos_is_selected (ilist, pos);
}


static gboolean
gfv_only_one_is_selected (GthFileView    *file_view)
{
	GthFileViewThumbs *gfv_thumbs = (GthFileViewThumbs *) file_view;
	GthImageList      *ilist = gfv_thumbs->priv->ilist;
        GList             *sel_list = gth_image_list_get_selection (ilist);
	gboolean           ret_val = FALSE;

	ret_val = (sel_list != NULL) && (sel_list->next == NULL);
	file_data_list_free (sel_list);

	return ret_val;
}


static gboolean
gfv_selection_not_null (GthFileView    *file_view)
{
	GthFileViewThumbs *gfv_thumbs = (GthFileViewThumbs *) file_view;
	GthImageList      *ilist = gfv_thumbs->priv->ilist;
        GList             *sel_list = gth_image_list_get_selection (ilist);
	gboolean           ret_val;

	ret_val = (sel_list != NULL);
	file_data_list_free (sel_list);

	return ret_val;
}


static int
gfv_get_first_selected (GthFileView *file_view)
{
	GthFileViewThumbs *gfv_thumbs = (GthFileViewThumbs *) file_view;
	GthImageList      *ilist = gfv_thumbs->priv->ilist;

	return gth_image_list_get_first_selected (ilist);
}


static int
gfv_get_last_selected (GthFileView *file_view)
{
	GthFileViewThumbs *gfv_thumbs = (GthFileViewThumbs *) file_view;
	GthImageList      *ilist = gfv_thumbs->priv->ilist;

	return gth_image_list_get_last_selected (ilist);
}


/* Setting spacing values */


static void
gfv_set_image_width (GthFileView     *file_view,
		     int              width)
{
	GthFileViewThumbs *gfv_thumbs = (GthFileViewThumbs *) file_view;
	GthImageList      *ilist = gfv_thumbs->priv->ilist;

	gth_image_list_set_image_width (ilist, width);
}


/* Attaching information to the items */


static void
gfv_set_image_data (GthFileView     *file_view,
		    int              pos,
		    gpointer         data)
{
	GthFileViewThumbs *gfv_thumbs = (GthFileViewThumbs *) file_view;
	GthImageList      *ilist = gfv_thumbs->priv->ilist;

	gth_image_list_set_image_data (ilist, pos, data);
}


static void
gfv_set_image_data_full (GthFileView     *file_view,
			 int              pos,
			 gpointer         data,
			 GtkDestroyNotify destroy)
{
	GthFileViewThumbs *gfv_thumbs = (GthFileViewThumbs *) file_view;
	GthImageList      *ilist = gfv_thumbs->priv->ilist;

	gth_image_list_set_image_data_full (ilist, pos, data, destroy);
}


static int
gfv_find_image_from_data (GthFileView     *file_view,
			  gpointer         data)
{
	GthFileViewThumbs *gfv_thumbs = (GthFileViewThumbs *) file_view;
	GthImageList      *ilist = gfv_thumbs->priv->ilist;

	return gth_image_list_find_image_from_data (ilist, data);
}


static gpointer
gfv_get_image_data (GthFileView     *file_view,
		    int              pos)
{
	GthFileViewThumbs *gfv_thumbs = (GthFileViewThumbs *) file_view;
	GthImageList      *ilist = gfv_thumbs->priv->ilist;

	return gth_image_list_get_image_data (ilist, pos);
}


/* Visibility */


static void
gfv_enable_thumbs (GthFileView *file_view,
		   gboolean     enable_thumbs)
{
	GthFileViewThumbs *gfv_thumbs = (GthFileViewThumbs *) file_view;
	GthImageList      *ilist = gfv_thumbs->priv->ilist;

	gth_image_list_enable_thumbs (ilist, enable_thumbs);
}


static void
gfv_set_view_mode (GthFileView *file_view,
		   GthViewMode  mode)
{
	GthFileViewThumbs *gfv_thumbs = (GthFileViewThumbs *) file_view;
	GthImageList      *ilist = gfv_thumbs->priv->ilist;

	gth_image_list_set_view_mode (ilist, mode);
}


static GthViewMode
gfv_get_view_mode (GthFileView *file_view)
{
	GthFileViewThumbs *gfv_thumbs = (GthFileViewThumbs *) file_view;
	GthImageList      *ilist = gfv_thumbs->priv->ilist;

	return gth_image_list_get_view_mode (ilist);
}


static void
gfv_moveto (GthFileView *file_view,
	    int          pos,
	    double       yalign)
{
	GthFileViewThumbs *gfv_thumbs = (GthFileViewThumbs *) file_view;
	GthImageList      *ilist = gfv_thumbs->priv->ilist;

	gth_image_list_moveto (ilist, pos, yalign);
}


static GthVisibility
gfv_image_is_visible (GthFileView *file_view,
		      int          pos)
{
	GthFileViewThumbs *gfv_thumbs = (GthFileViewThumbs *) file_view;
	GthImageList      *ilist = gfv_thumbs->priv->ilist;

	return gth_image_list_image_is_visible (ilist, pos);
}


static int
gfv_get_image_at (GthFileView *file_view,
		  int          x,
		  int          y)
{
	GthFileViewThumbs *gfv_thumbs = (GthFileViewThumbs *) file_view;
	GthImageList      *ilist = gfv_thumbs->priv->ilist;

	return gth_image_list_get_image_at (ilist, x, y);
}


static int
gfv_get_first_visible (GthFileView *file_view)
{
	GthFileViewThumbs *gfv_thumbs = (GthFileViewThumbs *) file_view;
	GthImageList      *ilist = gfv_thumbs->priv->ilist;

	return gth_image_list_get_first_visible (ilist);
}


static int
gfv_get_last_visible (GthFileView *file_view)
{
	GthFileViewThumbs *gfv_thumbs = (GthFileViewThumbs *) file_view;
	GthImageList      *ilist = gfv_thumbs->priv->ilist;

	return gth_image_list_get_last_visible (ilist);
}


static void
gfv_set_visible_func (GthFileView    *file_view,
                      GthVisibleFunc  func,
                      gpointer        data)
{
	GthFileViewThumbs *gfv_thumbs = (GthFileViewThumbs *) file_view;
	GthImageList      *ilist = gfv_thumbs->priv->ilist;

	gth_image_list_set_visible_func (ilist, func, data);
}


/* Sort */


static int
comp_func_name (gconstpointer  ptr1,
		gconstpointer  ptr2)
{
        const GthImageListItem *item1 = ptr1, *item2 = ptr2;
	const FileData         *fd1, *fd2;

	fd1 = item1->data;
	fd2 = item2->data;

	if ((fd1 == NULL) || (fd2 == NULL))
		return 0;

	return gth_sort_by_filename_but_ignore_path (fd1->name, fd2->name);
}


static int
comp_func_size (gconstpointer  ptr1,
		gconstpointer  ptr2)
{
        const GthImageListItem *item1 = ptr1, *item2 = ptr2;
	const FileData         *fd1, *fd2;

	fd1 = item1->data;
	fd2 = item2->data;

	if ((fd1 == NULL) || (fd2 == NULL))
		return 0;

	return gth_sort_by_size_then_name (fd1->size, fd2->size, fd1->path, fd2->path);
}


static int
comp_func_time (gconstpointer  ptr1,
		gconstpointer  ptr2)
{
        const GthImageListItem *item1 = ptr1, *item2 = ptr2;
	const FileData         *fd1, *fd2;

	fd1 = item1->data;
	fd2 = item2->data;

	if ((fd1 == NULL) || (fd2 == NULL))
		return 0;

	return gth_sort_by_filetime_then_name (fd1->mtime, fd2->mtime,
						 fd1->path, fd2->path);
}


static int
comp_func_exif_date (gconstpointer  ptr1,
		     gconstpointer  ptr2)
{
	const GthImageListItem *item1 = ptr1, *item2 = ptr2;
	FileData               *fd1, *fd2;

	fd1 = item1->data;
	fd2 = item2->data;

	if ((fd1 == NULL) || (fd2 == NULL))
		return 0;

	return gth_sort_by_exiftime_then_name (fd1, fd2);
}


static int
comp_func_path (gconstpointer  ptr1,
		gconstpointer  ptr2)
{
        const GthImageListItem *item1 = ptr1, *item2 = ptr2;
	const FileData         *fd1, *fd2;

	fd1 = item1->data;
	fd2 = item2->data;

	if ((fd1 == NULL) || (fd2 == NULL))
		return 0;

	return gth_sort_by_full_path (fd1->path, fd2->path);
}


static int
comp_func_comment (gconstpointer  ptr1, gconstpointer  ptr2)
{
	const GthImageListItem *item1 = ptr1, *item2 = ptr2;
	const FileData         *fd1, *fd2;

	fd1 = item1->data;
	fd2 = item2->data;

	return gth_sort_by_comment_then_name (item1->comment, item2->comment,
					fd1->path, fd2->path);
}


static GCompareFunc
get_compfunc_from_method (GthSortMethod sort_method)
{
	GCompareFunc func;

	switch (sort_method) {
	case GTH_SORT_METHOD_BY_NAME:
		func = comp_func_name;
		break;
	case GTH_SORT_METHOD_BY_TIME:
		func = comp_func_time;
		break;
	case GTH_SORT_METHOD_BY_SIZE:
		func = comp_func_size;
		break;
	case GTH_SORT_METHOD_BY_PATH:
		func = comp_func_path;
		break;
	case GTH_SORT_METHOD_BY_COMMENT:
		func = comp_func_comment;
		break;
	case GTH_SORT_METHOD_BY_EXIF_DATE:
		func = comp_func_exif_date;
		break;
	case GTH_SORT_METHOD_NONE:
	case GTH_SORT_METHOD_MANUAL:
	default:
		func = gth_sort_none;
		break;
	}

	return func;
}


static void
gfv_sorted (GthFileView   *file_view,
	    GthSortMethod  sort_method,
	    GtkSortType    sort_type)
{
	GthFileViewThumbs *gfv_thumbs = (GthFileViewThumbs *) file_view;
	GthImageList      *ilist = gfv_thumbs->priv->ilist;

	gth_image_list_sorted (GTH_IMAGE_LIST (ilist),
			       get_compfunc_from_method (sort_method),
			       sort_type);
}


static void
gfv_unsorted (GthFileView *file_view)
{
	GthFileViewThumbs *gfv_thumbs = (GthFileViewThumbs *) file_view;
	GthImageList      *ilist = gfv_thumbs->priv->ilist;

	gth_image_list_unsorted (ilist);
}


/* Misc */


static void
gfv_image_activated (GthFileView *file_view,
		     int          pos)
{
	GthFileViewThumbs *gfv_thumbs = (GthFileViewThumbs *) file_view;
	GthImageList      *ilist = gfv_thumbs->priv->ilist;

	gth_image_list_image_activated (ilist, pos);
}


static void
gfv_set_cursor (GthFileView *file_view,
		int          pos)
{
	GthFileViewThumbs *gfv_thumbs = (GthFileViewThumbs *) file_view;
	GthImageList      *ilist = gfv_thumbs->priv->ilist;

	gth_image_list_set_cursor (ilist, pos);
}


static int
gfv_get_cursor (GthFileView *file_view)
{
	GthFileViewThumbs *gfv_thumbs = (GthFileViewThumbs *) file_view;
	GthImageList      *ilist = gfv_thumbs->priv->ilist;

	return 	gth_image_list_get_cursor (ilist);
}


static int
get_default_icon_size (GtkWidget *widget)
{
	int icon_width, icon_height;

	gtk_icon_size_lookup_for_settings (gtk_widget_get_settings (widget),
                                           GTK_ICON_SIZE_DIALOG,
                                           &icon_width, &icon_height);
	return MAX (icon_width, icon_height);
}


static GdkPixbuf *
create_unknown_pixbuf (GthFileViewThumbs *gfv_thumbs)
{
	int         icon_size;
	char       *icon_name;
	char       *icon_path;
	GdkPixbuf  *pixbuf = NULL;
	int         width, height;

	icon_size = get_default_icon_size (GTK_WIDGET (gfv_thumbs->priv->ilist));

	icon_name = gnome_icon_lookup (gfv_thumbs->priv->icon_theme,
				       NULL,
				       NULL,
				       NULL,
				       NULL,
				       "image/*",
				       GNOME_ICON_LOOKUP_FLAGS_NONE,
				       NULL);
	icon_path = gnome_icon_theme_lookup_icon (gfv_thumbs->priv->icon_theme,
						  icon_name,
						  icon_size,
						  NULL,
						  NULL);
	g_free (icon_name);

	if (icon_path != NULL) {
		pixbuf = gdk_pixbuf_new_from_file (icon_path, NULL);
		g_free (icon_path);
	}

	if (pixbuf == NULL)
		pixbuf = gdk_pixbuf_new_from_inline (-1,
						     dir_16_rgba,
						     FALSE,
						     NULL);

	width = gdk_pixbuf_get_width (pixbuf);
	height = gdk_pixbuf_get_height (pixbuf);
	if (scale_keepping_ratio (&width, &height, icon_size, icon_size)) {
		GdkPixbuf *scaled;
		scaled = gdk_pixbuf_scale_simple (pixbuf,
						  width,
						  height,
						  GDK_INTERP_BILINEAR);
		g_object_unref (pixbuf);
		pixbuf = scaled;
	}

	return pixbuf;
}


static void
gth_file_view_thumbs_init (GthFileViewThumbs *gfv_thumbs)
{
	GthFileViewThumbsPrivate *priv;

	priv = g_new0 (GthFileViewThumbsPrivate, 1);
	gfv_thumbs->priv = priv;
}


static void
gfv_update_icon_theme (GthFileView *file_view)
{
	GthFileViewThumbs *gfv_thumbs = (GthFileViewThumbs *) file_view;

	if (gfv_thumbs->priv->unknown_pixbuf != NULL)
		g_object_unref (gfv_thumbs->priv->unknown_pixbuf);

	gfv_thumbs->priv->unknown_pixbuf = create_unknown_pixbuf (gfv_thumbs);
}


static void
gfv_set_no_image_text (GthFileView *file_view,
		       const char  *text)
{
	GthFileViewThumbs *gfv_thumbs = (GthFileViewThumbs *) file_view;
	GthImageList      *ilist = gfv_thumbs->priv->ilist;

	gth_image_list_set_no_image_text (ilist, text);
}


/* DnD */


static void
gfv_set_drag_dest_pos (GthFileView *file_view,
		       int          x,
		       int          y)
{
	GthFileViewThumbs *gfv_thumbs = (GthFileViewThumbs *) file_view;
	GthImageList      *ilist = gfv_thumbs->priv->ilist;

	gth_image_list_set_drag_dest_pos (ilist, x, y);
}


static void
gfv_get_drag_dest_pos (GthFileView     *file_view,
		       int             *pos)
{
	GthFileViewThumbs *gfv_thumbs = (GthFileViewThumbs *) file_view;
	GthImageList      *ilist = gfv_thumbs->priv->ilist;

	gth_image_list_get_drag_dest_pos (ilist, pos);
}


static void
gfv_set_reorderable (GthFileView  *file_view,
		     gboolean      value)
{
	GthFileViewThumbs *gfv_thumbs = (GthFileViewThumbs *) file_view;
	GthImageList      *ilist = gfv_thumbs->priv->ilist;

	gth_image_list_set_reorderable (ilist, value);
}


static gboolean
gfv_get_reorderable (GthFileView  *file_view)
{
	GthFileViewThumbs *gfv_thumbs = (GthFileViewThumbs *) file_view;
	GthImageList      *ilist = gfv_thumbs->priv->ilist;

	return gth_image_list_get_reorderable (ilist);
}


/* Interactive search */


static void
gfv_set_enable_search (GthFileView *file_view,
		       gboolean     enable_search)
{
	GthFileViewThumbs *gfv_thumbs = (GthFileViewThumbs *) file_view;
	GthImageList      *ilist = gfv_thumbs->priv->ilist;

	gth_image_list_set_enable_search (ilist, enable_search);
}


static gboolean
gfv_get_enable_search (GthFileView *file_view)
{
	GthFileViewThumbs *gfv_thumbs = (GthFileViewThumbs *) file_view;
	GthImageList      *ilist = gfv_thumbs->priv->ilist;

	return gth_image_list_get_enable_search (ilist);
}


/**/


static void
gth_file_view_thumbs_finalize (GObject *object)
{
  	GthFileViewThumbs *gfv_thumbs;

	g_return_if_fail (GTH_IS_FILE_VIEW_THUMBS (object));

	gfv_thumbs = (GthFileViewThumbs*) object;

	g_object_unref (gfv_thumbs->priv->icon_theme);
	g_object_unref (gfv_thumbs->priv->unknown_pixbuf);
	g_free (gfv_thumbs->priv);

        /* Chain up */
	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gth_file_view_thumbs_class_init (GthFileViewThumbsClass *file_view_thumbs_class)
{
	GObjectClass     *gobject_class;
	GthFileViewClass *file_view_class;

	parent_class = g_type_class_peek_parent (file_view_thumbs_class);
	gobject_class = (GObjectClass*) file_view_thumbs_class;
	file_view_class = (GthFileViewClass*) file_view_thumbs_class;

	/* Methods */

	gobject_class->finalize = gth_file_view_thumbs_finalize;

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
	file_view_class->set_unknown_pixbuf     = gfv_set_unknown_pixbuf;
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
	file_view_class->set_visible_func     = gfv_set_visible_func;
	file_view_class->sorted               = gfv_sorted;
	file_view_class->unsorted             = gfv_unsorted;
	file_view_class->image_activated      = gfv_image_activated;
	file_view_class->set_cursor           = gfv_set_cursor;
	file_view_class->get_cursor           = gfv_get_cursor;
	file_view_class->update_icon_theme    = gfv_update_icon_theme;
	file_view_class->set_no_image_text    = gfv_set_no_image_text;
	file_view_class->set_drag_dest_pos    = gfv_set_drag_dest_pos;
	file_view_class->get_drag_dest_pos    = gfv_get_drag_dest_pos;
	file_view_class->set_reorderable      = gfv_set_reorderable;
	file_view_class->get_reorderable      = gfv_get_reorderable;
	file_view_class->set_enable_search    = gfv_set_enable_search;
	file_view_class->get_enable_search    = gfv_get_enable_search;
}


GType
gth_file_view_thumbs_get_type (void)
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (GthFileViewThumbsClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_file_view_thumbs_class_init,
			NULL,
			NULL,
			sizeof (GthFileViewThumbs),
			0,
			(GInstanceInitFunc) gth_file_view_thumbs_init
                };

		type = g_type_register_static (GTH_TYPE_FILE_VIEW,
					       "GthFileViewThumbs",
					       &type_info,
					       0);
        }

	return type;
}


static void
selection_changed_cb (GthImageList      *image_list,
		      GthFileViewThumbs *gfv_thumbs)
{
	gth_file_view_selection_changed (GTH_FILE_VIEW (gfv_thumbs));
}


static void
item_activated_cb (GthImageList      *image_list,
		   int                pos,
		   GthFileViewThumbs *gfv_thumbs)
{
	gth_file_view_item_activated (GTH_FILE_VIEW (gfv_thumbs), pos);
}


static void
cursor_changed_cb (GthImageList      *image_list,
		   int                pos,
		   GthFileViewThumbs *gfv_thumbs)
{
	gth_file_view_cursor_changed (GTH_FILE_VIEW (gfv_thumbs), pos);
}


GthFileView *
gth_file_view_thumbs_new (guint image_width)
{
	GthFileViewThumbs *gfv_thumbs;

	gfv_thumbs = GTH_FILE_VIEW_THUMBS (g_object_new (GTH_TYPE_FILE_VIEW_THUMBS, NULL));

	gfv_thumbs->priv->ilist = (GthImageList *) gth_image_list_new (image_width);
	g_signal_connect (G_OBJECT (gfv_thumbs->priv->ilist),
			  "selection_changed",
			  G_CALLBACK (selection_changed_cb),
			  gfv_thumbs);
	g_signal_connect (G_OBJECT (gfv_thumbs->priv->ilist),
			  "item_activated",
			  G_CALLBACK (item_activated_cb),
			  gfv_thumbs);
	g_signal_connect (G_OBJECT (gfv_thumbs->priv->ilist),
			  "cursor_changed",
			  G_CALLBACK (cursor_changed_cb),
			  gfv_thumbs);

	/**/

	gfv_thumbs->priv->icon_theme = gnome_icon_theme_new ();
	gnome_icon_theme_set_allow_svg (gfv_thumbs->priv->icon_theme, TRUE);
	gfv_thumbs->priv->unknown_pixbuf = create_unknown_pixbuf (gfv_thumbs);

	return GTH_FILE_VIEW (gfv_thumbs);
}
