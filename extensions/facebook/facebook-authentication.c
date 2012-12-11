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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <glib.h>
#ifdef HAVE_LIBSECRET
#include <libsecret/secret.h>
#endif /* HAVE_LIBSECRET */
#include <extensions/oauth/oauth2-ask-authorization-dialog.h>
#include "facebook-account-chooser-dialog.h"
#include "facebook-account-manager-dialog.h"
#include "facebook-authentication.h"
#include "facebook-user.h"
#include "facebook-service.h"


#define FACEBOOK_AUTHENTICATION_RESPONSE_CHOOSE_ACCOUNT 2


/* Signals */
enum {
	READY,
	ACCOUNTS_CHANGED,
	LAST_SIGNAL
};

struct _FacebookAuthenticationPrivate
{
	FacebookConnection *conn;
	FacebookService    *service;
	GCancellable       *cancellable;
	GList              *accounts;
	FacebookAccount    *account;
	GtkWidget          *browser;
	GtkWidget          *dialog;
};


static guint facebook_authentication_signals[LAST_SIGNAL] = { 0 };


G_DEFINE_TYPE (FacebookAuthentication, facebook_authentication, G_TYPE_OBJECT)


static void
facebook_authentication_finalize (GObject *object)
{
	FacebookAuthentication *self;

	self = FACEBOOK_AUTHENTICATION (object);
	_g_object_unref (self->priv->conn);
	_g_object_unref (self->priv->service);
	_g_object_unref (self->priv->cancellable);
	_g_object_list_unref (self->priv->accounts);
	_g_object_unref (self->priv->account);

	G_OBJECT_CLASS (facebook_authentication_parent_class)->finalize (object);
}


static void
facebook_authentication_class_init (FacebookAuthenticationClass *class)
{
	GObjectClass *object_class;

	g_type_class_add_private (class, sizeof (FacebookAuthenticationPrivate));

	object_class = (GObjectClass*) class;
	object_class->finalize = facebook_authentication_finalize;

	/* signals */

	facebook_authentication_signals[READY] =
		g_signal_new ("ready",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (FacebookAuthenticationClass, ready),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
	facebook_authentication_signals[ACCOUNTS_CHANGED] =
		g_signal_new ("accounts_changed",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (FacebookAuthenticationClass, accounts_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
}


static void
facebook_authentication_init (FacebookAuthentication *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, FACEBOOK_TYPE_AUTHENTICATION, FacebookAuthenticationPrivate);
	self->priv->conn = NULL;
	self->priv->accounts = NULL;
	self->priv->account = NULL;
}


FacebookAuthentication *
facebook_authentication_new (FacebookConnection *conn,
			     FacebookService    *service,
			     GCancellable       *cancellable,
			     GtkWidget          *browser,
			     GtkWidget          *dialog)
{
	FacebookAuthentication *self;

	g_return_val_if_fail (conn != NULL, NULL);

	self = (FacebookAuthentication *) g_object_new (FACEBOOK_TYPE_AUTHENTICATION, NULL);
	self->priv->conn = g_object_ref (conn);
	self->priv->service = g_object_ref (service);
	self->priv->cancellable = _g_object_ref (cancellable);
	self->priv->accounts = facebook_accounts_load_from_file ();
	self->priv->account = facebook_accounts_find_default (self->priv->accounts);
	self->priv->browser = browser;
	self->priv->dialog = dialog;

	return self;
}


static void show_choose_account_dialog (FacebookAuthentication *self);


static void
authentication_error_dialog_response_cb (GtkDialog *dialog,
					 int        response_id,
					 gpointer   user_data)
{
	FacebookAuthentication *self = user_data;

	switch (response_id) {
	case GTK_RESPONSE_DELETE_EVENT:
	case GTK_RESPONSE_CANCEL:
		gtk_widget_destroy (GTK_WIDGET (dialog));
		gtk_dialog_response (GTK_DIALOG (self->priv->dialog), GTK_RESPONSE_DELETE_EVENT);
		break;

	case FACEBOOK_AUTHENTICATION_RESPONSE_CHOOSE_ACCOUNT:
		gtk_widget_destroy (GTK_WIDGET (dialog));
		show_choose_account_dialog (self);
		break;

	default:
		break;
	}
}


static void start_authorization_process (FacebookAuthentication *self);


static void
show_authentication_error_dialog (FacebookAuthentication  *self,
				  GError                 **error)
{
	GtkWidget *dialog;

	if (g_error_matches (*error, FACEBOOK_CONNECTION_ERROR, FACEBOOK_CONNECTION_ERROR_TOKEN_EXPIRED)) {
		start_authorization_process (self);
		return;
	}

	dialog = _gtk_message_dialog_new (GTK_WINDOW (self->priv->browser),
			             GTK_DIALOG_MODAL,
				     GTK_STOCK_DIALOG_ERROR,
				     _("Could not connect to the server"),
				     (*error)->message,
				     _("Choose _Account..."), FACEBOOK_AUTHENTICATION_RESPONSE_CHOOSE_ACCOUNT,
				     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				     NULL);
	if (self->priv->conn != NULL)
		gth_task_dialog (GTH_TASK (self->priv->conn), TRUE, dialog);

	g_signal_connect (dialog,
			  "delete-event",
			  G_CALLBACK (gtk_true),
			  NULL);
	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (authentication_error_dialog_response_cb),
			  self);
	gtk_widget_show (dialog);

	g_clear_error (error);
}


static void
set_current_account (FacebookAuthentication *self,
		     FacebookAccount        *account)
{
	GList *link;

	link = g_list_find_custom (self->priv->accounts, self->priv->account, (GCompareFunc) facebook_account_cmp);
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


/* -- facebook_authentication_auto_connect -- */


static void
facebook_authentication_account_ready (FacebookAuthentication *self)
{
	g_signal_emit (self, facebook_authentication_signals[READY], 0);
}


#ifdef HAVE_LIBSECRET
static void
password_store_ready_cb (GObject      *source_object,
			 GAsyncResult *result,
			 gpointer      user_data)
{
	FacebookAuthentication *self = user_data;

	secret_password_store_finish (result, NULL);
	facebook_authentication_account_ready (self);
}
#endif


static void
get_user_ready_cb (GObject      *source_object,
		   GAsyncResult *res,
		   gpointer      user_data)
{
	FacebookAuthentication *self = user_data;
	GError                 *error = NULL;
	FacebookUser           *user;
	FacebookAccount        *account;

	user = facebook_service_get_user_finish (self->priv->service, res, &error);
	if (user == NULL) {
		show_authentication_error_dialog (self, &error);
		return;
	}

	account = facebook_account_new ();
	facebook_account_set_username (account, user->name);
	facebook_account_set_user_id (account, user->id);
	facebook_account_set_token (account, facebook_connection_get_access_token (self->priv->conn));
	set_current_account (self, account);
	facebook_accounts_save_to_file (self->priv->accounts, self->priv->account);

#ifdef HAVE_LIBSECRET
	{
		secret_password_store (SECRET_SCHEMA_COMPAT_NETWORK,
				       NULL,
				       "Facebook",
				       account->token,
				       self->priv->cancellable,
				       password_store_ready_cb,
				       self,
				       "user", account->user_id,
				       "server", FACEBOOK_HTTP_SERVER,
				       "protocol", "https",
				       NULL);
	}
#else
	facebook_authentication_account_ready (self);
#endif

	g_object_unref (account);
	g_object_unref (user);
}


static void
connect_to_server_step2 (FacebookAuthentication *self)
{
	if (self->priv->account->token == NULL) {
		start_authorization_process (self);
		return;
	}
	facebook_connection_set_access_token (self->priv->conn, self->priv->account->token);
	facebook_service_get_user (self->priv->service,
				   self->priv->cancellable,
				   get_user_ready_cb,
				   self);
}


#ifdef HAVE_LIBSECRET
static void
password_lookup_ready_cb (GObject      *source_object,
			  GAsyncResult *result,
			  gpointer      user_data)
{
	FacebookAuthentication *self = user_data;
	char                   *secret;

	secret = secret_password_lookup_finish (result, NULL);
	if (secret != NULL) {
		facebook_account_set_token (self->priv->account, secret);
		g_free (secret);
	}

	connect_to_server_step2 (self);
}
#endif


static void
connect_to_server (FacebookAuthentication *self)
{
	g_return_if_fail (self->priv->account != NULL);
	g_return_if_fail (self->priv->account->user_id != NULL);

#ifdef HAVE_LIBSECRET
	if (self->priv->account->token == NULL) {
		secret_password_lookup (SECRET_SCHEMA_COMPAT_NETWORK,
					self->priv->cancellable,
					password_lookup_ready_cb,
					self,
					"user", self->priv->account->user_id,
					"server", FACEBOOK_HTTP_SERVER,
					"protocol", "https",
					NULL);
		return;
	}
#endif

	connect_to_server_step2 (self);
}


static void
ask_authorization_dialog_response_cb (GtkDialog *dialog,
				      int        response_id,
				      gpointer   user_data)
{
	FacebookAuthentication *self = user_data;

	switch (response_id) {
	case GTK_RESPONSE_HELP:
		show_help_dialog (GTK_WINDOW (dialog), "facebook-complete-authorization");
		break;

	case GTK_RESPONSE_DELETE_EVENT:
	case GTK_RESPONSE_CANCEL:
		gtk_widget_destroy (GTK_WIDGET (dialog));
		gtk_dialog_response (GTK_DIALOG (self->priv->dialog), GTK_RESPONSE_DELETE_EVENT);
		break;

	case GTK_RESPONSE_OK:
		gtk_widget_destroy (GTK_WIDGET (dialog));
		gth_task_dialog (GTH_TASK (self->priv->conn), FALSE, NULL);
		facebook_service_get_user (self->priv->service,
					   self->priv->cancellable,
					   get_user_ready_cb,
					   self);
		break;

	default:
		break;
	}
}


static void
ask_authorization_dialog_redirected_cb (OAuth2AskAuthorizationDialog *dialog,
					gpointer                      user_data)
{
	FacebookAuthentication *self = user_data;
	const char             *uri;

	uri = oauth2_ask_authorization_dialog_get_uri (dialog);
	if (g_str_has_prefix (uri, FACEBOOK_REDIRECT_URI)) {
		const char *uri_data;
		GHashTable *data;
		const char *access_token;

		uri_data = uri + strlen (FACEBOOK_REDIRECT_URI "#");

		data = soup_form_decode (uri_data);
		access_token = g_hash_table_lookup (data, "access_token");
		facebook_connection_set_access_token (self->priv->conn, access_token);
		gtk_dialog_response (GTK_DIALOG (dialog),
				     (access_token != NULL) ? GTK_RESPONSE_OK : GTK_RESPONSE_CANCEL);

		g_hash_table_destroy (data);
	}
}


static void
start_authorization_process (FacebookAuthentication *self)
{
	GtkWidget *dialog;

	gth_task_dialog (GTH_TASK (self->priv->conn), TRUE, NULL);

	dialog = oauth2_ask_authorization_dialog_new (_("Authorization Required"),
						      facebook_utils_get_authorization_url (FACEBOOK_ACCESS_WRITE));
	if (gtk_widget_get_visible (self->priv->dialog))
		gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (self->priv->dialog));
	else
		gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (self->priv->browser));
	gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);

	g_signal_connect (dialog,
			  "delete-event",
			  G_CALLBACK (gtk_true),
			  NULL);
	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (ask_authorization_dialog_response_cb),
			  self);
	g_signal_connect (OAUTH2_ASK_AUTHORIZATION_DIALOG (dialog),
			  "redirected",
			  G_CALLBACK (ask_authorization_dialog_redirected_cb),
			  self);

	gtk_widget_show (dialog);
}


static void
account_chooser_dialog_response_cb (GtkDialog *dialog,
				    int        response_id,
				    gpointer   user_data)
{
	FacebookAuthentication *self = user_data;

	switch (response_id) {
	case GTK_RESPONSE_DELETE_EVENT:
	case GTK_RESPONSE_CANCEL:
		gtk_widget_destroy (GTK_WIDGET (dialog));
		gtk_dialog_response (GTK_DIALOG (self->priv->dialog), GTK_RESPONSE_DELETE_EVENT);
		break;

	case GTK_RESPONSE_OK:
		_g_object_unref (self->priv->account);
		self->priv->account = facebook_account_chooser_dialog_get_active (FACEBOOK_ACCOUNT_CHOOSER_DIALOG (dialog));
		if (self->priv->account != NULL) {
			gtk_widget_destroy (GTK_WIDGET (dialog));
			connect_to_server (self);
		}
		break;

	case FACEBOOK_ACCOUNT_CHOOSER_RESPONSE_NEW:
		gtk_widget_destroy (GTK_WIDGET (dialog));
		start_authorization_process (self);
		break;

	default:
		break;
	}
}


static void
show_choose_account_dialog (FacebookAuthentication *self)
{
	GtkWidget *dialog;

	gth_task_dialog (GTH_TASK (self->priv->conn), TRUE, NULL);
	dialog = facebook_account_chooser_dialog_new (self->priv->accounts, self->priv->account);
	g_signal_connect (dialog,
			  "delete-event",
			  G_CALLBACK (gtk_true),
			  NULL);
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
facebook_authentication_auto_connect (FacebookAuthentication *self)
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
facebook_authentication_connect (FacebookAuthentication *self,
			         FacebookAccount        *account)
{
	set_current_account (self, account);
	facebook_authentication_auto_connect (self);
}


FacebookAccount *
facebook_authentication_get_account (FacebookAuthentication *self)
{
	return self->priv->account;
}


GList *
facebook_authentication_get_accounts (FacebookAuthentication *self)
{
	return self->priv->accounts;
}


/* -- facebook_authentication_edit_accounts -- */


static void
account_manager_dialog_response_cb (GtkDialog *dialog,
			            int        response_id,
			            gpointer   user_data)
{
	FacebookAuthentication *self = user_data;

	switch (response_id) {
	case GTK_RESPONSE_DELETE_EVENT:
	case GTK_RESPONSE_CANCEL:
		gtk_widget_destroy (GTK_WIDGET (dialog));
		break;

	case GTK_RESPONSE_OK:
		_g_object_list_unref (self->priv->accounts);
		self->priv->accounts = facebook_account_manager_dialog_get_accounts (FACEBOOK_ACCOUNT_MANAGER_DIALOG (dialog));
		if (! g_list_find_custom (self->priv->accounts, self->priv->account, (GCompareFunc) facebook_account_cmp)) {
			_g_object_unref (self->priv->account);
			self->priv->account = NULL;
			facebook_authentication_auto_connect (self);
		}
		else
			g_signal_emit (self, facebook_authentication_signals[ACCOUNTS_CHANGED], 0);
		facebook_accounts_save_to_file (self->priv->accounts, self->priv->account);
		gtk_widget_destroy (GTK_WIDGET (dialog));
		break;

	case FACEBOOK_ACCOUNT_MANAGER_RESPONSE_NEW:
		gtk_widget_destroy (GTK_WIDGET (dialog));
		start_authorization_process (self);
		break;

	default:
		break;
	}
}


void
facebook_authentication_edit_accounts (FacebookAuthentication *self,
				       GtkWindow              *parent)
{
	GtkWidget  *dialog;

	dialog = facebook_account_manager_dialog_new (self->priv->accounts);
	g_signal_connect (dialog,
			  "delete-event",
			  G_CALLBACK (gtk_true),
			  NULL);
	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (account_manager_dialog_response_cb),
			  self);

	gtk_window_set_title (GTK_WINDOW (dialog), _("Edit Accounts"));
	gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (self->priv->dialog));
	gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
	gtk_window_present (GTK_WINDOW (dialog));
}
