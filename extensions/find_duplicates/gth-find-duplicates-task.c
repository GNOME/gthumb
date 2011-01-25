/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2010 Free Software Foundation, Inc.
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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gthumb.h>
#include <extensions/catalogs/gth-catalog.h>
#include "gth-find-duplicates-task.h"


#define GET_WIDGET(x) (_gtk_builder_get_widget (self->priv->builder, (x)))
#define BUFFER_SIZE 4096


struct _GthFindDuplicatesTaskPrivate
{
	GthBrowser    *browser;
	GFile         *location;
	gboolean       recursive;
	GthTest       *test;
	GtkBuilder    *builder;
	GtkWidget     *duplicates_list;
	GString       *attributes;
	gboolean       io_operation;
	GthFileSource *file_source;
	int            n_duplicates;
	goffset        duplicates_size;
	int            n_files;
	int            n_file;
	GList         *files;
	GList         *directories;
	GFile         *current_directory;
	GthFileData   *current_file;
	guchar         buffer[BUFFER_SIZE];
	GChecksum     *checksum;
	GInputStream  *file_stream;
	GHashTable    *duplicated;
};


static gpointer parent_class = NULL;


typedef struct {
	GList   *files;
	goffset  total_size;
	int      n_files;
} DuplicatedData;


static DuplicatedData *
duplicated_data_new (void)
{
	DuplicatedData *d_data;

	d_data = g_new0 (DuplicatedData, 1);
	d_data->files = 0;
	d_data->total_size = 0;
	d_data->n_files = 0;

	return d_data;
}


static void
duplicated_data_free (DuplicatedData *d_data)
{
	_g_object_list_unref (d_data->files);
	g_free (d_data);
}


static void
gth_task_finalize (GObject *object)
{
	GthFindDuplicatesTask *self;

	self = GTH_FIND_DUPLICATES_TASK (object);

	g_object_unref (self->priv->location);
	_g_object_unref (self->priv->test);
	_g_object_unref (self->priv->builder);
	if (self->priv->attributes != NULL)
		g_string_free (self->priv->attributes, TRUE);
	_g_object_unref (self->priv->file_source);
	_g_object_list_unref (self->priv->files);
	_g_object_list_unref (self->priv->directories);
	_g_object_unref (self->priv->current_file);
	_g_object_unref (self->priv->current_directory);
	if (self->priv->checksum != NULL)
		g_checksum_free (self->priv->checksum);
	_g_object_unref (self->priv->file_stream);
	g_hash_table_unref (self->priv->duplicated);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void search_directory (GthFindDuplicatesTask *self,
			      GFile                 *directory);


static void
search_next_directory (GthFindDuplicatesTask *self)
{
	GList *first;

	if (self->priv->directories == NULL) {
		gtk_label_set_text (GTK_LABEL (GET_WIDGET ("progress_label")), _("Search completed"));
		gtk_label_set_text (GTK_LABEL (GET_WIDGET ("search_details_label")), "");
		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (GET_WIDGET ("search_progressbar")), 1.0);
		gtk_widget_set_sensitive (GET_WIDGET ("stop_button"), FALSE);
		/* FIXME */
		return;
	}

	first = self->priv->directories;
	self->priv->directories = g_list_remove_link (self->priv->directories, first);
	search_directory (self, (GFile *) first->data);

	_g_object_list_unref (first);
}


static void start_next_checksum (GthFindDuplicatesTask *self);


static void
file_input_stream_read_ready_cb (GObject      *source,
		    	    	 GAsyncResult *result,
		    	    	 gpointer      user_data)
{
	GthFindDuplicatesTask *self = user_data;
	GError                *error = NULL;
	gssize                 buffer_size;

	buffer_size = g_input_stream_read_finish (G_INPUT_STREAM (source), result, &error);
	if (buffer_size < 0) {
		gth_task_completed (GTH_TASK (self), error);
		return;
	}
	else if (buffer_size == 0) {
		const char     *checksum;
		DuplicatedData *d_data;

		self->priv->n_file += 1;

		g_object_unref (self->priv->file_stream);
		self->priv->file_stream = NULL;

		g_print ("MD5 %s: %s\n",
			 g_file_get_uri (self->priv->current_file->file),
			 g_checksum_get_string (self->priv->checksum));

		checksum = g_checksum_get_string (self->priv->checksum);
		g_file_info_set_attribute_string (self->priv->current_file->info,
						  "find-duplicates::checksum",
						  checksum);

		g_file_info_set_attribute_object (self->priv->current_file->info,
						  "find-duplicates::directory",
						  G_OBJECT (self->priv->current_directory));

		d_data = g_hash_table_lookup (self->priv->duplicated, checksum);
		if (d_data == NULL) {
			d_data = duplicated_data_new ();
			g_hash_table_insert (self->priv->duplicated, g_strdup (checksum), d_data);
		}
		d_data->files = g_list_prepend (d_data->files, g_object_ref (self->priv->current_file));
		d_data->n_files += 1;
		d_data->total_size += g_file_info_get_size (self->priv->current_file->info);
		if (d_data->n_files > 1) {
			GthFileData *file_data;
			char        *text;
			char        *size_formatted;
			GList       *list;

			file_data = g_list_last (d_data->files)->data;
			text = g_strdup_printf (g_dngettext (NULL, "%d duplicate", "%d duplicates", d_data->n_files - 1), d_data->n_files - 1);
			g_file_info_set_attribute_string (file_data->info,
							  "find-duplicates::n-duplicates",
							  text);
			g_free (text);

			self->priv->n_duplicates += 1;
			self->priv->duplicates_size += g_file_info_get_size (file_data->info);

			size_formatted = g_format_size_for_display (self->priv->duplicates_size);
			text = g_strdup_printf (g_dngettext (NULL, "%d file (%s)", "%d files (%s)", self->priv->n_duplicates), self->priv->n_duplicates, size_formatted);
			gtk_label_set_text (GTK_LABEL (GET_WIDGET ("total_duplicates_label")), text);
			g_free (text);
			g_free (size_formatted);

			list = g_list_append (NULL, file_data);

			if (d_data->n_files == 2)
				gth_file_list_add_files (GTH_FILE_LIST (self->priv->duplicates_list), list);
			else
				gth_file_list_update_files (GTH_FILE_LIST (self->priv->duplicates_list), list);

			g_list_free (list);
		}

		start_next_checksum (self);

		return;
	}

	g_checksum_update (self->priv->checksum, self->priv->buffer, buffer_size);
	g_input_stream_read_async (self->priv->file_stream,
				   self->priv->buffer,
				   BUFFER_SIZE,
				   G_PRIORITY_DEFAULT,
				   gth_task_get_cancellable (GTH_TASK (self)),
				   file_input_stream_read_ready_cb,
				   self);
}


static void
read_current_file_ready_cb (GObject      *source,
			    GAsyncResult *result,
			    gpointer      user_data)
{
	GthFindDuplicatesTask *self = user_data;
	GError                *error = NULL;

	if (self->priv->file_stream != NULL)
		g_object_unref (self->priv->file_stream);
	self->priv->file_stream = (GInputStream *) g_file_read_finish (G_FILE (source), result, &error);
	if (self->priv->file_stream == NULL) {
		start_next_checksum (self);
		return;
	}

	g_input_stream_read_async (self->priv->file_stream,
				   self->priv->buffer,
				   BUFFER_SIZE,
				   G_PRIORITY_DEFAULT,
				   gth_task_get_cancellable (GTH_TASK (self)),
				   file_input_stream_read_ready_cb,
				   self);
}


static void
start_next_checksum (GthFindDuplicatesTask *self)
{
	GList *link;
	char  *text;

	link = self->priv->files;
	if (link == NULL) {
		search_next_directory (self);
		return;
	}

	self->priv->files = g_list_remove_link (self->priv->files, link);
	_g_object_unref (self->priv->current_file);
	self->priv->current_file = (GthFileData *) link->data;
	g_list_free (link);

	text = g_strdup_printf (_("Checking file %s"), g_file_info_get_display_name (self->priv->current_file->info));
	gtk_label_set_text (GTK_LABEL (GET_WIDGET ("search_details_label")), text);
	g_free (text);

	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (GET_WIDGET ("search_progressbar")),
				       (double) (self->priv->n_file + 1) / (self->priv->n_files + 1));

	if (self->priv->checksum == NULL)
		self->priv->checksum = g_checksum_new (G_CHECKSUM_MD5);
	else
		g_checksum_reset (self->priv->checksum);

	g_file_read_async (self->priv->current_file->file,
			   G_PRIORITY_DEFAULT,
			   gth_task_get_cancellable (GTH_TASK (self)),
			   read_current_file_ready_cb,
			   self);
}


static void
done_func (GObject  *object,
	   GError   *error,
	   gpointer  user_data)
{
	GthFindDuplicatesTask *self = user_data;

	self->priv->io_operation = FALSE;

	if (error != NULL) {
		gth_task_completed (GTH_TASK (self), error);
		return;
	}

	self->priv->files = g_list_reverse (self->priv->files);
	if (self->priv->files != NULL) {
		self->priv->n_files = g_list_length (self->priv->files);
		self->priv->n_file = 0;
		start_next_checksum (self);
	}
	else
		search_next_directory (self);
}


static void
for_each_file_func (GFile     *file,
		    GFileInfo *info,
		    gpointer   user_data)
{
	GthFindDuplicatesTask *self = user_data;
	GthFileData           *file_data;

	if (g_file_info_get_file_type (info) != G_FILE_TYPE_REGULAR)
		return;

	file_data = gth_file_data_new (file, info);
	if (gth_test_match (self->priv->test, file_data))
		self->priv->files = g_list_prepend (self->priv->files, g_object_ref (file_data));

	g_object_unref (file_data);
}


static DirOp
start_dir_func (GFile      *directory,
		GFileInfo  *info,
		GError    **error,
		gpointer    user_data)
{
	GthFindDuplicatesTask *self = user_data;

	if (g_file_equal (directory, self->priv->current_directory))
		return DIR_OP_CONTINUE;

	self->priv->directories = g_list_prepend (self->priv->directories, g_object_ref (directory));

	return DIR_OP_SKIP;
}


static void
search_directory (GthFindDuplicatesTask *self,
		  GFile                 *directory)
{
	char *uri;
	char *text;

	gtk_widget_set_sensitive (GET_WIDGET ("stop_button"), TRUE);
	self->priv->io_operation = TRUE;

	_g_object_unref (self->priv->current_directory);
	self->priv->current_directory = g_object_ref (directory);

	uri = g_file_get_parse_name (self->priv->current_directory);
	text = g_strdup_printf ("Searching in %s", uri);
	gtk_label_set_text (GTK_LABEL (GET_WIDGET ("progress_label")), text);
	gtk_label_set_text (GTK_LABEL (GET_WIDGET ("search_details_label")), _("Getting the file list"));
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (GET_WIDGET ("search_progressbar")), 0.0);

	gth_file_source_for_each_child (self->priv->file_source,
					self->priv->current_directory,
					self->priv->recursive,
					self->priv->attributes->str,
					start_dir_func,
					for_each_file_func,
					done_func,
					self);

	g_free (text);
	g_free (uri);
}


static void
find_duplicates_dialog_destroy_cb (GtkWidget *dialog,
				   gpointer   user_data)
{
	GthFindDuplicatesTask *self = user_data;
	gth_task_completed (GTH_TASK (self), NULL);
}


static void
update_file_list_sensitivity (GthFindDuplicatesTask *self)
{
	GtkTreeModel *model;
	GtkTreeIter   iter;
	gboolean      one_active = FALSE;

	model = GTK_TREE_MODEL (GET_WIDGET ("files_liststore"));
	if (gtk_tree_model_get_iter_first (model, &iter)) {
		do {
			gboolean active;

			gtk_tree_model_get (model, &iter, 1, &active, -1);
			if (active) {
				one_active = TRUE;
				break;
			}
		}
		while (gtk_tree_model_iter_next(model, &iter));
	}

	gtk_widget_set_sensitive (GET_WIDGET ("view_button"), one_active);
	gtk_widget_set_sensitive (GET_WIDGET ("delete_button"), one_active);
}


static void
duplicates_list_view_selection_changed_cb (GtkIconView *iconview,
					   gpointer     user_data)
{
	GthFindDuplicatesTask *self = user_data;
	GtkWidget             *duplicates_view;
	int                    n_selected;

	duplicates_view = gth_file_list_get_view (GTH_FILE_LIST (self->priv->duplicates_list));
	n_selected = gth_file_selection_get_n_selected (GTH_FILE_SELECTION (duplicates_view));
	if (n_selected == 1) {
		GList          *items;
		GList          *file_list;
		GthFileData    *selected_file_data;
		const char     *checksum;
		DuplicatedData *d_data;
		GList          *scan;

		items = gth_file_selection_get_selected (GTH_FILE_SELECTION (duplicates_view));
		file_list = gth_file_list_get_files (GTH_FILE_LIST (self->priv->duplicates_list), items);
		selected_file_data = (GthFileData *) file_list->data;

		gtk_list_store_clear (GTK_LIST_STORE (GET_WIDGET ("files_liststore")));

		checksum = g_file_info_get_attribute_string (selected_file_data->info, "find-duplicates::checksum");
		d_data = g_hash_table_lookup (self->priv->duplicated, checksum);

		g_return_if_fail (d_data != NULL);

		for (scan = d_data->files; scan; scan = scan->next) {
			GthFileData *file_data = scan->data;
			char        *directory_name;
			GtkTreeIter  iter;

			directory_name = g_file_get_parse_name (G_FILE (g_file_info_get_attribute_object (file_data->info, "find-duplicates::directory")));
			gtk_list_store_append (GTK_LIST_STORE (GET_WIDGET ("files_liststore")), &iter);
			gtk_list_store_set (GTK_LIST_STORE (GET_WIDGET ("files_liststore")), &iter,
					    0, file_data,
					    1, TRUE,
					    2, g_file_info_get_display_name (file_data->info),
					    3, directory_name,
					    4, g_file_info_get_attribute_string (file_data->info, "gth::file::display-mtime"),
					    -1);

			g_free (directory_name);
		}

		update_file_list_sensitivity (self);

		_g_object_list_unref (file_list);
		_gtk_tree_path_list_free (items);
	}
}


static void
file_cellrenderertoggle_toggled_cb (GtkCellRendererToggle *cell_renderer,
                		    char                  *path,
                		    gpointer               user_data)
{
	GthFindDuplicatesTask *self = user_data;
	GtkTreeModel          *model;
	GtkTreePath           *tree_path;
	GtkTreeIter            iter;
	gboolean               active;

	model = GTK_TREE_MODEL (GET_WIDGET ("files_liststore"));
	tree_path = gtk_tree_path_new_from_string (path);
	if (! gtk_tree_model_get_iter (model, &iter, tree_path)) {
		gtk_tree_path_free (tree_path);
		return;
	}

	gtk_tree_model_get (model, &iter, 1, &active, -1);
	gtk_list_store_set (GTK_LIST_STORE (model), &iter, 1, ! active, -1);

	update_file_list_sensitivity (self);

	gtk_tree_path_free (tree_path);
}


static GList *
get_selected_files (GthFindDuplicatesTask *self)
{
	GtkTreeModel *model;
	GtkTreeIter   iter;
	GList        *list;

	model = GTK_TREE_MODEL (GET_WIDGET ("files_liststore"));
	if (! gtk_tree_model_get_iter_first (model, &iter))
		return NULL;

	list = NULL;
	do {
		GthFileData *file_data;
		gboolean     active;

		gtk_tree_model_get (model, &iter,
				    0, &file_data,
				    1, &active,
				    -1);
		if (active)
			list = g_list_prepend (list, g_object_ref (file_data->file));
	}
	while (gtk_tree_model_iter_next (model, &iter));

	return g_list_reverse (list);
}


static void
view_button_clicked_cb (GtkWidget *button,
			gpointer   user_data)
{
	GthFindDuplicatesTask *self = user_data;
	GList                 *files;
	GthCatalog            *catalog;
	GFile                 *catalog_file;

	files = get_selected_files (self);
	if (files == NULL)
		return;

	catalog = gth_catalog_new ();
	catalog_file = gth_catalog_file_from_relative_path (_("Duplicates"), ".catalog");
	gth_catalog_set_file (catalog, catalog_file);
	gth_catalog_set_file_list (catalog, files);
	gth_catalog_save (catalog);
	gth_browser_go_to (self->priv->browser, catalog_file, NULL);

	g_object_unref (catalog_file);
	g_object_unref (catalog);
	_g_object_list_unref (files);
}


static void
select_all_files (GthFindDuplicatesTask *self,
		  gboolean               active)
{
	GtkTreeModel *model;
	GtkTreeIter   iter;

	model = GTK_TREE_MODEL (GET_WIDGET ("files_liststore"));
	if (! gtk_tree_model_get_iter_first (model, &iter))
		return;

	do {
		gtk_list_store_set (GTK_LIST_STORE (model), &iter, 1, active, -1);
	}
	while (gtk_tree_model_iter_next (model, &iter));

	update_file_list_sensitivity (self);
}

static void
select_all_button_clicked_cb (GtkWidget *button,
			      gpointer   user_data)
{
	select_all_files (GTH_FIND_DUPLICATES_TASK (user_data), TRUE);
}


static void
unselect_all_button_clicked_cb (GtkWidget *button,
			        gpointer   user_data)
{
	select_all_files (GTH_FIND_DUPLICATES_TASK (user_data), FALSE);
}


static void
gth_find_duplicates_task_exec (GthTask *base)
{
	GthFindDuplicatesTask *self = (GthFindDuplicatesTask *) base;
	const char            *test_attributes;

	self->priv->file_source = gth_main_get_file_source (self->priv->location);
	gth_file_source_set_cancellable (self->priv->file_source, gth_task_get_cancellable (GTH_TASK (self)));

	self->priv->attributes = g_string_new (eel_gconf_get_boolean (PREF_FAST_FILE_TYPE, TRUE) ? GFILE_STANDARD_ATTRIBUTES_WITH_FAST_CONTENT_TYPE : GFILE_STANDARD_ATTRIBUTES_WITH_CONTENT_TYPE);
	g_string_append (self->priv->attributes, ",gth::file::display-size");
	test_attributes = gth_test_get_attributes (self->priv->test);
	if (test_attributes[0] != '\0') {
		g_string_append (self->priv->attributes, ",");
		g_string_append (self->priv->attributes, test_attributes);
	}

	self->priv->builder = _gtk_builder_new_from_file ("find-duplicates-dialog.ui", "find_duplicates");
	self->priv->duplicates_list = gth_file_list_new (GTH_FILE_LIST_TYPE_NORMAL, FALSE);
	gth_file_list_set_caption (GTH_FILE_LIST (self->priv->duplicates_list), "find-duplicates::n-duplicates,gth::file::display-size");
	gth_file_list_set_thumb_size (GTH_FILE_LIST (self->priv->duplicates_list), 112);
	gtk_widget_set_size_request (self->priv->duplicates_list, -1, 200);
	gtk_widget_show (self->priv->duplicates_list);
	gtk_container_add (GTK_CONTAINER (GET_WIDGET ("duplicates_list_box")), self->priv->duplicates_list);

	g_signal_connect (GET_WIDGET ("find_duplicates_dialog"),
			  "destroy",
			  G_CALLBACK (find_duplicates_dialog_destroy_cb),
			  self);
	g_signal_connect_swapped (GET_WIDGET ("close_button"),
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  GET_WIDGET ("find_duplicates_dialog"));
	g_signal_connect_swapped (GET_WIDGET ("stop_button"),
				  "clicked",
				  G_CALLBACK (gth_task_cancel),
				  self);
	g_signal_connect (G_OBJECT (gth_file_list_get_view (GTH_FILE_LIST (self->priv->duplicates_list))),
			  "selection_changed",
			  G_CALLBACK (duplicates_list_view_selection_changed_cb),
			  self);
	g_signal_connect (GET_WIDGET ("file_cellrenderertoggle"),
			  "toggled",
			  G_CALLBACK (file_cellrenderertoggle_toggled_cb),
			  self);
	g_signal_connect (GET_WIDGET ("view_button"),
			  "clicked",
			  G_CALLBACK (view_button_clicked_cb),
			  self);
	g_signal_connect (GET_WIDGET ("select_all_button"),
			  "clicked",
			  G_CALLBACK (select_all_button_clicked_cb),
			  self);
	g_signal_connect (GET_WIDGET ("unselect_all_button"),
			  "clicked",
			  G_CALLBACK (unselect_all_button_clicked_cb),
			  self);

	gtk_widget_show (GET_WIDGET ("find_duplicates_dialog"));
	gth_task_dialog (GTH_TASK (self), TRUE, GET_WIDGET ("find_duplicates_dialog"));

	search_directory (self, self->priv->location);
}


static void
gth_find_duplicates_task_cancelled (GthTask *self)
{
	/* FIXME
	if (! GTH_FIND_DUPLICATES_TASK (self)->priv->io_operation)
		gth_task_completed (self, g_error_new_literal (GTH_TASK_ERROR, GTH_TASK_ERROR_CANCELLED, ""));
	*/
}


static void
gth_find_duplicates_task_class_init (GthFindDuplicatesTaskClass *class)
{
	GObjectClass *object_class;
	GthTaskClass *task_class;

	parent_class = g_type_class_peek_parent (class);

	object_class = (GObjectClass*) class;
	object_class->finalize = gth_task_finalize;

	task_class = (GthTaskClass*) class;
	task_class->exec = gth_find_duplicates_task_exec;
	task_class->cancelled = gth_find_duplicates_task_cancelled;
}


static void
gth_find_duplicates_task_init (GthFindDuplicatesTask *self)
{
	self->priv = g_new0 (GthFindDuplicatesTaskPrivate, 1);
	self->priv->test = NULL;
	self->priv->builder = NULL;
	self->priv->attributes = NULL;
	self->priv->io_operation = FALSE;
	self->priv->n_duplicates = 0;
	self->priv->duplicates_size = 0;
	self->priv->file_source = NULL;
	self->priv->files = NULL;
	self->priv->directories = NULL;
	self->priv->current_directory = NULL;
	self->priv->current_file = NULL;
	self->priv->checksum = NULL;
	self->priv->file_stream = NULL;
	self->priv->duplicated = g_hash_table_new_full (g_str_hash,
							g_str_equal,
							g_free,
							(GDestroyNotify) duplicated_data_free);
}


GType
gth_find_duplicates_task_get_type (void)
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (GthFindDuplicatesTaskClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_find_duplicates_task_class_init,
			NULL,
			NULL,
			sizeof (GthFindDuplicatesTask),
			0,
			(GInstanceInitFunc) gth_find_duplicates_task_init
		};

		type = g_type_register_static (GTH_TYPE_TASK,
					       "GthFindDuplicatesTask",
					       &type_info,
					       0);
	}

	return type;
}


GthTask *
gth_find_duplicates_task_new (GthBrowser *browser,
			      GFile      *location,
			      gboolean    recursive,
			      const char *filter)
{
	GthFindDuplicatesTask *self;

	self = (GthFindDuplicatesTask *) g_object_new (GTH_TYPE_FIND_DUPLICATES_TASK, NULL);

	self->priv->browser = browser;
	self->priv->location = g_object_ref (location);
	self->priv->recursive = recursive;
	if (filter != NULL)
		self->priv->test = gth_main_get_registered_object (GTH_TYPE_TEST, filter);

	return (GthTask*) self;
}
