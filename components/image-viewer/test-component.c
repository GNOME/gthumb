/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001 The Free Software Foundation, Inc.
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

/* Based on : 
 * sample-control-container.c
 * 
 * Authors:
 *   Nat Friedman  (nat@helixcode.com)
 *   Michael Meeks (michael@helixcode.com)
 *
 * Copyright 1999, 2000 Helix Code, Inc.
 */

#include <fcntl.h>
#include <string.h>

#undef G_DISABLE_DEPRECATED
#undef GDK_DISABLE_DEPRECATED
#undef GDK_PIXBUF_DISABLE_DEPRECATED
#undef GTK_DISABLE_DEPRECATED
#undef GNOME_VFS_DISABLE_DEPRECATED
#undef GNOME_DISABLE_DEPRECATED
#undef BONOBO_DISABLE_DEPRECATED

#include <gtk/gtk.h>
#include <libgnome/libgnome.h>
#include <libgnomeui/libgnomeui.h>
#include <bonobo-activation/bonobo-activation.h>
#include <libbonobo.h>
#include <libbonoboui.h>

#include "iids.h"


static void
app_destroy_cb (GtkWidget *app, BonoboUIContainer *uic)
{
	if (uic != NULL)
		bonobo_object_unref (BONOBO_OBJECT (uic));
	gtk_main_quit ();
}


static int
app_delete_cb (GtkWidget *widget, GdkEvent *event, gpointer dummy)
{
	gtk_widget_destroy (GTK_WIDGET (widget));
	return FALSE;
}


static gboolean
populate_property_list (BonoboControlFrame *cf, GtkCList *clist)
{
	GList *property_list, *l;
	Bonobo_PropertyBag pb;

	pb = bonobo_control_frame_get_control_property_bag (cf, NULL);

	/* Get the list of properties. */
	if (pb == CORBA_OBJECT_NIL) 
		return FALSE;

	property_list = bonobo_pbclient_get_keys (pb, NULL);
	property_list = g_list_sort (property_list, (GCompareFunc) strcmp);
	for (l = property_list; l != NULL; l = l->next) {
		char *row_array[3];
		CORBA_TypeCode tc;
		gchar *name = l->data;

		row_array [0] = name;
		row_array [2] = NULL;

		tc = bonobo_property_bag_client_get_property_type (pb, name, NULL);
		switch (tc->kind) {

		case CORBA_tk_boolean:
			row_array [1] = g_strdup (bonobo_property_bag_client_get_value_gboolean (pb, name, NULL) ? "TRUE" : "FALSE");
			break;

		case CORBA_tk_string:
			row_array [1] = g_strdup (bonobo_property_bag_client_get_value_string (pb, name, NULL));
			break;

		case CORBA_tk_long:
			row_array [1] = g_strdup_printf ("%d", bonobo_property_bag_client_get_value_gint (pb, name, NULL));
			break;

		case CORBA_tk_float:
			row_array [1] = g_strdup_printf ("%2.2f", bonobo_property_bag_client_get_value_gfloat (pb, name, NULL));
			break;

		case CORBA_tk_double:
			row_array [1] = g_strdup_printf ("%2.2g", bonobo_property_bag_client_get_value_gdouble (pb, name, NULL));
			break;

		case CORBA_tk_null:
			row_array [1] = g_strdup ("(null)");
			break;

		default:
			row_array [1] = g_strdup ("Unhandled Property Type");
			break;
		}

		gtk_clist_append (clist, row_array);
		g_free (row_array [1]);
	}
	g_list_free (property_list);

	return TRUE;
}


static void
edit_property (GtkCList *clist, GdkEventButton *event, BonoboControlFrame *cf)
{
	gchar *prop;
	gint row, col;
	GList *l;
	CORBA_TypeCode tc;
	Bonobo_PropertyBag pb;
	Bonobo_PropertyFlags flags;

	pb = bonobo_control_frame_get_control_property_bag (cf, NULL);
	g_return_if_fail (pb != CORBA_OBJECT_NIL);

	if (event->button == 1) 
		return;

	gtk_clist_get_selection_info (clist, event->x, event->y,
				      &row, &col);
	if (row < 0) 
		return;

	l = bonobo_pbclient_get_keys (pb, NULL);
	l = g_list_sort (l, (GCompareFunc) strcmp);
	if (row > g_list_length (l) - 1) {
		g_list_free (l);
		return;
	}

	/* Get the value of the property they clicked on. */

	prop = g_list_nth_data (l, row);

	/* If the property is read-only do nothing. */

	flags = bonobo_pbclient_get_flags (pb, prop, NULL);
	if (! (flags & BONOBO_PROPERTY_WRITEABLE)) {
		g_list_free (l);
		return;
	}

	/* Change it appropriately. */

	tc = bonobo_property_bag_client_get_property_type (pb, prop, NULL);

	switch (tc->kind) {

	case CORBA_tk_boolean:
		bonobo_property_bag_client_set_value_gboolean (pb, prop, !bonobo_property_bag_client_get_value_gboolean (pb, prop, NULL), NULL);
		break;

	case CORBA_tk_double:
		bonobo_property_bag_client_set_value_gdouble (pb, prop, bonobo_property_bag_client_get_value_gdouble (pb, prop, NULL) + ((event->button == 3) ? 1.0 : -1.0 ) * 0.5, NULL);
		break;
		
	default:
		g_warning ("Cannot set_value this type of property yet, sorry.");
		break;
		
	}
	
	g_list_free (l);
		
	/* Redraw the property list. */

	gtk_clist_clear (clist);
	populate_property_list (cf, clist);
}


static GtkWidget *
create_proplist (BonoboControlFrame *cf)
{
	gchar *clist_titles[] = {"Property Name", "Value"};
	GtkWidget *clist;

	/* Put the property CList on the bottom. */
	clist = gtk_clist_new_with_titles (2, clist_titles);

	g_signal_connect (G_OBJECT (clist), 
			  "button_press_event",
			  G_CALLBACK (edit_property), 
			  cf);
 
	populate_property_list (cf, GTK_CLIST (clist));
	gtk_clist_columns_autosize (GTK_CLIST (clist));

	return clist;
}


void
show_property_dialog (BonoboControlFrame *cf)
{
	GtkWidget *dialog;
	GtkWidget *clist;
	GtkWidget *sw;
	
	clist = create_proplist (cf);
	
	if (! clist || ! cf) {
		dialog = gnome_message_box_new (
			_("Component has no editable properties"),
			GNOME_MESSAGE_BOX_INFO, GNOME_STOCK_BUTTON_OK, NULL);
	} else {
		dialog = gnome_dialog_new ("Properties", GNOME_STOCK_BUTTON_OK, NULL);
		
		sw = gtk_scrolled_window_new (NULL, NULL);
		gtk_container_add (GTK_CONTAINER (sw), clist);
		gtk_box_pack_start (GTK_BOX (GNOME_DIALOG (dialog)->vbox),
				    sw, TRUE, TRUE, 0);
		
		gtk_widget_show_all (sw);
	}
	
	gtk_widget_set_usize (dialog, 400, 505);
	gnome_dialog_run_and_close (GNOME_DIALOG (dialog));
}


static gint
show_prop_cb (GtkWidget *widget, BonoboWidget *control)
{
	BonoboControlFrame *cf;
	cf = bonobo_widget_get_control_frame (control);
	show_property_dialog (cf);
	return TRUE;
}


static void
zoom_in_cb (GtkWidget *widget, BonoboWidget *control)
{
	Bonobo_Unknown unknown;
	Bonobo_Zoomable zoomable;
	CORBA_Environment ev;

	CORBA_exception_init (&ev);

	unknown = bonobo_widget_get_objref (control);
	zoomable = Bonobo_Unknown_queryInterface (unknown, "IDL:Bonobo/Zoomable:1.0", &ev);
	if (zoomable == CORBA_OBJECT_NIL)
		return;

	Bonobo_Zoomable_zoomIn (zoomable, &ev);

	bonobo_object_release_unref (zoomable, &ev);
	CORBA_exception_free (&ev);
}


static void
zoom_out_cb (GtkWidget *widget, BonoboWidget *control)
{
	Bonobo_Unknown unknown;
	Bonobo_Zoomable zoomable;
	CORBA_Environment ev;

	CORBA_exception_init (&ev);

	unknown = bonobo_widget_get_objref (control);
	zoomable = Bonobo_Unknown_queryInterface (unknown, "IDL:Bonobo/Zoomable:1.0", &ev);
	if (zoomable == CORBA_OBJECT_NIL)
		return;

	Bonobo_Zoomable_zoomOut (zoomable, &ev);

	bonobo_object_release_unref (zoomable, &ev);
	CORBA_exception_free (&ev);
}


static void
zoom_fit_cb (GtkWidget *widget, BonoboWidget *control)
{
	Bonobo_Unknown unknown;
	Bonobo_Zoomable zoomable;
	CORBA_Environment ev;

	CORBA_exception_init (&ev);

	unknown = bonobo_widget_get_objref (control);
	zoomable = Bonobo_Unknown_queryInterface (unknown, "IDL:Bonobo/Zoomable:1.0", &ev);
	if (zoomable == CORBA_OBJECT_NIL)
		return;

	Bonobo_Zoomable_zoomFit (zoomable, &ev);

	bonobo_object_release_unref (zoomable, &ev);
	CORBA_exception_free (&ev);
}


static void
zoom_to_default_cb (GtkWidget *widget, BonoboWidget *control)
{
	Bonobo_Unknown unknown;
	Bonobo_Zoomable zoomable;
	CORBA_Environment ev;

	CORBA_exception_init (&ev);

	unknown = bonobo_widget_get_objref (control);
	zoomable = Bonobo_Unknown_queryInterface (unknown, "IDL:Bonobo/Zoomable:1.0", &ev);
	if (zoomable == CORBA_OBJECT_NIL)
		return;

	Bonobo_Zoomable_zoomDefault (zoomable, &ev);

	bonobo_object_release_unref (zoomable, &ev);
	CORBA_exception_free (&ev);
}


static GtkFileSelection *fs = NULL;


static void
load_cb (GtkWidget *widget, BonoboWidget *control)
{
	Bonobo_Unknown unknown;
	Bonobo_PersistStream persist;
	Bonobo_Stream stream;
	CORBA_Environment ev;
	gchar *filename;
	gchar *uri;

	filename = g_strdup (gtk_file_selection_get_filename (fs));
	gtk_widget_destroy (GTK_WIDGET (fs));
	if (filename == NULL) 
		return;

	CORBA_exception_init (&ev);

	uri = g_strconcat ("file://", filename, NULL);
	stream = bonobo_get_object (uri, 
				    "IDL:Bonobo/Stream:1.0",
                                    &ev);
	if (stream == CORBA_OBJECT_NIL) {
		char *error_msg;
		error_msg = g_strdup_printf (_("Could not open file %s"), 
					     filename);
		gnome_warning_dialog (error_msg);
		g_free (error_msg);
		return;
	}
	g_free (filename);
	g_free (uri);

	unknown = bonobo_widget_get_objref (control);
	persist = Bonobo_Unknown_queryInterface (unknown, "IDL:Bonobo/PersistStream:1.0", NULL);

	Bonobo_PersistStream_load (persist,
				   stream,
				   "", 
				   &ev);

	bonobo_object_release_unref (persist, &ev);
	Bonobo_Unknown_unref (stream, &ev);
	CORBA_exception_free (&ev);
}


static void
file_dialog_cb (GtkWidget *widget, BonoboWidget *control)
{
	fs = GTK_FILE_SELECTION (gtk_file_selection_new ("Load"));
	gtk_file_selection_hide_fileop_buttons (fs);
	g_signal_connect_swapped (G_OBJECT (fs->cancel_button), 
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  G_OBJECT (fs));
	g_signal_connect (G_OBJECT (fs->ok_button), 
			  "clicked",
			  G_CALLBACK (load_cb),
			  (gpointer) control);

	gtk_widget_show (GTK_WIDGET (fs));
	gtk_window_set_modal (GTK_WINDOW (fs), TRUE);
}


static guint
container_create (void)
{
	GtkWidget       *control;
	GtkWidget       *box, *zoom_box;
	GtkWidget       *button;
	BonoboUIContainer *uic;
	GtkWindow       *window;
	GtkWidget       *app;

	app = bonobo_window_new ("sample-control-container",
				 "Sample Bonobo Control Container");

	window = GTK_WINDOW (app);
	gtk_window_set_default_size (window, 500, 440);
	gtk_window_set_policy (window, TRUE, TRUE, FALSE);
	uic = bonobo_window_get_ui_container (BONOBO_WINDOW (app));

	g_signal_connect (G_OBJECT (window), 
			  "delete_event",
			  G_CALLBACK (app_delete_cb),
			  NULL);

	g_signal_connect (G_OBJECT (window), 
			  "destroy",
			  G_CALLBACK (app_destroy_cb), 
			  uic);

	box = gtk_vbox_new (FALSE, 5);
	bonobo_window_set_contents (BONOBO_WINDOW (app), box);

	control = bonobo_widget_new_control (OAFIID, BONOBO_OBJREF (uic));
	if (control)
		gtk_box_pack_start (GTK_BOX (box), control, TRUE, TRUE, 0);

	zoom_box = gtk_hbox_new (FALSE, 5);
	gtk_box_pack_start (GTK_BOX (box), zoom_box, FALSE, FALSE, 0);

	button = gtk_button_new_with_label ("zoom in");
	gtk_box_pack_start (GTK_BOX (zoom_box), button, FALSE, FALSE, 0);
	g_signal_connect (G_OBJECT (button),
			  "clicked",
			  G_CALLBACK (zoom_in_cb),
			  control);

	button = gtk_button_new_with_label ("zoom out");
	gtk_box_pack_start (GTK_BOX (zoom_box), button, FALSE, FALSE, 0);
	g_signal_connect (G_OBJECT (button), 
			  "clicked",
			  G_CALLBACK (zoom_out_cb),
			  control);

	button = gtk_button_new_with_label ("zoom fit");
	gtk_box_pack_start (GTK_BOX (zoom_box), button, FALSE, FALSE, 0);
	g_signal_connect (G_OBJECT (button), 
			  "clicked",
			  G_CALLBACK (zoom_fit_cb),
			  control);

	button = gtk_button_new_with_label ("zoom to default");
	gtk_box_pack_start (GTK_BOX (zoom_box), button, FALSE, FALSE, 0);
	g_signal_connect (G_OBJECT (button), 
			  "clicked",
			  G_CALLBACK (zoom_to_default_cb),
			  control);

	button = gtk_button_new_with_label ("Prop");
	gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);
	g_signal_connect (G_OBJECT (button), 
			  "clicked",
			  G_CALLBACK (show_prop_cb),
			  control);

	button = gtk_button_new_with_label ("Load");
	gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);
	g_signal_connect (G_OBJECT (button), 
			  "clicked",
			  G_CALLBACK (file_dialog_cb),
			  control);

	button = gtk_button_new_with_label ("Exit");
	gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);
	g_signal_connect_swapped (G_OBJECT (button), 
				  "clicked",
				  G_CALLBACK (app_delete_cb),
				  G_OBJECT (window));

	gtk_widget_show_all (GTK_WIDGET (window));

	return FALSE;
}


int
main (int argc, char **argv)
{
	if (! bonobo_ui_init ("sample-control-container", "0.0", &argc, argv))
		g_error ("Could not initialize Bonobo UI");

	/*
	 * We can't make any CORBA calls unless we're in the main
	 * loop.  So we delay creating the container here.
	 */
	gtk_idle_add ((GtkFunction) container_create, NULL);

	bonobo_ui_main ();

	return 0;
}
