public class Gth.UserDir {
	public static File? get_dir (UserDirType type) {
		File? dir = null;
		switch (type) {
		case UserDirType.CONFIG:
			dir = File.new_for_path (GLib.Environment.get_user_config_dir ());
			break;
		case UserDirType.CACHE:
			dir = File.new_for_path (GLib.Environment.get_user_cache_dir ());
			break;
		case UserDirType.DATA:
			dir = File.new_for_path (GLib.Environment.get_user_data_dir ());
			break;
		}
		return dir;
	}

	public static File? get_app_dir (UserDirType type) {
		var base_dir = UserDir.get_dir (type);
		return (base_dir != null) ? base_dir.get_child ("gthumb") : null;
	}

	public static File? get_app_file (UserDirType type, FileIntent intent, ...) {
		File app_dir = UserDir.get_app_dir (type);
		if (app_dir == null)
			return null;
		if ((intent == FileIntent.WRITE) && !Files.ensure_directory_exists (app_dir))
			return null;
		var result = app_dir;
		var list = va_list ();
		while (true) {
			string? child = list.arg ();
			if (child == null)
				break;
			if ((intent == FileIntent.WRITE) && !Files.make_directory (result))
				return null;
			result = result.get_child (child);
		}
		return result;
	}
}

public enum Gth.UserDirType {
	CONFIG,
	CACHE,
	DATA
}

public enum Gth.FileIntent {
	READ,
	WRITE,
}
