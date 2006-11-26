/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001, 2003 Free Software Foundation, Inc.
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
#include <string.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <libgnomevfs/gnome-vfs-types.h>
#include <libgnomevfs/gnome-vfs-ops.h>
#include <libgnomevfs/gnome-vfs-file-info.h>
#include <libgnomevfs/gnome-vfs-utils.h>

#include "gconf-utils.h"
#include "gthumb-init.h"
#include "file-data.h"
#include "gth-file-list.h"
#include "file-utils.h"
#include "gthumb-marshal.h"
#include "gth-file-view.h"
#include "gth-file-view-list.h"
#include "gth-file-view-thumbs.h"
#include "thumb-loader.h"
#include "typedefs.h"

#define THUMB_BORDER   14
#define ADD_LIST_DELAY 30
#define ADD_LIST_CHUNK 500
#define SCROLL_DELAY   20
#define DEF_THUMB_SIZE 128

enum {
	BUSY,
	IDLE,
	LAST_SIGNAL
};

static GObjectClass *parent_class = NULL;
static guint         gth_file_list_signals[LAST_SIGNAL] = { 0 };


static void 
update_thumb_in_clist (GthFileList *file_list)
{
	GdkPixbuf *pixbuf;

	pixbuf = thumb_loader_get_pixbuf (file_list->thumb_loader);

	if (pixbuf == NULL)
		return;

	gth_file_view_set_image_pixbuf (file_list->view, 
					file_list->thumb_pos, 
					pixbuf);
}


static void gth_file_list_update_next_thumb (GthFileList *file_list);
static void interrupt_thumbs__part2 (GthFileList *file_list);


static void 
load_thumb_error_cb (ThumbLoader *tl,
		     gpointer     data)
{
	GthFileList *file_list = data;
	float        percent;

	if (file_list == NULL)
		return;

	if (file_list->interrupt_thumbs) {
		interrupt_thumbs__part2 (file_list);
		return;
	}

	gth_file_view_set_unknown_pixbuf (file_list->view,  file_list->thumb_pos);

	file_list->thumb_fd->error = TRUE;
	file_list->thumb_fd->thumb = FALSE;

	percent = ((float) (file_list->thumbs_num - 1) 
		   / gth_file_view_get_images (file_list->view));

	if (file_list->progress_func) 
		file_list->progress_func (percent, file_list->progress_data);

	gth_file_list_update_next_thumb (file_list);
}


static void 
load_thumb_done_cb (ThumbLoader *tl,
		    gpointer     data)
{
	GthFileList *file_list = data;
	float        percent;

	if (file_list == NULL)
		return;

	if (file_list->interrupt_thumbs) {
		interrupt_thumbs__part2 (file_list);
		return;
	}

	update_thumb_in_clist (file_list);
	file_list->thumb_fd->error = FALSE;
	file_list->thumb_fd->thumb = TRUE;

	percent = ((float) (file_list->thumbs_num - 1) 
		   / gth_file_view_get_images (file_list->view));

	if (file_list->progress_func) 
		file_list->progress_func (percent, file_list->progress_data);

	gth_file_list_update_next_thumb (file_list);
}


static void
gth_file_list_free_list (GthFileList *file_list)
{
	g_return_if_fail (file_list != NULL);

	file_data_list_free (file_list->list);
	file_list->list = NULL;
}


static void 
gth_file_list_finalize (GObject *object)
{
	GthFileList *file_list;

        g_return_if_fail (GTH_IS_FILE_LIST (object));
	file_list = GTH_FILE_LIST (object);
	
	if (file_list->thumb_fd != NULL) {
		file_data_unref (file_list->thumb_fd);
		file_list->thumb_fd = NULL;
	}

	gth_file_list_free_list (file_list);
	g_object_unref (file_list->thumb_loader);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gth_file_list_class_init (GthFileListClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);

	gth_file_list_signals[BUSY] =
		g_signal_new ("busy",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthFileListClass, busy),
			      NULL, NULL,
			      gthumb_marshal_VOID__VOID,
			      G_TYPE_NONE, 
			      0);
	gth_file_list_signals[IDLE] =
		g_signal_new ("idle",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthFileListClass, idle),
			      NULL, NULL,
			      gthumb_marshal_VOID__VOID,
			      G_TYPE_NONE, 
			      0);

	object_class = G_OBJECT_CLASS (class);
	object_class->finalize = gth_file_list_finalize;
}


static void
update_thumbnails__step2 (gpointer data)
{
	GthFileList *file_list = data;

	file_list->starting_update = FALSE;
	gth_file_list_restart_thumbs (file_list, TRUE);
}


static gboolean
update_thumbnails_cb (gpointer data)
{
	GthFileList *file_list = data;

	if (file_list->scroll_timer != 0)
		g_source_remove (file_list->scroll_timer);
	file_list->scroll_timer = 0;

	gth_file_list_interrupt_thumbs (file_list,
					update_thumbnails__step2,
					file_list);

	return FALSE;
}


static gboolean
file_list_adj_value_changed (GtkAdjustment *adj, 
			     GthFileList   *file_list)
{	
	if (gth_file_view_is_frozen (file_list->view))
		return FALSE;

	if (file_list->starting_update)
		return FALSE;

	file_list->starting_update = TRUE;

	if (file_list->scroll_timer != 0) {
		g_source_remove (file_list->scroll_timer);
		file_list->scroll_timer = 0;
	}

	file_list->scroll_timer = g_timeout_add (SCROLL_DELAY,
						 update_thumbnails_cb,
						 file_list);

	return FALSE;
}


static void
gth_file_list_init (GthFileList *file_list)
{
	GtkAdjustment  *adj;
	GtkWidget      *scrolled_window;

	/* set the default values. */

	file_list->list              = NULL;
	file_list->sort_method       = pref_get_arrange_type ();
	file_list->sort_type         = pref_get_sort_order ();
	file_list->show_dot_files    = eel_gconf_get_boolean (PREF_SHOW_HIDDEN_FILES, FALSE);
	file_list->enable_thumbs     = eel_gconf_get_boolean (PREF_SHOW_THUMBNAILS, TRUE);
	file_list->thumb_size        = eel_gconf_get_integer (PREF_THUMBNAIL_SIZE, DEF_THUMB_SIZE);
	file_list->doing_thumbs      = FALSE;
	file_list->interrupt_thumbs = FALSE;
	file_list->thumb_loader      = THUMB_LOADER (thumb_loader_new (NULL, file_list->thumb_size, file_list->thumb_size));
	file_list->thumbs_num        = 0;
	file_list->thumb_fd          = NULL;
	file_list->thumb_pos         = -1;
	file_list->progress_func     = NULL;
	file_list->progress_data     = NULL;
	file_list->interrupt_done_func = NULL;
	file_list->interrupt_done_data = NULL;
	file_list->scroll_timer      = 0;

	file_list->starting_update   = FALSE;

	g_signal_connect (G_OBJECT (file_list->thumb_loader), 
			  "thumb_done",
			  G_CALLBACK (load_thumb_done_cb), 
			  file_list);
	g_signal_connect (G_OBJECT (file_list->thumb_loader), 
			  "thumb_error",
			  G_CALLBACK (load_thumb_error_cb), 
			  file_list);
	
	/* Create the widgets. */

	/* image list. */

	if (pref_get_view_as () == GTH_VIEW_AS_THUMBNAILS)
		file_list->view = gth_file_view_thumbs_new (eel_gconf_get_integer (PREF_THUMBNAIL_SIZE, DEF_THUMB_SIZE) + THUMB_BORDER);

	else if (pref_get_view_as () == GTH_VIEW_AS_LIST)
		file_list->view = gth_file_view_list_new (eel_gconf_get_integer (PREF_THUMBNAIL_SIZE, DEF_THUMB_SIZE) + THUMB_BORDER);

	gth_file_view_enable_thumbs (file_list->view, file_list->enable_thumbs);

	gth_file_view_set_image_width (file_list->view, file_list->thumb_size + THUMB_BORDER);
	gth_file_view_sorted (file_list->view, file_list->sort_method, file_list->sort_type);

	gth_file_view_set_view_mode (file_list->view, pref_get_view_mode ());

	/**/

	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
					GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type  (GTK_SCROLLED_WINDOW (scrolled_window),
					      GTK_SHADOW_ETCHED_IN);
	gtk_container_add (GTK_CONTAINER (scrolled_window), gth_file_view_get_widget (file_list->view));

	file_list->root_widget = scrolled_window;
	file_list->drag_source = gth_file_view_get_drag_source (file_list->view);
	if (file_list->drag_source == NULL)
		file_list->drag_source = file_list->root_widget;

	/* signals */

	adj = gth_file_view_get_vadjustment (file_list->view);

	g_signal_connect_after (G_OBJECT (adj), 
				"value_changed",
				G_CALLBACK (file_list_adj_value_changed),
				file_list);

	g_signal_connect_after (G_OBJECT (adj), 
				"changed",
				G_CALLBACK (file_list_adj_value_changed), 
				file_list);
}


GType
gth_file_list_get_type ()
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (GthFileListClass),
                        NULL,
                        NULL,
                        (GClassInitFunc) gth_file_list_class_init,
                        NULL,
                        NULL,
                        sizeof (GthFileList),
                        0,
                        (GInstanceInitFunc) gth_file_list_init
                };

                type = g_type_register_static (G_TYPE_OBJECT,
                                               "GthFileList",
                                               &type_info,
                                               0);
        }

        return type;
}


GthFileList*   
gth_file_list_new ()
{
	return GTH_FILE_LIST (g_object_new (GTH_TYPE_FILE_LIST, NULL));
}



/* ----- gth_file_list_set_list help functions ----- */


typedef struct {
	GthFileList *file_list;
	GList       *filtered;
	GList       *uri_list;
	GList       *new_list;
	DoneFunc     done_func;
	gpointer     done_func_data;
	guint        timeout_id;
	gboolean     doing_thumbs;
	gboolean     enable_thumbs;
} GetFileInfoData;


static GetFileInfoData *
get_file_info_data_new (GthFileList *file_list,
			GList       *new_list, 
			DoneFunc     done_func,
			gpointer     done_func_data)
{
	GetFileInfoData * data;

	data = g_new0 (GetFileInfoData, 1);
	data->file_list = file_list;
	data->filtered = NULL;
	data->uri_list = NULL;
	data->new_list = new_list;
	data->done_func = done_func;
	data->done_func_data = done_func_data;
	data->timeout_id = 0;
	data->doing_thumbs = file_list->doing_thumbs;
	data->enable_thumbs = file_list->enable_thumbs;

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

	if (data->new_list != NULL)
		path_list_free (data->new_list);

	file_data_list_free (data->filtered);

	g_free (data);
}


/* -- gth_file_list_set_list -- */


static gboolean add_list_in_chunks (gpointer callback_data);


static void 
set_list__get_file_info_done_cb (GnomeVFSAsyncHandle *handle,
				 GList *results, /* GnomeVFSGetFileInfoResult* items */
				 gpointer callback_data)
{
	GetFileInfoData *gfi_data = callback_data;
	GthFileList     *file_list = gfi_data->file_list;
	GList           *scan;

	g_signal_emit (G_OBJECT (file_list), gth_file_list_signals[IDLE], 0);

	if (file_list->interrupt_set_list) {
		DoneFunc done_func;

		file_list->interrupt_set_list = FALSE;
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

		if ((info_result->result != GNOME_VFS_OK) || (info_result->uri == NULL))
			continue;

		escaped = gnome_vfs_uri_to_string (info_result->uri, GNOME_VFS_URI_HIDE_NONE);
		full_path = gnome_vfs_unescape_string (escaped, "/");
		g_free (escaped);

		fd = file_data_new (full_path, info_result->file_info);
		g_free (full_path);

		gfi_data->filtered = g_list_prepend (gfi_data->filtered, fd);
	}

	add_list_in_chunks (gfi_data);
}


static void
set_list__step2 (GetFileInfoData *gfi_data)
{
	GnomeVFSAsyncHandle *handle;
	GthFileList         *file_list = gfi_data->file_list;
	GList               *scan;
	gboolean             fast_file_type;

	gth_file_view_set_no_image_text (file_list->view, _("Wait please..."));
	gth_file_view_clear (file_list->view); 
	gth_file_list_free_list (file_list);
	
	if (file_list->interrupt_set_list) {
		DoneFunc done_func;

		g_signal_emit (G_OBJECT (file_list), gth_file_list_signals[IDLE], 0);

		file_list->interrupt_set_list = FALSE;
		done_func = file_list->interrupt_done_func;
		file_list->interrupt_done_func = NULL;
		if (done_func != NULL)
			(*done_func) (file_list->interrupt_done_data);
		get_file_info_data_free (gfi_data);
		return;
	} 	

	/**/

	fast_file_type = eel_gconf_get_boolean (PREF_FAST_FILE_TYPE, TRUE);
	for (scan = gfi_data->new_list; scan; scan = scan->next) {
		char        *full_path = scan->data;
		const char  *name_only = file_name_from_path (full_path);
		GnomeVFSURI *uri;

		if ((! gfi_data->file_list->show_dot_files 
		     && file_is_hidden (name_only))
		    || ! file_is_image (full_path, fast_file_type))
			continue;

		uri = new_uri_from_path (full_path);
		if (uri != NULL)
			gfi_data->uri_list = g_list_prepend (gfi_data->uri_list, uri);
	}

	path_list_free (gfi_data->new_list);
	gfi_data->new_list = NULL;

	gnome_vfs_async_get_file_info (&handle,
				       gfi_data->uri_list,
				       (GNOME_VFS_FILE_INFO_DEFAULT 
					| GNOME_VFS_FILE_INFO_FOLLOW_LINKS),
				       GNOME_VFS_PRIORITY_MAX,
				       set_list__get_file_info_done_cb,
				       gfi_data);
}


void 
gth_file_list_set_list (GthFileList   *file_list,
			GList         *new_list,
			GthSortMethod  sort_method,
			GtkSortType    sort_type,
			DoneFunc       done_func,
			gpointer       done_func_data)
{
	GetFileInfoData *gfi_data;

	g_return_if_fail (file_list != NULL);

	g_signal_emit (G_OBJECT (file_list), gth_file_list_signals[BUSY], 0);

	file_list->sort_method = sort_method;
	file_list->sort_type = sort_type;
	file_list->interrupt_set_list = FALSE;

	gfi_data = get_file_info_data_new (file_list, 
					   new_list,
					   done_func, 
					   done_func_data);

	if (file_list->doing_thumbs)
		gth_file_list_interrupt_thumbs (file_list, 
						(DoneFunc) set_list__step2, 
						gfi_data);
	else
		set_list__step2 (gfi_data);
}


void
gth_file_list_interrupt_set_list (GthFileList *file_list,
				  DoneFunc     done_func,
				  gpointer     done_data)
{
	g_return_if_fail (file_list != NULL);

	if (file_list->interrupt_set_list) {
		if (done_func != NULL)
			(*done_func) (done_data);
		return;
	}

	file_list->interrupt_set_list = TRUE;
	file_list->interrupt_done_func = done_func;
	file_list->interrupt_done_data = done_data;
}


/* -- gth_file_list_add_list -- */


static void
start_update_next_thumb (GthFileList *file_list)
{
	if (file_list->interrupt_set_list || file_list->interrupt_thumbs)
		return;
	file_list->doing_thumbs = TRUE;
	gth_file_list_update_next_thumb (file_list);
}


static gboolean
add_list_in_chunks (gpointer callback_data)
{
	GetFileInfoData *gfi_data = callback_data;
	GthFileList     *file_list = gfi_data->file_list;
	GList           *scan, *chunk;
	int              i, n = ADD_LIST_CHUNK;

	if (gfi_data->timeout_id != 0) {
		g_source_remove (gfi_data->timeout_id);
		gfi_data->timeout_id = 0;
	}

	if (file_list->interrupt_set_list) {
		DoneFunc  done_func;

		file_list->enable_thumbs = gfi_data->enable_thumbs;

		file_list->interrupt_set_list = FALSE;
		done_func = file_list->interrupt_done_func;
		file_list->interrupt_done_func = NULL;
		if (done_func != NULL)
			(*done_func) (file_list->interrupt_done_data);

		gth_file_view_set_no_image_text (file_list->view, _("No image"));

		get_file_info_data_free (gfi_data);

		return FALSE;
	}

	if (gfi_data->filtered == NULL) {
		DoneFunc  done_func;

		gth_file_view_sorted (file_list->view,
			      	      file_list->sort_method,
			              file_list->sort_type);

		file_list->enable_thumbs = gfi_data->enable_thumbs;

		if ((file_list->list != NULL) && file_list->enable_thumbs)  
			start_update_next_thumb (file_list);

		done_func = gfi_data->done_func;
		gfi_data->done_func = NULL;
		file_list->interrupt_done_func = NULL;
		if (done_func != NULL)
			(*done_func) (gfi_data->done_func_data);

		gth_file_view_set_no_image_text (file_list->view, _("No image"));

		get_file_info_data_free (gfi_data);

		return FALSE;
	}

	if (file_list->enable_thumbs)
		file_list->enable_thumbs = FALSE;

	/**/

	gth_file_view_freeze (file_list->view);
	gth_file_view_unsorted (file_list->view);

	for (i = 0, scan = gfi_data->filtered; (i < n) && scan; i++, scan = scan->next) {
		FileData *fd = scan->data;
		int       pos;

		file_data_update_comment (fd); 
		
		pos = gth_file_view_append_with_data (file_list->view,
						      NULL,
						      fd->utf8_name,
						      fd->comment,
						      fd);
	}

	/*gth_file_view_sorted (file_list->view,
			      file_list->sort_method,
			      file_list->sort_type);*/

	gth_file_view_thaw (file_list->view);

	if ((scan != NULL) && (scan->prev != NULL)) {
		scan->prev->next = NULL;
		scan->prev = NULL;
	}

	chunk = gfi_data->filtered;
	gfi_data->filtered = scan;
	file_list->list = g_list_concat (file_list->list, chunk);
	
	gfi_data->timeout_id = g_timeout_add (ADD_LIST_DELAY, 
					      add_list_in_chunks, 
					      gfi_data);

	return FALSE;
}


static void 
add_list__get_file_info_done_cb (GnomeVFSAsyncHandle *handle,
				 GList *results, /* GnomeVFSGetFileInfoResult* items */
				 gpointer callback_data)
{
	GetFileInfoData *gfi_data = callback_data;
	GthFileList     *file_list = gfi_data->file_list;
	GList           *scan;

	if (file_list->interrupt_set_list) {
		DoneFunc done_func;
	
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

		escaped = gnome_vfs_uri_to_string (info_result->uri, GNOME_VFS_URI_HIDE_NONE);
		full_path = gnome_vfs_unescape_string (escaped, "");
		g_free (escaped);

		fd = file_data_new (full_path, info_result->file_info);
		g_free (full_path);

		gfi_data->filtered = g_list_prepend (gfi_data->filtered, fd);
	}

	add_list_in_chunks (gfi_data);

	return;
}


static void
add_list__step2 (GetFileInfoData *gfi_data)
{
	GnomeVFSAsyncHandle *handle;
	GthFileList         *file_list = gfi_data->file_list;
	GList               *scan;	
	gboolean             fast_file_type;
	
	if (file_list->interrupt_set_list) {
		DoneFunc done_func;

		done_func = file_list->interrupt_done_func;
		file_list->interrupt_done_func = NULL;
		if (done_func != NULL)
			(*done_func) (file_list->interrupt_done_data);
		get_file_info_data_free (gfi_data);
		return;
	}

	fast_file_type = eel_gconf_get_boolean (PREF_FAST_FILE_TYPE, TRUE);
	for (scan = gfi_data->new_list; scan; scan = scan->next) {
		char        *full_path = scan->data;
		const char  *name_only = file_name_from_path (full_path);
		GnomeVFSURI *uri;

		if (gth_file_list_pos_from_path (file_list, full_path) != -1)
			continue;

		if ((! gfi_data->file_list->show_dot_files 
		     && file_is_hidden (name_only))
		    || ! file_is_image (full_path, fast_file_type))
			continue;
		
		uri = new_uri_from_path (full_path);
		if (uri != NULL)
			gfi_data->uri_list = g_list_prepend (gfi_data->uri_list, uri);
	}
	
	if (gfi_data->uri_list == NULL) {
		if (gfi_data->done_func != NULL)
			(*gfi_data->done_func) (gfi_data->done_func_data);
		get_file_info_data_free (gfi_data);
		return;
	}

	gnome_vfs_async_get_file_info (&handle,
				       gfi_data->uri_list,
				       (GNOME_VFS_FILE_INFO_DEFAULT 
					| GNOME_VFS_FILE_INFO_FOLLOW_LINKS),
				       GNOME_VFS_PRIORITY_MAX,
				       add_list__get_file_info_done_cb,
				       gfi_data);	
}


void 
gth_file_list_add_list (GthFileList *file_list,
			GList       *new_list,
			DoneFunc     done_func,
			gpointer     done_func_data)
{
	GetFileInfoData *gfi_data;

	g_return_if_fail (file_list != NULL);

	file_list->interrupt_set_list = FALSE;
	gfi_data = get_file_info_data_new (file_list, 
					   new_list,
					   done_func, 
					   done_func_data);

	if (file_list->doing_thumbs)
		gth_file_list_interrupt_thumbs (file_list, 
						(DoneFunc) add_list__step2, 
						gfi_data);
	else
		add_list__step2 (gfi_data);
}


/* -- InterruptThumbsData -- */


typedef struct {
	GthFileList *file_list;
	gboolean  restart_thumbs;
	int       i_data;
	char     *s_data;
} InterruptThumbsData;


static InterruptThumbsData *
it_data_new (GthFileList   *file_list,
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


static void
set_sort_method__step2 (InterruptThumbsData *it_data)
{
	GthFileList    *file_list = it_data->file_list;
	GthSortMethod   new_method = it_data->i_data;

	file_list->sort_method = new_method;

	gth_file_view_sorted (file_list->view,
			      file_list->sort_method,
			      file_list->sort_type);

	if (it_data->restart_thumbs) 
		start_update_next_thumb (file_list);
	
	it_data_free (it_data);
}


void 
gth_file_list_set_sort_method (GthFileList   *file_list,
			       GthSortMethod  new_method,
			       gboolean       update)
{
	InterruptThumbsData *it_data;

	g_return_if_fail (file_list != NULL);

	if (file_list->sort_method == new_method) 
		return;

	if (! update) {
		file_list->sort_method = new_method;
		return;
	}

	if (file_list->doing_thumbs) {
		it_data = it_data_new (file_list, TRUE, new_method, NULL);
		gth_file_list_interrupt_thumbs (file_list, 
						(DoneFunc) set_sort_method__step2,
						it_data);
	} else {
		it_data = it_data_new (file_list, FALSE, new_method, NULL);
		set_sort_method__step2 (it_data);
	}
}


/* -- -- */


static void
set_sort_type__step2 (InterruptThumbsData *it_data)
{
	GthFileList *file_list = it_data->file_list;
	GtkSortType  sort_type = it_data->i_data;

	file_list->sort_type = sort_type;
	gth_file_view_sorted (file_list->view,
			      file_list->sort_method,
			      file_list->sort_type);

	if (it_data->restart_thumbs) 
		start_update_next_thumb (file_list);
	
	it_data_free (it_data);
}


void
gth_file_list_set_sort_type (GthFileList *file_list,
			     GtkSortType  sort_type,
			     gboolean     update)
{
	InterruptThumbsData *it_data;

	g_return_if_fail (file_list != NULL);

	if (! update) {
		file_list->sort_type = sort_type;
		return;
	}

	if (file_list->doing_thumbs) {
		it_data = it_data_new (file_list, TRUE, sort_type, NULL);
		gth_file_list_interrupt_thumbs (file_list, 
						(DoneFunc) set_sort_type__step2,
						it_data);
	} else {
		it_data = it_data_new (file_list, FALSE, sort_type, NULL);
		set_sort_type__step2 (it_data);
	}
}


/* -- -- */


static void gth_file_list_thumb_cleanup (GthFileList *file_list);


static void
interrupt_thumbs__part2 (GthFileList *file_list)
{
	DoneFunc done_func;

	gth_file_list_thumb_cleanup (file_list);

	done_func = file_list->interrupt_done_func;
	file_list->interrupt_done_func = NULL;
	file_list->interrupt_thumbs = FALSE;

	if (done_func != NULL)
		(*done_func) (file_list->interrupt_done_data);
}


void 
gth_file_list_interrupt_thumbs (GthFileList *file_list, 
				DoneFunc     done_func,
				gpointer     done_func_data)
{
	g_return_if_fail (file_list != NULL);

        if (file_list->doing_thumbs) {
		file_list->interrupt_thumbs = TRUE;
		file_list->interrupt_done_func = done_func;
		file_list->interrupt_done_data = done_func_data;
		file_list->doing_thumbs = FALSE;

	} else if (done_func != NULL)
		(*done_func) (done_func_data);
}


int 
gth_file_list_pos_from_path (GthFileList *file_list, 
			     const char  *path)
{
	GList *list, *scan;
	int    retval = -1, i;

	g_return_val_if_fail (file_list != NULL, -1);

	if (path == NULL)
		return -1;

	i = 0;
	list = gth_file_view_get_list (file_list->view);
	for (scan = list; scan; scan = scan->next) {
		FileData *fd = scan->data;

		if (same_uri (fd->path, path)) {
			retval = i;
			break;
		}

		i++;
	}
	g_list_free (list);

	return retval;
}


GList *
gth_file_list_get_all (GthFileList *file_list)
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


GList *
gth_file_list_get_all_from_view (GthFileList *file_list)
{
	GList *list, *scan, *path_list = NULL;

	g_return_val_if_fail (file_list != NULL, NULL);

	list = gth_file_view_get_list (file_list->view);
	for (scan = list; scan; scan = scan->next) {
		FileData *fd = scan->data;
		path_list = g_list_prepend (path_list, g_strdup (fd->path));
	}
	g_list_free (list);

	return g_list_reverse (path_list);
}


int 
gth_file_list_get_length (GthFileList *file_list)
{
	g_return_val_if_fail (file_list != NULL, 0);
	return g_list_length (file_list->list);
}


GList *
gth_file_list_get_selection (GthFileList *file_list)
{
	return gth_file_view_get_file_list_selection (file_list->view);
}


GList *
gth_file_list_get_selection_as_fd (GthFileList *file_list)
{
	return gth_file_view_get_selection (file_list->view);
}


int 
gth_file_list_get_selection_length (GthFileList *file_list)
{
	GList *sel_list;
	int    len;

	g_return_val_if_fail (file_list != NULL, 0);

	sel_list = gth_file_view_get_selection (file_list->view);
	len = g_list_length (sel_list);
	file_data_list_free (sel_list);

	return len;
}


gchar *
gth_file_list_path_from_pos (GthFileList *file_list,
			     int          pos)
{
	FileData *fd;
	char     *retval = NULL;

	g_return_val_if_fail (file_list != NULL, NULL);

	if ((pos < 0) || (pos >= gth_file_view_get_images (file_list->view)))
		return NULL;

	fd = gth_file_view_get_image_data (file_list->view, pos);
	if ((fd != NULL) && (fd->path != NULL))
		retval = g_strdup (fd->path);
	file_data_unref (fd);

	return retval;
}


gboolean
gth_file_list_is_selected (GthFileList *file_list,
			   int          pos)
{
	return gth_file_view_pos_is_selected (file_list->view, pos);
}


void 
gth_file_list_select_image_by_pos (GthFileList *file_list,
				   int          pos)
{
	GthVisibility  visibility;

	g_return_if_fail (file_list != NULL);

	gth_file_view_unselect_all (file_list->view);
	gth_file_view_select_image (file_list->view, pos);
	gth_file_view_set_cursor (file_list->view, pos);

	visibility = gth_file_view_image_is_visible (file_list->view, pos);
	if (visibility != GTH_VISIBILITY_FULL) {
		double offset = 0.5;
		switch (visibility) {
		case GTH_VISIBILITY_NONE:
			offset = 0.5; 
			break;
		case GTH_VISIBILITY_PARTIAL_TOP:
			offset = 0.0; 
			break;
		case GTH_VISIBILITY_PARTIAL_BOTTOM:
			offset = 1.0; 
			break;
		case GTH_VISIBILITY_PARTIAL:
		case GTH_VISIBILITY_FULL:
			return;
		}
		gth_file_view_moveto (file_list->view, pos, offset);
	}
}


void 
gth_file_list_select_all (GthFileList *file_list)
{
	g_return_if_fail (file_list != NULL);
	gth_file_view_select_all (file_list->view);
}


void 
gth_file_list_unselect_all (GthFileList *file_list)
{
	g_return_if_fail (file_list != NULL);
	gth_file_view_unselect_all (file_list->view);
}


static void gth_file_list_update_thumbs (GthFileList *file_list);


void
gth_file_list_enable_thumbs (GthFileList *file_list,
			     gboolean     enable,
			     gboolean     update)
{
	int i;

	g_return_if_fail (file_list != NULL);

	file_list->enable_thumbs = enable;

	if (!update)
		return;

	gth_file_view_enable_thumbs (file_list->view, file_list->enable_thumbs);

	for (i = 0; i < gth_file_view_get_images (file_list->view); i++)
		gth_file_view_set_unknown_pixbuf (file_list->view, i);

	if (file_list->enable_thumbs)
		gth_file_list_update_thumbs (file_list);
}


void
gth_file_list_set_progress_func (GthFileList *file_list,
				 ProgressFunc func,
				 gpointer     data)
{
	g_return_if_fail (file_list != NULL);

	file_list->progress_func = func;
	file_list->progress_data = data;
}


/**
 * gth_file_list_next_image:
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
int
gth_file_list_next_image (GthFileList *file_list,
			  int          pos,
			  gboolean     without_error,
			  gboolean     only_selected)
{
	int n;

	g_return_val_if_fail (file_list != NULL, -1);

	n = gth_file_view_get_images (file_list->view);

	pos++;
	for (; pos < n; pos++) {
		FileData *fd;

		fd = gth_file_view_get_image_data (file_list->view, pos);

		if (without_error && fd->error) {
			file_data_unref (fd);
			continue;
		}
		file_data_unref (fd);

		if (only_selected && ! gth_file_view_pos_is_selected (file_list->view, pos))
			continue;

		break;
	}

	if (pos >= n)
		return -1;

	return pos;
}


int
gth_file_list_prev_image (GthFileList *file_list,
			  int          pos,
			  gboolean     without_error,
			  gboolean     only_selected)
{
	g_return_val_if_fail (file_list != NULL, -1);

	pos--;
	for (; pos >= 0; pos--) {
		FileData *fd;

		fd = gth_file_view_get_image_data (file_list->view, pos);

		if (without_error && fd->error) {
			file_data_unref (fd);
			continue;
		}
		file_data_unref (fd);

		if (only_selected && ! gth_file_view_pos_is_selected (file_list->view, pos))
			continue;

		break;
	}

	if (pos < 0)
		return -1;

	return pos;
}


/* -- -- */


static void
delete_pos__step2 (InterruptThumbsData *it_data)
{
	GthFileList *file_list = it_data->file_list;
	FileData    *fd;
	int          pos = it_data->i_data;

	fd = gth_file_view_get_image_data (file_list->view, pos);

	g_return_if_fail (fd != NULL);

	file_data_unref (fd);
	file_list->list = g_list_remove (file_list->list, fd);
	file_data_unref (fd);

	gth_file_view_remove (file_list->view, pos);

	if (it_data->restart_thumbs) 
		start_update_next_thumb (file_list);

	it_data_free (it_data);
}


void
gth_file_list_delete_pos (GthFileList *file_list,
			  int          pos)
{
	InterruptThumbsData *it_data;

	g_return_if_fail (file_list != NULL);

	if ((pos < 0) || (pos >= gth_file_view_get_images (file_list->view)))
		return;

	if (file_list->doing_thumbs) {
		it_data = it_data_new (file_list, TRUE, pos, NULL);
		gth_file_list_interrupt_thumbs (file_list, 
						(DoneFunc) delete_pos__step2, 
						it_data);

	} else {
		it_data = it_data_new (file_list, FALSE, pos, NULL);
		delete_pos__step2 (it_data);
	}
}


/* -- -- */


static void
rename_pos__step2 (InterruptThumbsData *it_data)
{
	GthFileList    *file_list = it_data->file_list;
	int             pos = it_data->i_data;
	char           *path = it_data->s_data;
	FileData       *fd;

	/* update the FileData structure. */
	fd = gth_file_view_get_image_data (file_list->view, pos);
	file_data_set_path (fd, path);

	/* Set the new name. */
	gth_file_view_set_image_text (file_list->view, pos, fd->utf8_name);
	file_data_unref (fd);

	gth_file_view_sorted (file_list->view,
			      file_list->sort_method,
			      file_list->sort_type); 

	if (it_data->restart_thumbs)
		start_update_next_thumb (file_list);

	it_data_free (it_data);
}


void
gth_file_list_rename_pos (GthFileList *file_list,
			  int          pos, 
			  const char  *path)
{
	InterruptThumbsData *it_data;

	g_return_if_fail (file_list != NULL);

	if ((pos < 0) || (pos >= gth_file_view_get_images (file_list->view)))
		return;

	if (file_list->doing_thumbs) {
		it_data = it_data_new (file_list, TRUE, pos, path);
		gth_file_list_interrupt_thumbs (file_list, 
						(DoneFunc) rename_pos__step2, 
						it_data);
	} else {
		it_data = it_data_new (file_list, FALSE, pos, path);
		rename_pos__step2 (it_data);
	}
}


/* -- -- */


static void
update_comment__step2 (InterruptThumbsData *it_data)
{
	GthFileList   *file_list = it_data->file_list;
	int            pos = it_data->i_data;
	FileData      *fd;

	/* update the FileData structure. */
	fd = gth_file_view_get_image_data (file_list->view, pos);
	file_data_update_comment (fd);

	/* Set the new name. */
	gth_file_view_set_image_comment (file_list->view, pos, fd->comment);
	file_data_unref (fd);

	if (it_data->restart_thumbs)
		start_update_next_thumb (file_list);

	it_data_free (it_data);
}


void
gth_file_list_update_comment (GthFileList *file_list,
			      int          pos)
{
	InterruptThumbsData *it_data;

	g_return_if_fail (file_list != NULL);
	
	if ((pos < 0) || (pos >= gth_file_view_get_images (file_list->view)))
		return;

	if (file_list->doing_thumbs) {
		it_data = it_data_new (file_list, TRUE, pos, NULL);
		gth_file_list_interrupt_thumbs (file_list, 
						(DoneFunc) update_comment__step2, 
						it_data);
	} else {
		it_data = it_data_new (file_list, FALSE, pos, NULL);
		update_comment__step2 (it_data);
	}
}


/* -- -- */


static void
set_thumbs_size__step2 (InterruptThumbsData *it_data)
{
	GthFileList *file_list = it_data->file_list;
	int          size = it_data->i_data;

	file_list->thumb_size = size;

	g_object_unref (G_OBJECT (file_list->thumb_loader));
	file_list->thumb_loader = THUMB_LOADER (thumb_loader_new (NULL, size, size));
	g_signal_connect (G_OBJECT (file_list->thumb_loader), 
			  "thumb_done",
			  G_CALLBACK (load_thumb_done_cb), 
			  file_list);
	g_signal_connect (G_OBJECT (file_list->thumb_loader), 
			  "thumb_error",
			  G_CALLBACK (load_thumb_error_cb), 
			  file_list);

	gth_file_view_set_image_width (file_list->view, size + THUMB_BORDER);
	it_data_free (it_data);

	gth_file_list_update_thumbs (file_list);
}


void
gth_file_list_set_thumbs_size (GthFileList *file_list,
			       int          size)
{
	InterruptThumbsData *it_data;

	g_return_if_fail (file_list != NULL);
	if (file_list->thumb_size == size) return;

	if (file_list->doing_thumbs) {
		it_data = it_data_new (file_list, TRUE, size, NULL);
		gth_file_list_interrupt_thumbs (file_list, 
						(DoneFunc) set_thumbs_size__step2, 
						it_data);
	} else {
		it_data = it_data_new (file_list, FALSE, size, NULL);
		set_thumbs_size__step2 (it_data);
	}
}


/* -- -- */


static void
gth_file_list_update_current_thumb (GthFileList *file_list)
{
	gboolean  error = TRUE;
	char     *path;

	if (! file_list->doing_thumbs) {
		interrupt_thumbs__part2 (file_list);
		return;
	}

	g_return_if_fail (file_list->thumb_fd != NULL);

	path = g_strdup (file_list->thumb_fd->path);

	if (path_is_file (path)) {
		GnomeVFSResult  result;
		char           *resolved_path = NULL;

		result = resolve_all_symlinks (path, &resolved_path);

		if (result == GNOME_VFS_OK) {
			if (resolved_path != NULL) {
				thumb_loader_set_path (file_list->thumb_loader, resolved_path);
				thumb_loader_start (file_list->thumb_loader);
				error = FALSE;
			} 
		} else
			g_warning ("%s\n", gnome_vfs_result_to_string (result));

		g_free (resolved_path);
	}
		
	g_free (path);

	if (error) /* Error: the file does not exists. */
		g_signal_emit_by_name (G_OBJECT (file_list->thumb_loader),
				       "thumb_error",
				       0,
				       file_list);
}


void
gth_file_list_update_thumb (GthFileList  *file_list,
			    int           pos)
{
	FileData  *fd;

	if (! file_list->enable_thumbs)
		return;

	fd = gth_file_view_get_image_data (file_list->view, pos);

	file_data_update (fd);
	fd->error = FALSE;
	fd->thumb = FALSE;

	file_list->thumb_pos = pos;
	if (file_list->thumb_fd != NULL)
		file_data_unref (file_list->thumb_fd);
	file_list->thumb_fd = fd;

	gth_file_list_update_current_thumb (file_list);
}


void
gth_file_list_update_thumb_list (GthFileList  *file_list,
				 GList        *list)
{
	GList *scan;

	if (! file_list->enable_thumbs)
		return;

	for (scan = list; scan; scan = scan->next) {
		char      *path = scan->data;
		FileData  *fd;
		int        pos;

		pos = gth_file_list_pos_from_path (file_list, path);

		if (pos == -1)
			continue;

		fd = gth_file_view_get_image_data (file_list->view, pos);
		file_data_update (fd);
		fd->error = FALSE;
		fd->thumb = FALSE;
		file_data_unref (fd);
	}
	
	start_update_next_thumb (file_list);
}



/* -- thumbs update -- */


static void 
gth_file_list_thumb_cleanup (GthFileList *file_list)
{
	file_list->thumbs_num = 0;
	file_list->doing_thumbs = FALSE;

	if (file_list->thumb_fd != NULL) {
		file_data_unref (file_list->thumb_fd);
		file_list->thumb_fd = NULL;
	}

	if (file_list->progress_func) 
		file_list->progress_func (0.0, file_list->progress_data);
}


static void
gth_file_list_update_next_thumb (GthFileList *file_list)
{
	FileData *fd = NULL;
	int       first_pos, last_pos;
	int       pos = -1;
	GList    *list, *scan;
	int       new_pos = -1;

	if (file_list->interrupt_thumbs) {
		interrupt_thumbs__part2 (file_list);
		return;
	}

	/* Find first visible undone. */

	first_pos = gth_file_view_get_first_visible (file_list->view);
	last_pos = gth_file_view_get_last_visible (file_list->view);

	pos = first_pos;

	if ((pos == -1) || (last_pos < first_pos)) {
		gth_file_list_thumb_cleanup (file_list);
		return;
	}

	list = gth_file_view_get_list (file_list->view);
	scan = g_list_nth (list, pos);
	while (pos <= last_pos) {
		fd = scan->data;
		if (! fd->thumb && ! fd->error) {
			new_pos = pos;
			break;
		} else {
			pos++;
			scan = scan->next;
		}
	}
	g_list_free (list);

	if (new_pos == -1) {
		gth_file_list_thumb_cleanup (file_list);
		return;
	}

	g_assert (fd != NULL);

	file_list->thumb_pos = new_pos;
	file_list->thumbs_num++;

	if (file_list->thumb_fd != NULL)
		file_data_unref (file_list->thumb_fd);
	file_list->thumb_fd = fd;
	file_data_ref (file_list->thumb_fd);

	gth_file_list_update_current_thumb (file_list);
}


static void 
gth_file_list_update_thumbs (GthFileList *file_list)
{
	int i;
	GList *scan;

	if (! file_list->enable_thumbs || file_list->interrupt_thumbs)
		return;

	for (i = 0; i < gth_file_view_get_images (file_list->view); i++)
		gth_file_view_set_unknown_pixbuf (file_list->view, i);

	thumb_loader_set_max_file_size (THUMB_LOADER (file_list->thumb_loader), eel_gconf_get_integer (PREF_THUMBNAIL_LIMIT, 0));

	for (scan = file_list->list; scan; scan = scan->next) {
		FileData *fd = scan->data;
		fd->thumb = FALSE;
		fd->error = FALSE;
	}
		
	start_update_next_thumb (file_list);
}


void 
gth_file_list_restart_thumbs (GthFileList *file_list,
			      gboolean    _continue)
{
	if (! file_list->enable_thumbs)
		return;

	if (_continue) 
		start_update_next_thumb (file_list);
	else
		gth_file_list_update_thumbs (file_list);	
}
