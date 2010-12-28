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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <glib/gi18n.h>
#include "glib-utils.h"
#include "gth-main.h"
#include "gth-overwrite-dialog.h"
#include "gth-pixbuf-list-task.h"
#include "gtk-utils.h"
#include "pixbuf-io.h"


struct _GthPixbufListTaskPrivate {
	GthBrowser           *browser;
	GList                *file_list;
	GthTask              *task;
	gulong                task_completed;
	gulong                task_progress;
	gulong                task_dialog;
	GList                *current;
	int                   n_current;
	int                   n_files;
	GdkPixbuf            *original_pixbuf;
	GdkPixbuf            *new_pixbuf;
	GFile                *destination_folder;
	GthFileData          *destination_file_data;
	GthOverwriteMode      overwrite_mode;
	GthOverwriteResponse  overwrite_response;
	char                 *mime_type;
};


static gpointer parent_class = NULL;


static void
gth_pixbuf_list_task_finalize (GObject *object)
{
	GthPixbufListTask *self;

	self = GTH_PIXBUF_LIST_TASK (object);

	g_free (self->priv->mime_type);
	_g_object_unref (self->priv->destination_folder);
	_g_object_unref (self->priv->original_pixbuf);
	_g_object_unref (self->priv->new_pixbuf);
	g_signal_handler_disconnect (self->priv->task, self->priv->task_completed);
	g_signal_handler_disconnect (self->priv->task, self->priv->task_progress);
	g_signal_handler_disconnect (self->priv->task, self->priv->task_dialog);
	g_object_unref (self->priv->task);
	_g_object_list_unref (self->priv->file_list);
	_g_object_unref (self->priv->destination_file_data);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void process_current_file (GthPixbufListTask *self);


static void
process_next_file (GthPixbufListTask *self)
{
	self->priv->n_current++;
	self->priv->current = self->priv->current->next;
	process_current_file (self);
}


static void pixbuf_task_save_current_pixbuf (GthPixbufListTask *self,
					     GFile             *file,
					     gboolean           replace);


static void
overwrite_dialog_response_cb (GtkDialog *dialog,
                              gint       response_id,
                              gpointer   user_data)
{
	GthPixbufListTask *self = user_data;
	gboolean           close_overwrite_dialog = TRUE;

	if (response_id != GTK_RESPONSE_OK)
		self->priv->overwrite_response = GTH_OVERWRITE_RESPONSE_CANCEL;
	else
		self->priv->overwrite_response = gth_overwrite_dialog_get_response (GTH_OVERWRITE_DIALOG (dialog));

	gtk_widget_hide (GTK_WIDGET (dialog));
	gth_task_dialog (GTH_TASK (self), FALSE, NULL);

	switch (self->priv->overwrite_response) {
	case GTH_OVERWRITE_RESPONSE_NO:
	case GTH_OVERWRITE_RESPONSE_ALWAYS_NO:
	case GTH_OVERWRITE_RESPONSE_UNSPECIFIED:
		if (self->priv->overwrite_response == GTH_OVERWRITE_RESPONSE_ALWAYS_NO)
			self->priv->overwrite_mode = GTH_OVERWRITE_SKIP;
		process_next_file (self);
		break;

	case GTH_OVERWRITE_RESPONSE_YES:
	case GTH_OVERWRITE_RESPONSE_ALWAYS_YES:
		if (self->priv->overwrite_response == GTH_OVERWRITE_RESPONSE_ALWAYS_YES)
			self->priv->overwrite_mode = GTH_OVERWRITE_OVERWRITE;
		pixbuf_task_save_current_pixbuf (self, NULL, TRUE);
		break;

	case GTH_OVERWRITE_RESPONSE_RENAME:
		{
			GFile  *parent;
			GFile  *new_destination;
			GError *error = NULL;

			if (self->priv->destination_folder != NULL)
				parent = g_object_ref (self->priv->destination_folder);
			else
				parent = g_file_get_parent (self->priv->destination_file_data->file);

			new_destination = g_file_get_child_for_display_name (parent, gth_overwrite_dialog_get_filename (GTH_OVERWRITE_DIALOG (dialog)), &error);
			if (new_destination == NULL) {
				_gtk_error_dialog_from_gerror_run (GTK_WINDOW (dialog), _("Could not rename the file"), &error);
				gtk_widget_show (GTK_WIDGET (dialog));
				gth_task_dialog (GTH_TASK (self), TRUE, GTK_WIDGET (dialog));
				close_overwrite_dialog = FALSE;
			}
			else
				pixbuf_task_save_current_pixbuf (self, new_destination, FALSE);

			g_object_unref (new_destination);
			g_object_unref (parent);
		}
		break;

	case GTH_OVERWRITE_RESPONSE_CANCEL:
		{
			GError *error;

			error = g_error_new_literal (GTH_TASK_ERROR, GTH_TASK_ERROR_CANCELLED, "");
			gth_task_completed (GTH_TASK (self), error);
		}
		break;
	}

	if (close_overwrite_dialog)
		gtk_widget_destroy (GTK_WIDGET (dialog));
}


static void
pixbuf_saved_cb (GthFileData *file_data,
		 GError      *error,
		 gpointer     user_data)
{
	GthPixbufListTask *self = user_data;
	GFile             *parent;
	GList             *file_list;

	if (error != NULL) {
		if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_EXISTS)) {
			if (self->priv->overwrite_mode == GTH_OVERWRITE_SKIP) {
				process_next_file (self);
			}
			else  {
				GtkWidget *dialog;

				dialog = gth_overwrite_dialog_new (NULL,
								   self->priv->new_pixbuf,
								   self->priv->destination_file_data->file,
								   GTH_OVERWRITE_RESPONSE_YES,
								   (self->priv->n_files == 1));
				gth_task_dialog (GTH_TASK (self), TRUE, dialog);

				g_signal_connect (dialog,
						  "response",
						  G_CALLBACK (overwrite_dialog_response_cb),
						  self);
				gtk_widget_show (dialog);
			}
		}
		else
			gth_task_completed (GTH_TASK (self), error);
		return;
	}

	parent = g_file_get_parent (file_data->file);
	file_list = g_list_append (NULL, file_data->file);
	gth_monitor_folder_changed (gth_main_get_default_monitor (),
				    parent,
				    file_list,
				    GTH_MONITOR_EVENT_CHANGED);

	g_list_free (file_list);
	g_object_unref (parent);

	process_next_file (self);
}


static void
pixbuf_task_dialog_cb (GthTask   *task,
		       gboolean   opened,
		       GtkWidget *dialog,
		       gpointer   user_data)
{
	gth_task_dialog (GTH_TASK (user_data), opened, dialog);
}


static void
pixbuf_task_progress_cb (GthTask    *task,
		         const char *description,
		         const char *details,
		         gboolean    pulse,
		         double      fraction,
		         gpointer    user_data)
{
	GthPixbufListTask *self = user_data;
	double             total_fraction;
	double             file_fraction;

	total_fraction =  ((double) self->priv->n_current + 1) / (self->priv->n_files + 1);
	if (pulse)
		file_fraction = 0.5;
	else
		file_fraction = fraction;

	if (details == NULL) {
		GthFileData *source_file_data = self->priv->current->data;
		details = g_file_info_get_display_name (source_file_data->info);
	}

	gth_task_progress (GTH_TASK (self),
			   description,
			   details,
			   FALSE,
			   total_fraction + (file_fraction / (self->priv->n_files + 1)));
}


static void
pixbuf_task_save_current_pixbuf (GthPixbufListTask *self,
				 GFile             *file,
				 gboolean           replace)
{
	if (file != NULL)
		gth_file_data_set_file (self->priv->destination_file_data, file);

	if (GTH_PIXBUF_TASK (self->priv->task)->dest == NULL) {
		process_next_file (self);
		return;
	}

	/* add a reference before unref-ing new_pixbuf because dest and
	 * new_pixbuf can be the same object. */

	g_object_ref (GTH_PIXBUF_TASK (self->priv->task)->dest);
	_g_object_unref (self->priv->new_pixbuf);
	self->priv->new_pixbuf = GTH_PIXBUF_TASK (self->priv->task)->dest;

	_gdk_pixbuf_save_async (self->priv->new_pixbuf,
				self->priv->destination_file_data,
				gth_file_data_get_mime_type (self->priv->destination_file_data),
				replace,
				pixbuf_saved_cb,
				self);
}


static void
set_current_destination_file (GthPixbufListTask *self)
{
	char  *display_name;
	GFile *parent;
	GFile *destination;

	_g_object_unref (self->priv->destination_file_data);
	self->priv->destination_file_data = g_object_ref (self->priv->current->data);

	if (self->priv->mime_type != NULL) {
		char           *no_ext;
		GthPixbufSaver *saver;

		no_ext = _g_uri_remove_extension (g_file_info_get_display_name (self->priv->destination_file_data->info));
		saver = gth_main_get_pixbuf_saver (self->priv->mime_type);
		g_return_if_fail (saver != NULL);

		display_name = g_strconcat (no_ext, ".", gth_pixbuf_saver_get_default_ext (saver), NULL);
		gth_file_data_set_mime_type (self->priv->destination_file_data, self->priv->mime_type);

		g_object_unref (saver);
		g_free (no_ext);
	}
	else
		display_name = g_strdup (g_file_info_get_display_name (self->priv->destination_file_data->info));

	if (self->priv->destination_folder != NULL)
		parent = g_object_ref (self->priv->destination_folder);
	else
		parent = g_file_get_parent (self->priv->destination_file_data->file);
	destination = g_file_get_child_for_display_name (parent, display_name, NULL);

	gth_file_data_set_file (self->priv->destination_file_data, destination);

	g_object_unref (destination);
	g_object_unref (parent);
	g_free (display_name);
}


static void
pixbuf_task_completed_cb (GthTask  *task,
			  GError   *error,
			  gpointer  user_data)
{
	GthPixbufListTask *self = user_data;

	if (g_error_matches (error, GTH_TASK_ERROR, GTH_TASK_ERROR_SKIP_TO_NEXT_FILE)) {
		process_next_file (self);
		return;
	}

	if (error != NULL) {
		gth_task_completed (GTH_TASK (self), error);
		return;
	}

	set_current_destination_file (self);
	pixbuf_task_save_current_pixbuf (self, NULL, (self->priv->overwrite_mode == GTH_OVERWRITE_OVERWRITE));
}


static void
file_buffer_ready_cb (void     **buffer,
		      gsize      count,
		      GError    *error,
		      gpointer   user_data)
{
	GthPixbufListTask *self = user_data;
	GInputStream      *istream;
	GdkPixbuf         *pixbuf;

	if (error != NULL) {
		gth_task_completed (GTH_TASK (self), error);
		return;
	}

	istream = g_memory_input_stream_new_from_data (*buffer, count, NULL);
	pixbuf = gdk_pixbuf_new_from_stream (istream, gth_task_get_cancellable (GTH_TASK (self)), &error);
	if (pixbuf != NULL) {
		self->priv->original_pixbuf = gdk_pixbuf_apply_embedded_orientation (pixbuf);
		g_object_unref (pixbuf);
	}
	else
		self->priv->original_pixbuf = NULL;

	g_object_unref (istream);

	if (self->priv->original_pixbuf == NULL) {
		gth_task_completed (GTH_TASK (self), error);
		return;
	}

	gth_pixbuf_task_set_source (GTH_PIXBUF_TASK (self->priv->task), self->priv->original_pixbuf);
	gth_task_exec (self->priv->task, gth_task_get_cancellable (GTH_TASK (self)));
}


static void
file_info_ready_cb (GList    *files,
		    GError   *error,
		    gpointer  user_data)
{
	GthPixbufListTask *self = user_data;
	GthFileData       *updated_file_data;
	GthFileData       *source_file_data;

	if (error != NULL) {
		gth_task_completed (GTH_TASK (self), error);
		return;
	}

	source_file_data = self->priv->current->data;
	updated_file_data = (GthFileData*) files->data;
	g_file_info_copy_into (updated_file_data->info, source_file_data->info);

	g_load_file_async (source_file_data->file,
			   G_PRIORITY_DEFAULT,
			   gth_task_get_cancellable (GTH_TASK (self)),
			   file_buffer_ready_cb,
			   self);
}


static void
process_current_file (GthPixbufListTask *self)
{
	GthFileData *source_file_data;
	GList       *source_singleton;

	if (self->priv->current == NULL) {
		if (self->priv->destination_folder != NULL)
			gth_browser_go_to (self->priv->browser, self->priv->destination_folder, NULL);
		gth_task_completed (GTH_TASK (self), NULL);
		return;
	}

	_g_object_unref (self->priv->original_pixbuf);
	self->priv->original_pixbuf = NULL;

	_g_object_unref (self->priv->new_pixbuf);
	self->priv->new_pixbuf = NULL;

	gth_task_progress (GTH_TASK (self),
			   NULL,
			   NULL,
			   FALSE,
			   ((double) self->priv->n_current + 1) / (self->priv->n_files + 1));

	source_file_data = self->priv->current->data;
	source_singleton = g_list_append (NULL, g_object_ref (source_file_data->file));
	_g_query_all_metadata_async (source_singleton,
				     GTH_LIST_DEFAULT,
				     "*",
				     gth_task_get_cancellable (GTH_TASK (self)),
				     file_info_ready_cb,
				     self);

	_g_object_list_unref (source_singleton);
}


static void
gth_pixbuf_list_task_exec (GthTask *task)
{
	GthPixbufListTask *self;

	g_return_if_fail (GTH_IS_PIXBUF_LIST_TASK (task));

	self = GTH_PIXBUF_LIST_TASK (task);

	self->priv->current = self->priv->file_list;
	self->priv->n_current = 0;
	self->priv->n_files = g_list_length (self->priv->file_list);
	process_current_file (self);
}


static void
gth_pixbuf_list_task_class_init (GthPixbufListTaskClass *klass)
{
	GObjectClass *object_class;
	GthTaskClass *task_class;

	parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GthPixbufListTaskPrivate));

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = gth_pixbuf_list_task_finalize;

	task_class = GTH_TASK_CLASS (klass);
	task_class->exec = gth_pixbuf_list_task_exec;
}


static void
gth_pixbuf_list_task_init (GthPixbufListTask *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_PIXBUF_LIST_TASK, GthPixbufListTaskPrivate);
	self->priv->original_pixbuf = NULL;
	self->priv->new_pixbuf = NULL;
	self->priv->destination_folder = NULL;
	self->priv->overwrite_response = GTH_OVERWRITE_RESPONSE_UNSPECIFIED;
	self->priv->mime_type = NULL;
}


GType
gth_pixbuf_list_task_get_type (void)
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (GthPixbufListTaskClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_pixbuf_list_task_class_init,
			NULL,
			NULL,
			sizeof (GthPixbufListTask),
			0,
			(GInstanceInitFunc) gth_pixbuf_list_task_init
		};

		type = g_type_register_static (GTH_TYPE_TASK,
					       "GthPixbufListTask",
					       &type_info,
					       0);
	}

	return type;
}


GthTask *
gth_pixbuf_list_task_new (GthBrowser    *browser,
			  GList         *file_list,
			  GthPixbufTask *task)
{
	GthPixbufListTask *self;

	g_return_val_if_fail (task != NULL, NULL);
	g_return_val_if_fail (GTH_IS_PIXBUF_TASK (task), NULL);

	self = GTH_PIXBUF_LIST_TASK (g_object_new (GTH_TYPE_PIXBUF_LIST_TASK, NULL));
	self->priv->browser = browser;
	self->priv->file_list = _g_object_list_ref (file_list);
	self->priv->task = g_object_ref (task);
	self->priv->task_completed = g_signal_connect (self->priv->task,
						       "completed",
						       G_CALLBACK (pixbuf_task_completed_cb),
						       self);
	self->priv->task_progress = g_signal_connect (self->priv->task,
						      "progress",
						      G_CALLBACK (pixbuf_task_progress_cb),
						      self);
	self->priv->task_dialog = g_signal_connect (self->priv->task,
						    "dialog",
						    G_CALLBACK (pixbuf_task_dialog_cb),
						    self);

	return (GthTask *) self;
}


void
gth_pixbuf_list_task_set_destination (GthPixbufListTask *self,
				      GFile             *folder)
{
	_g_object_unref (self->priv->destination_folder);
	self->priv->destination_folder = _g_object_ref (folder);
}


void
gth_pixbuf_list_task_set_overwrite_mode (GthPixbufListTask    *self,
					 GthOverwriteMode      overwrite_mode)
{
	self->priv->overwrite_mode = overwrite_mode;
}


void
gth_pixbuf_list_task_set_output_mime_type (GthPixbufListTask *self,
					   const char        *mime_type)
{
	g_free (self->priv->mime_type);
	self->priv->mime_type = NULL;
	if (mime_type != NULL)
		self->priv->mime_type = g_strdup (mime_type);
}
