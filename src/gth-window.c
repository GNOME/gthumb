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
#include <gtk/gtk.h>
#include "glib-utils.h"
#include "gth-image-history.h"
#include "gth-window.h"
#include "dlg-comment.h"
#include "dlg-tags.h"
#include "main.h"

enum {
        PROP_0,
        PROP_CLOSE_ON_SAVE
};

static GtkWindowClass *parent_class = NULL;
static GList *window_list = NULL;


struct _GthWindowPrivateData {
	GtkWidget       *table;
	GtkWidget       *contents;
	GtkWidget       *comment_dlg;
	GtkWidget       *tags_dlg;
	gboolean         slideshow;
	gboolean         fullscreen;
	GthImageHistory *history;
};


static void
gth_window_finalize (GObject *object)
{
	GthWindow *window = GTH_WINDOW (object);

	if (window->priv != NULL) {
		g_object_unref (window->priv->history);
		g_free (window->priv);
		window->priv = NULL;
	}

	window_list = g_list_remove (window_list, window);

	G_OBJECT_CLASS (parent_class)->finalize (object);

	if (window_list == NULL)
		gtk_main_quit ();
}


static gboolean
gth_window_delete_event (GtkWidget   *widget,
			 GdkEventAny *event)
{
	gth_window_close ((GthWindow*) widget);
	return TRUE;
}


static void
base_close (GthWindow *window)
{
}


static ImageViewer *
base_get_image_viewer (GthWindow *window)
{
	return NULL;
}


static GThumbPreloader *
base_get_preloader (GthWindow *window)
{
	return NULL;
}


static FileData *
base_get_image_data (GthWindow *window)
{
	return NULL;
}


static void
base_set_image_modified (GthWindow *window,
			 gboolean   value)
{
}


static gboolean
base_get_image_modified (GthWindow *window)
{
	return FALSE;
}


static void
base_save_pixbuf (GthWindow *window,
		  GdkPixbuf *pixbuf,
		  FileData  *file)
{
}


static void
base_exec_pixbuf_op (GthWindow   *window,
		     GthPixbufOp *pixop,
		     gboolean     preview)
{
}


static void
base_reload_current_image (GthWindow *window)
{
}


static void
base_update_current_image_metadata (GthWindow *window)
{
}


static GList *
base_get_file_list_selection (GthWindow *window)
{
	return NULL;
}


static GList *
base_get_file_list_selection_as_fd (GthWindow *window)
{
	return NULL;
}


static void
base_set_animation (GthWindow *window,
		    gboolean   value)
{
}


static gboolean
base_get_animation (GthWindow *window)
{
	return FALSE;
}


static void
base_step_animation (GthWindow *window)
{
}


static void
base_set_fullscreen (GthWindow *window,
		     gboolean   value)
{
}


static void
base_set_slideshow (GthWindow *window,
		    gboolean   value)
{
}


static void
gth_window_class_init (GthWindowClass *class)
{
	GObjectClass   *gobject_class;
	GtkWidgetClass *widget_class;

	parent_class = g_type_class_peek_parent (class);
	widget_class = (GtkWidgetClass*) class;
	gobject_class = (GObjectClass*) class;

	gobject_class->finalize = gth_window_finalize;

	widget_class->delete_event = gth_window_delete_event;

	class->close = base_close;
	class->get_image_viewer = base_get_image_viewer;
	class->get_preloader = base_get_preloader;
	class->get_image_data = base_get_image_data;
	class->set_image_modified = base_set_image_modified;
	class->get_image_modified = base_get_image_modified;
	class->save_pixbuf = base_save_pixbuf;
	class->exec_pixbuf_op = base_exec_pixbuf_op;
	class->reload_current_image = base_reload_current_image;
	class->update_current_image_metadata = base_update_current_image_metadata;
	class->get_file_list_selection = base_get_file_list_selection;
	class->get_file_list_selection_as_fd = base_get_file_list_selection_as_fd;
	class->set_animation = base_set_animation;
	class->get_animation = base_get_animation;
	class->step_animation = base_step_animation;
	class->set_fullscreen = base_set_fullscreen;
	class->set_slideshow = base_set_slideshow;
}


static void
gth_window_init (GthWindow *window)
{
	GthWindowPrivateData *priv;

	priv = window->priv = g_new0 (GthWindowPrivateData, 1);

	priv->table = gtk_table_new (4, 1, FALSE);
	gtk_container_add (GTK_CONTAINER (window), priv->table);
	gtk_widget_show(priv->table);

	priv->contents = NULL;
	priv->comment_dlg = NULL;
	priv->tags_dlg = NULL;
	priv->history = gth_image_history_new ();

	window_list = g_list_prepend (window_list, window);
}


GType
gth_window_get_type ()
{
        static GType type = 0;

        if (! type) {
                GTypeInfo type_info = {
			sizeof (GthWindowClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_window_class_init,
			NULL,
			NULL,
			sizeof (GthWindow),
			0,
			(GInstanceInitFunc) gth_window_init
		};

		type = g_type_register_static (GTK_TYPE_WINDOW,
					       "GthWindow",
					       &type_info,
					       0);
	}

        return type;
}


void
gth_window_close (GthWindow *window)
{
	GthWindowPrivateData *priv = window->priv;
	GthWindowClass *class = GTH_WINDOW_GET_CLASS (G_OBJECT (window));

	if (priv->comment_dlg != NULL) {
		dlg_comment_close (priv->comment_dlg);
		priv->comment_dlg = NULL;
	}

	if (priv->tags_dlg != NULL) {
		dlg_tags_close (priv->tags_dlg);
		priv->tags_dlg = NULL;
	}

	class->close (window);
}


ImageViewer *
gth_window_get_image_viewer (GthWindow *window)
{
	GthWindowClass *class = GTH_WINDOW_GET_CLASS (G_OBJECT (window));
	return class->get_image_viewer (window);
}


GThumbPreloader *
gth_window_get_preloader (GthWindow *window)
{
	GthWindowClass *class = GTH_WINDOW_GET_CLASS (G_OBJECT (window));
	return class->get_preloader (window);
}


FileData *
gth_window_get_image_data (GthWindow *window)
{
	GthWindowClass *class = GTH_WINDOW_GET_CLASS (G_OBJECT (window));
	return class->get_image_data (window);	
}


const char *
gth_window_get_image_filename (GthWindow *window)
{
	FileData *file;
	
	file = gth_window_get_image_data (window);
	if (file == NULL)
		return NULL;
	else
		return file->path;
}


void
gth_window_set_image_modified (GthWindow *window,
			       gboolean   modified)
{
	GthWindowClass *class = GTH_WINDOW_GET_CLASS (G_OBJECT (window));
	class->set_image_modified (window, modified);
}


gboolean
gth_window_get_image_modified (GthWindow *window)
{
	GthWindowClass *class = GTH_WINDOW_GET_CLASS (G_OBJECT (window));
	return class->get_image_modified (window);
}


GdkPixbuf *
gth_window_get_image_pixbuf (GthWindow *window)
{
	ImageViewer *image_viewer;

	image_viewer = gth_window_get_image_viewer (window);
	if (image_viewer == NULL)
		return NULL;
	else
		return image_viewer_get_current_pixbuf (image_viewer);
}


void
gth_window_set_image_pixbuf (GthWindow *window,
			     GdkPixbuf *pixbuf)
{
	ImageViewer *image_viewer;

	gth_image_history_add_image (window->priv->history,
				     gth_window_get_image_pixbuf (window),
				     gth_window_get_image_modified (window));

	image_viewer = gth_window_get_image_viewer (window);
	if (image_viewer == NULL)
		return;
	image_viewer_set_pixbuf (image_viewer, pixbuf);
	gth_window_set_image_modified (window, TRUE);
}


void
gth_window_save_pixbuf (GthWindow *window,
			GdkPixbuf *pixbuf,
			FileData  *file)
{
	GthWindowClass *class = GTH_WINDOW_GET_CLASS (G_OBJECT (window));
	class->save_pixbuf (window, pixbuf, file);
}


void
gth_window_exec_pixbuf_op (GthWindow   *window,
			   GthPixbufOp *pixop,
			   gboolean     preview)
{
	GthWindowClass *class = GTH_WINDOW_GET_CLASS (G_OBJECT (window));
	class->exec_pixbuf_op (window, pixop, preview);
}


void
gth_window_undo (GthWindow *window)
{
	ImageViewer  *image_viewer;
	GthImageData *idata;

	image_viewer = gth_window_get_image_viewer (window);
	if (image_viewer == NULL)
		return;

	idata = gth_image_history_undo (window->priv->history,
					gth_window_get_image_pixbuf (window),
				        gth_window_get_image_modified (window));
	if (idata == NULL)
		return;
	image_viewer_set_pixbuf (image_viewer, idata->image);
	gth_window_set_image_modified (window, idata->unsaved);
	gth_image_data_unref (idata);
}


void
gth_window_redo (GthWindow *window)
{
	ImageViewer  *image_viewer;
	GthImageData *idata;

	image_viewer = gth_window_get_image_viewer (window);
	if (image_viewer == NULL)
		return;

	idata = gth_image_history_redo (window->priv->history,
					gth_window_get_image_pixbuf (window),
				        gth_window_get_image_modified (window));
	if (idata == NULL)
		return;
	image_viewer_set_pixbuf (image_viewer, idata->image);
	gth_window_set_image_modified (window, idata->unsaved);
	gth_image_data_unref (idata);
}


void
gth_window_clear_undo_history (GthWindow *window)
{
	gth_image_history_clear (window->priv->history);
}


gboolean
gth_window_get_can_undo (GthWindow *window)
{
	return gth_image_history_can_undo (window->priv->history);
}


gboolean
gth_window_get_can_redo (GthWindow *window)
{
	return gth_image_history_can_redo (window->priv->history);
}


void
gth_window_attach (GthWindow     *window,
		   GtkWidget     *child,
		   GthWindowArea  area)
{
	int position;

	g_return_if_fail (window != NULL);
	g_return_if_fail (GTH_IS_WINDOW (window));
	g_return_if_fail (child != NULL);
	g_return_if_fail (GTK_IS_WIDGET (child));

	switch (area) {
	case GTH_WINDOW_MENUBAR:
		position = 0;
		break;
	case GTH_WINDOW_TOOLBAR:
		position = 1;
		break;
	case GTH_WINDOW_CONTENTS:
		position = 2;
		if (window->priv->contents != NULL)
			gtk_widget_destroy (window->priv->contents);
		window->priv->contents = child;
		break;
	case GTH_WINDOW_STATUSBAR:
		position = 3;
		break;
	default:
		g_critical("%s: area not recognized!", G_STRFUNC);
		return;
		break;
	}

	gtk_table_attach (GTK_TABLE (window->priv->table),
			  child,
			  0, 1,
			  position, position + 1,
			  GTK_EXPAND | GTK_FILL,
			  ((area == GTH_WINDOW_CONTENTS) ? GTK_EXPAND : 0) | GTK_FILL,
			  0, 0);
}


void
gth_window_set_tags_dlg (GthWindow *window,
                         GtkWidget *dialog)
{
	window->priv->tags_dlg = dialog;
}


GtkWidget *
gth_window_get_tags_dlg (GthWindow *window)
{
	return window->priv->tags_dlg;
}


void
gth_window_set_comment_dlg (GthWindow *window,
			    GtkWidget *dialog)
{
	window->priv->comment_dlg = dialog;
}


GtkWidget *
gth_window_get_comment_dlg (GthWindow *window)
{
	return window->priv->comment_dlg;
}


void
gth_window_update_comment_tags_dlg (GthWindow *window)
{
	GthWindowPrivateData *priv = window->priv;

	if (priv->comment_dlg != NULL)
		dlg_comment_update (priv->comment_dlg);

	if (priv->tags_dlg != NULL)
		dlg_tags_update (priv->tags_dlg);
}


void
gth_window_reload_current_image (GthWindow *window)
{
	GthWindowClass *class = GTH_WINDOW_GET_CLASS (G_OBJECT (window));
	class->reload_current_image (window);
}


void
gth_window_update_current_image_metadata (GthWindow *window)
{
	GthWindowClass *class = GTH_WINDOW_GET_CLASS (G_OBJECT (window));
	class->update_current_image_metadata (window);
}


GList *
gth_window_get_file_list_selection (GthWindow *window)
{
	GthWindowClass *class = GTH_WINDOW_GET_CLASS (G_OBJECT (window));
	return class->get_file_list_selection (window);
}


GList *
gth_window_get_file_list_selection_as_fd (GthWindow *window)
{
	GthWindowClass *class = GTH_WINDOW_GET_CLASS (G_OBJECT (window));
	return class->get_file_list_selection_as_fd (window);
}


void
gth_window_set_animation (GthWindow *window,
			  gboolean   value)
{
	GthWindowClass *class = GTH_WINDOW_GET_CLASS (G_OBJECT (window));
	class->set_animation (window, value);
}


gboolean
gth_window_get_animation (GthWindow *window)
{
	GthWindowClass *class = GTH_WINDOW_GET_CLASS (G_OBJECT (window));
	return class->get_animation (window);
}


void
gth_window_step_animation (GthWindow *window)
{
	GthWindowClass *class = GTH_WINDOW_GET_CLASS (G_OBJECT (window));
	class->step_animation (window);
}


void
gth_window_edit_comment (GthWindow *window)
{
	GthWindowPrivateData *priv = window->priv;

	if (priv->comment_dlg == NULL)
		priv->comment_dlg = dlg_comment_new (window);
	else
		gtk_window_present (GTK_WINDOW (priv->comment_dlg));
}


void
gth_window_edit_tags (GthWindow *window)
{
	GthWindowPrivateData *priv = window->priv;

	if (priv->tags_dlg == NULL)
		priv->tags_dlg = dlg_tags_new (window);
	else
		gtk_window_present (GTK_WINDOW (priv->tags_dlg));
}


void
gth_window_set_fullscreen (GthWindow *window,
			   gboolean   value)
{
	GthWindowClass *class = GTH_WINDOW_GET_CLASS (G_OBJECT (window));

	window->priv->fullscreen = value;
	class->set_fullscreen (window, value);
}


gboolean
gth_window_get_fullscreen (GthWindow *window)
{
	return window->priv->fullscreen;
}


void
gth_window_set_slideshow (GthWindow *window,
			  gboolean   value)
{
	GthWindowClass *class = GTH_WINDOW_GET_CLASS (G_OBJECT (window));
	window->priv->slideshow = value;
	class->set_slideshow (window, value);
}


gboolean
gth_window_get_slideshow (GthWindow *window)
{
	return window->priv->slideshow;
}


int
gth_window_get_n_windows (void)
{
	return g_list_length (window_list);
}


GList *
gth_window_get_window_list (void)
{
	return window_list;
}
