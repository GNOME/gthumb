/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2009 Free Software Foundation, Inc.
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
#include <clutter/clutter.h>
#include <clutter-gtk/clutter-gtk.h>
#include "gth-slideshow.h"
#include "gth-transition.h"

#define HIDE_CURSOR_DELAY 1000
#define DEFAULT_DELAY 2000


struct _GthSlideshowPrivate {
	GthBrowser      *browser;
	GList           *file_list; /* GthFileData */
	gboolean         automatic;
	gboolean         loop;
	GList           *current;
	GthImageLoader  *image_loader;
	GList           *transitions; /* GthTransition */
	int              n_transitions;
	GthTransition   *transition;
	ClutterTimeline *timeline;
	ClutterActor    *texture1;
	ClutterActor    *texture2;
	guint            next_event;
	guint            delay;
	guint            hide_cursor_event;
	GRand           *rand;
	gboolean         first_show;
	gboolean         one_loaded;
};


static gpointer parent_class = NULL;


static GthTransition *
_gth_slideshow_get_transition (GthSlideshow *self)
{
	if (self->priv->transitions == NULL)
		return NULL;
	else if (self->priv->transitions->next == NULL)
		return self->priv->transitions->data;
	else
		return g_list_nth_data (self->priv->transitions,
					g_rand_int_range (self->priv->rand, 0, self->priv->n_transitions));
}


static void
_gth_slideshow_load_current_image (GthSlideshow *self)
{
	if (self->priv->next_event != 0) {
		g_source_remove (self->priv->next_event);
		self->priv->next_event = 0;
	}

	if (self->priv->current == NULL) {
		if (! self->priv->one_loaded || ! self->priv->loop) {
			gtk_widget_destroy (GTK_WIDGET (self));
			return;
		}

		if (clutter_timeline_get_direction (self->priv->timeline) == CLUTTER_TIMELINE_FORWARD)
			self->priv->current = g_list_first (self->priv->file_list);
		else
			self->priv->current = g_list_last (self->priv->file_list);
	}

	self->priv->transition = _gth_slideshow_get_transition (self);
	gth_image_loader_set_file_data (GTH_IMAGE_LOADER (self->priv->image_loader), (GthFileData *) self->priv->current->data);
	gth_image_loader_load (GTH_IMAGE_LOADER (self->priv->image_loader));
}


static void
_gth_slideshow_swap_current_and_next (GthSlideshow *self)
{
	ClutterGeometry tmp_geometry;

	self->current_texture = self->next_texture;
	if (self->current_texture == self->priv->texture1)
		self->next_texture = self->priv->texture2;
	else
		self->next_texture = self->priv->texture1;

	tmp_geometry = self->current_geometry;
	self->current_geometry = self->next_geometry;
	self->next_geometry = tmp_geometry;
}


static void
_gth_slideshow_animation_completed (GthSlideshow *self)
{
	if (clutter_timeline_get_direction (self->priv->timeline) == CLUTTER_TIMELINE_FORWARD)
		_gth_slideshow_swap_current_and_next (self);
}


static void
_gth_slideshow_load_next_image (GthSlideshow *self)
{
	if (clutter_timeline_is_playing (self->priv->timeline)) {
		clutter_timeline_pause (self->priv->timeline);
		_gth_slideshow_animation_completed (self);
	}

	self->priv->current = self->priv->current->next;
	clutter_timeline_set_direction (self->priv->timeline, CLUTTER_TIMELINE_FORWARD);
	_gth_slideshow_load_current_image (self);
}


static void
_gth_slideshow_load_prev_image (GthSlideshow *self)
{
	if (clutter_timeline_is_playing (self->priv->timeline)) {
		clutter_timeline_pause (self->priv->timeline);
		_gth_slideshow_animation_completed (self);
	}

	self->priv->current = self->priv->current->prev;
	clutter_timeline_set_direction (self->priv->timeline, CLUTTER_TIMELINE_BACKWARD);
	_gth_slideshow_load_current_image (self);
}


static gboolean
next_image_cb (gpointer user_data)
{
	GthSlideshow *self = user_data;

	if (self->priv->next_event != 0) {
		g_source_remove (self->priv->next_event);
		self->priv->next_event = 0;
	}
	_gth_slideshow_load_next_image (self);

	return FALSE;
}


static void
animation_completed_cb (ClutterTimeline *timeline,
			GthSlideshow    *self)
{
	_gth_slideshow_animation_completed (self);

	if (self->priv->automatic) {
		if (self->priv->next_event != 0)
			g_source_remove (self->priv->next_event);
		self->priv->next_event = g_timeout_add (self->priv->delay, next_image_cb, self);
	}
}


static void
animation_frame_cb (ClutterTimeline *timeline,
		    int              msecs,
		    GthSlideshow    *self)
{
	if (self->priv->transition != NULL)
		gth_transition_frame (self->priv->transition, self, msecs);
	if (self->first_frame)
		self->first_frame = FALSE;
}


static void
animation_started_cb (ClutterTimeline *timeline,
		      GthSlideshow    *self)
{
	self->first_frame = TRUE;
}


static void
image_loader_ready_cb (GthImageLoader *image_loader,
		       GError         *error,
		       GthSlideshow   *self)
{
	GdkPixbuf *image;
	int        image_w, image_h;
	float      stage_w, stage_h;
	float      image_x, image_y;

	if (error != NULL) {
		g_clear_error (&error);
		_gth_slideshow_load_next_image (self);
		return;
	}

	self->priv->one_loaded = TRUE;

	clutter_actor_hide (self->next_texture);

	image = gth_image_loader_get_pixbuf (GTH_IMAGE_LOADER (image_loader));
	gtk_clutter_texture_set_from_pixbuf (CLUTTER_TEXTURE (self->next_texture), image, NULL);

	image_w = gdk_pixbuf_get_width (image);
	image_h = gdk_pixbuf_get_height (image);
	clutter_actor_get_size (self->stage, &stage_w, &stage_h);
	scale_keeping_ratio (&image_w, &image_h, (int) stage_w, (int) stage_h, TRUE);
	clutter_actor_set_size (self->next_texture, (float) image_w, (float) image_h);

	image_x = (stage_w - image_w) / 2;
	image_y = (stage_h - image_h) / 2;
	clutter_actor_set_position (self->next_texture, image_x, image_y);

	self->next_geometry.x = image_x;
	self->next_geometry.y = image_y;
	self->next_geometry.width = image_w;
	self->next_geometry.height = image_h;

	if (clutter_timeline_get_direction (self->priv->timeline) == CLUTTER_TIMELINE_BACKWARD)
		_gth_slideshow_swap_current_and_next (self);

	clutter_timeline_rewind (self->priv->timeline);
	clutter_timeline_start (self->priv->timeline);
	if (self->current_texture == NULL)
		clutter_timeline_advance (self->priv->timeline, GTH_TRANSITION_DURATION);
}


static void
gth_slideshow_init (GthSlideshow *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_SLIDESHOW, GthSlideshowPrivate);
	self->priv->file_list = NULL;
	self->priv->next_event = 0;
	self->priv->delay = DEFAULT_DELAY;
	self->priv->automatic = FALSE;
	self->priv->loop = FALSE;
	self->priv->transitions = NULL;
	self->priv->n_transitions = 0;
	self->priv->rand = g_rand_new ();
	self->priv->first_show = TRUE;

	self->priv->image_loader = gth_image_loader_new (FALSE);
	g_signal_connect (self->priv->image_loader, "ready", G_CALLBACK (image_loader_ready_cb), self);
}


static void
gth_slideshow_finalize (GObject *object)
{
	GthSlideshow *self = GTH_SLIDESHOW (object);

	if (self->priv->next_event != 0)
		g_source_remove (self->priv->next_event);
	if (self->priv->hide_cursor_event != 0)
		g_source_remove (self->priv->hide_cursor_event);

	_g_object_list_unref (self->priv->file_list);
	_g_object_unref (self->priv->browser);
	_g_object_unref (self->priv->image_loader);
	_g_object_unref (self->priv->timeline);
	_g_object_list_unref (self->priv->transitions);
	g_rand_free (self->priv->rand);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gth_slideshow_class_init (GthSlideshowClass *klass)
{
	GObjectClass *gobject_class;

	parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GthSlideshowPrivate));

	gobject_class = G_OBJECT_CLASS (klass);
	gobject_class->finalize = gth_slideshow_finalize;
}


GType
gth_slideshow_get_type (void)
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (GthSlideshowClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_slideshow_class_init,
			NULL,
			NULL,
			sizeof (GthSlideshow),
			0,
			(GInstanceInitFunc) gth_slideshow_init
		};

		type = g_type_register_static (GTK_TYPE_WINDOW,
					       "GthSlideshow",
					       &type_info,
					       0);
	}

	return type;
}


static gboolean
hide_cursor_cb (gpointer data)
{
	GthSlideshow *self = data;

	g_source_remove (self->priv->hide_cursor_event);
	self->priv->hide_cursor_event = 0;

	clutter_stage_hide_cursor (CLUTTER_STAGE (self->stage));

	return FALSE;
}


static void
stage_input_cb (ClutterStage *stage,
	        ClutterEvent *event,
	        GthSlideshow *self)
{
	if (event->type == CLUTTER_MOTION) {
		clutter_stage_show_cursor (CLUTTER_STAGE (self->stage));
		if (self->priv->hide_cursor_event != 0)
			g_source_remove (self->priv->hide_cursor_event);
		self->priv->hide_cursor_event = g_timeout_add (HIDE_CURSOR_DELAY, hide_cursor_cb, self);
	}
	else if (event->type == CLUTTER_KEY_RELEASE) {
		switch (clutter_event_get_key_symbol (event)) {
		case CLUTTER_Escape:
			gtk_widget_destroy (GTK_WIDGET (self));
			break;

		case CLUTTER_space:
			_gth_slideshow_load_next_image (self);
			break;

		case CLUTTER_BackSpace:
			_gth_slideshow_load_prev_image (self);
			break;
		}
	}
}


static void
gth_slideshow_show_cb (GtkWidget    *widget,
		       GthSlideshow *self)
{
	if (! self->priv->first_show)
		return;

	_gth_slideshow_load_current_image (self);
	self->priv->first_show = FALSE;
}


static void
_gth_slideshow_construct (GthSlideshow *self,
			  GthBrowser   *browser,
			  GList        *file_list)
{
	GtkWidget    *embed;
	ClutterColor  stage_color = { 0x0, 0x0, 0x0, 0xff };

	self->priv->browser = _g_object_ref (browser);
	self->priv->file_list = _g_object_list_ref (file_list);
	self->priv->current = self->priv->file_list;
	self->priv->one_loaded = FALSE;

	embed = gtk_clutter_embed_new ();
	self->stage = gtk_clutter_embed_get_stage (GTK_CLUTTER_EMBED (embed));
	clutter_stage_hide_cursor (CLUTTER_STAGE (self->stage));
	clutter_stage_set_color (CLUTTER_STAGE (self->stage), &stage_color);
	g_signal_connect (self->stage, "motion-event", G_CALLBACK (stage_input_cb), self);
	g_signal_connect (self->stage, "key-release-event", G_CALLBACK (stage_input_cb), self);

	gtk_widget_show (embed);
	gtk_container_add (GTK_CONTAINER (self), embed);

	self->priv->texture1 = clutter_texture_new ();
	clutter_actor_hide (self->priv->texture1);
	clutter_container_add_actor (CLUTTER_CONTAINER (self->stage), self->priv->texture1);

	self->priv->texture2 = clutter_texture_new ();
	clutter_actor_hide (self->priv->texture2);
	clutter_container_add_actor (CLUTTER_CONTAINER (self->stage), self->priv->texture2);

	self->current_texture = NULL;
	self->next_texture = self->priv->texture1;

	self->priv->timeline = clutter_timeline_new (GTH_TRANSITION_DURATION);
	g_signal_connect (self->priv->timeline, "completed", G_CALLBACK (animation_completed_cb), self);
	g_signal_connect (self->priv->timeline, "new-frame", G_CALLBACK (animation_frame_cb), self);
	g_signal_connect (self->priv->timeline, "started", G_CALLBACK (animation_started_cb), self);

	g_signal_connect (self, "show", G_CALLBACK (gth_slideshow_show_cb), self);
}


GtkWidget *
gth_slideshow_new (GthBrowser *browser,
		   GList      *file_list /* GthFileData */)
{
	GthSlideshow *window;

	window = (GthSlideshow *) g_object_new (GTH_TYPE_SLIDESHOW, NULL);
	_gth_slideshow_construct (window, browser, file_list);

	return (GtkWidget*) window;
}


void
gth_slideshow_set_delay (GthSlideshow *self,
			 guint         msecs)
{
	self->priv->delay = msecs;
}


void
gth_slideshow_set_automatic (GthSlideshow *self,
			     gboolean      automatic)
{
	self->priv->automatic = automatic;
}


void
gth_slideshow_set_loop (GthSlideshow *self,
			gboolean      loop)
{
	self->priv->loop = loop;
}


void
gth_slideshow_set_transitions (GthSlideshow *self,
			       GList        *transitions)
{
	_g_object_list_unref (self->priv->transitions);
	self->priv->transitions = _g_object_list_ref (transitions);
	self->priv->n_transitions = g_list_length (self->priv->transitions);
}
