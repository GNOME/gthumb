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

#include <libgnome/libgnome.h>
#include <libbonobo.h>
#include <libbonoboui.h>

#include "gthumb-init.h"
#include "image-viewer.h"
#include "nav-window.h"
#include "viewer-control.h"
#include "viewer-nautilus-view.h"
#include "viewer-stream.h"
#include "ui.h"
#include "gtk-utils.h"
#include "iids.h"

#include "nav_button.xpm"


enum {
	ARG_0,

	ARG_IS_VOID,

	ARG_IMAGE_WIDTH, 
	ARG_IMAGE_HEIGHT,
	ARG_IMAGE_BPS, 
	ARG_HAS_TRANSPARENCY,

	ARG_RUNNING_ANIMATION, 
	ARG_IS_ANIMATION,

	ARG_ZOOM_VALUE,
	ARG_ZOOM_FIT,
	ARG_ZOOM_QUALITY_HIGH,
	ARG_ZOOM_CHANGE_ACTUAL_SIZE,
	ARG_ZOOM_CHANGE_FIT,
	ARG_ZOOM_CHANGE_KEEP_PREV,

	ARG_TRANSP_TYPE_WHITE,
	ARG_TRANSP_TYPE_NONE,
	ARG_TRANSP_TYPE_BLACK,
	ARG_TRANSP_TYPE_CHECKED,

	ARG_CHECK_TYPE_LIGHT,
	ARG_CHECK_TYPE_MIDTONE,
	ARG_CHECK_TYPE_DARK,

	ARG_CHECK_SIZE_SMALL,
	ARG_CHECK_SIZE_MEDIUM,
	ARG_CHECK_SIZE_LARGE,

	ARG_CURSOR_VISIBLE,
	ARG_BLACK_BACKGROUND
};


static BonoboControlClass *parent_class;


static BonoboUIVerb verbs [] = {
        BONOBO_UI_VERB ("ImageRotate",    verb_rotate),
        BONOBO_UI_VERB ("ImageRotate180", verb_rotate_180),
        BONOBO_UI_VERB ("ImageFlip",      verb_flip),
        BONOBO_UI_VERB ("ImageMirror",    verb_mirror),

	BONOBO_UI_VERB ("StartStopAnimation", verb_start_stop_ani),
	BONOBO_UI_VERB ("StepAnimation",      verb_step_ani),

	BONOBO_UI_VERB ("SaveImage",          verb_save_image),
	BONOBO_UI_VERB ("PrintImage",         verb_print_image),

	BONOBO_UI_VERB_END
};


static void
activate_control (ViewerControl *viewer_control)
{
	Bonobo_UIContainer  ui_container;
	BonoboUIComponent  *ui_component;
	BonoboControl      *control = BONOBO_CONTROL (viewer_control);

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
			       "GNOME_GThumb_Viewer.xml",
			       "gthumb-image-viewer",
			       NULL);

	bonobo_ui_component_add_verb_list_with_data (ui_component, 
						     verbs, 
						     control);

	setup_transparency_menu (viewer_control);
	setup_zoom_quality_menu (viewer_control);
	update_menu_sensitivity (viewer_control); 
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
viewer_control_activate (BonoboControl *object, 
			 gboolean       state)
{
	BonoboControl *control;

	g_return_if_fail (object != NULL);

	control = BONOBO_CONTROL (object);

	if (state) 
		activate_control (VIEWER_CONTROL (control));
	else
		deactivate_control (control);

	if (BONOBO_CONTROL_CLASS (parent_class)->activate)
		BONOBO_CONTROL_CLASS (parent_class)->activate (object, state);
}


static void
viewer_control_finalize (GObject *object)
{
	ViewerControl *control = VIEWER_CONTROL (object);

	g_free (control->priv);
	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
viewer_control_class_init (ViewerControl *klass)
{
	GObjectClass       *object_class;
	BonoboControlClass *control_class;

	parent_class  = g_type_class_peek_parent (klass);
	object_class  = (GObjectClass *) klass;
	control_class = (BonoboControlClass*) klass;

	object_class->finalize = viewer_control_finalize;
	control_class->activate = viewer_control_activate;
}


static void
viewer_control_init (ViewerControl *control)
{
	control->priv = g_new (ViewerControlPrivate, 1);
	control->priv->zoomable = NULL;
	control->priv->viewer   = NULL;
}


BONOBO_TYPE_FUNC (ViewerControl, BONOBO_TYPE_CONTROL, viewer_control);


static GthTranspType
get_transp_type_from_arg (guint arg_id)
{
	GthTranspType transp_type = GTH_TRANSP_TYPE_CHECKED;

	switch (arg_id) {
	case ARG_TRANSP_TYPE_WHITE:
		transp_type = GTH_TRANSP_TYPE_WHITE; break;
	case ARG_TRANSP_TYPE_NONE:
		transp_type = GTH_TRANSP_TYPE_NONE; break;
	case ARG_TRANSP_TYPE_BLACK:
		transp_type = GTH_TRANSP_TYPE_BLACK; break;
	case ARG_TRANSP_TYPE_CHECKED:
		transp_type = GTH_TRANSP_TYPE_CHECKED; break;
	default: break;
	}

	return transp_type;
}


static GthCheckType
get_check_type_from_arg (guint arg_id)
{
	GthCheckType check_type = GTH_CHECK_TYPE_MIDTONE;

	switch (arg_id) {
	case ARG_CHECK_TYPE_LIGHT:
		check_type = GTH_CHECK_TYPE_LIGHT; break;
	case ARG_CHECK_TYPE_MIDTONE:
		check_type = GTH_CHECK_TYPE_MIDTONE; break;
	case ARG_CHECK_TYPE_DARK:
		check_type = GTH_CHECK_TYPE_DARK; break;
	default: break;
	}

	return check_type;
}


static GthCheckSize
get_check_size_from_arg (guint arg_id)
{
	GthCheckSize check_size = GTH_CHECK_SIZE_MEDIUM;

	switch (arg_id) {
	case ARG_CHECK_SIZE_SMALL:
		check_size = GTH_CHECK_SIZE_SMALL; break;
	case ARG_CHECK_SIZE_MEDIUM:
		check_size = GTH_CHECK_SIZE_MEDIUM; break;
	case ARG_CHECK_SIZE_LARGE:
		check_size = GTH_CHECK_SIZE_LARGE; break;
	default: break;
	}

	return check_size;
}


static void
get_prop (BonoboPropertyBag *bag, 
	  BonoboArg         *arg, 
	  guint              arg_id, 
          CORBA_Environment *ev, 
	  gpointer           data)
{
	ViewerControl *control = VIEWER_CONTROL (data);
	ImageViewer   *viewer = control->priv->viewer;
	GthTranspType  transp_type;
	GthCheckType   check_type;
	GthCheckSize   check_size;
	int            int_val;
	gboolean       bool_val;
	double         double_val;

        switch (arg_id) {
        case ARG_IS_VOID:
		bool_val = image_viewer_is_void (viewer);
		BONOBO_ARG_SET_BOOLEAN (arg, bool_val);
		break;

	case ARG_RUNNING_ANIMATION:
		bool_val = image_viewer_is_playing_animation (viewer);
		BONOBO_ARG_SET_BOOLEAN (arg, bool_val);
		break;

	case ARG_IMAGE_WIDTH:
		int_val = image_viewer_get_image_width (viewer);
		BONOBO_ARG_SET_INT (arg, int_val);
		break;
	case ARG_IMAGE_HEIGHT:
		int_val = image_viewer_get_image_height (viewer);
		BONOBO_ARG_SET_INT (arg, int_val);
		break;
	case ARG_IMAGE_BPS: 
		int_val = image_viewer_get_image_bps (viewer);
		BONOBO_ARG_SET_INT (arg, int_val);
		break;
	case ARG_HAS_TRANSPARENCY:
		bool_val = image_viewer_get_has_alpha (viewer);
		BONOBO_ARG_SET_BOOLEAN (arg, bool_val);
		break;

	case ARG_IS_ANIMATION:
		bool_val = image_viewer_is_animation (viewer);
		BONOBO_ARG_SET_BOOLEAN (arg, bool_val);
		break;

	case ARG_ZOOM_VALUE:
		double_val = image_viewer_get_zoom (viewer);
		BONOBO_ARG_SET_DOUBLE (arg, double_val);
		break;
	case ARG_ZOOM_QUALITY_HIGH:
		int_val = image_viewer_get_zoom_quality (viewer);
		BONOBO_ARG_SET_BOOLEAN (arg, int_val == GTH_ZOOM_QUALITY_HIGH);
		break;
	case ARG_ZOOM_CHANGE_ACTUAL_SIZE:
		int_val = image_viewer_get_zoom_change (viewer);
		BONOBO_ARG_SET_BOOLEAN (arg, int_val == GTH_ZOOM_CHANGE_ACTUAL_SIZE);
		break;
	case ARG_ZOOM_CHANGE_FIT:
		int_val = image_viewer_get_zoom_change (viewer);
		BONOBO_ARG_SET_BOOLEAN (arg, int_val == GTH_ZOOM_CHANGE_FIT);
		break;
	case ARG_ZOOM_CHANGE_KEEP_PREV:
		int_val = image_viewer_get_zoom_change (viewer);
		BONOBO_ARG_SET_BOOLEAN (arg, int_val == GTH_ZOOM_CHANGE_KEEP_PREV);
		break;
	case ARG_ZOOM_FIT:
		bool_val = image_viewer_is_zoom_to_fit (viewer);
		BONOBO_ARG_SET_BOOLEAN (arg, bool_val);
		break;

	case ARG_TRANSP_TYPE_WHITE:
	case ARG_TRANSP_TYPE_NONE:
	case ARG_TRANSP_TYPE_BLACK:
	case ARG_TRANSP_TYPE_CHECKED:
		int_val = image_viewer_get_transp_type (viewer);
		transp_type = get_transp_type_from_arg (arg_id);
		BONOBO_ARG_SET_BOOLEAN (arg, int_val == transp_type);
		break;

	case ARG_CHECK_TYPE_LIGHT:
	case ARG_CHECK_TYPE_MIDTONE:
	case ARG_CHECK_TYPE_DARK:
		int_val = image_viewer_get_check_type (viewer);
		check_type = get_check_type_from_arg (arg_id);
		BONOBO_ARG_SET_BOOLEAN (arg, int_val == check_type);
		break;

	case ARG_CHECK_SIZE_SMALL:
	case ARG_CHECK_SIZE_MEDIUM:
	case ARG_CHECK_SIZE_LARGE:
		int_val = image_viewer_get_check_size (viewer);
		check_size = get_check_size_from_arg (arg_id);
		BONOBO_ARG_SET_BOOLEAN (arg, int_val == check_size);
		break;

	case ARG_CURSOR_VISIBLE:
		bool_val = image_viewer_is_cursor_visible (viewer);
		BONOBO_ARG_SET_BOOLEAN (arg, bool_val);
		break;
	case ARG_BLACK_BACKGROUND:
		bool_val = image_viewer_is_black_background (viewer);
		BONOBO_ARG_SET_BOOLEAN (arg, bool_val);
		break;

	default:
		break;
	}
}


static void
set_prop (BonoboPropertyBag *bag, 
	  const BonoboArg   *arg, 
	  guint              arg_id, 
          CORBA_Environment *ev, 
	  gpointer           data)
{
	ViewerControl *control = data;
	ImageViewer   *viewer = control->priv->viewer;
	gboolean       bool_val;
	double         double_val;
	GthTranspType  transp_type;
	GthCheckType   check_type;
	GthCheckSize   check_size;

        switch (arg_id) {
        case ARG_IS_VOID:
		image_viewer_set_void (viewer);
		break;

	case ARG_RUNNING_ANIMATION:
		bool_val = BONOBO_ARG_GET_BOOLEAN (arg);
		if (bool_val)
			image_viewer_start_animation (viewer);
		else
			image_viewer_stop_animation (viewer);
		break;

	case ARG_ZOOM_VALUE:
		double_val = BONOBO_ARG_GET_DOUBLE (arg);
		image_viewer_set_zoom (viewer, double_val);
		break;
	case ARG_ZOOM_QUALITY_HIGH:
		bool_val = BONOBO_ARG_GET_BOOLEAN (arg);
		image_viewer_set_zoom_quality (viewer, bool_val ? GTH_ZOOM_QUALITY_HIGH : GTH_ZOOM_QUALITY_LOW);
		image_viewer_update_view (viewer);
		break;
	case ARG_ZOOM_CHANGE_ACTUAL_SIZE:
		image_viewer_set_zoom_change (viewer, GTH_ZOOM_CHANGE_ACTUAL_SIZE);
		break;
	case ARG_ZOOM_CHANGE_FIT:
		image_viewer_set_zoom_change (viewer, GTH_ZOOM_CHANGE_FIT);
		break;
	case ARG_ZOOM_CHANGE_KEEP_PREV:
		image_viewer_set_zoom_change (viewer, GTH_ZOOM_CHANGE_KEEP_PREV);
		break;
	case ARG_ZOOM_FIT:
		bool_val = BONOBO_ARG_GET_BOOLEAN (arg);
		if (bool_val) 
			image_viewer_zoom_to_fit (viewer);
		else
			image_viewer_set_zoom (viewer, 
					       image_viewer_get_zoom (viewer));
		break;

	case ARG_TRANSP_TYPE_WHITE:
	case ARG_TRANSP_TYPE_NONE:
	case ARG_TRANSP_TYPE_BLACK:
	case ARG_TRANSP_TYPE_CHECKED:
		transp_type = get_transp_type_from_arg (arg_id);
		image_viewer_set_transp_type (viewer, transp_type);
		image_viewer_update_view (viewer);
		break;
	case ARG_CHECK_TYPE_LIGHT:
	case ARG_CHECK_TYPE_MIDTONE:
	case ARG_CHECK_TYPE_DARK:
		check_type = get_check_type_from_arg (arg_id);
		image_viewer_set_check_type (viewer, check_type);
		image_viewer_set_transp_type (viewer, image_viewer_get_transp_type (viewer));
		image_viewer_update_view (viewer);
		break;
	case ARG_CHECK_SIZE_SMALL:
	case ARG_CHECK_SIZE_MEDIUM:
	case ARG_CHECK_SIZE_LARGE:
		check_size = get_check_size_from_arg (arg_id);
		image_viewer_set_check_size (viewer, check_size);
		image_viewer_update_view (viewer);
		break;

	case ARG_CURSOR_VISIBLE:
		bool_val = BONOBO_ARG_GET_BOOLEAN (arg);
		if (bool_val)
			image_viewer_show_cursor (viewer);
		else
			image_viewer_hide_cursor (viewer);
		break;
	case ARG_BLACK_BACKGROUND:
		bool_val = BONOBO_ARG_GET_BOOLEAN (arg);
		image_viewer_set_black_background (viewer, bool_val);
		break;

	default:
		break;
	}
}


static float zoom_levels[] = {
        1.0 / 10.0,  1.0 / 8.0, 1.0 / 6.0, 1.0 / 5.0, 
        1.0 / 4.0, 1.0 / 3.0, 1.0 / 2.0, 2.0 / 3.0, 1.0, 1.5, 2.0,
        3.0, 4.0, 5.0, 6.0, 8.0, 10.0
};

static const gchar *zoom_level_names[] = {
        "1:10", "1:8", "1:6", "1:5", "1:4", "1:3",
        "1:2", "2:3",  "1:1", "3:2", "2:1", "3:1", "4:1", "5:1", "6:1",
        "8:1", "10:1"
};

static const gint n_zoom_levels = sizeof (zoom_levels) / sizeof (float);


static void
zoom_changed_cb (GtkWidget     *widget,
		 ViewerControl *control)
{
	float new_zoom;	

	if (! control->priv->zoomable)
		return;

	new_zoom = image_viewer_get_zoom (control->priv->viewer);
        bonobo_zoomable_report_zoom_level_changed (control->priv->zoomable, 
						   new_zoom,
						   NULL);
}


static void
size_changed_cb (GtkWidget     *widget, 
		 ViewerControl *control)
{
	GtkAdjustment *vadj, *hadj;
	gboolean       hide_vscr, hide_hscr;

	vadj = IMAGE_VIEWER (control->priv->viewer)->vadj;
	hadj = IMAGE_VIEWER (control->priv->viewer)->hadj;

	hide_vscr = vadj->upper <= vadj->page_size;
	hide_hscr = hadj->upper <= hadj->page_size;

	if (hide_vscr && hide_hscr) {
		if (! GTK_WIDGET_VISIBLE (control->priv->viewer_nav_btn))
			return;

		gtk_widget_hide (control->priv->viewer_vscr); 
		gtk_widget_hide (control->priv->viewer_hscr); 
		gtk_widget_hide (control->priv->viewer_nav_btn);
		gtk_container_check_resize (GTK_CONTAINER (control->priv->viewer_table));
	} else {
		if (GTK_WIDGET_VISIBLE (control->priv->viewer_nav_btn))
			return;

		gtk_widget_show (control->priv->viewer_nav_btn);
		gtk_widget_show (control->priv->viewer_vscr); 
		gtk_widget_show (control->priv->viewer_hscr); 
		gtk_container_check_resize (GTK_CONTAINER (control->priv->viewer_table));
	}
}


static void
zoomable_zoom_in_cb (BonoboZoomable *zoomable, 
		     ViewerControl  *control)
{
	image_viewer_zoom_in (control->priv->viewer);
}


static void
zoomable_zoom_out_cb (BonoboZoomable *zoomable, 
		      ViewerControl  *control)
{
	image_viewer_zoom_out (control->priv->viewer);
}


static void
zoomable_zoom_to_fit_cb (BonoboZoomable *zoomable, 
			 ViewerControl  *control)
{
	image_viewer_zoom_to_fit (control->priv->viewer);
}


static void
zoomable_zoom_to_default_cb (BonoboZoomable *zoomable, 
			     ViewerControl  *control)
{
	image_viewer_set_zoom (control->priv->viewer, 1.0);
}


static void
zoomable_set_zoom_level_cb (BonoboZoomable *zoomable, 
			    float           new_zoom_level,
			    ViewerControl  *control)
{
	image_viewer_set_zoom (control->priv->viewer, new_zoom_level);
}


static GtkWidget *
create_widgets (ViewerControl *control,
		ImageViewer   *viewer)
{
	GtkWidget *hbox;
	GtkWidget *table;
	GtkWidget *hscr;
	GtkWidget *vscr;
	GtkWidget *event_box;
	GtkWidget *pixmap;

	/* set viewer signals. */

	g_signal_connect (G_OBJECT (viewer), 
			  "zoom_changed",
			  G_CALLBACK (zoom_changed_cb), 
			  control);

	/* set viewer preferences. */

	image_viewer_set_zoom_quality (viewer, pref_get_zoom_quality ());
	image_viewer_set_zoom_change (viewer, pref_get_zoom_change ());
	image_viewer_set_check_type (viewer, pref_get_check_type ());
	image_viewer_set_check_size (viewer, pref_get_check_size ());
	image_viewer_set_transp_type (viewer, pref_get_transp_type ());

	/**/

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (hbox), GTK_WIDGET (viewer));

	control->priv->viewer_table = table = gtk_table_new (2, 2, FALSE);
	control->priv->viewer_vscr = vscr = gtk_vscrollbar_new (viewer->vadj);
	control->priv->viewer_hscr = hscr = gtk_hscrollbar_new (viewer->hadj);

	control->priv->viewer_nav_btn = event_box = gtk_event_box_new ();
	pixmap = _gtk_image_new_from_xpm_data (nav_button_xpm);
	gtk_container_add (GTK_CONTAINER (event_box), pixmap);

	gtk_table_attach (GTK_TABLE (table), hbox, 0, 1, 0, 1,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
	gtk_table_attach (GTK_TABLE (table), vscr, 1, 2, 0, 1,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
	gtk_table_attach (GTK_TABLE (table), hscr, 0, 1, 1, 2,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL), 0, 0);
	gtk_table_attach (GTK_TABLE (table), event_box, 1, 2, 1, 2,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL), 0, 0);

        /* signals */

	g_signal_connect (G_OBJECT (event_box), 
			  "button_press_event",
			  G_CALLBACK (nav_button_clicked_cb), 
			  viewer);
	g_signal_connect (G_OBJECT (viewer), 
			  "size_changed",
			  G_CALLBACK (size_changed_cb), 
			  control);

	gtk_widget_show_all (table);

	return table;
}


static void
construct_control (ViewerControl *control,
		   ImageViewer   *viewer)
{
	BonoboObject       *stream;
	BonoboPropertyBag  *pbag;
	BonoboZoomable     *zoomable;
	GtkWidget          *root_widget;
	
	g_return_if_fail (control != NULL);
	g_return_if_fail (viewer != NULL);

	/* create the control. */

	control->priv->viewer = viewer;
	root_widget = create_widgets (control, viewer);
	bonobo_control_construct (BONOBO_CONTROL (control), root_widget);

	/* add the Bonobo::PersistStream interface. */

	stream = viewer_stream_new (control);
	bonobo_object_add_interface (BONOBO_OBJECT (control), stream);

	/* add the Bonobo::PropertyBag interface. */

	pbag = bonobo_property_bag_new (get_prop, set_prop, control);
        bonobo_control_set_properties (BONOBO_CONTROL (control), 
				       BONOBO_OBJREF (pbag), 
				       NULL);

	/* add the Nautilus::View interface. */

	control->priv->nautilus_view = viewer_nautilus_view_new (control);
	bonobo_object_add_interface (BONOBO_OBJECT (control), control->priv->nautilus_view);

	/* - viewer content */

        bonobo_property_bag_add (pbag, 
				 "is_void", 
				 ARG_IS_VOID, 
				 BONOBO_ARG_BOOLEAN, 
				 NULL, 
				 "The content of the viewer is void",
				 (BONOBO_PROPERTY_READABLE 
				  | BONOBO_PROPERTY_WRITEABLE));

	/* - image info. */

        bonobo_property_bag_add (pbag, 
				 "width",
				 ARG_IMAGE_WIDTH, 
				 BONOBO_ARG_INT, 
				 NULL, 
				 "The width of the displayed image", 
				 BONOBO_PROPERTY_READABLE);
        bonobo_property_bag_add (pbag, 
				 "height",
				 ARG_IMAGE_HEIGHT, 
				 BONOBO_ARG_INT, 
				 NULL, 
                                 "The height of the displayed image", 
				 BONOBO_PROPERTY_READABLE);
        bonobo_property_bag_add (pbag, 
				 "bits_per_sample",
				 ARG_IMAGE_BPS, 
				 BONOBO_ARG_INT, 
				 NULL, 
                                 "Bits per sample of the displayed image", 
				 BONOBO_PROPERTY_READABLE);
        bonobo_property_bag_add (pbag, 
				 "has_transparency",
				 ARG_HAS_TRANSPARENCY, 
				 BONOBO_ARG_BOOLEAN, 
				 NULL, 
                                 "The image has a transparency", 
				 BONOBO_PROPERTY_READABLE);

	/* - animation. */

        bonobo_property_bag_add (pbag, 
				 "running_animation",
				 ARG_RUNNING_ANIMATION, 
				 BONOBO_ARG_BOOLEAN, 
				 NULL, 
				 "Start or stop the animation", 
				 (BONOBO_PROPERTY_READABLE
				  | BONOBO_PROPERTY_WRITEABLE));

        bonobo_property_bag_add (pbag, 
				 "is_animation",
				 ARG_IS_ANIMATION,
				 BONOBO_ARG_BOOLEAN, 
				 NULL, 
                                 "Whether the image is an animation",
				 BONOBO_PROPERTY_READABLE);

	/* - zoom. */

        bonobo_property_bag_add (pbag, 
				 "zoom_value",
				 ARG_ZOOM_VALUE,
				 BONOBO_ARG_DOUBLE, 
				 NULL, 
				 "The zoom value", 
				 (BONOBO_PROPERTY_READABLE 
				  | BONOBO_PROPERTY_WRITEABLE));

        bonobo_property_bag_add (pbag, 
				 "zoom_quality_high",
				 ARG_ZOOM_QUALITY_HIGH,
				 BONOBO_ARG_BOOLEAN, 
				 NULL, 
				 "Whether to use high quality zoom", 
				 (BONOBO_PROPERTY_READABLE 
				  | BONOBO_PROPERTY_WRITEABLE));

        bonobo_property_bag_add (pbag, 
				 "zoom_change_actual_size",
				 ARG_ZOOM_CHANGE_ACTUAL_SIZE,
				 BONOBO_ARG_BOOLEAN, 
				 NULL, 
				 "After loading an image set it to actual"
				 "size",
				 (BONOBO_PROPERTY_READABLE 
				  | BONOBO_PROPERTY_WRITEABLE));
        bonobo_property_bag_add (pbag, 
				 "zoom_change_fit",
				 ARG_ZOOM_CHANGE_FIT,
				 BONOBO_ARG_BOOLEAN, 
				 NULL, 
				 "After loading an image fit image to window",
				 (BONOBO_PROPERTY_READABLE 
				  | BONOBO_PROPERTY_WRITEABLE));
        bonobo_property_bag_add (pbag, 
				 "zoom_change_keep_prev",
				 ARG_ZOOM_CHANGE_KEEP_PREV,
				 BONOBO_ARG_BOOLEAN, 
				 NULL, 
				 "After loading an image keep previous"
				 "zoom level",
				 (BONOBO_PROPERTY_READABLE 
				  | BONOBO_PROPERTY_WRITEABLE));

        bonobo_property_bag_add (pbag, 
				 "zoom_fit",
				 ARG_ZOOM_FIT,
				 BONOBO_ARG_BOOLEAN, 
				 NULL, 
				 "Scale the image to fit the window", 
				 (BONOBO_PROPERTY_READABLE 
				  | BONOBO_PROPERTY_WRITEABLE));
	
	/* - visualization options. */

        bonobo_property_bag_add (pbag, 
				 "transp_type_white",
				 ARG_TRANSP_TYPE_WHITE,
				 BONOBO_ARG_BOOLEAN, 
				 NULL, 
				 "Use white color as transparency "
				 "indicator",
				 (BONOBO_PROPERTY_READABLE 
				  | BONOBO_PROPERTY_WRITEABLE));

        bonobo_property_bag_add (pbag, 
				 "transp_type_none",
				 ARG_TRANSP_TYPE_NONE,
				 BONOBO_ARG_BOOLEAN, 
				 NULL, 
				 "Use gtk theme background as transparency "
				 "indicator",
				 (BONOBO_PROPERTY_READABLE 
				  | BONOBO_PROPERTY_WRITEABLE));

        bonobo_property_bag_add (pbag, 
				 "transp_type_black",
				 ARG_TRANSP_TYPE_BLACK,
				 BONOBO_ARG_BOOLEAN, 
				 NULL, 
				 "Use black color as transparency "
				 "indicator",
				 (BONOBO_PROPERTY_READABLE 
				  | BONOBO_PROPERTY_WRITEABLE));

        bonobo_property_bag_add (pbag, 
				 "transp_type_checked",
				 ARG_TRANSP_TYPE_CHECKED,
				 BONOBO_ARG_BOOLEAN, 
				 NULL, 
				 "Use checks as transparency indicator",
				 (BONOBO_PROPERTY_READABLE 
				  | BONOBO_PROPERTY_WRITEABLE));

        bonobo_property_bag_add (pbag, 
				 "check_type_light",
				 ARG_CHECK_TYPE_LIGHT,
				 BONOBO_ARG_BOOLEAN, 
				 NULL, 
				 "Use light checks",
				 (BONOBO_PROPERTY_READABLE 
				  | BONOBO_PROPERTY_WRITEABLE));

        bonobo_property_bag_add (pbag, 
				 "check_type_midtone",
				 ARG_CHECK_TYPE_MIDTONE,
				 BONOBO_ARG_BOOLEAN, 
				 NULL, 
				 "Use middle tone checks",
				 (BONOBO_PROPERTY_READABLE 
				  | BONOBO_PROPERTY_WRITEABLE));

        bonobo_property_bag_add (pbag, 
				 "check_type_dark",
				 ARG_CHECK_TYPE_DARK,
				 BONOBO_ARG_BOOLEAN, 
				 NULL, 
				 "Use dark checks",
				 (BONOBO_PROPERTY_READABLE 
				  | BONOBO_PROPERTY_WRITEABLE));

        bonobo_property_bag_add (pbag, 
				 "check_size_small",
				 ARG_CHECK_SIZE_SMALL,
				 BONOBO_ARG_BOOLEAN, 
				 NULL, 
				 "Use small checks",
				 (BONOBO_PROPERTY_READABLE 
				  | BONOBO_PROPERTY_WRITEABLE));

        bonobo_property_bag_add (pbag, 
				 "check_size_medium",
				 ARG_CHECK_SIZE_MEDIUM,
				 BONOBO_ARG_BOOLEAN, 
				 NULL, 
				 "Use medium size checks",
				 (BONOBO_PROPERTY_READABLE 
				  | BONOBO_PROPERTY_WRITEABLE));

        bonobo_property_bag_add (pbag, 
				 "check_size_large",
				 ARG_CHECK_SIZE_LARGE,
				 BONOBO_ARG_BOOLEAN, 
				 NULL, 
				 "Use large checks",
				 (BONOBO_PROPERTY_READABLE 
				  | BONOBO_PROPERTY_WRITEABLE));

	/* - cursor */

        bonobo_property_bag_add (pbag, 
				 "cursor_visible",
				 ARG_CURSOR_VISIBLE,
				 BONOBO_ARG_BOOLEAN, 
				 NULL,
				 "Show the cursor", 
				 (BONOBO_PROPERTY_READABLE 
				  | BONOBO_PROPERTY_WRITEABLE));

	/* - misc */

        bonobo_property_bag_add (pbag, 
				 "black_background",
				 ARG_BLACK_BACKGROUND,
				 BONOBO_ARG_BOOLEAN, 
				 NULL, 
				 "Use a black background",
				 (BONOBO_PROPERTY_READABLE 
				  | BONOBO_PROPERTY_WRITEABLE));

	bonobo_object_add_interface (BONOBO_OBJECT (control), 
                                     BONOBO_OBJECT (pbag));

	/* add the Bonobo::Zoomable interface. */
	
	zoomable = bonobo_zoomable_new ();
	control->priv->zoomable = zoomable;

	g_signal_connect (G_OBJECT (zoomable), 
			  "set_zoom_level",
			  G_CALLBACK (zoomable_set_zoom_level_cb),
			  control);
	g_signal_connect (G_OBJECT (zoomable), 
			  "zoom_in",
			  G_CALLBACK (zoomable_zoom_in_cb), 
			  control);
	g_signal_connect (G_OBJECT (zoomable), 
			  "zoom_out",
			  G_CALLBACK (zoomable_zoom_out_cb), 
			  control);
	g_signal_connect (G_OBJECT (zoomable), 
			  "zoom_to_fit",
			  G_CALLBACK (zoomable_zoom_to_fit_cb), 
			  control);
	g_signal_connect (G_OBJECT (zoomable), 
			  "zoom_to_default",
			  G_CALLBACK (zoomable_zoom_to_default_cb), 
			  control);

        bonobo_zoomable_set_parameters_full (zoomable,
                                             1.0,
                                             zoom_levels [0],
                                             zoom_levels [n_zoom_levels - 1],
                                             TRUE, 
					     TRUE, 
					     TRUE,
                                             zoom_levels,
                                             zoom_level_names,
                                             n_zoom_levels);
        bonobo_object_add_interface (BONOBO_OBJECT (control),
                                     BONOBO_OBJECT (zoomable));
}


BonoboControl *
viewer_control_new (ImageViewer *viewer)
{
	BonoboControl *control;
	
	g_return_val_if_fail (viewer != NULL, NULL);

	control = g_object_new (VIEWER_TYPE_CONTROL, NULL);
	construct_control (VIEWER_CONTROL (control), viewer);

	return control;
}
