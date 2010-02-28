/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2010 Free Software Foundation, Inc.
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
#include <brasero/brasero-burn-dialog.h>
#include <brasero/brasero-burn-lib.h>
#include <brasero/brasero-burn-options.h>
#include <brasero/brasero-media.h>
#include <brasero/brasero-session.h>
#include <brasero/brasero-session-cfg.h>
#include <brasero/brasero-track-data-cfg.h>
#include <glib/gi18n.h>
#include <gthumb.h>


typedef struct {
	GthBrowser    *browser;
	GFile         *location;
	GList         *files;
	GtkWidget     *dialog;
	GtkBuilder    *builder;
	GthTest       *test;
	GthFileSource *file_source;
	char          *base_directory;
	char          *current_directory;
	GHashTable    *content;
	GHashTable *parents;
	BraseroSessionCfg   *session;
	BraseroTrackDataCfg *track;
} BurnData;


static void
free_file_list_from_content (gpointer key,
			     gpointer value,
			     gpointer user_data)
{
	_g_object_list_unref (value);
}


static void
burn_data_free (BurnData *data)
{
	g_hash_table_foreach (data->content, free_file_list_from_content, NULL);
	g_hash_table_unref (data->content);
	g_hash_table_unref (data->parents);
	g_free (data->current_directory);
	_g_object_unref (data->file_source);
	_g_object_unref (data->test);
	_g_object_unref (data->builder);
	_g_object_list_unref (data->files);
	g_free (data->base_directory);
	g_object_unref (data->location);
	g_object_unref (data->browser);
	g_free (data);
}


static void
add_file_to_track (BurnData   *data,
		   const char *parent_uri,
		   const char *relative_subfolder,
		   GFile      *file)
{
	char        *relative_parent;
	GtkTreePath *tree_path;
	char        *uri;

	relative_parent = g_build_path ("/", parent_uri + strlen (data->base_directory), relative_subfolder, NULL);
	if (relative_parent != NULL) {
		char **subfolders;
		int    i;
		char  *subfolder;

		/* add all the subfolders to the track data */

		subfolder = NULL;
		subfolders = g_strsplit (relative_parent, "/", -1);
		for (i = 0; subfolders[i] != NULL; i++) {
			char *subfolder_parent;

			subfolder_parent = subfolder;
			if (subfolder_parent != NULL)
				subfolder = g_strconcat (subfolder_parent, "/", subfolders[i], NULL);
			else
				subfolder = g_strdup (subfolders[i]);

			if ((strcmp (subfolder, "") != 0) && g_hash_table_lookup (data->parents, subfolder) == NULL) {
				GtkTreePath *subfolder_parent_tpath;
				GtkTreePath *subfolder_tpath;

				if (subfolder_parent != NULL)
					subfolder_parent_tpath = g_hash_table_lookup (data->parents, subfolder_parent);
				else
					subfolder_parent_tpath = NULL;
				subfolder_tpath = brasero_track_data_cfg_add_empty_directory (data->track, _g_uri_get_basename (subfolder), subfolder_parent_tpath);
				g_hash_table_insert (data->parents, g_strdup (subfolder), subfolder_tpath);
			}

			g_free (subfolder_parent);
		}

		g_free (subfolder);
		g_strfreev (subfolders);
	}

	tree_path = NULL;
	if (relative_parent != NULL)
		tree_path = g_hash_table_lookup (data->parents, relative_parent);
	uri = g_file_get_uri (file);
	brasero_track_data_cfg_add (data->track, uri, tree_path);

	g_free (uri);
	g_free (relative_parent);
}


static void
add_content_list (gpointer key,
		  gpointer value,
		  gpointer user_data)
{
	BurnData *data = user_data;
	char     *parent_uri = key;
	GList    *files = value;
	GList    *scan;

	for (scan = files; scan; scan = scan->next)
		add_file_to_track (data, parent_uri, NULL, (GFile *) scan->data);

	for (scan = files; scan; scan = scan->next) {
		GFile *file = scan->data;
		GFile *file_parent;
		GList *file_sidecars = NULL;
		GList *scan_sidecars;

		file_parent = g_file_get_parent (file);
		gth_hook_invoke ("add-sidecars", file, &file_sidecars);
		for (scan_sidecars = file_sidecars; scan_sidecars; scan_sidecars = scan_sidecars->next) {
			GFile *sidecar = scan_sidecars->data;
			char  *relative_path;
			char  *subfolder_path;

			if (! g_file_query_exists (sidecar, NULL))
				continue;

			relative_path = g_file_get_relative_path (file_parent, sidecar);
			subfolder_path = _g_uri_get_parent (relative_path);
			if (g_strcmp0 (subfolder_path, "") == 0) {
				g_free (subfolder_path);
				subfolder_path = NULL;
			}
			add_file_to_track (data, parent_uri, subfolder_path, sidecar);
		}

		_g_object_list_unref (file_sidecars);
		g_object_unref (file_parent);
	}
}


static void
label_entry_changed_cb (GtkEntry           *entry,
			BraseroBurnSession *session)
{
	brasero_burn_session_set_label (session, gtk_entry_get_text (entry));
}


static void
burn_content_to_disc (BurnData *data)
{
	static gboolean  initialized = FALSE;
	GtkWidget       *dialog;
	GtkBuilder      *builder;
	GtkWidget       *options;
	GtkResponseType  result;

	if (! initialized) {
		brasero_media_library_start ();
		brasero_burn_library_start (NULL, NULL);
		initialized = TRUE;
	}

	data->session = brasero_session_cfg_new ();
	data->track = brasero_track_data_cfg_new ();
	brasero_burn_session_add_track (BRASERO_BURN_SESSION (data->session),
					BRASERO_TRACK (data->track),
					NULL);
	g_object_unref (data->track);

	g_hash_table_foreach (data->content, add_content_list, data);

	dialog = brasero_burn_options_new (data->session);
	gtk_window_set_icon_name (GTK_WINDOW (dialog), gtk_window_get_icon_name (GTK_WINDOW (data->browser)));
	gtk_window_set_title (GTK_WINDOW (dialog), _("Write to Disc"));
	gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (data->browser));

	builder = _gtk_builder_new_from_file ("burn-disc-options.ui", "burn_disc");
	options = _gtk_builder_get_widget (builder, "options");
	gtk_entry_set_text (GTK_ENTRY (_gtk_builder_get_widget (builder, "label_entry")),
			    g_file_info_get_display_name (gth_browser_get_location_data (data->browser)->info));
	g_signal_connect (_gtk_builder_get_widget (builder, "label_entry"),
			  "changed",
			  G_CALLBACK (label_entry_changed_cb),
			  data->session);
	gtk_widget_show (options);
	brasero_burn_options_add_options (BRASERO_BURN_OPTIONS (dialog), options);

	result = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);

	if (result == GTK_RESPONSE_OK) {
		dialog = brasero_burn_dialog_new ();
		gtk_window_set_icon_name (GTK_WINDOW (dialog), gtk_window_get_icon_name (GTK_WINDOW (data->browser)));
		gtk_window_set_title (GTK_WINDOW (dialog), _("Write to Disc"));
		brasero_session_cfg_disable (data->session);
		gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (data->browser));
		gtk_window_present (GTK_WINDOW (dialog));
		brasero_burn_dialog_run (BRASERO_BURN_DIALOG (dialog),
					 BRASERO_BURN_SESSION (data->session));

		gtk_widget_destroy (dialog);
	}

	g_object_unref (data->session);
}


static void
done_func (GObject  *object,
	   GError   *error,
	   gpointer  user_data)
{
	BurnData *data = user_data;

	if (error != NULL) {
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (data->browser), _("Could not get the file list"), &error);
		burn_data_free (data);
		return;
	}

	burn_content_to_disc (data);
	burn_data_free (data);
}


static void
for_each_file_func (GFile     *file,
		    GFileInfo *info,
		    gpointer   user_data)
{
	BurnData    *data = user_data;
	GthFileData *file_data;

	if (g_file_info_get_file_type (info) != G_FILE_TYPE_REGULAR)
		return;

	file_data = gth_file_data_new (file, info);
	if (gth_test_match (data->test, file_data)) {
		GList *list;

		list = g_hash_table_lookup (data->content, data->current_directory);
		list = g_list_prepend (list, g_file_dup (file));
		g_hash_table_insert (data->content, g_strdup (data->current_directory), list);
	}

	g_object_unref (file_data);
}


static DirOp
start_dir_func (GFile      *directory,
		GFileInfo  *info,
		GError    **error,
		gpointer    user_data)
{
	BurnData *data = user_data;
	GFile    *parent;
	char     *escaped;
	GFile    *destination;
	char     *uri;

	g_free (data->current_directory);

	parent = g_file_get_parent (directory);
	escaped = _g_replace (g_file_info_get_display_name (info), "/", "-");
	destination = g_file_get_child_for_display_name (parent, escaped, NULL);
	uri = g_file_get_uri (destination);
	data->current_directory = g_uri_unescape_string (uri, NULL);
	g_hash_table_insert (data->content, g_strdup (data->current_directory), NULL);

	g_free (uri);
	g_object_unref (destination);
	g_free (escaped);
	g_object_unref (parent);

	return DIR_OP_CONTINUE;
}


static void
source_dialog_response_cb (GtkDialog *dialog,
			   int        response,
			   BurnData  *data)
{
	gtk_widget_hide (data->dialog);

	if (response == GTK_RESPONSE_OK) {
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (_gtk_builder_get_widget (data->builder, "selection_radiobutton")))) {
			g_hash_table_replace (data->content, g_file_get_uri (data->location), g_list_reverse (data->files));
			data->files = NULL;
			burn_content_to_disc (data);
			burn_data_free (data);
		}
		else {
			gboolean recursive;

			_g_object_list_unref (data->files);
			data->files = NULL;

			recursive = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (_gtk_builder_get_widget (data->builder, "folder_recursive_radiobutton")));
			data->test = gth_main_get_general_filter ();
			data->file_source = gth_main_get_file_source (data->location);
			gth_file_source_for_each_child (data->file_source,
							data->location,
							recursive,
							eel_gconf_get_boolean (PREF_FAST_FILE_TYPE, TRUE) ? GFILE_STANDARD_ATTRIBUTES_WITH_FAST_CONTENT_TYPE : GFILE_STANDARD_ATTRIBUTES_WITH_CONTENT_TYPE,
							start_dir_func,
							for_each_file_func,
							done_func,
							data);
		}
	}
	else
		burn_data_free (data);

	gtk_widget_destroy (data->dialog);
}


void
gth_browser_activate_action_burn_disc (GtkAction  *action,
				       GthBrowser *browser)
{
	GList    *items;
	GList    *file_list;
	BurnData *data;

	items = gth_file_selection_get_selected (GTH_FILE_SELECTION (gth_browser_get_file_list_view (browser)));
	if ((items != NULL) && (items->next != NULL))
		file_list = gth_file_list_get_files (GTH_FILE_LIST (gth_browser_get_file_list (browser)), items);
	else
		file_list = gth_file_store_get_visibles (GTH_FILE_STORE (gth_browser_get_file_store (browser)));

	data = g_new0 (BurnData, 1);
	data->browser = g_object_ref (browser);
	data->location = g_file_dup (gth_browser_get_location (browser));
	data->base_directory = g_file_get_uri (data->location);
	data->files = gth_file_data_list_to_file_list (file_list);
	data->content = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
	data->parents = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, (GDestroyNotify) gtk_tree_path_free);
	data->builder = _gtk_builder_new_from_file ("burn-source-selector.ui", "burn_disc");
	data->dialog = gtk_dialog_new_with_buttons (_("Write to Disc"),
						    GTK_WINDOW (browser),
						    GTK_DIALOG_NO_SEPARATOR,
						    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						    GTK_STOCK_OK, GTK_RESPONSE_OK,
						    NULL);
	gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (data->dialog))), _gtk_builder_get_widget (data->builder, "source_selector"));
	if (items == NULL)
		gtk_widget_set_sensitive (_gtk_builder_get_widget (data->builder, "selection_radiobutton"), FALSE);
	else if ((items != NULL) && (items->next != NULL))
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (_gtk_builder_get_widget (data->builder, "selection_radiobutton")), TRUE);

	g_signal_connect (data->dialog,
			  "response",
			  G_CALLBACK (source_dialog_response_cb),
			  data);
	gtk_widget_show_all (data->dialog);

	_g_object_list_unref (file_list);
	_gtk_tree_path_list_free (items);
}
