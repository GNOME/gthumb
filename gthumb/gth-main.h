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
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 */

#ifndef GTH_MAIN_H
#define GTH_MAIN_H

#include <glib.h>
#include <glib-object.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "gth-file-data.h"
#include "gth-extensions.h"
#include "gth-file-source.h"
#include "gth-filter-file.h"
#include "gth-hook.h"
#include "gth-metadata-provider.h"
#include "gth-monitor.h"
#include "gth-tags-file.h"
#include "gth-test.h"

G_BEGIN_DECLS

#define GTH_TYPE_MAIN         (gth_main_get_type ())
#define GTH_MAIN(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GTH_TYPE_MAIN, GthMain))
#define GTH_MAIN_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GTH_TYPE_MAIN, GthMainClass))
#define GTH_IS_MAIN(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTH_TYPE_MAIN))
#define GTH_IS_MAIN_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GTH_TYPE_MAIN))
#define GTH_MAIN_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), GTH_TYPE_MAIN, GthMainClass))

typedef struct _GthMain         GthMain;
typedef struct _GthMainPrivate  GthMainPrivate;
typedef struct _GthMainClass    GthMainClass;

typedef GdkPixbufAnimation* (*FileLoader) (GthFileData  *file_data,
				   	   GError      **error,
				   	   int           requested_width,
				   	   int           requested_height);

struct _GthMain {
	GObject __parent;
	GthMainPrivate *priv;
};

struct _GthMainClass {
	GObjectClass __parent_class;
};

GType                  gth_main_get_type                      (void) G_GNUC_CONST;
void                   gth_main_initialize                    (void);
void                   gth_main_release                       (void);
void                   gth_main_register_file_source          (GType                 file_source_type);
GthFileSource *        gth_main_get_file_source_for_uri       (const char           *uri);
GthFileSource *        gth_main_get_file_source               (GFile                *file);
GList *                gth_main_get_all_file_sources          (void);
GList *                gth_main_get_all_entry_points          (void);
char *                 gth_main_get_gio_uri                   (const char           *uri);
GFile *                gth_main_get_gio_file                  (GFile                *file);
void                   gth_main_register_metadata_category    (GthMetadataCategory  *metadata_category);
GthMetadataInfo *      gth_main_register_metadata_info        (GthMetadataInfo      *metadata_info);
void                   gth_main_register_metadata_info_v      (GthMetadataInfo       metadata_info[]);
void                   gth_main_register_metadata_provider    (GType                 metadata_provider_type);
GList *                gth_main_get_all_metadata_providers    (void);
char **                gth_main_get_metadata_attributes       (const char           *mask);
GthMetadataProvider *  gth_main_get_metadata_reader           (const char           *id);
GthMetadataCategory *  gth_main_get_metadata_category         (const char           *id);
GthMetadataInfo *      gth_main_get_metadata_info             (const char           *id);
GPtrArray *            gth_main_get_all_metadata_info         (void);
void                   gth_main_register_sort_type            (GthFileDataSort      *sort_type);
GthFileDataSort *      gth_main_get_sort_type                 (const char           *name);
GList *                gth_main_get_all_sort_types            (void);
void                   gth_main_register_test                 (const char           *id,
						               GType                 type,
						               const char           *first_property,
						              ...);
GthTest *              gth_main_get_test                      (const char           *id);
GList *                gth_main_get_all_tests                 (void);
void                   gth_main_register_file_loader          (FileLoader            loader,
						               const char           *first_mime_type,
						               ...);
FileLoader             gth_main_get_file_loader               (const char           *mime_type);
GthTest *              gth_main_get_general_filter            (void);
GthTest *              gth_main_add_general_filter            (GthTest              *filter);
void                   gth_main_register_object               (const char           *set_name,
							       GType                 object_type);
GPtrArray *            gth_main_get_object_set                (const char           *set_name);
void                   gth_main_register_type                 (const char           *set_name,
							       GType                 object_type);
GArray *               gth_main_get_type_set                  (const char           *set_name);
GBookmarkFile *        gth_main_get_default_bookmarks         (void);
void                   gth_main_bookmarks_changed             (void);
GthFilterFile *        gth_main_get_default_filter_file       (void);
GList *                gth_main_get_all_filters               (void);
void                   gth_main_filters_changed               (void);
GthTagsFile *          gth_main_get_default_tag_file          (void);
const char **          gth_main_get_all_tags                  (void);
void                   gth_main_tags_changed                  (void);
GthMonitor *           gth_main_get_default_monitor           (void);
GthExtensionManager *  gth_main_get_default_extension_manager (void);
void                   gth_main_register_default_hooks        (void);
void                   gth_main_register_default_tests        (void);
void                   gth_main_register_default_types        (void);
void                   gth_main_register_default_sort_types   (void);
void                   gth_main_register_default_metadata     (void);
void                   gth_main_activate_extensions           (void);

G_END_DECLS

#endif /* GTH_MAIN_H */
