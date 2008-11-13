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

#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <bonobo/bonobo-generic-factory.h>

#include "gth-application.h"
#include "gth-browser.h"
#include "gth-viewer.h"
#include "gth-browser-actions-callbacks.h"
#include "main.h"


static BonoboObject *
gth_application_factory (BonoboGenericFactory *this_factory,
			 const char           *iid,
			 gpointer              user_data)
{
	if (strcmp (iid, "OAFIID:GNOME_GThumb_Application") != 0)
		return NULL;
	else
		return BONOBO_OBJECT (g_object_new (GTH_TYPE_APPLICATION, NULL));
}


BonoboObject *
gth_application_new (GdkScreen *screen)
{
        BonoboGenericFactory *factory;
        char                 *display_name;
        char                 *registration_id;

        display_name = gdk_screen_make_display_name (screen);
        registration_id = bonobo_activation_make_registration_id ("OAFIID:GNOME_GThumb_Application_Factory", display_name);

        factory = bonobo_generic_factory_new (registration_id,
                                              gth_application_factory,
                                              NULL);
	g_free (display_name);
        g_free (registration_id);

        return BONOBO_OBJECT (factory);
}


static void
show_grabbing_focus (GtkWidget *new_window)
{
	const char *startup_id = NULL;
	guint32     timestamp = 0;

	gtk_widget_realize (new_window);

	startup_id = g_getenv ("DESKTOP_STARTUP_ID");
	if (startup_id != NULL) {
		char *startup_id_str = g_strdup (startup_id);
		char *ts;

		ts = g_strrstr (startup_id_str, "_TIME");
		if (ts != NULL) {
			ts = ts + 5;
			errno = 0;
			timestamp = strtoul (ts, NULL, 0);
			if ((errno == EINVAL) || (errno == ERANGE))
				timestamp = 0;
		}

		g_free (startup_id_str);
	}

#ifdef HAVE_GDKX
	if (timestamp == 0)
		timestamp = gdk_x11_get_server_time (new_window->window);
	gdk_x11_window_set_user_time (new_window->window, timestamp);
#endif

	gtk_window_present (GTK_WINDOW (new_window));
}


static void
impl_gth_application_open_browser (PortableServer_Servant  _servant,
				   const CORBA_char       *uri,
				   CORBA_Environment      *ev)
{
	if (*uri == '\0') 
		uri = NULL;
	show_grabbing_focus (gth_browser_new (uri));
}


static void
impl_gth_application_open_viewer (PortableServer_Servant  _servant,
				  const CORBA_char       *uri,
				  CORBA_Environment      *ev)
{
	if (*uri == '\0') 
		uri = NULL;
	show_grabbing_focus (gth_viewer_new (uri));
}


static void
impl_gth_application_load_image (PortableServer_Servant  _servant,
				 const CORBA_char       *uri,
				 CORBA_Environment      *ev)
{
	if (*uri == '\0') 
		uri = NULL;

	if (UseViewer) {
		GtkWidget *viewer = gth_viewer_get_current_viewer ();
		if (viewer == NULL) {
			show_grabbing_focus (gth_viewer_new (uri));
		}
		else {
			gth_viewer_load_from_uri (GTH_VIEWER (viewer), uri);
			show_grabbing_focus (viewer);
		}
	} 
	else {
		GtkWidget *browser = gth_browser_get_current_browser ();
		if (browser == NULL) {
			show_grabbing_focus (gth_browser_new (uri));
		}
		else {
			gth_browser_load_uri (GTH_BROWSER (browser), uri);
			show_grabbing_focus (browser);
		}
	}
}


static void
impl_gth_application_import_photos (PortableServer_Servant  _servant,
				    CORBA_Environment      *ev)
{
	gth_browser_activate_action_file_camera_import (NULL, NULL);
}


static void
gth_application_class_init (GthApplicationClass *klass)
{
        POA_GNOME_GThumb_Application__epv *epv = &klass->epv;
        epv->open_browser = impl_gth_application_open_browser;
        epv->open_viewer = impl_gth_application_open_viewer;
        epv->load_image = impl_gth_application_load_image;
        epv->import_photos = impl_gth_application_import_photos;
}


static void
gth_application_init (GthApplication *c)
{
}


BONOBO_TYPE_FUNC_FULL (
        GthApplication,
        GNOME_GThumb_Application,
        BONOBO_TYPE_OBJECT,
        gth_application);
