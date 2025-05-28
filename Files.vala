public class Gth.Files {
	public static File get_home_dir () {
		return File.new_for_path (Environment.get_home_dir ());
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

	const int BUFFER_SIZE = 8192;
	const uint8[] ZERO = {0};

	public static Bytes read_all (InputStream stream, Cancellable? cancellable = null, bool add_zero = false) throws Error {
		var result = new ByteArray ();
		var buffer = new uint8[BUFFER_SIZE];
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

	public static async Bytes read_all_async (InputStream stream, Cancellable? cancellable = null) throws Error {
		var result = new ByteArray ();
		var buffer = new uint8[BUFFER_SIZE];
		while (true) {
			size_t size;
			if (!yield stream.read_all_async (buffer, Priority.DEFAULT, cancellable, out size))
				break;
			unowned var valid_bytes = buffer[0:size];
			result.append (valid_bytes);
		}
		return ByteArray.free_to_bytes (result);
	}

	public static string load_contents_as_string (File file, Cancellable? cancellable = null) throws Error {
		var stream = file.read (cancellable);
		var bytes = Files.read_all (stream, cancellable, true);
		return (string) Bytes.unref_to_data (bytes);
	}

	public static void save_content (File file, string content, Cancellable? cancellable = null) throws Error {
		var stream = file.replace (null, false, FileCreateFlags.NONE, cancellable);
		stream.write_all (content.data, null, cancellable);
		stream.close (cancellable);
	}
}
