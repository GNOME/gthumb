/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2010 The Free Software Foundation, Inc.
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
#define GDK_PIXBUF_ENABLE_BACKEND
#include <gtk/gtk.h>
#include <gthumb.h>
#include <extensions/importer/importer.h>
#include "dlg-import-from-flickr.h"
#include "flickr-authentication.h"
#include "flickr-photo.h"
#include "flickr-photoset.h"
#include "flickr-service.h"
#include "flickr-types.h"
#include "flickr-user.h"


#define GET_WIDGET(x) (_gtk_builder_get_widget (data->builder, (x)))
#define FAKE_SIZE 100000


enum {
	ACCOUNT_DATA_COLUMN,
	ACCOUNT_NAME_COLUMN
};


enum {
	PHOTOSET_DATA_COLUMN,
	PHOTOSET_ICON_COLUMN,
	PHOTOSET_TITLE_COLUMN,
	PHOTOSET_N_PHOTOS_COLUMN
};


typedef struct {
	FlickrServer         *server;
	GthBrowser           *browser;
	GthFileData          *location;
	GtkBuilder           *builder;
	GtkWidget            *dialog;
	GtkWidget            *preferences_dialog;
	GtkWidget            *progress_dialog;
	FlickrConnection     *conn;
	FlickrAuthentication *auth;
	FlickrService        *service;
	GtkWidget            *file_list;
	FlickrUser           *user;
	GList                *photosets;
	FlickrPhotoset       *photoset;
	GList                *photos;
	GCancellable         *cancellable;
} DialogData;


static void
import_dialog_destroy_cb (GtkWidget  *widget,
			  DialogData *data)
{
	if (data->conn != NULL)
		gth_task_completed (GTH_TASK (data->conn), NULL);
	_g_object_unref (data->cancellable);
	_g_object_unref (data->conn);
	_g_object_unref (data->service);
	_g_object_unref (data->auth);
	_g_object_unref (data->user);
	_g_object_list_unref (data->photosets);
	_g_object_unref (data->photoset);
	_g_object_list_unref (data->photos);
	_g_object_unref (data->builder);
	_g_object_unref (data->location);
	g_free (data);
}


static GList *
get_files_to_download (DialogData *data)
{
	GthFileView *file_view;
	GList       *selected;
	GList       *file_list;

	file_view = (GthFileView *) gth_file_list_get_view (GTH_FILE_LIST (data->file_list));
	selected = gth_file_selection_get_selected (GTH_FILE_SELECTION (file_view));
	if (selected != NULL)
		file_list = gth_file_list_get_files (GTH_FILE_LIST (data->file_list), selected);
	else
		file_list = gth_file_store_get_visibles (GTH_FILE_STORE (gth_file_view_get_model (file_view)));

	_gtk_tree_path_list_free (selected);

	return file_list;
}


static void
import_dialog_response_cb (GtkDialog *dialog,
			   int        response_id,
			   gpointer   user_data)
{
	DialogData *data = user_data;

	switch (response_id) {
	case GTK_RESPONSE_HELP:
		show_help_dialog (GTK_WINDOW (data->browser), "export-to-picasaweb");
		break;

	case GTK_RESPONSE_DELETE_EVENT:
	case GTK_RESPONSE_CANCEL:
		gth_file_list_cancel (GTH_FILE_LIST (data->file_list), (DataFunc) gtk_widget_destroy, data->dialog);
		break;

	case GTK_RESPONSE_OK:
		{
			GtkTreeIter     iter;
			FlickrPhotoset *photoset;
			GList          *file_list;

			if (! gtk_combo_box_get_active_iter (GTK_COMBO_BOX (GET_WIDGET ("photoset_combobox")), &iter)) {
				gtk_widget_set_sensitive (GET_WIDGET ("download_button"), FALSE);
				return;
			}

			gtk_tree_model_get (GTK_TREE_MODEL (GET_WIDGET ("photoset_liststore")), &iter,
					    PHOTOSET_DATA_COLUMN, &photoset,
					    -1);

			file_list = get_files_to_download (data);
			if (file_list != NULL) {
				GFile               *destination;
				gboolean             single_subfolder;
				GthSubfolderType     subfolder_type;
				GthSubfolderFormat   subfolder_format;
				char                *custom_format;
				GthTask             *task;

				destination = gth_import_preferences_get_destination ();
				subfolder_type = eel_gconf_get_enum (PREF_IMPORT_SUBFOLDER_TYPE, GTH_TYPE_SUBFOLDER_TYPE, GTH_SUBFOLDER_TYPE_FILE_DATE);
				subfolder_format = eel_gconf_get_enum (PREF_IMPORT_SUBFOLDER_FORMAT, GTH_TYPE_SUBFOLDER_FORMAT, GTH_SUBFOLDER_FORMAT_YYYYMMDD);
				single_subfolder = eel_gconf_get_boolean (PREF_IMPORT_SUBFOLDER_SINGLE, FALSE);
				custom_format = eel_gconf_get_string (PREF_IMPORT_SUBFOLDER_CUSTOM_FORMAT, "");

				task = gth_import_task_new (data->browser,
							    file_list,
							    destination,
							    subfolder_type,
							    subfolder_format,
							    single_subfolder,
							    custom_format,
							    (photoset->title != NULL ? photoset->title : ""),
							    NULL,
							    FALSE,
							    FALSE,
							    FALSE);
				gth_browser_exec_task (data->browser, task, FALSE);
				gtk_widget_destroy (data->dialog);

				g_object_unref (task);
				_g_object_unref (destination);
			}

			_g_object_list_unref (file_list);
			g_object_unref (photoset);
		}
		break;

	default:
		break;
	}
}


static void
update_account_list (DialogData *data)
{
	int            current_account_idx;
	FlickrAccount *current_account;
	int            idx;
	GList         *scan;
	GtkTreeIter    iter;

	gtk_list_store_clear (GTK_LIST_STORE (GET_WIDGET ("account_liststore")));

	current_account_idx = 0;
	current_account = flickr_authentication_get_account (data->auth);
	for (scan = flickr_authentication_get_accounts (data->auth), idx = 0; scan; scan = scan->next, idx++) {
		FlickrAccount *account = scan->data;

		if ((current_account != NULL) && (g_strcmp0 (current_account->username, account->username) == 0))
			current_account_idx = idx;

		gtk_list_store_append (GTK_LIST_STORE (GET_WIDGET ("account_liststore")), &iter);
		gtk_list_store_set (GTK_LIST_STORE (GET_WIDGET ("account_liststore")), &iter,
				    ACCOUNT_DATA_COLUMN, account,
				    ACCOUNT_NAME_COLUMN, account->username,
				    -1);
	}
	gtk_combo_box_set_active (GTK_COMBO_BOX (GET_WIDGET ("account_combobox")), current_account_idx);
}


static void
photoset_list_ready_cb (GObject      *source_object,
			GAsyncResult *res,
			gpointer      user_data)
{
	DialogData *data = user_data;
	GError     *error = NULL;
	GList      *scan;

	_g_object_list_unref (data->photosets);
	data->photosets = flickr_service_list_photosets_finish (FLICKR_SERVICE (source_object), res, &error);
	if (error != NULL) {
		if (data->conn != NULL)
			gth_task_dialog (GTH_TASK (data->conn), TRUE, NULL);
		_gtk_error_dialog_from_gerror_run (GTK_WINDOW (data->browser), _("Could not connect to the server"), &error);
		gtk_widget_destroy (data->dialog);
		return;
	}

	gtk_list_store_clear (GTK_LIST_STORE (GET_WIDGET ("photoset_liststore")));
	for (scan = data->photosets; scan; scan = scan->next) {
		FlickrPhotoset *photoset = scan->data;
		char           *n_photos;
		GtkTreeIter     iter;

		n_photos = g_strdup_printf ("(%d)", photoset->n_photos);

		gtk_list_store_append (GTK_LIST_STORE (GET_WIDGET ("photoset_liststore")), &iter);
		gtk_list_store_set (GTK_LIST_STORE (GET_WIDGET ("photoset_liststore")), &iter,
				    PHOTOSET_DATA_COLUMN, photoset,
				    PHOTOSET_ICON_COLUMN, "file-catalog",
				    PHOTOSET_TITLE_COLUMN, photoset->title,
				    PHOTOSET_N_PHOTOS_COLUMN, n_photos,
				    -1);

		g_free (n_photos);
	}

	gth_task_dialog (GTH_TASK (data->conn), TRUE, NULL);

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), GTK_WINDOW (data->browser));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), FALSE);
	gtk_window_present (GTK_WINDOW (data->dialog));
}


static void
authentication_ready_cb (FlickrAuthentication *auth,
			 FlickrUser           *user,
			 DialogData           *data)
{
	_g_object_unref (data->user);
	data->user = g_object_ref (user);
	update_account_list (data);

	flickr_service_list_photosets (data->service,
				       NULL,
				       data->cancellable,
				       photoset_list_ready_cb,
				       data);
}


static void
authentication_accounts_changed_cb (FlickrAuthentication *auth,
				    gpointer              user_data)
{
	update_account_list ((DialogData *) user_data);
}


static void
edit_accounts_button_clicked_cb (GtkButton  *button,
				 DialogData *data)
{
	flickr_authentication_edit_accounts (data->auth, GTK_WINDOW (data->dialog));
}


static void
account_combobox_changed_cb (GtkComboBox *widget,
			     gpointer     user_data)
{
	DialogData    *data = user_data;
	GtkTreeIter    iter;
	FlickrAccount *account;

	if (! gtk_combo_box_get_active_iter (widget, &iter))
		return;

	gtk_tree_model_get (gtk_combo_box_get_model (widget),
			    &iter,
			    ACCOUNT_DATA_COLUMN, &account,
			    -1);

	if (flickr_account_cmp (account, flickr_authentication_get_account (data->auth)) != 0)
		flickr_authentication_connect (data->auth, account);

	g_object_unref (account);
}


static void
update_selection_status (DialogData *data)
{
	GList    *file_list;
	int       n_selected;
	char     *text_selected;

	file_list = get_files_to_download (data);
	n_selected = g_list_length (file_list);
	text_selected = g_strdup_printf (g_dngettext (NULL, "%d file", "%d files", n_selected), n_selected);
	gtk_label_set_text (GTK_LABEL (GET_WIDGET ("images_info_label")), text_selected);

	g_free (text_selected);
	_g_object_list_unref (file_list);
}


static void
list_photos_ready_cb (GObject      *source_object,
		      GAsyncResult *result,
		      gpointer      user_data)
{
	DialogData *data = user_data;
	GError     *error = NULL;
	GList      *list;
	GList      *scan;

	gth_task_dialog (GTH_TASK (data->conn), TRUE, NULL);

	_g_object_list_unref (data->photos);
	data->photos = flickr_service_list_photos_finish (data->service, result, &error);
	if (error != NULL) {
		if (data->conn != NULL)
			gth_task_dialog (GTH_TASK (data->conn), TRUE, NULL);
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (data->browser), _("Could not get the photo list"), &error);
		gtk_widget_destroy (data->dialog);
		return;
	}

	list = NULL;
	for (scan = data->photos; scan; scan = scan->next) {
		FlickrPhoto *photo = scan->data;
		const char  *url;
		GthFileData *file_data;

		url = NULL;
		if (photo->url_s != NULL)
			url = photo->url_s;
		else if (photo->url_m != NULL)
			url = photo->url_m;
		else if (photo->url_o != NULL)
			url = photo->url_o;

		if (url == NULL)
			url = photo->url_sq;

		if (url == NULL)
			continue;

		file_data = gth_file_data_new_for_uri (url, (photo->mime_type != NULL) ? photo->mime_type : "image/jpeg");
		g_file_info_set_file_type (file_data->info, G_FILE_TYPE_REGULAR);
		g_file_info_set_size (file_data->info, FAKE_SIZE); /* set a fake size to make the progress dialog work correctly */
		g_file_info_set_attribute_object (file_data->info, "flickr::object", G_OBJECT (photo));

		list = g_list_prepend (list, file_data);
	}
	gth_file_list_set_files (GTH_FILE_LIST (data->file_list), list);
	update_selection_status (data);
	gtk_widget_set_sensitive (GET_WIDGET ("download_button"), list != NULL);

	_g_object_list_unref (list);
}


static void
photoset_combobox_changed_cb (GtkComboBox *widget,
			      gpointer     user_data)
{
	DialogData  *data = user_data;
	GtkTreeIter  iter;

	if (! gtk_combo_box_get_active_iter (widget, &iter)) {
		gth_file_list_clear (GTH_FILE_LIST (data->file_list), _("No album selected"));
		return;
	}

	_g_object_unref (data->photoset);
	gtk_tree_model_get (gtk_combo_box_get_model (widget),
			    &iter,
			    PHOTOSET_DATA_COLUMN, &data->photoset,
			    -1);

	gth_import_preferences_dialog_set_event (GTH_IMPORT_PREFERENCES_DIALOG (data->preferences_dialog), data->photoset->title);

	gth_task_dialog (GTH_TASK (data->conn), FALSE, NULL);
	flickr_service_list_photos (data->service,
				    data->photoset,
				    "original_format, url_sq, url_t, url_s, url_m, url_o",
				    0,
				    0,
				    data->cancellable,
				    list_photos_ready_cb,
				    data);
}


static GthImage *
flickr_thumbnail_loader (GthFileData   *file_data,
			 int            requested_size,
			 int           *original_width,
			 int           *original_height,
			 gpointer       user_data,
			 GCancellable  *cancellable,
		         GError       **error)
{
	GthImage       *image = NULL;
	GthThumbLoader *thumb_loader = user_data;
	FlickrPhoto    *photo;
	const char     *uri = NULL;

	photo = (FlickrPhoto *) g_file_info_get_attribute_object (file_data->info, "flickr::object");
	requested_size = gth_thumb_loader_get_requested_size (thumb_loader);
	if (requested_size == FLICKR_SIZE_SMALL_SQUARE)
		uri = photo->url_sq;
	else if (requested_size == FLICKR_SIZE_THUMBNAIL)
		uri = photo->url_t;
	else if (requested_size == FLICKR_SIZE_SMALL)
		uri = photo->url_s;
	else if (requested_size == FLICKR_SIZE_MEDIUM)
		uri = photo->url_m;

	if (uri == NULL)
		uri = photo->url_o;

	if (uri != NULL) {
		GFile *file;
		void  *buffer;
		gsize  size;

		file = g_file_new_for_uri (uri);
		if (g_load_file_in_buffer (file, &buffer, &size, error)) {
			GInputStream *stream;
			GdkPixbuf    *pixbuf;

			stream = g_memory_input_stream_new_from_data (buffer, size, g_free);
			pixbuf = gdk_pixbuf_new_from_stream (stream, cancellable, error);
			if (pixbuf != NULL) {
				GdkPixbuf *rotated;

				rotated = gdk_pixbuf_apply_embedded_orientation (pixbuf);
				g_object_unref (pixbuf);
				pixbuf = rotated;

				image = gth_image_new ();
				gth_image_set_pixbuf (image, pixbuf);
			}

			g_object_unref (pixbuf);
			g_object_unref (stream);
		}

		g_object_unref (file);
	}
	else
		*error = g_error_new_literal (GTH_ERROR, 0, "cannot generate the thumbnail");

	return image;
}


static int
flickr_photo_position_func (GthFileData *a,
		            GthFileData *b)
{
	FlickrPhoto *photo_a;
	FlickrPhoto *photo_b;

	photo_a = (FlickrPhoto *) g_file_info_get_attribute_object (a->info, "flickr::object");
	photo_b = (FlickrPhoto *) g_file_info_get_attribute_object (b->info, "flickr::object");

	if (photo_a->position == photo_b->position)
		return strcmp (photo_a->title, photo_b->title);
	else if (photo_a->position > photo_b->position)
		return 1;
	else
		return -1;
}


static void
file_list_selection_changed_cb (GthFileView *file_view,
				gpointer     user_data)
{
	update_selection_status ((DialogData *) user_data);
}


void
dlg_import_from_flickr (FlickrServer *server,
		        GthBrowser   *browser)
{
	DialogData     *data;
	GthThumbLoader *thumb_loader;
	char           *title;

	data = g_new0 (DialogData, 1);
	data->server = server;
	data->browser = browser;
	data->location = gth_file_data_dup (gth_browser_get_location_data (browser));
	data->builder = _gtk_builder_new_from_file ("import-from-flickr.ui", "flicker");
	data->dialog = _gtk_builder_get_widget (data->builder, "import_dialog");
	data->cancellable = g_cancellable_new ();

	{
		GtkCellLayout   *cell_layout;
		GtkCellRenderer *renderer;

		cell_layout = GTK_CELL_LAYOUT (GET_WIDGET ("photoset_combobox"));

		renderer = gtk_cell_renderer_pixbuf_new ();
		gtk_cell_layout_pack_start (cell_layout, renderer, FALSE);
		gtk_cell_layout_set_attributes (cell_layout, renderer,
						"icon-name", PHOTOSET_ICON_COLUMN,
						NULL);

		renderer = gtk_cell_renderer_text_new ();
		gtk_cell_layout_pack_start (cell_layout, renderer, TRUE);
		gtk_cell_layout_set_attributes (cell_layout, renderer,
						"text", PHOTOSET_TITLE_COLUMN,
						NULL);

		renderer = gtk_cell_renderer_text_new ();
		gtk_cell_layout_pack_start (cell_layout, renderer, FALSE);
		gtk_cell_layout_set_attributes (cell_layout, renderer,
						"text", PHOTOSET_N_PHOTOS_COLUMN,
						NULL);
	}

	/* Set the widget data */

	data->file_list = gth_file_list_new (gth_icon_view_new (), GTH_FILE_LIST_TYPE_NORMAL, FALSE);
	thumb_loader = gth_file_list_get_thumb_loader (GTH_FILE_LIST (data->file_list));
	gth_thumb_loader_set_use_cache (thumb_loader, FALSE);
	gth_thumb_loader_set_loader_func (thumb_loader, flickr_thumbnail_loader);
	gth_file_list_set_thumb_size (GTH_FILE_LIST (data->file_list), FLICKR_SIZE_THUMBNAIL);
	gth_file_view_set_spacing (GTH_FILE_VIEW (gth_file_list_get_view (GTH_FILE_LIST (data->file_list))), 0);
	gth_file_list_enable_thumbs (GTH_FILE_LIST (data->file_list), TRUE);
	gth_file_list_set_ignore_hidden (GTH_FILE_LIST (data->file_list), TRUE);
	gth_file_list_set_caption (GTH_FILE_LIST (data->file_list), "none");
	gth_file_list_set_sort_func (GTH_FILE_LIST (data->file_list), flickr_photo_position_func, FALSE);
	gth_file_list_clear (GTH_FILE_LIST (data->file_list), _("No album selected"));
	gtk_widget_show (data->file_list);
	gtk_box_pack_start (GTK_BOX (GET_WIDGET ("images_box")), data->file_list, TRUE, TRUE, 0);

	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (GET_WIDGET ("photoset_liststore")), PHOTOSET_TITLE_COLUMN, GTK_SORT_ASCENDING);

	gtk_widget_set_sensitive (GET_WIDGET ("download_button"), FALSE);

	data->preferences_dialog = gth_import_preferences_dialog_new ();
	gtk_window_set_transient_for (GTK_WINDOW (data->preferences_dialog), GTK_WINDOW (data->dialog));

	gtk_box_pack_start (GTK_BOX (GET_WIDGET ("destination_button_box")),
			    gth_import_destination_button_new (GTH_IMPORT_PREFERENCES_DIALOG (data->preferences_dialog)),
			    TRUE,
			    TRUE,
			    0);
	gtk_widget_show_all (GET_WIDGET ("destination_button_box"));

	title = g_strdup_printf (_("Import from %s"), data->server->name);
	gtk_window_set_title (GTK_WINDOW (data->dialog), title);
	g_free (title);

	_gtk_window_resize_to_fit_screen_height (data->dialog, 500);

	/* Set the signals handlers. */

	g_signal_connect (data->dialog,
			  "destroy",
			  G_CALLBACK (import_dialog_destroy_cb),
			  data);
	g_signal_connect (data->dialog,
			  "delete-event",
			  G_CALLBACK (gtk_true),
			  NULL);
	g_signal_connect (data->dialog,
			  "response",
			  G_CALLBACK (import_dialog_response_cb),
			  data);
	g_signal_connect (GET_WIDGET ("edit_accounts_button"),
			  "clicked",
			  G_CALLBACK (edit_accounts_button_clicked_cb),
			  data);
	g_signal_connect (GET_WIDGET ("account_combobox"),
			  "changed",
			  G_CALLBACK (account_combobox_changed_cb),
			  data);
	g_signal_connect (GET_WIDGET ("photoset_combobox"),
			  "changed",
			  G_CALLBACK (photoset_combobox_changed_cb),
			  data);
	g_signal_connect (gth_file_list_get_view (GTH_FILE_LIST (data->file_list)),
			  "file-selection-changed",
			  G_CALLBACK (file_list_selection_changed_cb),
			  data);

	update_selection_status (data);
	gth_import_preferences_dialog_set_event (GTH_IMPORT_PREFERENCES_DIALOG (data->preferences_dialog), "");

	data->conn = flickr_connection_new (data->server);
	data->service = flickr_service_new (data->conn);
	data->auth = flickr_authentication_new (data->conn,
						data->service,
						data->cancellable,
						GTK_WIDGET (data->browser),
						data->dialog);
	g_signal_connect (data->auth,
			  "ready",
			  G_CALLBACK (authentication_ready_cb),
			  data);
	g_signal_connect (data->auth,
			  "accounts_changed",
			  G_CALLBACK (authentication_accounts_changed_cb),
			  data);

	data->progress_dialog = gth_progress_dialog_new (GTK_WINDOW (data->browser));
	gth_progress_dialog_add_task (GTH_PROGRESS_DIALOG (data->progress_dialog), GTH_TASK (data->conn));

	flickr_authentication_auto_connect (data->auth);
}
