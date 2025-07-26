public class Gth.Util {
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

	// 'video/mp4' -> 'video/*'
	public static string get_generic_type (string content_type) {
		var end = content_type.index_of_char ('/');
		if (end < 0)
			return content_type;
		return content_type.substring (0, end + 1) + "*";
	}

	public static Gth.DateTime? get_time_from_exif_date (string? exif_date) {
		if (exif_date == null)
			return null;

		unowned char[] chars = (char[]) exif_date.data;
		int idx = 0;

		while (chars[idx].isspace ())
			idx++;

		if (chars[idx] == 0)
			return null;

		int parse_int (int expected_digits) {
			var digits = 0;
			var result = 0;
			while ((chars[idx] != 0) && (digits < expected_digits)) {
				if (!chars[idx].isdigit ()) {
					break;
				}
				digits++;
				result = (result * 10) + (chars[idx] - '0');
				idx++;
			}
			return (digits == expected_digits) ? result : -1;
		}

		var year = parse_int (4);
		if ((year < 1) || (year >= 10000))
			return null;

		if (chars[idx++] != ':')
			return null;

		var month = parse_int (2);
		if ((month < 1) || (month > 12))
			return null;

		if (chars[idx++] != ':')
			return null;

		var day = parse_int (2);
		if ((day < 1) || (day > 31))
			return null;

		if (chars[idx++] != ' ')
			return null;

		var hours = parse_int (2);
		if ((hours < 0) || (hours > 23))
			return null;

		if (chars[idx++] != ':')
			return null;

		var minutes = parse_int (2);
		if ((minutes < 0) || (minutes > 59))
			return null;

		if (chars[idx++] != ':')
			return null;

		var seconds = parse_int (2);
		if ((seconds < 0) || (seconds > 59))
			return null;

		double useconds = 0.0;
		if ((chars[idx] == ',') || (chars[idx] == '.')) {
			idx++;
			var weight = 0.1;
			while ((weight > 0.0) && (chars[idx] != 0)) {
				if (!chars[idx].isdigit ()) {
					break;
				}
				useconds += weight * (chars[idx] - '0');
				weight /= 10.0;
				idx++;
			}
		}

		while (chars[idx].isspace ())
			idx++;

		if (chars[idx] != 0)
			return null;

		return new Gth.DateTime.from_ymd_hms (year, month, day, hours, minutes, seconds, (int) (useconds * 1000000));
	}

	public static int get_digits (int number) {
		int digits = 1;
		if (number < 0) {
			digits++;
			number = -number;
		}
		while (number > 10) {
			number = digits / 10;
			digits++;
		}
		return digits;
	}

	public static int enum_index (string[] values, string? value, int default = 0) {
		if (value != null) {
			for (var i = 0; i < values.length; i++) {
				if (value == values[i]) {
					return i;
				}
			}
		}
		return default;
	}

	public static string uri_from_path (string path) {
		return "file://" + GLib.Uri.unescape_string (path, GLib.Uri.RESERVED_CHARS_ALLOWED_IN_PATH);
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

	public static string concat_attributes (string? attributes, string? other_attributes) {
		if ((attributes == "*") || (other_attributes == "*"))
			return "*";
		var result = new StringBuilder ("");
		if (!Strings.empty (attributes)) {
			result.append (attributes);
		}
		if (!Strings.empty (other_attributes)) {
			if (result.len > 0)
				result.append (",");
			result.append (other_attributes);
		}
		return result.str;
	}

	public static bool attribute_matches_pattern (string attribute, string pattern) {
		var pattern_end = pattern.index_of_char ('*');
		if (pattern_end < 0) {
			return attribute == pattern;
		}
		else {
			return Strings.starts_with (attribute, pattern, pattern_end);
		}
	}

	static bool attributes_match_patterns (string[] attribute_v, string[] pattern_v) {
		foreach (unowned var attribute in attribute_v) {
			foreach (unowned var pattern in pattern_v) {
				if (Util.attribute_matches_pattern (attribute, pattern)) {
					return true;
				}
			}
		}
		return false;
	}

	public static bool attributes_match_any_pattern_v (string[] attribute_v, string[] pattern_v) {
		return Util.attributes_match_patterns (attribute_v, pattern_v)
			|| Util.attributes_match_patterns (pattern_v, attribute_v);
	}

	public static bool attributes_match_any_pattern (string attributes, string patterns) {
		var attribute_v = attributes.split (",");
		var pattern_v = patterns.split (",");
		return attributes_match_any_pattern_v (attribute_v, pattern_v);
	}

	public static bool attributes_match_all_patterns (string attributes, string patterns) {
		var matches_all = true;
		var attributes_v = attributes.split (",");
		var pattern_v = patterns.split (",");
		foreach (unowned var attribute in attributes_v) {
			var matches_mask = false;
			foreach (unowned var pattern in pattern_v) {
				if (Util.attribute_matches_pattern (attribute, pattern)) {
					matches_mask = true;
					break;
				}
			}
			if (!matches_mask) {
				matches_all = false;
				break;
			}
		}
		return matches_all;
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

	public static string remove_extension (string filename) {
		var ext_idx = filename.last_index_of_char ('.');
		if (ext_idx < 0)
			return filename;
		return filename.substring (0, ext_idx);
	}

	public static Gth.FileData? get_nearest_parent (File file, GenericList<FileData> locations) {
		Gth.FileData nearest_parent = null;
		var file_uri = Uri.parse (file.get_uri (), UriFlags.PARSE_RELAXED);
		var file_path = file_uri.get_path ();
		var max_length = 0;
		foreach (unowned var location in locations) {
			var location_uri = Uri.parse (location.file.get_uri (), UriFlags.PARSE_RELAXED);
			if (location_uri.get_scheme () == file_uri.get_scheme ()) {
				var location_path = location_uri.get_path ();
				if (file_path.has_prefix (location_path)) {
					var len = location_path.length;
					if (len > max_length) {
						nearest_parent = location;
						max_length = len;
					}
				}
			}
		}
		return nearest_parent;
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

	public static int get_extension_start (string? filename, int bytes = -1) {
		if (filename == null)
			return -1;
		if (bytes < 0)
			bytes = filename.data.length;
		int byte_offset = bytes;
		int next_byte_offset = byte_offset;
		unichar ch;
		while (filename.get_prev_char (ref byte_offset, out ch)) {
			if (ch == '.') {
				if (byte_offset <= 0)
					return -1;
				// .tar.gz
				//      ^
				if (Strings.ends_with (filename, ".tar", 4, byte_offset))
					next_byte_offset -= 4; // 4 = ".tar".length
				return next_byte_offset;
			}
			next_byte_offset = byte_offset;
		}
		return -1;
	}

	public static string? get_extension (string? filename) {
		if (filename == null)
			return null;
		var bytes = filename.data.length;
		if (bytes == 0)
			return null;
		var start = get_extension_start (filename, bytes);
		if (start < 0)
			return null;
		return filename.slice (start, bytes).down ();
	}

	public static void enable_action (SimpleActionGroup action_group, string name, bool enabled) {
		var action = action_group.lookup_action (name) as SimpleAction;
		if (action != null) {
			action.set_enabled (enabled);
		}
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
}
