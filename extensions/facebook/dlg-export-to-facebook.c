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
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include <gtk/gtk.h>
#include <gthumb.h>
#include "dlg-export-to-facebook.h"
#include "facebook-authentication.h"
#include "facebook-album.h"
#include "facebook-album-properties-dialog.h"
#include "facebook-service.h"
#include "facebook-user.h"


#define GET_WIDGET(x) (_gtk_builder_get_widget (data->builder, (x)))
#define _OPEN_IN_BROWSER_RESPONSE 1


enum {
	ACCOUNT_DATA_COLUMN,
	ACCOUNT_NAME_COLUMN
};


enum {
	ALBUM_DATA_COLUMN,
	ALBUM_ICON_COLUMN,
	ALBUM_TITLE_COLUMN,
	ALBUM_SIZE_COLUMN
};


typedef struct {
	GthBrowser             *browser;
	GthFileData            *location;
	GList                  *file_list;
	GtkBuilder             *builder;
	GtkWidget              *dialog;
	GtkWidget              *list_view;
	GtkWidget              *progress_dialog;
	FacebookConnection     *conn;
	FacebookAuthentication *auth;
	FacebookService        *service;
	FacebookUser           *user;
	GList                  *albums;
	FacebookAlbum          *album;
	GList                  *photos_ids;
	GCancellable           *cancellable;
} DialogData;


static void
destroy_dialog (DialogData *data)
{
	if (data->dialog != NULL)
		gtk_widget_destroy (data->dialog);
	if (data->conn != NULL)
		gth_task_completed (GTH_TASK (data->conn), NULL);
	_g_object_unref (data->cancellable);
	_g_string_list_free (data->photos_ids);
	_g_object_unref (data->album);
	_g_object_list_unref (data->albums);
	_g_object_unref (data->user);
	_g_object_unref (data->service);
	_g_object_unref (data->auth);
	_g_object_unref (data->conn);
	_g_object_unref (data->builder);
	_g_object_list_unref (data->file_list);
	_g_object_unref (data->location);
	g_free (data);
}


static void
completed_messagedialog_response_cb (GtkDialog *dialog,
				     int        response_id,
				     gpointer   user_data)
{
	DialogData *data = user_data;

	switch (response_id) {
	case GTK_RESPONSE_DELETE_EVENT:
	case GTK_RESPONSE_CLOSE:
		gtk_widget_destroy (GTK_WIDGET (dialog));
		gtk_dialog_response (GTK_DIALOG (data->dialog), GTK_RESPONSE_DELETE_EVENT);
		break;

	case _OPEN_IN_BROWSER_RESPONSE:
		{
			char   *url = NULL;
			GError *error = NULL;

			gtk_widget_destroy (GTK_WIDGET (dialog));

			if ((data->album != NULL) && (data->album->link != NULL))
				url = g_strdup (data->album->link);

			if ((url != NULL) && ! gtk_show_uri (gtk_widget_get_screen (GTK_WIDGET (dialog)), url, 0, &error)) {
				if (data->conn != NULL)
					gth_task_dialog (GTH_TASK (data->conn), TRUE);
				_gtk_error_dialog_from_gerror_run (GTK_WINDOW (data->browser), _("Could not connect to the server"), &error);
			}

			gtk_dialog_response (GTK_DIALOG (data->dialog), GTK_RESPONSE_DELETE_EVENT);

			g_free (url);
		}
		break;

	default:
		break;
	}
}


static void
export_completed_with_success (DialogData *data)
{
	GtkBuilder *builder;
	GtkWidget  *dialog;

	gth_task_dialog (GTH_TASK (data->conn), TRUE);

	builder = _gtk_builder_new_from_file ("facebook-export-completed.ui", "facebook");
	dialog = _gtk_builder_get_widget (builder, "completed_messagedialog");
	g_object_set_data_full (G_OBJECT (dialog), "builder", builder, g_object_unref);
	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (completed_messagedialog_response_cb),
			  data);

	gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (data->browser));
	gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
	gtk_window_present (GTK_WINDOW (dialog));
}


static void
upload_photos_ready_cb (GObject      *source_object,
		        GAsyncResult *result,
		        gpointer      user_data)
{
	DialogData *data = user_data;
	GError     *error = NULL;

	data->photos_ids = facebook_service_upload_photos_finish (FACEBOOK_SERVICE (source_object), result, &error);
	if (error != NULL) {
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (data->browser), _("Could not upload the files"), &error);
		gtk_dialog_response (GTK_DIALOG (data->dialog), GTK_RESPONSE_DELETE_EVENT);
		return;
	}

	export_completed_with_success (data);
}


static void
export_dialog_response_cb (GtkDialog *dialog,
			   int        response_id,
			   gpointer   user_data)
{
	DialogData *data = user_data;

	switch (response_id) {
	case GTK_RESPONSE_HELP:
		show_help_dialog (GTK_WINDOW (data->browser), "export-to-facebook");
		break;

	case GTK_RESPONSE_DELETE_EVENT:
	case GTK_RESPONSE_CANCEL:
		gth_file_list_cancel (GTH_FILE_LIST (data->list_view), (DataFunc) destroy_dialog, data);
		break;

	case GTK_RESPONSE_OK:
		{
			GtkTreeIter  iter;
			GList       *file_list;

			gtk_widget_hide (data->dialog);
			gth_task_dialog (GTH_TASK (data->conn), FALSE);

			data->album = NULL;
			if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (GET_WIDGET ("album_combobox")), &iter)) {
				gtk_tree_model_get (gtk_combo_box_get_model (GTK_COMBO_BOX (GET_WIDGET ("album_combobox"))),
						    &iter,
						    ALBUM_DATA_COLUMN, &data->album,
						    -1);
			}

			file_list = gth_file_data_list_to_file_list (data->file_list);
			facebook_service_upload_photos (data->service,
						        data->album,
						        file_list,
						        data->cancellable,
						        upload_photos_ready_cb,
						        data);

			_g_object_list_unref (file_list);
		}
		break;

	default:
		break;
	}
}


static void
update_account_list (DialogData *data)
{
	int              current_account_idx;
	FacebookAccount *current_account;
	int              idx;
	GList           *scan;
	GtkTreeIter      iter;

	gtk_list_store_clear (GTK_LIST_STORE (GET_WIDGET ("account_liststore")));

	current_account_idx = 0;
	current_account = facebook_authentication_get_account (data->auth);
	for (scan = facebook_authentication_get_accounts (data->auth), idx = 0; scan; scan = scan->next, idx++) {
		FacebookAccount *account = scan->data;

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
authentication_accounts_changed_cb (FacebookAuthentication *auth,
				    gpointer              user_data)
{
	update_account_list ((DialogData *) user_data);
}


static void
update_album_list (DialogData *data)
{
	GList *scan;

	gtk_list_store_clear (GTK_LIST_STORE (GET_WIDGET ("album_liststore")));
	for (scan = data->albums; scan; scan = scan->next) {
		FacebookAlbum *album = scan->data;
		char          *size;
		GtkTreeIter    iter;

		size = g_strdup_printf ("(%d)", album->size);

		gtk_list_store_append (GTK_LIST_STORE (GET_WIDGET ("album_liststore")), &iter);
		gtk_list_store_set (GTK_LIST_STORE (GET_WIDGET ("album_liststore")), &iter,
				    ALBUM_DATA_COLUMN, album,
				    ALBUM_ICON_COLUMN, "file-catalog",
				    ALBUM_TITLE_COLUMN, album->name,
				    ALBUM_SIZE_COLUMN, size,
				    -1);

		g_free (size);
	}
}


static void
get_albums_ready_cb (GObject      *source_object,
		     GAsyncResult *res,
		     gpointer      user_data)
{
	DialogData *data = user_data;
	GError     *error = NULL;

	_g_object_list_unref (data->albums);
	data->albums = facebook_service_get_albums_finish (FACEBOOK_SERVICE (source_object), res, &error);
	if (error != NULL) {
		if (data->conn != NULL)
			gth_task_dialog (GTH_TASK (data->conn), TRUE);
		_gtk_error_dialog_from_gerror_run (GTK_WINDOW (data->browser), _("Could not connect to the server"), &error);
		gtk_dialog_response (GTK_DIALOG (data->dialog), GTK_RESPONSE_DELETE_EVENT);
		return;
	}

	gtk_widget_set_sensitive (GET_WIDGET ("upload_button"), TRUE);
	update_album_list (data);

	gth_task_dialog (GTH_TASK (data->conn), TRUE);
	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), GTK_WINDOW (data->browser));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), FALSE);
	gtk_window_present (GTK_WINDOW (data->dialog));
}


static void
authentication_ready_cb (FacebookAuthentication *auth,
			 FacebookUser           *user,
			 DialogData             *data)
{
	_g_object_unref (data->user);
	data->user = g_object_ref (user);
	update_account_list (data);

	facebook_service_get_albums (data->service,
				     data->user->id,
				     data->cancellable,
				     get_albums_ready_cb,
				     data);
}


static void
edit_accounts_button_clicked_cb (GtkButton  *button,
				 DialogData *data)
{
	facebook_authentication_edit_accounts (data->auth, GTK_WINDOW (data->dialog));
}


static void
account_combobox_changed_cb (GtkComboBox *widget,
			     gpointer     user_data)
{
	DialogData      *data = user_data;
	GtkTreeIter      iter;
	FacebookAccount *account;

	if (! gtk_combo_box_get_active_iter (widget, &iter))
		return;

	gtk_tree_model_get (gtk_combo_box_get_model (widget),
			    &iter,
			    ACCOUNT_DATA_COLUMN, &account,
			    -1);

	if (facebook_account_cmp (account, facebook_authentication_get_account (data->auth)) != 0)
		facebook_authentication_connect (data->auth, account);

	g_object_unref (account);
}


static void
create_album_ready_cb (GObject      *source_object,
		       GAsyncResult *result,
		       gpointer      user_data)
{
	DialogData    *data = user_data;
	FacebookAlbum *album;
	GError        *error = NULL;

	album = facebook_service_create_album_finish (data->service, result, &error);
	if (error != NULL) {
		if (data->conn != NULL)
			gth_task_dialog (GTH_TASK (data->conn), TRUE);
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (data->browser), _("Could not create the album"), &error);
		return;
	}

	data->albums = g_list_append (data->albums, album);
	update_album_list (data);
}


static void
new_album_dialog_response_cb (GtkDialog *dialog,
			      int        response_id,
			      gpointer   user_data)
{
	DialogData *data = user_data;

	switch (response_id) {
	case GTK_RESPONSE_DELETE_EVENT:
	case GTK_RESPONSE_CANCEL:
		gtk_widget_destroy (GTK_WIDGET (dialog));
		break;

	case GTK_RESPONSE_OK:
		{
			FacebookAlbum *album;

			album = facebook_album_new ();
			facebook_album_set_name (album, facebook_album_properties_dialog_get_name (FACEBOOK_ALBUM_PROPERTIES_DIALOG (dialog)));
			facebook_album_set_location (album, facebook_album_properties_dialog_get_location (FACEBOOK_ALBUM_PROPERTIES_DIALOG (dialog)));
			facebook_album_set_description (album, facebook_album_properties_dialog_get_description (FACEBOOK_ALBUM_PROPERTIES_DIALOG (dialog)));
			album->visibility = facebook_album_properties_dialog_get_visibility (FACEBOOK_ALBUM_PROPERTIES_DIALOG (dialog));
			facebook_service_create_album (data->service,
						       album,
						       data->cancellable,
						       create_album_ready_cb,
						       data);

			g_object_unref (album);
		}
		gtk_widget_destroy (GTK_WIDGET (dialog));
		break;

	default:
		break;
	}
}


static void
add_album_button_clicked_cb (GtkButton *button,
			     gpointer   user_data)
{
	DialogData *data = user_data;
	GtkWidget  *dialog;

	dialog = facebook_album_properties_dialog_new (g_file_info_get_edit_name (data->location->info),
						       NULL,
						       NULL,
						       FACEBOOK_VISIBILITY_SELF);
	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (new_album_dialog_response_cb),
			  data);

	gtk_window_set_title (GTK_WINDOW (dialog), _("New Album"));
	gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (data->dialog));
	gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
	gtk_window_present (GTK_WINDOW (dialog));
}


void
dlg_export_to_facebook (GthBrowser *browser,
		        GList      *file_list)
{
	DialogData *data;
	GList      *scan;
	int         n_total;
	goffset     total_size;
	char       *total_size_formatted;
	char       *text;
	char       *title;

	data = g_new0 (DialogData, 1);
	data->browser = browser;
	data->location = gth_file_data_dup (gth_browser_get_location_data (browser));
	data->builder = _gtk_builder_new_from_file ("export-to-facebook.ui", "facebook");
	data->dialog = _gtk_builder_get_widget (data->builder, "export_dialog");
	data->cancellable = g_cancellable_new ();

	{
		GtkCellLayout   *cell_layout;
		GtkCellRenderer *renderer;

		cell_layout = GTK_CELL_LAYOUT (GET_WIDGET ("album_combobox"));

		renderer = gtk_cell_renderer_pixbuf_new ();
		gtk_cell_layout_pack_start (cell_layout, renderer, FALSE);
		gtk_cell_layout_set_attributes (cell_layout, renderer,
						"icon-name", ALBUM_ICON_COLUMN,
						NULL);

		renderer = gtk_cell_renderer_text_new ();
		gtk_cell_layout_pack_start (cell_layout, renderer, TRUE);
		gtk_cell_layout_set_attributes (cell_layout, renderer,
						"text", ALBUM_TITLE_COLUMN,
						NULL);

		renderer = gtk_cell_renderer_text_new ();
		gtk_cell_layout_pack_end (cell_layout, renderer, FALSE);
		gtk_cell_layout_set_attributes (cell_layout, renderer,
						"text", ALBUM_SIZE_COLUMN,
						NULL);
	}

	data->file_list = NULL;
	n_total = 0;
	total_size = 0;
	for (scan = file_list; scan; scan = scan->next) {
		GthFileData *file_data = scan->data;
		const char  *mime_type;

		mime_type = gth_file_data_get_mime_type (file_data);
		if (g_content_type_equals (mime_type, "image/gif") /* FIXME: check if the mime types are correct */
		    || g_content_type_equals (mime_type, "image/jpeg")
		    || g_content_type_equals (mime_type, "image/png")
		    || g_content_type_equals (mime_type, "image/psd")
		    || g_content_type_equals (mime_type, "image/tiff")
		    || g_content_type_equals (mime_type, "image/jp2")
		    || g_content_type_equals (mime_type, "image/iff")
		    || g_content_type_equals (mime_type, "image/bmp")
		    || g_content_type_equals (mime_type, "image/xbm"))
		{
			total_size += g_file_info_get_size (file_data->info);
			n_total++;
			data->file_list = g_list_prepend (data->file_list, g_object_ref (file_data));
		}
	}
	data->file_list = g_list_reverse (data->file_list);

	if (data->file_list == NULL) {
		GError *error;

		error = g_error_new_literal (GTH_ERROR, GTH_ERROR_GENERIC, _("No valid file selected."));
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (browser), _("Could not export the files"), &error);
		gtk_dialog_response (GTK_DIALOG (data->dialog), GTK_RESPONSE_DELETE_EVENT);

		return;
	}

	total_size_formatted = g_format_size_for_display (total_size);
	text = g_strdup_printf (g_dngettext (NULL, "%d file (%s)", "%d files (%s)", n_total), n_total, total_size_formatted);
	gtk_label_set_text (GTK_LABEL (GET_WIDGET ("images_info_label")), text);
	g_free (text);
	g_free (total_size_formatted);

	_gtk_window_resize_to_fit_screen_height (data->dialog, 500);

	/* Set the widget data */

	data->list_view = gth_file_list_new (GTH_FILE_LIST_TYPE_NO_SELECTION, FALSE);
	gth_file_list_set_thumb_size (GTH_FILE_LIST (data->list_view), 112);
	gth_file_view_set_spacing (GTH_FILE_VIEW (gth_file_list_get_view (GTH_FILE_LIST (data->list_view))), 0);
	gth_file_list_enable_thumbs (GTH_FILE_LIST (data->list_view), TRUE);
	gth_file_list_set_caption (GTH_FILE_LIST (data->list_view), "none");
	gth_file_list_set_sort_func (GTH_FILE_LIST (data->list_view), gth_main_get_sort_type ("file::name")->cmp_func, FALSE);
	gtk_widget_show (data->list_view);
	gtk_box_pack_start (GTK_BOX (GET_WIDGET ("images_box")), data->list_view, TRUE, TRUE, 0);
	gth_file_list_set_files (GTH_FILE_LIST (data->list_view), data->file_list);

	gtk_widget_set_sensitive (GET_WIDGET ("upload_button"), FALSE);

	title = g_strdup_printf (_("Export to %s"), "Facebook");
	gtk_window_set_title (GTK_WINDOW (data->dialog), title);
	g_free (title);

	/* Set the signals handlers. */

	g_signal_connect (data->dialog,
			  "delete-event",
			  G_CALLBACK (gtk_widget_hide_on_delete),
			  NULL);
	g_signal_connect (data->dialog,
			  "response",
			  G_CALLBACK (export_dialog_response_cb),
			  data);
	g_signal_connect (GET_WIDGET ("edit_accounts_button"),
			  "clicked",
			  G_CALLBACK (edit_accounts_button_clicked_cb),
			  data);
	g_signal_connect (GET_WIDGET ("account_combobox"),
			  "changed",
			  G_CALLBACK (account_combobox_changed_cb),
			  data);
	g_signal_connect (GET_WIDGET ("add_album_button"),
			  "clicked",
			  G_CALLBACK (add_album_button_clicked_cb),
			  data);

	data->conn = facebook_connection_new ();
	data->service = facebook_service_new (data->conn);
	data->auth = facebook_authentication_new (data->conn,
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

	facebook_authentication_auto_connect (data->auth);
}
