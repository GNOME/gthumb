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
#include "photobucket-album.h"
#include "photobucket-consumer.h"
#include "photobucket-photo.h"
#include "photobucket-service.h"


typedef struct {
	PhotobucketAlbum    *album;
	GList               *file_list;
	GCancellable        *cancellable;
        GAsyncReadyCallback  callback;
        gpointer             user_data;
	GList               *current;
	goffset              total_size;
	goffset              uploaded_size;
	int                  n_files;
	int                  uploaded_files;
	GList               *ids;
} PostPhotosData;


static void
post_photos_data_free (PostPhotosData *post_photos)
{
	if (post_photos == NULL)
		return;
	_g_string_list_free (post_photos->ids);
	_g_object_unref (post_photos->cancellable);
	_g_object_list_unref (post_photos->file_list);
	g_free (post_photos);
}


struct _PhotobucketServicePrivate
{
	OAuthConnection *conn;
	PostPhotosData  *post_photos;
};


static gpointer parent_class = NULL;


static void
photobucket_service_finalize (GObject *object)
{
	PhotobucketService *self;

	self = PHOTOBUCKET_SERVICE (object);

	_g_object_unref (self->priv->conn);
	post_photos_data_free (self->priv->post_photos);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
photobucket_service_class_init (PhotobucketServiceClass *klass)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (PhotobucketServicePrivate));

	object_class = (GObjectClass*) klass;
	object_class->finalize = photobucket_service_finalize;
}


static void
photobucket_service_init (PhotobucketService *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, PHOTOBUCKET_TYPE_SERVICE, PhotobucketServicePrivate);
	self->priv->conn = NULL;
	self->priv->post_photos = NULL;
}


GType
photobucket_service_get_type (void)
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (PhotobucketServiceClass),
			NULL,
			NULL,
			(GClassInitFunc) photobucket_service_class_init,
			NULL,
			NULL,
			sizeof (PhotobucketService),
			0,
			(GInstanceInitFunc) photobucket_service_init
		};

		type = g_type_register_static (G_TYPE_OBJECT,
					       "PhotobucketService",
					       &type_info,
					       0);
	}

	return type;
}


PhotobucketService *
photobucket_service_new (OAuthConnection *conn)
{
	PhotobucketService *self;

	self = (PhotobucketService *) g_object_new (PHOTOBUCKET_TYPE_SERVICE, NULL);
	self->priv->conn = g_object_ref (conn);

	return self;
}


#if 0


/* -- photobucket_service_get_user_info -- */


static void
get_logged_in_user_ready_cb (SoupSession *session,
			     SoupMessage *msg,
			     gpointer     user_data)
{
	PhotobucketService *self = user_data;
	GSimpleAsyncResult *result;
	SoupBuffer         *body;
	DomDocument        *doc = NULL;
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
	if (photobucket_utils_parse_response (body, &doc, result, &error)) {
		DomElement *root;
		char       *uid = NULL;

		root = DOM_ELEMENT (doc)->first_child;
		if (g_strcmp0 (root->tag_name, "users_getLoggedInUser_response") == 0)
			uid = g_strdup (dom_element_get_inner_text (root));

		if (uid == NULL) {
			error = g_error_new_literal (OAUTH_CONNECTION_ERROR, 0, _("Unknown error"));
			g_simple_async_result_set_from_error (result, error);
		}
		else
			g_simple_async_result_set_op_res_gpointer (result, uid, g_free);

		g_object_unref (doc);
	}
	else
		g_simple_async_result_set_from_error (result, error);

	g_simple_async_result_complete_in_idle (result);

	soup_buffer_free (body);
}


void
photobucket_service_get_logged_in_user (PhotobucketService  *self,
				        GCancellable        *cancellable,
				        GAsyncReadyCallback  callback,
				        gpointer             user_data)
{
	GHashTable  *data_set;
	SoupMessage *msg;

	gth_task_progress (GTH_TASK (self->priv->conn), _("Connecting to the server"), _("Getting account information"), TRUE, 0.0);

	data_set = g_hash_table_new (g_str_hash, g_str_equal);
	g_hash_table_insert (data_set, "method", "photobucket.users.getLoggedInUser");
	oauth_connection_add_signature (self->priv->conn, "photobucket.users.getLoggedInUser", );
	msg = soup_form_request_new_from_hash ("POST", PHOTOBUCKET_HTTPS_REST_SERVER, data_set);
	oauth_connection_send_message (self->priv->conn,
				       msg,
				       cancellable,
				       callback,
				       user_data,
				       photobucket_service_get_logged_in_user,
				       get_logged_in_user_ready_cb,
				       self);

	g_hash_table_destroy (data_set);
}


char *
photobucket_service_get_logged_in_user_finish (PhotobucketService  *self,
					       GAsyncResult        *result,
					       GError             **error)
{
	if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (result), error))
		return NULL;
	else
		return g_strdup (g_simple_async_result_get_op_res_gpointer (G_SIMPLE_ASYNC_RESULT (result)));
}


/* -- photobucket_service_get_user_info -- */


static void
get_user_info_ready_cb (SoupSession *session,
			SoupMessage *msg,
			gpointer     user_data)
{
	PhotobucketService      *self = user_data;
	GSimpleAsyncResult *result;
	SoupBuffer         *body;
	DomDocument        *doc = NULL;
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
	if (photobucket_utils_parse_response (body, &doc, &error)) {
		DomElement   *node;
		PhotobucketUser *user = NULL;

		for (node = DOM_ELEMENT (doc)->first_child; node; node = node->next_sibling) {
			if (g_strcmp0 (node->tag_name, "users_getInfo_response") == 0) {
				DomElement *child;

				for (child = node->first_child; child; child = child->next_sibling) {
					if (g_strcmp0 (child->tag_name, "user") == 0) {
						user = photobucket_user_new ();
						dom_domizable_load_from_element (DOM_DOMIZABLE (user), child);
						g_simple_async_result_set_op_res_gpointer (result, user, (GDestroyNotify) g_object_unref);
					}
				}
			}
		}

		if (user == NULL) {
			error = g_error_new_literal (OAUTH_CONNECTION_ERROR, 0, _("Unknown error"));
			g_simple_async_result_set_from_error (result, error);
		}

		g_object_unref (doc);
	}
	else
		g_simple_async_result_set_from_error (result, error);

	g_simple_async_result_complete_in_idle (result);

	soup_buffer_free (body);
}


void
photobucket_service_get_user_info (PhotobucketService  *self,
				   const char          *fields,
				   GCancellable        *cancellable,
				   GAsyncReadyCallback  callback,
				   gpointer             user_data)
{
	GHashTable  *data_set;
	SoupMessage *msg;

	gth_task_progress (GTH_TASK (self->priv->conn), _("Connecting to the server"), _("Getting account information"), TRUE, 0.0);

	data_set = g_hash_table_new (g_str_hash, g_str_equal);
	g_hash_table_insert (data_set, "method", "photobucket.users.getInfo");
	g_hash_table_insert (data_set, "uids", (char *) oauth_connection_get_user_id (self->priv->conn));
	g_hash_table_insert (data_set, "fields", (char *) fields);
	oauth_connection_add_api_sig (self->priv->conn, data_set);
	msg = soup_form_request_new_from_hash ("POST", PHOTOBUCKET_HTTPS_REST_SERVER, data_set);
	oauth_connection_send_message (self->priv->conn,
				       msg,
				       cancellable,
				       callback,
				       user_data,
				       photobucket_service_get_user_info,
				       get_user_info_ready_cb,
				       self);

	g_hash_table_destroy (data_set);
}


PhotobucketUser *
photobucket_service_get_user_info_finish (PhotobucketService  *self,
				          GAsyncResult        *result,
				          GError             **error)
{
	if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (result), error))
		return NULL;
	else
		return g_object_ref (g_simple_async_result_get_op_res_gpointer (G_SIMPLE_ASYNC_RESULT (result)));
}


#endif


/* -- photobucket_service_get_albums -- */


static DomElement *
get_content (DomDocument *doc)
{
	DomElement *root;

	for (root = DOM_ELEMENT (doc)->first_child; root; root = root->next_sibling) {
		if (g_strcmp0 (root->tag_name, "response") == 0) {
			DomElement *child;

			for (child = root->first_child; child; child = child->next_sibling)
				if (g_strcmp0 (child->tag_name, "content") == 0)
					return child;
		}
	}

	g_assert_not_reached ();

	return NULL;
}


static void
get_albums_ready_cb (SoupSession *session,
		     SoupMessage *msg,
		     gpointer     user_data)
{
	PhotobucketService *self = user_data;
	GSimpleAsyncResult *result;
	SoupBuffer         *body;
	DomDocument        *doc = NULL;
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
	if (photobucket_utils_parse_response (body, &doc, result, &error)) {
		GList      *albums = NULL;
		DomElement *node;

		for (node = get_content (doc)->first_child; node; node = node->next_sibling) {
			if (g_strcmp0 (node->tag_name, "album") == 0) {
				PhotobucketAlbum *album;

				album = photobucket_album_new ();
				dom_domizable_load_from_element (DOM_DOMIZABLE (album), node);
				albums = g_list_prepend (albums, album);
			}
		}

		albums = g_list_reverse (albums);
		g_simple_async_result_set_op_res_gpointer (result, albums, (GDestroyNotify) _g_object_list_unref);

		g_object_unref (doc);
	}
	else
		g_simple_async_result_set_from_error (result, error);

	g_simple_async_result_complete_in_idle (result);

	soup_buffer_free (body);
}


void
photobucket_service_get_albums (PhotobucketService  *self,
			        PhotobucketAccount  *account,
			        GCancellable        *cancellable,
			        GAsyncReadyCallback  callback,
			        gpointer             user_data)
{
	GHashTable  *data_set;
	char        *url;
	SoupMessage *msg;

	g_return_if_fail (account != NULL);
	g_return_if_fail (account->subdomain != NULL);

	gth_task_progress (GTH_TASK (self->priv->conn), _("Getting the album list"), NULL, TRUE, 0.0);

	data_set = g_hash_table_new (g_str_hash, g_str_equal);
	url = g_strconcat ("http://api.photobucket.com/album/", OAUTH_ACCOUNT (account)->username, NULL);
	oauth_connection_add_signature (self->priv->conn, "GET", url, data_set);
	g_free (url);

	url = g_strconcat ("http://", account->subdomain, "/album/", OAUTH_ACCOUNT (account)->username, NULL);
	msg = soup_form_request_new_from_hash ("GET", url, data_set);
	oauth_connection_send_message (self->priv->conn,
				       msg,
				       cancellable,
				       callback,
				       user_data,
				       photobucket_service_get_albums,
				       get_albums_ready_cb,
				       self);

	g_hash_table_destroy (data_set);
	g_free (url);
}


GList *
photobucket_service_get_albums_finish (PhotobucketService  *service,
				       GAsyncResult        *result,
				       GError             **error)
{
	if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (result), error))
		return NULL;
	else
		return _g_object_list_ref (g_simple_async_result_get_op_res_gpointer (G_SIMPLE_ASYNC_RESULT (result)));
}


/* -- photobucket_service_create_album -- */


static void
create_album_ready_cb (SoupSession *session,
		       SoupMessage *msg,
		       gpointer     user_data)
{
	PhotobucketService *self = user_data;
	GSimpleAsyncResult *result;
	SoupBuffer         *body;
	DomDocument        *doc = NULL;
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
	if (photobucket_utils_parse_response (body, &doc, result, &error)) {
		DomElement    *node;
		PhotobucketAlbum *album = NULL;

		for (node = DOM_ELEMENT (doc)->first_child; node; node = node->next_sibling) {
			if (g_strcmp0 (node->tag_name, "photos_createAlbum_response") == 0) {
				album = photobucket_album_new ();
				dom_domizable_load_from_element (DOM_DOMIZABLE (album), node);
				break;
			}
		}

		if (album == NULL) {
			error = g_error_new_literal (OAUTH_CONNECTION_ERROR, 0, _("Unknown error"));
			g_simple_async_result_set_from_error (result, error);
		}
		else
			g_simple_async_result_set_op_res_gpointer (result, album, (GDestroyNotify) _g_object_unref);

		g_object_unref (doc);
	}
	else
		g_simple_async_result_set_from_error (result, error);

	g_simple_async_result_complete_in_idle (result);

	soup_buffer_free (body);
}


void
photobucket_service_create_album (PhotobucketService  *self,
			          PhotobucketAlbum    *album,
			          GCancellable        *cancellable,
			          GAsyncReadyCallback  callback,
			          gpointer             user_data)
{
	GHashTable  *data_set;
	const char  *privacy;
	SoupMessage *msg;

	g_return_if_fail (album != NULL);
	g_return_if_fail (album->name != NULL);

	gth_task_progress (GTH_TASK (self->priv->conn), _("Creating the new album"), NULL, TRUE, 0.0);

	/* FIXME
	data_set = g_hash_table_new (g_str_hash, g_str_equal);
	g_hash_table_insert (data_set, "method", "photobucket.photos.createAlbum");
	g_hash_table_insert (data_set, "name", album->name);
	if (album->description != NULL)
		g_hash_table_insert (data_set, "description", album->description);
	if (album->location != NULL)
		g_hash_table_insert (data_set, "location", album->location);
	privacy = get_privacy_from_visibility (album->visibility);
	if (privacy != NULL)
		g_hash_table_insert (data_set, "privacy", (char *) privacy);
	oauth_connection_add_api_sig (self->priv->conn, data_set);
	msg = soup_form_request_new_from_hash ("POST", PHOTOBUCKET_HTTPS_REST_SERVER, data_set);
	oauth_connection_send_message (self->priv->conn,
				       msg,
				       cancellable,
				       callback,
				       user_data,
				       photobucket_service_create_album,
				       create_album_ready_cb,
				       self);

	g_hash_table_destroy (data_set);
	*/
}


PhotobucketAlbum *
photobucket_service_create_album_finish (PhotobucketService  *self,
				         GAsyncResult        *result,
				         GError             **error)
{
	if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (result), error))
		return NULL;
	else
		return g_object_ref (g_simple_async_result_get_op_res_gpointer (G_SIMPLE_ASYNC_RESULT (result)));
}


/* -- photobucket_service_upload_photos -- */


static void
upload_photos_done (PhotobucketService *self,
		    GError             *error)
{
	GSimpleAsyncResult *result;

	result = oauth_connection_get_result (self->priv->conn);
	if (error == NULL) {
		self->priv->post_photos->ids = g_list_reverse (self->priv->post_photos->ids);
		g_simple_async_result_set_op_res_gpointer (result, self->priv->post_photos->ids, (GDestroyNotify) _g_string_list_free);
		self->priv->post_photos->ids = NULL;
	}
	else {
		if (self->priv->post_photos->current != NULL) {
			GthFileData *file_data = self->priv->post_photos->current->data;
			char        *msg;

			msg = g_strdup_printf (_("Could not upload '%s': %s"), g_file_info_get_display_name (file_data->info), error->message);
			g_free (error->message);
			error->message = msg;
		}
		g_simple_async_result_set_from_error (result, error);
	}

	g_simple_async_result_complete_in_idle (result);
}


static void photobucket_service_upload_current_file (PhotobucketService *self);


static void
upload_photo_ready_cb (SoupSession *session,
		       SoupMessage *msg,
		       gpointer     user_data)
{
	PhotobucketService *self = user_data;
	GSimpleAsyncResult *result;
	SoupBuffer         *body;
	DomDocument        *doc = NULL;
	GError             *error = NULL;
	GthFileData        *file_data;

	result = oauth_connection_get_result (self->priv->conn);

	if (msg->status_code != 200) {
		GError *error;

		error = g_error_new_literal (SOUP_HTTP_ERROR, msg->status_code, soup_status_get_phrase (msg->status_code));
		upload_photos_done (self, error);
		g_error_free (error);

		return;
	}

	body = soup_message_body_flatten (msg->response_body);
	if (photobucket_utils_parse_response (body, &doc, result, &error)) {
		DomElement *node;

		/* save the photo id */

		for (node = DOM_ELEMENT (doc)->first_child; node; node = node->next_sibling) {
			if (g_strcmp0 (node->tag_name, "pid") == 0) {
				const char *id;

				id = dom_element_get_inner_text (node);
				self->priv->post_photos->ids = g_list_prepend (self->priv->post_photos->ids, g_strdup (id));
			}
		}

		g_object_unref (doc);
	}
	else {
		soup_buffer_free (body);
		upload_photos_done (self, error);
		return;
	}

	soup_buffer_free (body);

	file_data = self->priv->post_photos->current->data;
	self->priv->post_photos->uploaded_size += g_file_info_get_size (file_data->info);
	self->priv->post_photos->current = self->priv->post_photos->current->next;
	photobucket_service_upload_current_file (self);
}


static void
upload_photo_file_buffer_ready_cb (void     **buffer,
				   gsize      count,
				   GError    *error,
				   gpointer   user_data)
{
	PhotobucketService *self = user_data;
	GthFileData        *file_data;
	SoupMultipart      *multipart;
	char               *uri;
	SoupBuffer         *body;
	SoupMessage        *msg;

	if (error != NULL) {
		upload_photos_done (self, error);
		return;
	}

	file_data = self->priv->post_photos->current->data;
	multipart = soup_multipart_new ("multipart/form-data");

	/* the metadata part */

	{
		GHashTable *data_set;
		char       *title;
		char       *description;
		GList      *keys;
		GList      *scan;

		data_set = g_hash_table_new (g_str_hash, g_str_equal);

		g_hash_table_insert (data_set, "method", "photobucket.photos.upload");

		title = gth_file_data_get_attribute_as_string (file_data, "general::title");
		description = gth_file_data_get_attribute_as_string (file_data, "general::description");
		if (description != NULL)
			g_hash_table_insert (data_set, "caption", description);
		else if (title != NULL)
			g_hash_table_insert (data_set, "caption", title);

		if (self->priv->post_photos->album != NULL)
			g_hash_table_insert (data_set, "aid", self->priv->post_photos->album->name);

		oauth_connection_add_signature (self->priv->conn, "POST", "FIXME", data_set); /* FIXME */

		keys = g_hash_table_get_keys (data_set);
		for (scan = keys; scan; scan = scan->next) {
			char *key = scan->data;
			soup_multipart_append_form_string (multipart, key, g_hash_table_lookup (data_set, key));
		}

		g_list_free (keys);
		g_hash_table_unref (data_set);
	}

	/* the file part */

	uri = g_file_get_uri (file_data->file);
	body = soup_buffer_new (SOUP_MEMORY_TEMPORARY, *buffer, count);
	soup_multipart_append_form_file (multipart,
					 NULL,
					 _g_uri_get_basename (uri),
					 gth_file_data_get_mime_type (file_data),
					 body);

	soup_buffer_free (body);
	g_free (uri);

	/* send the file */

	{
		char *details;

		/* Translators: %s is a filename */
		details = g_strdup_printf (_("Uploading '%s'"), g_file_info_get_display_name (file_data->info));
		gth_task_progress (GTH_TASK (self->priv->conn),
				   NULL, details,
				   FALSE,
				   (double) (self->priv->post_photos->uploaded_size + (g_file_info_get_size (file_data->info) / 2.0)) / self->priv->post_photos->total_size);

		g_free (details);
	}

	/* FIXME
	msg = soup_form_request_new_from_multipart (PHOTOBUCKET_HTTPS_REST_SERVER, multipart);
	oauth_connection_send_message (self->priv->conn,
				       msg,
				       self->priv->post_photos->cancellable,
				       self->priv->post_photos->callback,
				       self->priv->post_photos->user_data,
				       photobucket_service_upload_photos,
				       upload_photo_ready_cb,
				       self);
	*/

	soup_multipart_free (multipart);
}


static void
photobucket_service_upload_current_file (PhotobucketService *self)
{
	GthFileData *file_data;

	if (self->priv->post_photos->current == NULL) {
		upload_photos_done (self, NULL);
		return;
	}

	file_data = self->priv->post_photos->current->data;
	g_load_file_async (file_data->file,
			   G_PRIORITY_DEFAULT,
			   self->priv->post_photos->cancellable,
			   upload_photo_file_buffer_ready_cb,
			   self);
}


static void
upload_photos_info_ready_cb (GList    *files,
		             GError   *error,
		             gpointer  user_data)
{
	PhotobucketService *self = user_data;
	GList              *scan;

	if (error != NULL) {
		upload_photos_done (self, error);
		return;
	}

	self->priv->post_photos->file_list = _g_object_list_ref (files);
	for (scan = self->priv->post_photos->file_list; scan; scan = scan->next) {
		GthFileData *file_data = scan->data;

		self->priv->post_photos->total_size += g_file_info_get_size (file_data->info);
		self->priv->post_photos->n_files += 1;
	}

	self->priv->post_photos->current = self->priv->post_photos->file_list;
	photobucket_service_upload_current_file (self);
}


void
photobucket_service_upload_photos (PhotobucketService  *self,
				   PhotobucketAlbum    *album,
				   GList               *file_list, /* GFile list */
				   GCancellable        *cancellable,
				   GAsyncReadyCallback  callback,
				   gpointer             user_data)
{
	gth_task_progress (GTH_TASK (self->priv->conn), _("Uploading the files to the server"), NULL, TRUE, 0.0);

	post_photos_data_free (self->priv->post_photos);
	self->priv->post_photos = g_new0 (PostPhotosData, 1);
	self->priv->post_photos->album = _g_object_ref (album);
	self->priv->post_photos->cancellable = _g_object_ref (cancellable);
	self->priv->post_photos->callback = callback;
	self->priv->post_photos->user_data = user_data;
	self->priv->post_photos->total_size = 0;
	self->priv->post_photos->n_files = 0;

	_g_query_all_metadata_async (file_list,
				     FALSE,
				     TRUE,
				     "*",
				     self->priv->post_photos->cancellable,
				     upload_photos_info_ready_cb,
				     self);
}


GList *
photobucket_service_upload_photos_finish (PhotobucketService  *self,
				          GAsyncResult        *result,
				          GError             **error)
{
	if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (result), error))
		return NULL;
	else
		return _g_string_list_dup (g_simple_async_result_get_op_res_gpointer (G_SIMPLE_ASYNC_RESULT (result)));
}


#if 0

/* -- photobucket_service_list_photos -- */


static void
list_photos_ready_cb (SoupSession *session,
		      SoupMessage *msg,
		      gpointer     user_data)
{
	PhotobucketService      *self = user_data;
	GSimpleAsyncResult *result;
	SoupBuffer         *body;
	DomDocument        *doc = NULL;
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
	if (photobucket_utils_parse_response (body, &doc, &error)) {
		DomElement *response;
		DomElement *node;
		GList      *photos = NULL;

		response = DOM_ELEMENT (doc)->first_child;
		for (node = response->first_child; node; node = node->next_sibling) {
			if (g_strcmp0 (node->tag_name, "photoset") == 0) {
				DomElement *child;
				int         position;

				position = 0;
				for (child = node->first_child; child; child = child->next_sibling) {
					if (g_strcmp0 (child->tag_name, "photo") == 0) {
						PhotobucketPhoto *photo;

						photo = photobucket_photo_new ();
						dom_domizable_load_from_element (DOM_DOMIZABLE (photo), child);
						photo->position = position++;
						photos = g_list_prepend (photos, photo);
					}
				}
			}
		}

		photos = g_list_reverse (photos);
		g_simple_async_result_set_op_res_gpointer (result, photos, (GDestroyNotify) _g_object_list_unref);

		g_object_unref (doc);
	}
	else
		g_simple_async_result_set_from_error (result, error);

	g_simple_async_result_complete_in_idle (result);

	soup_buffer_free (body);
}


void
photobucket_service_list_photos (PhotobucketService       *self,
			    PhotobucketPhotoset      *photoset,
			    const char          *extras,
			    int                  per_page,
			    int                  page,
			    GCancellable        *cancellable,
			    GAsyncReadyCallback  callback,
			    gpointer             user_data)
{
	GHashTable  *data_set;
	char        *s;
	SoupMessage *msg;

	g_return_if_fail (photoset != NULL);

	gth_task_progress (GTH_TASK (self->priv->conn), _("Getting the photo list"), NULL, TRUE, 0.0);

	data_set = g_hash_table_new (g_str_hash, g_str_equal);
	g_hash_table_insert (data_set, "method", "photobucket.photosets.getPhotos");
	g_hash_table_insert (data_set, "photoset_id", photoset->id);
	if (extras != NULL)
		g_hash_table_insert (data_set, "extras", (char *) extras);
	if (per_page > 0) {
		s = g_strdup_printf ("%d", per_page);
		g_hash_table_insert (data_set, "per_page", s);
		g_free (s);
	}
	if (page > 0) {
		s = g_strdup_printf ("%d", page);
		g_hash_table_insert (data_set, "page", s);
		g_free (s);
	}
	oauth_connection_add_api_sig (self->priv->conn, data_set);
	msg = soup_form_request_new_from_hash ("GET", "http://api.photobucket.com/services/rest", data_set);
	oauth_connection_send_message (self->priv->conn,
				       msg,
				       cancellable,
				       callback,
				       user_data,
				       photobucket_service_list_photos,
				       list_photos_ready_cb,
				       self);

	g_hash_table_destroy (data_set);
}


GList *
photobucket_service_list_photos_finish (PhotobucketService  *self,
				   GAsyncResult   *result,
				   GError        **error)
{
	if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (result), error))
		return NULL;
	else
		return _g_object_list_ref (g_simple_async_result_get_op_res_gpointer (G_SIMPLE_ASYNC_RESULT (result)));
}

#endif
