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
#include <gtk/gtk.h>
#include <glade/glade.h>
#include "gthumb-window.h"
#include "gtk-utils.h"
#include "gconf-utils.h"
#include "glib-utils.h"
#include "file-utils.h"
#include "dlg-file-utils.h"
#include "preferences.h"

#ifdef HAVE_LIBEXIF
#include <exif-data.h>
#include <exif-content.h>
#include <exif-entry.h>
#endif /* HAVE_LIBEXIF */


enum {
	RS_OLDNAME_COLUMN,
	RS_NEWNAME_COLUMN,
	RS_NUM_COLUMNS
};

int            sort_method_to_idx[] = { -1, 0, -1, 1, 2 };
GthSortMethod  idx_to_sort_method[] = { GTH_SORT_METHOD_BY_NAME, GTH_SORT_METHOD_BY_SIZE, GTH_SORT_METHOD_BY_TIME };


#define COMMENT_GLADE_FILE "gthumb_tools.glade"

typedef struct {
	GThumbWindow  *window;
	GladeXML      *gui;

	GtkWidget     *dialog;
	GtkWidget     *rs_template_entry;
	GtkWidget     *rs_start_at_spinbutton;
	GtkWidget     *rs_sort_optionmenu;
	GtkWidget     *rs_reverse_checkbutton;
	GtkWidget     *rs_list_treeview;

	GtkListStore  *rs_list_model;
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
	char  *template;

	/* Save options */

	template = _gtk_entry_get_locale_text (GTK_ENTRY (data->rs_template_entry));
	eel_gconf_set_string (PREF_RENAME_SERIES_TEMPLATE, template);
	g_free (template);

	eel_gconf_set_integer (PREF_RENAME_SERIES_START_AT, gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (data->rs_start_at_spinbutton)));

	pref_set_rename_sort_order (idx_to_sort_method [gtk_option_menu_get_history (GTK_OPTION_MENU (data->rs_sort_optionmenu))]);

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

	dlg_file_rename_series (data->window, old_names, new_names);
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


static GCompareFunc
get_compare_func_from_idx (int column_index)
{
	static GCompareFunc compare_funcs[3] = {
		sort_by_name,
		sort_by_size,
		sort_by_time
	};

	return compare_funcs [column_index % 3];
}


#ifdef HAVE_LIBEXIF

static time_t
get_exif_time (const char *filename)
{
	ExifData     *edata;
	unsigned int  i, j;
	time_t        time = 0;
	struct tm     tm = { 0, };

	edata = exif_data_new_from_file (filename);

	if (edata == NULL) 
                return (time_t)0;

	for (i = 0; i < EXIF_IFD_COUNT; i++) {
		ExifContent *content = edata->ifd[i];

		if (! edata->ifd[i] || ! edata->ifd[i]->count) 
			continue;

		for (j = 0; j < content->count; j++) {
			ExifEntry   *e = content->entries[j];
			char        *data;

			if (! content->entries[j]) 
				continue;

			if ((e->tag != EXIF_TAG_DATE_TIME) &&
			    (e->tag != EXIF_TAG_DATE_TIME_ORIGINAL) &&
			    (e->tag != EXIF_TAG_DATE_TIME_DIGITIZED))
				continue;

			data = g_strdup (e->data);
			data[4] = data[7] = data[10] = data[13] = data[16] = '\0';

			tm.tm_year = atoi (data) - 1900;
			tm.tm_mon  = atoi (data + 5) - 1;
			tm.tm_mday = atoi (data + 8);
			tm.tm_hour = atoi (data + 11);
			tm.tm_min  = atoi (data + 14);
			tm.tm_sec  = atoi (data + 17);
			time = mktime (&tm);

			g_free (data);

			break;
		}
	}

	exif_data_unref (edata);

	return time;
}

#endif /* HAVE_LIBEXIF */


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
	strftime (stime, 50, "%Y-%m-%d %T", ltime);

	return stime;
}


static void
update_list (DialogData *data)
{
	GList  *scan, *on_scan, *nn_scan;
	int     idx;
	int     start_at;
	char  **template;
	char   *template_s;

	idx = gtk_option_menu_get_history (GTK_OPTION_MENU (data->rs_sort_optionmenu));
	data->file_list = g_list_sort (data->file_list, get_compare_func_from_idx (idx));
	
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->rs_reverse_checkbutton)))
		data->file_list = g_list_reverse (data->file_list);

	/**/

	template_s = _gtk_entry_get_locale_text (GTK_ENTRY (data->rs_template_entry));
	template = _g_get_template_from_text (template_s);
	g_free (template_s);

	start_at = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (data->rs_start_at_spinbutton));

	if (data->new_names_list != NULL) {
		g_list_foreach (data->new_names_list, (GFunc) g_free, NULL);
		g_list_free (data->new_names_list);
		data->new_names_list = NULL;
	}

	for (scan = data->file_list; scan; scan = scan->next) {
		FileData *fdata = scan->data;
		char     *name1;
		char     *name_wo_ext;
		char     *name2;
		char     *image_date;
		char     *name3;
		char     *extension;
		char     *new_name;

		name1       = _g_get_name_from_template (template, start_at++);
		name_wo_ext = remove_extension_from_path (fdata->name);
		name2       = _g_substitute (name1, '*', name_wo_ext);
		image_date  = get_image_date (fdata->path);
		name3       = _g_substitute (name2, '?', image_date);
		extension   = strrchr (fdata->name, '.');
		new_name    = g_strconcat (name3, extension, NULL);

		data->new_names_list = g_list_prepend (data->new_names_list, new_name);

		g_free (name_wo_ext);
		g_free (name1);
		g_free (name2);
		g_free (name3);
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
		
		utf8_on = g_locale_to_utf8 (fdata->name, -1, NULL, NULL, NULL);
		utf8_nn = g_locale_to_utf8 (new_name, -1, NULL, NULL, NULL);

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



void
dlg_rename_series (GThumbWindow *window)
{
	DialogData        *data;
	GtkWidget         *btn_ok;
	GtkWidget         *btn_cancel;
	GList             *list;
	GtkCellRenderer   *renderer;
	GtkTreeViewColumn *column;
	char              *svalue;

	list = gth_file_list_get_selection_as_fd (window->file_list);
	if (list == NULL) {
		g_warning ("No file selected.");
		return;
	}

	data = g_new (DialogData, 1);

	data->file_list = list;
	data->new_names_list = NULL;
	data->window = window;
	data->gui = glade_xml_new (GTHUMB_GLADEDIR "/" COMMENT_GLADE_FILE , NULL, NULL);
	if (!data->gui) {
		g_free (data);
		g_warning ("Could not find " GLADE_FILE "\n");
		return;
	}

	/* Get the widgets. */

	data->dialog = glade_xml_get_widget (data->gui, "rename_series_dialog");
	data->rs_template_entry = glade_xml_get_widget (data->gui, "rs_template_entry");
	data->rs_start_at_spinbutton = glade_xml_get_widget (data->gui, "rs_start_at_spinbutton");
	data->rs_sort_optionmenu = glade_xml_get_widget (data->gui, "rs_sort_optionmenu");
	data->rs_reverse_checkbutton = glade_xml_get_widget (data->gui, "rs_reverse_checkbutton");
	data->rs_list_treeview = glade_xml_get_widget (data->gui, "rs_list_treeview");

        btn_ok = glade_xml_get_widget (data->gui, "rs_ok_button");
        btn_cancel = glade_xml_get_widget (data->gui, "rs_cancel_button");

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

	/**/

	svalue = eel_gconf_get_string (PREF_RENAME_SERIES_TEMPLATE);
	_gtk_entry_set_locale_text (GTK_ENTRY (data->rs_template_entry), svalue);
	g_free (svalue);

	gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->rs_start_at_spinbutton), eel_gconf_get_integer (PREF_RENAME_SERIES_START_AT));

	gtk_option_menu_set_history (GTK_OPTION_MENU (data->rs_sort_optionmenu), sort_method_to_idx [pref_get_rename_sort_order ()]);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->rs_reverse_checkbutton), eel_gconf_get_boolean (PREF_RENAME_SERIES_REVERSE));

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

	g_signal_connect (G_OBJECT (data->rs_template_entry), 
			  "changed",
			  G_CALLBACK (update_list_cb),
			  data);
	g_signal_connect (G_OBJECT (data->rs_start_at_spinbutton), 
			  "value_changed",
			  G_CALLBACK (update_list_cb),
			  data);
	g_signal_connect (G_OBJECT (data->rs_sort_optionmenu), 
			  "changed",
			  G_CALLBACK (update_list_cb),
			  data);
	g_signal_connect (G_OBJECT (data->rs_reverse_checkbutton),
			  "toggled",
			  G_CALLBACK (update_list_cb),
			  data);

	/* run dialog. */

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), 
				      GTK_WINDOW (window->app));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), TRUE);
	gtk_widget_show (data->dialog);
}
