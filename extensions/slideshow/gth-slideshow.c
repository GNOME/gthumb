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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#ifdef HAVE_CLUTTER
#include <clutter/clutter.h>
#include <clutter-gtk/clutter-gtk.h>
#endif /* HAVE_CLUTTER */
#if HAVE_GSTREAMER
#include <gst/gst.h>
#include <extensions/gstreamer_utils/gstreamer-utils.h>
#endif /* HAVE_GSTREAMER */
#include "gth-slideshow.h"
#include "gth-transition.h"

#define HIDE_CURSOR_DELAY 1000
#define DEFAULT_DELAY 2000


typedef enum {
	GTH_SLIDESHOW_DIRECTION_FORWARD,
	GTH_SLIDESHOW_DIRECTION_BACKWARD
} GthSlideshowDirection;


struct _GthSlideshowPrivate {
	GthProjector          *projector;
	GthBrowser            *browser;
	GList                 *file_list; /* GthFileData */
	gboolean               automatic;
	gboolean               wrap_around;
	GList                 *current;
	GthImagePreloader     *preloader;
	GList                 *transitions; /* GthTransition */
	int                    n_transitions;
	GthTransition         *transition;
	GthSlideshowDirection  direction;
#if HAVE_CLUTTER
	ClutterTimeline       *timeline;
	ClutterActor          *image1;
	ClutterActor          *image2;
#endif
	GtkWidget             *viewer;
	guint                  next_event;
	guint                  delay;
	guint                  hide_cursor_event;
	GRand                 *rand;
	gboolean               first_show;
	gboolean               one_loaded;
	char                 **audio_files;
	gboolean               audio_loop;
#if HAVE_GSTREAMER
	int                    current_audio_file;
	GstElement            *playbin;
#endif
	gboolean               paused;
	gboolean               animating;
};


static gpointer parent_class = NULL;


static void
_gth_slideshow_close (GthSlideshow *self)
{
	gboolean    close_browser;
	GthBrowser *browser;

	browser = self->priv->browser;
	close_browser = ! gtk_widget_get_visible (GTK_WIDGET (browser));
	gtk_widget_destroy (GTK_WIDGET (self));

	if (close_browser)
		gth_window_close (GTH_WINDOW (browser));
}


static void
_gth_slideshow_load_current_image (GthSlideshow *self)
{
	GthFileData *requested_file;
	GthFileData *next_file;
	GthFileData *prev_file;

	if (self->priv->next_event != 0) {
		g_source_remove (self->priv->next_event);
		self->priv->next_event = 0;
	}

	if (self->priv->current == NULL) {
		if (! self->priv->one_loaded || ! self->priv->wrap_around) {
			_gth_slideshow_close (self);
			return;
		}

		if (self->priv->direction == GTH_SLIDESHOW_DIRECTION_FORWARD)
			self->priv->current = g_list_first (self->priv->file_list);
		else
			self->priv->current = g_list_last (self->priv->file_list);
	}

	requested_file = (GthFileData *) self->priv->current->data;
	if (self->priv->current->next != NULL)
		next_file = (GthFileData *) self->priv->current->next->data;
	else
		next_file = NULL;
	if (self->priv->current->prev != NULL)
		prev_file = (GthFileData *) self->priv->current->prev->data;
	else
		prev_file = NULL;
	gth_image_preloader_load (self->priv->preloader,
				  requested_file,
				  next_file,
				  prev_file);
}


static void
_gth_slideshow_load_next_image (GthSlideshow *self)
{
	self->priv->projector->load_next_image (self);
	self->priv->direction = GTH_SLIDESHOW_DIRECTION_FORWARD;

	if (self->priv->paused)
		return;

	self->priv->current = self->priv->current->next;
	_gth_slideshow_load_current_image (self);
}


static void
_gth_slideshow_load_prev_image (GthSlideshow *self)
{
	self->priv->projector->load_prev_image (self);
	self->priv->direction = GTH_SLIDESHOW_DIRECTION_BACKWARD;

	if (self->priv->paused)
		return;

	self->priv->current = self->priv->current->prev;
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
view_next_image_automatically (GthSlideshow *self)
{
	if (self->priv->automatic) {
		if (self->priv->next_event != 0)
			g_source_remove (self->priv->next_event);
		self->priv->next_event = g_timeout_add (self->priv->delay, next_image_cb, self);
	}
}


static void
image_preloader_requested_ready_cb (GthImagePreloader *preloader,
				    GError            *error,
				    gpointer           user_data)
{
	GthSlideshow   *self = user_data;
	GthImageLoader *image_loader;
	GdkPixbuf      *pixbuf;

	if (error != NULL) {
		g_clear_error (&error);
		_gth_slideshow_load_next_image (self);
		return;
	}

	image_loader = gth_image_preloader_get_loader (self->priv->preloader, (GthFileData *) self->priv->current->data);
	if (image_loader == NULL) {
		_gth_slideshow_load_next_image (self);
		return;
	}

	pixbuf = gth_image_loader_get_pixbuf (image_loader);
	if (pixbuf == NULL) {
		_gth_slideshow_load_next_image (self);
		return;
	}

	self->priv->one_loaded = TRUE;
	self->priv->projector->image_ready (self, pixbuf);
}


static void
gth_slideshow_init (GthSlideshow *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_SLIDESHOW, GthSlideshowPrivate);
	self->priv->file_list = NULL;
	self->priv->next_event = 0;
	self->priv->delay = DEFAULT_DELAY;
	self->priv->automatic = FALSE;
	self->priv->wrap_around = FALSE;
	self->priv->transitions = NULL;
	self->priv->n_transitions = 0;
	self->priv->rand = g_rand_new ();
	self->priv->first_show = TRUE;
	self->priv->audio_files = NULL;
	self->priv->paused = FALSE;
	self->priv->animating = FALSE;
	self->priv->direction = GTH_SLIDESHOW_DIRECTION_FORWARD;

	self->priv->preloader = gth_image_preloader_new ();
	g_signal_connect (self->priv->preloader,
			  "requested_ready",
			  G_CALLBACK (image_preloader_requested_ready_cb),
			  self);
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
	_g_object_unref (self->priv->preloader);
	_g_object_list_unref (self->priv->transitions);
	g_rand_free (self->priv->rand);
	g_strfreev (self->priv->audio_files);
	self->priv->projector->finalize (self);

#if HAVE_GSTREAMER
	if (self->priv->playbin != NULL) {
		gst_element_set_state (self->priv->playbin, GST_STATE_NULL);
		gst_object_unref (GST_OBJECT (self->priv->playbin));
		self->priv->playbin = NULL;
	}
#endif

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
	self->priv->projector->hide_cursor (self);

	return FALSE;
}


static void
_gth_slideshow_toggle_pause (GthSlideshow *self)
{
	self->priv->paused = ! self->priv->paused;
	if (self->priv->paused) {
		self->priv->projector->paused (self);
#if HAVE_GSTREAMER
		if (self->priv->playbin != NULL)
			gst_element_set_state (self->priv->playbin, GST_STATE_PAUSED);
#endif
	}
	else { /* resume */
		_gth_slideshow_load_next_image (self);
#if HAVE_GSTREAMER
		if (self->priv->playbin != NULL)
			gst_element_set_state (self->priv->playbin, GST_STATE_PLAYING);
#endif
	}
}


#if HAVE_GSTREAMER
static void
bus_message_cb (GstBus     *bus,
                GstMessage *message,
                gpointer    user_data)
{
	GthSlideshow *self = user_data;

	if (GST_MESSAGE_TYPE (message) == GST_MESSAGE_EOS) {
		self->priv->current_audio_file++;
		if ((self->priv->audio_files[self->priv->current_audio_file] == NULL)
		    && self->priv->audio_loop)
		{
			self->priv->current_audio_file = 0;
		}
		gst_element_set_state (self->priv->playbin, GST_STATE_NULL);
		g_object_set (G_OBJECT (self->priv->playbin), "uri", self->priv->audio_files[self->priv->current_audio_file], NULL);
		gst_element_set_state (self->priv->playbin, GST_STATE_PLAYING);
	}
}
#endif


static void
gth_slideshow_show_cb (GtkWidget    *widget,
		       GthSlideshow *self)
{
	if (! self->priv->first_show)
		return;

#if HAVE_GSTREAMER
	if (gstreamer_init ()) {
		if ((self->priv->audio_files != NULL) && (self->priv->audio_files[0] != NULL)) {
			self->priv->current_audio_file = 0;
			if (self->priv->playbin == NULL) {
				GstBus *bus;

				self->priv->playbin = gst_element_factory_make ("playbin", "playbin");
				bus = gst_pipeline_get_bus (GST_PIPELINE (self->priv->playbin));
				gst_bus_add_signal_watch (bus);
				g_signal_connect (bus, "message", G_CALLBACK (bus_message_cb), self);
			}
			else
				gst_element_set_state (self->priv->playbin, GST_STATE_NULL);
			g_object_set (G_OBJECT (self->priv->playbin), "uri", self->priv->audio_files[self->priv->current_audio_file], NULL);
			gst_element_set_state (self->priv->playbin, GST_STATE_PLAYING);
		}
	}
#endif

	_gth_slideshow_load_current_image (self);
	self->priv->first_show = FALSE;
}


static void
_gth_slideshow_construct (GthSlideshow *self,
			  GthProjector *projector,
			  GthBrowser   *browser,
			  GList        *file_list)
{
	self->priv->projector = projector;
	self->priv->browser = _g_object_ref (browser);
	self->priv->file_list = _g_object_list_ref (file_list);
	self->priv->current = self->priv->file_list;
	self->priv->one_loaded = FALSE;

	self->priv->projector->construct (self);

	g_signal_connect (self, "show", G_CALLBACK (gth_slideshow_show_cb), self);
}


GtkWidget *
gth_slideshow_new (GthProjector *projector,
		   GthBrowser   *browser,
		   GList        *file_list /* GthFileData */)
{
	GthSlideshow *window;

	g_return_val_if_fail (projector != NULL, NULL);

	window = (GthSlideshow *) g_object_new (GTH_TYPE_SLIDESHOW, NULL);
	_gth_slideshow_construct (window, projector, browser, file_list);

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
gth_slideshow_set_wrap_around (GthSlideshow *self,
			       gboolean      wrap_around)
{
	self->priv->wrap_around = wrap_around;
}


void
gth_slideshow_set_transitions (GthSlideshow *self,
			       GList        *transitions)
{
	_g_object_list_unref (self->priv->transitions);
	self->priv->transitions = _g_object_list_ref (transitions);
	self->priv->n_transitions = g_list_length (self->priv->transitions);
}


void
gth_slideshow_set_playlist (GthSlideshow  *self,
			    char         **files)
{
	self->priv->audio_files = g_strdupv (files);
	self->priv->audio_loop = TRUE;
}


/* -- default projector -- */


static void
default_projector_load_next_image (GthSlideshow *self)
{
	/* void */
}


static void
default_projector_load_prev_image (GthSlideshow *self)
{
	/* void */
}


static void
default_projector_image_ready (GthSlideshow *self,
			       GdkPixbuf    *pixbuf)
{
	gth_image_viewer_set_pixbuf (GTH_IMAGE_VIEWER (self->priv->viewer), pixbuf);
	view_next_image_automatically (self);
}


static void
default_projector_finalize (GthSlideshow *self)
{
	/* void */
}


static void
default_projector_hide_cursor (GthSlideshow *self)
{
	gth_image_viewer_hide_cursor (GTH_IMAGE_VIEWER (self->priv->viewer));
}


static void
default_projector_paused (GthSlideshow *self)
{
	/* void */
}


static void
viewer_event_cb (GtkWidget    *widget,
	         GdkEvent     *event,
	         GthSlideshow *self)
{
	if (event->type == GDK_MOTION_NOTIFY) {
		gth_image_viewer_show_cursor (GTH_IMAGE_VIEWER (self->priv->viewer));
		if (self->priv->hide_cursor_event != 0)
			g_source_remove (self->priv->hide_cursor_event);
		self->priv->hide_cursor_event = g_timeout_add (HIDE_CURSOR_DELAY, hide_cursor_cb, self);
	}
	else if (event->type == GDK_BUTTON_PRESS) {
		switch (((GdkEventButton *) event)->button) {
		case 1:
			_gth_slideshow_load_next_image (self);
			break;
		case 3:
			_gth_slideshow_load_prev_image (self);
			break;
		default:
			break;
		}
	}
	else if (event->type == GDK_KEY_RELEASE) {
		switch (((GdkEventKey *) event)->keyval) {
		case GDK_Escape:
			_gth_slideshow_close (self);
			break;

		case GDK_space:
			_gth_slideshow_toggle_pause (self);
			break;

		case GDK_Down:
		case GDK_Right:
		case GDK_Page_Down:
			_gth_slideshow_load_next_image (self);
			break;

		case GDK_Up:
		case GDK_Left:
		case GDK_Page_Up:
			_gth_slideshow_load_prev_image (self);
			break;
		}
	}
}


static void
default_projector_construct (GthSlideshow *self)
{
	self->priv->viewer = gth_image_viewer_new ();
	gth_image_viewer_set_black_background (GTH_IMAGE_VIEWER (self->priv->viewer), TRUE);
	gth_image_viewer_hide_frame (GTH_IMAGE_VIEWER (self->priv->viewer));
	gth_image_viewer_set_fit_mode (GTH_IMAGE_VIEWER (self->priv->viewer), GTH_FIT_SIZE);
	gth_image_viewer_set_zoom_change (GTH_IMAGE_VIEWER (self->priv->viewer), GTH_ZOOM_CHANGE_FIT_SIZE);
	gth_image_viewer_set_transp_type (GTH_IMAGE_VIEWER (self->priv->viewer), GTH_TRANSP_TYPE_BLACK);
	gth_image_viewer_set_zoom_quality (GTH_IMAGE_VIEWER (self->priv->viewer), GTH_ZOOM_QUALITY_LOW);

	g_signal_connect (self->priv->viewer, "button-press-event", G_CALLBACK (viewer_event_cb), self);
	g_signal_connect (self->priv->viewer, "motion-notify-event", G_CALLBACK (viewer_event_cb), self);
	g_signal_connect (self->priv->viewer, "key-release-event", G_CALLBACK (viewer_event_cb), self);

	gtk_widget_show (self->priv->viewer);
	gtk_container_add (GTK_CONTAINER (self), self->priv->viewer);
}


GthProjector default_projector = {
	default_projector_construct,
	default_projector_paused,
	default_projector_hide_cursor,
	default_projector_finalize,
	default_projector_image_ready,
	default_projector_load_prev_image,
	default_projector_load_next_image
};


#ifdef HAVE_CLUTTER


/* -- clutter projector -- */


static void
_gth_slideshow_swap_current_and_next (GthSlideshow *self)
{
	ClutterGeometry tmp_geometry;

	self->current_image = self->next_image;
	if (self->current_image == self->priv->image1)
		self->next_image = self->priv->image2;
	else
		self->next_image = self->priv->image1;

	tmp_geometry = self->current_geometry;
	self->current_geometry = self->next_geometry;
	self->next_geometry = tmp_geometry;
}


static void
reset_texture_transformation (GthSlideshow *self,
			      ClutterActor *texture)
{
	float stage_w, stage_h;

	if (texture == NULL)
		return;

	clutter_actor_get_size (self->stage, &stage_w, &stage_h);

	clutter_actor_set_anchor_point (texture, 0.0, 0.0);
	clutter_actor_set_opacity (texture, 255);
	clutter_actor_set_rotation (texture,
				    CLUTTER_X_AXIS,
				    0.0,
				    stage_w / 2.0,
				    stage_h / 2.0,
				    0.0);
	clutter_actor_set_rotation (texture,
				    CLUTTER_Y_AXIS,
				    0.0,
				    stage_w / 2.0,
				    stage_h / 2.0,
				    0.0);
	clutter_actor_set_rotation (texture,
				    CLUTTER_Z_AXIS,
				    0.0,
				    stage_w / 2.0,
				    stage_h / 2.0,
				    0.0);
	clutter_actor_set_scale (texture, 1.0, 1.0);
}


static void
_gth_slideshow_reset_textures_position (GthSlideshow *self)
{
	if (self->next_image != NULL) {
		clutter_actor_set_size (self->next_image, (float) self->next_geometry.width, (float) self->next_geometry.height);
		clutter_actor_set_position (self->next_image, (float) self->next_geometry.x, (float) self->next_geometry.y);
	}

	if (self->current_image != NULL) {
		clutter_actor_set_size (self->current_image, (float) self->current_geometry.width, (float) self->current_geometry.height);
		clutter_actor_set_position (self->current_image, (float) self->current_geometry.x, (float) self->current_geometry.y);
	}

	if ((self->current_image != NULL) && (self->next_image != NULL)) {
		clutter_actor_raise (self->current_image, self->next_image);
		clutter_actor_hide (self->next_image);
	}

	if (self->current_image != NULL)
		clutter_actor_show (self->current_image);

	reset_texture_transformation (self, self->next_image);
	reset_texture_transformation (self, self->current_image);
}


static void
_gth_slideshow_animation_completed (GthSlideshow *self)
{
	self->priv->animating = FALSE;
	if (clutter_timeline_get_direction (self->priv->timeline) == CLUTTER_TIMELINE_FORWARD)
		_gth_slideshow_swap_current_and_next (self);
	_gth_slideshow_reset_textures_position (self);
}


static void
clutter_projector_load_next_image (GthSlideshow *self)
{
	if (clutter_timeline_is_playing (self->priv->timeline)) {
		clutter_timeline_pause (self->priv->timeline);
		_gth_slideshow_animation_completed (self);
	}
	clutter_timeline_set_direction (self->priv->timeline, CLUTTER_TIMELINE_FORWARD);
}


static void
clutter_projector_load_prev_image (GthSlideshow *self)
{
	if (clutter_timeline_is_playing (self->priv->timeline)) {
		clutter_timeline_pause (self->priv->timeline);
		_gth_slideshow_animation_completed (self);
	}
	clutter_timeline_set_direction (self->priv->timeline, CLUTTER_TIMELINE_BACKWARD);
}


static void
animation_completed_cb (ClutterTimeline *timeline,
			GthSlideshow    *self)
{
	_gth_slideshow_animation_completed (self);
	view_next_image_automatically (self);
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
	self->priv->animating = TRUE;
	self->first_frame = TRUE;
}


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
clutter_projector_image_ready (GthSlideshow *self,
			       GdkPixbuf    *pixbuf)
{
	GdkPixbuf    *image;
	ClutterActor *texture;
	int           pixbuf_w, pixbuf_h;
	float         stage_w, stage_h;
	int           pixbuf_x, pixbuf_y;

	clutter_actor_get_size (self->stage, &stage_w, &stage_h);
	if ((stage_w == 0) || (stage_h == 0))
		return;

	image = gdk_pixbuf_new (gdk_pixbuf_get_colorspace (pixbuf),
				FALSE,
				gdk_pixbuf_get_bits_per_sample (pixbuf),
				stage_w,
				stage_h);
	gdk_pixbuf_fill (image, 0x000000ff);

	pixbuf_w = gdk_pixbuf_get_width (pixbuf);
	pixbuf_h = gdk_pixbuf_get_height (pixbuf);
	scale_keeping_ratio (&pixbuf_w, &pixbuf_h, (int) stage_w, (int) stage_h, TRUE);
	pixbuf_x = (stage_w - pixbuf_w) / 2;
	pixbuf_y = (stage_h - pixbuf_h) / 2;

	gdk_pixbuf_composite (pixbuf,
			      image,
			      pixbuf_x,
			      pixbuf_y,
			      pixbuf_w,
			      pixbuf_h,
			      pixbuf_x,
			      pixbuf_y,
			      (double) pixbuf_w / gdk_pixbuf_get_width (pixbuf),
			      (double) pixbuf_h / gdk_pixbuf_get_height (pixbuf),
			      GDK_INTERP_BILINEAR,
			      255);

	if (self->next_image == self->priv->image1)
		texture = self->priv->image1;
	else
		texture = self->priv->image2;
	gtk_clutter_texture_set_from_pixbuf (CLUTTER_TEXTURE (texture), image, NULL);

	self->next_geometry.x = 0;
	self->next_geometry.y = 0;
	self->next_geometry.width = stage_w;
	self->next_geometry.height = stage_h;

	_gth_slideshow_reset_textures_position (self);
	if (clutter_timeline_get_direction (self->priv->timeline) == CLUTTER_TIMELINE_BACKWARD)
		_gth_slideshow_swap_current_and_next (self);

	self->priv->transition = _gth_slideshow_get_transition (self);
	clutter_timeline_rewind (self->priv->timeline);
	clutter_timeline_start (self->priv->timeline);
	if (self->current_image == NULL)
		clutter_timeline_advance (self->priv->timeline, GTH_TRANSITION_DURATION);

	g_object_unref (image);
}


static void
clutter_projector_finalize (GthSlideshow *self)
{
	_g_object_unref (self->priv->timeline);
}


static void
clutter_projector_hide_cursor (GthSlideshow *self)
{
	clutter_stage_hide_cursor (CLUTTER_STAGE (self->stage));
}


static void
clutter_projector_paused (GthSlideshow *self)
{
	if (self->priv->animating) {
		clutter_timeline_pause (self->priv->timeline);
		_gth_slideshow_animation_completed (self);
	}
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
	else if (event->type == CLUTTER_BUTTON_PRESS) {
		switch (clutter_event_get_button (event)) {
		case 1:
			_gth_slideshow_load_next_image (self);
			break;
		case 3:
			_gth_slideshow_load_prev_image (self);
			break;
		default:
			break;
		}
	}
	else if (event->type == CLUTTER_KEY_RELEASE) {
		switch (clutter_event_get_key_symbol (event)) {
		case CLUTTER_Escape:
			_gth_slideshow_close (self);
			break;

		case CLUTTER_space:
			_gth_slideshow_toggle_pause (self);
			break;

		case CLUTTER_Up:
		case CLUTTER_Right:
		case CLUTTER_Page_Up:
			_gth_slideshow_load_next_image (self);
			break;

		case CLUTTER_Down:
		case CLUTTER_Left:
		case CLUTTER_Page_Down:
			_gth_slideshow_load_prev_image (self);
			break;
		}
	}
}


static void
gth_slideshow_size_allocate_cb (GtkWidget     *widget,
				GtkAllocation *allocation,
				gpointer       user_data)
{
	GthSlideshow   *self = user_data;
	gfloat          stage_w, stage_h;
	GthImageLoader *image_loader;
	GdkPixbuf      *pixbuf;
	GdkPixbuf      *image;
	int             pixbuf_w, pixbuf_h;
	int             pixbuf_x, pixbuf_y;
	ClutterActor   *texture;

	if (self->current_image == NULL)
		return;

	clutter_actor_get_size (self->stage, &stage_w, &stage_h);
	if ((stage_w == 0) || (stage_h == 0))
		return;

	image_loader = gth_image_preloader_get_loader (self->priv->preloader, (GthFileData *) self->priv->current->data);
	if (image_loader == NULL)
		return;

	pixbuf = gth_image_loader_get_pixbuf (image_loader);
	if (pixbuf == NULL)
		return;

	image = gdk_pixbuf_new (gdk_pixbuf_get_colorspace (pixbuf),
				FALSE,
				gdk_pixbuf_get_bits_per_sample (pixbuf),
				stage_w,
				stage_h);
	gdk_pixbuf_fill (image, 0x000000ff);

	pixbuf_w = gdk_pixbuf_get_width (pixbuf);
	pixbuf_h = gdk_pixbuf_get_height (pixbuf);
	scale_keeping_ratio (&pixbuf_w, &pixbuf_h, (int) stage_w, (int) stage_h, TRUE);
	pixbuf_x = (stage_w - pixbuf_w) / 2;
	pixbuf_y = (stage_h - pixbuf_h) / 2;

	gdk_pixbuf_composite (pixbuf,
			      image,
			      pixbuf_x,
			      pixbuf_y,
			      pixbuf_w,
			      pixbuf_h,
			      pixbuf_x,
			      pixbuf_y,
			      (double) pixbuf_w / gdk_pixbuf_get_width (pixbuf),
			      (double) pixbuf_h / gdk_pixbuf_get_height (pixbuf),
			      GDK_INTERP_BILINEAR,
			      255);

	if (self->current_image == self->priv->image1)
		texture = self->priv->image1;
	else
		texture = self->priv->image2;
	gtk_clutter_texture_set_from_pixbuf (CLUTTER_TEXTURE (texture), image, NULL);

	self->current_geometry.x = 0;
	self->current_geometry.y = 0;
	self->current_geometry.width = stage_w;
	self->current_geometry.height = stage_h;
	_gth_slideshow_reset_textures_position (self);

	g_object_unref (image);
}


static void
clutter_projector_construct (GthSlideshow *self)
{
	GtkWidget    *embed;
	ClutterColor  background_color = { 0x0, 0x0, 0x0, 0xff };

	embed = gtk_clutter_embed_new ();
	self->stage = gtk_clutter_embed_get_stage (GTK_CLUTTER_EMBED (embed));
	clutter_stage_hide_cursor (CLUTTER_STAGE (self->stage));
	clutter_stage_set_color (CLUTTER_STAGE (self->stage), &background_color);
	g_signal_connect (self->stage, "button-press-event", G_CALLBACK (stage_input_cb), self);
	g_signal_connect (self->stage, "motion-event", G_CALLBACK (stage_input_cb), self);
	g_signal_connect (self->stage, "key-release-event", G_CALLBACK (stage_input_cb), self);

	gtk_widget_show (embed);
	gtk_container_add (GTK_CONTAINER (self), embed);

	self->priv->image1 = clutter_texture_new ();
	clutter_actor_hide (self->priv->image1);
	clutter_container_add_actor (CLUTTER_CONTAINER (self->stage), self->priv->image1);

	self->priv->image2 = clutter_texture_new ();
	clutter_actor_hide (self->priv->image2);
	clutter_container_add_actor (CLUTTER_CONTAINER (self->stage), self->priv->image2);

	self->current_image = NULL;
	self->next_image = self->priv->image1;

	self->priv->timeline = clutter_timeline_new (GTH_TRANSITION_DURATION);
	g_signal_connect (self->priv->timeline, "completed", G_CALLBACK (animation_completed_cb), self);
	g_signal_connect (self->priv->timeline, "new-frame", G_CALLBACK (animation_frame_cb), self);
	g_signal_connect (self->priv->timeline, "started", G_CALLBACK (animation_started_cb), self);

	g_signal_connect (self, "size-allocate", G_CALLBACK (gth_slideshow_size_allocate_cb), self);
}


GthProjector clutter_projector = {
	clutter_projector_construct,
	clutter_projector_paused,
	clutter_projector_hide_cursor,
	clutter_projector_finalize,
	clutter_projector_image_ready,
	clutter_projector_load_prev_image,
	clutter_projector_load_next_image
};


#endif /* HAVE_CLUTTER*/
