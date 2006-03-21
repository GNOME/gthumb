/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2005 Free Software Foundation, Inc.
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
#include <string.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <glade/glade.h>

#include "file-data.h"
#include "file-utils.h"
#include "gconf-utils.h"
#include "glib-utils.h"
#include "gthumb-marshal.h"
#include "gth-batch-op.h"
#include "gtk-utils.h"
#include "image-loader.h"
#include "main.h"
#include "pixbuf-utils.h"
#include "dlg-save-image.h"
#include "comments.h"
#ifdef HAVE_LIBEXIF
#include "gth-exif-utils.h"
#endif /* HAVE_LIBEXIF */

#define CONVERT_GLADE_FILE "gthumb_convert.glade"
#define PROGRESS_GLADE_FILE "gthumb_tools.glade"
#define PD(b) (((b)->priv))  /* Private Data */

struct _GthBatchOpPrivateData {
	GtkWindow        *parent;

	/* options */

	const char       *image_type;
	GthPixbufOp      *pixbuf_op;
	gpointer          extra_data;
	gboolean          remove_original;
	GthOverwriteMode  overwrite_mode;

	/* progess data */

	int               images;
	int               image;
	gboolean          stop_operation;

	GList            *file_list;
	GList            *current_image;
	GList            *saved_list;
	GList            *deleted_list;

	GdkPixbuf        *pixbuf;
	ImageLoader      *loader;
	char             *destination;
	char             *new_path;
	char            **keys;
	char            **values;

	/* gui */

	GladeXML         *progress_gui;
	GtkWidget        *progress_dlg;
	GtkWidget        *progress_label;
	GtkWidget        *overall_progressbar;
	GtkWidget        *image_progressbar;

	GladeXML         *gui;
	GtkWidget        *rename_dialog;
	GtkWidget        *conv_ren_message_label;
	GtkWidget        *conv_ren_name_entry;
};


enum {
	BOP_PROGRESS,
	BOP_DONE,
	LAST_SIGNAL
};


static GObjectClass *parent_class = NULL;
static guint         gth_batch_op_signals[LAST_SIGNAL] = { 0 };


static void
free_progress_data (GthBatchOp *bop) 
{
	g_return_if_fail (bop != NULL);

	if (PD(bop) == NULL)
		return;

	if (PD(bop)->loader != NULL) {
		g_object_unref (PD(bop)->loader);
		PD(bop)->loader = NULL;
	}

	if (PD(bop)->progress_dlg != NULL) {
		gtk_widget_destroy (PD(bop)->progress_dlg);
		PD(bop)->progress_dlg = NULL;
	}

	if (PD(bop)->rename_dialog != NULL) {
		gtk_widget_destroy (PD(bop)->rename_dialog);
		PD(bop)->rename_dialog = NULL;
	}

	if (PD(bop)->gui != NULL) {
		g_object_unref (PD(bop)->gui);
		PD(bop)->gui = NULL;
	}

	if (PD(bop)->progress_gui != NULL) {
		g_object_unref (PD(bop)->progress_gui);
		PD(bop)->progress_gui = NULL;
	}

	path_list_free (PD(bop)->saved_list);
	PD(bop)->saved_list = NULL;

	path_list_free (PD(bop)->deleted_list);
	PD(bop)->deleted_list = NULL;

	file_data_list_free (PD(bop)->file_list);
	PD(bop)->file_list = NULL;

	PD(bop)->current_image = NULL;
	PD(bop)->images = 0;
	PD(bop)->image = -1;

	g_free (PD(bop)->destination);
	PD(bop)->destination = NULL;

	g_free (PD(bop)->new_path);
	PD(bop)->new_path = NULL;

	if (PD(bop)->keys != NULL) {
		g_strfreev (PD(bop)->keys);
		PD(bop)->keys = NULL;
	}

	if (PD(bop)->values != NULL) {
		g_strfreev (PD(bop)->values);
		PD(bop)->values = NULL;
	}
}


static void 
gth_batch_op_finalize (GObject *object)
{
	GthBatchOp *bop;

	g_return_if_fail (GTH_IS_BATCH_OP (object));

	bop = (GthBatchOp *) object;
	if (PD(bop) != NULL) {
		free_progress_data (bop);

		if (PD(bop)->pixbuf_op != NULL) {
			g_object_unref (PD(bop)->pixbuf_op);
			PD(bop)->pixbuf_op = NULL;
		}

		g_free (PD(bop));
		PD(bop) = NULL;
	}

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gth_batch_op_class_init (GthBatchOpClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);

	gth_batch_op_signals[BOP_PROGRESS] =
		g_signal_new ("batch_op_progress",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthBatchOpClass, batch_op_progress),
			      NULL, NULL,
			      gthumb_marshal_VOID__FLOAT,
			      G_TYPE_NONE, 
			      1,
			      G_TYPE_FLOAT);
	gth_batch_op_signals[BOP_DONE] =
		g_signal_new ("batch_op_done",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthBatchOpClass, batch_op_done),
			      NULL, NULL,
			      gthumb_marshal_VOID__BOOLEAN,
			      G_TYPE_NONE, 
			      1,
			      G_TYPE_BOOLEAN);

	object_class = G_OBJECT_CLASS (class);
	object_class->finalize = gth_batch_op_finalize;
}


static void
gth_batch_op_init (GthBatchOp *bop)
{
	PD(bop) = g_new0 (GthBatchOpPrivateData, 1);
}


GType
gth_batch_op_get_type ()
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (GthBatchOpClass),
                        NULL,
                        NULL,
                        (GClassInitFunc) gth_batch_op_class_init,
                        NULL,
                        NULL,
                        sizeof (GthBatchOp),
                        0,
                        (GInstanceInitFunc) gth_batch_op_init
                };

                type = g_type_register_static (G_TYPE_OBJECT,
					       "GthBatchOp",
					       &type_info,
					       0);
        }

        return type;
}


GthBatchOp *
gth_batch_op_new (GthPixbufOp *pixbuf_op,
		  gpointer     extra_data)
{
	GthBatchOp *bop;
	
	g_return_val_if_fail (pixbuf_op != NULL, NULL);
	
	bop = GTH_BATCH_OP (g_object_new (GTH_TYPE_BATCH_OP, NULL));
	gth_batch_op_set_op (bop, pixbuf_op, extra_data);
	
	return bop;
}


static void pixbuf_op_done_cb (GthPixbufOp *pixop,
			       gboolean     completed,
			       GthBatchOp  *bop);


static void
pixbuf_op_progress_cb (GthPixbufOp *pixop,
		       float        p, 
		       GthBatchOp  *bop)
{
	if (PD(bop)->progress_dlg != NULL) 
		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (PD(bop)->image_progressbar), p);
}


void
gth_batch_op_set_op (GthBatchOp  *bop,
		     GthPixbufOp *pixbuf_op,
		     gpointer     extra_data)
{
	g_return_if_fail (GTH_IS_BATCH_OP (bop));
	g_return_if_fail (pixbuf_op != NULL);

	if (PD(bop)->pixbuf_op != NULL) {
		g_signal_handlers_disconnect_by_data (PD(bop)->pixbuf_op, bop);
		g_object_unref (PD(bop)->pixbuf_op);
		PD(bop)->pixbuf_op = NULL;
	}

	g_object_ref (pixbuf_op);
	PD(bop)->pixbuf_op = pixbuf_op;
	PD(bop)->extra_data = extra_data;

	g_signal_connect (G_OBJECT (pixbuf_op),
			  "pixbuf_op_done",
			  G_CALLBACK (pixbuf_op_done_cb),
			  bop);
	g_signal_connect (G_OBJECT (pixbuf_op),
			  "pixbuf_op_progress",
			  G_CALLBACK (pixbuf_op_progress_cb),
			  bop);
}


static void
stop_operation_cb (GtkWidget  *widget, 
		   GthBatchOp *bop)
{
	g_return_if_fail (GTH_IS_BATCH_OP (bop));
	PD(bop)->stop_operation = TRUE;
}


static void load_current_image (GthBatchOp *bop);


static void
load_next_image (GthBatchOp *bop)
{
	if (PD(bop)->pixbuf != NULL) {
		g_object_unref (PD(bop)->pixbuf);
		PD(bop)->pixbuf = NULL;
	}
	PD(bop)->image++;
	PD(bop)->current_image = g_list_next (PD(bop)->current_image);
	load_current_image (bop);
}


static void
notify_termination (GthBatchOp *bop)
{
	if (PD(bop)->deleted_list != NULL)
		all_windows_notify_files_deleted (PD(bop)->deleted_list);
	if (PD(bop)->saved_list != NULL)
		all_windows_notify_files_created (PD(bop)->saved_list);
	all_windows_add_monitor ();

	g_signal_emit (G_OBJECT (bop),
		       gth_batch_op_signals[BOP_DONE],
		       0,
		       ! PD(bop)->stop_operation);
}


static void
load_current_image (GthBatchOp *bop)
{
	FileData   *fd;
	char       *folder;
	char       *name_no_ext;
	char       *utf8_name;
	char       *message;
	const char *file_mime_type;

	if (PD(bop)->stop_operation || (PD(bop)->current_image == NULL)) {
		notify_termination (bop);
		return;
	}

	g_signal_emit (G_OBJECT (bop),
		       gth_batch_op_signals[BOP_PROGRESS],
		       0,
		       (float) PD(bop)->image / (PD(bop)->images+1));

	g_free (PD(bop)->new_path);
	PD(bop)->new_path = NULL;

	fd = (FileData*) PD(bop)->current_image->data;
	if (PD(bop)->destination == NULL)
		folder = remove_level_from_path (PD(bop)->new_path);
	else
		folder = g_strdup (PD(bop)->destination);
	name_no_ext = remove_extension_from_path (file_name_from_path (fd->path));

	file_mime_type = get_file_mime_type (fd->path, eel_gconf_get_boolean (PREF_FAST_FILE_TYPE, TRUE)); 
	if (strcmp_null_tollerant (file_mime_type, get_mime_type_from_ext (PD(bop)->image_type)) == 0)
		PD(bop)->new_path = g_strconcat (folder, "/", file_name_from_path (fd->path), NULL); 
	else
		PD(bop)->new_path = g_strconcat (folder, "/", name_no_ext, ".", PD(bop)->image_type, NULL);

	g_free (folder);
	g_free (name_no_ext);

	utf8_name = g_filename_display_basename (PD(bop)->new_path);
	message = g_strdup_printf (_("Converting image: %s"), utf8_name);

	gtk_label_set_text (GTK_LABEL (PD(bop)->progress_label), message);

	g_free (utf8_name);
	g_free (message);

	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (PD(bop)->overall_progressbar),
				       (double) PD(bop)->image / PD(bop)->images);
	
	image_loader_set_path (PD(bop)->loader, fd->path);
	image_loader_start (PD(bop)->loader);
}


static void
show_rename_dialog (GthBatchOp *bop)
{
	char  *message;
	char  *utf8_name;

	utf8_name = g_filename_display_basename (PD(bop)->new_path);

	message = g_strdup_printf (_("An image named \"%s\" is already present. " "Please specify a different name."), utf8_name);
	
	_gtk_label_set_locale_text (GTK_LABEL (PD(bop)->conv_ren_message_label), message);
	g_free (message);
	
	gtk_entry_set_text (GTK_ENTRY (PD(bop)->conv_ren_name_entry), utf8_name);

	g_free (utf8_name);

	gtk_window_set_modal (GTK_WINDOW (PD(bop)->rename_dialog), TRUE);
	gtk_widget_show (PD(bop)->rename_dialog);
	gtk_widget_grab_focus (PD(bop)->conv_ren_name_entry);
}


static void
pixbuf_op_done_cb (GthPixbufOp *pixop,
		   gboolean     completed,
		   GthBatchOp  *bop)
{
	GError   *error = NULL;
	FileData *fd = PD(bop)->current_image->data;

	if (!completed) {
		notify_termination (bop);
		return;
	}

	if (_gdk_pixbuf_savev (pixop->dest,
			       PD(bop)->new_path, 
			       PD(bop)->image_type,
			       PD(bop)->keys, 
			       PD(bop)->values,
			       &error)) {
		PD(bop)->saved_list = g_list_prepend (PD(bop)->saved_list, g_strdup (PD(bop)->new_path));

		if (! same_uri (fd->path, PD(bop)->new_path)) {
			comment_copy (fd->path, PD(bop)->new_path);
			if (PD(bop)->remove_original) {
				file_unlink (fd->path);
				PD(bop)->deleted_list = g_list_prepend (PD(bop)->deleted_list, g_strdup (fd->path));
			}
		}

	} else 
		_gtk_error_dialog_from_gerror_run (PD(bop)->parent, &error);

	load_next_image (bop);
}


static void
save_image_and_remove_original (GthBatchOp *bop)
{
	GdkPixbuf *src, *dest;

	if (path_is_file (PD(bop)->new_path))
		file_unlink (PD(bop)->new_path);

	src = PD(bop)->pixbuf;
	if (PD(bop)->pixbuf_op->step_func == NULL)
		dest = NULL;
	else
		dest = src;
	gth_pixbuf_op_set_pixbufs (PD(bop)->pixbuf_op, src, dest);
	gth_pixbuf_op_start (PD(bop)->pixbuf_op); 
}


static void
rename_response_cb (GtkWidget  *dialog,
		    int         response_id,
		    GthBatchOp *bop)
{
	char *new_name, *folder;

	gtk_widget_hide (dialog);
	gtk_window_set_modal (GTK_WINDOW (dialog), FALSE);

	if (response_id != GTK_RESPONSE_OK) {
		load_next_image (bop);
		return;
	}

	new_name = _gtk_entry_get_filename_text (GTK_ENTRY (PD(bop)->conv_ren_name_entry));

	g_free (PD(bop)->new_path);

	if (PD(bop)->destination == NULL)
		folder = remove_level_from_path (PD(bop)->new_path);
	else
		folder = g_strdup (PD(bop)->destination);
	PD(bop)->new_path = g_strconcat (folder, "/", new_name, NULL);
	g_free (folder);
	g_free (new_name);

	if (path_is_file (PD(bop)->new_path)) 
		show_rename_dialog (bop);
	else 
		save_image_and_remove_original (bop);
}


static void
overwrite_response_cb (GtkWidget  *dialog,
		       int         response_id,
		       GthBatchOp *bop)
{
	gtk_widget_destroy (dialog);

	if (response_id == GTK_RESPONSE_YES) 
		save_image_and_remove_original (bop);
	else
		load_next_image (bop);
}


static void
loader_done (ImageLoader *il,
	     GthBatchOp  *bop)
{
	gboolean save_image;

	PD(bop)->pixbuf = image_loader_get_pixbuf (il);
	if (PD(bop)->pixbuf == NULL) {
		load_next_image (bop);
		return;
	}

	g_object_ref (PD(bop)->pixbuf);

	save_image = TRUE;
	if (path_is_file (PD(bop)->new_path)) {
		GtkWidget *d;
		char      *message;
		char      *utf8_name;

		switch (PD(bop)->overwrite_mode) {
		case GTH_OVERWRITE_SKIP:
			save_image = FALSE;
			break;

		case GTH_OVERWRITE_OVERWRITE:
			save_image = TRUE;
			break;

		case GTH_OVERWRITE_ASK:
			utf8_name = g_filename_display_basename (PD(bop)->new_path);
			message = g_strdup_printf (_("An image named \"%s\" is already present. " "Do you want to overwrite it?"), utf8_name);

			d = _gtk_yesno_dialog_new (GTK_WINDOW (PD(bop)->progress_dlg),
						   GTK_DIALOG_MODAL,
						   message,
						   _("Skip"),
						   _("_Overwrite"));

			g_signal_connect (G_OBJECT (d), "response",
					  G_CALLBACK (overwrite_response_cb),
					  bop);

			g_free (message);
			g_free (utf8_name);

			gtk_widget_show (d);
			return;
			
		case GTH_OVERWRITE_RENAME:
			show_rename_dialog (bop);
			return;
		}
	} 

	if (save_image)
		save_image_and_remove_original (bop);
	else
		load_next_image (bop);	
}


static void
loader_error (ImageLoader *il,
	      GthBatchOp  *bop)
{
	load_next_image (bop);
}


static gboolean
progress_dlg_delete_event_cb (GtkWidget  *caller, 
			      GdkEvent   *event, 
			      GthBatchOp *bop)
{
	notify_termination (bop);
	return TRUE;
}


static gboolean
rename_dlg_delete_event_cb (GtkWidget  *caller, 
			    GdkEvent   *event, 
			    GthBatchOp *bop)
{
	return TRUE;
}


void
gth_batch_op_start (GthBatchOp       *bop,
		    const char       *image_type,
		    GList            *file_list,
		    const char       *destination,
		    GthOverwriteMode  overwrite_mode,
		    gboolean          remove_original,
		    GtkWindow        *window)
{
	GtkWidget *progress_cancel;

	g_return_if_fail (GTH_IS_BATCH_OP (bop));

	PD(bop)->parent = window;

	all_windows_remove_monitor ();
	PD(bop)->loader = IMAGE_LOADER (image_loader_new (NULL, FALSE));
	g_signal_connect (G_OBJECT (PD(bop)->loader),
			  "image_done",
			  G_CALLBACK (loader_done),
			  bop);
	g_signal_connect (G_OBJECT (PD(bop)->loader),
			  "image_error",
			  G_CALLBACK (loader_error),
			  bop);
	
	/**/

	PD(bop)->gui = glade_xml_new (GTHUMB_GLADEDIR "/" CONVERT_GLADE_FILE, NULL, NULL);
	if (! PD(bop)->gui) {
		g_warning ("Could not find " CONVERT_GLADE_FILE "\n");
		return;
	}

	PD(bop)->progress_gui = glade_xml_new (GTHUMB_GLADEDIR "/" PROGRESS_GLADE_FILE, NULL, NULL);
	if (! PD(bop)->progress_gui) {
		g_warning ("Could not find " PROGRESS_GLADE_FILE "\n");
		return;
	}

	PD(bop)->rename_dialog = glade_xml_get_widget (PD(bop)->gui, "convert_rename_dialog");
	PD(bop)->conv_ren_message_label = glade_xml_get_widget (PD(bop)->gui, "conv_ren_message_label");
	PD(bop)->conv_ren_name_entry = glade_xml_get_widget (PD(bop)->gui, "conv_ren_name_entry");

	PD(bop)->progress_dlg = glade_xml_get_widget (PD(bop)->progress_gui, "double_progress_dialog");
	PD(bop)->progress_label = glade_xml_get_widget (PD(bop)->progress_gui, "p_progress_label");
	PD(bop)->overall_progressbar = glade_xml_get_widget (PD(bop)->progress_gui, "p_overall_progressbar");
	PD(bop)->image_progressbar = glade_xml_get_widget (PD(bop)->progress_gui, "p_image_progressbar");

	progress_cancel = glade_xml_get_widget (PD(bop)->progress_gui, "p_cancelbutton");

	gtk_window_set_transient_for (GTK_WINDOW (PD(bop)->rename_dialog), 
				      GTK_WINDOW (PD(bop)->progress_dlg));

	/* Set the signals handlers. */
	
	g_signal_connect (G_OBJECT (progress_cancel), 
			  "clicked",
			  G_CALLBACK (stop_operation_cb),
			  bop);
	g_signal_connect (G_OBJECT (PD(bop)->rename_dialog), 
			  "response",
			  G_CALLBACK (rename_response_cb),
			  bop);
	g_signal_connect (G_OBJECT (PD(bop)->progress_dlg),
			  "delete_event",
			  G_CALLBACK (progress_dlg_delete_event_cb),
			  bop);
	g_signal_connect (G_OBJECT (PD(bop)->rename_dialog),
			  "delete_event",
			  G_CALLBACK (rename_dlg_delete_event_cb),
			  bop);

	/* Run dialog. */

	PD(bop)->image_type = image_type;
	PD(bop)->file_list = file_data_list_dup (file_list);
	if (destination != NULL)
		PD(bop)->destination = g_strdup (destination);
	PD(bop)->current_image = PD(bop)->file_list;
	PD(bop)->overwrite_mode = overwrite_mode;
	PD(bop)->remove_original = remove_original;
	PD(bop)->images = g_list_length (PD(bop)->file_list);
	PD(bop)->image = 1;

	if (!dlg_save_options (GTK_WINDOW (PD(bop)->parent), 
			       PD(bop)->image_type,
			       &(PD(bop)->keys), 
			       &(PD(bop)->values))) {
		free_progress_data (bop);
		return;
	}

	gtk_window_set_transient_for (GTK_WINDOW (PD(bop)->progress_dlg), 
				      GTK_WINDOW (PD(bop)->parent));
	gtk_window_set_modal (GTK_WINDOW (PD(bop)->progress_dlg), TRUE); 
	gtk_widget_show (PD(bop)->progress_dlg);

	if (PD(bop)->pixbuf_op->single_step)
		gtk_widget_hide (PD(bop)->image_progressbar);

	load_current_image (bop);
}


void
gth_batch_op_stop (GthBatchOp *bop)
{
	g_return_if_fail (GTH_IS_BATCH_OP (bop));
	PD(bop)->stop_operation = TRUE;
}

