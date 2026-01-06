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

	public void add_files (GenericList<File> files) {
		foreach (unowned var file in files) {
			add_file (file);
		}
		var location = File.new_for_uri ("selection:///%u".printf (number));
		app.monitor.files_added (location, files);
		app.monitor.selection_changed (number);
	}

	public void remove_files (GenericList<File> files) {
		foreach (unowned var file in files) {
			remove_file (file);
		}
		var location = File.new_for_uri ("selection:///%u".printf (number));
		app.monitor.files_removed_from_catalog (location, files);
		app.monitor.selection_changed (number);
	}

	public void set_files (GenericList<File> files) {
		sort_type = "Private::Unsorted";
		inverse_order = false;
		remove_all_files ();
		foreach (unowned var file in files) {
			add_file (file);
		}
		// TODO app.monitor.order_changed (location);
	}

	public void files_renamed (GenericList<RenamedFile> renamed_files) {
		foreach (var renamed in renamed_files) {
			int new_pos = -1;

			// Remove the old file
			if (file_set.contains (renamed.old_file)) {
				uint pos;
				if (files.find_with_equal_func (renamed.old_file, Util.file_equal, out pos)) {
					files.remove_index (pos);
					file_set.remove (renamed.old_file);
					new_pos = (int) pos;
				}
			}

			// Add the new file at the same position.
			if (!file_set.contains (renamed.new_file)) {
				if (new_pos >= 0) {
					files.insert (new_pos, renamed.new_file);
				}
				else {
					files.add (renamed.new_file);
				}
				file_set.add (renamed.new_file);
			}
		}
	}

	public GenericArray<File> get_files () {
		var new_files = new GenericArray<File>();
		foreach (unowned var file in files) {
			new_files.add (file);
		}
		return new_files;
	}

	public static FileData get_root () {
		var file = File.new_for_uri (ROOT);
		var info = new FileInfo ();
		Selection.update_file_info (file, info);
		return new FileData (file, info);
	}

	public static uint get_number (File file) {
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

	const string[] ICONS = {
		"gth-flag-symbolic",
		"gth-flag-1-symbolic",
		"gth-flag-2-symbolic",
		"gth-flag-3-symbolic",
	};

	public static void update_file_info (File file, FileInfo info) {
		var number = Selection.get_number (file);

		info.set_file_type (FileType.DIRECTORY);
		info.set_symbolic_icon (new ThemedIcon (ICONS[number]));
		info.set_sort_order ((int) number);
		info.set_attribute_boolean (FileAttribute.ACCESS_CAN_READ, true);
		if (number > 0) {
			info.set_content_type ("gthumb/selection");
			info.set_attribute_boolean (FileAttribute.ACCESS_CAN_WRITE, true);
		}
		else {
			info.set_content_type ("gthumb/selections");
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

	public Dom.Element create_element (Dom.Document doc) {
		var node = new Dom.Element.with_attributes ("selection", "number", "%u".printf (number));
		node.append_child (new Dom.Element.with_attributes ("order", "type", sort_type, "inverse", inverse_order ? "1" : "0"));
		var files_node = new Dom.Element ("files");
		foreach (unowned var file in files) {
			files_node.append_child (new Dom.Element.with_attributes ("file", "uri", file.get_uri ()));
		}
		node.append_child (files_node);
		return node;
	}

	public void load_from_element (Dom.Element node) {
		foreach (unowned var child in node) {
			switch (child.tag_name) {
			case "files":
				foreach (unowned var file_node in child) {
					unowned var uri = file_node.get_attribute ("uri");
					if (uri != null) {
						add_file (File.new_for_uri (uri));
					}
				}
				break;

			case "order":
				sort_type = app.migration.sorter.get_new_key (child.get_attribute ("type"));
				inverse_order = child.get_attribute ("inverse") == "1";
				break;

			default:
				break;
			}
		}
	}

	const string ROOT = "selection:///";
}
