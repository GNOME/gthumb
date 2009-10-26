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
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include <glib/gi18n.h>
#include <gthumb.h>
#include "gth-tga-saver.h"
#include "preferences.h"


struct _GthTgaSaverPrivate {
	GtkBuilder *builder;
};


static gpointer parent_class = NULL;


static void
gth_tga_saver_init (GthTgaSaver *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_TGA_SAVER, GthTgaSaverPrivate);
}


static void
gth_tga_saver_finalize (GObject *object)
{
	GthTgaSaver *self = GTH_TGA_SAVER (object);

	_g_object_unref (self->priv->builder);
	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static GtkWidget *
gth_tga_saver_get_control (GthPixbufSaver *base)
{
	GthTgaSaver *self = GTH_TGA_SAVER (base);

	if (self->priv->builder == NULL)
		self->priv->builder = _gtk_builder_new_from_file ("tga-options.ui", "pixbuf_savers");

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (_gtk_builder_get_widget (self->priv->builder, "tga_rle_compression_checkbutton")),
				      eel_gconf_get_boolean (PREF_TGA_RLE_COMPRESSION, TRUE));

	return _gtk_builder_get_widget (self->priv->builder, "tga_options");
}


static void
gth_tga_saver_save_options (GthPixbufSaver *base)
{
	GthTgaSaver *self = GTH_TGA_SAVER (base);

	eel_gconf_set_boolean (PREF_TGA_RLE_COMPRESSION, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (_gtk_builder_get_widget (self->priv->builder, "tga_rle_compression_checkbutton"))));
}


static gboolean
gth_tga_saver_can_save (GthPixbufSaver *self,
			const char     *mime_type)
{
	return g_content_type_equals (mime_type, "image/tga") || g_content_type_equals (mime_type, "image/x-tga");
}


/* -- _gdk_pixbuf_save_as_tga -- */


/* TRUEVISION-XFILE magic signature string */
static guchar magic[18] = {
	0x54, 0x52, 0x55, 0x45, 0x56, 0x49, 0x53, 0x49, 0x4f,
	0x4e, 0x2d, 0x58, 0x46, 0x49, 0x4c, 0x45, 0x2e, 0x0
};


static gboolean
rle_write (GthBufferData  *buffer_data,
	   guchar         *buffer,
	   guint           width,
	   guint           bytes,
	   GError        **error)
{
	int     repeat = 0;
	int     direct = 0;
	guchar *from = buffer;
	guint   x;

	for (x = 1; x < width; ++x) {
		if (memcmp (buffer, buffer + bytes, bytes)) {
			/* next pixel is different */
			if (repeat) {
				gth_buffer_data_putc (buffer_data, 128 + repeat, error);
				gth_buffer_data_write (buffer_data, from, bytes, error);
				from = buffer + bytes; /* point to first different pixel */
				repeat = 0;
				direct = 0;
			}
			else
				direct += 1;
		}
		else {
			/* next pixel is the same */
			if (direct) {
				gth_buffer_data_putc (buffer_data, direct - 1, error);
				gth_buffer_data_write (buffer_data, from, bytes * direct, error);
				from = buffer; /* point to first identical pixel */
				direct = 0;
				repeat = 1;
			}
			else
				repeat += 1;
		}

		if (repeat == 128) {
			gth_buffer_data_putc (buffer_data, 255, error);
			gth_buffer_data_write (buffer_data, from, bytes, error);
			from = buffer + bytes;
			direct = 0;
			repeat = 0;
		}
		else if (direct == 128) {
			gth_buffer_data_putc (buffer_data, 127, error);
			gth_buffer_data_write (buffer_data, from, bytes * direct, error);
			from = buffer + bytes;
			direct = 0;
			repeat = 0;
		}

		buffer += bytes;
	}

	if (repeat > 0) {
		gth_buffer_data_putc (buffer_data, 128 + repeat, error);
		gth_buffer_data_write (buffer_data, from, bytes, error);
	}
	else {
		gth_buffer_data_putc (buffer_data, direct, error);
		gth_buffer_data_write (buffer_data, from, bytes * (direct + 1), error);
	}

	return TRUE;
}


static void
bgr2rgb (guchar *dest,
	 guchar *src,
	 guint   width,
	 guint   bytes,
	 guint   alpha)
{
	guint x;

	if (alpha)
		for (x = 0; x < width; x++) {
			*(dest++) = src[2];
			*(dest++) = src[1];
			*(dest++) = src[0];
			*(dest++) = src[3];

			src += bytes;
		}
	else
		for (x = 0; x < width; x++) {
			*(dest++) = src[2];
			*(dest++) = src[1];
			*(dest++) = src[0];

			src += bytes;
		}
}


static gboolean
_gdk_pixbuf_save_as_tga (GdkPixbuf   *pixbuf,
			 char       **buffer,
			 gsize       *buffer_size,
			 char       **keys,
			 char       **values,
			 GError     **error)
{
	GthBufferData *buffer_data;
	int            out_bpp = 0;
	int            row;
	guchar         header[18];
	guchar         footer[26];
	gboolean       rle_compression;
	gboolean       alpha;
	guchar        *pixels, *ptr, *buf;
	int            width, height;
	int            rowstride;

	rle_compression = TRUE;

	if (keys && *keys) {
		char **kiter = keys;
		char **viter = values;

		while (*kiter) {
			if (strcmp (*kiter, "compression") == 0) {
				if (*viter == NULL) {
					g_set_error (error,
						     GDK_PIXBUF_ERROR,
						     GDK_PIXBUF_ERROR_BAD_OPTION,
						     "Must specify a compression type");
					return FALSE;
				}

				if (strcmp (*viter, "none") == 0)
					rle_compression = FALSE;

				else if (strcmp (*viter, "rle") == 0)
					rle_compression = TRUE;

				else {
					g_set_error (error,
						     GDK_PIXBUF_ERROR,
						     GDK_PIXBUF_ERROR_BAD_OPTION,
						     "Unsupported compression type passed to the TGA saver");
					return FALSE;
				}
			}
			else {
				g_warning ("Bad option name '%s' passed to the TGA saver", *kiter);
				return FALSE;
			}

			++kiter;
			++viter;
		}
	}

	width     = gdk_pixbuf_get_width (pixbuf);
	height    = gdk_pixbuf_get_height (pixbuf);
	alpha     = gdk_pixbuf_get_has_alpha (pixbuf);
	pixels    = gdk_pixbuf_get_pixels (pixbuf);
	rowstride = gdk_pixbuf_get_rowstride (pixbuf);

	buffer_data = gth_buffer_data_new ();

	/* write the header */

	header[0] = 0; /* No image identifier / description */
	header[1] = 0;
	header[2] = rle_compression ? 10 : 2;
	header[3] = header[4] = header[5] = header[6] = header[7] = 0;
	header[8]  = header[9]  = 0; /* xorigin */
	header[10] = header[11] = 0; /* yorigin */
	header[12] = width % 256;
	header[13] = width / 256;
	header[14] = height % 256;
	header[15] = height / 256;
	if (alpha) {
		out_bpp = 4;
		header[16] = 32; /* bpp */
		header[17] = 0x28; /* alpha + orientation */
	}
	else {
		out_bpp = 3;
		header[16] = 24; /* bpp */
		header[17] = 0x20; /* alpha + orientation */
	}
	gth_buffer_data_write (buffer_data, header, sizeof (header), error);

	/* allocate a small buffer to convert image data */
	buf = g_try_malloc (width * out_bpp * sizeof (guchar));
	if (! buf) {
		g_set_error_literal (error,
				     GDK_PIXBUF_ERROR,
				     GDK_PIXBUF_ERROR_INSUFFICIENT_MEMORY,
				     _("Insufficient memory"));
		return FALSE;
	}

	ptr = pixels;
	for (row = 0; row < height; ++row) {
		bgr2rgb (buf, ptr, width, out_bpp, alpha);

		if (rle_compression)
			rle_write (buffer_data, buf, width, out_bpp, error);
		else
			gth_buffer_data_write (buffer_data, buf, width * out_bpp, error);

		ptr += rowstride;
	}

	g_free (buf);

	/* write the footer  */

	memset (footer, 0, 8); /* No extensions, no developer directory */
	memcpy (footer + 8, magic, sizeof (magic)); /* magic signature */
	gth_buffer_data_write (buffer_data, footer, sizeof (footer), error);

	gth_buffer_data_get (buffer_data, buffer, buffer_size);
	gth_buffer_data_free (buffer_data, FALSE);

	return TRUE;
}


static gboolean
gth_tga_saver_save_pixbuf (GthPixbufSaver  *self,
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
	gboolean   result;

	pixbuf_type = get_pixbuf_type_from_mime_type (mime_type);

	option_keys = g_malloc (sizeof (char *) * 2);
	option_values = g_malloc (sizeof (char *) * 2);

	i++;
	option_keys[i] = g_strdup ("compression");
	option_values[i] = g_strdup (eel_gconf_get_boolean (PREF_TGA_RLE_COMPRESSION, TRUE) ? "rle" : "none");

	i++;
	option_keys[i] = NULL;
	option_values[i] = NULL;

	result = _gdk_pixbuf_save_as_tga (pixbuf,
					  buffer,
					  buffer_size,
					  option_keys,
					  option_values,
					  error);

	g_strfreev (option_keys);
	g_strfreev (option_values);
	g_free (pixbuf_type);

	return result;
}


static void
gth_tga_saver_class_init (GthTgaSaverClass *klass)
{
	GObjectClass        *object_class;
	GthPixbufSaverClass *pixbuf_saver_class;

	parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GthTgaSaverPrivate));

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = gth_tga_saver_finalize;

	pixbuf_saver_class = GTH_PIXBUF_SAVER_CLASS (klass);
	pixbuf_saver_class->id = "tga";
	pixbuf_saver_class->display_name = _("TGA");
	pixbuf_saver_class->mime_type = "image/x-tga";
	pixbuf_saver_class->extensions = "tga";
	pixbuf_saver_class->default_ext = "tga";
	pixbuf_saver_class->get_control = gth_tga_saver_get_control;
	pixbuf_saver_class->save_options = gth_tga_saver_save_options;
	pixbuf_saver_class->can_save = gth_tga_saver_can_save;
	pixbuf_saver_class->save_pixbuf = gth_tga_saver_save_pixbuf;
}


GType
gth_tga_saver_get_type (void)
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (GthTgaSaverClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_tga_saver_class_init,
			NULL,
			NULL,
			sizeof (GthTgaSaver),
			0,
			(GInstanceInitFunc) gth_tga_saver_init
		};

		type = g_type_register_static (GTH_TYPE_PIXBUF_SAVER,
					       "GthTgaSaver",
					       &type_info,
					       0);
	}

	return type;
}
