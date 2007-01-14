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

#ifdef HAVE_LIBGPHOTO

#include <string.h>
#include <strings.h>
#include <time.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <libgnome/gnome-help.h>
#include <libgnomeui/gnome-icon-lookup.h>
#include <libgnomeui/gnome-icon-theme.h>
#include <libgnomevfs/gnome-vfs-file-info.h>
#include <libgnomevfs/gnome-vfs-mime.h>
#include <libgnomevfs/gnome-vfs-ops.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <gphoto2/gphoto2-context.h>
#include <gphoto2/gphoto2-camera.h>
#include <gphoto2/gphoto2-abilities-list.h>
#include <glade/glade.h>

#include "gth-utils.h"
#include "gtk-utils.h"
#include "gth-window.h"
#include "gth-browser.h"
#include "file-data.h"
#include "file-utils.h"
#include "dlg-categories.h"
#include "comments.h"
#include "gth-image-list.h"
#include "dlg-file-utils.h"
#include "gconf-utils.h"
#include "preferences.h"
#include "pixbuf-utils.h"
#include "rotation-utils.h"
#include "main.h"

#define GLADE_FILE "gthumb_camera.glade"
#define CATEGORIES_GLADE_FILE "gthumb_comment.glade"
#define CAMERA_FILE "gphoto-48.png"
#define MUTE_FILE "volume-mute.png"
#define CATEGORY_SEPARATOR ";"
#define MAX_TRIES 50
#define THUMB_SIZE 100
#define THUMB_BORDER 14
#define REFRESH_RATE 10


typedef enum {
	GTH_IMPORTER_OP_LIST_ABILITIES,
	GTH_IMPORTER_OP_AUTO_DETECT
} GthImporterOp;


typedef struct {
	GthBrowser     *browser;
	GladeXML       *gui;

	GtkWidget      *dialog;
	GtkWidget      *import_dialog_vbox;
	GtkWidget      *import_preview_scrolledwindow;
	GtkWidget      *camera_model_label;
	GtkWidget      *select_model_button;
	GtkWidget      *destination_filechooserbutton;
	GtkWidget      *film_entry;
	GtkWidget      *keep_names_checkbutton;
	GtkWidget      *delete_checkbutton;
	GtkWidget      *choose_categories_button;
	GtkWidget      *categories_entry;
	GtkWidget      *import_progressbar;
	GtkWidget      *progress_camera_image;
	GtkWidget      *import_preview_box;
	GtkWidget      *import_reload_button;
	GtkWidget      *import_delete_button;
	GtkWidget      *import_ok_button;
	GtkWidget      *i_commands_table;
	GtkWidget      *reset_exif_tag_on_import_checkbutton;

	GtkWidget      *progress_info_image;
	GtkWidget      *progress_info_label;
	GtkWidget      *progress_info_box;

	GtkWidget      *image_list;

	GdkPixbuf      *no_camera_pixbuf, *camera_present_pixbuf;

	/**/

	Camera              *camera;
	gboolean             camera_setted, view_folder;
        GPContext           *context;
	CameraAbilitiesList *abilities_list;
	GPPortInfoList      *port_list;

	gboolean             keep_original_filename;
	gboolean             delete_from_camera;
	gboolean             adjust_orientation;

	int                  image_n;
	char                *local_folder;

	GthImporterOp        current_op;
	gboolean             async_operation;
	gboolean             interrupted;
	gboolean             error;
	float                target;
	float                fraction;
	char                *progress_info;
	gboolean             update_ui;
	const char          *msg_icon;
	char                *msg_text;

	GList               *categories_list;
	GList               *delete_list;
	GList               *adjust_orientation_list;
	GList               *saved_images_list;

	/**/

	GThread             *thread;
	guint                check_id;
	GMutex              *yes_or_no;
	gboolean             thread_done;

	guint                idle_id;

} DialogData;


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

	/**/

	g_mutex_lock (data->yes_or_no);
	thread_done = data->thread_done;
	g_mutex_unlock (data->yes_or_no);

	if (! thread_done && (data->thread != NULL))
		g_thread_join (data->thread);

	g_mutex_free (data->yes_or_no);

	/**/

	if (data->view_folder) {
		if (browser == NULL) {
			browser = gth_browser_get_current_browser ();
			if (browser != NULL)
				gth_browser_go_to_directory (GTH_BROWSER (browser), data->local_folder);
			else
				browser = gth_browser_new (data->local_folder);
			gtk_window_present (GTK_WINDOW (browser));
		} else
			gth_browser_go_to_directory (data->browser, data->local_folder);
	}

	/**/

	g_free (data->progress_info);
	g_free (data->msg_text);
	g_free (data->local_folder);

	if (data->no_camera_pixbuf != NULL)
		g_object_unref (data->no_camera_pixbuf);
	if (data->camera_present_pixbuf != NULL)
		g_object_unref (data->camera_present_pixbuf);
	path_list_free (data->categories_list);
	path_list_free (data->delete_list);
	path_list_free (data->adjust_orientation_list);
	path_list_free (data->saved_images_list);
	gp_camera_unref (data->camera);
	gp_context_unref (data->context);
	gp_abilities_list_free (data->abilities_list);
	gp_port_info_list_free (data->port_list);
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


static unsigned int
ctx_progress_start_func (GPContext  *context,
			 float       target,
                         const char *format,
			 va_list     args,
			 gpointer    callback_data)
{
	DialogData *data = callback_data;
	char *locale_string;

	g_mutex_lock (data->yes_or_no);
	data->update_ui = TRUE;
	data->interrupted = FALSE;
	data->target = target;
	data->fraction = 0.0;
	if (data->progress_info != NULL)
		g_free (data->progress_info);
	locale_string = g_strdup_vprintf (format, args);
	data->progress_info = g_locale_to_utf8 (locale_string, -1, NULL, NULL, NULL);
	g_free (locale_string);
	g_mutex_unlock (data->yes_or_no);

	return data->current_op;
}


static void
ctx_progress_update_func (GPContext    *context,
			  unsigned int  id,
                          float         current,
			  gpointer      callback_data)
{
	DialogData *data = callback_data;

	g_mutex_lock (data->yes_or_no);
	data->update_ui = TRUE;
	data->fraction = current / data->target;
	g_mutex_unlock (data->yes_or_no);
}


static gboolean
valid_mime_type (const char *name,
		 const char *type)
{
	int         i;
	const char *name_ext;

	if ((type != NULL) && (strcmp (type, "") != 0)) {
		const char *mime_types[] = { "image",
					     "video",
					     "audio" };
		for (i = 0; i < G_N_ELEMENTS (mime_types); i++) {
			const char *mime_type = mime_types[i];
			if (strncasecmp (type, mime_type, strlen (mime_type)) == 0)
				return TRUE;
		}
	}

	name_ext = get_filename_extension (name);
	if ((name_ext != NULL) && (strcmp (name_ext, "") != 0)) {
		const char *exts[] = { "JPG", "JPEG", "PNG", "TIF", "TIFF", "GIF",
				       "AVI", "MPG", "MPEG",
				       "AU", "WAV", "OGG", "MP3", "FLAC" };
		for (i = 0; i < G_N_ELEMENTS (exts); i++) {
			const char *ext = exts[i];
			if (strncasecmp (ext, name_ext, strlen (name_ext)) == 0)
				return TRUE;
		}
	}

	return FALSE;
}


static GList *
get_file_list (DialogData *data,
	       const char *folder)
{
	GList      *file_list = NULL;
	CameraList *list;
	int         n;

	gp_list_new (&list);
	gp_camera_folder_list_files (data->camera,
				     folder,
				     list,
				     data->context);
	n = gp_list_count (list);
	if (n > 0) {
		int i;
		for (i = 0; i < n; i++) {
			const char     *name;
			CameraFileInfo  info;

			gp_list_get_name (list, i, &name);
			if (gp_camera_file_get_info (data->camera,
						     folder,
						     name,
						     &info,
						     NULL) != GP_OK)
				continue;

			if (valid_mime_type (info.file.name, info.file.type)) {
				char *filename = g_build_filename (folder, name, NULL);
				file_list = g_list_prepend (file_list, filename);
			}
		}
	}
	gp_list_free (list);

	return g_list_reverse (file_list);
}


static GList *
get_folder_list (DialogData *data,
		 const char *folder)
{
	GList      *file_list = NULL;
	CameraList *list;
	int         n;

	gp_list_new (&list);
	gp_camera_folder_list_folders (data->camera,
				       folder,
				       list,
				       data->context);
	n = gp_list_count (list);
	if (n > 0) {
		int i;
		for (i = 0; i < n; i++) {
			const char *name;
			gp_list_get_name (list, i, &name);
			file_list = g_list_prepend (file_list,
						    g_build_filename (folder, name, NULL));
		}
	}
	gp_list_free (list);

	return g_list_reverse (file_list);
}


static GList *
get_all_files (DialogData *data,
	       const char *folder)
{
	GList *file_list, *folder_list, *scan;

	file_list = get_file_list (data, folder);
	folder_list = get_folder_list (data, folder);

	for (scan = folder_list; scan; scan = scan->next) {
		const char *folder = scan->data;
		file_list = g_list_concat (file_list, get_all_files (data, folder));
	}

	path_list_free (folder_list);

	return file_list;
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


static void
update_info (DialogData *data)
{
	gboolean    update_ui;
	gfloat      fraction = -0.1;
	char       *progress_info = NULL;
	char       *msg_text = NULL;
	const char *msg_icon = GTK_STOCK_DIALOG_ERROR;

	g_mutex_lock (data->yes_or_no);
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
	g_mutex_unlock (data->yes_or_no);

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
	gtk_widget_set_sensitive (data->select_model_button, value);
	gtk_widget_set_sensitive (data->import_ok_button, value);
	gtk_widget_set_sensitive (data->import_reload_button, value);
	gtk_widget_set_sensitive (data->import_delete_button, value);
	gtk_widget_set_sensitive (data->i_commands_table, value);
	gtk_widget_set_sensitive (data->reset_exif_tag_on_import_checkbutton, value);
}


 /**/


#define ASYNC_STEP_TIMEOUT 60

typedef struct _AsyncOperationData AsyncOperationData;

typedef void (*AsyncOpFunc) (AsyncOperationData *aodata,
			     DialogData         *data);


struct _AsyncOperationData {
	DialogData  *data;
	GList       *list, *scan;
	int          total, current;
	AsyncOpFunc  init_func, step_func, done_func;
	guint        timer_id;
};


static AsyncOperationData *
async_operation_new (GList       *list,
		     AsyncOpFunc  init_func,
		     AsyncOpFunc  step_func,
		     AsyncOpFunc  done_func,
		     DialogData  *data)
{
	AsyncOperationData *aodata;

	aodata = g_new0 (AsyncOperationData, 1);

	aodata->list = list;
	aodata->init_func = init_func;
	aodata->step_func = step_func;
	aodata->done_func = done_func;
	aodata->data = data;
	aodata->total = g_list_length (aodata->list);
	aodata->current = 1;

	return aodata;
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

	g_mutex_lock (aodata->data->yes_or_no);
	interrupted = aodata->data->interrupted;
	aodata->data->update_ui = TRUE;
	aodata->data->fraction = (float) aodata->current / aodata->total;
	g_mutex_unlock (aodata->data->yes_or_no);

	if ((aodata->scan == NULL) || interrupted) {
		g_mutex_lock (aodata->data->yes_or_no);
		aodata->data->async_operation = FALSE;
		g_mutex_unlock (aodata->data->yes_or_no);

		main_dialog_set_sensitive (aodata->data, TRUE);

		if (aodata->done_func != NULL)
			(*aodata->done_func) (aodata, aodata->data);
		g_free (aodata);

		return FALSE;
	}

	if (aodata->step_func) {
		(*aodata->step_func) (aodata, aodata->data);
		update_info (aodata->data);
	}

	aodata->current++;
	aodata->scan = aodata->scan->next;

	aodata->timer_id = g_timeout_add (ASYNC_STEP_TIMEOUT,
					  async_operation_step,
					  aodata);

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

	g_mutex_lock (aodata->data->yes_or_no);
	aodata->data->async_operation = TRUE;
	aodata->data->interrupted = FALSE;
	g_mutex_unlock (aodata->data->yes_or_no);

	async_operation_step (aodata);
}


 /**/


static void
load_images_preview__init (AsyncOperationData *aodata,
			   DialogData         *data)
{
	task_terminated (data);
}


static int
get_default_icon_size (GtkWidget *widget)
{
        int icon_width, icon_height;

        gtk_icon_size_lookup_for_settings (gtk_widget_get_settings (widget),
                                           GTK_ICON_SIZE_DIALOG,
                                           &icon_width, &icon_height);
        return MAX (icon_width, icon_height);
}


static GdkPixbuf*
get_icon_from_mime_type (DialogData *data,
			 const char *mime_type)
{
	GdkPixbuf      *pixbuf = NULL;
	int             icon_size;
        GnomeIconTheme *icon_theme = gnome_icon_theme_new ();
        char           *icon_name;
        char           *icon_path;

        icon_size = get_default_icon_size (data->dialog);
        icon_name = gnome_icon_lookup (icon_theme,
                                       NULL,
                                       NULL,
                                       NULL,
                                       NULL,
                                       mime_type,
                                       GNOME_ICON_LOOKUP_FLAGS_NONE,
                                       NULL);
	icon_path = gnome_icon_theme_lookup_icon (icon_theme,
                                                  icon_name,
                                                  icon_size,
                                                  NULL,
                                                  NULL);
        g_free (icon_name);

	if (icon_path != NULL) {
		 pixbuf = gdk_pixbuf_new_from_file (icon_path, NULL);
                g_free (icon_path);
	}

	g_object_unref (icon_theme);

	return pixbuf;
}


static const char *
get_mime_type_from_filename (const char*filename)
{
	const char *result;

	char *n1 = g_filename_display_name (filename);
	char *n2 = g_utf8_strdown (n1, -1);
	char *n3 = g_filename_from_utf8 (n2, -1, 0, 0, 0);
	result = gnome_vfs_mime_type_from_name_or_default (n3, NULL);
	g_free (n3);
	g_free (n2);
	g_free (n1);

	return result;
}


static GdkPixbuf*
get_mime_type_icon (DialogData *data,
		    const char *filename)
{
	GdkPixbuf  *pixbuf = NULL;
	const char *mime_type;

	mime_type = get_mime_type_from_filename (filename);
	pixbuf = get_icon_from_mime_type (data, mime_type);
	if (pixbuf == NULL)
		pixbuf = get_icon_from_mime_type (data, "image/*");

	return pixbuf;
}


static void
load_images_preview__step (AsyncOperationData *aodata,
			   DialogData         *data)
{
	const char *camera_path = aodata->scan->data;
	CameraFile *file;
	char       *camera_folder;
	const char *camera_filename;
	char       *tmp_dir;
	char       *tmp_filename;

	tmp_dir = get_temp_dir_name ();
	if (tmp_dir == NULL)
	{
		/* should we display an error message here? */
		return;
	}
	tmp_filename = get_temp_file_name (tmp_dir, NULL);

	gp_file_new (&file);

	camera_folder = remove_level_from_path (camera_path);
	camera_filename = file_name_from_path (camera_path);
	gp_camera_file_get (data->camera,
			    camera_folder,
			    camera_filename,
			    GP_FILE_TYPE_PREVIEW,
			    file,
			    data->context);

	if (gp_file_save (file, tmp_filename) >= 0) {
		GdkPixbuf *pixbuf;
		int        width, height;
		FileData  *fdata;

		pixbuf = gdk_pixbuf_new_from_file (tmp_filename, NULL);
		if (pixbuf == NULL)
			pixbuf = get_mime_type_icon (data, camera_filename);

		width = gdk_pixbuf_get_width (pixbuf);
		height = gdk_pixbuf_get_height (pixbuf);

		if (scale_keepping_ratio (&width,
					  &height,
					  THUMB_SIZE,
					  THUMB_SIZE)) {
			GdkPixbuf *tmp = pixbuf;
			pixbuf = gdk_pixbuf_scale_simple (tmp,
							  width,
							  height,
							  GDK_INTERP_BILINEAR);
			g_object_unref (tmp);
		}

		fdata = file_data_new (camera_path, NULL);
		gth_image_list_append_with_data (GTH_IMAGE_LIST (data->image_list),
						 pixbuf,
						 camera_filename,
						 NULL,
						 fdata);
		g_object_unref (pixbuf);
		file_unlink (tmp_filename);
	}

	g_free (tmp_filename);
	dir_remove (tmp_dir);
	g_free (tmp_dir);
	g_free (camera_folder);

	gp_file_unref (file);
}


static void
load_images_preview__done (AsyncOperationData *aodata,
			   DialogData         *data)
{
	path_list_free (aodata->list);
	task_terminated (data);
}


static void
load_images_preview (DialogData *data)
{
	GList              *file_list;
	gboolean            error;
	AsyncOperationData *aodata;

	gth_image_list_clear (GTH_IMAGE_LIST (data->image_list));

	g_mutex_lock (data->yes_or_no);
	data->error = FALSE;
	g_mutex_unlock (data->yes_or_no);

	file_list = get_all_files (data, "/");

	g_mutex_lock (data->yes_or_no);
	error = data->error;
	g_mutex_unlock (data->yes_or_no);

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

	} else {
		gtk_widget_show (data->import_preview_box);
		gtk_widget_hide (data->progress_info_box);
		gtk_window_set_resizable (GTK_WINDOW (data->dialog), TRUE);
	}

	aodata = async_operation_new (file_list,
				      load_images_preview__init,
				      load_images_preview__step,
				      load_images_preview__done,
				      data);

	async_operation_start (aodata);
}


static void
set_camera_model (DialogData *data,
		  const char *model,
		  const char *port)
{
        int r;

	if ((model == NULL) || (port == NULL)) {
		data->camera_setted = FALSE;
		gtk_widget_hide (data->import_preview_box);
		gtk_label_set_text (GTK_LABEL (data->camera_model_label), _("No camera detected"));
		gtk_image_set_from_pixbuf (GTK_IMAGE (data->progress_camera_image), data->no_camera_pixbuf);
		gtk_window_set_resizable (GTK_WINDOW (data->dialog), FALSE);
		return;
	}

	data->camera_setted = TRUE;

	r = gp_abilities_list_lookup_model (data->abilities_list, model);
	if (r >= 0) {
		CameraAbilities a;
		r = gp_abilities_list_get_abilities (data->abilities_list, r, &a);

		if (r >= 0) {
			gp_camera_set_abilities (data->camera, a);
			r = gp_port_info_list_lookup_path (data->port_list, port);

			if (r >= 0) {
				GPPortInfo port_info;
				gp_port_info_list_get_info (data->port_list, r, &port_info);
				gp_camera_set_port_info (data->camera, port_info);
			}
		}
	}

	if (r >= 0) {
		eel_gconf_set_string (PREF_PHOTO_IMPORT_MODEL, model);
		eel_gconf_set_string (PREF_PHOTO_IMPORT_PORT, port);

		_gtk_label_set_locale_text (GTK_LABEL (data->camera_model_label), model);
		gtk_image_set_from_pixbuf (GTK_IMAGE (data->progress_camera_image), data->camera_present_pixbuf);
		load_images_preview (data);

	} else {
		data->camera_setted = FALSE;
		display_error_dialog (data,
				      _("Could not import photos"),
				      gp_result_as_string (r));
		gtk_label_set_text (GTK_LABEL (data->camera_model_label),
				    _("No camera detected"));
		gtk_image_set_from_pixbuf (GTK_IMAGE (data->progress_camera_image),
					   data->no_camera_pixbuf);
	}
}


static gboolean
autodetect_camera (DialogData *data)
{
	CameraList *list = NULL;
	int         count;
	gboolean    detected = FALSE;
	const char *model = NULL, *port = NULL;

	data->current_op = GTH_IMPORTER_OP_AUTO_DETECT;

	gp_list_new (&list);

	gp_abilities_list_detect (data->abilities_list,
				  data->port_list,
				  list,
				  data->context);
	count = gp_list_count (list);
	if (count >= 1) {
		gp_list_get_name (list, 0, &model);
		gp_list_get_value (list, 0, &port);
		detected = TRUE;

	} else {
		model = NULL;
		port = NULL;
	}

	set_camera_model (data, model, port);

	gp_list_free (list);

	return detected;
}


static void
ctx_progress_stop_func (GPContext    *context,
			unsigned int  id,
			gpointer      callback_data)
{
	DialogData *data = callback_data;

	g_mutex_lock (data->yes_or_no);
	data->interrupted = FALSE;
	g_mutex_unlock (data->yes_or_no);
}


static GPContextFeedback
ctx_cancel_func (GPContext *context,
		 gpointer   callback_data)
{
	DialogData *data = callback_data;
	gboolean    interrupted;

	g_mutex_lock (data->yes_or_no);
	interrupted = data->interrupted;
	g_mutex_unlock (data->yes_or_no);

	if (interrupted)
		return GP_CONTEXT_FEEDBACK_CANCEL;
	else
		return GP_CONTEXT_FEEDBACK_OK;
}


static void
ctx_error_func (GPContext  *context,
		const char *format,
		va_list     args,
		gpointer    callback_data)
{
	DialogData *data = callback_data;
	char *locale_string;

	g_mutex_lock (data->yes_or_no);
	data->update_ui = TRUE;
	data->error = TRUE;
	if (data->msg_text != NULL)
		g_free (data->msg_text);
	locale_string = g_strdup_vprintf (format, args);
	data->msg_text = g_locale_to_utf8 (locale_string, -1, NULL, NULL, NULL);
	g_free (locale_string);
	data->msg_icon = GTK_STOCK_DIALOG_ERROR;
	g_mutex_unlock (data->yes_or_no);
}


static void
ctx_status_func (GPContext  *context,
		 const char *format,
		 va_list     args,
		 gpointer    callback_data)
{
	DialogData *data = callback_data;
	char *locale_string;

	g_mutex_lock (data->yes_or_no);
	data->update_ui = TRUE;
	if (data->msg_text != NULL)
		g_free (data->msg_text);
	locale_string = g_strdup_vprintf (format, args);
	data->msg_text = g_locale_to_utf8 (locale_string, -1, NULL, NULL, NULL);
	g_free (locale_string);
	data->msg_icon = GTK_STOCK_DIALOG_INFO;
	g_mutex_unlock (data->yes_or_no);
}


static void
ctx_message_func (GPContext  *context,
		  const char *format,
		  va_list     args,
		  gpointer    callback_data)
{
	DialogData *data = callback_data;
	char *locale_string;

	g_mutex_lock (data->yes_or_no);
	data->update_ui = TRUE;
	if (data->msg_text != NULL)
		g_free (data->msg_text);
	locale_string = g_strdup_vprintf (format, args);
	data->msg_text = g_locale_to_utf8 (locale_string, -1, NULL, NULL, NULL);
	g_free (locale_string);
	data->msg_icon = GTK_STOCK_DIALOG_WARNING;
	g_mutex_unlock (data->yes_or_no);
}


static gboolean
is_valid_filename (const char *name)
{
	int l = strlen (name);
	int i;

	if (name == NULL)
		return FALSE;
	if (*name == 0)
		return FALSE;

	/**/

	for (i = 0; i < l; i++)
		if (name[i] != ' ')
			break;
	if (i >= l)
		return FALSE;

	/**/

	if (strchr (name, '/') != NULL)
		return FALSE;

	return TRUE;
}


static char*
get_folder_name (DialogData *data)
{
	char *esc_destination;
	char *destination;
	char *film_name;
	char *path;

	esc_destination = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (data->destination_filechooserbutton));
	destination = gnome_vfs_unescape_string (esc_destination, "");
	g_free (esc_destination);

	eel_gconf_set_path (PREF_PHOTO_IMPORT_DESTINATION, destination);

	if (! dlg_check_folder (GTH_WINDOW (data->browser), destination)) {
		g_free (destination);
		return NULL;
	}

	film_name = _gtk_entry_get_filename_text (GTK_ENTRY (data->film_entry));
	if (! is_valid_filename (film_name)) {
		time_t     now;
		struct tm *tm;
		char       time_txt[50 + 1];

		g_free (film_name);

		time (&now);
		tm = localtime (&now);
		strftime (time_txt, 50, "%Y-%m-%d--%H.%M.%S", tm);

		film_name = g_strdup (time_txt);
	} else
		eel_gconf_set_path (PREF_PHOTO_IMPORT_FILM, film_name);

	path = g_build_filename (destination, film_name, NULL);
	g_free (film_name);
	g_free (destination);

	return path;
}


static void
set_lowercase (char *name)
{
	char *s;
	for (s = name; *s != 0; s++)
		*s = g_ascii_tolower (*s);
}


static char *
get_extension_lowercase (const char *path)
{
	const char *filename = file_name_from_path (path);
	const char *ext;
	char       *new_ext;

	ext = strrchr (filename, '.');
	if (ext == NULL)
		return NULL;

	new_ext = g_strdup (ext);
	set_lowercase (new_ext);

	return new_ext;
}


static char*
get_file_name (DialogData *data,
	       const char *camera_path,
	       const char *local_folder,
	       int         n)
{
	char *file_name;
	char *file_path;
	int   m = 1;

	if (data->keep_original_filename) {
		file_name = g_strdup (file_name_from_path (camera_path));
		/* set_lowercase (file_name); see #339291 */

	} else {
		char *s, *new_ext;

		new_ext = get_extension_lowercase (camera_path);
		file_name = g_strdup_printf ("%5d%s", n, new_ext);
		g_free (new_ext);

		for (s = file_name; *s != 0; s++)
			if (*s == ' ')
				*s = '0';
	}

	file_path = g_build_filename (local_folder, file_name, NULL);

	while (path_is_file (file_path)) {
		char *test_name;
		test_name = g_strdup_printf ("%d.%s", m++, file_name);
		g_free (file_path);
		file_path = g_build_filename (local_folder, test_name, NULL);
		g_free (test_name);
	}

	g_free (file_name);

	return file_path;
}


static void
add_categories_to_image (DialogData *data,
			 const char *filename)
{
	CommentData *cdata;
	GList       *scan;

	if (data->categories_list == NULL)
		return;

	cdata = comments_load_comment (filename, FALSE);
	if (cdata == NULL)
		cdata = comment_data_new ();

	for (scan = data->categories_list; scan; scan = scan->next) {
		const char *k = scan->data;
		comment_data_add_keyword (cdata, k);
	}

	comments_save_categories (filename, cdata);
	comment_data_free (cdata);
}


static void
save_image (DialogData *data,
	    const char *camera_path,
	    const char *local_folder,
	    int         n)
{
	CameraFile *file;
	char       *camera_folder;
	const char *camera_filename, *local_path;
	char       *file_uri;

	gp_file_new (&file);

	camera_folder = remove_level_from_path (camera_path);
	camera_filename = file_name_from_path (camera_path);
	gp_camera_file_get (data->camera,
			    camera_folder,
			    camera_filename,
			    GP_FILE_TYPE_NORMAL,
			    file,
			    data->context);

	file_uri = get_file_name (data, camera_path, local_folder, n);

	/* FIXME: support non-local saving */
	local_path = get_file_path_from_uri (file_uri);
	if ((local_path != NULL) && gp_file_save (file, local_path) >= 0) {
		if (data->delete_from_camera)
			data->delete_list = g_list_prepend (data->delete_list, g_strdup (camera_path));
		if (data->adjust_orientation) {
			data->adjust_orientation_list = g_list_prepend (data->adjust_orientation_list, g_strdup (local_path));
		}
		data->saved_images_list = g_list_prepend (data->saved_images_list, g_strdup (camera_path));
		add_categories_to_image (data, local_path);
	} else {
		g_mutex_lock (data->yes_or_no);
		data->error = TRUE;
		data->interrupted = TRUE;
		g_mutex_unlock (data->yes_or_no);
	}

	g_free (camera_folder);
	g_free (file_uri);
	gp_file_unref (file);
}


static void
add_film_keyword (const char *folder)
{
	CommentData *cdata;

	cdata = comments_load_comment (folder, FALSE);
	if (cdata == NULL)
		cdata = comment_data_new ();
	comment_data_add_keyword (cdata, _("Film"));
	comments_save_categories (folder, cdata);
	comment_data_free (cdata);
}


static void
delete_images__step (AsyncOperationData *aodata,
		     DialogData         *data)
{
	const char *camera_path = aodata->scan->data;
	char       *camera_folder;
	const char *camera_filename;

	camera_folder = remove_level_from_path (camera_path);
	camera_filename = file_name_from_path (camera_path);

	gp_camera_file_delete (data->camera,
			       camera_folder,
			       camera_filename,
			       data->context);

	g_free (camera_folder);
}


static void
delete_images__done (AsyncOperationData *aodata,
		     DialogData         *data)
{
	gboolean interrupted;

	task_terminated (data);

	g_mutex_lock (data->yes_or_no);
	interrupted = data->interrupted;
	g_mutex_unlock (data->yes_or_no);

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


static gboolean
notify_cb (gpointer cb_data)
{
	DialogData *data = cb_data;

	g_source_remove (data->idle_id);
	data->idle_id = 0;

	if (data->saved_images_list != NULL) {
		all_windows_notify_files_created (data->saved_images_list);
		path_list_free (data->saved_images_list);
		data->saved_images_list = NULL;
	}

	all_windows_add_monitor ();

	return FALSE;
}


static void
adjust_orientation__done (AsyncOperationData *aodata,
			  DialogData         *data)
{
	gboolean            interrupted;
	AsyncOperationData *new_aodata;

	g_mutex_lock (data->yes_or_no);
	interrupted = data->interrupted;
	g_mutex_unlock (data->yes_or_no);

	data->idle_id = g_idle_add (notify_cb, data);

	if (interrupted)
		return;

	new_aodata = async_operation_new (data->delete_list,
					  NULL,
					  delete_images__step,
					  delete_images__done,
					  data);
	async_operation_start (new_aodata);
}


static void
adjust_orientation__step (AsyncOperationData *aodata,
			  DialogData         *data)
{
	const char       *filepath = aodata->scan->data;
	GnomeVFSFileInfo  info;
	GtkWindow        *window = GTK_WINDOW (data->dialog);

	gnome_vfs_get_file_info (filepath, &info, GNOME_VFS_FILE_INFO_GET_ACCESS_RIGHTS|GNOME_VFS_FILE_INFO_FOLLOW_LINKS);

	if (file_is_image (filepath, TRUE)) {
		FileData     *fd = file_data_new (filepath, &info);
		GthTransform transform = read_orientation_field (fd->path);
		if (image_is_jpeg (filepath))
			apply_transformation_jpeg (window, fd->path, transform, FALSE);
		else
			apply_transformation_generic (window, fd->path, transform);

		file_data_unref (fd);
	}

	gnome_vfs_set_file_info (filepath, &info, GNOME_VFS_SET_FILE_INFO_PERMISSIONS|GNOME_VFS_SET_FILE_INFO_OWNER);
}


static void
save_images__init (AsyncOperationData *aodata,
		   DialogData         *data)
{
	all_windows_remove_monitor ();

	data->image_n = 1;
	if (data->delete_list != NULL) {
		path_list_free (data->delete_list);
		data->delete_list = NULL;
	}
	if (data->adjust_orientation_list != NULL) {
		path_list_free (data->adjust_orientation_list);
		data->adjust_orientation_list = NULL;
	}
	if (data->saved_images_list != NULL) {
		path_list_free (data->saved_images_list);
		data->saved_images_list = NULL;
	}
}


static void
save_images__step (AsyncOperationData *aodata,
		   DialogData         *data)
{
	const char *camera_path = aodata->scan->data;
	save_image (data, camera_path, data->local_folder, data->image_n++);
}


static void
save_images__done (AsyncOperationData *aodata,
		   DialogData         *data)
{
	gboolean            interrupted;
	gboolean            error;
	AsyncOperationData *new_aodata;

	g_mutex_lock (data->yes_or_no);
	interrupted = data->interrupted;
	error = data->error;
	g_mutex_unlock (data->yes_or_no);

	if (interrupted || error)
		return;

	new_aodata = async_operation_new (data->adjust_orientation_list,
					  NULL,
					  adjust_orientation__step,
					  adjust_orientation__done,
					  data);
	async_operation_start (new_aodata);
}


static void
cancel_clicked_cb (GtkButton  *button,
		   DialogData *data)
{
	gboolean async_operation;

	g_mutex_lock (data->yes_or_no);
	async_operation = data->async_operation;
	g_mutex_unlock (data->yes_or_no);

	if (async_operation) {
		g_mutex_lock (data->yes_or_no);
		data->interrupted = TRUE;
		g_mutex_unlock (data->yes_or_no);
	} else
		gtk_widget_destroy (data->dialog);
}


static void
ok_clicked_cb (GtkButton  *button,
	       DialogData *data)
{
	GList              *file_list = NULL, *scan;
	GList              *sel_list;
	gboolean            error;
	AsyncOperationData *aodata;
	GnomeVFSFileSize    total_size = 0;

	if (!data->camera_setted) {
		display_error_dialog (data,
				      _("Could not import photos"),
				      _("No camera detected"));
		return;
	}

	/**/

	g_free (data->local_folder);

	data->local_folder = get_folder_name (data);
	if (data->local_folder == NULL)
		return;

	data->keep_original_filename = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->keep_names_checkbutton));
	data->delete_from_camera = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->delete_checkbutton));
	data->adjust_orientation = eel_gconf_get_boolean (PREF_PHOTO_IMPORT_RESET_EXIF_ORIENTATION, TRUE);

	eel_gconf_set_boolean (PREF_PHOTO_IMPORT_KEEP_FILENAMES, data->keep_original_filename);
	eel_gconf_set_boolean (PREF_PHOTO_IMPORT_DELETE, data->delete_from_camera);

	/**/

	g_mutex_lock (data->yes_or_no);
	error = data->error;
	g_mutex_unlock (data->yes_or_no);

	if (error) {
		update_info (data);
		return;
	}

	sel_list = gth_image_list_get_selection (GTH_IMAGE_LIST (data->image_list));
	if (sel_list == NULL) {
		sel_list = gth_image_list_get_list (GTH_IMAGE_LIST (data->image_list));
		g_list_foreach (sel_list, (GFunc) file_data_ref, NULL);
	}

	if (sel_list != NULL) {
		for (scan = sel_list; scan; scan = scan->next) {
			FileData   *fdata = scan->data;
			const char *filename = file_data_local_path (fdata);
			file_list = g_list_prepend (file_list, g_strdup (filename));
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

	if (! ensure_dir_exists (data->local_folder, 0755)) {
		char *utf8_path;
		char *msg;
		utf8_path = g_filename_display_name (data->local_folder);
		msg = g_strdup_printf (_("Could not create the folder \"%s\": %s"),
				       utf8_path,
				       errno_to_string ());
		display_error_dialog (data, _("Could not import photos"), msg);

		g_free (utf8_path);
		g_free (msg);
		g_free (data->local_folder);
		data->local_folder = NULL;
		path_list_free (file_list);
		return;
	}

	if (! check_permissions (data->local_folder, R_OK | W_OK | X_OK)) {
		char *utf8_path;
		char *msg;
		utf8_path = g_filename_display_name (data->local_folder);
		msg = g_strdup_printf (_("You don't have the right permissions to create images in the folder \"%s\""), utf8_path);

		display_error_dialog (data, _("Could not import photos"), msg);

		g_free (utf8_path);
		g_free (msg);
		g_free (data->local_folder);
		data->local_folder = NULL;
		path_list_free (file_list);
		return;
	}

	for (scan = file_list; scan; scan = scan->next) {
		const char     *camera_path = scan->data;
		CameraFileInfo  info;
		char           *camera_folder;
		const char     *camera_filename;

		camera_folder = remove_level_from_path (camera_path);
		camera_filename = file_name_from_path (camera_path);
		if (gp_camera_file_get_info (data->camera,
					     camera_folder,
					     camera_filename,
					     &info,
					     NULL) == GP_OK)
			total_size += (GnomeVFSFileSize) info.file.size;
		g_free (camera_folder);
	}

	if (get_dest_free_space (data->local_folder) < total_size) {
		display_error_dialog (data,
				      _("Could not import photos"),
				      "Not enough free space left on disk"); /* FIXME: mark for translation. */

		g_free (data->local_folder);
		data->local_folder = NULL;
		path_list_free (file_list);
		return;
	}

	add_film_keyword (data->local_folder);

	aodata = async_operation_new (file_list,
				      save_images__init,
				      save_images__step,
				      save_images__done,
				      data);
	async_operation_start (aodata);
}


static void
choose_categories__done (gpointer callback_data)
{
	DialogData *data = callback_data;
	GString    *categories;
	GList      *scan;

	categories = g_string_new ("");

	for (scan = data->categories_list; scan; scan = scan->next) {
		char *name = scan->data;
		if (categories->len > 0)
			categories = g_string_append (categories, CATEGORY_SEPARATOR " ");
		categories = g_string_append (categories, name);
	}

	gtk_entry_set_text (GTK_ENTRY (data->categories_entry), categories->str);
	g_string_free (categories, TRUE);
}


static void
choose_categories_cb (GtkButton  *button,
		      DialogData *data)
{
	dlg_choose_categories (GTK_WINDOW (data->dialog),
			       NULL,
			       data->categories_list,
			       &(data->categories_list),
			       NULL,
			       choose_categories__done,
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
	if (! data->camera_setted)
		autodetect_camera (data);
	else
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
			FileData   *fdata = scan->data;
			const char *filename = file_data_local_path (fdata);
			delete_list = g_list_prepend (delete_list, g_strdup (filename));
		}
		if (delete_list != NULL)
			delete_list = g_list_reverse (delete_list);
		file_data_list_free (sel_list);
	}

	for (scan = delete_list; scan; scan = scan->next) {
		const char *camera_path = scan->data;
		char       *camera_folder;
		const char *camera_filename;

		camera_folder = remove_level_from_path (camera_path);
		camera_filename = file_name_from_path (camera_path);

		gp_camera_file_delete (data->camera, camera_folder, camera_filename, data->context);
		/* FIXME */
	}

	path_list_free (delete_list);

	task_terminated (data);

	load_images_preview (data);
}


static void dlg_select_camera_model_cb (GtkButton  *button, DialogData *data);


static void *
load_abilities_thread (void *thread_data)
{
	DialogData     *data = thread_data;
	GthImporterOp   current_op;

	g_mutex_lock (data->yes_or_no);
	current_op = data->current_op;
	g_mutex_unlock (data->yes_or_no);

	switch (current_op) {
	case GTH_IMPORTER_OP_LIST_ABILITIES:
		gp_abilities_list_load (data->abilities_list, data->context);
		gp_port_info_list_load (data->port_list);
		break;
	default:
		break;
	}

	g_mutex_lock (data->yes_or_no);
	data->thread_done = TRUE;
	g_mutex_unlock (data->yes_or_no);

	g_thread_exit (NULL);

	return NULL;
}


static int
check_thread (gpointer cb_data)
{
	DialogData *data = cb_data;
	gboolean    thread_done;

	g_source_remove (data->check_id);
	data->check_id = 0;

	update_info (data);

	g_mutex_lock (data->yes_or_no);
	thread_done = data->thread_done;
	g_mutex_unlock (data->yes_or_no);

	if (thread_done) {
		data->thread = NULL;
		task_terminated (data);

		switch (data->current_op) {
		case GTH_IMPORTER_OP_LIST_ABILITIES:
			if (! autodetect_camera (data)) {
				char *camera_model;
				char *camera_port;

				camera_model = eel_gconf_get_string (PREF_PHOTO_IMPORT_MODEL, NULL);
				camera_port = eel_gconf_get_string (PREF_PHOTO_IMPORT_PORT, NULL);

				if ((camera_model != NULL) && (camera_port != NULL))
					set_camera_model (data, camera_model, camera_port);

				g_free (camera_model);
				g_free (camera_port);
			}
			break;
		default:
			break;
		}

	} else	/* Add check again. */
		data->check_id = g_timeout_add (REFRESH_RATE, check_thread, data);

	return FALSE;
}


static void
start_operation (DialogData    *data,
		 GthImporterOp  operation)
{
	if (data->check_id != 0)
		g_source_remove (data->check_id);

	g_mutex_lock (data->yes_or_no);
	data->thread_done = FALSE;
	g_mutex_unlock (data->yes_or_no);

	data->current_op = operation;
	data->thread = g_thread_create (load_abilities_thread, data, TRUE, NULL);
	data->check_id = g_timeout_add (REFRESH_RATE, check_thread, data);
}


/* called when the "help" button is pressed. */
static void
help_cb (GtkWidget  *widget,
	 DialogData *data)
{
	gthumb_display_help (GTK_WINDOW (data->dialog), "gthumb-import-photos");
}


void
dlg_photo_importer (GthBrowser *browser)
{
	DialogData   *data;
	GtkWidget    *btn_cancel;
	GtkWidget    *btn_help;
	GdkPixbuf    *mute_pixbuf;
	char         *default_path;
	char         *esc_uri;
	char         *default_film_name;

	data = g_new0 (DialogData, 1);
	data->browser = browser;

	data->gui = glade_xml_new (GTHUMB_GLADEDIR "/" GLADE_FILE , NULL, NULL);
        if (!data->gui) {
		g_free (data);
                g_warning ("Could not find " GLADE_FILE "\n");
                return;
        }

	gp_camera_new (&data->camera);
	data->context = gp_context_new ();
	gp_context_set_cancel_func (data->context, ctx_cancel_func, data);
        gp_context_set_error_func (data->context, ctx_error_func, data);
        gp_context_set_status_func (data->context, ctx_status_func, data);
        gp_context_set_message_func (data->context, ctx_message_func, data);
	gp_context_set_progress_funcs (data->context,
				       ctx_progress_start_func,
				       ctx_progress_update_func,
				       ctx_progress_stop_func,
				       data);
        gp_abilities_list_new (&data->abilities_list);
	gp_port_info_list_new (&data->port_list);

	data->categories_list = NULL;
	data->delete_list = NULL;
	data->interrupted = FALSE;
	data->camera_setted = FALSE;

	data->yes_or_no = g_mutex_new ();
	data->check_id = 0;
	data->idle_id = 0;
	data->msg_text  = NULL;

	/* Get the widgets. */

	data->dialog = glade_xml_get_widget (data->gui, "import_photos_dialog");
	data->import_dialog_vbox = glade_xml_get_widget (data->gui, "import_dialog_vbox");
	data->import_preview_scrolledwindow = glade_xml_get_widget (data->gui, "import_preview_scrolledwindow");
	data->camera_model_label = glade_xml_get_widget (data->gui, "camera_model_label");
	data->select_model_button = glade_xml_get_widget (data->gui, "select_model_button");
	data->destination_filechooserbutton = glade_xml_get_widget (data->gui, "destination_filechooserbutton");
	data->film_entry = glade_xml_get_widget (data->gui, "film_entry");
	data->keep_names_checkbutton = glade_xml_get_widget (data->gui, "keep_names_checkbutton");
	data->delete_checkbutton = glade_xml_get_widget (data->gui, "delete_checkbutton");
	data->choose_categories_button = glade_xml_get_widget (data->gui, "choose_categories_button");
	data->categories_entry = glade_xml_get_widget (data->gui, "categories_entry");
	data->import_progressbar = glade_xml_get_widget (data->gui, "import_progressbar");
	data->progress_info_image = glade_xml_get_widget (data->gui, "progress_info_image");
	data->progress_info_label = glade_xml_get_widget (data->gui, "progress_info_label");
	data->progress_info_box = glade_xml_get_widget (data->gui, "progress_info_box");
	data->progress_camera_image = glade_xml_get_widget (data->gui, "progress_camera_image");
	data->import_preview_box = glade_xml_get_widget (data->gui, "import_preview_box");
	data->import_reload_button = glade_xml_get_widget (data->gui, "import_reload_button");
	data->import_delete_button = glade_xml_get_widget (data->gui, "import_delete_button");
	data->i_commands_table = glade_xml_get_widget (data->gui, "i_commands_table");
	data->import_ok_button = glade_xml_get_widget (data->gui, "import_okbutton");
        data->reset_exif_tag_on_import_checkbutton = glade_xml_get_widget (data->gui, "reset_exif_tag_on_import_checkbutton");
	btn_cancel = glade_xml_get_widget (data->gui, "import_cancelbutton");
	btn_help = glade_xml_get_widget (data->gui, "import_helpbutton");

	data->image_list = gth_image_list_new (THUMB_SIZE + THUMB_BORDER);
	gth_image_list_set_view_mode (GTH_IMAGE_LIST (data->image_list), GTH_VIEW_MODE_LABEL);
	gtk_widget_show (data->image_list);
	gtk_container_add (GTK_CONTAINER (data->import_preview_scrolledwindow), data->image_list);

	gtk_widget_hide (data->import_preview_box);
	gtk_window_set_resizable (GTK_WINDOW (data->dialog), FALSE);

	/* Set widgets data. */

	data->camera_present_pixbuf = gdk_pixbuf_new_from_file (GTHUMB_GLADEDIR "/" CAMERA_FILE, NULL);
	mute_pixbuf = gdk_pixbuf_new_from_file (GTHUMB_GLADEDIR "/" MUTE_FILE, NULL);

	data->no_camera_pixbuf = gdk_pixbuf_copy (data->camera_present_pixbuf);
	gdk_pixbuf_composite (mute_pixbuf,
			      data->no_camera_pixbuf,
			      0,
			      0,
			      gdk_pixbuf_get_width (mute_pixbuf),
			      gdk_pixbuf_get_height (mute_pixbuf),
			      0,
			      0,
			      1.0,
			      1.0,
			      GDK_INTERP_BILINEAR,
			      200);
	g_object_unref (mute_pixbuf);

	gtk_image_set_from_pixbuf (GTK_IMAGE (data->progress_camera_image), data->no_camera_pixbuf);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->keep_names_checkbutton), eel_gconf_get_boolean (PREF_PHOTO_IMPORT_KEEP_FILENAMES, FALSE));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->delete_checkbutton), eel_gconf_get_boolean (PREF_PHOTO_IMPORT_DELETE, FALSE));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->reset_exif_tag_on_import_checkbutton), eel_gconf_get_boolean (PREF_PHOTO_IMPORT_RESET_EXIF_ORIENTATION, TRUE));

	default_path = eel_gconf_get_path (PREF_PHOTO_IMPORT_DESTINATION, NULL);
	if ((default_path == NULL) || (*default_path == 0))
		default_path = g_strdup (g_get_home_dir());
	esc_uri = gnome_vfs_escape_host_and_path_string (default_path);
	gtk_file_chooser_set_uri (GTK_FILE_CHOOSER (data->destination_filechooserbutton), esc_uri);
	g_free (esc_uri);
	g_free (default_path);

	default_film_name = eel_gconf_get_path (PREF_PHOTO_IMPORT_FILM, "");
	_gtk_entry_set_filename_text (GTK_ENTRY (data->film_entry), default_film_name);
	g_free (default_film_name);

	task_terminated (data);

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
	g_signal_connect (G_OBJECT (data->select_model_button),
			  "clicked",
			  G_CALLBACK (dlg_select_camera_model_cb),
			  data);
	g_signal_connect (G_OBJECT (data->choose_categories_button),
			  "clicked",
			  G_CALLBACK (choose_categories_cb),
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

	/* run dialog. */

	if (browser != NULL)
		gtk_window_set_transient_for (GTK_WINDOW (data->dialog), GTK_WINDOW (browser));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), FALSE);
	gtk_widget_show (data->dialog);

	start_operation (data, GTH_IMPORTER_OP_LIST_ABILITIES);
}


typedef struct {
	DialogData     *data;
	GladeXML       *gui;

	GtkWidget      *dialog;
	GtkWidget      *cm_model_treeview;
	GtkWidget      *cm_port_combo_box;
	GtkWidget      *cm_refresh_button;
	GtkWidget      *cm_manual_selection_checkbutton;

	GHashTable     *autodetected_models;
} ModelDialogData;


static void
model__destroy_cb (GtkWidget       *widget,
		   ModelDialogData *data)
{
	g_object_unref (data->gui);

	if (data->autodetected_models) {
		g_hash_table_destroy (data->autodetected_models);
		data->autodetected_models = NULL;
	}

	g_free (data);
}


static void
model__ok_clicked_cb (GtkButton       *button,
		      ModelDialogData *mdata)
{
	gchar        *model = NULL;
	gchar        *port = NULL;
	GtkTreeModel *treemodel;
	GtkTreeIter  iter;

	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (mdata->cm_model_treeview));
	if (gtk_tree_selection_get_selected (selection, &treemodel, &iter))
		gtk_tree_model_get (treemodel, &iter, 0, &model, -1);

	if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (mdata->cm_port_combo_box), &iter)) {
		treemodel = gtk_combo_box_get_model (GTK_COMBO_BOX (mdata->cm_port_combo_box));
		gtk_tree_model_get (treemodel, &iter, 0, &port, -1);
	}

	gtk_widget_hide (mdata->dialog);
	if ((model != NULL) && (*model != 0))
		set_camera_model (mdata->data, model, port);

	gtk_widget_destroy (mdata->dialog);
	g_free(model);
	g_free(port);
}


static GList *
get_camera_port_list (ModelDialogData *mdata, GPPortType types)
{
	GList *list = NULL;
	int n, i;

	n = gp_port_info_list_count (mdata->data->port_list);
	if (n <= 0)
		return g_list_append (NULL, g_strdup (""));

	for (i = 0; i < n; i++) {
		GPPortInfo info;
		gp_port_info_list_get_info (mdata->data->port_list, i, &info);
                if (info.type & types)
                        list = g_list_prepend (list, g_strdup_printf ("%s", info.path));
	}

	return g_list_reverse (list);
}


static GList *
get_camera_model_list (ModelDialogData *mdata)
{
	GList *list = NULL;
	int    n, i;

	n = gp_abilities_list_count (mdata->data->abilities_list);
	if (n <= 0)
		return list;

	for (i = 0; i < n; i++) {
		CameraAbilities a;
		int r =	gp_abilities_list_get_abilities (mdata->data->abilities_list, i, &a);
		if (r >= 0)
			list = g_list_prepend (list, g_strdup (a.model));
	}

	return g_list_reverse (list);
}


static GList*
get_autodetect_model_list (ModelDialogData *mdata)
{
	DialogData   *data = mdata->data;
	CameraList   *camera_list = NULL;
	GList	     *model_list = NULL;
	GHashTable   *model_hash = NULL;
	int	     n, i;

	gp_list_new (&camera_list);

	/*
	 * Hack to force update of usb ports (which are added dynamically when a device is plugged in)
	 * (libgphoto2 should have a gp_port_info_list_update method)
	 */

	gp_port_info_list_free (data->port_list);
	gp_port_info_list_new (&data->port_list);
	gp_port_info_list_load (data->port_list);

	/* */

	gp_abilities_list_detect (data->abilities_list,
				  data->port_list,
				  camera_list,
				  data->context);

	n = gp_list_count (camera_list);

	model_hash = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, (GDestroyNotify)path_list_free);

	for (i = 0; i < n; i++) {
		const char *model = NULL;
		const char *port = NULL;
		GList* port_list;

		gp_list_get_name (camera_list, i, &model);
		gp_list_get_value (camera_list, i, &port);

		port_list = g_hash_table_lookup (model_hash, model);
		if (port_list == NULL) {
			model_list = g_list_append (model_list, g_strdup_printf ("%s", model));
			port_list = g_list_append (port_list, g_strdup_printf("%s", port));
			g_hash_table_insert(model_hash, g_strdup_printf("%s", model), port_list);
		} else {
			port_list = g_list_append (port_list, g_strdup_printf("%s", port));
		}
	}

	if (mdata->autodetected_models != NULL) {
		g_hash_table_destroy (mdata->autodetected_models);
	}

	gp_list_free (camera_list);
	mdata->autodetected_models = model_hash;
	return model_list;
}


static void
model__selection_changed_cb (GtkTreeSelection *selection,
			     ModelDialogData  *mdata)
{
        GtkTreeIter     iter;
	GtkTreeModel    *treemodel;
	gchar           *model;
	GtkListStore    *store;
	int             m;
	CameraAbilities a;
	GList           *list, *l;
	gboolean        is_manual_selection;

        if (gtk_tree_selection_get_selected (selection, &treemodel, &iter)) {
                gtk_tree_model_get (treemodel, &iter, 0, &model, -1);

		store =  gtk_list_store_new (1, G_TYPE_STRING);
		is_manual_selection = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(mdata->cm_manual_selection_checkbutton));
		if (is_manual_selection) {
			m = gp_abilities_list_lookup_model (mdata->data->abilities_list, model);
			if (m >= 0) {
				gp_abilities_list_get_abilities (mdata->data->abilities_list, m, &a);
				list = get_camera_port_list (mdata, a.port);
			} else
				list = NULL;
		} else
			list = g_hash_table_lookup (mdata->autodetected_models, model);

		for(l = g_list_first (list); l; l = l->next) {
			gchar *text = (gchar *)l->data;
			gtk_list_store_append (store, &iter);
			gtk_list_store_set (store, &iter, 0, text, -1);
		}

		if (is_manual_selection)
			path_list_free (list);

		gtk_combo_box_set_model (GTK_COMBO_BOX (mdata->cm_port_combo_box), GTK_TREE_MODEL (store));
 		gtk_combo_box_set_active (GTK_COMBO_BOX (mdata->cm_port_combo_box), 0);

		g_object_unref (store);
                g_free (model);
        }
}


static void
populate_model_treeview (ModelDialogData *mdata,
			 gboolean         autodetect)
{
	GList             *list, *l;
	GtkTreeIter        iter;
	GtkListStore      *store =  gtk_list_store_new (1, G_TYPE_STRING);
	GtkTreeViewColumn *column;
	GList             *renderers;
	GtkCellRenderer   *text_renderer;

	if (autodetect)
		list = get_autodetect_model_list (mdata);
	else
		list = get_camera_model_list (mdata);

	if (list == NULL) {
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter, 0, _("No camera detected"), -1);
	}

	for (l = g_list_first (list); l; l = l->next) {
		char *text = (char *)l->data;
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter, 0, text, -1);
	}

	column = gtk_tree_view_get_column (GTK_TREE_VIEW (mdata->cm_model_treeview), 0);
	renderers = gtk_tree_view_column_get_cell_renderers (column);
	text_renderer = renderers->data;
	g_object_set (text_renderer,
		      "style", (list != NULL) ? PANGO_STYLE_NORMAL : PANGO_STYLE_ITALIC,
		      "style-set", TRUE,
		      NULL);

	gtk_tree_view_set_model (GTK_TREE_VIEW (mdata->cm_model_treeview), GTK_TREE_MODEL (store));

	path_list_free (list);
	g_object_unref (store);
}


static void
model__refresh_cb (GtkButton       *button,
		   ModelDialogData *mdata)
{
	populate_model_treeview (mdata, TRUE);
}


static void
model__manual_select_cb (GtkButton       *button,
			 ModelDialogData *mdata)
{
	gboolean manual_selection;

	manual_selection = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));
	gtk_widget_set_sensitive (mdata->cm_refresh_button, ! manual_selection);
	populate_model_treeview (mdata, ! manual_selection);
}


static void
dlg_select_camera_model_cb (GtkButton  *button,
			    DialogData *data)
{
	ModelDialogData  *mdata;
	GtkWidget        *btn_ok, *btn_help, *btn_cancel;
	GtkCellRenderer  *renderer;
	GtkTreeSelection *treeview_selection;

	mdata = g_new (ModelDialogData, 1);
	mdata->data = data;

	mdata->gui = glade_xml_new (GTHUMB_GLADEDIR "/" GLADE_FILE , NULL, NULL);
        if (!mdata->gui) {
		g_free (mdata);
                g_warning ("Could not find " GLADE_FILE "\n");
                return;
        }

	/* Get the widgets. */

	mdata->dialog = glade_xml_get_widget (mdata->gui, "camera_model_dialog");
	mdata->cm_model_treeview = glade_xml_get_widget (mdata->gui, "cm_model_treeview");
	mdata->cm_port_combo_box = glade_xml_get_widget (mdata->gui, "cm_port_combobox");

	mdata->cm_refresh_button = glade_xml_get_widget (mdata->gui, "cm_refresh_button");
	mdata->cm_manual_selection_checkbutton = glade_xml_get_widget (mdata->gui, "cm_manual_selection_checkbutton");

	btn_ok = glade_xml_get_widget (mdata->gui, "cm_okbutton");
	btn_help = glade_xml_get_widget (mdata->gui, "cm_helpbutton");
	btn_cancel = glade_xml_get_widget (mdata->gui, "cm_cancelbutton");

	mdata->autodetected_models = NULL;

	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (mdata->cm_model_treeview),
						     0, NULL, renderer,
						     "text", 0,
						     NULL);
	populate_model_treeview (mdata, TRUE);
	treeview_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (mdata->cm_model_treeview));

	/* Set the signals handlers. */

	g_signal_connect (G_OBJECT (mdata->dialog),
			  "destroy",
			  G_CALLBACK (model__destroy_cb),
			  mdata);
	g_signal_connect (G_OBJECT (btn_ok),
			  "clicked",
			  G_CALLBACK (model__ok_clicked_cb),
			  mdata);
        g_signal_connect (G_OBJECT (btn_help),
                          "clicked",
                          G_CALLBACK (help_cb),
                          mdata);
	g_signal_connect_swapped (G_OBJECT (btn_cancel),
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  G_OBJECT (mdata->dialog));
	g_signal_connect (G_OBJECT (mdata->cm_refresh_button),
			  "clicked",
			  G_CALLBACK (model__refresh_cb),
			  mdata);
	g_signal_connect (G_OBJECT (mdata->cm_manual_selection_checkbutton),
			  "toggled",
			  G_CALLBACK (model__manual_select_cb),
			  mdata);
	g_signal_connect (G_OBJECT (treeview_selection),
			  "changed",
			  G_CALLBACK (model__selection_changed_cb),
			  mdata);

	/* run dialog. */

	gtk_window_set_transient_for (GTK_WINDOW (mdata->dialog), GTK_WINDOW (data->dialog));
	gtk_widget_show (mdata->dialog);
}


#endif /*HAVE_LIBGPHOTO */

