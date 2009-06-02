/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001, 2003, 2004, 2005 Free Software Foundation, Inc.
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
#include <gnome.h>
#include <glade/glade.h>
#include <gio/gio.h>
#include <libgnomevfs/gnome-vfs-init.h>
#include <libgnomeui/gnome-authentication-manager.h>

#include "catalog.h"
#include "comments.h"
#include "main.h"
#include "file-utils.h"
#include "glib-utils.h"
#include "gconf-utils.h"
#include "gtk-utils.h"
#include "gth-application.h"
#include "gth-browser.h"
#include "gth-browser-actions-callbacks.h"
#include "gth-dir-list.h"
#include "gth-image-list.h"
#include "gth-viewer.h"
#include "gthumb-init.h"
#include "gthumb-stock.h"
#include "image-viewer.h"
#include "pixbuf-utils.h"
#include "preferences.h"
#include "typedefs.h"

#include "icons/pixbufs.h"

typedef enum {
	ICON_NAME_DIRECTORY,
	ICON_NAME_HARDDISK,
	ICON_NAME_HOME,
	ICON_NAME_DESKTOP,
	ICON_NAMES
} IconName;

static char*icon_mime_name[ICON_NAMES] = { "gnome-fs-directory",
					   "gnome-dev-harddisk",
					   "gnome-fs-home",
					   "gnome-fs-desktop" };

GList         *file_urls = NULL, *dir_urls = NULL;
int            n_file_urls, n_dir_urls;
int            StartInFullscreen = FALSE;
int            StartSlideshow = FALSE;
int            ViewFirstImage = FALSE;
int            HideSidebar = FALSE;
gboolean       ExitAll = FALSE;
char          *ImageToDisplay = NULL;
gboolean       FirstStart = TRUE;
gboolean       ImportPhotos = FALSE;
gboolean       UseViewer = FALSE;

static gboolean         view_comline_catalog = FALSE;
static gboolean         view_single_image = FALSE;
static GdkPixbuf       *icon_pixbuf[ICON_NAMES] = { 0 };
static GtkWidget       *first_window = NULL;
static GtkIconTheme    *icon_theme = NULL;
static GOptionContext  *context;
static char           **remaining_args;
static BonoboObject    *gth_application = NULL;

static const GOptionEntry options[] = {
	{ "fullscreen", 'f', 0, G_OPTION_ARG_NONE, &StartInFullscreen,
	  N_("Start in fullscreen mode"),
	  0 },

	{ "slideshow", 's', 0, G_OPTION_ARG_NONE, &StartSlideshow,
	  N_("Automatically start a slideshow"),
	  0 },

	{ "import-photos", '\0', 0, G_OPTION_ARG_NONE, &ImportPhotos,
	  N_("Automatically import digital camera photos"),
	  0 },

	{ "viewer", '\0', 0, G_OPTION_ARG_NONE, &UseViewer,
	  N_("Use the viewer mode to view single images"),
	  0 },

	{ G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_STRING_ARRAY, &remaining_args,
	  NULL,
	  NULL },

	{ NULL }
};


static void
init_icon_pixbufs (void)
{
	int i;
	for (i = 0; i < ICON_NAMES; i++)
		icon_pixbuf[i] = NULL;
}


static void
free_icon_pixbufs (void)
{
	int i;
	for (i = 0; i < ICON_NAMES; i++)
		if (icon_pixbuf[i] != NULL) {
			g_object_unref (icon_pixbuf[i]);
			icon_pixbuf[i] = NULL;
		}
}


static void
theme_changed_cb (GtkIconTheme *theme,
		  gpointer        data)
{
	free_icon_pixbufs ();
	gth_monitor_notify_update_icon_theme ();
}


static void
create_default_tags_if_needed (void)
{
	Bookmarks *tags;
	char      *default_tags[] = { N_("Holidays"),
				      N_("Temporary"),
				      N_("Screenshots"),
				      N_("Science"),
				      N_("Favourite"),
				      N_("Important"),
				      N_("GNOME"),
				      N_("Games"),
				      N_("Party"),
				      N_("Birthday"),
				      N_("Astronomy"),
				      N_("Family"),
				      NULL };
	int        i;
	char      *path;

	path = g_strconcat (g_get_home_dir (),
			    "/",
			    RC_TAGS_FILE,
			    NULL);
	if (path_is_file (path)) {
		g_free (path);
		return;
	}
	g_free (path);

	tags = bookmarks_new (RC_TAGS_FILE);
	for (i = 0; default_tags[i] != NULL; i++)
		bookmarks_add (tags, _(default_tags[i]), TRUE, TRUE);
	bookmarks_write_to_disk (tags);
	bookmarks_free (tags);
}


static void
convert_old_comment (char     *real_file,
		     char     *rc_file,
		     gpointer  data)
{
	char *comment_file;
	char *comment_dir;

	comment_file = comments_get_comment_filename (real_file, TRUE);
	comment_dir = remove_level_from_path (comment_file);
	ensure_dir_exists (comment_dir);

	file_copy (rc_file, comment_file, TRUE, NULL);

	g_free (comment_dir);
	g_free (comment_file);
}


static void
convert_to_new_comment_system (void)
{
	if (!eel_gconf_get_boolean (PREF_MIGRATE_COMMENT_SYSTEM, TRUE))
		return;
	g_print ("converting comment system...");
	visit_rc_directory_sync (RC_COMMENTS_DIR,
				 COMMENT_EXT,
				 "",
				 TRUE,
				 convert_old_comment,
				 NULL);
	g_print ("done.");
	eel_gconf_set_boolean (PREF_MIGRATE_COMMENT_SYSTEM, FALSE);
}


/* SM support */


/* argv[0] from main(); used as the command to restart the program */
static const char *program_argv0 = NULL;

/* The master client we use for SM */
static GnomeClient *master_client = NULL;


static void
save_session (GnomeClient *client)
{
	const char  *prefix;
	GList       *scan;
	int          i = 0;

	prefix = gnome_client_get_config_prefix (client);
	gnome_config_push_prefix (prefix);

	for (scan = gth_window_get_window_list (); scan; scan = scan->next) {
		GthWindow  *window = scan->data;
		char       *uri = NULL;
		const char *location;
		char       *key;

		if (GTH_IS_VIEWER (window)) {
			location = gth_window_get_image_filename (window);
			if (location == NULL)
				continue;
			uri = add_scheme_if_absent (location);
		} 
		else {
			GthBrowser *browser = (GthBrowser*) window;

			switch (gth_browser_get_sidebar_content (browser)) {
			case GTH_SIDEBAR_DIR_LIST:
				location = gth_browser_get_current_directory (browser);
				if (location == NULL)
					continue;
				uri = add_scheme_if_absent (location);
				break;

			case GTH_SIDEBAR_CATALOG_LIST:
				location = gth_browser_get_current_catalog (browser);
				if (location == NULL)
					continue;
				uri = add_scheme_if_absent (location);
				break;

			default:
				break;
			}
		}

		if (uri == NULL)
			continue;

		key = g_strdup_printf ("Session/location%d", i);
		gnome_config_set_string (key, uri);

		g_free (uri);
		g_free (key);

		i++;
	}

	gnome_config_set_int ("Session/locations", i);

	gnome_config_pop_prefix ();
	gnome_config_sync ();
}


/* save_yourself handler for the master client */
static gboolean
client_save_yourself_cb (GnomeClient *client,
			 gint phase,
			 GnomeSaveStyle save_style,
			 gboolean shutdown,
			 GnomeInteractStyle interact_style,
			 gboolean fast,
			 gpointer data)
{
	const char *prefix;
	char       *argv[4] = { NULL };

	save_session (client);

	prefix = gnome_client_get_config_prefix (client);

	/* Tell the session manager how to discard this save */

	argv[0] = "rm";
	argv[1] = "-rf";
	argv[2] = gnome_config_get_real_path (prefix);
	argv[3] = NULL;
	gnome_client_set_discard_command (client, 3, argv);

	/* Tell the session manager how to clone or restart this instance */

	argv[0] = (char *) program_argv0;
	argv[1] = NULL; /* "--debug-session"; */

	gnome_client_set_clone_command (client, 1, argv);
	gnome_client_set_restart_command (client, 1, argv);

	return TRUE;
}


/* die handler for the master client */
static void
client_die_cb (GnomeClient *client, gpointer data)
{
	if (! client->save_yourself_emitted)
		save_session (client);

	gtk_main_quit ();
}


static void
init_session (const char *argv0)
{
	if (master_client != NULL)
		return;

	program_argv0 = argv0;

	master_client = gnome_master_client ();

	g_signal_connect (master_client, "save_yourself",
			  G_CALLBACK (client_save_yourself_cb),
			  NULL);

	g_signal_connect (master_client, "die",
			  G_CALLBACK (client_die_cb),
			  NULL);
}


static gboolean
session_is_restored (void)
{
	gboolean restored;

	if (! master_client)
		return FALSE;

	restored = (gnome_client_get_flags (master_client) & GNOME_CLIENT_RESTORED) != 0;

	return restored;
}


static char *
get_command_line_catalog_path (void)
{
	char *catalog_name_utf8;
	char *catalog_name;
	char *catalog_path;

	catalog_name_utf8 = g_strconcat (_("Command Line"),
					 CATALOG_EXT,
					 NULL);
	catalog_name = g_uri_escape_string (catalog_name_utf8, "", TRUE);
	catalog_path = get_catalog_full_path (catalog_name);
	g_free (catalog_name);
	g_free (catalog_name_utf8);

	return catalog_path;
}


static void
reset_command_line_catalog (void)
{
	char *catalog_path;
	char *catalog_uri;

	/* Reset the startup location if it was set to the command
	 * line catalog. */

	catalog_path = get_command_line_catalog_path ();
	catalog_uri = g_strconcat ("catalog://", remove_host_from_uri (catalog_path), NULL);

	if (strcmp (catalog_uri, preferences_get_startup_location ()) == 0)
		preferences_set_startup_location (get_home_uri ());

	g_free (catalog_uri);
	g_free (catalog_path);
}


/* Initialize application data. */
static void
initialize_data (void)
{
	char *current_dir;
	char *path, *filename;
	int   i = 0;

	convert_to_new_comment_system ();
	create_default_tags_if_needed ();

	eel_gconf_monitor_add ("/apps/gthumb");

	gth_monitor_get_instance ();

	/* Icon theme */

	icon_theme = gtk_icon_theme_get_default ();
	g_signal_connect (icon_theme,
			  "changed",
			  G_CALLBACK (theme_changed_cb),
			  NULL);

	/* Default windows icon */

	init_icon_pixbufs ();
	g_set_application_name (_("gThumb"));
	gtk_window_set_default_icon_name ("gthumb");

	/**/

	init_session ("gthumb");
	if (session_is_restored ())
		return;

	/* Parse command line arguments. */

	if (remaining_args == NULL) { /* No arguments specified. */
		reset_command_line_catalog ();
		return;
	}

	current_dir = g_get_current_dir ();
	while ((filename = remaining_args[i++]) != NULL) {
		GFile     *gfile;
		GFileType  file_type;
		GFileInfo *file_info;
		GError    *error = NULL;

		gfile = g_file_new_for_commandline_arg (filename);
		path = g_file_get_uri (gfile);
		file_info = g_file_query_info (gfile, G_FILE_ATTRIBUTE_STANDARD_TYPE, G_FILE_QUERY_INFO_NONE, NULL, &error);
		if (error != NULL) {
			g_object_unref (gfile);
			g_warning ("Error loading %s from command line argument: %s", filename, error->message);
			g_error_free (error);
			continue;
		}

		file_type = g_file_info_get_file_type (file_info);
		g_object_unref (file_info);

		if ((file_type != G_FILE_TYPE_DIRECTORY) && (file_type != G_FILE_TYPE_REGULAR)) {
			g_free (path);
			g_object_unref (gfile);
			continue;
		}

		if (file_type == G_FILE_TYPE_DIRECTORY)
			dir_urls = g_list_prepend (dir_urls, path);
		else
			file_urls = g_list_prepend (file_urls, path);

		g_object_unref (gfile);
	}

	n_file_urls = g_list_length (file_urls);
	n_dir_urls = g_list_length (dir_urls);

	if (n_file_urls == 1)
		view_single_image = TRUE;

	if (n_file_urls > 1) {
		/* Create a catalog with the command line list. */
		Catalog *catalog;
		char    *catalog_path;
		GList   *scan;

		catalog = catalog_new ();
		catalog_path = get_command_line_catalog_path ();
		catalog_set_path (catalog, catalog_path);
		g_free (catalog_path);

		for (scan = file_urls; scan; scan = scan->next)
			catalog_add_item (catalog, scan->data);

		catalog->sort_method = GTH_SORT_METHOD_MANUAL;

		catalog_write_to_disk (catalog, NULL);
		catalog_free (catalog);

		view_comline_catalog = TRUE;
	}
	else
		reset_command_line_catalog ();

	g_free (current_dir);
}


/* Free application data. */
static void
release_data (void)
{
	if (gth_application != NULL)
                bonobo_object_unref (gth_application);
                	
	free_icon_pixbufs ();

        g_object_unref (gth_monitor_get_instance ());

	gthumb_release ();
	eel_global_client_free ();
}


static void
open_viewer_window (const char               *uri,
		    gboolean                  use_factory,
		    GNOME_GThumb_Application  app,
		    CORBA_Environment        *env)
{
	GtkWidget *current_window;

	if (use_factory) {
		if (uri == NULL)
			uri = "";
		GNOME_GThumb_Application_open_viewer (app, uri, env);
		return;
	}

	current_window = gth_viewer_new (uri);
	gtk_widget_show (current_window);
	if (first_window == NULL)
		first_window = current_window;
}


static void
open_browser_window (const char               *uri,
		     gboolean                  show_window,
		     gboolean                  use_factory,
		     GNOME_GThumb_Application  app,
		     CORBA_Environment        *env)
{
	GtkWidget *current_window;

	if (use_factory) {
		if (uri == NULL)
			uri = "";
		GNOME_GThumb_Application_open_browser (app, uri, env);
		return;
	}

	current_window = gth_browser_new (uri);
	if (show_window)
		gtk_widget_show (current_window);
	if (first_window == NULL)
		first_window = current_window;
}


static void
load_session (gboolean                  use_factory,
	      GNOME_GThumb_Application  app,
	      CORBA_Environment        *env)
{
	int i, n;

	gnome_config_push_prefix (gnome_client_get_config_prefix (master_client));

	n = gnome_config_get_int ("Session/locations");
	for (i = 0; i < n; i++) {
		char *key;
		char *location;

		key = g_strdup_printf ("Session/location%d", i);
		location = gnome_config_get_string (key);

		if (uri_scheme_is_file (location) && path_is_file (location))
			open_viewer_window (location, use_factory, app, env);
		else
			open_browser_window (location, TRUE, use_factory, app, env);

		g_free (location);
		g_free (key);
	}

	gnome_config_pop_prefix ();
}


/* Create the windows. */
static void
prepare_app (void)
{
	CORBA_Object              factory;
	gboolean                  use_factory = FALSE;
	CORBA_Environment         env;
	GNOME_GThumb_Application  app;
	GList                    *scan;

	factory = bonobo_activation_activate_from_id ("OAFIID:GNOME_GThumb_Application_Factory",
                                                      Bonobo_ACTIVATION_FLAG_EXISTING_ONLY,
                                                      NULL, NULL);

	if (factory != NULL) {
		use_factory = TRUE;
		CORBA_exception_init (&env);
		app = bonobo_activation_activate_from_id ("OAFIID:GNOME_GThumb_Application", 0, NULL, &env);
	}

	if (session_is_restored ()) {
		load_session (use_factory, app, &env);
		return;
	}

	if (ImportPhotos) {
		if (use_factory)
		 	GNOME_GThumb_Application_import_photos (app, &env);
		else
			gth_browser_activate_action_file_camera_import (NULL, NULL);
	} 
	else if (! view_comline_catalog
		 && (n_dir_urls == 0)
		 && (n_file_urls == 0)) 
	{
		open_browser_window (NULL, TRUE, use_factory, app, &env);
	} 
	else if (view_single_image) {
		if (use_factory && eel_gconf_get_boolean (PREF_SINGLE_WINDOW, FALSE)) 
		{ 
			GNOME_GThumb_Application_load_image (app, file_urls->data, &env);
		}
		else {
			if (UseViewer)
				open_viewer_window (file_urls->data, use_factory, app, &env);
			else
				open_browser_window (file_urls->data, TRUE, use_factory, app, &env);
		}
	} 
	else if (view_comline_catalog) {
		char *catalog_path;
		char *catalog_uri;

		ViewFirstImage = TRUE;
		HideSidebar = TRUE;

		catalog_path = get_command_line_catalog_path ();
		catalog_uri = g_strconcat ("catalog://", remove_host_from_uri (catalog_path), NULL);

		open_browser_window (catalog_uri, TRUE, use_factory, app, &env);

		g_free (catalog_path);
		g_free (catalog_uri);
	}

	for (scan = dir_urls; scan; scan = scan->next) {
		/* Go to the specified directory. */
		open_browser_window (scan->data, TRUE, use_factory, app, &env);
	}

	path_list_free (file_urls);
	path_list_free (dir_urls);

	/**/

	if (use_factory) {
		bonobo_object_release_unref (app, &env);
		CORBA_exception_free (&env);
		gdk_notify_startup_complete ();
		exit (0);
	} 
	else
		gth_application = gth_application_new (gdk_screen_get_default ());
}


int
main (int   argc,
      char *argv[])
{
	GnomeProgram *program;
	char         *description;

	if (! g_thread_supported ()) {
		g_thread_init (NULL);
		gdk_threads_init ();
	}

	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	description = g_strdup_printf ("- %s", _("View and organize your images"));
	context = g_option_context_new (description);
	g_free (description);
	g_option_context_add_main_entries (context, options, GETTEXT_PACKAGE);

	program = gnome_program_init ("gthumb", VERSION,
				      LIBGNOMEUI_MODULE,
				      argc, argv,
				      GNOME_PARAM_GOPTION_CONTEXT, context,
				      GNOME_PARAM_HUMAN_READABLE_NAME, _("gThumb"),
				      GNOME_PARAM_APP_PREFIX, GTHUMB_PREFIX,
				      GNOME_PARAM_APP_SYSCONFDIR, GTHUMB_SYSCONFDIR,
				      GNOME_PARAM_APP_DATADIR, GTHUMB_DATADIR,
				      GNOME_PARAM_APP_LIBDIR, GTHUMB_LIBDIR,
				      NULL);

	gnome_vfs_init ();				      
	gnome_authentication_manager_init ();
	glade_gnome_init ();
	gthumb_init ();
	initialize_data ();
	prepare_app ();

	gdk_threads_enter ();
	gtk_main ();
	gdk_threads_leave ();

	release_data ();
	return 0;
}


/**/


static GdkPixbuf *
get_fs_icon (IconName icon_name,
	     double   icon_size)
{
	GdkPixbuf *pixbuf = NULL;
	gboolean   scale = TRUE;

	if (icon_pixbuf[icon_name] == NULL) {
		GtkIconInfo         *icon_info = NULL;

		icon_info = gtk_icon_theme_lookup_icon (icon_theme,
							icon_mime_name[icon_name],
							icon_size,
							0);

		if (icon_info == NULL) {
			icon_pixbuf[icon_name] = gdk_pixbuf_new_from_inline (-1,
									     dir_16_rgba,
									     FALSE,
									     NULL);
			scale = FALSE;
		} else {
			icon_pixbuf[icon_name] = gtk_icon_info_load_icon (icon_info, NULL);
			gtk_icon_info_free (icon_info);
		}
	}

	/* Scale keeping aspect ratio. */

 	if (! scale) {
		g_object_ref (icon_pixbuf[icon_name]);
		return icon_pixbuf[icon_name];
	}

	if (icon_pixbuf[icon_name] != NULL) {
		int w, h;

		w = gdk_pixbuf_get_width (icon_pixbuf[icon_name]);
		h = gdk_pixbuf_get_height (icon_pixbuf[icon_name]);
		if (scale_keeping_ratio (&w, &h, icon_size, icon_size, FALSE))
			pixbuf = gdk_pixbuf_scale_simple (icon_pixbuf[icon_name],
							  w,
							  h,
							  GDK_INTERP_BILINEAR);
		else {
			pixbuf = icon_pixbuf[icon_name];
			g_object_ref (pixbuf);
		}
	}

	return pixbuf;
}


GdkPixbuf *
get_folder_pixbuf (double icon_size)
{
	return get_fs_icon (ICON_NAME_DIRECTORY, icon_size);
}


int
get_folder_pixbuf_size_for_list (GtkWidget *widget)
{
	int icon_width, icon_height;

	gtk_icon_size_lookup_for_settings (gtk_widget_get_settings (widget),
					   GTK_ICON_SIZE_MENU,
					   &icon_width, &icon_height);
	return MAX (icon_width, icon_height);
}


int
get_folder_pixbuf_size_for_menu (GtkWidget *widget)
{
	int icon_width, icon_height;

	gtk_icon_size_lookup_for_settings (gtk_widget_get_settings (widget),
					   GTK_ICON_SIZE_MENU,
					   &icon_width, &icon_height);
	return MAX (icon_width, icon_height);
}


G_CONST_RETURN char *
get_stock_id_for_uri (const char *uri)
{
	const char *stock_id;

	if (strcmp (uri, g_get_home_dir ()) == 0)
		stock_id = GTK_STOCK_HOME;
	else if (strcmp (uri, get_home_uri ()) == 0)
		stock_id = GTK_STOCK_HOME;
	else if (uri_scheme_is_catalog (uri))
		stock_id = GTHUMB_STOCK_CATALOG;
	else if (uri_scheme_is_search (uri))
		stock_id = GTHUMB_STOCK_SEARCH;
	else
		stock_id = GTK_STOCK_OPEN;

	return stock_id;
}


GdkPixbuf *
get_icon_for_uri (GtkWidget  *widget,
		  const char *uri)
{
	const char *stock_id = NULL;
	int         menu_size;

	menu_size = get_folder_pixbuf_size_for_list (widget);

	if (strcmp (uri, g_get_home_dir ()) == 0)
		return get_fs_icon (ICON_NAME_HOME, menu_size);
	else if (strcmp (uri, get_home_uri ()) == 0)
		return get_fs_icon (ICON_NAME_HOME, menu_size);

	if ((strcmp (uri, "file:///") == 0) || (strcmp (uri, "/") == 0))
		return get_fs_icon (ICON_NAME_HARDDISK, menu_size);

	if (uri_scheme_is_catalog (uri)) {
		if (file_extension_is (uri, CATALOG_EXT))
			stock_id = GTHUMB_STOCK_CATALOG;
		else
			stock_id = GTHUMB_STOCK_LIBRARY;
	}
	else if (uri_scheme_is_search (uri))
		stock_id = GTHUMB_STOCK_SEARCH;

	if (stock_id != NULL)
		return gtk_widget_render_icon (widget, stock_id, GTK_ICON_SIZE_MENU, NULL);
	else
		return get_fs_icon (ICON_NAME_DIRECTORY, menu_size);
}
