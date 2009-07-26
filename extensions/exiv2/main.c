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
#include <gtk/gtk.h>
#include <gthumb.h>
#include "gth-metadata-provider-exiv2.h"
#include "exiv2-utils.h"


GthMetadataCategory exiv2_metadata_category[] = {
	{ "Exif::General", N_("Exif General"), 30 },
	{ "Exif::Conditions", N_("Exif Conditions"), 31 },
	{ "Exif::Structure", N_("Exif Structure"), 32 },
	{ "Exif::Thumbnail", N_("Exif Thumbnail"), 33 },
	{ "Exif::GPS", N_("Exif GPS"), 34 },
	{ "Exif::MakerNotes", N_("Exif Maker Notes"), 35 },
	{ "Exif::Versions", N_("Exif Versions"), 36 },
	{ "Exif::Other", N_("Exif Other"), 37 },
	{ "Iptc", N_("IPTC"), 38 },
	{ "Xmp::Embedded", N_("XMP Embedded"), 39 },
	{ "Xmp::Sidecar", N_("XMP Attached"), 40 },
	{ NULL, NULL, 0 }
};


GthMetadataInfo exiv2_metadata_info[] = {
	{ "Exif::Image::Make", NULL, "Exif::General", 1, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Image::Model", NULL, "Exif::General", 2, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Image::Software", NULL, "Exif::General", 3, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Image::DateTime", NULL, "Exif::General", 4, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::SubSecTime", NULL, "Exif::General", 5, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::DateTimeOriginal", NULL, "Exif::General", 6, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW},
	{ "Exif::Photo::SubSecTimeOriginal", NULL, "Exif::General", 7, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::DateTimeDigitized", NULL, "Exif::General", 8, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::SubSecTimeDigitized", NULL, "Exif::General", 9, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Image::Artist", NULL, "Exif::General", 11, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Image::Copyright", NULL, "Exif::General", 12, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::UniqueID", NULL, "Exif::General", 13, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::SoundFile", NULL, "Exif::General", 14, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },

	{ "Exif::Photo::ISOSpeedRatings", NULL, "Exif::Conditions", 2, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::BrightnessValue", NULL, "Exif::Conditions", 3, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::FNumber", NULL, "Exif::Conditions", 4, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::ApertureValue", NULL, "Exif::Conditions", 5, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::MaxApertureValue", NULL, "Exif::Conditions", 6, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::ExposureTime", NULL, "Exif::Conditions", 7, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::ExposureProgram", NULL, "Exif::Conditions", 8, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::ExposureIndex", NULL, "Exif::Conditions", 9, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::ExposureBiasValue", NULL, "Exif::Conditions", 10, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::ExposureMode", NULL, "Exif::Conditions", 11, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::ShutterSpeedValue", NULL, "Exif::Conditions", 12, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::MeteringMode", NULL, "Exif::Conditions", 13, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::LightSource", NULL, "Exif::Conditions", 14, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::WhiteBalance", NULL, "Exif::Conditions", 15, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::Flash", NULL, "Exif::Conditions", 16, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::FlashEnergy", NULL, "Exif::Conditions", 17, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::SubjectDistance", NULL, "Exif::Conditions", 18, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::SubjectDistanceRange", NULL, "Exif::Conditions", 19, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::SubjectArea", NULL, "Exif::Conditions", 20, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::SubjectLocation", NULL, "Exif::Conditions", 21, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::FocalLength", NULL, "Exif::Conditions", 22, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::FocalLengthIn35mmFilm", NULL, "Exif::Conditions", 23, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::FocalPlaneXResolution", NULL, "Exif::Conditions", 24, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::FocalPlaneYResolution", NULL, "Exif::Conditions", 25, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::FocalPlaneResolutionUnit", NULL, "Exif::Conditions", 26, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::Contrast", NULL, "Exif::Conditions", 27, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::Saturation", NULL, "Exif::Conditions", 28, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::Sharpness", NULL, "Exif::Conditions", 29, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::SceneType", NULL, "Exif::Conditions", 30, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::SceneCaptureType", NULL, "Exif::Conditions", 31, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::CustomRendered", NULL, "Exif::Conditions", 32, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::DigitalZoomRatio", NULL, "Exif::Conditions", 33, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::FileSource", NULL, "Exif::Conditions", 34, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::SensingMethod", NULL, "Exif::Conditions", 35, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::DeviceSettingDescription", NULL, "Exif::Conditions", 36, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::OECF", NULL, "Exif::Conditions", 37, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::SpatialFrequencyResponse", NULL, "Exif::Conditions", 38, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::SpectralSensitivity", NULL, "Exif::Conditions", 39, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::GainControl", NULL, "Exif::Conditions", 40, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },

	{ "Exif::Image::ImageWidth", NULL, "Exif::Structure", 1, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Image::ImageLength", NULL, "Exif::Structure", 2, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::PixelXDimension", NULL, "Exif::Structure", 3, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::PixelYDimension", NULL, "Exif::Structure", 4, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Image::Orientation", NULL, "Exif::Structure", 5, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Image::XResolution", NULL, "Exif::Structure", 6, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Image::YResolution", NULL, "Exif::Structure", 7, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Image::ResolutionUnit", NULL, "Exif::Structure", 8, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Image::Compression", NULL, "Exif::Structure", 9, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Image::SamplesPerPixel", NULL, "Exif::Structure", 10, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Image::BitsPerSample", NULL, "Exif::Structure", 11, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Image::PlanarConfiguration", NULL, "Exif::Structure", 12, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Image::YCbCrSubSampling", NULL, "Exif::Structure", 13, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Image::YCbCrPositioning", NULL, "Exif::Structure", 14, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Image::PhotometricInterpretation", NULL, "Exif::Structure", 15, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::ComponentsConfiguration", NULL, "Exif::Structure", 16, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::CompressedBitsPerPixel", NULL, "Exif::Structure", 17, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::StripOffset", NULL, "Exif::Structure", 18, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::RowsPerStrip", NULL, "Exif::Structure", 19, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::StripByteCounts", NULL, "Exif::Structure", 20, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::JPEGInterchangeFormat", NULL, "Exif::Structure", 21, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::JPEGInterchangeFormatLength", NULL, "Exif::Structure", 22, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::TransferFunction", NULL, "Exif::Structure", 23, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::WhitePoint", NULL, "Exif::Structure", 24, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::PrimaryChromaticities", NULL, "Exif::Structure", 25, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::YCbCrCoefficients", NULL, "Exif::Structure", 26, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::ReferenceBlackWhite", NULL, "Exif::Structure", 27, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::ColorSpace", NULL, "Exif::Structure", 28, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },

	{ "Exif::Photo::ExifVersion", NULL, "Exif::Versions", 1, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Image::ExifTag", NULL, "Exif::Versions", 2, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::FlashpixVersion", NULL, "Exif::Versions", 3, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Iop::InteroperabilityIndex", NULL, "Exif::Versions", 4, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Iop::InteroperabilityVersion", NULL, "Exif::Versions", 5, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::InteroperabilityTag", NULL, "Exif::Versions", 6, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::RelatedImageFileFormat", NULL, "Exif::Versions", 7, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::RelatedImageWidth", NULL, "Exif::Versions", 8, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::RelatedImageLength", NULL, "Exif::Versions", 9, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },

	{ "Exif::Photo::MakerNote", NULL, "Exif::Other", 0, GTH_METADATA_ALLOW_NOWHERE },

	{ NULL, NULL, NULL, 0, 0 }
};


G_MODULE_EXPORT void
gthumb_extension_activate (void)
{
	gth_main_register_metadata_category (exiv2_metadata_category);
	gth_main_register_metadata_info_v (exiv2_metadata_info);
	gth_main_register_metadata_provider (GTH_TYPE_METADATA_PROVIDER_EXIV2);
	gth_hook_add_callback ("save-pixbuf", 10, G_CALLBACK (exiv2_write_metadata), NULL);
}


G_MODULE_EXPORT void
gthumb_extension_deactivate (void)
{
}


G_MODULE_EXPORT gboolean
gthumb_extension_is_configurable (void)
{
	return FALSE;
}


G_MODULE_EXPORT void
gthumb_extension_configure (GtkWindow *parent)
{
}
