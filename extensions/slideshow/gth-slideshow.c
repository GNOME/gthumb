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

#define ANIMATION_DURATION 200
#define OPACITY_AT_MSECS(t)((int) (255 * ((double) (t) / ANIMATION_DURATION)))
#define HIDE_CURSOR_DELAY 1000
#define DEFAULT_DELAY 2000
#define ANGLE_AT_MSECS(t)((180.0 * ((double) (t) / ANIMATION_DURATION)))
#define POSITION_AT_MSECS(x, t)(((float)(x) * ((double) (t) / ANIMATION_DURATION)))

struct _GthSlideshowPrivate {
	GthBrowser      *browser;
	GList           *file_list; /* GthFileData */
	GList           *current;
	GthImageLoader  *image_loader;
	ClutterActor    *stage;
	ClutterActor    *texture1;
	ClutterActor    *texture2;
	ClutterActor    *current_texture;
	ClutterActor    *next_texture;
	ClutterTimeline *timeline;
	gboolean         first_frame;
	guint            next_event;
	guint            delay;
	guint            hide_cursor_event;
};


static gpointer parent_class = NULL;


static void
_gth_slideshow_load_current_image (GthSlideshow *self)
{
	if (self->priv->current == NULL) {
		gtk_widget_destroy (GTK_WIDGET (self));
		return;
	}

	gth_image_loader_set_file_data (GTH_IMAGE_LOADER (self->priv->image_loader), (GthFileData *) self->priv->current->data);
	gth_image_loader_load (GTH_IMAGE_LOADER (self->priv->image_loader));
}


static void
_gth_slideshow_load_next_image (GthSlideshow *self)
{
	if (clutter_timeline_is_playing (self->priv->timeline))
		return;
	self->priv->current = self->priv->current->next;
	clutter_timeline_set_direction (self->priv->timeline, CLUTTER_TIMELINE_FORWARD);
	_gth_slideshow_load_current_image (self);
}


static void
_gth_slideshow_load_prev_image (GthSlideshow *self)
{
	if (clutter_timeline_is_playing (self->priv->timeline))
			return;
	self->priv->current = self->priv->current->prev;
	clutter_timeline_set_direction (self->priv->timeline, CLUTTER_TIMELINE_BACKWARD);
	_gth_slideshow_load_current_image (self);
}


static gboolean
next_image_cb (gpointer user_data)
{
	GthSlideshow *self = user_data;

	g_source_remove (self->priv->next_event);
	self->priv->next_event = 0;
	_gth_slideshow_load_next_image (self);

	return FALSE;
}


static void
animation_completed_cb (ClutterTimeline *timeline,
			GthSlideshow    *self)
{
	if (clutter_timeline_get_direction (self->priv->timeline) == CLUTTER_TIMELINE_FORWARD) {
		self->priv->current_texture = self->priv->next_texture;
		if (self->priv->current_texture == self->priv->texture1)
			self->priv->next_texture = self->priv->texture2;
		else
			self->priv->next_texture = self->priv->texture1;
	}

	/*self->priv->next_event = g_timeout_add (self->priv->delay, next_image_cb, self);*/
}


static void
animation_frame_cb (ClutterTimeline *timeline,
		    int              msecs,
		    GthSlideshow    *self)
{
	float image_w, image_h;

#ifndef SLIDE
	clutter_actor_get_size (self->priv->stage, &image_w, &image_h);

	clutter_actor_set_x (self->priv->next_texture, POSITION_AT_MSECS(image_w, ANIMATION_DURATION - msecs));
	if (self->priv->current_texture != NULL)
		clutter_actor_set_x (self->priv->current_texture, POSITION_AT_MSECS(- image_w, msecs));

	if (self->priv->first_frame) {
		if (self->priv->current_texture != NULL)
			clutter_actor_show (self->priv->current_texture);
		clutter_actor_show (self->priv->next_texture);
		self->priv->first_frame = FALSE;
	}
#endif

#ifdef FADE
	if (self->priv->current_texture != NULL)
		clutter_actor_set_opacity (self->priv->current_texture, OPACITY_AT_MSECS(ANIMATION_DURATION - msecs));
	clutter_actor_set_opacity (self->priv->next_texture, OPACITY_AT_MSECS(msecs));

	if (self->priv->first_frame) {
		if (self->priv->current_texture != NULL) {
			clutter_actor_show (self->priv->current_texture);
			clutter_actor_raise (self->priv->next_texture, self->priv->current_texture);
		}
		clutter_actor_show (self->priv->next_texture);
		self->priv->first_frame = FALSE;
	}
#endif

#ifdef ROTATE
	if ((float) msecs >= (float) ANIMATION_DURATION / 2.0) {
		clutter_actor_show (self->priv->next_texture);
		if (self->priv->current_texture != NULL)
			clutter_actor_hide (self->priv->current_texture);
	}
	else {
		clutter_actor_hide (self->priv->next_texture);
		if (self->priv->current_texture != NULL)
			clutter_actor_show (self->priv->current_texture);
	}

	clutter_actor_get_size (self->priv->stage, &image_w, &image_h);
	clutter_actor_set_rotation (self->priv->next_texture,
				    CLUTTER_Y_AXIS,
		 		    ANGLE_AT_MSECS (ANIMATION_DURATION - msecs),
		 		    image_w / 2.0,
		 		    0.0,
		 		    0.0);
	if (self->priv->current_texture != NULL)
		clutter_actor_set_rotation (self->priv->current_texture,
					    CLUTTER_Y_AXIS,
					    ANGLE_AT_MSECS (- msecs),
					    image_w / 2.0,
					    0.0,
					    0.0);

	if (self->priv->first_frame) {
		if (self->priv->current_texture != NULL)
			clutter_actor_raise (self->priv->next_texture, self->priv->current_texture);
		clutter_actor_show (self->priv->next_texture);
		self->priv->first_frame = FALSE;
	}
#endif
}


static void
animation_started_cb (ClutterTimeline *timeline,
		      GthSlideshow    *self)
{
	self->priv->first_frame = TRUE;
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
	}

	clutter_actor_hide (self->priv->next_texture);

	image = gth_image_loader_get_pixbuf (GTH_IMAGE_LOADER (image_loader));
	gtk_clutter_texture_set_from_pixbuf (CLUTTER_TEXTURE (self->priv->next_texture), image, NULL);

	image_w = gdk_pixbuf_get_width (image);
	image_h = gdk_pixbuf_get_height (image);
	clutter_actor_get_size (self->priv->stage, &stage_w, &stage_h);
	scale_keeping_ratio (&image_w, &image_h, (int) stage_w, (int) stage_h, TRUE);
	clutter_actor_set_size (self->priv->next_texture, (float) image_w, (float) image_h);

	image_x = (stage_w - image_w) / 2;
	image_y = (stage_h - image_h) / 2;
	clutter_actor_set_position (self->priv->next_texture, image_x, image_y);

	if (clutter_timeline_get_direction (self->priv->timeline) == CLUTTER_TIMELINE_BACKWARD) {
		ClutterActor *tmp;

		tmp = self->priv->next_texture;
		self->priv->next_texture = self->priv->current_texture;
		self->priv->current_texture = tmp;
	}

	clutter_timeline_rewind (self->priv->timeline);
	clutter_timeline_start (self->priv->timeline);
	if (self->priv->current_texture == NULL)
		clutter_timeline_advance (self->priv->timeline, ANIMATION_DURATION);
}


static void
gth_slideshow_init (GthSlideshow *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_SLIDESHOW, GthSlideshowPrivate);
	self->priv->file_list = NULL;
	self->priv->next_event = 0;
	self->priv->delay = DEFAULT_DELAY;

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

	clutter_stage_hide_cursor (CLUTTER_STAGE (self->priv->stage));

	return FALSE;
}


static void
stage_input_cb (ClutterStage *stage,
	        ClutterEvent *event,
	        GthSlideshow *self)
{
	if (event->type == CLUTTER_MOTION) {
		clutter_stage_show_cursor (CLUTTER_STAGE (self->priv->stage));
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
_gth_slideshow_construct (GthSlideshow *self,
			  GthBrowser   *browser,
			  GList        *file_list)
{
	GtkWidget    *embed;
	ClutterColor  stage_color = { 0x0, 0x0, 0x0, 0xff };

	self->priv->browser = _g_object_ref (browser);
	self->priv->file_list = _g_object_list_ref (file_list);
	self->priv->current = self->priv->file_list;

	embed = gtk_clutter_embed_new ();
	self->priv->stage = gtk_clutter_embed_get_stage (GTK_CLUTTER_EMBED (embed));
	clutter_stage_hide_cursor (CLUTTER_STAGE (self->priv->stage));
	clutter_stage_set_color (CLUTTER_STAGE (self->priv->stage), &stage_color);
	g_signal_connect (self->priv->stage, "motion-event", G_CALLBACK (stage_input_cb), self);
	g_signal_connect (self->priv->stage, "key-release-event", G_CALLBACK (stage_input_cb), self);

	gtk_widget_show (embed);
	gtk_container_add (GTK_CONTAINER (self), embed);

	self->priv->texture1 = clutter_texture_new ();
	clutter_actor_hide (self->priv->texture1);
	clutter_container_add_actor (CLUTTER_CONTAINER (self->priv->stage), self->priv->texture1);

	self->priv->texture2 = clutter_texture_new ();
	clutter_actor_hide (self->priv->texture2);
	clutter_container_add_actor (CLUTTER_CONTAINER (self->priv->stage), self->priv->texture2);

	self->priv->current_texture = NULL;
	self->priv->next_texture = self->priv->texture1;

	self->priv->timeline = clutter_timeline_new (ANIMATION_DURATION);
	g_signal_connect (self->priv->timeline, "completed", G_CALLBACK (animation_completed_cb), self);
	g_signal_connect (self->priv->timeline, "new-frame", G_CALLBACK (animation_frame_cb), self);
	g_signal_connect (self->priv->timeline, "started", G_CALLBACK (animation_started_cb), self);
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


static gboolean
start_playing (gpointer user_data)
{
	GthSlideshow *self = user_data;

	_gth_slideshow_load_current_image (self);

	return FALSE;
}


void
gth_slideshow_play (GthSlideshow *self)
{
	g_idle_add (start_playing, self);
}
