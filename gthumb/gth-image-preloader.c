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
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 */


#include <config.h>
#include <string.h>
#include <glib.h>
#include "glib-utils.h"
#include "gth-image-preloader.h"


#define GTH_IMAGE_PRELOADER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GTH_TYPE_IMAGE_PRELOADER, GthImagePreloaderPrivate))
#define NEXT_LOAD_SMALL_TIMEOUT 100
#define NEXT_LOAD_BIG_TIMEOUT 400
#define N_LOADERS 3


enum {
	REQUESTED_READY,
	LAST_SIGNAL
};


typedef struct {
	GthFileData        *file_data;
	GthImageLoader     *loader;
	gboolean            loaded;
	gboolean            error;
	GthImagePreloader  *image_preloader;
} Preloader;


struct _GthImagePreloaderPrivate {
	Preloader   *loader[N_LOADERS];       /* Array of loaders, each loader
					       * will load an image. */
	int          requested;               /* This is the loader with the
					       * requested image.  The
					       * requested image is the image
					       * the user has expressly
					       * requested to view, when this
					       * image is loaded a
					       * requested_ready signal is
					       * emitted.
					       * Other images do not trigger
					       * any signal. */
	int          current;                 /* This is the loader that has
					       * a loading underway. */
	gboolean     stopped;                 /* Whether the preloader has
					       * been stopped. */
	DoneFunc     done_func;               /* Function to call after
					       * stopping the loader. */
	gpointer     done_func_data;
	guint        load_id;
};


static gpointer parent_class = NULL;
static guint gth_image_preloader_signals[LAST_SIGNAL] = { 0 };


/* -- Preloader -- */

static void        start_next_loader   (GthImagePreloader *image_preloader);
static Preloader * current_preloader   (GthImagePreloader *image_preloader);
static Preloader * requested_preloader (GthImagePreloader *image_preloader);


static gboolean
load_next (gpointer data)
{
	GthImagePreloader *image_preloader = data;

	if (image_preloader->priv->load_id != 0) {
		g_source_remove (image_preloader->priv->load_id);
		image_preloader->priv->load_id = 0;
	}

	start_next_loader (image_preloader);

	return FALSE;
}


static void
image_loader_ready_cb (GthImageLoader *il,
		       GError         *error,
		       Preloader      *preloader)
{
	GthImagePreloader *image_preloader = preloader->image_preloader;
	int                timeout = NEXT_LOAD_SMALL_TIMEOUT;

	preloader->loaded = (error == NULL);
	preloader->error  = (error != NULL);

	if (preloader == requested_preloader (image_preloader)) {
		g_signal_emit (G_OBJECT (image_preloader),
			       gth_image_preloader_signals[REQUESTED_READY],
			       0,
			       error);
		debug (DEBUG_INFO, "[requested] error");
		timeout = NEXT_LOAD_BIG_TIMEOUT;
	}

	image_preloader->priv->load_id = g_idle_add (load_next, image_preloader);
}


static Preloader*
preloader_new (GthImagePreloader *image_preloader)
{
	Preloader *preloader;

	preloader = g_new0 (Preloader, 1);
	preloader->file_data = NULL;
	preloader->loaded = FALSE;
	preloader->error = FALSE;
	preloader->loader = GTH_IMAGE_LOADER (gth_image_loader_new (TRUE));
	preloader->image_preloader = image_preloader;

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
	GthImagePreloader *image_preloader;
	int                i;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GTH_IS_IMAGE_PRELOADER (object));

	image_preloader = GTH_IMAGE_PRELOADER (object);

	if (image_preloader->priv->load_id != 0) {
		g_source_remove (image_preloader->priv->load_id);
		image_preloader->priv->load_id = 0;
	}

	for (i = 0; i < N_LOADERS; i++) {
		preloader_free (image_preloader->priv->loader[i]);
		image_preloader->priv->loader[i] = NULL;
	}

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
			      g_cclosure_marshal_VOID__POINTER,
			      G_TYPE_NONE,
			      1,
			      G_TYPE_POINTER);

	object_class = G_OBJECT_CLASS (class);
	object_class->finalize = gth_image_preloader_finalize;
}


static void
gth_image_preloader_init (GthImagePreloader *image_preloader)
{
	int i;

	image_preloader->priv = GTH_IMAGE_PRELOADER_GET_PRIVATE (image_preloader);

	for (i = 0; i < N_LOADERS; i++)
		image_preloader->priv->loader[i] = preloader_new (image_preloader);
	image_preloader->priv->requested = -1;
	image_preloader->priv->current = -1;
	image_preloader->priv->stopped = FALSE;
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
gth_image_preloader_new (void)
{
	return (GthImagePreloader *) g_object_new (GTH_TYPE_IMAGE_PRELOADER, NULL);
}


static Preloader *
current_preloader (GthImagePreloader *image_preloader)
{
	if (image_preloader->priv->current == -1)
		return NULL;
	else
		return image_preloader->priv->loader[image_preloader->priv->current];
}


static Preloader *
requested_preloader (GthImagePreloader *image_preloader)
{
	if (image_preloader->priv->requested == -1)
		return NULL;
	else
		return image_preloader->priv->loader[image_preloader->priv->requested];
}


/* -- gth_image_preloader_load -- */


#define N_ARGS 3


typedef struct {
	GthImagePreloader *image_preloader;
	GthFileData       *requested;
	GthFileData       *next1;
	GthFileData       *prev1;
} LoadData;


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


static LoadData*
load_data_new (GthImagePreloader *image_preloader,
	       GthFileData       *requested,
	       GthFileData       *next1,
	       GthFileData       *prev1)
{
	LoadData *load_data;

	load_data = g_new0 (LoadData, 1);

	load_data->image_preloader = image_preloader;
	load_data->requested = check_file (requested);
	load_data->next1 = check_file (next1);
	load_data->prev1 = check_file (prev1);

	return load_data;
}


static void
load_data_free (LoadData *load_data)
{
	if (load_data == NULL)
		return;

	_g_object_unref (load_data->requested);
	_g_object_unref (load_data->next1);
	_g_object_unref (load_data->prev1);
	g_free (load_data);
}


void
gth_image_preloader_load__step2 (LoadData *load_data)
{
	GthImagePreloader *image_preloader = load_data->image_preloader;
	GthFileData       *requested = load_data->requested;
	GthFileData       *next1 = load_data->next1;
	GthFileData       *prev1 = load_data->prev1;
	GthFileData       *files[N_ARGS];
	gboolean           file_assigned[N_LOADERS];
	gboolean           loader_assigned[N_LOADERS];
	int                i, j;

	files[0] = requested;
	files[1] = next1;
	files[2] = prev1;

	for (i = 0; i < N_LOADERS; i++) {
		loader_assigned[i] = FALSE;
		file_assigned[i] = FALSE;
	}

	image_preloader->priv->requested = -1;

	for (j = 0; j < N_ARGS; j++) {
		GthFileData *file_data = files[j];

		if (file_data == NULL)
			continue;

		/* check whether the image has already been loaded. */

		for (i = 0; i < N_LOADERS; i++) {
			Preloader *preloader = image_preloader->priv->loader[i];

			if ((preloader->file_data != NULL)
			    && g_file_equal (preloader->file_data->file, file_data->file)
			    && (_g_time_val_cmp (gth_file_data_get_modification_time (file_data),
			    			 gth_file_data_get_modification_time (preloader->file_data)) == 0)
			    && preloader->loaded)
			{
				char *uri;

				loader_assigned[i] = TRUE;
				file_assigned[j] = TRUE;
				if ((requested != NULL) && g_file_equal (file_data->file, requested->file)) {
					image_preloader->priv->requested = i;
					g_signal_emit (G_OBJECT (image_preloader), gth_image_preloader_signals[REQUESTED_READY], 0, NULL);
					debug (DEBUG_INFO, "[requested] preloaded");
				}

				uri = g_file_get_uri (file_data->file);
				debug (DEBUG_INFO, "[=] [%d] <- %s", i, uri);
				g_free (uri);
			}
		}
	}

	/* assign remaining paths. */

	for (j = 0; j < N_ARGS; j++) {
		GthFileData *file_data = files[j];
		Preloader   *preloader;
		int          k;
		char        *uri;

		if (file_data == NULL)
			continue;

		if (file_assigned[j])
			continue;

		/* find the first non-assigned loader */
		for (k = 0; (k < N_LOADERS) && loader_assigned[k]; k++)
			/* void */;

		g_return_if_fail (k < N_LOADERS);

		preloader = image_preloader->priv->loader[k];
		loader_assigned[k] = TRUE;
		preloader_set_file_data (preloader, file_data);

		if ((requested != NULL) && g_file_equal (file_data->file, requested->file)) {
			image_preloader->priv->requested = k;

			uri = g_file_get_uri (file_data->file);
			debug (DEBUG_INFO, "[requested] %s", uri);
			g_free (uri);
		}

		uri = g_file_get_uri (file_data->file);
		debug (DEBUG_INFO, "[+] [%d] <- %s", k, uri);
		g_free (uri);
	}

	load_data_free (load_data);

	image_preloader->priv->stopped = FALSE;
	start_next_loader (image_preloader);
}


void
gth_image_preloader_load (GthImagePreloader  *image_preloader,
			  GthFileData        *requested,
			  GthFileData        *next1,
			  GthFileData        *prev1)
{
	LoadData *load_data;

	load_data = load_data_new (image_preloader, requested, next1, prev1);
	gth_image_preloader_stop (image_preloader, (DoneFunc) gth_image_preloader_load__step2, load_data);
}


GthImageLoader *
gth_image_preloader_get_loader (GthImagePreloader *image_preloader,
				GthFileData       *file_data)
{
	int i;

	g_return_val_if_fail (image_preloader != NULL, NULL);

	if (file_data == NULL)
		return NULL;

	for (i = 0; i < N_LOADERS; i++) {
		Preloader *preloader = image_preloader->priv->loader[i];

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
gth_image_preloader_get_requested (GthImagePreloader *image_preloader)
{
	Preloader *preloader;

	preloader = requested_preloader (image_preloader);
	if (preloader == NULL)
		return NULL;
	else
		return preloader->file_data;
}


static void
start_next_loader (GthImagePreloader *image_preloader)
{
	int        i;
	Preloader *preloader;
	char      *uri;

	if (image_preloader->priv->stopped) {
		image_preloader->priv->current = -1;
		image_preloader->priv->stopped = FALSE;

		debug (DEBUG_INFO, "stopped");

		if (image_preloader->priv->done_func != NULL)
			(*image_preloader->priv->done_func) (image_preloader->priv->done_func_data);
		image_preloader->priv->done_func = NULL;

		return;
	}

	preloader = requested_preloader (image_preloader);
	if ((preloader != NULL)
	    && (preloader->file_data != NULL)
	    && ! preloader->error
	    && ! preloader->loaded)
	{
		i = image_preloader->priv->requested;
	}
	else {
		int n = 0;

		if  (image_preloader->priv->current == -1)
			i = 0;
		else
			i = (image_preloader->priv->current + 1) % N_LOADERS;

		for (i = 0; n < N_LOADERS; i = (i + 1) % N_LOADERS) {
			preloader = image_preloader->priv->loader[i];

			if ((preloader != NULL)
			    && (preloader->file_data != NULL)
			    && ! preloader->error
			    && ! preloader->loaded)
			{
				break;
			}

			n++;
		}

		if (n >= N_LOADERS) {
			image_preloader->priv->current = -1;
			debug (DEBUG_INFO, "done");
			return;
		}
	}

	image_preloader->priv->current = i;
	preloader = current_preloader (image_preloader);

	gth_image_loader_load (preloader->loader);

	uri = g_file_get_uri (preloader->file_data->file);
	debug (DEBUG_INFO, "load %s", uri);
	g_free (uri);
}


void
gth_image_preloader_stop (GthImagePreloader *image_preloader,
			  DoneFunc           done_func,
			  gpointer           done_func_data)
{
	if (image_preloader->priv->current == -1) {
		debug (DEBUG_INFO, "stopped");
		if (done_func != NULL)
			(*done_func) (done_func_data);
		return;
	}

	image_preloader->priv->stopped = TRUE;
	image_preloader->priv->done_func = done_func;
	image_preloader->priv->done_func_data = done_func_data;
}


void
gth_image_preloader_set (GthImagePreloader *dest,
			 GthImagePreloader *src)
{
	int i;

	if (src == NULL)
		return;

	for (i = 0; i < N_LOADERS; i++) {
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
