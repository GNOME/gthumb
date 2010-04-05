/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2010 The Free Software Foundation, Inc.
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
#include <gtk/gtk.h>
#include <gthumb.h>
#include "dlg-export-to-flickr.h"
#include "flickr-authentication.h"
#include "flickr-photoset.h"
#include "flickr-service.h"
#include "flickr-user.h"


#define GET_WIDGET(x) (_gtk_builder_get_widget (data->builder, (x)))
#define _OPEN_IN_BROWSER_RESPONSE 1


enum {
	ACCOUNT_DATA_COLUMN,
	ACCOUNT_NAME_COLUMN
};


enum {
	PHOTOSET_DATA_COLUMN,
	PHOTOSET_ICON_COLUMN,
	PHOTOSET_TITLE_COLUMN,
	PHOTOSET_N_PHOTOS_COLUMN
};


typedef struct {
	FlickrServer         *server;
	GthBrowser           *browser;
	GthFileData          *location;
	GList                *file_list;
	GtkBuilder           *builder;
	GtkWidget            *dialog;
	GtkWidget            *progress_dialog;
	FlickrConnection     *conn;
	FlickrAuthentication *auth;
	FlickrService        *service;
	FlickrUser           *user;
	GList                *photosets;
	FlickrPhotoset       *photoset;
	GList                *photos_ids;
	GCancellable         *cancellable;
} DialogData;


static void
export_dialog_destroy_cb (GtkWidget  *widget,
			  DialogData *data)
{
	if (data->conn != NULL)
		gth_task_completed (GTH_TASK (data->conn), NULL);
	_g_object_unref (data->cancellable);
	_g_string_list_free (data->photos_ids);
	_g_object_unref (data->photoset);
	_g_object_list_unref (data->photosets);
	_g_object_unref (data->user);
	_g_object_unref (data->service);
	_g_object_unref (data->auth);
	_g_object_unref (data->conn);
	_g_object_unref (data->builder);
	_g_object_list_unref (data->file_list);
	_g_object_unref (data->location);
	g_free (data);
}


static void
completed_messagedialog_response_cb (GtkDialog *dialog,
				     int        response_id,
				     gpointer   user_data)
{
	DialogData *data = user_data;

	switch (response_id) {
	case GTK_RESPONSE_DELETE_EVENT:
	case GTK_RESPONSE_CLOSE:
		gtk_widget_destroy (GTK_WIDGET (dialog));
		gtk_widget_destroy (data->dialog);
		break;

	case _OPEN_IN_BROWSER_RESPONSE:
		{
			char   *url = NULL;
			GError *error = NULL;

			gtk_widget_destroy (GTK_WIDGET (dialog));

			if (data->photoset == NULL) {
				GString *ids;
				GList   *scan;

				ids = g_string_new ("");
				for (scan = data->photos_ids; scan; scan = scan->next) {
					if (scan != data->photos_ids)
						g_string_append (ids, ",");
					g_string_append (ids, (char *) scan->data);
				}
				url = g_strconcat (data->server->url, "/photos/upload/edit/?ids=", ids->str, NULL);

				g_string_free (ids, TRUE);
			}
			else if (data->photoset->url != NULL)
				url = g_strdup (data->photoset->url);
			else if (data->photoset->id != NULL)
				url = g_strconcat (data->server->url, "/photos/", data->user->id, "/sets/", data->photoset->id, NULL);

			if ((url != NULL) && ! gtk_show_uri (gtk_widget_get_screen (GTK_WIDGET (dialog)), url, 0, &error)) {
				if (data->conn != NULL)
					gth_task_dialog (GTH_TASK (data->conn), TRUE);
				_gtk_error_dialog_from_gerror_run (GTK_WINDOW (data->browser), _("Could not connect to the server"), &error);
			}
			gtk_widget_destroy (data->dialog);

			g_free (url);
		}
		break;

	default:
		break;
	}
}


static void
export_completed_with_success (DialogData *data)
{
	GtkBuilder *builder;
	GtkWidget  *dialog;

	gth_task_dialog (GTH_TASK (data->conn), TRUE);

	builder = _gtk_builder_new_from_file ("flicker-export-completed.ui", "flicker");
	dialog = _gtk_builder_get_widget (builder, "completed_messagedialog");
	g_object_set_data_full (G_OBJECT (dialog), "builder", builder, g_object_unref);
	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (completed_messagedialog_response_cb),
			  data);

	gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (data->browser));
	gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
	gtk_window_present (GTK_WINDOW (dialog));
}


static void
add_photos_to_photoset_ready_cb (GObject      *source_object,
				 GAsyncResult *result,
				 gpointer      user_data)
{
	DialogData *data = user_data;
	GError     *error = NULL;

	if (! flickr_service_add_photos_to_set_finish (FLICKR_SERVICE (source_object), result, &error)) {
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (data->browser), _("Could not create the album"), &error);
		gtk_widget_destroy (data->dialog);
		return;
	}

	export_completed_with_success (data);
}


static void
add_photos_to_photoset (DialogData *data)
{
	flickr_service_add_photos_to_set (data->service,
					  data->photoset,
					  data->photos_ids,
					  data->cancellable,
					  add_photos_to_photoset_ready_cb,
					  data);
}


static void
create_photoset_ready_cb (GObject      *source_object,
			  GAsyncResult *result,
			  gpointer      user_data)
{
	DialogData *data = user_data;
	GError     *error = NULL;
	char       *primary;

	primary = g_strdup (data->photoset->primary);
	g_object_unref (data->photoset);
	data->photoset = flickr_service_create_photoset_finish (FLICKR_SERVICE (source_object), result, &error);
	if (error != NULL) {
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (data->browser), _("Could not create the album"), &error);
		gtk_widget_destroy (data->dialog);
	}
	else {
		flickr_photoset_set_primary (data->photoset, primary);
		add_photos_to_photoset (data);
	}

	g_free (primary);
}


static void
post_photos_ready_cb (GObject      *source_object,
		      GAsyncResult *result,
		      gpointer      user_data)
{
	DialogData *data = user_data;
	GError     *error = NULL;

	data->photos_ids = flickr_service_post_photos_finish (FLICKR_SERVICE (source_object), result, &error);
	if (error != NULL) {
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (data->browser), _("Could not upload the files"), &error);
		gtk_widget_destroy (data->dialog);
		return;
	}

	if (data->photoset == NULL) {
		export_completed_with_success (data);
		return;
	}

	/* create the photoset if it doesn't exists */

	if (data->photoset->id == NULL) {
		char *first_id;

		first_id = data->photos_ids->data;
		flickr_photoset_set_primary (data->photoset, first_id);
		flickr_service_create_photoset (data->service,
						data->photoset,
						data->cancellable,
						create_photoset_ready_cb,
						data);
	}
	else
		add_photos_to_photoset (data);
}


static int
find_photoset_by_title (FlickrPhotoset *photoset,
		        const char     *name)
{
	return g_strcmp0 (photoset->title, name);
}


static void
export_dialog_response_cb (GtkDialog *dialog,
			   int        response_id,
			   gpointer   user_data)
{
	DialogData *data = user_data;

	switch (response_id) {
	case GTK_RESPONSE_HELP:
		show_help_dialog (GTK_WINDOW (data->browser), "export-to-flickr");
		break;

	case GTK_RESPONSE_DELETE_EVENT:
	case GTK_RESPONSE_CANCEL:
		gtk_widget_destroy (data->dialog);
		break;

	case GTK_RESPONSE_OK:
		{
			char  *photoset_title;
			GList *file_list;

			gtk_widget_hide (data->dialog);
			gth_task_dialog (GTH_TASK (data->conn), FALSE);

			data->photoset = NULL;
			photoset_title = gtk_combo_box_get_active_text (GTK_COMBO_BOX (GET_WIDGET ("photoset_comboboxentry")));
			if ((photoset_title != NULL) && (g_strcmp0 (photoset_title, "") != 0)) {
				GList *link;

				link = g_list_find_custom (data->photosets, photoset_title, (GCompareFunc) find_photoset_by_title);
				if (link != NULL)
					data->photoset = g_object_ref (link->data);

				if (data->photoset == NULL) {
					data->photoset = flickr_photoset_new ();
					flickr_photoset_set_title (data->photoset, photoset_title);
				}
			}

			file_list = gth_file_data_list_to_file_list (data->file_list);
			flickr_service_post_photos (data->service,
						    gtk_combo_box_get_active (GTK_COMBO_BOX (GET_WIDGET ("privacy_combobox"))),
						    gtk_combo_box_get_active (GTK_COMBO_BOX (GET_WIDGET ("safety_combobox"))),
						    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("hidden_checkbutton"))),
						    file_list,
						    data->cancellable,
						    post_photos_ready_cb,
						    data);

			_g_object_list_unref (file_list);
			g_free (photoset_title);
		}
		break;

	default:
		break;
	}
}


static void
update_account_list (DialogData *data)
{
	int            current_account_idx;
	FlickrAccount *current_account;
	int            idx;
	GList         *scan;
	GtkTreeIter    iter;
	char          *free_space;

	gtk_list_store_clear (GTK_LIST_STORE (GET_WIDGET ("account_liststore")));

	current_account_idx = 0;
	current_account = flickr_authentication_get_account (data->auth);
	for (scan = flickr_authentication_get_accounts (data->auth), idx = 0; scan; scan = scan->next, idx++) {
		FlickrAccount *account = scan->data;

		if ((current_account != NULL) && (g_strcmp0 (current_account->username, account->username) == 0))
			current_account_idx = idx;

		gtk_list_store_append (GTK_LIST_STORE (GET_WIDGET ("account_liststore")), &iter);
		gtk_list_store_set (GTK_LIST_STORE (GET_WIDGET ("account_liststore")), &iter,
				    ACCOUNT_DATA_COLUMN, account,
				    ACCOUNT_NAME_COLUMN, account->username,
				    -1);
	}
	gtk_combo_box_set_active (GTK_COMBO_BOX (GET_WIDGET ("account_combobox")), current_account_idx);

	free_space = g_format_size_for_display (data->user->max_bandwidth - data->user->used_bandwidth);
	gtk_label_set_text (GTK_LABEL (GET_WIDGET ("free_space_label")), free_space);
	g_free (free_space);
}


static void
authentication_accounts_changed_cb (FlickrAuthentication *auth,
				    gpointer              user_data)
{
	update_account_list ((DialogData *) user_data);
}


static void
photoset_list_ready_cb (GObject      *source_object,
			GAsyncResult *res,
			gpointer      user_data)
{
	DialogData *data = user_data;
	GError     *error = NULL;
	GList      *scan;

	_g_object_list_unref (data->photosets);
	data->photosets = flickr_service_list_photosets_finish (FLICKR_SERVICE (source_object), res, &error);
	if (error != NULL) {
		if (data->conn != NULL)
			gth_task_dialog (GTH_TASK (data->conn), TRUE);
		_gtk_error_dialog_from_gerror_run (GTK_WINDOW (data->browser), _("Could not connect to the server"), &error);
		gtk_widget_destroy (data->dialog);
		return;
	}

	gtk_list_store_clear (GTK_LIST_STORE (GET_WIDGET ("photoset_liststore")));
	for (scan = data->photosets; scan; scan = scan->next) {
		FlickrPhotoset *photoset = scan->data;
		char           *n_photos;
		GtkTreeIter     iter;

		n_photos = g_strdup_printf ("(%d)", photoset->n_photos);

		gtk_list_store_append (GTK_LIST_STORE (GET_WIDGET ("photoset_liststore")), &iter);
		gtk_list_store_set (GTK_LIST_STORE (GET_WIDGET ("photoset_liststore")), &iter,
				    PHOTOSET_DATA_COLUMN, photoset,
				    PHOTOSET_ICON_COLUMN, "file-catalog",
				    PHOTOSET_TITLE_COLUMN, photoset->title,
				    PHOTOSET_N_PHOTOS_COLUMN, n_photos,
				    -1);

		g_free (n_photos);
	}

	gtk_widget_set_sensitive (GET_WIDGET ("upload_button"), TRUE);

	gth_task_dialog (GTH_TASK (data->conn), TRUE);

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), GTK_WINDOW (data->browser));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), FALSE);
	gtk_window_present (GTK_WINDOW (data->dialog));
}


static void
authentication_ready_cb (FlickrAuthentication *auth,
			 FlickrUser           *user,
			 DialogData           *data)
{
	_g_object_unref (data->user);
	data->user = g_object_ref (user);
	update_account_list (data);

	flickr_service_list_photosets (data->service,
				       NULL,
				       data->cancellable,
				       photoset_list_ready_cb,
				       data);
}


static void
edit_accounts_button_clicked_cb (GtkButton  *button,
				 DialogData *data)
{
	flickr_authentication_edit_accounts (data->auth, GTK_WINDOW (data->dialog));
}


static void
account_combobox_changed_cb (GtkComboBox *widget,
			     gpointer     user_data)
{
	DialogData    *data = user_data;
	GtkTreeIter    iter;
	FlickrAccount *account;

	if (! gtk_combo_box_get_active_iter (widget, &iter))
		return;

	gtk_tree_model_get (gtk_combo_box_get_model (widget),
			    &iter,
			    ACCOUNT_DATA_COLUMN, &account,
			    -1);

	if (flickr_account_cmp (account, flickr_authentication_get_account (data->auth)) != 0)
		flickr_authentication_connect (data->auth, account);

	g_object_unref (account);
}


void
dlg_export_to_flickr (FlickrServer *server,
		      GthBrowser   *browser,
		      GList        *file_list)
{
	DialogData *data;
	GList      *scan;
	int         n_total;
	goffset     total_size;
	char       *total_size_formatted;
	char       *text;
	GtkWidget  *list_view;

	data = g_new0 (DialogData, 1);
	data->server = server;
	data->browser = browser;
	data->location = gth_file_data_dup (gth_browser_get_location_data (browser));
	data->builder = _gtk_builder_new_from_file ("export-to-flickr.ui", "flicker");
	data->dialog = _gtk_builder_get_widget (data->builder, "export_dialog");
	data->cancellable = g_cancellable_new ();

	data->file_list = NULL;
	n_total = 0;
	total_size = 0;
	for (scan = file_list; scan; scan = scan->next) {
		GthFileData *file_data = scan->data;
		const char  *mime_type;

		mime_type = gth_file_data_get_mime_type (file_data);
		if (g_content_type_equals (mime_type, "image/bmp")
		    || g_content_type_equals (mime_type, "image/gif")
		    || g_content_type_equals (mime_type, "image/jpeg")
		    || g_content_type_equals (mime_type, "image/png"))
		{
			total_size += g_file_info_get_size (file_data->info);
			n_total++;
			data->file_list = g_list_prepend (data->file_list, g_object_ref (file_data));
		}
	}
	data->file_list = g_list_reverse (data->file_list);

	if (data->file_list == NULL) {
		GError *error;

		error = g_error_new_literal (GTH_ERROR, GTH_ERROR_GENERIC, _("No valid file selected."));
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (browser), _("Could not export the files"), &error);
		gtk_widget_destroy (data->dialog);

		return;
	}

	total_size_formatted = g_format_size_for_display (total_size);
	text = g_strdup_printf (g_dngettext (NULL, "%d file (%s)", "%d files (%s)", n_total), n_total, total_size_formatted);
	gtk_label_set_text (GTK_LABEL (GET_WIDGET ("images_info_label")), text);
	g_free (text);
	g_free (total_size_formatted);

	/* Set the widget data */

	list_view = gth_file_list_new (GTH_FILE_LIST_TYPE_NO_SELECTION);
	gth_file_list_set_thumb_size (GTH_FILE_LIST (list_view), 112);
	gth_file_view_set_spacing (GTH_FILE_VIEW (gth_file_list_get_view (GTH_FILE_LIST (list_view))), 0);
	gth_file_list_enable_thumbs (GTH_FILE_LIST (list_view), TRUE);
	gth_file_list_set_caption (GTH_FILE_LIST (list_view), "none");
	gth_file_list_set_sort_func (GTH_FILE_LIST (list_view), gth_main_get_sort_type ("file::name")->cmp_func, FALSE);
	gtk_widget_show (list_view);
	gtk_box_pack_start (GTK_BOX (GET_WIDGET ("images_box")), list_view, TRUE, TRUE, 0);
	gth_file_list_set_files (GTH_FILE_LIST (list_view), data->file_list);

	gtk_entry_set_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (GET_WIDGET ("photoset_comboboxentry")))), g_file_info_get_edit_name (data->location->info));
	gtk_widget_set_sensitive (GET_WIDGET ("upload_button"), FALSE);

	/* Set the signals handlers. */

	g_signal_connect (G_OBJECT (data->dialog),
			  "destroy",
			  G_CALLBACK (export_dialog_destroy_cb),
			  data);
	g_signal_connect (data->dialog,
			  "response",
			  G_CALLBACK (export_dialog_response_cb),
			  data);
	g_signal_connect (GET_WIDGET ("edit_accounts_button"),
			  "clicked",
			  G_CALLBACK (edit_accounts_button_clicked_cb),
			  data);
	g_signal_connect (GET_WIDGET ("account_combobox"),
			  "changed",
			  G_CALLBACK (account_combobox_changed_cb),
			  data);

	data->conn = flickr_connection_new (data->server);
	data->service = flickr_service_new (data->conn);
	data->auth = flickr_authentication_new (data->conn,
						data->service,
						data->cancellable,
						GTK_WIDGET (data->browser),
						data->dialog);
	g_signal_connect (data->auth,
			  "ready",
			  G_CALLBACK (authentication_ready_cb),
			  data);
	g_signal_connect (data->auth,
			  "accounts_changed",
			  G_CALLBACK (authentication_accounts_changed_cb),
			  data);

	data->progress_dialog = gth_progress_dialog_new (GTK_WINDOW (data->browser));
	gth_progress_dialog_add_task (GTH_PROGRESS_DIALOG (data->progress_dialog), GTH_TASK (data->conn));

	flickr_authentication_auto_connect (data->auth);
}
