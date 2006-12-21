/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2006 Free Software Foundation, Inc.
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
#include "gth-image-history.h"


#define MAX_UNDO_HISTORY_LEN 5


/* GthImageData */


GthImageData *
gth_image_data_new (GdkPixbuf *image,
		    gboolean   unsaved)
{
	GthImageData *idata;

	g_return_val_if_fail (image != NULL, NULL);

	idata = g_new0 (GthImageData, 1);

	idata->ref = 1;
	g_object_ref (image);
	idata->image = image;
	idata->unsaved = unsaved;

	return idata;
}


void
gth_image_data_ref (GthImageData *idata)
{
	g_return_if_fail (idata != NULL);
	idata->ref++;
}


void
gth_image_data_unref (GthImageData *idata)
{
	g_return_if_fail (idata != NULL);

	idata->ref--;
	if (idata->ref == 0) {
		g_object_unref (idata->image);
		g_free (idata);
	}
}


void
gth_image_data_list_free (GList *list)
{
	if (list == NULL)
		return;
	g_list_foreach (list, (GFunc) gth_image_data_unref, NULL);
	g_list_free (list);
}


/* GthImageHistory */


struct _GthImageHistoryPrivate {
	GList *undo_history;  /* GthImageData items */
	GList *redo_history;  /* GthImageData items */
};


enum {
	CHANGED,
	LAST_SIGNAL
};


static GObjectClass *parent_class = NULL;
static guint         gth_image_history_signals[LAST_SIGNAL] = { 0 };


static void
gth_image_history_finalize (GObject *object)
{
	GthImageHistory *history;

        g_return_if_fail (GTH_IS_IMAGE_HISTORY (object));
	history = GTH_IMAGE_HISTORY (object);

	if (history->priv != NULL) {
		gth_image_history_clear (history);
		g_free (history->priv);
		history->priv = NULL;
	}

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gth_image_history_class_init (GthImageHistoryClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);

	gth_image_history_signals[CHANGED] =
		g_signal_new ("changed",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthImageHistoryClass, changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

	object_class = G_OBJECT_CLASS (class);
	object_class->finalize = gth_image_history_finalize;
}


static void
gth_image_history_init (GthImageHistory *history)
{
	history->priv = g_new0 (GthImageHistoryPrivate, 1);
}


GType
gth_image_history_get_type (void)
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (GthImageHistoryClass),
                        NULL,
                        NULL,
                        (GClassInitFunc) gth_image_history_class_init,
                        NULL,
                        NULL,
                        sizeof (GthImageHistory),
                        0,
                        (GInstanceInitFunc) gth_image_history_init
                };

                type = g_type_register_static (G_TYPE_OBJECT,
                                               "GthImageHistory",
                                               &type_info,
                                               0);
        }

        return type;
}


GthImageHistory *
gth_image_history_new (void)
{
	return (GthImageHistory *) g_object_new (GTH_TYPE_IMAGE_HISTORY, NULL);
}


static GthImageData*
remove_first_image (GList **list)
{
	GList        *head;
	GthImageData *idata;

	if (*list == NULL)
		return NULL;

	head = *list;
	*list = g_list_remove_link (*list, head);
	idata = head->data;
	g_list_free (head);

	return idata;
}


static GList*
add_image_to_list (GList      *list,
		   GdkPixbuf  *pixbuf,
		   gboolean    unsaved)
{
	if (g_list_length (list) > MAX_UNDO_HISTORY_LEN) {
		GList *last;

		last = g_list_nth (list, MAX_UNDO_HISTORY_LEN - 1);
		if (last->prev != NULL) {
			last->prev->next = NULL;
			gth_image_data_list_free (last);
		}
	}

	if (pixbuf == NULL)
		return list;

	return g_list_prepend (list, gth_image_data_new (pixbuf, unsaved));
}


static void
add_image_to_undo_history (GthImageHistory *history,
		   	   GdkPixbuf       *pixbuf,
		   	   gboolean         unsaved)
{
	history->priv->undo_history = add_image_to_list (history->priv->undo_history,
						         pixbuf,
						         unsaved);
}


static void
add_image_to_redo_history (GthImageHistory *history,
	   		   GdkPixbuf       *pixbuf,
	   		   gboolean         unsaved)
{
	history->priv->redo_history = add_image_to_list (history->priv->redo_history,
						         pixbuf,
						         unsaved);
}


void
gth_image_history_add_image (GthImageHistory *history,
			     GdkPixbuf       *image,
			     gboolean         unsaved)
{
	add_image_to_undo_history (history, image, unsaved);
	gth_image_data_list_free (history->priv->redo_history);
	history->priv->redo_history = NULL;

	g_signal_emit (G_OBJECT (history),
		       gth_image_history_signals[CHANGED],
		       0);
}


GthImageData *
gth_image_history_undo (GthImageHistory *history,
			GdkPixbuf       *current_image,
			gboolean         image_is_unsaved)
{
	GthImageData *idata;

	if (history->priv->undo_history == NULL)
		return NULL;

	add_image_to_redo_history (history, current_image, image_is_unsaved);
	idata = remove_first_image (&(history->priv->undo_history));

	g_signal_emit (G_OBJECT (history),
		       gth_image_history_signals[CHANGED],
		       0);

	return idata;
}


GthImageData *
gth_image_history_redo (GthImageHistory *history,
			GdkPixbuf       *current_image,
			gboolean         image_is_unsaved)
{
	GthImageData *idata;

	if (history->priv->redo_history == NULL)
		return NULL;

	add_image_to_undo_history (history, current_image, image_is_unsaved);
	idata = remove_first_image (&(history->priv->redo_history));

	g_signal_emit (G_OBJECT (history),
		       gth_image_history_signals[CHANGED],
		       0);

	return idata;
}


void
gth_image_history_clear (GthImageHistory *history)
{
	gth_image_data_list_free (history->priv->undo_history);
	history->priv->undo_history = NULL;

	gth_image_data_list_free (history->priv->redo_history);
	history->priv->redo_history = NULL;
}


gboolean
gth_image_history_can_undo (GthImageHistory *history)
{
	return history->priv->undo_history != NULL;
}


gboolean
gth_image_history_can_redo (GthImageHistory *history)
{
	return history->priv->redo_history != NULL;
}
