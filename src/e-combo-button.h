/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* e-combo-button.h
 *
 * Copyright (C) 2001  Ximian, Inc.
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
 * Author: Ettore Perazzoli <ettore@ximian.com>
 */

/* Modified by Paolo Bacchilega <paolo.bacch@tin.it> */

#ifndef _E_COMBO_BUTTON_H_
#define _E_COMBO_BUTTON_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtkbutton.h>
#include <gtk/gtkmenu.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include "typedefs.h"

#ifdef __cplusplus
extern "C" {
#pragma }
#endif /* __cplusplus */

#define E_TYPE_COMBO_BUTTON			(e_combo_button_get_type ())
#define E_COMBO_BUTTON(obj)			(GTK_CHECK_CAST ((obj), E_TYPE_COMBO_BUTTON, EComboButton))
#define E_COMBO_BUTTON_CLASS(klass)		(GTK_CHECK_CLASS_CAST ((klass), E_TYPE_COMBO_BUTTON, EComboButtonClass))
#define E_IS_COMBO_BUTTON(obj)			(GTK_CHECK_TYPE ((obj), E_TYPE_COMBO_BUTTON))
#define E_IS_COMBO_BUTTON_CLASS(klass)		(GTK_CHECK_CLASS_TYPE ((obj), E_TYPE_COMBO_BUTTON))


typedef struct _EComboButton        EComboButton;
typedef struct _EComboButtonPrivate EComboButtonPrivate;
typedef struct _EComboButtonClass   EComboButtonClass;

struct _EComboButton {
	GtkButton parent;

	EComboButtonPrivate *priv;
};

struct _EComboButtonClass {
	GtkButtonClass parent_class;

	/* Signals.  */
	void (* activate_default) (EComboButton *combo_button);
};


GType      e_combo_button_get_type   (void);
void       e_combo_button_construct  (EComboButton *combo_button);
GtkWidget *e_combo_button_new        (void);

void       e_combo_button_set_icon   (EComboButton *combo_button,
				      GdkPixbuf    *pixbuf);
void       e_combo_button_set_label  (EComboButton *combo_button,
				      const char   *label);
void       e_combo_button_set_menu   (EComboButton *combo_button,
				      GtkMenu      *menu);
void       e_combo_button_popup_menu (EComboButton *combo_button);

void       e_combo_button_set_style  (EComboButton    *combo_button,
				      GthToolbarStyle  toolbar_style);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _E_COMBO_BUTTON_H_ */
