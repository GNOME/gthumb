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

	public static ByteArray read_all (InputStream stream, Cancellable? cancellable = null) throws Error {
		var result = new ByteArray ();
		var buffer = new uint8[BUFFER_SIZE];
		while (true) {
			var size = stream.read (buffer, cancellable);
			if (size <= 0)
				break;
			unowned var valid_bytes = buffer[0:size];
			result.append (valid_bytes);
		}
		return result;
	}

	public static string load_content (File file, Cancellable? cancellable = null) throws Error {
		var stream = file.read (cancellable);
		var bytes = Files.read_all (stream, cancellable);
		bytes.append ({ 0 }); // Add null to terminate the string.
		return (string) bytes.steal ();
	}

	public static void save_content (File file, string content, Cancellable? cancellable = null) throws Error {
		var stream = file.replace (null, false, FileCreateFlags.NONE, cancellable);
		stream.write_all (content.data, null, cancellable);
		stream.close (cancellable);
	}
}
