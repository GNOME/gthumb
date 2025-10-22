public class Gth.Selection {
	public const uint MAX_SELECTIONS = 3;

	public uint number;
	public string sort_type;
	public bool inverse_order;
	public GenericArray<File> files;
	public GenericSet<File> file_set;

	public Selection (uint _number) {
		number = _number;
		files = new GenericArray<File>();
		file_set = new GenericSet<File> (Util.file_hash, Util.file_equal);
		sort_type = "Private::Unsorted";
		inverse_order = false;
	}

	public void remove_all_files () {
		files.length = 0;
		file_set.remove_all ();
	}

	public bool add_file (File file) {
		if (file_set.contains (file)) {
			return false;
		}
		files.add (file);
		file_set.add (file);
		return true;
	}

	public bool remove_file (File file) {
		if (!file_set.contains (file)) {
			return false;
		}
		uint pos;
		if (files.find_with_equal_func (file, Util.file_equal, out pos)) {
			files.remove_index (pos);
		}
		file_set.remove (file);
		return true;
	}

	public void set_files (GenericList<File> files) {
		sort_type = "Private::Unsorted";
		inverse_order = false;
		remove_all_files ();
		foreach (unowned var file in files) {
			add_file (file);
		}
	}

	public static FileData get_root () {
		var file = File.new_for_uri (ROOT);
		var info = new FileInfo ();
		Selection.update_file_info (file, info);
		return new FileData (file, info);
	}

	public static uint get_selection_number (File file) {
		var uri = file.get_uri ();
		if (!uri.has_prefix (ROOT)) {
			return 0;
		}
		if (uri == ROOT) {
			return 0;
		}
		unowned string path = Strings.unowned_substring (uri, ROOT.length);
		uint number;
		if (!uint.try_parse (path, out number, null, 10)) {
			return 0;
		}
		if (number > MAX_SELECTIONS) {
			return 0;
		}
		return number;
	}

	public static FileData get_selection_data (uint number) {
		var file = File.new_for_uri ("selection:///%u".printf (number));
		var info = new FileInfo ();
		Selection.update_file_info (file, info);
		return new FileData (file, info);
	}

	public static void update_file_info (File file, FileInfo info) {
		var number = Selection.get_selection_number (file);

		info.set_file_type (FileType.DIRECTORY);
		info.set_content_type ("gthumb/selection");
		info.set_symbolic_icon (new ThemedIcon ("gth-flag-symbolic"));
		info.set_sort_order ((int) number);
		info.set_attribute_boolean (FileAttribute.ACCESS_CAN_READ, true);
		if (number > 0) {
			info.set_attribute_boolean (FileAttribute.ACCESS_CAN_WRITE, true);
		}
		info.set_attribute_boolean (FileAttribute.ACCESS_CAN_DELETE, false);
		info.set_attribute_boolean (FileAttribute.ACCESS_CAN_RENAME, false);
		info.set_attribute_boolean (FileAttribute.STANDARD_IS_HIDDEN, false);

		if (number > 0) {
			info.set_attribute_boolean ("gthumb::no-child", true);
			// Translators: %u is replaced with a number
			info.set_display_name (_("Selection %u").printf (number));
			info.set_name ("%u".printf (number));
			var selection = app.selections.get_selection (number);
			if (selection != null) {
				info.set_attribute_string ("sort::type", selection.sort_type);
				info.set_attribute_boolean ("sort::inverse", selection.inverse_order);
			}
		}
		else {
			info.set_attribute_boolean ("gthumb::no-child", false);
			info.set_display_name (_("Selections"));
			info.set_name ("");
		}
	}

	const string ROOT = "selection:///";
}
