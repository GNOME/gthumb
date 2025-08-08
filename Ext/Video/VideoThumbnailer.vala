const int DEFAULT_THUMBNAIL_SIZE = 256;

static int thumbnail_size = 0;
static string? input_path = null;
static string? output_path = null;
static bool version = false;

const GLib.OptionEntry[] options = {
	{
		"version",
		'v',
		OptionFlags.NONE,
		OptionArg.NONE,
		ref version,
		"Print the version number",
		null
	},
	{
		"size",
		's',
		OptionFlags.NONE,
		OptionArg.INT,
		ref thumbnail_size,
		"Thumbnail size (default 256)",
		"N"
	},
	{ null }
};

public static int main (string[] args) {
	Gst.init (ref args);

	var opt_context = new OptionContext ("VIDEO_FILE THUMBNAIL_FILE - generate a thumbnail for a video using gstreamer");
	opt_context.set_help_enabled (true);
	opt_context.add_main_entries (options, null);
	try {
		opt_context.parse (ref args);

		if (version) {
			stdout.printf ("%s\n", Config.VERSION);
			return 0;
		}

		input_path = args[1];
		if (input_path == null) {
			throw new OptionError.FAILED ("Video file not specified.");
		}

		output_path = args[2];
		if (output_path == null) {
			throw new OptionError.FAILED ("Thumbnail file not specified.");
		}

		if (thumbnail_size <= 0) {
			thumbnail_size = DEFAULT_THUMBNAIL_SIZE;
		}
	}
	catch (OptionError e) {
		stderr.printf ("Error: %s\n\n", e.message);
		stderr.printf ("%s", opt_context.get_help (true, null));
		return 1;
	}

	var result = 0;

	try {
		// Generate the thumbnail.
		var input_file = File.new_for_commandline_arg (input_path);
		if (input_file == null) {
			throw new IOError.FAILED ("Invalid video filename");
		}

		var cancellable = new Cancellable ();
		var image = generate_video_thumbnail (input_file, cancellable);

		// Scale to the requested size.
		var scaled = image.resize (thumbnail_size, Gth.ResizeFlags.UPSCALE, Gth.ScaleFilter.BEST, cancellable);

		// Save to output_path.
		var bytes = save_png (scaled, null, cancellable);
		var output_file = File.new_for_commandline_arg (output_path);
		if (output_file == null) {
			throw new IOError.FAILED ("Invalid thumbnail filename");
		}
		Gth.Files.save_file (output_file, bytes);
	}
	catch (Error e) {
		stderr.printf ("VideoThumbnailer: Error: %s (input_path: %s)\n\n", e.message, input_path);
		result = 1;
	}

	return result;
}
