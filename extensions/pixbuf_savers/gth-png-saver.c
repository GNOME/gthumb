/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2009 Free Software Foundation, Inc.
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
#include <glib/gi18n.h>
#include <gthumb.h>
#include "gth-png-saver.h"
#include "preferences.h"


struct _GthPngSaverPrivate {
	GtkBuilder *builder;
};


static gpointer parent_class = NULL;


static void
gth_png_saver_init (GthPngSaver *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_PNG_SAVER, GthPngSaverPrivate);
}


static void
gth_png_saver_finalize (GObject *object)
{
	GthPngSaver *self = GTH_PNG_SAVER (object);

	_g_object_unref (self->priv->builder);
	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static GtkWidget *
gth_png_saver_get_control (GthPixbufSaver *base)
{
	GthPngSaver *self = GTH_PNG_SAVER (base);

	if (self->priv->builder == NULL)
		self->priv->builder = _gtk_builder_new_from_file ("png-options.ui", "pixbuf_savers");

	gtk_adjustment_set_value (GTK_ADJUSTMENT (_gtk_builder_get_widget (self->priv->builder, "png_compression_adjustment")),
				  eel_gconf_get_integer (PREF_PNG_COMPRESSION_LEVEL, 6));

	return _gtk_builder_get_widget (self->priv->builder, "png_options");
}


static void
gth_png_saver_save_options (GthPixbufSaver *base)
{
	GthPngSaver *self = GTH_PNG_SAVER (base);

	eel_gconf_set_integer (PREF_PNG_COMPRESSION_LEVEL, (int) gtk_adjustment_get_value (GTK_ADJUSTMENT (_gtk_builder_get_widget (self->priv->builder, "png_compression_adjustment"))));
}


static gboolean
gth_png_saver_can_save (GthPixbufSaver *self,
			const char     *mime_type)
{
	return g_content_type_equals (mime_type, "image/png");
}


static gboolean
gth_png_saver_save_pixbuf (GthPixbufSaver  *self,
			   GdkPixbuf       *pixbuf,
			   char           **buffer,
			   gsize           *buffer_size,
			   const char      *mime_type,
			   GError         **error)
{
	char      *pixbuf_type;
	char     **option_keys;
	char     **option_values;
	int        i = -1;
	int        i_value;
	gboolean   result;

	pixbuf_type = get_pixbuf_type_from_mime_type (mime_type);

	option_keys = g_malloc (sizeof (char *) * 2);
	option_values = g_malloc (sizeof (char *) * 2);

	i++;
	i_value = eel_gconf_get_integer (PREF_PNG_COMPRESSION_LEVEL, 6);
	option_keys[i] = g_strdup ("compression");;
	option_values[i] = g_strdup_printf ("%d", i_value);

	i++;
	option_keys[i] = NULL;
	option_values[i] = NULL;

	result = gdk_pixbuf_save_to_bufferv (pixbuf,
					     buffer,
					     buffer_size,
					     pixbuf_type,
					     option_keys,
					     option_values,
					     error);

	g_strfreev (option_keys);
	g_strfreev (option_values);
	g_free (pixbuf_type);

	return result;
}


static void
gth_png_saver_class_init (GthPngSaverClass *klass)
{
	GObjectClass        *object_class;
	GthPixbufSaverClass *pixbuf_saver_class;

	parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GthPngSaverPrivate));

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = gth_png_saver_finalize;

	pixbuf_saver_class = GTH_PIXBUF_SAVER_CLASS (klass);
	pixbuf_saver_class->id = "png";
	pixbuf_saver_class->display_name = _("PNG");
	pixbuf_saver_class->mime_type = "image/png";
	pixbuf_saver_class->extensions = "png";
	pixbuf_saver_class->default_ext = "png";
	pixbuf_saver_class->get_control = gth_png_saver_get_control;
	pixbuf_saver_class->save_options = gth_png_saver_save_options;
	pixbuf_saver_class->can_save = gth_png_saver_can_save;
	pixbuf_saver_class->save_pixbuf = gth_png_saver_save_pixbuf;
}


GType
gth_png_saver_get_type (void)
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (GthPngSaverClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_png_saver_class_init,
			NULL,
			NULL,
			sizeof (GthPngSaver),
			0,
			(GInstanceInitFunc) gth_png_saver_init
		};

		type = g_type_register_static (GTH_TYPE_PIXBUF_SAVER,
					       "GthPngSaver",
					       &type_info,
					       0);
	}

	return type;
}
