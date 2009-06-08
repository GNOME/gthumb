/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2003, 2004, 2005 Free Software Foundation, Inc.
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
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <stdio.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <glade/glade.h>

#include "gth-utils.h"
#include "gtk-utils.h"
#include "gth-window.h"
#include "gth-browser.h"
#include "file-data.h"
#include "file-utils.h"
#include "gfile-utils.h"
#include "dlg-tags.h"
#include "comments.h"
#include "gth-image-list.h"
#include "dlg-file-utils.h"
#include "gconf-utils.h"
#include "preferences.h"
#include "pixbuf-utils.h"
#include "rotation-utils.h"
#include "main.h"

#define GLADE_FILE "gthumb_camera.glade"
#define TAGS_GLADE_FILE "gthumb_comment.glade"
#define TAG_SEPARATOR ";"
#define MAX_TRIES 50
#define THUMB_SIZE 100
#define THUMB_BORDER 14
#define REFRESH_RATE 10


typedef struct _DialogData DialogData;
typedef struct _AsyncOperationData AsyncOperationData;


struct _DialogData {
	GthBrowser          *browser;
	GladeXML            *gui;

	GtkWidget           *dialog;
	GtkWidget           *import_dialog_vbox;
	GtkWidget           *import_preview_scrolledwindow;
	GtkWidget           *destination_filechooserbutton;
	GtkWidget           *subfolder_combobox;
	GtkWidget           *format_code_entry;
	GtkWidget           *format_code_label;
	GtkWidget           *delete_checkbutton;
	GtkWidget           *choose_tags_button;
	GtkWidget           *tags_entry;
	GtkWidget           *import_progressbar;
	GtkWidget           *import_preview_box;
	GtkWidget           *import_reload_button;
        GtkWidget           *import_delete_button;
	GtkWidget           *import_ok_button;
	GtkWidget           *i_commands_table;
	GtkWidget           *reset_exif_tag_on_import_checkbutton;

	GtkWidget           *progress_info_image;
	GtkWidget           *progress_info_label;
	GtkWidget           *progress_info_box;

	GtkWidget           *image_list;

	/**/

	gboolean             view_folder;

	gboolean             delete_from_camera;
	gboolean             adjust_orientation;

	char                *main_dest_folder;
	char                *last_folder;

	gboolean             async_operation;
	gboolean             interrupted;
	gboolean             error;
	gboolean	     renamed_files;
	float                fraction;
	char                *progress_info;
	gboolean             update_ui;
	const char          *msg_icon;
	char                *msg_text;

	GList               *tags_list;
	GList               *delete_list;
	GList               *adjust_orientation_list;
	GList               *saved_images_list;

	/**/

	GThread             *thread;
	guint                check_id;
	GMutex              *data_mutex;
	gboolean             thread_done;

	guint                idle_id;

	AsyncOperationData  *aodata;

	GList               *dcim_dirs;
	gboolean	     dcim_dirs_only;
	char		    *uri;
};


/* Pseudo-Async Operation */


#define ASYNC_STEP_TIMEOUT 60

typedef void (*AsyncOpFunc) (AsyncOperationData *aodata,
			     DialogData         *data);
typedef void (*AsyncAsyncOpFunc) (AsyncOperationData *aodata,
				  DialogData         *data,
				  CopyDoneFunc        done_func);

struct _AsyncOperationData {
	DialogData  *data;
	char        *operation_info;
	GList       *list, *scan;
	int          total, current;
	AsyncOpFunc  init_func, done_func;
	union {
		AsyncOpFunc sync;
		AsyncAsyncOpFunc async;
	} step_func;
	guint        timer_id;
	gboolean     step_is_async;
};


static AsyncOperationData *
async_operation_new (const char  *operation_info,
		     GList       *list,
		     AsyncOpFunc  init_func,
		     AsyncOpFunc  step_func,
		     AsyncOpFunc  done_func,
		     DialogData  *data)
{
	AsyncOperationData *aodata;

	aodata = g_new0 (AsyncOperationData, 1);

	if (operation_info != NULL)
		aodata->operation_info = g_strdup (operation_info);
	else
		aodata->operation_info = NULL;
	aodata->list = list;
	aodata->init_func = init_func;
	aodata->step_func.sync = step_func;
	aodata->done_func = done_func;
	aodata->data = data;
	aodata->total = g_list_length (aodata->list);
	aodata->current = 1;
	aodata->step_is_async = FALSE;

	return aodata;
}


static void
async_operation_free (AsyncOperationData *aodata)
{
	g_free (aodata->operation_info);
	g_free (aodata);
}


static void main_dialog_set_sensitive (DialogData *data, gboolean value);
static void update_info (DialogData *data);
static gboolean async_operation_step (gpointer callback_data);


static void 
async_operation_next_step (AsyncOperationData *aodata)
{
	aodata->current++;
	aodata->scan = aodata->scan->next;
	aodata->timer_id = g_timeout_add (ASYNC_STEP_TIMEOUT,
					  async_operation_step,
					  aodata);	
}


static void
async_step_done (const char     *uri,
		 GError         *error,
                 gpointer        callback_data)
{
	AsyncOperationData *aodata = callback_data;

	update_info (aodata->data);
	async_operation_next_step (aodata);
}


static gboolean
async_operation_step (gpointer callback_data)
{
	AsyncOperationData *aodata = callback_data;
	gboolean            interrupted;

	if (aodata->timer_id != 0) {
		g_source_remove (aodata->timer_id);
		aodata->timer_id = 0;
	}

	g_mutex_lock (aodata->data->data_mutex);
	interrupted = aodata->data->interrupted;
	aodata->data->update_ui = TRUE;
	aodata->data->fraction = (float) aodata->current / aodata->total;
	g_mutex_unlock (aodata->data->data_mutex);

	if ((aodata->scan == NULL) || interrupted) {
		g_mutex_lock (aodata->data->data_mutex);
		aodata->data->async_operation = FALSE;
		g_mutex_unlock (aodata->data->data_mutex);

		main_dialog_set_sensitive (aodata->data, TRUE);

		if (aodata->done_func != NULL)
			(*aodata->done_func) (aodata, aodata->data);
		async_operation_free (aodata);

		return FALSE;
	}

	if (aodata->step_is_async)
		(*aodata->step_func.async) (aodata, aodata->data, async_step_done);
	else {
		(*aodata->step_func.sync) (aodata, aodata->data);
		update_info (aodata->data);
		async_operation_next_step (aodata);
	}

	return FALSE;
}


static void
async_operation_start (AsyncOperationData *aodata)
{
	aodata->timer_id = 0;
	aodata->scan = aodata->list;
	aodata->current = 1;
	if (aodata->init_func)
		(*aodata->init_func) (aodata, aodata->data);

	main_dialog_set_sensitive (aodata->data, FALSE);

	g_mutex_lock (aodata->data->data_mutex);
	aodata->data->async_operation = TRUE;
	aodata->data->interrupted = FALSE;
	if (aodata->data->progress_info != NULL)
		g_free (aodata->data->progress_info);
	aodata->data->progress_info = g_strdup (aodata->operation_info);
	g_mutex_unlock (aodata->data->data_mutex);

	async_operation_step (aodata);
}


static void
async_operation_cancel (AsyncOperationData *aodata)
{
	if (aodata->timer_id == 0)
		return;
	g_source_remove (aodata->timer_id);
	aodata->timer_id = 0;
	async_operation_free (aodata);
}


/**/


/* called when the main dialog is closed. */
static void
destroy_cb (GtkWidget  *widget,
	    DialogData *data)
{
	GtkWidget *browser = (GtkWidget*) data->browser;
	gboolean   thread_done;

	/* Remove check. */

	if (data->check_id != 0) {
		g_source_remove (data->check_id);
		data->check_id = 0;
	}

	if (data->idle_id != 0) {
		g_source_remove (data->idle_id);
		data->idle_id = 0;
	}

	if (data->aodata != NULL) {
		async_operation_cancel (data->aodata);
		data->aodata = NULL;
	}

	/**/

	g_mutex_lock (data->data_mutex);
	thread_done = data->thread_done;
	g_mutex_unlock (data->data_mutex);

	if (! thread_done && (data->thread != NULL))
		g_thread_join (data->thread);

	g_mutex_free (data->data_mutex);

	/**/

	if (data->view_folder) {
		if (browser == NULL) {
			browser = gth_browser_get_current_browser ();
			if (browser != NULL)
				gth_browser_go_to_directory (GTH_BROWSER (browser), data->last_folder);
			else
				browser = gth_browser_new (data->last_folder);
			gtk_window_present (GTK_WINDOW (browser));
		} else
			gth_browser_go_to_directory (data->browser, data->last_folder);
	}

	/**/

	g_free (data->progress_info);
	g_free (data->msg_text);
	g_free (data->main_dest_folder);
	g_free (data->last_folder);
	g_free (data->uri);

	gfile_list_free (data->dcim_dirs);
	path_list_free (data->tags_list);
	gfile_list_free (data->delete_list);
	gfile_list_free (data->adjust_orientation_list);
	gfile_list_free (data->saved_images_list);
	g_object_unref (data->gui);
	g_free (data);

	/**/

	if (ImportPhotos) {
		ImportPhotos = FALSE;
		if (browser != NULL)
			gth_window_close (GTH_WINDOW (browser));
		else
			gtk_main_quit ();
	}
}


static void
task_terminated (DialogData *data)
{
	gtk_widget_hide (data->progress_info_box);
	gtk_widget_hide (data->import_progressbar);
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (data->import_progressbar), 0.0);
	gtk_progress_bar_set_text (GTK_PROGRESS_BAR (data->import_progressbar), "");

	gtk_image_set_from_stock (GTK_IMAGE (data->progress_info_image),
				  GTK_STOCK_DIALOG_INFO,
				  GTK_ICON_SIZE_BUTTON);
}


static void
display_error_dialog (DialogData *data,
		      const char *msg1,
		      const char *msg2)
{
	GtkWidget *dialog;

	dialog = _gtk_message_dialog_new (GTK_WINDOW (data->dialog),
					  0,
					  GTK_STOCK_DIALOG_WARNING,
					  msg1,
					  msg2,
					  GTK_STOCK_CLOSE, GTK_RESPONSE_CANCEL,
					  NULL);

	g_signal_connect (G_OBJECT (dialog),
			  "response",
			  G_CALLBACK (gtk_widget_destroy),
			  NULL);

	gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
	gtk_widget_show (dialog);
}


#define RECURSION_LIMIT 3

static GList *
gfile_import_dir_list_recursive (GFile      *gfile,
				 GList      *dcim_dirs,
				 const char *filter,
				 int         n)
{
        GFileEnumerator *file_enum;
        GFileInfo       *info;
	GError		*error = NULL;

	n++; 

        file_enum = g_file_enumerate_children (gfile,
                                               G_FILE_ATTRIBUTE_STANDARD_NAME ","
                                               G_FILE_ATTRIBUTE_STANDARD_TYPE,
                                               0, NULL, &error);

        if (error != NULL) {
                gfile_warning ("Error while reading contents of directory", gfile, error);
                g_error_free (error);
        }

	if (file_enum == NULL)
		return dcim_dirs;

        while ((info = g_file_enumerator_next_file (file_enum, NULL, NULL)) != NULL) {
                GFile *child;
		char  *utf8_path;

                child = g_file_get_child (gfile, g_file_info_get_name (info));
		utf8_path = g_file_get_parse_name (child);

		debug (DEBUG_INFO, "Scanning directory %s, recursion level %d", utf8_path, n);

                switch (g_file_info_get_file_type (info)) {
                case G_FILE_TYPE_DIRECTORY:
			if (!(n > RECURSION_LIMIT) &&
			   (!filter || (filter && strstr (utf8_path, filter)))) {
				if (strstr (utf8_path, "dcim") || strstr (utf8_path, "DCIM")) {
					debug (DEBUG_INFO, "found DCIM dir at %s", utf8_path);
	        	        	dcim_dirs = g_list_prepend (dcim_dirs, g_file_dup (child));
				} else {
					debug (DEBUG_INFO, "no DCIM dir at %s", utf8_path);
					if (utf8_path[0] != '.') {
						dcim_dirs = gfile_import_dir_list_recursive (child, dcim_dirs, filter, n);
					}
				}
			} else {
				debug (DEBUG_INFO, "not checking %s", utf8_path);
			}
                        break;
                default:
                        break;
                }

		g_free (utf8_path);
                g_object_unref (child);
                g_object_unref (info);
        }

        g_object_unref (file_enum);

        return dcim_dirs;
}


static GList *
gfile_import_file_list_recursive (GFile  *gfile,
                                  GList  *files)
{
        GFileEnumerator *file_enum;
        GFileInfo       *info;
	GError		*error = NULL;

        file_enum = g_file_enumerate_children (gfile,
                                               G_FILE_ATTRIBUTE_STANDARD_NAME ","
                                               G_FILE_ATTRIBUTE_STANDARD_TYPE,
                                               0, NULL, &error);

        if (error != NULL) {
                gfile_warning ("Error while reading contents of directory", gfile, error);
                g_error_free (error);
                return files;
        }

	files = g_list_reverse (files);

        while ((info = g_file_enumerator_next_file (file_enum, NULL, NULL)) != NULL) {
                GFile      *child;
		const char *mime_type;

                child = g_file_get_child (gfile, g_file_info_get_name (info));

                switch (g_file_info_get_file_type (info)) {
                case G_FILE_TYPE_DIRECTORY:
                        files = gfile_import_file_list_recursive (child, files);
                        break;
                case G_FILE_TYPE_REGULAR:
			mime_type = gfile_get_mime_type (child, TRUE);
		        if ((mime_type_is_image (mime_type) ||
		             mime_type_is_video (mime_type) ||
		             mime_type_is_audio (mime_type)))
	                        files = g_list_prepend (files, g_file_dup (child));
                        break;
                default:
                        break;
                }

                g_object_unref (child);
                g_object_unref (info);
        }

	files = g_list_reverse (files);
        g_object_unref (file_enum);

        return files;
}


static GList *
get_all_files (DialogData *data)
{
	GFile *gfile;
	GList *file_list = NULL;
	GList *scan;

	if (data->dcim_dirs != NULL) {
		gfile_list_free (data->dcim_dirs);	
		data->dcim_dirs = NULL;
	}

	if (data->uri && !path_is_dir (data->uri)) {
		_gtk_info_dialog_run (GTK_WINDOW (data->dialog), _("%s is not a valid directory, scanning for attached devices instead"), data->uri);
		g_free (data->uri);
		data->uri = NULL;
	}

	if (data->uri) {
		if (data->dcim_dirs_only) {
			gfile = gfile_new (data->uri);
			data->dcim_dirs = gfile_import_dir_list_recursive (gfile, data->dcim_dirs, NULL, 0);
	                g_object_unref (gfile);
		} else {
			data->dcim_dirs = g_list_prepend (data->dcim_dirs, gfile_new (data->uri));
			}
		if (!file_list) {
			_gtk_info_dialog_run (GTK_WINDOW (data->dialog), _("No files found in %s, scanning for attached devices instead"), data->uri);
		}
	}

	if (!file_list) {
		char *gvfs_dir = g_strconcat (g_get_home_dir (), "/", ".gvfs", NULL);
		GFile *gfile = gfile_new (gvfs_dir);
		data->dcim_dirs = gfile_import_dir_list_recursive (gfile, data->dcim_dirs, "gphoto", 0);
		g_object_unref (gfile);
		g_free (gvfs_dir);

                gfile = gfile_new ("/media");
                data->dcim_dirs = gfile_import_dir_list_recursive (gfile, data->dcim_dirs, NULL, 0);
                g_object_unref (gfile);
	}

	for (scan = data->dcim_dirs; scan; scan = scan->next) {
		gfile = (GFile *) scan->data;
		file_list = gfile_import_file_list_recursive (gfile, file_list);
	}

	return file_list;
}


static void
update_info (DialogData *data)
{
	gboolean    update_ui;
	gfloat      fraction = -0.1;
	char       *progress_info = NULL;
	char       *msg_text = NULL;
	const char *msg_icon = GTK_STOCK_DIALOG_ERROR;

	g_mutex_lock (data->data_mutex);
	update_ui = data->update_ui;
	if (update_ui) {
		fraction = data->fraction;
		data->fraction = -1.0;
		if (data->progress_info != NULL) {
			progress_info = g_strdup (data->progress_info);
			g_free (data->progress_info);
			data->progress_info = NULL;
		}
		if (data->msg_text != NULL) {
			msg_text = g_strdup (data->msg_text);
			g_free (data->msg_text);
			data->msg_text = NULL;
		}
		msg_icon = data->msg_icon;
		data->update_ui = FALSE;
	}
	g_mutex_unlock (data->data_mutex);

	/**/

	if (update_ui) {
		if (fraction > -0.1) {
			gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (data->import_progressbar),  fraction);
			gtk_widget_show (data->import_progressbar);
		}

		if (progress_info != NULL) {
			gtk_progress_bar_set_text (GTK_PROGRESS_BAR (data->import_progressbar),
						   progress_info);
			g_free (progress_info);
			gtk_widget_show (data->import_progressbar);
		}

		if (msg_text != NULL) {
			char *esc, *markup;

			esc = g_markup_escape_text (msg_text, -1);
			markup = g_strdup_printf ("<i>%s</i>", esc);

			gtk_image_set_from_stock (GTK_IMAGE (data->progress_info_image),
						  msg_icon,
						  GTK_ICON_SIZE_BUTTON);
			gtk_label_set_markup (GTK_LABEL (data->progress_info_label), markup);

			g_free (esc);
			g_free (markup);
			g_free (msg_text);

			gtk_widget_show (data->progress_info_box);
		}

		gdk_flush ();
	}
}


static void
main_dialog_set_sensitive (DialogData *data,
			   gboolean    value)
{
	gtk_widget_set_sensitive (data->import_ok_button, value);
	gtk_widget_set_sensitive (data->import_reload_button, value);
	gtk_widget_set_sensitive (data->import_delete_button, value);
	gtk_widget_set_sensitive (data->i_commands_table, value);
	gtk_widget_set_sensitive (data->reset_exif_tag_on_import_checkbutton, value);
}


/* load_images_preview */


static void
load_images_preview__init (AsyncOperationData *aodata,
			   DialogData         *data)
{
	task_terminated (data);
}


static GdkPixbuf*
gfile_get_preview (GFile *gfile, int size)
{
        GdkPixbuf    *pixbuf = NULL;
	GdkPixbuf    *rotated = NULL;
        GIcon        *gicon;
        GtkIconTheme *theme;
	GFileInfo    *info;

        theme = gtk_icon_theme_get_default ();

        if (gfile == NULL)
                return NULL;

	char *local_path = g_file_get_path (gfile);
	debug (DEBUG_INFO, "need preview for %s", local_path);

        info = g_file_query_info (gfile,
                                  G_FILE_ATTRIBUTE_PREVIEW_ICON ","
				  G_FILE_ATTRIBUTE_STANDARD_ICON,
                                  G_FILE_QUERY_INFO_NONE,
                                  NULL,
                                  NULL);

	if (info == NULL) {
		debug (DEBUG_INFO, "no info found for %s", local_path);
		g_free (local_path);
		return NULL;
	}

        gicon = (GIcon *) g_file_info_get_attribute_object (info, G_FILE_ATTRIBUTE_PREVIEW_ICON);

	if (gicon == NULL)
		debug (DEBUG_INFO, "no preview icon found for %s", local_path);

	if (gicon == NULL && local_path) {
		/* TODO: is there a fast way to extract an embedded thumbnail,
                   instead of reading the entire file and scaling it down ? */
		pixbuf = gdk_pixbuf_new_from_file_at_scale (local_path, size, size, TRUE, NULL);
	}

	if ((gicon == NULL) && (pixbuf == NULL)) {
		debug (DEBUG_INFO, "no scaled pixbuf found for %s", local_path);	
		gicon = g_file_info_get_icon (info);
	}

        if ((gicon == NULL) && (pixbuf == NULL)) {
		debug (DEBUG_INFO, "no generic icon found for %s", local_path);
	}

	if (gicon != NULL) {
		GtkIconInfo *icon_info;
		icon_info = gtk_icon_theme_lookup_by_gicon (gtk_icon_theme_get_default (),
							    gicon,
							    size,
							    0);
		pixbuf = gtk_icon_info_load_icon (icon_info, NULL);
		if (pixbuf == NULL)
			debug (DEBUG_INFO, "valid gicon, but couldn't get pixbuf for %s", local_path); 
		gtk_icon_info_free (icon_info);
	}

	if (pixbuf == NULL) {
		debug (DEBUG_INFO, "no preview pixbuf created for %s", local_path);
	} else {
		rotated = gdk_pixbuf_apply_embedded_orientation (pixbuf);
		g_object_unref (pixbuf);
	}

	g_object_unref (info);
	g_free (local_path);

        return rotated;
}


static void
load_images_preview__step (AsyncOperationData *aodata,
			   DialogData         *data)
{
	GdkPixbuf *pixbuf;
	FileData  *fd;

	pixbuf = gfile_get_preview ((GFile *) aodata->scan->data, THUMB_SIZE);
	fd = file_data_new_from_gfile ((GFile *) aodata->scan->data);
	
	gth_image_list_append_with_data (GTH_IMAGE_LIST (data->image_list),
					 pixbuf,
					 fd->utf8_name,
					 NULL,
					 NULL,
					 fd);
	g_object_unref (pixbuf);
	file_data_unref (fd);
}


static void
load_images_preview__done (AsyncOperationData *aodata,
			   DialogData         *data)
{
	gfile_list_free (aodata->list);
	task_terminated (data);
	data->aodata = NULL;
}


static void
load_images_preview (DialogData *data)
{
	GList    *file_list;
	gboolean  error;

	gth_image_list_clear (GTH_IMAGE_LIST (data->image_list));

	g_mutex_lock (data->data_mutex);
	data->error = FALSE;
	g_mutex_unlock (data->data_mutex);

	file_list = get_all_files (data);

	g_mutex_lock (data->data_mutex);
	error = data->error;
	g_mutex_unlock (data->data_mutex);

	if (error) {
		update_info (data);
		return;
	}

	if (file_list == NULL) {
		gtk_widget_hide (data->import_preview_box);
		gtk_image_set_from_stock (GTK_IMAGE (data->progress_info_image),
					  GTK_STOCK_DIALOG_WARNING,
					  GTK_ICON_SIZE_BUTTON);
		gtk_label_set_text (GTK_LABEL (data->progress_info_label), _("No images found"));
		gtk_widget_show (data->progress_info_box);
		gtk_window_set_resizable (GTK_WINDOW (data->dialog), FALSE);
		return;
	} 
	else {
		gtk_widget_show (data->import_preview_box);
		gtk_widget_hide (data->progress_info_box);
		gtk_window_set_resizable (GTK_WINDOW (data->dialog), TRUE);
	}

	data->aodata = async_operation_new (NULL,
					    file_list,
					    load_images_preview__init,
					    load_images_preview__step,
					    load_images_preview__done,
					    data);
	async_operation_start (data->aodata);
}


static char*
get_folder_name (DialogData *data)
{
	char *destination;
	char *subfolder_name = NULL;
	char *path;
	GthSubFolder subfolder_value;

	destination = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (data->destination_filechooserbutton));
	eel_gconf_set_path (PREF_PHOTO_IMPORT_DESTINATION, destination);

	if (! dlg_check_folder (GTH_WINDOW (data->browser), destination)) {
		g_free (destination);
		return NULL;
	}

	subfolder_value = gtk_combo_box_get_active (GTK_COMBO_BOX (data->subfolder_combobox));

	/* If grouping by current date/time is enabled, we can append the current date and 
	   time to the folder name directly. For modes based on the exif date, the 
	   destination folder may be different for each image, so we deal with that later. */
	if (subfolder_value == GTH_IMPORT_SUBFOLDER_GROUP_NOW) {
		time_t     now;
		struct tm *tm;
		char       time_txt[50 + 1];

		time (&now);
		tm = localtime (&now);
		strftime (time_txt, 50, "%Y-%m-%d--%H.%M.%S", tm);
		subfolder_name = g_strdup (time_txt);
	}

	pref_set_import_subfolder (subfolder_value);

	if (subfolder_value == GTH_IMPORT_SUBFOLDER_GROUP_CUSTOM)
		eel_gconf_set_string (PREF_PHOTO_IMPORT_CUSTOM_FORMAT, gtk_entry_get_text (GTK_ENTRY (data->format_code_entry)));

	if (subfolder_name != NULL) {
		path = g_build_filename (destination, subfolder_name, NULL);
		g_free (subfolder_name);
	} else {
		path = g_strdup (destination);
	}
	g_free (destination);

	return path;
}


static char*
get_file_name (DialogData *data,
	       const char *path,
	       const char *main_dest_folder)
{
	char *file_name;
	char *file_path;
	int   m = 1;

	file_name = g_strdup (file_name_from_path (path));
	file_path = g_build_filename (main_dest_folder, file_name, NULL);

	while (path_is_file (file_path)) {
		char *test_name;
		test_name = g_strdup_printf ("%d.%s", m++, file_name);
		g_free (file_path);
		file_path = g_build_filename (main_dest_folder, test_name, NULL);
		g_free (test_name);
		data->renamed_files = TRUE;
	}

	g_free (file_name);

	return file_path;
}


static void
add_tags_to_image (DialogData *data,
                   const char *local_file)
{
	CommentData *cdata;
	GList       *scan;
	char        *uri;
	
	if (data->tags_list == NULL)
		return;

	uri = get_uri_from_local_path (local_file);
	cdata = comments_load_comment (uri, FALSE);
	if (cdata == NULL)
		cdata = comment_data_new ();

	for (scan = data->tags_list; scan; scan = scan->next) {
		const char *k = scan->data;
		comment_data_add_keyword (cdata, k);
	}

	comments_save_tags (uri, cdata);
	comment_data_free (cdata);
	
	g_free (uri);
}


static gboolean
notify_file_creation_cb (gpointer cb_data)
{
	DialogData *data = cb_data;

	g_source_remove (data->idle_id);
	data->idle_id = 0;

	if (data->saved_images_list != NULL) {
		gth_monitor_notify_update_files (GTH_MONITOR_EVENT_CREATED, data->saved_images_list);
		gfile_list_free (data->saved_images_list);
		data->saved_images_list = NULL;
	}

	gth_monitor_resume ();

	return FALSE;
}


static void
adjust_orientation__step (AsyncOperationData *aodata,
			  DialogData         *data)
{
	char     *uri;	
	gboolean  success = TRUE;
	GFile    *gfile = aodata->scan->data;

	uri = g_file_get_parse_name (gfile);
	
	if (file_is_image (uri, TRUE)) {
		FileData     *fd;
		GthTransform  transform;

		fd = file_data_new_from_path (uri);
		if (data->msg_text != NULL)
	                g_free (data->msg_text);
		data->msg_text = g_strdup_printf (_("Adjusting orientation of \'%s\'."), fd->utf8_name);

		transform = get_orientation_from_fd (fd);
		if (image_is_jpeg (uri))
			success = apply_transformation_jpeg (fd, transform, JPEG_MCU_ACTION_DONT_TRIM, NULL);
		else
			success = apply_transformation_generic (fd, transform, NULL);
		file_data_unref (fd);
	}

	if (! success)
		data->error = TRUE;

	g_free (uri);
}


static void
adjust_orientation__done (AsyncOperationData *aodata,
			  DialogData         *data)
{
	gboolean interrupted;

	data->aodata = NULL;

	g_mutex_lock (data->data_mutex);
	interrupted = data->interrupted;
	g_mutex_unlock (data->data_mutex);

	data->idle_id = g_idle_add (notify_file_creation_cb, data);

	if (interrupted)
		return;

	data->view_folder = TRUE;

	if (ImportPhotos) {
		ImportPhotos = FALSE;
		if (data->browser != NULL)
			gtk_widget_show (GTK_WIDGET (data->browser));
	}

	gtk_widget_destroy (data->dialog);
}


static void
save_images__init (AsyncOperationData *aodata,
		   DialogData         *data)
{
	gth_monitor_pause ();

	data->renamed_files = FALSE;

	if (data->adjust_orientation_list != NULL) {
		gfile_list_free (data->adjust_orientation_list);
		data->adjust_orientation_list = NULL;
	}
	if (data->saved_images_list != NULL) {
		gfile_list_free (data->saved_images_list);
		data->saved_images_list = NULL;
	}
}


static void
save_images__step (AsyncOperationData *aodata,
		   DialogData         *data)
{
	char         *initial_dest_path;
	GFile        *initial_dest_gfile;
	char         *final_dest_path;
	GFile        *final_dest_gfile;
	time_t        exif_date;
	GthSubFolder  subfolder_value;
	char	     *temp_dir = NULL;
	gboolean      error_found = FALSE;
	FileData     *folder_fd;
	GError       *error = NULL;

	GFile *gfile = aodata->scan->data;
	char *path = g_file_get_parse_name (gfile);

	subfolder_value = gtk_combo_box_get_active (GTK_COMBO_BOX (data->subfolder_combobox));
	folder_fd = file_data_new_from_path (data->main_dest_folder);

	/* When grouping by exif date, we need a temporary directory to upload the
	   photo to. The exif date tags are then read, and the file is then moved
	   to its final destination. */
	if ( (subfolder_value == GTH_IMPORT_SUBFOLDER_GROUP_DAY) || 
	     (subfolder_value == GTH_IMPORT_SUBFOLDER_GROUP_MONTH) || 
	     (subfolder_value == GTH_IMPORT_SUBFOLDER_GROUP_CUSTOM)) {
		temp_dir = get_temp_dir_name ();
		if (temp_dir == NULL) {
			error_found = TRUE;
		} else {
			initial_dest_path = get_temp_file_name (temp_dir, NULL);
		}
	} else {
		/* Otherwise, the images go straight into the destination folder */
		initial_dest_path = get_file_name (data, path, folder_fd->local_path);
	}

	if (initial_dest_path == NULL)
		error_found = TRUE;

	initial_dest_gfile = gfile_new (initial_dest_path);

	debug (DEBUG_INFO, "import file copy: %s to %s", path, initial_dest_path);
	if (!gfile_copy (gfile, initial_dest_gfile, FALSE, &error)) {
		if (error) {
			display_error_dialog (data, _("Import failed"), error->message);
			g_clear_error (&error);
		}
		error_found = TRUE;
	}

	if (error_found == FALSE) {

		if ( (subfolder_value == GTH_IMPORT_SUBFOLDER_GROUP_DAY) || 
		     (subfolder_value == GTH_IMPORT_SUBFOLDER_GROUP_MONTH) || 
		     (subfolder_value == GTH_IMPORT_SUBFOLDER_GROUP_CUSTOM)) {
			char     *dest_folder;
			FileData *file;

			/* Name a subfolder based on the exif date */
                        file = file_data_new_from_path (initial_dest_path);
                        file_data_update_all (file, FALSE);
			exif_date = get_exif_time_or_mtime (file);
			file_data_unref (file);

			if (exif_date != (time_t) 0) {
				struct tm  *exif_tm = localtime(&exif_date);
				char        dest_subfolder[255 + 1];
				size_t	    ret = 0;
				char       *tmp;

				if (subfolder_value == GTH_IMPORT_SUBFOLDER_GROUP_DAY)
					ret = strftime (dest_subfolder, 255, "%Y-%m-%d", exif_tm);
				else if (subfolder_value == GTH_IMPORT_SUBFOLDER_GROUP_MONTH)
					ret = strftime (dest_subfolder, 255, "%Y-%m", exif_tm);
				else 
					ret = strftime (dest_subfolder, 
						  255, 
						  gtk_entry_get_text (GTK_ENTRY (data->format_code_entry)), 
						  exif_tm);

				if (ret == 0)
					dest_subfolder[0] = '\0';

				/* Strip out weird characters possible in custom input. */

				tmp = dest_subfolder;
				while (*tmp) {
					if ((*tmp == '\t') ||
					    (*tmp == '\n') ||
					    (*tmp == '~')  ||
					    (*tmp == '\'') ||
					    (*tmp == '"')  ||
					    (*tmp == '|')  ||
					    (*tmp == '`')  ||
					    (*tmp == '/')  ||
					    (*tmp == '#')  ||
					    (*tmp == '%')  ||
					    (*tmp == '$')  ||
					    (*tmp == '&')     )
						*tmp = ' ';
					tmp++;
				}

				dest_folder = g_build_filename (folder_fd->local_path, dest_subfolder, NULL);
			} else {
				/* If no exif data was found, use the base folder */
				dest_folder = g_strdup (folder_fd->local_path);
			}

			final_dest_path = get_file_name (data, path, dest_folder);
			final_dest_gfile = gfile_new (final_dest_path);

			/* Create the subfolder if necessary, and move the 
			   temporary file to it */
			if (ensure_dir_exists (dest_folder) ) {
				debug (DEBUG_INFO, "import file move %s to %s", initial_dest_path, final_dest_path);
				if (!gfile_move (initial_dest_gfile, final_dest_gfile, FALSE, &error)) {
					display_error_dialog (data, _("Import failed"), error->message); 
					g_clear_error (&error);
					error_found = TRUE;
				}
			} else {
				error_found = TRUE;
			}

			g_free (data->last_folder);
			data->last_folder = g_strdup (dest_folder);

			g_free (dest_folder);
		} else {
			final_dest_path = g_strdup (initial_dest_path);
			final_dest_gfile = gfile_new (final_dest_path);
		}
	}

	/* Adjust the photo orientation based on the exif 
	   orientation tag, if requested */
	if (!error_found) {
		if (data->delete_from_camera) {
			debug (DEBUG_INFO, "import delete: %s", path);
			g_file_delete (gfile, NULL, &error);
	                if (error) {
	                        display_error_dialog (data, _("Could not delete photos on the camera - further deletes are disabled"), error->message);
				data->delete_from_camera = FALSE;
        	                g_clear_error (&error);
			}
                }

		if (data->adjust_orientation) 
			data->adjust_orientation_list = g_list_prepend (data->adjust_orientation_list,
									g_file_dup (final_dest_gfile));

		data->saved_images_list = g_list_prepend (data->saved_images_list, g_file_dup (final_dest_gfile));
		add_tags_to_image (data, final_dest_path);

	} else {
		g_mutex_lock (data->data_mutex);
		data->error = TRUE;
		data->interrupted = TRUE;
		g_mutex_unlock (data->data_mutex);
	}

	/* Clean up temp file and dir */
	if (temp_dir != NULL) {
		dir_remove_recursive (temp_dir);
		g_free (temp_dir);
	}

	g_free (path);
	g_free (final_dest_path);
	g_free (initial_dest_path);
	file_data_unref (folder_fd);
	g_object_unref (initial_dest_gfile);
	g_object_unref (final_dest_gfile);
}


static void
save_images__done (AsyncOperationData *aodata,
		   DialogData         *data)
{
	gboolean interrupted;
	gboolean error;

	if (data->renamed_files)
		_gtk_info_dialog_run (GTK_WINDOW (data->dialog), 
				      "Warning - one or more files were renamed (by adding a numeric prefix) to avoid overwriting existing files.");
	
	g_mutex_lock (data->data_mutex);
	interrupted = data->interrupted;
	error = data->error;
	g_mutex_unlock (data->data_mutex);

	data->aodata = NULL;
	if (interrupted || error)
		return;
	data->aodata = async_operation_new (NULL,
				  	    data->adjust_orientation_list,
					    NULL,
					    adjust_orientation__step,
					    adjust_orientation__done,
					    data);
	async_operation_start (data->aodata);
}


static void
cancel_clicked_cb (GtkButton  *button,
		   DialogData *data)
{
	gboolean async_operation;

	g_mutex_lock (data->data_mutex);
	async_operation = data->async_operation;
	g_mutex_unlock (data->data_mutex);

	if (async_operation) {
		g_mutex_lock (data->data_mutex);
		data->interrupted = TRUE;
		g_mutex_unlock (data->data_mutex);
	} else
		gtk_widget_destroy (data->dialog);
}


static void
ok_clicked_cb (GtkButton  *button,
	       DialogData *data)
{
	GList    *file_list = NULL, *scan;
	GList    *sel_list;
	gboolean  error;
	goffset   total_size = 0;
	FileData *folder_fd;

	/**/

	g_free (data->main_dest_folder);
	g_free (data->last_folder);

	data->main_dest_folder = get_folder_name (data);
	data->last_folder = g_strdup (data->main_dest_folder);

	if (data->main_dest_folder == NULL)
		return;

	data->delete_from_camera = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->delete_checkbutton));
	data->adjust_orientation = eel_gconf_get_boolean (PREF_PHOTO_IMPORT_RESET_EXIF_ORIENTATION, TRUE);

	eel_gconf_set_boolean (PREF_PHOTO_IMPORT_DELETE, data->delete_from_camera);

	/**/

	g_mutex_lock (data->data_mutex);
	error = data->error;
	g_mutex_unlock (data->data_mutex);

	if (error) {
		update_info (data);
		return;
	}

	sel_list = gth_image_list_get_selection (GTH_IMAGE_LIST (data->image_list));
	if (sel_list == NULL) 
		sel_list = gth_image_list_get_list (GTH_IMAGE_LIST (data->image_list));

	if (sel_list != NULL) {
		for (scan = sel_list; scan; scan = scan->next) {
			FileData *fd = scan->data;
			file_list = g_list_prepend (file_list, g_file_dup (fd->gfile));
		}
		if (file_list != NULL)
			file_list = g_list_reverse (file_list);
		file_data_list_free (sel_list);
	}

	if (file_list == NULL) {
		display_error_dialog (data,
				      _("Could not import photos"),
				      _("No images found"));
		return;
	}

	folder_fd = file_data_new_from_path (data->main_dest_folder);
	if (!file_data_has_local_path (folder_fd, GTK_WINDOW (data->dialog)) || 
	    !ensure_dir_exists (folder_fd->local_path)) {
		char *msg;
		msg = g_strdup_printf (_("Could not create the folder \"%s\": %s"),
				       folder_fd->utf8_name,
				       errno_to_string ());
		display_error_dialog (data, _("Could not import photos"), msg);

		g_free (msg);
		g_free (data->main_dest_folder);
		data->main_dest_folder = NULL;
		gfile_list_free (file_list);
		file_data_unref (folder_fd);
		return;
	}
	file_data_unref (folder_fd);

	for (scan = file_list; scan; scan = scan->next) {
		const char *utf8_path = scan->data;
		FileData   *fd;

		fd = file_data_new_from_path (utf8_path);
		total_size += fd->size;
		file_data_unref (fd);
	}

	if (get_destination_free_space (data->main_dest_folder) < total_size) {
		display_error_dialog (data,
				      _("Could not import photos"),
				      _("Not enough free space left on disk"));

		g_free (data->main_dest_folder);
		data->main_dest_folder = NULL;
		gfile_list_free (file_list);
		return;
	}

	data->aodata = async_operation_new (NULL,
					    file_list,
					    save_images__init,
					    save_images__step,
					    save_images__done,
					    data);
	async_operation_start (data->aodata);
}


static void
choose_tags__done (gpointer callback_data)
{
	DialogData *data = callback_data;
	GString    *tags;
	GList      *scan;

	tags = g_string_new ("");

	for (scan = data->tags_list; scan; scan = scan->next) {
		char *name = scan->data;
		if (tags->len > 0)
			tags = g_string_append (tags, TAG_SEPARATOR " ");
		tags = g_string_append (tags, name);
	}

	gtk_entry_set_text (GTK_ENTRY (data->tags_entry), tags->str);
	g_string_free (tags, TRUE);
}


static void
choose_tags_cb (GtkButton  *button,
                DialogData *data)
{
	dlg_choose_tags (GTK_WINDOW (data->dialog),
                         NULL,
                         data->tags_list,
                         &(data->tags_list),
                         NULL,
                         choose_tags__done,
                         data);
}


static void
reset_exif_tag_on_import_cb (GtkButton  *button,
					    DialogData *data)
{
	eel_gconf_set_boolean (PREF_PHOTO_IMPORT_RESET_EXIF_ORIENTATION,
		gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->reset_exif_tag_on_import_checkbutton)));
}


static void
import_reload_cb (GtkButton  *button,
                  DialogData *data)
{
	load_images_preview (data);
}


static void
subfolder_mode_changed_cb (GtkComboBox  *subfolder_combobox,
		           DialogData   *data)
{
	GthSubFolder subfolder_value;

	subfolder_value = gtk_combo_box_get_active (GTK_COMBO_BOX (data->subfolder_combobox));

        if (subfolder_value == GTH_IMPORT_SUBFOLDER_GROUP_CUSTOM) {
		gtk_widget_set_sensitive (data->format_code_entry, TRUE);
		gtk_widget_set_sensitive (data->format_code_label, TRUE);
		gtk_editable_set_editable (GTK_EDITABLE (data->format_code_entry), TRUE);
	}
	else {
		gtk_widget_set_sensitive (data->format_code_entry, FALSE);
		gtk_widget_set_sensitive (data->format_code_label, FALSE);
		gtk_editable_set_editable (GTK_EDITABLE (data->format_code_entry), FALSE);
	}

	switch (subfolder_value) {
		case GTH_IMPORT_SUBFOLDER_GROUP_CUSTOM:
			gtk_entry_set_text (GTK_ENTRY (data->format_code_entry), 
					    eel_gconf_get_string (PREF_PHOTO_IMPORT_CUSTOM_FORMAT,
						    	          _("Put name or format code here")));
			break;
		case GTH_IMPORT_SUBFOLDER_GROUP_DAY:
                        gtk_entry_set_text (GTK_ENTRY (data->format_code_entry), "%Y-%m-%d");
                        break;
                case GTH_IMPORT_SUBFOLDER_GROUP_MONTH:
                        gtk_entry_set_text (GTK_ENTRY (data->format_code_entry), "%Y-%m");
                        break;
                case GTH_IMPORT_SUBFOLDER_GROUP_NOW:
                        gtk_entry_set_text (GTK_ENTRY (data->format_code_entry), "%Y-%m-%d--%H.%M.%S");
                        break;
		default:
			gtk_entry_set_text (GTK_ENTRY (data->format_code_entry), "No subfolders");
                        break;
	}

}


/* delete_imported_images */


static void
delete_imported_images__init (AsyncOperationData *aodata,
                              DialogData         *data)
{
}


static void
delete_imported_images__step (AsyncOperationData *aodata,
                              DialogData         *data)
{
	GError *error = NULL;
	g_file_delete ((GFile *) aodata->scan->data, NULL, &error);
	if (error) {
		display_error_dialog (data, _("File delete failed"), error->message);
		g_error_free (error);
	}
}


static void
delete_imported_images__done (AsyncOperationData *aodata,
                              DialogData         *data)
{
        gfile_list_free (aodata->list);
        task_terminated (data);
        data->aodata = NULL;
        load_images_preview (data);
}


static void
import_delete_cb (GtkButton  *button,
                  DialogData *data)
{
        GList *sel_list;
        GList *scan;
        GList *delete_list = NULL;

        sel_list = gth_image_list_get_selection (GTH_IMAGE_LIST (data->image_list));
        if (sel_list != NULL) {
                for (scan = sel_list; scan; scan = scan->next) {
                        FileData *fd = scan->data;
                        delete_list = g_list_prepend (delete_list, fd->gfile);
                }
                if (delete_list != NULL)
                        delete_list = g_list_reverse (delete_list);
                file_data_list_free (sel_list);
        }

        data->aodata = async_operation_new (NULL,
                                            delete_list,
                                            delete_imported_images__init,
                                            delete_imported_images__step,
                                            delete_imported_images__done,
                                            data);
        async_operation_start (data->aodata);
}


/* called when the "help" button is pressed. */
static void
help_cb (GtkWidget  *widget,
	 DialogData *data)
{
	gthumb_display_help (GTK_WINDOW (data->dialog), "gthumb-import-photos");
}


void
dlg_photo_importer (GthBrowser *browser,
		    const char *uri,
		    gboolean    dcim_dirs_only)
{
	DialogData *data;
	GtkWidget  *btn_cancel;
	GtkWidget  *btn_help;
	char       *default_path;
	char       *default_uri;


	data = g_new0 (DialogData, 1);
	data->browser = browser;
	
	data->gui = glade_xml_new (GTHUMB_GLADEDIR "/" GLADE_FILE , NULL, NULL);
	if (! data->gui) {
		g_free (data);
		g_warning ("Could not find " GLADE_FILE "\n");
		return;
	}

	data->tags_list = NULL;
	data->delete_list = NULL;
	data->interrupted = FALSE;

	data->data_mutex = g_mutex_new ();
	data->check_id = 0;
	data->idle_id = 0;
	data->msg_text  = NULL;

	data->dcim_dirs = NULL;
	data->dcim_dirs_only = dcim_dirs_only;
	data->uri = g_strdup (uri);

	/* Get the widgets. */

	data->dialog = glade_xml_get_widget (data->gui, "import_photos_dialog");
	data->import_dialog_vbox = glade_xml_get_widget (data->gui, "import_dialog_vbox");
	data->import_preview_scrolledwindow = glade_xml_get_widget (data->gui, "import_preview_scrolledwindow");
	data->destination_filechooserbutton = glade_xml_get_widget (data->gui, "destination_filechooserbutton");
        data->subfolder_combobox = glade_xml_get_widget(data->gui, "group_into_subfolderscombobutton");
	data->format_code_entry = glade_xml_get_widget (data->gui, "format_code_entry");
	data->format_code_label = glade_xml_get_widget (data->gui, "format_code_label");
	data->delete_checkbutton = glade_xml_get_widget (data->gui, "delete_checkbutton");
	data->choose_tags_button = glade_xml_get_widget (data->gui, "choose_tags_button");
	data->tags_entry = glade_xml_get_widget (data->gui, "tags_entry");
	data->import_progressbar = glade_xml_get_widget (data->gui, "import_progressbar");
	data->progress_info_image = glade_xml_get_widget (data->gui, "progress_info_image");
	data->progress_info_label = glade_xml_get_widget (data->gui, "progress_info_label");
	data->progress_info_box = glade_xml_get_widget (data->gui, "progress_info_box");
	data->import_preview_box = glade_xml_get_widget (data->gui, "import_preview_box");
	data->import_reload_button = glade_xml_get_widget (data->gui, "import_reload_button");
	data->import_delete_button = glade_xml_get_widget (data->gui, "import_delete_button");
	data->i_commands_table = glade_xml_get_widget (data->gui, "i_commands_table");
	data->import_ok_button = glade_xml_get_widget (data->gui, "import_okbutton");
	data->reset_exif_tag_on_import_checkbutton = glade_xml_get_widget (data->gui, "reset_exif_tag_on_import_checkbutton");
	btn_cancel = glade_xml_get_widget (data->gui, "import_cancelbutton");
	btn_help = glade_xml_get_widget (data->gui, "import_helpbutton");

	data->image_list = gth_image_list_new (THUMB_SIZE + THUMB_BORDER, GTH_TYPE_FILE_DATA);
	gth_image_list_set_view_mode (GTH_IMAGE_LIST (data->image_list), GTH_VIEW_MODE_LABEL);
	gtk_widget_show (data->image_list);
	gtk_container_add (GTK_CONTAINER (data->import_preview_scrolledwindow), data->image_list);

	gtk_widget_hide (data->import_preview_box);
	gtk_window_set_resizable (GTK_WINDOW (data->dialog), FALSE);

	/* Set widgets data. */

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->delete_checkbutton), eel_gconf_get_boolean (PREF_PHOTO_IMPORT_DELETE, FALSE));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->reset_exif_tag_on_import_checkbutton), eel_gconf_get_boolean (PREF_PHOTO_IMPORT_RESET_EXIF_ORIENTATION, TRUE));

	default_path = eel_gconf_get_path (PREF_PHOTO_IMPORT_DESTINATION, NULL);
	if ((default_path == NULL) || (*default_path == 0))
		default_path = xdg_user_dir_lookup ("PICTURES");
	default_uri = add_scheme_if_absent (default_path);
		
	gtk_file_chooser_set_current_folder_uri (GTK_FILE_CHOOSER (data->destination_filechooserbutton),
					         default_uri);
	g_free (default_path);
	g_free (default_uri);

	task_terminated (data);

	gtk_combo_box_append_text (GTK_COMBO_BOX (data->subfolder_combobox),
                                   _("No grouping"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (data->subfolder_combobox),
                                   _("By day photo taken"));
        gtk_combo_box_append_text (GTK_COMBO_BOX (data->subfolder_combobox),
                                   _("By month photo taken"));
        gtk_combo_box_append_text (GTK_COMBO_BOX (data->subfolder_combobox),
                                   _("By current date and time"));
        gtk_combo_box_append_text (GTK_COMBO_BOX (data->subfolder_combobox),
                                   _("Custom subfolder"));

	gtk_combo_box_set_active (GTK_COMBO_BOX (data->subfolder_combobox), pref_get_import_subfolder());
	subfolder_mode_changed_cb (GTK_COMBO_BOX (data->subfolder_combobox), data);

	/* Set the signals handlers. */

	g_signal_connect (G_OBJECT (data->dialog),
			  "destroy",
			  G_CALLBACK (destroy_cb),
			  data);
	g_signal_connect (G_OBJECT (data->import_ok_button),
			  "clicked",
			  G_CALLBACK (ok_clicked_cb),
			  data);
	g_signal_connect (G_OBJECT (btn_help),
			  "clicked",
			  G_CALLBACK (help_cb),
			  data);
	g_signal_connect (G_OBJECT (btn_cancel),
			  "clicked",
			  G_CALLBACK (cancel_clicked_cb),
			  data);
	g_signal_connect (G_OBJECT (data->choose_tags_button),
			  "clicked",
			  G_CALLBACK (choose_tags_cb),
			  data);

	g_signal_connect (G_OBJECT (data->import_reload_button),
			  "clicked",
			  G_CALLBACK (import_reload_cb),
			  data);
	g_signal_connect (G_OBJECT (data->import_delete_button),
			  "clicked",
			  G_CALLBACK (import_delete_cb),
			  data);

	g_signal_connect (G_OBJECT (data->reset_exif_tag_on_import_checkbutton),
			  "clicked",
			  G_CALLBACK (reset_exif_tag_on_import_cb),
			  data);

	g_signal_connect (G_OBJECT (data->subfolder_combobox),
			  "changed", 
			  G_CALLBACK (subfolder_mode_changed_cb),
			  data);
	/* run dialog. */

	if (browser != NULL)
		gtk_window_set_transient_for (GTK_WINDOW (data->dialog), GTK_WINDOW (browser));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), FALSE);
	gtk_widget_show (data->dialog);

	load_images_preview (data);
}

