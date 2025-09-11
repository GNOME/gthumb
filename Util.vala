namespace Gth.Util {
	// 'video/mp4' -> 'video/*'
	public static string get_generic_type (string content_type) {
		var end = content_type.index_of_char ('/');
		if (end < 0)
			return content_type;
		return content_type.substring (0, end + 1) + "*";
	}

	public static int get_digits (int number) {
		int digits = 1;
		if (number < 0) {
			digits++;
			number = -number;
		}
		while (number >= 10) {
			number = number / 10;
			digits++;
		}
		return digits;
	}

	public static int get_extension_start (string? filename, int bytes = -1) {
		if (filename == null) {
			return -1;
		}
		if (bytes < 0) {
			bytes = filename.data.length;
		}
		int byte_offset = bytes;
		int next_byte_offset = byte_offset;
		unichar ch;
		while (filename.get_prev_char (ref byte_offset, out ch)) {
			if (ch == '.') {
				if (byte_offset <= 0) {
					return -1;
				}
				// .tar.gz
				//      ^
				if (Strings.ends_with (filename, ".tar", 4, byte_offset)) {
					byte_offset -= 4; // ".tar".length
				}
				return byte_offset;
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
		var start = Util.get_extension_start (filename, bytes);
		if (start < 0)
			return null;
		return filename.slice (start + 1, bytes).down ();
	}

	public static string remove_extension (string filename) {
		var ext_idx = Util.get_extension_start (filename);
		if (ext_idx < 0)
			return filename;
		return filename.substring (0, ext_idx);
	}

	public static string uri_from_path (string path) {
		return "file://" + GLib.Uri.escape_string (path, "/", true);
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

	public static bool attributes_match_any_pattern_v (string[] attribute_v, string[]? pattern_v) {
		return (pattern_v == null)
			|| Util.attributes_match_patterns (attribute_v, pattern_v)
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

	public static File get_duplicated (File file) {
		var uri = file.get_uri ();
		string uri_no_ext;
		string ext;
		var ext_start = Util.get_extension_start (uri);
		if (ext_start > 0) {
			uri_no_ext = uri.slice (0, ext_start);
			ext = uri.slice (ext_start, uri.length);
		}
		else {
			uri_no_ext = uri;
			ext = "";
		}
		var new_uri = new StringBuilder ();
		try {
			var regex = new Regex ("^(.*)%20\\(([0-9]+)\\)$");
			MatchInfo info;
			regex.match (uri_no_ext, 0, out info);
			if (info.matches ()) {
				new_uri.append (info.fetch (1));
				var n = uint.parse (info.fetch (2), 10);
				new_uri.append_printf ("%%20(%u)", n + 1);
			}
			else {
				throw new IOError.FAILED ("Not a duplicate");
			}
		}
		catch (Error error) {
			new_uri.append (uri_no_ext);
			new_uri.append ("%20(2)");
		}
		new_uri.append (ext);
		return File.new_for_uri (new_uri.str);
	}

	public static float calc_progress (uint64 completed, uint64 total) {
		return (float) (completed + 1) / (total + 1);
	}

	public static unowned string get_basename (string path, int bytes = -1) {
		if (bytes < 0) {
			bytes = path.data.length;
		}
		var byte_offset = bytes;
		var next_byte_offset = byte_offset;
		unichar ch;
		while (path.get_prev_char (ref byte_offset, out ch)) {
			if (ch == '/') {
				return (string) (path.data[next_byte_offset:bytes]);
			}
			next_byte_offset = byte_offset;
		}
		return path;
	}

	public static string get_parse_basename (File file) {
		var pathname = file.get_parse_name ();
		return Util.get_basename (pathname);
	}

	public static File build_destination_dir (File source, File source_base_dir, File destination_base_dir) {
		var uri = source.get_uri ();
		var base_uri = source_base_dir.get_uri ();
		var uri_suffix = uri.substring (base_uri.length, -1);
		var destination_base_uri = destination_base_dir.get_uri ();
		var destination_uri = destination_base_uri + "/" + uri_suffix;
		//stdout.printf ("> build_destination_dir('%s', '%s', '%s'): %s\n",
		//	uri, base_uri, destination_base_uri, destination_uri);
		return File.new_for_uri (destination_uri);
	}
}
