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
#include "oauth-account.h"
#include "oauth-account-chooser-dialog.h"
#include "oauth-account-manager-dialog.h"
#include "oauth-authentication.h"


#define TOKEN_SECRET_SEPARATOR ("::")
#define OAUTH_AUTHENTICATION_RESPONSE_CHOOSE_ACCOUNT 1


/* Signals */
enum {
	READY,
	ACCOUNTS_CHANGED,
	LAST_SIGNAL
};

struct _OAuthAuthenticationPrivate
{
	OAuthConnection *conn;
	GCancellable    *cancellable;
	GList           *accounts;
	OAuthAccount    *account;
	GtkWidget       *browser;
	GtkWidget       *dialog;
};


static GObjectClass *parent_class = NULL;
static guint oauth_authentication_signals[LAST_SIGNAL] = { 0 };


static void
oauth_authentication_finalize (GObject *object)
{
	OAuthAuthentication *self;

	self = OAUTH_AUTHENTICATION (object);
	_g_object_unref (self->priv->conn);
	_g_object_unref (self->priv->cancellable);
	_g_object_list_unref (self->priv->accounts);
	_g_object_unref (self->priv->account);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
oauth_authentication_class_init (OAuthAuthenticationClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	g_type_class_add_private (class, sizeof (OAuthAuthenticationPrivate));

	object_class = (GObjectClass*) class;
	object_class->finalize = oauth_authentication_finalize;

	/* signals */

	oauth_authentication_signals[READY] =
		g_signal_new ("ready",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (OAuthAuthenticationClass, ready),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
	oauth_authentication_signals[ACCOUNTS_CHANGED] =
		g_signal_new ("accounts_changed",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (OAuthAuthenticationClass, accounts_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
}


static void
oauth_authentication_init (OAuthAuthentication *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, OAUTH_TYPE_AUTHENTICATION, OAuthAuthenticationPrivate);
	self->priv->conn = NULL;
	self->priv->accounts = NULL;
	self->priv->account = NULL;
}


GType
oauth_authentication_get_type (void)
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (GthTaskClass),
			NULL,
			NULL,
			(GClassInitFunc) oauth_authentication_class_init,
			NULL,
			NULL,
			sizeof (GthTask),
			0,
			(GInstanceInitFunc) oauth_authentication_init
		};

		type = g_type_register_static (G_TYPE_OBJECT,
					       "OAuthAuthentication",
					       &type_info,
					       0);
	}

	return type;
}


OAuthAuthentication *
oauth_authentication_new (OAuthConnection *conn,
			  GType            account_type,
			  GCancellable    *cancellable,
			  GtkWidget       *browser,
			  GtkWidget       *dialog)
{
	OAuthAuthentication *self;

	g_return_val_if_fail (conn != NULL, NULL);

	self = (OAuthAuthentication *) g_object_new (OAUTH_TYPE_AUTHENTICATION, NULL);
	self->priv->conn = g_object_ref (conn);
	self->priv->cancellable = _g_object_ref (cancellable);
	self->priv->accounts = oauth_accounts_load_from_file (self->priv->conn->consumer->name, account_type);
	self->priv->account = oauth_accounts_find_default (self->priv->accounts);
	self->priv->browser = browser;
	self->priv->dialog = dialog;

	return self;
}


static void show_choose_account_dialog (OAuthAuthentication *self);


static void
authentication_error_dialog_response_cb (GtkDialog *dialog,
					 int        response_id,
					 gpointer   user_data)
{
	OAuthAuthentication *self = user_data;

	switch (response_id) {
	case GTK_RESPONSE_DELETE_EVENT:
	case GTK_RESPONSE_CANCEL:
		gtk_widget_destroy (GTK_WIDGET (dialog));
		gtk_widget_destroy (self->priv->dialog);
		break;

	case OAUTH_AUTHENTICATION_RESPONSE_CHOOSE_ACCOUNT:
		gtk_widget_destroy (GTK_WIDGET (dialog));
		show_choose_account_dialog (self);
		break;

	default:
		break;
	}
}


static void start_authorization_process (OAuthAuthentication *self);


static void
show_authentication_error_dialog (OAuthAuthentication  *self,
				  GError              **error)
{
	GtkWidget *dialog;

	if (g_error_matches (*error, OAUTH_CONNECTION_ERROR, OAUTH_CONNECTION_ERROR_INVALID_TOKEN)) {
		start_authorization_process (self);
		return;
	}

	if (self->priv->conn != NULL)
		gth_task_dialog (GTH_TASK (self->priv->conn), TRUE);

	dialog = _gtk_message_dialog_new (GTK_WINDOW (self->priv->browser),
					  GTK_DIALOG_MODAL,
					  GTK_STOCK_DIALOG_ERROR,
					  _("Could not connect to the server"),
					  (*error)->message,
					  _("Choose _Account..."), OAUTH_AUTHENTICATION_RESPONSE_CHOOSE_ACCOUNT,
					  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					  NULL);
	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (authentication_error_dialog_response_cb),
			  self);
	gtk_widget_show (dialog);

	g_clear_error (error);
}


/* -- oauth_authentication_auto_connect -- */


static void
check_token_ready_cb (GObject      *source_object,
		      GAsyncResult *res,
		      gpointer      user_data)
{
	OAuthAuthentication *self = user_data;
	GError              *error = NULL;

	if (! oauth_connection_check_token_finish (self->priv->conn, res, &error)) {
		show_authentication_error_dialog (self, &error);
		return;
	}
	oauth_accounts_save_to_file (self->priv->conn->consumer->name,
				     self->priv->accounts,
				     self->priv->account);
	g_signal_emit (self, oauth_authentication_signals[READY], 0);
}


static void
connect_to_server_step2 (OAuthAuthentication *self)
{
	if (self->priv->account->token == NULL) {
		start_authorization_process (self);
		return;
	}

	oauth_connection_set_token (self->priv->conn,
				    self->priv->account->token,
				    self->priv->account->token_secret);
	oauth_connection_check_token (self->priv->conn,
				      self->priv->account,
				      self->priv->cancellable,
				      check_token_ready_cb,
				      self);
}


#ifdef HAVE_GNOME_KEYRING
static void
find_password_cb (GnomeKeyringResult  result,
                  const char         *string,
                  gpointer            user_data)
{
	OAuthAuthentication *self = user_data;

	if (string != NULL) {
		char **values;

		values = g_strsplit (string, TOKEN_SECRET_SEPARATOR, 2);
		if ((values[0] != NULL) && (values[1] != NULL)) {
			self->priv->account->token = g_strdup (values[0]);
			self->priv->account->token_secret = g_strdup (values[1]);
		}

		g_strfreev (values);
	}

	connect_to_server_step2 (self);
}
#endif


static void
connect_to_server (OAuthAuthentication *self)
{
	g_return_if_fail (self->priv->account != NULL);

#ifdef HAVE_GNOME_KEYRING
	if (((self->priv->account->token == NULL) || (self->priv->account->token_secret == NULL)) && gnome_keyring_is_available ()) {
		gnome_keyring_find_password (GNOME_KEYRING_NETWORK_PASSWORD,
					     find_password_cb,
					     self,
					     NULL,
					     "user", self->priv->account->username,
					     "server", self->priv->conn->consumer->url,
					     "protocol", self->priv->conn->consumer->protocol,
					     NULL);
		return;
	}
#endif

	connect_to_server_step2 (self);
}


static void
set_account (OAuthAuthentication *self,
	     OAuthAccount        *account)
{
	GList *link;

	link = g_list_find_custom (self->priv->accounts, self->priv->account, (GCompareFunc) oauth_account_cmp);
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
	OAuthAuthentication *self = user_data;
	connect_to_server (self);
}
#endif


static void
get_access_token_ready_cb (GObject      *source_object,
			   GAsyncResult *res,
			   gpointer      user_data)
{
	OAuthAuthentication *self = user_data;
	GError              *error = NULL;
	OAuthAccount        *account;

	account = oauth_connection_get_access_token_finish (self->priv->conn, res, &error);
	if (error != NULL) {
		show_authentication_error_dialog (self, &error);
		return;
	}

	set_account (self, account);

#ifdef HAVE_GNOME_KEYRING
	if (gnome_keyring_is_available ()) {
		char *secret;

		secret = g_strconcat (account->token,
				      TOKEN_SECRET_SEPARATOR,
				      account->token_secret,
				      NULL);
		gnome_keyring_store_password (GNOME_KEYRING_NETWORK_PASSWORD,
					      NULL,
					      self->priv->conn->consumer->display_name,
					      secret,
					      store_password_done_cb,
					      self,
					      NULL,
					      "user", account->username,
					      "server", self->priv->conn->consumer->url,
					      "protocol", self->priv->conn->consumer->protocol,
					      NULL);
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
	OAuthAuthentication *self = user_data;

	switch (response_id) {
	case GTK_RESPONSE_HELP:
		show_help_dialog (GTK_WINDOW (dialog), "oauth-complete-authorization");
		break;

	case GTK_RESPONSE_DELETE_EVENT:
	case GTK_RESPONSE_CANCEL:
		gtk_widget_destroy (GTK_WIDGET (dialog));
		gtk_widget_destroy (self->priv->dialog);
		break;

	case GTK_RESPONSE_OK:
		gtk_widget_destroy (GTK_WIDGET (dialog));
		gth_task_dialog (GTH_TASK (self->priv->conn), FALSE);
		oauth_connection_get_access_token (self->priv->conn,
						   self->priv->cancellable,
						   get_access_token_ready_cb,
						   self);
		break;

	default:
		break;
	}
}


static void
complete_authorization (OAuthAuthentication *self)
{
	GtkBuilder *builder;
	GtkWidget  *dialog;
	char       *text;
	char       *secondary_text;

	gth_task_dialog (GTH_TASK (self->priv->conn), TRUE);

	builder = _gtk_builder_new_from_file ("oauth-complete-authorization.ui", "oauth");
	dialog = _gtk_builder_get_widget (builder, "complete_authorization_messagedialog");
	text = g_strdup_printf (_("Return to this window when you have finished the authorization process on %s"), self->priv->conn->consumer->display_name);
	secondary_text = g_strdup (_("Once you're done, click the 'Continue' button below."));
	g_object_set (dialog, "text", text, "secondary-text", secondary_text, NULL);
	g_object_set_data_full (G_OBJECT (dialog), "builder", builder, g_object_unref);
	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (complete_authorization_messagedialog_response_cb),
			  self);

	if (GTK_WIDGET_VISIBLE (self->priv->dialog))
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
	OAuthAuthentication *self = user_data;

	switch (response_id) {
	case GTK_RESPONSE_HELP:
		show_help_dialog (GTK_WINDOW (dialog), "flicker-ask-authorization");
		break;

	case GTK_RESPONSE_DELETE_EVENT:
	case GTK_RESPONSE_CANCEL:
		gtk_widget_destroy (GTK_WIDGET (dialog));
		gtk_widget_destroy (self->priv->dialog);
		break;

	case GTK_RESPONSE_OK:
		{
			char   *url;
			GError *error = NULL;

			gtk_widget_destroy (GTK_WIDGET (dialog));

			url = oauth_connection_get_login_link (self->priv->conn);
			if (gtk_show_uri (gtk_widget_get_screen (GTK_WIDGET (dialog)), url, 0, &error))
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
ask_authorization (OAuthAuthentication *self)
{
	GtkBuilder *builder;
	GtkWidget  *dialog;
	char       *text;
	char       *secondary_text;

	gth_task_dialog (GTH_TASK (self->priv->conn), TRUE);

	builder = _gtk_builder_new_from_file ("oauth-ask-authorization.ui", "oauth");
	dialog = _gtk_builder_get_widget (builder, "ask_authorization_messagedialog");
	text = g_strdup_printf (_("gthumb requires your authorization to upload the photos to %s"), self->priv->conn->consumer->display_name);
	secondary_text = g_strdup_printf (_("Click 'Authorize' to open your web browser and authorize gthumb to upload photos to %s. When you're finished, return to this window to complete the authorization."), self->priv->conn->consumer->display_name);
	g_object_set (dialog, "text", text, "secondary-text", secondary_text, NULL);
	g_object_set_data_full (G_OBJECT (dialog), "builder", builder, g_object_unref);
	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (ask_authorization_messagedialog_response_cb),
			  self);

	gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
	if (GTK_WIDGET_VISIBLE (self->priv->dialog))
		gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (self->priv->dialog));
	else
		gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (self->priv->browser));
	gtk_window_present (GTK_WINDOW (dialog));

	g_free (secondary_text);
	g_free (text);
}


static void
get_request_token_ready_cb (GObject      *source_object,
			    GAsyncResult *res,
			    gpointer      user_data)
{
	OAuthAuthentication *self = user_data;
	GError              *error = NULL;

	if (! oauth_connection_get_request_token_finish (self->priv->conn, res, &error))
		show_authentication_error_dialog (self, &error);
	else
		ask_authorization (self);
}


static void
start_authorization_process (OAuthAuthentication *self)
{
	gtk_widget_hide (self->priv->dialog);
	gth_task_dialog (GTH_TASK (self->priv->conn), FALSE);

	oauth_connection_get_request_token (self->priv->conn,
					    self->priv->cancellable,
					    get_request_token_ready_cb,
					    self);
}


static void
account_chooser_dialog_response_cb (GtkDialog *dialog,
				    int        response_id,
				    gpointer   user_data)
{
	OAuthAuthentication *self = user_data;

	switch (response_id) {
	case GTK_RESPONSE_DELETE_EVENT:
	case GTK_RESPONSE_CANCEL:
		gtk_widget_destroy (GTK_WIDGET (dialog));
		gtk_widget_destroy (self->priv->dialog);
		break;

	case GTK_RESPONSE_OK:
		_g_object_unref (self->priv->account);
		self->priv->account = oauth_account_chooser_dialog_get_active (OAUTH_ACCOUNT_CHOOSER_DIALOG (dialog));
		if (self->priv->account != NULL) {
			gtk_widget_destroy (GTK_WIDGET (dialog));
			connect_to_server (self);
		}
		break;

	case OAUTH_ACCOUNT_CHOOSER_RESPONSE_NEW:
		gtk_widget_destroy (GTK_WIDGET (dialog));
		start_authorization_process (self);
		break;

	default:
		break;
	}
}


static void
show_choose_account_dialog (OAuthAuthentication *self)
{
	GtkWidget *dialog;

	gth_task_dialog (GTH_TASK (self->priv->conn), TRUE);
	dialog = oauth_account_chooser_dialog_new (self->priv->accounts, self->priv->account);
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
oauth_authentication_auto_connect (OAuthAuthentication *self)
{
	gtk_widget_hide (self->priv->dialog);
	gth_task_dialog (GTH_TASK (self->priv->conn), FALSE);

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
oauth_authentication_connect (OAuthAuthentication *self,
			      OAuthAccount        *account)
{
	set_account (self, account);
	oauth_authentication_auto_connect (self);
}


OAuthAccount *
oauth_authentication_get_account (OAuthAuthentication *self)
{
	return self->priv->account;
}


GList *
oauth_authentication_get_accounts (OAuthAuthentication *self)
{
	return self->priv->accounts;
}


/* -- oauth_authentication_edit_accounts -- */


static void
account_manager_dialog_response_cb (GtkDialog *dialog,
			            int        response_id,
			            gpointer   user_data)
{
	OAuthAuthentication *self = user_data;

	switch (response_id) {
	case GTK_RESPONSE_DELETE_EVENT:
	case GTK_RESPONSE_CANCEL:
		gtk_widget_destroy (GTK_WIDGET (dialog));
		break;

	case GTK_RESPONSE_OK:
		_g_object_list_unref (self->priv->accounts);
		self->priv->accounts = oauth_account_manager_dialog_get_accounts (OAUTH_ACCOUNT_MANAGER_DIALOG (dialog));
		if (! g_list_find_custom (self->priv->accounts, self->priv->account, (GCompareFunc) oauth_account_cmp)) {
			_g_object_unref (self->priv->account);
			self->priv->account = NULL;
			oauth_authentication_auto_connect (self);
		}
		else
			g_signal_emit (self, oauth_authentication_signals[ACCOUNTS_CHANGED], 0);
		oauth_accounts_save_to_file (self->priv->conn->consumer->name, self->priv->accounts, self->priv->account);
		gtk_widget_destroy (GTK_WIDGET (dialog));
		break;

	case OAUTH_ACCOUNT_MANAGER_RESPONSE_NEW:
		gtk_widget_destroy (GTK_WIDGET (dialog));
		start_authorization_process (self);
		break;

	default:
		break;
	}
}


void
oauth_authentication_edit_accounts (OAuthAuthentication *self,
				    GtkWindow           *parent)
{
	GtkWidget *dialog;

	dialog = oauth_account_manager_dialog_new (self->priv->accounts);
	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (account_manager_dialog_response_cb),
			  self);

	gtk_window_set_title (GTK_WINDOW (dialog), _("Edit Accounts"));
	gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (self->priv->dialog));
	gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
	gtk_window_present (GTK_WINDOW (dialog));
}


/* utilities */


GList *
oauth_accounts_load_from_file (const char *service_name,
			       GType       account_type)
{
	GList       *accounts = NULL;
	char        *filename;
	char        *path;
	char        *buffer;
	gsize        len;
	GError      *error = NULL;
	DomDocument *doc;

	filename = g_strconcat (service_name, ".xml", NULL);
	path = gth_user_dir_get_file (GTH_DIR_CONFIG, GTHUMB_DIR, "accounts", filename, NULL);
	if (! g_file_get_contents (path, &buffer, &len, &error)) {
		g_warning ("%s\n", error->message);
		g_error_free (error);
		g_free (path);
		g_free (filename);
		return NULL;
	}

	doc = dom_document_new ();
	if (dom_document_load (doc, buffer, len, NULL)) {
		DomElement *node;

		node = DOM_ELEMENT (doc)->first_child;
		if ((node != NULL) && (g_strcmp0 (node->tag_name, "accounts") == 0)) {
			DomElement *child;

			for (child = node->first_child;
			     child != NULL;
			     child = child->next_sibling)
			{
				if (strcmp (child->tag_name, "account") == 0) {
					OAuthAccount *account;

					account = g_object_new (account_type, NULL);
					dom_domizable_load_from_element (DOM_DOMIZABLE (account), child);

					accounts = g_list_prepend (accounts, account);
				}
			}

			accounts = g_list_reverse (accounts);
		}
	}

	g_object_unref (doc);
	g_free (buffer);
	g_free (path);
	g_free (filename);

	return accounts;
}


OAuthAccount *
oauth_accounts_find_default (GList *accounts)
{
	GList *scan;

	for (scan = accounts; scan; scan = scan->next) {
		OAuthAccount *account = scan->data;

		if (account->is_default)
			return g_object_ref (account);
	}

	return NULL;
}


void
oauth_accounts_save_to_file (const char   *service_name,
			     GList        *accounts,
			     OAuthAccount *default_account)
{
	DomDocument *doc;
	DomElement  *root;
	GList       *scan;
	char        *buffer;
	gsize        len;
	char        *filename;
	char        *path;
	GFile       *file;

	doc = dom_document_new ();
	root = dom_document_create_element (doc, "accounts", NULL);
	dom_element_append_child (DOM_ELEMENT (doc), root);
	for (scan = accounts; scan; scan = scan->next) {
		OAuthAccount *account = scan->data;
		DomElement    *node;

		if ((default_account != NULL) && g_strcmp0 (account->username, default_account->username) == 0)
			account->is_default = TRUE;
		else
			account->is_default = FALSE;
		node = dom_domizable_create_element (DOM_DOMIZABLE (account), doc);
		dom_element_append_child (root, node);
	}

	filename = g_strconcat (service_name, ".xml", NULL);
	gth_user_dir_make_dir_for_file (GTH_DIR_CONFIG, GTHUMB_DIR, "accounts", filename, NULL);
	path = gth_user_dir_get_file (GTH_DIR_CONFIG, GTHUMB_DIR, "accounts", filename, NULL);
	file = g_file_new_for_path (path);
	buffer = dom_document_dump (doc, &len);
	g_write_file (file, FALSE, G_FILE_CREATE_PRIVATE | G_FILE_CREATE_REPLACE_DESTINATION, buffer, len, NULL, NULL);

	g_free (buffer);
	g_object_unref (file);
	g_free (path);
	g_free (filename);
	g_object_unref (doc);
}
