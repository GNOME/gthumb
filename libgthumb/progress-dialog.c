/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001,2003 The Free Software Foundation, Inc.
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
#include <glade/glade.h>
#include "progress-dialog.h"


#define GLADE_FILE "gthumb.glade"
#define DISPLAY_PROGRESS_DELAY 750


struct _ProgressDialog {
	GladeXML    *gui;
  	GtkWidget   *dialog;
	GtkWidget   *progressbar;
	GtkWidget   *info;
	GtkWidget   *cancel;
	guint        display_timeout;
	DoneFunc     done_func;
	gpointer     done_data;
};


static void
destroy_cb (GtkWidget      *widget, 
	    ProgressDialog *pd)
{
	if (pd->display_timeout != 0) {
		g_source_remove (pd->display_timeout);
		pd->display_timeout = 0;
	}
	pd->dialog = NULL;
	if (pd->done_func != NULL)
		(*pd->done_func) (pd->done_data);
}


ProgressDialog *
progress_dialog_new (GtkWindow *parent)
{
	ProgressDialog *pd = g_new0 (ProgressDialog, 1);

	pd->gui = glade_xml_new (GTHUMB_GLADEDIR "/" GLADE_FILE, NULL, NULL);
        if (!pd->gui) {
		g_free (pd);
                g_warning ("Could not find " GLADE_FILE "\n");
                return NULL;
        }

	pd->dialog = glade_xml_get_widget (pd->gui, "progress_dialog");
	pd->progressbar = glade_xml_get_widget (pd->gui, "progress_progressbar");
	pd->info = glade_xml_get_widget (pd->gui, "progress_info");
	pd->cancel = glade_xml_get_widget (pd->gui, "progress_cancel");
	pd->display_timeout = 0;
	pd->done_func = NULL;
	pd->done_data = NULL;

	g_signal_connect (G_OBJECT (pd->dialog), 
			  "destroy",
			  G_CALLBACK (destroy_cb),
			  pd);
	g_signal_connect_swapped (G_OBJECT (pd->cancel), 
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  G_OBJECT (pd->dialog));

	return pd;
}


void
progress_dialog_destroy (ProgressDialog *pd)
{
	pd->done_func = NULL;
	if (pd->dialog != NULL) {
		GtkWidget *w = pd->dialog;
		pd->dialog = NULL;
		gtk_widget_destroy (w);
	}
	g_object_unref (pd->gui);
	g_free (pd);
}


static int
display_dialog (gpointer data)
{
	ProgressDialog *pd = data;

	if (pd->display_timeout != 0) {
		g_source_remove (pd->display_timeout);
		pd->display_timeout = 0;
	}

	gtk_widget_show_all (pd->dialog);

	return FALSE;
}


void
progress_dialog_show (ProgressDialog *pd)
{
	if (pd->display_timeout != 0)
		return;
	pd->display_timeout = g_timeout_add (DISPLAY_PROGRESS_DELAY, display_dialog, pd);
}


void
progress_dialog_hide (ProgressDialog *pd)
{
	if (pd->display_timeout != 0) {
		g_source_remove (pd->display_timeout);
		pd->display_timeout = 0;
	}

	gtk_widget_hide (pd->dialog);
}


void
progress_dialog_set_progress (ProgressDialog *pd,
			      double          fraction)
{
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (pd->progressbar), fraction);
}


void
progress_dialog_set_info (ProgressDialog *pd,
			  const char     *info)
{
	gtk_label_set_text (GTK_LABEL (pd->info), info);
}


void
progress_dialog_set_cancel_func (ProgressDialog *pd,
				 DoneFunc        done_func,
				 gpointer        done_data)
{
	pd->done_func = done_func;
	pd->done_data = done_data;
}
