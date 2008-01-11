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

#include <exiv2/basicio.hpp>
#include <exiv2/xmp.hpp>
#include <exiv2/error.hpp>
#include <exiv2/image.hpp>
#include <exiv2/exif.hpp>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <iomanip>


using namespace std;

string improve(string value) {
	if (value.find('/') != value.npos) {
		vector<string> res;		

		int cut;
		while( (cut = value.find_first_of(" ")) != value.npos )	{
			if(cut > 0) {
			res.push_back(value.substr(0,cut));
			}
			value = value.substr(cut+1);
		}
		if ((value.length() > 0) and (value.find('/') != value.npos))  {
			res.push_back(value);
			value.clear();
		}
		stringstream stream;
		for (int i(0); i < res.size(); ++i) {
			int a, b;
			sscanf ( res[i].c_str(), "%d/%d", &a, &b);
			stream << (float)a/(float)b << " "; 
		}
		value = stream.str() + value;
		
		return value;
	}
	else return value;
}

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
			for (Exiv2::ExifData::const_iterator i = exifData.begin(); i != end; ++i) {
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
				stringstream stream;
				stream << *i;
				string value = stream.str();

				//disable "improve" untils it works :-)
				metadata = add (metadata, i->key().c_str(), value.c_str(), cat);
			}
		}

		Exiv2::IptcData &iptcData = image->iptcData();
		//abort if no data found
		if (!exifData.empty()) {

			//add iptc-metadata to glist
			GthMetadata *new_entry;
			Exiv2::IptcData::iterator end = iptcData.end();
			for (Exiv2::IptcData::iterator md = iptcData.begin(); md != end; ++md) {

				//determine metadata category
				GthMetadataCategory cat = GTH_METADATA_CATEGORY_IPTC;

				//fill entry
				stringstream value;
				value << *md;

                                stringstream name;
                                name << md->tagName();

				metadata = add (metadata, name.str().c_str(), value.str().c_str(), cat);
			}
		}

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

				stringstream name;
				name << md->groupName() << "." << md->tagName();

				metadata = add (metadata, name.str().c_str(), value.str().c_str(), cat);
			}
		}

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

                                stringstream name;
                                name << md->groupName() << "." << md->tagName();

                                metadata = add (metadata, name.str().c_str(), value.str().c_str(), cat);
			}
		}
	        Exiv2::XmpParser::terminate();
	        return metadata;
	} 
	catch (Exiv2::AnyError& e) {
	        std::cout << "Caught Exiv2 exception '" << e << "'\n";
	        return metadata;
	}
}
