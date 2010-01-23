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
	GList            *albums;
	GoogleConnection *conn;
	PicasaWebService *picasaweb;
	GCancellable     *cancellable;
} DialogData;


static void
export_dialog_destroy_cb (GtkWidget  *widget,
			  DialogData *data)
{
	_g_object_unref (data->cancellable);
	_g_object_unref (data->picasaweb);
	_g_object_unref (data->conn);
	_g_object_list_unref (data->albums);
	g_free (data->password);
	g_free (data->email);
	_g_string_list_free (data->accounts);
	g_object_unref (data->builder);
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

	case GTK_RESPONSE_CANCEL:
		gtk_widget_destroy (data->dialog);
		break;

	case GTK_RESPONSE_OK:
		break;

	default:
		break;
	}
}


static void
show_export_dialog (DialogData *data)
{
	/* FIXME: update widgets data */

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
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (data->browser), _("Could not connect to the server"), &error);
		gtk_widget_destroy (data->dialog);
		return;
	}

	show_export_dialog (data);
}


static void
connection_ready_cb (GObject      *source_object,
		     GAsyncResult *result,
		     gpointer      user_data)
{
	DialogData       *data = user_data;
	GoogleConnection *conn = GOOGLE_CONNECTION (source_object);
	GError           *error;

	if (! google_connection_connect_finish (conn, result, &error)) {
		if (g_error_matches (error, GOOGLE_CONNECTION_ERROR, GOOGLE_CONNECTION_ERROR_CAPTCHA_REQUIRED)) {
			/* FIXME: request to insert the captcha text */
		}
		else {
			_gtk_error_dialog_from_gerror_show (GTK_WINDOW (data->browser), _("Could not connect to the server"), &error);
			gtk_widget_destroy (data->dialog);
		}
		return;
	}

	if (data->picasaweb == NULL)
		data->picasaweb = picasa_web_service_new (conn);
	picasa_web_service_list_albums (data->picasaweb,
				        "default",
				        data->cancellable,
				        list_albums_ready_cb,
				        data);
}


static void
connect_to_server (DialogData *data)
{
	if (data->conn == NULL) {
		data->conn = google_connection_new (GOOGLE_SERVICE_PICASA_WEB_ALBUM);
		data->progress_dialog = gth_progress_dialog_new (GTK_WINDOW (data->browser));
		gth_progress_dialog_add_task (GTH_PROGRESS_DIALOG (data->progress_dialog), GTH_TASK (data->conn));
	}

	google_connection_connect (data->conn,
				   data->email,
				   data->password,
				   NULL,
				   data->cancellable,
				   connection_ready_cb,
				   data);
}


#ifdef HAVE_GNOME_KEYRING
static void
store_password_done_cb (GnomeKeyringResult result,
			gpointer           user_data)
{
	connect_to_server ((DialogData *) user_data);
}
#endif


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

	case GTK_RESPONSE_CANCEL:
		gtk_widget_destroy (GTK_WIDGET (dialog));
		gtk_widget_destroy (data->dialog);
		break;

	case GTK_RESPONSE_OK:
		{
			g_free (data->email);
			g_free (data->password);
			data->email = g_strdup (gth_account_properties_dialog_get_email (GTH_ACCOUNT_PROPERTIES_DIALOG (dialog)));
			data->password = g_strdup (gth_account_properties_dialog_get_password (GTH_ACCOUNT_PROPERTIES_DIALOG (dialog)));
			data->accounts = g_list_append (data->accounts, g_strdup (data->email));
#ifdef HAVE_GNOME_KEYRING
			if (gnome_keyring_is_available ())
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
			else
#endif
				connect_to_server (data);
		}
		break;

	default:
		break;
	}
}


static void
new_account_dialog (DialogData *data)
{
	GtkWidget *dialog;

	dialog = gth_account_properties_dialog_new (NULL, NULL);
	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (new_account_dialog_response_cb),
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
	case GTK_RESPONSE_CANCEL:
		gtk_widget_destroy (GTK_WIDGET (dialog));
		gtk_widget_destroy (data->dialog);
		break;

	case GTK_RESPONSE_OK:
		g_free (data->password);
		data->password = NULL;
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


static gboolean
album_combobox_row_separator_func (GtkTreeModel *model,
				   GtkTreeIter  *iter,
				   gpointer      data)
{
	int item_type;

	gtk_tree_model_get (model, iter, ALBUM_TYPE_COLUMN, &item_type, -1);

	return item_type == ITEM_TYPE_SEPARATOR;
}


static GList *
picasaweb_accounts_read_from_file (void)
{
	GList       *accounts = NULL;
	char        *filename;
	char        *buffer;
	gsize        len;
	GError      *error;
	DomDocument *doc;

	filename = gth_user_dir_get_file (GTH_DIR_CONFIG, GTHUMB_DIR, "accounts", "picasaweb.xml", NULL);
	g_file_get_contents (filename, &buffer, &len, &error);

	doc = dom_document_new ();
	if (dom_document_load (doc, buffer, len, &error)) {
		DomElement *node;

		node = DOM_ELEMENT (doc)->first_child;
		if ((node != NULL) && (g_strcmp0 (node->tag_name, "accounts") == 0)) {
			DomElement *child;

			for (child = node->first_child;
			     child != NULL;
			     child = child->next_sibling)
			{
				if (strcmp (child->tag_name, "account") == 0) {
					const char *value;

					value = dom_element_get_attribute (child, "email");
					if (value != NULL)
						accounts = g_list_prepend (accounts, g_strdup (value));
				}
			}

			accounts = g_list_reverse (accounts);
		}
	}

	g_object_unref (doc);
	g_free (buffer);
	g_free (filename);

	return accounts;
}


void
dlg_export_to_picasaweb (GthBrowser *browser)
{
	DialogData  *data;
	GtkTreeIter  iter;

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
	gtk_combo_box_set_row_separator_func (GTK_COMBO_BOX (GET_WIDGET ("album_combobox")),
					      album_combobox_row_separator_func,
					      data,
					      NULL);

	/* Account */

	data->accounts = picasaweb_accounts_read_from_file ();

	/*
	gtk_list_store_append (GTK_LIST_STORE (GET_WIDGET ("account_liststore")), &iter);
	gtk_list_store_set (GTK_LIST_STORE (GET_WIDGET ("account_liststore")), &iter,
			    ACCOUNT_DATA_COLUMN, NULL,
			    ACCOUNT_TYPE_COLUMN, ITEM_TYPE_ENTRY,
			    ACCOUNT_NAME_COLUMN, "",
			    ACCOUNT_SENSITIVE_COLUMN, FALSE,
			    -1);

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

	gtk_combo_box_set_active (GTK_COMBO_BOX (GET_WIDGET ("account_combobox")), 0);
	*/

	/* Album */

	/*
	gtk_list_store_append (GTK_LIST_STORE (GET_WIDGET ("album_liststore")), &iter);
	gtk_list_store_set (GTK_LIST_STORE (GET_WIDGET ("album_liststore")), &iter,
			    ALBUM_DATA_COLUMN, NULL,
			    ALBUM_TYPE_COLUMN, ITEM_TYPE_ENTRY,
			    ALBUM_NAME_COLUMN, _("(Empty)"),
			    ALBUM_SENSITIVE_COLUMN, FALSE,
			    -1);

	gtk_list_store_append (GTK_LIST_STORE (GET_WIDGET ("album_liststore")), &iter);
	gtk_list_store_set (GTK_LIST_STORE (GET_WIDGET ("album_liststore")), &iter,
			    ALBUM_TYPE_COLUMN, ITEM_TYPE_SEPARATOR,
			    ALBUM_SENSITIVE_COLUMN, TRUE,
			    -1);

	gtk_list_store_append (GTK_LIST_STORE (GET_WIDGET ("album_liststore")), &iter);
	gtk_list_store_set (GTK_LIST_STORE (GET_WIDGET ("album_liststore")), &iter,
			    ALBUM_DATA_COLUMN, NULL,
			    ALBUM_TYPE_COLUMN, ITEM_TYPE_COMMAND,
			    ALBUM_ICON_COLUMN, GTK_STOCK_EDIT,
			    ALBUM_NAME_COLUMN, _("Edit Albums..."),
			    ALBUM_SENSITIVE_COLUMN, TRUE,
			    -1);

	gtk_combo_box_set_active (GTK_COMBO_BOX (GET_WIDGET ("album_combobox")), 0);
	*/

	/* Set the signals handlers. */

	g_signal_connect (G_OBJECT (data->dialog),
			  "destroy",
			  G_CALLBACK (export_dialog_destroy_cb),
			  data);
	g_signal_connect (data->dialog,
			  "response",
			  G_CALLBACK (export_dialog_response_cb),
			  data);

	if (data->accounts != NULL) {
		if (data->accounts->next == NULL) {
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

			gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (browser));
			gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
			gtk_window_present (GTK_WINDOW (dialog));
		}
	}
	else
		new_account_dialog (data);
}
