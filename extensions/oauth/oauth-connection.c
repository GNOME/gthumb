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
oauth_connection_new (OAuthServer *server)
{
	OAuthConnection *self;

	self = (OAuthConnection *) g_object_new (OAUTH_TYPE_CONNECTION, NULL);
	self->consumer = server;

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
	self->priv->result = g_simple_async_result_new (G_OBJECT (soup_session_cb_data),
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
	char     *signature;

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

	/* Create parameter string */

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
	g_string_append (signature_key, self->consumer->consumer_key);
	g_string_append (signature_key, "&");
	if (self->priv->token != NULL)
		g_string_append (signature_key, self->priv->token);
	g_free (self->priv->signature);
	self->priv->signature = g_compute_signature_for_string (G_CHECKSUM_SHA1,
							        signature_key,
							        base_string->str,
							        base_string->len);
	g_hash_table_insert (parameters, "oauth_signature", self->priv->signature);

	g_string_free (signature_key, TRUE);
	g_string_free (base_string, TRUE);
	g_list_free (keys);
	g_string_free (param_string, TRUE);
}


/* -- oauth_connection_login_request -- */


static void
login_request_ready_cb (SoupSession *session,
			SoupMessage *msg,
			gpointer     user_data)
{
	OAuthConnection    *self = user_data;
	GSimpleAsyncResult *result;
	SoupBuffer         *body;
	GError             *error = NULL;

	result = oauth_connection_get_result (self->priv->conn);

	if (msg->status_code != 200) {
		g_simple_async_result_set_error (result,
						 SOUP_HTTP_ERROR,
						 msg->status_code,
						 "%s",
						 soup_status_get_phrase (msg->status_code));
		g_simple_async_result_complete_in_idle (result);
		return;
	}

	body = soup_message_body_flatten (msg->response_body);
	self->consumer->login_request_result (msg, body, result);
	g_simple_async_result_complete_in_idle (result);

	soup_buffer_free (body);
}


void
oauth_connection_login_request (OAuthConnection     *self,
				GCancellable        *cancellable,
				GAsyncReadyCallback  callback,
				gpointer             user_data)
{
	GHashTable  *data_set;
	SoupMessage *msg;

	gth_task_progress (GTH_TASK (self->priv->conn), _("Connecting to the server"), _("Getting account information"), TRUE, 0.0);

	data_set = g_hash_table_new (g_str_hash, g_str_equal);
	oauth_connection_add_signature (self->priv->conn, "POST", self->consumer->login_request_uri, data_set);
	msg = soup_form_request_new_from_hash ("POST", self->consumer->login_request_uri, data_set);
	oauth_connection_send_message (self->priv->conn,
				       msg,
				       cancellable,
				       callback,
				       user_data,
				       oauth_connection_login_request,
				       login_request_ready_cb,
				       self);

	g_hash_table_destroy (data_set);
}


gboolean
oauth_connection_login_request_finish (OAuthConnection  *self,
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

}


/* -- oauth_connection_get_access_token -- */


void
oauth_connection_get_access_token (OAuthConnection     *self,
				   GCancellable        *cancellable,
				   GAsyncReadyCallback  callback,
				   gpointer             user_data)
{
}


gboolean
oauth_connection_get_access_token_finish (OAuthConnection  *self,
					  GAsyncResult     *result,
					  GError          **error)
{

}


#if 0


static void
connection_frob_ready_cb (SoupSession *session,
			  SoupMessage *msg,
			  gpointer     user_data)
{
	OAuthConnection *self = user_data;
	SoupBuffer       *body;
	DomDocument      *doc = NULL;
	GError           *error = NULL;

	g_free (self->priv->frob);
	self->priv->frob = NULL;

	body = soup_message_body_flatten (msg->response_body);
	if (oauth_utils_parse_response (body, &doc, &error)) {
		DomElement *root;
		DomElement *child;

		root = DOM_ELEMENT (doc)->first_child;
		for (child = root->first_child; child; child = child->next_sibling)
			if (g_strcmp0 (child->tag_name, "frob") == 0)
				self->priv->frob = g_strdup (dom_element_get_inner_text (child));

		if (self->priv->frob == NULL) {
			error = g_error_new_literal (OAUTH_CONNECTION_ERROR, 0, _("Unknown error"));
			g_simple_async_result_set_from_error (self->priv->result, error);
		}
		else
			g_simple_async_result_set_op_res_gboolean (self->priv->result, TRUE);

		g_object_unref (doc);
	}
	else
		g_simple_async_result_set_from_error (self->priv->result, error);

	g_simple_async_result_complete_in_idle (self->priv->result);

	soup_buffer_free (body);
}


void
oauth_connection_get_frob (OAuthConnection    *self,
			    GCancellable        *cancellable,
			    GAsyncReadyCallback  callback,
			    gpointer             user_data)
{
	GHashTable  *data_set;
	SoupMessage *msg;

	gth_task_progress (GTH_TASK (self), _("Connecting to the server"), NULL, TRUE, 0.0);

	g_free (self->priv->token);
	self->priv->token = NULL;

	data_set = g_hash_table_new (g_str_hash, g_str_equal);
	g_hash_table_insert (data_set, "method", "oauth.auth.getFrob");
	oauth_connection_add_api_sig (self, data_set);
	msg = soup_form_request_new_from_hash ("GET", self->consumer->rest_url, data_set);
	oauth_connection_send_message (self,
					msg,
					cancellable,
					callback,
					user_data,
					oauth_connection_get_frob,
					connection_frob_ready_cb,
					self);

	g_hash_table_destroy (data_set);
}


gboolean
oauth_connection_get_frob_finish (OAuthConnection  *self,
				   GAsyncResult      *result,
				   GError           **error)
{
	if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (result), error))
		return FALSE;
	else
		return TRUE;
}


static char *
get_access_type_name (OAuthAccessType access_type)
{
	char *name = NULL;

	switch (access_type) {
	case OAUTH_ACCESS_READ:
		name = "read";
		break;

	case OAUTH_ACCESS_WRITE:
		name = "write";
		break;

	case OAUTH_ACCESS_DELETE:
		name = "delete";
		break;
	}

	return name;
}

char *
oauth_connection_get_login_link (OAuthConnection *self,
				  OAuthAccessType  access_type)
{
	GHashTable *data_set;
	GString    *link;
	GList      *keys;
	GList      *scan;

	g_return_val_if_fail (self->priv->frob != NULL, NULL);

	data_set = g_hash_table_new (g_str_hash, g_str_equal);
	g_hash_table_insert (data_set, "frob", self->priv->frob);
	g_hash_table_insert (data_set, "perms", get_access_type_name (access_type));
	oauth_connection_add_api_sig (self, data_set);

	link = g_string_new (self->consumer->authentication_url);
	g_string_append (link, "?");
	keys = g_hash_table_get_keys (data_set);
	for (scan = keys; scan; scan = scan->next) {
		char *key = scan->data;

		if (scan != keys)
			g_string_append (link, "&");
		g_string_append (link, key);
		g_string_append (link, "=");
		g_string_append (link, g_hash_table_lookup (data_set, key));
	}

	g_list_free (keys);
	g_hash_table_destroy (data_set);

	return g_string_free (link, FALSE);
}


static void
connection_token_ready_cb (SoupSession *session,
			   SoupMessage *msg,
			   gpointer     user_data)
{
	OAuthConnection *self = user_data;
	SoupBuffer       *body;
	DomDocument      *doc = NULL;
	GError           *error = NULL;

	body = soup_message_body_flatten (msg->response_body);
	if (oauth_utils_parse_response (body, &doc, &error)) {
		DomElement *response;
		DomElement *auth;

		response = DOM_ELEMENT (doc)->first_child;
		for (auth = response->first_child; auth; auth = auth->next_sibling) {
			if (g_strcmp0 (auth->tag_name, "auth") == 0) {
				DomElement *node;

				for (node = auth->first_child; node; node = node->next_sibling) {
					if (g_strcmp0 (node->tag_name, "token") == 0) {
						self->priv->token = g_strdup (dom_element_get_inner_text (node));
					}
					else if (g_strcmp0 (node->tag_name, "user") == 0) {
						self->priv->username = g_strdup (dom_element_get_attribute (node, "username"));
						self->priv->user_id = g_strdup (dom_element_get_attribute (node, "nsid"));
					}
				}
			}
		}

		if (self->priv->token == NULL) {
			error = g_error_new_literal (OAUTH_CONNECTION_ERROR, 0, _("Unknown error"));
			g_simple_async_result_set_from_error (self->priv->result, error);
		}
		else
			g_simple_async_result_set_op_res_gboolean (self->priv->result, TRUE);

		g_object_unref (doc);
	}
	else
		g_simple_async_result_set_from_error (self->priv->result, error);

	g_simple_async_result_complete_in_idle (self->priv->result);

	soup_buffer_free (body);
}


void
oauth_connection_get_token (OAuthConnection    *self,
			     GCancellable        *cancellable,
			     GAsyncReadyCallback  callback,
			     gpointer             user_data)
{
	GHashTable  *data_set;
	SoupMessage *msg;

	gth_task_progress (GTH_TASK (self), _("Connecting to the server"), NULL, TRUE, 0.0);

	g_free (self->priv->token);
	self->priv->token = NULL;

	data_set = g_hash_table_new (g_str_hash, g_str_equal);
	g_hash_table_insert (data_set, "method", "oauth.auth.getToken");
	g_hash_table_insert (data_set, "frob", self->priv->frob);
	oauth_connection_add_api_sig (self, data_set);
	msg = soup_form_request_new_from_hash ("GET", self->consumer->rest_url, data_set);
	oauth_connection_send_message (self,
					msg,
					cancellable,
					callback,
					user_data,
					oauth_connection_get_token,
					connection_token_ready_cb,
					self);

	g_hash_table_destroy (data_set);
}


gboolean
oauth_connection_get_token_finish (OAuthConnection  *self,
				    GAsyncResult      *result,
				    GError           **error)
{
	if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (result), error))
		return FALSE;
	else
		return TRUE;
}


void
oauth_connection_set_auth_token (OAuthConnection *self,
				  const char       *value)
{
	g_free (self->priv->token);
	self->priv->token = NULL;
	if (value != NULL)
		self->priv->token = g_strdup (value);
}


const char *
oauth_connection_get_auth_token (OAuthConnection *self)
{
	return self->priv->token;
}


const char *
oauth_connection_get_username (OAuthConnection *self)
{
	return self->priv->username;
}


const char *
oauth_connection_get_user_id (OAuthConnection *self)
{
	return self->priv->user_id;
}


/* utilities */


gboolean
oauth_utils_parse_response (SoupBuffer   *body,
			     DomDocument **doc_p,
			     GError      **error)
{
	DomDocument *doc;
	DomElement  *node;

	doc = dom_document_new ();
	if (! dom_document_load (doc, body->data, body->length, error)) {
		g_object_unref (doc);
		return FALSE;
	}

	for (node = DOM_ELEMENT (doc)->first_child; node; node = node->next_sibling) {
		if (g_strcmp0 (node->tag_name, "rsp") == 0) {
			if (g_strcmp0 (dom_element_get_attribute (node, "stat"), "ok") != 0) {
				DomElement *child;

				for (child = node->first_child; child; child = child->next_sibling) {
					if (g_strcmp0 (child->tag_name, "err") == 0) {
						*error = g_error_new_literal (OAUTH_CONNECTION_ERROR,
									      atoi (dom_element_get_attribute (child, "code")),
									      dom_element_get_attribute (child, "msg"));
					}
				}

				g_object_unref (doc);
				return FALSE;
			}
		}
	}

	*doc_p = doc;

	return TRUE;
}

#endif
