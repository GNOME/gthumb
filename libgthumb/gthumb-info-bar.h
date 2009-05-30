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

#ifndef GTHUMB_INFO_BAR_H
#define GTHUMB_INFO_BAR_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GTHUMB_TYPE_INFO_BAR            (gthumb_info_bar_get_type ())
#define GTHUMB_INFO_BAR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTHUMB_TYPE_INFO_BAR, GThumbInfoBar))
#define GTHUMB_INFO_BAR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTHUMB_TYPE_INFO_BAR, GThumbInfoBarClass))
#define GTHUMB_IS_INFO_BAR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTHUMB_TYPE_INFO_BAR))
#define GTHUMB_IS_INFO_BAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTHUMB_TYPE_INFO_BAR))
#define GTHUMB_INFO_BAR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GTHUMB_TYPE_INFO_BAR, GThumbInfoBarClass))

typedef struct _GThumbInfoBar          GThumbInfoBar;
typedef struct _GThumbInfoBarClass     GThumbInfoBarClass;
typedef struct _GThumbInfoBarPrivate   GThumbInfoBarPrivate;

struct _GThumbInfoBar 
{
	GtkEventBox __parent;
	GThumbInfoBarPrivate *priv;
};

struct _GThumbInfoBarClass
{
	GtkEventBoxClass __parent;
};

GType        gthumb_info_bar_get_type             (void);

GtkWidget*     gthumb_info_bar_new                ();

void           gthumb_info_bar_set_focused        (GThumbInfoBar *info_bar,
						   gboolean       focused);

void           gthumb_info_bar_set_text           (GThumbInfoBar *info_bar,
						   const char    *text,
						   const char    *tooltip);

#endif /* GTHUMB_INFO_BAR_H */
