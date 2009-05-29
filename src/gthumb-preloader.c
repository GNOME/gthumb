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


#include <string.h>
#include <glib.h>
#include "file-utils.h"
#include "glib-utils.h"
#include "gconf-utils.h"
#include "gthumb-preloader.h"
#include "gthumb-marshal.h"
#include "preferences.h"


#define NEXT_LOAD_SMALL_TIMEOUT 100
#define NEXT_LOAD_BIG_TIMEOUT 400


enum {
	REQUESTED_ERROR,
	REQUESTED_DONE,
	LAST_SIGNAL
};


static GObjectClass *parent_class = NULL;
static guint gthumb_preloader_signals[LAST_SIGNAL] = { 0 };


/* -- PreLoader -- */

static void        start_next_loader   (GThumbPreloader *gploader);
static PreLoader * current_preloader   (GThumbPreloader *gploader);
static PreLoader * requested_preloader (GThumbPreloader *gploader);


static gboolean
load_next (gpointer data)
{
	GThumbPreloader *gploader = data;

	if (gploader->load_id != 0) {
		g_source_remove (gploader->load_id);
		gploader->load_id = 0;
	}

	start_next_loader (gploader);

	return FALSE;
}


static void
loader_error_cb (ImageLoader  *il,
		 PreLoader    *ploader)
{
	GThumbPreloader *gploader = ploader->gploader;
	int              timeout = NEXT_LOAD_SMALL_TIMEOUT;

	ploader->loaded = FALSE;
	ploader->error  = TRUE;

	if (ploader == requested_preloader (gploader)) {
		g_signal_emit (G_OBJECT (gploader),
			       gthumb_preloader_signals[REQUESTED_ERROR], 0);
		debug (DEBUG_INFO, "[requested] error");
		timeout = NEXT_LOAD_BIG_TIMEOUT;
	}

	gploader->load_id = g_idle_add (load_next, gploader);
}


static void
loader_done_cb (ImageLoader *il,
		PreLoader   *ploader)
{
	GThumbPreloader *gploader = ploader->gploader;
	int              timeout = NEXT_LOAD_SMALL_TIMEOUT;

	ploader->loaded = TRUE;
	ploader->error  = FALSE;

	if (ploader == requested_preloader (gploader)) {
		g_signal_emit (G_OBJECT (gploader),
			       gthumb_preloader_signals[REQUESTED_DONE], 0);
		debug (DEBUG_INFO, "[requested] done");
		timeout = NEXT_LOAD_BIG_TIMEOUT;
	}

	gploader->load_id = g_timeout_add (timeout, load_next, gploader);
}


static PreLoader*
preloader_new (GThumbPreloader *gploader)
{
	PreLoader *ploader = g_new (PreLoader, 1);

	ploader->file     = NULL;
	ploader->loaded   = FALSE;
	ploader->error    = FALSE;
	ploader->loader   = IMAGE_LOADER (image_loader_new (TRUE));
	ploader->gploader = gploader;

	g_signal_connect (G_OBJECT (ploader->loader),
			  "image_done",
			  G_CALLBACK (loader_done_cb),
			  ploader);
	g_signal_connect (G_OBJECT (ploader->loader),
			  "image_error",
			  G_CALLBACK (loader_error_cb),
			  ploader);

	return ploader;
}


static void
preloader_free (PreLoader *ploader)
{
	if (ploader == NULL)
		return;
	g_object_unref (ploader->loader);
	file_data_unref (ploader->file);
	g_free (ploader);
}


static void
preloader_set_file (PreLoader *ploader,
		    FileData  *file)
{
	g_return_if_fail (ploader != NULL);

	if (ploader->file != file) {
		file_data_unref (ploader->file);
		ploader->file = NULL;
	}
	
	if (file != NULL) {
		ploader->file = file_data_dup (file);
		image_loader_set_file (ploader->loader, ploader->file);
	}
	
	ploader->loaded = FALSE;
	ploader->error = FALSE;
}


/* -- GThumbPreloader -- */


static void
gthumb_preloader_finalize (GObject *object)
{
	GThumbPreloader *gploader;
	int              i;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GTHUMB_IS_PRELOADER (object));

	gploader = GTHUMB_PRELOADER (object);

	if (gploader->load_id != 0) {
		g_source_remove (gploader->load_id);
		gploader->load_id = 0;
	}

	for (i = 0; i < N_LOADERS; i++) {
		preloader_free (gploader->loader[i]);
		gploader->loader[i] = NULL;
	}

	/* Chain up */
	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gthumb_preloader_class_init (GThumbPreloaderClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	parent_class = g_type_class_peek_parent (class);

	gthumb_preloader_signals[REQUESTED_ERROR] =
		g_signal_new ("requested_error",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GThumbPreloaderClass, requested_error),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
	gthumb_preloader_signals[REQUESTED_DONE] =
		g_signal_new ("requested_done",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GThumbPreloaderClass, requested_done),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);

	object_class->finalize = gthumb_preloader_finalize;

	class->requested_error = NULL;
	class->requested_done = NULL;
}


static void
gthumb_preloader_init (GThumbPreloader *gploader)
{
	int i;

	for (i = 0; i < N_LOADERS; i++)
		gploader->loader[i] = preloader_new (gploader);
	gploader->requested = -1;
	gploader->current = -1;
	gploader->stopped = FALSE;
}


GType
gthumb_preloader_get_type ()
{
        static GType type = 0;

        if (! type) {
                GTypeInfo type_info = {
			sizeof (GThumbPreloaderClass),
			NULL,
			NULL,
			(GClassInitFunc) gthumb_preloader_class_init,
			NULL,
			NULL,
			sizeof (GThumbPreloader),
			0,
			(GInstanceInitFunc) gthumb_preloader_init
		};

		type = g_type_register_static (G_TYPE_OBJECT,
					       "GThumbPreloader",
					       &type_info,
					       0);
	}

        return type;
}


GThumbPreloader *
gthumb_preloader_new (void)
{
	GObject *object;
	
	object = g_object_new (GTHUMB_TYPE_PRELOADER, NULL);
	
	return GTHUMB_PRELOADER (object);
}


static PreLoader *
current_preloader (GThumbPreloader *gploader)
{
	if (gploader->current == -1)
		return NULL;
	return gploader->loader[gploader->current];
}


static PreLoader *
requested_preloader (GThumbPreloader *gploader)
{
	if (gploader->requested == -1)
		return NULL;
	return gploader->loader[gploader->requested];
}


/* -- gthumb_preloader_load -- */


#define N_ARGS 3


typedef struct {
	GThumbPreloader  *gploader;
	FileData         *requested;
	FileData         *next1;
	FileData         *prev1;
} LoadData;


static LoadData*
load_data_new (GThumbPreloader *gploader,
	       FileData        *requested,
	       FileData        *next1,
	       FileData        *prev1)
{
	LoadData *load_data;

	load_data = g_new0 (LoadData, 1);

	load_data->gploader = gploader;
	if (requested != NULL)
		load_data->requested = file_data_dup (requested);
	if (next1 != NULL)
		load_data->next1 = file_data_dup (next1);
	if (prev1 != NULL)
		load_data->prev1 = file_data_dup (prev1);

	return load_data;
}


static void
load_data_free (LoadData *load_data)
{
	if (load_data == NULL)
		return;

	file_data_unref (load_data->requested);
	file_data_unref (load_data->next1);
	file_data_unref (load_data->prev1);
	g_free (load_data);
}


void
gthumb_preloader_load__step2 (LoadData *load_data)
{
	GThumbPreloader *gploader = load_data->gploader;
	FileData        *requested = load_data->requested;
	FileData        *next1 = load_data->next1;
	FileData        *prev1 = load_data->prev1;
	FileData        *files[N_ARGS];
	gboolean         file_assigned[N_LOADERS];
	gboolean         loader_assigned[N_LOADERS];
	int              i, j;

	if (requested == NULL)
		return;

	files[0] = requested;
	files[1] = next1;
	files[2] = prev1;

	for (i = 0; i < N_LOADERS; i++) {
		loader_assigned[i] = FALSE;
		file_assigned[i] = FALSE;
	}

	for (j = 0; j < N_ARGS; j++) {
		FileData *file = files[j];

		if (file == NULL)
			continue;

		/* check whether the image has already been loaded. */

		for (i = 0; i < N_LOADERS; i++) {
			PreLoader *ploader = gploader->loader[i];

			if ((ploader->file != NULL)
			    && file_data_same (ploader->file, file)
			    && (file->mtime == ploader->file->mtime)
			    && ploader->loaded) 
			{
				loader_assigned[i] = TRUE;
				file_assigned[j] = TRUE;
				if (file_data_same (file, requested)) {
					gploader->requested = i;
					g_signal_emit (G_OBJECT (gploader), gthumb_preloader_signals[REQUESTED_DONE], 0);
					debug (DEBUG_INFO, "[requested] preloaded");
				}
				debug (DEBUG_INFO, "[=] [%d] <- %s", i, file->utf8_path);
			}
		}
	}

	/* assign remaining paths. */
	
	for (j = 0; j < N_ARGS; j++) {
		FileData  *file = files[j];
		PreLoader *ploader;
		int        k;

		if (file == NULL)
			continue;

		if (file_assigned[j])
			continue;

		/* find the first non-assigned loader */
		for (k = 0; (k < N_LOADERS) && loader_assigned[k]; k++)
			;

		g_return_if_fail (k < N_LOADERS);

		ploader = gploader->loader[k];
		loader_assigned[k] = TRUE;
		preloader_set_file (ploader, file);

		if (file_data_same (file, requested)) {
			gploader->requested = k;
			debug (DEBUG_INFO, "[requested] %s", file->utf8_path);
		}

		debug (DEBUG_INFO, "[+] [%d] <- %s", k, file->utf8_path);
	}

	load_data_free (load_data);

	gploader->stopped = FALSE;
	start_next_loader (gploader);
}


void                
gthumb_preloader_load (GThumbPreloader  *gploader,
		       FileData         *requested,
		       FileData         *next1,
		       FileData         *prev1)
{
	LoadData *load_data;
	
	load_data = load_data_new (gploader, requested, next1, prev1);
	gthumb_preloader_stop (gploader, (DoneFunc) gthumb_preloader_load__step2, load_data);	
}


void
gthumb_preloader_start (GThumbPreloader *gploader,
			const char      *requested,
			const char      *next1,
			const char      *prev1)
{
	FileData *f_requested = NULL;
	FileData *f_next1 = NULL;
	FileData *f_prev1 = NULL;
	gboolean  fast_mime_type;
	
	g_return_if_fail (requested != NULL);
	
	if (! is_local_file (next1))
		next1 = NULL;
	if (! is_local_file (prev1))
		prev1 = NULL;
	
	fast_mime_type = eel_gconf_get_boolean (PREF_FAST_FILE_TYPE, TRUE);
	
	f_requested = file_data_new (requested);
	file_data_update_all (f_requested, fast_mime_type);
	if (next1 != NULL) {
		f_next1 = file_data_new (next1);
		file_data_update (f_next1);
		file_data_update_mime_type (f_next1, fast_mime_type);
	}
	if (prev1 != NULL) {
		f_prev1 = file_data_new (prev1);
		file_data_update_all (f_prev1, fast_mime_type);
	}
	
	gthumb_preloader_load (gploader, f_requested, f_next1, f_prev1);
	
	file_data_unref (f_requested);
	file_data_unref (f_next1);
	file_data_unref (f_prev1);
}
						 

ImageLoader *
gthumb_preloader_get_loader (GThumbPreloader  *gploader,
			     const char       *path)
{
	int i;
	time_t path_mtime;

	g_return_val_if_fail (gploader != NULL, NULL);
	g_return_val_if_fail (path != NULL, NULL);

	path_mtime = get_file_mtime (path);

	for (i = 0; i < N_LOADERS; i++) {
		PreLoader *ploader = gploader->loader[i];

		if ((ploader->file != NULL)
		    && file_data_same_path (ploader->file, path)
		    && (path_mtime == ploader->file->mtime)
		    && ploader->loaded)
			return ploader->loader;
	}

	return NULL;
}


static void
start_next_loader (GThumbPreloader *gploader)
{
	int        i;
	PreLoader *ploader;

	if (gploader->stopped) {
		gploader->current = -1;
		gploader->stopped = FALSE;

		debug (DEBUG_INFO, "stopped");

		if (gploader->done_func != NULL)
			(*gploader->done_func) (gploader->done_func_data);
		gploader->done_func = NULL;

		return;
	}

	ploader = requested_preloader (gploader);
	if ((ploader != NULL)
	    && (ploader->file != NULL)
	    && ! ploader->error
	    && ! ploader->loaded) 
	{
		i = gploader->requested;
	}
	else {
		int n = 0;

		if  (gploader->current == -1)
			i = 0;
		else
			i = (gploader->current + 1) % N_LOADERS;

		for (i = 0; n < N_LOADERS; i = (i + 1) % N_LOADERS) {
			ploader = gploader->loader[i];

			if ((ploader != NULL)
			    && (ploader->file != NULL)
			    && ! ploader->error
			    && ! ploader->loaded)
			{
				break;
			}

			n++;
		}

		if (n >= N_LOADERS) {
			gploader->current = -1;
			debug (DEBUG_INFO, "done");
			return;
		}
	}

	gploader->current = i;
	ploader = current_preloader (gploader);

	image_loader_start (ploader->loader);
	debug (DEBUG_INFO, "load %s", ploader->file->utf8_path);
}


void
gthumb_preloader_stop (GThumbPreloader *gploader,
		       DoneFunc         done_func,
		       gpointer         done_func_data)
{
	if (gploader->current == -1) {
		debug (DEBUG_INFO, "stopped");
		if (done_func != NULL)
			(*done_func) (done_func_data);
		return;
	}

	gploader->stopped = TRUE;
	gploader->done_func = done_func;
	gploader->done_func_data = done_func_data;
}


void                
gthumb_preloader_set (GThumbPreloader *dest,
		      GThumbPreloader *src)
{
	int i;
	
	if (src == NULL)
		return;
	
	for (i = 0; i < N_LOADERS; i++) {
		PreLoader *src_loader = src->loader[i];
		PreLoader *dest_loader = dest->loader[i];
		
		if ((src_loader->file != NULL) && src_loader->loaded && ! src_loader->error) {
		    	file_data_unref (dest_loader->file);
		    	dest_loader->file = file_data_dup (src_loader->file);
		    	
		    	g_signal_handlers_block_by_data (dest_loader->loader, dest_loader);
			image_loader_load_from_image_loader (dest_loader->loader, src_loader->loader);
			g_signal_handlers_unblock_by_data (dest_loader->loader, dest_loader);
			
			dest_loader->loaded = TRUE;
			dest_loader->error = FALSE;
		}
	}
}
