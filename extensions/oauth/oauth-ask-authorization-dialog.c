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
#include <glib/gi18n.h>
#include <webkit2/webkit2.h>
#include "oauth-ask-authorization-dialog.h"


#define GET_WIDGET(x) (_gtk_builder_get_widget (self->priv->builder, (x)))
#define _LOADING_PAGE 1
#define _WEB_VIEW_PAGE 0


G_DEFINE_TYPE (OAuthAskAuthorizationDialog, oauth_ask_authorization_dialog, GTK_TYPE_DIALOG)


/* Signals */
enum {
	LOAD_REQUEST,
	LOADED,
	REDIRECTED,
	LAST_SIGNAL
};


static guint oauth_ask_authorization_dialog_signals[LAST_SIGNAL] = { 0 };


struct _OAuthAskAuthorizationDialogPrivate {
	GtkWidget  *view;
	GtkBuilder *builder;
	gulong      load_changed_id;
};


static void
oauth_ask_authorization_dialog_finalize (GObject *obj)
{
	OAuthAskAuthorizationDialog *self;

	self = OAUTH_ASK_AUTHORIZATION_DIALOG (obj);
	if (g_signal_handler_is_connected (self->priv->view, self->priv->load_changed_id))
		g_signal_handler_disconnect (self->priv->view, self->priv->load_changed_id);
	webkit_web_view_stop_loading (WEBKIT_WEB_VIEW (self->priv->view));
	_g_object_unref (self->priv->builder);

	G_OBJECT_CLASS (oauth_ask_authorization_dialog_parent_class)->finalize (obj);
}


static void
oauth_ask_authorization_dialog_class_init (OAuthAskAuthorizationDialogClass *klass)
{
	GObjectClass *object_class;

	g_type_class_add_private (klass, sizeof (OAuthAskAuthorizationDialogPrivate));

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = oauth_ask_authorization_dialog_finalize;

	oauth_ask_authorization_dialog_signals[LOAD_REQUEST] =
		g_signal_new ("load-request",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (OAuthAskAuthorizationDialogClass, load_request),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
	oauth_ask_authorization_dialog_signals[LOADED] =
		g_signal_new ("loaded",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (OAuthAskAuthorizationDialogClass, loaded),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
	oauth_ask_authorization_dialog_signals[REDIRECTED] =
		g_signal_new ("redirected",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (OAuthAskAuthorizationDialogClass, redirected),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
}


static void
webkit_view_load_changed_cb (WebKitWebView   *web_view,
                	     WebKitLoadEvent  load_event,
                	     gpointer         user_data)
{
	OAuthAskAuthorizationDialog *self = user_data;

	switch (load_event) {
	case WEBKIT_LOAD_STARTED:
	case WEBKIT_LOAD_COMMITTED:
		gtk_notebook_set_current_page (GTK_NOTEBOOK(GET_WIDGET ("dialog_content")), _LOADING_PAGE);
		g_signal_emit (self, oauth_ask_authorization_dialog_signals[LOAD_REQUEST], 0);
		break;
	case WEBKIT_LOAD_REDIRECTED:
		g_signal_emit (self, oauth_ask_authorization_dialog_signals[REDIRECTED], 0);
		break;
	case WEBKIT_LOAD_FINISHED:
		gtk_notebook_set_current_page (GTK_NOTEBOOK(GET_WIDGET ("dialog_content")), _WEB_VIEW_PAGE);
		g_signal_emit (self, oauth_ask_authorization_dialog_signals[LOADED], 0);
		break;
	default:
		break;
	}
}


static void
oauth_ask_authorization_dialog_init (OAuthAskAuthorizationDialog *self)
{
	GtkWidget        *dialog_content;
	WebKitWebContext *context;

	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, OAUTH_TYPE_ASK_AUTHORIZATION_DIALOG, OAuthAskAuthorizationDialogPrivate);
	self->priv->builder = _gtk_builder_new_from_file ("oauth-ask-authorization.ui", "oauth");

	gtk_window_set_default_size (GTK_WINDOW (self), 500, 500);
	gtk_window_set_resizable (GTK_WINDOW (self), TRUE);
	gtk_box_set_spacing (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (self))), 5);
	gtk_container_set_border_width (GTK_CONTAINER (self), 5);

	dialog_content = GET_WIDGET ("dialog_content");
	gtk_widget_show (dialog_content);
	gtk_container_set_border_width (GTK_CONTAINER (dialog_content), 5);
	gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (self))), dialog_content, TRUE, TRUE, 0);

	self->priv->view = webkit_web_view_new ();
	context = webkit_web_view_get_context (WEBKIT_WEB_VIEW (self->priv->view));
	if (context != NULL) {
		WebKitCookieManager *cookie_manager;
		GFile               *file;
		char                *cookie_filename;

		file = gth_user_dir_get_file_for_write (GTH_DIR_CACHE, GTHUMB_DIR, "cookies", NULL);
		cookie_filename = g_file_get_path (file);

		cookie_manager = webkit_web_context_get_cookie_manager (context);
		webkit_cookie_manager_set_accept_policy (cookie_manager, WEBKIT_COOKIE_POLICY_ACCEPT_ALWAYS);
		webkit_cookie_manager_set_persistent_storage (cookie_manager, cookie_filename, WEBKIT_COOKIE_PERSISTENT_STORAGE_TEXT);
		webkit_web_context_set_cache_model (context, WEBKIT_CACHE_MODEL_DOCUMENT_BROWSER);

		g_free (cookie_filename);
		g_object_unref (file);
	}
	gtk_widget_show (self->priv->view);
	gtk_box_pack_start (GTK_BOX (GET_WIDGET ("webkit_view_container")), self->priv->view, TRUE, TRUE, 0);

	self->priv->load_changed_id = g_signal_connect (self->priv->view,
			  	  	  	  	"load-changed",
			  	  	  	  	G_CALLBACK (webkit_view_load_changed_cb),
			  	  	  	  	self);

	gtk_dialog_add_button (GTK_DIALOG (self),
			       GTK_STOCK_CANCEL,
			       GTK_RESPONSE_CANCEL);
}


GtkWidget *
oauth_ask_authorization_dialog_new (const char *uri)
{
	OAuthAskAuthorizationDialog *self;

	self = g_object_new (OAUTH_TYPE_ASK_AUTHORIZATION_DIALOG,
			     "title", _("Authorization Required"),
			     NULL);
	webkit_web_view_load_uri (WEBKIT_WEB_VIEW (self->priv->view), uri);

	return (GtkWidget *) self;
}


GtkWidget *
oauth_ask_authorization_dialog_get_view (OAuthAskAuthorizationDialog *self)
{
	return self->priv->view;
}


const char *
oauth_ask_authorization_dialog_get_uri (OAuthAskAuthorizationDialog *self)
{
	return webkit_web_view_get_uri (WEBKIT_WEB_VIEW (self->priv->view));
}


const char *
oauth_ask_authorization_dialog_get_title (OAuthAskAuthorizationDialog *self)
{
	return webkit_web_view_get_title (WEBKIT_WEB_VIEW (self->priv->view));
}
