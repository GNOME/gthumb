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
#include <glib.h>
#include <glib/gi18n.h>
#include <gthumb.h>
#include "gth-catalog.h"
#include "gth-organize-task.h"


#define GET_WIDGET(name) _gtk_builder_get_widget (self->priv->builder, (name))
#define KEY_FORMAT ("%Y.%m.%d")

enum {
	NAME_COLUMN = 0,
	CARDINALITY_COLUMN,
	CREATE_CATALOG_COLUMN,
	KEY_COLUMN,
	ICON_COLUMN
};


struct _GthOrganizeTaskPrivate
{
	GthBrowser     *browser;
	GFile          *folder;
	GthGroupPolicy  group_policy;
	gboolean        recursive;
	gboolean        create_singletons;
	GthCatalog     *singletons_catalog;
	GtkBuilder     *builder;
	GtkListStore   *results_liststore;
	GHashTable     *catalogs;
	GdkPixbuf      *icon_pixbuf;
	gboolean        organized;
	GtkWidget      *file_list;
	int             n_catalogs;
	int             n_files;
};


static gpointer parent_class = NULL;


static void
gth_organize_task_finalize (GObject *object)
{
	GthOrganizeTask *self;

	self = GTH_ORGANIZE_TASK (object);

	gtk_widget_destroy (GET_WIDGET ("organize_files_dialog"));
	g_object_unref (self->priv->folder);
	_g_object_unref (self->priv->singletons_catalog);
	g_object_unref (self->priv->builder);
	g_hash_table_destroy (self->priv->catalogs);
	g_object_unref (self->priv->icon_pixbuf);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
save_catalog (gpointer key,
	      gpointer value,
	      gpointer user_data)
{
	GthCatalog *catalog = value;

	gth_catalog_save (catalog);
}


static void
save_catalogs (GthOrganizeTask *self)
{
	GtkTreeIter iter;

	if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (self->priv->results_liststore), &iter)) {
		do {
			char     *key;
			char     *name;
			gboolean  create;

			gtk_tree_model_get (GTK_TREE_MODEL (self->priv->results_liststore), &iter,
					    KEY_COLUMN, &key,
					    NAME_COLUMN, &name,
					    CREATE_CATALOG_COLUMN, &create,
					    -1);
			if (create) {
				GthCatalog *catalog;
				char       *original_name;

				catalog = g_hash_table_lookup (self->priv->catalogs, key);

				/* remove the name if it is equal to the date
				 * to avoid a duplication in the display-name
				 * attribute. */
				original_name = gth_datetime_strftime (gth_catalog_get_date (catalog), "%x");
				if (g_strcmp0 (original_name, name) != 0)
					gth_catalog_set_name (catalog, name);
				else
					gth_catalog_set_name (catalog, NULL);

				g_free (original_name);
			}
			else
				g_hash_table_remove (self->priv->catalogs, key);

			g_free (name);
			g_free (key);
		}
		while (gtk_tree_model_iter_next (GTK_TREE_MODEL (self->priv->results_liststore), &iter));
	}
	g_hash_table_foreach (self->priv->catalogs, save_catalog, NULL);

	gth_task_completed (GTH_TASK (self), NULL);
}


static void
done_func (GError   *error,
	   gpointer  user_data)
{
	GthOrganizeTask *self = user_data;
	char            *status_text;

	if ((error != NULL) && ! g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
		gth_task_completed (GTH_TASK (self), error);
		return;
	}

	if (! self->priv->create_singletons) {
		GtkTreeIter iter;
		int         singletons = 0;

		if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (self->priv->results_liststore), &iter)) {
			do {
				char *key;
				int   n;

				gtk_tree_model_get (GTK_TREE_MODEL (self->priv->results_liststore), &iter,
						    KEY_COLUMN, &key,
						    CARDINALITY_COLUMN, &n,
						    -1);
				if (n == 1) {
					GthCatalog *catalog;
					GList      *file_list;

					gtk_list_store_set (self->priv->results_liststore, &iter,
							    CREATE_CATALOG_COLUMN, FALSE,
							    -1);
					singletons++;
					catalog = g_hash_table_lookup (self->priv->catalogs, key);
					file_list = gth_catalog_get_file_list (catalog);
					gth_catalog_insert_file (self->priv->singletons_catalog, file_list->data, -1);
					if (singletons == 1)
						g_hash_table_insert (self->priv->catalogs,
								     g_strdup (gth_catalog_get_name (self->priv->singletons_catalog)),
								     g_object_ref (self->priv->singletons_catalog));
				}

				g_free (key);
			}
			while (gtk_tree_model_iter_next (GTK_TREE_MODEL (self->priv->results_liststore), &iter));
		}

		if ((self->priv->singletons_catalog != NULL) && (singletons > 0)) {
			gtk_list_store_append (self->priv->results_liststore, &iter);
			gtk_list_store_set (self->priv->results_liststore, &iter,
					    KEY_COLUMN, gth_catalog_get_name (self->priv->singletons_catalog),
					    NAME_COLUMN, gth_catalog_get_name (self->priv->singletons_catalog),
					    CARDINALITY_COLUMN, singletons,
					    CREATE_CATALOG_COLUMN, TRUE,
					    ICON_COLUMN, self->priv->icon_pixbuf,
					    -1);
		}
	}
	self->priv->organized = TRUE;

	status_text = g_strdup_printf (_("Operation completed. Catalogs: %d. Images: %d."), self->priv->n_catalogs, self->priv->n_files);
	gtk_label_set_text (GTK_LABEL (GET_WIDGET ("progress_label")), status_text);
	gtk_label_set_ellipsize (GTK_LABEL (GET_WIDGET ("progress_label")), PANGO_ELLIPSIZE_NONE);
	g_free (status_text);

	gtk_widget_hide (GET_WIDGET ("cancel_button"));
	gtk_widget_show (GET_WIDGET ("close_button"));
	gtk_widget_show (GET_WIDGET ("ok_button"));
}


static void
for_each_file_func (GFile     *file,
		    GFileInfo *info,
		    gpointer   user_data)
{
	GthOrganizeTask *self = user_data;
	GthFileData     *file_data;
	char            *key;
	GTimeVal         timeval;
	GthCatalog      *catalog;

	if (g_file_info_get_file_type (info) != G_FILE_TYPE_REGULAR)
		return;

	key = NULL;
	file_data = gth_file_data_new (file, info);
	switch (self->priv->group_policy) {
	case GTH_GROUP_POLICY_DIGITALIZED_DATE:
		{
			GObject *metadata;

			metadata = g_file_info_get_attribute_object (info, "Embedded::Photo::DateTimeOriginal");
			if (metadata != NULL) {
				if (_g_time_val_from_exif_date (gth_metadata_get_raw (GTH_METADATA (metadata)), &timeval))
					key = _g_time_val_strftime (&timeval, KEY_FORMAT);
			}
		}
		break;
	case GTH_GROUP_POLICY_MODIFIED_DATE:
		timeval = *gth_file_data_get_modification_time (file_data);
		key = _g_time_val_strftime (&timeval, KEY_FORMAT);
		break;
	}

	if (key == NULL)
		return;

	catalog = g_hash_table_lookup (self->priv->catalogs, key);
	if (catalog == NULL) {
		GthDateTime *date_time;
		GFile       *catalog_file;
		char        *name;
		GtkTreeIter  iter;

		date_time = gth_datetime_new ();
		gth_datetime_from_timeval (date_time, &timeval);

		catalog_file = gth_catalog_get_file_for_date (date_time);
		catalog = gth_catalog_load_from_file (catalog_file);
		if (catalog == NULL)
			catalog = gth_catalog_new ();
		gth_catalog_set_for_date (catalog, date_time);

		g_hash_table_insert (self->priv->catalogs, g_strdup (key), catalog);

		name = gth_datetime_strftime (date_time, "%x");
		gtk_list_store_append (self->priv->results_liststore, &iter);
		gtk_list_store_set (self->priv->results_liststore, &iter,
				    KEY_COLUMN, key,
				    NAME_COLUMN, name,
				    CARDINALITY_COLUMN, 0,
				    CREATE_CATALOG_COLUMN, TRUE,
				    ICON_COLUMN, self->priv->icon_pixbuf,
				    -1);
		self->priv->n_catalogs++;

		g_free (name);
		g_object_unref (catalog_file);
		gth_datetime_free (date_time);
	}

	if (catalog != NULL) {
		GtkTreeIter iter;
		int         n = 0;

		if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (self->priv->results_liststore), &iter)) {
			do {
				char *k;

				gtk_tree_model_get (GTK_TREE_MODEL (self->priv->results_liststore),
						    &iter,
						    KEY_COLUMN, &k,
						    CARDINALITY_COLUMN, &n,
						    -1);
				if (g_strcmp0 (k, key) == 0) {
					gtk_list_store_set (self->priv->results_liststore, &iter,
							    CARDINALITY_COLUMN, n + 1,
							    -1);
					self->priv->n_files++;

					g_free (k);
					break;
				}

				g_free (k);
			}
			while (gtk_tree_model_iter_next (GTK_TREE_MODEL (self->priv->results_liststore), &iter));
		}

		gth_catalog_insert_file (catalog, file_data->file, -1);
	}

	g_object_unref (file_data);
	g_free (key);
}


static DirOp
start_dir_func (GFile      *directory,
		GFileInfo  *info,
		GError    **error,
		gpointer    user_data)
{
	GthOrganizeTask *self = user_data;
	char            *uri;
	char            *text;

	uri = g_file_get_parse_name (directory);
	text = g_strdup_printf ("Searching in %s", uri);
	gtk_label_set_text (GTK_LABEL (GET_WIDGET ("progress_label")), text);

	g_free (text);
	g_free (uri);

	return DIR_OP_CONTINUE;
}


static void
gth_organize_task_exec (GthTask *base)
{
	GthOrganizeTask *self;
	const char      *attributes;

	self = GTH_ORGANIZE_TASK (base);

	self->priv->organized = FALSE;
	self->priv->n_catalogs = 0;
	self->priv->n_files = 0;
	gtk_list_store_clear (self->priv->results_liststore);
	switch (self->priv->group_policy) {
	case GTH_GROUP_POLICY_DIGITALIZED_DATE:
		attributes = "standard::name,standard::type,time::modified,time::modified-usec,Embedded::Photo::DateTimeOriginal";
		break;
	case GTH_GROUP_POLICY_MODIFIED_DATE:
	default:
		attributes = "standard::name,standard::type,time::modified,time::modified-usec";
		break;
	}
	g_directory_foreach_child (self->priv->folder,
				   self->priv->recursive,
				   TRUE,
				   attributes,
				   gth_task_get_cancellable (GTH_TASK (self)),
				   start_dir_func,
				   for_each_file_func,
				   done_func,
				   self);

	gtk_widget_show (GET_WIDGET ("cancel_button"));
	gtk_widget_hide (GET_WIDGET ("close_button"));
	gtk_widget_hide (GET_WIDGET ("ok_button"));
	gtk_window_set_transient_for (GTK_WINDOW (GET_WIDGET ("organize_files_dialog")), GTK_WINDOW (self->priv->browser));
	gtk_window_set_modal (GTK_WINDOW (GET_WIDGET ("organize_files_dialog")), TRUE);
	gtk_widget_show (GET_WIDGET ("organize_files_dialog"));

	gth_task_dialog (base, TRUE);
}


static void
gth_organize_task_cancelled (GthTask *base)
{
	/* FIXME */
}


static void
organize_files_dialog_response_cb (GtkDialog *dialog,
				   int        response_id,
				   gpointer   user_data)
{
	GthOrganizeTask *self = user_data;

	if (response_id == GTK_RESPONSE_DELETE_EVENT) {
		if (self->priv->organized)
			response_id = GTK_RESPONSE_CLOSE;
		else
			response_id = GTK_RESPONSE_CANCEL;
	}

	switch (response_id) {
	case GTK_RESPONSE_CANCEL:
		gth_task_cancel (GTH_TASK (self));
		break;

	case GTK_RESPONSE_CLOSE:
		gth_task_completed (GTH_TASK (self), NULL);
		break;

	case GTK_RESPONSE_OK:
		save_catalogs (self);
		break;
	}
}


static void
gth_organize_task_class_init (GthOrganizeTaskClass *klass)
{
	GObjectClass *object_class;
	GthTaskClass *task_class;

	parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GthOrganizeTaskPrivate));

	object_class = (GObjectClass*) klass;
	object_class->finalize = gth_organize_task_finalize;

	task_class = (GthTaskClass*) klass;
	task_class->exec = gth_organize_task_exec;
	task_class->cancelled = gth_organize_task_cancelled;
}


static void
catalog_name_cellrenderertext_edited_cb (GtkCellRendererText *renderer,
					 char                *path,
					 char                *new_text,
					 gpointer             user_data)
{
	GthOrganizeTask *self = user_data;
	GtkTreePath     *tree_path;
	GtkTreeIter      iter;

	tree_path = gtk_tree_path_new_from_string (path);
	if (! gtk_tree_model_get_iter (GTK_TREE_MODEL (self->priv->results_liststore),
				       &iter,
				       tree_path))
	{
		gtk_tree_path_free (tree_path);
		return;
	}
	gtk_tree_path_free (tree_path);

	gtk_list_store_set (self->priv->results_liststore,
			    &iter,
			    NAME_COLUMN, new_text,
			    -1);
}


static void
create_cellrenderertoggle_toggled_cb (GtkCellRendererToggle *cell_renderer,
				      char                  *path,
				      gpointer               user_data)
{
	GthOrganizeTask *self = user_data;
	GtkTreePath     *tpath;
	GtkTreeIter      iter;

	tpath = gtk_tree_path_new_from_string (path);
	if (tpath == NULL)
		return;

	if (gtk_tree_model_get_iter (GTK_TREE_MODEL (self->priv->results_liststore), &iter, tpath)) {
		gboolean create;

		gtk_tree_model_get (GTK_TREE_MODEL (self->priv->results_liststore), &iter,
				    CREATE_CATALOG_COLUMN, &create,
				    -1);
		gtk_list_store_set (self->priv->results_liststore, &iter,
				    CREATE_CATALOG_COLUMN, ! create,
				    -1);
	}

	gtk_tree_path_free (tpath);
}


static void
file_list_info_ready_cb (GList    *files,
			 GError   *error,
			 gpointer  user_data)
{
	GthOrganizeTask *self = user_data;

	if (error != NULL)
		return;

	gth_file_list_set_files (GTH_FILE_LIST (self->priv->file_list), files);
}


static void
organization_treeview_selection_changed_cb (GtkTreeSelection *treeselection,
                                            gpointer          user_data)
{
	GthOrganizeTask *self = user_data;
	GtkTreeIter      iter;
	char            *key;
	GthCatalog      *catalog;

	if (! self->priv->organized)
		return;
	if (! gtk_tree_selection_get_selected (treeselection, NULL, &iter))
		return;

	gtk_tree_model_get (GTK_TREE_MODEL (self->priv->results_liststore), &iter,
			    KEY_COLUMN, &key,
			    -1);
	catalog = g_hash_table_lookup (self->priv->catalogs, key);
	if (catalog != NULL) {
		GList *file_list;

		gtk_widget_show (GET_WIDGET ("preview_box"));

		file_list = gth_catalog_get_file_list (catalog);
		_g_query_info_async (file_list,
				     FALSE,
				     TRUE,
				     GFILE_STANDARD_ATTRIBUTES_WITH_FAST_CONTENT_TYPE,
				     NULL,
				     file_list_info_ready_cb,
				     self);
	}

	g_free (key);
}


static void
select_all_button_clicked_cb (GtkButton *button,
			      gpointer   user_data)
{
	GthOrganizeTask *self = user_data;
	GtkTreeIter      iter;

	if (! gtk_tree_model_get_iter_first (GTK_TREE_MODEL (self->priv->results_liststore), &iter))
		return;
	do {
		gtk_list_store_set (self->priv->results_liststore, &iter,
				    CREATE_CATALOG_COLUMN, TRUE,
				    -1);
	}
	while (gtk_tree_model_iter_next (GTK_TREE_MODEL (self->priv->results_liststore), &iter));
}


static void
select_none_button_clicked_cb (GtkButton *button,
			       gpointer   user_data)
{
	GthOrganizeTask *self = user_data;
	GtkTreeIter      iter;

	if (! gtk_tree_model_get_iter_first (GTK_TREE_MODEL (self->priv->results_liststore), &iter))
		return;
	do {
		gtk_list_store_set (self->priv->results_liststore, &iter,
				    CREATE_CATALOG_COLUMN, FALSE,
				    -1);
	}
	while (gtk_tree_model_iter_next (GTK_TREE_MODEL (self->priv->results_liststore), &iter));
}


static void
gth_organize_task_init (GthOrganizeTask *self)
{
	GIcon *icon;

	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_ORGANIZE_TASK, GthOrganizeTaskPrivate);
	self->priv->builder = _gtk_builder_new_from_file ("organize-files-task.ui", "catalogs");
	self->priv->results_liststore = (GtkListStore *) gtk_builder_get_object (self->priv->builder, "results_liststore");
	self->priv->catalogs = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);

	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (self->priv->results_liststore), KEY_COLUMN, GTK_SORT_ASCENDING);
	g_object_set (GET_WIDGET ("catalog_name_cellrenderertext"), "editable", TRUE, NULL);

	icon = g_themed_icon_new ("file-catalog");
	self->priv->icon_pixbuf = _g_icon_get_pixbuf (icon,
						      _gtk_icon_get_pixel_size (GET_WIDGET ("organization_treeview"), GTK_ICON_SIZE_MENU),
						      gtk_icon_theme_get_for_screen (gtk_widget_get_screen (GET_WIDGET ("organization_treeview"))));
	g_object_unref (icon);

	self->priv->file_list = gth_file_list_new (GTH_FILE_LIST_TYPE_NORMAL);
	gth_file_list_set_caption (GTH_FILE_LIST (self->priv->file_list), "standard::display-name");
	gtk_widget_show (self->priv->file_list);
	gtk_box_pack_start (GTK_BOX (GET_WIDGET ("preview_box")), self->priv->file_list, TRUE, TRUE, 0);

	g_signal_connect (GET_WIDGET ("catalog_name_cellrenderertext"),
			  "edited",
			  G_CALLBACK (catalog_name_cellrenderertext_edited_cb),
			  self);
	g_signal_connect (GET_WIDGET ("create_cellrenderertoggle"),
			  "toggled",
			  G_CALLBACK (create_cellrenderertoggle_toggled_cb),
			  self);
	g_signal_connect (GET_WIDGET ("organize_files_dialog"),
			  "response",
			  G_CALLBACK (organize_files_dialog_response_cb),
			  self);
	g_signal_connect (gtk_tree_view_get_selection (GTK_TREE_VIEW (GET_WIDGET ("organization_treeview"))),
			  "changed",
			  G_CALLBACK (organization_treeview_selection_changed_cb),
			  self);
	g_signal_connect (GET_WIDGET ("select_all_button"),
			  "clicked",
			  G_CALLBACK (select_all_button_clicked_cb),
			  self);
	g_signal_connect (GET_WIDGET ("select_none_button"),
			  "clicked",
			  G_CALLBACK (select_none_button_clicked_cb),
			  self);
}


GType
gth_organize_task_get_type (void)
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (GthOrganizeTaskClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_organize_task_class_init,
			NULL,
			NULL,
			sizeof (GthOrganizeTask),
			0,
			(GInstanceInitFunc) gth_organize_task_init
		};

		type = g_type_register_static (GTH_TYPE_TASK,
					       "GthOrganizeTask",
					       &type_info,
					       0);
	}

	return type;
}


GthTask *
gth_organize_task_new (GthBrowser     *browser,
		       GFile          *folder,
		       GthGroupPolicy  group_policy)
{
	GthOrganizeTask *self;

	self = (GthOrganizeTask *) g_object_new (GTH_TYPE_ORGANIZE_TASK, NULL);
	self->priv->browser = browser;
	self->priv->folder = g_file_dup (folder);
	self->priv->group_policy = group_policy;

	return (GthTask*) self;
}


void
gth_organize_task_set_recursive (GthOrganizeTask *self,
				 gboolean         recursive)
{
	self->priv->recursive = recursive;
}


void
gth_organize_task_set_create_singletons (GthOrganizeTask *self,
					 gboolean         create)
{
	self->priv->create_singletons = create;
}


void
gth_organize_task_set_singletons_catalog (GthOrganizeTask *self,
					  const char      *catalog_name)
{
	GFile *file;

	_g_object_unref (self->priv->singletons_catalog);
	self->priv->singletons_catalog = NULL;
	if (catalog_name == NULL)
		return;

	self->priv->singletons_catalog = gth_catalog_new ();
	file = _g_file_new_for_display_name ("catalog:///", catalog_name, ".catalog");
	gth_catalog_set_file (self->priv->singletons_catalog, file);
	gth_catalog_set_name (self->priv->singletons_catalog, catalog_name);

	g_object_unref (file);
}
