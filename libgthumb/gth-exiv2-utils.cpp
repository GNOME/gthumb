/* -*- Mode: CPP; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2003 Free Software Foundation, Inc.
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

#include <exiv2/image.hpp>
#include <exiv2/exif.hpp>
#include <iostream>

inline static GList *
add (GList *metadata,
		const gchar *path, 
		const gchar *value, 
		GthMetadataCategory category)
{
	GthMetadata *new_entry;

	new_entry = g_new (GthMetadata, 1);
	new_entry->category = category;
	new_entry->name = g_strdup (path);
	new_entry->value = g_strdup (value);
	new_entry->position = 0;
	metadata = g_list_prepend (metadata, new_entry);

	return metadata;
}

/*
 * read_exif2_file
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
			metadata = add(metadata, "Failed to open file.", "could not open file to read metadata.", GTH_METADATA_CATEGORY_OTHER);
			return metadata;
		}
		image->readMetadata();

		Exiv2::ExifData &exifData = image->exifData();

		//abort if no data found
		if (exifData.empty()) {
			metadata = add(metadata, "No metadata found.", "could not find any EXIF metadata in the file", GTH_METADATA_CATEGORY_OTHER);
			return metadata;
		}

		//add metadata to glist
		GthMetadata *new_entry;
		for (Exiv2::ExifData::const_iterator i = exifData.begin(); i != exifData.end(); ++i) {
			//determine metadata category
			GthMetadataCategory cat;
			switch (i->ifdId ()) {
				//case Exiv2::ifd0Id : cat = GTH_METADATA_CATEGORY_EXIF_IMAGE; break;
				//case Exiv2::exifIfdId : cat = GTH_METADATA_CATEGORY_EXIF_IMAGE; break;
				//case Exiv2::iopIfdId : cat = GTH_METADATA_CATEGORY_VERSIONS; break;
				//case Exiv2::gpsIfdId : cat = GTH_METADATA_CATEGORY_GPS; break;
				//default : cat = GTH_METADATA_CATEGORY_OTHER; break;
				default : cat = GTH_METADATA_CATEGORY_EXIV2; break;				
			}
			//fill entry
			//metadata = add(metadata, i->tagName().c_str(), i->toString().c_str(), cat);
			metadata = add (metadata, i->key().c_str(), i->toString().c_str(), cat);
		}

		return metadata;
	}
	catch (Exiv2::AnyError& e) {
		std::cerr << "Caught Exiv2 exception '" << e << "'\n";
		return metadata;
	}
}

