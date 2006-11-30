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

#include <string.h>
#include <libbonobo.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <libgnomevfs/gnome-vfs-directory.h>
#include "catalog-nautilus-view.h"
#include "nautilus-view-component.h"
#include "file-utils.h"


static BonoboObjectClass *parent_class;


static void
catalog_nautilus_view_load_location (PortableServer_Servant  servant,
				     const char             *location,
				     CORBA_Environment      *ev)
{
	BonoboObject         *object = bonobo_object_from_servant (servant);
	CatalogNautilusView  *nautilus_view = CATALOG_NAUTILUS_VIEW (object);
	BonoboControl        *control;
	Bonobo_ControlFrame   control_frame;
	Nautilus_ViewFrame    view_frame;
	GList                *list, *scan, *file_list;
	GnomeVFSResult        result;
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

	/* */

	if (strncmp (location, "file://", 7) != 0) {
		Nautilus_ViewFrame_report_load_failed (view_frame, ev);
		return;
	}

	Nautilus_ViewFrame_report_load_underway (view_frame, ev);

	result = gnome_vfs_directory_list_load (&list, location,
                                                GNOME_VFS_FILE_INFO_GET_MIME_TYPE
                                                | GNOME_VFS_FILE_INFO_FOLLOW_LINKS);

        if (result != GNOME_VFS_OK) {
		Nautilus_ViewFrame_report_load_failed (view_frame, ev);
		return;
	}

	unesc_loc = gnome_vfs_unescape_string_for_display (location + 7);

	file_list = NULL;
	for (scan = list; scan != NULL; scan = scan->next) {
		GnomeVFSFileInfo *file_info = scan->data;
		char             *path;

		if ((strcmp (file_info->name, ".") == 0)
		    || (strcmp (file_info->name, "..") == 0))
			continue;

		path = g_build_filename (unesc_loc, file_info->name, NULL);

		file_list = g_list_prepend (file_list, path);
	}
	gnome_vfs_file_info_list_free (list);
	g_free (unesc_loc);

	gth_file_list_set_list (nautilus_view->control->file_list, 
				file_list,
				GTH_SORT_METHOD_BY_NAME,
				GTK_SORT_ASCENDING);
}


static void
catalog_nautilus_view_stop_loading (PortableServer_Servant  servant,
				    CORBA_Environment      *ev)
{
	BonoboObject        *object  = bonobo_object_from_servant (servant);
	CatalogNautilusView *nautilus_view = CATALOG_NAUTILUS_VIEW (object);

	/* FIXME */

	nautilus_view->view_frame = CORBA_OBJECT_NIL;
}


static void
catalog_nautilus_view_finalize (GObject *object)
{
	CatalogNautilusView *nautilus_view;

        g_return_if_fail (CATALOG_IS_NAUTILUS_VIEW (object));
	nautilus_view = CATALOG_NAUTILUS_VIEW (object);

	g_free (nautilus_view->location);

        /* Chain up */
	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
catalog_nautilus_view_class_init (CatalogNautilusViewClass *klass)
{
	GObjectClass           *gobject_class;
 	POA_Nautilus_View__epv *epv = &klass->epv;

	parent_class = g_type_class_peek_parent (klass);
	gobject_class = (GObjectClass*) klass;

	gobject_class->finalize = catalog_nautilus_view_finalize;

	epv->load_location = catalog_nautilus_view_load_location;
	epv->stop_loading = catalog_nautilus_view_stop_loading;
}


static void
catalog_nautilus_view_init (CatalogNautilusView *nautilus_view)
{
	nautilus_view->control = NULL;
	nautilus_view->location = NULL;
	nautilus_view->view_frame = CORBA_OBJECT_NIL;
}


BONOBO_TYPE_FUNC_FULL (
	CatalogNautilusView,     /* Glib class name */
	Nautilus_View,           /* CORBA interface name */
	BONOBO_TYPE_OBJECT,      /* parent type */
	catalog_nautilus_view);  /* local prefix ie. 'echo'_class_init */


BonoboObject * 
catalog_nautilus_view_new (CatalogControl *control)
{
	CatalogNautilusView *nautilus_view;

	nautilus_view = CATALOG_NAUTILUS_VIEW (g_object_new (CATALOG_TYPE_NAUTILUS_VIEW, NULL));
	nautilus_view->control = control;

	return (BonoboObject*) nautilus_view;
}

