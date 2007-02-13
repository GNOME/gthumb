/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001, 2003 Free Software Foundation, Inc.
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
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <libgnome/libgnome.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <glade/glade.h>
#include "gtk-utils.h"
#include "gconf-utils.h"
#include "file-utils.h"
#include "glib-utils.h"

#define REQUEST_ENTRY_WIDTH 220


GtkWidget *
_gtk_image_new_from_xpm_data (char * xpm_data[])
{
	GtkWidget *image;
	GdkPixbuf *pixbuf;

	pixbuf = gdk_pixbuf_new_from_xpm_data ((const char**) xpm_data);
	image = gtk_image_new_from_pixbuf (pixbuf);
	g_object_unref (G_OBJECT (pixbuf));

	return image;
}


GtkWidget *
_gtk_image_new_from_inline (const guint8 *data)
{
	GtkWidget *image;
	GdkPixbuf *pixbuf;

	pixbuf = gdk_pixbuf_new_from_inline (-1, data, FALSE, NULL);
	image = gtk_image_new_from_pixbuf (pixbuf);
	g_object_unref (G_OBJECT (pixbuf));

	return image;
}


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
	gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG (d)->vbox), 6);
	gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (d)->vbox), 8);

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

	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (d)->vbox),
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
	gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG (d)->vbox), 6);
	gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (d)->vbox), 12);

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

	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (d)->vbox),
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

	if (gtk_dialog_run (GTK_DIALOG (d)) == GTK_RESPONSE_YES)
		result = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));
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
	gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG (d)->vbox), 6);
	gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (d)->vbox), 8);

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

	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (d)->vbox),
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
	gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG (d)->vbox), 6);
	gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (d)->vbox), 8);

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

	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (d)->vbox),
			    hbox,
			    FALSE, FALSE, 0);

	/* Add checkbutton */

	check_button = gtk_check_button_new_with_mnemonic (check_button_label);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (d)->vbox),
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
	gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG (d)->vbox), 6);
	gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (d)->vbox), 8);

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

	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (d)->vbox),
			    hbox,
			    FALSE, FALSE, 0);

	gtk_widget_show_all (hbox);

	/* Add checkbutton */

	check_button = gtk_check_button_new_with_mnemonic (check_button_label);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (d)->vbox),
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
_gtk_error_dialog_from_gerror_run (GtkWindow        *parent,
				   GError          **gerror)
{
	GtkWidget *d;

	g_return_if_fail (*gerror != NULL);
	g_return_if_fail ((*gerror)->message != NULL);

	if ((*gerror)->message != NULL) {
		d = _gtk_message_dialog_new (parent,
					     GTK_DIALOG_MODAL,
					     GTK_STOCK_DIALOG_ERROR,
					     (*gerror)->message,
					     NULL,
					     GTK_STOCK_OK, GTK_RESPONSE_CANCEL,
					     NULL);
		g_signal_connect (G_OBJECT (d), "response",
				  G_CALLBACK (gtk_widget_destroy),
				  NULL);
		gtk_widget_show (d);
	}

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


void
_gtk_entry_set_locale_text (GtkEntry   *entry,
			    const char *text)
{
	char *utf8_text;

	if (text == NULL)
		text = "";

	utf8_text = g_locale_to_utf8 (text, -1, NULL, NULL, NULL);
	gtk_entry_set_text (entry, utf8_text);
	g_free (utf8_text);
}


char *
_gtk_entry_get_locale_text (GtkEntry *entry)
{
	const char *utf8_text;
	char       *text;

	utf8_text = gtk_entry_get_text (entry);
	if (utf8_text == NULL)
		return NULL;

	text = g_locale_from_utf8 (utf8_text, -1, NULL, NULL, NULL);

	return text;
}


void
_gtk_label_set_locale_text (GtkLabel   *label,
			    const char *text)
{
	char *utf8_text;

	if (text == NULL)
		text = "";

	utf8_text = g_locale_to_utf8 (text, -1, NULL, NULL, NULL);
	gtk_label_set_text (label, utf8_text);
	g_free (utf8_text);
}


char *
_gtk_label_get_locale_text (GtkLabel *label)
{
	const char *utf8_text;
	char       *text;

	utf8_text = gtk_label_get_text (label);
	if (utf8_text == NULL)
		return NULL;

	text = g_locale_from_utf8 (utf8_text, -1, NULL, NULL, NULL);

	return text;
}


void
_gtk_entry_set_filename_text (GtkEntry   *entry,
			      const char *text)
{
	char *utf8_text;

	if (text == NULL)
		text = "";
	utf8_text = gnome_vfs_unescape_string_for_display (text);
	gtk_entry_set_text (entry, utf8_text);
	g_free (utf8_text);
}


char *
_gtk_entry_get_filename_text (GtkEntry   *entry)
{
	const char *utf8_text;
	char       *text;

	utf8_text = gtk_entry_get_text (entry);
	if (utf8_text == NULL)
		return NULL;

	text = escape_uri (utf8_text);

	return text;
}


void
_gtk_label_set_filename_text (GtkLabel   *label,
			      const char *text)
{
	char *utf8_text;

	if (text == NULL)
		text = "";
	utf8_text = gnome_vfs_unescape_string_for_display (text);
	gtk_label_set_text (label, utf8_text);
	g_free (utf8_text);
}


char *
_gtk_label_get_filename_text (GtkLabel   *label)
{
	const char *utf8_text;
	char       *text;

	utf8_text = gtk_label_get_text (label);
	if (utf8_text == NULL)
		return NULL;

	text = gnome_vfs_escape_string (utf8_text);

	return text;
}


gboolean
exec_command (const char *application,
	      GList      *file_list)
{
        GString *command;
        GList   *scan;
	GError  *err = NULL;
	gboolean error;

	command = g_string_new ("");
	g_string_append (command, application);
        for (scan = file_list; scan; scan = scan->next) {
		char *filename = scan->data;
		char *e_filename;

		g_string_append_c (command, ' ');
		e_filename = shell_escape (filename);
		g_string_append (command, e_filename);

		g_free (e_filename);
        }

	error = (! g_spawn_command_line_async (command->str, &err) || (err != NULL));
	if (error)
		_gtk_error_dialog_from_gerror_run (NULL, &err);
	g_string_free (command, TRUE);

	return ! error;
}


void
exec_shell_script (GtkWindow  *window,
		   const char *script,
		   GList      *file_list)
{
        GladeXML  *gui;
        GtkWidget *dialog;
        GtkWidget *label;
        GtkWidget *bar;
        GList     *scan;
        int        i, n;

	if ((script == NULL) || (file_list == NULL)) 
		return;

	/* Add a progress indicator */
        gui = glade_xml_new (GTHUMB_GLADEDIR "/gthumb_tools.glade",
                             NULL,
                             NULL);

        dialog = glade_xml_get_widget (gui, "hotkey_progress");
        label = glade_xml_get_widget (gui, "progress_info");
        bar = glade_xml_get_widget (gui, "progress_progressbar");

        n = g_list_length (file_list);

        gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (window));
        gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);

        gtk_widget_show (dialog);

        while (gtk_events_pending())
                gtk_main_iteration();

	/* If the %F code is present, all selected files are processed by
	   one script instance. Otherwise, each file is handled sequentially. */

	if (strstr (script, "%F")) {
		char *command = NULL;
		char *file_list_string;

		file_list_string = g_strdup (" ");

	        for (scan = file_list; scan; scan = scan->next) {
			char *filename;
			char *e_filename;
			char *new_file_list;
		
			if (is_local_file (scan->data))
                                filename = gnome_vfs_unescape_string_for_display (remove_scheme_from_uri (scan->data));
                        else
	                        filename = gnome_vfs_unescape_string_for_display (scan->data);

			e_filename = shell_escape (filename);

			new_file_list = g_strconcat (file_list_string, e_filename, " ", NULL);

			g_free (e_filename);
			g_free (file_list_string);
			file_list_string = g_strdup (new_file_list);

			g_free (new_file_list);
        	}
		command = _g_substitute_pattern (script, 'F', file_list_string);
		g_free (file_list_string);

		system (command);
		g_free (command);

		_gtk_label_set_filename_text (GTK_LABEL (label), script);
                gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (bar),
                                               (gdouble) 1.0);
                while (gtk_events_pending())
                	gtk_main_iteration();
	} else {
	        i = 0;
        	for (scan = file_list; scan; scan = scan->next) {
                	char *filename;
	                char *e_filename;
			char *name_wo_ext = NULL;
			char *extension = NULL;
			char *parent = NULL;
			char *command0 = NULL;
			char *command1 = NULL;
			char *command2 = NULL;
			char *command3 = NULL;

			if (is_local_file (scan->data))
		                filename = gnome_vfs_unescape_string_for_display (remove_scheme_from_uri (scan->data));
		        else
		                filename = gnome_vfs_unescape_string_for_display (scan->data);

			name_wo_ext = remove_extension_from_path (filename);
			extension = g_filename_to_utf8 (strrchr (filename, '.'), -1, 0, 0, 0);
			parent = remove_level_from_path (filename);
	
        	        e_filename = shell_escape (filename);
			command3 = _g_substitute_pattern (script, 'f', e_filename);
	                g_free (e_filename);

			e_filename = shell_escape (name_wo_ext);
			command2 = _g_substitute_pattern (command3, 'n', e_filename);
			g_free (e_filename);
			g_free (command3);
			e_filename = shell_escape (extension);
			command1 = _g_substitute_pattern (command2, 'e', e_filename);
			g_free (e_filename);
			g_free (command2);

	                e_filename = shell_escape (parent);
        	        command0 = _g_substitute_pattern (command1, 'p', e_filename);
                	g_free (e_filename);		
			g_free (command1);

			g_free (filename);
			g_free (name_wo_ext);
			g_free (extension);
			g_free (parent);

			_gtk_label_set_filename_text (GTK_LABEL (label), command0);
        	        gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (bar),
                	                               (gdouble) (i + 0.5) / n);

			system (command0);
			g_free (command0);

	                while (gtk_events_pending())
        	                gtk_main_iteration();

			i++;
	        }
	}

        gtk_widget_destroy (dialog);
        g_object_unref (gui);	
}


/* This function from gnome-panel/panel-util.c
 * (C) 1997, 1998, 1999, 2000 The Free Software Foundation
 * Copyright 2000 Helix Code, Inc.
 * Copyright 2000,2001 Eazel, Inc.
 * Copyright 2001 George Lebl
 * Copyright 2002 Sun Microsystems Inc.
 *
 * Authors: George Lebl
 *          Jacob Berkman
 *          Mark McLoughlin
 *
 * Modified by Paolo Bacchilega for the Quick Lounge applet
 */
static char*
panel_find_icon (GtkIconTheme  *icon_theme,
                 const char    *icon_name,
                 gint           size)
{
        char        *retval  = NULL;
	GtkIconInfo *icon_info = NULL;
        char        *icon_no_extension;
        char        *p;


        if (icon_name == NULL || strcmp (icon_name, "") == 0)
                return NULL;

        if (g_path_is_absolute (icon_name)) {
                if (g_file_test (icon_name, G_FILE_TEST_EXISTS)) {
                        return g_strdup (icon_name);
                } else {
                        char *basename;

			basename = g_path_get_basename (icon_name);
                        retval = panel_find_icon (icon_theme,
						  basename,
                                                  size);
                        g_free (basename);

                        return retval;
                }
        }

        /* This is needed because some .desktop files have an icon name *and*
         * an extension as icon */
        icon_no_extension = g_strdup (icon_name);
        p = strrchr (icon_no_extension, '.');
        if (p &&
            (strcmp (p, ".png") == 0 ||
             strcmp (p, ".xpm") == 0 ||
             strcmp (p, ".svg") == 0)) {
            *p = 0;
        }

        icon_info = gtk_icon_theme_lookup_icon (icon_theme,
						icon_no_extension,
						size,
						0);
	retval = g_strdup (gtk_icon_info_get_filename (icon_info));

        g_free (icon_no_extension);
	gtk_icon_info_free (icon_info);

        return retval;
}


GdkPixbuf *
create_pixbuf (GtkIconTheme  *icon_theme,
	       const char    *icon_name,
	       int            icon_size)
{
	char      *icon_path;
	GdkPixbuf *pixbuf;
	int        iw;
	int        ih;

	g_return_val_if_fail (icon_theme != NULL, NULL);

	icon_path = panel_find_icon (icon_theme, icon_name, icon_size);
	if (icon_path == NULL)
		return NULL;

	pixbuf = gth_pixbuf_new_from_uri (icon_path, NULL, 0, 0, NULL);
        g_free (icon_path);

        if (pixbuf == NULL)
                return NULL;

        iw = gdk_pixbuf_get_width (pixbuf);
        ih = gdk_pixbuf_get_height (pixbuf);

        if ((iw > icon_size) || (ih > icon_size)) {
                GdkPixbuf *scaled;
                gdouble    factor;
                gdouble    max = icon_size;
                int        new_w, new_h;

                factor = MIN (max / iw, max / ih);
                new_w  = MAX ((int) (iw * factor), 1);
                new_h = MAX ((int) (ih * factor), 1);

                scaled = gdk_pixbuf_scale_simple (pixbuf,
                                                  new_w,
                                                  new_h,
                                                  GDK_INTERP_BILINEAR);
                g_object_unref (pixbuf);
                pixbuf = scaled;
        }

	return pixbuf;
}


GtkWidget *
create_image (GtkIconTheme  *icon_theme,
	      const char    *icon_path,
	      int            icon_size)
{
	GtkWidget *icon = NULL;
	GdkPixbuf *pixbuf;

	pixbuf = create_pixbuf (icon_theme, icon_path, icon_size);

	if (pixbuf == NULL)
		return NULL;

	icon = gtk_image_new ();
	gtk_image_set_from_pixbuf (GTK_IMAGE (icon), pixbuf);
	g_object_unref (pixbuf);

	gtk_widget_show (icon);

	return icon;
}


int
_gtk_widget_get_height (GtkWidget *widget)
{
	int w, h;
	gdk_drawable_get_size (widget->window, &w, &h);
	return h;
}
