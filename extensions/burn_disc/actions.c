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
		BraseroTrackDataCfg *track;
		GList               *sidecars;
		GList               *scan;
		BraseroSessionCfg   *session;
		GtkWidget           *dialog;
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

		sidecars = NULL;
		for (scan = file_list; scan; scan = scan->next) {
			GthFileData *file_data = scan->data;
			gth_hook_invoke ("add-sidecars", file_data->file, &sidecars);
		}
		sidecars = g_list_reverse (sidecars);

		for (scan = sidecars; scan; scan = scan->next) {
			GFile *file = scan->data;
			char  *uri;

			uri = g_file_get_uri (file);
			brasero_track_data_cfg_add (track, uri, NULL);

			g_free (uri);
		}

		dialog = brasero_burn_options_new (session);
		gtk_window_set_icon_name (GTK_WINDOW (dialog), gtk_window_get_icon_name (GTK_WINDOW (browser)));
		gtk_window_set_title (GTK_WINDOW (dialog), _("Write to Disc"));
		gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (browser));
		gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
		result = gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);

		if (result == GTK_RESPONSE_OK) {
			dialog = brasero_burn_dialog_new ();
			gtk_window_set_icon_name (GTK_WINDOW (dialog), gtk_window_get_icon_name (GTK_WINDOW (browser)));
			gtk_window_set_title (GTK_WINDOW (dialog), _("Write to Disc"));
			brasero_session_cfg_disable (session);
			gtk_window_present (GTK_WINDOW (dialog));
			brasero_burn_dialog_run (BRASERO_BURN_DIALOG (dialog),
						 BRASERO_BURN_SESSION (session));

			gtk_widget_destroy (dialog);
		}

		g_object_unref (session);
	}

	_g_object_list_unref (file_list);
	_gtk_tree_path_list_free (items);
}
