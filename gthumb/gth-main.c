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
#include <glib-object.h>
#include <gobject/gvaluecollector.h>
#include "gconf-utils.h"
#include "glib-utils.h"
#include "gth-duplicable.h"
#include "gth-filter.h"
#include "gth-main.h"
#include "gth-metadata-provider.h"
#include "gth-user-dir.h"
#include "gth-preferences.h"
#include "pixbuf-io.h"
#include "typedefs.h"


typedef struct {
	GType       object_type;
	guint       n_params;
	GParameter *params;
} GthTypeSpec;


static GthTypeSpec *
gth_type_spec_new (GType object_type)
{
	GthTypeSpec *spec;

	spec = g_new0 (GthTypeSpec, 1);
	spec->object_type = object_type;
	spec->n_params = 0;
	spec->params = NULL;

	return spec;
}


static void
gth_type_spec_free (GthTypeSpec *spec)
{
	while (spec->n_params--)
		g_value_unset (&spec->params[spec->n_params].value);
	g_free (spec->params);
	g_free (spec);
}


static GObject *
gth_type_spec_create_object (GthTypeSpec *spec,
			     const char  *object_id)
{
	GObject *object;

	object = g_object_newv (spec->object_type, spec->n_params, spec->params);
	g_object_set (object, "id", object_id, NULL);

	return object;
}


static GthMain *Main = NULL;


struct _GthMainPrivate
{
	GList               *file_sources;
	GList               *metadata_provider;
	GPtrArray           *metadata_category;
	GPtrArray           *metadata_info;
	GHashTable          *sort_types;
	GHashTable          *tests;
	GHashTable          *loaders;
	GList               *viewer_pages;
	GHashTable          *types;
	GHashTable          *objects;
	GBookmarkFile       *bookmarks;
	GthFilterFile       *filters;
	GthMonitor          *monitor;
	GthExtensionManager *extension_manager;
};


static GObjectClass *parent_class = NULL;


static void
gth_main_finalize (GObject *object)
{
	GthMain *gth_main = GTH_MAIN (object);

	if (gth_main->priv != NULL) {
		_g_object_list_unref (gth_main->priv->file_sources);
		_g_object_list_unref (gth_main->priv->viewer_pages);

		g_ptr_array_free (gth_main->priv->metadata_category, TRUE);
		g_ptr_array_free (gth_main->priv->metadata_info, TRUE);
		g_list_foreach (gth_main->priv->metadata_provider, (GFunc) g_object_unref, NULL);
		g_list_free (gth_main->priv->metadata_provider);

		g_hash_table_unref (gth_main->priv->sort_types);
		g_hash_table_unref (gth_main->priv->tests);
		g_hash_table_unref (gth_main->priv->loaders);

		g_hash_table_unref (gth_main->priv->types);

		if (gth_main->priv->bookmarks != NULL)
			g_bookmark_file_free (gth_main->priv->bookmarks);
		if (gth_main->priv->monitor != NULL)
			g_object_unref (gth_main->priv->monitor);
		if (gth_main->priv->extension_manager != NULL)
			g_object_unref (gth_main->priv->extension_manager);

		g_free (gth_main->priv);
		gth_main->priv = NULL;
	}

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gth_main_class_init (GthMainClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;

	object_class->finalize = gth_main_finalize;
}


static void
gth_main_init (GthMain *main)
{
	main->priv = g_new0 (GthMainPrivate, 1);
	main->priv->sort_types = g_hash_table_new_full (g_str_hash,
							g_str_equal,
							NULL,
							NULL);
	main->priv->tests = g_hash_table_new_full (g_str_hash,
						   (GEqualFunc) g_content_type_equals,
						   NULL,
						   (GDestroyNotify) gth_type_spec_free);
	main->priv->loaders = g_hash_table_new (g_str_hash, g_str_equal);
	main->priv->metadata_category = g_ptr_array_new ();
	main->priv->metadata_info = g_ptr_array_new ();
}


GType
gth_main_get_type (void)
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (GthMainClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_main_class_init,
			NULL,
			NULL,
			sizeof (GthMain),
			0,
			(GInstanceInitFunc) gth_main_init
		};

		type = g_type_register_static (G_TYPE_OBJECT,
					       "GthMain",
					       &type_info,
					       0);
	}

	return type;
}


void
gth_main_initialize (void)
{
	if (Main != NULL)
		return;
	Main = (GthMain*) g_object_new (GTH_TYPE_MAIN, NULL);
}


void
gth_main_release (void)
{
	if (Main != NULL)
		g_object_unref (Main);
}


void
gth_main_register_file_source (GType file_source_type)
{
	GObject *file_source;

	file_source = g_object_new (file_source_type, NULL);
	Main->priv->file_sources = g_list_append (Main->priv->file_sources, file_source);
}


GthFileSource *
gth_main_get_file_source_for_uri (const char *uri)
{
	GthFileSource *file_source = NULL;
	GList         *scan;

	for (scan = Main->priv->file_sources; scan; scan = scan->next) {
		GthFileSource *registered_file_source = scan->data;

		if (gth_file_source_supports_scheme (registered_file_source, uri)) {
			file_source = g_object_new (G_OBJECT_TYPE (registered_file_source), NULL);
			break;
		}
	}

	if ((file_source == NULL) && ! g_str_has_prefix (uri, "vfs+")) {
		char *vfs_uri;

		vfs_uri = g_strconcat ("vfs+", uri, NULL);
		file_source = gth_main_get_file_source_for_uri (vfs_uri);
		g_free (vfs_uri);
	}

	return file_source;
}


GthFileSource *
gth_main_get_file_source (GFile *file)
{
	char          *uri;
	GthFileSource *file_source;

	uri = g_file_get_uri (file);
	file_source = gth_main_get_file_source_for_uri (uri);
	g_free (uri);

	return file_source;
}

GList *
gth_main_get_all_file_sources (void)
{
	return Main->priv->file_sources;
}


GList *
gth_main_get_all_entry_points (void)
{
	GList *list = NULL;
	GList *scan;

	for (scan = gth_main_get_all_file_sources (); scan; scan = scan->next) {
		GthFileSource *file_source = scan->data;
		GList         *entry_points;
		GList         *scan_entry;

		entry_points = gth_file_source_get_entry_points (file_source);
		for (scan_entry = entry_points; scan_entry; scan_entry = scan_entry->next) {
			GthFileData *file_data = scan_entry->data;

			list = g_list_prepend (list, g_object_ref (file_data));
		}

		_g_object_list_unref (entry_points);
	}

	return g_list_reverse (list);
}


char *
gth_main_get_gio_uri (const char *uri)
{
	GthFileSource *file_source;
	GFile         *file;
	GFile         *gio_file;
	char          *gio_uri;

	file_source = gth_main_get_file_source_for_uri (uri);
	file = g_file_new_for_uri (uri);
	gio_file = gth_file_source_to_gio_file (file_source, file);
	gio_uri = g_file_get_uri (gio_file);

	g_object_unref (gio_file);
	g_object_unref (file);
	g_object_unref (file_source);

	return gio_uri;
}


GFile *
gth_main_get_gio_file (GFile *file)
{
	GthFileSource *file_source;
	GFile         *gio_file;

	file_source = gth_main_get_file_source (file);
	gio_file = gth_file_source_to_gio_file (file_source, file);
	g_object_unref (file_source);

	return gio_file;
}


void
gth_main_register_metadata_category (GthMetadataCategory *metadata_category)
{
	int i;

	for (i = 0; metadata_category[i].id != NULL; i++)
		if (gth_main_get_metadata_category (metadata_category[i].id) == NULL)
			g_ptr_array_add (Main->priv->metadata_category, &metadata_category[i]);
}


static GStaticMutex metadata_info_mutex = G_STATIC_MUTEX_INIT;


GthMetadataInfo *
gth_main_register_metadata_info (GthMetadataInfo *metadata_info)
{
	GthMetadataInfo *info;

	info = gth_metadata_info_dup (metadata_info);

	g_static_mutex_lock (&metadata_info_mutex);
	g_ptr_array_add (Main->priv->metadata_info, info);
	g_static_mutex_unlock (&metadata_info_mutex);

	return info;
}


void
gth_main_register_metadata_info_v (GthMetadataInfo metadata_info[])
{
	int i;

	g_static_mutex_lock (&metadata_info_mutex);
	for (i = 0; metadata_info[i].id != NULL; i++)
		g_ptr_array_add (Main->priv->metadata_info, &metadata_info[i]);
	g_static_mutex_unlock (&metadata_info_mutex);
}


void
gth_main_register_metadata_provider (GType metadata_provider_type)
{
	GObject *metadata;

	metadata = g_object_new (metadata_provider_type, NULL);
	Main->priv->metadata_provider = g_list_append (Main->priv->metadata_provider, metadata);
}


GList *
gth_main_get_all_metadata_providers (void)
{
	return Main->priv->metadata_provider;
}


char **
gth_main_get_metadata_attributes (const char *mask)
{
	GList                  *list = NULL;
	GFileAttributeMatcher  *matcher;
	int                     i;
	int                     n;
	GList                  *scan;
	char                  **values;

	matcher = g_file_attribute_matcher_new (mask);
	for (n = 0, i = 0; i < Main->priv->metadata_info->len; i++) {
		GthMetadataInfo *metadata_info = g_ptr_array_index (Main->priv->metadata_info, i);

		if (g_file_attribute_matcher_matches (matcher, metadata_info->id)) {
			list = g_list_append (list, (char *) metadata_info->id);
			n++;
		}
	}
	list = g_list_reverse (list);

	values = g_new (char *, n + 1);
	for (i = 0, scan = list; scan; i++, scan = scan->next)
		values[i] = g_strdup (scan->data);
	values[n] = NULL;

	g_list_free (list);
	g_file_attribute_matcher_unref (matcher);

	return values;
}


GthMetadataProvider *
gth_main_get_metadata_reader (const char *id)
{
	GthMetadataProvider *metadata = NULL;
	GList               *scan;
	const char          *attribute_v[] = { id, NULL };

	for (scan = Main->priv->metadata_provider; scan; scan = scan->next) {
		GthMetadataProvider *registered_metadata = scan->data;

		if (gth_metadata_provider_can_read (registered_metadata, (char **)attribute_v)) {
			metadata = g_object_new (G_OBJECT_TYPE (registered_metadata), NULL);
			break;
		}
	}

	return metadata;
}


GthMetadataCategory *
gth_main_get_metadata_category (const char *id)
{
	int i;

	for (i = 0; i < Main->priv->metadata_category->len; i++) {
		GthMetadataCategory *category;

		category = g_ptr_array_index (Main->priv->metadata_category, i);
		if (strcmp (category->id, id) == 0)
			return category;
	}

	return NULL;
}


GthMetadataInfo *
gth_main_get_metadata_info (const char *id)
{
	int i;

	for (i = 0; i < Main->priv->metadata_info->len; i++) {
		GthMetadataInfo *info;

		info = g_ptr_array_index (Main->priv->metadata_info, i);
		if (strcmp (info->id, id) == 0)
			return info;
	}

	return NULL;
}


GPtrArray *
gth_main_get_all_metadata_info (void)
{
	return Main->priv->metadata_info;
}


void
gth_main_register_sort_type (GthFileDataSort *sort_type)
{
	g_hash_table_insert (Main->priv->sort_types, (gpointer) sort_type->name, sort_type);
}


GthFileDataSort *
gth_main_get_sort_type (const char *name)
{
	if (name == NULL)
		return NULL;
	else
		return g_hash_table_lookup (Main->priv->sort_types, name);
}


static void
collect_sort_types (gpointer key,
		    gpointer value,
		    gpointer user_data)
{
	GList           **sort_types = user_data;
	GthFileDataSort  *sort_type = value;

	*sort_types = g_list_prepend (*sort_types, sort_type);
}


GList *
gth_main_get_all_sort_types (void)
{
	GList *sort_types = NULL;

	g_hash_table_foreach (Main->priv->sort_types, collect_sort_types, &sort_types);
	return g_list_reverse (sort_types);
}


static GthTypeSpec *
_gth_main_create_type_spec (GType       object_type,
			    const char *first_property_name,
			    va_list     var_args)
{
	GObject     *object;
	GthTypeSpec *type_spec;
	const char  *name;
	guint        n_alloced_params = 16;

	type_spec = gth_type_spec_new (object_type);

	if (first_property_name == NULL)
    		return type_spec;

    	object = g_object_new (object_type, NULL);

    	type_spec->params = g_new (GParameter, n_alloced_params);
  	name = first_property_name;
  	while (name != NULL) {
		GParamSpec *pspec;
		char       *error = NULL;

		pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (object), name);
		if (pspec == NULL) {
			g_warning ("%s: object class `%s' has no property named `%s'",
				   G_STRFUNC,
				   g_type_name (object_type),
				   name);
			break;
		}
		type_spec->params[type_spec->n_params].name = name;
		type_spec->params[type_spec->n_params].value.g_type = 0;
		g_value_init (&type_spec->params[type_spec->n_params].value, G_PARAM_SPEC_VALUE_TYPE (pspec));
		G_VALUE_COLLECT (&type_spec->params[type_spec->n_params].value, var_args, 0, &error);
		if (error != NULL) {
			g_warning ("%s: %s", G_STRFUNC, error);
			g_free (error);
			g_value_unset (&type_spec->params[type_spec->n_params].value);
			break;
		}
		type_spec->n_params++;
		name = va_arg (var_args, char*);
	}

	g_object_unref (object);

	return type_spec;
}


void
gth_main_register_test (const char *object_id,
			GType       object_type,
			const char *first_property_name,
			...)
{
	va_list      var_args;
	GthTypeSpec *spec;

	va_start (var_args, first_property_name);
	spec = _gth_main_create_type_spec (object_type, first_property_name, var_args);
	va_end (var_args);

	g_hash_table_insert (Main->priv->tests, (gpointer) object_id, spec);
}


GthTest *
gth_main_get_test (const char *object_id)
{
	GthTypeSpec *spec = NULL;

	if (object_id == NULL)
		return NULL;

	spec = g_hash_table_lookup (Main->priv->tests, object_id);
	if (spec == NULL)
		return NULL;

	return (GthTest *) gth_type_spec_create_object (spec, object_id);
}


static void
collect_objects (gpointer key,
		 gpointer value,
		 gpointer user_data)
{
	GList       **objects = user_data;
	GthTypeSpec  *spec = value;

	*objects = g_list_prepend (*objects, gth_type_spec_create_object (spec, key));
}


G_GNUC_UNUSED
static GList *
_gth_main_get_all_objects (GHashTable *table)
{
	GList *objects = NULL;

	g_hash_table_foreach (table, collect_objects, &objects);
	return g_list_reverse (objects);
}


static void
collect_names (gpointer key,
	       gpointer value,
	       gpointer user_data)
{
	GList **objects = user_data;

	*objects = g_list_prepend (*objects, g_strdup (key));
}


static GList *
_gth_main_get_all_object_names (GHashTable *table)
{
	GList *objects = NULL;

	g_hash_table_foreach (table, collect_names, &objects);
	return g_list_reverse (objects);
}


GList *
gth_main_get_all_tests (void)
{
	return _gth_main_get_all_object_names (Main->priv->tests);
}


void
gth_main_register_file_loader (FileLoader  loader,
			       const char *first_mime_type,
			       ...)
{
	va_list     var_args;
	const char *mime_type;

	va_start (var_args, first_mime_type);
	mime_type = first_mime_type;
  	while (mime_type != NULL) {
		mime_type = va_arg (var_args, const char *);
		g_hash_table_insert (Main->priv->loaders, (gpointer) get_static_string (mime_type), loader);
	}
	va_end (var_args);
}


FileLoader
gth_main_get_file_loader (const char *mime_type)
{
	FileLoader loader;

	loader = g_hash_table_lookup (Main->priv->loaders, mime_type);
	if (loader == NULL)
		loader = gth_pixbuf_animation_new_from_file;

	return loader;
}


GthTest *
gth_main_get_general_filter (void)
{
	char    *filter_name;
	GthTest *filter;

	filter_name = eel_gconf_get_string (PREF_GENERAL_FILTER, DEFAULT_GENERAL_FILTER);
	filter =  gth_main_get_test (filter_name);
	g_free (filter_name);

	return filter;
}


GthTest *
gth_main_add_general_filter (GthTest *original_filter)
{
	GthTest *test;

	if (original_filter == NULL)
		test = gth_main_get_general_filter ();
	else
		test = (GthTest *) gth_duplicable_duplicate (GTH_DUPLICABLE (original_filter));

	if (! GTH_IS_FILTER (test)) {
		GthTest   *new_chain;
		GthFilter *filter;

		filter = gth_filter_new ();

		new_chain = gth_test_chain_new (GTH_MATCH_TYPE_ALL, NULL);
		gth_test_chain_add_test (GTH_TEST_CHAIN (new_chain), test);
		g_object_unref (test);

		if (strncmp (gth_test_get_id (test), "file::type::", 12) != 0) {
			GthTest *file_type_filter;

			file_type_filter = gth_main_get_general_filter ();
			gth_test_chain_add_test (GTH_TEST_CHAIN (new_chain), file_type_filter);
			g_object_unref (file_type_filter);
		}
		gth_filter_set_test (filter, GTH_TEST_CHAIN (new_chain));
		g_object_unref (new_chain);

		test = (GthTest*) filter;
	}
	else {
		GthFilter    *filter;
		GthTestChain *filter_test;

		filter = (GthFilter *) gth_duplicable_duplicate (GTH_DUPLICABLE (test));
		filter_test = gth_filter_get_test (filter);
		if ((filter_test == NULL) || ! gth_test_chain_has_type_test (filter_test)) {
			GthTest *new_filter_test;
			GthTest *general_filter;

			new_filter_test = gth_test_chain_new (GTH_MATCH_TYPE_ALL, NULL);
			if (filter_test != NULL)
				gth_test_chain_add_test (GTH_TEST_CHAIN (new_filter_test), GTH_TEST (filter_test));

			general_filter = gth_main_get_general_filter ();
			gth_test_chain_add_test (GTH_TEST_CHAIN (new_filter_test), general_filter);
			g_object_unref (general_filter);

			gth_filter_set_test (filter, GTH_TEST_CHAIN (new_filter_test));
			g_object_unref (new_filter_test);
		}

		if (filter_test != NULL)
			g_object_unref (filter_test);
		g_object_unref (test);

		test = (GthTest*) filter;
	}

	return test;
}


void
gth_main_register_viewer_page (GType viewer_page_type)
{
	GObject *viewer_page;

	viewer_page = g_object_new (viewer_page_type, NULL);
	Main->priv->viewer_pages = g_list_prepend (Main->priv->viewer_pages, viewer_page);
}


GList *
gth_main_get_all_viewer_pages (void)
{
	return Main->priv->viewer_pages;
}


static void
_g_destroy_array (GArray *array)
{
	g_array_free (array, TRUE);
}


void
gth_main_register_type (const char *set_name,
			GType       object_type)
{
	GArray *set;

	if (Main->priv->types == NULL)
		Main->priv->types = g_hash_table_new_full (g_str_hash,
							   g_str_equal,
							   (GDestroyNotify) g_free,
							   (GDestroyNotify) _g_destroy_array);

	set = g_hash_table_lookup (Main->priv->types, set_name);
	if (set == NULL) {
		set = g_array_new (FALSE, FALSE, sizeof (GType));
		g_hash_table_insert (Main->priv->types, g_strdup (set_name), set);
	}

	g_array_append_val (set, object_type);
}


GArray *
gth_main_get_type_set (const char *set_name)
{
	return g_hash_table_lookup (Main->priv->types, set_name);
}


static void
_g_destroy_object_array (GPtrArray *array)
{
	g_ptr_array_foreach (array, (GFunc) g_object_unref, NULL);
	g_ptr_array_free (array, TRUE);
}


void
gth_main_register_object (const char *set_name,
			  GType       object_type)
{
	GPtrArray *set;

	if (Main->priv->objects == NULL)
		Main->priv->objects = g_hash_table_new_full (g_str_hash,
							     g_str_equal,
							     (GDestroyNotify) g_free,
							     (GDestroyNotify) _g_destroy_object_array);

	set = g_hash_table_lookup (Main->priv->objects, set_name);
	if (set == NULL) {
		set = g_ptr_array_new ();
		g_hash_table_insert (Main->priv->objects, g_strdup (set_name), set);
	}

	g_ptr_array_add (set, g_object_new (object_type, NULL));
}


GPtrArray *
gth_main_get_object_set (const char *set_name)
{
	return g_hash_table_lookup (Main->priv->objects, set_name);
}


GBookmarkFile *
gth_main_get_default_bookmarks (void)
{
	char *path;

	if (Main->priv->bookmarks != NULL)
		return Main->priv->bookmarks;

	Main->priv->bookmarks = g_bookmark_file_new ();

	path = gth_user_dir_get_file (GTH_DIR_CONFIG, GTHUMB_DIR, BOOKMARKS_FILE, NULL);
	g_bookmark_file_load_from_file (Main->priv->bookmarks,
					path,
					NULL);
	g_free (path);

	return Main->priv->bookmarks;
}


void
gth_main_bookmarks_changed (void)
{
	char *filename;

	gth_user_dir_make_dir_for_file (GTH_DIR_CONFIG, GTHUMB_DIR, BOOKMARKS_FILE, NULL);

	filename = gth_user_dir_get_file (GTH_DIR_CONFIG, GTHUMB_DIR, BOOKMARKS_FILE, NULL);
	g_bookmark_file_to_file (Main->priv->bookmarks, filename, NULL);
	g_free (filename);

	gth_monitor_bookmarks_changed (gth_main_get_default_monitor ());
}


GthFilterFile *
gth_main_get_default_filter_file (void)
{
	char *path;

	if (Main->priv->filters != NULL)
		return Main->priv->filters;

	Main->priv->filters = gth_filter_file_new ();
	path = gth_user_dir_get_file (GTH_DIR_CONFIG, GTHUMB_DIR, FILTERS_FILE, NULL);
	gth_filter_file_load_from_file (Main->priv->filters, path, NULL);
	g_free (path);

	return Main->priv->filters;
}


GList *
gth_main_get_all_filters (void)
{
	GthFilterFile *filter_file;
	GList         *filters;
	GList         *registered_tests;
	GList         *scan;
	gboolean       changed = FALSE;

	filter_file = gth_main_get_default_filter_file ();
	filters = gth_filter_file_get_tests (filter_file);

	registered_tests = gth_main_get_all_tests ();
	for (scan = registered_tests; scan; scan = scan->next) {
		const char *registered_test_id = scan->data;
		gboolean    test_present = FALSE;
		GList      *scan2;

		for (scan2 = filters; ! test_present && scan2; scan2 = scan2->next) {
			GthTest *test = scan2->data;

			if (g_strcmp0 (gth_test_get_id (test), registered_test_id) == 0)
				test_present = TRUE;
		}

		if (! test_present) {
			GthTest *registered_test;

			registered_test = gth_main_get_test (registered_test_id);
			filters = g_list_append (filters, registered_test);
			gth_filter_file_add (filter_file, registered_test);
			changed = TRUE;
		}
	}
	_g_string_list_free (registered_tests);

	if (changed)
		gth_main_filters_changed ();

	return filters;
}


void
gth_main_filters_changed (void)
{
	char *filename;

	gth_user_dir_make_dir_for_file (GTH_DIR_CONFIG, GTHUMB_DIR, FILTERS_FILE, NULL);

	filename = gth_user_dir_get_file (GTH_DIR_CONFIG, GTHUMB_DIR, FILTERS_FILE, NULL);
	gth_filter_file_to_file (Main->priv->filters, filename, NULL);
	g_free (filename);

	gth_monitor_filters_changed (gth_main_get_default_monitor ());
}


GthMonitor *
gth_main_get_default_monitor (void)
{
	if (Main->priv->monitor != NULL)
		return Main->priv->monitor;

	Main->priv->monitor = gth_monitor_new ();

	return Main->priv->monitor;
}


GthExtensionManager *
gth_main_get_default_extension_manager (void)
{
	return Main->priv->extension_manager;
}


void
gth_main_activate_extensions (void)
{
	const char *default_extensions[] = { "catalogs", "comments", "exiv2", "file_manager", "file_tools", "file_viewer", "image_viewer", "search", NULL };
	int         i;

	if (Main->priv->extension_manager == NULL)
		Main->priv->extension_manager = gth_extension_manager_new ();

	/* FIXME: read the extensions list from a gconf key */

	for (i = 0; default_extensions[i] != NULL; i++)
		gth_extension_manager_activate (Main->priv->extension_manager, default_extensions[i]);
}
