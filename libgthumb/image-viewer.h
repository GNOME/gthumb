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

#ifndef IMAGE_VIEWER_H
#define IMAGE_VIEWER_H

#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk-pixbuf/gdk-pixbuf-loader.h>
#include "image-loader.h"

G_BEGIN_DECLS

#define IMAGE_VIEWER_TYPE            (image_viewer_get_type ())
#define IMAGE_VIEWER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), IMAGE_VIEWER_TYPE, ImageViewer))
#define IMAGE_VIEWER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), IMAGE_VIEWER_TYPE, ImageViewerClass))
#define IS_IMAGE_VIEWER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IMAGE_VIEWER_TYPE))
#define IS_IMAGE_VIEWER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), IMAGE_VIEWER_TYPE))
#define IMAGE_VIEWER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), IMAGE_VIEWER_TYPE, ImageViewerClass))

typedef struct _ImageViewer       ImageViewer;
typedef struct _ImageViewerClass  ImageViewerClass;

#define FRAME_BORDER    1
#define FRAME_BORDER2   2    /* FRAME_BORDER * 2 */

typedef enum { /*< skip >*/
	ZOOM_QUALITY_HIGH = 0,
	ZOOM_QUALITY_LOW 
} ZoomQuality;


typedef enum { /*< skip >*/
	ZOOM_CHANGE_ACTUAL_SIZE = 0,
	ZOOM_CHANGE_FIT,
	ZOOM_CHANGE_KEEP_PREV,
	ZOOM_CHANGE_FIT_IF_LARGER
} ZoomChange;


/* transparenty type. */
typedef enum { /*< skip >*/
        TRANSP_TYPE_WHITE,
        TRANSP_TYPE_NONE,
        TRANSP_TYPE_BLACK,
	TRANSP_TYPE_CHECKED
} TranspType;


typedef enum { /*< skip >*/
        CHECK_TYPE_LIGHT,
        CHECK_TYPE_MIDTONE,
        CHECK_TYPE_DARK
} CheckType;


typedef enum { /*< skip >*/
        CHECK_SIZE_SMALL  = 4,
        CHECK_SIZE_MEDIUM = 8,
        CHECK_SIZE_LARGE  = 16
} CheckSize;


struct _ImageViewer
{
	GtkWidget __parent;

	/*< private >*/

	gboolean         is_animation;
	gboolean         play_animation;
	gboolean         rendering;
	gboolean         cursor_visible;

	TranspType       transp_type;
	CheckType        check_type;
	gint             check_size;
	guint32          check_color1;
	guint32          check_color2;

	guint            anim_id;
	GdkPixbuf       *frame_pixbuf;
	int              frame_delay;

	ImageLoader            *loader;
	GdkPixbufAnimation     *anim;
	GdkPixbufAnimationIter *iter;
	GTimeVal                time;        /* Timer used to get the right 
					      * frame. */

	GdkCursor       *cursor;
	GdkCursor       *cursor_void;

	double           zoom_level;
	guint            zoom_quality : 1;   /* A ZoomQualityType value. */
	guint            zoom_change : 2;    /* A ZoomChangeType value. */

	gboolean         zoom_fit;           /* Whether the fit to window zoom
					      * is selected. */

	gboolean         zoom_fit_if_larger; 

	gboolean         doing_zoom_fit;     /* Whether we are performing
					      * a zoom to fit the window. */
	gboolean         is_void;            /* If TRUE do not show anything. 
					      *	Itis resetted to FALSE we an 
					      * image is loaded. */

	gint             x_offset;           /* Scroll offsets. */
	gint             y_offset;

	gboolean         pressed;
	gboolean         dragging;
	gboolean         double_click;
	gboolean         just_focused;
	gint             drag_x;
	gint             drag_y;
	gint             drag_realx;
	gint             drag_realy;

	GdkPixbuf       *area_pixbuf;
	gint             area_max_width;
	gint             area_max_height;
        int              area_bps;
	GdkColorspace    area_color_space;

	GtkAdjustment   *vadj, *hadj;

	gboolean         black_bg;

	gboolean         skip_zoom_change;
	gboolean         skip_size_change;

	gboolean         next_scroll_repaint; /* used in fullscreen mode to delete the 
					       * comment before scrolling. */
};


struct _ImageViewerClass
{
	GtkWidgetClass __parent_class;

	/* -- Signals -- */

	void (* clicked)                (ImageViewer *viewer);
	void (* image_loaded)           (ImageViewer *viewer);
	void (* zoom_changed)           (ImageViewer *viewer);
	void (* size_changed)           (ImageViewer *viewer);
	void (* set_scroll_adjustments) (GtkWidget *widget,
                                         GtkAdjustment *hadj,
                                         GtkAdjustment *vadj);
	void (* repainted)              (ImageViewer *viewer);

	/* -- Key binding signals -- */

	void (*scroll)                  (GtkWidget *   widget,
					 GtkScrollType x_scroll_type,
					 GtkScrollType y_scroll_type);
};


GType          image_viewer_get_type           (void);

GtkWidget*     image_viewer_new                (void);

/* viewer content. */
void           image_viewer_load_image         (ImageViewer *viewer, 
						const gchar *path);

void           image_viewer_load_from_pixbuf_loader (ImageViewer     *viewer, 
						     GdkPixbufLoader *loader);

void           image_viewer_load_from_image_loader  (ImageViewer *viewer, 
						     ImageLoader *loader);

void           image_viewer_set_pixbuf         (ImageViewer *viewer,
						GdkPixbuf   *pixbuf);

void           image_viewer_set_void           (ImageViewer *viewer);

gboolean       image_viewer_is_void            (ImageViewer *viewer);

void           image_viewer_update_view        (ImageViewer *viewer);

/* image info. */
gchar*         image_viewer_get_image_filename (ImageViewer *viewer);

gint           image_viewer_get_image_width    (ImageViewer *viewer);

gint           image_viewer_get_image_height   (ImageViewer *viewer);

gint           image_viewer_get_image_bps      (ImageViewer *viewer);

gboolean       image_viewer_get_has_alpha      (ImageViewer *viewer);

GdkPixbuf *    image_viewer_get_current_pixbuf (ImageViewer *viewer);

/* animation. */
void           image_viewer_start_animation    (ImageViewer *viewer);

void           image_viewer_stop_animation     (ImageViewer *viewer);

void           image_viewer_step_animation     (ImageViewer *viewer);

gboolean       image_viewer_is_animation       (ImageViewer *viewer);

gboolean       image_viewer_is_playing_animation (ImageViewer *viewer);

/* zoom. */
void           image_viewer_set_zoom           (ImageViewer *viewer, 
						gdouble zoom);

gdouble        image_viewer_get_zoom           (ImageViewer *viewer); 

void           image_viewer_set_zoom_quality   (ImageViewer *viewer,
						ZoomQuality quality);

ZoomQuality    image_viewer_get_zoom_quality   (ImageViewer *viewer);

void           image_viewer_set_zoom_change    (ImageViewer *viewer,
						ZoomChange zoom_change);

ZoomChange     image_viewer_get_zoom_change    (ImageViewer *viewer);

void           image_viewer_zoom_in            (ImageViewer *viewer);

void           image_viewer_zoom_out           (ImageViewer *viewer);

void           image_viewer_zoom_to_fit        (ImageViewer *viewer);

gboolean       image_viewer_is_zoom_to_fit     (ImageViewer *viewer);

void           image_viewer_zoom_to_fit_if_larger    (ImageViewer *viewer);

gboolean       image_viewer_is_zoom_to_fit_if_larger (ImageViewer *viewer);

/* visualization options. */
void           image_viewer_set_transp_type    (ImageViewer *viewer, 
						TranspType transp_type);

TranspType     image_viewer_get_transp_type    (ImageViewer *viewer);

void           image_viewer_set_check_type     (ImageViewer *viewer, 
						CheckType check_type);

CheckType      image_viewer_get_check_type     (ImageViewer *viewer);

void           image_viewer_set_check_size     (ImageViewer *view, 
						CheckSize check_size);

CheckSize      image_viewer_get_check_size     (ImageViewer *viewer);

/* misc. */
void           image_viewer_clicked            (ImageViewer *viewer);

void           image_viewer_size               (ImageViewer *viewer,
						gint width,
						gint height);

void           image_viewer_set_black_background (ImageViewer *viewer,
						  gboolean set_black);

gboolean       image_viewer_is_black_background (ImageViewer *viewer);


/* Scrolling. */
void           image_viewer_scroll_to          (ImageViewer *viewer,
						gint x_offset,
						gint y_offset);

void           image_viewer_scroll_step_x      (ImageViewer *viewer,
						gboolean increment);

void           image_viewer_scroll_step_y      (ImageViewer *viewer,
						gboolean increment);

void           image_viewer_scroll_page_x      (ImageViewer *viewer,
						gboolean increment);

void           image_viewer_scroll_page_y      (ImageViewer *viewer,
						gboolean increment);

/* Cursor. */
void           image_viewer_show_cursor        (ImageViewer *viewer);

void           image_viewer_hide_cursor        (ImageViewer *viewer);

gboolean       image_viewer_is_cursor_visible  (ImageViewer *viewer);

G_END_DECLS

#endif /* IMAGE_VIEWER_H */
