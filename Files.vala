public class Gth.Files {
	static File home = null;

	public static File get_home () {
		if (home == null) {
			home = File.new_for_path (Environment.get_home_dir ());
		}
		return home;
	}

	public static string expand_home_uri (string uri) {
		if (!uri.has_prefix ("file://~")) {
			return uri;
		}
		return uri.replace ("file://~", Files.get_home ().get_uri ());
	}

	static File root = null;

	public static File get_root () {
		if (root == null) {
			root = File.new_for_uri ("file:///");
		}
		return root;
	}

	public static File? get_special_dir (UserDirectory directory) {
		var path = Environment.get_user_special_dir (directory);
		return (path != null) ? File.new_for_path (path) : null;
	}

	public static File? build_directory_v (Gth.FileIntent intent, File basedir, string[] children) {
		var directory = basedir;
		if ((intent == Gth.FileIntent.WRITE) && !Files.make_directory (directory))
			return null;
		foreach (unowned var child in children) {
			directory = directory.get_child (child);
			if ((intent == Gth.FileIntent.WRITE) && !Files.make_directory (directory)) {
				return null;
			}
		}
		return directory;
	}

	public static File? build_directory (Gth.FileIntent intent, File basedir, ...) {
		var array = new GenericArray<string> ();
		var list = va_list ();
		while (true) {
			string child = list.arg ();
			if (child == null)
				break;
			array.add (child);
		}
		var children = array.steal ();
		return Files.build_directory_v (intent, basedir, children);
	}

	public static bool is_directory (File file) {
		try {
			var info = file.query_info (FileAttribute.STANDARD_TYPE, FileQueryInfoFlags.NONE);
			return info.get_file_type () == FileType.DIRECTORY;
		}
		catch (Error error) {
			return false;
		}
	}

	public static bool make_directory (File dir) {
		try {
			dir.make_directory ();
		}
		catch (IOError e) {
			if (e.code != IOError.EXISTS)
				return false;
		}
		catch (Error e) {
			return false;
		}
		return true;
	}

	public static bool ensure_directory_exists (File dir) {
		try {
			dir.make_directory_with_parents ();
		}
		catch (IOError e) {
			if (e.code != IOError.EXISTS)
				return false;
		}
		catch (Error e) {
			return false;
		}
		return true;
	}

	public static File get_temp_file (string suffix = "") {
		var filename = Strings.new_random (10) + suffix;
		var path = Path.build_filename (Environment.get_tmp_dir (), filename);
		return File.new_for_path (path);
	}

	const int BUFFER_SIZE = 256 * 1024;

	public static Bytes read_all (InputStream stream, Cancellable? cancellable = null, bool add_zero = false) throws Error {
		var buffer = new uint8[BUFFER_SIZE];
		return Files.read_all_with_buffer (stream, cancellable, buffer, add_zero);
	}

	const uint8[] ZERO = {0};

	public static Bytes read_all_with_buffer (InputStream stream, Cancellable? cancellable = null, uint8[] buffer, bool add_zero = false) throws Error {
		var result = new ByteArray ();
		while (true) {
			var size = stream.read (buffer, cancellable);
			if (size <= 0)
				break;
			unowned var valid_bytes = buffer[0:size];
			result.append (valid_bytes);
		}
		if (add_zero) {
			result.append (ZERO); // Add null to terminate the string.
		}
		return ByteArray.free_to_bytes (result);
	}

	public static async Bytes read_all_async (InputStream stream, Cancellable? cancellable = null, bool add_zero = false) throws Error {
		var result = new ByteArray ();
		var buffer = new uint8[BUFFER_SIZE];
		while (true) {
			size_t size;
			yield stream.read_all_async (buffer, Priority.DEFAULT, cancellable, out size);
			if (size == 0)
				break;
			unowned var valid_bytes = buffer[0:size];
			result.append (valid_bytes);
		}
		if (add_zero) {
			result.append (ZERO); // Add null to terminate the string.
		}
		return ByteArray.free_to_bytes (result);
	}

	public static Bytes load_file (File file, Cancellable? cancellable = null) throws Error {
		var stream = file.read (cancellable);
		return Files.read_all (stream, cancellable, true);
	}

	public static string load_contents (File file, Cancellable? cancellable = null) throws Error {
		var stream = file.read (cancellable);
		var bytes = Files.read_all (stream, cancellable, true);
		return (string) Bytes.unref_to_data (bytes);
	}

	public static async Bytes load_file_async (File file, Cancellable? cancellable = null, bool add_zero = false) throws Error {
		var stream = yield file.read_async (Priority.DEFAULT, cancellable);
		return yield Files.read_all_async (stream, cancellable, add_zero);
	}

	public static async string load_contents_async (File file, Cancellable? cancellable = null) throws Error {
		var bytes = yield Files.load_file_async (file, cancellable, true);
		return (string) Bytes.unref_to_data (bytes);
	}

	public static void save_content (File file, string content, Cancellable? cancellable = null) throws Error {
		var stream = file.replace (null, false, FileCreateFlags.NONE, cancellable);
		stream.write_all (content.data, null, cancellable);
		stream.close (cancellable);
	}

	public static void save_file (File file, Bytes bytes, Cancellable? cancellable = null) throws Error {
		var stream = file.replace (null, false, FileCreateFlags.NONE, cancellable);
		stream.write_bytes (bytes, cancellable);
		stream.close (cancellable);
	}

	public static async void save_file_async (File file, Bytes bytes, Cancellable? cancellable = null) throws Error {
		var stream = yield file.replace_async (null, false, FileCreateFlags.NONE, Priority.DEFAULT, cancellable);
		yield stream.write_bytes_async (bytes, Priority.DEFAULT, cancellable);
		yield stream.close_async (Priority.DEFAULT, cancellable);
	}

	public static async FileInfo query_info (File file, string attributes, Cancellable cancellable) throws Error {
		return yield file.query_info_async (
				attributes,
				FileQueryInfoFlags.NONE,
				Priority.DEFAULT,
				cancellable);
	}

	public static async void make_directory_async (File dir, Cancellable cancellable) {
		try {
			yield dir.make_directory_async (Priority.DEFAULT, cancellable);
		}
		catch (Error error) {
			if (!(error is IOError.EXISTS)) {
				throw error;
			}
		}
	}

	public static async bool query_exists (File file, Cancellable cancellable) {
		try {
			var info = yield file.query_info_async (FileAttribute.STANDARD_TYPE,
				FileQueryInfoFlags.NONE,
				Priority.DEFAULT,
				cancellable);
			return true;
		}
		catch (Error error) {
			return false;
		}
	}
}

public enum Gth.FileIntent {
	READ,
	WRITE,
}
