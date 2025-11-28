namespace Gth.Util {
	public static uint next_tick (owned Gth.IdleFunc function, int priority = Priority.DEFAULT) {
		return Timeout.add_full (priority, 0, () => {
			function ();
			return Source.REMOVE;
		});
	}

	public static uint after_next_rearrange (owned Gth.IdleFunc function) {
		return Timeout.add_full (Priority.LOW, 0, () => {
			function ();
			return Source.REMOVE;
		});
	}

	public static uint after_timeout (uint interval, owned Gth.IdleFunc function) {
		return Timeout.add (interval, () => {
			function ();
			return Source.REMOVE;
		});
	}

	public static uint after_seconds (uint seconds, owned Gth.IdleFunc function) {
		return Timeout.add_seconds (seconds, () => {
			function ();
			return Source.REMOVE;
		});
	}

	public static int enum_index (string[] values, string? value, int _default = 0) {
		if (value != null) {
			for (var i = 0; i < values.length; i++) {
				if (value == values[i]) {
					return i;
				}
			}
		}
		return _default;
	}

	public static unowned string? get_uri_scheme (string uri_string) {
		try {
			var uri = GLib.Uri.parse (uri_string, UriFlags.PARSE_RELAXED);
			unowned var scheme = uri.get_scheme ();
			if (scheme == null)
				return null;
			return Strings.get_static (scheme);
		}
		catch (Error e) {
			return null;
		}
	}

	public static bool content_type_is_image (string content_type) {
		return ContentType.is_a (content_type, "image/*");
	}

	public static bool content_type_is_raw (string content_type) {
		return ContentType.is_a (content_type, "image/x-dcraw");
	}

	public static bool content_type_is_video (string content_type) {
		if (ContentType.is_a (content_type, "video/*")) {
			return true;
		}
		foreach (unowned var video_type in Other_Video_Types) {
			if (content_type == video_type) {
				return true;
			}
		}
		return false;
	}

	public static bool content_type_is_audio (string content_type) {
		return ContentType.is_a (content_type, "audio/*");
	}

	public static bool content_type_has_transparency (string content_type) {
		return (content_type == "image/png")
			|| (content_type == "image/gif")
			|| (content_type == "image/svg+xml")
			|| (content_type == "image/webp")
			|| (content_type == "image/jxl")
			|| (content_type == "image/avif")
			|| (content_type == "image/heif")
			|| (content_type == "image/heic")
			|| (content_type == "image/tiff")
			|| (content_type == "image/x-tga");
	}

	public static string[] extract_metadata_attributes (string attributes) {
		string[] result = {};
		var matcher = new FileAttributeMatcher (attributes);
		foreach (unowned var info in MetadataInfo.get_all ()) {
			if (matcher.matches (info.id)) {
				result += info.id;
			}
		}
		return result;
	}

	public static string extract_file_attributes (string attributes) {
		return (attributes == "*") ? STANDARD_ATTRIBUTES_WITH_FAST_CONTENT_TYPE : attributes;
	}

	const int BUFFER_SIZE_FOR_SNIFFING = 32;

	public static unowned string? guess_content_type_from_stream (InputStream stream, File? file, Cancellable cancellable) {
		var buffer = new uint8[BUFFER_SIZE_FOR_SNIFFING];
		size_t content_size;
		try {
			stream.read_all (buffer, out content_size, cancellable);
		}
		catch (Error error) {
			return null;
		}
		unowned var content_type = guess_mime_type (buffer);
		if (content_type == null) {
			bool result_uncertain;
			unowned var valid_data = buffer[0:content_size];
			content_type = Strings.get_static (ContentType.guess (null, valid_data, out result_uncertain));
			if (result_uncertain) {
				content_type = null;
			}
			if ((content_type == null)
				|| (content_type == "application/xml")
				|| (content_type == "image/tiff"))
			{
				if (file != null) {
					content_type = Util.guess_content_type_from_name (file.get_basename ());
				}
			}
		}
		return content_type;
	}

	public static unowned string guess_content_type_from_name (string filename) {
		// Types unknown to GLib
		if (filename.has_suffix (".webp"))
			return "image/webp";
		if (filename.has_suffix (".jxl"))
			return "image/jxl";
		return Strings.get_static (ContentType.guess (filename, null, null));
	}

	public static unowned string? get_content_type (File? file, FileInfo info) {
		unowned var result = info.get_attribute_string (FileAttribute.STANDARD_CONTENT_TYPE);
		if (result == null) {
			result = info.get_attribute_string (FileAttribute.STANDARD_FAST_CONTENT_TYPE);
		}
		if ((result == null) && (file != null)) {
			unowned var guessed_type = Util.guess_content_type_from_name (file.get_basename ());
			info.set_attribute_string (FileAttribute.STANDARD_FAST_CONTENT_TYPE, guessed_type);
			result = guessed_type;
		}
		return result;
	}

	public static int int_cmp (int x, int y) {
		return (x < y) ? -1 : (x > y) ? 1 : 0;
	}

	public static int uint_cmp (uint x, uint y) {
		return (x < y) ? -1 : (x > y) ? 1 : 0;
	}

	public static bool toggle_state (SimpleAction action) {
		var new_state = !action.get_state ().get_boolean ();
		action.set_state (new Variant.boolean (new_state));
		return new_state;
	}

	public static GLib.Settings? get_settings_if_schema_installed (string schema_id) {
		var source = SettingsSchemaSource.get_default ();
		if (source == null)
			return null;

		var schema = source.lookup (schema_id, true);
		if (schema == null)
			return null;

		return new GLib.Settings (schema_id);
	}

	public static MenuItem menu_item_for_file (File file, string? custom_name = null) {
		var file_source = app.get_source_for_file (file);
		var info = file_source.get_display_info (file);
		var item = new MenuItem (null, null);
		item.set_label (!Strings.empty (custom_name) ? custom_name : info.get_display_name ());
		item.set_icon (info.get_symbolic_icon ());
		return item;
	}

	public static int intcmp (int a, int b) {
		return (a == b) ? 0 : (a < b) ? -1 : 1;
	}

	public static string format_content_type (string? content_type) {
		if (content_type == null)
			return _("Unknown");
		var components = content_type.split ("/", 2);
		if ((components[0] != null) && (components[1] != null)) {
			if ((components[0] == "image")
				|| (components[0] == "video")
				|| (components[0] == "audio"))
			{
				if (components[1] == "svg+xml") {
					return "SVG";
				}
				else {
					return components[1].ascii_up ();
				}
			}
		}
		return content_type;
	}

	public static void get_widget_abs_position (Gtk.Widget widget, out int x, out int y, Gtk.Window? target = null) {
		if (target == null) {
			target = widget.root as Gtk.Window;
			if (target == null) {
				x = 0;
				y = 0;
				return;
			}
		}
		Graphene.Point p = Graphene.Point.zero ();
		widget.compute_point (target.child, Graphene.Point.zero (), out p);
		x = (int) Math.floorf (p.x);
		y = (int) Math.floorf (p.y);
	}

	static HashTable<string, Icon> icons = null;

	public static Icon get_themed_icon (string icon_name) {
		if (icons == null) {
			icons = new HashTable<string, Icon> (str_hash, str_equal);
		}
		var icon = icons.get (icon_name);
		if (icon == null) {
			icon = new ThemedIcon (icon_name);
			icons.set (icon_name, icon);
		}
		return icon;
	}

	public static string clear_for_filename (string name) {
		return name.replace ("/", "_");
	}

	public static void enable_action (SimpleActionGroup action_group, string name, bool enabled) {
		var action = action_group.lookup_action (name) as SimpleAction;
		if (action != null) {
			action.set_enabled (enabled);
		}
	}

	public static void set_state (SimpleActionGroup action_group, string name, Variant state) {
		var action = action_group.lookup_action (name) as SimpleAction;
		if (action != null) {
			action.set_state (state);
		}
	}

	public static void set_active (SimpleActionGroup action_group, string name, bool active) {
		var action = action_group.lookup_action (name) as SimpleAction;
		if (action != null) {
			action.set_state (new Variant.boolean (active));
		}
	}

	public static bool get_active (SimpleActionGroup action_group, string name) {
		var state = action_group.get_action_state (name);
		return (state != null) && state.get_boolean ();
	}

	public static void remove_all_children (Gtk.Box box) {
		unowned Gtk.Widget child;
		while ((child = box.get_first_child ()) != null) {
			box.remove (child);
		}
	}

	public static T get_parent<T> (Gtk.Widget widget) {
		unowned var p = widget.parent;
		while ((p != null) && !(p is T)) {
			p = p.parent;
		}
		return (p != null) ? (T) p : null;
	}

	public static void print_rectangle (string prefix, Graphene.Rect rect) {
		stdout.printf ("%s (%f,%f)[%f,%f]\n",
			prefix,
			rect.origin.x,
			rect.origin.y,
			rect.size.width,
			rect.size.height
		);
	}

	public static float get_zoom_to_fit_surface (uint natural_width, uint natural_height, int max_width, int max_height) {
		if (natural_width == 0)
			return 1f;
		if (natural_height == 0)
			return 1f;
		var x_ratio = (float) max_width / natural_width;
		var y_ratio = (float) max_height / natural_height;
		return (x_ratio < y_ratio) ? x_ratio : y_ratio;
	}

	public static float get_zoom_to_fill_surface (uint natural_width, uint natural_height, int max_width, int max_height) {
		if (natural_width == 0)
			return 1f;
		if (natural_height == 0)
			return 1f;
		var x_ratio = (float) max_width / natural_width;
		var y_ratio = (float) max_height / natural_height;
		return (x_ratio < y_ratio) ? y_ratio : x_ratio;
	}

	public static float get_zoom_to_fit_or_fill_surface (uint natural_width, uint natural_height, int max_width, int max_height) {
		if ((natural_width == 0) || (natural_height == 0)) {
			return 1f;
		}
		var x_ratio = (float) max_width / natural_width;
		var y_ratio = (float) max_height / natural_height;
		var ratio = (y_ratio > x_ratio) ? y_ratio : x_ratio;
		var resized_surface = (ratio * natural_width) * (ratio * natural_height);
		var available_space = max_width * max_height;
		var covered_space = resized_surface / available_space;
		//stdout.printf ("> covered_space: %f\n", covered_space);
		if (covered_space > 1.05f) {
			var adj = float.min (1.05f / covered_space, 1.05f);
			//stdout.printf ("  adj [1]: %f\n", adj);
			ratio *= adj;
		}
		else if (covered_space < 0.95f) {
			var adj = float.min (0.95f / covered_space, 1.05f);
			//stdout.printf ("  adj [2]: %f\n", adj);
			ratio *= adj;
		}
		return float.min (ratio, 2.25f);
	}

	public static inline float get_zoom_to_fit_length (uint natural, int max) {
		return (natural > 0) ? (float) max / natural : 1f;
	}

	public static inline float center_content (float container_size, float content_size) {
		return float.max ((container_size - content_size) / 2, 0);
	}

	public static uint file_hash (File a) {
		return a.hash ();
	}

	public static bool file_equal (File a, File b) {
		return a.equal (b);
	}

	public static string format_double (double value, string format = "%0.2f") {
		var buffer = new char[50];
		value.format (buffer, format);
		return (string) buffer;
	}

	public static Cairo.Pattern build_transparency_pattern (int half_size) {
		var size = half_size * 2;
		var surface = new Cairo.ImageSurface (Cairo.Format.RGB24, size, size);
		var ctx = new Cairo.Context (surface);

		ctx.set_source_rgba (0.66, 0.66, 0.66, 1.0);
		ctx.rectangle (0, 0, half_size, half_size);
		ctx.fill ();
		ctx.rectangle (half_size, half_size, half_size, half_size);
		ctx.fill ();

		ctx.set_source_rgba (0.33, 0.33, 0.33, 1.0);
		ctx.rectangle (half_size, 0, half_size, half_size);
		ctx.fill ();
		ctx.rectangle (0, half_size, half_size, half_size);
		ctx.fill ();

		var transparency_pattern = new Cairo.Pattern.for_surface (surface);
		transparency_pattern.set_extend (Cairo.Extend.REPEAT);
		return transparency_pattern;
	}

	public static Adw.Toast new_literal_toast (string title) {
		var toast = new Adw.Toast (title);
		toast.use_markup = false;
		return toast;
	}

	public static File get_xmp_sidecar (File file) {
		var uri = file.get_uri ();
		var sidecar_uri = Util.remove_extension (uri) + ".xmp";
		return File.new_for_uri (sidecar_uri);
	}

	public static unowned Adw.PreferencesDialog? get_preferences_dialog (Gtk.Widget widget) {
		unowned var parent = widget.parent;
		while (parent != null) {
			if (parent is Adw.PreferencesDialog) {
				return parent as Adw.PreferencesDialog;
			}
			parent = parent.parent;
		}
		return null;
	}

	public static uint get_workers (int max_workers = -1) {
		var n_workers = GLib.get_num_processors () - 1;
		if (max_workers > 0) {
			return n_workers.clamp (1, max_workers);
		}
		else {
			return uint.min (n_workers, 1);
		}
	}

	public static void select_filename_without_ext (Gtk.Entry entry) {
		var ext_start = Util.get_extension_start (entry.text);
		if (ext_start > 1) {
			entry.select_region (0, entry.text.char_count (ext_start));
		}
	}
}
