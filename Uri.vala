class Gth.Uri {
	public static string uri_from_path (string path) {
		return "file://" + GLib.Uri.unescape_string (path, GLib.Uri.RESERVED_CHARS_ALLOWED_IN_PATH);
	}
}
