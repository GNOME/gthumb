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
#include "dlg-export-to-facebook.h"
#include "facebook-account-chooser-dialog.h"
#include "facebook-account-manager-dialog.h"
#include "facebook-album.h"
#include "facebook-service.h"
#include "facebook-user.h"


#define GET_WIDGET(x) (_gtk_builder_get_widget (data->builder, (x)))
#define _OPEN_IN_BROWSER_RESPONSE 1


enum {
	ACCOUNT_DATA_COLUMN,
	ACCOUNT_NAME_COLUMN
};


enum {
	ALBUM_DATA_COLUMN,
	ALBUM_ICON_COLUMN,
	ALBUM_TITLE_COLUMN,
	ALBUM_N_PHOTOS_COLUMN
};


typedef struct {
	GthBrowser       *browser;
	GthFileData      *location;
	GList            *file_list;
	GtkBuilder       *builder;
	GtkWidget        *dialog;
	GtkWidget        *progress_dialog;
	GList            *accounts;
	FacebookAccount    *account;
	FacebookUser       *user;
	GList            *albums;
	FacebookPhotoset   *album;
	FacebookConnection *conn;
	FacebookService    *service;
	GList            *photos_ids;
	GCancellable     *cancellable;
} DialogData;


static void
export_dialog_destroy_cb (GtkWidget  *widget,
			  DialogData *data)
{
	if (data->conn != NULL)
		gth_task_completed (GTH_TASK (data->conn), NULL);
	_g_object_unref (data->cancellable);
	_g_object_unref (data->service);
	_g_object_unref (data->conn);
	_g_object_list_unref (data->accounts);
	_g_object_unref (data->account);
	_g_object_unref (data->user);
	_g_object_list_unref (data->albums);
	_g_object_unref (data->album);
	_g_string_list_free (data->photos_ids);
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

			if (data->album == NULL) {
				GString *ids;
				GList   *scan;

				ids = g_string_new ("");
				for (scan = data->photos_ids; scan; scan = scan->next) {
					if (scan != data->photos_ids)
						g_string_append (ids, ",");
					g_string_append (ids, (char *) scan->data);
				}
				url = g_strconcat ("http://www.facebook.com/photos/upload/edit/?ids=", ids->str, NULL);

				g_string_free (ids, TRUE);
			}
			else if (data->album->url != NULL)
				url = g_strdup (data->album->url);
			else if (data->album->id != NULL)
				url = g_strconcat ("http://www.facebook.com/photos/", data->user->id, "/sets/", data->album->id, NULL);

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
add_photos_to_album_ready_cb (GObject      *source_object,
				 GAsyncResult *result,
				 gpointer      user_data)
{
	DialogData *data = user_data;
	GError     *error = NULL;

	if (! facebook_service_add_photos_to_set_finish (FACEBOOK_SERVICE (source_object), result, &error)) {
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (data->browser), _("Could not create the album"), &error);
		gtk_widget_destroy (data->dialog);
		return;
	}

	export_completed_with_success (data);
}


static void
add_photos_to_album (DialogData *data)
{
	facebook_service_add_photos_to_set (data->service,
					  data->album,
					  data->photos_ids,
					  data->cancellable,
					  add_photos_to_album_ready_cb,
					  data);
}


static void
create_album_ready_cb (GObject      *source_object,
			  GAsyncResult *result,
			  gpointer      user_data)
{
	DialogData *data = user_data;
	GError     *error = NULL;
	char       *primary;

	primary = g_strdup (data->album->primary);
	g_object_unref (data->album);
	data->album = facebook_service_create_album_finish (FACEBOOK_SERVICE (source_object), result, &error);
	if (error != NULL) {
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (data->browser), _("Could not create the album"), &error);
		gtk_widget_destroy (data->dialog);
	}
	else {
		facebook_album_set_primary (data->album, primary);
		add_photos_to_album (data);
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

	data->photos_ids = facebook_service_post_photos_finish (FACEBOOK_SERVICE (source_object), result, &error);
	if (error != NULL) {
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (data->browser), _("Could not upload the files"), &error);
		gtk_widget_destroy (data->dialog);
		return;
	}

	if (data->album == NULL) {
		export_completed_with_success (data);
		return;
	}

	/* create the album if it doesn't exists */

	if (data->album->id == NULL) {
		char *first_id;

		first_id = data->photos_ids->data;
		facebook_album_set_primary (data->album, first_id);
		facebook_service_create_album (data->service,
						data->album,
						data->cancellable,
						create_album_ready_cb,
						data);
	}
	else
		add_photos_to_album (data);
}


static int
find_album_by_title (FacebookPhotoset *album,
		        const char     *name)
{
	return g_strcmp0 (album->title, name);
}


static void
export_dialog_response_cb (GtkDialog *dialog,
			   int        response_id,
			   gpointer   user_data)
{
	DialogData *data = user_data;

	switch (response_id) {
	case GTK_RESPONSE_HELP:
		show_help_dialog (GTK_WINDOW (data->browser), "export-to-facebook");
		break;

	case GTK_RESPONSE_DELETE_EVENT:
	case GTK_RESPONSE_CANCEL:
		facebook_accounts_save_to_file (data->accounts, data->account);
		gtk_widget_destroy (data->dialog);
		break;

	case GTK_RESPONSE_OK:
		{
			char  *album_title;
			GList *file_list;

			gtk_widget_hide (data->dialog);
			gth_task_dialog (GTH_TASK (data->conn), FALSE);

			data->album = NULL;
			album_title = gtk_combo_box_get_active_text (GTK_COMBO_BOX (GET_WIDGET ("album_comboboxentry")));
			if ((album_title != NULL) && (g_strcmp0 (album_title, "") != 0)) {
				GList *link;

				link = g_list_find_custom (data->albums, album_title, (GCompareFunc) find_album_by_title);
				if (link != NULL)
					data->album = g_object_ref (link->data);

				if (data->album == NULL) {
					data->album = facebook_album_new ();
					facebook_album_set_title (data->album, album_title);
				}
			}

			file_list = gth_file_data_list_to_file_list (data->file_list);
			facebook_service_post_photos (data->service,
						    gtk_combo_box_get_active (GTK_COMBO_BOX (GET_WIDGET ("privacy_combobox"))),
						    gtk_combo_box_get_active (GTK_COMBO_BOX (GET_WIDGET ("safety_combobox"))),
						    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("hidden_checkbutton"))),
						    file_list,
						    data->cancellable,
						    post_photos_ready_cb,
						    data);

			_g_object_list_unref (file_list);
			g_free (album_title);
		}
		break;

	default:
		break;
	}
}


static void
update_account_list (DialogData *data)
{
	GtkTreeIter  iter;
	int          current_account;
	int          idx;
	GList       *scan;
	char        *free_space;

	current_account = 0;
	gtk_list_store_clear (GTK_LIST_STORE (GET_WIDGET ("account_liststore")));
	for (scan = data->accounts, idx = 0; scan; scan = scan->next, idx++) {
		FacebookAccount *account = scan->data;

		if ((data->account != NULL) && (g_strcmp0 (data->account->username, account->username) == 0))
			current_account = idx;

		gtk_list_store_append (GTK_LIST_STORE (GET_WIDGET ("account_liststore")), &iter);
		gtk_list_store_set (GTK_LIST_STORE (GET_WIDGET ("account_liststore")), &iter,
				    ACCOUNT_DATA_COLUMN, account,
				    ACCOUNT_NAME_COLUMN, account->username,
				    -1);
	}
	gtk_combo_box_set_active (GTK_COMBO_BOX (GET_WIDGET ("account_combobox")), current_account);

	free_space = g_format_size_for_display (data->user->max_bandwidth - data->user->used_bandwidth);
	gtk_label_set_text (GTK_LABEL (GET_WIDGET ("free_space_label")), free_space);
	g_free (free_space);
}


static void
album_list_ready_cb (GObject      *source_object,
			GAsyncResult *res,
			gpointer      user_data)
{
	DialogData *data = user_data;
	GError     *error = NULL;
	GList      *scan;

	_g_object_list_unref (data->albums);
	data->albums = facebook_service_list_albums_finish (FACEBOOK_SERVICE (source_object), res, &error);
	if (error != NULL) {
		if (data->conn != NULL)
			gth_task_dialog (GTH_TASK (data->conn), TRUE);
		_gtk_error_dialog_from_gerror_run (GTK_WINDOW (data->browser), _("Could not connect to the server"), &error);
		gtk_widget_destroy (data->dialog);
		return;
	}

	gtk_list_store_clear (GTK_LIST_STORE (GET_WIDGET ("album_liststore")));
	for (scan = data->albums; scan; scan = scan->next) {
		FacebookPhotoset *album = scan->data;
		char           *n_photos;
		GtkTreeIter     iter;

		n_photos = g_strdup_printf ("(%d)", album->n_photos);

		gtk_list_store_append (GTK_LIST_STORE (GET_WIDGET ("album_liststore")), &iter);
		gtk_list_store_set (GTK_LIST_STORE (GET_WIDGET ("album_liststore")), &iter,
				    ALBUM_DATA_COLUMN, album,
				    ALBUM_ICON_COLUMN, "file-catalog",
				    ALBUM_TITLE_COLUMN, album->title,
				    ALBUM_N_PHOTOS_COLUMN, n_photos,
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
get_album_list (DialogData *data)
{
	facebook_service_list_albums (data->service,
				       NULL,
				       data->cancellable,
				       album_list_ready_cb,
				       data);
}


static void
upload_status_ready_cb (GObject      *source_object,
			GAsyncResult *res,
			gpointer      user_data)
{
	DialogData *data = user_data;
	GError     *error = NULL;

	_g_object_unref (data->user);
	data->user = facebook_service_get_upload_status_finish (FACEBOOK_SERVICE (source_object), res, &error);
	if (error != NULL) {
		if (data->conn != NULL)
			gth_task_dialog (GTH_TASK (data->conn), TRUE);
		_gtk_error_dialog_from_gerror_run (GTK_WINDOW (data->browser), _("Could not connect to the server"), &error);
		gtk_widget_destroy (data->dialog);
		return;
	}

	update_account_list (data);
	get_album_list (data);
}


static void
connect_to_server (DialogData *data)
{
	g_return_if_fail (data->account != NULL);

	facebook_connection_set_auth_token (data->conn, data->account->token);
	if (data->service == NULL)
		data->service = facebook_service_new (data->conn);
	facebook_service_get_upload_status (data->service,
					  data->cancellable,
					  upload_status_ready_cb,
					  data);
}


static void
connection_token_ready_cb (GObject      *source_object,
			   GAsyncResult *res,
			   gpointer      user_data)
{
	DialogData *data = user_data;
	GError     *error = NULL;
	GList      *link;

	if (! facebook_connection_get_token_finish (FACEBOOK_CONNECTION (source_object), res, &error)) {
		if (data->conn != NULL)
			gth_task_dialog (GTH_TASK (data->conn), TRUE);
		_gtk_error_dialog_from_gerror_run (GTK_WINDOW (data->browser), _("Could not connect to the server"), &error);
		gtk_widget_destroy (data->dialog);
		return;
	}

	_g_object_unref (data->account);
	data->account = facebook_account_new ();
	facebook_account_set_username (data->account, facebook_connection_get_username (data->conn));
	facebook_account_set_token (data->account, facebook_connection_get_auth_token (data->conn));

	link = g_list_find_custom (data->accounts, data->account, (GCompareFunc) facebook_account_cmp);
	if (link != NULL) {
		data->accounts = g_list_remove_link (data->accounts, link);
		_g_object_list_unref (link);
	}
	data->accounts = g_list_prepend (data->accounts, g_object_ref (data->account));

	connect_to_server (data);
}


static void
complete_authorization_messagedialog_response_cb (GtkDialog *dialog,
						  int        response_id,
						  gpointer   user_data)
{
	DialogData *data = user_data;

	switch (response_id) {
	case GTK_RESPONSE_HELP:
		show_help_dialog (GTK_WINDOW (dialog), "flicker-complete-authorization");
		break;

	case GTK_RESPONSE_DELETE_EVENT:
	case GTK_RESPONSE_CANCEL:
		gtk_widget_destroy (GTK_WIDGET (dialog));
		gtk_widget_destroy (data->dialog);
		break;

	case GTK_RESPONSE_OK:
		gtk_widget_destroy (GTK_WIDGET (dialog));
		gth_task_dialog (GTH_TASK (data->conn), FALSE);
		facebook_connection_get_token (data->conn,
					     data->cancellable,
					     connection_token_ready_cb,
					     data);
		break;

	default:
		break;
	}
}


static void
complete_authorization (DialogData *data)
{
	GtkBuilder *builder;
	GtkWidget  *dialog;

	gth_task_dialog (GTH_TASK (data->conn), TRUE);

	builder = _gtk_builder_new_from_file ("flicker-complete-authorization.ui", "flicker");
	dialog = _gtk_builder_get_widget (builder, "complete_authorization_messagedialog");
	g_object_set_data_full (G_OBJECT (dialog), "builder", builder, g_object_unref);
	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (complete_authorization_messagedialog_response_cb),
			  data);

	gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (data->browser));
	gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
	gtk_window_present (GTK_WINDOW (dialog));
}


static void
ask_authorization_messagedialog_response_cb (GtkDialog *dialog,
					     int        response_id,
					     gpointer   user_data)
{
	DialogData *data = user_data;

	switch (response_id) {
	case GTK_RESPONSE_HELP:
		show_help_dialog (GTK_WINDOW (dialog), "flicker-ask-authorization");
		break;

	case GTK_RESPONSE_DELETE_EVENT:
	case GTK_RESPONSE_CANCEL:
		gtk_widget_destroy (GTK_WIDGET (dialog));
		gtk_widget_destroy (data->dialog);
		break;

	case GTK_RESPONSE_OK:
		{
			char   *url;
			GError *error = NULL;

			gtk_widget_destroy (GTK_WIDGET (dialog));

			url = facebook_connection_get_login_link (data->conn, FACEBOOK_ACCESS_WRITE);
			if (gtk_show_uri (gtk_widget_get_screen (GTK_WIDGET (dialog)), url, 0, &error)) {
				complete_authorization (data);
			}
			else {
				if (data->conn != NULL)
					gth_task_dialog (GTH_TASK (data->conn), TRUE);
				_gtk_error_dialog_from_gerror_run (GTK_WINDOW (data->browser), _("Could not connect to the server"), &error);
				gtk_widget_destroy (data->dialog);
			}

			g_free (url);
		}
		break;

	default:
		break;
	}
}


static void
ask_authorization (DialogData *data)
{
	GtkBuilder *builder;
	GtkWidget  *dialog;

	gth_task_dialog (GTH_TASK (data->conn), TRUE);

	builder = _gtk_builder_new_from_file ("flicker-ask-authorization.ui", "flicker");
	dialog = _gtk_builder_get_widget (builder, "ask_authorization_messagedialog");
	g_object_set_data_full (G_OBJECT (dialog), "builder", builder, g_object_unref);
	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (ask_authorization_messagedialog_response_cb),
			  data);

	gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (data->browser));
	gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
	gtk_window_present (GTK_WINDOW (dialog));
}


static void
connection_frob_ready_cb (GObject      *source_object,
			  GAsyncResult *res,
			  gpointer      user_data)
{
	DialogData *data = user_data;
	GError     *error = NULL;

	if (! facebook_connection_get_frob_finish (FACEBOOK_CONNECTION (source_object), res, &error)) {
		if (data->conn != NULL)
			gth_task_dialog (GTH_TASK (data->conn), TRUE);
		_gtk_error_dialog_from_gerror_run (GTK_WINDOW (data->browser), _("Could not connect to the server"), &error);
		gtk_widget_destroy (data->dialog);
		return;
	}

	ask_authorization (data);
}


static void
start_authorization_process (DialogData *data)
{
	facebook_connection_get_frob (data->conn,
				    data->cancellable,
				    connection_frob_ready_cb,
				    data);
}


static void
account_chooser_dialog_response_cb (GtkDialog *dialog,
				    int        response_id,
				    gpointer   user_data)
{
	DialogData *data = user_data;

	switch (response_id) {
	case GTK_RESPONSE_DELETE_EVENT:
	case GTK_RESPONSE_CANCEL:
		gtk_widget_destroy (GTK_WIDGET (dialog));
		gtk_widget_destroy (data->dialog);
		break;

	case GTK_RESPONSE_OK:
		_g_object_unref (data->account);
		data->account = facebook_account_chooser_dialog_get_active (FACEBOOK_ACCOUNT_CHOOSER_DIALOG (dialog));
		if (data->account != NULL) {
			gtk_widget_destroy (GTK_WIDGET (dialog));
			connect_to_server (data);
		}

		break;

	case FACEBOOK_ACCOUNT_CHOOSER_RESPONSE_NEW:
		gtk_widget_destroy (GTK_WIDGET (dialog));
		start_authorization_process (data);
		break;

	default:
		break;
	}
}


static void
auto_select_account (DialogData *data)
{
	gtk_widget_hide (data->dialog);
	gth_task_dialog (GTH_TASK (data->conn), FALSE);

	if (data->accounts != NULL) {
		if (data->account != NULL) {
			connect_to_server (data);
		}
		else if (data->accounts->next == NULL) {
			data->account = g_object_ref (data->accounts->data);
			connect_to_server (data);
		}
		else {
			GtkWidget *dialog;

			gth_task_dialog (GTH_TASK (data->conn), TRUE);
			dialog = facebook_account_chooser_dialog_new (data->accounts, data->account);
			g_signal_connect (dialog,
					  "response",
					  G_CALLBACK (account_chooser_dialog_response_cb),
					  data);

			gtk_window_set_title (GTK_WINDOW (dialog), _("Choose Account"));
			gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (data->browser));
			gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
			gtk_window_present (GTK_WINDOW (dialog));
		}
	}
	else
		start_authorization_process (data);
}


static void
account_manager_dialog_response_cb (GtkDialog *dialog,
			            int        response_id,
			            gpointer   user_data)
{
	DialogData *data = user_data;

	switch (response_id) {
	case GTK_RESPONSE_DELETE_EVENT:
	case GTK_RESPONSE_CANCEL:
		gtk_widget_destroy (GTK_WIDGET (dialog));
		break;

	case GTK_RESPONSE_OK:
		_g_object_list_unref (data->accounts);
		data->accounts = facebook_account_manager_dialog_get_accounts (FACEBOOK_ACCOUNT_MANAGER_DIALOG (dialog));
		if (! g_list_find_custom (data->accounts, data->account, (GCompareFunc) facebook_account_cmp)) {
			_g_object_unref (data->account);
			data->account = NULL;
			auto_select_account (data);
		}
		else
			update_account_list (data);
		facebook_accounts_save_to_file (data->accounts, data->account);
		gtk_widget_destroy (GTK_WIDGET (dialog));
		break;

	default:
		break;
	}
}


static void
edit_accounts_button_clicked_cb (GtkButton *button,
			         gpointer   user_data)
{
	DialogData *data = user_data;
	GtkWidget  *dialog;

	dialog = facebook_account_manager_dialog_new (data->accounts);
	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (account_manager_dialog_response_cb),
			  data);

	gtk_window_set_title (GTK_WINDOW (dialog), _("Edit Accounts"));
	gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (data->dialog));
	gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
	gtk_window_present (GTK_WINDOW (dialog));
}


static void
account_combobox_changed_cb (GtkComboBox *widget,
			     gpointer     user_data)
{
	DialogData    *data = user_data;
	GtkTreeIter    iter;
	FacebookAccount *account;

	if (! gtk_combo_box_get_active_iter (widget, &iter))
		return;

	gtk_tree_model_get (gtk_combo_box_get_model (widget),
			    &iter,
			    ACCOUNT_DATA_COLUMN, &account,
			    -1);

	if (facebook_account_cmp (account, data->account) != 0) {
		_g_object_unref (data->account);
		data->account = account;
		auto_select_account (data);
	}
	else
		g_object_unref (account);
}


void
dlg_export_to_facebook (GthBrowser *browser,
		      GList      *file_list)
{
	DialogData *data;
	GList      *scan;
	int         n_total;
	goffset     total_size;
	char       *total_size_formatted;
	char       *text;
	GtkWidget  *list_view;

	data = g_new0 (DialogData, 1);
	data->browser = browser;
	data->location = gth_file_data_dup (gth_browser_get_location_data (browser));
	data->builder = _gtk_builder_new_from_file ("export-to-facebook.ui", "flicker");
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

	gtk_entry_set_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (GET_WIDGET ("album_comboboxentry")))), g_file_info_get_edit_name (data->location->info));
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

	data->conn = facebook_connection_new ();
	data->progress_dialog = gth_progress_dialog_new (GTK_WINDOW (data->browser));
	gth_progress_dialog_add_task (GTH_PROGRESS_DIALOG (data->progress_dialog), GTH_TASK (data->conn));

	data->accounts = facebook_accounts_load_from_file ();
	data->account = facebook_accounts_find_default (data->accounts);
	auto_select_account (data);
}
