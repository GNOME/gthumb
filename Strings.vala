public class Gth.Strings {
	// True if the string contains no data.
	public static bool empty (string? str) {
		return (str == null) || (str[0] == 0);
	}

	static HashTable<string, int> static_strings;

	public static unowned string get_static (string str) {
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
}
