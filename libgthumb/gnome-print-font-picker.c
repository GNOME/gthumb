/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gnome-print-font-picker.c
 * This file is part of gedit
 *
 * Copyright (C) 2002  Paolo Maggi 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, 
 * Boston, MA 02111-1307, USA.
 */

/*
 * This widget is based on GnomeFontPicker
 */

/* GNOME font picker button.
 * Copyright (C) 1998 David Abilleira Freijeiro <odaf@nexo.es>
 * All rights reserved.
 *
 * Based on gnome-color-picker by Federico Mena <federico@nuclecu.unam.mx>
 */

/*
 * Modified by the gedit Team, 2002. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libgnome/gnome-macros.h>

/* Must be before all other gnome includes!! */
#include <libgnome/gnome-i18n.h>

#include <gtk/gtksignal.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkalignment.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkvseparator.h>
#include <gtk/gtkfontsel.h>
#include <gtk/gtkimage.h>
#include <gtk/gtkstock.h>

#include <gnome-print-font-dialog.h>
/*
#include "gnometypebuiltins.h"
*/

#include "gnome-print-font-picker.h"

#include <string.h>
#include <stdio.h>

struct _GnomePrintFontPickerPrivate {
        gchar         *title;

        gchar         *font_name;
	GnomeFont     *font;
	
        gchar         *preview_text;

        int           use_font_in_label_size;

        GnomePrintFontPickerMode mode : 2;

        /* Only for GNOME_PRINT_FONT_PICKER_MODE_FONT_INFO */
        gboolean      use_font_in_label : 1;
        gboolean      show_size : 1;

	GtkWidget     *font_dialog;
        GtkWidget     *inside;
        GtkWidget     *font_label, *vsep, *size_label;
};


#define DEF_FONT_NAME N_("Sans Regular 12")
#define DEF_PREVIEW_TEXT N_("AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz")
#define DEF_TITLE N_("Pick a Font")

/* Signals */
enum {
	FONT_SET,
	LAST_SIGNAL
};

enum {
	PROP_0,
	PROP_TITLE,
	PROP_MODE,
	PROP_FONT_NAME,
	PROP_PREVIEW_TEXT,
	PROP_USE_FONT_IN_LABEL,
	PROP_LABEL_FONT_SIZE,
	PROP_SHOW_SIZE,
	PROP_FONT
};

/* Prototypes */
static void gnome_print_font_picker_destroy		(GtkObject            *object);
static void gnome_print_font_picker_finalize		(GObject              *object);
static void gnome_print_font_picker_get_property	(GObject              *object,
						 	 guint                 param_id,
						 	 GValue               *value,
							 GParamSpec           *pspec);
static void gnome_print_font_picker_set_property	(GObject              *object,
							 guint                 param_id,
							 const GValue         *value,
							 GParamSpec           *pspec);

static void gnome_print_font_picker_clicked		(GtkButton            *button);

/* Dialog response functions */
static void gnome_print_font_picker_dialog_ok_clicked 	(GtkWidget            *widget,
							 gpointer              data);
static void gnome_print_font_picker_dialog_cancel_clicked (GtkWidget          *widget,
						   	 gpointer              data);

static gboolean gnome_print_font_picker_dialog_delete_event (GtkWidget        *widget,
							 GdkEventAny          *ev,
							 gpointer              data);
static void gnome_print_font_picker_dialog_destroy 	(GtkWidget           *widget,
							 gpointer             data);

/* Auxiliary functions */
static GtkWidget *gnome_print_font_picker_create_inside (GnomePrintFontPicker   *gfs);

static void gnome_print_font_picker_label_use_font_in_label (GnomePrintFontPicker *gfs);
static void gnome_print_font_picker_update_font_info	(GnomePrintFontPicker     *gfs);




static guint font_picker_signals[LAST_SIGNAL] = { 0 };

GNOME_CLASS_BOILERPLATE (GnomePrintFontPicker, gnome_print_font_picker,
			 GtkButton, GTK_TYPE_BUTTON)

static void
gnome_print_font_picker_class_init (GnomePrintFontPickerClass *class)
{
	GtkObjectClass *object_class;
	GObjectClass *gobject_class;
	GtkButtonClass *button_class;

	object_class = (GtkObjectClass *) class;
	gobject_class = (GObjectClass *) class;
	button_class = (GtkButtonClass *) class;

	object_class->destroy = gnome_print_font_picker_destroy;

	gobject_class->finalize = gnome_print_font_picker_finalize;
	gobject_class->set_property = gnome_print_font_picker_set_property;
	gobject_class->get_property = gnome_print_font_picker_get_property;

	button_class->clicked = gnome_print_font_picker_clicked;

	class->font_set = NULL;

	g_object_class_install_property
		(gobject_class,
		 PROP_TITLE,
		 g_param_spec_string ("title",
				      _("Title"),
				      _("The title of the selection dialog box"),
				      _(DEF_TITLE),
				      (G_PARAM_READABLE |
				       G_PARAM_WRITABLE)));
/*
	g_object_class_install_property
		(gobject_class,
		 PROP_MODE,
		 g_param_spec_enum ("mode",
				    _("Mode"),
				    _("The mode of operation of the font picker"),
				    GnomePrintFontPickerMode,
				    GNOME_PRINT_FONT_PICKER_MODE_PIXMAP,
				    (G_PARAM_READABLE |
				     G_PARAM_WRITABLE)));
 */
	g_object_class_install_property
		(gobject_class,
		 PROP_FONT_NAME,
		 g_param_spec_string ("font_name",
				      _("Font name"),
				      _("Name of the selected font"),
				      _(DEF_FONT_NAME),
				      (G_PARAM_READABLE |
				       G_PARAM_WRITABLE)));
	g_object_class_install_property
		(gobject_class,
		 PROP_PREVIEW_TEXT,
		 g_param_spec_string ("preview_text",
				      _("Preview text"),
				      _("Preview text shown in the dialog"),
				      _(DEF_PREVIEW_TEXT),
				      (G_PARAM_READABLE |
				       G_PARAM_WRITABLE)));
	g_object_class_install_property
		(gobject_class,
		 PROP_USE_FONT_IN_LABEL,
		 g_param_spec_boolean ("use-font-in-label",
				       _("Use font in label"),
				       _("Use font in the label in font info mode"),
				       FALSE,
				       (G_PARAM_READABLE |
					G_PARAM_WRITABLE)));
	g_object_class_install_property
		(gobject_class,
		 PROP_LABEL_FONT_SIZE,
		 g_param_spec_int ("label-font-size",
				   _("Font size for label"),
				   _("Font size for label in font info mode"),
				   8 /* min */,
				   72 /* max */,
				   14 /* default */,
				   (G_PARAM_READABLE |
				    G_PARAM_WRITABLE)));
	g_object_class_install_property
		(gobject_class,
		 PROP_SHOW_SIZE,
		 g_param_spec_boolean ("show-size",
				      _("Show size"),
				      _("Show size in font info mode"),
				      TRUE,
				      (G_PARAM_READABLE |
				       G_PARAM_WRITABLE)));

	font_picker_signals[FONT_SET] =
		g_signal_new ("font_set",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (GnomePrintFontPickerClass, font_set),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__STRING,
			      G_TYPE_NONE, 1,
			      G_TYPE_STRING);

}

static void
gnome_print_font_picker_instance_init (GnomePrintFontPicker *gfp)
{

    gfp->_priv                         = g_new0(GnomePrintFontPickerPrivate, 1);

    /* Initialize fields */
    gfp->_priv->mode                   = GNOME_PRINT_FONT_PICKER_MODE_PIXMAP;
    gfp->_priv->font_name              = NULL;
    gfp->_priv->preview_text           = NULL;
    gfp->_priv->use_font_in_label      = FALSE;
    gfp->_priv->use_font_in_label_size = 14;
    gfp->_priv->show_size              = TRUE;
    gfp->_priv->font_dialog            = NULL;
    gfp->_priv->title                  = g_strdup(_(DEF_TITLE));


    /* Create pixmap or info widgets */
    gfp->_priv->inside = gnome_print_font_picker_create_inside(gfp);
    if (gfp->_priv->inside)
        gtk_container_add(GTK_CONTAINER(gfp), gfp->_priv->inside);

    gnome_print_font_picker_set_font_name(gfp, _(DEF_FONT_NAME));
    gnome_print_font_picker_set_preview_text(gfp, _(DEF_PREVIEW_TEXT));

    if (gfp->_priv->mode == GNOME_PRINT_FONT_PICKER_MODE_FONT_INFO) {
        /* Update */
        gnome_print_font_picker_update_font_info(gfp);
    }
}

static void
gnome_print_font_picker_destroy (GtkObject *object)
{
    GnomePrintFontPicker *gfp;

    g_return_if_fail (object != NULL);
    g_return_if_fail (GNOME_PRINT_IS_FONT_PICKER (object));

    /* remember, destroy can be run multiple times! */

    gfp = GNOME_PRINT_FONT_PICKER (object);

    if (gfp->_priv->font_dialog != NULL) {
	    gtk_widget_destroy (gfp->_priv->font_dialog);
	    gfp->_priv->font_dialog = NULL;
    }

    GNOME_CALL_PARENT (GTK_OBJECT_CLASS, destroy, (object));

} /* gnome_print_font_picker_destroy */

static void
gnome_print_font_picker_finalize (GObject *object)
{
    GnomePrintFontPicker *gfp;

    g_return_if_fail (object != NULL);
    g_return_if_fail (GNOME_PRINT_IS_FONT_PICKER (object));

    gfp = GNOME_PRINT_FONT_PICKER(object);

    /* g_free handles NULL */
    g_free(gfp->_priv->font_name);
    gfp->_priv->font_name = NULL;

    g_object_unref (gfp->_priv->font);
    gfp->_priv->font = NULL;

    /* g_free handles NULL */
    g_free(gfp->_priv->preview_text);
    gfp->_priv->preview_text = NULL;

    /* g_free handles NULL */
    g_free(gfp->_priv->title);
    gfp->_priv->title = NULL;

    /* g_free handles NULL */
    g_free(gfp->_priv);
    gfp->_priv = NULL;

    GNOME_CALL_PARENT (G_OBJECT_CLASS, finalize, (object));

} /* gnome_print_font_picker_finalize */

static void
gnome_print_font_picker_set_property (GObject *object,
				guint param_id,
				const GValue * value,
				GParamSpec * pspec)
{
	GnomePrintFontPicker *self;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_PRINT_IS_FONT_PICKER (object));

	self = GNOME_PRINT_FONT_PICKER (object);

	switch (param_id) {
	case PROP_TITLE:
		gnome_print_font_picker_set_title (self, g_value_get_string (value));
		break;
	case PROP_MODE:
		gnome_print_font_picker_set_mode (self, g_value_get_enum (value));
		break;
	case PROP_FONT_NAME:
		gnome_print_font_picker_set_font_name (self,
						 g_value_get_string (value));
		break;
	case PROP_PREVIEW_TEXT:
		gnome_print_font_picker_set_preview_text
			(self, g_value_get_string (value));
		break;
	case PROP_USE_FONT_IN_LABEL:
		gnome_print_font_picker_fi_set_use_font_in_label
			(self, g_value_get_boolean (value),
			 self->_priv->use_font_in_label_size);
		break;
	case PROP_LABEL_FONT_SIZE:
		gnome_print_font_picker_fi_set_use_font_in_label
			(self,
			 self->_priv->use_font_in_label,
			 g_value_get_int (value));
		break;
	case PROP_SHOW_SIZE:
		gnome_print_font_picker_fi_set_show_size
			(self, g_value_get_boolean (value));
		break;

	default:
		break;
	}
} /* gnome_print_font_picker_set_property */

static void
gnome_print_font_picker_get_property (GObject *object,
				guint param_id,
				GValue *value,
				GParamSpec * pspec)
{
	GnomePrintFontPicker *self;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_PRINT_IS_FONT_PICKER (object));

	self = GNOME_PRINT_FONT_PICKER (object);

	switch (param_id) {
	case PROP_TITLE:
		g_value_set_string (value, gnome_print_font_picker_get_title (self));
		break;
	case PROP_MODE:
		g_value_set_string (value, gnome_print_font_picker_get_title (self));
		break;
	case PROP_FONT_NAME:
		g_value_set_string (value,
				    gnome_print_font_picker_get_font_name (self));
		break;
	case PROP_PREVIEW_TEXT:
		g_value_set_string (value,
				    gnome_print_font_picker_get_preview_text (self));
		break;
	case PROP_USE_FONT_IN_LABEL:
		g_value_set_boolean (value,
				     self->_priv->use_font_in_label);
		break;
	case PROP_LABEL_FONT_SIZE:
		g_value_set_int (value,
				 self->_priv->use_font_in_label_size);
		break;
	case PROP_SHOW_SIZE:
		g_value_set_boolean (value,
				     self->_priv->show_size);
		break;

	default:
		break;
	}
} /* gnome_print_font_picker_get_property */

/*************************************************************************
 Public functions
 *************************************************************************/


/**
 * gnome_print_font_picker_new
 *
 * Description:
 * Create new font picker widget.
 *
 * Returns:
 * Pointer to new font picker widget.
 */

GtkWidget *
gnome_print_font_picker_new (void)
{
	return g_object_new (GNOME_PRINT_TYPE_FONT_PICKER, NULL);
} /* gnome_print_font_picker_new */



/**
 * gnome_print_font_picker_set_title
 * @gfp: Pointer to GNOME font picker widget.
 * @title: String containing font selection dialog title.
 *
 * Description:
 * Sets the title for the font selection dialog.  If @title is %NULL,
 * then the default is used.
 */

void
gnome_print_font_picker_set_title (GnomePrintFontPicker *gfp, const gchar *title)
{
	g_return_if_fail (gfp != NULL);
	g_return_if_fail (GNOME_PRINT_IS_FONT_PICKER (gfp));

	if ( ! title)
		title = _(DEF_TITLE);

	/* g_free handles NULL */
	g_free (gfp->_priv->title);
        gfp->_priv->title = g_strdup (title);

        /* If FontDialog is created change title */
        if (gfp->_priv->font_dialog)
            gtk_window_set_title(GTK_WINDOW(gfp->_priv->font_dialog),
				 gfp->_priv->title);
} /* gnome_print_font_picker_set_title */

/**
 * gnome_print_font_picker_get_title
 * @gfp: Pointer to GNOME font picker widget.
 *
 * Description:
 * Retrieve name of the font selection dialog title
 *
 * Returns:
 * Pointer to an internal copy of the title string
 */

const gchar*
gnome_print_font_picker_get_title(GnomePrintFontPicker *gfp)
{
    g_return_val_if_fail (gfp != NULL, NULL);
    g_return_val_if_fail (GNOME_PRINT_IS_FONT_PICKER (gfp), NULL);

    return gfp->_priv->title;
} /* gnome_print_font_picker_get_title */

/**
 * gnome_print_font_picker_get_mode
 * @gfp: Pointer to GNOME font picker widget.
 *
 * Description:
 * Returns current font picker button mode (or what to show).  Possible
 * values include %GNOME_PRINT_FONT_PICKER_MODE_PIXMAP,
 * %GNOME_PRINT_FONT_PICKER_MODE_FONT_INFO, and
 * %GNOME_PRINT_FONT_PICKER_MODE_USER_WIDGET.
 *
 * Returns:
 * Button mode currently set in font picker widget, or
 * %GNOME_PRINT_FONT_PICKER_MODE_UNKNOWN on error.
 */

GnomePrintFontPickerMode
gnome_print_font_picker_get_mode        (GnomePrintFontPicker *gfp)
{
    g_return_val_if_fail (gfp != NULL, GNOME_PRINT_FONT_PICKER_MODE_UNKNOWN);
    g_return_val_if_fail (GNOME_PRINT_IS_FONT_PICKER (gfp),
			  GNOME_PRINT_FONT_PICKER_MODE_UNKNOWN);

    return gfp->_priv->mode;
} /* gnome_print_font_picker_get_mode */


/**
 * gnome_print_font_picker_set_mode
 * @gfp: Pointer to GNOME font picker widget.
 * @mode: Value of subsequent font picker button mode (or what to show)
 *
 * Description:
 * Set value of subsequent font picker button mode (or what to show).
 */

void       gnome_print_font_picker_set_mode        (GnomePrintFontPicker *gfp,
                                              GnomePrintFontPickerMode mode)
{
    g_return_if_fail (gfp != NULL);
    g_return_if_fail (GNOME_PRINT_IS_FONT_PICKER (gfp));
    g_return_if_fail (mode >= 0 && mode < GNOME_PRINT_FONT_PICKER_MODE_UNKNOWN);

    if (gfp->_priv->mode != mode) {

        gfp->_priv->mode = mode;

        /* Next sentence will destroy gfp->_priv->inside after removing it */
        gtk_container_remove(GTK_CONTAINER(gfp), gfp->_priv->inside);
        gfp->_priv->inside = gnome_print_font_picker_create_inside(gfp);
        if (gfp->_priv->inside)
            gtk_container_add(GTK_CONTAINER(gfp), gfp->_priv->inside);

        if (gfp->_priv->mode == GNOME_PRINT_FONT_PICKER_MODE_FONT_INFO) {
            /* Update */
            gnome_print_font_picker_update_font_info(gfp);
        }
    }
} /* gnome_print_font_picker_set_mode */


/**
 * gnome_print_font_picker_fi_set_use_font_in_label
 * @gfp: Pointer to GNOME font picker widget.
 * @use_font_in_label: If %TRUE, font name will be written using font chosen.
 * @size: Display font using this point size.
 *
 * Description:
 * If @use_font_in_label is %TRUE, font name will be written using font chosen
 * by user and using @size passed to this function.  This only applies if
 * current button mode is %GNOME_PRINT_FONT_PICKER_MODE_FONT_INFO.
 */

void  gnome_print_font_picker_fi_set_use_font_in_label (GnomePrintFontPicker *gfp,
                                                  gboolean use_font_in_label,
                                                  gint size)
{
	gboolean old_use_font_in_label;
	int old_size;

	g_return_if_fail (gfp != NULL);
	g_return_if_fail (GNOME_PRINT_IS_FONT_PICKER (gfp));

	old_use_font_in_label = gfp->_priv->use_font_in_label;
	old_size = gfp->_priv->use_font_in_label_size;
	gfp->_priv->use_font_in_label = use_font_in_label;
	gfp->_priv->use_font_in_label_size = size;

	if (gfp->_priv->mode == GNOME_PRINT_FONT_PICKER_MODE_FONT_INFO &&
	    (old_use_font_in_label != use_font_in_label ||
	     old_size != size)) {
		if (gfp->_priv->use_font_in_label)
			gnome_print_font_picker_label_use_font_in_label (gfp);
		else
			gtk_widget_set_style (gfp->_priv->font_label, NULL);
	}
} /* gnome_print_font_picker_fi_set_use_font_in_label */


/**
 * gnome_print_font_picker_fi_set_show_size
 * @gfp: Pointer to GNOME font picker widget.
 * @show_size: %TRUE if font size should be displayed in dialog.
 *
 * Description:
 * If @show_size is %TRUE, font size will be displayed along with font chosen
 * by user.  This only applies if current button mode is
 * %GNOME_PRINT_FONT_PICKER_MODE_FONT_INFO.
 */

void
gnome_print_font_picker_fi_set_show_size (GnomePrintFontPicker *gfp,
				    gboolean show_size)
{
	gboolean old_show_size;

	g_return_if_fail (gfp != NULL);
	g_return_if_fail (GNOME_PRINT_IS_FONT_PICKER (gfp));

	/* Make sure it's normalized to 1/0 */
	old_show_size = gfp->_priv->show_size ? 1 : 0;
	gfp->_priv->show_size = show_size ? 1 : 0;

	if (gfp->_priv->mode == GNOME_PRINT_FONT_PICKER_MODE_FONT_INFO &&
	    old_show_size != gfp->_priv->show_size) {
		/* Next sentence will destroy gfp->_priv->inside
		 * after removing it */
		if (gfp->_priv->inside != NULL)
			gtk_container_remove (GTK_CONTAINER (gfp),
					      gfp->_priv->inside);

		gfp->_priv->inside = gnome_print_font_picker_create_inside (gfp);
		if (gfp->_priv->inside != NULL)
			gtk_container_add (GTK_CONTAINER (gfp),
					   gfp->_priv->inside);

		gnome_print_font_picker_update_font_info (gfp);
	}
} /* gnome_print_font_picker_fi_set_show_size */


/**
 * gnome_print_font_picker_uw_set_widget
 * @gfp: Pointer to GNOME font picker widget.
 * @widget: User widget to display for inside of font picker.
 *
 * Set the user-supplied @widget as the inside of the font picker.
 * This only applies with GNOME_PRINT_FONT_PICKER_MODE_USER_WIDGET.
 */

void
gnome_print_font_picker_uw_set_widget (GnomePrintFontPicker *gfp,
				 GtkWidget *widget)
{
    g_return_if_fail (gfp != NULL);
    g_return_if_fail (GNOME_PRINT_IS_FONT_PICKER (gfp));

    if (gfp->_priv->mode == GNOME_PRINT_FONT_PICKER_MODE_USER_WIDGET &&
	gfp->_priv->inside != widget) {
	    if (gfp->_priv->inside != NULL)
		    gtk_container_remove (GTK_CONTAINER (gfp),
					  gfp->_priv->inside);

	    gfp->_priv->inside = widget;
	    if (gfp->_priv->inside != NULL)
		    gtk_container_add (GTK_CONTAINER (gfp),
				       gfp->_priv->inside);
    }

} /* gnome_print_font_picker_uw_set_widget */

/**
 * gnome_print_font_picker_uw_get_widget
 * @gfp: Pointer to GNOME font picker widget.
 *
 * Get the user-supplied widget inside of the font picker.
 * This only applies with GNOME_PRINT_FONT_PICKER_MODE_USER_WIDGET.
 *
 * Returns: a #GtkWidget, or %NULL if
 * not in GNOME_PRINT_FONT_PICKER_MODE_USER_WIDGET mode
 */

GtkWidget *
gnome_print_font_picker_uw_get_widget (GnomePrintFontPicker *gfp)
{
    g_return_val_if_fail (gfp != NULL, NULL);
    g_return_val_if_fail (GNOME_PRINT_IS_FONT_PICKER (gfp), NULL);

    if (gfp->_priv->mode == GNOME_PRINT_FONT_PICKER_MODE_USER_WIDGET) {
	    return gfp->_priv->inside;
    } else {
	    return NULL;
    }
} /* gnome_print_font_picker_uw_set_widget */

/**
 * gnome_print_font_picker_get_font_name
 * @gfp: Pointer to GNOME font picker widget.
 *
 * Description:
 * Retrieve name of font from font selection dialog.
 *
 * Returns:
 * Pointer to an internal copy of the font name.
 */

const gchar*	   gnome_print_font_picker_get_font_name    (GnomePrintFontPicker *gfp)
{
    g_return_val_if_fail (gfp != NULL, NULL);
    g_return_val_if_fail (GNOME_PRINT_IS_FONT_PICKER (gfp), NULL);

    if (gfp->_priv->font_dialog) {
	/* g_free handles NULL */
        GnomeFont *font;
	GtkWidget *font_sel;
	
	g_free (gfp->_priv->font_name);

	if (gfp->_priv->font)
		g_object_unref (gfp->_priv->font);
	
	font_sel = gnome_print_font_dialog_get_fontsel (GNOME_PRINT_FONT_DIALOG (gfp->_priv->font_dialog));
	font = gnome_font_selection_get_font (GNOME_FONT_SELECTION (font_sel));
	
	gfp->_priv->font_name = g_strdup (gnome_font_get_full_name (font));
	gfp->_priv->font = font;
    }

    return gfp->_priv->font_name;
} /* gnome_print_font_picker_get_font_name */


/**
 * gnome_print_font_picker_set_font_name
 * @gfp: Pointer to GNOME font picker widget.
 * @fontname: Name of font to display in font selection dialog
 *
 * Description:
 * Set or update currently-displayed font in font picker dialog.
 *
 * Returns:
 * Return value of gtk_font_selection_dialog_set_font_name if the
 * font selection dialog exists, otherwise %FALSE.
 */

gboolean   gnome_print_font_picker_set_font_name    (GnomePrintFontPicker *gfp,
					       const gchar       *fontname)
{
    g_return_val_if_fail (gfp != NULL, FALSE);
    g_return_val_if_fail (GNOME_PRINT_IS_FONT_PICKER (gfp), FALSE);
    g_return_val_if_fail (fontname != NULL, FALSE);

    if (gfp->_priv->font_name != fontname) {
	    g_free(gfp->_priv->font_name);
	    gfp->_priv->font_name = g_strdup (fontname);

	    if (gfp->_priv->font != NULL)
		    g_object_unref (gfp->_priv->font);

	    
	    gfp->_priv->font = gnome_font_find_closest_from_full_name (fontname);	
    }

    if (gfp->_priv->mode == GNOME_PRINT_FONT_PICKER_MODE_FONT_INFO)
	gnome_print_font_picker_update_font_info (gfp);

    if (gfp->_priv->font_dialog)
    {
	GtkWidget *font_sel;
	
	font_sel = gnome_print_font_dialog_get_fontsel (GNOME_PRINT_FONT_DIALOG (gfp->_priv->font_dialog));
	gnome_font_selection_set_font (GNOME_FONT_SELECTION (font_sel), gfp->_priv->font);

        return TRUE;
    }
    else
        return FALSE;
} /* gnome_print_font_picker_set_font_name */


/**
 * gnome_print_font_picker_get_preview_text
 * @gfp: Pointer to GNOME font picker widget.
 *
 * Description:
 * Retrieve preview text from font selection dialog if available.
 *
 * Returns:
 * Reference to internal copy of preview text string, or %NULL if no
 * font dialog is being displayed.
 */

const gchar*	   gnome_print_font_picker_get_preview_text (GnomePrintFontPicker *gfp)
{
    g_return_val_if_fail (gfp != NULL, NULL);
    g_return_val_if_fail (GNOME_PRINT_IS_FONT_PICKER (gfp), NULL);

    if (gfp->_priv->font_dialog) {
        /* g_free handles NULL */
        g_free (gfp->_priv->preview_text);
	/* FIXME */
        gfp->_priv->preview_text = g_strdup (DEF_PREVIEW_TEXT);
    }

    return gfp->_priv->preview_text;

} /* gnome_print_font_picker_get_preview_text */


/**
 * gnome_print_font_picker_set_preview_text
 * @gfp: Pointer to GNOME font picker widget.
 * @text: New preview text
 *
 * Description:
 * Set preview text in font picker, and in font selection dialog if one
 * is being displayed.
 */

void	   gnome_print_font_picker_set_preview_text (GnomePrintFontPicker *gfp,
                                               const gchar     *text)
{
    g_return_if_fail (gfp != NULL);
    g_return_if_fail (GNOME_PRINT_IS_FONT_PICKER (gfp));
    g_return_if_fail (text != NULL);

    if (gfp->_priv->preview_text != text) {
	    g_free(gfp->_priv->preview_text);
	    gfp->_priv->preview_text = g_strdup (text);
    }

    if (gfp->_priv->font_dialog)
    {
	GtkWidget *preview;
	
	preview = gnome_print_font_dialog_get_preview (GNOME_PRINT_FONT_DIALOG (gfp->_priv->font_dialog));

	gnome_font_preview_set_phrase (GNOME_FONT_PREVIEW (preview), gfp->_priv->preview_text);;
    }

} /* gnome_print_font_picker_set_preview_text */

/* ************************************************************************
 Internal functions
 **************************************************************************/
static void
gnome_print_font_dialog_response_handler (GtkWidget *dialog, 
		gint response_id, gpointer data)
{
	switch (response_id) {
		case GTK_RESPONSE_OK:
			gnome_print_font_picker_dialog_ok_clicked (NULL, data);
		case GTK_RESPONSE_CANCEL:
			gnome_print_font_picker_dialog_cancel_clicked (NULL, data);
		default:
			gnome_print_font_picker_dialog_delete_event (NULL, NULL, data);
	}
	
}

static void
gnome_print_font_picker_clicked (GtkButton *button)
{
    GnomePrintFontPicker      *gfp;
    GnomePrintFontDialog *fsd;

    gfp = GNOME_PRINT_FONT_PICKER(button);

    if (!gfp->_priv->font_dialog) {
        GtkWidget *parent;

        parent = gtk_widget_get_toplevel (GTK_WIDGET (gfp));

        gfp->_priv->font_dialog = gnome_print_font_dialog_new (gfp->_priv->title);

        if (parent)
            gtk_window_set_transient_for(GTK_WINDOW(gfp->_priv->font_dialog),
                                         GTK_WINDOW(parent));

        fsd = GNOME_PRINT_FONT_DIALOG (gfp->_priv->font_dialog);

        /* If there is a grabed window, set new dialog as modal */
        if (gtk_grab_get_current())
            gtk_window_set_modal(GTK_WINDOW(gfp->_priv->font_dialog), TRUE);

	g_signal_connect(fsd, "response",
			 G_CALLBACK (gnome_print_font_dialog_response_handler),
			 gfp);
/*
        g_signal_connect(fsd->ok_button, "clicked",
			 G_CALLBACK (gnome_print_font_picker_dialog_ok_clicked),
			 gfp);

        g_signal_connect(fsd->cancel_button, "clicked",
			 G_CALLBACK (gnome_print_font_picker_dialog_cancel_clicked),
			 gfp);
        g_signal_connect(fsd,"delete_event",
			 G_CALLBACK (gnome_print_font_picker_dialog_delete_event),
			 gfp);
*/
        g_signal_connect(fsd, "destroy",
			 G_CALLBACK (gnome_print_font_picker_dialog_destroy),
			 gfp);
    } /* if */

    if (!GTK_WIDGET_VISIBLE(gfp->_priv->font_dialog)) {

        /* Set font and preview text */
	GtkWidget *font_sel;
	GtkWidget *preview;

	font_sel = gnome_print_font_dialog_get_fontsel (GNOME_PRINT_FONT_DIALOG (gfp->_priv->font_dialog));
	gnome_font_selection_set_font (GNOME_FONT_SELECTION (font_sel), gfp->_priv->font);

	preview = gnome_print_font_dialog_get_preview (GNOME_PRINT_FONT_DIALOG (gfp->_priv->font_dialog));

	gnome_font_preview_set_phrase (GNOME_FONT_PREVIEW (preview), gfp->_priv->preview_text);;
        
	gtk_widget_show(gfp->_priv->font_dialog);
    } else if (gfp->_priv->font_dialog->window) {
	/*raise the window so that if it is obscured that we see it*/
	gdk_window_raise(gfp->_priv->font_dialog->window);
    }/* if */
} /* gnome_print_font_picker_clicked */

static void
gnome_print_font_picker_dialog_ok_clicked(GtkWidget *widget,
				    gpointer   data)
{
    GnomePrintFontPicker *gfp;

    gfp = GNOME_PRINT_FONT_PICKER (data);

    gtk_widget_hide(gfp->_priv->font_dialog);

    /* These calls will update gfp->_priv->font_name and gfp->_priv->preview_text */
    gnome_print_font_picker_get_font_name(gfp);
    gnome_print_font_picker_get_preview_text(gfp);

    /* Set label font */
    if (gfp->_priv->mode == GNOME_PRINT_FONT_PICKER_MODE_FONT_INFO)
        gnome_print_font_picker_update_font_info(gfp);

    /* Emit font_set signal */
    g_signal_emit(gfp, font_picker_signals[FONT_SET], 0,
		  gfp->_priv->font_name);

} /* gnome_print_font_picker_dialog_ok_clicked */


static void
gnome_print_font_picker_dialog_cancel_clicked(GtkWidget *widget,
					gpointer   data)
{
    GnomePrintFontPicker *gfp;

    gfp = GNOME_PRINT_FONT_PICKER (data);

    gtk_widget_hide(gfp->_priv->font_dialog);

    /* FIXME: is this necessary? bug #67587 */
    /* Restore old values */
    gnome_print_font_picker_set_font_name(gfp,gfp->_priv->font_name);
    gnome_print_font_picker_set_preview_text(gfp,gfp->_priv->preview_text);

} /* gnome_print_font_picker_dialog_cancel_clicked */

static gboolean
gnome_print_font_picker_dialog_delete_event(GtkWidget *widget, GdkEventAny *ev,
				      gpointer   data)
{
    GnomePrintFontPicker *gfp;

    gfp = GNOME_PRINT_FONT_PICKER (data);

    /* FIXME: is this necessary? bug #67587 */
    /* Here we restore old values */
    gnome_print_font_picker_set_font_name(gfp,gfp->_priv->font_name);
    gnome_print_font_picker_set_preview_text(gfp,gfp->_priv->preview_text);

    return FALSE;
} /* gnome_print_font_picker_dialog_delete_event */

static void
gnome_print_font_picker_dialog_destroy(GtkWidget *widget,
				 gpointer   data)
{
    GnomePrintFontPicker *gfp;

    gfp = GNOME_PRINT_FONT_PICKER (data);

    /* Query GtkFontSelectionDialog before it get destroyed */
    /* These calls will update gfp->_priv->font_name and gfp->_priv->preview_text */
    gnome_print_font_picker_get_font_name(gfp);
    gnome_print_font_picker_get_preview_text(gfp);

    /* Dialog will get destroyed so reference is no valid now */
    gfp->_priv->font_dialog = NULL;
} /* gnome_print_font_picker_dialog_destroy */

GtkWidget *gnome_print_font_picker_create_inside(GnomePrintFontPicker *gfp)
{
    GtkWidget *widget;

    if (gfp->_priv->mode==GNOME_PRINT_FONT_PICKER_MODE_PIXMAP) {
        widget=gtk_image_new_from_stock(GTK_STOCK_SELECT_FONT,
					GTK_ICON_SIZE_BUTTON);
        gtk_widget_show(widget);
    } else if (gfp->_priv->mode==GNOME_PRINT_FONT_PICKER_MODE_FONT_INFO) {
        widget=gtk_hbox_new(FALSE,0);

        gfp->_priv->font_label=gtk_label_new(_("Font"));

        gtk_label_set_justify(GTK_LABEL(gfp->_priv->font_label),GTK_JUSTIFY_LEFT);
        gtk_box_pack_start(GTK_BOX(widget),gfp->_priv->font_label,TRUE,TRUE,5);

        if (gfp->_priv->show_size) {
            gfp->_priv->vsep=gtk_vseparator_new();
            gtk_box_pack_start(GTK_BOX(widget),gfp->_priv->vsep,FALSE,FALSE,0);

            gfp->_priv->size_label=gtk_label_new("14");
            gtk_box_pack_start(GTK_BOX(widget),gfp->_priv->size_label,FALSE,FALSE,5);
        }

        gtk_widget_show_all(widget);

    } else /* GNOME_PRINT_FONT_PICKER_MODE_USER_WIDGET */ {
        widget=NULL;
    }

    return widget;

} /* gnome_print_font_picker_create_inside */

static void
gnome_print_font_picker_label_use_font_in_label  (GnomePrintFontPicker *gfp)
{
	PangoFontDescription *desc;
	GtkStyle *style;

	desc = gnome_font_get_pango_description (gfp->_priv->font, 1 /*FIXME*/);
	if (desc == NULL) {
		return; /* Use widget default */
	}

	/* Change size */
	pango_font_description_set_size (desc,
					 gfp->_priv->use_font_in_label_size *
					 PANGO_SCALE);

	/* FIXME: listen for style changes */
	/* Change label style */
	gtk_widget_ensure_style (gfp->_priv->font_label);
	style = gtk_style_copy (gfp->_priv->font_label->style);
	if (style->font_desc != NULL)
		pango_font_description_free (style->font_desc);

	style->font_desc = desc;

	gtk_widget_set_style (gfp->_priv->font_label, style);
	g_object_unref (G_OBJECT (style));
} /* gnome_print_font_picker_set_label_font */

static void
gnome_print_font_picker_update_font_info (GnomePrintFontPicker *gfp)
{
	const char *name;

	name = gnome_font_get_family_name (gfp->_priv->font);

	gtk_label_set_text (GTK_LABEL (gfp->_priv->font_label), name);

	/* Extract font size */
	if (gfp->_priv->show_size) {
		gdouble size = gnome_font_get_size (gfp->_priv->font);
		gchar *size_str = g_strdup_printf ("%.1f", size);

		gtk_label_set_text (GTK_LABEL (gfp->_priv->size_label), size_str);

		g_free (size_str);
	}

	if (gfp->_priv->use_font_in_label) {
		gnome_print_font_picker_label_use_font_in_label (gfp);
	}
} /* gnome_print_font_picker_update_font_info */
