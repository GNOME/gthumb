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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GTK_UTILS_H
#define GTK_UTILS_H

#include <glib/gi18n.h>
#include <gio/gio.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define _GTK_ICON_NAME_DIALOG_ERROR "dialog-error-symbolic"
#define _GTK_ICON_NAME_DIALOG_INFO "dialog-information-symbolic"
#define _GTK_ICON_NAME_DIALOG_QUESTION "dialog-question-symbolic"
#define _GTK_ICON_NAME_DIALOG_WARNING "dialog-warning-symbolic"

#define _GTK_LABEL_CANCEL _("_Cancel")
#define _GTK_LABEL_CLOSE _("_Close")
#define _GTK_LABEL_COPY _("Copy")
#define _GTK_LABEL_DELETE _("_Delete")
#define _GTK_LABEL_HELP _("_Help")
#define _GTK_LABEL_NEW _("_New")
#define _GTK_LABEL_OK _("_Ok")
#define _GTK_LABEL_REMOVE _("_Remove")
#define _GTK_LABEL_SAVE _("_Save")


typedef struct {
	const char *action_name;
	const char *accelerator;
} GthAccelerator;

GtkWidget *     _gtk_message_dialog_new                    (GtkWindow        *parent,
							    GtkDialogFlags    flags,
							    const char       *icon_name,
							    const char       *message,
							    const char       *secondary_message,
							    const char       *first_button_text,
							    ...);
GtkWidget *     _gtk_yesno_dialog_new                      (GtkWindow        *parent,
							    GtkDialogFlags    flags,
							    const char       *message,
							    const char       *no_button_text,
							    const char       *yes_button_text);
void            _gtk_error_dialog_from_gerror_run          (GtkWindow        *parent,
					    	    	    const char       *title,
					    	    	    GError           *gerror);
void            _gtk_error_dialog_from_gerror_show         (GtkWindow        *parent,
					    	    	    const char       *title,
					    	    	    GError           *gerror);
void            _gtk_error_dialog_run                      (GtkWindow        *parent,
					    	    	    const gchar      *format,
					    	    	    ...) G_GNUC_PRINTF (2, 3);
void            _gtk_error_dialog_show                     (GtkWindow        *parent,
					    	    	    const char       *title,
					    	    	    const char       *format,
					    	    	    ...) G_GNUC_PRINTF (3, 4);
void            _gtk_info_dialog_run                       (GtkWindow        *parent,
					    	    	    const gchar      *format,
					    	    	    ...) G_GNUC_PRINTF (2, 3);
void            _gtk_dialog_add_to_window_group            (GtkDialog        *dialog);
GdkPixbuf *     _g_icon_get_pixbuf                         (GIcon            *icon,
		 			    	    	    int               icon_size,
		 			    	    	    GtkIconTheme     *icon_theme);
GdkPixbuf *     get_mime_type_pixbuf                       (const char       *mime_type,
					    	    	    int               icon_size,
					    	    	    GtkIconTheme     *icon_theme);
void            show_help_dialog                           (GtkWindow        *parent,
					    	    	    const char       *section);
void            _gtk_container_remove_children             (GtkContainer     *container,
					    	    	    gpointer          start_after_this,
					    	    	    gpointer          stop_before_this);
int             _gtk_container_get_pos                     (GtkContainer     *container,
					    	    	    GtkWidget        *child);
guint           _gtk_container_get_n_children              (GtkContainer     *container);
GtkBuilder *    _gtk_builder_new_from_file                 (const char       *filename,
					    	    	    const char       *extension);
GtkBuilder *    _gtk_builder_new_from_resource             (const char       *resource_path);
GtkWidget *     _gtk_builder_get_widget                    (GtkBuilder       *builder,
	 		 		    	    	    const char       *name);
GtkWidget *     _gtk_combo_box_new_with_texts              (const char       *first_text,
					    	    	    ...);
void            _gtk_combo_box_append_texts                (GtkComboBoxText  *combo_box,
					    	    	    const char       *first_text,
					    	    	    ...);
GtkWidget *     _gtk_image_new_from_xpm_data               (char             *xpm_data[]);
GtkWidget *     _gtk_image_new_from_inline                 (const guint8     *data);
void            _gtk_widget_get_screen_size                (GtkWidget        *widget,
					    	    	    int              *width,
					    	    	    int              *height);
int             _gtk_widget_get_allocated_width            (GtkWidget        *widget);
int             _gtk_widget_get_allocated_height           (GtkWidget        *widget);
int             _gtk_widget_lookup_for_size                (GtkWidget        *widget,
							    GtkIconSize       icon_size);
void            _gtk_tree_path_list_free                   (GList            *list);
int             _gtk_paned_get_position2                   (GtkPaned         *paned);
void            _g_launch_command                          (GtkWidget        *parent,
					    	    	    const char       *command,
					    	    	    const char       *name,
					    	    	    GList            *files);
void            _gtk_window_resize_to_fit_screen_height    (GtkWidget        *window,
					    	    	    int               default_width);
void            _gtk_info_bar_clear_action_area            (GtkInfoBar       *info_bar);
GdkDragAction   _gtk_menu_ask_drag_drop_action             (GtkWidget        *widget,
					      	      	    GdkDragAction     actions,
					      	      	    guint32           activate_time);
gboolean        _gdk_rgba_darker                           (GdkRGBA          *color,
		 					    GdkRGBA          *result);
gboolean        _gdk_rgba_lighter                          (GdkRGBA          *color,
							    GdkRGBA          *result);
GtkIconTheme *  _gtk_widget_get_icon_theme                 (GtkWidget        *widget);
void            _gtk_combo_box_add_image_sizes             (GtkComboBox      *combo_box,
							    int               active_width,
							    int               active_height);
gboolean        _gtk_file_chooser_set_file_parent          (GtkFileChooser   *chooser,
							    GFile            *file,
							    GError          **error);
GtkWidget *     _gtk_menu_button_new_for_header_bar        (void);
GtkWidget *     _gtk_image_button_new_for_header_bar       (const char       *icon_name);
GtkWidget *     _gtk_toggle_image_button_new_for_header_bar(const char       *icon_name);
void		_gtk_window_add_accelerator_for_action     (GtkWindow		*window,
							    GtkAccelGroup	*accel_group,
							    const char		*action_name,
							    const char		*accel,
							    GVariant		*target);
void		_gtk_window_add_accelerators_from_menu	   (GtkWindow		*window,
							    GMenuModel		*menu);
gboolean	_gtk_window_get_is_maximized 		   (GtkWindow		*window);
GdkDevice *     _gtk_widget_get_client_pointer		   (GtkWidget		*widget);
void            _gtk_list_box_add_separator		   (GtkListBox		*list_box);

G_END_DECLS

#endif
