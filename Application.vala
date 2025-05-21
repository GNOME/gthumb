public class Gth.Application : Adw.Application {
	public HashTable<string, Gth.Test> tests;
	public ListStore ordered_tests;
	public HashTable<string, Gth.SortInfo?> sorters;
	public Gth.FilterFile filter_file;
	public bool restart;
	public bool quitting;

	public Application () {
		Object (
			application_id: "app.gthumb.gThumb",
			register_session: true,
			flags: ApplicationFlags.HANDLES_COMMAND_LINE
		);
		restart = false;
		quitting = false;

		tests = new HashTable<string, Gth.Test>(str_hash, str_equal);
		ordered_tests = new ListStore (typeof (Gth.Test));
		register_test (typeof (Gth.TestFileName));
		register_test (typeof (Gth.TestFileSize));
		register_test (typeof (Gth.TestFileModifiedTime));
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

		sorters = new HashTable<string, Gth.SortInfo?>(str_hash, str_equal);
		register_sorter ({ "file::name", _("Name"), "standard::display-name", SortFunc.cmp_basename });
		register_sorter ({ "file::path", _("Path"), "standard::display-name", SortFunc.cmp_uri });
		register_sorter ({ "file::size", _("Bytes"), "standard::size", SortFunc.cmp_size });
		register_sorter ({ "file::mtime", _("Modified"), "time::modified,time::modified-usec", SortFunc.cmp_modified_time });
		register_sorter ({ "general::unsorted", _("No Sorting"), "", null });
		register_sorter ({ "general::dimensions", _("Pixels"), "frame::width,frame::height", SortFunc.cmp_frame_dimensions });
		register_sorter ({ "frame::aspect-ratio", _("Aspect Ratio"), "frame::width,frame::height", SortFunc.cmp_aspect_ratio });
	}

	public void register_sorter (Gth.SortInfo sorter) {
		sorters.insert (sorter.id, sorter);
	}

	public Gth.SortInfo? get_sorter_by_id (string id) {
		return sorters[id];
	}

	public void register_test (GLib.Type test_type) {
		var test = Object.new (test_type) as Gth.Test;
		tests.insert (test.id, test);
		ordered_tests.append (test);
	}

	public Gth.Test? get_test_by_id (string id) {
		return tests[id];
	}

	public ListStore get_visible_filters () {
		var filters = new ListStore (typeof (Gth.Test));

		var no_filter = new Gth.Test ();
		no_filter.id = "";
		no_filter.display_name = _("All Files");
		filters.append (no_filter);

		var iter = new ListModelIterator (filter_file.filters);
		while (iter.next ()) {
			unowned var filter = iter.get () as Gth.Test;
			if (filter.visible) {
				filters.append (filter.duplicate ());
			}
		}
		return filters;
	}

	public ListStore get_file_type_filters () {
		var filters = new ListStore (typeof (Gth.Test));

		//var no_filter = new Gth.Test ();
		//no_filter.id = "";
		//no_filter.display_name = _("All Files");
		//filters.append (no_filter);

		var iter = new ListModelIterator (ordered_tests);
		while (iter.next ()) {
			unowned var filter = iter.get () as Gth.Test;
			if (filter.id.has_prefix ("file::type::")) {
				filters.append (filter.duplicate ());
			}
		}
		return filters;
	}

	public override void startup () {
		base.startup ();
		init_settings ();
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
		var context = new OptionContext (_("— Image browser and viewer"));
		context.set_help_enabled (true);
		context.set_translation_domain (GETTEXT_PACKAGE);
		context.set_ignore_unknown_options (true);
		context.add_main_entries (option_entries, null);

		//static size_t initialized = false;
		//if (Once.init_enter (ref initialized)) {
		//	context.add_group ();
		//	Once.init_leave (ref initialized, true)
		//}

		return context;
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
			var settings = new GLib.Settings (GTHUMB_BROWSER_SCHEMA);
			var location = Gth.Settings.get_startup_location (settings);
			var file_to_select = Gth.Settings.get_file (settings, PREF_BROWSER_STARTUP_CURRENT_FILE);
			open_window (location, file_to_select, true);
			return 0;
		}

		// At least a location was specified.
		// TODO
		return 0;
	}

	void init_settings () {
		var css_provider = new Gtk.CssProvider ();
		Gtk.StyleContext.add_provider_for_display (Gdk.Display.get_default (), css_provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION);
		css_provider.load_from_resource ("/app/gthumb/gthumb/css/style.css");

		var icon_theme = Gtk.IconTheme.get_for_display (Gdk.Display.get_default ());
		icon_theme.add_resource_path ("/app/gthumb/gthumb/icons");

		filter_file = new Gth.FilterFile ();
	}

	void open_window (File location, File? file_to_select = null, bool force_new_window = false) {
		var reuse_active_window = false;
		if (!force_new_window) {
			var settings = new GLib.Settings (GTHUMB_BROWSER_SCHEMA);
			reuse_active_window = settings.get_boolean (PREF_BROWSER_REUSE_ACTIVE_WINDOW);
		}

		Gth.Window window = null;
		if (reuse_active_window) {
			window = active_window as Gth.Window;
		}

		if (window == null) {
			window = new Gth.Window (this, location, file_to_select);
		}
		else if (file_to_select != null) {
			window.go_to (location, file_to_select);
		}
		else {
			window.load_location (location);
		}

		if (!arg_slideshow) {
			window.present ();
		}
	}
}
