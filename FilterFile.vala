public class Gth.FilterFile {
	public GenericArray<Gth.Test> filters;

	public FilterFile () {
		filters = new GenericArray<Gth.Test>();
		load ();
	}

	public int find_test (Test test) {
		for (var i = 0; i < filters.length; i++) {
			if (filters[i].id == test.id) {
				return i;
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
			filters[idx] = test;
		}
		else {
			filters.add (test);
		}
	}

	public void remove (Test test) {
		var idx = find_test (test);
		if (idx >= 0) {
			filters.remove_index (idx);
		}
	}

	public void clear () {
		filters = new GenericArray<Gth.Test>();
	}

	public bool load () {
		filters = new GenericArray<Gth.Test>();
		var file = Gth.UserDir.get_app_file (Gth.UserDirType.CONFIG, Gth.FileIntent.READ, FILTERS_FILE);
		if (file == null) {
			return false;
		}
		var loaded = false;
		try {
			var doc = new Dom.Document ();
			doc.load_file (file);
			if ((doc.first_child != null) && (doc.first_child.tag_name == "filters")) {
				foreach (unowned var node in doc.first_child) {
					if (node.tag_name == "filter") {
						var filter = new Gth.Filter ();
						filter.load_from_element (node);
						filters.add (filter);
					}
					else if (node.tag_name == "test") {
						unowned var id = node.get_attribute ("id");
						if (id != null) {
							var test_info = app.get_test_by_id (id);
							if (test_info != null) {
								var test = Object.new (test_info.test_type) as Gth.Test;
								if (test != null) {
									test.load_from_element (node);
									filters.add (test);
								}
							}
						}
					}
				}
			}
			loaded = true;
		}
		catch (Error error) {
			stdout.printf ("ERROR: %s\n", error.message);
		}

		return loaded;
	}

	const string FILTER_FORMAT = "1.0";

	public void save () throws Error {
		var file = Gth.UserDir.get_app_file (Gth.UserDirType.CONFIG, Gth.FileIntent.WRITE, FILTERS_FILE);
		Files.save_content (file, to_xml ());
	}

	string to_xml () {
		var doc = new Dom.Document ();
		var root = new Dom.Element.with_attributes ("filters", "version", FILTER_FORMAT);
		doc.append_child (root);
		foreach (unowned var filter in filters) {
			root.append_child (filter.create_element (doc));
		}
		return doc.to_xml ();
	}
}
