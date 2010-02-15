/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2008 Free Software Foundation, Inc.
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
#include <gthumb.h>
#include "callbacks.h"
#include "gth-transition.h"
#include "preferences.h"


#define VALUE_AT_MSECS(v, t)(((double) (v) * ((double) (t) / GTH_TRANSITION_DURATION)))


static void
no_transition (GthSlideshow *self,
	       int           msecs)
{
	if (self->first_frame) {
		if (self->current_image != NULL)
			clutter_actor_hide (self->current_image);
		clutter_actor_show (self->next_image);
	}
}


static void
push_from_right_transition (GthSlideshow *self,
			    int           msecs)
{
	float stage_w, stage_h;

	clutter_actor_get_size (self->stage, &stage_w, &stage_h);

	clutter_actor_set_x (self->next_image, (float) VALUE_AT_MSECS (stage_w, GTH_TRANSITION_DURATION - msecs) + self->next_geometry.x);
	if (self->current_image != NULL)
		clutter_actor_set_x (self->current_image, (float) VALUE_AT_MSECS (- stage_w, msecs) + self->current_geometry.x);

	if (self->first_frame) {
		if (self->current_image != NULL)
			clutter_actor_show (self->current_image);
		clutter_actor_show (self->next_image);
	}
}


static void
push_from_bottom_transition (GthSlideshow *self,
			     int           msecs)
{
	float stage_w, stage_h;

	clutter_actor_get_size (self->stage, &stage_w, &stage_h);

	clutter_actor_set_y (self->next_image, (float) VALUE_AT_MSECS (stage_h, GTH_TRANSITION_DURATION - msecs) + self->next_geometry.y);
	if (self->current_image != NULL)
		clutter_actor_set_y (self->current_image, (float) VALUE_AT_MSECS (- stage_h, msecs) + self->current_geometry.y);

	if (self->first_frame) {
		if (self->current_image != NULL)
			clutter_actor_show (self->current_image);
		clutter_actor_show (self->next_image);
	}
}


static void
slide_from_right_transition (GthSlideshow *self,
			     int           msecs)
{
	float stage_w, stage_h;

	clutter_actor_get_size (self->stage, &stage_w, &stage_h);
	clutter_actor_set_x (self->next_image, (float) VALUE_AT_MSECS (stage_w, GTH_TRANSITION_DURATION - msecs) + self->next_geometry.x);

	if (self->first_frame) {
		if (self->current_image != NULL) {
			clutter_actor_show (self->current_image);
			clutter_actor_raise (self->next_image, self->current_image);
		}
		clutter_actor_show (self->next_image);
	}
}


static void
slide_from_bottom_transition (GthSlideshow *self,
			      int           msecs)
{
	float stage_w, stage_h;

	clutter_actor_get_size (self->stage, &stage_w, &stage_h);
	clutter_actor_set_y (self->next_image, (float) VALUE_AT_MSECS (stage_h, GTH_TRANSITION_DURATION - msecs) + self->next_geometry.y);

	if (self->first_frame) {
		if (self->current_image != NULL) {
			clutter_actor_show (self->current_image);
			clutter_actor_raise (self->next_image, self->current_image);
		}
		clutter_actor_show (self->next_image);
	}
}


static void
fade_transition (GthSlideshow *self,
		 int           msecs)
{
	if (self->current_image != NULL)
		clutter_actor_set_opacity (self->current_image, (int) VALUE_AT_MSECS (255.0, GTH_TRANSITION_DURATION - msecs));
	clutter_actor_set_opacity (self->next_image, (int) VALUE_AT_MSECS (255.0, msecs));

	if (self->first_frame) {
		if (self->current_image != NULL) {
			clutter_actor_show (self->current_image);
			clutter_actor_raise (self->next_image, self->current_image);
		}
		clutter_actor_show (self->next_image);
	}
}


static void
flip_transition (GthSlideshow *self,
		 int           msecs)
{
	if ((float) msecs >= (float) GTH_TRANSITION_DURATION / 2.0) {
		clutter_actor_show (self->next_image);
		if (self->current_image != NULL)
			clutter_actor_hide (self->current_image);
	}
	else {
		clutter_actor_hide (self->next_image);
		if (self->current_image != NULL)
			clutter_actor_show (self->current_image);
	}

	clutter_actor_set_rotation (self->next_image,
				    CLUTTER_Y_AXIS,
				    VALUE_AT_MSECS (180.0, GTH_TRANSITION_DURATION - msecs),
				    0.0,
				    0.0,
				    0.0);
	if (self->current_image != NULL)
		clutter_actor_set_rotation (self->current_image,
					    CLUTTER_Y_AXIS,
					    VALUE_AT_MSECS (180.0, - msecs),
					    0.0,
					    0.0,
					    0.0);

	if (self->first_frame) {
		if (self->current_image != NULL) {
			clutter_actor_raise (self->next_image, self->current_image);
			clutter_actor_move_anchor_point_from_gravity (self->current_image, CLUTTER_GRAVITY_CENTER);
		}
		clutter_actor_show (self->next_image);
		clutter_actor_move_anchor_point_from_gravity (self->next_image, CLUTTER_GRAVITY_CENTER);
	}
}


static void
cube_from_right_transition (GthSlideshow *self,
			    int           msecs)
{
	float stage_w, stage_h;

	clutter_actor_get_size (self->stage, &stage_w, &stage_h);

	if (self->current_image != NULL) {
		if (msecs >= GTH_TRANSITION_DURATION / 2)
			clutter_actor_raise (self->next_image, self->current_image);
		else
			clutter_actor_raise (self->current_image, self->next_image);
	}

	clutter_actor_set_rotation (self->next_image,
				    CLUTTER_Y_AXIS,
				    VALUE_AT_MSECS (90.0, -msecs) - 270.0,
				    0.0,
				    0.0,
				    - stage_w / 2.0);
	if (self->current_image != NULL)
		clutter_actor_set_rotation (self->current_image,
					    CLUTTER_Y_AXIS,
					    VALUE_AT_MSECS (90.0, -msecs),
					    0.0,
					    0.0,
					    - stage_w / 2.0);

	if (self->first_frame) {
		if (self->current_image != NULL)
			clutter_actor_move_anchor_point_from_gravity (self->current_image, CLUTTER_GRAVITY_CENTER);
		clutter_actor_show (self->next_image);
		clutter_actor_move_anchor_point_from_gravity (self->next_image, CLUTTER_GRAVITY_CENTER);
	}
}


static void
cube_from_bottom_transition (GthSlideshow *self,
			     int           msecs)
{
	float stage_w, stage_h;

	clutter_actor_get_size (self->stage, &stage_w, &stage_h);

	if (self->current_image != NULL) {
		if (msecs >= GTH_TRANSITION_DURATION / 2)
			clutter_actor_raise (self->next_image, self->current_image);
		else
			clutter_actor_raise (self->current_image, self->next_image);
	}

	clutter_actor_set_rotation (self->next_image,
				    CLUTTER_X_AXIS,
				    VALUE_AT_MSECS (90.0, msecs) + 270.0,
				    0.0,
				    0.0,
				    - stage_w / 2.0);
	if (self->current_image != NULL)
		clutter_actor_set_rotation (self->current_image,
					    CLUTTER_X_AXIS,
					    VALUE_AT_MSECS (90.0, msecs),
					    0.0,
					    0.0,
					    - stage_w / 2.0);

	if (self->first_frame) {
		if (self->current_image != NULL)
			clutter_actor_move_anchor_point_from_gravity (self->current_image, CLUTTER_GRAVITY_CENTER);
		clutter_actor_show (self->next_image);
		clutter_actor_move_anchor_point_from_gravity (self->next_image, CLUTTER_GRAVITY_CENTER);
	}
}


G_MODULE_EXPORT void
gthumb_extension_activate (void)
{
	gth_main_register_object (GTH_TYPE_TRANSITION,
				  "none",
				  GTH_TYPE_TRANSITION,
				  "display-name", _("None"),
				  "frame-func", no_transition,
				  NULL);
	gth_main_register_object (GTH_TYPE_TRANSITION,
				  "push-from-right",
				  GTH_TYPE_TRANSITION,
				  "display-name", _("Push from right"),
				  "frame-func", push_from_right_transition,
				  NULL);
	gth_main_register_object (GTH_TYPE_TRANSITION,
				  "push-from-bottom",
				  GTH_TYPE_TRANSITION,
				  "display-name", _("Push from bottom"),
				  "frame-func", push_from_bottom_transition,
				  NULL);
	gth_main_register_object (GTH_TYPE_TRANSITION,
				  "slide-from-right",
				  GTH_TYPE_TRANSITION,
				  "display-name", _("Slide from right"),
				  "frame-func", slide_from_right_transition,
				  NULL);
	gth_main_register_object (GTH_TYPE_TRANSITION,
				  "slide-from-bottom",
				  GTH_TYPE_TRANSITION,
				  "display-name", _("Slide from bottom"),
				  "frame-func", slide_from_bottom_transition,
				  NULL);
	gth_main_register_object (GTH_TYPE_TRANSITION,
				  "fade",
				  GTH_TYPE_TRANSITION,
				  "display-name", _("Fade in"),
				  "frame-func", fade_transition,
				  NULL);
	gth_main_register_object (GTH_TYPE_TRANSITION,
				  "flip",
				  GTH_TYPE_TRANSITION,
				  "display-name", _("Flip page"),
				  "frame-func", flip_transition,
				  NULL);
	gth_main_register_object (GTH_TYPE_TRANSITION,
				  "cube-from-right",
				  GTH_TYPE_TRANSITION,
				  "display-name", _("Cube from right"),
				  "frame-func", cube_from_right_transition,
				  NULL);
	gth_main_register_object (GTH_TYPE_TRANSITION,
				  "cube-from-bottom",
				  GTH_TYPE_TRANSITION,
				  "display-name", _("Cube from bottom"),
				  "frame-func", cube_from_bottom_transition,
				  NULL);

	gth_hook_add_callback ("slideshow", 10, G_CALLBACK (ss__slideshow_cb), NULL);
	gth_hook_add_callback ("gth-browser-construct", 10, G_CALLBACK (ss__gth_browser_construct_cb), NULL);
	gth_hook_add_callback ("gth-browser-update-sensitivity", 10, G_CALLBACK (ss__gth_browser_update_sensitivity_cb), NULL);
	gth_hook_add_callback ("dlg-preferences-construct", 20, G_CALLBACK (ss__dlg_preferences_construct_cb), NULL);
	gth_hook_add_callback ("gth-catalog-read-metadata", 10, G_CALLBACK (ss__gth_catalog_read_metadata), NULL);
	gth_hook_add_callback ("gth-catalog-write-metadata", 10, G_CALLBACK (ss__gth_catalog_write_metadata), NULL);
	gth_hook_add_callback ("gth-catalog-read-from-doc", 10, G_CALLBACK (ss__gth_catalog_read_from_doc), NULL);
	gth_hook_add_callback ("gth-catalog-write-to-doc", 10, G_CALLBACK (ss__gth_catalog_write_to_doc), NULL);
	gth_hook_add_callback ("dlg-catalog-properties", 10, G_CALLBACK (ss__dlg_catalog_properties), NULL);
	gth_hook_add_callback ("dlg-catalog-properties-save", 10, G_CALLBACK (ss__dlg_catalog_properties_save), NULL);
}


G_MODULE_EXPORT void
gthumb_extension_deactivate (void)
{
}


G_MODULE_EXPORT gboolean
gthumb_extension_is_configurable (void)
{
	return FALSE;
}


G_MODULE_EXPORT void
gthumb_extension_configure (GtkWindow *parent)
{
}
