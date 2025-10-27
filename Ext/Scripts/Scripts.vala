public class Gth.Scripts {
	public GenericList<Script> entries;

	public signal void changed (string? script_id = null) {
		save_to_file ();
		app.monitor.scripts_changed ();
	}

	public Scripts () {
		entries = new GenericList<Script> ();
		loaded = false;
	}

	public bool load_from_file () {
		if (loaded) {
			return false;
		}
		entries.model.remove_all ();
		app.shortcuts.remove_action ("exec-script");
		var local_job = app.jobs.new_job ("Loading Scripts");
		try {
			var scripts_file = UserDir.get_config_file (FileIntent.READ, SCRIPTS_FILE);
			var contents = Files.load_contents (scripts_file, local_job.cancellable);
			var doc = new Dom.Document ();
			doc.load_xml (contents);
			if ((doc.first_child != null) && (doc.first_child.tag_name == "scripts")) {
				foreach (unowned var node in doc.first_child) {
					if (node.tag_name == "script") {
						var script = new Gth.Script ();
						script.load_from_element (node);
						entries.model.append (script);
						app.shortcuts.add (script.create_shortcut ());
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

	void save_to_file () {
		try {
			var doc = new Dom.Document ();
			var root = new Dom.Element.with_attributes ("scripts", "version", SCRIPT_FORMAT);
			doc.append_child (root);
			foreach (unowned var script in entries) {
				root.append_child (script.create_element (doc));
			}
			var file = UserDir.get_config_file (FileIntent.WRITE, SCRIPTS_FILE);
			Files.save_content (file, doc.to_xml ());
		}
		catch (Error error) {
		}
	}

	public Script? get_script (string id) {
		var iter = entries.iterator ();
		var pos = iter.find_first ((script) => script.id == id);
		return (pos >= 0) ? entries.get (pos) : null;
	}

	bool loaded;

	const string SCRIPT_FORMAT = "1.1";
}
