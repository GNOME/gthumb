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
		return ContentType.is_a (content_type, "video/*")
			|| (content_type == "application/ogg")
			|| (content_type == "application/x-matroska")
			|| (content_type == "application/vnd.ms-asf")
			|| (content_type == "application/vnd.rn-realmedia");
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
		foreach (unowned var info in app.metadata_info_v) {
			if (matcher.matches (info.id)) {
				result += info.id;
			}
		}
		return result;
	}

	const int BUFFER_SIZE_FOR_SNIFFING = 32;

	static unowned string? get_mime_type_from_magic_numbers (uint8[] buffer, size_t content_size) {
		for (var i = 0; i < MAGIC_IDS.length; i++) {
			unowned var magic = MAGIC_IDS[i];
			if (magic.offset + magic.length > content_size)
				continue;
			unowned uint8[] buffer_data = buffer[magic.offset:-1];
			if (Posix.memcmp (buffer_data, magic.id.data, magic.length) == 0)
				return magic.mime_type;
		}
		return null;
	}

	struct MagicInfo {
		string mime_type;
		uint offset;
		uint length;
		string id;
	}

	const MagicInfo[] MAGIC_IDS = {
		// Some magic ids taken from magic/Magdir/archive from the file-4.21 tarball.
		{ "image/png",  0,  8, "\x89PNG\x0d\x0a\x1a\x0a" },
		{ "image/tiff", 0,  4, "MM\x00\x2a" },
		{ "image/tiff", 0,  4, "II\x2a\x00" },
		{ "image/gif",  0,  4, "GIF8" },
		{ "image/jpeg", 0,  3, "\xff\xd8\xff" },
		{ "image/webp", 8,  7, "WEBPVP8" },
		{ "image/jxl",  0,  2, "\xff\x0a" },
		{ "image/jxl",  0, 12, "\x00\x00\x00\x0cJXL\x20\x0d\x0a\x87\x0a" },
	};

	public static unowned string? guess_content_type_from_stream (InputStream stream, File? file, Cancellable cancellable) {
		var buffer = new uint8[BUFFER_SIZE_FOR_SNIFFING];
		size_t content_size;
		try {
			stream.read_all (buffer, out content_size, cancellable);
		}
		catch (Error error) {
			return null;
		}
		unowned var content_type = Util.get_mime_type_from_magic_numbers (buffer, content_size);
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
}
