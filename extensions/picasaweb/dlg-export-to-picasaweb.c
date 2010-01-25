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
#ifdef HAVE_GNOME_KEYRING
#include <gnome-keyring.h>
#endif /* HAVE_GNOME_KEYRING */
#include <gthumb.h>
#include "dlg-export-to-picasaweb.h"
#include "picasa-account-chooser-dialog.h"
#include "picasa-account-manager-dialog.h"
#include "picasa-account-properties-dialog.h"
#include "picasa-album-properties-dialog.h"
#include "picasa-web-album.h"
#include "picasa-web-service.h"
#include "picasa-web-user.h"


#define GET_WIDGET(x) (_gtk_builder_get_widget (data->builder, (x)))


typedef enum {
	ITEM_TYPE_COMMAND,
	ITEM_TYPE_ENTRY,
	ITEM_TYPE_SEPARATOR
} ItemType;


enum {
	ACCOUNT_EMAIL_COLUMN,
	ACCOUNT_TYPE_COLUMN,
	ACCOUNT_NAME_COLUMN,
	ACCOUNT_ICON_COLUMN
};


enum {
	ALBUM_DATA_COLUMN,
	ALBUM_NAME_COLUMN,
	ALBUM_ICON_COLUMN,
	ALBUM_REMAINING_IMAGES_COLUMN,
	ALBUM_USED_BYTES_COLUMN
};


typedef struct {
	GthBrowser       *browser;
	GtkBuilder       *builder;
	GtkWidget        *dialog;
	GtkWidget        *progress_dialog;
	GList            *accounts;
	PicasaWebUser    *user;
	char             *email;
	char             *password;
	char             *challange;
	GList            *albums;
	GoogleConnection *conn;
	PicasaWebService *picasaweb;
	GCancellable     *cancellable;
} DialogData;


static void
export_dialog_destroy_cb (GtkWidget  *widget,
			  DialogData *data)
{
	if (data->conn != NULL)
		gth_task_completed (GTH_TASK (data->conn), NULL);

	_g_object_unref (data->cancellable);
	_g_object_unref (data->picasaweb);
	_g_object_unref (data->conn);
	_g_object_list_unref (data->albums);
	g_free (data->challange);
	g_free (data->password);
	g_free (data->email);
	_g_object_unref (data->user);
	_g_string_list_free (data->accounts);
	_g_object_unref (data->builder);
	g_free (data);
}


static void
export_dialog_response_cb (GtkDialog *dialog,
			   int        response_id,
			   gpointer   user_data)
{
	DialogData *data = user_data;

	switch (response_id) {
	case GTK_RESPONSE_HELP:
		show_help_dialog (GTK_WINDOW (data->browser), "export-to-picasaweb");
		break;

	case GTK_RESPONSE_DELETE_EVENT:
	case GTK_RESPONSE_CANCEL:
		picasa_web_accounts_save_to_file (data->accounts, data->email);
		gtk_widget_destroy (data->dialog);
		break;

	case GTK_RESPONSE_OK:
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

	current_account = 0;
	gtk_list_store_clear (GTK_LIST_STORE (GET_WIDGET ("account_liststore")));
	for (scan = data->accounts, idx = 0; scan; scan = scan->next, idx++) {
		char *account = scan->data;

		if (g_strcmp0 (account, data->email) == 0)
			current_account = idx;

		gtk_list_store_append (GTK_LIST_STORE (GET_WIDGET ("account_liststore")), &iter);
		gtk_list_store_set (GTK_LIST_STORE (GET_WIDGET ("account_liststore")), &iter,
				    ACCOUNT_EMAIL_COLUMN, account,
				    ACCOUNT_TYPE_COLUMN, ITEM_TYPE_ENTRY,
				    ACCOUNT_NAME_COLUMN, account,
				    -1);
	}

	/* FIXME
	gtk_list_store_append (GTK_LIST_STORE (GET_WIDGET ("account_liststore")), &iter);
	gtk_list_store_set (GTK_LIST_STORE (GET_WIDGET ("account_liststore")), &iter,
			    ACCOUNT_TYPE_COLUMN, ITEM_TYPE_SEPARATOR,
			    -1);

	gtk_list_store_append (GTK_LIST_STORE (GET_WIDGET ("account_liststore")), &iter);
	gtk_list_store_set (GTK_LIST_STORE (GET_WIDGET ("account_liststore")), &iter,
			    ACCOUNT_EMAIL_COLUMN, NULL,
			    ACCOUNT_TYPE_COLUMN, ITEM_TYPE_COMMAND,
			    ACCOUNT_ICON_COLUMN, GTK_STOCK_EDIT,
			    ACCOUNT_NAME_COLUMN, _("Edit Accounts..."),
			    -1);
	*/

	gtk_combo_box_set_active (GTK_COMBO_BOX (GET_WIDGET ("account_combobox")), current_account);
}


static void
update_album_list (DialogData *data)
{
	GtkTreeIter  iter;
	GList       *scan;
	char        *free_space;

	g_return_if_fail (data->user != NULL);

	free_space = g_format_size_for_display (data->user->quota_limit - data->user->quota_current);
	gtk_label_set_text (GTK_LABEL (GET_WIDGET ("free_space_label")), free_space);
	g_free (free_space);

	gtk_list_store_clear (GTK_LIST_STORE (GET_WIDGET ("album_liststore")));
	for (scan = data->albums; scan; scan = scan->next) {
		PicasaWebAlbum *album = scan->data;
		char           *n_photos_remaining;
		char           *used_bytes;

		n_photos_remaining = g_strdup_printf ("%d", album->n_photos_remaining);
		used_bytes = g_format_size_for_display (album->used_bytes);

		gtk_list_store_append (GTK_LIST_STORE (GET_WIDGET ("album_liststore")), &iter);
		gtk_list_store_set (GTK_LIST_STORE (GET_WIDGET ("album_liststore")), &iter,
				    ALBUM_DATA_COLUMN, album,
				    ALBUM_ICON_COLUMN, "file-catalog",
				    ALBUM_NAME_COLUMN, album->title,
				    ALBUM_REMAINING_IMAGES_COLUMN, n_photos_remaining,
				    ALBUM_USED_BYTES_COLUMN, used_bytes,
				    -1);

		g_free (used_bytes);
		g_free (n_photos_remaining);
	}

	gtk_widget_set_sensitive (GET_WIDGET ("upload_button"), data->albums != NULL);
}


static void
show_export_dialog (DialogData *data)
{
	update_account_list (data);
	update_album_list (data);
	gth_task_dialog (GTH_TASK (data->conn), TRUE);

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), GTK_WINDOW (data->browser));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), FALSE);
	gtk_window_present (GTK_WINDOW (data->dialog));
}


static void
list_albums_ready_cb (GObject      *source_object,
		      GAsyncResult *result,
		      gpointer      user_data)
{
	DialogData       *data = user_data;
	PicasaWebService *picasaweb = PICASA_WEB_SERVICE (source_object);
	GError           *error = NULL;

	_g_object_list_unref (data->albums);
	data->albums = picasa_web_service_list_albums_finish (picasaweb, result, &error);
	if (error != NULL) {
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (data->browser), _("Could not get the album list"), &error);
		gtk_widget_destroy (data->dialog);
		return;
	}

	_g_object_unref (data->user);
	data->user = g_object_ref (picasa_web_service_get_user (picasaweb));
	show_export_dialog (data);
}


static void
get_album_list (DialogData *data)
{
	if (data->picasaweb == NULL)
		data->picasaweb = picasa_web_service_new (data->conn);
	picasa_web_service_list_albums (data->picasaweb,
				        "default",
				        data->cancellable,
				        list_albums_ready_cb,
				        data);
}


#ifdef HAVE_GNOME_KEYRING
static void
store_password_done_cb (GnomeKeyringResult result,
			gpointer           user_data)
{
	get_album_list ((DialogData *) user_data);
}
#endif


static void account_properties_dialog (DialogData *data,
			               const char *email);
static void challange_account_dialog  (DialogData *data);


static void
connection_ready_cb (GObject      *source_object,
		     GAsyncResult *result,
		     gpointer      user_data)
{
	DialogData       *data = user_data;
	GoogleConnection *conn = GOOGLE_CONNECTION (source_object);
	GError           *error = NULL;

	if (! google_connection_connect_finish (conn, result, &error)) {
		if (g_error_matches (error, GOOGLE_CONNECTION_ERROR, GOOGLE_CONNECTION_ERROR_CAPTCHA_REQUIRED)) {
			challange_account_dialog (data);
		}
		else if (g_error_matches (error, GOOGLE_CONNECTION_ERROR, GOOGLE_CONNECTION_ERROR_BAD_AUTHENTICATION)) {
			account_properties_dialog (data, data->email);
		}
		else {
			_gtk_error_dialog_from_gerror_show (GTK_WINDOW (data->browser), _("Could not connect to the server"), &error);
			gtk_widget_destroy (data->dialog);
		}
		return;
	}

	if (! g_list_find_custom (data->accounts, data->email, (GCompareFunc) strcmp))
		data->accounts = g_list_append (data->accounts, g_strdup (data->email));

#ifdef HAVE_GNOME_KEYRING
	if (gnome_keyring_is_available ()) {
		gnome_keyring_store_password (GNOME_KEYRING_NETWORK_PASSWORD,
					      GNOME_KEYRING_SESSION,
					      _("Picasa Web Album"),
					      data->password,
					      store_password_done_cb,
					      data,
					      NULL,
					      "user", data->email,
					      "server", "picasaweb.google.com",
					      "protocol", "http",
					      NULL);
		return;
	}
#endif

	get_album_list (data);
}


static void connect_to_server (DialogData *data);


static void
account_properties_dialog_response_cb (GtkDialog *dialog,
				       int        response_id,
				       gpointer   user_data)
{
	DialogData *data = user_data;

	switch (response_id) {
	case GTK_RESPONSE_HELP:
		show_help_dialog (GTK_WINDOW (dialog), "picasaweb-account-properties");
		break;

	case GTK_RESPONSE_DELETE_EVENT:
	case GTK_RESPONSE_CANCEL:
		gtk_widget_destroy (GTK_WIDGET (dialog));
		gtk_widget_destroy (data->dialog);
		break;

	case GTK_RESPONSE_OK:
		g_free (data->email);
		g_free (data->password);
		g_free (data->challange);
		data->email = g_strdup (picasa_account_properties_dialog_get_email (PICASA_ACCOUNT_PROPERTIES_DIALOG (dialog)));
		data->password = g_strdup (picasa_account_properties_dialog_get_password (PICASA_ACCOUNT_PROPERTIES_DIALOG (dialog)));
		data->challange = NULL;
		gtk_widget_destroy (GTK_WIDGET (dialog));
		connect_to_server (data);
		break;

	default:
		break;
	}
}


static void
account_properties_dialog (DialogData *data,
			   const char *email)
{
	GtkWidget *dialog;

	if (data->conn != NULL)
		gth_task_dialog (GTH_TASK (data->conn), TRUE);

	dialog = picasa_account_properties_dialog_new (email, NULL, NULL);
	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (account_properties_dialog_response_cb),
			  data);

	gtk_window_set_title (GTK_WINDOW (dialog), _("Account"));
	gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (data->browser));
	gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
	gtk_window_present (GTK_WINDOW (dialog));
}


static void
connect_to_server_step2 (DialogData *data)
{
	if (data->password == NULL) {
		gth_task_dialog (GTH_TASK (data->conn), TRUE);
		account_properties_dialog (data, data->email);
	}
	else {
		gth_task_dialog (GTH_TASK (data->conn), FALSE);
		google_connection_connect (data->conn,
					   data->email,
					   data->password,
					   data->challange,
					   data->cancellable,
					   connection_ready_cb,
					   data);
	}
}


#ifdef HAVE_GNOME_KEYRING
static void
find_password_cb (GnomeKeyringResult result,
                  const char        *string,
                  gpointer           user_data)
{
	DialogData *data = user_data;

	if (string != NULL)
		data->password = g_strdup (string);
	connect_to_server_step2 (data);
}
#endif


static void
connect_to_server (DialogData *data)
{
	if (data->conn == NULL) {
		data->conn = google_connection_new (GOOGLE_SERVICE_PICASA_WEB_ALBUM);
		data->progress_dialog = gth_progress_dialog_new (GTK_WINDOW (data->browser));
		gth_progress_dialog_add_task (GTH_PROGRESS_DIALOG (data->progress_dialog), GTH_TASK (data->conn));
	}

#ifdef HAVE_GNOME_KEYRING
	if ((data->password == NULL) && gnome_keyring_is_available ()) {
		gnome_keyring_find_password (GNOME_KEYRING_NETWORK_PASSWORD,
					     find_password_cb,
					     data,
					     NULL,
					     "user", data->email,
					     "server", "picasaweb.google.com",
					     "protocol", "http",
					     NULL);
		return;
	}
#endif

	connect_to_server_step2 (data);
}


static void
challange_account_dialog_response_cb (GtkDialog *dialog,
				      int        response_id,
				      gpointer   user_data)
{
	DialogData *data = user_data;

	switch (response_id) {
	case GTK_RESPONSE_HELP:
		show_help_dialog (GTK_WINDOW (dialog), "picasaweb-account-challange");
		break;

	case GTK_RESPONSE_DELETE_EVENT:
	case GTK_RESPONSE_CANCEL:
		gtk_widget_destroy (GTK_WIDGET (dialog));
		gtk_widget_destroy (data->dialog);
		break;

	case GTK_RESPONSE_OK:
		g_free (data->email);
		g_free (data->password);
		data->email = g_strdup (picasa_account_properties_dialog_get_email (PICASA_ACCOUNT_PROPERTIES_DIALOG (dialog)));
		data->password = g_strdup (picasa_account_properties_dialog_get_password (PICASA_ACCOUNT_PROPERTIES_DIALOG (dialog)));
		data->challange = g_strdup (picasa_account_properties_dialog_get_challange (PICASA_ACCOUNT_PROPERTIES_DIALOG (dialog)));
		gtk_widget_destroy (GTK_WIDGET (dialog));
		connect_to_server (data);
		break;

	default:
		break;
	}
}


static void
challange_account_dialog (DialogData *data)
{
	GtkWidget *dialog;

	dialog = picasa_account_properties_dialog_new (data->email, data->password, google_connection_get_challange_url (data->conn));
	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (challange_account_dialog_response_cb),
			  data);

	gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (data->browser));
	gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
	gtk_window_present (GTK_WINDOW (dialog));
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
		g_free (data->password);
		data->password = NULL;
		g_free (data->challange);
		data->challange = NULL;
		g_free (data->email);
		data->email = picasa_account_chooser_dialog_get_active (PICASA_ACCOUNT_CHOOSER_DIALOG (dialog));
		if (data->email != NULL) {
			gtk_widget_destroy (GTK_WIDGET (dialog));
			connect_to_server (data);
		}

		break;

	case PICASA_ACCOUNT_CHOOSER_RESPONSE_NEW:
		gtk_widget_destroy (GTK_WIDGET (dialog));
		account_properties_dialog (data, NULL);
		break;

	default:
		break;
	}
}


static gboolean
account_combobox_row_separator_func (GtkTreeModel *model,
				     GtkTreeIter  *iter,
				     gpointer      data)
{
	int item_type;

	gtk_tree_model_get (model, iter, ACCOUNT_TYPE_COLUMN, &item_type, -1);

	return item_type == ITEM_TYPE_SEPARATOR;
}


static void
create_album_ready_cb (GObject      *source_object,
		       GAsyncResult *result,
		       gpointer      user_data)
{
	DialogData       *data = user_data;
	PicasaWebService *picasaweb = PICASA_WEB_SERVICE (source_object);
	PicasaWebAlbum   *album;
	GError           *error = NULL;

	album = picasa_web_service_create_album_finish (picasaweb, result, &error);
	if (error != NULL) {
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (data->browser), _("Could not create the album"), &error);
		return;
	}

	data->albums = g_list_append (data->albums, album);
	update_album_list (data);
}


static void
new_album_dialog_response_cb (GtkDialog *dialog,
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
		{
			PicasaWebAlbum *album;

			album = picasa_web_album_new ();
			picasa_web_album_set_title (album, picasa_album_properties_dialog_get_name (PICASA_ALBUM_PROPERTIES_DIALOG (dialog)));
			album->access = picasa_album_properties_dialog_get_access (PICASA_ALBUM_PROPERTIES_DIALOG (dialog));
			picasa_web_service_create_album (data->picasaweb,
							 album,
							 data->cancellable,
							 create_album_ready_cb,
							 data);

			g_object_unref (album);
		}
		break;

	default:
		break;
	}
}


static void
add_album_button_clicked_cb (GtkButton *button,
			     gpointer   user_data)
{
	DialogData *data = user_data;
	GtkWidget  *dialog;

	dialog = picasa_album_properties_dialog_new (NULL, NULL, PICASA_WEB_ACCESS_PUBLIC);  /* FIXME: use the current catalog/folder name as default value */
	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (new_album_dialog_response_cb),
			  data);

	gtk_window_set_title (GTK_WINDOW (dialog), _("New Album"));
	gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (data->browser));
	gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
	gtk_window_present (GTK_WINDOW (dialog));
}


static void
auto_select_account (DialogData *data)
{
	gtk_widget_hide (data->dialog);

	if (data->accounts != NULL) {
		/* FIXME: remove comment when done
		if (data->email != NULL) {
			connect_to_server (data);
		}
		else if (data->accounts->next == NULL) {
			data->email = g_strdup ((char *)data->accounts->data);
			connect_to_server (data);
		}
		else {*/
			GtkWidget *dialog;

			dialog = picasa_account_chooser_dialog_new (data->accounts, data->email);
			g_signal_connect (dialog,
					  "response",
					  G_CALLBACK (account_chooser_dialog_response_cb),
					  data);

			gtk_window_set_title (GTK_WINDOW (dialog), _("Choose Account"));
			gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (data->browser));
			gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
			gtk_window_present (GTK_WINDOW (dialog));
		/*}*/
	}
	else
		account_properties_dialog (data, NULL);
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
		_g_string_list_free (data->accounts);
		data->accounts = picasa_account_manager_dialog_get_accounts (PICASA_ACCOUNT_MANAGER_DIALOG (dialog));
		if (! g_list_find_custom (data->accounts, data->email, (GCompareFunc) strcmp)) {
			g_free (data->email);
			data->email = NULL;
			auto_select_account (data);
		}
		else
			update_account_list (data);
		picasa_web_accounts_save_to_file (data->accounts, data->email);
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

	dialog = picasa_account_manager_dialog_new (data->accounts);
	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (account_manager_dialog_response_cb),
			  data);

	gtk_window_set_title (GTK_WINDOW (dialog), _("Edit Accounts"));
	gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (data->browser));
	gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
	gtk_window_present (GTK_WINDOW (dialog));
}


static void
albums_treeview_selection_changed_cb (GtkTreeSelection *treeselection,
				      gpointer          user_data)
{
	DialogData *data = user_data;
	gboolean    selected;

	selected = gtk_tree_selection_get_selected (treeselection, NULL, NULL);
	gtk_widget_set_sensitive (GET_WIDGET ("upload_button"), selected);
}


void
dlg_export_to_picasaweb (GthBrowser *browser)
{
	DialogData       *data;
	GtkTreeSelection *selection;

	data = g_new0 (DialogData, 1);
	data->browser = browser;
	data->builder = _gtk_builder_new_from_file ("export-to-picasaweb.ui", "picasaweb");
	data->dialog = _gtk_builder_get_widget (data->builder, "export_dialog");
	data->cancellable = g_cancellable_new ();

	/* Set the widget data */

	gtk_combo_box_set_row_separator_func (GTK_COMBO_BOX (GET_WIDGET ("account_combobox")),
					      account_combobox_row_separator_func,
					      data,
					      NULL);
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
	g_signal_connect (GET_WIDGET ("add_album_button"),
			  "clicked",
			  G_CALLBACK (add_album_button_clicked_cb),
			  data);
	g_signal_connect (GET_WIDGET ("edit_accounts_button"),
			  "clicked",
			  G_CALLBACK (edit_accounts_button_clicked_cb),
			  data);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (GET_WIDGET ("albums_treeview")));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
	g_signal_connect (selection,
			  "changed",
			  G_CALLBACK (albums_treeview_selection_changed_cb),
			  data);

	data->accounts = picasa_web_accounts_load_from_file (&data->email);
	auto_select_account (data);
}
