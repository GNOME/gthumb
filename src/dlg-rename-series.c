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
#include "gth-utils.h"
#include "gtk-utils.h"
#include "gconf-utils.h"
#include "glib-utils.h"
#include "gth-exif-utils.h"
#include "file-utils.h"
#include "dlg-file-utils.h"
#include "preferences.h"
#include "gth-sort-utils.h"

enum {
	RS_OLDNAME_COLUMN,
	RS_NEWNAME_COLUMN,
	RS_NUM_COLUMNS
};

static int           sort_method_to_idx[] = { -1, 0, 1, 2, 3, 4, 5 };
static GthSortMethod idx_to_sort_method[] = { GTH_SORT_METHOD_BY_NAME,
                                              GTH_SORT_METHOD_BY_PATH,
                                              GTH_SORT_METHOD_BY_SIZE,
                                              GTH_SORT_METHOD_BY_TIME,
                                              GTH_SORT_METHOD_BY_EXIF_DATE,
                                              GTH_SORT_METHOD_BY_COMMENT,
                                              GTH_SORT_METHOD_MANUAL};


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
	GthSortMethod  sort_method;
	gboolean       single_file;
	
	GHashTable    *date_cache;
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
	g_hash_table_destroy (data->date_cache);
	g_object_unref (data->gui);
	g_free (data);
}


/* called when the "ok" button is pressed. */
static void
ok_clicked_cb (GtkWidget  *widget,
	       DialogData *data)
{
	GList       *old_names = NULL;
	GList       *new_names = NULL;
	GList       *o_scan, *n_scan;
	const char  *template;

	/* Save options */

	template = gtk_entry_get_text (GTK_ENTRY (data->rs_template_entry));
	if (strchr (template, '/') != NULL) {
		_gtk_error_dialog_run (GTK_WINDOW (data->dialog),
				       _("The name \"%s\" is not valid because it contains the character \"/\". " "Please use a different name."), template);
		return;
	}

	if (! data->single_file) {
		eel_gconf_set_string (PREF_RENAME_SERIES_TEMPLATE, template);
		eel_gconf_set_integer (PREF_RENAME_SERIES_START_AT, gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (data->rs_start_at_spinbutton)));
		pref_set_rename_sort_order (idx_to_sort_method [gtk_combo_box_get_active (GTK_COMBO_BOX (data->rs_sort_combobox))]);
		eel_gconf_set_boolean (PREF_RENAME_SERIES_REVERSE, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->rs_reverse_checkbutton)));
	}

	for (o_scan = data->file_list, n_scan = data->new_names_list; o_scan && n_scan; o_scan = o_scan->next, n_scan = n_scan->next) {
		FileData *fdata         = o_scan->data;
		char     *old_full_path = fdata->path;
		char     *new_name      = n_scan->data;
		char     *old_path      = remove_level_from_path (old_full_path);
		char     *new_full_path;

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


static int
comp_func_name (gconstpointer  ptr1,
		gconstpointer  ptr2)
{
	const FileData *fd1 = ptr1, *fd2 = ptr2;

	if ((fd1 == NULL) || (fd2 == NULL))
		return 0;

	return gth_sort_by_filename_but_ignore_path (fd1->name, fd2->name);
}


static int
comp_func_size (gconstpointer  ptr1,
		gconstpointer  ptr2)
{
	const FileData *fd1 = ptr1, *fd2 = ptr2;

	if ((fd1 == NULL) || (fd2 == NULL))
		return 0;

	return gth_sort_by_size_then_name (fd1->size, fd2->size, fd1->path, fd2->path);
}


static int
comp_func_time (gconstpointer  ptr1,
		gconstpointer  ptr2)
{
	const FileData *fd1 = ptr1, *fd2 = ptr2;

	if ((fd1 == NULL) || (fd2 == NULL))
		return 0;

	return gth_sort_by_filetime_then_name (fd1->mtime, fd2->mtime,
						 fd1->path, fd2->path);
}


static int
comp_func_exif_date (gconstpointer  ptr1,
		     gconstpointer  ptr2)
{
	FileData *fd1 = (FileData *) ptr1, *fd2 = (FileData *) ptr2;

	if ((fd1 == NULL) || (fd2 == NULL))
		return 0;

	return gth_sort_by_exiftime_then_name (fd1, fd2);
}


static int
comp_func_path (gconstpointer  ptr1,
		gconstpointer  ptr2)
{
	const FileData *fd1 = ptr1, *fd2 = ptr2;

	if ((fd1 == NULL) || (fd2 == NULL))
		return 0;

	return gth_sort_by_full_path (fd1->path, fd2->path);
}


static int
comp_func_comment (gconstpointer ptr1, gconstpointer ptr2)
{
	const FileData *fd1 = ptr1, *fd2 = ptr2;

	return gth_sort_by_comment_then_name (fd1->comment, fd2->comment,
					      fd1->path, fd2->path);
}


static GCompareFunc
get_sortfunc (DialogData *data)
{
        GCompareFunc func;

        switch (data->sort_method) {
        case GTH_SORT_METHOD_BY_NAME:
                func = comp_func_name;
                break;
        case GTH_SORT_METHOD_BY_TIME:
                func = comp_func_time;
                break;
        case GTH_SORT_METHOD_BY_SIZE:
                func = comp_func_size;
                break;
        case GTH_SORT_METHOD_BY_PATH:
                func = comp_func_path;
                break;
        case GTH_SORT_METHOD_BY_COMMENT:
                func = comp_func_comment;
                break;
        case GTH_SORT_METHOD_BY_EXIF_DATE:
                func = comp_func_exif_date;
                break;
        case GTH_SORT_METHOD_NONE:
                func = gth_sort_none;
                break;
        default:
                func = gth_sort_none;
                break;
        }

        return func;
}


static char *
get_image_date (const char *filename)
{
	time_t     mtime = 0;
	struct tm *ltime;
	char      *stime;

	mtime = get_metadata_time (NULL, filename);

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
	GList       *scan, *on_scan, *nn_scan;
	int          start_at;
	char       **template;
	const char  *template_s;

	data->sort_method = idx_to_sort_method [gtk_combo_box_get_active (GTK_COMBO_BOX (data->rs_sort_combobox))];

	if (data->file_list != NULL)
		g_list_free (data->file_list);
	data->file_list = g_list_copy (data->original_file_list);
	data->file_list = g_list_sort (data->file_list, get_sortfunc (data));

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
		char     *name5;
		char     *extension = NULL;
		char     *new_name;
		char     *cached_date;

		name1 = _g_get_name_from_template (template, start_at++);
		utf8_txt = gnome_vfs_unescape_string_for_display (name_wo_ext);
		name2 = _g_substitute_pattern (name1, 'f', utf8_txt);
		g_free (name_wo_ext);
		g_free (utf8_txt);

		cached_date = g_hash_table_lookup (data->date_cache, fdata->path);
		if (cached_date != NULL)
			image_date = g_strdup (cached_date);
		else {
			image_date = get_image_date (fdata->path);
			if (image_date != NULL)
				g_hash_table_insert (data->date_cache, g_strdup (fdata->path), g_strdup (image_date));
		}
		utf8_txt = g_locale_to_utf8 (image_date, -1, 0, 0, 0);
		name3 = _g_substitute_pattern (name2, 'd', utf8_txt);
		g_free (image_date);
		g_free (utf8_txt);

		image_size = gnome_vfs_format_file_size_for_display (fdata->size);
		name4 = _g_substitute_pattern (name3, 's', image_size);
		g_free (image_size);

		if (strrchr (fdata->name, '.') != NULL)
			extension = g_filename_to_utf8 (strrchr (fdata->name, '.'), -1, 0, 0, 0);

		name5 = _g_substitute_pattern (name4, 'e', extension);
		new_name = gnome_vfs_escape_string (name5);
		data->new_names_list = g_list_prepend (data->new_names_list, new_name);

		g_free (extension);
		g_free (name1);
		g_free (name2);
		g_free (name3);
		g_free (name4);
		g_free (name5);
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

		utf8_on = gnome_vfs_unescape_string_for_display (fdata->name);
		utf8_nn = gnome_vfs_unescape_string_for_display (new_name);
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
	gthumb_display_help (GTK_WINDOW (data->dialog), "gthumb-rename-series");
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
	gboolean           reorderable;
	int                idx;

	list = gth_window_get_file_list_selection_as_fd (GTH_WINDOW (browser));
	if (list == NULL) {
		g_warning ("No file selected.");
		return;
	}

	data = g_new0 (DialogData, 1);

	data->original_file_list = list;
	data->single_file = (data->original_file_list->next == NULL);
	data->file_list = NULL;
	data->new_names_list = NULL;
	data->browser = browser;
	data->gui = glade_xml_new (GTHUMB_GLADEDIR "/" GLADE_FILE , NULL, NULL);
	if (!data->gui) {
		g_free (data);
		g_warning ("Could not find " GLADE_FILE "\n");
		return;
	}
	data->date_cache = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);

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
                                   _("by path"));
        gtk_combo_box_append_text (GTK_COMBO_BOX (data->rs_sort_combobox),
                                   _("by size"));
        gtk_combo_box_append_text (GTK_COMBO_BOX (data->rs_sort_combobox),
                                   _("by file modified time"));
        gtk_combo_box_append_text (GTK_COMBO_BOX (data->rs_sort_combobox),
                                   _("by Exif DateTime tag"));
        gtk_combo_box_append_text (GTK_COMBO_BOX (data->rs_sort_combobox),
                                   _("by comment"));
        if (reorderable)
                gtk_combo_box_append_text (GTK_COMBO_BOX (data->rs_sort_combobox),
                                           _("manual order"));
	idx = sort_method_to_idx [pref_get_rename_sort_order ()];
	if (!reorderable && (sort_method_to_idx[GTH_SORT_METHOD_MANUAL] == idx))
		idx = sort_method_to_idx[GTH_SORT_METHOD_BY_NAME];
	gtk_combo_box_set_active (GTK_COMBO_BOX (data->rs_sort_combobox), idx);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->rs_reverse_checkbutton), eel_gconf_get_boolean (PREF_RENAME_SERIES_REVERSE, FALSE));

	gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->rs_start_at_spinbutton), eel_gconf_get_integer (PREF_RENAME_SERIES_START_AT, 1));

	gtk_widget_set_sensitive (data->rs_sort_combobox, ! data->single_file);
	gtk_widget_set_sensitive (data->rs_reverse_checkbutton, ! data->single_file);
	gtk_widget_set_sensitive (data->rs_start_at_spinbutton, ! data->single_file);

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

	if (data->single_file) {
		FileData   *fd = data->original_file_list->data;
		const char *last_dot;
		glong       last_dot_pos;
		char       *template;

		template = gnome_vfs_unescape_string_for_display (fd->name);
		gtk_entry_set_text (GTK_ENTRY (data->rs_template_entry), template);
		last_dot = g_utf8_strrchr (template, -1, '.');
		if (last_dot != NULL)
			last_dot_pos = g_utf8_strlen (last_dot, -1);
		else
			last_dot_pos = 0;
		gtk_editable_select_region (GTK_EDITABLE (data->rs_template_entry), 0, g_utf8_strlen (template, -1) - last_dot_pos);
		g_free (template);
	}
	else {
		char *template;

		template = eel_gconf_get_string (PREF_RENAME_SERIES_TEMPLATE, "###");
		gtk_entry_set_text (GTK_ENTRY (data->rs_template_entry), template);
		g_free (template);
	}

	update_list (data);
}
