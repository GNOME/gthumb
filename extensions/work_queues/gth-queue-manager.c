/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2012 Free Software Foundation, Inc.
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
#include <glib.h>
#include <gtk/gtk.h>
#include "gth-queue-manager.h"


#define N_QUEUES 3


struct _GthQueueManagerPrivate {
	GList  *files[N_QUEUES];
	GMutex *mutex;
};


G_DEFINE_TYPE (GthQueueManager,
	       gth_queue_manager,
	       G_TYPE_OBJECT)


static GthQueueManager *the_manager = NULL;


static GObject *
gth_queue_manager_constructor (GType                  type,
			       guint                  n_construct_params,
			       GObjectConstructParam *construct_params)
{
	static GObject *object = NULL;

	if (the_manager == NULL) {
		object = G_OBJECT_CLASS (gth_queue_manager_parent_class)->constructor (type, n_construct_params, construct_params);
		the_manager = GTH_QUEUE_MANAGER (object);
	}
	else
		object =  G_OBJECT (the_manager);

	return object;
}


static void
gth_queue_manager_finalize (GObject *object)
{
	GthQueueManager *self;
	int              i;

	self = GTH_QUEUE_MANAGER (object);

	for (i = 0; i < N_QUEUES; i++)
		_g_object_list_unref (self->priv->files[i]);
	g_mutex_free (self->priv->mutex);

	G_OBJECT_CLASS (gth_queue_manager_parent_class)->finalize (object);
}


static void
gth_queue_manager_class_init (GthQueueManagerClass *klass)
{
	GObjectClass *object_class;

	g_type_class_add_private (klass, sizeof (GthQueueManagerPrivate));

	object_class = (GObjectClass*) klass;
	object_class->constructor = gth_queue_manager_constructor;
	object_class->finalize = gth_queue_manager_finalize;
}

static void
gth_queue_manager_init (GthQueueManager *self)
{
	int i;

	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_QUEUE_MANAGER, GthQueueManagerPrivate);
	self->priv->mutex = g_mutex_new ();
	for (i = 0; i < N_QUEUES; i++)
		self->priv->files[i] = NULL;
}


static GthQueueManager *
gth_queue_manager_get_default (void)
{
	return (GthQueueManager*) g_object_new (GTH_TYPE_QUEUE_MANAGER, NULL);
}


/* -- gth_queue_manager_for_each_child -- */


typedef struct {
	GthQueueManager      *queue_manager;
	GList                *files;
	GList                *current_file;
	char                 *attributes;
	GCancellable         *cancellable;
	ForEachChildCallback  for_each_file_func;
	ReadyCallback         ready_callback;
	gpointer              user_data;
} ForEachChildData;


static void
fec_data_free (ForEachChildData *data)
{
	_g_object_list_unref (data->files);
	g_free (data->attributes);
	_g_object_unref (data->cancellable);
	g_free (data);
}


static void
queue_manager_fec_done (ForEachChildData *data,
			GError           *error)
{
	if (data->ready_callback != NULL)
		data->ready_callback (NULL, error, data->user_data);
	fec_data_free (data);
}


static void
fec__file_info_ready_cb (GObject      *source_object,
			 GAsyncResult *result,
			 gpointer      user_data)
{
	ForEachChildData *data = user_data;
	GFile            *file;
	GFileInfo        *info;

	file = (GFile*) source_object;
	info = g_file_query_info_finish (file, result, NULL);
	if (info != NULL) {
		if (data->for_each_file_func != NULL)
			data->for_each_file_func (file, info, data->user_data);
		g_object_unref (info);
	}

	data->current_file = data->current_file->next;
	if (data->current_file == NULL) {
		queue_manager_fec_done (data, NULL);
		return;
	}

	g_file_query_info_async ((GFile *) data->current_file->data,
				 data->attributes,
				 0,
				 G_PRIORITY_DEFAULT,
				 data->cancellable,
				 fec__file_info_ready_cb,
				 data);

}


static void
queue_manager_fec_done_cb (GObject  *object,
			   GError   *error,
			   gpointer  user_data)
{
	queue_manager_fec_done (user_data, NULL);
}


static int
_g_file_get_n_queue (GFile *file)
{
	char *uri;
	int   n = -1;

	uri = g_file_get_uri (file);
	if (! g_str_has_prefix (uri, "queue:///"))
		n = -1;
	else if (strcmp (uri, "queue:///") == 0)
		n = 0;
	else
		n = atoi (uri + strlen ("queue:///"));

	g_free (uri);

	return n;
}


void
gth_queue_manager_update_file_info (GFile     *file,
				    GFileInfo *info)
{
	int   n_queue;
	char *display_name;

	n_queue = _g_file_get_n_queue (file);

	g_file_info_set_file_type (info, G_FILE_TYPE_DIRECTORY);
	g_file_info_set_content_type (info, "gthumb/queue");
	g_file_info_set_icon (info, g_themed_icon_new ("work-queue"));
	g_file_info_set_sort_order (info, n_queue);
	g_file_info_set_attribute_boolean (info, G_FILE_ATTRIBUTE_ACCESS_CAN_READ, TRUE);
	g_file_info_set_attribute_boolean (info, G_FILE_ATTRIBUTE_ACCESS_CAN_DELETE, FALSE);
	g_file_info_set_attribute_boolean (info, G_FILE_ATTRIBUTE_ACCESS_CAN_RENAME, FALSE);
	g_file_info_set_attribute_int32 (info, "gthumb::n-queue", n_queue);
	if (n_queue > 0) {
		g_file_info_set_attribute_boolean (info, "gthumb::no-child", TRUE);
		display_name = g_strdup_printf (_("Selection %d"), n_queue);
	}
	else if (n_queue == 0)
		display_name = g_strdup (_("Selections"));
	else
		display_name = g_strdup ("???");
	g_file_info_set_display_name (info, display_name);

	g_free (display_name);
}


static void
_gth_queue_manager_for_each_queue (gpointer user_data)
{
	ForEachChildData *data = user_data;
	int               i;

	for (i = 0; i < N_QUEUES; i++) {
		char      *uri;
		GFile     *file;
		GFileInfo *info;

		uri = g_strdup_printf ("queue:///%d", i + 1);
		file = g_file_new_for_uri (uri);
		info = g_file_info_new ();
		gth_queue_manager_update_file_info (file, info);
		data->for_each_file_func (file, info, data->user_data);

		g_object_unref (info);
		g_object_unref (file);
		g_free (uri);
	}

	object_ready_with_error (data->queue_manager,
				 data->ready_callback,
				 data->user_data,
				 NULL);
	fec_data_free (data);
}


void
gth_queue_manager_for_each_child (GFile                *folder,
				  const char           *attributes,
				  GCancellable         *cancellable,
				  ForEachChildCallback  for_each_file_func,
				  ReadyCallback         ready_callback,
				  gpointer              user_data)
{
	GthQueueManager  *self;
	int               n_queue;
	ForEachChildData *data;

	self = gth_queue_manager_get_default ();
	n_queue = _g_file_get_n_queue (folder);

	g_mutex_lock (self->priv->mutex);
	data = g_new0 (ForEachChildData, 1);
	data->queue_manager = self;
	if (n_queue > 0)
		data->files = _g_object_list_ref (self->priv->files[n_queue - 1]);
	data->current_file = data->files;
	data->attributes = g_strdup (attributes);
	data->cancellable = _g_object_ref(cancellable);
	data->for_each_file_func = for_each_file_func;
	data->ready_callback = ready_callback;
	data->user_data = user_data;
	g_mutex_unlock (self->priv->mutex);

	if (n_queue == 0) {
		call_when_idle (_gth_queue_manager_for_each_queue, data);
	}
	else if (data->current_file != NULL)
		g_file_query_info_async ((GFile *) data->current_file->data,
					 data->attributes,
					 0,
					 G_PRIORITY_DEFAULT,
					 data->cancellable,
					 fec__file_info_ready_cb,
					 data);
	else
		object_ready_with_error (NULL, queue_manager_fec_done_cb, data, NULL);
}


gboolean
gth_queue_manager_add_files (GFile *folder,
			     GList *file_list, /* GFile list */
			     int    destination_position)
{
	GthQueueManager *self;
	int              n_queue;
	GList           *new_list;
	GList           *link;

	if (! g_file_has_uri_scheme (folder, "queue"))
		return FALSE;

	self = gth_queue_manager_get_default ();
	n_queue = _g_file_get_n_queue (folder);
	if (n_queue <= 0)
		return FALSE;

	g_mutex_lock (self->priv->mutex);

	new_list = _g_file_list_dup (file_list);
	link = g_list_nth (self->priv->files[n_queue - 1], destination_position);
	if (link != NULL) {
		GList *last_new;

		/* insert 'new_list' before 'link' */

		if (link->prev != NULL)
			link->prev->next = new_list;
		new_list->prev = link->prev;

		last_new = g_list_last (new_list);
		last_new->next = link;
		link->prev = last_new;
	}
	else
		self->priv->files[n_queue - 1] = g_list_concat (self->priv->files[n_queue - 1], new_list);

	gth_monitor_folder_changed (gth_main_get_default_monitor (),
				    folder,
				    file_list,
				    GTH_MONITOR_EVENT_CREATED);

	g_mutex_unlock (self->priv->mutex);

	return TRUE;
}


void
gth_queue_manager_remove_files (GFile *folder,
				GList *file_list)
{
	GthQueueManager *self;
	int              n_queue;
	GHashTable      *files_to_remove;
	GList           *scan;
	GList           *new_list;

	self = gth_queue_manager_get_default ();
	n_queue = _g_file_get_n_queue (folder);
	if (n_queue <= 0)
		return;

	g_mutex_lock (self->priv->mutex);

	files_to_remove = g_hash_table_new (g_file_hash, (GEqualFunc) g_file_equal);
	for (scan = file_list; scan; scan = scan->next)
		g_hash_table_insert (files_to_remove, scan->data, GINT_TO_POINTER (1));

	new_list = NULL;
	for (scan = self->priv->files[n_queue - 1]; scan; scan = scan->next) {
		GFile *file = scan->data;

		if (g_hash_table_lookup (files_to_remove, file))
			continue;

		new_list = g_list_prepend (new_list, g_object_ref (file));
	}
	new_list = g_list_reverse (new_list);

	g_hash_table_unref (files_to_remove);

	_g_object_list_unref (self->priv->files[n_queue - 1]);
	self->priv->files[n_queue - 1] = new_list;

	gth_monitor_folder_changed (gth_main_get_default_monitor (),
				    folder,
				    file_list,
				    GTH_MONITOR_EVENT_REMOVED);

	g_mutex_unlock (self->priv->mutex);
}


void
gth_queue_manager_reorder (GFile *folder,
			   GList *visible_files, /* GFile list */
			   GList *files_to_move, /* GFile list */
			   int    dest_pos)
{
	/* FIXME */
}
