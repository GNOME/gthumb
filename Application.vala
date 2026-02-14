public class Gth.Application : Adw.Application {
	public GLib.Settings settings;
	public GLib.Settings viewer_settings;
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
	public HashTable<string, string> saver_extensions;
	public HashTable<string, GLib.Type> viewers;
	public Gth.Filters filters;
	public bool restart;
	public bool quitting;
	public Gth.JobQueue jobs;
	public ImageLoader image_loader;
	public ThumbLoader thumb_loader;
	public ImageSaver image_saver;
	public ColorManager color_manager;
	public MetadataReader metadata_reader;
	public MetadataWriter metadata_writer;
	public GenericList<FileData> roots;
	public Monitor monitor;
	public Bookmarks bookmarks;
	public Migration migration;
	public Scripts scripts;
	public ImageEditor image_editor;
	public Shortcuts shortcuts;
	public Work.Factory io_factory;
	public FileActions tools;
	public Selections selections;

	public Application () {
		Object (
			application_id: Config.APP_ID,
			register_session: true,
			flags: ApplicationFlags.HANDLES_COMMAND_LINE
		);
	}

	public override void startup () {
		base.startup ();

		restart = false;
		quitting = false;
		jobs = new Gth.JobQueue ();
		io_factory = new Work.Factory (Util.get_workers (MAX_IO_WORKERS));
		image_loader = new ImageLoader (io_factory);
		thumb_loader = new ThumbLoader (io_factory);
		image_saver = new ImageSaver (io_factory);
		metadata_reader = new MetadataReader (io_factory);
		metadata_writer = new MetadataWriter (io_factory);
		color_manager = new ColorManager ();
		roots = new GenericList<FileData>();
		monitor = new Monitor ();
		bookmarks = new Bookmarks ();
		migration = new Migration ();
		image_editor = new ImageEditor ();
		filters = new Filters ();
		scripts = new Scripts ();
		tools = new FileActions ();
		selections = new Selections ();

		tests = new HashTable<string, Gth.Test>(str_hash, str_equal);
		ordered_tests = new GenericList<Gth.Test>();
		register_test (typeof (Gth.TestFileName));
		register_test (typeof (Gth.TestFileSize));
		register_test (typeof (Gth.TestContentModifiedTime));
		register_test (typeof (Gth.TestFileCreatedTime));
		register_test (typeof (Gth.TestFileChangedTime));
		register_test (typeof (Gth.TestFileTypeRegular));
		register_test (typeof (Gth.TestFileTypeMedia));
		register_test (typeof (Gth.TestFileTypeImage));
		register_test (typeof (Gth.TestFileTypeVideo));
		register_test (typeof (Gth.TestFileTypeAudio));
		//register_test (typeof (Gth.TestFileTypeText));
		register_test (typeof (Gth.TestTimeOriginal));
		register_test (typeof (Gth.TestTitle));
		register_test (typeof (Gth.TestDescription));
		register_test (typeof (Gth.TestLocation));
		register_test (typeof (Gth.TestTag));
		register_test (typeof (Gth.TestRating));
		register_test (typeof (Gth.TestFrameAspectRatio));
		register_test (typeof (Gth.TestVisible));

		sorters = new HashTable<string, Gth.SortInfo?>(str_hash, str_equal);
		ordered_sorters = new GenericArray<string>();
		register_sorter ({ "File::Name", _("Name"), "standard::display-name", Sorters.cmp_basename });
		register_sorter ({ "Time::Modified", _("Modified"), "time::modified,time::modified-usec", Sorters.cmp_modified_time });
		register_sorter ({ "Time::Changed", _("File Changed"), "time::changed,time::changed-usec", Sorters.cmp_changed_time });
		register_sorter ({ "Time::Created", _("File Created"), "time::created,time::created-usec", Sorters.cmp_created_time });
		register_sorter ({ "File::Size", _("Bytes"), "standard::size", Sorters.cmp_size });
		register_sorter ({ "File::Path", _("File Path"), "standard::display-name", Sorters.cmp_uri });
		register_sorter ({ "Frame::Pixels", _("Pixels"), "Frame::Width,Frame::Height", Sorters.cmp_frame_dimensions });
		register_sorter ({ "Frame::AspectRatio", _("Aspect Ratio"), "Frame::Width,Frame::Height", Sorters.cmp_aspect_ratio });
		register_sorter ({ "Private::Unsorted", _("Unsorted"), "", Sorters.cmp_position });

		file_sources = new GenericArray<FileSource>();
		register_source (typeof (Gth.FileSourceVfs));
		register_source (typeof (Gth.FileSourceCatalogs));
		register_source (typeof (Gth.FileSourceSelections));

		MetadataCategory.init ();
		MetadataCategory.register ("File", N_("File"));
		MetadataCategory.register ("Comment", N_("Description"));
		MetadataCategory.register ("Metadata", N_("Metadata"));
		MetadataCategory.register ("Other", N_("Other"));
		ExifMetadata.register_categories ();

		MetadataInfo.init ();
		MetadataInfo.register ("standard::display-name", N_("Name"), "File", METADATA_ALLOW_EVERYWHERE);
		MetadataInfo.register ("Private::File::ContentType", N_("Type"), "File", METADATA_ALLOW_EVERYWHERE);
		MetadataInfo.register ("Private::File::DisplaySize", N_("Bytes"), "File", METADATA_ALLOW_EVERYWHERE);
		// Translators: the file modification time.
		MetadataInfo.register ("Private::File::DisplayModified", N_("Modified"), "File", METADATA_ALLOW_EVERYWHERE);
		// Translators: the file creation time.
		MetadataInfo.register ("Private::File::DisplayCreated", N_("File Created"), "File", MetadataFlags.ALLOW_IN_PRINT);
		// Translators: the file change time.
		MetadataInfo.register ("Private::File::DisplayChanged", N_("File Changed"), "File", METADATA_ALLOW_EVERYWHERE);
		MetadataInfo.register ("Private::File::Location", N_("Folder"), "File", MetadataFlags.ALLOW_IN_PRINT | MetadataFlags.ALLOW_IN_FILE_LIST | MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register (PrivateAttribute.LOADED_IMAGE_IS_MODIFIED, null, "File", MetadataFlags.HIDDEN);

		// Translators: width and height of images or videos.
		MetadataInfo.register ("Frame::Pixels", N_("Pixels"), "File", METADATA_ALLOW_EVERYWHERE);
		MetadataInfo.register ("Metadata::Duration", N_("Duration"), "File", METADATA_ALLOW_EVERYWHERE);
		MetadataInfo.register (PrivateAttribute.LOADED_IMAGE_COLOR_PROFILE, N_("Color Profile"), "File", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Animation::Frames", N_("Frames"), "File", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);

		MetadataInfo.register ("Metadata::Title", N_("Title"), "Comment", METADATA_ALLOW_EVERYWHERE);
		MetadataInfo.register ("Metadata::Description", N_("Comment"), "Comment", MetadataFlags.ALLOW_IN_PRINT | MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Metadata::Place", N_("Place"), "Comment", METADATA_ALLOW_EVERYWHERE);
		MetadataInfo.register ("Metadata::DateTime", N_("Date"), "Comment", METADATA_ALLOW_EVERYWHERE);
		MetadataInfo.register ("Metadata::Tags", N_("Tags"), "Comment", METADATA_ALLOW_EVERYWHERE);
		MetadataInfo.register ("Metadata::Rating", N_("Rating"), "Comment", METADATA_ALLOW_EVERYWHERE);

		MetadataInfo.register ("Metadata::Copyright", N_("Copyright"), "Metadata", METADATA_ALLOW_EVERYWHERE);
		MetadataInfo.register ("Metadata::Author", N_("Author"), "Metadata", METADATA_ALLOW_EVERYWHERE);
		MetadataInfo.register ("Metadata::Coordinates", N_("Coordinates"), "Metadata", METADATA_ALLOW_EVERYWHERE);

		MetadataInfo.register ("Photo::CameraModel", N_("Camera Model"), "Metadata", METADATA_ALLOW_EVERYWHERE);
		MetadataInfo.register ("Photo::Exposure", N_("Exposure"), "Metadata", MetadataFlags.ALLOW_IN_PRINT | MetadataFlags.ALLOW_IN_FILE_LIST);
		MetadataInfo.register ("Photo::Aperture", N_("Aperture"), "Metadata",  METADATA_ALLOW_EVERYWHERE);
		MetadataInfo.register ("Photo::ISOSpeed", N_("ISO Speed"), "Metadata", METADATA_ALLOW_EVERYWHERE);
		MetadataInfo.register ("Photo::ExposureTime", N_("Exposure Time"), "Metadata", METADATA_ALLOW_EVERYWHERE);
		MetadataInfo.register ("Photo::ShutterSpeed", N_("Shutter Speed"), "Metadata", METADATA_ALLOW_EVERYWHERE);
		MetadataInfo.register ("Photo::FocalLength", N_("Focal Length"), "Metadata", METADATA_ALLOW_EVERYWHERE);
		MetadataInfo.register ("Photo::Flash", N_("Flash"), "Metadata", METADATA_ALLOW_EVERYWHERE);

		VideoMetadata.register_info ();

		MetadataInfo.register ("Private::File::Emblems", "", "", MetadataFlags.HIDDEN);
		MetadataInfo.register ("Private::SecondarySortOrder", "", "", MetadataFlags.HIDDEN);
		MetadataInfo.register ("Frame::Width", "", "", MetadataFlags.HIDDEN);
		MetadataInfo.register ("Frame::Height", "", "", MetadataFlags.HIDDEN);
		MetadataInfo.register ("Photo::Orientation", "", "", MetadataFlags.HIDDEN);
		MetadataInfo.register ("Photo::DateTimeOriginal", "", "", MetadataFlags.HIDDEN);

		ExifMetadata.register_info ();

		metadata_providers = new GenericArray<MetadataProvider>();
		register_metadata_provider (typeof (FileMetadataProvider));
		register_metadata_provider (typeof (ExivMetadataProvider));
		register_metadata_provider (typeof (ImageMetadataProvider));
		register_metadata_provider (typeof (VideoMetadataProvider));
		// Always the last one, in order to give priority to the embedded
		// metadata if the file is newer than the comment file.
		register_metadata_provider (typeof (CommentMetadataProvider));

		loaders = new HashTable<string, Gth.LoadFunc>(str_hash, str_equal);
		viewers = new HashTable<string, GLib.Type>(str_hash, str_equal);

		register_image_loader ("image/png", load_png);
		register_image_loader ("image/apng", load_png);
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
#if HAVE_LIBTIFF
		register_image_loader ("image/tiff", load_tiff);
#endif
#if HAVE_LIBGIF
		register_image_loader ("image/gif", load_gif);
#endif

		external_loaders = new HashTable<string, Gth.LoadFileFunc>(str_hash, str_equal);
		register_external_loader ("video/*", load_video_thumbnail, typeof (VideoViewer));
		register_external_loader ("audio/*", load_video_thumbnail, typeof (VideoViewer));
		foreach (unowned var video_type in Other_Video_Types) {
			register_external_loader (video_type, load_video_thumbnail, typeof (VideoViewer));
		}

		savers = new HashTable<string, Gth.SaveFunc>(str_hash, str_equal);
		saver_preferences = new HashTable<string, GLib.Type>(str_hash, str_equal);
		saver_extensions = new HashTable<string, string>(str_hash, str_equal);
		register_image_saver ("image/jpeg", save_jpeg, typeof (JpegPreferences));
		register_image_saver ("image/png", save_png, typeof (PngPreferences));
#if HAVE_LIBWEBP
		register_image_saver ("image/webp", save_webp, typeof (WebpPreferences));
#endif
#if HAVE_LIBJXL
		register_image_saver ("image/jxl", save_jxl, typeof (JxlPreferences));
#endif
#if HAVE_LIBHEIF
		register_image_saver ("image/avif", save_avif, typeof (AvifPreferences));
#endif
#if HAVE_LIBTIFF
		register_image_saver ("image/tiff", save_tiff, typeof (TiffPreferences));
#endif

		tools.register (ToolCategory.IMAGES, "win.rotate-right", _("Rotate Right"), "gth-rotate-right-symbolic", "bracketright");
		tools.register (ToolCategory.IMAGES, "win.rotate-left", _("Rotate Left"), "gth-rotate-left-symbolic", "bracketleft");
		tools.register (ToolCategory.IMAGES, "win.convert-format", _("Convert Format"));
		tools.register (ToolCategory.IMAGES, "win.resize-images", _("Resize Images"));
		tools.register (ToolCategory.METADATA, "win.clear-metadata", _("Clear Metadata"));
		var tool = tools.register (ToolCategory.METADATA, "win.apply-orientation", _("Rotate Physically"));
		tool.visible = false;
		tool = tools.register (ToolCategory.METADATA, "win.reset-orientation", _("Reset EXIF Orientation"));
		tool.visible = false;
		tools.register (ToolCategory.METADATA, "win.update-thumbnail", _("Update Thumbnail"));

		shortcuts = new Shortcuts ();
		shortcuts.register_all ();

		register_types ();
		init_settings ();
		init_actions ();
	}

	public override void window_removed (Gtk.Window window) {
		base.window_removed (window);
		if (quitting) {
			foreach (unowned var win in get_windows ()) {
				if ((win != window) && (win is Gth.Window)) {
					unowned var other_window = win as Gth.Window;
					if (!other_window.closing) {
						other_window.close ();
						return;
					}
				}
			}
		}
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
			open_window (location, file_to_select);
		}
		else {
			// At least a location was specified.
			var one_per_window = remaining_args.length > 1;
			foreach (unowned var arg in remaining_args) {
				open_window (File.new_for_commandline_arg (arg), null, one_per_window);
			}
		}
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
		return get_sorter_by_id ((id == "modification-time") ? "Time::Modified" : "File::Name");
	}

	public void register_test (GLib.Type test_type) {
		var test = Object.new (test_type) as Gth.Test;
		tests.insert (test.id, test);
		ordered_tests.model.append (test);
	}

	public Gth.Test? get_test_by_id (string id) {
		return tests[app.migration.test.get_new_key (id)];
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

	public GenericList<Gth.Test> get_visible_filters () {
		var result = new GenericList<Gth.Test>();

		var no_filter = new Gth.Test ();
		no_filter.id = "";
		var general_filter = get_general_filter ();
		if (general_filter != null) {
			no_filter.display_name = general_filter.display_name;
		}
		else {
			no_filter.display_name = _("All Files");
		}
		result.model.append (no_filter);

		foreach (var filter in filters.entries) {
			if (filter.visible) {
				result.model.append (filter.duplicate ());
			}
		}
		return result;
	}

	public GenericList<Gth.Test> get_file_type_filters () {
		var result = new GenericList<Gth.Test>();
		foreach (unowned var filter in ordered_tests) {
			if (filter.id.has_prefix ("Type::")) {
				result.model.append (filter.duplicate ());
			}
		}
		return result;
	}

	public void register_source (GLib.Type source_type) {
		var source = Object.new (source_type) as Gth.FileSource;
		file_sources.add (source);
	}

	public inline FileSource get_source_for_file (File file) {
		return get_source_for_uri (file.get_uri ());
	}

	public GLib.Type get_source_type_for_uri (string uri) {
		for (var idx = file_sources.length - 1; idx >= 0; idx--) {
			unowned var source = file_sources[idx];
			if (source.supports_scheme (uri)) {
				return source.get_class ().get_type ();
			}
		}
		return typeof (FileSourceVfs);
	}

	public FileSource get_source_for_uri (string uri) {
		return Object.new (get_source_type_for_uri (uri)) as FileSource;
	}

	public void register_image_loader (string content_type, LoadFunc func) {
		loaders.set (content_type, func);
		register_file_viewer (content_type, typeof (ImageViewer));
	}

	public LoadFunc? get_load_func (string content_type) {
		return loaders.get (content_type);
	}

	public void register_external_loader (string content_type, LoadFileFunc func, GLib.Type viewer_type) {
		external_loaders.set (content_type, func);
		if (viewer_type != 0) {
			register_file_viewer (content_type, viewer_type);
		}
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
			var options = Object.new (options_type) as Gth.SaverPreferences;
			var extension_v = options.get_extensions ().split(",");
			foreach (unowned var extension in extension_v) {
				saver_extensions.set (extension, content_type);
			}
		}
	}

	public SaveFunc? get_save_func (string content_type) {
		return savers.get (content_type);
	}

	public bool can_save_extension (string extension) {
		return saver_extensions.contains (extension);
	}

	public unowned string get_content_type_from_name (File file) {
		var extension = Util.get_extension (file.get_basename ());
		return saver_extensions.get (extension);
	}

	public unowned string get_content_type_from_extension (string extension) {
		return saver_extensions.get (extension);
	}

	public GenericArray<SaverPreferences> get_ordered_savers () {
		var savers = new GenericArray<SaverPreferences>();
		foreach (unowned var type in saver_preferences.get_values ()) {
			var preferences = Object.new (type) as Gth.SaverPreferences;
			if (preferences != null) {
				savers.add (preferences);
			}
		}
		savers.sort ((a, b) => a.get_display_name ().collate (b.get_display_name ()));
		return savers;
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
		return Object.new (type) as Gth.FileViewer;
	}

	public bool viewer_can_view (GLib.Type viewer_type, string content_type) {
		return get_viewer_type_for_content_type (content_type) == viewer_type;
	}

	public Gth.Window? get_active_main_window () {
		foreach (var win in get_windows ()) {
			if (win is Gth.Window) {
				return win as Gth.Window;
			}
		}
		return null;
	}

	public void foreach_window (Gth.WindowFunc func) {
		foreach (var w in get_windows ()) {
			if (w is Gth.Window) {
				var win = w as Gth.Window;
				if ((win != null) && !win.closing) {
					func (win);
				}
			}
		}
	}

	public bool one_window () {
		var count = 0;
		foreach (var w in get_windows ()) {
			if (w is Gth.Window) {
				count++;
			}
		}
		return count <= 1;
	}

	const int MAX_ROOTS_PER_SOURCE = 1000;

	public async GenericList<FileData> update_roots () {
		roots.model.remove_all ();
		var job = jobs.new_job ("Updating Roots");
		var sort_order = 0;
		foreach (unowned var source in file_sources) {
			var source_roots = yield source.get_roots (job.cancellable);
			if (source_roots == null) {
				continue;
			}
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
		settings.changed.connect ((key) => {
			queue_setting_changed (key);
		});
		viewer_settings = new GLib.Settings (GTHUMB_VIEWER_SCHEMA);
		changed_keys = new GenericSet<string> (str_hash, str_equal);

		filters.load_from_file ();
		scripts.load_from_file ();
		tools.load_from_file ();
		selections.load_from_file ();
		shortcuts.load_from_file ();
		monitor.scripts_changed ();
	}

	uint setting_changed_id = 0;
	GenericSet<string> changed_keys;

	void queue_setting_changed (string key) {
		changed_keys.add (key);
		if (setting_changed_id == 0) {
			setting_changed_id = Util.after_timeout (100, () => {
				setting_changed_id = 0;
				foreach_window ((win) => {
					foreach (unowned var _key in changed_keys.get_values ()) {
						win.on_setting_change (_key);
					}
				});
				changed_keys.remove_all ();
			});
		}
	}

	void init_actions () {
		var action = new SimpleAction ("about", null);
		action.activate.connect (() => {
			const string[] developers = {
				"Paolo Bacchilega <paobac@src.gnome.org>",
			};
			Adw.show_about_dialog (active_window,
				"application-name", Config.APP_NAME,
				"application-icon", Config.APP_ID,
				"version", Config.APP_VERSION,
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

		action = new SimpleAction ("quit", null);
		action.activate.connect ((_action, param) => {
			quitting = true;
			var window = get_active_main_window ();
			if (window != null) {
				window.save_preferences ();
				window.close ();
			}
		});
		add_action (action);

		action = new SimpleAction ("shortcuts", null);
		action.activate.connect ((_action, param) => {
			shortcuts.show_help ();
		});
		add_action (action);
	}

	public void open_window (File location, File? file_to_select = null, bool force_new_window = false) {
		Gth.Window window = null;
		if (!force_new_window && settings.get_boolean (PREF_BROWSER_REUSE_ACTIVE_WINDOW)) {
			window = active_window as Gth.Window;
		}
		if (window == null) {
			window = new Gth.Window ();
		}
		if (file_to_select != null) {
			window.browser.open_location (location, LoadAction.OPEN, file_to_select);
		}
		else {
			window.open.begin (location);
		}
		if (!arg_slideshow) {
			window.present ();
		}
	}

	void register_types() {
		// Types used in .ui files
		typeof (Gth.ColorPreview).ensure ();
		typeof (Gth.FolderStatus).ensure ();
		typeof (Gth.ImageOverview).ensure ();
		typeof (Gth.ImageView).ensure ();
		typeof (Gth.SidebarResizer).ensure ();
		typeof (Gth.ToolButton).ensure ();
		typeof (Gth.VideoView).ensure ();
		typeof (Gth.FilterGrid).ensure ();
		typeof (Gth.HistogramView).ensure ();
		typeof (Gth.AspectRatioGroup).ensure ();
		typeof (Gth.GridGroup).ensure ();
		typeof (Gth.RenameTemplatePage).ensure ();
		typeof (Gth.ColorRange).ensure ();
		typeof (Gth.GeneralPreferences).ensure ();
		typeof (Gth.BrowserPreferences).ensure ();
		typeof (Gth.SaversPreferences).ensure ();
		typeof (Gth.ViewerPreferences).ensure ();
		typeof (Gth.ShortcutsPreferences).ensure ();
		typeof (Gth.FolderRow).ensure ();
	}

	const int MAX_IO_WORKERS = 4;
}

public delegate void Gth.WindowFunc (Gth.Window win);
