/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001, 2003 Free Software Foundation, Inc.
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
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <glade/glade.h>
#include <libgnomeui/libgnomeui.h>
#include <libgnomevfs/gnome-vfs.h>

#include "catalog.h"
#include "dlg-file-utils.h"
#include "glib-utils.h"
#include "gtk-utils.h"
#include "gconf-utils.h"
#include "gthumb-init.h"
#include "gth-browser.h"
#include "file-utils.h"
#include "md5.h"
#include "thumb-loader.h"

#ifdef WORDS_BIGENDIAN
# define SWAP(n)                                                        \
    (((n) << 24) | (((n) & 0xff00) << 8) | (((n) >> 8) & 0xff00) | ((n) >> 24))
#else
# define SWAP(n) (n)
#endif


#define GLADE_FILE  "gthumb_tools.glade"
#define MINI_IMAGE_SIZE   48
#define MAX_PATH_LENGTH   40
#define BLOCKSIZE         8192


enum {
	ICOLUMN_IMAGE_DATA,
	ICOLUMN_ICON,
	ICOLUMN_N,
	ICOLUMN_SIZE,
	INUMBER_OF_COLUMNS
};


enum {
	DCOLUMN_IMAGE_DATA,
	DCOLUMN_CHECKED,
	DCOLUMN_NAME,
	DCOLUMN_LOCATION,
	DCOLUMN_LAST_MODIFIED,
	DNUMBER_OF_COLUMNS
};


typedef struct {
	GthBrowser          *browser;
	GladeXML            *gui;

	GtkWidget           *dialog;
	GtkWidget           *results_dialog;

	GtkWidget           *fd_start_from_filechooserbutton;
	GtkWidget           *fd_include_subfolders_checkbutton;

	GtkWidget           *fdr_progress_table;
	GtkWidget           *fdr_current_dir_entry;
	GtkWidget           *fdr_current_image_entry;
	GtkWidget           *fdr_duplicates_label;
	GtkWidget           *fdr_images_treeview;
	GtkWidget           *fdr_duplicates_treeview;
	GtkWidget           *fdr_stop_button;
	GtkWidget           *fdr_close_button;

	GtkWidget           *fdr_notebook;
	GtkWidget           *fdr_ops_hbox;
	GtkWidget           *fdr_select_all_button;
	GtkWidget           *fdr_select_none_button;
	GtkWidget           *fdr_view_button;
	GtkWidget           *fdr_delete_button;

	GtkTreeModel        *images_model;
	GtkTreeModel        *duplicates_model;

	char                *start_from_path;
	gboolean             recursive;
	GnomeVFSAsyncHandle *handle;
	GnomeVFSURI         *uri;
	GList               *files;
	GList               *dirs;
	int                  duplicates;
	gboolean             scanning_dir;

	GList               *queue;
	gboolean             checking_file;
	gboolean             stopped;

	ThumbLoader         *loader;
	gboolean             loading_image;
	GList               *loader_queue;

	char                *current_path;

	char                 md5_buffer[BLOCKSIZE + 72];
	struct md5_ctx       md5_context;
	size_t               md5_bytes_read;
	md5_uint32           md5_len[2];
} DialogData;


typedef struct {
	int        ref;
	int        duplicates;
	int        size;
	GtkWidget *dup_label;
	GtkWidget *view_duplicates;
	gboolean   summed;          /* Used to make the sum of the
				     * size of all duplicates in
				     * update_duplicates_label */
} ImageDataCommon;


typedef struct {
	char            *path;
	char            *sum;
	ImageDataCommon *common;
	time_t           last_modified;
} ImageData;


static ImageData*
image_data_new (const char *filename,
		const char *sum)
{
	ImageData *idata;

	idata = g_new (ImageData, 1);
	idata->path = g_strdup (filename);
	idata->sum = g_strdup (sum);
	idata->common = NULL;

	idata->last_modified = get_file_mtime (idata->path);

	return idata;
}


static void
image_data_free (ImageData *idata)
{
	if (idata != NULL) {
		g_free (idata->path);
		g_free (idata->sum);
		if (--idata->common->ref == 0)
			g_free (idata->common);
		g_free (idata);
	}
}


static void
destroy_search_dialog_cb (GtkWidget  *widget,
			  DialogData *data)
{
	g_object_unref (G_OBJECT (data->gui));
	if (data->uri != NULL)
		gnome_vfs_uri_unref (data->uri);

	g_list_foreach (data->files, (GFunc) image_data_free, NULL);
	g_list_free (data->files);

	path_list_free (data->dirs);
	path_list_free (data->queue);

	g_free (data->start_from_path);

	if (data->loader != NULL)
		g_object_unref (data->loader);

	g_free (data);
}


static void search_finished (DialogData *data);


/* called when the "cancel" button in the progress dialog is pressed. */
static void
cancel_progress_dlg_cb (GtkWidget  *widget,
			DialogData *data)
{
	if (data->handle == NULL)
		return;

	gnome_vfs_async_cancel (data->handle);
	data->handle = NULL;

	data->stopped = TRUE;
}


static void
destroy_results_dialog_cb (GtkWidget  *widget,
			   DialogData *data)
{
	cancel_progress_dlg_cb (NULL, data);
	gtk_widget_destroy (data->dialog);
}


static void search_duplicates (DialogData *data);


static void
find_cb (GtkWidget  *widget,
	 DialogData *data)
{
	char *esc_path;

	esc_path = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (data->fd_start_from_filechooserbutton));
	data->start_from_path = gnome_vfs_unescape_string (esc_path, "");
	g_free (esc_path);

	data->recursive = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->fd_include_subfolders_checkbutton));

	gtk_widget_hide (data->dialog);
	search_duplicates (data);
}


static void
images_add_columns (GtkTreeView *treeview)
{
	GtkCellRenderer   *renderer;
	GtkTreeViewColumn *column;

	/* Image. */

	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column, _("Image"));

	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_attributes (column, renderer,
                                             "pixbuf", ICOLUMN_ICON,
                                             NULL);

	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

	/* N */

	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column, _("Duplicates"));

	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_attributes (column, renderer,
					     "text", ICOLUMN_N,
                                             NULL);

	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_sort_column_id (column, ICOLUMN_N);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

	/* Size */

	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column, _("Duplicates Size"));

	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_attributes (column, renderer,
					     "text", ICOLUMN_SIZE,
                                             NULL);

	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_sort_column_id (column, ICOLUMN_SIZE);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
}


static void
update_ops_sensitivity (DialogData *data)
{
	GtkTreeModel *model;
	GtkTreeIter   iter;
	ImageData    *idata;
	gboolean      image_list_empty = TRUE;

	model = data->duplicates_model;

	if (! gtk_tree_model_get_iter_first (model, &iter))
		return;

	gtk_tree_model_get (model, &iter, DCOLUMN_IMAGE_DATA, &idata, -1);

	do {
		gboolean checked;
		gtk_tree_model_get (model, &iter,
				    DCOLUMN_CHECKED, &checked, -1);

		if (checked) {
			image_list_empty = FALSE;
			break;

		} else if (! gtk_tree_model_iter_next (model, &iter))
			break;
	} while (TRUE);

	gtk_widget_set_sensitive (data->fdr_view_button, ! image_list_empty);
	gtk_widget_set_sensitive (data->fdr_delete_button, ! image_list_empty);
}


static void
image_toggled_cb (GtkCellRendererToggle *cell,
		  char                  *path_string,
		  gpointer               callback_data)
{
	DialogData   *data  = callback_data;
	GtkTreeModel *model = GTK_TREE_MODEL (data->duplicates_model);
	GtkTreeIter   iter;
	GtkTreePath  *path = gtk_tree_path_new_from_string (path_string);
	gboolean      value;

	gtk_tree_model_get_iter (model, &iter, path);
	value = gtk_cell_renderer_toggle_get_active (cell);
	gtk_list_store_set (GTK_LIST_STORE (model), &iter, DCOLUMN_CHECKED, !value, -1);

	gtk_tree_path_free (path);

	update_ops_sensitivity (data);
}


static void
duplicates_add_columns (DialogData  *data,
			GtkTreeView *treeview)
{
	GtkCellRenderer   *renderer;
	GtkTreeViewColumn *column;

	/* Name */

	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column, _("Name"));

	renderer = gtk_cell_renderer_toggle_new ();
	g_signal_connect (G_OBJECT (renderer),
			  "toggled",
			  G_CALLBACK (image_toggled_cb),
			  data);
	gtk_tree_view_insert_column_with_attributes (treeview,
						     -1, " ",
						     renderer,
						     "active", DCOLUMN_CHECKED,
						     NULL);

	/**/

	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_attributes (column, renderer,
					     "text", DCOLUMN_NAME,
                                             NULL);

	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_sort_column_id (column, DCOLUMN_NAME);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

	/* Last modified */

	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column, _("Last modified"));

	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_attributes (column, renderer,
					     "text", DCOLUMN_LAST_MODIFIED,
                                             NULL);

	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_sort_column_id (column, DCOLUMN_LAST_MODIFIED);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

	/* Location */

	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column, _("Location"));

	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_attributes (column, renderer,
					     "text", DCOLUMN_LOCATION,
                                             NULL);

	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_sort_column_id (column, DCOLUMN_LOCATION);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
}


static int
size_column_sort_func (GtkTreeModel *model,
		       GtkTreeIter  *a,
		       GtkTreeIter  *b,
		       gpointer      user_data)
{
	ImageData *idata1, *idata2;
	int        size1, size2;

        gtk_tree_model_get (model, a, ICOLUMN_IMAGE_DATA, &idata1, -1);
        gtk_tree_model_get (model, b, ICOLUMN_IMAGE_DATA, &idata2, -1);

	size1 = idata1->common->size * idata1->common->duplicates;
	size2 = idata2->common->size * idata2->common->duplicates;

	if (size1 == size2)
		return 0;
	else if (size1 < size2)
		return -1;
	else
		return 1;
}


static int
n_column_sort_func (GtkTreeModel *model,
		    GtkTreeIter  *a,
		    GtkTreeIter  *b,
		    gpointer      user_data)
{
	ImageData *idata1, *idata2;
	int        n1, n2;

        gtk_tree_model_get (model, a, ICOLUMN_IMAGE_DATA, &idata1, -1);
        gtk_tree_model_get (model, b, ICOLUMN_IMAGE_DATA, &idata2, -1);

	n1 = idata1->common->duplicates;
	n2 = idata2->common->duplicates;

	if (n1 == n2)
		return 0;
	else if (n1 < n2)
		return -1;
	else
		return 1;
}


static int
time_column_sort_func (GtkTreeModel *model,
		       GtkTreeIter  *a,
		       GtkTreeIter  *b,
		       gpointer      user_data)
{
	ImageData *idata1, *idata2;
	time_t     t1, t2;

        gtk_tree_model_get (model, a, DCOLUMN_IMAGE_DATA, &idata1, -1);
        gtk_tree_model_get (model, b, DCOLUMN_IMAGE_DATA, &idata2, -1);

	t1 = idata1->last_modified;
	t2 = idata2->last_modified;

	if (t1 == t2)
		return 0;
	else if (t1 < t2)
		return -1;
	else
		return 1;
}


static int
default_sort_func (GtkTreeModel *model,
		   GtkTreeIter  *a,
		   GtkTreeIter  *b,
		   gpointer      user_data)
{
	return 0;
}


static void
images_selection_changed_cb (GtkTreeSelection *selection,
			     gpointer          p)
{
	DialogData        *data = p;
	GtkTreeIter        iter;
	ImageData         *idata;
	GList             *scan;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (data->fdr_images_treeview));
	if (selection == NULL)
		return;

	if (! gtk_tree_selection_get_selected (selection, NULL, &iter))
		return;

	gtk_tree_model_get (data->images_model, &iter, ICOLUMN_IMAGE_DATA, &idata, -1);

	gtk_list_store_clear (GTK_LIST_STORE (data->duplicates_model));

	for (scan = data->files; scan; scan = scan->next) {
		ImageData    *idata2 = scan->data;
		GtkTreeIter   iter2;
		char         *location, *location_utf8;
		const char   *name;
		char         *name_utf8;
		char          time_txt[50], *time_txt_utf8;
		struct tm    *tm;

		if (idata->common != idata2->common)
			continue;

		location = remove_level_from_path (idata2->path);
		location_utf8 = g_filename_display_name (location);
		g_free (location);

		name = file_name_from_path (idata2->path);
		name_utf8 = g_filename_display_name (name);

		tm = localtime (&idata2->last_modified);
		strftime (time_txt, 50, _("%d %B %Y, %H:%M"), tm);
		time_txt_utf8 = g_locale_to_utf8 (time_txt, -1, 0, 0, 0);

		gtk_list_store_append (GTK_LIST_STORE (data->duplicates_model), &iter2);
		gtk_list_store_set (GTK_LIST_STORE (data->duplicates_model),
				    &iter2,
				    DCOLUMN_IMAGE_DATA, idata2,
				    DCOLUMN_NAME, name_utf8,
				    DCOLUMN_LOCATION, location_utf8,
				    DCOLUMN_LAST_MODIFIED, time_txt_utf8,
				    -1);

		g_free (time_txt_utf8);
		g_free (name_utf8);
		g_free (location_utf8);
	}
}


static void
select_all_cb (GtkWidget  *widget,
	       DialogData *data)
{
	GtkTreeIter iter;

	if (! gtk_tree_model_get_iter_first (data->duplicates_model, &iter))
		return;

	do {
		gtk_list_store_set (GTK_LIST_STORE (data->duplicates_model),
				    &iter,
				    DCOLUMN_CHECKED, TRUE,
				    -1);
	} while (gtk_tree_model_iter_next (data->duplicates_model, &iter));

	update_ops_sensitivity (data);
}


static void
select_none_cb (GtkWidget  *widget,
		DialogData *data)
{
	GtkTreeIter iter;

	if (! gtk_tree_model_get_iter_first (data->duplicates_model, &iter))
		return;

	do {
		gtk_list_store_set (GTK_LIST_STORE (data->duplicates_model),
				    &iter,
				    DCOLUMN_CHECKED, FALSE,
				    -1);
	} while (gtk_tree_model_iter_next (data->duplicates_model, &iter));

	update_ops_sensitivity (data);
}


static GList *
get_checked_images (DialogData *data)
{
	GtkTreeIter  iter;
	GList       *list = NULL;

	if (! gtk_tree_model_get_iter_first (data->duplicates_model, &iter))
		return list;

	do {
		ImageData *idata;
		gboolean   checked;

		gtk_tree_model_get (data->duplicates_model, &iter,
				    DCOLUMN_CHECKED, &checked,
				    DCOLUMN_IMAGE_DATA, &idata, -1);

		if (checked)
			list = g_list_prepend (list, g_strdup (idata->path));

	} while (gtk_tree_model_iter_next (data->duplicates_model, &iter));

	return list;
}


static void
view_cb (GtkWidget  *widget,
	 DialogData *data)
{
	GList     *list, *scan;
	Catalog   *catalog;
	char      *catalog_path;
	char      *catalog_name;
	GError    *gerror;

	list = get_checked_images (data);
	if (list == NULL)
		return;

	catalog = catalog_new ();

	catalog_name = g_strconcat (_("Duplicates"),
				    CATALOG_EXT,
				    NULL);
	catalog_path = get_catalog_full_path (catalog_name);
	g_free (catalog_name);

	catalog_set_path (catalog, catalog_path);

	for (scan = list; scan; scan = scan->next)
		catalog_add_item (catalog, (char*) scan->data);

	if (! catalog_write_to_disk (catalog, &gerror))
		_gtk_error_dialog_from_gerror_run (GTK_WINDOW (data->browser), &gerror);
	else {
		/* View the Drag&Drop catalog. */
		gth_browser_go_to_catalog (data->browser, catalog_path);
	}

	catalog_free (catalog);
	path_list_free (list);
	g_free (catalog_path);
}


static void update_entry (DialogData *data, ImageData  *idata);
static void update_duplicates_label (DialogData *data);


static void
delete_images_from_lists (DialogData *data,
			  GList      *list)
{
	GtkTreeModel *model;
	GtkTreeIter   iter;
	int           images_deleted;
	ImageData    *idata = NULL, *idata2;

	/* delete entries from the duplicates list. */

	model = data->duplicates_model;

	if (! gtk_tree_model_get_iter_first (model, &iter))
		return;

	gtk_tree_model_get (model, &iter, DCOLUMN_IMAGE_DATA, &idata, -1);

	do {
		gboolean checked;
		gtk_tree_model_get (model, &iter,
				    DCOLUMN_CHECKED, &checked, -1);

		if (checked) {
			gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
			if (! gtk_tree_model_get_iter_first (model, &iter))
				break;
		} else if (! gtk_tree_model_iter_next (model, &iter))
			break;
	} while (TRUE);

	/* delete entry from the images list if appropriate. */

	model = data->images_model;

	images_deleted = g_list_length (list);
	if (images_deleted == idata->common->duplicates + 1) {

		if (! gtk_tree_model_get_iter_first (model, &iter))
			return;

		do {
			gtk_tree_model_get (model, &iter,
					    ICOLUMN_IMAGE_DATA, &idata2, -1);

			if (idata->common == idata2->common) {
				gtk_list_store_remove (GTK_LIST_STORE (model),
						       &iter);
				break;
			}

		} while (gtk_tree_model_iter_next (model, &iter));

		data->duplicates -= idata->common->duplicates;
		idata->common->duplicates = 0;
	} else {
		data->duplicates -= images_deleted;
		idata->common->duplicates -= images_deleted;
		update_entry (data, idata);
	}

	update_duplicates_label (data);
}


static void
delete_cb (GtkWidget  *widget,
	   DialogData *data)
{
	GList *list;

	list = get_checked_images (data);
	if (list == NULL)
		return;

	if (dlg_file_delete__confirm (GTH_WINDOW (data->browser),
				      path_list_dup (list),
				      _("Checked images will be moved to the Trash, are you sure?")))
		delete_images_from_lists (data, list);

	path_list_free (list);
}


void
dlg_duplicates (GthBrowser *browser)
{
	DialogData        *data;
	GtkWidget         *ok_button;
	GtkWidget         *close_button;
	GtkTreeSelection  *selection;
	char              *esc_uri = NULL;

	data = g_new0 (DialogData, 1);

	data->browser = browser;
	data->gui = glade_xml_new (GTHUMB_GLADEDIR "/" GLADE_FILE, NULL,
				   NULL);

	if (! data->gui) {
		g_warning ("Could not find " GLADE_FILE "\n");
		return;
	}

	/* Get the widgets. */

	data->dialog = glade_xml_get_widget (data->gui, "duplicates_dialog");
	data->results_dialog = glade_xml_get_widget (data->gui, "duplicates_results_dialog");

	data->fd_start_from_filechooserbutton = glade_xml_get_widget (data->gui, "fd_start_from_filechooserbutton");
	data->fd_include_subfolders_checkbutton = glade_xml_get_widget (data->gui, "fd_include_subfolders_checkbutton");

	data->fdr_progress_table = glade_xml_get_widget (data->gui, "fdr_progress_table");
	data->fdr_current_image_entry = glade_xml_get_widget (data->gui, "fdr_current_image_entry");
	data->fdr_duplicates_label = glade_xml_get_widget (data->gui, "fdr_duplicates_label");
	data->fdr_current_dir_entry = glade_xml_get_widget (data->gui, "fdr_current_dir_entry");
	data->fdr_images_treeview = glade_xml_get_widget (data->gui, "fdr_images_treeview");
	data->fdr_duplicates_treeview = glade_xml_get_widget (data->gui, "fdr_duplicates_treeview");
	data->fdr_stop_button = glade_xml_get_widget (data->gui, "fdr_stop_button");
	data->fdr_close_button = glade_xml_get_widget (data->gui, "fdr_close_button");

	data->fdr_ops_hbox = glade_xml_get_widget (data->gui, "fdr_ops_hbox");
	data->fdr_select_all_button = glade_xml_get_widget (data->gui, "fdr_select_all_button");
	data->fdr_select_none_button = glade_xml_get_widget (data->gui, "fdr_select_none_button");
	data->fdr_view_button = glade_xml_get_widget (data->gui, "fdr_view_button");
	data->fdr_delete_button = glade_xml_get_widget (data->gui, "fdr_delete_button");
	data->fdr_notebook = glade_xml_get_widget (data->gui, "fdr_notebook");

	ok_button = glade_xml_get_widget (data->gui, "fd_ok_button");
	close_button = glade_xml_get_widget (data->gui, "fd_close_button");

	/* Set widgets data. */

	esc_uri = gnome_vfs_escape_host_and_path_string (gth_browser_get_current_directory (data->browser));
	gtk_file_chooser_set_uri (GTK_FILE_CHOOSER (data->fd_start_from_filechooserbutton), esc_uri);
	g_free (esc_uri);

	/* * Images model */

	data->images_model = GTK_TREE_MODEL (gtk_list_store_new (INUMBER_OF_COLUMNS,
								 G_TYPE_POINTER,
								 GDK_TYPE_PIXBUF,
								 G_TYPE_STRING,
								 G_TYPE_STRING));

	gtk_tree_view_set_model (GTK_TREE_VIEW (data->fdr_images_treeview),
				 data->images_model);
	g_object_unref (data->images_model);
	images_add_columns (GTK_TREE_VIEW (data->fdr_images_treeview));

	gtk_tree_sortable_set_default_sort_func (GTK_TREE_SORTABLE (data->images_model), default_sort_func,  NULL, NULL);

	gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (data->images_model),
					 ICOLUMN_N, n_column_sort_func,
					 NULL, NULL);

	gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (data->images_model),
					 ICOLUMN_SIZE, size_column_sort_func,
					 NULL, NULL);

	/* * Duplicates model */

	data->duplicates_model = GTK_TREE_MODEL (gtk_list_store_new (DNUMBER_OF_COLUMNS,
								     G_TYPE_POINTER,
								     G_TYPE_BOOLEAN,
								     G_TYPE_STRING,
								     G_TYPE_STRING,
								     G_TYPE_STRING));

	gtk_tree_view_set_model (GTK_TREE_VIEW (data->fdr_duplicates_treeview),
				 data->duplicates_model);
	g_object_unref (data->duplicates_model);
	duplicates_add_columns (data, GTK_TREE_VIEW (data->fdr_duplicates_treeview));

	gtk_tree_sortable_set_default_sort_func (GTK_TREE_SORTABLE (data->duplicates_model), default_sort_func,  NULL, NULL);

	gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (data->duplicates_model),
					 DCOLUMN_LAST_MODIFIED, time_column_sort_func,
					 NULL, NULL);

	/* Set the signals handlers. */

	g_signal_connect (G_OBJECT (data->dialog),
			  "destroy",
			  G_CALLBACK (destroy_search_dialog_cb),
			  data);
	g_signal_connect_swapped (G_OBJECT (close_button),
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  G_OBJECT (data->dialog));
	g_signal_connect (G_OBJECT (ok_button),
			  "clicked",
			  G_CALLBACK (find_cb),
			  data);

	g_signal_connect (G_OBJECT (data->results_dialog),
			  "destroy",
			  G_CALLBACK (destroy_results_dialog_cb),
			  data);
	g_signal_connect_swapped (G_OBJECT (data->fdr_close_button),
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  G_OBJECT (data->results_dialog));
	g_signal_connect (G_OBJECT (data->fdr_stop_button),
			  "clicked",
			  G_CALLBACK (cancel_progress_dlg_cb),
			  data);

	g_signal_connect (G_OBJECT (data->fdr_select_all_button),
			  "clicked",
			  G_CALLBACK (select_all_cb),
			  data);
	g_signal_connect (G_OBJECT (data->fdr_select_none_button),
			  "clicked",
			  G_CALLBACK (select_none_cb),
			  data);
	g_signal_connect (G_OBJECT (data->fdr_view_button),
			  "clicked",
			  G_CALLBACK (view_cb),
			  data);
	g_signal_connect (G_OBJECT (data->fdr_delete_button),
			  "clicked",
			  G_CALLBACK (delete_cb),
			  data);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (data->fdr_images_treeview));
	g_signal_connect (G_OBJECT (selection),
                          "changed",
                          G_CALLBACK (images_selection_changed_cb),
                          data);

	/* Run dialog. */

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog),
				      GTK_WINDOW (browser));
	gtk_widget_grab_focus (data->fdr_stop_button);
	gtk_widget_show (data->dialog);
}



/* ---- */


static gboolean
get_iter_from_image_data (DialogData  *data,
			  ImageData   *idata,
			  GtkTreeIter *iter)
{
	if (! gtk_tree_model_get_iter_first (data->images_model, iter))
		return FALSE;

	do {
		ImageData *idata2;

		gtk_tree_model_get (data->images_model, iter,
				    ICOLUMN_IMAGE_DATA, &idata2,
				    -1);
		if (idata->common == idata2->common)
			return TRUE;
	} while (gtk_tree_model_iter_next (data->images_model, iter));

	return FALSE;
}


static void start_loading_image (DialogData *data);


static void
set_image_and_go_on (DialogData  *data,
		     GdkPixbuf   *pixbuf)
{
	ImageData   *idata;
	GList       *node;
	GdkPixbuf   *pixbuf2;
	GtkTreeIter  iter;

	node = data->loader_queue;
	idata = node->data;
	data->loader_queue = g_list_remove_link (data->loader_queue, node);
	g_list_free (node);

	if (! get_iter_from_image_data (data, idata, &iter))
		return;

	if (pixbuf != NULL)
		pixbuf2 = gdk_pixbuf_new (gdk_pixbuf_get_colorspace (pixbuf),
					  TRUE,
					  gdk_pixbuf_get_bits_per_sample (pixbuf),
					  MINI_IMAGE_SIZE,
					  MINI_IMAGE_SIZE);
	else
		pixbuf2 = gdk_pixbuf_new (GDK_COLORSPACE_RGB,
					  TRUE,
					  8,
					  MINI_IMAGE_SIZE,
					  MINI_IMAGE_SIZE);

	gdk_pixbuf_fill (pixbuf2, 0xFFFFFF00);

	if (pixbuf != NULL) {
		int start_x, start_y;

		start_x = (MINI_IMAGE_SIZE - gdk_pixbuf_get_width (pixbuf)) / 2;
		start_y = (MINI_IMAGE_SIZE - gdk_pixbuf_get_height (pixbuf)) / 2;
		gdk_pixbuf_copy_area (pixbuf,
				      0, 0,
				      gdk_pixbuf_get_width (pixbuf),
				      gdk_pixbuf_get_height (pixbuf),
				      pixbuf2,
				      start_x, start_y);
	}

	gtk_list_store_set (GTK_LIST_STORE (data->images_model),
			    &iter,
			    ICOLUMN_ICON, pixbuf2,
			    -1);
	g_object_unref (pixbuf2);

	start_loading_image (data);
}


static void
image_loader_done (ImageLoader *il,
		   gpointer     user_data)
{
	DialogData  *data = user_data;
	GdkPixbuf   *pixbuf;

	pixbuf = thumb_loader_get_pixbuf (data->loader);
	set_image_and_go_on (data, pixbuf);
}


static void
image_loader_error (ImageLoader *il,
		    gpointer     user_data)
{
	set_image_and_go_on (user_data, NULL);
}


static void
start_loading_image (DialogData *data)
{
	ImageData *idata;

	if (data->stopped) {
		g_list_free (data->loader_queue);
		data->loader_queue = NULL;
		data->loading_image = FALSE;
		search_finished (data);
		return;
	}

	if (data->loader_queue == NULL) {
		data->loading_image = FALSE;
		search_finished (data);
		return;
	}

	idata = data->loader_queue->data;

	data->loading_image = TRUE;
	thumb_loader_set_path (data->loader, idata->path, NULL);
	thumb_loader_start (data->loader);
}


static void
queue_image_to_load (DialogData *data,
		     ImageData  *idata)
{
	if (data->loader == NULL) {
		data->loader = THUMB_LOADER (thumb_loader_new (NULL, MINI_IMAGE_SIZE, MINI_IMAGE_SIZE));
		thumb_loader_use_cache (data->loader, TRUE);
		g_signal_connect (G_OBJECT (data->loader),
				  "thumb_done",
				  G_CALLBACK (image_loader_done),
				  data);
		g_signal_connect (G_OBJECT (data->loader),
				  "thumb_error",
				  G_CALLBACK (image_loader_error),
				  data);
	}

	data->loader_queue = g_list_append (data->loader_queue, idata);

	if (data->loading_image)
		return;

	start_loading_image (data);
}


static void
update_entry (DialogData *data,
	      ImageData  *idata)
{
	GtkTreeIter  iter;
	char        *size_txt;
	char        *n_txt;

	if (! get_iter_from_image_data (data, idata, &iter))
		return;

	size_txt = gnome_vfs_format_file_size_for_display (idata->common->size * idata->common->duplicates);
	n_txt = g_strdup_printf ("%d", idata->common->duplicates);

	gtk_list_store_set (GTK_LIST_STORE (data->images_model),
			    &iter,
			    ICOLUMN_N, n_txt,
			    ICOLUMN_SIZE, size_txt,
			    -1);

	g_free (n_txt);
	g_free (size_txt);
}


static void
add_entry (DialogData *data,
	   ImageData  *idata)
{
        GtkTreeIter  iter;
	char        *size_txt;

	size_txt = gnome_vfs_format_file_size_for_display (idata->common->size);

	gtk_list_store_append (GTK_LIST_STORE (data->images_model), &iter);
	gtk_list_store_set (GTK_LIST_STORE (data->images_model),
			    &iter,
			    ICOLUMN_IMAGE_DATA, idata,
			    ICOLUMN_N, "1",
			    ICOLUMN_SIZE, size_txt,
			    -1);

	queue_image_to_load (data, idata);

	g_free (size_txt);
}


static void
update_duplicates_label (DialogData *data)
{
	char  *label, *size_txt;
	int    size = 0;
	GList *scan;

	if (data->duplicates == 0) {
		gtk_label_set_text (GTK_LABEL (data->fdr_duplicates_label), "0");
		return;
	}

	for (scan = data->files; scan; scan = scan->next) {
		ImageData *idata = scan->data;
		idata->common->summed = FALSE;
	}

	for (scan = data->files; scan; scan = scan->next) {
		ImageData *idata = scan->data;
		if (! idata->common->summed) {
			size += idata->common->size * idata->common->duplicates;
			idata->common->summed = TRUE;
		}
	}

	size_txt = gnome_vfs_format_file_size_for_display (size);
	label = g_strdup_printf ("%d (%s)", data->duplicates, size_txt);
	g_free (size_txt);

	gtk_label_set_text (GTK_LABEL (data->fdr_duplicates_label), label);
	g_free (label);
}


static void
check_image (DialogData *data,
	     ImageData  *idata)
{
	GList *scan;

	for (scan = data->files; scan; scan = scan->next) {
		ImageData *idata2 = scan->data;

		if ((strcmp (idata->sum, idata2->sum) == 0)
		    && ! same_uri (idata->path, idata2->path)) {
			idata->common = idata2->common;
			idata->common->ref++;
			idata->common->duplicates++;

			if (idata->common->duplicates == 1)
				add_entry (data, idata);
			else
				update_entry (data, idata);

			data->duplicates++;
			update_duplicates_label (data);

			return;
		}
	}

	idata->common = g_new0 (ImageDataCommon, 1);
	idata->common->ref++;
	idata->common->size = get_file_size (idata->path);
}


static void search_dir_async (DialogData *data, const char *dir);


static void
scan_next_dir (DialogData *data)
{
	gboolean good_dir_to_search_into = TRUE;

	if (! data->recursive || data->stopped) {
		data->scanning_dir = FALSE;
		search_finished (data);
		return;
	}

	do {
		GList *first_dir;
		char  *dir;

		if (data->dirs == NULL) {
			data->scanning_dir = FALSE;
			search_finished (data);
			return;
		}

		first_dir = data->dirs;
		data->dirs = g_list_remove_link (data->dirs, first_dir);
		dir = (char*) first_dir->data;
		g_list_free (first_dir);

		good_dir_to_search_into = ! file_is_hidden (file_name_from_path (dir));
		if (good_dir_to_search_into)
			search_dir_async (data, dir);
		g_free (dir);
	} while (! good_dir_to_search_into);
}


/* ----- */


static void start_next_checksum (DialogData *data);


static void
close_callback (GnomeVFSAsyncHandle *handle,
                GnomeVFSResult       result,
                gpointer             callback_data)
{
	DialogData *data = callback_data;

	g_free (data->current_path);
	data->current_path = NULL;

	start_next_checksum (data);
}


/* This array contains the bytes used to pad the buffer to the next
   64-byte boundary.  (RFC 1321, 3.1: Step 1)  */
static const unsigned char fillbuf[64] = { 0x80, 0 /* , 0, 0, ...  */ };


static void
process_block (DialogData *data)
{
	size_t  pad, sum;
	char   *buffer;

	buffer = data->md5_buffer;
	sum = data->md5_bytes_read;

	/* RFC 1321 specifies the possible length of the file up to 2^64 bits.
	   Here we only compute the number of bytes.  Do a double word
	   increment.  */
	data->md5_len[0] += sum;
	if (data->md5_len[0] < sum)
		++data->md5_len[1];

	if (sum == BLOCKSIZE) {
		/* Process buffer with BLOCKSIZE bytes.  Note that
		   BLOCKSIZE % 64 == 0
		*/
		md5_process_block (buffer, BLOCKSIZE, &data->md5_context);
		return;
	}

	/* We can copy 64 byte because the buffer is always big enough.
	   FILLBUF  contains the needed bits.  */
	memcpy (&buffer[sum], fillbuf, 64);

	/* Compute amount of padding bytes needed.  Alignment is done to
	   (N + PAD) % 64 == 56
	   There is always at least one byte padded.  I.e. even the alignment
	   is correctly aligned 64 padding bytes are added.  */
	pad = sum & 63;
	pad = pad >= 56 ? 64 + 56 - pad : 56 - pad;

	/* Put the 64-bit file length in *bits* at the end of the buffer.  */
	*(md5_uint32 *) &buffer[sum + pad] = SWAP (data->md5_len[0] << 3);
	*(md5_uint32 *) &buffer[sum + pad + 4] = SWAP ((data->md5_len[1] << 3) | (data->md5_len[0] >> 29));

	/* Process last bytes.  */
	md5_process_block (buffer, sum + pad + 8, &data->md5_context);
}


static void
read_callback (GnomeVFSAsyncHandle *handle,
               GnomeVFSResult       result,
               gpointer             buffer,
               GnomeVFSFileSize     bytes_requested,
               GnomeVFSFileSize     bytes_read,
               gpointer             callback_data)
{
	DialogData *data = callback_data;

	if (result == GNOME_VFS_ERROR_EOF) {
		unsigned char  md5_sum[16];
		char           sum[16*2+1] = "";
		size_t         cnt;
		ImageData     *idata;

		process_block (data);
		md5_read_ctx (&data->md5_context, md5_sum);

		for (cnt = 0; cnt < 16; ++cnt) {
			char s[3];
			snprintf (s, 3, "%02x", md5_sum[cnt]);
			strncat (sum, s, 2);
		}

#if 0
		{
			FILE *fp;

			fp = fopen (data->current_path, "rb");
			if (fp != NULL) {
				unsigned char md5_sum[16];
				char          sum2[16*2+1] = "";
				int           error;
				size_t        cnt;

				error = md5_stream (fp, md5_sum);
				fclose (fp);

				for (cnt = 0; cnt < 16; ++cnt) {
					char s[3];
					snprintf (s, 3, "%02x", md5_sum[cnt]);
					strncat (sum2, s, 2);
				}

				printf ("%s <-> %s", sum, sum2);
				if (strcmp (sum, sum2) != 0)
					printf ("[ERROR]\n");
				else
					printf ("\n");
			}
		}
#endif

		idata = image_data_new (data->current_path, sum);
		data->files = g_list_prepend (data->files, idata);
		check_image (data, idata);

		gnome_vfs_async_close (handle, close_callback, data);

		return;
	}

        if (result != GNOME_VFS_OK) {
		gnome_vfs_async_close (handle, close_callback, data);
		return;
        }

	/* Take care for partial reads. */

	data->md5_bytes_read += bytes_read;
	if (data->md5_bytes_read < BLOCKSIZE) {
		gnome_vfs_async_read (handle,
				      data->md5_buffer + data->md5_bytes_read,
				      BLOCKSIZE - data->md5_bytes_read,
				      read_callback,
				      data);
		return;
	}

	/* Process block. */

	process_block (data);

	/* Read next block. */

	data->md5_bytes_read = 0;
	gnome_vfs_async_read (handle,
			      data->md5_buffer + data->md5_bytes_read,
			      BLOCKSIZE - data->md5_bytes_read,
			      read_callback,
			      data);
}


static void
open_callback  (GnomeVFSAsyncHandle *handle,
                GnomeVFSResult result,
                gpointer callback_data)
{
	DialogData *data = callback_data;

        if (result != GNOME_VFS_OK) {
		g_free (data->current_path);
		data->current_path = NULL;

		start_next_checksum (data);

		return;
        }

	gnome_vfs_async_read (handle,
			      data->md5_buffer + data->md5_bytes_read,
			      BLOCKSIZE - data->md5_bytes_read,
			      read_callback,
			      data);
}


static void
start_next_checksum (DialogData *data)
{
	GnomeVFSAsyncHandle *handle;
	GList               *node;

	if ((data->queue == NULL) || data->stopped) {
		data->checking_file = FALSE;
		scan_next_dir (data);
		return;
	}

	data->checking_file = TRUE;

	node = data->queue;
	data->current_path = node->data;
	data->queue = g_list_remove_link (data->queue, node);
	g_list_free (node);

	/**/

	_gtk_entry_set_filename_text (GTK_ENTRY (data->fdr_current_image_entry),
				      file_name_from_path (data->current_path));

	/**/

	md5_init_ctx (&data->md5_context);
	data->md5_len[0] = 0;
	data->md5_len[1] = 0;
	data->md5_bytes_read = 0;

	gnome_vfs_async_open (&handle,
			      data->current_path,
			      GNOME_VFS_OPEN_READ,
                              GNOME_VFS_PRIORITY_MIN,
                              open_callback,
			      data);
}


static void
search_finished (DialogData *data)
{
	if (data->scanning_dir || data->checking_file || data->loading_image)
		return;

	gtk_entry_set_text (GTK_ENTRY (data->fdr_current_dir_entry), "");
	gtk_entry_set_text (GTK_ENTRY (data->fdr_current_image_entry), "");

	gtk_widget_set_sensitive (data->fdr_stop_button, FALSE);
	gtk_widget_set_sensitive (data->fdr_progress_table, FALSE);
	gtk_widget_set_sensitive (data->fdr_close_button, TRUE);
	if (data->duplicates > 0)
		gtk_widget_set_sensitive (data->fdr_ops_hbox, TRUE);

	gtk_widget_grab_focus (data->fdr_close_button);

	if (data->duplicates == 0)
		gtk_notebook_set_current_page (GTK_NOTEBOOK (data->fdr_notebook), 1);
}


static void
directory_load_cb (GnomeVFSAsyncHandle *handle,
		   GnomeVFSResult       result,
		   GList               *list,
		   guint                entries_read,
		   gpointer             callback_data)
{
	DialogData       *data = callback_data;
	GnomeVFSFileInfo *info;
	GList            *node, *files = NULL;

	for (node = list; node != NULL; node = node->next) {
		GnomeVFSURI *full_uri = NULL;
		char        *str_uri, *unesc_uri;

		info = node->data;

		switch (info->type) {
		case GNOME_VFS_FILE_TYPE_REGULAR:
			full_uri = gnome_vfs_uri_append_file_name (data->uri, info->name);
			str_uri = gnome_vfs_uri_to_string (full_uri, GNOME_VFS_URI_HIDE_TOPLEVEL_METHOD);
			unesc_uri = gnome_vfs_unescape_string (str_uri, NULL);

			if (file_is_image (unesc_uri, eel_gconf_get_boolean (PREF_FAST_FILE_TYPE, FALSE)))
				files = g_list_prepend (files, unesc_uri);
			else
				g_free (unesc_uri);

			g_free (str_uri);
			break;

		case GNOME_VFS_FILE_TYPE_DIRECTORY:
			if (SPECIAL_DIR (info->name))
				break;

			full_uri = gnome_vfs_uri_append_path (data->uri, info->name);
			str_uri = gnome_vfs_uri_to_string (full_uri, GNOME_VFS_URI_HIDE_TOPLEVEL_METHOD);
			unesc_uri = gnome_vfs_unescape_string (str_uri, NULL);

			data->dirs = g_list_prepend (data->dirs,  unesc_uri);
			g_free (str_uri);
			break;

		default:
			break;
		}

		if (full_uri)
			gnome_vfs_uri_unref (full_uri);
	}

	if (files != NULL)
		data->queue = g_list_concat (data->queue, files);

	if (result == GNOME_VFS_ERROR_EOF) {
		if (data->queue != NULL) {
			if (! data->checking_file)
				start_next_checksum (data);
		} else
			scan_next_dir (data);

	} else if (result != GNOME_VFS_OK) {
		char *path;

		path = gnome_vfs_uri_to_string (data->uri,
						GNOME_VFS_URI_HIDE_NONE);
		g_warning ("Cannot load directory \"%s\": %s\n", path,
			   gnome_vfs_result_to_string (result));
		g_free (path);

		data->scanning_dir = FALSE;
		search_finished (data);
	}
}


static void
search_dir_async (DialogData *data, const char *path)
{
	_gtk_entry_set_filename_text (GTK_ENTRY (data->fdr_current_dir_entry), path);
	gtk_entry_set_text (GTK_ENTRY (data->fdr_current_image_entry), "");

	if (data->uri != NULL)
		gnome_vfs_uri_unref (data->uri);
	data->uri = new_uri_from_path (path);

	data->scanning_dir = TRUE;

	gnome_vfs_async_load_directory_uri (
		& (data->handle),
		data->uri,
		GNOME_VFS_FILE_INFO_FOLLOW_LINKS,
		128 /* items_per_notification FIXME */,
		GNOME_VFS_PRIORITY_MIN,
		directory_load_cb,
		data);
}


static void
search_duplicates (DialogData *data)
{
	gtk_widget_set_sensitive (data->fdr_stop_button, TRUE);
	gtk_widget_set_sensitive (data->fdr_close_button, FALSE);
	gtk_widget_set_sensitive (data->fdr_ops_hbox, FALSE);

	gtk_widget_show (data->results_dialog);

	update_duplicates_label (data);

	search_dir_async (data, data->start_from_path);
}
