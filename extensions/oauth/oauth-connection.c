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
#include <glib/gi18n.h>
#include <gthumb.h>
#include "oauth-connection.h"


#define DEBUG_OAUTH_CONNECTION 1
#define OAUTH_VERSION "1.0"
#define OAUTH_SIGNATURE_METHOD "HMAC-SHA1"


GQuark
oauth_connection_error_quark (void)
{
	static GQuark quark;

        if (!quark)
                quark = g_quark_from_static_string ("oauth_connection");

        return quark;
}


/* -- OAuthConnection -- */


struct _OAuthConnectionPrivate
{
	SoupSession        *session;
	char               *timestamp;
	char               *nonce;
	char               *signature;
	char               *token;
	char               *token_secret;
	GCancellable       *cancellable;
	GSimpleAsyncResult *result;
};


static gpointer parent_class = NULL;


static void
oauth_connection_finalize (GObject *object)
{
	OAuthConnection *self;

	self = OAUTH_CONNECTION (object);

	_g_object_unref (self->priv->result);
	_g_object_unref (self->priv->cancellable);
	g_free (self->priv->token);
	g_free (self->priv->token_secret);
	g_free (self->priv->signature);
	g_free (self->priv->nonce);
	g_free (self->priv->timestamp);
	_g_object_unref (self->priv->session);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
oauth_connection_exec (GthTask *base)
{
	/* void */
}


static void
oauth_connection_cancelled (GthTask *base)
{
	/* void */
}


static void
oauth_connection_class_init (OAuthConnectionClass *klass)
{
	GObjectClass *object_class;
	GthTaskClass *task_class;

	parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (OAuthConnectionPrivate));

	object_class = (GObjectClass*) klass;
	object_class->finalize = oauth_connection_finalize;

	task_class = (GthTaskClass*) klass;
	task_class->exec = oauth_connection_exec;
	task_class->cancelled = oauth_connection_cancelled;
}


static void
oauth_connection_init (OAuthConnection *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, OAUTH_TYPE_CONNECTION, OAuthConnectionPrivate);
	self->priv->session = NULL;
	self->priv->timestamp = NULL;
	self->priv->nonce = NULL;
	self->priv->signature = NULL;
	self->priv->token = NULL;
	self->priv->token_secret = NULL;
	self->priv->cancellable = NULL;
	self->priv->result = NULL;
}


GType
oauth_connection_get_type (void)
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (OAuthConnectionClass),
			NULL,
			NULL,
			(GClassInitFunc) oauth_connection_class_init,
			NULL,
			NULL,
			sizeof (OAuthConnection),
			0,
			(GInstanceInitFunc) oauth_connection_init
		};

		type = g_type_register_static (GTH_TYPE_TASK,
					       "OAuthConnection",
					       &type_info,
					       0);
	}

	return type;
}


OAuthConnection *
oauth_connection_new (OAuthConsumer *consumer)
{
	OAuthConnection *self;

	self = (OAuthConnection *) g_object_new (OAUTH_TYPE_CONNECTION, NULL);
	self->consumer = consumer;

	return self;
}


void
oauth_connection_send_message (OAuthConnection     *self,
			       SoupMessage         *msg,
			       GCancellable        *cancellable,
			       GAsyncReadyCallback  callback,
			       gpointer             user_data,
			       gpointer             source_tag,
			       SoupSessionCallback  soup_session_cb,
			       gpointer             soup_session_cb_data)
{
	if (self->priv->session == NULL) {
		self->priv->session = soup_session_async_new_with_options (
#ifdef HAVE_LIBSOUP_GNOME
			SOUP_SESSION_ADD_FEATURE_BY_TYPE, SOUP_TYPE_PROXY_RESOLVER_GNOME,
#endif
			NULL);

#ifdef DEBUG_OAUTH_CONNECTION
		{
			SoupLogger *logger;

			logger = soup_logger_new (SOUP_LOGGER_LOG_BODY, -1);
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
							source_tag);

	soup_session_queue_message (self->priv->session,
				    msg,
				    soup_session_cb,
				    soup_session_cb_data);
}


GSimpleAsyncResult *
oauth_connection_get_result (OAuthConnection *self)
{
	return self->priv->result;
}


void
oauth_connection_reset_result (OAuthConnection *self)
{
	_g_object_unref (self->priv->result);
	self->priv->result = NULL;
}


/* -- oauth_connection_add_signature -- */


static char *
oauth_create_timestamp (GTimeVal *t)
{
	return g_strdup_printf ("%ld", t->tv_sec);
}


static char *
oauth_create_nonce (GTimeVal *t)
{
	char *s;
	char *v;

	s = g_strdup_printf ("%ld%u", t->tv_usec, g_random_int ());
	v = g_compute_checksum_for_string (G_CHECKSUM_MD5, s, -1);

	g_free (s);

	return v;
}


void
oauth_connection_add_signature (OAuthConnection *self,
				const char      *method,
				const char      *url,
			        GHashTable      *parameters)
{
	GTimeVal  t;
	GString  *param_string;
	GList    *keys;
	GList    *scan;
	GString  *base_string;
	GString  *signature_key;

	/* Add the OAuth specific parameters */

	g_get_current_time (&t);

	g_free (self->priv->timestamp);
	self->priv->timestamp = oauth_create_timestamp (&t);
	g_hash_table_insert (parameters, "oauth_timestamp", self->priv->timestamp);

	g_free (self->priv->nonce);
	self->priv->nonce = oauth_create_nonce (&t);
	g_hash_table_insert (parameters, "oauth_nonce", self->priv->nonce);

	g_hash_table_insert (parameters, "format", "xml");
	g_hash_table_insert (parameters, "oauth_version", OAUTH_VERSION);
	g_hash_table_insert (parameters, "oauth_signature_method", OAUTH_SIGNATURE_METHOD);
	g_hash_table_insert (parameters, "oauth_consumer_key", (gpointer) self->consumer->consumer_key);
	if (self->priv->token != NULL)
		g_hash_table_insert (parameters, "oauth_token", self->priv->token);

	/* Create the parameter string */

	param_string = g_string_new ("");
	keys = g_hash_table_get_keys (parameters);
	keys = g_list_sort (keys, (GCompareFunc) strcmp);
	for (scan = keys; scan; scan = scan->next) {
		char *key = scan->data;

		g_string_append (param_string, key);
		g_string_append (param_string, "=");
		g_string_append_uri_escaped (param_string,
					     g_hash_table_lookup (parameters, key),
					     NULL,
					     TRUE);
		if (scan->next != NULL)
			g_string_append (param_string, "&");
	}

	/* Create the Base String */

	base_string = g_string_new ("");
	g_string_append_uri_escaped (base_string, method, NULL, TRUE);
	g_string_append (base_string, "&");
	g_string_append_uri_escaped (base_string, url, NULL, TRUE);
	g_string_append (base_string, "&");
	g_string_append_uri_escaped (base_string, param_string->str, NULL, TRUE);

	/* Calculate the signature value */

	signature_key = g_string_new ("");
	g_string_append (signature_key, self->consumer->consumer_secret);
	g_string_append (signature_key, "&");
	if (self->priv->token_secret != NULL)
		g_string_append (signature_key, self->priv->token_secret);
	g_free (self->priv->signature);
	self->priv->signature = g_compute_signature_for_string (G_CHECKSUM_SHA1,
								G_SIGNATURE_ENC_BASE64,
							        signature_key->str,
							        signature_key->len,
							        base_string->str,
							        base_string->len);
	g_hash_table_insert (parameters, "oauth_signature", self->priv->signature);

	g_string_free (signature_key, TRUE);
	g_string_free (base_string, TRUE);
	g_list_free (keys);
	g_string_free (param_string, TRUE);
}


/* -- oauth_connection_get_request_token -- */


static void
get_request_token_ready_cb (SoupSession *session,
			    SoupMessage *msg,
			    gpointer     user_data)
{
	OAuthConnection *self = user_data;
	SoupBuffer      *body;

	if (msg->status_code != 200) {
		g_simple_async_result_set_error (self->priv->result,
						 SOUP_HTTP_ERROR,
						 msg->status_code,
						 "%s",
						 soup_status_get_phrase (msg->status_code));
		g_simple_async_result_complete_in_idle (self->priv->result);
		return;
	}

	body = soup_message_body_flatten (msg->response_body);
	self->consumer->get_request_token_response (self, msg, body, self->priv->result);
	g_simple_async_result_complete_in_idle (self->priv->result);

	soup_buffer_free (body);
}


void
oauth_connection_get_request_token (OAuthConnection     *self,
				    GCancellable        *cancellable,
				    GAsyncReadyCallback  callback,
				    gpointer             user_data)
{
	GHashTable  *data_set;
	SoupMessage *msg;

	gth_task_progress (GTH_TASK (self), _("Connecting to the server"), _("Getting account information"), TRUE, 0.0);

	data_set = g_hash_table_new (g_str_hash, g_str_equal);
	oauth_connection_add_signature (self, "POST", self->consumer->request_token_url, data_set);
	msg = soup_form_request_new_from_hash ("POST", self->consumer->request_token_url, data_set);
	oauth_connection_send_message (self,
				       msg,
				       cancellable,
				       callback,
				       user_data,
				       oauth_connection_get_request_token,
				       get_request_token_ready_cb,
				       self);

	g_hash_table_destroy (data_set);
}


gboolean
oauth_connection_get_request_token_finish (OAuthConnection  *self,
					   GAsyncResult     *result,
					   GError          **error)
{
	if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (result), error))
		return FALSE;
	else
		return TRUE;
}


char *
oauth_connection_get_login_link (OAuthConnection *self)
{
	return self->consumer->get_login_link (self);
}


/* -- oauth_connection_get_access_token -- */


static void
get_access_token_ready_cb (SoupSession *session,
			   SoupMessage *msg,
			   gpointer     user_data)
{
	OAuthConnection *self = user_data;
	SoupBuffer      *body;

	if (msg->status_code != 200) {
		g_simple_async_result_set_error (self->priv->result,
						 SOUP_HTTP_ERROR,
						 msg->status_code,
						 "%s",
						 soup_status_get_phrase (msg->status_code));
		g_simple_async_result_complete_in_idle (self->priv->result);
		return;
	}

	body = soup_message_body_flatten (msg->response_body);
	self->consumer->get_access_token_response (self, msg, body, self->priv->result);
	g_simple_async_result_complete_in_idle (self->priv->result);

	soup_buffer_free (body);
}


void
oauth_connection_get_access_token (OAuthConnection     *self,
				   GCancellable        *cancellable,
				   GAsyncReadyCallback  callback,
				   gpointer             user_data)
{
	GHashTable  *data_set;
	SoupMessage *msg;

	gth_task_progress (GTH_TASK (self), _("Connecting to the server"), _("Getting account information"), TRUE, 0.0);

	data_set = g_hash_table_new (g_str_hash, g_str_equal);
	oauth_connection_add_signature (self, "POST", self->consumer->access_token_url, data_set);
	msg = soup_form_request_new_from_hash ("POST", self->consumer->access_token_url, data_set);
	oauth_connection_send_message (self,
				       msg,
				       cancellable,
				       callback,
				       user_data,
				       oauth_connection_get_access_token,
				       get_access_token_ready_cb,
				       self);

	g_hash_table_destroy (data_set);
}


OAuthAccount *
oauth_connection_get_access_token_finish (OAuthConnection  *self,
					  GAsyncResult     *result,
					  GError          **error)
{
	if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (result), error))
		return FALSE;
	else
		return g_object_ref (g_simple_async_result_get_op_res_gpointer (G_SIMPLE_ASYNC_RESULT (result)));
}


void
oauth_connection_set_token (OAuthConnection *self,
			    const char      *token,
			    const char      *token_secret)
{
	_g_strset (&self->priv->token, token);
	_g_strset (&self->priv->token_secret, token_secret);
}


const char *
oauth_connection_get_token (OAuthConnection *self)
{
	return self->priv->token;
}


const char *
oauth_connection_get_token_secret (OAuthConnection *self)
{
	return self->priv->token_secret;
}


/* -- oauth_connection_check_token -- */


typedef struct {
	OAuthConnection *conn;
	OAuthAccount    *account;
} CheckTokenData;


static void
check_token_ready_cb (SoupSession *session,
		      SoupMessage *msg,
		      gpointer     user_data)
{
	CheckTokenData  *check_token_data = user_data;
	OAuthConnection *self = check_token_data->conn;

	if (msg->status_code != 200) {
		g_simple_async_result_set_error (self->priv->result,
						 SOUP_HTTP_ERROR,
						 msg->status_code,
						 "%s",
						 soup_status_get_phrase (msg->status_code));
		g_simple_async_result_complete_in_idle (self->priv->result);
		return;
	}

	self->consumer->check_token_response (self, msg, self->priv->result, check_token_data->account);
	g_simple_async_result_complete_in_idle (self->priv->result);

	g_free (check_token_data);
}


void
oauth_connection_check_token (OAuthConnection     *self,
			      OAuthAccount        *account,
			      GCancellable        *cancellable,
			      GAsyncReadyCallback  callback,
			      gpointer             user_data)
{
	CheckTokenData *check_token_data;
	GHashTable     *data_set;
	char           *url;
	SoupMessage    *msg;

	check_token_data = g_new0 (CheckTokenData, 1);
	check_token_data->conn = self;
	check_token_data->account = account;

	gth_task_progress (GTH_TASK (self), _("Connecting to the server"), _("Getting account information"), TRUE, 0.0);

	data_set = g_hash_table_new (g_str_hash, g_str_equal);

	url = self->consumer->get_check_token_url (self, account, TRUE);
	oauth_connection_add_signature (self, "GET", url, data_set);
	g_free (url);

	url = self->consumer->get_check_token_url (self, account, FALSE);
	msg = soup_form_request_new_from_hash ("GET", url, data_set);
	oauth_connection_send_message (self,
				       msg,
				       cancellable,
				       callback,
				       user_data,
				       oauth_connection_check_token,
				       check_token_ready_cb,
				       check_token_data);

	g_free (url);
	g_hash_table_destroy (data_set);
}


gboolean
oauth_connection_check_token_finish (OAuthConnection  *self,
				     GAsyncResult     *result,
				     GError          **error)
{
	if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (result), error))
		return FALSE;
	else
		return TRUE;
}
