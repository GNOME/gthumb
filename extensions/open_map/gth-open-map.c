/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2025 Free Software Foundation, Inc.
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
#include <math.h>
#include <glib/gi18n.h>
#include <gthumb.h>
#include "gth-open-map.h"


static void gth_open_map_gth_property_view_interface_init (GthPropertyViewInterface *iface);


struct _GthOpenMapPrivate {
	GtkWidget *link_button;
};


G_DEFINE_TYPE_WITH_CODE (GthOpenMap,
			 gth_open_map,
			 GTK_TYPE_BOX,
			 G_ADD_PRIVATE (GthOpenMap)
			 G_IMPLEMENT_INTERFACE (GTH_TYPE_PROPERTY_VIEW,
						gth_open_map_gth_property_view_interface_init))


/* Exif format: %d/%d %d/%d %d/%d */
static double
exif_coordinate_to_decimal (const char *raw)
{
	double   value;
	double   factor;
	char   **raw_values;
	int      i;

	value = 0.0;
	factor = 1.0;
	raw_values = g_strsplit (raw, " ", 3);
	for (i = 0; i < 3; i++) {
		int numerator;
		int denominator;

		if (raw_values[i] == NULL)
			break; /* Error */

		sscanf (raw_values[i], "%d/%d", &numerator, &denominator);
		if ((numerator != 0) && (denominator != 0))
			value += ((double) numerator / denominator) / factor;

		factor *= 60.0;
	}

	g_strfreev (raw_values);

	return value;
}


static char *
decimal_to_string (double value)
{
	GString *s;
	double   part;

	s = g_string_new ("");
	if (value < 0.0)
		value = - value;

	/* degree */

	part = floor (value);
	g_string_append_printf (s, "%.0f°", part);
	value = (value - part) * 60.0;

	/* minutes */

	part = floor (value);
	g_string_append_printf (s, " %.0fʹ", part);
	value = (value - part) * 60.0;

	/* seconds */

	g_string_append_printf (s, " %02.3fʺ", value);

	return g_string_free (s, FALSE);
}


static char *
decimal_coordinates_to_string (double latitude,
			       double longitude)
{
	char *latitude_s;
	char *longitude_s;
	char *s;

	latitude_s = decimal_to_string (latitude);
	longitude_s = decimal_to_string (longitude);
	s = g_strdup_printf ("%s %s\n%s %s",
			     latitude_s,
			     ((latitude < 0.0) ? C_("Cardinal point", "S") : C_("Cardinal point", "N")),
			     longitude_s,
			     ((longitude < 0.0) ? C_("Cardinal point", "W") : C_("Cardinal point", "E")));

	g_free (longitude_s);
	g_free (latitude_s);

	return s;
}


static int
gth_open_map_get_coordinates (GthOpenMap  *self,
			      GthFileData *file_data,
			      double      *out_latitude,
			      double      *out_longitude)
{
	int    coordinates_available;
	double latitude = 0.0;
	double longitude = 0.0;

	coordinates_available = 0;

	if (file_data != NULL) {
		GthMetadata *metadata;

		metadata = (GthMetadata *) g_file_info_get_attribute_object (file_data->info, "Exif::GPSInfo::GPSLatitude");
		if (metadata != NULL) {
			latitude = exif_coordinate_to_decimal (gth_metadata_get_raw (metadata));

			metadata = (GthMetadata *) g_file_info_get_attribute_object (file_data->info, "Exif::GPSInfo::GPSLatitudeRef");
			if (metadata != NULL) {
				if (g_strcmp0 (gth_metadata_get_raw (metadata), "S") == 0)
					latitude = - latitude;
			}

			coordinates_available++;
		}

		metadata = (GthMetadata *) g_file_info_get_attribute_object (file_data->info, "Exif::GPSInfo::GPSLongitude");
		if (metadata != NULL) {
			longitude = exif_coordinate_to_decimal (gth_metadata_get_raw (metadata));

			metadata = (GthMetadata *) g_file_info_get_attribute_object (file_data->info, "Exif::GPSInfo::GPSLongitudeRef");
			if (metadata != NULL) {
				if (g_strcmp0 (gth_metadata_get_raw (metadata), "W") == 0)
					longitude = - longitude;
			}

			coordinates_available++;
		}
	}

	if (out_latitude != NULL)
		*out_latitude = latitude;
	if (out_longitude != NULL)
		*out_longitude = longitude;

	return coordinates_available;
}


static gboolean
gth_open_map_real_can_view (GthPropertyView *base,
			   GthFileData     *file_data)
{
	GthOpenMap *self = GTH_OPEN_MAP (base);
	return gth_open_map_get_coordinates (self, file_data, NULL, NULL) == 2;
}


static void
gth_open_map_real_set_file (GthPropertyView *base,
			    GthFileData     *file_data)
{
	GthOpenMap *self = GTH_OPEN_MAP (base);
	int         coordinates_available;
	double      latitude;
	double      longitude;

	coordinates_available = gth_open_map_get_coordinates (self, file_data, &latitude, &longitude);
	if (coordinates_available == 2) {
		char *position = decimal_coordinates_to_string (latitude, longitude);
		gtk_button_set_label (GTK_BUTTON (self->priv->link_button), position);

		char latitude_s[G_ASCII_DTOSTR_BUF_SIZE];
		g_ascii_formatd (latitude_s, sizeof (latitude_s), "%0.6f", latitude);

		char longitude_s[G_ASCII_DTOSTR_BUF_SIZE];
		g_ascii_formatd (longitude_s, sizeof (longitude_s), "%0.6f", longitude);

		char *uri = g_strdup_printf ("https://www.openstreetmap.org/?mlat=%s&mlon=%s&zoom=6", latitude_s, longitude_s);
		gtk_link_button_set_uri (GTK_LINK_BUTTON (self->priv->link_button), uri);

		g_free (uri);
		g_free (position);
	}
}


static const char *
gth_open_map_real_get_name (GthPropertyView *self)
{
	return _("Map");
}


static const char *
gth_open_map_real_get_icon (GthPropertyView *self)
{
	return "map-symbolic";
}


static void
gth_open_map_finalize (GObject *base)
{
	G_OBJECT_CLASS (gth_open_map_parent_class)->finalize (base);
}


static void
gth_open_map_class_init (GthOpenMapClass *klass)
{
	G_OBJECT_CLASS (klass)->finalize = gth_open_map_finalize;
}


static void
gth_open_map_init (GthOpenMap *self)
{
	self->priv = gth_open_map_get_instance_private (self);

	gtk_box_set_spacing (GTK_BOX (self), 0);
	gtk_orientable_set_orientation (GTK_ORIENTABLE (self), GTK_ORIENTATION_VERTICAL);

	GtkWidget *frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
	gtk_widget_show (frame);
	gtk_style_context_add_class (gtk_widget_get_style_context (frame), GTK_STYLE_CLASS_VIEW);
	gtk_box_pack_start (GTK_BOX (self), frame, TRUE, TRUE, 0);

	self->priv->link_button = gtk_link_button_new ("");
	gtk_widget_show (self->priv->link_button);
	gtk_widget_set_margin_top (self->priv->link_button, 12);
	gtk_widget_set_margin_bottom (self->priv->link_button, 12);
	gtk_container_add (GTK_CONTAINER (frame), self->priv->link_button);
}


static void
gth_open_map_gth_property_view_interface_init (GthPropertyViewInterface *iface)
{
	iface->get_name = gth_open_map_real_get_name;
	iface->get_icon = gth_open_map_real_get_icon;
	iface->can_view = gth_open_map_real_can_view;
	iface->set_file = gth_open_map_real_set_file;
}
