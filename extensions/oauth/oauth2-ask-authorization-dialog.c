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
#include "oauth2-ask-authorization-dialog.h"


#define GET_WIDGET(x) (_gtk_builder_get_widget (self->priv->builder, (x)))


G_DEFINE_TYPE (OAuth2AskAuthorizationDialog, oauth2_ask_authorization_dialog, GTK_TYPE_DIALOG)


/* Signals */
enum {
	REDIRECTED,
	LAST_SIGNAL
};


static guint oauth2_ask_authorization_dialog_signals[LAST_SIGNAL] = { 0 };


struct _OAuth2AskAuthorizationDialogPrivate {
	GtkWidget *view;
};


static void
oauth2_ask_authorization_dialog_class_init (OAuth2AskAuthorizationDialogClass *klass)
{
	g_type_class_add_private (klass, sizeof (OAuth2AskAuthorizationDialogPrivate));

	oauth2_ask_authorization_dialog_signals[REDIRECTED] =
		g_signal_new ("redirected",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (OAuth2AskAuthorizationDialogClass, redirected),
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
	OAuth2AskAuthorizationDialog *self = user_data;

	if (load_event == WEBKIT_LOAD_REDIRECTED)
		g_signal_emit (self, oauth2_ask_authorization_dialog_signals[REDIRECTED], 0);
}


static void
oauth2_ask_authorization_dialog_init (OAuth2AskAuthorizationDialog *self)
{
	GtkWidget *box;

	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, OAUTH2_TYPE_ASK_AUTHORIZATION_DIALOG, OAuth2AskAuthorizationDialogPrivate);

	gtk_window_set_default_size (GTK_WINDOW (self), 500, 500);
	gtk_window_set_resizable (GTK_WINDOW (self), TRUE);
	gtk_box_set_spacing (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (self))), 5);
	gtk_container_set_border_width (GTK_CONTAINER (self), 5);

	box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_show (box);
	gtk_container_set_border_width (GTK_CONTAINER (box), 5);
	gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (self))), box, TRUE, TRUE, 0);

	self->priv->view = webkit_web_view_new ();
	gtk_widget_show (self->priv->view);
	gtk_box_pack_start (GTK_BOX (box), self->priv->view, TRUE, TRUE, 0);

	g_signal_connect (self->priv->view,
			  "load-changed",
			  G_CALLBACK (webkit_view_load_changed_cb),
			  self);

	gtk_dialog_add_button (GTK_DIALOG (self),
			       GTK_STOCK_CANCEL,
			       GTK_RESPONSE_CANCEL);
}


GtkWidget *
oauth2_ask_authorization_dialog_new (const char *title,
				     const char *uri)
{
	OAuth2AskAuthorizationDialog *self;

	self = g_object_new (OAUTH2_TYPE_ASK_AUTHORIZATION_DIALOG, "title", title, NULL);
	webkit_web_view_load_uri (WEBKIT_WEB_VIEW (self->priv->view), uri);

	return (GtkWidget *) self;
}


GtkWidget *
oauth2_ask_authorization_dialog_get_view (OAuth2AskAuthorizationDialog *self)
{
	return self->priv->view;
}


const char *
oauth2_ask_authorization_dialog_get_uri (OAuth2AskAuthorizationDialog *self)
{
	return webkit_web_view_get_uri (WEBKIT_WEB_VIEW (self->priv->view));
}
