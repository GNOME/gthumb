/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* e-combo-button.c
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

/*
 * Ported to gtk 2 and modified a bit.
 * Author: Paolo bacchilega <paolo.bacch@tin.it> 
 */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib-object.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkimage.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkmain.h>
#include "e-combo-button.h"
#include "glib-utils.h"
#include "gthumb-marshal.h"


struct _EComboButtonPrivate {
	GdkPixbuf *icon;

	GtkWidget *icon_image;
	GtkWidget *label;
	GtkWidget *arrow_image;
	GtkWidget *hbox;

	GtkMenu *menu;

	gboolean menu_popped_up;
};


#define SPACING 2

static const char *arrow_xpm[] = {
	"11 5  2 1",
	" 	c none",
	".	c #000000000000",
	" ......... ",
	"  .......  ",
	"   .....   ",
	"    ...    ",
	"     .     ",
};


#define PARENT_TYPE gtk_button_get_type ()
static GtkButtonClass *parent_class = NULL;

enum {
	ACTIVATE_DEFAULT,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };


/* Utility functions.  */


static GtkWidget *
create_empty_image_widget (void)
{
	GtkWidget *image_widget;
	GdkPixbuf *pixbuf;

	pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, 1, 1);
	image_widget = gtk_image_new_from_pixbuf (pixbuf);
	g_object_unref (pixbuf);

	return image_widget;
}

static GtkWidget *
create_arrow_image_widget (void)
{
	GtkWidget *image_widget;
	GdkPixbuf *pixbuf;

	pixbuf = gdk_pixbuf_new_from_xpm_data (arrow_xpm);
	image_widget = gtk_image_new_from_pixbuf (pixbuf);
	g_object_unref (pixbuf);

	return image_widget;
}

static void
set_icon (EComboButton *combo_button,
	  GdkPixbuf *pixbuf)
{
	EComboButtonPrivate *priv;

	priv = combo_button->priv;

	if (priv->icon != NULL)
		g_object_unref (priv->icon);

	if (pixbuf == NULL) {
		priv->icon = NULL;
		gtk_widget_hide (priv->icon_image);
		return;
	}

	priv->icon = g_object_ref (pixbuf);
	gtk_image_set_from_pixbuf (GTK_IMAGE (priv->icon_image), pixbuf);
	gtk_widget_show (priv->icon_image);
}


/* Paint the borders.  */

static void
paint (EComboButton *combo_button,
       GdkRectangle *area)
{
	EComboButtonPrivate *priv = combo_button->priv;
	GtkShadowType shadow_type;
	int separator_x;

	gdk_window_set_back_pixmap (GTK_WIDGET (combo_button)->window, NULL, TRUE);
	gdk_window_clear_area (GTK_WIDGET (combo_button)->window,
			       area->x, area->y,
			       area->width, area->height);

	/* Only paint the outline if we are in prelight state.  */
	if (GTK_WIDGET_STATE (combo_button) != GTK_STATE_PRELIGHT
	    && GTK_WIDGET_STATE (combo_button) != GTK_STATE_ACTIVE)
		return;

	separator_x = (priv->label->allocation.width
		       + priv->label->allocation.x
		       + priv->arrow_image->allocation.x) / 2;

	if (GTK_WIDGET_STATE (combo_button) == GTK_STATE_ACTIVE)
		shadow_type = GTK_SHADOW_IN;
	else
		shadow_type = GTK_SHADOW_OUT;

	gtk_paint_box (GTK_WIDGET (combo_button)->style,
		       GTK_WIDGET (combo_button)->window,
		       GTK_STATE_PRELIGHT,
		       shadow_type,
		       area,
		       GTK_WIDGET (combo_button),
		       "button",
		       0,
		       0,
		       GTK_WIDGET (combo_button)->allocation.width,
		       GTK_WIDGET (combo_button)->allocation.height);

	/*
	gtk_paint_box (GTK_WIDGET (combo_button)->style,
		       GTK_WIDGET (combo_button)->window,
		       GTK_STATE_PRELIGHT,
		       shadow_type,
		       area,
		       GTK_WIDGET (combo_button),
		       "button",
		       0,
		       0,
		       separator_x,
		       GTK_WIDGET (combo_button)->allocation.height);

	gtk_paint_box (GTK_WIDGET (combo_button)->style,
		       GTK_WIDGET (combo_button)->window,
		       GTK_STATE_PRELIGHT,
		       shadow_type,
		       area,
		       GTK_WIDGET (combo_button),
		       "button",
		       separator_x,
		       0,
		       GTK_WIDGET (combo_button)->allocation.width - separator_x,
		       GTK_WIDGET (combo_button)->allocation.height);
	*/
}


/* Callbacks for the associated menu.  */

static void
menu_detacher (GtkWidget *widget,
	       GtkMenu *menu)
{
	EComboButton *combo_button;
	EComboButtonPrivate *priv;

	combo_button = E_COMBO_BUTTON (widget);
	priv = combo_button->priv;

	g_signal_handlers_disconnect_by_data (G_OBJECT (menu), combo_button);
	priv->menu = NULL;
}

static void
menu_deactivate_callback (GtkMenuShell *menu_shell,
			  void *data)
{
	EComboButton *combo_button;
	EComboButtonPrivate *priv;

	combo_button = E_COMBO_BUTTON (data);
	priv = combo_button->priv;

	priv->menu_popped_up = FALSE;

	GTK_BUTTON (combo_button)->button_down = FALSE;
	GTK_BUTTON (combo_button)->in_button = FALSE;
	gtk_button_leave (GTK_BUTTON (combo_button));
}

static void
menu_position_func (GtkMenu *menu,
		    gint *x_return,
		    gint *y_return,
		    gboolean *push_in,
		    void *data)
{
	EComboButton *combo_button;
	GtkAllocation *allocation;

	combo_button = E_COMBO_BUTTON (data);
	allocation = & (GTK_WIDGET (combo_button)->allocation);

	gdk_window_get_origin (GTK_WIDGET (combo_button)->window, x_return, y_return);

	*y_return += allocation->height;
}


/* GtkObject methods.  */

static void
impl_destroy (GtkObject *object)
{
	EComboButton *combo_button;
	EComboButtonPrivate *priv;

	combo_button = E_COMBO_BUTTON (object);
	priv = combo_button->priv;

	if (priv->arrow_image != NULL) {
		gtk_widget_destroy (priv->arrow_image);
		priv->arrow_image = NULL;
	}

	if (priv->icon != NULL) {
		g_object_unref (priv->icon);
		priv->icon = NULL;
	}

	g_free (priv);

	(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}


/* GtkWidget methods.  */

static int
impl_button_press_event (GtkWidget *widget,
			 GdkEventButton *event)
{
	EComboButton *combo_button;
	EComboButtonPrivate *priv;

	combo_button = E_COMBO_BUTTON (widget);
	priv = combo_button->priv;

	if (event->type == GDK_BUTTON_PRESS && 
	    (event->button == 1 || event->button == 3)) {
		GTK_BUTTON (widget)->button_down = TRUE;

		if (event->button == 3 || 
		    event->x >= priv->arrow_image->allocation.x) {
			/* User clicked on the right side: pop up the menu.  */
			gtk_button_pressed (GTK_BUTTON (widget));

			priv->menu_popped_up = TRUE;
			gtk_menu_popup (GTK_MENU (priv->menu), NULL, NULL,
					menu_position_func, combo_button,
					event->button, event->time);
		} else {
			/* User clicked on the left side: just behave like a
			   normal button (i.e. not a toggle).  */
			gtk_grab_add (widget);
			gtk_button_pressed (GTK_BUTTON (widget));
		}
	}

	return TRUE;
}

static int
impl_leave_notify_event (GtkWidget *widget,
			 GdkEventCrossing *event)
{
	EComboButton *combo_button;
	EComboButtonPrivate *priv;

	combo_button = E_COMBO_BUTTON (widget);
	priv = combo_button->priv;

	/* This is to override the standard behavior of deactivating the button
	   when the pointer gets out of the widget, in the case in which we
	   have just popped up the menu.  Otherwise, the button would look as
	   inactive when the menu is popped up.  */
	if (! priv->menu_popped_up)
		return (* GTK_WIDGET_CLASS (parent_class)->leave_notify_event) (widget, event);

	return FALSE;
}

static int
impl_expose_event (GtkWidget *widget,
		   GdkEventExpose *event)
{
	GtkBin *bin;
	GdkEventExpose child_event;

	if (! GTK_WIDGET_DRAWABLE (widget))
		return FALSE;

	bin = GTK_BIN (widget);
      
	paint (E_COMBO_BUTTON (widget), &event->area);

	child_event = *event;
	if (bin->child && GTK_WIDGET_NO_WINDOW (bin->child) &&
	    gtk_widget_intersect (bin->child, &event->area, &child_event.area))
		gtk_container_propagate_expose (GTK_CONTAINER (widget), bin->child, &child_event);

	return FALSE;
}


/* GtkButton methods.  */

static void
impl_released (GtkButton *button)
{
	EComboButton *combo_button;
	EComboButtonPrivate *priv;

	combo_button = E_COMBO_BUTTON (button);
	priv = combo_button->priv;

	/* Massive cut & paste from GtkButton here...  The only change in
	   behavior here is that we want to emit ::activate_default when not
	   the menu hasn't been popped up.  */

	if (button->button_down) {
		int      new_state;
		gboolean activate_default = FALSE;

		gtk_grab_remove (GTK_WIDGET (button));
		button->button_down = FALSE;

		if (button->in_button) {
			gtk_button_clicked (button);

			if (! priv->menu_popped_up)
				activate_default = TRUE;
		}

		new_state = (button->in_button ? GTK_STATE_PRELIGHT : GTK_STATE_NORMAL);

		if (GTK_WIDGET_STATE (button) != new_state) {
			gtk_widget_set_state (GTK_WIDGET (button), new_state);

			/* We _draw () instead of queue_draw so that if the
			   operation blocks, the label doesn't vanish.  
			 FIXME */
			gtk_widget_queue_draw (GTK_WIDGET (button)); 
		}

		if (activate_default)
			g_signal_emit (G_OBJECT (button), signals[ACTIVATE_DEFAULT], 0);
	}
}


static void
class_init (GtkObjectClass *object_class)
{
	GtkWidgetClass *widget_class;
	GtkButtonClass *button_class;

	parent_class = g_type_class_peek_parent (object_class);

	object_class->destroy = impl_destroy;

	widget_class = GTK_WIDGET_CLASS (object_class);
	widget_class->button_press_event = impl_button_press_event;
	widget_class->leave_notify_event = impl_leave_notify_event;
	widget_class->expose_event       = impl_expose_event;

	button_class = GTK_BUTTON_CLASS (object_class);
	button_class->released = impl_released;

	signals[ACTIVATE_DEFAULT] = 
		g_signal_new ("activate_default",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (EComboButtonClass, activate_default),
			      NULL, NULL,
			      gthumb_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
}

static void
init (EComboButton *combo_button)
{
	EComboButtonPrivate *priv;

	priv = g_new (EComboButtonPrivate, 1);
	combo_button->priv = priv;

	priv->hbox = gtk_hbox_new (FALSE, SPACING);
	gtk_container_add (GTK_CONTAINER (combo_button), priv->hbox);
	gtk_widget_show (priv->hbox);

	priv->icon_image = create_empty_image_widget ();
	gtk_box_pack_start (GTK_BOX (priv->hbox), priv->icon_image, TRUE, TRUE, 0);
	gtk_widget_show (priv->icon_image);

	priv->label = gtk_label_new ("");
	gtk_label_set_use_underline (GTK_LABEL (priv->label), TRUE);
	gtk_box_pack_start (GTK_BOX (priv->hbox), priv->label, TRUE, TRUE,
			    2 * GTK_WIDGET (combo_button)->style->xthickness);
	gtk_widget_show (priv->label);

	priv->arrow_image = create_arrow_image_widget ();
	gtk_box_pack_start (GTK_BOX (priv->hbox), priv->arrow_image, TRUE, TRUE,
			    GTK_WIDGET (combo_button)->style->xthickness);
	gtk_widget_show (priv->arrow_image);

	priv->icon           = NULL;
	priv->menu           = NULL;
	priv->menu_popped_up = FALSE;
}


void
e_combo_button_construct (EComboButton *combo_button)
{
	EComboButtonPrivate *priv;

	g_return_if_fail (combo_button != NULL);
	g_return_if_fail (E_IS_COMBO_BUTTON (combo_button));

	priv = combo_button->priv;
	g_return_if_fail (priv->menu == NULL);

	GTK_WIDGET_UNSET_FLAGS (combo_button, GTK_CAN_FOCUS);

	gtk_button_set_relief (GTK_BUTTON (combo_button), GTK_RELIEF_NONE);
}

GtkWidget *
e_combo_button_new (void)
{
	EComboButton *new;

	new = g_object_new (E_TYPE_COMBO_BUTTON, NULL);
	/* new = gtk_type_new (e_combo_button_get_type ()); FIXME */
	e_combo_button_construct (new);

	return GTK_WIDGET (new);
}


void
e_combo_button_set_icon (EComboButton *combo_button,
			 GdkPixbuf *pixbuf)
{
	g_return_if_fail (combo_button != NULL);
	g_return_if_fail (E_IS_COMBO_BUTTON (combo_button));

	set_icon (combo_button, pixbuf);
}

void
e_combo_button_set_label (EComboButton *combo_button,
			  const char *label)
{
	EComboButtonPrivate *priv;

	g_return_if_fail (combo_button != NULL);
	g_return_if_fail (E_IS_COMBO_BUTTON (combo_button));
	g_return_if_fail (label != NULL);

	priv = combo_button->priv;

	if (label == NULL)
		label = "";

	gtk_label_set_text (GTK_LABEL (priv->label), label);
}

void
e_combo_button_set_menu (EComboButton *combo_button,
			 GtkMenu *menu)
{
	EComboButtonPrivate *priv;

	g_return_if_fail (combo_button != NULL);
	g_return_if_fail (E_IS_COMBO_BUTTON (combo_button));
	g_return_if_fail (menu != NULL);
	g_return_if_fail (GTK_IS_MENU (menu));

	priv = combo_button->priv;

	if (priv->menu != NULL)
		gtk_menu_detach (priv->menu);

	priv->menu = menu;
	if (menu == NULL)
		return;

	gtk_menu_attach_to_widget (menu, GTK_WIDGET (combo_button), menu_detacher);

	g_signal_connect (G_OBJECT (menu), 
			  "deactivate",
			  G_CALLBACK (menu_deactivate_callback),
			  combo_button);
}

void
e_combo_button_popup_menu (EComboButton *combo_button)
{
	EComboButtonPrivate *priv;

	g_return_if_fail (combo_button != NULL);
	g_return_if_fail (E_IS_COMBO_BUTTON (combo_button));

	priv = combo_button->priv;

	g_return_if_fail (priv->menu != NULL);
	g_return_if_fail (GTK_IS_MENU (priv->menu));

	gtk_button_pressed (GTK_BUTTON (combo_button));
	
	priv->menu_popped_up = TRUE;
	gtk_menu_popup (GTK_MENU (priv->menu), NULL, NULL,
			menu_position_func, combo_button,
			1, 
			GDK_CURRENT_TIME);
}

GType
e_combo_button_get_type ()
{
        static guint type = 0;
	
        if (! type) {
                GTypeInfo type_info = {
			sizeof (EComboButtonClass),
			NULL,
			NULL,
			(GClassInitFunc) class_init,
			NULL,
			NULL,
			sizeof (EComboButton),
			0,
			(GInstanceInitFunc) init
		};

		type = g_type_register_static (PARENT_TYPE,
					       "EComboButton",
					       &type_info,
					       0);
	}

        return type;
}
