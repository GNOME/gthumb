/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2009 Free Software Foundation, Inc.
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
#include <glib/gi18n.h>
#include <glib.h>
#include <gthumb.h>
#include "gth-catalog.h"


#define CATALOG_FORMAT "1.0"


struct _GthCatalogPrivate {
	GthCatalogType  type;
	GFile          *file;
	GList          *file_list;
	gboolean        active;
	char           *order;
	gboolean        order_inverse;
	GCancellable   *cancellable;
};


static gpointer parent_class = NULL;


static void
gth_catalog_finalize (GObject *object)
{
	GthCatalog *catalog = GTH_CATALOG (object);

	if (catalog->priv != NULL) {
		if (catalog->priv->file != NULL)
			g_object_unref (catalog->priv->file);
		_g_object_list_unref (catalog->priv->file_list);
		g_free (catalog->priv->order);
		g_free (catalog->priv);
		catalog->priv = NULL;
	}

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
read_catalog_data_from_xml (GthCatalog  *catalog,
		   	    const char  *buffer,
		   	    gsize        count,
		   	    GError     **error)
{
	DomDocument *doc;

	gth_catalog_set_file_list (catalog, NULL);

	doc = dom_document_new ();
	if (dom_document_load (doc, buffer, count, error)) {
		DomElement *root;
		DomElement *child;

		root = DOM_ELEMENT (doc)->first_child;
		for (child = root->first_child; child; child = child->next_sibling) {
			if (g_strcmp0 (child->tag_name, "files") == 0) {
				DomElement *file;

				for (file = child->first_child; file; file = file->next_sibling) {
					const char *uri;

					uri = dom_element_get_attribute (file, "uri");
					if (uri != NULL)
						catalog->priv->file_list = g_list_prepend (catalog->priv->file_list, g_file_new_for_uri (uri));
				}
			}
			if (g_strcmp0 (child->tag_name, "order") == 0)
				gth_catalog_set_order (catalog,
						       dom_element_get_attribute (child, "type"),
						       g_strcmp0 (dom_element_get_attribute (child, "inverse"), "1") == 0);
		}
		catalog->priv->file_list = g_list_reverse (catalog->priv->file_list);
	}

	g_object_unref (doc);
}


static void
read_catalog_data_old_format (GthCatalog *catalog,
			      const char *buffer,
			      gsize       count)
{
	GInputStream     *mem_stream;
	GDataInputStream *data_stream;
	gboolean          is_search;
	int               list_start;
	int               n_line;
	char             *line;

	mem_stream = g_memory_input_stream_new_from_data (buffer, count, NULL);
	data_stream = g_data_input_stream_new (mem_stream);

	is_search = (strncmp (buffer, "# Search", 8) == 0);
	if (is_search)
		list_start = 10;
	else
		list_start = 1;

	gth_catalog_set_file_list (catalog, NULL);

	n_line = 0;
	while ((line = g_data_input_stream_read_line (data_stream, NULL, NULL, NULL)) != NULL) {
		n_line++;

		if (is_search) {
			/* FIXME: read the search metadata here. */
		}

		if (n_line > list_start) {
			char *uri;

			uri = g_strndup (line + 1, strlen (line) - 2);
			catalog->priv->file_list = g_list_prepend (catalog->priv->file_list, g_file_new_for_uri (uri));

			g_free (uri);
		}
		g_free (line);
	}

	catalog->priv->file_list = g_list_reverse (catalog->priv->file_list);

	g_object_unref (data_stream);
	g_object_unref (mem_stream);
}


static void
base_load_from_data (GthCatalog  *catalog,
		     const void  *buffer,
		     gsize        count,
		     GError     **error)
{
	char *text_buffer;

	if (buffer == NULL)
		return;

	text_buffer = (char*) buffer;
	if (strncmp (text_buffer, "<?xml ", 6) == 0)
		read_catalog_data_from_xml (catalog, text_buffer, count, error);
	else
		read_catalog_data_old_format (catalog, text_buffer, count);
}


static char *
base_to_data (GthCatalog *catalog,
	      gsize      *length)
{
	DomDocument *doc;
	DomElement  *root;
	char        *data;

	doc = dom_document_new ();
	root = dom_document_create_element (doc, "catalog",
					    "version", CATALOG_FORMAT,
					    NULL);
	dom_element_append_child (DOM_ELEMENT (doc), root);
	if (catalog->priv->file_list != NULL) {
		DomElement *node;
		GList      *scan;

		if (catalog->priv->order != NULL)
			dom_element_append_child (root, dom_document_create_element (doc, "order",
										     "type", catalog->priv->order,
										     "inverse", (catalog->priv->order_inverse ? "1" : "0"),
										     NULL));

		node = dom_document_create_element (doc, "files", NULL);
		dom_element_append_child (root, node);

		for (scan = catalog->priv->file_list; scan; scan = scan->next) {
			GFile *file = scan->data;
			char  *uri;

			uri = g_file_get_uri (file);
			dom_element_append_child (DOM_ELEMENT (node), dom_document_create_element (doc, "file", "uri", uri, NULL));

			g_free (uri);
		}
	}
	data = dom_document_dump (doc, length);

	g_object_unref (doc);

	return data;
}


static void
gth_catalog_class_init (GthCatalogClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;

	object_class->finalize = gth_catalog_finalize;

	class->load_from_data = base_load_from_data;
	class->to_data = base_to_data;
}


static void
gth_catalog_init (GthCatalog *catalog)
{
	catalog->priv = g_new0 (GthCatalogPrivate, 1);
}


GType
gth_catalog_get_type (void)
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (GthCatalogClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_catalog_class_init,
			NULL,
			NULL,
			sizeof (GthCatalog),
			0,
			(GInstanceInitFunc) gth_catalog_init
		};

		type = g_type_register_static (G_TYPE_OBJECT,
					       "GthCatalog",
					       &type_info,
					       0);
	}

	return type;
}


GthCatalog *
gth_catalog_new (void)
{
	return (GthCatalog *) g_object_new (GTH_TYPE_CATALOG, NULL);
}


void
gth_catalog_set_file (GthCatalog *catalog,
		      GFile      *file)
{
	if (catalog->priv->file != NULL) {
		g_object_unref (catalog->priv->file);
		catalog->priv->file = NULL;
	}

	if (file != NULL)
		catalog->priv->file = g_file_dup (file);

	catalog->priv->type = GTH_CATALOG_TYPE_CATALOG;
}


GFile *
gth_catalog_get_file (GthCatalog *catalog)
{
	return catalog->priv->file;
}


void
gth_catalog_set_order (GthCatalog *catalog,
		       const char *order,
		       gboolean    inverse)
{
	g_free (catalog->priv->order);
	catalog->priv->order = NULL;

	if (order != NULL)
		catalog->priv->order = g_strdup (order);
	catalog->priv->order_inverse = inverse;
}


const char *
gth_catalog_get_order (GthCatalog *catalog,
		       gboolean   *inverse)
{
	*inverse = catalog->priv->order_inverse;
	return catalog->priv->order;
}


void
gth_catalog_load_from_data (GthCatalog  *catalog,
			    const void  *buffer,
			    gsize        count,
			    GError     **error)
{
	GTH_CATALOG_GET_CLASS (catalog)->load_from_data (catalog, buffer, count, error);
}


char *
gth_catalog_to_data (GthCatalog *catalog,
		     gsize      *length)
{
	return GTH_CATALOG_GET_CLASS (catalog)->to_data (catalog, length);
}


void
gth_catalog_set_file_list (GthCatalog *catalog,
			   GList      *file_list)
{
	_g_object_list_unref (catalog->priv->file_list);
	catalog->priv->file_list = NULL;

	if (file_list != NULL)
		catalog->priv->file_list = _g_file_list_dup (file_list);
}


GList *
gth_catalog_get_file_list (GthCatalog *catalog)
{
	return catalog->priv->file_list;
}


gboolean
gth_catalog_insert_file (GthCatalog *catalog,
			 int         pos,
			 GFile      *file)
{
	GList *link;

	link = g_list_find_custom (catalog->priv->file_list, file, (GCompareFunc) _g_file_cmp_uris);
	if (link != NULL)
		return FALSE;

	catalog->priv->file_list = g_list_insert (catalog->priv->file_list, g_file_dup (file), pos);

	return TRUE;
}


int
gth_catalog_remove_file (GthCatalog *catalog,
			 GFile      *file)
{
	GList *scan;
	int    i = 0;

	g_return_val_if_fail (catalog != NULL, -1);
	g_return_val_if_fail (file != NULL, -1);

	for (scan = catalog->priv->file_list; scan; scan = scan->next, i++)
		if (g_file_equal ((GFile *) scan->data, file))
			break;

	if (scan == NULL)
		return -1;

	catalog->priv->file_list = g_list_remove_link (catalog->priv->file_list, scan);
	_g_object_list_unref (scan);

	return i;
}


/* -- gth_catalog_list_async --  */


typedef struct {
	GthCatalog           *catalog;
	const char           *attributes;
	CatalogReadyCallback  list_ready_func;
	gpointer              user_data;
	GList                *current_file;
	GList                *files;
} ListData;


static void
gth_catalog_list_done (ListData *list_data,
		       GError   *error)
{
	GthCatalog *catalog = list_data->catalog;

	catalog->priv->active = FALSE;
	if (list_data->list_ready_func != NULL)
		list_data->list_ready_func (catalog, list_data->files, error, list_data->user_data);

	_g_object_list_unref (list_data->files);
	g_free (list_data);
}


static void
catalog_file_info_ready_cb (GObject      *source_object,
			    GAsyncResult *result,
			    gpointer      user_data)
{
	ListData   *list_data = user_data;
	GthCatalog *catalog = list_data->catalog;
	GFile      *file;
	GFileInfo  *info;

	file = (GFile*) source_object;
	info = g_file_query_info_finish (file, result, NULL);
	if (info != NULL) {
		list_data->files = g_list_prepend (list_data->files, gth_file_data_new (file, info));
		g_object_unref (info);
	}

	list_data->current_file = list_data->current_file->next;
	if (list_data->current_file == NULL) {
		gth_catalog_list_done (list_data, NULL);
		return;
	}

	g_file_query_info_async ((GFile *) list_data->current_file->data,
				 list_data->attributes,
				 0,
				 G_PRIORITY_DEFAULT,
				 catalog->priv->cancellable,
				 catalog_file_info_ready_cb,
				 list_data);
}


static void
list__catalog_buffer_ready_cb (void     *buffer,
			       gsize     count,
			       GError   *error,
			       gpointer  user_data)
{
	ListData   *list_data = user_data;
	GthCatalog *catalog = list_data->catalog;

	if ((error == NULL) && (buffer != NULL)) {
		gth_catalog_load_from_data (catalog, buffer,  count, &error);
		if (error != NULL) {
			gth_catalog_list_done (list_data, error);
			return;
		}

		list_data->current_file = catalog->priv->file_list;
		if (list_data->current_file == NULL) {
			gth_catalog_list_done (list_data, NULL);
			return;
		}

		g_file_query_info_async ((GFile *) list_data->current_file->data,
					 list_data->attributes,
					 0,
					 G_PRIORITY_DEFAULT,
					 catalog->priv->cancellable,
					 catalog_file_info_ready_cb,
					 list_data);
	}
	else
		gth_catalog_list_done (list_data, error);
}


void
gth_catalog_list_async (GthCatalog           *catalog,
			const char           *attributes,
			GCancellable         *cancellable,
			CatalogReadyCallback  ready_func,
			gpointer              user_data)
{
	ListData *list_data;

	g_return_if_fail (catalog->priv->file != NULL);

	if (catalog->priv->active) {
		/* FIXME: object_ready_with_error (catalog, ready_func, user_data, g_error_new (G_IO_ERROR, G_IO_ERROR_PENDING, "Action pending"));*/
		return;
	}

	catalog->priv->active = TRUE;
	catalog->priv->cancellable = cancellable;

	list_data = g_new0 (ListData, 1);
	list_data->catalog = catalog;
	list_data->attributes = attributes;
	list_data->list_ready_func = ready_func;
	list_data->user_data = user_data;

	g_load_file_async (catalog->priv->file,
			   G_PRIORITY_DEFAULT,
			   catalog->priv->cancellable,
			   list__catalog_buffer_ready_cb,
			   list_data);
}


void
gth_catalog_cancel (GthCatalog *catalog)
{
	g_cancellable_cancel (catalog->priv->cancellable);
}


/* utils */


GFile *
gth_catalog_get_base (void)
{
	char  *catalogs_dir;
	GFile *base;

	catalogs_dir = gth_user_dir_get_file (GTH_DIR_DATA, GTHUMB_DIR, "catalogs", NULL);
	/*catalogs_dir = g_strdup ("/home/paolo/.gnome2/gthumb/collections");*/
	base = g_file_new_for_path (catalogs_dir);

	g_free (catalogs_dir);

	return base;
}


GFile *
gth_catalog_file_to_gio_file (GFile *file)
{
	GFile *gio_file = NULL;
	char  *child_uri;

	child_uri = g_file_get_uri (file);
	if (strncmp (child_uri, "catalog:///", 11) == 0) {
		const char *query;

		query = strchr (child_uri, '?');
		if (query != NULL) {
			char *uri;

			uri = g_uri_unescape_string (query, "");
			gio_file = g_file_new_for_uri (uri);

			g_free (uri);
		}
		else {
			GFile      *base;
			char       *base_uri;
			const char *part;
			char       *full_uri;

			base = gth_catalog_get_base ();
			base_uri = g_file_get_uri (base);
			part = child_uri + 11;
			full_uri = g_strconcat (base_uri, part ? "/" : NULL, part, NULL);
			gio_file = g_file_new_for_uri (full_uri);

			g_free (base_uri);
			g_object_unref (base);
		}
	}
	else
		gio_file = g_file_dup (file);

	g_free (child_uri);

	return gio_file;
}


GFile *
gth_catalog_file_from_gio_file (GFile *gio_file,
				GFile *catalog)
{
	GFile *gio_base;
	GFile *file = NULL;
	char  *path;

	gio_base = gth_catalog_get_base ();
	if (g_file_equal (gio_base, gio_file)) {
		g_object_unref (gio_base);
		return g_file_new_for_uri ("catalog:///");
	}

	path = g_file_get_relative_path (gio_base, gio_file);
	if (path != NULL) {
		GFile *base;

		base = g_file_new_for_uri ("catalog:///");
		file = _g_file_append_path (base, path);

		g_object_unref (base);
	}
	else if (catalog != NULL) {
		char *catalog_uri;
		char *file_uri;
		char *query;
		char *uri;

		catalog_uri = g_file_get_uri (catalog);
		file_uri = g_file_get_uri (gio_file);
		query = g_uri_escape_string (file_uri, G_URI_RESERVED_CHARS_ALLOWED_IN_PATH, FALSE);
		uri = g_strconcat (file_uri, "?", query, NULL);
		file = g_file_new_for_uri (uri);

		g_free (uri);
		g_free (query);
		g_free (file_uri);
		g_free (catalog_uri);
	}

	g_free (path);
	g_object_unref (gio_base);

	return file;
}


GFile *
gth_catalog_file_from_relative_path (const char *name,
				     const char *file_extension)
{

	char  *partial_uri;
	char  *uri;
	GFile *file;

	partial_uri = g_uri_escape_string ((name[0] == '/' ? name + 1 : name), G_URI_RESERVED_CHARS_ALLOWED_IN_PATH, FALSE);
	uri = g_strconcat ("catalog:///", partial_uri, file_extension, NULL);
	file = g_file_new_for_uri (uri);

	g_free (uri);
	g_free (partial_uri);

	return file;
}


char *
gth_catalog_get_relative_path (GFile *file)
{
	GFile *base;
	char  *path;

	base = gth_catalog_get_base ();
	path = g_file_get_relative_path (base, file);

	g_object_unref (base);

	return path;
}


GIcon *
gth_catalog_get_icon (GFile *file)
{
	char  *uri;
	GIcon *icon;

	uri = g_file_get_uri (file);
	if (g_str_has_suffix (uri, ".catalog"))
		icon = g_themed_icon_new ("image-catalog");
	else
		icon = g_themed_icon_new ("image-library");

	g_free (uri);

	return icon;
}


char *
gth_catalog_get_display_name (GFile *file)
{
	char *display_name = NULL;
	char *basename;

	basename = g_file_get_basename (file);
	if ((basename != NULL) && (strcmp (basename, "/") != 0)) {
		char *name;

		name = _g_uri_remove_extension (basename);
		display_name = g_filename_to_utf8 (name, -1, NULL, NULL, NULL);

		g_free (name);
	}
	else
		display_name = g_strdup (_("Catalogs"));

	return display_name;
}


/* -- gth_catalog_load_from_file --*/


typedef struct {
	ReadyCallback ready_func;
	gpointer      user_data;
} LoadData;


static void
load__catalog_buffer_ready_cb (void     *buffer,
			       gsize     count,
			       GError   *error,
			       gpointer  user_data)
{
	LoadData   *load_data = user_data;
	GthCatalog *catalog = NULL;

	if (error == NULL) {
		catalog = gth_hook_invoke_get ("gth-catalog-load-from-data", buffer);
		if (catalog != NULL)
			gth_catalog_load_from_data (catalog, buffer, count, &error);
	}

	load_data->ready_func (G_OBJECT (catalog), error, load_data->user_data);

	g_free (load_data);
}


void
gth_catalog_load_from_file (GFile         *file,
		            GCancellable  *cancellable,
			    ReadyCallback  ready_func,
			    gpointer       user_data)
{
	LoadData *load_data;
	GFile    *gio_file;

	load_data = g_new0 (LoadData, 1);
	load_data->ready_func = ready_func;
	load_data->user_data = user_data;

	gio_file = gth_catalog_file_to_gio_file (file);
	g_load_file_async (gio_file,
			   G_PRIORITY_DEFAULT,
			   cancellable,
			   load__catalog_buffer_ready_cb,
			   load_data);

	g_object_unref (gio_file);
}
