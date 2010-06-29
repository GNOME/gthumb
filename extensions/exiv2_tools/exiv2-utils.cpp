/* -*- Mode: CPP; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2008-2009 Free Software Foundation, Inc.
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
#include <glib.h>
#define GDK_PIXBUF_ENABLE_BACKEND
#include <gdk-pixbuf/gdk-pixbuf-io.h>
#include <exiv2/basicio.hpp>
#include <exiv2/error.hpp>
#include <exiv2/image.hpp>
#include <exiv2/exif.hpp>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <exiv2/xmp.hpp>
#include <gthumb.h>
#include "exiv2-utils.h"

using namespace std;


/* Some bits of information may be contained in more than one metadata tag.
   The arrays below define the valid tags for a particular piece of
   information, in decreasing order of preference (best one first) */

const char *_DATE_TAG_NAMES[] = {
	"Exif::Image::DateTime",
	"Xmp::exif::DateTime",
	"Exif::Photo::DateTimeOriginal",
	"Xmp::exif::DateTimeOriginal",
	"Exif::Photo::DateTimeDigitized",
	"Xmp::exif::DateTimeDigitized",
	"Xmp::xmp::CreateDate",
	"Xmp::photoshop::DateCreated",
	"Xmp::xmp::ModifyDate",
	"Xmp::xmp::MetadataDate",
	NULL
};

const char *_ORIGINAL_DATE_TAG_NAMES[] = {
	"Exif::Photo::DateTimeOriginal",
	"Xmp::exif::DateTimeOriginal",
	"Exif::Photo::DateTimeDigitized",
	"Xmp::exif::DateTimeDigitized",
	"Xmp::xmp::CreateDate",
	"Xmp::photoshop::DateCreated",
	NULL
};

const char *_EXPOSURE_TIME_TAG_NAMES[] = {
	"Exif::Photo::ExposureTime",
	"Xmp::exif::ExposureTime",
	"Exif::Photo::ShutterSpeedValue",
	"Xmp::exif::ShutterSpeedValue",
	NULL
};

const char *_EXPOSURE_MODE_TAG_NAMES[] = {
	"Exif::Photo::ExposureMode",
	"Xmp::exif::ExposureMode",
	NULL
};

const char *_ISOSPEED_TAG_NAMES[] = {
	"Exif::Photo::ISOSpeedRatings",
	"Xmp::exif::ISOSpeedRatings",
	NULL
};

const char *_APERTURE_TAG_NAMES[] = {
	"Exif::Photo::ApertureValue",
	"Xmp::exif::ApertureValue",
	"Exif::Photo::FNumber",
	"Xmp::exif::FNumber",
	NULL
};

const char *_FOCAL_LENGTH_TAG_NAMES[] = {
	"Exif::Photo::FocalLength",
	"Xmp::exif::FocalLength",
	NULL
};

const char *_SHUTTER_SPEED_TAG_NAMES[] = {
	"Exif::Photo::ShutterSpeedValue",
	"Xmp::exif::ShutterSpeedValue",
	NULL
};

const char *_MAKE_TAG_NAMES[] = {
	"Exif::Image::Make",
	"Xmp::tiff::Make",
	NULL
};

const char *_MODEL_TAG_NAMES[] = {
	"Exif::Image::Model",
	"Xmp::tiff::Model",
	NULL
};

const char *_FLASH_TAG_NAMES[] = {
	"Exif::Photo::Flash",
	"Xmp::exif::Flash",
	NULL
};

const char *_ORIENTATION_TAG_NAMES[] = {
	"Exif::Image::Orientation",
	"Xmp::tiff::Orientation",
	NULL
};

const char *_DESCRIPTION_TAG_NAMES[] = {
	"Exif::Photo::UserComment",
	"Exif::Image::ImageDescription",
	"Xmp::dc::description",
	"Xmp::tiff::ImageDescription",
	"Iptc::Application2::Caption",
	"Iptc::Application2::Headline",
	NULL
};

const char *_TITLE_TAG_NAMES[] = {
	"Xmp::dc::title",
	NULL
};

const char *_LOCATION_TAG_NAMES[] = {
	"Xmp::iptc::Location",
	"Iptc::Application2::LocationName",
	NULL
};

const char *_KEYWORDS_TAG_NAMES[] = {
	"Xmp::dc::subject",
	"Xmp::iptc::Keywords",
	"Iptc::Application2::Keywords",
	NULL
};


inline static char *
exiv2_key_from_attribute (const char *attribute)
{
	return _g_replace (attribute, "::", ".");
}


inline static char *
exiv2_key_to_attribute (const char *key)
{
	return _g_replace (key, ".", "::");
}


static gboolean
attribute_is_date (const char *key)
{
	int i;

	for (i = 0; _DATE_TAG_NAMES[i] != NULL; i++) {
		if (strcmp (_DATE_TAG_NAMES[i], key) == 0)
			return TRUE;
	}

	return FALSE;
}


static void
set_file_info (GFileInfo  *info,
	       const char *key,
	       const char *description,
	       const char *formatted_value,
	       const char *raw_value,
	       const char *category,
	       const char *type_name)
{
	char            *attribute;
	GthMetadataInfo *metadata_info;
	GthMetadata     *metadata;
	char            *description_utf8;
	char            *formatted_value_utf8;

	/*if (_g_utf8_all_spaces (formatted_value))
		return;*/

	attribute = exiv2_key_to_attribute (key);
	description_utf8 = g_locale_to_utf8 (description, -1, NULL, NULL, NULL);
	if (attribute_is_date (attribute)) {
		GTimeVal time_;

		if (_g_time_val_from_exif_date (raw_value, &time_))
			formatted_value_utf8 = _g_time_val_strftime (&time_, "%x %X");
		else
			formatted_value_utf8 = g_locale_to_utf8 (formatted_value, -1, NULL, NULL, NULL);
	}
	else {
		const char *formatted_clean;

		if (strncmp (formatted_value, "lang=", 5) == 0)
			formatted_clean = strchr (formatted_value, ' ') + 1;
		else
			formatted_clean = formatted_value;
		formatted_value_utf8 = g_locale_to_utf8 (formatted_clean, -1, NULL, NULL, NULL);
	}

	metadata_info = gth_main_get_metadata_info (attribute);
	if ((metadata_info == NULL) && (category != NULL)) {
		GthMetadataInfo info;

		info.id = attribute;
		info.type = (type_name != NULL) ? g_strdup (type_name) : NULL;
		info.display_name = description_utf8;
		info.category = category;
		info.sort_order = 500;
		info.flags = GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW;
		metadata_info = gth_main_register_metadata_info (&info);
	}

	if ((metadata_info != NULL) && (metadata_info->type == NULL) && (type_name != NULL))
		metadata_info->type = g_strdup (type_name);

	if ((metadata_info != NULL) && (metadata_info->display_name == NULL) && (description_utf8 != NULL))
		metadata_info->display_name = g_strdup (description_utf8);

	metadata = gth_metadata_new ();
	g_object_set (metadata,
		      "id", key,
		      "description", description_utf8,
		      "formatted", formatted_value_utf8,
		      "raw", raw_value,
		      "value-type", type_name,
		      NULL);
	g_file_info_set_attribute_object (info, attribute, G_OBJECT (metadata));

	g_object_unref (metadata);
	g_free (formatted_value_utf8);
	g_free (description_utf8);
	g_free (attribute);
}


static void
set_attribute_from_tagset (GFileInfo  *info,
			   const char *attribute,
			   const char *tagset[])
{
	GObject *metadata;
	int      i;
	char    *key;
	char    *description;
	char    *formatted_value;
	char    *raw_value;
	char    *type_name;

	metadata = NULL;
	for (i = 0; tagset[i] != NULL; i++) {
		metadata = g_file_info_get_attribute_object (info, tagset[i]);
		if (metadata != NULL)
			break;
	}

	if (metadata == NULL)
		return;

	g_object_get (metadata,
		      "id", &key,
		      "description", &description,
		      "formatted", &formatted_value,
		      "raw", &raw_value,
		      "value-type", &type_name,
		      NULL);
	set_file_info (info,
		       attribute,
		       description,
		       formatted_value,
		       raw_value,
		       NULL,
		       type_name);
}


static void
set_string_list_attribute_from_tagset (GFileInfo  *info,
				       const char *attribute,
				       const char *tagset[])
{
	GObject        *metadata;
	int             i;
	char           *raw;
	char          **keywords;
	GthStringList  *string_list;

	metadata = NULL;
	for (i = 0; tagset[i] != NULL; i++) {
		metadata = g_file_info_get_attribute_object (info, tagset[i]);
		if (metadata != NULL)
			break;
	}

	if (metadata == NULL)
		return;

	g_object_get (metadata, "raw", &raw, NULL);
	keywords = g_strsplit (raw, ", ", -1);
	string_list = gth_string_list_new_from_strv (keywords);
	g_file_info_set_attribute_object (info, attribute, G_OBJECT (string_list));

	g_strfreev (keywords);
	g_free (raw);
}


static void
set_attributes_from_tagsets (GFileInfo *info)
{
	set_attribute_from_tagset (info, "general::datetime", _DATE_TAG_NAMES);
	set_attribute_from_tagset (info, "general::description", _DESCRIPTION_TAG_NAMES);
	set_attribute_from_tagset (info, "general::title", _TITLE_TAG_NAMES);
	set_attribute_from_tagset (info, "general::location", _LOCATION_TAG_NAMES);
	set_string_list_attribute_from_tagset (info, "general::tags", _KEYWORDS_TAG_NAMES);
	set_attribute_from_tagset (info, "Embedded::Photo::DateTimeOriginal", _ORIGINAL_DATE_TAG_NAMES);
	set_attribute_from_tagset (info, "Embedded::Image::Orientation", _ORIENTATION_TAG_NAMES);
}


static const char *
get_exif_default_category (const Exiv2::Exifdatum &md)
{
	if (Exiv2::ExifTags::isMakerIfd(md.ifdId()))
		return "Exif::MakerNotes";

	switch (md.ifdId()) {
	case Exiv2::ifd1Id:
		return "Exif::Thumbnail";
	case Exiv2::gpsIfdId:
		return "Exif::GPS";
	case Exiv2::iopIfdId:
		return "Exif::Versions";
	default:
		break;
	}

	return "Exif::Other";
}


static void
exiv2_read_metadata (Exiv2::Image::AutoPtr  image,
		     GFileInfo             *info)
{
	image->readMetadata();

	Exiv2::ExifData &exifData = image->exifData();
	if (! exifData.empty()) {
		Exiv2::ExifData::const_iterator end = exifData.end();
		for (Exiv2::ExifData::const_iterator md = exifData.begin(); md != end; ++md) {
			stringstream raw_value;
			raw_value << md->value();

			stringstream description;
			if (! md->tagLabel().empty())
				description << md->tagLabel();
			else if (md->ifdId () > Exiv2::ifd1Id)
				// Must be a MakerNote - include group name
				description << md->groupName() << "." << md->tagName();
			else
				// Normal exif tag - just use tag name
				description << md->tagName();

			set_file_info (info,
				       md->key().c_str(),
				       description.str().c_str(),
				       md->print().c_str(),
				       raw_value.str().c_str(),
				       get_exif_default_category (*md),
				       md->typeName());
		}
	}

	Exiv2::IptcData &iptcData = image->iptcData();
	if (! iptcData.empty()) {
		Exiv2::IptcData::iterator end = iptcData.end();
		for (Exiv2::IptcData::iterator md = iptcData.begin(); md != end; ++md) {
			stringstream raw_value;
			raw_value << md->value();

			stringstream description;
			if (! md->tagLabel().empty())
				description << md->tagLabel();
			else
				description << md->tagName();

			set_file_info (info,
				       md->key().c_str(),
				       description.str().c_str(),
				       md->print().c_str(),
				       raw_value.str().c_str(),
				       "Iptc",
				       md->typeName());
		}
	}

	Exiv2::XmpData &xmpData = image->xmpData();
	if (! xmpData.empty()) {
		Exiv2::XmpData::iterator end = xmpData.end();
		for (Exiv2::XmpData::iterator md = xmpData.begin(); md != end; ++md) {
			stringstream raw_value;
			raw_value << md->value();

			stringstream description;
			if (! md->tagLabel().empty())
				description << md->tagLabel();
			else
				description << md->groupName() << "." << md->tagName();

			set_file_info (info,
				       md->key().c_str(),
				       description.str().c_str(),
				       md->print().c_str(),
				       raw_value.str().c_str(),
				       "Xmp::Embedded",
				       md->typeName());
		}
	}

	set_attributes_from_tagsets (info);
}


/*
 * exiv2_read_metadata_from_file
 * reads metadata from image files
 * code relies heavily on example1 from the exiv2 website
 * http://www.exiv2.org/example1.html
 */
extern "C"
gboolean
exiv2_read_metadata_from_file (GFile      *file,
			       GFileInfo  *info,
			       GError    **error)
{
	try {
		char *path;

		path = g_file_get_path (file);
		if (path == NULL) {
			if (error != NULL)
				*error = g_error_new_literal (G_IO_ERROR, G_IO_ERROR_FAILED, _("Invalid file format"));
			return FALSE;
		}

		Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(path);
		g_free (path);

		if (image.get() == 0) {
			if (error != NULL)
				*error = g_error_new_literal (G_IO_ERROR, G_IO_ERROR_FAILED, _("Invalid file format"));
			return FALSE;
		}

		exiv2_read_metadata (image, info);
	}
	catch (Exiv2::AnyError& e) {
		if (error != NULL)
			*error = g_error_new_literal (G_IO_ERROR, G_IO_ERROR_FAILED, e.what());
		return FALSE;
	}

	return TRUE;
}


extern "C"
gboolean
exiv2_read_metadata_from_buffer (void       *buffer,
				 gsize       buffer_size,
				 GFileInfo  *info,
				 GError    **error)
{
	try {
		Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open ((Exiv2::byte*) buffer, buffer_size);

		if (image.get() == 0) {
			if (error != NULL)
				*error = g_error_new_literal (G_IO_ERROR, G_IO_ERROR_FAILED, _("Invalid file format"));
			return FALSE;
		}

		exiv2_read_metadata (image, info);
	}
	catch (Exiv2::AnyError& e) {
		if (error != NULL)
			*error = g_error_new_literal (G_IO_ERROR, G_IO_ERROR_FAILED, e.what());
		return FALSE;
	}

	return TRUE;
}


extern "C"
gboolean
exiv2_read_sidecar (GFile     *file,
		    GFileInfo *info)
{
	try {
		char *path;

		path = g_file_get_path (file);
		if (path == NULL)
			return FALSE;

		Exiv2::DataBuf buf = Exiv2::readFile(path);
		g_free (path);

		std::string xmpPacket;
		xmpPacket.assign(reinterpret_cast<char*>(buf.pData_), buf.size_);
		Exiv2::XmpData xmpData;

		if (0 != Exiv2::XmpParser::decode(xmpData, xmpPacket))
			return FALSE;

		if (! xmpData.empty()) {
			Exiv2::XmpData::iterator end = xmpData.end();
			for (Exiv2::XmpData::iterator md = xmpData.begin(); md != end; ++md) {
				stringstream raw_value;
				raw_value << md->value();

				stringstream description;
				if (! md->tagLabel().empty())
					description << md->tagLabel();
				else
					description << md->groupName() << "." << md->tagName();

				set_file_info (info,
					       md->key().c_str(),
					       description.str().c_str(),
					       md->print().c_str(),
					       raw_value.str().c_str(),
					       "Xmp::Sidecar",
					       md->typeName());
			}
		}
		Exiv2::XmpParser::terminate();

		set_attributes_from_tagsets (info);
	}
	catch (Exiv2::AnyError& e) {
		std::cerr << "Caught Exiv2 exception '" << e << "'\n";
		return FALSE;
	}

	return TRUE;
}


static void
mandatory_int (Exiv2::ExifData &checkdata,
	       const char      *tag,
	       int              value)
{
	Exiv2::ExifKey key = Exiv2::ExifKey(tag);
	if (checkdata.findKey(key) == checkdata.end())
		checkdata[tag] = value;
}


static void
mandatory_string (Exiv2::ExifData &checkdata,
		  const char      *tag,
		  const char      *value)
{
	Exiv2::ExifKey key = Exiv2::ExifKey(tag);
	if (checkdata.findKey(key) == checkdata.end())
		checkdata[tag] = value;
}


const char *
gth_main_get_metadata_type (GthMetadata *metadata,
			    const char  *key)
{
	const char      *value_type;
	GthMetadataInfo *metadatum_info;

	value_type = gth_metadata_get_value_type (metadata);
	if (g_strcmp0 (value_type, "Undefined") == 0)
			value_type = NULL;

	if (value_type != NULL)
		return value_type;

	metadatum_info = gth_main_get_metadata_info (key);
	if (metadatum_info != NULL)
		value_type = metadatum_info->type;

	return value_type;
}


static Exiv2::DataBuf
exiv2_write_metadata_private (Exiv2::Image::AutoPtr  image,
			      GFileInfo             *info,
			      GdkPixbuf             *pixbuf)
{
	char **attributes;
	int    i;

	image->clearMetadata();

	// EXIF Data

	Exiv2::ExifData ed;
	attributes = g_file_info_list_attributes (info, "Exif");
	for (i = 0; attributes[i] != NULL; i++) {
		GthMetadata *metadatum;
		char *key;

		metadatum = (GthMetadata *) g_file_info_get_attribute_object (info, attributes[i]);
		key = exiv2_key_from_attribute (attributes[i]);

		try {
			/* If the metadatum has no value yet, a new empty value
			 * is created. The type is taken from Exiv2's tag
			 * lookup tables. If the tag is not found in the table,
			 * the type defaults to ASCII.
			 * We always create the metadatum explicitly if the
			 * type is available to avoid type errors.
			 * See bug #610389 for more details.  The original
			 * explanation is here:
			 * http://uk.groups.yahoo.com/group/exiv2/message/1472
			 */

			const char *raw_value = gth_metadata_get_raw (metadatum);
			const char *value_type = gth_main_get_metadata_type (metadatum, key);

			if ((raw_value != NULL) && (strcmp (raw_value, "") != 0) &&  (value_type != NULL)) {
				Exiv2::Value::AutoPtr value = Exiv2::Value::create (Exiv2::TypeInfo::typeId (value_type));
				value->read (raw_value);
				Exiv2::ExifKey exif_key(key);
				ed.add (exif_key, value.get());
			}
		}
		catch (Exiv2::AnyError& e) {
			/* we don't care about invalid key errors */
			g_warning ("%s", e.what());
		}

		g_free (key);
	}
	g_strfreev (attributes);

	// Mandatory tags - add if not already present

	mandatory_int (ed, "Exif.Image.XResolution", 72);
	mandatory_int (ed, "Exif.Image.YResolution", 72);
	mandatory_int (ed, "Exif.Image.ResolutionUnit", 2);
	mandatory_int (ed, "Exif.Image.YCbCrPositioning", 1);
	mandatory_int (ed, "Exif.Photo.ColorSpace", 1);
	mandatory_string (ed, "Exif.Photo.ExifVersion", "48 50 50 49");
	mandatory_string (ed, "Exif.Photo.ComponentsConfiguration", "1 2 3 0");
	mandatory_string (ed, "Exif.Photo.FlashpixVersion", "48 49 48 48");

	// Overwrite the software tag

	ed["Exif.Image.Software"] = PACKAGE " " VERSION;

	// Update the dimension tags with actual image values

	int width = 0;
	int height = 0;

	if (pixbuf != NULL) {
		width = gdk_pixbuf_get_width (pixbuf);
		if (width > 0)
			ed["Exif.Photo.PixelXDimension"] = width;

		height = gdk_pixbuf_get_height (pixbuf);
		if (height > 0)
			ed["Exif.Photo.PixelYDimension"] = height;

		ed["Exif.Image.Orientation"] = 1;
	}

	// Update the thumbnail

	Exiv2::ExifThumb thumb(ed);
	if ((pixbuf != NULL) && (width > 0) && (height > 0)) {
		GdkPixbuf *thumb_pixbuf;
		char      *buffer;
		gsize      buffer_size;

		scale_keeping_ratio (&width, &height, 128, 128, FALSE);
		thumb_pixbuf = _gdk_pixbuf_scale_simple_safe (pixbuf, width, height, GDK_INTERP_BILINEAR);
		if (gdk_pixbuf_save_to_buffer (thumb_pixbuf, &buffer, &buffer_size, "jpeg", NULL, NULL)) {
			thumb.setJpegThumbnail ((Exiv2::byte*) buffer, buffer_size);
			ed["Exif.Thumbnail.XResolution"] = 72;
			ed["Exif.Thumbnail.YResolution"] = 72;
			ed["Exif.Thumbnail.ResolutionUnit"] =  2;
			g_free (buffer);
		}
		else
			thumb.erase();

		g_object_unref (thumb_pixbuf);
	}
	else
		thumb.erase();

	// Update the DateTime tag

	if (g_file_info_get_attribute_object (info, "Exif::Image::DateTime") == NULL) {
		GTimeVal current_time;
		g_get_current_time (&current_time);
		char *date_time = _g_time_val_to_exif_date (&current_time);
		ed["Exif.Image.DateTime"] = date_time;
		g_free (date_time);
	}

	// IPTC Data

	Exiv2::IptcData id;
	attributes = g_file_info_list_attributes (info, "Iptc");
	for (i = 0; attributes[i] != NULL; i++) {
		GthMetadata *metadatum = (GthMetadata *) g_file_info_get_attribute_object (info, attributes[i]);
		char *key = exiv2_key_from_attribute (attributes[i]);

		try {
			const char *raw_value = gth_metadata_get_raw (metadatum);
			const char *value_type = gth_main_get_metadata_type (metadatum, key);

			if ((raw_value != NULL) && (strcmp (raw_value, "") != 0) &&  (value_type != NULL)) {
				/* See the exif data code above for an explanation. */
				Exiv2::Value::AutoPtr value = Exiv2::Value::create (Exiv2::TypeInfo::typeId (value_type));
				value->read (raw_value);
				Exiv2::IptcKey iptc_key(key);
				id.add (iptc_key, value.get());
			}
		}
		catch (Exiv2::AnyError& e) {
			/* we don't care about invalid key errors */
			g_warning ("%s", e.what());
		}

		g_free (key);
	}
	g_strfreev (attributes);

	// XMP Data

	Exiv2::XmpData xd;
	attributes = g_file_info_list_attributes (info, "Xmp");
	for (i = 0; attributes[i] != NULL; i++) {
		GthMetadata *metadatum = (GthMetadata *) g_file_info_get_attribute_object (info, attributes[i]);
		char *key = exiv2_key_from_attribute (attributes[i]);

		// Remove existing tags of the same type.
		// Seems to be needed for storing category keywords.
		// Not exactly sure why!
		Exiv2::XmpData::iterator iter = xd.findKey (Exiv2::XmpKey (key));
		if (iter != xd.end ())
			xd.erase (iter);

		try {
			const char *raw_value = gth_metadata_get_raw (metadatum);
			const char *value_type = gth_main_get_metadata_type (metadatum, key);

			if ((raw_value != NULL) && (strcmp (raw_value, "") != 0) &&  (value_type != NULL)) {
				/* See the exif data code above for an explanation. */
				Exiv2::Value::AutoPtr value = Exiv2::Value::create (Exiv2::TypeInfo::typeId (value_type));
				value->read (raw_value);
				Exiv2::XmpKey xmp_key(key);
				xd.add (xmp_key, value.get());
			}
		}
		catch (Exiv2::AnyError& e) {
			/* we don't care about invalid key errors */
			g_warning ("%s", e.what());
		}

		g_free (key);
	}
	g_strfreev (attributes);

	try {
		image->setExifData(ed);
		image->setIptcData(id);
		image->setXmpData(xd);
		image->writeMetadata();
	}
	catch (Exiv2::AnyError& e) {
		g_warning ("%s", e.what());
	}

	Exiv2::BasicIo &io = image->io();
	io.open();

	return io.read(io.size());
}


extern "C"
gboolean
exiv2_supports_writes (const char *mime_type)
{
	return (g_content_type_equals (mime_type, "image/jpeg")
#if HAVE_EXIV2_020
		|| g_content_type_equals (mime_type, "image/tiff")
#endif
		|| g_content_type_equals (mime_type, "image/png"));
}


extern "C"
gboolean
exiv2_write_metadata (SavePixbufData *data)
{
	if (exiv2_supports_writes (data->mime_type)) {
		try {
			Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open ((Exiv2::byte*) data->buffer, data->buffer_size);
			g_assert (image.get() != 0);

			Exiv2::DataBuf buf = exiv2_write_metadata_private (image, data->file_data->info, data->pixbuf);

			g_free (data->buffer);
			data->buffer = g_memdup (buf.pData_, buf.size_);
			data->buffer_size = buf.size_;
		}
		catch (Exiv2::AnyError& e) {
			if (data->error != NULL)
				*data->error = g_error_new_literal (G_IO_ERROR, G_IO_ERROR_FAILED, e.what());
			g_warning ("%s\n", e.what());
			return FALSE;
		}
	}

	return TRUE;
}


extern "C"
gboolean
exiv2_write_metadata_to_buffer (void      **buffer,
				gsize      *buffer_size,
				GFileInfo  *info,
				GdkPixbuf  *pixbuf,
				GError    **error)
{
	try {
		Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open ((Exiv2::byte*) *buffer, *buffer_size);
		g_assert (image.get() != 0);

		Exiv2::DataBuf buf = exiv2_write_metadata_private (image, info, pixbuf);

		g_free (*buffer);
		*buffer = g_memdup (buf.pData_, buf.size_);
		*buffer_size = buf.size_;
	}
	catch (Exiv2::AnyError& e) {
		if (error != NULL)
			*error = g_error_new_literal (G_IO_ERROR, G_IO_ERROR_FAILED, e.what());
		return FALSE;
	}

	return TRUE;
}


#define MAX_RATIO_ERROR_TOLERANCE 0.01


GdkPixbuf *
exiv2_generate_thumbnail (const char *uri,
			  const char *mime_type,
			  int         requested_size)
{
	GdkPixbuf *pixbuf = NULL;

	if (! _g_content_type_is_a (mime_type, "image/jpeg")
	    && ! _g_content_type_is_a (mime_type, "image/tiff"))
	{
		return NULL;
	}

	try {
		char *path;

		path = g_filename_from_uri (uri, NULL, NULL);
		if (path != NULL) {
			Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open (path);
			image->readMetadata ();
			Exiv2::ExifThumbC exifThumb (image->exifData ());
			Exiv2::DataBuf thumb = exifThumb.copy ();

			if (thumb.pData_ != NULL) {
				Exiv2::ExifData &ed = image->exifData();

				long orientation = ed["Exif.Image.Orientation"].toLong();
				long image_width = ed["Exif.Photo.PixelXDimension"].toLong();
				long image_height = ed["Exif.Photo.PixelYDimension"].toLong();

				if ((orientation == 1) && (image_width > 0) && (image_height > 0)) {
					GInputStream *stream = g_memory_input_stream_new_from_data (thumb.pData_, thumb.size_, NULL);
					pixbuf = gdk_pixbuf_new_from_stream (stream, NULL, NULL);

					if (pixbuf != NULL) {
						/* Heuristic to find out-of-date thumbnails: the thumbnail and image aspect ratios must be equal */

						double image_ratio = (((double) image_width) / image_height);
						double thumbnail_ratio = (((double) gdk_pixbuf_get_width (pixbuf)) / gdk_pixbuf_get_height (pixbuf));
						double ratio_delta = (image_ratio > thumbnail_ratio) ? (image_ratio - thumbnail_ratio) : (thumbnail_ratio - image_ratio);

						if (ratio_delta > MAX_RATIO_ERROR_TOLERANCE) {
							g_object_unref (pixbuf);
							pixbuf = NULL;
						}
						else {
							/* Save the original image size in the pixbuf options */

							char *s = g_strdup_printf ("%ld", image_width);
							gdk_pixbuf_set_option (pixbuf, "tEXt::Thumb::Image::Width", s);
							g_object_set_data (G_OBJECT (pixbuf), "gnome-original-width", GINT_TO_POINTER ((int) image_width));
							g_free (s);

							s = g_strdup_printf ("%ld", image_height);
							gdk_pixbuf_set_option (pixbuf, "tEXt::Thumb::Image::Height", s);
							g_object_set_data (G_OBJECT (pixbuf), "gnome-original-height", GINT_TO_POINTER ((int) image_height));
							g_free (s);

							/* Set the orientation option to correctly rotate the thumbnail
							 * in gnome_desktop_thumbnail_factory_generate_thumbnail() */

							long  orientation = ed["Exif.Image.Orientation"].toLong();
							char *orientation_s = g_strdup_printf ("%ld", orientation);
							gdk_pixbuf_set_option (pixbuf, "orientation", orientation_s);
							g_free (orientation_s);
						}
					}

					g_object_unref (stream);
				}
			}

			g_free (path);
		}
	}
	catch (Exiv2::AnyError& e) {
	}

	return pixbuf;
}
