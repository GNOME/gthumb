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


static void
label_entry_changed_cb (GtkEntry           *entry,
			BraseroBurnSession *session)
{
	brasero_burn_session_set_label (session, gtk_entry_get_text (entry));
}


void
gth_browser_activate_action_burn_disc (GtkAction  *action,
				       GthBrowser *browser)
{
	static gboolean  initialized = FALSE;
	GList           *items;
	GList           *file_list;

	if (! initialized) {
		brasero_media_library_start ();
		brasero_burn_library_start (NULL, NULL);
		initialized = TRUE;
	}

	items = gth_file_selection_get_selected (GTH_FILE_SELECTION (gth_browser_get_file_list_view (browser)));
	if ((items == NULL) || (items->next == NULL))
		file_list = gth_file_store_get_visibles (GTH_FILE_STORE (gth_browser_get_file_store (browser)));
	else
		file_list = gth_file_list_get_files (GTH_FILE_LIST (gth_browser_get_file_list (browser)), items);

	{
		BraseroSessionCfg   *session;
		BraseroTrackDataCfg *track;
		GList               *scan;
		GHashTable          *parents;
		GtkWidget           *dialog;
		GtkBuilder          *builder;
		GtkWidget           *options;
		GtkResponseType      result;

		session = brasero_session_cfg_new ();
		track = brasero_track_data_cfg_new ();
		brasero_burn_session_add_track (BRASERO_BURN_SESSION (session),
						BRASERO_TRACK (track),
						NULL);
		g_object_unref (track);

		for (scan = file_list; scan; scan = scan->next) {
			GthFileData *file_data = scan->data;
			char        *uri;

			uri = g_file_get_uri (file_data->file);
			brasero_track_data_cfg_add (track, uri, NULL);

			g_free (uri);
		}

		parents = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, (GDestroyNotify) gtk_tree_path_free);
		for (scan = file_list; scan; scan = scan->next) {
			GthFileData *file_data = scan->data;
			GFile       *parent;
			GList       *file_sidecars = NULL;
			GList       *scan_sidecars;

			gth_hook_invoke ("add-sidecars", file_data->file, &file_sidecars);
			parent = g_file_get_parent (file_data->file);
			for (scan_sidecars = file_sidecars; scan_sidecars; scan_sidecars = scan_sidecars->next) {
				GFile       *sidecar = scan_sidecars->data;
				char        *relative_path;
				char        *parent_path;
				GtkTreePath *tree_path;
				char        *uri;

				relative_path = g_file_get_relative_path (parent, sidecar);
				parent_path = _g_uri_get_parent (relative_path);

				if (g_strcmp0 (parent_path, "") == 0) {
					g_free (parent_path);
					parent_path = NULL;
				}

				if (parent_path != NULL) {
					char **subfolders;
					int    i;
					char  *subfolder;

					/* add all the subfolders to the track data */

					subfolder = NULL;
					subfolders = g_strsplit (parent_path, "/", -1);
					for (i = 0; subfolders[i] != NULL; i++) {
						char *subfolder_parent;

						subfolder_parent = subfolder;
						if (subfolder_parent != NULL)
							subfolder = g_strconcat (subfolder_parent, "/", subfolders[i], NULL);
						else
							subfolder = g_strdup (subfolders[i]);

						tree_path = g_hash_table_lookup (parents, subfolder);
						if (tree_path == NULL) {
							GtkTreePath *subfolder_parent_tpath;
							GtkTreePath *subfolder_tpath;

							if (subfolder_parent != NULL)
								subfolder_parent_tpath = g_hash_table_lookup (parents, subfolder_parent);
							else
								subfolder_parent_tpath = NULL;
							subfolder_tpath = brasero_track_data_cfg_add_empty_directory (track, parent_path, subfolder_parent_tpath);
							g_hash_table_insert (parents, g_strdup (parent_path), subfolder_tpath);
						}

						g_free (subfolder_parent);
					}

					g_strfreev (subfolders);
				}

				tree_path = NULL;
				if (parent_path != NULL)
					tree_path = g_hash_table_lookup (parents, parent_path);

				uri = g_file_get_uri (sidecar);
				brasero_track_data_cfg_add (track, uri, tree_path);

				g_free (uri);
				g_free (parent_path);
				g_free (relative_path);
			}

			g_object_unref (parent);
			_g_object_list_unref (file_sidecars);
		}

		dialog = brasero_burn_options_new (session);
		gtk_window_set_icon_name (GTK_WINDOW (dialog), gtk_window_get_icon_name (GTK_WINDOW (browser)));
		gtk_window_set_title (GTK_WINDOW (dialog), _("Write to Disc"));
		gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (browser));

		builder = _gtk_builder_new_from_file ("burn-disc-options.ui", "burn_disc");
		options = _gtk_builder_get_widget (builder, "options");
		gtk_entry_set_text (GTK_ENTRY (_gtk_builder_get_widget (builder, "label_entry")),
				    g_file_info_get_display_name (gth_browser_get_location_data (browser)->info));
		g_signal_connect (_gtk_builder_get_widget (builder, "label_entry"),
				  "changed",
				  G_CALLBACK (label_entry_changed_cb),
				  session);
		gtk_widget_show (options);
		brasero_burn_options_add_options (BRASERO_BURN_OPTIONS (dialog), options);

		result = gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);

		if (result == GTK_RESPONSE_OK) {
			dialog = brasero_burn_dialog_new ();
			gtk_window_set_icon_name (GTK_WINDOW (dialog), gtk_window_get_icon_name (GTK_WINDOW (browser)));
			gtk_window_set_title (GTK_WINDOW (dialog), _("Write to Disc"));
			brasero_session_cfg_disable (session);
			gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (browser));
			gtk_window_present (GTK_WINDOW (dialog));
			brasero_burn_dialog_run (BRASERO_BURN_DIALOG (dialog),
						 BRASERO_BURN_SESSION (session));

			gtk_widget_destroy (dialog);
		}

		g_hash_table_unref (parents);
		g_object_unref (session);
	}

	_g_object_list_unref (file_list);
	_gtk_tree_path_list_free (items);
}
