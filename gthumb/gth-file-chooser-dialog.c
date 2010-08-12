/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2009 The Free Software Foundation, Inc.
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
#include <stdlib.h>
#include <gtk/gtk.h>
#include "eggfileformatchooser.h"
#include "glib-utils.h"
#include "gth-file-chooser-dialog.h"
#include "gth-main.h"


typedef struct {
	const char *type;
	const char *extensions;
	const char *default_ext;
} Format;


static gpointer parent_class = NULL;


struct _GthFileChooserDialogPrivate {
	GList                *supported_formats;
	EggFileFormatChooser *format_chooser;
};


static void
gth_file_chooser_dialog_finalize (GObject *object)
{
	GthFileChooserDialog *self;

	self = GTH_FILE_CHOOSER_DIALOG (object);

	g_list_foreach (self->priv->supported_formats, (GFunc) g_free, NULL);
	g_list_free (self->priv->supported_formats);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gth_file_chooser_dialog_class_init (GthFileChooserDialogClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	g_type_class_add_private (class, sizeof (GthFileChooserDialogPrivate));

	object_class = (GObjectClass*) class;
	object_class->finalize = gth_file_chooser_dialog_finalize;
}


static void
gth_file_chooser_dialog_init (GthFileChooserDialog *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_FILE_CHOOSER_DIALOG, GthFileChooserDialogPrivate);
	self->priv->supported_formats = NULL;
}


GType
gth_file_chooser_dialog_get_type (void)
{
        static GType type = 0;

        if (! type) {
                GTypeInfo type_info = {
			sizeof (GthFileChooserDialogClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_file_chooser_dialog_class_init,
			NULL,
			NULL,
			sizeof (GthFileChooserDialog),
			0,
			(GInstanceInitFunc) gth_file_chooser_dialog_init
		};

		type = g_type_register_static (GTK_TYPE_FILE_CHOOSER_DIALOG,
					       "GthFileChooserDialog",
					       &type_info,
					       0);
	}

        return type;
}


static void
format_chooser_selection_changed_cb (EggFileFormatChooser *format_chooser,
				     GthFileChooserDialog *self)
{
	char   *filename;
	int     n_format;
	Format *format;
	char   *basename;
	char   *basename_noext;
	char   *new_basename;

	filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (self));
	if (filename == NULL)
		return;

	n_format = egg_file_format_chooser_get_format (EGG_FILE_FORMAT_CHOOSER (self->priv->format_chooser), filename);

	if ((n_format < 1) || (n_format > g_list_length (self->priv->supported_formats))) {
		g_free (filename);
		return;
	}

	format = g_list_nth_data (self->priv->supported_formats, n_format - 1);
	basename = g_path_get_basename (filename);
	basename_noext = _g_uri_remove_extension (basename);
	new_basename = g_strconcat (basename_noext, ".", format->default_ext, NULL);
	gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (self), new_basename);

	g_free (new_basename);
	g_free (basename_noext);
	g_free (basename);
	g_free (filename);
}


static char *
get_icon_name_for_type (const char *mime_type)
{
	char *name = NULL;

	if (mime_type != NULL) {
		char *s;

		name = g_strconcat ("gnome-mime-", mime_type, NULL);
		for (s = name; *s; ++s)
			if (! g_ascii_isalpha (*s))
				*s = '-';
	}

	if ((name == NULL) || ! gtk_icon_theme_has_icon (gtk_icon_theme_get_default (), name)) {
		g_free (name);
		name = g_strdup ("image-x-generic");
	}

	return name;
}


static void
gth_file_chooser_dialog_construct (GthFileChooserDialog *self,
				   const char           *title,
			           GtkWindow            *parent,
				   const char           *allowed_savers)
{
	GArray *savers;
	int     i;
	GList  *scan;

	if (title != NULL)
    		gtk_window_set_title (GTK_WINDOW (self), title);
  	if (parent != NULL)
    		gtk_window_set_transient_for (GTK_WINDOW (self), parent);

  	gtk_file_chooser_set_action (GTK_FILE_CHOOSER (self), GTK_FILE_CHOOSER_ACTION_SAVE);
  	gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (self), FALSE);
	gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (self), TRUE);
	gtk_dialog_set_default_response (GTK_DIALOG (self), GTK_RESPONSE_ACCEPT);

	gtk_dialog_add_button (GTK_DIALOG (self), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
	gtk_dialog_add_button (GTK_DIALOG (self), GTK_STOCK_SAVE, GTK_RESPONSE_OK);

	/**/

	savers = gth_main_get_type_set (allowed_savers /*"pixbuf-saver"*/);
	for (i = 0; (savers != NULL) && (i < savers->len); i++) {
		GthPixbufSaver *saver;
		Format         *format;

		saver = g_object_new (g_array_index (savers, GType, i), NULL);
		format = g_new (Format, 1);
		format->type = get_static_string (gth_pixbuf_saver_get_mime_type (saver));
		format->extensions = get_static_string (gth_pixbuf_saver_get_extensions (saver));
		format->default_ext = get_static_string (gth_pixbuf_saver_get_default_ext (saver));
		self->priv->supported_formats = g_list_prepend (self->priv->supported_formats, format);

		g_object_unref (saver);
	}

	self->priv->format_chooser = (EggFileFormatChooser *) egg_file_format_chooser_new ();
	for (scan = self->priv->supported_formats; scan != NULL; scan = scan->next) {
		Format  *format = scan->data;
		char    *icon_name;
		char   **extensions;

		icon_name = get_icon_name_for_type (format->type);
		extensions = g_strsplit (format->extensions, " ", -1);
		egg_file_format_chooser_add_format (self->priv->format_chooser,
						    0,
						    g_content_type_get_description (format->type),
						    icon_name,
						    extensions[0],
						    extensions[1],
						    extensions[2],
						    NULL);

		g_strfreev (extensions);
		g_free (icon_name);
	}

	gtk_file_chooser_set_extra_widget (GTK_FILE_CHOOSER (self), GTK_WIDGET (self->priv->format_chooser));

	g_signal_connect (G_OBJECT (self->priv->format_chooser),
			  "selection-changed",
			  G_CALLBACK (format_chooser_selection_changed_cb),
			  self);
}


GtkWidget *
gth_file_chooser_dialog_new (const char *title,
			     GtkWindow  *parent,
			     const char *allowed_savers)
{
	GthFileChooserDialog *self;

	self = g_object_new (GTH_TYPE_FILE_CHOOSER_DIALOG, NULL);
	gth_file_chooser_dialog_construct (self, title, parent, allowed_savers);

	return (GtkWidget *) self;
}


gboolean
gth_file_chooser_dialog_get_file (GthFileChooserDialog  *self,
				  GFile                **file,
				  const char           **mime_type)
{
	char   *filename;
	int     n_format;
	Format *format;

	filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (self));
	n_format = egg_file_format_chooser_get_format (EGG_FILE_FORMAT_CHOOSER (self->priv->format_chooser), filename);
	g_free (filename);

	if ((n_format < 1) || (n_format > g_list_length (self->priv->supported_formats)))
		return FALSE;

	format = g_list_nth_data (self->priv->supported_formats, n_format - 1);
	if (file != NULL)
		*file = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (self));
	if (mime_type != NULL)
		*mime_type = format->type;

	return TRUE;
}
