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
#include "gth-image-saver-png.h"
#include "preferences.h"


G_DEFINE_TYPE (GthImageSaverPng, gth_image_saver_png, GTH_TYPE_IMAGE_SAVER)


struct _GthImageSaverPngPrivate {
	GtkBuilder *builder;
	GSettings  *settings;
};


static void
gth_image_saver_png_finalize (GObject *object)
{
	GthImageSaverPng *self = GTH_IMAGE_SAVER_PNG (object);

	_g_object_unref (self->priv->builder);
	_g_object_unref (self->priv->settings);
	G_OBJECT_CLASS (gth_image_saver_png_parent_class)->finalize (object);
}


static GtkWidget *
gth_image_saver_png_get_control (GthImageSaver *base)
{
	GthImageSaverPng *self = GTH_IMAGE_SAVER_PNG (base);

	if (self->priv->builder == NULL)
		self->priv->builder = _gtk_builder_new_from_file ("png-options.ui", "cairo_io");

	gtk_adjustment_set_value (GTK_ADJUSTMENT (_gtk_builder_get_widget (self->priv->builder, "png_compression_adjustment")),
				  g_settings_get_int (self->priv->settings, PREF_PNG_COMPRESSION_LEVEL));

	return _gtk_builder_get_widget (self->priv->builder, "png_options");
}


static void
gth_image_saver_png_save_options (GthImageSaver *base)
{
	GthImageSaverPng *self = GTH_IMAGE_SAVER_PNG (base);

	g_settings_set_int (self->priv->settings, PREF_PNG_COMPRESSION_LEVEL, (int) gtk_adjustment_get_value (GTK_ADJUSTMENT (_gtk_builder_get_widget (self->priv->builder, "png_compression_adjustment"))));
}


static gboolean
gth_image_saver_png_can_save (GthImageSaver *self,
			      const char    *mime_type)
{
	return g_content_type_equals (mime_type, "image/png");
}


static gboolean
gth_image_saver_png_save_image (GthImageSaver  *base,
				GthImage       *image,
				char          **buffer,
				gsize          *buffer_size,
				const char     *mime_type,
				GError        **error)
{
	GthImageSaverPng  *self = GTH_IMAGE_SAVER_PNG (base);
	GdkPixbuf         *pixbuf;
	char              *pixbuf_type;
	char             **option_keys;
	char             **option_values;
	int                i = -1;
	int                i_value;
	gboolean           result;

	pixbuf_type = get_pixbuf_type_from_mime_type (mime_type);

	option_keys = g_malloc (sizeof (char *) * 2);
	option_values = g_malloc (sizeof (char *) * 2);

	i++;
	i_value = g_settings_get_int (self->priv->settings, PREF_PNG_COMPRESSION_LEVEL);
	option_keys[i] = g_strdup ("compression");;
	option_values[i] = g_strdup_printf ("%d", i_value);

	i++;
	option_keys[i] = NULL;
	option_values[i] = NULL;

	/* FIXME: use libpng directly */
	pixbuf = gth_image_get_pixbuf (image);
	result = gdk_pixbuf_save_to_bufferv (pixbuf,
					     buffer,
					     buffer_size,
					     pixbuf_type,
					     option_keys,
					     option_values,
					     error);

	g_object_unref (pixbuf);
	g_strfreev (option_keys);
	g_strfreev (option_values);
	g_free (pixbuf_type);

	return result;
}


static void
gth_image_saver_png_class_init (GthImageSaverPngClass *klass)
{
	GObjectClass       *object_class;
	GthImageSaverClass *image_saver_class;

	g_type_class_add_private (klass, sizeof (GthImageSaverPngPrivate));

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = gth_image_saver_png_finalize;

	image_saver_class = GTH_IMAGE_SAVER_CLASS (klass);
	image_saver_class->id = "png";
	image_saver_class->display_name = _("PNG");
	image_saver_class->mime_type = "image/png";
	image_saver_class->extensions = "png";
	image_saver_class->get_default_ext = NULL;
	image_saver_class->get_control = gth_image_saver_png_get_control;
	image_saver_class->save_options = gth_image_saver_png_save_options;
	image_saver_class->can_save = gth_image_saver_png_can_save;
	image_saver_class->save_image = gth_image_saver_png_save_image;
}


static void
gth_image_saver_png_init (GthImageSaverPng *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_IMAGE_SAVER_PNG, GthImageSaverPngPrivate);
	self->priv->settings = g_settings_new (GTHUMB_IMAGE_SAVERS_PNG_SCHEMA);
}
