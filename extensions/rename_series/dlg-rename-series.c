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
#include <gtk/gtk.h>
#include <gthumb.h>
#include "dlg-rename-series.h"
#include "gth-rename-task.h"


#define GET_WIDGET(name) _gtk_builder_get_widget (data->builder, (name))


enum {
	GTH_CHANGE_CASE_NONE = 0,
	GTH_CHANGE_CASE_LOWER,
	GTH_CHANGE_CASE_UPPER,
};

enum {
	SORT_DATA_COLUMN,
	SORT_NAME_COLUMN,
	SORT_NUM_COLUMNS
};

enum {
	PREVIEW_OLD_NAME_COLUMN,
	PREVIEW_NEW_NAME_COLUMN,
	PREVIEW_NUM_COLUMNS
};


typedef struct {
	GthBrowser    *browser;
	GList         *file_list;
	GList         *new_file_list;
	GList         *new_names_list;
	gboolean       single_file;
	GtkBuilder    *builder;
	GtkWidget     *dialog;
	GtkWidget     *list_view;
	GtkWidget     *sort_combobox;
	GtkWidget     *change_case_combobox;
	GtkListStore  *list_store;
	GtkListStore  *sort_model;
	gboolean       help_visible;
} DialogData;


static void
destroy_cb (GtkWidget  *widget,
	    DialogData *data)
{
	gth_browser_set_dialog (data->browser, "rename_series", NULL);

	g_object_unref (data->builder);
	_g_object_list_unref (data->file_list);
	_g_string_list_free (data->new_names_list);
	g_list_free (data->new_file_list);
	g_free (data);
}


static void
ok_clicked_cb (GtkWidget  *widget,
	       DialogData *data)
{
	GList   *old_files;
	GList   *new_files;
	GList   *scan1;
	GList   *scan2;
	GthTask *task;

	old_files = NULL;
	new_files = NULL;

	for (scan1 = data->new_file_list, scan2 = data->new_names_list;
	     scan1 && scan2;
	     scan1 = scan1->next, scan2 = scan2->next)
	{
		GthFileData *file_data = scan1->data;
		char        *new_name  = scan2->data;
		GFile       *parent;
		GFile       *new_file;

		parent = g_file_get_parent (file_data->file);
		new_file = g_file_get_child (parent, new_name);

		old_files = g_list_prepend (old_files, g_object_ref (file_data->file));
		new_files = g_list_prepend (new_files, new_file);

		g_object_unref (parent);
	}
	old_files = g_list_reverse (old_files);
	new_files = g_list_reverse (new_files);

	task = gth_rename_task_new (old_files, new_files);
	gth_browser_exec_task (data->browser, task, FALSE);

	g_object_unref (task);
	gtk_widget_destroy (data->dialog);
}


typedef struct {
	const char   *template;
	GthFileData  *file_data;
	int           n;
	GError      **error;
} TemplateData;


static char *
get_original_enum (GthFileData *file_data,
	           char        *match)
{
	char    *basename;
	GRegex  *re;
	char   **a;
	char    *value = NULL;

	basename = g_file_get_basename (file_data->file);
	re = g_regex_new ("([0-9]+)", 0, 0, NULL);
	a = g_regex_split (re, basename, 0);
	if (g_strv_length (a) >= 2)
		value = g_strdup (g_strstrip (a[1]));

	g_strfreev (a);
	g_regex_unref (re);
	g_free (basename);

	return value;
}


static char *
get_attribute_value (GthFileData *file_data,
	             char        *match)
{
	GRegex    *re;
	char     **a;
	char      *attribute = NULL;
	char      *value = NULL;

	re = g_regex_new ("%attr\\{([^}]+)\\}", 0, 0, NULL);
	a = g_regex_split (re, match, 0);
	if (g_strv_length (a) >= 2)
		attribute = g_strstrip (a[1]);

	if ((attribute != NULL) && (*attribute != '\0')) {
		value = gth_file_data_get_attribute_as_string (file_data, attribute);
		if (value != NULL) {
			char *tmp_value;

			tmp_value = _g_utf8_replace (value, "[\r\n]", " ");
			g_free (value);
			value = tmp_value;
		}
	}

	g_strfreev (a);
	g_regex_unref (re);

	return value;
}


static gboolean
template_eval_cb (const GMatchInfo *info,
		  GString          *res,
		  gpointer          data)
{
	TemplateData *template_data = data;
	char         *r = NULL;
	char         *match;

	match = g_match_info_fetch (info, 0);
	if (strcmp (match, "%F") == 0) {
		r = g_file_get_basename (template_data->file_data->file);
	}
	else if (strcmp (match, "%E") == 0) {
		char *uri;

		uri = g_file_get_uri (template_data->file_data->file);
		r = g_strdup (_g_uri_get_file_extension (uri));

		g_free (uri);
	}
	else if (strcmp (match, "%N") == 0) {
		r = get_original_enum (template_data->file_data, match);
	}
	else if (strncmp (match, "%attr", 5) == 0) {
		r = get_attribute_value (template_data->file_data, match);
		/*if (r == NULL)
			*template_data->error = g_error_new_literal (GTH_TASK_ERROR, GTH_TASK_ERROR_FAILED, _("Malformed template"));*/
	}
	else if (strncmp (match, "#", 1) == 0) {
		char *format;

		format = g_strdup_printf ("%%0%" G_GSIZE_FORMAT "d", strlen (match));
		r = g_strdup_printf (format, template_data->n);

		g_free (format);
	}

	if (r != NULL)
		g_string_append (res, r);

	g_free (r);
	g_free (match);

	return FALSE;
}


static void
dlg_rename_series_update_preview (DialogData *data)
{
	GtkTreeIter   iter;
	int           change_case;
	TemplateData *template_data;
	GRegex       *re;
	GList        *scan;
	GError       *error = NULL;
	GList        *scan1, *scan2;

	if (data->new_names_list != NULL) {
		_g_string_list_free (data->new_names_list);
		data->new_names_list = NULL;
	}

	if (data->new_file_list != NULL) {
		g_list_free (data->new_file_list);
		data->new_file_list = NULL;
	}

	data->new_file_list = g_list_copy (data->file_list);
	if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (data->sort_combobox), &iter)) {
		GthFileDataSort *sort_type;

		gtk_tree_model_get (GTK_TREE_MODEL (data->sort_model),
				    &iter,
				    SORT_DATA_COLUMN, &sort_type,
				    -1);

		if (sort_type->cmp_func != NULL)
			data->new_file_list = g_list_sort (data->new_file_list, (GCompareFunc) sort_type->cmp_func);
	}
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("reverse_order_checkbutton"))))
		data->new_file_list = g_list_reverse (data->new_file_list);

	change_case = gtk_combo_box_get_active (GTK_COMBO_BOX (data->change_case_combobox));

	template_data = g_new0 (TemplateData, 1);
	template_data->error = &error;
	template_data->n = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (GET_WIDGET ("start_at_spinbutton")));
	template_data->template = gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("template_entry")));
	re = g_regex_new ("#+|%F|%E|%N|%attr\\{[^}]+\\}", 0, 0, NULL);
	for (scan = data->new_file_list; scan; scan = scan->next) {
		char *new_name;
		char *new_name2;

		template_data->file_data = scan->data;
		new_name = g_regex_replace_eval (re, template_data->template, -1, 0, 0, template_eval_cb, template_data, &error);
		if (error != NULL)
			break;

		switch (change_case) {
		case GTH_CHANGE_CASE_LOWER:
			new_name2 = g_utf8_strdown (new_name, -1);
			break;
		case GTH_CHANGE_CASE_UPPER:
			new_name2 = g_utf8_strup (new_name, -1);
			break;
		default:
			new_name2 = g_strdup (new_name);
			break;
		}

		data->new_names_list = g_list_prepend (data->new_names_list, new_name2);
		template_data->n = template_data->n + 1;

		g_free (new_name);
	}
	data->new_names_list = g_list_reverse (data->new_names_list);

	if (error != NULL) {
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (data->dialog), _("Could not rename the files"), &error);
		return;
	}

	/* -- update the list view -- */

	gtk_list_store_clear (data->list_store);
	for (scan1 = data->new_file_list, scan2 = data->new_names_list;
	     scan1 && scan2;
	     scan1 = scan1->next, scan2 = scan2->next)
	{
		GthFileData *file_data1 = scan1->data;
		GthFileData *new_name = scan2->data;
		GtkTreeIter  iter;

		gtk_list_store_append (data->list_store, &iter);
		gtk_list_store_set (data->list_store, &iter,
				    PREVIEW_OLD_NAME_COLUMN, g_file_info_get_display_name (file_data1->info),
				    PREVIEW_NEW_NAME_COLUMN, new_name,
				    -1);
	}
}


static void
template_entry_icon_press_cb (GtkEntry             *entry,
                              GtkEntryIconPosition  icon_pos,
                              GdkEvent             *event,
                              gpointer              user_data)
{
	DialogData *data = user_data;

	data->help_visible = ! data->help_visible;

	if (data->help_visible)
		gtk_widget_show (GET_WIDGET("template_help_table"));
	else
		gtk_widget_hide (GET_WIDGET("template_help_table"));
}


static void
update_preview_cb (GtkWidget  *widget,
		   DialogData *data)
{
	dlg_rename_series_update_preview (data);
}


void
dlg_rename_series (GthBrowser *browser,
		   GList      *file_list)
{
	DialogData        *data;
	GtkCellRenderer   *renderer;
	GtkTreeViewColumn *column;
	int                i;
	GList             *scan;

	if (gth_browser_get_dialog (browser, "rename_series") != NULL) {
		gtk_window_present (GTK_WINDOW (gth_browser_get_dialog (browser, "rename_series")));
		return;
	}

	data = g_new0 (DialogData, 1);
	data->browser = browser;
	data->builder = _gtk_builder_new_from_file ("rename-series.ui", "rename_series");
	data->file_list = gth_file_data_list_dup (file_list);

	/* Get the widgets. */

	data->dialog = _gtk_builder_get_widget (data->builder, "rename_series_dialog");
	gth_browser_set_dialog (browser, "rename_series", data->dialog);
	g_object_set_data (G_OBJECT (data->dialog), "dialog_data", data);

	/* Set widgets data. */

	data->list_store = gtk_list_store_new (PREVIEW_NUM_COLUMNS,
					       G_TYPE_STRING,
					       G_TYPE_STRING);
	data->list_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (data->list_store));
	g_object_unref (data->list_store);

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Old Name"),
							   renderer,
							   "text", PREVIEW_OLD_NAME_COLUMN,
							   NULL);
	gtk_tree_view_column_set_expand (GTK_TREE_VIEW_COLUMN (column), TRUE);
	gtk_tree_view_append_column (GTK_TREE_VIEW (data->list_view), column);

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("New Name"),
							   renderer,
							   "text", PREVIEW_NEW_NAME_COLUMN,
							   NULL);
	gtk_tree_view_column_set_expand (GTK_TREE_VIEW_COLUMN (column), TRUE);
	gtk_tree_view_append_column (GTK_TREE_VIEW (data->list_view), column);

	gtk_widget_show (data->list_view);
	gtk_container_add (GTK_CONTAINER (GET_WIDGET ("preview_scrolledwindow")), data->list_view);

	if (data->file_list->next == NULL) {
		GthFileData *file_data = data->file_list->data;
		gtk_entry_set_text (GTK_ENTRY (GET_WIDGET ("template_entry")), g_file_info_get_attribute_string (file_data->info, G_FILE_ATTRIBUTE_STANDARD_EDIT_NAME));
	}
	else
		gtk_entry_set_text (GTK_ENTRY (GET_WIDGET ("template_entry")), "####%E");

	gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("start_at_spinbutton")), 1.0);

	/* sort by */

	data->sort_model = gtk_list_store_new (SORT_NUM_COLUMNS,
					       G_TYPE_POINTER,
					       G_TYPE_STRING);
	data->sort_combobox = gtk_combo_box_new_with_model (GTK_TREE_MODEL (data->sort_model));
	g_object_unref (data->sort_model);
	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (data->sort_combobox), renderer, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (data->sort_combobox),
					renderer,
					"text", SORT_NAME_COLUMN,
					NULL);

	for (i = 0, scan = gth_main_get_all_sort_types (); scan; scan = scan->next, i++) {
		GthFileDataSort *sort_type = scan->data;
		GtkTreeIter      iter;

		gtk_list_store_append (data->sort_model, &iter);
		gtk_list_store_set (data->sort_model, &iter,
				    SORT_DATA_COLUMN, sort_type,
				    SORT_NAME_COLUMN, sort_type->display_name,
				    -1);
	}
	gtk_combo_box_set_active (GTK_COMBO_BOX (data->sort_combobox), 0);
	gtk_widget_show (data->sort_combobox);
	gtk_container_add (GTK_CONTAINER (GET_WIDGET ("sort_by_box")), data->sort_combobox);

	/* change case */

	data->change_case_combobox = _gtk_combo_box_new_with_texts (_("Keep original case"),
								    _("Convert to lower-case"),
								    _("Convert to upper-case"),
								    NULL);
	gtk_combo_box_set_active (GTK_COMBO_BOX (data->change_case_combobox), 0);
	gtk_widget_show (data->change_case_combobox);
	gtk_container_add (GTK_CONTAINER (GET_WIDGET ("change_case_box")), data->change_case_combobox);

	dlg_rename_series_update_preview (data);

	/* Set the signals handlers. */

	g_signal_connect (G_OBJECT (data->dialog),
			  "destroy",
			  G_CALLBACK (destroy_cb),
			  data);
	g_signal_connect (GET_WIDGET ("ok_button"),
			  "clicked",
			  G_CALLBACK (ok_clicked_cb),
			  data);
	g_signal_connect_swapped (GET_WIDGET ("cancel_button"),
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  G_OBJECT (data->dialog));
	g_signal_connect (GET_WIDGET ("template_entry"),
  			  "icon-press",
  			  G_CALLBACK (template_entry_icon_press_cb),
  			  data);
	g_signal_connect (GET_WIDGET ("template_entry"),
			  "changed",
			  G_CALLBACK (update_preview_cb),
			  data);
	g_signal_connect (GET_WIDGET ("start_at_spinbutton"),
			  "value_changed",
			  G_CALLBACK (update_preview_cb),
			  data);
	g_signal_connect (data->sort_combobox,
			  "changed",
			  G_CALLBACK (update_preview_cb),
			  data);
	g_signal_connect (data->change_case_combobox,
                          "changed",
                          G_CALLBACK (update_preview_cb),
                          data);
	g_signal_connect (GET_WIDGET ("reverse_order_checkbutton"),
			  "toggled",
			  G_CALLBACK (update_preview_cb),
			  data);

	/* Run dialog. */

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), GTK_WINDOW (browser));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), FALSE);
	gtk_widget_show (data->dialog);

	if (data->file_list->next == NULL) {
		const char *edit_name;
		const char *end_pos;

		edit_name = gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("template_entry")));
		end_pos = g_utf8_strrchr (edit_name, -1, '.');
		if (end_pos != NULL) {
			glong nchars;

			nchars = g_utf8_strlen (edit_name, (gssize) (end_pos - edit_name));
			gtk_editable_select_region (GTK_EDITABLE (GET_WIDGET ("template_entry")), 0, nchars);
		}
	}
}
