/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001-2009 Free Software Foundation, Inc.
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
#include <glib/gprintf.h>
#include <gtk/gtk.h>
#include <unique/unique.h>
#ifdef HAVE_CLUTTER
#include <clutter/clutter.h>
#include <clutter-gtk/clutter-gtk.h>
#endif
#include "eggsmclient.h"
#include "glib-utils.h"
#include "gth-browser.h"
#include "gth-file-data.h"
#include "gth-file-source-vfs.h"
#include "gth-main.h"
#include "gth-preferences.h"


enum {
	COMMAND_UNUSED,
	COMMAND_IMPORT_PHOTOS
};


gboolean NewWindow = FALSE;
gboolean StartInFullscreen = FALSE;
gboolean StartSlideshow = FALSE;
gboolean ImportPhotos = FALSE;


static UniqueApp   *gthumb_app;
static char       **remaining_args;
static const char  *program_argv0; /* argv[0] from main(); used as the command to restart the program */
static gboolean     restart = FALSE;
static gboolean     version = FALSE;


static const GOptionEntry options[] = {
	{ "new-window", 'n', 0, G_OPTION_ARG_NONE, &NewWindow,
	  N_("Open a new window"),
	  0 },

	{ "fullscreen", 'f', 0, G_OPTION_ARG_NONE, &StartInFullscreen,
	  N_("Start in fullscreen mode"),
	  0 },

	{ "slideshow", 's', 0, G_OPTION_ARG_NONE, &StartSlideshow,
	  N_("Automatically start a slideshow"),
	  0 },

	{ "import-photos", 'i', 0, G_OPTION_ARG_NONE, &ImportPhotos,
	  N_("Automatically import digital camera photos"),
	  0 },

	{ "version", 'v', 0, G_OPTION_ARG_NONE, &version,
	  N_("Show version"), NULL },

	{ G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_STRING_ARRAY, &remaining_args,
	  NULL,
	  NULL },

	{ NULL }
};


static void
gth_save_state (EggSMClient *client,
		GKeyFile    *state,
		gpointer     user_data)
{
	const char *argv[2] = { NULL };
	GList      *scan;
	guint       i;

	argv[0] = program_argv0;
	argv[1] = NULL;
	egg_sm_client_set_restart_command (client, 1, argv);

	i = 0;
	for (scan = gth_window_get_window_list (); scan; scan = scan->next) {
		GtkWidget *window = scan->data;
		GFile     *location;
		char      *key;
		char      *uri;

		location = gth_browser_get_location (GTH_BROWSER (window));
		if (location == NULL)
			continue;

		key = g_strdup_printf ("location%d", i++);
		uri = g_file_get_uri (location);
		g_key_file_set_string (state, "Session", key, uri);

		g_free (uri);
		g_free (key);
		g_object_unref (location);
	}

	g_key_file_set_integer (state, "Session", "locations", i);
}


static void
gth_session_manager_init (void)
{
	EggSMClient *client = NULL;

	client = egg_sm_client_get ();
	g_signal_connect (client, "save-state", G_CALLBACK (gth_save_state), NULL);
}


static void
gth_restore_session (EggSMClient *client)
{
	GKeyFile *state = NULL;
	guint i;

	state = egg_sm_client_get_state_file (client);

	i = g_key_file_get_integer (state, "Session", "locations", NULL);
	for (; i > 0; i--) {
		GtkWidget *window;
		char      *key;
		char      *location;

		key = g_strdup_printf ("location%d", i);
		location = g_key_file_get_string (state, "Session", key, NULL);
		g_free (key);

		window = gth_browser_new (location);
		gtk_widget_show (window);

		g_free (location);
	}
}


static void
show_window (GtkWindow  *window,
	     const char *startup_id,
	     GdkScreen  *screen)
{
	gtk_window_set_startup_id (window, startup_id);
	gtk_window_set_screen (window, screen);
	gtk_window_present (window);
}


static UniqueResponse
unique_app_message_received_cb (UniqueApp         *unique_app,
				UniqueCommand      command,
				UniqueMessageData *message,
				guint              time_,
				gpointer           user_data)
{
	UniqueResponse  res;
	char           *uri;
	GFile          *location;
	GtkWidget      *window;

	res = UNIQUE_RESPONSE_OK;

	switch (command) {
	case UNIQUE_OPEN:
	case UNIQUE_NEW:
		if (command == UNIQUE_NEW)
			window = gth_browser_new (NULL);
		else
			window = gth_window_get_current_window ();
		show_window (GTK_WINDOW (window),
			     unique_message_data_get_startup_id (message),
			     unique_message_data_get_screen (message));

		uri = unique_message_data_get_text (message);
		location = g_file_new_for_uri (uri);
		gth_browser_load_location (GTH_BROWSER (window), location);

		g_object_unref (location);
		g_free (uri);
		break;

	case COMMAND_IMPORT_PHOTOS:
		window = gth_window_get_current_window ();
		if (window == NULL)
			window = gth_browser_new (NULL);

		uri = unique_message_data_get_text (message);
		location = g_file_new_for_uri (uri);
		gth_hook_invoke ("import-photos", window, location, NULL);

		g_object_unref (location);
		g_free (uri);
		break;

	default:
		res = UNIQUE_RESPONSE_PASSTHROUGH;
		break;
	}

	return res;
}


static void
open_browser_window (GFile *location)
{
	if (unique_app_is_running (gthumb_app)) {
		UniqueMessageData *data;
		char              *uri;

		data = unique_message_data_new ();
		uri = g_file_get_uri (location);
		unique_message_data_set_text (data, uri, -1);
		unique_app_send_message (gthumb_app, NewWindow ? UNIQUE_NEW : UNIQUE_OPEN, data);

		g_free (uri);
		unique_message_data_free (data);
	}
	else {
		GtkWidget *window;

		window = gth_browser_new (NULL);
		gtk_widget_show (window);
		gth_browser_load_location (GTH_BROWSER (window), location);
	}
}


static void
import_photos_from_location (GFile *location)
{
	if (unique_app_is_running (gthumb_app)) {
		UniqueMessageData *data;
		char              *uri;

		data = unique_message_data_new ();
		uri = g_file_get_uri (location);
		unique_message_data_set_text (data, uri, -1);
		unique_app_send_message (gthumb_app, COMMAND_IMPORT_PHOTOS, data);

		g_free (uri);
		unique_message_data_free (data);
	}
	else {
		GtkWidget *window;

		window = gth_browser_new (NULL);
		gth_hook_invoke ("import-photos", window, location, NULL);
	}
}


static void
prepare_application (void)
{
	EggSMClient *client = NULL;
	const char  *arg;
	int          i;

	gthumb_app = unique_app_new_with_commands ("org.gnome.gthumb", NULL,
						   "import-photos", COMMAND_IMPORT_PHOTOS,
						   NULL);

	if (! unique_app_is_running (gthumb_app)) {
		gth_main_register_default_hooks ();
		gth_main_register_file_source (GTH_TYPE_FILE_SOURCE_VFS);
		gth_main_register_default_sort_types ();
		gth_main_register_default_tests ();
		gth_main_register_default_types ();
		gth_main_register_default_metadata ();
		gth_main_activate_extensions ();
		gth_hook_invoke ("initialize", NULL);
		g_signal_connect (gthumb_app,
				  "message-received",
				  G_CALLBACK (unique_app_message_received_cb),
				  NULL);
	}

	client = egg_sm_client_get ();
	if (egg_sm_client_is_resumed (client)) {
		gth_restore_session (client);
		return;
	}

	if (remaining_args == NULL) { /* No location specified. */
		GFile *location;

		location = g_file_new_for_uri (gth_pref_get_startup_location ());
		open_browser_window (location);

		g_object_unref (location);

		return;
	}

	if (ImportPhotos) {
		GFile *location;

		location = g_file_new_for_commandline_arg (remaining_args[0]);
		import_photos_from_location (location);

		return;
	}

	/* open each location in a new window */

	for (i = 0; (arg = remaining_args[i]) != NULL; i++) {
		GFile *location;

		location = g_file_new_for_commandline_arg (arg);
		open_browser_window (location);

		g_object_unref (location);
	}
}


int
main (int argc, char *argv[])
{
	GOptionContext *context = NULL;
	GError         *error = NULL;

	if (! g_thread_supported ()) {
		g_thread_init (NULL);
		gdk_threads_init ();
	}

	program_argv0 = argv[0];

	/* text domain */

	bindtextdomain (GETTEXT_PACKAGE, GTHUMB_LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	/* command line options */

#ifdef HAVE_CLUTTER
	if (gtk_clutter_init (&argc, &argv) != CLUTTER_INIT_SUCCESS)
		g_error ("Unable to initialize GtkClutter");
#endif

	context = g_option_context_new (N_("- Image browser and viewer"));
	g_option_context_set_translation_domain (context, GETTEXT_PACKAGE);
	g_option_context_add_main_entries (context, options, GETTEXT_PACKAGE);
	g_option_context_add_group (context, gtk_get_option_group (TRUE));
	g_option_context_add_group (context, egg_sm_client_get_option_group ());
	if (! g_option_context_parse (context, &argc, &argv, &error)) {
		g_critical ("Failed to parse arguments: %s", error->message);
		g_error_free (error);
		g_option_context_free (context);
		return EXIT_FAILURE;
	}
	g_option_context_free (context);

	if (version) {
		g_printf ("%s %s, Copyright (C) 2001-2009 Free Software Foundation, Inc.\n", PACKAGE_NAME, PACKAGE_VERSION);
		return 0;
	}

	/* other initializations */

	gth_session_manager_init ();
	gth_pref_initialize ();
	gth_main_initialize ();
	prepare_application ();

	if (! unique_app_is_running (gthumb_app)) {
		gdk_threads_enter ();
		gtk_main ();
		gdk_threads_leave ();
	}

	g_object_unref (gthumb_app);
	gth_main_release ();
	gth_pref_release ();

	if (restart)
		g_spawn_command_line_async (program_argv0, NULL);

	return 0;
}


void
gth_restart (void)
{
	GList *windows;
	GList *scan;

	windows = g_list_copy (gth_window_get_window_list ());
	for (scan = windows; scan; scan = scan->next)
		gth_window_close (GTH_WINDOW (scan->data));
	g_list_free (windows);

	restart = TRUE;
}
