/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2003 Free Software Foundation, Inc.
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

#ifndef GTH_IMAGE_SELECTOR_H
#define GTH_IMAGE_SELECTOR_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GTH_IMAGE_SELECTOR_TYPE            (gth_image_selector_get_type ())
#define GTH_IMAGE_SELECTOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTH_IMAGE_SELECTOR_TYPE, GthImageSelector))
#define GTH_IMAGE_SELECTOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTH_IMAGE_SELECTOR_TYPE, GthImageSelectorClass))
#define GTH_IS_IMAGE_SELECTOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTH_IMAGE_SELECTOR_TYPE))
#define GTH_IS_IMAGE_SELECTOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTH_IMAGE_SELECTOR_TYPE))
#define GTH_IMAGE_SELECTOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GTH_IMAGE_SELECTOR_TYPE, GthImageSelectorClass))

typedef struct _GthImageSelector       GthImageSelector;
typedef struct _GthImageSelectorClass  GthImageSelectorClass;
typedef struct _GthImageSelectorPriv   GthImageSelectorPriv; 

struct _GthImageSelector
{
	GtkWidget __parent;
	GthImageSelectorPriv *priv;
};

struct _GthImageSelectorClass
{
	GtkWidgetClass __parent_class;

	/* -- Signals -- */

	void (* selection_changed) (GthImageSelector *selector);
};


GType          gth_image_selector_get_type       (void);

GtkWidget*     gth_image_selector_new            (GdkPixbuf        *pixbuf);

void           gth_image_selector_set_pixbuf     (GthImageSelector *selector, 
						  GdkPixbuf        *pixbuf);

GdkPixbuf*     gth_image_selector_get_pixbuf     (GthImageSelector *selector);

void           gth_image_selector_set_selection  (GthImageSelector *selector,
						  GdkRectangle      selection);

void           gth_image_selector_get_selection  (GthImageSelector *selector,
						  GdkRectangle     *selection);

void           gth_image_selector_set_ratio      (GthImageSelector *selector,
						  gboolean          use_ratio,
						  double            ratio);

double         gth_image_selector_get_ratio      (GthImageSelector *selector);

gboolean       gth_image_selector_get_use_ratio  (GthImageSelector *selector);

G_END_DECLS

#endif /* GTH_IMAGE_SELECTOR_H */
