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
#include "facebook-connection.h"
#include "facebook-user.h"


#undef  DEBUG_FACEBOOK_CONNECTION
#define GTHUMB_FACEBOOK_API_KEY "1536ca726857c69843423d0312b9b356"
#define GTHUMB_FACEBOOK_SHARED_SECRET "8c0b99672a9bbc159ebec3c9a8240679"
#define FACEBOOK_API_VERSION "1.0"


GQuark
facebook_connection_error_quark (void)
{
	static GQuark quark;

        if (!quark)
                quark = g_quark_from_static_string ("facebook-connection-error");

        return quark;
}


/* -- FacebookConnection -- */


struct _FacebookConnectionPrivate
{
	SoupSession        *session;
	char               *token;
	char               *secret;
	char               *session_key;
	char               *user_id;
	GCancellable       *cancellable;
	GSimpleAsyncResult *result;
	GChecksum          *checksum;
	char               *call_id;
};


static gpointer parent_class = NULL;


static void
facebook_connection_finalize (GObject *object)
{
	FacebookConnection *self;

	self = FACEBOOK_CONNECTION (object);

	g_free (self->priv->call_id);
	g_checksum_free (self->priv->checksum);
	_g_object_unref (self->priv->result);
	_g_object_unref (self->priv->cancellable);
	g_free (self->priv->user_id);
	g_free (self->priv->session_key);
	g_free (self->priv->secret);
	g_free (self->priv->token);
	_g_object_unref (self->priv->session);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
facebook_connection_exec (GthTask *base)
{
	/* void */
}


static void
facebook_connection_cancelled (GthTask *base)
{
	/* void */
}


static void
facebook_connection_class_init (FacebookConnectionClass *klass)
{
	GObjectClass *object_class;
	GthTaskClass *task_class;

	parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (FacebookConnectionPrivate));

	object_class = (GObjectClass*) klass;
	object_class->finalize = facebook_connection_finalize;

	task_class = (GthTaskClass*) klass;
	task_class->exec = facebook_connection_exec;
	task_class->cancelled = facebook_connection_cancelled;
}


static void
facebook_connection_init (FacebookConnection *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, FACEBOOK_TYPE_CONNECTION, FacebookConnectionPrivate);
	self->priv->session = NULL;
	self->priv->token = NULL;
	self->priv->secret = NULL;
	self->priv->session_key = NULL;
	self->priv->user_id = NULL;
	self->priv->cancellable = NULL;
	self->priv->result = NULL;
	self->priv->checksum = g_checksum_new (G_CHECKSUM_MD5);
	self->priv->call_id = NULL;
}


GType
facebook_connection_get_type (void)
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (FacebookConnectionClass),
			NULL,
			NULL,
			(GClassInitFunc) facebook_connection_class_init,
			NULL,
			NULL,
			sizeof (FacebookConnection),
			0,
			(GInstanceInitFunc) facebook_connection_init
		};

		type = g_type_register_static (GTH_TYPE_TASK,
					       "FacebookConnection",
					       &type_info,
					       0);
	}

	return type;
}


FacebookConnection *
facebook_connection_new (void)
{
	return (FacebookConnection *) g_object_new (FACEBOOK_TYPE_CONNECTION, NULL);
}


void
facebook_connection_send_message (FacebookConnection  *self,
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

#ifdef DEBUG_FACEBOOK_CONNECTION
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
facebook_connection_get_result (FacebookConnection *self)
{
	return self->priv->result;
}


void
facebook_connection_reset_result (FacebookConnection *self)
{
	_g_object_unref (self->priv->result);
	self->priv->result = NULL;
}


void
facebook_connection_add_api_sig (FacebookConnection *self,
			         GHashTable         *data_set)
{
	GList *keys;
	GList *scan;

	g_hash_table_insert (data_set, "api_key", GTHUMB_FACEBOOK_API_KEY);
	g_hash_table_insert (data_set, "v", FACEBOOK_API_VERSION);
	g_hash_table_insert (data_set, "format", "XML");

	if (self->priv->session_key != NULL) {
		GTimeVal current_time;

		g_hash_table_insert (data_set, "session_key", self->priv->session_key);
		g_hash_table_insert (data_set, "ss", "true");

		g_free (self->priv->call_id);
		g_get_current_time (&current_time);
		self->priv->call_id = g_strdup_printf ("%ld.%ld", current_time.tv_sec, current_time.tv_usec);
		g_hash_table_insert (data_set, "call_id", self->priv->call_id);
	}

	g_checksum_reset (self->priv->checksum);
	keys = g_hash_table_get_keys (data_set);
	keys = g_list_sort (keys, (GCompareFunc) strcmp);
	for (scan = keys; scan; scan = scan->next) {
		char *key = scan->data;

		g_checksum_update (self->priv->checksum, (guchar *) key, -1);
		g_checksum_update (self->priv->checksum, (guchar *) "=", -1);
		g_checksum_update (self->priv->checksum, g_hash_table_lookup (data_set, key), -1);
	}
	if (self->priv->session_key != NULL)
		g_checksum_update (self->priv->checksum, (guchar *) self->priv->secret, -1);
	else
		g_checksum_update (self->priv->checksum, (guchar *) GTHUMB_FACEBOOK_SHARED_SECRET, -1);
	g_hash_table_insert (data_set, "sig", (gpointer) g_checksum_get_string (self->priv->checksum));

	g_list_free (keys);
}


static void
create_token_ready_cb (SoupSession *session,
		       SoupMessage *msg,
		       gpointer     user_data)
{
	FacebookConnection *self = user_data;
	SoupBuffer         *body;
	DomDocument        *doc = NULL;
	GError             *error = NULL;

	body = soup_message_body_flatten (msg->response_body);
	if (facebook_utils_parse_response (body, &doc, &error)) {
		DomElement *root;

		root = DOM_ELEMENT (doc)->first_child;
		if (g_strcmp0 (root->tag_name, "auth_createToken_response") == 0)
			self->priv->token = g_strdup (dom_element_get_inner_text (root));

		if (self->priv->token == NULL) {
			error = g_error_new_literal (FACEBOOK_CONNECTION_ERROR, 0, _("Unknown error"));
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
facebook_connection_create_token (FacebookConnection  *self,
				  GCancellable        *cancellable,
				  GAsyncReadyCallback  callback,
				  gpointer             user_data)
{
	GHashTable  *data_set;
	SoupMessage *msg;

	g_free (self->priv->token);
	g_free (self->priv->session_key);
	g_free (self->priv->secret);
	g_free (self->priv->user_id);
	self->priv->token = NULL;
	self->priv->session_key = NULL;
	self->priv->secret = NULL;
	self->priv->user_id = NULL;

	gth_task_progress (GTH_TASK (self), _("Connecting to the server"), NULL, TRUE, 0.0);

	data_set = g_hash_table_new (g_str_hash, g_str_equal);
	g_hash_table_insert (data_set, "method", "facebook.auth.createToken");
	facebook_connection_add_api_sig (self, data_set);
	msg = soup_form_request_new_from_hash ("POST", FACEBOOK_HTTPS_REST_SERVER, data_set);
	facebook_connection_send_message (self,
					  msg,
					  cancellable,
					  callback,
					  user_data,
					  facebook_connection_create_token,
					  create_token_ready_cb,
					  self);

	g_hash_table_destroy (data_set);
}


gboolean
facebook_connection_create_token_finish (FacebookConnection  *self,
					 GAsyncResult        *result,
				         GError             **error)
{
	if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (result), error))
		return FALSE;
	else
		return TRUE;
}


static char *
get_access_type_name (FacebookAccessType access_type)
{
	char *name = NULL;

	switch (access_type) {
	case FACEBOOK_ACCESS_READ:
		name = "read_stream,offline_access";
		break;

	case FACEBOOK_ACCESS_WRITE:
		name = "photo_upload,offline_access";
		break;
	}

	return name;
}


char *
facebook_connection_get_login_link (FacebookConnection *self,
				    FacebookAccessType  access_type)
{
	GHashTable *data_set;
	GString    *link;
	GList      *keys;
	GList      *scan;

	g_return_val_if_fail (self->priv->token != NULL, NULL);

	data_set = g_hash_table_new (g_str_hash, g_str_equal);
	g_hash_table_insert (data_set, "api_key", GTHUMB_FACEBOOK_API_KEY);
	g_hash_table_insert (data_set, "auth_token", self->priv->token);
	g_hash_table_insert (data_set, "req_perms", get_access_type_name (access_type));
	g_hash_table_insert (data_set, "connect_display", "page");
	g_hash_table_insert (data_set, "fbconnect", "true");

	link = g_string_new ("http://www.facebook.com/login.php?");
	keys = g_hash_table_get_keys (data_set);
	for (scan = keys; scan; scan = scan->next) {
		char *key = scan->data;
		char *encoded;

		if (scan != keys)
			g_string_append (link, "&");
		g_string_append (link, key);
		g_string_append (link, "=");
		encoded = soup_uri_encode (g_hash_table_lookup (data_set, key), NULL);
		g_string_append (link, encoded);

		g_free (encoded);
	}

	g_list_free (keys);
	g_hash_table_destroy (data_set);

	return g_string_free (link, FALSE);
}


static void
get_session_ready_cb (SoupSession *session,
		      SoupMessage *msg,
		      gpointer     user_data)
{
	FacebookConnection *self = user_data;
	SoupBuffer         *body;
	DomDocument        *doc = NULL;
	GError             *error = NULL;

	body = soup_message_body_flatten (msg->response_body);
	if (facebook_utils_parse_response (body, &doc, &error)) {
		DomElement *root;

		root = DOM_ELEMENT (doc)->first_child;
		if (g_strcmp0 (root->tag_name, "auth_getSession_response") == 0) {
			DomElement *node;

			for (node = root->first_child; node; node = node->next_sibling) {
				if (g_strcmp0 (node->tag_name, "session_key") == 0) {
					self->priv->session_key = g_strdup (dom_element_get_inner_text (node));
				}
				else if (g_strcmp0 (node->tag_name, "secret") == 0) {
					self->priv->secret = g_strdup (dom_element_get_inner_text (node));
				}
				else if (g_strcmp0 (node->tag_name, "uid") == 0) {
					self->priv->user_id = g_strdup (dom_element_get_inner_text (node));
				}
			}
		}

		if ((self->priv->session_key == NULL) || (self->priv->secret == NULL)) {
			error = g_error_new_literal (FACEBOOK_CONNECTION_ERROR, 0, _("Unknown error"));
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
facebook_connection_get_session (FacebookConnection  *self,
				 GCancellable        *cancellable,
				 GAsyncReadyCallback  callback,
				 gpointer             user_data)
{
	GHashTable  *data_set;
	SoupMessage *msg;

	gth_task_progress (GTH_TASK (self), _("Connecting to the server"), NULL, TRUE, 0.0);

	g_free (self->priv->session_key);
	g_free (self->priv->secret);
	g_free (self->priv->user_id);
	self->priv->session_key = NULL;
	self->priv->secret = NULL;
	self->priv->user_id = NULL;

	data_set = g_hash_table_new (g_str_hash, g_str_equal);
	g_hash_table_insert (data_set, "method", "facebook.auth.getSession");
	g_hash_table_insert (data_set, "auth_token", self->priv->token);
	facebook_connection_add_api_sig (self, data_set);
	msg = soup_form_request_new_from_hash ("POST", FACEBOOK_HTTPS_REST_SERVER, data_set);
	facebook_connection_send_message (self,
					  msg,
					  cancellable,
					  callback,
					  user_data,
					  facebook_connection_get_session,
					  get_session_ready_cb,
					  self);

	g_hash_table_destroy (data_set);
}


gboolean
facebook_connection_get_session_finish (FacebookConnection  *self,
					GAsyncResult        *result,
					GError             **error)
{
	if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (result), error))
		return FALSE;
	else
		return TRUE;
}


void
facebook_connection_set_session (FacebookConnection *self,
				 const char         *session_key,
				 const char         *secret)
{
	_g_strset (&self->priv->session_key, session_key);
	_g_strset (&self->priv->secret, secret);
}


const char *
facebook_connection_get_session_key (FacebookConnection *self)
{
	return self->priv->session_key;
}


const char *
facebook_connection_get_secret (FacebookConnection *self)
{
	return self->priv->secret;
}


const char *
facebook_connection_get_user_id (FacebookConnection *self)
{
	return self->priv->user_id;
}


/* utilities */


gboolean
facebook_utils_parse_response (SoupBuffer   *body,
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
		if (g_strcmp0 (node->tag_name, "error_response") == 0) {
			DomElement *child;
			int         code = 0;
			const char *message = NULL;

			for (child = node->first_child; child; child = child->next_sibling) {
				if (g_strcmp0 (child->tag_name, "error_code") == 0) {
					code = atoi (dom_element_get_inner_text (child));
				}
				else if (g_strcmp0 (child->tag_name, "error_msg") == 0) {
					message = dom_element_get_inner_text (child);
				}
			}

			*error = g_error_new_literal (FACEBOOK_CONNECTION_ERROR,
						      code,
						      message);

			g_object_unref (doc);
			return FALSE;
		}
	}

	*doc_p = doc;

	return TRUE;
}
