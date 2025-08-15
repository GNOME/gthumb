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
	public Migration migration;

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
			foreach_window ((win) => {
				win.browser.status.set_n_jobs (jobs.size ());
				win.viewer.status.set_n_jobs (jobs.size ());
			});
		});
		io_factory = new Work.Factory (get_workers (MAX_IO_WORKERS));
		image_loader = new ImageLoader (io_factory);
		thumb_loader = new ThumbLoader (io_factory);
		image_saver = new ImageSaver (io_factory);
		color_manager = new ColorManager ();
		roots = new GenericList<FileData>();
		monitor = new Monitor ();
		bookmarks = new Bookmarks ();
		migration = new Migration ();

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
		register_sorter ({ "File::Name", _("Name"), "standard::display-name", Sorters.cmp_basename });
		register_sorter ({ "Time::Modified", _("Modified"), "time::modified,time::modified-usec", Sorters.cmp_modified_time });
		register_sorter ({ "File::Size", _("Bytes"), "standard::size", Sorters.cmp_size });
		register_sorter ({ "File::Path", _("File Path"), "standard::display-name", Sorters.cmp_uri });
		register_sorter ({ "Frame::Pixels", _("Width and Height"), "Frame::Width,Frame::Height", Sorters.cmp_frame_dimensions });
		register_sorter ({ "Frame::AspectRatio", _("Aspect Ratio"), "Frame::Width,Frame::Height", Sorters.cmp_aspect_ratio });
		register_sorter ({ "Private::Unsorted", _("Unsorted"), "", null });

		file_sources = new GenericArray<FileSource>();
		register_source (typeof (Gth.FileSourceVfs));
		register_source (typeof (Gth.FileSourceCatalogs));

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
		// Translators: the file modified time.
		MetadataInfo.register ("Private::File::DisplayMtime", N_("Modified"), "File", METADATA_ALLOW_EVERYWHERE);
		MetadataInfo.register ("Private::File::Location", N_("Folder"), "File", MetadataFlags.ALLOW_IN_PRINT | MetadataFlags.ALLOW_IN_FILE_LIST | MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register (PrivateAttribute.LOADED_IMAGE_IS_MODIFIED, null, "File", MetadataFlags.HIDDEN);

		// Translators: width and height of images or videos.
		MetadataInfo.register ("Frame::Pixels", N_("Pixels"), "File", METADATA_ALLOW_EVERYWHERE);
		MetadataInfo.register ("Metadata::Duration", N_("Duration"), "File", METADATA_ALLOW_EVERYWHERE);
		MetadataInfo.register (PrivateAttribute.LOADED_IMAGE_COLOR_PROFILE, N_("Color Profile"), "File", MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);

		MetadataInfo.register ("Metadata::Title", N_("Title"), "Comment", METADATA_ALLOW_EVERYWHERE);
		MetadataInfo.register ("Metadata::Description", N_("Comment"), "Comment", MetadataFlags.ALLOW_IN_PRINT | MetadataFlags.ALLOW_IN_PROPERTIES_VIEW);
		MetadataInfo.register ("Metadata::Location", N_("Place"), "Comment", METADATA_ALLOW_EVERYWHERE);
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
		register_metadata_provider (typeof (MetadataProviderFile));
		register_metadata_provider (typeof (ExivMetadataProvider));
		register_metadata_provider (typeof (MetadataProviderImage));
		register_metadata_provider (typeof (MetadataProviderComment));
		register_metadata_provider (typeof (VideoMetadataProvider));

		loaders = new HashTable<string, Gth.LoadFunc>(str_hash, str_equal);
		viewers = new HashTable<string, GLib.Type>(str_hash, str_equal);

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
#if HAVE_LIBTIFF
		register_image_loader ("image/tiff", load_tiff);
#endif

		external_loaders = new HashTable<string, Gth.LoadFileFunc>(str_hash, str_equal);
#if HAVE_GSTREAMER
		register_external_loader ("video/*", load_video_thumbnail, typeof (VideoViewer));
		register_external_loader ("audio/*", load_video_thumbnail, typeof (VideoViewer));
		foreach (unowned var video_type in Other_Video_Types) {
			register_external_loader (video_type, load_video_thumbnail, typeof (VideoViewer));
		}
#endif

		savers = new HashTable<string, Gth.SaveFunc>(str_hash, str_equal);
		saver_preferences = new HashTable<string, GLib.Type>(str_hash, str_equal);
		saver_extensions = new HashTable<string, string>(str_hash, str_equal);
		register_image_saver ("image/png", save_png, typeof (PngPreferences));
		register_image_saver ("image/jpeg", save_jpeg, typeof (JpegPreferences));
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
			if (filter.id.has_prefix ("Type::")) {
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

	public inline Gth.Job new_job (string description, JobFlags flags = JobFlags.DEFAULT) {
		return jobs.new_job (description, flags);
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
		filter_file = new Gth.FilterFile ();

		changed_keys = new GenericSet<string> (str_hash, str_equal);
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
				"application-name", "Thumbnails",
				"application-icon", "app.gthumb.gthumb",
				"version", APP_VERSION,
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
			window = new Gth.Window (this);
		}
		window.browser.open_location (location, LoadAction.OPEN, file_to_select);

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
