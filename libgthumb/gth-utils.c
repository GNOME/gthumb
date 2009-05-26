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
#include <ctype.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <libgnome/libgnome.h>

void
gthumb_display_help (GtkWindow  *window,
	             const char *section)
{
	GError *err = NULL;

	gnome_help_display ("gthumb", section, &err);

	if (err != NULL) {
		GtkWidget *dialog;

		dialog = gtk_message_dialog_new (window,
						 GTK_DIALOG_MODAL |
						 GTK_DIALOG_DESTROY_WITH_PARENT,
						 GTK_MESSAGE_ERROR,
						 GTK_BUTTONS_CLOSE,
						 _("Could not display help"));

		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
		                                          "%s",
							  err->message);

		g_signal_connect (G_OBJECT (dialog), "response",
				  G_CALLBACK (gtk_widget_destroy),
				  NULL);

		gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

		gtk_widget_show (dialog);

		g_error_free (err);
	}
}


/* based on glib/glib/gmarkup.c (Copyright 2000, 2003 Red Hat, Inc.)
 * This version does not escape ' and ''. Needed  because IE does not recognize
 * &apos; and &quot; */
static void
_append_escaped_text_for_html (GString     *str,
			       const gchar *text,
			       gssize       length)
{
	const gchar *p;
	const gchar *end;
	gunichar     ch;
	int          state = 0;

	p = text;
	end = text + length;

	while (p != end) {
		const gchar *next;
		next = g_utf8_next_char (p);
		ch = g_utf8_get_char (p);

		switch (state) {
		    case 1: /* escaped */
			if ((ch > 127) ||  !isprint((char)ch))
				g_string_append_printf (str, "\\&#%d;", ch);
			else
				g_string_append_unichar (str, ch);
			state = 0;
			break;

		    default: /* not escaped */
			switch (*p) {
			    case '\\':
				state = 1; /* next character is escaped */
				break;

			    case '&':
				g_string_append (str, "&amp;");
				break;

			    case '<':
				g_string_append (str, "&lt;");
				break;

			    case '>':
				g_string_append (str, "&gt;");
				break;

			    case '\n':
				g_string_append (str, "<br />");
				break;

			    default:
				if ((ch > 127) ||  !isprint((char)ch))
					g_string_append_printf (str, "&#%d;", ch);
				else
					g_string_append_unichar (str, ch);
				state = 0;
				break;
			}
		}

		p = next;
	}
}


char*
_g_escape_text_for_html (const gchar *text,
			 gssize       length)
{
	GString *str;

	g_return_val_if_fail (text != NULL, NULL);

	if (length < 0)
		length = strlen (text);

	/* prealloc at least as long as original text */
	str = g_string_sized_new (length);
	_append_escaped_text_for_html (str, text, length);

	return g_string_free (str, FALSE);
}
