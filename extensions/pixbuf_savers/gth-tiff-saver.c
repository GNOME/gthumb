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
#ifdef HAVE_LIBTIFF
#include <tiffio.h>
#endif /* HAVE_LIBTIFF */
#include <glib/gi18n.h>
#include <gthumb.h>
#include "enum-types.h"
#include "gth-tiff-saver.h"
#include "preferences.h"


struct _GthTiffSaverPrivate {
	GtkBuilder *builder;
};


static gpointer parent_class = NULL;


static void
gth_tiff_saver_init (GthTiffSaver *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_TIFF_SAVER, GthTiffSaverPrivate);
}


static void
gth_tiff_saver_finalize (GObject *object)
{
	GthTiffSaver *self = GTH_TIFF_SAVER (object);

	_g_object_unref (self->priv->builder);
	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static GtkWidget *
gth_tiff_saver_get_control (GthPixbufSaver *base)
{
#ifdef HAVE_LIBTIFF

	GthTiffSaver       *self = GTH_TIFF_SAVER (base);
	GthTiffCompression  compression_type;

	if (self->priv->builder == NULL)
		self->priv->builder = _gtk_builder_new_from_file ("tiff-options.ui", "pixbuf_savers");

	compression_type = eel_gconf_get_enum (PREF_TIFF_COMPRESSION, GTH_TYPE_TIFF_COMPRESSION, GTH_TIFF_COMPRESSION_DEFLATE);
	switch (compression_type) {
	case GTH_TIFF_COMPRESSION_NONE:
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (_gtk_builder_get_widget (self->priv->builder, "tiff_comp_none_radiobutton")), TRUE);
		break;
	case GTH_TIFF_COMPRESSION_DEFLATE:
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (_gtk_builder_get_widget (self->priv->builder, "tiff_comp_deflate_radiobutton")), TRUE);
		break;
	case GTH_TIFF_COMPRESSION_JPEG:
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (_gtk_builder_get_widget (self->priv->builder, "tiff_comp_jpeg_radiobutton")), TRUE);
		break;
	}

	gtk_spin_button_set_value (GTK_SPIN_BUTTON (_gtk_builder_get_widget (self->priv->builder, "tiff_hdpi_spinbutton")), eel_gconf_get_integer (PREF_TIFF_HORIZONTAL_RES, 72));
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (_gtk_builder_get_widget (self->priv->builder, "tiff_vdpi_spinbutton")), eel_gconf_get_integer (PREF_TIFF_VERTICAL_RES, 72));

	return _gtk_builder_get_widget (self->priv->builder, "tiff_options");

#else /* ! HAVE_LIBTIFF */

	return GTH_PIXBUF_SAVER_CLASS (parent_class)->get_control (base);

#endif /* HAVE_LIBTIFF */
}


static void
gth_tiff_saver_save_options (GthPixbufSaver *base)
{
#ifdef HAVE_LIBTIFF

	GthTiffSaver     *self = GTH_TIFF_SAVER (base);
	GthTiffCompression  compression_type;

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (_gtk_builder_get_widget (self->priv->builder, "tiff_comp_none_radiobutton"))))
		compression_type = GTH_TIFF_COMPRESSION_NONE;
	else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (_gtk_builder_get_widget (self->priv->builder, "tiff_comp_deflate_radiobutton"))))
		compression_type = GTH_TIFF_COMPRESSION_DEFLATE;
	else
		compression_type = GTH_TIFF_COMPRESSION_JPEG;
	eel_gconf_set_enum (PREF_TIFF_COMPRESSION, GTH_TYPE_TIFF_COMPRESSION, compression_type);

	eel_gconf_set_integer (PREF_TIFF_HORIZONTAL_RES, (int) gtk_spin_button_get_value (GTK_SPIN_BUTTON (_gtk_builder_get_widget (self->priv->builder, "tiff_hdpi_spinbutton"))));
	eel_gconf_set_integer (PREF_TIFF_VERTICAL_RES, (int) gtk_spin_button_get_value (GTK_SPIN_BUTTON (_gtk_builder_get_widget (self->priv->builder, "tiff_vdpi_spinbutton"))));

#endif /* HAVE_LIBTIFF */
}


static gboolean
gth_tiff_saver_can_save (GthPixbufSaver *self,
			 const char     *mime_type)
{
#ifdef HAVE_LIBTIFF

	return g_content_type_equals (mime_type, "image/tiff");

#else /* ! HAVE_LIBTIFF */

	GSList          *formats;
	GSList          *scan;
	GdkPixbufFormat *tiff_format;

	if (! g_content_type_equals (mime_type, "image/tiff"))
		return FALSE;

	formats = gdk_pixbuf_get_formats ();
	tiff_format = NULL;
	for (scan = formats; (tiff_format == NULL) && (scan != NULL); scan = g_slist_next (scan)) {
		GdkPixbufFormat  *format = scan->data;
		char            **mime_types;
		int               i;

		mime_types = gdk_pixbuf_format_get_mime_types (format);
		for (i = 0; mime_types[i] != NULL; i++)
			if (g_content_type_equals (mime_types[i], "image/tiff"))
				break;

		if (mime_types[i] == NULL)
			continue;

		if (! gdk_pixbuf_format_is_writable (format))
			continue;

		tiff_format = format;
	}

	return tiff_format != NULL;

#endif /* HAVE_LIBTIFF */
}


#ifdef HAVE_LIBTIFF


/* -- gth_tiff_saver_save_pixbuf -- */


#define TILE_HEIGHT 40   /* FIXME */


static tsize_t
tiff_save_read (thandle_t handle, tdata_t buf, tsize_t size)
{
	return -1;
}


static tsize_t
tiff_save_write (thandle_t handle, tdata_t buf, tsize_t size)
{
        GthBufferData *buffer_data = (GthBufferData *)handle;

        gth_buffer_data_write (buffer_data, buf, size, NULL);
        return size;
}


static toff_t
tiff_save_seek (thandle_t handle, toff_t offset, int whence)
{
	GthBufferData *buffer_data = (GthBufferData *)handle;

	return gth_buffer_data_seek (buffer_data, offset, whence);
}


static int
tiff_save_close (thandle_t context)
{
        return 0;
}


static toff_t
tiff_save_size (thandle_t handle)
{
        return -1;
}


static gboolean
_gdk_pixbuf_save_as_tiff (GdkPixbuf   *pixbuf,
			  char       **buffer,
			  gsize       *buffer_size,
			  char       **keys,
			  char       **values,
			  GError     **error)
{
	GthBufferData *buffer_data;
	TIFF          *tif;
	int            cols, col, rows, row;
	glong          rowsperstrip;
	gushort        compression;
	int            alpha;
	gshort         predictor;
	gshort         photometric;
	gshort         samplesperpixel;
	gshort         bitspersample;
	int            rowstride;
	guchar        *pixels, *buf, *ptr;
	int            success;
	int            horizontal_dpi = 72, vertical_dpi = 72;
	gboolean       save_resolution = FALSE;

	compression = COMPRESSION_DEFLATE;

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
					compression = COMPRESSION_NONE;
				else if (strcmp (*viter, "pack bits") == 0)
					compression = COMPRESSION_PACKBITS;
				else if (strcmp (*viter, "lzw") == 0)
					compression = COMPRESSION_LZW;
				else if (strcmp (*viter, "deflate") == 0)
					compression = COMPRESSION_DEFLATE;
				else if (strcmp (*viter, "jpeg") == 0)
					compression = COMPRESSION_JPEG;
				else {
					g_set_error (error,
						     GDK_PIXBUF_ERROR,
						     GDK_PIXBUF_ERROR_BAD_OPTION,
						     "Unsupported compression type passed to the TIFF saver");
					return FALSE;
				}
			}
			else if (strcmp (*kiter, "vertical dpi") == 0) {
				char *endptr = NULL;
				vertical_dpi = strtol (*viter, &endptr, 10);
				save_resolution = TRUE;

				if (endptr == *viter) {
					g_set_error (error,
						     GDK_PIXBUF_ERROR,
						     GDK_PIXBUF_ERROR_BAD_OPTION,
						     "TIFF vertical dpi must be a value greater than 0; value '%s' could not be parsed.",
						     *viter);
					return FALSE;
				}

				if (vertical_dpi < 0) {
					g_set_error (error,
						     GDK_PIXBUF_ERROR,
						     GDK_PIXBUF_ERROR_BAD_OPTION,
						     "TIFF vertical dpi must be a value greater than 0; value '%d' is not allowed.",
						     vertical_dpi);
					return FALSE;
				}
			}
			else if (strcmp (*kiter, "horizontal dpi") == 0) {
				char *endptr = NULL;
				horizontal_dpi = strtol (*viter, &endptr, 10);
				save_resolution = TRUE;

				if (endptr == *viter) {
					g_set_error (error,
						     GDK_PIXBUF_ERROR,
						     GDK_PIXBUF_ERROR_BAD_OPTION,
						     "TIFF horizontal dpi must be a value greater than 0; value '%s' could not be parsed.",
						     *viter);
					return FALSE;
				}

				if (horizontal_dpi < 0) {
					g_set_error (error,
						     GDK_PIXBUF_ERROR,
						     GDK_PIXBUF_ERROR_BAD_OPTION,
						     "TIFF horizontal dpi must be a value greater than 0; value '%d' is not allowed.",
						     horizontal_dpi);
					return FALSE;
				}
			}
			else {
				g_warning ("Bad option name '%s' passed to the TIFF saver", *kiter);
				return FALSE;
			}

			++kiter;
			++viter;
		}
	}

	predictor    = 0;
	rowsperstrip = TILE_HEIGHT;

	buffer_data = gth_buffer_data_new ();
	tif = TIFFClientOpen ("gth-tiff-writer", "w", buffer_data,
	                      tiff_save_read, tiff_save_write,
	                      tiff_save_seek, tiff_save_close,
	                      tiff_save_size,
	                      NULL, NULL);
	if (tif == NULL) {
		g_set_error_literal (error,
				     GDK_PIXBUF_ERROR,
				     GDK_PIXBUF_ERROR_INSUFFICIENT_MEMORY,
				     "Couldn't allocate memory for writing TIFF file");
		return FALSE;
	}

	cols      = gdk_pixbuf_get_width (pixbuf);
	rows      = gdk_pixbuf_get_height (pixbuf);
	alpha     = gdk_pixbuf_get_has_alpha (pixbuf);
	pixels    = gdk_pixbuf_get_pixels (pixbuf);
	rowstride = gdk_pixbuf_get_rowstride (pixbuf);

	predictor       = 2;
	bitspersample   = 8;
	photometric     = PHOTOMETRIC_RGB;

	if (alpha)
		samplesperpixel = 4;
	else
		samplesperpixel = 3;

	/* Set TIFF parameters. */

	TIFFSetField (tif, TIFFTAG_SUBFILETYPE,   0);
	TIFFSetField (tif, TIFFTAG_IMAGEWIDTH,    cols);
	TIFFSetField (tif, TIFFTAG_IMAGELENGTH,   rows);
	TIFFSetField (tif, TIFFTAG_BITSPERSAMPLE, bitspersample);
	TIFFSetField (tif, TIFFTAG_ORIENTATION,   ORIENTATION_TOPLEFT);
	TIFFSetField (tif, TIFFTAG_COMPRESSION,   compression);

	if ((compression == COMPRESSION_LZW || compression == COMPRESSION_DEFLATE)
	    && (predictor != 0))
	{
		TIFFSetField (tif, TIFFTAG_PREDICTOR, predictor);
	}

	if (alpha) {
		gushort extra_samples[1];

		extra_samples [0] = EXTRASAMPLE_ASSOCALPHA;
		TIFFSetField (tif, TIFFTAG_EXTRASAMPLES, 1, extra_samples);
	}

	TIFFSetField (tif, TIFFTAG_PHOTOMETRIC,     photometric);
	/*TIFFSetField (tif, TIFFTAG_DOCUMENTNAME,    filename);*/
	TIFFSetField (tif, TIFFTAG_SAMPLESPERPIXEL, samplesperpixel);
	TIFFSetField (tif, TIFFTAG_ROWSPERSTRIP,    rowsperstrip);
	TIFFSetField (tif, TIFFTAG_PLANARCONFIG,    PLANARCONFIG_CONTIG);

	if (save_resolution) {
		TIFFSetField (tif, TIFFTAG_XRESOLUTION, (double) horizontal_dpi);
		TIFFSetField (tif, TIFFTAG_YRESOLUTION, (double) vertical_dpi);
		TIFFSetField (tif, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH);
	}

	/* allocate a small buffer to convert image data */
	buf = g_try_malloc (cols * samplesperpixel * sizeof (guchar));
	if (! buf) {
		g_set_error_literal (error,
				     GDK_PIXBUF_ERROR,
				     GDK_PIXBUF_ERROR_INSUFFICIENT_MEMORY,
				     "Couldn't allocate memory for writing TIFF file");
		return FALSE;
	}

	ptr = pixels;

	/* Now write the TIFF data. */
	for (row = 0; row < rows; row++) {
		/* convert scanline from ARGB to RGB packed */
		for (col = 0; col < cols; col++)
			memcpy (&(buf[col * 3]), &(ptr[col * samplesperpixel]), 3);

		success = TIFFWriteScanline (tif, buf, row, 0) >= 0;

		if (! success) {
			g_set_error (error,
				     GDK_PIXBUF_ERROR,
				     GDK_PIXBUF_ERROR_FAILED,
				     "TIFF Failed a scanline write on row %d",
				     row);
			return FALSE;
		}

		ptr += rowstride;
	}

	TIFFFlushData (tif);
	TIFFClose (tif);

	g_free (buf);

	gth_buffer_data_get (buffer_data, buffer, buffer_size);
	gth_buffer_data_free (buffer_data, FALSE);

	return TRUE;
}


#endif /* HAVE_LIBTIFF */


static gboolean
gth_tiff_saver_save_pixbuf (GthPixbufSaver  *self,
			    GdkPixbuf       *pixbuf,
			    char           **buffer,
			    gsize           *buffer_size,
			    const char      *mime_type,
			    GError         **error)
{
#ifdef HAVE_LIBTIFF

	char     **option_keys;
	char     **option_values;
	int        i = -1;
	int        i_value;
	gboolean   result;

	option_keys = g_malloc (sizeof (char *) * 4);
	option_values = g_malloc (sizeof (char *) * 4);

	i++;
	option_keys[i] = g_strdup ("compression");;
	option_values[i] = eel_gconf_get_string (PREF_TIFF_COMPRESSION, "deflate");

	i++;
	i_value = eel_gconf_get_integer (PREF_TIFF_VERTICAL_RES, 72);
	option_keys[i] = g_strdup ("vertical dpi");;
	option_values[i] = g_strdup_printf ("%d", i_value);

	i++;
	i_value = eel_gconf_get_integer (PREF_TIFF_HORIZONTAL_RES, 72);
	option_keys[i] = g_strdup ("horizontal dpi");;
	option_values[i] = g_strdup_printf ("%d", i_value);

	i++;
	option_keys[i] = NULL;
	option_values[i] = NULL;

	result = _gdk_pixbuf_save_as_tiff (pixbuf,
					   buffer,
					   buffer_size,
					   option_keys,
					   option_values,
					   error);

	g_strfreev (option_keys);
	g_strfreev (option_values);

#else /* ! HAVE_LIBTIFF */

	char     *pixbuf_type;
	gboolean  result;

	pixbuf_type = get_pixbuf_type_from_mime_type (mime_type);
	result = gdk_pixbuf_save_to_bufferv (pixbuf,
					     buffer,
					     buffer_size,
					     pixbuf_type,
					     NULL,
					     NULL,
					     error);

	g_free (pixbuf_type);

#endif /* HAVE_LIBTIFF */

	return result;
}


static void
gth_tiff_saver_class_init (GthTiffSaverClass *klass)
{
	GObjectClass        *object_class;
	GthPixbufSaverClass *pixbuf_saver_class;

	parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GthTiffSaverPrivate));

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = gth_tiff_saver_finalize;

	pixbuf_saver_class = GTH_PIXBUF_SAVER_CLASS (klass);
	pixbuf_saver_class->id = "tiff";
	pixbuf_saver_class->display_name = _("TIFF");
	pixbuf_saver_class->mime_type = "image/tiff";
	pixbuf_saver_class->default_ext = "tiff";
	pixbuf_saver_class->get_control = gth_tiff_saver_get_control;
	pixbuf_saver_class->save_options = gth_tiff_saver_save_options;
	pixbuf_saver_class->can_save = gth_tiff_saver_can_save;
	pixbuf_saver_class->save_pixbuf = gth_tiff_saver_save_pixbuf;
}


GType
gth_tiff_saver_get_type (void)
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (GthTiffSaverClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_tiff_saver_class_init,
			NULL,
			NULL,
			sizeof (GthTiffSaver),
			0,
			(GInstanceInitFunc) gth_tiff_saver_init
		};

		type = g_type_register_static (GTH_TYPE_PIXBUF_SAVER,
					       "GthTiffSaver",
					       &type_info,
					       0);
	}

	return type;
}
