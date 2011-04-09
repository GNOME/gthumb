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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <gtk/gtk.h>
#include "gconf-utils.h"
#include "glib-utils.h"
#include "gth-browser.h"
#include "gth-main.h"
#include "gth-preferences.h"
#include "gtk-utils.h"
#include "main.h"


#define GET_WIDGET(name) _gtk_builder_get_widget (data->builder, (name))
#define DEFAULT_ICON "application-x-executable"
#define EXTENSION_CATEGORY_ALL "*"
#define EXTENSION_CATEGORY_ENABLED "+"
#define EXTENSION_CATEGORY_DISABLED "-"
#define EXTENSION_CATEGORY_SEPARATOR "---"


enum {
	EXTENSION_DESCRIPTION_COLUMN,
	EXTENSION_ORIGINAL_STATUS_COLUMN,
	EXTENSION_COLUMNS
};


enum {
	CATEGORY_ID_COLUMN,
	CATEGORY_NAME_COLUMN,
	CATEGORY_ICON_COLUMN,
	CATEGORY_SEPARATOR_COLUMN,
	CATEGORY_COLUMNS
};


typedef struct {
	const char *id;
	const char *name;
	const char *icon;
} ExtensionCategory;


static ExtensionCategory extension_category[] = {
		{ EXTENSION_CATEGORY_ALL, N_("All"), "folder" },
		{ EXTENSION_CATEGORY_ENABLED, N_("Enabled"), "folder" },
		{ EXTENSION_CATEGORY_DISABLED, N_("Disabled"), "folder" },
		{ EXTENSION_CATEGORY_SEPARATOR, NULL, NULL },
		{ "Browser", N_("Browser"), "folder" },
		{ "Viewer", N_("Viewers"), "folder" },
		{ "Metadata", N_("Metadata"), "folder" },
		{ "File-Tool", N_("File tools"), "folder" },
		{ "List-Tool", N_("List tools"), "folder" },
		{ "Importer", N_("Importers"), "folder" },
		{ "Exporter", N_("Exporters"), "folder" },
		{ NULL, NULL, NULL }
};


typedef struct {
	GthBrowser   *browser;
	GtkBuilder   *builder;
	GtkWidget    *dialog;
	GtkWidget    *list_view;
	GtkListStore *list_store;
	GtkTreeModel *model_filter;
	GSList       *active_extensions;
	char         *current_category;
} DialogData;


static gboolean
list_equal (GSList *list1,
	    GSList *list2)
{
	GSList *sscan1;

	if (g_slist_length (list1) != g_slist_length (list2))
		return FALSE;

	for (sscan1 = list1; sscan1; sscan1 = sscan1->next) {
		char   *name1 = sscan1->data;
		GSList *sscan2;

		for (sscan2 = list2; sscan2; sscan2 = sscan2->next) {
			char *name2 = sscan2->data;

			if (strcmp (name1, name2) == 0)
				break;
		}

		if (sscan2 == NULL)
			return FALSE;
	}

	return TRUE;
}


/* called when the main dialog is closed. */
static void
destroy_cb (GtkWidget  *widget,
	    DialogData *data)
{
	GSList              *active_extensions;
	GthExtensionManager *manager;
	GList               *names;
	GList               *scan;

	active_extensions = NULL;
	manager = gth_main_get_default_extension_manager ();
	names = gth_extension_manager_get_extensions (manager);
	for (scan = names; scan; scan = scan->next) {
		char                    *name = scan->data;
		GthExtensionDescription *description;

		description = gth_extension_manager_get_description (manager, name);
		if ((description == NULL) || description->mandatory || description->hidden)
			continue;

		if (gth_extension_description_is_active (description))
			active_extensions = g_slist_prepend (active_extensions, g_strdup (name));
	}
	active_extensions = g_slist_reverse (active_extensions);
	eel_gconf_set_string_list (PREF_ACTIVE_EXTENSIONS, active_extensions);

	if (! list_equal (active_extensions, data->active_extensions)) {
		GtkWidget *dialog;
		int        response;

		dialog = _gtk_message_dialog_new (GTK_WINDOW (data->browser),
						  GTK_DIALOG_MODAL,
						  GTK_STOCK_DIALOG_WARNING,
						  _("Restart required"),
						  _("You need to restart gthumb for these changes to take effect"),
						  _("_Continue"), GTK_RESPONSE_CANCEL,
						  _("_Restart"), GTK_RESPONSE_OK,
						  NULL);
		response = gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);

		if (response == GTK_RESPONSE_OK)
			gth_restart ();
	}

	gth_browser_set_dialog (data->browser, "extensions", NULL);

	g_slist_foreach (active_extensions, (GFunc) g_free, NULL);
	g_slist_free (active_extensions);

	g_slist_foreach (data->active_extensions, (GFunc) g_free, NULL);
	g_slist_free (data->active_extensions);
	g_object_unref (data->builder);
	g_free (data->current_category);
	g_free (data);
}


static void
extension_description_data_func_cb (GtkTreeViewColumn *tree_column,
				    GtkCellRenderer   *cell,
				    GtkTreeModel      *tree_model,
				    GtkTreeIter       *iter,
				    gpointer           user_data)
{
	GthExtensionDescription *description;
	char                    *text;

	gtk_tree_model_get (tree_model, iter, 0, &description, -1);

	text = g_markup_printf_escaped ("<b>%s</b>\n<small>%s</small>", description->name, description->description);
	g_object_set (G_OBJECT (cell), "markup", text, NULL);
	g_object_set (G_OBJECT (cell), "sensitive", gth_extension_description_is_active (description), NULL);

	g_free (text);
	g_object_unref (description);
}


static void
extension_icon_data_func_cb (GtkTreeViewColumn *tree_column,
		 	     GtkCellRenderer   *cell,
			     GtkTreeModel      *tree_model,
			     GtkTreeIter       *iter,
			     gpointer           user_data)
{
	GthExtensionDescription *description;

	gtk_tree_model_get (tree_model, iter, 0, &description, -1);
	if (description->icon_name != NULL)
		g_object_set (G_OBJECT (cell), "icon-name", description->icon_name, NULL);
	else
		g_object_set (G_OBJECT (cell), "icon-name", DEFAULT_ICON, NULL);
	g_object_set (G_OBJECT (cell), "sensitive", gth_extension_description_is_active (description), NULL);

	g_object_unref (description);
}


static void
extension_active_data_func_cb (GtkTreeViewColumn *tree_column,
		 	       GtkCellRenderer   *cell,
			       GtkTreeModel      *tree_model,
			       GtkTreeIter       *iter,
			       gpointer           user_data)
{
	GthExtensionDescription *description;

	gtk_tree_model_get (tree_model, iter, 0, &description, -1);
	g_object_set (G_OBJECT (cell), "active", gth_extension_description_is_active (description), NULL);

	g_object_unref (description);
}


static void
cell_renderer_toggle_toggled_cb (GtkCellRendererToggle *cell_renderer,
				 char                  *path,
                                 gpointer               user_data)
{
	DialogData  *data = user_data;
	GtkTreePath *tree_path;
	GtkTreeIter  iter;

	tree_path = gtk_tree_path_new_from_string (path);
	if (tree_path == NULL)
		return;

	if (gtk_tree_model_get_iter (GTK_TREE_MODEL (data->model_filter), &iter, tree_path)) {
		GthExtensionDescription *description;
		GError                  *error = NULL;
		GtkTreeIter              child_iter;

		gtk_tree_model_get (GTK_TREE_MODEL (data->model_filter), &iter, 0, &description, -1);
		gtk_tree_model_filter_convert_iter_to_child_iter (GTK_TREE_MODEL_FILTER (data->model_filter), &child_iter, &iter);
		if (! gth_extension_description_is_active (description)) {
			if (! gth_extension_manager_activate (gth_main_get_default_extension_manager (), description->id, &error))
				_gtk_error_dialog_from_gerror_run (GTK_WINDOW (data->dialog), _("Could not activate the extension"), &error);
			else
				gtk_list_store_set (data->list_store, &child_iter, 0, description, -1);
		}
		else {
			if (! gth_extension_manager_deactivate (gth_main_get_default_extension_manager (), description->id, &error))
				_gtk_error_dialog_from_gerror_run (GTK_WINDOW (data->dialog), _("Could not deactivate the extension"), &error);
			else
				gtk_list_store_set (data->list_store, &child_iter, 0, description, -1);
		}

		g_object_unref (description);
	}

	gtk_tree_path_free (tree_path);
}


static void
add_columns (GtkTreeView *treeview,
	     DialogData  *data)
{
	GtkCellRenderer   *renderer;
	GtkTreeViewColumn *column;

	/* the checkbox column */

	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column, _("Use"));

	renderer = gtk_cell_renderer_toggle_new ();
	g_signal_connect (renderer,
			  "toggled",
			  G_CALLBACK (cell_renderer_toggle_toggled_cb),
			  data);

	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func (column, renderer, extension_active_data_func_cb, data, NULL);

	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

	/* the name column. */

	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column, _("Extension"));

	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	g_object_set (renderer, "stock-size", GTK_ICON_SIZE_BUTTON, NULL);
	gtk_tree_view_column_set_cell_data_func (column, renderer, extension_icon_data_func_cb, data, NULL);

	renderer = gtk_cell_renderer_text_new ();
        gtk_tree_view_column_pack_start (column, renderer, TRUE);
        g_object_set (renderer, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
        gtk_tree_view_column_set_cell_data_func (column, renderer, extension_description_data_func_cb, data, NULL);

        gtk_tree_view_column_set_expand (column, TRUE);
        gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
}


static int
extension_compare_func (GtkTreeModel *tree_model,
			GtkTreeIter  *iter_a,
			GtkTreeIter  *iter_b,
			gpointer      user_data)
{
	GthExtensionDescription *description_a;
	GthExtensionDescription *description_b;
	int                      result;

	gtk_tree_model_get (tree_model, iter_a, 0, &description_a, -1);
	gtk_tree_model_get (tree_model, iter_b, 0, &description_b, -1);

	result = strcmp (description_a->name, description_b->name);

	g_object_unref (description_a);
	g_object_unref (description_b);

	return result;
}


static void
list_view_selection_changed_cb (GtkTreeSelection *treeselection,
                                gpointer          user_data)
{
	DialogData              *data = user_data;
	GtkTreeModel            *model;
	GtkTreeIter              iter;
	GthExtensionDescription *description;
	GthExtension            *extension;

	model = GTK_TREE_MODEL (data->model_filter);
	if (! gtk_tree_selection_get_selected (treeselection, &model, &iter))
		return;

	gtk_tree_model_get (model, &iter, EXTENSION_DESCRIPTION_COLUMN, &description, -1);

	extension = gth_extension_description_get_extension (description);
	gtk_widget_set_sensitive (GET_WIDGET ("preferences_button"), (extension != NULL) && gth_extension_is_configurable (extension));

	g_object_unref (description);
}


static void
reset_original_extension_status (DialogData *data)
{
	GtkTreeModel *model;
	GtkTreeIter   iter;

	model = GTK_TREE_MODEL (data->list_store);
	if (! gtk_tree_model_get_iter_first (model, &iter))
		return;

	do {
		GthExtensionDescription *description;

		gtk_tree_model_get (model, &iter, EXTENSION_DESCRIPTION_COLUMN, &description, -1);
		gtk_list_store_set (GTK_LIST_STORE (model), &iter,
				    EXTENSION_ORIGINAL_STATUS_COLUMN, gth_extension_description_is_active (description),
				    -1);

		g_object_unref (description);
	}
	while (gtk_tree_model_iter_next (model, &iter));
}


static void
category_combobox_changed_cb (GtkComboBox *combo_box,
			      gpointer     user_data)
{
	DialogData  *data = user_data;
	GtkTreeIter  iter;

	if (! gtk_combo_box_get_active_iter (combo_box, &iter))
		return;

	reset_original_extension_status (data);

	g_free (data->current_category);
	gtk_tree_model_get (GTK_TREE_MODEL (GET_WIDGET ("category_liststore")),
			    &iter,
			    CATEGORY_ID_COLUMN, &data->current_category,
			    -1);
	gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (data->model_filter));
}


static void
about_button_clicked_cb (GtkButton *button,
			 gpointer   user_data)
{
	DialogData              *data = user_data;
	GtkTreeModel            *model;
	GtkTreeIter              iter;
	GthExtensionDescription *description;
	GtkWidget               *dialog;

	model = GTK_TREE_MODEL (data->model_filter);
	if (! gtk_tree_selection_get_selected (gtk_tree_view_get_selection (GTK_TREE_VIEW (data->list_view)), &model, &iter))
		return;

	gtk_tree_model_get (model, &iter, 0, &description, -1);

	dialog = gtk_about_dialog_new ();
	if (description->name != NULL)
		g_object_set (dialog, "program-name", description->name, NULL);
	if (description->description != NULL)
		g_object_set (dialog, "comments", description->description, NULL);
	if (description->version != NULL)
		g_object_set (dialog, "version", description->version, NULL);
	if (description->authors != NULL)
		g_object_set (dialog, "authors", description->authors, NULL);
	if (description->copyright != NULL)
		g_object_set (dialog, "copyright", description->copyright, NULL);
	if (description->icon_name != NULL)
		g_object_set (dialog, "logo-icon-name", description->icon_name, NULL);
	else
		g_object_set (dialog, "logo-icon-name", DEFAULT_ICON, NULL);
	if (description->url != NULL)
		g_object_set (dialog, "website", description->url, NULL);

	gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (data->dialog));
	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);

	g_object_unref (description);
}


static void
preferences_button_clicked_cb (GtkButton *button,
			       gpointer   user_data)
{
	DialogData              *data = user_data;
	GtkTreeModel            *model;
	GtkTreeIter              iter;
	GthExtensionDescription *description;
	GthExtension            *extension;

	model = GTK_TREE_MODEL (data->model_filter);
	if (! gtk_tree_selection_get_selected (gtk_tree_view_get_selection (GTK_TREE_VIEW (data->list_view)), &model, &iter))
		return;

	gtk_tree_model_get (model, &iter, 0, &description, -1);
	extension = gth_extension_description_get_extension (description);
	gth_extension_configure (extension, GTK_WINDOW (data->dialog));

	g_object_unref (description);
}


static gboolean
category_view_separator_func (GtkTreeModel *model,
			      GtkTreeIter  *iter,
			      gpointer      data)
{
	gboolean separator;

	gtk_tree_model_get (model, iter, CATEGORY_SEPARATOR_COLUMN, &separator, -1);
	return separator;
}


static gboolean
category_model_visible_func (GtkTreeModel *model,
			     GtkTreeIter  *iter,
			     gpointer      user_data)
{
	DialogData              *data = user_data;
	GthExtensionDescription *description;
	gboolean                 original_status_is_active;
	gboolean                 visible;

	if (strcmp (data->current_category, EXTENSION_CATEGORY_ALL) == 0)
		return TRUE;

	gtk_tree_model_get (model, iter,
			    EXTENSION_DESCRIPTION_COLUMN, &description,
			    EXTENSION_ORIGINAL_STATUS_COLUMN, &original_status_is_active,
			    -1);

	visible = FALSE;
	if ((strcmp (data->current_category, EXTENSION_CATEGORY_ENABLED) == 0) && original_status_is_active)
		visible = TRUE;
	else if ((strcmp (data->current_category, EXTENSION_CATEGORY_DISABLED) == 0) && ! original_status_is_active)
		visible = TRUE;
	else
		visible = g_strcmp0 (description->category, data->current_category) == 0;

	return visible;
}


void
dlg_extensions (GthBrowser *browser)
{
	DialogData          *data;
	GthExtensionManager *manager;
	GList               *extensions;
	GList               *scan;
	GSList              *all_active_extensions;
	GSList              *s_scan;
	GtkTreePath         *first;
	int                  i;

	if (gth_browser_get_dialog (browser, "extensions") != NULL) {
		gtk_window_present (GTK_WINDOW (gth_browser_get_dialog (browser, "extensions")));
		return;
	}

	manager = gth_main_get_default_extension_manager ();

	data = g_new0 (DialogData, 1);
	data->browser = browser;
	data->builder = _gtk_builder_new_from_file ("extensions.ui", NULL);

	data->active_extensions = NULL;
	all_active_extensions = eel_gconf_get_string_list (PREF_ACTIVE_EXTENSIONS);
	for (s_scan = all_active_extensions; s_scan; s_scan = s_scan->next) {
		char                    *name = s_scan->data;
		GthExtensionDescription *description;

		description = gth_extension_manager_get_description (manager, name);
		if ((description == NULL) || description->mandatory || description->hidden)
			continue;

		data->active_extensions = g_slist_prepend (data->active_extensions, g_strdup (name));
	}
	data->active_extensions = g_slist_reverse (data->active_extensions);
	g_slist_foreach (all_active_extensions, (GFunc) g_free, NULL);
	g_slist_free (all_active_extensions);

	/* Get the widgets. */

	data->dialog = _gtk_builder_get_widget (data->builder, "extensions_dialog");
	gth_browser_set_dialog (browser, "extensions", data->dialog);
	g_object_set_data (G_OBJECT (data->dialog), "dialog_data", data);

	/* Set widgets data. */

	data->list_store = gtk_list_store_new (EXTENSION_COLUMNS, G_TYPE_OBJECT, G_TYPE_BOOLEAN);
	data->model_filter = gtk_tree_model_filter_new (GTK_TREE_MODEL (data->list_store), NULL);
	data->list_view = gtk_tree_view_new_with_model (data->model_filter);
	g_object_unref (data->model_filter);
	g_object_unref (data->list_store);
        gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (data->list_view), TRUE);
        gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (data->list_view), TRUE);

        add_columns (GTK_TREE_VIEW (data->list_view), data);
	gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (data->list_store), 0, extension_compare_func, NULL, NULL);
        gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (data->list_store), 0, GTK_SORT_ASCENDING);

	gtk_widget_show (data->list_view);
	gtk_container_add (GTK_CONTAINER (GET_WIDGET ("extensions_scrolledwindow")), data->list_view);

	gtk_label_set_mnemonic_widget (GTK_LABEL (GET_WIDGET ("extensions_label")), data->list_view);
	gtk_label_set_use_underline (GTK_LABEL (GET_WIDGET ("extensions_label")), TRUE);

	manager = gth_main_get_default_extension_manager ();
	extensions = gth_extension_manager_get_extensions (manager);
	for (scan = extensions; scan; scan = scan->next) {
		const char              *name = scan->data;
		GthExtensionDescription *description;
		GtkTreeIter              iter;

		description = gth_extension_manager_get_description (manager, name);
		if ((description == NULL) || description->mandatory || description->hidden)
			continue;

		gtk_list_store_append (data->list_store, &iter);
		gtk_list_store_set (data->list_store, &iter,
				    EXTENSION_DESCRIPTION_COLUMN, description,
				    EXTENSION_ORIGINAL_STATUS_COLUMN, gth_extension_description_is_active (description),
				    -1);
	}
	g_list_free (extensions);

	/* the category combobox */

	data->current_category = g_strdup (EXTENSION_CATEGORY_ALL);
	for (i = 0; extension_category[i].id != NULL; i++) {
		GtkTreeIter iter;

		gtk_list_store_append (GTK_LIST_STORE (GET_WIDGET ("category_liststore")), &iter);
		if (strcmp (extension_category[i].id, EXTENSION_CATEGORY_SEPARATOR) == 0)
			gtk_list_store_set (GTK_LIST_STORE (GET_WIDGET ("category_liststore")),
					    &iter,
					    CATEGORY_SEPARATOR_COLUMN, TRUE,
					    -1);
		else
			gtk_list_store_set (GTK_LIST_STORE (GET_WIDGET ("category_liststore")),
					    &iter,
					    CATEGORY_NAME_COLUMN, _(extension_category[i].name),
					    CATEGORY_ID_COLUMN, extension_category[i].id,
					    /* CATEGORY_ICON_COLUMN, extension_category[i].icon, */
					    CATEGORY_SEPARATOR_COLUMN, FALSE,
					    -1);
	}
	gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (data->model_filter),
						category_model_visible_func,
						data,
						NULL);
	gtk_combo_box_set_row_separator_func (GTK_COMBO_BOX (GET_WIDGET ("category_combobox")),
					      category_view_separator_func,
					      data,
					      NULL);
	gtk_combo_box_set_active (GTK_COMBO_BOX (GET_WIDGET ("category_combobox")), 0);


	/* Set the signals handlers. */

	g_signal_connect (G_OBJECT (data->dialog),
			  "destroy",
			  G_CALLBACK (destroy_cb),
			  data);
	g_signal_connect_swapped (GET_WIDGET ("close_button"),
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  G_OBJECT (data->dialog));
	g_signal_connect (GET_WIDGET ("about_button"),
			  "clicked",
			  G_CALLBACK (about_button_clicked_cb),
			  data);
	g_signal_connect (GET_WIDGET ("preferences_button"),
			  "clicked",
			  G_CALLBACK (preferences_button_clicked_cb),
			  data);
	g_signal_connect (gtk_tree_view_get_selection (GTK_TREE_VIEW (data->list_view)),
			  "changed",
			  G_CALLBACK (list_view_selection_changed_cb),
			  data);
	g_signal_connect (GET_WIDGET ("category_combobox"),
			  "changed",
			  G_CALLBACK (category_combobox_changed_cb),
			  data);

	first = gtk_tree_path_new_first ();
	gtk_tree_selection_select_path (gtk_tree_view_get_selection (GTK_TREE_VIEW (data->list_view)), first);
	gtk_tree_path_free (first);

	/* run dialog. */

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog),
				      GTK_WINDOW (browser));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), TRUE);
	gtk_widget_show (data->dialog);
}
