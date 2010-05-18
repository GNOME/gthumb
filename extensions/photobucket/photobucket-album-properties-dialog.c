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
#include "photobucket-album-properties-dialog.h"


#define GET_WIDGET(x) (_gtk_builder_get_widget (self->priv->builder, (x)))


static gpointer parent_class = NULL;


struct _PhotobucketAlbumPropertiesDialogPrivate {
	GtkBuilder *builder;
};


static void
photobucket_album_properties_dialog_finalize (GObject *object)
{
	PhotobucketAlbumPropertiesDialog *self;

	self = PHOTOBUCKET_ALBUM_PROPERTIES_DIALOG (object);
	_g_object_unref (self->priv->builder);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
photobucket_album_properties_dialog_class_init (PhotobucketAlbumPropertiesDialogClass *klass)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (PhotobucketAlbumPropertiesDialogPrivate));

	object_class = (GObjectClass*) klass;
	object_class->finalize = photobucket_album_properties_dialog_finalize;
}


static void
photobucket_album_properties_dialog_init (PhotobucketAlbumPropertiesDialog *self)
{
	GtkWidget *content;

	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, PHOTOBUCKET_TYPE_ALBUM_PROPERTIES_DIALOG, PhotobucketAlbumPropertiesDialogPrivate);
	self->priv->builder = _gtk_builder_new_from_file ("photobucket-album-properties.ui", "photobucket");

	gtk_window_set_resizable (GTK_WINDOW (self), FALSE);
	gtk_dialog_set_has_separator (GTK_DIALOG (self), FALSE);
	gtk_box_set_spacing (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (self))), 5);
	gtk_container_set_border_width (GTK_CONTAINER (self), 5);

	content = _gtk_builder_get_widget (self->priv->builder, "album_properties");
	gtk_container_set_border_width (GTK_CONTAINER (content), 5);
  	gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (self))), content, TRUE, TRUE, 0);

	gtk_dialog_add_buttons (GTK_DIALOG (self),
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				GTK_STOCK_OK, GTK_RESPONSE_OK,
				NULL);
	gtk_dialog_set_default_response (GTK_DIALOG (self), GTK_RESPONSE_OK);
}


GType
photobucket_album_properties_dialog_get_type (void)
{
	static GType type = 0;

	if (type == 0) {
		static const GTypeInfo g_define_type_info = {
			sizeof (PhotobucketAlbumPropertiesDialogClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) photobucket_album_properties_dialog_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (PhotobucketAlbumPropertiesDialog),
			0,
			(GInstanceInitFunc) photobucket_album_properties_dialog_init,
			NULL
		};
		type = g_type_register_static (GTK_TYPE_DIALOG,
					       "PhotobucketAlbumPropertiesDialog",
					       &g_define_type_info,
					       0);
	}

	return type;
}


static void
photobucket_album_properties_dialog_construct (PhotobucketAlbumPropertiesDialog *self,
				               const char                       *name)
{
	if (name != NULL)
		gtk_entry_set_text (GTK_ENTRY (GET_WIDGET ("name_entry")), name);
}


GtkWidget *
photobucket_album_properties_dialog_new (const char *name)
{
	PhotobucketAlbumPropertiesDialog *self;

	self = g_object_new (PHOTOBUCKET_TYPE_ALBUM_PROPERTIES_DIALOG, NULL);
	photobucket_album_properties_dialog_construct (self, name);

	return (GtkWidget *) self;
}


const char *
photobucket_album_properties_dialog_get_name (PhotobucketAlbumPropertiesDialog *self)
{
	return gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("name_entry")));
}
