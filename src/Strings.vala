public class Gth.Strings {
	// True if the string contains no data.
	public static bool empty (string? str) {
		return (str == null) || (str[0] == 0);
	}

	public static bool all_spaces (string str) {
		int offset = 0;
		unichar ch;
		while (str.get_next_char (ref offset, out ch)) {
			if (!ch.isspace ()) {
				return false;
			}
		}
		return true;
	}

	public static string remove_newlines (string str) {
		var result = new StringBuilder ();
		int offset = 0;
		unichar ch;
		while (str.get_next_char (ref offset, out ch)) {
			if ((ch == '\n') || (ch == '\r')) {
				result.append_c (' ');
			}
			else {
				result.append_unichar (ch);
			}
		}
		return result.str;
	}

	static HashTable<string, int> static_strings;

	public static unowned string? get_static (string? str) {
		if (str == null)
			return null;
		if (static_strings == null) {
			static_strings = new HashTable<string, int> (str_hash, str_equal);
		}
		unowned string key;
		if (!static_strings.lookup_extended (str, out key, null)) {
			static_strings.insert (str, 1);
			static_strings.lookup_extended (str, out key, null);
		}
		return key;
	}

	public static void escape_xml_quote (StringBuilder str, string value) {
		int index = 0;
		unichar ch = 0;
		while (value.get_next_char (ref index, out ch)) {
			if (ch == '"') {
				str.append ("&quote;");
			}
			else {
				str.append_unichar (ch);
			}
		}
	}

	const string ALPHABET = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
	const int LETTERS = 52;
	const int WHOLE_ALPHABET = 62;

	public static string new_random (int len) {
		var gen = new Rand ();
		var str = new StringBuilder.sized (len + 1);
		var first = true;
		while (len-- > 0) {
			var idx = gen.int_range (0, first ? LETTERS : WHOLE_ALPHABET);
			str.append_c ((char) ALPHABET.data[idx]);
			first = false;
		}
		return str.str;
	}

	public static bool starts_with (string str, string prefix, int prefix_len = -1) {
		if (prefix_len == -1)
			prefix_len = prefix.length;
		int str_offset = 0;
		unichar str_ch = 0;
		int prefix_offset = 0;
		unichar prefix_ch = 0;
		while (str.get_next_char (ref str_offset, out str_ch)
			&& prefix.get_next_char (ref prefix_offset, out prefix_ch))
		{
			if (str_ch != prefix_ch) {
				return false;
			}
			if (prefix_offset >= prefix_len) {
				return true;
			}
		}
		return false;
	}

	public static bool ends_with (string str, string trail, int trail_offset = -1, int str_offset = -1) {
		if (trail_offset == -1)
			trail_offset = trail.data.length;
		if (str_offset == -1)
			str_offset = str.data.length;
		if ((str_offset == 0) && (trail_offset == 0))
			return true;
		unichar str_ch = 0;
		unichar trail_ch = 0;
		while (trail.get_prev_char (ref trail_offset, out trail_ch)
			&& str.get_prev_char (ref str_offset, out str_ch))
		{
			if (str_ch != trail_ch) {
				return false;
			}
		}
		return (str_offset >= 0) && (trail_offset <= 0);
	}

	public static unowned string unowned_substring (string str, int offset) {
		return (string) str.data[offset:-1];
	}

	public static string substring (string str, int max_characters, string? suffix_if_truncated = null) {
		var result = new StringBuilder ();
		var characters = 0;
		var index = 0;
		unichar ch = 0;
		while (str.get_next_char (ref index, out ch)) {
			result.append_unichar (ch);
			characters++;
			if (characters >= max_characters) {
				if (suffix_if_truncated != null) {
					result.append (suffix_if_truncated);
				}
				break;
			}
		}
		return result.str;
	}

	public static string? get_number_in_name (string name) {
		try {
			var re = new Regex ("[0-9]+");
			MatchInfo info;
			re.match (name, 0, out info);
			return info.matches () ? info.fetch (0) : null;
		}
		catch (Error error) {
			return null;
		}
	}
}
