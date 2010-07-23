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
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include <glib.h>
#ifdef HAVE_GNOME_KEYRING
#include <gnome-keyring.h>
#endif /* HAVE_GNOME_KEYRING */
#include "flickr-account-chooser-dialog.h"
#include "flickr-account-manager-dialog.h"
#include "flickr-authentication.h"
#include "flickr-user.h"
#include "flickr-service.h"


#define FLICKR_AUTHENTICATION_RESPONSE_CHOOSE_ACCOUNT 2


/* Signals */
enum {
	READY,
	ACCOUNTS_CHANGED,
	LAST_SIGNAL
};

struct _FlickrAuthenticationPrivate
{
	FlickrConnection *conn;
	FlickrService    *service;
	GCancellable     *cancellable;
	GList            *accounts;
	FlickrAccount    *account;
	GtkWidget        *browser;
	GtkWidget        *dialog;
};


static GObjectClass *parent_class = NULL;
static guint flickr_authentication_signals[LAST_SIGNAL] = { 0 };


static void
flickr_authentication_finalize (GObject *object)
{
	FlickrAuthentication *self;

	self = FLICKR_AUTHENTICATION (object);
	_g_object_unref (self->priv->conn);
	_g_object_unref (self->priv->service);
	_g_object_unref (self->priv->cancellable);
	_g_object_list_unref (self->priv->accounts);
	_g_object_unref (self->priv->account);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
flickr_authentication_class_init (FlickrAuthenticationClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	g_type_class_add_private (class, sizeof (FlickrAuthenticationPrivate));

	object_class = (GObjectClass*) class;
	object_class->finalize = flickr_authentication_finalize;

	/* signals */

	flickr_authentication_signals[READY] =
		g_signal_new ("ready",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (FlickrAuthenticationClass, ready),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE,
			      1,
			      G_TYPE_OBJECT);
	flickr_authentication_signals[ACCOUNTS_CHANGED] =
		g_signal_new ("accounts_changed",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (FlickrAuthenticationClass, accounts_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
}


static void
flickr_authentication_init (FlickrAuthentication *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, FLICKR_TYPE_AUTHENTICATION, FlickrAuthenticationPrivate);
	self->priv->conn = NULL;
	self->priv->accounts = NULL;
	self->priv->account = NULL;
}


GType
flickr_authentication_get_type (void)
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (GthTaskClass),
			NULL,
			NULL,
			(GClassInitFunc) flickr_authentication_class_init,
			NULL,
			NULL,
			sizeof (GthTask),
			0,
			(GInstanceInitFunc) flickr_authentication_init
		};

		type = g_type_register_static (G_TYPE_OBJECT,
					       "FlickrAuthentication",
					       &type_info,
					       0);
	}

	return type;
}


FlickrAuthentication *
flickr_authentication_new (FlickrConnection *conn,
			   FlickrService    *service,
			   GCancellable     *cancellable,
			   GtkWidget        *browser,
			   GtkWidget        *dialog)
{
	FlickrAuthentication *self;

	g_return_val_if_fail (conn != NULL, NULL);

	self = (FlickrAuthentication *) g_object_new (FLICKR_TYPE_AUTHENTICATION, NULL);
	self->priv->conn = g_object_ref (conn);
	self->priv->service = g_object_ref (service);
	self->priv->cancellable = _g_object_ref (cancellable);
	self->priv->accounts = flickr_accounts_load_from_file ();
	self->priv->account = flickr_accounts_find_default (self->priv->accounts);
	self->priv->browser = browser;
	self->priv->dialog = dialog;

	return self;
}


static void show_choose_account_dialog (FlickrAuthentication *self);


static void
authentication_error_dialog_response_cb (GtkDialog *dialog,
					 int        response_id,
					 gpointer   user_data)
{
	FlickrAuthentication *self = user_data;

	switch (response_id) {
	case GTK_RESPONSE_DELETE_EVENT:
	case GTK_RESPONSE_CANCEL:
		gtk_widget_destroy (GTK_WIDGET (dialog));
		gtk_dialog_response (GTK_DIALOG (self->priv->dialog), GTK_RESPONSE_DELETE_EVENT);
		break;

	case FLICKR_AUTHENTICATION_RESPONSE_CHOOSE_ACCOUNT:
		gtk_widget_destroy (GTK_WIDGET (dialog));
		show_choose_account_dialog (self);
		break;

	default:
		break;
	}
}


static void start_authorization_process (FlickrAuthentication *self);


static void
show_authentication_error_dialog (FlickrAuthentication  *self,
				  GError               **error)
{
	GtkWidget *dialog;

	if (g_error_matches (*error, FLICKR_CONNECTION_ERROR, FLICKR_CONNECTION_ERROR_INVALID_TOKEN)) {
		start_authorization_process (self);
		return;
	}

	dialog = _gtk_message_dialog_new (GTK_WINDOW (self->priv->browser),
			             GTK_DIALOG_MODAL,
				     GTK_STOCK_DIALOG_ERROR,
				     _("Could not connect to the server"),
				     (*error)->message,
				     _("Choose _Account..."), FLICKR_AUTHENTICATION_RESPONSE_CHOOSE_ACCOUNT,
				     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				     NULL);
	if (self->priv->conn != NULL)
		gth_task_dialog (GTH_TASK (self->priv->conn), TRUE, dialog);

	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (authentication_error_dialog_response_cb),
			  self);
	gtk_widget_show (dialog);

	g_clear_error (error);
}


/* -- flickr_authentication_auto_connect -- */


static void
upload_status_ready_cb (GObject      *source_object,
			GAsyncResult *res,
			gpointer      user_data)
{
	FlickrAuthentication *self = user_data;
	FlickrUser           *user;
	GError               *error = NULL;

	user = flickr_service_get_upload_status_finish (FLICKR_SERVICE (source_object), res, &error);
	if (error != NULL) {
		show_authentication_error_dialog (self, &error);
		return;
	}
	flickr_accounts_save_to_file (self->priv->accounts, self->priv->account);

	g_signal_emit (self, flickr_authentication_signals[READY], 0, user);

	g_object_unref (user);
}


static void
connect_to_server_step2 (FlickrAuthentication *self)
{
	if (self->priv->account->token == NULL) {
		start_authorization_process (self);
		return;
	}
	flickr_connection_set_auth_token (self->priv->conn, self->priv->account->token);
	flickr_service_get_upload_status (self->priv->service,
					  self->priv->cancellable,
					  upload_status_ready_cb,
					  self);
}


#ifdef HAVE_GNOME_KEYRING
static void
find_password_cb (GnomeKeyringResult  result,
                  const char         *string,
                  gpointer            user_data)
{
	FlickrAuthentication *self = user_data;

	if (string != NULL)
		self->priv->account->token = g_strdup (string);
	connect_to_server_step2 (self);
}
#endif


static void
connect_to_server (FlickrAuthentication *self)
{
	g_return_if_fail (self->priv->account != NULL);

#ifdef HAVE_GNOME_KEYRING
	if ((self->priv->account->token == NULL) && gnome_keyring_is_available ()) {
		gnome_keyring_find_password (GNOME_KEYRING_NETWORK_PASSWORD,
					     find_password_cb,
					     self,
					     NULL,
					     "user", self->priv->account->username,
					     "server", self->priv->conn->server->url,
					     "protocol", "http",
					     NULL);
		return;
	}
#endif

	connect_to_server_step2 (self);
}


static void
set_account (FlickrAuthentication *self,
	     FlickrAccount        *account)
{
	GList *link;

	link = g_list_find_custom (self->priv->accounts, self->priv->account, (GCompareFunc) flickr_account_cmp);
	if (link != NULL) {
		self->priv->accounts = g_list_remove_link (self->priv->accounts, link);
		_g_object_list_unref (link);
	}

	_g_object_unref (self->priv->account);
	self->priv->account = NULL;

	if (account != NULL) {
		self->priv->account = g_object_ref (account);
		self->priv->accounts = g_list_prepend (self->priv->accounts, g_object_ref (self->priv->account));
	}
}


#ifdef HAVE_GNOME_KEYRING
static void
store_password_done_cb (GnomeKeyringResult result,
			gpointer           user_data)
{
	FlickrAuthentication *self = user_data;
	connect_to_server (self);
}
#endif


static void
connection_token_ready_cb (GObject      *source_object,
			   GAsyncResult *res,
			   gpointer      user_data)
{
	FlickrAuthentication *self = user_data;
	GError               *error = NULL;
	FlickrAccount        *account;

	if (! flickr_connection_get_token_finish (FLICKR_CONNECTION (source_object), res, &error)) {
		show_authentication_error_dialog (self, &error);
		return;
	}

	account = flickr_account_new ();
	flickr_account_set_username (account, flickr_connection_get_username (self->priv->conn));
	flickr_account_set_token (account, flickr_connection_get_auth_token (self->priv->conn));
	set_account (self, account);
	g_object_unref (account);

#ifdef HAVE_GNOME_KEYRING
	if (gnome_keyring_is_available ()) {
		gnome_keyring_store_password (GNOME_KEYRING_NETWORK_PASSWORD,
					      NULL,
					      self->priv->conn->server->name,
					      flickr_connection_get_auth_token (self->priv->conn),
					      store_password_done_cb,
					      self,
					      NULL,
					      "user", flickr_connection_get_username (self->priv->conn),
					      "server", self->priv->conn->server->url,
					      "protocol", "http",
					      NULL);
		return;
	}
#endif

	connect_to_server (self);
}


static void
complete_authorization_messagedialog_response_cb (GtkDialog *dialog,
						  int        response_id,
						  gpointer   user_data)
{
	FlickrAuthentication *self = user_data;

	switch (response_id) {
	case GTK_RESPONSE_HELP:
		show_help_dialog (GTK_WINDOW (dialog), "flicker-complete-authorization");
		break;

	case GTK_RESPONSE_DELETE_EVENT:
	case GTK_RESPONSE_CANCEL:
		gtk_widget_destroy (GTK_WIDGET (dialog));
		gtk_dialog_response (GTK_DIALOG (self->priv->dialog), GTK_RESPONSE_DELETE_EVENT);
		break;

	case GTK_RESPONSE_OK:
		gtk_widget_destroy (GTK_WIDGET (dialog));
		gth_task_dialog (GTH_TASK (self->priv->conn), FALSE, NULL);
		flickr_connection_get_token (self->priv->conn,
					     self->priv->cancellable,
					     connection_token_ready_cb,
					     self);
		break;

	default:
		break;
	}
}


static void
complete_authorization (FlickrAuthentication *self)
{
	GtkBuilder *builder;
	GtkWidget  *dialog;
	char       *text;
	char       *secondary_text;

	gth_task_dialog (GTH_TASK (self->priv->conn), TRUE, NULL);

	builder = _gtk_builder_new_from_file ("flicker-complete-authorization.ui", "flicker");
	dialog = _gtk_builder_get_widget (builder, "complete_authorization_messagedialog");
	text = g_strdup_printf (_("Return to this window when you have finished the authorization process on %s"), self->priv->conn->server->name);
	secondary_text = g_strdup (_("Once you're done, click the 'Continue' button below."));
	g_object_set (dialog, "text", text, "secondary-text", secondary_text, NULL);
	g_object_set_data_full (G_OBJECT (dialog), "builder", builder, g_object_unref);
	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (complete_authorization_messagedialog_response_cb),
			  self);

	if (gtk_widget_get_visible (self->priv->dialog))
		gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (self->priv->dialog));
	else
		gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (self->priv->browser));
	gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
	gtk_window_present (GTK_WINDOW (dialog));

	g_free (secondary_text);
	g_free (text);
}


static void
ask_authorization_messagedialog_response_cb (GtkDialog *dialog,
					     int        response_id,
					     gpointer   user_data)
{
	FlickrAuthentication *self = user_data;

	switch (response_id) {
	case GTK_RESPONSE_HELP:
		show_help_dialog (GTK_WINDOW (dialog), "flicker-ask-authorization");
		break;

	case GTK_RESPONSE_DELETE_EVENT:
	case GTK_RESPONSE_CANCEL:
		gtk_widget_destroy (GTK_WIDGET (dialog));
		gtk_dialog_response (GTK_DIALOG (self->priv->dialog), GTK_RESPONSE_DELETE_EVENT);
		break;

	case GTK_RESPONSE_OK:
		{
			GdkScreen *screen;
			char      *url;
			GError    *error = NULL;

			screen = gtk_widget_get_screen (GTK_WIDGET (dialog));
			gtk_widget_destroy (GTK_WIDGET (dialog));

			url = flickr_connection_get_login_link (self->priv->conn, FLICKR_ACCESS_WRITE);
			if (gtk_show_uri (screen, url, 0, &error))
				complete_authorization (self);
			else
				show_authentication_error_dialog (self, &error);

			g_free (url);
		}
		break;

	default:
		break;
	}
}


static void
ask_authorization (FlickrAuthentication *self)
{
	GtkBuilder *builder;
	GtkWidget  *dialog;
	char       *text;
	char       *secondary_text;

	gth_task_dialog (GTH_TASK (self->priv->conn), TRUE, NULL);

	builder = _gtk_builder_new_from_file ("flicker-ask-authorization.ui", "flicker");
	dialog = _gtk_builder_get_widget (builder, "ask_authorization_messagedialog");
	text = g_strdup_printf (_("gthumb requires your authorization to upload the photos to %s"), self->priv->conn->server->name);
	secondary_text = g_strdup_printf (_("Click 'Authorize' to open your web browser and authorize gthumb to upload photos to %s. When you're finished, return to this window to complete the authorization."), self->priv->conn->server->name);
	g_object_set (dialog, "text", text, "secondary-text", secondary_text, NULL);
	g_object_set_data_full (G_OBJECT (dialog), "builder", builder, g_object_unref);
	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (ask_authorization_messagedialog_response_cb),
			  self);

	gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
	if (gtk_widget_get_visible (self->priv->dialog))
		gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (self->priv->dialog));
	else
		gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (self->priv->browser));
	gtk_window_present (GTK_WINDOW (dialog));

	g_free (secondary_text);
	g_free (text);
}


static void
connection_frob_ready_cb (GObject      *source_object,
			  GAsyncResult *res,
			  gpointer      user_data)
{
	FlickrAuthentication *self = user_data;
	GError               *error = NULL;

	if (! flickr_connection_get_frob_finish (FLICKR_CONNECTION (source_object), res, &error))
		show_authentication_error_dialog (self, &error);
	else
		ask_authorization (self);
}


static void
start_authorization_process (FlickrAuthentication *self)
{
	flickr_connection_get_frob (self->priv->conn,
				    self->priv->cancellable,
				    connection_frob_ready_cb,
				    self);
}


static void
account_chooser_dialog_response_cb (GtkDialog *dialog,
				    int        response_id,
				    gpointer   user_data)
{
	FlickrAuthentication *self = user_data;

	switch (response_id) {
	case GTK_RESPONSE_DELETE_EVENT:
	case GTK_RESPONSE_CANCEL:
		gtk_widget_destroy (GTK_WIDGET (dialog));
		gtk_dialog_response (GTK_DIALOG (self->priv->dialog), GTK_RESPONSE_DELETE_EVENT);
		break;

	case GTK_RESPONSE_OK:
		_g_object_unref (self->priv->account);
		self->priv->account = flickr_account_chooser_dialog_get_active (FLICKR_ACCOUNT_CHOOSER_DIALOG (dialog));
		if (self->priv->account != NULL) {
			gtk_widget_destroy (GTK_WIDGET (dialog));
			connect_to_server (self);
		}

		break;

	case FLICKR_ACCOUNT_CHOOSER_RESPONSE_NEW:
		gtk_widget_destroy (GTK_WIDGET (dialog));
		start_authorization_process (self);
		break;

	default:
		break;
	}
}


static void
show_choose_account_dialog (FlickrAuthentication *self)
{
	GtkWidget *dialog;

	gth_task_dialog (GTH_TASK (self->priv->conn), TRUE, NULL);
	dialog = flickr_account_chooser_dialog_new (self->priv->accounts, self->priv->account);
	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (account_chooser_dialog_response_cb),
			  self);

	gtk_window_set_title (GTK_WINDOW (dialog), _("Choose Account"));
	gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (self->priv->browser));
	gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
	gtk_window_present (GTK_WINDOW (dialog));
}


void
flickr_authentication_auto_connect (FlickrAuthentication *self)
{
	gtk_widget_hide (self->priv->dialog);
	gth_task_dialog (GTH_TASK (self->priv->conn), FALSE, NULL);

	if (self->priv->accounts != NULL) {
		if (self->priv->account != NULL) {
			connect_to_server (self);
		}
		else if (self->priv->accounts->next == NULL) {
			self->priv->account = g_object_ref (self->priv->accounts->data);
			connect_to_server (self);
		}
		else
			show_choose_account_dialog (self);
	}
	else
		start_authorization_process (self);
}


void
flickr_authentication_connect (FlickrAuthentication *self,
			       FlickrAccount        *account)
{
	set_account (self, account);
	flickr_authentication_auto_connect (self);
}


FlickrAccount *
flickr_authentication_get_account (FlickrAuthentication *self)
{
	return self->priv->account;
}


GList *
flickr_authentication_get_accounts (FlickrAuthentication *self)
{
	return self->priv->accounts;
}


/* -- flickr_authentication_edit_accounts -- */


static void
account_manager_dialog_response_cb (GtkDialog *dialog,
			            int        response_id,
			            gpointer   user_data)
{
	FlickrAuthentication *self = user_data;

	switch (response_id) {
	case GTK_RESPONSE_DELETE_EVENT:
	case GTK_RESPONSE_CANCEL:
		gtk_widget_destroy (GTK_WIDGET (dialog));
		break;

	case GTK_RESPONSE_OK:
		_g_object_list_unref (self->priv->accounts);
		self->priv->accounts = flickr_account_manager_dialog_get_accounts (FLICKR_ACCOUNT_MANAGER_DIALOG (dialog));
		if (! g_list_find_custom (self->priv->accounts, self->priv->account, (GCompareFunc) flickr_account_cmp)) {
			_g_object_unref (self->priv->account);
			self->priv->account = NULL;
			flickr_authentication_auto_connect (self);
		}
		else
			g_signal_emit (self, flickr_authentication_signals[ACCOUNTS_CHANGED], 0);
		flickr_accounts_save_to_file (self->priv->accounts, self->priv->account);
		gtk_widget_destroy (GTK_WIDGET (dialog));
		break;

	case FLICKR_ACCOUNT_MANAGER_RESPONSE_NEW:
		gtk_widget_destroy (GTK_WIDGET (dialog));
		start_authorization_process (self);
		break;

	default:
		break;
	}
}


void
flickr_authentication_edit_accounts (FlickrAuthentication *self,
				     GtkWindow            *parent)
{
	GtkWidget  *dialog;

	dialog = flickr_account_manager_dialog_new (self->priv->accounts);
	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (account_manager_dialog_response_cb),
			  self);

	gtk_window_set_title (GTK_WINDOW (dialog), _("Edit Accounts"));
	gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (self->priv->dialog));
	gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
	gtk_window_present (GTK_WINDOW (dialog));
}
