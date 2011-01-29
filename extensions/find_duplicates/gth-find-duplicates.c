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
#include <extensions/file_manager/actions.h>
#include "gth-find-duplicates.h"


#define GET_WIDGET(x) (_gtk_builder_get_widget (self->priv->builder, (x)))
#define BUFFER_SIZE 4096


struct _GthFindDuplicatesPrivate
{
	GthBrowser    *browser;
	GFile         *location;
	gboolean       recursive;
	GthTest       *test;
	GtkBuilder    *builder;
	GtkWidget     *duplicates_list;
	GString       *attributes;
	GCancellable  *cancellable;
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
gth_find_duplicates_finalize (GObject *object)
{
	GthFindDuplicates *self;

	self = GTH_FIND_DUPLICATES (object);

	g_object_unref (self->priv->location);
	_g_object_unref (self->priv->test);
	_g_object_unref (self->priv->builder);
	if (self->priv->attributes != NULL)
		g_string_free (self->priv->attributes, TRUE);
	g_object_unref (self->priv->cancellable);
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


static void
gth_find_duplicates_class_init (GthFindDuplicatesClass *klass)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GthFindDuplicatesPrivate));

	object_class = (GObjectClass*) klass;
	object_class->finalize = gth_find_duplicates_finalize;
}


static void
gth_find_duplicates_init (GthFindDuplicates *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_FIND_DUPLICATES, GthFindDuplicatesPrivate);
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
	self->priv->cancellable = g_cancellable_new ();
}


GType
gth_find_duplicates_get_type (void)
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (GthFindDuplicatesClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_find_duplicates_class_init,
			NULL,
			NULL,
			sizeof (GthFindDuplicates),
			0,
			(GInstanceInitFunc) gth_find_duplicates_init
		};

		type = g_type_register_static (G_TYPE_OBJECT,
					       "GthFindDuplicates",
					       &type_info,
					       0);
	}

	return type;
}


static void start_next_checksum (GthFindDuplicates *self);


static void
file_input_stream_read_ready_cb (GObject      *source,
		    	    	 GAsyncResult *result,
		    	    	 gpointer      user_data)
{
	GthFindDuplicates *self = user_data;
	GError            *error = NULL;
	gssize             buffer_size;

	buffer_size = g_input_stream_read_finish (G_INPUT_STREAM (source), result, &error);
	if (buffer_size < 0) {
		start_next_checksum (self);
		return;
	}
	else if (buffer_size == 0) {
		const char     *checksum;
		DuplicatedData *d_data;

		self->priv->n_file += 1;

		g_object_unref (self->priv->file_stream);
		self->priv->file_stream = NULL;

		checksum = g_checksum_get_string (self->priv->checksum);
		g_file_info_set_attribute_string (self->priv->current_file->info,
						  "find-duplicates::checksum",
						  checksum);

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
				   self->priv->cancellable,
				   file_input_stream_read_ready_cb,
				   self);
}


static void
read_current_file_ready_cb (GObject      *source,
			    GAsyncResult *result,
			    gpointer      user_data)
{
	GthFindDuplicates *self = user_data;
	GError            *error = NULL;

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
				   self->priv->cancellable,
				   file_input_stream_read_ready_cb,
				   self);
}


static void
start_next_checksum (GthFindDuplicates *self)
{
	GList *link;
	char  *text;
	int    n_remaining;

	link = self->priv->files;
	if (link == NULL) {
		gtk_notebook_set_current_page (GTK_NOTEBOOK (GET_WIDGET ("pages_notebook")), (self->priv->n_duplicates > 0) ? 0 : 1);
		gtk_label_set_text (GTK_LABEL (GET_WIDGET ("progress_label")), _("Search completed"));
		gtk_label_set_text (GTK_LABEL (GET_WIDGET ("search_details_label")), "");
		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (GET_WIDGET ("search_progressbar")), 1.0);
		gtk_widget_set_sensitive (GET_WIDGET ("stop_button"), FALSE);
		return;
	}

	self->priv->files = g_list_remove_link (self->priv->files, link);
	_g_object_unref (self->priv->current_file);
	self->priv->current_file = (GthFileData *) link->data;
	g_list_free (link);

	gtk_label_set_text (GTK_LABEL (GET_WIDGET ("progress_label")), _("Searching for duplicates"));

	n_remaining = self->priv->n_files - self->priv->n_file;
	text = g_strdup_printf (g_dngettext (NULL, "%d file remaining", "%d files remaining", n_remaining), n_remaining);
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
			   self->priv->cancellable,
			   read_current_file_ready_cb,
			   self);
}


static void
done_func (GObject  *object,
	   GError   *error,
	   gpointer  user_data)
{
	GthFindDuplicates *self = user_data;

	self->priv->io_operation = FALSE;

	if ((error != NULL) && ! g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (self->priv->browser), _("Could not perform the operation"), &error);
		gtk_widget_destroy (GET_WIDGET ("find_duplicates_dialog"));
		return;
	}

	self->priv->files = g_list_reverse (self->priv->files);
	self->priv->n_files = g_list_length (self->priv->files);
	self->priv->n_file = 0;
	start_next_checksum (self);
}


static void
for_each_file_func (GFile     *file,
		    GFileInfo *info,
		    gpointer   user_data)
{
	GthFindDuplicates *self = user_data;
	GthFileData       *file_data;

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
	GthFindDuplicates *self = user_data;

	_g_object_unref (self->priv->current_directory);
	self->priv->current_directory = g_object_ref (directory);

	return DIR_OP_CONTINUE;
}


static void
search_directory (GthFindDuplicates *self,
		  GFile             *directory)
{
	gtk_widget_set_sensitive (GET_WIDGET ("stop_button"), TRUE);
	self->priv->io_operation = TRUE;

	gtk_label_set_text (GTK_LABEL (GET_WIDGET ("progress_label")), _("Getting the file list"));
	gtk_label_set_text (GTK_LABEL (GET_WIDGET ("search_details_label")), "");
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (GET_WIDGET ("search_progressbar")), 0.0);

	gth_file_source_for_each_child (self->priv->file_source,
					directory,
					self->priv->recursive,
					self->priv->attributes->str,
					start_dir_func,
					for_each_file_func,
					done_func,
					self);
}


static void
find_duplicates_dialog_destroy_cb (GtkWidget *dialog,
				   gpointer   user_data)
{
	g_object_unref (GTH_FIND_DUPLICATES (user_data));
}


static void
update_file_list_sensitivity (GthFindDuplicates *self)
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
	GthFindDuplicates *self = user_data;
	GtkWidget         *duplicates_view;
	int                n_selected;

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
			GFile       *parent;
			char        *parent_name;
			GtkTreeIter  iter;

			parent = g_file_get_parent (file_data->file);
			if (parent != NULL)
				parent_name = g_file_get_parse_name (parent);
			else
				parent_name = NULL;
			gtk_list_store_append (GTK_LIST_STORE (GET_WIDGET ("files_liststore")), &iter);
			gtk_list_store_set (GTK_LIST_STORE (GET_WIDGET ("files_liststore")), &iter,
					    0, file_data,
					    1, TRUE,
					    2, g_file_info_get_display_name (file_data->info),
					    3, parent_name,
					    4, g_file_info_get_attribute_string (file_data->info, "gth::file::display-mtime"),
					    -1);

			g_free (parent_name);
			g_object_unref (parent);
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
	GthFindDuplicates *self = user_data;
	GtkTreeModel      *model;
	GtkTreePath       *tree_path;
	GtkTreeIter        iter;
	gboolean           active;

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
get_selected_files (GthFindDuplicates *self)
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
			list = g_list_prepend (list, g_object_ref (file_data));
	}
	while (gtk_tree_model_iter_next (model, &iter));

	return g_list_reverse (list);
}


static void
view_button_clicked_cb (GtkWidget *button,
			gpointer   user_data)
{
	GthFindDuplicates *self = user_data;
	GList             *file_data_list;
	GList             *file_list;
	GthCatalog        *catalog;
	GFile             *catalog_file;

	file_data_list = get_selected_files (self);
	if (file_data_list == NULL)
		return;

	file_list = gth_file_data_list_to_file_list (file_data_list);
	catalog = gth_catalog_new ();
	catalog_file = gth_catalog_file_from_relative_path (_("Duplicates"), ".catalog");
	gth_catalog_set_file (catalog, catalog_file);
	gth_catalog_set_file_list (catalog, file_list);
	gth_catalog_save (catalog);
	gth_browser_go_to (self->priv->browser, catalog_file, NULL);

	g_object_unref (catalog_file);
	g_object_unref (catalog);
	_g_object_list_unref (file_list);
	_g_object_list_unref (file_data_list);
}


static void
delete_button_clicked_cb (GtkWidget *button,
			  gpointer   user_data)
{
	GthFindDuplicates *self = user_data;
	GList             *file_data_list;

	file_data_list = get_selected_files (self);
	if (file_data_list != NULL)
		gth_file_mananger_delete_files (GTK_WINDOW (GET_WIDGET ("find_duplicates_dialog")), file_data_list);

	_g_object_list_unref (file_data_list);
}


static void
select_all_files (GthFindDuplicates *self,
		  gboolean           active)
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
	select_all_files (GTH_FIND_DUPLICATES (user_data), TRUE);
}


static void
unselect_all_button_clicked_cb (GtkWidget *button,
			        gpointer   user_data)
{
	select_all_files (GTH_FIND_DUPLICATES (user_data), FALSE);
}


void
gth_find_duplicates_exec (GthBrowser *browser,
		     	  GFile      *location,
		     	  gboolean    recursive,
		     	  const char *filter)
{
	GthFindDuplicates *self;
	const char        *test_attributes;

	self = (GthFindDuplicates *) g_object_new (GTH_TYPE_FIND_DUPLICATES, NULL);

	self->priv->browser = browser;
	self->priv->location = g_object_ref (location);
	self->priv->recursive = recursive;
	if (filter != NULL)
		self->priv->test = gth_main_get_registered_object (GTH_TYPE_TEST, filter);

	self->priv->file_source = gth_main_get_file_source (self->priv->location);
	gth_file_source_set_cancellable (self->priv->file_source, self->priv->cancellable);

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
				  G_CALLBACK (g_cancellable_cancel),
				  self->priv->cancellable);
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
	g_signal_connect (GET_WIDGET ("delete_button"),
			  "clicked",
			  G_CALLBACK (delete_button_clicked_cb),
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
	gtk_window_set_transient_for (GTK_WINDOW (GET_WIDGET ("find_duplicates_dialog")), GTK_WINDOW (self->priv->browser));

	search_directory (self, self->priv->location);
}
