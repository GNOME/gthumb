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
	GCancellable *cancellable;
	GList        *files;
	GthCatalog   *catalog;
	ListReady     ready_func;
	gpointer      ready_data;
};


static GthFileSourceClass *parent_class = NULL;


static GList *
get_entry_points (GthFileSource *file_source)
{
	GList       *list = NULL;
	GFile       *file;
	GFileInfo   *info;

	file = g_file_new_for_uri ("catalog:///");
	info = gth_file_source_get_file_info (file_source, file);
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
					 GFile         *file)
{
	GFile     *gio_file;
	GFileInfo *file_info;

	gio_file = gth_catalog_file_to_gio_file (file);
	file_info = g_file_query_info (gio_file,
				       "standard::display-name,standard::icon",
				       G_FILE_QUERY_INFO_NONE,
				       NULL,
				       NULL);
	if (file_info == NULL)
		file_info = g_file_info_new ();
	update_file_info (file_source, file, file_info);

	g_object_unref (gio_file);

	return file_info;
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


static GthFileData *
create_file_data (GthFileSourceCatalogs *catalogs,
		  GFile                 *file,
		  GFileInfo             *info)
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
		update_file_info (GTH_FILE_SOURCE (catalogs), catalog_file, info);
		file_data = gth_file_data_new (catalog_file, info);

		g_object_unref (catalog_file);
		break;

	case G_FILE_TYPE_DIRECTORY:
		catalog_file = gth_catalog_file_from_gio_file (file, NULL);
		update_file_info (GTH_FILE_SOURCE (catalogs), catalog_file, info);
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
list__for_each_file_func (GFile     *file,
			  GFileInfo *info,
			  gpointer   user_data)
{
	GthFileSourceCatalogs *catalogs = user_data;
	GthFileData           *file_data;

	file_data = create_file_data (catalogs, file, info);
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
	g_cancellable_reset (catalogs->priv->cancellable);

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
					catalogs->priv->cancellable,
					catalog_list_ready_cb,
					file_source);
	}
	else
		g_directory_foreach_child (gio_file,
					   FALSE,
					   TRUE,
					   attributes,
					   catalogs->priv->cancellable,
					   list__start_dir_func,
					   list__for_each_file_func,
					   list__done_func,
					   file_source);

	g_object_unref (gio_file);
	g_free (uri);
}


typedef struct {
	GthFileSourceCatalogs *catalogs;
	ListReady              ready_func;
	gpointer               ready_data;
} ReadAttributesData;


static void
read_attributes_data_free (ReadAttributesData *data)
{
	g_free (data);
}


static void
info_ready_cb (GList    *files,
	       GError   *error,
	       gpointer  user_data)
{
	ReadAttributesData    *data = user_data;
	GthFileSourceCatalogs *catalogs = data->catalogs;
	GList                 *scan;
	GList                 *result_files;

	g_object_ref (catalogs);

	result_files = NULL;
	for (scan = files; scan; scan = scan->next) {
		GthFileData *file_data = scan->data;
		result_files = g_list_prepend (result_files, create_file_data (catalogs, file_data->file, file_data->info));
	}
	result_files = g_list_reverse (result_files);

	data->ready_func ((GthFileSource *) catalogs,
			  result_files,
			  error,
			  data->ready_data);

	_g_object_list_unref (result_files);
	read_attributes_data_free (data);
	g_object_unref (catalogs);

	if (G_IS_OBJECT (catalogs))
		gth_file_source_set_active (GTH_FILE_SOURCE (catalogs), FALSE);
}


static void
gth_file_source_catalogs_read_attributes (GthFileSource *file_source,
					  GList         *files,
					  const char    *attributes,
					  ListReady      func,
					  gpointer       user_data)
{
	GthFileSourceCatalogs *catalogs = (GthFileSourceCatalogs *) file_source;
	ReadAttributesData    *data;
	GList                 *gio_files;

	gth_file_source_set_active (GTH_FILE_SOURCE (catalogs), TRUE);

	data = g_new0 (ReadAttributesData, 1);
	data->catalogs = catalogs;
	data->ready_func = func;
	data->ready_data = user_data;

	gio_files = gth_file_source_to_gio_file_list (GTH_FILE_SOURCE (catalogs), files);
	g_query_info_async (gio_files,
			    GTH_FILE_DATA_ATTRIBUTES_WITH_FAST_CONTENT_TYPE,
			    catalogs->priv->cancellable,
			    info_ready_cb,
			    data);

	_g_object_list_unref (gio_files);
}


static void
gth_file_source_catalogs_cancel (GthFileSource *file_source)
{
	GthFileSourceCatalogs *catalogs = (GthFileSourceCatalogs *) file_source;

	if (gth_file_source_is_active (file_source))
		g_cancellable_cancel (catalogs->priv->cancellable);
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
	file_source_class->list = gth_file_source_catalogs_list;
	file_source_class->read_attributes = gth_file_source_catalogs_read_attributes;
	file_source_class->cancel = gth_file_source_catalogs_cancel;
}


static void
gth_file_source_catalogs_init (GthFileSourceCatalogs *catalogs)
{
	gth_file_source_add_scheme (GTH_FILE_SOURCE (catalogs), "catalog://");

	catalogs->priv = g_new0 (GthFileSourceCatalogsPrivate, 1);
	catalogs->priv->cancellable = g_cancellable_new ();
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
