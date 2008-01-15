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
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <libgnome/gnome-help.h>
#include <glade/glade.h>
#include <libgnomevfs/gnome-vfs-utils.h>

#include "file-utils.h"
#include "gth-utils.h"
#include "gtk-utils.h"
#include "glib-utils.h"
#include "main.h"
#include "preferences.h"
#include "gconf-utils.h"
#include "thumb-loader.h"


#include "dlg-scripts.h"


#define SCRIPT_GLADE_FILE "gthumb_tools.glade"
#define DEF_THUMB_SIZE 128
#define MAX_SCRIPTS 10

typedef struct {
        GthWindow    *window;
	DoneFunc      done_func;
	gpointer      done_data;
        GladeXML     *gui;
        GtkWidget    *dialog;
} DialogData;


typedef struct {
        GladeXML     *gui;
        GtkWidget    *dialog;
	gboolean      cancel;
        GtkWidget    *progress_thumbnail;
        ThumbLoader  *loader;
        gboolean      loading_image;
	int	      thumb_size;
} ProgressData;


typedef struct {
        gint   number;
        gchar *short_name;
        gchar *script_text;
} ScriptStruct;

typedef struct {
	/* Name of the script */
	char *name;

	/* Command to run */
	char *command;
} ScriptCommand;


static ScriptCommand script_commands[] = { 
	{N_("Edit with GIMP"), "gimp-remote %F"},
	{N_("Add copyright"), "convert %f -font Helvetica -pointsize 20 -fill white  -box '#00000080'  -gravity South -annotate +0+5 ' Copyright 2007, Your Name Here ' %n-copyright%e"},
	{N_("Copy to \"approved\" folder"), "mkdir -p %p/approved ; cp %f %p/approved/"},
	{N_("Send by email"), "uuencode %f %f | mail -s Photos your@emailaddress.com"},
	{N_("Make a zip file"), "rm ~/myarchive.zip; zip -j ~/myarchive %F"},
	{N_("Make a zip file and email it"), "rm ~/myarchive.zip; zip -j ~/myarchive %F; uuencode ~/myarchive.zip ~/myarchive.zip | mail -s Photos your@emailaddress.com"}
};


enum {
        COLUMN_SCRIPT_NUMBER,
        COLUMN_SHORT_NAME,
        COLUMN_SCRIPT_TEXT,
        NUMBER_OF_COLUMNS
};


static GArray *script_array = NULL;


/* called when the main dialog is closed. */
static void
progress_cancel_cb (GtkWidget    *widget,
                    ProgressData *data)
{
	data->cancel = TRUE;	
}


static
gchar* get_prompt (GtkWindow  *window,
 		   GString     *prompt_name)
{
        GladeXML     *gui;
        GtkWidget    *dialog;
        GtkWidget    *entry;
        GtkWidget    *label;
	gchar	     *result = NULL;
	gchar	     *pref;

        gui = glade_xml_new (GTHUMB_GLADEDIR "/" SCRIPT_GLADE_FILE, NULL,
                             NULL);

        if (!gui) {
                g_warning ("Could not find " SCRIPT_GLADE_FILE "\n");
                return;
        }

        dialog = glade_xml_get_widget (gui, "prompt_dialog");
        label = glade_xml_get_widget (gui, "prompt_label");
        entry = glade_xml_get_widget (gui, "prompt_entry");

	gtk_label_set_text (GTK_LABEL (label), prompt_name->str);

	pref = g_strconcat (PREF_SCRIPT_BASE, gconf_escape_key (prompt_name->str, prompt_name->len), NULL);
	gtk_entry_set_text (GTK_ENTRY (entry), eel_gconf_get_string (pref, ""));

        gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (window));
        gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);

        gtk_widget_show (dialog);
	gtk_dialog_run (GTK_DIALOG (dialog));

	result = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));

	/* save response for future use, except passwords */
	if ((strcmp (prompt_name->str, "password") != 0) &&
	    (strcmp (prompt_name->str, "PASSWORD") != 0))
		eel_gconf_set_string (pref, result);

	g_free (pref);

	gtk_widget_destroy (dialog);

	return result;
}


static
gboolean delete_lowercase_keys (gpointer key, gpointer value, gpointer user_data)
{
	return g_unichar_islower (g_utf8_get_char (key));
}


static
char* get_date_strings (GtkWindow  *window,
			char       *filename,
			const char *text_in,
			GHashTable *date_prompts)
{
	gchar   *text_out = NULL;
	gchar	*pos;
	GString *new_string, *last_match, *last_date;
	GHashTable *prompts;

	new_string = g_string_new (NULL);
	last_match = last_date = NULL;
	prompts = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);

	for (pos = (char *) text_in; *pos != 0; pos = g_utf8_next_char (pos)) {
		gchar     *end;
		gchar     *check_char;
		GString   *date_str;
		gunichar  ch = g_utf8_get_char (pos);
		gboolean  closing_bracket_found = FALSE, prompt_mode = TRUE, silent_mode = FALSE;

		/* Treat curly bracket literally if preceded by a backslash */
		if (ch == '\\') {
			if (g_utf8_get_char (g_utf8_next_char (pos)) == '{') {
				/* do not include the backslash */
				pos = g_utf8_next_char (pos);
				g_string_append_unichar (new_string, '}');
			} else
				g_string_append_unichar (new_string, ch);
			continue;
		}

		date_str = g_string_new (NULL);

		if (ch == '{') {
			end = g_utf8_next_char (pos);
			if (g_utf8_get_char (end) == '#') {
				silent_mode = TRUE;
				end = g_utf8_next_char (end);
			}

			while ((*end != 0) && !closing_bracket_found) {
				gunichar ch2 = g_utf8_get_char (end);

				if (ch2 == '}') {
					closing_bracket_found = TRUE;
				} else {
					g_string_append_unichar (date_str, ch2);
					end = g_utf8_next_char (end);
				}
			}
		}

		if (!closing_bracket_found) {
			/* Not a date string. Just append this character to the output, without
			   any interpretation. */
			g_string_append_unichar (new_string, ch);
			g_string_free (date_str, TRUE);
			continue;
		}

		pos = end;

		/* if all upper case, then it's a prompt */
		for (check_char = date_str->str; *check_char; check_char = g_utf8_next_char (check_char)) {
			if ((!g_unichar_isupper (g_utf8_get_char (check_char)))) {
						prompt_mode = FALSE;
						break;
			}
		}

		if (!prompt_mode) {
			const gint date_str_replacement_size = date_str->len + 128;

			FileData* fd = file_data_new_from_local_path (filename);
			file_data_load_exif_data (fd);

			time_t exif_time = fd->exif_time;
			if (!exif_time)
				exif_time = fd->mtime;

			file_data_unref(fd);

			gchar date_str_replacement[date_str_replacement_size];
			gchar *date_str_escaped = shell_escape (date_str->str);
			if (strftime (date_str_replacement, date_str_replacement_size, date_str_escaped, localtime (&exif_time)) > 0 && !silent_mode)
				g_string_append (new_string, date_str_replacement);

			if (last_match)
				g_string_free (last_match, TRUE);
			if (last_date)
				g_string_free (last_date, TRUE);
			last_match = g_string_new ("");
			g_string_append (last_match, date_str_replacement);
			last_date = g_string_new (date_str->str);

			g_free (date_str_escaped);
		} else {
			/* Need a match for date prompt */
			if (!last_match || !last_date) {
				g_string_append_unichar (new_string, ch);
				g_string_free (date_str, TRUE);
				continue;
			}

			if (g_hash_table_lookup (prompts, date_str->str) == NULL) {
				GString *hash_key = g_string_new ("");
	
				/* Mash the values together to make a unique hash key */
				g_string_append (hash_key, date_str->str);
				g_string_append (hash_key, "}");
				g_string_append (hash_key, last_date->str);
				g_string_append (hash_key, "}");
				g_string_append (hash_key, last_match->str);
	
				if (g_hash_table_lookup (date_prompts, hash_key->str) == NULL) {
					GString *prompt_name;
	
					prompt_name = g_string_new ("");
					g_string_append (prompt_name, date_str->str);
					g_string_append (prompt_name, " for ");
					g_string_append (prompt_name, last_match->str);
	
					g_hash_table_insert (date_prompts,
							hash_key->str,
							get_prompt (window, prompt_name));
	
					g_string_free (prompt_name, TRUE);
				}
	
				if (g_hash_table_lookup (date_prompts, hash_key->str) != NULL) {
					GString *copy = g_string_new ("");
					g_string_append (copy, g_hash_table_lookup (date_prompts, hash_key->str));

					g_hash_table_insert (prompts, date_str->str, copy->str);
					g_string_free (copy, FALSE);
				}

				g_string_free (hash_key, FALSE);
			}

			if (g_hash_table_lookup (prompts, date_str->str) != NULL && !silent_mode)
				g_string_append (new_string, g_hash_table_lookup (prompts, date_str->str));
		}
		g_string_free (date_str, FALSE);
	}

	text_out = new_string->str;
	g_string_free (new_string, FALSE);
	g_hash_table_destroy (prompts);

	if (last_match)
		g_string_free (last_match, TRUE);
	if (last_date)
		g_string_free (last_date, TRUE);

	return text_out;
}


static
char* get_user_prompts (GtkWindow  *window,
			char       *text_in,
			GHashTable *user_prompts)
{
	char    *text_out = NULL;
	char	*pos;
	GString *new_string;

	/* Lower case keys are refreshed for each iteration. Delete existing values. */
	g_hash_table_foreach_remove (user_prompts, delete_lowercase_keys, NULL);

	new_string = g_string_new (NULL);

	for (pos = text_in; *pos != 0; pos = g_utf8_next_char (pos)) {
		char     *end;
		char     *check_char;
		GString  *prompt_name;
		gunichar  ch;
		gboolean  closing_bracket_found = FALSE;
		gboolean  valid_prompt_name = TRUE;
		gboolean  lower_case_mode;

		ch = g_utf8_get_char (pos);
		closing_bracket_found = FALSE;

		/* Treat square bracket literally if preceded by a backslash */
		if (ch == '\\') {
			if (g_utf8_get_char (g_utf8_next_char (pos)) == '[') {
				/* do not include the backslash */
				pos = g_utf8_next_char (pos);
				g_string_append_unichar (new_string, '[');
			} else
				g_string_append_unichar (new_string, ch);
			continue;
		}

		prompt_name = g_string_new (NULL);

		if (ch == '[') {
			end = g_utf8_next_char (pos);

			while ((*end != 0) && !closing_bracket_found) {
				gunichar ch2 = g_utf8_get_char (end);

				if (ch2 == ']') {
					closing_bracket_found = TRUE;
				} else {
					g_string_append_unichar (prompt_name, ch2);
					end = g_utf8_next_char (end);
				}
			}
		}

		if (closing_bracket_found) {
			lower_case_mode = g_unichar_islower (g_utf8_get_char (prompt_name->str));

			/* must be all upper case or all lower case */
			for (check_char = prompt_name->str; (*check_char !=0) && valid_prompt_name; check_char = g_utf8_next_char (check_char)) {
				if ((!g_unichar_isalpha (g_utf8_get_char (check_char))) ||
					(g_unichar_islower (g_utf8_get_char (check_char)) != lower_case_mode)) {
					valid_prompt_name = FALSE;
				}
			}
		}

		if (!closing_bracket_found || !valid_prompt_name) {
			/* Not a prompt. Just append this character to the output, without
			   any interpretation. */
			g_string_append_unichar (new_string, ch);
			g_string_free (prompt_name, TRUE);
			continue;
		}

		pos = end;

		if (g_hash_table_lookup (user_prompts, prompt_name->str) == NULL) {
			g_hash_table_insert (user_prompts, 
					     prompt_name->str, 
					     get_prompt (window, prompt_name));
		}

		if (g_hash_table_lookup (user_prompts, prompt_name->str) != NULL)
			g_string_append (new_string, g_hash_table_lookup (user_prompts, prompt_name->str));

		g_string_free (prompt_name, FALSE);
	}

	text_out = new_string->str;
	g_string_free (new_string, FALSE);

	return text_out;
}


static void
image_loader_done (ImageLoader *il,
                   gpointer     user_data)
{
	ProgressData *data = user_data;
	GdkPixbuf    *pixbuf;
	
        gtk_image_set_from_pixbuf (GTK_IMAGE (data->progress_thumbnail), 
				   thumb_loader_get_pixbuf (data->loader));

        data->loading_image = FALSE;
}


static void
image_loader_error (ImageLoader *il,
                    gpointer     user_data)
{
        ProgressData *data = user_data;

	gtk_image_set_from_pixbuf (GTK_IMAGE (data->progress_thumbnail), NULL);
	data->loading_image = FALSE;
}


static void
load_thumbnail (ProgressData *data,
		char         *filename)
{
	if (data->cancel)
		return;

	/* Just load what we can. Many scripts are so fast that most
	   thumbnails will be skipped. */
	if (data->loading_image)
		return;

	data->loading_image = TRUE;

        if (data->loader == NULL) {
                data->loader = THUMB_LOADER (thumb_loader_new (data->thumb_size, data->thumb_size));
                thumb_loader_use_cache (data->loader, TRUE);
                g_signal_connect (G_OBJECT (data->loader),
                                  "thumb_done",
                                  G_CALLBACK (image_loader_done),
                                  data);
                g_signal_connect (G_OBJECT (data->loader),
                                  "thumb_error",
                                  G_CALLBACK (image_loader_error),
                                  data);
        }

        thumb_loader_set_path (data->loader, filename);
        thumb_loader_start (data->loader);
}


static void
exec_shell_script (GtkWindow  *window,
		   const char *script,
		   const char *name,
		   GList      *file_list)
{
	ProgressData *data;
	GtkWidget    *label;
	GtkWidget    *bar;
	GtkWidget    *progress_cancel_button;
	GList        *scan;
	char	     *full_name;
	int           i, n;
	GHashTable   *user_prompts;
	GHashTable   *date_prompts;


	if ((script == NULL) || (file_list == NULL))
		return;

	data = g_new0 (ProgressData, 1);

	/* Add a progress indicator */
	data->gui = glade_xml_new (GTHUMB_GLADEDIR "/" SCRIPT_GLADE_FILE, NULL,
                                   NULL);

        if (!data->gui) {
                g_warning ("Could not find " SCRIPT_GLADE_FILE "\n");
                return;
        }

	data->dialog = glade_xml_get_widget (data->gui, "hotkey_progress");
	label = glade_xml_get_widget (data->gui, "progress_info");
	bar = glade_xml_get_widget (data->gui, "progress_progressbar");
	progress_cancel_button = glade_xml_get_widget (data->gui, "progress_cancel_button");

	data->progress_thumbnail = glade_xml_get_widget (data->gui, "progress_thumbnail");
	data->loader = NULL;
	data->loading_image = FALSE;
	data->thumb_size = eel_gconf_get_integer (PREF_THUMBNAIL_SIZE, DEF_THUMB_SIZE);

	n = g_list_length (file_list);

	if (name == NULL)
		gtk_window_set_title (GTK_WINDOW (data->dialog), _("Script Progress"));
	else {
		full_name = g_strconcat (_("Script Progress"), " - ", name, NULL);
		gtk_window_set_title (GTK_WINDOW (data->dialog), full_name);
	}

	data->cancel = FALSE;

        g_signal_connect (G_OBJECT (data->dialog),
                          "destroy",
                          G_CALLBACK (progress_cancel_cb),
                          data);

        g_signal_connect (G_OBJECT (progress_cancel_button),
                          "clicked",
                          G_CALLBACK (progress_cancel_cb),
                          data);

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), GTK_WINDOW (window));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), TRUE);

	gtk_widget_show (data->dialog);

	while (gtk_events_pending())
		gtk_main_iteration();

	user_prompts = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
	date_prompts = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);

	/* If the %F code is present, all selected files are processed by
	   one script instance. Otherwise, each file is handled sequentially. */

	if (strstr (script, "%F")) {
		char *command1 = NULL;
		char *command0 = NULL;
		char *file_list_string;

		file_list_string = g_strdup (" ");

		/* Load thumbnail for first image only */
		load_thumbnail (data, file_list->data);

		for (scan = file_list; scan; scan = scan->next) {
			char *filename;
			char *e_filename;
			char *new_file_list;

			if (is_local_file (scan->data))
				filename = gnome_vfs_unescape_string_for_display (remove_host_from_uri (scan->data));
			else
				filename = gnome_vfs_unescape_string_for_display (scan->data);

			e_filename = shell_escape (filename);

			new_file_list = g_strconcat (file_list_string, e_filename, " ", NULL);

			g_free (e_filename);
			g_free (file_list_string);
			file_list_string = g_strdup (new_file_list);

			g_free (new_file_list);
		}

		command1 = _g_substitute_pattern (script, 'F', file_list_string);
		command0 = get_user_prompts (window, command1, user_prompts);

		g_free (command1);
		g_free (file_list_string);

		system (command0);
		g_free (command0);


		_gtk_label_set_filename_text (GTK_LABEL (label), script);
		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (bar),
					       (gdouble) 1.0);
		while (gtk_events_pending())
			gtk_main_iteration();

		if (data->cancel)
			_gtk_error_dialog_run (GTK_WINDOW (window), 
					       _("Scripts using the %%F code can not be cancelled. Sorry!"));

	} else {
		i = 0;
		for (scan = file_list; scan && !data->cancel; scan = scan->next) {
			char *filename;
			char *e_filename;
			char *name_wo_ext = NULL;
			char *extension = NULL;
			char *parent = NULL;
			char *basename = NULL;
			char *basename_wo_ext = NULL;
			char *command0 = NULL;
			char *command1 = NULL;
			char *command2 = NULL;
			char *command3 = NULL;
			char *command4 = NULL;
			char *command5 = NULL;
			char *command6 = NULL;
			char *command7 = NULL;

			load_thumbnail (data, scan->data);

			if (is_local_file (scan->data))
				filename = gnome_vfs_unescape_string_for_display (remove_host_from_uri (scan->data));
			else
				filename = gnome_vfs_unescape_string_for_display (scan->data);

			name_wo_ext = remove_extension_from_path (filename);
			extension = g_filename_to_utf8 (strrchr (filename, '.'), -1, 0, 0, 0);
			parent = remove_level_from_path (filename);
			basename = g_strdup (file_name_from_path (filename));
			basename_wo_ext = remove_extension_from_path (basename);

			command7 = get_date_strings (window, filename, script, date_prompts);

			e_filename = shell_escape (filename);
			command6 = _g_substitute_pattern (command7, 'f', e_filename);
			g_free (e_filename);
			g_free (command7);

                        e_filename = shell_escape (basename);
                        command5 = _g_substitute_pattern (command6, 'b', e_filename);
                        g_free (e_filename);
                        g_free (command6);

			e_filename = shell_escape (name_wo_ext);
			command4 = _g_substitute_pattern (command5, 'n', e_filename);
			g_free (e_filename);
			g_free (command5);

                        e_filename = shell_escape (basename_wo_ext);
                        command3 = _g_substitute_pattern (command4, 'm', e_filename);
                        g_free (e_filename);
                        g_free (command4);

			e_filename = shell_escape (extension);
			command2 = _g_substitute_pattern (command3, 'e', e_filename);
			g_free (e_filename);
			g_free (command3);

			e_filename = shell_escape (parent);
			command1 = _g_substitute_pattern (command2, 'p', e_filename);
			g_free (e_filename);
			g_free (command2);

			command0 = get_user_prompts (window, command1, user_prompts);
			g_free (command1);

			g_free (filename);
			g_free (name_wo_ext);
			g_free (extension);
			g_free (parent);
			g_free (basename);
			g_free (basename_wo_ext);

			_gtk_label_set_filename_text (GTK_LABEL (label), command0);
			gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (bar),
						       (gdouble) (i + 0.5) / n);

			system (command0);
			g_free (command0);

			while (gtk_events_pending ())
				gtk_main_iteration ();

			i++;
		}
	}

	/* Finish last thumbnail */
	while (data->loading_image)
		gtk_main_iteration ();

	if (data->dialog)
		gtk_widget_destroy (data->dialog);

	g_hash_table_destroy (user_prompts);
	g_hash_table_destroy (date_prompts);
	g_object_unref (data->gui);
	g_free (data);
	g_free (full_name);
}


char*
gconf_script_path (unsigned int script_number)
{
	return g_strdup_printf (PREF_HOTKEY_PREFIX "%d", script_number);
}


char*
gconf_script_name_path (unsigned int script_number)
{
	return g_strdup_printf (PREF_HOTKEY_PREFIX "%d_name", script_number);
}


void
gconf_get_script (unsigned int number, char **name, char **command) {

	char *user_name, *current_command, *default_name, *default_command;
	char *script_name, *script_command;
	char *dummy_name = g_strdup_printf (_("Script %d"), number);

	if (number < sizeof (script_commands) / sizeof (ScriptCommand)) {
		default_name = _(script_commands[number].name);
		default_command = script_commands[number].command;
	} else {
		default_name = dummy_name;
		default_command = "";
	}

	/* First check if the user has specified a name for the script (gthumb >= 2.11) */
	user_name = eel_gconf_get_string (gconf_script_name_path (number), "");

	/* Check if the script's command is the default one */
	current_command = eel_gconf_get_string (gconf_script_path (number), "");

	if (!strcmp (user_name, "") || !strcmp (user_name, dummy_name)) {
		if (!strcmp (current_command, default_command) || !strcmp (current_command, "")) {
			/* The user did not define a custom command */
			script_name = g_strdup ((char*) default_name);
			script_command = g_strdup ((char*) default_command);
		} else {
			/* The user did define a custom command but no name (gthumb < 2.11) */
			script_name = g_strdup ((char*) dummy_name);
			script_command = g_strdup (current_command);
		}
	} else  {
		/* There was a non-default value stored in gconf, so return it */
		script_name = g_strdup (user_name);
		/* Not sure of that : can there be a name but no command ? Better check... */
		if (!strcmp (current_command, ""))
			script_command = g_strdup ((char*) default_command);
		else
			script_command = g_strdup ((char*) current_command);
	}

	g_free (user_name);
	g_free (current_command);
	g_free (dummy_name);

	if (name) *name = script_name;
	if (command) *command = script_command;

}


void exec_script (GtkAction *action, ScriptCallbackData *cb_data) {

	GList *list = gth_window_get_file_list_selection (cb_data->window);

        if (list != NULL) {
		char *name, *command;
		gconf_get_script (cb_data->number, &name, &command);
        	exec_shell_script ( GTK_WINDOW (cb_data->window), 
				    command,
				    name,
				    list);
                path_list_free (list);
		/* We don't need the callback data anymore */
		g_free (cb_data);
		g_free (name);
		g_free (command);
	}
}


void exec_upload_flickr (GtkAction *action, GthWindow *window) {
        GList *list = gth_window_get_file_list_selection (window);
	if (list != NULL) {
        	if (gnome_vfs_is_executable_command_string ("postr"))
	                exec_shell_script ( GTK_WINDOW (window), 
                        	            "postr %F &", 
                                	    "Upload to Flickr",
	                                    list);
        	else
        		_gtk_error_dialog_run (GTK_WINDOW (window), _("The \"Postr\" software package must be installed to use this function."));

		path_list_free (list);
	}
}


void
setup_script_struct (ScriptStruct *s,
		     int	   number)
{
	char *name, *command;
	gconf_get_script (number, &name, &command);

	s->number = number;
	s->short_name = name;
	s->script_text = command;
}


static void
add_scripts (void)
{
        ScriptStruct new_entry;
	int i;

        g_return_if_fail (script_array != NULL);

	for (i = 0 ; i < MAX_SCRIPTS ; i++) {
		setup_script_struct (&new_entry, i);
		g_array_append_vals (script_array, &new_entry, 1);
	}

}


static GtkTreeModel *
create_script_model (void)
{
        gint i = 0;
        GtkListStore *model;
        GtkTreeIter iter;

        /* create array */
        script_array = g_array_sized_new (FALSE, FALSE, sizeof (ScriptStruct), 1);

        add_scripts ();

        /* create list store */
        model = gtk_list_store_new (NUMBER_OF_COLUMNS, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING);

        /* add scripts */
        for (i = 0; i < script_array->len; i++) {
                gtk_list_store_append (model, &iter);

                gtk_list_store_set (model, &iter,
                                    COLUMN_SCRIPT_NUMBER,
                                    g_array_index (script_array, ScriptStruct, i).number,
                                    COLUMN_SHORT_NAME,
                                    g_array_index (script_array, ScriptStruct, i).short_name,
                                    COLUMN_SCRIPT_TEXT,
                                    g_array_index (script_array, ScriptStruct, i).script_text,
                                    -1);
        }

        return GTK_TREE_MODEL (model);
}


static gboolean
separator_row (GtkTreeModel *model,
               GtkTreeIter  *iter,
               gpointer      data)
{
        GtkTreePath *path;
        gint idx;

        path = gtk_tree_model_get_path (model, iter);
        idx = gtk_tree_path_get_indices (path)[0];

        gtk_tree_path_free (path);

        return idx == 5;
}


static void
editing_started (GtkCellRenderer *cell,
                 GtkCellEditable *editable,
                 const gchar     *path,
                 gpointer         data)
{
        gtk_combo_box_set_row_separator_func (GTK_COMBO_BOX (editable),
                                              separator_row, NULL, NULL);
}


static void
cell_edited (GtkCellRendererText *cell,
             const gchar         *path_string,
             const gchar         *new_text,
             gpointer             data)
{
        GtkTreeModel *model = (GtkTreeModel *)data;
        GtkTreePath *path = gtk_tree_path_new_from_string (path_string);
        GtkTreeIter iter;

        gint column = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (cell), "column"));

        gtk_tree_model_get_iter (model, &iter, path);

        switch (column) {

        case COLUMN_SHORT_NAME: {
                gint i;
                gchar *old_text;

                gtk_tree_model_get (model, &iter, column, &old_text, -1);
                g_free (old_text);

                i = gtk_tree_path_get_indices (path)[0];
                g_free (g_array_index (script_array, ScriptStruct, i).short_name);
                g_array_index (script_array, ScriptStruct, i).short_name = g_strdup (new_text);

                gtk_list_store_set (GTK_LIST_STORE (model), &iter, column,
                                    g_array_index (script_array, ScriptStruct, i).short_name, -1);
                break;
        }

        case COLUMN_SCRIPT_TEXT: {
                gint i;
                gchar *old_text;

                gtk_tree_model_get (model, &iter, column, &old_text, -1);
                g_free (old_text);

                i = gtk_tree_path_get_indices (path)[0];
                g_free (g_array_index (script_array, ScriptStruct, i).script_text);
                g_array_index (script_array, ScriptStruct, i).script_text = g_strdup (new_text);

                gtk_list_store_set (GTK_LIST_STORE (model), &iter, column,
                                    g_array_index (script_array, ScriptStruct, i).script_text, -1);
                break;
        }

        }

        gtk_tree_path_free (path);
}


static void
add_columns (GtkTreeView  *treeview,
             GtkTreeModel *script_model)
{
        GtkCellRenderer *renderer;

        /* number column */
        renderer = gtk_cell_renderer_text_new ();
        g_object_set (renderer, "editable", FALSE, NULL);
        g_signal_connect (renderer, "edited", G_CALLBACK (cell_edited), script_model);
        g_object_set_data (G_OBJECT (renderer), "column", GINT_TO_POINTER (COLUMN_SCRIPT_NUMBER));

        gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (treeview),
                        -1, _("Hot key"), renderer,
                        "text", COLUMN_SCRIPT_NUMBER,
                        NULL);


        /* short_name column */
        renderer = gtk_cell_renderer_text_new ();
        g_object_set (renderer, "editable", TRUE, NULL);
        g_signal_connect (renderer, "edited", G_CALLBACK (cell_edited), script_model);
        g_object_set_data (G_OBJECT (renderer), "column", GINT_TO_POINTER (COLUMN_SHORT_NAME));

        gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (treeview),
                        -1, _("Short Name"), renderer,
                        "text", COLUMN_SHORT_NAME,
                        NULL);

        /* script_text column */
        renderer = gtk_cell_renderer_text_new ();
        g_object_set (renderer, "editable", TRUE, NULL);
        g_signal_connect (renderer, "edited", G_CALLBACK (cell_edited), script_model);
        g_object_set_data (G_OBJECT (renderer), "column", GINT_TO_POINTER (COLUMN_SCRIPT_TEXT));

        gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (treeview),
                        -1, _("Script"), renderer,
                        "text", COLUMN_SCRIPT_TEXT,
                        NULL);
}


/* called when the "help" button is clicked. */
static void
help_cb (GtkWidget  *widget,
         DialogData *data)
{
        gthumb_display_help (GTK_WINDOW (data->dialog), "scripts");
}


/* called when the main dialog is closed. */
static void
destroy_cb (GtkWidget  *widget,
            DialogData *data)
{
	g_object_unref (data->gui);
        g_free (data);
}


/* called when the main dialog is closed. */
static void
cancel_cb (GtkWidget  *widget,
           DialogData *data)
{
	gtk_widget_destroy (data->dialog);
}


/* called when the main dialog is closed. */
static void
save_cb (GtkWidget  *widget,
         DialogData *data)
{
	unsigned int i;
	char *pref_key_path = NULL, *pref_key_name = NULL;
	for (i = 0 ; i < MAX_SCRIPTS ; i++) {
		pref_key_path = gconf_script_path (i);
		pref_key_name = gconf_script_name_path (i);
        	eel_gconf_set_string (pref_key_path, g_array_index (script_array, ScriptStruct, i).script_text);
		eel_gconf_set_string (pref_key_name, g_array_index (script_array, ScriptStruct, i).short_name);
		g_free (pref_key_path);
		g_free (pref_key_name);
	}

	data->done_func (data->done_data);
	gtk_widget_destroy (data->dialog);

}


void
dlg_scripts (GthWindow *window, DoneFunc done_func, gpointer done_data)
{
	DialogData   *data;
        GtkWidget    *script_help_button;
        GtkWidget    *script_cancel_button;
        GtkWidget    *script_save_button;
 	GtkWidget    *vbox_tree;
        GtkWidget    *sw;
        GtkWidget    *treeview;
        GtkTreeModel *script_model;
        int	      width, height;

        /* create window, etc */

        data = g_new0 (DialogData, 1);

        data->window = window;
	data->done_func = done_func;
	data->done_data = done_data;
        data->gui = glade_xml_new (GTHUMB_GLADEDIR "/" SCRIPT_GLADE_FILE, NULL,
                                   NULL);

        if (! data->gui) {
                g_warning ("Could not find " SCRIPT_GLADE_FILE "\n");
                g_free (data);
                return;
        }

        data->dialog = glade_xml_get_widget (data->gui, "scripts_dialog");
        script_help_button = glade_xml_get_widget (data->gui, "script_help_button");
        script_cancel_button = glade_xml_get_widget (data->gui, "script_cancel_button");
        script_save_button = glade_xml_get_widget (data->gui, "script_save_button");
	vbox_tree = glade_xml_get_widget (data->gui, "vbox_tree");

        sw = gtk_scrolled_window_new (NULL, NULL);
        gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw),
                                             GTK_SHADOW_ETCHED_IN);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
                                        GTK_POLICY_AUTOMATIC,
                                        GTK_POLICY_AUTOMATIC);
        gtk_box_pack_start (GTK_BOX (vbox_tree), sw, TRUE, TRUE, 0);

        /* create models */
        script_model = create_script_model ();

        /* create tree view */
        treeview = gtk_tree_view_new_with_model (script_model);
        gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (treeview), TRUE);
        gtk_tree_selection_set_mode (gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview)),
                                     GTK_SELECTION_SINGLE);

        add_columns (GTK_TREE_VIEW (treeview), script_model);

        g_object_unref (script_model);

        gtk_container_add (GTK_CONTAINER (sw), treeview);


        g_signal_connect (G_OBJECT (data->dialog),
                          "destroy",
                          G_CALLBACK (destroy_cb),
                          data);

        g_signal_connect (G_OBJECT (script_cancel_button),
                          "clicked",
                          G_CALLBACK (cancel_cb),
                          data);

        g_signal_connect (G_OBJECT (script_help_button),
                          "clicked",
                          G_CALLBACK (help_cb),
                          data);

        g_signal_connect (G_OBJECT (script_save_button),
                          "clicked",
                          G_CALLBACK (save_cb),
                          data);


        gth_get_screen_size (GTK_WINDOW (window), &width, &height);
        gtk_window_set_default_size (GTK_WINDOW (data->dialog),
                                     width * 8 / 10,
                                     height * 7 / 10);

        gtk_window_set_transient_for (GTK_WINDOW (data->dialog),
                                      GTK_WINDOW (window));
        gtk_window_set_modal (GTK_WINDOW (data->dialog), TRUE);
        gtk_widget_show_all (data->dialog);
}


static void add_menu_item_and_action (GtkUIManager   *ui, 
   				      GtkActionGroup *action_group,
				      GthWindow      *window,
				      guint           merge_id,
				      int	      script_number)
{
	GtkAction *action;
	char      *full_label;
	char	  *label;
	char      *name, *command;

	name = g_strdup_printf ("Script_%d",script_number);

	gconf_get_script (script_number, &label, &command);

	full_label = g_strdup_printf ("%d: %s", script_number, label);

	action = g_object_new (GTK_TYPE_ACTION,
      			       "name", name,
			       "label", full_label,
			       "stock_id", GTK_STOCK_EXECUTE,
			       NULL);
	ScriptCallbackData *cb_data = g_new0 (ScriptCallbackData, 1);
	cb_data->number = script_number;
	cb_data->window = window;

        g_signal_connect (action, "activate",
                          G_CALLBACK (exec_script),
                          cb_data);

	gtk_action_group_add_action (action_group, action);
	g_object_unref (action);	
	gtk_ui_manager_add_ui (ui, 
			       merge_id,
			       "/MenuBar/Scripts/User_Defined_Scripts",
			       name,
			       name, 
			       GTK_UI_MANAGER_MENUITEM,
			       FALSE);

	g_free (full_label);
	g_free (label);
	g_free (name);
	g_free (command);
}


guint
generate_script_menu (GtkUIManager   *ui,
		      GtkActionGroup *action_group,
		      GthWindow      *window,
		      guint	      merge_id)
{
	/* Remove the previously-defined menu items and their associated actions */
        if (merge_id != 0) {
		int i;	
		char *action_name;

		for (i = 0; i < MAX_SCRIPTS; i++) {
			action_name = g_strdup_printf ("Script_%d", i);
			gtk_action_group_remove_action (action_group, 
				gtk_action_group_get_action (action_group, action_name));
			g_free (action_name);
		}

	        gtk_ui_manager_remove_ui (ui, merge_id);
	}

	/* Identify this batch of menu additions (for later removal, if required) */
	merge_id = gtk_ui_manager_new_merge_id (ui);                

	unsigned int i;

	for (i = 0 ; i < MAX_SCRIPTS ; i++) {
		add_menu_item_and_action (ui, action_group, window, merge_id, i);
	}

	return merge_id;
}
