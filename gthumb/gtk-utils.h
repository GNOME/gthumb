/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  File-Roller
 *
 *  Copyright (C) 2001-2008 Free Software Foundation, Inc.
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

#ifndef GTK_UTILS_H
#define GTK_UTILS_H

#include <glib/gi18n.h>
#include <gio/gio.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

GtkWidget*  _gtk_message_dialog_new      (GtkWindow        *parent,
					  GtkDialogFlags    flags,
					  const char       *stock_id,
					  const char       *message,
					  const char       *secondary_message,
					  const char       *first_button_text,
					  ...);
GtkWidget*
_gtk_message_dialog_with_checkbutton_new (GtkWindow        *parent,
			 		  GtkDialogFlags    flags,
			 		  const char       *stock_id,
			 		  const char       *message,
			 		  const char       *secondary_message,
			 		  const char       *gconf_key,
			 		  const char       *check_button_label,
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
GtkWidget*
_gtk_yesno_dialog_with_checkbutton_new   (GtkWindow        *parent,
					  GtkDialogFlags    flags,
					  const char       *message,
					  const char       *no_button_text,
					  const char       *yes_button_text,
					  const char       *check_button_label,
					  const char       *gconf_key);
void
_gtk_error_dialog_from_gerror_run        (GtkWindow        *parent,
					  const char       *title,
					  GError          **gerror);
void
_gtk_error_dialog_from_gerror_show       (GtkWindow        *parent,
					  const char       *title,
					  GError          **gerror);
void        _gtk_error_dialog_run        (GtkWindow        *parent,
					  const gchar      *format,
					  ...) G_GNUC_PRINTF (2, 3);
void        _gtk_info_dialog_run         (GtkWindow        *parent,
					  const gchar      *format,
					  ...) G_GNUC_PRINTF (2, 3);
GdkPixbuf * _g_icon_get_pixbuf           (GIcon            *icon,
		 			  int               size,
		 			  GtkIconTheme     *icon_theme);
GdkPixbuf * get_mime_type_pixbuf         (const char       *mime_type,
					  int               icon_size,
					  GtkIconTheme     *icon_theme);
int         _gtk_icon_get_pixel_size     (GtkWidget        *widget,
					  GtkIconSize       size);
void        show_help_dialog             (GtkWindow        *parent,
					  const char       *section);
void        _gtk_container_remove_children
					 (GtkContainer     *container,
					  gpointer          start_after_this,
			   		  gpointer          stop_before_this);
int         _gtk_container_get_pos       (GtkContainer     *container,
					  GtkWidget        *child);
guint       _gtk_container_get_n_children (GtkContainer     *container);
GtkBuilder *
	    _gtk_builder_new_from_file   (const char       *filename,
					  const char       *extension);
GtkWidget *
	    _gtk_builder_get_widget      (GtkBuilder       *builder,
			 		  const char       *name);
GtkWidget * _gtk_combo_box_new_with_texts (const char       *first_text,
					   ...);
void        _gtk_combo_box_append_texts  (GtkComboBox      *combo_box,
					  const char       *first_text,
					  ...);
GtkWidget * _gtk_image_new_from_xpm_data (char             *xpm_data[]);
GtkWidget * _gtk_image_new_from_inline   (const guint8     *data);
void        _gtk_widget_get_screen_size  (GtkWidget        *widget,
					  int              *width,
					  int              *height);
void        _gtk_tree_path_list_free     (GList            *list);

G_END_DECLS

#endif
