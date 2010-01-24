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
#include "gth-account-chooser-dialog.h"
#include "gth-account-properties-dialog.h"
#include "gth-album-properties-dialog.h"
#include "picasa-web-album.h"
#include "picasa-web-service.h"


#define GET_WIDGET(x) (_gtk_builder_get_widget (data->builder, (x)))


typedef enum {
	ITEM_TYPE_COMMAND,
	ITEM_TYPE_ENTRY,
	ITEM_TYPE_SEPARATOR
} ItemType;


enum {
	ACCOUNT_DATA_COLUMN,
	ACCOUNT_TYPE_COLUMN,
	ACCOUNT_NAME_COLUMN,
	ACCOUNT_ICON_COLUMN,
	ACCOUNT_SENSITIVE_COLUMN
};


enum {
	ALBUM_DATA_COLUMN,
	ALBUM_TYPE_COLUMN,
	ALBUM_NAME_COLUMN,
	ALBUM_ICON_COLUMN,
	ALBUM_SENSITIVE_COLUMN
};


typedef struct {
	GthBrowser       *browser;
	GtkBuilder       *builder;
	GtkWidget        *dialog;
	GtkWidget        *progress_dialog;
	GList            *accounts;
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
		picasa_web_accounts_save_to_file (data->accounts);
		gtk_widget_destroy (data->dialog);
		break;

	case GTK_RESPONSE_OK:
		break;

	default:
		break;
	}
}


static void
update_album_list (DialogData *data)
{
	GtkTreeIter  iter;
	GList       *scan;

	gtk_list_store_clear (GTK_LIST_STORE (GET_WIDGET ("album_liststore")));

	for (scan = data->albums; scan; scan = scan->next) {
		PicasaWebAlbum *album = scan->data;

		gtk_list_store_append (GTK_LIST_STORE (GET_WIDGET ("album_liststore")), &iter);
		gtk_list_store_set (GTK_LIST_STORE (GET_WIDGET ("album_liststore")), &iter,
				    ALBUM_DATA_COLUMN, album,
				    ALBUM_TYPE_COLUMN, ITEM_TYPE_ENTRY,
				    ALBUM_ICON_COLUMN, "file-catalog",
				    ALBUM_NAME_COLUMN, album->title,
				    ALBUM_SENSITIVE_COLUMN, TRUE,
				    -1);
	}

	gtk_widget_set_sensitive (GET_WIDGET ("upload_button"), data->albums != NULL);
}


static void
show_export_dialog (DialogData *data)
{
	GtkTreeIter  iter;
	int          current_account;
	int          idx;
	GList       *scan;

	/* Accounts */

	gtk_list_store_clear (GTK_LIST_STORE (GET_WIDGET ("account_liststore")));

	current_account = 0;
	for (scan = data->accounts, idx = 0; scan; scan = scan->next, idx++) {
		char *account = scan->data;

		if (g_strcmp0 (account, data->email) == 0)
			current_account = idx;
		gtk_list_store_append (GTK_LIST_STORE (GET_WIDGET ("account_liststore")), &iter);
		gtk_list_store_set (GTK_LIST_STORE (GET_WIDGET ("account_liststore")), &iter,
				    ACCOUNT_DATA_COLUMN, account,
				    ACCOUNT_TYPE_COLUMN, ITEM_TYPE_ENTRY,
				    ACCOUNT_NAME_COLUMN, account,
				    ACCOUNT_SENSITIVE_COLUMN, TRUE,
				    -1);
	}

	gtk_list_store_append (GTK_LIST_STORE (GET_WIDGET ("account_liststore")), &iter);
	gtk_list_store_set (GTK_LIST_STORE (GET_WIDGET ("account_liststore")), &iter,
			    ACCOUNT_TYPE_COLUMN, ITEM_TYPE_SEPARATOR,
			    ACCOUNT_SENSITIVE_COLUMN, TRUE,
			    -1);

	gtk_list_store_append (GTK_LIST_STORE (GET_WIDGET ("account_liststore")), &iter);
	gtk_list_store_set (GTK_LIST_STORE (GET_WIDGET ("account_liststore")), &iter,
			    ACCOUNT_DATA_COLUMN, NULL,
			    ACCOUNT_TYPE_COLUMN, ITEM_TYPE_COMMAND,
			    ACCOUNT_ICON_COLUMN, GTK_STOCK_EDIT,
			    ACCOUNT_NAME_COLUMN, _("Edit Accounts..."),
			    ACCOUNT_SENSITIVE_COLUMN, TRUE,
			    -1);

	gtk_combo_box_set_active (GTK_COMBO_BOX (GET_WIDGET ("account_combobox")), current_account);

	update_album_list (data);

	/**/

	gth_task_dialog (GTH_TASK (data->conn), TRUE); /* FIXME */

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


static void challange_account_dialog (DialogData *data);


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


static void
connect_to_server (DialogData *data)
{
	if (data->conn == NULL) {
		data->conn = google_connection_new (GOOGLE_SERVICE_PICASA_WEB_ALBUM);
		data->progress_dialog = gth_progress_dialog_new (GTK_WINDOW (data->browser));
		gth_progress_dialog_add_task (GTH_PROGRESS_DIALOG (data->progress_dialog), GTH_TASK (data->conn));
	}

#ifdef HAVE_GNOME_KEYRING
	if (data->password == NULL) {
		if (gnome_keyring_is_available ()) {
			gnome_keyring_find_password_sync (GNOME_KEYRING_NETWORK_PASSWORD,
							  &data->password,
							  "user", data->email,
							  "server", "picasaweb.google.com",
							  "protocol", "http",
							  NULL);
		}
	}
#endif

	google_connection_connect (data->conn,
				   data->email,
				   data->password,
				   data->challange,
				   data->cancellable,
				   connection_ready_cb,
				   data);
}


static void
challange_account_dialog_response_cb (GtkDialog *dialog,
				      int        response_id,
				      gpointer   user_data)
{
	DialogData *data = user_data;

	switch (response_id) {
	case GTK_RESPONSE_HELP:
		show_help_dialog (GTK_WINDOW (dialog), "challange-picasaweb-account");
		break;

	case GTK_RESPONSE_DELETE_EVENT:
	case GTK_RESPONSE_CANCEL:
		gtk_widget_destroy (GTK_WIDGET (dialog));
		gtk_widget_destroy (data->dialog);
		break;

	case GTK_RESPONSE_OK:
		g_free (data->email);
		g_free (data->password);
		data->email = g_strdup (gth_account_properties_dialog_get_email (GTH_ACCOUNT_PROPERTIES_DIALOG (dialog)));
		data->password = g_strdup (gth_account_properties_dialog_get_password (GTH_ACCOUNT_PROPERTIES_DIALOG (dialog)));
		data->challange = g_strdup (gth_account_properties_dialog_get_challange (GTH_ACCOUNT_PROPERTIES_DIALOG (dialog)));
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

	dialog = gth_account_properties_dialog_new (data->email, data->password, google_connection_get_challange_url (data->conn));
	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (challange_account_dialog_response_cb),
			  data);

	gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (data->browser));
	gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
	gtk_window_present (GTK_WINDOW (dialog));
}


static void
new_account_dialog_response_cb (GtkDialog *dialog,
				int        response_id,
				gpointer   user_data)
{
	DialogData *data = user_data;

	switch (response_id) {
	case GTK_RESPONSE_HELP:
		show_help_dialog (GTK_WINDOW (dialog), "new-picasaweb-account");
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
		data->email = g_strdup (gth_account_properties_dialog_get_email (GTH_ACCOUNT_PROPERTIES_DIALOG (dialog)));
		data->password = g_strdup (gth_account_properties_dialog_get_password (GTH_ACCOUNT_PROPERTIES_DIALOG (dialog)));
		data->challange = NULL;
		gtk_widget_destroy (GTK_WIDGET (dialog));
		connect_to_server (data);
		break;

	default:
		break;
	}
}


static void
new_account_dialog (DialogData *data)
{
	GtkWidget *dialog;

	dialog = gth_account_properties_dialog_new (NULL, NULL, NULL);
	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (new_account_dialog_response_cb),
			  data);

	gtk_window_set_title (GTK_WINDOW (dialog), _("New Account"));
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
		data->email = g_strdup (gth_account_chooser_dialog_get_active (GTH_ACCOUNT_CHOOSER_DIALOG (dialog)));
		if (data->email != NULL) {
			gtk_widget_destroy (GTK_WIDGET (dialog));
			connect_to_server (data);
		}
		break;

	case GTH_ACCOUNT_CHOOSER_RESPONSE_NEW:
		gtk_widget_destroy (GTK_WIDGET (dialog));
		new_account_dialog (data);
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
	case GTK_RESPONSE_CANCEL:
		gtk_widget_destroy (GTK_WIDGET (dialog));
		break;

	case GTK_RESPONSE_OK:
		{
			PicasaWebAlbum *album;

			album = picasa_web_album_new ();
			picasa_web_album_set_title (album, gth_album_properties_dialog_get_name (GTH_ALBUM_PROPERTIES_DIALOG (dialog)));
			album->access = gth_album_properties_dialog_get_access (GTH_ALBUM_PROPERTIES_DIALOG (dialog));
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

	dialog = gth_album_properties_dialog_new (NULL, PICASA_WEB_ACCESS_PUBLIC);  /* FIXME: use the current catalog/folder name as default value */
	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (new_album_dialog_response_cb),
			  data);

	gtk_window_set_title (GTK_WINDOW (dialog), _("New Album"));
	gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (data->browser));
	gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
	gtk_window_present (GTK_WINDOW (dialog));

}


void
dlg_export_to_picasaweb (GthBrowser *browser)
{
	DialogData *data;

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

	data->accounts = picasa_web_accounts_load_from_file ();
	if (data->accounts != NULL) {
		if (data->accounts->next != NULL) { /* FIXME: == */
			data->email = g_strdup ((char *)data->accounts->data);
			connect_to_server (data);
		}
		else {
			GtkWidget *dialog;

			dialog = gth_account_chooser_dialog_new (data->accounts);
			g_signal_connect (dialog,
					  "response",
					  G_CALLBACK (account_chooser_dialog_response_cb),
					  data);

			gtk_window_set_title (GTK_WINDOW (dialog), _("Choose Account"));
			gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (browser));
			gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
			gtk_window_present (GTK_WINDOW (dialog));
		}
	}
	else
		new_account_dialog (data);
}
