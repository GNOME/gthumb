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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <glib.h>
#include "glib-utils.h"
#include "gth-pixbuf-task.h"


struct _GthPixbufTaskPrivate {
	const char     *description;
	PixbufOpFunc    init_func;
	PixbufOpFunc    step_func;
	PixbufDataFunc  release_func;
	gboolean        single_step;
	GDestroyNotify  destroy_data_func;
};


static gpointer parent_class = NULL;


static void
release_pixbufs (GthPixbufTask *self)
{
	if (self->src != NULL) {
		g_object_unref (self->src);
		self->src = NULL;
	}

	if (self->dest != NULL) {
		g_object_unref (self->dest);
		self->dest = NULL;
	}
}


static void
gth_pixbuf_task_finalize (GObject *object)
{
	GthPixbufTask *self;

	g_return_if_fail (GTH_IS_PIXBUF_TASK (object));
	self = GTH_PIXBUF_TASK (object);

	release_pixbufs (self);
	if (self->priv->destroy_data_func != NULL)
		(*self->priv->destroy_data_func) (self->data);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
before_execute_pixbuf_task (GthAsyncTask *base,
			    gpointer      user_data)
{
	GthPixbufTask *self = GTH_PIXBUF_TASK (base);

	gth_task_progress (GTH_TASK (self),
			   self->priv->description,
			   NULL,
			   TRUE,
			   0.0);
}


static gboolean
execute_step (GthPixbufTask *self)
{
	gboolean terminated;
	gboolean cancelled;
	double   progress;
	int      dir;

	gth_async_task_get_data (GTH_ASYNC_TASK (self), &terminated, &cancelled, NULL);
	if (self->line >= self->height)
		terminated = TRUE;
	progress = (double) self->line / self->height;
	gth_async_task_set_data (GTH_ASYNC_TASK (self), &terminated, NULL, &progress);

	if (terminated || cancelled)
		return FALSE;

	self->src_pixel = self->src_line;
	self->src_line += self->rowstride;

	self->dest_pixel = self->dest_line;
	self->dest_line += self->rowstride;

	if (! self->ltr) { /* right to left */
		int ofs = (self->width - 1) * self->bytes_per_pixel;
		self->src_pixel += ofs;
		self->dest_pixel += ofs;
		dir = -1;
		self->column = self->width - 1;
	}
	else {
		dir = 1;
		self->column = 0;
	}

	self->line_step = 0;
	while (self->line_step < self->width) {
		if (self->priv->step_func != NULL)
			(*self->priv->step_func) (self);
		self->src_pixel += dir * self->bytes_per_pixel;
		self->dest_pixel += dir * self->bytes_per_pixel;
		self->column += dir;
		self->line_step++;
	}

	self->line++;

	return TRUE;
}


static gpointer
execute_pixbuf_task (GthAsyncTask *base,
		     gpointer      user_data)
{
	GthPixbufTask *self = GTH_PIXBUF_TASK (base);

	self->line = 0;
	if (self->priv->init_func != NULL)
		(*self->priv->init_func) (self);

	if (self->dest != NULL)
		self->dest_line = gdk_pixbuf_get_pixels (self->dest);

	if (self->priv->single_step) {
		gboolean terminated;

		if (self->priv->step_func != NULL)
			(*self->priv->step_func) (self);
		terminated = TRUE;
		gth_async_task_set_data (GTH_ASYNC_TASK (self), &terminated, NULL, NULL);
	}
	else {
		while (execute_step (self))
			/* void */;
	}

	return NULL;
}


static void
after_execute_pixbuf_task (GthAsyncTask *base,
			   GError       *error,
			   gpointer      user_data)
{
	GthPixbufTask *self = GTH_PIXBUF_TASK (base);

	if (self->priv->release_func != NULL)
		(*self->priv->release_func) (self, error);
}


static void
gth_pixbuf_task_class_init (GthPixbufTaskClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	g_type_class_add_private (class, sizeof (GthPixbufTaskPrivate));

	object_class = G_OBJECT_CLASS (class);
	object_class->finalize = gth_pixbuf_task_finalize;
}


static void
gth_pixbuf_task_init (GthPixbufTask *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_PIXBUF_TASK, GthPixbufTaskPrivate);
	self->priv->init_func = NULL;
	self->priv->step_func = NULL;
	self->priv->release_func = NULL;
	self->priv->destroy_data_func = NULL;
	self->dest = NULL;

	self->data = NULL;
	self->src = NULL;
	self->dest = NULL;
	self->src_line = NULL;
	self->src_pixel = NULL;
	self->dest_line = NULL;
	self->dest_pixel = NULL;
	self->ltr = TRUE;
	self->line = 0;
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

		type = g_type_register_static (GTH_TYPE_ASYNC_TASK,
					       "GthPixbufTask",
					       &type_info,
					       0);
	}

	return type;
}


GthTask *
gth_pixbuf_task_new (const char      *description,
		     gboolean         single_step,
		     PixbufOpFunc     init_func,
		     PixbufOpFunc     step_func,
		     PixbufDataFunc   release_func,
		     gpointer         data,
		     GDestroyNotify   destroy_data_func)
{
	GthPixbufTask *self;

	self = (GthPixbufTask *) g_object_new (GTH_TYPE_PIXBUF_TASK,
					       "before-thread", before_execute_pixbuf_task,
					       "thread-func", execute_pixbuf_task,
					       "after-thread", after_execute_pixbuf_task,
					       NULL);

	self->priv->description = description;
	self->priv->single_step = single_step;
	self->priv->init_func = init_func;
	self->priv->step_func = step_func;
	self->priv->release_func = release_func;
	self->data = data;
	self->priv->destroy_data_func = destroy_data_func;

	return (GthTask *) self;
}


void
gth_pixbuf_task_set_source (GthPixbufTask *self,
			    GdkPixbuf     *src)
{
	g_return_if_fail (GDK_IS_PIXBUF (src));

	g_object_ref (src);
	release_pixbufs (self);
	self->src = src;
	self->has_alpha       = gdk_pixbuf_get_has_alpha (src);
	self->bytes_per_pixel = self->has_alpha ? 4 : 3;
	self->width           = gdk_pixbuf_get_width (src);
	self->height          = gdk_pixbuf_get_height (src);
	self->rowstride       = gdk_pixbuf_get_rowstride (src);
	self->src_line        = gdk_pixbuf_get_pixels (src);
}


GdkPixbuf *
gth_pixbuf_task_get_destination (GthPixbufTask *self)
{
	return self->dest;
}


void
copy_source_to_destination (GthPixbufTask *pixbuf_task)
{
	g_return_if_fail (pixbuf_task->src != NULL);

	_g_object_unref (pixbuf_task->dest);
	pixbuf_task->dest = gdk_pixbuf_copy (pixbuf_task->src);
}
