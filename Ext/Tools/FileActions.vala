public class Gth.FileActions {
	public GenericList<FileAction> entries;
	public ActionCategory actions;
	public ActionCategory tools;
	public ActionCategory images;
	public ActionCategory metadata_tools;
	public ActionCategory scripts;

	public signal void changed () {
		save_to_file ();
		app.monitor.scripts_changed ();
	}

	public FileActions () {
		entries = new GenericList<FileAction> ();
		actions = new ActionCategory ("", 0);
		images = new ActionCategory (_("Images"), 1);
		metadata_tools = new ActionCategory (_("Metadata"), 2);
		tools = new ActionCategory (_("Tools"), 3);
		scripts = new ActionCategory (_("Scripts"), 4);
		loaded = false;
	}

	public FileAction register (ToolCategory category, string action_name, string display_name, string? icon_name = null, string? default_shortcut = null) {
		var action = new FileAction ();
		action.category = category;
		action.action_name = action_name;
		action.display_name = display_name;
		action.icon_name = icon_name;
		action.default_shortcut = default_shortcut;
		entries.model.append (action);
		return action;
	}

	public bool load_from_file () {
		if (loaded) {
			return false;
		}
		var local_job = app.jobs.new_job ("Loading Tools");
		try {
			var tools_file = UserDir.get_config_file (FileIntent.READ, TOOLS_FILE);
			var contents = Files.load_contents (tools_file, local_job.cancellable);
			var doc = new Dom.Document ();
			doc.load_xml (contents);
			if ((doc.first_child != null) && (doc.first_child.tag_name == "tools")) {
				uint pos = 0;
				foreach (unowned var node in doc.first_child) {
					if (node.tag_name == "tool") {
						var action = get_action (node.get_attribute ("id"));
						if (action != null) {
							action.position = pos;
							action.load_from_element (node);
						}
					}
					pos++;
				}
				entries.sort ((a, b) => Util.uint_cmp (a.position, b.position));
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
			var root = new Dom.Element.with_attributes ("tools", "version", FILE_FORMAT);
			doc.append_child (root);
			foreach (unowned var action in entries) {
				root.append_child (action.create_element (doc));
			}
			var file = UserDir.get_config_file (FileIntent.WRITE, TOOLS_FILE);
			Files.save_content (file, doc.to_xml ());
		}
		catch (Error error) {
		}
	}

	public FileAction? get_action (string id) {
		var iter = entries.iterator ();
		var pos = iter.find_first ((script) => script.action_name == id);
		return (pos >= 0) ? entries.get (pos) : null;
	}

	bool loaded;
	const string FILE_FORMAT = "1.0";
}
