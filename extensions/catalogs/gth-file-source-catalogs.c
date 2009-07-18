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

#include <config.h>
#include <string.h>
#include <glib/gi18n.h>
#include <glib.h>
#include <gthumb.h>
#include "gth-catalog.h"
#include "gth-file-source-catalogs.h"


struct _GthFileSourceCatalogsPrivate
{
	GList        *files;
	GthCatalog   *catalog;
	ListReady     ready_func;
	gpointer      ready_data;
};


static gpointer parent_class = NULL;


static GList *
get_entry_points (GthFileSource *file_source)
{
	GList       *list = NULL;
	GFile       *file;
	GFileInfo   *info;

	file = g_file_new_for_uri ("catalog:///");
	info = gth_file_source_get_file_info (file_source, file, GFILE_BASIC_ATTRIBUTES);
	list = g_list_append (list, gth_file_data_new (file, info));

	g_object_unref (info);
	g_object_unref (file);

	return list;
}


static GFile *
gth_file_source_catalogs_to_gio_file (GthFileSource *file_source,
				      GFile         *file)
{
	return gth_catalog_file_to_gio_file (file);
}


static void
update_file_info (GthFileSource *file_source,
		  GFile         *catalog_file,
		  GFileInfo     *info)
{
	char *uri;
	char *name;

	uri = g_file_get_uri (catalog_file);

	if (g_str_has_suffix (uri, ".gqv") || g_str_has_suffix (uri, ".catalog")) {
		g_file_info_set_file_type (info, G_FILE_TYPE_DIRECTORY);
		g_file_info_set_icon (info, g_themed_icon_new ("image-catalog"));
		g_file_info_set_sort_order (info, 1);
		g_file_info_set_attribute_boolean (info, "gthumb::no-child", TRUE);

		name = gth_catalog_get_display_name (catalog_file);
		g_file_info_set_display_name (info, name);
		g_free (name);
	}
	else if (g_str_has_suffix (uri, ".search")) {
		g_file_info_set_file_type (info, G_FILE_TYPE_DIRECTORY);
		g_file_info_set_icon (info, g_themed_icon_new ("image-search"));
		g_file_info_set_sort_order (info, 1);
		g_file_info_set_attribute_boolean (info, "gthumb::no-child", TRUE);

		name = gth_catalog_get_display_name (catalog_file);
		g_file_info_set_display_name (info, name);
		g_free (name);
	}
	else {
		g_file_info_set_file_type (info, G_FILE_TYPE_DIRECTORY);
		g_file_info_set_icon (info, g_themed_icon_new ("image-library"));
		g_file_info_set_sort_order (info, 0);
		g_file_info_set_attribute_boolean (info, "gthumb::no-child", FALSE);

		name = gth_catalog_get_display_name (catalog_file);
		g_file_info_set_display_name (info, name);
		g_free (name);
	}

	g_free (uri);
}


static GFileInfo *
gth_file_source_catalogs_get_file_info (GthFileSource *file_source,
					GFile         *file,
					const char    *attributes)
{
	GFile     *gio_file;
	GFileInfo *file_info;

	gio_file = gth_catalog_file_to_gio_file (file);
	file_info = g_file_query_info (gio_file,
				       attributes,
				       G_FILE_QUERY_INFO_NONE,
				       NULL,
				       NULL);
	if (file_info == NULL)
		file_info = g_file_info_new ();
	update_file_info (file_source, file, file_info);

	g_object_unref (gio_file);

	return file_info;
}


static GthFileData *
gth_file_source_catalogs_get_file_data (GthFileSource *file_source,
					GFile         *file,
					GFileInfo     *info)
{
	GthFileData *file_data = NULL;
	char        *uri;
	GFile       *catalog_file;

	uri = g_file_get_uri (file);

	switch (g_file_info_get_file_type (info)) {
	case G_FILE_TYPE_REGULAR:
		if (! g_str_has_suffix (uri, ".gqv")
		    && ! g_str_has_suffix (uri, ".catalog")
		    && ! g_str_has_suffix (uri, ".search"))
		{
			file_data = gth_file_data_new (file, info);
			break;
		}

		catalog_file = gth_catalog_file_from_gio_file (file, NULL);
		update_file_info (file_source, catalog_file, info);
		file_data = gth_file_data_new (catalog_file, info);

		g_object_unref (catalog_file);
		break;

	case G_FILE_TYPE_DIRECTORY:
		catalog_file = gth_catalog_file_from_gio_file (file, NULL);
		update_file_info (file_source, catalog_file, info);
		file_data = gth_file_data_new (catalog_file, info);

		g_object_unref (catalog_file);
		break;

	default:
		break;
	}

	g_free (uri);

	return file_data;
}



static void
list__done_func (GError   *error,
		 gpointer  user_data)
{
	GthFileSourceCatalogs *catalogs = user_data;

	if (G_IS_OBJECT (catalogs))
		gth_file_source_set_active (GTH_FILE_SOURCE (catalogs), FALSE);

	if (error != NULL) {
		_g_object_list_unref (catalogs->priv->files);
		catalogs->priv->files = NULL;
		g_clear_error (&error);
	}

	g_object_ref (catalogs);
	catalogs->priv->ready_func ((GthFileSource *) catalogs,
				    catalogs->priv->files,
				    error,
				    catalogs->priv->ready_data);
	g_object_unref (catalogs);
}


static void
list__for_each_file_func (GFile     *file,
			  GFileInfo *info,
			  gpointer   user_data)
{
	GthFileSourceCatalogs *catalogs = user_data;
	GthFileData           *file_data;

	file_data = gth_file_source_get_file_data (GTH_FILE_SOURCE (catalogs), file, info);
	if (file_data != NULL)
		catalogs->priv->files = g_list_prepend (catalogs->priv->files, file_data);
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
catalog_list_ready_cb (GthCatalog *catalog,
		       GList      *files,
		       GError     *error,
		       gpointer    user_data)
{
	GthFileSourceCatalogs *catalogs = user_data;

	g_object_ref (catalogs);

	catalogs->priv->ready_func ((GthFileSource *) catalogs,
				    files,
				    error,
				    catalogs->priv->ready_data);

	gth_catalog_set_file_list (catalogs->priv->catalog, NULL);
	g_object_unref (catalogs);

	if (G_IS_OBJECT (catalogs))
		gth_file_source_set_active (GTH_FILE_SOURCE (catalogs), FALSE);
}


static void
gth_file_source_catalogs_list (GthFileSource *file_source,
			       GFile         *folder,
			       const char    *attributes,
			       ListReady      func,
			       gpointer       user_data)
{
	GthFileSourceCatalogs *catalogs = (GthFileSourceCatalogs *) file_source;
	char                  *uri;
	GFile                 *gio_file;

	gth_file_source_set_active (GTH_FILE_SOURCE (catalogs), TRUE);
	g_cancellable_reset (gth_file_source_get_cancellable (file_source));

	_g_object_list_unref (catalogs->priv->files);
	catalogs->priv->files = NULL;

	catalogs->priv->ready_func = func;
	catalogs->priv->ready_data = user_data;

	uri = g_file_get_uri (folder);
	gio_file = gth_file_source_to_gio_file (file_source, folder);

	if (g_str_has_suffix (uri, ".gqv")
	    || g_str_has_suffix (uri, ".catalog")
	    || g_str_has_suffix (uri, ".search"))
	{
		gth_catalog_set_file (catalogs->priv->catalog, gio_file);
		gth_catalog_list_async (catalogs->priv->catalog,
					attributes,
					gth_file_source_get_cancellable (file_source),
					catalog_list_ready_cb,
					file_source);
	}
	else
		g_directory_foreach_child (gio_file,
					   FALSE,
					   TRUE,
					   attributes,
					   gth_file_source_get_cancellable (file_source),
					   list__start_dir_func,
					   list__for_each_file_func,
					   list__done_func,
					   file_source);

	g_object_unref (gio_file);
	g_free (uri);
}


/* -- gth_file_source_vfs_copy -- */


typedef struct {
	GthFileSourceVfs *file_source;
	GthFileData      *destination;
	GList            *file_list;
	ProgressCallback  progress_callback;
	ReadyCallback     ready_callback;
	gpointer          user_data;
	GList            *files;
	GthCatalog       *catalog;
	char             *buffer;
	gsize             length;
} CopyOpData;


static void
copy_op_data_free (CopyOpData *cod)
{
	g_free (cod->buffer);
	_g_object_unref (cod->catalog);
	_g_object_list_unref (cod->files);
	_g_object_list_unref (cod->file_list);
	g_object_unref (cod->destination);
	g_object_unref (cod->file_source);
	g_free (cod);
}


static void
catalog_save_done_cb (void     *buffer,
		      gsize     count,
		      GError   *error,
		      gpointer  user_data)
{
	CopyOpData *cod = user_data;

	if (error == NULL)
		gth_monitor_folder_changed (gth_main_get_default_monitor (),
				            cod->destination->file,
				            cod->files,
					    GTH_MONITOR_EVENT_CREATED);

	cod->ready_callback (G_OBJECT (cod->file_source), error, cod->user_data);
	copy_op_data_free (cod);
}


static void
catalog_ready_cb (GObject  *catalog,
		  GError   *error,
		  gpointer  user_data)
{
	CopyOpData *cod = user_data;
	GList      *scan;
	GFile      *gio_file;

	if (error != NULL) {
		cod->ready_callback (G_OBJECT (cod->file_source), error, cod->user_data);
		copy_op_data_free (cod);
		return;
	}

	cod->catalog = (GthCatalog *) catalog;

	for (scan = cod->files; scan; scan = scan->next)
		gth_catalog_insert_file (cod->catalog, -1, (GFile *) scan->data);

	cod->buffer = gth_catalog_to_data (cod->catalog, &cod->length);
	gio_file = gth_catalog_file_to_gio_file (cod->destination->file);
	g_write_file_async (gio_file,
			    cod->buffer,
			    cod->length,
			    G_PRIORITY_DEFAULT,
			    NULL,
			    catalog_save_done_cb,
			    cod);

	g_object_unref (gio_file);
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
		case G_FILE_TYPE_REGULAR:
		case G_FILE_TYPE_SYMBOLIC_LINK:
			cod->files = g_list_prepend (cod->files, g_object_ref (file_data->file));
			break;
		default:
			break;
		}
	}
	cod->files = g_list_reverse (cod->files);

	gth_catalog_load_from_file (cod->destination->file, catalog_ready_cb, cod);
}


static void
gth_file_source_catalogs_copy (GthFileSource    *file_source,
			       GthFileData      *destination,
			       GList            *file_list, /* GFile * list */
			       ProgressCallback  progress_callback,
			       ReadyCallback     ready_callback,
			       gpointer          data)
{
	CopyOpData *cod;

	cod = g_new0 (CopyOpData, 1);
	cod->file_source = g_object_ref (file_source);
	cod->destination = g_object_ref (destination);
	cod->file_list = _g_object_list_ref (file_list);
	cod->progress_callback = progress_callback;
	cod->ready_callback = ready_callback;
	cod->user_data = data;

	if (cod->progress_callback != NULL) {
		char *message;

		message = g_strdup_printf (_("Copying files to '%s'"), g_file_info_get_display_name (destination->info));
		(cod->progress_callback) (G_OBJECT (file_source), message, NULL, TRUE, 0.0, cod->user_data);

		g_free (message);
	}

	g_query_info_async (cod->file_list,
			    G_FILE_ATTRIBUTE_STANDARD_TYPE,
			    gth_file_source_get_cancellable (file_source),
			    copy__file_list_info_ready_cb,
			    cod);
}


static void
gth_file_source_catalogs_finalize (GObject *object)
{
	GthFileSourceCatalogs *catalogs = GTH_FILE_SOURCE_CATALOGS (object);

	if (catalogs->priv != NULL) {
		g_object_unref (catalogs->priv->catalog);
		_g_object_list_unref (catalogs->priv->files);
		catalogs->priv->files = NULL;

		g_free (catalogs->priv);
		catalogs->priv = NULL;
	}

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gth_file_source_catalogs_class_init (GthFileSourceCatalogsClass *class)
{
	GObjectClass       *object_class;
	GthFileSourceClass *file_source_class;

	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;
	file_source_class = (GthFileSourceClass*) class;

	object_class->finalize = gth_file_source_catalogs_finalize;
	file_source_class->get_entry_points = get_entry_points;
	file_source_class->to_gio_file = gth_file_source_catalogs_to_gio_file;
	file_source_class->get_file_info = gth_file_source_catalogs_get_file_info;
	file_source_class->get_file_data = gth_file_source_catalogs_get_file_data;
	file_source_class->list = gth_file_source_catalogs_list;
	file_source_class->copy = gth_file_source_catalogs_copy;
}


static void
gth_file_source_catalogs_init (GthFileSourceCatalogs *catalogs)
{
	gth_file_source_add_scheme (GTH_FILE_SOURCE (catalogs), "catalog://");

	catalogs->priv = g_new0 (GthFileSourceCatalogsPrivate, 1);
	catalogs->priv->catalog = gth_catalog_new ();
}


GType
gth_file_source_catalogs_get_type (void)
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (GthFileSourceCatalogsClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_file_source_catalogs_class_init,
			NULL,
			NULL,
			sizeof (GthFileSourceCatalogs),
			0,
			(GInstanceInitFunc) gth_file_source_catalogs_init
		};

		type = g_type_register_static (GTH_TYPE_FILE_SOURCE,
					       "GthFileSourceCatalogs",
					       &type_info,
					       0);
	}

	return type;
}
