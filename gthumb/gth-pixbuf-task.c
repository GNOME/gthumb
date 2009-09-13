/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001, 2009 Free Software Foundation, Inc.
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

#include <glib.h>
#include "gth-pixbuf-task.h"


#define PROGRESS_DELAY 200 /* delay between progress notifications */


static gpointer parent_class = NULL;


static void
release_pixbufs (GthPixbufTask *pixbuf_task)
{
	if (pixbuf_task->src != NULL) {
		g_object_unref (pixbuf_task->src);
		pixbuf_task->src = NULL;
	}

	if (pixbuf_task->dest != NULL) {
		g_object_unref (pixbuf_task->dest);
		pixbuf_task->dest = NULL;
	}
}


static void
gth_pixbuf_task_finalize (GObject *object)
{
	GthPixbufTask *pixbuf_task;

	g_return_if_fail (GTH_IS_PIXBUF_TASK (object));
	pixbuf_task = GTH_PIXBUF_TASK (object);

	if (pixbuf_task->progress_event != 0) {
		g_source_remove (pixbuf_task->progress_event);
		pixbuf_task->progress_event = 0;
	}

	release_pixbufs (pixbuf_task);
	if (pixbuf_task->free_data_func != NULL)
		(*pixbuf_task->free_data_func) (pixbuf_task);

	g_mutex_free (pixbuf_task->data_mutex);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static gboolean
update_progress (gpointer data)
{
	GthPixbufTask *pixbuf_task = data;
	gboolean       interrupt;
	int            line;

	g_mutex_lock (pixbuf_task->data_mutex);
	interrupt = pixbuf_task->interrupt;
	line = pixbuf_task->line;
	g_mutex_unlock (pixbuf_task->data_mutex);

	if ((line >= pixbuf_task->height) || interrupt) {
		GError *error = NULL;

		g_source_remove (pixbuf_task->progress_event);
		pixbuf_task->progress_event = 0;

		if (interrupt)
			error = g_error_new_literal (GTH_TASK_ERROR, GTH_TASK_ERROR_CANCELLED, "");

		if (pixbuf_task->release_func != NULL)
			(*pixbuf_task->release_func) (pixbuf_task, error);

		gth_task_completed (GTH_TASK (pixbuf_task), error);

		return FALSE;
	}

	gth_task_progress (GTH_TASK (pixbuf_task),
			   pixbuf_task->description,
			   NULL,
			   FALSE,
			   (double) line / pixbuf_task->height);

	return TRUE;
}


static gboolean
execute_step (GthPixbufTask *pixbuf_task)
{
	int      dir = 1;
	gboolean interrupt;
	int      line;

	g_mutex_lock (pixbuf_task->data_mutex);
	interrupt = pixbuf_task->interrupt;
	line = pixbuf_task->line;
	g_mutex_unlock (pixbuf_task->data_mutex);

	if ((line >= pixbuf_task->height) || interrupt)
		return FALSE;

	pixbuf_task->src_pixel = pixbuf_task->src_line;
	pixbuf_task->src_line += pixbuf_task->rowstride;

	pixbuf_task->dest_pixel = pixbuf_task->dest_line;
	pixbuf_task->dest_line += pixbuf_task->rowstride;

	if (! pixbuf_task->ltr) { /* right to left */
		int ofs = (pixbuf_task->width - 1) * pixbuf_task->bytes_per_pixel;
		pixbuf_task->src_pixel += ofs;
		pixbuf_task->dest_pixel += ofs;
		dir = -1;
		pixbuf_task->column = pixbuf_task->width - 1;
	}
	else
		pixbuf_task->column = 0;

	pixbuf_task->line_step = 0;
	while (pixbuf_task->line_step < pixbuf_task->width) {
		(*pixbuf_task->step_func) (pixbuf_task);
		pixbuf_task->src_pixel += dir * pixbuf_task->bytes_per_pixel;
		pixbuf_task->dest_pixel += dir * pixbuf_task->bytes_per_pixel;
		pixbuf_task->column += dir;
		pixbuf_task->line_step++;
	}

	g_mutex_lock (pixbuf_task->data_mutex);
	pixbuf_task->line++;
	g_mutex_unlock (pixbuf_task->data_mutex);

	return TRUE;
}


static gpointer
execute_task (gpointer user_data)
{
	GthPixbufTask *pixbuf_task = user_data;

	g_mutex_lock (pixbuf_task->data_mutex);
	pixbuf_task->line = 0;
	g_mutex_unlock (pixbuf_task->data_mutex);

	if (pixbuf_task->init_func != NULL)
		(*pixbuf_task->init_func) (pixbuf_task);

	while (execute_step (pixbuf_task)) /* void */;

	return NULL;
}


static void
gth_pixbuf_task_exec (GthTask *task)
{
	GthPixbufTask *pixbuf_task;

	pixbuf_task = GTH_PIXBUF_TASK (task);

	g_return_if_fail (pixbuf_task->src != NULL);

	g_thread_create (execute_task, pixbuf_task, FALSE, NULL);
	pixbuf_task->progress_event = g_timeout_add (PROGRESS_DELAY, update_progress, pixbuf_task);
}


static void
gth_pixbuf_task_cancel (GthTask *task)
{
	GthPixbufTask *pixbuf_task;

	g_return_if_fail (GTH_IS_PIXBUF_TASK (task));

	pixbuf_task = GTH_PIXBUF_TASK (task);

	g_mutex_lock (pixbuf_task->data_mutex);
	pixbuf_task->interrupt = TRUE;
	g_mutex_unlock (pixbuf_task->data_mutex);
}


static void
gth_pixbuf_task_class_init (GthPixbufTaskClass *class)
{
	GObjectClass *object_class;
	GthTaskClass *task_class;

	parent_class = g_type_class_peek_parent (class);

	object_class = G_OBJECT_CLASS (class);
	object_class->finalize = gth_pixbuf_task_finalize;

	task_class = GTH_TASK_CLASS (class);
	task_class->exec = gth_pixbuf_task_exec;
	task_class->cancel = gth_pixbuf_task_cancel;
}


static void
gth_pixbuf_task_init (GthPixbufTask *pixbuf_task)
{
	pixbuf_task->src = NULL;
	pixbuf_task->dest = NULL;
	pixbuf_task->data = NULL;

	pixbuf_task->init_func = NULL;
	pixbuf_task->step_func = NULL;
	pixbuf_task->release_func = NULL;
	pixbuf_task->free_data_func = NULL;

	pixbuf_task->src_line = NULL;
	pixbuf_task->src_pixel = NULL;
	pixbuf_task->dest_line = NULL;
	pixbuf_task->dest_pixel = NULL;

	pixbuf_task->ltr = TRUE;

	pixbuf_task->progress_event = 0;
	pixbuf_task->line = 0;
	pixbuf_task->interrupt = FALSE;

	pixbuf_task->data_mutex = g_mutex_new ();
}


GType
gth_pixbuf_task_get_type (void)
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (GthPixbufTaskClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_pixbuf_task_class_init,
			NULL,
			NULL,
			sizeof (GthPixbufTask),
			0,
			(GInstanceInitFunc) gth_pixbuf_task_init
		};

		type = g_type_register_static (GTH_TYPE_TASK,
					       "GthPixbufTask",
					       &type_info,
					       0);
	}

	return type;
}


GthTask *
gth_pixbuf_task_new (const char     *description,
		     GdkPixbuf      *src,
		     GdkPixbuf      *dest,
		     PixbufOpFunc    init_func,
		     PixbufOpFunc    step_func,
		     PixbufDoneFunc  release_func,
		     gpointer        data)
{
	GthPixbufTask *pixbuf_task;

	pixbuf_task = GTH_PIXBUF_TASK (g_object_new (GTH_TYPE_PIXBUF_TASK, NULL));

	pixbuf_task->description = description;
	pixbuf_task->init_func = init_func;
	pixbuf_task->step_func = step_func;
	pixbuf_task->release_func = release_func;
	pixbuf_task->data = data;

	gth_pixbuf_task_set_pixbufs (pixbuf_task, src, dest);

	return (GthTask *) pixbuf_task;
}


void
gth_pixbuf_task_set_pixbufs (GthPixbufTask *pixbuf_task,
			     GdkPixbuf     *src,
			     GdkPixbuf     *dest)
{
	if (src == NULL)
		return;

	/* NOTE that src and dest MAY be the same pixbuf! */

	g_return_if_fail (GDK_IS_PIXBUF (src));
	if (dest != NULL) {
		g_return_if_fail (GDK_IS_PIXBUF (dest));
		g_return_if_fail (gdk_pixbuf_get_has_alpha (src) == gdk_pixbuf_get_has_alpha (dest));
		g_return_if_fail (gdk_pixbuf_get_width (src) == gdk_pixbuf_get_width (dest));
		g_return_if_fail (gdk_pixbuf_get_height (src) == gdk_pixbuf_get_height (dest));
		g_return_if_fail (gdk_pixbuf_get_colorspace (src) == gdk_pixbuf_get_colorspace (dest));
	}

	release_pixbufs (pixbuf_task);

	g_object_ref (src);
	pixbuf_task->src = src;

	pixbuf_task->has_alpha       = gdk_pixbuf_get_has_alpha (src);
	pixbuf_task->bytes_per_pixel = pixbuf_task->has_alpha ? 4 : 3;
	pixbuf_task->width           = gdk_pixbuf_get_width (src);
	pixbuf_task->height          = gdk_pixbuf_get_height (src);
	pixbuf_task->rowstride       = gdk_pixbuf_get_rowstride (src);
	pixbuf_task->src_line        = gdk_pixbuf_get_pixels (src);

	if (dest != NULL) {
		g_object_ref (dest);
		pixbuf_task->dest = dest;
		pixbuf_task->dest_line = gdk_pixbuf_get_pixels (dest);
	}
}
