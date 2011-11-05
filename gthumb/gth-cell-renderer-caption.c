/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2011 Free Software Foundation, Inc.
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
#include "gth-cell-renderer-caption.h"
#include "gth-file-data.h"
#include "glib-utils.h"


#define LINE_SPACING 4


enum {
	PROP_0,
	PROP_FILE,
	PROP_FILE_ATTRIBUTES
};


struct _GthCellRendererCaptionPrivate
{
	GthFileData  *file;
	char         *attributes;
	char        **attributes_v;
	char         *text;
	GHashTable   *heights_cache;
	int           last_width;
};


G_DEFINE_TYPE (GthCellRendererCaption, gth_cell_renderer_caption, GTK_TYPE_CELL_RENDERER)


static void
gth_cell_renderer_caption_finalize (GObject *object)
{
	GthCellRendererCaption *self = GTH_CELL_RENDERER_CAPTION (object);

	g_strfreev (self->priv->attributes_v);
	g_free (self->priv->attributes);
	_g_object_unref (self->priv->file);
	g_free (self->priv->text);
	g_hash_table_destroy (self->priv->heights_cache);

	G_OBJECT_CLASS (gth_cell_renderer_caption_parent_class)->finalize (object);
}


static void
gth_cell_renderer_caption_get_property (GObject    *object,
					guint       param_id,
					GValue     *value,
					GParamSpec *pspec)
{
	GthCellRendererCaption *self = (GthCellRendererCaption *) object;

	switch (param_id) {
	case PROP_FILE:
		g_value_set_object (value, self->priv->file);
		break;

	case PROP_FILE_ATTRIBUTES:
		g_value_set_string (value, self->priv->attributes);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}


static void
_gth_cell_renderer_caption_set_attributes (GthCellRendererCaption *self,
					   const char             *attributes)
{
	g_free (self->priv->attributes);
	self->priv->attributes = g_strdup (attributes);

	if (self->priv->attributes_v != NULL) {
		g_strfreev (self->priv->attributes_v);
		self->priv->attributes_v = NULL;
	}
	if (self->priv->attributes != NULL)
		self->priv->attributes_v = g_strsplit (self->priv->attributes, ",", -1);
}


#define MAX_TEXT_LENGTH 70
#define ODD_ROW_ATTR_STYLE ""
#define EVEN_ROW_ATTR_STYLE " style='italic'"


static void
_gth_cell_renderer_caption_update_text (GthCellRendererCaption *self)
{
	GString  *metadata;
	gboolean  odd;
	int       i;

	g_free (self->priv->text);
	self->priv->text = NULL;

	if ((self->priv->file == NULL)
	    || (self->priv->attributes_v == NULL)
	    || g_str_equal (self->priv->attributes_v[0], "none"))
	{
		return;
	}

	metadata = g_string_new (NULL);

	odd = TRUE;
	for (i = 0; self->priv->attributes_v[i] != NULL; i++) {
		char *value;

		value = gth_file_data_get_attribute_as_string (self->priv->file, self->priv->attributes_v[i]);
		if ((value != NULL) && ! g_str_equal (value, "")) {
			char *escaped;

			if (metadata->len > 0)
				g_string_append (metadata, "\n");
			if (g_utf8_strlen (value, -1) > MAX_TEXT_LENGTH) {
				char *tmp;

				tmp = g_strdup (value);
				g_utf8_strncpy (tmp, value, MAX_TEXT_LENGTH);
				g_free (value);
				value = g_strdup_printf ("%s…", tmp);

				g_free (tmp);
			}

			escaped = g_markup_escape_text (value, -1);
			g_string_append_printf (metadata, "<span%s>%s</span>", (odd ? ODD_ROW_ATTR_STYLE : EVEN_ROW_ATTR_STYLE), value);

			g_free (escaped);
		}
		odd = ! odd;

		g_free (value);
	}

	self->priv->text = g_string_free (metadata, FALSE);
}


static void
gth_cell_renderer_caption_set_property (GObject      *object,
				        guint         param_id,
				        const GValue *value,
				        GParamSpec   *pspec)
{
	GthCellRendererCaption *self = (GthCellRendererCaption *) object;

	switch (param_id) {
	case PROP_FILE:
		if (self->priv->file != NULL)
			g_object_unref (self->priv->file);
		self->priv->file = g_value_dup_object (value);
		_gth_cell_renderer_caption_update_text (self);
		break;

	case PROP_FILE_ATTRIBUTES:
		_gth_cell_renderer_caption_set_attributes (self, g_value_get_string (value));
		_gth_cell_renderer_caption_update_text (self);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}


static GtkSizeRequestMode
gth_cell_renderer_caption_get_request_mode (GtkCellRenderer *cell)
{
	return GTK_SIZE_REQUEST_HEIGHT_FOR_WIDTH;
}


static void
gth_cell_renderer_caption_get_preferred_width (GtkCellRenderer *cell,
                			       GtkWidget       *widget,
                			       int             *minimum_size,
                			       int             *natural_size)
{
	GthCellRendererCaption *self = (GthCellRendererCaption *) cell;
	int                     width;

	width = 0;
	g_object_get (cell, "width", &width, NULL);

	if (self->priv->last_width != width) {
		self->priv->last_width = width;
		g_hash_table_remove_all (self->priv->heights_cache);
	}

	if (minimum_size) *minimum_size = width;
	if (natural_size) *natural_size = width;
}


static PangoLayout *
_gth_cell_renderer_caption_create_pango_layout (GthCellRendererCaption *self,
						GtkWidget              *widget,
						const char             *text)
{
	PangoLayout *pango_layout;

	pango_layout = gtk_widget_create_pango_layout (widget, NULL);
	pango_layout_set_markup (pango_layout, text, -1);
	pango_layout_set_wrap (pango_layout, PANGO_WRAP_WORD_CHAR);
	pango_layout_set_alignment (pango_layout, PANGO_ALIGN_CENTER);
	pango_layout_set_spacing (pango_layout, LINE_SPACING);

	return pango_layout;
}


static void
gth_cell_renderer_caption_get_preferred_height_for_width (GtkCellRenderer *cell,
							  GtkWidget       *widget,
							  int              width,
							  int             *minimum_height,
							  int             *natural_height)
{
	GthCellRendererCaption *self = (GthCellRendererCaption *) cell;
	int                     height;

	height = 0;

	if ((self->priv->text != NULL) && ! g_str_equal (self->priv->text, "")) {
		int *height_p;

		height_p = g_hash_table_lookup (self->priv->heights_cache, self->priv->text);
		if (height_p != NULL) {
			height = GPOINTER_TO_INT (height_p);
		}
		else {
			PangoLayout *pango_layout;

			pango_layout = _gth_cell_renderer_caption_create_pango_layout (self, widget, self->priv->text);
			pango_layout_set_width (pango_layout, width * PANGO_SCALE);
			pango_layout_get_pixel_size (pango_layout, &width, &height);

			g_hash_table_insert (self->priv->heights_cache,
					     g_strdup (self->priv->text),
					     GINT_TO_POINTER (height));

			g_object_unref (pango_layout);
		}
	}

	if (minimum_height) *minimum_height = height;
	if (natural_height) *natural_height = height;
}


static void
gth_cell_renderer_caption_render (GtkCellRenderer      *cell,
				  cairo_t              *cr,
				  GtkWidget            *widget,
				  const GdkRectangle   *background_area,
				  const GdkRectangle   *cell_area,
				  GtkCellRendererState  cell_state)
{
	GthCellRendererCaption *self = (GthCellRendererCaption *) cell;
	PangoLayout             *pango_layout;
	GtkStyleContext        *style_context;
	GdkRGBA                 foreground_color;

	if (self->priv->text == NULL)
		return;

	pango_layout = _gth_cell_renderer_caption_create_pango_layout (self, widget, self->priv->text);
	pango_layout_set_width (pango_layout, cell_area->width * PANGO_SCALE);

	style_context = gtk_widget_get_style_context (widget);
	gtk_style_context_get_color (style_context, gtk_widget_get_state_flags (widget), &foreground_color);
	gdk_cairo_set_source_rgba (cr, &foreground_color);
	cairo_move_to (cr, cell_area->x, cell_area->y);
	pango_cairo_show_layout (cr, pango_layout);

	g_object_unref (pango_layout);
}


static void
gth_cell_renderer_caption_class_init (GthCellRendererCaptionClass *klass)
{
	GObjectClass         *object_class;
	GtkCellRendererClass *cell_renderer;

	g_type_class_add_private (klass, sizeof (GthCellRendererCaptionPrivate));

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = gth_cell_renderer_caption_finalize;
	object_class->get_property = gth_cell_renderer_caption_get_property;
	object_class->set_property = gth_cell_renderer_caption_set_property;

	cell_renderer = GTK_CELL_RENDERER_CLASS (klass);
	cell_renderer->get_request_mode = gth_cell_renderer_caption_get_request_mode;
	cell_renderer->get_preferred_width = gth_cell_renderer_caption_get_preferred_width;
	cell_renderer->get_preferred_height_for_width = gth_cell_renderer_caption_get_preferred_height_for_width;
	cell_renderer->render = gth_cell_renderer_caption_render;

	/* properties */

	g_object_class_install_property (object_class,
					 PROP_FILE,
					 g_param_spec_object ("file",
							      "File",
							      "The file data",
							      GTH_TYPE_FILE_DATA,
							      G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_FILE_ATTRIBUTES,
					 g_param_spec_string ("file-attributes",
							      "File attributes",
							      "The attributes to display, separated by a comma",
							      NULL,
							      G_PARAM_READWRITE));
}


static void
gth_cell_renderer_caption_init (GthCellRendererCaption *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_CELL_RENDERER_CAPTION, GthCellRendererCaptionPrivate);
	self->priv->file = NULL;
	self->priv->attributes = NULL;
	self->priv->attributes_v = NULL;
	self->priv->text = NULL;
	self->priv->heights_cache = g_hash_table_new_full (g_str_hash,
							   g_str_equal,
							   g_free,
							   NULL);
	self->priv->last_width = 0;
}


GtkCellRenderer *
gth_cell_renderer_caption_new (void)
{
	return g_object_new (GTH_TYPE_CELL_RENDERER_CAPTION, NULL);
}


void
gth_cell_renderer_caption_clear_cache (GthCellRendererCaption *self)
{
	self->priv->last_width = 0;
	g_hash_table_remove_all (self->priv->heights_cache);
}
