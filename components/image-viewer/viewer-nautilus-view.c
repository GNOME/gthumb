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

#include <libbonobo.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include "viewer-nautilus-view.h"
#include "nautilus-view-component.h"


static BonoboObjectClass *parent_class;


static void
viewer_nautilus_view_load_location (PortableServer_Servant  servant,
				    const char             *location,
				    CORBA_Environment      *ev)
{
	BonoboObject         *object  = bonobo_object_from_servant (servant);
	ViewerNautilusView   *nautilus_view = VIEWER_NAUTILUS_VIEW (object);
	Bonobo_Stream         stream;
	Bonobo_ControlFrame   control_frame;
	Bonobo_PersistStream  persist;
	Nautilus_ViewFrame    view_frame;
	BonoboControl        *control;
	char                 *unesc_loc;

	g_free (nautilus_view->location);
	nautilus_view->location = g_strdup (location);

	/* */

	control = BONOBO_CONTROL (nautilus_view->control);
	control_frame = bonobo_control_get_control_frame (control, ev);
	if (control_frame == CORBA_OBJECT_NIL) 
		return;
	view_frame = Bonobo_Unknown_queryInterface (control_frame, "IDL:Nautilus/ViewFrame:1.0", ev);
	if (view_frame == CORBA_OBJECT_NIL) 
		return;

	nautilus_view->view_frame = view_frame;
	Nautilus_ViewFrame_report_load_underway (view_frame, ev);

	/* */

	unesc_loc = gnome_vfs_unescape_string_for_display (location);
	stream = bonobo_get_object (unesc_loc, "IDL:Bonobo/Stream:1.0", ev);
	if (stream == CORBA_OBJECT_NIL) {
		Nautilus_ViewFrame_report_load_failed (view_frame, ev);
		nautilus_view->view_frame = CORBA_OBJECT_NIL;
		g_free (unesc_loc);
		return;
	}
	g_free (unesc_loc);

	persist = bonobo_object_query_interface (object, "IDL:Bonobo/PersistStream:1.0", ev);
	if (persist == CORBA_OBJECT_NIL) {
		Nautilus_ViewFrame_report_load_failed (view_frame, ev);
		nautilus_view->view_frame = CORBA_OBJECT_NIL;
		return;
	}

	Bonobo_PersistStream_load (persist, stream, "", ev);

	Bonobo_Unknown_unref (stream, ev);
	bonobo_object_release_unref (persist, ev);
}


static void
viewer_nautilus_view_stop_loading (PortableServer_Servant  servant,
				   CORBA_Environment      *ev)
{
	BonoboObject       *object  = bonobo_object_from_servant (servant);
	ViewerNautilusView *nautilus_view = VIEWER_NAUTILUS_VIEW (object);

	/* FIXME */

	nautilus_view->view_frame = CORBA_OBJECT_NIL;
}


static void
viewer_nautilus_view_finalize (GObject *object)
{
	ViewerNautilusView *nautilus_view;

        g_return_if_fail (VIEWER_IS_NAUTILUS_VIEW (object));
	nautilus_view = VIEWER_NAUTILUS_VIEW (object);

	g_free (nautilus_view->location);

        /* Chain up */
	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
viewer_nautilus_view_class_init (ViewerNautilusViewClass *klass)
{
	GObjectClass           *gobject_class;
 	POA_Nautilus_View__epv *epv = &klass->epv;

	parent_class = g_type_class_peek_parent (klass);
	gobject_class = (GObjectClass*) klass;

	gobject_class->finalize = viewer_nautilus_view_finalize;

	epv->load_location = viewer_nautilus_view_load_location;
	epv->stop_loading = viewer_nautilus_view_stop_loading;
}


static void
viewer_nautilus_view_init (ViewerNautilusView *nautilus_view)
{
	nautilus_view->control = NULL;
	nautilus_view->location = NULL;
	nautilus_view->view_frame = CORBA_OBJECT_NIL;
}


BONOBO_TYPE_FUNC_FULL (
	ViewerNautilusView,      /* Glib class name */
	Nautilus_View,           /* CORBA interface name */
	BONOBO_TYPE_OBJECT,      /* parent type */
	viewer_nautilus_view);   /* local prefix ie. 'echo'_class_init */


BonoboObject * 
viewer_nautilus_view_new (ViewerControl *control)
{
	ViewerNautilusView *nautilus_view;

	nautilus_view = VIEWER_NAUTILUS_VIEW (g_object_new (VIEWER_TYPE_NAUTILUS_VIEW, NULL));
	nautilus_view->control = control;

	return (BonoboObject*) nautilus_view;
}

