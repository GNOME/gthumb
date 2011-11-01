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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <glib/gi18n.h>
#include "gth-edit-metadata-dialog.h"


G_DEFINE_TYPE (GthEditMetadataDialog,  gth_edit_metadata_dialog, GTK_TYPE_DIALOG)


struct _GthEditMetadataDialogPrivate {
	GtkWidget *notebook;
	GtkWidget *save_changed_checkbutton;
};


static void
gth_edit_metadata_dialog_class_init (GthEditMetadataDialogClass *klass)
{
	g_type_class_add_private (klass, sizeof (GthEditMetadataDialogPrivate));
}


static void
gth_edit_metadata_dialog_init (GthEditMetadataDialog *self)
{
	GtkWidget *vbox;
	GArray    *pages;
	int        i;

	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_EDIT_METADATA_DIALOG, GthEditMetadataDialogPrivate);

	gtk_window_set_resizable (GTK_WINDOW (self), TRUE);
	gtk_box_set_spacing (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (self))), 5);
	gtk_container_set_border_width (GTK_CONTAINER (self), 5);

	gtk_dialog_add_button (GTK_DIALOG (self), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
	gtk_dialog_add_button (GTK_DIALOG (self), GTK_STOCK_SAVE, GTK_RESPONSE_OK);

	vbox = gtk_vbox_new (FALSE, 6);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);
	gtk_widget_show (vbox);
	gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (self))), vbox, TRUE, TRUE, 0);

	self->priv->notebook = gtk_notebook_new ();
	gtk_widget_show (self->priv->notebook);
	gtk_box_pack_start (GTK_BOX (vbox), self->priv->notebook, TRUE, TRUE, 0);

	self->priv->save_changed_checkbutton = gtk_check_button_new_with_mnemonic (_("Save only cha_nged fields"));
	gtk_widget_show (self->priv->save_changed_checkbutton);
	gtk_box_pack_start (GTK_BOX (vbox), self->priv->save_changed_checkbutton, FALSE, FALSE, 0);

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
	return g_object_new (GTH_TYPE_EDIT_METADATA_DIALOG, NULL);
}


void
gth_edit_metadata_dialog_set_file_list (GthEditMetadataDialog *dialog,
				        GList                 *file_list)
{
	int    n_files;
	char  *title;
	GList *pages;
	GList *scan;

	if (file_list == NULL)
		return;

	n_files = g_list_length (file_list);
	if (n_files == 1) {
		GthFileData *file_data = file_list->data;

		/* Translators: the %s symbol in the string is a file name */
		title = g_strdup_printf (_("%s Metadata"), g_file_info_get_display_name (file_data->info));
		gtk_window_set_title (GTK_WINDOW (dialog), title);
	}
	else {
		title = g_strdup_printf (g_dngettext (NULL, "%d file", "%d files", n_files), n_files);
		gtk_window_set_title (GTK_WINDOW (dialog), title);
	}

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->priv->save_changed_checkbutton), n_files > 1);
	gtk_widget_set_sensitive (dialog->priv->save_changed_checkbutton, n_files > 1);

	pages = gtk_container_get_children (GTK_CONTAINER (dialog->priv->notebook));
	for (scan = pages; scan; scan = scan->next)
		gth_edit_metadata_page_set_file_list (GTH_EDIT_METADATA_PAGE (scan->data), file_list);

	g_list_free (pages);
	g_free (title);
}


void
gth_edit_metadata_dialog_update_info (GthEditMetadataDialog *dialog,
				      GList                 *file_list /* GthFileData list */)
{
	gboolean  only_modified_fields;
	GList    *pages;
	GList    *scan;

	only_modified_fields = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->priv->save_changed_checkbutton));
	pages = gtk_container_get_children (GTK_CONTAINER (dialog->priv->notebook));
	for (scan = pages; scan; scan = scan->next) {
		GList *scan_file;

		for (scan_file = file_list; scan_file; scan_file = scan_file->next) {
			GthFileData *file_data = scan_file->data;
			gth_edit_metadata_page_update_info (GTH_EDIT_METADATA_PAGE (scan->data), file_data->info, only_modified_fields);
		}
	}

	g_list_free (pages);
}


/* -- gth_edit_metadata_dialog_page -- */


G_DEFINE_INTERFACE (GthEditMetadataPage, gth_edit_metadata_page, 0)


static void
gth_edit_metadata_page_default_init (GthEditMetadataPageInterface *iface)
{
	/* void */
}


void
gth_edit_metadata_page_set_file_list (GthEditMetadataPage *self,
				      GList               *file_list)
{
	GTH_EDIT_METADATA_PAGE_GET_INTERFACE (self)->set_file_list (self, file_list);
}


void
gth_edit_metadata_page_update_info (GthEditMetadataPage *self,
				    GFileInfo           *info,
				    gboolean             only_modified_fields)
{
	GTH_EDIT_METADATA_PAGE_GET_INTERFACE (self)->update_info (self, info, only_modified_fields);
}


const char *
gth_edit_metadata_page_get_name (GthEditMetadataPage *self)
{
	return GTH_EDIT_METADATA_PAGE_GET_INTERFACE (self)->get_name (self);
}
