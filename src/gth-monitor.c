/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2005 Free Software Foundation, Inc.
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

#include <libgnomevfs/gnome-vfs-monitor.h>
#include <libgnomevfs/gnome-vfs-ops.h>
#include <libgnomevfs/gnome-vfs-result.h>
#include <libgnomevfs/gnome-vfs-utils.h>

#include "file-utils.h"
#include "glib-utils.h"
#include "gstringlist.h"
#include "gthumb-marshal.h"
#include "gth-monitor.h"

#define UPDATE_DIR_DELAY 500

typedef enum {
	MONITOR_EVENT_FILE_CREATED = 0,
	MONITOR_EVENT_FILE_DELETED,
	MONITOR_EVENT_DIR_CREATED,
	MONITOR_EVENT_DIR_DELETED,
	MONITOR_EVENT_FILE_CHANGED,
	MONITOR_EVENT_NUM
} MonitorEventType;

enum {
	UPDATE_ICON_THEME,
	UPDATE_BOOKMARKS,
	UPDATE_CAT_FILES,
	UPDATE_FILES,
	UPDATE_DIRECTORY,
	UPDATE_CATALOG,
	FILE_RENAMED,
	DIRECTORY_RENAMED,
	CATALOG_RENAMED,
	LAST_SIGNAL
};

struct _GthMonitorPrivateData {
	GnomeVFSMonitorHandle *monitor_handle;
	guint                  monitor_enabled : 1;
	guint                  update_changes_timeout;
	GList                 *monitor_events[MONITOR_EVENT_NUM]; /* char * lists */
};

static GObjectClass *parent_class = NULL;
static guint monitor_signals[LAST_SIGNAL] = { 0 };


static void 
gth_monitor_finalize (GObject *object)
{
	GthMonitor *monitor = GTH_MONITOR (object);

	if (monitor->priv != NULL) {
		GthMonitorPrivateData *priv = monitor->priv;
		int i;

		if (priv->monitor_handle != NULL) {
			gnome_vfs_monitor_cancel (priv->monitor_handle);
			priv->monitor_handle = NULL;
		}
		
		for (i = 0; i < MONITOR_EVENT_NUM; i++)
			path_list_free (priv->monitor_events[i]);

		/**/

		g_free (monitor->priv);
		monitor->priv = NULL;
	}

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gth_monitor_init (GthMonitor *monitor)
{
	GthMonitorPrivateData *priv;
	int i;

	priv = monitor->priv = g_new0 (GthMonitorPrivateData, 1);

	priv->monitor_handle = NULL;
	priv->monitor_enabled = FALSE;
	priv->update_changes_timeout = 0;

	for (i = 0; i < MONITOR_EVENT_NUM; i++)
		priv->monitor_events[i] = NULL;
}


/* -- file system monitor -- */


static gboolean
proc_monitor_events (gpointer data)
{
	GthMonitor            *monitor = data;
	GthMonitorPrivateData *priv = monitor->priv;
	GList                 *dir_created_list, *dir_deleted_list;
	GList                 *file_created_list, *file_deleted_list, *file_changed_list;
	GList                 *scan;

	dir_created_list = priv->monitor_events[MONITOR_EVENT_DIR_CREATED];
	priv->monitor_events[MONITOR_EVENT_DIR_CREATED] = NULL;

	dir_deleted_list = priv->monitor_events[MONITOR_EVENT_DIR_DELETED];
	priv->monitor_events[MONITOR_EVENT_DIR_DELETED] = NULL;

	file_created_list = priv->monitor_events[MONITOR_EVENT_FILE_CREATED];
	priv->monitor_events[MONITOR_EVENT_FILE_CREATED] = NULL;

	file_deleted_list = priv->monitor_events[MONITOR_EVENT_FILE_DELETED];
	priv->monitor_events[MONITOR_EVENT_FILE_DELETED] = NULL;

	file_changed_list = priv->monitor_events[MONITOR_EVENT_FILE_CHANGED];
	priv->monitor_events[MONITOR_EVENT_FILE_CHANGED] = NULL;

	if (priv->update_changes_timeout != 0) 
		g_source_remove (priv->update_changes_timeout);

	/**/

	for (scan = dir_created_list; scan; scan = scan->next) {
		char *path = scan->data;
		const char *name = file_name_from_path (path);

		/* ignore hidden directories. */
		if (name[0] == '.')
			continue;

		/* dir_list_add_directory (priv->dir_list, path); FIXME */
	}
	path_list_free (dir_created_list);

	/**/

	for (scan = dir_deleted_list; scan; scan = scan->next) {
		char *path = scan->data;
		/* dir_list_remove_directory (priv->dir_list, path); FIXME */
	}
	path_list_free (dir_deleted_list);

	/**/

	if (file_created_list != NULL) {
		/* notify_files_added (browser, file_created_list); FIXME */
		path_list_free (file_created_list);
	}

	if (file_deleted_list != NULL) {
		/* notify_files_deleted (browser, file_deleted_list); FIXME */
		path_list_free (file_deleted_list);
	}

	if (file_changed_list != NULL) {
		/* gth_browser_notify_files_changed (browser, file_changed_list); FIXME */
		path_list_free (file_changed_list);
	}

	/**/

	priv->update_changes_timeout = 0;

	return FALSE;
}


static gboolean
remove_if_present (GList            **monitor_events,
		   MonitorEventType   type,
		   const char        *path)
{
	GList *list, *link;

	list = monitor_events[type];
	link = path_list_find_path (list, path);
	if (link != NULL) {
		monitor_events[type] = g_list_remove_link (list, link);
		path_list_free (link);
		return TRUE;
	}

	return FALSE;
}


static gboolean
add_if_not_present (GList            **monitor_events,
		    MonitorEventType   type,
		    MonitorEventType   add_type,
		    const char        *path)
{
	GList *list, *link;

	list = monitor_events[type];
	link = path_list_find_path (list, path);
	if (link == NULL) {
		monitor_events[add_type] = g_list_append (list, g_strdup (path));
		return TRUE;
	}

	return FALSE;
}


static void
add_monitor_event (GthMonitor                *monitor,
		   GnomeVFSMonitorEventType   event_type,
		   char                      *path,
		   GList                    **monitor_events)
{
	MonitorEventType type;

#ifdef DEBUG /* FIXME */
	{
		char *op;

		if (event_type == GNOME_VFS_MONITOR_EVENT_CREATED)
			op = "CREATED";
		else if (event_type == GNOME_VFS_MONITOR_EVENT_DELETED)
			op = "DELETED";
		else 
			op = "CHANGED";

		debug (DEBUG_INFO, "[%s] %s", op, path);
	}
#endif

	if (event_type == GNOME_VFS_MONITOR_EVENT_CREATED) {
		if (path_is_dir (path))
			type = MONITOR_EVENT_DIR_CREATED;
		else
			type = MONITOR_EVENT_FILE_CREATED;

	} else if (event_type == GNOME_VFS_MONITOR_EVENT_DELETED) {
		if (path_is_dir (path))
			type = MONITOR_EVENT_DIR_DELETED;
		else
			type = MONITOR_EVENT_FILE_DELETED;

	} else {
		if (path_is_file (path))
			type = MONITOR_EVENT_FILE_CHANGED;
		else 
			return;
	}

	if (type == MONITOR_EVENT_FILE_CREATED) {
		if (remove_if_present (monitor_events, 
				       MONITOR_EVENT_FILE_DELETED, 
				       path))
			type = MONITOR_EVENT_FILE_CHANGED;
		
	} else if (type == MONITOR_EVENT_FILE_DELETED) {
		remove_if_present (monitor_events, 
				   MONITOR_EVENT_FILE_CREATED, 
				   path);
		remove_if_present (monitor_events, 
				   MONITOR_EVENT_FILE_CHANGED, 
				   path);

	} else if (type == MONITOR_EVENT_FILE_CHANGED) {
		remove_if_present (monitor_events, 
				   MONITOR_EVENT_FILE_CHANGED, 
				   path);
		
	} else if (type == MONITOR_EVENT_DIR_CREATED) {
		remove_if_present (monitor_events, 
				   MONITOR_EVENT_DIR_DELETED,
				   path);

	} else if (type == MONITOR_EVENT_DIR_DELETED) 
		remove_if_present (monitor_events, 
				   MONITOR_EVENT_DIR_CREATED, 
				   path);

	monitor_events[type] = g_list_append (monitor_events[type], g_strdup (path));
}


static void
directory_changed (GnomeVFSMonitorHandle    *handle,
		   const char               *monitor_uri,
		   const char               *info_uri,
		   GnomeVFSMonitorEventType  event_type,
		   gpointer                  user_data)
{
	GthMonitor            *monitor = user_data; 
	GthMonitorPrivateData *priv = monitor->priv;
	char                  *path;

	path = gnome_vfs_unescape_string (info_uri + strlen ("file://"), NULL);
	add_monitor_event (monitor, event_type, path, priv->monitor_events);
	g_free (path);

	priv->update_changes_timeout = g_timeout_add (UPDATE_DIR_DELAY,
						      proc_monitor_events,
						      monitor);
}


void
gth_monitor_add_uri (GthMonitor *monitor,
		     const char *uri)
{
	/* FIXME
	GthMonitorPrivateData *priv = monitor->priv;
	GnomeVFSResult         result;
	char                  *uri;

	if (priv->monitor_handle != NULL)
		gnome_vfs_monitor_cancel (priv->monitor_handle);

	uri = g_strconcat ("file://", priv->dir_list->path, NULL);
	result = gnome_vfs_monitor_add (&priv->monitor_handle,
					uri,
					GNOME_VFS_MONITOR_DIRECTORY,
					directory_changed,
					browser);
	g_free (uri);
	priv->monitor_enabled = (result == GNOME_VFS_OK);
	*/
}


void
gth_monitor_remove_uri (GthMonitor *monitor,
			const char *uri)
{
	/* FIXME
	GthBrowserPrivateData *priv = browser->priv;

	if (priv->monitor_handle != NULL) {
		gnome_vfs_monitor_cancel (priv->monitor_handle);
		priv->monitor_handle = NULL;
	}
	priv->monitor_enabled = FALSE;
	 */
}


GthMonitor * 
gth_monitor_new (void)
{
	return (GthMonitor*) g_object_new (GTH_TYPE_MONITOR, NULL);
}


void
gth_monitor_pause (GthMonitor *monitor)
{
}


void
gth_monitor_resume (GthMonitor *monitor)
{
}


void
gth_monitor_notify_update_icon_theme (GthMonitor *monitor)
{
	g_return_if_fail (GTH_IS_MONITOR (monitor));
	g_signal_emit (G_OBJECT (monitor), 
		       monitor_signals[UPDATE_ICON_THEME], 
		       0);
}


void
gth_monitor_notify_update_bookmarks (GthMonitor *monitor)
{
	g_return_if_fail (GTH_IS_MONITOR (monitor));
	g_signal_emit (G_OBJECT (monitor), 
		       monitor_signals[UPDATE_BOOKMARKS], 
		       0);
}


void
gth_monitor_notify_update_cat_files (GthMonitor      *monitor,
				     const char      *catalog_path,
				     GthMonitorEvent  event,
				     GList           *list)
{	
	g_return_if_fail (GTH_IS_MONITOR (monitor));
	g_signal_emit (G_OBJECT (monitor), 
		       monitor_signals[UPDATE_CAT_FILES], 
		       0,
		       catalog_path,
		       event,
		       list);
}


void
gth_monitor_notify_update_files (GthMonitor      *monitor,
				 GthMonitorEvent  event,
				 GList           *list)
{	
	g_return_if_fail (GTH_IS_MONITOR (monitor));
	g_signal_emit (G_OBJECT (monitor), 
		       monitor_signals[UPDATE_FILES], 
		       0,
		       event,
		       list);
}


void
gth_monitor_notify_update_directory (GthMonitor      *monitor,
				     const char      *dir_path,
				     GthMonitorEvent  event)
{	
	g_return_if_fail (GTH_IS_MONITOR (monitor));
	g_signal_emit (G_OBJECT (monitor), 
		       monitor_signals[UPDATE_DIRECTORY], 
		       0,
		       dir_path,
		       event);
}


void
gth_monitor_notify_update_catalog (GthMonitor      *monitor,
				   const char      *catalog_path,
				   GthMonitorEvent  event)
{	
	g_return_if_fail (GTH_IS_MONITOR (monitor));
	g_signal_emit (G_OBJECT (monitor), 
		       monitor_signals[UPDATE_CATALOG], 
		       0,
		       catalog_path,
		       event);
}


void
gth_monitor_notify_file_renamed (GthMonitor      *monitor,
				 const char      *old_name,
				 const char      *new_name)
{
	g_return_if_fail (GTH_IS_MONITOR (monitor));
	g_signal_emit (G_OBJECT (monitor), 
		       monitor_signals[FILE_RENAMED], 
		       0,
		       old_name,
		       new_name);
}


void
gth_monitor_notify_directory_renamed (GthMonitor      *monitor,
				      const char      *old_name,
				      const char      *new_name)
{
	g_return_if_fail (GTH_IS_MONITOR (monitor));
	g_signal_emit (G_OBJECT (monitor), 
		       monitor_signals[DIRECTORY_RENAMED], 
		       0,
		       old_name,
		       new_name);
}


void
gth_monitor_notify_catalog_renamed (GthMonitor      *monitor,
				    const char      *old_name,
				    const char      *new_name)
{
	g_return_if_fail (GTH_IS_MONITOR (monitor));
	g_signal_emit (G_OBJECT (monitor), 
		       monitor_signals[CATALOG_RENAMED], 
		       0,
		       old_name,
		       new_name);
}


static void
gth_monitor_class_init (GthMonitorClass *class)
{
	GObjectClass  *gobject_class;

	parent_class = g_type_class_peek_parent (class);

	gobject_class = (GObjectClass*) class;
	gobject_class->finalize = gth_monitor_finalize;

	monitor_signals[UPDATE_ICON_THEME] =
		g_signal_new ("update-icon_theme",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthMonitorClass, update_icon_theme),
			      NULL, NULL,
			      gthumb_marshal_VOID__VOID,
			      G_TYPE_NONE, 
			      0);
	monitor_signals[UPDATE_BOOKMARKS] =
		g_signal_new ("update-bookmarks",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthMonitorClass, update_bookmarks),
			      NULL, NULL,
			      gthumb_marshal_VOID__VOID,
			      G_TYPE_NONE, 
			      0);
	monitor_signals[UPDATE_CAT_FILES] =
		g_signal_new ("update-cat-files",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthMonitorClass, update_cat_files),
			      NULL, NULL,
			      gthumb_marshal_VOID__STRING_INT_BOXED,
			      G_TYPE_NONE, 
			      3,
			      G_TYPE_STRING,
			      G_TYPE_INT,
			      G_TYPE_STRING_LIST);
	monitor_signals[UPDATE_FILES] =
		g_signal_new ("update-files",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthMonitorClass, update_files),
			      NULL, NULL,
			      gthumb_marshal_VOID__INT_BOXED,
			      G_TYPE_NONE, 
			      2,
			      G_TYPE_INT,
			      G_TYPE_STRING_LIST);
	monitor_signals[UPDATE_DIRECTORY] =
		g_signal_new ("update-directory",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthMonitorClass, update_directory),
			      NULL, NULL,
			      gthumb_marshal_VOID__STRING_INT,
			      G_TYPE_NONE, 
			      2,
			      G_TYPE_STRING,
			      G_TYPE_INT);
	monitor_signals[UPDATE_CATALOG] =
		g_signal_new ("update-catalog",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthMonitorClass, update_catalog),
			      NULL, NULL,
			      gthumb_marshal_VOID__STRING_INT,
			      G_TYPE_NONE, 
			      2,
			      G_TYPE_STRING,
			      G_TYPE_INT);

	monitor_signals[FILE_RENAMED] =
		g_signal_new ("file-renamed",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthMonitorClass, file_renamed),
			      NULL, NULL,
			      gthumb_marshal_VOID__STRING_STRING,
			      G_TYPE_NONE, 
			      2,
			      G_TYPE_STRING,
			      G_TYPE_STRING);
	monitor_signals[DIRECTORY_RENAMED] =
		g_signal_new ("directory-renamed",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthMonitorClass, directory_renamed),
			      NULL, NULL,
			      gthumb_marshal_VOID__STRING_STRING,
			      G_TYPE_NONE, 
			      2,
			      G_TYPE_STRING,
			      G_TYPE_STRING);
	monitor_signals[CATALOG_RENAMED] =
		g_signal_new ("catalog-renamed",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthMonitorClass, catalog_renamed),
			      NULL, NULL,
			      gthumb_marshal_VOID__STRING_STRING,
			      G_TYPE_NONE, 
			      2,
			      G_TYPE_STRING,
			      G_TYPE_STRING);
}


GType
gth_monitor_get_type (void)
{
        static GType type = 0;

        if (! type) {
                GTypeInfo type_info = {
			sizeof (GthMonitorClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_monitor_class_init,
			NULL,
			NULL,
			sizeof (GthMonitor),
			0,
			(GInstanceInitFunc) gth_monitor_init
		};

		type = g_type_register_static (G_TYPE_OBJECT,
					       "GthMonitor",
					       &type_info,
					       0);
	}

        return type;
}
