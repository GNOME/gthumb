/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001 The Free Software Foundation, Inc.
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
#include <libgnomeui/gnome-window-icon.h>
#include <libgnomeui/gnome-ui-init.h>
#include <libbonoboui.h>
#include <libgnomevfs/gnome-vfs-init.h>
#include <libgnomevfs/gnome-vfs-utils.h>

#include "main.h"
#include "auto-completion.h"
#include "catalog.h"
#include "dir-list.h"
#include "file-utils.h"
#include "fullscreen.h"
#include "gconf-utils.h"
#include "gthumb-init.h"
#include "gthumb-window.h"
#include "image-viewer.h"
#include "gth-image-list.h"
#include "preferences.h"
#include "icons/pixbufs.h"
#include "typedefs.h"

#define ICON_NAME_DIRECTORY "gnome-fs-directory"


GList               *window_list = NULL;
FullScreen          *fullscreen;
char               **file_urls, **dir_urls;
int                  n_file_urls, n_dir_urls;
int                  StartInFullscreen;
int                  StartSlideshow;
int                  ViewFirstImage = FALSE;
int                  HideSidebar = FALSE;
gboolean             ExitAll = FALSE;
char                *ImageToDisplay = NULL;


static gboolean        view_comline_catalog = FALSE;
static gboolean        view_single_image = FALSE;
static GdkPixbuf      *folder_pixbuf = NULL;
static GThumbWindow   *first_window = NULL;
static GnomeIconTheme *icon_theme = NULL;


static void     prepare_app         ();
static void     initialize_data     (poptContext pctx);
static void     release_data        ();

static void     init_session        (const char *argv0);
static gboolean session_is_restored (void);
static gboolean load_session        (void);


struct poptOption options[] = {
	{ "fullscreen", 'f', POPT_ARG_NONE, &StartInFullscreen, 0,
	  N_("Start in fullscreen mode"),
	  0 },

	{ "slideshow", 's', POPT_ARG_NONE, &StartSlideshow, 0,
	  N_("Automatically start a slideshow"),
	  0 },

	{ NULL, '\0', 0, NULL, 0 }
};


/* -- Main -- */


static gboolean 
check_whether_to_set_fullscreen (gpointer data) 
{
	if (first_window == NULL)
		return FALSE;

	if (StartInFullscreen) {
		StartInFullscreen = FALSE;
		fullscreen_start (fullscreen, first_window);
	}
	
	return FALSE;
}


int 
main (int argc, char *argv[])
{
	GnomeProgram *program;
	GValue        value = { 0 };
	poptContext   pctx;

	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	program = gnome_program_init ("gthumb", VERSION,
				      LIBGNOMEUI_MODULE, 
				      argc, argv,
				      GNOME_PARAM_POPT_TABLE, options,
				      GNOME_PARAM_HUMAN_READABLE_NAME, _("gThumb"),
				      GNOME_PARAM_APP_PREFIX, GTHUMB_PREFIX,
				      GNOME_PARAM_APP_SYSCONFDIR, GTHUMB_SYSCONFDIR,
				      GNOME_PARAM_APP_DATADIR, GTHUMB_DATADIR,
				      GNOME_PARAM_APP_LIBDIR, GTHUMB_LIBDIR,
				      NULL);

	if (! g_thread_supported ())
		g_thread_init (NULL);
	gdk_threads_init ();

	g_object_get_property (G_OBJECT (program),
			       GNOME_PARAM_POPT_CONTEXT,
			       g_value_init (&value, G_TYPE_POINTER));
	pctx = g_value_get_pointer (&value);

	glade_gnome_init ();

	gthumb_init ();

	initialize_data (pctx);
	poptFreeContext (pctx);

	prepare_app ();

	g_idle_add (check_whether_to_set_fullscreen, NULL);

	bonobo_main ();

	release_data ();

	return 0;
}


static void
create_default_categories_if_needed ()
{
	Bookmarks *categories;
	char      *default_categories[] = { N_("Holidays"), 
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
			    RC_CATEGORIES_FILE,
			    NULL);
	if (path_is_file (path)) {
		g_free (path);
		return;
	}
	g_free (path);

	categories = bookmarks_new (RC_CATEGORIES_FILE);
	for (i = 0; default_categories[i] != NULL; i++) 
		bookmarks_add (categories, _(default_categories[i]), TRUE, TRUE);
	bookmarks_write_to_disk (categories);
	bookmarks_free (categories);
}


static void
theme_changed_cb (GnomeIconTheme *theme, 
		  gpointer        data)
{
	if (folder_pixbuf != NULL) {
		g_object_unref (folder_pixbuf);
		folder_pixbuf = NULL;
	}
	all_windows_notify_update_icon_theme ();
}


/* Initialize application data. */
static void 
initialize_data (poptContext pctx)
{
	const char **argv;
	int          argc;
	char        *current_dir;
	char        *path;
	char        *pixmap_file;
	int          i;

	create_default_categories_if_needed ();

	eel_gconf_monitor_add ("/apps/gthumb");

	fullscreen = fullscreen_new ();

	pixmap_file = PIXMAPSDIR "gthumb.png";
	if (g_file_test (pixmap_file, G_FILE_TEST_EXISTS))
		gnome_window_icon_set_default_from_file (pixmap_file);

	icon_theme = gnome_icon_theme_new ();
	gnome_icon_theme_set_allow_svg (icon_theme, TRUE);
	g_signal_connect (icon_theme,
			  "changed",
			  G_CALLBACK (theme_changed_cb),
			  NULL);

	init_session ("gthumb");
	if (session_is_restored ()) 
		return;

	/* Parse command line arguments. */

	argv = poptGetArgs (pctx);

	if (argv == NULL)
		return;

	for (argc = 0; argv[argc] != NULL; argc++) 
		;

	if (argc == 0)
		return;

	file_urls = g_new0 (char*, argc);
	dir_urls = g_new0 (char*, argc);

	n_file_urls = 0;
	n_dir_urls = 0;

	current_dir = g_get_current_dir ();
	for (i = 0; i < argc; i++) {
		char     *tmp1, *tmp2;
		gboolean  is_dir;

		if (! g_path_is_absolute (argv[i]))
			tmp1 = g_strconcat (current_dir, "/", argv[i], NULL);
		else
			tmp1 = g_strdup (argv[i]);

		tmp2= remove_special_dirs_from_path (tmp1);

		if (! g_path_is_absolute (tmp2))
			path = g_strconcat (current_dir, "/", tmp2, NULL);
		else {
			path = tmp2;
			tmp2 = NULL;
		}
		
		g_free (tmp1);
		g_free (tmp2);

		if (path_is_dir (path))
			is_dir = TRUE;
		else if (path_is_file (path))
			is_dir = FALSE;
		else {
			g_free (path);
			continue;
		}

		if (is_dir) {
			dir_urls[n_dir_urls++] = g_strconcat ("file://",
							      path,
							      NULL);
			g_free (path);
		} else
			file_urls[n_file_urls++] = path;
	}

	if ((n_file_urls == 1) && (file_urls[0] != NULL)) {
		view_single_image = TRUE;

	} else if (n_file_urls > 1) {
		/* Create a catalog with the command line list. */
		Catalog *catalog;
		char    *catalog_path;
		char    *catalog_name, *catalog_name_utf8;

		catalog = catalog_new ();

		catalog_name_utf8 = g_strconcat (_("Command Line"),
						 CATALOG_EXT,
						 NULL);
		catalog_name = g_locale_from_utf8 (catalog_name_utf8, -1, 0, 0, 0);
		catalog_path = get_catalog_full_path (catalog_name);
		g_free (catalog_name);
		g_free (catalog_name_utf8);

		catalog_set_path (catalog, catalog_path);
		g_free (catalog_path);

		for (i = 0; i < n_file_urls; i++) 
			catalog_add_item (catalog, file_urls[i]);

		catalog_write_to_disk (catalog, NULL);
		catalog_free (catalog);

		view_comline_catalog = TRUE;
	}

	g_free (current_dir);
}


/* Free application data. */
static void 
release_data ()
{
	if (folder_pixbuf != NULL)
		g_object_unref (folder_pixbuf);

	g_object_unref (icon_theme);

	fullscreen_close (fullscreen);
	preferences_release ();
	eel_global_client_free ();
	auto_compl_reset ();
}


/* Create the windows. */
static void 
prepare_app ()
{
	GThumbWindow *current_window = NULL;
	int           i;
	
	if (session_is_restored ()) {
		load_session ();
		return;
	}

	if (! view_comline_catalog 
	    && (n_dir_urls == 0) 
	    && (n_file_urls == 0)) {
		current_window = window_new ();
		gtk_widget_show (current_window->app);
		if (first_window == NULL)
			first_window = current_window;
	}

	if (view_single_image) {
		char *image_folder;

		ImageToDisplay = g_strdup (file_urls[0]);
		HideSidebar = TRUE;

		image_folder = remove_level_from_path (ImageToDisplay);
		eel_gconf_set_locale_string (PREF_STARTUP_LOCATION, image_folder);
		g_free (image_folder);

		current_window = window_new ();
		gtk_widget_show (current_window->app);

		if (first_window == NULL)
			first_window = current_window;

	} else if (view_comline_catalog) {
		char *catalog_uri;
		char *catalog_path;
		char *catalog_name;

		catalog_name = g_strconcat (_("Command Line"), CATALOG_EXT, NULL);
		catalog_path = get_catalog_full_path (catalog_name);
		g_free (catalog_name);

		catalog_uri = g_strconcat ("catalog://", catalog_path, NULL);
		g_free (catalog_path);

		eel_gconf_set_locale_string (PREF_STARTUP_LOCATION, catalog_uri);
		g_free (catalog_uri);

		ViewFirstImage = TRUE;
		HideSidebar = TRUE;

		current_window = window_new ();
		gtk_widget_show (current_window->app);

		if (first_window == NULL)
			first_window = current_window;
	}

	for (i = 0; i < n_dir_urls; i++) {
		/* Go to the specified directory. */
		eel_gconf_set_locale_string (PREF_STARTUP_LOCATION, dir_urls[i]);

		current_window = window_new ();
		gtk_widget_show (current_window->app);

		if (first_window == NULL)
			first_window = current_window;
	}

	/* Free urls. */

	if (n_file_urls > 0) {
		for (i = 0; i < n_file_urls; i++)
			g_free (file_urls[i]);
		g_free (file_urls);
	}

	if (n_dir_urls > 0) {
		for (i = 0; i < n_dir_urls; i++)
			g_free (dir_urls[i]);
		g_free (dir_urls);
	}
}


void 
all_windows_update_file_list ()
{
	g_list_foreach (window_list, (GFunc) window_update_file_list, NULL);
}


void 
all_windows_update_catalog_list ()
{
	g_list_foreach (window_list, (GFunc) window_update_catalog_list, NULL);
}


void 
all_windows_update_bookmark_list ()
{
	g_list_foreach (window_list, (GFunc) window_update_bookmark_list, NULL);
}


void 
all_windows_update_viewer_options ()
{
	GList *scan;

	for (scan = window_list; scan; scan = scan->next) {
		GThumbWindow *window = scan->data;

		image_viewer_set_check_type   (IMAGE_VIEWER (window->viewer),
					       pref_get_check_type ());
		image_viewer_set_check_size   (IMAGE_VIEWER (window->viewer),
					       pref_get_check_size ());
		image_viewer_set_transp_type  (IMAGE_VIEWER (window->viewer),
					       pref_get_transp_type ());
		image_viewer_set_zoom_quality (IMAGE_VIEWER (window->viewer),
					       pref_get_zoom_quality ());
		image_viewer_set_zoom_change  (IMAGE_VIEWER (window->viewer),
					       pref_get_zoom_change ());
		image_viewer_update_view      (IMAGE_VIEWER (window->viewer));

		window_sync_menu_with_preferences (window);
	}
}


void 
all_windows_update_browser_options ()
{
	GList *scan;

	for (scan = window_list; scan; scan = scan->next) {
		GThumbWindow  *window = scan->data;
		
		window->file_list->enable_thumbs = eel_gconf_get_boolean (PREF_SHOW_THUMBNAILS);
		gth_file_list_set_thumbs_size (window->file_list, eel_gconf_get_integer (PREF_THUMBNAIL_SIZE));
		gth_file_view_set_view_mode (window->file_list->view,
					     pref_get_view_mode ());
		window_update_file_list (window);
		dir_list_update_underline (window->dir_list);
	}
}


void 
all_windows_notify_files_created (GList *list)
{
	GList *scan;

	for (scan = window_list; scan; scan = scan->next) {
		GThumbWindow *window = scan->data;

		if ((window->sidebar_content == GTH_SIDEBAR_DIR_LIST) &&
		    window->monitor_enabled)
			continue;

		window_notify_files_created (window, list);
	}
}


void 
all_windows_notify_files_deleted (GList *list)
{
	GList *scan;

	for (scan = window_list; scan; scan = scan->next) {
		GThumbWindow *window = scan->data;

		if ((window->sidebar_content == GTH_SIDEBAR_DIR_LIST) &&
		    window->monitor_enabled)
			continue;

		window_notify_files_deleted (window, list);
	}
}


void 
all_windows_notify_files_changed (GList *list)
{
	GList *scan;

	for (scan = window_list; scan; scan = scan->next) {
		GThumbWindow *window = scan->data;

		if ((window->sidebar_content == GTH_SIDEBAR_DIR_LIST) &&
		    window->monitor_enabled)
			continue;

		window_notify_files_changed (window, list);
	}
}


void 
all_windows_notify_cat_files_added (const char *catalog_path,
				    GList      *list)
{
	GList *scan;

	if (list == NULL)
		return;

	for (scan = window_list; scan; scan = scan->next) {
		GThumbWindow *window = scan->data;
		window_notify_cat_files_added (window, catalog_path, list);
	}
}


void 
all_windows_notify_cat_files_deleted (const char *catalog_path,
				      GList      *list)
{
	GList *scan;

	if (list == NULL)
		return;

	for (scan = window_list; scan; scan = scan->next) {
		GThumbWindow *window = scan->data;
		window_notify_cat_files_deleted (window, catalog_path, list);
	}
}


void 
all_windows_notify_file_rename (const gchar *old_name,
				const gchar *new_name)
{
	GList *scan;

	for (scan = window_list; scan; scan = scan->next) {
		GThumbWindow *window = scan->data;

		if ((window->sidebar_content == GTH_SIDEBAR_DIR_LIST) &&
		    window->monitor_enabled)
			continue;

		window_notify_file_rename (window, old_name, new_name);
	}
}


void
all_windows_notify_files_rename (GList       *old_names,
				 GList       *new_names)
{
	GList *o_scan, *n_scan;

	for (o_scan = old_names, n_scan = new_names; o_scan && n_scan;) {
		const char *old_name = o_scan->data;
		const char *new_name = n_scan->data;

		all_windows_notify_file_rename (old_name, new_name);
		
		o_scan = o_scan->next;
		n_scan = n_scan->next;
	}
}


void 
all_windows_notify_directory_rename (const gchar *old_name,
				     const gchar *new_name)
{
	GList *scan;

	for (scan = window_list; scan; scan = scan->next) {
		GThumbWindow *window = scan->data;
		window_notify_directory_rename (window, old_name, new_name);
	}
}


void 
all_windows_notify_directory_delete (const gchar *path)
{
	GList *scan;

	for (scan = window_list; scan; scan = scan->next) {
		GThumbWindow *window = scan->data;
		window_notify_directory_delete (window, path);
	}
}


void 
all_windows_notify_directory_new (const gchar *path)
{
	GList *scan;

	for (scan = window_list; scan; scan = scan->next) {
		GThumbWindow *window = scan->data;
		window_notify_directory_new (window, path);
	}
}


void
all_windows_notify_catalog_rename (const gchar *oldname,
				   const gchar *newname)
{
	GList *scan;

	for (scan = window_list; scan; scan = scan->next) {
		GThumbWindow *window = scan->data;
		window_notify_catalog_rename (window, oldname, newname);
	}
}


void
all_windows_notify_catalog_new (const gchar *path)
{
	GList *scan;

	for (scan = window_list; scan; scan = scan->next) {
		GThumbWindow *window = scan->data;
		window_notify_catalog_new (window, path);
	}
}


void
all_windows_notify_catalog_delete (const gchar *path)
{
	GList *scan;

	for (scan = window_list; scan; scan = scan->next) {
		GThumbWindow *window = scan->data;
		window_notify_catalog_delete (window, path);
	}
}


void 
all_windows_notify_update_comment (const gchar *filename)
{
	GList *scan;

	for (scan = window_list; scan; scan = scan->next) {
		GThumbWindow *window = scan->data;
		window_notify_update_comment (window, filename);
	}
}


void 
all_windows_notify_update_directory (const gchar *dir_path)
{
	GList *scan;

	for (scan = window_list; scan; scan = scan->next) {
		GThumbWindow *window = scan->data;

		if ((window->sidebar_content == GTH_SIDEBAR_DIR_LIST) &&
		    window->monitor_enabled)
			continue;

		window_notify_update_directory (window, dir_path);
	}
}


void
all_windows_notify_update_icon_theme ()
{
	GList *scan;

	for (scan = window_list; scan; scan = scan->next) {
		GThumbWindow *window = scan->data;
		window_notify_update_icon_theme (window);
	}
}


void
all_windows_remove_monitor ()
{
	GList *scan;
	
	for (scan = window_list; scan; scan = scan->next) {
		GThumbWindow *window = scan->data;
		window_remove_monitor (window);
	}
}


void
all_windows_add_monitor ()
{
	GList *scan;
	
	for (scan = window_list; scan; scan = scan->next) {
		GThumbWindow *window = scan->data;
		window_add_monitor (window);
	}
}


GdkPixbuf *
get_folder_pixbuf (double icon_size)
{
	GdkPixbuf       *pixbuf = NULL;
	static gboolean  scale = TRUE;

	if (folder_pixbuf == NULL) {
		char *icon_path;
		
		icon_path = gnome_icon_theme_lookup_icon (icon_theme,
							  ICON_NAME_DIRECTORY,
							  icon_size,
							  NULL,
							  NULL);

		if (icon_path == NULL) {
			folder_pixbuf = gdk_pixbuf_new_from_inline (-1, 
								    dir_16_rgba, 
								    FALSE, 
								    NULL);
			scale = FALSE;

		} else {
			folder_pixbuf = gdk_pixbuf_new_from_file (icon_path, 
								  NULL);
			g_free (icon_path);
		}
	}

	/* Scale keeping aspect ratio. */

 	if (! scale) {
		g_object_ref (folder_pixbuf);
		return folder_pixbuf;
	}
	
	if (folder_pixbuf != NULL) {
		int       new_w, new_h;
		int       w, h;
		double    factor;
		
		w = gdk_pixbuf_get_width (folder_pixbuf);
		h = gdk_pixbuf_get_height (folder_pixbuf);
		
		factor = MIN (icon_size / w, icon_size / h);
		new_w  = MAX ((gint) (factor * w), 1);
		new_h  = MAX ((gint) (factor * h), 1);
				
		pixbuf = gdk_pixbuf_scale_simple (folder_pixbuf,
						  new_w,
						  new_h,
						  GDK_INTERP_BILINEAR);
	}

	return pixbuf;
}




/* SM support */

/* The master client we use for SM */
static GnomeClient *master_client = NULL;

/* argv[0] from main(); used as the command to restart the program */
static const char *program_argv0 = NULL;


static void
save_session (GnomeClient *client)
{
	const char  *prefix;
	GList        *scan;
	int          i = 0;

	prefix = gnome_client_get_config_prefix (client);
	gnome_config_push_prefix (prefix);

	for (scan = window_list; scan; scan = scan->next) {
		GThumbWindow *window = scan->data;
		char         *location = NULL;
		char         *key;

		if ((window->sidebar_content == GTH_SIDEBAR_DIR_LIST) 
		    && (window->dir_list != NULL)
		    && (window->dir_list->path != NULL))
			location = g_strconcat ("file://",
						window->dir_list->path,
						NULL);
		else if ((window->sidebar_content == GTH_SIDEBAR_CATALOG_LIST) 
			 && (window->catalog_path != NULL))
			location = g_strconcat ("catalog://",
						window->catalog_path,
						NULL);
		
		if (location == NULL)
			continue;

		key = g_strdup_printf ("Session/location%d", i);
		gnome_config_set_string (key, location);

		g_free (key);
		g_free (location);

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


static gboolean
load_session (void)
{
	int i, n;
	
	gnome_config_push_prefix (gnome_client_get_config_prefix (master_client));

	n = gnome_config_get_int ("Session/locations");
	for (i = 0; i < n; i++) {
		GThumbWindow *window;
		char         *key;
		char         *location;

		key = g_strdup_printf ("Session/location%d", i);
		location = gnome_config_get_string (key);
		g_free (key);

		eel_gconf_set_locale_string (PREF_STARTUP_LOCATION, location);

		window = window_new ();
		gtk_widget_show (window->app);

		g_free (location);
	}

	gnome_config_pop_prefix ();

	return TRUE;
}


int
get_default_folder_pixbuf_size (GtkWidget *widget)
{
	int icon_width, icon_height;

	gtk_icon_size_lookup_for_settings (gtk_widget_get_settings (widget),
                                           ICON_GTK_SIZE,
                                           &icon_width, &icon_height);
	return MAX (icon_width, icon_height);
}
