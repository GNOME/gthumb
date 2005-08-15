/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001, 2003, 2004 Free Software Foundation, Inc.
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

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <libgnomeui/gnome-help.h>
#include <glade/glade.h>

#include "typedefs.h"
#include "file-utils.h"
#include "preferences.h"
#include "main.h"
#include "gth-window.h"
#include "gtk-utils.h"
#include "gtkcellrendererthreestates.h"
#include "gth-file-view.h"
#include "comments.h"
#include "dlg-categories.h"


typedef void (*SaveFunc) (GList *file_list, gpointer data);


#define DIALOG_NAME "categories"
#define GLADE_FILE "gthumb_comments.glade"
#define CATEGORY_SEPARATOR ";"


enum {
	IS_EDITABLE_COLUMN,
	HAS_THIRD_STATE_COLUMN,
	USE_CATEGORY_COLUMN,
	CATEGORY_COLUMN,
	NUM_COLUMNS
};


typedef struct {
	GladeXML      *gui;
	GList         *file_list;
	GList         *default_categories_list;
	GList        **add_list;
	GList        **remove_list;
	SaveFunc       save_func;
	gpointer       save_data;
	DoneFunc       done_func;
	gpointer       done_data;

	GthWindow     *window;
	GtkWidget     *dialog;

	GtkWidget     *c_ok_button;
	GtkWidget     *keyword_entry;
	GtkWidget     *add_key_button;
	GtkWidget     *remove_key_button;
	GtkWidget     *keywords_list_view;
	GtkListStore  *keywords_list_model;

	CommentData   *original_cdata;
} DialogData;



static void
free_dialog_data (DialogData *data)
{
	if (data->file_list != NULL) {
		g_list_foreach (data->file_list, (GFunc) g_free, NULL);
		g_list_free (data->file_list);
		data->file_list = NULL;
	}
	
	if (data->original_cdata != NULL) {
		comment_data_free (data->original_cdata);
		data->original_cdata = NULL;
	}
}


/* called when the main dialog is closed. */
static void
destroy_cb (GtkWidget  *widget, 
	    DialogData *data)
{
	if (data->window != NULL)
		gth_window_set_categories_dlg (data->window, NULL);

	g_object_unref (data->gui);
	free_dialog_data (data);
	path_list_free (data->default_categories_list);

	if (data->done_func)
		(*data->done_func) (data->done_data);

	g_free (data);
}


static gboolean
unrealize_cb (GtkWidget  *widget, 
	  DialogData *data)
{
	pref_util_save_window_geometry (GTK_WINDOW (widget), DIALOG_NAME);
	return FALSE;
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

			gtk_tree_model_get (model, &iter, 
					    CATEGORY_COLUMN, &utf8_name, 
					    -1);

			if (categories->len > 0)
				categories = g_string_append (categories, CATEGORY_SEPARATOR " ");
			categories = g_string_append (categories, utf8_name);

			g_free (utf8_name);
		}
	} while (gtk_tree_model_iter_next (model, &iter));

	gtk_entry_set_text (GTK_ENTRY (data->keyword_entry), categories->str);
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
		if ((state_to_get == -1) || (state == state_to_get)) 
			list = g_list_prepend (list, utf8_name);
		else
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


static gboolean
category_name_is_valid (const char *name)
{
	gboolean valid = TRUE;
	if (strchr (name, ',') != NULL)
		valid = FALSE;
	return valid;
}


/* called when the "add category" button is pressed. */
static void
add_category_cb (GtkWidget  *widget, 
		 DialogData *data)
{
	GtkTreeIter  iter;
	char        *new_category;

	new_category = _gtk_request_dialog_run (GTK_WINDOW (data->dialog),
						GTK_DIALOG_MODAL,
						_("Enter the new category name"),
						"",
						1024,
						GTK_STOCK_CANCEL,
						_("C_reate"));

	if (new_category == NULL)
		return;

	if (! category_name_is_valid (new_category)) {
		_gtk_error_dialog_run (GTK_WINDOW (data->dialog),
				       _("The name \"%s\" is not valid because it contains the character \",\". " "Please use a different name."),
				       new_category);

	} else if (category_present (data, new_category)) {
		_gtk_error_dialog_run (GTK_WINDOW (data->dialog),
				       _("The category \"%s\" is already present. Please use a different name."), 
				       new_category);

	} else {
		gtk_list_store_append (data->keywords_list_model, &iter);
		gtk_list_store_set (data->keywords_list_model, &iter,
				    IS_EDITABLE_COLUMN, TRUE,
				    USE_CATEGORY_COLUMN, 1,
				    CATEGORY_COLUMN, new_category,
				    -1);
		update_category_entry (data);
		gtk_widget_set_sensitive (data->c_ok_button, TRUE);
	}

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
	gtk_widget_set_sensitive (data->c_ok_button, TRUE);
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


/* called when the "cancel" button is pressed. */
static void
cancel_clicked_cb (GtkWidget  *widget, 
		   DialogData *data)
{
	save_categories (data);
	gtk_widget_destroy (data->dialog);
}


/* called when the "ok" button is pressed. */
static void
ok_clicked_cb (GtkWidget  *widget, 
	       DialogData *data)
{
	save_categories (data);

	if (data->add_list != NULL) {
		if (*(data->add_list) != NULL)
			path_list_free (*(data->add_list));
		*(data->add_list) = get_categories (data->keywords_list_model, 1);
	}

	if (data->remove_list != NULL) {
		if (*(data->remove_list) != NULL)
			path_list_free (*(data->remove_list));
		*(data->remove_list) = get_categories (data->keywords_list_model, 0);
	}

	if (data->save_func) {
		(*data->save_func) (data->file_list, data->save_data);
		gtk_widget_set_sensitive (data->c_ok_button, FALSE);
		if (data->window != NULL)
			gth_window_reload_current_image (data->window);
	} else
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
	gtk_widget_set_sensitive (data->c_ok_button, TRUE);
}


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

		for (scan2 = cat_list; scan2 && !found; scan2 = scan2->next) {
			char *category2 = scan2->data;
			if (strcmp (category1, category2) == 0)
				found = TRUE;
		}

		if (found) 
			continue;

		gtk_list_store_append (data->keywords_list_model, &iter);

		gtk_list_store_set (data->keywords_list_model, &iter,
				    IS_EDITABLE_COLUMN, FALSE,
				    USE_CATEGORY_COLUMN, 0,
				    CATEGORY_COLUMN, category1,
				    -1);
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


static int
name_column_sort_func (GtkTreeModel *model, 
                       GtkTreeIter  *a, 
                       GtkTreeIter  *b, 
                       gpointer      user_data)
{
        char *category1, *category2;
	int   result;

        gtk_tree_model_get (model, a, CATEGORY_COLUMN, &category1, -1);
        gtk_tree_model_get (model, b, CATEGORY_COLUMN, &category2, -1);

	result = g_utf8_collate (category1, category2);

	g_free (category1);
	g_free (category2);

        return result;
}





static GtkWidget*
dlg_categories_common (GtkWindow     *parent,
		       GthWindow     *window,
		       GList         *file_list,
		       GList         *default_categories_list,
		       GList        **add_categories_list,
		       GList        **remove_categories_list,
		       SaveFunc       save_func,
		       gpointer       save_data,
		       DoneFunc       done_func,
		       gpointer       done_data,
		       gboolean       modal)
{
	DialogData        *data;
	GtkWidget         *btn_ok;
	GtkWidget         *btn_cancel;
	GtkWidget         *btn_help;
	GtkCellRenderer   *renderer;
	GtkTreeViewColumn *column;


	data = g_new (DialogData, 1);

	data->window = window;
	data->default_categories_list = path_list_dup (default_categories_list);
	data->add_list = add_categories_list;
	data->remove_list = remove_categories_list;
	data->save_func = save_func;
	data->save_data = save_data;
	data->done_func = done_func;
	data->done_data = done_data;
	data->original_cdata = NULL;

	data->gui = glade_xml_new (GTHUMB_GLADEDIR "/" GLADE_FILE , NULL, NULL);
        if (!data->gui) {
		g_free (data);
                g_warning ("Could not find " GLADE_FILE "\n");
                return NULL;
        }

	if (file_list != NULL)
		data->file_list = path_list_dup (file_list);
	else
		data->file_list = NULL;

	/* Get the widgets. */

	data->dialog = glade_xml_get_widget (data->gui, "categories_dialog");

	data->keyword_entry = glade_xml_get_widget (data->gui, "c_keyword_entry");
	data->add_key_button = glade_xml_get_widget (data->gui, "c_add_key_button");
	data->remove_key_button = glade_xml_get_widget (data->gui, "c_remove_key_button");
	data->keywords_list_view = glade_xml_get_widget (data->gui, "c_keywords_treeview");

	data->c_ok_button = btn_ok = glade_xml_get_widget (data->gui, "c_ok_button");
	btn_cancel = glade_xml_get_widget (data->gui, "c_cancel_button");
	btn_help = glade_xml_get_widget (data->gui, "c_help_button");

	if (!modal) {
		gtk_button_set_label (GTK_BUTTON (btn_ok), GTK_STOCK_SAVE);
		gtk_button_set_label (GTK_BUTTON (btn_cancel), GTK_STOCK_CLOSE);
	}

	/* Set widgets data. */

	g_object_set_data (G_OBJECT (data->dialog), "dialog_data", data);

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

	gtk_tree_view_column_set_sort_column_id (column, 0);
	gtk_tree_view_append_column (GTK_TREE_VIEW (data->keywords_list_view),
				     column);

	gtk_tree_sortable_set_default_sort_func (GTK_TREE_SORTABLE (data->keywords_list_model), name_column_sort_func, NULL, NULL);

	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (data->keywords_list_model), GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID, GTK_SORT_ASCENDING);

	/* Set the signals handlers. */
	
	g_signal_connect (G_OBJECT (data->dialog), 
			  "destroy",
			  G_CALLBACK (destroy_cb),
			  data);
	g_signal_connect (G_OBJECT (data->dialog), 
			  "unrealize",
			  G_CALLBACK (unrealize_cb),
			  data);
	g_signal_connect (G_OBJECT (btn_ok), 
			  "clicked",
			  G_CALLBACK (ok_clicked_cb),
			  data);
	g_signal_connect (G_OBJECT (btn_cancel), 
			  "clicked",
			  G_CALLBACK (cancel_clicked_cb),
			  data);
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

	if (parent != NULL)
		gtk_window_set_transient_for (GTK_WINDOW (data->dialog), parent);

	gtk_window_set_modal (GTK_WINDOW (data->dialog), modal);
	pref_util_restore_window_geometry (GTK_WINDOW (data->dialog), DIALOG_NAME);
	dlg_categories_update (data->dialog);

	gtk_widget_grab_focus (data->keywords_list_view);

	return data->dialog;
}


/**/


void
dlg_choose_categories (GtkWindow     *parent,
		       GList         *file_list,
		       GList         *default_categories_list,
		       GList        **add_categories_list,
		       GList        **remove_categories_list,
		       DoneFunc       done_func,
		       gpointer       done_data)
{
	dlg_categories_common (parent,
			       NULL,
			       file_list,
			       default_categories_list,
			       add_categories_list,
			       remove_categories_list,
			       NULL,
			       NULL,
			       done_func,
			       done_data,
			       TRUE);
}


/**/


typedef struct {
	GthWindow     *window;
	GList         *add_list;
	GList         *remove_list;
} DlgCategoriesData;


static void
dlg_categories__done (gpointer data)
{
	DlgCategoriesData *dcdata = data;

	path_list_free (dcdata->add_list);
	dcdata->add_list = NULL;
	
	path_list_free (dcdata->remove_list);
	dcdata->remove_list = NULL;

	g_free (dcdata);
}


static void
dlg_categories__save (GList    *file_list,
		      gpointer  data)
{
	DlgCategoriesData *dcdata = data;
	GList             *scan;
	
	for (scan = file_list; scan; scan = scan->next) {
		const char  *filename = scan->data;
		CommentData *cdata;
		GList       *scan2;

		cdata = comments_load_comment (filename, TRUE);
		if (cdata == NULL)
			cdata = comment_data_new ();
		else 
			for (scan2 = dcdata->remove_list; scan2; scan2 = scan2->next) {
				const char *k = scan2->data;
				comment_data_remove_keyword (cdata, k);
			}

		for (scan2 = dcdata->add_list; scan2; scan2 = scan2->next) {
			const char *k = scan2->data;
			comment_data_add_keyword (cdata, k);
		}

		comments_save_categories (filename, cdata);
		comment_data_free (cdata);
	}

	path_list_free (dcdata->add_list);
	dcdata->add_list = NULL;

	path_list_free (dcdata->remove_list);
	dcdata->remove_list = NULL;
}


GtkWidget*
dlg_categories_new (GthWindow *window)
{
	GtkWidget         *current_dlg;
	DlgCategoriesData *dcdata;

	current_dlg = gth_window_get_categories_dlg (window);
	if (current_dlg != NULL) {
		gtk_window_present (GTK_WINDOW (current_dlg));
		return current_dlg;
	}

	dcdata = g_new0 (DlgCategoriesData, 1);
	dcdata->window = window;
	dcdata->add_list = NULL;
	dcdata->remove_list = NULL;

	return dlg_categories_common (GTK_WINDOW (window),
				      window,
				      NULL,
				      NULL,
				      &(dcdata->add_list),
				      &(dcdata->remove_list),
				      dlg_categories__save,
				      dcdata,
				      dlg_categories__done,
				      dcdata,
				      FALSE);
}


void
dlg_categories_update (GtkWidget *dlg)
{
	DialogData    *data;
	CommentData   *cdata = NULL;
	GList         *scan;
	GList         *other_keys = NULL;


	g_return_if_fail (dlg != NULL);

	data = g_object_get_data (G_OBJECT (dlg), "dialog_data");
	g_return_if_fail (data != NULL);

	gtk_list_store_clear (data->keywords_list_model);

	/**/

	if (data->window != NULL) {
		free_dialog_data (data);
		data->file_list = gth_window_get_file_list_selection (data->window);
	} else if (data->original_cdata != NULL) {
		comment_data_free (data->original_cdata);
		data->original_cdata = NULL;
	}

	if (data->file_list != NULL) {
		char *first_image = data->file_list->data;
		data->original_cdata = cdata = comments_load_comment (first_image, TRUE);
	}
	
	if (cdata != NULL) {
		comment_data_free_comment (cdata);

		/* remove a category if it is not in all comments. */
		for (scan = data->file_list->next; scan; scan = scan->next) {
			CommentData *scan_cdata;
			int          i;

			scan_cdata = comments_load_comment (scan->data, TRUE);
		
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

		scan_cdata = comments_load_comment (scan->data, TRUE);
		
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

			gtk_list_store_append (data->keywords_list_model, 
					       &iter);

			gtk_list_store_set (data->keywords_list_model, &iter,
					    IS_EDITABLE_COLUMN, FALSE,
					    HAS_THIRD_STATE_COLUMN, FALSE,
					    USE_CATEGORY_COLUMN, 1,
					    CATEGORY_COLUMN, cdata->keywords[i],
					    -1);
		}
	}

	for (scan = data->default_categories_list; scan; scan = scan->next) {
		char        *keyword = scan->data;
		GtkTreeIter  iter;
		
		gtk_list_store_append (data->keywords_list_model, 
				       &iter);
		
		gtk_list_store_set (data->keywords_list_model, &iter,
				    IS_EDITABLE_COLUMN, FALSE,
				    HAS_THIRD_STATE_COLUMN, FALSE,
				    USE_CATEGORY_COLUMN, 1,
				    CATEGORY_COLUMN, keyword,
				    -1);
	}

	for (scan = other_keys; scan; scan = scan->next) {
		char        *keyword = scan->data;
		GtkTreeIter  iter;
		
		gtk_list_store_append (data->keywords_list_model, 
				       &iter);
		
		gtk_list_store_set (data->keywords_list_model, &iter,
				    IS_EDITABLE_COLUMN, FALSE,
				    HAS_THIRD_STATE_COLUMN, TRUE,
				    USE_CATEGORY_COLUMN, 2,
				    CATEGORY_COLUMN, keyword,
				    -1);
	}

	if (other_keys != NULL)
		path_list_free (other_keys);

	add_saved_categories (data);
	update_category_entry (data);
	gtk_widget_set_sensitive (data->c_ok_button, FALSE);
}


void
dlg_categories_close  (GtkWidget *dlg)
{
	DialogData *data;

	g_return_if_fail (dlg != NULL);

	data = g_object_get_data (G_OBJECT (dlg), "dialog_data");
	g_return_if_fail (data != NULL);

	gtk_widget_destroy (dlg);
}
