public class Gth.Selections {
	public Selections () {
		entries = new Selection[3];
		loaded = false;
		for (var i = 0; i < Selection.MAX_SELECTIONS; i++) {
			entries[i] = new Selection (i + 1);
		}
	}

	public int get_size () {
		return entries.length;
	}

	public unowned Selection? get_selection (uint number) {
		return ((number > 0) && (number <= entries.length)) ? entries[number - 1] : null;
	}

	public Selection? get_selection_from_file (File file) {
		var number = Selection.get_number (file);
		return get_selection (number);
	}

	public bool update_private_attribute (File file, FileInfo file_info) {
		var changed = false;
		uint[] selections = null;
		for (var i = 0; i < entries.length; i++) {
			unowned var selection = entries[i];
			if ((selection != null) && selection.contains (file)) {
				if (selections == null) {
					selections = new uint[0];
				}
				selections += selection.number;
			}
		}

		if (selections != null) {
			file_info.set_attribute_object (PrivateAttribute.SELECTIONS, new UintList.from_uintv (selections));
			changed = true;
		}
		else if (file_info.has_attribute (PrivateAttribute.SELECTIONS)) {
			file_info.remove_attribute (PrivateAttribute.SELECTIONS);
			changed = true;
		}
		return changed;
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
