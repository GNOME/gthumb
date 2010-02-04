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
#include <gio/gio.h>
#include <extensions/edit_metadata/gth-edit-metadata-dialog.h>
#include "exiv2-utils.h"
#include "gth-edit-exiv2-page.h"


#define GTH_EDIT_EXIV2_PAGE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GTH_TYPE_EDIT_EXIV2_PAGE, GthEditExiv2PagePrivate))
#define GET_WIDGET(name) _gtk_builder_get_widget (self->priv->builder, (name))


static gpointer gth_edit_exiv2_page_parent_class = NULL;


struct _GthEditExiv2PagePrivate {
	GtkBuilder  *builder;
	gboolean     supported;
};


static void
set_entry_value (GthEditExiv2Page *self,
		 GthFileData      *file_data,
		 const char       *attribute,
		 const char       *widget_id)
{
	GthMetadata *metadata;

	metadata = (GthMetadata *) g_file_info_get_attribute_object (file_data->info, attribute);
	if (metadata != NULL)
		gtk_entry_set_text (GTK_ENTRY (GET_WIDGET (widget_id)), gth_metadata_get_formatted (metadata));
	else
		gtk_entry_set_text (GTK_ENTRY (GET_WIDGET (widget_id)), "");
}


void
gth_edit_exiv2_page_real_set_file (GthEditMetadataPage *base,
		 		   GthFileData         *file_data)
{
	GthEditExiv2Page *self;
	GthMetadata      *metadata;

	self = GTH_EDIT_EXIV2_PAGE (base);

	self->priv->supported = exiv2_supports_writes (gth_file_data_get_mime_type (file_data));
	if (! self->priv->supported) {
		gtk_widget_hide (GTK_WIDGET (base));
		return;
	}

	set_entry_value (self, file_data, "Iptc::Application2::Copyright", "copyright_entry");
	set_entry_value (self, file_data, "Iptc::Application2::Credit", "credit_entry");
	set_entry_value (self, file_data, "Iptc::Application2::Byline", "byline_entry");
	set_entry_value (self, file_data, "Iptc::Application2::BylineTitle", "byline_title_entry");
	set_entry_value (self, file_data, "Iptc::Application2::CountryName", "country_name_entry");
	set_entry_value (self, file_data, "Iptc::Application2::CountryCode", "country_code_entry");
	set_entry_value (self, file_data, "Iptc::Application2::City", "city_entry");
	set_entry_value (self, file_data, "Iptc::Application2::Language", "language_entry");
	set_entry_value (self, file_data, "Iptc::Application2::ObjectName", "object_name_entry");
	set_entry_value (self, file_data, "Iptc::Application2::Source", "source_entry");
	set_entry_value (self, file_data, "Iptc::Envelope::Destination", "destination_entry");

	metadata = (GthMetadata *) g_file_info_get_attribute_object (file_data->info, "Iptc::Application2::Urgency");
	if (metadata != NULL) {
		float v;

		if (sscanf (gth_metadata_get_formatted (metadata), "%f", &v) == 1)
			gtk_adjustment_set_value (GTK_ADJUSTMENT (GET_WIDGET ("urgency_adjustment")), (double) v);
		else
			gtk_adjustment_set_value (GTK_ADJUSTMENT (GET_WIDGET ("urgency_adjustment")), 0.0);
	}
	else
		gtk_adjustment_set_value (GTK_ADJUSTMENT (GET_WIDGET ("urgency_adjustment")), 0.0);

	gtk_widget_show (GTK_WIDGET (base));
}


static void
set_attribute_from_entry (GthEditExiv2Page *self,
			  GFileInfo        *info,
			  const char       *attribute,
			  const char       *widget_id)
{
	GthMetadata *metadata;

	metadata = g_object_new (GTH_TYPE_METADATA,
				 "id", attribute,
				 "raw", gtk_entry_get_text (GTK_ENTRY (GET_WIDGET (widget_id))),
				 "formatted", gtk_entry_get_text (GTK_ENTRY (GET_WIDGET (widget_id))),
				 NULL);
	g_file_info_set_attribute_object (info, attribute, G_OBJECT (metadata));
	g_object_unref (metadata);
}


void
gth_edit_exiv2_page_real_update_info (GthEditMetadataPage *base,
				      GFileInfo           *info)
{
	GthEditExiv2Page *self;
	double            v;
	char             *s;
	GthMetadata      *metadata;

	self = GTH_EDIT_EXIV2_PAGE (base);

	if (! self->priv->supported)
		return;

	set_attribute_from_entry (self, info, "Iptc::Application2::Copyright", "copyright_entry");
	set_attribute_from_entry (self, info, "Iptc::Application2::Credit", "credit_entry");
	set_attribute_from_entry (self, info, "Iptc::Application2::Byline", "byline_entry");
	set_attribute_from_entry (self, info, "Iptc::Application2::BylineTitle", "byline_title_entry");
	set_attribute_from_entry (self, info, "Iptc::Application2::CountryName", "country_name_entry");
	set_attribute_from_entry (self, info, "Iptc::Application2::CountryCode", "country_code_entry");
	set_attribute_from_entry (self, info, "Iptc::Application2::City", "city_entry");
	set_attribute_from_entry (self, info, "Iptc::Application2::Language", "language_entry");
	set_attribute_from_entry (self, info, "Iptc::Application2::ObjectName", "object_name_entry");
	set_attribute_from_entry (self, info, "Iptc::Application2::Source", "source_entry");
	set_attribute_from_entry (self, info, "Iptc::Envelope::Destination", "destination_entry");

	/* urgency */

	v = gtk_adjustment_get_value (GTK_ADJUSTMENT (GET_WIDGET ("urgency_adjustment")));
	s = g_strdup_printf ("%1.g", v);
	metadata = g_object_new (GTH_TYPE_METADATA,
				 "id", "Iptc::Application2::Urgency",
				 "raw", s,
				 "formatted", s,
				 NULL);
	g_file_info_set_attribute_object (info, "Iptc::Application2::Urgency", G_OBJECT (metadata));
	g_object_unref (metadata);
	g_free (s);
}


const char *
gth_edit_exiv2_page_real_get_name (GthEditMetadataPage *self)
{
	return _("Other");
}


static void
gth_edit_exiv2_page_finalize (GObject *object)
{
	GthEditExiv2Page *self;

	self = GTH_EDIT_EXIV2_PAGE (object);

	g_object_unref (self->priv->builder);

	G_OBJECT_CLASS (gth_edit_exiv2_page_parent_class)->finalize (object);
}


static void
gth_edit_exiv2_page_class_init (GthEditExiv2PageClass *klass)
{
	gth_edit_exiv2_page_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GthEditExiv2PagePrivate));

	G_OBJECT_CLASS (klass)->finalize = gth_edit_exiv2_page_finalize;
}


static void
gth_edit_exiv2_page_init (GthEditExiv2Page *self)
{
	self->priv = GTH_EDIT_EXIV2_PAGE_GET_PRIVATE (self);

	gtk_container_set_border_width (GTK_CONTAINER (self), 12);

	self->priv->builder = _gtk_builder_new_from_file ("edit-exiv2-page.ui", "exiv2");
  	gtk_box_pack_start (GTK_BOX (self), _gtk_builder_get_widget (self->priv->builder, "content"), TRUE, TRUE, 0);
}


static void
gth_edit_exiv2_page_gth_edit_exiv2_page_interface_init (GthEditMetadataPageIface *iface)
{
	iface->set_file = gth_edit_exiv2_page_real_set_file;
	iface->update_info = gth_edit_exiv2_page_real_update_info;
	iface->get_name = gth_edit_exiv2_page_real_get_name;
}


GType
gth_edit_exiv2_page_get_type (void)
{
	static GType type = 0;

	if (type == 0) {
		static const GTypeInfo g_define_type_info = {
			sizeof (GthEditExiv2PageClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) gth_edit_exiv2_page_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (GthEditExiv2Page),
			0,
			(GInstanceInitFunc) gth_edit_exiv2_page_init,
			NULL
		};
		static const GInterfaceInfo gth_edit_exiv2_page_info = {
			(GInterfaceInitFunc) gth_edit_exiv2_page_gth_edit_exiv2_page_interface_init,
			(GInterfaceFinalizeFunc) NULL,
			NULL
		};
		type = g_type_register_static (GTK_TYPE_VBOX,
					       "GthEditExiv2Page",
					       &g_define_type_info,
					       0);
		g_type_add_interface_static (type, GTH_TYPE_EDIT_METADATA_PAGE, &gth_edit_exiv2_page_info);
	}

	return type;
}
