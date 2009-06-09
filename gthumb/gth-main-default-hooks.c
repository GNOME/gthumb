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
#include <glib/gi18n.h>
#include "gth-hook.h"
#include "gth-main.h"


void
gth_main_register_default_hooks (void)
{
	gth_hooks_initialize ();

	/**
	 * Called after the window has been initialized.  Can be used by
	 * an extension to create and attach specific data to the window.
	 *
	 * @browser (GthBrowser*): the relative window.
	 **/
	gth_hook_register ("gth-browser-construct", 1);

	/**
	 * Called before closing a window.
	 *
	 * @browser (GthBrowser*): the window to be closed.
	 **/
	gth_hook_register ("gth-browser-close", 1);

	/**
	 * Called before closing the last window and after the
	 * 'gth-browser-close' hook functions.
	 *
	 * @browser (GthBrowser*): the window to be closed.
	 **/
	gth_hook_register ("gth-browser-close-last-window", 1);

	/**
	 *  Called when a sensitivity update is requested.
	 *
	 * @browser (GthBrowser*): the relative window.
	 **/
	gth_hook_register ("gth-browser-update-sensitivity", 1);

	/**
	 *  Called when the current page changes.
	 *
	 * @browser (GthBrowser*): the relative window.
	 **/
	gth_hook_register ("gth-browser-set-current-page", 1);

	/**
	 * Called before loading a folder.
	 *
	 * @browser (GthBrowser*): the window
	 * @folder (GFile*): the folder to load
	 **/
	gth_hook_register ("gth-browser-load-location-before", 2);

	/**
	 * Called after the folder has been loaded.
	 *
	 * @browser (GthBrowser*): the window
	 * @folder (GFile*): the loaded folder
	 * @error (GError*): the error or NULL if the folder was loaded
	 * correctly.
	 **/
	gth_hook_register ("gth-browser-load-location-after", 3);

	/**
	 * Called before displaying the folder tree popup menu.
	 *
	 * @browser (GthBrowser*): the relative window.
	 * @file_source (GthFileSource*): the file_source relative to the file.
	 * @file (GFile*): the relative folder or NULL if the button was
	 * pressed in an empty area.
	 **/
	gth_hook_register ("gth-browser-folder-tree-popup-before", 3);

	/**
	 * Called to view file
	 *
	 * @browser (GthBrowser*): the relative window.
	 * @file (GthFileData*): the file
	 **/
	gth_hook_register ("gth-browser-view-file", 2);

	/**
	 * Called in _gdk_pixbuf_save_async
	 *
	 * @data (SavePixbufData*):
	 **/
	gth_hook_register ("save-pixbuf", 1);

	/**
	 * Called when copying files in g_copy_files_async with the
	 * G_FILE_COPY_ALL_METADATA flag activated and when deleting file
	 * with _g_delete_files.  Used to add sidecar files that contain
	 * file metadata.
	 *
	 * @sources (GList *): the original file list, a GFile * list.
	 * @sidecar_sources (GList **): the sidecars list.
	 */
	gth_hook_register ("add-sidecars", 2);

	/**
	 * Called when creating the preferences dialog to add other tabs to
	 * the dialog.
	 *
	 * @dialog (GtkWidget *): the preferences dialog.
	 * @browser (GthBrowser*): the relative window.
	 * @builder (GtkBuilder*): the dialog ui builder.
	 **/
	gth_hook_register ("dlg-preferences-construct", 3);

	/**
	 * Called when to apply the changes from the preferences dialog.
	 *
	 * @dialog (GtkWidget *): the preferences dialog.
	 * @browser (GthBrowser*): the relative window.
	 * @builder (GtkBuilder*): the dialog ui builder.
	 **/
	gth_hook_register ("dlg-preferences-apply", 3);
}
