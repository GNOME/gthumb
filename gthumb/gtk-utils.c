/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  File-Roller
 *
 *  Copyright (C) 2001-2008 The Free Software Foundation, Inc.
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

#include <config.h>
#include <string.h>
#include "color-utils.h"
#include "gth-image-utils.h"
#include "gtk-utils.h"


#define REQUEST_ENTRY_WIDTH_IN_CHARS 40
#define GTHUMB_RESOURCE_BASE_PATH "/org/gnome/gThumb/resources/"


GtkWidget*
_gtk_message_dialog_new (GtkWindow        *parent,
			 GtkDialogFlags    flags,
			 const char       *icon_name,
			 const char       *message,
			 const char       *secondary_message,
			 const char       *first_button_text,
			 ...)
{
	GtkBuilder  *builder;
	GtkWidget   *dialog;
	GtkWidget   *label;
	va_list      args;
	const gchar *text;
	int          response_id;
	char        *markup_text;

	builder = _gtk_builder_new_from_resource ("message-dialog.ui");
	dialog = _gtk_builder_get_widget (builder, "message_dialog");
	gtk_window_set_title (GTK_WINDOW (dialog), "");
	gtk_window_set_transient_for (GTK_WINDOW (dialog), parent);
	gtk_window_set_modal (GTK_WINDOW (dialog), (flags & GTK_DIALOG_MODAL));
	gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), (flags & GTK_DIALOG_DESTROY_WITH_PARENT));
	g_object_weak_ref (G_OBJECT (dialog), (GWeakNotify) g_object_unref, builder);

	if (flags & GTK_DIALOG_MODAL)
		_gtk_dialog_add_to_window_group (GTK_DIALOG (dialog));

	/* set the icon */

	gtk_image_set_from_icon_name (GTK_IMAGE (_gtk_builder_get_widget (builder, "icon_image")),
				      icon_name,
				      GTK_ICON_SIZE_DIALOG);

	/* set the message */

	label = _gtk_builder_get_widget (builder, "message_label");

	if (message != NULL) {
		char *escaped_message;

		escaped_message = g_markup_escape_text (message, -1);
		if (secondary_message != NULL) {
			char *escaped_secondary_message;

			escaped_secondary_message = g_markup_escape_text (secondary_message, -1);
			markup_text = g_strdup_printf ("<span weight=\"bold\" size=\"larger\">%s</span>\n\n%s",
						       escaped_message,
						       escaped_secondary_message);

			g_free (escaped_secondary_message);
		}
		else
			markup_text = g_strdup_printf ("<span weight=\"bold\" size=\"larger\">%s</span>", escaped_message);

		g_free (escaped_message);
	}
	else
		markup_text = g_markup_escape_text (secondary_message, -1);

	gtk_label_set_markup (GTK_LABEL (label), markup_text);
	g_free (markup_text);

	/* add the buttons */

	if (first_button_text == NULL)
		return dialog;

	va_start (args, first_button_text);

	text = first_button_text;
	response_id = va_arg (args, gint);

	while (text != NULL) {
		gtk_dialog_add_button (GTK_DIALOG (dialog), text, response_id);

		text = va_arg (args, char*);
		if (text == NULL)
			break;
		response_id = va_arg (args, int);
	}

	va_end (args);

	return dialog;
}


GtkWidget*
_gtk_yesno_dialog_new (GtkWindow        *parent,
		       GtkDialogFlags    flags,
		       const char       *message,
		       const char       *no_button_text,
		       const char       *yes_button_text)
{
	GtkWidget    *d;
	GtkWidget    *label;
	GtkWidget    *image;
	GtkWidget    *hbox;
	GtkWidget    *button;

	d = gtk_dialog_new_with_buttons ("", parent, flags, NULL, NULL);
	gtk_window_set_resizable (GTK_WINDOW (d), FALSE);

	gtk_container_set_border_width (GTK_CONTAINER (d), 6);
	gtk_container_set_border_width (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (d))), 6);
	gtk_box_set_spacing (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (d))), 8);

	/* Add label and image */

	image = gtk_image_new_from_icon_name (_GTK_ICON_NAME_DIALOG_QUESTION, GTK_ICON_SIZE_DIALOG);
	gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0.0);

	label = gtk_label_new (message);
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	gtk_label_set_selectable (GTK_LABEL (label), TRUE);

	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 6);

	gtk_box_pack_start (GTK_BOX (hbox), image,
			    FALSE, FALSE, 0);

	gtk_box_pack_start (GTK_BOX (hbox), label,
			    TRUE, TRUE, 0);

	gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (d))),
			    hbox,
			    FALSE, FALSE, 0);

	gtk_widget_show_all (hbox);

	/* Add buttons */

	button = gtk_button_new_with_label (no_button_text);
	gtk_widget_show (button);
	gtk_dialog_add_action_widget (GTK_DIALOG (d),
				      button,
				      GTK_RESPONSE_CANCEL);

	/**/

	button = gtk_button_new_with_label (yes_button_text);
	gtk_widget_show (button);
	gtk_dialog_add_action_widget (GTK_DIALOG (d),
				      button,
				      GTK_RESPONSE_YES);

	/**/

	gtk_dialog_set_default_response (GTK_DIALOG (d), GTK_RESPONSE_YES);

	return d;
}


void
_gtk_error_dialog_from_gerror_run (GtkWindow   *parent,
				   const char  *title,
				   GError      *gerror)
{
	GtkWidget *d;

	g_return_if_fail (gerror != NULL);

	d = _gtk_message_dialog_new (parent,
				     GTK_DIALOG_DESTROY_WITH_PARENT,
				     _GTK_ICON_NAME_DIALOG_ERROR,
				     title,
				     gerror->message,
				     _GTK_LABEL_OK, GTK_RESPONSE_OK,
				     NULL);
	gtk_dialog_run (GTK_DIALOG (d));

	gtk_widget_destroy (d);
}


static void
error_dialog_response_cb (GtkDialog *dialog,
			  int        response,
			  gpointer   user_data)
{
	gtk_widget_destroy (GTK_WIDGET (dialog));
}


void
_gtk_error_dialog_from_gerror_show (GtkWindow   *parent,
				    const char  *title,
				    GError      *gerror)
{
	GtkWidget *d;

	d = _gtk_message_dialog_new (parent,
				     GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				     _GTK_ICON_NAME_DIALOG_ERROR,
				     title,
				     (gerror != NULL) ? gerror->message : NULL,
				     _GTK_LABEL_OK, GTK_RESPONSE_OK,
				     NULL);
	g_signal_connect (d, "response", G_CALLBACK (error_dialog_response_cb), NULL);

	gtk_window_present (GTK_WINDOW (d));
}


void
_gtk_error_dialog_run (GtkWindow        *parent,
		       const gchar      *format,
		       ...)
{
	GtkWidget *d;
	char      *message;
	va_list    args;

	va_start (args, format);
	message = g_strdup_vprintf (format, args);
	va_end (args);

	d =  _gtk_message_dialog_new (parent,
				      GTK_DIALOG_MODAL,
				      _GTK_ICON_NAME_DIALOG_ERROR,
				      message,
				      NULL,
				      _GTK_LABEL_CLOSE, GTK_RESPONSE_CANCEL,
				      NULL);
	g_free (message);

	g_signal_connect (G_OBJECT (d), "response",
			  G_CALLBACK (gtk_widget_destroy),
			  NULL);

	gtk_widget_show (d);
}


void
_gtk_error_dialog_show (GtkWindow  *parent,
			const char *title,
			const char *format,
			...)
{
	va_list    args;
	char      *message;
	GtkWidget *d;

	va_start (args, format);
	message = g_strdup_vprintf (format, args);
	va_end (args);

	d = _gtk_message_dialog_new (parent,
				     GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				     _GTK_ICON_NAME_DIALOG_ERROR,
				     title,
				     message,
				     _GTK_LABEL_OK, GTK_RESPONSE_OK,
				     NULL);
	g_signal_connect (d, "response", G_CALLBACK (error_dialog_response_cb), NULL);

	gtk_window_present (GTK_WINDOW (d));

	g_free (message);
}


void
_gtk_info_dialog_run (GtkWindow        *parent,
		      const gchar      *format,
		      ...)
{
	GtkWidget *d;
	char      *message;
	va_list    args;

	va_start (args, format);
	message = g_strdup_vprintf (format, args);
	va_end (args);

	d =  _gtk_message_dialog_new (parent,
				      GTK_DIALOG_MODAL,
				      _GTK_ICON_NAME_DIALOG_INFO,
				      message,
				      NULL,
				      _GTK_LABEL_CLOSE, GTK_RESPONSE_CANCEL,
				      NULL);
	g_free (message);

	g_signal_connect (G_OBJECT (d), "response",
			  G_CALLBACK (gtk_widget_destroy),
			  NULL);

	gtk_widget_show (d);
}


void
_gtk_dialog_add_to_window_group (GtkDialog *dialog)
{
	GtkWidget *toplevel;

	g_return_if_fail (dialog != NULL);

	toplevel = gtk_widget_get_toplevel (GTK_WIDGET (dialog));
	if (gtk_widget_is_toplevel (toplevel) && gtk_window_has_group (GTK_WINDOW (toplevel)))
		gtk_window_group_add_window (gtk_window_get_group (GTK_WINDOW (toplevel)), GTK_WINDOW (dialog));
}


GdkPixbuf *
_g_icon_get_pixbuf (GIcon        *icon,
		    int           icon_size,
		    GtkIconTheme *icon_theme)
{
	GdkPixbuf   *pixbuf = NULL;
	GtkIconInfo *icon_info;

	if (icon_theme == NULL)
		icon_theme = gtk_icon_theme_get_default ();

	icon_info = gtk_icon_theme_lookup_by_gicon (icon_theme,
						    icon,
						    icon_size,
						    GTK_ICON_LOOKUP_USE_BUILTIN);

	if (icon_info != NULL) {
		GError *error = NULL;

		pixbuf = gtk_icon_info_load_icon (icon_info, &error);
		if (error != NULL) {
			g_print ("%s\n", error->message);
			g_error_free (error);
		}

		g_object_unref (icon_info);
	}

	return pixbuf;
}


GdkPixbuf *
get_mime_type_pixbuf (const char   *mime_type,
		      int           icon_size,
		      GtkIconTheme *icon_theme)
{
	GdkPixbuf *pixbuf = NULL;
	GIcon     *icon;

	if (icon_theme == NULL)
		icon_theme = gtk_icon_theme_get_default ();

	icon = g_content_type_get_icon (mime_type);
	pixbuf = _g_icon_get_pixbuf (icon, icon_size, icon_theme);
	g_object_unref (icon);

	return pixbuf;
}


void
show_help_dialog (GtkWindow  *parent,
		  const char *section)
{
	char   *uri;
	GError *error = NULL;

	uri = g_strconcat ("help:gthumb", section ? "/" : NULL, section, NULL);
	if (! gtk_show_uri (gtk_window_get_screen (parent), uri, GDK_CURRENT_TIME, &error)) {
  		GtkWidget *dialog;

		dialog = _gtk_message_dialog_new (parent,
						  GTK_DIALOG_DESTROY_WITH_PARENT,
						  _GTK_ICON_NAME_DIALOG_ERROR,
						  _("Could not display help"),
						  error->message,
						  _GTK_LABEL_OK, GTK_RESPONSE_OK,
						  NULL);
		gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

		g_signal_connect (G_OBJECT (dialog), "response",
				  G_CALLBACK (gtk_widget_destroy),
				  NULL);

		gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

		gtk_widget_show (dialog);

		g_clear_error (&error);
	}
	g_free (uri);
}


void
_gtk_container_remove_children (GtkContainer *container,
				gpointer      start_after_this,
				gpointer      stop_before_this)
{
	GList *children;
	GList *scan;

	children = gtk_container_get_children (container);

	if (start_after_this != NULL) {
		for (scan = children; scan; scan = scan->next)
			if (scan->data == start_after_this) {
				scan = scan->next;
				break;
			}
	}
	else
		scan = children;

	for (/* void */; scan && (scan->data != stop_before_this); scan = scan->next)
		gtk_container_remove (container, scan->data);
	g_list_free (children);
}


int
_gtk_container_get_pos (GtkContainer *container,
			GtkWidget    *child)
{
	GList    *children;
	gboolean  found = FALSE;
	int       idx;
	GList    *scan;

	children = gtk_container_get_children (container);
	for (idx = 0, scan = children; ! found && scan; idx++, scan = scan->next) {
		if (child == scan->data) {
			found = TRUE;
			break;
		}
	}
	g_list_free (children);

	return ! found ? -1 : idx;
}


guint
_gtk_container_get_n_children (GtkContainer *container)
{
	GList *children;
	guint  len;

	children = gtk_container_get_children (container);
	len = g_list_length (children);
	g_list_free (children);

	return len;
}


GtkBuilder *
_gtk_builder_new_from_file (const char *ui_file,
			    const char *extension)
{
	char       *filename;
	GtkBuilder *builder;
	GError     *error = NULL;

#ifdef RUN_IN_PLACE
	if (extension == NULL)
		filename = g_build_filename (GTHUMB_UI_DIR, ui_file, NULL);
	else
		filename = g_build_filename (GTHUMB_EXTENSIONS_UI_DIR, extension, "data", "ui", ui_file, NULL);
#else
	filename = g_build_filename (GTHUMB_UI_DIR, ui_file, NULL);
#endif

	builder = gtk_builder_new ();
	if (! gtk_builder_add_from_file (builder, filename, &error)) {
		g_warning ("%s\n", error->message);
		g_clear_error (&error);
	}
	g_free (filename);

	return builder;
}


GtkBuilder *
_gtk_builder_new_from_resource (const char *resource_path)
{
	GtkBuilder *builder;
	char       *full_path;
	GError     *error = NULL;

	builder = gtk_builder_new ();
	full_path = g_strconcat (GTHUMB_RESOURCE_BASE_PATH, resource_path, NULL);
        if (! gtk_builder_add_from_resource (builder, full_path, &error)) {
                g_warning ("%s\n", error->message);
                g_clear_error (&error);
        }
	g_free (full_path);

        return builder;
}


GtkWidget *
_gtk_builder_get_widget (GtkBuilder *builder,
			 const char *name)
{
	return (GtkWidget *) gtk_builder_get_object (builder, name);
}


GtkWidget *
_gtk_combo_box_new_with_texts (const char *first_text,
			       ...)
{
	GtkWidget  *combo_box;
	va_list     args;
	const char *text;

	combo_box = gtk_combo_box_text_new ();

	va_start (args, first_text);

	text = first_text;
	while (text != NULL) {
		gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo_box), text);
		text = va_arg (args, const char *);
	}

	va_end (args);

	return combo_box;
}


void
_gtk_combo_box_append_texts (GtkComboBoxText *combo_box,
			     const char      *first_text,
			     ...)
{
	va_list     args;
	const char *text;

	va_start (args, first_text);

	text = first_text;
	while (text != NULL) {
		gtk_combo_box_text_append_text (combo_box, text);
		text = va_arg (args, const char *);
	}

	va_end (args);
}


GtkWidget *
_gtk_image_new_from_xpm_data (char * xpm_data[])
{
	GdkPixbuf *pixbuf;
	GtkWidget *image;

	pixbuf = gdk_pixbuf_new_from_xpm_data ((const char**) xpm_data);
	image = gtk_image_new_from_pixbuf (pixbuf);
	gtk_widget_show (image);

	g_object_unref (G_OBJECT (pixbuf));

	return image;
}


GtkWidget *
_gtk_image_new_from_inline (const guint8 *data)
{
	GdkPixbuf *pixbuf;
	GtkWidget *image;

	pixbuf = gdk_pixbuf_new_from_inline (-1, data, FALSE, NULL);
	image = gtk_image_new_from_pixbuf (pixbuf);
	gtk_widget_show (image);

	g_object_unref (G_OBJECT (pixbuf));

	return image;
}


void
_gtk_widget_get_screen_size (GtkWidget *widget,
			     int       *width,
			     int       *height)
{
	GdkScreen             *screen;
	cairo_rectangle_int_t  geometry;

	screen = gtk_widget_get_screen (widget);
	gdk_screen_get_monitor_geometry (screen,
					 gdk_screen_get_monitor_at_window (screen, gtk_widget_get_window (widget)),
					 &geometry);

	*width = geometry.width;
	*height = geometry.height;
}


int
_gtk_widget_get_allocated_width (GtkWidget *widget)
{
	int width = 0;

	if ((widget != NULL) && gtk_widget_get_mapped (widget)) {
		width = gtk_widget_get_allocated_width (widget);
		width += gtk_widget_get_margin_left (widget);
		width += gtk_widget_get_margin_right (widget);
	}

	return width;
}


int
_gtk_widget_get_allocated_height (GtkWidget *widget)
{
	int height = 0;

	if ((widget != NULL) && gtk_widget_get_mapped (widget)) {
		height = gtk_widget_get_allocated_height (widget);
		height += gtk_widget_get_margin_top (widget);
		height += gtk_widget_get_margin_bottom (widget);
	}

	return height;
}


int
_gtk_widget_lookup_for_size (GtkWidget   *widget,
			     GtkIconSize  icon_size)
{
	int w, h;

	gtk_icon_size_lookup (icon_size, &w, &h);

	return MAX (w, h);
}


void
_gtk_tree_path_list_free (GList *list)
{
	g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (list);
}


int
_gtk_paned_get_position2 (GtkPaned *paned)
{
	int           pos;
	GtkAllocation allocation;
	int           size;

	if (! gtk_widget_get_visible (GTK_WIDGET (paned)))
		return 0;

	pos = gtk_paned_get_position (paned);
	if (pos == 0)
		return 0;

	gtk_widget_get_allocation (GTK_WIDGET (paned), &allocation);
	if (gtk_orientable_get_orientation (GTK_ORIENTABLE (paned)) == GTK_ORIENTATION_HORIZONTAL)
		size = allocation.width;
	else
		size = allocation.height;

	if (size == 0)
		return 0;

	return size - pos;
}


void
_g_launch_command (GtkWidget  *parent,
		   const char *command,
		   const char *name,
		   GList      *files)
{
	GError              *error = NULL;
	GAppInfo            *app_info;
	GdkAppLaunchContext *launch_context;

	app_info = g_app_info_create_from_commandline (command, name, G_APP_INFO_CREATE_SUPPORTS_URIS, &error);
	if (app_info == NULL) {
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (parent), _("Could not launch the application"), error);
		g_clear_error (&error);
		return;
	}

	launch_context = gdk_display_get_app_launch_context (gtk_widget_get_display (parent));
	if (! g_app_info_launch (app_info, files, G_APP_LAUNCH_CONTEXT (launch_context), &error)) {
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (parent), _("Could not launch the application"), error);
		g_clear_error (&error);
		return;
	}

	g_object_unref (app_info);
}


void
_gtk_window_resize_to_fit_screen_height (GtkWidget *window,
					 int        default_width)
{
	GdkScreen *screen;

	screen = gtk_widget_get_screen (window);
	if ((screen != NULL) && (gdk_screen_get_height (screen) < 768))
		/* maximize on netbooks */
		gtk_window_maximize (GTK_WINDOW (window));
	else
		/* This should fit on a XGA/WXGA (height 768) screen
		 * with top and bottom panels */
		gtk_window_set_default_size (GTK_WINDOW (window), default_width, 670);
}


void
_gtk_info_bar_clear_action_area (GtkInfoBar *info_bar)
{
	_gtk_container_remove_children (GTK_CONTAINER (gtk_info_bar_get_action_area (info_bar)), NULL, NULL);
}


/* -- _gtk_menu_ask_drag_drop_action -- */


typedef struct {
	GMainLoop     *loop;
	GdkDragAction  action_name;
} DropActionData;


static void
ask_drag_drop_action_menu_deactivate_cb (GtkMenuShell *menushell,
					 gpointer      user_data)
{
	DropActionData *drop_data = user_data;

	if (g_main_loop_is_running (drop_data->loop))
		g_main_loop_quit (drop_data->loop);
}


static void
ask_drag_drop_action_item_activate_cb (GtkMenuItem *menuitem,
                		       gpointer     user_data)
{
	DropActionData *drop_data = user_data;

	drop_data->action_name = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (menuitem), "drop-action"));
	if (g_main_loop_is_running (drop_data->loop))
		g_main_loop_quit (drop_data->loop);
}


static void
_gtk_menu_ask_drag_drop_action_append_item (GtkWidget      *menu,
					    const char     *label,
					    GdkDragAction   actions,
					    GdkDragAction   action,
					    DropActionData *drop_data)
{
	GtkWidget *item;

	item = gtk_menu_item_new_with_mnemonic (label);
	g_object_set_data (G_OBJECT (item), "drop-action", GINT_TO_POINTER (action));
	gtk_widget_set_sensitive (item, ((actions & action) != 0));
	gtk_widget_show (item);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

	g_signal_connect (item,
			  "activate",
			  G_CALLBACK (ask_drag_drop_action_item_activate_cb),
			  drop_data);
}


GdkDragAction
_gtk_menu_ask_drag_drop_action (GtkWidget     *widget,
				GdkDragAction  actions,
				guint32        activate_time)
{
	DropActionData  drop_data;
	GtkWidget      *menu;
	GtkWidget      *item;

	drop_data.action_name = 0;
	drop_data.loop = g_main_loop_new (NULL, FALSE);

	menu = gtk_menu_new ();
	gtk_menu_set_screen (GTK_MENU (menu), gtk_widget_get_screen (widget));

	_gtk_menu_ask_drag_drop_action_append_item (menu,
						    _("_Copy Here"),
						    actions,
						    GDK_ACTION_COPY,
						    &drop_data);
	_gtk_menu_ask_drag_drop_action_append_item (menu,
						    _("_Move Here"),
						    actions,
						    GDK_ACTION_MOVE,
						    &drop_data);
	_gtk_menu_ask_drag_drop_action_append_item (menu,
						    _("_Link Here"),
						    actions,
						    GDK_ACTION_LINK,
						    &drop_data);

	item = gtk_separator_menu_item_new ();
	gtk_widget_show (item);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

	item = gtk_menu_item_new_with_label (_("Cancel"));
	gtk_widget_show (item);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

	g_signal_connect (menu,
        		  "deactivate",
                          G_CALLBACK (ask_drag_drop_action_menu_deactivate_cb),
                          &drop_data);

	gtk_menu_popup (GTK_MENU (menu),
			NULL,
			NULL,
			NULL,
			NULL,
			0,
			activate_time);
	gtk_grab_add (menu);
	g_main_loop_run (drop_data.loop);

	gtk_grab_remove (menu);
	gtk_widget_destroy (menu);
	g_main_loop_unref (drop_data.loop);

	return drop_data.action_name;
}


static gboolean
_gdk_rgba_shade (GdkRGBA *color,
		 GdkRGBA *result,
		 gdouble  factor)
{
	guchar hue, sat, lum;
	guchar red, green, blue;
	double tmp;

	gimp_rgb_to_hsl (color->red * 255,
			 color->green * 255,
			 color->blue * 255,
			 &hue,
			 &sat,
			 &lum);

	tmp = factor * (double) sat;
	sat = (guchar) CLAMP (tmp, 0, 255);

	tmp = factor * (double) lum;
	lum = (guchar) CLAMP (tmp, 0, 255);

	gimp_hsl_to_rgb (hue, sat, lum, &red, &green, &blue);

	result->red = (double) red / 255.0;
	result->green = (double) green / 255.0;
	result->blue = (double) blue / 255.0;
	result->alpha = color->alpha;

	return TRUE;
}


gboolean
_gdk_rgba_darker (GdkRGBA *color,
		  GdkRGBA *result)
{
	return _gdk_rgba_shade (color, result, 0.8);
}


gboolean
_gdk_rgba_lighter (GdkRGBA *color,
		   GdkRGBA *result)
{
	return _gdk_rgba_shade (color, result, 1.2);
}


GtkIconTheme *
_gtk_widget_get_icon_theme (GtkWidget *widget)
{
	GdkScreen *screen;

	screen = gtk_widget_get_screen (widget);
	if (screen == NULL)
		return NULL;

	return gtk_icon_theme_get_for_screen (screen);
}


void
_gtk_combo_box_add_image_sizes (GtkComboBox *combo_box,
				int          active_width,
				int          active_height)
{
	GtkListStore *list_store;
	int           active_index;
	int           i;

	list_store = GTK_LIST_STORE (gtk_combo_box_get_model (combo_box));
	active_index = 0;
	for (i = 0; i < G_N_ELEMENTS (ImageSizeValues); i++) {
		GtkTreeIter  iter;
		char        *name;

		gtk_list_store_append (list_store, &iter);

		if ((ImageSizeValues[i].width == active_width) && (ImageSizeValues[i].height == active_height))
			active_index = i;

		/* Translators: this is an image size, such as 1024 × 768 */
		name = g_strdup_printf (_("%d × %d"), ImageSizeValues[i].width, ImageSizeValues[i].height);
		gtk_list_store_set (list_store, &iter,
				    0, name,
				    -1);

		g_free (name);
	}
	gtk_combo_box_set_active (combo_box, active_index);
}


gboolean
_gtk_file_chooser_set_file_parent (GtkFileChooser   *chooser,
				   GFile            *file,
				   GError          **error)
{
	GFile *parent;
	gboolean result;

	parent = g_file_get_parent (file);
	result = gtk_file_chooser_set_file (chooser, parent, error);

	if (parent != NULL)
		g_object_unref (parent);

	return result;
}


static void
_gtk_menu_button_set_style_for_header_bar (GtkWidget *button)
{
	GtkStyleContext *context;

	gtk_widget_set_valign (button, GTK_ALIGN_CENTER);
	context = gtk_widget_get_style_context (button);
	gtk_style_context_add_class (context, "image-button");
	gtk_style_context_remove_class (context, "text-button");
}


GtkWidget *
_gtk_menu_button_new_for_header_bar (void)
{
	GtkWidget *button;

	button = gtk_menu_button_new ();
	_gtk_menu_button_set_style_for_header_bar (button);

	return button;
}


GtkWidget *
_gtk_image_button_new_for_header_bar (const char *icon_name)
{
	GtkWidget *button;

	button = gtk_button_new_from_icon_name (icon_name, GTK_ICON_SIZE_MENU);
	_gtk_menu_button_set_style_for_header_bar (button);

	return button;
}


GtkWidget *
_gtk_toggle_image_button_new_for_header_bar (const char *icon_name)
{
	GtkWidget *button;
	GtkWidget *image;

	button = gtk_toggle_button_new ();
	image = gtk_image_new_from_icon_name (icon_name, GTK_ICON_SIZE_MENU);
	gtk_widget_show (image);
	gtk_container_add (GTK_CONTAINER (button), image);
	_gtk_menu_button_set_style_for_header_bar (button);

	return button;
}


/* -- _gtk_window_add_accelerator_for_action -- */


typedef struct {
	GtkWindow *window;
	char      *action_name;
	GVariant  *target;
} AccelData;


static void
accel_data_free (gpointer  user_data,
                 GClosure *closure)
{
	AccelData *accel_data = user_data;

	g_return_if_fail (accel_data != NULL);

	if (accel_data->target != NULL)
		g_variant_unref (accel_data->target);
	g_free (accel_data->action_name);
	g_free (accel_data);
}


static void
window_accelerator_activated_cb (GtkAccelGroup	*accel_group,
				 GObject		*object,
				 guint		 key,
				 GdkModifierType	 mod,
				 gpointer		 user_data)
{
	AccelData *accel_data = user_data;
	GAction   *action;

	action = g_action_map_lookup_action (G_ACTION_MAP (accel_data->window), accel_data->action_name);
	if (action != NULL)
		g_action_activate (action, accel_data->target);
}


void
_gtk_window_add_accelerator_for_action (GtkWindow	*window,
					GtkAccelGroup	*accel_group,
					const char	*action_name,
					const char	*accel,
					GVariant	*target)
{
	AccelData	*accel_data;
	guint		 key;
	GdkModifierType  mods;
	GClosure	*closure;

	if ((action_name == NULL) || (accel == NULL))
		return;

	if (g_str_has_prefix (action_name, "app."))
		return;

	accel_data = g_new0 (AccelData, 1);
	accel_data->window = window;
	/* remove the win. prefix from the action name */
	if (g_str_has_prefix (action_name, "win."))
		accel_data->action_name = g_strdup (action_name + strlen ("win."));
	else
		accel_data->action_name = g_strdup (action_name);
	if (target != NULL)
		accel_data->target = g_variant_ref (target);

	gtk_accelerator_parse (accel, &key, &mods);
	closure = g_cclosure_new (G_CALLBACK (window_accelerator_activated_cb),
				  accel_data,
				  accel_data_free);
	gtk_accel_group_connect (accel_group,
				 key,
				 mods,
				 0,
				 closure);
}


/* -- _gtk_window_add_accelerators_from_menu --  */


static void
add_accelerators_from_menu_item (GtkWindow      *window,
				 GtkAccelGroup  *accel_group,
				 GMenuModel     *model,
				 int             item)
{
	GMenuAttributeIter	*iter;
	const char		*key;
	GVariant		*value;
	const char		*accel = NULL;
	const char		*action = NULL;
	GVariant		*target = NULL;

	iter = g_menu_model_iterate_item_attributes (model, item);
	while (g_menu_attribute_iter_get_next (iter, &key, &value)) {
		if (g_str_equal (key, "action") && g_variant_is_of_type (value, G_VARIANT_TYPE_STRING))
			action = g_variant_get_string (value, NULL);
		else if (g_str_equal (key, "accel") && g_variant_is_of_type (value, G_VARIANT_TYPE_STRING))
			accel = g_variant_get_string (value, NULL);
		else if (g_str_equal (key, "target"))
			target = g_variant_ref (value);
		g_variant_unref (value);
	}
	g_object_unref (iter);

	_gtk_window_add_accelerator_for_action (window,
						accel_group,
						action,
						accel,
						target);

	if (target != NULL)
		g_variant_unref (target);
}


static void
add_accelerators_from_menu (GtkWindow      *window,
			    GtkAccelGroup  *accel_group,
			    GMenuModel     *model)
{
	int		 i;
	GMenuLinkIter	*iter;
	const char	*key;
	GMenuModel	*m;

	for (i = 0; i < g_menu_model_get_n_items (model); i++) {
		add_accelerators_from_menu_item (window, accel_group, model, i);

		iter = g_menu_model_iterate_item_links (model, i);
		while (g_menu_link_iter_get_next (iter, &key, &m)) {
			add_accelerators_from_menu (window, accel_group, m);
			g_object_unref (m);
		}
		g_object_unref (iter);
	}
}


void
_gtk_window_add_accelerators_from_menu (GtkWindow  *window,
					GMenuModel *menu)
{
	GtkAccelGroup *accel_group;

	accel_group = gtk_accel_group_new ();
	add_accelerators_from_menu (window, accel_group, menu);
	gtk_window_add_accel_group (window, accel_group);
}


gboolean
_gtk_window_get_is_maximized (GtkWindow *window)
{
	GdkWindow *gdk_win;

	gdk_win = gtk_widget_get_window (GTK_WIDGET (window));
	if (gdk_win == NULL)
		return FALSE;

	return (gdk_window_get_state (gdk_win) & GDK_WINDOW_STATE_MAXIMIZED) != 0;
}
