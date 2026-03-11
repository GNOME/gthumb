public class Gth.Filters {
	public GenericList<Gth.Test> entries;

	public signal void changed (string? changed_id) {
		save_to_file ();
	}

	public Filters () {
		entries = new GenericList<Gth.Test>();
	}

	public int find_test (Test test) {
		var iter = entries.iterator ();
		while (iter.next ()) {
			var filter = iter.get ();
			if (filter.id == test.id) {
				return iter.index ();
			}
		}
		return -1;
	}

	public bool has_test (Test test) {
		return find_test (test) >= 0;
	}

	public void add (Test test) {
		var idx = find_test (test);
		if (idx >= 0) {
			entries.model.splice (idx, 1, { test });
		}
		else {
			entries.model.append (test);
		}
		changed (test.id);
	}

	public void remove (Test test) {
		var idx = find_test (test);
		if (idx >= 0) {
			entries.model.remove (idx);
		}
		changed (test.id);
	}

	public void load_from_file () {
		entries.model.remove_all ();
		app.shortcuts.remove_action ("set-filter");
		var loaded_tests = new GenericSet<string> (str_hash, str_equal);
		var file_loaded = false;
		try {
			var file = Gth.UserDir.get_config_file (Gth.FileIntent.READ, FILTERS_FILE);
			if (file == null) {
				throw new IOError.NOT_FOUND ("Not Found");
			}
			var doc = new Dom.Document ();
			doc.load_file (file);
			if ((doc.first_child != null) && (doc.first_child.tag_name == "filters")) {
				foreach (unowned var node in doc.first_child) {
					if (node.tag_name == "filter") {
						var filter = new Gth.Filter ();
						filter.load_from_element (node);
						entries.model.append (filter);
						app.shortcuts.add (filter.create_shortcut ());
					}
					else if (node.tag_name == "test") {
						unowned var id = node.get_attribute ("id");
						if (id != null) {
							var registered_test = app.get_test_by_id (id);
							if ((registered_test != null) && !loaded_tests.contains (registered_test.id)) {
								var test = Object.new (registered_test.get_type ()) as Gth.Test;
								test.load_from_element (node);
								entries.model.append (test);
								app.shortcuts.add (registered_test.create_shortcut ());
								loaded_tests.add (test.id);
							}
						}
					}
				}
			}
			file_loaded = true;
		}
		catch (Error error) {
			//stdout.printf ("ERROR: %s\n", error.message);
		}

		// Add the remaining registered tests as not visible.
		foreach (unowned var registered_test in app.ordered_tests) {
			if (!loaded_tests.contains (registered_test.id)) {
				var test = registered_test.duplicate ();
				if (file_loaded) {
					test.visible = false;
				}
				else {
					test.visible = is_active_by_default (registered_test.id);
				}
				entries.model.append (test);
				app.shortcuts.add (registered_test.create_shortcut ());
			}
		}
		changed (null);
	}

	bool is_active_by_default (string test_id) {
		foreach (unowned var id in DEFAULT_ACTIVE_TESTS) {
			if (test_id == id) {
				return true;
			}
		}
		return false;
	}

	const string[] DEFAULT_ACTIVE_TESTS = {
		"File::Name",
		"File::Size",
		"Time::Modified",
		"Time::Created",
		"Type::Video",
		"Type::Audio"
	};

	public void save_to_file () {
		try {
			var doc = new Dom.Document ();
			var root = new Dom.Element.with_attributes ("filters", "version", FILTER_FORMAT);
			doc.append_child (root);
			foreach (unowned var filter in entries) {
				root.append_child (filter.create_element (doc));
			}
			var file = UserDir.get_config_file (FileIntent.WRITE, FILTERS_FILE);
			Files.save_content (file, doc.to_xml ());
		}
		catch (Error error) {
		}
	}

	const string FILTER_FORMAT = "1.0";
}
