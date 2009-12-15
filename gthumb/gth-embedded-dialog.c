/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2009 The Free Software Foundation, Inc.
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
#include "gth-embedded-dialog.h"


static gpointer parent_class = NULL;


struct _GthEmbeddedDialogPrivate {
	GtkWidget *icon_image;
	GtkWidget *primary_text_label;
	GtkWidget *secondary_text_label;
};


static void
gth_embedded_dialog_finalize (GObject *object)
{
	GthEmbeddedDialog *dialog;

	dialog = GTH_EMBEDDED_DIALOG (object);

	if (dialog->priv != NULL)
		dialog->priv = NULL;

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gth_embedded_dialog_class_init (GthEmbeddedDialogClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;

	object_class->finalize = gth_embedded_dialog_finalize;
}


static void
gth_embedded_dialog_init (GthEmbeddedDialog *dialog)
{
	dialog->priv = g_new0 (GthEmbeddedDialogPrivate, 1);
}


GType
gth_embedded_dialog_get_type (void)
{
        static GType type = 0;

        if (! type) {
                GTypeInfo type_info = {
			sizeof (GthEmbeddedDialogClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_embedded_dialog_class_init,
			NULL,
			NULL,
			sizeof (GthEmbeddedDialog),
			0,
			(GInstanceInitFunc) gth_embedded_dialog_init
		};

		type = g_type_register_static (GEDIT_TYPE_MESSAGE_AREA,
					       "GthEmbeddedEditorDialog",
					       &type_info,
					       0);
	}

        return type;
}


static void
gth_embedded_dialog_construct (GthEmbeddedDialog *self)
{
	GtkWidget *hbox_content;
	GtkWidget *image;
	GtkWidget *vbox;
	GtkWidget *primary_label;
	GtkWidget *secondary_label;
	
	hbox_content = gtk_hbox_new (FALSE, 8);
	gtk_widget_show (hbox_content);

	self->priv->icon_image = image = gtk_image_new ();
	gtk_box_pack_start (GTK_BOX (hbox_content), image, FALSE, FALSE, 0);
	gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0.5);

	vbox = gtk_vbox_new (FALSE, 6);
	gtk_widget_show (vbox);
	gtk_box_pack_start (GTK_BOX (hbox_content), vbox, TRUE, TRUE, 0);

	self->priv->primary_text_label = primary_label = gtk_label_new (NULL);
	gtk_box_pack_start (GTK_BOX (vbox), primary_label, TRUE, TRUE, 0);
	gtk_label_set_use_markup (GTK_LABEL (primary_label), TRUE);
	gtk_label_set_line_wrap (GTK_LABEL (primary_label), TRUE);
	gtk_misc_set_alignment (GTK_MISC (primary_label), 0, 0.5);
	gtk_misc_set_padding (GTK_MISC (primary_label), 0, 6);
	GTK_WIDGET_SET_FLAGS (primary_label, GTK_CAN_FOCUS);
	gtk_label_set_selectable (GTK_LABEL (primary_label), TRUE);
	
	self->priv->secondary_text_label = secondary_label = gtk_label_new (NULL);
	gtk_box_pack_start (GTK_BOX (vbox), secondary_label, TRUE, TRUE, 0);
	GTK_WIDGET_SET_FLAGS (secondary_label, GTK_CAN_FOCUS);
	gtk_label_set_use_markup (GTK_LABEL (secondary_label), TRUE);
	gtk_label_set_line_wrap (GTK_LABEL (secondary_label), TRUE);
	gtk_label_set_selectable (GTK_LABEL (secondary_label), TRUE);
	gtk_label_set_ellipsize (GTK_LABEL (secondary_label), PANGO_ELLIPSIZE_END);
	gtk_misc_set_alignment (GTK_MISC (secondary_label), 0, 0.5);
	
	gedit_message_area_set_contents (GEDIT_MESSAGE_AREA (self), hbox_content);
}


GtkWidget *
gth_embedded_dialog_new (const char *icon_stock_id,
			 const char *primary_text,
			 const char *secondary_text)
{
	GthEmbeddedDialog *self;

	self = g_object_new (GTH_TYPE_EMBEDDED_DIALOG, NULL);
	gth_embedded_dialog_construct (self);
	gth_embedded_dialog_set_icon (self, icon_stock_id);
	gth_embedded_dialog_set_primary_text (self, primary_text);
	gth_embedded_dialog_set_secondary_text (self, secondary_text);
	
	return (GtkWidget *) self;
}


void
gth_embedded_dialog_set_icon (GthEmbeddedDialog *dialog,
			      const char        *icon_stock_id)
{
	if (icon_stock_id == NULL) {
		gtk_widget_hide (dialog->priv->icon_image);
		return;
	}

	gtk_image_set_from_stock (GTK_IMAGE (dialog->priv->icon_image), icon_stock_id, GTK_ICON_SIZE_BUTTON);
	gtk_widget_show (dialog->priv->icon_image);
}


void
gth_embedded_dialog_set_gicon (GthEmbeddedDialog *dialog,
			       GIcon             *icon)
{
	if (icon == NULL) {
		gtk_widget_hide (dialog->priv->icon_image);
		return;
	}

	gtk_image_set_from_gicon (GTK_IMAGE (dialog->priv->icon_image), icon, GTK_ICON_SIZE_BUTTON);
	gtk_widget_show (dialog->priv->icon_image);
}


void
gth_embedded_dialog_set_primary_text (GthEmbeddedDialog *dialog,
				      const char        *text)
{
	char *escaped;
	char *markup;

	if (text == NULL) {
		gtk_widget_hide (dialog->priv->primary_text_label);
		return;
	}
	
	escaped = g_markup_escape_text (text, -1);
	markup = g_strdup_printf ("<b>%s</b>", escaped);
	gtk_label_set_markup (GTK_LABEL (dialog->priv->primary_text_label), markup);
	gtk_widget_show (dialog->priv->primary_text_label);
	
	g_free (markup);
	g_free (escaped);
}


void
gth_embedded_dialog_set_secondary_text (GthEmbeddedDialog *dialog,
					const char        *text)
{
	char *escaped;
	char *markup;

	if (text == NULL) {
		gtk_widget_hide (dialog->priv->secondary_text_label);
		return;
	}
	
	escaped = g_markup_escape_text (text, -1);
	markup = g_strdup_printf ("<small>%s</small>", escaped);
	gtk_label_set_markup (GTK_LABEL (dialog->priv->secondary_text_label), markup);
	gtk_widget_show (dialog->priv->secondary_text_label);
	
	g_free (markup);
	g_free (escaped);
}
