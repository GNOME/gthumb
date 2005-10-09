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
#include <string.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <glade/glade.h>
#include <libgnome/gnome-help.h>
#include <libgnomevfs/gnome-vfs-utils.h>

#include "gth-browser.h"
#include "gtk-utils.h"
#include "gconf-utils.h"
#include "glib-utils.h"
#include "gth-exif-utils.h"
#include "file-utils.h"
#include "dlg-file-utils.h"
#include "preferences.h"

enum {
	RS_OLDNAME_COLUMN,
	RS_NEWNAME_COLUMN,
	RS_NUM_COLUMNS
};

int            sort_method_to_idx[] = { -1, 0, -1, 1, 2, 3 };
GthSortMethod  idx_to_sort_method[] = { GTH_SORT_METHOD_BY_NAME, GTH_SORT_METHOD_BY_SIZE, GTH_SORT_METHOD_BY_TIME, GTH_SORT_METHOD_MANUAL };


#define GLADE_FILE "gthumb_tools.glade"

typedef struct {
	GthBrowser    *browser;
	GladeXML      *gui;

	GtkWidget     *dialog;
	GtkWidget     *rs_template_entry;
	GtkWidget     *rs_start_at_spinbutton;
	GtkWidget     *rs_sort_combobox;
	GtkWidget     *rs_reverse_checkbutton;
	GtkWidget     *rs_list_treeview;

	GtkListStore  *rs_list_model;
	GList         *original_file_list;
	GList         *file_list;
	GList         *new_names_list;
} DialogData;


/* called when the main dialog is closed. */
static void
destroy_cb (GtkWidget  *widget, 
	    DialogData *data)
{
	if (data->file_list != NULL)
		g_list_free (data->file_list);
	file_data_list_free (data->original_file_list); 
	if (data->new_names_list != NULL) {
		g_list_foreach (data->new_names_list, (GFunc) g_free, NULL);
		g_list_free (data->new_names_list);
	}
	g_object_unref (data->gui);
	g_free (data);
}


/* called when the "ok" button is pressed. */
static void
ok_clicked_cb (GtkWidget  *widget, 
	       DialogData *data)
{
	GList *old_names = NULL;
	GList *new_names = NULL;
	GList *o_scan, *n_scan;
	const char  *template;

	/* Save options */

	template = gtk_entry_get_text (GTK_ENTRY (data->rs_template_entry));
	eel_gconf_set_string (PREF_RENAME_SERIES_TEMPLATE, template);

	eel_gconf_set_integer (PREF_RENAME_SERIES_START_AT, gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (data->rs_start_at_spinbutton)));

	pref_set_rename_sort_order (idx_to_sort_method [gtk_combo_box_get_active (GTK_COMBO_BOX (data->rs_sort_combobox))]);

	eel_gconf_set_boolean (PREF_RENAME_SERIES_REVERSE, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->rs_reverse_checkbutton)));

	/**/

	for (o_scan = data->file_list, n_scan = data->new_names_list; o_scan && n_scan; o_scan = o_scan->next, n_scan = n_scan->next) {
		FileData *fdata     = o_scan->data;
		char *old_full_path = fdata->path;
		char *new_name      = n_scan->data;
		char *old_path      = remove_level_from_path (old_full_path);
		char *new_full_path;

		new_full_path = g_strconcat (old_path, "/", new_name, NULL);
		g_free (old_path);

		old_names = g_list_prepend (old_names, g_strdup (fdata->path));
		new_names = g_list_prepend (new_names, new_full_path);
	}
	old_names = g_list_reverse (old_names);
	new_names = g_list_reverse (new_names);

	dlg_file_rename_series (GTH_WINDOW (data->browser), old_names, new_names);
	gtk_widget_destroy (data->dialog);
}


static gint
sort_by_name (gconstpointer  ptr1,
              gconstpointer  ptr2)
{
	const FileData *fdata1 = ptr1, *fdata2 = ptr2;
	return strcasecmp (fdata1->name, fdata2->name); 
}


static gint
sort_by_size (gconstpointer  ptr1,
              gconstpointer  ptr2)
{
	const FileData *fdata1 = ptr1, *fdata2 = ptr2;

	if (fdata1->size == fdata2->size)
		return sort_by_name (ptr1, ptr2);
	else if (fdata1->size > fdata2->size)
		return 1;
	else
		return -1;
}


static gint
sort_by_time (gconstpointer  ptr1,
              gconstpointer  ptr2)
{
	const FileData *fdata1 = ptr1, *fdata2 = ptr2;

	if (fdata1->mtime == fdata2->mtime)
		return sort_by_name (ptr1, ptr2);
	else if (fdata1->mtime > fdata2->mtime)
		return 1;
	else
		return -1;
}


static gint
sort_by_manual_order (gconstpointer  ptr1,
		      gconstpointer  ptr2)
{
	return -1;
}


static GCompareFunc
get_compare_func_from_idx (int column_index)
{
	static GCompareFunc compare_funcs[4] = {
		sort_by_name,
		sort_by_size,
		sort_by_time,
		sort_by_manual_order
	};

	return compare_funcs [column_index % 4];
}


static char *
get_image_date (const char *filename)
{
	time_t     mtime = 0;
	struct tm *ltime;
	char      *stime;

#ifdef HAVE_LIBEXIF
	mtime = get_exif_time (filename);
#endif /* HAVE_LIBEXIF */

	if (mtime == 0)
		mtime = get_file_mtime (filename);

	ltime = localtime (&mtime);

	stime = g_new (char, 50 + 1);
	strftime (stime, 50, "%Y-%m-%d--%H.%M.%S", ltime);

	return stime;
}


static void
update_list (DialogData *data)
{
	GList  *scan, *on_scan, *nn_scan;
	int     idx;
	int     start_at;
	char  **template;
	const char   *template_s;

	idx = gtk_combo_box_get_active (GTK_COMBO_BOX (data->rs_sort_combobox));

	if (data->file_list != NULL)
		g_list_free (data->file_list);
	data->file_list = g_list_copy (data->original_file_list);
	data->file_list = g_list_sort (data->file_list, get_compare_func_from_idx (idx));
	
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->rs_reverse_checkbutton)))
		data->file_list = g_list_reverse (data->file_list);

	/**/

	template_s = gtk_entry_get_text (GTK_ENTRY (data->rs_template_entry));
	template = _g_get_template_from_text (template_s);

	start_at = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (data->rs_start_at_spinbutton));

	if (data->new_names_list != NULL) {
		g_list_foreach (data->new_names_list, (GFunc) g_free, NULL);
		g_list_free (data->new_names_list);
		data->new_names_list = NULL;
	}

	for (scan = data->file_list; scan; scan = scan->next) {
		FileData *fdata = scan->data;
		char     *name_wo_ext = remove_extension_from_path (fdata->name);
		char     *utf8_txt, *image_date, *image_size;
		char     *name1 = NULL;
		char     *name2;
		char     *name3;
		char     *name4;
		char     *extension;
		char     *new_name;

		name1 = _g_get_name_from_template (template, start_at++);

		utf8_txt = g_filename_to_utf8 (name_wo_ext, -1, 0, 0, 0);
		name2 = _g_substitute_pattern (name1, 'f', utf8_txt);
		g_free (name_wo_ext);
		g_free (utf8_txt);

		image_date = get_image_date (fdata->path);
		utf8_txt = g_locale_to_utf8 (image_date, -1, 0, 0, 0);
		name3 = _g_substitute_pattern (name2, 'd', utf8_txt);
		g_free (image_date);
		g_free (utf8_txt);

		image_size = gnome_vfs_format_file_size_for_display (get_file_size (fdata->path));
		name4 = _g_substitute_pattern (name3, 's', image_size);
		g_free (image_size);

		extension = g_filename_to_utf8 (strrchr (fdata->name, '.'), -1, 0, 0, 0);
		new_name = g_strconcat (name4, extension, NULL);

		data->new_names_list = g_list_prepend (data->new_names_list, g_filename_from_utf8 (new_name, -1, 0, 0, 0));

		g_free (extension);
		g_free (name1);
		g_free (name2);
		g_free (name3);
		g_free (name4);
		g_free (new_name);
	}
	data->new_names_list = g_list_reverse (data->new_names_list);
	g_strfreev (template);

	/**/

	gtk_list_store_clear (data->rs_list_model);
	nn_scan = data->new_names_list;
	for (on_scan = data->file_list; on_scan && nn_scan; on_scan = on_scan->next) {
		FileData    *fdata = on_scan->data;
		char        *new_name = nn_scan->data;
		GtkTreeIter  iter;
		char        *utf8_on;
		char        *utf8_nn;
		
		gtk_list_store_append (data->rs_list_model, &iter);
		
		utf8_on = g_filename_display_name (fdata->name);
		utf8_nn = g_filename_display_name (new_name);
		gtk_list_store_set (data->rs_list_model, &iter,
				    RS_OLDNAME_COLUMN, utf8_on,
				    RS_NEWNAME_COLUMN, utf8_nn,
				    -1);
		g_free (utf8_on);
		g_free (utf8_nn);

		nn_scan = nn_scan->next;
	}
}


static void
update_list_cb (GtkWidget  *widget,
		DialogData *data)
{
	update_list (data);
}


/* called when the "help" button is clicked. */
static void
help_cb (GtkWidget  *widget, 
	 DialogData *data)
{
	GError *err;

	err = NULL;  
	gnome_help_display ("gthumb", "gthumb-rename-series", &err);
	
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



void
dlg_rename_series (GthBrowser *browser)
{
	DialogData        *data;
	GtkWidget         *btn_ok;
	GtkWidget         *btn_cancel;
	GtkWidget         *help_button;
	GList             *list;
	GtkCellRenderer   *renderer;
	GtkTreeViewColumn *column;
	char              *svalue;
	gboolean           reorderable;
	int                idx;

	list = gth_window_get_file_list_selection_as_fd (GTH_WINDOW (browser));
	if (list == NULL) {
		g_warning ("No file selected.");
		return;
	}

	data = g_new0 (DialogData, 1);

	data->original_file_list = list;
	data->file_list = NULL;
	data->new_names_list = NULL;
	data->browser = browser;
	data->gui = glade_xml_new (GTHUMB_GLADEDIR "/" GLADE_FILE , NULL, NULL);
	if (!data->gui) {
		g_free (data);
		g_warning ("Could not find " GLADE_FILE "\n");
		return;
	}

	reorderable = gth_file_view_get_reorderable (gth_browser_get_file_view (browser));

	/* Get the widgets. */

	data->dialog = glade_xml_get_widget (data->gui, "rename_series_dialog");
	data->rs_template_entry = glade_xml_get_widget (data->gui, "rs_template_entry");
	data->rs_start_at_spinbutton = glade_xml_get_widget (data->gui, "rs_start_at_spinbutton");
	data->rs_sort_combobox = glade_xml_get_widget (data->gui, "rs_sort_combobox");
	data->rs_reverse_checkbutton = glade_xml_get_widget (data->gui, "rs_reverse_checkbutton");
	data->rs_list_treeview = glade_xml_get_widget (data->gui, "rs_list_treeview");

        btn_ok = glade_xml_get_widget (data->gui, "rs_ok_button");
        btn_cancel = glade_xml_get_widget (data->gui, "rs_cancel_button");
	help_button = glade_xml_get_widget (data->gui, "rs_help_button");

	/* Set widgets data. */

	data->rs_list_model = gtk_list_store_new (RS_NUM_COLUMNS,
						  G_TYPE_STRING, 
						  G_TYPE_STRING);
	gtk_tree_view_set_model (GTK_TREE_VIEW (data->rs_list_treeview),
				 GTK_TREE_MODEL (data->rs_list_model));
	g_object_unref (data->rs_list_model);

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Old Name"),
							   renderer,
							   "text", RS_OLDNAME_COLUMN,
							   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (data->rs_list_treeview),
				     column);

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("New Name"),
							   renderer,
							   "text", RS_NEWNAME_COLUMN,
							   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (data->rs_list_treeview),
				     column);

	gtk_combo_box_append_text (GTK_COMBO_BOX (data->rs_sort_combobox),
				   _("by size"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (data->rs_sort_combobox),
				   _("by modified time"));
	if (reorderable)
		gtk_combo_box_append_text (GTK_COMBO_BOX (data->rs_sort_combobox),
					   _("manual order"));

	/**/

	svalue = eel_gconf_get_string (PREF_RENAME_SERIES_TEMPLATE, "###");
	gtk_entry_set_text (GTK_ENTRY (data->rs_template_entry), svalue);
	g_free (svalue);

	gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->rs_start_at_spinbutton), eel_gconf_get_integer (PREF_RENAME_SERIES_START_AT, 1));

	idx = sort_method_to_idx [pref_get_rename_sort_order ()];
	if (!reorderable && (sort_method_to_idx[GTH_SORT_METHOD_MANUAL] == idx))
		idx = sort_method_to_idx[GTH_SORT_METHOD_BY_NAME];
	gtk_combo_box_set_active (GTK_COMBO_BOX (data->rs_sort_combobox), idx);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->rs_reverse_checkbutton), eel_gconf_get_boolean (PREF_RENAME_SERIES_REVERSE, FALSE));

	update_list (data);

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
	g_signal_connect (G_OBJECT (help_button), 
			  "clicked",
			  G_CALLBACK (help_cb),
			  data);

	g_signal_connect (G_OBJECT (data->rs_template_entry), 
			  "changed",
			  G_CALLBACK (update_list_cb),
			  data);
	g_signal_connect (G_OBJECT (data->rs_start_at_spinbutton), 
			  "value_changed",
			  G_CALLBACK (update_list_cb),
			  data);
	g_signal_connect (G_OBJECT (data->rs_sort_combobox), 
			  "changed",
			  G_CALLBACK (update_list_cb),
			  data);
	g_signal_connect (G_OBJECT (data->rs_reverse_checkbutton),
			  "toggled",
			  G_CALLBACK (update_list_cb),
			  data);

	/* run dialog. */

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), 
				      GTK_WINDOW (browser));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), TRUE);
	gtk_widget_show (data->dialog);
}
