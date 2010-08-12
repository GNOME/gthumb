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
#include <glib/gi18n.h>
#include <gthumb.h>
#include "google-connection.h"

#undef  DEBUG_GOOGLE_CONNECTION
#define SOUP_LOG_LEVEL SOUP_LOGGER_LOG_BODY
#define GTHUMB_SOURCE ("GNOME-" PACKAGE "-" VERSION)


GQuark
google_connection_error_quark (void)
{
	static GQuark quark;

        if (!quark)
                quark = g_quark_from_static_string ("google_connection");

        return quark;
}


/* -- GoogleConnection -- */


struct _GoogleConnectionPrivate
{
	char               *service;
	SoupSession        *session;
	char               *token;
	char               *challange_url;
	GCancellable       *cancellable;
	GSimpleAsyncResult *result;
};


static gpointer parent_class = NULL;


static void
google_connection_finalize (GObject *object)
{
	GoogleConnection *self;

	self = GOOGLE_CONNECTION (object);

	_g_object_unref (self->priv->result);
	_g_object_unref (self->priv->cancellable);
	g_free (self->priv->challange_url);
	g_free (self->priv->token);
	_g_object_unref (self->priv->session);
	g_free (self->priv->service);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
google_connection_exec (GthTask *base)
{
	/* void */
}


static void
google_connection_cancelled (GthTask *base)
{
	/* void */
}


static void
google_connection_class_init (GoogleConnectionClass *klass)
{
	GObjectClass *object_class;
	GthTaskClass *task_class;

	parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GoogleConnectionPrivate));

	object_class = (GObjectClass*) klass;
	object_class->finalize = google_connection_finalize;

	task_class = (GthTaskClass*) klass;
	task_class->exec = google_connection_exec;
	task_class->cancelled = google_connection_cancelled;
}


static void
google_connection_init (GoogleConnection *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GOOGLE_TYPE_CONNECTION, GoogleConnectionPrivate);
	self->priv->service = NULL;
	self->priv->session = NULL;
	self->priv->token = NULL;
	self->priv->challange_url = NULL;
	self->priv->cancellable = NULL;
	self->priv->result = NULL;
}


GType
google_connection_get_type (void)
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (GoogleConnectionClass),
			NULL,
			NULL,
			(GClassInitFunc) google_connection_class_init,
			NULL,
			NULL,
			sizeof (GoogleConnection),
			0,
			(GInstanceInitFunc) google_connection_init
		};

		type = g_type_register_static (GTH_TYPE_TASK,
					       "GoogleConnection",
					       &type_info,
					       0);
	}

	return type;
}


GoogleConnection *
google_connection_new (const char *service)
{
	GoogleConnection *self;

	self = (GoogleConnection *) g_object_new (GOOGLE_TYPE_CONNECTION, NULL);
	self->priv->service = g_strdup (service);

	return self;
}


void
google_connection_send_message (GoogleConnection    *self,
				SoupMessage         *msg,
				GCancellable        *cancellable,
				GAsyncReadyCallback  callback,
				gpointer             user_data,
				gpointer             source_tag,
				SoupSessionCallback  soup_session_cb,
				gpointer             soup_session_cb_data)
{
	char *value;

	_g_object_unref (self->priv->cancellable);
	self->priv->cancellable = _g_object_ref (cancellable);

	_g_object_unref (self->priv->result);
	self->priv->result = g_simple_async_result_new (G_OBJECT (soup_session_cb_data),
							callback,
							user_data,
							source_tag);

	value = g_strconcat ("GoogleLogin auth=", self->priv->token, NULL);
	soup_message_headers_replace (msg->request_headers, "Authorization", value);
	g_free (value);

	soup_message_headers_replace (msg->request_headers, "GData-Version", "2");

	soup_session_queue_message (self->priv->session,
				    msg,
				    soup_session_cb,
				    soup_session_cb_data);
}


GSimpleAsyncResult *
google_connection_get_result (GoogleConnection *self)
{
	return self->priv->result;
}


static GHashTable *
get_keys_from_message_body (SoupBuffer *body)
{
	GHashTable  *keys;
	char       **lines;
	int          i;

	keys = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
	lines = g_strsplit (body->data, "\n", -1);
	for (i = 0; lines[i] != NULL; i++) {
		char **pair;

		pair = g_strsplit (lines[i], "=", 2);
		if ((pair[0] != NULL) && (pair[1] != NULL))
			g_hash_table_insert (keys, g_strdup (pair[0]), g_strdup (pair[1]));

		g_strfreev (pair);
	}

	g_strfreev (lines);

	return keys;
}


static void
connect_ready_cb (SoupSession *session,
		  SoupMessage *msg,
		  gpointer     user_data)
{
	GoogleConnection *self = user_data;
	SoupBuffer       *body;
	GHashTable       *keys;

	body = soup_message_body_flatten (msg->response_body);
	keys = get_keys_from_message_body (body);

	g_free (self->priv->token);
	self->priv->token = NULL;

	if (msg->status_code == 403) {
		char   *error_name;
		GError *error;
		int     error_code;
		char   *error_message;

		error_name = g_hash_table_lookup (keys, "Error");
		error_code = GOOGLE_CONNECTION_ERROR_UNKNOWN;
		error_message = "The error is unknown or unspecified; the request contained invalid input or was malformed.";

		if (error_name == NULL) {
			/* void */
		}
		else if (strcmp (error_name, "BadAuthentication") == 0) {
			error_code = GOOGLE_CONNECTION_ERROR_BAD_AUTHENTICATION;
			error_message = "The login request used a username or password that is not recognized.";
		}
		else if (strcmp (error_name, "NotVerified") == 0) {
			error_code = GOOGLE_CONNECTION_ERROR_NOT_VERIFIED;
			error_message = "The account email address has not been verified. The user will need to access their Google account directly to resolve the issue before logging in using a non-Google application.";
		}
		else if (strcmp (error_name, "TermsNotAgreed") == 0) {
			error_code = GOOGLE_CONNECTION_ERROR_TERMS_NOT_AGREED;
			error_message = "The user has not agreed to terms. The user will need to access their Google account directly to resolve the issue before logging in using a non-Google application.";
		}
		else if (strcmp (error_name, "CaptchaRequired") == 0) {
			error_code = GOOGLE_CONNECTION_ERROR_CAPTCHA_REQUIRED;
			error_message = "A CAPTCHA is required.";
		}
		else if (strcmp (error_name, "AccountDeleted") == 0) {
			error_code = GOOGLE_CONNECTION_ERROR_ACCOUNT_DELETED;
			error_message = "The user account has been deleted.";
		}
		else if (strcmp (error_name, "AccountDisabled") == 0) {
			error_code = GOOGLE_CONNECTION_ERROR_ACCOUNT_DISABLED;
			error_message = "The user account has been disabled.";
		}
		else if (strcmp (error_name, "ServiceDisabled") == 0) {
			error_code = GOOGLE_CONNECTION_ERROR_SERVICE_DISABLED;
			error_message = "The user's access to the specified service has been disabled.";
		}
		else if (strcmp (error_name, "ServiceUnavailable") == 0) {
			error_code = GOOGLE_CONNECTION_ERROR_SERVICE_UNAVAILABLE;
			error_message = "The service is not available; try again later.";
		}

		error = g_error_new_literal (GOOGLE_CONNECTION_ERROR, error_code, error_message);
		if (error_code == GOOGLE_CONNECTION_ERROR_CAPTCHA_REQUIRED) {
			g_free (self->priv->challange_url);
			self->priv->token = g_strdup (g_hash_table_lookup (keys, "CaptchaToken"));
			self->priv->challange_url = g_strdup (g_hash_table_lookup (keys, "CaptchaUrl"));
		}

		g_simple_async_result_set_from_error (self->priv->result, error);
		g_error_free (error);
	}
	else if (msg->status_code == 200) {
		self->priv->token = g_strdup (g_hash_table_lookup (keys, "Auth"));
		g_simple_async_result_set_op_res_gboolean (self->priv->result, TRUE);
	}
	else
		g_simple_async_result_set_error (self->priv->result,
						 SOUP_HTTP_ERROR,
						 msg->status_code,
						 "%s",
						 soup_status_get_phrase (msg->status_code));

	g_simple_async_result_complete_in_idle (self->priv->result);

	g_hash_table_destroy (keys);
	soup_buffer_free (body);
}


void
google_connection_connect (GoogleConnection    *self,
			   const char          *email,
			   const char          *password,
			   const char          *challange,
			   GCancellable        *cancellable,
			   GAsyncReadyCallback  callback,
			   gpointer             user_data)
{
	SoupMessage *msg;
	GHashTable  *data_set;

	g_return_if_fail (email != NULL);
	g_return_if_fail (password != NULL);

	if (self->priv->session == NULL) {
		self->priv->session = soup_session_async_new_with_options (
#ifdef HAVE_LIBSOUP_GNOME
			SOUP_SESSION_ADD_FEATURE_BY_TYPE, SOUP_TYPE_PROXY_RESOLVER_GNOME,
#endif
			NULL);

#ifdef DEBUG_GOOGLE_CONNECTION
		{
			SoupLogger *logger;

			logger = soup_logger_new (SOUP_LOG_LEVEL, -1);
			soup_session_add_feature (self->priv->session, SOUP_SESSION_FEATURE (logger));

			g_object_unref (logger);
		}
#endif
	}

	_g_object_unref (self->priv->cancellable);
	self->priv->cancellable = _g_object_ref (cancellable);

	_g_object_unref (self->priv->result);
	self->priv->result = g_simple_async_result_new (G_OBJECT (self),
							callback,
							user_data,
							google_connection_connect);

	gth_task_progress (GTH_TASK (self), _("Connecting to the server"), NULL, TRUE, 0.0);

	data_set = g_hash_table_new (g_str_hash, g_str_equal);
	g_hash_table_insert (data_set, "accountType", "HOSTED_OR_GOOGLE");
	g_hash_table_insert (data_set, "service", self->priv->service);
	g_hash_table_insert (data_set, "Email", (char *) email);
	g_hash_table_insert (data_set, "Passwd", (char *) password);
	g_hash_table_insert (data_set, "source", GTHUMB_SOURCE);
	if (self->priv->token != NULL)
		g_hash_table_insert (data_set, "logintoken", self->priv->token);
	if (challange != NULL)
		g_hash_table_insert (data_set, "logincaptcha", (char *) challange);
	msg = soup_form_request_new_from_hash ("POST",
					       "https://www.google.com/accounts/ClientLogin",
					       data_set);
	soup_session_queue_message (self->priv->session, msg, connect_ready_cb, self);

	g_hash_table_destroy (data_set);
}


gboolean
google_connection_connect_finish (GoogleConnection  *conn,
				  GAsyncResult      *result,
				  GError           **error)
{
	if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (result), error))
		return FALSE;
	else
		return TRUE;
}


const char *
google_connection_get_challange_url (GoogleConnection *self)
{
	return self->priv->challange_url;
}
