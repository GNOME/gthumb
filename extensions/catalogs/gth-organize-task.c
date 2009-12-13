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
	CREATE_CATALOG_COLUMN
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
};


static gpointer parent_class = NULL;


static void
gth_organize_task_finalize (GObject *object)
{
	GthOrganizeTask *self;

	self = GTH_ORGANIZE_TASK (object);

	g_object_unref (self->priv->folder);
	_g_object_unref (self->priv->singletons_catalog);
	g_object_unref (self->priv->builder);
	g_hash_table_destroy (self->priv->catalogs);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
save_catalog (gpointer key,
	      gpointer value,
	      gpointer user_data)
{
	GthOrganizeTask *self = user_data;
	GthCatalog      *catalog = value;
	GFile           *gio_file;
	GFile           *gio_parent;
	char            *data;
	gsize            size;
	GError          *error = NULL;

	if (! self->priv->create_singletons) {
		GList *file_list = gth_catalog_get_file_list (catalog);
		if ((file_list == NULL) || (file_list->next == NULL))
			return;
	}

	gio_file = gth_catalog_file_to_gio_file (gth_catalog_get_file (catalog));
	gio_parent = g_file_get_parent (gio_file);
	g_file_make_directory_with_parents (gio_parent, NULL, NULL);
	data = gth_catalog_to_data (catalog, &size);
	if (! g_write_file (gio_file,
			    FALSE,
			    G_FILE_CREATE_NONE,
			    data,
			    size,
			    gth_task_get_cancellable (GTH_TASK (self)),
			    &error))
	{
		g_warning ("%s", error->message);
		g_clear_error (&error);
	}

	g_free (data);
	g_object_unref (gio_parent);
	g_object_unref (gio_file);
}


static void
create_singletons_catalog (gpointer key,
		           gpointer value,
		           gpointer user_data)
{
	GthOrganizeTask *self = user_data;
	GthCatalog      *catalog = value;
	GList           *file_list;

	file_list = gth_catalog_get_file_list (catalog);
	if ((file_list != NULL) && (file_list->next == NULL))
		gth_catalog_append_file (self->priv->singletons_catalog, (GFile *) file_list->data);
}


static void
done_func (GError   *error,
	   gpointer  user_data)
{
	GthOrganizeTask *self = user_data;

	if (error != NULL) {
		gth_task_completed (GTH_TASK (self), error);
		return;
	}

	g_hash_table_foreach (self->priv->catalogs, save_catalog, self);

	if (! self->priv->create_singletons && (self->priv->singletons_catalog != NULL)) {
		GFile *gio_file;
		char  *data;
		gsize  size;

		g_hash_table_foreach (self->priv->catalogs, create_singletons_catalog, self);

		gio_file = gth_catalog_file_to_gio_file (gth_catalog_get_file (self->priv->singletons_catalog));
		data = gth_catalog_to_data (self->priv->singletons_catalog, &size);
		g_write_file (gio_file,
			      FALSE,
			      G_FILE_CREATE_NONE,
			      data,
			      size,
			      gth_task_get_cancellable (GTH_TASK (self)),
			      NULL);

		g_free (data);
		g_object_unref (gio_file);
	}

	gtk_widget_destroy (GET_WIDGET ("organize_files_dialog"));
	gth_task_completed (GTH_TASK (self), NULL);
}


static GFile *
get_catalog_file (const char *base_uri,
		  const char *display_name)
{
	GFile *base;
	char  *name;
	char  *name_escaped;
	GFile *catalog_file;

	base = g_file_new_for_uri (base_uri);
	name = g_strdup_printf ("%s.catalog", display_name);
	name_escaped = _g_utf8_replace (name, "/", ".");
	catalog_file = g_file_get_child_for_display_name (base, name_escaped, NULL);

	g_free (name_escaped);
	g_free (name);
	g_object_unref (base);

	return catalog_file;
}


static GFile *
get_catalog_file_for_time (GTimeVal *timeval)
{
	char  *year;
	char  *uri;
	GFile *base;
	char  *display_name;
	GFile *catalog_file;

	year = _g_time_val_strftime (timeval, "%Y");
	uri = g_strconcat ("catalog:///", year, "/", NULL);
	base = g_file_new_for_uri (uri);
	display_name = _g_time_val_strftime (timeval, "%m-%d");
	catalog_file = get_catalog_file (uri, display_name);

	g_free (display_name);
	g_object_unref (base);
	g_free (uri);
	g_free (year);

	return catalog_file;
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

			metadata = g_file_info_get_attribute_object (info, "Embedded::Image::DateTime");
			if (metadata != NULL) {
				if (_g_time_val_from_exif_date (gth_metadata_get_raw (GTH_METADATA (metadata)), &timeval))
					key = g_strdup (_g_time_val_strftime (&timeval, KEY_FORMAT));
			}
		}
		break;

	case GTH_GROUP_POLICY_MODIFIED_DATE:
		timeval = *gth_file_data_get_modification_time (file_data);
		key = g_strdup (_g_time_val_strftime (&timeval, KEY_FORMAT));
		break;
	}

	if (key == NULL)
		return;

	catalog = g_hash_table_lookup (self->priv->catalogs, key);
	if (catalog == NULL) {
		GthDateTime *date_time;
		char        *exif_date;
		GFile       *catalog_file;
		GtkTreeIter  iter;

		catalog = gth_catalog_new ();
		date_time = gth_datetime_new ();
		exif_date = _g_time_val_to_exif_date (&timeval);
		gth_datetime_from_exif_date (date_time, exif_date);
		gth_catalog_set_date (catalog, date_time);
		catalog_file = get_catalog_file_for_time (&timeval);
		gth_catalog_set_file (catalog, catalog_file);
		g_hash_table_insert (self->priv->catalogs, g_strdup (key), catalog);

		gtk_list_store_append (self->priv->results_liststore, &iter);
		gtk_list_store_set (self->priv->results_liststore, &iter,
				    NAME_COLUMN, key,
				    CARDINALITY_COLUMN, 0,
				    CREATE_CATALOG_COLUMN, TRUE,
				    -1);

		g_object_unref (catalog_file);
		g_free (exif_date);
		gth_datetime_free (date_time);
	}

	if (catalog != NULL)
		gth_catalog_append_file (catalog, file_data->file);

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

	gtk_list_store_clear (self->priv->results_liststore);

	switch (self->priv->group_policy) {
	case GTH_GROUP_POLICY_DIGITALIZED_DATE:
		attributes = "standard::name,standard::type,time::modified,time::modified-usec,Embedded::Image::DateTime";
		break;
	case GTH_GROUP_POLICY_MODIFIED_DATE:
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

	gth_task_dialog (base, TRUE);
	gtk_window_set_transient_for (GTK_WINDOW (GET_WIDGET ("organize_files_dialog")), GTK_WINDOW (self->priv->browser));
	gtk_window_set_modal (GTK_WINDOW (GET_WIDGET ("organize_files_dialog")), TRUE);
	gtk_widget_show (GET_WIDGET ("organize_files_dialog"));
}


static void
gth_organize_task_cancelled (GthTask *base)
{
	/* FIXME */
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
gth_organize_task_init (GthOrganizeTask *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_ORGANIZE_TASK, GthOrganizeTaskPrivate);
	self->priv->builder = _gtk_builder_new_from_file ("organize-files-task.ui", "catalogs");
	self->priv->results_liststore = (GtkListStore *) gtk_builder_get_object (self->priv->builder, "results_liststore");
	self->priv->catalogs = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);
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
	file = get_catalog_file ("catalog:///", catalog_name);
	gth_catalog_set_file (self->priv->singletons_catalog, file);

	g_object_unref (file);
}
