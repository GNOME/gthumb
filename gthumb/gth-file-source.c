/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2008-2009 Free Software Foundation, Inc.
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
#include <glib.h>
#include <gtk/gtk.h>
#include "gth-file-data.h"
#include "glib-utils.h"
#include "gth-file-source.h"
#include "gth-main.h"


struct _GthFileSourcePrivate
{
	GList    *schemes;
	gboolean  active;
	GList    *queue;
};


static GObjectClass *parent_class = NULL;


/* -- queue -- */


typedef enum {
	FILE_SOURCE_OP_LIST,
	FILE_SOURCE_OP_READ_ATTRIBUTES,
	FILE_SOURCE_OP_RENAME,
	FILE_SOURCE_OP_COPY
} FileSourceOp;


typedef struct {
	GFile      *folder;
	const char *attributes;
	ListReady   func;
	gpointer    data;
} ListData;


typedef struct {
	GList      *files;
	const char *attributes;
	ListReady   func;
	gpointer    data;
} ReadAttributesData;


typedef struct {
	GFile         *file;
	GFile         *new_file;
	ReadyCallback  callback;
	gpointer       data;
} RenameData;


typedef struct {
	GFile         *destination;
	GList         *file_list;
	ReadyCallback  callback;
	gpointer       data;
} CopyData;


typedef struct {
	GthFileSource *file_source;
	FileSourceOp   op;
	union {
		ListData           list;
		ReadAttributesData read_attributes;
		RenameData         rename;
		CopyData           copy;
	} data;
} FileSourceAsyncOp;


static void
file_source_async_op_free (FileSourceAsyncOp *async_op)
{
	switch (async_op->op) {
	case FILE_SOURCE_OP_LIST:
		g_object_unref (async_op->data.list.folder);
		break;
	case FILE_SOURCE_OP_READ_ATTRIBUTES:
		_g_object_list_unref (async_op->data.read_attributes.files);
		break;
	case FILE_SOURCE_OP_RENAME:
		g_object_unref (async_op->data.rename.file);
		g_object_unref (async_op->data.rename.new_file);
		break;
	case FILE_SOURCE_OP_COPY:
		g_object_unref (async_op->data.copy.destination);
		_g_object_list_unref (async_op->data.copy.file_list);
		break;
	}

	g_free (async_op);
}


static void
gth_file_source_queue_list (GthFileSource *file_source,
			    GFile         *folder,
			    const char    *attributes,
			    ListReady      func,
			    gpointer       data)
{
	FileSourceAsyncOp *async_op;

	async_op = g_new0 (FileSourceAsyncOp, 1);
	async_op->file_source = file_source;
	async_op->op = FILE_SOURCE_OP_LIST;
	async_op->data.list.folder = g_file_dup (folder);
	async_op->data.list.attributes = attributes;
	async_op->data.list.func = func;
	async_op->data.list.data = data;

	file_source->priv->queue = g_list_append (file_source->priv->queue, async_op);
}


static void
gth_file_source_queue_read_attributes (GthFileSource *file_source,
				       GList         *files,
				       const char    *attributes,
				       ListReady      func,
				       gpointer       data)
{
	FileSourceAsyncOp *async_op;

	async_op = g_new0 (FileSourceAsyncOp, 1);
	async_op->file_source = file_source;
	async_op->op = FILE_SOURCE_OP_READ_ATTRIBUTES;
	async_op->data.read_attributes.files = _g_object_list_ref (files);
	async_op->data.read_attributes.attributes = attributes;
	async_op->data.read_attributes.func = func;
	async_op->data.read_attributes.data = data;

	file_source->priv->queue = g_list_append (file_source->priv->queue, async_op);
}


static void
gth_file_source_queue_rename (GthFileSource *file_source,
			      GFile         *file,
			      GFile         *new_file,
			      ReadyCallback  callback,
			      gpointer       data)
{
	FileSourceAsyncOp *async_op;

	async_op = g_new0 (FileSourceAsyncOp, 1);
	async_op->file_source = file_source;
	async_op->op = FILE_SOURCE_OP_RENAME;
	async_op->data.rename.file = g_file_dup (file);
	async_op->data.rename.new_file = g_file_dup (new_file);
	async_op->data.rename.callback = callback;
	async_op->data.rename.data = data;

	file_source->priv->queue = g_list_append (file_source->priv->queue, async_op);
}


static void
gth_file_source_queue_copy (GthFileSource *file_source,
			    GFile          *destination,
			    GList          *file_list,
			    ReadyCallback   callback,
			    gpointer        data)
{
	FileSourceAsyncOp *async_op;

	async_op = g_new0 (FileSourceAsyncOp, 1);
	async_op->file_source = file_source;
	async_op->op = FILE_SOURCE_OP_COPY;
	async_op->data.copy.destination = g_file_dup (destination);
	async_op->data.copy.file_list = _g_file_list_dup (file_list);
	async_op->data.copy.callback = callback;
	async_op->data.copy.data = data;

	file_source->priv->queue = g_list_append (file_source->priv->queue, async_op);
}


static void
gth_file_source_exec_next_in_queue (GthFileSource *file_source)
{
	GList             *head;
	FileSourceAsyncOp *async_op;

	if (file_source->priv->queue == NULL)
		return;

	head = file_source->priv->queue;
	file_source->priv->queue = g_list_remove_link (file_source->priv->queue, head);

	async_op = head->data;
	switch (async_op->op) {
	case FILE_SOURCE_OP_LIST:
		gth_file_source_list (file_source,
				      async_op->data.list.folder,
				      async_op->data.list.attributes,
				      async_op->data.list.func,
				      async_op->data.list.data);
		break;
	case FILE_SOURCE_OP_READ_ATTRIBUTES:
		gth_file_source_read_attributes (file_source,
						 async_op->data.read_attributes.files,
						 async_op->data.read_attributes.attributes,
						 async_op->data.read_attributes.func,
						 async_op->data.read_attributes.data);
		break;
	case FILE_SOURCE_OP_RENAME:
		gth_file_source_rename (file_source,
					async_op->data.rename.file,
					async_op->data.rename.new_file,
					async_op->data.rename.callback,
					async_op->data.rename.data);
		break;
	case FILE_SOURCE_OP_COPY:
		gth_file_source_copy (file_source,
				      async_op->data.copy.destination,
				      async_op->data.copy.file_list,
				      async_op->data.copy.callback,
				      async_op->data.copy.data);
		break;
	}

	file_source_async_op_free (async_op);
	g_list_free (head);
}


static void
gth_file_source_clear_queue (GthFileSource  *file_source)
{
	g_list_foreach (file_source->priv->queue, (GFunc) file_source_async_op_free, NULL);
	g_list_free (file_source->priv->queue);
	file_source->priv->queue = NULL;
}


/* -- */


static GList *
base_get_entry_points (GthFileSource  *file_source)
{
	return NULL;
}


static GList *
base_get_current_list (GthFileSource  *file_source,
		       GFile          *file)
{
	GList *list = NULL;
	GFile *parent;

	if (file == NULL)
		return NULL;

	parent = g_file_dup (file);
	while (parent != NULL) {
		GFile *tmp;

		list = g_list_prepend (list, g_object_ref (parent));
		tmp = g_file_get_parent (parent);
		g_object_unref (parent);
		parent = tmp;
	}

	return g_list_reverse (list);
}


static GFile *
base_to_gio_file (GthFileSource *file_source,
		  GFile         *file)
{
	return g_file_dup (file);
}


static GFileInfo *
base_get_file_info (GthFileSource *file_source,
		    GFile         *file)
{
	return NULL;
}


static void
base_list (GthFileSource *file_source,
	   GFile         *folder,
	   const char    *attributes,
	   ListReady      func,
	   gpointer       data)
{
	/* void */
}


static void
base_read_attributes (GthFileSource *file_source,
		      GList         *files,
		      const char    *attributes,
		      ListReady      func,
		      gpointer       data)
{
	/* void */
}


static void
base_cancel (GthFileSource *file_source)
{
	/* void */
}


static void
base_rename (GthFileSource *file_source,
	     GFile         *file,
	     GFile         *new_file,
	     ReadyCallback  callback,
	     gpointer       data)
{
	GFile  *source;
	GFile  *destination;
	GError *error = NULL;

	source = gth_file_source_to_gio_file (file_source, file);
	destination = gth_file_source_to_gio_file (file_source, new_file);

	if (g_file_move (source, destination, 0, NULL, NULL, NULL, &error)) {
		GthMonitor *monitor;

		monitor = gth_main_get_default_monitor ();
		gth_monitor_file_renamed (monitor, file, new_file);
	}
	object_ready_with_error (file_source, callback, data, error);

	g_object_unref (destination);
	g_object_unref (source);
}


static gboolean
base_can_cut (GthFileSource *file_source)
{
	return FALSE;
}


static void
base_monitor_entry_points (GthFileSource *file_source)
{
	/* void */
}


static void
base_monitor_directory (GthFileSource  *file_source,
			GFile          *file,
			gboolean        activate)
{
	/* void */
}


static void
gth_file_source_finalize (GObject *object)
{
	GthFileSource *file_source = GTH_FILE_SOURCE (object);

	if (file_source->priv != NULL) {
		gth_file_source_clear_queue (file_source);
		_g_string_list_free (file_source->priv->schemes);

		g_free (file_source->priv);
		file_source->priv = NULL;
	}

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gth_file_source_class_init (GthFileSourceClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;

	object_class->finalize = gth_file_source_finalize;
	class->get_entry_points = base_get_entry_points;
	class->get_current_list = base_get_current_list;
	class->to_gio_file = base_to_gio_file;
	class->get_file_info = base_get_file_info;
	class->list = base_list;
	class->read_attributes = base_read_attributes;
	class->cancel = base_cancel;
	class->rename = base_rename;
	class->can_cut = base_can_cut;
	class->monitor_entry_points = base_monitor_entry_points;
	class->monitor_directory = base_monitor_directory;
}


static void
gth_file_source_init (GthFileSource *file_source)
{
	file_source->priv = g_new0 (GthFileSourcePrivate, 1);
}


GType
gth_file_source_get_type (void)
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (GthFileSourceClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_file_source_class_init,
			NULL,
			NULL,
			sizeof (GthFileSource),
			0,
			(GInstanceInitFunc) gth_file_source_init
		};

		type = g_type_register_static (G_TYPE_OBJECT,
					       "GthFileSource",
					       &type_info,
					       0);
	}

	return type;
}


void
gth_file_source_add_scheme (GthFileSource  *file_source,
			    const char     *scheme)
{
	file_source->priv->schemes = g_list_prepend (file_source->priv->schemes, g_strdup (scheme));
}


gboolean
gth_file_source_supports_scheme (GthFileSource *file_source,
				 const char    *uri)
{
	gboolean  result = FALSE;
	GList    *scan;

	for (scan = file_source->priv->schemes; scan; scan = scan->next) {
		const char *scheme = scan->data;

		if (strncmp (uri, scheme, strlen (scheme)) == 0) {
			result = TRUE;
			break;
		}
	}

	return result;
}


void
gth_file_source_set_active (GthFileSource *file_source,
			    gboolean       active)
{
	file_source->priv->active = active;
	if (! active)
		gth_file_source_exec_next_in_queue (file_source);
}


GList *
gth_file_source_get_entry_points (GthFileSource *file_source)
{
	return GTH_FILE_SOURCE_GET_CLASS (G_OBJECT (file_source))->get_entry_points (file_source);
}


GList *
gth_file_source_get_current_list (GthFileSource *file_source,
				  GFile         *file)
{
	return GTH_FILE_SOURCE_GET_CLASS (G_OBJECT (file_source))->get_current_list (file_source, file);
}


GFile *
gth_file_source_to_gio_file (GthFileSource *file_source,
			     GFile         *file)
{
	return GTH_FILE_SOURCE_GET_CLASS (G_OBJECT (file_source))->to_gio_file (file_source, file);
}


GList *
gth_file_source_to_gio_file_list (GthFileSource *file_source,
				  GList         *files)
{
	GList *gio_files = NULL;
	GList *scan;

	for (scan = files; scan; scan = scan->next)
		gio_files = g_list_prepend (gio_files, gth_file_source_to_gio_file (file_source, (GFile *) scan->data));

	return g_list_reverse (gio_files);
}


GFileInfo *
gth_file_source_get_file_info (GthFileSource *file_source,
			       GFile         *file)
{
	return GTH_FILE_SOURCE_GET_CLASS (G_OBJECT (file_source))->get_file_info (file_source, file);
}


gboolean
gth_file_source_is_active (GthFileSource *file_source)
{
	return file_source->priv->active;
}


void
gth_file_source_cancel (GthFileSource *file_source)
{
	gth_file_source_clear_queue (file_source);
	GTH_FILE_SOURCE_GET_CLASS (G_OBJECT (file_source))->cancel (file_source);
}


void
gth_file_source_list (GthFileSource *file_source,
		      GFile         *folder,
		      const char    *attributes,
		      ListReady      func,
		      gpointer       data)
{
	if (gth_file_source_is_active (file_source)) {
		gth_file_source_queue_list (file_source, folder, attributes, func, data);
		return;
	}
	GTH_FILE_SOURCE_GET_CLASS (G_OBJECT (file_source))->list (file_source, folder, attributes, func, data);
}


void
gth_file_source_read_attributes (GthFileSource  *file_source,
				 GList          *files,
				 const char     *attributes,
				 ListReady       func,
				 gpointer        data)
{
	if (gth_file_source_is_active (file_source)) {
		gth_file_source_queue_read_attributes (file_source, files, attributes, func, data);
		return;
	}
	GTH_FILE_SOURCE_GET_CLASS (G_OBJECT (file_source))->read_attributes (file_source, files, attributes, func, data);
}


void
gth_file_source_rename (GthFileSource  *file_source,
			GFile          *file,
			GFile          *new_file,
			ReadyCallback   callback,
			gpointer        data)
{
	if (gth_file_source_is_active (file_source)) {
		gth_file_source_queue_rename (file_source, file, new_file, callback, data);
		return;
	}
	GTH_FILE_SOURCE_GET_CLASS (G_OBJECT (file_source))->rename (file_source, file, new_file, callback, data);
}


void
gth_file_source_copy (GthFileSource  *file_source,
		      GFile          *destination,
		      GList          *file_list, /* GFile * list */
		      ReadyCallback   callback,
		      gpointer        data)
{
	if (gth_file_source_is_active (file_source)) {
		gth_file_source_queue_copy (file_source, destination, file_list, callback, data);
		return;
	}
	GTH_FILE_SOURCE_GET_CLASS (G_OBJECT (file_source))->copy (file_source, destination, file_list, callback, data);
}


gboolean
gth_file_source_can_cut (GthFileSource *file_source)
{
	return GTH_FILE_SOURCE_GET_CLASS (G_OBJECT (file_source))->can_cut (file_source);
}


void
gth_file_source_monitor_entry_points (GthFileSource *file_source)
{
	GTH_FILE_SOURCE_GET_CLASS (G_OBJECT (file_source))->monitor_entry_points (file_source);
}


void
gth_file_source_monitor_directory (GthFileSource  *file_source,
				   GFile          *file,
				   gboolean        activate)
{
	GTH_FILE_SOURCE_GET_CLASS (G_OBJECT (file_source))->monitor_directory (file_source, file, activate);
}
