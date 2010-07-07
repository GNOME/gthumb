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
#include "facebook-account.h"
#include "facebook-album.h"
#include "facebook-connection.h"
#include "facebook-photo.h"
#include "facebook-service.h"
#include "facebook-user.h"

#define FACEBOOK_MAX_IMAGE_SIZE 720

typedef struct {
	FacebookAlbum       *album;
	GList               *file_list;
	FacebookVisibility   visibility_level;
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


struct _FacebookServicePrivate
{
	FacebookConnection *conn;
	FacebookUser       *user;
	PostPhotosData     *post_photos;
};


static gpointer parent_class = NULL;


static void
facebook_service_finalize (GObject *object)
{
	FacebookService *self;

	self = FACEBOOK_SERVICE (object);

	_g_object_unref (self->priv->conn);
	_g_object_unref (self->priv->user);
	post_photos_data_free (self->priv->post_photos);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
facebook_service_class_init (FacebookServiceClass *klass)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (FacebookServicePrivate));

	object_class = (GObjectClass*) klass;
	object_class->finalize = facebook_service_finalize;
}


static void
facebook_service_init (FacebookService *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, FACEBOOK_TYPE_SERVICE, FacebookServicePrivate);
	self->priv->conn = NULL;
	self->priv->user = NULL;
	self->priv->post_photos = NULL;
}


GType
facebook_service_get_type (void)
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (FacebookServiceClass),
			NULL,
			NULL,
			(GClassInitFunc) facebook_service_class_init,
			NULL,
			NULL,
			sizeof (FacebookService),
			0,
			(GInstanceInitFunc) facebook_service_init
		};

		type = g_type_register_static (G_TYPE_OBJECT,
					       "FacebookService",
					       &type_info,
					       0);
	}

	return type;
}


FacebookService *
facebook_service_new (FacebookConnection *conn)
{
	FacebookService *self;

	self = (FacebookService *) g_object_new (FACEBOOK_TYPE_SERVICE, NULL);
	self->priv->conn = g_object_ref (conn);

	return self;
}


/* -- facebook_service_get_user_info -- */


static void
get_logged_in_user_ready_cb (SoupSession *session,
			     SoupMessage *msg,
			     gpointer     user_data)
{
	FacebookService    *self = user_data;
	GSimpleAsyncResult *result;
	SoupBuffer         *body;
	DomDocument        *doc = NULL;
	GError             *error = NULL;

	result = facebook_connection_get_result (self->priv->conn);

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
	if (facebook_utils_parse_response (body, &doc, &error)) {
		DomElement *root;
		char       *uid = NULL;

		root = DOM_ELEMENT (doc)->first_child;
		if (g_strcmp0 (root->tag_name, "users_getLoggedInUser_response") == 0)
			uid = g_strdup (dom_element_get_inner_text (root));

		if (uid == NULL) {
			error = g_error_new_literal (FACEBOOK_CONNECTION_ERROR, 0, _("Unknown error"));
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
facebook_service_get_logged_in_user (FacebookService     *self,
				     GCancellable        *cancellable,
				     GAsyncReadyCallback  callback,
				     gpointer             user_data)
{
	GHashTable  *data_set;
	SoupMessage *msg;

	gth_task_progress (GTH_TASK (self->priv->conn), _("Connecting to the server"), _("Getting account information"), TRUE, 0.0);

	data_set = g_hash_table_new (g_str_hash, g_str_equal);
	g_hash_table_insert (data_set, "method", "facebook.users.getLoggedInUser");
	facebook_connection_add_api_sig (self->priv->conn, data_set);
	msg = soup_form_request_new_from_hash ("POST", FACEBOOK_HTTPS_REST_SERVER, data_set);
	facebook_connection_send_message (self->priv->conn,
					  msg,
					  cancellable,
					  callback,
					  user_data,
					  facebook_service_get_logged_in_user,
					  get_logged_in_user_ready_cb,
					  self);

	g_hash_table_destroy (data_set);
}


char *
facebook_service_get_logged_in_user_finish (FacebookService  *self,
					    GAsyncResult     *result,
					    GError          **error)
{
	if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (result), error))
		return NULL;
	else
		return g_strdup (g_simple_async_result_get_op_res_gpointer (G_SIMPLE_ASYNC_RESULT (result)));
}


/* -- facebook_service_get_user_info -- */


static void
get_user_info_ready_cb (SoupSession *session,
			SoupMessage *msg,
			gpointer     user_data)
{
	FacebookService      *self = user_data;
	GSimpleAsyncResult *result;
	SoupBuffer         *body;
	DomDocument        *doc = NULL;
	GError             *error = NULL;

	result = facebook_connection_get_result (self->priv->conn);

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
	if (facebook_utils_parse_response (body, &doc, &error)) {
		DomElement   *node;
		FacebookUser *user = NULL;

		for (node = DOM_ELEMENT (doc)->first_child; node; node = node->next_sibling) {
			if (g_strcmp0 (node->tag_name, "users_getInfo_response") == 0) {
				DomElement *child;

				for (child = node->first_child; child; child = child->next_sibling) {
					if (g_strcmp0 (child->tag_name, "user") == 0) {
						user = facebook_user_new ();
						dom_domizable_load_from_element (DOM_DOMIZABLE (user), child);
						g_simple_async_result_set_op_res_gpointer (result, user, (GDestroyNotify) g_object_unref);
					}
				}
			}
		}

		if (user == NULL) {
			error = g_error_new_literal (FACEBOOK_CONNECTION_ERROR, 0, _("Unknown error"));
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
facebook_service_get_user_info (FacebookService     *self,
				const char          *fields,
				GCancellable        *cancellable,
				GAsyncReadyCallback  callback,
				gpointer             user_data)
{
	GHashTable  *data_set;
	SoupMessage *msg;

	gth_task_progress (GTH_TASK (self->priv->conn), _("Connecting to the server"), _("Getting account information"), TRUE, 0.0);

	data_set = g_hash_table_new (g_str_hash, g_str_equal);
	g_hash_table_insert (data_set, "method", "facebook.users.getInfo");
	g_hash_table_insert (data_set, "uids", (char *) facebook_connection_get_user_id (self->priv->conn));
	g_hash_table_insert (data_set, "fields", (char *) fields);
	facebook_connection_add_api_sig (self->priv->conn, data_set);
	msg = soup_form_request_new_from_hash ("POST", FACEBOOK_HTTPS_REST_SERVER, data_set);
	facebook_connection_send_message (self->priv->conn,
					  msg,
					  cancellable,
					  callback,
					  user_data,
					  facebook_service_get_user_info,
					  get_user_info_ready_cb,
					  self);

	g_hash_table_destroy (data_set);
}


FacebookUser *
facebook_service_get_user_info_finish (FacebookService  *self,
				       GAsyncResult     *result,
				       GError          **error)
{
	if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (result), error))
		return NULL;
	else
		return g_object_ref (g_simple_async_result_get_op_res_gpointer (G_SIMPLE_ASYNC_RESULT (result)));
}


/* -- facebook_service_get_albums -- */


static void
get_albums_ready_cb (SoupSession *session,
		     SoupMessage *msg,
		     gpointer     user_data)
{
	FacebookService    *self = user_data;
	GSimpleAsyncResult *result;
	SoupBuffer         *body;
	DomDocument        *doc = NULL;
	GError             *error = NULL;

	result = facebook_connection_get_result (self->priv->conn);

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
	if (facebook_utils_parse_response (body, &doc, &error)) {
		DomElement *node;
		GList      *albums = NULL;

		for (node = DOM_ELEMENT (doc)->first_child; node; node = node->next_sibling) {
			if (g_strcmp0 (node->tag_name, "photos_getAlbums_response") == 0) {
				DomElement *child;

				for (child = node->first_child; child; child = child->next_sibling) {
					if (g_strcmp0 (child->tag_name, "album") == 0) {
						FacebookAlbum *album;

						album = facebook_album_new ();
						dom_domizable_load_from_element (DOM_DOMIZABLE (album), child);
						albums = g_list_prepend (albums, album);
					}
				}
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
facebook_service_get_albums (FacebookService     *self,
			     const char          *user_id,
			     GCancellable        *cancellable,
			     GAsyncReadyCallback  callback,
			     gpointer             user_data)
{
	GHashTable  *data_set;
	SoupMessage *msg;

	g_return_if_fail (user_id != NULL);

	gth_task_progress (GTH_TASK (self->priv->conn), _("Getting the album list"), NULL, TRUE, 0.0);

	data_set = g_hash_table_new (g_str_hash, g_str_equal);
	g_hash_table_insert (data_set, "method", "facebook.photos.getAlbums");
	g_hash_table_insert (data_set, "uid", (char *) user_id);
	facebook_connection_add_api_sig (self->priv->conn, data_set);
	msg = soup_form_request_new_from_hash ("GET", FACEBOOK_HTTPS_REST_SERVER, data_set);
	facebook_connection_send_message (self->priv->conn,
					  msg,
					  cancellable,
					  callback,
					  user_data,
					  facebook_service_get_albums,
					  get_albums_ready_cb,
					  self);

	g_hash_table_destroy (data_set);
}


GList *
facebook_service_get_albums_finish (FacebookService  *service,
				    GAsyncResult     *result,
				    GError          **error)
{
	if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (result), error))
		return NULL;
	else
		return _g_object_list_ref (g_simple_async_result_get_op_res_gpointer (G_SIMPLE_ASYNC_RESULT (result)));
}


/* -- facebook_service_create_album -- */


static void
create_album_ready_cb (SoupSession *session,
		       SoupMessage *msg,
		       gpointer     user_data)
{
	FacebookService    *self = user_data;
	GSimpleAsyncResult *result;
	SoupBuffer         *body;
	DomDocument        *doc = NULL;
	GError             *error = NULL;

	result = facebook_connection_get_result (self->priv->conn);

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
	if (facebook_utils_parse_response (body, &doc, &error)) {
		DomElement    *node;
		FacebookAlbum *album = NULL;

		for (node = DOM_ELEMENT (doc)->first_child; node; node = node->next_sibling) {
			if (g_strcmp0 (node->tag_name, "photos_createAlbum_response") == 0) {
				album = facebook_album_new ();
				dom_domizable_load_from_element (DOM_DOMIZABLE (album), node);
				break;
			}
		}

		if (album == NULL) {
			error = g_error_new_literal (FACEBOOK_CONNECTION_ERROR, 0, _("Unknown error"));
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


static const char *
get_privacy_from_visibility (FacebookVisibility visibility)
{
	char *value = NULL;

	switch (visibility) {
	case FACEBOOK_VISIBILITY_EVERYONE:
		value = "{ value: \"EVERYONE\" }";
		break;

	case FACEBOOK_VISIBILITY_ALL_FRIENDS:
		value = "{ value: \"ALL_FRIENDS\" }";
		break;

	case FACEBOOK_VISIBILITY_SELF:
		value = "{ value: \"SELF\" }";
		break;

	default:
		break;
	}

	return value;
}


void
facebook_service_create_album (FacebookService     *self,
			       FacebookAlbum       *album,
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

	data_set = g_hash_table_new (g_str_hash, g_str_equal);
	g_hash_table_insert (data_set, "method", "facebook.photos.createAlbum");
	g_hash_table_insert (data_set, "name", album->name);
	if (album->description != NULL)
		g_hash_table_insert (data_set, "description", album->description);
	if (album->location != NULL)
		g_hash_table_insert (data_set, "location", album->location);
	privacy = get_privacy_from_visibility (album->visibility);
	if (privacy != NULL)
		g_hash_table_insert (data_set, "privacy", (char *) privacy);
	facebook_connection_add_api_sig (self->priv->conn, data_set);
	msg = soup_form_request_new_from_hash ("POST", FACEBOOK_HTTPS_REST_SERVER, data_set);
	facebook_connection_send_message (self->priv->conn,
					  msg,
					  cancellable,
					  callback,
					  user_data,
					  facebook_service_create_album,
					  create_album_ready_cb,
					  self);

	g_hash_table_destroy (data_set);
}


FacebookAlbum *
facebook_service_create_album_finish (FacebookService  *self,
				      GAsyncResult     *result,
				      GError          **error)
{
	if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (result), error))
		return NULL;
	else
		return g_object_ref (g_simple_async_result_get_op_res_gpointer (G_SIMPLE_ASYNC_RESULT (result)));
}


/* -- facebook_service_upload_photos -- */


static void
upload_photos_done (FacebookService *self,
		    GError          *error)
{
	GSimpleAsyncResult *result;

	result = facebook_connection_get_result (self->priv->conn);
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


static void facebook_service_upload_current_file (FacebookService *self);


static void
upload_photo_ready_cb (SoupSession *session,
		       SoupMessage *msg,
		       gpointer     user_data)
{
	FacebookService *self = user_data;
	SoupBuffer    *body;
	DomDocument   *doc = NULL;
	GError        *error = NULL;
	GthFileData   *file_data;

	if (msg->status_code != 200) {
		GError *error;

		error = g_error_new_literal (SOUP_HTTP_ERROR, msg->status_code, soup_status_get_phrase (msg->status_code));
		upload_photos_done (self, error);
		g_error_free (error);

		return;
	}

	body = soup_message_body_flatten (msg->response_body);
	if (facebook_utils_parse_response (body, &doc, &error)) {
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
	facebook_service_upload_current_file (self);
}


static void
upload_photo_file_buffer_ready_cb (void     **buffer,
				   gsize      count,
				   GError    *error,
				   gpointer   user_data)
{
	FacebookService *self = user_data;
	GthFileData     *file_data;
	SoupMultipart   *multipart;
	GthPixbufSaver  *saver;
	char            *uri;
	SoupBuffer      *body;
	SoupMessage     *msg;

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

		g_hash_table_insert (data_set, "method", "facebook.photos.upload");

		title = gth_file_data_get_attribute_as_string (file_data, "general::title");
		description = gth_file_data_get_attribute_as_string (file_data, "general::description");
		if (description != NULL)
			g_hash_table_insert (data_set, "caption", description);
		else if (title != NULL)
			g_hash_table_insert (data_set, "caption", title);

		if (self->priv->post_photos->album != NULL)
			g_hash_table_insert (data_set, "aid", self->priv->post_photos->album->id);

		facebook_connection_add_api_sig (self->priv->conn, data_set);

		keys = g_hash_table_get_keys (data_set);
		for (scan = keys; scan; scan = scan->next) {
			char *key = scan->data;
			soup_multipart_append_form_string (multipart, key, g_hash_table_lookup (data_set, key));
		}

		g_list_free (keys);
		g_hash_table_unref (data_set);
	}

	/* the file part: rotate and scale the image if required */

	saver = gth_main_get_pixbuf_saver (gth_file_data_get_mime_type (file_data));
	if (saver != NULL) {
		GInputStream *stream;
		GdkPixbuf    *pixbuf;
		GdkPixbuf    *tmp_pixbuf;
		int           width;
		int           height;

		stream = g_memory_input_stream_new_from_data (*buffer, count, NULL);
		pixbuf = gdk_pixbuf_new_from_stream (stream, NULL, &error);
		if (pixbuf == NULL) {
			g_object_unref (stream);
			soup_multipart_free (multipart);
			upload_photos_done (self, error);
			return;
		}

		g_object_unref (stream);

		tmp_pixbuf = gdk_pixbuf_apply_embedded_orientation (pixbuf);
		g_object_unref (pixbuf);
		pixbuf = tmp_pixbuf;

		width = gdk_pixbuf_get_width (pixbuf);
		height = gdk_pixbuf_get_height (pixbuf);
		if (scale_keeping_ratio (&width,
					 &height,
					 FACEBOOK_MAX_IMAGE_SIZE,
					 FACEBOOK_MAX_IMAGE_SIZE,
					 FALSE))
		{
			tmp_pixbuf = _gdk_pixbuf_scale_simple_safe (pixbuf, width, height, GDK_INTERP_BILINEAR);
			g_object_unref (pixbuf);
			pixbuf = tmp_pixbuf;
		}

		g_free (*buffer);
		*buffer = NULL;

		if (! gth_pixbuf_saver_save_pixbuf (saver,
						    pixbuf,
						    (char **)buffer,
						    &count,
						    gth_file_data_get_mime_type (file_data),
						    &error))
		{
			g_object_unref (pixbuf);
			g_object_unref (saver);
			soup_multipart_free (multipart);
			upload_photos_done (self, error);
			return;
		}

		g_object_unref (pixbuf);
		g_object_unref (saver);
	}

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

	msg = soup_form_request_new_from_multipart (FACEBOOK_HTTPS_REST_SERVER, multipart);
	facebook_connection_send_message (self->priv->conn,
					  msg,
					  self->priv->post_photos->cancellable,
					  self->priv->post_photos->callback,
					  self->priv->post_photos->user_data,
					  facebook_service_upload_photos,
					  upload_photo_ready_cb,
					  self);

	soup_multipart_free (multipart);
}


static void
facebook_service_upload_current_file (FacebookService *self)
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
	FacebookService *self = user_data;
	GList           *scan;

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
	facebook_service_upload_current_file (self);
}


void
facebook_service_upload_photos (FacebookService     *self,
				FacebookAlbum       *album,
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
facebook_service_upload_photos_finish (FacebookService  *self,
				       GAsyncResult     *result,
				       GError          **error)
{
	if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (result), error))
		return NULL;
	else
		return _g_string_list_dup (g_simple_async_result_get_op_res_gpointer (G_SIMPLE_ASYNC_RESULT (result)));
}


#if 0

/* -- facebook_service_list_photos -- */


static void
list_photos_ready_cb (SoupSession *session,
		      SoupMessage *msg,
		      gpointer     user_data)
{
	FacebookService      *self = user_data;
	GSimpleAsyncResult *result;
	SoupBuffer         *body;
	DomDocument        *doc = NULL;
	GError             *error = NULL;

	result = facebook_connection_get_result (self->priv->conn);

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
	if (facebook_utils_parse_response (body, &doc, &error)) {
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
						FacebookPhoto *photo;

						photo = facebook_photo_new ();
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
facebook_service_list_photos (FacebookService       *self,
			    FacebookPhotoset      *photoset,
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
	g_hash_table_insert (data_set, "method", "facebook.photosets.getPhotos");
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
	facebook_connection_add_api_sig (self->priv->conn, data_set);
	msg = soup_form_request_new_from_hash ("GET", "http://api.facebook.com/services/rest", data_set);
	facebook_connection_send_message (self->priv->conn,
					msg,
					cancellable,
					callback,
					user_data,
					facebook_service_list_photos,
					list_photos_ready_cb,
					self);

	g_hash_table_destroy (data_set);
}


GList *
facebook_service_list_photos_finish (FacebookService  *self,
				   GAsyncResult   *result,
				   GError        **error)
{
	if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (result), error))
		return NULL;
	else
		return _g_object_list_ref (g_simple_async_result_get_op_res_gpointer (G_SIMPLE_ASYNC_RESULT (result)));
}

#endif


/* utilities */


GList *
facebook_accounts_load_from_file (void)
{
	GList       *accounts = NULL;
	char        *filename;
	char        *buffer;
	gsize        len;
	DomDocument *doc;

	filename = gth_user_dir_get_file (GTH_DIR_CONFIG, GTHUMB_DIR, "accounts", "facebook.xml", NULL);
	if (! g_file_get_contents (filename, &buffer, &len, NULL)) {
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
					FacebookAccount *account;

					account = facebook_account_new ();
					dom_domizable_load_from_element (DOM_DOMIZABLE (account), child);

					accounts = g_list_prepend (accounts, account);
				}
			}

			accounts = g_list_reverse (accounts);
		}
	}

	g_object_unref (doc);
	g_free (buffer);
	g_free (filename);

	return accounts;
}


FacebookAccount *
facebook_accounts_find_default (GList *accounts)
{
	GList *scan;

	for (scan = accounts; scan; scan = scan->next) {
		FacebookAccount *account = scan->data;

		if (account->is_default)
			return g_object_ref (account);
	}

	return NULL;
}


void
facebook_accounts_save_to_file (GList         *accounts,
			      FacebookAccount *default_account)
{
	DomDocument *doc;
	DomElement  *root;
	GList       *scan;
	char        *buffer;
	gsize        len;
	char        *filename;
	GFile       *file;

	doc = dom_document_new ();
	root = dom_document_create_element (doc, "accounts", NULL);
	dom_element_append_child (DOM_ELEMENT (doc), root);
	for (scan = accounts; scan; scan = scan->next) {
		FacebookAccount *account = scan->data;
		DomElement    *node;

		if ((default_account != NULL) && g_strcmp0 (account->username, default_account->username) == 0)
			account->is_default = TRUE;
		else
			account->is_default = FALSE;
		node = dom_domizable_create_element (DOM_DOMIZABLE (account), doc);
		dom_element_append_child (root, node);
	}

	gth_user_dir_make_dir_for_file (GTH_DIR_CONFIG, GTHUMB_DIR, "accounts", "facebook.xml", NULL);
	filename = gth_user_dir_get_file (GTH_DIR_CONFIG, GTHUMB_DIR, "accounts", "facebook.xml", NULL);
	file = g_file_new_for_path (filename);
	buffer = dom_document_dump (doc, &len);
	g_write_file (file, FALSE, G_FILE_CREATE_PRIVATE | G_FILE_CREATE_REPLACE_DESTINATION, buffer, len, NULL, NULL);

	g_free (buffer);
	g_object_unref (file);
	g_free (filename);
	g_object_unref (doc);
}
