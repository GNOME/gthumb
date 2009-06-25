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
#include <gio/gio.h>
#include <string.h>

#include "file-utils.h"
#include "gfile-utils.h"
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
	UPDATE_METADATA,
	FILE_RENAMED,
	DIRECTORY_RENAMED,
	CATALOG_RENAMED,
	RELOAD_CATALOGS,
	LAST_SIGNAL
};

typedef struct {
	GFileMonitor *handle;
	GFile        *gfile;
	int           ref;
} MonitorHandle;

static GthMonitor *instance = NULL;

MonitorHandle *
monitor_handle_new (GFileMonitor *monitor_handle,
		    GFile        *gfile)
{
	MonitorHandle *mh;

	g_return_val_if_fail (monitor_handle != NULL, NULL);
	g_return_val_if_fail (gfile != NULL, NULL);

	mh = g_new0 (MonitorHandle, 1);
	mh->handle = monitor_handle;
	mh->gfile = gfile;
	g_object_ref (gfile);
	mh->ref = 1;

	return mh;
}


static void
monitor_handle_ref (MonitorHandle *mh)
{
	if (mh == NULL)
		return;
	mh->ref++;
}


static void
monitor_handle_destroy (MonitorHandle *mh)
{
	if (mh == NULL)
		return;
	if (mh->handle != NULL)
		g_file_monitor_cancel (mh->handle);
	g_object_unref (mh->gfile);
	g_free (mh);
}


static void
monitor_handle_unref (MonitorHandle *mh)
{
	if (mh == NULL)
		return;
	mh->ref--;
	if (mh->ref == 0)
		monitor_handle_destroy (mh);
}


static GList *
find_monitor_from_gfile (GList *gfile_monitors,
		         GFile *gfile)
{
	GList *scan;

	for (scan = gfile_monitors; scan; scan = scan->next) {
		MonitorHandle *mh = scan->data;
		if (g_file_equal (mh->gfile, gfile)) {
			return scan;
                }
	}

	return NULL;
}


struct _GthMonitorPrivateData {
	GList    *gfile_monitors; /* MonitorHandle list */
	guint     monitor_enabled : 1;
	guint     update_changes_timeout;
	GList    *monitor_events[MONITOR_EVENT_NUM]; /* char * lists */
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

		if (priv->gfile_monitors != NULL) {
			g_list_foreach (priv->gfile_monitors,
					(GFunc) monitor_handle_destroy,
					NULL);
			g_list_free (priv->gfile_monitors);
			priv->gfile_monitors = NULL;
		}

		for (i = 0; i < MONITOR_EVENT_NUM; i++)
			gfile_list_free (priv->monitor_events[i]);

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

	priv->gfile_monitors = NULL;
	priv->monitor_enabled = TRUE;
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

	if (priv->update_changes_timeout != 0)
		g_source_remove (priv->update_changes_timeout);
	priv->update_changes_timeout = 0;

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
		GFile      *gfile = scan->data;
		char       *path = g_file_get_parse_name (gfile);
		const char *name = file_name_from_path (path);

		/* ignore hidden directories. */
		if (name[0] == '.')
			continue;

                gth_monitor_notify_update_directory (path, GTH_MONITOR_EVENT_CREATED);
		g_free (path);
	}
	gfile_list_free (dir_created_list);

	/**/

	for (scan = dir_deleted_list; scan; scan = scan->next) {
		GFile *gfile = scan->data;
		char  *path = g_file_get_parse_name (gfile);
		gth_monitor_notify_update_directory (path, GTH_MONITOR_EVENT_DELETED);
		g_free (path);
	}
	gfile_list_free (dir_deleted_list);

	/**/

	if (file_created_list != NULL) {
		gth_monitor_notify_update_gfiles (GTH_MONITOR_EVENT_CREATED, file_created_list);
		gfile_list_free (file_created_list);
	}

	if (file_deleted_list != NULL) {
		gth_monitor_notify_update_gfiles (GTH_MONITOR_EVENT_DELETED, file_deleted_list);
		gfile_list_free (file_deleted_list);
	}

	if (file_changed_list != NULL) {
		gth_monitor_notify_update_gfiles (GTH_MONITOR_EVENT_CHANGED, file_changed_list);
		gfile_list_free (file_changed_list);
	}

	return FALSE;
}


static gboolean
remove_if_present (GList            **monitor_events,
		   MonitorEventType   type,
		   GFile             *gfile)
{
	GList *list, *link;

	list = monitor_events[type];
	link = gfile_list_find_gfile (list, gfile);
	if (link != NULL) {
		monitor_events[type] = g_list_remove_link (list, link);
		g_object_unref (link->data);
		g_list_free_1 (link);
		return TRUE;
	}

	return FALSE;
}


static void
add_monitor_event (GthMonitor        *monitor,
		   GFileMonitorEvent  event_type,
		   GFile             *gfile,
		   GList            **monitor_events)
{
	MonitorEventType  type;
	char             *op;
	
	if (!monitor->priv->monitor_enabled)
		return;

	if (event_type == G_FILE_MONITOR_EVENT_CREATED)
		op = "CREATED";
	else if (event_type == G_FILE_MONITOR_EVENT_DELETED)
		op = "DELETED";
	else
		op = "CHANGED";

	if (event_type == G_FILE_MONITOR_EVENT_CREATED) {
		if (gfile_is_file (gfile))
			type = MONITOR_EVENT_FILE_CREATED;
		else if (gfile_is_dir (gfile))
			type = MONITOR_EVENT_DIR_CREATED;
		else
			return;

	} else if (event_type == G_FILE_MONITOR_EVENT_DELETED) {
		if (gfile_is_file (gfile))
			type = MONITOR_EVENT_FILE_DELETED;
		else
			type = MONITOR_EVENT_DIR_DELETED;

	} else {
		if (gfile_is_file (gfile))
			type = MONITOR_EVENT_FILE_CHANGED;
		else
			return;
	}

	if (type == MONITOR_EVENT_FILE_CREATED) {
		if (remove_if_present (monitor_events,
				       MONITOR_EVENT_FILE_DELETED,
				       gfile))
			type = MONITOR_EVENT_FILE_CHANGED;

	} else if (type == MONITOR_EVENT_FILE_DELETED) {
		remove_if_present (monitor_events,
				   MONITOR_EVENT_FILE_CREATED,
				   gfile);
		remove_if_present (monitor_events,
				   MONITOR_EVENT_FILE_CHANGED,
				   gfile);

	} else if (type == MONITOR_EVENT_FILE_CHANGED) {
		remove_if_present (monitor_events,
				   MONITOR_EVENT_FILE_CHANGED,
				   gfile);

	} else if (type == MONITOR_EVENT_DIR_CREATED) {
		remove_if_present (monitor_events,
				   MONITOR_EVENT_DIR_DELETED,
				   gfile);

	} else if (type == MONITOR_EVENT_DIR_DELETED)
		remove_if_present (monitor_events,
				   MONITOR_EVENT_DIR_CREATED,
				   gfile);

	monitor_events[type] = g_list_append (monitor_events[type], gfile);
	g_object_ref (gfile);
}


static void
directory_changed (GFileMonitor      *handle,
		   GFile             *gfile,
		   GFile             *other_file,
		   GFileMonitorEvent  event_type,
		   gpointer           user_data)
{
	GthMonitor            *monitor = user_data;
	GthMonitorPrivateData *priv = monitor->priv;

	add_monitor_event (monitor, event_type, gfile, priv->monitor_events);

	if (priv->update_changes_timeout != 0)
		g_source_remove (priv->update_changes_timeout);
	priv->update_changes_timeout = g_timeout_add (UPDATE_DIR_DELAY,
						      proc_monitor_events,
						      monitor);
}


void
gth_monitor_add_gfile (GFile *gfile)
{
	GthMonitorPrivateData *priv = instance->priv;
	GFileMonitor          *monitor_handle;
	GList                 *item;

	item = find_monitor_from_gfile (priv->gfile_monitors, gfile);
	if (item != NULL) {
		MonitorHandle *mh = item->data;
		monitor_handle_ref (mh);
		return;
	}

	monitor_handle = g_file_monitor_directory (gfile, G_FILE_MONITOR_NONE, NULL, NULL);

	if (monitor_handle != NULL) {
		MonitorHandle *mh = monitor_handle_new (monitor_handle, gfile);
		priv->gfile_monitors = g_list_prepend (priv->gfile_monitors, mh);

	        g_signal_connect (G_OBJECT (monitor_handle),
                          "changed",
                          G_CALLBACK (directory_changed),
                          instance);
	}

	priv->monitor_enabled = TRUE;
}


void
gth_monitor_remove_gfile (GFile *gfile)
{
	GthMonitorPrivateData *priv = instance->priv;
	GList                 *item;
	MonitorHandle         *mh;

	if (gfile == NULL)
		return;

	item = find_monitor_from_gfile (priv->gfile_monitors, gfile);
	if (item == NULL)
		return;

	mh = item->data;
	if (mh->ref == 1)
		priv->gfile_monitors = g_list_remove_link (priv->gfile_monitors, item);
	monitor_handle_unref (mh);
}

GthMonitor *
gth_monitor_get_instance (void)
{
        if (!instance) {
                instance = GTH_MONITOR (g_object_new (GTH_TYPE_MONITOR, NULL));
        }
	return instance;
}


void
gth_monitor_pause (void)
{
	instance->priv->monitor_enabled = FALSE;
}


gboolean
gth_monitor_resume (void)
{
	instance->priv->monitor_enabled = TRUE;

	/* to cancel idle calls, if used */
	return FALSE;
}


void
gth_monitor_notify_update_icon_theme (void)
{
	g_return_if_fail (GTH_IS_MONITOR (instance));
	g_signal_emit (G_OBJECT (instance),
		       monitor_signals[UPDATE_ICON_THEME],
		       0);
}


void
gth_monitor_notify_update_bookmarks (void)
{
	g_return_if_fail (GTH_IS_MONITOR (instance));
	g_signal_emit (G_OBJECT (instance),
		       monitor_signals[UPDATE_BOOKMARKS],
		       0);
}


void
gth_monitor_notify_update_cat_files (const char      *catalog_path,
				     GthMonitorEvent  event,
				     GList           *list)
{
	g_return_if_fail (GTH_IS_MONITOR (instance));

	if (list == NULL)
		return;

	g_signal_emit (G_OBJECT (instance),
		       monitor_signals[UPDATE_CAT_FILES],
		       0,
		       catalog_path,
		       event,
		       list);
}


void
gth_monitor_notify_update_gfiles (GthMonitorEvent  event,
				  GList           *list)
{
	GList *path_list = NULL;
	GList *scan;

	g_return_if_fail (GTH_IS_MONITOR (instance));

	if (list == NULL)
		return;

	/* FIXME: use gfile list in signals */
        for (scan = list; scan; scan = scan->next) {
                GFile *gfile = scan->data;
		path_list = g_list_prepend (path_list, g_file_get_parse_name (gfile));
        }

	g_signal_emit (G_OBJECT (instance),
		       monitor_signals[UPDATE_FILES],
		       0,
		       event,
		       path_list);

	path_list_free (path_list);
}


void
gth_monitor_notify_update_files (GthMonitorEvent  event,
				 GList           *list)
{
	g_return_if_fail (GTH_IS_MONITOR (instance));

	if (list == NULL)
		return;

	/* FIXME: start using the above gfile equivalent */
	g_signal_emit (G_OBJECT (instance),
		       monitor_signals[UPDATE_FILES],
		       0,
		       event,
		       list);
}


void
gth_monitor_notify_update_directory (const char      *dir_path,
				     GthMonitorEvent  event)
{
	g_return_if_fail (GTH_IS_MONITOR (instance));
	g_signal_emit (G_OBJECT (instance),
		       monitor_signals[UPDATE_DIRECTORY],
		       0,
		       dir_path,
		       event);
}


void
gth_monitor_notify_update_catalog (const char      *catalog_path,
				   GthMonitorEvent  event)
{
	g_return_if_fail (GTH_IS_MONITOR (instance));
	g_signal_emit (G_OBJECT (instance),
		       monitor_signals[UPDATE_CATALOG],
		       0,
		       catalog_path,
		       event);
}


void
gth_monitor_notify_update_metadata (const char *path)
{
	g_return_if_fail (GTH_IS_MONITOR (instance));
	g_signal_emit (G_OBJECT (instance),
		       monitor_signals[UPDATE_METADATA],
		       0,
		       path);
}


void
gth_monitor_notify_file_renamed (const char      *old_name,
				 const char      *new_name)
{
	g_return_if_fail (GTH_IS_MONITOR (instance));
	g_signal_emit (G_OBJECT (instance),
		       monitor_signals[FILE_RENAMED],
		       0,
		       old_name,
		       new_name);
}


void
gth_monitor_notify_directory_renamed (const char      *old_name,
				      const char      *new_name)
{
	g_return_if_fail (GTH_IS_MONITOR (instance));
	g_signal_emit (G_OBJECT (instance),
		       monitor_signals[DIRECTORY_RENAMED],
		       0,
		       old_name,
		       new_name);
}


void
gth_monitor_notify_catalog_renamed (const char      *old_name,
				    const char      *new_name)
{
	g_return_if_fail (GTH_IS_MONITOR (instance));
	g_signal_emit (G_OBJECT (instance),
		       monitor_signals[CATALOG_RENAMED],
		       0,
		       old_name,
		       new_name);
}


void
gth_monitor_notify_reload_catalogs (void)
{
	g_return_if_fail (GTH_IS_MONITOR (instance));
	g_signal_emit (G_OBJECT (instance),
		       monitor_signals[RELOAD_CATALOGS],
		       0);
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
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
	monitor_signals[UPDATE_BOOKMARKS] =
		g_signal_new ("update-bookmarks",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthMonitorClass, update_bookmarks),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
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
	monitor_signals[UPDATE_METADATA] =
		g_signal_new ("update-metadata",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthMonitorClass, update_metadata),
			      NULL, NULL,
			      gthumb_marshal_VOID__STRING,
			      G_TYPE_NONE,
			      1,
			      G_TYPE_STRING);
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
	monitor_signals[RELOAD_CATALOGS] =
		g_signal_new ("reload-catalogs",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthMonitorClass, reload_catalogs),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
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
