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

	public void add_filename_generator () {
		generator = new TextGenerator () {
			tooltip = _("Automatic Filename"),
			icon_name = "gth-script-symbolic",
			generate = get_unused_filename,
		};
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

	string get_unused_filename (string value) throws Error {
		if (folder == null) {
			return value;
		}
		var tries = 0u;
		var destination = folder.get_child_for_display_name (value);
		while (destination.query_exists (null)) {
			tries++;
			if (tries >= MAX_TRIES) {
				throw new IOError.FAILED ("Too many files");
			}
			destination = Util.get_duplicated (destination);
		}
		return destination.get_basename ();
	}

	const uint MAX_TRIES = 1000;
}
