/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* 
 * GThumb
 *
 * Copyright (C) 2003  Free Software Foundation, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author: Paolo Bacchilega
 */

#ifndef _GTH_TOGGLE_BUTTON_H_
#define _GTH_TOGGLE_BUTTON_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtktogglebutton.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "typedefs.h"

#ifdef __cplusplus
extern "C" {
#pragma }
#endif /* __cplusplus */

#define GTH_TYPE_TOGGLE_BUTTON			(gth_toggle_button_get_type ())
#define GTH_TOGGLE_BUTTON(obj)			(GTK_CHECK_CAST ((obj), GTH_TYPE_TOGGLE_BUTTON, GthToggleButton))
#define GTH_TOGGLE_BUTTON_CLASS(klass)		(GTK_CHECK_CLASS_CAST ((klass), GTH_TYPE_TOGGLE_BUTTON, GthToggleButtonClass))
#define GTH_IS_TOGGLE_BUTTON(obj)		(GTK_CHECK_TYPE ((obj), GTH_TYPE_TOGGLE_BUTTON))
#define GTH_IS_TOGGLE_BUTTON_CLASS(klass)	(GTK_CHECK_CLASS_TYPE ((obj), GTH_TYPE_TOGGLE_BUTTON))


typedef struct _GthToggleButton        GthToggleButton;
typedef struct _GthToggleButtonPrivate GthToggleButtonPrivate;
typedef struct _GthToggleButtonClass   GthToggleButtonClass;

struct _GthToggleButton {
	GtkToggleButton parent;
	GthToggleButtonPrivate *priv;
};

struct _GthToggleButtonClass {
	GtkToggleButtonClass parent_class;
};



GType      gth_toggle_button_get_type           (void);

void       gth_toggle_button_construct          (GthToggleButton *toggle_button);

GtkWidget *gth_toggle_button_new                (void);

void       gth_toggle_button_set_icon           (GthToggleButton *toggle_button,
						 GdkPixbuf       *pixbuf);

void       gth_toggle_button_set_label          (GthToggleButton *toggle_button,
						 const char      *label);

void       gth_toggle_button_set_style          (GthToggleButton *toggle_button,
						 GthToolbarStyle  toolbar_style);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _GTH_TOGGLE_BUTTON_H_ */
