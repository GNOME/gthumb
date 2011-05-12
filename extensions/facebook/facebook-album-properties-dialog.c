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
#include "facebook-album-properties-dialog.h"


#define GET_WIDGET(x) (_gtk_builder_get_widget (self->priv->builder, (x)))


static gpointer parent_class = NULL;


struct _FacebookAlbumPropertiesDialogPrivate {
	GtkBuilder *builder;
};


static void
facebook_album_properties_dialog_finalize (GObject *object)
{
	FacebookAlbumPropertiesDialog *self;

	self = FACEBOOK_ALBUM_PROPERTIES_DIALOG (object);
	_g_object_unref (self->priv->builder);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
facebook_album_properties_dialog_class_init (FacebookAlbumPropertiesDialogClass *klass)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (FacebookAlbumPropertiesDialogPrivate));

	object_class = (GObjectClass*) klass;
	object_class->finalize = facebook_album_properties_dialog_finalize;
}


static void
facebook_album_properties_dialog_init (FacebookAlbumPropertiesDialog *self)
{
	GtkWidget *content;

	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, FACEBOOK_TYPE_ALBUM_PROPERTIES_DIALOG, FacebookAlbumPropertiesDialogPrivate);
	self->priv->builder = _gtk_builder_new_from_file ("facebook-album-properties.ui", "facebook");

	gtk_window_set_resizable (GTK_WINDOW (self), FALSE);
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
facebook_album_properties_dialog_get_type (void)
{
	static GType type = 0;

	if (type == 0) {
		static const GTypeInfo g_define_type_info = {
			sizeof (FacebookAlbumPropertiesDialogClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) facebook_album_properties_dialog_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (FacebookAlbumPropertiesDialog),
			0,
			(GInstanceInitFunc) facebook_album_properties_dialog_init,
			NULL
		};
		type = g_type_register_static (GTK_TYPE_DIALOG,
					       "FacebookAlbumPropertiesDialog",
					       &g_define_type_info,
					       0);
	}

	return type;
}


static void
facebook_album_properties_dialog_construct (FacebookAlbumPropertiesDialog *self,
				            const char                    *name,
				            const char                    *location,
				            const char                    *description,
				            FacebookVisibility             visibility)
{
	int idx;

	if (name != NULL)
		gtk_entry_set_text (GTK_ENTRY (GET_WIDGET ("name_entry")), name);
	if (location != NULL)
		gtk_entry_set_text (GTK_ENTRY (GET_WIDGET ("location_entry")), location);
	if (description != NULL)
		gtk_entry_set_text (GTK_ENTRY (GET_WIDGET ("description_entry")), description);

	switch (visibility) {
	case FACEBOOK_VISIBILITY_EVERYONE:
		idx = 0;
		break;
	case FACEBOOK_VISIBILITY_ALL_FRIENDS:
		idx = 1;
		break;
	case FACEBOOK_VISIBILITY_SELF:
		idx = 2;
		break;
	default:
		idx = 0;
		break;
	}
	gtk_combo_box_set_active (GTK_COMBO_BOX (GET_WIDGET ("visibility_combobox")), idx);
}


GtkWidget *
facebook_album_properties_dialog_new (const char         *name,
				      const char         *location,
				      const char         *description,
				      FacebookVisibility  visibility)
{
	FacebookAlbumPropertiesDialog *self;

	self = g_object_new (FACEBOOK_TYPE_ALBUM_PROPERTIES_DIALOG, NULL);
	facebook_album_properties_dialog_construct (self, name, location, description, visibility);

	return (GtkWidget *) self;
}


const char *
facebook_album_properties_dialog_get_name (FacebookAlbumPropertiesDialog *self)
{
	return gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("name_entry")));
}


const char *
facebook_album_properties_dialog_get_location (FacebookAlbumPropertiesDialog *self)
{
	return gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("location_entry")));
}


const char *
facebook_album_properties_dialog_get_description (FacebookAlbumPropertiesDialog *self)
{
	return gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("description_entry")));
}


FacebookVisibility
facebook_album_properties_dialog_get_visibility (FacebookAlbumPropertiesDialog *self)
{
	GtkTreeIter        iter;
	FacebookVisibility value;

	if (! gtk_combo_box_get_active_iter (GTK_COMBO_BOX (GET_WIDGET ("visibility_combobox")), &iter))
		return FACEBOOK_VISIBILITY_SELF;

	gtk_tree_model_get (gtk_combo_box_get_model (GTK_COMBO_BOX (GET_WIDGET ("visibility_combobox"))),
			    &iter,
			    1, &value,
			    -1);

	return value;
}
