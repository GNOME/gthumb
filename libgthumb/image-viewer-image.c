/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001-2008 The Free Software Foundation, Inc.
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

#include <math.h>

#include "image-viewer.h"
#include "image-viewer-image.h"

#define FLOAT_EQUAL(a,b) (fabs ((a) - (b)) < 1e-6)

#define MINIMUM_DELAY   10    /* When an animation frame has a 0 milli seconds
			       * delay use this delay instead. */

typedef struct _ImageViewerImagePrivate ImageViewerImagePrivate;

struct _ImageViewerImagePrivate {
	/* Pointer to parent widget to get size info, animation playback status,
	 * etc. Not ref'ed as we are destroyed by parent when parent.
	 * (This is identical to how GtkWidget's hold their parent widgets.
	 * Make it a construct-only property perhaps? */
	ImageViewer            *viewer;

	FileData               *file;

	gboolean                is_animation;

	guint                   anim_id;
	GdkPixbuf              *anim_pixbuf; /* Pixbuf used for animation, not ref'd */
	gint                    frame_delay;
	GdkPixbufAnimation     *anim;
	GdkPixbufAnimationIter *iter;
	GTimeVal                time;        /* Timer used to get the right
					      * frame. */

	GdkPixbuf              *static_pixbuf; /* Pixbuf used for static images, is ref'd */

	gdouble                 zoom_level;

	GthFit                  fit;

	gdouble                x_offset;           /* Scroll offsets. */
	gdouble                y_offset;

	cairo_surface_t       *buffer;
};

enum {
	ZOOM_CHANGED,
	LAST_SIGNAL
};

cairo_surface_t*               pixbuf_to_surface (GdkPixbuf *pixbuf);

static guint image_viewer_image_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE(ImageViewerImage, image_viewer_image, G_TYPE_OBJECT)
#define IMAGE_VIEWER_IMAGE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), \
					   IMAGE_VIEWER_IMAGE_TYPE, \
					   ImageViewerImagePrivate))

static void image_viewer_image_finalize (GObject *object);

static void
image_viewer_image_class_init (ImageViewerImageClass *klass)
{
	GObjectClass   *gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->finalize = image_viewer_image_finalize;

	g_type_class_add_private (klass, sizeof (ImageViewerImagePrivate));

	image_viewer_image_signals[ZOOM_CHANGED] = g_signal_new ("zoom_changed",
		      G_TYPE_FROM_CLASS (klass),
		      G_SIGNAL_RUN_LAST,
		      G_STRUCT_OFFSET (ImageViewerImageClass, zoom_changed),
		      NULL, NULL,
		      g_cclosure_marshal_VOID__VOID,
		      G_TYPE_NONE,
		      0);

	klass->zoom_changed = NULL;

}

static void
image_viewer_image_init (ImageViewerImage *self)
{
	ImageViewerImagePrivate  *priv = IMAGE_VIEWER_IMAGE_GET_PRIVATE (self);

	priv->viewer = NULL;
	priv->is_animation = FALSE;

	priv->anim_pixbuf = NULL;
	priv->frame_delay = 0;
	priv->anim_id = 0;

	priv->anim = NULL;
	priv->iter = NULL;

	priv->static_pixbuf = NULL;

	priv->zoom_level = 1.0;
	priv->fit = GTH_FIT_NONE;

	priv->x_offset = 0;
	priv->y_offset = 0;

	priv->buffer = NULL;
}

static void
image_viewer_image_finalize (GObject *object)
{
        ImageViewerImage        *self;
	ImageViewerImagePrivate *priv;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_IMAGE_VIEWER_IMAGE (object));

        self = IMAGE_VIEWER_IMAGE (object);
	priv = IMAGE_VIEWER_IMAGE_GET_PRIVATE (self);

	priv->viewer = NULL;

	if (priv->anim_id != 0) {
		g_source_remove (priv->anim_id);
		priv->anim_id = 0;
	}

	if (priv->iter != NULL) {
		g_object_unref (priv->iter);
		priv->iter = NULL;
	}

	if (priv->anim != NULL) {
		g_object_unref (priv->anim);
		priv->anim = NULL;
	}

	if (priv->static_pixbuf != NULL) {
		g_object_unref (priv->static_pixbuf);
		priv->static_pixbuf = NULL;
	}

	if (priv->file != NULL) {
		file_data_unref (priv->file);
		priv->file = NULL;
	}

	if (priv->buffer != NULL) {
		cairo_surface_destroy (priv->buffer);
		priv->buffer = NULL;
	}

        /* Chain up */
	G_OBJECT_CLASS (image_viewer_image_parent_class)->finalize (object);
}


static gboolean change_frame_cb (gpointer data);

static void
add_change_frame_timeout (ImageViewerImage *image)
{
	ImageViewerImagePrivate *priv = IMAGE_VIEWER_IMAGE_GET_PRIVATE (image);
	if (priv->is_animation &&
	    image_viewer_get_play_animation (priv->viewer) &&
	    (priv->anim_id == 0)) {
		priv->anim_id = g_timeout_add (
			MAX (MINIMUM_DELAY, priv->frame_delay),
			change_frame_cb,
			image);
	}
}

static void
create_pixbuf_from_iter (ImageViewerImage *image)
{
	ImageViewerImagePrivate *priv = IMAGE_VIEWER_IMAGE_GET_PRIVATE (image);
	GdkPixbufAnimationIter  *iter = priv->iter;
	priv->anim_pixbuf = gdk_pixbuf_animation_iter_get_pixbuf (iter);
	priv->frame_delay = gdk_pixbuf_animation_iter_get_delay_time (iter);

	if (priv->buffer != NULL) {
		cairo_surface_destroy (priv->buffer);
		priv->buffer = NULL;
	}

	if (priv->anim_pixbuf != NULL) {
		priv->buffer = pixbuf_to_surface (priv->anim_pixbuf);
	}

	gtk_widget_queue_draw (GTK_WIDGET (priv->viewer));
}


static void
create_first_pixbuf (ImageViewerImage *image)
{
	ImageViewerImagePrivate *priv = IMAGE_VIEWER_IMAGE_GET_PRIVATE (image);

	priv->anim_pixbuf = NULL;
	priv->frame_delay = 0;

	if (priv->iter != NULL)
		g_object_unref (priv->iter);

	g_get_current_time (&priv->time);

	priv->iter = gdk_pixbuf_animation_get_iter (priv->anim, &priv->time);
	create_pixbuf_from_iter (image);
}


static gboolean
change_frame_cb (gpointer data)
{
	ImageViewerImage        *image = IMAGE_VIEWER_IMAGE (data);
	ImageViewerImagePrivate *priv  = IMAGE_VIEWER_IMAGE_GET_PRIVATE (image);

	if (priv->anim_id != 0) {
		g_source_remove (priv->anim_id);
		priv->anim_id = 0;
	}

	g_time_val_add (&priv->time, (glong) priv->frame_delay * 1000);
	gdk_pixbuf_animation_iter_advance (priv->iter, &priv->time);

	create_pixbuf_from_iter (image);
	add_change_frame_timeout (image);

	return FALSE;
}

static void
clamp_offsets (ImageViewerImage *image)
{
	ImageViewerImagePrivate *priv = IMAGE_VIEWER_IMAGE_GET_PRIVATE (image);
	gint                     border, border2;
	gint                     gdk_width, gdk_height;
	gdouble                  width, height;

	if (image_viewer_is_frame_visible (priv->viewer)) {
		border  = FRAME_BORDER;
		border2 = FRAME_BORDER2;
	} else {
		border = border2 = 0;
	}

	gdk_width = GTK_WIDGET (priv->viewer)->allocation.width - border2;
	gdk_height = GTK_WIDGET (priv->viewer)->allocation.height - border2;

	image_viewer_image_get_zoomed_size (image, &width, &height);

	if (width > gdk_width)
		priv->x_offset = CLAMP (priv->x_offset,
				floor (-width/2 - border),
				floor ( width/2 + border));
	else
		priv->x_offset = 0;

	if (height > gdk_height)
		priv->y_offset = CLAMP (priv->y_offset,
				floor (-height/2 - border),
				floor ( height/2 + border));
	else
		priv->y_offset = 0;
}


static gdouble zooms[] = {                  0.05, 0.07, 0.10,
			  0.15, 0.20, 0.30, 0.50, 0.75, 1.0,
			  1.5 , 2.0 , 3.0 , 5.0 , 7.5,  10.0,
			  15.0, 20.0, 30.0, 50.0, 75.0, 100.0};

static const gint nzooms = G_N_ELEMENTS (zooms);


static gdouble
get_next_zoom (gdouble zoom)
{
	gint i;

	i = 0;
	while ((i < nzooms) && (zooms[i] <= zoom))
		i++;
	i = CLAMP (i, 0, nzooms - 1);

	return zooms[i];
}


static gdouble
get_prev_zoom (gdouble zoom)
{
	gint i;

	i = nzooms - 1;
	while ((i >= 0) && (zooms[i] >= zoom))
		i--;
	i = CLAMP (i, 0, nzooms - 1);

	return zooms[i];
}


static void
set_zoom_at_point (ImageViewerImage *image,
		   gdouble           zoom_level,
		   gdouble           center_x,
		   gdouble           center_y)
{
	ImageViewerImagePrivate *priv = IMAGE_VIEWER_IMAGE_GET_PRIVATE (image);
	GtkWidget               *widget = GTK_WIDGET (priv->viewer);
	gdouble                  gdk_width, gdk_height;
	gint                     border;
	gdouble                  zoom_ratio;

	border = image_viewer_is_frame_visible (priv->viewer) ?
		 FRAME_BORDER2 : 0;

	gdk_width = widget->allocation.width - border;
	gdk_height = widget->allocation.height - border;

	/* try to keep the center of the view visible. */

	zoom_ratio = zoom_level / priv->zoom_level;
	priv->x_offset = ((priv->x_offset - gdk_width  / 2 + center_x) * zoom_ratio);
	priv->y_offset = ((priv->y_offset - gdk_height / 2 + center_y) * zoom_ratio);

	priv->zoom_level = zoom_level;

	/* Check whether the offset is still valid. */
	clamp_offsets (image);

	if (priv->fit == GTH_FIT_NONE)
		gtk_widget_queue_resize (GTK_WIDGET (priv->viewer));
	else
		gtk_widget_queue_draw (GTK_WIDGET (priv->viewer));

	g_signal_emit (G_OBJECT (image),
		       image_viewer_image_signals[ZOOM_CHANGED],
		       0);
}


static void
set_zoom_centered (ImageViewerImage* image,
		   gdouble           zoom_level)
{
	ImageViewerImagePrivate *priv = IMAGE_VIEWER_IMAGE_GET_PRIVATE (image);
	GtkWidget               *widget = GTK_WIDGET (priv->viewer);
	gint                     border;

	border = image_viewer_is_frame_visible (priv->viewer) ?
		 FRAME_BORDER2 : 0;

	set_zoom_at_point (image,
			   zoom_level,
			   (widget->allocation.width - border) / 2,
			   (widget->allocation.height - border) / 2);
}


static void
halt_animation (ImageViewerImage *image)
{
	ImageViewerImagePrivate *priv = IMAGE_VIEWER_IMAGE_GET_PRIVATE (image);

	if (priv->anim_id != 0) {
		g_source_remove (priv->anim_id);
		priv->anim_id = 0;
	}
}


/* Public Methods */

ImageViewerImage*
image_viewer_image_new (ImageViewer* viewer,
			ImageLoader* loader)
{
	ImageViewerImage        *image;
	ImageViewerImagePrivate *priv;

	image = IMAGE_VIEWER_IMAGE (g_object_new (IMAGE_VIEWER_IMAGE_TYPE,
						  NULL));

	priv = IMAGE_VIEWER_IMAGE_GET_PRIVATE (image);

	/* Set the link the the ImageViewer widget -- use a construct-only property? */
	priv->viewer = viewer;

	/* Do stuff to set viewer and sync from loader */
	priv->file = image_loader_get_file (loader);
	priv->anim = image_loader_get_animation (loader);
	priv->static_pixbuf = image_loader_get_pixbuf (loader);
	if (priv->static_pixbuf)
		g_object_ref (priv->static_pixbuf);

	priv->is_animation = (priv->anim != NULL) &&
		! gdk_pixbuf_animation_is_static_image (priv->anim);

	if (priv->is_animation) {
		create_first_pixbuf (image);
		add_change_frame_timeout(image);
	}
	else if (priv->static_pixbuf != NULL) {
		priv->buffer = pixbuf_to_surface (priv->static_pixbuf);
	}

	return image;
}


gint
image_viewer_image_get_bps (ImageViewerImage* image)
{
	ImageViewerImagePrivate *priv;
	GdkPixbuf               *pixbuf;

	g_return_val_if_fail (image != NULL, 0);

	priv = IMAGE_VIEWER_IMAGE_GET_PRIVATE (image);

	if (priv->iter != NULL)
		pixbuf = gdk_pixbuf_animation_iter_get_pixbuf (priv->iter);
	else
		pixbuf = priv->static_pixbuf;

	g_return_val_if_fail (pixbuf != NULL, 0);

	return gdk_pixbuf_get_bits_per_sample (pixbuf);
}


GthFit
image_viewer_image_get_fit_mode (ImageViewerImage *image)
{
	ImageViewerImagePrivate *priv;

	g_return_val_if_fail (image != NULL, 0);

	priv = IMAGE_VIEWER_IMAGE_GET_PRIVATE (image);
	return priv->fit;
}


gboolean
image_viewer_image_get_has_alpha (ImageViewerImage* image)
{
	ImageViewerImagePrivate *priv;
	GdkPixbuf               *pixbuf;

	g_return_val_if_fail (image != NULL, 0);

	priv = IMAGE_VIEWER_IMAGE_GET_PRIVATE (image);

	if (priv->iter != NULL)
		pixbuf = gdk_pixbuf_animation_iter_get_pixbuf (priv->iter);
	else
		pixbuf = priv->static_pixbuf;

	g_return_val_if_fail (pixbuf != NULL, 0);

	return gdk_pixbuf_get_has_alpha (pixbuf);
}


gint
image_viewer_image_get_height (ImageViewerImage* image)
{
	ImageViewerImagePrivate *priv;

	g_return_val_if_fail (image != NULL, 0);

	priv = IMAGE_VIEWER_IMAGE_GET_PRIVATE (image);

	if (priv->anim != NULL)
		return gdk_pixbuf_animation_get_height (priv->anim);

	if (priv->static_pixbuf != NULL)
		return gdk_pixbuf_get_height (priv->static_pixbuf);

	return 0;
}


gboolean
image_viewer_image_get_is_animation (ImageViewerImage *image)
{
	ImageViewerImagePrivate *priv;

	g_return_val_if_fail (image != NULL, FALSE);

	priv = IMAGE_VIEWER_IMAGE_GET_PRIVATE (image);

	return priv->is_animation;
}


gchar*
image_viewer_image_get_path (ImageViewerImage* image)
{
	ImageViewerImagePrivate *priv;

	g_return_val_if_fail (image != NULL, NULL);

	priv = IMAGE_VIEWER_IMAGE_GET_PRIVATE (image);

	if (priv->file)
		return g_strdup (priv->file->path);
	else
		return NULL;
}


GdkPixbuf*
image_viewer_image_get_pixbuf (ImageViewerImage *image)
{
	ImageViewerImagePrivate *priv;

	g_return_val_if_fail (image != NULL, NULL);

	priv = IMAGE_VIEWER_IMAGE_GET_PRIVATE (image);

	if (priv->is_animation)
		return priv->anim_pixbuf;
	else
		return priv->static_pixbuf;
}


void
image_viewer_image_get_scroll_offsets (ImageViewerImage *image,
				       gdouble          *x,
				       gdouble          *y)
{
	ImageViewerImagePrivate *priv;

	g_return_if_fail (image != NULL);

	priv = IMAGE_VIEWER_IMAGE_GET_PRIVATE (image);

	*x = priv->x_offset;
	*y = priv->y_offset;
}


gint
image_viewer_image_get_width (ImageViewerImage* image)
{
	ImageViewerImagePrivate *priv;

	g_return_val_if_fail (image != NULL, 0);

	priv = IMAGE_VIEWER_IMAGE_GET_PRIVATE (image);

	if (priv->anim != NULL)
		return gdk_pixbuf_animation_get_width (priv->anim);

	if (priv->static_pixbuf != NULL)
		return gdk_pixbuf_get_width (priv->static_pixbuf);

	return 0;
}


gint
image_viewer_image_get_x_offset (ImageViewerImage* image)
{
	ImageViewerImagePrivate *priv;

	g_return_val_if_fail (image != NULL, 0);

	priv = IMAGE_VIEWER_IMAGE_GET_PRIVATE (image);

	return priv->x_offset;
}


gint
image_viewer_image_get_y_offset (ImageViewerImage* image)
{
	ImageViewerImagePrivate *priv;

	g_return_val_if_fail (image != NULL, 0);

	priv = IMAGE_VIEWER_IMAGE_GET_PRIVATE (image);

	return priv->y_offset;
}


gdouble
image_viewer_image_get_zoom_level (ImageViewerImage* image)
{
	ImageViewerImagePrivate *priv;

	g_return_val_if_fail (image != NULL, 0);

	priv = IMAGE_VIEWER_IMAGE_GET_PRIVATE (image);

	return priv->zoom_level;
}

void
image_viewer_image_get_zoomed_size (ImageViewerImage *image,
				    gdouble          *width,
				    gdouble          *height)
{
	ImageViewerImagePrivate *priv;
	gint w, h;

	g_return_if_fail (image != NULL);

	priv = IMAGE_VIEWER_IMAGE_GET_PRIVATE (image);

	w = image_viewer_image_get_width (image);
	h = image_viewer_image_get_height (image);

	*width  = floor ((double) w * priv->zoom_level);
	*height = floor ((double) h * priv->zoom_level);
}


void
image_viewer_image_scroll_relative (ImageViewerImage *image,
				    gdouble           delta_x,
				    gdouble           delta_y)
{
	ImageViewerImagePrivate *priv;

	g_return_if_fail (image != NULL);

	priv = IMAGE_VIEWER_IMAGE_GET_PRIVATE (image);

	priv->x_offset += delta_x;
	priv->y_offset += delta_y;

	clamp_offsets (image);

	gtk_widget_queue_draw (GTK_WIDGET (priv->viewer));
}


void
image_viewer_image_set_fit_mode (ImageViewerImage *image,
				 GthFit            fit_mode)
{
	ImageViewerImagePrivate *priv = IMAGE_VIEWER_IMAGE_GET_PRIVATE (image);
	GtkWidget               *widget = GTK_WIDGET (priv->viewer);
	gdouble                  x_level, y_level, new_level;
	gint                     gdk_width, gdk_height;
	gint                     img_width, img_height;
	gint                     border;

	priv->fit = fit_mode;

	border = image_viewer_is_frame_visible (priv->viewer) ?
		 FRAME_BORDER2 : 0;

	gdk_width = widget->allocation.width - border;
	gdk_height = widget->allocation.height - border;

	img_width = image_viewer_image_get_width   (image);
	img_height = image_viewer_image_get_height (image);

	if(img_width != 0 && img_height != 0) {
		x_level = (double) gdk_width  / img_width;
		y_level = (double) gdk_height / img_height;

		switch (priv->fit) {
		case GTH_FIT_SIZE:
			new_level = MIN (x_level, y_level);
			break;
		case GTH_FIT_SIZE_IF_LARGER:
			new_level = MIN (MIN (x_level, y_level), 1.0);
			break;
		case GTH_FIT_WIDTH:
			new_level = x_level;
			break;
		case GTH_FIT_WIDTH_IF_LARGER:
			new_level = MIN (x_level, 1.0);
			break;
		default:
			new_level = 1.0;
			break;
		}
	}

	if (priv->fit != GTH_FIT_NONE)
		set_zoom_centered (image, new_level);
}


void
image_viewer_image_set_scroll_offsets (ImageViewerImage* image,
				       gdouble           x_offset,
				       gdouble           y_offset)
{
	ImageViewerImagePrivate* priv;

	g_return_if_fail (image != NULL);

	priv = IMAGE_VIEWER_IMAGE_GET_PRIVATE (image);

	priv->x_offset = x_offset;
	priv->y_offset = y_offset;

	clamp_offsets (image);

	gtk_widget_queue_draw (GTK_WIDGET (priv->viewer));
}


void
image_viewer_image_set_zoom_level (ImageViewerImage* image,
				   gdouble           zoom_level)
{
	ImageViewerImagePrivate* priv;

	g_return_if_fail (image != NULL);

	priv = IMAGE_VIEWER_IMAGE_GET_PRIVATE (image);

	/* set_zoom doesn't reset fit, should be used by internal zoom-fit options. */
	priv->fit = GTH_FIT_NONE;

	set_zoom_centered (image, zoom_level);
}


void
image_viewer_image_start_animation (ImageViewerImage *image)
{
	ImageViewerImagePrivate *priv;

	g_return_if_fail (image != NULL);

	priv = IMAGE_VIEWER_IMAGE_GET_PRIVATE (image);

	if (! priv->is_animation) return;

	create_first_pixbuf (image);
	add_change_frame_timeout (image);
}


void
image_viewer_image_step_animation (ImageViewerImage *image)
{
	ImageViewerImagePrivate *priv;

	g_return_if_fail (image != NULL);

	priv = IMAGE_VIEWER_IMAGE_GET_PRIVATE (image);

	g_return_if_fail (priv->anim != NULL);

	change_frame_cb (image);
}


void
image_viewer_image_stop_animation (ImageViewerImage *image)
{
	g_return_if_fail (image != NULL);

	halt_animation (image);
}


void
image_viewer_image_zoom_at_point (ImageViewerImage *image,
				  gdouble           zoom_level,
				  gdouble           x,
				  gdouble           y)
{
	ImageViewerImagePrivate* priv;

	g_return_if_fail (image != NULL);

	priv = IMAGE_VIEWER_IMAGE_GET_PRIVATE (image);

	/* set_zoom doesn't reset fit, should be used by internal zoom-fit options. */
	priv->fit = GTH_FIT_NONE;

	set_zoom_at_point (image, zoom_level, x, y);
}


void
image_viewer_image_zoom_centered (ImageViewerImage *image,
				  gdouble           zoom_level)
{
	ImageViewerImagePrivate* priv;

	g_return_if_fail (image != NULL);

	priv = IMAGE_VIEWER_IMAGE_GET_PRIVATE (image);

	/* set_zoom doesn't reset fit, should be used by internal zoom-fit options. */
	priv->fit = GTH_FIT_NONE;

	set_zoom_centered (image, zoom_level);
}

void
image_viewer_image_zoom_in_at_point (ImageViewerImage *image,
				     gdouble           x,
				     gdouble           y)
{
	ImageViewerImagePrivate* priv;

	g_return_if_fail (image != NULL);

	priv = IMAGE_VIEWER_IMAGE_GET_PRIVATE (image);

	/* set_zoom doesn't reset fit, should be used by internal zoom-fit options. */
	priv->fit = GTH_FIT_NONE;

	set_zoom_at_point (image, get_next_zoom (priv->zoom_level), x, y);
}


void
image_viewer_image_zoom_in_centered (ImageViewerImage *image)
{
	ImageViewerImagePrivate* priv;

	g_return_if_fail (image != NULL);

	priv = IMAGE_VIEWER_IMAGE_GET_PRIVATE (image);

	/* set_zoom doesn't reset fit, should be used by internal zoom-fit options. */
	priv->fit = GTH_FIT_NONE;

	set_zoom_centered (image, get_next_zoom (priv->zoom_level));
}


void
image_viewer_image_zoom_out_at_point (ImageViewerImage *image,
				      gdouble           x,
				      gdouble           y)
{
	ImageViewerImagePrivate* priv;

	g_return_if_fail (image != NULL);

	priv = IMAGE_VIEWER_IMAGE_GET_PRIVATE (image);

	/* set_zoom doesn't reset fit, should be used by internal zoom-fit options. */
	priv->fit = GTH_FIT_NONE;

	set_zoom_at_point (image, get_prev_zoom (priv->zoom_level), x, y);
}


void
image_viewer_image_zoom_out_centered (ImageViewerImage *image)
{
	ImageViewerImagePrivate* priv;

	g_return_if_fail (image != NULL);

	priv = IMAGE_VIEWER_IMAGE_GET_PRIVATE (image);

	/* set_zoom doesn't reset fit, should be used by internal zoom-fit options. */
	priv->fit = GTH_FIT_NONE;

	set_zoom_centered (image, get_prev_zoom (priv->zoom_level));
}

/* image_viewer_image_paint */

cairo_surface_t*
pixbuf_to_surface (GdkPixbuf *pixbuf)
{
	/* based on gdk_cairo_set_source_pixbuf */
	gint    width;
	gint    height;
	guchar *gdk_pixels;
	int     gdk_rowstride;
	int     n_channels;
	int     cairo_stride;
	guchar *cairo_pixels;
	cairo_format_t format;
	cairo_surface_t *surface;
	static const cairo_user_data_key_t key;
	int j;

	g_return_val_if_fail (pixbuf != NULL, NULL);

	width = gdk_pixbuf_get_width (pixbuf);
	height = gdk_pixbuf_get_height (pixbuf);
	gdk_pixels = gdk_pixbuf_get_pixels (pixbuf);
	gdk_rowstride = gdk_pixbuf_get_rowstride (pixbuf);
	n_channels = gdk_pixbuf_get_n_channels (pixbuf);

	if (n_channels == 3)
		format = CAIRO_FORMAT_RGB24;
	else
		format = CAIRO_FORMAT_ARGB32;

	cairo_stride = cairo_format_stride_for_width (format, width);
	/* XXX store this buffer in the struct; reuse for multiple frames of animation? */
	cairo_pixels = g_malloc (cairo_stride * height);

	surface = cairo_image_surface_create_for_data (
			(unsigned char *)cairo_pixels,
			format,
			width, height, cairo_stride);
	cairo_surface_set_user_data (surface, &key,
			cairo_pixels, (cairo_destroy_func_t)g_free);

	for (j = height; j; j--)
	{
		guchar *p = gdk_pixels;
		guchar *q = cairo_pixels;

		if (n_channels == 3)
		{
			guchar *end = p + 3 * width;

			while (p < end)
			{
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
				q[0] = p[2];
				q[1] = p[1];
				q[2] = p[0];
#else	  
				q[1] = p[0];
				q[2] = p[1];
				q[3] = p[2];
#endif
				p += 3;
				q += 4;
			}
		}
		else
		{
			guchar *end = p + 4 * width;
			guint t1,t2,t3;

#define MULT(d,c,a,t) G_STMT_START { t = c * a + 0x7f; d = ((t >> 8) + t) >> 8; } G_STMT_END

			while (p < end)
			{
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
				MULT(q[0], p[2], p[3], t1);
				MULT(q[1], p[1], p[3], t2);
				MULT(q[2], p[0], p[3], t3);
				q[3] = p[3];
#else	  
				q[0] = p[3];
				MULT(q[1], p[0], p[3], t1);
				MULT(q[2], p[1], p[3], t2);
				MULT(q[3], p[2], p[3], t3);
#endif

				p += 4;
				q += 4;
			}

#undef MULT
		}

		gdk_pixels += gdk_rowstride;
		cairo_pixels += 4 * width;
	}

	return surface;
}


static cairo_surface_t*
checker_pattern (int width, int height, double color1, double color2)
{
	cairo_surface_t *surface;
	cairo_t         *cr;

	surface = cairo_image_surface_create (CAIRO_FORMAT_RGB24,
					      width * 2, height *2);
	cr = cairo_create (surface);

	cairo_set_source_rgb (cr, color1, color1, color1);
	cairo_paint (cr);

	cairo_set_source_rgb (cr, color2, color2, color2);
	cairo_rectangle (cr, 0,     0,      width, height);
	cairo_rectangle (cr, width, height, width, height);
	cairo_fill (cr);
	cairo_destroy (cr);

	return surface;
}


void
image_viewer_image_paint (ImageViewerImage *image,
			  cairo_t          *cr)
{
	ImageViewerImagePrivate *priv;
	GtkStyle                *style;

	gint                     border, border2;
	gdouble                  gdk_width, gdk_height;
	gdouble                  src_x, src_y;
	gdouble                  width, height;
	GthTranspType            transp_type;
	GthZoomQuality           zoom_quality;
	cairo_filter_t           filter;
	cairo_matrix_t           matrix;
	cairo_pattern_t         *pattern;	

	g_return_if_fail (image != NULL);

	priv   = IMAGE_VIEWER_IMAGE_GET_PRIVATE (image);
	style  = GTK_WIDGET (priv->viewer)->style;

	if (image_viewer_is_frame_visible (priv->viewer)) {
		border  = FRAME_BORDER;
		border2 = FRAME_BORDER2;
	} else {
		border = border2 = 0;
	}

	gdk_width = GTK_WIDGET (priv->viewer)->allocation.width - border2;
	gdk_height = GTK_WIDGET (priv->viewer)->allocation.height - border2;

	image_viewer_image_get_zoomed_size (image, &width, &height);

	transp_type = image_viewer_get_transp_type (priv->viewer);

	zoom_quality = image_viewer_get_zoom_quality (priv->viewer);
	if (zoom_quality == GTH_ZOOM_QUALITY_LOW)
		filter = CAIRO_FILTER_FAST;
	else
		filter = CAIRO_FILTER_GOOD;
/*		filter = CAIRO_FILTER_BEST;*/

	if (FLOAT_EQUAL (priv->zoom_level, 1.0))
		filter = CAIRO_FILTER_FAST;

	src_x = floor ((gdk_width  - width)  / 2 - priv->x_offset + border);
	src_y = floor ((gdk_height - height) / 2 - priv->y_offset + border);

	cairo_save (cr);

	if (image_viewer_image_get_has_alpha (image) &&
			transp_type != GTH_TRANSP_TYPE_NONE) {
		cairo_surface_t *checker;
		int              check_size;
		GthCheckType     check_type;
		double           color1, color2;

		switch (transp_type) {
		case GTH_TRANSP_TYPE_NONE:
			g_assert_not_reached ();
			break;

		case GTH_TRANSP_TYPE_BLACK:
			color1 = color2 = 0;
			break;

		case GTH_TRANSP_TYPE_WHITE:
			color1 = color2 = 1;
			break;

		case GTH_TRANSP_TYPE_CHECKED:
			check_type = image_viewer_get_check_type (priv->viewer);
			switch (check_type) {
			case GTH_CHECK_TYPE_DARK:
				color1 = 0;
				color2 = 0.2;
				break;

			case GTH_CHECK_TYPE_MIDTONE:
				color1 = 0.4;
				color2 = 0.6;
				break;

			case GTH_CHECK_TYPE_LIGHT:
				color1 = 0.8;
				color2 = 1;
				break;
			}
			break;
		}

		check_size = (int) image_viewer_get_check_size (priv->viewer);

		checker = checker_pattern (check_size, check_size, color1, color2);
		cairo_set_source_surface (cr, checker, -src_x, -src_y);
		cairo_pattern_set_extend (cairo_get_source (cr), CAIRO_EXTEND_REPEAT);
		cairo_pattern_set_filter (cairo_get_source (cr), CAIRO_FILTER_NEAREST);
		cairo_rectangle (cr, src_x, src_y, width, height);
		cairo_fill (cr);
		cairo_surface_destroy (checker);
	}

	pattern = cairo_pattern_create_for_surface (priv->buffer);
	cairo_matrix_init_scale (&matrix, 1/priv->zoom_level, 1/priv->zoom_level);
	cairo_matrix_translate (&matrix, -src_x, -src_y);
	cairo_pattern_set_matrix (pattern, &matrix);
	cairo_pattern_set_filter (pattern, filter);

	cairo_set_source (cr, pattern);
	cairo_pattern_destroy(pattern);
	cairo_paint (cr);

	if(border != 0) {
		cairo_set_antialias (cr, CAIRO_ANTIALIAS_NONE);
		cairo_set_line_width (cr, 1);
		gdk_cairo_set_source_color (cr, &style->dark[GTK_STATE_NORMAL]);
		cairo_move_to (cr, src_x - border, src_y + height + border);
		cairo_line_to (cr, src_x - border, src_y - border);
		cairo_line_to (cr, src_x + width + border, src_y - border);
		cairo_stroke (cr);
		gdk_cairo_set_source_color (cr, &style->light[GTK_STATE_NORMAL]);
		cairo_move_to (cr, src_x + width, src_y);
		cairo_line_to (cr, src_x + width, src_y + height);
		cairo_line_to (cr, src_x, src_y + height);
		cairo_stroke (cr);
	}

	cairo_restore (cr);

}
