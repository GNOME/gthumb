using Gth;

namespace ExifMetadata {
	public void register_categories () {
		MetadataCategory.register ("Exif::General", N_("General"));
		MetadataCategory.register ("Exif::Conditions", N_("Conditions"));
		MetadataCategory.register ("Exif::Structure", N_("Structure"));
		MetadataCategory.register ("Exif::Thumbnail", N_("Thumbnail"));
		MetadataCategory.register ("Exif::GPS", N_("GPS"));
		MetadataCategory.register ("Exif::MakerNotes", N_("Maker Notes"));
		MetadataCategory.register ("Exif::Versions", N_("Versions"));
		MetadataCategory.register ("Exif::Other", N_("Other"));
		MetadataCategory.register ("Iptc", N_("IPTC"));
		MetadataCategory.register ("Xmp::Embedded", N_("XMP"));
		MetadataCategory.register ("Xmp::Sidecar", N_("Attached"));
	}

	public void register_info () {
		// Specify the type of a field to allow to create it if it's not
		// already present in the image.

		MetadataInfo.register ("Exif::Image::Make", null, "Exif::General", METADATA_ALLOW_EVERYWHERE);
		MetadataInfo.register ("Exif::Image::Model", null, "Exif::General",  METADATA_ALLOW_EVERYWHERE);
		MetadataInfo.register ("Exif::Image::Software", null, "Exif::General", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Exif::Image::DateTime", null, "Exif::General", METADATA_ALLOW_EVERYWHERE);
		MetadataInfo.register ("Exif::Photo::SubSecTime", null, "Exif::General", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Exif::Photo::DateTimeOriginal", null, "Exif::General", METADATA_ALLOW_EVERYWHERE, "Ascii");
		MetadataInfo.register ("Exif::Photo::SubSecTimeOriginal", null, "Exif::General", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW, "Ascii");
		MetadataInfo.register ("Exif::Photo::DateTimeDigitized", null, "Exif::General", METADATA_ALLOW_EVERYWHERE);
		MetadataInfo.register ("Exif::Photo::SubSecTimeDigitized", null, "Exif::General", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Exif::Image::Artist", null, "Exif::General", METADATA_ALLOW_EVERYWHERE);
		MetadataInfo.register ("Exif::Image::Copyright", null, "Exif::General", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Exif::Photo::UniqueID", null, "Exif::General", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Exif::Photo::SoundFile", null, "Exif::General", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);

		MetadataInfo.register ("Exif::Photo::ISOSpeedRatings", null, "Exif::Conditions", METADATA_ALLOW_EVERYWHERE);
		MetadataInfo.register ("Exif::Photo::BrightnessValue", null, "Exif::Conditions", METADATA_ALLOW_EVERYWHERE);
		MetadataInfo.register ("Exif::Photo::FNumber", null, "Exif::Conditions", METADATA_ALLOW_EVERYWHERE);
		MetadataInfo.register ("Exif::Photo::ApertureValue", null, "Exif::Conditions", METADATA_ALLOW_EVERYWHERE);
		MetadataInfo.register ("Exif::Photo::MaxApertureValue", null, "Exif::Conditions", METADATA_ALLOW_EVERYWHERE);
		MetadataInfo.register ("Exif::Photo::ExposureTime", null, "Exif::Conditions", METADATA_ALLOW_EVERYWHERE);
		MetadataInfo.register ("Exif::Photo::ExposureProgram", null, "Exif::Conditions", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Exif::Photo::ExposureIndex", null, "Exif::Conditions", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Exif::Photo::ExposureBiasValue", null, "Exif::Conditions", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Exif::Photo::ExposureMode", null, "Exif::Conditions", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Exif::Photo::ShutterSpeedValue", null, "Exif::Conditions", METADATA_ALLOW_EVERYWHERE);
		MetadataInfo.register ("Exif::Photo::MeteringMode", null, "Exif::Conditions", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Exif::Photo::LightSource", null, "Exif::Conditions", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Exif::Photo::WhiteBalance", null, "Exif::Conditions", METADATA_ALLOW_EVERYWHERE);
		MetadataInfo.register ("Exif::Photo::Flash", null, "Exif::Conditions", METADATA_ALLOW_EVERYWHERE);
		MetadataInfo.register ("Exif::Photo::FlashEnergy", null, "Exif::Conditions", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Exif::Photo::SubjectDistance", null, "Exif::Conditions", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Exif::Photo::SubjectDistanceRange", null, "Exif::Conditions", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Exif::Photo::SubjectArea", null, "Exif::Conditions", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Exif::Photo::SubjectLocation", null, "Exif::Conditions", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Exif::Photo::FocalLength", null, "Exif::Conditions", METADATA_ALLOW_EVERYWHERE);
		MetadataInfo.register ("Exif::Photo::FocalLengthIn35mmFilm", null, "Exif::Conditions", METADATA_ALLOW_EVERYWHERE);
		MetadataInfo.register ("Exif::Photo::FocalPlaneXResolution", null, "Exif::Conditions", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Exif::Photo::FocalPlaneYResolution", null, "Exif::Conditions", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Exif::Photo::FocalPlaneResolutionUnit", null, "Exif::Conditions", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Exif::Photo::Contrast", null, "Exif::Conditions", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Exif::Photo::Saturation", null, "Exif::Conditions", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Exif::Photo::Sharpness", null, "Exif::Conditions", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Exif::Photo::SceneType", null, "Exif::Conditions", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Exif::Photo::SceneCaptureType", null, "Exif::Conditions", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Exif::Photo::CustomRendered", null, "Exif::Conditions", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Exif::Photo::DigitalZoomRatio", null, "Exif::Conditions", METADATA_ALLOW_EVERYWHERE);
		MetadataInfo.register ("Exif::Photo::FileSource", null, "Exif::Conditions", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Exif::Photo::SensingMethod", null, "Exif::Conditions", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Exif::Photo::DeviceSettingDescription", null, "Exif::Conditions", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Exif::Photo::OECF", null, "Exif::Conditions", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Exif::Photo::SpatialFrequencyResponse", null, "Exif::Conditions", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Exif::Photo::SpectralSensitivity", null, "Exif::Conditions", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Exif::Photo::GainControl", null, "Exif::Conditions", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Exif::Photo::CFAPattern", null, "Exif::Conditions", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);

		MetadataInfo.register ("Exif::Image::ImageWidth", null, "Exif::Structure", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Exif::Image::ImageLength", null, "Exif::Structure", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Exif::Image::Orientation", null, "Exif::Structure", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Exif::Photo::PixelXDimension", null, "Exif::Structure", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Exif::Photo::PixelYDimension", null, "Exif::Structure", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Exif::Image::XResolution", null, "Exif::Structure", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Exif::Image::YResolution", null, "Exif::Structure", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Exif::Image::ResolutionUnit", null, "Exif::Structure", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Exif::Image::Compression", null, "Exif::Structure", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Exif::Image::SamplesPerPixel", null, "Exif::Structure", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Exif::Image::BitsPerSample", null, "Exif::Structure", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Exif::Image::PlanarConfiguration", null, "Exif::Structure", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Exif::Image::YCbCrSubSampling", null, "Exif::Structure", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Exif::Image::YCbCrPositioning", null, "Exif::Structure", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Exif::Image::PhotometricInterpretation", null, "Exif::Structure", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Exif::Photo::ComponentsConfiguration", null, "Exif::Structure", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Exif::Photo::CompressedBitsPerPixel", null, "Exif::Structure", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Exif::Photo::StripOffset", null, "Exif::Structure", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Exif::Photo::RowsPerStrip", null, "Exif::Structure", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Exif::Photo::StripByteCounts", null, "Exif::Structure", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Exif::Photo::JPEGInterchangeFormat", null, "Exif::Structure", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Exif::Photo::JPEGInterchangeFormatLength", null, "Exif::Structure", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Exif::Photo::TransferFunction", null, "Exif::Structure", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Exif::Photo::WhitePoint", null, "Exif::Structure", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Exif::Photo::PrimaryChromaticities", null, "Exif::Structure", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Exif::Photo::YCbCrCoefficients", null, "Exif::Structure", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Exif::Photo::ReferenceBlackWhite", null, "Exif::Structure", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Exif::Photo::ColorSpace", null, "Exif::Structure", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);

		MetadataInfo.register ("Exif::Photo::ExifVersion", null, "Exif::Versions", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Exif::Image::ExifTag", null, "Exif::Versions", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Exif::Photo::FlashpixVersion", null, "Exif::Versions", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Exif::Iop::InteroperabilityIndex", null, "Exif::Versions", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Exif::Iop::InteroperabilityVersion", null, "Exif::Versions", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Exif::Photo::InteroperabilityTag", null, "Exif::Versions", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Exif::Photo::RelatedImageFileFormat", null, "Exif::Versions", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Exif::Photo::RelatedImageWidth", null, "Exif::Versions", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Exif::Photo::RelatedImageLength", null, "Exif::Versions", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);

		MetadataInfo.register ("Exif::Photo::MakerNote", null, "Exif::Other", MetadataFlags.HIDDEN);
		MetadataInfo.register ("Exif::Photo::UserComment", null, "Exif::Other", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW, "Ascii");

		MetadataInfo.register ("Xmp::dc::description", null, "Xmp::Embedded", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW, "XmpText");
		MetadataInfo.register ("Xmp::dc::title", null, "Xmp::Embedded", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW, "XmpText");
		MetadataInfo.register ("Xmp::iptc::Location", null, "Xmp::Embedded", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW, "XmpText");
		MetadataInfo.register ("Xmp::iptc::Keywords", null, "Xmp::Embedded", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW, "XmpBag");
		MetadataInfo.register ("Xmp::exif::DateTimeOriginal", null, "Xmp::Embedded", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW, "XmpText");
		MetadataInfo.register ("Xmp::xmp::Rating", null, "Xmp::Embedded", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW, "XmpText");

		MetadataInfo.register ("Iptc::Application2::Headline", null, "Iptc", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW, "String");
		MetadataInfo.register ("Iptc::Application2::Caption", null, "Iptc", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW, "String");
		MetadataInfo.register ("Iptc::Application2::LocationName", null, "Iptc", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW, "String");
		MetadataInfo.register ("Iptc::Application2::Keywords", null, "Iptc", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW, "String");

		// The editable fields specified in the "Other" tab of the "edit metadata" dialog.

		MetadataInfo.register ("Iptc::Application2::Copyright", null, "Iptc", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW | MetadataFlags.ALLOW_IN_PRINT, "String");
		MetadataInfo.register ("Iptc::Application2::Credit", null, "Iptc", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW | MetadataFlags.ALLOW_IN_PRINT, "String");
		MetadataInfo.register ("Iptc::Application2::Byline", null, "Iptc", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW | MetadataFlags.ALLOW_IN_PRINT, "String");
		MetadataInfo.register ("Iptc::Application2::BylineTitle", null, "Iptc", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW | MetadataFlags.ALLOW_IN_PRINT, "String");
		MetadataInfo.register ("Iptc::Application2::CountryName", null, "Iptc", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW | MetadataFlags.ALLOW_IN_PRINT, "String");
		MetadataInfo.register ("Iptc::Application2::CountryCode", null, "Iptc",  MetadataFlags.ALLOW_IN_PROPERTIES_VIEW | MetadataFlags.ALLOW_IN_PRINT, "String");
		MetadataInfo.register ("Iptc::Application2::ProvinceState", null, "Iptc",  MetadataFlags.ALLOW_IN_PROPERTIES_VIEW | MetadataFlags.ALLOW_IN_PRINT, "String");
		MetadataInfo.register ("Iptc::Application2::City", null, "Iptc", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW | MetadataFlags.ALLOW_IN_PRINT, "String");
		MetadataInfo.register ("Iptc::Application2::Language", null, "Iptc", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW | MetadataFlags.ALLOW_IN_PRINT, "String");
		MetadataInfo.register ("Iptc::Application2::ObjectName", null, "Iptc", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW | MetadataFlags.ALLOW_IN_PRINT, "String");
		MetadataInfo.register ("Iptc::Application2::Source", null, "Iptc", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW | MetadataFlags.ALLOW_IN_PRINT, "String");
		MetadataInfo.register ("Iptc::Envelope::Destination", null, "Iptc", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW | MetadataFlags.ALLOW_IN_PRINT, "String");
		MetadataInfo.register ("Iptc::Application2::Urgency", null, "Iptc", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW | MetadataFlags.ALLOW_IN_PRINT, "String");
	}
}
