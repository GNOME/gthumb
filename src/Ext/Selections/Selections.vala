public class Gth.Selections {
	public Selections () {
		entries = new Selection[3];
		loaded = false;
		for (var i = 0; i < Selection.MAX_SELECTIONS; i++) {
			entries[i] = new Selection (i + 1);
		}
	}

	public Selection? get_selection (uint number) {
		return ((number > 0) && (number <= Selection.MAX_SELECTIONS)) ? entries[number - 1] : null;
	}

	public Selection? get_selection_from_file (File file) {
		var number = Selection.get_number (file);
		return get_selection (number);
	}

	public void save_to_file () {
		try {
			var doc = new Dom.Document ();
			var root = new Dom.Element.with_attributes ("selections", "version", FILE_FORMAT);
			doc.append_child (root);
			foreach (unowned var selection in entries) {
				root.append_child (selection.create_element (doc));
			}
			var file = UserDir.get_config_file (FileIntent.WRITE, SELECTIONS_FILE);
			Files.save_content (file, doc.to_xml ());
		}
		catch (Error error) {
		}
	}

	public bool load_from_file () {
		if (loaded) {
			return false;
		}
		var local_job = app.jobs.new_job ("Loading Selections");
		try {
			var tools_file = UserDir.get_config_file (FileIntent.READ, SELECTIONS_FILE);
			var contents = Files.load_contents (tools_file, local_job.cancellable);
			var doc = new Dom.Document ();
			doc.load_xml (contents);
			if ((doc.first_child != null) && (doc.first_child.tag_name == "selections")) {
				foreach (unowned var node in doc.first_child) {
					if (node.tag_name == "selection") {
						uint number;
						if (node.get_attribute_as_uint ("number", out number)) {
							var selection = get_selection (number);
							if (selection != null) {
								selection.load_from_element (node);
							}
						}
					}
				}
			}
		}
		catch (Error error) {
			// Ignored
			// stdout.printf (">> ERROR: %s\n", error.message);
		}
		local_job.done ();
		loaded = true;
		return true;
	}

	Selection[] entries;
	bool loaded;
	const string FILE_FORMAT = "1.0";
}
