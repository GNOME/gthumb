/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001-2006 Free Software Foundation, Inc.
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

#define THUMB_BORDER        14
#define ADD_LIST_DELAY      30
#define ADD_LIST_CHUNK_SIZE 500
#define SCROLL_DELAY        20
#define DEF_THUMB_SIZE      128


typedef enum {
	GTH_FILE_LIST_OP_TYPE_RENAME,
	GTH_FILE_LIST_OP_TYPE_UPDATE_COMMENT,
	GTH_FILE_LIST_OP_TYPE_UPDATE_THUMB,
	GTH_FILE_LIST_OP_TYPE_UPDATE_THUMB_LIST,
	GTH_FILE_LIST_OP_TYPE_SET_THUMBS_SIZE,
	GTH_FILE_LIST_OP_TYPE_ADD_LIST,
	GTH_FILE_LIST_OP_TYPE_SET_LIST,
	GTH_FILE_LIST_OP_TYPE_DELETE_LIST,
	GTH_FILE_LIST_OP_TYPE_EMPTY_LIST,
	GTH_FILE_LIST_OP_TYPE_ENABLE_THUMBS,
	GTH_FILE_LIST_OP_TYPE_SET_FILTER
} GthFileListOpType;


typedef struct {
	GthFileListOpType type;
	union {
		GList *uri_list;
		char  *uri;
		int    ival;
	} file;
} GthFileListOp;


struct _GthFileListPrivateData {
	GList         *new_list;

	GthSortMethod  sort_method;         /* How to sort the list. */
	GtkSortType    sort_type;           /* ascending or discending sort. */

	GthFileView   *view;                /* The view that contains the
			 	             * file list. */
	GthFilter     *filter;

	gboolean       show_dot_files;      /* Whether to show files that starts
					     * with a dot (hidden files).*/
	gboolean       load_thumbs;

	int            thumb_size;          /* Thumbnails max size. */

	/* -- thumbs update data -- */

	ThumbLoader   *thumb_loader;
	int            thumbs_num;
	FileData      *thumb_fd;
	int            thumb_pos;           /* The position of the item we are
					     * genereting a thumbnail. */
	guint          scroll_timer;

	/**/

	gboolean       stop_it;
	gboolean       loading_thumbs;
	GList         *queue; /* list of GthFileListOp */
};


enum {
	BUSY,
	IDLE,
	DONE,
	LAST_SIGNAL
};


static GObjectClass *parent_class = NULL;
static guint         gth_file_list_signals[LAST_SIGNAL] = { 0 };


/* OPs */


static void gth_file_list_exec_next_op (GthFileList *file_list);


static GthFileListOp *
gth_file_list_op_new (op_type)
{
	GthFileListOp *op;

	op = g_new0 (GthFileListOp, 1);
	op->type = op_type;

	return op;
}


static void
gth_file_list_op_free (GthFileListOp *op)
{
	switch (op->type) {
	case GTH_FILE_LIST_OP_TYPE_UPDATE_COMMENT:
	case GTH_FILE_LIST_OP_TYPE_UPDATE_THUMB:
		g_free (op->file.uri);
		break;
	case GTH_FILE_LIST_OP_TYPE_RENAME:
	case GTH_FILE_LIST_OP_TYPE_UPDATE_THUMB_LIST:
	case GTH_FILE_LIST_OP_TYPE_ADD_LIST:
	case GTH_FILE_LIST_OP_TYPE_SET_LIST:
		path_list_free (op->file.uri_list);
		break;
	default:
		break;
	}
	g_free (op);
}


static void
gth_file_list_clear_queue (GthFileList *file_list)
{
	g_list_foreach (file_list->priv->queue, (GFunc) gth_file_list_op_free, NULL);
	g_list_free (file_list->priv->queue);
	file_list->priv->queue = NULL;
}


static void
gth_file_list_remove_op (GthFileList       *file_list,
			 GthFileListOpType  op_type)
{
	GList *scan;

	scan = file_list->priv->queue;
	while (scan != NULL) {
		GthFileListOp *op = scan->data;

		if (op->type != op_type) {
			scan = scan->next;
			continue;
		}

		file_list->priv->queue =  g_list_remove_link (file_list->priv->queue, scan);
		gth_file_list_op_free (op);
		g_list_free (scan);

		scan = file_list->priv->queue;
	}
}


static void
gth_file_list_queue_op (GthFileList   *file_list,
			GthFileListOp *op)
{
	if (op->type == GTH_FILE_LIST_OP_TYPE_SET_LIST)
		gth_file_list_clear_queue (file_list);
	if (op->type == GTH_FILE_LIST_OP_TYPE_SET_FILTER)
		gth_file_list_remove_op (file_list, GTH_FILE_LIST_OP_TYPE_SET_FILTER);
	file_list->priv->queue = g_list_append (file_list->priv->queue, op);
	if (! file_list->busy)
		gth_file_list_exec_next_op (file_list);
}


static void
gth_file_list_queue_op_with_uri (GthFileList       *file_list,
			         GthFileListOpType  op_type,
			         const char        *uri)
{
	GthFileListOp *op;

	op = gth_file_list_op_new (op_type);
	op->file.uri = g_strdup (uri);

	gth_file_list_queue_op (file_list, op);
}


static void
gth_file_list_queue_op_with_list (GthFileList       *file_list,
				  GthFileListOpType  op_type,
				  GList             *uri_list)
{
	GthFileListOp *op;

	op = gth_file_list_op_new (op_type);
	op->file.uri_list = path_list_dup (uri_list);

	gth_file_list_queue_op (file_list, op);
}


static void
gth_file_list_queue_op_with_2_uris (GthFileList       *file_list,
			            GthFileListOpType  op_type,
			            const char        *uri1,
			            const char        *uri2)
{
	GList *uri_list = NULL;

	uri_list = g_list_append (uri_list, g_strdup (uri1));
	uri_list = g_list_append (uri_list, g_strdup (uri2));
	gth_file_list_queue_op_with_list (file_list, op_type, uri_list);
}


static void
gth_file_list_queue_op_with_int (GthFileList       *file_list,
			         GthFileListOpType  op_type,
			         int                value)
{
	GthFileListOp *op;

	op = gth_file_list_op_new (op_type);
	op->file.ival = value;
	gth_file_list_queue_op (file_list, op);
}


/* -- thumbs update -- */


static void
gth_file_list_thumb_cleanup (GthFileList *file_list)
{
	file_list->priv->thumbs_num = 0;
	if (file_list->priv->thumb_fd != NULL) {
		file_data_unref (file_list->priv->thumb_fd);
		file_list->priv->thumb_fd = NULL;
	}
}


static gboolean
update_thumbs_stopped (gpointer callback_data)
{
	GthFileList *file_list = callback_data;

	file_list->priv->loading_thumbs = FALSE;
	gth_file_list_exec_next_op (file_list);

	return FALSE;
}


static void
gth_file_list_update_current_thumb (GthFileList *file_list)
{
	gboolean  error = TRUE;
	char     *path;

	if (file_list->priv->stop_it || (file_list->priv->queue != NULL)) {
		g_idle_add (update_thumbs_stopped, file_list);
		return;
	}

	g_return_if_fail (file_list->priv->thumb_fd != NULL);

	file_list->priv->loading_thumbs = TRUE;

	path = g_strdup (file_list->priv->thumb_fd->path);
	if (path_is_file (path)) {
		GnomeVFSResult  result;
		char           *resolved_path = NULL;

		result = resolve_all_symlinks (path, &resolved_path);

		if (result == GNOME_VFS_OK) {
			if (resolved_path != NULL) {
				thumb_loader_set_path (file_list->priv->thumb_loader, resolved_path, file_list->priv->thumb_fd->mime_type);
				thumb_loader_start (file_list->priv->thumb_loader);
				error = FALSE;
			}
		} else
			g_warning ("%s\n", gnome_vfs_result_to_string (result));

		g_free (resolved_path);
	}
	g_free (path);

	if (error) /* Error: the file does not exists. */
		g_signal_emit_by_name (G_OBJECT (file_list->priv->thumb_loader),
				       "thumb_error",
				       0,
				       file_list);
}


static void
gth_file_list_done (GthFileList *file_list)
{
	gth_file_list_thumb_cleanup (file_list);
	file_list->busy = FALSE;
	file_list->priv->stop_it = FALSE;
	file_list->priv->loading_thumbs = FALSE;
}


static void
gth_file_list_update_next_thumb (GthFileList *file_list)
{
	FileData *fd = NULL;
	int       first_pos, last_pos;
	int       pos = -1;
	GList    *list, *scan;
	int       new_pos = -1;

	if (file_list->priv->stop_it || (file_list->priv->queue != NULL)) {
		g_idle_add (update_thumbs_stopped, file_list);
		return;
	}

	/* Find first visible undone. */

	first_pos = gth_file_view_get_first_visible (file_list->view);
	last_pos = gth_file_view_get_last_visible (file_list->view);

	pos = first_pos;
	if ((pos == -1) || (last_pos < first_pos)) {
		gth_file_list_done (file_list);
		return;
	}

	list = gth_file_view_get_list (file_list->view);
	scan = g_list_nth (list, pos);
	if (scan == NULL) {
		gth_file_list_done (file_list);
		return;
	}

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
		gth_file_list_done (file_list);
		return;
	}

	g_assert (fd != NULL);

	file_list->priv->thumb_pos = new_pos;
	file_list->priv->thumbs_num++;

	if (file_list->priv->thumb_fd != NULL)
		file_data_unref (file_list->priv->thumb_fd);
	file_list->priv->thumb_fd = fd;
	file_data_ref (file_list->priv->thumb_fd);

	gth_file_list_update_current_thumb (file_list);
}


static void
start_update_next_thumb (GthFileList *file_list)
{
	if (! file_list->priv->load_thumbs)
		return;
	file_list->busy = TRUE;
	gth_file_list_update_next_thumb (file_list);
}


static void
gth_file_list_update_thumbs (GthFileList *file_list)
{
	int    i;
	GList *scan;

	for (i = 0; i < gth_file_view_get_images (file_list->view); i++)
		gth_file_view_set_unknown_pixbuf (file_list->view, i);

	thumb_loader_set_max_file_size (THUMB_LOADER (file_list->priv->thumb_loader), eel_gconf_get_integer (PREF_THUMBNAIL_LIMIT, 0));

	for (scan = file_list->list; scan; scan = scan->next) {
		FileData *fd = scan->data;
		fd->thumb = FALSE;
		fd->error = FALSE;
	}

	file_list->priv->load_thumbs = file_list->enable_thumbs;
	start_update_next_thumb (file_list);
}


void
gfl_enable_thumbs (GthFileList *file_list)
{
	int i;

	gth_file_view_enable_thumbs (file_list->view, file_list->enable_thumbs);

	for (i = 0; i < gth_file_view_get_images (file_list->view); i++)
		gth_file_view_set_unknown_pixbuf (file_list->view, i);

	if (file_list->enable_thumbs)
		gth_file_list_update_thumbs (file_list);
}


void
gth_file_list_enable_thumbs (GthFileList *file_list,
			     gboolean     enable,
			     gboolean     update)
{
	file_list->enable_thumbs = enable;
	if (!update)
		return;
	gth_file_list_queue_op (file_list, gth_file_list_op_new (GTH_FILE_LIST_OP_TYPE_ENABLE_THUMBS));
}


/**/


static void
update_thumb_in_clist (GthFileList *file_list)
{
	GdkPixbuf *pixbuf;

	pixbuf = thumb_loader_get_pixbuf (file_list->priv->thumb_loader);

	if (pixbuf == NULL)
		return;

	gth_file_view_set_image_pixbuf (file_list->view,
					file_list->priv->thumb_pos,
					pixbuf);
}


static void
load_thumb_error_cb (ThumbLoader *tl,
		     gpointer     data)
{
	GthFileList *file_list = data;

	if (file_list->priv->stop_it || (file_list->priv->queue != NULL)) {
		g_idle_add (update_thumbs_stopped, file_list);
		return;
	}

	gth_file_view_set_unknown_pixbuf (file_list->view,  file_list->priv->thumb_pos);

	file_list->priv->thumb_fd->error = TRUE;
	file_list->priv->thumb_fd->thumb = FALSE;

	gth_file_list_update_next_thumb (file_list);
}


static void
load_thumb_done_cb (ThumbLoader *tl,
		    gpointer     data)
{
	GthFileList *file_list = data;

	if (file_list->priv->stop_it || (file_list->priv->queue != NULL)) {
		g_idle_add (update_thumbs_stopped, file_list);
		return;
	}

	update_thumb_in_clist (file_list);
	file_list->priv->thumb_fd->error = FALSE;
	file_list->priv->thumb_fd->thumb = TRUE;

	gth_file_list_update_next_thumb (file_list);
}


static void
gth_file_list_free_list (GthFileList *file_list)
{
	g_return_if_fail (file_list != NULL);

	path_list_free (file_list->priv->new_list);
	file_list->priv->new_list = NULL;

	file_data_list_free (file_list->list);
	file_list->list = NULL;
}


static void
gth_file_list_finalize (GObject *object)
{
	GthFileList *file_list;

        g_return_if_fail (GTH_IS_FILE_LIST (object));
	file_list = GTH_FILE_LIST (object);

	if (file_list->priv->thumb_fd != NULL) {
		file_data_unref (file_list->priv->thumb_fd);
		file_list->priv->thumb_fd = NULL;
	}

	gth_file_list_free_list (file_list);
	g_object_unref (file_list->priv->thumb_loader);
	if (file_list->priv->filter != NULL)
		g_object_unref (file_list->priv->filter);

	g_free (file_list->priv);

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
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
	gth_file_list_signals[IDLE] =
		g_signal_new ("idle",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthFileListClass, idle),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
	gth_file_list_signals[DONE] =
		g_signal_new ("done",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthFileListClass, done),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);

	object_class = G_OBJECT_CLASS (class);
	object_class->finalize = gth_file_list_finalize;
}


static gboolean
update_thumbnails_cb (gpointer data)
{
	GthFileList *file_list = data;

	if (file_list->priv->scroll_timer != 0)
		g_source_remove (file_list->priv->scroll_timer);
	file_list->priv->scroll_timer = 0;

	gth_file_list_restart_thumbs (file_list, TRUE);

	return FALSE;
}


static gboolean
file_list_adj_value_changed (GtkAdjustment *adj,
			     GthFileList   *file_list)
{
	if (gth_file_view_is_frozen (file_list->view))
		return FALSE;

	if (file_list->busy)
		return FALSE;

	if (file_list->priv->scroll_timer != 0) {
		g_source_remove (file_list->priv->scroll_timer);
		file_list->priv->scroll_timer = 0;
	}

	file_list->priv->scroll_timer = g_timeout_add (SCROLL_DELAY,
						 update_thumbnails_cb,
						 file_list);

	return FALSE;
}


static void
gth_file_list_init (GthFileList *file_list)
{
	GtkAdjustment *adj;
	GtkWidget     *scrolled_window;

	/* set the default values. */

	file_list->priv = g_new0 (GthFileListPrivateData, 1);

	file_list->priv->new_list       = NULL;
	file_list->list                 = NULL;
	file_list->priv->sort_method    = pref_get_arrange_type ();
	file_list->priv->sort_type      = pref_get_sort_order ();
	file_list->priv->show_dot_files = eel_gconf_get_boolean (PREF_SHOW_HIDDEN_FILES, FALSE);
	file_list->enable_thumbs        = eel_gconf_get_boolean (PREF_SHOW_THUMBNAILS, TRUE);
	file_list->priv->thumb_size     = eel_gconf_get_integer (PREF_THUMBNAIL_SIZE, DEF_THUMB_SIZE);
	file_list->priv->thumb_loader   = THUMB_LOADER (thumb_loader_new (NULL, file_list->priv->thumb_size, file_list->priv->thumb_size));
	file_list->priv->thumbs_num     = 0;
	file_list->priv->thumb_fd       = NULL;
	file_list->priv->thumb_pos      = -1;
	file_list->priv->scroll_timer   = 0;
	file_list->busy                 = FALSE;
	file_list->priv->stop_it        = FALSE;
	file_list->priv->loading_thumbs = FALSE;
	file_list->priv->filter         = NULL;

	g_signal_connect (G_OBJECT (file_list->priv->thumb_loader),
			  "thumb_done",
			  G_CALLBACK (load_thumb_done_cb),
			  file_list);
	g_signal_connect (G_OBJECT (file_list->priv->thumb_loader),
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
	gth_file_view_set_image_width (file_list->view, file_list->priv->thumb_size + THUMB_BORDER);
	gth_file_view_sorted (file_list->view, file_list->priv->sort_method, file_list->priv->sort_type);
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
gth_file_list_new (void)
{
	return GTH_FILE_LIST (g_object_new (GTH_TYPE_FILE_LIST, NULL));
}


GtkWidget *
gth_file_list_get_widget (GthFileList *file_list)
{
	return file_list->root_widget;
}


GthFileView*
gth_file_list_get_view (GthFileList *file_list)
{
	return file_list->view;
}


/* -- GetFileInfoData -- */


typedef struct {
	GthFileList *file_list;
	GList       *new_list;
	GList       *filtered;
	GList       *uri_list;
	GHashTable  *mime_types;
	guint        timeout_id;
	gboolean     doing_thumbs;
	gboolean     destroyed;
} GetFileInfoData;


static GetFileInfoData *
get_file_info_data_new (GthFileList *file_list)
{
	GetFileInfoData * data;

	data = g_new0 (GetFileInfoData, 1);
	data->file_list = file_list;
	data->new_list = file_list->priv->new_list;
	file_list->priv->new_list = NULL;
	data->filtered = NULL;
	data->uri_list = NULL;
	data->mime_types = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, NULL);
	data->timeout_id = 0;
	data->destroyed = FALSE;

	return data;
}


static void
get_file_info_data_free (GetFileInfoData *data)
{
	if (data == NULL)
		return;

	g_hash_table_destroy (data->mime_types);

	if (data->uri_list != NULL) {
		g_list_foreach (data->uri_list,
				(GFunc) gnome_vfs_uri_unref,
				NULL);
		g_list_free (data->uri_list);
	}

	path_list_free (data->new_list);
	file_data_list_free (data->filtered);

	g_free (data);
}


/* -- add_list_in_chunks -- */


static gboolean
add_list_in_chunks_stopped (gpointer callback_data)
{
	GetFileInfoData *gfi_data = callback_data;
	GthFileList     *file_list = gfi_data->file_list;

	get_file_info_data_free (gfi_data);
	gth_file_list_exec_next_op (file_list);

	return FALSE;
}


static void
add_list_done (GthFileList *file_list)
{
	file_list->priv->load_thumbs = file_list->enable_thumbs;

	gth_file_view_freeze (file_list->view);
	gth_file_view_sorted (file_list->view,
		      	      file_list->priv->sort_method,
		              file_list->priv->sort_type);
	gth_file_view_thaw (file_list->view);

	g_signal_emit (file_list, gth_file_list_signals[DONE], 0);

	gth_file_list_done (file_list);
	gth_file_list_exec_next_op (file_list);
}


static gboolean
add_list_in_chunks (gpointer callback_data)
{
	GetFileInfoData *gfi_data = callback_data;
	GthFileList     *file_list;
	GList           *scan, *chunk;
	int              i;

	if ((gfi_data == NULL) || gfi_data->destroyed)
		return FALSE;

	file_list = gfi_data->file_list;

	if (gfi_data->timeout_id != 0) {
		g_source_remove (gfi_data->timeout_id);
		gfi_data->timeout_id = 0;
	}

	if (file_list->priv->stop_it) {
		gfi_data->destroyed = TRUE;
		file_list->priv->load_thumbs = file_list->enable_thumbs;
		g_idle_add (add_list_in_chunks_stopped, gfi_data);
		return FALSE;
	}

	if (gfi_data->filtered == NULL) {
		gfi_data->destroyed = TRUE;
		get_file_info_data_free (gfi_data);
		add_list_done (file_list);
		return FALSE;
	}

	file_list->priv->load_thumbs = FALSE;

	/**/

	gth_file_view_freeze (file_list->view);
	gth_file_view_unsorted (file_list->view);

	for (i = 0, scan = gfi_data->filtered; (i < ADD_LIST_CHUNK_SIZE) && scan; i++, scan = scan->next) {
		FileData *fd = scan->data;

		file_data_update_comment (fd);
		gth_file_view_append_with_data (file_list->view,
				      		NULL,
						fd->display_name,
						fd->comment,
						fd);
	}

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


/* -- gth_file_list_set_list -- */


static gboolean
set_list_stopped (gpointer callback_data)
{
	GetFileInfoData *gfi_data = callback_data;
	GthFileList     *file_list = gfi_data->file_list;

	get_file_info_data_free (gfi_data);
	gth_file_list_exec_next_op (file_list);

	return FALSE;
}


static void
set_list__get_file_info_done_cb (GnomeVFSAsyncHandle *handle,
				 GList               *results, /* GnomeVFSGetFileInfoResult* items */
				 gpointer             callback_data)
{
	GetFileInfoData *gfi_data = callback_data;
	GthFileList     *file_list;
	GList           *scan;

	if ((gfi_data == NULL) || gfi_data->destroyed)
		return;

	file_list = gfi_data->file_list;
	g_signal_emit (G_OBJECT (file_list), gth_file_list_signals[IDLE], 0);

	if (file_list->priv->stop_it) {
		gfi_data->destroyed = TRUE;
		g_idle_add (set_list_stopped, gfi_data);
		return;
	}

	for (scan = results; scan; scan = scan->next) {
		GnomeVFSGetFileInfoResult *info_result = scan->data;
		char                      *uri;
		FileData                  *fd;

		if ((info_result->result != GNOME_VFS_OK) || (info_result->uri == NULL))
			continue;

		uri = gnome_vfs_uri_to_string (info_result->uri, GNOME_VFS_URI_HIDE_NONE);

		fd = file_data_new (uri, info_result->file_info);
		fd->mime_type = g_hash_table_lookup (gfi_data->mime_types, uri);
		gfi_data->filtered = g_list_prepend (gfi_data->filtered, fd);

		g_free (uri);
	}

	add_list_in_chunks (gfi_data);
}


static void
load_new_list (GthFileList *file_list)
{
	GetFileInfoData     *gfi_data;
	GnomeVFSAsyncHandle *handle;
	GList               *scan;
	gboolean             fast_file_type;

	if (file_list->priv->new_list == NULL) {
		add_list_done (file_list);
		return;
	}

	gfi_data = get_file_info_data_new (file_list);

	file_list->busy = TRUE;
	file_list->priv->stop_it = FALSE;

	g_signal_emit (G_OBJECT (file_list), gth_file_list_signals[BUSY], 0);

	fast_file_type = eel_gconf_get_boolean (PREF_FAST_FILE_TYPE, TRUE);
	for (scan = gfi_data->new_list; scan; scan = scan->next) {
		char        *full_path = scan->data;
		const char  *name_only = file_name_from_path (full_path);
		const char  *mime_type;
		GnomeVFSURI *uri;
		FileData    *fd;

		mime_type = get_file_mime_type (full_path, fast_file_type);
		if (mime_type == NULL)
			continue;

		if ((! file_list->priv->show_dot_files && file_is_hidden (name_only))
		    || ! (mime_type_is_image (mime_type) || mime_type_is_video (mime_type)))
			continue;

		/* if the image is already present invalidate the thumbnail
		 * instead of adding the image twice. */

		fd = gth_file_list_filedata_from_path (file_list, full_path);
		if (fd != NULL) {
			fd->mime_type = mime_type;
			fd->error = FALSE;
			fd->thumb = FALSE;
			continue;
		}

		uri = new_uri_from_path (full_path);
		if (uri != NULL) {
			gfi_data->uri_list = g_list_prepend (gfi_data->uri_list, uri);
			g_hash_table_insert (gfi_data->mime_types,
					     full_path,
					     (gpointer) mime_type);
		}
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
gfl_set_list (GthFileList *file_list,
	      GList       *new_list)
{
	if (file_list->priv->filter != NULL)
		gth_filter_reset (file_list->priv->filter);

	gth_file_view_clear (file_list->view);
	gth_file_list_free_list (file_list);

	if (file_list->priv->new_list != new_list)
		path_list_free (file_list->priv->new_list);
	file_list->priv->new_list = new_list;

	load_new_list (file_list);
}


void
gth_file_list_set_list (GthFileList   *file_list,
			GList         *new_list,
			GthSortMethod  sort_method,
			GtkSortType    sort_type)
{
	file_list->priv->sort_method = sort_method;
	file_list->priv->sort_type = sort_type;
	gth_file_list_queue_op_with_list (file_list,
					  GTH_FILE_LIST_OP_TYPE_SET_LIST,
					  new_list);
}


void
gfl_empty_list (GthFileList *file_list)
{
	gth_file_view_clear (file_list->view);
	gth_file_list_free_list (file_list);

	if (file_list->priv->new_list != NULL) {
		path_list_free (file_list->priv->new_list);
		file_list->priv->new_list = NULL;
	}
}


void
gth_file_list_set_empty_list (GthFileList *file_list)
{
	gth_file_list_queue_op (file_list,
				gth_file_list_op_new (GTH_FILE_LIST_OP_TYPE_EMPTY_LIST));
}


/* -- gth_file_list_add_list -- */


void
gfl_add_list (GthFileList *file_list,
	      GList       *new_list)
{
	file_list->priv->new_list = g_list_concat (file_list->priv->new_list, new_list);
	load_new_list (file_list);
}


void
gth_file_list_add_list (GthFileList *file_list,
			GList       *new_list)
{
	gth_file_list_queue_op_with_list (file_list,
					  GTH_FILE_LIST_OP_TYPE_ADD_LIST,
					  new_list);
}


void
gth_file_list_stop (GthFileList *file_list)
{
	path_list_free (file_list->priv->new_list);
	file_list->priv->new_list = NULL;
	gth_file_list_clear_queue (file_list);
	if (file_list->busy)
		file_list->priv->stop_it = TRUE;
	else
		gth_file_list_done (file_list);
}


void
gth_file_list_set_sort_method (GthFileList   *file_list,
			       GthSortMethod  new_method,
			       gboolean       update)
{
	g_return_if_fail (file_list != NULL);

	if (file_list->priv->sort_method == new_method)
		return;

	file_list->priv->sort_method = new_method;
	if (! update)
		return;

	gth_file_view_sorted (file_list->view,
			      file_list->priv->sort_method,
			      file_list->priv->sort_type);
}


void
gth_file_list_set_sort_type (GthFileList *file_list,
			     GtkSortType  sort_type,
			     gboolean     update)
{
	g_return_if_fail (file_list != NULL);

	if (file_list->priv->sort_type == sort_type)
		return;

	file_list->priv->sort_type = sort_type;
	if (! update)
		return;

	gth_file_view_sorted (file_list->view,
			      file_list->priv->sort_method,
			      file_list->priv->sort_type);
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


FileData *
gth_file_list_filedata_from_path (GthFileList *file_list,
			          const char  *path)
{
	FileData *result = NULL;
	GList    *list, *scan;

	g_return_val_if_fail (file_list != NULL, NULL);

	if (path == NULL)
		return NULL;

	list = gth_file_view_get_list (file_list->view);
	for (scan = list; scan; scan = scan->next) {
		FileData *fd = scan->data;
		if (same_uri (fd->path, path)) {
			result = fd;
			break;
		}
	}
	g_list_free (list);

	return result;
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


static void
gfl_delete (GthFileList *file_list,
            const char  *uri)
{
	FileData *fd;

	fd = gth_file_list_filedata_from_path (file_list, uri);
	if (fd == NULL)
		return;

	gth_file_view_remove (file_list->view, fd);
}


static void
gfl_delete_list (GthFileList *file_list,
                 GList       *uri_list)
{
	GList *scan;

	gth_file_view_freeze (file_list->view);
	for (scan = uri_list; scan; scan = scan->next)
		gfl_delete (file_list, scan->data);
	gth_file_view_thaw (file_list->view);

	g_signal_emit (file_list, gth_file_list_signals[DONE], 0);
}


void
gth_file_list_delete_list (GthFileList *file_list,
			   GList       *uri_list)
{
	gth_file_list_queue_op_with_list (file_list,
					  GTH_FILE_LIST_OP_TYPE_DELETE_LIST,
					  uri_list);
}


void
gth_file_list_delete (GthFileList *file_list,
		      const char  *uri)
{
	gth_file_list_delete_list (file_list, g_list_append (NULL, (gpointer)uri));
}


static void
gfl_rename (GthFileList *file_list,
	    const char  *from_uri,
	    const char  *to_uri)
{
	FileData *fd;
	int       pos;

	fd = gth_file_list_filedata_from_path (file_list, from_uri);
	if (fd == NULL)
		return;

	/* update the FileData structure. */

	file_data_set_path (fd, to_uri);

	/* Set the new name. */

	pos = gth_file_list_pos_from_path (file_list, from_uri);
	if (pos != -1) {
		gth_file_view_set_image_text (file_list->view, pos, fd->display_name);
		gth_file_view_sorted (file_list->view,
				      file_list->priv->sort_method,
				      file_list->priv->sort_type);
	}

	file_data_unref (fd);
}


void
gth_file_list_rename (GthFileList *file_list,
		      const char  *from_uri,
		      const char  *to_uri)
{
	gth_file_list_queue_op_with_2_uris (file_list,
					    GTH_FILE_LIST_OP_TYPE_RENAME,
					    from_uri,
					    to_uri);
}


static void
gfl_update_comment (GthFileList *file_list,
		    const char  *uri)
{
	int pos;

	pos = gth_file_list_pos_from_path (file_list, uri);
	if ((pos >= 0) && (pos < gth_file_view_get_images (file_list->view))) {
		FileData *fd;

		/* update the FileData structure. */

		fd = gth_file_view_get_image_data (file_list->view, pos);
		file_data_update_comment (fd);

		/* Set the new name. */

		gth_file_view_set_image_comment (file_list->view, pos, fd->comment);
		file_data_unref (fd);
	}
}


void
gth_file_list_update_comment (GthFileList *file_list,
			      const char  *uri)
{
	gth_file_list_queue_op_with_uri (file_list,
					 GTH_FILE_LIST_OP_TYPE_UPDATE_COMMENT,
					 uri);
}


static void
gfl_update_thumb (GthFileList *file_list,
		  const char  *uri)
{
	FileData *fd;
	int       pos;

	fd = gth_file_list_filedata_from_path (file_list, uri);
	if (fd == NULL)
		return;

	file_data_update (fd);

	pos = gth_file_list_pos_from_path (file_list, uri);
	if (pos == -1)
		return;

	file_list->priv->thumb_pos = pos;
	if (file_list->priv->thumb_fd != NULL)
		file_data_unref (file_list->priv->thumb_fd);
	file_list->priv->thumb_fd = fd;
}


void
gth_file_list_update_thumb (GthFileList *file_list,
			    const char  *uri)
{
	gth_file_list_queue_op_with_uri (file_list,
					 GTH_FILE_LIST_OP_TYPE_UPDATE_THUMB,
					 uri);
}


static void
gfl_update_thumb_list (GthFileList  *file_list,
		       GList        *list)
{
	GList *scan;

	for (scan = list; scan; scan = scan->next) {
		char      *path = scan->data;
		FileData  *fd;
		int        pos;

		pos = gth_file_list_pos_from_path (file_list, path);
		if (pos == -1)
			continue;

		fd = gth_file_view_get_image_data (file_list->view, pos);
		file_data_update (fd);
		file_data_unref (fd);
	}
}


void
gth_file_list_update_thumb_list (GthFileList  *file_list,
				 GList        *list)
{
	gth_file_list_queue_op_with_list (file_list,
					  GTH_FILE_LIST_OP_TYPE_UPDATE_THUMB_LIST,
					  list);
}


void
gth_file_list_restart_thumbs (GthFileList *file_list,
			      gboolean    _continue)
{
	if (file_list->busy)
		return;

	if (_continue)
		start_update_next_thumb (file_list);
	else
		gth_file_list_update_thumbs (file_list);
}


static void
gfl_set_thumbs_size (GthFileList *file_list,
	             int          size)
{
	file_list->priv->thumb_size = size;

	g_object_unref (G_OBJECT (file_list->priv->thumb_loader));
	file_list->priv->thumb_loader = THUMB_LOADER (thumb_loader_new (NULL, size, size));
	g_signal_connect (G_OBJECT (file_list->priv->thumb_loader),
			  "thumb_done",
			  G_CALLBACK (load_thumb_done_cb),
			  file_list);
	g_signal_connect (G_OBJECT (file_list->priv->thumb_loader),
			  "thumb_error",
			  G_CALLBACK (load_thumb_error_cb),
			  file_list);

	gth_file_view_set_image_width (file_list->view, size + THUMB_BORDER);

	gth_file_list_update_thumbs (file_list);
}


void
gth_file_list_set_thumbs_size (GthFileList *file_list,
			       int          size)
{
	gth_file_list_queue_op_with_int (file_list,
					 GTH_FILE_LIST_OP_TYPE_SET_THUMBS_SIZE,
					 size);
}


static gboolean
gfl_visible_func (gpointer callback_data,
    		  gpointer image_data)
{
	GthFileList *file_list = callback_data;
	FileData    *fdata = image_data;

	if (file_list->priv->filter == NULL)
		return TRUE;
	if (fdata == NULL)
		return FALSE;

	return gth_filter_match (file_list->priv->filter, fdata);
}


static void
gfl_set_filter (GthFileList *file_list)
{
	if (file_list->priv->filter == NULL)
		gth_file_view_set_visible_func (file_list->view, NULL, NULL);
	else
		gth_file_view_set_visible_func (file_list->view,
                	                        gfl_visible_func,
                        	                file_list);
	g_signal_emit (file_list, gth_file_list_signals[DONE], 0);
}


void
gth_file_list_set_filter (GthFileList *file_list,
			  GthFilter   *filter)
{
	if (file_list->priv->filter != NULL)
		g_object_unref (file_list->priv->filter);
	file_list->priv->filter = filter;

	gth_file_list_queue_op (file_list, gth_file_list_op_new (GTH_FILE_LIST_OP_TYPE_SET_FILTER));
}


static void
gth_file_list_exec_next_op (GthFileList *file_list)
{
	GList         *first;
	GthFileListOp *op;
	gboolean       exec_next_op = TRUE;

	if (file_list->priv->queue == NULL) {
		start_update_next_thumb (file_list);
		return;
	}

	first = file_list->priv->queue;
	file_list->priv->queue = g_list_remove_link (file_list->priv->queue, first);

	op = first->data;
	g_list_free (first);

	switch (op->type) {
	case GTH_FILE_LIST_OP_TYPE_UPDATE_COMMENT:
		gfl_update_comment (file_list, op->file.uri);
		break;
	case GTH_FILE_LIST_OP_TYPE_UPDATE_THUMB:
		gfl_update_thumb (file_list, op->file.uri);
		break;
	case GTH_FILE_LIST_OP_TYPE_RENAME:
		gfl_rename (file_list, op->file.uri_list->data, op->file.uri_list->next->data);
		break;
	case GTH_FILE_LIST_OP_TYPE_UPDATE_THUMB_LIST:
		gfl_update_thumb_list (file_list, op->file.uri_list);
		break;
	case GTH_FILE_LIST_OP_TYPE_ADD_LIST:
		gfl_add_list (file_list, op->file.uri_list);
		exec_next_op = FALSE;
		break;
	case GTH_FILE_LIST_OP_TYPE_SET_LIST:
		gfl_set_list (file_list, op->file.uri_list);
		exec_next_op = FALSE;
		break;
	case GTH_FILE_LIST_OP_TYPE_DELETE_LIST:
		gfl_delete_list (file_list, op->file.uri_list);
		break;
	case GTH_FILE_LIST_OP_TYPE_SET_THUMBS_SIZE:
		gfl_set_thumbs_size (file_list, op->file.ival);
		exec_next_op = FALSE;
		break;
	case GTH_FILE_LIST_OP_TYPE_ENABLE_THUMBS:
		gfl_enable_thumbs (file_list);
		exec_next_op = FALSE;
		break;
	case GTH_FILE_LIST_OP_TYPE_EMPTY_LIST:
		gfl_empty_list (file_list);
		break;
	case GTH_FILE_LIST_OP_TYPE_SET_FILTER:
		gfl_set_filter (file_list);
		exec_next_op = FALSE;
		break;
	}
	op->file.uri_list = NULL;
	gth_file_list_op_free (op);

	if (exec_next_op)
		gth_file_list_exec_next_op (file_list);
}
