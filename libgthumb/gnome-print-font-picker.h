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

#ifndef GNOME_PRINT_FONT_PICKER_H
#define GNOME_PRINT_FONT_PICKER_H

#include <gtk/gtkbutton.h>
#include <libgnomeprint/gnome-font.h>


G_BEGIN_DECLS

/* GnomePrintFontPicker is a button widget that allow user to select a 
 * gnome print font.
 */

/* Button Mode or What to show */
typedef enum {
    GNOME_PRINT_FONT_PICKER_MODE_PIXMAP,
    GNOME_PRINT_FONT_PICKER_MODE_FONT_INFO,
    GNOME_PRINT_FONT_PICKER_MODE_USER_WIDGET,
    GNOME_PRINT_FONT_PICKER_MODE_UNKNOWN
} GnomePrintFontPickerMode;
        

#define GNOME_PRINT_TYPE_FONT_PICKER            (gnome_print_font_picker_get_type ())
#define GNOME_PRINT_FONT_PICKER(obj)            (GTK_CHECK_CAST ((obj), GNOME_PRINT_TYPE_FONT_PICKER, GnomePrintFontPicker))
#define GNOME_PRINT_FONT_PICKER_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GNOME_PRINT_TYPE_FONT_PICKER, GnomePrintFontPickerClass))
#define GNOME_PRINT_IS_FONT_PICKER(obj)         (GTK_CHECK_TYPE ((obj), GNOME_PRINT_TYPE_FONT_PICKER))
#define GNOME_PRINT_IS_FONT_PICKER_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_PRINT_TYPE_FONT_PICKER))
#define GNOME_PRINT_FONT_PICKER_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), GNOME_PRINT_TYPE_FONT_PICKER, GnomePrintFontPickerClass))

typedef struct _GnomePrintFontPicker        GnomePrintFontPicker;
typedef struct _GnomePrintFontPickerPrivate GnomePrintFontPickerPrivate;
typedef struct _GnomePrintFontPickerClass   GnomePrintFontPickerClass;

struct _GnomePrintFontPicker {
        GtkButton button;
    
	/*< private >*/
	GnomePrintFontPickerPrivate *_priv;
};

struct _GnomePrintFontPickerClass {
	GtkButtonClass parent_class;

	/* font_set signal is emitted when font is choosed */
	void (* font_set) (GnomePrintFontPicker *gfp, const gchar *font_name);

	/* It is possible we may need more signals */
	gpointer padding1;
	gpointer padding2;
};


/* Standard Gtk function */
GType 		 gnome_print_font_picker_get_type 	(void) G_GNUC_CONST;

/* Creates a new font picker widget */
GtkWidget 	*gnome_print_font_picker_new (void);

/* Sets the title for the font selection dialog */
void       	 gnome_print_font_picker_set_title	(GnomePrintFontPicker *gfp,
					      		 const gchar *title);
const gchar 	*gnome_print_font_picker_get_title 	(GnomePrintFontPicker *gfp);

/* Button mode */
GnomePrintFontPickerMode
		 gnome_print_font_picker_get_mode	(GnomePrintFontPicker *gfp);

void		 gnome_print_font_picker_set_mode	(GnomePrintFontPicker *gfp,
                                                	 GnomePrintFontPickerMode mode);
/* With  GNOME_PRINT_FONT_PICKER_MODE_FONT_INFO */
/* If use_font_in_label is true, font name will be writen using font choosed by user and
 using size passed to this function*/
void       	 gnome_print_font_picker_fi_set_use_font_in_label (GnomePrintFontPicker *gfp,
							 gboolean use_font_in_label,
							 gint size);

void       	 gnome_print_font_picker_fi_set_show_size (GnomePrintFontPicker *gfp,
							  gboolean show_size);

/* With GNOME_PRINT_FONT_PICKER_MODE_USER_WIDGET */
void       	 gnome_print_font_picker_uw_set_widget	(GnomePrintFontPicker *gfp,
							 GtkWidget *widget);
GtkWidget 	*gnome_print_font_picker_uw_get_widget	(GnomePrintFontPicker *gfp);

/* Functions to interface with GtkFontSelectionDialog */
const gchar 	*gnome_print_font_picker_get_font_name	(GnomePrintFontPicker *gfp);

gboolean   	 gnome_print_font_picker_set_font_name	(GnomePrintFontPicker *gfp,
                                               	     	 const gchar *fontname);
#include <gtk/gtkbutton.h>

const gchar	*gnome_print_font_picker_get_preview_text (GnomePrintFontPicker *gfp);

void	   	 gnome_print_font_picker_set_preview_text (GnomePrintFontPicker *gfp,
                                                     	   const gchar *text);

G_END_DECLS
    
#endif /* GNOME_PRINT_FONT_PICKER_H */
