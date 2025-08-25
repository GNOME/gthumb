#include <config.h>
#include <glib.h>
#include <exiv2/basicio.hpp>
#include <exiv2/error.hpp>
#include <exiv2/image.hpp>
#include <exiv2/exif.hpp>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <iomanip>
#include <exiv2/exiv2.hpp>
#include <exiv2/xmp_exiv2.hpp>
#include "lib/exiv2-utils.h"
#include "lib/gth-string-list.h"
#include "lib/gth-metadata.h"
#include "lib/util.h"
#include "lib/io/save-jpeg.h"

#define INVALID_VALUE N_("(invalid value)")
#define EXPOSURE_SEPARATOR " · "

using namespace std;

// Some bits of information may be contained in more than one metadata tag.
// The arrays below define the valid tags for a particular piece of
// information, in decreasing order of preference (best one first).

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

const char *_LAST_DATE_TAG_NAMES[] = {
	"Exif::Image::DateTime",
	"Xmp::exif::DateTime",
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
	"Iptc::Application2::Caption",
	"Xmp::dc::description",
	"Exif::Photo::UserComment",
	"Exif::Image::ImageDescription",
	"Xmp::tiff::ImageDescription",
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
	"Iptc::Application2::Keywords",
	"Xmp::iptc::Keywords",
	"Xmp::dc::subject",
	NULL
};

const char *_RATING_TAG_NAMES[] = {
	"Xmp::xmp::Rating",
	NULL
};

const char *_AUTHOR_TAG_NAMES[] = {
	"Exif::Image::Artist",
	"Iptc::Application2::Byline",
	"Xmp::dc::creator",
	"Xmp::xmpDM::artist",
	"Xmp::tiff::Artist",
	"Xmp::plus::ImageCreator",
	"Xmp::plus::ImageCreatorName",
	NULL
};

const char *_COPYRIGHT_TAG_NAMES[] = {
	"Exif::Image::Copyright",
	"Iptc::Application2::Copyright",
	"Xmp::dc::rights",
	"Xmp::xmpDM::copyright",
	"Xmp::tiff::Copyright",
	"Xmp::plus::CopyrightOwner",
	"Xmp::plus::CopyrightOwnerName",
	NULL
};

// Some cameras fill in the ImageDescription or UserComment fields
// with useless fluff. Try to filter these out, so they do not show up
// as comments.
const char *useless_comment_filter[] = {
	"OLYMPUS DIGITAL CAMERA",
	"SONY DSC",
	"KONICA MINOLTA DIGITAL CAMERA",
	"MINOLTA DIGITAL CAMERA",
	"binary comment",
	NULL
};


static inline char * exiv2_key_from_attribute (const char *attribute) {
	return _g_utf8_replace (attribute, "::", ".");
}


static inline char * exiv2_key_to_attribute (const char *key) {
	return _g_utf8_replace (key, ".", "::");
}


static gboolean attribute_is_date (const char *key) {
	if (key == NULL)
		return FALSE;
	for (int i = 0; _DATE_TAG_NAMES[i] != NULL; i++) {
		if (strcmp (_DATE_TAG_NAMES[i], key) == 0)
			return TRUE;
	}
	return FALSE;
}


static GthMetadata * create_metadata (
	const char *key,
	const char *description,
	const char *formatted_value,
	const char *raw_value,
	const char *category,
	const char *type_name)
{
	char *formatted_value_utf8 = _g_utf8_from_any (formatted_value);
	if (_g_utf8_all_spaces (formatted_value_utf8))
		return NULL;

	char *description_utf8 = _g_utf8_from_any (description);

	char *attribute = exiv2_key_to_attribute (key);
	if (attribute_is_date (attribute)) {
		g_free (formatted_value_utf8);
		formatted_value_utf8 = NULL;

		GDateTime *datetime = _g_date_time_new_from_exif_date (raw_value);
		if (datetime != NULL) {
			formatted_value_utf8 = g_date_time_format (datetime, "%x %X");
			g_date_time_unref (datetime);
		}
		else {
			formatted_value_utf8 = g_locale_to_utf8 (formatted_value, -1, NULL, NULL, NULL);
		}
	}
	else {
		char *tmp = _g_utf8_remove_string_properties (formatted_value_utf8);
		g_free (formatted_value_utf8);
		formatted_value_utf8 = tmp;
	}

	if (formatted_value_utf8 == NULL)
		formatted_value_utf8 = g_strdup (INVALID_VALUE);

	GthMetadataInfo *metadata_info = gth_metadata_info_get (attribute);
	if ((metadata_info == NULL) && (category != NULL)) {
		metadata_info = gth_metadata_info_register (
			attribute,
			description_utf8,
			category,
			GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW,
			type_name
		);
		metadata_info->sort_order = 500;
	}

	if ((metadata_info != NULL) && (metadata_info->type == NULL) && (type_name != NULL))
		metadata_info->type = g_strdup (type_name);

	if ((metadata_info != NULL) && (metadata_info->display_name == NULL) && (description_utf8 != NULL))
		metadata_info->display_name = g_strdup (description_utf8);

	GthMetadata *metadata = gth_metadata_new ();
	g_object_set (metadata,
		"id", key,
		"description", description_utf8,
		"formatted", formatted_value_utf8,
		"raw", raw_value,
		"value-type", type_name,
		NULL);

	g_free (formatted_value_utf8);
	g_free (description_utf8);
	g_free (attribute);

	return metadata;
}


static void add_string_list_to_metadata (GthMetadata *metadata, const Exiv2::Metadatum &value) {
	GList *list = NULL;
	for (guint i = 0; i < value.count(); i++)
		list = g_list_prepend (list, g_strdup (value.toString(i).c_str()));
	GthStringList *string_list = gth_string_list_new (g_list_reverse (list));
	g_object_set (metadata, "string-list", string_list, NULL);

	g_object_unref (string_list);
	_g_string_list_free (list);
}


static void set_file_info (
	GFileInfo *info,
	const char *key,
	const char *description,
	const char *formatted_value,
	const char *raw_value,
	const char *category,
	const char *type_name)
{
	char *attribute = exiv2_key_to_attribute (key);
	GthMetadata *metadata = create_metadata (key, description,
		formatted_value, raw_value, category, type_name);
	if (metadata != NULL) {
		g_file_info_set_attribute_object (info, attribute, G_OBJECT (metadata));
		g_object_unref (metadata);
	}
	g_free (attribute);
}


static GHashTable * create_metadata_hash (void) {
	return g_hash_table_new_full (
		g_str_hash,
		g_str_equal,
		g_free,
		g_object_unref);
}


static void add_metadata_to_hash (GHashTable *table, GthMetadata *metadata) {
	if (metadata == NULL)
		return;

	char *key = exiv2_key_to_attribute (gth_metadata_get_id (metadata));
	gpointer object = g_hash_table_lookup (table, key);
	if (object != NULL) {
		GthStringList *string_list;
		GList *list;

		string_list = NULL;
		switch (gth_metadata_get_data_type (GTH_METADATA (object))) {
		case GTH_METADATA_TYPE_STRING:
			string_list = gth_string_list_new (NULL);
			list = g_list_append (NULL, g_strdup (gth_metadata_get_formatted (GTH_METADATA (object))));
			gth_string_list_set_list (string_list, list);
			break;

		case GTH_METADATA_TYPE_STRING_LIST:
			string_list = (GthStringList *) g_object_ref (gth_metadata_get_string_list (GTH_METADATA (object)));
			break;

		case GTH_METADATA_TYPE_POINT:
			break;
		}

		if (string_list == NULL) {
			g_hash_table_insert (table, g_strdup (key), g_object_ref (metadata));
			return;
		}

		switch (gth_metadata_get_data_type (metadata)) {
		case GTH_METADATA_TYPE_STRING:
			list = gth_string_list_get_list (string_list);
			list = g_list_append (list, g_strdup (gth_metadata_get_formatted (metadata)));
			gth_string_list_set_list (string_list, list);
			break;

		case GTH_METADATA_TYPE_STRING_LIST:
			gth_string_list_concat (string_list, gth_metadata_get_string_list (metadata));
			break;

		case GTH_METADATA_TYPE_POINT:
			break;
		}

		g_object_set (metadata, "string-list", string_list, NULL);
		g_hash_table_replace (table, g_strdup (key), g_object_ref (metadata));

		g_object_unref (string_list);
	}
	else {
		g_hash_table_insert (table, g_strdup (key), g_object_ref (metadata));
	}

	g_free (key);
}


static void set_file_info_from_hash (GFileInfo *info, GHashTable *table) {
	GHashTableIter iter;
	gpointer key;
	gpointer value;

	g_hash_table_iter_init (&iter, table);
	while (g_hash_table_iter_next (&iter, &key, &value)) {
		g_file_info_set_attribute_object (info, (char *)key, G_OBJECT (value));
	}
}


static void set_attribute_from_metadata (GFileInfo *info, const char *attribute, GObject *metadata) {
	if (metadata == NULL)
		return;

	char *description;
	char *formatted_value;
	char *raw_value;
	char *type_name;
	g_object_get (
		metadata,
		"description", &description,
		"formatted", &formatted_value,
		"raw", &raw_value,
		"value-type", &type_name,
		NULL);

	set_file_info (
		info,
		attribute,
		description,
		formatted_value,
		raw_value,
		NULL,
		type_name);

	g_free (description);
	g_free (formatted_value);
	g_free (raw_value);
	g_free (type_name);
}


static GObject * get_attribute_from_tagset (GFileInfo *info, const char *tagset[]) {
	GObject *metadata = NULL;
	for (int i = 0; tagset[i] != NULL; i++) {
		metadata = g_file_info_get_attribute_object (info, tagset[i]);
		if (metadata != NULL)
			return metadata;
	}
	return NULL;
}


static void set_attribute_from_tagset (GFileInfo *info, const char *attribute, const char *tagset[]) {
	GObject *metadata = get_attribute_from_tagset (info, tagset);
	if (metadata != NULL) {
		set_attribute_from_metadata (info, attribute, metadata);
	}
}


static void set_string_list_attribute_from_tagset (GFileInfo *info, const char *attribute, const char *tagset[]) {
	GObject *metadata = NULL;
	for (int i = 0; tagset[i] != NULL; i++) {
		metadata = g_file_info_get_attribute_object (info, tagset[i]);
		if (metadata != NULL)
			break;
	}

	if (metadata == NULL)
		return;

	if (GTH_IS_METADATA (metadata)
		&& (gth_metadata_get_data_type (GTH_METADATA (metadata)) != GTH_METADATA_TYPE_STRING_LIST))
	{
		char *raw;
		g_object_get (metadata, "raw", &raw, NULL);

		char *utf8_raw = _g_utf8_try_from_any (raw);
		if (utf8_raw == NULL) {
			return;
		}

		char **keywords = g_strsplit (utf8_raw, ",", -1);
		GthStringList *string_list = gth_string_list_new_from_strv (keywords);
		metadata = (GObject *) gth_metadata_new_for_string_list (string_list);
		g_file_info_set_attribute_object (info, attribute, metadata);

		g_object_unref (metadata);
		g_object_unref (string_list);
		g_strfreev (keywords);
		g_free (raw);
		g_free (utf8_raw);
	}
	else {
		g_file_info_set_attribute_object (info, attribute, metadata);
	}
}


static void clear_useless_comments_from_tagset (GFileInfo *info, const char *tagset[]) {
	for (int i = 0; tagset[i] != NULL; i++) {
		GObject *metadata = g_file_info_get_attribute_object (info, tagset[i]);
		if ((metadata == NULL) || ! GTH_IS_METADATA (metadata))
			continue;

		const char *value = gth_metadata_get_formatted (GTH_METADATA (metadata));
		for (int j = 0; useless_comment_filter[j] != NULL; j++) {
			if (strstr (value, useless_comment_filter[j]) == value) {
				g_file_info_remove_attribute (info, tagset[i]);
				break;
			}
		}
	}
}


static void exiv2_update_general_attributes (GFileInfo *info) {
	set_attribute_from_tagset (info, "Metadata::DateTime", _ORIGINAL_DATE_TAG_NAMES);
	set_attribute_from_tagset (info, "Metadata::Description", _DESCRIPTION_TAG_NAMES);
	set_attribute_from_tagset (info, "Metadata::Title", _TITLE_TAG_NAMES);

	// if iptc::caption and iptc::headline are different use iptc::headline
	// to set general::title, if not already set.

	if (g_file_info_get_attribute_object (info, "Metadata::Title") == NULL) {
		GObject *iptc_caption;
		GObject *iptc_headline;

		iptc_caption = g_file_info_get_attribute_object (info, "Iptc::Application2::Caption");
		iptc_headline = g_file_info_get_attribute_object (info, "Iptc::Application2::Headline");

		if ((iptc_caption != NULL)
		    && (iptc_headline != NULL)
		    && (g_strcmp0 (gth_metadata_get_raw (GTH_METADATA (iptc_caption)),
				   gth_metadata_get_raw (GTH_METADATA (iptc_headline))) != 0))
		{
			set_attribute_from_metadata (info, "Metadata::Title", iptc_headline);
		}
	}

	set_attribute_from_tagset (info, "Metadata::Location", _LOCATION_TAG_NAMES);
	set_string_list_attribute_from_tagset (info, "Metadata::Tags", _KEYWORDS_TAG_NAMES);
	set_attribute_from_tagset (info, "Metadata::Rating", _RATING_TAG_NAMES);
}


static void set_attributes_from_tagsets (GFileInfo *info, gboolean update_general_attributes) {
	clear_useless_comments_from_tagset (info, _DESCRIPTION_TAG_NAMES);
	clear_useless_comments_from_tagset (info, _TITLE_TAG_NAMES);

	if (update_general_attributes) {
		exiv2_update_general_attributes (info);
	}

	set_attribute_from_tagset (info, "Photo::DateTimeOriginal", _ORIGINAL_DATE_TAG_NAMES);
	set_attribute_from_tagset (info, "Photo::Orientation", _ORIENTATION_TAG_NAMES);

	set_attribute_from_tagset (info, "Photo::Aperture", _APERTURE_TAG_NAMES);
	set_attribute_from_tagset (info, "Photo::ISOSpeed", _ISOSPEED_TAG_NAMES);
	set_attribute_from_tagset (info, "Photo::ExposureTime", _EXPOSURE_TIME_TAG_NAMES);
	set_attribute_from_tagset (info, "Photo::ShutterSpeed", _SHUTTER_SPEED_TAG_NAMES);
	set_attribute_from_tagset (info, "Photo::FocalLength", _FOCAL_LENGTH_TAG_NAMES);
	set_attribute_from_tagset (info, "Photo::Flash", _FLASH_TAG_NAMES);
	set_attribute_from_tagset (info, "Metadata::Author", _AUTHOR_TAG_NAMES);
	set_attribute_from_tagset (info, "Metadata::Copyright", _COPYRIGHT_TAG_NAMES);

	GObject *make_metadata = get_attribute_from_tagset (info, _MAKE_TAG_NAMES);
	if (make_metadata != NULL) {
		GObject *model_metadata = get_attribute_from_tagset (info, _MODEL_TAG_NAMES);
		if (model_metadata != NULL) {
			char *make_formatted_value;
			char *make_raw_value;
			char *make_type_name;
			g_object_get (make_metadata, "formatted", &make_formatted_value, "raw", &make_raw_value, "value-type", &make_type_name, NULL);

			char *model_formatted_value;
			char *model_raw_value;
			char *model_type_name;
			g_object_get (model_metadata, "formatted", &model_formatted_value, "raw", &model_raw_value, "value-type", &model_type_name, NULL);

			char *make_first_word = strchrnul (make_formatted_value, ' ');
			char *model_first_word = strchrnul (model_formatted_value, ' ');
			int make_first_word_len = make_first_word - make_formatted_value;
			int model_first_word_len = model_first_word - model_formatted_value;
			gboolean model_contains_maker = ((make_first_word_len == model_first_word_len)
				&& (strncmp (model_formatted_value, make_formatted_value, model_first_word_len) == 0));

			GString *full_formatted_value = g_string_new ("");
			if (!model_contains_maker) {
				g_string_append (full_formatted_value, make_formatted_value);
				g_string_append (full_formatted_value, " ");
			}
			g_string_append (full_formatted_value, model_formatted_value);

			GString *full_raw_value = g_string_new ("");
			if (!model_contains_maker) {
				g_string_append (full_raw_value, make_raw_value);
				g_string_append (full_raw_value, " ");
			}
			g_string_append (full_raw_value, model_raw_value);

			set_file_info (
				info,
				"Photo::CameraModel",
				NULL,
				full_formatted_value->str,
				full_raw_value->str,
				NULL,
				model_type_name);

			g_string_free (full_raw_value, TRUE);
			g_string_free (full_formatted_value, TRUE);
			g_free (model_type_name);
			g_free (model_raw_value);
			g_free (model_formatted_value);

			g_free (make_type_name);
			g_free (make_raw_value);
			g_free (make_formatted_value);
		}
	}

	/* Photo::Exposure */

	GObject *aperture;
	GObject *iso_speed;
	GObject *shutter_speed;
	GObject *exposure_time;
	GString *exposure;

	aperture = get_attribute_from_tagset (info, _APERTURE_TAG_NAMES);
	iso_speed = get_attribute_from_tagset (info, _ISOSPEED_TAG_NAMES);
	shutter_speed = get_attribute_from_tagset (info, _SHUTTER_SPEED_TAG_NAMES);
	exposure_time = get_attribute_from_tagset (info, _EXPOSURE_TIME_TAG_NAMES);

	exposure = g_string_new ("");

	if (aperture != NULL) {
		char *formatted_value;

		g_object_get (aperture, "formatted", &formatted_value, NULL);
		if (formatted_value != NULL) {
			g_string_append (exposure, formatted_value);
			g_free (formatted_value);
		}
	}

	if (iso_speed != NULL) {
		char *formatted_value;

		g_object_get (iso_speed, "formatted", &formatted_value, NULL);
		if (formatted_value != NULL) {
			if (exposure->len > 0)
				g_string_append (exposure, EXPOSURE_SEPARATOR);
			g_string_append (exposure, "ISO ");
			g_string_append (exposure, formatted_value);
			g_free (formatted_value);
		}
	}

	if (shutter_speed != NULL) {
		char *formatted_value;

		g_object_get (shutter_speed, "formatted", &formatted_value, NULL);
		if (formatted_value != NULL) {
			if (exposure->len > 0)
				g_string_append (exposure, EXPOSURE_SEPARATOR);
			g_string_append (exposure, formatted_value);
			g_free (formatted_value);
		}
	}
	else if (exposure_time != NULL) {
		char *formatted_value;

		g_object_get (exposure_time, "formatted", &formatted_value, NULL);
		if (formatted_value != NULL) {
			if (exposure->len > 0)
				g_string_append (exposure, EXPOSURE_SEPARATOR);
			g_string_append (exposure, formatted_value);
			g_free (formatted_value);
		}
	}

	set_file_info (info,
		       "Photo::Exposure",
		       _("Exposure"),
		       exposure->str,
		       NULL,
		       NULL,
		       NULL);

	g_string_free (exposure, TRUE);

	// GPS Coordinates

	double latitude, longitude;
	if (exiv2_get_coordinates (info, &latitude, &longitude) == 2) {
		GthMetadata *metadata = gth_metadata_new_for_point (latitude, longitude);
		char *formatted = exiv2_decimal_coordinates_to_string (latitude, longitude);
		g_object_set (metadata,
			"id", "Metadata::Coordinates",
			"formatted", formatted,
			NULL);
		g_file_info_set_attribute_object (info, "Metadata::Coordinates", G_OBJECT (metadata));

		g_free (formatted);
		g_object_unref (metadata);
	}
}


static const char * get_exif_default_category (const Exiv2::Exifdatum &md) {
	if (Exiv2::ExifTags::isMakerGroup(md.groupName()))
		return "Exif::MakerNotes";
	if (md.groupName().compare("Thumbnail") == 0)
		return "Exif::Thumbnail";
	else if (md.groupName().compare("GPSInfo") == 0)
		return "Exif::GPS";
	else if (md.groupName().compare("Iop") == 0)
		return "Exif::Versions";
	return "Exif::Other";
}


static void exiv2_read_metadata (Exiv2::Image::UniquePtr image, GFileInfo *info, gboolean update_general_attributes) {
	image->readMetadata();

	Exiv2::ExifData &exifData = image->exifData();
	if (!exifData.empty()) {
		Exiv2::ExifData::const_iterator end = exifData.end();
		for (Exiv2::ExifData::const_iterator md = exifData.begin(); md != end; ++md) {
			stringstream raw_value;
			raw_value << md->value();

			stringstream description;
			if (!md->tagLabel().empty()) {
				description << md->tagLabel();
			}
			else if (Exiv2::ExifTags::isMakerGroup(md->groupName())) {
				// Must be a MakerNote - include group name
				description << md->groupName() << "." << md->tagName();
			}
			else {
				// Normal exif tag - just use tag name
				description << md->tagName();
			}

			set_file_info (info,
				md->key().c_str(),
				description.str().c_str(),
				md->print(&exifData).c_str(),
				raw_value.str().c_str(),
				get_exif_default_category (*md),
				md->typeName());
		}
	}

	Exiv2::IptcData &iptcData = image->iptcData();
	if (!iptcData.empty()) {
		GHashTable *table = create_metadata_hash ();

		Exiv2::IptcData::iterator end = iptcData.end();
		for (Exiv2::IptcData::iterator md = iptcData.begin(); md != end; ++md) {
			stringstream raw_value;
			raw_value << md->value();

			stringstream description;
			if (! md->tagLabel().empty())
				description << md->tagLabel();
			else
				description << md->tagName();

			GthMetadata *metadata = create_metadata (
				md->key().c_str(),
				description.str().c_str(),
				md->print().c_str(),
				raw_value.str().c_str(),
				"Iptc",
				md->typeName());
			if (metadata != NULL) {
				add_metadata_to_hash (table, metadata);
				g_object_unref (metadata);
			}
		}
		set_file_info_from_hash (info, table);
		g_hash_table_unref (table);
	}

	Exiv2::XmpData &xmpData = image->xmpData();
	if (!xmpData.empty()) {
		GHashTable *table = create_metadata_hash ();

		Exiv2::XmpData::iterator end = xmpData.end();
		for (Exiv2::XmpData::iterator md = xmpData.begin(); md != end; ++md) {
			stringstream raw_value;
			raw_value << md->value();

			stringstream description;
			if (! md->tagLabel().empty())
				description << md->tagLabel();
			else
				description << md->groupName() << "." << md->tagName();

			GthMetadata *metadata = create_metadata (
				md->key().c_str(),
				description.str().c_str(),
				md->print().c_str(),
				raw_value.str().c_str(),
				"Xmp::Embedded",
				md->typeName());
			if (metadata != NULL) {
				if ((g_strcmp0 (md->typeName(), "XmpBag") == 0)
				    || (g_strcmp0 (md->typeName(), "XmpSeq") == 0))
				{
					add_string_list_to_metadata (metadata, *md);
				}
				add_metadata_to_hash (table, metadata);
				g_object_unref (metadata);
			}
		}

		set_file_info_from_hash (info, table);
		g_hash_table_unref (table);
	}

	set_attributes_from_tagsets (info, update_general_attributes);
}


extern "C"
gboolean exiv2_read_metadata_from_buffer (GBytes *bytes, GFileInfo *info, gboolean update_general_attributes, GError **error) {
	try {
		gsize buffer_size;
		gconstpointer buffer = g_bytes_get_data (bytes, &buffer_size);
		Exiv2::Image::UniquePtr image = Exiv2::ImageFactory::open ((Exiv2::byte*) buffer, buffer_size);
		if (image.get() == 0) {
			g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED, _("Invalid file format"));
			return FALSE;
		}
		exiv2_read_metadata (std::move(image), info, update_general_attributes);
		return TRUE;
	}
	catch (Exiv2::Error& e) {
		g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED, e.what ());
		return FALSE;
	}
}

// Exif format: %d/%d %d/%d %d/%d
static double exif_coordinate_to_decimal (const char *raw) {
	double value = 0.0;
	double factor = 1.0;
	char **raw_values = g_strsplit (raw, " ", 3);
	for (int i = 0; i < 3; i++) {
		if (raw_values[i] == NULL)
			break; // Error
		int numerator;
		int denominator;
		sscanf (raw_values[i], "%d/%d", &numerator, &denominator);
		if ((numerator != 0) && (denominator != 0))
			value += ((double) numerator / denominator) / factor;
		factor *= 60.0;
	}
	g_strfreev (raw_values);
	return value;
}

extern "C"
int exiv2_get_coordinates (GFileInfo *info, double *out_latitude, double *out_longitude) {
	double latitude = 0.0;
	double longitude = 0.0;
	int coordinates_available = 0;
	if (info != NULL) {
		GthMetadata *metadata = (GthMetadata *) g_file_info_get_attribute_object (info, "Exif::GPSInfo::GPSLatitude");
		if (metadata != NULL) {
			latitude = exif_coordinate_to_decimal (gth_metadata_get_raw (metadata));

			metadata = (GthMetadata *) g_file_info_get_attribute_object (info, "Exif::GPSInfo::GPSLatitudeRef");
			if (metadata != NULL) {
				if (g_strcmp0 (gth_metadata_get_raw (metadata), "S") == 0)
					latitude = - latitude;
			}
			coordinates_available++;
		}

		metadata = (GthMetadata *) g_file_info_get_attribute_object (info, "Exif::GPSInfo::GPSLongitude");
		if (metadata != NULL) {
			longitude = exif_coordinate_to_decimal (gth_metadata_get_raw (metadata));

			metadata = (GthMetadata *) g_file_info_get_attribute_object (info, "Exif::GPSInfo::GPSLongitudeRef");
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

static char * decimal_to_string (double value) {
	GString *s = g_string_new ("");

	if (value < 0.0)
		value = - value;

	// degree
	double part = floor (value);
	g_string_append_printf (s, "%.0f°", part);
	value = (value - part) * 60.0;

	// minutes
	part = floor (value);
	g_string_append_printf (s, " %.0fʹ", part);
	value = (value - part) * 60.0;

	// seconds
	g_string_append_printf (s, " %02.3fʺ", value);

	return g_string_free (s, FALSE);
}


extern "C"
char * exiv2_decimal_coordinates_to_string (double latitude, double longitude) {
	char *latitude_s = decimal_to_string (latitude);
	char *longitude_s = decimal_to_string (longitude);
	char *s = g_strdup_printf ("%s %s\n%s %s",
		latitude_s,
		((latitude < 0.0) ? C_("Cardinal point", "S") : C_("Cardinal point", "N")),
		longitude_s,
		((longitude < 0.0) ? C_("Cardinal point", "W") : C_("Cardinal point", "E")));

	g_free (longitude_s);
	g_free (latitude_s);
	return s;
}


extern "C"
gboolean exiv2_can_write_metadata (const char *mime_type) {
	return (g_content_type_equals (mime_type, "image/jpeg")
		|| g_content_type_equals (mime_type, "image/tiff")
		|| g_content_type_equals (mime_type, "image/png")
		|| g_content_type_equals (mime_type, "image/webp"));
}


static void mandatory_int (Exiv2::ExifData &checkdata, const char *tag, int value) {
	Exiv2::ExifKey key = Exiv2::ExifKey(tag);
	if (checkdata.findKey(key) == checkdata.end())
		checkdata[tag] = value;
}


static void mandatory_string (Exiv2::ExifData &checkdata, const char *tag, const char *value) {
	Exiv2::ExifKey key = Exiv2::ExifKey(tag);
	if (checkdata.findKey(key) == checkdata.end())
		checkdata[tag] = value;
}

static const char * get_metadata_type (gpointer metadata, const char *attribute) {
	const char *value_type = NULL;

	if (GTH_IS_METADATA (metadata)) {
		value_type = gth_metadata_get_value_type (GTH_METADATA (metadata));
		if ((g_strcmp0 (value_type, "Undefined") == 0) || (g_strcmp0 (value_type, "") == 0)) {
			value_type = NULL;
		}
		if (value_type != NULL) {
			return value_type;
		}
	}

	GthMetadataInfo *metadatum_info = gth_metadata_info_get (attribute);
	if (metadatum_info != NULL) {
		value_type = metadatum_info->type;
	}

	return value_type;
}


static Exiv2::DataBuf exiv2_write_metadata_private (Exiv2::Image::UniquePtr image, GFileInfo *info, GthImage *image_data) {
	// Preserve the ICC profile if the image was not modified.
	gboolean was_modified = g_file_info_get_attribute_boolean (info, "Loaded::Image::WasModified");
	if (was_modified)
		image->clearIccProfile();
	else
		image->readMetadata();
	image->clearExifData();
	image->clearIptcData();
	image->clearXmpPacket();
	image->clearXmpData();
	image->clearComment();

	// EXIF Data

	Exiv2::ExifData ed;
	char **attributes = g_file_info_list_attributes (info, "Exif");
	for (int i = 0; attributes[i] != NULL; i++) {
		GthMetadata *metadatum;
		char *key;

		metadatum = (GthMetadata *) g_file_info_get_attribute_object (info, attributes[i]);
		key = exiv2_key_from_attribute (attributes[i]);

		try {
			// If the metadatum has no value yet, a new empty value
			// is created. The type is taken from Exiv2's tag
			// lookup tables. If the tag is not found in the table,
			// the type defaults to ASCII.
			// We always create the metadatum explicitly if the
			// type is available to avoid type errors.
			// See bug #610389 for more details.  The original
			// explanation is here:
			// http://uk.groups.yahoo.com/group/exiv2/message/1472

			const char *raw_value = gth_metadata_get_raw (metadatum);
			const char *value_type = get_metadata_type (metadatum, attributes[i]);

			if ((raw_value != NULL) && (strcmp (raw_value, "") != 0) && (value_type != NULL)) {
				Exiv2::Value::UniquePtr value = Exiv2::Value::create (Exiv2::TypeInfo::typeId (value_type));
				value->read (raw_value);
				Exiv2::ExifKey exif_key(key);
				ed.add (exif_key, value.get());
			}
		}
		catch (Exiv2::Error& e) {
			// We don't care about invalid key errors
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
	mandatory_int (ed, "Exif.Photo.ColorSpace", 1); // TODO
	mandatory_string (ed, "Exif.Photo.ExifVersion", "48 50 50 49");
	mandatory_string (ed, "Exif.Photo.ComponentsConfiguration", "1 2 3 0");
	mandatory_string (ed, "Exif.Photo.FlashpixVersion", "48 49 48 48");

	// Overwrite the software tag if the image content was modified

	if (was_modified) {
		ed["Exif.Image.ProcessingSoftware"] = APP " " VERSION;
	}

	// Update tags related to the image content

	Exiv2::ExifThumb thumb(ed);
	gboolean thumbnail_updated = FALSE;
	if (image_data != NULL) {
		int width = gth_image_get_width (image_data);
		int height = gth_image_get_height (image_data);

		if ((width > 0) && (height > 0)) {
			// Update the dimension tags

			ed["Exif.Photo.PixelXDimension"] = width;
			ed["Exif.Image.ImageWidth"] = width;
			ed["Exif.Photo.PixelYDimension"] = height;
			ed["Exif.Image.ImageLength"] = height;
			ed["Exif.Image.Orientation"] = 1;

			// Update the thumbnail

			GthImage *thumbnail_data = gth_image_resize (image_data, 128, GTH_RESIZE_DEFAULT, GTH_SCALE_FILTER_GOOD, NULL);
			GBytes *thumbnail_bytes = save_jpeg (thumbnail_data, NULL, NULL, NULL);
			if (thumbnail_bytes != NULL) {
				gsize thumbnail_size;
				gconstpointer thumbnail_buffer = g_bytes_get_data (thumbnail_bytes, &thumbnail_size);
				thumb.setJpegThumbnail ((Exiv2::byte *) thumbnail_buffer, thumbnail_size);
				ed["Exif.Thumbnail.XResolution"] = 72;
				ed["Exif.Thumbnail.YResolution"] = 72;
				ed["Exif.Thumbnail.ResolutionUnit"] =  2;

				g_bytes_unref (thumbnail_bytes);
				thumbnail_updated = TRUE;
			}
			g_object_unref (thumbnail_data);
		}
	}

	if (was_modified && !thumbnail_updated) {
		thumb.erase();
	}

	// Update the DateTime tag

	if (g_file_info_get_attribute_object (info, "Exif::Image::DateTime") == NULL) {
		GDateTime *current_time = g_date_time_new_now_local ();
		char *exif_date = _g_date_time_to_exif_date (current_time);
		ed["Exif.Image.DateTime"] = exif_date;
		g_free (exif_date);
		g_date_time_unref (current_time);
	}
	ed.sortByKey();

	// IPTC Data

	Exiv2::IptcData id;
	attributes = g_file_info_list_attributes (info, "Iptc");
	for (int i = 0; attributes[i] != NULL; i++) {
		gpointer metadatum = (GthMetadata *) g_file_info_get_attribute_object (info, attributes[i]);
		char *key = exiv2_key_from_attribute (attributes[i]);

		try {
			const char *value_type = get_metadata_type (metadatum, attributes[i]);
			if (value_type != NULL) {
				// See the exif data code above for an explanation.
				Exiv2::Value::UniquePtr value = Exiv2::Value::create (Exiv2::TypeInfo::typeId (value_type));
				Exiv2::IptcKey iptc_key(key);

				const char *raw_value = NULL;
				GthStringList *string_list = NULL;

				switch (gth_metadata_get_data_type (GTH_METADATA (metadatum))) {
				case GTH_METADATA_TYPE_STRING:
					raw_value = gth_metadata_get_raw (GTH_METADATA (metadatum));
					if ((raw_value != NULL) && (strcmp (raw_value, "") != 0)) {
						value->read (raw_value);
						id.add (iptc_key, value.get());
					}
					break;

				case GTH_METADATA_TYPE_STRING_LIST:
					string_list = gth_metadata_get_string_list (GTH_METADATA (metadatum));
					for (GList *scan = gth_string_list_get_list (string_list); scan; scan = scan->next) {
						char *single_value = (char *) scan->data;

						value->read (single_value);
						id.add (iptc_key, value.get());
					}
					break;

				default:
					break;
				}
			}
		}
		catch (Exiv2::Error& e) {
			// We don't care about invalid key errors.
			g_warning ("%s", e.what());
		}

		g_free (key);
	}
	id.sortByKey();
	g_strfreev (attributes);

	// XMP Data

	Exiv2::XmpData xd;
	attributes = g_file_info_list_attributes (info, "Xmp");
	for (int i = 0; attributes[i] != NULL; i++) {
		gpointer metadatum = (GthMetadata *) g_file_info_get_attribute_object (info, attributes[i]);
		char *key = exiv2_key_from_attribute (attributes[i]);

		try {
			const char *value_type = get_metadata_type (metadatum, attributes[i]);
			if (value_type != NULL) {
				// See the exif data code above for an explanation.
				Exiv2::Value::UniquePtr value = Exiv2::Value::create (Exiv2::TypeInfo::typeId (value_type));
				Exiv2::XmpKey xmp_key(key);

				const char *raw_value = NULL;
				GthStringList *string_list = NULL;

				switch (gth_metadata_get_data_type (GTH_METADATA (metadatum))) {
				case GTH_METADATA_TYPE_STRING:
					raw_value = gth_metadata_get_raw (GTH_METADATA (metadatum));
					if ((raw_value != NULL) && (strcmp (raw_value, "") != 0)) {
						value->read (raw_value);
						xd.add (xmp_key, value.get());
					}
					break;

				case GTH_METADATA_TYPE_STRING_LIST:
					string_list = gth_metadata_get_string_list (GTH_METADATA (metadatum));
					for (GList *scan = gth_string_list_get_list (string_list); scan; scan = scan->next) {
						char *single_value = (char *) scan->data;

						value->read (single_value);
						xd.add (xmp_key, value.get());
					}
					break;

				default:
					break;
				}
			}
		}
		catch (Exiv2::Error& e) {
			// We don't care about invalid key errors.
			g_warning ("%s", e.what());
		}

		g_free (key);
	}
	xd.sortByKey();
	g_strfreev (attributes);

	image->setExifData(ed);
	image->setIptcData(id);
	image->setXmpData(xd);
	image->writeMetadata();

	Exiv2::BasicIo &io = image->io();
	io.open();

	return io.read(io.size());
}


extern "C"
GBytes * exiv2_write_metadata_to_buffer (GBytes *bytes, GFileInfo *info, GthImage *image_data, GError **error) {
	try {
		gsize buffer_size;
		gconstpointer buffer = g_bytes_get_data (bytes, &buffer_size);
		Exiv2::Image::UniquePtr image = Exiv2::ImageFactory::open ((Exiv2::byte*) buffer, buffer_size);
		if (image.get() == 0) {
			if (error != NULL)
				*error = g_error_new_literal (G_IO_ERROR, G_IO_ERROR_FAILED, _("Invalid file format"));
			return FALSE;
		}
		Exiv2::DataBuf result = exiv2_write_metadata_private (std::move(image), info, image_data);
		return g_bytes_new (result.data(), result.size());
	}
	catch (Exiv2::Error& e) {
		if (error != NULL) {
			*error = g_error_new_literal (G_IO_ERROR, G_IO_ERROR_FAILED, e.what());
		}
	}
	return NULL;
}


extern "C"
GBytes * exiv2_clear_metadata (GBytes *bytes, GError **error) {
	try {
		gsize buffer_size;
		gconstpointer buffer = g_bytes_get_data (bytes, &buffer_size);
		Exiv2::Image::UniquePtr image = Exiv2::ImageFactory::open ((Exiv2::byte*) buffer, buffer_size);
		if (image.get() == 0) {
			if (error != NULL)
				*error = g_error_new_literal (G_IO_ERROR, G_IO_ERROR_FAILED, _("Invalid file format"));
			return FALSE;
		}

		// Read the metadata to preserve the ICC Profile.
		image->readMetadata();
		image->clearExifData();
		image->clearIptcData();
		image->clearXmpPacket();
		image->clearXmpData();
		image->clearComment();
		image->writeMetadata();

		Exiv2::BasicIo &io = image->io();
		io.open();
		Exiv2::DataBuf result = io.read(io.size());
		return g_bytes_new (result.data(), result.size());
	}
	catch (Exiv2::Error& e) {
		if (error != NULL)
			*error = g_error_new_literal (G_IO_ERROR, G_IO_ERROR_FAILED, e.what());
	}
	return NULL;
}
