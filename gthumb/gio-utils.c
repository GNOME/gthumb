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

#include <string.h>
#include <glib.h>
#include <gio/gio.h>
#include "gth-file-data.h"
#include "gth-hook.h"
#include "glib-utils.h"
#include "gio-utils.h"


#define N_FILES_PER_REQUEST 128


/* -- filter -- */


typedef enum {
	FILTER_DEFAULT = 0,
	FILTER_NODOTFILES = 1 << 1,
	FILTER_IGNORECASE = 1 << 2,
	FILTER_NOBACKUPFILES = 1 << 3
} FilterOptions;


typedef struct {
	char           *pattern;
	FilterOptions   options;
	GRegex        **regexps;
} Filter;


static Filter *
filter_new (const char    *pattern,
	    FilterOptions  options)
{
	Filter             *filter;
	GRegexCompileFlags  flags;

	filter = g_new0 (Filter, 1);

	if ((pattern != NULL) && (strcmp (pattern, "*") != 0))
		filter->pattern = g_strdup (pattern);

	filter->options = options;
	if (filter->options & FILTER_IGNORECASE)
		flags = G_REGEX_CASELESS;
	else
		flags = 0;
	filter->regexps = get_regexps_from_pattern (pattern, flags);

	return filter;
}


static void
filter_destroy (Filter *filter)
{
	if (filter == NULL)
		return;

	g_free (filter->pattern);
	if (filter->regexps != NULL)
		free_regexps (filter->regexps);
	g_free (filter);
}


static gboolean
filter_matches (Filter     *filter,
		const char *name)
{
	const char *file_name;
	char       *utf8_name;
	gboolean    matched;

	g_return_val_if_fail (name != NULL, FALSE);

	file_name = _g_uri_get_basename (name);

	if ((filter->options & FILTER_NODOTFILES)
	    && ((file_name[0] == '.') || (strstr (file_name, "/.") != NULL)))
		return FALSE;

	if ((filter->options & FILTER_NOBACKUPFILES)
	    && (file_name[strlen (file_name) - 1] == '~'))
		return FALSE;

	if (filter->pattern == NULL)
		return TRUE;

	utf8_name = g_filename_to_utf8 (file_name, -1, NULL, NULL, NULL);
	matched = string_matches_regexps (filter->regexps, utf8_name, 0);
	g_free (utf8_name);

	return matched;
}


static gboolean
filter_empty (Filter *filter)
{
	return ((filter->pattern == NULL) || (strcmp (filter->pattern, "*") == 0));
}


/* -- g_directory_foreach_child -- */


typedef struct {
	GFile     *file;
	GFileInfo *info;
} ChildData;


static ChildData *
child_data_new (GFile     *file,
		GFileInfo *info)
{
	ChildData *data;

	data = g_new0 (ChildData, 1);
	data->file = g_file_dup (file);
	data->info = g_file_info_dup (info);

	return data;
}


static void
child_data_free (ChildData *data)
{
	if (data == NULL)
		return;
	if (data->file != NULL)
		g_object_unref (data->file);
	if (data->info != NULL)
		g_object_unref (data->info);
	g_free (data);
}


static void
clear_child_data (ChildData **data)
{
	if (*data != NULL)
		child_data_free (*data);
	*data = NULL;
}


typedef struct {
	GFile                *base_directory;
	gboolean              recursive;
	gboolean              follow_links;
	StartDirCallback      start_dir_func;
	ForEachChildCallback  for_each_file_func;
	ForEachDoneCallback   done_func;
	gpointer              user_data;

	/* private */

	ChildData            *current;
	GHashTable           *already_visited;
	GList                *to_visit;
	const char           *attributes;
	GCancellable         *cancellable;
	GFileEnumerator      *enumerator;
	GError               *error;
	guint                 source_id;
} ForEachChildData;


static void
for_each_child_data_free (ForEachChildData *fec)
{
	if (fec == NULL)
		return;

	g_object_unref (fec->base_directory);
	if (fec->already_visited)
		g_hash_table_destroy (fec->already_visited);
	clear_child_data (&(fec->current));
	if (fec->to_visit != NULL) {
		g_list_foreach (fec->to_visit, (GFunc) child_data_free, NULL);
		g_list_free (fec->to_visit);
	}
	g_free (fec);
}


static gboolean
for_each_child_done_cb (gpointer user_data)
{
	ForEachChildData *fec = user_data;

	g_source_remove (fec->source_id);
	clear_child_data (&(fec->current));
	if (fec->done_func)
		fec->done_func (fec->error, fec->user_data);
	for_each_child_data_free (fec);

	return FALSE;
}


static void
for_each_child_done (ForEachChildData *fec)
{
	fec->source_id = g_idle_add (for_each_child_done_cb, fec);
}


static void for_each_child_start_current (ForEachChildData *fec);


static gboolean
for_each_child_start_cb (gpointer user_data)
{
	ForEachChildData *fec = user_data;

	g_source_remove (fec->source_id);
	for_each_child_start_current (fec);

	return FALSE;
}


static void
for_each_child_start (ForEachChildData *fec)
{
	fec->source_id = g_idle_add (for_each_child_start_cb, fec);
}


static void
for_each_child_set_current (ForEachChildData *fec,
			    ChildData        *data)
{
	clear_child_data (&(fec->current));
	fec->current = data;
}


static void
for_each_child_start_next_sub_directory (ForEachChildData *fec)
{
	ChildData *child = NULL;

	if (fec->to_visit != NULL) {
		GList *tmp;

		child = (ChildData *) fec->to_visit->data;
		tmp = fec->to_visit;
		fec->to_visit = g_list_remove_link (fec->to_visit, tmp);
		g_list_free (tmp);
	}

	if (child != NULL) {
		for_each_child_set_current (fec, child);
		for_each_child_start (fec);
	}
	else
		for_each_child_done (fec);
}


static void
for_each_child_close_enumerator (GObject      *source_object,
				 GAsyncResult *result,
		      		 gpointer      user_data)
{
	ForEachChildData *fec = user_data;
	GError           *error = NULL;

	if (! g_file_enumerator_close_finish (fec->enumerator,
					      result,
					      &error))
	{
		if (fec->error == NULL)
			fec->error = g_error_copy (error);
		else
			g_clear_error (&error);
	}

	if ((fec->error == NULL) && fec->recursive)
		for_each_child_start_next_sub_directory (fec);
	else
		for_each_child_done (fec);
}


static void
for_each_child_next_files_ready (GObject      *source_object,
				 GAsyncResult *result,
				 gpointer      user_data)
{
	ForEachChildData *fec = user_data;
	GList            *children, *scan;

	children = g_file_enumerator_next_files_finish (fec->enumerator,
							result,
							&(fec->error));

	if (children == NULL) {
		g_file_enumerator_close_async (fec->enumerator,
					       G_PRIORITY_DEFAULT,
					       fec->cancellable,
					       for_each_child_close_enumerator,
					       fec);
		return;
	}

	for (scan = children; scan; scan = scan->next) {
		GFileInfo *child_info = scan->data;
		GFile     *file;

		file = g_file_get_child (fec->current->file, g_file_info_get_name (child_info));

		if (g_file_info_get_file_type (child_info) == G_FILE_TYPE_DIRECTORY) {
			/* avoid to visit a directory more than ones */

			if (g_hash_table_lookup (fec->already_visited, file) == NULL) {
				g_hash_table_insert (fec->already_visited, g_file_dup (file), GINT_TO_POINTER (1));
				fec->to_visit = g_list_append (fec->to_visit, child_data_new (file, child_info));
			}
		}

		fec->for_each_file_func (file, child_info, fec->user_data);

		g_object_unref (file);
	}

	g_file_enumerator_next_files_async (fec->enumerator,
					    N_FILES_PER_REQUEST,
					    G_PRIORITY_DEFAULT,
					    fec->cancellable,
					    for_each_child_next_files_ready,
					    fec);
}

static void
for_each_child_ready (GObject      *source_object,
		      GAsyncResult *result,
		      gpointer      user_data)
{
	ForEachChildData *fec = user_data;

	fec->enumerator = g_file_enumerate_children_finish (G_FILE (source_object), result, &(fec->error));
	if (fec->enumerator == NULL) {
		for_each_child_done (fec);
		return;
	}

	g_file_enumerator_next_files_async (fec->enumerator,
					    N_FILES_PER_REQUEST,
					    G_PRIORITY_DEFAULT,
					    fec->cancellable,
					    for_each_child_next_files_ready,
					    fec);
}


static void
for_each_child_start_current (ForEachChildData *fec)
{
	if (fec->start_dir_func != NULL) {
		DirOp  op;

		op = fec->start_dir_func (fec->current->file, fec->current->info, &(fec->error), fec->user_data);
		switch (op) {
		case DIR_OP_SKIP:
			for_each_child_start_next_sub_directory (fec);
			return;
		case DIR_OP_STOP:
			for_each_child_done (fec);
			return;
		case DIR_OP_CONTINUE:
			break;
		}
	}

	g_file_enumerate_children_async (fec->current->file,
					 fec->attributes,
					 fec->follow_links ? G_FILE_QUERY_INFO_NONE : G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
					 G_PRIORITY_DEFAULT,
					 fec->cancellable,
					 for_each_child_ready,
					 fec);
}


static void
directory_info_ready_cb (GObject      *source_object,
			 GAsyncResult *result,
			 gpointer      user_data)
{
	ForEachChildData *fec = user_data;
	GFileInfo        *info;
	ChildData        *child;

	info = g_file_query_info_finish (G_FILE (source_object), result, &(fec->error));
	if (info == NULL) {
		for_each_child_done (fec);
		return;
	}

	child = child_data_new (fec->base_directory, info);
	g_object_unref (info);

	for_each_child_set_current (fec, child);
	for_each_child_start_current (fec);
}


/**
 * g_directory_foreach_child:
 * @directory: The directory to visit.
 * @recursive: Whether to traverse the @directory recursively.
 * @follow_links: Whether to dereference the symbolic links.
 * @attributes: The GFileInfo attributes to read.
 * @cancellable: An optional @GCancellable object, used to cancel the process.
 * @start_dir_func: the function called for each sub-directory, or %NULL if
 *   not needed.
 * @for_each_file_func: the function called for each file.  Can't be %NULL.
 * @done_func: the function called at the end of the traversing process.
 *   Can't be %NULL.
 * @user_data: data to pass to @done_func
 *
 * Traverse the @directory's filesystem structure calling the
 * @for_each_file_func function for each file in the directory; the
 * @start_dir_func function on each directory before it's going to be
 * traversed, this includes @directory too; the @done_func function is
 * called at the end of the process.
 * Some traversing options are available: if @recursive is TRUE the
 * directory is traversed recursively; if @follow_links is TRUE, symbolic
 * links are dereferenced, otherwise they are returned as links.
 * Each callback uses the same @user_data additional parameter.
 */
void
g_directory_foreach_child (GFile                *directory,
			   gboolean              recursive,
			   gboolean              follow_links,
			   const char           *attributes,
			   GCancellable         *cancellable,
			   StartDirCallback      start_dir_func,
			   ForEachChildCallback  for_each_file_func,
			   ForEachDoneCallback   done_func,
			   gpointer              user_data)
{
	ForEachChildData *fec;

	g_return_if_fail (for_each_file_func != NULL);

	fec = g_new0 (ForEachChildData, 1);

	fec->base_directory = g_file_dup (directory);
	fec->recursive = recursive;
	fec->follow_links = follow_links;
	fec->attributes = attributes;
	fec->cancellable = cancellable;
	fec->start_dir_func = start_dir_func;
	fec->for_each_file_func = for_each_file_func;
	fec->done_func = done_func;
	fec->user_data = user_data;
	fec->already_visited = g_hash_table_new_full (g_file_hash,
						      (GEqualFunc) g_file_equal,
						      g_object_unref,
						      NULL);

	g_file_query_info_async (fec->base_directory,
				 fec->attributes,
				 G_FILE_QUERY_INFO_NONE,
				 G_PRIORITY_DEFAULT,
				 cancellable,
				 directory_info_ready_cb,
				 fec);
}


/* -- get_file_list_data -- */


typedef struct {
	GList             *files;
	GList             *dirs;
	GFile             *directory;
	char              *base_dir;
	GCancellable      *cancellable;
	ListReadyCallback  done_func;
	gpointer           done_data;
	GList             *to_visit;
	GList             *current_dir;
	Filter            *include_filter;
	Filter            *exclude_filter;
	Filter            *exclude_folders_filter;
	guint              visit_timeout;
} GetFileListData;


static void
get_file_list_data_free (GetFileListData *gfl)
{
	if (gfl == NULL)
		return;

	filter_destroy (gfl->include_filter);
	filter_destroy (gfl->exclude_filter);
	filter_destroy (gfl->exclude_folders_filter);
	_g_string_list_free (gfl->files);
	_g_string_list_free (gfl->dirs);
	_g_string_list_free (gfl->to_visit);
	g_object_unref (gfl->directory);
	g_free (gfl->base_dir);
	g_free (gfl);
}


/* -- g_directory_list_async -- */


static GList*
get_relative_file_list (GList      *rel_list,
			GList      *file_list,
			const char *base_dir)
{
	GList *scan;
	int    base_len;

	if (base_dir == NULL)
		return NULL;

	base_len = 0;
	if (strcmp (base_dir, "/") != 0)
		base_len = strlen (base_dir);

	for (scan = file_list; scan; scan = scan->next) {
		char *uri = scan->data;
		if (_g_uri_parent_of_uri (base_dir, uri)) {
			char *rel_uri = g_strdup (uri + base_len + 1);
			rel_list = g_list_prepend (rel_list, rel_uri);
		}
	}

	return rel_list;
}


static GList*
get_dir_list_from_file_list (GHashTable *h_dirs,
			     const char *base_dir,
			     GList      *files,
			     gboolean    is_dir_list)
{
	GList *scan;
	GList *dir_list = NULL;
	int    base_dir_len;

	if (base_dir == NULL)
		base_dir = "";
	base_dir_len = strlen (base_dir);

	for (scan = files; scan; scan = scan->next) {
		char *filename = scan->data;
		char *dir_name;

		if (strlen (filename) <= base_dir_len)
			continue;

		if (is_dir_list)
			dir_name = g_strdup (filename + base_dir_len + 1);
		else
			dir_name = _g_uri_get_parent (filename + base_dir_len + 1);

		while ((dir_name != NULL) && (dir_name[0] != '\0') && (strcmp (dir_name, "/") != 0)) {
			char *tmp;
			char *dir;

			/* avoid to insert duplicated folders */

			dir = g_strconcat (base_dir, "/", dir_name, NULL);
			if (g_hash_table_lookup (h_dirs, dir) == NULL) {
				g_hash_table_insert (h_dirs, dir, GINT_TO_POINTER (1));
				dir_list = g_list_prepend (dir_list, dir);
			}
			else
				g_free (dir);

			tmp = dir_name;
			dir_name = _g_uri_get_parent (tmp);
			g_free (tmp);
		}

		g_free (dir_name);
	}

	return dir_list;
}


static void
get_file_list_done (GError   *error,
		    gpointer  user_data)
{
	GetFileListData *gfl = user_data;
	GHashTable      *h_dirs;
	GList           *scan;

	gfl->files = g_list_reverse (gfl->files);
	gfl->dirs = g_list_reverse (gfl->dirs);

	if (! filter_empty (gfl->include_filter) || (gfl->exclude_filter->pattern != NULL)) {
		_g_string_list_free (gfl->dirs);
		gfl->dirs = NULL;
	}

	h_dirs = g_hash_table_new (g_str_hash, g_str_equal);

	/* Always include the base directory, this way empty base
 	 * directories are added to the archive as well.  */

	if (gfl->base_dir != NULL) {
		char *dir;

		dir = g_strdup (gfl->base_dir);
		gfl->dirs = g_list_prepend (gfl->dirs, dir);
		g_hash_table_insert (h_dirs, dir, GINT_TO_POINTER (1));
	}

	/* Add all the parent directories in gfl->files/gfl->dirs to the
	 * gfl->dirs list, the hash table is used to avoid duplicated
	 * entries. */

	for (scan = gfl->dirs; scan; scan = scan->next)
		g_hash_table_insert (h_dirs, (char*)scan->data, GINT_TO_POINTER (1));

	gfl->dirs = g_list_concat (gfl->dirs, get_dir_list_from_file_list (h_dirs, gfl->base_dir, gfl->files, FALSE));

	if (filter_empty (gfl->include_filter))
		gfl->dirs = g_list_concat (gfl->dirs, get_dir_list_from_file_list (h_dirs, gfl->base_dir, gfl->dirs, TRUE));

	/**/

	if (error == NULL) {
		GList *rel_files, *rel_dirs;

		if (gfl->base_dir != NULL) {
			rel_files = get_relative_file_list (NULL, gfl->files, gfl->base_dir);
			rel_dirs = get_relative_file_list (NULL, gfl->dirs, gfl->base_dir);
		}
		else {
			rel_files = gfl->files;
			rel_dirs = gfl->dirs;
			gfl->files = NULL;
			gfl->dirs = NULL;
		}

		/* rel_files/rel_dirs must be deallocated in done_func */
		gfl->done_func (rel_files, rel_dirs, NULL, gfl->done_data);
	}
	else
		gfl->done_func (NULL, NULL, error, gfl->done_data);

	g_hash_table_destroy (h_dirs);
	get_file_list_data_free (gfl);
}


static void
get_file_list_for_each_file (GFile     *file,
			     GFileInfo *info,
			     gpointer   user_data)
{
	GetFileListData *gfl = user_data;
	char            *uri;

	uri = g_file_get_uri (file);

	switch (g_file_info_get_file_type (info)) {
	case G_FILE_TYPE_REGULAR:
		if (filter_matches (gfl->include_filter, uri))
			if ((gfl->exclude_filter->pattern == NULL) || ! filter_matches (gfl->exclude_filter, uri))
				gfl->files = g_list_prepend (gfl->files, g_strdup (uri));
		break;
	default:
		break;
	}

	g_free (uri);
}


static DirOp
get_file_list_start_dir (GFile       *directory,
			 GFileInfo   *info,
			 GError     **error,
			 gpointer     user_data)
{
	DirOp            dir_op = DIR_OP_CONTINUE;
	GetFileListData *gfl = user_data;
	char            *uri;

	uri = g_file_get_uri (directory);
	if ((gfl->exclude_folders_filter->pattern == NULL) || ! filter_matches (gfl->exclude_folders_filter, uri)) {
		gfl->dirs = g_list_prepend (gfl->dirs, g_strdup (uri));
		dir_op = DIR_OP_CONTINUE;
	}
	else
		dir_op = DIR_OP_SKIP;

	g_free (uri);

	return dir_op;
}


void
g_directory_list_async (GFile             *directory,
			const char        *base_dir,
			gboolean           recursive,
			gboolean           follow_links,
			gboolean           no_backup_files,
			gboolean           no_dot_files,
			const char        *include_files,
			const char        *exclude_files,
			const char        *exclude_folders,
			gboolean           ignorecase,
			GCancellable      *cancellable,
			ListReadyCallback  done_func,
			gpointer           done_data)
{
	GetFileListData *gfl;
	FilterOptions    filter_options;

	gfl = g_new0 (GetFileListData, 1);
	gfl->directory = g_file_dup (directory);
	gfl->base_dir = g_strdup (base_dir);
	gfl->done_func = done_func;
	gfl->done_data = done_data;

	filter_options = FILTER_DEFAULT;
	if (no_backup_files)
		filter_options |= FILTER_NOBACKUPFILES;
	if (no_dot_files)
		filter_options |= FILTER_NODOTFILES;
	if (ignorecase)
		filter_options |= FILTER_IGNORECASE;
	gfl->include_filter = filter_new (include_files, filter_options);
	gfl->exclude_filter = filter_new (exclude_files, ignorecase ? FILTER_IGNORECASE : FILTER_DEFAULT);
	gfl->exclude_folders_filter = filter_new (exclude_folders, ignorecase ? FILTER_IGNORECASE : FILTER_DEFAULT);

	g_directory_foreach_child (directory,
				   recursive,
				   follow_links,
				   "standard::name,standard::type",
				   cancellable,
				   get_file_list_start_dir,
				   get_file_list_for_each_file,
				   get_file_list_done,
				   gfl);
}


/* -- g_query_info_async -- */


typedef struct {
	GList             *files;
	GList             *current;
	const char        *attributes;
	GCancellable      *cancellable;
	InfoReadyCallback  ready_func;
	gpointer           user_data;
	GList             *file_data;
} QueryInfoData;


static void
query_data_free (QueryInfoData *query_data)
{
	_g_object_list_unref (query_data->files);
	g_free (query_data);
}


static void
query_info_ready_cb (GObject      *source_object,
		     GAsyncResult *result,
		     gpointer      user_data)
{
	QueryInfoData *query_data = user_data;
	GFile         *file;
	GFileInfo     *info;
	GError        *error = NULL;

	file = (GFile*) source_object;
	info = g_file_query_info_finish (file, result, &error);
	if (info == NULL) {
		query_data->ready_func (NULL, error, query_data->user_data);
		query_data_free (query_data);
		return;
	}

	query_data->file_data = g_list_prepend (query_data->file_data, gth_file_data_new (file, info));
	g_object_unref (info);

	query_data->current = query_data->current->next;
	if (query_data->current == NULL) {
		query_data->file_data = g_list_reverse (query_data->file_data);
		query_data->ready_func (query_data->file_data, NULL, query_data->user_data);
		query_data_free (query_data);
		return;
	}

	g_file_query_info_async ((GFile*) query_data->current->data,
				 query_data->attributes,
				 0,
				 G_PRIORITY_DEFAULT,
				 query_data->cancellable,
				 query_info_ready_cb,
				 query_data);
}


void
g_query_info_async (GList             *files,
		    const char        *attributes,
		    GCancellable      *cancellable,
		    InfoReadyCallback  ready_func,
		    gpointer           user_data)
{
	QueryInfoData *query_data;

	query_data = g_new0 (QueryInfoData, 1);
	query_data->files = _g_object_list_ref (files);
	query_data->attributes = attributes;
	query_data->cancellable = cancellable;
	query_data->ready_func = ready_func;
	query_data->user_data = user_data;

	query_data->current = query_data->files;

	g_file_query_info_async ((GFile*) query_data->current->data,
				 query_data->attributes,
				 0,
				 G_PRIORITY_DEFAULT,
				 query_data->cancellable,
				 query_info_ready_cb,
				 query_data);
}


/* -- g_copy_files_async -- */


typedef struct {
	GList                 *sources;
	GList                 *destinations;
	GFileCopyFlags         flags;
	int                    io_priority;
	GCancellable          *cancellable;
	CopyProgressCallback   progress_callback;
	gpointer               progress_callback_data;
	CopyDoneCallback       callback;
	gpointer               user_data;

	GList                 *source;
	GList                 *destination;
	int                    n_file;
	int                    tot_files;

	guint                  dummy_event;
} CopyFilesData;


static CopyFilesData*
copy_files_data_new (GList                 *sources,
		     GList                 *destinations,
		     GFileCopyFlags         flags,
		     int                    io_priority,
		     GCancellable          *cancellable,
		     CopyProgressCallback   progress_callback,
		     gpointer               progress_callback_data,
		     CopyDoneCallback       callback,
		     gpointer               user_data)
{
	CopyFilesData *cfd;

	cfd = g_new0 (CopyFilesData, 1);
	cfd->sources = _g_file_list_dup (sources);
	cfd->destinations = _g_file_list_dup (destinations);
	cfd->flags = flags;
	cfd->io_priority = io_priority;
	cfd->cancellable = cancellable;
	cfd->progress_callback = progress_callback;
	cfd->progress_callback_data = progress_callback_data;
	cfd->callback = callback;
	cfd->user_data = user_data;

	cfd->source = cfd->sources;
	cfd->destination = cfd->destinations;
	cfd->n_file = 1;
	cfd->tot_files = g_list_length (cfd->sources);

	return cfd;
}


static void
copy_files_data_free (CopyFilesData *cfd)
{
	if (cfd == NULL)
		return;
	_g_file_list_free (cfd->sources);
	_g_file_list_free (cfd->destinations);
	g_free (cfd);
}


gboolean
_g_dummy_file_op_completed (gpointer data)
{
	CopyFilesData *cfd = data;

	if (cfd->dummy_event != 0) {
		g_source_remove (cfd->dummy_event);
		cfd->dummy_event = 0;
	}

	if (cfd->callback)
		cfd->callback (NULL, cfd->user_data);

	copy_files_data_free (cfd);

	return FALSE;
}


void
_g_dummy_file_op_async (CopyDoneCallback callback,
			gpointer         user_data)
{
	CopyFilesData *cfd;

	cfd = copy_files_data_new (NULL,
				   NULL,
				   0,
				   0,
				   NULL,
				   NULL,
				   NULL,
				   callback,
				   user_data);
	cfd->dummy_event = g_idle_add (_g_dummy_file_op_completed, cfd);
}


static void g_copy_current_file (CopyFilesData *cfd);


static void
g_copy_next_file (CopyFilesData *cfd)
{
	cfd->source = g_list_next (cfd->source);
	cfd->destination = g_list_next (cfd->destination);
	cfd->n_file++;

	g_copy_current_file (cfd);
}


static void
g_copy_files_ready_cb (GObject      *source_object,
		       GAsyncResult *result,
		       gpointer      user_data)
{
	CopyFilesData *cfd = user_data;
	GFile         *source = cfd->source->data;
	GError        *error = NULL;

	if (! g_file_copy_finish (source, result, &error)) {
		if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND)) {
			g_clear_error (&error);
			g_copy_next_file (cfd);
			return;
		}

		if (cfd->callback)
			cfd->callback (error, cfd->user_data);
		copy_files_data_free (cfd);
		return;
	}

	g_copy_next_file (cfd);
}


static void
g_copy_files_progess_cb (goffset  current_num_bytes,
			 goffset  total_num_bytes,
			 gpointer user_data)
{
	CopyFilesData *cfd = user_data;

	if (cfd->progress_callback)
		cfd->progress_callback (cfd->n_file,
					cfd->tot_files,
					(GFile*) cfd->source->data,
					(GFile*) cfd->destination->data,
					current_num_bytes,
					total_num_bytes,
					cfd->progress_callback_data);
}


static void
g_copy_current_file (CopyFilesData *cfd)
{
	if ((cfd->source == NULL) || (cfd->destination == NULL)) {
		if (cfd->callback)
			cfd->callback (NULL, cfd->user_data);
		copy_files_data_free (cfd);
		return;
	}

	g_file_copy_async ((GFile*) cfd->source->data,
			   (GFile*) cfd->destination->data,
			   cfd->flags,
			   cfd->io_priority,
			   cfd->cancellable,
			   g_copy_files_progess_cb,
			   cfd,
			   g_copy_files_ready_cb,
			   cfd);
}


void
g_copy_files_async (GList                 *sources,
		    GList                 *destinations,
		    GFileCopyFlags         flags,
		    int                    io_priority,
		    GCancellable          *cancellable,
		    CopyProgressCallback   progress_callback,
		    gpointer               progress_callback_data,
		    CopyDoneCallback       callback,
		    gpointer               user_data)
{
	CopyFilesData *cfd;

	cfd = copy_files_data_new (sources,
				   destinations,
				   flags,
				   io_priority,
				   cancellable,
				   progress_callback,
				   progress_callback_data,
				   callback,
				   user_data);

	/* add the metadata sidecars if requested */

	if (flags && G_FILE_COPY_ALL_METADATA) {
		GList *source_sidecars = NULL;
		GList *destination_sidecars = NULL;


		gth_hook_invoke ("add-sidecars", sources, &source_sidecars);
		gth_hook_invoke ("add-sidecars", destinations, &destination_sidecars);

		source_sidecars = g_list_reverse (source_sidecars);
		destination_sidecars = g_list_reverse (destination_sidecars);

		cfd->sources = g_list_concat (cfd->sources, source_sidecars);
		cfd->destinations = g_list_concat (cfd->destinations, destination_sidecars);
	}

	/* create the destination folders */

	{
		GHashTable *folders;
		GList      *scan;
		GList      *folder_list;

		folders = g_hash_table_new_full (g_file_hash, (GEqualFunc) g_file_equal, (GDestroyNotify) g_object_unref, NULL);
		for (scan = cfd->destinations; scan; scan = scan->next) {
			GFile *file = scan->data;
			GFile *folder;

			folder = g_file_get_parent (file);
			if (folder == NULL)
				continue;

			if (! g_hash_table_lookup (folders, folder))
				g_hash_table_insert (folders, g_object_ref (folder), GINT_TO_POINTER (1));

			g_object_unref (folder);
		}

		folder_list = g_hash_table_get_keys (folders);
		for (scan = folder_list; scan; scan = scan->next) {
			GFile *folder = scan->data;

			g_file_make_directory_with_parents (folder, NULL, NULL);
		}

		g_list_free (folder_list);
		g_hash_table_destroy (folders);
	}

	/* copy a file at a time */

	g_copy_current_file (cfd);
}


void
_g_copy_file_async (GFile                 *source,
		    GFile                 *destination,
		    GFileCopyFlags         flags,
		    int                    io_priority,
		    GCancellable          *cancellable,
		    CopyProgressCallback   progress_callback,
		    gpointer               progress_callback_data,
		    CopyDoneCallback       callback,
		    gpointer               user_data)
{
	GList *source_files;
	GList *destination_files;

	source_files = g_list_append (NULL, (gpointer) source);
	destination_files = g_list_append (NULL, (gpointer) destination);

	g_copy_files_async (source_files,
			    destination_files,
			    flags,
			    io_priority,
			    cancellable,
			    progress_callback,
			    progress_callback_data,
			    callback,
			    user_data);

	g_list_free (source_files);
	g_list_free (destination_files);
}


/* -- g_directory_copy_async -- */


typedef struct {
	GFile                 *source;
	GFile                 *destination;
	GFileCopyFlags         flags;
	int                    io_priority;
	GCancellable          *cancellable;
	CopyProgressCallback   progress_callback;
	gpointer               progress_callback_data;
	CopyDoneCallback       callback;
	gpointer               user_data;
	GError                *error;

	GList                 *to_copy;
	GList                 *current;
	GFile                 *current_source;
	GFile                 *current_destination;
	int                    n_file, tot_files;
	guint                  source_id;
} DirectoryCopyData;


static void
directory_copy_data_free (DirectoryCopyData *dcd)
{
	if (dcd == NULL)
		return;

	g_object_unref (dcd->source);
	g_object_unref (dcd->destination);
	if (dcd->current_source != NULL) {
		g_object_unref (dcd->current_source);
		dcd->current_source = NULL;
	}
	if (dcd->current_destination != NULL) {
		g_object_unref (dcd->current_destination);
		dcd->current_destination = NULL;
	}
	g_list_foreach (dcd->to_copy, (GFunc) child_data_free, NULL);
	g_list_free (dcd->to_copy);
	g_object_unref (dcd->cancellable);
	g_free (dcd);
}


static gboolean
g_directory_copy_done (gpointer user_data)
{
	DirectoryCopyData *dcd = user_data;

	if (dcd->source_id != 0)
		g_source_remove (dcd->source_id);

	if (dcd->callback)
		dcd->callback (dcd->error, dcd->user_data);
	directory_copy_data_free (dcd);

	return FALSE;
}


static GFile *
get_destination_for_file (DirectoryCopyData *dcd,
			  GFile             *file)
{
	char  *uri;
	char  *source_uri;
	char  *partial_uri;
	char  *path;
	GFile *destination_file;

	if (! g_file_has_prefix (file, dcd->source))
		return NULL;

	uri = g_file_get_uri (file);
	source_uri = g_file_get_uri (dcd->source);
	partial_uri = uri + strlen (source_uri) + 1;
	path = g_uri_unescape_string (partial_uri, "");
	destination_file = _g_file_append_path (dcd->destination, path);

	g_free (path);
	g_free (source_uri);
	g_free (uri);

	return destination_file;
}


static void g_directory_copy_current_child (DirectoryCopyData *dcd);


static gboolean
g_directory_copy_next_child (gpointer user_data)
{
	DirectoryCopyData *dcd = user_data;

	g_source_remove (dcd->source_id);

	dcd->current = g_list_next (dcd->current);
	dcd->n_file++;
	g_directory_copy_current_child (dcd);

	return FALSE;
}


static void
g_directory_copy_child_done_cb (GObject      *source_object,
				GAsyncResult *result,
				 gpointer     user_data)
{
	DirectoryCopyData *dcd = user_data;

	if (! g_file_copy_finish ((GFile*)source_object, result, &(dcd->error))) {
		dcd->source_id = g_idle_add (g_directory_copy_done, dcd);
		return;
	}

	dcd->source_id = g_idle_add (g_directory_copy_next_child, dcd);
}


static void
g_directory_copy_child_progess_cb (goffset  current_num_bytes,
				      goffset  total_num_bytes,
				      gpointer user_data)
{
	DirectoryCopyData *dcd = user_data;

	if (dcd->progress_callback)
		dcd->progress_callback (dcd->n_file,
					dcd->tot_files,
					dcd->current_source,
					dcd->current_destination,
					current_num_bytes,
					total_num_bytes,
					dcd->progress_callback_data);
}


static void
g_directory_copy_current_child (DirectoryCopyData *dcd)
{
	ChildData *child;
	gboolean   async_op = FALSE;

	if (dcd->current == NULL) {
		dcd->source_id = g_idle_add (g_directory_copy_done, dcd);
		return;
	}

	if (dcd->current_source != NULL) {
		g_object_unref (dcd->current_source);
		dcd->current_source = NULL;
	}
	if (dcd->current_destination != NULL) {
		g_object_unref (dcd->current_destination);
		dcd->current_destination = NULL;
	}

	child = dcd->current->data;
	dcd->current_source = g_file_dup (child->file);
	dcd->current_destination = get_destination_for_file (dcd, child->file);
	if (dcd->current_destination == NULL) {
		dcd->source_id = g_idle_add (g_directory_copy_next_child, dcd);
		return;
	}

	switch (g_file_info_get_file_type (child->info)) {
	case G_FILE_TYPE_DIRECTORY:
		/* FIXME: how to make a directory asynchronously ? */

		/* doesn't check the returned error for now, because when an
		 * error occurs the code is not returned (for example when
		 * a directory already exists the G_IO_ERROR_EXISTS code is
		 * *not* returned), so we cannot discriminate between warnings
		 * and fatal errors. (see bug #525155) */

		g_file_make_directory (dcd->current_destination,
				       NULL,
				       NULL);

		/*if (! g_file_make_directory (dcd->current_destination,
					     dcd->cancellable,
					     &(dcd->error)))
		{
			dcd->source_id = g_idle_add (g_directory_copy_done, dcd);
			return;
		}*/
		break;
	case G_FILE_TYPE_SYMBOLIC_LINK:
		/* FIXME: how to make a link asynchronously ? */

		g_file_make_symbolic_link (dcd->current_destination,
					   g_file_info_get_symlink_target (child->info),
					   NULL,
					   NULL);

		/*if (! g_file_make_symbolic_link (dcd->current_destination,
						 g_file_info_get_symlink_target (child->info),
						 dcd->cancellable,
						 &(dcd->error)))
		{
			dcd->source_id = g_idle_add (g_directory_copy_done, dcd);
			return;
		}*/
		break;
	case G_FILE_TYPE_REGULAR:
		g_file_copy_async (dcd->current_source,
				   dcd->current_destination,
				   dcd->flags,
				   dcd->io_priority,
				   dcd->cancellable,
				   g_directory_copy_child_progess_cb,
				   dcd,
				   g_directory_copy_child_done_cb,
				   dcd);
		async_op = TRUE;
		break;
	default:
		break;
	}

	if (! async_op)
		dcd->source_id = g_idle_add (g_directory_copy_next_child, dcd);
}


static gboolean
g_directory_copy_start_copying (gpointer user_data)
{
	DirectoryCopyData *dcd = user_data;

	g_source_remove (dcd->source_id);

	dcd->to_copy = g_list_reverse (dcd->to_copy);
	dcd->current = dcd->to_copy;
	dcd->n_file = 1;
	g_directory_copy_current_child (dcd);

	return FALSE;
}


static void
g_directory_copy_list_ready (GError   *error,
			     gpointer  user_data)
{
	DirectoryCopyData *dcd = user_data;

	if (error != NULL) {
		dcd->error = g_error_copy (error);
		dcd->source_id = g_idle_add (g_directory_copy_done, dcd);
		return;
	}

	dcd->source_id = g_idle_add (g_directory_copy_start_copying, dcd);
}


static void
g_directory_copy_for_each_file (GFile     *file,
				GFileInfo *info,
				gpointer   user_data)
{
	DirectoryCopyData *dcd = user_data;

	dcd->to_copy = g_list_prepend (dcd->to_copy, child_data_new (file, info));
	dcd->tot_files++;
}


static DirOp
g_directory_copy_start_dir (GFile      *directory,
			    GFileInfo  *info,
			    GError    **error,
			    gpointer    user_data)
{
	DirectoryCopyData *dcd = user_data;

	dcd->to_copy = g_list_prepend (dcd->to_copy, child_data_new (directory, info));
	dcd->tot_files++;

	return DIR_OP_CONTINUE;
}


static void
g_directory_copy_destination_info_ready_cb (GObject      *source_object,
                                            GAsyncResult *result,
                                            gpointer      user_data)
{
	DirectoryCopyData *dcd = user_data;
	GFileInfo         *info;

	info = g_file_query_info_finish (G_FILE (source_object), result, &dcd->error);
	if (info == NULL) {
		if (! g_error_matches (dcd->error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND)) {
			g_directory_copy_done (dcd);
			return;
		}
	}

	g_clear_error (&dcd->error);

	if ((info != NULL) && (g_file_info_get_file_type (info) != G_FILE_TYPE_DIRECTORY)) {
		dcd->error = g_error_new_literal (G_IO_ERROR, G_IO_ERROR_NOT_DIRECTORY, "");
		g_directory_copy_done (dcd);
		return;
	}

	if (! g_file_make_directory (dcd->destination,
			             dcd->cancellable,
			             &dcd->error))
	{
		g_directory_copy_done (dcd);
		return;
	}

	g_clear_error (&dcd->error);

	g_directory_foreach_child (dcd->source,
				   TRUE,
				   TRUE,
				   "standard::name,standard::type",
				   dcd->cancellable,
				   g_directory_copy_start_dir,
				   g_directory_copy_for_each_file,
				   g_directory_copy_list_ready,
				   dcd);
}


void
g_directory_copy_async (GFile                 *source,
			GFile                 *destination,
			GFileCopyFlags         flags,
			int                    io_priority,
			GCancellable          *cancellable,
			CopyProgressCallback   progress_callback,
			gpointer               progress_callback_data,
			CopyDoneCallback       callback,
			gpointer               user_data)
{
	DirectoryCopyData *dcd;

	dcd = g_new0 (DirectoryCopyData, 1);
	dcd->source = g_file_dup (source);
	dcd->destination = g_file_dup (destination);
	dcd->flags = flags;
	dcd->io_priority = io_priority;
	dcd->cancellable = g_object_ref (cancellable);
	dcd->progress_callback = progress_callback;
	dcd->progress_callback_data = progress_callback_data;
	dcd->callback = callback;
	dcd->user_data = user_data;

	g_file_query_info_async (dcd->destination,
				 "standard::name,standard::type",
				 0,
				 G_PRIORITY_DEFAULT,
				 dcd->cancellable,
				 g_directory_copy_destination_info_ready_cb,
				 dcd);
}


gboolean
_g_delete_files (GList     *file_list,
		 gboolean   include_metadata,
		 GError   **error)
{
	GList *scan;

	for (scan = file_list; scan; scan = scan->next) {
		GFile *file = scan->data;

		if (! g_file_delete (file, NULL, error))
			return FALSE;
	}

	if (include_metadata) {
		GList *sidecars = NULL;

		gth_hook_invoke ("add-sidecars", file_list, &sidecars);
		sidecars = g_list_reverse (sidecars);
		for (scan = sidecars; scan; scan = scan->next) {
			GFile *file = scan->data;

			g_file_delete (file, NULL, NULL);
		}

		_g_object_list_unref (sidecars);
	}

	return TRUE;
}


#define BUFFER_SIZE 4096


gboolean
g_load_file_in_buffer (GFile   *file,
		       void   **buffer,
		       gsize   *size,
		       GError **error)
{
	GFileInputStream *istream;
	gboolean          retval;
	void             *local_buffer;
	gsize             count;
	gssize            n;
	char              tmp_buffer[BUFFER_SIZE];

	istream = g_file_read (file, NULL, error);
	if (istream == NULL)
		return FALSE;

	retval = FALSE;
	local_buffer = NULL;
	count = 0;
	for (;;) {
		n = g_input_stream_read (G_INPUT_STREAM (istream), tmp_buffer, BUFFER_SIZE, NULL, error);
		if (n < 0) {
			g_free (local_buffer);
			retval = FALSE;
			break;
		}
		else if (n == 0) {
			*buffer = local_buffer;
			*size = count;
			retval = TRUE;
			break;
		}

		local_buffer = g_realloc (local_buffer, count + n + 1);
		memcpy (local_buffer + count, tmp_buffer, n);
		count += n;
	}

	g_object_unref (istream);

	return retval;
}


typedef struct {
	int                  io_priority;
	GCancellable        *cancellable;
	BufferReadyCallback  callback;
	gpointer             user_data;
	GInputStream        *stream;
	char                 tmp_buffer[BUFFER_SIZE];
	void                *buffer;
	gsize                count;
} LoadData;


static void
load_data_free (LoadData *load_data)
{
	if (load_data->stream != NULL)
		g_object_unref (load_data->stream);
	g_free (load_data->buffer);
	g_free (load_data);
}


static void
load_file__stream_read_cb (GObject      *source_object,
			   GAsyncResult *result,
			   gpointer      user_data)
{
	LoadData *load_data = user_data;
	GError   *error = NULL;
	gssize    count;

	count = g_input_stream_read_finish (load_data->stream, result, &error);
	if (count < 0) {
		load_data->callback (NULL, -1, error, load_data->user_data);
		load_data_free (load_data);
		return;
	}
	else if (count == 0) {
		if (load_data->buffer != NULL)
			((char *)load_data->buffer)[load_data->count] = 0;
		load_data->callback (load_data->buffer, load_data->count, NULL, load_data->user_data);
		load_data_free (load_data);
		return;
	}

	load_data->buffer = g_realloc (load_data->buffer, load_data->count + count + 1);
	memcpy (load_data->buffer + load_data->count, load_data->tmp_buffer, count);
	load_data->count += count;

	g_input_stream_read_async (load_data->stream,
				   load_data->tmp_buffer,
				   BUFFER_SIZE,
				   load_data->io_priority,
				   load_data->cancellable,
				   load_file__stream_read_cb,
				   load_data);
}


static void
load_file__file_read_cb (GObject      *source_object,
			 GAsyncResult *result,
			 gpointer      user_data)
{
	LoadData *load_data = user_data;
	GError   *error = NULL;

	load_data->stream = (GInputStream *) g_file_read_finish (G_FILE (source_object), result, &error);
	if (load_data->stream == NULL) {
		load_data->callback (NULL, -1, error, load_data->user_data);
		load_data_free (load_data);
		return;
	}

	load_data->count = 0;
	g_input_stream_read_async (load_data->stream,
				   load_data->tmp_buffer,
				   BUFFER_SIZE,
				   load_data->io_priority,
				   load_data->cancellable,
				   load_file__stream_read_cb,
				   load_data);
}


void
g_load_file_async (GFile               *file,
		   int                  io_priority,
		   GCancellable        *cancellable,
		   BufferReadyCallback  callback,
		   gpointer             user_data)
{
	LoadData *load_data;

	load_data = g_new0 (LoadData, 1);
	load_data->io_priority = io_priority;
	load_data->cancellable = cancellable;
	load_data->callback = callback;
	load_data->user_data = user_data;

	g_file_read_async (file, io_priority, cancellable, load_file__file_read_cb, load_data);
}


/* -- g_write_file_async -- */


typedef struct {
	int                  io_priority;
	GCancellable        *cancellable;
	BufferReadyCallback  callback;
	gpointer             user_data;
	void                *buffer;
	gsize                count;
	gsize                written;
	GError              *error;
} WriteData;


static void
write_data_free (WriteData *write_data)
{
	g_free (write_data);
}


static void
write_file__notify (gpointer user_data)
{
	WriteData *write_data = user_data;

	write_data->callback (write_data->buffer, write_data->count, write_data->error, write_data->user_data);
	write_data_free (write_data);
}


static void
write_file__stream_flush_cb (GObject      *source_object,
			     GAsyncResult *result,
			     gpointer      user_data)
{
	GOutputStream *stream = (GOutputStream *) source_object;
	WriteData     *write_data = user_data;
	GError        *error = NULL;

	g_output_stream_flush_finish (stream, result, &error);
	write_data->error = error;
	g_object_unref (stream);

	call_when_idle (write_file__notify, write_data);
}


static void
write_file__stream_write_ready_cb (GObject      *source_object,
				   GAsyncResult *result,
				   gpointer      user_data)
{
	GOutputStream *stream = (GOutputStream *) source_object;
	WriteData     *write_data = user_data;
	GError        *error = NULL;
	gssize         count;

	count = g_output_stream_write_finish (stream, result, &error);
	write_data->written += count;

	if ((count == 0) || (write_data->written == write_data->count)) {
		g_output_stream_flush_async (stream,
					     write_data->io_priority,
					     write_data->cancellable,
					     write_file__stream_flush_cb,
					     user_data);
		return;
	}

	g_output_stream_write_async (stream,
				     write_data->buffer + write_data->written,
				     write_data->count - write_data->written,
				     write_data->io_priority,
				     write_data->cancellable,
				     write_file__stream_write_ready_cb,
				     write_data);
}


static void
write_file__replace_ready_cb (GObject      *source_object,
			      GAsyncResult *result,
			      gpointer      user_data)
{
	WriteData     *write_data = user_data;
	GOutputStream *stream;
	GError        *error = NULL;

	stream = (GOutputStream*) g_file_replace_finish ((GFile*) source_object, result, &error);
	if (stream == NULL) {
		write_data->callback (write_data->buffer, write_data->count, error, write_data->user_data);
		write_data_free (write_data);
		return;
	}

	write_data->written = 0;
	g_output_stream_write_async (stream,
				     write_data->buffer,
				     write_data->count,
				     write_data->io_priority,
				     write_data->cancellable,
				     write_file__stream_write_ready_cb,
				     write_data);
}


void
g_write_file_async (GFile               *file,
		    void                *buffer,
		    gsize                count,
		    int                  io_priority,
		    GCancellable        *cancellable,
		    BufferReadyCallback  callback,
		    gpointer             user_data)
{
	WriteData *write_data;

	write_data = g_new0 (WriteData, 1);
	write_data->buffer = buffer;
	write_data->count = count;
	write_data->io_priority = io_priority;
	write_data->cancellable = cancellable;
	write_data->callback = callback;
	write_data->user_data = user_data;

	g_file_replace_async (file, NULL, FALSE, 0, io_priority, cancellable, write_file__replace_ready_cb, write_data);
}


GFile *
_g_file_create_unique (GFile       *parent,
		       const char  *display_name,
		       const char  *suffix,
		       GError     **error)
{
	GFile             *file = NULL;
	GError            *local_error = NULL;
	int                n;
	GFileOutputStream *stream;

	file = g_file_get_child_for_display_name (parent, display_name, &local_error);
	n = 0;
	do {
		char *new_display_name;

		if (file != NULL)
			g_object_unref (file);

		n++;
		if (n == 1)
			new_display_name = g_strdup_printf ("%s%s", display_name, suffix);
		else
			new_display_name = g_strdup_printf ("%s %d%s", display_name, n, suffix);

		file = g_file_get_child_for_display_name (parent, new_display_name, &local_error);
		if (local_error == NULL)
			stream = g_file_create (file, 0, NULL, &local_error);

		if ((stream == NULL) && g_error_matches (local_error, G_IO_ERROR, G_IO_ERROR_EXISTS))
			g_clear_error (&local_error);

		g_free (new_display_name);
	}
	while ((stream == NULL) && (local_error == NULL));

	if (stream == NULL) {
		g_object_unref (file);
		file = NULL;
	}
	else
		g_object_unref (stream);

	return file;
}


GFile *
_g_directory_create_unique (GFile       *parent,
			    const char  *display_name,
			    const char  *suffix,
			    GError     **error)
{
	GFile    *file = NULL;
	gboolean  created = FALSE;
	GError   *local_error = NULL;
	int       n;

	file = g_file_get_child_for_display_name (parent, display_name, &local_error);
	if (file == NULL) {
		g_propagate_error (error, local_error);
		return NULL;
	}

	n = 0;
	do {
		char *new_display_name;

		if (file != NULL)
			g_object_unref (file);

		n++;
		if (n == 1)
			new_display_name = g_strdup_printf ("%s%s", display_name, suffix);
		else
			new_display_name = g_strdup_printf ("%s %d%s", display_name, n, suffix);

		file = g_file_get_child_for_display_name (parent, new_display_name, &local_error);
		if (local_error == NULL)
			created = g_file_make_directory (file, NULL, &local_error);

		if (! created && g_error_matches (local_error, G_IO_ERROR, G_IO_ERROR_EXISTS))
			g_clear_error (&local_error);

		g_free (new_display_name);
	}
	while (! created && (local_error == NULL));

	if (local_error != NULL) {
		g_object_unref (file);
		file = NULL;
	}

	if (local_error != NULL)
		g_propagate_error (error, local_error);

	return file;
}


GFileType
_g_file_get_standard_type (GFile *file)
{
	GFileType  result;
	GFileInfo *info;
	GError    *error = NULL;

	info = g_file_query_info (file, G_FILE_ATTRIBUTE_STANDARD_TYPE, 0, NULL, &error);
	if (error == NULL) {
		result = g_file_info_get_file_type (info);
	}
	else {
		result = G_FILE_ATTRIBUTE_TYPE_INVALID;
		g_error_free (error);
	}

	g_object_unref (info);

	return result;
}



/* -- g_write_file -- */


gboolean
g_write_file (GFile             *file,
	      gboolean           make_backup,
	      GFileCreateFlags   flags,
	      void              *buffer,
	      gsize              count,
	      GCancellable      *cancellable,
	      GError           **error)
{
	gboolean       success;
	GOutputStream *stream;

	stream = (GOutputStream *) g_file_replace (file, NULL, make_backup, flags, cancellable, error);
	if (stream != NULL)
		success = g_output_stream_write_all (stream, buffer, count, NULL, cancellable, error);
	else
		success = FALSE;

	_g_object_unref (stream);

	return success;
}


gboolean
_g_directory_make (GFile    *file,
		   guint32   unix_mode,
		   GError  **error)
{
	if (! g_file_make_directory (file, NULL, error)) {
		if (! (*error)->code == G_IO_ERROR_EXISTS)
			return FALSE;
		g_clear_error (error);
	}

	return g_file_set_attribute_uint32 (file,
					    G_FILE_ATTRIBUTE_UNIX_MODE,
					    unix_mode,
					    G_FILE_QUERY_INFO_NONE,
					    NULL,
					    error);
}
