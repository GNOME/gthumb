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

#include <libgnome/libgnome.h>
#include <libbonobo.h>

#include "async-pixbuf-ops.h"
#include "gthumb-init.h"
#include "image-viewer.h"
#include "viewer-control.h"
#include "pixbuf-utils.h"
#include "print-callbacks.h"
#include "viewer-nautilus-view.h"


void
verb_rotate (BonoboUIComponent *component, 
	     gpointer callback_data, 
	     const char *cname)
{
	ViewerControl *control = callback_data;
	ImageViewer   *viewer = control->priv->viewer;
	GdkPixbuf     *src_pixbuf;
	GdkPixbuf     *dest_pixbuf;

	src_pixbuf = image_viewer_get_current_pixbuf (viewer);
	dest_pixbuf = _gdk_pixbuf_copy_rotate_90 (src_pixbuf, FALSE);
	image_viewer_set_pixbuf (viewer, dest_pixbuf);
	g_object_unref (dest_pixbuf);
}


void
verb_rotate_180 (BonoboUIComponent *component, 
		 gpointer callback_data, 
		 const char *cname)
{
	ViewerControl *control = callback_data;
	ImageViewer   *viewer = control->priv->viewer;
	GdkPixbuf     *src_pixbuf;
	GdkPixbuf     *dest_pixbuf;

	src_pixbuf = image_viewer_get_current_pixbuf (viewer);
	dest_pixbuf = _gdk_pixbuf_copy_mirror (src_pixbuf, TRUE, TRUE);
	image_viewer_set_pixbuf (viewer, dest_pixbuf);
	g_object_unref (dest_pixbuf);
}


void
verb_flip (BonoboUIComponent *component, 
	   gpointer callback_data, 
	   const char *cname)
{
	ViewerControl *control = callback_data;
	ImageViewer   *viewer = control->priv->viewer;
	GdkPixbuf     *src_pixbuf;
	GdkPixbuf     *dest_pixbuf;

	src_pixbuf = image_viewer_get_current_pixbuf (viewer);
	dest_pixbuf = _gdk_pixbuf_copy_mirror (src_pixbuf, FALSE, TRUE);
	image_viewer_set_pixbuf (viewer, dest_pixbuf);
	g_object_unref (dest_pixbuf);
}


void
verb_mirror (BonoboUIComponent *component, 
	     gpointer callback_data, 
	     const char *cname)
{
	ViewerControl *control = callback_data;
	ImageViewer   *viewer = control->priv->viewer;
	GdkPixbuf     *src_pixbuf;
	GdkPixbuf     *dest_pixbuf;

	src_pixbuf = image_viewer_get_current_pixbuf (viewer);
	dest_pixbuf = _gdk_pixbuf_copy_mirror (src_pixbuf, TRUE, FALSE);
	image_viewer_set_pixbuf (viewer, dest_pixbuf);
	g_object_unref (dest_pixbuf);
}


void
verb_black_white (BonoboUIComponent *component, 
		  gpointer callback_data,
		  const char *cname)
{
	ViewerControl *control = callback_data;
	ImageViewer   *viewer = control->priv->viewer;
	GdkPixbuf     *src_pixbuf;
	GdkPixbuf     *dest_pixbuf;

	src_pixbuf = image_viewer_get_current_pixbuf (viewer);
	dest_pixbuf = gdk_pixbuf_copy (src_pixbuf);
	_gdk_pixbuf_desaturate (dest_pixbuf, dest_pixbuf);
	image_viewer_set_pixbuf (viewer, dest_pixbuf);
	g_object_unref (dest_pixbuf);
}


void
verb_start_stop_ani (BonoboUIComponent *component, 
		     gpointer callback_data, 
		     const char *cname)
{
	ViewerControl *control = callback_data;

	if (! control->priv->viewer->is_animation)
		return;

	if (! control->priv->viewer->play_animation)
		image_viewer_start_animation (control->priv->viewer);
	else
		image_viewer_stop_animation (control->priv->viewer);
}


void
verb_step_ani (BonoboUIComponent *component, 
	       gpointer callback_data, 
	       const char *cname)
{
	ViewerControl *control = callback_data;

	if (! control->priv->viewer->is_animation)
		return;

	image_viewer_step_animation (control->priv->viewer);
}


void
verb_print_image (BonoboUIComponent *component, 
		  gpointer           callback_data, 
		  const char        *cname)
{
	ViewerControl *control = callback_data;
	BonoboObject  *nautilus_view = control->priv->nautilus_view;
	const char    *location;

	location = VIEWER_NAUTILUS_VIEW (nautilus_view)->location;
	print_image_dlg (NULL, control->priv->viewer, location);
}


static struct {
	TranspType transp_type;
	const gchar *verb;
} transp_type_assoc[] = {
	{TRANSP_TYPE_WHITE,   "TransparencyWhite"},
	{TRANSP_TYPE_NONE,    "TransparencyNone"},
	{TRANSP_TYPE_BLACK,   "TransparencyBlack"},
	{TRANSP_TYPE_CHECKED, "TransparencyChecked"},
	{0, NULL}
};
#define TRANSPARENCY_MENU_ITEMS 4


static void 
transp_type_cb (BonoboUIComponent *component,
		const char *path,
		Bonobo_UIComponent_EventType type,
		const char *state,
		gpointer callback_data)
     
{
	ViewerControl *control = callback_data;
        int i;

        if (!atoi (state))
                return; 

	for (i = 0; transp_type_assoc[i].verb != NULL; i++) {
		TranspType transp_type;

                if (strcmp (path, transp_type_assoc[i].verb) != 0) 
			continue;

		transp_type = transp_type_assoc[i].transp_type;
		image_viewer_set_transp_type (control->priv->viewer, transp_type);
		image_viewer_update_view (control->priv->viewer);

		/* Save option. */
		pref_set_transp_type (transp_type);
		return;
	}
}


void
setup_transparency_menu (ViewerControl *control)
{
	ImageViewer *viewer = control->priv->viewer;
	BonoboUIComponent *ui_component;
	TranspType transp_type;
	CORBA_Environment ev;
	char *full_path;
	gint i;

	ui_component =  bonobo_control_get_ui_component (BONOBO_CONTROL (control));
        g_return_if_fail (ui_component != NULL);

	/* update menu status */

	transp_type = image_viewer_get_transp_type (viewer);
	full_path = g_strconcat ("/commands/",
				 transp_type_assoc[transp_type].verb,
				 NULL);

	CORBA_exception_init (&ev);
	bonobo_ui_component_set_prop (ui_component, 
				      full_path, 
				      "state", 
				      "1", 
				      &ev);
	CORBA_exception_free (&ev);
	g_free (full_path);

	/* add listeners */

	for (i = 0; i < TRANSPARENCY_MENU_ITEMS; i++)
		bonobo_ui_component_add_listener (ui_component, 
						  transp_type_assoc[i].verb,
						  transp_type_cb, 
						  control);
}


static struct {
	ZoomQuality zoom_quality;
	const gchar *verb;
} zoom_quality_assoc[] = {
	{ZOOM_QUALITY_HIGH,   "ZoomQualityHigh"},
	{ZOOM_QUALITY_LOW,    "ZoomQualityLow"},
	{0, NULL}
};
#define ZOOM_QUALITY_MENU_ITEMS 2


static void 
zoom_quality_cb (BonoboUIComponent *component,
		 const char *path,
		 Bonobo_UIComponent_EventType type,
		 const char *state,
		 gpointer callback_data)
     
{
        ViewerControl *control = callback_data;
        int i;

        if (!atoi (state))
                return; 

	for (i = 0; zoom_quality_assoc[i].verb != NULL; i++) {
		ZoomQuality zoom_quality;

                if (strcmp (path, zoom_quality_assoc[i].verb) != 0)
			continue;

		zoom_quality = zoom_quality_assoc[i].zoom_quality;
		image_viewer_set_zoom_quality (control->priv->viewer, zoom_quality);

		image_viewer_update_view (control->priv->viewer);

		/* Save option. */
		pref_set_zoom_quality (zoom_quality);
		return;
	}
}


void
setup_zoom_quality_menu (ViewerControl *control)
{
	ImageViewer *viewer = control->priv->viewer;
	BonoboUIComponent *ui_component;
	ZoomQuality zoom_quality;
	CORBA_Environment ev;
	char *full_path;
	gint i;

	ui_component =  bonobo_control_get_ui_component (BONOBO_CONTROL (control));
        g_return_if_fail (ui_component != NULL);

	/* update menu status */

	zoom_quality = image_viewer_get_zoom_quality (viewer);
	full_path = g_strconcat ("/commands/",
				 zoom_quality_assoc[zoom_quality].verb,
				 NULL);

	CORBA_exception_init (&ev);
	bonobo_ui_component_set_prop (ui_component, 
				      full_path, 
				      "state", 
				      "1", 
				      &ev);
	CORBA_exception_free (&ev);
	g_free (full_path);

	/* add listeners */

	for (i = 0; i < ZOOM_QUALITY_MENU_ITEMS; i++)
		bonobo_ui_component_add_listener (ui_component, 
						  zoom_quality_assoc[i].verb,
						  zoom_quality_cb, 
						  control);
}


void
update_menu_sensitivity (ViewerControl *control)
{
	ImageViewer *viewer = control->priv->viewer;
	BonoboUIComponent *ui_component;
	CORBA_Environment ev;
	gboolean is_animation, has_alpha;
	gchar value[2] = {0, 0};

	ui_component =  bonobo_control_get_ui_component (BONOBO_CONTROL (control));
	g_return_if_fail (ui_component != NULL);

	if (bonobo_ui_component_get_container (ui_component) == CORBA_OBJECT_NIL)
		return;

	CORBA_exception_init (&ev);

	/* disable these items when the image is an animation */

	is_animation = image_viewer_is_animation (viewer);
	value[0] = is_animation ? '0' : '1';

	bonobo_ui_component_set_prop (ui_component, 
				      "/commands/ImageRotate",
				      "sensitive", 
				      value, 
				      &ev);
	bonobo_ui_component_set_prop (ui_component, 
				      "/commands/ImageRotate180",
				      "sensitive", 
				      value, 
				      &ev);
	bonobo_ui_component_set_prop (ui_component, 
				      "/commands/ImageFlip",
				      "sensitive", 
				      value, 
				      &ev);
	bonobo_ui_component_set_prop (ui_component, 
				      "/commands/ImageMirror",
				      "sensitive", 
				      value, 
				      &ev);
	bonobo_ui_component_set_prop (ui_component, 
				      "/commands/BlackWhite",
				      "sensitive", 
				      value, 
				      &ev);

	/* enable these items when the image is not an animation */

	value[0] = is_animation ? '1' : '0';

	bonobo_ui_component_set_prop (ui_component, 
				      "/commands/StartStopAnimation",
				      "sensitive", 
				      value, 
				      &ev);

	bonobo_ui_component_set_prop (ui_component, 
				      "/commands/StepAnimation",
				      "sensitive", 
				      value, 
				      &ev);

	/* enable these items if the images has transparencies */

	has_alpha = image_viewer_get_has_alpha (viewer);
	value[0] = has_alpha ? '1' : '0';

	bonobo_ui_component_set_prop (ui_component, 
				      "/commands/TransparencyWhite",
				      "sensitive", 
				      value, 
				      &ev);
	bonobo_ui_component_set_prop (ui_component, 
				      "/commands/TransparencyNone",
				      "sensitive", 
				      value, 
				      &ev);
	bonobo_ui_component_set_prop (ui_component, 
				      "/commands/TransparencyBlack",
				      "sensitive", 
				      value, 
				      &ev);
	bonobo_ui_component_set_prop (ui_component, 
				      "/commands/TransparencyChecked",
				      "sensitive", 
				      value, 
				      &ev);

	CORBA_exception_free (&ev);
}
