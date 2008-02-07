/* -*- Mode: CPP; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2008 Free Software Foundation, Inc.
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

#include "gth-exiv2-utils.hpp"

#include <exiv2/basicio.hpp>
#include <exiv2/error.hpp>
#include <exiv2/image.hpp>
#include <exiv2/exif.hpp>

#include <iostream>
#include <string>
#include <sstream>
#include <vector>

#ifdef HAVE_EXIV2_XMP_HPP
#include <exiv2/xmp.hpp>
#endif

#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>


using namespace std;

#define MAX_TAGS_PER_CATEGORY 50

static int exif_tag_category_map[GTH_METADATA_CATEGORIES][MAX_TAGS_PER_CATEGORY] = {

	/* GTH_METADATA_CATEGORY_FILE */
	/* The filesystem info is generated and inserted by gthumb in the
	   correct order, so we don't need to sort it. */
	{ -1 },

	/* GTH_METADATA_CATEGORY_EXIF_CAMERA */
	/* This is general information about the camera, date and user,
	   that exists in the IFD0 or EXIF blocks. */
	{ 
	271,	//EXIF_TAG_MAKE
	272,	//EXIF_TAG_MODEL
	305,	//EXIF_TAG_SOFTWARE

	306,	//EXIF_TAG_DATE_TIME
	37520,	//EXIF_TAG_SUB_SEC_TIME
	36867,	//EXIF_TAG_DATE_TIME_ORIGINAL
	37521,	//EXIF_TAG_SUB_SEC_TIME_ORIGINAL
	36868,	//EXIF_TAG_DATE_TIME_DIGITIZED
	37522,	//EXIF_TAG_SUB_SEC_TIME_DIGITIZED

	37510,	//EXIF_TAG_USER_COMMENT
	270,	//EXIF_TAG_IMAGE_DESCRIPTION
	315,	//EXIF_TAG_ARTIST
	33432,	//EXIF_TAG_COPYRIGHT

	42016,	//EXIF_TAG_IMAGE_UNIQUE_ID

	40964,	//EXIF_TAG_RELATED_SOUND_FILE
	-1 },

	/* GTH_METADATA_CATEGORY_EXIF_CONDITIONS */
	/* These tags describe the conditions when the photo was taken,
	   and are located in the IFD0 or EXIF blocks. */
	{ 
	34855,	//EXIF_TAG_ISO_SPEED_RATINGS
	37379,	//EXIF_TAG_BRIGHTNESS_VALUE

	33437,	//EXIF_TAG_FNUMBER
	37378,	//EXIF_TAG_APERTURE_VALUE
	37381,	//EXIF_TAG_MAX_APERTURE_VALUE

	33434,	//EXIF_TAG_EXPOSURE_TIME
	34850,	//EXIF_TAG_EXPOSURE_PROGRAM
	41493,	//EXIF_TAG_EXPOSURE_INDEX
	37380,	//EXIF_TAG_EXPOSURE_BIAS_VALUE
	41986,	//EXIF_TAG_EXPOSURE_MODE
	37377,	//EXIF_TAG_SHUTTER_SPEED_VALUE

	37383,	//EXIF_TAG_METERING_MODE
	37384,	//EXIF_TAG_LIGHT_SOURCE
	41987,	//EXIF_TAG_WHITE_BALANCE
	37385,	//EXIF_TAG_FLASH
	41483,	//EXIF_TAG_FLASH_ENERGY

	37382,	//EXIF_TAG_SUBJECT_DISTANCE
	41996,	//EXIF_TAG_SUBJECT_DISTANCE_RANGE
	37396,	//EXIF_TAG_SUBJECT_AREA
	41492,	//EXIF_TAG_SUBJECT_LOCATION

	37386,	//EXIF_TAG_FOCAL_LENGTH
	41989,	//EXIF_TAG_FOCAL_LENGTH_IN_35MM_FILM
	41486,	//EXIF_TAG_FOCAL_PLANE_X_RESOLUTION
	41487,	//EXIF_TAG_FOCAL_PLANE_Y_RESOLUTION
	41488,	//EXIF_TAG_FOCAL_PLANE_RESOLUTION_UNIT

	41992,	//EXIF_TAG_CONTRAST
	41993,	//EXIF_TAG_SATURATION
	41994,	//EXIF_TAG_SHARPNESS

	41729,	//EXIF_TAG_SCENE_TYPE
	41990,	//EXIF_TAG_SCENE_CAPTURE_TYPE

	41985,	//EXIF_TAG_CUSTOM_RENDERED

	41988,	//EXIF_TAG_DIGITAL_ZOOM_RATIO

	41728,	//EXIF_TAG_FILE_SOURCE

	41495,	//EXIF_TAG_SENSING_METHOD
	33422,	//EXIF_TAG_CFA_PATTERN	exif.image
	41730,	//EXIF_TAG_CFA_PATTERN	exif.photo

	41995,	//EXIF_TAG_DEVICE_SETTING_DESCRIPTION
	34856,	//EXIF_TAG_OECF
	41484,	//EXIF_TAG_SPATIAL_FREQUENCY_RESPONSE
	34852,	//EXIF_TAG_SPECTRAL_SENSITIVITY
	41991,	//EXIF_TAG_GAIN_CONTROL

	  -1 },

	/* GTH_METADATA_CATEGORY_EXIF_IMAGE */
	/* These tags describe the main image data structures, and
	   come from the IFD0 and EXIF blocks. */
	{

	  // Image data structure

	256,	//EXIF_TAG_IMAGE_WIDTH
	257,	//EXIF_TAG_IMAGE_LENGTH
	40962,	//EXIF_TAG_PIXEL_X_DIMENSION
	40963,	//EXIF_TAG_PIXEL_Y_DIMENSION

	274,	//EXIF_TAG_ORIENTATION

	282,	//EXIF_TAG_X_RESOLUTION
	283,	//EXIF_TAG_Y_RESOLUTION
	296,	//EXIF_TAG_RESOLUTION_UNIT

	259,	//EXIF_TAG_COMPRESSION

	277,	//EXIF_TAG_SAMPLES_PER_PIXEL
	258,	//EXIF_TAG_BITS_PER_SAMPLE

	284,	//EXIF_TAG_PLANAR_CONFIGURATION
	530,	//EXIF_TAG_YCBCR_SUB_SAMPLING
	531,	//EXIF_TAG_YCBCR_POSITIONING
	262,	//EXIF_TAG_PHOTOMETRIC_INTERPRETATION
	37121,	//EXIF_TAG_COMPONENTS_CONFIGURATION
	37122,	//EXIF_TAG_COMPRESSED_BITS_PER_PIXEL

	  // Offsets

	273,	//EXIF_TAG_STRIP_OFFSETS
	278,	//EXIF_TAG_ROWS_PER_STRIP
	279,	//EXIF_TAG_STRIP_BYTE_COUNTS
	513,	//EXIF_TAG_JPEG_INTERCHANGE_FORMAT
	514,	//EXIF_TAG_JPEG_INTERCHANGE_FORMAT_LENGTH

	  // Image data characteristics

	301,	//EXIF_TAG_TRANSFER_FUNCTION
	318,	//EXIF_TAG_WHITE_POINT
	319,	//EXIF_TAG_PRIMARY_CHROMATICITIES
	529,	//EXIF_TAG_YCBCR_COEFFICIENTS
	532,	//EXIF_TAG_REFERENCE_BLACK_WHITE

	40961,	//EXIF_TAG_COLOR_SPACE

	  -1 },

	/* GTH_METADATA_CATEGORY_EXIF_THUMBNAIL */
	/* There are normally only a few of these tags, so we don't bother
	   sorting them. They are displayed in the order that they appear in
	   the file. IFD0 (main image) and IFD1 (thumbnail) share many of the
	   same tags. The IFD0 tags are sorted by the structures above. The
	   IFD1 tags are placed into this category. */
	{ -1 },

	/* GTH_METADATA_CATEGORY_GPS */
	/* GPS data is stored in a special IFD (GPS) */
	{ 
	1,	//EXIF_TAG_GPS_LATITUDE_REF
	2,	//EXIF_TAG_GPS_LATITUDE
	3,	//EXIF_TAG_GPS_LONGITUDE_REF
	4,	//EXIF_TAG_GPS_LONGITUDE
	5,	//EXIF_TAG_GPS_ALTITUDE_REF
	6,	//EXIF_TAG_GPS_ALTITUDE
	7,	//EXIF_TAG_GPS_TIME_STAMP
	8,	//EXIF_TAG_GPS_SATELLITES
	9,	//EXIF_TAG_GPS_STATUS
	10,	//EXIF_TAG_GPS_MEASURE_MODE
	11,	//EXIF_TAG_GPS_DOP
	12,	//EXIF_TAG_GPS_SPEED_REF
	13,	//EXIF_TAG_GPS_SPEED
	14,	//EXIF_TAG_GPS_TRACK_REF
	15,	//EXIF_TAG_GPS_TRACK
	16,	//EXIF_TAG_GPS_IMG_DIRECTION_REF
	17,	//EXIF_TAG_GPS_IMG_DIRECTION
	18,	//EXIF_TAG_GPS_MAP_DATUM
	19,	//EXIF_TAG_GPS_DEST_LATITUDE_REF
	20,	//EXIF_TAG_GPS_DEST_LATITUDE
	21,	//EXIF_TAG_GPS_DEST_LONGITUDE_REF
	22,	//EXIF_TAG_GPS_DEST_LONGITUDE
	23,	//EXIF_TAG_GPS_DEST_BEARING_REF
	24,	//EXIF_TAG_GPS_DEST_BEARING
	25,	//EXIF_TAG_GPS_DEST_DISTANCE_REF
	26,	//EXIF_TAG_GPS_DEST_DISTANCE
	27,	//EXIF_TAG_GPS_PROCESSING_METHOD
	28,	//EXIF_TAG_GPS_AREA_INFORMATION
	29,	//EXIF_TAG_GPS_DATE_STAMP
	30,	//EXIF_TAG_GPS_DIFFERENTIAL
 	0,	//EXIF_TAG_GPS_VERSION_ID
	  -1 },

	/* GTH_METADATA_CATEGORY_MAKERNOTE */
	/* These tags are semi-proprietary and vary from manufacturer to
	   manufacturer, so we don't bother trying to sort them. They are
	   listed in the order that they appear in the jpeg file. */
	{ -1 },

	/* GTH_METADATA_CATEGORY_VERSIONS */
	/* From IFD0, EXIF,or INTEROPERABILITY blocks. */
	{ 
	36864,	//EXIF_TAG_EXIF_VERSION
	40960,	//EXIF_TAG_FLASH_PIX_VERSION
	1,	//EXIF_TAG_INTEROPERABILITY_INDEX
	2,	//EXIF_TAG_INTEROPERABILITY_VERSION
	4096,	//EXIF_TAG_RELATED_IMAGE_FILE_FORMAT
	4097,	//EXIF_TAG_RELATED_IMAGE_WIDTH
	4098,	//EXIF_TAG_RELATED_IMAGE_LENGTH
	  -1 },

        /* GTH_METADATA_CATEGORY_IPTC */
        /* Reserved for IPTC, not exif */
        { -1},

	/* GTH_METADATA_CATEGORY_XMP_EMBEDDED */
	/* Reserved for XMP, not exif */
	{ -1},

	/* GTH_METADATA_CATEGORY_XMP_SIDECAR */
	/* Reserved for XMP, not exif */
	{ -1},

        /* GTH_METADATA_CATEGORY_GSTREAMER */
        /* Reserved for audio/video stuff, not exif */
        { -1},

	/* GTH_METADATA_CATEGORY_OTHER */
	/* New and unrecognized tags automatically go here. */
	{ -1 }
};

static GthMetadataCategory
tag_category_exiv2 (const Exiv2::Exifdatum &md, int &position)
{
	GthMetadataCategory category;
	int tempCategory;

	/* Tags that are not explicitly sorted by this function will be sorted
	   alphabetically by the display functions. This function categorizes 
	   tags (always) and applies sorting overrides (sometimes). */

	/* Makernotes are easily identified. No special sorting. */
        if (Exiv2::ExifTags::isMakerIfd(md.ifdId()))
                return GTH_METADATA_CATEGORY_MAKERNOTE;

	/* Standard (non-Makernote) IFDs are processed here. */
	switch (md.ifdId()) {
		/* Some IFDs require special handling to ensure proper categorization.
		   This is because:
		   	1) IFD0 and IFD1 may duplicate some tags (with different values)
			2) Some tag IDs (numeric values) overlap in the GPS and
			   INTEROPERABILITY IFDs.					*/
	case Exiv2::ifd1Id:
		/* Data in IFD1 is for the embedded thumbnail. Keep it separate, do not sort */
		return GTH_METADATA_CATEGORY_EXIF_THUMBNAIL;
		break;

	case Exiv2::gpsIfdId:
		/* Go straight to the GPS category if this is in a GPS IFD, to
		   avoid the tag ID overlap problem. Do sort. */
		category = GTH_METADATA_CATEGORY_GPS;
		break;

	case Exiv2::iopIfdId:
		/* Go straight to the version category if this is in an
		   interop IFD, to avoid the tag ID overlap problem. Do sort. */
		category = GTH_METADATA_CATEGORY_VERSIONS;
		break;

	default:
		/* Start the tag search at the beginning, and use the first match */
		category = (GthMetadataCategory)1;
	}

	/* Apply sorting overrides by setting the "position" parameter 
	   to a non-zero value. */
	while (category < GTH_METADATA_CATEGORIES) {
		int j = 0;
		while (exif_tag_category_map[category][j] != -1) {
			if  (exif_tag_category_map[category][j] == md.tag()) {
				position = j+1;
				return category;
			}
			j++;
		}
		tempCategory = (int)category;
		++tempCategory;
		category = (GthMetadataCategory)tempCategory;
	}

	/* Hmm, didn't recognize that tag! */
	return GTH_METADATA_CATEGORY_OTHER;
}


static gboolean
has_non_whitespace_text (const char *text_in)
{
        gchar *pos;

        if (text_in == NULL)
                return FALSE;

        for (pos = (char *) text_in; *pos != 0; pos = g_utf8_next_char (pos)) {
                gunichar ch = g_utf8_get_char (pos);
                if (!g_unichar_isspace (ch))
                        return TRUE;
        }

        return FALSE;
}


/* Add the tag the gThumb metadata store. */
inline static GList *
add (GList              *metadata,
     const gchar        *full_name, 
     const gchar        *display_name,
     const gchar	*formatted_value,
     const gchar	*raw_value,
     GthMetadataCategory category,
     const int		 position)
{
	GthMetadata *new_entry;

	/* skip blank values */
	if (!has_non_whitespace_text (formatted_value))
		return metadata;

	new_entry = g_new (GthMetadata, 1);
	new_entry->category = category;
	new_entry->full_name = g_strdup (full_name);
	new_entry->display_name = g_strdup (display_name);
	new_entry->formatted_value = g_strdup (formatted_value);
	new_entry->raw_value = g_strdup (raw_value);
	new_entry->position = position;
	new_entry->writeable = TRUE;
	metadata = g_list_prepend (metadata, new_entry);

	return metadata;
}

/*
 * read_exiv2_file
 * reads metadata from image files
 * code relies heavily on example1 from the exiv2 website
 * http://www.exiv2.org/example1.html
 */
extern "C"
GList *
read_exiv2_file (const char *uri, GList *metadata)
{
	try {
		Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(uri);
		if (image.get() == 0) {
			//die silently if image cannot be opened
			return metadata;
		}
		image->readMetadata();

		Exiv2::ExifData &exifData = image->exifData();

		//abort if no data found
		if (!exifData.empty()) {

			//add exif-metadata to glist
			GthMetadata *new_entry;
			Exiv2::ExifData::const_iterator end = exifData.end();
			for (Exiv2::ExifData::const_iterator md = exifData.begin(); md != end; ++md) {
				//determine metadata category
				int pos;
				GthMetadataCategory cat = tag_category_exiv2 (*md, pos);
				
				//fill entry
				stringstream value;
				value << *md;

				stringstream short_name;
				if (md->ifdId () > Exiv2::ifd1Id) {
					// Must be a MakerNote - include group name
					short_name << md->groupName() << "." << md->tagName();
				} else {
					// Normal exif tag - just use tag name
					short_name << md->tagName();	
				}

				metadata = add (metadata, 
						md->key().c_str(), 
						short_name.str().c_str(), 
						value.str().c_str(),
						md->toString().c_str(), 
						cat, 
						pos);
			}
		}

		Exiv2::IptcData &iptcData = image->iptcData();
		//abort if no data found
		if (!iptcData.empty()) {

			//add iptc-metadata to glist
			GthMetadata *new_entry;
			Exiv2::IptcData::iterator end = iptcData.end();
			for (Exiv2::IptcData::iterator md = iptcData.begin(); md != end; ++md) {

				//determine metadata category
				GthMetadataCategory cat = GTH_METADATA_CATEGORY_IPTC;

				//fill entry
				stringstream value;
				value << *md;

				stringstream short_name;
				short_name << md->tagName();

				metadata = add (metadata, 
						md->key().c_str(), 
						short_name.str().c_str(), 
						value.str().c_str(), 
						md->toString().c_str(),
						cat, 
						0);
			}
		}

#ifdef HAVE_EXIV2_XMP_HPP
		Exiv2::XmpData &xmpData = image->xmpData();
		if (!xmpData.empty()) {

			//add xmp-metadata to glist
			GthMetadata *new_entry;
			Exiv2::XmpData::iterator end = xmpData.end();
			for (Exiv2::XmpData::iterator md = xmpData.begin(); md != end; ++md) {

				//determine metadata category
				GthMetadataCategory cat = GTH_METADATA_CATEGORY_XMP_EMBEDDED;

				//fill entry
				stringstream value;
				value << *md;

				stringstream short_name;
				short_name << md->groupName() << "." << md->tagName();

				metadata = add (metadata, 
						md->key().c_str(), 
						short_name.str().c_str(), 
						value.str().c_str(), 
						md->toString().c_str(),
						cat, 
						0);
			}
		}
#endif

		return metadata;
	}
	catch (Exiv2::AnyError& e) {
		std::cerr << "Caught Exiv2 exception '" << e << "'\n";
		return metadata;
	}
}


extern "C"
GList *
read_exiv2_sidecar (const char *uri, GList *metadata)
{
	try {
#ifdef HAVE_EXIV2_XMP_HPP
		Exiv2::DataBuf buf = Exiv2::readFile(uri);
		std::string xmpPacket;
		xmpPacket.assign(reinterpret_cast<char*>(buf.pData_), buf.size_);
		Exiv2::XmpData xmpData;

		if (0 != Exiv2::XmpParser::decode(xmpData, xmpPacket))
			return metadata;

		if (!xmpData.empty()) {

			//add xmp-metadata to glist
			GthMetadata *new_entry;
			Exiv2::XmpData::iterator end = xmpData.end();
			for (Exiv2::XmpData::iterator md = xmpData.begin(); md != end; ++md) {

				//determine metadata category
				GthMetadataCategory cat = GTH_METADATA_CATEGORY_XMP_SIDECAR;

				//fill entry
				stringstream value;
				value << *md;

				stringstream short_name;
				short_name << md->groupName() << "." << md->tagName();

				metadata = add (metadata, 
						md->key().c_str(), 
						short_name.str().c_str(), 
						value.str().c_str(), 
						md->toString().c_str(),
						cat, 
						0);
			}
		}
		Exiv2::XmpParser::terminate();
#endif
		return metadata;
	} 
	catch (Exiv2::AnyError& e) {
		std::cout << "Caught Exiv2 exception '" << e << "'\n";
		return metadata;
	}
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

extern "C"
void
write_metadata (const char *from_file,
		const char *to_file,
		GList      *metadata_in)
{
	try {
		GList *scan;

		//Open first image
		Exiv2::Image::AutoPtr image1 = Exiv2::ImageFactory::open (from_file);
		g_assert (image1.get() != 0);
	
		// Load existing metadata
		image1->readMetadata();

		Exiv2::ExifData &ed = image1->exifData();
		Exiv2::IptcData &id = image1->iptcData();
#ifdef HAVE_EXIV2_XMP_HPP
		Exiv2::XmpData &xd = image1->xmpData();
#endif

		for (scan = metadata_in; scan; scan = scan->next) {
			// Update the requested tag
			GthMetadata *metadatum = (GthMetadata *) scan->data;
			if (metadatum->full_name != NULL) {
				if (g_str_has_prefix (metadatum->full_name, "Exif")) {
					ed[metadatum->full_name] = metadatum->raw_value;
				}
				else if (g_str_has_prefix (metadatum->full_name, "Iptc")) {
					id[metadatum->full_name] = metadatum->raw_value;
		        	}
#ifdef HAVE_EXIV2_XMP_HPP
				else if (g_str_has_prefix (metadatum->full_name, "Xmp")) {
					xd[metadatum->full_name] = metadatum->raw_value;
		        	}
#endif
			}
		}

		// Delete thumbnail and IFD1 tags, because the main image may
		// have changed, and gThumb doesn't use the embedded thumbnails
		// anyways.
		image1->exifData().eraseThumbnail();

		// Mandatory tags
		mandatory_int (ed, "Exif.Image.XResolution", 72);
		mandatory_int (ed, "Exif.Image.YResolution", 72);
		mandatory_int (ed, "Exif.Image.ResolutionUnit", 2);
		mandatory_int (ed, "Exif.Image.YCbCrPositioning", 1);
		mandatory_int (ed, "Exif.Photo.ColorSpace", 1);
		mandatory_string (ed, "Exif.Photo.ExifVersion", "48 50 50 49");
		mandatory_string (ed, "Exif.Photo.ComponentsConfiguration", "1 2 3 0");
		mandatory_string (ed, "Exif.Photo.FlashpixVersion", "48 49 48 48");
		mandatory_string (ed, "Exif.Image.Software", "gThumb " VERSION);

		int width = 0;
		int height = 0;
		gdk_pixbuf_get_file_info (to_file, &width, &height);
		
		if (width > 0)
			ed["Exif.Photo.PixelXDimension"] = width;

		if (height > 0)
			ed["Exif.Photo.PixelYDimension"] = height;

		time_t curtime = 0;
		struct tm  tm;

		time (&curtime);
		localtime_r (&curtime, &tm);
                char *buf = g_strdup_printf ("%04d:%02d:%02d %02d:%02d:%02d ",
                                             tm.tm_year + 1900,
                                             tm.tm_mon + 1,
                                             tm.tm_mday,
                                             tm.tm_hour,
                                             tm.tm_min,
                                             tm.tm_sec );
		ed["Exif.Image.DateTime"] = buf;
		g_free (buf);
	
		// Open second image (in many applications, this will actually
		// be the same as the the first image (i.e., updating a file
		// in-place.
		Exiv2::Image::AutoPtr image2 = Exiv2::ImageFactory::open (to_file);
		g_assert (image2.get() != 0);

		image2->setExifData (image1->exifData());
		image2->setIptcData (image1->iptcData());
#ifdef HAVE_EXIV2_XMP_HPP
		image2->setXmpData (image1->xmpData());
#endif

		// overwrite existing metadata with new metadata
		image2->writeMetadata();
	}

	catch (const Exiv2::AnyError& error) {
		// TODO: signal an error to the caller?
		std::cerr << error << "\n";
	}
}
