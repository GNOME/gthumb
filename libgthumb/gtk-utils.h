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

#ifndef _GTK_UTILS_H
#define _GTK_UTILS_H

#include <gtk/gtkwidget.h>

GtkWidget * _gtk_image_new_from_xpm_data (char             *xpm_data[]);
GtkWidget * _gtk_image_new_from_inline   (const guint8     *data);

GtkWidget*  _gtk_message_dialog_new      (GtkWindow        *parent,
					  GtkDialogFlags    flags,
					  const char       *stock_id,
					  const char       *message,
					  const char       *secondary_message,
					  const char       *first_button_text,
					  ...);

gchar*      _gtk_request_dialog_run      (GtkWindow        *parent,
					  GtkDialogFlags    flags,
					  const char       *message,
					  const char       *default_value,
					  int               max_length,
					  const char       *no_button_text,
					  const char       *yes_button_text);

GtkWidget*  _gtk_yesno_dialog_new        (GtkWindow        *parent,
					  GtkDialogFlags    flags,
					  const char       *message,
					  const char       *no_button_text,
					  const char       *yes_button_text);

void        _gtk_error_dialog_from_gerror_run  (GtkWindow        *parent,
						GError          **gerror);

void        _gtk_error_dialog_run        (GtkWindow        *parent,
					  const gchar      *format,
					  ...) G_GNUC_PRINTF (2, 3);

void        _gtk_info_dialog_run         (GtkWindow        *parent,
					  const gchar      *format,
					  ...) G_GNUC_PRINTF (2, 3);

void        _gtk_entry_set_locale_text   (GtkEntry   *entry,
					  const char *text);

char *      _gtk_entry_get_locale_text   (GtkEntry   *entry);

void        _gtk_label_set_locale_text   (GtkLabel   *label,
					  const char *text);

char *      _gtk_label_get_locale_text   (GtkLabel   *label);


#endif /* _GTK_UTILS_H */
