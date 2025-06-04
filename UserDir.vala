public class Gth.UserDir {
	public static File? get_directory_v (Gth.FileIntent intent, Gth.DirType type, string[] children) {
		File? directory = null;
		switch (type) {
		case Gth.DirType.CONFIG:
			directory = File.new_for_path (GLib.Environment.get_user_config_dir ());
			break;
		case Gth.DirType.CACHE:
			directory = File.new_for_path (GLib.Environment.get_user_cache_dir ());
			break;
		case Gth.DirType.DATA:
			directory = File.new_for_path (GLib.Environment.get_user_data_dir ());
			break;
		}
		return (directory != null) ? Files.build_directory_v (intent, directory, children) : null;
	}

	public static File? get_directory (Gth.FileIntent intent, Gth.DirType type, ...) {
		var array = new GenericArray<string> ();
		var list = va_list ();
		while (true) {
			string child = list.arg ();
			if (child == null)
				break;
			array.add (child);
		}
		var children = array.steal ();
		return get_directory_v (intent, type, children);
	}

	public static File? get_config_file (Gth.FileIntent intent, string filename) {
		var dir = UserDir.get_directory (intent, DirType.CONFIG, APP_DIR);
		return (dir != null) ? dir.get_child (filename) : null;
	}
}

public enum Gth.DirType {
	CONFIG,
	CACHE,
	DATA
}
