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
#include <gdk/gdkkeysyms.h>
#include <libgnome/libgnome.h>
#include <libgnomeui/gnome-dialog.h>
#include <libgnomeui/gnome-dialog-util.h>
#include <libgnomeui/gnome-dateedit.h>
#include <glade/glade.h>
#include "typedefs.h"
#include "main.h"
#include "gthumb-window.h"
#include "gtk-utils.h"
#include "gtkcellrendererthreestates.h"
#include "image-list.h"
#include "image-list-utils.h"
#include "comments.h"


#define COMMENT_GLADE_FILE "gthumb_comments.glade"
#define CATEGORY_SEPARATOR ";"


enum {
	IS_EDITABLE_COLUMN,
	HAS_THIRD_STATE_COLUMN,
	USE_CATEGORY_COLUMN,
	CATEGORY_COLUMN,
	NUM_COLUMNS
};


typedef struct {
	GThumbWindow  *window;
	GladeXML      *gui;
	GList         *file_list;

	GtkWidget     *dialog;

	GtkWidget     *keyword_entry;
	GtkWidget     *add_key_button;
	GtkWidget     *remove_key_button;
	GtkWidget     *keywords_list_view;
	GtkListStore  *keywords_list_model;

	CommentData   *original_cdata;
} DialogData;


/* called when the main dialog is closed. */
static void
destroy_cb (GtkWidget *widget, 
	    DialogData *data)
{
	data->window->categories_dlg = NULL;

	g_object_unref (data->gui);
	g_list_foreach (data->file_list, (GFunc) g_free, NULL);
	g_list_free (data->file_list);
	comment_data_free (data->original_cdata);

	g_free (data);
}


static void
update_category_entry (DialogData *data)
{
	GtkTreeIter   iter;
	GtkTreeModel *model = GTK_TREE_MODEL (data->keywords_list_model);
	GString      *categories;

	if (! gtk_tree_model_get_iter_first (model, &iter)) {
		gtk_entry_set_text (GTK_ENTRY (data->keyword_entry), "");
		return;
	}

	categories = g_string_new (NULL);
	do {
		guint use_category;
		gtk_tree_model_get (model, &iter, USE_CATEGORY_COLUMN, &use_category, -1);
		if (use_category == 1) {
			char *utf8_name;
			char *category_name;

			gtk_tree_model_get (model, &iter, 
					    CATEGORY_COLUMN, &utf8_name, 
					    -1);
			category_name = g_locale_from_utf8 (utf8_name, -1,
							    NULL, NULL, NULL);

			if (categories->len > 0)
				categories = g_string_append (categories, CATEGORY_SEPARATOR " ");
			categories = g_string_append (categories, category_name);
			g_free (category_name);
			g_free (utf8_name);
		}
	} while (gtk_tree_model_iter_next (model, &iter));

	_gtk_entry_set_locale_text (GTK_ENTRY (data->keyword_entry), categories->str);
	g_string_free (categories, TRUE);
}


/* if state_to_get == -1 get all categories. */
static GList *
get_categories (GtkListStore *store, 
		guint         state_to_get)
{
	GtkTreeModel *model = GTK_TREE_MODEL (store);
	GList        *list = NULL;
	GtkTreeIter   iter;
	
	if (! gtk_tree_model_get_iter_first (model, &iter))
		return NULL;

	do {
		guint  state;
		char  *utf8_name;

		gtk_tree_model_get (model, 
				    &iter, 
				    USE_CATEGORY_COLUMN, &state, 
				    CATEGORY_COLUMN, &utf8_name, 
				    -1);
		if ((state_to_get == -1) || (state == state_to_get)) {
			char *name;
			name = g_locale_from_utf8 (utf8_name, 
						   -1, 
						   NULL, 
						   NULL, 
						   NULL);
			list = g_list_prepend (list, name);
		}
		g_free (utf8_name);

	} while (gtk_tree_model_iter_next (model, &iter));

	return g_list_reverse (list);
}


static gboolean
category_present (DialogData *data,
		  char       *category)
{
	GList    *all_categories;
	GList    *scan;
	gboolean  present = FALSE;

	all_categories = get_categories (data->keywords_list_model, -1);
	for (scan = all_categories; scan; scan = scan->next) {
		char *category2 = scan->data;
		if (strcmp (category2, category) == 0) 
			present = TRUE;
	}
	path_list_free (all_categories);
	
	return present;
}


/* called when the "add category" button is pressed. */
static void
add_category_cb (GtkWidget  *widget, 
		 DialogData *data)
{
	GtkTreeIter  iter;
	char        *new_category;
	char        *utf8_name;

	new_category = _gtk_request_dialog_run (GTK_WINDOW (data->dialog),
						GTK_DIALOG_MODAL,
						_("Enter the new category name"),
						"",
						1024,
						GTK_STOCK_CANCEL,
						_("C_reate"));

	if (new_category == NULL)
		return;

	utf8_name = g_locale_to_utf8 (new_category, -1, NULL, NULL, NULL);
	
	if (category_present (data, new_category)) {
		_gtk_error_dialog_run (GTK_WINDOW (data->dialog),
				       _("The category \"%s\" is already present. Please use a different name."), 
				       utf8_name);
	} else {
		gtk_list_store_append (data->keywords_list_model, &iter);
		gtk_list_store_set (data->keywords_list_model, &iter,
				    IS_EDITABLE_COLUMN, TRUE,
				    USE_CATEGORY_COLUMN, 0,
				    CATEGORY_COLUMN, utf8_name,
				    -1);
		update_category_entry (data);
	}

	g_free (utf8_name);
	g_free (new_category);
}


/* called when the "remove category" button is pressed. */
static void
remove_category_cb (GtkWidget *widget, 
		    DialogData *data)
{
	GtkTreeSelection *selection;
	GtkTreeIter       iter;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (data->keywords_list_view));
	if (! gtk_tree_selection_get_selected (selection, NULL, &iter))
		return;

	gtk_list_store_remove (data->keywords_list_model, &iter);
	update_category_entry (data);
}


static void
save_categories (DialogData *data)
{
	Bookmarks *categories;
	GList     *scan;
	GList     *cat_list;

	cat_list = get_categories (data->keywords_list_model, -1);
	categories = bookmarks_new (RC_CATEGORIES_FILE);

	for (scan = cat_list; scan; scan = scan->next) {
		char *category = scan->data;
		bookmarks_add (categories, category, TRUE, TRUE);
	}

	bookmarks_write_to_disk (categories);

	path_list_free (cat_list);
	bookmarks_free (categories);
}


/* called when the "ok" button is pressed. */
static void
ok_clicked_cb (GtkWidget  *widget, 
	       DialogData *data)
{
	GList *add_list, *remove_list, *scan;

	save_categories (data);

	add_list = get_categories (data->keywords_list_model, 1);
	remove_list = get_categories (data->keywords_list_model, 0);

	for (scan = data->file_list; scan; scan = scan->next) {
		const char  *filename = scan->data;
		CommentData *cdata;
		GList       *scan2;

		cdata = comments_load_comment (filename);
		if (cdata == NULL)
			cdata = comment_data_new ();
		else 
			for (scan2 = remove_list; scan2; scan2 = scan2->next) {
				const char *k = scan2->data;
				comment_data_remove_keyword (cdata, k);
			}

		for (scan2 = add_list; scan2; scan2 = scan2->next) {
			const char *k = scan2->data;
			comment_data_add_keyword (cdata, k);
		}

		comments_save_categories (filename, cdata);
		comment_data_free (cdata);
	}

	path_list_free (add_list);
	path_list_free (remove_list);

	gtk_widget_destroy (data->dialog);
}


/* called when the "help" button in the search dialog is pressed. */
static void
help_cb (GtkWidget  *widget, 
	 DialogData *data)
{
	GError *err;

	err = NULL;  
	gnome_help_display ("gthumb", "comments", &err);
	
	if (err != NULL) {
		GtkWidget *dialog;
		
		dialog = gtk_message_dialog_new (GTK_WINDOW (data->dialog),
						 0,
						 GTK_MESSAGE_ERROR,
						 GTK_BUTTONS_CLOSE,
						 _("Could not display help: %s"),
						 err->message);
		
		g_signal_connect (G_OBJECT (dialog), "response",
				  G_CALLBACK (gtk_widget_destroy),
				  NULL);
		
		gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
		
		gtk_widget_show (dialog);
		
		g_error_free (err);
	}
}


static void
use_category_toggled (GtkCellRendererThreeStates *cell,
		      gchar                      *path_string,
		      gpointer                    callback_data)
{
	DialogData   *data  = callback_data;
	GtkTreeModel *model = GTK_TREE_MODEL (data->keywords_list_model);
	GtkTreeIter   iter;
	GtkTreePath  *path = gtk_tree_path_new_from_string (path_string);
	guint         value;
	
	gtk_tree_model_get_iter (model, &iter, path);
	value = gtk_cell_renderer_three_states_get_next_state (cell);
	gtk_list_store_set (GTK_LIST_STORE (model), &iter, USE_CATEGORY_COLUMN, value, -1);
	
	gtk_tree_path_free (path);
	update_category_entry (data);
}


#ifdef XXX /* FIXME : delete or fix */
static void
category_edited (GtkCellRendererText *cell,
		 gchar               *path_string,
		 gchar               *new_text,
		 gpointer             callback_data)
{
	DialogData   *data  = callback_data;
	GtkTreeModel *model = GTK_TREE_MODEL (data->keywords_list_model);
	GtkTreeIter   iter;
	GtkTreePath  *path = gtk_tree_path_new_from_string (path_string);

	if (category_present (data, new_text)) {
		
		_gtk_error_dialog_run (GTK_WINDOW (data->dialog),
		/*_*/("The category \"%s\" is already present. Please use a different name."), 
				       new_text);
	} else {
		gtk_tree_model_get_iter (model, &iter, path);
		gtk_list_store_set (GTK_LIST_STORE (model), &iter, CATEGORY_COLUMN, new_text, -1);
		update_category_entry (data);
	}

	gtk_tree_path_free (path);
}
#endif


static void
add_saved_categories (DialogData *data)
{
	Bookmarks *categories;
	GList     *scan;
	GList     *cat_list;

	cat_list = get_categories (data->keywords_list_model, -1);
	categories = bookmarks_new (RC_CATEGORIES_FILE);
	bookmarks_load_from_disk (categories);

	for (scan = categories->list; scan; scan = scan->next) {
		GtkTreeIter  iter;
		GList       *scan2;
		gboolean     found = FALSE;
		char        *category1 = scan->data;
		char        *utf8_name;

		for (scan2 = cat_list; scan2 && !found; scan2 = scan2->next) {
			char *category2 = scan2->data;
			if (strcmp (category1, category2) == 0)
				found = TRUE;
		}

		if (found) 
			continue;

		gtk_list_store_append (data->keywords_list_model, &iter);

		utf8_name = g_locale_to_utf8 (category1,
                                              -1, NULL, NULL, NULL);
		gtk_list_store_set (data->keywords_list_model, &iter,
				    IS_EDITABLE_COLUMN, FALSE /*TRUE*/,
				    USE_CATEGORY_COLUMN, 0,
				    CATEGORY_COLUMN, utf8_name,
				    -1);
		g_free (utf8_name);
	}

	bookmarks_free (categories);
	path_list_free (cat_list);
}


static gboolean
key_in_list (GList      *list, 
	     const char *key)
{
	GList *scan;

	for (scan = list; scan; scan = scan->next)
		if (strcmp (scan->data, key) == 0)
			return TRUE;
	return FALSE;
}



void
dlg_categories (GtkWidget *widget, 
		gpointer   wdata)
{
	GThumbWindow      *window = wdata;
	DialogData        *data;
	ImageList         *ilist;
	GtkWidget         *btn_ok;
	GtkWidget         *btn_cancel;
	GtkWidget         *btn_help;
	CommentData       *cdata;
	GList             *scan;
	GtkCellRenderer   *renderer;
	GtkTreeViewColumn *column;
	char              *first_image;
	GList             *other_keys = NULL;

	if (window->categories_dlg != NULL) {
		gtk_window_present (GTK_WINDOW (window->categories_dlg));
		return;
	}

	data = g_new (DialogData, 1);

	data->window = window;

	data->gui = glade_xml_new (GTHUMB_GLADEDIR "/" COMMENT_GLADE_FILE , NULL, NULL);
        if (!data->gui) {
                g_warning ("Could not find " GLADE_FILE "\n");
                return;
        }

	ilist = IMAGE_LIST (window->file_list->ilist);
	data->file_list = ilist_utils_get_file_list_selection (ilist);
	if (data->file_list == NULL) {
		g_free (data);
		return;
	}

	/* Get the widgets. */

	data->dialog = glade_xml_get_widget (data->gui, "categories_dialog");
	window->categories_dlg = data->dialog;

	data->keyword_entry = glade_xml_get_widget (data->gui, "c_keyword_entry");
	data->add_key_button = glade_xml_get_widget (data->gui, "c_add_key_button");
	data->remove_key_button = glade_xml_get_widget (data->gui, "c_remove_key_button");
	data->keywords_list_view = glade_xml_get_widget (data->gui, "c_keywords_treeview");

	btn_ok = glade_xml_get_widget (data->gui, "c_ok_button");
	btn_cancel = glade_xml_get_widget (data->gui, "c_cancel_button");
	btn_help = glade_xml_get_widget (data->gui, "c_help_button");

	/* Set widgets data. */

	data->keywords_list_model = gtk_list_store_new (NUM_COLUMNS,
							G_TYPE_BOOLEAN,
							G_TYPE_BOOLEAN,
							G_TYPE_UINT, 
							G_TYPE_STRING);
	gtk_tree_view_set_model (GTK_TREE_VIEW (data->keywords_list_view),
				 GTK_TREE_MODEL (data->keywords_list_model));
	g_object_unref (data->keywords_list_model);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (data->keywords_list_view), FALSE);

	renderer = gtk_cell_renderer_three_states_new ();
	g_signal_connect (G_OBJECT (renderer), 
			  "toggled",
			  G_CALLBACK (use_category_toggled), 
			  data);
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (data->keywords_list_view),
						     -1, "Use",
						     renderer,
						     "state", USE_CATEGORY_COLUMN,
						     "has_third_state", HAS_THIRD_STATE_COLUMN,
						     NULL);

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("",
							   renderer,
							   "text", CATEGORY_COLUMN,
							   "editable", IS_EDITABLE_COLUMN,
							   NULL);

	/*
	g_signal_connect (G_OBJECT (renderer), 
			  "edited",
			  G_CALLBACK (category_edited), 
			  data);
	*/

	gtk_tree_view_column_set_sort_column_id (column, 0);
	gtk_tree_view_append_column (GTK_TREE_VIEW (data->keywords_list_view),
				     column);

	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (data->keywords_list_model), CATEGORY_COLUMN, GTK_SORT_ASCENDING);

	/**/

	first_image = data->file_list->data;
	data->original_cdata = cdata = comments_load_comment (first_image);

	if (cdata != NULL) {
		comment_data_free_comment (cdata);

		/* remove a category if it is not in all comments. */
		for (scan = data->file_list->next; scan; scan = scan->next) {
			CommentData *scan_cdata;
			int          i;

			scan_cdata = comments_load_comment (scan->data);
		
			if (scan_cdata == NULL) {
				comment_data_free_keywords (cdata);
				break;
			}

			for (i = 0; i < cdata->keywords_n; i++) {
				char     *k1 = cdata->keywords[i];
				gboolean  found = FALSE;
				int       j;
				
				for (j = 0; j < scan_cdata->keywords_n; j++) {
					char *k2 = scan_cdata->keywords[j];
					if (strcmp (k1, k2) == 0) {
						found = TRUE;
						break;
					}
				}

				if (!found) 
					comment_data_remove_keyword (cdata, k1);
			}

			comment_data_free (scan_cdata);
		}
	}

	for (scan = data->file_list; scan; scan = scan->next) {
		CommentData *scan_cdata;
		int          j;

		scan_cdata = comments_load_comment (scan->data);
		
		if (scan_cdata == NULL) 
			continue;
				
		for (j = 0; j < scan_cdata->keywords_n; j++) {
			char     *k2 = scan_cdata->keywords[j];
			gboolean  found = FALSE;
			int       i;

			if (cdata != NULL)
				for (i = 0; i < cdata->keywords_n; i++) {
					char *k1 = cdata->keywords[i];
					if (strcmp (k1, k2) == 0) {
						found = TRUE;
						break;
					}
				}
			
			if (! found && ! key_in_list (other_keys, k2))
				other_keys = g_list_prepend (other_keys,
							     g_strdup (k2));
		}
		comment_data_free (scan_cdata);
	}
		
	if (cdata != NULL) {
		int i;	

		for (i = 0; i < cdata->keywords_n; i++) {
			GtkTreeIter  iter;
			char        *utf8_name;

			gtk_list_store_append (data->keywords_list_model, 
					       &iter);

			utf8_name = g_locale_to_utf8 (cdata->keywords[i],
						      -1, NULL, NULL, NULL);
			gtk_list_store_set (data->keywords_list_model, &iter,
					    IS_EDITABLE_COLUMN, FALSE /*TRUE*/,
					    HAS_THIRD_STATE_COLUMN, FALSE,
					    USE_CATEGORY_COLUMN, 1,
					    CATEGORY_COLUMN, utf8_name,
					    -1);
			g_free (utf8_name);
		}
	}

	for (scan = other_keys; scan; scan = scan->next) {
		char        *keyword = scan->data;
		GtkTreeIter  iter;
		char        *utf8_name;
		
		gtk_list_store_append (data->keywords_list_model, 
				       &iter);
		
		utf8_name = g_locale_to_utf8 (keyword,
					      -1, NULL, NULL, NULL);
		
		gtk_list_store_set (data->keywords_list_model, &iter,
				    IS_EDITABLE_COLUMN, FALSE /*TRUE*/,
				    HAS_THIRD_STATE_COLUMN, TRUE,
				    USE_CATEGORY_COLUMN, 2,
				    CATEGORY_COLUMN, utf8_name,
				    -1);
		g_free (utf8_name);
	}

	if (other_keys != NULL)
		path_list_free (other_keys);

	add_saved_categories (data);
	update_category_entry (data);

	/* Set the signals handlers. */
	
	g_signal_connect (G_OBJECT (data->dialog), 
			  "destroy",
			  G_CALLBACK (destroy_cb),
			  data);
	g_signal_connect (G_OBJECT (btn_ok), 
			  "clicked",
			  G_CALLBACK (ok_clicked_cb),
			  data);
	g_signal_connect_swapped (G_OBJECT (btn_cancel), 
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  G_OBJECT (data->dialog));
	g_signal_connect (G_OBJECT (btn_help), 
			  "clicked",
			  G_CALLBACK (help_cb),
			  data);

	g_signal_connect (G_OBJECT (data->add_key_button), 
			  "clicked",
			  G_CALLBACK (add_category_cb),
			  data);
	g_signal_connect (G_OBJECT (data->remove_key_button), 
			  "clicked",
			  G_CALLBACK (remove_category_cb),
			  data);

	/* run dialog. */

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), 
				      GTK_WINDOW (window->app));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), FALSE);
	gtk_widget_show (data->dialog);

	gtk_widget_grab_focus (data->keywords_list_view);
}
