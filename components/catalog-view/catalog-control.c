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

#include <config.h>
#include <stdio.h>
#include <string.h>

#include <libgnome/libgnome.h>
#include <libbonobo.h>
#include <libbonoboui.h>

#include "gth-image-list.h"
#include "gth-file-list.h"
#include "gthumb-init.h"
#include "catalog-control.h"
#include "catalog-nautilus-view.h"
#include "gtk-utils.h"
#include "iids.h"


static BonoboControlClass *parent_class;


static BonoboUIVerb verbs [] = {
	/*BONOBO_UI_VERB ("PrintImage",         verb_print_image),*/
	BONOBO_UI_VERB_END
};


static void
activate_control (CatalogControl *catalog_control)
{
	Bonobo_UIContainer  ui_container;
	BonoboUIComponent  *ui_component;
	BonoboControl      *control = BONOBO_CONTROL (catalog_control);

	ui_container = bonobo_control_get_remote_ui_container (control, NULL);
	if (ui_container == CORBA_OBJECT_NIL)
                return;

        ui_component = bonobo_control_get_ui_component (control);
	if (ui_component == CORBA_OBJECT_NIL)
                return;

        bonobo_ui_component_set_container (ui_component, ui_container, NULL);
	bonobo_object_release_unref (ui_container, NULL);

	bonobo_ui_util_set_ui (ui_component, 
			       GNOMEDATADIR,
			       "GNOME_GThumb_CatalogView.xml",
			       "gthumb-catalog-view",
			       NULL);

	bonobo_ui_component_add_verb_list_with_data (ui_component, 
						     verbs, 
						     control);
}


static void
deactivate_control (BonoboControl *control)
{
	BonoboUIComponent *ui_component;

	ui_component = bonobo_control_get_ui_component (control);

	if (ui_component == CORBA_OBJECT_NIL)
                return;

	bonobo_ui_component_unset_container (ui_component, NULL);
}


static void
catalog_control_activate (BonoboControl *object, 
			 gboolean       state)
{
	BonoboControl *control;

	g_return_if_fail (object != NULL);

	control = BONOBO_CONTROL (object);

	if (state) 
		activate_control (CATALOG_CONTROL (control));
	else
		deactivate_control (control);

	if (BONOBO_CONTROL_CLASS (parent_class)->activate)
		BONOBO_CONTROL_CLASS (parent_class)->activate (object, state);
}


static void
catalog_control_finalize (GObject *object)
{
	CatalogControl *control = CATALOG_CONTROL (object);

	g_object_unref (control->file_list);
	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
catalog_control_class_init (CatalogControl *klass)
{
	GObjectClass       *object_class;
	BonoboControlClass *control_class;

	parent_class  = g_type_class_peek_parent (klass);
	object_class  = (GObjectClass *) klass;
	control_class = (BonoboControlClass*) klass;

	object_class->finalize = catalog_control_finalize;
	control_class->activate = catalog_control_activate;
}


static void
catalog_control_init (CatalogControl *control)
{
}


BONOBO_TYPE_FUNC (CatalogControl, BONOBO_TYPE_CONTROL, catalog_control);


static void
file_selection_changed_cb (GtkWidget *widget, 
			   int        pos, 
			   GdkEvent  *event,
			   gpointer   data)
{
	CatalogControl      *control;
	CatalogNautilusView *nautilus_view;
	GList               *selection, *scan;
	Nautilus_URIList    *uri_list;
	Nautilus_URI        *uris;
	int                  i = 0;
	
	control = data;
	nautilus_view = CATALOG_NAUTILUS_VIEW (control->nautilus_view);

	selection = gth_file_list_get_selection (control->file_list);

	uri_list = Nautilus_URIList__alloc();
	CORBA_sequence_set_release (uri_list, FALSE);

	uris =  Nautilus_URIList_allocbuf (g_list_length (selection));

	g_print ("-->\n");

	for (scan = selection; scan; scan = scan->next) {
		char *uri = g_strconcat ("file://", scan->data, NULL);

		uris[i] = CORBA_string_alloc (strlen (uri));
		strcpy (uris[i], uri);
		g_free (uri);

		g_print ("%s\n", uris[i]);
		i++;
	}

	g_print ("<-%d-\n\n", i);

	uri_list->_buffer = uris;
	uri_list->_length = i;

	Nautilus_ViewFrame_report_selection_change (nautilus_view->view_frame, 
						    uri_list, 
						    NULL);

	Nautilus_URIList__freekids (uri_list, NULL);
	CORBA_free (uri_list); 
}


static int
file_button_press_cb (GtkWidget      *widget, 
		      GdkEventButton *event,
		      gpointer        data)
{
	CatalogControl      *control;
	CatalogNautilusView *nautilus_view;

	control = data;
	nautilus_view = CATALOG_NAUTILUS_VIEW (control->nautilus_view);

	if (event->type == GDK_3BUTTON_PRESS) 
		return FALSE;

	if ((event->button != 1) && (event->button != 3))
		return FALSE;

	if ((event->state & GDK_SHIFT_MASK)
	    || (event->state & GDK_CONTROL_MASK))
		return FALSE;

	if (event->button == 1) {
		int pos;

		pos = gth_file_view_get_image_at (control->file_list->view, event->x, event->y);
		if (pos == -1)
			return FALSE;

		if (event->type == GDK_2BUTTON_PRESS) {
			char *path, *uri;
			CORBA_string Uri;

			path = gth_file_list_path_from_pos (control->file_list, pos);
			uri = g_strconcat ("file://", path, NULL);

			Uri = CORBA_string_alloc (strlen (uri));
			strcpy (Uri, uri);
			g_free (uri);
			g_free (path);

			Nautilus_ViewFrame_open_location_in_this_window (nautilus_view->view_frame, Uri, NULL);

			return TRUE;
		}

	} else if (event->button == 3) {
	}

	return FALSE;
}


static void
construct_control (CatalogControl *control,
		   GthFileList    *file_list)
{
	GtkWidget *view_widget;

	g_return_if_fail (control != NULL);
	g_return_if_fail (file_list != NULL);

	/* create the control. */

	control->file_list = file_list;
	gtk_widget_show_all (file_list->root_widget);
	bonobo_control_construct (BONOBO_CONTROL (control), file_list->root_widget);

	/* add the Nautilus::View interface. */

	control->nautilus_view = catalog_nautilus_view_new (control);
	bonobo_object_add_interface (BONOBO_OBJECT (control), control->nautilus_view);

	/* add signal handlers */

	view_widget = gth_file_view_get_widget (file_list->view);
	g_signal_connect (G_OBJECT (view_widget), 
			  "select_image",
			  G_CALLBACK (file_selection_changed_cb), 
			  control);

	g_signal_connect (G_OBJECT (view_widget), 
			  "unselect_image",
			  G_CALLBACK (file_selection_changed_cb), 
			  control);

	g_signal_connect_after (G_OBJECT (view_widget), 
				"button_press_event",
				G_CALLBACK (file_button_press_cb), 
				control);
}


BonoboControl *
catalog_control_new (GthFileList *file_list)
{
	BonoboControl *control;
	
	g_return_val_if_fail (file_list != NULL, NULL);

	control = g_object_new (CATALOG_TYPE_CONTROL, NULL);
	construct_control (CATALOG_CONTROL (control), file_list);

	return control;
}
