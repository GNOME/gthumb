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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <glib-object.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkimage.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkmain.h>
#include "gconf-utils.h"
#include "gth-toggle-button.h"
#include "gtkorientationbox.h"
#include "glib-utils.h"
#include "gthumb-marshal.h"


struct _GthToggleButtonPrivate {
	GdkPixbuf *icon;

	GtkWidget *icon_image;
	GtkWidget *label;
	GtkWidget *orient_box;

	gboolean button_active;

	GthToolbarStyle toolbar_style;
};

#define SPACING       2
#define ARROW_SPACING 5


#define PARENT_TYPE gtk_toggle_button_get_type ()
static GtkToggleButtonClass *parent_class = NULL;


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


static void
set_icon (GthToggleButton *toggle_button,
	  GdkPixbuf       *pixbuf)
{
	GthToggleButtonPrivate *priv;

	priv = toggle_button->priv;

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


static void
gth_toggle_button_destroy (GtkObject *object)
{
	GthToggleButton *toggle_button;

	toggle_button = GTH_TOGGLE_BUTTON (object);

	if (toggle_button->priv != NULL) {
		GthToggleButtonPrivate *priv = toggle_button->priv;

		if (priv->icon != NULL) {
			g_object_unref (priv->icon);
			priv->icon = NULL;
		}
		
		g_free (priv);
		toggle_button->priv = NULL;
	}

	(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}


static void
class_init (GtkObjectClass *object_class)
{
	parent_class = g_type_class_peek_parent (object_class);
	object_class->destroy = gth_toggle_button_destroy;
}


static void
object_init (GthToggleButton *toggle_button)
{
	GthToggleButtonPrivate *priv;

	priv = g_new (GthToggleButtonPrivate, 1);
	toggle_button->priv = priv;

	priv->orient_box = gtk_orientation_box_new (FALSE, SPACING, GTK_ORIENTATION_VERTICAL);
	gtk_container_set_border_width (GTK_CONTAINER (priv->orient_box), 0);
	gtk_container_add (GTK_CONTAINER (toggle_button), priv->orient_box);
	gtk_widget_show (priv->orient_box);

	/**/

	priv->icon_image = create_empty_image_widget ();
	gtk_box_pack_start (GTK_BOX (priv->orient_box), priv->icon_image, TRUE, TRUE, 0);
	gtk_widget_show (priv->icon_image);

	/**/

	priv->label = gtk_label_new ("");
	gtk_label_set_use_underline (GTK_LABEL (priv->label), TRUE);
	gtk_box_pack_start (GTK_BOX (priv->orient_box), priv->label, FALSE, TRUE, 0);
	gtk_widget_show (priv->label);

	priv->icon           = NULL;
	priv->button_active  = TRUE;
}


void
gth_toggle_button_construct (GthToggleButton *toggle_button)
{
	GthToggleButtonPrivate *priv;

	g_return_if_fail (toggle_button != NULL);
	g_return_if_fail (GTH_IS_TOGGLE_BUTTON (toggle_button));

	priv = toggle_button->priv;

	GTK_WIDGET_UNSET_FLAGS (toggle_button, GTK_CAN_FOCUS);

	gtk_button_set_relief (GTK_BUTTON (toggle_button), GTK_RELIEF_NONE);
}


GtkWidget *
gth_toggle_button_new (void)
{
	GthToggleButton *new;

	new = g_object_new (GTH_TYPE_TOGGLE_BUTTON, NULL);
	gth_toggle_button_construct (new);

	return GTK_WIDGET (new);
}


void
gth_toggle_button_set_icon (GthToggleButton *toggle_button,
			    GdkPixbuf       *pixbuf)
{
	g_return_if_fail (toggle_button != NULL);
	g_return_if_fail (GTH_IS_TOGGLE_BUTTON (toggle_button));

	set_icon (toggle_button, pixbuf);
}


void
gth_toggle_button_set_label (GthToggleButton *toggle_button,
			     const char      *label)
{
	GthToggleButtonPrivate *priv;

	g_return_if_fail (toggle_button != NULL);
	g_return_if_fail (GTH_IS_TOGGLE_BUTTON (toggle_button));
	g_return_if_fail (label != NULL);

	priv = toggle_button->priv;

	if (label == NULL)
		label = "";

	gtk_label_set_text (GTK_LABEL (priv->label), label);
}


GType
gth_toggle_button_get_type ()
{
        static GType type = 0;
	
        if (! type) {
                GTypeInfo type_info = {
			sizeof (GthToggleButtonClass),
			NULL,
			NULL,
			(GClassInitFunc) class_init,
			NULL,
			NULL,
			sizeof (GthToggleButton),
			0,
			(GInstanceInitFunc) object_init
		};

		type = g_type_register_static (PARENT_TYPE,
					       "GthToggleButton",
					       &type_info,
					       0);
	}

        return type;
}


void
gth_toggle_button_set_style  (GthToggleButton *toggle_button,
			      GthToolbarStyle  toolbar_style)
{
	GthToggleButtonPrivate *priv;

	g_return_if_fail (toggle_button != NULL);
	g_return_if_fail (GTH_IS_TOGGLE_BUTTON (toggle_button));

	priv = toggle_button->priv;

	if (toolbar_style == GTH_TOOLBAR_STYLE_SYSTEM) {
		char *system_style;

		system_style = eel_gconf_get_string ("/desktop/gnome/interface/toolbar_style", "system");

		if (system_style == NULL)
			toolbar_style = GTH_TOOLBAR_STYLE_TEXT_BELOW;
		else if (strcmp (system_style, "both") == 0)
			toolbar_style = GTH_TOOLBAR_STYLE_TEXT_BELOW;
		else if (strcmp (system_style, "both-horiz") == 0)
			toolbar_style = GTH_TOOLBAR_STYLE_TEXT_BESIDE;
		else if (strcmp (system_style, "icons") == 0)
			toolbar_style = GTH_TOOLBAR_STYLE_ICONS;
		else if (strcmp (system_style, "text") == 0)
			toolbar_style = GTH_TOOLBAR_STYLE_TEXT;
		else
			toolbar_style = GTH_TOOLBAR_STYLE_TEXT_BELOW;

		g_free (system_style);
	}

	priv->toolbar_style = toolbar_style;

	switch (toolbar_style) {
	case GTH_TOOLBAR_STYLE_TEXT_BELOW:
		gtk_widget_show (priv->label);
		if (toggle_button->priv->button_active)
			gtk_widget_show (priv->icon_image);
		gtk_orientation_box_set_orient (GTK_ORIENTATION_BOX (priv->orient_box),
						GTK_ORIENTATION_VERTICAL);
		break;

	case GTH_TOOLBAR_STYLE_TEXT_BESIDE:
		gtk_widget_show (priv->label);
		if (toggle_button->priv->button_active)
			gtk_widget_show (priv->icon_image);
		gtk_orientation_box_set_orient (GTK_ORIENTATION_BOX (priv->orient_box),
						GTK_ORIENTATION_HORIZONTAL);
		break;

	case GTH_TOOLBAR_STYLE_ICONS:
		gtk_widget_hide (priv->label);
		if (toggle_button->priv->button_active)
			gtk_widget_show (priv->icon_image);
		gtk_orientation_box_set_orient (GTK_ORIENTATION_BOX (priv->orient_box),
						GTK_ORIENTATION_HORIZONTAL);
		break;
		break;

	case GTH_TOOLBAR_STYLE_TEXT:
		gtk_widget_show (priv->label);
		if (toggle_button->priv->button_active)
			gtk_widget_hide (priv->icon_image);
		gtk_orientation_box_set_orient (GTK_ORIENTATION_BOX (priv->orient_box),
						GTK_ORIENTATION_HORIZONTAL);
		break;

	default:
		break;
	}
}
