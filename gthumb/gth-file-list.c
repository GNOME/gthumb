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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <string.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include "glib-utils.h"
#include "gth-cell-renderer-thumbnail.h"
#include "gth-dumb-notebook.h"
#include "gth-empty-list.h"
#include "gth-file-list.h"
#include "gth-file-store.h"
#include "gth-icon-cache.h"
#include "gth-icon-view.h"
#include "gth-thumb-loader.h"
#include "gtk-utils.h"


#define DEFAULT_THUMBNAIL_SIZE 112
#define FLASH_THUMBNAIL_QUEUE_TIMEOUT 75
#define UPDATE_THUMBNAILS_AFTER_SCROLL_TIMEOUT 125
#define RESTART_LOADING_THUMBS_DELAY 1500
#define N_VIEWAHEAD 50
#define N_CREATEAHEAD 50000
#define EMPTY (N_("(Empty)"))
#define THUMBNAIL_BORDER (8 * 2)
#define CHECK_JOBS_INTERVAL 50


typedef enum {
	GTH_FILE_LIST_OP_TYPE_SET_FILES,
	GTH_FILE_LIST_OP_TYPE_CLEAR_FILES,
	GTH_FILE_LIST_OP_TYPE_ADD_FILES,
	GTH_FILE_LIST_OP_TYPE_UPDATE_FILES,
	GTH_FILE_LIST_OP_TYPE_DELETE_FILES,
	GTH_FILE_LIST_OP_TYPE_SET_FILTER,
	GTH_FILE_LIST_OP_TYPE_SET_SORT_FUNC,
	GTH_FILE_LIST_OP_TYPE_ENABLE_THUMBS,
	GTH_FILE_LIST_OP_TYPE_RENAME_FILE
	/*GTH_FILE_LIST_OP_TYPE_SET_THUMBS_SIZE,*/
} GthFileListOpType;


typedef struct {
	GthFileListOpType    type;
	GtkTreeModel        *model;
	GthTest             *filter;
	GList               *file_list; /* GthFileData */
	GList               *files; /* GFile */
	GthFileDataCompFunc  cmp_func;
	gboolean             inverse_sort;
	char                *sval;
	int                  ival;
	GFile               *file;
	GthFileData         *file_data;
} GthFileListOp;


typedef struct {
	int   ref;
	guint error : 1;         /* Whether an error occurred loading
				  * this file. */
	guint thumb_loaded : 1;  /* Whether we have a thumb of this
				  * image. */
	guint thumb_created : 1; /* Whether a thumb has been
				  * created for this image. */
	GdkPixbuf *pixbuf;
} ThumbData;


enum {
	FILE_POPUP,
	LAST_SIGNAL
};


enum {
	GTH_FILE_LIST_PANE_VIEW,
	GTH_FILE_LIST_PANE_MESSAGE
};

struct _GthFileListPrivateData
{
	GthFileListType  type;
	GtkAdjustment   *vadj;
	GtkWidget       *notebook;
	GtkWidget       *view;
	GtkWidget       *message;
	GtkWidget       *scrolled_window;
	GthIconCache    *icon_cache;
	GthFileSource   *file_source;
	gboolean         load_thumbs;
	int              thumb_size;
	gboolean         ignore_hidden_thumbs;
	GHashTable      *thumb_data;
	GthThumbLoader  *thumb_loader;
	gboolean         loading_thumbs;
	gboolean         dirty;
	guint            dirty_event;
	guint            restart_thumb_update;
	GList           *queue; /* list of GthFileListOp */
	GList           *jobs; /* list of ThumbnailJob */
	GtkCellRenderer *thumbnail_renderer;
	GtkCellRenderer *text_renderer;
	GtkCellRenderer *checkbox_renderer;
	char           **caption_attributes_v;
	gboolean         cancelled;
};


/* OPs */


static void _gth_file_list_exec_next_op (GthFileList *file_list);


static GthFileListOp *
gth_file_list_op_new (GthFileListOpType op_type)
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
	case GTH_FILE_LIST_OP_TYPE_SET_FILES:
		_g_object_list_unref (op->file_list);
		break;
	case GTH_FILE_LIST_OP_TYPE_CLEAR_FILES:
		g_free (op->sval);
		break;
	case GTH_FILE_LIST_OP_TYPE_ADD_FILES:
	case GTH_FILE_LIST_OP_TYPE_UPDATE_FILES:
		_g_object_list_unref (op->file_list);
		break;
	case GTH_FILE_LIST_OP_TYPE_DELETE_FILES:
		_g_object_list_unref (op->files);
		break;
	case GTH_FILE_LIST_OP_TYPE_SET_FILTER:
		g_object_unref (op->filter);
		break;
	case GTH_FILE_LIST_OP_TYPE_RENAME_FILE:
		g_object_unref (op->file);
		g_object_unref (op->file_data);
		break;
	default:
		break;
	}
	g_free (op);
}


static void
_gth_file_list_clear_queue (GthFileList *file_list)
{
	if (file_list->priv->dirty_event != 0) {
		g_source_remove (file_list->priv->dirty_event);
		file_list->priv->dirty = FALSE;
	}

	if (file_list->priv->restart_thumb_update != 0) {
		g_source_remove (file_list->priv->restart_thumb_update);
		file_list->priv->restart_thumb_update = 0;
	}

	g_list_foreach (file_list->priv->queue, (GFunc) gth_file_list_op_free, NULL);
	g_list_free (file_list->priv->queue);
	file_list->priv->queue = NULL;
}


static void
_gth_file_list_remove_op (GthFileList       *file_list,
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

		file_list->priv->queue = g_list_remove_link (file_list->priv->queue, scan);
		gth_file_list_op_free (op);
		g_list_free (scan);

		scan = file_list->priv->queue;
	}
}


static void
_gth_file_list_queue_op (GthFileList   *file_list,
			 GthFileListOp *op)
{
	if ((op->type == GTH_FILE_LIST_OP_TYPE_SET_FILES) || (op->type == GTH_FILE_LIST_OP_TYPE_CLEAR_FILES))
		_gth_file_list_clear_queue (file_list);
	if (op->type == GTH_FILE_LIST_OP_TYPE_SET_FILTER)
		_gth_file_list_remove_op (file_list, GTH_FILE_LIST_OP_TYPE_SET_FILTER);
	file_list->priv->queue = g_list_append (file_list->priv->queue, op);
	file_list->priv->cancelled = FALSE;
	if (! file_list->priv->loading_thumbs)
		_gth_file_list_exec_next_op (file_list);
}


/* -- gth_file_list -- */


static GtkHBoxClass *parent_class = NULL;


static void
gth_file_list_finalize (GObject *object)
{
	GthFileList *file_list;

	file_list = GTH_FILE_LIST (object);

	if (file_list->priv != NULL) {
		g_hash_table_unref (file_list->priv->thumb_data);
		if (file_list->priv->icon_cache != NULL)
			gth_icon_cache_free (file_list->priv->icon_cache);
		g_strfreev (file_list->priv->caption_attributes_v);

		g_free (file_list->priv);
		file_list->priv = NULL;
	}

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gth_file_list_size_request (GtkWidget      *widget,
			    GtkRequisition *requisition)
{
	GthFileList *file_list;

	GTK_WIDGET_CLASS (parent_class)->size_request (widget, requisition);

	file_list = GTH_FILE_LIST (widget);

	if (file_list->priv->type == GTH_FILE_LIST_TYPE_V_SIDEBAR) {
		int        xthickness;
		GtkWidget *vscrollbar;

		xthickness = gtk_widget_get_style (file_list->priv->scrolled_window)->xthickness;
		requisition->width = file_list->priv->thumb_size + (THUMBNAIL_BORDER * 2) + xthickness * 2;

		vscrollbar = gtk_scrolled_window_get_vscrollbar (GTK_SCROLLED_WINDOW (file_list->priv->scrolled_window));
		if (gtk_widget_get_visible (vscrollbar)) {
			GtkRequisition vscrollbar_requisition;
			int            scrollbar_spacing;

			gtk_widget_size_request (vscrollbar, &vscrollbar_requisition);
			gtk_widget_style_get (file_list->priv->scrolled_window,
					      "scrollbar-spacing", &scrollbar_spacing,
					      NULL);
			requisition->width += vscrollbar_requisition.width + scrollbar_spacing;
		}
	}
	else if (file_list->priv->type == GTH_FILE_LIST_TYPE_H_SIDEBAR) {
		int ythickness;

		ythickness = gtk_widget_get_style (file_list->priv->scrolled_window)->ythickness;
		requisition->height = file_list->priv->thumb_size + (THUMBNAIL_BORDER * 2) + ythickness * 2;
	}
}


static void
gth_file_list_class_init (GthFileListClass *class)
{
	GObjectClass   *object_class;
	GtkWidgetClass *widget_class;

	parent_class = g_type_class_peek_parent (class);

	object_class = (GObjectClass*) class;
	object_class->finalize = gth_file_list_finalize;

	widget_class = (GtkWidgetClass*) class;
	widget_class->size_request = gth_file_list_size_request;
}


static ThumbData *
thumb_data_new (void)
{
	ThumbData *data;

	data = g_new0 (ThumbData, 1);
	data->ref = 1;

	return data;
}


static ThumbData *
thumb_data_ref (ThumbData *data)
{
	data->ref++;
	return data;
}


static void
thumb_data_unref (ThumbData *data)
{
	data->ref--;
	if (data->ref > 0)
		return;
	_g_object_unref (data->pixbuf);
	g_free (data);
}


static void
gth_file_list_init (GthFileList *file_list)
{
	file_list->priv = g_new0 (GthFileListPrivateData, 1);
	file_list->priv->thumb_data = g_hash_table_new_full (g_file_hash, (GEqualFunc) g_file_equal, g_object_unref, (GDestroyNotify) thumb_data_unref);
	file_list->priv->thumb_size = DEFAULT_THUMBNAIL_SIZE;
	file_list->priv->ignore_hidden_thumbs = FALSE;
	file_list->priv->load_thumbs = TRUE;
	file_list->priv->caption_attributes_v = g_strsplit ("none", ",", -1);
	file_list->priv->cancelled = FALSE;
}


static void _gth_file_list_update_next_thumb (GthFileList *file_list);


static gboolean
restart_thumb_update_cb (gpointer data)
{
	GthFileList *file_list = data;

	if (file_list->priv->restart_thumb_update != 0) {
		g_source_remove (file_list->priv->restart_thumb_update);
		file_list->priv->restart_thumb_update = 0;
	}

	file_list->priv->loading_thumbs = TRUE;
	_gth_file_list_update_next_thumb (file_list);

	return FALSE;
}


static void
start_update_next_thumb (GthFileList *file_list)
{
	if (file_list->priv->loading_thumbs)
		return;

	if (! file_list->priv->load_thumbs) {
		file_list->priv->loading_thumbs = FALSE;
		return;
	}

	if (file_list->priv->restart_thumb_update != 0)
		g_source_remove (file_list->priv->restart_thumb_update);
	file_list->priv->restart_thumb_update = g_timeout_add (UPDATE_THUMBNAILS_AFTER_SCROLL_TIMEOUT, restart_thumb_update_cb, file_list);
}


static void
vadj_changed_cb (GtkAdjustment *adjustment,
		 gpointer       user_data)
{
	start_update_next_thumb (GTH_FILE_LIST (user_data));
}


static void
file_view_drag_data_get_cb (GtkWidget        *widget,
			    GdkDragContext   *drag_context,
			    GtkSelectionData *data,
			    guint             info,
			    guint             time,
			    gpointer          user_data)
{
	GthFileList  *file_list = user_data;
	GList        *items;
	GList        *files;
	GList        *scan;
	int           n_uris;
	char        **uris;
	int           i;

	items = gth_file_selection_get_selected (GTH_FILE_SELECTION (file_list->priv->view));
	files = gth_file_list_get_files (file_list, items);
	n_uris = g_list_length (files);
	uris = g_new (char *, n_uris + 1);
	for (scan = files, i = 0; scan; scan = scan->next, i++) {
		GthFileData *file_data = scan->data;
		uris[i] = g_file_get_uri (file_data->file);
	}
	uris[i] = NULL;

	gtk_selection_data_set_uris (data, uris);

	g_strfreev (uris);
	_g_object_list_unref (files);
	_gtk_tree_path_list_free (items);
}


static void
checkbox_toggled_cb (GtkCellRendererToggle *cell_renderer,
                     char                  *path,
                     gpointer               user_data)
{
	GthFileList  *file_list = user_data;
	GtkTreePath  *tpath;
	GthFileStore *file_store;
	GtkTreeIter   iter;

	tpath = gtk_tree_path_new_from_string (path);
	if (tpath == NULL)
		return;

	file_store = (GthFileStore*) gth_file_view_get_model (GTH_FILE_VIEW (file_list->priv->view));
	if (gtk_tree_model_get_iter (GTK_TREE_MODEL (file_store), &iter, tpath)) {
		gboolean checked;

		gtk_tree_model_get (GTK_TREE_MODEL (file_store), &iter,
				    GTH_FILE_STORE_CHECKED_COLUMN, &checked,
				    -1);
		gth_file_store_set (file_store,
				    &iter,
				    GTH_FILE_STORE_CHECKED_COLUMN, ! checked,
				    -1);
	}

	gtk_tree_path_free (tpath);
}


static void
_gth_file_list_update_orientation (GthFileList *file_list)
{
	if (file_list->priv->type == GTH_FILE_LIST_TYPE_V_SIDEBAR)
		gtk_orientable_set_orientation (GTK_ORIENTABLE (file_list), GTK_ORIENTATION_VERTICAL);
	else if (file_list->priv->type == GTH_FILE_LIST_TYPE_H_SIDEBAR)
		gtk_orientable_set_orientation (GTK_ORIENTABLE (file_list), GTK_ORIENTATION_HORIZONTAL);
}


static void
_gth_file_list_set_type (GthFileList     *file_list,
		 	 GthFileListType  list_type)
{
	file_list->priv->type = list_type;

	if ((file_list->priv->type == GTH_FILE_LIST_TYPE_SELECTOR) || (file_list->priv->type == GTH_FILE_LIST_TYPE_NO_SELECTION))
		gth_file_selection_set_selection_mode (GTH_FILE_SELECTION (file_list->priv->view), GTK_SELECTION_NONE);
	else if ((file_list->priv->type == GTH_FILE_LIST_TYPE_H_SIDEBAR) || (file_list->priv->type == GTH_FILE_LIST_TYPE_V_SIDEBAR))
		gth_file_selection_set_selection_mode (GTH_FILE_SELECTION (file_list->priv->view), GTK_SELECTION_SINGLE);
	else
		gth_file_selection_set_selection_mode (GTH_FILE_SELECTION (file_list->priv->view), GTK_SELECTION_MULTIPLE);

	g_object_set (file_list->priv->checkbox_renderer,
		      "visible", ((file_list->priv->type == GTH_FILE_LIST_TYPE_BROWSER) || (file_list->priv->type == GTH_FILE_LIST_TYPE_SELECTOR)),
		      NULL);

	g_object_set (file_list->priv->thumbnail_renderer,
		      "fixed_size", (file_list->priv->type == GTH_FILE_LIST_TYPE_H_SIDEBAR) || (file_list->priv->type == GTH_FILE_LIST_TYPE_V_SIDEBAR),
		      NULL);

	_gth_file_list_update_orientation (file_list);
}


static void
gth_file_list_construct (GthFileList     *file_list,
			 GthFileListType  list_type,
			 gboolean         enable_drag_drop)
{
	GtkWidget       *viewport;
	GtkCellRenderer *renderer;
	GthFileStore    *model;

	file_list->priv->thumb_loader = gth_thumb_loader_new (file_list->priv->thumb_size);
	file_list->priv->icon_cache = gth_icon_cache_new (gtk_icon_theme_get_for_screen (gtk_widget_get_screen (GTK_WIDGET (file_list))), file_list->priv->thumb_size / 2);

	/* the main notebook */

	file_list->priv->notebook = gth_dumb_notebook_new ();

	/* the message pane */

	viewport = gtk_viewport_new (NULL, NULL);
	gtk_viewport_set_shadow_type (GTK_VIEWPORT (viewport),
				      GTK_SHADOW_ETCHED_IN);

	file_list->priv->message = gth_empty_list_new (_(EMPTY));

	/* the file view */

	file_list->priv->scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (file_list->priv->scrolled_window),
					GTK_POLICY_NEVER,
					GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (file_list->priv->scrolled_window),
					     GTK_SHADOW_ETCHED_IN);

	file_list->priv->vadj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (file_list->priv->scrolled_window));
	g_signal_connect (G_OBJECT (file_list->priv->vadj),
			  "changed",
			  G_CALLBACK (vadj_changed_cb),
			  file_list);
	g_signal_connect (G_OBJECT (file_list->priv->vadj),
			  "value-changed",
			  G_CALLBACK (vadj_changed_cb),
			  file_list);

	model = gth_file_store_new ();
	file_list->priv->view = gth_icon_view_new_with_model (GTK_TREE_MODEL (model));
	g_object_unref (model);

	if (enable_drag_drop) {
		GtkTargetList  *target_list;
		GtkTargetEntry *targets;
		int             n_targets;

		target_list = gtk_target_list_new (NULL, 0);
		gtk_target_list_add_uri_targets (target_list, 0);
		gtk_target_list_add_text_targets (target_list, 0);
		targets = gtk_target_table_new_from_list (target_list, &n_targets);
		gth_file_view_enable_drag_source (GTH_FILE_VIEW (file_list->priv->view),
						  GDK_BUTTON1_MASK,
						  targets,
						  n_targets,
						  GDK_ACTION_MOVE | GDK_ACTION_COPY);

		gtk_target_list_unref (target_list);
		gtk_target_table_free (targets, n_targets);

		g_signal_connect (G_OBJECT (file_list->priv->view),
				  "drag-data-get",
				  G_CALLBACK (file_view_drag_data_get_cb),
				  file_list);
	}

	/* checkbox */

	file_list->priv->checkbox_renderer = renderer = gtk_cell_renderer_toggle_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (file_list->priv->view), renderer, FALSE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (file_list->priv->view),
					renderer,
					"active", GTH_FILE_STORE_CHECKED_COLUMN,
					NULL);
	g_signal_connect (file_list->priv->checkbox_renderer,
			  "toggled",
			  G_CALLBACK (checkbox_toggled_cb),
			  file_list);

	/* thumbnail */

	file_list->priv->thumbnail_renderer = renderer = gth_cell_renderer_thumbnail_new ();
	g_object_set (renderer,
		      "size", file_list->priv->thumb_size,
		      "yalign", 1.0,
		      NULL);
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (file_list->priv->view), renderer, FALSE);

	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (file_list->priv->view),
					renderer,
					"thumbnail", GTH_FILE_STORE_THUMBNAIL_COLUMN,
					"is_icon", GTH_FILE_STORE_IS_ICON_COLUMN,
					"file", GTH_FILE_STORE_FILE_DATA_COLUMN,
					NULL);

	if (file_list->priv->type == GTH_FILE_LIST_TYPE_BROWSER)
		gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (file_list->priv->view),
					       renderer,
					       "selected",
					       GTH_FILE_STORE_CHECKED_COLUMN);
	else if (file_list->priv->type == GTH_FILE_LIST_TYPE_SELECTOR)
		gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (file_list->priv->view),
					       renderer,
					       "checked",
					       GTH_FILE_STORE_CHECKED_COLUMN);

	/* text */

	file_list->priv->text_renderer = renderer = gtk_cell_renderer_text_new ();
	g_object_set (G_OBJECT (renderer),
		      /*"ellipsize", PANGO_ELLIPSIZE_NONE,*/
		      "alignment", PANGO_ALIGN_CENTER,
		      "width", file_list->priv->thumb_size + THUMBNAIL_BORDER,
		      "wrap-mode", PANGO_WRAP_WORD_CHAR,
		      "wrap-width", file_list->priv->thumb_size + THUMBNAIL_BORDER,
		      NULL);

	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (file_list->priv->view), renderer, FALSE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (file_list->priv->view),
					renderer,
					"text", GTH_FILE_STORE_METADATA_COLUMN,
					NULL);

	_gth_file_list_set_type (file_list, list_type);

	/* pack the widgets together */

	gtk_widget_show (file_list->priv->view);
	gtk_container_add (GTK_CONTAINER (file_list->priv->scrolled_window), file_list->priv->view);

	gtk_widget_show (file_list->priv->scrolled_window);
	gtk_container_add (GTK_CONTAINER (file_list->priv->notebook), file_list->priv->scrolled_window);

	gtk_widget_show (file_list->priv->message);
	gtk_container_add (GTK_CONTAINER (viewport), file_list->priv->message);

	gtk_widget_show (viewport);
	gtk_container_add (GTK_CONTAINER (file_list->priv->notebook), viewport);

	gtk_widget_show (file_list->priv->notebook);
	gtk_box_pack_start (GTK_BOX (file_list), file_list->priv->notebook, TRUE, TRUE, 0);

	gth_dumb_notebook_show_child (GTH_DUMB_NOTEBOOK (file_list->priv->notebook), GTH_FILE_LIST_PANE_MESSAGE);
}


GType
gth_file_list_get_type (void)
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

		type = g_type_register_static (GTK_TYPE_VBOX,
					       "GthFileList",
					       &type_info,
					       0);
	}

	return type;
}


GtkWidget*
gth_file_list_new (GthFileListType list_type,
		   gboolean        enable_drag_drop)
{
	GtkWidget *widget;

	widget = GTK_WIDGET (g_object_new (GTH_TYPE_FILE_LIST, NULL));
	gth_file_list_construct (GTH_FILE_LIST (widget), list_type, enable_drag_drop);

	return widget;
}


void
gth_file_list_set_type (GthFileList     *file_list,
			GthFileListType  list_type)
{
	_gth_file_list_set_type (file_list, list_type);
}


static void
_gth_file_list_done (GthFileList *file_list)
{
	file_list->priv->loading_thumbs = FALSE;
}


typedef struct {
	GthFileList    *file_list;
	GthThumbLoader *loader;
	GCancellable   *cancellable;
	GthFileData    *file_data;
	gboolean        update_in_view;
	int             pos;
} ThumbnailJob;


static void
thumbnail_job_free (ThumbnailJob *job)
{
	job->file_list->priv->jobs = g_list_remove (job->file_list->priv->jobs, job);
	if (job->file_list->priv->jobs == NULL)
		_gth_file_list_done (job->file_list);

	_g_object_unref (job->file_data);
	_g_object_unref (job->cancellable);
	_g_object_unref (job->loader);
	_g_object_unref (job->file_list);
	g_free (job);
}


static void
thumbnail_job_cancel (ThumbnailJob *job)
{
	if (! g_cancellable_is_cancelled (job->cancellable))
		g_cancellable_cancel (job->cancellable);
}


typedef struct {
	GthFileList *file_list;
	DataFunc     done_func;
	gpointer     user_data;
	guint        check_id;
} CancelData;


static void
cancel_data_free (CancelData *cancel_data)
{
	if (cancel_data->check_id != 0)
		g_source_remove (cancel_data->check_id);
	g_object_unref (cancel_data->file_list);
	g_free (cancel_data);
}


static gboolean
wait_for_jobs_to_finish (gpointer user_data)
{
	CancelData *cancel_data = user_data;

	if (cancel_data->file_list->priv->jobs == NULL) {
		if (cancel_data->check_id != 0) {
			g_source_remove (cancel_data->check_id);
			cancel_data->check_id = 0;
		}
		if (cancel_data->done_func != NULL)
			cancel_data->done_func (cancel_data->user_data);
		cancel_data_free (cancel_data);
		return FALSE;
	}

	return TRUE;
}


static void
_gth_file_list_cancel_jobs (GthFileList *file_list,
			    DataFunc     done_func,
			    gpointer     user_data)
{
	CancelData *cancel_data;
	GList      *scan;

	cancel_data = g_new0 (CancelData, 1);
	cancel_data->file_list = g_object_ref (file_list);
	cancel_data->done_func = done_func;
	cancel_data->user_data = user_data;

	if (file_list->priv->jobs == NULL) {
		cancel_data->check_id = g_idle_add (wait_for_jobs_to_finish, cancel_data);
		return;
	}

	for (scan = file_list->priv->jobs; scan; scan = scan->next) {
		ThumbnailJob *job = scan->data;
		thumbnail_job_cancel (job);
	}

	cancel_data->check_id = g_timeout_add (CHECK_JOBS_INTERVAL,
					       wait_for_jobs_to_finish,
					       cancel_data);
}


void
gth_file_list_cancel (GthFileList    *file_list,
		      DataFunc        done_func,
		      gpointer        user_data)
{
	file_list->priv->cancelled = TRUE;
	_gth_file_list_clear_queue (file_list);
	_gth_file_list_cancel_jobs (file_list, done_func, user_data);
}


GthThumbLoader *
gth_file_list_get_thumb_loader (GthFileList *file_list)
{
	return file_list->priv->thumb_loader;
}


static void
gfl_clear_list (GthFileList *file_list,
		const char  *message)
{
	GthFileStore *file_store;

	gth_file_selection_unselect_all (GTH_FILE_SELECTION (file_list->priv->view));

	file_store = (GthFileStore*) gth_file_view_get_model (GTH_FILE_VIEW (file_list->priv->view));
	gth_file_store_clear (file_store);
	g_hash_table_remove_all (file_list->priv->thumb_data);

	gth_empty_list_set_text (GTH_EMPTY_LIST (file_list->priv->message), message);
	gth_dumb_notebook_show_child (GTH_DUMB_NOTEBOOK (file_list->priv->notebook), GTH_FILE_LIST_PANE_MESSAGE);
}


void
gth_file_list_clear (GthFileList *file_list,
		     const char  *message)
{
	GthFileListOp *op;

	op = gth_file_list_op_new (GTH_FILE_LIST_OP_TYPE_CLEAR_FILES);
	op->sval = g_strdup (message != NULL ? message : _(EMPTY));
	_gth_file_list_queue_op (file_list, op);
}


static void
_gth_file_list_update_pane (GthFileList *file_list)
{
	GthFileStore *file_store;

	file_store = (GthFileStore*) gth_file_view_get_model (GTH_FILE_VIEW (file_list->priv->view));

	if (gth_file_store_n_visibles (file_store) > 0) {
		gth_dumb_notebook_show_child (GTH_DUMB_NOTEBOOK (file_list->priv->notebook), GTH_FILE_LIST_PANE_VIEW);
	}
	else {
		gth_empty_list_set_text (GTH_EMPTY_LIST (file_list->priv->message), _(EMPTY));
		gth_dumb_notebook_show_child (GTH_DUMB_NOTEBOOK (file_list->priv->notebook), GTH_FILE_LIST_PANE_MESSAGE);
	}
}


static GString *
_gth_file_list_get_metadata (GthFileList *file_list,
			     GthFileData *file_data)
{
	GString *metadata;
	int      i;

	metadata = g_string_new ("");
	for (i = 0; file_list->priv->caption_attributes_v[i] != NULL; i++) {
		char *value;

		value = gth_file_data_get_attribute_as_string (file_data, file_list->priv->caption_attributes_v[i]);
		if (value == NULL)
			value = g_strdup ("-");
		if (metadata->len > 0)
			g_string_append (metadata, "\n");
		g_string_append (metadata, value);

		g_free (value);
	}

	return metadata;
}


static void
gfl_add_files (GthFileList *file_list,
	       GList       *files)
{
	GthFileStore *file_store;
	GList        *scan;

	file_store = (GthFileStore*) gth_file_view_get_model (GTH_FILE_VIEW (file_list->priv->view));

	for (scan = files; scan; scan = scan->next) {
		GthFileData *file_data = scan->data;
		GIcon       *icon;
		GdkPixbuf   *pixbuf = NULL;
		GString     *metadata;

		if (g_file_info_get_file_type (file_data->info) != G_FILE_TYPE_REGULAR)
			continue;

		if (g_hash_table_lookup (file_list->priv->thumb_data, file_data->file) != NULL)
			continue;

		g_hash_table_insert (file_list->priv->thumb_data,
				     g_object_ref (file_data->file),
				     thumb_data_new ());

		icon = g_file_info_get_icon (file_data->info);
		pixbuf = gth_icon_cache_get_pixbuf (file_list->priv->icon_cache, icon);
		metadata = _gth_file_list_get_metadata (file_list, file_data);
		gth_file_store_queue_add (file_store,
					  file_data,
					  pixbuf,
					  TRUE,
					  metadata->str,
					  TRUE);

		g_string_free (metadata, TRUE);
		if (pixbuf != NULL)
			g_object_unref (pixbuf);
	}

	gth_file_store_exec_add (file_store);
	_gth_file_list_update_pane (file_list);
}


void
gth_file_list_add_files (GthFileList *file_list,
			 GList       *files)
{
	GthFileListOp *op;

	op = gth_file_list_op_new (GTH_FILE_LIST_OP_TYPE_ADD_FILES);
	op->file_list = _g_object_list_ref (files);
	_gth_file_list_queue_op (file_list, op);
}


static void
gfl_delete_files (GthFileList *file_list,
		  GList       *files)
{
	GthFileStore *file_store;
	GList        *scan;

	file_store = (GthFileStore*) gth_file_view_get_model (GTH_FILE_VIEW (file_list->priv->view));
	for (scan = files; scan; scan = scan->next) {
		GFile       *file = scan->data;
		GtkTreeIter  iter;

		if (g_hash_table_lookup (file_list->priv->thumb_data, file) == NULL)
			continue;

		g_hash_table_remove (file_list->priv->thumb_data, file);

		if (gth_file_store_find (file_store, file, &iter))
			gth_file_store_queue_remove (file_store, &iter);
	}
	gth_file_store_exec_remove (file_store);
	_gth_file_list_update_pane (file_list);
}


void
gth_file_list_delete_files (GthFileList *file_list,
			    GList       *files)
{
	GthFileListOp *op;

	op = gth_file_list_op_new (GTH_FILE_LIST_OP_TYPE_DELETE_FILES);
	op->files = _g_object_list_ref (files);
	_gth_file_list_queue_op (file_list, op);
}


static void
gfl_update_files (GthFileList *file_list,
		  GList       *files)
{
	GthFileStore *file_store;
	GList        *scan;

	file_store = (GthFileStore*) gth_file_view_get_model (GTH_FILE_VIEW (file_list->priv->view));
	for (scan = files; scan; scan = scan->next) {
		GthFileData *file_data = scan->data;
		ThumbData   *thumb_data;
		GtkTreeIter  iter;

		thumb_data = g_hash_table_lookup (file_list->priv->thumb_data, file_data->file);
		if (thumb_data == NULL)
			continue;

		thumb_data->error = FALSE;
		thumb_data->thumb_loaded = FALSE;
		thumb_data->thumb_created = FALSE;

		if (gth_file_store_find (file_store, file_data->file, &iter)) {
			GString *metadata;

			metadata = _gth_file_list_get_metadata (file_list, file_data);
			gth_file_store_queue_set (file_store,
						  &iter,
						  GTH_FILE_STORE_FILE_DATA_COLUMN, file_data,
						  GTH_FILE_STORE_METADATA_COLUMN, metadata->str,
						  -1);

			g_string_free (metadata, TRUE);
		}
	}
	gth_file_store_exec_set (file_store);
	_gth_file_list_update_pane (file_list);
}


void
gth_file_list_update_files (GthFileList *file_list,
			    GList       *files)
{
	GthFileListOp *op;

	op = gth_file_list_op_new (GTH_FILE_LIST_OP_TYPE_UPDATE_FILES);
	op->file_list = _g_object_list_ref (files);
	_gth_file_list_queue_op (file_list, op);
}


static void
gfl_rename_file (GthFileList *file_list,
		 GFile       *file,
		 GthFileData *file_data)
{
	GthFileStore *file_store;
	GtkTreeIter   iter;

	file_store = (GthFileStore*) gth_file_view_get_model (GTH_FILE_VIEW (file_list->priv->view));
	if (gth_file_store_find (file_store, file, &iter)) {
		ThumbData *thumb_data;
		GString   *metadata;

		thumb_data = g_hash_table_lookup (file_list->priv->thumb_data, file);
		g_assert (thumb_data != NULL);

		g_hash_table_insert (file_list->priv->thumb_data,
				     g_object_ref (file_data->file),
				     thumb_data_ref (thumb_data));
		g_hash_table_remove (file_list->priv->thumb_data, file);

		metadata = _gth_file_list_get_metadata (file_list, file_data);
		gth_file_store_set (file_store,
				    &iter,
				    GTH_FILE_STORE_FILE_DATA_COLUMN, file_data,
				    GTH_FILE_STORE_METADATA_COLUMN, metadata->str,
				    -1);

		g_string_free (metadata, TRUE);
	}
	_gth_file_list_update_pane (file_list);
}


void
gth_file_list_rename_file (GthFileList *file_list,
			   GFile       *file,
			   GthFileData *file_data)
{
	GthFileListOp *op;

	op = gth_file_list_op_new (GTH_FILE_LIST_OP_TYPE_RENAME_FILE);
	op->file = g_object_ref (file);
	op->file_data = g_object_ref (file_data);
	_gth_file_list_queue_op (file_list, op);
}


static void
gfl_set_files (GthFileList *file_list,
	       GList       *files)
{
	gth_file_selection_unselect_all (GTH_FILE_SELECTION (file_list->priv->view));

	gth_file_store_clear ((GthFileStore*) gth_file_view_get_model (GTH_FILE_VIEW (file_list->priv->view)));
	g_hash_table_remove_all (file_list->priv->thumb_data);
	gfl_add_files (file_list, files);
}


void
gth_file_list_set_files (GthFileList *file_list,
			 GList       *files)
{
	GthFileListOp *op;

	if (files != NULL) {
		op = gth_file_list_op_new (GTH_FILE_LIST_OP_TYPE_SET_FILES);
		op->file_list = _g_object_list_ref (files);
		_gth_file_list_queue_op (file_list, op);
	}
	else {
		op = gth_file_list_op_new (GTH_FILE_LIST_OP_TYPE_CLEAR_FILES);
		op->sval = g_strdup (_(EMPTY));
		_gth_file_list_queue_op (file_list, op);
	}
}


GList *
gth_file_list_get_files (GthFileList *file_list,
			 GList       *items)
{
	GList        *list = NULL;
	GthFileStore *file_store;
	GList        *scan;

	file_store = (GthFileStore*) gth_file_view_get_model (GTH_FILE_VIEW (file_list->priv->view));
	for (scan = items; scan; scan = scan->next) {
		GtkTreePath *tree_path = scan->data;
		GtkTreeIter  iter;
		GthFileData *file_data;

		if (! gtk_tree_model_get_iter (GTK_TREE_MODEL (file_store), &iter, tree_path))
			continue;
		file_data = gth_file_store_get_file (file_store, &iter);
		if (file_data != NULL)
			list = g_list_prepend (list, g_object_ref (file_data));
	}

	return g_list_reverse (list);
}


static void
gfl_set_filter (GthFileList *file_list,
		GthTest     *filter)
{
	GthFileStore *file_store;

	file_store = (GthFileStore*) gth_file_view_get_model (GTH_FILE_VIEW (file_list->priv->view));
	if (file_store != NULL)
		gth_file_store_set_filter (file_store, filter);
	_gth_file_list_update_pane (file_list);
}


void
gth_file_list_set_filter (GthFileList *file_list,
			  GthTest     *filter)
{
	GthFileListOp *op;

	op = gth_file_list_op_new (GTH_FILE_LIST_OP_TYPE_SET_FILTER);
	if (filter != NULL)
		op->filter = g_object_ref (filter);
	else
		op->filter = gth_test_new ();
	_gth_file_list_queue_op (file_list, op);
}


static void
gfl_set_sort_func (GthFileList         *file_list,
		   GthFileDataCompFunc  cmp_func,
		   gboolean             inverse_sort)
{
	GthFileStore *file_store;

	file_store = (GthFileStore*) gth_file_view_get_model (GTH_FILE_VIEW (file_list->priv->view));
	if (file_store != NULL)
		gth_file_store_set_sort_func (file_store, cmp_func, inverse_sort);
}


void
gth_file_list_set_sort_func (GthFileList         *file_list,
			     GthFileDataCompFunc  cmp_func,
			     gboolean             inverse_sort)
{
	GthFileListOp *op;

	op = gth_file_list_op_new (GTH_FILE_LIST_OP_TYPE_SET_SORT_FUNC);
	op->cmp_func = cmp_func;
	op->inverse_sort = inverse_sort;
	_gth_file_list_queue_op (file_list, op);
}


static void
gfl_enable_thumbs (GthFileList *file_list,
		   gboolean     enable)
{
	GthFileStore *file_store;
	GtkTreeIter   iter;

	file_list->priv->load_thumbs = enable;

	file_store = (GthFileStore*) gth_file_view_get_model (GTH_FILE_VIEW (file_list->priv->view));
	if (gth_file_store_get_first (file_store, &iter)) {
		do {
			GthFileData *file_data;
			ThumbData   *thumb_data;
			GIcon       *icon;
			GdkPixbuf   *pixbuf;

			file_data = gth_file_store_get_file (file_store, &iter);

			thumb_data = g_hash_table_lookup (file_list->priv->thumb_data, file_data->file);
			g_assert (thumb_data != NULL);
			thumb_data->thumb_loaded = FALSE;

			icon = g_file_info_get_icon (file_data->info);
			pixbuf = gth_icon_cache_get_pixbuf (file_list->priv->icon_cache, icon);
			gth_file_store_queue_set (file_store,
						  &iter,
						  GTH_FILE_STORE_THUMBNAIL_COLUMN, pixbuf,
						  GTH_FILE_STORE_IS_ICON_COLUMN, TRUE,
						  -1);

			_g_object_unref (pixbuf);
		}
		while (gth_file_store_get_next (file_store, &iter));

		gth_file_store_exec_set (file_store);
	}

	start_update_next_thumb (file_list);
}


void
gth_file_list_enable_thumbs (GthFileList *file_list,
			     gboolean     enable)
{
	GthFileListOp *op;

	op = gth_file_list_op_new (GTH_FILE_LIST_OP_TYPE_ENABLE_THUMBS);
	op->ival = enable;
	_gth_file_list_queue_op (file_list, op);
}


void
gth_file_list_set_ignore_hidden (GthFileList *file_list,
				 gboolean      value)
{
	file_list->priv->ignore_hidden_thumbs = value;
}


void
gth_file_list_set_thumb_size (GthFileList *file_list,
			      int          size)
{
	file_list->priv->thumb_size = size;
	gth_thumb_loader_set_requested_size (file_list->priv->thumb_loader, size);

	gth_icon_cache_free (file_list->priv->icon_cache);
	file_list->priv->icon_cache = gth_icon_cache_new (gtk_icon_theme_get_for_screen (gtk_widget_get_screen (GTK_WIDGET (file_list))), size / 2);

	g_object_set (file_list->priv->thumbnail_renderer,
		      "size", file_list->priv->thumb_size,
		      NULL);
	g_object_set (file_list->priv->text_renderer,
		      "width", file_list->priv->thumb_size + THUMBNAIL_BORDER,
		      "wrap-width", file_list->priv->thumb_size + THUMBNAIL_BORDER,
		      NULL);

	_gth_file_list_update_orientation (file_list);
}


void
gth_file_list_set_caption (GthFileList *file_list,
			   const char  *attributes)
{
	GthFileStore *file_store;
	GtkTreeIter   iter;
	gboolean      metadata_visible;

	g_strfreev (file_list->priv->caption_attributes_v);
	file_list->priv->caption_attributes_v = g_strsplit (attributes, ",", -1);

	metadata_visible = (strcmp (file_list->priv->caption_attributes_v[0], "none") != 0);
	g_object_set (file_list->priv->text_renderer,
		      "visible", metadata_visible,
		      "height", metadata_visible ? -1 : 0,
		      NULL);

	file_store = (GthFileStore *) gth_file_view_get_model (GTH_FILE_VIEW (file_list->priv->view));
	if (gth_file_store_get_first (file_store, &iter)) {
		do {
			GthFileData *file_data;
			GString     *metadata;

			file_data = gth_file_store_get_file (file_store, &iter);
			metadata = _gth_file_list_get_metadata (file_list, file_data);
			gth_file_store_queue_set (file_store,
						  &iter,
						  GTH_FILE_STORE_METADATA_COLUMN, metadata->str,
						  -1);

			g_string_free (metadata, TRUE);
		}
		while (gth_file_store_get_next (file_store, &iter));

		gth_file_store_exec_set (file_store);
	}
}


GtkWidget *
gth_file_list_get_view (GthFileList *file_list)
{
	return file_list->priv->view;
}


GtkWidget *
gth_file_list_get_empty_view (GthFileList *file_list)
{
	return file_list->priv->message;
}


GtkAdjustment *
gth_file_list_get_vadjustment (GthFileList *file_list)
{
	return file_list->priv->vadj;
}


/* thumbs */


static void
flash_queue (GthFileList *file_list)
{
	if (file_list->priv->dirty) {
		GthFileStore *file_store;

		file_store = (GthFileStore *) gth_file_view_get_model (GTH_FILE_VIEW (file_list->priv->view));
		gth_file_store_exec_set (file_store);
		file_list->priv->dirty = FALSE;
	}

	if (file_list->priv->dirty_event != 0) {
		g_source_remove (file_list->priv->dirty_event);
		file_list->priv->dirty_event = 0;
	}
}


static gboolean
flash_queue_cb (gpointer data)
{
	flash_queue ((GthFileList *) data);
	return FALSE;
}


static void
queue_flash_updates (GthFileList *file_list)
{
	file_list->priv->dirty = TRUE;
	if (file_list->priv->dirty_event == 0)
		file_list->priv->dirty_event = g_timeout_add (FLASH_THUMBNAIL_QUEUE_TIMEOUT, flash_queue_cb, file_list);
}


static gboolean
get_file_data_iter_with_suggested_pos (GthFileStore *file_store,
				       GthFileData  *file_data,
				       int           try_pos,
				       GtkTreeIter  *iter_p)
{
	if (gth_file_store_get_nth_visible (file_store, try_pos, iter_p)) {
		GthFileData *nth_file_data;

		nth_file_data = gth_file_store_get_file (file_store, iter_p);
		if (g_file_equal (file_data->file, nth_file_data->file))
			return TRUE;

		if (gth_file_store_find (file_store, file_data->file, iter_p))
			return TRUE;
	}

	return FALSE;
}


static void
update_thumb_in_file_view (GthFileList *file_list,
		    	   GthFileData *file_data,
			   int          try_pos)
{
	GthFileStore *file_store;
	GtkTreeIter   iter;
	ThumbData    *thumb_data;

	file_store = (GthFileStore *) gth_file_view_get_model (GTH_FILE_VIEW (file_list->priv->view));
	if (! get_file_data_iter_with_suggested_pos (file_store, file_data, try_pos, &iter))
		return;

	thumb_data = g_hash_table_lookup (file_list->priv->thumb_data, file_data->file);
	if (thumb_data == NULL)
		return;

	if (thumb_data->pixbuf == NULL)
		return;

	gth_file_store_queue_set (file_store,
				  &iter,
				  GTH_FILE_STORE_THUMBNAIL_COLUMN, thumb_data->pixbuf,
				  GTH_FILE_STORE_IS_ICON_COLUMN, FALSE,
				  -1);
	queue_flash_updates (file_list);
}


static void
set_mime_type_icon (GthFileList *file_list,
		    GthFileData *file_data,
		    int          try_pos)
{
	GthFileStore *file_store;
	GtkTreeIter   iter;
	GIcon        *icon;
	GdkPixbuf    *pixbuf;

	file_store = (GthFileStore *) gth_file_view_get_model (GTH_FILE_VIEW (file_list->priv->view));
	if (! get_file_data_iter_with_suggested_pos (file_store, file_data, try_pos, &iter))
		return;

	icon = g_file_info_get_icon (file_data->info);
	pixbuf = gth_icon_cache_get_pixbuf (file_list->priv->icon_cache, icon);
	gth_file_store_queue_set (file_store,
				  &iter,
				  GTH_FILE_STORE_THUMBNAIL_COLUMN, pixbuf,
				  GTH_FILE_STORE_IS_ICON_COLUMN, TRUE,
				  -1);
	queue_flash_updates (file_list);

	_g_object_unref (pixbuf);
}


static void
thumbnail_job_ready_cb (GObject      *source_object,
		        GAsyncResult *result,
		        gpointer      user_data)
{
	ThumbnailJob *job = user_data;
	GthFileList  *file_list = job->file_list;
	gboolean      success;
	GdkPixbuf    *pixbuf = NULL;
	GError       *error = NULL;
	ThumbData    *thumb_data;

	success = gth_thumb_loader_load_finish (GTH_THUMB_LOADER (source_object),
						result,
						&pixbuf,
						&error);

	if (! success && g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
		thumbnail_job_free (job);
		return;
	}

	thumb_data = g_hash_table_lookup (file_list->priv->thumb_data, job->file_data->file);
	if (thumb_data == NULL) {
		_g_object_unref (pixbuf);
		thumbnail_job_free (job);
		_gth_file_list_update_next_thumb (file_list);
		return;
	}

	_g_object_unref (thumb_data->pixbuf);
	thumb_data->pixbuf = NULL;

	if (! success) {
		thumb_data->thumb_created = FALSE;
		thumb_data->thumb_loaded = FALSE;
		if (job->update_in_view)
			set_mime_type_icon (file_list, job->file_data, job->pos);

		thumb_data->error = TRUE;
	}
	else {
		thumb_data->pixbuf = g_object_ref (pixbuf);
		thumb_data->thumb_created = TRUE;
		thumb_data->error = FALSE;
		if (job->update_in_view) {
			thumb_data->thumb_loaded = TRUE;
			update_thumb_in_file_view (file_list, job->file_data, job->pos);
		}
	}

	_g_object_unref (pixbuf);
	thumbnail_job_free (job);

	_gth_file_list_update_next_thumb (file_list);
}


static void
set_loading_icon (GthFileList *file_list,
		  GthFileData *file_data,
		  int          try_pos)
{
	GthFileStore *file_store;
	GtkTreeIter   iter;
	GIcon        *icon;
	GdkPixbuf    *pixbuf;

	file_store = (GthFileStore *) gth_file_view_get_model (GTH_FILE_VIEW (file_list->priv->view));
	if (! get_file_data_iter_with_suggested_pos (file_store, file_data, try_pos, &iter))
		return;

	icon = g_themed_icon_new ("image-loading");
	pixbuf = gth_icon_cache_get_pixbuf (file_list->priv->icon_cache, icon);
	gth_file_store_queue_set (file_store,
				  &iter,
				  GTH_FILE_STORE_THUMBNAIL_COLUMN, pixbuf,
				  GTH_FILE_STORE_IS_ICON_COLUMN, TRUE,
				  -1);
	queue_flash_updates (file_list);

	_g_object_unref (pixbuf);
	g_object_unref (icon);
}


static gboolean
start_thumbnail_job (gpointer user_data)
{
	ThumbnailJob *job = user_data;

	gth_thumb_loader_load (job->loader,
			       job->file_data,
			       job->cancellable,
			       thumbnail_job_ready_cb,
			       job);

	return FALSE;
}


static void
_gth_file_list_update_thumb (GthFileList  *file_list,
			     ThumbnailJob *job)
{
	GList *scan;

	if (! job->update_in_view) {
		ThumbData *thumb_data;

		thumb_data = g_hash_table_lookup (file_list->priv->thumb_data, job->file_data->file);

		if (gth_thumb_loader_has_valid_thumbnail (file_list->priv->thumb_loader, job->file_data)) {
			thumb_data->thumb_created = TRUE;
			thumb_data->error = FALSE;
			thumbnail_job_free (job);
			job = NULL;
		}
		else if (gth_thumb_loader_has_failed_thumbnail (file_list->priv->thumb_loader, job->file_data)) {
			thumb_data->thumb_created = TRUE;
			thumb_data->error = TRUE;
			thumbnail_job_free (job);
			job = NULL;
		}

		if (job == NULL) {
			g_idle_add (restart_thumb_update_cb, file_list);
			return;
		}
	}

	for (scan = file_list->priv->jobs; scan; scan = scan->next) {
		ThumbnailJob *other_job = scan->data;
		thumbnail_job_cancel (other_job);
	}
	file_list->priv->jobs = g_list_prepend (file_list->priv->jobs, job);

	if (job->update_in_view)
		set_loading_icon (job->file_list, job->file_data, job->pos);
	g_idle_add (start_thumbnail_job, job);
}


static void
_gth_file_list_thumbs_completed (GthFileList *file_list)
{
	flash_queue (file_list);
	_gth_file_list_done (file_list);
}


static gboolean
update_thumbs_stopped (gpointer callback_data)
{
	GthFileList *file_list = callback_data;

	file_list->priv->loading_thumbs = FALSE;
	_gth_file_list_exec_next_op (file_list);

	return FALSE;
}


static gboolean
can_create_file_thumbnail (GthFileData *file_data,
			   ThumbData   *thumb_data,
			   GTimeVal    *current_time,
			   gboolean    *young_file_found)
{

	time_t     time_diff;
	gboolean   young_file;

	/* Check for files that are exactly 0 or 1 seconds old; they may still be changing. */
	time_diff = current_time->tv_sec - gth_file_data_get_mtime (file_data);
	young_file = (time_diff <= 1) && (time_diff >= 0);

	if (young_file)
		*young_file_found = TRUE;

	return ! thumb_data->error && ! young_file;
}


static void
_gth_file_list_update_next_thumb (GthFileList *file_list)
{
	GthFileStore  *file_store;
	int            pos;
	int            first_pos;
	int            last_pos;
	GthFileData   *file_data = NULL;
	ThumbData     *thumb_data;
	GList         *list, *scan;
	int            new_pos = -1;
	GTimeVal       current_time;
	gboolean       young_file_found = FALSE;
	ThumbnailJob  *job;

	if (file_list->priv->queue != NULL) {
		g_idle_add (update_thumbs_stopped, file_list);
		return;
	}

	file_store = (GthFileStore *) gth_file_view_get_model (GTH_FILE_VIEW (file_list->priv->view));

	/* Find first visible undone. */

	list = gth_file_store_get_visibles (file_store);
	first_pos = gth_file_view_get_first_visible (GTH_FILE_VIEW (file_list->priv->view));
	last_pos = gth_file_view_get_last_visible (GTH_FILE_VIEW (file_list->priv->view));

	scan = NULL;
	pos = first_pos;
	if (pos >= 0)
		scan = g_list_nth (list, pos);
	if (scan == NULL) {
		_g_object_list_unref (list);
		_gth_file_list_thumbs_completed (file_list);
		return;
	}

	/* Find a not loaded thumbnail among the visible images. */

	g_get_current_time (&current_time);

	while (pos <= last_pos) {
		file_data = scan->data;
		thumb_data = g_hash_table_lookup (file_list->priv->thumb_data, file_data->file);
		if (! thumb_data->thumb_loaded && can_create_file_thumbnail (file_data, thumb_data, &current_time, &young_file_found)) {
			new_pos = pos;
			break;
		}
		else {
			pos++;
			scan = scan->next;
		}
	}

	if (! file_list->priv->ignore_hidden_thumbs) {

		/* Find a not created thumbnail among the not-visible images. */

		/* start from the one after the last visible image... */

		if (new_pos == -1) {
			pos = last_pos + 1;
			for (scan = g_list_nth (list, pos); scan && (pos <= last_pos + N_CREATEAHEAD); scan = scan->next) {
				gboolean requested_action_performed;

				file_data = scan->data;
				thumb_data = g_hash_table_lookup (file_list->priv->thumb_data, file_data->file);

				if (pos <= last_pos + N_VIEWAHEAD)
					requested_action_performed = thumb_data->thumb_loaded;
				else
					requested_action_performed = thumb_data->thumb_created;

				if (! requested_action_performed && can_create_file_thumbnail (file_data, thumb_data, &current_time, &young_file_found)) {
					new_pos = pos;
					break;
				}
				pos++;
			}
		}

		/* ...continue from the one before the first visible upward to
		 * the first one. */

		if (new_pos == -1) {
			pos = first_pos - 1;
			for (scan = g_list_nth (list, pos); scan && (pos >= first_pos - N_CREATEAHEAD); scan = scan->prev) {
				gboolean requested_action_performed;

				file_data = scan->data;
				thumb_data = g_hash_table_lookup (file_list->priv->thumb_data, file_data->file);

				/* disabled to avoid thumbnail jumps (bug #603642)
				if (pos >= first_pos - N_VIEWAHEAD)
					requested_action_performed = thumb_data->thumb_loaded;
				else*/
					requested_action_performed = thumb_data->thumb_created;

				if (! requested_action_performed && can_create_file_thumbnail (file_data, thumb_data, &current_time, &young_file_found)) {
					new_pos = pos;
					break;
				}
				pos--;
			}
		}
	}

	if (new_pos != -1)
		file_data = g_object_ref (file_data);

	_g_object_list_unref (list);

	if (new_pos == -1) {
		_gth_file_list_thumbs_completed (file_list);
		if (young_file_found && (file_list->priv->restart_thumb_update == 0))
			file_list->priv->restart_thumb_update = g_timeout_add (RESTART_LOADING_THUMBS_DELAY, restart_thumb_update_cb, file_list);
		return;
	}

	job = g_new0 (ThumbnailJob, 1);
	job->file_list = g_object_ref (file_list);
	job->loader = gth_thumb_loader_new (file_list->priv->thumb_size);
	job->cancellable = g_cancellable_new ();
	job->file_data = file_data; /* already ref-ed above */
	job->pos = new_pos;

	/* disabled to avoid thumbnail jumps (bug #603642)
	 job->update_in_view = (new_pos >= (first_pos - N_VIEWAHEAD)) && (new_pos <= (last_pos + N_VIEWAHEAD)); */

	job->update_in_view = (new_pos >= first_pos) && (job->pos <= last_pos + N_VIEWAHEAD);

#if 0
	g_print ("%d in [%d, %d] => %d\n",
 		 job->pos,
		 (first_pos - N_VIEWAHEAD),
		 (last_pos + N_VIEWAHEAD),
		 job->update_in_view);
#endif

	_gth_file_list_update_thumb (file_list, job);
}


static void
_gth_file_list_exec_next_op (GthFileList *file_list)
{
	GList         *first;
	GthFileListOp *op;
	gboolean       exec_next_op = TRUE;

	if ((file_list->priv->queue == NULL) && ! file_list->priv->cancelled) {
		start_update_next_thumb (file_list);
		return;
	}

	first = file_list->priv->queue;
	file_list->priv->queue = g_list_remove_link (file_list->priv->queue, first);

	op = first->data;

	switch (op->type) {
	case GTH_FILE_LIST_OP_TYPE_SET_FILES:
		gfl_set_files (file_list, op->file_list);
		break;
	case GTH_FILE_LIST_OP_TYPE_ADD_FILES:
		gfl_add_files (file_list, op->file_list);
		break;
	case GTH_FILE_LIST_OP_TYPE_DELETE_FILES:
		gfl_delete_files (file_list, op->files);
		break;
	case GTH_FILE_LIST_OP_TYPE_UPDATE_FILES:
		gfl_update_files (file_list, op->file_list);
		break;
	case GTH_FILE_LIST_OP_TYPE_ENABLE_THUMBS:
		gfl_enable_thumbs (file_list, op->ival);
		exec_next_op = FALSE;
		break;
	case GTH_FILE_LIST_OP_TYPE_CLEAR_FILES:
		gfl_clear_list (file_list, op->sval);
		break;
	case GTH_FILE_LIST_OP_TYPE_SET_FILTER:
		gfl_set_filter (file_list, op->filter);
		break;
	case GTH_FILE_LIST_OP_TYPE_SET_SORT_FUNC:
		gfl_set_sort_func (file_list, op->cmp_func, op->inverse_sort);
		break;
	case GTH_FILE_LIST_OP_TYPE_RENAME_FILE:
		gfl_rename_file (file_list, op->file, op->file_data);
		break;
	default:
		exec_next_op = FALSE;
		break;
	}

	gth_file_list_op_free (op);
	g_list_free (first);

	if (exec_next_op)
		_gth_file_list_exec_next_op (file_list);
}


int
gth_file_list_first_file (GthFileList *file_list,
			  gboolean     skip_broken,
			  gboolean     only_selected)
{
	GthFileView *view;
	GList       *files;
	GList       *scan;
	int          pos;

	view = GTH_FILE_VIEW (file_list->priv->view);
	files = gth_file_store_get_visibles (GTH_FILE_STORE (gth_file_view_get_model (view)));

	pos = 0;
	for (scan = files; scan; scan = scan->next, pos++) {
		GthFileData *file_data = scan->data;
		ThumbData   *thumb_data;

		thumb_data = g_hash_table_lookup (file_list->priv->thumb_data, file_data->file);
		if (skip_broken && thumb_data->error)
			continue;
		if (only_selected && ! gth_file_selection_is_selected (GTH_FILE_SELECTION (view), pos))
			continue;

		return pos;
	}

	return -1;
}


int
gth_file_list_last_file (GthFileList *file_list,
			 gboolean     skip_broken,
			 gboolean     only_selected)
{
	GthFileView *view;
	GList       *files;
	GList       *scan;
	int          pos;

	view = GTH_FILE_VIEW (file_list->priv->view);
	files = gth_file_store_get_visibles (GTH_FILE_STORE (gth_file_view_get_model (view)));

	pos = g_list_length (files) - 1;
	if (pos < 0)
		return -1;

	for (scan = g_list_nth (files, pos); scan; scan = scan->prev, pos--) {
		GthFileData *file_data = scan->data;
		ThumbData   *thumb_data;

		thumb_data = g_hash_table_lookup (file_list->priv->thumb_data, file_data->file);
		if (skip_broken && thumb_data->error)
			continue;
		if (only_selected && ! gth_file_selection_is_selected (GTH_FILE_SELECTION (view), pos))
			continue;

		return pos;
	}

	return -1;
}


int
gth_file_list_next_file (GthFileList *file_list,
			 int          pos,
			 gboolean     skip_broken,
			 gboolean     only_selected,
			 gboolean     wrap)
{
	GthFileView *view;
	GList       *files;
	GList       *scan;

	view = GTH_FILE_VIEW (file_list->priv->view);
	files = gth_file_store_get_visibles (GTH_FILE_STORE (gth_file_view_get_model (view)));

	pos++;
	if (pos >= 0)
		scan = g_list_nth (files, pos);
	else if (wrap)
		scan = g_list_first (files);
	else
		scan = NULL;

	for (/* void */; scan; scan = scan->next, pos++) {
		GthFileData *file_data = scan->data;
		ThumbData   *thumb_data;

		thumb_data = g_hash_table_lookup (file_list->priv->thumb_data, file_data->file);
		if (skip_broken && thumb_data->error)
			continue;
		if (only_selected && ! gth_file_selection_is_selected (GTH_FILE_SELECTION (view), pos))
			continue;

		break;
	}

	_g_object_list_unref (files);

	return (scan != NULL) ? pos : -1;
}


int
gth_file_list_prev_file (GthFileList *file_list,
			 int          pos,
			 gboolean     skip_broken,
			 gboolean     only_selected,
			 gboolean     wrap)
{
	GthFileView *view;
	GList       *files;
	GList       *scan;

	view = GTH_FILE_VIEW (file_list->priv->view);
	files = gth_file_store_get_visibles (GTH_FILE_STORE (gth_file_view_get_model (view)));

	pos--;
	if (pos >= 0)
		scan = g_list_nth (files, pos);
	else if (wrap) {
		pos = g_list_length (files) - 1;
		scan = g_list_nth (files, pos);
	}
	else
		scan = NULL;

	for (/* void */; scan; scan = scan->prev, pos--) {
		GthFileData *file_data = scan->data;
		ThumbData   *thumb_data;

		thumb_data = g_hash_table_lookup (file_list->priv->thumb_data, file_data->file);
		if (skip_broken && thumb_data->error)
			continue;
		if (only_selected && ! gth_file_selection_is_selected (GTH_FILE_SELECTION (view), pos))
			continue;

		break;
	}

	_g_object_list_unref (files);

	return (scan != NULL) ? pos : -1;
}

