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
#ifdef HAVE_GNOME_KEYRING
#include <gnome-keyring.h>
#endif /* HAVE_GNOME_KEYRING */
#include "facebook-account-chooser-dialog.h"
#include "facebook-account-manager-dialog.h"
#include "facebook-authentication.h"
#include "facebook-user.h"
#include "facebook-service.h"


#define SECRET_SEPARATOR ("::")
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


static GObjectClass *parent_class = NULL;
static guint facebook_authentication_signals[LAST_SIGNAL] = { 0 };


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

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
facebook_authentication_class_init (FacebookAuthenticationClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
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
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE,
			      1,
			      G_TYPE_OBJECT);
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


GType
facebook_authentication_get_type (void)
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (GthTaskClass),
			NULL,
			NULL,
			(GClassInitFunc) facebook_authentication_class_init,
			NULL,
			NULL,
			sizeof (GthTask),
			0,
			(GInstanceInitFunc) facebook_authentication_init
		};

		type = g_type_register_static (G_TYPE_OBJECT,
					       "FacebookAuthentication",
					       &type_info,
					       0);
	}

	return type;
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

	if (g_error_matches (*error, FACEBOOK_CONNECTION_ERROR, FACEBOOK_CONNECTION_ERROR_SESSION_KEY_INVALID)) {
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


/* -- facebook_authentication_auto_connect -- */


static void
get_user_info_ready_cb (GObject      *source_object,
			GAsyncResult *res,
			gpointer      user_data)
{
	FacebookAuthentication *self = user_data;
	FacebookUser           *user;
	GError                 *error = NULL;

	user = facebook_service_get_user_info_finish (FACEBOOK_SERVICE (source_object), res, &error);
	if (error != NULL) {
		show_authentication_error_dialog (self, &error);
		return;
	}

	facebook_account_set_username (self->priv->account, user->username);
	facebook_accounts_save_to_file (self->priv->accounts, self->priv->account);

	g_signal_emit (self, facebook_authentication_signals[READY], 0, user);

	g_object_unref (user);
}


static void
get_logged_in_user_ready_cb (GObject      *source_object,
			     GAsyncResult *res,
			     gpointer      user_data)
{
	FacebookAuthentication *self = user_data;
	char                   *uid;
	GError                 *error = NULL;

	uid = facebook_service_get_logged_in_user_finish (FACEBOOK_SERVICE (source_object), res, &error);
	if (error != NULL) {
		show_authentication_error_dialog (self, &error);
		return;
	}

	if (g_strcmp0 (uid, self->priv->account->user_id) == 0) {
		FacebookUser *user;

		user = facebook_user_new ();
		facebook_user_set_id (user, uid);
		facebook_user_set_username (user, self->priv->account->username);
		g_signal_emit (self, facebook_authentication_signals[READY], 0, user);

		g_object_unref (user);
	}
	else {
		/* Authorization required */
		start_authorization_process (self);
	}

	g_free (uid);
}


static void
connect_to_server_step2 (FacebookAuthentication *self)
{
	if ((self->priv->account->session_key == NULL) || (self->priv->account->secret == NULL)) {
		start_authorization_process (self);
		return;
	}
	facebook_connection_set_session (self->priv->conn,
					 self->priv->account->session_key,
					 self->priv->account->secret);
	if (self->priv->account->username == NULL)
		facebook_service_get_user_info (self->priv->service,
						"first_name,middle_name,last_name,name",
						self->priv->cancellable,
						get_user_info_ready_cb,
						self);
	else
		facebook_service_get_logged_in_user (self->priv->service,
						     self->priv->cancellable,
						     get_logged_in_user_ready_cb,
						     self);
}


#ifdef HAVE_GNOME_KEYRING
static void
find_password_cb (GnomeKeyringResult  result,
                  const char         *string,
                  gpointer            user_data)
{
	FacebookAuthentication *self = user_data;

	if (string != NULL) {
		char **values;

		values = g_strsplit (string, SECRET_SEPARATOR, 2);
		if ((values[0] != NULL) && (values[1] != NULL)) {
			self->priv->account->session_key = g_strdup (values[0]);
			self->priv->account->secret = g_strdup (values[1]);
		}

		g_strfreev (values);
	}

	connect_to_server_step2 (self);
}
#endif


static void
connect_to_server (FacebookAuthentication *self)
{
	g_return_if_fail (self->priv->account != NULL);

#ifdef HAVE_GNOME_KEYRING
	if (((self->priv->account->session_key == NULL) || (self->priv->account->secret == NULL)) && gnome_keyring_is_available ()) {
		gnome_keyring_find_password (GNOME_KEYRING_NETWORK_PASSWORD,
					     find_password_cb,
					     self,
					     NULL,
					     "user", self->priv->account->user_id,
					     "server", FACEBOOK_HTTPS_REST_SERVER,
					     "protocol", "https",
					     NULL);
		return;
	}
#endif

	connect_to_server_step2 (self);
}


static void
set_account (FacebookAuthentication *self,
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


#ifdef HAVE_GNOME_KEYRING
static void
store_password_done_cb (GnomeKeyringResult result,
			gpointer           user_data)
{
	FacebookAuthentication *self = user_data;
	connect_to_server (self);
}
#endif


static void
get_session_ready_cb (GObject      *source_object,
		      GAsyncResult *res,
		      gpointer      user_data)
{
	FacebookAuthentication *self = user_data;
	GError                 *error = NULL;
	FacebookAccount        *account;

	if (! facebook_connection_get_session_finish (FACEBOOK_CONNECTION (source_object), res, &error)) {
		show_authentication_error_dialog (self, &error);
		return;
	}

	account = facebook_account_new ();
	facebook_account_set_session_key (account, facebook_connection_get_session_key (self->priv->conn));
	facebook_account_set_secret (account, facebook_connection_get_secret (self->priv->conn));
	facebook_account_set_user_id (account, facebook_connection_get_user_id (self->priv->conn));
	set_account (self, account);

#ifdef HAVE_GNOME_KEYRING
	if (gnome_keyring_is_available ()) {
		char *secret;

		secret = g_strconcat (account->session_key, SECRET_SEPARATOR, account->secret, NULL);
		gnome_keyring_store_password (GNOME_KEYRING_NETWORK_PASSWORD,
					      NULL,
					      "Facebook",
					      secret,
					      store_password_done_cb,
					      self,
					      NULL,
					      "user", account->user_id,
					      "server", FACEBOOK_HTTPS_REST_SERVER,
					      "protocol", "https",
					      NULL);
		g_free (secret);
		return;
	}
#endif

	g_object_unref (account);
	connect_to_server (self);
}


static void
complete_authorization_messagedialog_response_cb (GtkDialog *dialog,
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
		facebook_connection_get_session (self->priv->conn,
						 self->priv->cancellable,
						 get_session_ready_cb,
						 self);
		break;

	default:
		break;
	}
}


static void
complete_authorization (FacebookAuthentication *self)
{
	GtkBuilder *builder;
	GtkWidget  *dialog;
	char       *text;
	char       *secondary_text;

	gth_task_dialog (GTH_TASK (self->priv->conn), TRUE, NULL);

	builder = _gtk_builder_new_from_file ("facebook-complete-authorization.ui", "facebook");
	dialog = _gtk_builder_get_widget (builder, "complete_authorization_messagedialog");
	text = g_strdup_printf (_("Return to this window when you have finished the authorization process on %s"), "Facebook");
	secondary_text = g_strdup (_("Once you're done, click the 'Continue' button below."));
	g_object_set (dialog, "text", text, "secondary-text", secondary_text, NULL);
	g_object_set_data_full (G_OBJECT (dialog), "builder", builder, g_object_unref);
	g_signal_connect (dialog,
			  "delete-event",
			  G_CALLBACK (gtk_true),
			  NULL);
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
	FacebookAuthentication *self = user_data;

	switch (response_id) {
	case GTK_RESPONSE_HELP:
		show_help_dialog (GTK_WINDOW (dialog), "facebook-ask-authorization");
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

			url = facebook_connection_get_login_link (self->priv->conn, FACEBOOK_ACCESS_WRITE);
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
ask_authorization (FacebookAuthentication *self)
{
	GtkBuilder *builder;
	GtkWidget  *dialog;
	char       *text;
	char       *secondary_text;

	gth_task_dialog (GTH_TASK (self->priv->conn), TRUE, NULL);

	builder = _gtk_builder_new_from_file ("facebook-ask-authorization.ui", "facebook");
	dialog = _gtk_builder_get_widget (builder, "ask_authorization_messagedialog");
	if (gtk_widget_get_visible (self->priv->dialog))
		gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (self->priv->dialog));
	else
		gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (self->priv->browser));

	text = g_strdup_printf (_("gthumb requires your authorization to upload the photos to %s"), "Facebook");
	secondary_text = g_strdup_printf (_("Click 'Authorize' to open your web browser and authorize gthumb to upload photos to %s. When you're finished, return to this window to complete the authorization."), "Facebook");
	g_object_set (dialog, "text", text, "secondary-text", secondary_text, NULL);
	g_object_set_data_full (G_OBJECT (dialog), "builder", builder, g_object_unref);
	g_signal_connect (dialog,
			  "delete-event",
			  G_CALLBACK (gtk_true),
			  NULL);
	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (ask_authorization_messagedialog_response_cb),
			  self);

	gtk_widget_show (dialog);

	g_free (secondary_text);
	g_free (text);
}


static void
create_token_ready_cb (GObject      *source_object,
		       GAsyncResult *res,
		       gpointer      user_data)
{
	FacebookAuthentication *self = user_data;
	GError                 *error = NULL;

	if (! facebook_connection_create_token_finish (FACEBOOK_CONNECTION (source_object), res, &error))
		show_authentication_error_dialog (self, &error);
	else
		ask_authorization (self);
}


static void
start_authorization_process (FacebookAuthentication *self)
{
	facebook_connection_create_token (self->priv->conn,
					  self->priv->cancellable,
					  create_token_ready_cb,
					  self);
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
	set_account (self, account);
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
