/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gnome-print-font-dialog.h
 * This file is part of gedit
 *
 * Copyright (C) 2001-2002 Paolo Maggi 
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
 * Modified by the gedit Team, 1998-2002. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 */

#ifndef __GNOME_PRINT_FONT_DIALOG_H__
#define __GNOME_PRINT_FONT_DIALOG_H__

#include <libgnomeprintui/gnome-font-dialog.h>

#define GNOME_PRINT_TYPE_FONT_DIALOG (gnome_print_font_dialog_get_type ())
#define GNOME_PRINT_FONT_DIALOG(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), GNOME_PRINT_TYPE_FONT_DIALOG, GnomePrintFontDialog))
#define GNOME_PRINT_FONT_DIALOG_CLASS(k) (G_TYPE_CHECK_CLASS_CAST ((k), GNOME_PRINT_TYPE_FONT_DIALOG, GnomePrintFontDialogClass))
#define GNOME_PRINT_IS_FONT_DIALOG(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GNOME_PRINT_TYPE_FONT_DIALOG))
#define GNOME_PRINT_IS_FONT_DIALOG_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), GNOME_PRINT_TYPE_FONT_DIALOG))
#define GNOME_PRINT_FONT_DIALOG_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GNOME_PRINT_TYPE_FONT_DIALOG, GnomePrintFontDialogClass))

typedef struct _GnomePrintFontDialog GnomePrintFontDialog;
typedef struct _GnomePrintFontDialogClass GnomePrintFontDialogClass;

GType gnome_print_font_dialog_get_type (void);

GtkWidget *gnome_print_font_dialog_new (const gchar *title);

GtkWidget *gnome_print_font_dialog_get_fontsel (GnomePrintFontDialog *gfsd);

GtkWidget *gnome_print_font_dialog_get_preview (GnomePrintFontDialog *gfsd);

#endif  /* __GNOME_PRINT_FONT_DIALOG_H__ */

