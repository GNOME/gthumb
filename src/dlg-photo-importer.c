/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2003 Free Software Foundation, Inc.
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
#include <time.h>

#include <gtk/gtk.h>
#include <libgnome/libgnome.h>
#include <libgnomeui/gnome-dialog.h>
#include <libgnomeui/gnome-dialog-util.h>
#include <libgnomeui/gnome-file-entry.h>
#include <gphoto2/gphoto2-context.h>
#include <gphoto2/gphoto2-camera.h>
#include <gphoto2/gphoto2-abilities-list.h>
#include <glade/glade.h>

#include "gtk-utils.h"
#include "gthumb-window.h"
#include "file-data.h"
#include "file-utils.h"
#include "dlg-categories.h"
#include "comments.h"
#include "gth-image-list.h"
#include "dlg-file-utils.h"
#include "gconf-utils.h"
#include "preferences.h"
#include "pixbuf-utils.h"


#define GLADE_FILE "gthumb_camera.glade"
#define CATEGORIES_GLADE_FILE "gthumb_comment.glade"
#define CAMERA_FILE "gphoto-48.png"
#define MUTE_FILE "volume-mute.png"
#define CATEGORY_SEPARATOR ";"
#define MAX_TRIES 50
#define THUMB_SIZE 100
#define THUMB_BORDER 14


typedef enum {
	GTH_IMPORTER_OPS_LIST_ABILITIES,
	GTH_IMPORTER_OPS_AUTO_DETECT
} GthImporterOps;


typedef struct {
	GThumbWindow   *window;
	GladeXML       *gui;

	GtkWidget      *dialog;
	GtkWidget      *import_dialog_vbox;
	GtkWidget      *import_preview_scrolledwindow;
	GtkWidget      *camera_model_label; 
	GtkWidget      *select_model_button;
	GtkWidget      *destination_fileentry;
	GtkWidget      *destination_entry;
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

	GtkWidget      *progress_info_image;
	GtkWidget      *progress_info_label;
	GtkWidget      *progress_info_box;

	GtkWidget      *image_list;

	GdkPixbuf      *no_camera_pixbuf, *camera_present_pixbuf;

	/**/

	Camera              *camera;
        GPContext           *context;
	CameraAbilitiesList *abilities_list;
	GPPortInfoList      *port_list;
	float                target;
	GthImporterOps       current_op;

	gboolean             camera_setted;
	gboolean             keep_original_filename;
	gboolean             delete_from_camera;
	gboolean             interrupted;
	gboolean             error;

	GList               *categories_list;
	GList               *delete_list;
} DialogData;


/* called when the main dialog is closed. */
static void
destroy_cb (GtkWidget  *widget, 
	    DialogData *data)
{
	if (data->no_camera_pixbuf != NULL)
		g_object_unref  (data->no_camera_pixbuf);
	if (data->camera_present_pixbuf != NULL)
		g_object_unref  (data->camera_present_pixbuf);
	path_list_free (data->categories_list);
	path_list_free (data->delete_list);
	gp_camera_unref (data->camera);
	gp_context_unref (data->context);
	gp_abilities_list_free (data->abilities_list);
	gp_port_info_list_free (data->port_list);
	g_object_unref (data->gui);
	g_free (data);
}


static void
update_ui (void)
{
	while (gtk_events_pending())
		gtk_main_iteration();
	gdk_flush();
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
	char       *msg;

	data->interrupted = FALSE;
	data->target = target;

	gtk_widget_show (data->import_progressbar);
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (data->import_progressbar), 0.0);

	msg = g_strdup_vprintf (format, args);
	gtk_progress_bar_set_text (GTK_PROGRESS_BAR (data->import_progressbar), msg);
	g_free (msg);

	update_ui ();

	return data->current_op;
}


static void
ctx_progress_update_func (GPContext    *context, 
			  unsigned int  id,
                          float         current, 
			  gpointer      callback_data)
{
	DialogData *data = callback_data;
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (data->import_progressbar), current / data->target);
	update_ui ();
}


static gboolean
valid_mime_type (const char *type)
{
	const char *mime_types[] = { GP_MIME_PNG, 
				     GP_MIME_PGM,
				     GP_MIME_PPM, 
				     GP_MIME_PNM, 
				     GP_MIME_JPEG,
				     GP_MIME_TIFF,
				     GP_MIME_BMP };
	int         i;

	for (i = 0; i < G_N_ELEMENTS (mime_types); i++)
		if (strcasecmp (type, mime_types[i]) == 0)
			return TRUE;

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
			gp_camera_file_get_info (data->camera, folder, name, &info, data->context);
			
			if (valid_mime_type (info.file.type))
				file_list = g_list_prepend (file_list, g_build_filename (folder, name, NULL));
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


static char*
get_temp_filename (void)
{
	char       *result = NULL;
        static int  count = 0;
        int         try = 0;
 
        do {
                g_free (result);
                result = g_strdup_printf ("%s%s.%d.%d",
                                          g_get_tmp_dir (),
                                          "/gthumb",
                                          getpid (),
                                          count++);
        } while (path_is_file (result) && (try++ < MAX_TRIES));
 
        return result;

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
load_images_preview (DialogData *data)
{
	GList *file_list, *scan;

	gtk_widget_show (data->import_preview_box);
	gth_image_list_clear (GTH_IMAGE_LIST (data->image_list));

	data->error = FALSE;
	file_list = get_all_files (data, "/");

	if (file_list == NULL) {
		if (!data->error) {
			gtk_image_set_from_stock (GTK_IMAGE (data->progress_info_image),
						  GTK_STOCK_DIALOG_WARNING,
						  GTK_ICON_SIZE_BUTTON);
			gtk_label_set_text (GTK_LABEL (data->progress_info_label), _("No images found"));
			gtk_widget_show (data->progress_info_box);
			update_ui ();
		}
		return;
	} else
		gtk_widget_hide (data->progress_info_box);

	for (scan = file_list; scan; scan = scan->next) {
		const char *camera_path = scan->data;
		CameraFile *file;
		char       *camera_folder;
		const char *camera_filename;
		char       *tmp_filename;

		gp_file_new (&file);

		camera_folder = remove_level_from_path (camera_path);
		camera_filename = file_name_from_path (camera_path);
		gp_camera_file_get (data->camera, 
				    camera_folder, 
				    camera_filename,
				    GP_FILE_TYPE_PREVIEW,
				    file, 
				    data->context);

		tmp_filename = get_temp_filename ();
		if (gp_file_save (file, tmp_filename) >= 0) {
			GdkPixbuf *pixbuf;
			int        width, height;
			FileData  *fdata;

			pixbuf = gdk_pixbuf_new_from_file (tmp_filename, NULL);
			width = gdk_pixbuf_get_width (pixbuf);
			height = gdk_pixbuf_get_height (pixbuf);

			if (scale_keepping_ratio (&width, &height, THUMB_SIZE, THUMB_SIZE)) {
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
							 NULL,
							 NULL,
							 fdata);
			g_object_unref (pixbuf);
			unlink (tmp_filename);
		}
		g_free (tmp_filename);

		gp_file_unref (file);
	}

	task_terminated (data);
}


static void
set_camera_model (DialogData *data,
		  const char *model,
		  const char *port)
{
        int r;

	data->camera_setted = TRUE;

	if ((model == NULL) || (port == NULL)) {
		data->camera_setted = FALSE;
		gtk_widget_hide (data->import_preview_box);
		_gtk_label_set_locale_text (GTK_LABEL (data->camera_model_label), _("No camera detected"));
		gtk_image_set_from_pixbuf (GTK_IMAGE (data->progress_camera_image), data->no_camera_pixbuf);
		return;
	}

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
		_gtk_label_set_locale_text (GTK_LABEL (data->camera_model_label), model);
		gtk_image_set_from_pixbuf (GTK_IMAGE (data->progress_camera_image), data->camera_present_pixbuf);
		load_images_preview (data);

	} else {
		data->camera_setted = FALSE;
		display_error_dialog (data, 
				      _("Could not import photos"),  
				      gp_result_as_string (r));
		_gtk_label_set_locale_text (GTK_LABEL (data->camera_model_label), _("No camera detected"));
		gtk_image_set_from_pixbuf (GTK_IMAGE (data->progress_camera_image), data->no_camera_pixbuf);
	}
}


static gboolean
autodetect_camera (DialogData *data)
{
	CameraList *list = NULL;
	int         count;
	gboolean    detected = FALSE;
	const char *model = NULL, *port = NULL;

	data->current_op = GTH_IMPORTER_OPS_AUTO_DETECT;

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

	task_terminated (data);
	data->interrupted = FALSE;
	update_ui ();
}


static GPContextFeedback
ctx_cancel_func (GPContext *context,
		 gpointer   callback_data)
{
	DialogData *data = callback_data;

	if (data->interrupted) 
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
	char       *msg;

	data->error = TRUE;

	msg = g_strdup_vprintf (format, args);
	gtk_image_set_from_stock (GTK_IMAGE (data->progress_info_image),
				  GTK_STOCK_DIALOG_ERROR,
				  GTK_ICON_SIZE_BUTTON);
	gtk_label_set_text (GTK_LABEL (data->progress_info_label), msg);
	gtk_widget_show (data->progress_info_box);
	update_ui ();

	g_free (msg);
}


static void
ctx_status_func (GPContext  *context, 
		 const char *format, 
		 va_list     args,
		 gpointer    callback_data)
{
	DialogData *data = callback_data;
	char       *msg;

	msg = g_strdup_vprintf (format, args);
	gtk_progress_bar_set_text (GTK_PROGRESS_BAR (data->import_progressbar), msg);

	gtk_image_set_from_stock (GTK_IMAGE (data->progress_info_image),
				  GTK_STOCK_DIALOG_INFO,
				  GTK_ICON_SIZE_BUTTON);
	gtk_label_set_text (GTK_LABEL (data->progress_info_label), msg);
	gtk_widget_show (data->progress_info_box);

	g_free (msg);

	update_ui ();
}


static void
ctx_message_func (GPContext  *context, 
		  const char *format, 
		  va_list     args,
		  gpointer    callback_data)
{
	DialogData *data = callback_data;
	char       *msg;

	msg = g_strdup_vprintf (format, args);
	gtk_progress_bar_set_text (GTK_PROGRESS_BAR (data->import_progressbar), msg);

	gtk_image_set_from_stock (GTK_IMAGE (data->progress_info_image),
				  GTK_STOCK_DIALOG_WARNING,
				  GTK_ICON_SIZE_BUTTON);
	gtk_label_set_text (GTK_LABEL (data->progress_info_label), msg);
	gtk_widget_show (data->progress_info_box);

	g_free (msg);

	update_ui ();
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
	char *entry_val;
	char *destination;
	char *film_name;
	char *path;

	entry_val = _gtk_entry_get_filename_text (GTK_ENTRY (data->destination_entry));
	destination = remove_ending_separator (entry_val);
	g_free (entry_val);

	if (! dlg_check_folder (data->window, destination)) {
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
		strftime (time_txt, 50, "%Y-%m-%d--%H:%M:%S", tm);
	
		film_name = g_strdup (time_txt);
	}

	path = g_build_filename (destination, film_name, NULL);
	g_free (film_name);

	eel_gconf_set_path (PREF_PHOTO_IMPORT_DESTINATION, destination);
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
		set_lowercase (file_name);

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

	cdata = comments_load_comment (filename);
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
	const char *camera_filename;
	char       *local_path;

	gp_file_new (&file);

	camera_folder = remove_level_from_path (camera_path);
	camera_filename = file_name_from_path (camera_path);
	gp_camera_file_get (data->camera, 
			    camera_folder, 
			    camera_filename,
			    GP_FILE_TYPE_NORMAL,
			    file, 
			    data->context);
	g_free (camera_folder);

	local_path = get_file_name (data, camera_path, local_folder, n);

	if (gp_file_save (file, local_path) >= 0) {
		if (data->delete_from_camera) 
			data->delete_list = g_list_prepend (data->delete_list, 
							    g_strdup (camera_path));
		add_categories_to_image (data, local_path);
	}
	
	g_free (local_path);
	gp_file_unref (file);
}


static void
add_film_keyword (const char *folder)
{
	CommentData *cdata;

	cdata = comments_load_comment (folder);
	if (cdata == NULL)
		cdata = comment_data_new ();
	comment_data_add_keyword (cdata, _("Film"));
	comments_save_categories (folder, cdata);
	comment_data_free (cdata);
}


static void
ok_clicked_cb (GtkButton  *button,
	       DialogData *data)
{
	GList *file_list = NULL, *scan;
	char  *local_folder;
	int    n = 1;
	GList *sel_list;

	local_folder = get_folder_name (data);
	if (local_folder == NULL)
		return;

	data->keep_original_filename = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->keep_names_checkbutton));
	data->delete_from_camera = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->delete_checkbutton));

	eel_gconf_set_boolean (PREF_PHOTO_IMPORT_KEEP_FILENAMES, data->keep_original_filename);
	eel_gconf_set_boolean (PREF_PHOTO_IMPORT_DELETE, data->delete_from_camera);
		
	sel_list = gth_image_list_get_selection (GTH_IMAGE_LIST (data->image_list));
	if (sel_list != NULL) {
		for (scan = sel_list; scan; scan = scan->next) {
			FileData   *fdata = scan->data;
			const char *filename = fdata->path;
			file_list = g_list_prepend (file_list, g_strdup (filename));
		}
		if (file_list != NULL)
			file_list = g_list_reverse (file_list);
		file_data_list_free (sel_list);
		
	} else
		file_list = get_all_files (data, "/");
	
	if (file_list == NULL) {
		display_error_dialog (data,
				      _("Could not import photos"), 
				      _("No images found"));
		return;
	}
	
	if (! ensure_dir_exists (local_folder, 0755)) {
		char *msg;
		
		msg = g_strdup_printf (_("Could not create the folder \"%s\": %s"),
				       local_folder,
				       errno_to_string());
		display_error_dialog (data, _("Could not import photos"), msg);
		g_free (msg);
		
		g_free (local_folder);
		return; 
	}

	add_film_keyword (local_folder);

	for (scan = file_list; scan; scan = scan->next) {
		const char *camera_path = scan->data;
		save_image (data, camera_path, local_folder, n++);
		update_ui ();
	}
	path_list_free (file_list);
	
	for (scan = data->delete_list; scan; scan = scan->next) {
		const char *camera_path = scan->data;
		char       *camera_folder;
		const char *camera_filename;
		
		camera_folder = remove_level_from_path (camera_path);
		camera_filename = file_name_from_path (camera_path);
		
		gp_camera_file_delete (data->camera, camera_folder, camera_filename, data->context);
		update_ui ();
	}
	
	task_terminated (data);
	update_ui ();
	
	window_go_to_directory (data->window, local_folder);
	g_free (local_folder);

	gtk_widget_destroy (data->dialog);
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
			const char *filename = fdata->path;
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
		update_ui ();
	}

	path_list_free (delete_list);

	task_terminated (data);
	update_ui ();

	load_images_preview (data);
}


static void dlg_select_camera_model_cb (GtkButton  *button, DialogData *data);


void
dlg_photo_importer (GThumbWindow *window)
{
	DialogData   *data;
	GtkWidget    *btn_ok, *btn_cancel;
	GdkPixbuf    *mute_pixbuf;
	char         *default_path;

	data = g_new0 (DialogData, 1);
	data->window = window;

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

	/* Get the widgets. */

	data->dialog = glade_xml_get_widget (data->gui, "import_photos_dialog");
	data->import_dialog_vbox = glade_xml_get_widget (data->gui, "import_dialog_vbox");
	data->import_preview_scrolledwindow = glade_xml_get_widget (data->gui, "import_preview_scrolledwindow");
	data->camera_model_label = glade_xml_get_widget (data->gui, "camera_model_label");
	data->select_model_button = glade_xml_get_widget (data->gui, "select_model_button");
	data->destination_fileentry = glade_xml_get_widget (data->gui, "destination_fileentry");
	data->destination_entry = glade_xml_get_widget (data->gui, "destination_entry");
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
	btn_ok = glade_xml_get_widget (data->gui, "import_okbutton");
	btn_cancel = glade_xml_get_widget (data->gui, "import_cancelbutton");

	data->image_list = gth_image_list_new (THUMB_SIZE + THUMB_BORDER);
	gtk_widget_show (data->image_list);
	gtk_container_add (GTK_CONTAINER (data->import_preview_scrolledwindow), data->image_list);

	gtk_widget_hide (data->import_preview_box);

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

	default_path = eel_gconf_get_path (PREF_PHOTO_IMPORT_DESTINATION, NULL);
	if ((default_path == NULL) || (*default_path == 0))
		default_path = g_strdup (g_get_home_dir());
	gnome_file_entry_set_default_path (GNOME_FILE_ENTRY (data->destination_fileentry), default_path);
	_gtk_entry_set_filename_text (GTK_ENTRY (data->destination_entry), default_path);
	g_free (default_path);

	/* Set the signals handlers. */

	g_signal_connect (G_OBJECT (data->dialog), 
			  "destroy",
			  G_CALLBACK (destroy_cb),
			  data);
	g_signal_connect (G_OBJECT (btn_ok), 
			  "clicked",
			  G_CALLBACK (ok_clicked_cb),
			  data);
	g_signal_connect_swapped (G_OBJECT (btn_cancel), 
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  G_OBJECT (data->dialog));
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

	/* run dialog. */

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), GTK_WINDOW (window->app));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), FALSE);
	gtk_widget_show (data->dialog);

	data->current_op = GTH_IMPORTER_OPS_LIST_ABILITIES;
	gp_abilities_list_load (data->abilities_list, data->context);
	gp_port_info_list_load (data->port_list);

	autodetect_camera (data);
}





typedef struct {
	DialogData     *data;
	GladeXML       *gui;

	GtkWidget      *dialog;
	GtkWidget      *cm_model_combo;
	GtkWidget      *cm_model_combo_entry;
	GtkWidget      *cm_port_combo;
	GtkWidget      *cm_port_combo_entry;
	GtkWidget      *cm_detect_button;
} ModelDialogData;


static void
model__destroy_cb (GtkWidget       *widget, 
		   ModelDialogData *data)
{
	g_object_unref (data->gui);
	g_free (data);
}


static void
model__ok_clicked_cb (GtkButton       *button,
		      ModelDialogData *mdata)
{
	const char *model, *port;

	model = gtk_entry_get_text (GTK_ENTRY (mdata->cm_model_combo_entry));
	port = gtk_entry_get_text (GTK_ENTRY (mdata->cm_port_combo_entry));

	gtk_widget_hide (mdata->dialog);
	if ((model != NULL) && (*model != 0))
		set_camera_model (mdata->data, model, port);
	gtk_widget_destroy (mdata->dialog);
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
	GList *list = g_list_append (NULL, g_strdup (""));
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


static void
model__model_changed_cb (GtkEditable     *editable,
			 ModelDialogData *mdata)
{
	const char      *model;
	int              m;
	CameraAbilities  a;
	GList           *list;

	model = gtk_entry_get_text (GTK_ENTRY (mdata->cm_model_combo_entry));
	m = gp_abilities_list_lookup_model (mdata->data->abilities_list, model);
	if (m < 0)
		return;
	gp_abilities_list_get_abilities (mdata->data->abilities_list, m, &a);

	list = get_camera_port_list (mdata, a.port);
	gtk_combo_set_popdown_strings (GTK_COMBO (mdata->cm_port_combo), list);
	path_list_free (list);
}


static void
model__autodetect_cb (GtkButton       *button,
		      ModelDialogData *mdata)
{
	DialogData *data = mdata->data;
	CameraList *list = NULL;
	int         count;

	gp_list_new (&list);
	gp_abilities_list_detect (data->abilities_list,
				  data->port_list,
				  list, 
				  data->context);
	count = gp_list_count (list);
	
	if (count >= 1) {
		const char *model = NULL, *port = NULL;
		gp_list_get_name (list, 0, &model);
		gp_list_get_value (list, 0, &port);

		gtk_entry_set_text (GTK_ENTRY (mdata->cm_model_combo_entry), model);
		gtk_entry_set_text (GTK_ENTRY (mdata->cm_port_combo_entry), port);
	}
	
	gp_list_free (list);
}


static void
dlg_select_camera_model_cb (GtkButton  *button,
			    DialogData *data)
{
	ModelDialogData *mdata;
	GtkWidget       *btn_ok, *btn_cancel;
	GList           *list;
	CameraAbilities  a;
	int              r;

	if (autodetect_camera (data))
		return;

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
	mdata->cm_model_combo = glade_xml_get_widget (mdata->gui, "cm_model_combo");
	mdata->cm_model_combo_entry = glade_xml_get_widget (mdata->gui, "cm_model_combo_entry");
	mdata->cm_port_combo = glade_xml_get_widget (mdata->gui, "cm_port_combo");
	mdata->cm_port_combo_entry = glade_xml_get_widget (mdata->gui, "cm_port_combo_entry");
	mdata->cm_detect_button = glade_xml_get_widget (mdata->gui, "cm_detect_button");
	btn_ok = glade_xml_get_widget (mdata->gui, "cm_okbutton");
	btn_cancel = glade_xml_get_widget (mdata->gui, "cm_cancelbutton");

	/* Set widgets data. */

	list = get_camera_model_list (mdata);
	gtk_combo_set_popdown_strings (GTK_COMBO (mdata->cm_model_combo), list);
	path_list_free (list);

	/* Set the signals handlers. */

	g_signal_connect (G_OBJECT (mdata->dialog), 
			  "destroy",
			  G_CALLBACK (model__destroy_cb),
			  mdata);
	g_signal_connect (G_OBJECT (btn_ok), 
			  "clicked",
			  G_CALLBACK (model__ok_clicked_cb),
			  mdata);
	g_signal_connect_swapped (G_OBJECT (btn_cancel), 
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  G_OBJECT (mdata->dialog));
	g_signal_connect (G_OBJECT (mdata->cm_model_combo_entry),
			  "changed",
			  G_CALLBACK (model__model_changed_cb),
			  mdata);
	g_signal_connect (G_OBJECT (mdata->cm_detect_button), 
			  "clicked",
			  G_CALLBACK (model__autodetect_cb),
			  mdata);

	/* run dialog. */

	gtk_window_set_transient_for (GTK_WINDOW (mdata->dialog), GTK_WINDOW (data->dialog));
	gtk_window_set_modal (GTK_WINDOW (mdata->dialog), TRUE);
	gtk_widget_show (mdata->dialog);

	/**/

	r = gp_camera_get_abilities (mdata->data->camera, &a);
	if (r >= 0) {
		GPPortInfo  port_info;

		r = gp_camera_get_port_info (mdata->data->camera, &port_info);
		if (r >= 0) {
			char *port = g_strdup_printf ("%s", port_info.path);
			gtk_entry_set_text (GTK_ENTRY (mdata->cm_port_combo_entry), port);
			g_free (port);
		}

		gtk_entry_set_text (GTK_ENTRY (mdata->cm_model_combo_entry), a.model);
	}
}


#endif /*HAVE_LIBGPHOTO */
