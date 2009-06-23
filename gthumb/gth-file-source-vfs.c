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
#include <gio/gunixmounts.h>
#include "file-cache.h"
#include "gth-file-data.h"
#include "gio-utils.h"
#include "glib-utils.h"
#include "gth-file-source-vfs.h"
#include "gth-main.h"

#define GTH_MONITOR_N_EVENTS 3
#define MONITOR_UPDATE_DELAY 500
#define DEBUG_MONITOR 1

struct _GthFileSourceVfsPrivate
{
	GCancellable      *cancellable;
	GList             *files;
	ListReady          ready_func;
	gpointer           ready_data;
	GHashTable        *monitors;
	GList             *monitor_queue[GTH_MONITOR_N_EVENTS];
	guint              monitor_update_id;
	GUnixMountMonitor *mount_monitor;
};


static GthFileSourceClass *parent_class = NULL;
static guint mount_monitor_id = 0;


static GList *
get_entry_points (GthFileSource *file_source)
{
	GList     *list;
	GFile     *file;
	GFileInfo *info;
	GList     *mounts;
	GList     *scan;

	list = NULL;

	file = g_file_new_for_uri (get_home_uri ());
	info = gth_file_source_get_file_info (file_source, file);
	g_file_info_set_display_name (info, _("Home Folder"));
	list = g_list_append (list, gth_file_data_new (file, info));
	g_object_unref (info);
	g_object_unref (file);

	file = g_file_new_for_uri ("file:///");
	info = gth_file_source_get_file_info (file_source, file);
	g_file_info_set_display_name (info, _("File System"));
	list = g_list_append (list, gth_file_data_new (file, info));
	g_object_unref (info);
	g_object_unref (file);

	mounts = g_volume_monitor_get_mounts (g_volume_monitor_get ());
	for (scan = mounts; scan; scan = scan->next) {
		GMount  *mount = scan->data;
		GVolume *volume;

		if (g_mount_is_shadowed (mount))
			continue;

		file = g_mount_get_root (mount);
		info = gth_file_source_get_file_info (file_source, file);

		volume = g_mount_get_volume (mount);
		if (volume != NULL) {
			char  *name;
			GIcon *icon;

			name = g_volume_get_name (volume);
			g_file_info_set_display_name (info, name);

			icon = g_volume_get_icon (volume);
			g_file_info_set_icon (info, icon);

			g_object_unref (icon);
			g_free (name);
			g_object_unref (volume);
		}

		list = g_list_append (list, gth_file_data_new (file, info));
		g_object_unref (info);
		g_object_unref (file);
	}

	g_list_foreach (mounts, (GFunc) g_object_unref, NULL);
	g_list_free (mounts);

	return list;
}


static GFile *
to_gio_file (GthFileSource *file_source,
	     GFile         *file)
{
	char  *uri;
	GFile *gio_file;

	uri = g_file_get_uri (file);
	gio_file = g_file_new_for_uri (g_str_has_prefix (uri, "vfs+") ? uri + 4 : uri);
	g_free (uri);

	return gio_file;
}


static GFileInfo *
get_file_info (GthFileSource *file_source,
	       GFile         *file)
{
	GFile     *gio_file;
	GFileInfo *file_info;

	gio_file = gth_file_source_to_gio_file (file_source, file);
	file_info = g_file_query_info (gio_file,
				       "standard::display-name,standard::icon,standard::type",
				       G_FILE_QUERY_INFO_NONE,
				       NULL,
				       NULL);

	g_object_unref (gio_file);

	return file_info;
}


static void
list__done_func (GError   *error,
		 gpointer  user_data)
{
	GthFileSourceVfs *file_source_vfs = user_data;

	if (G_IS_OBJECT (file_source_vfs))
		gth_file_source_set_active (GTH_FILE_SOURCE (file_source_vfs), FALSE);

	file_source_vfs->priv->ready_func ((GthFileSource *)file_source_vfs,
					   file_source_vfs->priv->files,
					   error,
					   file_source_vfs->priv->ready_data);
	g_object_unref (file_source_vfs);
}


static void
list__for_each_file_func (GFile     *file,
			  GFileInfo *info,
			  gpointer   user_data)
{
	GthFileSourceVfs *file_source_vfs = user_data;

	switch (g_file_info_get_file_type (info)) {
	case G_FILE_TYPE_REGULAR:
	case G_FILE_TYPE_DIRECTORY:
		file_source_vfs->priv->files = g_list_prepend (file_source_vfs->priv->files, gth_file_data_new (file, info));
		break;
	default:
		break;
	}
}


static DirOp
list__start_dir_func (GFile       *directory,
		      GFileInfo   *info,
		      GError     **error,
		      gpointer     user_data)
{
	return DIR_OP_CONTINUE;
}


static void
list (GthFileSource *file_source,
      GFile         *folder,
      const char    *attributes,
      ListReady      func,
      gpointer       user_data)
{
	GthFileSourceVfs *file_source_vfs = (GthFileSourceVfs *) file_source;
	GFile            *gio_folder;

	gth_file_source_set_active (file_source, TRUE);
	g_cancellable_reset (file_source_vfs->priv->cancellable);

	_g_object_list_unref (file_source_vfs->priv->files);
	file_source_vfs->priv->files = NULL;

	file_source_vfs->priv->ready_func = func;
	file_source_vfs->priv->ready_data = user_data;

	g_object_ref (file_source);
	gio_folder = gth_file_source_to_gio_file (file_source, folder);
	g_directory_foreach_child (gio_folder,
				   FALSE,
				   TRUE,
				   attributes,
				   file_source_vfs->priv->cancellable,
				   list__start_dir_func,
				   list__for_each_file_func,
				   list__done_func,
				   file_source);

	g_object_unref (gio_folder);
}


static void
info_ready_cb (GList    *files,
	       GError   *error,
	       gpointer  user_data)
{
	GthFileSourceVfs *file_source_vfs = user_data;
	GList            *scan;
	GList            *result_files;

	if (G_IS_OBJECT (file_source_vfs))
		gth_file_source_set_active (GTH_FILE_SOURCE (file_source_vfs), FALSE);

	result_files = NULL;
	for (scan = files; scan; scan = scan->next)
		result_files = g_list_prepend (result_files, g_object_ref ((GthFileData *) scan->data));
	result_files = g_list_reverse (result_files);

	file_source_vfs->priv->ready_func ((GthFileSource *) file_source_vfs,
					   result_files,
					   error,
					   file_source_vfs->priv->ready_data);

	_g_object_list_unref (result_files);
	g_object_unref (file_source_vfs);
}


static void
read_attributes (GthFileSource *file_source,
		 GList         *files,
		 const char    *attributes,
		 ListReady      func,
		 gpointer       user_data)
{
	GthFileSourceVfs *file_source_vfs = (GthFileSourceVfs *) file_source;

	gth_file_source_set_active (file_source, TRUE);
	g_cancellable_reset (file_source_vfs->priv->cancellable);

	file_source_vfs->priv->ready_func = func;
	file_source_vfs->priv->ready_data = user_data;

	g_object_ref (file_source_vfs);
	g_query_info_async (files,
			    attributes,
			    file_source_vfs->priv->cancellable,
			    info_ready_cb,
			    file_source_vfs);
}


static void
cancel (GthFileSource *file_source)
{
	GthFileSourceVfs *file_source_vfs = (GthFileSourceVfs *) file_source;

	g_cancellable_cancel (file_source_vfs->priv->cancellable);
}


/* -- gth_file_source_vfs_copy -- */


typedef struct {
	GthFileSourceVfs *file_source;
	GFile            *destination;
	GList            *file_list;
	ReadyCallback     callback;
	gpointer          user_data;
	GList            *files;
	GList            *dirs;
	GList            *current_dir;
} CopyOpData;


static void
copy_op_data_free (CopyOpData *cod)
{
	g_object_unref (cod->file_source);
	g_object_unref (cod->destination);
	_g_object_list_unref (cod->file_list);
	_g_object_list_unref (cod->files);
	_g_object_list_unref (cod->dirs);
	g_free (cod);
}


static void
copy__copy_files_done (GError   *error,
		       gpointer  user_data)
{
	CopyOpData *cod = user_data;

	cod->callback (G_OBJECT (cod->file_source), error, cod->user_data);
	copy_op_data_free (cod);
}


static void
copy__copy_files (CopyOpData *cod)
{
	GList *destinations;
	GList *scan;

	destinations = NULL;
	for (scan = cod->files; scan; scan = scan->next) {
		GFile *source = scan->data;
		char  *source_basename;

		source_basename = g_file_get_basename (source);
		destinations = g_list_prepend (destinations, g_file_get_child (cod->destination, source_basename));

		g_free (source_basename);
	}

	g_copy_files_async  (cod->files,
			     destinations,
			     G_FILE_COPY_NONE,
			     G_PRIORITY_DEFAULT,
			     cod->file_source->priv->cancellable,
			     NULL,
			     NULL,
			     copy__copy_files_done,
			     cod);

	_g_object_list_unref (destinations);
}


static void copy__copy_current_dir (CopyOpData *cod);


static void
copy__copy_current_dir_done (GError   *error,
			     gpointer  user_data)
{
	CopyOpData *cod = user_data;

	if (error != NULL) {
		cod->callback (G_OBJECT (cod->file_source), error, cod->user_data);
		copy_op_data_free (cod);
		return;
	}

	cod->current_dir = cod->current_dir->next;
	copy__copy_current_dir (cod);
}


static void
copy__copy_current_dir (CopyOpData *cod)
{
	GFile *source;
	char  *source_basename;
	GFile *destination;

	if (cod->current_dir == NULL) {
		copy__copy_files (cod);
		return;
	}

	source = (GFile *) cod->current_dir->data;
	source_basename = g_file_get_basename (source);
	destination = g_file_get_child (cod->destination, source_basename);

	g_directory_copy_async (source,
				destination,
				G_FILE_COPY_NONE,
				G_PRIORITY_DEFAULT,
				cod->file_source->priv->cancellable,
				NULL,
				NULL,
				copy__copy_current_dir_done,
				cod);

	g_object_unref (destination);
	g_free (source_basename);
}


static void
copy__file_list_info_ready_cb (GList    *files,
			       GError   *error,
			       gpointer  user_data)
{
	CopyOpData *cod = user_data;
	GList      *scan;

	for (scan = files; scan; scan = scan->next) {
		GthFileData *file_data = scan->data;

		switch (g_file_info_get_file_type (file_data->info)) {
		case G_FILE_TYPE_DIRECTORY:
			cod->dirs = g_list_prepend (cod->dirs, g_object_ref (file_data->file));
			break;
		case G_FILE_TYPE_REGULAR:
		case G_FILE_TYPE_SYMBOLIC_LINK:
			cod->files = g_list_prepend (cod->files, g_object_ref (file_data->file));
			break;
		default:
			break;
		}
	}
	cod->files = g_list_reverse (cod->files);
	cod->dirs = g_list_reverse (cod->dirs);

	cod->current_dir = cod->dirs;
	copy__copy_current_dir (cod);
}


static void
gth_file_source_vfs_copy (GthFileSource *file_source,
			  GFile         *destination,
			  GList         *file_list, /* GFile * list */
			  ReadyCallback  callback,
			  gpointer       data)
{
	CopyOpData *cod;

	cod = g_new0 (CopyOpData, 1);
	cod->file_source = g_object_ref (file_source);
	cod->destination = g_object_ref (destination);
	cod->file_list = _g_object_list_ref (file_list);
	cod->callback = callback;
	cod->user_data = data;

	g_query_info_async (cod->file_list,
			    G_FILE_ATTRIBUTE_STANDARD_TYPE,
			    cod->file_source->priv->cancellable,
			    copy__file_list_info_ready_cb,
			    cod);
}


static gboolean
gth_file_source_vfs_can_cut (GthFileSource *file_source)
{
	return TRUE;
}


static void
mount_monitor_mountpoints_changed_cb (GUnixMountMonitor *monitor,
				      gpointer           user_data)
{
	gth_monitor_file_entry_points_changed (gth_main_get_default_monitor ());
}


static void
monitor_entry_points (GthFileSource *file_source)
{
	GthFileSourceVfs *file_source_vfs = (GthFileSourceVfs *) file_source;

	if (mount_monitor_id != 0)
		return;

	file_source_vfs->priv->mount_monitor = g_unix_mount_monitor_new ();
	mount_monitor_id = g_signal_connect (file_source_vfs->priv->mount_monitor,
					     "mounts-changed",
					     G_CALLBACK (mount_monitor_mountpoints_changed_cb),
					     file_source_vfs);
}


static gboolean
process_event_queue (gpointer data)
{
	GthFileSourceVfs *file_source_vfs = data;
	GthMonitor       *monitor;
	GList            *monitor_queue[GTH_MONITOR_N_EVENTS];
	int               event_type;

	if (file_source_vfs->priv->monitor_update_id != 0)
		g_source_remove (file_source_vfs->priv->monitor_update_id);
	file_source_vfs->priv->monitor_update_id = 0;

	for (event_type = 0; event_type < GTH_MONITOR_N_EVENTS; event_type++) {
		monitor_queue[event_type] = file_source_vfs->priv->monitor_queue[event_type];
		file_source_vfs->priv->monitor_queue[event_type] = NULL;
	}

	monitor = gth_main_get_default_monitor ();
	for (event_type = 0; event_type < GTH_MONITOR_N_EVENTS; event_type++) {
		GList *scan;

		for (scan = monitor_queue[event_type]; scan; scan = scan->next) {
			GFile *file = scan->data;
			GFile *parent;
			GList *list;

#ifdef DEBUG_MONITOR
			switch (event_type) {
			case GTH_MONITOR_EVENT_CREATED:
				g_print ("GTH_MONITOR_EVENT_CREATED");
				break;
			case GTH_MONITOR_EVENT_DELETED:
				g_print ("GTH_MONITOR_EVENT_DELETED");
				break;
			case GTH_MONITOR_EVENT_CHANGED:
				g_print ("GTH_MONITOR_EVENT_CHANGED");
				break;
			}
			g_print (" ==> %s\n", g_file_get_uri (file));
#endif

			parent = g_file_get_parent (file);
			list = g_list_prepend (NULL, g_object_ref (file));
			gth_monitor_folder_changed (monitor,
						    parent,
						    list,
						    event_type);

			_g_object_list_unref (list);
			g_object_unref (parent);
		}
		_g_object_list_unref (monitor_queue[event_type]);
		monitor_queue[event_type] = NULL;
	}

	return FALSE;
}


static gboolean
remove_if_present (GthFileSourceVfs *file_source_vfs,
		   GthMonitorEvent   event_type,
		   GFile            *file)
{
	GList *link;

	link = _g_file_list_find_file (file_source_vfs->priv->monitor_queue[event_type], file);
	if (link != NULL) {
		file_source_vfs->priv->monitor_queue[event_type] = g_list_remove_link (file_source_vfs->priv->monitor_queue[event_type], link);
		_g_object_list_unref (link);
		return TRUE;
	}

	return FALSE;
}


static void
monitor_changed_cb (GFileMonitor      *file_monitor,
		    GFile             *file,
		    GFile             *other_file,
		    GFileMonitorEvent  file_event_type,
		    gpointer           user_data)
{
	GthFileSourceVfs *file_source_vfs = user_data;
	GthMonitorEvent   event_type;

	switch (file_event_type) {
	case G_FILE_MONITOR_EVENT_CREATED:
		event_type = GTH_MONITOR_EVENT_CREATED;
		break;

	case G_FILE_MONITOR_EVENT_DELETED:
		event_type = GTH_MONITOR_EVENT_DELETED;
		break;

	case G_FILE_MONITOR_EVENT_CHANGED:
	case G_FILE_MONITOR_EVENT_ATTRIBUTE_CHANGED:
		event_type = GTH_MONITOR_EVENT_CHANGED;
		break;

	case G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT:
	case G_FILE_MONITOR_EVENT_PRE_UNMOUNT:
	case G_FILE_MONITOR_EVENT_UNMOUNTED:
		return;
	}

#ifdef DEBUG_MONITOR
	g_print ("[RAW] ");
	switch (event_type) {
	case GTH_MONITOR_EVENT_CREATED:
		g_print ("GTH_MONITOR_EVENT_CREATED");
		break;
	case GTH_MONITOR_EVENT_DELETED:
		g_print ("GTH_MONITOR_EVENT_DELETED");
		break;
	case GTH_MONITOR_EVENT_CHANGED:
		g_print ("GTH_MONITOR_EVENT_CHANGED");
		break;
	}
	g_print (" ==> %s\n", g_file_get_uri (file));
#endif

	if (event_type == GTH_MONITOR_EVENT_CREATED) {
		if (remove_if_present (file_source_vfs, GTH_MONITOR_EVENT_DELETED, file))
			event_type = GTH_MONITOR_EVENT_CHANGED;
	}
	else if (event_type == GTH_MONITOR_EVENT_DELETED) {
		remove_if_present (file_source_vfs, GTH_MONITOR_EVENT_CREATED, file);
		remove_if_present (file_source_vfs, GTH_MONITOR_EVENT_CHANGED, file);
	}
	else if (event_type == GTH_MONITOR_EVENT_CHANGED) {
		if (_g_file_list_find_file (file_source_vfs->priv->monitor_queue[GTH_MONITOR_EVENT_CREATED], file))
			return;
		remove_if_present (file_source_vfs, GTH_MONITOR_EVENT_CHANGED, file);
	}

	file_source_vfs->priv->monitor_queue[event_type] = g_list_prepend (file_source_vfs->priv->monitor_queue[event_type], g_file_dup (file));

	if (file_source_vfs->priv->monitor_update_id != 0)
		g_source_remove (file_source_vfs->priv->monitor_update_id);
	file_source_vfs->priv->monitor_update_id = g_timeout_add (MONITOR_UPDATE_DELAY,
								  process_event_queue,
								  file_source_vfs);
}


static void
monitor_directory (GthFileSource *file_source,
		   GFile         *file,
		   gboolean       activate)
{
	GthFileSourceVfs *file_source_vfs = (GthFileSourceVfs *) file_source;
	GFileMonitor     *monitor;

	if (! activate) {
		g_hash_table_remove (file_source_vfs->priv->monitors, file);
		return;
	}

	if (g_hash_table_lookup (file_source_vfs->priv->monitors, file) != NULL)
		return;

	monitor = g_file_monitor_directory (file, 0, NULL, NULL);
	if (monitor == NULL)
		return;

	g_hash_table_insert (file_source_vfs->priv->monitors, g_object_ref (file), monitor);
	g_signal_connect (monitor, "changed", G_CALLBACK (monitor_changed_cb), file_source);
}


static void
gth_file_source_vfs_finalize (GObject *object)
{
	GthFileSourceVfs *file_source_vfs = GTH_FILE_SOURCE_VFS (object);

	if (file_source_vfs->priv != NULL) {
		int i;

		if (file_source_vfs->priv->monitor_update_id != 0) {
			g_source_remove (file_source_vfs->priv->monitor_update_id);
			file_source_vfs->priv->monitor_update_id = 0;
		}

		g_hash_table_destroy (file_source_vfs->priv->monitors);

		for (i = 0; i < GTH_MONITOR_N_EVENTS; i++) {
			_g_object_list_unref (file_source_vfs->priv->monitor_queue[i]);
			file_source_vfs->priv->monitor_queue[i] = NULL;
		}
		_g_object_list_unref (file_source_vfs->priv->files);

		g_free (file_source_vfs->priv);
		file_source_vfs->priv = NULL;
	}

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gth_file_source_vfs_class_init (GthFileSourceVfsClass *class)
{
	GObjectClass       *object_class;
	GthFileSourceClass *file_source_class;

	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;
	file_source_class = (GthFileSourceClass*) class;

	object_class->finalize = gth_file_source_vfs_finalize;
	file_source_class->get_entry_points = get_entry_points;
	file_source_class->to_gio_file = to_gio_file;
	file_source_class->get_file_info = get_file_info;
	file_source_class->list = list;
	file_source_class->read_attributes = read_attributes;
	file_source_class->cancel = cancel;
	file_source_class->copy = gth_file_source_vfs_copy;
	file_source_class->can_cut = gth_file_source_vfs_can_cut;
	file_source_class->monitor_entry_points = monitor_entry_points;
	file_source_class->monitor_directory = monitor_directory;
}


static void
gth_file_source_vfs_init (GthFileSourceVfs *file_source)
{
	int i;

	file_source->priv = g_new0 (GthFileSourceVfsPrivate, 1);
	gth_file_source_add_scheme (GTH_FILE_SOURCE (file_source), "vfs+");
	file_source->priv->cancellable = g_cancellable_new ();
	file_source->priv->monitors = g_hash_table_new_full (g_file_hash, (GEqualFunc) g_file_equal, g_object_unref, g_object_unref);
	for (i = 0; i < GTH_MONITOR_N_EVENTS; i++)
		file_source->priv->monitor_queue[i] = NULL;
}


GType
gth_file_source_vfs_get_type (void)
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (GthFileSourceVfsClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_file_source_vfs_class_init,
			NULL,
			NULL,
			sizeof (GthFileSourceVfs),
			0,
			(GInstanceInitFunc) gth_file_source_vfs_init
		};

		type = g_type_register_static (GTH_TYPE_FILE_SOURCE,
					       "GthFileSourceVfs",
					       &type_info,
					       0);
	}

	return type;
}
