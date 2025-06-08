public class Gth.Application : Adw.Application {
	public GLib.Settings browser_settings;
	public HashTable<string, Gth.Test> tests;
	public GenericList<Gth.Test> ordered_tests;
	public HashTable<string, Gth.SortInfo?> sorters;
	public GenericArray<string> ordered_sorters;
	public GenericArray<FileSource> file_sources;
	public GenericArray<MetadataProvider> metadata_providers;
	public GenericArray<string> ordered_metadata_categories;
	public HashTable<string, Gth.MetadataCategory?> metadata_categories;
	public HashTable<string, Gth.MetadataInfo?> metadata_info;
	public GenericArray<Gth.MetadataInfo?> metadata_info_v;
	public HashTable<string, Gth.LoadFunc> loaders;
	public HashTable<string, Gth.SaveFunc> savers;
	public Gth.FilterFile filter_file;
	public Gth.JobQueue jobs;
	public bool restart;
	public bool quitting;
	public ImageLoader image_loader;
	public ThumbLoader thumb_loader;
	public ImageSaver image_saver;
	public ColorManager color_manager;

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
			foreach_window ((win) => win.status.set_n_jobs (jobs.size ()));
		});

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
		register_test (typeof (Gth.TestAspectRatio));
		register_test (typeof (Gth.TestVisible));

		sorters = new HashTable<string, Gth.SortInfo?>(str_hash, str_equal);
		ordered_sorters = new GenericArray<string>();
		register_sorter ({ "file::name", _("Name"), "standard::display-name", SortFunc.cmp_basename });
		register_sorter ({ "file::mtime", _("Modified"), "time::modified,time::modified-usec", SortFunc.cmp_modified_time });
		register_sorter ({ "file::size", _("Size"), "standard::size", SortFunc.cmp_size });
		register_sorter ({ "file::path", _("Location and Name"), "standard::display-name", SortFunc.cmp_uri });
		register_sorter ({ "general::dimensions", _("Width and Height"), "frame::width,frame::height", SortFunc.cmp_frame_dimensions });
		register_sorter ({ "frame::aspect-ratio", _("Aspect Ratio"), "frame::width,frame::height", SortFunc.cmp_aspect_ratio });
		register_sorter ({ "general::unsorted", _("Unsorted"), "", null });

		file_sources = new GenericArray<FileSource>();
		register_source (typeof (Gth.FileSourceVfs));
		register_source (typeof (Gth.FileSourceCatalogs));

		ordered_metadata_categories = new GenericArray<string>();
		metadata_categories = new HashTable<string, Gth.MetadataCategory?>(str_hash, str_equal);
		register_metadata_category ({ "file", N_("File"), 1 });
		register_metadata_category ({ "general", N_("Metadata"), 2 });

		metadata_info = new HashTable<string, Gth.MetadataInfo?>(str_hash, str_equal);
		metadata_info_v = new GenericArray<Gth.MetadataInfo?>();
		register_metadata_info ({ "standard::display-name", N_("Name"), "file", 1, null, METADATA_ALLOW_EVERYWHERE });
		register_metadata_info ({ "gth::file::display-size", N_("Size"), "file", 2, null, METADATA_ALLOW_EVERYWHERE });
		register_metadata_info ({ "standard::size", N_("Bytes"), "file", 3, null, METADATA_ALLOW_EVERYWHERE });
		// Translators: the file modified time.
		register_metadata_info ({ "gth::file::display-mtime", N_("Modified Date and Time"), "file", 4, null, METADATA_ALLOW_EVERYWHERE });
		register_metadata_info ({ "standard::fast-content-type", N_("Type"), "file", 5, null, METADATA_ALLOW_EVERYWHERE });
		register_metadata_info ({ "gth::file::is-modified", null, "file", 6, null, MetadataFlags.HIDDEN });
		register_metadata_info ({ "gth::file::full-name", N_("Location"), "file", 7, null, MetadataFlags.ALLOW_IN_PRINT | MetadataFlags.ALLOW_IN_FILE_LIST | MetadataFlags.ALLOW_IN_PROPERTIES_VIEW });
		register_metadata_info ({ "general::format", N_("Format"), "file", 10, null, MetadataFlags.ALLOW_IN_PROPERTIES_VIEW });
		register_metadata_info ({ "general::dimensions", N_("Dimensions"), "file", 12, null, METADATA_ALLOW_EVERYWHERE });
		register_metadata_info ({ "general::duration", N_("Duration"), "file", 11, null, METADATA_ALLOW_EVERYWHERE });

		register_metadata_info ({ "Embedded::Photo::Exposure", N_("Exposure Settings"), "general", 10, null, MetadataFlags.ALLOW_IN_PRINT | MetadataFlags.ALLOW_IN_FILE_LIST  });
		register_metadata_info ({ "Embedded::Photo::Aperture", N_("Aperture"), "general", 11, null, METADATA_ALLOW_EVERYWHERE  });
		register_metadata_info ({ "Embedded::Photo::ISOSpeed", N_("ISO Speed"), "general", 12, null, METADATA_ALLOW_EVERYWHERE  });
		register_metadata_info ({ "Embedded::Photo::ExposureTime", N_("Exposure Time"), "general", 13, null, METADATA_ALLOW_EVERYWHERE  });
		register_metadata_info ({ "Embedded::Photo::ShutterSpeed", N_("Shutter Speed"), "general", 14, null, METADATA_ALLOW_EVERYWHERE  });
		register_metadata_info ({ "Embedded::Photo::FocalLength", N_("Focal Length"), "general", 16, null, METADATA_ALLOW_EVERYWHERE  });
		register_metadata_info ({ "Embedded::Photo::Flash", N_("Flash"), "general", 17, null, METADATA_ALLOW_EVERYWHERE  });
		register_metadata_info ({ "Embedded::Photo::CameraModel", N_("Camera Model"), "general", 18, null, METADATA_ALLOW_EVERYWHERE  });
		register_metadata_info ({ "Loaded::Image::ColorProfile", N_("Color Profile"), "general", 19, null, MetadataFlags.ALLOW_IN_PROPERTIES_VIEW });

		register_metadata_info ({ "general::datetime", N_("General Date and Time"), "general", 20, null, METADATA_ALLOW_EVERYWHERE });
		register_metadata_info ({ "general::title", N_("Title"), "general", 21, null, METADATA_ALLOW_EVERYWHERE });
		register_metadata_info ({ "general::location", N_("Place"), "general", 22, null, METADATA_ALLOW_EVERYWHERE });
		register_metadata_info ({ "general::description", N_("Description"), "general", 23, null, MetadataFlags.ALLOW_IN_PRINT });
		register_metadata_info ({ "general::tags", N_("Tags"), "general", 24, null, METADATA_ALLOW_EVERYWHERE });
		register_metadata_info ({ "general::rating", N_("Rating"), "general", 25, null, METADATA_ALLOW_EVERYWHERE });

		register_metadata_info ({ "Embedded::Photo::Author", N_("Author"), "general", 30, null, METADATA_ALLOW_EVERYWHERE  });
		register_metadata_info ({ "Embedded::Photo::Copyright", N_("Copyright"), "general", 31, null, METADATA_ALLOW_EVERYWHERE  });

		register_metadata_info ({ "gth::file::emblems", "", "", 0, null, MetadataFlags.HIDDEN });
		register_metadata_info ({ "gth::standard::secondary-sort-order", "", "", 0, null, MetadataFlags.HIDDEN });
		register_metadata_info ({ "image::width", "", "", 0, null, MetadataFlags.HIDDEN });
		register_metadata_info ({ "image::height", "", "", 0, null, MetadataFlags.HIDDEN });
		register_metadata_info ({ "frame::width", "", "", 0, null, MetadataFlags.HIDDEN });
		register_metadata_info ({ "frame::height", "", "", 0, null, MetadataFlags.HIDDEN });
		register_metadata_info ({ "Embedded::Image::Orientation", "", "", 0, null, MetadataFlags.HIDDEN });
		register_metadata_info ({ "Embedded::Photo::DateTimeOriginal", "", "", 0, null, MetadataFlags.HIDDEN });

		metadata_providers = new GenericArray<MetadataProvider>();
		register_metadata_provider (typeof (MetadataProviderFile));

		loaders = new HashTable<string, Gth.LoadFunc>(str_hash, str_equal);
		register_image_loader ("image/png", load_png);
		register_image_loader ("image/jpeg", load_jpeg);
#if HAVE_LIBWEBP
		register_image_loader ("image/webp", load_webp);
#endif
#if HAVE_LIBRSVG
		register_image_loader ("image/svg+xml", load_svg);
#endif

		savers = new HashTable<string, Gth.SaveFunc>(str_hash, str_equal);
		register_image_saver ("image/png", save_png);

		io_factory = new Work.Factory (get_workers (MAX_IO_WORKERS));
		image_loader = new ImageLoader (io_factory);
		thumb_loader = new ThumbLoader (io_factory);
		image_saver = new ImageSaver (io_factory);
		color_manager = new ColorManager ();
	}

	public override void startup () {
		base.startup ();
		init_settings ();
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
			var location = Gth.Settings.get_startup_location (browser_settings);
			var file_to_select = Gth.Settings.get_file (browser_settings, PREF_BROWSER_STARTUP_CURRENT_FILE);
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
		var filter_id = browser_settings.get_string (PREF_BROWSER_GENERAL_FILTER);
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
			// Reset the last filter only in the first window.
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
					if (id != null) {
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

	public void register_image_saver (string content_type, SaveFunc func) {
		savers.set (content_type, func);
	}

	public SaveFunc? get_save_func (string content_type) {
		return savers.get (content_type);
	}

	public void register_metadata_provider (GLib.Type provider_type) {
		var provider = Object.new (provider_type) as Gth.MetadataProvider;
		metadata_providers.add (provider);
	}

	public void register_metadata_info (Gth.MetadataInfo info) {
		metadata_info[info.id] = info;
		metadata_info_v.add (info);
	}

	public void register_metadata_category (Gth.MetadataCategory category) {
		ordered_metadata_categories.add (category.id);
		metadata_categories[category.id] = category;
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

	public async GenericArray<FileData> get_roots () {
		var job = new_job ("Get roots");
		var roots = new GenericArray<FileData>();
		var sort_order = 0;
		foreach (unowned var source in file_sources) {
			var source_roots = yield source.get_roots (job.cancellable);
			if (source_roots == null)
				continue;
			foreach (unowned var root in source_roots) {
				root.info.set_sort_order (sort_order++);
				roots.add (root);
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

		browser_settings = new GLib.Settings (GTHUMB_BROWSER_SCHEMA);

		filter_file = new Gth.FilterFile ();
	}

	void open_window (File location, File? file_to_select = null, bool force_new_window = false) {
		var reuse_active_window = false;
		if (!force_new_window) {
			reuse_active_window = browser_settings.get_boolean (PREF_BROWSER_REUSE_ACTIVE_WINDOW);
		}

		Gth.Window window = null;
		if (reuse_active_window) {
			window = active_window as Gth.Window;
		}

		if (window == null) {
			window = new Gth.Window (this, location, file_to_select);
		}
		else {
			window.open_location (location, LoadAction.OPEN, file_to_select);
		}

		if (!arg_slideshow) {
			window.present ();
		}
	}

	uint get_workers (int max_workers) {
		// 1 processors -> 1 thread
		// 2 processors -> 1 thread
		// 3 processors -> 2 thread
		// 4 processors -> 3 threads
		// 5 processors -> 3 threads
		// 6 processors -> 3 threads
		var n_proc = (int) GLib.get_num_processors () - 1;
		return n_proc.clamp (1, max_workers);
	}

	const int MAX_IO_WORKERS = 3;
	Work.Factory io_factory;
}

public delegate void Gth.WindowFunc (Gth.Window win);
