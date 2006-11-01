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
#include "gth-window.h"
#include "dlg-comment.h"
#include "dlg-categories.h"

#define MAX_UNDO_HISTORY_LEN 5

enum {
        PROP_0,
        PROP_CLOSE_ON_SAVE
};

static GnomeAppClass *parent_class = NULL;
static GList *window_list = NULL;


struct _GthWindowPrivateData {
	GtkWidget  *comment_dlg;
	GtkWidget  *categories_dlg;
	gboolean    slideshow;
	gboolean    fullscreen;
	GList      *undo_history;  /* ImageData items */
	GList      *redo_history;  /* ImageData items */
};


typedef struct {
	int ref;
	GdkPixbuf *image;
	gboolean   modified;
} ImageData;


static ImageData*
image_data_new (GdkPixbuf *image,
		gboolean   modified)
{
	ImageData *idata;

	g_return_val_if_fail (image != NULL, NULL);

	idata = g_new0 (ImageData, 1);

	idata->ref = 1;
	g_object_ref (image);
	idata->image = image;
	idata->modified = modified;

	return idata;
}


#if 0
static void
image_data_ref (ImageData *idata)
{
	g_return_if_fail (idata != NULL);
	idata->ref++;
}
#endif


static void
image_data_unref (ImageData *idata)
{
	g_return_if_fail (idata != NULL);

	idata->ref--;
	if (idata->ref == 0) {
		g_object_unref (idata->image);
		g_free (idata);
	}
}


static void
image_data_list_free (GList *list)
{
	if (list == NULL)
		return;
	g_list_foreach (list, (GFunc) image_data_unref, NULL);
	g_list_free (list);
}


static void 
gth_window_finalize (GObject *object)
{
	GthWindow *window = GTH_WINDOW (object);

	if (window->priv != NULL) {
		GthWindowPrivateData *priv = window->priv;

		image_data_list_free (priv->undo_history);
		image_data_list_free (priv->redo_history);

		g_free (window->priv);
		window->priv = NULL;
	}

	G_OBJECT_CLASS (parent_class)->finalize (object);
	
	window_list = g_list_remove (window_list, window);
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


static const char *
base_get_image_filename (GthWindow *window)
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
base_save_pixbuf (GthWindow  *window,
		  GdkPixbuf  *pixbuf,
		  const char *filename)
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
	class->get_image_filename = base_get_image_filename;
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
	priv->comment_dlg = NULL;
	priv->categories_dlg = NULL;
	priv->undo_history = NULL;
	priv->redo_history = NULL;

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

		type = g_type_register_static (GNOME_TYPE_APP,
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
	
	if (priv->categories_dlg != NULL) {
		dlg_categories_close (priv->categories_dlg);
		priv->categories_dlg = NULL;
	}

	class->close (window);
}


ImageViewer *
gth_window_get_image_viewer (GthWindow *window)
{
	GthWindowClass *class = GTH_WINDOW_GET_CLASS (G_OBJECT (window));
	return class->get_image_viewer (window);
}


const char *
gth_window_get_image_filename (GthWindow *window)
{
	GthWindowClass *class = GTH_WINDOW_GET_CLASS (G_OBJECT (window));
	return class->get_image_filename (window);
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


static void 
remove_first_image (GList **list)
{
	GList *head;

	head = *list;
	*list = g_list_remove_link (*list, head);
	image_data_list_free (head);
}


static ImageData*
get_nth_image_data (GList *list,
		    int    n)
{
	return (ImageData*) g_list_nth_data (list, n);
}


static GList*
add_image_to_list (GList      *list,
		   GdkPixbuf  *pixbuf,
		   gboolean    modified) 
{
	if (g_list_length (list) > MAX_UNDO_HISTORY_LEN) {
		GList *last;

		last = g_list_nth (list, MAX_UNDO_HISTORY_LEN - 1);
		if (last->prev != NULL) {
			last->prev->next = NULL;
			image_data_list_free (last);
		}
	}

	if (pixbuf == NULL)
		return list;

	return g_list_prepend (list, image_data_new (pixbuf, modified));
}


static void
add_current_image_to_undo_history (GthWindow *window)
{
	GthWindowPrivateData *priv = window->priv;

	priv->undo_history = add_image_to_list (priv->undo_history, 
						gth_window_get_image_pixbuf (window), 
						gth_window_get_image_modified (window));
}


static void
add_current_image_to_redo_history (GthWindow *window)
{
	GthWindowPrivateData *priv = window->priv;
	priv->redo_history = add_image_to_list (priv->redo_history, 
						gth_window_get_image_pixbuf (window), 
						gth_window_get_image_modified (window));
}


void
gth_window_set_image_pixbuf (GthWindow *window,
			     GdkPixbuf *pixbuf)
{
	ImageViewer *image_viewer;

	add_current_image_to_undo_history (window);

	image_data_list_free (window->priv->redo_history);
	window->priv->redo_history = NULL;

	image_viewer = gth_window_get_image_viewer (window);
	if (image_viewer == NULL)
		return;
	image_viewer_set_pixbuf (image_viewer, pixbuf);

	gth_window_set_image_modified (window, TRUE);
}

void
gth_window_save_pixbuf (GthWindow  *window,
			GdkPixbuf  *pixbuf,
			const char *filename)
{
	GthWindowClass *class = GTH_WINDOW_GET_CLASS (G_OBJECT (window));
	class->save_pixbuf (window, pixbuf, filename);
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
	GthWindowPrivateData *priv = window->priv;
	ImageViewer          *image_viewer;
	ImageData            *idata;
	gboolean              modified;

	if (priv->undo_history == NULL)
		return;

	image_viewer = gth_window_get_image_viewer (window);
	if (image_viewer == NULL)
		return;

	add_current_image_to_redo_history (window);

	idata = get_nth_image_data (priv->undo_history, 0);
	image_viewer_set_pixbuf (image_viewer, idata->image);
	modified = idata->modified;
	remove_first_image (&(priv->undo_history));

	gth_window_set_image_modified (window, modified);
}


void
gth_window_redo (GthWindow *window)
{
	GthWindowPrivateData *priv = window->priv;
	ImageViewer          *image_viewer;
	ImageData            *idata;
	gboolean              modified;

	if (priv->redo_history == NULL)
		return;

	image_viewer = gth_window_get_image_viewer (window);
	if (image_viewer == NULL)
		return;

	add_current_image_to_undo_history (window);

	idata = get_nth_image_data (priv->redo_history, 0);
	image_viewer_set_pixbuf (image_viewer, idata->image);
	modified = idata->modified;
	remove_first_image (&(priv->redo_history));

	gth_window_set_image_modified (window, modified);
}


void
gth_window_clear_undo_history (GthWindow *window)
{
	g_return_if_fail (GTH_IS_WINDOW (window));

	image_data_list_free (window->priv->undo_history);
	window->priv->undo_history = NULL;

	image_data_list_free (window->priv->redo_history);
	window->priv->redo_history = NULL;
}


gboolean
gth_window_get_can_undo (GthWindow *window)
{
	g_return_val_if_fail (GTH_IS_WINDOW (window), FALSE);

	return window->priv->undo_history != NULL;
}


gboolean
gth_window_get_can_redo (GthWindow *window)
{
	g_return_val_if_fail (GTH_IS_WINDOW (window), FALSE);
	return window->priv->redo_history != NULL;
}


void
gth_window_set_categories_dlg (GthWindow *window,
			       GtkWidget *dialog)
{
	window->priv->categories_dlg = dialog;
}


GtkWidget *
gth_window_get_categories_dlg (GthWindow *window)
{
	return window->priv->categories_dlg;
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
gth_window_update_comment_categories_dlg (GthWindow *window)
{
	GthWindowPrivateData *priv = window->priv;
	
	if (priv->comment_dlg != NULL)
		dlg_comment_update (priv->comment_dlg);
	
	if (priv->categories_dlg != NULL)
		dlg_categories_update (priv->categories_dlg);
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
gth_window_edit_categories (GthWindow *window)
{
	GthWindowPrivateData *priv = window->priv;

	if (priv->categories_dlg == NULL) 
		priv->categories_dlg = dlg_categories_new (window);
	else
		gtk_window_present (GTK_WINDOW (priv->categories_dlg));
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
