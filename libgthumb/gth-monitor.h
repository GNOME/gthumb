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

#ifndef GTH_MONITOR_H
#define GTH_MONITOR_H

#include "typedefs.h"

#define GTH_TYPE_MONITOR              (gth_monitor_get_type ())
#define GTH_MONITOR(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_MONITOR, GthMonitor))
#define GTH_MONITOR_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_TYPE_MONITOR, GthMonitorClass))
#define GTH_IS_MONITOR(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_MONITOR))
#define GTH_IS_MONITOR_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_MONITOR))
#define GTH_MONITOR_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_MONITOR, GthMonitorClass))

typedef struct _GthMonitor            GthMonitor;
typedef struct _GthMonitorClass       GthMonitorClass;
typedef struct _GthMonitorPrivateData GthMonitorPrivateData;

struct _GthMonitor
{
	GObject __parent;
	GthMonitorPrivateData *priv;
};

struct _GthMonitorClass
{
	GObjectClass __parent_class;

	/*< signals >*/

	void        (*update_icon_theme)   (GthMonitor      *monitor);
	void        (*update_bookmarks)    (GthMonitor      *monitor);
	void        (*update_cat_files)    (GthMonitor      *monitor,
					    const char      *catalog_path,
					    GthMonitorEvent  event,
					    GList           *list);
	void        (*update_files)        (GthMonitor      *monitor,
					    GthMonitorEvent  event,
					    GList           *list);
	void        (*update_directory)    (GthMonitor      *monitor,
					    const char      *dir_path,
					    GthMonitorEvent  event);
	void        (*update_catalog)      (GthMonitor      *monitor,
					    const char      *catalog_path,
					    GthMonitorEvent  event);
	void        (*update_metadata)     (GthMonitor      *monitor,
					    const char      *path);
	void        (*file_renamed)        (GthMonitor      *monitor,
					    const char      *old_name,
					    const char      *new_name);
	void        (*directory_renamed)   (GthMonitor      *monitor,
					    const char      *old_name,
					    const char      *new_name);
	void        (*catalog_renamed)     (GthMonitor      *monitor,
					    const char      *old_name,
					    const char      *new_name);
	void        (*reload_catalogs)     (GthMonitor      *monitor);
};

GType        gth_monitor_get_type                    (void);
GthMonitor*  gth_monitor_get_instance                (void);
void         gth_monitor_add_gfile                   (GFile *gfile);
void         gth_monitor_remove_gfile                (GFile *gfile);
void         gth_monitor_pause                       (void);
gboolean     gth_monitor_resume                      (void);
void         gth_monitor_notify_update_icon_theme    (void);
void         gth_monitor_notify_update_bookmarks     (void);
void         gth_monitor_notify_update_cat_files     (const char      *catalog_path,
						      GthMonitorEvent  event,
						      GList           *list);
void         gth_monitor_notify_update_gfiles        (GthMonitorEvent  event,
                                                      GList           *list);
void         gth_monitor_notify_update_files         (GthMonitorEvent  event,
						      GList           *list);
void         gth_monitor_notify_update_directory     (const char      *dir_path,
						      GthMonitorEvent  event);
void         gth_monitor_notify_update_catalog       (const char      *catalog_path,
						      GthMonitorEvent  event);
void         gth_monitor_notify_update_metadata      (const char      *path);
void         gth_monitor_notify_file_renamed         (const char      *old_name,
						      const char      *new_name);
void         gth_monitor_notify_directory_renamed    (const char      *old_name,
						      const char      *new_name);
void         gth_monitor_notify_catalog_renamed      (const char      *old_name,
						      const char      *new_name);
void         gth_monitor_notify_reload_catalogs      (void);

#endif /* GTH_MONITOR_H */
