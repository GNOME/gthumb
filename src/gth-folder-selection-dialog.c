/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001 The Free Software Foundation, Inc.
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
#include <string.h>
#include <gtk/gtk.h>
#include <libgnome/libgnome.h>
#include <libgnomeui/libgnomeui.h>
#include "gth-folder-selection-dialog.h"
#include "bookmark-list.h"
#include "bookmarks.h"
#include "typedefs.h"
#include "preferences.h"
#include "gtk-utils.h"
#include "file-utils.h"


#define RC_RECENT_FILE      ".gnome2/gthumb/recents"
#define DEFAULT_LIST_WIDTH  300 
#define DEFAULT_LIST_HEIGHT 120 
#define MAX_RECENT_LIST     20


struct _GthFolderSelectionPrivate
{
	char         *title;
	BookmarkList *bookmark_list;
	Bookmarks    *bookmarks;
	BookmarkList *recent_list;
	Bookmarks    *recents;
	GtkWidget    *file_entry;
};


static GtkDialogClass *parent_class = NULL;


static void
gth_folder_selection_destroy (GtkObject *object)
{
	GthFolderSelection *folder_sel;

	folder_sel = GTH_FOLDER_SELECTION (object);

	if (folder_sel->priv != NULL) {
		g_free (folder_sel->priv->title);
		folder_sel->priv->title = NULL;

		if (folder_sel->priv->bookmark_list != NULL) {
			bookmark_list_free (folder_sel->priv->bookmark_list);
			folder_sel->priv->bookmark_list = NULL;
		}

		if (folder_sel->priv->bookmarks != NULL) {
			bookmarks_free (folder_sel->priv->bookmarks);
			folder_sel->priv->bookmarks = NULL;
		}


		if (folder_sel->priv->recent_list != NULL) {
			bookmark_list_free (folder_sel->priv->recent_list);
			folder_sel->priv->recent_list = NULL;
		}

		if (folder_sel->priv->recents != NULL) {
			bookmarks_free (folder_sel->priv->recents);
			folder_sel->priv->recents = NULL;
		}

		g_free (folder_sel->priv);
		folder_sel->priv = NULL;
	}

	GTK_OBJECT_CLASS (parent_class)->destroy (object);
}


static void
gth_folder_selection_class_init (GthFolderSelectionClass *class)
{
	GtkObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	object_class = (GtkObjectClass*) class;

	object_class->destroy = gth_folder_selection_destroy;
}


static void
gth_folder_selection_init (GthFolderSelection *folder_sel)
{
	folder_sel->priv = g_new0 (GthFolderSelectionPrivate, 1);
}


static gboolean
list_view_button_press_cb (GdkEventButton     *event,
			   GthFolderSelection *folder_sel,
			   gboolean            bookmarks)
{
        GtkTreePath  *path;
        GtkTreeIter   iter;
        char         *folder_path, *utf8_folder_path;
	BookmarkList *bookmark_list;

	if (bookmarks) 
		bookmark_list = folder_sel->priv->bookmark_list;
	else
		bookmark_list = folder_sel->priv->recent_list;

        if (! gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (bookmark_list->list_view),
                                             event->x, event->y,
                                             &path, NULL, NULL, NULL))
                return FALSE;

        if (! gtk_tree_model_get_iter (GTK_TREE_MODEL (bookmark_list->list_store), 
				       &iter, 
				       path)) {
                gtk_tree_path_free (path);
                return FALSE;
        }

        gtk_tree_model_get (GTK_TREE_MODEL (bookmark_list->list_store), &iter,
                            2, &folder_path,
                            -1);

	if (bookmarks)
		utf8_folder_path = g_filename_to_utf8 (folder_path + FILE_PREFIX_L, -1, 0, 0, 0);
	else
		utf8_folder_path = g_filename_to_utf8 (folder_path, -1, 0, 0, 0);

	gth_folder_selection_set_folder (GTH_FOLDER_SELECTION (folder_sel),
					 utf8_folder_path);

        g_free (utf8_folder_path);
        g_free (folder_path);
        gtk_tree_path_free (path);

        return FALSE;
}


static int
bookmark_button_press_cb (GtkWidget      *widget,
			  GdkEventButton *event,
			  gpointer        callback_data)
{
	GthFolderSelection *folder_sel = callback_data;
	return list_view_button_press_cb (event,
					  folder_sel,
					  TRUE);
}


static int
recent_button_press_cb (GtkWidget      *widget,
			GdkEventButton *event,
			gpointer        callback_data)
{
	GthFolderSelection *folder_sel = callback_data;
	return list_view_button_press_cb (event,
					  folder_sel,
					  FALSE);
}


static void
list_view_activated_cb (GtkTreePath        *path,
			GthFolderSelection *folder_sel,
			gboolean            bookmarks)
{
	GtkTreeIter   iter;
	char         *folder_path, *utf8_folder_path;
	BookmarkList *bookmark_list;

	if (bookmarks) 
		bookmark_list = folder_sel->priv->bookmark_list;
	else
		bookmark_list = folder_sel->priv->recent_list;

	if (! gtk_tree_model_get_iter (GTK_TREE_MODEL (bookmark_list->list_store),
				       &iter, 
				       path)) 
		return;
	
	gtk_tree_model_get (GTK_TREE_MODEL (bookmark_list->list_store), &iter,
			    2, &folder_path,
			    -1);
	
	if (bookmarks)
		utf8_folder_path = g_filename_to_utf8 (folder_path + FILE_PREFIX_L, -1, 0, 0, 0);
	else
		utf8_folder_path = g_filename_to_utf8 (folder_path, -1, 0, 0, 0);

	gth_folder_selection_set_folder (GTH_FOLDER_SELECTION (folder_sel),
					 utf8_folder_path);

        g_free (utf8_folder_path);
        g_free (folder_path);

	gtk_dialog_response (GTK_DIALOG (folder_sel), GTK_RESPONSE_OK);
}


static void
bookmark_activated_cb (GtkTreeView       *tree_view,
		       GtkTreePath       *path,
		       GtkTreeViewColumn *column,
		       gpointer           callback_data)
{
	GthFolderSelection *folder_sel = callback_data;
	list_view_activated_cb (path, folder_sel, TRUE);
}


static void
recent_activated_cb (GtkTreeView       *tree_view,
		     GtkTreePath       *path,
		     GtkTreeViewColumn *column,
		     gpointer           callback_data)
{
	GthFolderSelection *folder_sel = callback_data;
	list_view_activated_cb (path, folder_sel, FALSE);
}


static void
file_sel_ok_clicked_cb (GObject  *object,
			gpointer  data)
{
	GtkWidget          *file_sel = data;
	GthFolderSelection *folder_sel;
	char               *folder;

	folder_sel = g_object_get_data (G_OBJECT (file_sel), "folder_sel");
	
	folder = g_strdup (gtk_file_selection_get_filename (GTK_FILE_SELECTION (file_sel)));
	gtk_widget_destroy (file_sel);

	if (folder == NULL) 
		return;

	gth_folder_selection_set_folder (folder_sel, folder);

	g_free (folder);
}


static void
browse_button_clicked_cb (GtkWidget *widget, 
			  gpointer   data)
{
	GthFolderSelection *folder_sel = data;
	GtkWidget   *file_sel;
	GtkWidget   *entry;
	const char  *utf8_folder;
	char        *folder;
	
	file_sel = gtk_file_selection_new (folder_sel->priv->title);
	
	entry = folder_sel->priv->file_entry;

	utf8_folder = gtk_entry_get_text (GTK_ENTRY (entry));
	folder = g_filename_from_utf8 (utf8_folder, -1, 0, 0, 0);
	if (folder[strlen (folder) - 1] != '/') {
		char *tmp;
		tmp = g_strconcat (folder, "/", NULL);
		g_free (folder);
		folder = tmp;
	}

	gtk_file_selection_set_filename (GTK_FILE_SELECTION (file_sel), folder);
	g_free (folder);
	
	g_object_set_data (G_OBJECT (file_sel), "folder_sel", folder_sel);

	g_signal_connect (G_OBJECT (GTK_FILE_SELECTION (file_sel)->ok_button),
			  "clicked", 
			  G_CALLBACK (file_sel_ok_clicked_cb), 
			  file_sel);
	
	g_signal_connect_swapped (G_OBJECT (GTK_FILE_SELECTION (file_sel)->cancel_button),
				  "clicked", 
				  G_CALLBACK (gtk_widget_destroy),
				  G_OBJECT (file_sel));

	gtk_widget_set_sensitive (GTK_WIDGET (GTK_FILE_SELECTION (file_sel)->file_list)->parent, FALSE);
	gtk_window_set_transient_for (GTK_WINDOW (file_sel), 
				      GTK_WINDOW (folder_sel));
	gtk_window_set_modal (GTK_WINDOW (file_sel), TRUE);
	gtk_widget_show (file_sel);
}


static void
folder_sel__response_cb (GObject *object,
			 int      response_id,
			 gpointer data)
{
	GthFolderSelection *folder_sel = data;
	char               *folder, *dir;	

	if (response_id != GTK_RESPONSE_OK) 
		return;

	folder = gth_folder_selection_get_folder (folder_sel);
	if (folder == NULL) 
		return;

	dir = remove_ending_separator (folder);

	bookmarks_remove_all_instances (folder_sel->priv->recents, dir);
	bookmarks_add (folder_sel->priv->recents, dir, TRUE, FALSE);
	bookmarks_set_max_lines (folder_sel->priv->recents, MAX_RECENT_LIST);
	bookmarks_write_to_disk (folder_sel->priv->recents);

	g_free (dir);
	g_free (folder);
}


static void
gth_folder_selection_construct (GthFolderSelection *folder_sel,
				const char         *title)
{
	GtkDialog *dialog;
	GtkWidget *main_box, *vbox, *vbox2, *hbox, *hbox2;
	GtkWidget *label;
	GtkWidget *browse_button;
	char      *label_txt;
	Bookmarks *bookmarks;
	GList     *scan;

	folder_sel->priv->title = g_strdup (title);
	gtk_window_set_title (GTK_WINDOW (folder_sel), title);

	dialog = GTK_DIALOG (folder_sel);
	gtk_dialog_add_buttons (dialog, 
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				GTK_STOCK_OK, GTK_RESPONSE_OK,
				NULL);
	gtk_dialog_set_default_response (dialog, GTK_RESPONSE_OK);
	gtk_dialog_set_has_separator (dialog, FALSE);

	main_box = gtk_vbox_new (FALSE, 12);
	gtk_box_pack_start (GTK_BOX (dialog->vbox), main_box, TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (main_box), 6);

	gtk_container_set_border_width (GTK_CONTAINER (dialog->vbox), 6);
	gtk_box_set_spacing (GTK_BOX (dialog->vbox), 8);

	gtk_container_set_border_width (GTK_CONTAINER (dialog), 6);

	/* Folder */

	vbox = gtk_vbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (main_box), vbox, FALSE, FALSE, 0);

	label = gtk_label_new (NULL);
	label_txt = g_strdup_printf ("<b>%s</b>", _("Folder"));
	gtk_label_set_markup (GTK_LABEL (label), label_txt);
	g_free (label_txt);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

	label = gtk_label_new ("    ");
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	/**/

	hbox2 = gtk_hbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (hbox), hbox2, TRUE, TRUE, 0);

	folder_sel->priv->file_entry = gtk_entry_new ();
	gtk_entry_set_activates_default (GTK_ENTRY (folder_sel->priv->file_entry), TRUE);

	gtk_box_pack_start (GTK_BOX (hbox2), folder_sel->priv->file_entry, TRUE, TRUE, 0);

	browse_button = gtk_button_new_with_mnemonic (_("_Browse..."));
	gtk_box_pack_start (GTK_BOX (hbox2), browse_button, FALSE, FALSE, 0);

	/* Recents */

	vbox = gtk_vbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (main_box), vbox, TRUE, TRUE, 0);

	label = gtk_label_new (NULL);
	label_txt = g_strdup_printf ("<b>%s</b>", _("_Recents:"));
	gtk_label_set_markup (GTK_LABEL (label), label_txt);
	gtk_label_set_use_underline(GTK_LABEL (label), TRUE);
	g_free (label_txt);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

	label = gtk_label_new ("    ");
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	vbox2 = gtk_vbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (hbox), vbox2, FALSE, FALSE, 0);

	folder_sel->priv->recent_list = bookmark_list_new ();
	gtk_widget_set_size_request (folder_sel->priv->recent_list->root_widget, DEFAULT_LIST_WIDTH, DEFAULT_LIST_HEIGHT);
	gtk_box_pack_start (GTK_BOX (vbox2), folder_sel->priv->recent_list->root_widget, TRUE, TRUE, 0);

	folder_sel->priv->recents = bookmarks_new (RC_RECENT_FILE);
	bookmarks_load_from_disk (folder_sel->priv->recents);
	bookmark_list_set (folder_sel->priv->recent_list, 
			   folder_sel->priv->recents->list);

	/* Bookmarks */

	vbox = gtk_vbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (main_box), vbox, TRUE, TRUE, 0);

	label = gtk_label_new (NULL);
	label_txt = g_strdup_printf ("<b>%s</b>", _("_Bookmarks:"));
	gtk_label_set_markup (GTK_LABEL (label), label_txt);
	gtk_label_set_use_underline(GTK_LABEL (label), TRUE);
	g_free (label_txt);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

	label = gtk_label_new ("    ");
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	vbox2 = gtk_vbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (hbox), vbox2, FALSE, FALSE, 0);

	folder_sel->priv->bookmark_list = bookmark_list_new ();
	gtk_widget_set_size_request (folder_sel->priv->bookmark_list->root_widget, DEFAULT_LIST_WIDTH, DEFAULT_LIST_HEIGHT);
	gtk_box_pack_start (GTK_BOX (vbox2), folder_sel->priv->bookmark_list->root_widget, TRUE, TRUE, 0);

	folder_sel->priv->bookmarks = bookmarks_new (RC_BOOKMARKS_FILE);
	bookmarks = folder_sel->priv->bookmarks;

	bookmarks_load_from_disk (bookmarks);
	for (scan = bookmarks->list; scan; ) {
		char *path = scan->data;
		if (! pref_util_location_is_file (path)) {
			bookmarks->list = g_list_remove_link (bookmarks->list, scan);
			g_free (scan->data);
			g_list_free (scan);
			scan = bookmarks->list;
		} else
			scan = scan->next;
	}

	bookmark_list_set (folder_sel->priv->bookmark_list, 
			   folder_sel->priv->bookmarks->list);

	gtk_widget_show_all (main_box);

	/**/

	g_signal_connect (G_OBJECT (folder_sel->priv->bookmark_list->list_view),
                          "button_press_event",
                          G_CALLBACK (bookmark_button_press_cb),
                          folder_sel);

	g_signal_connect (G_OBJECT (folder_sel->priv->bookmark_list->list_view),
                          "row_activated",
                          G_CALLBACK (bookmark_activated_cb),
                          folder_sel);

	g_signal_connect (G_OBJECT (folder_sel->priv->recent_list->list_view),
                          "button_press_event",
                          G_CALLBACK (recent_button_press_cb),
                          folder_sel);

	g_signal_connect (G_OBJECT (folder_sel->priv->recent_list->list_view),
                          "row_activated",
                          G_CALLBACK (recent_activated_cb),
                          folder_sel);

	g_signal_connect (G_OBJECT (browse_button),
                          "clicked",
                          G_CALLBACK (browse_button_clicked_cb),
                          folder_sel);

	g_signal_connect (G_OBJECT (folder_sel),
			  "response", 
			  G_CALLBACK (folder_sel__response_cb), 
			  folder_sel);
}


GType
gth_folder_selection_get_type ()
{
        static GType type = 0;

        if (! type) {
                GTypeInfo type_info = {
			sizeof (GthFolderSelectionClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_folder_selection_class_init,
			NULL,
			NULL,
			sizeof (GthFolderSelection),
			0,
			(GInstanceInitFunc) gth_folder_selection_init
		};

		type = g_type_register_static (GTK_TYPE_DIALOG,
					       "GthFolderSelection",
					       &type_info,
					       0);
	}

        return type;
}


GtkWidget*     
gth_folder_selection_new (const char *title)
{
	GtkWidget *widget;

	widget = GTK_WIDGET (g_object_new (GTH_TYPE_FOLDER_SELECTION, NULL));
	gth_folder_selection_construct (GTH_FOLDER_SELECTION (widget), title);

	return widget;
}


void
gth_folder_selection_set_folder (GthFolderSelection *fsel,
				 const char         *folder)
{
	_gtk_entry_set_filename_text (GTK_ENTRY (fsel->priv->file_entry), folder);
}


char *
gth_folder_selection_get_folder (GthFolderSelection *fsel)
{
	return _gtk_entry_get_filename_text (GTK_ENTRY (fsel->priv->file_entry));
}
