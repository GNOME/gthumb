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
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include <string.h>
#include "gconf-utils.h"
#include "gtk-utils.h"
#include "pixbuf-utils.h"

#define REQUEST_ENTRY_WIDTH 220


static GtkWidget *
create_button (const char *stock_id,
	       const char *text)
{
	GtkWidget    *button;
	GtkWidget    *hbox;
	GtkWidget    *image;
	GtkWidget    *label;
	GtkWidget    *align;
	const char   *label_text;
	gboolean      text_is_stock;
	GtkStockItem  stock_item;

	button = gtk_button_new ();

	if (gtk_stock_lookup (text, &stock_item)) {
		label_text = stock_item.label;
		text_is_stock = TRUE;
	} else {
		label_text = text;
		text_is_stock = FALSE;
	}

	if (text_is_stock)
		image = gtk_image_new_from_stock (text, GTK_ICON_SIZE_BUTTON);
	else
		image = gtk_image_new_from_stock (stock_id, GTK_ICON_SIZE_BUTTON);
	label = gtk_label_new_with_mnemonic (label_text);
	hbox = gtk_hbox_new (FALSE, 2);
	align = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);

	GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), GTK_WIDGET (button));

	gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
	gtk_box_pack_end (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	gtk_container_add (GTK_CONTAINER (button), align);
	gtk_container_add (GTK_CONTAINER (align), hbox);

	gtk_widget_show_all (button);

	return button;
}


GtkWidget*
_gtk_message_dialog_new (GtkWindow        *parent,
			 GtkDialogFlags    flags,
			 const char       *stock_id,
			 const char       *message,
			 const char       *secondary_message,
			 const char       *first_button_text,
			 ...)
{
	GtkWidget    *d;
	GtkWidget    *label;
	GtkWidget    *image;
	GtkWidget    *hbox;
	va_list       args;
	const gchar  *text;
	int           response_id;
	char         *escaped_message, *markup_text;

	g_return_val_if_fail (message != NULL, NULL);

	if (stock_id == NULL)
		stock_id = GTK_STOCK_DIALOG_INFO;

	d = gtk_dialog_new_with_buttons ("", parent, flags, NULL);
	gtk_window_set_resizable (GTK_WINDOW (d), FALSE);

	gtk_dialog_set_has_separator (GTK_DIALOG (d), FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (d), 6);
	gtk_container_set_border_width (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (d))), 6);
	gtk_box_set_spacing (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (d))), 8);

	/* Add label and image */

	image = gtk_image_new_from_stock (stock_id, GTK_ICON_SIZE_DIALOG);
	gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0.0);

	label = gtk_label_new ("");

	escaped_message = g_markup_escape_text (message, -1);
	if (secondary_message != NULL) {
		char *escaped_secondary_message = g_markup_escape_text (secondary_message, -1);
		markup_text = g_strdup_printf ("<span weight=\"bold\" size=\"larger\">%s</span>\n\n%s",
					       escaped_message,
					       escaped_secondary_message);
		g_free (escaped_secondary_message);
	}
	else
		markup_text = g_strdup (escaped_message);
	gtk_label_set_markup (GTK_LABEL (label), markup_text);
	g_free (markup_text);
	g_free (escaped_message);

	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	gtk_label_set_selectable (GTK_LABEL (label), TRUE);

	hbox = gtk_hbox_new (FALSE, 18);
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

	if (first_button_text == NULL)
		return d;

	va_start (args, first_button_text);

	text = first_button_text;
	response_id = va_arg (args, gint);

	while (text != NULL) {
		gtk_dialog_add_button (GTK_DIALOG (d), text, response_id);

		text = va_arg (args, gchar*);
		if (text == NULL)
			break;
		response_id = va_arg (args, int);
	}

	va_end (args);

	gtk_dialog_set_default_response (GTK_DIALOG (d), GTK_RESPONSE_YES);

	return d;
}


char *
_gtk_request_dialog_run (GtkWindow        *parent,
			 GtkDialogFlags    flags,
			 const char       *message,
			 const char       *default_value,
			 int               max_length,
			 const char       *no_button_text,
			 const char       *yes_button_text)
{
	GtkWidget    *d;
	GtkWidget    *label;
	GtkWidget    *image;
	GtkWidget    *hbox;
	GtkWidget    *vbox;
	GtkWidget    *entry;
	GtkWidget    *button;
	char         *result = NULL;
	char         *stock_id = GTK_STOCK_DIALOG_QUESTION;

	d = gtk_dialog_new_with_buttons ("", parent, flags, NULL);
	gtk_window_set_resizable (GTK_WINDOW (d), FALSE);

	gtk_dialog_set_has_separator (GTK_DIALOG (d), FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (d), 6);
	gtk_container_set_border_width (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (d))), 6);
	gtk_box_set_spacing (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (d))), 12);

	/* Add label and image */

	image = gtk_image_new_from_stock (stock_id, GTK_ICON_SIZE_DIALOG);
	gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0.0);

	label = gtk_label_new (message);
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	gtk_label_set_selectable (GTK_LABEL (label), FALSE);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);

	entry = gtk_entry_new ();
	gtk_widget_set_size_request (entry, REQUEST_ENTRY_WIDTH, -1);
	gtk_entry_set_max_length (GTK_ENTRY (entry), max_length);
	gtk_entry_set_text (GTK_ENTRY (entry), default_value);
	gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);

	hbox = gtk_hbox_new (FALSE, 6);
	vbox = gtk_vbox_new (FALSE, 6);

	gtk_container_set_border_width (GTK_CONTAINER (hbox), 6);
	gtk_box_set_spacing (GTK_BOX (hbox), 12);
	gtk_box_set_spacing (GTK_BOX (vbox), 6);

	gtk_box_pack_start (GTK_BOX (vbox), label,
			    TRUE, TRUE, 0);

	gtk_box_pack_start (GTK_BOX (vbox), entry,
			    FALSE, FALSE, 0);

	gtk_box_pack_start (GTK_BOX (hbox), image,
			    FALSE, FALSE, 0);

	gtk_box_pack_start (GTK_BOX (hbox), vbox,
			    TRUE, TRUE, 0);

	gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (d))),
			    hbox,
			    FALSE, FALSE, 0);

	gtk_widget_show_all (hbox);

	/* Add buttons */

	button = create_button (GTK_STOCK_CANCEL, no_button_text);
	gtk_dialog_add_action_widget (GTK_DIALOG (d),
				      button,
				      GTK_RESPONSE_CANCEL);

	/**/

	button = create_button (GTK_STOCK_OK, yes_button_text);
	gtk_dialog_add_action_widget (GTK_DIALOG (d),
				      button,
				      GTK_RESPONSE_YES);

	/**/

	gtk_dialog_set_default_response (GTK_DIALOG (d),
					 GTK_RESPONSE_YES);
	gtk_widget_grab_focus (entry);

	/* Run dialog */

	if ((gtk_dialog_run (GTK_DIALOG (d)) == GTK_RESPONSE_YES) &&
	    (strlen (gtk_entry_get_text (GTK_ENTRY (entry))) > 0) )
		/* Normalize unicode text to "NFC" form for consistency. */
		result = g_utf8_normalize (gtk_entry_get_text (GTK_ENTRY (entry)),
					   -1,
					   G_NORMALIZE_NFC);
	else
		result = NULL;

	gtk_widget_destroy (d);

	return result;
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
	char         *stock_id = GTK_STOCK_DIALOG_QUESTION;

	d = gtk_dialog_new_with_buttons ("", parent, flags, NULL);
	gtk_window_set_resizable (GTK_WINDOW (d), FALSE);

	gtk_dialog_set_has_separator (GTK_DIALOG (d), FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (d), 6);
	gtk_container_set_border_width (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (d))), 6);
	gtk_box_set_spacing (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (d))), 8);

	/* Add label and image */

	image = gtk_image_new_from_stock (stock_id, GTK_ICON_SIZE_DIALOG);
	gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0.0);

	label = gtk_label_new (message);
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	gtk_label_set_selectable (GTK_LABEL (label), TRUE);

	hbox = gtk_hbox_new (FALSE, 12);
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

	button = create_button (GTK_STOCK_CANCEL, no_button_text);
	gtk_dialog_add_action_widget (GTK_DIALOG (d),
				      button,
				      GTK_RESPONSE_CANCEL);

	/**/

	button = create_button (GTK_STOCK_OK, yes_button_text);
	gtk_dialog_add_action_widget (GTK_DIALOG (d),
				      button,
				      GTK_RESPONSE_YES);

	/**/

	gtk_dialog_set_default_response (GTK_DIALOG (d), GTK_RESPONSE_YES);

	return d;
}


static void
yesno__check_button_toggled_cb (GtkToggleButton *button,
				const char      *gconf_key)
{
	eel_gconf_set_boolean (gconf_key,
			       ! gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)));
}


GtkWidget*
_gtk_yesno_dialog_with_checkbutton_new (GtkWindow        *parent,
					GtkDialogFlags    flags,
					const char       *message,
					const char       *no_button_text,
					const char       *yes_button_text,
					const char       *check_button_label,
					const char       *gconf_key)
{
	GtkWidget *d;
	GtkWidget *label;
	GtkWidget *image;
	GtkWidget *hbox;
	GtkWidget *button;
	GtkWidget *check_button;
	char      *stock_id = GTK_STOCK_DIALOG_QUESTION;

	d = gtk_dialog_new_with_buttons ("", parent, flags, NULL);
	gtk_window_set_resizable (GTK_WINDOW (d), FALSE);

	gtk_dialog_set_has_separator (GTK_DIALOG (d), FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (d), 6);
	gtk_container_set_border_width (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (d))), 6);
	gtk_box_set_spacing (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (d))), 8);

	/* Add label and image */

	image = gtk_image_new_from_stock (stock_id, GTK_ICON_SIZE_DIALOG);
	gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0.0);

	label = gtk_label_new (message);
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	gtk_label_set_selectable (GTK_LABEL (label), TRUE);

	hbox = gtk_hbox_new (FALSE, 12);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 6);

	gtk_box_pack_start (GTK_BOX (hbox), image,
			    FALSE, FALSE, 0);

	gtk_box_pack_start (GTK_BOX (hbox), label,
			    TRUE, TRUE, 0);

	gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (d))),
			    hbox,
			    FALSE, FALSE, 0);

	/* Add checkbutton */

	check_button = gtk_check_button_new_with_mnemonic (check_button_label);
	gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (d))),
			    check_button,
			    FALSE, FALSE, 0);
	gtk_widget_show (check_button);

	gtk_widget_show_all (hbox);

	/* Add buttons */

	button = create_button (GTK_STOCK_CANCEL, no_button_text);
	gtk_dialog_add_action_widget (GTK_DIALOG (d),
				      button,
				      GTK_RESPONSE_CANCEL);

	/**/

	button = create_button (GTK_STOCK_OK, yes_button_text);
	gtk_dialog_add_action_widget (GTK_DIALOG (d),
				      button,
				      GTK_RESPONSE_YES);

	/**/

	gtk_dialog_set_default_response (GTK_DIALOG (d), GTK_RESPONSE_YES);

	g_signal_connect (G_OBJECT (check_button),
			  "toggled",
			  G_CALLBACK (yesno__check_button_toggled_cb),
			  (gpointer) gconf_key);

	return d;
}


GtkWidget*
_gtk_message_dialog_with_checkbutton_new (GtkWindow        *parent,
			 		  GtkDialogFlags    flags,
			 		  const char       *stock_id,
			 		  const char       *message,
			 		  const char       *secondary_message,
			 		  const char       *gconf_key,
			 		  const char       *check_button_label,
			 		  const char       *first_button_text,
			 		  ...)
{
	GtkWidget  *d;
	GtkWidget  *label;
	GtkWidget  *image;
	GtkWidget  *hbox;
	GtkWidget  *check_button;
	va_list     args;
	const char *text;
	int         response_id;
	char       *escaped_message, *markup_text;

	g_return_val_if_fail (message != NULL, NULL);

	if (stock_id == NULL)
		stock_id = GTK_STOCK_DIALOG_INFO;

	d = gtk_dialog_new_with_buttons ("", parent, flags, NULL);
	gtk_window_set_resizable (GTK_WINDOW (d), FALSE);

	gtk_dialog_set_has_separator (GTK_DIALOG (d), FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (d), 6);
	gtk_container_set_border_width (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (d))), 6);
	gtk_box_set_spacing (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (d))), 8);

	/* Add label and image */

	image = gtk_image_new_from_stock (stock_id, GTK_ICON_SIZE_DIALOG);
	gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0.0);

	label = gtk_label_new ("");

	escaped_message = g_markup_escape_text (message, -1);
	if (secondary_message != NULL) {
		char *escaped_secondary_message = g_markup_escape_text (secondary_message, -1);
		markup_text = g_strdup_printf ("<span weight=\"bold\" size=\"larger\">%s</span>\n\n%s",
					       escaped_message,
					       escaped_secondary_message);
		g_free (escaped_secondary_message);
	} else
		markup_text = g_strdup (escaped_message);
	gtk_label_set_markup (GTK_LABEL (label), markup_text);
	g_free (markup_text);
	g_free (escaped_message);

	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	gtk_label_set_selectable (GTK_LABEL (label), TRUE);

	hbox = gtk_hbox_new (FALSE, 6);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 6);

	gtk_box_pack_start (GTK_BOX (hbox), image,
			    FALSE, FALSE, 0);

	gtk_box_pack_start (GTK_BOX (hbox), label,
			    TRUE, TRUE, 0);

	gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (d))),
			    hbox,
			    FALSE, FALSE, 0);

	gtk_widget_show_all (hbox);

	/* Add checkbutton */

	check_button = gtk_check_button_new_with_mnemonic (check_button_label);
	gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (d))),
			    check_button,
			    FALSE, FALSE, 0);
	gtk_widget_show (check_button);

	/* Add buttons */

	if (first_button_text == NULL)
		return d;

	va_start (args, first_button_text);

	text = first_button_text;
	response_id = va_arg (args, gint);

	while (text != NULL) {
		gtk_dialog_add_button (GTK_DIALOG (d), text, response_id);

		text = va_arg (args, gchar*);
		if (text == NULL)
			break;
		response_id = va_arg (args, int);
	}

	va_end (args);

	gtk_dialog_set_default_response (GTK_DIALOG (d), GTK_RESPONSE_YES);

	g_signal_connect (G_OBJECT (check_button),
			  "toggled",
			  G_CALLBACK (yesno__check_button_toggled_cb),
			  (gpointer) gconf_key);

	return d;
}


void
_gtk_error_dialog_from_gerror_run (GtkWindow   *parent,
				   const char  *title,
				   GError     **gerror)
{
	GtkWidget *d;

	g_return_if_fail (*gerror != NULL);

	d = _gtk_message_dialog_new (parent,
				     GTK_DIALOG_DESTROY_WITH_PARENT,
				     GTK_STOCK_DIALOG_ERROR,
				     title,
				     (*gerror)->message,
				     GTK_STOCK_OK, GTK_RESPONSE_OK,
				     NULL);
	gtk_dialog_run (GTK_DIALOG (d));

	gtk_widget_destroy (d);
	g_clear_error (gerror);
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
				    GError     **gerror)
{
	GtkWidget *d;

	d = _gtk_message_dialog_new (parent,
				     GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				     GTK_STOCK_DIALOG_ERROR,
				     title,
				     (gerror != NULL) ? (*gerror)->message : NULL,
				     GTK_STOCK_OK, GTK_RESPONSE_OK,
				     NULL);
	g_signal_connect (d, "response", G_CALLBACK (error_dialog_response_cb), NULL);

	gtk_window_present (GTK_WINDOW (d));

	if (gerror != NULL)
		g_clear_error (gerror);
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
				      GTK_STOCK_DIALOG_ERROR,
				      message,
				      NULL,
				      GTK_STOCK_CLOSE, GTK_RESPONSE_CANCEL,
				      NULL);
	g_free (message);

	g_signal_connect (G_OBJECT (d), "response",
			  G_CALLBACK (gtk_widget_destroy),
			  NULL);

	gtk_widget_show (d);
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
				      GTK_STOCK_DIALOG_INFO,
				      message,
				      NULL,
				      GTK_STOCK_CLOSE, GTK_RESPONSE_CANCEL,
				      NULL);
	g_free (message);

	g_signal_connect (G_OBJECT (d), "response",
			  G_CALLBACK (gtk_widget_destroy),
			  NULL);

	gtk_widget_show (d);
}


static GdkPixbuf *
get_themed_icon_pixbuf (GThemedIcon  *icon,
			int           size,
			GtkIconTheme *icon_theme)
{
	char        **icon_names;
	GtkIconInfo  *icon_info;
	GdkPixbuf    *pixbuf;
	GError       *error = NULL;

	g_object_get (icon, "names", &icon_names, NULL);

	icon_info = gtk_icon_theme_choose_icon (icon_theme, (const char **)icon_names, size, 0);
	if (icon_info == NULL)
		icon_info = gtk_icon_theme_lookup_icon (icon_theme, "text-x-generic", size, GTK_ICON_LOOKUP_USE_BUILTIN);

	pixbuf = gtk_icon_info_load_icon (icon_info, &error);
	if (pixbuf == NULL) {
		g_warning ("could not load icon pixbuf: %s\n", error->message);
		g_clear_error (&error);
	}

	gtk_icon_info_free (icon_info);
	g_strfreev (icon_names);

	return pixbuf;
}


static GdkPixbuf *
get_file_icon_pixbuf (GFileIcon *icon,
		      int        size)
{
	GFile     *file;
	char      *filename;
	GdkPixbuf *pixbuf;

	file = g_file_icon_get_file (icon);
	filename = g_file_get_path (file);
	pixbuf = gdk_pixbuf_new_from_file_at_size (filename, size, -1, NULL);
	g_free (filename);
	g_object_unref (file);

	return pixbuf;
}


GdkPixbuf *
_g_icon_get_pixbuf (GIcon        *icon,
		    int           size,
		    GtkIconTheme *theme)
{
	GdkPixbuf *pixbuf;
	int        w, h;

	if (icon == NULL)
		return NULL;

	if (G_IS_THEMED_ICON (icon))
		pixbuf = get_themed_icon_pixbuf (G_THEMED_ICON (icon), size, theme);
	if (G_IS_FILE_ICON (icon))
		pixbuf = get_file_icon_pixbuf (G_FILE_ICON (icon), size);

	if (pixbuf == NULL)
		return NULL;

	w = gdk_pixbuf_get_width (pixbuf);
	h = gdk_pixbuf_get_height (pixbuf);
	if (scale_keeping_ratio (&w, &h, size, size, FALSE)) {
		GdkPixbuf *scaled;

		scaled = gdk_pixbuf_scale_simple (pixbuf, w, h, GDK_INTERP_BILINEAR);
		g_object_unref (pixbuf);
		pixbuf = scaled;
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


int
_gtk_icon_get_pixel_size (GtkWidget   *widget,
			  GtkIconSize  size)
{
	int icon_width, icon_height;

	gtk_icon_size_lookup_for_settings (gtk_widget_get_settings (widget),
					   size,
					   &icon_width, &icon_height);
	return MAX (icon_width, icon_height);
}


void
show_help_dialog (GtkWindow  *parent,
		  const char *section)
{
	char   *uri;
	GError *error = NULL;

	uri = g_strconcat ("ghelp:gthumb", section ? "?" : NULL, section, NULL);
	if (! gtk_show_uri (gtk_window_get_screen (parent), uri, GDK_CURRENT_TIME, &error)) {
  		GtkWidget *dialog;

		dialog = _gtk_message_dialog_new (parent,
						  GTK_DIALOG_DESTROY_WITH_PARENT,
						  GTK_STOCK_DIALOG_ERROR,
						  _("Could not display help"),
						  error->message,
						  GTK_STOCK_OK, GTK_RESPONSE_OK,
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

	combo_box = gtk_combo_box_new_text ();

	va_start (args, first_text);

	text = first_text;
	while (text != NULL) {
		gtk_combo_box_append_text (GTK_COMBO_BOX (combo_box), text);
		text = va_arg (args, const char *);
	}

	va_end (args);

	return combo_box;
}


void
_gtk_combo_box_append_texts (GtkComboBox *combo_box,
			     const char  *first_text,
			     ...)
{
	va_list     args;
	const char *text;

	va_start (args, first_text);

	text = first_text;
	while (text != NULL) {
		gtk_combo_box_append_text (combo_box, text);
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
	GdkScreen    *screen;
	GdkRectangle  screen_geom;

	screen = gtk_widget_get_screen (widget);
	gdk_screen_get_monitor_geometry (screen,
					 gdk_screen_get_monitor_at_window (screen, widget->window),
					 &screen_geom);

	*width = screen_geom.width;
	*height = screen_geom.height;
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
	GtkRequisition requisition;
	int            pos;
	int            size;

	if (! GTK_WIDGET_VISIBLE (paned))
		return 0;

	pos = gtk_paned_get_position (paned);

	gtk_window_get_size (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (paned))), &(requisition.width), &(requisition.height));
	if (gtk_orientable_get_orientation (GTK_ORIENTABLE (paned)) == GTK_ORIENTATION_HORIZONTAL)
		size = requisition.width;
	else
		size = requisition.height;

	return size - pos;
}


void
_gtk_paned_set_position2 (GtkPaned *paned,
			  int       pos)
{
	GtkWidget *top_level;
	int        size;

	top_level = gtk_widget_get_toplevel (GTK_WIDGET (paned));
	if (gtk_orientable_get_orientation (GTK_ORIENTABLE (paned)) == GTK_ORIENTATION_HORIZONTAL)
		size = top_level->allocation.width;
	else
		size = top_level->allocation.height;

	if (pos > 0)
		gtk_paned_set_position (paned, size - pos);
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
		_gtk_error_dialog_from_gerror_show(GTK_WINDOW (parent), _("Could not launch the application"), &error);
		return;
	}

	launch_context = gdk_app_launch_context_new ();
	gdk_app_launch_context_set_screen (launch_context, gtk_widget_get_screen (parent));

	if (! g_app_info_launch (app_info, files, G_APP_LAUNCH_CONTEXT (launch_context), &error)) {
		_gtk_error_dialog_from_gerror_show(GTK_WINDOW (parent), _("Could not launch the application"), &error);
		return;
	}

	g_object_unref (app_info);
}
