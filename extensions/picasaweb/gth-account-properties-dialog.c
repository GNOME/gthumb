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
#include <glib/gi18n.h>
#include "gth-account-properties-dialog.h"


#define GET_WIDGET(x) (_gtk_builder_get_widget (self->priv->builder, (x)))


static gpointer parent_class = NULL;


struct _GthAccountPropertiesDialogPrivate {
	GtkBuilder   *builder;
	char         *email;
	char         *password;
	char         *challange_url;
	GCancellable *cancellable;
};


static void
gth_account_properties_dialog_finalize (GObject *object)
{
	GthAccountPropertiesDialog *self;

	self = GTH_ACCOUNT_PROPERTIES_DIALOG (object);
	_g_object_unref (self->priv->builder);
	g_free (self->priv->email);
	g_free (self->priv->password);
	g_free (self->priv->challange_url);
	g_object_unref (self->priv->cancellable);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gth_account_properties_dialog_class_init (GthAccountPropertiesDialogClass *klass)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GthAccountPropertiesDialogPrivate));

	object_class = (GObjectClass*) klass;
	object_class->finalize = gth_account_properties_dialog_finalize;
}


static void
gth_account_properties_dialog_init (GthAccountPropertiesDialog *self)
{
	GtkWidget *content;

	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_ACCOUNT_PROPERTIES_DIALOG, GthAccountPropertiesDialogPrivate);
	self->priv->email = NULL;
	self->priv->password = NULL;
	self->priv->challange_url = NULL;
	self->priv->cancellable = g_cancellable_new ();
	self->priv->builder = _gtk_builder_new_from_file ("picasa-web-account-properties.ui", "picasaweb");

	gtk_window_set_resizable (GTK_WINDOW (self), FALSE);
	gtk_dialog_set_has_separator (GTK_DIALOG (self), FALSE);
	gtk_box_set_spacing (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (self))), 5);
	gtk_container_set_border_width (GTK_CONTAINER (self), 5);

	content = _gtk_builder_get_widget (self->priv->builder, "account_properties");
	gtk_container_set_border_width (GTK_CONTAINER (content), 0);
  	gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (self))), content, TRUE, TRUE, 0);

  	gtk_entry_set_visibility (GTK_ENTRY (GET_WIDGET ("password_entry")), FALSE);

	gtk_dialog_add_buttons (GTK_DIALOG (self),
				GTK_STOCK_HELP, GTK_RESPONSE_HELP,
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				GTK_STOCK_OK, GTK_RESPONSE_OK,
				NULL);
	gtk_dialog_set_default_response (GTK_DIALOG (self), GTK_RESPONSE_OK);
}


GType
gth_account_properties_dialog_get_type (void)
{
	static GType type = 0;

	if (type == 0) {
		static const GTypeInfo g_define_type_info = {
			sizeof (GthAccountPropertiesDialogClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) gth_account_properties_dialog_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (GthAccountPropertiesDialog),
			0,
			(GInstanceInitFunc) gth_account_properties_dialog_init,
			NULL
		};
		type = g_type_register_static (GTK_TYPE_DIALOG,
					       "GthAccountPropertiesDialog",
					       &g_define_type_info,
					       0);
	}

	return type;
}


static void
image_buffer_ready_cb (void     *buffer,
		       gsize     count,
		       GError   *error,
		       gpointer  user_data)
{
	GthAccountPropertiesDialog *self = user_data;
	GInputStream               *stream;
	GdkPixbuf                  *pixbuf;

	if (error != NULL) {
		/* FIXME: show the error dialog */
		return;
	}

	stream = g_memory_input_stream_new_from_data (buffer, count, NULL);
	pixbuf = gdk_pixbuf_new_from_stream (stream, NULL, NULL);
	if (pixbuf != NULL) {
		gtk_widget_show (GET_WIDGET ("challange_box"));
		gtk_image_set_from_pixbuf (GTK_IMAGE (GET_WIDGET ("challenge_image")), pixbuf);
		g_object_unref (pixbuf);
	}

	g_object_unref (stream);
}


static void
gth_account_properties_dialog_construct (GthAccountPropertiesDialog *self,
					 const char                 *email,
					 const char                 *password,
					 const char                 *challange_url)
{
	if (email != NULL) {
		self->priv->email = g_strdup (email);
		gtk_entry_set_text (GTK_ENTRY (GET_WIDGET ("email_entry")), self->priv->email);
	}

	if (password != NULL) {
		self->priv->password = g_strdup (password);
		gtk_entry_set_text (GTK_ENTRY (GET_WIDGET ("password_entry")), self->priv->password);
	}

	if (challange_url != NULL) {
		char  *url;
		GFile *file;

		self->priv->challange_url = g_strdup (challange_url);

		url = g_strconcat ("http://www.google.com/accounts/", challange_url, NULL);
		file = g_file_new_for_uri (url);
		g_load_file_async (file,
				   G_PRIORITY_DEFAULT,
				   self->priv->cancellable,
				   image_buffer_ready_cb,
				   self);

		g_object_unref (file);
		g_free (url);
	}
}


GtkWidget *
gth_account_properties_dialog_new (const char *email,
			           const char *password,
				   const char *challange_url)
{
	GthAccountPropertiesDialog *self;

	self = g_object_new (GTH_TYPE_ACCOUNT_PROPERTIES_DIALOG, NULL);
	gth_account_properties_dialog_construct (self, email, password, challange_url);

	return (GtkWidget *) self;
}


const char *
gth_account_properties_dialog_get_email (GthAccountPropertiesDialog *self)
{
	return gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("email_entry")));
}


const char *
gth_account_properties_dialog_get_password (GthAccountPropertiesDialog *self)
{
	return gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("password_entry")));
}


const char *
gth_account_properties_dialog_get_challange (GthAccountPropertiesDialog *self)
{
	return gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("challenge_entry")));
}
