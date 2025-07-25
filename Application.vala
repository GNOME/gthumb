public class Gth.Application : Adw.Application {
	public GLib.Settings settings;
	public HashTable<string, Gth.Test> tests;
	public GenericList<Gth.Test> ordered_tests;
	public HashTable<string, Gth.SortInfo?> sorters;
	public GenericArray<string> ordered_sorters;
	public GenericArray<FileSource> file_sources;
	public GenericArray<MetadataProvider> metadata_providers;
	public HashTable<string, Gth.LoadFunc> loaders;
	public HashTable<string, Gth.LoadFileFunc> external_loaders;
	public HashTable<string, Gth.SaveFunc> savers;
	public HashTable<string, GLib.Type> saver_preferences;
	public HashTable<string, GLib.Type> viewers;
	public Gth.FilterFile filter_file;
	public bool restart;
	public bool quitting;
	public Gth.JobQueue jobs;
	public ImageLoader image_loader;
	public ThumbLoader thumb_loader;
	public ImageSaver image_saver;
	public ColorManager color_manager;
	public GenericList<FileData> roots;
	public Monitor monitor;
	public Bookmarks bookmarks;

	public Application () {
		Object (
			application_id: "app.gthumb.gthumb",
			register_session: true,
			flags: ApplicationFlags.HANDLES_COMMAND_LINE
		);

		restart = false;
		quitting = false;
		jobs = new Gth.JobQueue ();
		jobs.size_changed.connect (() => {
			foreach_window ((win) => win.browser.status.set_n_jobs (jobs.size ()));
		});
		io_factory = new Work.Factory (get_workers (MAX_IO_WORKERS));
		image_loader = new ImageLoader (io_factory);
		thumb_loader = new ThumbLoader (io_factory);
		image_saver = new ImageSaver (io_factory);
		color_manager = new ColorManager ();
		roots = new GenericList<FileData>();
		monitor = new Monitor ();
		bookmarks = new Bookmarks ();

		tests = new HashTable<string, Gth.Test>(str_hash, str_equal);
		ordered_tests = new GenericList<Gth.Test>();
		register_test (typeof (Gth.TestFileName));
		register_test (typeof (Gth.TestFileSize));
		register_test (typeof (Gth.TestFileModifiedTime));
		register_test (typeof (Gth.TestFileCreatedTime));
		register_test (typeof (Gth.TestFileTypeRegular));
		register_test (typeof (Gth.TestFileTypeMedia));
		register_test (typeof (Gth.TestFileTypeImage));
		register_test (typeof (Gth.TestFileTypeVideo));
		register_test (typeof (Gth.TestFileTypeAudio));
		register_test (typeof (Gth.TestFileTypeJpeg));
		register_test (typeof (Gth.TestFileTypeRaw));
		//register_test (typeof (Gth.TestFileTypeText));
		register_test (typeof (Gth.TestTimeOriginal));
		register_test (typeof (Gth.TestTitleEmbedded));
		register_test (typeof (Gth.TestDescriptionEmbedded));
		register_test (typeof (Gth.TestTagEmbedded));
		register_test (typeof (Gth.TestRating));
		register_test (typeof (Gth.TestFrameAspectRatio));
		register_test (typeof (Gth.TestVisible));

		sorters = new HashTable<string, Gth.SortInfo?>(str_hash, str_equal);
		ordered_sorters = new GenericArray<string>();
		register_sorter ({ "file::name", _("Name"), "standard::display-name", Sorters.cmp_basename });
		register_sorter ({ "file::mtime", _("Modified"), "time::modified,time::modified-usec", Sorters.cmp_modified_time });
		register_sorter ({ "file::size", _("Bytes"), "standard::size", Sorters.cmp_size });
		register_sorter ({ "file::path", _("Path and Name"), "standard::display-name", Sorters.cmp_uri });
		register_sorter ({ "general::dimensions", _("Width and Height"), "frame::width,frame::height", Sorters.cmp_frame_dimensions });
		register_sorter ({ "frame::aspect-ratio", _("Aspect Ratio"), "frame::width,frame::height", Sorters.cmp_aspect_ratio });
		register_sorter ({ "general::unsorted", _("Unsorted"), "", null });

		file_sources = new GenericArray<FileSource>();
		register_source (typeof (Gth.FileSourceVfs));
		register_source (typeof (Gth.FileSourceCatalogs));

		MetadataCategory.init ();
		MetadataCategory.register ("file", _("File"));
		MetadataCategory.register ("comment", _("Description"));
		MetadataCategory.register ("general", _("Metadata"));
		MetadataCategory.register ("Exif::General", _("Exif General"));
		MetadataCategory.register ("Exif::Conditions", _("Exif Conditions"));
		MetadataCategory.register ("Exif::Structure", _("Exif Structure"));
		MetadataCategory.register ("Exif::Thumbnail", _("Exif Thumbnail"));
		MetadataCategory.register ("Exif::GPS", _("Exif GPS"));
		MetadataCategory.register ("Exif::MakerNotes", _("Exif Maker Notes"));
		MetadataCategory.register ("Exif::Versions", _("Exif Versions"));
		MetadataCategory.register ("Exif::Other", _("Exif Other"));
		MetadataCategory.register ("Iptc", _("IPTC"));
		MetadataCategory.register ("Xmp::Embedded", _("XMP Embedded"));
		MetadataCategory.register ("Xmp::Sidecar", _("XMP Attached"));

		MetadataInfo.init ();
		MetadataInfo.register ("standard::display-name", N_("Name"), "file", METADATA_ALLOW_EVERYWHERE);
		MetadataInfo.register ("gth::file::content-type", N_("Type"), "file", METADATA_ALLOW_EVERYWHERE);
		MetadataInfo.register ("gth::file::display-size", N_("Bytes"), "file", METADATA_ALLOW_EVERYWHERE);
		//MetadataInfo.register ("gth::file::size", N_("Bytes"), "file", METADATA_ALLOW_EVERYWHERE);
		// Translators: the file modified time.
		MetadataInfo.register ("gth::file::display-mtime", N_("Modified"), "file", METADATA_ALLOW_EVERYWHERE);
		MetadataInfo.register ("gth::file::location", N_("Folder"), "file", MetadataFlags.ALLOW_IN_PRINT | MetadataFlags.ALLOW_IN_FILE_LIST | MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("gth::file::is-modified", null, "file", MetadataFlags.HIDDEN);

		// Translators: width and height of the image/video.
		MetadataInfo.register ("general::dimensions", N_("Pixels"), "file", METADATA_ALLOW_EVERYWHERE);
		MetadataInfo.register ("general::duration", N_("Duration"), "file", METADATA_ALLOW_EVERYWHERE);
		MetadataInfo.register ("Loaded::Image::ColorProfile", N_("Color Profile"), "file", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);

		MetadataInfo.register ("general::title", N_("Title"), "comment", METADATA_ALLOW_EVERYWHERE);
		MetadataInfo.register ("general::description", N_("Comment"), "comment", MetadataFlags.ALLOW_IN_PRINT | MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("general::location", N_("Place"), "comment", METADATA_ALLOW_EVERYWHERE);
		MetadataInfo.register ("general::datetime", N_("Date"), "comment", METADATA_ALLOW_EVERYWHERE);
		MetadataInfo.register ("general::tags", N_("Tags"), "comment", METADATA_ALLOW_EVERYWHERE);
		MetadataInfo.register ("general::rating", N_("Rating"), "comment", METADATA_ALLOW_EVERYWHERE);

		MetadataInfo.register ("Embedded::Photo::Copyright", N_("Copyright"), "general", METADATA_ALLOW_EVERYWHERE);
		MetadataInfo.register ("Embedded::Photo::Author", N_("Author"), "general", METADATA_ALLOW_EVERYWHERE);
		MetadataInfo.register ("Embedded::Photo::Coordinates", N_("Coordinates"), "general", METADATA_ALLOW_EVERYWHERE);

		MetadataInfo.register ("Embedded::Photo::CameraModel", N_("Camera Model"), "general", METADATA_ALLOW_EVERYWHERE);
		MetadataInfo.register ("Embedded::Photo::Exposure", N_("Exposure"), "general", MetadataFlags.ALLOW_IN_PRINT | MetadataFlags.ALLOW_IN_FILE_LIST);
		MetadataInfo.register ("Embedded::Photo::Aperture", N_("Aperture"), "general",  METADATA_ALLOW_EVERYWHERE);
		MetadataInfo.register ("Embedded::Photo::ISOSpeed", N_("ISO Speed"), "general", METADATA_ALLOW_EVERYWHERE);
		MetadataInfo.register ("Embedded::Photo::ExposureTime", N_("Exposure Time"), "general", METADATA_ALLOW_EVERYWHERE);
		MetadataInfo.register ("Embedded::Photo::ShutterSpeed", N_("Shutter Speed"), "general", METADATA_ALLOW_EVERYWHERE);
		MetadataInfo.register ("Embedded::Photo::FocalLength", N_("Focal Length"), "general", METADATA_ALLOW_EVERYWHERE);
		MetadataInfo.register ("Embedded::Photo::Flash", N_("Flash"), "general", METADATA_ALLOW_EVERYWHERE);

		MetadataInfo.register ("gth::file::emblems", "", "", MetadataFlags.HIDDEN);
		MetadataInfo.register ("gth::standard::secondary-sort-order", "", "", MetadataFlags.HIDDEN);
		MetadataInfo.register ("image::width", "", "", MetadataFlags.HIDDEN);
		MetadataInfo.register ("image::height", "", "", MetadataFlags.HIDDEN);
		MetadataInfo.register ("frame::width", "", "", MetadataFlags.HIDDEN);
		MetadataInfo.register ("frame::height", "", "", MetadataFlags.HIDDEN);
		MetadataInfo.register ("Embedded::Image::Orientation", "", "", MetadataFlags.HIDDEN);
		MetadataInfo.register ("Embedded::Photo::DateTimeOriginal", "", "", MetadataFlags.HIDDEN);

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
		MetadataInfo.register ("Iptc::Application2::City", null, "Iptc", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW | MetadataFlags.ALLOW_IN_PRINT, "String");
		MetadataInfo.register ("Iptc::Application2::Language", null, "Iptc", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW | MetadataFlags.ALLOW_IN_PRINT, "String");
		MetadataInfo.register ("Iptc::Application2::ObjectName", null, "Iptc", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW | MetadataFlags.ALLOW_IN_PRINT, "String");
		MetadataInfo.register ("Iptc::Application2::Source", null, "Iptc", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW | MetadataFlags.ALLOW_IN_PRINT, "String");
		MetadataInfo.register ("Iptc::Envelope::Destination", null, "Iptc", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW | MetadataFlags.ALLOW_IN_PRINT, "String");
		MetadataInfo.register ("Iptc::Application2::Urgency", null, "Iptc", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW | MetadataFlags.ALLOW_IN_PRINT, "String");

		metadata_providers = new GenericArray<MetadataProvider>();
		register_metadata_provider (typeof (MetadataProviderFile));
		register_metadata_provider (typeof (MetadataProviderExiv2));
		register_metadata_provider (typeof (MetadataProviderImage));
		register_metadata_provider (typeof (MetadataProviderComment));
		register_metadata_provider (typeof (VideoMetadataProvider));

		loaders = new HashTable<string, Gth.LoadFunc>(str_hash, str_equal);
		register_image_loader ("image/png", load_png);
		register_image_loader ("image/jpeg", load_jpeg);
#if HAVE_LIBWEBP
		register_image_loader ("image/webp", load_webp);
#endif
#if HAVE_LIBRSVG
		register_image_loader ("image/svg+xml", load_svg);
#endif
#if HAVE_LIBJXL
		register_image_loader ("image/jxl", load_jxl);
#endif
#if HAVE_LIBHEIF
		register_image_loader ("image/heif", load_heif);
		register_image_loader ("image/heic", load_heif);
		register_image_loader ("image/avif", load_heif);
#endif

		external_loaders = new HashTable<string, Gth.LoadFileFunc>(str_hash, str_equal);
#if HAVE_GSTREAMER
		// Same types listed in Util.content_type_is_video
		register_external_loader ("video/*", load_video_thumbnail);
		register_external_loader ("application/ogg", load_video_thumbnail);
		register_external_loader ("application/x-matroska", load_video_thumbnail);
		register_external_loader ("application/vnd.ms-asf", load_video_thumbnail);
		register_external_loader ("application/vnd.rn-realmedia", load_video_thumbnail);
#endif

		savers = new HashTable<string, Gth.SaveFunc>(str_hash, str_equal);
		saver_preferences = new HashTable<string, GLib.Type>(str_hash, str_equal);
		register_image_saver ("image/png", save_png, typeof (PngPreferences));
		register_image_saver ("image/jpeg", save_jpeg, typeof (JpegPreferences));

		viewers = new HashTable<string, GLib.Type>(str_hash, str_equal);
		register_file_viewer ("image/png", typeof (ImageViewer));
		register_file_viewer ("image/jpeg", typeof (ImageViewer));
		register_file_viewer ("video/*", typeof (VideoViewer));
		register_file_viewer ("audio/*", typeof (VideoViewer));
	}

	public override void startup () {
		base.startup ();
		init_settings ();
		init_actions ();
	}

	public override bool local_command_line (ref unowned string[] arguments, out int exit_status) {
		var handled_locally = false;
		exit_status = 0;

		// Parse command line options.
		string[] local_arguments = arguments;
		var context = get_command_line_option_context ();
		try {
			context.parse_strv (ref local_arguments);
		}
		catch (OptionError e) {
			critical ("Failed to parse arguments: %s", e.message);
			stderr.printf ("%s", context.get_help (true, null));
			handled_locally = true;
			exit_status = 1;
		}

		if (arg_version) {
			stdout.printf ("Thumb 0, Copyright © 2001-2025 The Thumb Authors.\n");
			handled_locally = true;
		}

		return handled_locally;
	}

	public override int command_line (ApplicationCommandLine command_line) {
		// Parse command line options.
		var context = get_command_line_option_context ();
		try {
			var arguments = command_line.get_arguments ();
			context.parse_strv (ref arguments);
		}
		catch (OptionError e) {
			critical ("Failed to parse arguments: %s", e.message);
			stderr.printf ("%s", context.get_help (true, null));
			return 1;
		}

		// Exec the command line.

		if (arg_import_photos) {
			// TODO
			return 0;
		}

		if (remaining_args == null) {
			// No location specified.
			var location = Gth.Settings.get_startup_location (settings);
			var file_to_select = Gth.Settings.get_file (settings, PREF_BROWSER_STARTUP_CURRENT_FILE);
			open_window (location, file_to_select, true);
			return 0;
		}

		// At least a location was specified.
		// TODO
		return 0;
	}

	public void register_sorter (Gth.SortInfo sorter) {
		sorters.insert (sorter.id, sorter);
		ordered_sorters.add (sorter.id);
	}

	public unowned Gth.SortInfo? get_sorter_by_id (string? id) {
		return (id != null) ? sorters[id] : null;
	}

	public unowned Gth.SortInfo? get_folder_sorter_by_id (string? id) {
		return get_sorter_by_id ((id == "modification-time") ? "file::mtime" : "file::name");
	}

	public void register_test (GLib.Type test_type) {
		var test = Object.new (test_type) as Gth.Test;
		tests.insert (test.id, test);
		ordered_tests.model.append (test);
	}

	public Gth.Test? get_test_by_id (string id) {
		return tests[id];
	}

	public Gth.Test? get_general_filter () {
		var filter_id = settings.get_string (PREF_BROWSER_GENERAL_FILTER);
		if (filter_id == null)
			return null;
		return get_test_by_id (filter_id);
	}

	public Gth.Filter add_general_filter (Gth.Test? active_test) {
		if ((active_test == null) || Strings.empty (active_test.id)) {
			// The active test is not set, use the general filter.
			var tests = new Gth.TestExpr (TestExpr.Operation.INTERSECTION);
			tests.add (get_general_filter ());
			var filter = new Gth.Filter ();
			filter.tests = tests;
			return filter;
		}

		if (active_test is Gth.Filter) {
			// The active test is a filter, add the general filter to it if
			// it doesn't contain a file type test.
			var original_filter = active_test as Gth.Filter;
			var filter = original_filter.duplicate () as Gth.Filter;
			if (!original_filter.tests.contains_type_test ()) {
				var tests = new Gth.TestExpr (TestExpr.Operation.INTERSECTION);
				if (!original_filter.tests.is_empty ()) {
					tests.add (original_filter.tests);
				}
				tests.add (get_general_filter ());
				filter.tests = tests;
			}
			return filter;
		}

		// The active test is a simple test, create a filter adding the general
		// filter as well if the active test is not a file type test.
		var filter = new Gth.Filter ();
		var tests = new Gth.TestExpr (TestExpr.Operation.INTERSECTION);
		if (!(active_test is TestFileType)) {
			tests.add (get_general_filter ());
		}
		tests.add (active_test);
		filter.tests = tests;
		return filter;
	}

	bool active_filter_loaded = false;

	public Gth.Test? get_last_active_filter () {
		if (active_filter_loaded) {
			// Restore the filter only in the first window.
			// TODO: use the active window filter.
			return null;
		}
		Gth.Test filter = null;
		var file = Gth.UserDir.get_config_file (Gth.FileIntent.READ, "active_filter.xml");
		if (file != null) {
			try {
				var doc = new Dom.Document ();
				doc.load_file (file);
				if (doc.first_child != null) {
					var id = doc.first_child.get_attribute ("id");
					if (!Strings.empty (id)) {
						var test = get_test_by_id (id);
						if (test != null) {
							filter = test.duplicate ();
						}
						else {
							filter = new Gth.Filter ();
						}
						filter.load_from_element (doc.first_child);
					}
				}
			}
			catch (Error error) {
				// Ignored.
			}
		}
		active_filter_loaded = true;
		return filter;
	}

	public void save_active_filter (Gth.Test? test) {
		var file = Gth.UserDir.get_config_file (Gth.FileIntent.WRITE, "active_filter.xml");
		if (file == null)
			return;

		var content = "";
		if (test != null) {
			var doc = new Dom.Document ();
			doc.append_child (test.create_element (doc));
			content = doc.to_xml ();
		}
		try {
			Files.save_content (file, content, null);
		}
		catch (Error error) {
		}
	}

	public GenericList<Gth.Test> get_visible_filters () {
		var filters = new GenericList<Gth.Test>();

		var no_filter = new Gth.Test ();
		no_filter.id = "";
		var general_filter = get_general_filter ();
		if (general_filter != null) {
			no_filter.display_name = general_filter.display_name;
		}
		else {
			no_filter.display_name = _("All Files");
		}
		filters.model.append (no_filter);

		foreach (var filter in filter_file.filters) {
			if (filter.visible) {
				filters.model.append (filter.duplicate ());
			}
		}
		return filters;
	}

	public GenericList<Gth.Test> get_file_type_filters () {
		var filters = new GenericList<Gth.Test> ( );
		foreach (unowned var filter in ordered_tests) {
			if (filter.id.has_prefix ("file::type::")) {
				filters.model.append (filter.duplicate ());
			}
		}
		return filters;
	}

	public void register_source (GLib.Type source_type) {
		var source = Object.new (source_type) as Gth.FileSource;
		file_sources.add (source);
	}

	public inline FileSource get_source_for_file (File file) {
		return get_source_for_uri (file.get_uri ());
	}

	public FileSource get_source_for_uri (string uri) {
		var source_type = typeof (FileSourceVfs);
		for (var idx = file_sources.length - 1; idx >= 0; idx--) {
			unowned var source = file_sources[idx];
			if (source.supports_scheme (uri)) {
				source_type = source.get_class ().get_type ();
				break;
			}
		}
		return Object.new (source_type) as FileSource;
	}

	public void register_image_loader (string content_type, LoadFunc func) {
		loaders.set (content_type, func);
	}

	public LoadFunc? get_load_func (string content_type) {
		return loaders.get (content_type);
	}

	public void register_external_loader (string content_type, LoadFileFunc func) {
		external_loaders.set (content_type, func);
	}

	public LoadFileFunc? get_load_file_func (string content_type) {
		LoadFileFunc func = null;
		if (!external_loaders.contains (content_type)) {
			// 'video/mp4' -> 'video/*'
			var generic_type = Util.get_generic_type (content_type);
			func = external_loaders.get (generic_type);
			external_loaders.set (content_type, func);
		}
		return external_loaders.get (content_type);
	}

	public void register_image_saver (string content_type, SaveFunc func, GLib.Type options_type) {
		savers.set (content_type, func);
		if (options_type != 0) {
			saver_preferences.set (content_type, options_type);
		}
	}

	public SaveFunc? get_save_func (string content_type) {
		return savers.get (content_type);
	}

	public SaverPreferences? get_saver_preferences (string content_type) {
		var options_type = saver_preferences.get (content_type);
		if (options_type == 0)
			return null;
		return Object.new (options_type) as Gth.SaverPreferences;
	}

	public void register_metadata_provider (GLib.Type provider_type) {
		var provider = Object.new (provider_type) as Gth.MetadataProvider;
		metadata_providers.add (provider);
	}

	public void register_file_viewer (string content_type, GLib.Type viewer_type) {
		viewers.set (content_type, viewer_type);
	}

	GLib.Type get_viewer_type_for_content_type (string content_type) {
		if (!viewers.contains (content_type)) {
			var generic_type = Util.get_generic_type (content_type);
			var viewer_type = viewers.get (generic_type);
			if (viewer_type == 0) {
				viewer_type = typeof (UnknownViewer);
			}
			viewers.set (content_type, viewer_type);
		}
		return viewers.get (content_type);
	}

	public FileViewer? get_viewer_for_type (string content_type) {
		var type = get_viewer_type_for_content_type (content_type);
		if ((type == 0) || (type == typeof (UnknownViewer)))
			return null;
		return Object.new (type) as Gth.FileViewer;
	}

	public bool viewer_can_view (GLib.Type viewer_type, string content_type) {
		return get_viewer_type_for_content_type (content_type) == viewer_type;
	}

	public inline Gth.Job new_job (string description, string? status = null, bool background = false) {
		return jobs.new_job (description, status, background);
	}

	public inline Gth.Job new_background_job (string description, string? status = null) {
		return new_job (description, status, true);
	}

	public void foreach_window (Gth.WindowFunc func) {
		foreach (var w in get_windows ()) {
			var win = w as Gth.Window;
			if (win != null)
				func (win);
		}
	}

	public bool one_window () {
		unowned var list = get_windows ();
		return (list == null) || (list.next == null);
	}

	const int MAX_ROOTS_PER_SOURCE = 1000;

	public async GenericList<FileData> update_roots () {
		roots.model.remove_all ();
		var job = new_job ("Update roots");
		var sort_order = 0;
		foreach (unowned var source in file_sources) {
			var source_roots = yield source.get_roots (job.cancellable);
			if (source_roots == null)
				continue;
			foreach (unowned var root in source_roots) {
				root.info.set_sort_order (sort_order++);
				roots.model.append (root);
			}
			sort_order += MAX_ROOTS_PER_SOURCE;
		}
		job.done ();
		return roots;
	}

	bool arg_version = false;
	bool arg_new_window = false;
	bool arg_fullscreen = false;
	bool arg_slideshow = false;
	bool arg_import_photos = false;
	[CCode (array_length = false, array_null_terminated = true)]
	string[]? remaining_args = null;

	OptionContext get_command_line_option_context () {
		GLib.OptionEntry[] option_entries = {
			{
				"version",
				'v',
				OptionFlags.NONE,
				OptionArg.NONE,
				ref arg_version,
				N_("Show the application version"),
				null
			},
			{
				"new-window",
				'n',
				OptionFlags.NONE,
				OptionArg.NONE,
				ref arg_new_window,
				N_("Open a new window"),
				null
			},
			{
				"fullscreen",
				'f',
				OptionFlags.NONE,
				OptionArg.NONE,
				ref arg_fullscreen,
				N_("Open a new window"),
				null
			},
			{
				"slideshow",
				's',
				OptionFlags.NONE,
				OptionArg.NONE,
				ref arg_slideshow,
				N_("Automatically start a presentation"),
				null
			},
			{
				"import-photos",
				'i',
				OptionFlags.NONE,
				OptionArg.NONE,
				ref arg_import_photos,
				N_("Automatically import digital camera photos"),
				null
			},
			{
				GLib.OPTION_REMAINING,
				0,
				OptionFlags.NONE,
				OptionArg.STRING_ARRAY,
				ref remaining_args,
				null,
				N_("[FILE…] [DIRECTORY…]")
			},
			{ null }
		};
		var context = new OptionContext (_("— Image Browser and Viewer"));
		context.set_help_enabled (true);
		context.set_translation_domain (Config.GETTEXT_PACKAGE);
		context.set_ignore_unknown_options (true);
		context.add_main_entries (option_entries, null);
		return context;
	}

	void init_settings () {
		var style_manager = Adw.StyleManager.get_default ();
		style_manager.color_scheme = Adw.ColorScheme.FORCE_DARK;

		var css_provider = new Gtk.CssProvider ();
		Gtk.StyleContext.add_provider_for_display (Gdk.Display.get_default (), css_provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION);
		css_provider.load_from_resource ("/app/gthumb/gthumb/css/style.css");

		var icon_theme = Gtk.IconTheme.get_for_display (Gdk.Display.get_default ());
		icon_theme.add_resource_path ("/app/gthumb/gthumb/icons");

		settings = new GLib.Settings (GTHUMB_SCHEMA);
		filter_file = new Gth.FilterFile ();
	}

	void init_actions () {
		var action = new SimpleAction ("about", null);
		action.activate.connect (() => {
			const string[] developers = {
				"Paolo Bacchilega <paobac@src.gnome.org>",
			};
			Adw.show_about_dialog (active_window,
				"application-name", "Thumbnails",
				"application-icon", "app.gthumb.gthumb",
				"version", Config.VERSION,
				"license-type", Gtk.License.GPL_2_0,
				"translator-credits", _("translator-credits"),
				"website", "https://gitlab.gnome.org/GNOME/gthumb/",
				"issue-url", "https://gitlab.gnome.org/GNOME/gthumb/-/issues",
				"developers", developers
			);
		});
		add_action (action);

		action = new SimpleAction ("preferences", null);
		action.activate.connect (() => {
			var dialog = new Gth.PreferencesDialog ();
			dialog.present (active_window);
		});
		add_action (action);

		action = new SimpleAction ("open-new-window", VariantType.STRING);
		action.activate.connect ((_action, param) => {
			var file = File.new_for_uri (param.get_string ());
			open_window (file.get_parent (), file, true);
		});
		add_action (action);
	}

	public void open_window (File location, File? file_to_select = null, bool force_new_window = false) {
		var reuse_active_window = false;
		if (!force_new_window) {
			reuse_active_window = settings.get_boolean (PREF_BROWSER_REUSE_ACTIVE_WINDOW);
		}

		Gth.Window window = null;
		if (reuse_active_window) {
			window = active_window as Gth.Window;
		}

		if (window == null) {
			window = new Gth.Window (this, location, file_to_select);
		}
		else {
			window.browser.open_location (location, LoadAction.OPEN, file_to_select);
		}

		if (!arg_slideshow) {
			window.present ();
		}
	}

	uint get_workers (int max_workers) {
		var n_workers = GLib.get_num_processors () - 1;
		return n_workers.clamp (1, max_workers);
	}

	const int MAX_IO_WORKERS = 4;
	Work.Factory io_factory;
}

public delegate void Gth.WindowFunc (Gth.Window win);
