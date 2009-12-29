/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2009 Free Software Foundation, Inc.
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
#include "gth-edit-metadata-dialog.h"


#define GTH_EDIT_METADATA_DIALOG_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GTH_TYPE_EDIT_METADATA_DIALOG, GthEditMetadataDialogPrivate))


static gpointer gth_edit_metadata_dialog_parent_class = NULL;


struct _GthEditMetadataDialogPrivate {
	GtkWidget *notebook;
};


static void
gth_edit_metadata_dialog_class_init (GthEditMetadataDialogClass *klass)
{
	gth_edit_metadata_dialog_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GthEditMetadataDialogPrivate));
}


static void
gth_edit_metadata_dialog_init (GthEditMetadataDialog *edit_metadata_dialog)
{
	edit_metadata_dialog->priv = GTH_EDIT_METADATA_DIALOG_GET_PRIVATE (edit_metadata_dialog);
}


GType
gth_edit_metadata_dialog_get_type (void)
{
	static GType type = 0;

	if (type == 0) {
		static const GTypeInfo g_define_type_info = {
			sizeof (GthEditMetadataDialogClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) gth_edit_metadata_dialog_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (GthEditMetadataDialog),
			0,
			(GInstanceInitFunc) gth_edit_metadata_dialog_init,
			NULL
		};
		type = g_type_register_static (GTK_TYPE_DIALOG,
					       "GthEditMetadataDialog",
					       &g_define_type_info,
					       0);
	}

	return type;
}


static void
gth_edit_metadata_dialog_construct (GthEditMetadataDialog *self)
{
	GArray *pages;
	int     i;

	gtk_window_set_resizable (GTK_WINDOW (self), TRUE);
	gtk_dialog_set_has_separator (GTK_DIALOG (self), FALSE);
	gtk_box_set_spacing (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (self))), 5);
	gtk_container_set_border_width (GTK_CONTAINER (self), 5);

	gtk_dialog_add_button (GTK_DIALOG (self), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
	gtk_dialog_add_button (GTK_DIALOG (self), GTK_STOCK_SAVE, GTK_RESPONSE_OK);

	self->priv->notebook = gtk_notebook_new ();
	gtk_widget_show (self->priv->notebook);
	gtk_container_set_border_width (GTK_CONTAINER (self->priv->notebook), 5);
	gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (self))), self->priv->notebook, TRUE, TRUE, 0);

	pages = gth_main_get_type_set ("edit-metadata-dialog-page");
	if (pages == NULL)
		return;

	for (i = 0; i < pages->len; i++) {
		GType      page_type;
		GtkWidget *page;

		page_type = g_array_index (pages, GType, i);
		page = g_object_new (page_type, NULL);
		if (! GTH_IS_EDIT_METADATA_PAGE (page)) {
			g_object_unref (page);
			continue;
		}

		gtk_widget_show (page);
		gtk_notebook_append_page (GTK_NOTEBOOK (self->priv->notebook),
					  page,
					  gtk_label_new (gth_edit_metadata_page_get_name (GTH_EDIT_METADATA_PAGE (page))));
	}
}


GtkWidget *
gth_edit_metadata_dialog_new (void)
{
	GthEditMetadataDialog *self;

	self = g_object_new (GTH_TYPE_EDIT_METADATA_DIALOG, NULL);
	gth_edit_metadata_dialog_construct (self);

	return (GtkWidget *) self;
}


void
gth_edit_metadata_dialog_set_file (GthEditMetadataDialog *dialog,
				   GthFileData           *file)
{
	char  *title;
	GList *pages;
	GList *scan;

	if (file == NULL)
		return;

	title = g_strdup_printf (_("%s Metadata"), g_file_info_get_display_name (file->info));
	gtk_window_set_title (GTK_WINDOW (dialog), title);

	pages = gtk_container_get_children (GTK_CONTAINER (dialog->priv->notebook));
	for (scan = pages; scan; scan = scan->next)
		gth_edit_metadata_page_set_file (GTH_EDIT_METADATA_PAGE (scan->data), file);

	g_list_free (pages);
	g_free (title);
}


void
gth_edit_metadata_dialog_update_info (GthEditMetadataDialog *dialog,
				      GFileInfo             *info)
{
	GList *pages;
	GList *scan;

	pages = gtk_container_get_children (GTK_CONTAINER (dialog->priv->notebook));
	for (scan = pages; scan; scan = scan->next)
		gth_edit_metadata_page_update_info (GTH_EDIT_METADATA_PAGE (scan->data), info);

	g_list_free (pages);
}


/* -- gth_edit_metadata_dialog_page -- */


GType
gth_edit_metadata_page_get_type (void) {
	static GType gth_edit_metadata_page_type_id = 0;
	if (gth_edit_metadata_page_type_id == 0) {
		static const GTypeInfo g_define_type_info = {
			sizeof (GthEditMetadataPageIface),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) NULL,
			(GClassFinalizeFunc) NULL,
			NULL,
			0,
			0,
			(GInstanceInitFunc) NULL,
			NULL
		};
		gth_edit_metadata_page_type_id = g_type_register_static (G_TYPE_INTERFACE, "GthEditMetadataPageIface", &g_define_type_info, 0);
	}
	return gth_edit_metadata_page_type_id;
}


void
gth_edit_metadata_page_set_file (GthEditMetadataPage *self,
				 GthFileData         *file_data)
{
	GTH_EDIT_METADATA_PAGE_GET_INTERFACE (self)->set_file (self, file_data);
}


void
gth_edit_metadata_page_update_info (GthEditMetadataPage *self,
				    GFileInfo           *info)
{
	GTH_EDIT_METADATA_PAGE_GET_INTERFACE (self)->update_info (self, info);
}


const char *
gth_edit_metadata_page_get_name (GthEditMetadataPage *self)
{
	return GTH_EDIT_METADATA_PAGE_GET_INTERFACE (self)->get_name (self);
}
