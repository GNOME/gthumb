/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2006 Free Software Foundation, Inc.
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

#include "gth-nav-window.h"
#include "gth-iviewer.h"
#include "nav-window.h"
#include "gtk-utils.h"

#include "icons/nav_button.xpm"


struct _GthNavWindowPrivateData {
	GthIViewer *viewer;
	GtkWidget  *viewer_vscr;
	GtkWidget  *viewer_hscr;
	GtkWidget  *viewer_nav_event_box;
};


static GtkHBoxClass *parent_class = NULL;


static void 
gth_nav_window_finalize (GObject *object)
{
	GthNavWindow *nav_window = GTH_NAV_WINDOW (object);

	if (nav_window->priv != NULL) {
		g_free (nav_window->priv);
		nav_window->priv = NULL;
	}

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gth_nav_window_class_init (GthNavWindowClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);

	object_class = G_OBJECT_CLASS (class);
	object_class->finalize = gth_nav_window_finalize;
}


static void
gth_nav_window_init (GthNavWindow *nav_window)
{
	nav_window->priv = g_new0 (GthNavWindowPrivateData, 1);
}


GType
gth_nav_window_get_type ()
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (GthNavWindowClass),
                        NULL,
                        NULL,
                        (GClassInitFunc) gth_nav_window_class_init,
                        NULL,
                        NULL,
                        sizeof (GthNavWindow),
                        0,
                        (GInstanceInitFunc) gth_nav_window_init
                };

                type = g_type_register_static (GTK_TYPE_HBOX,
                                               "GthNavWindow",
                                               &type_info,
                                               0);
        }

        return type;
}


static gboolean
size_changed_cb (GtkWidget    *widget, 
		 GthNavWindow *nav_window)
{
	GthNavWindowPrivateData *priv = nav_window->priv;
	GtkAdjustment           *vadj, *hadj;
	gboolean                 hide_vscr, hide_hscr;

	gth_iviewer_get_adjustments (priv->viewer, &hadj, &vadj);

	g_return_val_if_fail (hadj != NULL, FALSE);
	g_return_val_if_fail (vadj != NULL, FALSE);

	hide_vscr = (vadj->upper <= vadj->page_size);
	hide_hscr = (hadj->upper <= hadj->page_size);

	if (hide_vscr && hide_hscr) {
		gtk_widget_hide (priv->viewer_vscr); 
		gtk_widget_hide (priv->viewer_hscr); 
		gtk_widget_hide (priv->viewer_nav_event_box);
	} else {
		gtk_widget_show (priv->viewer_vscr); 
		gtk_widget_show (priv->viewer_hscr); 
		gtk_widget_show (priv->viewer_nav_event_box);
	}

	return TRUE;	
}


static void
gth_nav_window_construct (GthNavWindow *nav_window, 
			  GthIViewer   *viewer)
{
	GthNavWindowPrivateData *priv = nav_window->priv;
	GtkAdjustment           *vadj = NULL, *hadj = NULL;
	GtkWidget               *hbox;
	GtkWidget               *table;

	priv->viewer = viewer;
	g_signal_connect (G_OBJECT (priv->viewer), 
			  "size_changed",
			  G_CALLBACK (size_changed_cb),
			  nav_window);

	gth_iviewer_get_adjustments (priv->viewer, &hadj, &vadj);
	priv->viewer_hscr = gtk_hscrollbar_new (hadj);
	priv->viewer_vscr = gtk_vscrollbar_new (vadj);

	priv->viewer_nav_event_box = gtk_event_box_new ();
	gtk_container_add (GTK_CONTAINER (priv->viewer_nav_event_box), _gtk_image_new_from_xpm_data (nav_button_xpm));

	g_signal_connect (G_OBJECT (priv->viewer_nav_event_box), 
			  "button_press_event",
			  G_CALLBACK (nav_button_clicked_cb), 
			  priv->viewer);

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (hbox), GTK_WIDGET (priv->viewer));

	table = gtk_table_new (2, 2, FALSE);
	gtk_table_attach (GTK_TABLE (table), hbox, 0, 1, 0, 1,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
	gtk_table_attach (GTK_TABLE (table), priv->viewer_vscr, 1, 2, 0, 1,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
	gtk_table_attach (GTK_TABLE (table), priv->viewer_hscr, 0, 1, 1, 2,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL), 0, 0);
	gtk_table_attach (GTK_TABLE (table), priv->viewer_nav_event_box, 1, 2, 1, 2,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL), 0, 0);

	gtk_widget_show_all (table);

	gtk_container_add (GTK_CONTAINER (nav_window), table);
}


GtkWidget *
gth_nav_window_new (GthIViewer *viewer)
{
	GthNavWindow *nav_window;

	g_return_val_if_fail (viewer != NULL, NULL);

	nav_window = GTH_NAV_WINDOW (g_object_new (GTH_TYPE_NAV_WINDOW, NULL));
	gth_nav_window_construct (nav_window, viewer);

	return (GtkWidget*)nav_window;
}
