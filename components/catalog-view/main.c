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
#include <string.h>

#include <libgnome/libgnome.h>
#include <bonobo-activation/bonobo-activation.h>
#include <libbonobo.h>
#include <libbonoboui.h>
#include <libgnomevfs/gnome-vfs-init.h>

#include "gthumb-init.h"
#include "iids.h"
#include "catalog-control.h"


static BonoboGenericFactory *factory = NULL;
static int active_controls           = 0;
static gboolean init_gthumb          = TRUE;


static void
control_destroy_cb (BonoboControl * control, 
		    gpointer callback_data)
{
        bonobo_object_unref (BONOBO_OBJECT (callback_data));
	active_controls--;

        if (active_controls > 0)
		return;

	gthumb_release ();
	bonobo_object_unref (BONOBO_OBJECT (factory));
	gtk_main_quit ();
}


static BonoboObject *
control_factory (BonoboGenericFactory *factory, 
		 const char           *object_id,
		 gpointer              callback_data)
{
	BonoboControl *control;
	GthFileList   *file_list;

	if (init_gthumb) {
		init_gthumb = FALSE;
		gthumb_init ();
	}

	g_return_val_if_fail (object_id != NULL, NULL);
        if (strcmp (object_id, OAFIID) != 0)
		return NULL;

	file_list = gth_file_list_new ();
	control = catalog_control_new (file_list);

	if (control == NULL) {
		g_object_unref (file_list);
		return NULL;
	}

	g_signal_connect (G_OBJECT (file_list->root_widget), 
			  "destroy",
			  G_CALLBACK (control_destroy_cb), 
			  control);

	active_controls++;

	return BONOBO_OBJECT (control);
}


int 
main (int argc, char **argv)
{
	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	if (! bonobo_ui_init ("gthumb-catalog-view", VERSION, &argc, argv))
		g_error ("Could not initialize Bonobo UI");

	factory = bonobo_generic_factory_new (OAFIID_FACTORY,
					      control_factory,
					      NULL);
	if (factory == NULL)
		g_error ("Could not register the GNOME_GThumb_CatalogView factory");

	bonobo_ui_main ();

	return 0;
}
