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
#include <time.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <libgnome/gnome-help.h>
#include <libgnomeui/gnome-dateedit.h>
#include <glade/glade.h>
#include <libgnomevfs/gnome-vfs.h>

#include "file-utils.h"
#include "catalog.h"
#include "comments.h"
#include "dlg-search.h"
#include "gthumb-init.h"
#include "gconf-utils.h"
#include "search.h"
#include "gth-browser.h"
#include "gth-utils.h"
#include "gtk-utils.h"
#include "glib-utils.h"


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
#define ONE_DAY (60*60*24)

static void dlg_search_ui (GthBrowser *browser,
			   char       *catalog_path,
			   gboolean    start_search);


void
dlg_search (GtkWidget *widget, gpointer data)
{
	dlg_search_ui (GTH_BROWSER (data), NULL, FALSE);
}


void
dlg_catalog_edit_search (GthBrowser *browser,
			 const char *catalog_path)
{
	dlg_search_ui (browser, g_strdup (catalog_path), FALSE);
}


void
dlg_catalog_search (GthBrowser *browser,
		    const char *catalog_path)
{
	dlg_search_ui (browser, g_strdup (catalog_path), TRUE);
}


/* -- implementation -- */


#define SEARCH_GLADE_FILE      "gthumb_search.glade"

typedef struct {
	GthBrowser     *browser;
	GladeXML       *gui;

	GtkWidget      *dialog;
	GtkWidget      *search_progress_dialog;

	GtkWidget      *s_start_from_filechooserbutton;
	GtkWidget      *s_include_subfold_checkbutton;
	GtkWidget      *s_filename_entry;
	GtkWidget      *s_comment_entry;
	GtkWidget      *s_place_entry;
	GtkWidget      *s_categories_entry;
	GtkWidget      *s_choose_categories_button;
	GtkWidget      *s_date_optionmenu;
	GtkWidget      *s_date_dateedit;

	GthFileList    *file_list;

	GtkWidget      *p_current_dir_entry;
	GtkWidget      *p_notebook;
	GtkWidget      *p_view_button;
	GtkWidget      *p_search_button;
	GtkWidget      *p_cancel_button;
	GtkWidget      *p_close_button;
	GtkWidget      *p_searching_in_hbox;
	GtkWidget      *p_images_label;

	/* -- search data -- */

	SearchData     *search_data;
	char          **file_patterns;
	char          **comment_patterns;
	char          **place_patterns;
	char          **keywords_patterns;
	gboolean        all_keywords;

	GnomeVFSAsyncHandle *handle;
	GnomeVFSURI    *uri;
	GList          *files;
	GList          *dirs;

	char           *catalog_path;

	GHashTable     *folders_comment;
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


static gboolean
remove_folder_comment_cb (gpointer key, gpointer value, gpointer user_data)
{
	g_free (key);
	comment_data_free (value);
        return TRUE;
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

	g_hash_table_foreach_remove (data->folders_comment, remove_folder_comment_cb, NULL);
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
	g_hash_table_destroy (data->folders_comment);
	g_free (data);
}


static void search_images_async (DialogData *data);


static void
start_search_now (DialogData *data)
{
	/* pop up the progress dialog. */

	gtk_window_set_modal (GTK_WINDOW (data->dialog), FALSE);
	gtk_widget_hide (data->dialog);

	gtk_notebook_set_current_page (GTK_NOTEBOOK (data->p_notebook), 0);
	gtk_widget_set_sensitive (data->p_searching_in_hbox, TRUE);
	gtk_widget_set_sensitive (data->p_view_button, FALSE);
	gtk_widget_set_sensitive (data->p_search_button, FALSE);
	gtk_widget_set_sensitive (data->p_close_button, FALSE);
	gtk_label_set_text (GTK_LABEL (data->p_images_label), "");

	gtk_window_set_modal (GTK_WINDOW (data->dialog), TRUE);
	gtk_widget_show (data->search_progress_dialog);

	search_images_async (data);
}


/* called when the "search" button is pressed. */
static void
search_clicked_cb (GtkWidget  *widget,
		   DialogData *data)
{
	char       *full_path;
	char       *esc_path;
	const char *entry;

	/* collect search data. */

	free_search_criteria_data (data);
	search_data_free (data->search_data);
	data->search_data = search_data_new ();

	/* * start from */

	esc_path = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (data->s_start_from_filechooserbutton));
	full_path = gnome_vfs_unescape_string (esc_path, "");
	g_free (esc_path);

	search_data_set_start_from (data->search_data, full_path);
	g_free (full_path);

	/* * recursive */

	search_data_set_recursive (data->search_data, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->s_include_subfold_checkbutton)));

	/* * file pattern */

	entry = gtk_entry_get_text (GTK_ENTRY (data->s_filename_entry));
	search_data_set_file_pattern (data->search_data, entry);
	if (entry != NULL)
		data->file_patterns = search_util_get_file_patterns (entry);

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
	search_data_set_keywords_pattern (data->search_data, entry, data->all_keywords);
	if (entry != NULL)
		data->keywords_patterns = search_util_get_patterns (entry);

	/* * date scope pattern */

	search_data_set_date_scope (data->search_data, gtk_option_menu_get_history (GTK_OPTION_MENU (data->s_date_optionmenu)));

	/* * date */

	search_data_set_date (data->search_data, gnome_date_edit_get_time (GNOME_DATE_EDIT (data->s_date_dateedit)));

	/**/

	start_search_now (data);
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

	gtk_window_set_modal (GTK_WINDOW (data->dialog), FALSE);
	gtk_widget_hide (data->search_progress_dialog);

	gtk_window_set_modal (GTK_WINDOW (data->dialog), TRUE);
	gtk_widget_show (data->dialog);
}


/* called when the "view" button in the progress dialog is pressed. */
static void
view_result_cb (GtkWidget  *widget,
		DialogData *data)
{
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
	catalog_name = g_filename_from_utf8 (catalog_name_utf8, -1, 0, 0, 0);
	catalog_path = get_catalog_full_path (catalog_name);
	g_free (catalog_name);
	g_free (catalog_name_utf8);

	catalog_set_path (catalog, catalog_path);
	catalog_set_search_data (catalog, data->search_data);

	for (scan = data->files; scan; scan = scan->next)
		catalog_add_item (catalog, (gchar*) scan->data);

	if (! catalog_write_to_disk (catalog, &gerror))
		_gtk_error_dialog_from_gerror_run (GTK_WINDOW (data->dialog), &gerror);

	gth_browser_go_to_catalog (data->browser, catalog_path);

	gtk_widget_destroy (data->search_progress_dialog);
	catalog_free (catalog);
	g_free (catalog_path);
}


/* called when the "ok" button is pressed. */
static void
save_result_cb (GtkWidget  *widget,
		DialogData *data)
{
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

	gth_browser_go_to_catalog (data->browser, catalog_path);

	gtk_widget_destroy (data->search_progress_dialog);
	catalog_free (catalog);
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
	gtk_widget_set_sensitive (data->p_view_button, TRUE);
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
	gthumb_display_help (GTK_WINDOW (data->dialog), "gthumb-find");
}


/* -- choose category dialog -- */


static void choose_categories_dialog (DialogData *data);


static void
choose_categories_cb (GtkWidget  *widget,
		      DialogData *data)
{
	choose_categories_dialog (data);
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


static gboolean
idle_start_search_cb (gpointer data)
{
	start_search_now (data);
	return FALSE;
}


static void
dlg_search_ui (GthBrowser *browser,
	       char       *catalog_path,
	       gboolean    start_search)
{
	DialogData *data;
	GtkWidget  *progress_hbox;

	g_return_if_fail (GTH_IS_BROWSER (browser));

	data = g_new0 (DialogData, 1);
	data->gui = glade_xml_new (GTHUMB_GLADEDIR "/" SEARCH_GLADE_FILE, NULL, NULL);
	if (! data->gui) {
		g_free (data);
		g_warning ("Could not find " SEARCH_GLADE_FILE "\n");
		return;
	}

	data->file_patterns = NULL;
	data->comment_patterns = NULL;
	data->place_patterns = NULL;
	data->keywords_patterns = NULL;
	data->dirs = NULL;
	data->files = NULL;
	data->browser = browser;
	data->handle = NULL;
	data->search_data = NULL;
	data->uri = NULL;
	data->catalog_path = catalog_path;
	data->folders_comment = g_hash_table_new (g_str_hash, g_str_equal);

	/* Get the widgets. */

	data->dialog = glade_xml_get_widget (data->gui, "search_dialog");

	data->s_start_from_filechooserbutton = glade_xml_get_widget (data->gui, "s_start_from_filechooserbutton");
	data->s_include_subfold_checkbutton = glade_xml_get_widget (data->gui, "s_include_subfold_checkbutton");
	data->s_filename_entry = glade_xml_get_widget (data->gui, "s_filename_entry");
	data->s_comment_entry = glade_xml_get_widget (data->gui, "s_comment_entry");
	data->s_place_entry = glade_xml_get_widget (data->gui, "s_place_entry");
	data->s_categories_entry = glade_xml_get_widget (data->gui, "s_categories_entry");

	data->s_choose_categories_button = glade_xml_get_widget (data->gui, "s_choose_categories_button");
	data->s_date_optionmenu = glade_xml_get_widget (data->gui, "s_date_optionmenu");
	data->s_date_dateedit = glade_xml_get_widget (data->gui, "s_date_dateedit");

	if (catalog_path == NULL) {
		data->search_progress_dialog = glade_xml_get_widget (data->gui, "search_progress_dialog");
		data->p_current_dir_entry = glade_xml_get_widget (data->gui, "p_current_dir_entry");
		data->p_notebook = glade_xml_get_widget (data->gui, "p_notebook");
		data->p_view_button = glade_xml_get_widget (data->gui, "p_view_button");
		data->p_search_button = glade_xml_get_widget (data->gui, "p_search_button");
		data->p_cancel_button = glade_xml_get_widget (data->gui, "p_cancel_button");
		data->p_close_button = glade_xml_get_widget (data->gui, "p_close_button");
		data->p_searching_in_hbox = glade_xml_get_widget (data->gui, "p_searching_in_hbox");
		data->p_images_label = glade_xml_get_widget (data->gui, "p_images_label");
		progress_hbox = glade_xml_get_widget (data->gui, "p_progress_hbox");
	} else {
		data->search_progress_dialog = glade_xml_get_widget (data->gui, "edit_search_progress_dialog");
		data->p_current_dir_entry = glade_xml_get_widget (data->gui, "ep_current_dir_entry");
		data->p_notebook = glade_xml_get_widget (data->gui, "ep_notebook");
		data->p_view_button = glade_xml_get_widget (data->gui, "ep_view_button");
		data->p_search_button = glade_xml_get_widget (data->gui, "ep_search_button");
		data->p_cancel_button = glade_xml_get_widget (data->gui, "ep_cancel_button");
		data->p_close_button = glade_xml_get_widget (data->gui, "ep_close_button");
		data->p_searching_in_hbox = glade_xml_get_widget (data->gui, "ep_searching_in_hbox");
		data->p_images_label = glade_xml_get_widget (data->gui, "ep_images_label");
		progress_hbox = glade_xml_get_widget (data->gui, "ep_progress_hbox");
	}

	data->file_list = gth_file_list_new ();
	gtk_widget_show_all (data->file_list->root_widget);
	gtk_box_pack_start (GTK_BOX (progress_hbox), data->file_list->root_widget, TRUE, TRUE, 0);

	/* Set widgets data. */

	if (catalog_path == NULL) {
		char *esc_uri = NULL;

		esc_uri = gnome_vfs_escape_host_and_path_string (gth_browser_get_current_directory (data->browser));
		gtk_file_chooser_set_uri (GTK_FILE_CHOOSER (data->s_start_from_filechooserbutton), esc_uri);
		g_free (esc_uri);

		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->s_include_subfold_checkbutton), eel_gconf_get_boolean (PREF_SEARCH_RECURSIVE, TRUE));

	} else {
		Catalog    *catalog;
		SearchData *search_data;
		char       *esc_uri = NULL;

		catalog = catalog_new ();
		catalog_load_from_disk (catalog, data->catalog_path, NULL);

		search_data = catalog->search_data;

		free_search_criteria_data (data);
		search_data_free (data->search_data);
		data->search_data = search_data_new ();
		search_data_copy (data->search_data, search_data);

		data->all_keywords =  data->search_data->all_keywords;
		data->file_patterns = search_util_get_file_patterns (data->search_data->file_pattern);
		data->comment_patterns = search_util_get_patterns (data->search_data->comment_pattern);
		data->place_patterns = search_util_get_patterns (data->search_data->place_pattern);
		data->keywords_patterns = search_util_get_patterns (data->search_data->keywords_pattern);

		/**/

		esc_uri = gnome_vfs_escape_host_and_path_string (search_data->start_from);
		gtk_file_chooser_set_uri (GTK_FILE_CHOOSER (data->s_start_from_filechooserbutton), esc_uri);
		g_free (esc_uri);

		/**/

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

		catalog_free (catalog);
	}

	/**/

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
	g_signal_connect (G_OBJECT (data->s_date_optionmenu),
			  "changed",
			  G_CALLBACK (date_option_changed_cb),
			  data);

	g_signal_connect (G_OBJECT (data->dialog),
			  "response",
			  G_CALLBACK (response_cb),
			  data);

	/* Run dialog. */

	gtk_window_set_transient_for (GTK_WINDOW (data->search_progress_dialog),
				      GTK_WINDOW (browser));
	gtk_window_set_transient_for (GTK_WINDOW (data->dialog),
				      GTK_WINDOW (browser));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), TRUE);

	gtk_widget_grab_focus (data->s_filename_entry);

	if (! start_search)
		gtk_widget_show (data->dialog);
	else
		g_idle_add (idle_start_search_cb, data);
}


/* --------------------------- */


static gboolean
pattern_matched_by_keywords (char  *pattern,
			     char **keywords)
{
	GPatternSpec *spec;
	gboolean      retval = FALSE;
	int           i;

	if (pattern == NULL)
		return TRUE;

	if ((keywords == NULL) || (keywords[0] == NULL))
		return FALSE;

	spec = g_pattern_spec_new (pattern);
	for (i = 0; keywords[i] != NULL; i++) {
		char     *case_string;
		gboolean  match;

		case_string = g_utf8_casefold (keywords[i], -1);
		match = g_pattern_match_string (spec, case_string);
		g_free (case_string);

		if (match) {
			retval = TRUE;
			break;
		}
	}
	g_pattern_spec_free (spec);

	return retval;
}


static gboolean
match_patterns (char       **patterns,
		const char  *string)
{
	char     *case_string;
	int       i;
	gboolean  retval = FALSE;

	if ((patterns == NULL) || (patterns[0] == NULL))
		return TRUE;

	if (string == NULL)
		return FALSE;

	case_string = g_utf8_casefold (string, -1);
	for (i = 0; patterns[i] != NULL; i++) {
		if (g_pattern_match_simple (patterns[i], case_string)) {
			retval = TRUE;
			break;
		}
	}
	g_free (case_string);

	return retval;
}


static void
load_parents_comments (DialogData *data,
		       const char *filename)
{
	char *parent = g_strdup (filename);

	do {
		char *tmp = parent;

		parent = remove_level_from_path (tmp);
		g_free (tmp);

		if (parent == NULL)
			break;

		if (g_hash_table_lookup (data->folders_comment, parent) == NULL) {
			CommentData *comment_data = comments_load_comment (parent, FALSE);
			if (comment_data == NULL)
				comment_data = comment_data_new ();

			g_hash_table_insert (data->folders_comment,
					     g_strdup (parent),
					     comment_data);
		}

	} while (! uri_is_root (parent));

	g_free (parent);
}


static void
add_parents_comments (CommentData *comment_data,
		      DialogData  *data,
		      const char  *filename)
{
	char *parent = g_strdup (filename);

	do {
		char        *tmp = parent;
		CommentData *parent_data;

		parent = remove_level_from_path (tmp);
		g_free (tmp);

		if (parent == NULL)
			break;

		parent_data = g_hash_table_lookup (data->folders_comment, parent);

		if (parent_data != NULL) {
			int i;
			for (i = 0; i < parent_data->keywords_n; i++)
				comment_data_add_keyword (comment_data, parent_data->keywords[i]);
		}

	} while (! uri_is_root (parent));

	g_free (parent);
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
	time_t       time = 0;
	const char  *name_only;

	if (! file_is_image (filename, eel_gconf_get_boolean (PREF_FAST_FILE_TYPE, TRUE)))
		return FALSE;

	load_parents_comments (data, filename);

	comment_data = comments_load_comment (filename, FALSE);
	if (comment_data == NULL)
		comment_data = comment_data_new ();
	add_parents_comments (comment_data, data, filename);

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

	if (time == 0)
		time = get_file_mtime (filename);

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
		 && (time >= data->search_data->date)
		 && (time <= data->search_data->date + ONE_DAY))
		match_date = TRUE;
	else if ((data->search_data->date_scope == DATE_AFTER)
		 && (time != 0)
		 && (time > data->search_data->date + ONE_DAY))
		match_date = TRUE;

	name_only = filename + strlen (data->search_data->start_from);
	result = (match_patterns (data->file_patterns, name_only)
		  && match_patterns (data->comment_patterns, comment)
		  && match_patterns (data->place_patterns, place)
		  && match_keywords
		  && match_date);

	comment_data_free (comment_data);

	return result;
}


static void
add_file_list (DialogData *data,
	       GList      *file_list)
{
	char *images;

	data->files = g_list_concat (data->files, file_list);

	images = g_strdup_printf ("%d", g_list_length (data->files));
	gtk_label_set_text (GTK_LABEL (data->p_images_label), images);
	g_free (images);

	gth_file_list_add_list (data->file_list, file_list);
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
			str_uri = gnome_vfs_uri_to_string (full_uri, GNOME_VFS_URI_HIDE_NONE);
			unesc_uri = gnome_vfs_unescape_string (str_uri, NULL);

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

			data->dirs = g_list_prepend (data->dirs, unesc_uri);
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
search_dir_async (DialogData *data,
		  char       *dir)
{
	_gtk_entry_set_filename_text (GTK_ENTRY (data->p_current_dir_entry), dir);

	/**/

	if (data->uri != NULL)
		gnome_vfs_uri_unref (data->uri);
	data->uri = new_uri_from_path (dir);

	gnome_vfs_async_load_directory_uri (
		& (data->handle),
		data->uri,
		GNOME_VFS_FILE_INFO_FOLLOW_LINKS,
		128 /* items_per_notification FIXME */,
		GNOME_VFS_PRIORITY_DEFAULT,
		directory_load_cb,
		data);
}


/* FIXME
static gboolean
empty_pattern (const char *pattern)
{
	return (pattern == NULL) || (pattern[0] == 0);
}
*/


static void
search_images_async (DialogData *data)
{
	SearchData *search_data = data->search_data;

	free_search_results_data (data);
	gth_file_list_set_list (data->file_list, NULL, pref_get_arrange_type (), pref_get_sort_order ());

	search_dir_async (data, search_data->start_from);
}





typedef struct {
	DialogData     *data;
	GladeXML       *gui;

	GtkWidget      *dialog;
	GtkWidget      *c_categories_entry;
	GtkWidget      *c_categories_treeview;
	GtkWidget      *c_ok_button;
	GtkWidget      *c_cancel_button;
	GtkWidget      *s_at_least_one_cat_radiobutton;
	GtkWidget      *s_all_cat_radiobutton;

	GtkListStore   *c_categories_list_model;
} CategoriesDialogData;


/* called when the main dialog is closed. */
static void
categories_dialog__destroy_cb (GtkWidget            *widget,
			       CategoriesDialogData *cdata)
{
	g_object_unref (cdata->gui);
	g_free (cdata);
}


static void
update_category_entry (CategoriesDialogData *cdata)
{
	GtkTreeIter   iter;
	GtkTreeModel *model = GTK_TREE_MODEL (cdata->c_categories_list_model);
	GString      *categories;

	if (! gtk_tree_model_get_iter_first (model, &iter)) {
		gtk_entry_set_text (GTK_ENTRY (cdata->c_categories_entry), "");
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

	gtk_entry_set_text (GTK_ENTRY (cdata->c_categories_entry), categories->str);
	g_string_free (categories, TRUE);
}


static GList *
get_categories_from_entry (CategoriesDialogData *cdata)
{
	GList       *cat_list = NULL;
	const char  *utf8_text;
	char       **categories;
	int          i;

	utf8_text = gtk_entry_get_text (GTK_ENTRY (cdata->c_categories_entry));
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
add_saved_categories (CategoriesDialogData *cdata,
		      GList                *cat_list)
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

		gtk_list_store_append (cdata->c_categories_list_model, &iter);

		gtk_list_store_set (cdata->c_categories_list_model, &iter,
				    C_USE_CATEGORY_COLUMN, FALSE,
				    C_CATEGORY_COLUMN, category1,
				    -1);
	}

	bookmarks_free (categories);
}


static void
update_list_from_entry (CategoriesDialogData *cdata)
{
	GList *categories = NULL;
	GList *scan;

	categories = get_categories_from_entry (cdata);

	gtk_list_store_clear (cdata->c_categories_list_model);
	for (scan = categories; scan; scan = scan->next) {
		char        *category = scan->data;
		GtkTreeIter  iter;

		gtk_list_store_append (cdata->c_categories_list_model, &iter);

		gtk_list_store_set (cdata->c_categories_list_model, &iter,
				    C_USE_CATEGORY_COLUMN, TRUE,
				    C_CATEGORY_COLUMN, category,
				    -1);
	}
	add_saved_categories (cdata, categories);
	path_list_free (categories);
}


static void
use_category_toggled (GtkCellRendererToggle *cell,
		      gchar                 *path_string,
		      gpointer               callback_data)
{
	CategoriesDialogData *cdata  = callback_data;
	GtkTreeModel *model = GTK_TREE_MODEL (cdata->c_categories_list_model);
	GtkTreeIter   iter;
	GtkTreePath  *path = gtk_tree_path_new_from_string (path_string);
	gboolean      value;

	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_model_get (model, &iter, C_USE_CATEGORY_COLUMN, &value, -1);

	value = !value;
	gtk_list_store_set (GTK_LIST_STORE (model), &iter, C_USE_CATEGORY_COLUMN, value, -1);

	gtk_tree_path_free (path);
	update_category_entry (cdata);
}


static void
choose_categories_ok_cb (GtkWidget            *widget,
			 CategoriesDialogData *cdata)
{
	const char *categories;

	categories = gtk_entry_get_text (GTK_ENTRY (cdata->c_categories_entry));
	gtk_entry_set_text (GTK_ENTRY (cdata->data->s_categories_entry), categories);
	cdata->data->all_keywords = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (cdata->s_all_cat_radiobutton));
	gtk_widget_destroy (cdata->dialog);
}


static void
choose_categories_dialog (DialogData *data)
{
	CategoriesDialogData *cdata;
	GtkCellRenderer      *renderer;
	GtkTreeViewColumn    *column;

	cdata = g_new (CategoriesDialogData, 1);
	cdata->data = data;

	cdata->gui = glade_xml_new (GTHUMB_GLADEDIR "/" SEARCH_GLADE_FILE,
				    NULL,
				    NULL);

	if (! cdata->gui) {
		g_free (cdata);
		g_warning ("Could not find " SEARCH_GLADE_FILE "\n");
		return;
	}

	/* Get the widgets. */

	cdata->dialog = glade_xml_get_widget (cdata->gui, "categories_dialog");
	cdata->c_categories_entry = glade_xml_get_widget (cdata->gui, "c_categories_entry");
	cdata->c_categories_treeview = glade_xml_get_widget (cdata->gui, "c_categories_treeview");
	cdata->c_ok_button = glade_xml_get_widget (cdata->gui, "c_ok_button");
	cdata->c_cancel_button = glade_xml_get_widget (cdata->gui, "c_cancel_button");
	cdata->s_at_least_one_cat_radiobutton = glade_xml_get_widget (cdata->gui, "s_at_least_one_cat_radiobutton");
	cdata->s_all_cat_radiobutton = glade_xml_get_widget (cdata->gui, "s_all_cat_radiobutton");

	/* Set widgets data. */

	cdata->c_categories_list_model = gtk_list_store_new (C_NUM_COLUMNS,
							     G_TYPE_BOOLEAN,
							     G_TYPE_STRING);
	gtk_tree_view_set_model (GTK_TREE_VIEW (cdata->c_categories_treeview),
				 GTK_TREE_MODEL (cdata->c_categories_list_model));
	g_object_unref (cdata->c_categories_list_model);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (cdata->c_categories_treeview), FALSE);

	renderer = gtk_cell_renderer_toggle_new ();
	g_signal_connect (G_OBJECT (renderer),
			  "toggled",
			  G_CALLBACK (use_category_toggled),
			  cdata);
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (cdata->c_categories_treeview),
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
	gtk_tree_view_append_column (GTK_TREE_VIEW (cdata->c_categories_treeview),
				     column);

	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (cdata->c_categories_list_model), C_CATEGORY_COLUMN, GTK_SORT_ASCENDING);

	gtk_entry_set_text (GTK_ENTRY (cdata->c_categories_entry), gtk_entry_get_text (GTK_ENTRY (cdata->data->s_categories_entry)));
	update_list_from_entry (cdata);

	if (data->all_keywords)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cdata->s_all_cat_radiobutton), TRUE);
	else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cdata->s_at_least_one_cat_radiobutton), TRUE);

	/* Set the signals handlers. */

	g_signal_connect (G_OBJECT (cdata->dialog),
			  "destroy",
			  G_CALLBACK (categories_dialog__destroy_cb),
			  cdata);
	g_signal_connect (G_OBJECT (cdata->c_ok_button),
			  "clicked",
			  G_CALLBACK (choose_categories_ok_cb),
			  cdata);
	g_signal_connect_swapped (G_OBJECT (cdata->c_cancel_button),
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  G_OBJECT (cdata->dialog));

	/* Run dialog. */

	gtk_widget_grab_focus (cdata->c_categories_treeview);

	gtk_window_set_transient_for (GTK_WINDOW (cdata->dialog),
				      GTK_WINDOW (data->dialog));
	gtk_window_set_modal (GTK_WINDOW (cdata->dialog), TRUE);

	gtk_widget_show (cdata->dialog);
}
