/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001 The Free Software Foundation, Inc.
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

#include <config.h>
#include <gtk/gtk.h>
#include <string.h>
#include <libgnome/libgnome.h>
#include <libgnomevfs/gnome-vfs-types.h>
#include <libgnomevfs/gnome-vfs-ops.h>
#include <libgnomevfs/gnome-vfs-file-info.h>
#include <libgnomevfs/gnome-vfs-utils.h>

#include "gconf-utils.h"
#include "gthumb-init.h"
#include "file-data.h"
#include "file-list.h"
#include "file-utils.h"
#include "image-list.h"
#include "thumb-loader.h"
#include "typedefs.h"
#include "icons/pixbufs.h"

#define THUMB_BORDER 13
#define ROW_SPACING  14
#define COL_SPACING  14

static GdkPixbuf *unknown_pixbuf = NULL;


static gint
comp_func_name (gconstpointer  ptr1,
		gconstpointer  ptr2)
{
        const Image    *pos1 = ptr1, *pos2 = ptr2;
	const FileData *fd1, *fd2;

	fd1 = pos1->data;
	fd2 = pos2->data;

	if ((fd1 == NULL) || (fd2 == NULL))
		return 0;

	return strcasecmp (fd1->name, fd2->name);
}


static gint
comp_func_size (gconstpointer  ptr1,
		gconstpointer  ptr2)
{
        const Image    *pos1 = ptr1, *pos2 = ptr2;
	const FileData *fd1, *fd2;

	fd1 = pos1->data;
	fd2 = pos2->data;

	if ((fd1 == NULL) || (fd2 == NULL))
		return 0;

	if (fd1->size < fd2->size) return -1;
	if (fd1->size > fd2->size) return 1;

	/* if the size is the same order by name. */
	return comp_func_name (ptr1, ptr2);
}


static gint
comp_func_time (gconstpointer  ptr1,
		gconstpointer  ptr2)
{
        const Image    *pos1 = ptr1, *pos2 = ptr2;
	const FileData *fd1, *fd2;

	fd1 = pos1->data;
	fd2 = pos2->data;

	if ((fd1 == NULL) || (fd2 == NULL))
		return 0;

	if (fd1->mtime < fd2->mtime) return -1;
	if (fd1->mtime > fd2->mtime) return 1;

	/* if time is the same order by name. */
	return comp_func_name (ptr1, ptr2);
}


static gint
comp_func_path (gconstpointer  ptr1,
		gconstpointer  ptr2)
{
        const Image    *pos1 = ptr1, *pos2 = ptr2;
	const FileData *fd1, *fd2;

	fd1 = pos1->data;
	fd2 = pos2->data;

	if ((fd1 == NULL) || (fd2 == NULL))
		return 0;

	return strcmp (fd1->path, fd2->path);
}


static gint
comp_func_none (gconstpointer  ptr1,
		gconstpointer  ptr2)
{
	return 0;
}


static void
set_unknown_icon (FileList *file_list, gint pos)
{
	image_list_set_image_pixbuf (IMAGE_LIST (file_list->ilist), 
				     pos, unknown_pixbuf);
}


static void 
update_thumb_in_clist (FileList *file_list)
{
	GdkPixbuf *pixbuf;

	pixbuf = thumb_loader_get_pixbuf (file_list->thumb_loader);

	if (pixbuf == NULL)
		return;

	image_list_set_image_pixbuf (IMAGE_LIST (file_list->ilist), 
				     file_list->thumb_pos, 
				     pixbuf);
}


static void file_list_update_next_thumb (FileList *file_list);
static void interrupt_thumbs__part2 (FileList *file_list);


static void 
load_thumb_error_cb (ThumbLoader *tl,
		     gpointer data)
{
	FileList *file_list = data;

	set_unknown_icon (file_list, file_list->thumb_pos);
	file_list->thumb_fd->error = TRUE;
	file_list->thumb_fd->thumb = FALSE;

	file_list_update_next_thumb (file_list);
}


static void 
load_thumb_done_cb (ThumbLoader *tl,
		    gpointer data)
{
	FileList *file_list = data;
	gfloat    percent;

	update_thumb_in_clist (file_list);
	file_list->thumb_fd->error = FALSE;
	file_list->thumb_fd->thumb = TRUE;

	percent = ((float) (file_list->thumbs_num) 
		   / IMAGE_LIST (file_list->ilist)->images);
	if (file_list->progress_func) 
		file_list->progress_func (percent, file_list->progress_data);

	file_list_update_next_thumb (file_list);
}


static GCompareFunc
get_compfunc_from_method (SortMethod sort_method)
{
	GCompareFunc func;

	switch (sort_method) {
	case SORT_BY_NAME:
		func = comp_func_name;
		break;
	case SORT_BY_TIME:
		func = comp_func_time;
		break;
	case SORT_BY_SIZE:
		func = comp_func_size;
		break;
	case SORT_BY_PATH:
		func = comp_func_path;
		break;
	case SORT_NONE:
		func = comp_func_none;
		break;
	default:
		func = comp_func_none;
	}

	return func;
}


FileList*   
file_list_new ()
{
	FileList *file_list;
	GtkWidget *ilist;
	GtkAdjustment *adj;
        GtkWidget *vsb, *hbox;
	GtkWidget *frame; 
	ImageListViewMode view_mode;

	file_list = g_new (FileList, 1);
	
	/* set the default values. */
	file_list->list              = NULL;
	file_list->sort_method       = pref_get_arrange_type ();
	file_list->sort_type         = pref_get_sort_order ();
	file_list->show_dot_files    = eel_gconf_get_boolean (PREF_SHOW_HIDDEN_FILES);
	file_list->enable_thumbs     = eel_gconf_get_boolean (PREF_SHOW_THUMBNAILS);
	file_list->thumb_size        = eel_gconf_get_integer (PREF_THUMBNAIL_SIZE);
	file_list->doing_thumbs      = FALSE;
	file_list->thumb_loader      = THUMB_LOADER (thumb_loader_new (NULL, file_list->thumb_size, file_list->thumb_size));
	file_list->thumbs_num        = 0;
	file_list->thumb_fd          = NULL;
	file_list->thumb_pos         = -1;
	file_list->progress_func     = NULL;
	file_list->progress_data     = NULL;
	file_list->interrupt_done_func = NULL;
	file_list->interrupt_done_data = NULL;

	if (unknown_pixbuf == NULL)
		unknown_pixbuf = gdk_pixbuf_new_from_inline (-1, unknown_48_rgba, FALSE, NULL);
	else
		g_object_ref (G_OBJECT (unknown_pixbuf));

	g_signal_connect (G_OBJECT (file_list->thumb_loader), 
			  "done",
			  G_CALLBACK (load_thumb_done_cb), 
			  file_list);
	g_signal_connect (G_OBJECT (file_list->thumb_loader), 
			  "error",
			  G_CALLBACK (load_thumb_error_cb), 
			  file_list);
	
	/* Create the widgets. */

	/* vertical scrollbar. */
	adj = GTK_ADJUSTMENT (gtk_adjustment_new (0, 0, 1, 1, 1, 1));
        vsb = gtk_vscrollbar_new (adj);

	/* image list. */
	ilist = image_list_new (eel_gconf_get_integer (PREF_THUMBNAIL_SIZE) + THUMB_BORDER,
				adj, 
				0);
        image_list_set_selection_mode (IMAGE_LIST (ilist), 
                                       GTK_SELECTION_MULTIPLE);
        image_list_set_row_spacing (IMAGE_LIST (ilist), ROW_SPACING);
        image_list_set_col_spacing (IMAGE_LIST (ilist), COL_SPACING);
	image_list_set_compare_func (IMAGE_LIST (ilist),
				    get_compfunc_from_method (file_list->sort_method));
	image_list_set_sort_type (IMAGE_LIST (ilist), file_list->sort_type);
	if (eel_gconf_get_boolean (PREF_SHOW_COMMENTS))
		view_mode = IMAGE_LIST_VIEW_ALL;
	else
		view_mode = IMAGE_LIST_VIEW_TEXT;
	image_list_set_view_mode (IMAGE_LIST (ilist), view_mode);

	/**/

	frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
	gtk_container_add (GTK_CONTAINER (frame), ilist);

	/**/
	hbox = gtk_hbox_new (FALSE, 3);
	gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), vsb, FALSE, FALSE, 0);

	file_list->ilist = ilist;
	file_list->root_widget = hbox;

	return file_list;
}


static void
free_file_data_list (GList *list)
{
	GList *scan;
	for (scan = list; scan; scan = scan->next) 
		file_data_unref (scan->data);
	g_list_free (list);
}


static void
file_list_free_list (FileList *file_list)
{
	g_return_if_fail (file_list != NULL);

	free_file_data_list (file_list->list);
	file_list->list = NULL;
}


void
file_list_free (FileList *file_list)
{
	file_list_free_list (file_list);
	g_object_unref (G_OBJECT (file_list->thumb_loader));
	g_free (file_list);
	g_object_unref (G_OBJECT (unknown_pixbuf));
}


/* used by file_list_update_list. */
static void file_list_update_thumbs (FileList *file_list);


/* -- file_list_update_list -- */


void
update_list__step2 (FileList *file_list)
{
	GtkWidget *ilist;
	int        pos;
	GList     *scan;

	ilist = file_list->ilist;
	image_list_freeze (IMAGE_LIST (ilist));
	image_list_clear (IMAGE_LIST (ilist));

	for (scan = file_list->list; scan; scan = scan->next) {
		FileData *fd = scan->data;

		pos = image_list_append (IMAGE_LIST (ilist),
					 unknown_pixbuf,
					 fd->name,
					 fd->comment);

		image_list_set_image_data (IMAGE_LIST (ilist), pos, fd);
	}

	image_list_sort (IMAGE_LIST (ilist));
	image_list_thaw (IMAGE_LIST (ilist));

	if ((file_list->list != NULL) && file_list->enable_thumbs)
		file_list_update_thumbs (file_list);
}


static void
file_list_update_list (FileList *file_list)
{
	file_list_interrupt_thumbs (file_list, (DoneFunc) update_list__step2, file_list);
}



/* ----- file_list_set_list help functions ----- */


typedef struct {
	FileList *file_list;
	GList    *filtered;
	GList    *uri_list;
	DoneFunc  done_func;
	gpointer  done_func_data;
} GetFileInfoData;


static GetFileInfoData *
get_file_info_data_new (FileList *file_list, 
			DoneFunc  done_func,
			gpointer  done_func_data)
{
	GetFileInfoData * data;

	data = g_new0 (GetFileInfoData, 1);
	data->file_list = file_list;
	data->filtered = NULL;
	data->uri_list = NULL;
	data->done_func = done_func;
	data->done_func_data = done_func_data;

	return data;
}


static void
get_file_info_data_free (GetFileInfoData *data)
{
	if (data == NULL)
		return;

	if (data->uri_list != NULL) {
		g_list_foreach (data->uri_list, 
				(GFunc) gnome_vfs_uri_unref, 
				NULL);
		g_list_free (data->uri_list);
	}

	if (data->filtered != NULL) {
		g_list_foreach (data->filtered, 
				(GFunc) file_data_unref, 
				NULL);
		g_list_free (data->filtered);
	}

	g_free (data);
}


/* -- file_list_set_list -- */


static void 
set_list__get_file_info_done_cb (GnomeVFSAsyncHandle *handle,
				 GList *results, /* GnomeVFSGetFileInfoResult* items */
				 gpointer callback_data)
{
	GetFileInfoData *gfi_data = callback_data;
	FileList        *file_list = gfi_data->file_list;
	GList           *scan;
	DoneFunc         done_func;

	if (file_list->interrupt_set_list) {
		done_func = file_list->interrupt_done_func;
		file_list->interrupt_done_func = NULL;
		if (done_func != NULL)
			(*done_func) (file_list->interrupt_done_data);
		get_file_info_data_free (gfi_data);
		return;
	}

	for (scan = results; scan; scan = scan->next) {
		GnomeVFSGetFileInfoResult *info_result = scan->data;
		char                      *full_path;
		char                      *escaped;
		FileData                  *fd;

		if (info_result->result != GNOME_VFS_OK) 
			continue;

		escaped = gnome_vfs_uri_to_string (info_result->uri, GNOME_VFS_URI_HIDE_TOPLEVEL_METHOD);
		full_path = gnome_vfs_unescape_string (escaped, "/");
		g_free (escaped);

		fd = file_data_new (full_path, info_result->file_info);
		g_free (full_path);

		file_data_update_comment (fd);
		gfi_data->filtered = g_list_prepend (gfi_data->filtered, fd);
	}

	/**/

	file_list->list = gfi_data->filtered;
	gfi_data->filtered = NULL;
	file_list_update_list (file_list);

	done_func = gfi_data->done_func;
	gfi_data->done_func = NULL;
	file_list->interrupt_done_func = NULL;
	if (done_func != NULL)
		(*done_func) (gfi_data->done_func_data);

	get_file_info_data_free (gfi_data);
}


void
set_list__step2 (GetFileInfoData *gfi_data)
{
	GnomeVFSAsyncHandle *handle;
	FileList            *file_list = gfi_data->file_list;

	if (file_list->interrupt_set_list) {
		if (file_list->interrupt_done_func != NULL)
			(*file_list->interrupt_done_func) (file_list->interrupt_done_data);
		file_list->interrupt_done_func = NULL;
		get_file_info_data_free (gfi_data);
		return;
	}

 	file_list_free_list (file_list);
	gnome_vfs_async_get_file_info (&handle,
				       gfi_data->uri_list,
				       (GNOME_VFS_FILE_INFO_DEFAULT 
					| GNOME_VFS_FILE_INFO_FOLLOW_LINKS),
				       GNOME_VFS_PRIORITY_MAX,
				       set_list__get_file_info_done_cb,
				       gfi_data);	
}


void 
file_list_set_list (FileList *file_list,
		    GList    *new_list,
		    DoneFunc  done_func,
		    gpointer  done_func_data)
{
	GetFileInfoData *gfi_data;
	GList           *scan;

	g_return_if_fail (file_list != NULL);

	file_list->interrupt_set_list = FALSE;
	gfi_data = get_file_info_data_new (file_list, 
					   done_func, 
					   done_func_data);

	for (scan = new_list; scan; scan = scan->next) {
		char        *full_path = scan->data;
		const char  *name_only = file_name_from_path (full_path);
		char        *escaped;
		GnomeVFSURI *uri;

		if ((! gfi_data->file_list->show_dot_files 
		     && file_is_hidden (name_only))
		    || ! file_is_image (full_path, eel_gconf_get_boolean (PREF_FAST_FILE_TYPE)))
			continue;

		escaped = gnome_vfs_escape_path_string (full_path);
		uri = gnome_vfs_uri_new (escaped);
		g_free (escaped);

		if (uri != NULL)
			gfi_data->uri_list = g_list_prepend (gfi_data->uri_list, uri);
	}

	if (file_list->doing_thumbs)
		file_list_interrupt_thumbs (file_list, 
					    (DoneFunc) set_list__step2, 
					    gfi_data);
	else
		set_list__step2 (gfi_data);
}


void
file_list_interrupt_set_list (FileList *file_list,
			      DoneFunc done_func,
			      gpointer done_data)
{
	g_return_if_fail (file_list != NULL);

	file_list->interrupt_set_list = TRUE;
	file_list->interrupt_done_func = done_func;
	file_list->interrupt_done_data = done_data;
}


/* -- file_list_add_list -- */


static void 
add_list__get_file_info_done_cb (GnomeVFSAsyncHandle *handle,
				 GList *results, /* GnomeVFSGetFileInfoResult* items */
				 gpointer callback_data)
{
	GetFileInfoData *gfi_data = callback_data;
	FileList        *file_list = gfi_data->file_list;
	GtkWidget       *ilist = file_list->ilist;
	GList           *scan;
	DoneFunc         done_func;

	if (file_list->interrupt_set_list) {
		done_func = file_list->interrupt_done_func;
		file_list->interrupt_done_func = NULL;
		if (done_func != NULL)
			(*done_func) (file_list->interrupt_done_data);
		get_file_info_data_free (gfi_data);
		return;
	}

	for (scan = results; scan; scan = scan->next) {
		GnomeVFSGetFileInfoResult *info_result = scan->data;
		char                      *full_path;
		char                      *escaped;
		FileData                  *fd;

		if (info_result->result != GNOME_VFS_OK) 
			continue;

		escaped = gnome_vfs_uri_to_string (info_result->uri, GNOME_VFS_URI_HIDE_TOPLEVEL_METHOD);
		full_path = gnome_vfs_unescape_string (escaped, "/");
		g_free (escaped);

		fd = file_data_new (full_path, info_result->file_info);
		g_free (full_path);

		file_data_update_comment (fd);
		gfi_data->filtered = g_list_prepend (gfi_data->filtered, fd);
	}

	if (gfi_data->filtered != NULL) {
		image_list_freeze (IMAGE_LIST (ilist));

		for (scan = file_list->list; scan; scan = scan->next) {
			FileData *fd = scan->data;
			fd->thumb = TRUE;
			fd->error = FALSE;
		}

		for (scan = gfi_data->filtered; scan; scan = scan->next) {
			FileData *fd = scan->data;
			int       pos;
			
			pos = image_list_append (IMAGE_LIST (ilist),
						 unknown_pixbuf,
						 fd->name,
						 fd->comment);
			
			image_list_set_image_data (IMAGE_LIST (ilist), pos, fd);
		}
		image_list_sort (IMAGE_LIST (ilist));
		image_list_thaw (IMAGE_LIST (ilist));
		
		file_list->list = g_list_concat (file_list->list, 
						 gfi_data->filtered);
		gfi_data->filtered = NULL;

		if ((file_list->list != NULL) && file_list->enable_thumbs) {
			file_list->doing_thumbs = TRUE;
			file_list_update_next_thumb (file_list);
		}
	}

	/**/

	done_func = gfi_data->done_func;
	gfi_data->done_func = NULL;
	file_list->interrupt_done_func = NULL;
	if (done_func != NULL)
		(*done_func) (gfi_data->done_func_data);

	get_file_info_data_free (gfi_data);
}


void
add_list__step2 (GetFileInfoData *gfi_data)
{
	GnomeVFSAsyncHandle *handle;
	FileList            *file_list = gfi_data->file_list;

	if (file_list->interrupt_set_list) {
		if (file_list->interrupt_done_func != NULL)
			(*file_list->interrupt_done_func) (file_list->interrupt_done_data);
		file_list->interrupt_done_func = NULL;
		get_file_info_data_free (gfi_data);
		return;
	}

	/* file_list_free_list (file_list); FIXME */
	gnome_vfs_async_get_file_info (&handle,
				       gfi_data->uri_list,
				       (GNOME_VFS_FILE_INFO_DEFAULT 
					| GNOME_VFS_FILE_INFO_FOLLOW_LINKS),
				       GNOME_VFS_PRIORITY_MAX,
				       add_list__get_file_info_done_cb,
				       gfi_data);	
}


void 
file_list_add_list (FileList *file_list,
		    GList    *new_list,
		    DoneFunc  done_func,
		    gpointer  done_func_data)
{
	GetFileInfoData *gfi_data;
	GList           *scan;

	g_return_if_fail (file_list != NULL);

	file_list->interrupt_set_list = FALSE;
	gfi_data = get_file_info_data_new (file_list, 
					   done_func, 
					   done_func_data);

	for (scan = new_list; scan; scan = scan->next) {
		char        *full_path = scan->data;
		const char  *name_only = file_name_from_path (full_path);
		char        *escaped;
		GnomeVFSURI *uri;

		if (file_list_pos_from_path (file_list, full_path) != -1)
			continue;

		if ((! gfi_data->file_list->show_dot_files 
		     && file_is_hidden (name_only))
		    || ! file_is_image (full_path, eel_gconf_get_boolean (PREF_FAST_FILE_TYPE)))
			continue;
		
		escaped = gnome_vfs_escape_path_string (full_path);
		uri = gnome_vfs_uri_new (escaped);
		g_free (escaped);

		if (uri != NULL)
			gfi_data->uri_list = g_list_prepend (gfi_data->uri_list, uri);
	}
	
	if (gfi_data->uri_list == NULL) {
		get_file_info_data_free (gfi_data);
		if (done_func != NULL)
			(*done_func) (done_func_data);
		return;
	}

	if (file_list->doing_thumbs)
		file_list_interrupt_thumbs (file_list, 
					    (DoneFunc) add_list__step2, 
					    gfi_data);
	else
		add_list__step2 (gfi_data);
}


/* -- InterruptThumbsData -- */


typedef struct {
	FileList *file_list;
	gboolean  restart_thumbs;
	int       i_data;
	char     *s_data;
} InterruptThumbsData;


static InterruptThumbsData *
it_data_new (FileList   *file_list,
	     gboolean    restart_thumbs,
	     int         i_data,
	     const char *s_data)
{
	InterruptThumbsData *it_data;

	it_data = g_new (InterruptThumbsData, 1);
	it_data->file_list = file_list;
	it_data->restart_thumbs = restart_thumbs;
	it_data->i_data = i_data;
	if (s_data != NULL)
		it_data->s_data = g_strdup (s_data);
	else
		it_data->s_data = NULL;

	return it_data;
}


static void
it_data_free (InterruptThumbsData *it_data)
{
	if (it_data->s_data != NULL)
		g_free (it_data->s_data);
	g_free (it_data);
}


/* -- -- */


void
set_sort_method__step2 (InterruptThumbsData *it_data)
{
	FileList   *file_list = it_data->file_list;
	SortMethod  new_method = it_data->i_data;

	file_list->sort_method = new_method;
	image_list_set_compare_func (IMAGE_LIST (file_list->ilist), 
				     get_compfunc_from_method (new_method));
	image_list_sort (IMAGE_LIST (file_list->ilist));

	if (it_data->restart_thumbs) {
		file_list->doing_thumbs = TRUE;
		file_list_update_next_thumb (file_list);
	}
	
	it_data_free (it_data);
}


void 
file_list_set_sort_method (FileList   *file_list,
			   SortMethod  new_method)
{
	InterruptThumbsData *it_data;

	g_return_if_fail (file_list != NULL);

	if (file_list->sort_method == new_method) 
		return;

	if (file_list->doing_thumbs) {
		it_data = it_data_new (file_list, TRUE, new_method, NULL);
		file_list_interrupt_thumbs (file_list, 
					    (DoneFunc) set_sort_method__step2,
					    it_data);
	} else {
		it_data = it_data_new (file_list, FALSE, new_method, NULL);
		set_sort_method__step2 (it_data);
	}
}


/* -- -- */


void
set_sort_type__step2 (InterruptThumbsData *it_data)
{
	FileList    *file_list = it_data->file_list;
	GtkSortType  sort_type = it_data->i_data;

	file_list->sort_type = sort_type;
	image_list_set_sort_type (IMAGE_LIST (file_list->ilist), sort_type);
	image_list_sort (IMAGE_LIST (file_list->ilist));

	if (it_data->restart_thumbs) {
		file_list->doing_thumbs = TRUE;
		file_list_update_next_thumb (file_list);
	}
	
	it_data_free (it_data);
}


void
file_list_set_sort_type (FileList    *file_list,
			 GtkSortType  sort_type)
{
	InterruptThumbsData *it_data;

	g_return_if_fail (file_list != NULL);

	if (file_list->doing_thumbs) {
		it_data = it_data_new (file_list, TRUE, sort_type, NULL);
		file_list_interrupt_thumbs (file_list, 
					    (DoneFunc) set_sort_type__step2,
					    it_data);
	} else {
		it_data = it_data_new (file_list, FALSE, sort_type, NULL);
		set_sort_type__step2 (it_data);
	}
}


/* -- -- */


static void file_list_thumb_cleanup (FileList *file_list);


static void
interrupt_thumbs__part2 (FileList *file_list)
{
	DoneFunc done_func = file_list->interrupt_done_func;

	file_list_thumb_cleanup (file_list);
	file_list->interrupt_done_func = NULL;
	if (done_func != NULL)
		(*done_func) (file_list->interrupt_done_data);
}


void 
file_list_interrupt_thumbs (FileList *file_list, 
			    DoneFunc  done_func,
			    gpointer  done_func_data)
{
	g_return_if_fail (file_list != NULL);

        if (file_list->doing_thumbs) {
		file_list->interrupt_done_func = done_func;
		file_list->interrupt_done_data = done_func_data;
		file_list->doing_thumbs = FALSE;
	} else if (done_func != NULL)
		(*done_func) (done_func_data);
		
}


gint 
file_list_pos_from_path (FileList    *file_list, 
			 const char  *path)
{
	GList *scan;
	int    i;

	g_return_val_if_fail (file_list != NULL, -1);

	if (path == NULL)
		return -1;

	i = 0;
	scan = image_list_get_list (IMAGE_LIST (file_list->ilist));
	for (; scan; scan = scan->next) {
		FileData *fd;

		fd = IMAGE (scan->data)->data;

		if (strcmp (fd->path, path) == 0)
			return i;

		i++;
	}

	return -1;
}


GList *
file_list_get_all (FileList *file_list)
{
	GList *list;
	GList *scan;

	g_return_val_if_fail (file_list != NULL, NULL);

	list = NULL;
	for (scan = file_list->list; scan; scan = scan->next) {
		FileData *fd = scan->data;
		list = g_list_prepend (list, g_strdup (fd->path));
	}

	return g_list_reverse (list);
}


gint 
file_list_get_length (FileList *file_list)
{
	g_return_val_if_fail (file_list != NULL, 0);
	return g_list_length (file_list->list);
}


GList *
file_list_get_selection (FileList *file_list)
{
	GList *list = NULL;
	GList *scan;

	g_return_val_if_fail (file_list != NULL, NULL);

	scan = IMAGE_LIST (file_list->ilist)->selection;
	for (; scan; scan = scan->next) {
		FileData *fd = image_list_get_image_data (IMAGE_LIST (file_list->ilist), GPOINTER_TO_INT (scan->data));
		if (fd) list = g_list_prepend (list, g_strdup (fd->path));
	}

	return g_list_reverse (list);
}


GList *
file_list_get_selection_as_fd (FileList *file_list)
{
	GList *list = NULL;
	GList *scan;

	g_return_val_if_fail (file_list != NULL, NULL);

	scan = IMAGE_LIST (file_list->ilist)->selection;
	for (; scan; scan = scan->next) {
		FileData *fd = image_list_get_image_data (IMAGE_LIST (file_list->ilist), GPOINTER_TO_INT (scan->data));
		if (fd) list = g_list_prepend (list, fd);
	}

	return g_list_reverse (list);
}


gint 
file_list_get_selection_length (FileList *file_list)
{
	g_return_val_if_fail (file_list != NULL, 0);
	return g_list_length (IMAGE_LIST (file_list->ilist)->selection);
}


gchar *
file_list_path_from_pos (FileList *file_list,
			 int       pos)
{
	FileData *fd;

	g_return_val_if_fail (file_list != NULL, NULL);

	if ((pos < 0) || (pos >= IMAGE_LIST (file_list->ilist)->images))
		return NULL;

	fd = image_list_get_image_data (IMAGE_LIST (file_list->ilist), pos);
	if ((fd != NULL) && (fd->path != NULL))
		return g_strdup (fd->path);

	return NULL;
}


gboolean
file_list_is_selected (FileList *file_list,
		       int       pos)
{
	GList *scan;

	g_return_val_if_fail (file_list != NULL, FALSE);

	scan = IMAGE_LIST (file_list->ilist)->selection;
	for (; scan; scan = scan->next) 
		if (GPOINTER_TO_INT (scan->data) == pos) 
			return TRUE;

	return FALSE;
}


void 
file_list_select_image_by_pos (FileList *file_list,
			       int       pos)
{
	GtkWidget        *ilist;
	GthumbVisibility  visibility;

	g_return_if_fail (file_list != NULL);

	ilist = file_list->ilist;

	image_list_unselect_all (IMAGE_LIST (ilist), NULL, FALSE);
	image_list_select_image (IMAGE_LIST (ilist), pos);
	image_list_focus_image (IMAGE_LIST (ilist), pos);

	visibility = image_list_image_is_visible (IMAGE_LIST (ilist), pos);
	if (visibility != GTHUMB_VISIBILITY_FULL) {
		gdouble offset;
		switch (visibility) {
		case GTHUMB_VISIBILITY_NONE:
			offset = 0.5; 
			break;
		case GTHUMB_VISIBILITY_PARTIAL_TOP:
			offset = 0.0; 
			break;
		case GTHUMB_VISIBILITY_PARTIAL_BOTTOM:
			offset = 1.0; 
			break;
		case GTHUMB_VISIBILITY_PARTIAL:
		case GTHUMB_VISIBILITY_FULL:
			return;
		}
		image_list_moveto (IMAGE_LIST (ilist), pos, offset);
	}
}


void 
file_list_select_all (FileList *file_list)
{
	g_return_if_fail (file_list != NULL);
	image_list_select_all (IMAGE_LIST (file_list->ilist));
}


void 
file_list_unselect_all (FileList *file_list)
{
	g_return_if_fail (file_list != NULL);
	image_list_unselect_all (IMAGE_LIST (file_list->ilist), NULL, FALSE);
}


static void
enable_thumbs__step2 (FileList *file_list)
{
	ImageList *ilist = IMAGE_LIST (file_list->ilist);
	int        i;
	
	for (i = 0; i < ilist->images; i++)
		image_list_set_image_pixbuf (ilist, i, unknown_pixbuf);
}


void
file_list_enable_thumbs (FileList *file_list,
			 gboolean  enable)
{
	g_return_if_fail (file_list != NULL);

	file_list->enable_thumbs = enable;

	if (file_list->enable_thumbs)
		file_list_update_thumbs (file_list);
	else 
		file_list_interrupt_thumbs (file_list, 
					    (DoneFunc) enable_thumbs__step2, 
					    file_list);
}


void
file_list_set_progress_func (FileList *file_list,
			    ProgressFunc func,
			    gpointer data)
{
	g_return_if_fail (file_list != NULL);

	file_list->progress_func = func;
	file_list->progress_data = data;
}


/**
 * file_list_next_image:
 * @file_list: 
 * @pos: 
 * @without_error: 
 * 
 * returns the next row that contains an image starting from row 
 * @row, if @without_error is %TRUE ignore the rows that contains 
 * broken images.
 * 
 * Return value: the next row that contains an image or -1 if no row could
 * be found.
 **/
gint
file_list_next_image (FileList *file_list,
		      int       pos,
		      gboolean  without_error)
{
	int n;

	g_return_val_if_fail (file_list != NULL, -1);

	n = IMAGE_LIST (file_list->ilist)->images;

	pos++;
	if (! without_error) {
		if (pos >= n)
			return -1;
		return pos;
	}

	while (pos < n) {
		FileData *fd;

		fd = image_list_get_image_data (IMAGE_LIST (file_list->ilist),
						pos);
		if (! fd->error)
			break;
		pos++;
	}

	if (pos >= n)
		return -1;

	return pos;
}


gint
file_list_prev_image (FileList *file_list,
		      int       pos,
		      gboolean  without_error)
{
	g_return_val_if_fail (file_list != NULL, -1);

	pos--;
	if (! without_error) {
		if (pos < 0)
			return -1;
		return pos;
	}

	while (pos >= 0) {
		FileData *fd;

		fd = image_list_get_image_data (IMAGE_LIST (file_list->ilist),
						pos);
		if (! fd->error)
			break;
		pos--;
	}

	if (pos < 0)
		return -1;

	return pos;
}


/* -- -- */


void
delete_pos__step2 (InterruptThumbsData *it_data)
{
	FileList  *file_list = it_data->file_list;
	int        pos = it_data->i_data;
	ImageList *ilist = IMAGE_LIST (file_list->ilist);
	FileData  *fd;

	fd = image_list_get_image_data (ilist, pos);

	g_return_if_fail (fd != NULL);

	file_list->list = g_list_remove (file_list->list, fd);
	file_data_unref (fd);

	image_list_remove (ilist, pos);

	if (it_data->restart_thumbs) {
		file_list->doing_thumbs = TRUE;
		file_list_update_next_thumb (file_list);
	}

	it_data_free (it_data);
}


void
file_list_delete_pos (FileList *file_list,
		      int       pos)
{
	ImageList           *ilist;
	InterruptThumbsData *it_data;

	g_return_if_fail (file_list != NULL);
	ilist = IMAGE_LIST (file_list->ilist);

	if ((pos < 0) || (pos >= ilist->images))
		return;

	if (file_list->doing_thumbs) {
		it_data = it_data_new (file_list, TRUE, pos, NULL);
		file_list_interrupt_thumbs (file_list, 
					    (DoneFunc) delete_pos__step2, 
					    it_data);
	} else {
		it_data = it_data_new (file_list, FALSE, pos, NULL);
		delete_pos__step2 (it_data);
	}
}


/* -- -- */


void
rename_pos__step2 (InterruptThumbsData *it_data)
{
	FileList    *file_list = it_data->file_list;
	int          pos = it_data->i_data;
	char *       path = it_data->s_data;
	ImageList   *ilist = IMAGE_LIST (file_list->ilist);
	FileData    *fd;

	/* update the FileData structure. */
	fd = image_list_get_image_data (ilist, pos);
	file_data_set_path (fd, path);

	/* Set the new name. */
	image_list_set_image_text (ilist, pos, fd->name);
	image_list_sort (ilist); 

	if (it_data->restart_thumbs) {
		file_list->doing_thumbs = TRUE;
		file_list_update_next_thumb (file_list);
	}

	it_data_free (it_data);
}


void
file_list_rename_pos (FileList   *file_list,
		      int         pos, 
		      const char *path)
{
	ImageList           *ilist;
	InterruptThumbsData *it_data;

	g_return_if_fail (file_list != NULL);
	ilist = IMAGE_LIST (file_list->ilist);

	if ((pos < 0) || (pos >= ilist->images))
		return;

	if (file_list->doing_thumbs) {
		it_data = it_data_new (file_list, TRUE, pos, path);
		file_list_interrupt_thumbs (file_list, 
					    (DoneFunc) rename_pos__step2, 
					    it_data);
	} else {
		it_data = it_data_new (file_list, FALSE, pos, path);
		rename_pos__step2 (it_data);
	}
}


/* -- -- */


void
update_comment__step2 (InterruptThumbsData *it_data)
{
	FileList    *file_list = it_data->file_list;
	int          pos = it_data->i_data;
	ImageList   *ilist = IMAGE_LIST (file_list->ilist);
	FileData    *fd;

	/* update the FileData structure. */
	fd = image_list_get_image_data (ilist, pos);
	file_data_update_comment (fd);

	/* Set the new name. */
	image_list_freeze (ilist);
	image_list_set_image_comment (ilist, pos, fd->comment);
	image_list_thaw (ilist);

	if (it_data->restart_thumbs) {
		file_list->doing_thumbs = TRUE;
		file_list_update_next_thumb (file_list);
	}

	it_data_free (it_data);
}


void
file_list_update_comment (FileList *file_list,
			  int       pos)
{
	ImageList           *ilist;
	InterruptThumbsData *it_data;

	g_return_if_fail (file_list != NULL);
	
	ilist = IMAGE_LIST (file_list->ilist);

	if ((pos < 0) || (pos >= ilist->images))
		return;

	if (file_list->doing_thumbs) {
		it_data = it_data_new (file_list, TRUE, pos, NULL);
		file_list_interrupt_thumbs (file_list, 
					    (DoneFunc) update_comment__step2, 
					    it_data);
	} else {
		it_data = it_data_new (file_list, FALSE, pos, NULL);
		update_comment__step2 (it_data);
	}
}


/* -- -- */


void
set_thumbs_size__step2 (InterruptThumbsData *it_data)
{
	FileList    *file_list = it_data->file_list;
	int          size = it_data->i_data;

	file_list->thumb_size = size;

	g_object_unref (G_OBJECT (file_list->thumb_loader));
	file_list->thumb_loader = THUMB_LOADER (thumb_loader_new (NULL, size, size));
	g_signal_connect (G_OBJECT (file_list->thumb_loader), 
			  "done",
			  G_CALLBACK (load_thumb_done_cb), 
			  file_list);
	g_signal_connect (G_OBJECT (file_list->thumb_loader), 
			  "error",
			  G_CALLBACK (load_thumb_error_cb), 
			  file_list);

	image_list_set_image_width (IMAGE_LIST (file_list->ilist), 
				    size + THUMB_BORDER);

	if (it_data->restart_thumbs) {
		file_list->doing_thumbs = TRUE;
		file_list_update_next_thumb (file_list);
	}

	it_data_free (it_data);
}


void
file_list_set_thumbs_size (FileList *file_list,
			   int       size)
{
	InterruptThumbsData *it_data;

	g_return_if_fail (file_list != NULL);
	if (file_list->thumb_size == size) return;

	if (file_list->doing_thumbs) {
		it_data = it_data_new (file_list, TRUE, size, NULL);
		file_list_interrupt_thumbs (file_list, 
					    (DoneFunc) set_thumbs_size__step2, 
					    it_data);
	} else {
		it_data = it_data_new (file_list, FALSE, size, NULL);
		set_thumbs_size__step2 (it_data);
	}
}


/* -- -- */


static void
file_list_update_current_thumb (FileList *file_list)
{
	if (path_is_file (file_list->thumb_fd->path)) {
		thumb_loader_set_path (file_list->thumb_loader, 
				       file_list->thumb_fd->path);
		thumb_loader_start (file_list->thumb_loader);
		
	} else /* Error: the file does not exists. */
		g_signal_emit_by_name (G_OBJECT (file_list->thumb_loader),
				       "error",
				       0,
				       file_list);
}


void
file_list_update_thumb (FileList     *file_list,
			int           pos)
{
	FileData  *fd;

	if (! file_list->enable_thumbs)
		return;

	fd = image_list_get_image_data (IMAGE_LIST (file_list->ilist), pos);
	file_data_update (fd);
	fd->error = FALSE;
	fd->thumb = FALSE;

	file_list->thumb_pos = pos;
	/* file_list->thumbs_num++; */
	file_list->thumb_fd = fd;
	
	/* file_list->doing_thumbs = FALSE; FIXME */
	file_list_update_current_thumb (file_list);
}


void
file_list_update_thumb_list (FileList     *file_list,
			     GList        *list)
{
	GList     *scan;
	ImageList *ilist;

	if (! file_list->enable_thumbs)
		return;

	for (scan = file_list->list; scan; scan = scan->next) {
		FileData *fd = scan->data;
		fd->thumb = TRUE;
		fd->error = FALSE;
	}

	ilist = IMAGE_LIST (file_list->ilist);
	for (scan = list; scan; scan = scan->next) {
		char      *path = scan->data;
		FileData  *fd;
		int        pos;

		pos = file_list_pos_from_path (file_list, path);

		if (pos == -1)
			continue;

		fd = image_list_get_image_data (ilist, pos);

		file_data_update (fd);
		fd->error = FALSE;
		fd->thumb = FALSE;
	}
	
	file_list->doing_thumbs = TRUE;
	file_list_update_next_thumb (file_list);
}



/* -- thumbs update -- */


static void 
file_list_thumb_cleanup (FileList *file_list)
{
	file_list->thumbs_num = 0;
	file_list->doing_thumbs = FALSE;

	if (file_list->progress_func) 
		file_list->progress_func (0.0, file_list->progress_data);
}


static void
file_list_update_next_thumb (FileList *file_list)
{
	ImageList *ilist;
	FileData  *fd = NULL;
	int        new_pos = -1;
	int        pos = -1;

	if (! file_list->doing_thumbs) {
		interrupt_thumbs__part2 (file_list);
		return;
	}

	ilist = IMAGE_LIST (file_list->ilist);

	/* Find first visible undone. */
	pos = image_list_get_first_visible (ilist);
	if (pos != -1) { 
		GList *scan; 
		int n = ilist->images;

		scan = image_list_get_list (ilist);
		scan = g_list_nth (scan, pos);
		while ((pos < n) && image_list_image_is_visible (ilist, pos)) {
			Image *image = scan->data;			
			fd = image->data;
			if (! fd->thumb && ! fd->error) {
				new_pos = pos;
				break;
			} else {
				pos++;
				scan = scan->next;
			}
		}
	}

	/* If not found then search the first undone. */
	if (new_pos == -1) {
		GList *scan;
		int    n = ilist->images;

		pos = 0;
		scan = image_list_get_list (ilist);
		while ((pos < n) && (new_pos == -1)) {
			Image *image = scan->data;			
			fd = image->data;			
			if (! fd->thumb && ! fd->error) 
				new_pos = pos;
			else {
				pos++;
				scan = scan->next;
			}
		}
		
		if (pos == n) {
			/* Thumbnails creation terminated. */
			file_list_thumb_cleanup (file_list);
			return;
		}
	}

	g_assert (new_pos != -1);
	g_assert (fd != NULL);

	file_list->thumb_pos = new_pos;
	file_list->thumbs_num++;
	file_list->thumb_fd = fd;

	file_list_update_current_thumb (file_list);

	/* FIXME */

#if 0
	if (path_is_file (file_list->thumb_fd->path)) {
		thumb_loader_set_path (file_list->thumb_loader, 
				       file_list->thumb_fd->path);
		thumb_loader_start (file_list->thumb_loader);

	} else /* Error: the file does not exists. */
		g_signal_emit_by_name (G_OBJECT (file_list->thumb_loader),
				       "error",
				       0,
				       file_list);
#endif
}


static void 
file_list_update_thumbs (FileList *file_list)
{
	GList *scan;

	if (! file_list->enable_thumbs)
		return;

	thumb_loader_set_max_file_size (THUMB_LOADER (file_list->thumb_loader), eel_gconf_get_integer (PREF_THUMBNAIL_LIMIT));

	for (scan = file_list->list; scan; scan = scan->next) {
		FileData *fd = scan->data;
		fd->thumb = FALSE;
		fd->error = FALSE;
	}
		
	file_list->doing_thumbs = TRUE;
	file_list_update_next_thumb (file_list);
}


void 
file_list_restart_thumbs (FileList *file_list,
			  gboolean  _continue)
{
	if (! file_list->enable_thumbs)
		return;

	if (_continue) {
		file_list->doing_thumbs = TRUE;
		file_list_update_next_thumb (file_list);
	} else
		file_list_update_thumbs (file_list);	
}
