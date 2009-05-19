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
#include "gth-utils.h"
#include "gth-window.h"
#include "gtk-utils.h"
#include "gtkcellrendererthreestates.h"
#include "gth-file-view.h"
#include "comments.h"
#include "dlg-tags.h"
#include "gth-sort-utils.h"

typedef void (*SaveFunc) (GList *file_list, gpointer data);


#define DIALOG_NAME "tags"
#define GLADE_FILE "gthumb_comments.glade"
#define TAG_SEPARATOR ";"


enum {
	IS_EDITABLE_COLUMN,
	HAS_THIRD_STATE_COLUMN,
	USE_TAG_COLUMN,
	TAG_COLUMN,
	NUM_COLUMNS
};


typedef struct {
	GladeXML      *gui;
	GList         *file_list;
	GList         *default_tags_list;
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
		gth_window_set_tags_dlg (data->window, NULL);

	g_object_unref (data->gui);
	free_dialog_data (data);
	path_list_free (data->default_tags_list);

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
update_tag_entry (DialogData *data)
{
	GtkTreeIter   iter;
	GtkTreeModel *model = GTK_TREE_MODEL (data->keywords_list_model);
	GString      *tags;

	if (! gtk_tree_model_get_iter_first (model, &iter)) {
		gtk_entry_set_text (GTK_ENTRY (data->keyword_entry), "");
		return;
	}

	tags = g_string_new (NULL);
	do {
		guint use_tag;
		gtk_tree_model_get (model, &iter, USE_TAG_COLUMN, &use_tag, -1);
		if (use_tag == 1) {
			char *utf8_name;

			gtk_tree_model_get (model, &iter,
					    TAG_COLUMN, &utf8_name,
					    -1);

			if (tags->len > 0)
				tags = g_string_append (tags, TAG_SEPARATOR " ");
			tags = g_string_append (tags, utf8_name);

			g_free (utf8_name);
		}
	} while (gtk_tree_model_iter_next (model, &iter));

	gtk_entry_set_text (GTK_ENTRY (data->keyword_entry), tags->str);
	g_string_free (tags, TRUE);
}


/* if state_to_get == -1 get all tags. */
static GList *
get_tags (GtkListStore *store,
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
				    USE_TAG_COLUMN, &state,
				    TAG_COLUMN, &utf8_name,
				    -1);
		if ((state_to_get == -1) || (state == state_to_get))
			list = g_list_prepend (list, utf8_name);
		else
			g_free (utf8_name);

	} while (gtk_tree_model_iter_next (model, &iter));

	return g_list_reverse (list);
}


static gboolean
tag_present (DialogData *data,
             char       *tag)
{
	GList    *all_tags;
	GList    *scan;
	gboolean  present = FALSE;

	all_tags = get_tags (data->keywords_list_model, -1);
	for (scan = all_tags; scan; scan = scan->next) {
		char *tag2 = scan->data;
		if (strcmp (tag2, tag) == 0)
			present = TRUE;
	}
	path_list_free (all_tags);

	return present;
}


static gboolean
tag_name_is_valid (const char *name)
{
	gboolean valid = TRUE;
	if (strchr (name, ',') != NULL)
		valid = FALSE;
	return valid;
}


/* called when the "add tag" button is pressed. */
static void
add_tag_cb (GtkWidget  *widget,
            DialogData *data)
{
	GtkTreeIter  iter;
	char        *new_tag;

	new_tag = _gtk_request_dialog_run (GTK_WINDOW (data->dialog),
						GTK_DIALOG_MODAL,
						_("Enter the new tag name"),
						"",
						1024,
						GTK_STOCK_CANCEL,
						_("C_reate"));

	if (new_tag == NULL)
		return;

	if (! tag_name_is_valid (new_tag)) {
		_gtk_error_dialog_run (GTK_WINDOW (data->dialog),
				       _("The name \"%s\" is not valid because it contains the character \",\". " "Please use a different name."),
				       new_tag);

	} else if (tag_present (data, new_tag)) {
		_gtk_error_dialog_run (GTK_WINDOW (data->dialog),
				       _("The tag \"%s\" is already present. Please use a different name."),
				       new_tag);

	} else {
		gtk_list_store_append (data->keywords_list_model, &iter);
		gtk_list_store_set (data->keywords_list_model, &iter,
				    IS_EDITABLE_COLUMN, TRUE,
				    USE_TAG_COLUMN, 1,
				    TAG_COLUMN, new_tag,
				    -1);
		update_tag_entry (data);
	}

	g_free (new_tag);
}


/* called when the "remove tag" button is pressed. */
static void
remove_tag_cb (GtkWidget *widget,
		    DialogData *data)
{
	GtkTreeSelection *selection;
	GtkTreeIter       iter;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (data->keywords_list_view));
	if (! gtk_tree_selection_get_selected (selection, NULL, &iter))
		return;

	gtk_list_store_remove (data->keywords_list_model, &iter);
	update_tag_entry (data);
}


static void
save_tags (DialogData *data)
{
	Bookmarks *tags;
	GList     *scan;
	GList     *cat_list;

	cat_list = get_tags (data->keywords_list_model, -1);
	tags = bookmarks_new (RC_TAGS_FILE);
	bookmarks_set_max_lines (tags, -1);

	for (scan = cat_list; scan; scan = scan->next) {
		char *tag = scan->data;
		bookmarks_add (tags, tag, TRUE, TRUE);
	}

	bookmarks_write_to_disk (tags);

	path_list_free (cat_list);
	bookmarks_free (tags);
}


/* called when the "cancel" button is pressed. */
static void
cancel_clicked_cb (GtkWidget  *widget,
		   DialogData *data)
{
	save_tags (data);
	gtk_widget_destroy (data->dialog);
}


/* called when the "ok" button is pressed. */
static void
ok_clicked_cb (GtkWidget  *widget,
	       DialogData *data)
{
	save_tags (data);

	if (data->add_list != NULL) {
		if (*(data->add_list) != NULL)
			path_list_free (*(data->add_list));
		*(data->add_list) = get_tags (data->keywords_list_model, 1);
	}

	if (data->remove_list != NULL) {
		if (*(data->remove_list) != NULL)
			path_list_free (*(data->remove_list));
		*(data->remove_list) = get_tags (data->keywords_list_model, 0);
	}

	if (data->save_func) {
		(*data->save_func) (data->file_list, data->save_data);
		if (data->window != NULL) {
			gth_window_update_current_image_metadata (data->window);
			dlg_tags_close (data->dialog);
		}
	} else
		gtk_widget_destroy (data->dialog);
}


/* called when the "help" button in the search dialog is pressed. */
static void
help_cb (GtkWidget  *widget,
	 DialogData *data)
{
	gthumb_display_help (GTK_WINDOW (data->dialog), "gthumb-tags");
}


static void
use_tag_toggled (GtkCellRendererThreeStates *cell,
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
	gtk_list_store_set (GTK_LIST_STORE (model), &iter, USE_TAG_COLUMN, value, -1);

	gtk_tree_path_free (path);
	update_tag_entry (data);
}


static void
add_saved_tags (DialogData *data)
{
	Bookmarks *tags;
	GList     *scan;
	GList     *cat_list;

	cat_list = get_tags (data->keywords_list_model, -1);
	tags = bookmarks_new (RC_TAGS_FILE);
	bookmarks_load_from_disk (tags);

	for (scan = tags->list; scan; scan = scan->next) {
		GtkTreeIter  iter;
		GList       *scan2;
		gboolean     found = FALSE;
		char        *tag1 = scan->data;

		for (scan2 = cat_list; scan2 && !found; scan2 = scan2->next) {
			char *tag2 = scan2->data;
			if (strcmp (tag1, tag2) == 0)
				found = TRUE;
		}

		if (found)
			continue;

		gtk_list_store_append (data->keywords_list_model, &iter);

		gtk_list_store_set (data->keywords_list_model, &iter,
				    IS_EDITABLE_COLUMN, FALSE,
				    USE_TAG_COLUMN, 0,
				    TAG_COLUMN, tag1,
				    -1);
	}

	bookmarks_free (tags);
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
        char *tag1, *tag2;
	int   result;

        gtk_tree_model_get (model, a, TAG_COLUMN, &tag1, -1);
        gtk_tree_model_get (model, b, TAG_COLUMN, &tag2, -1);

	result = g_utf8_collate (tag1, tag2);

	g_free (tag1);
	g_free (tag2);

        return result;
}


static gboolean
keyword_equal_func (GtkTreeModel *model,
		    gint          column,
		    const gchar  *key,
		    GtkTreeIter  *iter,
		    gpointer      search_data)
{
	char *cell;
	gtk_tree_model_get (model, iter, column, &cell, -1);
	return case_insens_utf8_cmp (key, cell) > 0;
}


static GtkWidget*
dlg_tags_common (GtkWindow     *parent,
                 GthWindow     *window,
                 GList         *file_list,
                 GList         *default_tags_list,
                 GList        **add_tags_list,
                 GList        **remove_tags_list,
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
	data->default_tags_list = path_list_dup (default_tags_list);
	data->add_list = add_tags_list;
	data->remove_list = remove_tags_list;
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

	data->dialog = glade_xml_get_widget (data->gui, "tags_dialog");

	data->keyword_entry = glade_xml_get_widget (data->gui, "c_keyword_entry");
	data->add_key_button = glade_xml_get_widget (data->gui, "c_add_key_button");
	data->remove_key_button = glade_xml_get_widget (data->gui, "c_remove_key_button");
	data->keywords_list_view = glade_xml_get_widget (data->gui, "c_keywords_treeview");

	data->c_ok_button = btn_ok = glade_xml_get_widget (data->gui, "c_ok_button");
	btn_cancel = glade_xml_get_widget (data->gui, "c_cancel_button");
	btn_help = glade_xml_get_widget (data->gui, "c_help_button");

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

	gtk_tree_view_set_search_column (GTK_TREE_VIEW (data->keywords_list_view),
					 TAG_COLUMN);
	gtk_tree_view_set_search_equal_func (GTK_TREE_VIEW (data->keywords_list_view),
					     keyword_equal_func,
					     NULL,
					     NULL);

	column = gtk_tree_view_column_new ();

	renderer = gtk_cell_renderer_three_states_new ();
	g_signal_connect (G_OBJECT (renderer),
			  "toggled",
			  G_CALLBACK (use_tag_toggled),
			  data);
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_attributes (column, renderer,
					     "state", USE_TAG_COLUMN,
					     "has_third_state", HAS_THIRD_STATE_COLUMN,
					     NULL);

	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_attributes (column, renderer,
					     "text", TAG_COLUMN,
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
			  G_CALLBACK (add_tag_cb),
			  data);
	g_signal_connect (G_OBJECT (data->remove_key_button),
			  "clicked",
			  G_CALLBACK (remove_tag_cb),
			  data);

	/* run dialog. */

	if (parent != NULL)
		gtk_window_set_transient_for (GTK_WINDOW (data->dialog), parent);

	gtk_window_set_modal (GTK_WINDOW (data->dialog), modal);
	pref_util_restore_window_geometry (GTK_WINDOW (data->dialog), DIALOG_NAME);
	dlg_tags_update (data->dialog);

	gtk_widget_grab_focus (data->keywords_list_view);

	return data->dialog;
}


/**/


void
dlg_choose_tags (GtkWindow     *parent,
                 GList         *file_list,
                 GList         *default_tags_list,
                 GList        **add_tags_list,
                 GList        **remove_tags_list,
                 DoneFunc       done_func,
                 gpointer       done_data)
{
	dlg_tags_common (parent,
                         NULL,
                         file_list,
                         default_tags_list,
                         add_tags_list,
                         remove_tags_list,
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
} DlgTagsData;


static void
dlg_tags__done (gpointer data)
{
	DlgTagsData *dcdata = data;

	path_list_free (dcdata->add_list);
	dcdata->add_list = NULL;

	path_list_free (dcdata->remove_list);
	dcdata->remove_list = NULL;

	g_free (dcdata);
}


static void
dlg_tags__save (GList    *file_list,
                gpointer  data)
{
	DlgTagsData *dcdata = data;
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

		comments_save_tags (filename, cdata);
		comment_data_free (cdata);
	}

	path_list_free (dcdata->add_list);
	dcdata->add_list = NULL;

	path_list_free (dcdata->remove_list);
	dcdata->remove_list = NULL;
}


GtkWidget*
dlg_tags_new (GthWindow *window)
{
	GtkWidget         *current_dlg;
	DlgTagsData *dcdata;

	current_dlg = gth_window_get_tags_dlg (window);
	if (current_dlg != NULL) {
		gtk_window_present (GTK_WINDOW (current_dlg));
		return current_dlg;
	}

	dcdata = g_new0 (DlgTagsData, 1);
	dcdata->window = window;
	dcdata->add_list = NULL;
	dcdata->remove_list = NULL;

	return dlg_tags_common (GTK_WINDOW (window),
                                window,
                                NULL,
                                NULL,
                                &(dcdata->add_list),
                                &(dcdata->remove_list),
                                dlg_tags__save,
                                dcdata,
                                dlg_tags__done,
                                dcdata,
                                FALSE);
}


void
dlg_tags_update (GtkWidget *dlg)
{
	DialogData    *data;
	CommentData   *cdata = NULL;
	GList         *scan;
	GList         *other_keys = NULL;
        GSList        *tmp1, *tmp2;


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

		/* remove a tag if it is not in all comments. */
		for (scan = data->file_list->next; scan; scan = scan->next) {
			CommentData *scan_cdata;

			scan_cdata = comments_load_comment (scan->data, TRUE);

			if (scan_cdata == NULL) {
				comment_data_free_keywords (cdata);
				break;
			}

			for (tmp1 = cdata->keywords; tmp1; tmp1 = g_slist_next (tmp1)) {
				char     *k1 = tmp1->data;
				gboolean  found = FALSE;

				for (tmp2 =  scan_cdata->keywords; tmp2; tmp2 = g_slist_next (tmp2)) {
					char *k2 = tmp2->data;
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

		scan_cdata = comments_load_comment (scan->data, TRUE);

		if (scan_cdata == NULL)
			continue;

                for (tmp2 =  scan_cdata->keywords; tmp2; tmp2 = g_slist_next (tmp2)) {
                        char *k2 = tmp2->data;
			gboolean  found = FALSE;

			if (cdata != NULL)
                                for (tmp1 = cdata->keywords; tmp1; tmp1 = g_slist_next (tmp1)) {
                                        char     *k1 = tmp1->data;
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
                for (tmp1 = cdata->keywords; tmp1; tmp1 = g_slist_next (tmp1)) {
			GtkTreeIter  iter;

			gtk_list_store_append (data->keywords_list_model,
					       &iter);

			gtk_list_store_set (data->keywords_list_model, &iter,
					    IS_EDITABLE_COLUMN, FALSE,
					    HAS_THIRD_STATE_COLUMN, FALSE,
					    USE_TAG_COLUMN, 1,
					    TAG_COLUMN, tmp1->data,
					    -1);
		}
	}

	for (scan = data->default_tags_list; scan; scan = scan->next) {
		char        *keyword = scan->data;
		GtkTreeIter  iter;

		gtk_list_store_append (data->keywords_list_model,
				       &iter);

		gtk_list_store_set (data->keywords_list_model, &iter,
				    IS_EDITABLE_COLUMN, FALSE,
				    HAS_THIRD_STATE_COLUMN, FALSE,
				    USE_TAG_COLUMN, 1,
				    TAG_COLUMN, keyword,
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
				    USE_TAG_COLUMN, 2,
				    TAG_COLUMN, keyword,
				    -1);
	}

	if (other_keys != NULL)
		path_list_free (other_keys);

	add_saved_tags (data);
	update_tag_entry (data);
}


void
dlg_tags_close  (GtkWidget *dlg)
{
	DialogData *data;

	g_return_if_fail (dlg != NULL);

	data = g_object_get_data (G_OBJECT (dlg), "dialog_data");
	g_return_if_fail (data != NULL);

	gtk_widget_destroy (dlg);
}
