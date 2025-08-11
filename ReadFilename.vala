public class Gth.ReadFilename : Gth.ReadText {
	public ReadFilename (string _title, string? _save_label = null) {
		base (_title, _save_label);
		is_filename = true;
		check_func = ReadFilename.check_filename;
	}

	static bool check_filename (string value) throws Error {
		var ext_start = Util.get_extension_start (value);
		if (ext_start <= 0) {
			throw new IOError.FAILED (_("Enter a file extension"));
		}
		var ext = Util.get_extension (value);
		if ((ext == null) || !app.can_save_extension (ext)) {
			throw new IOError.FAILED (_("Cannot save this kind of files"));
		}
		return true;
	}
}
