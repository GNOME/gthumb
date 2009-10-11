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
#ifdef HAVE_LIBJPEG
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>
#include <jpeglib.h>
#include <extensions/jpeg_utils/jmemorydest.h>
#endif /* HAVE_LIBJPEG */
#include <glib/gi18n.h>
#include <gthumb.h>
#include "gth-jpeg-saver.h"
#include "preferences.h"


struct _GthJpegSaverPrivate {
	GtkBuilder *builder;
};


static gpointer parent_class = NULL;


static void
gth_jpeg_saver_init (GthJpegSaver *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_JPEG_SAVER, GthJpegSaverPrivate);
}


static void
gth_jpeg_saver_finalize (GObject *object)
{
	GthJpegSaver *self = GTH_JPEG_SAVER (object);

	_g_object_unref (self->priv->builder);
	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static GtkWidget *
gth_jpeg_saver_get_control (GthPixbufSaver *base)
{
	GthJpegSaver *self = GTH_JPEG_SAVER (base);

	if (self->priv->builder == NULL)
		self->priv->builder = _gtk_builder_new_from_file ("jpeg-options.ui", "pixbuf_savers");

	gtk_adjustment_set_value (GTK_ADJUSTMENT (_gtk_builder_get_widget (self->priv->builder, "jpeg_quality_adjustment")),
				  eel_gconf_get_integer (PREF_JPEG_QUALITY, 85));
	gtk_adjustment_set_value (GTK_ADJUSTMENT (_gtk_builder_get_widget (self->priv->builder, "jpeg_smooth_adjustment")),
				  eel_gconf_get_integer (PREF_JPEG_SMOOTHING, 0));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (_gtk_builder_get_widget (self->priv->builder, "jpeg_optimize_checkbutton")),
				      eel_gconf_get_boolean (PREF_JPEG_OPTIMIZE, TRUE));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (_gtk_builder_get_widget (self->priv->builder, "jpeg_progressive_checkbutton")),
				      eel_gconf_get_boolean (PREF_JPEG_PROGRESSIVE, FALSE));

	return _gtk_builder_get_widget (self->priv->builder, "jpeg_options");
}


static void
gth_jpeg_saver_save_options (GthPixbufSaver *base)
{
	GthJpegSaver *self = GTH_JPEG_SAVER (base);

	eel_gconf_set_integer (PREF_JPEG_QUALITY, (int) gtk_adjustment_get_value (GTK_ADJUSTMENT (_gtk_builder_get_widget (self->priv->builder, "jpeg_quality_adjustment"))));
	eel_gconf_set_integer (PREF_JPEG_SMOOTHING, (int) gtk_adjustment_get_value (GTK_ADJUSTMENT (_gtk_builder_get_widget (self->priv->builder, "jpeg_smooth_adjustment"))));
	eel_gconf_set_boolean (PREF_JPEG_OPTIMIZE, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (_gtk_builder_get_widget (self->priv->builder, "jpeg_optimize_checkbutton"))));
	eel_gconf_set_boolean (PREF_JPEG_PROGRESSIVE, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (_gtk_builder_get_widget (self->priv->builder, "jpeg_progressive_checkbutton"))));
}


static gboolean
gth_jpeg_saver_can_save (GthPixbufSaver *self,
			 const char     *mime_type)
{
#ifdef HAVE_LIBJPEG

	return g_content_type_equals (mime_type, "image/jpeg");

#else /* ! HAVE_LIBJPEG */

	GSList          *formats;
	GSList          *scan;
	GdkPixbufFormat *jpeg_format;

	if (! g_content_type_equals (mime_type, "image/jpeg"))
		return FALSE;

	formats = gdk_pixbuf_get_formats ();
	jpeg_format = NULL;
	for (scan = formats; (jpeg_format == NULL) && (scan != NULL); scan = g_slist_next (scan)) {
		GdkPixbufFormat  *format = scan->data;
		char            **mime_types;
		int               i;

		mime_types = gdk_pixbuf_format_get_mime_types (format);
		for (i = 0; mime_types[i] != NULL; i++)
			if (g_content_type_equals (mime_types[i], "image/jpeg"))
				break;

		if (mime_types[i] == NULL)
			continue;

		if (! gdk_pixbuf_format_is_writable (format))
			continue;

		jpeg_format = format;
	}

	return jpeg_format != NULL;

#endif /* HAVE_LIBJPEG */
}


#ifdef HAVE_LIBJPEG


/* error handler data */

struct error_handler_data {
	struct jpeg_error_mgr   pub;
	sigjmp_buf              setjmp_buffer;
        GError                **error;
};


static void
fatal_error_handler (j_common_ptr cinfo)
{
	struct error_handler_data *errmgr;
        char buffer[JMSG_LENGTH_MAX];

	errmgr = (struct error_handler_data *) cinfo->err;

        /* Create the message */
        (* cinfo->err->format_message) (cinfo, buffer);

        /* broken check for *error == NULL for robustness against
         * crappy JPEG library
         */
        if (errmgr->error && *errmgr->error == NULL) {
                g_set_error (errmgr->error,
                             GDK_PIXBUF_ERROR,
                             GDK_PIXBUF_ERROR_CORRUPT_IMAGE,
			     "Error interpreting JPEG image file (%s)",
                             buffer);
        }

	siglongjmp (errmgr->setjmp_buffer, 1);

        g_assert_not_reached ();
}


static void
output_message_handler (j_common_ptr cinfo)
{
	/* This method keeps libjpeg from dumping crap to stderr */
	/* do nothing */
}


static gboolean
_gdk_pixbuf_save_as_jpeg (GdkPixbuf   *pixbuf,
			  char       **buffer,
			  gsize       *buffer_size,
			  char       **keys,
			  char       **values,
			  GError     **error)
{
	struct jpeg_compress_struct cinfo;
	struct error_handler_data jerr;
	guchar            *buf = NULL;
	guchar            *ptr;
	guchar            *pixels = NULL;
	volatile int       quality = 85; /* default; must be between 0 and 100 */
	volatile int       smoothing = 0;
	volatile gboolean  optimize = FALSE;
	volatile gboolean  progressive = FALSE;
	int                i, j;
	int                w, h = 0;
	int                rowstride = 0;
	volatile int       bpp;

	if (keys && *keys) {
		char **kiter = keys;
		char **viter = values;

		while (*kiter) {
			if (strcmp (*kiter, "quality") == 0) {
				char *endptr = NULL;
				quality = strtol (*viter, &endptr, 10);

				if (endptr == *viter) {
					g_set_error (error,
						     GDK_PIXBUF_ERROR,
						     GDK_PIXBUF_ERROR_BAD_OPTION,
						     "JPEG quality must be a value between 0 and 100; value '%s' could not be parsed.",
						     *viter);

					return FALSE;
				}

				if (quality < 0 || quality > 100) {
					g_set_error (error,
						     GDK_PIXBUF_ERROR,
						     GDK_PIXBUF_ERROR_BAD_OPTION,
						     "JPEG quality must be a value between 0 and 100; value '%d' is not allowed.",
						     quality);

					return FALSE;
				}
			}
			else if (strcmp (*kiter, "smooth") == 0) {
				char *endptr = NULL;
				smoothing = strtol (*viter, &endptr, 10);

				if (endptr == *viter) {
					g_set_error (error,
						     GDK_PIXBUF_ERROR,
						     GDK_PIXBUF_ERROR_BAD_OPTION,
						     "JPEG smoothing must be a value between 0 and 100; value '%s' could not be parsed.",
						     *viter);

					return FALSE;
				}

				if (smoothing < 0 || smoothing > 100) {
					g_set_error (error,
						     GDK_PIXBUF_ERROR,
						     GDK_PIXBUF_ERROR_BAD_OPTION,
						     "JPEG smoothing must be a value between 0 and 100; value '%d' is not allowed.",
						     smoothing);

					return FALSE;
				}
			}
			else if (strcmp (*kiter, "optimize") == 0) {
				if (strcmp (*viter, "yes") == 0)
					optimize = TRUE;
				else if (strcmp (*viter, "no") == 0)
					optimize = FALSE;
				else {
					g_set_error (error,
						     GDK_PIXBUF_ERROR,
						     GDK_PIXBUF_ERROR_BAD_OPTION,
						     "JPEG optimize option must be 'yes' or 'no', value is: %s", *viter);

					return FALSE;
				}
			}
			else if (strcmp (*kiter, "progressive") == 0) {
				if (strcmp (*viter, "yes") == 0)
					progressive = TRUE;
				else if (strcmp (*viter, "no") == 0)
					progressive = FALSE;
				else {
					g_set_error (error,
						     GDK_PIXBUF_ERROR,
						     GDK_PIXBUF_ERROR_BAD_OPTION,
						     "JPEG progressive option must be 'yes' or 'no', value is: %s", *viter);

					return FALSE;
				}
			}
			else {
				g_warning ("Bad option name '%s' passed to JPEG saver", *kiter);
				return FALSE;
			}

			++kiter;
			++viter;
		}
	}

	rowstride = gdk_pixbuf_get_rowstride (pixbuf);
	w = gdk_pixbuf_get_width (pixbuf);
	h = gdk_pixbuf_get_height (pixbuf);
	if (gdk_pixbuf_get_has_alpha (pixbuf))
		bpp = 4;
	else
		bpp = 3;
	pixels = gdk_pixbuf_get_pixels (pixbuf);
	g_return_val_if_fail (pixels != NULL, FALSE);

	/* allocate a small buffer to convert image data */

	buf = g_try_malloc (w * bpp * sizeof (guchar));
	if (! buf) {
		g_set_error (error,
			     GDK_PIXBUF_ERROR,
			     GDK_PIXBUF_ERROR_INSUFFICIENT_MEMORY,
			     "Couldn't allocate memory for loading JPEG file");
		return FALSE;
	}

	/* set up error handling */

	cinfo.err = jpeg_std_error (&(jerr.pub));
	jerr.pub.error_exit = fatal_error_handler;
	jerr.pub.output_message = output_message_handler;
	jerr.error = error;
	if (sigsetjmp (jerr.setjmp_buffer, 1)) {
		jpeg_destroy_compress (&cinfo);
		g_free (buf);
		return FALSE;
	}

	/* setup compress params */

	jpeg_create_compress (&cinfo);
	_jpeg_memory_dest (&cinfo, (void **)buffer, buffer_size);

	cinfo.image_width      = w;
	cinfo.image_height     = h;
	cinfo.input_components = 3;
	cinfo.in_color_space   = JCS_RGB;

	/* set up jepg compression parameters */

	jpeg_set_defaults (&cinfo);
	jpeg_set_quality (&cinfo, quality, TRUE);
	cinfo.smoothing_factor = smoothing;
	cinfo.optimize_coding = optimize;

#ifdef HAVE_PROGRESSIVE_JPEG
	if (progressive)
		jpeg_simple_progression (&cinfo);
#endif /* HAVE_PROGRESSIVE_JPEG */

	jpeg_start_compress (&cinfo, TRUE);
	/* get the start pointer */
	ptr = pixels;
	/* go one scanline at a time... and save */
	i = 0;
	while (cinfo.next_scanline < cinfo.image_height) {
		JSAMPROW *jbuf;

		/* convert scanline from ARGB to RGB packed */
		for (j = 0; j < w; j++)
			memcpy (&(buf[j * 3]),
				&(ptr[i * rowstride + j * bpp]),
				3);

		/* write scanline */
		jbuf = (JSAMPROW *)(&buf);
		jpeg_write_scanlines (&cinfo, jbuf, 1);
		i++;
	}

	/* finish off */

	jpeg_finish_compress (&cinfo);
	jpeg_destroy_compress(&cinfo);
	g_free (buf);

	return TRUE;
}


#endif /* HAVE_LIBJPEG */


static gboolean
gth_jpeg_saver_save_pixbuf (GthPixbufSaver  *self,
			    GdkPixbuf       *pixbuf,
			    char           **buffer,
			    gsize           *buffer_size,
			    const char      *mime_type,
			    GError         **error)
{
#ifdef HAVE_LIBJPEG

	char     **option_keys;
	char     **option_values;
	int        i = -1;
	int        i_value;
	gboolean   result;

	option_keys = g_malloc (sizeof (char *) * 5);
	option_values = g_malloc (sizeof (char *) * 5);

	i++;
	i_value = eel_gconf_get_integer (PREF_JPEG_QUALITY, 85);
	option_keys[i] = g_strdup ("quality");
	option_values[i] = g_strdup_printf ("%d", i_value);

	i++;
	i_value = eel_gconf_get_integer (PREF_JPEG_SMOOTHING, 0);
	option_keys[i] = g_strdup ("smooth");
	option_values[i] = g_strdup_printf ("%d", i_value);

	i++;
	i_value = eel_gconf_get_boolean (PREF_JPEG_OPTIMIZE, TRUE);
	option_keys[i] = g_strdup ("optimize");
	option_values[i] = g_strdup (i_value != 0 ? "yes" : "no");

	i++;
	i_value = eel_gconf_get_boolean (PREF_JPEG_PROGRESSIVE, TRUE);
	option_keys[i] = g_strdup ("progressive");
	option_values[i] = g_strdup (i_value != 0 ? "yes" : "no");

	i++;
	option_keys[i] = NULL;
	option_values[i] = NULL;

	result = _gdk_pixbuf_save_as_jpeg (pixbuf,
					   buffer,
					   buffer_size,
					   option_keys,
					   option_values,
					   error);

	g_strfreev (option_keys);
	g_strfreev (option_values);

#else /* ! HAVE_LIBJPEG */

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

#endif /* HAVE_LIBJPEG */

	return result;
}


static void
gth_jpeg_saver_class_init (GthJpegSaverClass *klass)
{
	GObjectClass        *object_class;
	GthPixbufSaverClass *pixbuf_saver_class;

	parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GthJpegSaverPrivate));

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = gth_jpeg_saver_finalize;

	pixbuf_saver_class = GTH_PIXBUF_SAVER_CLASS (klass);
	pixbuf_saver_class->id = "jpeg";
	pixbuf_saver_class->display_name = _("JPEG");
	pixbuf_saver_class->get_control = gth_jpeg_saver_get_control;
	pixbuf_saver_class->save_options = gth_jpeg_saver_save_options;
	pixbuf_saver_class->can_save = gth_jpeg_saver_can_save;
	pixbuf_saver_class->save_pixbuf = gth_jpeg_saver_save_pixbuf;
}


GType
gth_jpeg_saver_get_type (void)
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (GthJpegSaverClass),
			NULL,
			NULL,
			(GClassInitFunc) gth_jpeg_saver_class_init,
			NULL,
			NULL,
			sizeof (GthJpegSaver),
			0,
			(GInstanceInitFunc) gth_jpeg_saver_init
		};

		type = g_type_register_static (GTH_TYPE_PIXBUF_SAVER,
					       "GthJpegSaver",
					       &type_info,
					       0);
	}

	return type;
}
