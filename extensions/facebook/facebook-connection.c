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
	SoupMessage        *msg;
	char               *token;
	GCancellable       *cancellable;
	GSimpleAsyncResult *result;
};


G_DEFINE_TYPE (FacebookConnection, facebook_connection, GTH_TYPE_TASK)


static void
facebook_connection_finalize (GObject *object)
{
	FacebookConnection *self;

	self = FACEBOOK_CONNECTION (object);

	_g_object_unref (self->priv->result);
	_g_object_unref (self->priv->cancellable);
	g_free (self->priv->token);
	_g_object_unref (self->priv->session);

	G_OBJECT_CLASS (facebook_connection_parent_class)->finalize (object);
}


static void
facebook_connection_exec (GthTask *base)
{
	/* void */
}


static void
facebook_connection_cancelled (GthTask *base)
{
	FacebookConnection *self = FACEBOOK_CONNECTION (base);

	if ((self->priv->session == NULL) || (self->priv->msg == NULL))
		return;

	soup_session_cancel_message (self->priv->session, self->priv->msg, SOUP_STATUS_CANCELLED);
}


static void
facebook_connection_class_init (FacebookConnectionClass *klass)
{
	GObjectClass *object_class;
	GthTaskClass *task_class;

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
	self->priv->msg = NULL;
	self->priv->token = NULL;
	self->priv->cancellable = NULL;
	self->priv->result = NULL;
}


FacebookConnection *
facebook_connection_new (void)
{
	return (FacebookConnection *) g_object_new (FACEBOOK_TYPE_CONNECTION, NULL);
}


void
facebook_connection_set_access_token (FacebookConnection *self,
				      const char         *token)
{
	_g_strset (&self->priv->token, token);
}


const char *
facebook_connection_get_access_token (FacebookConnection *self)
{
	return self->priv->token;
}


void
facebook_connection_add_access_token (FacebookConnection *self,
				      GHashTable         *data_set)
{
	g_return_if_fail (self->priv->token != NULL);

	g_hash_table_insert (data_set, "access_token", self->priv->token);
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
	self->priv->result = g_simple_async_result_new (G_OBJECT (self),
							callback,
							user_data,
							source_tag);

	self->priv->msg = msg;
	g_object_add_weak_pointer (G_OBJECT (msg), (gpointer *) &self->priv->msg);

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


/* utilities */


static char *
get_access_type_name (FacebookAccessType access_type)
{
	char *name = NULL;

	switch (access_type) {
	case FACEBOOK_ACCESS_READ:
		name = "";
		break;

	case FACEBOOK_ACCESS_WRITE:
		name = "publish_actions ";
		break;
	}

	return name;
}


char *
facebook_utils_get_authorization_url (FacebookAccessType access_type)
{
	GHashTable *data_set;
	GString    *link;
	GList      *keys;
	GList      *scan;

	data_set = g_hash_table_new (g_str_hash, g_str_equal);
	g_hash_table_insert (data_set, "client_id", GTHUMB_FACEBOOK_API_KEY);
	g_hash_table_insert (data_set, "redirect_uri", FACEBOOK_REDIRECT_URI);
	g_hash_table_insert (data_set, "scope", get_access_type_name (access_type));
	g_hash_table_insert (data_set, "response_type", "token");

	link = g_string_new ("https://www.facebook.com/dialog/oauth?");
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


gboolean
facebook_utils_parse_response (SoupMessage  *msg,
			       JsonNode    **node,
			       GError      **error)
{
	JsonParser *parser;
	SoupBuffer *body;

	g_return_val_if_fail (msg != NULL, FALSE);
	g_return_val_if_fail (node != NULL, FALSE);

	*node = NULL;

	if ((msg->status_code != 200) && (msg->status_code != 400)) {
		*error = g_error_new (SOUP_HTTP_ERROR,
				      msg->status_code,
				      "%s",
				      soup_status_get_phrase (msg->status_code));
		return FALSE;
	}

	body = soup_message_body_flatten (msg->response_body);
	parser = json_parser_new ();
	if (json_parser_load_from_data (parser, body->data, body->length, error)) {
		JsonObject *obj;

		*node = json_node_copy (json_parser_get_root (parser));

		obj = json_node_get_object (*node);
		if (json_object_has_member (obj, "error")) {
			JsonObject *error_obj;

			error_obj = json_object_get_object_member (obj, "error");
			*error = g_error_new (FACEBOOK_CONNECTION_ERROR,
					      json_object_get_int_member (error_obj, "code"),
					      "%s",
					      json_object_get_string_member (error_obj, "message"));

			json_node_free (*node);
			*node = NULL;
		}
	}

	g_object_unref (parser);
	soup_buffer_free (body);

	return *node != NULL;
}
