public class Gth.ReadFilename : Gth.ReadText {
	public bool check_extension;
	public bool check_exists;
	public File folder;

	public ReadFilename (string _title, string? _save_label = null) {
		base (_title, _save_label);
		is_filename = true;
		check_extension = false;
		check_exists = false;
		folder = null;
		check_func = check_filename;
	}

	bool check_filename (string value) throws Error {
		if (check_extension) {
			var ext_start = Util.get_extension_start (value);
			if (ext_start <= 0) {
				throw new IOError.FAILED (_("Enter a file extension"));
			}
			var ext = Util.get_extension (value);
			if ((ext == null) || !app.can_save_extension (ext)) {
				throw new IOError.FAILED (_("Cannot save this kind of files"));
			}
		}
		if (check_exists && (folder != null)) {
			var destination = folder.get_child_for_display_name (value);
			if (destination.query_exists (null)) {
				throw new IOError.FAILED (_("Name already used"));
			}
		}
		return true;
	}
}
