/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2005 Free Software Foundation, Inc.
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

#ifndef GTH_WINDOW_H
#define GTH_WINDOW_H

#include <gtk/gtk.h>
#include "gth-pixbuf-op.h"
#include "image-viewer.h"
#include "gthumb-preloader.h"
#include "file-data.h"


#define GTH_TYPE_WINDOW              (gth_window_get_type ())
#define GTH_WINDOW(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_TYPE_WINDOW, GthWindow))
#define GTH_WINDOW_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_WINDOW_TYPE, GthWindowClass))
#define GTH_IS_WINDOW(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_TYPE_WINDOW))
#define GTH_IS_WINDOW_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_TYPE_WINDOW))
#define GTH_WINDOW_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_TYPE_WINDOW, GthWindowClass))

typedef struct _GthWindow            GthWindow;
typedef struct _GthWindowClass       GthWindowClass;
typedef struct _GthWindowPrivateData GthWindowPrivateData;

struct _GthWindow
{
	GtkWindow __parent;
	GthWindowPrivateData *priv;
};

struct _GthWindowClass
{
	GtkWindowClass __parent_class;

	/*<virtual functions>*/

	void             (*close)                         (GthWindow   *window);
	ImageViewer *    (*get_image_viewer)              (GthWindow   *window);
	GThumbPreloader *(*get_preloader)                 (GthWindow   *window);
	FileData *       (*get_image_data)                (GthWindow   *window);
	void             (*set_image_modified)            (GthWindow   *window,
							   gboolean     value);
	gboolean         (*get_image_modified)            (GthWindow   *window);
	void             (*save_pixbuf)                   (GthWindow   *window,
							   GdkPixbuf   *pixbuf,
							   FileData    *file);
	void             (*exec_pixbuf_op)                (GthWindow   *window,
							   GthPixbufOp *pixop,
							   gboolean     preview);
	void             (*reload_current_image)          (GthWindow   *window);
	void             (*update_current_image_metadata) (GthWindow   *window);
	GList *          (*get_file_list_selection)       (GthWindow   *window);
	GList *          (*get_file_list_selection_as_fd) (GthWindow   *window);
	void             (*set_animation)                 (GthWindow   *window,
							   gboolean     value);
	gboolean         (*get_animation)                 (GthWindow   *window);
	void             (*step_animation)                (GthWindow   *window);
	void             (*set_fullscreen)                (GthWindow   *window,
							   gboolean     value);
	void             (*set_slideshow)                 (GthWindow   *window,
							   gboolean     value);
};

GType             gth_window_get_type                     (void);
void              gth_window_close                        (GthWindow   *window);
ImageViewer *     gth_window_get_image_viewer             (GthWindow   *window);
GThumbPreloader * gth_window_get_preloader                (GthWindow   *window);
FileData *        gth_window_get_image_data               (GthWindow   *window);
const char *      gth_window_get_image_filename           (GthWindow   *window);
void              gth_window_set_image_modified           (GthWindow   *window,
							   gboolean     value);
gboolean          gth_window_get_image_modified           (GthWindow   *window);
GdkPixbuf     *   gth_window_get_image_pixbuf             (GthWindow   *window);
void              gth_window_set_image_pixbuf             (GthWindow   *window,
							   GdkPixbuf   *pixbuf);
void              gth_window_save_pixbuf                  (GthWindow   *window,
							   GdkPixbuf   *pixbuf,
							   FileData    *file);
void              gth_window_exec_pixbuf_op               (GthWindow   *window,
							   GthPixbufOp *pixop,
							   gboolean     preview);

void              gth_window_undo                         (GthWindow   *window);
void              gth_window_redo                         (GthWindow   *window);
void              gth_window_clear_undo_history           (GthWindow   *window);
gboolean          gth_window_get_can_undo                 (GthWindow   *window);
gboolean          gth_window_get_can_redo                 (GthWindow   *window);

typedef enum {
	GTH_WINDOW_MENUBAR,
	GTH_WINDOW_TOOLBAR,
	GTH_WINDOW_CONTENTS,
	GTH_WINDOW_STATUSBAR,
} GthWindowArea;

void           gth_window_attach                         (GthWindow     *window,
							  GtkWidget     *child,
							  GthWindowArea  area);
void           gth_window_set_tags_dlg                   (GthWindow   *window,
							  GtkWidget   *dialog);
GtkWidget *    gth_window_get_tags_dlg                   (GthWindow   *window);
void           gth_window_set_comment_dlg                (GthWindow   *window,
							  GtkWidget   *dialog);
GtkWidget *    gth_window_get_comment_dlg                (GthWindow   *window);
void           gth_window_update_comment_tags_dlg        (GthWindow   *window);
void           gth_window_reload_current_image           (GthWindow   *window);
void           gth_window_update_current_image_metadata  (GthWindow   *window);
GList *        gth_window_get_file_list_selection        (GthWindow   *window);
GList *        gth_window_get_file_list_selection_as_fd  (GthWindow   *window);
void           gth_window_set_animation                  (GthWindow   *window,
							  gboolean     value);
gboolean       gth_window_get_animation                  (GthWindow   *window);
void           gth_window_step_animation                 (GthWindow   *window);
void           gth_window_edit_comment                   (GthWindow   *window);
void           gth_window_edit_tags                      (GthWindow   *window);
void           gth_window_set_fullscreen                 (GthWindow   *window,
							  gboolean     value);
gboolean       gth_window_get_fullscreen                 (GthWindow   *window);
void           gth_window_set_slideshow                  (GthWindow   *window,
							  gboolean     value);

/**/

int            gth_window_get_n_windows                  (void);
GList *        gth_window_get_window_list                (void);

#endif /* GTH_WINDOW_H */
