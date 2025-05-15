static bool arg_version = false;
static bool arg_new_window = false;
static bool arg_fullscreen = false;
static bool arg_slideshow = false;
static bool arg_import_photos = false;
[CCode (array_length = false, array_null_terminated = true)]
static string[]? remaining_args = null;

const GLib.OptionEntry[] Options = {
	{
		"version",
		'v',
		OptionFlags.NONE,
		OptionArg.NONE,
		ref arg_version,
		"Show the application version",
		null
	},
	{
		"new-window",
		'n',
		OptionFlags.NONE,
		OptionArg.NONE,
		ref arg_new_window,
		"Open a new window",
		null
	},
	{
		"fullscreen",
		'f',
		OptionFlags.NONE,
		OptionArg.NONE,
		ref arg_fullscreen,
		"Open a new window",
		null
	},
	{
		"slideshow",
		's',
		OptionFlags.NONE,
		OptionArg.NONE,
		ref arg_slideshow,
		"Automatically start a presentation",
		null
	},
	{
		"import-photos",
		'i',
		OptionFlags.NONE,
		OptionArg.NONE,
		ref arg_import_photos,
		"Automatically import digital camera photos",
		null
	},
	{
		GLib.OPTION_REMAINING,
		0,
		OptionFlags.NONE,
		OptionArg.STRING_ARRAY,
		ref remaining_args,
		null,
		"[FILE…] [DIRECTORY…]"
	},
	{ null }
};

public class Gth.Application : Gtk.Application {
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
	}

	public override void startup () {
		base.startup ();
		var gtk_settings = Gtk.Settings.get_default ();
		gtk_settings.gtk_application_prefer_dark_theme = true;
	}

	OptionContext get_command_line_option_context () {
		var context = new OptionContext ("— Image browser and viewer");
		context.set_help_enabled (true);
		//context.set_translation_domain (GETTEXT_PACKAGE);
		context.set_ignore_unknown_options (true);
		context.add_main_entries (Options, null);

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
