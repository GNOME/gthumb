public class Gth.FilterFile {
	public ListStore filters;

	public FilterFile () {
		filters = new ListStore (typeof (Gth.Test));
		load ();
	}

	public int find_test (Test test) {
		var iter = new ListModelIterator (filters);
		while (iter.next ()) {
			var filter = iter.get () as Gth.Test;
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
			filters.insert (idx, test);
		}
		else {
			filters.append (test);
		}
	}

	public void remove (Test test) {
		var idx = find_test (test);
		if (idx >= 0) {
			filters.remove (idx);
		}
	}

	public void clear () {
		filters.remove_all ();
	}

	public bool load () {
		filters.remove_all ();
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
						filters.append (filter);
					}
					else if (node.tag_name == "test") {
						unowned var id = node.get_attribute ("id");
						if (id != null) {
							var registered_test = app.get_test_by_id (id);
							if (registered_test != null) {
								var test = Object.new (registered_test.get_type ()) as Gth.Test;
								test.load_from_element (node);
								filters.append (test);
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
		var iter = new ListModelIterator (filters);
		while (iter.next ()) {
			var filter = iter.get () as Gth.Test;
			root.append_child (filter.create_element (doc));
		}
		return doc.to_xml ();
	}
}
