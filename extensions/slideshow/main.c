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


#define OPACITY_AT_MSECS(t)((int) (255 * ((double) (t) / GTH_TRANSITION_DURATION)))
#define ANGLE_AT_MSECS(t)((180.0 * ((double) (t) / GTH_TRANSITION_DURATION)))
#define POSITION_AT_MSECS(x, t)(((float)(x) * ((double) (t) / GTH_TRANSITION_DURATION)))


static void
no_transition (GthSlideshow *self,
	       int           msecs)
{
	if (self->first_frame) {
		if (self->current_texture != NULL)
			clutter_actor_hide (self->current_texture);
		clutter_actor_show (self->next_texture);
	}
}


static void
slide_in_transition (GthSlideshow *self,
		     int           msecs)
{
	float stage_w, stage_h;

	clutter_actor_get_size (self->stage, &stage_w, &stage_h);

	clutter_actor_set_x (self->next_texture, POSITION_AT_MSECS (stage_w, GTH_TRANSITION_DURATION - msecs) + self->next_geometry.x);
	if (self->current_texture != NULL)
		clutter_actor_set_x (self->current_texture, POSITION_AT_MSECS (- stage_w, msecs) + self->current_geometry.x);

	if (self->first_frame) {
		if (self->current_texture != NULL)
			clutter_actor_show (self->current_texture);
		clutter_actor_show (self->next_texture);
	}
}


static void
fade_in_transition (GthSlideshow *self,
		    int           msecs)
{
	if (self->current_texture != NULL)
		clutter_actor_set_opacity (self->current_texture, OPACITY_AT_MSECS (GTH_TRANSITION_DURATION - msecs));
	clutter_actor_set_opacity (self->next_texture, OPACITY_AT_MSECS(msecs));

	if (self->first_frame) {
		if (self->current_texture != NULL) {
			clutter_actor_show (self->current_texture);
			clutter_actor_raise (self->next_texture, self->current_texture);
		}
		clutter_actor_show (self->next_texture);
	}
}


static void
flip_transition (GthSlideshow *self,
		 int           msecs)
{
	float stage_w, stage_h;

	if ((float) msecs >= (float) GTH_TRANSITION_DURATION / 2.0) {
		clutter_actor_show (self->next_texture);
		if (self->current_texture != NULL)
			clutter_actor_hide (self->current_texture);
	}
	else {
		clutter_actor_hide (self->next_texture);
		if (self->current_texture != NULL)
			clutter_actor_show (self->current_texture);
	}

	clutter_actor_get_size (self->stage, &stage_w, &stage_h);
	clutter_actor_set_rotation (self->next_texture,
				    CLUTTER_Y_AXIS,
		 		    ANGLE_AT_MSECS (GTH_TRANSITION_DURATION - msecs),
		 		    stage_w / 2.0,
		 		    0.0,
		 		    0.0);
	if (self->current_texture != NULL)
		clutter_actor_set_rotation (self->current_texture,
					    CLUTTER_Y_AXIS,
					    ANGLE_AT_MSECS (- msecs),
					    stage_w / 2.0,
					    0.0,
					    0.0);

	if (self->first_frame) {
		if (self->current_texture != NULL)
			clutter_actor_raise (self->next_texture, self->current_texture);
		clutter_actor_show (self->next_texture);
		self->first_frame = FALSE;
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
				  "slide-in",
				  GTH_TYPE_TRANSITION,
				  "display-name", _("Slide In"),
				  "frame-func", slide_in_transition,
				  NULL);
	gth_main_register_object (GTH_TYPE_TRANSITION,
				  "fade-in",
				  GTH_TYPE_TRANSITION,
				  "display-name", _("Fade In"),
				  "frame-func", fade_in_transition,
				  NULL);
	gth_main_register_object (GTH_TYPE_TRANSITION,
				  "flip",
				  GTH_TYPE_TRANSITION,
				  "display-name", _("Flip Page"),
				  "frame-func", flip_transition,
				  NULL);

	gth_hook_add_callback ("gth-browser-construct", 10, G_CALLBACK (ss__gth_browser_construct_cb), NULL);
	gth_hook_add_callback ("gth-browser-update-sensitivity", 10, G_CALLBACK (ss__gth_browser_update_sensitivity_cb), NULL);
	gth_hook_add_callback ("dlg-preferences-construct", 20, G_CALLBACK (ss__dlg_preferences_construct_cb), NULL);
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
