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
#include <time.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <libgnome/libgnome.h>
#include <libgnomeui/gnome-dialog.h>
#include <libgnomeui/gnome-dialog-util.h>
#include <libgnomeui/gnome-dateedit.h>
#include <libgnomeui/gnome-file-entry.h>
#include <glade/glade.h>
#include <libgnomevfs/gnome-vfs.h>
#include "catalog.h"
#include "comments.h"
#include "dlg-search.h"
#include "gthumb-init.h"
#include "gconf-utils.h"
#include "search.h"
#include "gthumb-window.h"
#include "gtk-utils.h"
#include "glib-utils.h"
#include "utf8-fnmatch.h"


enum {
	C_USE_CATEGORY_COLUMN,
	C_CATEGORY_COLUMN,
	C_NUM_COLUMNS
};

enum {
	P_FILENAME_COLUMN,
	P_FOLDER_COLUMN,
	P_NUM_COLUMNS
};

#define CATEGORY_SEPARATOR_C   ';'
#define CATEGORY_SEPARATOR_STR ";"

static void dlg_search_ui (GThumbWindow *window, 
			   char         *catalog_path, 
			   gboolean      start_search);


void 
dlg_search (GtkWidget *widget, gpointer data)
{
	GThumbWindow *window = data;
	dlg_search_ui (window, NULL, FALSE);
}


void 
dlg_catalog_edit_search (GThumbWindow *window,
			 const char   *catalog_path)
{
	dlg_search_ui (window, g_strdup (catalog_path), FALSE);
}


void 
dlg_catalog_search (GThumbWindow *window,
		    const char   *catalog_path)
{
	dlg_search_ui (window, g_strdup (catalog_path), TRUE);
}


/* -- implementation -- */


#define SEARCH_GLADE_FILE      "gthumb_search.glade"

typedef struct {
	GThumbWindow   *window;
	GladeXML       *gui;

	GtkWidget      *dialog;
	GtkWidget      *search_progress_dialog;

	GtkWidget      *s_start_from_fileentry;
	GtkWidget      *s_start_from_entry;
	GtkWidget      *s_include_subfold_checkbutton;
	GtkWidget      *s_filename_entry;
	GtkWidget      *s_comment_entry;
	GtkWidget      *s_place_entry;
	GtkWidget      *s_categories_entry; 
	GtkWidget      *s_choose_categories_button;
	GtkWidget      *s_date_optionmenu;
	GtkWidget      *s_date_dateedit;
	GtkWidget      *s_at_least_one_cat_radiobutton;
	GtkWidget      *s_all_cat_radiobutton;

	GtkWidget      *p_progress_tree_view;
	GtkListStore   *p_progress_tree_model;
	GtkWidget      *p_current_dir_entry;
	GtkWidget      *p_notebook;
	GtkWidget      *p_view_button;
	GtkWidget      *p_search_button;
	GtkWidget      *p_cancel_button;
	GtkWidget      *p_close_button;
	GtkWidget      *p_searching_in_hbox;
	GtkWidget      *p_images_label;

	GtkWidget      *categories_dialog;
	GtkWidget      *c_categories_entry;
	GtkWidget      *c_categories_treeview;
	GtkWidget      *c_ok_button;
	GtkWidget      *c_cancel_button;
	GtkListStore   *c_categories_list_model;

	/* -- search data -- */

	SearchData     *search_data;
	char          **file_patterns;
	char          **comment_patterns;
	char          **place_patterns;
	char          **keywords_patterns; 

	GnomeVFSAsyncHandle *handle;
	GnomeVFSURI    *uri;
	GList          *files;
	GList          *dirs;

	char           *catalog_path;
	gboolean        search_comments;
} DialogData;


static void
free_search_criteria_data (DialogData *data)
{
	if (data->file_patterns) {
		g_strfreev (data->file_patterns);
		data->file_patterns = NULL;
	}

	if (data->comment_patterns) {
		g_strfreev (data->comment_patterns);
		data->comment_patterns = NULL;
	}

	if (data->place_patterns) {
		g_strfreev (data->place_patterns);
		data->place_patterns = NULL;
	}

	if (data->keywords_patterns) {
		g_strfreev (data->keywords_patterns);
		data->keywords_patterns = NULL;
	} 
}


static void
free_search_results_data (DialogData *data)
{
	if (data->files) {
		path_list_free (data->files);
		data->files = NULL;
	}

	if (data->dirs) {
		path_list_free (data->dirs);
		data->dirs = NULL;
	}
}


/* called when the main dialog is closed. */
static void
destroy_cb (GtkWidget  *widget, 
	    DialogData *data)
{
	eel_gconf_set_boolean (PREF_SEARCH_RECURSIVE, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->s_include_subfold_checkbutton)));

	/**/

	g_object_unref (G_OBJECT (data->gui));
	free_search_criteria_data (data);
	free_search_results_data (data);
	search_data_free (data->search_data);
	if (data->uri != NULL)
		gnome_vfs_uri_unref (data->uri);
	if (data->catalog_path != NULL)
		g_free (data->catalog_path);
	g_free (data);
}


static void search_images_async (DialogData *data);


/* called when the "search" button is pressed. */
static void
search_clicked_cb (GtkWidget  *widget, 
		   DialogData *data)
{
	char       *full_path;
	const char *entry;
	char       *utf8_path;

	/* collect search data. */

	free_search_criteria_data (data);
	search_data_free (data->search_data);

	data->search_data = search_data_new ();

	/* * start from */

	utf8_path = gnome_file_entry_get_full_path (GNOME_FILE_ENTRY (data->s_start_from_fileentry), FALSE);
	full_path = g_locale_from_utf8 (utf8_path, -1, 0, 0, 0);
	g_free (utf8_path);

	search_data_set_start_from (data->search_data, full_path);
	g_free (full_path);

	/* * recursive */

	search_data_set_recursive (data->search_data, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->s_include_subfold_checkbutton)));

	/* * file pattern */

	entry = gtk_entry_get_text (GTK_ENTRY (data->s_filename_entry));
	search_data_set_file_pattern (data->search_data, entry);
	if (entry != NULL) 
		data->file_patterns = search_util_get_patterns (entry);

	/* * comment pattern */

	entry = gtk_entry_get_text (GTK_ENTRY (data->s_comment_entry));
	search_data_set_comment_pattern (data->search_data, entry);
	if (entry != NULL) 
		data->comment_patterns = search_util_get_patterns (entry);

	/* * place pattern */

	entry = gtk_entry_get_text (GTK_ENTRY (data->s_place_entry));
	search_data_set_place_pattern (data->search_data, entry);
	if (entry != NULL) 
		data->place_patterns = search_util_get_patterns (entry);

	/* * keywords pattern */

	entry = gtk_entry_get_text (GTK_ENTRY (data->s_categories_entry));
	search_data_set_keywords_pattern (data->search_data, entry, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->s_all_cat_radiobutton)));
	if (entry != NULL) 
		data->keywords_patterns = search_util_get_patterns (entry);

	/* * date scope pattern */

	search_data_set_date_scope (data->search_data, gtk_option_menu_get_history (GTK_OPTION_MENU (data->s_date_optionmenu)));

	/* * date */

	search_data_set_date (data->search_data, gnome_date_edit_get_time (GNOME_DATE_EDIT (data->s_date_dateedit)));

	/* pop up the progress dialog. */

	gtk_widget_hide (data->dialog);
	gtk_notebook_set_current_page (GTK_NOTEBOOK (data->p_notebook), 0);
	gtk_widget_set_sensitive (data->p_searching_in_hbox, TRUE);
	gtk_widget_set_sensitive (data->p_view_button, FALSE);
	gtk_widget_set_sensitive (data->p_search_button, FALSE);
	gtk_widget_set_sensitive (data->p_close_button, FALSE);
	gtk_label_set_text (GTK_LABEL (data->p_images_label), "");
	gtk_widget_show (data->search_progress_dialog);

	search_images_async (data);
}


static void cancel_progress_dlg_cb (GtkWidget *widget, DialogData *data);


/* called when the progress dialog is closed. */
static void
destroy_progress_cb (GtkWidget *widget, 
		     DialogData *data)
{
	cancel_progress_dlg_cb (widget, data);
	gtk_widget_destroy (data->dialog);
}


/* called when the "new search" button in the progress dialog is pressed. */
static void
new_search_clicked_cb (GtkWidget *widget, 
		       DialogData *data)
{
	cancel_progress_dlg_cb (widget, data);
	gtk_widget_hide (data->search_progress_dialog);
	gtk_widget_show (data->dialog);
}


/* called when the "view" button in the progress dialog is pressed. */
static void
view_result_cb (GtkWidget  *widget, 
		DialogData *data)
{
	GThumbWindow *window = data->window;
	Catalog      *catalog;
	char         *catalog_name, *catalog_path, *catalog_name_utf8;
	GList        *scan;
	GError       *gerror;

	if (data->files == NULL)
		return;

	catalog = catalog_new ();

	catalog_name_utf8 = g_strconcat (_("Search Result"),
					 CATALOG_EXT,
					 NULL);
	catalog_name = g_locale_from_utf8 (catalog_name_utf8, -1, 0, 0, 0);
	catalog_path = get_catalog_full_path (catalog_name);
	g_free (catalog_name);
	g_free (catalog_name_utf8);

	catalog_set_path (catalog, catalog_path);
	catalog_set_search_data (catalog, data->search_data);

	for (scan = data->files; scan; scan = scan->next)
		catalog_add_item (catalog, (gchar*) scan->data);

	if (! catalog_write_to_disk (catalog, &gerror)) 
		_gtk_error_dialog_from_gerror_run (GTK_WINDOW (data->dialog), &gerror);

	gtk_widget_destroy (data->search_progress_dialog);
	catalog_free (catalog);

	window_go_to_catalog (window, catalog_path);
	g_free (catalog_path);
}


/* called when the "ok" button is pressed. */
static void
save_result_cb (GtkWidget  *widget, 
		DialogData *data)
{
	GThumbWindow *window = data->window;
	char         *catalog_path;
	Catalog      *catalog;
	GList        *scan;
	GError       *gerror;

	/* save search data. */

	catalog_path = g_strdup (data->catalog_path);

	catalog = catalog_new ();
	catalog_set_path (catalog, catalog_path);
	catalog_set_search_data (catalog, data->search_data);	
	for (scan = data->files; scan; scan = scan->next)
		catalog_add_item (catalog, (gchar*) scan->data);

	if (! catalog_write_to_disk (catalog, &gerror))
		_gtk_error_dialog_from_gerror_run (GTK_WINDOW (data->dialog), &gerror);

	catalog_free (catalog);
	gtk_widget_destroy (data->search_progress_dialog);

	window_go_to_catalog (window, catalog_path);
	g_free (catalog_path);
}


static void
view_or_save_cb (GtkWidget  *widget, 
		 DialogData *data)
{
	if (data->catalog_path == NULL)
		view_result_cb (widget, data);
	else
		save_result_cb (widget, data);
}


static void
search_finished (DialogData *data)
{
	gtk_entry_set_text (GTK_ENTRY (data->p_current_dir_entry), " ");

	if (data->files == NULL)
		gtk_notebook_set_current_page (GTK_NOTEBOOK ((data)->p_notebook), 1);

	gtk_widget_set_sensitive (data->p_searching_in_hbox, FALSE);
	gtk_widget_set_sensitive (data->p_view_button, data->files != NULL);
	gtk_widget_set_sensitive (data->p_search_button, TRUE);
	gtk_widget_set_sensitive (data->p_close_button, TRUE);
}


/* called when the "cancel" button in the progress dialog is pressed. */
static void
cancel_progress_dlg_cb (GtkWidget  *widget, 
			DialogData *data)
{
	if (data->handle == NULL)
		return;
	gnome_vfs_async_cancel (data->handle);
	data->handle = NULL;
	search_finished (data);
}


/* called when the "help" button in the search dialog is pressed. */
static void
help_cb (GtkWidget  *widget, 
	 DialogData *data)
{
	GError *err;

	err = NULL;  
	gnome_help_display ("gthumb", "find", &err);
	
	if (err != NULL) {
		GtkWidget *d;
		
		d = gtk_message_dialog_new (GTK_WINDOW (data->dialog),
					    0,
					    GTK_MESSAGE_ERROR,
					    GTK_BUTTONS_CLOSE,
					    _("Could not display help: %s"),
					    err->message);
		
		g_signal_connect (G_OBJECT (d), "response",
				  G_CALLBACK (gtk_widget_destroy),
				  NULL);
		
		gtk_window_set_resizable (GTK_WINDOW (d), FALSE);
		gtk_widget_show (d);
		
		g_error_free (err);
	}
}


/* -- choose category dialog -- */


static void
update_category_entry (DialogData *data)
{
	GtkTreeIter   iter;
	GtkTreeModel *model = GTK_TREE_MODEL (data->c_categories_list_model);
	GString      *categories;

	if (! gtk_tree_model_get_iter_first (model, &iter)) {
		gtk_entry_set_text (GTK_ENTRY (data->c_categories_entry), "");
		return;
	}

	categories = g_string_new (NULL);
	do {
		gboolean use_category;
		gtk_tree_model_get (model, &iter, C_USE_CATEGORY_COLUMN, &use_category, -1);
		if (use_category) {
			char *category_name;

			gtk_tree_model_get (model, &iter, 
					    C_CATEGORY_COLUMN, &category_name, 
					    -1);
			if (categories->len > 0)
				categories = g_string_append (categories, CATEGORY_SEPARATOR_STR " ");
			categories = g_string_append (categories, category_name);
			g_free (category_name);
		}
	} while (gtk_tree_model_iter_next (model, &iter));

	gtk_entry_set_text (GTK_ENTRY (data->c_categories_entry), categories->str);
	g_string_free (categories, TRUE);
}


GList *
get_categories_from_entry (DialogData *data)
{
	GList       *cat_list = NULL;
	const char  *utf8_text;
	char       **categories;
	int          i;

	utf8_text = gtk_entry_get_text (GTK_ENTRY (data->c_categories_entry));
	if (utf8_text == NULL)
		return NULL;
	
	categories = _g_utf8_strsplit (utf8_text, CATEGORY_SEPARATOR_C);

	for (i = 0; categories[i] != NULL; i++) {
		char *s;

		s = _g_utf8_strstrip (categories[i]);

		if (s != NULL)
			cat_list = g_list_prepend (cat_list, s);
	}
	g_strfreev (categories);

	return g_list_reverse (cat_list);
}


static void
add_saved_categories (DialogData  *data,
		      GList       *cat_list)
{
	Bookmarks *categories;
	GList     *scan;

	categories = bookmarks_new (RC_CATEGORIES_FILE);
	bookmarks_load_from_disk (categories);

	for (scan = categories->list; scan; scan = scan->next) {
		GtkTreeIter  iter;
		GList       *scan2;
		gboolean     found = FALSE;
		char        *category1 = scan->data;

		for (scan2 = cat_list; scan2 && !found; scan2 = scan2->next) {
			char *category2 = scan2->data;
			if (strcmp (category1, category2) == 0)
				found = TRUE;
		}

		if (found) 
			continue;

		gtk_list_store_append (data->c_categories_list_model, &iter);

		gtk_list_store_set (data->c_categories_list_model, &iter,
				    C_USE_CATEGORY_COLUMN, FALSE,
				    C_CATEGORY_COLUMN, category1,
				    -1);
	}

	bookmarks_free (categories);
}


static void
update_list_from_entry (DialogData *data)
{
	GList *categories = NULL;
	GList *scan;

	categories = get_categories_from_entry (data);

	gtk_list_store_clear (data->c_categories_list_model); 
	for (scan = categories; scan; scan = scan->next) {
		char        *category = scan->data;
		GtkTreeIter  iter;
		
		gtk_list_store_append (data->c_categories_list_model, &iter);
		
		gtk_list_store_set (data->c_categories_list_model, &iter,
				    C_USE_CATEGORY_COLUMN, TRUE,
				    C_CATEGORY_COLUMN, category,
				    -1);
	}
	add_saved_categories (data, categories);
	path_list_free (categories);
}


static void
use_category_toggled (GtkCellRendererToggle *cell,
		      gchar                 *path_string,
		      gpointer               callback_data)
{
	DialogData   *data  = callback_data;
	GtkTreeModel *model = GTK_TREE_MODEL (data->c_categories_list_model);
	GtkTreeIter   iter;
	GtkTreePath  *path = gtk_tree_path_new_from_string (path_string);
	gboolean      value;
	
	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_model_get (model, &iter, C_USE_CATEGORY_COLUMN, &value, -1);
	
	value = !value;
	gtk_list_store_set (GTK_LIST_STORE (model), &iter, C_USE_CATEGORY_COLUMN, value, -1);
	
	gtk_tree_path_free (path);
	update_category_entry (data);
}


static void
choose_categories_cb (GtkWidget  *widget, 
		      DialogData *data)
{
	gtk_entry_set_text (GTK_ENTRY (data->c_categories_entry), gtk_entry_get_text (GTK_ENTRY (data->s_categories_entry)));
	update_list_from_entry (data);

	gtk_widget_show (data->categories_dialog);
	gtk_widget_grab_focus (data->c_categories_treeview);
}


static void
choose_categories_ok_cb (GtkWidget  *widget, 
			 DialogData *data)
{
	const char *categories;

	categories = gtk_entry_get_text (GTK_ENTRY (data->c_categories_entry));
	gtk_entry_set_text (GTK_ENTRY (data->s_categories_entry), categories);
	gtk_widget_hide (data->categories_dialog);
}


static void
choose_categories_cancel_cb (GtkWidget  *widget, 
			     DialogData *data)
{
	gtk_widget_hide (data->categories_dialog);
}


static void
date_option_changed_cb (GtkOptionMenu *option_menu,
			DialogData    *data)
{
	gtk_widget_set_sensitive (data->s_date_dateedit, gtk_option_menu_get_history (option_menu) != 0);
}


/* -- -- */


static void
response_cb (GtkWidget  *dialog,
	     int         response_id,
	     DialogData *data)
{
	switch (response_id) {
	case GTK_RESPONSE_OK:
		search_clicked_cb (NULL, data);
		break;
	case GTK_RESPONSE_CLOSE:
	default:
		gtk_widget_destroy (data->dialog);
		break;
	case GTK_RESPONSE_HELP:
		help_cb (NULL, data);
		break;
	}
}


static void
dlg_search_ui (GThumbWindow *window, 
	       char         *catalog_path, 
	       gboolean      start_search)
{
	DialogData        *data;
	GtkCellRenderer   *renderer;
	GtkTreeViewColumn *column;

	data = g_new (DialogData, 1);

	data->file_patterns = NULL;
	data->comment_patterns = NULL;
	data->place_patterns = NULL;
	data->keywords_patterns = NULL; 
	data->dirs = NULL;
	data->files = NULL;
	data->window = window;
	data->handle = NULL;
	data->search_data = NULL;
	data->uri = NULL;
	data->catalog_path = catalog_path;

	data->gui = glade_xml_new (GTHUMB_GLADEDIR "/" SEARCH_GLADE_FILE, NULL,
				   NULL);

	if (! data->gui) {
		g_warning ("Could not find " GLADE_FILE "\n");
		return;
	}

	/* Get the widgets. */

	data->dialog = glade_xml_get_widget (data->gui, "search_dialog");

	data->s_start_from_fileentry = glade_xml_get_widget (data->gui, "s_start_from_fileentry");
	data->s_start_from_entry = glade_xml_get_widget (data->gui, "s_start_from_entry");
	data->s_include_subfold_checkbutton = glade_xml_get_widget (data->gui, "s_include_subfold_checkbutton");
	data->s_filename_entry = glade_xml_get_widget (data->gui, "s_filename_entry");
	data->s_comment_entry = glade_xml_get_widget (data->gui, "s_comment_entry");
	data->s_place_entry = glade_xml_get_widget (data->gui, "s_place_entry");
	data->s_categories_entry = glade_xml_get_widget (data->gui, "s_categories_entry");
	data->s_at_least_one_cat_radiobutton = glade_xml_get_widget (data->gui, "s_at_least_one_cat_radiobutton");
	data->s_all_cat_radiobutton = glade_xml_get_widget (data->gui, "s_all_cat_radiobutton");

	data->s_choose_categories_button = glade_xml_get_widget (data->gui, "s_choose_categories_button");
	data->s_date_optionmenu = glade_xml_get_widget (data->gui, "s_date_optionmenu");
	data->s_date_dateedit = glade_xml_get_widget (data->gui, "s_date_dateedit");

	if (catalog_path == NULL) {
		data->search_progress_dialog = glade_xml_get_widget (data->gui, "search_progress_dialog");
		data->p_progress_tree_view = glade_xml_get_widget (data->gui, "p_progress_treeview");
		data->p_current_dir_entry = glade_xml_get_widget (data->gui, "p_current_dir_entry");
		data->p_notebook = glade_xml_get_widget (data->gui, "p_notebook");
		data->p_view_button = glade_xml_get_widget (data->gui, "p_view_button");
		data->p_search_button = glade_xml_get_widget (data->gui, "p_search_button");
		data->p_cancel_button = glade_xml_get_widget (data->gui, "p_cancel_button");
		data->p_close_button = glade_xml_get_widget (data->gui, "p_close_button");
		data->p_searching_in_hbox = glade_xml_get_widget (data->gui, "p_searching_in_hbox");
		data->p_images_label = glade_xml_get_widget (data->gui, "p_images_label");
	} else {
		data->search_progress_dialog = glade_xml_get_widget (data->gui, "edit_search_progress_dialog");
		data->p_progress_tree_view = glade_xml_get_widget (data->gui, "ep_progress_treeview");
		data->p_current_dir_entry = glade_xml_get_widget (data->gui, "ep_current_dir_entry");
		data->p_notebook = glade_xml_get_widget (data->gui, "ep_notebook");
		data->p_view_button = glade_xml_get_widget (data->gui, "ep_view_button");
		data->p_search_button = glade_xml_get_widget (data->gui, "ep_search_button");
		data->p_cancel_button = glade_xml_get_widget (data->gui, "ep_cancel_button");
		data->p_close_button = glade_xml_get_widget (data->gui, "ep_close_button");
		data->p_searching_in_hbox = glade_xml_get_widget (data->gui, "ep_searching_in_hbox");
		data->p_images_label = glade_xml_get_widget (data->gui, "ep_images_label");
	}

	data->categories_dialog = glade_xml_get_widget (data->gui, "categories_dialog");
	data->c_categories_entry = glade_xml_get_widget (data->gui, "c_categories_entry");
	data->c_categories_treeview = glade_xml_get_widget (data->gui, "c_categories_treeview");
	data->c_ok_button = glade_xml_get_widget (data->gui, "c_ok_button");
	data->c_cancel_button = glade_xml_get_widget (data->gui, "c_cancel_button");

	/* Set widgets data. */

	if (catalog_path == NULL) {
		if (data->window->dir_list->path != NULL)
			_gtk_entry_set_locale_text (GTK_ENTRY (data->s_start_from_entry), data->window->dir_list->path);
		else
			_gtk_entry_set_locale_text (GTK_ENTRY (data->s_start_from_entry), g_get_home_dir ());
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->s_include_subfold_checkbutton), eel_gconf_get_boolean (PREF_SEARCH_RECURSIVE));

	} else {
		Catalog    *catalog;
		SearchData *search_data;

		catalog = catalog_new ();
		catalog_load_from_disk (catalog, data->catalog_path, NULL);

		search_data = catalog->search_data;

		_gtk_entry_set_locale_text (GTK_ENTRY (data->s_start_from_entry), search_data->start_from);
	
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->s_include_subfold_checkbutton), search_data->recursive);
		
		gtk_entry_set_text (GTK_ENTRY (data->s_filename_entry),
				    search_data->file_pattern);
		gtk_entry_set_text (GTK_ENTRY (data->s_comment_entry),
				    search_data->comment_pattern);
		gtk_entry_set_text (GTK_ENTRY (data->s_place_entry),
				    search_data->place_pattern);
		gtk_entry_set_text (GTK_ENTRY (data->s_categories_entry),
				    search_data->keywords_pattern);

		gtk_option_menu_set_history (GTK_OPTION_MENU (data->s_date_optionmenu),
					     search_data->date_scope);
		gnome_date_edit_set_time (GNOME_DATE_EDIT (data->s_date_dateedit),
					  search_data->date);

		if (search_data->all_keywords)
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->s_all_cat_radiobutton), TRUE);
		else
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->s_at_least_one_cat_radiobutton), TRUE);

		catalog_free (catalog);
	}

	/**/

	data->p_progress_tree_model = gtk_list_store_new (P_NUM_COLUMNS, 
							  G_TYPE_STRING,
							  G_TYPE_STRING);
	gtk_tree_view_set_model (GTK_TREE_VIEW (data->p_progress_tree_view),
				 GTK_TREE_MODEL (data->p_progress_tree_model));
	g_object_unref (G_OBJECT (data->p_progress_tree_model));

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Filename"),
							   renderer,
							   "text", P_FILENAME_COLUMN,
							   NULL);
	gtk_tree_view_column_set_sort_column_id (column, P_FILENAME_COLUMN);
	gtk_tree_view_append_column (GTK_TREE_VIEW (data->p_progress_tree_view),
				     column);

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Folder"),
							   renderer,
							   "text", P_FOLDER_COLUMN,
							   NULL);
	gtk_tree_view_column_set_sort_column_id (column, P_FOLDER_COLUMN);
	gtk_tree_view_append_column (GTK_TREE_VIEW (data->p_progress_tree_view),
				     column);

	/**/

	data->c_categories_list_model = gtk_list_store_new (C_NUM_COLUMNS,
							    G_TYPE_BOOLEAN, 
							    G_TYPE_STRING);
	gtk_tree_view_set_model (GTK_TREE_VIEW (data->c_categories_treeview),
				 GTK_TREE_MODEL (data->c_categories_list_model));
	g_object_unref (data->c_categories_list_model);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (data->c_categories_treeview), FALSE);

	renderer = gtk_cell_renderer_toggle_new ();
	g_signal_connect (G_OBJECT (renderer), 
			  "toggled",
			  G_CALLBACK (use_category_toggled), 
			  data);
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (data->c_categories_treeview),
						     -1, "Use",
						     renderer,
						     "active", C_USE_CATEGORY_COLUMN,
						     NULL);

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("",
							   renderer,
							   "text", C_CATEGORY_COLUMN,
							   NULL);

	gtk_tree_view_column_set_sort_column_id (column, 0);
	gtk_tree_view_append_column (GTK_TREE_VIEW (data->c_categories_treeview),
				     column);

	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (data->c_categories_list_model), C_CATEGORY_COLUMN, GTK_SORT_ASCENDING);

	gtk_entry_set_text (GTK_ENTRY (data->c_categories_entry), gtk_entry_get_text (GTK_ENTRY (data->s_categories_entry)));
	update_list_from_entry (data);

	gtk_widget_set_sensitive (data->s_date_dateedit, gtk_option_menu_get_history (GTK_OPTION_MENU (data->s_date_optionmenu)) != 0);

	/* Set the signals handlers. */
	
	g_signal_connect (G_OBJECT (data->dialog),
			  "destroy",
			  G_CALLBACK (destroy_cb),
			  data);
	g_signal_connect (G_OBJECT (data->search_progress_dialog), 
			  "destroy",
			  G_CALLBACK (destroy_progress_cb),
			  data);
	g_signal_connect (G_OBJECT (data->p_search_button), 
			  "clicked",
			  G_CALLBACK (new_search_clicked_cb),
			  data);
	g_signal_connect_swapped (G_OBJECT (data->p_close_button), 
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  G_OBJECT (data->search_progress_dialog));
	g_signal_connect (G_OBJECT (data->p_cancel_button), 
			  "clicked",
			  G_CALLBACK (cancel_progress_dlg_cb),
			  data);
	g_signal_connect (G_OBJECT (data->p_view_button), 
			  "clicked",
			  G_CALLBACK (view_or_save_cb),
			  data);
	g_signal_connect (G_OBJECT (data->s_choose_categories_button), 
			  "clicked",
			  G_CALLBACK (choose_categories_cb),
			  data);
	g_signal_connect (G_OBJECT (data->c_ok_button), 
			  "clicked",
			  G_CALLBACK (choose_categories_ok_cb),
			  data);
	g_signal_connect (G_OBJECT (data->c_cancel_button), 
			  "clicked",
			  G_CALLBACK (choose_categories_cancel_cb),
			  data);
	g_signal_connect (G_OBJECT (data->s_date_optionmenu), 
			  "changed",
			  G_CALLBACK (date_option_changed_cb),
			  data);

	g_signal_connect (G_OBJECT (data->dialog), 
			  "response",
			  G_CALLBACK (response_cb),
			  data);

	/* Run dialog. */

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), 
				      GTK_WINDOW (window->app));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), TRUE);
	gtk_window_set_transient_for (GTK_WINDOW (data->search_progress_dialog), GTK_WINDOW (window->app));

	gtk_widget_grab_focus (data->s_filename_entry);

	if (! start_search) 
		gtk_widget_show (data->dialog);
	else 
		search_clicked_cb (NULL, data);
}


/* --------------------------- */


static gboolean
pattern_matched_by_keywords (char  *pattern,
			     char **keywords)
{
	int i;

	if (pattern == NULL)
		return TRUE;

	if ((keywords == NULL) || (keywords[0] == NULL))
		return FALSE;

	for (i = 0; keywords[i] != NULL; i++) 
		if (g_utf8_fnmatch (pattern, keywords[i], FNM_CASEFOLD) == 0)
			return TRUE;

	return FALSE;
}


static gboolean
match_patterns (char       **patterns, 
		const char  *string)
{
	int i;

	if (patterns[0] == NULL)
		return TRUE;

	if (string == NULL)
		return FALSE;


	for (i = 0; patterns[i] != NULL; i++) 
		if (g_utf8_fnmatch (patterns[i], string, FNM_CASEFOLD) == 0)
			return TRUE;

	return FALSE;
}


static gboolean
file_respects_search_criteria (DialogData *data, 
			       char       *filename)
{
	CommentData *comment_data;
	gboolean     result;
	gboolean     match_keywords;
	gboolean     match_date;
	int          i;
	char        *comment;
	char        *place;
	int          keywords_n;
	time_t       time;
	const char  *name_only;

	if (! file_is_image (filename, eel_gconf_get_boolean (PREF_FAST_FILE_TYPE)))
		return FALSE;

	comment_data = comments_load_comment (filename);

	if (comment_data == NULL) {
		comment = NULL;
		place = NULL;
		keywords_n = 0;
		time = 0;
	} else {
		comment = comment_data->comment;
		place = comment_data->place;
		keywords_n = comment_data->keywords_n;
		time = comment_data->time;
	}

	match_keywords = data->keywords_patterns[0] == NULL;
	for (i = 0; data->keywords_patterns[i] != NULL; i++) {
		if (comment_data == NULL)
			break;

		match_keywords = pattern_matched_by_keywords (data->keywords_patterns[i], comment_data->keywords);

		if (match_keywords && ! data->search_data->all_keywords) 
			break;
		else if (! match_keywords && data->search_data->all_keywords)
			break;
	}

	match_date = FALSE;
	if (data->search_data->date_scope == DATE_ANY)
		match_date = TRUE;
	else if ((data->search_data->date_scope == DATE_BEFORE) 
		 && (time != 0)
		 && (time < data->search_data->date))
		match_date = TRUE;
	else if ((data->search_data->date_scope == DATE_EQUAL_TO) 
		 && (time != 0)
		 && (time == data->search_data->date))
		match_date = TRUE;
	else if ((data->search_data->date_scope == DATE_AFTER) 
		 && (time != 0)
		 && (time > data->search_data->date))
		match_date = TRUE;

	name_only = file_name_from_path (filename);

	result = (match_patterns (data->file_patterns, name_only)
		  && match_patterns (data->comment_patterns, comment)
		  && match_patterns (data->place_patterns, place)
		  && match_keywords
		  && match_date);

	comment_data_free (comment_data);

	return result;
}


static void
add_file_list (DialogData *data, GList *file_list)
{
	GList *scan;
	char  *images;

	for (scan = file_list; scan; scan = scan->next) {
		GtkTreeIter  iter;
		char        *path = (char*) scan->data;
		const char  *filename;
		char        *folder;
		char        *utf8_file;
		char        *utf8_folder;

		filename = file_name_from_path (path);
		folder = remove_level_from_path (path);

		utf8_file = g_locale_to_utf8 (filename, -1, NULL, NULL, NULL);
		utf8_folder = g_locale_to_utf8 (folder, -1, NULL, NULL, NULL);
		
		gtk_list_store_append (data->p_progress_tree_model, &iter);
		gtk_list_store_set (data->p_progress_tree_model, 
				    &iter,
				    P_FILENAME_COLUMN, utf8_file,
				    P_FOLDER_COLUMN, utf8_folder,
				    -1);

		g_free (utf8_file);
		g_free (utf8_folder);
		g_free (folder);
	}

	data->files = g_list_concat (data->files, file_list);

	images = g_strdup_printf ("%d", g_list_length (data->files));
	gtk_label_set_text (GTK_LABEL (data->p_images_label), images);
	g_free (images);
}


static char *
uri_from_comment_uri (const char *comment_uri,
		      gboolean    is_file)
{
	char *base;
	char *uri;
	int   comment_uri_l;
	int   base_l;

	if (comment_uri == NULL)
		return NULL;

	comment_uri_l = strlen (comment_uri);

	base = comments_get_comment_dir (NULL, TRUE);
	base_l = strlen (base);

	if (comment_uri_l == base_l) 
		uri = g_strdup ("/");

	else if (comment_uri_l > base_l) {
		uri = g_strdup (comment_uri + base_l);

		if (is_file) { /* remove the comment extension. */
			int uri_l = comment_uri_l - base_l;
			uri [uri_l - strlen (COMMENT_EXT)] = 0;
		}

	} else
		uri = NULL;
	
	g_free (base);

	return uri;
}


static void search_dir_async (DialogData *data, gchar *dir);


static gboolean
cache_dir (const char *folder)
{
	if (folder == NULL)
		return FALSE;

	if (strcmp (folder, ".nautilus") == 0)
		return TRUE;

	if (strcmp (folder, ".thumbnails") == 0)
		return TRUE;

	if (strcmp (folder, ".xvpics") == 0)
		return TRUE;

	return FALSE;
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

			if (data->search_comments) {
				char *tmp;
				tmp = uri_from_comment_uri (unesc_uri, TRUE);
				g_free (unesc_uri);
				unesc_uri = tmp;
			}

			if (file_respects_search_criteria (data, unesc_uri)) 
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

			if (data->search_comments) {
				char *tmp;
				tmp = uri_from_comment_uri (unesc_uri, FALSE);
				g_free (unesc_uri);
				unesc_uri = tmp;
			}

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
		add_file_list (data, files);

	if (result == GNOME_VFS_ERROR_EOF) {
		gboolean good_dir_to_search_into = TRUE;

		if (! data->search_data->recursive) {
			search_finished (data);
			return;
		}
		
		do {
			GList *first_dir;
			char  *dir;

			if (data->dirs == NULL) {
				search_finished (data);
				return;
			}

			first_dir = data->dirs;
			data->dirs = g_list_remove_link (data->dirs, first_dir);
			dir = (char*) first_dir->data; 
			g_list_free (first_dir);

			good_dir_to_search_into = ! cache_dir (file_name_from_path (dir));
			if (good_dir_to_search_into)
				search_dir_async (data, dir);
			g_free (dir);
		} while (! good_dir_to_search_into);

	} else if (result != GNOME_VFS_OK) {
		char *path;

		path = gnome_vfs_uri_to_string (data->uri, 
						GNOME_VFS_URI_HIDE_NONE);
		g_warning ("Cannot load directory \"%s\": %s\n", path,
			   gnome_vfs_result_to_string (result));
		g_free (path);

		search_finished (data);
	}
}


static void 
search_dir_async (DialogData *data, char *dir)
{
	char *escaped;
	char *start_from;

	if (data->search_comments) 
		start_from = comments_get_comment_dir (dir, TRUE);
	else
		start_from = g_strdup (dir);

	_gtk_entry_set_locale_text (GTK_ENTRY (data->p_current_dir_entry), dir);

	/**/

	if (data->uri != NULL)
		gnome_vfs_uri_unref (data->uri);

	escaped = gnome_vfs_escape_path_string (start_from);
	data->uri = gnome_vfs_uri_new (escaped);

	g_free (start_from);
	g_free (escaped);

	gnome_vfs_async_load_directory_uri (
		& (data->handle),
		data->uri,
		GNOME_VFS_FILE_INFO_FOLLOW_LINKS,
		32 /* items_per_notification FIXME */,
		GNOME_VFS_PRIORITY_DEFAULT,
		directory_load_cb,
		data);
}


static gboolean
empty_pattern (const char *pattern)
{
	return (pattern == NULL) || (pattern[0] == 0);
}


static void 
search_images_async (DialogData *data)
{
	SearchData *search_data = data->search_data;

	free_search_results_data (data);
	gtk_list_store_clear (data->p_progress_tree_model);

	/* if the search criteria include comment data just search in
	 * the comments tree */

	data->search_comments = ! (empty_pattern (search_data->comment_pattern)
				   && empty_pattern (search_data->place_pattern)
				   && empty_pattern (search_data->keywords_pattern)
				   && (search_data->date_scope == DATE_ANY));
	
	search_dir_async (data, search_data->start_from);
}
