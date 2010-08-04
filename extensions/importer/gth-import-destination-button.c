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
#include "gth-import-destination-button.h"


static gpointer parent_class = NULL;


struct _GthImportDestinationButtonPrivate {
	GtkWidget *destination_icon;
	GtkWidget *destination_label;
	GtkWidget *subfolder_label;
};


static void
gth_import_destination_button_finalize (GObject *object)
{
	GthImportDestinationButton *self;

	self = GTH_IMPORT_DESTINATION_BUTTON (object);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gth_import_destination_button_class_init (GthImportDestinationButtonClass *klass)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GthImportDestinationButtonPrivate));

	object_class = (GObjectClass*) klass;
	object_class->finalize = gth_import_destination_button_finalize;
}


static void
gth_import_destination_button_init (GthImportDestinationButton *self)
{
	GtkWidget      *box;
	GtkWidget      *label_box;
	PangoAttrList  *attr_list;
	PangoAttribute *attr;

	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_IMPORT_DESTINATION_BUTTON, GthImportDestinationButtonPrivate);

	box = gtk_hbox_new (FALSE, 6);
	gtk_widget_show (box);
	gtk_container_add (GTK_CONTAINER (self), box);

	self->priv->destination_icon = gtk_image_new ();
	gtk_widget_show (self->priv->destination_icon);
	gtk_box_pack_start (GTK_BOX (box), self->priv->destination_icon, FALSE, FALSE, 0);

	label_box = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (label_box);
	gtk_box_pack_start (GTK_BOX (box), label_box, TRUE, TRUE, 0);

	self->priv->destination_label = gtk_label_new ("");
	gtk_widget_show (self->priv->destination_label);
	gtk_box_pack_start (GTK_BOX (label_box), self->priv->destination_label, FALSE, FALSE, 0);

	self->priv->subfolder_label = gtk_label_new ("");
	gtk_label_set_ellipsize (GTK_LABEL (self->priv->subfolder_label), PANGO_ELLIPSIZE_END);
	gtk_misc_set_alignment (GTK_MISC (self->priv->subfolder_label), 0.0, 0.5);

	attr_list = pango_attr_list_new ();
	attr = pango_attr_foreground_new (45489, 13107, 1799);
	pango_attr_list_insert (attr_list, attr);
	g_object_set (self->priv->subfolder_label, "attributes", attr_list, NULL);
	pango_attr_list_unref (attr_list);

	gtk_widget_show (self->priv->subfolder_label);
	gtk_box_pack_start (GTK_BOX (label_box), self->priv->subfolder_label, TRUE, TRUE, 0);
}


GType
gth_import_destination_button_get_type (void)
{
	static GType type = 0;

	if (type == 0) {
		static const GTypeInfo g_define_type_info = {
			sizeof (GthImportDestinationButtonClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) gth_import_destination_button_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (GthImportDestinationButton),
			0,
			(GInstanceInitFunc) gth_import_destination_button_init,
			NULL
		};
		type = g_type_register_static (GTK_TYPE_BUTTON,
					       "GthImportDestinationButton",
					       &g_define_type_info,
					       0);
	}

	return type;
}


static void
preferences_dialog_destination_changed_cb (GthImportPreferencesDialog *dialog,
					   GthImportDestinationButton *self)
{
	GFile *destination;
	GFile *destination_example;

	destination = gth_import_preferences_dialog_get_destination (dialog);
	destination_example = gth_import_preferences_dialog_get_destination_example (dialog);
	if ((destination != NULL) && (destination_example != NULL)) {
		char *name;

		name = g_file_get_parse_name (destination);
		gtk_image_set_from_icon_name(GTK_IMAGE (self->priv->destination_icon), "folder", GTK_ICON_SIZE_MENU);
		gtk_label_set_text (GTK_LABEL (self->priv->destination_label), name);
		g_free (name);

		name = g_file_get_relative_path (destination, destination_example);
		if ((name != NULL) && (strcmp (name, "") != 0)) {
			char *example_path;

			example_path = g_strconcat (G_DIR_SEPARATOR_S, name, NULL);
			gtk_label_set_text (GTK_LABEL (self->priv->subfolder_label), example_path);

			g_free (example_path);
		}
		else
			gtk_label_set_text (GTK_LABEL (self->priv->subfolder_label), "");

		g_free (name);
	}
	else {
		gtk_image_set_from_icon_name (GTK_IMAGE (self->priv->destination_icon), "dialog-error", GTK_ICON_SIZE_MENU);
		gtk_label_set_text (GTK_LABEL (self->priv->destination_label), _("Invalid Destination"));
		gtk_label_set_text (GTK_LABEL (self->priv->subfolder_label), "");
	}

	_g_object_unref (destination_example);
	_g_object_unref (destination);
}


static void
gth_import_destination_button_construct (GthImportDestinationButton *button,
					 GthImportPreferencesDialog *dialog)
{
	g_signal_connect (dialog,
			  "destination_changed",
			  G_CALLBACK (preferences_dialog_destination_changed_cb),
			  button);
	g_signal_connect_swapped (button,
			  	  "clicked",
			  	  G_CALLBACK (gtk_window_present),
			  	  dialog);
}


GtkWidget *
gth_import_destination_button_new (GthImportPreferencesDialog *dialog)
{
	GtkWidget *button;

	button = (GtkWidget *) g_object_new (GTH_TYPE_IMPORT_DESTINATION_BUTTON, NULL);
	gth_import_destination_button_construct (GTH_IMPORT_DESTINATION_BUTTON (button), dialog);

	return button;
}
