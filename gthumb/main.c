/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001-2010 Free Software Foundation, Inc.
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
#include "main-migrate.h"


enum {
	COMMAND_UNUSED,
	COMMAND_FULLSCREEN,
	COMMAND_IMPORT_PHOTOS,
	COMMAND_SLIDESHOW
};


gboolean NewWindow = FALSE;
gboolean StartInFullscreen = FALSE;
gboolean StartSlideshow = FALSE;
gboolean ImportPhotos = FALSE;
#ifdef HAVE_CLUTTER
int      ClutterInitResult = CLUTTER_INIT_ERROR_UNKNOWN;
#endif


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
migrate_data (void)
{
	migrate_catalogs_from_2_10 ();
}


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
		GtkWidget   *window = scan->data;
		GFile       *location;
		char        *key;
		char        *uri;
		GthFileData *focused_file = NULL;

		focused_file = gth_browser_get_current_file (GTH_BROWSER (window));
		if (focused_file == NULL)
			location = gth_browser_get_location (GTH_BROWSER (window));
		else
			location = focused_file->file;

		if (location == NULL)
			continue;

		key = g_strdup_printf ("location%d", ++i);
		uri = g_file_get_uri (location);
		g_key_file_set_string (state, "Session", key, uri);

		g_free (uri);
		g_free (key);
		g_object_unref (location);
	}

	g_key_file_set_integer (state, "Session", "locations", i);
}


/* quit_requested handler for the master client */


static GList *client_window = NULL;


static void modified_file_saved_cb (GthBrowser  *browser,
				    gboolean     cancelled,
				    gpointer     user_data);


static void
check_whether_to_save (EggSMClient *client)
{
	for (/* void */; client_window; client_window = client_window->next) {
		GtkWidget *window = client_window->data;

		if (gth_browser_get_file_modified (GTH_BROWSER (window))) {
			gth_browser_ask_whether_to_save (GTH_BROWSER (window),
							 modified_file_saved_cb,
							 client);
			return;
		}
	}

	egg_sm_client_will_quit (client, TRUE);
}


static void
modified_file_saved_cb (GthBrowser *browser,
			gboolean    cancelled,
			gpointer    user_data)
{
	EggSMClient *client = user_data;

	if (cancelled) {
		egg_sm_client_will_quit (client, FALSE);
	}
	else {
		client_window = client_window->next;
		check_whether_to_save (client);
	}
}


static void
client_quit_requested_cb (EggSMClient *client,
			  gpointer     data)
{
	client_window = gth_window_get_window_list ();
	check_whether_to_save (client);
}


static void
client_quit_cb (EggSMClient *client,
		gpointer     data)
{
	gtk_main_quit ();
}


static void
gth_session_manager_init (void)
{
	EggSMClient *client = NULL;

	client = egg_sm_client_get ();
	g_signal_connect (client,
			  "save_state",
			  G_CALLBACK (gth_save_state),
			  NULL);
	g_signal_connect (client,
			  "quit_requested",
			  G_CALLBACK (client_quit_requested_cb),
			  NULL);
	g_signal_connect (client,
			  "quit",
			  G_CALLBACK (client_quit_cb),
			  NULL);
}


static void
gth_restore_session (EggSMClient *client)
{
	GKeyFile *state = NULL;
	guint     i;

	state = egg_sm_client_get_state_file (client);

	i = g_key_file_get_integer (state, "Session", "locations", NULL);
	g_assert (i > 0);
	for (; i > 0; i--) {
		GtkWidget *window;
		char      *key;
		char      *location;
		GFile     *file;

		key = g_strdup_printf ("location%d", i);
		location = g_key_file_get_string (state, "Session", key, NULL);
		g_free (key);

		g_assert (location != NULL);

		window = gth_browser_new (NULL);
		gtk_widget_show (window);

		file = g_file_new_for_uri (location);
		gth_browser_load_location(GTH_BROWSER (window), file);

		g_object_unref (file);
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
	GFile          *location = NULL;
	GtkWidget      *window = NULL;

	res = UNIQUE_RESPONSE_OK;

	switch (command) {
	case UNIQUE_OPEN:
	case UNIQUE_NEW:
		if (command == UNIQUE_OPEN)
			window = gth_window_get_current_window ();

		if (window == NULL)
			window = gth_browser_new (NULL);

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
		if ((uri != NULL) && (strcmp (uri, "") != 0))
			location = g_file_new_for_uri (uri);
		gth_hook_invoke ("import-photos", window, location, NULL);

		_g_object_unref (location);
		g_free (uri);
		break;

	case COMMAND_SLIDESHOW:
		window = gth_window_get_current_window ();
		if (window == NULL)
			window = gth_browser_new (NULL);
		gth_hook_invoke ("slideshow", window, NULL);

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
		if (! StartSlideshow)
			gtk_window_present (GTK_WINDOW (window));
		gth_browser_load_location (GTH_BROWSER (window), location);
	}
}


static void
import_photos_from_location (GFile *location)
{
	if (unique_app_is_running (gthumb_app)) {
		UniqueMessageData *data;

		data = unique_message_data_new ();
		if (location != NULL) {
			char *uri;
			uri = g_file_get_uri (location);
			unique_message_data_set_text (data, uri, -1);
			g_free (uri);
		}
		unique_app_send_message (gthumb_app, COMMAND_IMPORT_PHOTOS, data);
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
	GList       *files;
	GList       *dirs;
	GFile       *location;
	GList       *scan;

	gthumb_app = unique_app_new_with_commands ("org.gnome.gthumb", NULL,
						   "import-photos", COMMAND_IMPORT_PHOTOS,
						   NULL);

	gth_main_register_default_hooks ();
	gth_main_register_file_source (GTH_TYPE_FILE_SOURCE_VFS);
	gth_main_register_default_sort_types ();
	gth_main_register_default_tests ();
	gth_main_register_default_types ();
	gth_main_register_default_metadata ();
	gth_main_activate_extensions ();
	gth_hook_invoke ("initialize", NULL);

	if (! unique_app_is_running (gthumb_app))
		g_signal_connect (gthumb_app,
				  "message-received",
				  G_CALLBACK (unique_app_message_received_cb),
				  NULL);

	client = egg_sm_client_get ();
	if (egg_sm_client_is_resumed (client)) {
		gth_restore_session (client);
		return;
	}

	if (ImportPhotos) {
		GFile *location = NULL;

		if (remaining_args != NULL)
			location = g_file_new_for_commandline_arg (remaining_args[0]);
		import_photos_from_location (location);

		return;
	}

	if (remaining_args == NULL) { /* No location specified. */
		GFile *location;

		location = g_file_new_for_uri (gth_pref_get_startup_location ());
		open_browser_window (location);

		g_object_unref (location);

		return;
	}

	/* At least a location was specified */

	files = NULL;
	dirs = NULL;
	for (i = 0; (arg = remaining_args[i]) != NULL; i++) {
		GFile     *location;
		GFileType  file_type;

		location = g_file_new_for_commandline_arg (arg);
		file_type = _g_file_get_standard_type (location);
		if (file_type == G_FILE_TYPE_REGULAR)
			files = g_list_prepend (files, location);
		else
			dirs = g_list_prepend (dirs, location);
	}
	files = g_list_reverse (files);
	dirs = g_list_reverse (dirs);

	location = gth_hook_invoke_get ("command-line-files", files);
	if (location != NULL) {
		open_browser_window (location);
		g_object_unref (location);
	}
	else /* Open each file in a new window */
		for (scan = files; scan; scan = scan->next)
			open_browser_window ((GFile *) scan->data);

	/* Open each dir in a new window */

	for (scan = dirs; scan; scan = scan->next)
		open_browser_window ((GFile *) scan->data);

	_g_object_list_unref (dirs);
	_g_object_list_unref (files);
}


int
main (int argc, char *argv[])
{
	GOptionContext *context;
	GError         *error = NULL;

	if (! g_thread_supported ())
		g_thread_init (NULL);

	program_argv0 = argv[0];

	/* text domain */

	bindtextdomain (GETTEXT_PACKAGE, GTHUMB_LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	/* command line options */

	context = g_option_context_new (N_("- Image browser and viewer"));
	g_option_context_set_translation_domain (context, GETTEXT_PACKAGE);
	g_option_context_add_main_entries (context, options, GETTEXT_PACKAGE);
	g_option_context_add_group (context, gtk_get_option_group (TRUE));
	g_option_context_add_group (context, egg_sm_client_get_option_group ());
#ifdef HAVE_CLUTTER
	g_option_context_add_group (context, cogl_get_option_group ());
	g_option_context_add_group (context, clutter_get_option_group_without_init ());
#endif
	if (! g_option_context_parse (context, &argc, &argv, &error)) {
		g_critical ("Failed to parse arguments: %s", error->message);
		g_error_free (error);
		g_option_context_free (context);
		return EXIT_FAILURE;
	}

	if (version) {
		g_printf ("%s %s, Copyright © 2001-2010 Free Software Foundation, Inc.\n", PACKAGE_NAME, PACKAGE_VERSION);
		return 0;
	}

#ifdef HAVE_CLUTTER
	ClutterInitResult = gtk_clutter_init (NULL, NULL);
#endif

	/* other initializations */

	gth_session_manager_init ();
	gth_pref_initialize ();
	migrate_data ();
	gth_main_initialize ();
	prepare_application ();

	g_option_context_free (context);

	if (! unique_app_is_running (gthumb_app))
		gtk_main ();

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
