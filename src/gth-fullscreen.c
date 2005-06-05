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

#include <stdlib.h>
#include <string.h>

#include <glib/gi18n.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <glade/glade.h>
#include <libgnomevfs/gnome-vfs-utils.h>

#ifdef HAVE_LIBEXIF
#include <libexif/exif-data.h>
#include "jpegutils/jpeg-data.h"
#endif /* HAVE_LIBEXIF */

#include "file-utils.h"
#include "glib-utils.h"
#include "gth-fullscreen.h"
#include "gthumb-stock.h"
#include "image-viewer.h"
#include "preferences.h"

#define HIDE_DELAY 1000
#define X_PADDING  12
#define Y_PADDING  6
#define MOTION_THRESHOLD 3


struct _GthFullscreenPrivateData {
	GdkPixbuf  *image;
	GList      *file_list;
	gboolean    first_time_show;
	guint       mouse_hide_id;
	GtkWidget  *viewer;
	GtkWidget  *toolbar_window;
};

static GthWindowClass *parent_class = NULL;


static void 
gth_fullscreen_finalize (GObject *object)
{
	GthFullscreen *fullscreen = GTH_FULLSCREEN (object);

	debug (DEBUG_INFO, "Gth::Fullscreen::Finalize");

	if (fullscreen->priv != NULL) {
		GthFullscreenPrivateData *priv = fullscreen->priv;

		if (priv->toolbar_window != NULL) {
			gtk_widget_destroy (priv->toolbar_window);
			priv->toolbar_window = NULL;
		}

		if (priv->mouse_hide_id != 0)
			g_source_remove (priv->mouse_hide_id);

		if (priv->image != NULL) {
			g_object_unref (priv->image);
			priv->image = NULL;
		}
		path_list_free (priv->file_list);

		/**/

		g_free (fullscreen->priv);
		fullscreen->priv = NULL;
	}

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gth_fullscreen_init (GthFullscreen *fullscreen)
{
	GthFullscreenPrivateData *priv;

	priv = fullscreen->priv = g_new0 (GthFullscreenPrivateData, 1);
	priv->first_time_show = TRUE;
	priv->mouse_hide_id = 0;
}


static GtkWidget*
create_button (const char *stock_icon,
	       const char *label,
	       gboolean    toggle)
{
	GtkWidget *button;
	GtkWidget *image;
	GtkWidget *button_box;

	if (toggle)
		button = gtk_toggle_button_new ();
	else
		button = gtk_button_new ();
	if (pref_get_real_toolbar_style() == GTH_TOOLBAR_STYLE_TEXT_BELOW)
		button_box = gtk_vbox_new (FALSE, 5);
	else
		button_box = gtk_hbox_new (FALSE, 5);
	gtk_container_add (GTK_CONTAINER (button), button_box);

	image = gtk_image_new_from_stock (stock_icon, GTK_ICON_SIZE_BUTTON);
	gtk_box_pack_start (GTK_BOX (button_box), image, FALSE, FALSE, 0);

	if (label != NULL)
		gtk_box_pack_start (GTK_BOX (button_box), gtk_label_new (label), FALSE, FALSE, 0);
	
	return button;
}


static void
create_toolbar_window (GthFullscreen *fullscreen)
{
	GthFullscreenPrivateData *priv = fullscreen->priv;
	GtkWidget *button;
	GtkWidget *frame;
	GtkWidget *hbox;

	priv->toolbar_window = gtk_window_new (GTK_WINDOW_POPUP);
	gtk_window_set_default_size (GTK_WINDOW (priv->toolbar_window),
				     gdk_screen_get_width (gtk_widget_get_screen (priv->toolbar_window)),
				     -1);

	frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);
	gtk_container_set_border_width (GTK_CONTAINER (frame), 1);

	hbox = gtk_hbox_new (FALSE, 6);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 0);
	gtk_container_add (GTK_CONTAINER (frame), hbox);

	/* restore normal view */

	button = create_button (GTHUMB_STOCK_FULLSCREEN, _("Restore Normal View"), FALSE);
	gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
	g_signal_connect_swapped (G_OBJECT (button),
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  fullscreen);
	
	/**/

	gtk_widget_show_all (frame);
	gtk_container_add (GTK_CONTAINER (priv->toolbar_window), frame);
}


static gboolean
hide_mouse_pointer_cb (gpointer data)
{
	GthFullscreen            *fullscreen = data;
	GthFullscreenPrivateData *priv = fullscreen->priv;
	int                       x, y, w, h, px, py;

	gdk_window_get_pointer (priv->toolbar_window->window, &px, &py, 0);
	gdk_window_get_geometry (priv->toolbar_window->window, &x, &y, &w, &h, NULL);

	if ((px >= x) && (px <= x + w) && (py >= y) && (py <= y + h)) 
		return TRUE;

	gtk_widget_hide (priv->toolbar_window);

	image_viewer_hide_cursor (IMAGE_VIEWER (priv->viewer));
	priv->mouse_hide_id = 0;

	/* FIXME
	if (image_data_visible)
		comment_visible = TRUE;
	if (comment_visible) 
		gtk_widget_queue_draw (fullscreen->viewer);
	*/

        return FALSE;
}


static gboolean
motion_notify_event_cb (GtkWidget      *widget, 
			GdkEventMotion *event, 
			gpointer        data)
{
	GthFullscreen            *fullscreen = GTH_FULLSCREEN (widget);
	GthFullscreenPrivateData *priv = fullscreen->priv;
	static int                last_px = 0, last_py = 0;
	int                       px, py;

	gdk_window_get_pointer (widget->window, &px, &py, NULL);

	if (last_px == 0)
		last_px = px;
	if (last_py == 0)
		last_py = py;

	if ((abs (last_px - px) > MOTION_THRESHOLD) 
	    || (abs (last_py - py) > MOTION_THRESHOLD)) 
		if (! GTK_WIDGET_VISIBLE (priv->toolbar_window)) {
			gtk_widget_show (priv->toolbar_window);
			image_viewer_show_cursor (IMAGE_VIEWER (priv->viewer));
		}

	if (priv->mouse_hide_id != 0)
		g_source_remove (priv->mouse_hide_id);
	priv->mouse_hide_id = g_timeout_add (HIDE_DELAY,
					     hide_mouse_pointer_cb,
					     fullscreen);

	last_px = px;
	last_py = py;

	return FALSE;
}


static void
gth_fullscreen_construct (GthFullscreen *fullscreen,
			  GdkPixbuf     *image,
			  GList         *file_list)
{
	GthFullscreenPrivateData *priv = fullscreen->priv;

	if (image != NULL) {
		priv->image = image;
		g_object_ref (image);
	}

	if (file_list != NULL)
		priv->file_list = path_list_dup (file_list);

	create_toolbar_window (fullscreen);

	priv->viewer = image_viewer_new ();
	image_viewer_set_zoom_quality (IMAGE_VIEWER (priv->viewer),
				       pref_get_zoom_quality ());
	image_viewer_set_zoom_change  (IMAGE_VIEWER (priv->viewer),
				       GTH_ZOOM_CHANGE_FIT_IF_LARGER);
	image_viewer_set_check_type   (IMAGE_VIEWER (priv->viewer),
				       pref_get_check_type ());
	image_viewer_set_check_size   (IMAGE_VIEWER (priv->viewer),
				       pref_get_check_size ());
	image_viewer_set_transp_type  (IMAGE_VIEWER (priv->viewer),
				       pref_get_transp_type ());
	image_viewer_set_black_background (IMAGE_VIEWER (priv->viewer), TRUE);
	image_viewer_zoom_to_fit_if_larger (IMAGE_VIEWER (priv->viewer));
	gtk_widget_show (priv->viewer);

	if (priv->image != NULL) 
		image_viewer_set_pixbuf (IMAGE_VIEWER (priv->viewer), priv->image);
	else if (priv->file_list != NULL) {
		char *filename = priv->file_list->data;
		image_viewer_load_image (IMAGE_VIEWER (priv->viewer), filename);
	}

	gnome_app_set_contents (GNOME_APP (fullscreen), priv->viewer);

	/**/

	g_signal_connect (G_OBJECT (fullscreen),
			  "motion_notify_event",
			  G_CALLBACK (motion_notify_event_cb),
			  fullscreen);
}


GtkWidget * 
gth_fullscreen_new (GdkPixbuf *image,
		    GList     *file_list)
{
	GthFullscreen *fullscreen;

	fullscreen = (GthFullscreen*) g_object_new (GTH_TYPE_FULLSCREEN, NULL);
	gth_fullscreen_construct (fullscreen, image, file_list);

	return (GtkWidget*) fullscreen;
}


static void 
gth_fullscreen_show (GtkWidget *widget)
{	
	GthFullscreen *fullscreen = GTH_FULLSCREEN (widget);

	GTK_WIDGET_CLASS (parent_class)->show (widget);

	if (!fullscreen->priv->first_time_show) 
		return;
	fullscreen->priv->first_time_show = FALSE;

	gtk_window_fullscreen (GTK_WINDOW (widget));
	image_viewer_hide_cursor (IMAGE_VIEWER (fullscreen->priv->viewer));
}


static void
gth_fullscreen_close (GthWindow *window)
{
	gtk_widget_destroy (GTK_WIDGET (window));
}


static void
gth_fullscreen_class_init (GthFullscreenClass *class)
{
	GObjectClass   *gobject_class;
	GtkWidgetClass *widget_class;
	GthWindowClass *window_class;

	parent_class = g_type_class_peek_parent (class);
	gobject_class = (GObjectClass*) class;
	widget_class = (GtkWidgetClass*) class;
	window_class = (GthWindowClass*) class;

	gobject_class->finalize = gth_fullscreen_finalize;

	widget_class->show = gth_fullscreen_show;

	window_class->close = gth_fullscreen_close;
	/*
	window_class->get_image_viewer = gth_fullscreen_get_image_viewer;
	window_class->get_image_filename = gth_fullscreen_get_image_filename;
	window_class->get_image_modified = gth_fullscreen_get_image_modified;
	window_class->set_image_modified = gth_fullscreen_set_image_modified;
	window_class->save_pixbuf = gth_fullscreen_save_pixbuf;
	window_class->exec_pixbuf_op = gth_fullscreen_exec_pixbuf_op;

	window_class->set_categories_dlg = gth_fullscreen_set_categories_dlg;
	window_class->get_categories_dlg = gth_fullscreen_get_categories_dlg;
	window_class->set_comment_dlg = gth_fullscreen_set_comment_dlg;
	window_class->get_comment_dlg = gth_fullscreen_get_comment_dlg;
	window_class->reload_current_image = gth_fullscreen_reload_current_image;
	window_class->update_current_image_metadata = gth_fullscreen_update_current_image_metadata;
	window_class->get_file_list_selection = gth_fullscreen_get_file_list_selection;
	window_class->get_file_list_selection_as_fd = gth_fullscreen_get_file_list_selection_as_fd;

	window_class->set_animation = gth_fullscreen_set_animation;
	window_class->get_animation = gth_fullscreen_get_animation;
	window_class->step_animation = gth_fullscreen_step_animation;
	window_class->delete_image = gth_fullscreen_delete_image;
	window_class->edit_comment = gth_fullscreen_edit_comment;
	window_class->edit_categories = gth_fullscreen_edit_categories;
	window_class->set_fullscreen = gth_fullscreen_set_fullscreen;
	window_class->get_fullscreen = gth_fullscreen_get_fullscreen;
	window_class->set_slideshow = gth_fullscreen_set_slideshow;
	window_class->get_slideshow = gth_fullscreen_get_slideshow;
	*/
}


GType
gth_fullscreen_get_type ()
{
        static GType type = 0;

        if (! type) {
                GTypeInfo type_info = {
			sizeof (GthFullscreenClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_fullscreen_class_init,
			NULL,
			NULL,
			sizeof (GthFullscreen),
			0,
			(GInstanceInitFunc) gth_fullscreen_init
		};

		type = g_type_register_static (GTH_TYPE_WINDOW,
					       "GthFullscreen",
					       &type_info,
					       0);
	}

        return type;
}
