/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2005-2008 Free Software Foundation, Inc.
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
#include "glib-utils.h"
#include "gth-enum-types.h"
#include "gth-marshal.h"
#include "gth-monitor.h"


#define UPDATE_DIR_DELAY 500


enum {
	ICON_THEME_CHANGED,
	BOOKMARKS_CHANGED,
	FILTERS_CHANGED,
	FOLDER_CONTENT_CHANGED,
	FILE_RENAMED,
	METADATA_CHANGED,
	ENTRY_POINTS_CHANGED,
	LAST_SIGNAL
};


struct _GthMonitorPrivateData {
	gboolean active;
};


static GObjectClass *parent_class = NULL;
static guint monitor_signals[LAST_SIGNAL] = { 0 };


static void
gth_monitor_finalize (GObject *object)
{
	GthMonitor *monitor = GTH_MONITOR (object);

	if (monitor->priv != NULL) {
		g_free (monitor->priv);
		monitor->priv = NULL;
	}

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gth_monitor_init (GthMonitor *monitor)
{
	monitor->priv = g_new0 (GthMonitorPrivateData, 1);
}


static void
gth_monitor_class_init (GthMonitorClass *class)
{
	GObjectClass  *gobject_class;

	parent_class = g_type_class_peek_parent (class);

	gobject_class = (GObjectClass*) class;
	gobject_class->finalize = gth_monitor_finalize;

	monitor_signals[ICON_THEME_CHANGED] =
		g_signal_new ("icon-theme-changed",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthMonitorClass, icon_theme_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
	monitor_signals[BOOKMARKS_CHANGED] =
		g_signal_new ("bookmarks-changed",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthMonitorClass, bookmarks_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
	monitor_signals[FILTERS_CHANGED] =
		g_signal_new ("filters-changed",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthMonitorClass, filters_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
	monitor_signals[FOLDER_CONTENT_CHANGED] =
		g_signal_new ("folder-changed",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthMonitorClass, folder_changed),
			      NULL, NULL,
			      gth_marshal_VOID__OBJECT_BOXED_ENUM,
			      G_TYPE_NONE,
			      3,
			      G_TYPE_OBJECT,
			      G_TYPE_OBJECT_LIST,
			      GTH_TYPE_MONITOR_EVENT);
	monitor_signals[FILE_RENAMED] =
		g_signal_new ("file-renamed",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthMonitorClass, file_renamed),
			      NULL, NULL,
			      gth_marshal_VOID__OBJECT_OBJECT,
			      G_TYPE_NONE,
			      2,
			      G_TYPE_OBJECT,
			      G_TYPE_OBJECT);
	monitor_signals[METADATA_CHANGED] =
		g_signal_new ("metadata-changed",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthMonitorClass, metadata_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE,
			      1,
			      G_TYPE_OBJECT);
	monitor_signals[ENTRY_POINTS_CHANGED] =
		g_signal_new ("entry-points-changed",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthMonitorClass, entry_points_changed),
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


GthMonitor *
gth_monitor_new (void)
{
	return (GthMonitor*) g_object_new (GTH_TYPE_MONITOR, NULL);
}


void
gth_monitor_pause (GthMonitor *monitor)
{
	monitor->priv->active = FALSE;
}


void
gth_monitor_resume (GthMonitor *monitor)
{
	monitor->priv->active = TRUE;
}


void
gth_monitor_icon_theme_changed (GthMonitor *monitor)
{
	g_return_if_fail (GTH_IS_MONITOR (monitor));

	g_signal_emit (G_OBJECT (monitor),
		       monitor_signals[ICON_THEME_CHANGED],
		       0);
}


void
gth_monitor_bookmarks_changed (GthMonitor *monitor)
{
	g_return_if_fail (GTH_IS_MONITOR (monitor));

	g_signal_emit (G_OBJECT (monitor),
		       monitor_signals[BOOKMARKS_CHANGED],
		       0);
}


void
gth_monitor_filters_changed (GthMonitor *monitor)
{
	g_return_if_fail (GTH_IS_MONITOR (monitor));

	g_signal_emit (G_OBJECT (monitor),
		       monitor_signals[FILTERS_CHANGED],
		       0);
}


void
gth_monitor_folder_changed (GthMonitor      *monitor,
			    GFile           *parent,
			    GList           *list,
			    GthMonitorEvent  event)
{
	g_return_if_fail (GTH_IS_MONITOR (monitor));

	g_signal_emit (G_OBJECT (monitor),
		       monitor_signals[FOLDER_CONTENT_CHANGED],
		       0,
		       parent,
		       list,
		       event);
}


void
gth_monitor_file_renamed (GthMonitor *monitor,
			  GFile      *file,
			  GFile      *new_file)
{
	g_return_if_fail (GTH_IS_MONITOR (monitor));

	g_signal_emit (G_OBJECT (monitor),
		       monitor_signals[FILE_RENAMED],
		       0,
		       file,
		       new_file);
}


void
gth_monitor_metadata_changed (GthMonitor  *monitor,
			      GthFileData *file_data)
{
	g_return_if_fail (GTH_IS_MONITOR (monitor));

	g_signal_emit (G_OBJECT (monitor),
		       monitor_signals[METADATA_CHANGED],
		       0,
		       file_data);
}


void
gth_monitor_file_entry_points_changed (GthMonitor *monitor)
{
	g_return_if_fail (GTH_IS_MONITOR (monitor));

	g_signal_emit (G_OBJECT (monitor),
		       monitor_signals[ENTRY_POINTS_CHANGED],
		       0);
}
