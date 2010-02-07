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
#include <extensions/catalogs/gth-catalog.h>
#include "gth-search-task.h"


struct _GthSearchTaskPrivate
{
	GthBrowser    *browser;
	GthSearch     *search;
	GthTestChain  *test;
	GFile         *search_catalog;
	gboolean       io_operation;
	GError        *error;
	gulong         location_ready_id;
	GtkWidget     *dialog;
	GthFileSource *file_source;
};


static gpointer parent_class = NULL;


static void
browser_unref_cb (gpointer  data,
		  GObject  *browser)
{
	((GthSearchTask *) data)->priv->browser = NULL;
}


static void
gth_task_finalize (GObject *object)
{
	GthSearchTask *task;

	task = GTH_SEARCH_TASK (object);

	if (task->priv != NULL) {
		g_object_unref (task->priv->file_source);
		g_object_unref (task->priv->search);
		g_object_unref (task->priv->test);
		g_object_unref (task->priv->search_catalog);
		if (task->priv->browser != NULL)
			g_object_weak_unref (G_OBJECT (task->priv->browser), browser_unref_cb, task);
		g_free (task->priv);
		task->priv = NULL;
	}

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


typedef struct {
	GthBrowser    *browser;
	GthSearchTask *task;
} EmbeddedDialogData;


static void
embedded_dialog_destroy_cb (GeditMessageArea *message_area,
			    gpointer          user_data)
{
	g_free ((EmbeddedDialogData *) user_data);
}


static void
embedded_dialog_response_cb (GeditMessageArea *message_area,
			     int               response_id,
			     gpointer          user_data)
{
	EmbeddedDialogData *data = user_data;

	switch (response_id) {
	case GTK_RESPONSE_CANCEL:
		gth_task_cancel (GTH_TASK (data->task));
		break;
	default:
		break;
	}
}


static void
save_search_result_copy_done_cb (void     **buffer,
				 gsize      count,
				 GError    *error,
				 gpointer   user_data)
{
	GthSearchTask *task = user_data;

	gth_browser_update_extra_widget (task->priv->browser);

	task->priv->io_operation = FALSE;
	gth_task_completed (GTH_TASK (task), task->priv->error);
}


static void
done_func (GObject  *object,
	   GError   *error,
	   gpointer  user_data)
{
	GthSearchTask *task = user_data;
	DomDocument   *doc;
	char          *data;
	gsize          size;
	GFile         *search_result_real_file;

	task->priv->error = NULL;
	if (error != NULL) {
		if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
			task->priv->error = g_error_new_literal (GTH_TASK_ERROR, GTH_TASK_ERROR_CANCELLED, "");
			g_error_free (error);
		}
		else
			task->priv->error = error;
	}

	gth_monitor_folder_changed (gth_main_get_default_monitor (),
				    task->priv->search_catalog,
				    gth_catalog_get_file_list (GTH_CATALOG (task->priv->search)),
				    GTH_MONITOR_EVENT_CREATED);

	/* save the search result */

	doc = dom_document_new ();
	dom_element_append_child (DOM_ELEMENT (doc), dom_domizable_create_element (DOM_DOMIZABLE (task->priv->search), doc));
	data = dom_document_dump (doc, &size);

	search_result_real_file = gth_catalog_file_to_gio_file (task->priv->search_catalog);
	g_write_file_async (search_result_real_file,
			    data,
			    size,
			    TRUE,
			    G_PRIORITY_DEFAULT,
			    gth_task_get_cancellable (GTH_TASK (task)),
			    save_search_result_copy_done_cb,
			    task);

	g_object_unref (search_result_real_file);
	g_object_unref (doc);
}


static void
for_each_file_func (GFile     *file,
		    GFileInfo *info,
		    gpointer   user_data)
{
	GthSearchTask *task = user_data;
	GthFileData   *file_data;

	if (g_file_info_get_file_type (info) != G_FILE_TYPE_REGULAR)
		return;

	file_data = gth_file_data_new (file, info);

	if (gth_test_match (GTH_TEST (task->priv->test), file_data))
		gth_catalog_insert_file (GTH_CATALOG (task->priv->search), file_data->file, -1);

	g_object_unref (file_data);
}


static DirOp
start_dir_func (GFile      *directory,
		GFileInfo  *info,
		GError    **error,
		gpointer    user_data)
{
	GthSearchTask *task = user_data;
	char          *uri;
	char          *text;

	uri = g_file_get_parse_name (directory);
	text = g_strdup_printf ("Searching in %s", uri);
	gth_embedded_dialog_set_primary_text (GTH_EMBEDDED_DIALOG (task->priv->dialog), text);

	g_free (text);
	g_free (uri);

	return DIR_OP_CONTINUE;
}


static void
browser_location_ready_cb (GthBrowser    *browser,
			   GFile         *folder,
			   gboolean       error,
			   GthSearchTask *task)
{
	EmbeddedDialogData *dialog_data;

	g_signal_handler_disconnect (task->priv->browser, task->priv->location_ready_id);

	if (error) {
		gth_task_completed (GTH_TASK (task), NULL);
		return;
	}

	task->priv->dialog = gth_browser_get_list_extra_widget (browser);
	gth_embedded_dialog_set_icon (GTH_EMBEDDED_DIALOG (task->priv->dialog), GTK_STOCK_FIND);
	gth_embedded_dialog_set_primary_text (GTH_EMBEDDED_DIALOG (task->priv->dialog), _("Searching..."));
	gedit_message_area_clear_action_area (GEDIT_MESSAGE_AREA (task->priv->dialog));
	gedit_message_area_add_stock_button_with_text (GEDIT_MESSAGE_AREA (task->priv->dialog),
						       NULL,
						       GTK_STOCK_CANCEL,
						       GTK_RESPONSE_CANCEL);

	dialog_data = g_new0 (EmbeddedDialogData, 1);
	dialog_data->browser = task->priv->browser;
	dialog_data->task = task;
	g_signal_connect (task->priv->dialog,
			  "destroy",
			  G_CALLBACK (embedded_dialog_destroy_cb),
			  dialog_data);
	g_signal_connect (task->priv->dialog,
			  "response",
			  G_CALLBACK (embedded_dialog_response_cb),
			  dialog_data);

	/**/

	if (gth_search_get_test (task->priv->search) != NULL)
		task->priv->test = (GthTestChain*) gth_duplicable_duplicate (GTH_DUPLICABLE (gth_search_get_test (task->priv->search)));
	else
		task->priv->test = (GthTestChain*) gth_test_chain_new (GTH_MATCH_TYPE_ALL, NULL);

	if (! gth_test_chain_has_type_test (task->priv->test)) {
		GthTest *general_filter;

		general_filter = gth_main_get_general_filter ();
		gth_test_chain_add_test (task->priv->test, general_filter);

		g_object_unref (general_filter);
	}

	task->priv->io_operation = TRUE;

	task->priv->file_source = gth_main_get_file_source (gth_search_get_folder (task->priv->search));
	gth_file_source_set_cancellable (task->priv->file_source, gth_task_get_cancellable (GTH_TASK (task)));
	gth_file_source_for_each_child (task->priv->file_source,
					gth_search_get_folder (task->priv->search),
					gth_search_is_recursive (task->priv->search),
					eel_gconf_get_boolean (PREF_FAST_FILE_TYPE, TRUE) ? GFILE_STANDARD_ATTRIBUTES_WITH_FAST_CONTENT_TYPE : GFILE_STANDARD_ATTRIBUTES_WITH_CONTENT_TYPE,
					start_dir_func,
					for_each_file_func,
					done_func,
					task);
}


static void
clear_search_result_copy_done_cb (void     **buffer,
				  gsize      count,
				  GError    *error,
				  gpointer   user_data)
{
	GthSearchTask *task = user_data;

	if (error != NULL) {
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (task->priv->browser), _("Could not create the catalog"), &error);
		return;
	}

	task->priv->location_ready_id = g_signal_connect (task->priv->browser,
							  "location-ready",
							  G_CALLBACK (browser_location_ready_cb),
							  task);
	gth_browser_go_to (task->priv->browser, task->priv->search_catalog, NULL);
}


static void
gth_search_task_exec (GthTask *base)
{
	GthSearchTask *task = (GthSearchTask *) base;
	DomDocument   *doc;
	char          *data;
	gsize          size;
	GFile         *search_result_real_file;

	gth_catalog_set_file_list (GTH_CATALOG (task->priv->search), NULL);
	task->priv->io_operation = FALSE;

	/* save the search result */

	doc = dom_document_new ();
	dom_element_append_child (DOM_ELEMENT (doc), dom_domizable_create_element (DOM_DOMIZABLE (task->priv->search), doc));
	data = dom_document_dump (doc, &size);

	search_result_real_file = gth_catalog_file_to_gio_file (task->priv->search_catalog);
	g_write_file_async (search_result_real_file,
			    data,
			    size,
			    TRUE,
			    G_PRIORITY_DEFAULT,
			    gth_task_get_cancellable (GTH_TASK (task)),
			    clear_search_result_copy_done_cb,
			    task);

	g_object_unref (search_result_real_file);
	g_object_unref (doc);
}


static void
gth_search_task_cancelled (GthTask *task)
{
	if (! GTH_SEARCH_TASK (task)->priv->io_operation)
		gth_task_completed (task, g_error_new_literal (GTH_TASK_ERROR, GTH_TASK_ERROR_CANCELLED, ""));
}


static void
gth_search_task_class_init (GthSearchTaskClass *class)
{
	GObjectClass *object_class;
	GthTaskClass *task_class;

	parent_class = g_type_class_peek_parent (class);

	object_class = (GObjectClass*) class;
	object_class->finalize = gth_task_finalize;

	task_class = (GthTaskClass*) class;
	task_class->exec = gth_search_task_exec;
	task_class->cancelled = gth_search_task_cancelled;
}


static void
gth_search_task_init (GthSearchTask *task)
{
	task->priv = g_new0 (GthSearchTaskPrivate, 1);
}


GType
gth_search_task_get_type (void)
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (GthSearchTaskClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_search_task_class_init,
			NULL,
			NULL,
			sizeof (GthSearchTask),
			0,
			(GInstanceInitFunc) gth_search_task_init
		};

		type = g_type_register_static (GTH_TYPE_TASK,
					       "GthSearchTask",
					       &type_info,
					       0);
	}

	return type;
}


GthTask *
gth_search_task_new (GthBrowser *browser,
		     GthSearch  *search,
		     GFile      *search_catalog)
{
	GthSearchTask *task;

	task = (GthSearchTask *) g_object_new (GTH_TYPE_SEARCH_TASK, NULL);

	task->priv->browser = browser;
	g_object_weak_ref (G_OBJECT (task->priv->browser), browser_unref_cb, task);

	task->priv->search = g_object_ref (search);
	task->priv->search_catalog = g_object_ref (search_catalog);

	return (GthTask*) task;
}
