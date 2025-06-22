public class Gth.Sorters {
	public static int cmp_basename (Gth.FileData a, Gth.FileData b) {
		unowned var name_a = a.get_filename_sort_key ();
		unowned var name_b = b.get_filename_sort_key ();
		return GLib.strcmp (name_a, name_b);
	}

	public static int cmp_uri (Gth.FileData a, Gth.FileData b) {
		var parent_a = a.file.get_parent ();
		var parent_b = b.file.get_parent ();
		if (parent_a.equal (parent_b)) {
			return Sorters.cmp_basename (a, b);
		}
		else {
			var uri_a = a.file.get_uri ();
			var uri_b = b.file.get_uri ();
			return GLib.strcmp (uri_a, uri_b);
		}
	}

	public static int cmp_size (Gth.FileData a, Gth.FileData b) {
		var size_a = a.info.get_size ();
		var size_b = b.info.get_size ();
		return (size_a < size_b) ? -1 : (size_a > size_b) ? 1 : 0;
	}

	public static int cmp_modified_time (Gth.FileData a, Gth.FileData b) {
		var time_a = a.get_modification_time ();
		var time_b = b.get_modification_time ();
		return time_a.compare (time_b);
	}

	public static int cmp_frame_dimensions (Gth.FileData a, Gth.FileData b) {
		return 0;
	}

	public static int cmp_aspect_ratio (Gth.FileData a, Gth.FileData b) {
		return 0;
	}
}
