/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001-2006 The Free Software Foundation, Inc.
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
#include "file-data.h"
#include "image-loader.h"
#include "image-viewer-enums.h"

G_BEGIN_DECLS

#define IMAGE_VIEWER_TYPE            (image_viewer_get_type ())
#define IMAGE_VIEWER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), IMAGE_VIEWER_TYPE, ImageViewer))
#define IMAGE_VIEWER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), IMAGE_VIEWER_TYPE, ImageViewerClass))
#define IS_IMAGE_VIEWER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IMAGE_VIEWER_TYPE))
#define IS_IMAGE_VIEWER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), IMAGE_VIEWER_TYPE))
#define IMAGE_VIEWER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), IMAGE_VIEWER_TYPE, ImageViewerClass))

typedef struct _ImageViewer       ImageViewer;
typedef struct _ImageViewerClass  ImageViewerClass;


struct _ImageViewer
{
	GtkWidget __parent;
};


struct _ImageViewerClass
{
	GtkWidgetClass __parent_class;

	/* -- Signals -- */

	void (* clicked)                (ImageViewer        *viewer);
	void (* image_loaded)           (ImageViewer        *viewer);
	void (* image_error)            (ImageViewer        *viewer);
	void (* image_progress)         (ImageViewer        *viewer,
					 float               progress);
	void (* zoom_changed)           (ImageViewer        *viewer);
	void (* set_scroll_adjustments) (GtkWidget          *widget,
                                         GtkAdjustment      *hadj,
                                         GtkAdjustment      *vadj);
	void (* repainted)              (ImageViewer        *viewer);
	void (* mouse_wheel_scroll)	(ImageViewer        *viewer,
	 				 GdkScrollDirection  direction);

	/* -- Key binding signals -- */

	void (*scroll)                  (GtkWidget          *widget,
					 GtkScrollType       x_scroll_type,
					 GtkScrollType       y_scroll_type);
	void (* zoom_in)                (ImageViewer        *viewer);
	void (* zoom_out)               (ImageViewer        *viewer);
	void (* set_zoom)               (ImageViewer        *viewer,
					 gdouble             zoom);
	void (* set_fit_mode)           (ImageViewer        *viewer,
					 GthFit              fit_mode);
};


GType          image_viewer_get_type                 (void);
GtkWidget*     image_viewer_new                      (void);

/* viewer content. */

void           image_viewer_load_image_from_uri      (ImageViewer     *viewer,
						      const char      *path);
void           image_viewer_load_image                (ImageViewer     *viewer,
			 			      FileData        *file);						      
void           image_viewer_load_from_pixbuf_loader  (ImageViewer     *viewer,
						      GdkPixbufLoader *loader);
void           image_viewer_load_from_image_loader   (ImageViewer     *viewer,
						      ImageLoader     *loader);
void           image_viewer_set_pixbuf               (ImageViewer     *viewer,
						      GdkPixbuf       *pixbuf);
void           image_viewer_set_void                 (ImageViewer     *viewer);
gboolean       image_viewer_is_void                  (ImageViewer     *viewer);
void           image_viewer_update_view              (ImageViewer     *viewer);

/* image info. */

char*          image_viewer_get_image_filename       (ImageViewer     *viewer);
int            image_viewer_get_image_width          (ImageViewer     *viewer);
int            image_viewer_get_image_height         (ImageViewer     *viewer);
int            image_viewer_get_image_bps            (ImageViewer     *viewer);
gboolean       image_viewer_get_has_alpha            (ImageViewer     *viewer);
GdkPixbuf *    image_viewer_get_current_pixbuf       (ImageViewer     *viewer);

/* animation. */

void           image_viewer_start_animation          (ImageViewer     *viewer);
void           image_viewer_stop_animation           (ImageViewer     *viewer);
void           image_viewer_step_animation           (ImageViewer     *viewer);
gboolean       image_viewer_get_play_animation       (ImageViewer     *viewer);
gboolean       image_viewer_is_animation             (ImageViewer     *viewer);
gboolean       image_viewer_is_playing_animation     (ImageViewer     *viewer);

/* zoom. */

void           image_viewer_set_zoom                 (ImageViewer     *viewer,
						      gdouble          zoom);
gdouble        image_viewer_get_zoom                 (ImageViewer     *viewer);
void           image_viewer_set_zoom_quality         (ImageViewer     *viewer,
						      GthZoomQuality   quality);
GthZoomQuality image_viewer_get_zoom_quality         (ImageViewer     *viewer);
void           image_viewer_set_zoom_change          (ImageViewer     *viewer,
						      GthZoomChange    zoom_change);
GthZoomChange  image_viewer_get_zoom_change          (ImageViewer     *viewer);
void           image_viewer_zoom_in                  (ImageViewer     *viewer);
void           image_viewer_zoom_out                 (ImageViewer     *viewer);
void           image_viewer_set_fit_mode             (ImageViewer     *viewer,
						      GthFit           fit_mode);
GthFit         image_viewer_get_fit_mode             (ImageViewer     *viewer);

/* visualization options. */

void           image_viewer_set_transp_type          (ImageViewer     *viewer,
						      GthTranspType    transp_type);
GthTranspType  image_viewer_get_transp_type          (ImageViewer     *viewer);
void           image_viewer_set_check_type           (ImageViewer     *viewer,
						      GthCheckType     check_type);
GthCheckType   image_viewer_get_check_type           (ImageViewer     *viewer);
void           image_viewer_set_check_size           (ImageViewer     *view,
						      GthCheckSize     check_size);
GthCheckSize   image_viewer_get_check_size           (ImageViewer     *viewer);

/* misc. */

void           image_viewer_clicked                  (ImageViewer     *viewer);
void           image_viewer_size                     (ImageViewer     *viewer,
						      int              width,
						      int              height);
void           image_viewer_set_black_background     (ImageViewer     *viewer,
						      gboolean         set_black);
gboolean       image_viewer_is_black_background      (ImageViewer     *viewer);

/* Scrolling. */

void           image_viewer_scroll_to                (ImageViewer     *viewer,
						      int              x_offset,
						      int              y_offset);
void           image_viewer_scroll_step_x            (ImageViewer     *viewer,
						      gboolean         increment);
void           image_viewer_scroll_step_y            (ImageViewer     *viewer,
						      gboolean         increment);
void           image_viewer_scroll_page_x            (ImageViewer     *viewer,
						      gboolean         increment);
void           image_viewer_scroll_page_y            (ImageViewer     *viewer,
						      gboolean         increment);
void           image_viewer_get_scroll_offset        (ImageViewer     *viewer,
						      int             *x,
						      int             *y);
void           image_viewer_set_reset_scrollbars     (ImageViewer     *viewer,
  						      gboolean         reset);
gboolean       image_viewer_get_reset_scrollbars     (ImageViewer     *viewer);

/* Cursor. */

void           image_viewer_show_cursor              (ImageViewer     *viewer);
void           image_viewer_hide_cursor              (ImageViewer     *viewer);
gboolean       image_viewer_is_cursor_visible        (ImageViewer     *viewer);

/* Frame. */

void           image_viewer_show_frame               (ImageViewer     *viewer);
void           image_viewer_hide_frame               (ImageViewer     *viewer);
gboolean       image_viewer_is_frame_visible         (ImageViewer     *viewer);

G_END_DECLS

#endif /* IMAGE_VIEWER_H */
