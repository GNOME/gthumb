/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001-2009 The Free Software Foundation, Inc.
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
#include <string.h>
#include <glib.h>
#include "glib-utils.h"
#include "gth-image-preloader.h"
#include "gth-marshal.h"


#undef DEBUG_PRELOADER
#define GTH_IMAGE_PRELOADER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GTH_TYPE_IMAGE_PRELOADER, GthImagePreloaderPrivate))
#define NEXT_LOAD_SMALL_TIMEOUT 100
#define NEXT_LOAD_BIG_TIMEOUT 300


enum {
	REQUESTED_READY,
	LAST_SIGNAL
};


typedef struct {
	GthFileData        *file_data;
	int                 requested_size;
	GthImageLoader     *loader;
	gboolean            loaded;
	gboolean            error;
	GthImagePreloader  *self;
} Preloader;


struct _GthImagePreloaderPrivate {
	GthLoadPolicy load_policy;
	int           n_preloaders;
	Preloader   **loader;                  /* Array of loaders, each loader
					        * will load an image. */
	int           requested;               /* This is the loader with the
					        * requested image.  The
					        * requested image is the image
					        * the user has expressly
					        * requested to view, when this
					        * image is loaded a
					        * requested_ready signal is
					        * emitted.
					        * Other images do not trigger
					        * any signal. */
	GFile        *requested_file;
	int           requested_size;
	GthFileData **files;
	int           current;                 /* This is the loader that has
					        * a loading underway. */
	guint         load_id;
};


static gpointer parent_class = NULL;
static guint gth_image_preloader_signals[LAST_SIGNAL] = { 0 };


/* -- Preloader -- */

static void        start_next_loader   (GthImagePreloader *self);
static Preloader * current_preloader   (GthImagePreloader *self);
static Preloader * requested_preloader (GthImagePreloader *self);


static gboolean
load_next (gpointer data)
{
	GthImagePreloader *self = data;

	if (self->priv->load_id != 0) {
		g_source_remove (self->priv->load_id);
		self->priv->load_id = 0;
	}

	start_next_loader (self);

	return FALSE;
}


static gboolean
preloader_need_second_step (Preloader *preloader)
{
	int                 original_width;
	int                 original_height;
	GdkPixbufAnimation *animation;

	gth_image_loader_get_original_size (preloader->loader, &original_width, &original_height);
	animation = gth_image_loader_get_animation (preloader->loader);

	return (! preloader->error
		&& (preloader->requested_size != -1)
		&& ((original_width > preloader->requested_size) || (original_height > preloader->requested_size))
		&& (animation != NULL)
		&& gdk_pixbuf_animation_is_static_image (animation));
}


static void
image_loader_ready_cb (GthImageLoader *iloader,
		       GError         *error,
		       Preloader      *preloader)
{
	GthImagePreloader *self = preloader->self;
	int                interval = NEXT_LOAD_SMALL_TIMEOUT;

	preloader->loaded = (error == NULL);
	preloader->error  = (error != NULL);

	g_object_ref (self);

	if (_g_file_equal (preloader->file_data->file, self->priv->requested_file)) {
#if DEBUG_PRELOADER
		debug (DEBUG_INFO, "[requested] %s => %s [size: %d]", (error == NULL) ? "ready" : "error", g_file_get_uri (preloader->file_data->file), preloader->requested_size);
#endif

		g_signal_emit (G_OBJECT (self),
			       gth_image_preloader_signals[REQUESTED_READY],
			       0,
			       preloader->file_data,
			       preloader->loader,
			       error);

		/* Reload only if the original size is bigger then the
		 * requested size, and if the image is not an animation. */

		if ((self->priv->load_policy == GTH_LOAD_POLICY_TWO_STEPS)
		    && preloader_need_second_step (preloader))
		{
			/* Reload the image at the original size */
			preloader->loaded = FALSE;
			preloader->requested_size = -1;
		}

		interval = NEXT_LOAD_BIG_TIMEOUT;
	}

	self->priv->load_id = g_timeout_add (interval, load_next, self);

	g_object_unref (self);
}


static Preloader *
preloader_new (GthImagePreloader *self)
{
	Preloader *preloader;

	preloader = g_new0 (Preloader, 1);
	preloader->file_data = NULL;
	preloader->loaded = FALSE;
	preloader->error = FALSE;
	preloader->loader = GTH_IMAGE_LOADER (gth_image_loader_new (TRUE));
	preloader->self = self;

	g_signal_connect (G_OBJECT (preloader->loader),
			  "ready",
			  G_CALLBACK (image_loader_ready_cb),
			  preloader);

	return preloader;
}


static void
preloader_free (Preloader *preloader)
{
	if (preloader == NULL)
		return;
	_g_object_unref (preloader->loader);
	_g_object_unref (preloader->file_data);
	g_free (preloader);
}


static void
preloader_set_file_data (Preloader    *preloader,
			 GthFileData  *file_data)
{
	g_return_if_fail (preloader != NULL);

	if (preloader->file_data != file_data) {
		_g_object_unref (preloader->file_data);
		preloader->file_data = NULL;
	}

	if (file_data != NULL) {
		preloader->file_data = g_object_ref (file_data);
		gth_image_loader_set_file_data (preloader->loader, preloader->file_data);
	}

	preloader->loaded = FALSE;
	preloader->error = FALSE;
}


/* -- GthImagePreloader -- */


static void
gth_image_preloader_finalize (GObject *object)
{
	GthImagePreloader *self;
	int                i;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GTH_IS_IMAGE_PRELOADER (object));

	self = GTH_IMAGE_PRELOADER (object);

	if (self->priv->load_id != 0) {
		g_source_remove (self->priv->load_id);
		self->priv->load_id = 0;
	}

	for (i = 0; i < self->priv->n_preloaders; i++) {
		preloader_free (self->priv->loader[i]);
		self->priv->loader[i] = NULL;
	}
	g_free (self->priv->loader);
	self->priv->loader = NULL;

	_g_object_unref (self->priv->requested_file);

	for (i = 0; i < self->priv->n_preloaders; i++) {
		_g_object_unref (self->priv->files[i]);
		self->priv->files[i] = NULL;
	}
	g_free (self->priv->files);
	self->priv->files = NULL;

	/* Chain up */
	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gth_image_preloader_class_init (GthImagePreloaderClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	g_type_class_add_private (class, sizeof (GthImagePreloaderPrivate));

	gth_image_preloader_signals[REQUESTED_READY] =
		g_signal_new ("requested_ready",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthImagePreloaderClass, requested_ready),
			      NULL, NULL,
			      gth_marshal_VOID__OBJECT_OBJECT_POINTER,
			      G_TYPE_NONE,
			      3,
			      G_TYPE_OBJECT,
			      G_TYPE_OBJECT,
			      G_TYPE_POINTER);

	object_class = G_OBJECT_CLASS (class);
	object_class->finalize = gth_image_preloader_finalize;
}


static void
gth_image_preloader_init (GthImagePreloader *self)
{
	self->priv = GTH_IMAGE_PRELOADER_GET_PRIVATE (self);
	self->priv->loader = NULL;
	self->priv->requested = -1;
	self->priv->requested_file = NULL;
	self->priv->current = -1;
	self->priv->files = NULL;
	self->priv->load_policy = GTH_LOAD_POLICY_ONE_STEP;
}


GType
gth_image_preloader_get_type (void)
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (GthImagePreloaderClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_image_preloader_class_init,
			NULL,
			NULL,
			sizeof (GthImagePreloader),
			0,
			(GInstanceInitFunc) gth_image_preloader_init
		};

		type = g_type_register_static (G_TYPE_OBJECT,
					       "GthImagePreloader",
					       &type_info,
					       0);
	}

	return type;
}


GthImagePreloader *
gth_image_preloader_new (GthLoadPolicy load_policy,
			 int           n_preloaders)
{
	GthImagePreloader *self;
	int                i;

	g_return_val_if_fail (n_preloaders > 0, NULL);

	self = g_object_new (GTH_TYPE_IMAGE_PRELOADER, NULL);

	self->priv->n_preloaders = n_preloaders;
	self->priv->load_policy = load_policy;
	self->priv->loader = g_new0 (Preloader *, self->priv->n_preloaders);
	for (i = 0; i < self->priv->n_preloaders; i++)
		self->priv->loader[i] = preloader_new (self);
	self->priv->files = g_new0 (GthFileData *, self->priv->n_preloaders);

	return self;
}


static Preloader *
current_preloader (GthImagePreloader *self)
{
	if (self->priv->current == -1)
		return NULL;
	else
		return self->priv->loader[self->priv->current];
}


static Preloader *
requested_preloader (GthImagePreloader *self)
{
	if (self->priv->requested == -1)
		return NULL;
	else
		return self->priv->loader[self->priv->requested];
}


/* -- gth_image_preloader_load -- */


typedef struct {
	GthImagePreloader *self;
	GthFileData       *requested;
	int                requested_size;
} LoadData;


static void
load_data_free (LoadData *load_data)
{
	if (load_data == NULL)
		return;

	_g_object_unref (load_data->requested);
	g_free (load_data);
}


void
gth_image_preloader_load__step2 (LoadData *load_data)
{
	GthImagePreloader *self = load_data->self;
	gboolean          *file_assigned;
	gboolean          *loader_assigned;
	GthFileData       *requested;
	int                i, j;

	if (! _g_file_equal (load_data->requested->file, self->priv->requested_file)
	    || (load_data->requested_size != self->priv->requested_size))
	{
		load_data_free (load_data);
		return;
	}

	requested = load_data->requested;

	file_assigned = g_new (gboolean, self->priv->n_preloaders);
	loader_assigned = g_new (gboolean, self->priv->n_preloaders);
	for (i = 0; i < self->priv->n_preloaders; i++) {
		loader_assigned[i] = FALSE;
		file_assigned[i] = FALSE;
	}

	self->priv->requested = -1;

	for (j = 0; j < self->priv->n_preloaders; j++) {
		GthFileData *file_data = self->priv->files[j];

		if (file_data == NULL)
			continue;

		/* check whether the image has already been loaded. */

		for (i = 0; i < self->priv->n_preloaders; i++) {
			Preloader *preloader = self->priv->loader[i];

			if ((preloader->file_data != NULL)
			    && g_file_equal (preloader->file_data->file, file_data->file)
			    && (_g_time_val_cmp (gth_file_data_get_modification_time (file_data),
			    			 gth_file_data_get_modification_time (preloader->file_data)) == 0)
			    && preloader->loaded)
			{
				loader_assigned[i] = TRUE;
				file_assigned[j] = TRUE;
				if ((requested != NULL) && g_file_equal (file_data->file, requested->file)) {
					self->priv->requested = i;
					g_signal_emit (G_OBJECT (self),
							gth_image_preloader_signals[REQUESTED_READY],
							0,
							preloader->file_data,
							preloader->loader,
							NULL);
#if DEBUG_PRELOADER
					debug (DEBUG_INFO, "[requested] preloaded");
#endif

					if ((self->priv->load_policy == GTH_LOAD_POLICY_TWO_STEPS)
					    && preloader_need_second_step (preloader))
					{
						/* Reload the image at the original size */
						preloader->loaded = FALSE;
						preloader->requested_size = -1;
					}
				}

#if DEBUG_PRELOADER
				{
					char *uri;

					uri = g_file_get_uri (file_data->file);
					debug (DEBUG_INFO, "[=] [%d] <- %s", i, uri);
					g_free (uri);
				}
#endif

				break;
			}
		}
	}

	/* assign remaining files */

	for (j = 0; j < self->priv->n_preloaders; j++) {
		GthFileData *file_data = self->priv->files[j];
		Preloader   *preloader;
		int          k;

		if (file_data == NULL)
			continue;

		if (file_assigned[j])
			continue;

		/* find the first non-assigned loader */
		for (k = 0; (k < self->priv->n_preloaders) && loader_assigned[k]; k++)
			/* void */;

		g_return_if_fail (k < self->priv->n_preloaders);

		preloader = self->priv->loader[k];
		loader_assigned[k] = TRUE;
		preloader_set_file_data (preloader, file_data);
		preloader->requested_size = load_data->requested_size;

		if ((requested != NULL) && g_file_equal (file_data->file, requested->file)) {
			self->priv->requested = k;
#if DEBUG_PRELOADER
			{
				char *uri;

				uri = g_file_get_uri (file_data->file);
				debug (DEBUG_INFO, "[requested] %s", uri);
				g_free (uri);
			}
#endif
		}

#if DEBUG_PRELOADER
		uri = g_file_get_uri (file_data->file);
		debug (DEBUG_INFO, "[+] [%d] <- %s", k, uri);
		g_free (uri);
#endif
	}

	load_data_free (load_data);
	g_free (loader_assigned);
	g_free (file_assigned);

	start_next_loader (self);
}


static GthFileData *
check_file (GthFileData *file_data)
{
	if (file_data == NULL)
		return NULL;

	if (! g_file_is_native (file_data->file))
		return NULL;

	if (! _g_mime_type_is_image (gth_file_data_get_mime_type (file_data)))
		return NULL;

	return gth_file_data_dup (file_data);
}


void
gth_image_preloader_load (GthImagePreloader *self,
			  GthFileData       *requested,
			  int                requested_size,
			  ...)
{
	int          i;
	int          n;
	va_list      args;
	GthFileData *file_data;
	LoadData    *load_data;

	_g_object_unref (self->priv->requested_file);
	self->priv->requested_file = g_object_ref (requested->file);
	self->priv->requested_size = requested_size;
	self->priv->requested = -1;

	for (i = 0; i < self->priv->n_preloaders; i++) {
		_g_object_unref (self->priv->files[i]);
		self->priv->files[i] = NULL;
	}

	self->priv->files[0] = gth_file_data_dup (requested);
	n = 1;
	va_start (args, requested_size);
	while ((n < self->priv->n_preloaders) && (file_data = va_arg (args, GthFileData *)) != NULL)
		self->priv->files[n++] = check_file (file_data);
	va_end (args);

	load_data = g_new0 (LoadData, 1);
	load_data->self = self;
	load_data->requested = gth_file_data_dup (requested);
	load_data->requested_size = requested_size;

	gth_image_preloader_stop (self, (DataFunc) gth_image_preloader_load__step2, load_data);
}


void
gth_image_prelaoder_set_load_policy (GthImagePreloader *self,
				     GthLoadPolicy      policy)
{
	self->priv->load_policy = policy;
}


GthImageLoader *
gth_image_preloader_get_loader (GthImagePreloader *self,
				GthFileData       *file_data)
{
	int i;

	g_return_val_if_fail (self != NULL, NULL);

	if (file_data == NULL)
		return NULL;

	for (i = 0; i < self->priv->n_preloaders; i++) {
		Preloader *preloader = self->priv->loader[i];

		if ((preloader->file_data != NULL)
		    && g_file_equal (preloader->file_data->file, file_data->file)
		    && (_g_time_val_cmp (gth_file_data_get_modification_time (file_data),
			    		 gth_file_data_get_modification_time (preloader->file_data)) == 0)
		    && preloader->loaded)
		{
			return preloader->loader;
		}
	}

	return NULL;
}


GthFileData *
gth_image_preloader_get_requested (GthImagePreloader *self)
{
	Preloader *preloader;

	preloader = requested_preloader (self);
	if (preloader == NULL)
		return NULL;
	else
		return preloader->file_data;
}


static void
start_next_loader (GthImagePreloader *self)
{
	Preloader *preloader;
	int        i;

	preloader = requested_preloader (self);
	if ((preloader != NULL)
	    && (preloader->file_data != NULL)
	    && ! preloader->error
	    && ! preloader->loaded)
	{
		i = self->priv->requested;
	}
	else {
		int n = 0;

		if  (self->priv->current == -1)
			i = 0;
		else
			i = (self->priv->current + 1) % self->priv->n_preloaders;

		for (i = 0; n < self->priv->n_preloaders; i = (i + 1) % self->priv->n_preloaders) {
			preloader = self->priv->loader[i];

			if ((preloader != NULL)
			    && (preloader->file_data != NULL)
			    && ! preloader->error
			    && ! preloader->loaded)
			{
				break;
			}

			n++;
		}

		if (n >= self->priv->n_preloaders) {
			self->priv->current = -1;
#if DEBUG_PRELOADER
			debug (DEBUG_INFO, "done");
#endif
			return;
		}
	}

	self->priv->current = i;
	preloader = current_preloader (self);

#if DEBUG_PRELOADER
	{
		char *uri;

		uri = g_file_get_uri (preloader->file_data->file);
		debug (DEBUG_INFO, "load %s [size: %d]", uri, preloader->requested_size);
		g_free (uri);
	}
#endif

	gth_image_loader_load_at_size (preloader->loader, preloader->requested_size);
}


void
gth_image_preloader_stop (GthImagePreloader *self,
			  DataFunc           done_func,
			  gpointer           done_func_data)
{
	Preloader *preloader;

	preloader = current_preloader (self);

	if (preloader == NULL) {
#if DEBUG_PRELOADER
		debug (DEBUG_INFO, "stopped");
#endif
		call_when_idle (done_func, done_func_data);
		return;
	}

	self->priv->current = -1;
	gth_image_loader_cancel (preloader->loader, done_func, done_func_data);
}


void
gth_image_preloader_set (GthImagePreloader *dest,
			 GthImagePreloader *src)
{
	int i;

	if (src == NULL)
		return;

	g_return_if_fail (src->priv->n_preloaders == dest->priv->n_preloaders);

	for (i = 0; i < src->priv->n_preloaders; i++) {
		Preloader *src_loader = src->priv->loader[i];
		Preloader *dest_loader = dest->priv->loader[i];

		if ((src_loader->file_data != NULL) && src_loader->loaded && ! src_loader->error) {
		    	g_object_unref (dest_loader->file_data);
		    	dest_loader->file_data = g_object_ref (src_loader->file_data);

		    	g_signal_handlers_block_by_data (dest_loader->loader, dest_loader);
			gth_image_loader_load_from_image_loader (dest_loader->loader, src_loader->loader);
			g_signal_handlers_unblock_by_data (dest_loader->loader, dest_loader);

			dest_loader->loaded = TRUE;
			dest_loader->error = FALSE;
		}
	}
}
